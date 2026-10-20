#include "config.h"

#include <cstdio>
#include <cstdlib>
#include <sstream>

#include "log.h"

void Config::load(const SdFs &fs) {
  { std::lock_guard<std::mutex> lk(mu_); kv_.clear(); }
  std::vector<uint8_t> data;
  if (!fs.readAll("/config.ini", data)) {
    logf("CONFIG", "config.ini absent : valeurs par defaut");
    return;
  }
  std::istringstream in(std::string(data.begin(), data.end()));
  std::string line;
  while (std::getline(in, line)) {
    line = trim(line);
    if (line.empty() || line[0] == '#' || line[0] == ';') continue;
    size_t eq = line.find('=');
    if (eq == std::string::npos) continue;
    std::string key = toLower(trim(line.substr(0, eq)));
    std::string value = trim(line.substr(eq + 1));
    // Pas de troncature "commentaire en ligne" pour le SSID / mot de passe WiFi (donnees opaques, voir le firmware).
    if (key != "wifi_ssid" && key != "wifi_password") {
      size_t cp = value.find('#');
      if (cp != std::string::npos) value = value.substr(0, cp);
      cp = value.find(';');
      if (cp != std::string::npos) value = value.substr(0, cp);
      value = trim(value);
    }
    { std::lock_guard<std::mutex> lk(mu_); kv_[key] = value; }  // la derniere occurrence gagne, comme dans le firmware
  }

  refreshTyped();
  logf("CONFIG", "config.ini lu : %zu cles, playlist='%s' random=%d luminosite=%d/255", (size_t)kv_.size(), playlistName.c_str(), playlistRandom ? 1 : 0, screenBrightness);
}

void Config::refreshTyped() {
  // Valeurs typees (memes regles que loadConfig())
  playlistName = get("playlist");
  playlistRandom = get("random", "1") != "0";
  showInfo = get("info", "1") != "0";
  if (has("brightness")) {
    int b = std::atoi(get("brightness").c_str());
    if (b < 0) b = 0;
    if (b > 100) b = 100;
    screenBrightness = b * 255 / 100;  // map(constrain(v,0,100),0,100,0,255) en entiers
  }
  std::string lang = get("language");
  if (lang == "fr" || lang == "en" || lang == "es") uiLanguage = lang;
}

std::string Config::get(const std::string &key, const std::string &def) const {
  std::lock_guard<std::mutex> lk(mu_);
  auto it = kv_.find(key);
  return it == kv_.end() ? def : it->second;
}

bool Config::getBool(const std::string &key, bool def) const {
  std::lock_guard<std::mutex> lk(mu_);
  auto it = kv_.find(key);
  return it == kv_.end() ? def : it->second != "0";
}

int Config::getInt(const std::string &key, int def) const {
  std::lock_guard<std::mutex> lk(mu_);
  auto it = kv_.find(key);
  return it == kv_.end() ? def : std::atoi(it->second.c_str());
}

bool Config::writeKeys(const SdFs &fs, const std::map<std::string, std::string> &kv) {
  std::vector<uint8_t> data;
  bool existed = fs.exists("/config.ini");
  if (existed && !fs.readAll("/config.ini", data)) {
    logf("CFGWRITE", "ABANDON (lecture impossible, l'ecriture effacerait le contenu existant)");
    return false;
  }
  std::string all(data.begin(), data.end());
  for (const auto &p : kv) {
    std::string needle = p.first + "=";
    // premiere occurrence de "cle=" EN DEBUT DE LIGNE (le firmware cherche la sous-chaine n'importe ou : « TZ= » pouvait tomber au milieu d'une autre cle)
    size_t pos = all.find(needle);
    while (pos != std::string::npos && pos > 0 && all[pos - 1] != '\n') pos = all.find(needle, pos + 1);
    if (pos != std::string::npos) {
      size_t eol = all.find('\n', pos);
      if (eol == std::string::npos) eol = all.size();
      all = all.substr(0, pos) + needle + p.second + "\n" + (eol < all.size() ? all.substr(eol + 1) : std::string());
    } else {
      if (!all.empty() && all.back() != '\n') all += "\n";
      all += needle + p.second + "\n";
    }
    {
      std::lock_guard<std::mutex> lk(mu_);
      kv_[toLower(p.first)] = p.second;  // le fichier garde la casse du firmware (CLOCK_*, WIDGET_*...), la table est en minuscules
    }
  }
  std::string real = existed ? fs.resolve("/config.ini") : fs.root() + "/config.ini";
  std::string tmp = fs.root() + "/config.tmp";
  FILE *f = std::fopen(tmp.c_str(), "wb");
  if (!f) {
    logf("CFGWRITE", "ecriture de config.tmp impossible");
    return false;
  }
  bool ok = std::fwrite(all.data(), 1, all.size(), f) == all.size();
  ok = (std::fflush(f) == 0) && ok;
  std::fclose(f);
  if (!ok || std::rename(tmp.c_str(), real.c_str()) != 0) {
    logf("CFGWRITE", "echec d'ecriture / de remplacement de config.ini");
    std::remove(tmp.c_str());
    return false;
  }
  return true;
}
