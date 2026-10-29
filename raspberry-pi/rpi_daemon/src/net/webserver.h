// webserver.h -- serveur HTTP du demon (port 80 par defaut). Enveloppe fine sur cpp-httplib : le reste du code n'inclut jamais
// httplib.h (compilation lente sur le Pi 3B). Jalon 4 : GET /handshake (decouverte ZeDMD-WiFi). Jalon 6 : la page de
// configuration (~55 routes) s'enregistre ici, avec un pool de threads (le firmware etait mono-connexion).
#pragma once
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <vector>

struct WebFile {
  std::string name, filename, contentType, content;  // name = champ du formulaire
};
struct WebRequest {
  std::string method, path, body, remoteAddr, localAddr;
  std::map<std::string, std::string> params;  // requete + formulaire
  std::vector<WebFile> files;                  // fichiers envoyes (multipart), dans l'ordre
  std::map<std::string, std::string> headers;  // en-tetes (noms en minuscules)
  std::string header(const std::string &k) const { auto it = headers.find(k); return it == headers.end() ? std::string() : it->second; }
  bool has(const std::string &k) const { return params.count(k) > 0; }
  std::string arg(const std::string &k) const { auto it = params.find(k); return it == params.end() ? std::string() : it->second; }
};
struct WebReply {
  int status = 200;
  std::string contentType = "text/plain";
  std::string body;
  std::map<std::string, std::string> headers;  // en-tetes supplementaires (Cache-Control, ...)
};
using WebHandler = std::function<WebReply(const WebRequest &)>;

class WebServer {
 public:
  WebServer();
  ~WebServer();
  void get(const std::string &path, WebHandler h);
  void post(const std::string &path, WebHandler h);
  void any(const std::string &path, WebHandler h) { get(path, h); post(path, h); }  // « on(path, handler) » du firmware : toute methode
  void setReplyFilter(std::function<void(WebReply &)> f);  // v3.75 : appele sur chaque reponse avant l'envoi (traduction des messages)
  bool start(int port);  // thread dedie ; false si le port est indisponible
  void stop();

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
  std::thread th_;
};
