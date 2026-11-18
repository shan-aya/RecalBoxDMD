// gifmatch.h -- correspondance TITRE DU JEU <-> NOM DE GIF (GIFs du jeu, firmware v422 / demon Pi 3.84).
// COPIE EXACTE de la regle du firmware ESP32 (RecalBox_DMD.ino : ggNormW / ggScanMaster) et de tools/gifmatch.py ; cas de test communs :
// tools/test_gifmatch.py (Python) et rpi_daemon/tools/test_gifmatch.cpp (celui-ci). Toute modification de la regle doit etre faite PARTOUT.
//
//  - normalisation : minuscules, lettres/chiffres ASCII seuls, accents latins courants replies sur la lettre de base (e, a, o, u... ; ae, oe, ss),
//    texte entre (...) et [...] ignore, tout autre caractere ignore ;
//  - MOTS d'un nom : coupes aux separateurs, aux passages minuscule -> Majuscule, lettre <-> chiffre et en fin de sigle (NEOGEOMetal -> NEOGEO + Metal) ;
//  - rang 3 : nom du GIF identique au titre ; rang 2 : le nom du GIF est contenu dans le titre ; rang 1 : le titre est contenu dans le nom du GIF ET couvre des MOTS
//    ENTIERS (« Aliens » ne correspond plus a ARCADE_AlienStorm01) ; le plus court des deux doit avoir au moins 4 caracteres (identite : 3) ;
//    un GIF dont le nom normalise a moins de 3 caracteres est ignore ; seul le meilleur rang est retenu (le DMD tire au hasard parmi eux).
#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// Texte normalise + debuts de mots (wb[i] = 1 si un mot commence au caractere i). Meme algorithme octet par octet que le firmware.
inline std::string ggNormW(const std::string &s, std::vector<uint8_t> *wbOut = nullptr) {
  static const char *const F[32] = {"a", "a", "a", "a", "a", "a", "ae", "c", "e", "e", "e", "e", "i", "i", "i", "i",
                                    "d", "n", "o", "o", "o", "o", "o",  "",  "o", "u", "u", "u", "u", "y", "",  "ss"};
  enum { NONE = 0, LOWER = 1, UPPER = 2, DIGIT = 3 };
  std::string out;
  std::vector<uint8_t> wb;
  int depth = 0, prev = NONE, prev2 = NONE;
  for (size_t i = 0; i < s.size(); i++) {
    unsigned char c = (unsigned char)s[i];
    if (c == '(' || c == '[') { depth++; prev = prev2 = NONE; continue; }
    if (c == ')' || c == ']') { if (depth > 0) depth--; prev = prev2 = NONE; continue; }
    if (depth > 0) continue;
    const char *fold = nullptr;
    int cls = LOWER;
    if (c == 0xC3 && i + 1 < s.size()) {  // lettres accentuees latines U+00C0..U+00FF
      const unsigned char d = (unsigned char)s[i + 1];
      i++;
      if (d >= 0x80 && d <= 0xBF) {
        fold = F[(d >= 0xA0 ? d - 0xA0 : d - 0x80) & 31];
        if (d == 0xBF) fold = "y";  // ÿ
      } else {
        fold = "";
      }
    } else if (c == 0xC5 && i + 1 < s.size()) {
      const unsigned char d = (unsigned char)s[i + 1];
      i++;
      fold = (d == 0x92 || d == 0x93) ? "oe" : "";
    } else if (c >= 0x80) {
      prev = prev2 = NONE;  // autre octet non ASCII : separateur
      continue;
    } else if (c >= 'A' && c <= 'Z') {
      cls = UPPER;
      c = (unsigned char)(c + 32);
    } else if (c >= 'a' && c <= 'z') {
      cls = LOWER;
    } else if (c >= '0' && c <= '9') {
      cls = DIGIT;
    } else {
      prev = prev2 = NONE;  // separateur
      continue;
    }
    const char one[2] = {(char)c, 0};
    for (const char *p = fold ? fold : one; *p; p++) {
      bool start = false;
      if (prev == NONE) start = true;
      else if (cls == DIGIT && prev != DIGIT) start = true;               // lettre -> chiffre
      else if (cls != DIGIT && prev == DIGIT) start = true;               // chiffre -> lettre
      else if (cls == UPPER && prev == LOWER) start = true;               // minuscule -> Majuscule
      else if (cls == LOWER && prev == UPPER && prev2 == UPPER && !out.empty()) wb[out.size() - 1] = 1;  // fin de sigle
      out += *p;
      wb.push_back(start ? 1 : 0);
      prev2 = prev;
      prev = cls;
    }
  }
  if (wbOut) *wbOut = wb;
  return out;
}

inline std::string ggNorm(const std::string &s) { return ggNormW(s, nullptr); }

// Cle normalisee + debuts de mots du nom de fichier d'un GIF (sans dossier ni .gif) ; false si ce n'est pas un .gif exploitable (nom normalise < 3).
inline bool ggGifInfo(const std::string &path, std::string &key, std::vector<uint8_t> &wb) {
  const size_t sl = path.find_last_of("/\\");
  const std::string base = (sl == std::string::npos) ? path : path.substr(sl + 1);
  if (base.size() < 5) return false;
  const std::string ext = base.substr(base.size() - 4);
  if (!(ext[0] == '.' && (ext[1] == 'g' || ext[1] == 'G') && (ext[2] == 'i' || ext[2] == 'I') && (ext[3] == 'f' || ext[3] == 'F'))) return false;
  key = ggNormW(base.substr(0, base.size() - 4), &wb);
  return key.size() >= 3;
}

// Cle seule (compatibilite) ; "" si inexploitable.
inline std::string ggGifKey(const std::string &path) {
  std::string k;
  std::vector<uint8_t> wb;
  return ggGifInfo(path, k, wb) ? k : std::string();
}

// Le titre apparait dans le nom du GIF en commencant ET en finissant a une frontiere de mot (au moins une occurrence).
inline bool ggWholeWords(const std::string &t, const std::string &g, const std::vector<uint8_t> &wb) {
  const size_t tl = t.size(), gl = g.size();
  size_t p = g.find(t);
  while (p != std::string::npos) {
    if (wb[p] && (p + tl == gl || wb[p + tl])) return true;
    p = g.find(t, p + 1);
  }
  return false;
}

// 3 identique, 2 le GIF est contenu dans le titre, 1 le titre est contenu dans le GIF en MOTS ENTIERS, 0 aucun.
inline int ggRank(const std::string &t, const std::string &g, const std::vector<uint8_t> &wb) {
  const size_t tl = t.size(), gl = g.size();
  if (gl == tl && g == t) return 3;
  if (gl >= 4 && gl < tl && t.find(g) != std::string::npos) return 2;
  if (tl >= 4 && tl < gl && g.find(t) != std::string::npos && ggWholeWords(t, g, wb)) return 1;
  return 0;
}
// ----------------------------------------------------------------------------- exclusions (v423 / Pi 3.85)
// /playlists/game_gifs_exclude.txt (cree et gere par le toolkit, Mode 21) : une regle par ligne « titre;GIF », `#` = commentaire.
//   Kong;ARCADE_DonkeyKong06.gif  ce GIF n'est jamais joue pour ce titre (compare sur le titre NORMALISE : tous les clones / regions)
//   Kong;*                        aucun GIF pour ce titre
//   *;ARCADE_1Player.gif          ce GIF n'est jamais joue en partie, pour aucun jeu
// Nom du GIF = celui du FICHIER (sans dossier), sans casse, « .gif » facultatif ; « *;* » est ignore. Meme resultat que gifmatch.parse_exclusions().
struct GgExcl {
  bool all = false;                     // aucun GIF pour ce titre
  std::vector<std::string> names;       // noms de fichier en minuscules, avec .gif
  bool has(const std::string &fileNameLower) const {
    for (const std::string &n : names) if (n == fileNameLower) return true;
    return false;
  }
};

inline GgExcl ggParseExclusions(const std::string &text, const std::string &title) {
  GgExcl ex;
  const std::string tn = ggNorm(title);
  size_t pos = 0;
  while (pos <= text.size()) {
    size_t e = text.find('\n', pos);
    if (e == std::string::npos) e = text.size();
    std::string ln = text.substr(pos, e - pos);
    pos = e + 1;
    auto trim = [](std::string s) {
      size_t a = 0, b = s.size();
      while (a < b && (s[a] == ' ' || s[a] == '\t' || s[a] == '\r')) a++;
      while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t' || s[b - 1] == '\r')) b--;
      return s.substr(a, b - a);
    };
    ln = trim(ln);
    if (ln.empty() || ln[0] == '#') continue;
    const size_t semi = ln.find(';');
    if (semi == std::string::npos) continue;
    const std::string left = trim(ln.substr(0, semi));
    std::string right = trim(ln.substr(semi + 1));
    if (left.empty() || right.empty()) continue;
    if (left != "*" && !(tn.size() >= 3 && ggNorm(left) == tn)) continue;
    if (right == "*") {
      if (left != "*") ex.all = true;
      continue;
    }
    const size_t sl = right.find_last_of("/\\");
    if (sl != std::string::npos) right = right.substr(sl + 1);
    for (char &c : right) if (c >= 'A' && c <= 'Z') c = (char)(c + 32);
    if (right.size() < 4 || right.compare(right.size() - 4, 4, ".gif") != 0) right += ".gif";
    ex.names.push_back(right);
  }
  return ex;
}

// ----------------------------------------------------------------------------- MODE AUTO STRICT (v424 / Pi 3.86)
// Sans dossier dedie /gifs_jeux : nom du GIF IDENTIQUE au titre apres retrait des prefixes « XXX_ » (jusqu'a 3) et d'un numero de serie final de 2 ou 3 chiffres commencant par 0 ; les GIFs de collection
// (premier prefixe HALLOWEEN_, XMAS_, QUIZ_, LOGO_, MOVIES_, XXX_, HAPPY_) ne sont jamais joues automatiquement. Meme regle que gifmatch.py strict_match() et le firmware ggStrict().
// base = nom de fichier SANS « .gif », tN = titre normalise.
inline bool ggStrictMatch(const std::string &base, const std::string &tN) {
  if (tN.size() < 3) return false;
  size_t pos[4];
  unsigned np = 1;
  pos[0] = 0;
  const size_t n = base.size();
  for (size_t p = 0; np < 4;) {
    size_t q = p;
    while (q < n && q - p < 10 && ((base[q] >= '0' && base[q] <= '9') || (base[q] >= 'a' && base[q] <= 'z') || (base[q] >= 'A' && base[q] <= 'Z'))) q++;
    if (q == p || q >= n || base[q] != '_') break;
    if (np == 1) {
      std::string tok = base.substr(p, q - p);
      for (char &c : tok) if (c >= 'A' && c <= 'Z') c = (char)(c + 32);
      for (const char *coll : {"halloween", "xmas", "quiz", "logo", "movies", "xxx", "happy"}) if (tok == coll) return false;
    }
    p = q + 1;
    pos[np++] = p;
  }
  for (unsigned k = 0; k < np; k++) {
    const std::string s = base.substr(pos[k]);
    if (s.empty()) break;
    if (ggNorm(s) == tN) return true;
    // numero de SERIE final : 2 ou 3 chiffres commencant par 0 (01, 02, 003) ; « Alien3 », « Sonic2 » sont des suites
    const size_t m = s.size();
    auto dg = [&](size_t i) { return s[i] >= '0' && s[i] <= '9'; };
    size_t e = m;
    if (m >= 4 && dg(m - 1) && dg(m - 2) && dg(m - 3) && s[m - 3] == '0') e = m - 3;
    else if (m >= 3 && dg(m - 1) && dg(m - 2) && s[m - 2] == '0') e = m - 2;
    if (e < m && ggNorm(s.substr(0, e)) == tN) return true;
  }
  return false;
}
