// webconfig.h -- page de configuration web du demon (portage de web_config.h du firmware).
//
// Les pages HTML / CSS / JS sont CELLES du firmware (extraites par tools/port_web_assets.py vers rpi_daemon/web/) : meme interface que sur
// l'ESP32. Ici : les ~55 routes que ces pages appellent. Difference de fond : l'etat est dans config.ini (et le moteur), pas dans des
// variables globales ; /save met a jour les cles recues, avec les memes validations que le firmware, puis le moteur relit la configuration.
// Les gestionnaires HTTP tournent dans des threads du pool ; tout ce qui touche l'affichage passe par Engine::runOnMain().
#pragma once
#include <atomic>
#include <memory>
#include <string>

#include "config.h"
#include "engine.h"
#include "sdfs.h"
#include "udpserver.h"
#include "webserver.h"

class WifiManager;
class Button;

class WebConfig {
 public:
  WebConfig(Engine &eng, Config &cfg, SdFs &fs, UdpServer &udp, WebServer &web, std::string webDir)
      : eng_(eng), cfg_(cfg), fs_(fs), udp_(udp), web_(web), webDir_(std::move(webDir)) {}
  void registerRoutes();
  void setButton(Button *b) { btn_ = b; }      // diagnostic du bouton physique dans /load
  void setWifi(WifiManager *w) { wifi_ = w; }  // point d'acces de configuration (jalon 7) : page « / », /scan-wifi, /save-ap

 private:
  WebReply page(const std::string &file, const WebRequest &r);  // ouvre aussi l'ecran de configuration du DMD (comme le firmware)
  void registerMediaRoutes();                                      // webmedia.cpp : playlists, dossiers de GIF, televersement, citations
  WebReply asset(const std::string &file, const char *type, const char *cache);
  WebReply load();
  WebReply save(const WebRequest &r);
  WebReply handshake(const WebRequest &r);
  WebReply nstat();
  std::string wg(const std::string &key, const std::string &def = "") const;  // cle WIDGET_* (ou ancienne DECO_*)

  Engine &eng_;
  Config &cfg_;
  SdFs &fs_;
  UdpServer &udp_;
  WebServer &web_;
  std::string webDir_;
  WifiManager *wifi_ = nullptr;
  std::atomic<bool> netDirty_{false};  // reseau (SSID / mot de passe / IP fixe) modifie depuis le demarrage : « Redemarrer » redemarre alors le Pi
  Button *btn_ = nullptr;
  struct PlGen;
  std::shared_ptr<PlGen> gen_;
};
