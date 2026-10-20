// brightsched.h -- planificateur de luminosite (meme grammaire et memes regles que le firmware ESP32, v357).
// Cles config.ini : BRIGHT_SCHED_ON (0/1) et BRIGHT_SCHED = « HH:MM=POURCENT,HH:MM=POURCENT » (24 plages au plus, une par heure, 0 % = ecran eteint).
// Chaque plage vaut de son heure jusqu'a la suivante ; la derniere court jusqu'a la premiere du lendemain.
#pragma once
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

using BrightSlots = std::vector<std::pair<int, int>>;  // (minute du jour, pourcentage), triees par heure

inline BrightSlots parseBrightSched(const std::string &in) {
  BrightSlots v;
  size_t pos = 0;
  while (pos < in.size() && v.size() < 24) {
    size_t c = in.find(',', pos);
    std::string tok = in.substr(pos, c == std::string::npos ? std::string::npos : c - pos);
    pos = (c == std::string::npos) ? in.size() : c + 1;
    while (!tok.empty() && (tok.front() == ' ' || tok.front() == '\t')) tok.erase(tok.begin());
    while (!tok.empty() && (tok.back() == ' ' || tok.back() == '\r' || tok.back() == '\n')) tok.pop_back();
    if (tok.size() < 7 || tok[2] != ':' || tok[5] != '=') continue;
    const int hh = std::atoi(tok.substr(0, 2).c_str()), mm = std::atoi(tok.substr(3, 2).c_str()), pc = std::atoi(tok.substr(6).c_str());
    if (hh < 0 || hh > 23 || mm < 0 || mm > 59 || pc < 0 || pc > 100) continue;
    const int mi = hh * 60 + mm;
    const bool dup = std::any_of(v.begin(), v.end(), [&](const std::pair<int, int> &s) { return s.first == mi; });
    if (!dup) v.push_back({mi, pc});
  }
  std::sort(v.begin(), v.end());
  return v;
}

inline std::string formatBrightSched(const BrightSlots &v) {
  std::string s;
  for (size_t i = 0; i < v.size(); i++) {
    char b[16];
    std::snprintf(b, sizeof(b), "%02d:%02d=%d", v[i].first / 60, v[i].first % 60, v[i].second);
    if (i) s += ',';
    s += b;
  }
  return s;
}

// indice de la plage en vigueur a la minute `cur` (-1 si aucune plage)
inline int brightSlotAt(const BrightSlots &v, int cur) {
  if (v.empty()) return -1;
  int slot = -1;
  for (size_t i = 0; i < v.size(); i++)
    if (v[i].first <= cur) slot = (int)i;
  return slot < 0 ? (int)v.size() - 1 : slot;  // avant la 1re plage du jour : celle de la veille
}
