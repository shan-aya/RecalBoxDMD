#include "engine.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <random>
#include <sstream>

#include "log.h"
#include "brightsched.h"
#include "netutil.h"
#include "pngdecode.h"
#include "score.h"
#include "i18n.h"

namespace {
constexpr long SCORE_DISPLAY_DURATION_MS = 6000;         // duree par defaut de l'ecran de score
constexpr long SCORE_DURATION_OVERRIDE_MIN_MS = 500;     // bornes du prefixe « @<ms>| »
constexpr long SCORE_DURATION_OVERRIDE_MAX_MS = 15000;
const char *DEFAULT_RAW565_PATH = "/systems/_defaults/default.raw565";
const char *SHUFFLE_GIF_PATH = "/systems/_defaults/_shuffle.gif";  // cle ".gif" : voir v108 du firmware

bool isSlowFlag(char f) { return f == 'L' || f == 'l'; }

unsigned random_() {  // tirages du decor (creneau principal, melange des widgets)
  static std::mt19937 rng{std::random_device{}()};
  return (unsigned)rng();
}

// "/systems/<sys>/_default" -> "<sys>" ("" sinon)
std::string sysNameOfDefaultPath(const std::string &path) {
  if (!endsWith(path, "/_default")) return "";
  size_t s2 = path.rfind('/');
  if (s2 == std::string::npos || s2 == 0) return "";
  size_t s1 = path.rfind('/', s2 - 1);
  if (s1 == std::string::npos) return "";
  return path.substr(s1 + 1, s2 - s1 - 1);
}

std::string defaultPath(const std::string &sys) { return "/systems/_defaults/" + sys; }

// "/systems/<sys>/..." -> "<sys>" ; "" pour _defaults ou hors /systems/
std::string sysNameFromSystemsPath(const std::string &p) {
  if (!startsWith(p, "/systems/")) return "";
  if (startsWith(p, "/systems/_defaults/")) return "";
  size_t s0 = std::strlen("/systems/");
  size_t slash = p.find('/', s0);
  if (slash == std::string::npos) return "";
  return p.substr(s0, slash - s0);
}
}  // namespace

const char *modeName(Mode m) {
  switch (m) {
    case Mode::PLAYLIST: return "PLAYLIST";
    case Mode::GIF: return "GIF";
    case Mode::PNG: return "PNG";
    case Mode::BLACK: return "BLACK";
    case Mode::CONFIG: return "CONFIG";
    case Mode::SCORE: return "SCORE";
    case Mode::CLOCK: return "CLOCK";
    case Mode::ZEDMD: return "ZEDMD";
  }
  return "?";
}

// ------------------------------------------------------------------ horloge et decor

void Engine::loadDecoConfig() {
  // [WIDGETS] : WIDGET_MODE_<id> / WIDGET_DUR_<id> (anciennes cles DECO_* encore lues), [CLOCK] : CLOCK_ENABLED / CLOCK_DURATION
  // comme migration quand les cles WIDGET_*_0 n'existent pas (loadConfig() du firmware, v267-v283).
  auto key = [&](const char *base, int id) -> std::string {
    const std::string w = std::string("widget_") + base + std::to_string(id), o = std::string("deco_") + base + std::to_string(id);
    return cfg_.has(w) ? w : o;
  };
  auto keyPlain = [&](const char *base) -> std::string {
    const std::string w = std::string("widget_") + base, o = std::string("deco_") + base;
    return cfg_.has(w) ? w : o;
  };
  bool seen0 = false;
  for (int id = 0; id < 11; id++) {
    const std::string mk = key("mode_", id), dk = key("dur_", id);
    if (cfg_.has(mk)) {
      const int v = cfg_.getInt(mk, 0);
      if (v >= 0 && v <= 2) {
        deco_.mode[id] = (uint8_t)v;
        if (id == 0) seen0 = true;
      }
    }
    if (cfg_.has(dk)) {
      const int v = cfg_.getInt(dk, 0);
      if (v >= 1 && v <= 120) {
        deco_.dur[id] = (uint8_t)v;
        if (id == 0) seen0 = true;
      }
    }
  }
  const bool clockEnabled = cfg_.getInt("clock_enabled", 0) != 0;
  if (!cfg_.has(key("mode_", 0)) && !seen0) deco_.mode[0] = clockEnabled ? 1 : 0;
  if (!cfg_.has(key("dur_", 0))) {
    const int v = cfg_.getInt("clock_duration", 60);
    if (v >= 1 && v <= 120) deco_.dur[0] = (uint8_t)v;
  }
  deco_.mode[6] = 0;  // widget « statistiques » retire du firmware
  deco_.gifs = cfg_.getInt("clock_interval", 10);
  deco_.minutes = cfg_.getInt("clock_interval_min", 0);
  const std::string tk = keyPlain("trigger");
  deco_.trig = cfg_.has(tk) ? (cfg_.getInt(tk, 0) == 1 ? 1 : 0) : (deco_.minutes > 0 ? 1 : 0);
  deco_.widgetsOnly = cfg_.getInt(keyPlain("bg"), 0) == 1;
  {  // v352 du firmware : PREMIER DEMARRAGE sans playlist et sans aucun widget actif -> « widgets seuls » + horloge par defaut (on ne reste pas bloque sur
    // « choisissez une playlist »). Uniquement tant que first_boot=1 ; une carte preparee avec sa playlist n'est pas touchee.
    bool anyW = false;
    for (int i = 0; i < 11; i++)
      if (i != 6 && deco_.mode[i] != 0) anyW = true;
    if (cfg_.getBool("first_boot", true) && cfg_.playlistName.empty() && !anyW) {
      deco_.widgetsOnly = true;
      deco_.mode[0] = 1;
      deco_.dur[0] = 60; deco_.dur[1] = 10; deco_.dur[2] = 15; deco_.dur[3] = 10; deco_.dur[5] = 15;
      logf("BOOT", "1er demarrage sans playlist ni widget : widgets seuls + horloge par defaut");
    }
  }
  deco_.mainOrder = cfg_.getInt(keyPlain("main_order"), 0) == 1 ? 1 : 0;
  deco_.addMode = cfg_.getInt(keyPlain("add_mode"), 0) == 1 ? 1 : 0;
  const int ac = cfg_.getInt(keyPlain("add_count"), 1);
  deco_.addCount = (ac >= 1 && ac <= 10) ? ac : 1;
  clock_.setDuration(deco_.dur[0]);
  widgets_.attach(d_, fs_);
  widgets_.configure(cfg_, deco_.dur);
  logf("DECO", "horloge %s, duree %d s, declencheur %s (tous les %d GIF / %d min)", deco_.mode[0] ? "ACTIVE" : "inactive", deco_.dur[0],
       deco_.trig ? "temps" : "nombre de GIF", deco_.gifs, deco_.minutes);
}

bool Engine::decoAnyActive() const {
  for (int i = 0; i < 11; i++)
    if (i != 6 && deco_.mode[i]) return true;
  return false;
}

void Engine::startDecoSequence(uint64_t now) {
  int pool[11], addList[11];
  int n = 0, nAdd = 0;
  for (int i = 0; i < 11; i++) {
    if (i == 6) continue;
    if (deco_.mode[i] == 1) pool[n++] = i;
    else if (deco_.mode[i] == 2) addList[nAdd++] = i;
  }
  decoQueue_.clear();
  if (n == 0 && nAdd == 0) {  // rien d'actif (appel manuel) : l'horloge, jamais de sequence vide
    decoQueue_.push_back(0);
  } else {
    if (n > 0) {
      int pick = pool[0];
      if (deco_.mainOrder == 1) {  // tourniquet : prochain identifiant apres le dernier joue
        for (int k = 0; k < n; k++)
          if (pool[k] > lastMainId_) { pick = pool[k]; break; }
        lastMainId_ = pick;
      } else {  // aleatoire, jamais le meme deux fois de suite
        if (n > 1) {
          do pick = pool[random_() % n];
          while (pick == lastRandMain_);
        }
        lastRandMain_ = pick;
      }
      decoQueue_.push_back(pick);
    }
    if (nAdd > 1 && deco_.addMode == 1) {  // melange (Fisher-Yates), puis on n'en garde que addCount
      for (int k = nAdd - 1; k > 0; k--) {
        const int j = (int)(random_() % (unsigned)(k + 1));
        std::swap(addList[k], addList[j]);
      }
    }
    if (deco_.addMode == 1 && nAdd > deco_.addCount) nAdd = deco_.addCount;
    for (int k = 0; k < nAdd; k++) decoQueue_.push_back(addList[k]);
  }
  advanceDeco(now);
}

void Engine::advanceDeco(uint64_t now) {
  while (!decoQueue_.empty()) {
    const int id = decoQueue_.front();
    decoQueue_.erase(decoQueue_.begin());
    if (id == 0) {
      clock_.begin(now);
      mode_ = Mode::CLOCK;
      return;
    }
    // widget bloquant (comme dans le firmware) : il rend la main quand sa duree est ecoulee ou qu'une commande attend
    d_.clearScreen();
    if (!widgets_.run(id)) {  // interrompu : la sequence s'arrete, la commande en attente s'applique au prochain tour
      decoQueue_.clear();
      mode_ = modeAfterDeco_;
      if (mode_ == Mode::PNG) pngDrawn_ = false;
      d_.show();
      return;
    }
  }
  mode_ = modeAfterDeco_;  // fin de sequence : on reprend ce qui tournait (le GIF suivant est deja ouvert)
  if (mode_ == Mode::PNG) pngDrawn_ = false;
  d_.show();
}

void Engine::abortDeco() {
  clock_.stop();
  decoQueue_.clear();
  if (mode_ == Mode::CLOCK) mode_ = modeAfterDeco_;
}

void Engine::startClockPreview(const std::string &arg) {  // « clock_preview <0..14> » : session d'horloge forcee (test)
  modeAfterDeco_ = Mode::BLACK;
  decoQueue_.clear();
  clock_.begin(nowMs(), arg.empty() ? -1 : std::atoi(arg.c_str()));
  mode_ = Mode::CLOCK;
}

void Engine::applyThemeControl() {
  std::string theme, lang, region;
  {
    std::lock_guard<std::mutex> lk(tctl_.m);
    themeSeen_ = tctl_.version.load();
    theme = tctl_.haveTheme ? tctl_.theme : themes_.wantedTheme();
    lang = tctl_.haveOpt ? tctl_.lang : themes_.wantedLang();
    region = tctl_.haveOpt ? tctl_.region : themes_.wantedRegion();
  }
  const bool seenChanged = themes_.noteSeen(theme);  // v398 : theme annonce -> liste « vus » (+ coche d'office si reels_auto_new), AVANT setWanted
  const bool changed = themes_.setWanted(fs_, theme, lang, region);
  if (seenChanged) cfg_.writeKeys(fs_, {{"reels_seen", themes_.reelsSeen()}, {"reels_themes", themes_.reelsChecked()}});
  if (themes_.takePersist()) {  // memorise la derniere selection (theme_last / lang_last / region_last), seulement quand elle change
    cfg_.writeKeys(fs_, {{"theme_last", themes_.wantedTheme()}, {"lang_last", themes_.wantedLang()}, {"region_last", themes_.wantedRegion()}});
    logf("THEME", "derniere selection memorisee (config.ini) : theme=%s langue=%s region=%s", themes_.wantedTheme().c_str(),
         themes_.wantedLang().c_str(), themes_.wantedRegion().c_str());
  }
  // la selection a change et un logo de systeme est a l'ecran : on le redessine (sinon visible seulement au systeme suivant)
  if (changed && !lastSys_.empty() && mode_ == Mode::PNG && startsWith(currentPngPath_, "/systems/_defaults/")) {
    logf("THEME", "logo systeme a l'ecran redessine : %s", lastSys_.c_str());
    pngDrawn_ = false;
    currentPngPath_.clear();
    cmdSystem(lastSys_);
    d_.show();
  }
}

// ------------------------------------------------------------------ page web (jalon 6)

bool Engine::runOnMain(std::function<void()> fn) {
  auto pr = std::make_shared<std::promise<void>>();
  auto fut = pr->get_future();
  {
    std::lock_guard<std::mutex> lk(mainMu_);
    mainTasks_.emplace_back(std::move(fn), pr);
  }
  return fut.wait_for(std::chrono::seconds(8)) == std::future_status::ready;
}

void Engine::postMain(std::function<void()> fn) {
  auto pr = std::make_shared<std::promise<void>>();
  std::lock_guard<std::mutex> lk(mainMu_);
  mainTasks_.emplace_back(std::move(fn), pr);
}

void Engine::pumpMain() {
  for (;;) {
    std::pair<std::function<void()>, std::shared_ptr<std::promise<void>>> task;
    {
      std::lock_guard<std::mutex> lk(mainMu_);
      if (mainTasks_.empty()) return;
      task = std::move(mainTasks_.front());
      mainTasks_.pop_front();
    }
    task.first();
    task.second->set_value();
  }
}

bool Engine::mainTaskPending() {
  std::lock_guard<std::mutex> lk(mainMu_);
  return !mainTasks_.empty();
}

// config.ini vient d'etre modifie (page web) : on relit les valeurs typees et on applique ce qui peut l'etre en direct.
// Le mode natif / Pinball / fenetre RA ne s'appliquent qu'au redemarrage, comme dans le firmware.
void Engine::reloadConfig() {
  cfg_.refreshTyped();
  d_.setBrightness8(cfg_.brightness8());
  playlist_.configure(cfg_.playlistName, cfg_.playlistRandom);
  clock_.configure(cfg_);
  loadDecoConfig();  // reglages du decor + widgets
  themes_.setLangPriority(cfg_.get("feat_lang_priority", "0") == "1");
  const bool reelsChanged = themes_.setReels(fs_, cfg_.getInt("feat_reels_mode", 0), cfg_.get("reels_auto_new", "0") == "1", cfg_.get("reels_themes"));  // v398
  const bool followChanged = themes_.setFollow(fs_, cfg_.getBool("feat_theme_follow", true));
  if ((reelsChanged || followChanged) && !lastSys_.empty() && mode_ == Mode::PNG && startsWith(currentPngPath_, "/systems/_defaults/")) {
    pngDrawn_ = false;
    currentPngPath_.clear();
    cmdSystem(lastSys_);  // « suivi du theme » (de)coche : le logo a l'ecran est redessine tout de suite
    d_.show();
  }
  loadBrightSched();
  logf("WEB", "configuration relue");
  if (reloadHook_) reloadHook_();
}

void Engine::gameLogosChanged(const std::vector<std::string> &games) {
  if (lastGameArg_.empty()) return;
  const std::string cur = toLower(lastGameArg_);
  for (const std::string &g : games)
    if (toLower(g) == cur) {
      pngDrawn_ = false;
      currentPngPath_.clear();
      cmdGame(lastGameArg_);  // le jeu a l'ecran est redessine avec son nouveau logo (ou le repli si le logo vient d'etre supprime)
      d_.show();
      logf("WEB", "logo de jeu : %s redessine", lastGameArg_.c_str());
      return;
    }
}

void Engine::reelsRefresh() {
  themes_.refresh(fs_);
  if (!lastSys_.empty() && mode_ == Mode::PNG && startsWith(currentPngPath_, "/systems/_defaults/")) {
    pngDrawn_ = false;
    currentPngPath_.clear();
    cmdSystem(lastSys_);  // le logo a l'ecran est redessine avec les nouveaux fichiers
    d_.show();
  }
  logf("WEB", "logos reels : fichiers recharges");
}

// deux couleurs : le prefixe d'instruction dans cfgSubColor_, la valeur (SSID, adresse) en blanc
void Engine::drawSubAt(int x) {
  d_.setTextWrap(false);
  d_.setTextSize(1);
  if (cfgSubWhiteFrom_ >= 0 && cfgSubWhiteFrom_ < (int)cfgSubMsg_.size()) {
    const std::string prefix = cfgSubMsg_.substr(0, (size_t)cfgSubWhiteFrom_), value = cfgSubMsg_.substr((size_t)cfgSubWhiteFrom_);
    d_.setTextColor(cfgSubColor_);
    d_.setCursor((int16_t)x, 24);
    d_.print(prefix);
    d_.setTextColor(0xFFFF);
    d_.setCursor((int16_t)(x + (int)prefix.size() * 6), 24);
    d_.print(value);
  } else {
    d_.setTextColor(cfgSubColor_);
    d_.setCursor((int16_t)x, 24);
    d_.print(cfgSubMsg_);
  }
}

void Engine::drawConfigScreen() {
  scroll1_ = scroll2_ = 0;
  subPauseUntilMs_ = 0;
  d_.clearScreen();
  d_.setFont(nullptr);
  d_.setTextWrap(false);
  d_.setTextSize(1);
  d_.setTextColor(0xFFE0);
  d_.setCursor(1, 4);
  d_.print(cfgMainMsg_);
  d_.fillRect(0, 24, 128, 8, 0);
  drawSubAt(1);
  d_.show();
}

// Defilement horizontal : pas de 4 px toutes les 100 ms ; ligne 2 : pause de 1,8 s quand la fin de la chaine (la valeur en blanc) devient
// entierement visible (loop() du firmware, MODE_CONFIG)
void Engine::tickConfigScroll(uint64_t now) {
  bool changed = false;
  const int w1 = (int)cfgMainMsg_.size() * 6;
  if (w1 > 128 && now - lastScroll1Ms_ > 100) {
    scroll1_ = (scroll1_ + 4) % (w1 + 32);
    lastScroll1Ms_ = now;
    d_.setFont(nullptr);
    d_.fillRect(0, 4, 128, 8, 0);
    d_.setTextWrap(false);
    d_.setTextColor(0xFFE0);
    d_.setCursor((int16_t)(1 - scroll1_), 4);
    d_.print(cfgMainMsg_);
    changed = true;
  }
  const int w2 = (int)cfgSubMsg_.size() * 6;
  if (w2 > 128 && now - lastScroll2Ms_ > 100) {
    if (subPauseUntilMs_ != 0 && now < subPauseUntilMs_) {
      // en pause
    } else {
      subPauseUntilMs_ = 0;
      const int reveal = w2 - 128;
      const int next = scroll2_ + 4;
      const bool justRevealed = scroll2_ < reveal && next >= reveal;
      scroll2_ = next % (w2 + 32);
      lastScroll2Ms_ = now;
      d_.fillRect(0, 24, 128, 8, 0);
      drawSubAt(1 - scroll2_);
      if (justRevealed) subPauseUntilMs_ = now + 1800;
      changed = true;
    }
  }
  if (changed) d_.show();
}

// ecran « configuration » persistant (premier demarrage, point d'acces) : titre, message de la ligne 2 et sa mise en forme
void Engine::configScreen(const std::string &main, const std::string &sub, uint16_t subColor, int whiteFrom) {
  logf("CONFIG", "ecran « %s » / « %s »", main.c_str(), sub.c_str());
  cfgMainMsg_ = main;
  cfgSubMsg_ = sub;
  cfgSubColor_ = subColor;
  cfgSubWhiteFrom_ = whiteFrom;
  cfgPersistentSub_ = sub;
  cfgPersistentColor_ = subColor;
  cfgPersistentWhite_ = whiteFrom;
  cfgSubSetMs_ = 0;  // jamais d'expiration : c'est le message permanent
  configActive_ = true;
  webPaused_ = false;  // ecran de premier demarrage / point d'acces : jamais de reprise automatique
  if (zed_.native()) zed_.setOverlay(true);
  else {
    abortDeco();
    clock_.stop();
    closeGifAll();
    gifOpened_ = false;
    currentPngPath_.clear();
    pngDrawn_ = false;
    mode_ = Mode::CONFIG;
  }
  drawConfigScreen();
}

void Engine::configMainLine(const std::string &main) {
  cfgMainMsg_ = main;
  scroll1_ = 0;
  lastScroll1Ms_ = nowMs();
  d_.setFont(nullptr);
  d_.setTextWrap(false);
  d_.fillRect(0, 4, 128, 8, 0);
  d_.setTextColor(0xFFE0);
  d_.setCursor(1, 4);
  d_.print(cfgMainMsg_);
  d_.show();
}

void Engine::configSubLine(const std::string &sub, uint16_t subColor, int whiteFrom) {
  cfgSubMsg_ = sub;
  cfgSubColor_ = subColor;
  cfgSubWhiteFrom_ = whiteFrom;
  cfgPersistentSub_ = sub;
  cfgPersistentColor_ = subColor;
  cfgPersistentWhite_ = whiteFrom;
  cfgSubSetMs_ = 0;
  scroll2_ = 0;
  subPauseUntilMs_ = 0;
  d_.fillRect(0, 24, 128, 8, 0);
  drawSubAt(1);
  d_.show();
}
// Ecran de configuration : le titre en haut, le message (page ouverte, « Enregistrement »...) en bas.
void Engine::webTouch() {
  webLastMs_ = nowMs();
  webLeaveAtMs_ = 0;  // une page est (re)ouverte ou vivante : le depart annonce est annule
}

void Engine::webLeaveSoon(uint64_t delayMs) {
  // ignoree si une pause vient d'avoir lieu (< 3 s) : au F5 la requete de la page rechargee peut passer AVANT l'annonce de depart de l'ancienne
  if (webPaused_ && nowMs() - webPausedAtMs_.load() >= 3000) webLeaveAtMs_ = nowMs() + delayMs;
}

// Reprise automatique du DMD quand plus aucune page de configuration n'est ouverte (voir engine.h).
void Engine::tickWebWatchdog(uint64_t now) {
  if (!webPaused_ || !configActive_) return;
  constexpr uint64_t WEB_IDLE_MS = 180000;  // 3 min sans battement (20 s) ni message : navigateur plante, Wi-Fi coupe, onglet masque
  if (webBusy_ && webBusy_()) {
    webLastMs_ = now;
    return;
  }
  const uint64_t leaveAt = webLeaveAtMs_.load();
  const char *why = nullptr;
  if (leaveAt != 0 && now >= leaveAt) why = "page fermee";
  else if (now - webLastMs_.load() > WEB_IDLE_MS) why = "aucune activite depuis 3 min";
  if (!why) return;
  logf("WEB", "reprise automatique du DMD (%s)", why);
  webLeaveAtMs_ = 0;
  webResume();
}

void Engine::webPause(const std::string &msg, uint16_t color) {
  webPaused_ = true;
  webPausedAtMs_ = nowMs();
  webTouch();
  cfgSubMsg_ = msg;
  cfgSubColor_ = color;
  cfgSubWhiteFrom_ = -1;
  cfgSubSetMs_ = nowMs();
  configActive_ = true;
  logf("WEB", "DMD pause : %s", msg.c_str());
  if (zed_.native()) {  // le flux d'ES continue d'etre decode mais n'est plus dessine ; le message apparait par-dessus
    zed_.setOverlay(true);
    drawConfigScreen();
    return;
  }
  abortDeco();
  clock_.stop();
  closeGifAll();
  gifOpened_ = false;
  currentPngPath_.clear();
  pngDrawn_ = false;
  mode_ = Mode::CONFIG;
  drawConfigScreen();
}

void Engine::webOpen(const std::string &msg, const std::string &ip) {
  cfgMainMsg_ = "WEB DMD CONFIG";
  cfgPersistentSub_ = msg + " " + ip;
  cfgPersistentColor_ = 0xFFFF;
  cfgPersistentWhite_ = -1;
  webPause(cfgPersistentSub_, 0xFFFF);
}

void Engine::webPage(const std::string &url) {
  cfgMainMsg_ = "WEB DMD CONFIG";
  cfgPersistentSub_ = url;
  cfgPersistentColor_ = 0xFFE0;
  cfgPersistentWhite_ = -1;
  webPause(url, 0xFFE0);
}

void Engine::showFirstBootScreen() {
  const std::string lang = (cfg_.uiLanguage == "en" || cfg_.uiLanguage == "es") ? cfg_.uiLanguage : "fr";
  const std::string main = lang == "en" ? "Configuration page" : lang == "es" ? "Pagina de configuracion" : "Page de configuration";
  const NetInfo ni = netInfo();
  std::string sub;
  int white = -1;
  if (ni.ip.empty()) {
    sub = lang == "en" ? "Connecting to network..." : lang == "es" ? "Conectando a la red..." : "Connexion au reseau...";
  } else {
    const std::string pre = lang == "en" ? "Open http://" : lang == "es" ? "Abrir http://" : "Ouvrir http://";
    sub = pre + ni.ip;
    white = (int)pre.size();
  }
  configScreen(main, sub, 0x07E0, white);
}

void Engine::webResume() {
  diagActive_ = false;
  webPaused_ = false;
  webLeaveAtMs_ = 0;
  // premier demarrage : l'affichage normal ne reprend qu'apres une sauvegarde conforme (first_boot passe a 0 dans /save)
  if (cfg_.getBool("first_boot", true) && !zed_.native()) {
    logf("WEB", "premier demarrage : reprise refusee, l'ecran de configuration reste affiche jusqu'a une sauvegarde valide");
    showFirstBootScreen();
    return;
  }
  logf("WEB", "DMD reprise -> retour a l'affichage normal");
  configActive_ = false;
  clock_.stop();
  clock_.setDuration(deco_.dur[0]);
  screenOff_ = false;
  d_.setBrightness8(cfg_.brightness8());  // fin d'un eventuel apercu de luminosite
  if (zed_.native()) {  // rien a reprendre : l'ecran reste a ES, on redessine sa derniere image
    zed_.setOverlay(false);
    return;
  }
  if (mode_ == Mode::CONFIG || mode_ == Mode::CLOCK) {
    resumePlaylist();
    d_.show();
    helloRequested_ = true;  // la Recalbox renvoie son etat (comme webDmdResume)
  }
}

void Engine::previewBrightness(int pct) {  // RAM seulement : le reglage reel (config.ini) ne change pas
  if (pct >= 0 && pct <= 100) {
    d_.setBrightness8((uint8_t)(pct * 255 / 100));
    d_.show();  // la luminosite n'est appliquee qu'a la presentation de l'image
  }
}

// apercu d'un theme d'horloge : « -1 » (au hasard) .. « 14 », ou « stop » (on quitte la page : retour a l'ecran de configuration)
void Engine::clockPreview(const std::string &theme) {
  if (theme == "stop") {
    if (mode_ == Mode::CLOCK) {
      clock_.stop();
      clock_.setDuration(deco_.dur[0]);
      if (configActive_) {
        mode_ = Mode::CONFIG;
        drawConfigScreen();
      } else {
        webResume();
      }
    }
    return;
  }
  if (theme.size() > 1 && theme[0] == 'w') {  // apercu d'un widget du decor : une duree, puis retour a l'ecran de configuration
    abortDeco();
    clock_.stop();
    closeGifAll();
    gifOpened_ = false;
    currentPngPath_.clear();
    pngDrawn_ = false;
    mode_ = Mode::BLACK;
    d_.clearScreen();
    logf("DECO", "apercu widget %s", theme.c_str());
    widgets_.previewSpec(theme.substr(1));
    mode_ = Mode::CONFIG;
    drawConfigScreen();
    return;
  }
  clock_.setDuration(300);  // l'apercu dure jusqu'a « stop » (ou 5 minutes)
  startClockPreview(theme);
}
// ------------------------------------------------------------------ bouton physique (btnRunAction du firmware)
void Engine::buttonAction(int a) {
  if (a <= 0 || a > 12) return;
  if (diagActive_.load()) {  // un nouvel appui quitte le mode diagnostic
    diagStop();
    return;
  }
  logf("BTN", "action %d", a);
  // mode Recalbox natif : l'ecran appartient a EmulationStation, seules les actions de maintenance s'appliquent
  const bool maint = (a == 4 || a == 5 || a == 6 || a == 7 || a == 9 || a == 10 || a == 12);
  if (zed_.native() && !maint) {
    logf("BTN", "ignore (mode natif)");
    return;
  }
  const uint64_t now = nowMs();
  switch (a) {
    case 1:  // GIF suivant
      if (!configActive_) requestNextGif_ = true;
      break;
    case 2:  // pause / reprise
      if (configActive_) {
        infoUntilMs_ = 0;
        webResume();
      } else {
        cfgMainMsg_ = "PAUSE";
        cfgPersistentSub_.clear();
        webPause(L3(cfg_.uiLanguage, "BOUTON : REPRISE", "BUTTON: RESUME", "BOTON: REANUDAR"), 0xFFE0);
      }
      break;
    case 3:  // sequence des widgets maintenant
    case 11: // horloge seule, meme si elle est desactivee
      if (configActive_ || mode_ == Mode::ZEDMD || mode_ == Mode::SCORE) break;
      if (mode_ != Mode::CLOCK) modeAfterDeco_ = mode_;
      if (a == 3) {
        startDecoSequence(now);
      } else {
        decoQueue_.clear();
        clock_.begin(now);
        mode_ = Mode::CLOCK;
      }
      break;
    case 4: {  // luminosite : cycle 25 / 50 / 75 / 100 %
      const int cur = (int)std::lround(cfg_.screenBrightness * 100.0 / 255.0);
      const int next = (cur < 25) ? 25 : (cur < 50) ? 50 : (cur < 75) ? 75 : (cur < 100) ? 100 : 25;
      cfg_.writeKeys(fs_, {{"brightness", std::to_string(next)}});
      cfg_.refreshTyped();
      screenOff_ = false;
      d_.setBrightness8(cfg_.brightness8());
      d_.show();
      logf("BTN", "luminosite -> %d%%", next);
      break;
    }
    case 5:  // ecran eteint / allume
      screenOff_ = !screenOff_;
      d_.setBrightness8(screenOff_ ? 0 : cfg_.brightness8());
      d_.show();
      logf("BTN", "ecran %s", screenOff_ ? "eteint" : "allume");
      break;
    case 6: {  // ecran d'information : adresse, version, duree de fonctionnement
      const NetInfo ni = netInfo();
      const std::string ip = ni.ip.empty() ? "no network" : ni.ip;
      cfgMainMsg_ = ip;
      cfgPersistentSub_.clear();
      webPause("Pi 3.53 UP " + std::to_string((unsigned long)(now / 60000UL)) + "min", 0x07FF);
      infoUntilMs_ = now + 6000;
      break;
    }
    case 7: {  // adresse de la page de configuration
      const NetInfo ni = netInfo();
      webPage("http://" + (ni.ip.empty() ? std::string("0.0.0.0") : ni.ip));
      break;
    }
    case 8:  // reprendre le DMD
      if (configActive_) {
        infoUntilMs_ = 0;
        webResume();
      }
      break;
    case 9:  // point d'acces de secours
      if (wifiRecoveryHook_) wifiRecoveryHook_();
      break;
    case 12:  // diagnostic Wi-Fi / liaison Recalbox (en boucle, un nouvel appui le quitte)
      diagStart(false);
      break;
    case 10:  // redemarrer
      restart_ = true;
      break;
  }
}
void Engine::start() {
  if (zed_.native()) {  // mode natif : pas de playlist, l'ecran appartient au flux d'EmulationStation
    zed_.enterFromBoot(nowMs());
    mode_ = Mode::ZEDMD;
    return;
  }
  if (cfg_.getBool("first_boot", true)) {  // premier demarrage : ecran de configuration, pas de playlist (needWebConfigMode du firmware)
    showFirstBootScreen();
    return;
  }
  resumePlaylist();
  d_.show();
}

void Engine::begin() {
  if (!sys_.loadOrBuild(fs_)) logf("CACHE", "systems_cache.dat absent et /systems introuvable : tous les systemes seront « inconnus »");
  d_.setBrightness8(cfg_.brightness8());
  playlist_.configure(cfg_.playlistName, cfg_.playlistRandom);
  clock_.attach(d_);
  clock_.configure(cfg_);
  zed_.configure();
  themes_.begin(cfg_, fs_);
  loadBrightSched();
  themeSeen_ = tctl_.version.load();
  loadDecoConfig();
  decoLastMs_ = nowMs();
  logf("BOOT", "playlist='%s' : %d GIF", cfg_.playlistName.c_str(), cfg_.playlistName.empty() ? 0 : playlist_.count(fs_));
}

// ------------------------------------------------------------------ medias

bool Engine::drawRaw565(const std::string &rawPath) {
  // sous-dossier alphabetique d'abord, puis chemin plat (jamais deux fois le meme chemin : cas de _defaults)
  const std::string sub = alphaSubdirPath(rawPath);
  std::string use;
  const std::string langPath = themes_.defaultsLangPath(rawPath);  // traduction de l'image par defaut (langue de la Recalbox)
  if (!langPath.empty() && fs_.exists(langPath)) use = langPath;
  else if (fs_.exists(sub)) use = sub;
  else if (sub != rawPath && fs_.exists(rawPath)) use = rawPath;
  else return false;
  std::vector<uint8_t> buf;
  if (!fs_.readAll(use, buf, RAW565_FRAME_BYTES) || buf.size() != RAW565_FRAME_BYTES) return false;
  d_.drawRGBBitmap(0, 0, reinterpret_cast<const uint16_t *>(buf.data()), DMD_W, DMD_H);
  return true;
}

bool Engine::ensureDefaultRaw565Cached() {
  int64_t mt = fs_.mtime(DEFAULT_RAW565_PATH);
  if (defaultCached_ && mt == defaultMtime_) return true;
  std::vector<uint8_t> buf;
  if (!fs_.readAll(DEFAULT_RAW565_PATH, buf, RAW565_FRAME_BYTES) || buf.size() != RAW565_FRAME_BYTES) {
    logf("CACHE", "default.raw565 absent ou incomplet");
    defaultCached_ = false;
    return false;
  }
  defaultBuf_.resize(DMD_W * DMD_H);
  std::memcpy(defaultBuf_.data(), buf.data(), RAW565_FRAME_BYTES);
  defaultCached_ = true;
  defaultMtime_ = mt;
  return true;
}

bool Engine::drawDefaultRaw565Cached() {
  if (!ensureDefaultRaw565Cached()) return false;
  d_.drawRGBBitmap(0, 0, defaultBuf_.data(), DMD_W, DMD_H);
  return true;
}

// ------------------------------------------------------------------ message « RecalBox connectee » (surimpression)
// Memes regles que le firmware ESP32 (v357) : message de 2 lignes, TAILLE IDENTIQUE dans toutes les langues (calculee sur les 3 traductions : taille 2 a
// pas de 11 px, soit 11 caracteres par ligne, sinon taille 1), contour noir complet, 5 s par-dessus l'image de secours puis le logo seul.
namespace {
struct OvText { std::string l1, l2; };
// fond assombri a 30 % pendant le message (lisibilite : le logo reste visible en filigrane mais ne parasite plus les lettres)
uint16_t dimRgb565(uint16_t p) {
  const unsigned r = ((p >> 11) & 31) * 77 >> 8, g = ((p >> 5) & 63) * 77 >> 8, b = (p & 31) * 77 >> 8;
  return (uint16_t)((r << 11) | (g << 5) | b);
}
OvText trConnectedFor(const std::string &lang) {
  OvText t;
  t.l1 = "RecalBox";   // v365 du firmware : plus de smiley
  t.l2 = lang == "en" ? "connected" : lang == "es" ? "conectada" : "connectee";
  return t;
}
// v365 du firmware -- CENTRAGE SUR L'ENCRE : certains glyphes laissent 1 ou 2 colonnes vides a leur bord (« i », « l »...) ; table copiee du firmware (colonnes vides gauche = quartet haut, droite = quartet bas)
static const uint8_t kGlyphBlank[95] = {   // colonnes vides a GAUCHE (quartet haut) et a DROITE (quartet bas) de chaque glyphe 5x7, ' ' (32) a '~' (126), tirees de glcdfont.c
  0x00, 0x22, 0x11, 0x00, 0x00, 0x00, 0x00, 0x11, 0x11, 0x11, 0x00, 0x00, 0x11, 0x00, 0x21, 0x00,
  0x00, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x22, 0x12, 0x10, 0x00, 0x10, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x10, 0x00, 0x00,
  0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x11, 0x01, 0x01, 0x11, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x22, 0x11, 0x00
};
uint8_t glyphBlank(unsigned char c) { return (c >= 32 && c <= 126) ? kGlyphBlank[c - 32] : 0; }
OvText trDisconnectedFor(const std::string &lang) {
  OvText t;
  t.l1 = "RecalBox";
  t.l2 = lang == "en" ? "offline" : lang == "es" ? "offline" : "hors ligne";
  return t;
}
OvText trNoWifiFor(const std::string &lang) {
  OvText t;
  if (lang == "en") { t.l1 = "No wifi"; t.l2 = "No Recalbox"; }
  else if (lang == "es") { t.l1 = "Sin wifi"; t.l2 = "ni Recalbox"; }
  else { t.l1 = "ni wifi"; t.l2 = "ni recalbox"; }
  return t;
}
int overlaySize(OvText (*fn)(const std::string &)) {
  size_t maxLen = 0;
  for (const char *lg : {"fr", "en", "es"}) {
    const OvText t = fn(lg);
    maxLen = std::max(maxLen, std::max(t.l1.size(), t.l2.size()));
  }
  return (int)maxLen * 11 - 1 <= DMD_W ? 2 : 1;
}
void drawOverlayText(Display &d, const OvText &t, uint16_t color, int size) {
  const int adv = size == 2 ? 11 : 6, glyphW = 5 * size, glyphH = 7 * size, lineH = glyphH + 2, blockH = lineH * 2;
  const int y0 = (DMD_H - blockH) / 2;
  const std::string *lines[2] = {&t.l1, &t.l2};
  for (int i = 0; i < 2; i++) {
    const std::string &s = *lines[i];
    const int n = (int)s.size();
    const int blankL = n ? (glyphBlank((unsigned char)s[0]) >> 4) : 0, blankR = n ? (glyphBlank((unsigned char)s[n - 1]) & 15) : 0;
    const int inkW = (n - 1) * adv + (5 - blankL - blankR) * size;
    int x = (DMD_W - inkW) / 2 - blankL * size;
    if (x < 0) x = 0;
    const int y = y0 + 1 + i * lineH;
    // 2 passes : TOUS les contours d'abord, puis les lettres (le contour du caractere suivant ne doit pas manger le bord du precedent : pas de 11 px)
    for (int k = 0; k < n; k++) {
      const int cx = x + k * adv;
      for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++)
          if (dx || dy) d.drawChar((int16_t)(cx + dx), (int16_t)(y + dy), (unsigned char)s[k], 0, 0, (uint8_t)size);  // contour (bg == couleur : transparent)
    }
    for (int k = 0; k < n; k++) d.drawChar((int16_t)(x + k * adv), (int16_t)y, (unsigned char)s[k], color, color, (uint8_t)size);
  }
}
}  // namespace

void Engine::drawConnectedFrame(bool visible) { drawOverlayFrame(visible, 0); }

void Engine::drawOverlayFrame(bool visible, int kind) {
  if (!visible) {  // temps « eteint » du clignotement et fin des 5 s : l'image de secours seule
    drawDefaultRaw565Cached();
    return;
  }
  std::vector<uint16_t> dim(defaultBuf_.size());
  for (size_t i = 0; i < dim.size(); i++) dim[i] = dimRgb565(defaultBuf_[i]);
  d_.drawRGBBitmap(0, 0, dim.data(), DMD_W, DMD_H);
  std::string lang = cfg_.get("language");
  if (lang != "en" && lang != "es") lang = "fr";
  OvText (*fn)(const std::string &) = kind == 1 ? trNoWifiFor : kind == 2 ? trDisconnectedFor : trConnectedFor;
  const uint16_t color = kind == 1 ? Display::color565(255, 0, 0) : kind == 2 ? Display::color565(255, 140, 0) : Display::color565(0, 255, 0);
  drawOverlayText(d_, fn(lang), color, overlaySize(fn));
}

void Engine::showNoWifiAlert() {
  if (mode_ == Mode::ZEDMD) { noWifiPending_ = false; return; }  // flux natif / Pinball en cours : pas d'alerte par-dessus
  if (mode_ == Mode::CLOCK) abortDeco();
  closeGifAll();
  gifOpened_ = false;
  displayedMaskSys_.clear();
  d_.clearScreen();
  const bool ok = ensureDefaultRaw565Cached();
  mode_ = ok ? Mode::PNG : Mode::BLACK;
  currentPngPath_ = DEFAULT_RAW565_PATH;
  pngDrawn_ = ok;
  if (ok) drawOverlayFrame(true, 1);
  d_.show();
  noWifiActive_ = true;
  noWifiVisible_ = true;
  noWifiBlinkMs_ = nowMs();
  noWifiUntilMs_ = nowMs() + 5000;
  noWifiPending_ = false;
  logf("WIFI", "No wifi, No Recalbox -- alerte affichee");
}

void Engine::showDisconnectedAlert() {
  if (mode_ == Mode::ZEDMD) { discPending_ = false; return; }  // la Recalbox, chargee par VPX, repond parfois tard aux PING
  if (mode_ == Mode::CLOCK) abortDeco();
  closeGifAll();
  gifOpened_ = false;
  displayedMaskSys_.clear();
  d_.clearScreen();
  const bool ok = ensureDefaultRaw565Cached();
  mode_ = ok ? Mode::PNG : Mode::BLACK;
  currentPngPath_ = DEFAULT_RAW565_PATH;
  pngDrawn_ = ok;
  if (ok) drawOverlayFrame(true, 2);
  d_.show();
  discActive_ = true;
  discVisible_ = true;
  discBlinkMs_ = nowMs();
  discUntilMs_ = nowMs() + 5000;
  discPending_ = false;
  logf("CMD", "RecalBox non connectee -- alerte affichee");
}

void Engine::clearAlerts() {
  noWifiActive_ = noWifiPending_ = discActive_ = discPending_ = false;
}

void Engine::tickAlerts(uint64_t now) {
  static const uint64_t bootMs = now;  // la surveillance du reseau attend 15 s apres le demarrage (NetworkManager et 1re connexion Wi-Fi)
  // « ni wifi / ni recalbox » : plus de route par defaut (Wi-Fi ou Ethernet) pendant >= 1 s ; 3 alertes au plus, la 2e et la 3e a 20 s d'intervalle (firmware : WiFi.status())
  if (now - lastNetPollMs_ >= 1000) {
    lastNetPollMs_ = now;
    bool up = false;
    std::ifstream rf("/proc/net/route");
    std::string line;
    std::getline(rf, line);
    while (std::getline(rf, line)) {
      std::istringstream ls(line);
      std::string ifc, dest, gw, flags;
      ls >> ifc >> dest >> gw >> flags;
      if (dest == "00000000" && (std::strtoul(flags.c_str(), nullptr, 16) & 3) == 3) { up = true; break; }
    }
    netUp_ = up;
  }
  if (!netUp_) {
    if (wifiDownSinceMs_ == 0) wifiDownSinceMs_ = now;
    if (!configActive_ && wifiAlertCount_ < 3 && now - bootMs >= 15000) {
      const uint64_t down = now - wifiDownSinceMs_;
      const bool due = (wifiAlertCount_ == 0 && down >= 1000) || (wifiAlertCount_ > 0 && down >= (uint64_t)wifiAlertCount_ * 20000ULL);
      if (due) { noWifiPending_ = true; wifiAlertCount_++; }
    }
  } else {
    wifiDownSinceMs_ = 0;
    wifiAlertCount_ = 0;
  }
  // une demande en attente s'affiche des que rien d'important ne joue (hors playlist : tout de suite ; en playlist : entre deux GIF, voir tick())
  if (noWifiPending_ && mode_ != Mode::PLAYLIST && !configActive_) showNoWifiAlert();
  if (discPending_ && mode_ != Mode::PLAYLIST && !configActive_) showDisconnectedAlert();
  const bool still = mode_ == Mode::PNG && pngDrawn_ && currentPngPath_ == DEFAULT_RAW565_PATH && !configActive_;
  if (noWifiActive_ && still) {  // clignotement (400 ms) puis, au bout de 5 s, reprise de la playlist
    if (now - noWifiBlinkMs_ > 400) { noWifiBlinkMs_ = now; noWifiVisible_ = !noWifiVisible_; drawOverlayFrame(noWifiVisible_, 1); d_.show(); }
    if (now >= noWifiUntilMs_) { noWifiActive_ = false; logf("WIFI", "No wifi, No Recalbox -- delai ecoule, reprise playlist"); resumePlaylist(); d_.show(); }
  }
  if (discActive_ && still) {
    if (now - discBlinkMs_ > 400) { discBlinkMs_ = now; discVisible_ = !discVisible_; drawOverlayFrame(discVisible_, 2); d_.show(); }
    if (now >= discUntilMs_) {
      discActive_ = false;
      discForced_ = true;  // ce passage en playlist est force par la coupure : resynchronisation au prochain PONG
      logf("CMD", "RecalBox non connectee -- delai ecoule, reprise playlist");
      resumePlaylist();
      d_.show();
    }
  }
}

void Engine::onRecalboxConnected() {
  // l'ecran appartient a la page web / au flux natif / a un jeu en cours : rien a afficher
  if (configActive_ || mode_ == Mode::ZEDMD || mode_ == Mode::CONFIG || mode_ == Mode::SCORE || inGame_.load()) return;
  if (!ensureDefaultRaw565Cached()) return;
  if (mode_ == Mode::CLOCK) abortDeco();
  closeGifAll();
  gifOpened_ = false;
  displayedMaskSys_.clear();
  d_.clearScreen();
  mode_ = Mode::PNG;
  currentPngPath_ = DEFAULT_RAW565_PATH;  // non vide : sinon le cas PNG de tick() repasse en noir
  pngDrawn_ = true;                       // le message reste affiche : pas de redessin
  connectedVisible_ = true;
  drawConnectedFrame(true);
  connectedBlinkMs_ = nowMs();
  connectedOverlayUntilMs_ = nowMs() + 5000;
  d_.show();
  logf("CMD", "Recalbox connectee -> message 5 s sur l'image de secours, puis logo seul");
}

bool Engine::drawPng(const std::string &path) {
  // 1) rapide : le .raw565 pre-converti
  const std::string raw = pngToRaw565Path(path);
  if (drawRaw565(raw)) return true;
  logf("PNG-RAW", "MISSING raw565 path=%s", path.c_str());

  // 2) le systeme est-il « lent » pour ce bucket ?
  const std::string sys = sysNameFromSystemsPath(path);
  size_t ls = path.rfind('/');
  const std::string fname = (ls == std::string::npos) ? path : path.substr(ls + 1);
  const char bucket = bucketLetterForFilename(fname);
  const char slow = sys_.bucketSlowFlag(sys, bucket);
  const bool isSlow = isSlowFlag(slow);
  logf("PNG-RAW", "raw absent -> sys=%s bucket=%c slowFlag=%c isSlow=%d", sys.c_str(), bucket, slow, isSlow ? 1 : 0);

  // 3) systeme lent : jamais de decodage PNG, image de secours immediate
  if (isSlow) {
    if (drawDefaultRaw565Cached()) {
      logf("PNG-RAW", "FALLBACK default.raw565 (systeme lent) pour %s", path.c_str());
      return true;
    }
    return false;
  }

  // 4) systeme rapide : on decode le .png (comme pngle dans le firmware : pixels tels quels, alpha ignore, aucune mise a
  //    l'echelle, ce qui depasse 128x32 est rogne). Absent ou illisible -> image de secours.
  std::vector<uint8_t> file;
  std::vector<uint8_t> rgba;
  int pw = 0, ph = 0;
  // (le firmware n'ouvre que le chemin plat ; ici le sous-dossier alphabetique est essaye d'abord, comme pour les .raw565)
  const std::string subPng = alphaSubdirPath(path);
  const bool found = (fs_.exists(subPng) && fs_.readAll(subPng, file)) || fs_.readAll(path, file);
  if (!found) {
    if (drawDefaultRaw565Cached()) {
      logf("PNG-RAW", "FALLBACK default.raw565 (png absent) pour %s", path.c_str());
      return true;
    }
    return false;
  }
  const uint64_t t0 = nowMs();
  if (!decodePngRgba(file, rgba, pw, ph)) {
    logf("PNG-RAW", "decodage PNG echoue (%s) -> image de secours", path.c_str());
    return drawDefaultRaw565Cached();
  }
  const int cw = pw < DMD_W ? pw : DMD_W, chh = ph < DMD_H ? ph : DMD_H;
  for (int y = 0; y < chh; y++)
    for (int x = 0; x < cw; x++) {
      const uint8_t *q = &rgba[((size_t)y * pw + x) * 4];
      d_.drawPixel((int16_t)x, (int16_t)y, Display::color565(q[0], q[1], q[2]));
    }
  logf("PNG-RAW", "png decode OK %s (%dx%d%s) en %llu ms", path.c_str(), pw, ph, (pw > DMD_W || ph > DMD_H) ? ", rogne" : "",
       (unsigned long long)(nowMs() - t0));
  return true;
}

void Engine::closeGifAll() {
  pack_.close();
  stdgif_.close();
  rawMode_ = false;
  packIdx_ = 0;
}

bool Engine::openGif(const std::string &path, bool clearBefore, bool skipProbe, bool skipRawPack) {
  if (!skipProbe) {
    if (pref_.pick(fs_, alphaSubdirPath(path), path).empty()) return false;
  }
  closeGifAll();
  gifOpened_ = false;
  const bool isDefaults = path.find("/systems/_defaults/") != std::string::npos;

  if (!skipRawPack) {
    if (clearBefore) d_.clearScreen();
    if (pack_.open(fs_, pref_, path)) {
      rawMode_ = true;
      packIdx_ = 0;
      gifOpened_ = true;
      frameDueMs_ = 0;  // une nouvelle animation demarre tout de suite (le firmware interrompt l'attente de l'ancienne)
      return true;
    }
    pack_.close();
  }

  // masques _defaults : raw uniquement, pas de repli GIF standard
  if (isDefaults) {
    logf("GIF", "open FAIL (mask _defaults raw-only) req=%s -- raw565pack/meta introuvable", path.c_str());
    return false;
  }

  if (clearBefore) d_.clearScreen();
  if (stdgif_.open(fs_, path)) {
    rawMode_ = false;
    gifOpened_ = true;
    frameDueMs_ = 0;
    return true;
  }
  rawMode_ = false;
  gifOpened_ = false;
  return false;
}

bool Engine::playFrame(bool first, int *delayMs) {
  if (rawMode_) {
    if (first) packIdx_ = 0;
    if (packIdx_ >= pack_.frameCount()) return false;
    uint16_t buf[DMD_W * DMD_H];
    *delayMs = (int)pack_.delayMs(packIdx_);
    if (pack_.readFrame(packIdx_, buf)) d_.drawRGBBitmap(0, 0, buf, DMD_W, DMD_H);
    packIdx_++;
    return true;
  }
  return stdgif_.play(first, delayMs, d_);
}

void Engine::gifReset() {
  if (rawMode_) packIdx_ = 0;
  else stdgif_.reset();
}

Mode Engine::openBestMedia(const std::string &basePath, const std::string &systemPath) {
  const bool isDefault = endsWith(basePath, "/_default");
  if (!isDefault) {
    const std::string path = basePath + ".png";
    if (path == currentPngPath_ && pngDrawn_) return Mode::PNG;
    d_.clearScreen();
    if (drawPng(path)) {
      currentPngPath_ = path;
      pngDrawn_ = true;
      return Mode::PNG;
    }
    if (openGif(basePath + ".gif", true, true)) {
      pngDrawn_ = false;
      currentPngPath_.clear();
      return Mode::GIF;
    }
  } else {
    const std::string sys = sysNameOfDefaultPath(basePath);
    // logo du THEME Recalbox d'abord (fichier .raw565 seul : drawRaw565() renvoie false s'il est absent, SANS repli sur default.raw565,
    // contrairement a drawPng()) ; ordre : variante de langue, de region, base
    for (const std::string &cand : themes_.candidates(sys)) {
      d_.clearScreen();
      const bool ok = drawRaw565(cand);
      logf("THEME", "%s %s", cand.c_str(), ok ? "OK" : "absent");
      if (ok) {
        currentPngPath_ = cand.substr(0, cand.size() - 7) + ".png";
        pngDrawn_ = true;
        return Mode::PNG;
      }
    }
    if (drawPng(defaultPath(sys) + ".png")) {
      currentPngPath_ = defaultPath(sys) + ".png";
      pngDrawn_ = true;
      return Mode::PNG;
    }
  }

  if (!systemPath.empty()) {
    const std::string sys = sysNameOfDefaultPath(systemPath);
    const char t = sys_.defaultType(sys);
    const std::string path = defaultPath(sys) + ".png";
    auto openDG = [&]() { return openGif(defaultPath(sys) + ".gif", true, true); };
    if (t == 'p') {
      if (path == currentPngPath_ && pngDrawn_) return Mode::PNG;
      d_.clearScreen();
      if (drawPng(path)) {
        currentPngPath_ = path;
        pngDrawn_ = true;
        return Mode::PNG;
      }
      if (openDG()) {
        pngDrawn_ = false;
        currentPngPath_.clear();
        return Mode::GIF;
      }
    } else {  // 'g', 'B' ou inconnu : raw565pack d'abord
      if (path == currentPngPath_ && pngDrawn_) return Mode::PNG;
      if (openDG()) {
        pngDrawn_ = false;
        currentPngPath_.clear();
        return Mode::GIF;
      }
      d_.clearScreen();
      if (drawPng(path)) {
        currentPngPath_ = path;
        pngDrawn_ = true;
        return Mode::PNG;
      }
    }
  }

  // repli final : image de secours en RAM
  stdgif_.close();
  pack_.close();
  gifOpened_ = false;
  pngDrawn_ = true;
  currentPngPath_.clear();
  d_.clearScreen();
  if (drawDefaultRaw565Cached()) return Mode::PNG;

  const char defType = sys_.defaultType("default");
  if (defType != 'p' && openGif(defaultPath("default") + ".gif", true, true)) {
    pngDrawn_ = false;
    currentPngPath_.clear();
    return Mode::GIF;
  }
  if (defType != 'g') {
    const std::string path = defaultPath("default") + ".png";
    d_.clearScreen();
    if (drawPng(path)) {
      currentPngPath_ = path;
      pngDrawn_ = true;
      return Mode::PNG;
    }
  }
  if (!pngDrawn_ && !gifOpened_) {
    d_.clearScreen();
    currentPngPath_.clear();
  }
  return Mode::BLACK;
}

// ------------------------------------------------------------------ playlist

void Engine::openNextGif() {
  const std::string next = playlist_.next(fs_);
  const bool ok = !next.empty() && openGif(next, false, true, true);
  if (!ok) {
    if (!next.empty()) logf("PLAYLIST", "ouverture impossible : %s", next.c_str());
    gifOpened_ = false;
    mode_ = Mode::BLACK;
    d_.clearScreen();
    return;
  }
  mode_ = Mode::PLAYLIST;
  frameDueMs_ = 0;
}

void Engine::resumePlaylist() {
  closeGifAll();
  gifOpened_ = false;
  currentPngPath_.clear();
  pngDrawn_ = false;
  displayedMaskSys_.clear();
  if (playlist_.count(fs_) > 0) {
    mode_ = Mode::PLAYLIST;
    openNextGif();
  } else {
    mode_ = Mode::BLACK;
    d_.clearScreen();
  }
}

// ------------------------------------------------------------------ commandes

void Engine::handle(const std::string &cmd, const std::string &arg) {
  if (diagActive_.load()) diagStop();  // une instruction de la Recalbox (jeu, systeme...) reprend l'ecran
  // le flux ZeDMD possede l'ecran : seules les commandes d'administration passent (une commande d'affichage dessinerait par-dessus)
  if (mode_ == Mode::ZEDMD && cmd != "reboot" && cmd != "wifi_recovery" && cmd != "show_config" && cmd != "mem_probe") {
    logf("CMD", "%s ignoree (le flux ZeDMD possede l'ecran)", cmd.c_str());
    return;
  }
  // une commande en attente interrompt l'horloge (le firmware la verifie a chaque image) : la sequence s'arrete, la commande s'applique
  // ecran de configuration (page web) : les commandes d'affichage sont ignorees tant que « Reprendre DMD » n'est pas demande
  if (configActive_ && mode_ == Mode::CONFIG && (cmd == "game" || cmd == "system" || cmd == "default" || cmd == "stop" || cmd == "score" ||
                                                  cmd == "startclip" || cmd == "resumesys")) {
    logf("CMD", "%s ignoree (page de configuration ouverte)", cmd.c_str());
    return;
  }
  if (mode_ == Mode::CLOCK && cmd != "ingame" && cmd != "clock_preview") abortDeco();
  if (cmd == "rbw_feed") {  // outil de test : donnees de widgets Recalbox sans passer par le reseau
    widgets_.feedRbw(arg);
    return;
  }
  if (cmd == "widget_preview") {  // outil de test : « <id>[,<secondes>[,<style d'ambiance>]] », bloquant ; jamais envoye par le reseau
    int id = 0, sec = 0, style = -1;
    std::sscanf(arg.c_str(), "%d,%d,%d", &id, &sec, &style);
    if (sec > 0) widgets_.setDuration(id, sec);
    if (style >= 0) widgets_.setAmbStyle(style);
    d_.clearScreen();
    widgets_.run(id, true);
    d_.clearScreen();
    d_.show();
    mode_ = Mode::BLACK;
    return;
  }
  if (cmd == "clock_preview") {  // outil de test, jamais envoye par le reseau (UdpServer ne le transmet pas)
    startClockPreview(arg);
    return;
  }
  if (cmd == "stop" || cmd == "default" || cmd == "system" || cmd == "resumesys" || cmd == "game" || cmd == "score" || cmd == "startclip") clearAlerts();  // un vrai contenu reprend la main
  if (cmd == "stop") cmdStop();
  else if (cmd == "default") cmdDefault();
  else if (cmd == "system" || cmd == "resumesys") cmdSystem(arg);
  else if (cmd == "game") cmdGame(arg);
  else if (cmd == "startclip") {
    logf("CMD", "startgameclip -> playlist");
    resumePlaylist();
  } else if (cmd == "ingame") {
    inGame_ = (arg == "1");
    logf("CMD", "ingame=%d", inGame_.load() ? 1 : 0);
    return;  // simple drapeau d'etat, aucun affichage
  } else if (cmd == "brightness") {
    // pourcentage 0..100, en RAM seulement (la page web « Sauvegarder » reste seule a ecrire config.ini)
    const int pct = std::atoi(arg.c_str());  // toInt() d'Arduino : texte invalide = 0
    if (pct >= 0 && pct <= 100) {
      cfg_.screenBrightness = pct * 255 / 100;  // map(pct,0,100,0,255)
      d_.setBrightness8(cfg_.brightness8());
      logf("CMD", "brightness -> %d%%", pct);
    } else {
      logf("CMD", "brightness ignoree (valeur hors 0-100: %s)", arg.c_str());
    }
  } else if (cmd == "brightness_up" || cmd == "brightness_down") {
    // +/-10 %, PERSISTE dans config.ini seulement si la valeur change reellement
    const int curPct = (int)std::lround(cfg_.screenBrightness * 100.0 / 255.0);
    const int delta = (cmd == "brightness_up") ? 10 : -10;
    const int newPct = std::max(0, std::min(100, curPct + delta));
    if (newPct != curPct) {
      cfg_.screenBrightness = newPct * 255 / 100;
      d_.setBrightness8(cfg_.brightness8());
      cfg_.writeKeys(fs_, {{"brightness", std::to_string(newPct)}});
      logf("CMD", "brightness %+d%% -> %d%% (sauvegarde config.ini)", delta, newPct);
    } else {
      logf("CMD", "brightness deja au %s (%d%%)", newPct == 0 ? "minimum" : "maximum", newPct);
    }
  } else if (cmd == "reboot") {
    logf("CMD", "reboot demande : le service va redemarrer");
    restart_ = true;
    return;
  } else if (cmd == "wifi_recovery") {
    logf("CMD", "wifi_recovery : point d'acces de secours");
    if (wifiRecoveryHook_) wifiRecoveryHook_();
    return;
  } else if (cmd == "show_config" || cmd == "mem_probe") {
    logf("CMD", "%s : sans objet sur le Raspberry Pi", cmd.c_str());
    return;
  } else if (cmd == "score") {
    cmdScore(arg);
  } else {
    logf("CMD", "commande inconnue ou non portee : %s", cmd.c_str());
    return;
  }
  d_.show();
}

// CMD=score : ecran plein cadre pendant 6 s (ou « @<ms>| » en tete : 500-15000 ms), puis retour AUTOMATIQUE au mode d'avant
// (minuterie locale : aucun message de la Recalbox n'est necessaire pour revenir). Ne touche pas a l'animation en dessous.
void Engine::cmdScore(const std::string &arg) {
  if (mode_ != Mode::SCORE) modeBeforeScore_ = mode_;  // jamais MODE_SCORE lui-meme (un 2e score pendant le 1er)
  std::string payload = arg;
  long dur = SCORE_DISPLAY_DURATION_MS;
  if (payload.size() > 1 && payload[0] == '@') {
    const size_t sep = payload.find('|');
    if (sep != std::string::npos && sep > 1) {
      const long parsed = std::atol(payload.substr(1, sep - 1).c_str());
      if (parsed >= SCORE_DURATION_OVERRIDE_MIN_MS && parsed <= SCORE_DURATION_OVERRIDE_MAX_MS) {
        dur = parsed;
        payload = payload.substr(sep + 1);
      }  // hors bornes : prefixe ignore, comportement par defaut (le texte « @... » reste dans le payload, comme le firmware)
    }
  }
  drawScoreScreen(d_, payload);
  mode_ = Mode::SCORE;
  scoreUntilMs_ = nowMs() + (uint64_t)dur;
  logf("CMD", "score -> affiche %.1fs puis retour auto (mode precedent : %s)", dur / 1000.0, modeName(modeBeforeScore_));
}

void Engine::cmdStop() {
  if (mode_ == Mode::PLAYLIST) {
    logf("CMD", "stop ignored");
    return;
  }
  closeGifAll();
  gifOpened_ = false;
  currentPngPath_.clear();
  pngDrawn_ = false;
  mode_ = Mode::BLACK;
  d_.clearScreen();
}

void Engine::cmdDefault() {
  // un « default » alors que la Recalbox annonce etre EN JEU (ingame=1) est perime : ignore (v122)
  if (inGame_.load()) {
    logf("CMD", "default ignore (RB toujours en jeu selon ingame=1)");
    return;
  }
  resumePlaylist();
}

void Engine::cmdSystem(const std::string &sys) {
  lastSys_ = sys;
  lastGameArg_.clear();
  closeGifAll();
  gifOpened_ = false;
  pngDrawn_ = false;
  currentPngPath_.clear();
  mode_ = Mode::BLACK;
  mode_ = openBestMedia("/systems/" + sys + "/_default");
  displayedMaskSys_ = sys;
}

void Engine::cmdGame(const std::string &arg) {
  if (arg == "!SHUFFLE") {  // coupe-circuit anti-rafale : animation dediee en boucle
    if (openGif(SHUFFLE_GIF_PATH, true, true)) {
      pngDrawn_ = false;
      currentPngPath_.clear();
      mode_ = Mode::GIF;
      logf("CMD", "game -> shuffle (anti-rafale)");
    }
    return;
  }
  const size_t slash = arg.find('/');
  const std::string sys = (slash != std::string::npos) ? arg.substr(0, slash) : arg;
  const std::string rom = (slash != std::string::npos) ? arg.substr(slash + 1) : arg;
  const std::string sysBase = "/systems/" + sys + "/_default";
  const std::string gameBase = "/systems/" + arg;
  lastGameArg_ = arg;

  const char bucket = bucketLetterForFilename(rom);
  const char slow = sys_.bucketSlowFlag(sys, bucket);
  const bool isSlow = isSlowFlag(slow);
  logf("CMD_GAME", "sys=%s rom=%s bucket=%c slowFlag=%c isSlow=%d", sys.c_str(), rom.c_str(), bucket, slow, isSlow ? 1 : 0);

  // ---------------------------------------------------------------- chemin RAPIDE
  if (!isSlow) {
    const std::string gamePng = gameBase + ".png", gameGif = gameBase + ".gif";
    const char sysT = sys_.defaultType(sys);
    const char cachedFast = games_.find(fs_, sys, rom);
    const bool skipToDefault = (cachedFast == '?');  // jeu absent de l'index : directement le repli
    logf("CMD_GAME", "fast sysT=%c cache=%c", sysT, cachedFast);

    d_.clearScreen();
    if (!skipToDefault) {
      if (sysT == 'B' && openGif(gameGif, false, true)) {  // B : raw565pack d'abord
        pngDrawn_ = false;
        currentPngPath_.clear();
        mode_ = Mode::GIF;
        return;
      }
      if (drawPng(gamePng)) {
        pngDrawn_ = true;
        currentPngPath_ = gamePng;
        mode_ = Mode::PNG;
        return;
      }
      if (openGif(gameGif, false, true)) {
        pngDrawn_ = false;
        currentPngPath_.clear();
        mode_ = Mode::GIF;
        return;
      }
    }
    const std::string defPng = "/systems/_defaults/default.png";
    d_.clearScreen();
    if (drawPng(defPng)) {
      pngDrawn_ = true;
      currentPngPath_ = defPng;
      mode_ = Mode::PNG;
      return;
    }
    logf("CMD_GAME", "fast path totalement echoue (jeu+gif+default.png)");
    mode_ = Mode::BLACK;
    currentPngPath_.clear();
    pngDrawn_ = false;
    d_.clearScreen();
    return;
  }

  // ---------------------------------------------------------------- chemin LENT (gros dossiers)
  closeGifAll();
  gifOpened_ = false;
  pngDrawn_ = false;
  currentPngPath_.clear();

  const bool needDrawMask = (displayedMaskSys_ != sys);
  bool maskDrawn = false;
  if (needDrawMask) {  // 1) mask d'attente (laisse a l'ecran pendant le chargement)
    const std::string maskBase = "/systems/_defaults/" + sys;
    const char maskType = sys_.defaultType(sys);
    if (maskType == 'p') {
      const std::string maskPng = maskBase + ".png";
      d_.clearScreen();
      if (drawPng(maskPng)) {
        currentPngPath_ = maskPng;
        pngDrawn_ = true;
        mode_ = Mode::PNG;
        maskDrawn = true;
      } else {
        int fd = 0;
        if (openGif(maskBase + ".gif", true, true)) {
          playFrame(true, &fd);
          mode_ = Mode::GIF;
          pngDrawn_ = false;
          currentPngPath_.clear();
          maskDrawn = true;
        } else {
          mode_ = Mode::BLACK;
        }
      }
    } else {  // le mask est toujours un raw565 (jamais raw565pack)
      if (drawRaw565(maskBase + ".raw565")) {
        mode_ = Mode::BLACK;  // on garde juste le contenu affiche du mask
        pngDrawn_ = false;
        currentPngPath_.clear();
        maskDrawn = true;
      } else {
        mode_ = Mode::BLACK;
      }
    }
    displayedMaskSys_ = maskDrawn ? sys : "";
  }

  // 2) le jeu est-il dans l'index ?
  char cached = games_.find(fs_, sys, rom);
  const char sysT = sys_.defaultType(sys);
  logf("CMD_GAME", "slow cache=%c sysT=%c", cached, sysT);

  if (cached == '?') {  // jamais de .gif lent sur un dossier de 800+ fichiers : repli RAM direct
    if (drawDefaultRaw565Cached()) {
      displayedMaskSys_ = sys;  // pour que le mode PNG n'efface pas l'ecran
      pngDrawn_ = true;
      currentPngPath_.clear();
      mode_ = Mode::BLACK;
      return;
    }
  }
  if (sysT == 'g' || sysT == 'B') cached = 'g';  // le type du systeme force le type d'affichage
  else if (sysT == 'p') cached = 'p';

  if (cached == 'p') {
    const std::string path = gameBase + ".png";
    currentPngPath_ = path;
    if (displayedMaskSys_.empty()) d_.clearScreen();
    const bool okDraw = drawPng(path);
    if (okDraw) {
      pngDrawn_ = true;
      displayedMaskSys_.clear();
      mode_ = Mode::PNG;
    } else {
      pngDrawn_ = false;
      int fd = 0;
      if (openGif(gameBase + ".gif", false, true)) {
        playFrame(true, &fd);
        displayedMaskSys_.clear();
        mode_ = Mode::GIF;
      } else {
        mode_ = Mode::BLACK;
        d_.clearScreen();
        displayedMaskSys_.clear();
      }
      currentPngPath_.clear();
    }
    return;
  }
  if (cached == 'g') {
    if (openGif(gameBase + ".gif", false, true)) {  // le mask reste visible jusqu'a la 1re image
      mode_ = Mode::GIF;
      displayedMaskSys_.clear();
      return;
    }
    const std::string rawPath = gameBase + ".raw565";  // pack echoue : le raw565 du jeu
    if (drawRaw565(rawPath)) {
      pngDrawn_ = true;
      currentPngPath_ = rawPath;
      mode_ = Mode::PNG;
      return;
    }
    if (drawDefaultRaw565Cached()) {
      pngDrawn_ = true;
      currentPngPath_.clear();
      displayedMaskSys_.clear();
      mode_ = Mode::PNG;
      return;
    }
  }

  // 3) dernier essai : le .png direct, puis openBestMedia
  {
    const std::string pngPath = gameBase + ".png";
    if (drawPng(pngPath)) {
      currentPngPath_ = pngPath;
      pngDrawn_ = true;
      mode_ = Mode::PNG;
      displayedMaskSys_.clear();
      return;
    }
  }
  mode_ = openBestMedia(gameBase, sysBase);
}

// ------------------------------------------------------------------ boucle

// ------------------------------------------------------------------ planificateur de luminosite (BRIGHT_SCHED_ON / BRIGHT_SCHED)
void Engine::loadBrightSched() {
  bs_ = parseBrightSched(cfg_.get("bright_sched"));
  bsOn_ = cfg_.getBool("bright_sched_on", false) && !bs_.empty();
  bsLastSlot_ = -2;
  bsLastMs_ = 0;
  if (!bsOn_ && cfg_.schedBrightness.load() >= 0) {  // planificateur coupe : retour a la luminosite de base
    cfg_.schedBrightness = -1;
    d_.setBrightness8(cfg_.brightness8());
    if (mode_ != Mode::ZEDMD) d_.show();
  }
  logf("BRIGHT", "planificateur %s (%d plage(s))", bsOn_ ? "actif" : "inactif", (int)bs_.size());
}

void Engine::tickBrightSched(uint64_t now) {
  if (!bsOn_) return;
  if (bsLastMs_ != 0 && now - bsLastMs_ < 15000) return;
  bsLastMs_ = now ? now : 1;
  const time_t t = time(nullptr);
  struct tm lt;
  localtime_r(&t, &lt);
  if (lt.tm_year <= 100) return;  // heure inconnue (NTP pas encore synchronise)
  const int slot = brightSlotAt(bs_, lt.tm_hour * 60 + lt.tm_min);
  if (slot < 0 || slot == bsLastSlot_) return;
  bsLastSlot_ = slot;
  const int pct = bs_[(size_t)slot].second;
  cfg_.schedBrightness = (pct * 255 + 50) / 100;
  d_.setBrightness8((uint8_t)cfg_.schedBrightness.load());
  if (mode_ != Mode::ZEDMD) d_.show();
  logf("BRIGHT", "plage %d/%d : %d %%%s", slot + 1, (int)bs_.size(), pct, pct == 0 ? " (ecran eteint)" : "");
}

void Engine::tick() {
  diagTick(nowMs());
  tickBrightSched(nowMs());
  tickWebWatchdog(nowMs());
  if (zed_.enabled()) {
    switch (zed_.poll(nowMs())) {
      case ZeDmd::Event::ENTERED:  // le 1er datagramme ZeDMD valide prend l'ecran : tout le reste est suspendu
        if (mode_ != Mode::ZEDMD) {
          abortDeco();
          clock_.stop();
          closeGifAll();
          currentPngPath_.clear();
          pngDrawn_ = false;
          displayedMaskSys_.clear();
          mode_ = Mode::ZEDMD;
        }
        break;
      case ZeDmd::Event::EXIT_RECALBOX:  // vpinball_end : la playlist reprend (comme la boucle du firmware, qui n'avait fait que se suspendre) ;
        resumePlaylist();                  // ingame 0 / system, envoyes ensuite par marquee.sh, affichent le logo du systeme
        d_.show();
        break;
      case ZeDmd::Event::EXIT_RESUME:  // silence / fin de fenetre RA : playlist + resynchronisation (webDmdResume)
        resumePlaylist();
        d_.show();
        helloRequested_ = true;
        break;
      case ZeDmd::Event::NONE:
        break;
    }
  }
  if (diagActive_.load()) return;  // le diagnostic garde l'ecran : ni alertes, ni playlist, ni horloge
  if (tctl_.version.load() != themeSeen_) applyThemeControl();
  pumpRbw();  // donnees des widgets Recalbox recues par UDP
  if (mode_ == Mode::ZEDMD) {
    // mode natif : un message de la page web ne suspend le flux d'ES que 15 s (nativeOverlayMs du firmware), puis l'image d'ES revient
    noWifiPending_ = discPending_ = noWifiActive_ = discActive_ = false;  // pas d'alerte par-dessus le flux natif / Pinball
    if (configActive_ && zed_.native()) tickConfigScroll(nowMs());
    if (configActive_ && zed_.native() && ((cfgSubSetMs_ != 0 && nowMs() - cfgSubSetMs_ > 15000) || (infoUntilMs_ != 0 && nowMs() >= infoUntilMs_))) {
      infoUntilMs_ = 0;
      configActive_ = false;
      zed_.setOverlay(false);
    }
    return;
  }
  // « RecalBox connectee » : 5 s, puis le logo reste SEUL jusqu'a la prochaine instruction (plus de message permanent)
  if (connectedOverlayUntilMs_ != 0) {
    const bool still = mode_ == Mode::PNG && pngDrawn_ && currentPngPath_ == DEFAULT_RAW565_PATH;
    if (!still) connectedOverlayUntilMs_ = 0;  // une instruction a pris la main
    else if (nowMs() >= connectedOverlayUntilMs_) {
      connectedOverlayUntilMs_ = 0;
      drawConnectedFrame(false);               // fin des 5 s : le logo reste SEUL, quel que soit l'etat du clignotement
      d_.show();
    } else if (nowMs() - connectedBlinkMs_ >= 400) {  // le message clignote pendant les 5 s
      connectedBlinkMs_ = nowMs();
      connectedVisible_ = !connectedVisible_;
      drawConnectedFrame(connectedVisible_);
      d_.show();
    }
  }
  tickAlerts(nowMs());
  if (requestNextGif_) {
    requestNextGif_ = false;
    openNextGif();
    d_.show();
  }
  const uint64_t now = nowMs();
  switch (mode_) {
    case Mode::PLAYLIST: {
      if (!gifOpened_) {
        d_.clearScreen();
        mode_ = Mode::BLACK;
        d_.show();
        break;
      }
      if (now < frameDueMs_) break;  // delai de l'image en cours
      int fd = 0;
      const bool frameOk = playFrame(false, &fd);
      d_.show();
      if (!frameOk) {  // animation terminee : la suivante (sans attendre le delai de la derniere image)
        // alertes en attente : affichees ENTRE deux GIF (jamais au milieu d'une animation), puis la playlist reprend a l'expiration
        if (noWifiPending_ && !configActive_) { showNoWifiAlert(); break; }
        if (discPending_ && !configActive_) { showDisconnectedAlert(); break; }
        // cycle du decor : declencheur = nombre de GIF OU temps ecoule, evalue ENTRE deux GIF (jamais au milieu d'une animation)
        const bool decoOn = decoAnyActive();
        const bool trigTime = (deco_.trig == 1 && deco_.minutes > 0);
        if (decoOn && !trigTime) clockGifCounter_++;
        openNextGif();
        d_.show();
        if (decoOn) {
          const int gifEvery = (deco_.gifs < 1) ? 10 : deco_.gifs;
          const bool fire = trigTime ? (now - decoLastMs_ >= (uint64_t)deco_.minutes * 60000ULL) : (clockGifCounter_ >= gifEvery);
          if (fire) {
            clockGifCounter_ = 0;
            decoLastMs_ = now;
            modeAfterDeco_ = mode_;  // le GIF suivant est deja ouvert : on y retourne apres la sequence
            startDecoSequence(now);
          }
        }
        break;
      }
      if (fd <= 0) fd = 10;
      frameDueMs_ = now + (uint64_t)fd;
      break;
    }
    case Mode::GIF: {
      if (!gifOpened_) {
        d_.clearScreen();
        mode_ = Mode::BLACK;
        d_.show();
        break;
      }
      if (now < frameDueMs_) break;
      int fd = 0;
      const bool frameOk = playFrame(false, &fd);
      d_.show();
      if (!frameOk) {  // boucle sur l'animation du jeu
        gifReset();
        break;
      }
      if (fd <= 0) fd = 10;
      frameDueMs_ = now + (uint64_t)fd;
      break;
    }
    case Mode::PNG:
      if (currentPngPath_.empty()) {
        if (displayedMaskSys_.empty()) d_.clearScreen();  // un mask doit rester visible
        mode_ = Mode::BLACK;
        d_.show();
        break;
      }
      if (!pngDrawn_) {
        drawPng(currentPngPath_);
        pngDrawn_ = true;
        d_.show();
      }
      break;
    case Mode::CLOCK:
      if (!clock_.tick(now)) advanceDeco(now);  // session terminee : element suivant de la sequence, ou fin
      break;
    case Mode::SCORE:
      if (now >= scoreUntilMs_) {
        mode_ = modeBeforeScore_;
        // MODE_PNG saute son dessin si pngDrawn_ : sans ce reset l'ecran resterait fige sur le score
        if (mode_ == Mode::PNG) pngDrawn_ = false;
        logf("CMD", "score expire -> retour au jeu (mode=%s)", modeName(mode_));
      }
      break;
    case Mode::CONFIG:  // page de configuration ouverte : rien ne bouge tant que « Reprendre DMD » n'est pas demande
      // un message transitoire (« Enregistrement... ») expire apres 5 s : retour a l'adresse de la page (comme le firmware, SD_OP_SUBMSG_EXPIRE_MS)
      tickConfigScroll(now);
      if (infoUntilMs_ != 0 && now >= infoUntilMs_) {  // ecran d'information du bouton : retour a l'affichage normal
        infoUntilMs_ = 0;
        webResume();
        break;
      }
      if (cfgSubSetMs_ != 0 && !cfgPersistentSub_.empty() && cfgSubMsg_ != cfgPersistentSub_ && now - cfgSubSetMs_ > 5000) {
        cfgSubMsg_ = cfgPersistentSub_;
        cfgSubColor_ = cfgPersistentColor_;
        cfgSubWhiteFrom_ = cfgPersistentWhite_;
        cfgSubSetMs_ = now;
        logf("WEB", "message transitoire expire : retour a l'adresse de la page");
        drawConfigScreen();
      }
      break;
    case Mode::BLACK:
    default:
      // fond « widgets seuls » : aucun GIF, la sequence du decor s'enchaine en continu (garde-fou de 600 ms) tant qu'aucun jeu n'occupe l'ecran
      if (deco_.widgetsOnly && decoAnyActive() && !pngDrawn_ && !configActive_ && now - widgetsOnlyMs_ >= 600) {
        widgetsOnlyMs_ = now;
        modeAfterDeco_ = Mode::BLACK;
        startDecoSequence(now);
        break;
      }
      // retente la playlist toutes les 3 s si rien n'est vraiment affiche (v92) ; pngDrawn = contenu reellement a l'ecran
      if (!deco_.widgetsOnly && !pngDrawn_ && now - lastBlackRetryMs_ >= 3000) {
        lastBlackRetryMs_ = now;
        requestNextGif_ = true;
      }
      break;
  }
}

// ------------------------------------------------------------------ mode diagnostic Wi-Fi / liaison Recalbox (portage du firmware v391-v393)
// Une mesure par seconde : signal (iw) + 1 PING vers la Recalbox active ; le PONG est consomme par le thread UDP (UdpServer::pongCount()).
// Affichage : 4 pages de 4 s sur l'ecran de configuration (ligne 1 jaune, ligne 2 coloree selon l'etat). Journal des evenements : seulement
// pendant le diagnostic (le demon ne surveille pas le Wi-Fi en permanence, contrairement a l'ESP32).
static std::string diagJsonEsc(const std::string &s) {
  std::string o = "\"";
  for (char ch : s) {
    if (ch == '"' || ch == '\\') {
      o += '\\';
      o += ch;
    } else if ((unsigned char)ch >= 0x20) {
      o += ch;
    }
  }
  return o + "\"";
}

void Engine::diagWebTouch() {
  diagWebSeenMs_ = nowMs();
  webTouch();
}

void Engine::diagShow(const std::string &l1, const std::string &l2, uint16_t color) {
  cfgMainMsg_ = l1;
  cfgSubMsg_ = l2;
  cfgSubColor_ = color;
  cfgSubWhiteFrom_ = -1;
  cfgSubSetMs_ = 0;
  configActive_ = true;
  drawConfigScreen();
}

void Engine::diagStart(bool fromWeb) {
  if (diagActive_.load()) return;
  diagFromWeb_ = fromWeb;
  diagWebSeenMs_ = nowMs();
  diag_.reset();
  diagPingOut_ = false;
  diagLastTickMs_ = 0;
  diagHaveLink_ = false;
  diagPrevUp_ = false;
  diagPrevBssid_.clear();
  diagPageAtMs_ = nowMs();
  diagShown1_.clear();
  diagShown2_.clear();
  diagMemMin_ = -1;
  if (!fromWeb || !configActive_) {  // lance par le bouton : l'ecran passe en « pause » ; lance par la page : l'ecran de configuration est deja la
    cfgMainMsg_ = L3(cfg_.uiLanguage, "DIAGNOSTIC", "DIAGNOSTIC", "DIAGNOSTICO");
    cfgPersistentSub_.clear();
    webPause(L3(cfg_.uiLanguage, "MESURE...", "MEASURING...", "MIDIENDO..."), 0x07FF);
  }
  diagActive_ = true;
  logf("DIAG", "mode diagnostic en boucle (%s)", fromWeb ? "page web" : "bouton");
}

void Engine::diagStop() {
  if (!diagActive_.exchange(false)) return;
  const DiagStats::Calc c = diag_.calc();
  logf("DIAG", "sortie : fenetre=%d/%d perte=%d%% min=%d avg=%d max=%d gigue=%d reconnexions=%lu", c.got, c.n, c.lossPct, c.mn, c.avg, c.mx, c.jit, diag_.disc());
  if (diagFromWeb_) {  // la page de configuration reste ouverte : on redessine son ecran
    cfgMainMsg_ = "WEB DMD CONFIG";
    cfgSubMsg_ = cfgPersistentSub_;
    cfgSubColor_ = cfgPersistentColor_;
    cfgSubWhiteFrom_ = cfgPersistentWhite_;
    cfgSubSetMs_ = 0;
    if (configActive_) drawConfigScreen();
  } else {
    webResume();
  }
}

void Engine::diagTick(uint64_t now) {
  if (!diagActive_.load()) return;
  if (diagFromWeb_) {
    if (now - diagWebSeenMs_.load() > 8000) {
      logf("DIAG", "page web silencieuse > 8 s : sortie");
      diagStop();
      return;
    }
  } else {
    webTouch();  // pas de reprise automatique au bout de 3 min pendant un diagnostic lance par le bouton
  }
  // 1) reponse du PING en cours (800 ms au plus)
  if (diagPingOut_) {
    if (diagHooks_.pongCount && diagHooks_.pongCount() != diagPongBase_) {
      const uint64_t arr = diagHooks_.lastPongMs ? diagHooks_.lastPongMs() : now;
      diag_.push(diagRssi_, (int)(arr >= diagSentMs_ ? arr - diagSentMs_ : 0));
      diagPingOut_ = false;
    } else if (now - diagSentMs_ >= 800) {
      diag_.push(diagRssi_, -1);
      diagPingOut_ = false;
    }
  }
  // 2) nouvelle mesure toutes les secondes
  if (!diagPingOut_ && (diagLastTickMs_ == 0 || now - diagLastTickMs_ >= 1000)) {
    diagLastTickMs_ = now;
    const WifiLink w = wifiLink();
    const NetInfo ni = netInfo();
    const long mem = memAvailableBytes();
    if (diagHaveLink_) {
      if (diagPrevUp_ && !w.up) diag_.event('D', "1", now / 1000);
      else if (w.up && (!diagPrevUp_ || w.bssid != diagPrevBssid_)) diag_.event('C', w.bssid, now / 1000);
    } else if (w.up) {
      diag_.event('C', w.bssid, now / 1000);
    }
    diagHaveLink_ = true;
    diagPrevUp_ = w.up;
    diagPrevBssid_ = w.bssid;
    {
      std::lock_guard<std::mutex> lk(diagMu_);
      diagLink_ = w;
      diagNet_ = ni;
      diagMem_ = mem;
      if (mem >= 0 && (diagMemMin_ < 0 || mem < diagMemMin_)) diagMemMin_ = mem;
    }
    diagRssi_ = w.up ? w.rssi : -127;
    const std::string rb = diagHooks_.rbIp ? diagHooks_.rbIp() : std::string();
    if (!ni.ip.empty() && !rb.empty() && diagHooks_.sendPing) {
      diagPongBase_ = diagHooks_.pongCount ? diagHooks_.pongCount() : 0;
      diagSentMs_ = now;
      diagHooks_.sendPing();
      diagPingOut_ = true;
    } else {
      diag_.push(diagRssi_, -2);
    }
  }
  // 3) page affichee (4 s chacune), redessinee seulement quand son texte change
  const uint16_t GREEN = 0x07E0, YEL = 0xFFE0, RED = 0xF800, CYAN = 0x07FF;
  const int page = (int)(((now - diagPageAtMs_) / 4000) % 4);
  WifiLink w;
  NetInfo ni;
  {
    std::lock_guard<std::mutex> lk(diagMu_);
    w = diagLink_;
    ni = diagNet_;
  }
  const std::string rb = diagHooks_.rbIp ? diagHooks_.rbIp() : std::string();
  const DiagStats::Calc c = diag_.calc();
  std::string l1, l2;
  uint16_t col = CYAN;
  const std::string &dl = cfg_.uiLanguage;   // v3.75 : langue du DMD (fr / en / es)
  const uint16_t lc = (c.n == 0) ? CYAN : (c.lossPct == 0 && c.avg < 80) ? GREEN : (c.lossPct <= 10 && c.avg < 200) ? YEL : RED;
  if (page == 3) {
    l1 = std::string(L3(dl, "RECONNEXIONS: ", "RECONNECTIONS: ", "RECONEXIONES: ")) + std::to_string(diag_.disc());
    if (diag_.disc() == 0) {
      l2 = L3(dl, "AUCUNE DEPUIS DIAG", "NONE SINCE DIAG", "NINGUNA DESDE DIAG");
      col = GREEN;
    } else {
      l2 = std::string(L3(dl, "DERNIERE IL Y A ", "LAST ONE ", "ULTIMA HACE ")) + std::to_string((unsigned long)(now / 1000 - diag_.discAtS())) + (dl == "en" ? "s AGO" : "s");
      col = YEL;
    }
  } else if (page == 0) {
    if (w.up) {
      const char *ql = w.rssi >= -55 ? L3(dl, "EXCELLENT", "EXCELLENT", "EXCELENTE") : w.rssi >= -67 ? L3(dl, "BON", "GOOD", "BUENA") : w.rssi >= -75 ? L3(dl, "MOYEN", "FAIR", "MEDIA") : w.rssi >= -82 ? L3(dl, "FAIBLE", "WEAK", "DEBIL") : L3(dl, "TRES FAIBLE", "VERY WEAK", "MUY DEBIL");
      l1 = "WIFI " + std::to_string(w.rssi) + " dBm";
      l2 = std::string(ql) + " CH" + std::to_string(w.channel);
      col = w.rssi >= -67 ? GREEN : w.rssi >= -75 ? YEL : RED;
    } else if (!ni.ip.empty()) {
      l1 = "ETHERNET";
      l2 = L3(dl, "PAS DE WIFI", "NO WIFI", "SIN WIFI");
      col = GREEN;
    } else {
      l1 = L3(dl, "RESEAU NON CONNECTE", "NETWORK NOT CONNECTED", "RED NO CONECTADA");
      l2 = L3(dl, "PAS DE SIGNAL", "NO SIGNAL", "SIN SENAL");
      col = RED;
    }
  } else if (rb.empty()) {
    l1 = "RECALBOX";
    l2 = L3(dl, "IP INCONNUE", "UNKNOWN IP", "IP DESCONOCIDA");
    col = RED;
  } else if (c.n == 0) {
    l1 = L3(dl, "TEST RECALBOX...", "TESTING RECALBOX...", "PROBANDO RECALBOX...");
    l2 = rb;
  } else if (page == 1) {
    l1 = "RB " + rb;
    l2 = c.got == 0 ? std::string(L3(dl, "AUCUNE REPONSE", "NO ANSWER", "SIN RESPUESTA")) : std::to_string(c.got) + "/" + std::to_string(c.n) + " PONG " + std::to_string(c.avg) + "ms";
    col = c.got == 0 ? RED : lc;
  } else {
    l1 = std::string(L3(dl, "PERTE ", "LOSS ", "PERDIDA ")) + std::to_string(c.lossPct) + "% MAX " + std::to_string(c.mx) + "ms";
    l2 = std::string(L3(dl, "GIGUE ", "JITTER ", "JITTER ")) + std::to_string(c.jit) + std::string("ms ") + L3(dl, "COUP ", "DROPS ", "CORTES ") + std::to_string(diagHooks_.gapCount ? (unsigned long)diagHooks_.gapCount() : 0UL);
    col = lc;
  }
  if (l1 != diagShown1_ || l2 != diagShown2_ || col != diagShownColor_) {
    diagShown1_ = l1;
    diagShown2_ = l2;
    diagShownColor_ = col;
    diagShow(l1, l2, col);
  }
}

std::string Engine::diagJson() {
  const DiagStats::Calc c = diag_.calc();
  std::string lat, rs;
  std::vector<DiagStats::Ev> ev;
  unsigned long disc = 0;
  uint64_t discAt = 0;
  diag_.arrays(lat, rs, ev, disc, discAt);
  WifiLink w;
  NetInfo ni;
  long mem, memMin;
  {
    std::lock_guard<std::mutex> lk(diagMu_);
    w = diagLink_;
    ni = diagNet_;
    mem = diagMem_;
    memMin = diagMemMin_;
  }
  const uint64_t now = nowMs();
  const std::string rb = diagHooks_.rbIp ? diagHooks_.rbIp() : std::string();
  const uint64_t lastPkt = diagHooks_.lastPacketMs ? diagHooks_.lastPacketMs() : 0;
  std::string o = "{\"on\":";
  o += diagActive_.load() ? "1" : "0";
  o += ",\"up\":" + std::string(w.up ? "1" : "0");
  o += ",\"ssid\":" + diagJsonEsc(w.ssid) + ",\"bssid\":" + diagJsonEsc(w.bssid);
  o += ",\"ch\":" + std::to_string(w.up ? w.channel : 0) + ",\"rssi\":" + std::to_string(w.up ? w.rssi : -127);
  o += ",\"ip\":" + diagJsonEsc(ni.ip) + ",\"gw\":" + diagJsonEsc(ni.gw) + ",\"rb\":" + diagJsonEsc(rb);
  o += ",\"n\":" + std::to_string(c.n) + ",\"got\":" + std::to_string(c.got) + ",\"loss\":" + std::to_string(c.lossPct);
  o += ",\"min\":" + std::to_string(c.mn) + ",\"avg\":" + std::to_string(c.avg) + ",\"max\":" + std::to_string(c.mx) + ",\"jit\":" + std::to_string(c.jit);
  o += ",\"age\":" + std::to_string(lastPkt ? (long)(now - lastPkt) : -1L);
  o += ",\"gaps\":" + std::to_string(diagHooks_.gapCount ? (unsigned long)diagHooks_.gapCount() : 0UL);
  o += ",\"pongs\":" + std::to_string(diagHooks_.pongCount ? (unsigned long)diagHooks_.pongCount() : 0UL);
  o += ",\"heap\":" + std::to_string(mem < 0 ? 0 : mem) + ",\"minheap\":" + std::to_string(memMin < 0 ? 0 : memMin);
  o += ",\"up_s\":" + std::to_string((unsigned long)(now / 1000));
  o += ",\"disc\":" + std::to_string(disc) + ",\"dr\":" + std::to_string(disc ? 1 : 0) + ",\"da\":" + std::to_string((unsigned long)discAt);
  o += ",\"pi\":1,\"eth\":" + std::string(!w.up && !ni.ip.empty() ? "1" : "0");
  o += ",\"ev\":[";
  for (size_t i = 0; i < ev.size(); i++) {
    if (i) o += ',';
    o += "[\"" + std::string(1, ev[i].type) + "\"," + std::to_string((unsigned long)ev[i].atS) + "," + diagJsonEsc(ev[i].data) + "]";
  }
  o += "],\"lat\":[" + lat + "],\"rs\":[" + rs + "]}";
  return o;
}
