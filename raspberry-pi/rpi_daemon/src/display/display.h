// display.h -- couche d'affichage du demon Raspberry Pi.
//
// Framebuffer RGB565 128x32 en memoire, avec l'API d'Adafruit_GFX / ESP32-HUB75 utilisee par
// RecalBox_DMD.ino (memes noms : drawPixel, fillRect, setTextColor, print, drawRGBBitmap...), pour que
// le code porte du firmware change le moins possible. Les algorithmes (lignes, cercles, triangles,
// texte) sont ceux d'Adafruit_GFX 1.12.6 (BSD, voir LICENSE-Adafruit-GFX.txt) : memes pixels que l'ESP32.
//
// Le rendu se fait dans le framebuffer ; show() l'envoie au Backend (dalles hzeller, ou fichier PNG).
#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "gfxfont.h"

constexpr int DMD_W = 128;  // PANEL_RES_X (64) * PANEL_CHAIN (2)
constexpr int DMD_H = 32;   // PANEL_RES_Y

class Backend {
 public:
  virtual ~Backend() = default;
  virtual bool begin() = 0;
  // fb : DMD_W*DMD_H pixels RGB565, ligne par ligne. brightness8 : 0..255 (comme setBrightness8()).
  virtual void present(const uint16_t *fb, uint8_t brightness8) = 0;
};

class Display {
 public:
  explicit Display(Backend *b) : backend_(b) { fb_ = fbStatic_; }
  // canevas hors ecran de taille quelconque (texte defilant des citations : plus haut que 32 pixels, comme le GFXcanvas1 du firmware)
  Display(int w, int h) : backend_(nullptr), w_(w), h_(h), fbv_((size_t)w * (size_t)h, 0) { fb_ = fbv_.data(); }
  Display(const Display &) = delete;
  Display &operator=(const Display &) = delete;

  bool begin();
  void show();  // envoie le framebuffer au backend

  void setBrightness8(uint8_t b) { brightness8_ = b; }
  uint8_t getBrightness8() const { return brightness8_; }

  static uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
    return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
  }
  const uint16_t *framebuffer() const { return fb_; }
  int width() const { return w_; }
  int height() const { return h_; }

  // --- primitives ---
  void drawPixel(int16_t x, int16_t y, uint16_t c);
  void clearScreen() { fillScreen(0); }
  void fillScreen(uint16_t c);
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t c);
  void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t c);
  void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t c);
  void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t c);
  void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t c);
  void drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t c);
  void fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t c);
  void drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t c);
  void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t c);
  // Bitmap RGB565 (little-endian natif, comme les .raw565), clippe aux bords.
  void drawRGBBitmap(int16_t x, int16_t y, const uint16_t *bitmap, int16_t w, int16_t h);

  // --- texte (semantique d'Adafruit_GFX) ---
  void setCursor(int16_t x, int16_t y) { cursor_x_ = x; cursor_y_ = y; }
  int16_t getCursorX() const { return cursor_x_; }
  int16_t getCursorY() const { return cursor_y_; }
  void setTextColor(uint16_t c) { textcolor_ = textbgcolor_ = c; }
  void setTextColor(uint16_t c, uint16_t bg) { textcolor_ = c; textbgcolor_ = bg; }
  void setTextSize(uint8_t s) { setTextSize(s, s); }
  void setTextSize(uint8_t sx, uint8_t sy) { textsize_x_ = sx ? sx : 1; textsize_y_ = sy ? sy : 1; }
  void setTextWrap(bool w) { wrap_ = w; }
  void setFont(const GFXfont *f);
  void drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size) {
    drawChar(x, y, c, color, bg, size, size);
  }
  void drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t sx, uint8_t sy);
  size_t write(uint8_t c);
  size_t print(const char *s);
  size_t print(const std::string &s) { return print(s.c_str()); }
  size_t print(char c) { return write((uint8_t)c); }
  size_t print(int v) { return print(std::to_string(v)); }
  size_t print(unsigned v) { return print(std::to_string(v)); }
  size_t print(long v) { return print(std::to_string(v)); }
  size_t println(const char *s) { size_t n = print(s); return n + write('\n'); }
  size_t println(const std::string &s) { return println(s.c_str()); }
  void getTextBounds(const char *str, int16_t x, int16_t y, int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h);

 private:
  void charBounds(unsigned char c, int16_t *x, int16_t *y, int16_t *minx, int16_t *miny, int16_t *maxx,
                  int16_t *maxy);

  Backend *backend_;
  int w_ = DMD_W, h_ = DMD_H;
  uint16_t fbStatic_[DMD_W * DMD_H] = {0};
  std::vector<uint16_t> fbv_;  // framebuffer d'un canevas de taille libre
  uint16_t *fb_ = nullptr;     // fbStatic_ (ecran) ou fbv_ (canevas)
  uint8_t brightness8_ = 255;

  int16_t cursor_x_ = 0, cursor_y_ = 0;
  uint16_t textcolor_ = 0xFFFF, textbgcolor_ = 0xFFFF;
  uint8_t textsize_x_ = 1, textsize_y_ = 1;
  bool wrap_ = true;
  const GFXfont *gfx_font_ = nullptr;
};
