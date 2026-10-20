// color.h -- conversion RGB565 -> RGB888, identique a tools/raw565_convert.py (decode) :
// c8 = (c5 * 255) // 31  pour R et B, (c6 * 255) // 63 pour G. Un seul endroit pour tous les backends
// afin que la comparaison pixel a pixel avec le toolkit reste exacte.
#pragma once
#include <cstdint>

inline void unpack565(uint16_t v, uint8_t &r, uint8_t &g, uint8_t &b) {
  r = (uint8_t)((((v >> 11) & 0x1F) * 255) / 31);
  g = (uint8_t)((((v >> 5) & 0x3F) * 255) / 63);
  b = (uint8_t)(((v & 0x1F) * 255) / 31);
}
