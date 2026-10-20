// playlist.h -- playlist de GIF : /playlists/<nom> (lignes commencant par '/'), lecture sequentielle ou aleatoire.
// Le firmware derive des fichiers .cache/.idx/.sig pour economiser la RAM ; ici la liste est simplement gardee en
// memoire (rechargee si le fichier change), avec le meme tirage aleatoire : jamais deux fois de suite le meme
// (10 essais), index 0 s'il n'y en a qu'un.
#pragma once
#include <cstdint>
#include <random>
#include <string>
#include <vector>

#include "sdfs.h"

class Playlist {
 public:
  // name vide = aucune playlist. random : tirage aleatoire plutot que sequentiel.
  void configure(const std::string &name, bool random);
  int count(const SdFs &fs);
  std::string next(const SdFs &fs);  // "" si vide
  std::string name() const { return name_; }

 private:
  void reloadIfChanged(const SdFs &fs);
  std::string name_;
  bool random_ = true;
  std::vector<std::string> lines_;
  int64_t mtime_ = -1;
  size_t seq_ = 0;
  int lastRandom_ = -1;
  std::mt19937 rng_{std::random_device{}()};
};
