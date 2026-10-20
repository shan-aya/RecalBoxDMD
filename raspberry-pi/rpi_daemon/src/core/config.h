// config.h -- lecture de /config.ini (meme grammaire que loadConfig() du firmware) et ecriture de cles
// (writeConfigFlags : on ne touche que les lignes concernees, le reste du fichier est conserve).
#pragma once
#include <atomic>
#include <map>
#include <mutex>
#include <string>

#include "sdfs.h"

class Config {
 public:
  // Lit /config.ini. Absent = valeurs par defaut du firmware, sans erreur.
  void load(const SdFs &fs);

  // Cles brutes (minuscules), valeur apres retrait des commentaires en ligne.
  std::string get(const std::string &key, const std::string &def = "") const;
  bool has(const std::string &key) const {
    std::lock_guard<std::mutex> lk(mu_);
    return kv_.count(key) > 0;
  }
  bool getBool(const std::string &key, bool def) const;  // "!= 0" comme le firmware (sauf cles « == 1 »)
  int getInt(const std::string &key, int def) const;

  // --- valeurs typees (defauts = ceux de RecalBox_DMD.ino) ---
  std::string playlistName;     // "playlist="  (vide = aucune)
  bool playlistRandom = true;   // "random="    (!= "0")
  int screenBrightness = 120;   // 0..255, "brightness=" 0..100 converti par map(x,0,100,0,255)
  std::atomic<int> schedBrightness{-1};  // plage du planificateur de luminosite en vigueur (0..255), -1 = aucune
  uint8_t brightness8() const { const int s = schedBrightness.load(); return (uint8_t)(s >= 0 ? s : screenBrightness); }  // luminosite de BASE a reappliquer
  bool showInfo = true;         // "info="
  std::string uiLanguage = "fr";

  // Recalcule les valeurs typees (playlist, random, info, luminosite, langue) depuis les cles : apres load(), ou apres un enregistrement web.
  void refreshTyped();

  // Ecrit (ou ajoute) key=value dans config.ini, de facon atomique (fichier temporaire puis rename).
  bool writeKeys(const SdFs &fs, const std::map<std::string, std::string> &kv);

 private:
  mutable std::mutex mu_;  // la page web (threads HTTP) et le thread reseau lisent pendant que la boucle principale ecrit
  std::map<std::string, std::string> kv_;
};
