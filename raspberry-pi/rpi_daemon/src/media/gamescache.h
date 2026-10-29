// gamescache.h -- /games_cache.bin : index bigramme des jeux ayant une image (meme format que le firmware).
//   uint32 nbSystemes ; nbSystemes x { char nom[32] ; uint32 offset } ;
//   a chaque offset : uint32 table[703] (index bigramme) pointant vers des tranches de { char type ; nom\0 }.
// findInGamesCache() : correspondance exacte (insensible a la casse) sinon plus long prefixe strict ; '?' = absent.
#pragma once
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "sdfs.h"

constexpr int NB_IDX = 703;  // 1 + 26*27

int bigramIndex(const std::string &name);
std::string bigramLabel(int bi);

class GamesCache {
 public:
  // Recharge automatiquement si games_cache.bin a change sur disque (mise a jour par le toolkit).
  char find(const SdFs &fs, const std::string &sys, const std::string &game);
  size_t systems() const { return idx_.size(); }

  // v3.75 : complete l'index avec des jeux dont le logo vient d'etre envoye (kind=logo). games = { systeme, nom du jeu sans extension }. Meme disposition que le Mode 6 du toolkit
  // (en-tete { nom[32], offset absolu }, par systeme une table de 703 offsets absolus puis des tranches { type, nom en minuscules, 0 }) ; un jeu ajoute va en fin de sa tranche.
  // games_cache.bin est reecrit (.tmp + renommage) SEULEMENT si un jeu manque ET si la relecture-reecriture du fichier actuel redonne exactement les memes octets (sinon le format
  // n'est pas celui qu'on sait reproduire : rien n'est ecrit). dry : ne rien ecrire. -> nombre de jeux ajoutes (ou qui le seraient), -1 si impossible (fichier absent, illisible ou non reproductible).
  // identical (facultatif) : la reecriture a l'identique du fichier lu redonne les memes octets. N'utilise aucun etat de l'objet : appelable depuis un fil HTTP.
  static int addGames(const SdFs &fs, const std::vector<std::pair<std::string, std::string>> &games, bool dry = false, bool *identical = nullptr);

 private:
  bool ensureLoaded(const SdFs &fs);
  struct SysIdx {
    std::string name;
    uint32_t offset;
  };
  std::vector<uint8_t> data_;
  std::vector<SysIdx> idx_;
  int64_t mtime_ = -1;
  int64_t size_ = -1;
};
