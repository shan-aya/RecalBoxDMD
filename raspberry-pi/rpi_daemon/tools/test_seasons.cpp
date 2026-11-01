// Test hote de seasons.cpp (playlists saisonnieres du demon) avec les VRAIS sdfs.cpp / config.cpp, sur un dossier SD temporaire.
// Compilation (depuis rpi_daemon/) :
//   g++ -std=c++17 -Wall -Wextra -Isrc/core -Isrc/media -Isrc/ui tools/test_seasons.cpp src/media/seasons.cpp src/core/sdfs.cpp src/core/config.cpp -o /tmp/test_seasons && /tmp/test_seasons
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include "config.h"
#include "sdfs.h"
#include "seasons.h"

namespace fs = std::filesystem;
static int fails = 0;
#define CHECK(c) do { if (!(c)) { std::printf("ECHEC l.%d : %s\n", __LINE__, #c); fails++; } } while (0)

static void put(const fs::path &p, const std::string &s) {
  fs::create_directories(p.parent_path());
  std::ofstream(p, std::ios::binary) << s;
}
static std::string slurp(const fs::path &p) {
  std::ifstream i(p, std::ios::binary);
  std::stringstream ss;
  ss << i.rdbuf();
  return ss.str();
}

int main() {
  const fs::path root = fs::temp_directory_path() / "dmd_test_seasons";
  fs::remove_all(root);
  put(root / "playlists" / "Defaut.txt", "# FULL:a\n/gifs/a/1.gif\n/gifs/a/2.gif");  // sans saut de ligne final
  put(root / "playlists" / "ALL.txt", "/gifs/b/3.gif\n");
  put(root / "playlists" / "Hiver.txt", "/gifs/c/4.gif\n/gifs/c/5.gif\n");
  SdFs sd(root.string());
  auto cfgWith = [&](Config &c, const std::string &extra) {
    put(root / "config.ini", "playlist=Defaut.txt\nrandom=1\n" + extra);
    c.load(sd);
  };

  // 1. pas de seasons.ini => defaut
  { Config c; cfgWith(c, "fake_date=20261101\n"); auto p = seasons::choose(sd, c, true); CHECK(p.file.empty() && p.key.empty() && p.ymd == 0); }
  // 2. exclusive Halloween : active le 01/11, pas le 01/12
  put(root / "playlists" / "seasons.ini", "# commentaire\nfete=halloween|playlist=ALL.txt|mode=excl\n");
  { Config c; cfgWith(c, "fake_date=20261101\n"); auto p = seasons::choose(sd, c, true); CHECK(p.file == "ALL.txt" && p.key == "x:ALL.txt" && p.ymd == 20261101); }
  { Config c; cfgWith(c, "fake_date=20261201\n"); auto p = seasons::choose(sd, c, true); CHECK(p.file.empty() && p.key.empty() && p.ymd == 20261201); }
  // 3. playlist saisonniere absente => defaut
  put(root / "playlists" / "seasons.ini", "fete=halloween|playlist=Inexistante.txt|mode=excl\n");
  { Config c; cfgWith(c, "fake_date=20261101\n"); auto p = seasons::choose(sd, c, true); CHECK(p.file.empty() && p.key.empty()); }
  // 4. mixte : defaut + Hiver => fichier genere
  put(root / "playlists" / "seasons.ini", "fete=|start=1201|end=0228|playlist=Hiver.txt|mode=mix\n");
  { Config c; cfgWith(c, "fake_date=20261215\n"); auto p = seasons::choose(sd, c, true);
    CHECK(p.file == "_saison_mix.txt" && p.key == "m:1" && p.add == "Hiver");
    const std::string mix = slurp(root / "playlists" / "_saison_mix.txt");
    CHECK(mix.rfind("# MIX:Defaut.txt:", 0) == 0);
    CHECK(mix.find("/gifs/a/2.gif\n/gifs/c/4.gif\n/gifs/c/5.gif\n") != std::string::npos);   // saut de ligne ajoute apres la playlist sans \n final
    // 4b. deuxieme appel : fichier non reecrit (meme contenu, mtime inchange)
    const auto t0 = fs::last_write_time(root / "playlists" / "_saison_mix.txt");
    seasons::choose(sd, c, true);
    CHECK(fs::last_write_time(root / "playlists" / "_saison_mix.txt") == t0);
    // 4c. une source change => regeneree
    put(root / "playlists" / "Hiver.txt", "/gifs/c/4.gif\n/gifs/c/5.gif\n/gifs/c/6.gif\n");
    seasons::choose(sd, c, true);
    CHECK(slurp(root / "playlists" / "_saison_mix.txt").find("/gifs/c/6.gif") != std::string::npos);
  }
  // 5. exclusive prioritaire sur mixte
  put(root / "playlists" / "seasons.ini", "fete=|start=1201|end=0228|playlist=Hiver.txt|mode=mix\nfete=noel|playlist=ALL.txt|mode=excl\n");
  { Config c; cfgWith(c, "fake_date=20261215\n"); auto p = seasons::choose(sd, c, true); CHECK(p.file == "ALL.txt" && p.key == "x:ALL.txt"); }
  // 6. sans playlist par defaut : rien (widgets seuls)
  { put(root / "config.ini", "fake_date=20261215\n"); Config c; c.load(sd); auto p = seasons::choose(sd, c, true); CHECK(p.file.empty() && p.ymd == 0); }
  // 7. date inconnue (ni fake_date, ni last_date) : l'horloge systeme du PC de test est valide -> on force une config sans date et on accepte les 2 issues
  // 8. last_date= utilise si l'horloge est invalide : non testable ici (horloge hote valide)
  // 9. chargement des regles : ligne trop longue sautee, derniere ligne sans \n
  { std::string longLine = "fete=noel|playlist=" + std::string(400, 'x') + ".txt|mode=excl\n";
    put(root / "playlists" / "seasons.ini", longLine + "fete=paques|playlist=ALL.txt|mode=excl");
    SeasonRule r[PS_MAX_RULES]; const int n = seasons::loadRules(sd, r, PS_MAX_RULES);
    CHECK(n == 1 && r[0].fete >= 0 && std::string(r[0].playlist) == "ALL.txt"); }

  fs::remove_all(root);
  std::printf(fails ? "%d echec(s)\n" : "OK : seasons.cpp (demon Pi) conforme\n", fails);
  return fails ? 1 : 0;
}
