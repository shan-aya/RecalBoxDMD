// ============================================
// web_config.h — Interface web de configuration
//
// safe-modify — Historique des modifications
// ============================================
// Version actuelle : v26
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

// ================================================
// HTML — i18n avec FR/EN/ES + data-i18n
// ================================================
static const char WEB_CONFIG_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="fr">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title data-i18n="page_title">RecalBox DMD Configuration</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:'Segoe UI',Tahoma,sans-serif;background:#1a1a2e;color:#eee;padding:16px;max-width:800px;margin:auto}
h1{color:#ffd146;text-align:center;margin:16px 0;font-size:24px;border-bottom:2px solid #ffd146;padding-bottom:8px}
h2{color:#8ab4f8;font-size:18px;margin:20px 0 10px;border-left:3px solid #8ab4f8;padding-left:8px}
.section{background:#16213e;border-radius:8px;padding:16px;margin:12px 0}
.row{display:flex;flex-wrap:wrap;align-items:center;margin:8px 0}
.row label{flex:0 0 160px;font-size:14px;color:#aaa}
.row input,.row select{flex:1;min-width:120px;padding:6px 8px;border:1px solid #333;border-radius:4px;background:#0f3460;color:#eee;font-size:14px}
.row input:focus,.row select:focus{outline:2px solid #ffd146}
.row input[type=checkbox]{flex:0 0 20px;width:20px;height:20px;margin:0 8px 0 0}
.row .hint{flex:0 0 100%;font-size:11px;color:#666;margin-top:2px;margin-left:160px}
.row input[type=file]{flex:1;padding:4px;color:#eee;font-size:13px}
.btn-row{display:flex;gap:12px;justify-content:center;margin:20px 0;flex-wrap:wrap}
.btn{padding:12px 32px;border:none;border-radius:6px;font-size:16px;font-weight:bold;cursor:pointer}
.btn-save{background:#ffd146;color:#1a1a2e}
.btn-save:hover{background:#ffe070}
.btn-reboot{background:#e63946;color:#fff}
.btn-reboot:hover{background:#ff5a67}
.btn-gen{background:#2d6a4f;color:#fff}
.btn-gen:hover{background:#40916c}
.btn-upload{background:#1a6b9e;color:#fff}
.btn-upload:hover{background:#2880b8}
.btn-del{background:#555;color:#fff}
.btn-del:hover{background:#777}
.msg{position:fixed;top:20px;left:50%;transform:translateX(-50%);z-index:999;padding:14px 24px;border-radius:8px;display:none;font-weight:bold;text-align:center;font-size:16px;box-shadow:0 4px 16px rgba(0,0,0,.6);max-width:90%;pointer-events:none}
.msg-ok{background:#2d6a4f;color:#d8f3dc;border:2px solid #52b788}
.msg-err{background:#6b0f0f;color:#ffcccc;border:2px solid #e63946}
.gen-folder{cursor:pointer;margin:4px 0;padding:4px 8px;background:#0f3460;border-radius:4px;display:flex;align-items:center;gap:8px}
.gen-folder:hover{background:#1a4a80}
.gen-folder input{margin:0}
.gen-folder span{flex:1;font-size:14px}
.progress{height:4px;background:#333;border-radius:2px;margin:8px 0;display:none}
.progress-bar{height:4px;background:#52b788;border-radius:2px;width:0%}
footer{text-align:center;color:#555;font-size:12px;margin:24px 0}
.lang-bar{text-align:right;margin-bottom:8px}
.lang-bar select{background:#0f3460;color:#eee;border:1px solid #333;padding:4px 8px;border-radius:4px;font-size:13px;cursor:pointer}
</style>
</head>
<body>
<div class="lang-bar"><select id="langSelect" onchange="setLang(this.value)" data-i18n-ignore></select></div>
<h1>&#x1F4BB; <span data-i18n="app_title">RecalBox DMD Config</span></h1>
<div id="msg" class="msg"></div>
<form id="configForm" onsubmit="saveConfig(event)">

<div class="section">
<h2>&#x1F4A1; <span data-i18n="sec_display">Affichage</span></h2>
<div class="row"><label data-i18n="lbl_brightness">Luminosit&eacute; (%)</label><input type="range" id="brightness" min="0" max="100" data-i18n-title="tip_brightness" oninput="document.getElementById('bval').textContent=this.value"><span id="bval" style="margin-left:8px;color:#ffd146;min-width:30px">50</span></div>
<div class="row"><label data-i18n="lbl_silent_boot">D&eacute;marrage silencieux</label><input type="checkbox" id="silent_boot" data-i18n-title="tip_silent_boot"></div>
</div>

<div class="section">
<h2>&#x1F4BF; <span data-i18n="sec_playlist">Playlist</span></h2>
<div class="row"><label data-i18n="lbl_playlist_file">Fichier playlist</label><select id="playlist" data-i18n-title="tip_playlist_file"></select></div>
<div class="row"><label data-i18n="lbl_random">Al&eacute;atoire</label><input type="checkbox" id="random" data-i18n-title="tip_random"></div>
<div class="row"><label data-i18n="lbl_delete_playlist">Supprimer</label><select id="deletePlaylistSelect" style="flex:1;padding:6px;border:1px solid #333;border-radius:4px;background:#0f3460;color:#eee;font-size:14px" data-i18n-title="tip_delete_select"></select></div>
<div class="btn-row"><button type="button" class="btn btn-del" onclick="deletePlaylist()">&#x1F5D1; <span data-i18n="btn_delete_playlist">Supprimer playlist</span></button></div>
</div>

<div class="section">
<h2>&#x2699; <span data-i18n="sec_genplaylist">G&eacute;n&eacute;rateur de playlists</span></h2>
<div style="margin:12px 0;font-size:13px;color:#aaa" data-i18n="gen_desc">Coche les dossiers &#x1F4C1; puis donne un nom pour creer une playlist contenant tous les .gif de ces dossiers.</div>
<div style="margin:4px 0 8px;display:flex;gap:8px;flex-wrap:wrap">
<button type="button" style="padding:3px 10px;border:none;border-radius:3px;background:#1a6b9e;color:#fff;font-size:11px;cursor:pointer" onclick="selectAllDirs(true)">Tout selectionner</button>
<button type="button" style="padding:3px 10px;border:none;border-radius:3px;background:#555;color:#fff;font-size:11px;cursor:pointer" onclick="selectAllDirs(false)">Rien selectionner</button>
</div>
<div id="genDirs" style="margin:8px 0;max-height:300px;overflow-y:auto"></div>
<div id="fileList" style="display:none;margin:8px 0;padding:8px;background:#0a1628;border-radius:4px;max-height:250px;overflow-y:auto"></div>
<div class="row" style="margin-top:8px"><label data-i18n="lbl_gen_name">Nom de la liste</label><input type="text" id="genName" data-i18n-placeholder="placeholder_gen_name" data-i18n-title="tip_gen_name"></div>
<div class="btn-row">
<button type="button" class="btn btn-gen" onclick="genPlaylist()">&#x2699; <span data-i18n="btn_generate">G&eacute;n&eacute;rer</span></button>
<button type="button" class="btn btn-del" onclick="deleteSelected()">&#x1F5D1; <span data-i18n="btn_del_selection">Supprimer s\u00e9lection</span></button>
</div>
</div>

<div class="section">
<h2>&#x1F4E4; <span data-i18n="sec_upload">Envoi GIF</span></h2>
<div style="margin:12px 0;font-size:13px;color:#aaa" data-i18n="upload_desc">Ajoute des fichiers .gif directement depuis ton navigateur dans un dossier de /gifs/. Si le dossier n&apos;existe pas il sera cr&eacute;&eacute;.</div>
<div class="row"><label data-i18n="lbl_upload_dir">Dossier cible</label>
<select id="uploadDirSelect" style="flex:1;padding:6px;border:1px solid #333;border-radius:4px;background:#0f3460;color:#eee;font-size:14px;margin-right:4px" data-i18n-title="tip_upload_dir"></select>
<input type="text" id="uploadDirCustom" data-i18n-placeholder="placeholder_upload_dir" style="flex:1;min-width:100px;padding:6px;border:1px solid #333;border-radius:4px;background:#0f3460;color:#eee;font-size:14px">
</div>
<div class="row"><label data-i18n="lbl_upload_file">Fichiers .gif</label><input type="file" id="uploadFile" accept=".gif" multiple data-i18n-title="tip_upload_file"></div>
<div class="progress" id="uploadProgress"><div class="progress-bar" id="uploadProgressBar"></div></div>
<div id="uploadFileList" style="margin:4px 0;font-size:12px;color:#aaa"></div>
<div class="btn-row"><button type="button" class="btn btn-upload" onclick="doUpload()">&#x1F4E4; <span data-i18n="btn_upload">Uploader</span></button></div>
</div>

<div class="section">
<h2>&#x1F4F6; <span data-i18n="sec_wifi">Wi-Fi</span></h2>
<div class="row"><label data-i18n="lbl_wifi_enabled">Activ&eacute;</label><input type="checkbox" id="wifi_enabled" data-i18n-title="tip_wifi_enabled"></div>
<div class="row"><label>SSID</label><input type="text" id="wifi_ssid" data-i18n-title="tip_wifi_ssid"></div>
<div class="row"><label data-i18n="lbl_wifi_password">Mot de passe</label><input type="password" id="wifi_password" data-i18n-title="tip_wifi_password"></div>
<div class="row"><label data-i18n="lbl_wifi_static">IP statique</label><input type="checkbox" id="wifi_static_enabled" data-i18n-title="tip_wifi_static"></div>
<div class="row"><label data-i18n="lbl_wifi_ip">IP fixe</label><input type="text" id="wifi_static_ip" data-i18n-title="tip_wifi_ip"></div>
<div class="row"><label>Gateway</label><input type="text" id="wifi_gateway" data-i18n-title="tip_wifi_gateway"></div>
<div class="row"><label>Subnet</label><input type="text" id="wifi_subnet" data-i18n-title="tip_wifi_subnet"></div>
<div class="row"><label>DNS 1</label><input type="text" id="wifi_dns1" data-i18n-title="tip_wifi_dns1"></div>
<div class="row"><label>DNS 2</label><input type="text" id="wifi_dns2" data-i18n-title="tip_wifi_dns2"></div>
</div>

<div class="section">
<h2>&#x1F4F1; <span data-i18n="sec_bt">Bluetooth</span></h2>
<div class="row"><label data-i18n="lbl_bt_enabled">Activ&eacute;</label><input type="checkbox" id="bluetooth_enabled" data-i18n-title="tip_bt_enabled"></div>
<div class="row"><label data-i18n="lbl_bt_name">Nom</label><input type="text" id="bluetooth_name" data-i18n-title="tip_bt_name"></div>
</div>

<div class="section">
<h2>&#x1F310; <span data-i18n="sec_mqtt">MQTT</span></h2>
<div class="row"><label data-i18n="lbl_mqtt_ip">IP Recalbox</label><input type="text" id="recalbox_ip" data-i18n-title="tip_mqtt_ip"></div>
</div>

<div class="section">
<h2>&#x23F0; <span data-i18n="sec_clock">Horloge</span></h2>
<div class="row"><label data-i18n="lbl_clock_enabled">Activ&eacute;e</label><input type="checkbox" id="clock_enabled" data-i18n-title="tip_clock_enabled"></div>
<div class="row">
<label data-i18n="lbl_clock_theme">Th&egrave;me</label>
<select id="clock_theme" data-i18n-title="tip_clock_theme">
<option value="-1" data-i18n="opt_random">Al&eacute;atoire</option>
<option value="0" data-i18n="opt_mario">Mario</option><option value="1" data-i18n="opt_tetris">Tetris</option>
<option value="2" data-i18n="opt_pacman">Pac-Man</option><option value="3" data-i18n="opt_spaceinv">Space Invaders</option>
<option value="4" data-i18n="opt_pong">Pong</option><option value="5" data-i18n="opt_neon">Neon</option>
<option value="6" data-i18n="opt_matrix">Matrix</option><option value="7" data-i18n="opt_fire">Fire</option>
<option value="8" data-i18n="opt_rainbow">Rainbow</option>
<option value="9" data-i18n="opt_level11">Level 1-1</option>
</select>
</div>
<div class="row"><label data-i18n="lbl_clock_neon_color">Couleur Neon</label>
<input type="color" id="clock_neon_color" value="#ff2878" data-i18n-title="tip_clock_neon_color">
<label style="font-weight:normal;display:inline-flex;align-items:center;gap:4px;margin-left:10px"><input type="checkbox" id="clock_neon_color_enabled"><span data-i18n="lbl_clock_neon_color_enabled">Personnalis&eacute;e</span></label>
<div class="hint" data-i18n="hint_clock_neon_color">Th&egrave;me Neon uniquement ; la teinte des deux-points est calcul&eacute;e automatiquement (inverse)</div>
</div>
<div class="row"><label data-i18n="lbl_clock_interval_gifs">Intervalle (GIFs)</label><input type="number" id="clock_interval" min="1" max="999" data-i18n-title="tip_clock_interval_gifs"></div>
<div class="row"><label data-i18n="lbl_clock_interval_min">Intervalle (min)</label><input type="number" id="clock_interval_min" min="0" max="999" data-i18n-title="tip_clock_interval_min"><div class="hint" data-i18n="hint_interval_min">0 = d&eacute;sactiv&eacute;</div></div>
<div class="row"><label data-i18n="lbl_clock_duration">Dur&eacute;e (sec)</label><input type="number" id="clock_duration" min="1" max="120" data-i18n-title="tip_clock_duration"></div>
<div class="row"><label data-i18n="lbl_clock_tz">Timezone</label>
<select id="clock_tz" data-i18n-title="tip_clock_tz">
<option value="CET-1CEST,M3.5.0,M10.5.0/3" data-i18n="opt_tz_ce">France / Espagne / Allemagne / Italie</option>
<option value="GMT0BST,M3.5.0/1,M10.5.0" data-i18n="opt_tz_uk">Angleterre (UK) / Portugal</option>
<option value="EST5EDT,M3.2.0,M11.1.0" data-i18n="opt_tz_usa_e">USA - Est (New York)</option>
<option value="CST6CDT,M3.2.0,M11.1.0" data-i18n="opt_tz_usa_c">USA - Centre (Chicago)</option>
<option value="MST7MDT,M3.2.0,M11.1.0" data-i18n="opt_tz_usa_m">USA - Montagnes (Denver)</option>
<option value="PST8PDT,M3.2.0,M11.1.0" data-i18n="opt_tz_usa_p">USA - Pacifique (Los Angeles)</option>
<option value="EET-2EEST,M3.5.0/3,M10.5.0/4" data-i18n="opt_tz_ee">Gr&egrave;ce / Roumanie / Finlande</option>
<option value="UTC5">UTC-5</option>
<option value="UTC4">UTC-4</option>
<option value="UTC3">UTC-3</option>
<option value="UTC2">UTC-2</option>
<option value="UTC1">UTC-1</option>
<option value="UTC0">UTC+0</option>
<option value="UTC-1">UTC+1</option>
<option value="UTC-2">UTC+2</option>
<option value="UTC-3">UTC+3</option>
<option value="UTC-4">UTC+4</option>
<option value="UTC-5">UTC+5</option>
</select>
</div>
</div>

<div class="btn-row">
<button type="submit" class="btn btn-save">&#x1F4BE; <span data-i18n="btn_save">Sauvegarder</span></button>
<button type="button" class="btn btn-reboot" onclick="saveAndReboot()">&#x1F504; <span data-i18n="btn_save_reboot">Sauvegarder &amp; Red&eacute;marrer</span></button>
<button type="button" class="btn btn-del" onclick="doReboot()">&#x1F504; <span data-i18n="btn_reboot">Red&eacute;marrer</span></button>
<button type="button" class="btn btn-gen" onclick="dmdResume()">&#x25B6; <span data-i18n="btn_resume">Reprendre DMD</span></button>
</div>
</form>
<footer><span data-i18n="footer">RecalBox DMD v7 &mdash; Interface web &mdash; Shan_ayA 2026</span></footer>
<script>
// ============= I18N =============
const LANGUAGES={fr:'Fran\u00e7ais',en:'English',es:'Espa\u00f1ol'};
const I18N={
fr:{
page_title:'RecalBox DMD Configuration',app_title:'RecalBox DMD Config',
sec_display:'Affichage',sec_playlist:'Playlist',sec_genplaylist:'G\u00e9n\u00e9rateur de playlists',
sec_upload:'Envoi GIF',sec_wifi:'Wi-Fi',sec_bt:'Bluetooth',sec_mqtt:'MQTT',sec_clock:'Horloge',
lbl_brightness:'Luminosit\u00e9 (%)',lbl_silent_boot:'D\u00e9marrage silencieux',lbl_playlist_file:'Fichier playlist',lbl_random:'Al\u00e9atoire',lbl_delete_playlist:'Supprimer',
lbl_gen_name:'Nom de la liste',lbl_upload_dir:'Dossier cible',lbl_upload_file:'Fichier .gif',
lbl_wifi_enabled:'Activ\u00e9',lbl_wifi_password:'Mot de passe',lbl_wifi_static:'IP statique',
lbl_wifi_ip:'IP fixe',lbl_bt_enabled:'Activ\u00e9',lbl_bt_name:'Nom',lbl_mqtt_ip:'IP Recalbox',
lbl_clock_enabled:'Activ\u00e9e',lbl_clock_theme:'Th\u00e8me',
lbl_clock_neon_color:'Couleur Neon',lbl_clock_neon_color_enabled:'Personnalis\u00e9e',
lbl_clock_interval_gifs:'Intervalle (GIFs)',lbl_clock_interval_min:'Intervalle (min)',
lbl_clock_duration:'Dur\u00e9e (sec)',lbl_clock_tz:'Fuseau horaire',
tip_brightness:'Luminosit\u00e9 de l\'\u00e9cran DMD (0 = \u00e9teint, 100 = max)',
tip_silent_boot:'Si coch\u00e9 : masque les \u00e9crans de d\u00e9marrage (WiFi, NTP...), n\'affiche que le titre et un sablier',
tip_playlist_file:'Playlist active affich\u00e9e en boucle. Fichiers .txt dans /playlists/',
tip_random:'Si coch\u00e9 : lecture al\u00e9atoire. Sinon : ordre du fichier playlist',
tip_gen_name:'Nom du fichier .txt \u00e0 cr\u00e9er dans /playlists/ (sans extension)',
tip_delete_select:'S\u00e9lectionne une playlist \u00e0 supprimer (fichier .txt + caches associ\u00e9s)',
tip_upload_dir:'Nom du sous-dossier dans /gifs/ (ex: NES, MAME, ne pas mettre de /)',
tip_upload_file:'S\u00e9lectionne un fichier .gif sur ton ordinateur (max ~2 Mo)',
tip_wifi_enabled:'Active ou d\u00e9sactive le Wi-Fi (et le serveur web / MQTT)',
tip_wifi_ssid:'Nom du r\u00e9seau Wi-Fi (ex: Livebox-XXXX, Orange-XXXX)',
tip_wifi_password:'Cl\u00e9 de s\u00e9curit\u00e9 WPA2 du r\u00e9seau Wi-Fi',
tip_wifi_static:'Si coch\u00e9 : utiliser une IP fixe au lieu du DHCP',
tip_wifi_ip:'Adresse IP fixe (ex: 192.168.0.100) \u2014 requis si IP statique active',
tip_wifi_gateway:'Passerelle par d\u00e9faut (ex: 192.168.0.1)',
tip_wifi_subnet:'Masque de sous-r\u00e9seau (ex: 255.255.255.0)',
tip_wifi_dns1:'Serveur DNS primaire (ex: 8.8.8.8)',
tip_wifi_dns2:'Serveur DNS secondaire (ex: 8.8.4.4)',
tip_bt_enabled:'Active ou d\u00e9sactive le Bluetooth (nom visible sur t\u00e9l\u00e9phone/tablette)',
tip_bt_name:'Nom Bluetooth affich\u00e9 lors du scan (ex: ESP32-DMD)',
tip_mqtt_ip:'Adresse IP de la Recalbox (pour recevoir les commandes MQTT)',
tip_clock_enabled:'Affiche l\'horloge r\u00e9tro entre les jeux (plusieurs th\u00e8mes pixel-art)',
tip_clock_theme:'Th\u00e8me de l\'horloge : -1 = al\u00e9atoire, 0-8 = th\u00e8me fixe (Mario, Tetris, Pac-Man...)',
tip_clock_neon_color:'Couleur de base du th\u00e8me Neon (ne s\'applique qu\'\u00e0 ce th\u00e8me)',
hint_clock_neon_color:'Th\u00e8me Neon uniquement ; la teinte des deux-points est calcul\u00e9e automatiquement (inverse)',
tip_clock_interval_gifs:'Nombre de jeux (GIFs) affich\u00e9s avant que l\'horloge r\u00e9apparaisse (ex: 10 = horloge tous les 10 jeux)',
tip_clock_interval_min:'Minutes entre chaque horloge (0 = d\u00e9sactiv\u00e9, utilise l\'intervalle en GIFs)',
tip_clock_duration:'Dur\u00e9e d\'affichage de l\'horloge en secondes avant de reprendre les jeux',
tip_clock_tz:'Choisis ton pays ou un décalage UTC fixe',
btn_generate:'G\u00e9n\u00e9rer',btn_delete:'Supprimer la s\u00e9lection',
btn_upload:'T\u00e9l\u00e9verser',btn_save:'Sauvegarder',btn_save_reboot:'Sauvegarder & Red\u00e9marrer',btn_delete_playlist:'Supprimer playlist',btn_del_selection:'Supprimer s\u00e9lection',btn_reboot:'Red\u00e9marrer',
gen_desc:'Coche les dossiers \uD83D\uDCC1 puis donne un nom pour cr\u00e9er une playlist contenant tous les .gif de ces dossiers.',
upload_desc:'Ajoute un fichier .gif directement depuis ton navigateur dans un dossier de /gifs/. Si le dossier n\'existe pas il sera cr\u00e9\u00e9.',
placeholder_gen_name:'ex: MaPlaylist',placeholder_upload_dir:'ex: NES',
hint_interval_min:'0 = d\u00e9sactiv\u00e9',
opt_random:'Al\u00e9atoire',opt_mario:'Mario',opt_tetris:'Tetris',opt_pacman:'Pac-Man',
opt_spaceinv:'Space Invaders',opt_pong:'Pong',opt_neon:'Neon',opt_matrix:'Matrix',
opt_fire:'Fire',opt_rainbow:'Rainbow',opt_level11:'Level 1-1',
opt_tz_ce:'France / Espagne / Allemagne / Italie',opt_tz_uk:'Angleterre (UK) / Portugal',
opt_tz_usa_e:'USA - Est (New York)',opt_tz_usa_c:'USA - Centre (Chicago)',
opt_tz_usa_m:'USA - Montagnes (Denver)',opt_tz_usa_p:'USA - Pacifique (Los Angeles)',
opt_tz_ee:'Gr\u00e8ce / Roumanie / Finlande',
msg_network_error:'Erreur r\u00e9seau',msg_no_playlist_name:'Donne un nom \u00e0 la playlist',msg_del_folder_confirm:'Supprimer les dossiers',msg_upload_ok:'fichier(s) uploade(s) avec succ\u00e8s',
msg_select_folder:'S\u00e9lectionne au moins un dossier',
msg_select_playlist_to_del:'S\u00e9lectionne une playlist \u00e0 supprimer',
msg_confirm_delete:'Supprimer ${0} (fichiers cache inclus) ?',
msg_specify_dir:'Pr\u00e9cise un dossier cible dans /gifs/',
msg_select_file:'S\u00e9lectionne un fichier .gif',
msg_only_gif:'Seuls les fichiers .gif sont accept\u00e9s',
msg_uploading:'Upload en cours...',msg_upload_error:'Erreur upload',
msg_load_error:'Erreur chargement config',msg_no_dirs:'Aucun dossier dans /gifs/',
msg_confirm_reboot:'Red\u00e9marrer l\u2019ESP32 ?',msg_rebooting:'Red\u00e9marrage...',
msg_none:'(aucune)',msg_welcome:'WEB DMD CONFIG',btn_resume:'Reprendre DMD',footer:'RecalBox DMD v7 \u2014 Interface web \u2014 Shan_ayA 2026'
},
en:{
page_title:'RecalBox DMD Configuration',app_title:'RecalBox DMD Config',
sec_display:'Display',sec_playlist:'Playlist',sec_genplaylist:'Playlist Generator',
sec_upload:'GIF Upload',sec_wifi:'Wi-Fi',sec_bt:'Bluetooth',sec_mqtt:'MQTT',sec_clock:'Clock',
lbl_brightness:'Brightness (%)',lbl_silent_boot:'Silent boot',lbl_playlist_file:'Playlist file',lbl_random:'Random',lbl_delete_playlist:'Delete',
lbl_gen_name:'List name',lbl_upload_dir:'Target folder',lbl_upload_file:'.gif file',
lbl_wifi_enabled:'Enabled',lbl_wifi_password:'Password',lbl_wifi_static:'Static IP',
lbl_wifi_ip:'Fixed IP',lbl_bt_enabled:'Enabled',lbl_bt_name:'Name',lbl_mqtt_ip:'Recalbox IP',
lbl_clock_enabled:'Enabled',lbl_clock_theme:'Theme',
lbl_clock_neon_color:'Neon color',lbl_clock_neon_color_enabled:'Custom',
lbl_clock_interval_gifs:'Interval (GIFs)',lbl_clock_interval_min:'Interval (min)',
lbl_clock_duration:'Duration (sec)',lbl_clock_tz:'Timezone',
tip_brightness:'DMD screen brightness (0 = off, 100 = max)',
tip_silent_boot:'If checked: hides startup screens (WiFi, NTP...), only shows the title and an hourglass',
tip_playlist_file:'Active playlist shown in loop. .txt files in /playlists/',
tip_random:'If checked: random playback. Otherwise: playlist file order',
tip_gen_name:'.txt filename to create in /playlists/ (without extension)',
tip_delete_select:'Select a playlist to delete (.txt file + associated caches)',
tip_upload_dir:'Subfolder name in /gifs/ (e.g. NES, MAME, do not use /)',
tip_upload_file:'Select a .gif file on your computer (max ~2 MB)',
tip_wifi_enabled:'Enable or disable Wi-Fi (and web server / MQTT)',
tip_wifi_ssid:'Wi-Fi network name (e.g. Livebox-XXXX, Orange-XXXX)',
tip_wifi_password:'WPA2 security key of the Wi-Fi network',
tip_wifi_static:'If checked: use a fixed IP instead of DHCP',
tip_wifi_ip:'Fixed IP address (e.g. 192.168.0.100) \u2014 required if static IP is enabled',
tip_wifi_gateway:'Default gateway (e.g. 192.168.0.1)',
tip_wifi_subnet:'Subnet mask (e.g. 255.255.255.0)',
tip_wifi_dns1:'Primary DNS server (e.g. 8.8.8.8)',
tip_wifi_dns2:'Secondary DNS server (e.g. 8.8.4.4)',
tip_bt_enabled:'Enable or disable Bluetooth (name visible on phone/tablet)',
tip_bt_name:'Bluetooth name shown during scan (e.g. ESP32-DMD)',
tip_mqtt_ip:'Recalbox IP address (to receive MQTT commands)',
tip_clock_enabled:'Shows retro clock between games (multiple pixel-art themes)',
tip_clock_theme:'Clock theme: -1 = random, 0-8 = fixed theme (Mario, Tetris, Pac-Man...)',
tip_clock_neon_color:'Base color for the Neon theme (applies to that theme only)',
hint_clock_neon_color:'Neon theme only; the colon’s hue is computed automatically (inverse)',
tip_clock_interval_gifs:'Number of games (GIFs) shown before the clock reappears (e.g. 10 = clock every 10 games)',
tip_clock_interval_min:'Minutes between each clock (0 = disabled, uses GIF interval)',
tip_clock_duration:'Clock display duration in seconds before resuming games',
tip_clock_tz:'Pick your country or a fixed UTC offset',
btn_generate:'Generate',btn_delete:'Delete selection',
btn_upload:'Upload',btn_save:'Save',btn_save_reboot:'Save & Reboot',btn_delete_playlist:'Delete playlist',btn_del_selection:'Delete selected',btn_reboot:'Reboot',
gen_desc:'Check folders \uD83D\uDCC1 then give a name to create a playlist containing all .gif files from those folders.',
upload_desc:'Add a .gif file directly from your browser into a folder in /gifs/. If the folder does not exist it will be created.',
placeholder_gen_name:'e.g. MyPlaylist',placeholder_upload_dir:'e.g. NES',
hint_interval_min:'0 = disabled',
opt_random:'Random',opt_mario:'Mario',opt_tetris:'Tetris',opt_pacman:'Pac-Man',
opt_spaceinv:'Space Invaders',opt_pong:'Pong',opt_neon:'Neon',opt_matrix:'Matrix',
opt_fire:'Fire',opt_rainbow:'Rainbow',opt_level11:'Level 1-1',
opt_tz_ce:'France / Spain / Germany / Italy',opt_tz_uk:'England (UK) / Portugal',
opt_tz_usa_e:'USA - East (New York)',opt_tz_usa_c:'USA - Central (Chicago)',
opt_tz_usa_m:'USA - Mountain (Denver)',opt_tz_usa_p:'USA - Pacific (Los Angeles)',
opt_tz_ee:'Greece / Romania / Finland',
msg_network_error:'Network error',msg_no_playlist_name:'Please name the playlist',msg_del_folder_confirm:'Delete folders',msg_upload_ok:'file(s) uploaded successfully',
msg_select_folder:'Select at least one folder',
msg_select_playlist_to_del:'Select a playlist to delete',
msg_confirm_delete:'Delete ${0} (including cache files)?',
msg_specify_dir:'Please specify a target folder in /gifs/',
msg_select_file:'Please select a .gif file',
msg_only_gif:'Only .gif files are accepted',
msg_uploading:'Uploading...',msg_upload_error:'Upload error',
msg_load_error:'Config load error',msg_no_dirs:'No folders in /gifs/',
msg_confirm_reboot:'Reboot the ESP32 ?',msg_rebooting:'Rebooting...',
msg_none:'(none)',msg_welcome:'WEB DMD CONFIG',btn_resume:'Resume DMD',footer:'RecalBox DMD v7 \u2014 Web interface \u2014 Shan_ayA 2026'
},
es:{
page_title:'Configuraci\u00f3n de RecalBox DMD',app_title:'RecalBox DMD Config',
sec_display:'Pantalla',sec_playlist:'Lista de reproducci\u00f3n',sec_genplaylist:'Generador de listas',
sec_upload:'Subir GIF',sec_wifi:'Wi-Fi',sec_bt:'Bluetooth',sec_mqtt:'MQTT',sec_clock:'Reloj',
lbl_brightness:'Brillo (%)',lbl_silent_boot:'Arranque silencioso',lbl_playlist_file:'Archivo de lista',lbl_random:'Aleatorio',lbl_delete_playlist:'Eliminar',
lbl_gen_name:'Nombre de la lista',lbl_upload_dir:'Carpeta destino',lbl_upload_file:'Archivo .gif',
lbl_wifi_enabled:'Activado',lbl_wifi_password:'Contrase\u00f1a',lbl_wifi_static:'IP est\u00e1tica',
lbl_wifi_ip:'IP fija',lbl_bt_enabled:'Activado',lbl_bt_name:'Nombre',lbl_mqtt_ip:'IP de Recalbox',
lbl_clock_enabled:'Activado',lbl_clock_theme:'Tema',
lbl_clock_neon_color:'Color Neon',lbl_clock_neon_color_enabled:'Personalizado',
lbl_clock_interval_gifs:'Intervalo (GIFs)',lbl_clock_interval_min:'Intervalo (min)',
lbl_clock_duration:'Duraci\u00f3n (seg)',lbl_clock_tz:'Zona horaria',
tip_brightness:'Brillo de la pantalla DMD (0 = apagado, 100 = m\u00e1ximo)',
tip_silent_boot:'Si est\u00e1 marcado: oculta las pantallas de arranque (WiFi, NTP...), solo muestra el t\u00edtulo y un reloj de arena',
tip_playlist_file:'Lista activa mostrada en bucle. Archivos .txt en /playlists/',
tip_random:'Si est\u00e1 marcado: reproducci\u00f3n aleatoria. Si no: orden del archivo',
tip_gen_name:'Nombre del archivo .txt a crear en /playlists/ (sin extensi\u00f3n)',
tip_delete_select:'Selecciona una lista para eliminar (archivo .txt + cach\u00e9s asociados)',
tip_upload_dir:'Nombre de subcarpeta en /gifs/ (ej: NES, MAME, no usar /)',
tip_upload_file:'Selecciona un archivo .gif en tu ordenador (m\u00e1x ~2 MB)',
tip_wifi_enabled:'Activa o desactiva el Wi-Fi (y el servidor web / MQTT)',
tip_wifi_ssid:'Nombre de la red Wi-Fi (ej: Livebox-XXXX, Orange-XXXX)',
tip_wifi_password:'Clave de seguridad WPA2 de la red Wi-Fi',
tip_wifi_static:'Si est\u00e1 marcado: usar IP fija en lugar de DHCP',
tip_wifi_ip:'Direcci\u00f3n IP fija (ej: 192.168.0.100) \u2014 requerido si IP est\u00e1tica est\u00e1 activa',
tip_wifi_gateway:'Puerta de enlace predeterminada (ej: 192.168.0.1)',
tip_wifi_subnet:'M\u00e1scara de subred (ej: 255.255.255.0)',
tip_wifi_dns1:'Servidor DNS primario (ej: 8.8.8.8)',
tip_wifi_dns2:'Servidor DNS secundario (ej: 8.8.4.4)',
tip_bt_enabled:'Activa o desactiva el Bluetooth (nombre visible en tel\u00e9fono/tableta)',
tip_bt_name:'Nombre Bluetooth mostrado durante el escaneo (ej: ESP32-DMD)',
tip_mqtt_ip:'Direcci\u00f3n IP de la Recalbox (para recibir comandos MQTT)',
tip_clock_enabled:'Muestra el reloj retro entre juegos (varios temas pixel-art)',
tip_clock_theme:'Tema del reloj: -1 = aleatorio, 0-8 = tema fijo (Mario, Tetris, Pac-Man...)',
tip_clock_neon_color:'Color base del tema Neon (solo se aplica a ese tema)',
hint_clock_neon_color:'Solo tema Neon; el tono de los dos puntos se calcula autom\u00e1ticamente (inverso)',
tip_clock_interval_gifs:'N\u00famero de juegos (GIFs) antes de que el reloj reaparezca (ej: 10 = reloj cada 10 juegos)',
tip_clock_interval_min:'Minutos entre cada reloj (0 = desactivado, usa intervalo en GIFs)',
tip_clock_duration:'Duraci\u00f3n del reloj en segundos antes de reanudar los juegos',
tip_clock_tz:'Elige tu pa\u00eds o un desfase UTC fijo',
btn_generate:'Generar',btn_delete:'Eliminar selecci\u00f3n',
btn_upload:'Subir',btn_save:'Guardar',btn_save_reboot:'Guardar y Reiniciar',btn_delete_playlist:'Eliminar lista',btn_del_selection:'Eliminar selecci\u00f3n',btn_reboot:'Reiniciar',
gen_desc:'Marca las carpetas \uD83D\uDCC1 y da un nombre para crear una lista con todos los .gif de esas carpetas.',
upload_desc:'A\u00f1ade un archivo .gif desde tu navegador a una carpeta en /gifs/. Si la carpeta no existe se crear\u00e1.',
placeholder_gen_name:'ej: MiLista',placeholder_upload_dir:'ej: NES',
hint_interval_min:'0 = desactivado',
opt_random:'Aleatorio',opt_mario:'Mario',opt_tetris:'Tetris',opt_pacman:'Pac-Man',
opt_spaceinv:'Space Invaders',opt_pong:'Pong',opt_neon:'Neon',opt_matrix:'Matrix',
opt_fire:'Fire',opt_rainbow:'Rainbow',opt_level11:'Level 1-1',
opt_tz_ce:'Francia / España / Alemania / Italia',opt_tz_uk:'Inglaterra (UK) / Portugal',
opt_tz_usa_e:'EE.UU. - Este (Nueva York)',opt_tz_usa_c:'EE.UU. - Centro (Chicago)',
opt_tz_usa_m:'EE.UU. - Montañas (Denver)',opt_tz_usa_p:'EE.UU. - Pacífico (Los Ángeles)',
opt_tz_ee:'Grecia / Rumanía / Finlandia',
msg_network_error:'Error de red',msg_no_playlist_name:'Pon nombre a la lista',msg_del_folder_confirm:'Eliminar carpetas',msg_upload_ok:'archivo(s) subido(s) correctamente',
msg_select_folder:'Selecciona al menos una carpeta',
msg_select_playlist_to_del:'Selecciona una lista para eliminar',
msg_confirm_delete:'\u00bfEliminar ${0} (incluyendo archivos de cach\u00e9)?',
msg_specify_dir:'Especifica una carpeta destino en /gifs/',
msg_select_file:'Selecciona un archivo .gif',
msg_only_gif:'Solo se aceptan archivos .gif',
msg_uploading:'Subiendo...',msg_upload_error:'Error de subida',
msg_load_error:'Error al cargar configuraci\u00f3n',msg_no_dirs:'No hay carpetas en /gifs/',
msg_confirm_reboot:'\u00bfReiniciar el ESP32 ?',msg_rebooting:'Reiniciando...',
msg_none:'(ninguna)',msg_welcome:'WEB DMD CONFIG',btn_resume:'Reanudar DMD',footer:'RecalBox DMD v7 \u2014 Interfaz web \u2014 Shan_ayA 2026'
}
};
let currentLang='fr';
function tr(key){return I18N[currentLang]&&I18N[currentLang][key]!==undefined?I18N[currentLang][key]:I18N.fr[key]!==undefined?I18N.fr[key]:key;}
function trTpl(key,...args){let s=tr(key);args.forEach((v,i)=>{s=s.replace('${'+i+'}',v);});return s;}
function applyLang(){
let stored=localStorage.getItem('dmd_lang');
if(stored&&I18N[stored]){currentLang=stored;}else{
const nav=(navigator.language||'').substring(0,2);
currentLang=I18N[nav]?nav:'fr';}
document.documentElement.lang=currentLang;
document.querySelectorAll('[data-i18n]:not([data-i18n-ignore])').forEach(el=>el.innerHTML=tr(el.dataset.i18n));
document.querySelectorAll('[data-i18n-title]').forEach(el=>el.title=tr(el.dataset.i18nTitle));
document.querySelectorAll('[data-i18n-placeholder]').forEach(el=>el.placeholder=tr(el.dataset.i18nPlaceholder));
document.getElementById('langSelect').innerHTML='';
Object.keys(LANGUAGES).forEach(code=>{
const o=document.createElement('option');o.value=code;o.textContent=LANGUAGES[code];
if(code===currentLang)o.selected=true;document.getElementById('langSelect').appendChild(o);
});
}
function setLang(code){localStorage.setItem('dmd_lang',code);applyLang();}
// ============= END I18N =============

function stripAccents(s){return s.normalize('NFD').replace(/[\u0300-\u036f]/g,'').replace(/[^ -~]/g,'?');}
function showMsg(text, isOk) {
  const el = document.getElementById('msg');
  el.textContent = text;
  el.className = 'msg ' + (isOk ? 'msg-ok' : 'msg-err');
  el.style.display = 'block';
  el.scrollIntoView({behavior:'smooth',block:'center'});
  if(window._msgTimer)clearTimeout(window._msgTimer);
  window._msgTimer=setTimeout(()=>{el.style.display='none';},8000);
  fetch('/dmd-pause',{method:'POST',body:new URLSearchParams({msg:stripAccents(text),color:isOk?'1':'2'}),headers:{'Content-Type':'application/x-www-form-urlencoded'}}).catch(()=>{});
}
function serialize() {
  const f=(id)=>document.getElementById(id);
  const v=(id)=>f(id).value.trim();
  const c=(id)=>f(id).checked?'1':'0';
  return new URLSearchParams({
    brightness: f('brightness').value, info: f('silent_boot').checked?'0':'1', playlist: v('playlist'), random: c('random'),
    wifi_enabled: c('wifi_enabled'), wifi_ssid: v('wifi_ssid'), wifi_password: v('wifi_password'),
    wifi_static_enabled: c('wifi_static_enabled'), wifi_static_ip: v('wifi_static_ip'),
    wifi_gateway: v('wifi_gateway'), wifi_subnet: v('wifi_subnet'),
    wifi_dns1: v('wifi_dns1'), wifi_dns2: v('wifi_dns2'),
    bluetooth_enabled: c('bluetooth_enabled'), bluetooth_name: v('bluetooth_name'),
    recalbox_ip: v('recalbox_ip'),
    clock_enabled: c('clock_enabled'), clock_theme: v('clock_theme'),
    clock_neon_color: v('clock_neon_color'), clock_neon_color_enabled: c('clock_neon_color_enabled'),
    clock_interval: v('clock_interval'), clock_interval_min: v('clock_interval_min'),
    clock_duration: v('clock_duration'), clock_tz: v('clock_tz')
  });
}
function saveConfig(e) {
  e.preventDefault();
  fetch('/save',{method:'POST',body:serialize(),headers:{'Content-Type':'application/x-www-form-urlencoded'}})
    .then(r=>r.text()).then(t=>showMsg(t,t.includes('OK'))).catch(()=>showMsg(tr('msg_network_error'),false));
}
function saveAndReboot() {
  fetch('/save',{method:'POST',body:serialize(),headers:{'Content-Type':'application/x-www-form-urlencoded'}})
    .then(r=>r.text()).then(t=>{showMsg(t,t.includes('OK'));if(t.includes('OK'))setTimeout(()=>fetch('/reboot').catch(()=>{}),1000);})
    .catch(()=>showMsg(tr('msg_network_error'),false));
}
function doReboot() {
  if(!confirm(tr('msg_confirm_reboot')))return;
  showMsg(tr('msg_rebooting'),true);
  fetch('/reboot').catch(()=>{});
}
function refreshPlaylistSelect(sid,selVal){
  fetch('/lsplaylists').then(r=>{if(!r.ok)throw Error(r.status);return r.json();}).then(pl=>{
    const sel=document.getElementById(sid);if(!sel)return;sel.innerHTML='';
    if(sid==='deletePlaylistSelect'){const o=document.createElement('option');o.value='';o.textContent='---';o.disabled=true;o.selected=true;sel.appendChild(o);}
    pl.forEach(p=>{const o=document.createElement('option');o.value=p;o.textContent=p;if(p===selVal)o.selected=true;sel.appendChild(o);});
    if(!pl.length){const o=document.createElement('option');o.value='';o.textContent=tr('msg_none');sel.appendChild(o);}
  }).catch(()=>{
    const sel=document.getElementById(sid);if(!sel)return;
    sel.innerHTML='<option value="">\u26A0 '+tr('msg_network_error')+'</option>';
  });
}
function refreshGifDirs(){
  fetch('/lsgifdirs').then(r=>r.json()).then(dirs=>{
    const c=document.getElementById('genDirs');c.innerHTML='';
    const sel=document.getElementById('uploadDirSelect');
    if(!dirs.length){
      c.innerHTML='<div style="color:#666">'+tr('msg_no_dirs')+'</div>';
      if(sel){sel.innerHTML='';const o=document.createElement('option');o.value='';o.textContent='---';sel.appendChild(o);}
      document.getElementById('fileList').style.display='none';
      return;
    }
    dirs.forEach(name=>{
      const div=document.createElement('div');div.className='gen-folder';
      div.id='gf_'+name;
      div.innerHTML='<input type="checkbox" value="'+name+'"><span>&#x1F4C1; '+name+'</span> <span style="cursor:pointer;color:#4fc3f7;font-weight:bold;font-size:16px" onclick="event.stopPropagation();openFolder(\''+name+'\')">[+]</span>';
      div.addEventListener('dblclick',function(){
        const n=this.id.replace('gf_','');
        openFolder(n);
      });
      c.appendChild(div);
    });
    if(sel){
      sel.innerHTML='';const o=document.createElement('option');o.value='';o.textContent='---';sel.appendChild(o);
      dirs.forEach(name=>{const o=document.createElement('option');o.value=name;o.textContent=name;sel.appendChild(o);});
    }
  }).catch(()=>{});
}
let _currentFolder='';
function openFolder(name){
  _currentFolder=name;
  const fl=document.getElementById('fileList');fl.style.display='block';
  fl.innerHTML='<div style="margin-bottom:6px"><a href="#" onclick="closeFolder();return false" style="color:#4fc3f7">&#x25C0; Retour</a> <b style="margin-left:8px">&#x1F4C1; '+name+'</b></div>';
  fetch('/lsgiffiles?dir='+encodeURIComponent(name)).then(r=>r.json()).then(files=>{
    if(!files||!files.length){
      fl.innerHTML+='<div style="color:#666">'+tr('msg_no_dirs')+'</div>';return;
    }
    files.forEach(f=>{
      const fname=f.replace(/^.*\//,'');
      fl.innerHTML+='<div class="gen-folder"><input type="checkbox" value="'+fname+'"><span>&#x1F5C4; '+fname+'</span></div>';
    });
  }).catch(()=>{fl.innerHTML+='<div style="color:#666">'+tr('msg_network_error')+'</div>';});
}
function closeFolder(){
  _currentFolder='';
  document.getElementById('fileList').style.display='none';
}
function deleteSelected(){
  const fl=document.getElementById('fileList');
  if(fl.style.display!='none'&&_currentFolder.length>0){
    const files=[];fl.querySelectorAll('input:checked').forEach(cb=>files.push(cb.value));
    if(!files.length){showMsg(tr('msg_select_file'),false);return;}
    if(!confirm('Supprimer '+files.length+' fichier(s) de '+_currentFolder+' ?'))return;
    dmdPause(tr('btn_del_folders'));
    const params=new URLSearchParams({dir:_currentFolder,files:files.join(',')});
    fetch('/delete-files',{method:'POST',body:params,headers:{'Content-Type':'application/x-www-form-urlencoded'}})
      .then(r=>r.text()).then(t=>{showMsg(t,t.includes('OK'));if(t.includes('OK'))openFolder(_currentFolder);})
      .catch(()=>{showMsg(tr('msg_network_error'),false);});
  } else {
    const dirs=[];document.querySelectorAll('#genDirs input:checked').forEach(cb=>dirs.push(cb.value));
    if(!dirs.length){showMsg(tr('msg_select_folder'),false);return;}
    if(!confirm(tr('msg_del_folder_confirm')+' ('+dirs.join(', ')+') ?'))return;
    dmdPause(tr('btn_del_folders'));
    fetch('/delete-folders',{method:'POST',body:new URLSearchParams({dirs:dirs.join(',')}),headers:{'Content-Type':'application/x-www-form-urlencoded'}})
      .then(r=>r.text()).then(t=>{showMsg(t,t.includes('OK'));if(t.includes('OK')){closeFolder();refreshGifDirs();}})
      .catch(()=>{showMsg(tr('msg_network_error'),false);});
  }
}
function dmdPause(msg){
  showMsg(msg,true);
}
function dmdResume(){
  fetch('/dmd-resume',{method:'POST'}).then(r=>r.text()).then(t=>{
    const el=document.getElementById('msg');
    el.textContent=t;el.className='msg '+(t.includes('OK')?'msg-ok':'msg-err');
    el.style.display='block';el.scrollIntoView({behavior:'smooth',block:'center'});
    if(window._msgTimer)clearTimeout(window._msgTimer);
    window._msgTimer=setTimeout(()=>{el.style.display='none';},8000);
  }).catch(()=>{});
}

function selectAllDirs(select){document.querySelectorAll('#genDirs input[type=checkbox]').forEach(function(cb){cb.checked=select;});}
function genPlaylist(){
  const name=document.getElementById('genName').value.trim();
  if(!name){showMsg(tr('msg_no_playlist_name'),false);return;}
  const dirs=[];document.querySelectorAll('#genDirs input:checked').forEach(cb=>dirs.push(cb.value));
  if(!dirs.length){showMsg(tr('msg_select_folder'),false);return;}
  // Message persistant (pas d'auto-hide)
  const el=document.getElementById('msg');
  el.textContent='Generation playlist...';el.className='msg msg-ok';el.style.display='block';
  if(window._msgTimer)clearTimeout(window._msgTimer);
  fetch('/generate-playlist',{method:'POST',body:new URLSearchParams({name,dirs:dirs.join(',')}),headers:{'Content-Type':'application/x-www-form-urlencoded'}})
    .then(r=>r.text()).then(t=>{showMsg(t,t.includes('OK'));if(t.includes('OK')){refreshPlaylistSelect('playlist',name+'.txt');refreshPlaylistSelect('deletePlaylistSelect','');}})
    .catch(()=>{showMsg(tr('msg_network_error'),false);});
}
function deletePlaylist(){
  const sel=document.getElementById('deletePlaylistSelect');
  const name=sel.options[sel.selectedIndex]?.value;
  if(!name){showMsg(tr('msg_select_playlist_to_del'),false);return;}
  if(!confirm(trTpl('msg_confirm_delete',name)))return;
  dmdPause(tr('btn_delete_playlist'));
  fetch('/delete-playlist',{method:'POST',body:new URLSearchParams({name}),headers:{'Content-Type':'application/x-www-form-urlencoded'}})
    .then(r=>r.text()).then(t=>{showMsg(t,t.includes('OK'));if(t.includes('OK')){refreshPlaylistSelect('playlist','');refreshPlaylistSelect('deletePlaylistSelect','');}})
    .catch(()=>{showMsg(tr('msg_network_error'),false);});
}
async function doUpload(){
  const sel=document.getElementById('uploadDirSelect');
  const custom=document.getElementById('uploadDirCustom').value.trim();
  const dir=sel.value||custom;
  if(!dir){showMsg(tr('msg_specify_dir'),false);return;}
  const fileInput=document.getElementById('uploadFile');
  if(!fileInput.files.length){showMsg(tr('msg_select_file'),false);return;}
  const files=Array.from(fileInput.files).filter(f=>f.name.toLowerCase().endsWith('.gif'));
  if(!files.length){showMsg(tr('msg_only_gif'),false);return;}
  const bar=document.getElementById('uploadProgress');bar.style.display='block';
  const barInner=document.getElementById('uploadProgressBar');
  const fileList=document.getElementById('uploadFileList');
  fileList.textContent='';
  // Message persistant (ne s'auto-efface pas)
  const msgEl=document.getElementById('msg');
  msgEl.className='msg msg-ok';msgEl.style.display='block';
  msgEl.textContent=tr('msg_uploading');
  let okCount=0;
  for(let i=0;i<files.length;i++){
    const file=files[i];
    const pct=Math.round(((i+1)/files.length)*100);
    barInner.style.width=Math.max(pct,5)+'%';
    fileList.textContent=file.name+' ('+(i+1)+'/'+files.length+')';
    msgEl.textContent=tr('msg_uploading')+' '+file.name;
    try{await fetch('/dmd-pause',{method:'POST',body:new URLSearchParams({msg:stripAccents(file.name+' ('+(i+1)+'/'+files.length+')'),color:'1'}),headers:{'Content-Type':'application/x-www-form-urlencoded'}});}catch(_){}
    const formData=new FormData();
    formData.append('dir',dir);
    formData.append('file',file);
    try{
      const r=await fetch('/upload',{method:'POST',body:formData});
      const t=await r.text();
      if(t.includes('OK')){
        okCount++;
        fileList.textContent=file.name+' OK ('+okCount+'/'+files.length+')';
      } else {
        msgEl.className='msg msg-err';msgEl.textContent=file.name+': '+t;
      }
    }catch(e){
      msgEl.className='msg msg-err';msgEl.textContent=file.name+': '+tr('msg_upload_error');
    }
  }
  barInner.style.width='100%';
  fileList.textContent='';
  refreshGifDirs();
  if(_currentFolder.length>0)openFolder(_currentFolder);
  const result=okCount+'/'+files.length+' '+tr('msg_upload_ok');
  showMsg(result,okCount===files.length);
}
applyLang();
fetch('/dmd-open',{method:'POST',body:new URLSearchParams({msg:stripAccents(tr('msg_welcome'))}),headers:{'Content-Type':'application/x-www-form-urlencoded'}})
  .then(()=>{const el=document.getElementById('msg');el.textContent=tr('msg_welcome');el.className='msg msg-ok';el.style.display='block';setTimeout(()=>{el.style.display='none';},4000);})
  .catch(()=>{});
fetch('/load').then(r=>r.json()).then(d=>{
  document.getElementById('brightness').value=d.brightness;document.getElementById('bval').textContent=d.brightness;
  document.getElementById('silent_boot').checked=d.info=='0';
  refreshPlaylistSelect('playlist',d.playlist);refreshPlaylistSelect('deletePlaylistSelect','');
  document.getElementById('random').checked=d.random=='1';
  document.getElementById('wifi_enabled').checked=d.wifi_enabled=='1';
  document.getElementById('wifi_ssid').value=d.wifi_ssid;document.getElementById('wifi_password').value=d.wifi_password;
  document.getElementById('wifi_static_enabled').checked=d.wifi_static_enabled=='1';
  document.getElementById('wifi_static_ip').value=d.wifi_static_ip;document.getElementById('wifi_gateway').value=d.wifi_gateway;
  document.getElementById('wifi_subnet').value=d.wifi_subnet;
  document.getElementById('wifi_dns1').value=d.wifi_dns1;document.getElementById('wifi_dns2').value=d.wifi_dns2;
  document.getElementById('bluetooth_enabled').checked=d.bluetooth_enabled=='1';
  document.getElementById('bluetooth_name').value=d.bluetooth_name;
  document.getElementById('recalbox_ip').value=d.recalbox_ip;
  document.getElementById('clock_enabled').checked=d.clock_enabled=='1';
  document.getElementById('clock_theme').value=d.clock_theme;document.getElementById('clock_interval').value=d.clock_interval;
  document.getElementById('clock_interval_min').value=d.clock_interval_min;document.getElementById('clock_duration').value=d.clock_duration;
  document.getElementById('clock_tz').value=d.clock_tz;
  if(d.clock_neon_color) document.getElementById('clock_neon_color').value=d.clock_neon_color;
  document.getElementById('clock_neon_color_enabled').checked=d.clock_neon_color_enabled=='1';
}).catch(()=>showMsg(tr('msg_load_error'),false));
refreshGifDirs();
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
<title>RecalBox DMD - Configuration WiFi</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:'Segoe UI',Tahoma,sans-serif;background:#1a1a2e;color:#eee;padding:16px;max-width:500px;margin:auto;display:flex;flex-direction:column;min-height:100vh;justify-content:center}
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
</style>
</head>
<body>
<h1>&#x1F4E1; Configuration WiFi</h1>
<div class="hint">Connectez-vous au r&eacute;seau <b>RecalBox-DMD-Config</b> puis s&eacute;lectionnez votre WiFi.</div>
<div id="msg" class="msg"></div>
<div class="section">
<div class="row"><label>R&eacute;seau WiFi</label>
<select id="wifi_ssid" style="width:100%"><option value="">-- Scan en cours... --</option></select>
<button class="btn-scan" onclick="scanWiFi()">&#x1F50D; Scanner les r&eacute;seaux</button>
<input type="text" id="wifi_ssid_text" placeholder="Ou saisir le nom manuellement" style="width:100%;margin-top:4px;padding:8px 10px;border:1px solid #555;border-radius:4px;background:#0f3460;color:#eee;font-size:14px">
</div>
<div class="row"><label>Mot de passe</label>
<div class="pwd-row"><input type="password" id="wifi_password" placeholder="Mot de passe WiFi"><button class="pwd-toggle" id="pwdToggle" onclick="togglePwd()">&#x1F441;</button></div>
</div>
<div class="row"><label>IP statique (optionnel)</label><input type="text" id="wifi_static_ip" placeholder="Laisser vide pour DHCP"></div>
<div class="btn-row"><button class="btn" onclick="saveWiFi()">&#x1F504; Sauvegarder &amp; Red&eacute;marrer</button></div>
</div>
<script>
function showMsg(t,ok){var e=document.getElementById('msg');e.textContent=t;e.className='msg '+(ok?'msg-ok':'msg-err');e.style.display='block';setTimeout(function(){e.style.display='none';},5000);}
function togglePwd(){var p=document.getElementById('wifi_password');p.type=(p.type=='password'?'text':'password');}
function scanWiFi(){
  var sel=document.getElementById('wifi_ssid');sel.innerHTML='<option value="">Scan...</option>';
  fetch('/scan-wifi').then(function(r){return r.json();}).then(function(nets){
    sel.innerHTML='<option value="">-- S&eacute;lectionnez --</option>';
    if(nets&&nets.length) nets.forEach(function(n){sel.innerHTML+='<option value="'+n+'">'+n+'</option>';});
    else sel.innerHTML='<option value="">Aucun r&eacute;seau trouv&eacute;</option>';
  }).catch(function(){sel.innerHTML='<option value="">Erreur scan</option>';});
}
scanWiFi();
function saveWiFi(){
  var sel=document.getElementById('wifi_ssid');
  var txt=document.getElementById('wifi_ssid_text');
  var ssid=sel.value||txt.value.trim();
  if(!ssid){showMsg('Veuillez selectionner ou saisir un reseau WiFi',false);return;}
  var pwd=document.getElementById('wifi_password').value.trim();
  var ip=document.getElementById('wifi_static_ip').value.trim();
  var body='wifi_enabled=1&wifi_ssid='+encodeURIComponent(ssid)+'&wifi_password='+encodeURIComponent(pwd);
  body+='&wifi_static_enabled='+(ip?1:0)+'&wifi_static_ip='+encodeURIComponent(ip);
  showMsg('Enregistrement...',true);
  fetch('/save-ap',{method:'POST',body:body,headers:{'Content-Type':'application/x-www-form-urlencoded'}})
    .then(function(r){return r.text();})
    .then(function(t){if(t.includes('OK'))showMsg('Redemarrage...',true);else showMsg(t,false);})
    .catch(function(){showMsg('Erreur reseau',false);});
}
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
  webServer->send(200, "application/json", json);
}

static void handleWebConfigListPlaylists()
{
  String json = "[";
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
        json += "\"" + name + "\""; first = false;
      }
      entry.close(); entry = dir.openNextFile();
      delay(1);
    }
    dir.close();
  }
  json += "]";
  webServer->send(200, "application/json", json);
}

static void handleWebConfigListGifDirs()
{
  String json = "[";
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
        first = false;
      }
      entry.close(); entry = dir.openNextFile();
      delay(1);
    }
    dir.close();
  }
  json += "]";
  webServer->send(200, "application/json", json);
}

static void handleWebConfigListGifFiles()
{
  if (!webServer->hasArg("dir")) { webServer->send(400, "application/json", "[]"); return; }
  String dirName = webServer->arg("dir"); dirName.trim();
  String json = "[";
  File sub = SD.open(("/gifs/" + dirName).c_str());
  if (sub && sub.isDirectory()) {
    bool first = true;
    File f = sub.openNextFile();
    while (f) {
      String fn = String(f.name());
      if (!f.isDirectory() && fn.endsWith(".gif")) {
        if (!first) json += ",";
        json += "\"" + fn + "\"";
        first = false;
      }
      f.close(); f = sub.openNextFile();
      delay(1);
    }
    sub.close();
  }
  json += "]";
  webServer->send(200, "application/json", json);
}

static void handleWebConfigGifCount()
{
  if (!webServer->hasArg("dir")) { webServer->send(400, "text/plain", "0"); return; }
  String dirName = webServer->arg("dir"); dirName.trim();
  int count = 0;
  File sub = SD.open(("/gifs/" + dirName).c_str());
  if (sub && sub.isDirectory()) {
    File f = sub.openNextFile();
    while (f) {
      if (!f.isDirectory() && String(f.name()).endsWith(".gif")) count++;
      f.close(); f = sub.openNextFile();
      delay(1);
    }
    sub.close();
  }
  webServer->send(200, "text/plain", String(count));
}

static void handleWebConfigGeneratePlaylist()
{
  if (!webServer->hasArg("name") || !webServer->hasArg("dirs")) {
    webServer->send(400, "text/plain", "ERR: manque nom ou dirs"); return;
  }
  String name = webServer->arg("name");
  String dirs = webServer->arg("dirs");
  String outputPath = "/playlists/" + name + ".txt";
  if (!SD.exists("/playlists")) SD.mkdir("/playlists");
  if (SD.exists(outputPath.c_str())) SD.remove(outputPath.c_str());
  File outf = SD.open(outputPath.c_str(), FILE_WRITE);
  if (!outf) { webServer->send(500, "text/plain", "ERR: ecriture impossible"); return; }
  String buf;
  // Compter d'abord le nombre de dossiers pour la progression
  int totalDirs = 1, processedDirs = 0;
  for (int i = 0; i < dirs.length(); i++) if (dirs.charAt(i) == ',') totalDirs++;
  int totalGifs = 0, startIdx = 0;
  while (true) {
    int comma = dirs.indexOf(',', startIdx);
    String dirName = (comma < 0) ? dirs.substring(startIdx) : dirs.substring(startIdx, comma);
    dirName.trim();
    if (dirName.length() > 0) {
      processedDirs++;
      webDmdPause("Scan: " + dirName + " (" + String(processedDirs) + "/" + String(totalDirs) + ")", 0x07E0);
      File sysDir = SD.open(("/gifs/" + dirName).c_str());
      if (sysDir && sysDir.isDirectory()) {
        File f = sysDir.openNextFile();
        while (f) {
          String fname = String(f.name());
          if (!f.isDirectory() && fname.endsWith(".gif")) {
            buf += "/gifs/" + dirName + "/" + fname + "\n";
            totalGifs++;
            if (buf.length() > 4000) { outf.print(buf); buf = ""; }
          }
          f.close(); f = sysDir.openNextFile();
          delay(1);
        }
      }
      if (sysDir) sysDir.close();
    }
    if (comma < 0) break;
    startIdx = comma + 1;
    delay(1);
  }
  if (buf.length() > 0) outf.print(buf);
  outf.close();
  webDmdPause("Playlist creee: " + String(totalGifs) + " GIFs", 0x07E0);
  String msg = "OK: " + String(totalGifs) + " GIFs ajoutes dans la playlist " + name + ".txt";
  Serial.println("[WEB] " + msg);
  webServer->send(200, "text/plain", msg);
}

static void handleWebConfigDeletePlaylist()
{
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
  String msg = "OK: " + String(deleted) + " fichiers supprimes pour " + name;
  Serial.println("[WEB] " + msg);
  webServer->send(200, "text/plain", msg);
}

// Ajoute un fichier a toutes les playlists contenant deja des fichiers du meme dossier
static void addFileToPlaylists(const String &folder, const String &fileName)
{
  if (folder.length() == 0 || fileName.length() == 0) return;
  String gifPath = "/gifs/" + folder + "/" + fileName;
  int appended = 0;
  File plDir = SD.open("/playlists");
  if (!plDir || !plDir.isDirectory()) { if (plDir) plDir.close(); return; }
  File entry = plDir.openNextFile();
  while (entry) {
    String name = String(entry.name());
    int slash = name.lastIndexOf('/');
    String base = (slash >= 0) ? name.substring(slash + 1) : name;
    if (!entry.isDirectory() && base.endsWith(".txt")) {
      String plPath = "/playlists/" + base;
      bool found = false;
      File pl = SD.open(plPath.c_str());
      if (pl) {
        while (pl.available()) {
          String line = pl.readStringUntil('\n'); line.trim();
          if (line.startsWith("/gifs/" + folder + "/")) { found = true; break; }
          delay(1);
        }
        pl.close();
      }
      if (found) {
        bool already = false;
        File pl2 = SD.open(plPath.c_str());
        if (pl2) {
          while (pl2.available()) {
            String line = pl2.readStringUntil('\n'); line.trim();
            if (line == gifPath) { already = true; break; }
            delay(1);
          }
          pl2.close();
        }
        if (!already) {
          File plApp = SD.open(plPath.c_str(), FILE_APPEND);
          if (plApp) { plApp.println(gifPath); plApp.close(); appended++; }
        }
      }
    }
    entry.close(); entry = plDir.openNextFile();
    delay(1);
  }
  plDir.close();
  if (appended > 0) Serial.println("[WEB] add-to-playlists: " + String(appended) + " playliste(s) mise(s) a jour pour " + gifPath);
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
    webServer->send(400, "text/plain", "ERR: aucun fichier recu");
  }
}

static void handleWebConfigUploadFile()
{
  HTTPUpload &upload = webServer->upload();
  if (upload.status == UPLOAD_FILE_START) {
    uploadSuccess = false;
    uploadDir = webServer->arg("dir");
    uploadDir.trim();
    if (uploadDir.length() == 0) {
      webServer->send(400, "text/plain", "ERR: dossier cible manquant");
      return;
    }
    String filename = upload.filename;
    { int p = filename.lastIndexOf('/'); if (p >= 0) filename = filename.substring(p + 1); }
    { int p = filename.lastIndexOf('\\'); if (p >= 0) filename = filename.substring(p + 1); }
    if (filename.length() == 0) { webServer->send(400, "text/plain", "ERR: nom fichier invalide"); return; }
    String path = "/gifs/" + uploadDir + "/" + filename;
    String dirPath = "/gifs/" + uploadDir;
    if (!SD.exists(dirPath.c_str())) {
      SD.mkdir(dirPath.c_str());
      // Workaround : creer+supprimer un fichier temporaire pour forcer la
      // creation propre de l'entree du repertoire (evite attribut lecture seule)
      File tmp = SD.open(dirPath + "/.tmp", FILE_WRITE);
      if (tmp) { tmp.close(); SD.remove(dirPath + "/.tmp"); }
    }
    if (SD.exists(path.c_str())) SD.remove(path.c_str());
    uploadFile = SD.open(path.c_str(), FILE_WRITE);
    uploadStartMs = millis();
    uploadTotalBytes = 0;
    if (!uploadFile) {
      webServer->send(500, "text/plain", "ERR: ecriture SD impossible");
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
    if (uploadFile) {
      uploadFile.close();
      uploadFile = File();
      uploadSuccess = true;
      unsigned long dt = millis() - uploadStartMs;
      Serial.println("[WEB] Upload done: " + String(uploadTotalBytes) + " bytes in " + String(dt) + "ms");
      // Mise a jour automatique des playlists concernees
      String fname = upload.filename;
      { int p = fname.lastIndexOf('/'); if (p >= 0) fname = fname.substring(p + 1); }
      { int p = fname.lastIndexOf('\\'); if (p >= 0) fname = fname.substring(p + 1); }
      addFileToPlaylists(uploadDir, fname);
    }
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    if (uploadFile) { uploadFile.close(); uploadFile = File(); }
    Serial.println("[WEB] Upload aborted");
  }
}

static void handleWebConfigSave()
{
  if (!webServer->hasArg("brightness")) { webServer->send(400, "text/plain", "ERR: missing params"); return; }
  int b = webServer->arg("brightness").toInt();
  if (b >= 0 && b <= 100) screenBrightness = map(b, 0, 100, 0, 255);
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

static void handleWebConfigDeleteFiles()
{
  if (!webServer->hasArg("dir") || !webServer->hasArg("files")) { webServer->send(400, "text/plain", "ERR: missing dir or files"); return; }
  String dirName = webServer->arg("dir"); dirName.trim();
  String files = webServer->arg("files");
  int count = 0, fail = 0, start = 0;
  while (true) {
    int comma = files.indexOf(',', start);
    String f = (comma < 0) ? files.substring(start) : files.substring(start, comma);
    f.trim();
    if (f.length() > 0) {
      String path = "/gifs/" + dirName + "/" + f;
      if (SD.exists(path.c_str())) {
        if (SD.remove(path.c_str())) { count++; }
        else { fail++; }
      } else { fail++; }
    }
    if (comma < 0) break;
    start = comma + 1;
  }
  String msg = "OK: " + String(count) + " supprime(s)" + (fail>0?", " + String(fail) + " echec(s)":"");
  webServer->send(200, "text/plain", msg);
}

static void handleWebConfigAddToPlaylists()
{
  if (!webServer->hasArg("dir") || !webServer->hasArg("file")) { webServer->send(200, "text/plain", "OK:0"); return; }
  String folder = webServer->arg("dir"); folder.trim();
  String fileName = webServer->arg("file"); fileName.trim();
  if (folder.length() == 0 || fileName.length() == 0) { webServer->send(200, "text/plain", "OK:0"); return; }
  String gifPath = "/gifs/" + folder + "/" + fileName;
  int appended = 0;
  Serial.println("[WEB] add-to-playlists folder=" + folder + " file=" + fileName);
  File plDir = SD.open("/playlists");
  if (!plDir || !plDir.isDirectory()) {
    if (plDir) plDir.close();
    Serial.println("[WEB] add-to-playlists: /playlists/ introuvable");
    webServer->send(200, "text/plain", "OK:0"); return;
  }
  File entry = plDir.openNextFile();
  while (entry) {
    String name = String(entry.name());
    int slash = name.lastIndexOf('/');
    String base = (slash >= 0) ? name.substring(slash + 1) : name;
    if (!entry.isDirectory() && base.endsWith(".txt")) {
      String plPath = "/playlists/" + base;
      bool found = false;
      File pl = SD.open(plPath.c_str());
      if (pl) {
        while (pl.available()) {
          String line = pl.readStringUntil('\n'); line.trim();
          if (line.startsWith("/gifs/" + folder + "/")) { found = true; break; }
          delay(1);
        }
        pl.close();
      }
      if (found) {
        Serial.println("[WEB] add-to-playlists: playlist " + base + " contient " + folder + ", ajout...");
        bool already = false;
        File pl2 = SD.open(plPath.c_str());
        if (pl2) {
          while (pl2.available()) {
            String line = pl2.readStringUntil('\n'); line.trim();
            if (line == gifPath) { already = true; break; }
            delay(1);
          }
          pl2.close();
        }
        if (!already) {
          File plApp = SD.open(plPath.c_str(), FILE_APPEND);
          if (plApp) { plApp.println(gifPath); plApp.close(); appended++; Serial.println("[WEB] ajoute " + gifPath + " a " + base); }
          else { Serial.println("[WEB] ERR ouverture " + base + " en ecriture"); }
        } else {
          Serial.println("[WEB] " + gifPath + " deja dans " + base);
        }
      }
    }
    entry.close(); entry = plDir.openNextFile();
    delay(1);
  }
  plDir.close();
  Serial.println("[WEB] add-to-playlists: " + String(appended) + " playliste(s) mise(s) a jour");
  String msg = "OK:" + String(appended);
  webServer->send(200, "text/plain", msg);
}

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
  String ip = WiFi.localIP().toString();
  String full = msg + " " + ip;
  clearFirstBoot();
  webDmdSetMainMsg(msg);
  webDmdPause(ip, 0xFFE0);
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
  // Ecrire config.ini
  Serial.println("[WEB] AP save: ecriture config.ini (SSID=" + wifiSSID + ")");
  if (SD.exists("/config.ini")) SD.remove("/config.ini");
  delay(100);
  File f = SD.open("/config.ini", FILE_WRITE);
  if (!f) { Serial.println("[WEB] AP save: ERREUR ouverture fichier"); webServer->send(500, "text/plain", "ERR: SD write failed"); return; }
  f.println("wifi_enabled=1");
  f.println("wifi_ssid=" + wifiSSID);
  f.println("wifi_password=" + wifiPassword);
  f.println("wifi_static_enabled=" + String(wifiStaticEnabled ? "1" : "0"));
  f.println("wifi_static_ip=" + wifiStaticIP);
  f.println("first_boot=0");
  f.flush();
  f.close();
  Serial.println("[WEB] AP save: fichier ecrit avec SSID=" + wifiSSID + " -> reboot");
  webServer->send(200, "text/plain", "OK");
  delay(1000);
  ESP.restart();
}

static void handleWebConfigRoot() {
  if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
    webServer->send_P(200, "text/html", WEB_CONFIG_AP_HTML);
  } else {
    webServer->send_P(200, "text/html", WEB_CONFIG_HTML);
  }
}

void setupWebConfig()
{
  if (webServer) delete webServer;
  webServer = new WebServer(80);
  webServer->on("/", handleWebConfigRoot);
  webServer->on("/load", handleWebConfigLoad);
  webServer->on("/lsplaylists", handleWebConfigListPlaylists);
  webServer->on("/lsgifdirs", handleWebConfigListGifDirs);
  webServer->on("/lsgiffiles", handleWebConfigListGifFiles);
  webServer->on("/gifcount", handleWebConfigGifCount);
  webServer->on("/generate-playlist", HTTP_POST, handleWebConfigGeneratePlaylist);
  webServer->on("/delete-playlist", HTTP_POST, handleWebConfigDeletePlaylist);
  webServer->on("/upload", HTTP_POST, handleWebConfigUpload, handleWebConfigUploadFile);
  webServer->on("/delete-folders", HTTP_POST, handleWebConfigDeleteFolders);
  webServer->on("/delete-files", HTTP_POST, handleWebConfigDeleteFiles);
  webServer->on("/scan-wifi", handleWebConfigScanWiFi);
  webServer->on("/save-ap", HTTP_POST, handleWebConfigSaveAP);
  webServer->on("/add-to-playlists", HTTP_POST, handleWebConfigAddToPlaylists);
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
