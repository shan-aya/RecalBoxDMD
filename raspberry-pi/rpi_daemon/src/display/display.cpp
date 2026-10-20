// display.cpp -- voir display.h. Algorithmes d'Adafruit_GFX 1.12.6 (BSD, Adafruit Industries).
#include "display.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>

#include "TomThumb.h"  // inclus ici pour verifier la compilation du header ; la police est exposee ailleurs
#include "glcdfont.h"

namespace {
template <typename T>
inline void swap16(T &a, T &b) {
  T t = a;
  a = b;
  b = t;
}
}  // namespace

bool Display::begin() {
  if (!backend_ || !backend_->begin()) return false;
  clearScreen();
  show();
  return true;
}

void Display::show() {
  if (backend_) backend_->present(fb_, brightness8_);
}

void Display::drawPixel(int16_t x, int16_t y, uint16_t c) {
  if (x < 0 || y < 0 || x >= w_ || y >= h_) return;
  fb_[(size_t)y * w_ + x] = c;
}

void Display::fillScreen(uint16_t c) { std::fill(fb_, fb_ + w_ * h_, c); }

void Display::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t c) {
  if (w <= 0 || h <= 0) return;
  int x0 = std::max<int>(x, 0), y0 = std::max<int>(y, 0);
  int x1 = std::min<int>(x + w, w_), y1 = std::min<int>(y + h, h_);
  if (x0 >= x1 || y0 >= y1) return;  // rectangle entierement hors de l'ecran (ex. sprite qui sort a droite) : rien a dessiner
  for (int yy = y0; yy < y1; yy++) std::fill(fb_ + (size_t)yy * w_ + x0, fb_ + (size_t)yy * w_ + x1, c);
}

void Display::drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t c) { fillRect(x, y, w, 1, c); }
void Display::drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t c) { fillRect(x, y, 1, h, c); }

void Display::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t c) {
  drawFastHLine(x, y, w, c);
  drawFastHLine(x, y + h - 1, w, c);
  drawFastVLine(x, y, h, c);
  drawFastVLine(x + w - 1, y, h, c);
}

void Display::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t c) {
  // Bresenham d'Adafruit_GFX::writeLine
  int16_t steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
    swap16(x0, y0);
    swap16(x1, y1);
  }
  if (x0 > x1) {
    swap16(x0, x1);
    swap16(y0, y1);
  }
  int16_t dx = x1 - x0, dy = abs(y1 - y0);
  int16_t err = dx / 2;
  int16_t ystep = (y0 < y1) ? 1 : -1;
  for (; x0 <= x1; x0++) {
    if (steep)
      drawPixel(y0, x0, c);
    else
      drawPixel(x0, y0, c);
    err -= dy;
    if (err < 0) {
      y0 += ystep;
      err += dx;
    }
  }
}

void Display::drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t c) {
  int16_t f = 1 - r, ddF_x = 1, ddF_y = -2 * r, x = 0, y = r;
  drawPixel(x0, y0 + r, c);
  drawPixel(x0, y0 - r, c);
  drawPixel(x0 + r, y0, c);
  drawPixel(x0 - r, y0, c);
  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
    drawPixel(x0 + x, y0 + y, c);
    drawPixel(x0 - x, y0 + y, c);
    drawPixel(x0 + x, y0 - y, c);
    drawPixel(x0 - x, y0 - y, c);
    drawPixel(x0 + y, y0 + x, c);
    drawPixel(x0 - y, y0 + x, c);
    drawPixel(x0 + y, y0 - x, c);
    drawPixel(x0 - y, y0 - x, c);
  }
}

void Display::fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t c) {
  drawFastVLine(x0, y0 - r, 2 * r + 1, c);
  // fillCircleHelper(x0, y0, r, 3, 0, c)
  int16_t delta = 1;  // delta++ d'Adafruit
  int16_t f = 1 - r, ddF_x = 1, ddF_y = -2 * r, x = 0, y = r, px = x, py = y;
  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
    if (x < (y + 1)) {
      drawFastVLine(x0 + x, y0 - y, 2 * y + delta, c);
      drawFastVLine(x0 - x, y0 - y, 2 * y + delta, c);
    }
    if (y != py) {
      drawFastVLine(x0 + py, y0 - px, 2 * px + delta, c);
      drawFastVLine(x0 - py, y0 - px, 2 * px + delta, c);
      py = y;
    }
    px = x;
  }
}

void Display::drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t c) {
  drawLine(x0, y0, x1, y1, c);
  drawLine(x1, y1, x2, y2, c);
  drawLine(x2, y2, x0, y0, c);
}

void Display::fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t c) {
  int16_t a, b, y, last;
  if (y0 > y1) {
    swap16(y0, y1);
    swap16(x0, x1);
  }
  if (y1 > y2) {
    swap16(y2, y1);
    swap16(x2, x1);
  }
  if (y0 > y1) {
    swap16(y0, y1);
    swap16(x0, x1);
  }
  if (y0 == y2) {
    a = b = x0;
    if (x1 < a)
      a = x1;
    else if (x1 > b)
      b = x1;
    if (x2 < a)
      a = x2;
    else if (x2 > b)
      b = x2;
    drawFastHLine(a, y0, b - a + 1, c);
    return;
  }
  int16_t dx01 = x1 - x0, dy01 = y1 - y0, dx02 = x2 - x0, dy02 = y2 - y0, dx12 = x2 - x1, dy12 = y2 - y1;
  int32_t sa = 0, sb = 0;
  last = (y1 == y2) ? y1 : y1 - 1;
  for (y = y0; y <= last; y++) {
    a = x0 + sa / dy01;
    b = x0 + sb / dy02;
    sa += dx01;
    sb += dx02;
    if (a > b) swap16(a, b);
    drawFastHLine(a, y, b - a + 1, c);
  }
  sa = (int32_t)dx12 * (y - y1);
  sb = (int32_t)dx02 * (y - y0);
  for (; y <= y2; y++) {
    a = x1 + sa / dy12;
    b = x0 + sb / dy02;
    sa += dx12;
    sb += dx02;
    if (a > b) swap16(a, b);
    drawFastHLine(a, y, b - a + 1, c);
  }
}

void Display::drawRGBBitmap(int16_t x, int16_t y, const uint16_t *bitmap, int16_t w, int16_t h) {
  if (!bitmap || w <= 0 || h <= 0) return;
  for (int16_t j = 0; j < h; j++) {
    int yy = y + j;
    if (yy < 0 || yy >= h_) continue;
    int i0 = std::max<int>(0, -x), i1 = std::min<int>(w, w_ - x);
    if (i1 <= i0) continue;
    std::memcpy(fb_ + (size_t)yy * w_ + (x + i0), bitmap + (size_t)j * w + i0, (size_t)(i1 - i0) * 2);
  }
}

// ---------------------------------------------------------------- texte

void Display::setFont(const GFXfont *f) {
  if (f) {
    if (!gfx_font_) cursor_y_ += 6;  // classique -> GFX : le curseur passe sur la ligne de base
  } else if (gfx_font_) {
    cursor_y_ -= 6;
  }
  gfx_font_ = f;
}

void Display::drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size_x,
                       uint8_t size_y) {
  if (!gfx_font_) {  // police classique 5x7
    if (x >= w_ || y >= h_ || (x + 6 * size_x - 1) < 0 || (y + 8 * size_y - 1) < 0) return;
    if (c >= 176) c++;  // _cp437 faux par defaut dans Adafruit_GFX
    for (int8_t i = 0; i < 5; i++) {
      uint8_t line = glcdfont[c * 5 + i];
      for (int8_t j = 0; j < 8; j++, line >>= 1) {
        if (line & 1) {
          if (size_x == 1 && size_y == 1)
            drawPixel(x + i, y + j, color);
          else
            fillRect(x + i * size_x, y + j * size_y, size_x, size_y, color);
        } else if (bg != color) {
          if (size_x == 1 && size_y == 1)
            drawPixel(x + i, y + j, bg);
          else
            fillRect(x + i * size_x, y + j * size_y, size_x, size_y, bg);
        }
      }
    }
    if (bg != color) {
      if (size_x == 1 && size_y == 1)
        drawFastVLine(x + 5, y, 8, bg);
      else
        fillRect(x + 5 * size_x, y, size_x, 8 * size_y, bg);
    }
  } else {  // police GFX (ex. TomThumb) : pas de fond, par conception
    c -= (uint8_t)gfx_font_->first;
    const GFXglyph *glyph = &gfx_font_->glyph[c];
    const uint8_t *bitmap = gfx_font_->bitmap;
    uint16_t bo = glyph->bitmapOffset;
    uint8_t w = glyph->width, h = glyph->height;
    int8_t xo = glyph->xOffset, yo = glyph->yOffset;
    uint8_t bits = 0, bit = 0;
    int16_t xo16 = 0, yo16 = 0;
    if (size_x > 1 || size_y > 1) {
      xo16 = xo;
      yo16 = yo;
    }
    for (uint8_t yy = 0; yy < h; yy++) {
      for (uint8_t xx = 0; xx < w; xx++) {
        if (!(bit++ & 7)) bits = bitmap[bo++];
        if (bits & 0x80) {
          if (size_x == 1 && size_y == 1)
            drawPixel(x + xo + xx, y + yo + yy, color);
          else
            fillRect(x + (xo16 + xx) * size_x, y + (yo16 + yy) * size_y, size_x, size_y, color);
        }
        bits <<= 1;
      }
    }
  }
}

size_t Display::write(uint8_t c) {
  if (!gfx_font_) {
    if (c == '\n') {
      cursor_x_ = 0;
      cursor_y_ += textsize_y_ * 8;
    } else if (c != '\r') {
      if (wrap_ && ((cursor_x_ + textsize_x_ * 6) > w_)) {
        cursor_x_ = 0;
        cursor_y_ += textsize_y_ * 8;
      }
      drawChar(cursor_x_, cursor_y_, c, textcolor_, textbgcolor_, textsize_x_, textsize_y_);
      cursor_x_ += textsize_x_ * 6;
    }
  } else {
    if (c == '\n') {
      cursor_x_ = 0;
      cursor_y_ += (int16_t)textsize_y_ * gfx_font_->yAdvance;
    } else if (c != '\r') {
      uint8_t first = (uint8_t)gfx_font_->first;
      if (c >= first && c <= (uint8_t)gfx_font_->last) {
        const GFXglyph *glyph = &gfx_font_->glyph[c - first];
        uint8_t w = glyph->width, h = glyph->height;
        if (w > 0 && h > 0) {
          int16_t xo = glyph->xOffset;
          if (wrap_ && ((cursor_x_ + textsize_x_ * (xo + w)) > w_)) {
            cursor_x_ = 0;
            cursor_y_ += (int16_t)textsize_y_ * gfx_font_->yAdvance;
          }
          drawChar(cursor_x_, cursor_y_, c, textcolor_, textbgcolor_, textsize_x_, textsize_y_);
        }
        cursor_x_ += glyph->xAdvance * (int16_t)textsize_x_;
      }
    }
  }
  return 1;
}

size_t Display::print(const char *s) {
  size_t n = 0;
  while (s && *s) n += write((uint8_t)*s++);
  return n;
}

void Display::charBounds(unsigned char c, int16_t *x, int16_t *y, int16_t *minx, int16_t *miny, int16_t *maxx,
                         int16_t *maxy) {
  if (gfx_font_) {
    if (c == '\n') {
      *x = 0;
      *y += textsize_y_ * gfx_font_->yAdvance;
    } else if (c != '\r') {
      uint8_t first = (uint8_t)gfx_font_->first, last = (uint8_t)gfx_font_->last;
      if (c >= first && c <= last) {
        const GFXglyph *glyph = &gfx_font_->glyph[c - first];
        uint8_t gw = glyph->width, gh = glyph->height, xa = glyph->xAdvance;
        int8_t xo = glyph->xOffset, yo = glyph->yOffset;
        if (wrap_ && ((*x + (((int16_t)xo + gw) * textsize_x_)) > w_)) {
          *x = 0;
          *y += textsize_y_ * gfx_font_->yAdvance;
        }
        int16_t tsx = textsize_x_, tsy = textsize_y_, x1 = *x + xo * tsx, y1 = *y + yo * tsy,
                x2 = x1 + gw * tsx - 1, y2 = y1 + gh * tsy - 1;
        if (x1 < *minx) *minx = x1;
        if (y1 < *miny) *miny = y1;
        if (x2 > *maxx) *maxx = x2;
        if (y2 > *maxy) *maxy = y2;
        *x += xa * tsx;
      }
    }
  } else {
    if (c == '\n') {
      *x = 0;
      *y += textsize_y_ * 8;
    } else if (c != '\r') {
      if (wrap_ && ((*x + textsize_x_ * 6) > w_)) {
        *x = 0;
        *y += textsize_y_ * 8;
      }
      int x2 = *x + textsize_x_ * 6 - 1, y2 = *y + textsize_y_ * 8 - 1;
      if (x2 > *maxx) *maxx = x2;
      if (y2 > *maxy) *maxy = y2;
      if (*x < *minx) *minx = *x;
      if (*y < *miny) *miny = *y;
      *x += textsize_x_ * 6;
    }
  }
}

void Display::getTextBounds(const char *str, int16_t x, int16_t y, int16_t *x1, int16_t *y1, uint16_t *w,
                            uint16_t *h) {
  int16_t minx = 0x7FFF, miny = 0x7FFF, maxx = -1, maxy = -1;
  *x1 = x;
  *y1 = y;
  *w = *h = 0;
  uint8_t c;
  while ((c = (uint8_t)*str++)) charBounds(c, &x, &y, &minx, &miny, &maxx, &maxy);
  if (maxx >= minx) {
    *x1 = minx;
    *w = maxx - minx + 1;
  }
  if (maxy >= miny) {
    *y1 = miny;
    *h = maxy - miny + 1;
  }
}
