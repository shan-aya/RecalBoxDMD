// ============================================
// boot_seasons.h -- Animations de demarrage SAISONNIERES : table des fetes et calculs de dates (aucune dependance Arduino)
// ============================================
// Version actuelle : v1 (2026-10-22)
//
// v1 - 2026-10-22 - safe-modify - Creation (firmware 3.67). Chaque fete a une FENETRE autour de la date (pas le jour exact) pendant laquelle le DMD joue /gifs/_boot/boot_<nom>.raw565pack
//   au lieu de l'animation par defaut ; hors fenetre, ou si le fichier est absent/invalide, c'est l'animation par defaut. Les fichiers saisonniers sont telecharges (mode mise a jour) seulement
//   BOOT_SEASON_LEAD_DAYS jours avant le debut de la fenetre. Fenetres : Halloween 15/10-02/11, Noel 10/12-26/12, Nouvel An 27/12-03/01 (chevauche le 31/12), Saint-Valentin 07/02-14/02,
//   Paques = dimanche de Paques -7 jours a +1 jour (date calculee chaque annee). Dates sans lien avec le DMD : ce fichier se compile et se teste sur n'importe quel PC.
#pragma once
#include <stdint.h>

struct BootSeason {
  const char *name;      // sert au nom du fichier : boot_<name>.raw565pack, et aux cles du manifeste : boot_<name>_ver / _size / _sha256
  uint16_t startMD;      // MMDD de debut (0 = fete mobile : Paques)
  uint16_t endMD;        // MMDD de fin ; < startMD = la fenetre chevauche le 31/12 (fin l'annee suivante)
  int8_t easterFrom;     // Paques : jours relatifs au dimanche de Paques
  int8_t easterTo;
};

#define BOOT_SEASON_COUNT 5
#define BOOT_SEASON_LEAD_DAYS 21     // un fichier saisonnier est recupere au plus tot 21 jours avant sa fenetre
static const BootSeason kBootSeasons[BOOT_SEASON_COUNT] = {
  {"halloween", 1015, 1102,  0, 0},
  {"noel",      1210, 1226,  0, 0},
  {"nouvelan",  1227,  103,  0, 0},
  {"valentin",   207,  214,  0, 0},
  {"paques",       0,    0, -7, 1},
};

// Jours ecoules depuis 1970-01-01 (algorithme « days_from_civil » de Howard Hinnant, domaine public).
static inline int32_t bsDays(int y, int m, int d)
{
  y -= (m <= 2);
  const int era = (y >= 0 ? y : y - 399) / 400;
  const unsigned yoe = (unsigned)(y - era * 400);
  const unsigned doy = (unsigned)((153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1);
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + (int32_t)doe - 719468;
}

// Dimanche de Paques de l'annee y (calendrier gregorien, algorithme de Meeus/Jones/Butcher), en jours depuis 1970-01-01.
static inline int32_t bsEaster(int y)
{
  const int a = y % 19, b = y / 100, c = y % 100, d = b / 4, e = b % 4, f = (b + 8) / 25, g = (b - f + 1) / 3;
  const int h = (19 * a + b - d - g + 15) % 30, i = c / 4, k = c % 4, l = (32 + 2 * e + 2 * i - h - k) % 7, m = (a + 11 * h + 22 * l) / 451;
  const int month = (h + l - 7 * m + 114) / 31, day = ((h + l - 7 * m + 114) % 31) + 1;
  return bsDays(y, month, day);
}

// AAAAMMJJ -> y/m/d ; false si la date est impossible (0, hors 2000-2099, mois ou jour hors limites).
static inline bool bsSplit(uint32_t ymd, int &y, int &m, int &d)
{
  y = (int)(ymd / 10000); m = (int)((ymd / 100) % 100); d = (int)(ymd % 100);
  return y >= 2000 && y <= 2099 && m >= 1 && m <= 12 && d >= 1 && d <= 31;
}

// La date ymd tombe-t-elle dans la fenetre de la fete s, elargie de leadDays AVANT son debut ?
static inline bool bsInWindow(const BootSeason &s, uint32_t ymd, int leadDays)
{
  int ty, tm, td;
  if (!bsSplit(ymd, ty, tm, td)) return false;
  const int32_t today = bsDays(ty, tm, td);
  for (int dy = -1; dy <= 0; dy++) {              // fenetre commencee l'annee precedente (chevauchement du 31/12) ou cette annee
    const int y = ty + dy;
    int32_t st, en;
    if (s.startMD == 0) { const int32_t e = bsEaster(y); st = e + s.easterFrom; en = e + s.easterTo; }
    else {
      st = bsDays(y, s.startMD / 100, s.startMD % 100);
      en = bsDays(y + (s.endMD < s.startMD ? 1 : 0), s.endMD / 100, s.endMD % 100);
    }
    if (today >= st - leadDays && today <= en) return true;
  }
  return false;
}

// Indice de la fete EN COURS a la date ymd (-1 = aucune : animation par defaut).
static inline int bsSeasonAt(uint32_t ymd)
{
  for (int i = 0; i < BOOT_SEASON_COUNT; i++) if (bsInWindow(kBootSeasons[i], ymd, 0)) return i;
  return -1;
}

// Masque des fetes dont le fichier doit etre disponible a la date ymd : en cours ou commencant dans les BOOT_SEASON_LEAD_DAYS jours (bit i = fete i).
static inline uint8_t bsWantedMask(uint32_t ymd)
{
  uint8_t mask = 0;
  for (int i = 0; i < BOOT_SEASON_COUNT; i++) if (bsInWindow(kBootSeasons[i], ymd, BOOT_SEASON_LEAD_DAYS)) mask |= (uint8_t)(1u << i);
  return mask;
}
