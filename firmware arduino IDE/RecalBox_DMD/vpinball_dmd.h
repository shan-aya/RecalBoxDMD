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
// Fix : ne PAS recompiler miniz, juste declarer le PROTOTYPE des symboles
// bas niveau deja presents dans le binaire et les appeler directement --
// aucune redefinition, juste un appel a des symboles existants.
//
// v3 - 2026-09-17 - safe-modify - API BAS NIVEAU au lieu du wrapper
// tinfl_decompress_mem_to_mem() : ce wrapper declare son `tinfl_decompressor
// decomp;` en variable LOCALE (donc sur la pile de loopTask), cause d'un
// vrai crash reproduit en test materiel ("Stack canary watchpoint
// triggered (loopTask)", voir DECISIONS.md) des le premier paquet
// COMPRESSE recu depuis un vrai client vpinball/libdmdutil -- decouvert
// crucial en Phase 3 (test reel avec la table Batman sur RB2) : le vrai
// client envoie ses zones EXCLUSIVEMENT compressees (compressed=1 sur 100%
// des trames cmd=4/5 observees), contrairement a notre script de test
// Phase 2 qui envoyait du non-compresse -- la compression n'est donc PAS
// un cas marginal a ignorer, elle est indispensable pour tout affichage
// reel. Correction de la documentation d'alors : la structure
// tinfl_decompressor ne fait PAS 32 Ko comme documente au moment du crash
// (confusion avec `inflate_state::m_dict[TINFL_LZ_DICT_SIZE]`, une AUTRE
// structure, propre a l'API haut niveau mz_inflate*() jamais utilisee ici)
// -- lecture directe de pngle/src/miniz.c (struct tinfl_decompressor_tag,
// ~ligne 769) donne une taille reelle d'environ 11 Ko (3 tables de Huffman
// de ~3,4 Ko chacune + une poignee de champs scalaires), toujours bien
// trop pour une variable locale a cette profondeur d'appel mais permettant
// un fix simple : la meme structure declaree en variable GLOBALE/statique
// (donc en BSS, jamais sur la pile) suffit. tinfl_decompress() (la
// fonction coeur, deja liee via pngle, appelee EN INTERNE par le wrapper)
// est utilisable directement de la meme facon que tinfl_decompress_mem_to_mem
// le fait lui-meme (memes 2 flags, meme pOut_buf_start==pOut_buf_next) --
// tinfl_init() n'est qu'une macro amont (`r->m_state = 0`), reproduite ici
// a l'identique (pas de symbole a lier, juste une ecriture de champ).
// Definitions (struct/enums) copiees verbatim depuis pngle/src/miniz.c
// (lignes ~688-776) -- types mz_uintNN remplaces par les uintNN_t
// standard strictement equivalents (memes tailles), layout memoire
// identique donc compatible avec le tinfl_decompress() deja compile.
enum {
  VPINBALL_TINFL_MAX_HUFF_TABLES   = 3,
  VPINBALL_TINFL_MAX_HUFF_SYMBOLS0 = 288,
  VPINBALL_TINFL_MAX_HUFF_SYMBOLS1 = 32,
  VPINBALL_TINFL_FAST_LOOKUP_BITS  = 10,
  VPINBALL_TINFL_FAST_LOOKUP_SIZE  = 1 << VPINBALL_TINFL_FAST_LOOKUP_BITS,
};
typedef struct {
  uint8_t m_code_size[VPINBALL_TINFL_MAX_HUFF_SYMBOLS0];
  int16_t m_look_up[VPINBALL_TINFL_FAST_LOOKUP_SIZE];
  int16_t m_tree[VPINBALL_TINFL_MAX_HUFF_SYMBOLS0 * 2];
} vpinball_tinfl_huff_table;
struct vpinball_tinfl_decompressor {
  uint32_t m_state, m_num_bits, m_zhdr0, m_zhdr1, m_z_adler32, m_final, m_type,
           m_check_adler32, m_dist, m_counter, m_num_extra,
           m_table_sizes[VPINBALL_TINFL_MAX_HUFF_TABLES];
  uint64_t m_bit_buf;
  size_t m_dist_from_out_buf_start;
  vpinball_tinfl_huff_table m_tables[VPINBALL_TINFL_MAX_HUFF_TABLES];
  uint8_t m_raw_header[4];
  uint8_t m_len_codes[VPINBALL_TINFL_MAX_HUFF_SYMBOLS0 + VPINBALL_TINFL_MAX_HUFF_SYMBOLS1 + 137];
};
typedef enum {
  VPINBALL_TINFL_STATUS_FAILED = -1,
  VPINBALL_TINFL_STATUS_DONE   = 0,
} vpinball_tinfl_status;
extern "C" int tinfl_decompress(struct vpinball_tinfl_decompressor *r,
                                 const uint8_t *pIn_buf_next, size_t *pIn_buf_size,
                                 uint8_t *pOut_buf_start, uint8_t *pOut_buf_next,
                                 size_t *pOut_buf_size, uint32_t decomp_flags);
#define VPINBALL_TINFL_FLAG_PARSE_ZLIB_HEADER 1
#define VPINBALL_TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF 4
// v7 - 2026-09-17 - safe-modify - HISTORIQUE : v5 tentait de liberer
// gifRawFrameBuf/gifRawDelayCache/raw565FullBuf EN DIRECT (systeme deja
// demarre) pour faire de la place au decompresseur (~11 Ko) -- BUG REEL
// confirme en test materiel (Phase 3, vraie table Batman/RB2) : ces buffers
// n'existent que pour le chemin "raw565pack" -- un GIF standard (le
// decodeur pngle classique) ne les alloue jamais, donc les liberer ne
// recupere RIEN dans ce cas, et le malloc echoue quand meme (confirme :
// dizaines d'echecs consecutifs en usage reel, maxalloc jamais au-dessus de
// ~15 Ko une fois GIF+WebServer actifs). Idee de l'utilisateur, bien plus
// robuste : un REBOOT DEDIE au mode vpinball plutot que d'essayer de
// grappiller de la RAM sur un systeme deja charge. Reprend un mecanisme
// DEJA EXISTANT et deja eprouve en production dans ce firmware :
// g_skipPlaylistForConfig/force_config_boot (RecalBox_DMD.ino, setup()) --
// un "reboot cible" qui saute le chargement des caches jeux/systemes ET
// l'ouverture de tout GIF, pour repartir en mode config avec un maximum de
// heap disponible (mesure reelle juste apres ce chargement de caches, tout
// autre boot : heap libre=51840 maxalloc=42996 -- tres largement suffisant
// pour nos 11 Ko). Meme principe ici via force_vpinball_boot (config.ini,
// voir le bloc g_bootForVpinball dans setup()) : le PREMIER paquet vpinball
// recu EN MODE NORMAL declenche desormais un reboot cible (voir
// triggerVpinballBootReboot() plus bas), pas un malloc en direct.
static struct vpinball_tinfl_decompressor *vpinballTinflState = nullptr;

// v3 - 2026-09-19 - safe-modify - BUG REEL (test DMD2, mode Pinball actif en
// fonctionnement normal) : les 2 buffers ci-dessous etaient des tableaux
// STATIQUES (BSS, 3,5 Ko reserves EN PERMANENCE, meme avec feat_vpinball_dmd
// desactive), pris directement sur le heap du firmware -- avec l'ecoute UDP,
// le heap tombait a ~7,6 Ko libres (maxalloc 4,6 Ko) des l'ouverture du 1er
// GIF et le serveur web cessait de repondre (joignable ~20 s apres le boot
// seulement). Ils ne servent qu'en mode vpinball (boot cible, heap ~50 Ko) :
// alloues dans enterVpinballModeFromBoot(), liberes a la sortie -- en mode
// normal, l'ecoute UDP ne lit plus qu'un en-tete de 5 octets sur la pile
// (voir pollVpinballUdp()). Tailles : voir leur historique plus bas.
#define VPINBALL_DMD_RECV_BUF_SIZE 1500
#define VPINBALL_DMD_DECOMP_BUF_SIZE 2048
static uint8_t *vpinballRecvBuf = nullptr;
static uint8_t *vpinballDecompBuf = nullptr;

// v7 -- etat du mode vpinball. Mis a true UNIQUEMENT depuis setup()
// (RecalBox_DMD.ino, bloc g_bootForVpinball) sur un boot cible reussi --
// jamais depuis loop()/vpinballProcessPacket() desormais. Timeout de
// silence -> reboot vers le mode normal (voir exitVpinballMode()) --
// heuristique inchangee, le protocole ZeDMD n'a toujours aucun signal
// explicite de fin de partie/deconnexion.
bool vpinballModeActive = false;
static unsigned long vpinballLastPacketMs = 0;
#define VPINBALL_MODE_TIMEOUT_MS 5000

// v7 -- declenche depuis vpinballProcessPacket() des qu'un paquet vpinball
// arrive alors qu'on est en mode NORMAL (pas encore en mode vpinball) :
// pose le flag config.ini (persiste a travers le reboot, meme mecanisme que
// force_config_boot) puis redemarre. Le paquet qui a declenche ce reboot
// est perdu (vpinball en envoie en continu, le prochain sera capte des le
// retour en ligne, ~10-17s plus tard le temps du boot+reconnexion WiFi).
static void triggerVpinballBootReboot()
{
  Serial.println("[VPINBALL] 1er paquet detecte en mode normal -- reboot cible vers le mode vpinball (heap max)");
  writeConfigFlag("force_vpinball_boot", "1");
  // v1 - 2026-09-17 - safe-modify - retour utilisateur : message avant le
  // redemarrage, meme si bref ici (interrompt un GIF en cours) -- le message
  // principal ("Connexion...", visible ~10-12s) est celui affiche au debut
  // du boot cible (setup(), RecalBox_DMD.ino).
  if (display) showMessage("VPINBALL", "Detecte...", display->color565(255, 165, 0));
  delay(100);
  ESP.restart();
}

// v7 -- appelee UNIQUEMENT depuis setup() (RecalBox_DMD.ino), sur le boot
// cible g_bootForVpinball -- heap encore proche de son maximum post-boot
// (caches jeux/systemes et GIF/playlist jamais charges sur ce boot precis,
// meme garde que force_config_boot). Alloue le decompresseur et active le
// mode -- si le malloc echoue MEME ICI (heap anormalement bas des le boot,
// jamais observe en test mais reste possible), le mode reste inactif et
// loop() continue normalement (pipeline GIF/playlist standard, mais SANS
// caches jeux/systemes puisqu'on est sur ce boot cible -- limite mineure
// acceptee, cas degrade improbable).
static void enterVpinballModeFromBoot()
{
  // setupVpinballDmd() n'est PAS rappelee ici : deja invoquee juste avant,
  // depuis setupWiFiFromConfig() (RecalBox_DMD.ino), des que le WiFi se
  // connecte -- ce chemin s'execute forcement APRES (vient de goto
  // start_mqtt_task, atteint apres le retour de setupWiFiFromConfig()).
  vpinballTinflState = (struct vpinball_tinfl_decompressor*)malloc(sizeof(struct vpinball_tinfl_decompressor));
  vpinballRecvBuf = (uint8_t*)malloc(VPINBALL_DMD_RECV_BUF_SIZE);
  vpinballDecompBuf = (uint8_t*)malloc(VPINBALL_DMD_DECOMP_BUF_SIZE);
  if (!vpinballTinflState || !vpinballRecvBuf || !vpinballDecompBuf)
  {
    Serial.println("[VPINBALL] echec malloc decompresseur/buffers MEME sur boot cible (heap anormalement bas) -- mode vpinball non active");
    if (vpinballTinflState) { free(vpinballTinflState); vpinballTinflState = nullptr; }
    if (vpinballRecvBuf) { free(vpinballRecvBuf); vpinballRecvBuf = nullptr; }
    if (vpinballDecompBuf) { free(vpinballDecompBuf); vpinballDecompBuf = nullptr; }
    return;
  }
  vpinballModeActive = true;
  vpinballLastPacketMs = millis();
  Serial.println("[VPINBALL] mode actif (boot cible, heap max) -- decompresseur alloue, pret a recevoir");
}

// v7 -- quitte le mode vpinball par TIMEOUT (silence prolonge) : reboot
// vers le mode normal plutot qu'une simple reprise en place -- ce boot
// cible n'a jamais charge les caches jeux/systemes ni le pipeline GIF/
// playlist (meme garde que force_config_boot), un reboot complet est
// necessaire pour les reinitialiser proprement. Le flag config.ini
// force_vpinball_boot a deja ete consomme (remis a "0") a l'entree de ce
// boot cible (RecalBox_DMD.ino) -- ce redemarrage repart donc bien en boot
// normal standard, pas en boucle sur le mode vpinball.
static void exitVpinballMode()
{
  if (!vpinballModeActive) return;
  Serial.println("[VPINBALL] timeout -- reboot vers le mode normal (playlist/GIF/caches)");
  if (vpinballTinflState) { free(vpinballTinflState); vpinballTinflState = nullptr; }
  if (vpinballRecvBuf) { free(vpinballRecvBuf); vpinballRecvBuf = nullptr; }
  if (vpinballDecompBuf) { free(vpinballDecompBuf); vpinballDecompBuf = nullptr; }
  // v1 - 2026-09-17 - safe-modify - retour utilisateur : message visible
  // avant le redemarrage (symetrique du "VPINBALL - Connexion..." affiche a
  // l'entree, voir setup() dans RecalBox_DMD.ino). 600ms au lieu des 100ms
  // d'origine -- juste assez pour etre lisible, negligeable face aux ~2,4s
  // deja recuperes sur ce boot en sautant les ecrans de statut bluetooth/
  // WIFI OK/NTP (showMessage() deja disponible, aucune dependance
  // supplementaire).
  if (display) showMessage("VPINBALL", "Retour normal...", display->color565(255, 165, 0));
  delay(600);
  ESP.restart();
}

// Geometrie des zones -- formule identique a ZeDMD (panel.h), verifiee sur
// notre panneau 128x32 : 16 zones/ligne x 8 lignes = 128 zones de 8x4 px.
#define VPINBALL_DMD_ZONE_WIDTH    (VPINBALL_DMD_TOTAL_WIDTH / 16)
#define VPINBALL_DMD_ZONE_HEIGHT   (VPINBALL_DMD_TOTAL_HEIGHT / 8)
#define VPINBALL_DMD_ZONES_PER_ROW (VPINBALL_DMD_TOTAL_WIDTH / VPINBALL_DMD_ZONE_WIDTH)
#define VPINBALL_DMD_NUM_ZONES     (VPINBALL_DMD_ZONES_PER_ROW * (VPINBALL_DMD_TOTAL_HEIGHT / VPINBALL_DMD_ZONE_HEIGHT))
#define VPINBALL_DMD_ZONE_SIZE_888 (VPINBALL_DMD_ZONE_WIDTH * VPINBALL_DMD_ZONE_HEIGHT * 3)
#define VPINBALL_DMD_ZONE_SIZE_565 (VPINBALL_DMD_ZONE_WIDTH * VPINBALL_DMD_ZONE_HEIGHT * 2)

// Taille max d'un paquet UDP recu (marge sous la MTU Ethernet/WiFi usuelle).
// (VPINBALL_DMD_RECV_BUF_SIZE = 1500, defini plus haut.)
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
// (VPINBALL_DMD_DECOMP_BUF_SIZE = 2048, defini plus haut.)

WiFiUDP vpinballUdp;
bool    vpinballUdpStarted = false;

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
      const uint8_t *zoneData = payload;
      size_t zoneDataLen = payloadSize;
      if (compressed)
      {
        // v4 - 2026-09-17 - safe-modify - decompression REELLEMENT
        // necessaire : test Phase 3 sur RB2 (vraie table Batman) montre que
        // le client reel (plugin DMDUtil de VPX) envoie 100% de ses zones
        // COMPRESSEES -- voir explication complete pres de la declaration
        // de vpinballTinflState plus haut (API bas niveau tinfl_decompress()
        // + etat global, evite le crash pile de l'ancien wrapper).
        if (!vpinballTinflState || !vpinballDecompBuf)
        {
          // malloc a echoue a l'entree en mode vpinball (heap trop bas a cet
          // instant precis) -- trame ignoree proprement, pas de crash.
          return;
        }
        size_t inLen = payloadSize;
        size_t outLen = VPINBALL_DMD_DECOMP_BUF_SIZE;
        vpinballTinflState->m_state = 0; // tinfl_init(), macro amont reproduite ici
        const int status = tinfl_decompress(
            vpinballTinflState,
            payload, &inLen,
            vpinballDecompBuf, vpinballDecompBuf, &outLen,
            VPINBALL_TINFL_FLAG_PARSE_ZLIB_HEADER | VPINBALL_TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);
        if (status != VPINBALL_TINFL_STATUS_DONE)
        {
          Serial.println("[VPINBALL] erreur decompression (status=" + String(status) + ")");
          return;
        }
        zoneData = vpinballDecompBuf;
        zoneDataLen = outLen;
      }
      vpinballDecodeZones(zoneData, zoneDataLen, isRgb565);
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
  // v7 -- 1er paquet ZeDMD-WiFi recu EN MODE NORMAL = reboot cible vers le
  // mode vpinball (voir triggerVpinballBootReboot()) -- ne redemarre PAS si
  // deja en mode vpinball (boot cible reussi), chaque paquet repousse alors
  // simplement le timeout d'inactivite verifie dans pollVpinballUdp().
  if (!vpinballModeActive) { triggerVpinballBootReboot(); return; }
  vpinballLastPacketMs = millis();
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
  // v5 -- sortie du mode vpinball apres un silence prolonge (voir
  // enterVpinballMode()/exitVpinballMode() -- protocole ZeDMD sans signal
  // explicite de fin de partie, timeout = seule heuristique disponible).
  if (vpinballModeActive && (millis() - vpinballLastPacketMs > VPINBALL_MODE_TIMEOUT_MS))
  {
    exitVpinballMode();
  }
  int packetSize;
  const int MAX_DRAIN_PER_CALL = 10;
  int drained = 0;
  while ((packetSize = vpinballUdp.parsePacket()) > 0 && drained < MAX_DRAIN_PER_CALL)
  {
    drained++;
    if (!vpinballModeActive || !vpinballRecvBuf)
    {
      // v3 -- mode NORMAL : aucun buffer de reception (voir sa declaration).
      // On lit seulement l'en-tete de 5 octets ("ZeDMD") sur la pile : un
      // vrai paquet ZeDMD-WiFi declenche le reboot cible, tout autre paquet
      // parasite sur ce port est ignore (avant : n'importe quel paquet
      // declenchait le reboot).
      uint8_t hdr[5];
      const int n = vpinballUdp.read(hdr, sizeof(hdr));
      vpinballUdp.flush();
      if (n == (int)sizeof(hdr) && memcmp(hdr, VPINBALL_SYNC, sizeof(hdr)) == 0 && !vpinballModeActive)
        triggerVpinballBootReboot();
      continue;
    }
    const int len = vpinballUdp.read(vpinballRecvBuf, VPINBALL_DMD_RECV_BUF_SIZE);
    if (len <= 0) continue;
    vpinballProcessPacket(vpinballRecvBuf, len);
  }
}
