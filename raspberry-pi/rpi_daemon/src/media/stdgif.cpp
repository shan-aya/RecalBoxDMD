#include "stdgif.h"

#include <gif_lib.h>

#include <algorithm>

#include "log.h"

void StdGif::close() {
  frames_.clear();
  delays_.clear();
  w_ = h_ = 0;
  idx_ = 0;
}

bool StdGif::open(const SdFs &fs, const std::string &sdPath) {
  close();
  std::string real = fs.resolve(sdPath);
  if (real.empty()) return false;
  int err = 0;
  GifFileType *g = DGifOpenFileName(real.c_str(), &err);
  if (!g) {
    logf("GIF", "open FAIL standard path=%s err=%d", sdPath.c_str(), err);
    return false;
  }
  if (DGifSlurp(g) != GIF_OK || g->ImageCount < 1 || g->SWidth <= 0 || g->SHeight <= 0) {
    logf("GIF", "open FAIL standard (decodage) path=%s", sdPath.c_str());
    DGifCloseFile(g, &err);
    return false;
  }
  w_ = g->SWidth;
  h_ = g->SHeight;
  std::vector<uint16_t> canvas((size_t)w_ * h_, 0), saved;
  int prevDisposal = 0, pl = 0, pt = 0, pw = 0, ph = 0;
  for (int i = 0; i < g->ImageCount; i++) {
    const SavedImage &si = g->SavedImages[i];
    GraphicsControlBlock gcb;
    bool hasGce = DGifSavedExtensionToGCB(g, i, &gcb) == GIF_OK;
    int disposal = hasGce ? gcb.DisposalMode : 0;
    int transparent = hasGce ? gcb.TransparentColor : -1;
    int delay = 0;
    if (hasGce) {
      delay = gcb.DelayTime * 10;
      if (delay <= 1) delay = 100;  // 0-1 -> 100 ms, comme AnimatedGIF
    }

    // elimination de l'image precedente
    if (prevDisposal == 2) {
      for (int y = pt; y < pt + ph && y < h_; y++)
        for (int x = pl; x < pl + pw && x < w_; x++)
          if (x >= 0 && y >= 0) canvas[(size_t)y * w_ + x] = 0;
    } else if (prevDisposal == 3 && !saved.empty()) {
      canvas = saved;
    }
    if (disposal == 3) saved = canvas;

    const ColorMapObject *cm = si.ImageDesc.ColorMap ? si.ImageDesc.ColorMap : g->SColorMap;
    const int L = si.ImageDesc.Left, T = si.ImageDesc.Top, W = si.ImageDesc.Width, H = si.ImageDesc.Height;
    if (si.RasterBits && cm) {
      for (int y = 0; y < H; y++) {
        int cy = T + y;
        if (cy < 0 || cy >= h_) continue;
        for (int x = 0; x < W; x++) {
          int cx = L + x;
          if (cx < 0 || cx >= w_) continue;
          int idx = si.RasterBits[(size_t)y * W + x];
          if (idx == transparent) continue;  // transparent : on garde le contenu dessous
          if (idx >= cm->ColorCount) continue;
          const GifColorType &c = cm->Colors[idx];
          canvas[(size_t)cy * w_ + cx] = Display::color565(c.Red, c.Green, c.Blue);
        }
      }
    }
    frames_.push_back(canvas);
    delays_.push_back(delay);
    prevDisposal = disposal;
    pl = L;
    pt = T;
    pw = W;
    ph = H;
  }
  DGifCloseFile(g, &err);
  logf("GIF", "open OK standard path=%s (%dx%d, %zu images)", sdPath.c_str(), w_, h_, frames_.size());
  return true;
}

bool StdGif::play(bool first, int *delayMs, Display &d) {
  if (frames_.empty()) return false;
  if (first || idx_ >= frames_.size()) idx_ = 0;  // la bibliotheque rembobine apres la fin
  const std::vector<uint16_t> &cv = frames_[idx_];
  // GIFDraw : y = ligne + (32 - H)/2 (division C, vers zero), xOffset = (128 - L)/2 borne a 0, largeur bornee a 128
  const int yOffset = (DMD_H - h_) / 2;
  int xOffset = (DMD_W - w_) / 2;
  if (xOffset < 0) xOffset = 0;
  const int iWidth = std::min(w_, DMD_W);
  for (int ly = 0; ly < h_; ly++) {
    int y = ly + yOffset;
    if (y < 0 || y >= DMD_H) continue;
    d.drawRGBBitmap((int16_t)xOffset, (int16_t)y, cv.data() + (size_t)ly * w_, (int16_t)iWidth, 1);
  }
  if (delayMs) *delayMs = delays_[idx_];
  idx_++;
  return idx_ < frames_.size();
}
