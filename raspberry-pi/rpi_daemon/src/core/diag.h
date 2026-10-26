// diag.h -- mesures du mode diagnostic (portage de DiagCalc / diagPush / wlogAdd du firmware ESP32, v391-v393).
// Fenetre glissante des N derniers tests (1 PING + signal par seconde) : perte, latence min / moyenne / max, gigue ; plus un journal des
// 8 derniers evenements de liaison (C = connexion + BSSID, D = deconnexion). Partagee entre la boucle principale (ecriture) et les
// gestionnaires HTTP (lecture) : tout passe par le verrou.
#pragma once
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

class DiagStats {
 public:
  static constexpr int N = 20;
  static constexpr int EV = 8;
  static constexpr uint16_t LOST = 0xFFFF;  // PING sans PONG
  static constexpr uint16_t NONE = 0xFFFE;  // pas de test (pas de reseau ou IP Recalbox inconnue)
  struct Calc {
    int n = 0, got = 0, lossPct = 0, mn = 0, avg = 0, mx = 0, jit = 0, last = -2;
  };
  struct Ev {
    char type = 'C';
    uint64_t atS = 0;
    std::string data;  // C : BSSID ; D : code de raison (texte)
  };

  void reset() {
    std::lock_guard<std::mutex> lk(mu_);
    lat_.clear();
    rssi_.clear();
    ev_.clear();
    sent_ = got_ = 0;
    disc_ = 0;
    discAtS_ = 0;
  }
  // lat : ms ; -1 = perdu ; -2 = pas de test
  void push(int rssi, int lat) {
    std::lock_guard<std::mutex> lk(mu_);
    lat_.push_back(lat == -1 ? LOST : lat == -2 ? NONE : (uint16_t)(lat > 60000 ? 60000 : lat));
    rssi_.push_back((int8_t)(rssi < -127 ? -127 : rssi > 0 ? 0 : rssi));
    if ((int)lat_.size() > N) {
      lat_.erase(lat_.begin());
      rssi_.erase(rssi_.begin());
    }
    if (lat != -2) sent_++;
    if (lat >= 0) got_++;
  }
  void event(char type, const std::string &data, uint64_t atS) {
    std::lock_guard<std::mutex> lk(mu_);
    ev_.push_back({type, atS, data});
    if ((int)ev_.size() > EV) ev_.erase(ev_.begin());
    if (type == 'D') {
      disc_++;
      discAtS_ = atS;
    }
  }
  Calc calc() const {
    std::lock_guard<std::mutex> lk(mu_);
    Calc c;
    int tested = 0, prev = -1;
    long sum = 0, jsum = 0;
    int jn = 0;
    c.mn = 0x7FFF;
    for (uint16_t v : lat_) {
      if (v == NONE) {
        c.last = -2;
        continue;
      }
      tested++;
      if (v == LOST) {
        c.last = -1;
        prev = -1;
        continue;
      }
      c.got++;
      sum += v;
      if (v < c.mn) c.mn = v;
      if (v > c.mx) c.mx = v;
      c.last = v;
      if (prev >= 0) {
        jsum += v > prev ? v - prev : prev - v;
        jn++;
      }
      prev = v;
    }
    c.n = tested;
    if (!c.got) c.mn = 0;
    else c.avg = (int)(sum / c.got);
    c.lossPct = tested ? (tested - c.got) * 100 / tested : 0;
    c.jit = jn ? (int)(jsum / jn) : 0;
    return c;
  }
  // tableaux JSON « lat » et « rs » (du plus ancien au plus recent), evenements du plus recent au plus ancien
  void arrays(std::string &lat, std::string &rs, std::vector<Ev> &evOut, unsigned long &disc, uint64_t &discAtS) const {
    std::lock_guard<std::mutex> lk(mu_);
    lat.clear();
    rs.clear();
    for (size_t i = 0; i < lat_.size(); i++) {
      if (i) {
        lat += ',';
        rs += ',';
      }
      lat += std::to_string(lat_[i] == LOST ? -1 : lat_[i] == NONE ? -2 : (int)lat_[i]);
      rs += std::to_string((int)rssi_[i]);
    }
    evOut.assign(ev_.rbegin(), ev_.rend());
    disc = disc_;
    discAtS = discAtS_;
  }
  unsigned long disc() const {
    std::lock_guard<std::mutex> lk(mu_);
    return disc_;
  }
  uint64_t discAtS() const {
    std::lock_guard<std::mutex> lk(mu_);
    return discAtS_;
  }

 private:
  mutable std::mutex mu_;
  std::vector<uint16_t> lat_;
  std::vector<int8_t> rssi_;
  std::vector<Ev> ev_;
  unsigned long sent_ = 0, got_ = 0, disc_ = 0;
  uint64_t discAtS_ = 0;
};
