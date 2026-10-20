#include "backend_hzeller.h"

#include <cmath>
#include <cstdio>
#include <string>

#include "color.h"

rgb_matrix::RGBMatrix::Options HzellerBackend::defaultOptions() {
  rgb_matrix::RGBMatrix::Options o;
  o.hardware_mapping = "regular";
  o.rows = 32;
  o.cols = 64;
  o.chain_length = 2;
  o.parallel = 1;
  o.pwm_bits = 11;
  o.multiplexing = 0;
  o.row_address_type = 0;
  o.led_rgb_sequence = "RGB";
  // 50 ns (defaut hzeller : 130) : rafraichissement ~386 Hz au lieu de ~200 Hz. Sur ces dalles 1/16 de scan, la
  // marche visible a mi-hauteur sur les objets en mouvement (moitie haute / moitie basse allumees a des
  // instants differents) est inversement proportionnelle a la frequence ; mesure A/B/C du 18/10 : 386 Hz le
  // plus faible, 60 Hz le plus fort. Cout : luminosite tres legerement plus basse (surtout le blanc), aucune
  // difference sur les degrades.
  o.pwm_lsb_nanoseconds = 50;
  o.brightness = 100;  // la luminosite reelle passe par present() (setBrightness8)
  return o;
}

std::string HzellerBackend::boardModel() {
  for (const char *p : {"/proc/device-tree/model", "/sys/firmware/devicetree/base/model"}) {
    if (FILE *f = std::fopen(p, "r")) {
      char buf[160] = {0};
      size_t n = std::fread(buf, 1, sizeof(buf) - 1, f);
      std::fclose(f);
      buf[n] = 0;
      if (n) return std::string(buf);  // le fichier est termine par un NUL, std::string s'arrete avant
    }
  }
  return "";
}

// Ralentissement des acces GPIO selon la carte : plus le SoC est rapide, plus il faut ralentir (doc hzeller :
// Pi 1 = 0-1, Pi 2/3/Zero 2 = 1-2, Pi 4 = 3-4). Valeurs prudentes : un slowdown trop faible = scintillement, trop
// fort = rafraichissement plus bas. Le modele inconnu garde 2 (valeur validee sur Pi 3B).
int HzellerBackend::defaultSlowdown(const std::string &model) {
  auto has = [&](const char *s) { return model.find(s) != std::string::npos; };
  if (has("Raspberry Pi 5")) return 4;
  if (has("Raspberry Pi 4") || has("Raspberry Pi 400") || has("Compute Module 4")) return 4;
  if (has("Raspberry Pi Zero 2") || has("Raspberry Pi 3") || has("Raspberry Pi 2")) return 2;
  if (has("Raspberry Pi Zero") || has("Raspberry Pi Model") || has("Raspberry Pi 1")) return 1;
  return 2;
}

rgb_matrix::RuntimeOptions HzellerBackend::defaultRuntime() {
  rgb_matrix::RuntimeOptions r;
  r.gpio_slowdown = defaultSlowdown(boardModel());
  // On garde les droits root : le demon lit la carte de contenu et ses fichiers apres l'init. Le passage a un
  // utilisateur dedie viendra avec le deploiement (jalon 7).
  r.drop_privileges = -1;
  return r;
}

HzellerBackend::HzellerBackend(rgb_matrix::RGBMatrix::Options o, rgb_matrix::RuntimeOptions r, float gamma)
    : opt_(o), rt_(r) {
  setGamma(gamma);
}

HzellerBackend::~HzellerBackend() {
  tune_.stop();
  if (matrix_) {
    matrix_->Clear();
    delete matrix_;
  }
}

void HzellerBackend::setGamma(float g) {
  for (int i = 0; i < 256; i++) {
    float v = (g == 1.0f) ? (float)i : 255.0f * std::pow(i / 255.0f, g);
    gamma_lut_[i] = (uint8_t)std::lround(v);
  }
}

bool HzellerBackend::begin() {
  matrix_ = rgb_matrix::RGBMatrix::CreateFromOptions(opt_, rt_);
  if (!matrix_) return false;
  if (matrix_->width() != DMD_W || matrix_->height() != DMD_H) {
    std::fprintf(stderr, "HzellerBackend: matrice %dx%d, attendu %dx%d\n", matrix_->width(), matrix_->height(),
                 DMD_W, DMD_H);
    return false;
  }
  canvas_ = matrix_->CreateFrameCanvas();
  if (schedTune_) tune_.start();  // le thread de rafraichissement existe maintenant
  return true;
}

void HzellerBackend::present(const uint16_t *fb, uint8_t brightness8) {
  if (!matrix_) return;
  // setBrightness8() 0..255 -> pourcentage hzeller 1..100 (0 = noir : on vide simplement l'image)
  int pct = (brightness8 * 100 + 127) / 255;
  if (pct < 1 && brightness8 > 0) pct = 1;
  if (pct != last_pct_ && pct > 0) {
    matrix_->SetBrightness((uint8_t)pct);
    last_pct_ = pct;
  }
  for (int y = 0; y < DMD_H; y++) {
    for (int x = 0; x < DMD_W; x++) {
      uint8_t r = 0, g = 0, b = 0;
      if (pct > 0) {
        unpack565(fb[(size_t)y * DMD_W + x], r, g, b);
        r = gamma_lut_[r];
        g = gamma_lut_[g];
        b = gamma_lut_[b];
      }
      canvas_->SetPixel(x, y, r, g, b);
    }
  }
  canvas_ = matrix_->SwapOnVSync(canvas_);
}
