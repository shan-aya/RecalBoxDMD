#include "udpserver.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "cmdparse.h"
#include "log.h"

namespace {
constexpr int MAX_UDP_DRAIN_PER_CALL = 20;
constexpr int SKIP_AHEAD_THRESHOLD = 3;
constexpr uint64_t BOOT_HELLO_MIN_DELAY_MS = 5000;

bool startsWithStr(const std::string &s, const char *p) { return s.compare(0, std::strlen(p), p) == 0; }
}  // namespace

std::string featuresPayload(const Config &c) {
  auto clampInt = [](int v, int lo, int hi) { return std::max(lo, std::min(hi, v)); };
  const bool native = c.getBool("recalbox_native_mode", false);
  char buf[256];
  std::snprintf(buf, sizeof(buf),
                "hiscore_ingame=%d;hiscore_browse=%d;info_ingame=%d;info_browse=%d;"
                "description_ingame=%d;description_browse=%d;ra_ingame=%d;ra_browse=%d;"
                "repeat_cycles=%d;repeat_browse_cycles=%d;dwell_seconds=%d;"
                "demo_follow=%d;clip_follow=%d;vpinball_dmd=%d;ra_window=%d",
                c.getBool("feat_hiscore_ingame", true), c.getBool("feat_hiscore_browse", false),
                c.getBool("feat_info_ingame", false), c.getBool("feat_info_browse", true),
                c.getBool("feat_description_ingame", false), c.getBool("feat_description_browse", true),
                c.getBool("feat_ra_ingame", true), c.getBool("feat_ra_browse", false),
                clampInt(c.getInt("feat_repeat_cycles", 3), 0, 20),
                clampInt(c.getInt("feat_repeat_browse_cycles", 3), 0, 20),
                clampInt(c.getInt("feat_dwell_seconds", 5), 3, 30), c.getBool("feat_demo_follow", true),
                c.getBool("feat_clip_follow", true), (c.getBool("feat_vpinball_dmd", true) && !native) ? 1 : 0,
                (c.getBool("feat_ra_window", true) && !native) ? 1 : 0);
  return buf;
}

bool UdpServer::start(int cmdPort, int helloPort) {
  helloPort_ = helloPort;
  fd_ = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd_ < 0) {
    logf("UDP", "socket() impossible");
    return false;
  }
  int one = 1;
  setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
  int rcvbuf = 256 * 1024;
  setsockopt(fd_, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf));
  sockaddr_in a{};
  a.sin_family = AF_INET;
  a.sin_addr.s_addr = htonl(INADDR_ANY);
  a.sin_port = htons((uint16_t)cmdPort);
  if (bind(fd_, (sockaddr *)&a, sizeof(a)) != 0) {
    logf("UDP", "bind(%d) impossible", cmdPort);
    close(fd_);
    fd_ = -1;
    return false;
  }
  rbInit();
  pingIntervalMs_ = (uint64_t)std::max(1000, cfg_.getInt("udp_ping_interval_ms", 15000));  // 15 s comme le firmware ; reglable pour les essais
  startMs_ = nowMs();
  lastPingMs_ = startMs_;  // premier PING 15 s apres le demarrage (g_lastPingSentMs = 0 dans le firmware)
  stop_ = false;
  th_ = std::thread([this] { run(); });
  {
    const std::string act = activeRecalbox();
    logf("UDP", "ecoute sur le port %d (HELLO/PING/FEATURES vers %s:%d)", cmdPort, act.empty() ? "(recalbox_ip absent)" : act.c_str(), helloPort_);
  }
  return true;
}

void UdpServer::stop() {
  stop_ = true;
  if (th_.joinable()) th_.join();
  if (fd_ >= 0) {
    close(fd_);
    fd_ = -1;
  }
}

void UdpServer::run() {
  while (!stop_) {
    pollfd p{fd_, POLLIN, 0};
    int r = poll(&p, 1, 10);  // le firmware lit toutes les 10 ms (udpListenerTask)
    if (r > 0 && (p.revents & POLLIN)) drainOnce();
  }
}

void UdpServer::drainOnce() {
  char scratch[1025];
  std::string buf, shuffleBuf;
  std::vector<std::string> batch;  // tous les paquets de commande lus, dans l'ordre d'arrivee
  bool gotPacket = false, gotShuffle = false;
  int drained = 0;
  while (drained < MAX_UDP_DRAIN_PER_CALL) {
    sockaddr_in from{};
    socklen_t fl = sizeof(from);
    ssize_t l = recvfrom(fd_, scratch, sizeof(scratch) - 1, MSG_DONTWAIT, (sockaddr *)&from, &fl);
    if (l <= 0) break;
    drained++;
    packetsSeen_++;
    lastSeenMs_ = nowMs();
    if (firstSeenMs_.load() == 0) firstSeenMs_ = lastSeenMs_.load();
    scratch[l] = 0;
    const std::string m(scratch, (size_t)l);
    if (rxTrace_) logf("UDPRX", "#%d %s", drained, trim(m).substr(0, 70).c_str());  // chaque paquet lu, retenu ou non

    // plusieurs Recalbox connues : seule l'ACTIVE commande le DMD ; le PONG d'une autre (sonde) la rend active si l'active ne repond plus
    {
      bool multi;
      std::string active;
      {
        std::lock_guard<std::mutex> lk(rbMu_);
        multi = rbIps_.size() > 1;
        active = rbActive_;
      }
      if (multi) {
        char ipb[INET_ADDRSTRLEN] = "";
        inet_ntop(AF_INET, &from.sin_addr, ipb, sizeof(ipb));
        const std::string rip = ipb;
        if (rip != active && rbListed(rip)) {
          bool probing;
          int target;
          {
            std::lock_guard<std::mutex> lk(rbMu_);
            probing = rbProbing_;
            target = rbTarget_;
          }
          if (m == "PONG" && probing && target == 0) rbSwitchTo(rip);
          else continue;
        }
      }
    }
    if (m == "PONG") {  // reponse a notre PING : jamais un « CMD= », consommee ici
      { std::lock_guard<std::mutex> lk(rbMu_); rbProbing_ = false; }
      lastPongMs_ = lastSeenMs_.load();
      if (resendFeaturesOnPong_.exchange(false)) {
        connectedEvent_ = true;
        sendFeatures();
        logf("UDPCHK", "pong apres demarrage/coupure -> reglages (FEATURES) renvoyes a la Recalbox");
      }
      continue;
    }
    noteOnReceipt(m);  // jamais soumis a la regle « un seul paquet retenu » ni au retard de la file cadencee
    // theme / themeopt / scriptver : deposes pour la boucle principale (appliques ensemble au tour suivant)
    if (startsWithStr(m, "CMD=theme") || startsWithStr(m, "CMD=scriptver")) {
      const ParsedCmd pc = parseCommand(m);
      if (pc.cmd == "theme") theme_.setTheme(pc.arg);
      else if (pc.cmd == "themeopt") theme_.setOpt(pc.arg);
      else if (pc.cmd == "scriptver") theme_.setScriptVer(pc.arg);
      logf("UDP", "-> %s", trim(m).c_str());
      continue;
    }
    if (startsWithStr(m, "CMD=rbw")) {  // donnees des widgets Recalbox : deposees pour la boucle principale
      theme_.pushRbw(parseCommand(m).arg);
      logf("UDP", "-> %s", trim(m).substr(0, 60).c_str());
      continue;
    }
    if (startsWithStr(m, "CMD=ra_window") || startsWithStr(m, "CMD=vpinball_end")) {  // traites a la reception, rien a mettre en file
      logf("UDP", "-> %s", trim(m).c_str());
      continue;
    }
    batch.push_back(m);
    if (m.find("ARG=!SHUFFLE") != std::string::npos) {  // signal rare et important : retenu a part, prioritaire
      shuffleBuf = m;
      gotShuffle = true;
    }
    if (!gotPacket || drained > SKIP_AHEAD_THRESHOLD) {  // les premiers un par un, ensuite le plus recent
      buf = m;
      gotPacket = true;
    }
  }
  if (q_.paced()) {
    // mode cadence : AUCUN paquet n'est jete ici, la file les applique dans l'ordre (voir CommandQueue)
    for (const auto &m : batch) {
      const std::string msg = trim(m);
      logf("UDP", "-> %s", msg.c_str());
      dispatch(msg);
      packetsKept_++;
    }
    return;
  }
  if (gotShuffle) {
    buf = shuffleBuf;
    gotPacket = true;
  }
  if (!gotPacket) return;
  const std::string msg = trim(buf);
  logf("UDP", "-> %s", msg.c_str());
  dispatch(msg);
  packetsKept_++;
}

// Equivalent de raNoteGameCommand() + applyThemeUdp(ra_window) + vpinball_end + g_nativeInGame du firmware : ces etats sont poses
// au moment de la reception, pas quand la file applique la commande.
void UdpServer::noteOnReceipt(const std::string &msg) {
  const ParsedCmd p = parseCommand(msg);
  const std::string &cmd = p.cmd;
  if (cmd == "ingame") {
    zed_.nativeInGame = std::atoi(p.arg.c_str()) != 0;
  } else if (cmd == "system" || cmd == "default" || cmd == "game") {
    if (cmd != "game") zed_.nativeInGame = false;  // retour a la navigation
    if (cfg_.getBool("feat_ra_window", true)) {
      const bool pin = cfg_.getBool("feat_vpinball_dmd", true) && msg.find("ARG=vpinball") != std::string::npos;
      if (pin != zed_.pinballContext.load()) {
        zed_.pinballContext = pin;
        logf("RAWIN", pin ? "contexte vpinball ouvert (partie annoncee par la Recalbox)" : "contexte vpinball ferme");
      }
    }
  } else if (cmd == "vpinball_end") {
    zed_.pinballContext = false;
    zed_.endRequests++;
  } else if (cmd == "ra_window") {
    if (!cfg_.getBool("feat_ra_window", true)) {
      logf("RAWIN", "ra_window ignore (option desactivee)");
    } else if (zed_.pinballContext.load()) {
      logf("RAWIN", "ra_window ignore (partie vpinball en cours : pas de RetroAchievements)");
    } else {
      long ms = std::atol(p.arg.c_str());  // toInt() d'Arduino : texte invalide = 0 -> 8000
      if (ms < 500) ms = (ms <= 0) ? 8000 : 500;
      if (ms > 15000) ms = 15000;
      zed_.raWindowUntilMs = nowMs() + (uint64_t)ms;
      logf("RAWIN", "fenetre RA ouverte pour %ld ms", ms);
    }
  }
}

void UdpServer::dispatch(const std::string &msg) {
  const ParsedCmd p = parseCommand(msg);
  const std::string &cmd = p.cmd, &arg = p.arg;
  if (cmd == "ingame") {
    inGame_ = (arg == "1");  // appliquee tout de suite, hors file (comme g_recalboxInGame)
    return;
  }
  // commandes SANS argument : l'ARG eventuel est ignore (comme le firmware, qui n'en recopie pas)
  const bool noArg = cmd == "stop" || cmd == "default" || cmd == "show_config" || cmd == "wifi_recovery" ||
                     cmd == "mem_probe" || cmd == "reboot" || cmd == "brightness_up" || cmd == "brightness_down";
  const bool withArg = cmd == "system" || cmd == "game" || cmd == "brightness";
  if (noArg) q_.post(cmd, std::string());
  else if (withArg) q_.post(cmd, arg);
  else if (cmd == "score") {
    if (!arg.empty()) q_.post(cmd, arg);
  }
  // autres commandes : ignorees, comme le firmware
}

void UdpServer::sendToRecalbox(const std::string &payload, const char *what) {
  const std::string host = activeRecalbox();
  if (host.empty()) return;
  (void)what;
  sendToHost(host, payload);
}

std::string UdpServer::activeRecalbox() {
  std::lock_guard<std::mutex> lk(rbMu_);
  return rbActive_;
}

void UdpServer::sendToHost(const std::string &host, const std::string &payload) {
  const char *what = "udp";
  addrinfo hints{}, *res = nullptr;
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  if (getaddrinfo(host.c_str(), std::to_string(helloPort_).c_str(), &hints, &res) != 0 || !res) {
    logf("UDP", "%s : adresse « %s » non resolue", what, host.c_str());
    return;
  }
  sendto(fd_, payload.data(), payload.size(), 0, res->ai_addr, res->ai_addrlen);
  freeaddrinfo(res);
}

void UdpServer::sendHello() {
  sendToRecalbox("HELLO", "hello");
  logf("UDP", "hello envoye a %s:%d", activeRecalbox().c_str(), helloPort_);
}

void UdpServer::sendPing() { sendToRecalbox("PING", "ping"); }

void UdpServer::sendFeatures() {
  const std::string f = featuresPayload(cfg_);
  sendToRecalbox("FEATURES:" + f, "features");
  logf("UDP", "features envoye a %s:%d -> %s", activeRecalbox().c_str(), helloPort_, f.c_str());
}

// --------------------------------------------------------------------------------------------------------- multi-Recalbox (v316)
// recalbox_ips = liste (4 max, separateurs « , ; espace »), recalbox_target = 0 automatique / 1..4 imposee, recalbox_ip = IP active.
// rbNormalizeList() du firmware : doublons retires ; une IP active absente de la liste passe en tete ; cible hors liste = automatique ;
// cible imposee = l'IP correspondante est l'active ; sinon l'active vide prend la tete de liste.
void UdpServer::rbInit() {
  std::vector<std::string> tmp;
  std::string cur;
  const std::string csv = cfg_.get("recalbox_ips");
  for (size_t i = 0; i <= csv.size(); i++) {
    const char c = (i < csv.size()) ? csv[i] : ',';
    if (c == ',' || c == ';' || c == ' ') {
      const std::string s = trim(cur);
      if (!s.empty() && tmp.size() < 4 && std::find(tmp.begin(), tmp.end(), s) == tmp.end()) tmp.push_back(s);
      cur.clear();
    } else {
      cur += c;
    }
  }
  std::string active = trim(cfg_.get("recalbox_ip"));
  if (!active.empty() && std::find(tmp.begin(), tmp.end(), active) == tmp.end()) {  // ancien config.ini : l'IP active passe en tete
    if (tmp.size() >= 4) tmp.pop_back();
    tmp.insert(tmp.begin(), active);
  }
  int target = cfg_.getInt("recalbox_target", 0);
  if (target < 0 || target > 4) target = 0;
  if (target > (int)tmp.size()) target = 0;
  if (!tmp.empty()) {
    if (target > 0) active = tmp[(size_t)target - 1];
    else if (active.empty()) active = tmp[0];
  }
  std::lock_guard<std::mutex> lk(rbMu_);
  rbIps_ = tmp;
  rbActive_ = active;
  rbTarget_ = target;
  if (tmp.size() > 1) logf("RBSW", "%zu Recalbox connues, active %s, cible %s", tmp.size(), active.c_str(), target ? "imposee" : "automatique");
}

bool UdpServer::rbListed(const std::string &ip) const {
  std::lock_guard<std::mutex> lk(const_cast<std::mutex &>(rbMu_));
  return std::find(rbIps_.begin(), rbIps_.end(), ip) != rbIps_.end();
}

// PAS d'ecriture de config.ini ici (coupure d'alimentation pendant l'ecriture = fichier tronque) : au demarrage suivant le DMD repart
// de la tete de liste et rebascule seul
void UdpServer::rbSwitchTo(const std::string &ip) {
  std::string old;
  {
    std::lock_guard<std::mutex> lk(rbMu_);
    old = rbActive_;
    rbActive_ = ip;
    rbProbing_ = false;
  }
  logf("RBSW", "bascule de la Recalbox %s vers %s", old.c_str(), ip.c_str());
  lastPongMs_ = nowMs();
  resendFeaturesOnPong_ = true;
  sendHello();
}

void UdpServer::rbProbeOthers() {
  std::vector<std::string> others;
  {
    std::lock_guard<std::mutex> lk(rbMu_);
    rbProbing_ = true;
    for (const std::string &s : rbIps_)
      if (s != rbActive_) others.push_back(s);
  }
  for (const std::string &s : others) {
    sendToHost(s, "PING");
    logf("RBSW", "sonde %s", s.c_str());
  }
}

void UdpServer::tickPresence(uint64_t now) {
  if (fd_ < 0) return;
  if (!bootHelloSent_ && now - startMs_ >= BOOT_HELLO_MIN_DELAY_MS) {  // le firmware attend 5 s apres le boot
    bootHelloSent_ = true;
    sendHello();
    resendFeaturesOnPong_ = true;
    return;
  }
  if (now - lastPingMs_ >= pingIntervalMs_) {
    if (activeRecalbox().empty()) {  // aucune Recalbox configuree : rien a sonder (et pas de « ping rate » a journaliser)
      lastPingMs_ = now;
      return;
    }
    // le PING du cycle precedent n'a jamais recu son PONG : cycle rate (prevPingSentMs_ == 0 : tout premier PING, rien a juger)
    const bool missed = prevPingSentMs_ != 0 && lastPongMs_.load() < prevPingSentMs_;
    lastPingMs_ = now;
    prevPingSentMs_ = now;
    sendPing();
    if (!missed) {
      consecutiveMissed_ = 0;
    } else {
      consecutiveMissed_++;
      size_t n;
      int target;
      {
        std::lock_guard<std::mutex> lk(rbMu_);
        n = rbIps_.size();
        target = rbTarget_;
      }
      if (n > 1 && target == 0) rbProbeOthers();  // l'active ne repond pas : sonder les autres Recalbox de la liste
      resendFeaturesOnPong_ = true;
      logf("UDPCHK", "ping rate (consecutif=%d)", consecutiveMissed_);
    }
  }
}