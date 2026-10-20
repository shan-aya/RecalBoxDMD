// gamescache.h -- /games_cache.bin : index bigramme des jeux ayant une image (meme format que le firmware).
//   uint32 nbSystemes ; nbSystemes x { char nom[32] ; uint32 offset } ;
//   a chaque offset : uint32 table[703] (index bigramme) pointant vers des tranches de { char type ; nom\0 }.
// findInGamesCache() : correspondance exacte (insensible a la casse) sinon plus long prefixe strict ; '?' = absent.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "sdfs.h"

constexpr int NB_IDX = 703;  // 1 + 26*27

int bigramIndex(const std::string &name);
std::string bigramLabel(int bi);

class GamesCache {
 public:
  // Recharge automatiquement si games_cache.bin a change sur disque (mise a jour par le toolkit).
  char find(const SdFs &fs, const std::string &sys, const std::string &game);
  size_t systems() const { return idx_.size(); }

 private:
  bool ensureLoaded(const SdFs &fs);
  struct SysIdx {
    std::string name;
    uint32_t offset;
  };
  std::vector<uint8_t> data_;
  std::vector<SysIdx> idx_;
  int64_t mtime_ = -1;
  int64_t size_ = -1;
};
