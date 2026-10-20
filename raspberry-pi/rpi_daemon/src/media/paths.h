// paths.h -- conventions de chemins du firmware (alphaSubdirPath, pngToRaw565Path, gifToRaw565PackPath...).
#pragma once
#include <cctype>
#include <string>

#include "sdfs.h"

inline bool endsWith(const std::string &s, const char *suf) {
  size_t n = std::char_traits<char>::length(suf);
  return s.size() >= n && s.compare(s.size() - n, n, suf) == 0;
}
inline bool startsWith(const std::string &s, const char *pre) {
  size_t n = std::char_traits<char>::length(pre);
  return s.size() >= n && s.compare(0, n, pre) == 0;
}

// "/systems/nes/zeld.raw565" -> "/systems/nes/Z/zeld.raw565" ; non alphabetique -> "#" ; _defaults jamais modifie.
inline std::string alphaSubdirPath(const std::string &path) {
  if (path.find("/_defaults/") != std::string::npos) return path;
  size_t ls = path.rfind('/');
  if (ls == std::string::npos) return path;
  std::string dir = path.substr(0, ls), fname = path.substr(ls + 1);
  if (fname.empty()) return path;
  char first = (char)std::toupper((unsigned char)fname[0]);
  std::string sub = std::isalpha((unsigned char)first) ? std::string(1, first) : std::string("#");
  return dir + "/" + sub + "/" + fname;
}

// Idempotent : ne rajoute jamais ".raw565" s'il y est deja (correctif v93 du firmware).
inline std::string pngToRaw565Path(const std::string &p) {
  if (p.size() >= 4 && endsWith(p, ".png")) return p.substr(0, p.size() - 4) + ".raw565";
  if (endsWith(p, ".raw565")) return p;
  return p + ".raw565";
}
inline std::string gifToRaw565PackPath(const std::string &g) {
  if (g.size() >= 4 && endsWith(g, ".gif")) return g.substr(0, g.size() - 4) + ".raw565pack";
  return g + ".raw565pack";
}
inline std::string gifToRaw565MetaPath(const std::string &g) {
  if (g.size() >= 4 && endsWith(g, ".gif")) return g.substr(0, g.size() - 4) + ".meta";
  return g + ".meta";
}

// "Preference" sous-dossier / plat (g_sdSubdirPreference) : la convention est celle de la carte entiere, on tente
// donc d'abord celle qui a reussi la derniere fois. Sur Linux le cout est nul, mais l'ordre fixe l'issue quand
// les deux variantes existent : on garde la meme regle que le firmware.
class SubdirPref {
 public:
  // Renvoie le chemin SD retenu (sous-dossier ou plat) ou "" si aucun n'existe.
  std::string pick(const SdFs &fs, const std::string &subPath, const std::string &flatPath) {
    if (pref_) {
      if (fs.exists(subPath)) return subPath;
      if (fs.exists(flatPath)) {
        pref_ = false;
        return flatPath;
      }
    } else {
      if (fs.exists(flatPath)) return flatPath;
      if (fs.exists(subPath)) {
        pref_ = true;
        return subPath;
      }
    }
    return "";
  }

 private:
  bool pref_ = true;
};
