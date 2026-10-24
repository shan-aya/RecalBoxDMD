// zedcontrol.h -- etat partage entre le thread reseau (UDP 5005) et le flux ZeDMD (boucle principale).
// Le firmware pose ces drapeaux « a la reception » du paquet (boucle de vidage UDP), avant et independamment de la file de
// commandes : on garde la meme chose, car la file cadencee du Pi peut retarder l'application d'une commande.
#pragma once
#include <atomic>
#include <cstdint>

struct ZedControl {
  std::atomic<bool> pinballContext{false};      // g_pinballContext : la Recalbox a annonce une partie vpinball (CMD=game ARG=vpinball/...)
  std::atomic<uint64_t> raWindowUntilMs{0};     // raWindowUntilMs : fin de la fenetre RetroAchievements (nowMs)
  std::atomic<uint32_t> endRequests{0};         // incremente a chaque CMD=vpinball_end
  std::atomic<uint32_t> releaseRequests{0};     // CMD=rb_shutdown (extinction de la Recalbox) ou Recalbox hors ligne : le mode natif rend l'ecran a la playlist
  std::atomic<bool> nativeInGame{false};       // g_nativeInGame : « en jeu » (ingame 1) ; system / default = retour a la navigation
};
