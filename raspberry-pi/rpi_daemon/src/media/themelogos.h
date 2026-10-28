// themelogos.h -- logos systeme du THEME de la Recalbox (portage de themeSelect / openBestMedia / defaultsLangPath du firmware, v233-v247).
//
// Sur la SD : /systems/_defaults/_themes/<theme>/<systeme>.raw565 (logos de base, US / anglais) et, seulement pour les logos qui
// different, <theme>/l_<langue>/ et <theme>/r_<region>/ ; chaque dossier a un _index.bin (hash FNV-1a 32 bits tries, 4 octets par logo)
// qui evite d'ouvrir un fichier absent. Ordre de recherche d'un logo : langue, region, base -- c'est ce que la Recalbox affiche.
// Images par defaut traduites : /systems/_defaults/<langue>/<nom>.raw565 (fr, es...), cherchees avant /systems/_defaults/<nom>.raw565.
// Un index absent veut dire « on ne sait pas » : on essaie.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "config.h"
#include "sdfs.h"

class ThemeLogos {
 public:
  // feat_theme_follow (defaut 1), feat_lang_priority (defaut 0) ; theme_last / lang_last / region_last = derniere selection memorisee
  void begin(const Config &cfg, const SdFs &fs);

  // Memorise ce que la Recalbox veut et applique. Retourne true si la selection EFFECTIVE (theme|langue|region) a change.
  bool setWanted(const SdFs &fs, const std::string &theme, const std::string &lang, const std::string &region);
  void setLangPriority(bool on) { langPriority_ = on; }
  // v398 -- LOGOS REELS (dossier _themes/logos-reels : base monde + r_us / r_eu / r_jp), passent avant la region et la base du theme.
  // mode : 0 = desactive, 1 = tous les themes, 2 = seulement les themes coches (checked = « ,a,b, »). Retourne true si la selection EFFECTIVE a change.
  bool setReels(const SdFs &fs, int mode, bool autoNew, const std::string &checked);
  // Theme annonce par la Recalbox : l'ajoute a la liste « vus » (et le coche si autoNew). Retourne true si seen / checked ont change (a ecrire dans config.ini).
  bool noteSeen(const std::string &theme);
  const std::string &reelsSeen() const { return reelsSeen_; }
  const std::string &reelsChecked() const { return reelsChecked_; }
  // Option « suivi du theme » changee (page web) : reapplique.
  bool setFollow(const SdFs &fs, bool follow);
  // v3.74 -- des logos reels viennent d'etre envoyes (toolkit, Wi-Fi) : recharge les index / dossiers meme si la selection n'a pas change.
  bool refresh(const SdFs &fs) { requested_.clear(); return apply(fs); }

  const std::string &wantedTheme() const { return wTheme_; }
  const std::string &wantedLang() const { return wLang_; }
  const std::string &wantedRegion() const { return wRegion_; }
  bool takePersist();  // true une fois si theme / langue / region different de ce qui est dans config.ini (a ecrire)

  // Fichiers a essayer, dans l'ordre, pour le logo du systeme (vide = pas de theme actif ou logo absent du theme).
  std::vector<std::string> candidates(const std::string &sys) const;
  // Traduction de /systems/_defaults/<nom>.raw565 : chemin /systems/_defaults/<langue>/<nom>.raw565, ou "" si non applicable
  std::string defaultsLangPath(const std::string &rawPath) const;

 private:
  struct Idx {
    std::vector<uint32_t> v;
    bool loaded = false;  // false = pas d'index : tout est « peut-etre present »
    bool has(const std::string &name) const;
  };
  static uint32_t hash(const std::string &s);
  static bool loadIdx(const SdFs &fs, const std::string &path, Idx &out);
  bool apply(const SdFs &fs);
  void defaultsLangSelect(const SdFs &fs, const std::string &lang);
  bool anyLogo(const std::string &n) const;
  static bool listHas(const std::string &list, const std::string &name);
  bool reelsWanted() const;  // le theme annonce doit-il utiliser les logos reels ?

  bool follow_ = true, langPriority_ = false;
  std::string wTheme_, wLang_, wRegion_;
  std::string pTheme_, pLang_, pRegion_;  // derniere selection ecrite dans config.ini
  std::string requested_;                 // theme|langue|region deja applique (evite de recharger a chaque HELLO)

  std::string logoTheme_;                  // g_logoTheme : theme actif ("" = logos par defaut)
  Idx themeIdx_, langIdx_, regionIdx_;
  int reelsMode_ = 0;  // v398
  bool reelsAutoNew_ = false;
  std::string reelsChecked_, reelsSeen_;  // listes « ,a,b, »
  std::string reelsDir_, reelsRegionDir_;
  Idx reelsIdx_, reelsRegionIdx_;
  std::string langDir_, regionDir_;        // variantes presentes sur la SD

  std::string defLang_, defLangRequested_;  // langue des images par defaut
  bool defLangDirOk_ = false;
  Idx defLangIdx_;
};
