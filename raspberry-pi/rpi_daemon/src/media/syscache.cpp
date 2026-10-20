#include "syscache.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <sstream>

#include "log.h"

const char BUCKET_LETTERS[] = "#ABCDEFGHIJKLMNOPQRSTUVWXYZ";

char bucketLetterForFilename(const std::string &fname) {
  if (fname.empty()) return '#';
  char first = (char)std::toupper((unsigned char)fname[0]);
  return std::isalpha((unsigned char)first) ? first : '#';
}

const SysCache::Entry *SysCache::find(const std::string &sys) const {
  for (const auto &e : e_)
    if (e.name == sys) return &e;  // comparaison sensible a la casse, comme le firmware
  return nullptr;
}

char SysCache::defaultType(const std::string &sys) const {
  const Entry *e = find(sys);
  return e ? e->type : '?';
}

char SysCache::slowFlag(const std::string &sys) const {
  const Entry *e = find(sys);
  return e ? e->slow : 'N';
}

char SysCache::bucketSlowFlag(const std::string &sys, char bucketLetter) const {
  const Entry *e = find(sys);
  if (!e) return 'N';  // systeme totalement inconnu
  bucketLetter = (char)std::toupper((unsigned char)bucketLetter);
  const char *p = bucketLetter ? std::strchr(BUCKET_LETTERS, bucketLetter) : nullptr;
  if (p) {
    char c = e->perLetter[p - BUCKET_LETTERS];
    if (c == 'L' || c == 'N') return c;  // donnee par bucket disponible
  }
  return e->slow;  // repli : flag systeme agrege
}

bool SysCache::load(const SdFs &fs) {
  std::vector<uint8_t> data;
  if (!fs.readAll("/systems_cache.dat", data)) return false;
  e_.clear();
  std::istringstream in(std::string(data.begin(), data.end()));
  std::string line;
  while (std::getline(in, line)) {
    line = trim(line);
    if (line.size() < 3) continue;
    char val = line[0];
    if (val != 'g' && val != 'p' && val != 'B') continue;
    std::string rest = trim(line.substr(2));
    size_t sp2 = rest.find(' ');
    Entry en;
    en.name = (sp2 == std::string::npos) ? rest : rest.substr(0, sp2);
    en.type = val;
    en.slow = 'N';
    std::string bucketStr;  // vide = pas de donnee par bucket (ancien format a 3 champs)
    if (sp2 != std::string::npos) {
      std::string flag = trim(rest.substr(sp2 + 1));
      if (!flag.empty()) {
        en.slow = flag[0];
        size_t sp3 = flag.find(' ');
        if (sp3 != std::string::npos) bucketStr = trim(flag.substr(sp3 + 1));
      }
    }
    std::memset(en.perLetter, '?', BUCKET_COUNT);
    if (bucketStr.size() == BUCKET_COUNT) {
      for (int k = 0; k < BUCKET_COUNT; k++) {
        char c = bucketStr[k];
        if (c == 'l') c = 'L';
        if (c == 'n') c = 'N';
        if (c == 'L' || c == 'N') en.perLetter[k] = c;  // un octet invalide ne casse que son bucket
      }
    }
    e_.push_back(en);
  }
  logf("CACHE", "systems_cache.dat charge : %zu systemes", e_.size());
  return !e_.empty();
}

// Compte recursivement les .raw565 / .raw565pack d'un dossier, avec arret au-dela de `limit`.
static void countOver(const SdFs &fs, const std::string &dir, int limit, int &png, int &gif, bool &pngOver,
                      bool &gifOver) {
  if (pngOver && gifOver) return;
  for (const auto &name : fs.list(dir)) {
    if (pngOver && gifOver) return;
    std::string sub = dir + "/" + name;
    if (fs.isDir(sub)) {
      countOver(fs, sub, limit, png, gif, pngOver, gifOver);
    } else if (name.size() > 7 && name.compare(name.size() - 7, 7, ".raw565") == 0) {
      if (++png > limit) pngOver = true;
    } else if (name.size() > 11 && name.compare(name.size() - 11, 11, ".raw565pack") == 0) {
      if (++gif > limit) gifOver = true;
    }
  }
}

void SysCache::build(const SdFs &fs) {
  e_.clear();
  for (const auto &sys : fs.list("/systems")) {
    if (!fs.isDir("/systems/" + sys) || sys == "_defaults") continue;
    Entry en;
    en.name = sys;
    std::string base = "/systems/_defaults/" + sys;
    bool hasPack = fs.exists(base + ".raw565pack") && fs.exists(base + ".meta");
    bool hasRaw = fs.exists(base + ".raw565");
    en.type = (hasPack && hasRaw) ? 'B' : hasPack ? 'g' : hasRaw ? 'p' : '?';
    int png = 0, gif = 0;
    bool pngOver = false, gifOver = false;
    countOver(fs, "/systems/" + sys, 5000, png, gif, pngOver, gifOver);  // seuil 5000, comme l'outil PC
    en.slow = (pngOver || gifOver) ? 'L' : 'N';
    std::memset(en.perLetter, '?', BUCKET_COUNT);  // pas de detail par bucket dans ce repli
    e_.push_back(en);
  }
  logf("CACHE", "%zu systemes indexes (reconstruction)", e_.size());
}

void SysCache::save(const SdFs &fs) const {
  std::string path = fs.root() + "/systems_cache.dat";
  FILE *f = std::fopen(path.c_str(), "wb");
  if (!f) return;
  for (const auto &e : e_) std::fprintf(f, "%c %s %c\n", e.type, e.name.c_str(), e.slow);
  std::fclose(f);
}

bool SysCache::loadOrBuild(const SdFs &fs) {
  if (load(fs)) return true;
  if (!fs.isDir("/systems")) return false;
  build(fs);
  save(fs);
  return !e_.empty();
}
