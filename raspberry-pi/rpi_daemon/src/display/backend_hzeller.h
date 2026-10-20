// backend_hzeller.h -- sortie sur les dalles HUB75 via rpi-rgb-led-matrix (hzeller, GPL 2.0+).
// Compile seulement avec -DWITH_HZELLER=ON (voir CMakeLists.txt).
#pragma once
#include "display.h"
#include "led-matrix.h"
#include "sched_tune.h"

class HzellerBackend : public Backend {
 public:
  // Valeurs par defaut = reglages de la distribution RPI2DMD d'origine releves sur le materiel
  // (64x32, chaine de 2, slowdown 2, RGB, PWM 11, multiplexage 0, row address 0, mapping regular).
  static rgb_matrix::RGBMatrix::Options defaultOptions();
  static rgb_matrix::RuntimeOptions defaultRuntime();  // gpio_slowdown choisi selon le modele de carte
  static std::string boardModel();                      // /proc/device-tree/model (vide si illisible)
  static int defaultSlowdown(const std::string &model);

  HzellerBackend(rgb_matrix::RGBMatrix::Options o, rgb_matrix::RuntimeOptions r, float gamma = 1.0f);
  ~HzellerBackend() override;

  bool begin() override;
  void present(const uint16_t *fb, uint8_t brightness8) override;
  void setGamma(float g);
  // Ordonnancement (voir sched_tune.h) : evite que l'affichage temps reel affame le WiFi. Actif par defaut.
  void setSchedTune(bool on) { schedTune_ = on; }

 private:
  bool schedTune_ = true;
  SchedTune tune_;
  rgb_matrix::RGBMatrix::Options opt_;
  rgb_matrix::RuntimeOptions rt_;
  rgb_matrix::RGBMatrix *matrix_ = nullptr;
  rgb_matrix::FrameCanvas *canvas_ = nullptr;
  uint8_t gamma_lut_[256];
  int last_pct_ = -1;
};
