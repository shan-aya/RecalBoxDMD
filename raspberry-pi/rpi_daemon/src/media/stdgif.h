// stdgif.h -- lecture des GIF standard (playlists /gifs/...) via giflib, avec la semantique d'AnimatedGIF + GIFDraw du
// firmware : toutes les images sont decodees a l'ouverture (composition + disposal), puis affichees centrees dans le
// panneau (xOffset = (128-L)/2 borne a 0, yOffset = (32-H)/2 ; trop grand = recadre), transparent = noir.
//   play() renvoie true s'il reste des images apres celle-ci, false apres la derniere (la derniere image n'attend
//   pas son delai, comme dans le firmware), delai : 0 cs -> 100 ms, pas de GCE -> 0 (l'appelant met 10 ms).
#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "display.h"
#include "sdfs.h"

class StdGif {
 public:
  bool open(const SdFs &fs, const std::string &sdPath);
  void close();
  bool isOpen() const { return !frames_.empty(); }
  void reset() { idx_ = 0; }
  bool play(bool first, int *delayMs, Display &d);
  uint32_t frameCount() const { return (uint32_t)frames_.size(); }

 private:
  int w_ = 0, h_ = 0;
  uint32_t idx_ = 0;
  std::vector<std::vector<uint16_t>> frames_;  // canevas complet RGB565 par image
  std::vector<int> delays_;                    // ms (valeur AnimatedGIF)
};
