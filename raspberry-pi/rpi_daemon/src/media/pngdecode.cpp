#include "pngdecode.h"

#include <png.h>

#include <cstring>

namespace {
struct MemSrc {
  const uint8_t *p;
  size_t n, pos;
};
void readFn(png_structp png, png_bytep out, png_size_t len) {
  MemSrc *s = static_cast<MemSrc *>(png_get_io_ptr(png));
  if (s->pos + len > s->n) png_error(png, "PNG tronque");
  std::memcpy(out, s->p + s->pos, len);
  s->pos += len;
}
}  // namespace

bool decodePngRgba(const std::vector<uint8_t> &file, std::vector<uint8_t> &rgba, int &w, int &h) {
  if (file.size() < 8 || png_sig_cmp(file.data(), 0, 8) != 0) return false;
  png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
  if (!png) return false;
  png_infop info = png_create_info_struct(png);
  if (!info) {
    png_destroy_read_struct(&png, nullptr, nullptr);
    return false;
  }
  std::vector<png_bytep> rows;
  if (setjmp(png_jmpbuf(png))) {
    png_destroy_read_struct(&png, &info, nullptr);
    return false;
  }
  MemSrc src{file.data(), file.size(), 0};
  png_set_read_fn(png, &src, readFn);
  png_read_info(png, info);
  w = (int)png_get_image_width(png, info);
  h = (int)png_get_image_height(png, info);
  if (w <= 0 || h <= 0 || (size_t)w * (size_t)h > 64u * 1024u * 1024u) {  // garde-fou memoire
    png_destroy_read_struct(&png, &info, nullptr);
    return false;
  }
  png_set_expand(png);        // palette / gris / tRNS -> RGB(A)
  png_set_strip_16(png);      // 16 bits -> 8 bits
  png_set_gray_to_rgb(png);
  if (!(png_get_color_type(png, info) & PNG_COLOR_MASK_ALPHA)) png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
  png_set_interlace_handling(png);
  png_read_update_info(png, info);
  rgba.assign((size_t)w * h * 4, 0);
  rows.resize((size_t)h);
  for (int y = 0; y < h; y++) rows[(size_t)y] = rgba.data() + (size_t)y * w * 4;
  png_read_image(png, rows.data());
  png_destroy_read_struct(&png, &info, nullptr);
  return true;
}
