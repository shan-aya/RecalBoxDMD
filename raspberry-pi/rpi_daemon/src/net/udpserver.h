// udpserver.h -- canal de commandes UDP (portage de handleUdpCommand / sendUdpHello / sendUdpPing /
// sendUdpFeatureStatus du firmware).
//   - ecoute le port 5005 (UDP_CMD_PORT) : « CMD=<nom> ARG=<reste> », 1024 octets max ;
//   - emet vers <recalbox_ip>:5006 (UDP_HELLO_PORT) depuis ce MEME socket (la Recalbox repond « PONG » a l'expediteur) :
//     « HELLO » (resynchronisation d'etat), « PING » (toutes les 15 s), « FEATURES:k=v;... » (reglages).
// Vidange comme le firmware : jusqu'a 20 paquets par passage, les 3 premiers traites un par un puis seul le dernier est
// retenu (retard confirme), « ARG=!SHUFFLE » toujours retenu en priorite, « PONG » consomme a part.
#pragma once
#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "commandqueue.h"
#include "config.h"
#include "themecontrol.h"
#include "zedcontrol.h"

class UdpServer {
 public:
  UdpServer(CommandQueue &q, const Config &cfg, std::atomic<bool> &inGame, ZedControl &zed, ThemeControl &theme)
      : q_(q), cfg_(cfg), inGame_(inGame), zed_(zed), theme_(theme) {}
  ~UdpServer() { stop(); }

  bool start(int cmdPort = 5005, int helloPort = 5006);
  void setRxTrace(bool on) { rxTrace_ = on; }  // journalise TOUS les paquets recus (diagnostic de rafales)
  void stop();

  // Presence (appele depuis la boucle principale) : HELLO de demarrage, PING periodique.
  void tickPresence(uint64_t nowMs);
  uint64_t lastPacketMs() const { return lastSeenMs_.load(); }
  // mode diagnostic : PONG recus (compteur), heure d'arrivee du dernier, coupures (2 cycles de PING sans PONG de suite)
  uint64_t pongCount() const { return pongCount_.load(); }
  uint64_t lastPongMs() const { return lastPongMs_.load(); }
  uint64_t gapCount() const { return gapCount_.load(); }

  // Recalbox active (celle qui commande le DMD) : voir « multi-Recalbox » dans udpserver.cpp
  std::string activeRecalbox();
  void rbReload() { rbInit(); }                // liste / cible / IP active modifiees (page web)
  uint64_t firstPacketMs() const { return firstSeenMs_.load(); }
  // la Recalbox vient de repondre pour la 1re fois (apres le demarrage ou une coupure) : la boucle principale affiche « RecalBox connectee » (5 s)
  bool takeConnectedEvent() { return connectedEvent_.exchange(false); }
  // « RecalBox hors ligne » : 2 cycles de PING sans PONG de suite (firmware v200), 3 alertes au plus par episode ; `gate` = l'ecran peut-il l'afficher (hors playlist, page de configuration fermee)
  void setAlertGate(std::function<bool()> gate) { alertGate_ = std::move(gate); }
  bool takeDisconnectedEvent() { return discEvent_.exchange(false); }
  bool takePongEvent() { return pongEvent_.exchange(false); }  // un PONG vient d'arriver (resynchronisation apres une coupure)
  void sendHello();
  void sendPing();
  void sendFeatures();

 private:
  void run();
  void drainOnce();
  void dispatch(const std::string &msg);
  void noteOnReceipt(const std::string &msg);  // drapeaux ZeDMD poses a la reception (contexte vpinball, fenetre RA, vpinball_end, en jeu)
  void sendToRecalbox(const std::string &payload, const char *what);
  void sendToHost(const std::string &host, const std::string &payload);
  void rbInit();                               // liste, cible et IP active (recalbox_ips / recalbox_target / recalbox_ip)
  void rbSwitchTo(const std::string &ip);      // bascule vers une autre Recalbox de la liste
  void rbProbeOthers();                        // PING aux autres Recalbox quand l'active ne repond plus
  bool rbListed(const std::string &ip) const;

  CommandQueue &q_;
  const Config &cfg_;
  std::atomic<bool> &inGame_;
  ZedControl &zed_;
  ThemeControl &theme_;
  int fd_ = -1;
  int helloPort_ = 5006;
  std::thread th_;
  std::atomic<bool> stop_{false};
  bool rxTrace_ = false;
  std::atomic<uint64_t> lastSeenMs_{0};
  std::atomic<uint64_t> firstSeenMs_{0};
  std::atomic<uint64_t> lastPongMs_{0};
  std::atomic<uint64_t> pongCount_{0}, gapCount_{0};
  std::atomic<bool> resendFeaturesOnPong_{true};
  std::atomic<bool> connectedEvent_{false};
  std::atomic<bool> discEvent_{false}, pongEvent_{false};
  std::function<bool()> alertGate_;
  int discAlertCount_ = 0;
  uint64_t startMs_ = 0, lastPingMs_ = 0;
  bool bootHelloSent_ = false;
  // multi-Recalbox (jusqu'a 4) : seule l'ACTIVE commande le DMD ; le PONG d'une autre (sonde) la rend active si l'active ne repond plus
  std::mutex rbMu_;
  std::vector<std::string> rbIps_;
  std::string rbActive_;
  int rbTarget_ = 0;                           // 0 = automatique, 1..4 = Recalbox imposee (jamais de bascule)
  bool rbProbing_ = false;
  uint64_t pingIntervalMs_ = 15000, prevPingSentMs_ = 0;
  int consecutiveMissed_ = 0;
  uint64_t packetsSeen_ = 0, packetsKept_ = 0;
};

// Charge utile « hiscore_ingame=1;...;ra_window=0 » construite depuis config.ini (broadcastFeatureStatus).
std::string featuresPayload(const Config &cfg);
