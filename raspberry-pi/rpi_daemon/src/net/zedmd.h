// zedmd.h -- flux ZeDMD-WiFi (UDP 3333) : portage de vpinball_dmd.h du firmware (v21).
//
// EmulationStation (libzedmd/libdmdutil) et Visual Pinball envoient l'image du DMD par UDP : datagrammes « FRAME »? +
// messages « ZeDMD » <cmd> <taille BE16> <flag zlib> <charge>. Les zones (8x4 pixels, 16 par ligne, 128 au total) sont
// RGB888 (cmd 4) ou RGB565 (cmd 5), compressees zlib ; 6 = Render, 10 = Clear, 11 = KeepAlive, 22 = luminosite 0-15.
// Un message peut etre COUPE entre deux datagrammes (reassemblage < 300 ms).
//
// Trois usages, comme le firmware :
//  * Pinball (feat_vpinball_dmd) : le 1er datagramme valide prend l'ecran ; fin = « vpinball_end » (immediat) ou 5 s de silence.
//  * Mode Recalbox natif (recalbox_native_mode) : l'ecran appartient en permanence au flux d'ES ; derniere image conservee
//    dans /native_last.raw565 et reaffichee au demarrage ; un NOUVEAU CLIENT (IP, port source) efface l'ecran.
//  * Fenetre RetroAchievements (feat_ra_window) : le flux n'est accepte que pendant « CMD=ra_window ARG=<ms> » ou pendant une
//    partie vpinball annoncee ; la fin de la fenetre rend l'ecran a la playlist.
// Difference avec l'ESP32 : la socket reste ouverte en permanence (le firmware la fermait hors fenetre pour economiser le tas :
// ES recevait un « port inaccessible ») ; les trames hors fenetre sont lues puis jetees.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "config.h"
#include "display.h"
#include "sdfs.h"
#include "zedcontrol.h"

class ZeDmd {
 public:
  // Ce qui s'est passe pendant poll() (le moteur change de mode en consequence).
  enum class Event {
    NONE,
    ENTERED,        // le flux prend l'ecran
    EXIT_RECALBOX,  // vpinball_end : l'ecran reste vide, les commandes qui suivent le redessinent
    EXIT_RESUME     // silence ou fin de fenetre RA : reprise de la playlist + HELLO (webDmdResume)
  };

  ZeDmd(Display &d, Config &cfg, const SdFs &fs, ZedControl &ctl) : d_(d), cfg_(cfg), fs_(fs), ctl_(ctl) {}
  ~ZeDmd() { stop(); }

  void configure();  // feat_vpinball_dmd, recalbox_native_mode, feat_ra_window
  bool enabled() const { return vpx_ || native_ || ra_; }
  bool native() const { return native_; }
  bool active() const { return active_; }

  bool start(int port = 3333);  // ouvre la socket (sans effet si rien n'est active)
  void stop();
  int port() const { return port_; }

  Event poll(uint64_t now);
  // Mode natif au demarrage : ecran au flux, derniere image reaffichee.
  void enterFromBoot(uint64_t now);

  // Reponse de GET /handshake (decouverte ZeDMD-WiFi) ; vide si le flux est desactive (-> 404).
  std::string handshake() const;

  // GET /nstat : etat du flux (diagnostic sans redemarrer le DMD)
  std::string statusJson() const;

  // Messages de la page web en mode natif (jalon 6) : le flux continue d'etre decode dans la copie mais plus dessine.
  void setOverlay(bool on);

 private:
  bool processPacket(const uint8_t *buf, int len, uint64_t now);
  void handleCommand(uint8_t command, const uint8_t *payload, uint16_t size, bool compressed);
  void decodeZones(const uint8_t *buf, size_t len, bool rgb565);
  void renderZone(uint8_t zone, bool rgb565, const uint8_t *pix);
  void enter(uint64_t now);
  void leave();
  void clearAll();
  void repaintFromShadow();
  void snapRestore();
  void snapService(uint64_t now);
  void logStats(uint64_t now);

  Display &d_;
  Config &cfg_;
  const SdFs &fs_;
  ZedControl &ctl_;
  bool vpx_ = false, native_ = false, ra_ = false;
  int fd_ = -1, port_ = 3333;
  bool active_ = false, entered_ = false, leaving_ = false;

  std::vector<uint8_t> rx_, decomp_, carry_;
  bool carryActive_ = false;
  uint32_t carryHave_ = 0, carrySize_ = 0;
  uint8_t carryCmd_ = 0;
  bool carryComp_ = false;
  uint64_t carryMs_ = 0;

  uint64_t lastPacketMs_ = 0, noEnterUntilMs_ = 0, lastStatsMs_ = 0;
  uint32_t seenEnd_ = 0;
  bool raWindowActive_ = false;  // le mode a ete ouvert PAR la fenetre RA (donc refermé avec elle)
  uint32_t raDropped_ = 0;
  uint64_t raDroppedLogMs_ = 0;

  // copie de l'image (RGB565) : derniere image du mode natif + messages web par-dessus
  std::vector<uint16_t> shadow_;
  bool overlay_ = false;
  bool drawDirty_ = false, shadowDirty_ = false, snapRestored_ = false, snapFailLogged_ = false;
  uint64_t shadowChangeMs_ = 0, snapTryMs_ = 0;

  // session cliente (mode natif)
  bool sessKnown_ = false, sessDisabled_ = false;
  uint32_t sessIp_ = 0, savedIp_ = 0;  // adresse IPv4 : 1er octet dans l'octet de poids faible (comme IPAddress de l'ESP32)
  uint16_t sessPort_ = 0, savedPort_ = 0;
  uint8_t sessChanges_ = 0;
  uint64_t sessWindowMs_ = 0;

  // statistiques (journal toutes les 5 s tant que le flux est actif)
  uint32_t stPkts_ = 0, stZones_ = 0, stBytes_ = 0, stInflateErr_ = 0, stSplitDone_ = 0, stSplitLost_ = 0, stRenders_ = 0,
           stClears_ = 0, stEmpty_ = 0;
  uint32_t pktTotal_ = 0;
};
