#include "wifimanager.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <sstream>

#include "log.h"
#include "netutil.h"

namespace {
const char *AP_SSID = "RecalBox-DMD-Config";  // meme nom que le firmware
const char *AP_CON = "recalboxdmd-ap";
const char *WIFI_CON = "recalboxdmd-wifi";
const char *AP_IP = "192.168.4.1";             // meme adresse que le point d'acces de l'ESP32

void sleepMs(int ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }

// textes des ecrans (trWifiRecoveryCountdown, trConnectWifiMsg, trOpenUrl, trConfigPageMsg, trJoinWifi, trOpenInBrowser du firmware)
std::string trCountdown(const std::string &l, long s) {
  return std::string(l == "en" ? "WiFi Recovery " : l == "es" ? "Recuperacion WiFi " : "Secours WiFi ") + std::to_string(s) + "s";
}
std::string trConnect(const std::string &l) {
  return l == "en" ? "Connect to WiFi RecalBox-DMD-Config" : l == "es" ? "Conectese al WiFi RecalBox-DMD-Config" : "Connectez-vous au WiFi RecalBox-DMD-Config";
}
std::string trOpenUrl(const std::string &l, const std::string &ip) {
  return std::string(l == "en" ? "Open http://" : l == "es" ? "Abrir http://" : "Ouvrir http://") + ip;
}
std::string trJoin(const std::string &l, const std::string &ssid) {
  return std::string(l == "en" ? "Join the wifi " : l == "es" ? "Unase al wifi " : "Rejoignez le wifi ") + ssid;
}
std::string trOpenBrowser(const std::string &l, const std::string &url) {
  return std::string(l == "en" ? "Open in a browser " : l == "es" ? "Abra en un navegador " : "Ouvrez dans un navigateur ") + url;
}

std::string trim1(const std::string &s) { return trim(s); }
}  // namespace

void WifiManager::start() {
  stop_ = false;
  th_ = std::thread([this] { run(); });
}

void WifiManager::stop() {
  stop_ = true;
  if (th_.joinable()) th_.join();
}

std::vector<std::string> WifiManager::scanCache() {
  std::lock_guard<std::mutex> lk(mu_);
  return scan_;
}

// ------------------------------------------------------------------------------------------------------------------ NetworkManager
// profil « recalboxdmd-wifi » : SSID, mot de passe, IP fixe (si complete). Compare avec le profil existant pour ne rien toucher quand
// config.ini n'a pas change (une connexion qui fonctionne n'est jamais interrompue pour rien).
bool WifiManager::reconcileProfile() {
  const std::string ssid = cfg_.get("wifi_ssid"), pass = cfg_.get("wifi_password");
  if (ssid.empty()) return false;
  const bool wantStatic = cfg_.getBool("wifi_static_enabled", false);
  std::string ip = cfg_.get("wifi_static_ip"), gw = cfg_.get("wifi_gateway"), mask = cfg_.get("wifi_subnet", "255.255.255.0");
  const std::string dns1 = cfg_.get("wifi_dns1"), dns2 = cfg_.get("wifi_dns2");
  bool useStatic = wantStatic && !ip.empty();
  if (useStatic && gw.empty()) {  // la page du point d'acces ne demande que l'IP : passerelle = x.y.z.1 par defaut
    const size_t p = ip.rfind('.');
    if (p != std::string::npos) gw = ip.substr(0, p) + ".1";
  }
  int prefix = 24;
  {
    unsigned a = 0, b = 0, c = 0, d = 0;
    if (std::sscanf(mask.c_str(), "%u.%u.%u.%u", &a, &b, &c, &d) == 4) {
      uint32_t m = (a << 24) | (b << 16) | (c << 8) | d;
      prefix = 0;
      while (m & 0x80000000u) { prefix++; m <<= 1; }
    }
  }
  // profil actuel (secrets inclus : root)
  int rc = 0;
  const std::string cur = runArgs({"nmcli", "-s", "-g", "802-11-wireless.ssid,802-11-wireless-security.psk,ipv4.method,ipv4.addresses,ipv4.gateway", "connection", "show", WIFI_CON}, 8000, &rc);
  std::vector<std::string> lines;
  {
    std::istringstream in(cur);
    std::string l;
    while (std::getline(in, l)) lines.push_back(l);
  }
  const std::string wantAddr = useStatic ? ip + "/" + std::to_string(prefix) : "";
  const bool same = rc == 0 && lines.size() >= 5 && lines[0] == ssid && lines[1] == pass && lines[2] == (useStatic ? "manual" : "auto") &&
                    (!useStatic || (lines[3] == wantAddr && lines[4] == gw));
  if (same) return true;
  logf("WIFI", "profil NetworkManager « %s » mis a jour (reseau %s%s)", WIFI_CON, ssid.c_str(), useStatic ? ", IP fixe" : "");
  runArgs({"nmcli", "connection", "delete", WIFI_CON}, 8000);
  std::vector<std::string> a = {"nmcli", "connection", "add", "type", "wifi", "ifname", iface_, "con-name", WIFI_CON, "ssid", ssid,
                                "connection.autoconnect", "yes", "connection.autoconnect-priority", "100"};
  if (!pass.empty()) {
    a.insert(a.end(), {"wifi-sec.key-mgmt", "wpa-psk", "wifi-sec.psk", pass});
  }
  if (useStatic) {
    a.insert(a.end(), {"ipv4.method", "manual", "ipv4.addresses", wantAddr, "ipv4.gateway", gw});
    std::string dns = dns1.empty() ? gw : dns1;
    if (!dns2.empty()) dns += " " + dns2;
    a.insert(a.end(), {"ipv4.dns", dns});
  }
  runArgs(a, 15000, &rc);
  if (rc != 0) {
    logf("WIFI", "creation du profil impossible (nmcli rc=%d)", rc);
    return false;
  }
  return true;
}

bool WifiManager::waitConnectivity(int seconds) {
  for (int i = 0; i < seconds * 2 && !stop_; i++) {
    if (hasDefaultRoute()) return true;
    sleepMs(500);
  }
  return hasDefaultRoute();
}

bool WifiManager::connectConfigured(int waitSec) {
  if (cfg_.get("wifi_ssid").empty()) return false;
  if (!reconcileProfile()) return false;
  int rc = 0;
  runArgs({"nmcli", "--wait", std::to_string(waitSec), "connection", "up", WIFI_CON}, (waitSec + 5) * 1000, &rc);
  if (rc != 0) {  // mot de passe faux, reseau absent... (une route par defaut peut exister quand meme : NetworkManager retombe sur un autre profil)
    logf("WIFI", "connexion a « %s » impossible (nmcli rc=%d)", cfg_.get("wifi_ssid").c_str(), rc);
    return false;
  }
  return waitConnectivity(waitSec);
}

bool WifiManager::startAp() {
  {
    std::lock_guard<std::mutex> lk(mu_);
    if (scan_.empty()) {  // liste des reseaux AVANT d'ouvrir le point d'acces (le scan est impossible ensuite, comme sur l'ESP32)
      std::map<std::string, int> best;
      std::istringstream in(runArgs({"nmcli", "-t", "-f", "SIGNAL,SSID", "dev", "wifi", "list", "--rescan", "yes"}, 15000));
      std::string line;
      while (std::getline(in, line)) {
        const size_t c = line.find(':');
        if (c == std::string::npos) continue;
        const int sig = std::atoi(line.substr(0, c).c_str());
        std::string ssid;
        for (size_t i = c + 1; i < line.size(); i++) {
          if (line[i] == '\\' && i + 1 < line.size()) i++;
          ssid += line[i];
        }
        if (ssid.empty()) continue;
        auto it = best.find(ssid);
        if (it == best.end() || sig > it->second) best[ssid] = sig;
      }
      std::vector<std::pair<int, std::string>> v;
      for (const auto &p : best) v.push_back({p.second, p.first});
      std::sort(v.begin(), v.end(), [](const auto &a, const auto &b) { return a.first > b.first; });
      for (const auto &p : v) scan_.push_back(p.second);
    }
  }
  runArgs({"nmcli", "connection", "delete", AP_CON}, 8000);
  int rc = 0;
  runArgs({"nmcli", "connection", "add", "type", "wifi", "ifname", iface_, "con-name", AP_CON, "autoconnect", "no", "ssid", AP_SSID,
           "802-11-wireless.mode", "ap", "802-11-wireless.band", "bg", "ipv4.method", "shared", "ipv4.addresses", std::string(AP_IP) + "/24",
           "ipv6.method", "ignore"},
          15000, &rc);
  if (rc != 0) {
    logf("WIFI", "creation du point d'acces impossible (nmcli rc=%d)", rc);
    return false;
  }
  runArgs({"nmcli", "--wait", "20", "connection", "up", AP_CON}, 25000, &rc);
  if (rc != 0) {
    logf("WIFI", "ouverture du point d'acces impossible (nmcli rc=%d)", rc);
    runArgs({"nmcli", "connection", "delete", AP_CON}, 8000);
    return false;
  }
  apActive_ = true;
  logf("WIFI", "point d'acces « %s » ouvert : http://%s", AP_SSID, AP_IP);
  return true;
}

void WifiManager::stopAp() {
  if (!apActive_.load()) return;
  runArgs({"nmcli", "connection", "down", AP_CON}, 10000);
  runArgs({"nmcli", "connection", "delete", AP_CON}, 8000);
  apActive_ = false;
  logf("WIFI", "point d'acces ferme");
  runArgs({"nmcli", "device", "connect", iface_}, 20000);  // reprend la connexion normale (autoconnect)
}

void WifiManager::onConnected() {
  const NetInfo ni = netInfo();
  logf("WIFI", "connecte : %s (%s)", ni.ip.c_str(), ni.dev.c_str());
  const bool firstBootNow = cfg_.getBool("first_boot", true);
  logf("WIFI", "first_boot=%d : %s", firstBootNow ? 1 : 0, firstBootNow ? "ecran de configuration" : "affichage normal");
  if (!firstBootNow) {  // appareil deja configure : on quitte l'ecran du point d'acces s'il est affiche, sinon l'affichage normal continue
    eng_.postMain([this] {
      if (eng_.configScreenActive()) eng_.webResume();
    });
    return;
  }
  // premier demarrage : « Page de configuration » + adresse de la page, jusqu'au premier /save valide
  eng_.postMain([this] { eng_.showFirstBootScreen(); });
}

// ------------------------------------------------------------------------------------------------------------------ points d'acces
// Ecrans du point d'acces. Premier demarrage / premiere configuration : « Connectez-vous au WiFi RecalBox-DMD-Config » + « Ouvrir http://192.168.4.1 » ;
// secours (recovery) : compte a rebours en ligne 1, ligne 2 alternee toutes les 6 s entre le SSID et l'adresse (SSID / adresse en blanc).
void WifiManager::showApScreens(uint64_t startMs, int total, bool recovery) {
  if (!recovery) return;
  const std::string lang = i18nLang(cfg_.uiLanguage);
  const uint64_t now = nowMs();
  const uint64_t elapsed = now - startMs;
  if (apLastSecMs_ == 0 || now - apLastSecMs_ >= 1000) {
    apLastSecMs_ = now;
    const long remaining = std::max<long>(0, total - (long)(elapsed / 1000));
    const std::string m = trCountdown(lang, remaining);
    eng_.postMain([this, m] { eng_.configMainLine(m); });
  }
  if (apLastToggleMs_ == 0 || now - apLastToggleMs_ >= 6000) {  // le SSID reste affiche 6 s, puis l'adresse 6 s, etc.
    apLastToggleMs_ = now;
    apShowSsid_ = !apShowSsid_;
    std::string sub;
    int white;
    if (apShowSsid_) {
      sub = trJoin(lang, AP_SSID);
      white = (int)sub.size() - (int)std::string(AP_SSID).size();
    } else {
      const std::string url = std::string("http://") + AP_IP;
      sub = trOpenBrowser(lang, url);
      white = (int)sub.size() - (int)url.size();
    }
    eng_.postMain([this, sub, white] { eng_.configSubLine(sub, 0xFFE0, white); });
  }
}
void WifiManager::apLoop(bool recovery, int seconds) {
  const std::string lang = i18nLang(cfg_.uiLanguage);
  if (!startAp()) return;
  const uint64_t t0 = nowMs();
  // ecran de depart du point d'acces (recommence apres un echec de connexion : le compte a rebours du secours reste affiche)
  auto showStart = [&] {
    if (recovery) {
      const long remaining = std::max<long>(0, seconds - (long)((nowMs() - t0) / 1000));
      const std::string m = trCountdown(lang, remaining), sub = trJoin(lang, AP_SSID);
      const int white = (int)sub.size() - (int)std::string(AP_SSID).size();
      eng_.postMain([this, m, sub, white] { eng_.configScreen(m, sub, 0xFFE0, white); });
      apLastSecMs_ = apLastToggleMs_ = nowMs();  // prochaine mise a jour du compte a rebours dans 1 s, alternance SSID / adresse dans 6 s
      apShowSsid_ = true;                        // le SSID est deja affiche
    } else {
      const std::string m = trConnect(lang), sub = trOpenUrl(lang, AP_IP);
      eng_.postMain([this, m, sub] { eng_.configScreen(m, sub, 0xFFE0, -1); });
    }
  };
  showStart();
  while (!stop_) {
    sleepMs(500);
    if (recovery) showApScreens(t0, seconds, true);
    if (applyReq_.exchange(false)) {  // la page du point d'acces vient d'ecrire le reseau choisi
      logf("WIFI", "reseau saisi dans la page : application");
      sleepMs(1500);  // laisse la reponse « OK » parvenir au telephone avant de couper le point d'acces
      stopAp();
      if (connectConfigured(30)) {
        onConnected();
        return;
      }
      logf("WIFI", "connexion impossible (identifiants faux ?) : le point d'acces revient");
      if (!startAp()) return;
      showStart();
      continue;
    }
    if (recovery && (int)((nowMs() - t0) / 1000) >= seconds) {
      logf("WIFI", "fin du mode secours : retour au reseau normal");
      stopAp();
      waitConnectivity(20);
      if (cfg_.getBool("first_boot", true)) onConnected();  // premier demarrage : retour a l'ecran « Page de configuration »
      else eng_.postMain([this] { eng_.webResume(); });
      return;
    }
    if (!recovery && hasDefaultRoute()) {  // un reseau (Ethernet, par exemple) est apparu : plus besoin du point d'acces
      logf("WIFI", "connectivite retrouvee : le point d'acces se ferme");
      stopAp();
      onConnected();
      return;
    }
  }
  stopAp();
}
// ------------------------------------------------------------------------------------------------------------------ demarrage
void WifiManager::run() {
  // commande de la Recalbox / du bouton recue pendant le demarrage ou plus tard : on boucle, toute la vie du demon
  const bool wifiEnabled = cfg_.getBool("wifi_enabled", true);
  const int recoverySec = std::max(10, cfg_.getInt("ap_recovery_seconds", 180));
  {  // interface Wi-Fi reelle
    const std::string d = runShell("nmcli -t -f DEVICE,TYPE dev 2>/dev/null | grep ':wifi$' | head -1 | cut -d: -f1");
    const std::string t = trim1(d);
    if (!t.empty()) iface_ = t;
  }
  logf("WIFI", "gestionnaire Wi-Fi demarre (interface %s)", iface_.c_str());
  if (!wifiEnabled) {
    logf("WIFI", "wifi_enabled=0 : le Pi garde son reseau tel quel");
  } else if (cfg_.getBool("force_ap_recovery", false)) {
    logf("WIFI", "force_ap_recovery=1 : point d'acces de secours");
    eng_.postMain([this] { cfg_.writeKeys(fs_, {{"force_ap_recovery", "0"}}); });
    apLoop(true, recoverySec);
  } else {
    const bool hadConfig = fs_.exists("/config.ini");
    const bool firstBoot = cfg_.getBool("first_boot", true);  // cle absente = premier demarrage (comme le firmware)
    const std::string ssid = cfg_.get("wifi_ssid");
    if (!ssid.empty()) reconcileProfile();
    bool up = hasDefaultRoute();
    if (!up) up = waitConnectivity(35);
    if (up) {
      onConnected();
    } else if (!stop_) {
      const bool profileKnown = !trim1(runShell("nmcli -t -f TYPE connection show 2>/dev/null | grep -c 802-11-wireless")).empty() &&
                                std::atoi(runShell("nmcli -t -f TYPE connection show 2>/dev/null | grep -c 802-11-wireless").c_str()) > 0;
      const bool wifiKnown = !ssid.empty() || profileKnown;
      if (firstBoot || !hadConfig || !wifiKnown) {
        logf("WIFI", "pas de reseau (%s%s%s) -> point d'acces de configuration", firstBoot ? "premier demarrage " : "", !hadConfig ? "config.ini absent " : "",
             !wifiKnown ? "aucune configuration Wi-Fi valide" : "");
        apLoop(false, 0);
      } else {
        logf("WIFI", "reseau injoignable, first_boot=0 : pas de repli AP, NetworkManager reessaie");
      }
    }
  }
  // vie du demon : secours demandes plus tard (CMD=wifi_recovery, bouton), reseau saisi sans point d'acces
  while (!stop_) {
    sleepMs(500);
    if (recoveryReq_.exchange(false)) {
      logf("WIFI", "secours demande -> point d'acces de secours %d s", recoverySec);
      apLoop(true, recoverySec);
    }
    if (applyReq_.exchange(false)) {
      logf("WIFI", "nouveau reseau enregistre : application");
      if (connectConfigured(30)) onConnected();
      else logf("WIFI", "connexion impossible : NetworkManager reessaie");
    }
  }
}
