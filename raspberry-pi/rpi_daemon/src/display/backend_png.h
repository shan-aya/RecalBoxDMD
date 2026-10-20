// backend_png.h -- sortie fichier PNG (tests sans materiel, captures de rendu).
#pragma once
#include <string>

#include "display.h"

class PngBackend : public Backend {
 public:
  // zoom : agrandissement entier (1 = pixels exacts, pour la comparaison avec raw565_convert.py decode).
  // useBrightness : applique setBrightness8() aux pixels (sinon le PNG montre le contenu du framebuffer tel quel).
  explicit PngBackend(std::string path, int zoom = 1, bool useBrightness = false)
      : path_(std::move(path)), zoom_(zoom < 1 ? 1 : zoom), useBrightness_(useBrightness) {}
  bool begin() override { return true; }
  void present(const uint16_t *fb, uint8_t brightness8) override;
  bool ok() const { return ok_; }

 private:
  std::string path_;
  int zoom_;
  bool useBrightness_;
  bool ok_ = true;
};
