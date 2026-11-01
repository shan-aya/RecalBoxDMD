// ============================================
// playlist_seasons.h -- Playlists SAISONNIERES : regles de planification annuelle, chevauchements, choix (aucune dependance Arduino)
// ============================================
// Version actuelle : v1 (2026-11-01)
//
// v1 - 2026-11-01 - safe-modify - Creation. Une regle = une fenetre annuelle recurrente (fete du calendrier de boot_seasons.h, dates surchargees, ou plage libre MMDD->MMDD)
//   + une playlist + un mode : EXCLUSIVE (remplace la playlist par defaut pendant la fenetre) ou MIXEE (s'ajoute a la playlist par defaut). Stockage : /playlists/seasons.ini,
//   une regle par ligne :  fete=noel|start=1210|end=1226|playlist=Noel.txt|mode=excl   (fete= vide = plage libre ; start/end absents = dates de la fete).
//   Chevauchement : deux regles se recouvrent si leurs fenetres ont un jour commun UNE annee quelconque des PS_OVERLAP_YEARS prochaines (Paques bouge) ; BLOQUANT si les deux
//   sont exclusives (le firmware/toolkit refuse d'enregistrer), simple AVERTISSEMENT si l'une est mixee. Choix a l'execution : exclusive active (fenetre la plus courte, puis la derniere
//   du fichier) sinon mixees actives sinon playlist par defaut. Aucune allocation dynamique ; se compile et se teste sur n'importe quel PC. Depend de boot_seasons.h.
#pragma once
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "boot_seasons.h"

#define PS_MAX_RULES      8
#define PS_PLAYLIST_LEN   64
#define PS_OVERLAP_YEARS  40      // nombre d'annees examinees pour la detection de chevauchement (couvre le cycle de Paques en pratique)

struct SeasonRule {
  int8_t   fete;                  // indice dans kBootSeasons[], -1 = plage libre
  uint16_t startMD, endMD;        // 0 = reprendre ceux de la fete
  uint8_t  mix;                   // 0 = exclusive, 1 = mixee avec la playlist par defaut
  char     playlist[PS_PLAYLIST_LEN];   // nom du fichier dans /playlists (ex. "Noel.txt")
};

// Fenetre effective d'une regle sous forme de BootSeason (reutilise bsInWindow/bsEaster). Regle sans fete ni dates valides : fenetre vide (startMD=1,endMD=0 impossible -> jamais active).
static inline BootSeason psWindow(const SeasonRule &r)
{
  BootSeason w = {"", 0, 0, 0, 0};
  if (r.fete >= 0 && r.fete < BOOT_SEASON_COUNT) {
    w = kBootSeasons[r.fete];
    if (r.startMD && r.endMD) { w.startMD = r.startMD; w.endMD = r.endMD; w.easterFrom = w.easterTo = 0; }   // dates surchargees
    return w;
  }
  w.startMD = r.startMD; w.endMD = r.endMD;
  return w;
}

static inline bool psValidMD(uint16_t md) { const int m = md / 100, d = md % 100; return m >= 1 && m <= 12 && d >= 1 && d <= 31; }

static inline bool psRuleValid(const SeasonRule &r)
{
  if (!r.playlist[0]) return false;
  if (r.fete >= BOOT_SEASON_COUNT) return false;
  if (r.fete < 0) return psValidMD(r.startMD) && psValidMD(r.endMD);
  if (r.startMD || r.endMD) return psValidMD(r.startMD) && psValidMD(r.endMD);
  return true;
}

// Cette regle est-elle active a la date ymd (AAAAMMJJ) ?
static inline bool psRuleActive(const SeasonRule &r, uint32_t ymd)
{
  if (!psRuleValid(r)) return false;
  const BootSeason w = psWindow(r);
  if (w.startMD == 0 && !(r.fete >= 0)) return false;
  return bsInWindow(w, ymd, 0);
}

// Intervalle [st,en] (jours depuis 1970) de la fenetre w commencant l'annee y.
static inline void psSpan(const BootSeason &w, int y, int32_t &st, int32_t &en)
{
  if (w.startMD == 0) { const int32_t e = bsEaster(y); st = e + w.easterFrom; en = e + w.easterTo; }
  else {
    st = bsDays(y, w.startMD / 100, w.startMD % 100);
    en = bsDays(y + (w.endMD < w.startMD ? 1 : 0), w.endMD / 100, w.endMD % 100);
  }
}

// Les fenetres de a et b ont-elles un jour commun une annee quelconque des PS_OVERLAP_YEARS a partir de fromYear ?
static inline bool psOverlap(const SeasonRule &a, const SeasonRule &b, int fromYear)
{
  if (!psRuleValid(a) || !psRuleValid(b)) return false;
  const BootSeason wa = psWindow(a), wb = psWindow(b);
  for (int y = fromYear; y < fromYear + PS_OVERLAP_YEARS; y++) {
    int32_t sa, ea;
    psSpan(wa, y, sa, ea);
    for (int dy = -1; dy <= 1; dy++) {              // fenetre de b commencee l'annee precedente / suivante (31/12)
      int32_t sb, eb;
      psSpan(wb, y + dy, sb, eb);
      if (sa <= eb && sb <= ea) return true;
    }
  }
  return false;
}

// Gravite du chevauchement : 0 = aucun, 1 = avertissement (une des deux mixee), 2 = BLOQUANT (deux exclusives).
static inline int psConflict(const SeasonRule &a, const SeasonRule &b, int fromYear)
{
  if (!psOverlap(a, b, fromYear)) return 0;
  return (!a.mix && !b.mix) ? 2 : 1;
}

// Pire gravite de la regle idx contre toutes les autres (n regles). other = indice de la premiere regle en conflit de cette gravite (-1 si aucune).
static inline int psWorstConflict(const SeasonRule *rules, int n, int idx, int fromYear, int &other)
{
  int worst = 0; other = -1;
  for (int i = 0; i < n; i++) {
    if (i == idx) continue;
    const int c = psConflict(rules[idx], rules[i], fromYear);
    if (c > worst) { worst = c; other = i; }
  }
  return worst;
}

// Nombre de paires bloquantes dans la liste (0 = enregistrement autorise).
static inline int psBlockingPairs(const SeasonRule *rules, int n, int fromYear)
{
  int cnt = 0;
  for (int i = 0; i < n; i++) for (int j = i + 1; j < n; j++) if (psConflict(rules[i], rules[j], fromYear) == 2) cnt++;
  return cnt;
}

// Duree (jours) de la fenetre de r l'annee y : sert a departager deux exclusives actives (la plus courte, donc la plus precise, gagne).
static inline int32_t psSpanLen(const SeasonRule &r, int y)
{
  int32_t st, en; psSpan(psWindow(r), y, st, en); return en - st;
}

struct PsChoice {
  int8_t  excl;                   // indice de la regle exclusive retenue, -1 = aucune
  uint8_t mixMask;                // bit i = regle i mixee active (ignore si excl >= 0)
};

// Choix a la date ymd. Exclusive active : elle seule (priorite fenetre la plus courte, a egalite la derniere du fichier). Sinon les mixees actives s'ajoutent a la playlist par defaut.
static inline PsChoice psChoose(const SeasonRule *rules, int n, uint32_t ymd)
{
  PsChoice c = {-1, 0};
  int32_t best = 0x7fffffff;
  const int y = (int)(ymd / 10000);
  for (int i = 0; i < n && i < PS_MAX_RULES; i++) {
    if (!psRuleActive(rules[i], ymd)) continue;
    if (rules[i].mix) { c.mixMask |= (uint8_t)(1u << i); continue; }
    const int32_t len = psSpanLen(rules[i], y);
    if (len <= best) { best = len; c.excl = (int8_t)i; }    // <= : a egalite, la derniere gagne
  }
  if (c.excl >= 0) c.mixMask = 0;
  return c;
}

// ---- fichier seasons.ini ----
// Cle=valeur separes par '|'. Retourne false si la ligne est vide, commentee (# ou ;) ou sans playlist.
static inline bool psParseLine(const char *line, SeasonRule &r)
{
  memset(&r, 0, sizeof(r)); r.fete = -1;
  while (*line == ' ' || *line == '\t') line++;
  if (!*line || *line == '#' || *line == ';') return false;
  const char *p = line;
  while (*p) {
    const char *e = p; while (*e && *e != '|' && *e != '\r' && *e != '\n') e++;
    const char *eq = p; while (eq < e && *eq != '=') eq++;
    if (eq < e) {
      const size_t kl = (size_t)(eq - p), vl = (size_t)(e - eq - 1);
      char v[PS_PLAYLIST_LEN]; const size_t cl = vl < sizeof(v) - 1 ? vl : sizeof(v) - 1; memcpy(v, eq + 1, cl); v[cl] = 0;
      if (kl == 4 && !strncmp(p, "fete", 4)) {
        for (int i = 0; i < BOOT_SEASON_COUNT; i++) if (!strcmp(v, kBootSeasons[i].name)) r.fete = (int8_t)i;
      }
      else if (kl == 5 && !strncmp(p, "start", 5)) r.startMD = (uint16_t)atoi(v);
      else if (kl == 3 && !strncmp(p, "end", 3))   r.endMD   = (uint16_t)atoi(v);
      else if (kl == 8 && !strncmp(p, "playlist", 8)) { strncpy(r.playlist, v, PS_PLAYLIST_LEN - 1); r.playlist[PS_PLAYLIST_LEN - 1] = 0; }
      else if (kl == 4 && !strncmp(p, "mode", 4)) r.mix = (!strcmp(v, "mix")) ? 1 : 0;
    }
    p = e; while (*p == '|') p++;
    if (*p == '\r' || *p == '\n') break;
  }
  return r.playlist[0] != 0;
}

// Ecrit la regle sous forme de ligne (sans '\n'). Retourne la longueur ecrite, 0 si le tampon est trop petit.
static inline size_t psFormatLine(const SeasonRule &r, char *buf, size_t n)
{
  const char *fn = (r.fete >= 0 && r.fete < BOOT_SEASON_COUNT) ? kBootSeasons[r.fete].name : "";
  int k;
  if (r.startMD || r.endMD) k = snprintf(buf, n, "fete=%s|start=%04u|end=%04u|playlist=%s|mode=%s", fn, (unsigned)r.startMD, (unsigned)r.endMD, r.playlist, r.mix ? "mix" : "excl");
  else                      k = snprintf(buf, n, "fete=%s|playlist=%s|mode=%s", fn, r.playlist, r.mix ? "mix" : "excl");
  return (k > 0 && (size_t)k < n) ? (size_t)k : 0;
}
