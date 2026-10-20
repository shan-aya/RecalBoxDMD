// clock.h -- horloge a themes (portage de showClock() du firmware, version NON BLOQUANTE).
//
// 15 themes : 10 « retro » pixel-art (clock_themes.h du firmware, utilise TEL QUEL : Mario, Tetris, Pac-Man, Space
// Invaders, Pong, Neon, Matrix, Fire, Rainbow, Level 1-1) + 5 sur fond anime (Plasma, Life, Stars, Rain, Lava, code extrait
// du .ino par tools/port_clock_ambient.py). Une session = nom du theme 800 ms, puis le theme pendant `duration` secondes.
// L'heure vient de l'horloge systeme (NTP) ; fuseau = cle TZ de config.ini (chaine POSIX) sinon celui du systeme.
#pragma once
#include <cstdint>
#include <string>

#include "config.h"
#include "display.h"

class Clock {
 public:
  void attach(Display &d);
  // Lit CLOCK_* : enabled, theme (-1 aleatoire), themes_mask, duree, couleurs, TZ.
  void configure(const Config &cfg);

  // Couleurs en direct (page « couleurs des horloges », sans ecriture sur la SD) : slot 0..4, a = animation, d = chiffres, n = Neon ; « #RRGGBB » ou vide (automatique)
  static void setLiveColors(int slot, const std::string *a, const std::string *d, const std::string *neon);

  void setDuration(int sec) { durationSec_ = sec < 1 ? 1 : sec; }  // WIDGET_DUR_0 (ou CLOCK_DURATION)
  int durationSec() const { return durationSec_; }

  // Demarre une session. forceTheme : -2 = selon la config (theme fixe ou tirage parmi le masque) ; 0..14 = theme impose.
  void begin(uint64_t now, int forceTheme = -2);
  // Avance la session ; false = terminee (l'ecran est efface).
  bool tick(uint64_t now);
  void stop();
  bool running() const { return state_ != State::IDLE; }

 private:
  enum class State { IDLE, BANNER, RUN };
  int pickTheme(int prev);
  void drawBanner();

  Display *d_ = nullptr;
  State state_ = State::IDLE;
  int theme_ = -1;          // CLOCK_THEME : -1 = aleatoire
  int mask_ = 0x7FFF;       // CLOCK_THEMES_MASK : themes admis au tirage aleatoire
  int durationSec_ = 60;
  int current_ = 0;
  uint64_t endMs_ = 0, bannerEndMs_ = 0, themeStartMs_ = 0, lastDrawMs_ = 0;
  int lastDrawn_ = -1;
};
