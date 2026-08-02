// ============================================
// web_config.h — Interface web de configuration
//
// safe-modify — Historique des modifications
// ============================================
// Version actuelle : v35
//
// v35 — 2026-08-02 — safe-modify — Retours test reel sur Partie A :
//   (1) le redemarrage apres suppression de dossier(s) lie(s) a des
//   playlists n'est plus automatique -- popup confirm() oui/non
//   (msg_confirm_reboot_playlists, remplace msg_folders_deleted_reboot)
//   laisse l'utilisateur choisir le moment ; bloquant par nature, empeche
//   aussi toute autre action pendant que la decision est en attente.
//   (2) Cache sessionStorage partage entre les pages Affichage et MEDIA
//   (cle 'dmd_gifdirs_cache', readDirsCache()/writeDirsCache()) pour la
//   liste des dossiers /gifs -- demande utilisateur : le va-et-vient
//   frequent entre les deux pages redemandait /lsgifdirs a chaque fois,
//   avec le risque d'echec reseau deja documente cette session. Affichage
//   immediat depuis le cache si present, rafraichissement en arriere-plan
//   qui remet le cache a jour ensuite (jamais bloquant sur le reseau).
//   Compilation via compile.ps1 : OK (0 erreur, 63% flash, 28% RAM). PAS
//   ENCORE reteste sur materiel reel.
//
// v34 — 2026-08-02 — safe-modify — Portage de la Partie A du plan
//   "cache_master_gifs" (jusque-la seulement sur master) dans ce worktree
//   dev/tous-txt-filter, pour permettre de tester A+B+C ensemble sur le
//   meme firmware pendant la session de test materiel en cours : nouvelle
//   fonction stripDeletedFoldersFromPlaylist() (reutilise le
//   writeBufChecked() deja present ici pour Partie B) branchee dans
//   handleWebConfigDeleteFolders() -- chaque suppression de dossier retire
//   desormais les lignes mortes des playlists concernees (cache_master_gifs.dat
//   exclu de ce nettoyage, jamais lu playlist par playlist a la lecture DMD)
//   et supprime leurs compagnons .cache/.sig/.idx. Cote JS (page MEDIA,
//   deleteSelected()) : message explicite puis redemarrage automatique
//   (doReboot(true)) si des playlists ont ete mises a jour. Nouvelles cles
//   i18n FR/EN/ES : msg_folders_deleted_reboot. Compilation via
//   compile.ps1 : OK (0 erreur, 63% flash, 28% RAM). PAS ENCORE teste sur
//   materiel reel (portage identique au code deja teste sur master, mais
//   jamais verifie sur CE worktree precis).
//
// v33 — 2026-08-02 — safe-modify — Retrait du compte de fichiers par
//   dossier (page Affichage), a titre de test suite a un crash reel
//   out-of-memory (abort() dans WebServer::_parseForm(), heap epuise
//   pendant un upload) observe en session de test materiel -- tentative
//   d'isoler si le scan de TOUS_MASTER_PATH dans handleWebConfigListGifDirs()
//   (+ le tableau static String dirNames[128]) et le nouvel endpoint
//   /lsgifdircount contribuaient a la pression heap ambiante. Retour a la
//   version simple de /lsgifdirs (liste de noms uniquement) ;
//   handleWebConfigGifCountFolder()/route /lsgifdircount retires ;
//   loadGenDirs() (JS) revient a un affichage sans compte, tri alphabetique
//   CONSERVE (pur JS, aucun cout heap firmware). Reste du plan (Partie
//   B hybride/marqueur FULL, Partie C etiquette SD) inchange. Compilation
//   via compile.ps1 : OK (0 erreur, 63% flash, 28% RAM -- variables
//   globales legerement reduites, 94444 vs 96532 octets, coherent avec le
//   retrait du tableau static). PAS ENCORE reteste sur materiel reel.
//
// v32 — 2026-08-01 — safe-modify — Partie B du plan "cache_master_gifs"
//   (simplification radicale, apres une longue serie de bugs reels trouves
//   en test materiel sur tousSyncTask()) : RETRAIT COMPLET de
//   tousSyncTask()/resync incrementale (struct ChangedFolderInfo,
//   fnv1aString(), defines TOUS_SYNC_MAX_*, handleWebConfigResyncTous(),
//   route /resync-tous, bouton "Resynchroniser l'index GIFs" + i18n
//   FR/EN/ES associes, champs isResync/foldersChanged/linesAdded/
//   linesRemoved de PlaylistGenStatus) -- elimine du meme coup la limite
//   des 1024 fichiers/dossier (n'existait que dans le code retire).
//   REMPLACE par une generation de playlist HYBRIDE :
//   handleWebConfigGeneratePlaylist() verifie desormais PAR DOSSIER (pas
//   globalement) la presence dans cache_master_gifs.dat (corrige au passage
//   un bug ou cocher un dossier neuf a cote de dossiers en cache produisait
//   une playlist silencieusement incomplete), filtre la portion deja en
//   cache (filterMasterIntoFile(), quasi instantane) et ne scanne que les
//   dossiers neufs (scanFoldersToPlaylistFile(), inchangee) -- qui sont
//   ensuite EMBARQUES AUTOMATIQUEMENT dans le fichier maitre
//   (appendMatchingLines(), plus besoin de rescanner /gifs/). Marqueur
//   "# FULL:dossier1,dossier2" ecrit en tete de chaque playlist generee par
//   le DMD (toujours des dossiers entiers) : handleWebConfigAddToPlaylists
//   Batch() le lit desormais pour decider si un nouveau fichier uploade
//   doit y etre ajoute (plus precis que l'ancien fileContainsNeedle() seul,
//   qui aurait pu polluer une playlist hybride cree cote PC -- retrocompat
//   totale pour les playlists sans marqueur). cache_master_gifs.dat est
//   desormais une cible d'ajout INCONDITIONNELLE lors d'un upload (avant :
//   seulement s'il referencait deja le dossier). /lsgifdirs renvoie
//   {name,count} (compte depuis le cache, "?" si dossier jamais vu) +
//   nouvel endpoint /lsgifdircount?dir= (compte exact d'UN SEUL dossier, a
//   la demande -- jamais /lsgiffiles, retiree v92, jamais reintroduite) ;
//   page Affichage : tri alphabetique + affichage/rafraichissement du
//   compte a la coche. Compilation via compile.ps1 : OK (0 erreur, 63%
//   flash, 29% RAM). PAS ENCORE teste sur materiel reel -- chantier volumineux,
//   tester en priorite : generation cache-seul, generation hybride
//   (cache+scan), premiere generation jamais lancee (bootstrap), upload
//   vers dossier neuf, playlist hybride creee cote PC (marqueur # FULL:
//   absent cote outil PC pour l'instant, session separee a prevoir).
//
// v31 — 2026-07-29 — safe-modify — BRANCHE DEV (dev/freertos-playlist-scan) :
//   playlistGenStep() (machine a etats appelee depuis loop()) remplacee par
//   playlistGenTask(), tache FreeRTOS dediee (cf. RecalBox_DMD.ino v37) --
//   un scan de dossier lent (Arcade/Consoles/Halloween/Vertical_DMD, confirme
//   plusieurs secondes/fichier par moments) ne bloque plus la page web ni le
//   bouton Arreter. Tout acces SD encadre par sdAccessMutex, non-bloquant
//   cote loop()/lecture GIF, bloquant cote tache. Bugs materiels reels
//   trouves et corriges pendant cette session : creation de tache jamais
//   verifiee (echec silencieux si heap insuffisant), pile 8192 trop grande
//   ramenee a 4096, un acces SD (forceDeleteFile sur arret demande) hors
//   mutex -- seul crash reel observe, corrige. Ajout d'un garde-fou heap
//   critique (ESP.getMaxAllocHeap() < 4096 -> arret propre au lieu d'un
//   abort()) suite a un second crash identique en scan normal (fragmentation
//   heap sur un tres long scan), avec message distinct cote utilisateur
//   ("memoire insuffisante" vs "annulee"). Bouton Arreter (JS) rendu robuste
//   (retry 3x) apres un cas reel de requete perdue laissant le bouton
//   desactive sans effet. Cache par dossier avec peremption par mtime
//   ESSAYE puis ABANDONNE le meme jour : 4 bugs reels trouves d'affilee
//   (descripteurs simultanes -> abort() fopen()/lock_init_generic(), dossier
//   modifie pendant son enumeration -> 0 fichier trouve, flush incrementaux
//   perdant des fichiers, comptages erratifs persistant meme apres passage
//   en RAM-only) -- le dernier test reel a confirme que le probleme venait
//   de ce code de cache lui-meme (pas de la creation/destruction repetee de
//   tache, hypothese testee et infirmee via une tache persistante puis
//   revertee). Retire entierement : playlistGenTask() revenue a un scan
//   direct simple, sans aucun cache par dossier. A remplacer eventuellement
//   par une approche filtrage-de-texte sur un TOUS.txt tenu a jour (evite
//   toute re-enumeration de /gifs/<dossier>), pas encore concue en detail.
//
// v30 — 2026-07-23 — safe-modify — Bug confirme (retour utilisateur :
//   "recalbox_ip disparu du config.ini") : handleWebConfigSaveAP() faisait
//   encore SD.remove("/config.ini") puis ne reecrivait que 6 cles WiFi --
//   EXACTEMENT le meme bug deja corrige le 2026-07-21 (voir memoire
//   projet), reintroduit par la refonte multi-pages qui repartait d'un
//   instantane anterieur a ce fix. Chaque passage par la page AP (premier
//   boot ou mode secours WiFi) effacait donc recalbox_ip/playlist/
//   brightness/clock_*/language -- expliquant que les scripts Recalbox
//   (MQTT, installes et fonctionnels cote firmware/GitHub) semblaient ne
//   "rien faire" : le DMD n'avait plus l'IP pour se connecter au broker
//   MQTT de la Recalbox. Fix : re-applique le patch cle-par-cle
//   (writeConfigFlag()) au lieu du remove+rewrite complet -- preserve
//   desormais toutes les autres cles. Compilation verifiee OK (62%
//   flash). PAS ENCORE reteste sur materiel reel.
//
// v29 — 2026-07-23 — safe-modify — Parite fonctionnelle page fractionnee
//   vs ancienne page unique (retour test reel utilisateur), sur les 4
//   pages BASIC/NETWORK/CLOCK/MEDIA (backend deja intact, tous les
//   handlers /save /lsgifdirs /generate-playlist /delete-playlist /upload
//   etc. existaient toujours -- uniquement le FRONTEND avait regresse) :
//   (1) Bug SSID non recupere du config.ini corrige : scanWiFi() faisait
//   sel.innerHTML='' puis reconstruisait la liste depuis /scan-wifi SANS
//   jamais re-selectionner le SSID sauvegarde (charge juste avant par
//   loadConfig() puis efface par le scan) -- nouvelle variable savedSsid
//   memorisee avant le scan, re-selectionnee dans la liste scannee (ou
//   ajoutee en option separee si absente du scan).
//   (2) Barre de navigation permanente (topnav, 4 liens) ajoutee en haut
//   des 4 pages -- demande utilisateur, remplace le simple lien "Retour
//   au menu".
//   (3) Actions de bas de page manquantes restaurees sur les 4 pages :
//   Enregistrer / Enregistrer & Redemarrer / Redemarrer (/reboot) /
//   Reprendre DMD (/dmd-resume) -- absentes de la refonte, seul un simple
//   bouton "Enregistrer" existait.
//   (4) CLOCK : clock_theme et clock_tz etaient des <input> texte brut
//   (l'utilisateur devait connaitre les numeros de theme/codes POSIX) --
//   remplaces par les <select> complets de l'ancienne page (10 themes
//   nommes, liste pays/UTC).
//   (5) BASIC : ajout suppression de playlist (existait dans l'ancienne
//   page, absente de la refonte). MEDIA : ajout boutons "Tout/Rien
//   selectionner" pour les dossiers de la playlist generee.
//   (6) Esthetique alignee sur l'ancienne page (sections cartes, h2 avec
//   bordure, boutons colores par fonction) au lieu du style plat minimal
//   de la refonte -- retour utilisateur "esthetiquement moins beau".
//   Tailles apres regeneration gzip : BASIC 2345, NETWORK 2436, CLOCK
//   2581, MEDIA 2567 octets (AP inchangee, 3102) -- toutes tres en-dessous
//   du seuil ~12.5 Ko a risque (voir v28). Compilation verifiee OK (62%
//   flash, +4 Ko negligeable). PAS ENCORE teste sur materiel reel.
//
// v28 — 2026-07-23 — safe-modify — Reintegration multilingue (page AP
//   uniquement, decision utilisateur) suite au constat v27 : le passage a
//   l'architecture multi-pages (session anterieure) avait ete fait a
//   partir d'une base predatant TOUT travail i18n (meme l'ancien systeme
//   navigateur-only), pas seulement mon integration backend -- les 6
//   nouvelles pages etaient 100% francais en dur, y compris WEB_CONFIG_AP_HTML.
//   Reconstruit entierement sur cette page (dict AP_I18N fr/en/es inline,
//   data-i18n/data-i18n-placeholder, tr()/applyLang()/setLang(),
//   selecteur #langSelect) -- PAS sur les 5 autres pages (MENU/BASIC/
//   NETWORK/CLOCK/MEDIA), volontairement laissees en francais pour
//   l'instant (portee reduite, decision utilisateur : ce chantier sur les
//   6 pages aurait ete disproportionne). Priorite de langue : localStorage
//   > config.ini (nouvelle route GET /lang) > navigator.language > 'fr'.
//   setLang() persiste immediatement via POST /save-language (nouvelle
//   route, valide fr/en/es, writeConfigFlag("language", lang) + met a jour
//   uiLanguage en RAM). Nouveau extern String uiLanguage (variable definie
//   dans RecalBox_DMD.ino, forward-declaree ici comme les autres globals
//   partages). Page AP : 4969->8712 octets HTML, gzip 1995->3102 octets --
//   reste tres en-dessous du seuil ~12.5KB souponne d'etre a l'origine des
//   coupures reseau ayant motive le fractionnement (voir note v27).
//   Compilation verifiee OK (62% flash, +2KB negligeable).
//
// v27 — 2026-07-23 — safe-modify — Fix erreur de compilation reelle :
//   triggerWebConfigMode(const String&) est appelee par handleDmdOpen()
//   avant sa definition (celle-ci n'apparait que plus bas, juste avant
//   handleWebConfigRoot() qui l'utilise aussi) -- "not declared in this
//   scope". Fix : forward declaration ajoutee avant handleDmdPause()/
//   handleDmdOpen(). NOTE : le passage a l'architecture multi-pages
//   (WEB_CONFIG_MENU/BASIC/NETWORK/CLOCK/MEDIA_HTML, routes /config/*) est
//   deja present dans ce fichier au moment de ce fix mais n'a jamais ete
//   documente dans ce changelog (toujours v26) -- vraisemblablement une
//   session anterieure. Voir memoire projet pour la remise en etat en
//   cours (travail multilingue backend -- uiLanguage/route /lang/
//   /save-language -- absent de cette architecture, a reintegrer).
//
// v26 — 2026-07-14 — Integration horloge: 10eme theme "Level 1-1" dans le dropdown (+ i18n
//   FR/EN/ES). Section couleur Neon refaite: un seul champ couleur "clock_neon_color" + case
//   "Personnalisee" (clock_neon_color_enabled) remplacent les 2 selecteurs clock_neon_color1/2 -
//   cle config.ini CLOCK_COLOR remplace CLOCK_NEON_COLOR1/CLOCK_NEON_COLOR2.
// v25 — 2026-07-13 — Fuseau horaire: select pays/UTC-5..+5 au lieu du champ texte POSIX
//   (valeur = code POSIX injecte directement, aucun changement backend). Nouvelle case
//   "Demarrage silencieux" (section Affichage) <-> variable showInfo (inversee: cochee = info=0).
// v24 — 2026-07-02 — lsgifdirs retourne name+count (rapide, pas de string building). Tooltip liste fichiers chargé au survol via /lsgiffiles?dir=
// v20 — 2026-07-02 — showMsg push sur DMD (couleur succes/échec), msg_welcome "WEB DMD CONFIG"+IP, défilement DMD
// v19 — 2026-07-02 — Fix: deleteFolderRecursive chemin relatif (f.name() sans path), forceDeleteFile/rmdir avec fullPath
// v18 — 2026-07-02 — Page web ouverte = DMD en mode attente avec msg permanent, plus de resume auto, shutdown clock
// v17 — 2026-07-02 — SUPPRIME /gifcount + async count (causait freeze SD SPI au refresh page), test msg chargement supprime
// v16 — 2026-07-02 — Fix: delay(1) dans toutes les boucles SD longues — WDT timeout bloquait le resume DMD
// v15 — 2026-07-02 — Fix: refreshPlaylistSelect error visible, delay(1) dans lsplaylists/lsgifdirs/gifcount, async gifcount
// v14 — 2026-07-02 — Fix: lsgifdirs revert noms simples (WDT), /gifcount asynchrone, bouton reboot + i18n
// v13 — 2026-07-02 — Fix: msg flottant (position:fixed), uploadFile reset + uploadSuccess flag, add-to-playlists await, rename trick pour RO FAT32, chemin sous-dossier GIF fixe, delays DMD, rmdir rename fallback
// v12 — 2026-07-02 — Fix: mkdir workaround (RO), forceDeleteFile, msg DMD persistant MODE_BLACK, msg web scrollIntoView, compteur GIFs + tooltip, test msg chargement
// v11 — 2026-07-02 — Fix: ${name}->${0}, nettoyage filename, logs, rmdir fallback, showMsg dans dmdPause
// v10 — 2026-07-02 — Pause DMD pdt operations SD (upload/delete/gen), resume auto
// v9 — 2026-07-02 — Multi-upload, deletion dossiers, dropdown upload, regen playlists auto
// v8 — 2026-07-02 — i18n FR/EN/ES, auto-detect langue navigateur
// v7 — 2026-07-02 — Tooltips sur tous les champs
// ============================================

#ifndef WEB_CONFIG_H
#define WEB_CONFIG_H

#include <WiFi.h>
#include <WebServer.h>
#include "web_config_html_gz.h"

extern int    screenBrightness;
extern bool   wifiEnabled;
extern String wifiSSID;
extern String wifiPassword;
extern bool   wifiStaticEnabled;
extern String wifiStaticIP;
extern String wifiGateway;
extern String wifiSubnet;
extern String wifiDNS1;
extern String wifiDNS2;
extern bool   bluetoothEnabled;
extern String bluetoothName;
extern bool   showInfo;
extern String playlistName;
extern bool   playlistRandom;
extern String recalboxIP;
extern bool   clockEnabled;
extern int    clockTheme;
extern int    clockIntervalGifs;
extern int    clockIntervalMin;
extern int    clockDuration;
extern String clockTimeZone;
extern bool    clockNeonCustomColor;
extern uint8_t clockNeonR, clockNeonG, clockNeonB;
extern bool   requestReboot;
extern String uiLanguage;
extern void webDmdPause(const String &msg, uint16_t color = 0xFFFF);
extern void webDmdResume();
extern void webDmdSetMainMsg(const String &msg);
extern void clearFirstBoot();
extern String g_sdOpSubMsg;

static WebServer *webServer = nullptr;
static File uploadFile;
static String uploadDir;
static unsigned long uploadStartMs;
static int uploadTotalBytes;
static bool uploadSuccess = false;
static String uploadErrorMsg;

// v92 -- bloc WEB_CONFIG_HTML (ancienne page monolithique pre-fractionnement,
// jamais servie par aucun handler depuis le passage aux 6 pages minces)
// retire integralement lors de la reconstruction depuis cette base -- code
// mort, cf. plan de reconstruction.
static const char WEB_CONFIG_MENU_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="fr">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>RecalBox DMD</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:'Segoe UI',Tahoma,sans-serif;background:#1a1a2e;color:#eee;padding:16px;max-width:700px;margin:auto}
.logo-wrap{text-align:center;margin:4px 0 10px}
.logo-wrap img{max-width:100%;height:auto;border-radius:8px}
.tagline{text-align:center;color:#aaa;font-size:13px;margin:0 0 14px}
.section{background:#16213e;border-radius:8px;padding:16px;margin:12px 0}
.continue{display:none;text-align:center;padding:12px;border-radius:8px;background:#0f766e;color:#ecfeff;font-weight:700;text-decoration:none;margin-bottom:12px}
.menu{display:grid;gap:10px}
.btn{display:block;padding:12px 14px;border-radius:8px;background:#0f3460;color:#eee;font-weight:600;text-decoration:none;text-align:center}
.btn:hover{background:#16478a}
.small{font-size:12px;color:#9ca3af;margin-top:10px;text-align:center}
#langSelect{position:absolute;top:10px;right:10px;width:auto;padding:6px 8px;font-size:13px;background:#16213e;color:#8ab4f8;border:1px solid #333;border-radius:4px}
body{position:relative}
</style>
</head>
<body>
<select id="langSelect" onchange="setLang(this.value)"><option value="fr">FR</option><option value="en">EN</option><option value="es">ES</option></select>
<div class="logo-wrap"><img src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAQQAAAC4CAMAAAAyqWKCAAAAYFBMVEX///8U//n+/v77+/vr6+tw6Nz7lJ/9ixTNqaFwkZSUTlJXOy9EOjk6KSXpEBDmDQ17Dg45Hx45Dg4RUlQPKiodFhYZDxAKEREVBgULBgYFBAQCAwMBAQE4AAAAAAEAAAAgLdlgAAAQH0lEQVR42u2di3aiOhSGU2A0QGhF5RZief+3PPuSQECwdk5nIdW91rRgtWvy9d+X3IjoXtaJF4IXhBeEF4QXhBeEF4QXhBeEH4fw+YIAZvqL9mkhtLNAngmCMaf4zE03XZVtn8K3IbRta6r47cRiMCaWqe7aJ3SH89vb2RgWQhzLuNo4hW9C+DSn07k6gRA6pNB2WYwUim1TEN/MCdXb21sM3nCuOD4qGSugcN50XPiuEtrTKX5DA0WgGFKZdmdQQ7ZlCt+PCS14wxuSOEG721iqtgAIEBfM80CAv/75LT5XZ1QCxkUMCjKOm6fKDq2BuNjakiljAunZPFeKNB0mBywXEAIRaJ+uYkQIZ5cS24w00JrnK5v1+K41T6gEjI7jdhvwjudSgrlcasuia5JE5WXzdEo4fIAdLh1VzcmOLAEWz1MsGUMMwJBCDwEt227/4bt9hwOq4IIkaqybkl1S5ipNEoRgnkQJNTDA7xf8bjoNnkCv57td8SQQDDYeFaBREXBb7nYKUqRu1W7XPBOExoUG8AYQQO5iw7NkB4JwoSoBlaA7EECBSjAQG56nF6khImoXE3SX7nZcPSKEtnui7ADpgbODRi+AEiHNwC3Sp4FgKdg6YVQmqKeB0EIiIBkcagoBOqMSAa1onwJCP+FW12Y04dIUucrpDea3Q0AEhVKqwFCoTS8NvDVFBj+YTs/9OggggzKWkRAijGLsLMHIEhq0Wqs4CuEHkUw2Ogsj7h5ESJCACPCLkEOfsVX8Ov0gUpscZBJ3dh51DBqglgZBGCCGoqgarZWEV+wPQtBDvEUK4j5f0FKELAKyIES3QBPD60ACXt8iBXFfPAAGYmRBMP5OF47Cb4TQdrFjAJIfWo3mcQkthe0V0OIeBir0feHKkEwQJgXEhwBDxubGmMSdAYFbK6IEksQECN/G8NYqAmmEQrYbCwviDiEkzhlCobrBNXoGURxHItNQNhGt7TmE+Do7NhG7PoYA1bYqCMeRQMJSBRUq6FR2pJIgiBpjfhUE7YRALQclJH5OYHU0oIEo65qY8yVKQf8mCFAYR4HVP5QFAEFRdeB5gzJNA04SSRctwkC2v0UJLRoModk6MVJVVcEfWMM3FfVaAHfAnlUEGpAqi+2bwTno41uH4P6WHAhHRdCoeILGJzFWjpF27w4pV2xnnadYZqAkWsYih6Z6M46lnycD6kFGIjbYl8BbUEeGn43VNiiI5QqJ44AKub2UCSUsX4X1anJcK4ShLDody661URTey33LcBuDbmI5MULjoHmqr5i97DCulygEdDoCH8g4WEC0lPhpITeRK8WiEALSdTzURlAdh8q0UEOPi2goEguulKS0snGfC8JNLPMUS9UBNz5Sfv8xYCWE076DSChBBL1EAIJCl9lIxbDkDhj/gwAW5o1DoCwKOe07YPpMMmx0X0ti3oTSKcDvm1VC2xUhVwB60l8aFUrTfrRfSxviKKItLPIUC96gsAKGnsIEQiBEMNuZFpOhBQnVEv+ODUhhSQn0VyxhTiGaZoKFMYVpzxKUUERUNultQjDYdvbnJhK3xlOWhllEpB1J/fhZUixVSgFH9v8BgTPMFsaZxGKC5GGyawhYLlzZ1WBT1HCtsQl/EIsJUkQwx/T3SsDVO9o51QYh2AQZ44yjvgqMYTxjYRBcQ2B/iB5/RZdYGkyi5AbLkSYQQpHM/ZrJwCNAaBGCwmzy+ElSLCdILnPG0y5UAPB4iWduiHX0NtP1SXKDEGA/S+/Lk7knRDM3ctYaau4IQv9pSBRmcxC8BImTT0HoM1hIeLD9w6dgcwJnmeDh/UEs9SA5v1P97DFYbA9OU/lDboqVkIVbSJJz7mBsgqTrqh9svsWAcEXDJI3E9Ioz+ptIkuJGghx8I6B/4Av61gQFDkfSW4XzgD5JthuDAMtT/d4fTkmzxL/aEg0LeiR3pcLhwxRe1IP7g1hMkH3+g6FVuJcwqHSVGyeZsqO3wrjk8M5yC/4gFhJkNHqxyO7e4VPQKrbB6Jc9+Ap4MZsgodCLVeZZkd1rRVHQF7YMC+qHLxrF0hDrzf6RZ+Ft86qG7UDACVjBU8s/YkE/4rghCJQgg68FcK/Z7tRjJ0kxGxJ+2B49KIi/CAnftkcPCi8ILwgvCC8ILwgvCC8I/wQC9h1EeHsB+EsJvx8CrmFM4jBOEnmr2P7dEGjVZgHdo5mVTNuHMPQCg5nL/gpap3UWwTL/OAhn3k2/6PfHBFJC1rnR2N+kBN7phhb0V+Hci4GDkMy9B2Ze8OtGIUT9ICGsbS/KsiqLKpZwhS+VKsqqEgwupYVgGvqJlKX9nMJfIXHJ3wZWLs1DwNX7Ma3vDmmHgyzxHl6U/Aenr3TvlKD620ri6LwKYUE4jTzLx5+JW4AAG+Al+zMs5IVt4gTF4II+8nHIibB1HPZ7ySEmRBpvDc5PK2NgeTiuBu8KudmYEMFzEiTGe5g9ajU/dDGSsA6BIUS46ymO8U1x20J2aFuEwLcFfgbWMIoYJyCKeLMQpFudQauZae+CJL3zgzixMFAoAusOBTGxt22MrVchXKoEd89FG3WHCMQueflqDJc0qyIjd6XwCi7hC8zOKdgJ526ldFcx/wq4z6In7zuEW68Yp5MNc2Wk2yw/uuV32F/xFBXjqxf5gvCC8ILwgvAkEIKfhxBsUAk/bhtTAu7UED9uwbbWJ+BG8fjHTW1s9dq/+e+aza1y1z9u7euAvO51SuALwgvCC8ILwvdOSyB7KWFlRCtDgJqkyHOY6GvWFMP6SpD07Guc6Lt+CGLbVviYL5jbMp9fqWWrEIzuHwC+gwffNjMN4dP46By6pZOIPu8429fcdKs1IcCDCXaeyfGBKQZO4judmMFbrJeakO73CVre/v3TKlaEgA828yHskpEWWjyg1LerMymNrhud7NkOt0RwPIAdwepHg6DLZDc22e+q/vw0upfBm3OJ8TZtOGhkP1iSpulx6QmIh3drx8eCAFvqd7jawTZfoijksJMOHlrwNrXxmZSmqdP92A7XfXargvePdziL4P390DTaPBQEWvHBENyFOzQFTuGL//z587bsEKY77qeWpMfruPDx7hsey3CFQawXFQte3IQK4JUvIwjxG7xkKfyBy6kWdJHury2ZRo3DATUw5mAeRwmmgKUfPoTIgwCbk7HlVgt/eh5VD6nc7/EjrvVAkCDk5VQHHxMG7x+HQz0WzIru0OFKB246rmfgK/tE09bEDAbbPlyNIKAPScegv/ZPIQIdQPKEZg++wEQuj6EEOFENlz3ZpmeZg9C/QcVJbJsuSTGRD6F3JkkCkD6EwZqPDwmLzHYWwscO9JLgDeTLR4AA29J3KruCIJOcD5OpzvxThIBrXsIxBGNK60PY9J1dPyknEDSsuY56CEQklKyFg59H1oOQ7OgvLEfugKcI4QljcEytUokHwSrh3FcCn4o/RA3HdVTuund3eLKNFDbaYMM59Ej2j4eBsBulSOlOXGQIsQ+BhGLrZ/7f56niN1DDfSB+7OXQi39+aHjklttiungcCF6t6Gj0ECgecnagBXMuXbbca9rvFyDsa+8pB5kVkcQkkcQ2uDCE7uEg+GdvOgh/RnWCK5x6CGk6D2GfuGnP40G5H2BaSO3NpiB45hWPPQSqE2yKhJ0nA4S9lYL5eE8XIEBg1JuD4JmNCbY4cHXCLhyu97araA7vh0UI7+5Qs01CqAYIvuGa2wkE6DtCVuSEQIf/ggMRBCgVPqhiMusXS/8DAnx6v2wDBKiPuDSgUvGDNmVQhnRHAf92CIaU8I6jTtxoCwFfpNvHgSCTf6mEj7G5HsSjQUg+7WhzJv8NhL7jNDUYZTLrd6Uz6jVolfBoSrIblc3/C4Lp3WHZvKJx1ZElKXNY0JVj07UuFXlHMYLQX5y+AeG4GQg40IpTDaYtIXQVdvzdVf7VGScc4pODEPc0fhUE2lqlSjp4FGDgUFiewYmc/RgjjSm2RpMOzu0pnoWQp1uG0MBho6nKS93AWfWqgC5ygw8t+/RnXiqccDm5lhONqmVGqaVQmz4SHDcHwZQ59m5UDufPAoSsaOCFNM0at8brDKbZM+AC4gRpwRtvRgqHY6PTg+0xNPdAGFKF14cSqwUEajxvmMLvOd/nZUujxNUZldCazxYuHI1qeEpua1JuLjxamhretPqYcmdqgGBHFS/1DI8HgAAnEKM7sNnvMIeEksADaonB6QzN5gt4vqPGi8r7DXWNE/o8q5tQWvxkHfRnwh8O7gxsywM6DMbxWB9CW3oQsPEDBJQCuMCJKfD3U0+jcu5ijmme1/aI57rGgAqhNT0kpTecfsEGX2CY5XK85rE6BE0QmEKaWgr4FSBoFIJtvDOPBkfGushx8hFkY+oMYgnjgA3ctfbqsfpA7aYRFqwcL/CY7frCPA6rQ2gYQs8gVVYNAKEx1xA8Gtp6E34qP9ZNeczpou004kj92RfTi8SQbzCP+mMy3LwOhJYg9BRQEil7BULQ+haECmNGfSQI0Pjj0eGoS74qhwn+epiOZx48kwsztB/H1UeWtA+hN4ZQNHAE3y0I+EztHgJYanHkecZXpXah80IQCMqUR/O5MgSYVYeS+ZoCZkxYxVXdhgArUtAHbOOdZY4BigIzBZAiBsfjpdY1X18whpiGLtdWQts05RAURhDywkI4L0KAYmEGwmAAQXtCgPZeao+HfflSDzP0YpVuA7rDvBLugqDr2xDKhuVmW45N7i96HsdhguJxIZwXIeivIVDc8CAcr3GgFlaGAE9pwWdBz4QEOMOeIZwXITQEIZtnkAEEbN4CBA9HL4V1YgJDyOchoBKWpIAvVxVDyG9AgAB4B4TaPASEbEYIX0DAn0JyyZekkBMECJ4A4fIFBL0uBAwKUwrZdyAAhSxbgFBA8/Q9EGxZtUpM0JpyJEHIroTQQzgveEOFT3m6CQGkQBC+codGrxoYyR/GFDIfQvG3EDILARj0EPIlCPWKEIyDMFCgJ9bkxRjCeYYBQeh1NA8B+pJsRIBsAQL7w2oxgdphKVjzGFh/OF8z+A6EsnQMLAX/em0IfVAoKMhbBMSgh1BcURgYoD8sQMgcBHQHCp+9Hb1LW0zXvMh3PQgVU+j/mxaBZeCkcJ4wsBAqmx6yGQa4kaamoGAhZPmVOQj1ihBanwKRsN9hHxDYiMIw0jYwsGH1mkLGcfEGhGyQAiqB8oNYa+sTRYWBgmNAEJoxBN+Ygc2wVxSsV5XUvGbiDvhD9jsrhZUhUJK8olA6BssUKh8CSyGbMBhDKHwGSCFnChYCFFVmNQgUFZrKp1A6BrjNvpqnwC9raqDTeo8h6xmQN1jGEwheUChXhmCjAlHwzTFYgGAZWBUVA4U+yeZcbjGEuvSkQK6QTyHwHhCx4o7IZorBMsADKKsZDJWDoEct9AiMhODczY8JV0pYFQJKQVv/L71gAPKkUzh1NcVQDQwmFDxjt7IM7FuKfDZFUkXV6FWVAG21EEamae+fp4WpsVC4hVcUisIXgpPC8fgFhP8A7kK1Ey30vv0AAAAASUVORK5CYII=" alt="RecalBox"></div>
<div class="tagline" data-i18n="tagline">Configuration DMD</div>
<div class="section">
<a id="continueLink" class="continue" href="#"></a>
<div class="menu">
<a class="btn" href="/config/basic" data-i18n="menu_basic">&#x1F4A1; Affichage &amp; Playlists</a>
<a class="btn" href="/config/network" data-i18n="menu_network">&#x1F4F6; Wi-Fi &amp; Bluetooth</a>
<a class="btn" href="/config/clock" data-i18n="menu_clock">&#x23F0; Horloge</a>
<a class="btn" href="/config/media" data-i18n="menu_media">&#x1F4BF; M&eacute;dias</a>
</div>
<div class="small" data-i18n="small_hint">Page fractionn&eacute;e pour un chargement rapide et fiable sur ESP32.</div>
</div>
<script>
const MENU_I18N={
fr:{title:'RecalBox DMD',tagline:'Configuration DMD',menu_basic:'&#x1F4A1; Affichage &amp; Playlists',menu_network:'&#x1F4F6; Wi-Fi &amp; Bluetooth',menu_clock:'&#x23F0; Horloge',menu_media:'&#x1F4BF; Médias',small_hint:'Page fractionnée pour un chargement rapide et fiable sur ESP32.',cont_basic:'&#x1F4A1; Continuer : Affichage & Playlists',cont_network:'&#x1F4F6; Continuer : Wi-Fi & Bluetooth',cont_clock:'&#x23F0; Continuer : Horloge',cont_media:'&#x1F4BF; Continuer : Médias'},
en:{title:'RecalBox DMD',tagline:'DMD Configuration',menu_basic:'&#x1F4A1; Display &amp; Playlists',menu_network:'&#x1F4F6; Wi-Fi &amp; Bluetooth',menu_clock:'&#x23F0; Clock',menu_media:'&#x1F4BF; Media',small_hint:'Split page for fast, reliable loading on ESP32.',cont_basic:'&#x1F4A1; Resume: Display & Playlists',cont_network:'&#x1F4F6; Resume: Wi-Fi & Bluetooth',cont_clock:'&#x23F0; Resume: Clock',cont_media:'&#x1F4BF; Resume: Media'},
es:{title:'RecalBox DMD',tagline:'Configuración DMD',menu_basic:'&#x1F4A1; Pantalla y listas',menu_network:'&#x1F4F6; Wi-Fi y Bluetooth',menu_clock:'&#x23F0; Reloj',menu_media:'&#x1F4BF; Medios',small_hint:'Página dividida para una carga rápida y fiable en ESP32.',cont_basic:'&#x1F4A1; Continuar: Pantalla y listas',cont_network:'&#x1F4F6; Continuar: Wi-Fi y Bluetooth',cont_clock:'&#x23F0; Continuar: Reloj',cont_media:'&#x1F4BF; Continuar: Medios'}
};
let currentLang='fr';
function tr(k){return (MENU_I18N[currentLang]&&MENU_I18N[currentLang][k])||MENU_I18N.fr[k]||k;}
function applyLang(backendLang){
  const stored=localStorage.getItem('dmd_lang');
  if(stored&&MENU_I18N[stored]){currentLang=stored;}
  else if(backendLang&&MENU_I18N[backendLang]){currentLang=backendLang;}
  else{const nav=(navigator.language||'').substring(0,2);currentLang=MENU_I18N[nav]?nav:'fr';}
  document.documentElement.lang=currentLang;
  document.title=tr('title');
  document.querySelectorAll('[data-i18n]').forEach(function(el){el.innerHTML=tr(el.dataset.i18n);});
  document.getElementById('langSelect').value=currentLang;
  updateContinueLink();
}
function setLang(code){
  localStorage.setItem('dmd_lang',code);
  applyLang();
  fetch('/save-language',{method:'POST',body:'language='+code,headers:{'Content-Type':'application/x-www-form-urlencoded'}}).catch(function(){});
}
const SECTIONS={basic:{url:'/config/basic',key:'cont_basic'},network:{url:'/config/network',key:'cont_network'},clock:{url:'/config/clock',key:'cont_clock'},media:{url:'/config/media',key:'cont_media'}};
function updateContinueLink(){
  const last=localStorage.getItem('dmd_last_section');
  const a=document.getElementById('continueLink');
  if(last&&SECTIONS[last]){a.href=SECTIONS[last].url;a.innerHTML=tr(SECTIONS[last].key);a.style.display='block';}
  else{a.style.display='none';}
}
fetch('/lang').then(function(r){return r.json();}).then(function(d){applyLang(d.language);}).catch(function(){applyLang();});
// Reprise auto a la fermeture -- ESSAYEE puis RETIREE (2026-07-29) :
// aucun moyen fiable de distinguer une vraie fermeture d'un simple
// rafraichissement de page (habitude trop ancree pour l'utilisateur, faux
// positifs trop frequents).
</script>
</body>
</html>
)rawliteral";

static const char WEB_CONFIG_BASIC_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="fr">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>RecalBox DMD - Affichage</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:'Segoe UI',Tahoma,sans-serif;background:#1a1a2e;color:#eee;padding:16px;max-width:700px;margin:auto}
h1{color:#ffd146;text-align:center;margin:8px 0 14px;font-size:22px;border-bottom:2px solid #ffd146;padding-bottom:8px}
.topnav{display:flex;gap:6px;flex-wrap:wrap;justify-content:center;margin-bottom:14px}
.topnav a{padding:8px 14px;border-radius:6px;background:#16213e;color:#8ab4f8;font-size:13px;font-weight:600;text-decoration:none}
.topnav a.active{background:#8ab4f8;color:#1a1a2e}
body.gen-busy .topnav a{pointer-events:none;opacity:.4}
.section{background:#16213e;border-radius:8px;padding:16px;margin:12px 0}
h2{color:#8ab4f8;font-size:15px;margin:0 0 10px;border-left:3px solid #8ab4f8;padding-left:8px}
.row{display:flex;flex-wrap:wrap;align-items:center;margin:10px 0}
.row label{flex:0 0 150px;font-size:14px;color:#aaa}
.row input,.row select{flex:1;min-width:120px;padding:8px 10px;border:1px solid #333;border-radius:4px;background:#0f3460;color:#eee;font-size:14px}
.row input[type=checkbox]{flex:0 0 20px;width:20px;height:20px;margin:0 8px 0 0}
.btn-row{display:flex;gap:10px;justify-content:center;margin:18px 0;flex-wrap:wrap}
.btn{padding:12px 20px;border:none;border-radius:6px;font-size:13px;font-weight:bold;cursor:pointer}
.btn-save{background:#ffd146;color:#1a1a2e}
.btn-reboot{background:#e63946;color:#fff}
.btn-resume{background:#2d6a4f;color:#fff}
.btn-del{background:#555;color:#fff}
.btn-gen{background:#2d6a4f;color:#fff}
.desc{font-size:12px;color:#aaa;margin-bottom:8px}
.dirs{margin:8px 0;max-height:220px;overflow-y:auto}
.dirs label{display:flex;align-items:center;gap:8px;font-size:14px;padding:3px 0}
.dirs label span.name{flex:1}
.mini-row{display:flex;gap:8px;margin-bottom:8px}
.mini-btn{padding:4px 10px;border:none;border-radius:4px;background:#1a6b9e;color:#fff;font-size:11px;cursor:pointer}
.msg{position:fixed;top:20px;left:50%;transform:translateX(-50%);z-index:999;padding:12px 20px;border-radius:8px;display:none;font-weight:bold;text-align:center;font-size:14px;box-shadow:0 4px 16px rgba(0,0,0,.6)}
.ok{background:#2d6a4f;color:#d8f3dc}
.err{background:#6b0f0f;color:#ffcccc}
#langSelect{position:absolute;top:10px;right:10px;width:auto;padding:6px 8px;font-size:13px;background:#16213e;color:#8ab4f8;border:1px solid #333;border-radius:4px}
body{position:relative}
</style>
</head>
<body>
<select id="langSelect" onchange="setLang(this.value)"><option value="fr">FR</option><option value="en">EN</option><option value="es">ES</option></select>
<div class="topnav">
<a href="/config/basic" class="active" data-i18n="nav_basic">&#x1F4A1; Affichage &amp; Playlists</a>
<a href="/config/network" data-i18n="nav_network">&#x1F4F6; Wi-Fi &amp; BT</a>
<a href="/config/clock" data-i18n="nav_clock">&#x23F0; Horloge</a>
<a href="/config/media" data-i18n="nav_media">&#x1F4BF; M&eacute;dias</a>
</div>
<h1 data-i18n="h1">Affichage &amp; Playlists</h1>
<form id="basicForm" onsubmit="saveConfig(event)">
<div class="section">
<h2 data-i18n="sec_display">&#x1F4A1; Affichage</h2>
<div class="row"><label for="brightness" data-i18n="lbl_brightness">Luminosit&eacute; (%)</label><input id="brightness" type="range" min="0" max="100" value="50" oninput="document.getElementById('bval').textContent=this.value"><span id="bval" style="margin-left:8px;color:#ffd146;min-width:24px">50</span></div>
<div class="row"><label data-i18n="lbl_silent_boot">D&eacute;marrage silencieux</label><input id="silent_boot" type="checkbox"></div>
</div>
<div class="section">
<h2 data-i18n="sec_playlist">&#x1F4BF; Playlist</h2>
<div class="row"><label for="playlist" data-i18n="lbl_playlist_file">Playlist par d&eacute;faut</label><select id="playlist"></select></div>
<div class="row"><label data-i18n="lbl_random">Lecture al&eacute;atoire</label><input id="random" type="checkbox"></div>
</div>
<div class="section">
<h2 data-i18n="sec_manage_playlists">&#x2699; Gestion des playlists</h2>
<div class="desc" data-i18n="desc_gen_playlist">Cochez des dossiers pour g&eacute;n&eacute;rer une nouvelle playlist.</div>
<div class="mini-row">
<button type="button" class="mini-btn" onclick="selectAllGenDirs(true)" data-i18n="btn_select_all">Tout s&eacute;lectionner</button>
<button type="button" class="mini-btn" onclick="selectAllGenDirs(false)" data-i18n="btn_select_none">Rien s&eacute;lectionner</button>
</div>
<div id="genDirList" class="dirs"></div>
<div class="row"><label for="loadPlaylistSelect" data-i18n="lbl_load_playlist">Modifier une playlist existante</label><select id="loadPlaylistSelect" onchange="loadPlaylistForEdit()"><option value="">---</option></select></div>
<div class="row"><label for="playlistName" data-i18n="lbl_playlist_name">Nom playlist</label><input id="playlistName" data-i18n-placeholder="placeholder_playlist_name" placeholder="ex: MaPlaylist"></div>
<div class="btn-row"><button type="button" class="btn btn-gen" onclick="generatePlaylist()" data-i18n="btn_gen_playlist">&#x2699; G&eacute;n&eacute;rer playlist</button><button type="button" class="btn btn-del" id="genStopBtn" style="display:none" onclick="stopGeneratePlaylist()" data-i18n="btn_stop_gen">&#x23F9; Arr&ecirc;ter</button></div>
<div class="row"><label for="deletePlaylistSelect" data-i18n="lbl_delete_playlist">Supprimer</label><select id="deletePlaylistSelect"></select></div>
<div class="btn-row"><button type="button" class="btn btn-del" onclick="deletePlaylist()" data-i18n="btn_delete_playlist">&#x1F5D1; Supprimer playlist</button></div>
</div>
<div class="btn-row">
<button type="submit" class="btn btn-save" data-i18n="btn_save">&#x1F4BE; Enregistrer</button>
<button type="button" class="btn btn-reboot" onclick="saveAndReboot()" data-i18n="btn_save_reboot">&#x1F504; Enreg. &amp; Red&eacute;marrer</button>
<button type="button" class="btn btn-del" onclick="doReboot()" data-i18n="btn_reboot">&#x1F504; Red&eacute;marrer</button>
<button type="button" class="btn btn-resume" onclick="dmdResume()" data-i18n="btn_resume">&#x25B6; Reprendre DMD</button>
</div>
</form>
<div id="msg" class="msg"></div>
<script>
const PAGE_I18N={
fr:{title:'RecalBox DMD - Affichage',h1:'Affichage &amp; Playlists',nav_basic:'&#x1F4A1; Affichage &amp; Playlists',nav_network:'&#x1F4F6; Wi-Fi &amp; BT',nav_clock:'&#x23F0; Horloge',nav_media:'&#x1F4BF; Médias',sec_display:'&#x1F4A1; Affichage',sec_playlist:'&#x1F4BF; Playlist',lbl_brightness:'Luminosité (%)',lbl_silent_boot:'Démarrage silencieux',lbl_playlist_file:'Playlist par défaut',lbl_random:'Lecture aléatoire',lbl_delete_playlist:'Supprimer',btn_delete_playlist:'&#x1F5D1; Supprimer playlist',btn_save:'&#x1F4BE; Enregistrer',btn_save_reboot:'&#x1F504; Enreg. &amp; Redémarrer',btn_reboot:'&#x1F504; Redémarrer',btn_resume:'&#x25B6; Reprendre DMD',msg_saving:'Enregistrement...',msg_net_error:'Erreur réseau',msg_confirm_unsaved:'Des modifications non enregistrées seront perdues. Continuer ?',msg_confirm_reboot:'Redémarrer l\'ESP32 ?',msg_rebooting:'Redémarrage...',msg_dmd_resumed:'DMD repris',msg_select_playlist:'Sélectionnez une playlist à supprimer',msg_confirm_delete:'Supprimer ${0} ?',msg_confirm_delete_default:'ATTENTION : ${0} est actuellement la playlist par defaut ! La supprimer peut empecher le DMD de demarrer normalement. Continuer ?',msg_deleting:'Suppression...',msg_load_error:'Impossible de charger la config',sec_manage_playlists:'&#x2699; Gestion des playlists',desc_gen_playlist:'Cochez des dossiers pour générer une nouvelle playlist. &#x26A0;&#xFE0F; La création n\'est performante que sur des dossiers avec un nombre limité de fichiers. Pour des playlists contenant des dossiers conséquents, passez par l\'utilitaire RecalboxDMD_tool sur PC.',btn_select_all:'Tout sélectionner',btn_select_none:'Rien sélectionner',lbl_playlist_name:'Nom playlist',placeholder_playlist_name:'ex: MaPlaylist',btn_gen_playlist:'&#x2699; Générer playlist',msg_no_playlist_name:'Donnez un nom à la playlist',msg_select_folder:'Choisissez au moins un dossier',msg_generating:'Generation...',lbl_load_playlist:'Modifier une playlist existante',msg_scanning:'Analyse',msg_gen_busy:'Generation deja en cours ailleurs',msg_gen_start_error:'Impossible de demarrer la generation',msg_gen_leave_warning:'Une generation de playlist est en cours. Quitter la page ?',btn_stop_gen:'&#x23F9; Arreter',msg_confirm_stop_gen:'Arreter la generation ? La playlist en cours de creation sera supprimee.',msg_stopping_gen:'Arret playlist en cours, veuillez patienter...',msg_stop_gen_failed:'Echec de la demande d\'arret (reseau) -- reessayez'},
en:{title:'RecalBox DMD - Display',h1:'Display &amp; Playlists',nav_basic:'&#x1F4A1; Display &amp; Playlists',nav_network:'&#x1F4F6; Wi-Fi &amp; BT',nav_clock:'&#x23F0; Clock',nav_media:'&#x1F4BF; Media',sec_display:'&#x1F4A1; Display',sec_playlist:'&#x1F4BF; Playlist',lbl_brightness:'Brightness (%)',lbl_silent_boot:'Silent boot',lbl_playlist_file:'Default playlist',lbl_random:'Random playback',lbl_delete_playlist:'Delete',btn_delete_playlist:'&#x1F5D1; Delete playlist',btn_save:'&#x1F4BE; Save',btn_save_reboot:'&#x1F504; Save &amp; Reboot',btn_reboot:'&#x1F504; Reboot',btn_resume:'&#x25B6; Resume DMD',msg_saving:'Saving...',msg_net_error:'Network error',msg_confirm_unsaved:'Unsaved changes will be lost. Continue?',msg_confirm_reboot:'Reboot the ESP32?',msg_rebooting:'Rebooting...',msg_dmd_resumed:'DMD resumed',msg_select_playlist:'Select a playlist to delete',msg_confirm_delete:'Delete ${0}?',msg_confirm_delete_default:'WARNING: ${0} is currently the default playlist! Deleting it may prevent the DMD from starting normally. Continue?',msg_deleting:'Deleting...',msg_load_error:'Unable to load config',sec_manage_playlists:'&#x2699; Playlist management',desc_gen_playlist:'Check folders to generate a new playlist. &#x26A0;&#xFE0F; Generation is only fast on folders with a limited number of files. For playlists covering large folders, use the RecalboxDMD_tool utility on PC instead.',btn_select_all:'Select all',btn_select_none:'Select none',lbl_playlist_name:'Playlist name',placeholder_playlist_name:'e.g. MyPlaylist',btn_gen_playlist:'&#x2699; Generate playlist',msg_no_playlist_name:'Please name the playlist',msg_select_folder:'Select at least one folder',msg_generating:'Generating...',lbl_load_playlist:'Edit an existing playlist',msg_scanning:'Scanning',msg_gen_busy:'A generation is already running',msg_gen_start_error:'Could not start generation',msg_gen_leave_warning:'A playlist generation is in progress. Leave the page?',btn_stop_gen:'&#x23F9; Stop',msg_confirm_stop_gen:'Stop generation? The playlist being created will be deleted.',msg_stopping_gen:'Stopping playlist generation, please wait...',msg_stop_gen_failed:'Stop request failed (network) -- please retry'},
es:{title:'RecalBox DMD - Pantalla',h1:'Pantalla y listas',nav_basic:'&#x1F4A1; Pantalla y listas',nav_network:'&#x1F4F6; Wi-Fi y BT',nav_clock:'&#x23F0; Reloj',nav_media:'&#x1F4BF; Medios',sec_display:'&#x1F4A1; Pantalla',sec_playlist:'&#x1F4BF; Lista',lbl_brightness:'Brillo (%)',lbl_silent_boot:'Arranque silencioso',lbl_playlist_file:'Lista predeterminada',lbl_random:'Reproducción aleatoria',lbl_delete_playlist:'Eliminar',btn_delete_playlist:'&#x1F5D1; Eliminar lista',btn_save:'&#x1F4BE; Guardar',btn_save_reboot:'&#x1F504; Guardar y reiniciar',btn_reboot:'&#x1F504; Reiniciar',btn_resume:'&#x25B6; Reanudar DMD',msg_saving:'Guardando...',msg_net_error:'Error de red',msg_confirm_unsaved:'Los cambios no guardados se perderán. ¿Continuar?',msg_confirm_reboot:'¿Reiniciar el ESP32?',msg_rebooting:'Reiniciando...',msg_dmd_resumed:'DMD reanudado',msg_select_playlist:'Selecciona una lista para eliminar',msg_confirm_delete:'¿Eliminar ${0}?',msg_confirm_delete_default:'ATENCIÓN: ¡${0} es actualmente la lista predeterminada! Eliminarla puede impedir que el DMD arranque normalmente. ¿Continuar?',msg_deleting:'Eliminando...',msg_load_error:'No se pudo cargar la configuración',sec_manage_playlists:'&#x2699; Gestión de listas',desc_gen_playlist:'Marque las carpetas para generar una nueva lista. &#x26A0;&#xFE0F; La creación solo es rápida en carpetas con un número limitado de archivos. Para listas con carpetas voluminosas, use la utilidad RecalboxDMD_tool en el PC.',btn_select_all:'Seleccionar todo',btn_select_none:'Deseleccionar todo',lbl_playlist_name:'Nombre de la lista',placeholder_playlist_name:'ej: MiLista',btn_gen_playlist:'&#x2699; Generar lista',msg_no_playlist_name:'Póngale un nombre a la lista',msg_select_folder:'Elija al menos una carpeta',msg_generating:'Generando...',lbl_load_playlist:'Editar una lista existente',msg_scanning:'Analizando',msg_gen_busy:'Ya hay una generación en curso',msg_gen_start_error:'No se pudo iniciar la generación',msg_gen_leave_warning:'Hay una generación de lista en curso. ¿Salir de la página?',btn_stop_gen:'&#x23F9; Detener',msg_confirm_stop_gen:'¿Detener la generación? La lista en creación se eliminará.',msg_stopping_gen:'Deteniendo la generación de la lista, espere...'}
};
let currentLang='fr';
let _plNameAutoFilled=false; // suivi de la suggestion auto de nom (voir updatePlaylistNameSuggestion())
function tr(k){return (PAGE_I18N[currentLang]&&PAGE_I18N[currentLang][k])||PAGE_I18N.fr[k]||k;}
function trTpl(k,v){return tr(k).replace('${0}',v);}
function applyLang(backendLang){
  const stored=localStorage.getItem('dmd_lang');
  if(stored&&PAGE_I18N[stored]){currentLang=stored;}
  else if(backendLang&&PAGE_I18N[backendLang]){currentLang=backendLang;}
  else{const nav=(navigator.language||'').substring(0,2);currentLang=PAGE_I18N[nav]?nav:'fr';}
  document.documentElement.lang=currentLang;
  document.title=tr('title');
  document.querySelectorAll('[data-i18n]').forEach(function(el){el.innerHTML=tr(el.dataset.i18n);});
  document.querySelectorAll('[data-i18n-placeholder]').forEach(function(el){el.placeholder=tr(el.dataset.i18nPlaceholder);});
  document.getElementById('langSelect').value=currentLang;
}
function setLang(code){
  localStorage.setItem('dmd_lang',code);
  applyLang();
  fetch('/save-language',{method:'POST',body:'language='+code,headers:{'Content-Type':'application/x-www-form-urlencoded'}}).catch(function(){});
}
let _formDirty=false;
function stripAccents(s){return s.normalize('NFD').replace(new RegExp('['+String.fromCharCode(768)+'-'+String.fromCharCode(879)+']','g'),'').replace(/[^ -~]/g,'?');}
function showMsg(txt,ok){const el=document.getElementById('msg');el.textContent=txt;el.className='msg '+(ok?'ok':'err');el.style.display='block';if(window._msgTimer)clearTimeout(window._msgTimer);window._msgTimer=setTimeout(()=>{el.style.display='none';},5000);fetch('/dmd-pause',{method:'POST',body:new URLSearchParams({msg:stripAccents(txt),color:ok?'1':'2'}),headers:{'Content-Type':'application/x-www-form-urlencoded'}}).catch(()=>{});}
// showMsg() sans miroir DMD : necessaire pour la confirmation de reprise
// (dmdResume()) -- /dmd-pause remet justement le DMD en mode pause/config,
// ce qui annulait la reprise a peine effectuee (ecran fige juste apres
// "DMD repris", confirme en test reel).
function showMsgLocal(txt,ok){const el=document.getElementById('msg');el.textContent=txt;el.className='msg '+(ok?'ok':'err');el.style.display='block';if(window._msgTimer)clearTimeout(window._msgTimer);window._msgTimer=setTimeout(()=>{el.style.display='none';},5000);}
function serialize(){return new URLSearchParams({brightness:document.getElementById('brightness').value,info:document.getElementById('silent_boot').checked?'0':'1',playlist:document.getElementById('playlist').value,random:document.getElementById('random').checked?'1':'0'});}
function saveConfig(e){if(e&&e.preventDefault)e.preventDefault();showMsg(tr('msg_saving'),true);return fetch('/save',{method:'POST',body:serialize(),headers:{'Content-Type':'application/x-www-form-urlencoded'}}).then(r=>r.text()).then(t=>{showMsg(t.includes('OK')?tr('msg_saving'):t,t.includes('OK'));if(t.includes('OK'))_formDirty=false;}).catch(()=>showMsg(tr('msg_net_error'),false));}
function doReboot(skipConfirm){if(_formDirty&&!confirm(tr('msg_confirm_unsaved')))return;if(!skipConfirm&&!confirm(tr('msg_confirm_reboot')))return;showMsg(tr('msg_rebooting'),true);fetch('/reboot').catch(()=>{});}
// skipConfirm=true (2026-07-29) : "Enreg. & Redemarrer" a deja un intitule
// explicite -- redemander confirmation juste apres la sauvegarde est
// redondant, contrairement au bouton "Redemarrer" seul.
function saveAndReboot(){saveConfig().then(()=>setTimeout(()=>doReboot(true),400));}
function dmdResume(){if(_formDirty&&!confirm(tr('msg_confirm_unsaved')))return;fetch('/dmd-resume',{method:'POST'}).then(()=>showMsgLocal(tr('msg_dmd_resumed'),true)).catch(()=>showMsg(tr('msg_net_error'),false));}
// Reprise auto a la fermeture -- ESSAYEE puis RETIREE (2026-07-29) : aucun
// moyen fiable de distinguer une vraie fermeture d'onglet/navigateur d'un
// simple rafraichissement de page (habitude trop ancree pour l'utilisateur,
// faux positifs trop frequents -- ni le JS ni le serveur ne peuvent
// distinguer les deux cas, une connexion qui se ferme se ressemble dans
// tous les cas).
// B (plan cache_master_gifs, retour test reel 2026-08-01) -- retry (5
// tentatives, 500ms d'ecart) SEULEMENT sur echec reel (fetch/parse), jamais
// sur une reponse vide reussie (contrairement a loadGenDirs()/loadDirs() :
// une liste de playlists vide est un etat legitime, pas forcement une
// anomalie transitoire). Meme cause que loadDirs() : un simple
// fetch().catch(()=>{}) laissait les 3 listes vides en silence des qu'une
// seule requete /lsplaylists echouait au chargement de la page.
async function fillPlaylists(selVal){
  for(let attempt=0;attempt<5;attempt++){
    try{
      const pl=await(await fetch('/lsplaylists')).json();
      const sel=document.getElementById('playlist');const del=document.getElementById('deletePlaylistSelect');const load=document.getElementById('loadPlaylistSelect');
      sel.innerHTML='';del.innerHTML='';load.innerHTML='';
      const opt=document.createElement('option');opt.value='';opt.textContent='---';sel.appendChild(opt);
      const opt2=document.createElement('option');opt2.value='';opt2.textContent='---';del.appendChild(opt2);
      const opt3=document.createElement('option');opt3.value='';opt3.textContent='---';load.appendChild(opt3);
      pl.forEach(p=>{const o=document.createElement('option');o.value=p;o.textContent=p;if(p===selVal)o.selected=true;sel.appendChild(o);const o2=document.createElement('option');o2.value=p;o2.textContent=p;del.appendChild(o2);const o3=document.createElement('option');o3.value=p;o3.textContent=p;load.appendChild(o3);});
      return;
    }catch(e){ if(attempt<4) await new Promise(r=>setTimeout(r,500)); }
  }
}
function deletePlaylist(){const name=document.getElementById('deletePlaylistSelect').value;if(!name){showMsg(tr('msg_select_playlist'),false);return;}
  // Playlist par defaut (demande utilisateur, 2026-07-30) : popup de
  // confirmation distincte et plus explicite si la playlist qu'on s'apprete
  // a supprimer est aussi celle configuree par defaut -- 'playlist' (le
  // select "Playlist par defaut") est deja pre-selectionne sur cette valeur
  // par fillPlaylists(), simple comparaison, aucun nouvel appel serveur.
  const isDefault=document.getElementById('playlist').value===name;
  if(isDefault){if(!confirm(trTpl('msg_confirm_delete_default',name)))return;}
  else if(!confirm(trTpl('msg_confirm_delete',name)))return;
  // showMsgLocal (pas showMsg) : meme raison que dans generatePlaylist()
  // ci-dessous -- eviter le fetch('/dmd-pause') interne de showMsg() en
  // concurrence avec le fetch('/delete-playlist') juste apres.
  showMsgLocal(tr('msg_deleting'),true);
  fetch('/delete-playlist',{method:'POST',body:new URLSearchParams({name:name}),headers:{'Content-Type':'application/x-www-form-urlencoded'}}).then(r=>r.text()).then(t=>{showMsg(t,t.includes('OK'));fillPlaylists('');}).catch(()=>showMsg(tr('msg_net_error'),false));}
// Generation de playlist (deplacee depuis MEDIA -- demande utilisateur :
// la gestion des playlists va dans Affichage, la gestion physique des
// fichiers/dossiers reste dans MEDIA). Liste des dossiers ici pour
// COCHER uniquement -- pas d'icone d'ouverture/consultation du contenu,
// reservee a la page MEDIA.
// Retry (5 tentatives, 500ms d'ecart) : un simple fetch().catch(()=>{})
// laissait la liste vide en silence, sans aucun message ni nouvelle
// tentative, si cette requete echouait une seule fois au chargement de la
// page (observe en test reel 2026-07-29 -- l'endpoint repond pourtant bien
// quand on le teste isolement juste apres). Retente aussi si la reponse est
// VIDE (pas juste en echec reseau) : sur ce projet, /lsgifdirs ne renvoie
// [] que si plGenIsActive() etait actif pile a ce moment (transitoire) --
// il existe toujours des dossiers reels, donc une liste vide est ici
// toujours anormale/transitoire, jamais un etat legitime a accepter tel
// quel.
// Cache sessionStorage PARTAGE avec la page MEDIA (meme cle,
// 'dmd_gifdirs_cache') -- demande utilisateur 2026-08-02 : le va-et-vient
// frequent entre Affichage et MEDIA redemandait /lsgifdirs a chaque fois,
// avec le risque d'echec reseau observe en test reel. Affiche IMMEDIATEMENT
// le contenu en cache si present (aucune attente reseau), PUIS rafraichit
// en arriere-plan et met a jour le cache.
function readDirsCache(){
  try{
    const raw=sessionStorage.getItem('dmd_gifdirs_cache');
    return raw?JSON.parse(raw):null;
  }catch(e){return null;}
}
function writeDirsCache(dirs){
  try{sessionStorage.setItem('dmd_gifdirs_cache',JSON.stringify(dirs));}catch(e){}
}
function renderGenDirs(dirs){
  const list=document.getElementById('genDirList');
  // B (plan cache_master_gifs) -- tri alphabetique cote JS, fonctionne quel
  // que soit l'etat/l'origine de cache_master_gifs.dat.
  const sorted=dirs.slice().sort((a,b)=>{
    const na=(a&&typeof a==='object')?a.name:a;
    const nb=(b&&typeof b==='object')?b.name:b;
    return na.localeCompare(nb);
  });
  list.innerHTML='';
  sorted.forEach(d=>{
    const name=(d&&typeof d==='object')?d.name:d;
    const row=document.createElement('label');
    row.innerHTML='<input type="checkbox" value="'+name+'"><span class="name">&#x1F4C1; '+name+'</span>';
    list.appendChild(row);
  });
}
async function loadGenDirs(){
  const cached=readDirsCache();
  if(cached&&cached.length)renderGenDirs(cached);
  for(let attempt=0;attempt<5;attempt++){
    if(attempt>0)await new Promise(r=>setTimeout(r,500));
    try{
      const dirs=await(await fetch('/lsgifdirs')).json();
      if(!dirs.length&&attempt<4)continue;
      renderGenDirs(dirs);
      writeDirsCache(dirs);
      return;
    }catch(e){}
  }
}
function selectAllGenDirs(v){document.querySelectorAll('#genDirList input').forEach(i=>i.checked=v);updatePlaylistNameSuggestion();}
// Suggestion de nom (demande utilisateur) : si exactement un dossier est
// coche, pre-remplit "Nom playlist" avec son nom -- efface a la 1ere prise
// de controle du champ par l'utilisateur (focus), jamais ecrase apres. Ne
// s'applique QUE pour une NOUVELLE playlist (loadPlaylistSelect vide) --
// ne touche jamais au nom d'une playlist existante en cours d'edition.
function updatePlaylistNameSuggestion(){
  if(document.getElementById('loadPlaylistSelect').value)return;
  const checked=[].slice.call(document.querySelectorAll('#genDirList input:checked'));
  const nameEl=document.getElementById('playlistName');
  if(checked.length===1){
    if(_plNameAutoFilled||nameEl.value==='') { nameEl.value=checked[0].value; _plNameAutoFilled=true; }
  } else if(_plNameAutoFilled){
    nameEl.value=''; _plNameAutoFilled=false;
  }
}
document.getElementById('genDirList').addEventListener('change',function(e){if(e.target&&e.target.type==='checkbox')updatePlaylistNameSuggestion();});
document.getElementById('playlistName').addEventListener('focus',function(){if(_plNameAutoFilled){this.value='';_plNameAutoFilled=false;}});
// Modifier une playlist existante (demande utilisateur) : precoche les
// dossiers qu'elle referme deja au lieu de forcer une re-selection complete
// avant de regenerer (la regeneration ecrase le fichier a l'identique --
// meme nom).
function loadPlaylistForEdit(){
  const raw=document.getElementById('loadPlaylistSelect').value;
  if(!raw){
    // Retour a "---" (demande utilisateur) : decoche tout plutot que de
    // laisser les cases d'une precedente edition/selection.
    selectAllGenDirs(false);
    document.getElementById('playlistName').value='';
    _plNameAutoFilled=false;
    return;
  }
  // raw vient de /lsplaylists, QUI INCLUT ".txt" -- retire l'extension avant
  // de l'utiliser : sinon /playlist-dirs cherche "name.txt.txt" (introuvable,
  // dossiers jamais precoches) et une regeneration ecrirait un fichier
  // "name.txt.txt" au lieu d'ecraser l'original.
  const name=raw.replace(/\.txt$/i,'');
  _plNameAutoFilled=false; // le nom vient d'une playlist existante, jamais ecrase par la suggestion auto
  document.getElementById('playlistName').value=name;
  fetch('/playlist-dirs?name='+encodeURIComponent(name)).then(r=>r.json()).then(dirs=>{
    document.querySelectorAll('#genDirList input[type=checkbox]').forEach(c=>{c.checked=dirs.indexOf(c.value)>=0;});
  }).catch(()=>showMsg(tr('msg_net_error'),false));
}
// Verrouille/deverrouille toute la page pendant la generation -- empeche de
// lancer une autre action (upload, suppression...) pendant qu'un scan est en
// cours, en plus du garde cote serveur (g_plGenStatus.active, RecalBox_DMD.ino).
function setPageBusy(busy){document.querySelectorAll('button,input,select').forEach(e=>{if(e.id!=='genStopBtn')e.disabled=busy;});document.body.classList.toggle('gen-busy',busy);
  document.getElementById('genStopBtn').style.display=busy?'inline-block':'none';
  if(busy)document.getElementById('genStopBtn').disabled=false; // etat frais a chaque nouvelle generation (peut avoir ete desactive par un arret precedent)
  // beforeunload : la generation continue cote serveur meme si l'utilisateur
  // quitte la page (machine a etats independante du navigateur), mais le
  // polling JS s'arreterait -- avertir plutot que laisser croire a un blocage
  // silencieux si jamais le verrou CSS est contourne (ex. navigation clavier).
  if(busy)window.onbeforeunload=function(){return tr('msg_gen_leave_warning');};else window.onbeforeunload=null;
}
// _stopRequestPending (2026-07-30, demande utilisateur : "Arreter" echoue
// presque a chaque fois) : le serveur ESP32 est mono-thread (une seule
// requete HTTP traitee a la fois) et la boucle de polling
// (generatePlaylist(), toutes les 700ms) tourne EN
// PERMANENCE pendant qu'un scan est actif -- la fenetre de collision avec
// la requete d'arret (qui doit pourtant reussir vite) est donc quasi
// garantie, le retry existant (3x/500ms) retombant lui-meme regulierement
// sur le sondage suivant. Les boucles de polling verifient ce drapeau et
// SAUTENT leur propre requete pendant qu'un arret est en cours, laissant le
// champ libre au serveur mono-thread plutot que de continuer a le
// solliciter en parallele.
let _stopRequestPending=false;
async function stopGeneratePlaylist(){
  if(!confirm(tr('msg_confirm_stop_gen')))return;
  // Message persistant immediat (pas de setTimeout d'auto-masquage) : le
  // temps reel d'arret depend de la lenteur SD en cours (jusqu'a ~1 min
  // observe en test reel) -- sans ca, rien n'indique que le clic a bien ete
  // pris en compte pendant cette attente. Reste affiche jusqu'a ce que la
  // boucle de polling deja en cours dans generatePlaylist() detecte la fin
  // reelle (!active) et affiche le resultat definitif.
  const msgEl=document.getElementById('msg');
  if(window._msgTimer)clearTimeout(window._msgTimer);
  msgEl.className='msg ok';msgEl.style.display='block';msgEl.textContent=tr('msg_stopping_gen');
  document.getElementById('genStopBtn').disabled=true; // evite un double-clic pendant l'attente
  _stopRequestPending=true;
  // Retry (3 tentatives, 500ms d'ecart) : un simple fetch().catch(()=>{})
  // avalait silencieusement tout echec -- si cette requete tombe pile au
  // meme moment qu'un sondage de statut en cours (serveur ESP32 mono-thread,
  // une seule requete traitee a la fois), elle peut echouer sans laisser de
  // trace, bloquant l'utilisateur sur "Arret en cours..." indefiniment sans
  // que rien ne soit jamais retente (observe en test reel 2026-07-29).
  let ok=false;
  for(let attempt=0;attempt<3&&!ok;attempt++){
    if(attempt>0)await new Promise(r=>setTimeout(r,500));
    try{const r=await fetch('/generate-playlist-stop',{method:'POST'});ok=r.ok;}catch(e){ok=false;}
  }
  _stopRequestPending=false;
  if(!ok){
    msgEl.className='msg err';
    msgEl.textContent=tr('msg_stop_gen_failed');
    document.getElementById('genStopBtn').disabled=false;
  }
}
async function generatePlaylist(){
  const name=document.getElementById('playlistName').value.trim();
  const dirs=[].slice.call(document.querySelectorAll('#genDirList input:checked')).map(i=>i.value).join(',');
  if(!name){showMsg(tr('msg_no_playlist_name'),false);return;}
  if(!dirs){showMsg(tr('msg_select_folder'),false);return;}
  setPageBusy(true);
  const msgEl=document.getElementById('msg');
  if(window._msgTimer)clearTimeout(window._msgTimer);
  msgEl.className='msg ok';msgEl.style.display='block';msgEl.textContent=tr('msg_generating');
  // finishGen() : affichage final partage entre la fin du polling (generation
  // classique, asynchrone) et une reponse DEJA terminee recue directement au
  // POST initial (filterPlaylistFromMaster() -- filtrage synchrone depuis
  // le fichier maitre interne, aucune tache creee cote serveur puisque aucun
  // scan de /gifs/ n'est necessaire). Avant ce correctif, une reussite synchrone tombait
  // dans la meme branche que "generation deja en cours"/erreur reseau (ci-
  // dessous) : jamais de minuteur d'auto-masquage (message fige en rouge en
  // permanence) ni de rafraichissement de la liste des playlists (nouvelle
  // playlist invisible sans F5) -- constate en test reel 2026-07-30.
  function finishGen(resultText,ok){
    msgEl.textContent=resultText||tr('msg_gen_start_error');
    msgEl.className='msg '+(ok?'ok':'err');
    if(window._msgTimer)clearTimeout(window._msgTimer);
    window._msgTimer=setTimeout(()=>{msgEl.style.display='none';},5000);
    fetch('/dmd-pause',{method:'POST',body:new URLSearchParams({msg:stripAccents(resultText||''),color:'1'}),headers:{'Content-Type':'application/x-www-form-urlencoded'}}).catch(()=>{});
    document.getElementById('playlistName').value='';
    fillPlaylists('');
  }
  let started=false;
  try{
    const r=await fetch('/generate-playlist',{method:'POST',body:new URLSearchParams({name:name,dirs:dirs}),headers:{'Content-Type':'application/x-www-form-urlencoded'}});
    const t=await r.text();
    started=t.includes('STARTED');
    if(!started){
      if(r.ok&&t.startsWith('OK')){
        // Filtrage synchrone depuis le fichier maitre interne : deja termine, pas de tache a suivre.
        finishGen(t,true);
        setPageBusy(false);
        return;
      }
      msgEl.textContent=(r.status===409)?tr('msg_gen_busy'):t;msgEl.className='msg err';
      if(window._msgTimer)clearTimeout(window._msgTimer);
      window._msgTimer=setTimeout(()=>{msgEl.style.display='none';},5000);
    }
  }catch(e){
    // La reponse ("STARTED") peut echouer a arriver jusqu'au navigateur
    // (heap degrade apres plusieurs generations enchainees dans la meme
    // session) alors que la tache a deja bien demarre cote serveur --
    // observe en test reel (2026-07-29) : generation qui continue tres
    // normalement (DMD/logs), mais page qui affiche "erreur reseau" et
    // abandonne tout suivi. Avant d'abandonner, verifier le statut reel
    // plutot que de perdre le suivi d'une generation pourtant en cours.
    try{
      const st=await(await fetch('/generate-playlist-status')).json();
      started=!!st.active;
    }catch(e2){started=false;}
    if(!started){
      msgEl.textContent=tr('msg_net_error');msgEl.className='msg err';
      if(window._msgTimer)clearTimeout(window._msgTimer);
      window._msgTimer=setTimeout(()=>{msgEl.style.display='none';},5000);
    }
  }
  if(!started){setPageBusy(false);return;}
  // Polling de progression (le WebServer ESP32 est mono-thread : impossible
  // de pousser une mise a jour depuis le serveur pendant que le scan tourne,
  // la page doit donc interroger periodiquement /generate-playlist-status).
  while(true){
    await new Promise(res=>setTimeout(res,700));
    if(_stopRequestPending)continue; // laisse la requete d'arret passer seule (serveur mono-thread)
    let st;
    try{
      // AbortController : sans ca, une seule requete de statut qui reste
      // bloquee (observe en test reel 2026-07-28 -- page figee a 20/165
      // pendant que le DMD, lui, continuait a avancer normalement) fige le
      // polling pour de bon, la boucle n'atteignant jamais l'iteration
      // suivante puisqu'elle reste indefiniment en attente du fetch().
      // 9000ms (pas 4000) : certains dossiers ont des lenteurs SD localisees
      // ou plusieurs fichiers consecutifs prennent chacun plusieurs secondes
      // (Halloween/Vertical_DMD/tous, confirme en test reel) -- un timeout
      // trop court se remettait lui-meme a echouer en boucle sur ces series,
      // sans jamais laisser au serveur (mono-thread, deja occupe par le scan)
      // le temps de repondre. Mitigation legere : pas une elimination du gel
      // possible (deplacer le scan sur une tache dediee reglerait la cause,
      // pas fait ici sur decision explicite -- juste tolerer une serie plus
      // longue avant d'abandonner une requete).
      const ctrl=new AbortController();
      const abortTimer=setTimeout(()=>ctrl.abort(),9000);
      st=await(await fetch('/generate-playlist-status',{signal:ctrl.signal})).json();
      clearTimeout(abortTimer);
    }catch(e){continue;}
    if(!st.active){
      finishGen(st.result,st.done);
      break;
    }
    // Compteur numerique reintroduit (2026-07-29) : retire le 2026-07-28 car
    // playlistGenStep() tournait alors dans loop(), donc un blocage SD figeait
    // aussi le serveur web -- le compteur affiche restait fige en meme temps
    // que tout le reste, donnant une fausse impression de gel. Depuis le
    // passage a playlistGenTask() (tache FreeRTOS dediee), /generate-
    // playlist-status repond toujours rapidement (plGenStatusMutex jamais
    // tenu pendant un acces SD) meme pendant un dossier lent -- le compteur
    // redevient donc une information fiable plutot qu'un faux signal de gel.
    msgEl.textContent=tr('msg_scanning')+': '+st.dir+' ('+st.dirIdx+'/'+st.totalDirs+') - '+st.curDirGifs+' GIFs ('+st.gifs+' total)';
  }
  setPageBusy(false);
}
function loadConfig(){return fetch('/load').then(r=>r.json()).then(d=>{document.getElementById('brightness').value=Math.max(0,Math.min(100,parseInt(d.brightness||50,10)));document.getElementById('bval').textContent=document.getElementById('brightness').value;document.getElementById('silent_boot').checked=d.info==='0';fillPlaylists(d.playlist||'');document.getElementById('random').checked=d.random==='1';}).catch(()=>showMsg(tr('msg_load_error'),false));}
localStorage.setItem('dmd_last_section','basic');
// loadGenDirs() enchainee APRES /lang+/load (jamais en parallele) : le
// WebServer ESP32 ne traite qu'une requete a la fois -- des fetch()
// concurrents corrompent silencieusement l'une des reponses (bug deja
// documente et corrige sur MEDIA via queuedFetch(), reintroduit ici par
// inattention lors de l'ajout de la generation de playlist, v79).
fetch('/lang').then(r=>r.json()).then(d=>{applyLang(d.language);return loadConfig();}).catch(()=>{applyLang();return loadConfig();}).then(loadGenDirs);
document.getElementById('basicForm').addEventListener('input',()=>{_formDirty=true;});
</script>
</body>
</html>
)rawliteral";

static const char WEB_CONFIG_NETWORK_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="fr">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>RecalBox DMD - Wi-Fi</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:'Segoe UI',Tahoma,sans-serif;background:#1a1a2e;color:#eee;padding:16px;max-width:700px;margin:auto}
h1{color:#ffd146;text-align:center;margin:8px 0 14px;font-size:22px;border-bottom:2px solid #ffd146;padding-bottom:8px}
.topnav{display:flex;gap:6px;flex-wrap:wrap;justify-content:center;margin-bottom:14px}
.topnav a{padding:8px 14px;border-radius:6px;background:#16213e;color:#8ab4f8;font-size:13px;font-weight:600;text-decoration:none}
.topnav a.active{background:#8ab4f8;color:#1a1a2e}
.section{background:#16213e;border-radius:8px;padding:16px;margin:12px 0}
h2{color:#8ab4f8;font-size:15px;margin:0 0 10px;border-left:3px solid #8ab4f8;padding-left:8px}
.row{display:flex;flex-wrap:wrap;align-items:center;margin:10px 0}
.row label{flex:0 0 150px;font-size:14px;color:#aaa}
.row input,.row select{flex:1;min-width:120px;padding:8px 10px;border:1px solid #333;border-radius:4px;background:#0f3460;color:#eee;font-size:14px}
.row input[type=checkbox]{flex:0 0 20px;width:20px;height:20px;margin:0 8px 0 0}
.btn-row{display:flex;gap:10px;justify-content:center;margin:18px 0;flex-wrap:wrap}
.btn{padding:12px 20px;border:none;border-radius:6px;font-size:13px;font-weight:bold;cursor:pointer}
.btn-save{background:#ffd146;color:#1a1a2e}
.btn-reboot{background:#e63946;color:#fff}
.btn-resume{background:#2d6a4f;color:#fff}
.btn-del{background:#555;color:#fff}
.msg{position:fixed;top:20px;left:50%;transform:translateX(-50%);z-index:999;padding:12px 20px;border-radius:8px;display:none;font-weight:bold;text-align:center;font-size:14px;box-shadow:0 4px 16px rgba(0,0,0,.6)}
.ok{background:#2d6a4f;color:#d8f3dc}
.err{background:#6b0f0f;color:#ffcccc}
#langSelect{position:absolute;top:10px;right:10px;width:auto;padding:6px 8px;font-size:13px;background:#16213e;color:#8ab4f8;border:1px solid #333;border-radius:4px}
body{position:relative}
</style>
</head>
<body>
<select id="langSelect" onchange="setLang(this.value)"><option value="fr">FR</option><option value="en">EN</option><option value="es">ES</option></select>
<div class="topnav">
<a href="/config/basic" data-i18n="nav_basic">&#x1F4A1; Affichage &amp; Playlists</a>
<a href="/config/network" class="active" data-i18n="nav_network">&#x1F4F6; Wi-Fi &amp; BT</a>
<a href="/config/clock" data-i18n="nav_clock">&#x23F0; Horloge</a>
<a href="/config/media" data-i18n="nav_media">&#x1F4BF; M&eacute;dias</a>
</div>
<h1 data-i18n="h1">Wi-Fi &amp; Bluetooth</h1>
<form id="networkForm" onsubmit="saveConfig(event)">
<div class="section">
<h2 data-i18n="sec_wifi">&#x1F4F6; Wi-Fi</h2>
<div class="row"><label data-i18n="lbl_enabled">Activ&eacute;</label><input id="wifi_enabled" type="checkbox"></div>
<div class="row"><label for="wifi_ssid" data-i18n="lbl_network">R&eacute;seau</label><select id="wifi_ssid"><option value="" data-i18n="opt_scanning">-- Scan en cours... --</option></select></div>
<div class="row"><label for="wifi_password" data-i18n="lbl_password">Mot de passe</label><input id="wifi_password" type="password"></div>
<div class="row"><label data-i18n="lbl_static_ip">IP statique</label><input id="wifi_static_enabled" type="checkbox"></div>
<div class="row"><label for="wifi_static_ip" data-i18n="lbl_fixed_ip">IP fixe</label><input id="wifi_static_ip"></div>
<div class="row"><label for="wifi_gateway" data-i18n="lbl_gateway">Passerelle</label><input id="wifi_gateway"></div>
<div class="row"><label for="wifi_subnet" data-i18n="lbl_subnet">Masque</label><input id="wifi_subnet"></div>
<div class="row"><label for="wifi_dns1" data-i18n="lbl_dns1">DNS 1</label><input id="wifi_dns1"></div>
<div class="row"><label for="wifi_dns2" data-i18n="lbl_dns2">DNS 2</label><input id="wifi_dns2"></div>
</div>
<div class="section">
<h2 data-i18n="sec_bt">&#x1F4F1; Bluetooth</h2>
<div class="row"><label data-i18n="lbl_enabled">Activ&eacute;</label><input id="bluetooth_enabled" type="checkbox"></div>
<div class="row"><label for="bluetooth_name" data-i18n="lbl_bt_name">Nom</label><input id="bluetooth_name"></div>
</div>
<div class="section">
<h2 data-i18n="sec_mqtt">&#x1F310; MQTT</h2>
<div class="row"><label for="recalbox_ip" data-i18n="lbl_mqtt_ip">IP Recalbox</label><input id="recalbox_ip"></div>
</div>
<div class="btn-row">
<button type="submit" class="btn btn-save" data-i18n="btn_save">&#x1F4BE; Enregistrer</button>
<button type="button" class="btn btn-reboot" onclick="saveAndReboot()" data-i18n="btn_save_reboot">&#x1F504; Enreg. &amp; Red&eacute;marrer</button>
<button type="button" class="btn btn-del" onclick="doReboot()" data-i18n="btn_reboot">&#x1F504; Red&eacute;marrer</button>
<button type="button" class="btn btn-resume" onclick="dmdResume()" data-i18n="btn_resume">&#x25B6; Reprendre DMD</button>
</div>
</form>
<div id="msg" class="msg"></div>
<script>
const PAGE_I18N={
fr:{title:'RecalBox DMD - Wi-Fi',h1:'Wi-Fi &amp; Bluetooth',nav_basic:'&#x1F4A1; Affichage &amp; Playlists',nav_network:'&#x1F4F6; Wi-Fi &amp; BT',nav_clock:'&#x23F0; Horloge',nav_media:'&#x1F4BF; Médias',sec_wifi:'&#x1F4F6; Wi-Fi',sec_bt:'&#x1F4F1; Bluetooth',sec_mqtt:'&#x1F310; MQTT',lbl_enabled:'Activé',lbl_network:'Réseau',lbl_password:'Mot de passe',lbl_static_ip:'IP statique',lbl_fixed_ip:'IP fixe',lbl_gateway:'Passerelle',lbl_subnet:'Masque',lbl_dns1:'DNS 1',lbl_dns2:'DNS 2',lbl_bt_name:'Nom',lbl_mqtt_ip:'IP Recalbox',opt_scanning:'-- Scan en cours... --',opt_select:'-- Sélectionnez --',opt_scan_error:'Erreur scan',btn_save:'&#x1F4BE; Enregistrer',btn_save_reboot:'&#x1F504; Enreg. &amp; Redémarrer',btn_reboot:'&#x1F504; Redémarrer',btn_resume:'&#x25B6; Reprendre DMD',msg_saving:'Enregistrement...',msg_net_error:'Erreur réseau',msg_confirm_unsaved:'Des modifications non enregistrées seront perdues. Continuer ?',msg_confirm_reboot:'Redémarrer l\'ESP32 ?',msg_rebooting:'Redémarrage...',msg_dmd_resumed:'DMD repris',msg_load_error:'Impossible de charger la config'},
en:{title:'RecalBox DMD - Wi-Fi',h1:'Wi-Fi &amp; Bluetooth',nav_basic:'&#x1F4A1; Display &amp; Playlists',nav_network:'&#x1F4F6; Wi-Fi &amp; BT',nav_clock:'&#x23F0; Clock',nav_media:'&#x1F4BF; Media',sec_wifi:'&#x1F4F6; Wi-Fi',sec_bt:'&#x1F4F1; Bluetooth',sec_mqtt:'&#x1F310; MQTT',lbl_enabled:'Enabled',lbl_network:'Network',lbl_password:'Password',lbl_static_ip:'Static IP',lbl_fixed_ip:'Fixed IP',lbl_gateway:'Gateway',lbl_subnet:'Subnet mask',lbl_dns1:'DNS 1',lbl_dns2:'DNS 2',lbl_bt_name:'Name',lbl_mqtt_ip:'Recalbox IP',opt_scanning:'-- Scanning... --',opt_select:'-- Select --',opt_scan_error:'Scan error',btn_save:'&#x1F4BE; Save',btn_save_reboot:'&#x1F504; Save &amp; Reboot',btn_reboot:'&#x1F504; Reboot',btn_resume:'&#x25B6; Resume DMD',msg_saving:'Saving...',msg_net_error:'Network error',msg_confirm_unsaved:'Unsaved changes will be lost. Continue?',msg_confirm_reboot:'Reboot the ESP32?',msg_rebooting:'Rebooting...',msg_dmd_resumed:'DMD resumed',msg_load_error:'Unable to load config'},
es:{title:'RecalBox DMD - Wi-Fi',h1:'Wi-Fi y Bluetooth',nav_basic:'&#x1F4A1; Pantalla y listas',nav_network:'&#x1F4F6; Wi-Fi y BT',nav_clock:'&#x23F0; Reloj',nav_media:'&#x1F4BF; Medios',sec_wifi:'&#x1F4F6; Wi-Fi',sec_bt:'&#x1F4F1; Bluetooth',sec_mqtt:'&#x1F310; MQTT',lbl_enabled:'Activado',lbl_network:'Red',lbl_password:'Contraseña',lbl_static_ip:'IP estática',lbl_fixed_ip:'IP fija',lbl_gateway:'Puerta de enlace',lbl_subnet:'Máscara de subred',lbl_dns1:'DNS 1',lbl_dns2:'DNS 2',lbl_bt_name:'Nombre',lbl_mqtt_ip:'IP de Recalbox',opt_scanning:'-- Escaneando... --',opt_select:'-- Seleccione --',opt_scan_error:'Error de escaneo',btn_save:'&#x1F4BE; Guardar',btn_save_reboot:'&#x1F504; Guardar y reiniciar',btn_reboot:'&#x1F504; Reiniciar',btn_resume:'&#x25B6; Reanudar DMD',msg_saving:'Guardando...',msg_net_error:'Error de red',msg_confirm_unsaved:'Los cambios no guardados se perderán. ¿Continuar?',msg_confirm_reboot:'¿Reiniciar el ESP32?',msg_rebooting:'Reiniciando...',msg_dmd_resumed:'DMD reanudado',msg_load_error:'No se pudo cargar la configuración'}
};
let currentLang='fr';
function tr(k){return (PAGE_I18N[currentLang]&&PAGE_I18N[currentLang][k])||PAGE_I18N.fr[k]||k;}
function applyLang(backendLang){
  const stored=localStorage.getItem('dmd_lang');
  if(stored&&PAGE_I18N[stored]){currentLang=stored;}
  else if(backendLang&&PAGE_I18N[backendLang]){currentLang=backendLang;}
  else{const nav=(navigator.language||'').substring(0,2);currentLang=PAGE_I18N[nav]?nav:'fr';}
  document.documentElement.lang=currentLang;
  document.title=tr('title');
  document.querySelectorAll('[data-i18n]').forEach(function(el){el.innerHTML=tr(el.dataset.i18n);});
  document.getElementById('langSelect').value=currentLang;
}
function setLang(code){
  localStorage.setItem('dmd_lang',code);
  applyLang();
  fetch('/save-language',{method:'POST',body:'language='+code,headers:{'Content-Type':'application/x-www-form-urlencoded'}}).catch(function(){});
}
let savedSsid='';
let _formDirty=false;
function stripAccents(s){return s.normalize('NFD').replace(new RegExp('['+String.fromCharCode(768)+'-'+String.fromCharCode(879)+']','g'),'').replace(/[^ -~]/g,'?');}
function showMsg(txt,ok){const el=document.getElementById('msg');el.textContent=txt;el.className='msg '+(ok?'ok':'err');el.style.display='block';if(window._msgTimer)clearTimeout(window._msgTimer);window._msgTimer=setTimeout(()=>{el.style.display='none';},5000);fetch('/dmd-pause',{method:'POST',body:new URLSearchParams({msg:stripAccents(txt),color:ok?'1':'2'}),headers:{'Content-Type':'application/x-www-form-urlencoded'}}).catch(()=>{});}
function showMsgLocal(txt,ok){const el=document.getElementById('msg');el.textContent=txt;el.className='msg '+(ok?'ok':'err');el.style.display='block';if(window._msgTimer)clearTimeout(window._msgTimer);window._msgTimer=setTimeout(()=>{el.style.display='none';},5000);}
function serialize(){return new URLSearchParams({wifi_enabled:document.getElementById('wifi_enabled').checked?'1':'0',wifi_ssid:document.getElementById('wifi_ssid').value,wifi_password:document.getElementById('wifi_password').value,wifi_static_enabled:document.getElementById('wifi_static_enabled').checked?'1':'0',wifi_static_ip:document.getElementById('wifi_static_ip').value,wifi_gateway:document.getElementById('wifi_gateway').value,wifi_subnet:document.getElementById('wifi_subnet').value,wifi_dns1:document.getElementById('wifi_dns1').value,wifi_dns2:document.getElementById('wifi_dns2').value,bluetooth_enabled:document.getElementById('bluetooth_enabled').checked?'1':'0',bluetooth_name:document.getElementById('bluetooth_name').value,recalbox_ip:document.getElementById('recalbox_ip').value});}
function saveConfig(e){if(e&&e.preventDefault)e.preventDefault();showMsg(tr('msg_saving'),true);return fetch('/save',{method:'POST',body:serialize(),headers:{'Content-Type':'application/x-www-form-urlencoded'}}).then(r=>r.text()).then(t=>{showMsg(t.includes('OK')?tr('msg_saving'):t,t.includes('OK'));if(t.includes('OK'))_formDirty=false;}).catch(()=>showMsg(tr('msg_net_error'),false));}
function doReboot(skipConfirm){if(_formDirty&&!confirm(tr('msg_confirm_unsaved')))return;if(!skipConfirm&&!confirm(tr('msg_confirm_reboot')))return;showMsg(tr('msg_rebooting'),true);fetch('/reboot').catch(()=>{});}
// skipConfirm=true (2026-07-29) : "Enreg. & Redemarrer" a deja un intitule
// explicite -- redemander confirmation juste apres la sauvegarde est
// redondant, contrairement au bouton "Redemarrer" seul.
function saveAndReboot(){saveConfig().then(()=>setTimeout(()=>doReboot(true),400));}
function dmdResume(){if(_formDirty&&!confirm(tr('msg_confirm_unsaved')))return;fetch('/dmd-resume',{method:'POST'}).then(()=>showMsgLocal(tr('msg_dmd_resumed'),true)).catch(()=>showMsg(tr('msg_net_error'),false));}
// Reprise auto a la fermeture -- ESSAYEE puis RETIREE (2026-07-29) : aucun
// moyen fiable de distinguer une vraie fermeture d'onglet/navigateur d'un
// simple rafraichissement de page (habitude trop ancree pour l'utilisateur,
// faux positifs trop frequents -- ni le JS ni le serveur ne peuvent
// distinguer les deux cas, une connexion qui se ferme se ressemble dans
// tous les cas).
function scanWiFi(){
  const sel=document.getElementById('wifi_ssid');
  fetch('/scan-wifi').then(r=>r.json()).then(nets=>{
    sel.innerHTML='';
    const opt=document.createElement('option');opt.value='';opt.textContent=tr('opt_select');sel.appendChild(opt);
    let found=false;
    nets.forEach(n=>{const o=document.createElement('option');o.value=n;o.textContent=n;if(n===savedSsid){o.selected=true;found=true;}sel.appendChild(o);});
    if(savedSsid&&!found){const o=new Option(savedSsid,savedSsid,true,true);sel.add(o);}
  }).catch(()=>{sel.innerHTML='';const opt=document.createElement('option');opt.value=savedSsid;opt.textContent=savedSsid||tr('opt_scan_error');sel.appendChild(opt);});
}
function loadConfig(){fetch('/load').then(r=>r.json()).then(d=>{document.getElementById('wifi_enabled').checked=d.wifi_enabled==='1';savedSsid=d.wifi_ssid||'';document.getElementById('wifi_password').value=d.wifi_password||'';document.getElementById('wifi_static_enabled').checked=d.wifi_static_enabled==='1';document.getElementById('wifi_static_ip').value=d.wifi_static_ip||'';document.getElementById('wifi_gateway').value=d.wifi_gateway||'';document.getElementById('wifi_subnet').value=d.wifi_subnet||'';document.getElementById('wifi_dns1').value=d.wifi_dns1||'';document.getElementById('wifi_dns2').value=d.wifi_dns2||'';document.getElementById('bluetooth_enabled').checked=d.bluetooth_enabled==='1';document.getElementById('bluetooth_name').value=d.bluetooth_name||'';document.getElementById('recalbox_ip').value=d.recalbox_ip||'';scanWiFi();}).catch(()=>showMsg(tr('msg_load_error'),false));}
localStorage.setItem('dmd_last_section','network');
fetch('/lang').then(r=>r.json()).then(d=>{applyLang(d.language);loadConfig();}).catch(()=>{applyLang();loadConfig();});
document.getElementById('networkForm').addEventListener('input',()=>{_formDirty=true;});
</script>
</body>
</html>
)rawliteral";

static const char WEB_CONFIG_CLOCK_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="fr">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>RecalBox DMD - Horloge</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:'Segoe UI',Tahoma,sans-serif;background:#1a1a2e;color:#eee;padding:16px;max-width:700px;margin:auto}
h1{color:#ffd146;text-align:center;margin:8px 0 14px;font-size:22px;border-bottom:2px solid #ffd146;padding-bottom:8px}
.topnav{display:flex;gap:6px;flex-wrap:wrap;justify-content:center;margin-bottom:14px}
.topnav a{padding:8px 14px;border-radius:6px;background:#16213e;color:#8ab4f8;font-size:13px;font-weight:600;text-decoration:none}
.topnav a.active{background:#8ab4f8;color:#1a1a2e}
.section{background:#16213e;border-radius:8px;padding:16px;margin:12px 0}
.row{display:flex;flex-wrap:wrap;align-items:center;margin:10px 0}
.row label{flex:0 0 150px;font-size:14px;color:#aaa}
.row input,.row select{flex:1;min-width:120px;padding:8px 10px;border:1px solid #333;border-radius:4px;background:#0f3460;color:#eee;font-size:14px}
.row input[type=checkbox]{flex:0 0 20px;width:20px;height:20px;margin:0 8px 0 0}
.row input[type=color]{flex:0 0 60px;padding:2px}
.hint{flex:0 0 100%;font-size:11px;color:#666;margin-top:2px;margin-left:150px}
.btn-row{display:flex;gap:10px;justify-content:center;margin:18px 0;flex-wrap:wrap}
.btn{padding:12px 20px;border:none;border-radius:6px;font-size:13px;font-weight:bold;cursor:pointer}
.btn-save{background:#ffd146;color:#1a1a2e}
.btn-reboot{background:#e63946;color:#fff}
.btn-resume{background:#2d6a4f;color:#fff}
.btn-del{background:#555;color:#fff}
.msg{position:fixed;top:20px;left:50%;transform:translateX(-50%);z-index:999;padding:12px 20px;border-radius:8px;display:none;font-weight:bold;text-align:center;font-size:14px;box-shadow:0 4px 16px rgba(0,0,0,.6)}
.ok{background:#2d6a4f;color:#d8f3dc}
.err{background:#6b0f0f;color:#ffcccc}
#langSelect{position:absolute;top:10px;right:10px;width:auto;padding:6px 8px;font-size:13px;background:#16213e;color:#8ab4f8;border:1px solid #333;border-radius:4px}
body{position:relative}
</style>
</head>
<body>
<select id="langSelect" onchange="setLang(this.value)"><option value="fr">FR</option><option value="en">EN</option><option value="es">ES</option></select>
<div class="topnav">
<a href="/config/basic" data-i18n="nav_basic">&#x1F4A1; Affichage &amp; Playlists</a>
<a href="/config/network" data-i18n="nav_network">&#x1F4F6; Wi-Fi &amp; BT</a>
<a href="/config/clock" class="active" data-i18n="nav_clock">&#x23F0; Horloge</a>
<a href="/config/media" data-i18n="nav_media">&#x1F4BF; M&eacute;dias</a>
</div>
<h1 data-i18n="h1">Horloge</h1>
<form id="clockForm" onsubmit="saveConfig(event)">
<div class="section">
<div class="row"><label data-i18n="lbl_enabled">Activ&eacute;e</label><input id="clock_enabled" type="checkbox"></div>
<div class="row"><label for="clock_theme" data-i18n="lbl_theme">Th&egrave;me</label>
<select id="clock_theme">
<option value="-1" data-i18n="opt_random">Al&eacute;atoire</option>
<option value="0" data-i18n="opt_mario">Mario</option><option value="1" data-i18n="opt_tetris">Tetris</option>
<option value="2" data-i18n="opt_pacman">Pac-Man</option><option value="3" data-i18n="opt_spaceinv">Space Invaders</option>
<option value="4" data-i18n="opt_pong">Pong</option><option value="5" data-i18n="opt_neon">Neon</option>
<option value="6" data-i18n="opt_matrix">Matrix</option><option value="7" data-i18n="opt_fire">Fire</option>
<option value="8" data-i18n="opt_rainbow">Rainbow</option><option value="9" data-i18n="opt_level11">Level 1-1</option>
</select>
</div>
<div class="row"><label for="clock_neon_color" data-i18n="lbl_neon_color">Couleur Neon</label><input id="clock_neon_color" type="color" value="#ff2878">
<label style="flex:0 0 auto;font-size:13px;display:inline-flex;align-items:center;gap:4px;margin-left:10px"><input id="clock_neon_color_enabled" type="checkbox" style="flex:0 0 16px;width:16px;height:16px"> <span data-i18n="lbl_custom">Personnalis&eacute;e</span></label>
<div class="hint" data-i18n="hint_neon">Th&egrave;me Neon uniquement</div>
</div>
<div class="row"><label for="clock_interval" data-i18n="lbl_interval_gifs">Intervalle (GIFs)</label><input id="clock_interval" type="number" min="1" max="999"></div>
<div class="row"><label for="clock_interval_min" data-i18n="lbl_interval_min">Intervalle (min)</label><input id="clock_interval_min" type="number" min="0" max="999"><div class="hint" data-i18n="hint_interval_min">0 = d&eacute;sactiv&eacute;</div></div>
<div class="row"><label for="clock_duration" data-i18n="lbl_duration">Dur&eacute;e (sec)</label><input id="clock_duration" type="number" min="1" max="120"></div>
<div class="row"><label for="clock_tz" data-i18n="lbl_tz">Fuseau horaire</label>
<select id="clock_tz">
<option value="CET-1CEST,M3.5.0,M10.5.0/3" data-i18n="opt_tz_ce">France / Espagne / Allemagne / Italie</option>
<option value="GMT0BST,M3.5.0/1,M10.5.0" data-i18n="opt_tz_uk">Angleterre (UK) / Portugal</option>
<option value="EST5EDT,M3.2.0,M11.1.0" data-i18n="opt_tz_usa_e">USA - Est (New York)</option>
<option value="CST6CDT,M3.2.0,M11.1.0" data-i18n="opt_tz_usa_c">USA - Centre (Chicago)</option>
<option value="MST7MDT,M3.2.0,M11.1.0" data-i18n="opt_tz_usa_m">USA - Montagnes (Denver)</option>
<option value="PST8PDT,M3.2.0,M11.1.0" data-i18n="opt_tz_usa_p">USA - Pacifique (Los Angeles)</option>
<option value="EET-2EEST,M3.5.0/3,M10.5.0/4" data-i18n="opt_tz_ee">Gr&egrave;ce / Roumanie / Finlande</option>
<option value="UTC5">UTC-5</option><option value="UTC4">UTC-4</option><option value="UTC3">UTC-3</option>
<option value="UTC2">UTC-2</option><option value="UTC1">UTC-1</option><option value="UTC0">UTC+0</option>
<option value="UTC-1">UTC+1</option><option value="UTC-2">UTC+2</option><option value="UTC-3">UTC+3</option>
<option value="UTC-4">UTC+4</option><option value="UTC-5">UTC+5</option>
</select>
</div>
</div>
<div class="btn-row">
<button type="submit" class="btn btn-save" data-i18n="btn_save">&#x1F4BE; Enregistrer</button>
<button type="button" class="btn btn-reboot" onclick="saveAndReboot()" data-i18n="btn_save_reboot">&#x1F504; Enreg. &amp; Red&eacute;marrer</button>
<button type="button" class="btn btn-del" onclick="doReboot()" data-i18n="btn_reboot">&#x1F504; Red&eacute;marrer</button>
<button type="button" class="btn btn-resume" onclick="dmdResume()" data-i18n="btn_resume">&#x25B6; Reprendre DMD</button>
</div>
</form>
<div id="msg" class="msg"></div>
<script>
const PAGE_I18N={
fr:{title:'RecalBox DMD - Horloge',h1:'Horloge',nav_basic:'&#x1F4A1; Affichage &amp; Playlists',nav_network:'&#x1F4F6; Wi-Fi &amp; BT',nav_clock:'&#x23F0; Horloge',nav_media:'&#x1F4BF; Médias',lbl_enabled:'Activée',lbl_theme:'Thème',lbl_neon_color:'Couleur Neon',lbl_custom:'Personnalisée',hint_neon:'Thème Neon uniquement',lbl_interval_gifs:'Intervalle (GIFs)',lbl_interval_min:'Intervalle (min)',hint_interval_min:'0 = désactivé',lbl_duration:'Durée (sec)',lbl_tz:'Fuseau horaire',opt_random:'Aléatoire',opt_mario:'Mario',opt_tetris:'Tetris',opt_pacman:'Pac-Man',opt_spaceinv:'Space Invaders',opt_pong:'Pong',opt_neon:'Neon',opt_matrix:'Matrix',opt_fire:'Fire',opt_rainbow:'Rainbow',opt_level11:'Level 1-1',opt_tz_ce:'France / Espagne / Allemagne / Italie',opt_tz_uk:'Angleterre (UK) / Portugal',opt_tz_usa_e:'USA - Est (New York)',opt_tz_usa_c:'USA - Centre (Chicago)',opt_tz_usa_m:'USA - Montagnes (Denver)',opt_tz_usa_p:'USA - Pacifique (Los Angeles)',opt_tz_ee:'Grèce / Roumanie / Finlande',btn_save:'&#x1F4BE; Enregistrer',btn_save_reboot:'&#x1F504; Enreg. &amp; Redémarrer',btn_reboot:'&#x1F504; Redémarrer',btn_resume:'&#x25B6; Reprendre DMD',msg_saving:'Enregistrement...',msg_net_error:'Erreur réseau',msg_confirm_unsaved:'Des modifications non enregistrées seront perdues. Continuer ?',msg_confirm_reboot:'Redémarrer l\'ESP32 ?',msg_rebooting:'Redémarrage...',msg_dmd_resumed:'DMD repris',msg_load_error:'Impossible de charger la config'},
en:{title:'RecalBox DMD - Clock',h1:'Clock',nav_basic:'&#x1F4A1; Display &amp; Playlists',nav_network:'&#x1F4F6; Wi-Fi &amp; BT',nav_clock:'&#x23F0; Clock',nav_media:'&#x1F4BF; Media',lbl_enabled:'Enabled',lbl_theme:'Theme',lbl_neon_color:'Neon color',lbl_custom:'Custom',hint_neon:'Neon theme only',lbl_interval_gifs:'Interval (GIFs)',lbl_interval_min:'Interval (min)',hint_interval_min:'0 = disabled',lbl_duration:'Duration (sec)',lbl_tz:'Timezone',opt_random:'Random',opt_mario:'Mario',opt_tetris:'Tetris',opt_pacman:'Pac-Man',opt_spaceinv:'Space Invaders',opt_pong:'Pong',opt_neon:'Neon',opt_matrix:'Matrix',opt_fire:'Fire',opt_rainbow:'Rainbow',opt_level11:'Level 1-1',opt_tz_ce:'France / Spain / Germany / Italy',opt_tz_uk:'England (UK) / Portugal',opt_tz_usa_e:'USA - East (New York)',opt_tz_usa_c:'USA - Central (Chicago)',opt_tz_usa_m:'USA - Mountain (Denver)',opt_tz_usa_p:'USA - Pacific (Los Angeles)',opt_tz_ee:'Greece / Romania / Finland',btn_save:'&#x1F4BE; Save',btn_save_reboot:'&#x1F504; Save &amp; Reboot',btn_reboot:'&#x1F504; Reboot',btn_resume:'&#x25B6; Resume DMD',msg_saving:'Saving...',msg_net_error:'Network error',msg_confirm_unsaved:'Unsaved changes will be lost. Continue?',msg_confirm_reboot:'Reboot the ESP32?',msg_rebooting:'Rebooting...',msg_dmd_resumed:'DMD resumed',msg_load_error:'Unable to load config'},
es:{title:'RecalBox DMD - Reloj',h1:'Reloj',nav_basic:'&#x1F4A1; Pantalla y listas',nav_network:'&#x1F4F6; Wi-Fi y BT',nav_clock:'&#x23F0; Reloj',nav_media:'&#x1F4BF; Medios',lbl_enabled:'Activado',lbl_theme:'Tema',lbl_neon_color:'Color Neon',lbl_custom:'Personalizado',hint_neon:'Solo tema Neon',lbl_interval_gifs:'Intervalo (GIFs)',lbl_interval_min:'Intervalo (min)',hint_interval_min:'0 = desactivado',lbl_duration:'Duración (seg)',lbl_tz:'Zona horaria',opt_random:'Aleatorio',opt_mario:'Mario',opt_tetris:'Tetris',opt_pacman:'Pac-Man',opt_spaceinv:'Space Invaders',opt_pong:'Pong',opt_neon:'Neon',opt_matrix:'Matrix',opt_fire:'Fire',opt_rainbow:'Rainbow',opt_level11:'Level 1-1',opt_tz_ce:'Francia / España / Alemania / Italia',opt_tz_uk:'Inglaterra (UK) / Portugal',opt_tz_usa_e:'EE.UU. - Este (Nueva York)',opt_tz_usa_c:'EE.UU. - Centro (Chicago)',opt_tz_usa_m:'EE.UU. - Montañas (Denver)',opt_tz_usa_p:'EE.UU. - Pacífico (Los Ángeles)',opt_tz_ee:'Grecia / Rumanía / Finlandia',btn_save:'&#x1F4BE; Guardar',btn_save_reboot:'&#x1F504; Guardar y reiniciar',btn_reboot:'&#x1F504; Reiniciar',btn_resume:'&#x25B6; Reanudar DMD',msg_saving:'Guardando...',msg_net_error:'Error de red',msg_confirm_unsaved:'Los cambios no guardados se perderán. ¿Continuar?',msg_confirm_reboot:'¿Reiniciar el ESP32?',msg_rebooting:'Reiniciando...',msg_dmd_resumed:'DMD reanudado',msg_load_error:'No se pudo cargar la configuración'}
};
let currentLang='fr';
function tr(k){return (PAGE_I18N[currentLang]&&PAGE_I18N[currentLang][k])||PAGE_I18N.fr[k]||k;}
function applyLang(backendLang){
  const stored=localStorage.getItem('dmd_lang');
  if(stored&&PAGE_I18N[stored]){currentLang=stored;}
  else if(backendLang&&PAGE_I18N[backendLang]){currentLang=backendLang;}
  else{const nav=(navigator.language||'').substring(0,2);currentLang=PAGE_I18N[nav]?nav:'fr';}
  document.documentElement.lang=currentLang;
  document.title=tr('title');
  const savedTheme=document.getElementById('clock_theme').value;
  const savedTz=document.getElementById('clock_tz').value;
  document.querySelectorAll('[data-i18n]').forEach(function(el){el.innerHTML=tr(el.dataset.i18n);});
  document.getElementById('clock_theme').value=savedTheme;
  document.getElementById('clock_tz').value=savedTz;
  document.getElementById('langSelect').value=currentLang;
}
function setLang(code){
  localStorage.setItem('dmd_lang',code);
  applyLang();
  fetch('/save-language',{method:'POST',body:'language='+code,headers:{'Content-Type':'application/x-www-form-urlencoded'}}).catch(function(){});
}
let _formDirty=false;
function stripAccents(s){return s.normalize('NFD').replace(new RegExp('['+String.fromCharCode(768)+'-'+String.fromCharCode(879)+']','g'),'').replace(/[^ -~]/g,'?');}
function showMsg(txt,ok){const el=document.getElementById('msg');el.textContent=txt;el.className='msg '+(ok?'ok':'err');el.style.display='block';if(window._msgTimer)clearTimeout(window._msgTimer);window._msgTimer=setTimeout(()=>{el.style.display='none';},5000);fetch('/dmd-pause',{method:'POST',body:new URLSearchParams({msg:stripAccents(txt),color:ok?'1':'2'}),headers:{'Content-Type':'application/x-www-form-urlencoded'}}).catch(()=>{});}
function showMsgLocal(txt,ok){const el=document.getElementById('msg');el.textContent=txt;el.className='msg '+(ok?'ok':'err');el.style.display='block';if(window._msgTimer)clearTimeout(window._msgTimer);window._msgTimer=setTimeout(()=>{el.style.display='none';},5000);}
function serialize(){return new URLSearchParams({clock_enabled:document.getElementById('clock_enabled').checked?'1':'0',clock_theme:document.getElementById('clock_theme').value,clock_interval:document.getElementById('clock_interval').value,clock_interval_min:document.getElementById('clock_interval_min').value,clock_duration:document.getElementById('clock_duration').value,clock_tz:document.getElementById('clock_tz').value,clock_neon_color:document.getElementById('clock_neon_color').value,clock_neon_color_enabled:document.getElementById('clock_neon_color_enabled').checked?'1':'0'});}
function saveConfig(e){if(e&&e.preventDefault)e.preventDefault();showMsg(tr('msg_saving'),true);return fetch('/save',{method:'POST',body:serialize(),headers:{'Content-Type':'application/x-www-form-urlencoded'}}).then(r=>r.text()).then(t=>{showMsg(t.includes('OK')?tr('msg_saving'):t,t.includes('OK'));if(t.includes('OK'))_formDirty=false;}).catch(()=>showMsg(tr('msg_net_error'),false));}
function doReboot(skipConfirm){if(_formDirty&&!confirm(tr('msg_confirm_unsaved')))return;if(!skipConfirm&&!confirm(tr('msg_confirm_reboot')))return;showMsg(tr('msg_rebooting'),true);fetch('/reboot').catch(()=>{});}
// skipConfirm=true (2026-07-29) : "Enreg. & Redemarrer" a deja un intitule
// explicite -- redemander confirmation juste apres la sauvegarde est
// redondant, contrairement au bouton "Redemarrer" seul.
function saveAndReboot(){saveConfig().then(()=>setTimeout(()=>doReboot(true),400));}
function dmdResume(){if(_formDirty&&!confirm(tr('msg_confirm_unsaved')))return;fetch('/dmd-resume',{method:'POST'}).then(()=>showMsgLocal(tr('msg_dmd_resumed'),true)).catch(()=>showMsg(tr('msg_net_error'),false));}
// Reprise auto a la fermeture -- ESSAYEE puis RETIREE (2026-07-29) : aucun
// moyen fiable de distinguer une vraie fermeture d'onglet/navigateur d'un
// simple rafraichissement de page (habitude trop ancree pour l'utilisateur,
// faux positifs trop frequents -- ni le JS ni le serveur ne peuvent
// distinguer les deux cas, une connexion qui se ferme se ressemble dans
// tous les cas).
function loadConfig(){fetch('/load').then(r=>r.json()).then(d=>{document.getElementById('clock_enabled').checked=d.clock_enabled==='1';document.getElementById('clock_theme').value=d.clock_theme||'0';document.getElementById('clock_interval').value=d.clock_interval||'0';document.getElementById('clock_interval_min').value=d.clock_interval_min||'0';document.getElementById('clock_duration').value=d.clock_duration||'0';document.getElementById('clock_tz').value=d.clock_tz||'UTC0';document.getElementById('clock_neon_color').value=d.clock_neon_color||'#ff2878';document.getElementById('clock_neon_color_enabled').checked=d.clock_neon_color_enabled==='1';}).catch(()=>showMsg(tr('msg_load_error'),false));}
localStorage.setItem('dmd_last_section','clock');
fetch('/lang').then(r=>r.json()).then(d=>{applyLang(d.language);loadConfig();}).catch(()=>{applyLang();loadConfig();});
document.getElementById('clockForm').addEventListener('input',()=>{_formDirty=true;});
</script>
</body>
</html>
)rawliteral";

static const char WEB_CONFIG_MEDIA_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="fr">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>RecalBox DMD - Médias</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:'Segoe UI',Tahoma,sans-serif;background:#1a1a2e;color:#eee;padding:16px;max-width:700px;margin:auto}
h1{color:#ffd146;text-align:center;margin:8px 0 14px;font-size:22px;border-bottom:2px solid #ffd146;padding-bottom:8px}
.topnav{display:flex;gap:6px;flex-wrap:wrap;justify-content:center;margin-bottom:14px}
.topnav a{padding:8px 14px;border-radius:6px;background:#16213e;color:#8ab4f8;font-size:13px;font-weight:600;text-decoration:none}
.topnav a.active{background:#8ab4f8;color:#1a1a2e}
.section{background:#16213e;border-radius:8px;padding:16px;margin:12px 0}
h2{color:#8ab4f8;font-size:15px;margin:0 0 10px;border-left:3px solid #8ab4f8;padding-left:8px}
.row{display:flex;flex-wrap:wrap;align-items:center;margin:10px 0}
.row label{flex:0 0 150px;font-size:14px;color:#aaa}
.row input,.row select{flex:1;min-width:120px;padding:8px 10px;border:1px solid #333;border-radius:4px;background:#0f3460;color:#eee;font-size:14px}
.desc{font-size:12px;color:#aaa;margin-bottom:8px}
.dirs{margin:8px 0;max-height:220px;overflow-y:auto}
.dirs label{display:flex;align-items:center;gap:8px;font-size:14px;padding:3px 0}
.dirs label span.name{flex:1}
.mini-row{display:flex;gap:8px;margin-bottom:8px}
.mini-btn{padding:4px 10px;border:none;border-radius:4px;background:#1a6b9e;color:#fff;font-size:11px;cursor:pointer}
.upload-row{display:flex;gap:6px;flex-wrap:wrap;margin:10px 0}
.upload-row select,.upload-row input{flex:1;min-width:110px;padding:8px 10px;border:1px solid #333;border-radius:4px;background:#0f3460;color:#eee;font-size:14px}
.btn-row{display:flex;gap:10px;justify-content:center;margin:14px 0;flex-wrap:wrap}
.btn{padding:10px 18px;border:none;border-radius:6px;font-size:13px;font-weight:bold;cursor:pointer}
.btn-upload{background:#1a6b9e;color:#fff}
.btn-del{background:#555;color:#fff}
.btn-reboot{background:#e63946;color:#fff}
.btn-resume{background:#0f766e;color:#fff}
.btn-stop{background:#c0392b;color:#fff}
.progress{height:4px;background:#333;border-radius:2px;margin:8px 0;display:none}
.progress-bar{height:4px;background:#52b788;border-radius:2px;width:0%}
.msg{position:fixed;top:20px;left:50%;transform:translateX(-50%);z-index:999;padding:12px 20px;border-radius:8px;display:none;font-weight:bold;text-align:center;font-size:14px;box-shadow:0 4px 16px rgba(0,0,0,.6)}
.ok{background:#2d6a4f;color:#d8f3dc}
.err{background:#6b0f0f;color:#ffcccc}
#langSelect{position:absolute;top:10px;right:10px;width:auto;padding:6px 8px;font-size:13px;background:#16213e;color:#8ab4f8;border:1px solid #333;border-radius:4px}
body{position:relative}
</style>
</head>
<body>
<select id="langSelect" onchange="setLang(this.value)"><option value="fr">FR</option><option value="en">EN</option><option value="es">ES</option></select>
<div class="topnav">
<a href="/config/basic" data-i18n="nav_basic">&#x1F4A1; Affichage &amp; Playlists</a>
<a href="/config/network" data-i18n="nav_network">&#x1F4F6; Wi-Fi &amp; BT</a>
<a href="/config/clock" data-i18n="nav_clock">&#x23F0; Horloge</a>
<a href="/config/media" class="active" data-i18n="nav_media">&#x1F4BF; M&eacute;dias</a>
</div>
<h1 data-i18n="h1">M&eacute;dias</h1>
<div class="section">
<h2 data-i18n="sec_dirs">&#x1F4C1; Dossiers (/gifs/)</h2>
<div class="desc" data-i18n="desc_dirs">Cochez des dossiers pour les supprimer.</div>
<div class="mini-row">
<button type="button" class="mini-btn" onclick="selectAllDirs(true)" data-i18n="btn_select_all">Tout s&eacute;lectionner</button>
<button type="button" class="mini-btn" onclick="selectAllDirs(false)" data-i18n="btn_select_none">Rien s&eacute;lectionner</button>
</div>
<div id="dirList" class="dirs"></div>
<div class="btn-row">
<button type="button" class="btn btn-del" onclick="deleteSelected()" data-i18n="btn_delete_sel">&#x1F5D1; Supprimer la s&eacute;lection</button>
</div>
</div>
<div class="section">
<h2 data-i18n="sec_upload">&#x1F4E4; Envoi GIF</h2>
<div class="desc" data-i18n="desc_upload">Ajoutez un fichier .gif directement depuis votre navigateur dans un dossier de /gifs/. Choisissez un dossier existant OU tapez un nouveau nom (cr&eacute;&eacute; automatiquement). &#x26A0;&#xFE0F; Pas fait pour transferer de nombreux fichiers (debit lent, risque d'erreur d'ecriture) -- reserve a l'ajout ponctuel de quelques fichiers. Pour un transfert consequent, retirez la carte SD et copiez-la depuis un PC.</div>
<div class="upload-row">
<select id="uploadDir"></select>
<input id="uploadDirCustom" data-i18n-placeholder="placeholder_upload_dir" placeholder="ou nouveau dossier...">
</div>
<div class="row"><label for="uploadFile" data-i18n="lbl_upload_file">Fichiers .gif</label><input id="uploadFile" type="file" accept=".gif" multiple></div>
<div class="progress" id="uploadProgress"><div class="progress-bar" id="uploadProgressBar"></div></div>
<div id="uploadFileList" style="margin:4px 0;font-size:12px;color:#aaa"></div>
<div class="btn-row">
<button type="button" class="btn btn-upload" onclick="uploadGif()" data-i18n="btn_upload">&#x1F4E4; Uploader</button>
<button type="button" class="btn btn-stop" id="uploadStopBtn" onclick="stopUpload()" style="display:none" data-i18n="btn_stop">&#x23F9; Arr&ecirc;ter</button>
</div>
</div>
<div class="btn-row">
<button type="button" class="btn btn-reboot" onclick="doReboot()" data-i18n="btn_reboot">&#x1F504; Red&eacute;marrer</button>
<button type="button" class="btn btn-resume" onclick="dmdResume()" data-i18n="btn_resume">&#x25B6; Reprendre DMD</button>
</div>
<div id="msg" class="msg"></div>
<script>
const PAGE_I18N={
fr:{title:'RecalBox DMD - Médias',h1:'Médias',nav_basic:'&#x1F4A1; Affichage &amp; Playlists',nav_network:'&#x1F4F6; Wi-Fi &amp; BT',nav_clock:'&#x23F0; Horloge',nav_media:'&#x1F4BF; Médias',
sec_dirs:'&#x1F4C1; Dossiers (/gifs/)',desc_dirs:'Cochez des dossiers pour les supprimer.',btn_select_all:'Tout sélectionner',btn_select_none:'Rien sélectionner',btn_delete_sel:'&#x1F5D1; Supprimer la sélection',
sec_upload:'&#x1F4E4; Envoi GIF',desc_upload:'Ajoutez un fichier .gif directement depuis votre navigateur dans un dossier de /gifs/. Choisissez un dossier existant OU tapez un nouveau nom (créé automatiquement). &#x26A0;&#xFE0F; Pas fait pour transférer de nombreux fichiers (débit lent, risque d\'erreur d\'écriture) -- réservé à l\'ajout ponctuel de quelques fichiers. Pour un transfert consequent, retirez la carte SD et copiez-la depuis un PC.',placeholder_upload_dir:'ou nouveau dossier...',lbl_upload_file:'Fichiers .gif',btn_upload:'&#x1F4E4; Uploader',btn_stop:'&#x23F9; Arrêter',
btn_reboot:'&#x1F504; Redémarrer',btn_resume:'&#x25B6; Reprendre DMD',
net_error:'Erreur réseau',msg_deleting:'Suppression...',msg_select_folder:'Choisissez au moins un dossier',msg_confirm_delete_folders:'Supprimer ${0} ?',msg_specify_dir:'Précisez un dossier cible',msg_select_gif:'Choisissez un fichier GIF',msg_select_gif_files:'Choisissez des fichiers .gif',msg_preparing_folder:'Preparation du dossier...',msg_cannot_create_folder:'Impossible de creer le dossier: ${0}',msg_net_error_folder:'Erreur reseau (creation dossier)',msg_uploading:'Upload...',msg_attempt:'tentative ${0}/${1}',msg_stopped_by_user:'Arrete par l\'utilisateur (${0}/${1})',msg_upload_fail:'ECHEC',msg_failures:'Echecs: ${0}',msg_upload_result:'${0}/${1} fichier(s) uploade(s)',msg_upload_result_fail:' -- echecs: ${0}',msg_confirm_reboot:'Redemarrer l\'ESP32 ?',msg_rebooting:'Redemarrage...',msg_dmd_resumed:'DMD repris',msg_updating_playlists:'Mise a jour des playlists...',msg_confirm_reboot_playlists:'Dossiers supprimes, ${0} playlist(s) mise(s) a jour. La suppression d\'un dossier lie a des playlists necessite un redemarrage du DMD pour etre prise en compte. Redemarrer maintenant ?'},
en:{title:'RecalBox DMD - Media',h1:'Media',nav_basic:'&#x1F4A1; Display &amp; Playlists',nav_network:'&#x1F4F6; Wi-Fi &amp; BT',nav_clock:'&#x23F0; Clock',nav_media:'&#x1F4BF; Media',
sec_dirs:'&#x1F4C1; Folders (/gifs/)',desc_dirs:'Check folders to delete them.',btn_select_all:'Select all',btn_select_none:'Select none',btn_delete_sel:'&#x1F5D1; Delete selection',
sec_upload:'&#x1F4E4; GIF Upload',desc_upload:'Add a .gif file directly from your browser into a folder in /gifs/. Choose an existing folder OR type a new name (created automatically). &#x26A0;&#xFE0F; Not designed for transferring many files (slow throughput, risk of write errors) -- meant for occasionally adding a few files. For a large transfer, remove the SD card and copy from a PC instead.',placeholder_upload_dir:'or new folder...',lbl_upload_file:'.gif files',btn_upload:'&#x1F4E4; Upload',btn_stop:'&#x23F9; Stop',
btn_reboot:'&#x1F504; Reboot',btn_resume:'&#x25B6; Resume DMD',
net_error:'Network error',msg_deleting:'Deleting...',msg_select_folder:'Select at least one folder',msg_confirm_delete_folders:'Delete ${0}?',msg_specify_dir:'Please specify a target folder',msg_select_gif:'Select a GIF file',msg_select_gif_files:'Select .gif files',msg_preparing_folder:'Preparing folder...',msg_cannot_create_folder:'Unable to create folder: ${0}',msg_net_error_folder:'Network error (folder creation)',msg_uploading:'Uploading...',msg_attempt:'attempt ${0}/${1}',msg_stopped_by_user:'Stopped by user (${0}/${1})',msg_upload_fail:'FAILED',msg_failures:'Failures: ${0}',msg_upload_result:'${0}/${1} file(s) uploaded',msg_upload_result_fail:' -- failures: ${0}',msg_confirm_reboot:'Reboot the ESP32?',msg_rebooting:'Rebooting...',msg_dmd_resumed:'DMD resumed',msg_updating_playlists:'Updating playlists...',msg_confirm_reboot_playlists:'Folders deleted, ${0} playlist(s) updated. Deleting a folder linked to playlists requires a DMD reboot to take effect. Reboot now?'},
es:{title:'RecalBox DMD - Medios',h1:'Medios',nav_basic:'&#x1F4A1; Pantalla y listas',nav_network:'&#x1F4F6; Wi-Fi y BT',nav_clock:'&#x23F0; Reloj',nav_media:'&#x1F4BF; Medios',
sec_dirs:'&#x1F4C1; Carpetas (/gifs/)',desc_dirs:'Marque las carpetas para eliminarlas.',btn_select_all:'Seleccionar todo',btn_select_none:'Deseleccionar todo',btn_delete_sel:'&#x1F5D1; Eliminar selección',
sec_upload:'&#x1F4E4; Subir GIF',desc_upload:'Añada un archivo .gif desde su navegador a una carpeta en /gifs/. Elija una carpeta existente O escriba un nombre nuevo (se crea automáticamente). &#x26A0;&#xFE0F; No pensado para transferir muchos archivos (velocidad lenta, riesgo de error de escritura) -- reservado para añadir algunos archivos puntualmente. Para una transferencia importante, retire la tarjeta SD y cópiela desde un PC.',placeholder_upload_dir:'o nueva carpeta...',lbl_upload_file:'Archivos .gif',btn_upload:'&#x1F4E4; Subir',btn_stop:'&#x23F9; Detener',
btn_reboot:'&#x1F504; Reiniciar',btn_resume:'&#x25B6; Reanudar DMD',
net_error:'Error de red',msg_deleting:'Eliminando...',msg_select_folder:'Elija al menos una carpeta',msg_confirm_delete_folders:'¿Eliminar ${0}?',msg_specify_dir:'Especifique una carpeta destino',msg_select_gif:'Seleccione un archivo GIF',msg_select_gif_files:'Seleccione archivos .gif',msg_preparing_folder:'Preparando carpeta...',msg_cannot_create_folder:'No se pudo crear la carpeta: ${0}',msg_net_error_folder:'Error de red (creación de carpeta)',msg_uploading:'Subiendo...',msg_attempt:'intento ${0}/${1}',msg_stopped_by_user:'Detenido por el usuario (${0}/${1})',msg_upload_fail:'ERROR',msg_failures:'Errores: ${0}',msg_upload_result:'${0}/${1} archivo(s) subido(s)',msg_upload_result_fail:' -- errores: ${0}',msg_confirm_reboot:'¿Reiniciar el ESP32?',msg_rebooting:'Reiniciando...',msg_dmd_resumed:'DMD reanudado',msg_updating_playlists:'Actualizando listas...',msg_confirm_reboot_playlists:'Carpetas eliminadas, ${0} lista(s) de reproduccion actualizada(s). Eliminar una carpeta vinculada a listas requiere reiniciar el DMD para aplicarse. ¿Reiniciar ahora?'}
};
let currentLang='fr';
function tr(k){return (PAGE_I18N[currentLang]&&PAGE_I18N[currentLang][k])||PAGE_I18N.fr[k]||k;}
function trTpl(k){const args=[].slice.call(arguments,1);let s=tr(k);args.forEach((v,i)=>{s=s.split('${'+i+'}').join(v);});return s;}
function applyLang(backendLang){
  const stored=localStorage.getItem('dmd_lang');
  if(stored&&PAGE_I18N[stored]){currentLang=stored;}
  else if(backendLang&&PAGE_I18N[backendLang]){currentLang=backendLang;}
  else{const nav=(navigator.language||'').substring(0,2);currentLang=PAGE_I18N[nav]?nav:'fr';}
  document.documentElement.lang=currentLang;
  document.title=tr('title');
  document.querySelectorAll('[data-i18n]').forEach(function(el){el.innerHTML=tr(el.dataset.i18n);});
  document.querySelectorAll('[data-i18n-placeholder]').forEach(function(el){el.placeholder=tr(el.dataset.i18nPlaceholder);});
  document.getElementById('langSelect').value=currentLang;
}
function setLang(code){
  localStorage.setItem('dmd_lang',code);
  applyLang();
  fetch('/save-language',{method:'POST',body:'language='+code,headers:{'Content-Type':'application/x-www-form-urlencoded'}}).catch(function(){});
}
function stripAccents(s){return s.normalize('NFD').replace(new RegExp('['+String.fromCharCode(768)+'-'+String.fromCharCode(879)+']','g'),'').replace(/[^ -~]/g,'?');}
// File d'attente globale : l'ESP32 (WebServer mono-requete) ne traite
// qu'une connexion a la fois. Plusieurs fetch() partis en parallele (ex.
// les appels de chargement initial + un clic utilisateur pendant ce temps)
// se faisaient concurrence sur la meme connexion -- confirme en test reel
// comme net::ERR_INVALID_CHUNKED_ENCODING, meme sur des reponses courtes
// servies depuis le cache. TOUS les fetch() de cette page passent
// desormais par queuedFetch(), qui les serialise strictement.
let _reqQueue=Promise.resolve();
function queuedFetch(url,opts){
  const p=_reqQueue.then(()=>fetch(url,opts));
  _reqQueue=p.catch(()=>{});
  return p;
}
function showMsg(txt,ok){const el=document.getElementById('msg');el.textContent=txt;el.className='msg '+(ok?'ok':'err');el.style.display='block';if(window._msgTimer)clearTimeout(window._msgTimer);window._msgTimer=setTimeout(()=>{el.style.display='none';},5000);queuedFetch('/dmd-pause',{method:'POST',body:new URLSearchParams({msg:stripAccents(txt),color:ok?'1':'2'}),headers:{'Content-Type':'application/x-www-form-urlencoded'}}).catch(()=>{});}
function showMsgLocal(txt,ok){const el=document.getElementById('msg');el.textContent=txt;el.className='msg '+(ok?'ok':'err');el.style.display='block';if(window._msgTimer)clearTimeout(window._msgTimer);window._msgTimer=setTimeout(()=>{el.style.display='none';},5000);}
function doReboot(skipConfirm){if(!skipConfirm&&!confirm(tr('msg_confirm_reboot')))return;showMsg(tr('msg_rebooting'),true);queuedFetch('/reboot').catch(()=>{});}
function dmdResume(){queuedFetch('/dmd-resume',{method:'POST'}).then(()=>showMsgLocal(tr('msg_dmd_resumed'),true)).catch(()=>showMsg(tr('net_error'),false));}
// Reprise auto a la fermeture -- ESSAYEE puis RETIREE (2026-07-29, voir
// page Affichage pour le detail) : aucun moyen fiable de distinguer une
// vraie fermeture d'un simple rafraichissement de page.
function selectAllDirs(v){document.querySelectorAll('#dirList input').forEach(i=>i.checked=v);}
// v85 : plus de navigation dans un dossier (contenu individuel des GIF) ni
// de statut cached/excluded -- decision utilisateur de retirer cette
// fonctionnalite (voir changelog web_config.h). renderDirs() redevient une
// simple liste de dossiers a cocher (creation/suppression/upload
// uniquement).
function renderDirs(dirs){
  const list=document.getElementById('dirList');list.innerHTML='';
  const sel=document.getElementById('uploadDir');sel.innerHTML='';
  const opt=document.createElement('option');opt.value='';opt.textContent='---';sel.appendChild(opt);
  dirs.forEach(d=>{
    const name=(d&&typeof d==='object')?d.name:d;
    const row=document.createElement('label');
    row.innerHTML='<input type="checkbox" value="'+name+'"><span class="name">&#x1F4C1; '+name+'</span>';
    list.appendChild(row);
    const o=document.createElement('option');o.value=name;o.textContent=name;sel.appendChild(o);
  });
}
// loadDirs() et loadUploadDirs() appelaient chacun /lsgifdirs
// independamment (2 scans SD + 2 parsings JSON pour la MEME donnee a
// chaque chargement de page/rafraichissement) -- fusionnes en un seul
// fetch partage pour reduire la pression heap qui contribuait au crash
// abort() observe en test reel apres plusieurs operations consecutives.
// B (plan cache_master_gifs, retour test reel 2026-08-01) -- retry (5
// tentatives, 500ms d'ecart), meme raison et meme pattern que loadGenDirs()
// (page Affichage, deja corrige le 2026-07-29 pour EXACTEMENT ce probleme) :
// un simple fetch().catch(()=>{}) laissait la liste MEDIA vide en silence,
// sans retenter, des qu'une seule requete /lsgifdirs echouait au chargement
// de la page -- necessitait un F5 manuel pour reessayer. Cette fonction
// n'avait jamais recu le meme correctif que loadGenDirs() a l'epoque.
// Cache sessionStorage PARTAGE avec la page Affichage (meme cle,
// 'dmd_gifdirs_cache') -- demande utilisateur 2026-08-02 : le va-et-vient
// frequent entre Affichage et MEDIA redemandait /lsgifdirs a chaque fois,
// avec le risque d'echec reseau observe en test reel. Affiche IMMEDIATEMENT
// le contenu en cache si present (aucune attente reseau), PUIS rafraichit
// en arriere-plan et met a jour le cache -- la liste se corrige donc
// silencieusement si elle avait change entre-temps (creation/suppression de
// dossier), sans jamais bloquer l'affichage initial sur le reseau.
function readDirsCache(){
  try{
    const raw=sessionStorage.getItem('dmd_gifdirs_cache');
    return raw?JSON.parse(raw):null;
  }catch(e){return null;}
}
function writeDirsCache(dirs){
  try{sessionStorage.setItem('dmd_gifdirs_cache',JSON.stringify(dirs));}catch(e){}
}
async function loadDirs(){
  const cached=readDirsCache();
  if(cached&&cached.length)renderDirs(cached);
  for(let attempt=0;attempt<5;attempt++){
    if(attempt>0)await new Promise(r=>setTimeout(r,500));
    try{
      const dirs=await(await queuedFetch('/lsgifdirs')).json();
      if(!dirs.length&&attempt<4)continue;
      renderDirs(dirs);
      writeDirsCache(dirs);
      return;
    }catch(e){}
  }
}
function loadUploadDirs(){return Promise.resolve();} // conserve pour compatibilite des appels existants -- loadDirs() peuple desormais aussi #uploadDir
function deleteSelected(){
  const dirs=[].slice.call(document.querySelectorAll('#dirList input:checked')).map(i=>i.value);
  if(!dirs.length){showMsg(tr('msg_select_folder'),false);return;}
  if(!confirm(trTpl('msg_confirm_delete_folders',dirs.join(', '))))return;
  showMsg(tr('msg_deleting'),true);
  queuedFetch('/delete-folders',{method:'POST',body:new URLSearchParams({dirs:dirs.join(',')}),headers:{'Content-Type':'application/x-www-form-urlencoded'}})
    .then(r=>r.text()).then(t=>{
      // A.3 (plan cache_master_gifs) -- si la reponse indique qu'au moins une
      // playlist a ete mise a jour (lignes mortes retirees), un redemarrage
      // est necessaire pour que la session de lecture EN COURS (deja son
      // cache playlist .idx charge en RAM) soit corrigee -- mais laisse a
      // l'utilisateur le choix du moment (demande utilisateur 2026-08-02,
      // popup oui/non plutot qu'un redemarrage automatique impose). confirm()
      // est bloquant : empeche aussi toute autre action pendant que cette
      // decision est en attente (demande utilisateur : "info web... pour
      // eviter une action utilisateur inappropriee").
      const m=t.match(/(\d+) playlist/);
      if(m){
        if(confirm(trTpl('msg_confirm_reboot_playlists',m[1]))){
          doReboot(true);
        } else {
          showMsg(t,true);loadDirs();loadUploadDirs();
        }
      } else {
        showMsg(t,t.includes('OK'));loadDirs();loadUploadDirs();
      }
    })
    .catch(()=>showMsg(tr('net_error'),false));
}
async function uploadGif(){
  const sel=document.getElementById('uploadDir');
  const custom=document.getElementById('uploadDirCustom').value.trim();
  const dir=custom||sel.value;
  if(!dir){showMsg(tr('msg_specify_dir'),false);return;}
  const fileInput=document.getElementById('uploadFile');
  if(!fileInput.files.length){showMsg(tr('msg_select_gif'),false);return;}
  const files=Array.from(fileInput.files).filter(f=>f.name.toLowerCase().endsWith('.gif'));
  if(!files.length){showMsg(tr('msg_select_gif_files'),false);return;}
  _uploadStopRequested=false;
  const stopBtn=document.getElementById('uploadStopBtn');stopBtn.style.display='inline-block';
  const bar=document.getElementById('uploadProgress');bar.style.display='block';
  const barInner=document.getElementById('uploadProgressBar');
  const fileList=document.getElementById('uploadFileList');
  const msgEl=document.getElementById('msg');
  msgEl.className='msg ok';msgEl.style.display='block';msgEl.textContent=tr('msg_preparing_folder');
  try{
    const cr=await queuedFetch('/create-folder',{method:'POST',body:new URLSearchParams({dir:dir}),headers:{'Content-Type':'application/x-www-form-urlencoded'}});
    const ct=await cr.text();
    if(!ct.includes('OK')){stopBtn.style.display='none';showMsg(trTpl('msg_cannot_create_folder',ct),false);return;}
    // v92 -- ne rafraichir la liste des dossiers que si /create-folder en a
    // reellement cree un NOUVEAU ("OK: cree") : pour un dossier deja
    // existant ("OK: existant", cas le plus courant), la liste affichee
    // depuis le chargement de la page est deja a jour -- ce fetch /lsgifdirs
    // supplementaire n'apportait rien et ajoutait un cout heap par requete
    // mesure en test reel (diagnostic v90).
    if(ct.indexOf('cree')>=0){await loadDirs();await loadUploadDirs();}
  }catch(e){stopBtn.style.display='none';showMsg(tr('msg_net_error_folder'),false);return;}
  msgEl.textContent=tr('msg_uploading');
  let okCount=0;const failed=[];const uploaded=[];
  for(let i=0;i<files.length;i++){
    if(_uploadStopRequested){fileList.textContent=trTpl('msg_stopped_by_user',i,files.length);break;}
    const file=files[i];
    const pct=Math.round(((i+1)/files.length)*100);
    barInner.style.width=Math.max(pct,5)+'%';
    fileList.textContent=file.name+' ('+(i+1)+'/'+files.length+')';
    msgEl.textContent=tr('msg_uploading')+' '+file.name;
    try{await queuedFetch('/dmd-pause',{method:'POST',body:new URLSearchParams({msg:file.name+' ('+(i+1)+'/'+files.length+')',color:'1'}),headers:{'Content-Type':'application/x-www-form-urlencoded'}});}catch(e){}
    let ok=false,lastErr='';
    for(let attempt=0;attempt<3&&!ok;attempt++){
      if(attempt>0){
        const attemptTxt=file.name+' ('+(i+1)+'/'+files.length+') - '+trTpl('msg_attempt',attempt+1,3);
        fileList.textContent=attemptTxt;
        try{await queuedFetch('/dmd-pause',{method:'POST',body:new URLSearchParams({msg:attemptTxt,color:'1'}),headers:{'Content-Type':'application/x-www-form-urlencoded'}});}catch(e){}
        await new Promise(r=>setTimeout(r,500));
      }
      const form=new FormData();form.append('dir',dir);form.append('file',file);
      try{
        const r=await queuedFetch('/upload',{method:'POST',body:form});
        const t=await r.text();
        if(t.includes('OK')){ok=true;} else {lastErr=t;}
      }catch(e){lastErr=tr('net_error');}
    }
    if(ok){okCount++;uploaded.push(file.name);fileList.textContent=file.name+' OK ('+okCount+'/'+files.length+')';}
    else {failed.push(file.name);fileList.textContent=file.name+' '+tr('msg_upload_fail');}
  }
  stopBtn.style.display='none';
  if(uploaded.length){
    msgEl.textContent=tr('msg_updating_playlists');
    try{await queuedFetch('/dmd-pause',{method:'POST',body:new URLSearchParams({msg:stripAccents(tr('msg_updating_playlists')),color:'1'}),headers:{'Content-Type':'application/x-www-form-urlencoded'}});}catch(e){}
    try{await queuedFetch('/add-to-playlists-batch',{method:'POST',body:new URLSearchParams({dir:dir,files:uploaded.join(',')}),headers:{'Content-Type':'application/x-www-form-urlencoded'}});}catch(e){}
  }
  fileList.textContent=failed.length?trTpl('msg_failures',failed.join(', ')):'';
  document.getElementById('uploadDirCustom').value='';
  // v92 -- retire le rafraichissement /lsgifdirs de fin d'upload : uploader
  // des FICHIERS dans un dossier ne change jamais l'ENSEMBLE des noms de
  // dossiers affiches (#dirList/#uploadDir) -- un dossier nouvellement cree
  // est deja reflete par le rafraichissement conditionnel juste apres
  // /create-folder ci-dessus. Ce fetch etait systematiquement inutile et
  // ajoutait un cout heap par requete (diagnostic v90) -- c'est lui qui
  // echouait "heap critique, liste vide" apres plusieurs tentatives d'upload
  // ratees en test reel, faisant croire a une disparition de la liste.
  const result=trTpl('msg_upload_result',okCount,files.length)+(failed.length?trTpl('msg_upload_result_fail',failed.join(', ')):'');
  showMsg(result,failed.length===0);
}
function stopUpload(){_uploadStopRequested=true;}
let _uploadStopRequested=false;
localStorage.setItem('dmd_last_section','media');
// Sequence de chargement initial serialisee via queuedFetch() -- avant
// v45, /lang + loadDirs() + loadPlaylists() + loadUploadDirs() partaient
// TOUS en parallele au chargement de la page (4 requetes concurrentes sur
// un serveur qui n'en traite qu'une a la fois), confirme en test reel
// comme la cause de net::ERR_INVALID_CHUNKED_ENCODING des le premier clic
// sur un dossier si l'utilisateur cliquait pendant que ce lot initial
// etait encore en cours.
queuedFetch('/lang').then(r=>r.json()).then(d=>{applyLang(d.language);}).catch(()=>{applyLang();});
loadDirs();loadUploadDirs();
</script>
</body>
</html>
)rawliteral";

static const char WEB_CONFIG_AP_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="fr">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>RecalBox DMD</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:'Segoe UI',Tahoma,sans-serif;background:#1a1a2e;color:#eee;padding:16px;max-width:500px;margin:auto;display:flex;flex-direction:column;min-height:100vh;justify-content:center;position:relative}
h1{color:#ffd146;text-align:center;margin:16px 0;font-size:22px}
.section{background:#16213e;border-radius:8px;padding:24px;margin:12px 0}
.row{display:flex;flex-direction:column;margin:12px 0}
.row label{font-size:14px;color:#aaa;margin-bottom:4px}
.row input,.row select{padding:10px 12px;border:1px solid #555;border-radius:6px;background:#0f3460;color:#eee;font-size:16px}
.row input:focus,.row select:focus{outline:2px solid #ffd146}
.row select{width:100%}
.pwd-row{display:flex;gap:8px;align-items:center}
.pwd-row input{flex:1}
.pwd-toggle{background:none;border:none;color:#888;font-size:20px;cursor:pointer;padding:4px 8px}
.pwd-toggle:hover{color:#ffd146}
.btn-row{text-align:center;margin:20px 0}
.btn{padding:14px 40px;border:none;border-radius:6px;font-size:18px;font-weight:bold;cursor:pointer;background:#ffd146;color:#1a1a2e;width:100%}
.btn:hover{background:#ffe070}
.btn-scan{background:#1a6b9e;color:#fff;font-size:14px;padding:8px 16px;border:none;border-radius:4px;cursor:pointer;margin-top:4px;width:100%}
.btn-scan:hover{background:#2880b8}
.hint{font-size:13px;color:#888;text-align:center;margin:8px 0 4px;line-height:1.5}
.msg{position:fixed;top:20px;left:50%;transform:translateX(-50%);z-index:999;padding:14px 24px;border-radius:8px;display:none;font-weight:bold;text-align:center;font-size:16px;box-shadow:0 4px 16px rgba(0,0,0,.6);max-width:90%}
.msg-ok{background:#2d6a4f;color:#d8f3dc;border:2px solid #52b788}
.msg-err{background:#6b0f0f;color:#ffcccc;border:2px solid #e63946}
#langSelect{position:absolute;top:10px;right:10px;width:auto;padding:6px 8px;font-size:13px}
</style>
</head>
<body>
<select id="langSelect" onchange="setLang(this.value)"><option value="fr">FR</option><option value="en">EN</option><option value="es">ES</option></select>
<h1 data-i18n="h1">&#x1F4E1; Configuration WiFi</h1>
<div class="hint" data-i18n="hint">Connectez-vous au r&eacute;seau <b>RecalBox-DMD-Config</b> puis s&eacute;lectionnez votre WiFi.</div>
<div id="msg" class="msg"></div>
<div class="section">
<div class="row"><label data-i18n="lbl_wifi">R&eacute;seau WiFi</label>
<select id="wifi_ssid" style="width:100%"><option value="" data-i18n="scan_wait">-- Scan en cours... --</option></select>
<button class="btn-scan" onclick="scanWiFi()" data-i18n="btn_scan">&#x1F50D; Scanner les r&eacute;seaux</button>
<input type="text" id="wifi_ssid_text" data-i18n-placeholder="ph_manual" placeholder="Ou saisir le nom manuellement" style="width:100%;margin-top:4px;padding:8px 10px;border:1px solid #555;border-radius:4px;background:#0f3460;color:#eee;font-size:14px">
</div>
<div class="row"><label data-i18n="lbl_pwd">Mot de passe</label>
<div class="pwd-row"><input type="password" id="wifi_password" data-i18n-placeholder="ph_pwd" placeholder="Mot de passe WiFi"><button class="pwd-toggle" id="pwdToggle" onclick="togglePwd()">&#x1F441;</button></div>
</div>
<div class="row"><label data-i18n="lbl_static_ip">IP statique (optionnel)</label><input type="text" id="wifi_static_ip" data-i18n-placeholder="ph_static_ip" placeholder="Laisser vide pour DHCP"></div>
<div class="btn-row"><button class="btn" onclick="saveWiFi()" data-i18n="btn_save">&#x1F504; Sauvegarder &amp; Red&eacute;marrer</button></div>
</div>
<script>
// ============= I18N (page AP -- premier boot / mode secours WiFi) =============
const AP_I18N={
fr:{title:'RecalBox DMD',h1:'&#x1F4E1; Configuration WiFi',hint:'Connectez-vous au réseau <b>RecalBox-DMD-Config</b> puis sélectionnez votre WiFi.',lbl_wifi:'Réseau WiFi',scan_wait:'-- Scan en cours... --',btn_scan:'&#x1F50D; Scanner les réseaux',ph_manual:'Ou saisir le nom manuellement',lbl_pwd:'Mot de passe',ph_pwd:'Mot de passe WiFi',lbl_static_ip:'IP statique (optionnel)',ph_static_ip:'Laisser vide pour DHCP',btn_save:'&#x1F504; Sauvegarder &amp; Redémarrer',sel_placeholder:'-- Sélectionnez --',no_networks:'Aucun réseau trouvé',scan_error:'Erreur scan',need_ssid:'Veuillez sélectionner ou saisir un réseau WiFi',saving:'Enregistrement...',restarting:'Redémarrage...',net_error:'Erreur réseau'},
en:{title:'RecalBox DMD',h1:'&#x1F4E1; WiFi Setup',hint:'Connect to the <b>RecalBox-DMD-Config</b> network then select your WiFi.',lbl_wifi:'WiFi network',scan_wait:'-- Scanning... --',btn_scan:'&#x1F50D; Scan networks',ph_manual:'Or type the name manually',lbl_pwd:'Password',ph_pwd:'WiFi password',lbl_static_ip:'Static IP (optional)',ph_static_ip:'Leave empty for DHCP',btn_save:'&#x1F504; Save &amp; Restart',sel_placeholder:'-- Select --',no_networks:'No network found',scan_error:'Scan error',need_ssid:'Please select or type a WiFi network',saving:'Saving...',restarting:'Restarting...',net_error:'Network error'},
es:{title:'RecalBox DMD',h1:'&#x1F4E1; Configuración WiFi',hint:'Conéctese a la red <b>RecalBox-DMD-Config</b> y luego seleccione su WiFi.',lbl_wifi:'Red WiFi',scan_wait:'-- Escaneando... --',btn_scan:'&#x1F50D; Escanear redes',ph_manual:'O escriba el nombre manualmente',lbl_pwd:'Contraseña',ph_pwd:'Contraseña WiFi',lbl_static_ip:'IP estática (opcional)',ph_static_ip:'Dejar vacío para DHCP',btn_save:'&#x1F504; Guardar y reiniciar',sel_placeholder:'-- Seleccione --',no_networks:'No se encontraron redes',scan_error:'Error de escaneo',need_ssid:'Seleccione o escriba una red WiFi',saving:'Guardando...',restarting:'Reiniciando...',net_error:'Error de red'}
};
let currentLang='fr';
function tr(k){return (AP_I18N[currentLang]&&AP_I18N[currentLang][k])||AP_I18N.fr[k]||k;}
function applyLang(backendLang){
  const stored=localStorage.getItem('dmd_lang');
  if(stored&&AP_I18N[stored]){currentLang=stored;}
  else if(backendLang&&AP_I18N[backendLang]){currentLang=backendLang;}
  else{const nav=(navigator.language||'').substring(0,2);currentLang=AP_I18N[nav]?nav:'fr';}
  document.documentElement.lang=currentLang;
  document.title=tr('title');
  document.querySelectorAll('[data-i18n]').forEach(function(el){el.innerHTML=tr(el.dataset.i18n);});
  document.querySelectorAll('[data-i18n-placeholder]').forEach(function(el){el.placeholder=tr(el.dataset.i18nPlaceholder);});
  document.getElementById('langSelect').value=currentLang;
}
function setLang(code){
  localStorage.setItem('dmd_lang',code);
  applyLang();
  fetch('/save-language',{method:'POST',body:'language='+code,headers:{'Content-Type':'application/x-www-form-urlencoded'}}).catch(function(){});
}
// ============= END I18N =============
function stripAccents(s){return s.normalize('NFD').replace(new RegExp('['+String.fromCharCode(768)+'-'+String.fromCharCode(879)+']','g'),'').replace(/[^ -~]/g,'?');}
function showMsg(t,ok){var e=document.getElementById('msg');e.textContent=t;e.className='msg '+(ok?'msg-ok':'msg-err');e.style.display='block';if(window._msgTimer)clearTimeout(window._msgTimer);window._msgTimer=setTimeout(function(){e.style.display='none';},5000);fetch('/dmd-pause',{method:'POST',body:new URLSearchParams({msg:stripAccents(t),color:ok?'1':'2'}),headers:{'Content-Type':'application/x-www-form-urlencoded'}}).catch(function(){});}
function togglePwd(){var p=document.getElementById('wifi_password');p.type=(p.type=='password'?'text':'password');}
function scanWiFi(){
  var sel=document.getElementById('wifi_ssid');sel.innerHTML='<option value="">'+tr('scan_wait')+'</option>';
  fetch('/scan-wifi').then(function(r){return r.json();}).then(function(nets){
    sel.innerHTML='<option value="">'+tr('sel_placeholder')+'</option>';
    if(nets&&nets.length) nets.forEach(function(n){sel.innerHTML+='<option value="'+n+'">'+n+'</option>';});
    else sel.innerHTML='<option value="">'+tr('no_networks')+'</option>';
  }).catch(function(){sel.innerHTML='<option value="">'+tr('scan_error')+'</option>';});
}
function saveWiFi(){
  var sel=document.getElementById('wifi_ssid');
  var txt=document.getElementById('wifi_ssid_text');
  var ssid=sel.value||txt.value.trim();
  if(!ssid){showMsg(tr('need_ssid'),false);return;}
  var pwd=document.getElementById('wifi_password').value.trim();
  var ip=document.getElementById('wifi_static_ip').value.trim();
  var body='wifi_enabled=1&wifi_ssid='+encodeURIComponent(ssid)+'&wifi_password='+encodeURIComponent(pwd);
  body+='&wifi_static_enabled='+(ip?1:0)+'&wifi_static_ip='+encodeURIComponent(ip);
  showMsg(tr('saving'),true);
  fetch('/save-ap',{method:'POST',body:body,headers:{'Content-Type':'application/x-www-form-urlencoded'}})
    .then(function(r){return r.text();})
    .then(function(t){if(t.includes('OK'))showMsg(tr('restarting'),true);else showMsg(t,false);})
    .catch(function(){showMsg(tr('net_error'),false);});
}
fetch('/lang').then(function(r){return r.json();}).then(function(d){applyLang(d.language);scanWiFi();}).catch(function(){applyLang();scanWiFi();});
</script>
</body>
</html>
)rawliteral";

// ================================================
// Handler helpers
// ================================================
static String jsonEscape(const String &s)
{
  String out;
  out.reserve(s.length() + 4);
  for (unsigned int i = 0; i < s.length(); i++) {
    char c = s.charAt(i);
    if (c == '"') out += "\\\"";
    else if (c == '\\') out += "\\\\";
    else if (c == '\r') out += "\\r";
    else if (c == '\n') out += "\\n";
    else if (c == '\t') out += "\\t";
    else out += c;
  }
  return out;
}

static void handleWebConfigLoad()
{
  unsigned long t0 = millis(); // DIAGNOSTIC TEMPORAIRE (2026-07-30) -- lenteur page rapportee hors generation
  int b = (screenBrightness * 100 + 127) / 255;
  String json = "{";
  json += "\"brightness\":\"" + String(b) + "\"";
  json += ",\"info\":\"" + String(showInfo ? '1' : '0') + "\"";
  json += ",\"playlist\":\"" + jsonEscape(playlistName) + "\"";
  json += ",\"random\":\"" + String(playlistRandom ? '1' : '0') + "\"";
  json += ",\"wifi_enabled\":\"" + String(wifiEnabled ? '1' : '0') + "\"";
  json += ",\"wifi_ssid\":\"" + jsonEscape(wifiSSID) + "\"";
  json += ",\"wifi_password\":\"" + jsonEscape(wifiPassword) + "\"";
  json += ",\"wifi_static_enabled\":\"" + String(wifiStaticEnabled ? '1' : '0') + "\"";
  json += ",\"wifi_static_ip\":\"" + jsonEscape(wifiStaticIP) + "\"";
  json += ",\"wifi_gateway\":\"" + jsonEscape(wifiGateway) + "\"";
  json += ",\"wifi_subnet\":\"" + jsonEscape(wifiSubnet) + "\"";
  json += ",\"wifi_dns1\":\"" + jsonEscape(wifiDNS1) + "\"";
  json += ",\"wifi_dns2\":\"" + jsonEscape(wifiDNS2) + "\"";
  json += ",\"bluetooth_enabled\":\"" + String(bluetoothEnabled ? '1' : '0') + "\"";
  json += ",\"bluetooth_name\":\"" + jsonEscape(bluetoothName) + "\"";
  json += ",\"recalbox_ip\":\"" + jsonEscape(recalboxIP) + "\"";
  json += ",\"clock_enabled\":\"" + String(clockEnabled ? '1' : '0') + "\"";
  json += ",\"clock_theme\":\"" + String(clockTheme) + "\"";
  {
    char neonColorBuf[8];
    snprintf(neonColorBuf, sizeof(neonColorBuf), "#%02X%02X%02X", clockNeonR, clockNeonG, clockNeonB);
    json += ",\"clock_neon_color\":\"" + String(neonColorBuf) + "\"";
  }
  json += ",\"clock_neon_color_enabled\":\"" + String(clockNeonCustomColor ? '1' : '0') + "\"";
  json += ",\"clock_interval\":\"" + String(clockIntervalGifs) + "\"";
  json += ",\"clock_interval_min\":\"" + String(clockIntervalMin) + "\"";
  json += ",\"clock_duration\":\"" + String(clockDuration) + "\"";
  json += ",\"clock_tz\":\"" + jsonEscape(clockTimeZone) + "\"";
  json += "}";
  Serial.println("[WEB] load: " + String(millis() - t0) + "ms"); // DIAGNOSTIC TEMPORAIRE
  webServer->send(200, "application/json", json);
}

// Lecture rapide (mutex non bloquant, hold time negligeable) de l'etat
// "generation de playlist active ?" -- utilisee pour garder les handlers
// listes ci-dessous en dehors de toute generation en cours, meme regle que
// les autres handlers SD deja gardes (upload/creation-suppression de dossier/
// suppression de playlist) : une ecriture/lecture SD concurrente avec
// playlistGenTask() (qui peut tenir sdAccessMutex plusieurs secondes sur un
// dossier lent) serait a risque.
static bool plGenIsActive()
{
  bool a = false;
  if (xSemaphoreTake(plGenStatusMutex, 0) == pdTRUE) { a = g_plGenStatus.active; xSemaphoreGive(plGenStatusMutex); }
  return a;
}

static void handleWebConfigListPlaylists()
{
  unsigned long t0 = millis(); // DIAGNOSTIC TEMPORAIRE (2026-07-30) -- lenteur page rapportee hors generation, y compris hors upload
  // Rafraichissement silencieux (pas une action utilisateur explicite) --
  // renvoie une liste vide plutot qu'une erreur 409 pendant une generation.
  if (plGenIsActive()) {
    Serial.println("[WEB] lsplaylists: generation active, liste vide (" + String(millis() - t0) + "ms)"); // DIAGNOSTIC TEMPORAIRE
    webServer->send(200, "application/json", "[]");
    return;
  }
  String json = "[";
  int n = 0;
  File dir = SD.open("/playlists");
  if (dir && dir.isDirectory()) {
    bool first = true;
    File entry = dir.openNextFile();
    while (entry) {
      String name = String(entry.name());
      int slash = name.lastIndexOf('/');
      if (slash >= 0) name = name.substring(slash + 1);
      if (!entry.isDirectory() && name.endsWith(".txt")) {
        if (!first) json += ",";
        json += "\"" + name + "\""; first = false; n++;
      }
      entry.close(); entry = dir.openNextFile();
      delay(1);
    }
    dir.close();
  }
  json += "]";
  Serial.println("[WEB] lsplaylists: " + String(n) + " playlist(s) en " + String(millis() - t0) + "ms, maxalloc=" + String(ESP.getMaxAllocHeap())); // DIAGNOSTIC TEMPORAIRE
  webServer->send(200, "application/json", json);
}

// Chemin du fichier maitre interne (plan cache_master_gifs, simplification
// 2026-08-01 -- voir commentaire de filterMasterIntoFile() plus bas pour
// l'historique complet). Extension ".dat" (distincte de toute playlist
// ".txt") : jamais confondu avec une playlist nulle part, MAIS desormais
// tenu a jour AUTOMATIQUEMENT par deux mecanismes independants (plus de
// bouton "Resynchroniser" manuel, retire avec tousSyncTask()) : (1) tout
// upload web vers un dossier deja connu OU nouveau via
// handleWebConfigAddToPlaylistsBatch() (cache_master_gifs.dat est toujours
// une cible d'ajout inconditionnelle), (2) l'embarquement automatique d'un
// dossier a sa premiere apparition dans une generation de playlist (voir
// playlistGenTask() plus bas). Defini ici (avant sa premiere utilisation
// dans ce fichier, handleWebConfigListGifDirs() juste en dessous).
#define TOUS_MASTER_PATH "/playlists/cache_master_gifs.dat"

// B (plan cache_master_gifs) -- RETIRE (2026-08-02, retour test reel) : le
// compte de fichiers par dossier (scan de TOUS_MASTER_PATH ici +
// /lsgifdircount a la coche) a ete retire a titre de test pour isoler sa
// contribution eventuelle a la pression heap observee pendant cette session
// (crash reel out-of-memory dans WebServer::_parseForm() pendant un upload,
// voir changelog). Retour a la version simple (liste de noms uniquement),
// tri alphabetique cote JS conserve (pur JS, sans cout heap firmware).
static void handleWebConfigListGifDirs()
{
  unsigned long t0 = millis(); // DIAGNOSTIC TEMPORAIRE (2026-07-30)
  if (plGenIsActive()) {
    Serial.println("[WEB] lsgifdirs: generation active, liste vide (" + String(millis() - t0) + "ms)"); // DIAGNOSTIC TEMPORAIRE
    webServer->send(200, "application/json", "[]");
    return;
  }
  String json = "[";
  int n = 0;
  File dir = SD.open("/gifs");
  if (dir && dir.isDirectory()) {
    bool first = true;
    File entry = dir.openNextFile();
    while (entry) {
      if (entry.isDirectory()) {
        String name = String(entry.name());
        int slash = name.lastIndexOf('/');
        if (slash >= 0) name = name.substring(slash + 1);
        if (!first) json += ",";
        json += "\"" + name + "\"";
        first = false; n++;
      }
      entry.close(); entry = dir.openNextFile();
      delay(1);
    }
    dir.close();
  }
  json += "]";
  Serial.println("[WEB] lsgifdirs: " + String(n) + " dossier(s) en " + String(millis() - t0) + "ms, maxalloc=" + String(ESP.getMaxAllocHeap())); // DIAGNOSTIC TEMPORAIRE
  webServer->send(200, "application/json", json);
}

// v92 -- handleWebConfigListGifFiles()/handleWebConfigGifCount() (listing du
// CONTENU d'un dossier, compteur de fichiers) retires : aucune page reelle
// ne les appelle plus, la navigation/suppression fichier par fichier ayant
// ete abandonnee (decision produit anterieure) -- voir plan de
// reconstruction.

// Forward declaration -- definie plus bas avec le cache g_plRefCache*, mais
// invalidee ici (creation/suppression de playlist) et dans
// handleWebConfigDeletePlaylist().
static void invalidatePlaylistRefCache();

// Machine a etats non-bloquante de generation de playlist, sur sa PROPRE
// tache FreeRTOS (playlistGenTask(), 2026-07-28) -- remplace l'ancienne
// version qui tournait sur loop() par petits pas bornes
// (PLGEN_MAX_FILES_PER_STEP=1) : une lenteur SD localisee (confirmee en test
// reel sur plusieurs dossiers distincts -- simple listing openNextFile(),
// sans lecture de contenu, parfois plusieurs secondes par fichier, cause non
// identifiee mais pas un bug de code) gelait quand meme loop() -- donc le
// serveur web ET le bouton "Arreter" ET /reboot -- pendant toute la duree de
// l'appel SD en cours, meme avec un lot de 1 fichier. Deplacer le scan sur sa
// propre tache elimine le probleme a la racine : loop() (donc le WebServer
// et la lecture GIF) ne depend plus jamais de la vitesse d'un appel SD
// individuel de ce scan.
//
// POST /generate-playlist demarre la tache et repond immediatement
// ("STARTED") ; GET /generate-playlist-status lit un instantane de
// g_plGenStatus (struct definie dans RecalBox_DMD.ino avant #include
// "web_config.h", meme raison que MqttCommand : web_config.h l'utilise avant
// sa "vraie" position dans le fichier) sous plGenStatusMutex -- jamais de SD
// dans la section critique, hold time toujours negligeable des 2 cotes ;
// POST /generate-playlist-stop pose juste stopRequested, la tache se termine
// proprement a son prochain point de controle (entre deux dossiers ou deux
// fichiers).
//
// IMPORTANT -- sdAccessMutex protege tout acces SD partage entre cette tache
// et loop() (lecture GIF a chaque frame, voir gifPlayFrameCompat() dans
// RecalBox_DMD.ino) : SEULE cette tache peut l'attendre de facon bloquante
// (portMAX_DELAY, utilise partout ci-dessous). loop()/les handlers HTTP ne
// doivent JAMAIS l'attendre bloquant -- toujours xSemaphoreTake(sdAccessMutex,
// 0) + degradation gracieuse si indisponible, sinon un scan lent regelerait
// exactement le meme probleme, juste deplace vers la lecture GIF au lieu du
// serveur web (voir le commentaire complet dans RecalBox_DMD.ino, juste avant
// #include "web_config.h").
//
// Chaque appel SD individuel (un SD.open(), un openNextFile(), un print())
// prend et rend sdAccessMutex separement -- jamais un lock tenu sur tout un
// dossier ou plusieurs fichiers d'affilee : ca borne la fenetre de blocage
// possible du thread principal a la duree d'un seul appel SD, jamais plus.
//
// Cette tache ne touche JAMAIS gif/display/currentMode directement (proprietes
// de loop()) -- seulement g_plGenStatus. C'est loop() qui, periodiquement, lit
// cet instantane et met a jour l'ecran DMD si le mode config est actif (voir
// webDmdOverlayLine2()/RecalBox_DMD.ino, loop()).

// Texte DMD compact -- fonction pure (pas de lecture de globals), utilisable
// a la fois depuis playlistGenTask() et depuis loop() (overlay progression).
// "Scan: <nom> (i/total) X/Y" pouvait depasser 30 caracteres, largement
// au-dessus de ce qu'un panneau 128px affiche sans defilement, et les mises
// a jour frequentes interrompaient le defilement avant un tour complet
// (illisible en pratique, retour utilisateur 2026-07-28) -- nom tronque a 10
// caracteres, pas de prefixe/index (deja visibles sur la page web). Comptage
// du total cible par dossier retire (meme date, demande explicite) : un 2e
// listing complet doublait l'exposition aux lenteurs SD deja documentees
// pour un gain d'affichage juge trop couteux.
String plGenDmdText(const String &dirName, int count)
{
  String n = dirName;
  if (n.length() > 10) n = n.substring(0, 10) + "..";
  return n + " " + String(count);
}

// Forward declaration -- definie plus bas avec deleteFolderRecursive() (meme
// fonction de suppression tolerante FAT32 lecture-seule), utilisee par
// playlistGenTask() pour supprimer la playlist partielle en cas d'arret
// demande par l'utilisateur.
static bool forceDeleteFile(const String &path);

// Ecrit buf dans f en verifiant le nombre reel d'octets ecrits (2026-07-30) :
// File::print() peut ecrire MOINS que demande sans lever d'erreur -- valeur
// de retour jamais verifiee jusqu'ici dans tout ce fichier, a chaque flush
// intermediaire de buffer (toutes les fonctions de scan/filtrage). Perte de
// donnees SILENCIEUSE confirmee en test reel (2026-07-30) : 15 fichiers
// consecutifs (meme prefixe, meme dossier) manquants dans _master_gifs.txt
// apres un scan complet reussi sans aucune erreur signalee -- un seul flush
// partiel explique exactement ce genre de trou contigu. Reessaie jusqu'a 3
// fois la partie non ecrite (delay(2) entre tentatives, laisse une chance a
// un hoquet SPI/SD transitoire de se resorber) avant d'abandonner avec un
// avertissement explicite (perte de donnees rarissime mais au moins visible
// au lieu de silencieuse).
// Retourne false si une partie du buffer n'a pas pu etre ecrite meme apres
// retries (2026-07-30) : permet a l'appelant de signaler le resultat comme
// suspect (voir hadWriteLoss dans playlistGenTask()/filterMasterIntoFile())
// plutot que de faire confiance a un fichier potentiellement troue en
// silence.
static bool writeBufChecked(File &f, const String &buf)
{
  size_t total = buf.length();
  size_t offset = 0;
  int attempts = 0;
  while (offset < total && attempts < 3) {
    // buf.substring(offset) SEULEMENT si necessaire (offset>0, cas de retry
    // rarissime) -- BUG CORRIGE (2026-07-30) : appeler substring(0) sur
    // CHAQUE tentative dupliquait inutilement tout le buffer (encore ~1000
    // octets a allouer) juste pour appeler print(), en plus de buf lui-meme
    // -- sous heap deja critique (confirme en test reel : maxalloc=8692 au
    // demarrage de la tache, degrade ensuite), cette allocation supplementaire
    // echouait silencieusement (String::substring() sur allocation ratee
    // renvoie une chaine VIDE, pas une erreur) -- print("") renvoie alors 0,
    // faussement interprete comme un echec d'ecriture SD alors que c'etait
    // uniquement ce correctif lui-meme qui aggravait la pression heap.
    size_t w = (offset == 0) ? f.print(buf) : f.print(buf.substring(offset));
    if (w == 0) { attempts++; delay(2); continue; }
    offset += w;
  }
  if (offset < total) {
    Serial.println("[WEB] writeBufChecked: PERTE DE DONNEES -- " + String(total - offset) + "/" + String(total) + " octets non ecrits apres retries");
    return false;
  }
  return true;
}

// Forward declarations -- definies plus bas dans ce fichier, mais utilisees
// par playlistGenTask()/handleWebConfigGeneratePlaylist() ci-dessous.
static bool fileContainsNeedle(File &f, const String &needle);
static bool filterMasterIntoFile(const String &dirsCsv, File &outFile, int &linesWrittenOut, bool &hadWriteLossOut, String &errOut);
static bool appendMatchingLines(const String &srcPath, const String &wantedCsv, const String &destPath, bool &hadWriteLossOut);

// name : nom de la playlist a creer. cachedDirsCsv/uncachedDirsCsv (format
// ",dir1,dir2,") : repartition decidee par handleWebConfigGeneratePlaylist()
// selon la presence de chaque dossier dans TOUS_MASTER_PATH. fullMarker :
// tous les dossiers demandes (cachedDirsCsv + uncachedDirsCsv), format
// "dir1,dir2" sans virgule d'encadrement -- ecrit tel quel en tete du
// fichier de sortie (marqueur "# FULL:", voir playlistGenTask()).
struct PlaylistGenRequest { String name; String cachedDirsCsv; String uncachedDirsCsv; String fullMarker; };

// Tourne du debut a la fin sur sa propre tache (creee a la demande, voir
// handleWebConfigGeneratePlaylist()) -- plus besoin d'une borne "fichiers par
// appel" (existait uniquement pour borner le cout par appel loop(), obsolete
// des que ce n'est plus loop() qui l'appelle). Tache PERSISTANTE essayee puis
// abandonnee le 2026-07-29 -- cf. commentaire au-dessus de la declaration de
// playlistGenTaskHandle (RecalBox_DMD.ino) pour le detail de ce qui a ete
// tente et pourquoi.
// Coeur du scan (parse dirsCsv, ouvre chaque /gifs/<dossier>, ecrit les
// chemins .gif trouves dans outFile deja ouvert) -- appelee par
// playlistGenTask() UNIQUEMENT sur les dossiers pas encore couverts par le
// fichier maitre interne (uncachedDirsCsv, voir plan cache_master_gifs) --
// la portion deja couverte est desormais filtree depuis le fichier maitre
// (filterMasterIntoFile(), quasi instantane) sans jamais toucher /gifs/.
// Ne touche JAMAIS g_plGenStatus.active/resultMsg/done -- l'appelant garde
// la responsabilite de les positionner, seul g_plGenStatus.curDirName/
// dirIdx/curDirGifs/totalGifs (progression, deja affichee sur le DMD/la
// page web) est mis a jour ici.
static void scanFoldersToPlaylistFile(const String &dirsCsv, File &outFile,
                                       int &totalGifsOut, bool &stoppedOut, bool &lowHeapAbortOut,
                                       bool &hadWriteLossOut)
{
  int totalGifs = 0, dirIdx = 0, parseIdx = 0;
  String buf;
  bool stopped = false;
  bool lowHeapAbort = false;
  bool hadWriteLoss = false;

  while (parseIdx <= (int)dirsCsv.length())
  {
    if (xSemaphoreTake(plGenStatusMutex, portMAX_DELAY) == pdTRUE) {
      stopped = g_plGenStatus.stopRequested;
      xSemaphoreGive(plGenStatusMutex);
    }
    if (stopped) break;

    int comma = dirsCsv.indexOf(',', parseIdx);
    String dirName = (comma < 0) ? dirsCsv.substring(parseIdx) : dirsCsv.substring(parseIdx, comma);
    dirName.trim();
    parseIdx = (comma < 0) ? (int)(dirsCsv.length() + 1) : (comma + 1);
    if (dirName.length() == 0) continue; // segment vide (virgules successives)

    dirIdx++;
    int curDirGifs = 0;
    if (xSemaphoreTake(plGenStatusMutex, portMAX_DELAY) == pdTRUE) {
      g_plGenStatus.curDirName = dirName;
      g_plGenStatus.dirIdx = dirIdx;
      g_plGenStatus.curDirGifs = 0;
      xSemaphoreGive(plGenStatusMutex);
    }

    File dir;
    if (xSemaphoreTake(sdAccessMutex, portMAX_DELAY) == pdTRUE) {
      dir = SD.open(("/gifs/" + dirName).c_str());
      xSemaphoreGive(sdAccessMutex);
    }
    bool dirOpen = dir && dir.isDirectory();
    if (!dirOpen && dir) {
      if (xSemaphoreTake(sdAccessMutex, portMAX_DELAY) == pdTRUE) { dir.close(); xSemaphoreGive(sdAccessMutex); }
    }

    // Cache par dossier ESSAYE puis RETIRE le 2026-07-29 (mtime + cache
    // centralise /playlists/<dossier>_dircache.txt, plusieurs iterations :
    // dans le dossier lui-meme, puis centralise, puis RAM-only) -- 4 bugs
    // reels trouves sur cette seule fonctionnalite (descripteurs simultanes,
    // dossier modifie en cours d'enumeration, perte de donnees au flush,
    // comptages erratifs), et le dernier test reel a confirme que meme la
    // version RAM-only + tache creee a la demande (design d'origine)
    // continuait a planter/donner des comptages faux -- donc le probleme
    // n'etait pas la tache persistante, mais ce code de cache lui-meme.
    // Abandonne : le gain reel ne couvrait de toute facon pas les gros
    // dossiers lents (Arcade/Consoles/Halloween/Vertical_DMD, la vraie
    // cible), un plafond RAM les excluant systematiquement. Remplace a
    // terme par une approche filtrage-de-texte sur un TOUS.txt tenu a jour
    // (voir discussion/plan a venir), qui evite completement l'enumeration
    // repetee de /gifs/<dossier>.
    while (dirOpen)
    {
      if (xSemaphoreTake(plGenStatusMutex, portMAX_DELAY) == pdTRUE) {
        stopped = g_plGenStatus.stopRequested;
        xSemaphoreGive(plGenStatusMutex);
      }
      // Garde-fou heap critique (2026-07-29, crash reel : abort() par
      // allocation heap echouee, meme classe de bug deja documentee sur ce
      // projet -- exceptions C++ desactivees -> abort() direct au lieu d'une
      // exception rattrapable). maxalloc se degrade au fil d'un long scan ;
      // sans ce garde, une allocation (String/File) finissait par echouer et
      // faisait planter/redemarrer tout l'appareil. Traite comme un arret
      // demande : sortie propre plutot qu'un crash.
      // Seuil laisse a 4096 (2026-07-29) : hypothese revue -- maxalloc
      // pendant un fonctionnement normal reussi se situe couramment entre
      // 4500 et 9000, donc un seuil remonte a 8192 declencherait le
      // garde-fou en permanence, meme sur un petit dossier (marge reelle
      // entre succes/crash mesuree a seulement ~250 octets, pas plusieurs
      // milliers). Suspicion actuelle : le crash vient d'une course avec
      // mqttTask() (meme coeur, tentatives de connexion concurrentes) plutot
      // que d'un heap simplement trop bas -- mqttTask() ne tente plus de
      // connexion pendant une generation active (voir RecalBox_DMD.ino),
      // teste en isolation avant de reconsiderer ce seuil.
      if (!stopped && ESP.getMaxAllocHeap() < 4096) {
        Serial.println("[WEB] playlistGenTask: heap critique (maxalloc=" + String(ESP.getMaxAllocHeap()) + "), arret propre du scan");
        stopped = true;
        lowHeapAbort = true;
      }
      if (stopped) {
        if (xSemaphoreTake(sdAccessMutex, portMAX_DELAY) == pdTRUE) { dir.close(); xSemaphoreGive(sdAccessMutex); }
        break;
      }

      File f;
      if (xSemaphoreTake(sdAccessMutex, portMAX_DELAY) == pdTRUE) {
        f = dir.openNextFile();
        xSemaphoreGive(sdAccessMutex);
      }
      if (!f) {
        if (xSemaphoreTake(sdAccessMutex, portMAX_DELAY) == pdTRUE) { dir.close(); xSemaphoreGive(sdAccessMutex); }
        dirOpen = false;
        break;
      }
      if (!f.isDirectory()) {
        String fname = String(f.name());
        if (fname.endsWith(".gif")) {
          buf += "/gifs/" + dirName + "/" + fname + "\n";
          totalGifs++;
          curDirGifs++;
          // Seuil de flush reduit de 4000 a 1000 (2026-07-29) : reduit la
          // taille de pic d'allocation transitoire pendant la concatenation
          // (String::operator+= peut reallouer un buffer plus grand avant de
          // copier), un contributeur plausible au heap critique ci-dessus
          // sur un scan long.
          if (buf.length() > 1000) {
            if (xSemaphoreTake(sdAccessMutex, portMAX_DELAY) == pdTRUE) { if (!writeBufChecked(outFile, buf)) hadWriteLoss = true; xSemaphoreGive(sdAccessMutex); }
            buf = "";
          }
        }
      }
      f.close();

      if (xSemaphoreTake(plGenStatusMutex, portMAX_DELAY) == pdTRUE) {
        g_plGenStatus.curDirGifs = curDirGifs;
        g_plGenStatus.totalGifs = totalGifs;
        xSemaphoreGive(plGenStatusMutex);
      }
      vTaskDelay(1); // laisse tourner mqttTask()/l'idle task -- bonne conduite FreeRTOS, pas une borne de cout
    }
    if (stopped) break;
  }

  // Flush final du reliquat -- SEULEMENT si le scan s'est termine
  // normalement (un arret/heap-critique doit laisser le fichier de sortie
  // intact pour que l'appelant puisse decider de le supprimer ou non selon
  // son propre contexte).
  if (!stopped && buf.length() > 0) {
    if (xSemaphoreTake(sdAccessMutex, portMAX_DELAY) == pdTRUE) { if (!writeBufChecked(outFile, buf)) hadWriteLoss = true; xSemaphoreGive(sdAccessMutex); }
  }
  totalGifsOut = totalGifs;
  stoppedOut = stopped;
  lowHeapAbortOut = lowHeapAbort;
  hadWriteLossOut = hadWriteLoss;
}

void playlistGenTask(void *param)
{
  PlaylistGenRequest *req = (PlaylistGenRequest *)param;
  String name = req->name;
  String cachedDirsCsv = req->cachedDirsCsv;
  String uncachedDirsCsv = req->uncachedDirsCsv;
  String fullMarker = req->fullMarker;
  delete req;

  String outputPath = "/playlists/" + name + ".txt";
  File outFile;
  if (xSemaphoreTake(sdAccessMutex, portMAX_DELAY) == pdTRUE) {
    outFile = SD.open(outputPath.c_str(), FILE_WRITE);
    xSemaphoreGive(sdAccessMutex);
  }
  if (!outFile) {
    // Deja valide par handleWebConfigGeneratePlaylist() avant de lancer cette
    // tache -- ne devrait pas arriver, protection quand meme.
    if (xSemaphoreTake(plGenStatusMutex, portMAX_DELAY) == pdTRUE) {
      g_plGenStatus.resultMsg = "ERR: ecriture impossible (" + name + ".txt)";
      g_plGenStatus.active = false;
      g_plGenStatus.done = true;
      xSemaphoreGive(plGenStatusMutex);
    }
    playlistGenTaskHandle = nullptr;
    vTaskDelete(nullptr);
    return;
  }

  bool hadWriteLoss = false;

  // Marqueur "# FULL:" (plan cache_master_gifs) -- ECRIT ICI et jamais avant
  // (par handleWebConfigGeneratePlaylist()) : FILE_WRITE vaut "w" (voir
  // FS.h), qui TRONQUE le fichier a l'ouverture -- un marqueur ecrit plus
  // tot serait silencieusement efface des que cette tache rouvre
  // outputPath ci-dessus. Toute selection DMD porte toujours sur des
  // dossiers ENTIERS (jamais une selection fichier par fichier) : ce
  // marqueur protege cette playlist d'un ajout automatique errone lors d'un
  // futur upload vers un dossier dont seule une partie aurait ete demandee
  // (voir handleWebConfigAddToPlaylistsBatch()).
  if (xSemaphoreTake(sdAccessMutex, portMAX_DELAY) == pdTRUE) {
    String marker = "# FULL:" + fullMarker + "\n";
    if (!writeBufChecked(outFile, marker)) hadWriteLoss = true;
    xSemaphoreGive(sdAccessMutex);
  }

  // Portion "deja en cache" (generation hybride, plan cache_master_gifs) --
  // quasi instantanee, ecrite en premier directement dans le fichier de
  // sortie deja ouvert (pas de temp+rename separe ici : l'integrite globale
  // du fichier est deja garantie par le mecanisme existant plus bas, qui
  // supprime outputPath entierement en cas d'arret/heap-critique pendant la
  // phase de scan qui suit).
  int totalGifsFromCache = 0;
  if (cachedDirsCsv.length() > 1) {
    int linesWritten = 0;
    bool cacheWriteLoss = false;
    String err;
    if (xSemaphoreTake(sdAccessMutex, portMAX_DELAY) == pdTRUE) {
      filterMasterIntoFile(cachedDirsCsv, outFile, linesWritten, cacheWriteLoss, err);
      xSemaphoreGive(sdAccessMutex);
    }
    if (cacheWriteLoss) hadWriteLoss = true;
    totalGifsFromCache = linesWritten;
  }

  // Scan classique -- SEULEMENT sur les dossiers pas encore couverts par le
  // fichier maitre.
  int totalGifsScanned = 0;
  bool stopped = false, lowHeapAbort = false, scanWriteLoss = false;
  if (uncachedDirsCsv.length() > 1) {
    scanFoldersToPlaylistFile(uncachedDirsCsv, outFile, totalGifsScanned, stopped, lowHeapAbort, scanWriteLoss);
    if (scanWriteLoss) hadWriteLoss = true;
  }

  if (stopped)
  {
    if (xSemaphoreTake(sdAccessMutex, portMAX_DELAY) == pdTRUE) {
      outFile.close();
      // gere elle-meme SD.exists()/le cas lecture-seule FAT32 -- BUG CORRIGE
      // (2026-07-28) : cet appel restait hors du mutex jusqu'ici, seul acces
      // SD non protege de toute la tache, exactement sur le chemin declenche
      // par le bouton Arreter -- crash reel observe (abort(), reboot) en
      // test materiel, tres probablement du a cet acces concurrent non
      // protege au bus SD/SPI pendant que l'autre tache lisait une frame GIF.
      // Garde heap AJOUTEE (2026-07-30) : un second crash reel, meme
      // signature exacte (abort()->lock_init_generic()->__sfp, confirme via
      // addr2line), s'est reproduit ICI MEME malgre le mutex ci-dessus --
      // maxalloc etait deja tombe a 5108 avant meme le debut du scan (arret
      // demande apres 311 GIFs). Le mutex protege le BUS SD contre l'acces
      // concurrent, mais forceDeleteFile() ouvre/renomme/supprime un fichier
      // (donc alloue potentiellement un nouveau verrou libc via fopen()),
      // sans le garde-fou heap deja present dans la boucle de scan
      // (scanFoldersToPlaylistFile()) elle-meme. Si le heap est deja trop
      // bas ICI, ne pas tenter le nettoyage -- le fichier partiel reste sur
      // la SD, sera simplement ecrase par la prochaine tentative.
      if (ESP.getMaxAllocHeap() >= 4096) {
        forceDeleteFile(outputPath);
      } else {
        Serial.println("[WEB] playlistGenTask: heap trop bas pour nettoyer " + name + ".txt (fichier partiel laisse sur SD, maxalloc=" + String(ESP.getMaxAllocHeap()) + ")");
      }
      xSemaphoreGive(sdAccessMutex);
    }
    if (lowHeapAbort) {
      Serial.println("[WEB] playlistGenTask: heap insuffisant, " + name + ".txt annulee/supprimee");
    } else {
      Serial.println("[WEB] playlistGenTask: arret demande, " + name + ".txt annulee/supprimee");
    }
    if (xSemaphoreTake(plGenStatusMutex, portMAX_DELAY) == pdTRUE) {
      g_plGenStatus.resultMsg = lowHeapAbort
        ? "Memoire insuffisante, playlist supprimee. Redemarrez le DMD puis reessayez"
        : "Generation annulee, playlist supprimee";
      g_plGenStatus.active = false;
      g_plGenStatus.done = true;
      xSemaphoreGive(plGenStatusMutex);
    }
  }
  else
  {
    if (xSemaphoreTake(sdAccessMutex, portMAX_DELAY) == pdTRUE) {
      outFile.close();
      xSemaphoreGive(sdAccessMutex);
    }
    // Pure RAM, pas de SD -- doit imperativement s'executer AVANT le flip
    // active=false ci-dessous : c'est cet ordre (pas un mutex sur le cache
    // lui-meme) qui garantit qu'un handler du thread principal voyant
    // active=false ne peut lire ce cache qu'apres que cette tache ait fini
    // de le toucher.
    invalidatePlaylistRefCache();

    // Embarquement automatique (plan cache_master_gifs) -- les dossiers
    // nouvellement scannes ci-dessus sont "adoptes" par le fichier maitre :
    // relit les lignes qui viennent d'etre ecrites dans outputPath (pas
    // besoin de rescanner /gifs/ une seconde fois) et les ajoute
    // (FILE_APPEND) a TOUS_MASTER_PATH, qui se cree tout seul au tout
    // premier appel (bootstrap organique -- aucune capacite de bootstrap
    // explicite n'est reintroduite cote firmware). Tout futur upload web
    // vers ce dossier sera desormais suivi automatiquement par
    // handleWebConfigAddToPlaylistsBatch(), sans action supplementaire.
    int adoptedDirCount = 0;
    if (uncachedDirsCsv.length() > 1) {
      int cp = 1;
      while (cp < (int)uncachedDirsCsv.length()) { int cc = uncachedDirsCsv.indexOf(',', cp); if (cc < 0) break; adoptedDirCount++; cp = cc + 1; }
      bool adoptWriteLoss = false;
      bool adoptOk = false;
      size_t masterSizeAfter = 0;
      if (xSemaphoreTake(sdAccessMutex, portMAX_DELAY) == pdTRUE) {
        adoptOk = appendMatchingLines(outputPath, uncachedDirsCsv, TOUS_MASTER_PATH, adoptWriteLoss);
        File chk = SD.open(TOUS_MASTER_PATH, FILE_READ);
        if (chk) { masterSizeAfter = chk.size(); chk.close(); }
        xSemaphoreGive(sdAccessMutex);
      }
      // DIAGNOSTIC TEMPORAIRE (2026-08-01, retour test reel : count "?" en
      // permanence sur la page Affichage) -- confirme si l'embarquement a
      // reellement ecrit quelque chose dans TOUS_MASTER_PATH.
      Serial.println("[WEB] playlistGenTask: embarquement cache -- adoptOk=" + String(adoptOk ? "1" : "0") + " writeLoss=" + String(adoptWriteLoss ? "1" : "0") + " tailleCacheApres=" + String((unsigned long)masterSizeAfter) + " octets");
      if (adoptWriteLoss) hadWriteLoss = true;
    }

    int totalGifs = totalGifsFromCache + totalGifsScanned;
    bool hybrid = (cachedDirsCsv.length() > 1 && uncachedDirsCsv.length() > 1);
    String resultMsg;
    if (hybrid) {
      resultMsg = "OK: " + String(totalGifs) + " GIFs (" + String(totalGifsFromCache) + " depuis le cache + " + String(totalGifsScanned) + " nouvellement scannes";
      if (adoptedDirCount > 0) resultMsg += ", " + String(adoptedDirCount) + " dossier(s) ajoute(s) au cache";
      resultMsg += ") dans la playlist " + name + ".txt";
    } else {
      resultMsg = "OK: " + String(totalGifs) + " GIFs ajoutes dans la playlist " + name + ".txt";
      if (adoptedDirCount > 0) resultMsg += " (" + String(adoptedDirCount) + " dossier(s) ajoute(s) au cache)";
    }
    // hadWriteLoss : contrairement au fichier maitre interne (adopte via
    // appendMatchingLines() ci-dessus), une playlist classique n'a pas de
    // mecanisme de revalidation automatique -- seul un signal explicite
    // permet a l'utilisateur de savoir qu'une regeneration est justifiee.
    if (hadWriteLoss) resultMsg += " (ATTENTION: ecriture incomplete detectee, regenerez cette playlist pour verifier)";
    Serial.println("[WEB] " + resultMsg);
    if (xSemaphoreTake(plGenStatusMutex, portMAX_DELAY) == pdTRUE) {
      g_plGenStatus.resultMsg = resultMsg;
      g_plGenStatus.active = false;
      g_plGenStatus.done = true;
      xSemaphoreGive(plGenStatusMutex);
    }
  }

  // DIAGNOSTIC TEMPORAIRE : marge de pile reellement utilisee (en mots de 4
  // octets sur ESP32) -- valide que 4096 (voir xTaskCreatePinnedToCore() dans
  // handleWebConfigGeneratePlaylist()) est suffisant sans etre dangereusement
  // juste. A retirer une fois confirme sur quelques scans reels.
  Serial.println("[WEB] playlistGenTask: marge de pile restante=" + String(uxTaskGetStackHighWaterMark(nullptr) * 4) + " octets");

  playlistGenTaskHandle = nullptr;
  vTaskDelete(nullptr);
}

// Coeur du filtrage de TOUS_MASTER_PATH, ECRIT DIRECTEMENT dans un File
// deja ouvert (outFile) -- partage par filterPlaylistFromMaster()
// (playlist entierement en cache, chemin synchrone avec son propre
// temp+rename) et playlistGenTask() (portion "deja en cache" d'une
// generation hybride, ecrite directement dans le fichier de sortie deja
// proprietaire de la tache). Meme algorithme de lecture par blocs de 512
// octets que handleWebConfigPlaylistDirs() (pending += buf, decoupage sur
// '\n', report du reliquat, PLUS traitement de la derniere ligne sans '\n'
// final -- piege facile a oublier en adaptant ce motif). Ne touche JAMAIS
// /gifs/.
static bool filterMasterIntoFile(const String &dirsCsv, File &outFile, int &linesWrittenOut, bool &hadWriteLossOut, String &errOut)
{
  linesWrittenOut = 0;
  hadWriteLossOut = false;
  File src = SD.open(TOUS_MASTER_PATH, FILE_READ);
  if (!src) { errOut = "fichier maitre introuvable"; return false; }

  // ",dir1,dir2," -- meme convention que "seen" dans handleWebConfigPlaylistDirs().
  // reserve() : bug reel confirme en test materiel -- sans reservation
  // prealable, String::operator+=() peut echouer SILENCIEUSEMENT sous heap
  // critique (maxalloc=4596 observe) en pleine boucle de concatenation,
  // faisant purement et simplement disparaitre un ou plusieurs dossiers de
  // "wanted" SANS AUCUNE ERREUR VISIBLE -- 5 GIFs obtenus au lieu de ~11000
  // attendus (tous les dossiers coches) sur ce test precis. Une seule
  // grosse allocation en amont (au lieu de N petites reallocations
  // incrementales, chacune un point de defaillance silencieux distinct) et
  // une verification explicite de son succes transforment ce risque en
  // echec net et immediat plutot qu'un resultat faux et muet.
  String wanted = ",";
  if (!wanted.reserve(dirsCsv.length() + 4)) {
    src.close();
    errOut = "memoire insuffisante (liste de dossiers)";
    return false;
  }
  {
    int start = 0;
    while (true) {
      int comma = dirsCsv.indexOf(',', start);
      String d = (comma < 0) ? dirsCsv.substring(start) : dirsCsv.substring(start, comma);
      d.trim();
      if (d.length() > 0) wanted += d + ",";
      if (comma < 0) break;
      start = comma + 1;
    }
  }

  int written = 0;
  bool hadWriteLoss = false;
  String outBuf;
  const size_t BUFSZ = 512;
  char buf[BUFSZ + 1];
  String pending;
  // reserve() (meme classe de bug que "wanted" plus haut) : pending ne
  // depasse jamais vraiment BUFSZ + une ligne (il est retaille a son
  // reliquat apres chaque bloc), donc une seule petite reservation en amont
  // evite les N reallocations incrementales repetees (une par bloc lu,
  // potentiellement des centaines sur un gros fichier maitre) qui sont
  // sinon autant de points de defaillance silencieuse individuels sous heap
  // critique -- une desynchronisation de pending corrompt le decoupage en
  // lignes pour TOUT le reste du fichier, pas seulement la ligne courante.
  pending.reserve(BUFSZ + 256);
  int chunkCount = 0;
  while (true) {
    int n = src.read((uint8_t *)buf, BUFSZ);
    if (n <= 0) break;
    buf[n] = 0;
    pending += buf;
    int lineStart = 0;
    while (true) {
      int nl = pending.indexOf('\n', lineStart);
      if (nl < 0) break;
      String line = pending.substring(lineStart, nl);
      line.trim();
      // Segment dossier = meme extraction que handleWebConfigPlaylistDirs()
      // (indexOf('/', 6) sur le chemin entier) -- jamais un test de
      // sous-chaine naif : "Arcade" ne doit pas matcher dans "Arcade2".
      if (line.startsWith("/gifs/")) {
        int s2 = line.indexOf('/', 6);
        if (s2 > 6) {
          String dir = line.substring(6, s2);
          if (wanted.indexOf("," + dir + ",") >= 0) {
            outBuf += line + "\n";
            written++;
            if (outBuf.length() > 1000) { if (!writeBufChecked(outFile, outBuf)) hadWriteLoss = true; outBuf = ""; }
          }
        }
      }
      lineStart = nl + 1;
    }
    pending = pending.substring(lineStart); // reliquat (ligne a cheval sur 2 blocs) pour le prochain tour
    if ((size_t)n < BUFSZ) break;
    if (++chunkCount % 20 == 0) yield(); // watchdog-safe sur un tres gros fichier maitre (aucun autre point de cession dans cette boucle)
  }
  pending.trim();
  if (pending.startsWith("/gifs/")) { // derniere ligne sans retour a la ligne final
    int s2 = pending.indexOf('/', 6);
    if (s2 > 6) {
      String dir = pending.substring(6, s2);
      if (wanted.indexOf("," + dir + ",") >= 0) { outBuf += pending + "\n"; written++; }
    }
  }
  if (outBuf.length() > 0) { if (!writeBufChecked(outFile, outBuf)) hadWriteLoss = true; }
  src.close();

  linesWrittenOut = written;
  hadWriteLossOut = hadWriteLoss;
  return true;
}

// Filtre le fichier maitre interne (TOUS_MASTER_PATH) vers outputPath (via
// filterMasterIntoFile() ci-dessus), en ecrivant d'abord le marqueur
// "# FULL:" (voir handleWebConfigAddToPlaylistsBatch()) -- toute selection
// DMD porte toujours sur des dossiers entiers. Chemin RAPIDE : dirsCsv est
// entierement couvert par le cache -- tourne directement dans le thread
// loop() (meme raison que handleWebConfigPlaylistDirs()/
// handleWebConfigAddToPlaylistsBatch() qui font deja ca sans tache ni mutex
// : aucune lenteur SD localisee possible sur un fichier texte). Ecrit
// d'abord dans outputPath+".flt" puis remplace atomiquement
// (forceDeleteFile + rename), jamais d'ecriture directe sur outputPath.
static bool filterPlaylistFromMaster(const String &dirsCsv, const String &outputPath, const String &fullMarker,
                                      int &linesWrittenOut, bool &hadWriteLossOut, String &errOut)
{
  String tmpPath = outputPath + ".flt";
  if (SD.exists(tmpPath.c_str())) SD.remove(tmpPath.c_str());
  File out = SD.open(tmpPath.c_str(), FILE_WRITE);
  if (!out) { errOut = "ecriture impossible"; return false; }

  bool hadWriteLoss = false;
  String marker = "# FULL:" + fullMarker + "\n";
  if (!writeBufChecked(out, marker)) hadWriteLoss = true;

  int written = 0;
  bool innerWriteLoss = false;
  bool ok = filterMasterIntoFile(dirsCsv, out, written, innerWriteLoss, errOut);
  if (innerWriteLoss) hadWriteLoss = true;
  out.close();
  if (!ok) { forceDeleteFile(tmpPath); return false; }

  if (SD.exists(outputPath.c_str())) forceDeleteFile(outputPath);
  SD.rename(tmpPath.c_str(), outputPath.c_str());

  // Nettoyage des compagnons perimes -- meme pattern que handleWebConfigDeletePlaylist().
  {
    String base = outputPath.substring(outputPath.lastIndexOf('/') + 1);
    int dot = base.lastIndexOf('.');
    if (dot > 0) base = base.substring(0, dot);
    const char *exts[] = {".cache", ".sig", ".idx"};
    for (int i = 0; i < 3; i++) {
      String p = "/playlists/" + base + exts[i];
      if (SD.exists(p.c_str())) SD.remove(p.c_str());
    }
  }
  invalidatePlaylistRefCache();

  linesWrittenOut = written;
  hadWriteLossOut = hadWriteLoss;
  return true;
}

// B.2.c (plan cache_master_gifs) -- transfere (par ajout, FILE_APPEND) les
// lignes de srcPath dont le dossier appartient a wantedCsv (",dir1,dir2,")
// vers destPath. Utilise pour "adopter" dans TOUS_MASTER_PATH les dossiers
// venant d'etre scannes pour la premiere fois (playlistGenTask()) -- evite
// un second scan de /gifs/, il suffit de relire la playlist qui vient
// elle-meme d'etre ecrite. Cree destPath s'il n'existe pas encore
// (bootstrap organique du fichier maitre, premiere generation de playlist
// jamais lancee sur cette carte). Ignore silencieusement toute ligne qui ne
// commence pas par "/gifs/" (dont le marqueur "# FULL:" en tete de
// srcPath).
static bool appendMatchingLines(const String &srcPath, const String &wantedCsv, const String &destPath, bool &hadWriteLossOut)
{
  hadWriteLossOut = false;
  File src = SD.open(srcPath.c_str(), FILE_READ);
  if (!src) return false;
  File dest = SD.open(destPath.c_str(), FILE_APPEND);
  if (!dest) { src.close(); return false; }

  bool hadWriteLoss = false;
  String outBuf; outBuf.reserve(1200);
  const size_t BUFSZ = 512;
  char buf[BUFSZ + 1];
  String pending; pending.reserve(BUFSZ + 256);
  int chunkCount = 0;
  while (true) {
    int n = src.read((uint8_t *)buf, BUFSZ);
    if (n <= 0) break;
    buf[n] = 0;
    pending += buf;
    int lineStart = 0;
    while (true) {
      int nl = pending.indexOf('\n', lineStart);
      if (nl < 0) break;
      String line = pending.substring(lineStart, nl);
      line.trim();
      if (line.startsWith("/gifs/")) {
        int s2 = line.indexOf('/', 6);
        if (s2 > 6) {
          String dir = line.substring(6, s2);
          if (wantedCsv.indexOf("," + dir + ",") >= 0) { outBuf += line; outBuf += "\n"; }
        }
      }
      lineStart = nl + 1;
    }
    pending = pending.substring(lineStart);
    if (outBuf.length() > 1000) { if (!writeBufChecked(dest, outBuf)) hadWriteLoss = true; outBuf = ""; }
    if ((size_t)n < BUFSZ) break;
    if (++chunkCount % 20 == 0) yield();
  }
  pending.trim();
  if (pending.startsWith("/gifs/")) {
    int s2 = pending.indexOf('/', 6);
    if (s2 > 6) {
      String dir = pending.substring(6, s2);
      if (wantedCsv.indexOf("," + dir + ",") >= 0) { outBuf += pending; outBuf += "\n"; }
    }
  }
  if (outBuf.length() > 0) { if (!writeBufChecked(dest, outBuf)) hadWriteLoss = true; }
  dest.close();
  src.close();
  hadWriteLossOut = hadWriteLoss;
  return true;
}

static void handleWebConfigGeneratePlaylist()
{
  bool alreadyActive = false;
  if (xSemaphoreTake(plGenStatusMutex, portMAX_DELAY) == pdTRUE) {
    alreadyActive = g_plGenStatus.active;
    xSemaphoreGive(plGenStatusMutex);
  }
  if (alreadyActive) { webServer->send(409, "text/plain", "ERR: generation deja en cours"); return; }
  if (!webServer->hasArg("name") || !webServer->hasArg("dirs")) {
    webServer->send(400, "text/plain", "ERR: manque nom ou dirs"); return;
  }
  String name = webServer->arg("name");
  String dirsRaw = webServer->arg("dirs");
  String outputPath = "/playlists/" + name + ".txt";

  // B.1 (plan cache_master_gifs) -- verification PAR DOSSIER (pas globale)
  // de la presence dans le fichier maitre : cocher un dossier jamais mis en
  // cache aux cotes de dossiers deja en cache produisait auparavant une
  // playlist silencieusement incomplete (l'ancien test global -- "le
  // fichier maitre existe-t-il ?" -- prenait le chemin rapide pour TOUT des
  // qu'il existait, sans verifier que chaque dossier demande y etait
  // reellement represente). cachedDirsCsv/uncachedDirsCsv au format
  // ",dir1,dir2,". allDirsClean : tous les dossiers demandes, "dir1,dir2"
  // sans virgule d'encadrement -- marqueur "# FULL:" ecrit tel quel.
  String cachedDirsCsv = ",", uncachedDirsCsv = ",";
  String allDirsClean;
  {
    File master = SD.exists(TOUS_MASTER_PATH) ? SD.open(TOUS_MASTER_PATH, FILE_READ) : File();
    int start = 0;
    while (true) {
      int comma = dirsRaw.indexOf(',', start);
      String d = (comma < 0) ? dirsRaw.substring(start) : dirsRaw.substring(start, comma);
      d.trim();
      if (d.length() > 0) {
        if (allDirsClean.length() > 0) allDirsClean += ",";
        allDirsClean += d;
        bool inMaster = false;
        if (master) { master.seek(0); inMaster = fileContainsNeedle(master, "/gifs/" + d + "/"); }
        if (inMaster) cachedDirsCsv += d + ","; else uncachedDirsCsv += d + ",";
      }
      if (comma < 0) break;
      start = comma + 1;
    }
    if (master) master.close();
  }

  // Chemin RAPIDE : tous les dossiers demandes sont deja couverts par le
  // fichier maitre -- filtrage texte synchrone (tourne dans loop(), pas de
  // tache), ne touche jamais /gifs/.
  if (uncachedDirsCsv.length() <= 1) {
    int linesWritten = 0;
    bool hadWriteLoss = false;
    String err;
    bool ok = filterPlaylistFromMaster(cachedDirsCsv, outputPath, allDirsClean, linesWritten, hadWriteLoss, err);
    if (ok) {
      String msg = "OK: " + String(linesWritten) + " GIFs (generation rapide)";
      if (hadWriteLoss) msg += " (ATTENTION: ecriture incomplete detectee, regenerez cette playlist pour verifier)";
      Serial.println("[WEB] generate-playlist: " + msg + " -> " + name + ".txt");
      webServer->send(200, "text/plain", msg);
    } else {
      Serial.println("[WEB] generate-playlist: ECHEC filtrage (" + err + ")");
      webServer->send(500, "text/plain", "ERR: " + err);
    }
    return;
  }

  // Chemin hybride/complet (generation en tache de fond, avec progression) :
  // au moins un dossier demande n'est pas encore dans le fichier maitre
  // (soit il n'existe pas du tout -- bootstrap -- soit certains dossiers
  // sont neufs). playlistGenTask() ecrit d'abord la portion cachedDirsCsv
  // (quasi instantanee) puis scanne uniquement uncachedDirsCsv, avant
  // d'adopter automatiquement ces derniers dans le fichier maitre.
  if (!SD.exists("/playlists")) SD.mkdir("/playlists");
  if (SD.exists(outputPath.c_str())) SD.remove(outputPath.c_str());
  File outf = SD.open(outputPath.c_str(), FILE_WRITE);
  if (!outf) { webServer->send(500, "text/plain", "ERR: ecriture impossible"); return; }
  outf.close(); // validation d'ecriture seulement -- playlistGenTask() rouvre le fichier et ecrit le marqueur "# FULL:" en tout premier

  int totalDirsToScan = 0;
  { int cp = 1; while (cp < (int)uncachedDirsCsv.length()) { int cc = uncachedDirsCsv.indexOf(',', cp); if (cc < 0) break; totalDirsToScan++; cp = cc + 1; } }

  if (xSemaphoreTake(plGenStatusMutex, portMAX_DELAY) == pdTRUE) {
    g_plGenStatus = PlaylistGenStatus();
    g_plGenStatus.active = true;
    g_plGenStatus.totalDirs = totalDirsToScan;
    xSemaphoreGive(plGenStatusMutex);
  }

  PlaylistGenRequest *req = new PlaylistGenRequest{ name, cachedDirsCsv, uncachedDirsCsv, allDirsClean };
  Serial.println("[WEB] generate-playlist: creation tache, heap libre=" + String(ESP.getFreeHeap()) + " maxalloc=" + String(ESP.getMaxAllocHeap())); // DIAGNOSTIC TEMPORAIRE
  // 4096 (pas 8192) : confirme en test reel (2026-07-28) que maxalloc peut
  // descendre a ~8180 octets a ce point du fonctionnement normal (boot +
  // navigation web) -- une pile de 8192 echouait de justesse (bloc contigu
  // introuvable), laissant active bloque a true pour toujours avant l'ajout
  // de la verification ci-dessous. 4096 correspond a la taille deja utilisee
  // avec succes par mqttTask() dans ce meme environnement contraint ; marge
  // reelle a confirmer via uxTaskGetStackHighWaterMark() (log en fin de
  // tache, voir playlistGenTask()).
  BaseType_t taskOk = xTaskCreatePinnedToCore(playlistGenTask, "playlistGen", 4096, req, 1, &playlistGenTaskHandle, 0);
  if (taskOk != pdPASS) {
    // xTaskCreatePinnedToCore() peut echouer (heap fragmente -- pile de 8 Ko
    // = un bloc contigu a allouer, deja documente sur ce projet comme
    // difficile a garantir) : SANS cette verification, g_plGenStatus.active
    // restait bloque a true pour toujours (rien ne le repasse a false
    // puisque la tache censee le faire n'a jamais demarre) -- symptome
    // observe en test reel (2026-07-28) : "0" affiche indefiniment, aucune
    // progression. delete req ici pour eviter la fuite (la tache qui aurait
    // du le liberer n'existe pas).
    delete req;
    Serial.println("[WEB] generate-playlist: ECHEC creation tache (heap insuffisant ?)");
    if (xSemaphoreTake(plGenStatusMutex, portMAX_DELAY) == pdTRUE) {
      g_plGenStatus.active = false;
      g_plGenStatus.done = true;
      g_plGenStatus.resultMsg = "ERR: impossible de demarrer la generation (heap insuffisant)";
      xSemaphoreGive(plGenStatusMutex);
    }
    webServer->send(500, "text/plain", "ERR: impossible de demarrer la generation");
    return;
  }
  Serial.println("[WEB] generate-playlist: demarrage " + name + ".txt, dirs=" + dirsRaw);
  webServer->send(200, "text/plain", "STARTED");
}

static void handleWebConfigGeneratePlaylistStatus()
{
  PlaylistGenStatus snap;
  if (xSemaphoreTake(plGenStatusMutex, portMAX_DELAY) == pdTRUE) {
    snap = g_plGenStatus;
    xSemaphoreGive(plGenStatusMutex);
  }
  String json = "{\"active\":" + String(snap.active ? "true" : "false");
  json += ",\"done\":" + String(snap.done ? "true" : "false");
  json += ",\"dir\":\"" + jsonEscape(snap.curDirName) + "\"";
  json += ",\"dirIdx\":" + String(snap.dirIdx);
  json += ",\"totalDirs\":" + String(snap.totalDirs);
  json += ",\"gifs\":" + String(snap.totalGifs);
  json += ",\"curDirGifs\":" + String(snap.curDirGifs);
  json += ",\"result\":\"" + jsonEscape(snap.resultMsg) + "\"}";
  webServer->send(200, "application/json", json);
}

// Arret demande par l'utilisateur (bouton "Arreter") : pose juste le drapeau,
// ne touche plus AUCUN File -- playlistGenTask() est desormais la SEULE
// proprietaire de g_plGenOutFile/du dossier en cours, elimine par construction
// tout risque de double-fermeture/concurrence sur ces objets (au lieu de le
// gerer par verrouillage). La tache se ferme/nettoie elle-meme a son prochain
// point de controle ; le polling web deja en place detecte la fin via son
// chemin normal (!active), sans changement JS necessaire.
static void handleWebConfigGeneratePlaylistStop()
{
  bool wasActive = false;
  if (xSemaphoreTake(plGenStatusMutex, portMAX_DELAY) == pdTRUE) {
    wasActive = g_plGenStatus.active;
    if (wasActive) g_plGenStatus.stopRequested = true;
    xSemaphoreGive(plGenStatusMutex);
  }
  Serial.println(String("[WEB] generate-playlist-stop: ") + (wasActive ? "arret demande" : "rien a arreter"));
  webServer->send(200, "text/plain", wasActive ? "OK: arret demande" : "OK: rien a arreter");
}


// Renvoie la liste (JSON) des dossiers distincts references par une playlist
// existante -- utilise par la page web pour pre-cocher les cases du dossier
// correspondant quand l'utilisateur choisit de modifier une playlist deja
// generee, plutot que de devoir tout re-cocher a la main.
static void handleWebConfigPlaylistDirs()
{
  if (plGenIsActive()) { webServer->send(200, "application/json", "[]"); return; }
  if (!webServer->hasArg("name")) { webServer->send(400, "text/plain", "ERR: manque nom"); return; }
  String name = webServer->arg("name");
  int dotExt = name.lastIndexOf('.');
  if (dotExt > 0) name = name.substring(0, dotExt); // defensif : accepte "nom" ou "nom.txt"
  File f = SD.open(("/playlists/" + name + ".txt").c_str());
  if (!f) { webServer->send(200, "application/json", "[]"); return; }

  // Lecture par blocs fixes + extraction ligne par ligne -- jamais tout le
  // fichier en une seule String (meme raison que fileContainsNeedle : une
  // grosse playlist comme "tous", ~400 Ko, a montre en test reel des
  // blocages de plusieurs dizaines de secondes avec un simple readString()).
  String json = "[";
  String seen = ",";
  bool first = true;
  const size_t BUFSZ = 512;
  char buf[BUFSZ + 1];
  String pending;
  // reserve() (2026-07-30) : bug reel confirme en test materiel -- sur une
  // grosse playlist (ALL2.txt, ~11000 lignes/18 dossiers), reouverte pour
  // modification, un SEUL dossier se retrouvait precoche au lieu de tous.
  // Meme cause que "wanted" dans filterPlaylistFromMaster() : pending +=
  // buf peut echouer silencieusement sous heap critique a l'un des
  // (potentiellement) centaines de blocs lus -- une seule desynchronisation
  // corrompt le decoupage en lignes pour TOUT le reste du fichier, faisant
  // disparaitre la quasi-totalite des dossiers reconnus d'un coup. pending
  // ne depasse jamais vraiment BUFSZ + une ligne (retaille a son reliquat
  // apres chaque bloc) -- une seule petite reservation en amont evite les
  // N reallocations incrementales, chacune un point de defaillance distinct.
  pending.reserve(BUFSZ + 256);
  while (true) {
    int n = f.read((uint8_t *)buf, BUFSZ);
    if (n <= 0) break;
    buf[n] = 0;
    pending += buf;
    int lineStart = 0;
    while (true) {
      int nl = pending.indexOf('\n', lineStart);
      if (nl < 0) break;
      String line = pending.substring(lineStart, nl);
      line.trim();
      if (line.startsWith("/gifs/")) {
        int s2 = line.indexOf('/', 6);
        if (s2 > 6) {
          String dir = line.substring(6, s2);
          if (seen.indexOf("," + dir + ",") < 0) {
            seen += dir + ",";
            if (!first) json += ",";
            json += "\"" + jsonEscape(dir) + "\"";
            first = false;
          }
        }
      }
      lineStart = nl + 1;
    }
    pending = pending.substring(lineStart); // garde le reste incomplet (ligne a cheval sur 2 blocs) pour le prochain tour
    if ((size_t)n < BUFSZ) break;
  }
  f.close();
  pending.trim();
  if (pending.startsWith("/gifs/")) { // derniere ligne sans retour a la ligne final
    int s2 = pending.indexOf('/', 6);
    if (s2 > 6) {
      String dir = pending.substring(6, s2);
      if (seen.indexOf("," + dir + ",") < 0) {
        if (!first) json += ",";
        json += "\"" + jsonEscape(dir) + "\"";
      }
    }
  }
  json += "]";
  webServer->send(200, "application/json", json);
}

static void handleWebConfigDeletePlaylist()
{
  if (plGenIsActive()) { webServer->send(409, "text/plain", "ERR: generation en cours"); return; }
  if (!webServer->hasArg("name")) { webServer->send(400, "text/plain", "ERR: manque nom"); return; }
  String name = webServer->arg("name");
  String base = name;
  int dot = base.lastIndexOf('.');
  if (dot > 0) base = base.substring(0, dot);
  const char *exts[] = {".txt", ".cache", ".sig", ".idx"};
  int deleted = 0;
  for (int i = 0; i < 4; i++) {
    String path = "/playlists/" + base + exts[i];
    if (SD.exists(path.c_str())) { SD.remove(path.c_str()); deleted++; }
  }
  invalidatePlaylistRefCache();
  String msg = "OK: " + String(deleted) + " fichiers supprimes pour " + name;
  Serial.println("[WEB] " + msg);
  webServer->send(200, "text/plain", msg);
}

// v92 -- addFileToPlaylists() (mise a jour PAR FICHIER, relisait chaque
// playlist ligne par ligne a chaque appel) remplacee par le cache RAM
// g_plRefCache*/handleWebConfigAddToPlaylistsBatch() ci-dessous : c'est le
// fix exact du probleme "mise a jour playlist lente, sans buffer" -- un
// seul passage par LOT d'upload (pas par fichier), lecture bufferisee
// File::readString() au lieu de readStringUntil('\n') ligne par ligne.

// Cache RAM : pour g_plRefCacheFolder, liste (CSV) des playlists .txt qui
// referencent deja ce dossier. Invalide (chaine vide) a la creation ou
// suppression d'une playlist -- voir invalidatePlaylistRefCache().
static String g_plRefCacheFolder = "";
static String g_plRefCachePlaylists = "";

static void invalidatePlaylistRefCache() { g_plRefCacheFolder = ""; g_plRefCachePlaylists = ""; }

// Cherche needle dans f SANS charger tout le fichier en memoire (contrairement
// a f.readString(), qui a montre en test reel (2026-07-28) des blocages de
// 40-44s ET un resultat FAUX -- "found=0" pour une playlist "Tous"/~400 Ko
// qui referencait pourtant bien le dossier -- des qu'un fichier depasse
// quelques centaines de Ko avec un heap deja fragmente (maxalloc mesure a
// ~9-10 Ko a ce moment du boot) : la reallocation progressive d'une String
// Arduino jusqu'a des centaines de Ko dans un tas aussi fragmente est soit
// catastrophiquement lente, soit echoue silencieusement en cours de route.
// Lecture par blocs fixes (BUFSZ), avec chevauchement pour ne pas rater une
// correspondance a cheval sur 2 blocs -- cout memoire constant, quelle que
// soit la taille du fichier.
static bool fileContainsNeedle(File &f, const String &needle)
{
  const size_t BUFSZ = 512;
  size_t nlen = needle.length();
  if (nlen == 0 || nlen > 64) return false; // needle attendu court ("/gifs/<dossier>/")
  char buf[BUFSZ + 64];
  size_t carried = 0;
  while (true) {
    int n = f.read((uint8_t *)(buf + carried), BUFSZ);
    if (n <= 0) break;
    size_t total = carried + (size_t)n;
    if (total >= nlen) {
      for (size_t i = 0; i + nlen <= total; i++) {
        if (memcmp(buf + i, needle.c_str(), nlen) == 0) return true;
      }
    }
    size_t keep = (nlen > 1) ? (nlen - 1) : 0;
    if (keep > total) keep = total;
    if (keep > 0) memmove(buf, buf + (total - keep), keep);
    carried = keep;
    if ((size_t)n < BUFSZ) break; // fin de fichier
  }
  return false;
}

// Meme principe que fileContainsNeedle() ci-dessus, mais pour plusieurs
// chemins candidats en un seul passage streaming sur le fichier (utilise par
// la boucle d'ajout : verifie lesquels des fichiers d'un lot d'upload sont
// deja presents dans une playlist, sans jamais charger tout son contenu en
// memoire). candidates[]/found[] : memes indices, meme taille nCandidates.
static void fileFindExistingPaths(File &f, int nCandidates, const String candidates[], bool found[])
{
  for (int i = 0; i < nCandidates; i++) found[i] = false;
  const size_t BUFSZ = 512;
  char buf[BUFSZ + 300]; // marge pour l'overlap (chemins de fichiers potentiellement longs)
  buf[0] = '\n'; // emule le prefixe "\n"+contenu de l'ancienne version (detecte une correspondance des le tout debut du fichier)
  size_t carried = 1;
  while (true) {
    int n = f.read((uint8_t *)(buf + carried), BUFSZ);
    if (n <= 0) break;
    size_t total = carried + (size_t)n;
    size_t maxOverlap = 0;
    for (int i = 0; i < nCandidates; i++) {
      if (found[i]) continue;
      String withNl = candidates[i] + "\n";
      size_t nlen = withNl.length();
      if (nlen == 0 || nlen > 260) continue;
      if (nlen - 1 > maxOverlap) maxOverlap = nlen - 1;
      if (total >= nlen) {
        const char *needle = withNl.c_str();
        for (size_t p = 0; p + nlen <= total; p++) {
          if (memcmp(buf + p, needle, nlen) == 0) { found[i] = true; break; }
        }
      }
    }
    size_t keep = (maxOverlap > total) ? total : maxOverlap;
    if (keep > 0) memmove(buf, buf + (total - keep), keep);
    carried = keep;
    if ((size_t)n < BUFSZ) break;
  }
}

// Version "lot" : traite plusieurs fichiers du MEME dossier en un seul
// appel. Meme dossier => memes playlists concernees (via g_plRefCache*), et
// surtout chaque playlist candidate n'est lue qu'UNE FOIS (au lieu d'une
// fois par fichier du lot) pour verifier quels fichiers y sont deja
// presents, puis tous les fichiers manquants sont ajoutes en un seul
// SD.open(FILE_APPEND). Appelee par le JS (uploadGif()) une seule fois a la
// fin de tout un lot d'upload.
static void handleWebConfigAddToPlaylistsBatch()
{
  if (plGenIsActive()) { webServer->send(409, "text/plain", "ERR: generation de playlist en cours"); return; }
  unsigned long tFn0 = millis(); // DIAGNOSTIC TEMPORAIRE (68s constates en test reel 2026-07-28) -- a retirer une fois la cause trouvee
  if (!webServer->hasArg("dir") || !webServer->hasArg("files")) { webServer->send(200, "text/plain", "OK:0"); return; }
  String folder = webServer->arg("dir"); folder.trim();
  String filesArg = webServer->arg("files");
  if (folder.length() == 0 || filesArg.length() == 0) { webServer->send(200, "text/plain", "OK:0"); return; }

  if (g_plRefCacheFolder != folder) {
    g_plRefCacheFolder = folder;
    g_plRefCachePlaylists = "";
    // Detection "quelle playlist reference ce dossier" : lecture bufferisee
    // complete (readString(), reutilisee plus bas dans cette meme fonction
    // pour la verification de doublons) + un seul indexOf(), au lieu d'une
    // relecture ligne par ligne (readStringUntil('\n') + delay(1) PAR
    // LIGNE) -- tres lent des qu'une playlist contient beaucoup d'entrees.
    unsigned long tScan0 = millis(); // DIAGNOSTIC TEMPORAIRE
    int plCount = 0;
    String needle = "/gifs/" + folder + "/";
    // Fichier maitre interne (cache_master_gifs.dat, TOUS_MASTER_PATH) --
    // reintroduit ici explicitement PAR NOM : son extension .dat (changee
    // volontairement pour ne plus jamais etre confondu avec une playlist
    // ailleurs, cf listing) le fait sortir du filtre ".txt" ci-dessous, qui
    // l'aurait sinon exclu de ce scan et donc de la mise a jour automatique
    // lors d'un upload.
    String masterBase = String(TOUS_MASTER_PATH);
    masterBase = masterBase.substring(masterBase.lastIndexOf('/') + 1);
    File plDir = SD.open("/playlists");
    if (plDir && plDir.isDirectory()) {
      File entry = plDir.openNextFile();
      while (entry) {
        String name = String(entry.name());
        int slash = name.lastIndexOf('/');
        String base = (slash >= 0) ? name.substring(slash + 1) : name;
        bool isMaster = (base == masterBase);
        if (!entry.isDirectory() && (base.endsWith(".txt") || isMaster)) {
          plCount++;
          unsigned long tEntry0 = millis(); // DIAGNOSTIC TEMPORAIRE
          bool found;
          if (isMaster) {
            // B (plan cache_master_gifs) -- cache_master_gifs.dat est cense
            // contenir TOUT /gifs/ par construction : il doit toujours etre
            // une cible d'ajout, meme pour un dossier flambant neuf qu'il ne
            // referencait pas encore (contrairement a une playlist
            // utilisateur, ou "n'ajouter que si elle reference deja ce
            // dossier" respecte une selection volontaire).
            found = true;
          } else {
            // Marqueur "# FULL:dossier1,dossier2" (plan cache_master_gifs) --
            // playlist "hybride" (dossiers entiers + selection personnalisee
            // de fichiers dans d'autres dossiers, voir outil PC) : sans ce
            // marqueur, fileContainsNeedle() (juste "cette playlist
            // reference-t-elle AU MOINS UNE ligne de ce dossier ?") ajouterait
            // a tort un nouveau fichier a une playlist qui n'a jamais demande
            // la totalite de ce dossier. Ne lit que la premiere ligne (peu
            // couteux) ; playlist "ancien style" sans marqueur -> comportement
            // inchange (fileContainsNeedle() sur tout le fichier).
            String firstLine;
            entry.seek(0);
            char peekBuf[513];
            int pn = entry.read((uint8_t *)peekBuf, sizeof(peekBuf) - 1);
            if (pn > 0) {
              peekBuf[pn] = 0;
              String chunk = String(peekBuf);
              int nl = chunk.indexOf('\n');
              firstLine = (nl >= 0) ? chunk.substring(0, nl) : chunk;
              firstLine.trim();
            }
            if (firstLine.startsWith("# FULL:")) {
              String listCsv = "," + firstLine.substring(7) + ",";
              found = listCsv.indexOf("," + folder + ",") >= 0;
            } else {
              entry.seek(0);
              // entry est deja un handle ouvert sur ce fichier precis (obtenu
              // par iteration via openNextFile(), pas par nom) -- pas besoin
              // de le rouvrir. fileContainsNeedle() lit par blocs fixes (voir
              // plus haut) : evite de charger tout le fichier en memoire,
              // cause reelle des blocages 40-44s mesures en test reel sur
              // "Tous"/"gaming" (l'ancienne hypothese "recherche par nom" a
              // ete infirmee par un test dedie).
              found = fileContainsNeedle(entry, needle);
            }
          }
          Serial.println("[WEB] plscan " + base + " " + String(millis() - tEntry0) + "ms found=" + String(found ? "1" : "0")); // DIAGNOSTIC TEMPORAIRE
          if (found) {
            if (g_plRefCachePlaylists.length() > 0) g_plRefCachePlaylists += ",";
            g_plRefCachePlaylists += base;
          }
        }
        entry.close(); entry = plDir.openNextFile();
        delay(1);
      }
      plDir.close();
    }
    Serial.println("[WEB] add-to-playlists-batch: scan " + String(plCount) + " playlist(s) en " + String(millis() - tScan0) + "ms, maxalloc=" + String(ESP.getMaxAllocHeap())); // DIAGNOSTIC TEMPORAIRE
  }

  unsigned long tAppend0 = millis(); // DIAGNOSTIC TEMPORAIRE
  int totalAppended = 0;

  // Chemins candidats du lot, calcules une seule fois (identiques pour
  // toutes les playlists candidates ci-dessous) -- bornes a MAX_BATCH_FILES,
  // largement au-dessus des lots observes en usage reel (jusqu'a ~15).
  const int MAX_BATCH_FILES = 64;
  String candidates[MAX_BATCH_FILES];
  int nCand = 0;
  {
    int fstart = 0;
    while (fstart <= (int)filesArg.length() && nCand < MAX_BATCH_FILES) {
      int fcomma = filesArg.indexOf(',', fstart);
      String fname = (fcomma < 0) ? filesArg.substring(fstart) : filesArg.substring(fstart, fcomma);
      fname.trim();
      if (fname.length() > 0) candidates[nCand++] = "/gifs/" + folder + "/" + fname;
      if (fcomma < 0) break;
      fstart = fcomma + 1;
    }
  }

  int pstart = 0;
  while (pstart <= (int)g_plRefCachePlaylists.length()) {
    int pcomma = g_plRefCachePlaylists.indexOf(',', pstart);
    String base = (pcomma < 0) ? g_plRefCachePlaylists.substring(pstart) : g_plRefCachePlaylists.substring(pstart, pcomma);
    if (base.length() > 0) {
      String plPath = "/playlists/" + base;
      // fileFindExistingPaths() lit par blocs fixes (voir fileContainsNeedle
      // plus haut) -- evite de charger toute la playlist en memoire
      // (pl.readString() sur une grosse playlist a montre en test reel des
      // blocages de plusieurs dizaines de secondes, meme cause que le scan
      // de detection ci-dessus).
      bool already[MAX_BATCH_FILES];
      File pl = SD.open(plPath.c_str());
      if (pl) { fileFindExistingPaths(pl, nCand, candidates, already); pl.close(); }
      else { for (int i = 0; i < nCand; i++) already[i] = false; }
      String toAppend;
      for (int i = 0; i < nCand; i++) {
        if (!already[i]) { toAppend += candidates[i] + "\n"; totalAppended++; }
      }
      if (toAppend.length() > 0) {
        File plApp = SD.open(plPath.c_str(), FILE_APPEND);
        if (plApp) { plApp.print(toAppend); plApp.close(); }
      }
    }
    if (pcomma < 0) break;
    pstart = pcomma + 1;
  }
  Serial.println("[WEB] add-to-playlists-batch: dir=" + folder + " -> " + String(totalAppended) + " ajout(s), append=" + String(millis() - tAppend0) + "ms, total=" + String(millis() - tFn0) + "ms"); // DIAGNOSTIC TEMPORAIRE : timings ajoutes
  webServer->send(200, "text/plain", "OK:" + String(totalAppended));
}

// Cree /gifs/<dir> si absent, en route dediee (idempotente, appelee par le
// JS AVANT le premier fichier d'un upload). Decouple la creation de dossier
// du chemin critique de l'upload multipart : le workaround (mkdir + creer/
// supprimer un fichier temoin, necessaire pour eviter un attribut lecture
// seule sur certaines cartes SD) restait auparavant dans UPLOAD_FILE_START,
// ou meme un timeout client elargi ne suffisait pas toujours (requete
// concurrente du flux de donnees fichier en cours de reception). En le
// sortant du multipart, cette requete a son propre budget de temps et le
// dossier existe deja quand l'upload demarre vraiment.
static void handleWebConfigCreateFolder()
{
  if (plGenIsActive()) { webServer->send(409, "text/plain", "ERR: generation de playlist en cours"); return; }
  if (!webServer->hasArg("dir")) { webServer->send(400, "text/plain", "ERR: dossier manquant"); return; }
  String dirName = webServer->arg("dir"); dirName.trim();
  if (dirName.length() == 0) { webServer->send(400, "text/plain", "ERR: dossier manquant"); return; }
  String dirPath = "/gifs/" + dirName;
  webServer->client().setTimeout(15000);
  if (SD.exists(dirPath.c_str())) {
    webServer->client().setTimeout(3000);
    webServer->send(200, "text/plain", "OK: existant");
    return;
  }
  unsigned long t0 = millis();
  bool mkOk = SD.mkdir(dirPath.c_str());
  Serial.println("[WEB] create-folder: mkdir " + dirPath + " -> " + (mkOk ? "OK" : "FAIL") + " (" + String(millis() - t0) + "ms)");
  unsigned long t1 = millis();
  File tmp = SD.open(dirPath + "/.tmp", FILE_WRITE);
  if (tmp) { tmp.close(); SD.remove(dirPath + "/.tmp"); }
  Serial.println("[WEB] create-folder: fichier temoin " + dirPath + " -> " + (tmp ? "OK" : "FAIL") + " (" + String(millis() - t1) + "ms)");
  webServer->client().setTimeout(3000);
  bool ok = SD.exists(dirPath.c_str());
  webServer->send(ok ? 200 : 500, "text/plain", ok ? "OK: cree" : "ERR: creation echouee");
}

static void handleWebConfigUpload()
{
  if (uploadFile) { uploadFile.close(); uploadFile = File(); }
  if (uploadSuccess) {
    uploadSuccess = false;
    String msg = "OK: fichier uploade dans /gifs/" + uploadDir;
    Serial.println("[WEB] " + msg);
    webServer->send(200, "text/plain", msg);
  } else {
    // Ne JAMAIS appeler webServer->send() depuis handleWebConfigUploadFile()
    // (callback UPLOAD_FILE_*) : le client est encore en train d'envoyer le
    // corps multipart a ce moment-la, et une reponse prematuree casse la
    // connexion HTTP en cours (vu cote navigateur comme une erreur reseau).
    // Seul ce handler, appele une fois le corps entierement consomme, a le
    // droit d'envoyer une reponse.
    String msg = uploadErrorMsg.length() ? uploadErrorMsg : "ERR: aucun fichier recu";
    uploadErrorMsg = "";
    webServer->send(400, "text/plain", msg);
  }
}

static void handleWebConfigUploadFile()
{
  HTTPUpload &upload = webServer->upload();
  if (upload.status == UPLOAD_FILE_START) {
    if (plGenIsActive()) { uploadErrorMsg = "ERR: generation de playlist en cours"; return; }
    // Timeout client elargi (defaut lib WebServer ~3s) le temps de l'upload :
    // les ecritures SD sous charge peuvent le depasser facilement -> la lib
    // coupe alors la connexion, vu cote navigateur comme ERR_CONNECTION_
    // RESET/TIMED_OUT. Remis a une valeur courte des la fin/l'abandon de
    // l'upload.
    webServer->client().setTimeout(15000);
    uploadSuccess = false;
    uploadErrorMsg = "";
    uploadDir = webServer->arg("dir");
    uploadDir.trim();
    if (uploadDir.length() == 0) {
      uploadErrorMsg = "ERR: dossier cible manquant";
      return;
    }
    String filename = upload.filename;
    { int p = filename.lastIndexOf('/'); if (p >= 0) filename = filename.substring(p + 1); }
    { int p = filename.lastIndexOf('\\'); if (p >= 0) filename = filename.substring(p + 1); }
    if (filename.length() == 0) { uploadErrorMsg = "ERR: nom fichier invalide"; return; }
    String path = "/gifs/" + uploadDir + "/" + filename;
    String dirPath = "/gifs/" + uploadDir;
    if (!SD.exists(dirPath.c_str())) {
      // Le JS appelle /create-folder avant le premier fichier -- ce cas ne
      // devrait normalement plus se produire. Filet de securite minimal
      // (pas de workaround fichier-temoin ici : trop lent pour le chemin
      // critique de l'upload, cf. handleWebConfigCreateFolder()).
      SD.mkdir(dirPath.c_str());
      Serial.println("[WEB] Upload: dossier absent au demarrage, mkdir de secours " + dirPath);
    }
    if (SD.exists(path.c_str())) SD.remove(path.c_str());
    uploadFile = SD.open(path.c_str(), FILE_WRITE);
    if (!uploadFile) {
      // Un dossier tout juste cree peut ne pas etre immediatement pret en
      // ecriture sur certaines cartes SD -- une nouvelle tentative apres un
      // court delai resout ce cas sans risquer de casser la connexion HTTP.
      delay(50);
      uploadFile = SD.open(path.c_str(), FILE_WRITE);
      Serial.println("[WEB] Upload: 2e tentative ouverture " + path + " -> " + (uploadFile ? "OK" : "FAIL"));
    }
    uploadStartMs = millis();
    uploadTotalBytes = 0;
    if (!uploadFile) {
      uploadErrorMsg = "ERR: ecriture SD impossible";
      Serial.println("[WEB] Upload start FAIL: " + path);
      return;
    }
    Serial.println("[WEB] Upload start: " + path);
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile) {
      uploadFile.write(upload.buf, upload.currentSize);
      uploadTotalBytes += upload.currentSize;
      delay(1);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    webServer->client().setTimeout(3000);
    if (uploadFile) {
      uploadFile.close();
      uploadFile = File();
      uploadSuccess = true;
      unsigned long dt = millis() - uploadStartMs;
      Serial.println("[WEB] Upload done: " + String(uploadTotalBytes) + " bytes in " + String(dt) + "ms");
      // Mise a jour des playlists PLUS appelee ici par fichier -- le JS
      // (uploadGif()) appelle /add-to-playlists-batch UNE SEULE FOIS a la
      // fin de tout le lot, avec la liste des fichiers uploades avec succes.
    }
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    webServer->client().setTimeout(3000);
    if (uploadFile) { uploadFile.close(); uploadFile = File(); }
    Serial.println("[WEB] Upload aborted");
  }
}

static void handleWebConfigSave()
{
  // "brightness" n'est plus obligatoire : chaque page (BASIC/NETWORK/CLOCK/
  // MEDIA) n'envoie que SES propres champs a /save -- l'exiger fait echouer
  // la sauvegarde depuis toutes les pages sauf BASIC.
  if (webServer->hasArg("brightness")) {
    int b = webServer->arg("brightness").toInt();
    if (b >= 0 && b <= 100) screenBrightness = map(b, 0, 100, 0, 255);
  }
  if (webServer->hasArg("playlist"))        playlistName = webServer->arg("playlist");
  if (webServer->hasArg("random"))          playlistRandom = webServer->arg("random") == "1";
  if (webServer->hasArg("info"))            showInfo = webServer->arg("info") == "1";
  if (webServer->hasArg("wifi_enabled"))    wifiEnabled = webServer->arg("wifi_enabled") == "1";
  if (webServer->hasArg("wifi_ssid"))       wifiSSID = webServer->arg("wifi_ssid");
  if (webServer->hasArg("wifi_password"))   wifiPassword = webServer->arg("wifi_password");
  if (webServer->hasArg("wifi_static_enabled")) wifiStaticEnabled = webServer->arg("wifi_static_enabled") == "1";
  if (webServer->hasArg("wifi_static_ip"))  wifiStaticIP = webServer->arg("wifi_static_ip");
  if (webServer->hasArg("wifi_gateway"))    wifiGateway = webServer->arg("wifi_gateway");
  if (webServer->hasArg("wifi_subnet"))     wifiSubnet = webServer->arg("wifi_subnet");
  if (webServer->hasArg("wifi_dns1"))       wifiDNS1 = webServer->arg("wifi_dns1");
  if (webServer->hasArg("wifi_dns2"))       wifiDNS2 = webServer->arg("wifi_dns2");
  if (webServer->hasArg("bluetooth_enabled")) bluetoothEnabled = webServer->arg("bluetooth_enabled") == "1";
  if (webServer->hasArg("bluetooth_name"))  bluetoothName = webServer->arg("bluetooth_name");
  if (webServer->hasArg("recalbox_ip"))     recalboxIP = webServer->arg("recalbox_ip");
  if (webServer->hasArg("clock_enabled"))   clockEnabled = webServer->arg("clock_enabled") == "1";
  if (webServer->hasArg("clock_theme"))     clockTheme = webServer->arg("clock_theme").toInt();
  if (webServer->hasArg("clock_neon_color_enabled")) clockNeonCustomColor = webServer->arg("clock_neon_color_enabled") == "1";
  if (webServer->hasArg("clock_neon_color")) {
    String v = webServer->arg("clock_neon_color");
    if (v.startsWith("#") && v.length() == 7) {
      unsigned long cv = strtoul(v.substring(1).c_str(), NULL, 16);
      clockNeonR = (cv >> 16) & 0xFF; clockNeonG = (cv >> 8) & 0xFF; clockNeonB = cv & 0xFF;
    }
  }
  if (webServer->hasArg("clock_interval"))  clockIntervalGifs = webServer->arg("clock_interval").toInt();
  if (webServer->hasArg("clock_interval_min")) clockIntervalMin = webServer->arg("clock_interval_min").toInt();
  if (webServer->hasArg("clock_duration"))  clockDuration = webServer->arg("clock_duration").toInt();
  if (webServer->hasArg("clock_tz"))        clockTimeZone = webServer->arg("clock_tz");

  int b = (screenBrightness * 100 + 127) / 255;

  File f = SD.open("/config.ini", FILE_WRITE);
  if (!f) { webServer->send(500, "text/plain", "ERR: SD write failed"); return; }
  f.println("# Info"); f.println("info=" + String(showInfo ? "1" : "0"));
  f.println(); f.println("# Affichage"); f.println("brightness=" + String(b));
  f.println(); f.println("# Playlist"); f.println("playlist=" + playlistName); f.println("random=" + String(playlistRandom ? "1" : "0"));
  f.println(); f.println("# Wi-Fi & Bluetooth");
  f.println("wifi_enabled=" + String(wifiEnabled ? "1" : "0")); f.println("wifi_ssid=" + wifiSSID); f.println("wifi_password=" + wifiPassword);
  f.println("bluetooth_enabled=" + String(bluetoothEnabled ? "1" : "0")); f.println("bluetooth_name=" + bluetoothName);
  f.println(); f.println("wifi_static_enabled=" + String(wifiStaticEnabled ? "1" : "0")); f.println("wifi_static_ip=" + wifiStaticIP);
  f.println("wifi_gateway=" + wifiGateway); f.println("wifi_subnet=" + wifiSubnet);
  f.println("wifi_dns1=" + wifiDNS1); f.println("wifi_dns2=" + wifiDNS2);
  f.println(); f.println("# MQTT"); f.println("recalbox_ip=" + recalboxIP);
  f.println(); f.println("# Clock (horloge retro themes)");
  f.println("[CLOCK]"); f.println("CLOCK_ENABLED=" + String(clockEnabled ? "1" : "0"));
  f.println("CLOCK_THEME=" + String(clockTheme)); f.println("CLOCK_INTERVAL=" + String(clockIntervalGifs));
  f.println("CLOCK_INTERVAL_MIN=" + String(clockIntervalMin)); f.println("CLOCK_DURATION=" + String(clockDuration));
  if (clockNeonCustomColor) {
    char neonColorBuf[8];
    snprintf(neonColorBuf, sizeof(neonColorBuf), "#%02X%02X%02X", clockNeonR, clockNeonG, clockNeonB);
    f.println("CLOCK_COLOR=" + String(neonColorBuf));
  } else {
    f.println("CLOCK_COLOR=");
  }
  f.println("TZ=" + clockTimeZone);
  f.println(); f.println("first_boot=0");
  f.close();
  Serial.println("[WEB] config.ini saved (brightness=" + String(b) + "%)");
  webServer->send(200, "text/plain", "OK");
}

static bool forceDeleteFile(const String &path)
{
  if (SD.remove(path.c_str())) return true;
  // Lecture seule FAT32 : rename fonctionne (f_rename ignore AM_RDO),
  // puis on supprime le fichier renomme
  String tmpPath = path + ".del";
  int tries = 0;
  while (SD.exists(tmpPath.c_str()) && tries < 20) { tmpPath += "_"; tries++; }
  if (!SD.exists(tmpPath.c_str()) && SD.rename(path.c_str(), tmpPath.c_str())) {
    bool ok = SD.remove(tmpPath.c_str());
    if (ok) return true;
    SD.rename(tmpPath.c_str(), path.c_str()); // restaurer si echec
  }
  Serial.println("[WEB] forceDeleteFile FAIL: " + path);
  return false;
}

static bool deleteFolderRecursive(const String &path)
{
  File dir = SD.open(path.c_str());
  if (!dir) { Serial.println("[WEB] deleteFolder: impossible d'ouvrir " + path); return false; }
  if (!dir.isDirectory()) { dir.close(); bool ok = forceDeleteFile(path); Serial.println("[WEB] deleteFile: " + path + " -> " + (ok?"OK":"FAIL")); return ok; }
  bool allOk = true;
  File f = dir.openNextFile();
  while (f) {
    String fn = String(f.name());
    String fullPath = path + "/" + fn;
    if (f.isDirectory()) {
      f.close();
      if (fn == "." || fn == "..") { f = dir.openNextFile(); delay(1); continue; }
      if (!deleteFolderRecursive(fullPath)) allOk = false;
    } else {
      f.close();
      if (!forceDeleteFile(fullPath)) allOk = false;
    }
    f = dir.openNextFile();
    delay(1);
  }
  dir.close();
  bool ok = SD.rmdir(path.c_str());
  if (!ok) {
    // FAT32 lecture seule : rename le dossier puis rmdir le renomme
    String tmpPath = path + ".del";
    int tries = 0;
    while (SD.exists(tmpPath.c_str()) && tries < 20) { tmpPath += "_"; tries++; }
    if (!SD.exists(tmpPath.c_str()) && SD.rename(path.c_str(), tmpPath.c_str())) {
      if (SD.rmdir(tmpPath.c_str())) {
        ok = true;
      } else {
        SD.rename(tmpPath.c_str(), path.c_str()); // restaurer
      }
    }
    if (!ok) {
      Serial.println("[WEB] rmdir FAIL (readonly?) : " + path);
      allOk = false;
    }
  }
  return allOk;
}

// A.1 (plan cache_master_gifs, portee ici depuis master 2026-08-02 pour
// tester Parties A/B/C ensemble) -- Nettoie une playlist des lignes qui
// referencent un dossier venant d'etre supprime. Sans cela, rien ne met a
// jour les playlists existantes quand un dossier qu'elles referencent
// disparait : openNextGif() (RecalBox_DMD.ino) n'a aucune tolerance aux
// fichiers manquants -- ecran noir fige a cet index. deletedNamesCsv au
// format ",nom1,nom2," (test d'appartenance par indexOf("," + dir + ",")).
// Lecture par blocs fixes de 512 octets avec report de ligne incomplete
// (meme algorithme que handleWebConfigPlaylistDirs()) -- jamais
// f.readString() (blocage 40-44s mesure sur une grosse playlist en test
// reel). Fichier temporaire + echange atomique (forceDeleteFile() +
// SD.rename(), jamais de rename par-dessus un fichier existant). Retourne
// false si aucune ligne n'a ete retiree (rien a faire).
static bool stripDeletedFoldersFromPlaylist(const String &plBaseName, const String &deletedNamesCsv, int &linesRemovedOut)
{
  linesRemovedOut = 0;
  String path = "/playlists/" + plBaseName + ".txt";
  File f = SD.open(path.c_str());
  if (!f) return false;

  String tmpPath = path + ".new";
  int tries = 0;
  while (SD.exists(tmpPath.c_str()) && tries < 20) { tmpPath += "_"; tries++; }
  File out = SD.open(tmpPath.c_str(), FILE_WRITE);
  if (!out) {
    f.close();
    Serial.println("[WEB] stripDeletedFoldersFromPlaylist: impossible de creer " + tmpPath);
    return false;
  }

  const size_t BUFSZ = 512;
  char buf[BUFSZ + 1];
  String pending; pending.reserve(BUFSZ + 300);
  String outBuf; outBuf.reserve(1200);
  int removed = 0;

  while (true) {
    int n = f.read((uint8_t *)buf, BUFSZ);
    if (n <= 0) break;
    buf[n] = 0;
    pending += buf;
    int lineStart = 0;
    while (true) {
      int nl = pending.indexOf('\n', lineStart);
      if (nl < 0) break;
      String line = pending.substring(lineStart, nl);
      String trimmed = line; trimmed.trim();
      bool drop = false;
      if (trimmed.startsWith("/gifs/")) {
        int s2 = trimmed.indexOf('/', 6);
        if (s2 > 6) {
          String dirName = trimmed.substring(6, s2);
          if (deletedNamesCsv.indexOf("," + dirName + ",") >= 0) drop = true;
        }
      }
      if (drop) removed++;
      else { outBuf += line; outBuf += "\n"; }
      lineStart = nl + 1;
    }
    pending = pending.substring(lineStart); // garde le reste incomplet pour le prochain tour
    if (outBuf.length() > 1000) { writeBufChecked(out, outBuf); outBuf = ""; }
    if ((size_t)n < BUFSZ) break;
  }
  pending.trim();
  if (pending.length() > 0) { // derniere ligne sans retour a la ligne final
    bool drop = false;
    if (pending.startsWith("/gifs/")) {
      int s2 = pending.indexOf('/', 6);
      if (s2 > 6) {
        String dirName = pending.substring(6, s2);
        if (deletedNamesCsv.indexOf("," + dirName + ",") >= 0) drop = true;
      }
    }
    if (drop) removed++;
    else { outBuf += pending; outBuf += "\n"; }
  }
  if (outBuf.length() > 0) writeBufChecked(out, outBuf);
  f.close();
  out.close();

  if (removed == 0) {
    forceDeleteFile(tmpPath); // rien a faire, jeter le brouillon
    return false;
  }

  if (!forceDeleteFile(path) || !SD.rename(tmpPath.c_str(), path.c_str())) {
    Serial.println("[WEB] stripDeletedFoldersFromPlaylist: echec remplacement " + path);
    forceDeleteFile(tmpPath);
    return false;
  }

  const char *companionExts[] = {".cache", ".sig", ".idx"};
  for (int i = 0; i < 3; i++) {
    String companion = "/playlists/" + plBaseName + companionExts[i];
    if (SD.exists(companion.c_str())) SD.remove(companion.c_str());
  }
  invalidatePlaylistRefCache();
  linesRemovedOut = removed;
  Serial.println("[WEB] stripDeletedFoldersFromPlaylist: " + plBaseName + ".txt -- " + String(removed) + " ligne(s) retiree(s)");
  return true;
}

static void handleWebConfigDeleteFolders()
{
  if (plGenIsActive()) { webServer->send(409, "text/plain", "ERR: generation de playlist en cours"); return; }
  if (!webServer->hasArg("dirs")) { webServer->send(400, "text/plain", "ERR: missing dirs"); return; }
  String dirs = webServer->arg("dirs");
  int count = 0, fail = 0, start = 0;
  // A.2 (plan cache_master_gifs) -- accumule uniquement les dossiers
  // REELLEMENT supprimes (deleteFolderRecursive() == true), au format
  // ",nom1,nom2," attendu par stripDeletedFoldersFromPlaylist().
  String deletedNamesCsv = ",";
  while (true) {
    int comma = dirs.indexOf(',', start);
    String d = (comma < 0) ? dirs.substring(start) : dirs.substring(start, comma);
    d.trim();
    if (d.length() > 0) {
      String path = "/gifs/" + d;
      if (SD.exists(path.c_str())) {
        Serial.println("[WEB] deleteFolder start: " + path);
        if (deleteFolderRecursive(path)) { count++; deletedNamesCsv += d + ","; Serial.println("[WEB] deleteFolder OK: " + path); }
        else { fail++; Serial.println("[WEB] deleteFolder FAIL: " + path); }
      } else {
        Serial.println("[WEB] deleteFolder introuvable: " + path);
      }
    }
    if (comma < 0) break;
    start = comma + 1;
  }

  // A.2 -- nettoie toutes les playlists existantes des lignes qui
  // referencaient un des dossiers effectivement supprimes ci-dessus.
  // cache_master_gifs.dat (TOUS_MASTER_PATH) est intentionnellement exclu :
  // il n'est jamais lu playlist par playlist pendant la lecture DMD, son
  // eventuel contenu perime pour ce dossier sera simplement ignore/reecrit
  // a la prochaine generation qui le concerne.
  int plModified = 0, totalLinesRemoved = 0;
  if (deletedNamesCsv.length() > 1) {
    String masterBase = String(TOUS_MASTER_PATH);
    masterBase = masterBase.substring(masterBase.lastIndexOf('/') + 1);
    File plDir = SD.open("/playlists");
    if (plDir && plDir.isDirectory()) {
      File entry = plDir.openNextFile();
      while (entry) {
        String name = String(entry.name());
        bool isDirEntry = entry.isDirectory();
        entry.close();
        int slash = name.lastIndexOf('/');
        String base = (slash >= 0) ? name.substring(slash + 1) : name;
        if (!isDirEntry && base.endsWith(".txt") && base != masterBase) {
          String plBaseName = base.substring(0, base.length() - 4);
          int linesRemoved = 0;
          if (stripDeletedFoldersFromPlaylist(plBaseName, deletedNamesCsv, linesRemoved)) {
            plModified++;
            totalLinesRemoved += linesRemoved;
            Serial.println("[WEB] playlist mise a jour: " + plBaseName + ".txt (" + String(linesRemoved) + " ligne(s) retiree(s))");
          }
        }
        entry = plDir.openNextFile();
        delay(1);
      }
      plDir.close();
    }
  }

  String msg = "OK: " + String(count) + " supprime(s)" + (fail>0?", " + String(fail) + " echec(s)":"");
  if (plModified > 0) msg += ", " + String(plModified) + " playlist(s) mise(s) a jour (" + String(totalLinesRemoved) + " ligne(s) retiree(s)), redemarrage necessaire";
  webServer->send(200, "text/plain", msg);
}

// v92 -- handleWebConfigDeleteFiles() (suppression de fichiers individuels
// dans un dossier) retiree : plus aucune page ne l'appelle -- voir plan de
// reconstruction.

// v92 -- handleWebConfigAddToPlaylists() (mise a jour PAR FICHIER, route
// /add-to-playlists singulier) retiree : remplacee par
// handleWebConfigAddToPlaylistsBatch()//add-to-playlists-batch (cache
// g_plRefCache* + lecture bufferisee, voir plus haut).

// Forward declaration : definie plus bas (juste avant handleWebConfigRoot,
// qui l'utilise aussi), mais appelee ici par handleDmdOpen() -- sans cette
// declaration, erreur de compilation "not declared in this scope".
static void triggerWebConfigMode(const String &msg);

static void handleDmdPause()
{
  if (!webServer->hasArg("msg")) { webServer->send(400, "text/plain", "ERR: missing msg"); return; }
  String msg = webServer->arg("msg");
  String colorStr = webServer->arg("color");
  uint16_t color = 0xFFFF;
  if (colorStr == "1") color = 0x07E0;
  else if (colorStr == "2") color = 0xF800;
  webDmdPause(msg, color);
  webServer->send(200, "text/plain", "OK");
}

static void handleDmdResume()
{
  webServer->send(200, "text/plain", "OK REBOOT");
  delay(100);
  webDmdResume();
}

static void handleDmdOpen()
{
  if (!webServer->hasArg("msg")) { webServer->send(400, "text/plain", "ERR: missing msg"); return; }
  String msg = webServer->arg("msg");
  String full = msg + " " + WiFi.localIP().toString();
  triggerWebConfigMode(msg);
  webServer->send(200, "text/plain", "OK " + full);
}

static void handleWebConfigReboot() { webServer->send(200, "text/plain", "REBOOT"); delay(500); ESP.restart(); }

static void handleWebConfigScanWiFi()
{
  int n = WiFi.scanComplete();
  if (n == WIFI_SCAN_FAILED) { WiFi.scanNetworks(true); webServer->send(200, "application/json", "[]"); return; }
  if (n == WIFI_SCAN_RUNNING) { webServer->send(200, "application/json", "[]"); return; }
  String json = "[";
  for (int i = 0; i < n; i++) {
    if (i > 0) json += ",";
    String ssid = WiFi.SSID(i);
    ssid.replace("\"", "\\\"");
    json += "\"" + ssid + "\"";
  }
  json += "]";
  WiFi.scanDelete();
  webServer->send(200, "application/json", json);
}

static void handleWebConfigLang()
{
  webServer->send(200, "application/json", "{\"language\":\"" + uiLanguage + "\"}");
}

static void handleWebConfigSaveLanguage()
{
  if (!webServer->hasArg("language")) { webServer->send(400, "text/plain", "ERR: missing language"); return; }
  String lang = webServer->arg("language");
  if (lang != "fr" && lang != "en" && lang != "es") { webServer->send(400, "text/plain", "ERR: invalid language"); return; }
  writeConfigFlag("language", lang);
  uiLanguage = lang;
  webServer->send(200, "text/plain", "OK");
}

static void handleWebConfigSaveAP()
{
  if (!webServer->hasArg("wifi_ssid")) { webServer->send(400, "text/plain", "ERR: missing SSID"); return; }
  wifiEnabled = true;
  wifiSSID = webServer->arg("wifi_ssid"); wifiSSID.trim();
  wifiPassword = webServer->arg("wifi_password"); wifiPassword.trim();
  bool hasStatic = webServer->hasArg("wifi_static_ip");
  if (hasStatic) { String sip = webServer->arg("wifi_static_ip"); sip.trim(); hasStatic = (sip.length() > 0); }
  if (hasStatic) {
    wifiStaticEnabled = true;
    wifiStaticIP = webServer->arg("wifi_static_ip"); wifiStaticIP.trim();
  } else {
    wifiStaticEnabled = false;
    wifiStaticIP = "";
  }
  // Ecrire config.ini -- patch cle par cle (writeConfigFlag), JAMAIS un
  // SD.remove()+rewrite complet : ecraserait silencieusement toutes les
  // autres cles deja presentes (recalbox_ip, playlist, brightness, clock_*,
  // language...). Meme bug/fix que le 2026-07-21 (voir memoire projet),
  // reintroduit par la refonte multi-pages -- confirme responsable de la
  // disparition de recalbox_ip signalee par l'utilisateur.
  Serial.println("[WEB] AP save: ecriture config.ini (SSID=" + wifiSSID + ")");
  writeConfigFlag("wifi_enabled", "1");
  writeConfigFlag("wifi_ssid", wifiSSID);
  writeConfigFlag("wifi_password", wifiPassword);
  writeConfigFlag("wifi_static_enabled", wifiStaticEnabled ? "1" : "0");
  writeConfigFlag("wifi_static_ip", wifiStaticIP);
  writeConfigFlag("first_boot", "0");
  Serial.println("[WEB] AP save: fichier ecrit avec SSID=" + wifiSSID + " -> reboot");
  webServer->send(200, "text/plain", "OK");
  delay(1000);
  ESP.restart();
}

static void triggerWebConfigMode(const String &msg)
{
  // "http://" explicite (2026-07-30, demande utilisateur) : certains
  // navigateurs (Firefox "HTTPS-First", Edge) tentent une connexion HTTPS
  // avant HTTP des qu'une adresse est tapee SANS schema, avec un delai
  // d'attente de plusieurs dizaines de secondes avant le repli sur HTTP (le
  // DMD n'ecoute qu'en HTTP, aucun moyen cote firmware d'empecher cette
  // tentative HTTPS qui se joue entierement avant l'envoi de la moindre
  // requete). En affichant l'URL complete avec schema, un utilisateur qui
  // COPIE/RETAPE exactement ce qui est affiche evite le declenchement de ce
  // mecanisme, sans reglage navigateur particulier.
  String url = "http://" + WiFi.localIP().toString();
  clearFirstBoot();
  webDmdSetMainMsg(msg);
  webDmdPause(url, 0xFFE0);
}

static void sendGzipHtml(const uint8_t *content, size_t len)
{
  // Garde-fou heap (2026-07-29, ERR_EMPTY_RESPONSE reel en test materiel) :
  // envoyer une page complete (plusieurs Ko gzip) peut echouer si le heap
  // est deja tres sollicite par un long scan playlistGenTask() en cours --
  // la connexion se fermait alors sans aucune donnee envoyee (vu cote
  // navigateur comme une page vide, ERR_EMPTY_RESPONSE, sur plusieurs
  // navigateurs differents -- pas un souci cote client). Repli sur une
  // reponse texte minimaliste (bien moins gourmande a envoyer, donc bien
  // plus susceptible de reussir meme sous pression heap) plutot que de
  // risquer le meme echec silencieux.
  // Seuil corrige de 8192 a 4096 (2026-07-29, meme erreur que pour le
  // garde-fou du scan) : maxalloc se situe couramment entre 4500 et 9000 en
  // fonctionnement tout a fait normal (meme sans generation active) --
  // 8192 declenchait ce message quasi en permanence, meme entre 2 pages ou
  // sur le simple menu. 4096 correspond a la valeur deja validee sans souci
  // par le garde-fou du scan lui-meme.
  if (ESP.getMaxAllocHeap() < 4096) {
    // Message volontairement generique (2026-07-29) : la cause reelle du
    // heap bas n'est pas forcement une generation de playlist en cours
    // (retour utilisateur : message trompeur affiche hors de tout scan) --
    // ne pas presumer d'une cause precise qui peut etre fausse.
    Serial.println("[WEB] sendGzipHtml: repli memoire faible, maxalloc=" + String(ESP.getMaxAllocHeap()) + " libre=" + String(ESP.getFreeHeap())); // DIAGNOSTIC TEMPORAIRE (2026-07-30) -- lenteur page rapportee hors generation
    webServer->send(200, "text/plain", "Memoire faible, reessayez dans quelques secondes");
    return;
  }
  webServer->sendHeader("Content-Encoding", "gzip");
  webServer->send_P(200, "text/html", reinterpret_cast<PGM_P>(content), len);
}

static void handleWebConfigRoot()
{
  triggerWebConfigMode("WEB DMD CONFIG");
  if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
    sendGzipHtml(WEB_CONFIG_AP_HTML_GZ, WEB_CONFIG_AP_HTML_GZ_LEN);
  } else {
    sendGzipHtml(WEB_CONFIG_MENU_HTML_GZ, WEB_CONFIG_MENU_HTML_GZ_LEN);
  }
}

static void handleWebConfigBasicPage()
{
  triggerWebConfigMode("WEB DMD CONFIG");
  sendGzipHtml(WEB_CONFIG_BASIC_HTML_GZ, WEB_CONFIG_BASIC_HTML_GZ_LEN);
}

static void handleWebConfigNetworkPage()
{
  triggerWebConfigMode("WEB DMD CONFIG");
  sendGzipHtml(WEB_CONFIG_NETWORK_HTML_GZ, WEB_CONFIG_NETWORK_HTML_GZ_LEN);
}

static void handleWebConfigClockPage()
{
  triggerWebConfigMode("WEB DMD CONFIG");
  sendGzipHtml(WEB_CONFIG_CLOCK_HTML_GZ, WEB_CONFIG_CLOCK_HTML_GZ_LEN);
}

static void handleWebConfigMediaPage()
{
  triggerWebConfigMode("WEB DMD CONFIG");
  sendGzipHtml(WEB_CONFIG_MEDIA_HTML_GZ, WEB_CONFIG_MEDIA_HTML_GZ_LEN);
}

void setupWebConfig()
{
  if (webServer) delete webServer;
  webServer = new WebServer(80);
  webServer->on("/", handleWebConfigRoot);
  webServer->on("/config/basic", handleWebConfigBasicPage);
  webServer->on("/config/network", handleWebConfigNetworkPage);
  webServer->on("/config/clock", handleWebConfigClockPage);
  webServer->on("/config/media", handleWebConfigMediaPage);
  webServer->on("/load", handleWebConfigLoad);
  webServer->on("/lsplaylists", handleWebConfigListPlaylists);
  webServer->on("/lsgifdirs", handleWebConfigListGifDirs);
  webServer->on("/generate-playlist", HTTP_POST, handleWebConfigGeneratePlaylist);
  webServer->on("/generate-playlist-status", handleWebConfigGeneratePlaylistStatus);
  webServer->on("/generate-playlist-stop", HTTP_POST, handleWebConfigGeneratePlaylistStop);
  webServer->on("/playlist-dirs", handleWebConfigPlaylistDirs);
  webServer->on("/delete-playlist", HTTP_POST, handleWebConfigDeletePlaylist);
  webServer->on("/upload", HTTP_POST, handleWebConfigUpload, handleWebConfigUploadFile);
  webServer->on("/create-folder", HTTP_POST, handleWebConfigCreateFolder);
  webServer->on("/delete-folders", HTTP_POST, handleWebConfigDeleteFolders);
  webServer->on("/scan-wifi", handleWebConfigScanWiFi);
  webServer->on("/save-ap", HTTP_POST, handleWebConfigSaveAP);
  // Firefox/Edge demandent systematiquement /favicon.ico au chargement de
  // toute page (Chrome aussi, mais semble plus tolerant) -- sans route
  // dediee, cette requete tombe sur le 404 par defaut de la lib WebServer,
  // point d'incertitude ecarte ici a peu de frais (2026-07-30, lenteur page
  // rapportee, plus marquee sur Firefox/Edge que Chrome).
  webServer->on("/favicon.ico", []() { webServer->send(204); });
  webServer->on("/lang", handleWebConfigLang);
  webServer->on("/save-language", HTTP_POST, handleWebConfigSaveLanguage);
  webServer->on("/add-to-playlists-batch", HTTP_POST, handleWebConfigAddToPlaylistsBatch);
  webServer->on("/dmd-pause", HTTP_POST, handleDmdPause);
  webServer->on("/dmd-resume", HTTP_POST, handleDmdResume);
  webServer->on("/dmd-open", HTTP_POST, handleDmdOpen);
  webServer->on("/save", HTTP_POST, handleWebConfigSave);
  webServer->on("/reboot", handleWebConfigReboot);
  webServer->begin();
  Serial.println("[WEB] Interface config sur http://" + WiFi.localIP().toString());
}

void handleWebConfig() { if (webServer) webServer->handleClient(); }

#endif
