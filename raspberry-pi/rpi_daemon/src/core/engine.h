// engine.h -- coeur « marquee » : portage de la logique de RecalBox_DMD.ino (processPendingMqttCommand, openBestMedia,
// drawPng/drawRaw565, openGif, playlist, et les cases MODE_* de loop()). Les gardes de tas ESP32 disparaissent ; la
// structure de decision (rapide / lent, mask, replis) est conservee a l'identique pour obtenir les memes affichages.
//
// Mono-thread : le reseau poste les commandes (voir CommandQueue, jalon 2b), tick() les applique puis avance
// l'animation. tick() ne bloque jamais : l'attente du delai d'une image est un simple test d'horloge.
#pragma once
#include <atomic>
#include <cstdint>
#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "brightsched.h"
#include "clock.h"
#include "config.h"
#include "display.h"
#include "gamescache.h"
#include "pack.h"
#include "paths.h"
#include "playlist.h"
#include "sdfs.h"
#include "stdgif.h"
#include "syscache.h"
#include "widgets.h"
#include "themecontrol.h"
#include "themelogos.h"
#include "zedcontrol.h"
#include "zedmd.h"

enum class Mode { PLAYLIST, GIF, PNG, BLACK, CONFIG, SCORE, CLOCK, ZEDMD };
const char *modeName(Mode m);

class Engine {
 public:
  Engine(SdFs &fs, Display &d, Config &cfg) : fs_(fs), d_(d), cfg_(cfg), zed_(d, cfg, fs, zctl_) {}

  // Charge config/caches/playlist. N'affiche rien.
  void begin();
  // Demarre la lecture (equivalent de resumePlaylist() au boot).
  void start();

  // Commande « CMD=<cmd> ARG=<arg> » (cmd en minuscules) : stop, default, system, game, resumesys, startclip, ingame.
  void handle(const std::string &cmd, const std::string &arg);
  // Une iteration de loop() : avance l'animation si le delai de l'image est ecoule.
  void tick();

  Mode mode() const { return mode_; }
  bool inGame() const { return inGame_.load(); }
  bool configScreenActive() const { return configActive_; }  // ecran de configuration / point d'acces affiche (boucle principale seulement)
  std::atomic<bool> &inGameFlag() { return inGame_; }  // positionne tout de suite par le thread reseau (CMD=ingame)
  bool restartRequested() const { return restart_; }   // CMD=reboot : le service doit redemarrer
  void setWifiRecoveryHook(std::function<void()> fn) { wifiRecoveryHook_ = std::move(fn); }  // CMD=wifi_recovery / bouton : point d'acces de secours
  // horloge / decor : reglages lus dans [CLOCK] et [WIDGETS] de config.ini
  struct Deco {
    uint8_t mode[11] = {0};  // par identifiant : 0 desactive, 1 « a la place » (creneau principal), 2 « en plus »
    uint8_t dur[11] = {60, 10, 15, 10, 60, 15, 8, 1, 15, 30, 15};
    int trig = 0;        // 0 = tous les N GIF (clock_interval), 1 = toutes les M minutes (clock_interval_min)
    int mainOrder = 0;   // 0 aleatoire, 1 tourniquet
    int addMode = 0;     // 0 tous les widgets « en plus » dans l'ordre, 1 addCount tires au hasard
    int addCount = 1;
    int gifs = 10;
    int minutes = 0;
    bool widgetsOnly = false;  // WIDGET_BG = 1 : aucun GIF, les widgets s'enchainent en continu
  };
  const Deco &deco() const { return deco_; }
  ThemeControl &themeControl() { return tctl_; }       // theme / langue / region annonces par la Recalbox (thread reseau -> boucle)
  ZedControl &zedControl() { return zctl_; }           // drapeaux poses a la reception par le thread reseau
  ZeDmd &zedmd() { return zed_; }                      // flux ZeDMD-WiFi (port 3333) : Pinball, mode natif, fenetre RA
  // pendant un widget (bloquant) : commande en attente (= interrompre) et entretien du reseau (HELLO / PING)
  void setWidgetHooks(std::function<bool()> commandPending, std::function<void()> service) { widgets_.setHooks(std::move(commandPending), std::move(service)); }
  // donnees de widgets Recalbox (CMD=rbw) recues : analysees tout de suite, y compris pendant un widget bloquant
  void pumpRbw() { for (const std::string &r : tctl_.takeRbw()) widgets_.feedRbw(r); }
  // --- page web (jalon 6) : les gestionnaires HTTP tournent dans d'autres threads ; tout ce qui touche l'affichage ou l'etat du moteur
  // passe par runOnMain() (execute dans la boucle principale, attend la fin, 8 s au plus)
  bool runOnMain(std::function<void()> fn);
  void postMain(std::function<void()> fn);                // idem sans attendre (apercus de widgets, longs)
  void pumpMain();
  bool mainTaskPending();
  void loadBrightSched();                                  // BRIGHT_SCHED* relus (demarrage, rechargement de config.ini)
  void reloadConfig();                                    // config.ini vient d'etre modifie : relit et applique en direct
  void webPause(const std::string &msg, uint16_t color);  // « ecran de configuration » : titre + message (page web ouverte / enregistrement)
  void webResume();                                       // « Reprendre DMD » : retour a l'affichage normal
  void webOpen(const std::string &msg, const std::string &ip);
  int widgetsRestoreQuotes(const std::string &lang) { return widgets_.restoreQuotes(lang); }
  // ecrans « configuration » propres au demon (premier demarrage, point d'acces de secours) : persistants, avec defilement
  void configScreen(const std::string &main, const std::string &sub, uint16_t subColor, int whiteFrom = -1);
  void configMainLine(const std::string &main);           // ligne 1 seule (compte a rebours) : ne remet pas le defilement de la ligne 2 a zero
  void configSubLine(const std::string &sub, uint16_t subColor, int whiteFrom);
  void buttonAction(int a);                               // bouton physique (action 1..11), appelee dans la boucle principale (jalon 7)
  void setReloadHook(std::function<void()> fn) { reloadHook_ = std::move(fn); }  // appelee apres chaque reloadConfig() (le bouton relit BTN_*)
  void onRecalboxConnected();                             // 1re reponse de la Recalbox : image de secours + « RecalBox connectee » 5 s, puis le logo seul jusqu'a une instruction
  void showFirstBootScreen();                             // « Page de configuration » + adresse (ou attente du reseau) : premier demarrage
  void setConfigTitle(const std::string &t) { cfgMainMsg_ = t.empty() ? std::string("WEB DMD CONFIG") : t; }  // titre (ligne 1) de l'ecran de configuration, ex. « INSTALLATION »
  void webPage(const std::string &url);                   // une page est servie : « WEB DMD CONFIG » + adresse (triggerWebConfigModeSoft)
  void previewBrightness(int pct);                        // apercu live (RAM seulement)
  void clockPreview(const std::string &theme);            // « -1 »..« 14 » ou « stop »
  bool takeHelloRequest() { return helloRequested_.exchange(false); }  // reprise apres un flux ZeDMD : annoncer le DMD a la Recalbox
  int playlistCount() { return cfg_.playlistName.empty() ? 0 : playlist_.count(fs_); }
  bool pngDrawn() const { return pngDrawn_; }
  const std::string &currentPngPath() const { return currentPngPath_; }
  const std::string &displayedMask() const { return displayedMaskSys_; }

 private:
  // --- affichage de medias ---
  bool drawRaw565(const std::string &rawPath);
  bool drawDefaultRaw565Cached();
  bool ensureDefaultRaw565Cached();
  bool drawPng(const std::string &path);  // raw565 d'abord, puis replis (voir drawPng() du firmware)
  bool openGif(const std::string &path, bool clearBefore = true, bool skipProbe = false, bool skipRawPack = false);
  void closeGifAll();
  bool playFrame(bool first, int *delayMs);
  void gifReset();
  Mode openBestMedia(const std::string &basePath, const std::string &systemPath = "");

  // --- playlist ---
  void openNextGif();
  void resumePlaylist();

  // --- horloge et decor (sequence « horloge puis widgets ») ---
  void loadDecoConfig();
  bool decoAnyActive() const;
  void startDecoSequence(uint64_t now);  // constitue la file (creneau principal + widgets « en plus ») et lance le 1er element
  void advanceDeco(uint64_t now);        // element suivant, ou fin de sequence (retour au mode d'avant)
  void abortDeco();                      // une commande arrive : la sequence s'arrete
  void startClockPreview(const std::string &arg);  // outil de test (jamais appele par le reseau)
  void applyThemeControl();                         // CMD=theme / themeopt recus : applique, memorise dans config.ini, redessine le logo a l'ecran

  // --- commandes ---
  void cmdStop();
  void cmdScore(const std::string &arg);
  void cmdDefault();
  void cmdSystem(const std::string &sys);
  void cmdGame(const std::string &arg);

  SdFs &fs_;
  Display &d_;
  Config &cfg_;
  SysCache sys_;
  GamesCache games_;
  Playlist playlist_;
  SubdirPref pref_;

  Mode mode_ = Mode::BLACK;
  Mode modeBeforeScore_ = Mode::BLACK;
  uint64_t scoreUntilMs_ = 0;

  Deco deco_;
  Clock clock_;
  Widgets widgets_;
  std::vector<int> decoQueue_;
  Mode modeAfterDeco_ = Mode::BLACK;
  int clockGifCounter_ = 0;
  uint64_t widgetsOnlyMs_ = 0;
  uint64_t decoLastMs_ = 0;
  int lastMainId_ = -1, lastRandMain_ = -1;
  bool pngDrawn_ = false;
  std::string currentPngPath_;
  std::string displayedMaskSys_;
  std::atomic<bool> inGame_{false};
  bool restart_ = false;
  std::function<void()> wifiRecoveryHook_, reloadHook_;
  BrightSlots bs_;                                         // planificateur de luminosite : plages triees
  bool bsOn_ = false;
  int bsLastSlot_ = -2;
  uint64_t bsLastMs_ = 0;
  void tickBrightSched(uint64_t now);
  bool screenOff_ = false;                                 // action 5 du bouton : ecran eteint (luminosite 0), rien n'est ecrit dans config.ini
  uint64_t infoUntilMs_ = 0;                               // action 6 : ecran d'information affiche jusqu'a cette date
  std::mutex mainMu_;
  std::deque<std::pair<std::function<void()>, std::shared_ptr<std::promise<void>>>> mainTasks_;
  bool configActive_ = false;                              // ecran de configuration affiche (page web)
  std::string cfgMainMsg_ = "WEB DMD CONFIG", cfgSubMsg_;
  uint16_t cfgSubColor_ = 0xFFFF;
  std::string cfgPersistentSub_;                           // adresse de la page (reste affichee) ; les messages transitoires expirent apres 5 s
  uint16_t cfgPersistentColor_ = 0xFFE0;
  uint64_t cfgSubSetMs_ = 0;
  void drawConfigScreen();                                 // redessin complet (defilements remis a zero)
  void drawSubAt(int x);                                   // ligne 2, en deux couleurs (prefixe / valeur blanche) a partir de cfgSubWhiteFrom_
  void tickConfigScroll(uint64_t now);                     // defilement horizontal des lignes plus larges que 128 px (comme loop() du firmware)
  int cfgSubWhiteFrom_ = -1, cfgPersistentWhite_ = -1;                               // index de debut de la valeur (SSID / adresse) affichee en blanc, -1 = tout d'une couleur
  int scroll1_ = 0, scroll2_ = 0;
  uint64_t lastScroll1Ms_ = 0, lastScroll2Ms_ = 0, subPauseUntilMs_ = 0;
  ZedControl zctl_;
  ZeDmd zed_;
  ThemeControl tctl_;
  ThemeLogos themes_;
  uint32_t themeSeen_ = 0;
  std::string lastSys_;   // lastSysName : dernier « system » recu (pour redessiner son logo quand le theme change)
  std::atomic<bool> helloRequested_{false};
  uint64_t connectedOverlayUntilMs_ = 0;                  // fin du message « RecalBox connectee » (0 = pas de message)
  uint64_t connectedBlinkMs_ = 0;                         // dernier clignotement du message (toutes les 400 ms)
  bool connectedVisible_ = true;
  void drawConnectedFrame(bool visible);                  // image de secours (assombrie + message si visible, sinon l'image seule)

  // animation courante
  bool gifOpened_ = false;
  bool rawMode_ = false;  // raw565pack (sinon GIF standard)
  Raw565Pack pack_;
  StdGif stdgif_;
  uint32_t packIdx_ = 0;
  uint64_t frameDueMs_ = 0;
  bool requestNextGif_ = false;
  uint64_t lastBlackRetryMs_ = 0;

  // image de secours en RAM
  std::vector<uint16_t> defaultBuf_;
  bool defaultCached_ = false;
  int64_t defaultMtime_ = -1;
};
