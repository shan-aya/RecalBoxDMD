// webmedia.cpp -- routes « medias » de la configuration web : playlists, dossiers de GIF, televersement, citations, couleurs d'horloge en direct.
// Portage de handleWebConfigListPlaylists / ListGifDirs / GeneratePlaylist / PlaylistDirs / DeletePlaylist / AddToPlaylistsBatch / CreateFolder /
// Upload / DeleteFolders et des routes de citations du firmware. Le cache maitre cache_master_gifs.dat n'existe pas ici (le Pi relit les dossiers
// en quelques instants) ; la generation reste asynchrone (« STARTED » puis /generate-playlist-status) car la page attend ce contrat.
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <set>
#include <sstream>
#include <thread>

#include "boot_seasons.h"
#include "log.h"
#include "webconfig.h"

namespace fs = std::filesystem;

namespace {
std::string jsonEsc(const std::string &s) {
  std::string out;
  for (char c : s) {
    if (c == '"') out += "\\\"";
    else if (c == '\\') out += "\\\\";
    else if (c == '\r') out += "\\r";
    else if (c == '\n') out += "\\n";
    else if (c == '\t') out += "\\t";
    else out += c;
  }
  return out;
}

// nom de dossier / de fichier sur : ni chemin, ni « .. », ni controle (le firmware ne verifiait rien ; ici le demon tourne en root sur le reseau)
bool safeName(const std::string &n) {
  if (n.empty() || n == "." || n == ".." || n.find('/') != std::string::npos || n.find('\\') != std::string::npos) return false;
  for (unsigned char c : n)
    if (c < 32) return false;
  return true;
}
std::string baseName(std::string n) {
  size_t p = n.find_last_of("/\\");
  if (p != std::string::npos) n = n.substr(p + 1);
  return n;
}
bool langOk(const std::string &l) { return l == "fr" || l == "en" || l == "es"; }
// /gifs/_boot/ (animations de demarrage) : dossier PROTEGE, comme sur le firmware v383 : masque dans la liste des dossiers, suppression refusee, et seuls les 6 fichiers d'animation connus
// peuvent y etre envoyes (Mode 19 de la boite a outils).
bool bootDirProtected(const std::string &d) { return toLower(trim(d)) == "_boot"; }
bool bootAnimNameOk(const std::string &n) {
  static const char *ok[] = {"boot_anim.raw565pack", "boot_halloween.raw565pack", "boot_noel.raw565pack", "boot_nouvelan.raw565pack", "boot_valentin.raw565pack", "boot_paques.raw565pack"};
  for (const char *k : ok)
    if (toLower(n) == k) return true;
  return false;
}
std::string slug(const std::string &in) {  // minuscules, [a-z0-9_-], espaces -> '-', 24 car. max
  std::string o;
  for (size_t i = 0; i < in.size() && o.size() < 24; i++) {
    char c = in[i];
    if (c >= 'A' && c <= 'Z') c = (char)(c + 32);
    if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_') o += c;
    else if (c == ' ') o += '-';
  }
  return o;
}
std::vector<std::string> splitCsv(const std::string &s) {
  std::vector<std::string> v;
  size_t start = 0;
  while (start <= s.size()) {
    const size_t c = s.find(',', start);
    std::string t = trim(c == std::string::npos ? s.substr(start) : s.substr(start, c - start));
    if (!t.empty()) v.push_back(t);
    if (c == std::string::npos) break;
    start = c + 1;
  }
  return v;
}
bool endsWithNoCase(const std::string &s, const std::string &suf) {
  if (s.size() < suf.size()) return false;
  for (size_t i = 0; i < suf.size(); i++)
    if (std::tolower((unsigned char)s[s.size() - suf.size() + i]) != std::tolower((unsigned char)suf[i])) return false;
  return true;
}
WebReply text(int status, const std::string &body) {
  WebReply r;
  r.status = status;
  r.body = body;
  return r;
}
WebReply json(const std::string &body) {
  WebReply r;
  r.contentType = "application/json";
  r.body = body;
  return r;
}
}  // namespace

// generation de playlist en tache de fond : la page l'interroge toutes les 700 ms
struct WebConfig::PlGen {
  std::atomic<bool> active{false}, done{false}, stop{false};
  std::mutex m;  // protege les chaines et compteurs ci-dessous
  std::string dir, result, name;
  int dirIdx = 0, totalDirs = 0, gifs = 0, curDirGifs = 0;
  std::thread th;
  ~PlGen() {
    if (th.joinable()) th.join();
  }
};

void WebConfig::registerMediaRoutes() {
  gen_ = std::make_shared<PlGen>();
  eng_.setWebBusyHook([gen = gen_] { return gen->active.load(); });  // generation de playlist en cours : le DMD ne reprend pas tout seul (Engine::tickWebWatchdog)

  // chemin reel (insensible a la casse pour ce qui existe, tel quel pour ce qu'on cree)
  auto real = [this](const std::string &sdPath) {
    const std::string r = fs_.resolve(sdPath);
    return r.empty() ? fs_.root() + sdPath : r;
  };
  auto ensureDir = [real](const std::string &sdPath) {
    std::error_code ec;
    fs::create_directories(real(sdPath), ec);
    return !ec;
  };

  web_.any("/lsplaylists", [this](const WebRequest &) {
    if (gen_->active) return json("[]");  // rafraichissement silencieux pendant une generation
    std::vector<std::string> names;
    for (const std::string &n : fs_.list("/playlists"))
      if (n.size() > 4 && n.compare(n.size() - 4, 4, ".txt") == 0) names.push_back(n);
    std::sort(names.begin(), names.end());
    std::string j = "[";
    for (size_t i = 0; i < names.size(); i++) j += (i ? "," : "") + std::string("\"") + jsonEsc(names[i]) + "\"";
    return json(j + "]");
  });
  web_.any("/lsgifdirs", [this](const WebRequest &) {
    if (gen_->active) return json("[]");
    std::vector<std::string> names;
    for (const std::string &n : fs_.list("/gifs"))
      if (fs_.isDir("/gifs/" + n) && !bootDirProtected(n)) names.push_back(n);
    std::sort(names.begin(), names.end());
    std::string j = "[";
    for (size_t i = 0; i < names.size(); i++) j += (i ? "," : "") + std::string("\"") + jsonEsc(names[i]) + "\"";
    return json(j + "]");
  });

  // --- generation d'une playlist : /gifs/<dossier>/*.gif de chaque dossier coche, precedee du marqueur « # FULL:<dossiers> »
  web_.post("/generate-playlist", [this, real, ensureDir](const WebRequest &r) {
    if (gen_->active) return text(409, "ERR: generation deja en cours");
    if (!r.has("name") || !r.has("dirs")) return text(400, "ERR: manque nom ou dirs");
    const std::string name = r.arg("name");
    if (!safeName(name)) return text(400, "ERR: nom de playlist invalide");
    std::vector<std::string> dirs;
    for (const std::string &d : splitCsv(r.arg("dirs")))
      if (safeName(d)) dirs.push_back(d);
    std::string marker;
    for (const std::string &d : dirs) marker += (marker.empty() ? "" : ",") + d;
    if (gen_->th.joinable()) gen_->th.join();
    {
      std::lock_guard<std::mutex> lk(gen_->m);
      gen_->name = name;
      gen_->dir.clear();
      gen_->result.clear();
      gen_->dirIdx = 0;
      gen_->totalDirs = (int)dirs.size();
      gen_->gifs = gen_->curDirGifs = 0;
    }
    gen_->stop = false;
    gen_->done = false;
    gen_->active = true;
    auto gen = gen_;
    gen_->th = std::thread([this, gen, dirs, marker, name, real, ensureDir] {
      ensureDir("/playlists");
      const std::string out = real("/playlists") + "/" + name + ".txt";
      std::ofstream f(out, std::ios::binary | std::ios::trunc);
      std::string msg;
      bool stopped = false;
      if (!f) {
        msg = "ERR: ecriture impossible";
      } else {
        f << "# FULL:" << marker << "\n";
        int total = 0;
        for (size_t i = 0; i < dirs.size() && !stopped; i++) {
          {
            std::lock_guard<std::mutex> lk(gen->m);
            gen->dir = dirs[i];
            gen->dirIdx = (int)i + 1;
            gen->curDirGifs = 0;
          }
          std::vector<std::string> files;
          for (const std::string &n : fs_.list("/gifs/" + dirs[i]))
            if (endsWithNoCase(n, ".gif") && !fs_.isDir("/gifs/" + dirs[i] + "/" + n)) files.push_back(n);
          std::sort(files.begin(), files.end());
          int cur = 0;
          for (const std::string &n : files) {
            if (gen->stop) { stopped = true; break; }
            f << "/gifs/" << dirs[i] << "/" << n << "\n";
            total++;
            cur++;
            std::lock_guard<std::mutex> lk(gen->m);
            gen->curDirGifs = cur;
            gen->gifs = total;
          }
        }
        f.close();
        if (stopped) {
          std::error_code ec;
          fs::remove(out, ec);
          msg = "Generation annulee, playlist supprimee";
        } else {
          msg = "OK: " + std::to_string(total) + " GIFs ajoutes dans la playlist " + name + ".txt";
        }
      }
      logf("WEB", "generate-playlist : %s", msg.c_str());
      {
        std::lock_guard<std::mutex> lk(gen->m);
        gen->result = msg;
      }
      gen->active = false;
      gen->done = true;
    });
    return text(200, "STARTED");
  });
  web_.any("/generate-playlist-status", [this](const WebRequest &) {
    std::lock_guard<std::mutex> lk(gen_->m);
    return json(std::string("{\"active\":") + (gen_->active ? "true" : "false") + ",\"done\":" + (gen_->done ? "true" : "false") + ",\"dir\":\"" +
                jsonEsc(gen_->dir) + "\",\"dirIdx\":" + std::to_string(gen_->dirIdx) + ",\"totalDirs\":" + std::to_string(gen_->totalDirs) +
                ",\"gifs\":" + std::to_string(gen_->gifs) + ",\"curDirGifs\":" + std::to_string(gen_->curDirGifs) + ",\"result\":\"" +
                jsonEsc(gen_->result) + "\"}");
  });
  web_.post("/generate-playlist-stop", [this](const WebRequest &) {
    const bool was = gen_->active;
    if (was) gen_->stop = true;
    return text(200, was ? "OK: arret demande" : "OK: rien a arreter");
  });

  // dossiers distincts references par une playlist (pre-cochage des cases quand on la modifie)
  web_.any("/playlist-dirs", [this](const WebRequest &r) {
    if (gen_->active) return json("[]");
    if (!r.has("name")) return text(400, "ERR: manque nom");
    std::string name = baseName(r.arg("name"));
    const size_t dot = name.find_last_of('.');
    if (dot != std::string::npos && dot > 0) name = name.substr(0, dot);
    if (!safeName(name)) return json("[]");
    std::vector<uint8_t> data;
    if (!fs_.readAll("/playlists/" + name + ".txt", data)) return json("[]");
    std::istringstream in(std::string(data.begin(), data.end()));
    std::string line, seen = ",", j = "[";
    bool first = true;
    while (std::getline(in, line)) {
      line = trim(line);
      if (line.compare(0, 6, "/gifs/") != 0) continue;
      const size_t s2 = line.find('/', 6);
      if (s2 == std::string::npos || s2 <= 6) continue;
      const std::string d = line.substr(6, s2 - 6);
      if (seen.find("," + d + ",") != std::string::npos) continue;
      seen += d + ",";
      j += (first ? "" : ",") + std::string("\"") + jsonEsc(d) + "\"";
      first = false;
    }
    return json(j + "]");
  });

  web_.post("/delete-playlist", [this, real](const WebRequest &r) {
    if (gen_->active) return text(409, "ERR: generation en cours");
    if (!r.has("name")) return text(400, "ERR: manque nom");
    std::string base = baseName(r.arg("name"));
    const size_t dot = base.find_last_of('.');
    if (dot != std::string::npos && dot > 0) base = base.substr(0, dot);
    if (!safeName(base)) return text(400, "ERR: nom invalide");
    int deleted = 0;
    for (const char *ext : {".txt", ".cache", ".sig", ".idx"}) {
      const std::string p = "/playlists/" + base + ext;
      if (fs_.exists(p)) {
        std::error_code ec;
        if (fs::remove(real(p), ec)) deleted++;
      }
    }
    return text(200, "OK: " + std::to_string(deleted) + " fichiers supprimes pour " + r.arg("name"));
  });

  // apres un televersement : ajoute les nouveaux GIF aux playlists qui referencent deja le dossier (ou qui l'ont demande en entier : « # FULL: »)
  web_.post("/add-to-playlists-batch", [this, real](const WebRequest &r) {
    if (gen_->active) return text(409, "ERR: generation de playlist en cours");
    if (!r.has("dir") || !r.has("files")) return text(200, "OK:0");
    const std::string folder = trim(r.arg("dir"));
    if (!safeName(folder)) return text(200, "OK:0");
    std::vector<std::string> cands;
    for (const std::string &f : splitCsv(r.arg("files")))
      if (safeName(f) && cands.size() < 64) cands.push_back("/gifs/" + folder + "/" + f);
    if (cands.empty()) return text(200, "OK:0");
    int appended = 0;
    for (const std::string &n : fs_.list("/playlists")) {
      if (n.size() <= 4 || n.compare(n.size() - 4, 4, ".txt") != 0) continue;
      const std::string sd = "/playlists/" + n;
      std::vector<uint8_t> data;
      if (!fs_.readAll(sd, data)) continue;
      const std::string all(data.begin(), data.end());
      std::istringstream in(all);
      std::string first, line;
      std::getline(in, first);
      first = trim(first);
      bool wants;
      if (first.compare(0, 7, "# FULL:") == 0) {
        const std::string list = "," + first.substr(7) + ",";
        wants = list.find("," + folder + ",") != std::string::npos;
      } else {
        wants = all.find("/gifs/" + folder + "/") != std::string::npos;  // ancien style : reference deja au moins un fichier du dossier
      }
      if (!wants) continue;
      std::set<std::string> present;
      {
        std::istringstream in2(all);
        while (std::getline(in2, line)) present.insert(trim(line));
      }
      std::string add;
      for (const std::string &c : cands)
        if (!present.count(c)) {
          add += c + "\n";
          appended++;
        }
      if (add.empty()) continue;
      std::ofstream f(real(sd), std::ios::binary | std::ios::app);
      if (!all.empty() && all.back() != '\n') f << "\n";
      f << add;
    }
    return text(200, "OK:" + std::to_string(appended));
  });

  web_.post("/create-folder", [this, real, ensureDir](const WebRequest &r) {
    if (gen_->active) return text(409, "ERR: generation de playlist en cours");
    const std::string d = trim(r.arg("dir"));
    if (d.empty()) return text(400, "ERR: dossier manquant");
    if (!safeName(d)) return text(400, "ERR: nom de dossier invalide");
    if (fs_.exists("/gifs/" + d)) return text(200, "OK: existant");
    const bool ok = ensureDir("/gifs/" + d) && fs_.exists("/gifs/" + d);
    return text(ok ? 200 : 500, ok ? "OK: cree" : "ERR: creation echouee");
  });

  web_.post("/prepare-upload", [](const WebRequest &) { return json("{\"reboot\":false}"); });  // pas de redemarrage pour liberer la memoire : le Pi n'en a pas besoin

  // televersement (multipart) : GIF dans /gifs/<dossier>/, theme de citations (kind=quote, 64 Ko max), fond d'image (kind=qbg : 8192 octets exactement)
  web_.post("/upload", [this, real, ensureDir](const WebRequest &r) {
    std::string results;
    int okCount = 0;
    for (const WebFile &wf : r.files) {
      std::string name = baseName(wf.filename), dir = trim(r.arg("dir"));
      std::string err, path, dirPath;
      const std::string kind = r.arg("kind");
      bool isBg = kind == "qbg", isQuote = kind == "quote", isDef = kind == "defimg", isReels = kind == "reels";
      const bool reelsRaw = isReels && name.size() > 7 && name.compare(name.size() - 7, 7, ".raw565") == 0;
      size_t limit = 0;
      if (gen_->active) err = "ERR: generation de playlist en cours";
      else if (dir.empty()) err = "ERR: dossier cible manquant";
      else if (name.empty()) err = "ERR: nom fichier invalide";
      else if (isReels) {   // v3.74 : logos reels envoyes par le toolkit (Wi-Fi). dir = base | r_us | r_eu | r_jp ; fichiers : <systeme>.raw565 (8192 octets), _index.bin, LISEZMOI.txt
        const bool dirOk = dir == "base" || dir == "r_us" || dir == "r_eu" || dir == "r_jp";
        bool nameOk = name == "_index.bin" || (name == "LISEZMOI.txt" && dir == "base") || (reelsRaw && name.size() <= 48);
        for (size_t i = 0; nameOk && reelsRaw && i < name.size(); i++) {
          const char c = name[i];
          nameOk = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-';
        }
        if (!dirOk || !nameOk) err = "ERR: logo reel invalide";
        else {
          dirPath = "/systems/_defaults/_themes/logos-reels" + (dir == "base" ? std::string() : "/" + dir);
          limit = reelsRaw ? 8192 : 16384;
        }
      } else if (isDef) {   // image de secours envoyee par le toolkit : nom impose, 8192 octets exactement
        name = "default.raw565";
        dirPath = "/systems/_defaults";
        limit = 8192;
      } else if (isBg) {
        const std::string th = slug(dir);
        if (th.empty()) err = "ERR: nom de fond invalide";
        else {
          name = th + (r.arg("ext") == "col" ? ".col" : ".raw565");
          dirPath = "/widgets/quotes/bg";
          limit = (name.size() > 4 && name.compare(name.size() - 4, 4, ".col") == 0) ? 32 : 8192;
        }
      } else if (isQuote) {
        const std::string lg = r.arg("lang"), th = slug(dir);
        if (!langOk(lg) || th.empty()) err = "ERR: theme invalide";
        else {
          name = th + ".txt";
          dirPath = "/widgets/quotes/" + lg;
          limit = 65536;
        }
      } else {
        if (!safeName(dir) || !safeName(name)) err = "ERR: nom invalide";
        else if (bootDirProtected(dir) && !bootAnimNameOk(name)) err = "ERR: dossier reserve aux animations de demarrage";
        else dirPath = "/gifs/" + dir;
      }
      if (err.empty()) {
        path = dirPath + "/" + name;
        ensureDir(dirPath);
        std::string data = wf.content;
        const size_t sent = data.size();
        if (limit && data.size() > limit) data.resize(limit);
        if (isDef && sent != 8192) {
          err = "ERR: l'image de secours doit faire 8192 octets (128x32 raw565)";
        } else if (isReels && reelsRaw && sent != 8192) {
          err = "ERR: un logo doit faire 8192 octets (128x32 raw565)";
        } else if (isReels && name == "_index.bin" && (sent == 0 || sent % 4 != 0 || sent > 16384)) {
          err = "ERR: index de logos invalide";
        } else if (isReels) {   // temporaire puis renommage : un envoi interrompu ne laisse jamais un logo tronque
          const std::string tmp = real(path) + ".tmp";
          {
            std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
            if (!f || !f.write(data.data(), (std::streamsize)data.size())) err = "ERR: ecriture SD impossible";
          }
          if (err.empty() && std::rename(tmp.c_str(), real(path).c_str()) != 0) err = "ERR: remplacement du logo impossible";
          if (!err.empty()) std::remove(tmp.c_str());
        } else if (isDef) {   // temporaire puis renommage : un envoi interrompu ne laisse jamais une image tronquee ; le cache du demon suit la date du fichier
          const std::string tmp = real(path) + ".tmp";
          {
            std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
            if (!f || !f.write(data.data(), (std::streamsize)data.size())) err = "ERR: ecriture SD impossible";
          }
          if (err.empty() && std::rename(tmp.c_str(), real(path).c_str()) != 0) err = "ERR: remplacement de l'image de secours impossible";
          if (!err.empty()) std::remove(tmp.c_str());
        } else if (isBg && name.size() > 7 && name.compare(name.size() - 7, 7, ".raw565") == 0 && data.size() != 8192) {
          err = "ERR: un fond doit faire 8192 octets (128x32 raw565)";
        } else {
          std::ofstream f(real(path), std::ios::binary | std::ios::trunc);
          if (!f || !f.write(data.data(), (std::streamsize)data.size())) err = "ERR: ecriture SD impossible";
        }
        if (err.empty()) logf("WEB", "upload : %s (%zu octets)", path.c_str(), data.size());
      }
      if (!results.empty()) results += ",";
      if (err.empty()) {
        results += "{\"name\":\"" + jsonEsc(name) + "\",\"ok\":true}";
        okCount++;
      } else {
        logf("WEB", "upload echec : %s (%s)", name.c_str(), err.c_str());
        results += "{\"name\":\"" + jsonEsc(name) + "\",\"ok\":false,\"err\":\"" + jsonEsc(err) + "\"}";
      }
    }
    if (results.empty()) return text(400, "ERR: aucun fichier recu");
    return json("{\"ok\":" + std::to_string(okCount) + ",\"files\":[" + results + "]}");
  });

  // GET /boot-anim[?s=<fete>] : taille + empreinte FNV-1a des images entieres (meme contrat que le firmware, v362 / v367) : le toolkit verifie ainsi la copie apres l'envoi
  // {"size":0} = fichier absent. ?s= : fichier d'une fete (halloween, noel, nouvelan, valentin, paques) au lieu de l'animation par defaut.
  web_.get("/boot-anim", [this](const WebRequest &r) {
    std::string path = "/gifs/_boot/boot_anim.raw565pack";
    if (r.has("s")) {
      const std::string s = r.arg("s");
      bool known = false;
      for (int i = 0; i < BOOT_SEASON_COUNT; i++)
        if (s == kBootSeasons[i].name) known = true;
      if (!known) return text(400, "ERR: fete inconnue");
      path = "/gifs/_boot/boot_" + s + ".raw565pack";
    }
    std::vector<uint8_t> data;
    if (!fs_.readAll(path, data)) return json("{\"size\":0}");
    const size_t FB = 128 * 32 * 2, whole = (data.size() / FB) * FB;
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < whole; i++) {
      h ^= data[i];
      h *= 16777619u;
    }
    char out[112];
    std::snprintf(out, sizeof(out), "{\"size\":%u,\"frames\":%u,\"fnv\":\"%08x\"}", (unsigned)data.size(), (unsigned)(data.size() / FB), (unsigned)h);
    return json(out);
  });
  // GET /boot-season?d=AAAAMMJJ : quelle animation serait jouee a cette date, et quels fichiers saisonniers sont attendus (fonction pure : essais des fenetres de fetes)
  web_.get("/boot-season", [](const WebRequest &r) {
    const uint32_t d = (uint32_t)std::strtoul(r.arg("d").c_str(), nullptr, 10);
    const int si = bsSeasonAt(d);
    const uint8_t want = bsWantedMask(d);
    std::string out = "{\"d\":" + std::to_string(d) + ",\"fete\":\"" + (si >= 0 ? std::string(kBootSeasons[si].name) : std::string()) + "\",\"telecharger\":[";
    bool first = true;
    for (int i = 0; i < BOOT_SEASON_COUNT; i++)
      if (want & (1u << i)) {
        if (!first) out += ",";
        out += std::string("\"") + kBootSeasons[i].name + "\"";
        first = false;
      }
    return json(out + "]}");
  });

  // suppression de dossiers de GIF, puis nettoyage des playlists qui les referencaient
  web_.post("/delete-folders", [this, real](const WebRequest &r) {
    if (gen_->active) return text(409, "ERR: generation de playlist en cours");
    if (!r.has("dirs")) return text(400, "ERR: missing dirs");
    int count = 0, fail = 0;
    std::vector<std::string> deleted;
    for (const std::string &d : splitCsv(r.arg("dirs"))) {
      if (!safeName(d) || bootDirProtected(d)) { fail++; continue; }   // dossier des animations de demarrage : jamais supprime par la page
      const std::string p = "/gifs/" + d;
      if (!fs_.exists(p)) continue;
      std::error_code ec;
      fs::remove_all(real(p), ec);
      if (!ec && !fs_.exists(p)) { count++; deleted.push_back(d); }
      else fail++;
    }
    int plModified = 0, linesRemoved = 0;
    if (!deleted.empty()) {
      for (const std::string &n : fs_.list("/playlists")) {
        if (n.size() <= 4 || n.compare(n.size() - 4, 4, ".txt") != 0) continue;
        const std::string sd = "/playlists/" + n;
        std::vector<uint8_t> data;
        if (!fs_.readAll(sd, data)) continue;
        std::istringstream in(std::string(data.begin(), data.end()));
        std::string line, out;
        int removed = 0;
        while (std::getline(in, line)) {
          const std::string t = trim(line);
          bool drop = false;
          if (t.compare(0, 6, "/gifs/") == 0) {
            const size_t s2 = t.find('/', 6);
            if (s2 != std::string::npos && s2 > 6 && std::find(deleted.begin(), deleted.end(), t.substr(6, s2 - 6)) != deleted.end()) drop = true;
          }
          if (drop) removed++;
          else out += line + "\n";
        }
        if (!removed) continue;
        {
          std::ofstream f(real(sd), std::ios::binary | std::ios::trunc);
          f << out;
        }
        const std::string stem = "/playlists/" + n.substr(0, n.size() - 4);
        for (const char *ext : {".cache", ".sig", ".idx"}) {
          std::error_code ec;
          if (fs_.exists(stem + ext)) fs::remove(real(stem + ext), ec);
        }
        plModified++;
        linesRemoved += removed;
      }
    }
    std::string msg = "OK: " + std::to_string(count) + " supprime(s)" + (fail > 0 ? ", " + std::to_string(fail) + " echec(s)" : "");
    if (plModified > 0)
      msg += ", " + std::to_string(plModified) + " playlist(s) mise(s) a jour (" + std::to_string(linesRemoved) + " ligne(s) retiree(s)), redemarrage necessaire";
    return text(200, msg);
  });

  // --- citations : /widgets/quotes/<langue>/<theme>.txt
  web_.any("/quotes-list", [this](const WebRequest &r) {
    const std::string lg = r.arg("lang");
    if (!langOk(lg)) return text(400, "ERR: langue");
    std::string j = "[";
    int n = 0;
    std::vector<std::string> names = fs_.list("/widgets/quotes/" + lg);
    std::sort(names.begin(), names.end());
    for (const std::string &e : names) {
      if (n >= 24) break;
      if (e.size() > 4 && e.compare(e.size() - 4, 4, ".txt") == 0 && !fs_.isDir("/widgets/quotes/" + lg + "/" + e)) {
        j += (n++ ? "," : "") + std::string("{\"n\":\"") + jsonEsc(e.substr(0, e.size() - 4)) + "\",\"s\":" + std::to_string(fs_.fileSize("/widgets/quotes/" + lg + "/" + e)) + "}";
      }
    }
    return json(j + "]");
  });
  web_.any("/quotes-get", [this](const WebRequest &r) {
    const std::string lg = r.arg("lang"), nm = slug(r.arg("name"));
    if (!langOk(lg) || nm.empty()) return text(400, "ERR");
    std::vector<uint8_t> data;
    if (!fs_.readAll("/widgets/quotes/" + lg + "/" + nm + ".txt", data)) return text(404, "ERR: introuvable");
    WebReply out;
    out.contentType = "text/plain; charset=utf-8";
    out.body.assign(data.begin(), data.end());
    return out;
  });
  web_.post("/quotes-delete", [this, real](const WebRequest &r) {
    const std::string lg = r.arg("lang"), nm = slug(r.arg("name"));
    if (!langOk(lg) || nm.empty()) return text(400, "ERR");
    const std::string p = "/widgets/quotes/" + lg + "/" + nm + ".txt";
    std::error_code ec;
    const bool ok = fs_.exists(p) && fs::remove(real(p), ec);
    return text(ok ? 200 : 404, ok ? "OK" : "ERR: introuvable");
  });
  web_.post("/quotes-restore", [this](const WebRequest &r) {
    const std::string lg = r.arg("lang");
    if (!langOk(lg)) return text(400, "ERR: langue");
    int n = -1;
    eng_.runOnMain([&] { n = eng_.widgetsRestoreQuotes(lg); });
    return n >= 0 ? text(200, "OK " + std::to_string(n)) : text(503, "ERR");
  });

  // couleurs des horloges animees / du theme Neon en direct (page « couleurs »), sans ecriture sur la SD
  web_.post("/clock-color", [this](const WebRequest &r) {
    const std::string a = r.arg("a"), d = r.arg("d"), n = r.arg("n");
    const int slot = r.has("slot") ? std::atoi(r.arg("slot").c_str()) : -1;
    eng_.runOnMain([&] { Clock::setLiveColors(slot, r.has("a") ? &a : nullptr, r.has("d") ? &d : nullptr, r.has("n") ? &n : nullptr); });
    return text(200, "OK");
  });
  web_.post("/stats-reset", [](const WebRequest &) { return text(200, "OK"); });  // le widget « statistiques » n'existe plus
}
