#include "gamescache.h"

#include <cctype>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <utility>

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

namespace {
struct CEntry {
  char type;
  std::string name;
};
struct CSys {
  std::string name;
  std::vector<std::vector<CEntry>> sl = std::vector<std::vector<CEntry>>(NB_IDX);
};

// Lit le fichier comme find() : fin d'une tranche = prochain offset non nul de la table, sinon debut du systeme suivant, sinon fin de fichier. Tout ce qui est incoherent -> false.
bool parseCache(const std::vector<uint8_t> &d, std::vector<CSys> &out) {
  if (d.size() < 4) return false;
  const uint32_t nb = rd32(d.data());
  if (nb == 0 || nb > 4096 || d.size() < 4 + (size_t)nb * 36) return false;
  std::vector<uint32_t> offs(nb);
  for (uint32_t i = 0; i < nb; i++) offs[i] = rd32(d.data() + 4 + (size_t)i * 36 + 32);
  for (uint32_t i = 0; i < nb; i++) {
    CSys s;
    char nm[33];
    std::memcpy(nm, d.data() + 4 + (size_t)i * 36, 32);
    nm[32] = 0;
    s.name = nm;
    const uint32_t so = offs[i];
    if ((uint64_t)so + (uint64_t)NB_IDX * 4 > d.size()) return false;
    const uint8_t *table = d.data() + so;
    const uint32_t sysEnd = (i + 1 < nb && offs[i + 1] > so) ? offs[i + 1] : (uint32_t)d.size();
    for (int bi = 0; bi < NB_IDX; bi++) {
      const uint32_t off = rd32(table + (size_t)bi * 4);
      if (off == 0) continue;
      if (off >= d.size() || off < so + (uint32_t)NB_IDX * 4) return false;
      uint32_t next = 0;
      for (int n = bi + 1; n < NB_IDX; n++) {
        const uint32_t v = rd32(table + (size_t)n * 4);
        if (v != 0) {
          next = v;
          break;
        }
      }
      if (next == 0 || next <= off) next = sysEnd;
      if (next <= off || next > d.size()) next = (uint32_t)d.size();
      const uint8_t *ptr = d.data() + off, *end = d.data() + next;
      while (ptr < end) {
        const char type = (char)*ptr++;
        const uint8_t *ns = ptr;
        while (ptr < end && *ptr != 0) ptr++;
        if (ptr >= end) return false;  // nom non termine
        s.sl[bi].push_back({type, std::string((const char *)ns, (size_t)(ptr - ns))});
        ptr++;
      }
    }
    out.push_back(std::move(s));
  }
  return true;
}

void wr32(uint8_t *p, uint32_t v) {
  p[0] = (uint8_t)v;
  p[1] = (uint8_t)(v >> 8);
  p[2] = (uint8_t)(v >> 16);
  p[3] = (uint8_t)(v >> 24);
}

// Meme ecriture que _write_cache_binary() du toolkit.
std::vector<uint8_t> serializeCache(const std::vector<CSys> &v) {
  const size_t header = 4 + v.size() * 36;
  std::vector<uint8_t> buf;
  std::vector<uint32_t> offs;
  for (const CSys &s : v) {
    offs.push_back((uint32_t)buf.size());
    const size_t tpos = buf.size();
    buf.resize(buf.size() + (size_t)NB_IDX * 4, 0);
    for (int li = 0; li < NB_IDX; li++) {
      if (s.sl[li].empty()) continue;
      wr32(buf.data() + tpos + (size_t)li * 4, (uint32_t)(header + buf.size()));
      for (const CEntry &e : s.sl[li]) {
        buf.push_back((uint8_t)e.type);
        buf.insert(buf.end(), e.name.begin(), e.name.end());
        buf.push_back(0);
      }
    }
  }
  std::vector<uint8_t> out(4 + v.size() * 36, 0);
  wr32(out.data(), (uint32_t)v.size());
  for (size_t i = 0; i < v.size(); i++) {
    uint8_t *p = out.data() + 4 + i * 36;
    std::memcpy(p, v[i].name.data(), v[i].name.size() < 31 ? v[i].name.size() : 31);
    wr32(p + 32, (uint32_t)(header + offs[i]));
  }
  out.insert(out.end(), buf.begin(), buf.end());
  return out;
}
}  // namespace

int GamesCache::addGames(const SdFs &fs, const std::vector<std::pair<std::string, std::string>> &games, bool dry, bool *identical) {
  if (identical) *identical = false;
  std::vector<uint8_t> d;
  if (!fs.readAll("/games_cache.bin", d)) return -1;
  std::vector<CSys> v;
  if (!parseCache(d, v)) {
    logf("GCACHE", "ajout impossible : games_cache.bin non reconnu");
    return -1;
  }
  const bool same = serializeCache(v) == d;
  if (identical) *identical = same;
  int added = 0;
  for (const auto &g : games) {
    if (g.first.empty() || g.second.empty()) continue;
    const std::string low = toLower(g.second);
    CSys *sys = nullptr;
    for (CSys &s : v)
      if (s.name == g.first) {
        sys = &s;
        break;
      }
    if (!sys) {
      v.emplace_back();
      v.back().name = g.first;
      sys = &v.back();
    }
    std::vector<CEntry> &slice = sys->sl[bigramIndex(low)];
    bool known = false;
    for (const CEntry &e : slice)
      if (e.name == low) {
        known = true;
        break;
      }
    if (known) continue;
    slice.push_back({'p', low});
    added++;
  }
  if (added == 0 || dry) return added;
  if (!same) {
    logf("GCACHE", "ajout refuse : la reecriture de games_cache.bin n'est pas identique a l'original (lancer le Mode 6 du toolkit)");
    return -1;
  }
  const std::vector<uint8_t> out = serializeCache(v);
  const std::string path = fs.resolve("/games_cache.bin"), tmp = path + ".tmp";
  {
    std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
    if (!f || !f.write((const char *)out.data(), (std::streamsize)out.size())) {
      std::remove(tmp.c_str());
      return -1;
    }
  }
  if (std::rename(tmp.c_str(), path.c_str()) != 0) {
    std::remove(tmp.c_str());
    return -1;
  }
  logf("GCACHE", "%d jeu(x) ajoute(s) a games_cache.bin (%zu octets)", added, out.size());
  return added;
}

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
