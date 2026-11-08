// clock.cpp -- horloge a themes. Reutilise TELS QUELS le code du firmware :
//   * clock_themes.h (5 themes retro dont Neon a 4 styles, v404) via la couche compat/ ;
//   * clock_ambient.inc (4 themes animes Plasma/Life/Stars/Rain, extrait du .ino par tools/port_clock_ambient.py).
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
int clockNeonStyle = 0;                                        // v404 : style du theme Neon (0 Neon, 1 Matrix, 2 Fire, 3 Rainbow)

#include "clock_themes.h"       // dossier parent (firmware) : RETRO_THEME_COUNT, drawRetroClockTheme(), retroThemeNames
#include "clock_ambient.inc"    // decoClockAmbient() et ses dependances

#pragma GCC diagnostic pop

namespace {
constexpr int CLOCK_THEME_TOTAL = RETRO_THEME_COUNT + 4;  // v404 : 5 retro (Tetris, Pac-Man, Invaders, Pong, Neon) + Plasma, Life, Stars, Rain

const char *clockThemeName(int i) {
  static const char *extra[4] = {"Plasma", "Life", "Stars", "Rain"};
  if (i == 4) return neonStyleNames[(clockNeonStyle >= 0 && clockNeonStyle <= 3) ? clockNeonStyle : 0];  // le theme 4 porte le nom de son style
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

void Clock::readCfg(const Config &cfg, int &theme, int &mask, int &neonStyle) {
  // v404 : 15 -> 9 themes. clock_schema >= 2 = numerotation compacte ; sinon migration de l'ancienne (0 Mario, 1 Tetris, 2 Pac-Man,
  // 3 Invaders, 4 Pong, 5 Neon, 6 Matrix, 7 Fire, 8 Rainbow, 9 Level 1-1, 10 Plasma, 11 Life, 12 Stars, 13 Rain, 14 Lava).
  static const int8_t MAP[15] = {-1, 0, 1, 2, 3, 4, 4, 4, 4, -1, 5, 6, 7, 8, -1};
  const bool v2 = cfg.getInt("clock_schema", 1) >= 2;
  int ns = cfg.getInt("clock_neon_style", 0);
  theme = cfg.getInt("clock_theme", -1);
  mask = cfg.getInt("clock_themes_mask", v2 ? 0x1FF : 0x7FFF);
  if (!v2) {
    if (theme >= 5 && theme <= 8) ns = theme - 5;
    theme = (theme >= 0 && theme < 15) ? MAP[theme] : -1;  // theme supprime -> aleatoire
    int nm = 0;
    for (int i = 0; i < 15; i++)
      if (((mask >> i) & 1) && MAP[i] >= 0) nm |= (1 << MAP[i]);
    mask = nm;
  }
  neonStyle = (ns >= 0 && ns <= 3) ? ns : 0;
  if (theme < -1 || theme >= CLOCK_THEME_TOTAL) theme = -1;
  if (!(mask > 0 && mask <= 0x1FF)) mask = 0x1FF;
}

void Clock::configure(const Config &cfg) {
  readCfg(cfg, theme_, mask_, cfgNeonStyle_);
  clockNeonStyle = cfgNeonStyle_;  const std::string col = cfg.get("clock_color");
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

void Clock::begin(uint64_t now, int forceTheme, int neonStyle) {
  clockNeonStyle = (neonStyle >= 0 && neonStyle <= 3) ? neonStyle : cfgNeonStyle_;  // v404 : un apercu d'un autre style ne survit pas a sa session
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
