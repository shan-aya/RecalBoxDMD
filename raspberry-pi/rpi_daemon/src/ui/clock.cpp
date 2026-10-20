// clock.cpp -- horloge a themes. Reutilise TELS QUELS le code du firmware :
//   * clock_themes.h (10 themes retro, genere par generate_clock_themes.py) via la couche compat/ ;
//   * clock_ambient.inc (5 themes animes, extrait du .ino par tools/port_clock_ambient.py).
// Seuls le sequencement (non bloquant) et la lecture de l'heure sont reecrits ici.
#include "clock.h"

#include <cstdlib>
#include <ctime>

#include "log.h"

// Le code du firmware n'est pas ecrit pour nos options de compilation : on coupe ses avertissements, pas les notres.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#pragma GCC diagnostic ignored "-Wparentheses"
#pragma GCC diagnostic ignored "-Wtype-limits"

#include "Arduino.h"
#include "ESP32-HUB75-MatrixPanel-I2S-DMA.h"

// globaux attendus par clock_themes.h / clock_ambient.inc (definis dans le .ino sur l'ESP32)
Display *display = nullptr;
bool clockNeonCustomColor = false;                            // CLOCK_COLOR defini ? (theme Neon)
uint8_t clockNeonR = 255, clockNeonG = 40, clockNeonB = 120;  // defauts = rose du theme Neon
int32_t clockAnimCol[5] = {-1, -1, -1, -1, -1};               // couleur d'animation des themes animes (0xRRGGBB, -1 = auto)
int32_t clockDigitCol[5] = {-1, -1, -1, -1, -1};              // couleur des chiffres
uint16_t g_clkColorVer = 0;                                   // incremente a chaque changement de couleur en direct

#include "clock_themes.h"       // dossier parent (firmware) : RETRO_THEME_COUNT, drawRetroClockTheme(), retroThemeNames
#include "clock_ambient.inc"    // decoClockAmbient() et ses dependances

#pragma GCC diagnostic pop

namespace {
constexpr int CLOCK_THEME_TOTAL = RETRO_THEME_COUNT + 5;  // 10 retro + Plasma, Life, Stars, Rain, Lava

const char *clockThemeName(int i) {
  static const char *extra[5] = {"Plasma", "Life", "Stars", "Rain", "Lava"};
  return (i < RETRO_THEME_COUNT) ? retroThemeNames[i] : extra[i - RETRO_THEME_COUNT];
}

void clockDrawTheme(int theme, int h, int m, int s, unsigned long ms) {
  if (theme >= RETRO_THEME_COUNT) decoClockAmbient(theme - RETRO_THEME_COUNT, h, m, ms);
  else drawRetroClockTheme(theme, h, m, s, ms);
}

// heure locale ; false tant que le systeme n'est pas synchronise (annee < 2000)
bool getClockTime(int &h, int &m, int &s) {
  time_t now;
  struct tm ti;
  time(&now);
  localtime_r(&now, &ti);
  if (ti.tm_year < 100) return false;
  h = ti.tm_hour;
  m = ti.tm_min;
  s = ti.tm_sec;
  return true;
}

int32_t hexColor(const std::string &s) {  // « #RRGGBB » -> 0xRRGGBB, sinon -1 (auto)
  if (s.size() != 7 || s[0] != '#') return -1;
  return (int32_t)(std::strtoul(s.c_str() + 1, nullptr, 16) & 0xFFFFFF);
}
}  // namespace

void Clock::setLiveColors(int slot, const std::string *a, const std::string *d, const std::string *neon) {
  if (slot >= 0 && slot < 5) {
    if (a) clockAnimCol[slot] = hexColor(*a);
    if (d) clockDigitCol[slot] = hexColor(*d);
  }
  if (neon) {
    const int32_t c = hexColor(*neon);
    if (c >= 0) {
      clockNeonCustomColor = true;
      clockNeonR = (c >> 16) & 0xFF;
      clockNeonG = (c >> 8) & 0xFF;
      clockNeonB = c & 0xFF;
    } else {
      clockNeonCustomColor = false;
    }
  }
  g_clkColorVer++;
}

void Clock::attach(Display &d) {
  d_ = &d;
  display = &d;
}

void Clock::configure(const Config &cfg) {
  theme_ = cfg.getInt("clock_theme", -1);
  if (theme_ < -1 || theme_ >= CLOCK_THEME_TOTAL) theme_ = -1;
  int mask = cfg.getInt("clock_themes_mask", 0x7FFF);
  if (mask == 0x3FFF) mask = 0x7FFF;  // l'ancien « tous » (10 themes + 4) devient « tous »
  mask_ = (mask > 0 && mask <= 0x7FFF) ? mask : 0x7FFF;
  const std::string col = cfg.get("clock_color");
  if (col.size() == 7 && col[0] == '#') {
    const unsigned long cv = std::strtoul(col.c_str() + 1, nullptr, 16);
    clockNeonR = (cv >> 16) & 0xFF;
    clockNeonG = (cv >> 8) & 0xFF;
    clockNeonB = cv & 0xFF;
    clockNeonCustomColor = true;
  } else {
    clockNeonCustomColor = false;
  }
  for (int i = 0; i < 5; i++) {
    clockAnimCol[i] = hexColor(cfg.get("clock_anim_" + std::to_string(i)));
    clockDigitCol[i] = hexColor(cfg.get("clock_digit_" + std::to_string(i)));
  }
  g_clkColorVer++;
  const std::string tz = cfg.get("tz");
  if (!tz.empty()) {  // chaine POSIX (ex. CET-1CEST,M3.5.0,M10.5.0/3), sinon on garde le fuseau du systeme
    setenv("TZ", tz.c_str(), 1);
    tzset();
  }
}

int Clock::pickTheme(int prev) {
  int allowed[16];
  int n = 0;
  for (int i = 0; i < CLOCK_THEME_TOTAL && i < 16; i++)
    if (mask_ & (1 << i)) allowed[n++] = i;
  if (n == 0)
    for (int i = 0; i < CLOCK_THEME_TOTAL && i < 16; i++) allowed[n++] = i;
  if (prev < 0) prev = lastDrawn_;  // jamais deux fois le meme d'une session a l'autre
  int t;
  if (n == 1) t = allowed[0];
  else {
    do t = allowed[random(0, n)];
    while (t == prev);
  }
  lastDrawn_ = t;
  return t;
}

void Clock::drawBanner() {
  d_->fillRect(0, 0, 128, 32, 0);
  d_->setFont(nullptr);
  d_->setTextSize(1);
  d_->setTextColor(Display::color565(255, 255, 255));
  const std::string name = clockThemeName(current_);
  int tx = (128 - (int)name.size() * 6) / 2;
  if (tx < 0) tx = 0;
  d_->setCursor((int16_t)tx, 12);
  d_->print(name);
}

void Clock::begin(uint64_t now, int forceTheme) {
  const bool preview = (forceTheme != -2);
  const int source = preview ? forceTheme : theme_;
  current_ = (source >= 0 && source < CLOCK_THEME_TOTAL) ? source : pickTheme(-1);
  themeStartMs_ = now;
  endMs_ = now + (uint64_t)durationSec_ * 1000ULL;  // la duree compte depuis le debut, bandeau du nom compris
  drawBanner();  // nom du theme pendant 800 ms
  bannerEndMs_ = now + 800;
  state_ = State::BANNER;
  d_->show();
}

bool Clock::tick(uint64_t now) {
  if (state_ == State::IDLE) return false;
  if (state_ == State::BANNER) {
    if (now < bannerEndMs_) return true;
    d_->clearScreen();
    d_->show();
    logf("CLOCK", "Start theme=%s id=%d", clockThemeName(current_), current_);
    state_ = State::RUN;
    lastDrawMs_ = 0;
  }
  if (now >= endMs_) {
    state_ = State::IDLE;
    d_->clearScreen();
    d_->show();
    logf("CLOCK", "End display");
    return false;
  }
  if (now - lastDrawMs_ >= 10) {  // le firmware dessine puis attend 10 ms
    lastDrawMs_ = now;
    int h = 0, m = 0, s = 0;
    getClockTime(h, m, s);
    clockDrawTheme(current_, h, m, s, (unsigned long)(now - themeStartMs_));
    d_->show();
  }
  return true;
}

void Clock::stop() { state_ = State::IDLE; }
