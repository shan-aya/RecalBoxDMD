#include "webconfig.h"

#include <unistd.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>

#include "brightsched.h"
#include <chrono>
#include <thread>

#include "log.h"
#include "netutil.h"
#include "wifimanager.h"
#include "button.h"

namespace {
std::string jsonEscape(const std::string &s) {
  std::string out;
  out.reserve(s.size() + 4);
  for (char c : s) {
    if (c == '"') out += "\\\"";
    else if (c == '\\') out += "\\\\";
    else if (c == '\r') out += "\\r";
    else if (c == '\n') out += "\\n";
    else if (c == '\t') out += "\\t";
    else out += c;
  }
  return out;
}

int32_t hexColor(const std::string &s) {  // « #RRGGBB » -> 0xRRGGBB, sinon -1 (automatique)
  if (s.size() != 7 || s[0] != '#') return -1;
  return (int32_t)(std::strtoul(s.c_str() + 1, nullptr, 16) & 0xFFFFFF);
}
std::string colorHex(int32_t c) {  // 0xRRGGBB -> « #RRGGBB », -1 -> « » (automatique)
  if (c < 0) return "";
  char b[10];
  std::snprintf(b, sizeof(b), "#%06X", (unsigned)c & 0xFFFFFF);
  return b;
}

std::string quoteSlug(const std::string &in) {  // minuscules, [a-z0-9_-], espaces -> '-', 24 car. max (sert de nom de fichier)
  std::string o;
  for (size_t i = 0; i < in.size() && o.size() < 24; i++) {
    char c = in[i];
    if (c >= 'A' && c <= 'Z') c = (char)(c + 32);
    if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_') o += c;
    else if (c == ' ') o += '-';
  }
  return o;
}
std::string quoteSlugList(const std::string &in) {  // liste « a,b,c » (24 au maximum)
  std::string out;
  size_t start = 0;
  int cnt = 0;
  while (start <= in.size() && cnt < 24) {
    const size_t c = in.find(',', start);
    const std::string tok = quoteSlug(c == std::string::npos ? in.substr(start) : in.substr(start, c - start));
    if (!tok.empty()) {
      if (!out.empty()) out += ",";
      out += tok;
      cnt++;
    }
    if (c == std::string::npos) break;
    start = c + 1;
  }
  return out;
}

bool readFile(const std::string &path, std::string &out) {
  std::ifstream f(path, std::ios::binary);
  if (!f) return false;
  std::ostringstream ss;
  ss << f.rdbuf();
  out = ss.str();
  return true;
}

// sortie d'une commande fixe (nmcli / ip) : voir netutil.h
std::string runCmd(const std::string &cmd) { return runShell(cmd); }
std::string b01(bool v) { return v ? "1" : "0"; }
// dossier d'installation (<exe>/..) : /opt/recalboxdmd ; vide si l'executable n'est pas dans une installation (lancement a la main)
std::string installPrefix() {
  char buf[4096];
  const ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
  if (n <= 0) return "";
  std::string p(buf, (size_t)n);
  const size_t a = p.rfind('/');
  if (a == std::string::npos || a == 0) return "";
  p = p.substr(0, a);                                  // .../bin
  const size_t b = p.rfind('/');
  if (b == std::string::npos) return "";
  return p.substr(0, b);                               // ...
}
std::string readSmall(const std::string &path) {
  std::ifstream f(path);
  std::string s((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
  while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' ')) s.pop_back();
  return s;
}
int clampi(int v, int lo, int hi) { return std::max(lo, std::min(hi, v)); }

// v398 -- liste de themes « ,a,b, » (noms [A-Za-z0-9._-] de 31 car. max, 24 noms max) ; tout le reste est ignore (meme regle que le firmware ESP32).
std::string reelsSanitizeList(const std::string &raw) {
  std::string out, cur;
  int n = 0;
  for (size_t i = 0; i <= raw.size(); i++) {
    const char ch = (i < raw.size()) ? raw[i] : ',';
    if (ch == ',') {
      if (!cur.empty() && cur != "." && cur != ".." && n < 24 && out.find("," + cur + ",") == std::string::npos) {
        out += (out.empty() ? std::string(",") : std::string()) + cur + ",";
        n++;
      }
      cur.clear();
    } else if (((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '.' || ch == '_' || ch == '-') && cur.size() < 31) {
      cur += ch;
    }
  }
  return out;
}
std::string trimStr(const std::string &s) { return trim(s); }
}  // namespace

std::string WebConfig::wg(const std::string &key, const std::string &def) const {
  return cfg_.has("widget_" + key) ? cfg_.get("widget_" + key, def) : cfg_.get("deco_" + key, def);
}

WebReply WebConfig::page(const std::string &file, const WebRequest &req) {
  // une page affichee = « WEB DMD CONFIG » + adresse sur le DMD, sauf requete speculative du navigateur (prefetch / prerender)
  const bool speculative = req.header("purpose") == "prefetch" || !req.header("sec-purpose").empty() || !req.header("x-moz").empty();
  if (!speculative) {
    const std::string url = "http://" + (req.localAddr.empty() ? std::string("0.0.0.0") : req.localAddr);
    eng_.runOnMain([&] { eng_.webPage(url); });
  }
  WebReply r;
  if (!readFile(webDir_ + "/" + file, r.body)) {
    r.status = 404;
    r.body = "page absente : " + file + " (dossier web : " + webDir_ + ")";
    return r;
  }
  r.contentType = "text/html; charset=utf-8";
  r.headers["Cache-Control"] = "no-store";
  return r;
}

WebReply WebConfig::asset(const std::string &file, const char *type, const char *cache) {
  WebReply r;
  if (!readFile(webDir_ + "/" + file, r.body)) {
    r.status = 404;
    r.body = "absent";
    return r;
  }
  r.contentType = type;
  r.headers["Cache-Control"] = cache;
  return r;
}

// ------------------------------------------------------------------------------------------------------------------------ /load
// meme reponse que handleWebConfigLoad() : toutes les valeurs sous forme de chaines ; elles viennent de config.ini (valeurs par defaut du
// firmware) et de l'etat du moteur (decor).
WebReply WebConfig::load() {
  Engine::Deco d;
  int sb = 0;
  eng_.runOnMain([&] {
    d = eng_.deco();
    sb = cfg_.screenBrightness;
  });
  auto s = [&](const char *k, const std::string &def = "") { return cfg_.get(k, def); };
  auto fb = [&](const char *k, bool def) { return b01(cfg_.getBool(k, def)); };
  std::string j = "{";
  auto add = [&](const std::string &k, const std::string &v, bool first = false) {
    if (!first) j += ",";
    j += "\"" + k + "\":\"" + jsonEscape(v) + "\"";
  };
  add("brightness", std::to_string((sb * 100 + 127) / 255), true);
  add("info", b01(cfg_.showInfo));
  add("playlist", cfg_.playlistName);
  add("random", b01(cfg_.playlistRandom));
  add("feat_vpinball_dmd", fb("feat_vpinball_dmd", true));
  add("recalbox_native_mode", fb("recalbox_native_mode", false));
  add("feat_ra_window", fb("feat_ra_window", true));
  add("feat_hiscore_ingame", fb("feat_hiscore_ingame", true));
  add("feat_hiscore_browse", fb("feat_hiscore_browse", false));
  add("feat_info_ingame", fb("feat_info_ingame", false));
  add("feat_info_browse", fb("feat_info_browse", true));
  add("feat_description_ingame", fb("feat_description_ingame", false));
  add("feat_description_browse", fb("feat_description_browse", true));
  add("feat_ra_ingame", fb("feat_ra_ingame", true));
  add("feat_ra_browse", fb("feat_ra_browse", false));
  add("feat_repeat_cycles", std::to_string(clampi(cfg_.getInt("feat_repeat_cycles", 3), 0, 20)));
  add("feat_repeat_browse_cycles", std::to_string(clampi(cfg_.getInt("feat_repeat_browse_cycles", 3), 0, 20)));
  add("feat_dwell_seconds", std::to_string(clampi(cfg_.getInt("feat_dwell_seconds", 5), 3, 30)));
  add("feat_demo_follow", fb("feat_demo_follow", true));
  add("feat_clip_follow", fb("feat_clip_follow", true));
  add("feat_theme_follow", fb("feat_theme_follow", true));
  add("feat_lang_priority", b01(s("feat_lang_priority", "0") == "1"));
  add("feat_reels_mode", std::to_string(clampi(cfg_.getInt("feat_reels_mode", 0), 0, 2)));  // v398 : logos reels (0 / 1 / 2)
  add("reels_auto_new", b01(s("reels_auto_new", "0") == "1"));
  add("reels_themes", reelsSanitizeList(s("reels_themes")));  // noms assainis [A-Za-z0-9._-] : pas d'echappement JSON necessaire
  add("reels_seen", reelsSanitizeList(s("reels_seen")));
  // version du demon (fichier PI_VERSION de l'installation, meme format M.mm que le firmware) : la page la compare a la cle « pi » du manifeste GitHub.
  // Vide (lancement a la main, installation sans ce fichier) : pas d'avis de mise a jour.
  add("fw_version", installPrefix().empty() ? "" : readSmall(installPrefix() + "/PI_VERSION"));
  add("platform", "rpi");  // materiel annonce au toolkit (l'ESP32 annonce "esp32") : compare la bonne version publiee (cle pi / version du manifeste)
  {
    std::lock_guard<std::mutex> lk(eng_.themeControl().m);
    add("scripts_bundle", eng_.themeControl().scriptVer);
  }
  add("ota_last", installPrefix().empty() ? s("ota_last") : readSmall(installPrefix() + "/ota_last"));  // ecrit par dmd_update.py : en-cours / ok:<version> / ok:deja ... / echec:<raison>
  const uint64_t first = udp_.firstPacketMs();
  add("rb_age", std::to_string(first ? (unsigned long)((nowMs() - first) / 1000UL) : 0UL));
  add("wifi_enabled", fb("wifi_enabled", true));
  {  // config.ini sans reseau (carte preparee par le toolkit : le Wi-Fi est dans NetworkManager) : on montre le reseau ACTUEL du Pi
    std::string ssid = s("wifi_ssid"), pass = s("wifi_password");
    if (ssid.empty()) {
      const std::string act = runArgs({"nmcli", "-t", "-f", "NAME,TYPE,DEVICE", "connection", "show", "--active"}, 5000);
      std::istringstream in(act);
      std::string line;
      while (std::getline(in, line)) {
        const size_t a = line.find(":802-11-wireless:");
        if (a == std::string::npos) continue;
        std::string name = line.substr(0, a);
        if (name == "recalboxdmd-ap") continue;
        ssid = trim(runArgs({"nmcli", "-g", "802-11-wireless.ssid", "connection", "show", name}, 5000));
        pass = trim(runArgs({"nmcli", "-s", "-g", "802-11-wireless-security.psk", "connection", "show", name}, 5000));
        break;
      }
    }
    add("wifi_ssid", ssid);
    add("wifi_password", pass);
  }
  add("playlist_last", s("playlist_last"));
  {  // une IP statique incomplete (IP, passerelle ou masque vide) n'est jamais « activee » (le firmware la desactive aussi a l'enregistrement)
    const bool complete = !s("wifi_static_ip").empty() && !s("wifi_gateway").empty() && !s("wifi_subnet").empty();
    add("wifi_static_enabled", b01(cfg_.getBool("wifi_static_enabled", false) && complete));
  }
  add("wifi_static_ip", s("wifi_static_ip"));
  add("wifi_gateway", s("wifi_gateway"));
  add("wifi_subnet", s("wifi_subnet"));
  add("wifi_dns1", s("wifi_dns1"));
  add("wifi_dns2", s("wifi_dns2"));
  {  // connexion actuelle du Pi (pre-remplissage de l'IP fixe)
    const NetInfo ni = netInfo();
    if (!ni.ip.empty()) {
      add("wifi_cur_ip", ni.ip);
      add("wifi_cur_gw", ni.gw);
      add("wifi_cur_mask", ni.mask);
      add("wifi_cur_dns", ni.dns);
    }
  }
  add("bluetooth_enabled", fb("bluetooth_enabled", false));
  add("bluetooth_name", s("bluetooth_name"));
  {  // liste des Recalbox : recalbox_ips, sinon recalbox_ip seule
    std::string csv = s("recalbox_ips");
    if (csv.empty()) csv = s("recalbox_ip");
    add("recalbox_ip", csv);
  }
  add("recalbox_active", udp_.activeRecalbox());
  add("btn_level", std::to_string(btn_ ? btn_->level() : -1));
  add("btn_low_ms", std::to_string(btn_ ? btn_->lowMs() : 0U));
  add("btn_armed", b01(btn_ && btn_->armed()));
  add("boot_reason", "raspberry-pi");
  add("uptime_s", std::to_string((unsigned long)(nowMs() / 1000UL)));
  add("recalbox_target", std::to_string(clampi(cfg_.getInt("recalbox_target", 0), 0, 4)));
  add("clock_enabled", b01(d.mode[0] != 0));
  add("clock_theme", std::to_string(cfg_.getInt("clock_theme", -1)));
  add("clock_themes_mask", std::to_string(cfg_.getInt("clock_themes_mask", 0x7FFF)));
  {
    const int32_t nc = hexColor(s("clock_color"));
    const unsigned v = nc >= 0 ? (unsigned)nc : 0xFF2878u;  // rose du theme Neon par defaut
    char b[10];
    std::snprintf(b, sizeof(b), "#%06X", v & 0xFFFFFF);
    add("clock_neon_color", b);
    add("clock_neon_color_enabled", b01(nc >= 0));
  }
  add("clock_interval", std::to_string(d.gifs));
  add("clock_interval_min", std::to_string(d.minutes));
  add("clock_duration", std::to_string(d.dur[0]));
  {
    std::string tz = s("tz");
    if (tz.empty() && std::getenv("TZ")) tz = std::getenv("TZ");
    add("clock_tz", tz);
  }
  for (int i = 0; i < 5; i++) {
    add("clk_anim_" + std::to_string(i), colorHex(hexColor(s(("clock_anim_" + std::to_string(i)).c_str()))));
    add("clk_digit_" + std::to_string(i), colorHex(hexColor(s(("clock_digit_" + std::to_string(i)).c_str()))));
  }
  add("bright_sched_on", fb("bright_sched_on", false));  // planificateur de luminosite
  add("bright_sched", formatBrightSched(parseBrightSched(s("bright_sched"))));
  add("btn_enabled", fb("btn_enabled", false));
  add("btn_pin", std::to_string(cfg_.getInt("btn_pin", 21)));
  add("btn_act_short", std::to_string(cfg_.getInt("btn_act_short", 1)));
  add("btn_act_double", std::to_string(cfg_.getInt("btn_act_double", 3)));
  add("btn_act_long", std::to_string(cfg_.getInt("btn_act_long", 2)));
  add("btn_act_vlong", std::to_string(cfg_.getInt("btn_act_vlong", 6)));
  add("btn_long_ms", std::to_string(cfg_.getInt("btn_long_ms", 1500)));
  add("btn_vlong_ms", std::to_string(cfg_.getInt("btn_vlong_ms", 10000)));
  add("deco_qtheme", wg("qtheme"));
  add("deco_amb", std::to_string(clampi(std::atoi(wg("amb", "0").c_str()), 0, 8)));
  add("deco_lava_oil", colorHex(hexColor(wg("lava_oil"))));
  add("deco_lava_wax", colorHex(hexColor(wg("lava_wax"))));
  for (int n = 1; n <= 7; n++) {
    add("deco_amb_ca_" + std::to_string(n), colorHex(hexColor(wg("ambc_" + std::to_string(n) + "_a"))));
    add("deco_amb_cb_" + std::to_string(n), colorHex(hexColor(wg("ambc_" + std::to_string(n) + "_b"))));
  }
  for (int i = 1; i <= 10; i++) {  // DECO_EV_n = nom|AAAAMMJJ|annuel[|actif] -> nom|AAAA-MM-JJ|annuel|actif
    const std::string v = wg("ev_" + std::to_string(i));
    std::string out;
    const size_t a = v.find('|'), b = (a == std::string::npos) ? std::string::npos : v.find('|', a + 1);
    if (a != std::string::npos && a > 0 && b != std::string::npos) {
      const unsigned long dt = std::strtoul(v.substr(a + 1, b - a - 1).c_str(), nullptr, 10);
      const size_t c4 = v.find('|', b + 1);
      const bool on = !(c4 != std::string::npos && std::atoi(v.substr(c4 + 1).c_str()) == 0);
      char d8[16];
      std::snprintf(d8, sizeof(d8), "%04lu-%02lu-%02lu", dt / 10000, (dt / 100) % 100, dt % 100);
      if (dt) out = v.substr(0, a) + "|" + d8 + "|" + (std::atoi(v.substr(b + 1).c_str()) != 0 ? "1" : "0") + "|" + (on ? "1" : "0");
    }
    add("deco_ev_" + std::to_string(i), out);
  }
  {
    char b[32];
    std::snprintf(b, sizeof(b), "%.4f", std::atof(wg("lat", "0").c_str()));
    add("deco_lat", b);
    std::snprintf(b, sizeof(b), "%.4f", std::atof(wg("lon", "0").c_str()));
    add("deco_lon", b);
  }
  add("deco_place", wg("place"));
  add("deco_bg", b01(d.widgetsOnly));
  add("deco_trigger", std::to_string(d.trig));
  add("deco_main_order", std::to_string(d.mainOrder));
  add("deco_add_mode", std::to_string(d.addMode));
  add("deco_add_count", std::to_string(d.addCount));
  for (int i = 0; i < 11; i++) {
    add("deco_mode_" + std::to_string(i), std::to_string(d.mode[i]));
    add("deco_dur_" + std::to_string(i), std::to_string(d.dur[i]));
  }
  j += "}";
  WebReply r;
  r.contentType = "application/json";
  r.body = j;
  return r;
}

// ------------------------------------------------------------------------------------------------------------------------ /save
// Memes champs, memes validations, memes refus que handleWebConfigSave() ; au lieu de reecrire tout config.ini a partir de variables, on
// met a jour les cles recues (les autres lignes du fichier sont conservees telles quelles). Les noms de cles gardent la casse du firmware
// (CLOCK_*, WIDGET_*, BTN_*, TZ en majuscules) : la meme carte reste utilisable sur l'ESP32.
WebReply WebConfig::save(const WebRequest &r) {
  auto has = [&](const char *k) { return r.has(k); };
  auto arg = [&](const char *k) { return r.arg(k); };
  auto toInt = [](const std::string &s) { return std::atoi(s.c_str()); };
  WebReply bad;
  bad.status = 409;

  Engine::Deco d;
  eng_.runOnMain([&] { d = eng_.deco(); });

  // GARDE-FOU « page non chargee » : une page dont /load a echoue renvoie tous ses champs vides ; l'enregistrer effacerait le WiFi / la playlist
  if ((has("wifi_ssid") && arg("wifi_ssid").empty() && !cfg_.get("wifi_ssid").empty() && !has("wifi_clear")) ||
      (has("playlist") && arg("playlist").empty() && !cfg_.playlistName.empty())) {
    logf("WEB", "/save refuse : champs WiFi / playlist vides alors qu'une valeur est configuree (page non chargee ?)");
    bad.body = "ERR: la page n'a pas charge la configuration (champs vides) - rien enregistre, rechargez la page";
    return bad;
  }
  if (has("deco_bg") && toInt(arg("deco_bg")) == 1) {  // widgets seuls : au moins un widget actif (horloge comprise), refus AVANT toute modification
    bool any = false;
    for (int i = 0; i < 11; i++) {
      if (i == 6) continue;
      const std::string km = "deco_mode_" + std::to_string(i);
      const int m = has(km.c_str()) ? toInt(arg(km.c_str())) : (int)d.mode[i];
      if (m != 0) any = true;
    }
    if (!any) {
      logf("WEB", "/save refuse : fond widgets seuls sans aucun widget actif");
      const std::string lg = cfg_.uiLanguage;
      bad.body = lg == "en" ? "ERROR: Widgets-only background needs at least one widget (or the clock) - nothing saved."
                 : lg == "es" ? "ERROR: el fondo Solo widgets necesita al menos un widget (o el reloj) - no se guardo nada."
                              : "ERREUR : le fond Widgets seuls demande au moins un widget (ou l'horloge) - rien n'a ete enregistre.";
      return bad;
    }
  }

  {  // diagnostic : valeurs du decor reellement recues dans cette requete
    std::string seen;
    for (int i = 0; i < 11; i++) {
      const std::string km = "deco_mode_" + std::to_string(i), kd = "deco_dur_" + std::to_string(i);
      if (has(km.c_str())) seen += " m" + std::to_string(i) + "=" + arg(km.c_str());
      if (has(kd.c_str())) seen += " d" + std::to_string(i) + "=" + arg(kd.c_str());
    }
    logf("WEB", "/save decor recu :%s | bg=%s", seen.empty() ? " (aucune cle du decor)" : seen.c_str(), has("deco_bg") ? arg("deco_bg").c_str() : "-");
  }
  std::map<std::string, std::string> kv;
  std::string note;
  const auto markNet = [&]() {  // reseau modifie : un simple rechargement du demon ne suffit pas toujours (profil NetworkManager)
    for (const char *k : {"wifi_ssid", "wifi_password", "wifi_static_enabled", "wifi_static_ip", "wifi_gateway", "wifi_subnet", "wifi_dns1", "wifi_dns2"})
      if (has(k)) {
        std::string cur = cfg_.get(k), now = arg(k);
        if (std::string(k) == "wifi_static_enabled") { if (cur.empty()) cur = "0"; if (now.empty()) now = "0"; }
        if (now != cur) netDirty_ = true;
      }
  };
  markNet();
  const std::string curPlaylist = cfg_.playlistName;
  std::string playlist = curPlaylist, playlistLast = cfg_.get("playlist_last");
  bool widgetsOnly = d.widgetsOnly;

  // luminosite : la valeur courante (apercu compris) est celle que le firmware reecrit a chaque enregistrement
  {
    int b = (cfg_.screenBrightness * 100 + 127) / 255;
    if (has("brightness")) {
      const int v = toInt(arg("brightness"));
      if (v >= 0 && v <= 100) b = v;
    }
    kv["brightness"] = std::to_string(b);
  }
  if (has("playlist")) {  // choisir une playlist redonne le fond « Playlist de GIFs »
    const std::string np = arg("playlist");
    if (!np.empty()) {
      playlistLast = np;
      if (np != curPlaylist) widgetsOnly = false;
    }
    playlist = np;
  }
  if (has("random")) kv["random"] = b01(arg("random") == "1");
  if (has("info")) kv["info"] = b01(arg("info") == "1");
  for (const char *k : {"feat_hiscore_ingame", "feat_hiscore_browse", "feat_info_ingame", "feat_info_browse", "feat_description_ingame", "feat_description_browse",
                        "feat_ra_ingame", "feat_ra_browse", "feat_vpinball_dmd", "recalbox_native_mode", "feat_ra_window", "feat_demo_follow", "feat_clip_follow",
                        "feat_theme_follow", "feat_lang_priority"})
    if (has(k)) kv[k] = b01(arg(k) == "1");
  if (has("feat_reels_mode")) kv["feat_reels_mode"] = std::to_string(clampi(toInt(arg("feat_reels_mode")), 0, 2));  // v398
  if (has("reels_auto_new")) kv["reels_auto_new"] = b01(arg("reels_auto_new") == "1");
  if (has("reels_themes")) kv["reels_themes"] = reelsSanitizeList(arg("reels_themes"));
  if (has("feat_repeat_cycles")) kv["feat_repeat_cycles"] = std::to_string(clampi(toInt(arg("feat_repeat_cycles")), 0, 20));
  if (has("feat_repeat_browse_cycles")) kv["feat_repeat_browse_cycles"] = std::to_string(clampi(toInt(arg("feat_repeat_browse_cycles")), 0, 20));
  if (has("feat_dwell_seconds")) kv["feat_dwell_seconds"] = std::to_string(clampi(toInt(arg("feat_dwell_seconds")), 3, 30));

  // reseau : n'est applique que si la requete porte un SSID non vide (un brouillon d'une autre page ne doit pas effacer le WiFi) ; le Pi
  // n'applique rien lui-meme (reseau gere par le systeme), les valeurs sont seulement conservees
  const bool netOk = has("wifi_ssid") && !arg("wifi_ssid").empty();
  if (has("wifi_ssid")) kv["wifi_ssid"] = arg("wifi_ssid");
  if (has("wifi_password")) kv["wifi_password"] = arg("wifi_password");
  if (netOk) {
    for (const char *k : {"wifi_enabled", "wifi_static_enabled", "bluetooth_enabled"})
      if (has(k)) kv[k] = b01(arg(k) == "1");
    for (const char *k : {"wifi_static_ip", "wifi_gateway", "wifi_subnet", "wifi_dns1", "wifi_dns2"})
      if (has(k)) kv[k] = arg(k);
    // une IP statique incomplete n'est jamais enregistree comme active
    const bool se = kv.count("wifi_static_enabled") ? kv["wifi_static_enabled"] == "1" : cfg_.getBool("wifi_static_enabled", false);
    const std::string sip = kv.count("wifi_static_ip") ? kv["wifi_static_ip"] : cfg_.get("wifi_static_ip");
    const std::string sgw = kv.count("wifi_gateway") ? kv["wifi_gateway"] : cfg_.get("wifi_gateway");
    const std::string ssn = kv.count("wifi_subnet") ? kv["wifi_subnet"] : cfg_.get("wifi_subnet");
    if (se && (sip.empty() || sgw.empty() || ssn.empty())) kv["wifi_static_enabled"] = "0";
  }
  if (has("bluetooth_name")) kv["bluetooth_name"] = arg("bluetooth_name");
  if (has("bright_sched_on")) kv["BRIGHT_SCHED_ON"] = b01(arg("bright_sched_on") == "1");  // planificateur de luminosite (page Reglages / /config/brightness)
  if (has("bright_sched")) kv["BRIGHT_SCHED"] = formatBrightSched(parseBrightSched(arg("bright_sched")));

  // Recalbox : liste d'IP (virgules) + cible ; l'IP active est conservee si elle reste dans la liste (rbApplyFromWeb)
  bool rbChanged = false;
  if (has("recalbox_ip")) {
    std::vector<std::string> list;
    std::string cur;
    const std::string csv = arg("recalbox_ip");
    for (size_t i = 0; i <= csv.size(); i++) {
      const char c = (i < csv.size()) ? csv[i] : ',';
      if (c == ',' || c == ';' || c == ' ') {
        const std::string t = trimStr(cur);
        if (!t.empty() && list.size() < 4 && std::find(list.begin(), list.end(), t) == list.end()) list.push_back(t);
        cur.clear();
      } else {
        cur += c;
      }
    }
    const std::string old = cfg_.get("recalbox_ip");
    std::string active = std::find(list.begin(), list.end(), old) != list.end() ? old : "";
    int target = has("recalbox_target") ? toInt(arg("recalbox_target")) : cfg_.getInt("recalbox_target", 0);
    if (target < 0 || target > 4) target = 0;
    if (target > (int)list.size()) target = 0;
    if (!list.empty()) {
      if (target > 0) active = list[(size_t)target - 1];
      else if (active.empty()) active = list[0];
    }
    std::string joined;
    for (const std::string &s : list) joined += (joined.empty() ? "" : ",") + s;
    kv["recalbox_ips"] = joined;
    kv["recalbox_ip"] = active;
    kv["recalbox_target"] = std::to_string(target);
    rbChanged = true;
  }

  // horloge
  if (has("clock_theme")) kv["CLOCK_THEME"] = std::to_string(toInt(arg("clock_theme")));
  if (has("clock_themes_mask")) {
    const int m = toInt(arg("clock_themes_mask"));
    kv["CLOCK_THEMES_MASK"] = std::to_string((m > 0 && m <= 0x7FFF) ? m : 0x7FFF);
  }
  if (has("clock_neon_color_enabled") || has("clock_neon_color")) {
    int32_t cur = hexColor(cfg_.get("clock_color"));
    bool enabled = cur >= 0;
    int32_t color = cur >= 0 ? cur : (int32_t)0xFF2878;
    if (has("clock_neon_color_enabled")) enabled = arg("clock_neon_color_enabled") == "1";
    if (has("clock_neon_color")) {
      const int32_t c = hexColor(arg("clock_neon_color"));
      if (c >= 0) color = c;
    }
    kv["CLOCK_COLOR"] = enabled ? colorHex(color) : "";
  }
  if (has("clock_interval")) kv["CLOCK_INTERVAL"] = std::to_string(toInt(arg("clock_interval")));
  if (has("clock_interval_min")) kv["CLOCK_INTERVAL_MIN"] = std::to_string(toInt(arg("clock_interval_min")));
  for (int i = 0; i < 5; i++) {  // couleurs des themes d'horloge animes (vide = automatique)
    const std::string ka = "clk_anim_" + std::to_string(i), kd = "clk_digit_" + std::to_string(i);
    if (has(ka.c_str())) kv["CLOCK_ANIM_" + std::to_string(i)] = colorHex(hexColor(arg(ka.c_str())));
    if (has(kd.c_str())) kv["CLOCK_DIGIT_" + std::to_string(i)] = colorHex(hexColor(arg(kd.c_str())));
  }
  if (has("clock_tz") && !arg("clock_tz").empty()) kv["TZ"] = arg("clock_tz");

  // bouton physique (valeurs bornees comme au chargement du firmware)
  if (has("btn_enabled")) kv["BTN_ENABLED"] = b01(toInt(arg("btn_enabled")) != 0);
  if (has("btn_pin")) {
    const int v = toInt(arg("btn_pin"));
    if (v == 0 || v == 21 || v == 2 || (v >= 34 && v <= 39)) kv["BTN_PIN"] = std::to_string(v);
  }
  for (const auto &p : std::vector<std::pair<const char *, const char *>>{{"btn_act_short", "BTN_ACT_SHORT"}, {"btn_act_double", "BTN_ACT_DOUBLE"},
                                                                          {"btn_act_long", "BTN_ACT_LONG"}, {"btn_act_vlong", "BTN_ACT_VLONG"}})
    if (has(p.first)) {
      const int v = toInt(arg(p.first));
      if (v >= 0 && v <= 12) kv[p.second] = std::to_string(v);
    }
  if (has("btn_long_ms")) {
    const int v = toInt(arg("btn_long_ms"));
    if (v >= 500 && v <= 5000) kv["BTN_LONG_MS"] = std::to_string(v);
  }
  if (has("btn_vlong_ms")) {
    const int v = toInt(arg("btn_vlong_ms"));
    if (v >= 3000 && v <= 30000) kv["BTN_VLONG_MS"] = std::to_string(v);
  }

  // decor (page widgets)
  if (has("deco_qtheme")) kv["WIDGET_QTHEME"] = quoteSlugList(arg("deco_qtheme"));
  if (has("deco_amb")) {
    const int v = toInt(arg("deco_amb"));
    if (v >= 0 && v <= 8) kv["WIDGET_AMB"] = std::to_string(v);
  }
  if (has("deco_lava_oil")) kv["WIDGET_LAVA_OIL"] = colorHex(hexColor(arg("deco_lava_oil")));
  if (has("deco_lava_wax")) kv["WIDGET_LAVA_WAX"] = colorHex(hexColor(arg("deco_lava_wax")));
  for (int n = 1; n <= 7; n++) {
    const std::string ka = "deco_amb_ca_" + std::to_string(n), kb = "deco_amb_cb_" + std::to_string(n);
    if (has(ka.c_str())) kv["WIDGET_AMBC_" + std::to_string(n) + "_a"] = colorHex(hexColor(arg(ka.c_str())));
    if (has(kb.c_str())) kv["WIDGET_AMBC_" + std::to_string(n) + "_b"] = colorHex(hexColor(arg(kb.c_str())));
  }
  for (int i = 1; i <= 10; i++) {  // nom|AAAA-MM-JJ|annuel|actif -> nom|AAAAMMJJ|annuel|actif (ligne vide = evenement efface)
    const std::string k = "deco_ev_" + std::to_string(i);
    if (!has(k.c_str())) continue;
    const std::string v = trimStr(arg(k.c_str()));
    std::string out;
    const size_t a = v.find('|'), b = (a == std::string::npos) ? std::string::npos : v.find('|', a + 1);
    if (a != std::string::npos && a > 0 && b != std::string::npos && b > a) {
      const std::string nm = v.substr(0, a), dt = v.substr(a + 1, b - a - 1);
      std::string o;
      for (size_t q = 0; q < nm.size() && o.size() < 40; q++) {
        const unsigned char ch = (unsigned char)nm[q];
        o += (ch < 32 || ch > 126 || ch == '"' || ch == '\\' || ch == '|') ? '?' : (char)ch;
      }
      const int y = std::atoi(dt.substr(0, 4).c_str()), mo = std::atoi(dt.substr(5, 2).c_str()), dd = std::atoi(dt.substr(8, 2).c_str());
      const size_t c4 = v.find('|', b + 1);
      const int on = (c4 != std::string::npos && std::atoi(v.substr(c4 + 1).c_str()) == 0) ? 0 : 1;
      if (!o.empty() && y >= 2000 && y <= 2100 && mo >= 1 && mo <= 12 && dd >= 1 && dd <= 31)
        out = o + "|" + std::to_string((unsigned long)y * 10000UL + (unsigned long)mo * 100UL + (unsigned long)dd) + "|" + (std::atoi(v.substr(b + 1).c_str()) != 0 ? "1" : "0") + "|" + std::to_string(on);
    }
    kv["WIDGET_EV_" + std::to_string(i)] = out;
  }
  if (has("deco_lat")) {
    const double v = std::atof(arg("deco_lat").c_str());
    if (v >= -90.0 && v <= 90.0) { char b[32]; std::snprintf(b, sizeof(b), "%.4f", v); kv["WIDGET_LAT"] = b; }
  }
  if (has("deco_lon")) {
    const double v = std::atof(arg("deco_lon").c_str());
    if (v >= -180.0 && v <= 180.0) { char b[32]; std::snprintf(b, sizeof(b), "%.4f", v); kv["WIDGET_LON"] = b; }
  }
  if (has("deco_place")) {
    const std::string s = trimStr(arg("deco_place"));
    std::string o;
    for (size_t k = 0; k < s.size() && o.size() < 24; k++) {
      const unsigned char ch = (unsigned char)s[k];
      o += (ch < 32 || ch > 126 || ch == '"' || ch == '\\') ? '?' : (char)ch;
    }
    kv["WIDGET_PLACE"] = o;
  }
  if (has("deco_bg")) {  // fond : playlist de GIFs ou widgets seuls, couple a la playlist
    const bool nb = toInt(arg("deco_bg")) == 1;
    if (nb && !widgetsOnly) {  // -> widgets seuls : la playlist en cours est deselectionnee (memorisee)
      if (!playlist.empty()) { playlistLast = playlist; playlist.clear(); }
    } else if (!nb && widgetsOnly) {  // -> playlist de GIFs : on reactive la derniere
      if (playlist.empty() && !playlistLast.empty()) { playlist = playlistLast; note = " RESTORED"; }
      else if (playlist.empty()) note = " NOPLAYLIST";
    }
    widgetsOnly = nb;
  }
  if (has("deco_trigger")) kv["WIDGET_TRIGGER"] = toInt(arg("deco_trigger")) == 1 ? "1" : "0";
  if (has("deco_main_order")) kv["WIDGET_MAIN_ORDER"] = toInt(arg("deco_main_order")) == 1 ? "1" : "0";
  if (has("deco_add_mode")) kv["WIDGET_ADD_MODE"] = toInt(arg("deco_add_mode")) == 1 ? "1" : "0";
  if (has("deco_add_count")) {
    const int v = toInt(arg("deco_add_count"));
    if (v >= 1 && v <= 10) kv["WIDGET_ADD_COUNT"] = std::to_string(v);
  }
  int mode0 = d.mode[0], dur0 = d.dur[0];
  bool anyW = false;
  for (int i = 0; i < 11; i++) {
    const std::string km = "deco_mode_" + std::to_string(i), kd = "deco_dur_" + std::to_string(i);
    int m = d.mode[i];
    if (has(km.c_str())) {
      const int v = toInt(arg(km.c_str()));
      if (v >= 0 && v <= 2) { m = v; kv["WIDGET_MODE_" + std::to_string(i)] = std::to_string(v); }
    }
    if (has(kd.c_str())) {
      const int v = toInt(arg(kd.c_str()));
      if (v >= 1 && v <= 120) {
        kv["WIDGET_DUR_" + std::to_string(i)] = std::to_string(v);
        if (i == 0) dur0 = v;
      }
    }
    if (i == 0) mode0 = m;
    if (i != 6 && m != 0) anyW = true;
  }
  kv["CLOCK_ENABLED"] = mode0 != 0 ? "1" : "0";  // l'horloge est l'element 0 du decor
  kv["CLOCK_DURATION"] = std::to_string(dur0);
  kv["WIDGET_BG"] = widgetsOnly ? "1" : "0";
  kv["playlist"] = playlist;
  kv["playlist_last"] = playlistLast;
  if (widgetsOnly ? anyW : !playlist.empty()) kv["first_boot"] = "0";  // configuration valide : fin du premier demarrage

  bool ok = false;
  eng_.runOnMain([&] {
    ok = cfg_.writeKeys(fs_, kv);
    if (ok) eng_.reloadConfig();
  });
  if (!ok) {
    WebReply e;
    e.status = 500;
    e.body = "ERR: SD write failed (config.ini conserve)";
    return e;
  }
  if (rbChanged) {  // liste / cible de Recalbox modifiees : le DMD s'annonce tout de suite a la nouvelle Recalbox active (elle renvoie son etat)
    udp_.rbReload();
    udp_.sendHello();
  }
  udp_.sendFeatures();  // diffuse le nouveau reglage aux scripts de la Recalbox
  logf("WEB", "config.ini enregistre (%zu cles)", kv.size());
  WebReply okr;
  okr.body = "OK" + note;
  return okr;
}

// ------------------------------------------------------------------------------------------------------------------------ /handshake, /nstat
WebReply WebConfig::handshake(const WebRequest &) {
  WebReply r;
  r.body = eng_.zedmd().handshake();
  if (r.body.empty()) {
    r.status = 404;
    r.body = "disabled";
  }
  return r;
}

WebReply WebConfig::nstat() {
  WebReply r;
  r.contentType = "application/json";
  r.body = eng_.zedmd().statusJson();
  return r;
}

// ------------------------------------------------------------------------------------------------------------------------ routes
void WebConfig::registerRoutes() {
  const char *forever = "public, max-age=31536000, immutable";
  web_.any("/", [this](const WebRequest &r) { return page(wifi_ && wifi_->apActive() ? "ap.html" : "menu.html", r); });  // point d'acces ouvert : page de choix du reseau (comme handleWebConfigRoot)
  web_.any("/cfg.css", [this, forever](const WebRequest &) { return asset("cfg.css", "text/css", forever); });
  web_.any("/cfg.js", [this, forever](const WebRequest &) { return asset("cfg.js", "application/javascript", forever); });
  web_.any("/native.js", [this](const WebRequest &) { return asset("native.js", "application/javascript", "no-cache"); });
  for (const auto &p : std::vector<std::pair<const char *, const char *>>{
           {"/config/settings", "settings.html"}, {"/config/basic", "basic.html"},   {"/config/playlist", "playlist.html"}, {"/config/network", "network.html"},
           {"/config/clock", "clock.html"},       {"/config/button", "button.html"}, {"/config/brightness", "brightness.html"}, {"/config/widgets", "widgets.html"},   {"/config/deco", "widgets.html"},
           {"/config/place", "place.html"},       {"/config/events", "events.html"}, {"/config/clockcolors", "clockcolors.html"}, {"/config/diag", "diag.html"},
           {"/config/quotes", "quotes.html"},     {"/config/media", "media.html"}}) {
    const std::string file = p.second;
    web_.any(p.first, [this, file](const WebRequest &r) { return page(file, r); });
  }
  web_.any("/favicon.ico", [](const WebRequest &) { WebReply r; r.status = 204; return r; });
  web_.any("/load", [this](const WebRequest &) { return load(); });
  web_.any("/reels_installed", [this](const WebRequest &) {  // v398 : themes installes sur la SD (liste a cocher des logos reels), hors logos-reels et dossiers « _ »
    WebReply r;
    r.contentType = "application/json";
    std::string j = "[";
    bool first = true;
    for (const std::string &n : fs_.list("/systems/_defaults/_themes")) {
      bool ok = !n.empty() && n.size() <= 31 && n != "logos-reels" && n[0] != '_' && fs_.isDir("/systems/_defaults/_themes/" + n);
      for (size_t i = 0; ok && i < n.size(); i++) {
        const char ch = n[i];
        ok = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '.' || ch == '_' || ch == '-';
      }
      if (ok) {
        if (!first) j += ",";
        j += "\"" + n + "\"";
        first = false;
      }
    }
    r.body = j + "]";
    return r;
  });
  web_.post("/reels-refresh", [this](const WebRequest &) {  // v3.74 : fin d'un envoi de logos reels par le toolkit
    WebReply r;
    eng_.runOnMain([&] { eng_.reelsRefresh(); });
    r.body = "OK";
    return r;
  });
  web_.post("/save", [this](const WebRequest &r) { return save(r); });
  web_.any("/lang", [this](const WebRequest &) {
    WebReply r;
    r.contentType = "application/json";
    r.body = "{\"language\":\"" + cfg_.uiLanguage + "\",\"first_boot\":\"" + b01(cfg_.getBool("first_boot", true)) + "\"}";
    return r;
  });
  web_.post("/save-language", [this](const WebRequest &r) {
    WebReply out;
    const std::string l = r.arg("language");
    if (l != "fr" && l != "en" && l != "es") { out.status = 400; out.body = "ERR: langue inconnue"; return out; }
    eng_.runOnMain([&] {
      cfg_.writeKeys(fs_, {{"language", l}});
      eng_.reloadConfig();
    });
    out.body = "OK";
    return out;
  });
  web_.post("/set-brightness", [this](const WebRequest &r) {  // apercu live (RAM seulement)
    WebReply out;
    if (!r.has("value")) { out.status = 400; out.body = "ERR: missing value"; return out; }
    const int b = std::atoi(r.arg("value").c_str());
    if (b < 0 || b > 100) { out.status = 400; out.body = "ERR: out of range"; return out; }
    eng_.runOnMain([&] { eng_.previewBrightness(b); });
    out.body = "OK";
    return out;
  });
  web_.post("/clock-preview", [this](const WebRequest &r) {
    WebReply out;
    if (!r.has("theme")) { out.status = 400; out.body = "ERR: missing theme"; return out; }
    const std::string th = r.arg("theme");
    if (th.size() > 1 && th[0] == 'w') eng_.postMain([this, th] { eng_.clockPreview(th); });  // un widget dure plusieurs secondes : on repond tout de suite, un nouvel apercu interrompt celui-ci
    else eng_.runOnMain([&] { eng_.clockPreview(th); });
    out.body = "OK";
    return out;
  });
  web_.post("/dmd-pause", [this](const WebRequest &r) {
    WebReply out;
    if (!r.has("msg")) { out.status = 400; out.body = "ERR: missing msg"; return out; }
    const std::string c = r.arg("color");
    const uint16_t color = c == "1" ? 0x07E0 : c == "2" ? 0xF800 : 0xFFFF;
    const std::string msg = r.arg("msg");
    const bool hasTitle = r.has("title");  // titre facultatif de la ligne 1 (le toolkit annonce ainsi « INSTALLATION », « COPIE 35% »...)
    const std::string title = r.arg("title");
    eng_.runOnMain([&] {
      if (hasTitle) eng_.setConfigTitle(title);
      eng_.webPause(msg, color);
    });
    out.body = "OK";
    return out;
  });
  // Battement des pages ouvertes (toutes les 20 s) et annonce de depart (pagehide : croix, fermeture du navigateur) : voir Engine::tickWebWatchdog()
  web_.post("/dmd-hb", [this](const WebRequest &) {
    WebReply out;
    eng_.webTouch();
    out.body = "OK";
    return out;
  });
  web_.post("/dmd-leave", [this](const WebRequest &) {
    WebReply out;
    eng_.webLeaveSoon(10000);  // reprise dans 10 s, annulee si une page se rouvre (F5, autre page de configuration)
    out.body = "OK";
    return out;
  });
  // Mode diagnostic Wi-Fi / Recalbox (page /config/diag, meme page que le firmware ESP32) : /diag-data = mesures ET signe de vie (8 s sans appel : le DMD quitte le mode)
  web_.any("/diag-data", [this](const WebRequest &) {
    WebReply out;
    eng_.diagWebTouch();
    out.contentType = "application/json";
    out.body = eng_.diagJson();
    return out;
  });
  web_.post("/diag-start", [this](const WebRequest &) {
    WebReply out;
    eng_.diagWebTouch();
    eng_.runOnMain([&] { eng_.diagStart(true); });
    out.body = "OK";
    return out;
  });
  web_.post("/diag-stop", [this](const WebRequest &) {
    WebReply out;
    eng_.runOnMain([&] { eng_.diagStop(); });
    out.body = "OK";
    return out;
  });  web_.post("/dmd-resume", [this](const WebRequest &) {
    WebReply out;
    out.body = "OK REBOOT";
    eng_.runOnMain([&] { eng_.webResume(); });
    return out;
  });
  web_.post("/dmd-open", [this](const WebRequest &r) {
    WebReply out;
    if (!r.has("msg")) { out.status = 400; out.body = "ERR: missing msg"; return out; }
    std::string ip = r.localAddr;
    eng_.runOnMain([&] { eng_.webOpen(r.arg("msg"), ip); });
    out.body = "OK " + r.arg("msg") + " " + ip;
    return out;
  });
  web_.any("/reboot", [this](const WebRequest &) {
    WebReply out;
    if (netDirty_.exchange(false)) {  // reseau modifie : redemarrage complet du Raspberry Pi (la page attend son retour puis l'annonce)
      logf("WEB", "reseau modifie : redemarrage du Raspberry Pi");
      out.body = "PIREBOOT";
      eng_.runOnMain([&] { eng_.webPause("Redemarrage du Pi...", 0xFFE0); });
      std::thread([] {
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));  // laisse partir la reponse HTTP
        runArgs({"systemctl", "reboot"}, 15000);
      }).detach();
      return out;
    }
    out.body = "DAEMON";  // sinon : le demon se relance lui-meme (2 a 3 s)
    eng_.runOnMain([&] { eng_.handle("reboot", ""); });
    return out;
  });
  web_.any("/nstat", [this](const WebRequest &) { return nstat(); });
  web_.any("/handshake", [this](const WebRequest &r) { return handshake(r); });
  // reseau et mise a jour : geres par le systeme du Pi, sans objet ici
  // mise a jour du demon depuis la derniere release GitHub « pi-v... » : dmd_update.py tourne DETACHE du service (systemd-run) car le service est redemarre
  web_.post("/ota/start", [this](const WebRequest &) {
    WebReply r;
    const std::string prefix = installPrefix();
    const std::string script = prefix + "/bin/dmd_update.py";
    if (prefix.empty() || access(script.c_str(), X_OK) != 0) {
      r.status = 501;
      r.body = "ERR: mise a jour indisponible (dmd_update.py absent de l'installation)";
      return r;
    }
    {
      std::ofstream st(prefix + "/ota_last", std::ios::trunc);
      st << "en-cours\n";
    }
    int rc = -1;
    runArgs({"systemd-run", "--no-block", "--collect", "--unit=recalboxdmd-update", script}, 15000, &rc);
    if (rc != 0) {  // deja en cours (unite existante) ou systemd-run indisponible
      std::ofstream st(prefix + "/ota_last", std::ios::trunc);
      st << "echec:lancement impossible (mise a jour deja en cours ?)\n";
      r.status = 500;
      r.body = "ERR: lancement impossible";
      return r;
    }
    logf("WEB", "mise a jour du demon lancee (dmd_update.py)");
    eng_.runOnMain([&] { eng_.webPause("Mise a jour...", 0xFFE0); });
    r.body = "OK";
    return r;
  });
  web_.any("/scan-wifi", [this](const WebRequest &) {  // reseaux Wi-Fi visibles (NetworkManager), dedoublonnes par nom, signal decroissant
    if (wifi_ && wifi_->apActive()) {  // point d'acces ouvert : le scan est impossible, on sert la liste prise AVANT son ouverture
      std::string j = "[";
      const std::vector<std::string> v = wifi_->scanCache();
      for (size_t i = 0; i < v.size(); i++) j += (i ? "," : "") + std::string("\"") + jsonEscape(v[i]) + "\"";
      WebReply r;
      r.contentType = "application/json";
      r.body = j + "]";
      return r;
    }
    runCmd("nmcli dev wifi rescan >/dev/null 2>&1 &");  // la liste suivante sera plus fraiche (la page redemande tant qu'elle est vide)
    std::istringstream in(runCmd("nmcli -t -f SIGNAL,SSID dev wifi list 2>/dev/null"));
    std::map<std::string, int> best;
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
      if (ssid.empty()) continue;  // reseau cache
      auto it = best.find(ssid);
      if (it == best.end() || sig > it->second) best[ssid] = sig;
    }
    std::vector<std::pair<int, std::string>> v;
    for (const auto &p : best) v.push_back({p.second, p.first});
    std::sort(v.begin(), v.end(), [](const auto &a, const auto &b) { return a.first > b.first; });
    std::string j = "[";
    for (size_t i = 0; i < v.size(); i++) j += (i ? "," : "") + std::string("\"") + jsonEscape(v[i].second) + "\"";
    WebReply r;
    r.contentType = "application/json";
    r.body = j + "]";
    return r;
  });
  registerMediaRoutes();
  web_.post("/save-ap", [this](const WebRequest &r) {  // reseau choisi dans la page du point d'acces : ecrit dans config.ini puis applique par WifiManager
    WebReply out;
    if (!wifi_) { out.status = 501; out.body = "ERR: gestionnaire Wi-Fi indisponible"; return out; }
    if (!r.has("wifi_ssid")) { out.status = 400; out.body = "ERR: missing SSID"; return out; }
    const std::string ssid = trim(r.arg("wifi_ssid")), pass = trim(r.arg("wifi_password")), ip = trim(r.arg("wifi_static_ip"));
    if (ssid.empty()) { out.status = 400; out.body = "ERR: missing SSID"; return out; }
    // meme jeu de cles que handleWebConfigSaveAP ; first_boot n'est PAS touche (il ne passe a 0 que dans /save, voir DECISIONS.md)
    cfg_.writeKeys(fs_, {{"wifi_enabled", "1"}, {"wifi_ssid", ssid}, {"wifi_password", pass}, {"wifi_static_enabled", ip.empty() ? "0" : "1"}, {"wifi_static_ip", ip}});
    logf("WEB", "AP save : reseau « %s »%s", ssid.c_str(), ip.empty() ? "" : " (IP fixe)");
    wifi_->requestApply();
    out.body = "OK";
    return out;
  });
}
