// gamegifcontrol.h -- demandes « CMD=gamegif » (GIFs du jeu) entre le thread reseau et la boucle principale.
// Poses A LA RECEPTION (hors file de commandes, dont l'emplacement generique est ecrase par un score) : une demande ne se perd jamais
// derriere un score, et l'annulation « ARG=0 » est vue tout de suite. Meme esprit que ZedControl / ThemeControl.
//   CMD=gamegif ARG=<n>[|<titre du jeu>]   n = 1..5 GIFs a jouer, titre = <name> de la gamelist (sinon la rom)
//   CMD=gamegif ARG=0                       annule le bloc en cours (fin de jeu, retour en navigation, veille...)
#pragma once
#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>

struct GameGifControl {
  std::atomic<int> req{0};         // 0 = aucune demande ; sinon nombre de GIFs demandes (1..5)
  std::atomic<bool> abort{false};  // « ARG=0 »
  std::atomic<uint64_t> reqMs{0};  // date de reception (la demande est abandonnee si le logo n'est pas stable apres 3 s)

  void post(int n, const std::string &title, uint64_t nowMs) {
    {
      std::lock_guard<std::mutex> lk(mu_);
      title_ = title.substr(0, 60);
    }
    reqMs = nowMs;
    req = n < 1 ? 1 : (n > 5 ? 5 : n);
  }
  std::string takeTitle() {
    std::lock_guard<std::mutex> lk(mu_);
    std::string t;
    t.swap(title_);
    return t;
  }

 private:
  std::mutex mu_;
  std::string title_;
};
