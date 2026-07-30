// ============================================
// web_config.h — Interface web de configuration
//
// safe-modify — Historique des modifications
// ============================================
// Version actuelle : v31
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
<div class="desc" style="margin-top:12px;border-top:1px solid #333;padding-top:12px" data-i18n="desc_resync_tous">Met &agrave; jour l'index interne utilis&eacute; pour g&eacute;n&eacute;rer les playlists rapidement, en ne r&eacute;analysant que les dossiers modifi&eacute;s depuis la derni&egrave;re fois (utile apr&egrave;s un ajout/retrait de fichiers directement sur la carte SD).</div>
<div class="btn-row"><button type="button" class="btn btn-gen" onclick="resyncTous()" data-i18n="btn_resync_tous">&#x1F504; Resynchroniser l'index GIFs</button></div>
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
fr:{title:'RecalBox DMD - Affichage',h1:'Affichage &amp; Playlists',nav_basic:'&#x1F4A1; Affichage &amp; Playlists',nav_network:'&#x1F4F6; Wi-Fi &amp; BT',nav_clock:'&#x23F0; Horloge',nav_media:'&#x1F4BF; Médias',sec_display:'&#x1F4A1; Affichage',sec_playlist:'&#x1F4BF; Playlist',lbl_brightness:'Luminosité (%)',lbl_silent_boot:'Démarrage silencieux',lbl_playlist_file:'Playlist par défaut',lbl_random:'Lecture aléatoire',lbl_delete_playlist:'Supprimer',btn_delete_playlist:'&#x1F5D1; Supprimer playlist',btn_save:'&#x1F4BE; Enregistrer',btn_save_reboot:'&#x1F504; Enreg. &amp; Redémarrer',btn_reboot:'&#x1F504; Redémarrer',btn_resume:'&#x25B6; Reprendre DMD',msg_saving:'Enregistrement...',msg_net_error:'Erreur réseau',msg_confirm_unsaved:'Des modifications non enregistrées seront perdues. Continuer ?',msg_confirm_reboot:'Redémarrer l\'ESP32 ?',msg_rebooting:'Redémarrage...',msg_dmd_resumed:'DMD repris',msg_select_playlist:'Sélectionnez une playlist à supprimer',msg_confirm_delete:'Supprimer ${0} ?',msg_confirm_delete_default:'ATTENTION : ${0} est actuellement la playlist par defaut ! La supprimer peut empecher le DMD de demarrer normalement. Continuer ?',msg_deleting:'Suppression...',msg_load_error:'Impossible de charger la config',sec_manage_playlists:'&#x2699; Gestion des playlists',desc_gen_playlist:'Cochez des dossiers pour générer une nouvelle playlist. &#x26A0;&#xFE0F; La création n\'est performante que sur des dossiers avec un nombre limité de fichiers. Pour des playlists contenant des dossiers conséquents, passez par l\'utilitaire RecalboxDMD_tool sur PC.',btn_select_all:'Tout sélectionner',btn_select_none:'Rien sélectionner',lbl_playlist_name:'Nom playlist',placeholder_playlist_name:'ex: MaPlaylist',btn_gen_playlist:'&#x2699; Générer playlist',msg_no_playlist_name:'Donnez un nom à la playlist',msg_select_folder:'Choisissez au moins un dossier',msg_generating:'Generation...',lbl_load_playlist:'Modifier une playlist existante',msg_scanning:'Analyse',msg_gen_busy:'Generation deja en cours ailleurs',msg_gen_start_error:'Impossible de demarrer la generation',msg_gen_leave_warning:'Une generation de playlist est en cours. Quitter la page ?',btn_stop_gen:'&#x23F9; Arreter',msg_confirm_stop_gen:'Arreter la generation ? La playlist en cours de creation sera supprimee.',msg_stopping_gen:'Arret playlist en cours, veuillez patienter...',msg_stop_gen_failed:'Echec de la demande d\'arret (reseau) -- reessayez',desc_resync_tous:'Met à jour l\'index interne utilisé pour générer les playlists rapidement, en ne réanalysant que les dossiers modifiés depuis la dernière fois (utile après un ajout/retrait de fichiers directement sur la carte SD).',btn_resync_tous:'&#x1F504; Resynchroniser l\'index GIFs',msg_resync_starting:'Resynchronisation...',msg_resync_progress:'Analyse: ${0} (${1} dossier(s) modifié(s))'},
en:{title:'RecalBox DMD - Display',h1:'Display &amp; Playlists',nav_basic:'&#x1F4A1; Display &amp; Playlists',nav_network:'&#x1F4F6; Wi-Fi &amp; BT',nav_clock:'&#x23F0; Clock',nav_media:'&#x1F4BF; Media',sec_display:'&#x1F4A1; Display',sec_playlist:'&#x1F4BF; Playlist',lbl_brightness:'Brightness (%)',lbl_silent_boot:'Silent boot',lbl_playlist_file:'Default playlist',lbl_random:'Random playback',lbl_delete_playlist:'Delete',btn_delete_playlist:'&#x1F5D1; Delete playlist',btn_save:'&#x1F4BE; Save',btn_save_reboot:'&#x1F504; Save &amp; Reboot',btn_reboot:'&#x1F504; Reboot',btn_resume:'&#x25B6; Resume DMD',msg_saving:'Saving...',msg_net_error:'Network error',msg_confirm_unsaved:'Unsaved changes will be lost. Continue?',msg_confirm_reboot:'Reboot the ESP32?',msg_rebooting:'Rebooting...',msg_dmd_resumed:'DMD resumed',msg_select_playlist:'Select a playlist to delete',msg_confirm_delete:'Delete ${0}?',msg_confirm_delete_default:'WARNING: ${0} is currently the default playlist! Deleting it may prevent the DMD from starting normally. Continue?',msg_deleting:'Deleting...',msg_load_error:'Unable to load config',sec_manage_playlists:'&#x2699; Playlist management',desc_gen_playlist:'Check folders to generate a new playlist. &#x26A0;&#xFE0F; Generation is only fast on folders with a limited number of files. For playlists covering large folders, use the RecalboxDMD_tool utility on PC instead.',btn_select_all:'Select all',btn_select_none:'Select none',lbl_playlist_name:'Playlist name',placeholder_playlist_name:'e.g. MyPlaylist',btn_gen_playlist:'&#x2699; Generate playlist',msg_no_playlist_name:'Please name the playlist',msg_select_folder:'Select at least one folder',msg_generating:'Generating...',lbl_load_playlist:'Edit an existing playlist',msg_scanning:'Scanning',msg_gen_busy:'A generation is already running',msg_gen_start_error:'Could not start generation',msg_gen_leave_warning:'A playlist generation is in progress. Leave the page?',btn_stop_gen:'&#x23F9; Stop',msg_confirm_stop_gen:'Stop generation? The playlist being created will be deleted.',msg_stopping_gen:'Stopping playlist generation, please wait...',msg_stop_gen_failed:'Stop request failed (network) -- please retry',desc_resync_tous:'Updates the internal index used to generate playlists quickly, by only rescanning folders that changed since last time (useful after adding/removing files directly on the SD card).',btn_resync_tous:'&#x1F504; Resync GIF index',msg_resync_starting:'Resyncing...',msg_resync_progress:'Scanning: ${0} (${1} folder(s) changed)'},
es:{title:'RecalBox DMD - Pantalla',h1:'Pantalla y listas',nav_basic:'&#x1F4A1; Pantalla y listas',nav_network:'&#x1F4F6; Wi-Fi y BT',nav_clock:'&#x23F0; Reloj',nav_media:'&#x1F4BF; Medios',sec_display:'&#x1F4A1; Pantalla',sec_playlist:'&#x1F4BF; Lista',lbl_brightness:'Brillo (%)',lbl_silent_boot:'Arranque silencioso',lbl_playlist_file:'Lista predeterminada',lbl_random:'Reproducción aleatoria',lbl_delete_playlist:'Eliminar',btn_delete_playlist:'&#x1F5D1; Eliminar lista',btn_save:'&#x1F4BE; Guardar',btn_save_reboot:'&#x1F504; Guardar y reiniciar',btn_reboot:'&#x1F504; Reiniciar',btn_resume:'&#x25B6; Reanudar DMD',msg_saving:'Guardando...',msg_net_error:'Error de red',msg_confirm_unsaved:'Los cambios no guardados se perderán. ¿Continuar?',msg_confirm_reboot:'¿Reiniciar el ESP32?',msg_rebooting:'Reiniciando...',msg_dmd_resumed:'DMD reanudado',msg_select_playlist:'Selecciona una lista para eliminar',msg_confirm_delete:'¿Eliminar ${0}?',msg_confirm_delete_default:'ATENCIÓN: ¡${0} es actualmente la lista predeterminada! Eliminarla puede impedir que el DMD arranque normalmente. ¿Continuar?',msg_deleting:'Eliminando...',msg_load_error:'No se pudo cargar la configuración',sec_manage_playlists:'&#x2699; Gestión de listas',desc_gen_playlist:'Marque las carpetas para generar una nueva lista. &#x26A0;&#xFE0F; La creación solo es rápida en carpetas con un número limitado de archivos. Para listas con carpetas voluminosas, use la utilidad RecalboxDMD_tool en el PC.',btn_select_all:'Seleccionar todo',btn_select_none:'Deseleccionar todo',lbl_playlist_name:'Nombre de la lista',placeholder_playlist_name:'ej: MiLista',btn_gen_playlist:'&#x2699; Generar lista',msg_no_playlist_name:'Póngale un nombre a la lista',msg_select_folder:'Elija al menos una carpeta',msg_generating:'Generando...',lbl_load_playlist:'Editar una lista existente',msg_scanning:'Analizando',msg_gen_busy:'Ya hay una generación en curso',msg_gen_start_error:'No se pudo iniciar la generación',msg_gen_leave_warning:'Hay una generación de lista en curso. ¿Salir de la página?',btn_stop_gen:'&#x23F9; Detener',msg_confirm_stop_gen:'¿Detener la generación? La lista en creación se eliminará.',msg_stopping_gen:'Deteniendo la generación de la lista, espere...',desc_resync_tous:'Actualiza el índice interno usado para generar listas rápidamente, reanalizando solo las carpetas que cambiaron desde la última vez (útil tras añadir/quitar archivos directamente en la tarjeta SD).',btn_resync_tous:'&#x1F504; Resincronizar índice GIFs',msg_resync_starting:'Resincronizando...',msg_resync_progress:'Analizando: ${0} (${1} carpeta(s) modificada(s))'}
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
function fillPlaylists(selVal){fetch('/lsplaylists').then(r=>r.json()).then(pl=>{const sel=document.getElementById('playlist');const del=document.getElementById('deletePlaylistSelect');const load=document.getElementById('loadPlaylistSelect');sel.innerHTML='';del.innerHTML='';load.innerHTML='';const opt=document.createElement('option');opt.value='';opt.textContent='---';sel.appendChild(opt);const opt2=document.createElement('option');opt2.value='';opt2.textContent='---';del.appendChild(opt2);const opt3=document.createElement('option');opt3.value='';opt3.textContent='---';load.appendChild(opt3);pl.forEach(p=>{const o=document.createElement('option');o.value=p;o.textContent=p;if(p===selVal)o.selected=true;sel.appendChild(o);const o2=document.createElement('option');o2.value=p;o2.textContent=p;del.appendChild(o2);const o3=document.createElement('option');o3.value=p;o3.textContent=p;load.appendChild(o3);});}).catch(()=>{});}
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
async function loadGenDirs(){
  const list=document.getElementById('genDirList');
  for(let attempt=0;attempt<5;attempt++){
    if(attempt>0)await new Promise(r=>setTimeout(r,500));
    try{
      const dirs=await(await fetch('/lsgifdirs')).json();
      if(!dirs.length&&attempt<4)continue;
      list.innerHTML='';
      dirs.forEach(d=>{
        const name=(d&&typeof d==='object')?d.name:d;
        const row=document.createElement('label');
        row.innerHTML='<input type="checkbox" value="'+name+'"><span class="name">&#x1F4C1; '+name+'</span>';
        list.appendChild(row);
      });
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
// requete HTTP traitee a la fois) et les deux boucles de polling
// (generatePlaylist()/resyncTous(), toutes les 700ms) tournent EN
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
// Resynchronisation incrementale du fichier maitre interne (plan tous-txt-
// filtrage-diff-sync) : reutilise le meme mecanisme de progression/arret que
// generatePlaylist() (meme g_plGenStatus cote firmware, meme bouton
// genStopBtn/stopGeneratePlaylist()) -- seul le texte de progression differe
// (isResync=true cote firmware, pas de dirIdx/totalDirs/gifs pendant la
// Phase 1 de detection, juste le dossier en cours et le nombre de dossiers
// changes detectes jusque-la).
async function resyncTous(){
  setPageBusy(true);
  const msgEl=document.getElementById('msg');
  if(window._msgTimer)clearTimeout(window._msgTimer);
  msgEl.className='msg ok';msgEl.style.display='block';msgEl.textContent=tr('msg_resync_starting');
  let started=false;
  try{
    const r=await fetch('/resync-tous',{method:'POST'});
    const t=await r.text();
    started=t.includes('STARTED');
    if(!started){msgEl.textContent=(r.status===409)?tr('msg_gen_busy'):t;msgEl.className='msg err';}
  }catch(e){
    try{
      const st=await(await fetch('/generate-playlist-status')).json();
      started=!!st.active;
    }catch(e2){started=false;}
    if(!started){msgEl.textContent=tr('msg_net_error');msgEl.className='msg err';}
  }
  if(!started){setPageBusy(false);return;}
  while(true){
    await new Promise(res=>setTimeout(res,700));
    if(_stopRequestPending)continue; // laisse la requete d'arret passer seule (serveur mono-thread)
    let st;
    try{
      const ctrl=new AbortController();
      const abortTimer=setTimeout(()=>ctrl.abort(),9000);
      st=await(await fetch('/generate-playlist-status',{signal:ctrl.signal})).json();
      clearTimeout(abortTimer);
    }catch(e){continue;}
    if(!st.active){
      msgEl.textContent=st.result||tr('msg_gen_start_error');
      msgEl.className='msg '+(st.done?'ok':'err');
      if(window._msgTimer)clearTimeout(window._msgTimer);
      window._msgTimer=setTimeout(()=>{msgEl.style.display='none';},5000);
      fetch('/dmd-pause',{method:'POST',body:new URLSearchParams({msg:stripAccents(st.result||''),color:'1'}),headers:{'Content-Type':'application/x-www-form-urlencoded'}}).catch(()=>{});
      fillPlaylists('');
      break;
    }
    msgEl.textContent=tr('msg_resync_progress').replace('${0}',st.dir||'').replace('${1}',st.foldersChanged);
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
net_error:'Erreur réseau',msg_deleting:'Suppression...',msg_select_folder:'Choisissez au moins un dossier',msg_confirm_delete_folders:'Supprimer ${0} ?',msg_specify_dir:'Précisez un dossier cible',msg_select_gif:'Choisissez un fichier GIF',msg_select_gif_files:'Choisissez des fichiers .gif',msg_preparing_folder:'Preparation du dossier...',msg_cannot_create_folder:'Impossible de creer le dossier: ${0}',msg_net_error_folder:'Erreur reseau (creation dossier)',msg_uploading:'Upload...',msg_attempt:'tentative ${0}/${1}',msg_stopped_by_user:'Arrete par l\'utilisateur (${0}/${1})',msg_upload_fail:'ECHEC',msg_failures:'Echecs: ${0}',msg_upload_result:'${0}/${1} fichier(s) uploade(s)',msg_upload_result_fail:' -- echecs: ${0}',msg_confirm_reboot:'Redemarrer l\'ESP32 ?',msg_rebooting:'Redemarrage...',msg_dmd_resumed:'DMD repris',msg_updating_playlists:'Mise a jour des playlists...'},
en:{title:'RecalBox DMD - Media',h1:'Media',nav_basic:'&#x1F4A1; Display &amp; Playlists',nav_network:'&#x1F4F6; Wi-Fi &amp; BT',nav_clock:'&#x23F0; Clock',nav_media:'&#x1F4BF; Media',
sec_dirs:'&#x1F4C1; Folders (/gifs/)',desc_dirs:'Check folders to delete them.',btn_select_all:'Select all',btn_select_none:'Select none',btn_delete_sel:'&#x1F5D1; Delete selection',
sec_upload:'&#x1F4E4; GIF Upload',desc_upload:'Add a .gif file directly from your browser into a folder in /gifs/. Choose an existing folder OR type a new name (created automatically). &#x26A0;&#xFE0F; Not designed for transferring many files (slow throughput, risk of write errors) -- meant for occasionally adding a few files. For a large transfer, remove the SD card and copy from a PC instead.',placeholder_upload_dir:'or new folder...',lbl_upload_file:'.gif files',btn_upload:'&#x1F4E4; Upload',btn_stop:'&#x23F9; Stop',
btn_reboot:'&#x1F504; Reboot',btn_resume:'&#x25B6; Resume DMD',
net_error:'Network error',msg_deleting:'Deleting...',msg_select_folder:'Select at least one folder',msg_confirm_delete_folders:'Delete ${0}?',msg_specify_dir:'Please specify a target folder',msg_select_gif:'Select a GIF file',msg_select_gif_files:'Select .gif files',msg_preparing_folder:'Preparing folder...',msg_cannot_create_folder:'Unable to create folder: ${0}',msg_net_error_folder:'Network error (folder creation)',msg_uploading:'Uploading...',msg_attempt:'attempt ${0}/${1}',msg_stopped_by_user:'Stopped by user (${0}/${1})',msg_upload_fail:'FAILED',msg_failures:'Failures: ${0}',msg_upload_result:'${0}/${1} file(s) uploaded',msg_upload_result_fail:' -- failures: ${0}',msg_confirm_reboot:'Reboot the ESP32?',msg_rebooting:'Rebooting...',msg_dmd_resumed:'DMD resumed',msg_updating_playlists:'Updating playlists...'},
es:{title:'RecalBox DMD - Medios',h1:'Medios',nav_basic:'&#x1F4A1; Pantalla y listas',nav_network:'&#x1F4F6; Wi-Fi y BT',nav_clock:'&#x23F0; Reloj',nav_media:'&#x1F4BF; Medios',
sec_dirs:'&#x1F4C1; Carpetas (/gifs/)',desc_dirs:'Marque las carpetas para eliminarlas.',btn_select_all:'Seleccionar todo',btn_select_none:'Deseleccionar todo',btn_delete_sel:'&#x1F5D1; Eliminar selección',
sec_upload:'&#x1F4E4; Subir GIF',desc_upload:'Añada un archivo .gif desde su navegador a una carpeta en /gifs/. Elija una carpeta existente O escriba un nombre nuevo (se crea automáticamente). &#x26A0;&#xFE0F; No pensado para transferir muchos archivos (velocidad lenta, riesgo de error de escritura) -- reservado para añadir algunos archivos puntualmente. Para una transferencia importante, retire la tarjeta SD y cópiela desde un PC.',placeholder_upload_dir:'o nueva carpeta...',lbl_upload_file:'Archivos .gif',btn_upload:'&#x1F4E4; Subir',btn_stop:'&#x23F9; Detener',
btn_reboot:'&#x1F504; Reiniciar',btn_resume:'&#x25B6; Reanudar DMD',
net_error:'Error de red',msg_deleting:'Eliminando...',msg_select_folder:'Elija al menos una carpeta',msg_confirm_delete_folders:'¿Eliminar ${0}?',msg_specify_dir:'Especifique una carpeta destino',msg_select_gif:'Seleccione un archivo GIF',msg_select_gif_files:'Seleccione archivos .gif',msg_preparing_folder:'Preparando carpeta...',msg_cannot_create_folder:'No se pudo crear la carpeta: ${0}',msg_net_error_folder:'Error de red (creación de carpeta)',msg_uploading:'Subiendo...',msg_attempt:'intento ${0}/${1}',msg_stopped_by_user:'Detenido por el usuario (${0}/${1})',msg_upload_fail:'ERROR',msg_failures:'Errores: ${0}',msg_upload_result:'${0}/${1} archivo(s) subido(s)',msg_upload_result_fail:' -- errores: ${0}',msg_confirm_reboot:'¿Reiniciar el ESP32?',msg_rebooting:'Reiniciando...',msg_dmd_resumed:'DMD reanudado',msg_updating_playlists:'Actualizando listas...'}
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
function doReboot(){if(!confirm(tr('msg_confirm_reboot')))return;showMsg(tr('msg_rebooting'),true);queuedFetch('/reboot').catch(()=>{});}
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
function loadDirs(){
  return queuedFetch('/lsgifdirs').then(r=>r.json()).then(renderDirs).catch(()=>{});
}
function loadUploadDirs(){return Promise.resolve();} // conserve pour compatibilite des appels existants -- loadDirs() peuple desormais aussi #uploadDir
function deleteSelected(){
  const dirs=[].slice.call(document.querySelectorAll('#dirList input:checked')).map(i=>i.value);
  if(!dirs.length){showMsg(tr('msg_select_folder'),false);return;}
  if(!confirm(trTpl('msg_confirm_delete_folders',dirs.join(', '))))return;
  showMsg(tr('msg_deleting'),true);
  queuedFetch('/delete-folders',{method:'POST',body:new URLSearchParams({dirs:dirs.join(',')}),headers:{'Content-Type':'application/x-www-form-urlencoded'}})
    .then(r=>r.text()).then(t=>{showMsg(t,t.includes('OK'));loadDirs();loadUploadDirs();})
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
// retries (2026-07-30) : permet a l'appelant de considerer le resultat
// global comme suspect et d'invalider l'etat de resync sauvegarde (voir
// tousSyncTask()) plutot que de faire confiance a un fichier maitre
// potentiellement troue en silence.
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

struct PlaylistGenRequest { String name; String dirsCsv; };

// Tourne du debut a la fin sur sa propre tache (creee a la demande, voir
// handleWebConfigGeneratePlaylist()) -- plus besoin d'une borne "fichiers par
// appel" (existait uniquement pour borner le cout par appel loop(), obsolete
// des que ce n'est plus loop() qui l'appelle). Tache PERSISTANTE essayee puis
// abandonnee le 2026-07-29 -- cf. commentaire au-dessus de la declaration de
// playlistGenTaskHandle (RecalBox_DMD.ino) pour le detail de ce qui a ete
// tente et pourquoi.
// Coeur du scan (parse dirsCsv, ouvre chaque /gifs/<dossier>, ecrit les
// chemins .gif trouves dans outFile deja ouvert) -- extrait de
// playlistGenTask() (2026-07-30) pour etre reutilise tel quel par
// tousSyncTask() (cas "premier lancement, tous.txt n'existe pas encore" --
// voir plan tous-txt-filtrage-diff-sync.md). Ne touche JAMAIS
// g_plGenStatus.active/resultMsg/done -- chaque appelant garde la
// responsabilite de les positionner selon son propre contexte (generation
// classique vs bootstrap de tous.txt), seul g_plGenStatus.curDirName/
// dirIdx/curDirGifs/totalGifs (progression, deja affichee sur le DMD/la
// page web) est mis a jour ici, identique pour les deux appelants.
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
  String dirsCsv = req->dirsCsv;
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

  int totalGifs = 0;
  bool stopped = false, lowHeapAbort = false, hadWriteLoss = false;
  scanFoldersToPlaylistFile(dirsCsv, outFile, totalGifs, stopped, lowHeapAbort, hadWriteLoss);

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
    String resultMsg = "OK: " + String(totalGifs) + " GIFs ajoutes dans la playlist " + name + ".txt";
    // hadWriteLoss (2026-07-30) : contrairement au fichier maitre interne
    // (tousSyncTask()), une playlist classique n'a pas de mecanisme de
    // revalidation automatique -- seul un signal explicite permet a
    // l'utilisateur de savoir qu'une regeneration est justifiee.
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

// Chemin du fichier maitre interne (plan tous-txt-filtrage-diff-sync,
// 2026-07-30) utilise par filterPlaylistFromMaster()/tousSyncTask().
// Nomme et EXTENSIONNE distinctement de toute playlist classique (demande
// utilisateur 2026-07-30, apres le premier nom "_master_gifs.txt") : ".txt"
// le rendait detectable par tout code qui filtre les fichiers de
// /playlists sur cette extension -- notamment handleWebConfigAddToPlaylists
// Batch() (scan declenche a chaque upload), qui le traitait donc comme une
// playlist normale. Extension ".dat" (au lieu de ".txt") : n'est plus
// jamais confondu avec une playlist nulle part, MAIS n'est plus non plus
// tenu a jour automatiquement par ce meme mecanisme lors d'un upload --
// seul un clic explicite sur "Resynchroniser l'index GIFs" le met a jour
// desormais (compromis assume, discute avec l'utilisateur).
#define TOUS_MASTER_PATH "/playlists/cache_master_gifs.dat"

// Filtre le fichier maitre interne (TOUS_MASTER_PATH) vers outputPath, ne
// gardant que les lignes dont le segment dossier appartient a dirsCsv
// (format ",dir1,dir2,"). Meme algorithme de lecture par blocs de 512
// octets que handleWebConfigPlaylistDirs() (pending += buf, decoupage sur
// '\n', report du reliquat, PLUS traitement de la derniere ligne sans '\n'
// final -- piege facile a oublier en adaptant ce motif). Ne touche JAMAIS
// /gifs/ -- tourne directement dans le thread loop() (meme raison que
// handleWebConfigPlaylistDirs()/handleWebConfigAddToPlaylistsBatch() qui
// font deja ca sans tache ni mutex : aucune lenteur SD localisee possible
// sur un fichier texte). Ecrit d'abord dans outputPath+".flt" puis
// remplace atomiquement (forceDeleteFile + rename), jamais d'ecriture
// directe sur outputPath.
static bool filterPlaylistFromMaster(const String &dirsCsv, const String &outputPath,
                                      int &linesWrittenOut, String &errOut)
{
  File src = SD.open(TOUS_MASTER_PATH, FILE_READ);
  if (!src) { errOut = "fichier maitre introuvable"; return false; }

  // ",dir1,dir2," -- meme convention que "seen" dans handleWebConfigPlaylistDirs().
  // reserve() AJOUTE (2026-07-30) : bug reel confirme en test materiel --
  // sans reservation prealable, String::operator+=() peut echouer
  // SILENCIEUSEMENT sous heap critique (maxalloc=4596 observe) en pleine
  // boucle de concatenation, faisant purement et simplement disparaitre un
  // ou plusieurs dossiers de "wanted" SANS AUCUNE ERREUR VISIBLE -- 5 GIFs
  // obtenus au lieu de ~11000 attendus (tous les dossiers coches) sur ce
  // test precis. Une seule grosse allocation en amont (au lieu de N petites
  // reallocations incrementales, chacune un point de defaillance silencieux
  // distinct) et une verification explicite de son succes transforment ce
  // risque en echec net et immediat plutot qu'un resultat faux et muet.
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

  String tmpPath = outputPath + ".flt";
  if (SD.exists(tmpPath.c_str())) SD.remove(tmpPath.c_str());
  File out = SD.open(tmpPath.c_str(), FILE_WRITE);
  if (!out) { src.close(); errOut = "ecriture impossible"; return false; }

  int written = 0;
  String outBuf;
  const size_t BUFSZ = 512;
  char buf[BUFSZ + 1];
  String pending;
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
            if (outBuf.length() > 1000) { writeBufChecked(out, outBuf); outBuf = ""; }
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
  if (outBuf.length() > 0) writeBufChecked(out, outBuf);
  out.close();
  src.close();

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
  String dirs = webServer->arg("dirs");
  String outputPath = "/playlists/" + name + ".txt";

  // Filtrage texte depuis le fichier maitre interne (2026-07-30, plan
  // tous-txt-filtrage-diff-sync) -- chemin rapide, synchrone (tourne dans
  // loop(), pas de tache), ne touche jamais /gifs/. S'applique a n'importe
  // quel nom de playlist (y compris "tous", la playlist CLASSIQUE du
  // projet -- distincte du fichier maitre interne, voir TOUS_MASTER_PATH).
  // Bascule automatiquement sur l'ancien chemin (scan complet classique
  // ci-dessous) si le fichier maitre n'existe pas encore (bootstrap) --
  // aucun code de "premier lancement" separe necessaire.
  if (SD.exists(TOUS_MASTER_PATH)) {
    int linesWritten = 0;
    String err;
    bool ok = filterPlaylistFromMaster(dirs, outputPath, linesWritten, err);
    if (ok) {
      Serial.println("[WEB] generate-playlist: " + String(linesWritten) + " GIFs (generation rapide) -> " + name + ".txt");
      webServer->send(200, "text/plain", "OK: " + String(linesWritten) + " GIFs (generation rapide)");
    } else {
      Serial.println("[WEB] generate-playlist: ECHEC filtrage (" + err + ")");
      webServer->send(500, "text/plain", "ERR: " + err);
    }
    return;
  }

  if (!SD.exists("/playlists")) SD.mkdir("/playlists");
  if (SD.exists(outputPath.c_str())) SD.remove(outputPath.c_str());
  File outf = SD.open(outputPath.c_str(), FILE_WRITE);
  if (!outf) { webServer->send(500, "text/plain", "ERR: ecriture impossible"); return; }
  outf.close(); // validation d'ecriture seulement -- playlistGenTask() rouvre le fichier elle-meme

  int totalDirs = 1;
  for (int i = 0; i < dirs.length(); i++) if (dirs.charAt(i) == ',') totalDirs++;

  if (xSemaphoreTake(plGenStatusMutex, portMAX_DELAY) == pdTRUE) {
    g_plGenStatus = PlaylistGenStatus();
    g_plGenStatus.active = true;
    g_plGenStatus.totalDirs = totalDirs;
    xSemaphoreGive(plGenStatusMutex);
  }

  PlaylistGenRequest *req = new PlaylistGenRequest{ name, dirs };
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
  Serial.println("[WEB] generate-playlist: demarrage " + name + ".txt, dirs=" + dirs);
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
  json += ",\"isResync\":" + String(snap.isResync ? "true" : "false");
  json += ",\"foldersChanged\":" + String(snap.foldersChanged);
  json += ",\"linesAdded\":" + String(snap.linesAdded);
  json += ",\"linesRemoved\":" + String(snap.linesRemoved);
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

// Plafond RAM pour la detection d'AJOUTS pendant une resynchronisation
// (voir tousSyncTask()) -- au-dela, le retrait des fichiers disparus
// continue normalement (verification SD.exists() par ligne, aucune
// accumulation necessaire), seule la detection de nouveaux fichiers est
// abandonnee pour ce dossier precis. Nombre de dossiers pouvant changer
// SIMULTANEMENT en une seule resynchronisation -- cas normal : 1-3 (le
// but meme de la resync incrementale est d'eviter d'en avoir beaucoup a
// la fois).
#define TOUS_SYNC_MAX_ENTRIES_PER_FOLDER 1024
#define TOUS_SYNC_MAX_CHANGED_FOLDERS 32

struct ChangedFolderInfo {
  String dirName;
  String listedCsv;  // ",f1.gif,f2.gif,..." -- fichiers reellement presents (source de verite), vide si capped/deleted
  String seenCsv;    // ",f1.gif,..." -- fichiers deja rencontres comme ligne existante dans tous.txt (Phase 2)
  bool capped = false;
  bool deleted = false; // dossier disparu de /gifs/ depuis le dernier etat sauvegarde
};

// FNV-1a 32 bits sur un nom de fichier seul (pas le chemin complet) --
// XOR-combine sur tout un dossier (voir tousSyncTask()) pour etre
// independant de l'ordre d'enumeration, jamais garanti stable sur FAT.
static uint32_t fnv1aString(const String &s)
{
  uint32_t h = 2166136261u;
  for (size_t i = 0; i < s.length(); i++) { h ^= (uint8_t)s[i]; h *= 16777619u; }
  return h;
}

// tousSyncTask() -- resynchronisation du fichier maitre interne
// (TOUS_MASTER_PATH) par diff (2026-07-30, plan tous-txt-filtrage-diff-
// sync). 2 cas :
//
// 1) le fichier maitre n'existe pas encore (bootstrap) : delegue entierement
//    a scanFoldersToPlaylistFile() sur TOUS les dossiers de /gifs/ -- scan
//    complet classique, comme n'importe quelle generation de playlist.
//
// 2) le fichier maitre existe deja : Phase 1 (detection legere par dossier,
//    compte + hash independant de l'ordre, compare a un etat sauvegarde
//    separe TOUS_MASTER_PATH+".dircache_state" -- JAMAIS playlistSigPath,
//    qui ne suit que la playlist active en config.ini, RecalBox_DMD.ino)
//    puis Phase 2 (une seule passe de correction sur le fichier maitre,
//    seulement si au moins un dossier a change).
//
// stopRequested/garde heap verifies a chaque dossier en Phase 1 (jamais en
// pleine Phase 2, qui reste une operation courte une fois demarree) --
// meme discipline que scanFoldersToPlaylistFile(). Le fichier maitre est
// toujours soit entierement patche (un seul renommage final), soit
// integralement intact -- jamais partiel, meme en cas de coupure de courant
// ou d'arret
// demande en pleine Phase 2 (le fichier temporaire ".flt" est alors
// simplement abandonne).
void tousSyncTask(void *param)
{
  (void)param;
  const char *TOUS_PATH = TOUS_MASTER_PATH;
  const char *STATE_PATH = TOUS_MASTER_PATH ".dircache_state";

  bool stopped = false;
  bool lowHeapAbort = false;

  // Enumeration live des dossiers reels sous /gifs/ -- JAMAIS depuis l'etat
  // sauvegarde : un dossier supprime de /gifs/ doit etre traite comme
  // "change" (toutes ses lignes a retirer), pas silencieusement ignore.
  String liveDirsCsv;
  if (xSemaphoreTake(sdAccessMutex, portMAX_DELAY) == pdTRUE) {
    File dir = SD.open("/gifs");
    if (dir && dir.isDirectory()) {
      File entry = dir.openNextFile();
      while (entry) {
        if (entry.isDirectory()) {
          String name = String(entry.name());
          int slash = name.lastIndexOf('/');
          if (slash >= 0) name = name.substring(slash + 1);
          if (liveDirsCsv.length() > 0) liveDirsCsv += ",";
          liveDirsCsv += name;
        }
        entry.close();
        entry = dir.openNextFile();
      }
      dir.close();
    } else if (dir) {
      dir.close();
    }
    xSemaphoreGive(sdAccessMutex);
  }

  bool bootstrap = !SD.exists(TOUS_PATH);

  if (bootstrap)
  {
    File outFile;
    if (xSemaphoreTake(sdAccessMutex, portMAX_DELAY) == pdTRUE) {
      outFile = SD.open(TOUS_PATH, FILE_WRITE);
      xSemaphoreGive(sdAccessMutex);
    }
    if (!outFile) {
      if (xSemaphoreTake(plGenStatusMutex, portMAX_DELAY) == pdTRUE) {
        g_plGenStatus.resultMsg = "ERR: ecriture impossible (fichier maitre)";
        g_plGenStatus.active = false;
        g_plGenStatus.done = true;
        xSemaphoreGive(plGenStatusMutex);
      }
      playlistGenTaskHandle = nullptr;
      vTaskDelete(nullptr);
      return;
    }
    int totalGifs = 0;
    bool hadWriteLoss = false;
    scanFoldersToPlaylistFile(liveDirsCsv, outFile, totalGifs, stopped, lowHeapAbort, hadWriteLoss);
    if (stopped) {
      // Garde heap avant nettoyage (2026-07-30, meme raison que dans
      // playlistGenTask()) : forceDeleteFile() peut lui-meme declencher
      // l'abort() lock_init_generic()/__sfp si le heap est deja trop bas.
      if (xSemaphoreTake(sdAccessMutex, portMAX_DELAY) == pdTRUE) {
        outFile.close();
        if (ESP.getMaxAllocHeap() >= 4096) forceDeleteFile(String(TOUS_PATH));
        xSemaphoreGive(sdAccessMutex);
      }
      if (xSemaphoreTake(plGenStatusMutex, portMAX_DELAY) == pdTRUE) {
        g_plGenStatus.resultMsg = lowHeapAbort
          ? "Memoire insuffisante, fichier maitre non cree. Redemarrez le DMD puis reessayez"
          : "Creation du fichier maitre annulee";
        g_plGenStatus.active = false;
        g_plGenStatus.done = true;
        xSemaphoreGive(plGenStatusMutex);
      }
    } else {
      if (xSemaphoreTake(sdAccessMutex, portMAX_DELAY) == pdTRUE) { outFile.close(); xSemaphoreGive(sdAccessMutex); }
      String resultMsg = "OK: fichier maitre cree (" + String(totalGifs) + " GIFs)";
      // hadWriteLoss (2026-07-30) : le bootstrap n'ecrit JAMAIS STATE_PATH
      // (voir plus bas) -- une resynchronisation suivante repart donc
      // toujours d'une verification complete de chaque dossier (aucune
      // signature prealable a comparer), ce qui comble automatiquement tout
      // trou via la detection d'ajout de la Phase 2. Le message previent
      // simplement l'utilisateur qu'un second passage est recommande plutot
      // que de laisser croire a un resultat garanti complet.
      if (hadWriteLoss) resultMsg += " (ATTENTION: ecriture incomplete detectee, relancez une resynchronisation pour verifier/completer)";
      Serial.println("[WEB] tousSyncTask: " + resultMsg);
      if (xSemaphoreTake(plGenStatusMutex, portMAX_DELAY) == pdTRUE) {
        g_plGenStatus.resultMsg = resultMsg;
        g_plGenStatus.active = false;
        g_plGenStatus.done = true;
        xSemaphoreGive(plGenStatusMutex);
      }
    }
    playlistGenTaskHandle = nullptr;
    vTaskDelete(nullptr);
    return;
  }

  // --- Cas courant : le fichier maitre existe deja ---------------------

  // Etat sauvegarde precedent, charge en RAM (petit fichier -- quelques
  // dizaines de dossiers, jamais des milliers -- sans rapport avec la
  // taille du fichier maitre lui-meme).
  String prevState;
  {
    File sf = SD.open(STATE_PATH, FILE_READ);
    if (sf) {
      while (sf.available()) prevState += (char)sf.read();
      sf.close();
    }
  }
  auto findPrevSignature = [&](const String &dirName, int &countOut, uint32_t &hashOut) -> bool {
    int searchFrom = 0;
    while (searchFrom <= (int)prevState.length()) {
      int nl = prevState.indexOf('\n', searchFrom);
      String line = (nl < 0) ? prevState.substring(searchFrom) : prevState.substring(searchFrom, nl);
      if (line.length() > 0) {
        int c1 = line.indexOf(',');
        int c2 = (c1 >= 0) ? line.indexOf(',', c1 + 1) : -1;
        if (c1 > 0 && c2 > c1 && line.substring(0, c1) == dirName) {
          countOut = line.substring(c1 + 1, c2).toInt();
          hashOut = (uint32_t)strtoul(line.substring(c2 + 1).c_str(), NULL, 10);
          return true;
        }
      }
      if (nl < 0) break;
      searchFrom = nl + 1;
    }
    return false;
  };

  static ChangedFolderInfo changed[TOUS_SYNC_MAX_CHANGED_FOLDERS];
  for (int i = 0; i < TOUS_SYNC_MAX_CHANGED_FOLDERS; i++) changed[i] = ChangedFolderInfo();
  int changedCount = 0;
  String newState;
  // Dossiers dont au moins une ligne a ete perdue pendant l'ecriture Phase 2
  // (2026-07-30) -- format ",dir1,dir2," comme les autres accumulateurs CSV
  // de ce fichier. Rempli plus bas (chunkDirs/writeBufChecked). Seuls CES
  // dossiers precis verront leur signature omise de newState avant sauvegarde
  // (voir fin de fonction) : une resynchronisation suivante ne revverifiera
  // donc QU'EUX, jamais l'integralite -- correction ciblee plutot que
  // d'invalider tout l'etat (qui forcerait un rescan complet a chaque raté,
  // potentiellement en boucle sans jamais converger si les pertes persistent
  // -- retour utilisateur explicite sur ce risque).
  String badFolders = ",";

  // Phase 1 : une enumeration par dossier reel, comparaison a l'etat sauvegarde.
  {
    int start = 0;
    while (start <= (int)liveDirsCsv.length())
    {
      if (xSemaphoreTake(plGenStatusMutex, portMAX_DELAY) == pdTRUE) {
        stopped = g_plGenStatus.stopRequested;
        xSemaphoreGive(plGenStatusMutex);
      }
      if (!stopped && ESP.getMaxAllocHeap() < 4096) {
        Serial.println("[WEB] tousSyncTask: heap critique (maxalloc=" + String(ESP.getMaxAllocHeap()) + "), arret propre (Phase 1)");
        stopped = true;
        lowHeapAbort = true;
      }
      if (stopped) break;

      int comma = liveDirsCsv.indexOf(',', start);
      String dirName = (comma < 0) ? liveDirsCsv.substring(start) : liveDirsCsv.substring(start, comma);
      dirName.trim();
      start = (comma < 0) ? (int)(liveDirsCsv.length() + 1) : (comma + 1);
      if (dirName.length() == 0) continue;

      if (xSemaphoreTake(plGenStatusMutex, portMAX_DELAY) == pdTRUE) {
        g_plGenStatus.curDirName = dirName;
        g_plGenStatus.curDirGifs = 0;
        xSemaphoreGive(plGenStatusMutex);
      }

      int liveCount = 0;
      uint32_t liveHash = 0;
      String listedCsv = ",";
      bool capped = false;

      File dir;
      if (xSemaphoreTake(sdAccessMutex, portMAX_DELAY) == pdTRUE) {
        dir = SD.open(("/gifs/" + dirName).c_str());
        xSemaphoreGive(sdAccessMutex);
      }
      bool dirOpen = dir && dir.isDirectory();
      if (!dirOpen && dir) {
        if (xSemaphoreTake(sdAccessMutex, portMAX_DELAY) == pdTRUE) { dir.close(); xSemaphoreGive(sdAccessMutex); }
      }
      while (dirOpen) {
        if (xSemaphoreTake(plGenStatusMutex, portMAX_DELAY) == pdTRUE) {
          stopped = g_plGenStatus.stopRequested;
          xSemaphoreGive(plGenStatusMutex);
        }
        if (!stopped && ESP.getMaxAllocHeap() < 4096) {
          Serial.println("[WEB] tousSyncTask: heap critique (maxalloc=" + String(ESP.getMaxAllocHeap()) + "), arret propre (Phase 1)");
          stopped = true;
          lowHeapAbort = true;
        }
        if (stopped) {
          if (xSemaphoreTake(sdAccessMutex, portMAX_DELAY) == pdTRUE) { dir.close(); xSemaphoreGive(sdAccessMutex); }
          break;
        }
        File f;
        if (xSemaphoreTake(sdAccessMutex, portMAX_DELAY) == pdTRUE) { f = dir.openNextFile(); xSemaphoreGive(sdAccessMutex); }
        if (!f) {
          if (xSemaphoreTake(sdAccessMutex, portMAX_DELAY) == pdTRUE) { dir.close(); xSemaphoreGive(sdAccessMutex); }
          dirOpen = false;
          break;
        }
        if (!f.isDirectory()) {
          String fname = String(f.name());
          if (fname.endsWith(".gif")) {
            liveHash ^= fnv1aString(fname);
            liveCount++;
            if (!capped) {
              if (liveCount <= TOUS_SYNC_MAX_ENTRIES_PER_FOLDER) {
                listedCsv += fname + ",";
              } else {
                capped = true;
                listedCsv = "";
              }
            }
          }
        }
        f.close();
        // Progression affichee (2026-07-30) : g_plGenStatus.curDirGifs
        // n'etait jamais mis a jour pendant la Phase 1, contrairement au
        // scan classique (scanFoldersToPlaylistFile()) qui l'incremente a
        // chaque fichier -- l'ecran DMD restait bloque a "<dossier> 0"
        // pendant tout le traitement d'un gros dossier (Arcade, ~1400
        // fichiers), donnant une fausse impression de blocage alors que
        // l'enumeration avancait normalement (confirme en test reel).
        if (xSemaphoreTake(plGenStatusMutex, portMAX_DELAY) == pdTRUE) {
          g_plGenStatus.curDirGifs = liveCount;
          xSemaphoreGive(plGenStatusMutex);
        }
        vTaskDelay(1);
      }
      if (stopped) break;

      int prevCount = -1; uint32_t prevHash = 0;
      bool hadPrev = findPrevSignature(dirName, prevCount, prevHash);
      bool isChanged = !hadPrev || prevCount != liveCount || prevHash != liveHash;

      newState += dirName + "," + String(liveCount) + "," + String(liveHash) + "\n";

      if (isChanged && changedCount < TOUS_SYNC_MAX_CHANGED_FOLDERS) {
        changed[changedCount].dirName = dirName;
        changed[changedCount].listedCsv = listedCsv;
        changed[changedCount].capped = capped;
        changed[changedCount].deleted = false;
        changedCount++;
      }

      if (xSemaphoreTake(plGenStatusMutex, portMAX_DELAY) == pdTRUE) {
        g_plGenStatus.foldersChanged = changedCount;
        xSemaphoreGive(plGenStatusMutex);
      }
      vTaskDelay(1);
    }
  }

  // Dossiers presents dans l'ancien etat mais disparus de /gifs/ (supprimes
  // hors de l'appli) -- toutes leurs lignes doivent etre retirees,
  // deleted=true (pas de detection d'ajout possible pour eux, evidemment).
  if (!stopped) {
    int lineStart = 0;
    String liveWrapped = "," + liveDirsCsv + ",";
    while (lineStart < (int)prevState.length() && changedCount < TOUS_SYNC_MAX_CHANGED_FOLDERS) {
      int nl = prevState.indexOf('\n', lineStart);
      String line = (nl < 0) ? prevState.substring(lineStart) : prevState.substring(lineStart, nl);
      int c1 = line.indexOf(',');
      if (c1 > 0) {
        String d = line.substring(0, c1);
        if (liveWrapped.indexOf("," + d + ",") < 0) {
          bool already = false;
          for (int i = 0; i < changedCount; i++) if (changed[i].dirName == d) { already = true; break; }
          if (!already) {
            changed[changedCount].dirName = d;
            changed[changedCount].capped = false;
            changed[changedCount].deleted = true;
            changedCount++;
          }
        }
      }
      if (nl < 0) break;
      lineStart = nl + 1;
    }
  }

  // --- Phase 2 : une seule passe de correction sur le fichier maitre,
  // seulement si au moins un dossier a change et si Phase 1 n'a pas ete
  // interrompue. Reessayee automatiquement EN ENCHAINE jusqu'a
  // MAX_PHASE2_ATTEMPTS fois (2026-07-30, retour utilisateur explicite :
  // invalider tout l'etat sur un simple raté aurait force un rescan COMPLET
  // au prochain clic, avec un risque reel de boucle sans fin si les pertes
  // persistent -- l'utilisateur ne comprendrait jamais pourquoi "ca ne
  // marche pas"). Chaque tentative relit le fichier maitre actuel (jamais
  // modifie tant que l'echange atomique final n'a pas eu lieu -- meme apres
  // une tentative partiellement ratee, puisque le fichier vient d'etre
  // remplace par sa propre sortie, cette tentative suivante detecte et
  // rajoute a nouveau ce qui manque encore, en toute coherence) et refait le
  // filtrage+ecriture au complet. Seuls les dossiers ENCORE en echec apres
  // la derniere tentative sont exclus de l'etat sauvegarde (voir plus bas
  // apres cette boucle) : une resynchronisation future ne revverifiera QU'
  // EUX, jamais l'integralite -- correction ciblee, bornee, qui ne boucle
  // jamais indefiniment.
  int totalLinesAdded = 0, totalLinesRemoved = 0;
  bool phase2Ok = true;
  const int MAX_PHASE2_ATTEMPTS = 3;

  if (!stopped && changedCount > 0)
  {
    for (int attempt = 0; attempt < MAX_PHASE2_ATTEMPTS; attempt++)
    {
      if (xSemaphoreTake(plGenStatusMutex, portMAX_DELAY) == pdTRUE) {
        stopped = g_plGenStatus.stopRequested;
        xSemaphoreGive(plGenStatusMutex);
      }
      if (stopped) break;

      phase2Ok = false;
      badFolders = ",";
      totalLinesAdded = 0;
      totalLinesRemoved = 0;
      for (int i = 0; i < changedCount; i++) changed[i].seenCsv = "";

      String tmpPath = String(TOUS_PATH) + ".flt";
      File src, out;
      if (xSemaphoreTake(sdAccessMutex, portMAX_DELAY) == pdTRUE) {
        src = SD.open(TOUS_PATH, FILE_READ);
        if (SD.exists(tmpPath.c_str())) SD.remove(tmpPath.c_str());
        if (src) out = SD.open(tmpPath.c_str(), FILE_WRITE);
        xSemaphoreGive(sdAccessMutex);
      }

      if (src && out) {
        // flushChecked() : flush + rattache tout dossier present dans ce
        // chunk (chunkDirs) a badFolders si l'ecriture a echoue -- ainsi
        // seuls les dossiers reellement touches par une perte sont exclus
        // de l'etat sauvegarde, jamais l'ensemble (voir commentaire au-dessus
        // de cette boucle).
        auto flushChecked = [&](String &outBufRef, String &chunkDirsRef) {
          if (outBufRef.length() == 0) return;
          bool ok;
          if (xSemaphoreTake(sdAccessMutex, portMAX_DELAY) == pdTRUE) { ok = writeBufChecked(out, outBufRef); xSemaphoreGive(sdAccessMutex); }
          if (!ok) {
            int cp = 1;
            while (cp < (int)chunkDirsRef.length()) {
              int cc = chunkDirsRef.indexOf(',', cp);
              if (cc < 0) break;
              String d = chunkDirsRef.substring(cp, cc);
              if (d.length() > 0 && badFolders.indexOf("," + d + ",") < 0) badFolders += d + ",";
              cp = cc + 1;
            }
          }
          outBufRef = "";
          chunkDirsRef = ",";
        };

        String outBuf;
        String chunkDirs = ",";
        const size_t BUFSZ = 512;
        char buf[BUFSZ + 1];
        String pending;
        int chunkCount = 0;
        while (true) {
          int n = 0;
          if (xSemaphoreTake(sdAccessMutex, portMAX_DELAY) == pdTRUE) { n = src.read((uint8_t *)buf, BUFSZ); xSemaphoreGive(sdAccessMutex); }
          if (n <= 0) break;
          buf[n] = 0;
          pending += buf;
          int lineStart2 = 0;
          while (true) {
            int nl = pending.indexOf('\n', lineStart2);
            if (nl < 0) break;
            String line = pending.substring(lineStart2, nl);
            String trimmed = line; trimmed.trim();
            bool keep = true;
            String lineDir;
            if (trimmed.startsWith("/gifs/")) {
              int s2 = trimmed.indexOf('/', 6);
              if (s2 > 6) {
                lineDir = trimmed.substring(6, s2);
                int ci = -1;
                for (int i = 0; i < changedCount; i++) if (changed[i].dirName == lineDir) { ci = i; break; }
                if (ci >= 0) {
                  if (changed[ci].deleted) {
                    keep = false;
                  } else {
                    bool exists;
                    if (xSemaphoreTake(sdAccessMutex, portMAX_DELAY) == pdTRUE) { exists = SD.exists(trimmed.c_str()); xSemaphoreGive(sdAccessMutex); }
                    if (!exists) {
                      keep = false;
                    } else {
                      int lastSlash = trimmed.lastIndexOf('/');
                      String fname = (lastSlash >= 0) ? trimmed.substring(lastSlash + 1) : trimmed;
                      if (changed[ci].seenCsv.indexOf("," + fname + ",") < 0) changed[ci].seenCsv += "," + fname + ",";
                    }
                  }
                }
              }
            }
            if (keep) {
              outBuf += line + "\n";
              if (lineDir.length() > 0 && chunkDirs.indexOf("," + lineDir + ",") < 0) chunkDirs += lineDir + ",";
            } else { totalLinesRemoved++; }
            if (outBuf.length() > 1000) flushChecked(outBuf, chunkDirs);
            lineStart2 = nl + 1;
          }
          pending = pending.substring(lineStart2);
          if ((size_t)n < BUFSZ) break;
          if (++chunkCount % 20 == 0) yield();
        }
        pending.trim();
        if (pending.length() > 0) {
          bool keep = true;
          String lineDir;
          if (pending.startsWith("/gifs/")) {
            int s2 = pending.indexOf('/', 6);
            if (s2 > 6) {
              lineDir = pending.substring(6, s2);
              for (int i = 0; i < changedCount; i++) {
                if (changed[i].dirName == lineDir) {
                  bool exists = false;
                  if (!changed[i].deleted && xSemaphoreTake(sdAccessMutex, portMAX_DELAY) == pdTRUE) { exists = SD.exists(pending.c_str()); xSemaphoreGive(sdAccessMutex); }
                  if (changed[i].deleted || !exists) {
                    keep = false;
                  } else {
                    int lastSlash = pending.lastIndexOf('/');
                    String fname = (lastSlash >= 0) ? pending.substring(lastSlash + 1) : pending;
                    if (changed[i].seenCsv.indexOf("," + fname + ",") < 0) changed[i].seenCsv += "," + fname + ",";
                  }
                  break;
                }
              }
            }
          }
          if (keep) {
            outBuf += pending + "\n";
            if (lineDir.length() > 0 && chunkDirs.indexOf("," + lineDir + ",") < 0) chunkDirs += lineDir + ",";
          } else { totalLinesRemoved++; }
        }

        // Ajouts : pour chaque dossier change non capped/non supprime, tout
        // fichier de listedCsv (source de verite complete) absent de seenCsv
        // (jamais rencontre comme ligne existante ci-dessus) est nouveau.
        for (int i = 0; i < changedCount; i++) {
          if (changed[i].deleted || changed[i].capped) continue;
          String &lc = changed[i].listedCsv;
          int p = 1;
          while (p < (int)lc.length()) {
            int nextComma = lc.indexOf(',', p);
            if (nextComma < 0) break;
            String fname = lc.substring(p, nextComma);
            if (fname.length() > 0 && changed[i].seenCsv.indexOf("," + fname + ",") < 0) {
              outBuf += "/gifs/" + changed[i].dirName + "/" + fname + "\n";
              if (chunkDirs.indexOf("," + changed[i].dirName + ",") < 0) chunkDirs += changed[i].dirName + ",";
              totalLinesAdded++;
              if (outBuf.length() > 1000) flushChecked(outBuf, chunkDirs);
            }
            p = nextComma + 1;
          }
        }
        flushChecked(outBuf, chunkDirs);
        if (xSemaphoreTake(sdAccessMutex, portMAX_DELAY) == pdTRUE) {
          out.close();
          src.close();
          forceDeleteFile(String(TOUS_PATH));
          SD.rename(tmpPath.c_str(), TOUS_PATH);
          xSemaphoreGive(sdAccessMutex);
        }
        phase2Ok = true;
      } else {
        if (xSemaphoreTake(sdAccessMutex, portMAX_DELAY) == pdTRUE) {
          if (src) src.close();
          if (out) out.close();
          xSemaphoreGive(sdAccessMutex);
        }
      }

      if (badFolders == ",") break; // rien a rattraper, inutile de retenter
      // Statut visible (DMD/page web) pendant les tentatives suivantes --
      // demande utilisateur (2026-07-30) : ne pas laisser l'utilisateur sans
      // aucune indication pendant qu'un nouvel essai automatique est en
      // cours. Reutilise g_plGenStatus.curDirName/curDirGifs, meme
      // convention d'affichage ("<texte> <nombre>") que la Phase 1.
      {
        int nBad = 0;
        int cp = 1;
        while (cp < (int)badFolders.length()) { int cc = badFolders.indexOf(',', cp); if (cc < 0) break; nBad++; cp = cc + 1; }
        if (xSemaphoreTake(plGenStatusMutex, portMAX_DELAY) == pdTRUE) {
          g_plGenStatus.curDirName = "Correction (essai " + String(attempt + 2) + "/" + String(MAX_PHASE2_ATTEMPTS) + ")";
          g_plGenStatus.curDirGifs = nBad;
          xSemaphoreGive(plGenStatusMutex);
        }
      }
      Serial.println("[WEB] tousSyncTask: perte d'ecriture sur au moins un dossier (tentative " + String(attempt + 1) + "/" + String(MAX_PHASE2_ATTEMPTS) + "), nouvel essai immediat");
    }
  }

  // Etat sauvegarde mis a jour -- seulement si Phase 1 a pu se terminer
  // (newState est alors complet et correct). Les dossiers encore presents
  // dans badFolders APRES la boucle de tentatives ci-dessus (2026-07-30,
  // retour utilisateur : correction ciblee, pas une invalidation globale) en
  // sont exclus avant sauvegarde -- ainsi seuls CES dossiers precis seront
  // reconsideres comme "jamais vus" (hadPrev=false) au prochain resync,
  // qui les revverifiera et completera automatiquement ce qui manque encore
  // via la detection d'ajout de la Phase 2, sans jamais retomber sur un
  // rescan integral.
  if (!stopped) {
    String filteredState = newState;
    if (badFolders != ",") {
      filteredState = "";
      int lineStart3 = 0;
      while (lineStart3 <= (int)newState.length()) {
        int nl = newState.indexOf('\n', lineStart3);
        String ln = (nl < 0) ? newState.substring(lineStart3) : newState.substring(lineStart3, nl);
        int c1 = ln.indexOf(',');
        String d = (c1 > 0) ? ln.substring(0, c1) : "";
        if (d.length() == 0 || badFolders.indexOf("," + d + ",") < 0) {
          if (ln.length() > 0) filteredState += ln + "\n";
        }
        if (nl < 0) break;
        lineStart3 = nl + 1;
      }
    }
    if (xSemaphoreTake(sdAccessMutex, portMAX_DELAY) == pdTRUE) {
      if (SD.exists(STATE_PATH)) SD.remove(STATE_PATH);
      File sf = SD.open(STATE_PATH, FILE_WRITE);
      if (sf) { sf.print(filteredState); sf.close(); }
      xSemaphoreGive(sdAccessMutex);
    }
    if (badFolders != ",") {
      Serial.println("[WEB] tousSyncTask: dossier(s) encore en echec apres " + String(MAX_PHASE2_ATTEMPTS) + " tentatives -- exclus de l'etat sauvegarde, seront revverifies au prochain resync: " + badFolders);
    }
  }

  String resultMsg;
  if (stopped) {
    resultMsg = lowHeapAbort
      ? "Memoire insuffisante, resynchronisation annulee (fichier maitre inchange)"
      : "Resynchronisation annulee (fichier maitre inchange)";
    Serial.println("[WEB] tousSyncTask: " + resultMsg);
  } else if (changedCount == 0) {
    resultMsg = "OK: index deja a jour (0 dossier modifie)";
    Serial.println("[WEB] " + resultMsg);
  } else if (!phase2Ok) {
    resultMsg = "ERR: echec de la resynchronisation (ecriture impossible)";
    Serial.println("[WEB] tousSyncTask: " + resultMsg);
  } else {
    invalidatePlaylistRefCache();
    resultMsg = "OK: " + String(changedCount) + " dossier(s) modifie(s), " + String(totalLinesAdded) + " ajout(s), " + String(totalLinesRemoved) + " retrait(s)";
    // badFolders encore non-vide ici = echec PERSISTANT sur les memes
    // dossiers malgre MAX_PHASE2_ATTEMPTS tentatives consecutives et
    // immediates (2026-07-30, demande utilisateur) -- au-dela d'un simple
    // heap transitoire (deja couvert par les retries), ceci pointe plutot
    // vers un probleme materiel persistant (mauvais contact de la carte SD,
    // usure) qu'une nouvelle tentative differee ne resoudra probablement
    // pas seule.
    if (badFolders != ",") {
      int nBad = 0;
      int cp = 1;
      while (cp < (int)badFolders.length()) { int cc = badFolders.indexOf(',', cp); if (cc < 0) break; nBad++; cp = cc + 1; }
      resultMsg += " -- ATTENTION: echec d'ecriture persistant sur " + String(nBad) + " dossier(s) malgre " + String(MAX_PHASE2_ATTEMPTS) + " tentatives, verifiez la carte SD (contact, usure) avant de relancer";
    }
    Serial.println("[WEB] tousSyncTask: " + resultMsg);
  }
  if (xSemaphoreTake(plGenStatusMutex, portMAX_DELAY) == pdTRUE) {
    g_plGenStatus.resultMsg = resultMsg;
    g_plGenStatus.foldersChanged = changedCount;
    g_plGenStatus.linesAdded = totalLinesAdded;
    g_plGenStatus.linesRemoved = totalLinesRemoved;
    g_plGenStatus.active = false;
    g_plGenStatus.done = true;
    xSemaphoreGive(plGenStatusMutex);
  }

  Serial.println("[WEB] tousSyncTask: marge de pile restante=" + String(uxTaskGetStackHighWaterMark(nullptr) * 4) + " octets");
  playlistGenTaskHandle = nullptr;
  vTaskDelete(nullptr);
}

static void handleWebConfigResyncTous()
{
  bool alreadyActive = false;
  if (xSemaphoreTake(plGenStatusMutex, portMAX_DELAY) == pdTRUE) {
    alreadyActive = g_plGenStatus.active;
    xSemaphoreGive(plGenStatusMutex);
  }
  if (alreadyActive) { webServer->send(409, "text/plain", "ERR: generation deja en cours"); return; }

  if (xSemaphoreTake(plGenStatusMutex, portMAX_DELAY) == pdTRUE) {
    g_plGenStatus = PlaylistGenStatus();
    g_plGenStatus.active = true;
    g_plGenStatus.isResync = true;
    xSemaphoreGive(plGenStatusMutex);
  }

  Serial.println("[WEB] resync-tous: creation tache, heap libre=" + String(ESP.getFreeHeap()) + " maxalloc=" + String(ESP.getMaxAllocHeap()));
  BaseType_t taskOk = xTaskCreatePinnedToCore(tousSyncTask, "tousSync", 4096, nullptr, 1, &playlistGenTaskHandle, 0);
  if (taskOk != pdPASS) {
    Serial.println("[WEB] resync-tous: ECHEC creation tache (heap insuffisant ?)");
    if (xSemaphoreTake(plGenStatusMutex, portMAX_DELAY) == pdTRUE) {
      g_plGenStatus.active = false;
      g_plGenStatus.done = true;
      g_plGenStatus.resultMsg = "ERR: impossible de demarrer la resynchronisation (heap insuffisant)";
      xSemaphoreGive(plGenStatusMutex);
    }
    webServer->send(500, "text/plain", "ERR: impossible de demarrer la resynchronisation");
    return;
  }
  Serial.println("[WEB] resync-tous: demarrage");
  webServer->send(200, "text/plain", "STARTED");
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
    // reintroduit ici explicitement PAR NOM (2026-07-30, demande
    // utilisateur) : son extension .dat (changee volontairement pour ne
    // plus jamais etre confondu avec une playlist ailleurs, cf listing) le
    // fait sortir du filtre ".txt" ci-dessous, qui l'aurait sinon exclu de
    // ce scan et donc de la mise a jour automatique lors d'un upload.
    String masterBase = String(TOUS_MASTER_PATH);
    masterBase = masterBase.substring(masterBase.lastIndexOf('/') + 1);
    File plDir = SD.open("/playlists");
    if (plDir && plDir.isDirectory()) {
      File entry = plDir.openNextFile();
      while (entry) {
        String name = String(entry.name());
        int slash = name.lastIndexOf('/');
        String base = (slash >= 0) ? name.substring(slash + 1) : name;
        if (!entry.isDirectory() && (base.endsWith(".txt") || base == masterBase)) {
          plCount++;
          unsigned long tEntry0 = millis(); // DIAGNOSTIC TEMPORAIRE
          // entry est deja un handle ouvert sur ce fichier precis (obtenu par
          // iteration via openNextFile(), pas par nom) -- pas besoin de le
          // rouvrir. fileContainsNeedle() lit par blocs fixes (voir plus haut) :
          // evite de charger tout le fichier en memoire, cause reelle des
          // blocages 40-44s mesures en test reel sur "Tous"/"gaming" (l'ancienne
          // hypothese "recherche par nom" a ete infirmee par un test dedie).
          bool found = fileContainsNeedle(entry, needle);
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

static void handleWebConfigDeleteFolders()
{
  if (plGenIsActive()) { webServer->send(409, "text/plain", "ERR: generation de playlist en cours"); return; }
  if (!webServer->hasArg("dirs")) { webServer->send(400, "text/plain", "ERR: missing dirs"); return; }
  String dirs = webServer->arg("dirs");
  int count = 0, fail = 0, start = 0;
  while (true) {
    int comma = dirs.indexOf(',', start);
    String d = (comma < 0) ? dirs.substring(start) : dirs.substring(start, comma);
    d.trim();
    if (d.length() > 0) {
      String path = "/gifs/" + d;
      if (SD.exists(path.c_str())) {
        Serial.println("[WEB] deleteFolder start: " + path);
        if (deleteFolderRecursive(path)) { count++; Serial.println("[WEB] deleteFolder OK: " + path); }
        else { fail++; Serial.println("[WEB] deleteFolder FAIL: " + path); }
      } else {
        Serial.println("[WEB] deleteFolder introuvable: " + path);
      }
    }
    if (comma < 0) break;
    start = comma + 1;
  }
  String msg = "OK: " + String(count) + " supprime(s)" + (fail>0?", " + String(fail) + " echec(s)":"");
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
  webServer->on("/resync-tous", HTTP_POST, handleWebConfigResyncTous);
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
