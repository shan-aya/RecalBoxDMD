#include "webserver.h"

#include <httplib.h>

#include <cctype>
#include <chrono>

#include "log.h"

struct WebServer::Impl {
  httplib::Server svr;
};

WebServer::WebServer() : impl_(new Impl) {}
WebServer::~WebServer() { stop(); }

namespace {
WebRequest toRequest(const httplib::Request &req) {
  WebRequest r;
  r.method = req.method;
  r.path = req.path;
  r.body = req.body;
  r.remoteAddr = req.remote_addr;
  r.localAddr = req.local_addr;
  for (const auto &p : req.params) r.params[p.first] = p.second;
  for (const auto &f : req.files) {
    if (f.second.filename.empty()) r.params[f.first] = f.second.content;  // champ texte d'un formulaire multipart
    else r.files.push_back({f.first, f.second.filename, f.second.content_type, f.second.content});
  }
  for (const auto &h : req.headers) {
    std::string k = h.first;
    for (char &c : k) c = (char)std::tolower((unsigned char)c);
    r.headers[k] = h.second;
  }
  return r;
}
void fill(const WebReply &rep, httplib::Response &res) {
  res.status = rep.status;
  res.set_content(rep.body, rep.contentType);
  res.set_header("Cache-Control", "no-cache");
  for (const auto &h : rep.headers) res.set_header(h.first.c_str(), h.second.c_str());
}
}  // namespace

void WebServer::get(const std::string &path, WebHandler h) {
  impl_->svr.Get(path, [h](const httplib::Request &req, httplib::Response &res) { fill(h(toRequest(req)), res); });
}
void WebServer::post(const std::string &path, WebHandler h) {
  impl_->svr.Post(path, [h](const httplib::Request &req, httplib::Response &res) { fill(h(toRequest(req)), res); });
}

bool WebServer::start(int port) {
  if (!impl_->svr.bind_to_port("0.0.0.0", port)) {
    logf("WEB", "port %d indisponible (deja utilise, ou droits insuffisants)", port);
    return false;
  }
  th_ = std::thread([this] { impl_->svr.listen_after_bind(); });
  logf("WEB", "serveur HTTP sur le port %d", port);
  return true;
}

void WebServer::stop() {
  impl_->svr.stop();
  if (th_.joinable()) th_.join();
}
