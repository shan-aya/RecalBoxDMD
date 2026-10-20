// wifimanager.h -- reseau Wi-Fi du Pi (NetworkManager) et point d'acces de secours : equivalent du mode AP de l'ESP32.
//
// Garde-fous repris du firmware (setupWiFiFromConfig / maintainApRecovery / setup) :
//   * config.ini absent, OU aucune configuration Wi-Fi valide (ni wifi_ssid ni profil Wi-Fi connu de NetworkManager), OU premier demarrage
//     (first_boot, ecrit par le toolkit) sans connexion : point d'acces « RecalBox-DMD-Config » (ouvert, 192.168.4.1) + page de configuration,
//     jusqu'a ce qu'un reseau soit configure et joint ;
//   * appareil deja configure (first_boot=0) dont le Wi-Fi tombe : AUCUN repli AP, NetworkManager reessaie (comme maintainWiFi()) ;
//   * CMD=wifi_recovery (scripts de la Recalbox), bouton physique (action 9), cle force_ap_recovery=1 : point d'acces PUR pendant 3 minutes
//     avec compte a rebours, puis retour au reseau normal ;
//   * identifiants faux saisis dans la page du point d'acces : apres l'echec de connexion le point d'acces revient (on peut les ressaisir).
// Le reseau choisi est applique par un profil NetworkManager « recalboxdmd-wifi » (cree / mis a jour a chaque demarrage si config.ini
// a change). Aucune commande n'est lancee par un shell : SSID et mot de passe viennent du reseau (execvp, jamais sh -c).
#pragma once
#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "config.h"
#include "engine.h"
#include "sdfs.h"

class WifiManager {
 public:
  WifiManager(Engine &eng, Config &cfg, SdFs &fs) : eng_(eng), cfg_(cfg), fs_(fs) {}
  ~WifiManager() { stop(); }

  void start();  // thread : etat du reseau au demarrage, garde-fous, point d'acces
  void stop();

  bool apActive() const { return apActive_.load(); }  // la page « / » sert alors la page du point d'acces
  void requestRecovery() { recoveryReq_ = true; }     // CMD=wifi_recovery, bouton, cle force_ap_recovery
  void requestApply() { applyReq_ = true; }           // /save-ap : nouveau SSID / mot de passe ecrits dans config.ini
  std::vector<std::string> scanCache();               // reseaux vus AVANT l'ouverture du point d'acces (le scan est impossible ensuite)

 private:
  void run();
  bool reconcileProfile();                   // cree / met a jour le profil NetworkManager depuis config.ini
  bool connectConfigured(int waitSec);       // monte le profil et attend une connectivite
  bool waitConnectivity(int seconds);
  bool startAp();
  void stopAp();
  void apLoop(bool recovery, int seconds);   // point d'acces : premier demarrage / secours (recovery : avec compte a rebours puis retour)
  void onConnected();
  void showApScreens(uint64_t startMs, int total, bool recovery);

  Engine &eng_;
  Config &cfg_;
  SdFs &fs_;
  std::thread th_;
  std::atomic<bool> stop_{false}, apActive_{false}, recoveryReq_{false}, applyReq_{false};
  std::mutex mu_;
  std::vector<std::string> scan_;
  std::string iface_ = "wlan0";
  uint64_t apLastSecMs_ = 0, apLastToggleMs_ = 0;  // ecrans du secours : derniere mise a jour du compte a rebours / derniere alternance SSID <-> adresse
  bool apShowSsid_ = false;
};
