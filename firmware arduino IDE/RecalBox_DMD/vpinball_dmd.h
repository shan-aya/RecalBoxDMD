// ============================================
// vpinball_dmd.h — Phase 2 integration vpinball/libdmdutil
//
// safe-modify — Historique des modifications
// ============================================
// Version actuelle : v1
//
// v1 - 2026-09-16 - safe-modify - Canal de donnees ZeDMD-WiFi (voir
// DECISIONS.md "Nouveau chantier -- integration vpinball", worktree
// dev/vpinball-integration, Phase 1 = web_config.h::handleVpinballHandshake).
// Ecoute UDP sur VPINBALL_DMD_UDP_PORT, decode le protocole binaire ZeDMD
// (sync "ZeDMD" + header 4o + payload optionnellement compresse zlib/miniz)
// et affiche les zones recues via drawPixel(). Format verifie dans 2 sources
// reelles : generateur ZeDMD (main.cpp::HandleData(), PPUC/ZeDMD) et
// consommateur libzedmd (utilise par libdmdutil/Visual Pinball Standalone).
//
// Hypothese de simplification actee (a re-verifier si le decodage semble
// incorrect en test reel) : un paquet UDP contient un ou plusieurs MESSAGES
// COMPLETS (jamais un message coupe entre 2 paquets) -- HandleUdpPacket() de
// ZeDMD lui-meme appelle HandleData() avec un packet.data()/packet.length()
// complet, sans etat de fragmentation persistant entre paquets UDP distincts
// (contrairement au chemin TCP, qui lui accumule un flux). Simplifie
// enormement l'implementation (pas de machine a etats inter-paquets a
// maintenir) au prix de cette hypothese, raisonnable pour de l'UDP.
//
// N'est actif QUE si featVpinballDmd est active (config.ini) -- voir
// setupVpinballDmd()/pollVpinballUdp(), appeles depuis RecalBox_DMD.ino
// uniquement sous garde du flag.
//
// Decompression : PAS de copie vendoree de miniz ici -- la bibliotheque
// pngle (deja utilisee par ce firmware pour le PNG) embarque SA PROPRE
// copie complete de miniz.c, deja compilee et liee dans le binaire final.
// Une 2e copie provoque un conflit d'edition de liens ("multiple
// definition of mz_adler32/tinfl_decompress/...", constate en compilant).
// Fix : ne PAS recompiler miniz, juste declarer le PROTOTYPE de la
// fonction bas niveau deja presente dans le binaire (tinfl_decompress_mem_to_mem,
// signature copiee telle quelle depuis pngle/src/miniz.c) et l'appeler
// directement -- aucune redefinition, juste un appel a un symbole existant.
// TINFL_FLAG_PARSE_ZLIB_HEADER=1 (meme fichier) : nos payloads sont
// compresses au format zlib standard (2 octets d'en-tete + adler32 en
// fin de flux, PAS du deflate brut comme le PNG) -- flag necessaire pour
// que le decodeur saute l'en-tete zlib avant le flux deflate lui-meme.
extern "C" size_t tinfl_decompress_mem_to_mem(void *pOut_buf, size_t out_buf_len,
                                              const void *pSrc_buf, size_t src_buf_len,
                                              int flags);
#define VPINBALL_TINFL_FLAG_PARSE_ZLIB_HEADER 1
#define VPINBALL_TINFL_DECOMPRESS_FAILED ((size_t)(-1))

// Geometrie des zones -- formule identique a ZeDMD (panel.h), verifiee sur
// notre panneau 128x32 : 16 zones/ligne x 8 lignes = 128 zones de 8x4 px.
#define VPINBALL_DMD_ZONE_WIDTH    (VPINBALL_DMD_TOTAL_WIDTH / 16)
#define VPINBALL_DMD_ZONE_HEIGHT   (VPINBALL_DMD_TOTAL_HEIGHT / 8)
#define VPINBALL_DMD_ZONES_PER_ROW (VPINBALL_DMD_TOTAL_WIDTH / VPINBALL_DMD_ZONE_WIDTH)
#define VPINBALL_DMD_NUM_ZONES     (VPINBALL_DMD_ZONES_PER_ROW * (VPINBALL_DMD_TOTAL_HEIGHT / VPINBALL_DMD_ZONE_HEIGHT))
#define VPINBALL_DMD_ZONE_SIZE_888 (VPINBALL_DMD_ZONE_WIDTH * VPINBALL_DMD_ZONE_HEIGHT * 3)
#define VPINBALL_DMD_ZONE_SIZE_565 (VPINBALL_DMD_ZONE_WIDTH * VPINBALL_DMD_ZONE_HEIGHT * 2)

// Taille max d'un paquet UDP recu (marge sous la MTU Ethernet/WiFi usuelle).
#define VPINBALL_DMD_RECV_BUF_SIZE 1500
// v2 - 2026-09-16 - safe-modify - BUG REEL trouve en test materiel : un
// buffer initialement dimensionne au pire cas THEORIQUE (128 zones x 97
// octets = 12416, arrondi 12800) reduisait le heap dispo au bilan STATIQUE
// (BSS, reserve en permanence, pas seulement pendant l'usage) au point de
// faire echouer une allocation `operator new` PENDANT setupWebConfig()
// (WebServer::on() alloue un std::function sur le tas) -- crash reproduit
// (abort()/std::bad_alloc) UNIQUEMENT avec ce buffer present, absent sans
// (isole par comparaison directe avec la Phase 1 seule, voir DECISIONS.md).
// Fix : 2048 octets, MEME taille que ZeDMD lui-meme utilise en pratique
// pour son propre uncompressBuffer (main.cpp, valeur fixe quelle que soit
// la taille du panneau reel) -- une mise a jour realiste ne touche jamais
// les 128 zones a la fois d'un coup. Limite acceptee : si une trame
// legitimement plus grosse arrive, mz/tinfl echoue proprement (retour
// FAILED, log, frame ignoree) plutot que de crasher -- meme compromis que
// la reference elle-meme.
#define VPINBALL_DMD_DECOMP_BUF_SIZE 2048

WiFiUDP vpinballUdp;
bool    vpinballUdpStarted = false;
uint8_t vpinballRecvBuf[VPINBALL_DMD_RECV_BUF_SIZE];
uint8_t vpinballDecompBuf[VPINBALL_DMD_DECOMP_BUF_SIZE];

static const uint8_t VPINBALL_SYNC[5] = {'Z', 'e', 'D', 'M', 'D'};

// v1 -- demarre l'ecoute UDP. Appelee depuis setup() UNIQUEMENT si
// featVpinballDmd est actif (voir RecalBox_DMD.ino) -- ne consomme aucune
// ressource sinon.
void setupVpinballDmd()
{
  if (!featVpinballDmd) return;
  if (vpinballUdpStarted) return;
  if (vpinballUdp.begin(VPINBALL_DMD_UDP_PORT))
  {
    vpinballUdpStarted = true;
    Serial.println("[VPINBALL] ecoute UDP demarree sur le port " + String(VPINBALL_DMD_UDP_PORT));
  }
  else
  {
    Serial.println("[VPINBALL] echec demarrage UDP");
  }
}

// v1 -- remplit ou efface une zone a l'ecran. zoneIdx deja valide (< NUM_ZONES)
// par l'appelant. pix == nullptr => efface la zone (noir).
static void vpinballRenderZone(uint8_t zoneIdx, bool isRgb565, const uint8_t *pix)
{
  if (!display) return;
  const int xOffset = (zoneIdx % VPINBALL_DMD_ZONES_PER_ROW) * VPINBALL_DMD_ZONE_WIDTH;
  const int yOffset = (zoneIdx / VPINBALL_DMD_ZONES_PER_ROW) * VPINBALL_DMD_ZONE_HEIGHT;

  if (!pix)
  {
    for (int y = 0; y < VPINBALL_DMD_ZONE_HEIGHT; y++)
      for (int x = 0; x < VPINBALL_DMD_ZONE_WIDTH; x++)
        display->drawPixel(xOffset + x, yOffset + y, 0);
    return;
  }

  int p = 0;
  for (int y = 0; y < VPINBALL_DMD_ZONE_HEIGHT; y++)
  {
    for (int x = 0; x < VPINBALL_DMD_ZONE_WIDTH; x++)
    {
      uint8_t r, g, b;
      if (isRgb565)
      {
        // Little-endian, meme convention que ZeDMD (main.cpp, decode zone).
        const uint16_t v = pix[p] | ((uint16_t)pix[p + 1] << 8);
        p += 2;
        r = (uint8_t)(((v >> 11) & 0x1f) * 255 / 31);
        g = (uint8_t)(((v >> 5) & 0x3f) * 255 / 63);
        b = (uint8_t)((v & 0x1f) * 255 / 31);
      }
      else
      {
        r = pix[p]; g = pix[p + 1]; b = pix[p + 2];
        p += 3;
      }
      display->drawPixel(xOffset + x, yOffset + y, display->color565(r, g, b));
    }
  }
}

// v1 -- decode une trame de zones DEJA DECOMPRESSEE (voir format documente
// dans DECISIONS.md : suite d'enregistrements, chacun 1 octet d'index puis,
// si idx<128, les pixels de la zone -- idx>=128 = effacer la zone idx-128).
static void vpinballDecodeZones(const uint8_t *buf, size_t len, bool isRgb565)
{
  const size_t zoneDataSize = isRgb565 ? VPINBALL_DMD_ZONE_SIZE_565 : VPINBALL_DMD_ZONE_SIZE_888;
  size_t pos = 0;
  while (pos < len)
  {
    const uint8_t raw = buf[pos++];
    if (raw >= 128)
    {
      const uint8_t zoneIdx = raw - 128;
      if (zoneIdx < VPINBALL_DMD_NUM_ZONES) vpinballRenderZone(zoneIdx, isRgb565, nullptr);
      continue;
    }
    const uint8_t zoneIdx = raw;
    if (pos + zoneDataSize > len) break; // trame tronquee, on s'arrete proprement
    if (zoneIdx < VPINBALL_DMD_NUM_ZONES) vpinballRenderZone(zoneIdx, isRgb565, &buf[pos]);
    pos += zoneDataSize;
  }
}

// v1 -- traite UN message complet (deja localise/verifie par le scanner
// d'appelant) : command + payload brut (encore compresse le cas echeant).
static void vpinballHandleCommand(uint8_t command, const uint8_t *payload, uint16_t payloadSize, bool compressed)
{
  switch (command)
  {
    case 4:   // RGB888 Zones Stream
    case 5:   // RGB565 Zones Stream
    {
      const bool isRgb565 = (command == 5);
      if (compressed)
      {
        // v3 - 2026-09-16 - safe-modify - BUG REEL trouve en test materiel :
        // tinfl_decompress_mem_to_mem() alloue son tinfl_decompressor (donc
        // son dictionnaire LZ77 TINFL_LZ_DICT_SIZE=32768 octets) en variable
        // LOCALE -- sur la PILE de loopTask, PAS sur le tas. Reproduit en
        // reel : "Guru Meditation Error... Stack canary watchpoint triggered
        // (loopTask)" des le premier paquet compresse envoye, backtrace
        // confirme dans tinfl_decompress <- tinfl_decompress_mem_to_mem <-
        // pollVpinballUdp() <- loop(). Aucune pile de ce firmware (loopTask
        // standard Arduino-ESP32) ne peut absorber 32 Ko d'un coup.
        // Fix retenu POUR CETTE PHASE : ignorer proprement les trames
        // compressees plutot que de risquer ce crash -- les trames NON
        // compressees (validees en reel, 4 zones colorees correctement
        // decodees et affichees) restent pleinement fonctionnelles. Lever
        // cette limite necessiterait soit (a) une tache FreeRTOS DEDIEE
        // avec sa propre pile >=40 Ko rien que pour la decompression, soit
        // (b) heap-allouer tinfl_decompressor (mais 32 Ko d'un coup sur un
        // heap deja tendu -- voir crash operator new/WebServer::on() plus
        // haut dans ce fichier -- reintroduirait probablement ce risque) :
        // aucune des deux tentee ici, a evaluer si un besoin reel de
        // compression se confirme (libdmdutil peut tres bien fonctionner
        // en non-compresse pour un affichage aussi simple qu'un DMD 128x32).
        Serial.println("[VPINBALL] trame compressee ignoree (decompression non supportee sur ce firmware, voir DECISIONS.md)");
        return;
      }
      vpinballDecodeZones(payload, payloadSize, isRgb565);
      break;
    }
    case 6:   // Render -- rien a faire, chaque zone est deja dessinee
              // directement (pas de double-buffering cote firmware ici,
              // contrairement a ZeDMD -- a revisiter si un scintillement
              // est observe en test reel).
      break;
    case 10:  // Clear screen
      if (display) display->clearScreen();
      break;
    case 11:  // KeepAlive -- rien a faire pour l'instant, pas de timeout
              // de connexion implemente cote firmware (a ajouter si utile).
      break;
    case 22:  // set brightness -- 1 octet, echelle 0-15 cote client ZeDMD.
      if (payloadSize >= 1 && display)
      {
        // v1 -- conversion grossiere 0-15 (echelle ZeDMD) -> 0-255 (echelle
        // native screenBrightness) : a affiner si le rendu est trop
        // sombre/clair en test reel.
        const uint8_t lvl = payload[0] > 15 ? 15 : payload[0];
        display->setBrightness8((uint8_t)(lvl * 255 / 15));
      }
      break;
    default:
      // Commandes USB/menu de reglages ZeDMD (12, 23, 26-99...) : sans
      // objet sur notre canal WiFi minimal, ignorees silencieusement.
      break;
  }
}

// v1 -- scanne un buffer (un paquet UDP recu en une fois) a la recherche
// d'un ou plusieurs messages complets consecutifs (voir hypothese de
// simplification en tete de fichier). Une synchro non trouvee a une
// position ne fait PAS avancer d'un seul octet a chaque essai (couteux) --
// recherche directe de la sequence sur le reste du buffer.
static void vpinballProcessPacket(const uint8_t *buf, int len)
{
  int pos = 0;
  while (pos + 5 <= len)
  {
    if (memcmp(&buf[pos], VPINBALL_SYNC, 5) != 0) { pos++; continue; }
    pos += 5;
    if (pos + 4 > len) break; // header incomplet, message tronque -- abandon
    const uint8_t command = buf[pos++];
    const uint16_t payloadSize = ((uint16_t)buf[pos] << 8) | buf[pos + 1];
    pos += 2;
    const bool compressed = buf[pos++] != 0;
    if (pos + payloadSize > len) break; // payload tronque -- abandon
    vpinballHandleCommand(command, &buf[pos], payloadSize, compressed);
    pos += payloadSize;
  }
}

// v1 -- a appeler depuis loop() a chaque iteration (no-op si le flag est
// desactive ou si begin() a echoue) -- meme convention que le drainage
// dmdUdp existant plus haut dans ce fichier.
void pollVpinballUdp()
{
  if (!vpinballUdpStarted) return;
  int packetSize;
  const int MAX_DRAIN_PER_CALL = 10;
  int drained = 0;
  while ((packetSize = vpinballUdp.parsePacket()) > 0 && drained < MAX_DRAIN_PER_CALL)
  {
    drained++;
    const int len = vpinballUdp.read(vpinballRecvBuf, sizeof(vpinballRecvBuf));
    if (len <= 0) continue;
    vpinballProcessPacket(vpinballRecvBuf, len);
  }
}
