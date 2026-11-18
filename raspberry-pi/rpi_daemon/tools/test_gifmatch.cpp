// test_gifmatch.cpp -- memes cas que tools/test_gifmatch.py (regle de correspondance titre <-> GIF, voir src/media/gifmatch.h).
// Build : g++ -std=c++17 -I src/media -o /tmp/test_gifmatch tools/test_gifmatch.cpp && /tmp/test_gifmatch
#include <cstdio>
#include <string>
#include <vector>

#include "gifmatch.h"

struct NormCase { const char *in, *want; };
struct WordsCase { const char *in; std::vector<const char *> want; };
struct MatchCase { const char *title; std::vector<const char *> gifs; int rank; std::vector<const char *> want; };

static int bestMatches(const std::string &title, const std::vector<const char *> &gifs, std::vector<std::string> &found) {
  const std::string t = ggNorm(title);
  found.clear();
  if (t.size() < 3) return 0;
  int best = 0;
  for (const char *p : gifs) {
    std::string g;
    std::vector<uint8_t> wb;
    if (!ggGifInfo(p, g, wb)) continue;
    const int r = ggRank(t, g, wb);
    if (r == 0 || r < best) continue;
    if (r > best) { best = r; found.clear(); }
    found.push_back(p);
  }
  return best;
}

int main() {
  const NormCase norms[] = {
      {"Street Fighter II: The World Warrior (World 910522)", "streetfighteriitheworldwarrior"},
      {"Pok\xC3\xA9mon Rouge [!]", "pokemonrouge"},
      {"Q*bert", "qbert"},
      {"\xC5\x92uvre \xC3\x86r\xC3\xB8 \xC3\x9F", "oeuvreaeross"},
      {"(USA) Sonic 2", "sonic2"},
      {"\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E", ""},
  };
  const WordsCase words[] = {
      {"ARCADE_AlienStorm01", {"arcade", "alien", "storm", "01"}},
      {"NEOGEOMetal", {"neogeo", "metal"}},
      {"SIGN_eLLuiGi07_ARCADE_Tetris", {"sign", "e", "l", "lui", "gi", "07", "arcade", "tetris"}},
      {"MD_Sonic2", {"md", "sonic", "2"}},
      {"Pok\xC3\xA9mon Rouge (USA)", {"pokemon", "rouge"}},
      {"R-Type II", {"r", "type", "ii"}},
  };
  const MatchCase matches[] = {
      {"Alien Soldier", {"/gifs/Consoles/MD_AlienSoldier05.gif", "/gifs/Consoles/MD_Sonic01.gif"}, 1, {"/gifs/Consoles/MD_AlienSoldier05.gif"}},
      {"Street Fighter II: The World Warrior", {"/gifs/a/Street Fighter II.gif", "/gifs/a/Street Fighter II Turbo.gif"}, 2, {"/gifs/a/Street Fighter II.gif"}},
      {"R-Type", {"/gifs/a/R-Type.gif", "/gifs/a/R-Type II.gif"}, 3, {"/gifs/a/R-Type.gif"}},
      {"R-Type", {"/gifs/a/R-Type II.gif"}, 1, {"/gifs/a/R-Type II.gif"}},
      {"Pang", {"/gifs/a/Super Pang.gif", "/gifs/a/Pang.gif"}, 3, {"/gifs/a/Pang.gif"}},
      {"Pokemon Rouge", {"/gifs/a/Pok\xC3\xA9mon Rouge.gif"}, 3, {"/gifs/a/Pok\xC3\xA9mon Rouge.gif"}},
      {"Pac", {"/gifs/a/Pac-Man.gif"}, 0, {}},
      {"1942", {"/gifs/a/1942.gif", "/gifs/a/1942 Joint Strike.gif"}, 3, {"/gifs/a/1942.gif"}},
      {"ab", {"/gifs/a/ab.gif"}, 0, {}},
      {"Sonic The Hedgehog", {"/gifs/a/x.txt", "/gifs/a/Sonic.gif"}, 2, {"/gifs/a/Sonic.gif"}},
      // v422 : rang 1 = MOTS ENTIERS seulement
      {"Aliens", {"/gifs/a/ARCADE_AlienStorm01.gif", "/gifs/a/MD_AlienSoldier01.gif"}, 0, {}},
      {"Tron", {"/gifs/a/ARCADE_Robotron.gif"}, 0, {}},
      {"Darius", {"/gifs/a/ARCADE_Gdarius.gif"}, 0, {}},
      {"Bomber", {"/gifs/a/ARCADE_Bomberman.gif"}, 0, {}},
      {"Ship", {"/gifs/a/PINBALL_Starship.gif"}, 0, {}},
      {"Splat!", {"/gifs/a/PCE_SplatterHouse01.gif"}, 0, {}},
      {"Mortal Kombat", {"/gifs/a/ARCADE_MortalKombat05SubZero.gif"}, 1, {"/gifs/a/ARCADE_MortalKombat05SubZero.gif"}},
      {"Cyber-Lip", {"/gifs/a/ARCADE_NEOGEO_Cyberlip01.gif", "/gifs/a/ARCADE_NEOGEO_Cyberlip02.gif"}, 1, {"/gifs/a/ARCADE_NEOGEO_Cyberlip01.gif", "/gifs/a/ARCADE_NEOGEO_Cyberlip02.gif"}},
      {"Tetris", {"/gifs/a/SIGN_eLLuiGi07_ARCADE_Tetris.gif"}, 1, {"/gifs/a/SIGN_eLLuiGi07_ARCADE_Tetris.gif"}},
      {"Sonic 2", {"/gifs/a/MD_Sonic2.gif", "/gifs/a/MD_Sonic205.gif"}, 1, {"/gifs/a/MD_Sonic2.gif"}},
      {"Metal Slug", {"/gifs/a/ARCADE_NEOGEO_MetalSlugDie01.gif", "/gifs/a/ARCADE_NEOGEO_Metalslug3marspeople02.gif"}, 1,
       {"/gifs/a/ARCADE_NEOGEO_MetalSlugDie01.gif", "/gifs/a/ARCADE_NEOGEO_Metalslug3marspeople02.gif"}},
      {"Neo Turf Masters", {"/gifs/a/ARCADE_NEOGEO_NeoTurfMasters_ga12.gif"}, 1, {"/gifs/a/ARCADE_NEOGEO_NeoTurfMasters_ga12.gif"}},
      {"Alien", {"/gifs/a/NEOGEOAlien.gif", "/gifs/a/ARCADE_NEOGEOMetal.gif"}, 1, {"/gifs/a/NEOGEOAlien.gif"}},
  };
  // exclusions (v423) : memes cas que tools/test_gifmatch.py
  const char *exclFile = "# commentaire\nKong;ARCADE_DonkeyKong06.gif\nAliens ; arcade_alienstorm01\nJeu Sans GIF;*\n*;ARCADE_1Player.gif\n; sans titre\ntitre;\n";
  struct ExclCase { const char *title; std::vector<const char *> gifs; int rank; std::vector<const char *> want; };
  const ExclCase excls[] = {
      {"Kong", {"/gifs/a/ARCADE_DonkeyKong06.gif", "/gifs/a/ARCADE_DonkeyKong07.gif"}, 1, {"/gifs/a/ARCADE_DonkeyKong07.gif"}},
      {"KONG", {"/gifs/a/ARCADE_DonkeyKong06.gif"}, 0, {}},
      {"Kong (USA)", {"/gifs/a/ARCADE_DonkeyKong06.gif"}, 0, {}},
      {"Aliens", {"/gifs/a/ARCADE_Aliens.gif"}, 1, {"/gifs/a/ARCADE_Aliens.gif"}},
      {"Jeu Sans GIF", {"/gifs/a/JeuSansGIF.gif"}, 0, {}},
      {"Tetris", {"/gifs/a/ARCADE_1Player.gif", "/gifs/a/ARCADE_Tetris.gif"}, 1, {"/gifs/a/ARCADE_Tetris.gif"}},
      {"1Player", {"/gifs/a/ARCADE_1Player.gif"}, 0, {}},
      {"Pang", {"/gifs/a/Pang.gif"}, 3, {"/gifs/a/Pang.gif"}},
  };
  struct StrictCase { const char *title, *gif; bool want; };
  const StrictCase stricts[] = {
      {"Pinball", "PINBALL_Pinball_01", true}, {"Pinball", "PINBALL_Short_Xyz", false}, {"Alien", "ARCADE_Alien_Storm_01", false},
      {"Alien Storm", "ARCADE_Alien_Storm_01", true}, {"Halloween", "HALLOWEEN_Halloween", false}, {"Mario", "NES_Mario_Bros", false},
      {"Sonic 2", "MD_Sonic2", true}, {"Sonic", "MD_Sonic205", false}, {"Alien", "SNES_Alien3", false}, {"Alien", "ARCADE_Alien03", true}, {"Sonic", "MD_Sonic2", false}, {"1942", "ARCADE_1942", true}, {"The Legend of Zelda", "The_Legend_of_Zelda", true},
      {"Legend of Zelda", "The_Legend_of_Zelda", true}, {"1942", "ARCADE_1942_01", true}, {"Pang", "Super Pang", false}, {"ab", "ab", false},
      {"Puzzle Bobble", "ARCADE_NEOGEO_PuzzleBobble01", true}, {"Tetris", "SIGN_eLLuiGi07_ARCADE_Tetris", true},
      {"Pang", "ARCADE_Pang_clivefrog", false}, {"Quiz", "QUIZ_Quiz", false},
  };
  int bad = 0;
  for (const StrictCase &c : stricts) {
    const bool got = ggStrictMatch(c.gif, ggNorm(c.title));
    if (got != c.want) { bad++; std::printf("STRICT ECHEC '%s' / '%s': %d != %d\n", c.title, c.gif, got, c.want); }
  }
  for (const ExclCase &c : excls) {
    const GgExcl ex = ggParseExclusions(exclFile, c.title);
    std::vector<const char *> kept;
    if (!ex.all) {
      for (const char *p : c.gifs) {
        std::string base = p;
        const size_t sl = base.find_last_of("/");
        if (sl != std::string::npos) base = base.substr(sl + 1);
        for (char &ch : base) if (ch >= 'A' && ch <= 'Z') ch = (char)(ch + 32);
        if (!ex.has(base)) kept.push_back(p);
      }
    }
    std::vector<std::string> found;
    const int r = ex.all ? 0 : bestMatches(c.title, kept, found);
    bool same = r == c.rank && found.size() == c.want.size();
    for (size_t i = 0; same && i < found.size(); i++) same = found[i] == c.want[i];
    if (!same) { bad++; std::printf("EXCL ECHEC '%s': rang %d (%zu) != rang %d (%zu)\n", c.title, r, found.size(), c.rank, c.want.size()); }
  }
  for (const NormCase &c : norms) {
    const std::string got = ggNorm(c.in);
    if (got != c.want) { bad++; std::printf("NORM ECHEC '%s': '%s' != '%s'\n", c.in, got.c_str(), c.want); }
  }
  for (const WordsCase &c : words) {
    std::vector<uint8_t> wb;
    const std::string nb = ggNormW(c.in, &wb);
    std::vector<size_t> cuts;
    for (size_t i = 0; i < wb.size(); i++) if (wb[i]) cuts.push_back(i);
    cuts.push_back(nb.size());
    bool same = (cuts.size() - 1) == c.want.size();
    std::string got;
    for (size_t k = 0; k + 1 < cuts.size(); k++) {
      const std::string w = nb.substr(cuts[k], cuts[k + 1] - cuts[k]);
      got += (k ? "," : "") + w;
      if (same && k < c.want.size() && w != c.want[k]) same = false;
    }
    if (!same) { bad++; std::printf("WORDS ECHEC '%s': %s\n", c.in, got.c_str()); }
  }
  for (const MatchCase &c : matches) {
    std::vector<std::string> found;
    const int r = bestMatches(c.title, c.gifs, found);
    bool same = r == c.rank && found.size() == c.want.size();
    for (size_t i = 0; same && i < found.size(); i++) same = found[i] == c.want[i];
    if (!same) { bad++; std::printf("MATCH ECHEC '%s': rang %d (%zu) != rang %d (%zu)\n", c.title, r, found.size(), c.rank, c.want.size()); }
  }
  std::printf(bad ? "%d echec(s)\n" : "OK\n", bad);
  return bad ? 1 : 0;
}
