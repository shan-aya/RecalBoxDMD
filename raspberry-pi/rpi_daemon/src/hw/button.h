// button.h -- bouton physique (optionnel), portage de pollPhysicalButton() du firmware (v265, armement v333).
//
// Un fil lit la broche (GPIO21 par defaut, la meme que sur l'ESP32) et reconnait quatre gestes : appui court, double appui, appui long, appui tres
// long. Le geste est relie a une action 0..11 par config.ini (BTN_ACT_SHORT / DOUBLE / LONG / VLONG) ; l'action est executee par la boucle
// principale (Engine::buttonAction). Memes temps et memes protections que le firmware :
//   * anti-rebond 30 ms ;
//   * ARMEMENT : aucune action tant que la broche n'est pas restee 1 s CONTINUE a l'etat haut (bouton absent, broche tenue basse par le materiel :
//     jamais d'action fantome, voir v333) ; une broche restee basse > BTN_VLONG_MS + 3 s est declaree bloquee et desarmee jusqu'au prochain repos ;
//   * appui court differe de 350 ms quand une action « double » est configuree (sinon immediat).
// Acces a la broche par le peripherique caractere /dev/gpiochipN (API noyau, aucun paquet supplementaire). Pull-up demandee au noyau ; si le noyau
// refuse, la broche est lue sans pull-up (resistance externe).
#pragma once
#include <atomic>
#include <functional>
#include <thread>

#include "config.h"

class Button {
 public:
  using Runner = std::function<void(int)>;  // action 1..11 (appelee depuis le fil du bouton : elle poste dans la boucle principale)
  explicit Button(Runner run) : run_(std::move(run)) {}
  ~Button() { stop(); }

  void configure(const Config &cfg);  // relit BTN_* (appel depuis la boucle principale : au demarrage et a chaque rechargement de config.ini)
  void start();
  void stop();

  // diagnostic (GET /load : btn_level, btn_low_ms, btn_armed)
  int level() const { return level_.load(); }
  unsigned lowMs() const { return lowMs_.load(); }
  bool armed() const { return armed_.load(); }

 private:
  void loop();
  bool openLine(int pin);
  void closeLine();
  int readLine();  // 0 / 1, -1 si erreur

  Runner run_;
  std::thread th_;
  std::atomic<bool> stop_{false};
  std::atomic<bool> enabled_{false};
  std::atomic<int> pin_{21}, actShort_{1}, actDouble_{3}, actLong_{2}, actVLong_{6}, longMs_{1500}, vlongMs_{10000};
  std::atomic<int> level_{-1};
  std::atomic<unsigned> lowMs_{0};
  std::atomic<bool> armed_{false};
  int chipFd_ = -1, lineFd_ = -1, openPin_ = -1;
};
