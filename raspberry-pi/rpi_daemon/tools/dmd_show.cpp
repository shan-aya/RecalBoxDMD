// dmd_show -- outil de validation de la couche d'affichage (jalon 1).
//
//   dmd_show --file X.raw565 [--hold 20]            affiche une image raw565 sur les dalles
//   dmd_show --pattern grid|colors|text|animate|solid|vlines   mires de controle (orientation, couleurs, gamma,
//                                                   fluidite ; solid --color R,G,B ; animate --speed N --fps N)
//   dmd_show ... --png out.png [--zoom 4]           ecrit un PNG au lieu d'utiliser les dalles (tests sans materiel)
//   options : --brightness 0..255 (defaut 255)  --gamma 1.0  --hold SECONDES (defaut 15)
//   les options --led-* de hzeller sont acceptees (defauts = reglages RPI2DMD, voir backend_hzeller.cpp).
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

#include "TomThumb.h"
#include "backend_png.h"
#include "display.h"
#ifdef WITH_HZELLER
#include "backend_hzeller.h"
#endif

namespace {
bool loadRaw565(const std::string &path, std::vector<uint16_t> &out) {
  FILE *f = std::fopen(path.c_str(), "rb");
  if (!f) {
    std::fprintf(stderr, "ouverture impossible : %s\n", path.c_str());
    return false;
  }
  std::vector<uint8_t> buf(DMD_W * DMD_H * 2 + 1);
  size_t n = std::fread(buf.data(), 1, buf.size(), f);
  std::fclose(f);
  if (n != (size_t)DMD_W * DMD_H * 2) {
    std::fprintf(stderr, "%s : %zu octets, attendu %d\n", path.c_str(), n, DMD_W * DMD_H * 2);
    return false;
  }
  out.resize((size_t)DMD_W * DMD_H);
  for (size_t i = 0; i < out.size(); i++) out[i] = (uint16_t)(buf[2 * i] | (buf[2 * i + 1] << 8));  // LE
  return true;
}

void patternGrid(Display &d) {
  const uint16_t W = Display::color565(255, 255, 255), R = Display::color565(255, 0, 0),
                 G = Display::color565(0, 255, 0), B = Display::color565(0, 0, 255),
                 Y = Display::color565(255, 255, 0), M = Display::color565(255, 0, 255);
  d.clearScreen();
  d.drawRect(0, 0, DMD_W, DMD_H, W);
  d.drawFastVLine(63, 1, DMD_H - 2, Y);  // jonction entre les deux dalles
  d.drawFastVLine(64, 1, DMD_H - 2, Y);
  d.fillRect(2, 2, 4, 4, R);                       // haut gauche  = rouge
  d.fillRect(DMD_W - 6, 2, 4, 4, G);               // haut droite  = vert
  d.fillRect(2, DMD_H - 6, 4, 4, B);               // bas gauche   = bleu
  d.fillRect(DMD_W - 6, DMD_H - 6, 4, 4, W);       // bas droite   = blanc
  d.drawLine(0, 0, DMD_W - 1, DMD_H - 1, M);       // diagonales : controle du chainage
  d.drawLine(0, DMD_H - 1, DMD_W - 1, 0, M);
  d.setTextWrap(false);
  d.setTextSize(1);
  d.setTextColor(W, 0);
  d.setCursor(10, 12);
  d.print("LEFT");
  d.setCursor(DMD_W - 10 - 5 * 6, 12);
  d.print("RIGHT");
  d.setCursor(46, 12);
  d.print("128x32");
}

void patternColors(Display &d) {
  d.clearScreen();
  const uint8_t bars[8][3] = {{255, 255, 255}, {255, 255, 0}, {0, 255, 255}, {0, 255, 0},
                              {255, 0, 255},   {255, 0, 0},   {0, 0, 255},   {0, 0, 0}};
  for (int i = 0; i < 8; i++) d.fillRect(i * 16, 0, 16, 16, Display::color565(bars[i][0], bars[i][1], bars[i][2]));
  for (int x = 0; x < DMD_W; x++) {
    uint8_t v = (uint8_t)(x * 255 / (DMD_W - 1));
    d.drawFastVLine(x, 16, 4, Display::color565(v, 0, 0));
    d.drawFastVLine(x, 20, 4, Display::color565(0, v, 0));
    d.drawFastVLine(x, 24, 4, Display::color565(0, 0, v));
    d.drawFastVLine(x, 28, 4, Display::color565(v, v, v));
  }
}

// Couleur pleine : repere les LED mortes/eteintes (voir toujours au meme endroit physique = defaut de dalle).
void patternSolid(Display &d, int r, int g, int b) { d.fillScreen(Display::color565(r, g, b)); }

// Traits verticaux fixes d'1 pixel tous les 8 px, pleine hauteur, + traits horizontaux aux lignes 0/15/16/31 :
// un decalage d'une rangee entre moitie haute et moitie basse se voit comme une marche au milieu des traits.
void patternVlines(Display &d) {
  d.clearScreen();
  const uint16_t W = Display::color565(255, 255, 255);
  for (int x = 4; x < DMD_W; x += 8) d.drawFastVLine(x, 0, DMD_H, W);
  d.drawFastHLine(0, 15, DMD_W, Display::color565(255, 0, 0));
  d.drawFastHLine(0, 16, DMD_W, Display::color565(0, 255, 0));
}

void patternText(Display &d) {
  d.clearScreen();
  d.setTextWrap(false);
  d.setTextSize(1);
  d.setTextColor(Display::color565(255, 255, 255));
  d.setCursor(0, 0);
  d.print("Adafruit 5x7 ABC abc 0123");
  d.setTextSize(2);
  d.setTextColor(Display::color565(255, 200, 0));
  d.setCursor(0, 9);
  d.print("HI-SCORE");
  d.setTextSize(1);
  d.setFont(&TomThumb);
  d.setTextColor(Display::color565(90, 90, 255));
  d.setCursor(0, 31);
  d.print("TOMTHUMB 3X5 FONT 0123456789");
  d.setFont(nullptr);
}
}  // namespace

int main(int argc, char **argv) {
  std::string file, pattern, png;
  int zoom = 1, brightness = 255, speed = 2, fps = 60, cr = 255, cg = 255, cb = 255;
  double hold = 15.0;
  float gamma = 1.0f;

#ifdef WITH_HZELLER
  rgb_matrix::RGBMatrix::Options opt = HzellerBackend::defaultOptions();
  rgb_matrix::RuntimeOptions rt = HzellerBackend::defaultRuntime();
  if (!rgb_matrix::ParseOptionsFromFlags(&argc, &argv, &opt, &rt)) return 2;  // retire les --led-*
#endif

  for (int i = 1; i < argc; i++) {
    auto next = [&](const char *name) -> const char * {
      if (i + 1 >= argc) {
        std::fprintf(stderr, "%s attend une valeur\n", name);
        std::exit(2);
      }
      return argv[++i];
    };
    if (!std::strcmp(argv[i], "--file")) file = next("--file");
    else if (!std::strcmp(argv[i], "--pattern")) pattern = next("--pattern");
    else if (!std::strcmp(argv[i], "--png")) png = next("--png");
    else if (!std::strcmp(argv[i], "--zoom")) zoom = std::atoi(next("--zoom"));
    else if (!std::strcmp(argv[i], "--brightness")) brightness = std::atoi(next("--brightness"));
    else if (!std::strcmp(argv[i], "--gamma")) gamma = (float)std::atof(next("--gamma"));
    else if (!std::strcmp(argv[i], "--hold")) hold = std::atof(next("--hold"));
    else if (!std::strcmp(argv[i], "--speed")) speed = std::atoi(next("--speed"));  // animate : pixels par image
    else if (!std::strcmp(argv[i], "--fps")) fps = std::atoi(next("--fps"));        // animate : images par seconde
    else if (!std::strcmp(argv[i], "--color")) {                                     // solid : R,G,B
      if (std::sscanf(next("--color"), "%d,%d,%d", &cr, &cg, &cb) != 3) {
        std::fprintf(stderr, "--color attend R,G,B\n");
        return 2;
      }
    }
    else {
      std::fprintf(stderr, "option inconnue : %s\n", argv[i]);
      return 2;
    }
  }
  if (file.empty() && pattern.empty()) {
    std::fprintf(stderr, "usage : dmd_show (--file X.raw565 | --pattern grid|colors|text|animate) [--png out.png]\n");
    return 2;
  }

  std::vector<uint16_t> image;
  if (!file.empty() && !loadRaw565(file, image)) return 1;  // lu AVANT d'ouvrir les dalles

  Backend *backend = nullptr;
  if (!png.empty()) {
    backend = new PngBackend(png, zoom);
  } else {
#ifdef WITH_HZELLER
    backend = new HzellerBackend(opt, rt, gamma);
#else
    std::fprintf(stderr, "compile sans hzeller : utiliser --png\n");
    return 2;
#endif
  }
  (void)gamma;

  Display d(backend);
  if (!d.begin()) {
    std::fprintf(stderr, "initialisation de l'affichage impossible\n");
    return 1;
  }
  d.setBrightness8((uint8_t)(brightness < 0 ? 0 : brightness > 255 ? 255 : brightness));

  if (pattern == "quality") {
    // dessine dans la boucle ci-dessous
  } else if (!image.empty()) d.drawRGBBitmap(0, 0, image.data(), DMD_W, DMD_H);
  else if (pattern == "grid") patternGrid(d);
  else if (pattern == "colors") patternColors(d);
  else if (pattern == "text") patternText(d);
  else if (pattern == "solid") patternSolid(d, cr, cg, cb);
  else if (pattern == "vlines") patternVlines(d);
  else if (pattern != "animate") {
    std::fprintf(stderr, "mire inconnue : %s\n", pattern.c_str());
    return 2;
  }

  using clock = std::chrono::steady_clock;
  auto t0 = clock::now();
  if (pattern == "quality" && png.empty()) {
    // cycle de 20 s : gris uniforme 50 % | barres + degrades | image (--file) ou mire | defilement
    // (le gris uniforme et les degrades sombres revelent le mieux les variations de luminosite / le scintillement)
    int x = 0;
    while (std::chrono::duration<double>(clock::now() - t0).count() < hold) {
      const double t = std::fmod(std::chrono::duration<double>(clock::now() - t0).count(), 20.0);
      if (t < 5.0) patternSolid(d, 128, 128, 128);
      else if (t < 10.0) patternColors(d);
      else if (t < 15.0) {
        if (!image.empty()) d.drawRGBBitmap(0, 0, image.data(), DMD_W, DMD_H);
        else patternGrid(d);
      } else {
        d.clearScreen();
        d.fillRect(x, 0, 6, DMD_H, Display::color565(255, 255, 255));
        d.fillRect((x + 64) % DMD_W, 0, 6, DMD_H, Display::color565(255, 0, 0));
        x = (x + speed) % DMD_W;
      }
      d.show();
      std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    return 0;
  }
  if (pattern == "animate" && png.empty()) {
    // barre verticale qui balaie l'ecran a ~60 images/s : fluidite, dechirure, scintillement
    int x = 0;
    while (std::chrono::duration<double>(clock::now() - t0).count() < hold) {
      d.clearScreen();
      d.fillRect(x, 0, 6, DMD_H, Display::color565(255, 255, 255));
      d.fillRect((x + 64) % DMD_W, 0, 6, DMD_H, Display::color565(255, 0, 0));
      d.show();
      x = (x + speed) % DMD_W;
      std::this_thread::sleep_for(std::chrono::microseconds(1000000 / (fps > 0 ? fps : 60)));
    }
  } else {
    d.show();
    if (png.empty()) std::this_thread::sleep_for(std::chrono::milliseconds((long)(hold * 1000)));
  }
  return 0;
}
