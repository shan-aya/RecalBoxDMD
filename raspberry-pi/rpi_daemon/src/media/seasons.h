// seasons.h -- playlists SAISONNIERES du demon (portage de seasonChoosePlaylist / seasonBuildMix du firmware ESP32 4.01).
// Regles dans /playlists/seasons.ini (voir playlist_seasons.h, copie IDENTIQUE de celle du firmware) : une EXCLUSIVE active remplace la playlist par defaut,
// des MIXEES actives s'y ajoutent (fichier genere /playlists/_saison_mix.txt, que Playlist lit comme n'importe quelle playlist).
#pragma once
#include <cstdint>
#include <string>

#include "config.h"
#include "playlist_seasons.h"
#include "sdfs.h"

namespace seasons {

#define SEASON_MIX_FILE "_saison_mix.txt"

// Date du jour AAAAMMJJ : fake_date= (essais), puis horloge systeme si elle est valide (NTP / RTC), puis last_date= ; 0 = inconnue.
uint32_t todayYmd(const Config &cfg);
// Annee en cours (2026 si l'horloge n'est pas valide) : point de depart de la detection de chevauchement.
int yearNow();
// Regles de /playlists/seasons.ini (lignes valides, maxN au plus) ; 0 si le fichier est absent.
int loadRules(const SdFs &fs, SeasonRule *rules, int maxN);

struct Pick {
  std::string file;  // playlist a lire (« ALL.txt », « _saison_mix.txt »), "" = playlist par defaut (cfg.playlistName)
  std::string key;   // "" = defaut, "x:<fichier>" = exclusive, "m:<masque>" = mixees ; sert a detecter un changement de choix
  uint32_t ymd = 0;  // date utilisee (0 = aucune regle ou date inconnue)
  std::string add;   // mixte : noms des playlists saisonnieres ajoutees, sans extension, separes par « + » (ecran de playlist) ; vide sinon
};
// Choix a la date du jour. quiet : pas de journal (controle periodique). Genere le fichier mixte si besoin.
Pick choose(const SdFs &fs, const Config &cfg, bool quiet);

}  // namespace seasons
