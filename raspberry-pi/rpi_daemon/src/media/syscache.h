// syscache.h -- /systems_cache.dat : type d'image par defaut (g/p/B) et drapeau « lent » (L/N) par systeme et par
// bucket alphabetique. Meme format et memes regles que loadSysDefaultCache() / sysBucketSlowFlag() du firmware :
//   <g|p|B> <systeme> <L|N> <27 drapeaux L/N pour #ABC..Z>      (3e et 4e champs optionnels)
#pragma once
#include <string>
#include <vector>

#include "sdfs.h"

constexpr int BUCKET_COUNT = 27;
extern const char BUCKET_LETTERS[];  // "#ABCDEFGHIJKLMNOPQRSTUVWXYZ"

// 1ere lettre du nom de fichier en majuscule, '#' si non alphabetique ou vide (bucketLetterForFilename).
char bucketLetterForFilename(const std::string &fname);

class SysCache {
 public:
  // Charge le fichier ; s'il est absent, le reconstruit en parcourant /systems (buildSysDefaultCache) et l'ecrit.
  bool loadOrBuild(const SdFs &fs);
  bool load(const SdFs &fs);

  char defaultType(const std::string &sys) const;      // 'g','p','B' ou '?' (systeme inconnu)
  char slowFlag(const std::string &sys) const;         // flag agrege 'L'/'N' ('N' si inconnu)
  char bucketSlowFlag(const std::string &sys, char bucketLetter) const;  // par bucket, repli sur le flag agrege
  size_t count() const { return e_.size(); }

 private:
  struct Entry {
    std::string name;
    char type = '?';
    char slow = 'N';
    char perLetter[BUCKET_COUNT];
  };
  const Entry *find(const std::string &sys) const;
  void build(const SdFs &fs);
  void save(const SdFs &fs) const;
  std::vector<Entry> e_;
};
