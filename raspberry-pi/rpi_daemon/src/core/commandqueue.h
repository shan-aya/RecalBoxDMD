// commandqueue.h -- file de commandes entre le thread reseau et la boucle principale.
//
// Deux modes (config.ini `nav_paced`, defaut 1) :
//  * CADENCE (defaut, specifique au Pi) : les commandes de navigation default/system/game sont gardees DANS L'ORDRE
//    et appliquees a raison d'une toutes les `nav_dwell_ms` (defaut 60 ms), pour que chaque logo traverse l'ecran meme
//    quand la Recalbox en envoie 3 ou 4 d'un coup apres un silence (constate sur RB1 : 4 paquets dans la meme
//    milliseconde). Si plus de `nav_backlog` (defaut 5) commandes attendent, les plus anciennes sont abandonnees (saut en
//    avant). « !SHUFFLE » n'est jamais abandonne. Le firmware ESP32 ne pouvait pas faire ca (un jeu y prend 100-300 ms).
//  * FIRMWARE : slots dedies default / system / game (le dernier ecrase le precedent du MEME type), priorite
//    default > system > game, comme processPendingMqttCommand().
// Dans les deux modes, les autres commandes (stop, brightness, score...) partagent UN emplacement generique.
#pragma once
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>

#include "log.h"

struct Cmd {
  std::string cmd, arg;
  uint64_t postedMs = 0;  // horodatage de la mise en file (mesure de latence : attente avant traitement)
};

class CommandQueue {
 public:
  void configure(bool paced, int dwellMs, int backlog) {
    std::lock_guard<std::mutex> lk(m_);
    paced_ = paced;
    dwellMs_ = dwellMs < 0 ? 0 : dwellMs;
    backlog_ = backlog < 1 ? 1 : backlog;
  }
  bool paced() const { return paced_; }

  void post(const std::string &cmd, const std::string &arg) {
    std::lock_guard<std::mutex> lk(m_);
    const uint64_t now = nowMs();
    const bool nav = (cmd == "default" || cmd == "system" || cmd == "game");
    if (nav && paced_) {
      nav_.push_back({cmd, arg, now});
      // trop de retard : on abandonne les plus anciennes (jamais un !SHUFFLE)
      while ((int)nav_.size() > backlog_) {
        auto it = nav_.begin();
        while (it != nav_.end() && it->cmd == "game" && it->arg == "!SHUFFLE") ++it;
        if (it == nav_.end() || it == nav_.end() - 1) break;  // rien d'abandonnable (on garde toujours la plus recente)
        nav_.erase(it);
        skipped_++;
      }
    } else if (cmd == "default") { hasDefault_ = true; defaultMs_ = now; }
    else if (cmd == "system") { hasSystem_ = true; systemArg_ = arg; systemMs_ = now; }
    else if (cmd == "game") { hasGame_ = true; gameArg_ = arg; gameMs_ = now; }
    else { hasGeneric_ = true; generic_ = {cmd, arg, now}; }
  }

  // Renvoie la prochaine commande a appliquer a l'instant `now` (en mode cadence : pas avant dwell ms apres la precedente).
  bool take(Cmd &out, uint64_t now) {
    std::lock_guard<std::mutex> lk(m_);
    if (paced_ && !nav_.empty()) {
      const bool shuffle = nav_.front().cmd == "game" && nav_.front().arg == "!SHUFFLE";
      if (shuffle || now - lastNavMs_ >= (uint64_t)dwellMs_) {
        out = nav_.front();
        nav_.pop_front();
        lastNavMs_ = now;
        return true;
      }
    }
    if (hasDefault_) { hasDefault_ = false; out = {"default", "", defaultMs_}; return true; }
    if (hasSystem_) { hasSystem_ = false; out = {"system", systemArg_, systemMs_}; return true; }
    if (hasGame_) { hasGame_ = false; out = {"game", gameArg_, gameMs_}; return true; }
    if (hasGeneric_) { hasGeneric_ = false; out = generic_; return true; }
    return false;
  }
  bool pending() {
    std::lock_guard<std::mutex> lk(m_);
    return !nav_.empty() || hasDefault_ || hasSystem_ || hasGame_ || hasGeneric_;
  }
  uint64_t skipped() {
    std::lock_guard<std::mutex> lk(m_);
    return skipped_;
  }

 private:
  std::mutex m_;
  bool paced_ = true;
  int dwellMs_ = 60, backlog_ = 5;
  std::deque<Cmd> nav_;
  uint64_t lastNavMs_ = 0, skipped_ = 0;
  bool hasDefault_ = false, hasSystem_ = false, hasGame_ = false, hasGeneric_ = false;
  std::string systemArg_, gameArg_;
  uint64_t defaultMs_ = 0, systemMs_ = 0, gameMs_ = 0;
  Cmd generic_;
};
