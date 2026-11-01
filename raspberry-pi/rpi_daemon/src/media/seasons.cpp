// seasons.cpp -- voir seasons.h
#include "seasons.h"

#include <cstdlib>
#include <ctime>
#include <fstream>
#include <vector>

#include "log.h"

namespace seasons {

uint32_t todayYmd(const Config &cfg) {
  const uint32_t fake = (uint32_t)std::strtoul(cfg.get("fake_date", "0").c_str(), nullptr, 10);
  if (fake) return fake;
  const time_t now = time(nullptr);
  if (now >= 1700000000L) {  // horloge systeme synchronisee (NTP) ou lue sur la RTC de la carte d'interface
    struct tm ti;
    localtime_r(&now, &ti);
    return (uint32_t)(ti.tm_year + 1900) * 10000u + (uint32_t)(ti.tm_mon + 1) * 100u + (uint32_t)ti.tm_mday;
  }
  return (uint32_t)std::strtoul(cfg.get("last_date", "0").c_str(), nullptr, 10);
}

int yearNow() {
  const time_t now = time(nullptr);
  if (now < 1700000000L) return 2026;
  struct tm ti;
  localtime_r(&now, &ti);
  return ti.tm_year + 1900;
}

int loadRules(const SdFs &fs, SeasonRule *rules, int maxN) {
  std::vector<uint8_t> raw;
  if (!fs.readAll("/playlists/seasons.ini", raw)) return 0;
  int n = 0;
  std::string line;
  bool tooLong = false;
  auto flush = [&] {
    if (!tooLong && n < maxN && psParseLine(line.c_str(), rules[n])) n++;
    line.clear();
    tooLong = false;
  };
  for (uint8_t c : raw) {
    if (c == '\n') flush();
    else if (c != '\r') {
      if (line.size() >= 191) tooLong = true;  // comme le firmware : une ligne trop longue est sautee
      else line.push_back((char)c);
    }
  }
  flush();  // derniere ligne sans saut de ligne
  return n;
}

namespace {

// Cle du choix a la date ymd (voir Pick::key). Une playlist saisonniere ABSENTE de la SD est ignoree ; une exclusive absente => defaut.
std::string keyAt(const SdFs &fs, uint32_t ymd, const SeasonRule *rules, int n, PsChoice &c, bool quiet) {
  c = psChoose(rules, n, ymd);
  if (c.excl >= 0) {
    const std::string nm = rules[c.excl].playlist;
    if (fs.exists("/playlists/" + nm)) return "x:" + nm;
    if (!quiet) logf("SEASON", "playlist saisonniere absente de la SD : %s -> playlist par defaut", nm.c_str());
    return "";
  }
  uint8_t m = 0;
  for (int i = 0; i < n && i < PS_MAX_RULES; i++)
    if ((c.mixMask & (1u << i)) && fs.exists(std::string("/playlists/") + rules[i].playlist)) m |= (uint8_t)(1u << i);
  c.mixMask = m;
  return m ? "m:" + std::to_string((int)m) : std::string();
}

// Genere /playlists/_saison_mix.txt = playlist par defaut + playlists mixees actives. Entete « # MIX:<fichier>:<taille>|... » : reecrit seulement si une source a change.
bool buildMix(const SdFs &fs, const std::string &defaultName, const SeasonRule *rules, uint8_t mask, bool quiet) {
  std::vector<std::string> srcs{defaultName};
  for (int i = 0; i < PS_MAX_RULES; i++)
    if (mask & (1u << i)) srcs.push_back(rules[i].playlist);
  std::string header = "# MIX:";
  for (const std::string &s : srcs) header += s + ":" + std::to_string((long long)(fs.fileSize("/playlists/" + s) < 0 ? 0 : fs.fileSize("/playlists/" + s))) + "|";
  const std::string outSd = "/playlists/" SEASON_MIX_FILE;
  std::vector<uint8_t> cur;
  if (fs.readAll(outSd, cur, header.size() + 1)) {
    const std::string first((const char *)cur.data(), cur.size());
    if (first.compare(0, header.size(), header) == 0 && first.size() > header.size() && first[header.size()] == '\n') return true;  // deja a jour
  }
  const std::string real = fs.resolve(outSd).empty() ? fs.root() + outSd : fs.resolve(outSd);
  std::ofstream o(real, std::ios::binary | std::ios::trunc);
  if (!o) {
    logf("SEASON", "playlist mixee : ecriture impossible (%s)", real.c_str());
    return false;
  }
  o << header << "\n";
  for (const std::string &s : srcs) {
    std::vector<uint8_t> data;
    if (!fs.readAll("/playlists/" + s, data) || data.empty()) continue;
    o.write((const char *)data.data(), (std::streamsize)data.size());
    if (data.back() != '\n') o << "\n";
  }
  if (!quiet) logf("SEASON", "playlist mixee regeneree : %s", header.c_str());
  return (bool)o;
}

}  // namespace

Pick choose(const SdFs &fs, const Config &cfg, bool quiet) {
  Pick p;
  if (cfg.playlistName.empty()) return p;  // pas de playlist par defaut (widgets seuls...) : rien a remplacer
  SeasonRule rules[PS_MAX_RULES];
  const int n = loadRules(fs, rules, PS_MAX_RULES);
  const uint32_t ymd = todayYmd(cfg);
  if (!n || !ymd) return p;
  p.ymd = ymd;
  PsChoice c;
  const std::string key = keyAt(fs, ymd, rules, n, c, quiet);
  if (key.empty()) {
    if (!quiet) logf("SEASON", "%d regle(s), date %u : aucune playlist saisonniere active -> playlist par defaut", n, (unsigned)ymd);
    return p;
  }
  if (c.excl >= 0) {
    p.key = key;
    p.file = rules[c.excl].playlist;
    if (!quiet) logf("SEASON", "date %u : playlist saisonniere exclusive %s", (unsigned)ymd, p.file.c_str());
    return p;
  }
  if (!buildMix(fs, cfg.playlistName, rules, c.mixMask, quiet)) return p;
  p.key = key;
  p.file = SEASON_MIX_FILE;
  for (int i = 0; i < PS_MAX_RULES; i++)
    if (c.mixMask & (1u << i)) {
      std::string nm = rules[i].playlist;
      const size_t dot = nm.find_last_of('.');
      if (dot != std::string::npos && dot > 0) nm.resize(dot);
      p.add += (p.add.empty() ? "" : "+") + nm;
    }
  if (!quiet) logf("SEASON", "date %u : playlist MIXEE (defaut + saisonnieres), masque %d", (unsigned)ymd, (int)c.mixMask);
  return p;
}

}  // namespace seasons
