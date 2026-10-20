// dmd_daemon -- le demon RecalBoxDMD pour Raspberry Pi : remplace l'ESP32.
//   dmd_daemon --sd /home/pi/dmd/sd [--udp-port 5005] [--hello-port 5006] [--no-panel] [--led-* ...]
// Boucle principale (equivalent de loop()) : applique les commandes recues par UDP, avance l'animation, entretient la
// presence (HELLO/PING). Le thread reseau ne fait que poster dans la file de commandes.
#include <unistd.h>

#include <csignal>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

#include "backend_png.h"
#include "commandqueue.h"
#include "config.h"
#include "display.h"
#include "engine.h"
#include "log.h"
#include "sdfs.h"
#include "udpserver.h"
#include "wifimanager.h"
#include "button.h"
#include "webconfig.h"
#include "webserver.h"
#ifdef WITH_HZELLER
#include "backend_hzeller.h"
#endif

namespace {
volatile std::sig_atomic_t g_stop = 0;
void onSignal(int) { g_stop = 1; }

// Backend « rien » pour les essais sans dalles
class NullBackend : public Backend {
 public:
  bool begin() override { return true; }
  void present(const uint16_t *, uint8_t) override {}
};

constexpr uint64_t OFFLINE_FALLBACK_MS = 60000;  // MQTT_OFFLINE_FALLBACK_MS : 60 s sans paquet -> reprise de la playlist
}  // namespace

int main(int argc, char **argv) {
  std::string exePath = "/proc/self/exe";  // chemin REEL : execv("/proc/self/exe") donnerait au processus le nom « exe » (pgrep / pkill ne le verraient plus)
  {
    char eb[512];
    const ssize_t en = readlink("/proc/self/exe", eb, sizeof(eb) - 1);
    if (en > 0) exePath.assign(eb, (size_t)en);
  }
  const std::vector<std::string> origArgs(argv, argv + argc);  // pour se relancer (CMD=reboot / « Enregistrer et redemarrer ») : hzeller retire ses options de argv
  std::string sd = "/home/pi/dmd/sd";
  int udpPort = 5005, helloPort = 5006, zedPort = 3333, webPort = 80;
  std::string webDir;  // pages de la configuration web (tools/port_web_assets.py) ; defaut : ../web a cote de l'executable
  bool noPanel = false;
  bool manageWifi = false;  // --wifi : garde-fous reseau (NetworkManager, point d'acces de configuration) ; le service systemd le passe, un lancement a la main non
  bool rxTrace = false, noSchedTune = false;
  std::string pngOut;  // --png-out FICHIER : sans dalles, ecrit l'image courante (zoom 4) a chaque affichage (tests)

#ifdef WITH_HZELLER
  rgb_matrix::RGBMatrix::Options opt = HzellerBackend::defaultOptions();
  rgb_matrix::RuntimeOptions rt = HzellerBackend::defaultRuntime();
  bool cliSlowdown = false;
  for (int i = 1; i < argc; i++)
    if (!std::strncmp(argv[i], "--led-slowdown-gpio", 19)) cliSlowdown = true;
  if (!rgb_matrix::ParseOptionsFromFlags(&argc, &argv, &opt, &rt)) return 2;
#endif
  for (int i = 1; i < argc; i++) {
    auto next = [&]() -> const char * {
      if (i + 1 >= argc) {
        std::fprintf(stderr, "%s attend une valeur\n", argv[i]);
        std::exit(2);
      }
      return argv[++i];
    };
    if (!std::strcmp(argv[i], "--sd")) sd = next();
    else if (!std::strcmp(argv[i], "--udp-port")) udpPort = std::atoi(next());
    else if (!std::strcmp(argv[i], "--hello-port")) helloPort = std::atoi(next());
    else if (!std::strcmp(argv[i], "--zedmd-port")) zedPort = std::atoi(next());
    else if (!std::strcmp(argv[i], "--web-port")) webPort = std::atoi(next());
    else if (!std::strcmp(argv[i], "--web-dir")) webDir = next();
    else if (!std::strcmp(argv[i], "--no-panel")) noPanel = true;
    else if (!std::strcmp(argv[i], "--wifi")) manageWifi = true;
    else if (!std::strcmp(argv[i], "--rx-trace")) rxTrace = true;
    else if (!std::strcmp(argv[i], "--no-sched-tune")) noSchedTune = true;
    else if (!std::strcmp(argv[i], "--png-out")) { pngOut = next(); noPanel = true; }
    else {
      std::fprintf(stderr, "option inconnue : %s\n", argv[i]);
      return 2;
    }
  }

  std::signal(SIGINT, onSignal);
  std::signal(SIGTERM, onSignal);

  SdFs fs(sd);
  Config cfg;
  cfg.load(fs);

  Backend *backend = nullptr;
#ifdef WITH_HZELLER
  if (!noPanel) {
    // gpio_slowdown : option de ligne de commande (--led-slowdown-gpio) > config.ini (gpio_slowdown=1..4) > defaut du modele
    int cfgSlow = cfg.getInt("gpio_slowdown", 0);
    if (cfgSlow >= 1 && cfgSlow <= 4 && !cliSlowdown) rt.gpio_slowdown = cfgSlow;
    std::fprintf(stderr, "[HZ] carte : %s ; gpio_slowdown=%d (%s)\n", HzellerBackend::boardModel().c_str(), rt.gpio_slowdown,
                 cliSlowdown ? "ligne de commande" : (cfgSlow >= 1 && cfgSlow <= 4) ? "config.ini" : "auto selon le modele");
    auto *hb = new HzellerBackend(opt, rt);
    hb->setSchedTune(cfg.getBool("sched_tune", true) && !noSchedTune);  // evite que l'affichage fige le WiFi
    backend = hb;
  }
#else
  noPanel = true;
#endif
  if (!backend) backend = pngOut.empty() ? static_cast<Backend *>(new NullBackend()) : new PngBackend(pngOut, 4);
  Display display(backend);
  if (!display.begin()) {
    std::fprintf(stderr, "initialisation de l'affichage impossible\n");
    return 1;
  }

  Engine engine(fs, display, cfg);
  engine.begin();
  engine.start();

  CommandQueue queue;
  queue.configure(cfg.getBool("nav_paced", true), cfg.getInt("nav_dwell_ms", 60), cfg.getInt("nav_backlog", 5));
  logf("BOOT", "navigation : %s (nav_dwell_ms=%d nav_backlog=%d)", queue.paced() ? "cadencee, aucun logo saute sauf gros retard" : "comme le firmware (dernier paquet gagne)",
       cfg.getInt("nav_dwell_ms", 60), cfg.getInt("nav_backlog", 5));
  UdpServer udp(queue, cfg, engine.inGameFlag(), engine.zedControl(), engine.themeControl());
  // un widget (bloquant) s'arrete des qu'une commande attend ; pendant ce temps la presence UDP (HELLO / PING) est entretenue
  // (SIGTERM / SIGINT arretent aussi le widget en cours : sans cela l'arret attend la fin de toute la sequence)
  engine.setWidgetHooks([&queue, &engine] { return queue.pending() || g_stop || engine.mainTaskPending(); }, [&udp, &engine] {
    udp.tickPresence(nowMs());
    engine.pumpRbw();
  });
  udp.setRxTrace(rxTrace);
  if (!udp.start(udpPort, helloPort)) logf("BOOT", "ATTENTION : serveur UDP indisponible, le DMD ne recevra aucune commande");

  // flux ZeDMD-WiFi (UDP 3333) + decouverte HTTP /handshake (EmulationStation / Visual Pinball)
  engine.zedmd().start(zedPort);
  WebServer web;
  if (webDir.empty()) {  // <executable>/../../web (build/ -> rpi_daemon/web), sinon ./web
    char exe[512];
    const ssize_t n = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
    std::string base = n > 0 ? std::string(exe, (size_t)n) : std::string(".");
    base = base.substr(0, base.find_last_of('/'));
    webDir = base + "/../web";
  }
  WebConfig webcfg(engine, cfg, fs, udp, web, webDir);
  webcfg.registerRoutes();
  web.start(cfg.getInt("web_port", webPort));
  WifiManager wifi(engine, cfg, fs);
  if (manageWifi) {
    webcfg.setWifi(&wifi);
    engine.setWifiRecoveryHook([&wifi] { wifi.requestRecovery(); });
    wifi.start();
  }

  // bouton physique (GPIO21) : l'action est executee par la boucle principale ; BTN_* relus a chaque rechargement de config.ini
  Button button([&engine](int a) { engine.postMain([&engine, a] { engine.buttonAction(a); }); });
  button.configure(cfg);
  engine.setReloadHook([&button, &cfg] { button.configure(cfg); });
  webcfg.setButton(&button);
  button.start();

  logf("BOOT", "demon pret (carte = %s)", sd.c_str());
  uint64_t lastFallbackMs = 0;
  uint64_t prevLoopEnd = nowMs();
  while (!g_stop && !engine.restartRequested()) {
    const uint64_t loopStart = nowMs();
    if (loopStart - prevLoopEnd > 100) logf("PERF", "boucle principale non rendue pendant %llu ms", (unsigned long long)(loopStart - prevLoopEnd));
    engine.pumpMain();  // taches de la page web (affichage, configuration)
    Cmd c;
    while (queue.take(c, loopStart)) {
      const uint64_t t0 = nowMs();
      engine.handle(c.cmd, c.arg);
      const uint64_t t1 = nowMs();
      // latence = temps entre la reception du paquet et la fin du dessin ; traitement = duree de handle() (SD + dessin + affichage)
      logf("PERF", "%s %s : attente %llu ms, traitement %llu ms", c.cmd.c_str(), c.arg.substr(0, 40).c_str(),
           (unsigned long long)(t0 - c.postedMs), (unsigned long long)(t1 - t0));
    }
    engine.tick();
    const uint64_t now = nowMs();
    udp.tickPresence(now);
    if (engine.takeHelloRequest()) udp.sendHello();  // fin d'un flux ZeDMD : la Recalbox renvoie l'etat (webDmdResume)
    if (udp.takeConnectedEvent()) engine.onRecalboxConnected();  // « RecalBox connectee » 5 s sur l'image de secours, puis le logo seul

    // filet : Recalbox muette depuis 60 s alors qu'on affiche autre chose que la playlist -> reprise de la playlist
    const uint64_t seen = udp.lastPacketMs();
    // (l'horloge fait partie de la playlist : dans le firmware elle tourne DANS MODE_PLAYLIST, le filet ne doit pas la couper)
    if (seen > 0 && now - seen >= OFFLINE_FALLBACK_MS && engine.mode() != Mode::PLAYLIST && engine.mode() != Mode::CLOCK && engine.mode() != Mode::ZEDMD &&
        now - lastFallbackMs >= 5000 &&
        engine.playlistCount() > 0) {
      lastFallbackMs = now;
      logf("UDP", "injoignable (%llus sans paquet) -> reprise playlist", (unsigned long long)(OFFLINE_FALLBACK_MS / 1000));
      queue.post("default", "");
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    prevLoopEnd = nowMs();
  }
  logf("BOOT", "arret (%s)", engine.restartRequested() ? "redemarrage demande" : "signal");
  button.stop();
  wifi.stop();
  web.stop();
  udp.stop();
  engine.zedmd().stop();
  const bool relaunch = engine.restartRequested();
  // image noire presentee AVANT de quitter. La destruction de la matrice hzeller (~RGBMatrix) peut rester bloquee a 100 % de CPU dans
  // WaitPulseFinished (constate le 19/10 : « Enregistrer et redemarrer » -> demon jamais relance, ecran vide) : en cas de relance on ne la
  // detruit PAS (exec remet tout a zero), sinon une alarme de 3 s tue le processus si elle ne rend pas la main.
  display.clearScreen();
  display.show();
  std::this_thread::sleep_for(std::chrono::milliseconds(60));
  if (!relaunch) {
    alarm(3);
    delete backend;  // efface les dalles
  }
  if (relaunch) {  // le demon se relance lui-meme (sans service systemd rien ne le ferait ; avec systemd le PID reste le meme, Restart= n'intervient pas)
    logf("BOOT", "relance du demon");
    for (int fd = 3; fd < 1024; fd++) close(fd);  // aucune socket heritee (le port 3333 / 5005 / 80 doit pouvoir etre relie)
    std::vector<char *> av;
    for (const std::string &a : origArgs) av.push_back(const_cast<char *>(a.c_str()));
    av.push_back(nullptr);
    execv(exePath.c_str(), av.data());
    logf("BOOT", "relance impossible (execv)");
  }
  return 0;
}
