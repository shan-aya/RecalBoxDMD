#include "gamescache.h"

#include <cctype>
#include <cstring>

#include "log.h"

int bigramIndex(const std::string &name) {
  if (name.empty()) return 0;
  char c1 = (char)std::toupper((unsigned char)name[0]);
  if (!std::isalpha((unsigned char)c1)) return 0;
  int base = 1 + (c1 - 'A') * 27;
  if (name.size() < 2) return base;
  char c2 = (char)std::toupper((unsigned char)name[1]);
  if (!std::isalpha((unsigned char)c2)) return base;
  return base + (c2 - 'A') + 1;
}

std::string bigramLabel(int bi) {
  if (bi == 0) return "#";
  int idx = bi - 1;
  int i1 = idx / 27, i2 = idx % 27;
  std::string s(1, (char)('A' + i1));
  if (i2 != 0) s += (char)('A' + i2 - 1);
  return s;
}

static uint32_t rd32(const uint8_t *p) { return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24); }

bool GamesCache::ensureLoaded(const SdFs &fs) {
  int64_t mt = fs.mtime("/games_cache.bin");
  int64_t sz = fs.fileSize("/games_cache.bin");
  if (sz < 0) {
    if (!idx_.empty() || mtime_ != -1) logf("GCACHE", "games_cache.bin absent");
    idx_.clear();
    data_.clear();
    mtime_ = -1;
    size_ = -1;
    return false;
  }
  if (mt == mtime_ && sz == size_ && !idx_.empty()) return true;
  mtime_ = mt;
  size_ = sz;
  idx_.clear();
  if (!fs.readAll("/games_cache.bin", data_) || data_.size() < 4) return false;
  uint32_t nb = rd32(data_.data());
  if (nb == 0 || data_.size() < 4 + (size_t)nb * 36) {
    logf("GCACHE", "index jeux rejete : %u systemes", nb);
    return false;
  }
  for (uint32_t i = 0; i < nb; i++) {
    const uint8_t *p = data_.data() + 4 + (size_t)i * 36;
    char nm[33];
    std::memcpy(nm, p, 32);
    nm[32] = 0;
    idx_.push_back({std::string(nm), rd32(p + 32)});
  }
  logf("GCACHE", "%zu systemes (games_cache.bin, %zu octets)", idx_.size(), data_.size());
  return true;
}

char GamesCache::find(const SdFs &fs, const std::string &sys, const std::string &game) {
  if (!ensureLoaded(fs)) return '?';
  size_t si = idx_.size();
  for (size_t i = 0; i < idx_.size(); i++)
    if (idx_[i].name == sys) {
      si = i;
      break;
    }
  if (si == idx_.size()) return '?';
  const uint32_t sysOff = idx_[si].offset;
  if ((uint64_t)sysOff + (uint64_t)NB_IDX * 4 > data_.size()) return '?';
  const uint8_t *table = data_.data() + sysOff;
  int bi = bigramIndex(game);
  uint32_t off = rd32(table + (size_t)bi * 4);
  if (off == 0 || off >= data_.size()) return '?';

  // fin de la tranche : prochain offset non nul de la table, sinon debut du systeme suivant, sinon fin de fichier
  uint32_t next = 0;
  for (int n = bi + 1; n < NB_IDX; n++) {
    uint32_t v = rd32(table + (size_t)n * 4);
    if (v != 0) {
      next = v;
      break;
    }
  }
  if (next == 0 || next <= off) {
    if (si + 1 < idx_.size()) next = idx_[si + 1].offset;
    if (next == 0 || next <= off) next = (uint32_t)data_.size();
  }
  if (next > data_.size()) next = (uint32_t)data_.size();

  std::string low = toLower(game);
  const uint8_t *ptr = data_.data() + off, *end = data_.data() + next;
  char bestType = '?';
  size_t bestLen = 0;
  while (ptr < end) {
    if (ptr + 1 >= end) break;
    char type = (char)*ptr++;
    const uint8_t *ns = ptr;
    while (ptr < end && *ptr != 0) ptr++;
    if (ptr >= end) break;
    size_t nl = (size_t)(ptr - ns);
    ptr++;
    if (nl == low.size()) {
      bool match = true;
      for (size_t i = 0; i < nl && match; i++)
        if (std::tolower(ns[i]) != (unsigned char)low[i]) match = false;
      if (match) return type;
    }
    if (nl < low.size() && nl > bestLen) {
      bool pfx = true;
      for (size_t i = 0; i < nl && pfx; i++)
        if (std::tolower(ns[i]) != (unsigned char)low[i]) pfx = false;
      if (pfx) {
        bestLen = nl;
        bestType = type;
      }
    }
  }
  return bestType;
}
