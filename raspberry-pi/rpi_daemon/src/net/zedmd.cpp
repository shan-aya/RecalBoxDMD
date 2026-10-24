#include "zedmd.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <zlib.h>

#include <algorithm>
#include <cstdio>
#include <cstring>

#include "log.h"

namespace {
constexpr int W = 128, H = 32;
constexpr int ZONE_W = W / 16, ZONE_H = H / 8;  // 8 x 4 pixels, formule de ZeDMD (panel.h)
constexpr int ZONES_PER_ROW = W / ZONE_W;       // 16
constexpr int NUM_ZONES = ZONES_PER_ROW * (H / ZONE_H);  // 128
constexpr size_t ZONE_SIZE_888 = ZONE_W * ZONE_H * 3;
constexpr size_t ZONE_SIZE_565 = ZONE_W * ZONE_H * 2;
constexpr uint64_t MODE_TIMEOUT_MS = 5000;        // VPINBALL_MODE_TIMEOUT_MS : silence = fin de partie (hors mode natif)
constexpr uint64_t CARRY_TIMEOUT_MS = 300;        // suite d'un message coupe
constexpr int MAX_DRAIN_PER_POLL = 64;
constexpr const char *SNAP_PATH = "/native_last.raw565";
const uint8_t SYNC[5] = {'Z', 'e', 'D', 'M', 'D'};
const uint8_t FRAME[5] = {'F', 'R', 'A', 'M', 'E'};  // prefixe de datagramme (libzedmd recent)
}  // namespace

void ZeDmd::configure() {
  vpx_ = cfg_.getBool("feat_vpinball_dmd", true);
  native_ = cfg_.getBool("recalbox_native_mode", false);
  ra_ = cfg_.getBool("feat_ra_window", true);
  shadow_.assign((size_t)W * H, 0);
}

bool ZeDmd::start(int port) {
  if (!enabled()) return false;
  port_ = port;
  fd_ = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (fd_ < 0) {
    logf("ZEDMD", "socket() impossible");
    return false;
  }
  int one = 1;
  setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
  int rcvbuf = 1024 * 1024;  // VPX envoie plusieurs centaines de datagrammes par seconde
  setsockopt(fd_, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf));
  sockaddr_in a{};
  a.sin_family = AF_INET;
  a.sin_addr.s_addr = htonl(INADDR_ANY);
  a.sin_port = htons((uint16_t)port);
  if (::bind(fd_, (sockaddr *)&a, sizeof(a)) != 0) {
    logf("ZEDMD", "bind(%d) impossible", port);
    ::close(fd_);
    fd_ = -1;
    return false;
  }
  fcntl(fd_, F_SETFL, fcntl(fd_, F_GETFL, 0) | O_NONBLOCK);
  rx_.assign(4096, 0);
  decomp_.assign(65536, 0);
  carry_.assign(65536, 0);
  logf("ZEDMD", "ecoute sur le port %d (%s%s%s)", port, vpx_ ? "pinball " : "", native_ ? "mode Recalbox natif " : "",
       ra_ ? "fenetre RA" : "");
  return true;
}

void ZeDmd::stop() {
  if (fd_ >= 0) {
    ::close(fd_);
    fd_ = -1;
  }
}

std::string ZeDmd::handshake() const {
  if (!enabled()) return "";
  char host[64] = "dmdpi";
  gethostname(host, sizeof(host) - 1);
  std::string ssid = cfg_.get("wifi_ssid");
  if (ssid.empty()) ssid = host;
  // 20 champs du protocole ZeDMD-WiFi (libzedmd ZeDMDWiFi::DoConnect) : largeur | hauteur | version | S3 | protocole | port |
  // udpDelay | usbPackageSize | luminosite | rgbOrder | 6 reglages panneau | ssid | hdHalf | shortId | wifiPower.
  // Seuls largeur et hauteur sont validees par le client (128x32 ou 256x64).
  return std::to_string(W) + "|" + std::to_string(H) + "|1.0.0|0|UDP|" + std::to_string(port_) + "|0|0|15|0|0|0|0|0|0|0|" + ssid +
         "|0|0|0";
}

std::string ZeDmd::statusJson() const {
  uint32_t sum = 0;  // empreinte de la copie de l'image (voir si elle change)
  for (uint16_t v : shadow_) sum = sum * 31u + v;
  char ip[24];
  std::snprintf(ip, sizeof(ip), "%u.%u.%u.%u", sessIp_ & 255, (sessIp_ >> 8) & 255, (sessIp_ >> 16) & 255, sessIp_ >> 24);
  char b[700];
  std::snprintf(b, sizeof(b),
                "{\"native\":%d,\"modeActive\":%d,\"udpStarted\":%d,\"overlay\":%d,\"inGame\":%d,\"pktTotal\":%u,\"lastPktAgoMs\":%llu,\"sessKnown\":%d,"
                "\"sessIp\":\"%s\",\"sessPort\":%u,\"sessChanges\":%u,\"sessDisabled\":%d,\"snapRestored\":%d,\"shadowDirty\":%d,\"shadowSum\":%u,"
                "\"emptyDropped\":%u}",
                native_ ? 1 : 0, active_ ? 1 : 0, fd_ >= 0 ? 1 : 0, overlay_ ? 1 : 0, ctl_.nativeInGame.load() ? 1 : 0, pktTotal_,
                (unsigned long long)(nowMs() - lastPacketMs_), sessKnown_ ? 1 : 0, ip, sessPort_, sessChanges_, sessDisabled_ ? 1 : 0,
                snapRestored_ ? 1 : 0, shadowDirty_ ? 1 : 0, sum, stEmpty_);
  return b;
}

// ------------------------------------------------------------------ rendu

void ZeDmd::clearAll() {
  if (!overlay_) d_.clearScreen();
  std::fill(shadow_.begin(), shadow_.end(), 0);
  shadowDirty_ = true;
  drawDirty_ = true;
  shadowChangeMs_ = nowMs();
}

void ZeDmd::repaintFromShadow() {
  for (int y = 0; y < H; y++)
    for (int x = 0; x < W; x++) d_.drawPixel(x, y, shadow_[(size_t)y * W + x]);
}

void ZeDmd::setOverlay(bool on) {
  if (overlay_ == on) return;
  overlay_ = on;
  if (!on && active_) {
    repaintFromShadow();
    d_.show();
  }
}

void ZeDmd::renderZone(uint8_t zone, bool rgb565, const uint8_t *pix) {
  stZones_++;
  const int xo = (zone % ZONES_PER_ROW) * ZONE_W;
  const int yo = (zone / ZONES_PER_ROW) * ZONE_H;
  shadowDirty_ = true;
  drawDirty_ = true;
  shadowChangeMs_ = nowMs();
  int p = 0;
  for (int y = 0; y < ZONE_H; y++) {
    for (int x = 0; x < ZONE_W; x++) {
      uint16_t c = 0;
      if (pix) {
        uint8_t r, g, b;
        if (rgb565) {  // petit-boutiste, comme ZeDMD
          const uint16_t v = (uint16_t)(pix[p] | (pix[p + 1] << 8));
          p += 2;
          r = (uint8_t)(((v >> 11) & 0x1f) * 255 / 31);
          g = (uint8_t)(((v >> 5) & 0x3f) * 255 / 63);
          b = (uint8_t)((v & 0x1f) * 255 / 31);
        } else {
          r = pix[p];
          g = pix[p + 1];
          b = pix[p + 2];
          p += 3;
        }
        c = Display::color565(r, g, b);
      }
      if (!overlay_) d_.drawPixel(xo + x, yo + y, c);
      shadow_[(size_t)(yo + y) * W + xo + x] = c;
    }
  }
}

// suite d'enregistrements : 1 octet d'index puis, si index < 128, les pixels de la zone ; index >= 128 = effacer la zone index-128
void ZeDmd::decodeZones(const uint8_t *buf, size_t len, bool rgb565) {
  const size_t zsize = rgb565 ? ZONE_SIZE_565 : ZONE_SIZE_888;
  size_t pos = 0;
  while (pos < len) {
    const uint8_t raw = buf[pos++];
    if (raw >= 128) {
      if (raw - 128 < NUM_ZONES) renderZone((uint8_t)(raw - 128), rgb565, nullptr);
      continue;
    }
    if (pos + zsize > len) break;  // trame tronquee
    if (raw < NUM_ZONES) renderZone(raw, rgb565, &buf[pos]);
    pos += zsize;
  }
}

void ZeDmd::handleCommand(uint8_t command, const uint8_t *payload, uint16_t size, bool compressed) {
  switch (command) {
    case 4:  // zones RGB888
    case 5: {  // zones RGB565
      const uint8_t *zd = payload;
      size_t zlen = size;
      if (compressed) {
        uLongf out = (uLongf)decomp_.size();
        const int st = uncompress(decomp_.data(), &out, payload, size);
        if (st != Z_OK) {
          stInflateErr_++;
          logf("ZEDMD", "erreur de decompression (zlib %d)", st);
          return;
        }
        zd = decomp_.data();
        zlen = out;
      }
      decodeZones(zd, zlen, command == 5);
      break;
    }
    case 6:  // Render : chaque zone est deja dessinee (pas de double tampon, comme le firmware)
      stRenders_++;
      break;
    case 10:  // Clear screen
      stClears_++;
      clearAll();
      break;
    case 11:  // KeepAlive
      break;
    case 22:  // luminosite 0-15 (echelle ZeDMD) -> 0-255
      if (size >= 1) d_.setBrightness8((uint8_t)((payload[0] > 15 ? 15 : payload[0]) * 255 / 15));
      break;
    default: {
      static uint8_t seen[32] = {0};  // une ligne par commande inconnue (USB / menu de reglages ZeDMD sans objet ici)
      if (!(seen[command >> 3] & (1 << (command & 7)))) {
        seen[command >> 3] |= (uint8_t)(1 << (command & 7));
        logf("ZEDMD", "commande ZeDMD non geree cmd=%u size=%u compressee=%d", command, size, compressed ? 1 : 0);
      }
      break;
    }
  }
}

// retourne false si le datagramme est ignore (hors fenetre, retardataire apres vpinball_end)
bool ZeDmd::processPacket(const uint8_t *buf, int len, uint64_t now) {
  if (!active_) {
    if (now < noEnterUntilMs_) return false;  // paquet retardataire apres vpinball_end
    if (ra_ && !native_) {  // mode normal + fenetre RA : une trame n'est acceptee que pendant la fenetre ou une partie vpinball
      const bool win = (int64_t)(ctl_.raWindowUntilMs.load() - now) > 0;
      const bool ctx = ctl_.pinballContext.load();
      if (win && !ctx) {
        raWindowActive_ = true;
        logf("RAWIN", "fenetre ouverte -- affichage de la trame d'ES");
      } else if (ctx) {
        raWindowActive_ = false;
        logf("RAWIN", "partie vpinball annoncee -- affichage du flux de la table");
      } else {
        raDropped_++;
        return false;
      }
    }
    enter(now);
  }
  lastPacketMs_ = now;
  int pos = 0;
  if (carryActive_) {  // suite d'un message coupe par le datagramme precedent
    if ((len >= 5 && std::memcmp(buf, FRAME, 5) == 0) || now - carryMs_ > CARRY_TIMEOUT_MS) {
      carryActive_ = false;  // le datagramme attendu s'est perdu
      stSplitLost_++;
    } else {
      int take = (int)(carrySize_ - carryHave_);
      if (take > len) take = len;
      std::memcpy(carry_.data() + carryHave_, buf, (size_t)take);
      carryHave_ += (uint32_t)take;
      pos = take;
      if (carryHave_ >= carrySize_) {
        carryActive_ = false;
        stSplitDone_++;
        handleCommand(carryCmd_, carry_.data(), (uint16_t)carrySize_, carryComp_);
      }
    }
  }
  while (pos + 5 <= len) {
    if (std::memcmp(&buf[pos], SYNC, 5) != 0) {
      pos++;
      continue;
    }
    pos += 5;
    if (pos + 4 > len) {  // en-tete lui-meme coupe : non gere (comme le firmware)
      stSplitLost_++;
      break;
    }
    const uint8_t command = buf[pos++];
    const uint16_t size = (uint16_t)((buf[pos] << 8) | buf[pos + 1]);
    pos += 2;
    const bool compressed = buf[pos++] != 0;
    if (pos + size > len) {  // charge coupee en fin de datagramme : on garde le debut, la suite est attendue
      const int have = len - pos;
      std::memcpy(carry_.data(), &buf[pos], (size_t)have);
      carryHave_ = (uint32_t)have;
      carrySize_ = size;
      carryCmd_ = command;
      carryComp_ = compressed;
      carryMs_ = now;
      carryActive_ = true;
      break;
    }
    handleCommand(command, &buf[pos], size, compressed);
    pos += size;
  }
  return true;
}

// ------------------------------------------------------------------ etats

void ZeDmd::enter(uint64_t now) {
  active_ = true;
  entered_ = true;
  lastPacketMs_ = now;
  carryActive_ = false;
  if (!overlay_) d_.clearScreen();  // plus de restes de l'affichage precedent sous le flux
  drawDirty_ = true;
  std::fill(shadow_.begin(), shadow_.end(), 0);
  logf("ZEDMD", "mode actif (%s) -- playlist suspendue", native_ ? "natif" : "flux ZeDMD");
}

void ZeDmd::leave() {
  active_ = false;
  raWindowActive_ = false;
  carryActive_ = false;
  d_.clearScreen();
  d_.setBrightness8(cfg_.brightness8());  // le flux a pu changer la luminosite (cmd 22)
  d_.show();
}

void ZeDmd::enterFromBoot(uint64_t now) {
  active_ = true;
  lastPacketMs_ = now;
  d_.clearScreen();
  if (native_) snapRestore();
  d_.show();
  logf("ZEDMD", "mode actif (boot) -- l'ecran appartient au flux d'EmulationStation");
}

// ------------------------------------------------------------------ derniere image (mode natif)

void ZeDmd::snapRestore() {
  std::vector<uint8_t> raw;
  const size_t n = (size_t)W * H * 2;
  if (!fs_.readAll(SNAP_PATH, raw)) {
    logf("NATSNAP", "aucune image sauvegardee");
    return;
  }
  if (raw.size() != n && raw.size() != n + 8) {
    logf("NATSNAP", "image sauvegardee de taille inattendue, ignoree");
    return;
  }
  std::memcpy(shadow_.data(), raw.data(), n);
  if (raw.size() == n + 8) {  // pied : IP (4) + port (2) + 2 octets nuls du client qui a produit l'image
    savedIp_ = (uint32_t)raw[n] | ((uint32_t)raw[n + 1] << 8) | ((uint32_t)raw[n + 2] << 16) | ((uint32_t)raw[n + 3] << 24);
    savedPort_ = (uint16_t)(raw[n + 4] | (raw[n + 5] << 8));
  }
  repaintFromShadow();
  snapRestored_ = true;
  logf("NATSNAP", "derniere image restauree (%u octets)", (unsigned)n);
}

// ecrite quand l'image est STABLE (2 s sans changement) et seulement pendant la navigation (jamais en jeu)
void ZeDmd::snapService(uint64_t now) {
  if (!native_ || !shadowDirty_ || ctl_.nativeInGame.load()) return;
  if ((int64_t)(now - shadowChangeMs_) < 2000) return;  // signe : shadowChangeMs_ peut etre posterieur a `now` (lu plus tard dans le meme passage)
  if (snapTryMs_ != 0 && now - snapTryMs_ < 30000) return;  // apres un echec : on reessaie toutes les 30 s
  snapTryMs_ = now;
  std::string path = fs_.resolve(SNAP_PATH);
  if (path.empty()) path = fs_.root() + SNAP_PATH;
  const std::string tmp = path + ".tmp";
  FILE *f = std::fopen(tmp.c_str(), "wb");
  if (!f) {
    if (!snapFailLogged_) logf("NATSNAP", "ouverture en ecriture impossible (carte en lecture seule ?) : derniere image non conservee");
    snapFailLogged_ = true;
    return;
  }
  const size_t want = (size_t)W * H * 2;
  const uint8_t tr[8] = {(uint8_t)sessIp_,         (uint8_t)(sessIp_ >> 8), (uint8_t)(sessIp_ >> 16), (uint8_t)(sessIp_ >> 24),
                         (uint8_t)sessPort_,       (uint8_t)(sessPort_ >> 8), 0, 0};  // client producteur
  const bool ok = std::fwrite(shadow_.data(), 1, want, f) == want && std::fwrite(tr, 1, 8, f) == 8;
  std::fclose(f);
  if (!ok || std::rename(tmp.c_str(), path.c_str()) != 0) {
    logf("NATSNAP", "ecriture impossible");
    return;
  }
  shadowDirty_ = false;
  snapTryMs_ = 0;
  logf("NATSNAP", "image sauvegardee (%u octets)", (unsigned)want);
}

void ZeDmd::logStats(uint64_t now) {
  if (!active_ || now - lastStatsMs_ < 5000) return;
  lastStatsMs_ = now;
  if (stPkts_ == 0 && stRenders_ == 0) return;
  logf("ZEDMD", "stats 5s: pkts=%u zones=%u octets=%u errZlib=%u rendus=%u effacements=%u recolles=%u perdus=%u vides=%u", stPkts_,
       stZones_, stBytes_, stInflateErr_, stRenders_, stClears_, stSplitDone_, stSplitLost_, stEmpty_);
  stPkts_ = stZones_ = stBytes_ = stInflateErr_ = stRenders_ = stClears_ = stSplitDone_ = stSplitLost_ = stEmpty_ = 0;
}

// ------------------------------------------------------------------ boucle

ZeDmd::Event ZeDmd::poll(uint64_t now) {
  if (!enabled()) return Event::NONE;
  Event ev = Event::NONE;
  entered_ = false;

  // fin de partie annoncee par la Recalbox (marquee.sh a l'evenement endgame) : sortie immediate, sauf en mode natif
  const uint32_t e = ctl_.endRequests.load();
  if (e != seenEnd_) {
    seenEnd_ = e;
    if (!native_ && active_) {
      noEnterUntilMs_ = now + 3000;  // un dernier « clear » du client en train de quitter ne doit pas rouvrir le mode
      leave();
      logf("VPINBALL", "fin de partie (Recalbox) -- retour immediat a l'affichage normal");
      ev = Event::EXIT_RECALBOX;
    }
  }
  const uint32_t rel = ctl_.releaseRequests.load();
  if (rel != seenRelease_) {
    seenRelease_ = rel;
    if (native_ && active_) {
      noEnterUntilMs_ = now + 5000;  // les dernieres images d'ES qui s'eteint ne doivent pas reprendre l'ecran
      leave();
      logf("ZEDMD", "Recalbox eteinte / hors ligne -- l'ecran revient a la playlist");
      ev = Event::EXIT_RESUME;
    }
  }
  if (active_ && !native_) {
    const bool winOpen = (int64_t)(ctl_.raWindowUntilMs.load() - now) > 0;
    if (now - lastPacketMs_ > MODE_TIMEOUT_MS) {
      logf("VPINBALL", "timeout -- retour a l'affichage normal");
      leave();
      ev = Event::EXIT_RESUME;
    } else if (ra_ && raWindowActive_ && !winOpen && !ctl_.pinballContext.load()) {
      logf("RAWIN", "fin de la fenetre -- retour a l'affichage normal");
      leave();
      ev = Event::EXIT_RESUME;
    }
  }

  if (fd_ >= 0) {
    for (int i = 0; i < MAX_DRAIN_PER_POLL; i++) {
      sockaddr_in from{};
      socklen_t fl = sizeof(from);
      const ssize_t r = recvfrom(fd_, rx_.data(), rx_.size(), MSG_DONTWAIT, (sockaddr *)&from, &fl);
      if (r < 0) break;
      if (r == 0) {  // datagramme vide : consomme
        stEmpty_++;
        continue;
      }
      const int len = (int)r;
      pktTotal_++;
      stPkts_++;
      stBytes_ += (uint32_t)len;
      const uint8_t *buf = rx_.data();
      if (!carryActive_ && (len < 5 || (std::memcmp(buf, SYNC, 5) != 0 && std::memcmp(buf, FRAME, 5) != 0))) continue;  // bruit
      if (native_ && !sessDisabled_) {
        // un NOUVEAU CLIENT (IP + port source) part d'une base noire : on efface l'image precedente, sauf s'il s'agit du client qui
        // a produit la copie restauree au demarrage (ES garde son socket, donc son port, a travers un redemarrage du DMD)
        const uint32_t rip = (uint32_t)from.sin_addr.s_addr;
        const uint16_t rport = ntohs(from.sin_port);
        if (!sessKnown_ || rip != sessIp_ || rport != sessPort_) {
          const bool keep = !sessKnown_ && snapRestored_ && savedPort_ != 0 && rip == savedIp_ && rport == savedPort_;
          if (now - sessWindowMs_ > 3000) {
            sessWindowMs_ = now;
            sessChanges_ = 0;
          }
          if (++sessChanges_ > 4) {
            sessDisabled_ = true;
            logf("NATSNAP", "le port source change sans cesse : detection de nouveau client desactivee");
          }
          sessKnown_ = true;
          sessIp_ = rip;
          sessPort_ = rport;
          logf("NATSNAP", "client ZeDMD %u.%u.%u.%u:%u : %s", rip & 255, (rip >> 8) & 255, (rip >> 16) & 255, rip >> 24, rport,
               keep ? "meme client que la copie restauree, image conservee" : "nouveau client, ecran efface");
          if (!keep) clearAll();
        }
      } else if (native_ && !sessKnown_) {  // detection coupee : 1 seul effacement au 1er paquet
        sessKnown_ = true;
        if (!snapRestored_) clearAll();
      }
      const bool wasActive = active_;
      if (processPacket(buf, len, now) && !wasActive && active_) ev = Event::ENTERED;
    }
    if (drawDirty_) {
      drawDirty_ = false;
      if (active_ && !overlay_) d_.show();  // une seule presentation par passage, quel que soit le nombre de datagrammes
    }
  }
  if (entered_) ev = Event::ENTERED;

  if (ra_ && !native_ && raDropped_ > 0 && now - raDroppedLogMs_ >= 10000) {
    raDroppedLogMs_ = now;
    logf("RAWIN", "%u trames d'ES ignorees (fenetre fermee)", raDropped_);
    raDropped_ = 0;
  }
  snapService(now);
  logStats(now);
  return ev;
}
