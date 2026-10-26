#include "button.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstring>

#include "log.h"

#if defined(__linux__) && __has_include(<linux/gpio.h>)
#include <linux/gpio.h>
#define HAVE_GPIO_UAPI 1
#endif

namespace {
// broches libres avec le mapping hzeller « regular » et une seule chaine (GPIO2 / GPIO3 : horloge RTC I2C de la carte d'interface)
bool pinAllowed(int p) { return p == 21 || p == 16 || p == 19 || p == 20 || p == 26 || p == 13 || p == 12 || p == 5 || p == 6; }
}  // namespace

void Button::configure(const Config &cfg) {
  enabled_ = cfg.getBool("btn_enabled", false);
  const int p = cfg.getInt("btn_pin", 21);
  pin_ = pinAllowed(p) ? p : 21;
  auto act = [&](const char *k, int def) {
    const int a = cfg.getInt(k, def);
    return (a >= 0 && a <= 12) ? a : def;
  };
  actShort_ = act("btn_act_short", 1);
  actDouble_ = act("btn_act_double", 3);
  actLong_ = act("btn_act_long", 2);
  actVLong_ = act("btn_act_vlong", 6);
  const int lm = cfg.getInt("btn_long_ms", 1500), vm = cfg.getInt("btn_vlong_ms", 10000);
  longMs_ = (lm >= 500 && lm <= 5000) ? lm : 1500;
  vlongMs_ = (vm >= 3000 && vm <= 30000) ? vm : 10000;
}

void Button::start() {
  stop_ = false;
  th_ = std::thread([this] { loop(); });
}

void Button::stop() {
  stop_ = true;
  if (th_.joinable()) th_.join();
  closeLine();
}

bool Button::openLine(int pin) {
#ifdef HAVE_GPIO_UAPI
  closeLine();
  chipFd_ = open("/dev/gpiochip0", O_RDWR | O_CLOEXEC);
  if (chipFd_ < 0) {
    logf("BTN", "/dev/gpiochip0 inaccessible (%s)", std::strerror(errno));
    return false;
  }
  gpiohandle_request rq;
  std::memset(&rq, 0, sizeof(rq));
  rq.lineoffsets[0] = (unsigned)pin;
  rq.lines = 1;
  std::strncpy(rq.consumer_label, "recalboxdmd-btn", sizeof(rq.consumer_label) - 1);
#ifdef GPIOHANDLE_REQUEST_BIAS_PULL_UP
  rq.flags = GPIOHANDLE_REQUEST_INPUT | GPIOHANDLE_REQUEST_BIAS_PULL_UP;
  if (ioctl(chipFd_, GPIO_GET_LINEHANDLE_IOCTL, &rq) < 0) {
    logf("BTN", "pull-up refusee par le noyau (%s) : broche lue sans pull-up", std::strerror(errno));
    rq.flags = GPIOHANDLE_REQUEST_INPUT;
    if (ioctl(chipFd_, GPIO_GET_LINEHANDLE_IOCTL, &rq) < 0) rq.fd = -1;
  }
#else
  rq.flags = GPIOHANDLE_REQUEST_INPUT;
  if (ioctl(chipFd_, GPIO_GET_LINEHANDLE_IOCTL, &rq) < 0) rq.fd = -1;
#endif
  if (rq.fd < 0) {
    logf("BTN", "GPIO%d indisponible (%s)", pin, std::strerror(errno));
    closeLine();
    return false;
  }
  lineFd_ = rq.fd;
  openPin_ = pin;
  logf("BTN", "bouton sur GPIO%d", pin);
  return true;
#else
  (void)pin;
  return false;
#endif
}

void Button::closeLine() {
  if (lineFd_ >= 0) close(lineFd_);
  if (chipFd_ >= 0) close(chipFd_);
  lineFd_ = chipFd_ = -1;
  openPin_ = -1;
}

int Button::readLine() {
#ifdef HAVE_GPIO_UAPI
  if (lineFd_ < 0) return -1;
  gpiohandle_data d;
  std::memset(&d, 0, sizeof(d));
  if (ioctl(lineFd_, GPIOHANDLE_GET_LINE_VALUES_IOCTL, &d) < 0) return -1;
  return d.values[0] ? 1 : 0;
#else
  return -1;
#endif
}

void Button::loop() {
  using clk = std::chrono::steady_clock;
  auto ms = []() -> unsigned long {
    return (unsigned long)std::chrono::duration_cast<std::chrono::milliseconds>(clk::now().time_since_epoch()).count();
  };
  // etat de l'automate (memes noms que pollPhysicalButton)
  unsigned long highSince = 0, lowSince = 0, rawSince = 0, pressAt = 0, releaseAt = 0;
  bool stable = false, raw = false;
  int clicks = 0;
  unsigned long lastOpenTry = 0;
  const unsigned long bootMs = ms();
  // COUPE-BOUCLE (comme le firmware ESP32 v359) : une action « redemarrer » (10) ou « secours Wi-Fi » (9) venue du bouton moins de 5 min apres le demarrage du demon
  // est presque toujours un faux appui (broche flottante) : ignoree et journalisee, pour qu'un bouton absent ne puisse jamais faire boucler le demon.
  auto guarded = [&](int act) {
    if ((act == 9 || act == 10) && ms() - bootMs < 300000UL) {
      logf("BTN", "action %d ignoree (moins de 5 min apres le demarrage : appui suspect)", act);
      return;
    }
    run_(act);
  };
  while (!stop_) {
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    if (!enabled_.load()) {
      armed_ = false;
      level_ = -1;
      lowMs_ = 0;
      if (lineFd_ >= 0) closeLine();
      highSince = lowSince = 0;
      stable = raw = false;
      clicks = 0;
      std::this_thread::sleep_for(std::chrono::milliseconds(95));
      continue;
    }
    const unsigned long now = ms();
    if (lineFd_ < 0 || openPin_ != pin_.load()) {
      if (lineFd_ >= 0) closeLine();
      armed_ = false;
      if (now - lastOpenTry < 5000 && lastOpenTry != 0) continue;
      lastOpenTry = now;
      if (!openLine(pin_.load())) continue;
    }
    const int lv = readLine();
    level_ = lv;
    if (lv < 0) continue;
    // ARMEMENT (v333) : 1 s continue a l'etat haut avant de prendre le bouton en compte ; broche bloquee basse -> desarmee
    if (lv == 0) {
      if (lowSince == 0) lowSince = now;
      lowMs_ = (unsigned)(now - lowSince);
    } else {
      lowSince = 0;
      lowMs_ = 0;
    }
    if (lv == 1) {
      if (highSince == 0) highSince = now;
      if (!armed_.load() && now - highSince >= 1000) {
        armed_ = true;
        logf("BTN", "arme (repos haut >= 1 s)");
      }
    } else {
      highSince = 0;
    }
    if (armed_.load() && lv == 0 && lowMs_.load() > (unsigned)vlongMs_.load() + 3000U) {
      armed_ = false;
      logf("BTN", "broche bloquee a l'etat bas : desarme");
    }
    if (!armed_.load()) {
      stable = raw = false;
      clicks = 0;
      continue;
    }
    const bool r = (lv == 0);  // appuye = bas (pull-up)
    if (r != raw) {
      raw = r;
      rawSince = now;
    }
    if (r != stable && now - rawSince >= 30) {
      stable = r;
      if (stable) {
        pressAt = now;
      } else {
        const unsigned long held = now - pressAt;
        if (held >= (unsigned long)vlongMs_.load()) {  // appui « tres long » au RELACHEMENT (jamais pendant que la broche est tenue basse : une broche bloquee ne declenche rien)
          clicks = 0;
          guarded(actVLong_.load());
        } else if (held >= (unsigned long)longMs_.load()) {
          clicks = 0;
          guarded(actLong_.load());
        } else {
          clicks++;
          releaseAt = now;
          if (actDouble_.load() == 0) {  // pas d'action « double » : l'appui court part tout de suite
            clicks = 0;
            guarded(actShort_.load());
          } else if (clicks >= 2) {
            clicks = 0;
            guarded(actDouble_.load());
          }
        }
      }
    }
    if (!stable && clicks == 1 && now - releaseAt >= 350) {
      clicks = 0;
      guarded(actShort_.load());
    }
  }
}
