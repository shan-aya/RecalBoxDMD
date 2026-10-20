// backend_png.cpp -- encodeur PNG minimal (deflate en blocs "stored", aucune dependance).
#include "backend_png.h"

#include <cstdio>
#include <vector>

#include "color.h"

namespace {
uint32_t crc_table[256];
bool crc_ready = false;

void crc_init() {
  for (uint32_t n = 0; n < 256; n++) {
    uint32_t c = n;
    for (int k = 0; k < 8; k++) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
    crc_table[n] = c;
  }
  crc_ready = true;
}

uint32_t crc32(const uint8_t *p, size_t n, uint32_t c = 0xFFFFFFFFu) {
  if (!crc_ready) crc_init();
  for (size_t i = 0; i < n; i++) c = crc_table[(c ^ p[i]) & 0xFF] ^ (c >> 8);
  return c;
}

void put32(std::vector<uint8_t> &v, uint32_t x) {
  v.push_back(x >> 24);
  v.push_back(x >> 16);
  v.push_back(x >> 8);
  v.push_back(x);
}

void chunk(std::vector<uint8_t> &out, const char *type, const std::vector<uint8_t> &data) {
  put32(out, (uint32_t)data.size());
  std::vector<uint8_t> td(type, type + 4);
  td.insert(td.end(), data.begin(), data.end());
  out.insert(out.end(), td.begin(), td.end());
  put32(out, crc32(td.data(), td.size()) ^ 0xFFFFFFFFu);
}
}  // namespace

void PngBackend::present(const uint16_t *fb, uint8_t brightness8) {
  const int W = DMD_W * zoom_, H = DMD_H * zoom_;
  std::vector<uint8_t> raw;  // lignes filtrees (octet de filtre 0)
  raw.reserve((size_t)H * (W * 3 + 1));
  for (int y = 0; y < H; y++) {
    raw.push_back(0);
    for (int x = 0; x < W; x++) {
      uint8_t r, g, b;
      unpack565(fb[(size_t)(y / zoom_) * DMD_W + (x / zoom_)], r, g, b);
      if (useBrightness_ && brightness8 != 255) {
        r = (uint8_t)(r * brightness8 / 255);
        g = (uint8_t)(g * brightness8 / 255);
        b = (uint8_t)(b * brightness8 / 255);
      }
      raw.push_back(r);
      raw.push_back(g);
      raw.push_back(b);
    }
  }
  // zlib : en-tete 78 01, blocs stored <= 65535 octets, adler32
  std::vector<uint8_t> z = {0x78, 0x01};
  size_t pos = 0;
  do {
    size_t n = raw.size() - pos;
    if (n > 65535) n = 65535;
    bool last = (pos + n == raw.size());
    z.push_back(last ? 1 : 0);
    z.push_back(n & 0xFF);
    z.push_back(n >> 8);
    z.push_back(~n & 0xFF);
    z.push_back((~n >> 8) & 0xFF);
    z.insert(z.end(), raw.begin() + pos, raw.begin() + pos + n);
    pos += n;
  } while (pos < raw.size());
  uint32_t a = 1, b2 = 0;
  for (uint8_t c : raw) {
    a = (a + c) % 65521;
    b2 = (b2 + a) % 65521;
  }
  put32(z, (b2 << 16) | a);

  std::vector<uint8_t> out = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
  std::vector<uint8_t> ihdr;
  put32(ihdr, W);
  put32(ihdr, H);
  ihdr.insert(ihdr.end(), {8, 2, 0, 0, 0});
  chunk(out, "IHDR", ihdr);
  chunk(out, "IDAT", z);
  chunk(out, "IEND", {});

  // ecriture atomique (fichier temporaire puis rename) : un lecteur ne voit jamais un PNG a moitie ecrit
  const std::string tmp = path_ + ".tmp";
  FILE *f = std::fopen(tmp.c_str(), "wb");
  ok_ = f && std::fwrite(out.data(), 1, out.size(), f) == out.size();
  if (f) std::fclose(f);
  if (ok_) ok_ = std::rename(tmp.c_str(), path_.c_str()) == 0;
}
