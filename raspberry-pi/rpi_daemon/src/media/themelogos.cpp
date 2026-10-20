#include "themelogos.h"

#include <cstring>

#include "log.h"
#include "themecontrol.h"

namespace {
const char *THEMES_DIR = "/systems/_defaults/_themes/";
const char *DEFAULTS_DIR = "/systems/_defaults/";

// ES nomme un systeme virtuel de genre « genre-<x> » mais cherche son logo de theme sous « auto-<x> » (v243) ; "" = pas d'alias
std::string autoAlias(const std::string &sys) {
  if (sys.compare(0, 6, "genre-") == 0) return "auto-" + sys.substr(6);
  if (sys.compare(0, 5, "auto-") != 0) return "auto-" + sys;
  return "";
}
}  // namespace

uint32_t ThemeLogos::hash(const std::string &s) {  // FNV-1a 32 bits sur le nom en minuscules
  uint32_t h = 2166136261u;
  for (unsigned char c : s) {
    h ^= (uint8_t)((c >= 'A' && c <= 'Z') ? c + 32 : c);
    h *= 16777619u;
  }
  return h;
}

bool ThemeLogos::Idx::has(const std::string &name) const {
  if (!loaded) return true;  // pas d'index => on ne sait pas => on essaie
  const uint32_t h = ThemeLogos::hash(name);
  size_t lo = 0, hi = v.size();
  while (lo < hi) {  // recherche dichotomique dans le tableau trie
    const size_t mid = (lo + hi) / 2;
    if (v[mid] == h) return true;
    if (v[mid] < h) lo = mid + 1;
    else hi = mid;
  }
  return false;
}

bool ThemeLogos::loadIdx(const SdFs &fs, const std::string &path, Idx &out) {
  out = Idx();
  std::vector<uint8_t> raw;
  if (!fs.readAll(path, raw) || raw.size() < 4 || raw.size() % 4 != 0) return false;
  out.v.resize(raw.size() / 4);
  for (size_t i = 0; i < out.v.size(); i++)  // petit-boutiste (uint32 de l'ESP32)
    out.v[i] = (uint32_t)raw[i * 4] | ((uint32_t)raw[i * 4 + 1] << 8) | ((uint32_t)raw[i * 4 + 2] << 16) | ((uint32_t)raw[i * 4 + 3] << 24);
  out.loaded = true;
  return true;
}

void ThemeLogos::begin(const Config &cfg, const SdFs &fs) {
  follow_ = cfg.getBool("feat_theme_follow", true);
  langPriority_ = cfg.get("feat_lang_priority", "0") == "1";
  wTheme_ = themeCleanName(cfg.get("theme_last"));
  wLang_ = themeTwoLetters(cfg.get("lang_last"));
  wRegion_ = themeTwoLetters(cfg.get("region_last"));
  pTheme_ = wTheme_;
  pLang_ = wLang_;
  pRegion_ = wRegion_;
  if (!wTheme_.empty() || !wLang_.empty() || !wRegion_.empty()) {  // derniere selection reappliquee des le demarrage
    defaultsLangSelect(fs, wLang_);
    apply(fs);
  }
}

bool ThemeLogos::setWanted(const SdFs &fs, const std::string &theme, const std::string &lang, const std::string &region) {
  wTheme_ = theme;
  wLang_ = lang;
  wRegion_ = region;
  defaultsLangSelect(fs, wLang_);
  return apply(fs);
}

bool ThemeLogos::setFollow(const SdFs &fs, bool follow) {
  follow_ = follow;
  return apply(fs);
}

bool ThemeLogos::takePersist() {
  if (wTheme_ == pTheme_ && wLang_ == pLang_ && wRegion_ == pRegion_) return false;
  pTheme_ = wTheme_;
  pLang_ = wLang_;
  pRegion_ = wRegion_;
  return true;
}

void ThemeLogos::defaultsLangSelect(const SdFs &fs, const std::string &lang) {
  if (lang == defLangRequested_) return;
  defLangRequested_ = lang;
  defLangIdx_ = Idx();
  defLang_.clear();
  defLangDirOk_ = false;
  if (lang.size() != 2 || lang == "en") {
    logf("LANG", "images par defaut : base (anglais)");
    return;
  }
  const std::string dir = std::string(DEFAULTS_DIR) + lang;
  if (!fs.isDir(dir)) {
    logf("LANG", "%s absent de la SD -> images par defaut en anglais", dir.c_str());
    return;
  }
  defLang_ = lang;
  defLangDirOk_ = true;
  loadIdx(fs, dir + "/_index.bin", defLangIdx_);
  logf("LANG", "images par defaut : %s%s", lang.c_str(),
       defLangIdx_.loaded ? (" (index " + std::to_string(defLangIdx_.v.size()) + ")").c_str() : " (sans index)");
}

// Applique le theme voulu selon l'option : suivi actif -> theme de la Recalbox (+ langue / region annoncees), sinon logos par defaut
bool ThemeLogos::apply(const SdFs &fs) {
  const std::string clean = follow_ ? wTheme_ : std::string();
  const std::string key = clean + "|" + wLang_ + "|" + wRegion_;
  if (key == requested_) return false;  // HELLO / PING repetes : rien a refaire
  requested_ = key;
  themeIdx_ = langIdx_ = regionIdx_ = Idx();
  langDir_.clear();
  regionDir_.clear();
  logoTheme_ = clean;
  if (clean.empty()) {
    logf("THEME", "logos systeme : (defaut)");
    return true;
  }
  const std::string dir = std::string(THEMES_DIR) + clean;
  if (!fs.isDir(dir)) {
    logf("THEME", "dossier %s absent de la SD -> logos par defaut", dir.c_str());
    logoTheme_.clear();
    return true;
  }
  loadIdx(fs, dir + "/_index.bin", themeIdx_);
  auto variant = [&](const std::string &code, std::string &vdir, Idx &vidx) {
    const std::string d = dir + "/" + code;
    if (!fs.isDir(d)) return;
    vdir = d;
    loadIdx(fs, d + "/_index.bin", vidx);
  };
  if (wLang_.size() == 2 && wLang_ != "en") variant("l_" + wLang_, langDir_, langIdx_);
  if (wRegion_.size() == 2 && wRegion_ != "us") variant("r_" + wRegion_, regionDir_, regionIdx_);
  logf("THEME", "logos systeme : %s%s variantes : langue=%s region=%s", clean.c_str(),
       themeIdx_.loaded ? (" (index " + std::to_string(themeIdx_.v.size()) + " logos)").c_str() : " (sans index -> recherche directe)",
       langDir_.empty() ? "-" : ("l_" + wLang_).c_str(), regionDir_.empty() ? "-" : ("r_" + wRegion_).c_str());
  return true;
}

bool ThemeLogos::anyLogo(const std::string &n) const {
  return (!langDir_.empty() && langIdx_.has(n)) || (!regionDir_.empty() && regionIdx_.has(n)) || themeIdx_.has(n);
}

std::vector<std::string> ThemeLogos::candidates(const std::string &sys) const {
  std::vector<std::string> cand;
  if (logoTheme_.empty()) return cand;
  std::string tn = sys;  // nom cherche dans le theme (alias auto-<x> si le theme n'a pas le nom recu)
  if (!anyLogo(tn)) {
    const std::string alt = autoAlias(sys);
    if (!alt.empty() && anyLogo(alt)) tn = alt;
  }
  const bool themeLangHas = !langDir_.empty() && langIdx_.has(tn);
  // le DMD affiche ce que la Recalbox affiche (v246) : logo du theme (langue, region, base) AVANT toute image par defaut ;
  // sauf option feat_lang_priority : image par defaut TRADUITE disponible (index charge) et pas de variante de langue dans le theme
  const bool langDefaultWins = langPriority_ && !themeLangHas && defLangDirOk_ && defLangIdx_.loaded && defLangIdx_.has(sys);
  if (themeLangHas) cand.push_back(langDir_ + "/" + tn + ".raw565");
  if (!langDefaultWins && !regionDir_.empty() && regionIdx_.has(tn)) cand.push_back(regionDir_ + "/" + tn + ".raw565");
  if (!langDefaultWins && themeIdx_.has(tn)) cand.push_back(std::string(THEMES_DIR) + logoTheme_ + "/" + tn + ".raw565");
  return cand;
}

std::string ThemeLogos::defaultsLangPath(const std::string &rawPath) const {
  if (!defLangDirOk_) return "";
  const size_t pl = std::strlen(DEFAULTS_DIR);
  if (rawPath.compare(0, pl, DEFAULTS_DIR) != 0 || rawPath.size() < 7 || rawPath.compare(rawPath.size() - 7, 7, ".raw565") != 0) return "";
  const std::string name = rawPath.substr(pl);
  // default.raw565 et les fichiers « _* » ne sont jamais traduits
  if (name.empty() || name.find('/') != std::string::npos || name[0] == '_' || name == "default.raw565") return "";
  if (!defLangIdx_.has(name.substr(0, name.size() - 7))) return "";
  return std::string(DEFAULTS_DIR) + defLang_ + "/" + name;
}
