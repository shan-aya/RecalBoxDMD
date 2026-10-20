// sched_tune.h -- reglage de l'ordonnancement pour cohabiter avec le WiFi du Raspberry Pi 3B.
//
// PROBLEME (mesure sur le Pi, 18/10) : la bibliotheque d'affichage (hzeller) fait tourner un thread SCHED_FIFO 99 en
// boucle continue sur le coeur 3. Les interruptions du WiFi (SDIO) arrivent sur les 4 coeurs (le controleur du Pi 3
// ne permet pas de les orienter) ; quand l'une tombe sur le coeur 3, la tache du noyau qui la poursuit (kworker/3:*,
// ksoftirqd/3) reste bloquee tant que l'affichage monopolise le coeur : le WiFi se fige 0,5 a 1 s (jusqu'a 20 s si ces
// taches sont temps reel mais sous 99). Resultat : la Recalbox semble « geler » puis tout deverser d'un coup (ping vers
// le routeur : 95 % sous 10 ms SANS affichage, jusqu'a 960 ms AVEC ; meme effet avec une simple boucle sans aucun
// acces aux broches).
//
// REMEDE (valide : WiFi 95 % sous 9 ms, max < 30 ms ; affichage 417 Hz stable, aucune difference visible) :
//   - le thread de rafraichissement passe de FIFO 99 a FIFO 90 (il reste temps reel) ;
//   - les taches du noyau du coeur 3 (kworker/3:*, ksoftirqd/3) et les threads d'interruption (irq/*) passent en FIFO
//     91 : elles peuvent preempter l'affichage le temps de quelques microsecondes. ATTENTION a l'ordre : des taches
//     temps reel SOUS le thread d'affichage aggravent le blocage (le bridage temps reel ne laisse la main qu'aux
//     taches normales) ;
//   - les threads noyau non lies a un coeur sont interdits de coeur 3 (affinite 0-2).
// Un thread de fond rebalaie /proc toutes les 2 s (de nouveaux kworker/3:N apparaissent a la demande).
// Necessite root. Desactivable : `sched_tune=0` dans config.ini ou `--no-sched-tune`.
#pragma once

class SchedTune {
 public:
  SchedTune() = default;
  ~SchedTune();
  SchedTune(const SchedTune &) = delete;
  SchedTune &operator=(const SchedTune &) = delete;

  // A appeler APRES la creation de la matrice (le thread de rafraichissement doit exister).
  void start();
  void stop();

 private:
  struct Impl;
  Impl *impl_ = nullptr;
};
