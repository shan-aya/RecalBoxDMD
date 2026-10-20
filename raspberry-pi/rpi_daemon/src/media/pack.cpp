#include "pack.h"

#include "log.h"

void Raw565Pack::close() {
  if (file_) std::fclose(file_);
  file_ = nullptr;
  frames_ = 0;
  delays_.clear();
}

bool Raw565Pack::open(const SdFs &fs, SubdirPref &pref, const std::string &gifKey) {
  close();
  const std::string rawPack = gifToRaw565PackPath(gifKey), meta = gifToRaw565MetaPath(gifKey);
  std::string packPath = pref.pick(fs, alphaSubdirPath(rawPack), rawPack);
  std::string metaPath = pref.pick(fs, alphaSubdirPath(meta), meta);
  if (packPath.empty() || metaPath.empty()) return false;  // les deux fichiers sont requis

  std::vector<uint8_t> m;
  if (!fs.readAll(metaPath, m) || m.size() < 2) return false;
  delays_.resize(m.size() / 2);
  for (size_t i = 0; i < delays_.size(); i++) delays_[i] = (uint16_t)(m[2 * i] | (m[2 * i + 1] << 8));

  std::string real = fs.resolve(packPath);
  file_ = real.empty() ? nullptr : std::fopen(real.c_str(), "rb");
  if (!file_) return false;
  std::fseek(file_, 0, SEEK_END);
  long sz = std::ftell(file_);
  frames_ = sz > 0 ? (uint32_t)((size_t)sz / RAW565_FRAME_BYTES) : 0;
  if (frames_ == 0) {
    close();
    return false;
  }
  logf("GIF", "open OK raw565pack req=%s (%u images, %zu delais)", gifKey.c_str(), frames_, delays_.size());
  return true;
}

bool Raw565Pack::readFrame(uint32_t index, uint16_t *dst) const {
  if (!file_ || index >= frames_) return false;
  if (std::fseek(file_, (long)((uint64_t)index * RAW565_FRAME_BYTES), SEEK_SET) != 0) return false;
  return std::fread(dst, 1, RAW565_FRAME_BYTES, file_) == RAW565_FRAME_BYTES;  // .raw565 = uint16 LE = natif ARM/x86
}

uint16_t Raw565Pack::delayMs(uint32_t index) const {
  uint16_t ms = (index < delays_.size()) ? delays_[index] : 33;
  if (ms == 0) ms = PACK_MIN_DELAY_MS;
  ms = (uint16_t)((uint32_t)ms * PACK_SPEED_PERCENT / 100U);
  if (ms < PACK_MIN_DELAY_MS) ms = PACK_MIN_DELAY_MS;
  return ms;
}
