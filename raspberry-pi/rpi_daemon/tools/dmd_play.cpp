// dmd_play -- rejoue des commandes du protocole (« CMD=game ARG=nes/zelda ») sur le moteur, sans reseau.
//
//   dmd_play --sd DIR [--start] [--no-panel] [--wait SEC] [--snap PREFIXE] --do "CMD=game ARG=nes/zelda" --do ...
//     --sd        dossier qui joue le role de la carte SD (config.ini, systems/, playlists/, ...)
//     --start     demarre d'abord la playlist (comme au boot)
//     --no-panel  n'utilise pas les dalles (aucun PNG/affichage materiel) : pour les tests de logique
//     --wait      secondes d'animation apres chaque commande (defaut 1.5)
//     --snap      ecrit PREFIXE_<n>.png (zoom 4) apres chaque commande
//     --live      ecrit en continu l'image courante dans ce PNG (au plus toutes les 150 ms) : echantillonnage des widgets / animations
//   les options --led-* de hzeller sont acceptees (defauts RPI2DMD).
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

#include "backend_png.h"
#include "cmdparse.h"
#include "config.h"
#include "display.h"
#include "engine.h"
#include "log.h"
#include "sdfs.h"
#ifdef WITH_HZELLER
#include "backend_hzeller.h"
#endif

namespace {
// Backend « tee » : garde la derniere image pour les captures, transmet au backend reel (ou rien).
class SnapBackend : public Backend {
 public:
  explicit SnapBackend(Backend *inner) : inner_(inner) {}
  void setLive(const std::string &path) { live_ = path; }
  bool begin() override { return inner_ ? inner_->begin() : true; }
  void present(const uint16_t *fb, uint8_t b) override {
    std::memcpy(last_, fb, sizeof(last_));
    if (inner_) inner_->present(fb, b);
    if (!live_.empty() && nowMs() - liveMs_ >= 150) {
      liveMs_ = nowMs();
      PngBackend png(live_, 4);
      png.present(last_, 255);
    }
  }
  void save(const std::string &path) {
    PngBackend png(path, 4);
    png.present(last_, 255);  // contenu du framebuffer, sans la luminosite (pour comparer aux .raw565)
  }

 private:
  Backend *inner_;
  std::string live_;
  uint64_t liveMs_ = 0;
  uint16_t last_[DMD_W * DMD_H] = {0};
};
}  // namespace

int main(int argc, char **argv) {
  std::string sd = "/home/pi/dmd/sd", snap, live;
  bool start = false, noPanel = false;
  double wait = 1.5;
  std::vector<std::string> cmds;

#ifdef WITH_HZELLER
  rgb_matrix::RGBMatrix::Options opt = HzellerBackend::defaultOptions();
  rgb_matrix::RuntimeOptions rt = HzellerBackend::defaultRuntime();
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
    else if (!std::strcmp(argv[i], "--start")) start = true;
    else if (!std::strcmp(argv[i], "--no-panel")) noPanel = true;
    else if (!std::strcmp(argv[i], "--wait")) wait = std::atof(next());
    else if (!std::strcmp(argv[i], "--snap")) snap = next();
    else if (!std::strcmp(argv[i], "--live")) live = next();
    else if (!std::strcmp(argv[i], "--do")) cmds.push_back(next());
    else {
      std::fprintf(stderr, "option inconnue : %s\n", argv[i]);
      return 2;
    }
  }

  SdFs fs(sd);
  Config cfg;
  cfg.load(fs);

  Backend *real = nullptr;
#ifdef WITH_HZELLER
  if (!noPanel) real = new HzellerBackend(opt, rt);
#else
  noPanel = true;
#endif
  SnapBackend backend(real);
  if (!live.empty()) backend.setLive(live);
  Display d(&backend);
  if (!d.begin()) {
    std::fprintf(stderr, "initialisation de l'affichage impossible\n");
    return 1;
  }
  Engine eng(fs, d, cfg);
  eng.begin();
  if (start) eng.start();

  auto run = [&](double sec) {
    const uint64_t end = nowMs() + (uint64_t)(sec * 1000);
    while (nowMs() < end) {
      eng.tick();
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  };

  if (start) run(wait);
  int n = 0;
  for (const auto &c : cmds) {
    ParsedCmd p = parseCommand(c);
    logf("PLAY", ">>> %s  (cmd=%s arg=%s)", c.c_str(), p.cmd.c_str(), p.arg.c_str());
    eng.handle(p.cmd, p.arg);
    run(wait);
    logf("PLAY", "mode=%s pngDrawn=%d mask=%s path=%s", modeName(eng.mode()), eng.pngDrawn() ? 1 : 0,
         eng.displayedMask().c_str(), eng.currentPngPath().c_str());
    if (!snap.empty()) backend.save(snap + "_" + std::to_string(++n) + ".png");
  }
  if (cmds.empty()) run(wait);
  return 0;
}
