// themecontrol.h -- theme / langue / region annonces par la Recalbox (CMD=theme, CMD=themeopt, CMD=scriptver).
// Le firmware les traite « a la reception » (jamais soumis a la regle du paquet unique) ; ici le thread reseau les depose et la
// boucle principale les applique au tour suivant (les index et le dessin ne sont pas thread-safe). Les deux commandes envoyees
// coup sur coup (themeopt puis theme) sont appliquees ENSEMBLE : un seul changement de selection, un seul redessin.
#pragma once
#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

// Nom de dossier de theme assaini : [A-Za-z0-9._-], 31 max, ni "." ni ".." (meme regle pour CMD=theme et config.ini).
inline std::string themeCleanName(const std::string &raw) {
  size_t a = raw.find_first_not_of(" \t\r\n"), b = raw.find_last_not_of(" \t\r\n");
  const std::string t = (a == std::string::npos) ? "" : raw.substr(a, b - a + 1);
  std::string clean;
  for (char c : t) {
    if (clean.size() >= 31) break;
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-') clean += c;
  }
  if (clean == ".." || clean == ".") clean.clear();
  return clean;
}

// Deux lettres minuscules (langue ou region) ou chaine vide.
inline std::string themeTwoLetters(const std::string &raw) {
  size_t a = raw.find_first_not_of(" \t\r\n"), b = raw.find_last_not_of(" \t\r\n");
  std::string s = (a == std::string::npos) ? "" : raw.substr(a, b - a + 1);
  for (char &c : s)
    if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
  return (s.size() == 2 && s[0] >= 'a' && s[0] <= 'z' && s[1] >= 'a' && s[1] <= 'z') ? s : std::string();
}

struct ThemeControl {
  std::mutex m;
  std::string theme, lang, region;  // derniere valeur recue (deja assainie)
  bool haveTheme = false, haveOpt = false;
  std::vector<std::string> rbw;      // CMD=rbw : donnees des widgets Recalbox, analysees par la boucle principale (dans l'ordre d'arrivee)
  std::string scriptVer;            // CMD=scriptver : version des scripts Recalbox (chiffres, 6 max), pour la page web
  std::atomic<uint32_t> version{0};

  void setTheme(const std::string &arg) {
    std::lock_guard<std::mutex> lk(m);
    theme = themeCleanName(arg);
    haveTheme = true;
    version++;
  }
  // « <langue>,<region> » (ex. « fr,eu »), 2 lettres minuscules chacune, sinon ignore
  void setOpt(const std::string &arg) {
    std::lock_guard<std::mutex> lk(m);
    std::string a = arg;
    for (char &c : a)
      if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
    const size_t comma = a.find(',');
    lang = themeTwoLetters(comma == std::string::npos ? a : a.substr(0, comma));
    region = themeTwoLetters(comma == std::string::npos ? std::string() : a.substr(comma + 1));
    haveOpt = true;
    version++;
  }
  void pushRbw(const std::string &arg) {
    std::lock_guard<std::mutex> lk(m);
    if (rbw.size() < 16) rbw.push_back(arg);
  }
  std::vector<std::string> takeRbw() {
    std::lock_guard<std::mutex> lk(m);
    std::vector<std::string> v;
    v.swap(rbw);
    return v;
  }
  void setScriptVer(const std::string &arg) {
    std::string v;
    for (char c : arg)
      if (c >= '0' && c <= '9' && v.size() < 6) v += c;
    std::lock_guard<std::mutex> lk(m);
    scriptVer = v;
  }
};
