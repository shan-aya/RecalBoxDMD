// sdfs.h -- acces au contenu de la "carte SD" (un dossier Linux) avec la semantique FAT du firmware :
// chemins "SD" commencant par '/', INSENSIBLES A LA CASSE (SD.exists()/SD.open() le sont sur FAT ; ext4 ne l'est pas).
// Les noms reels sont resolus composant par composant, avec un cache de listing par dossier.
#pragma once
#include <chrono>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

class SdFs {
 public:
  explicit SdFs(std::string root);
  const std::string &root() const { return root_; }

  // Chemin reel d'un chemin SD, ou "" s'il n'existe pas.
  std::string resolve(const std::string &sdPath) const;
  bool exists(const std::string &sdPath) const { return !resolve(sdPath).empty(); }
  bool isDir(const std::string &sdPath) const;
  int64_t fileSize(const std::string &sdPath) const;  // -1 si absent
  int64_t mtime(const std::string &sdPath) const;     // secondes, 0 si absent

  // Lit tout le fichier (ou les `maxBytes` premiers octets si > 0). false si absent/illisible.
  bool readAll(const std::string &sdPath, std::vector<uint8_t> &out, size_t maxBytes = 0) const;
  // Lit exactement n octets a l'offset donne.
  bool readAt(const std::string &sdPath, uint64_t off, void *buf, size_t n) const;
  // v3.93 : vrai si les noms de fichiers non ASCII (UTF-8) sont utilisables : la racine n'est PAS sur une FAT, ou la FAT est montee en iocharset=utf8.
  // Une FAT en iocharset=ascii (defaut du noyau) rend ces noms introuvables (« ? » a l'affichage, EINVAL a la creation). Lit /proc/mounts : a appeler au demarrage.
  bool utf8Names() const;
  // Noms reels des entrees d'un dossier (hors . et ..).
  std::vector<std::string> list(const std::string &sdDir) const;

 private:
  struct DirCache {
    std::chrono::steady_clock::time_point at;
    std::unordered_map<std::string, std::string> lower2real;
  };
  const DirCache &dirCache(const std::string &realDir, bool forceRefresh) const;

  std::string root_;
  mutable std::unordered_map<std::string, DirCache> dirs_;
};

std::string toLower(std::string s);
std::string trim(const std::string &s);
