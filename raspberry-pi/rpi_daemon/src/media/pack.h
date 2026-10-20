// pack.h -- lecteur de .raw565pack (N images de 8192 octets) + .meta (N x uint16 LE : delai en ms par image).
// Vitesse (comme le firmware) : delai 0 -> 5 ms, puis x50 % (lecture 2x), plancher 5 ms.
#pragma once
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "display.h"
#include "paths.h"
#include "sdfs.h"

constexpr size_t RAW565_FRAME_BYTES = (size_t)DMD_W * DMD_H * 2;  // 8192
constexpr uint16_t PACK_SPEED_PERCENT = 50;
constexpr uint16_t PACK_MIN_DELAY_MS = 5;

class Raw565Pack {
 public:
  ~Raw565Pack() { close(); }
  // gifKey : "chemin.gif" ; le pack est "chemin.raw565pack", le meta "chemin.meta" (sous-dossier alpha, puis plat).
  bool open(const SdFs &fs, SubdirPref &pref, const std::string &gifKey);
  void close();
  bool isOpen() const { return file_ != nullptr; }
  uint32_t frameCount() const { return frames_; }
  bool readFrame(uint32_t index, uint16_t *dst) const;  // DMD_W*DMD_H pixels RGB565
  uint16_t delayMs(uint32_t index) const;

 private:
  FILE *file_ = nullptr;
  uint32_t frames_ = 0;
  std::vector<uint16_t> delays_;
};
