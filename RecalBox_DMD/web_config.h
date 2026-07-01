// ============================================
// web_config.h — Interface web de configuration
// ============================================
// v1 - 2026-07-01 - Serveur HTTP pour modifier config.ini
// Ne bloque pas la boucle playlist (non-bloquant via handleClient)
// ============================================

#ifndef WEB_CONFIG_H
#define WEB_CONFIG_H

#include <WiFi.h>
#include <WebServer.h>

// Forward declarations des variables externes
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
extern bool   requestReboot;

// WebServer instance (heap, cree dans setupWebConfig)
static WebServer *webServer = nullptr;
static bool webConfigEnabled = false;

// ================================================
// Page HTML de configuration (integrée dans le .ino)
// ================================================
static const char WEB_CONFIG_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="fr">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>RecalBox DMD Config</title>
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
.btn-row{display:flex;gap:12px;justify-content:center;margin:20px 0}
.btn{padding:12px 32px;border:none;border-radius:6px;font-size:16px;font-weight:bold;cursor:pointer}
.btn-save{background:#ffd146;color:#1a1a2e}
.btn-save:hover{background:#ffe070}
.btn-reboot{background:#e63946;color:#fff}
.btn-reboot:hover{background:#ff5a67}
.msg{padding:12px;border-radius:6px;margin:12px 0;display:none}
.msg-ok{background:#1b4332;color:#95d5b2;border:1px solid #52b788}
.msg-err{background:#4a0e0e;color:#f5a3a3;border:1px solid #e63946}
footer{text-align:center;color:#555;font-size:12px;margin:24px 0}
</style>
</head>
<body>
<h1>&#x1F4BB; RecalBox DMD Config</h1>
<div id="msg" class="msg"></div>
<form id="configForm" onsubmit="saveConfig(event)">
<!-- Affichage -->
<div class="section">
<h2>&#x1F4A1; Affichage</h2>
<div class="row"><label>Luminosité (%)</label><input type="range" id="brightness" min="0" max="100" oninput="document.getElementById('bval').textContent=this.value"><span id="bval" style="margin-left:8px;color:#ffd146;min-width:30px">50</span></div>
</div>
<!-- Playlist -->
<div class="section">
<h2>&#x1F4BF; Playlist</h2>
<div class="row"><label>Fichier playlist</label><input type="text" id="playlist" placeholder="ex: RecalBox_intros.txt"></div>
<div class="row"><label>Aléatoire</label><input type="checkbox" id="random"></div>
</div>
<!-- WiFi -->
<div class="section">
<h2>&#x1F4F6; Wi-Fi</h2>
<div class="row"><label>Activé</label><input type="checkbox" id="wifi_enabled"></div>
<div class="row"><label>SSID</label><input type="text" id="wifi_ssid"></div>
<div class="row"><label>Mot de passe</label><input type="password" id="wifi_password"></div>
<div class="row"><label>IP statique</label><input type="checkbox" id="wifi_static_enabled"></div>
<div class="row"><label>IP fixe</label><input type="text" id="wifi_static_ip" placeholder="192.168.0.46"></div>
<div class="row"><label>Gateway</label><input type="text" id="wifi_gateway"></div>
<div class="row"><label>Subnet</label><input type="text" id="wifi_subnet"></div>
<div class="row"><label>DNS 1</label><input type="text" id="wifi_dns1"></div>
<div class="row"><label>DNS 2</label><input type="text" id="wifi_dns2"></div>
</div>
<!-- Bluetooth -->
<div class="section">
<h2>&#x1F4F1; Bluetooth</h2>
<div class="row"><label>Activé</label><input type="checkbox" id="bluetooth_enabled"></div>
<div class="row"><label>Nom</label><input type="text" id="bluetooth_name"></div>
</div>
<!-- MQTT -->
<div class="section">
<h2>&#x1F310; MQTT</h2>
<div class="row"><label>IP Recalbox</label><input type="text" id="recalbox_ip" placeholder="192.168.0.35"></div>
</div>
<!-- Clock -->
<div class="section">
<h2>&#x23F0; Horloge</h2>
<div class="row"><label>Activée</label><input type="checkbox" id="clock_enabled"></div>
<div class="row">
<label>Thème</label>
<select id="clock_theme">
<option value="-1">Aléatoire</option>
<option value="0">Mario</option>
<option value="1">Tetris</option>
<option value="2">Pac-Man</option>
<option value="3">Space Invaders</option>
<option value="4">Pong</option>
<option value="5">Neon</option>
<option value="6">Matrix</option>
<option value="7">Fire</option>
<option value="8">Rainbow</option>
</select>
</div>
<div class="row"><label>Intervalle (GIFs)</label><input type="number" id="clock_interval" min="1" max="999"></div>
<div class="row"><label>Intervalle (min)</label><input type="number" id="clock_interval_min" min="0" max="999"><div class="hint">0 = désactivé, utilise l'intervalle en GIFs</div></div>
<div class="row"><label>Durée (sec)</label><input type="number" id="clock_duration" min="1" max="120"></div>
<div class="row"><label>Timezone</label><input type="text" id="clock_tz" placeholder="CET-1CEST,M3.5.0,M10.5.0/3"></div>
</div>
<!-- Boutons -->
<div class="btn-row">
<button type="submit" class="btn btn-save">&#x1F4BE; Sauvegarder</button>
<button type="button" class="btn btn-reboot" onclick="saveAndReboot()">&#x1F504; Sauvegarder & Redémarrer</button>
</div>
</form>
<footer>RecalBox DMD v7 — Interface web</footer>
<script>
function showMsg(text, isOk) {
  const el = document.getElementById('msg');
  el.textContent = text;
  el.className = 'msg ' + (isOk ? 'msg-ok' : 'msg-err');
  el.style.display = 'block';
  setTimeout(() => { el.style.display = 'none'; }, 5000);
}
function serialize() {
  const f = (id) => document.getElementById(id);
  const v = (id) => f(id).value.trim();
  const c = (id) => f(id).checked ? '1' : '0';
  return new URLSearchParams({
    brightness: f('brightness').value,
    playlist: v('playlist'),
    random: c('random'),
    wifi_enabled: c('wifi_enabled'),
    wifi_ssid: v('wifi_ssid'),
    wifi_password: v('wifi_password'),
    wifi_static_enabled: c('wifi_static_enabled'),
    wifi_static_ip: v('wifi_static_ip'),
    wifi_gateway: v('wifi_gateway'),
    wifi_subnet: v('wifi_subnet'),
    wifi_dns1: v('wifi_dns1'),
    wifi_dns2: v('wifi_dns2'),
    bluetooth_enabled: c('bluetooth_enabled'),
    bluetooth_name: v('bluetooth_name'),
    recalbox_ip: v('recalbox_ip'),
    clock_enabled: c('clock_enabled'),
    clock_theme: v('clock_theme'),
    clock_interval: v('clock_interval'),
    clock_interval_min: v('clock_interval_min'),
    clock_duration: v('clock_duration'),
    clock_tz: v('clock_tz')
  });
}
function saveConfig(e) {
  e.preventDefault();
  fetch('/save', { method: 'POST', body: serialize(), headers: {'Content-Type':'application/x-www-form-urlencoded'} })
    .then(r => r.text()).then(t => showMsg(t, t.includes('OK')))
    .catch(() => showMsg('Erreur réseau', false));
}
function saveAndReboot() {
  fetch('/save', { method: 'POST', body: serialize(), headers: {'Content-Type':'application/x-www-form-urlencoded'} })
    .then(r => r.text()).then(t => {
      showMsg(t, t.includes('OK'));
      if (t.includes('OK')) setTimeout(() => fetch('/reboot').catch(()=>{}), 1000);
    })
    .catch(() => showMsg('Erreur réseau', false));
}
// Charger les valeurs actuelles
fetch('/load').then(r => r.json()).then(d => {
  document.getElementById('brightness').value = d.brightness || 50;
  document.getElementById('bval').textContent = d.brightness || 50;
  document.getElementById('playlist').value = d.playlist || '';
  document.getElementById('random').checked = d.random == '1';
  document.getElementById('wifi_enabled').checked = d.wifi_enabled == '1';
  document.getElementById('wifi_ssid').value = d.wifi_ssid || '';
  document.getElementById('wifi_password').value = d.wifi_password || '';
  document.getElementById('wifi_static_enabled').checked = d.wifi_static_enabled == '1';
  document.getElementById('wifi_static_ip').value = d.wifi_static_ip || '';
  document.getElementById('wifi_gateway').value = d.wifi_gateway || '';
  document.getElementById('wifi_subnet').value = d.wifi_subnet || '';
  document.getElementById('wifi_dns1').value = d.wifi_dns1 || '';
  document.getElementById('wifi_dns2').value = d.wifi_dns2 || '';
  document.getElementById('bluetooth_enabled').checked = d.bluetooth_enabled == '1';
  document.getElementById('bluetooth_name').value = d.bluetooth_name || '';
  document.getElementById('recalbox_ip').value = d.recalbox_ip || '';
  document.getElementById('clock_enabled').checked = d.clock_enabled == '1';
  document.getElementById('clock_theme').value = d.clock_theme || '-1';
  document.getElementById('clock_interval').value = d.clock_interval || '10';
  document.getElementById('clock_interval_min').value = d.clock_interval_min || '0';
  document.getElementById('clock_duration').value = d.clock_duration || '8';
  document.getElementById('clock_tz').value = d.clock_tz || '';
}).catch(() => showMsg('Erreur chargement config', false));
</script>
</body>
</html>
)rawliteral";

// ================================================
// Handlers
// ================================================

// GET /load — renvoie la config actuelle en JSON
static void handleWebConfigLoad()
{
  String json = "{";
  json += "\"brightness\":\"" + String(screenBrightness) + "\"";
  json += ",\"playlist\":\"" + playlistName + "\"";
  json += ",\"random\":\"" + String(playlistRandom ? '1' : '0') + "\"";
  json += ",\"wifi_enabled\":\"" + String(wifiEnabled ? '1' : '0') + "\"";
  json += ",\"wifi_ssid\":\"" + wifiSSID + "\"";
  json += ",\"wifi_password\":\"" + wifiPassword + "\"";
  json += ",\"wifi_static_enabled\":\"" + String(wifiStaticEnabled ? '1' : '0') + "\"";
  json += ",\"wifi_static_ip\":\"" + wifiStaticIP + "\"";
  json += ",\"wifi_gateway\":\"" + wifiGateway + "\"";
  json += ",\"wifi_subnet\":\"" + wifiSubnet + "\"";
  json += ",\"wifi_dns1\":\"" + wifiDNS1 + "\"";
  json += ",\"wifi_dns2\":\"" + wifiDNS2 + "\"";
  json += ",\"bluetooth_enabled\":\"" + String(bluetoothEnabled ? '1' : '0') + "\"";
  json += ",\"bluetooth_name\":\"" + bluetoothName + "\"";
  json += ",\"recalbox_ip\":\"" + recalboxIP + "\"";
  json += ",\"clock_enabled\":\"" + String(clockEnabled ? '1' : '0') + "\"";
  json += ",\"clock_theme\":\"" + String(clockTheme) + "\"";
  json += ",\"clock_interval\":\"" + String(clockIntervalGifs) + "\"";
  json += ",\"clock_interval_min\":\"" + String(clockIntervalMin) + "\"";
  json += ",\"clock_duration\":\"" + String(clockDuration) + "\"";
  json += ",\"clock_tz\":\"" + clockTimeZone + "\"";
  json += "}";
  webServer->send(200, "application/json", json);
}

// POST /save — reçoit les nouveaux paramètres et écrit config.ini
static void handleWebConfigSave()
{
  if (!webServer->hasArg("brightness")) {
    webServer->send(400, "text/plain", "ERR: missing params");
    return;
  }

  // Lire tous les champs
  int b = webServer->arg("brightness").toInt();
  if (b >= 0 && b <= 100) screenBrightness = map(b, 0, 100, 0, 255);

  if (webServer->hasArg("playlist"))        playlistName = webServer->arg("playlist");
  if (webServer->hasArg("random"))          playlistRandom = webServer->arg("random") == "1";
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
  if (webServer->hasArg("clock_interval"))  clockIntervalGifs = webServer->arg("clock_interval").toInt();
  if (webServer->hasArg("clock_interval_min")) clockIntervalMin = webServer->arg("clock_interval_min").toInt();
  if (webServer->hasArg("clock_duration"))  clockDuration = webServer->arg("clock_duration").toInt();
  if (webServer->hasArg("clock_tz"))        clockTimeZone = webServer->arg("clock_tz");

  // Écrire le nouveau config.ini sur la SD
  File f = SD.open("/config.ini", FILE_WRITE);
  if (!f) {
    webServer->send(500, "text/plain", "ERR: SD write failed");
    return;
  }

  f.println("# Info");
  f.println("info=" + String(showInfo ? "1" : "0"));
  f.println();
  f.println("# Affichage");
  f.println("brightness=" + String(b));
  f.println();
  f.println("# Playlist");
  f.println("playlist=" + playlistName);
  f.println("random=" + String(playlistRandom ? "1" : "0"));
  f.println();
  f.println("# Wi-Fi & Bluetooth");
  f.println("wifi_enabled=" + String(wifiEnabled ? "1" : "0"));
  f.println("wifi_ssid=" + wifiSSID);
  f.println("wifi_password=" + wifiPassword);
  f.println("bluetooth_enabled=" + String(bluetoothEnabled ? "1" : "0"));
  f.println("bluetooth_name=" + bluetoothName);
  f.println();
  f.println("wifi_static_enabled=" + String(wifiStaticEnabled ? "1" : "0"));
  f.println("wifi_static_ip=" + wifiStaticIP);
  f.println("wifi_gateway=" + wifiGateway);
  f.println("wifi_subnet=" + wifiSubnet);
  f.println("wifi_dns1=" + wifiDNS1);
  f.println("wifi_dns2=" + wifiDNS2);
  f.println();
  f.println("# MQTT");
  f.println("recalbox_ip=" + recalboxIP);
  f.println();
  f.println("# Clock (horloge retro themes)");
  f.println("[CLOCK]");
  f.println("CLOCK_ENABLED=" + String(clockEnabled ? "1" : "0"));
  f.println("CLOCK_THEME=" + String(clockTheme));
  f.println("CLOCK_INTERVAL=" + String(clockIntervalGifs));
  f.println("CLOCK_INTERVAL_MIN=" + String(clockIntervalMin));
  f.println("CLOCK_DURATION=" + String(clockDuration));
  f.println("TZ=" + clockTimeZone);

  f.close();

  Serial.println("[WEB] config.ini saved (brightness=" + String(b) + "%)");
  webServer->send(200, "text/plain", "OK");
}

// GET /reboot — redémarre l'ESP32
static void handleWebConfigReboot()
{
  webServer->send(200, "text/plain", "REBOOT");
  delay(500);
  ESP.restart();
}

// GET / — page HTML
static void handleWebConfigRoot()
{
  webServer->send_P(200, "text/html", WEB_CONFIG_HTML);
}

// ================================================
// Initialisation
// ================================================
void setupWebConfig()
{
  if (webServer) {
    delete webServer;
    webServer = nullptr;
  }

  webServer = new WebServer(80);

  webServer->on("/", handleWebConfigRoot);
  webServer->on("/load", handleWebConfigLoad);
  webServer->on("/save", HTTP_POST, handleWebConfigSave);
  webServer->on("/reboot", handleWebConfigReboot);

  webServer->begin();
  webConfigEnabled = true;
  Serial.println("[WEB] Interface config sur http://" + WiFi.localIP().toString());
}

// ================================================
// À appeler dans loop() — non-bloquant
// ================================================
void handleWebConfig()
{
  if (webServer) {
    webServer->handleClient();
  }
}

#endif // WEB_CONFIG_H
