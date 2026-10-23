// bootanim.cpp -- voir bootanim.h
#include "bootanim.h"

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <thread>
#include <vector>

#include "boot_seasons.h"
#include "log.h"

namespace {
constexpr size_t FRAME_BYTES = (size_t)DMD_W * DMD_H * 2;  // 8192 : une image RGB565 128x32
constexpr uint32_t SEASON_FILE_BYTES = 819200;               // 100 images : taille exacte attendue d'un fichier saisonnier
constexpr uint32_t T_TITLE_ON = 2000, T_TITLE_OFF = 4500, T_END = 6500, FRAME_MS = 40;

uint32_t todayYmd(const Config &cfg) {
  const uint32_t fake = (uint32_t)std::strtoul(cfg.get("fake_date", "0").c_str(), nullptr, 10);
  if (fake) return fake;
  const time_t now = time(nullptr);
  if (now >= 1700000000L) {  // horloge systeme synchronisee (NTP) ou lue sur la RTC de la carte d'interface
    struct tm ti;
    localtime_r(&now, &ti);
    return (uint32_t)(ti.tm_year + 1900) * 10000u + (uint32_t)(ti.tm_mon + 1) * 100u + (uint32_t)ti.tm_mday;
  }
  return (uint32_t)std::strtoul(cfg.get("last_date", "0").c_str(), nullptr, 10);
}

// titre par-dessus l'image : meme dessin que bootAnimDrawTitle() (liseré noir de 8 voisins, puis « Recal » rouge, « Box » bleu, « DMD » vert)
void drawTitle(Display &cv, const std::string &edition) {
  cv.setTextWrap(false);
  cv.setTextSize(1);
  const int x1 = 31, y1 = 8, x2 = (DMD_W - (int)edition.size() * 6) / 2, y2 = 21;
  for (int dy = -1; dy <= 1; dy++)
    for (int dx = -1; dx <= 1; dx++) {
      if (!dx && !dy) continue;
      cv.setTextColor(0x0000);
      cv.setCursor(x1 + dx, y1 + dy);
      cv.print("RecalBoxDMD");
      cv.setCursor(x2 + dx, y2 + dy);
      cv.print(edition);
    }
  cv.setCursor(x1, y1);
  cv.setTextColor(Display::color565(255, 40, 40));
  cv.print("Recal");
  cv.setTextColor(Display::color565(60, 130, 255));
  cv.print("Box");
  cv.setTextColor(Display::color565(50, 220, 80));
  cv.print("DMD");
  cv.setCursor(x2, y2);
  cv.setTextColor(Display::color565(235, 235, 235));
  cv.print(edition);
}
}  // namespace

bool playBootAnimation(Display &d, const SdFs &fs, const Config &cfg, const std::string &edition, const std::function<bool()> &stop) {
  std::string path;
  const uint32_t today = todayYmd(cfg);
  const int si = today ? bsSeasonAt(today) : -1;
  logf("BOOTANIM", "date=%u fete=%s", (unsigned)today, si >= 0 ? kBootSeasons[si].name : "aucune");
  if (si >= 0) {
    const std::string sp = std::string("/gifs/_boot/boot_") + kBootSeasons[si].name + ".raw565pack";
    const int64_t sz = fs.fileSize(sp);
    if (sz == (int64_t)SEASON_FILE_BYTES) path = sp;
    else logf("BOOTANIM", "%s %s -> animation par defaut", sp.c_str(), sz < 0 ? "absent" : "de taille inattendue");
  }
  if (path.empty() && fs.exists("/gifs/_boot/boot_anim.raw565pack")) path = "/gifs/_boot/boot_anim.raw565pack";
  if (path.empty() && fs.exists("/boot_anim.raw565pack")) path = "/boot_anim.raw565pack";
  if (path.empty()) {
    logf("BOOTANIM", "aucune animation (/gifs/_boot/boot_anim.raw565pack absent) -> demarrage sans animation");
    return false;
  }
  std::vector<uint8_t> data;
  if (!fs.readAll(path, data) || data.size() < 2 * FRAME_BYTES) {
    logf("BOOTANIM", "%s illisible ou trop court -> demarrage sans animation", path.c_str());
    return false;
  }
  const uint32_t frames = (uint32_t)(data.size() / FRAME_BYTES);
  Display cv(DMD_W, DMD_H);  // image composee hors ecran puis envoyee d'un bloc (pas de scintillement du titre)
  const auto t0 = std::chrono::steady_clock::now();
  auto next = t0;
  uint32_t idx = 0, shown = 0;
  while (true) {
    const uint32_t el = (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();
    if (el >= T_END || stop()) break;
    if (std::chrono::steady_clock::now() < next) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      continue;
    }
    next += std::chrono::milliseconds(FRAME_MS);
    cv.drawRGBBitmap(0, 0, reinterpret_cast<const uint16_t *>(data.data() + (size_t)idx * FRAME_BYTES), DMD_W, DMD_H);
    if (el >= T_TITLE_ON && el < T_TITLE_OFF) drawTitle(cv, edition);
    d.drawRGBBitmap(0, 0, cv.framebuffer(), DMD_W, DMD_H);
    d.show();
    idx = (idx + 1) % frames;
    shown++;
  }
  logf("BOOTANIM", "%s : %u images affichees (fichier = %u images) en %u ms", path.c_str(), (unsigned)shown, (unsigned)frames,
       (unsigned)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count());
  return shown > 0;
}