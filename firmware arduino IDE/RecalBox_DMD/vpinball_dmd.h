// ============================================
// vpinball_dmd.h — Phase 2 integration vpinball/libdmdutil
//
// safe-modify — Historique des modifications
// ============================================
// Version actuelle : v11 (l'en-tete listait encore v1 ; les v2-v7 sont decrites
// a leurs emplacements dans le fichier et dans DECISIONS.md)
//
// v11 - 2026-09-20 - safe-modify - BUG REEL CORRIGE : REASSEMBLAGE des messages ZeDMD coupes entre 2 datagrammes (voir vpinballCarryBuf). Cause des fonds
// colores en surimpression / textes absents sur les tables colorisees (Serum). Stats : recolles / perdus.
//
// v10 - 2026-09-20 - safe-modify - Stats : + commandes Render (6), Clear (10), zones videes (idx>=128), flux RGB565/RGB888 recus -- pour comprendre
// pourquoi des fonds colorises (Serum) ne se rafraichiraient pas correctement (le firmware dessine chaque zone tout de suite, sans double tampon).
//
// v9 - 2026-09-20 - safe-modify - Mesures du mode Pinball : ligne [VPINBALL] stats toutes
// les 5 s (paquets, zones, octets, erreurs inflate, datagrammes tronques, temps
// de traitement max, intervalle max entre 2 passages de boucle, RSSI, heap) --
// diagnostic d'un affichage glitche/lent/surimprime constate sur la table Diner.
// Aucun changement de comportement d'affichage.
//
// v8 - 2026-09-20 - safe-modify - Exploration tables "score seul" : une
// commande ZeDMD non geree (default de vpinballHandleCommand) est desormais
// loggee une fois par valeur (cmd/size/compressee) au lieu d'etre ignoree
// silencieusement. Aucun changement de comportement d'affichage.
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
// v4 - 2026-09-19 - safe-modify - MODE PINBALL SANS REBOOT. Le decompresseur
// miniz/tinfl (etat de ~11 Ko, un seul bloc contigu) est REMPLACE par
// mini_inflate.h (~2 Ko de pile, aucune allocation) : impossible d'allouer
// 11 Ko d'un bloc en regime etabli (heap fragmente, maxalloc 4,6 Ko mesure
// sur DMD2), d'ou le reboot dedie a chaque lancement de table des v5-v7.
// Il ne reste que 2 buffers (reception 1500 o + sortie decompressee 2048 o)
// alloues UNE FOIS au demarrage (voir setupVpinballDmd(), quand le heap est
// encore sain) ; le 1er paquet ZeDMD valide bascule en mode vpinball EN PLACE
// (le pipeline GIF/playlist est simplement suspendu), sans perdre ce paquet,
// et le silence prolonge reprend l'affichage normal via webDmdResume() (hello
// UDP + resynchronisation depuis la Recalbox). Historique du reboot dedie
// (force_vpinball_boot, boot cible g_bootForVpinball) : conserve comme REPLI
// si l'allocation des 2 buffers echoue au demarrage (heap anormalement bas).
#define VPINBALL_DMD_RECV_BUF_SIZE 1500
#define VPINBALL_DMD_DECOMP_BUF_SIZE 2048
static uint8_t *vpinballRecvBuf = nullptr;
static uint8_t *vpinballDecompBuf = nullptr;

bool vpinballModeActive = false;
static unsigned long vpinballLastPacketMs = 0;
static uint32_t vpinballPktTotal = 0; // diagnostic
#define VPINBALL_MODE_TIMEOUT_MS 5000

// Repli : 1er paquet en mode NORMAL sans buffers alloues -> reboot cible vers
// le mode vpinball (mecanisme v7, force_vpinball_boot dans config.ini).
static void triggerVpinballBootReboot()
{
  Serial.println("[VPINBALL] buffers absents -- repli : reboot cible vers le mode vpinball");
  writeConfigFlag("force_vpinball_boot", "1");
  if (display) showMessage("VPINBALL", "Detecte...", display->color565(255, 165, 0));
  delay(100);
  ESP.restart();
}

// Boot cible (repli) : les buffers existent deja (setupVpinballDmd()), il ne
// reste qu'a activer le mode.
static void enterVpinballModeFromBoot()
{
  if (!vpinballRecvBuf || !vpinballDecompBuf)
  {
    Serial.println("[VPINBALL] boot cible : buffers absents -- mode vpinball non active");
    return;
  }
  vpinballModeActive = true;
  vpinballLastPacketMs = millis();
  Serial.println("[VPINBALL] mode actif (boot cible)");
}

// Bascule EN PLACE (sans reboot) au 1er paquet ZeDMD valide.
static void enterVpinballModeInPlace()
{
  vpinballModeActive = true;
  vpinballLastPacketMs = millis();
  if (display) display->clearScreen();
  Serial.println("[VPINBALL] mode actif (en place, sans reboot) -- pipeline GIF/playlist suspendu");
}

// v5 -- sortie IMMEDIATE demandee par la Recalbox (marquee.sh envoie
// "CMD=vpinball_end" a l'evenement ES endgame d'une table vpinball) : le mode
// est desactive sans webDmdResume() -- les commandes que marquee.sh envoie
// juste apres (ingame 0, system vpinball) restaurent l'affichage normal.
// Fenetre de 3 s pendant laquelle un paquet ZeDMD retardataire (le client
// peut envoyer un dernier "clear" en quittant) ne re-declenche pas le mode.
static unsigned long vpinballNoEnterUntilMs = 0;
void vpinballExitFromRecalbox()
{
  if (!vpinballModeActive) return;
  vpinballModeActive = false;
  vpinballNoEnterUntilMs = millis() + 3000;
  if (display) { display->clearScreen(); display->setBrightness8(screenBrightness); } // le flux ZeDMD peut avoir change la luminosite (cmd 22)
  Serial.println("[VPINBALL] fin de partie (Recalbox) -- retour immediat a l'affichage normal");
}

// Sortie par TIMEOUT (silence prolonge, le protocole ZeDMD n'a pas de signal
// explicite de fin de partie) : reprend l'affichage normal SANS reboot.
static void exitVpinballMode()
{
  if (!vpinballModeActive) return;
  Serial.println("[VPINBALL] timeout -- retour a l'affichage normal (sans reboot)");
  vpinballModeActive = false;
  if (display) { display->clearScreen(); display->setBrightness8(screenBrightness); } // voir vpinballExitFromRecalbox()
  webDmdResume();
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

// v9 -- mesures du mode Pinball (log toutes les 5 s tant que le mode est actif) :
// retour utilisateur sur une table (Diner) : ecran 'glitche', plus lent que la
// table, textes en surimpression -- sans mesure impossible de dire si c'est le
// WiFi (paquets perdus -> zones restees perimees), le decodage (trop lent) ou
// une boucle trop peu frequente. Compteurs simples, aucune allocation.
static uint32_t vpinballStatPkts = 0, vpinballStatZones = 0, vpinballStatBytes = 0;
static uint32_t vpinballStatInflateErr = 0, vpinballStatTruncated = 0;
static uint32_t vpinballStatRenders = 0, vpinballStatClears = 0, vpinballStatZoneClears = 0, vpinballStatRgb565 = 0, vpinballStatRgb888 = 0;
static uint32_t vpinballStatMaxHandleMs = 0, vpinballStatMaxGapMs = 0;
static unsigned long vpinballStatLastLogMs = 0, vpinballStatLastPollMs = 0;
static uint32_t vpinballStatSplitDone = 0, vpinballStatSplitLost = 0;

// v11 -- REASSEMBLAGE des messages coupes entre 2 datagrammes. BUG REEL trouve
// par capture cote Recalbox (paquets envoyes decodes sur PC, table Diner
// coloree) : le client (libzedmd) remplit chaque datagramme jusqu'a 1400 octets
// et COUPE un message au milieu ; la suite arrive dans le datagramme suivant,
// SANS prefixe "FRAME" ni "ZeDMD" (28 coupures sur 1207 datagrammes en 148 s).
// L'ancien code traitait chaque datagramme isolement : message tronque jete +
// suite ignoree comme "bruit" -> toutes les zones de ce message (les gros fonds
// colores !) jamais dessinees -> fond en surimpression / textes absents
// ("WINNER", score avant "REPLAY"), alors que les petits ecrans (CREDIT 0
// INSERT COIN) s'affichaient parfaitement. Un message en cours est accumule
// ici puis traite des qu'il est complet.
#define VPINBALL_DMD_CARRY_BUF_SIZE 2400
static uint8_t *vpinballCarryBuf = nullptr;
static bool     vpinballCarryActive = false;
static uint16_t vpinballCarryHave = 0, vpinballCarrySize = 0;
static uint8_t  vpinballCarryCmd = 0;
static bool     vpinballCarryComp = false;
static unsigned long vpinballCarryMs = 0;
static const uint8_t VPINBALL_SYNC[5] = {'Z', 'e', 'D', 'M', 'D'};
static const uint8_t VPINBALL_FRAME[5] = {'F', 'R', 'A', 'M', 'E'}; // prefixe de datagramme (libzedmd recent)

// v1 -- demarre l'ecoute UDP. Appelee depuis setup() UNIQUEMENT si
// featVpinballDmd est actif (voir RecalBox_DMD.ino) -- ne consomme aucune
// ressource sinon.
void setupVpinballDmd()
{
  if (!featVpinballDmd) return;
  if (vpinballUdpStarted) return;
  // v4 -- buffers alloues UNE FOIS ici (heap encore sain au boot) : voir le
  // commentaire "MODE PINBALL SANS REBOOT" plus haut. Echec -> repli reboot.
  if (!vpinballRecvBuf) vpinballRecvBuf = (uint8_t*)malloc(VPINBALL_DMD_RECV_BUF_SIZE);
  if (!vpinballDecompBuf) vpinballDecompBuf = (uint8_t*)malloc(VPINBALL_DMD_DECOMP_BUF_SIZE);
  if (!vpinballCarryBuf) vpinballCarryBuf = (uint8_t*)malloc(VPINBALL_DMD_CARRY_BUF_SIZE); // v11 -- absent = pas de reassemblage (ancien comportement)
  if (!vpinballRecvBuf || !vpinballDecompBuf)
    Serial.println("[VPINBALL] echec malloc buffers -- repli reboot cible");
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
  vpinballStatZones++;
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
      vpinballStatZoneClears++;
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
      if (isRgb565) vpinballStatRgb565++; else vpinballStatRgb888++;
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
        if (!vpinballDecompBuf) return;
        size_t outLen = VPINBALL_DMD_DECOMP_BUF_SIZE;
        const int status = minf_zlib_inflate(vpinballDecompBuf, &outLen, payload, payloadSize);
        if (status != 0)
        {
          Serial.println("[VPINBALL] erreur decompression (status=" + String(status) + ")");
          vpinballStatInflateErr++;
          return;
        }
        zoneData = vpinballDecompBuf;
        zoneDataLen = outLen;
      }
      vpinballDecodeZones(zoneData, zoneDataLen, isRgb565);
      break;
    }
    case 6:   // Render -- rien a faire, chaque zone est deja dessinee
      vpinballStatRenders++;
              // directement (pas de double-buffering cote firmware ici,
              // contrairement a ZeDMD -- a revisiter si un scintillement
              // est observe en test reel).
      break;
    case 10:  // Clear screen
      vpinballStatClears++;
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
    {
      // Commandes USB/menu de reglages ZeDMD (12, 23, 26-99...) : sans
      // objet sur notre canal WiFi minimal, ignorees. v8 : chaque valeur
      // inconnue est LOGGUEE UNE FOIS (exploration tables score seul / 256x64 :
      // savoir quelles commandes le client envoie reellement et qu'on ne gere
      // pas encore) -- un bitmap de 256 bits, aucun cout d'allocation.
      static uint8_t s_seenUnknownCmd[32] = {0};
      if (!(s_seenUnknownCmd[command >> 3] & (1 << (command & 7))))
      {
        s_seenUnknownCmd[command >> 3] |= (1 << (command & 7));
        Serial.println("[VPINBALL] commande ZeDMD non geree cmd=" + String(command) + " size=" + String(payloadSize) + " compressee=" + String(compressed ? 1 : 0));
      }
      break;
    }
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
  if (!vpinballModeActive)
  {
    if ((long)(millis() - vpinballNoEnterUntilMs) < 0) return; // paquet retardataire apres vpinball_end
    enterVpinballModeInPlace();
  }
  vpinballLastPacketMs = millis();
  int pos = 0;
  // v11 -- suite d'un message coupe par le datagramme precedent (voir plus haut).
  if (vpinballCarryActive)
  {
    if ((len >= 5 && memcmp(buf, VPINBALL_FRAME, 5) == 0) || (millis() - vpinballCarryMs) > 300)
    {
      vpinballCarryActive = false; // le datagramme attendu s'est perdu : on abandonne ce message
      vpinballStatSplitLost++;
    }
    else
    {
      int take = (int)(vpinballCarrySize - vpinballCarryHave);
      if (take > len) take = len;
      memcpy(vpinballCarryBuf + vpinballCarryHave, buf, take);
      vpinballCarryHave += take;
      pos = take;
      if (vpinballCarryHave >= vpinballCarrySize)
      {
        vpinballCarryActive = false;
        vpinballStatSplitDone++;
        vpinballHandleCommand(vpinballCarryCmd, vpinballCarryBuf, vpinballCarrySize, vpinballCarryComp);
      }
    }
  }
  while (pos + 5 <= len)
  {
    if (memcmp(&buf[pos], VPINBALL_SYNC, 5) != 0) { pos++; continue; }
    pos += 5;
    if (pos + 4 > len) { vpinballStatSplitLost++; break; } // en-tete lui-meme coupe -- non gere
    const uint8_t command = buf[pos++];
    const uint16_t payloadSize = ((uint16_t)buf[pos] << 8) | buf[pos + 1];
    pos += 2;
    const bool compressed = buf[pos++] != 0;
    if (pos + payloadSize > len)
    {
      // payload coupe en fin de datagramme : on garde le debut, la suite est attendue
      if (vpinballCarryBuf && payloadSize <= VPINBALL_DMD_CARRY_BUF_SIZE)
      {
        const int have = len - pos;
        memcpy(vpinballCarryBuf, &buf[pos], have);
        vpinballCarryHave = (uint16_t)have;
        vpinballCarrySize = payloadSize;
        vpinballCarryCmd = command;
        vpinballCarryComp = compressed;
        vpinballCarryMs = millis();
        vpinballCarryActive = true;
      }
      else vpinballStatSplitLost++;
      break;
    }    vpinballHandleCommand(command, &buf[pos], payloadSize, compressed);
    pos += payloadSize;
  }
}

// v1 -- a appeler depuis loop() a chaque iteration (no-op si le flag est
// desactive ou si begin() a echoue) -- meme convention que le drainage
// dmdUdp existant plus haut dans ce fichier.
void pollVpinballUdp()
{
  if (!vpinballUdpStarted) return;
  if (vpinballModeActive)
  {
    const unsigned long nowStat = millis();
    if (vpinballStatLastPollMs != 0)
    {
      const uint32_t gap = (uint32_t)(nowStat - vpinballStatLastPollMs);
      if (gap > vpinballStatMaxGapMs) vpinballStatMaxGapMs = gap;
    }
    vpinballStatLastPollMs = nowStat;
    if (nowStat - vpinballStatLastLogMs >= 5000)
    {
      vpinballStatLastLogMs = nowStat;
      Serial.printf("[VPINBALL] stats 5s: pkts=%u zones=%u octets=%u errInflate=%u tronques=%u maxTraitement=%ums maxIntervalleLoop=%ums rssi=%d heap=%u rendus=%u effacements=%u zonesVidees=%u rgb565=%u rgb888=%u recolles=%u perdus=%u\n",
        (unsigned)vpinballStatPkts, (unsigned)vpinballStatZones, (unsigned)vpinballStatBytes, (unsigned)vpinballStatInflateErr,
        (unsigned)vpinballStatTruncated, (unsigned)vpinballStatMaxHandleMs, (unsigned)vpinballStatMaxGapMs, (int)WiFi.RSSI(), (unsigned)ESP.getFreeHeap(),
        (unsigned)vpinballStatRenders, (unsigned)vpinballStatClears, (unsigned)vpinballStatZoneClears, (unsigned)vpinballStatRgb565, (unsigned)vpinballStatRgb888, (unsigned)vpinballStatSplitDone, (unsigned)vpinballStatSplitLost);
      vpinballStatPkts = vpinballStatZones = vpinballStatBytes = vpinballStatInflateErr = vpinballStatTruncated = 0;
      vpinballStatMaxHandleMs = vpinballStatMaxGapMs = 0;
      vpinballStatRenders = vpinballStatClears = vpinballStatZoneClears = vpinballStatRgb565 = vpinballStatRgb888 = 0;
      vpinballStatSplitDone = vpinballStatSplitLost = 0;
    }
  }
  else vpinballStatLastPollMs = 0;
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
    if (!vpinballRecvBuf || !vpinballDecompBuf)
    {
      // Repli (buffers absents) : en-tete de 5 octets sur la pile, un vrai
      // paquet ZeDMD declenche le reboot cible (mecanisme v7).
      uint8_t hdr[5];
      const int n = vpinballUdp.read(hdr, sizeof(hdr));
      vpinballUdp.flush();
      if (n == (int)sizeof(hdr) && (memcmp(hdr, VPINBALL_SYNC, sizeof(hdr)) == 0 || memcmp(hdr, VPINBALL_FRAME, sizeof(hdr)) == 0) && !vpinballModeActive)
        triggerVpinballBootReboot();
      continue;
    }
    const int len = vpinballUdp.read(vpinballRecvBuf, VPINBALL_DMD_RECV_BUF_SIZE);
    vpinballStatPkts++;
    if (len > 0) vpinballStatBytes += (uint32_t)len;
    if (packetSize > VPINBALL_DMD_RECV_BUF_SIZE) vpinballStatTruncated++;
    // v4 -- log hexadecimal des 3 PREMIERS paquets depuis le boot (1 ligne, cout
    // nul ensuite) : a permis de trouver le prefixe "FRAME" du client reel ; si
    // une mise a jour de libzedmd change encore le format, c'est ce qui montre
    // pourquoi le mode reste muet, sans reflasher.
    vpinballPktTotal++;
    if (vpinballPktTotal <= 3)
    {
      char hex[3 * 24 + 1];
      int hn = 0;
      for (int i = 0; i < len && i < 24; i++) hn += snprintf(hex + hn, sizeof(hex) - hn, "%02x ", vpinballRecvBuf[i]);
      Serial.printf("[VPINBALL] pkt #%u len=%d actif=%d hex=%s\n", (unsigned)vpinballPktTotal, len, vpinballModeActive ? 1 : 0, hex);
    }
    // BUG REEL v4 : le client reel (libzedmd recent) prefixe CHAQUE datagramme
    // de 5 octets "FRAME" avant les messages "ZeDMD..." (constate en test
    // reel, DMD2 + VPX/RB2) -- le filtre "commence par ZeDMD" rejetait donc
    // 100% des vrais paquets. vpinballProcessPacket() cherche deja la synchro
    // n'importe ou dans le buffer, le prefixe y est saute tout seul.
    if (!vpinballCarryActive && (len < 5 || (memcmp(vpinballRecvBuf, VPINBALL_SYNC, 5) != 0 && memcmp(vpinballRecvBuf, VPINBALL_FRAME, 5) != 0))) continue; // bruit (sauf suite d'un message coupe, v11)
    const unsigned long tHandle = millis();
    vpinballProcessPacket(vpinballRecvBuf, len);
    const uint32_t dtHandle = (uint32_t)(millis() - tHandle);
    if (dtHandle > vpinballStatMaxHandleMs) vpinballStatMaxHandleMs = dtHandle;
  }
}
