// COPIE GENEREE par tools/port_widgets.py depuis ../../liquid_floaters.h (firmware ESP32) -- NE PAS EDITER ICI : modifier l'original puis relancer le script.
// ============================================
// safe-modify -- Historique des modifications
// ============================================
// Version actuelle : v2
//
// v2 -- 2026-11-09 -- RENDU VOLUMIQUE (retour utilisateur : les objets sont en volume, la rotation d'un sprite plat ne peut pas donner ce rendu) : format .spr version 2 = serie de VUES autour de l'axe vertical
//                    (plateau tournant, -90 a +90 degres) ; UNE SEULE vue en RAM a la fois (lue a la demande sur la SD quand le cap change de vue), les vues de 90 a 180 degres sont le miroir horizontal des vues 90 a 0.
// v1 -- 2026-11-09 -- Creation (branche dev/liquid-floaters, firmware v418) : chargeur de sprites .spr et echantillonneur du widget Liquide.
//                    Dans un .h (et non dans le .ino) : l'IDE Arduino genere les prototypes de fonctions AVANT les definitions de struct du .ino.
// ============================================
// v418 -- OBJETS FLOTTANTS du widget Liquide : sprites .spr sur la SD (/widgets/liquid/<nom>.spr, voir tools/gen_liquid_sprites.py). Le widget ouvre le fichier UNE FOIS a son lancement et le referme a sa sortie ;
// le tampon RAM ne contient qu'UNE vue (2,5 Ko pour le canard). Fichier absent / invalide / tas insuffisant => le widget tourne sans objet (comportement d'avant).
// Format version 3 : en-tete de 16 octets 'L' 'S' 3 nv | sw sh (texels d'UNE vue) | lw lh (pixels a l'ecran) | draft% | sub% | yaw (couple de girouette, 0 = defaut 3) | pas (degres entre deux vues)
// | plunge% (enfournement max, % de la hauteur) | emerge% | 0 0, puis nv vues de sw*sh*4 octets RGBA, cap de -90 a +90 degres. Stocke en double resolution, tourne (houle) par echantillonnage bilineaire.
// v3 du .h (2026-11-09) : en-tete de 16 octets, bornes d'enfournement / d'emergence PROPRES A CHAQUE OBJET (le canard supporte 50 %, la coque du Benchy 6 % : l'eau passait par-dessus la coque).
#pragma once

// ACCES AUX FICHIERS : sur l'ESP32 ce sont les fichiers de la carte SD (classe File) ; sur le Raspberry Pi (compile avec LQ_RPI, voir rpi_daemon/src/ui/widgets.cpp) ce sont des fichiers stdio dont le chemin reel est resolu par
// le demon (SdFs : chemins insensibles a la casse comme la FAT). Meme API des deux cotes : seek / read / size / close / operator bool.
#ifdef LQ_RPI
#include <cstdio>
struct LqFile {
  FILE *fp = nullptr;
  explicit operator bool() const { return fp != nullptr; }
  bool seek(size_t p) { return fp && std::fseek(fp, (long)p, SEEK_SET) == 0; }
  size_t read(uint8_t *b, size_t n) { return fp ? std::fread(b, 1, n, fp) : 0; }
  size_t size() { if (!fp) return 0; const long c = std::ftell(fp); std::fseek(fp, 0, SEEK_END); const long e = std::ftell(fp); std::fseek(fp, c, SEEK_SET); return (size_t)(e < 0 ? 0 : e); }
  void close() { if (fp) { std::fclose(fp); fp = nullptr; } }
};
LqFile lqOpen(const char *sdPath);   // defini dans widgets.cpp (ecrit d'abord le sprite embarque s'il manque)
#else
typedef File LqFile;
static inline LqFile lqOpen(const char *sdPath) { return SD.open(sdPath, FILE_READ); }
#endif

// reglages du widget (definis dans RecalBox_DMD.ino)
extern int decoLiqObj;     // 1 = avec objet flottant, 0 = sans
extern int decoLiqForce;   // -1 = type de liquide tire au hasard, 0..5 = type force (apercu de la page Widgets seulement)

struct LqSpr {
  uint8_t plunge = 30, emerge = 35;   // bornes de la reponse verticale, en % de la hauteur : enfournement max DANS L'EAU (multiplie par un facteur du liquide : eau 1, sirop 0,2...) / emergence max ; propres a chaque objet
  bool roll = false;                  // l'objet ROULE (ballon) : ses vues sont des angles de ROULIS (24 vues, eclairage fixe), choisies selon la distance parcourue, au lieu d'un cap
  bool wet = false;                   // l'objet garde un FILM de liquide (adherence) selon la viscosite : ballons
  bool swim = false; float swimV = 0.0f;   // l'objet NAGE seul (requin) : vitesse de nage en px / image (octet 15 de l'en-tete / 100) ; demi-tour aux parois, sillage, cycle de remontee / plongee
  uint8_t sw = 0, sh = 0, lw = 0, lh = 0, draft = 30, sub = 25, yaw = 3, nv = 0, step = 15;   // yaw : couple de girouette (x 0,001) : faible (3) = canard, ne se retourne que sur un evenement ; fort (8) = coque
  uint8_t *px = nullptr;   // la vue COURANTE (sw*sh*4 octets, couleurs premultipliees)
  LqFile f; int cur = -1;  // fichier ouvert pendant le widget, indice de la vue en RAM
  bool ok() const { return px != nullptr; }
};
struct LqSprFree { LqSpr *s; ~LqSprFree() { if (s->px) { free(s->px); s->px = nullptr; } if (s->f) s->f.close(); } };

// ETAT COMPLET d'un objet flottant (sprite + position, vitesse, cap, flottaison, film de liquide, rectangle englobant...) : le widget en gere JUSQU'A DEUX (le requin + un flottant tire au hasard).
// Le destructeur libere le tampon et ferme le fichier a la sortie du widget.
struct LqObj {
  LqSpr spr;
  float oU = 64.0f, oVel = 0.0f, oAng = 0.0f, oSuPrev = -1.0f, oFloorW = 0.0f, oLagMax = 0.0f, oLagSum = 0.0f, oSpin = 0.0f, oWetTop = -1.0f, oVo = -1.0f, oVv = 0.0f, oFlowF = 0.0f, oPsi = 0.0f, oOm = 0.0f, oDir = 1.0f;
  float ocx = 0.0f, ocy = 0.0f, sca = 1.0f, ssa = 0.0f, sS = 1.0f, subF = 0.25f, draftF = 0.3f, spx0 = 0.0f, spy0 = 0.0f;
  int oFace = 1, oFloorN = 0, oLagN = 0, oCool = 0, sbx0 = 0, sbx1 = -1, sby0 = 0, sby1 = -1;
  ~LqObj() { if (spr.px) { free(spr.px); spr.px = nullptr; } if (spr.f) spr.f.close(); }
};
static bool lqSetView(LqSpr &s, int k)
{
  if (k < 0) k = 0; if (k >= s.nv) k = s.nv - 1;
  if (k == s.cur) return true;
  const size_t n = (size_t)s.sw * s.sh * 4;
  if (!s.f.seek(16 + (size_t)k * n)) return false;
  if (s.f.read(s.px, n) != n) { s.cur = -1; return false; }
  for (size_t i = 0; i < n; i += 4) { const uint32_t a = s.px[i + 3]; s.px[i] = (uint8_t)((s.px[i] * a + 127) / 255); s.px[i + 1] = (uint8_t)((s.px[i + 1] * a + 127) / 255); s.px[i + 2] = (uint8_t)((s.px[i + 2] * a + 127) / 255); }   // premultiplie
  s.cur = k;
  return true;
}

static bool lqLoadSpr(const char *name, LqSpr &s)
{
  char path[48]; snprintf(path, sizeof(path), "/widgets/liquid/%s.spr", name);
  LqFile f = lqOpen(path);
  if (!f) return false;
  uint8_t hd[16];
  bool ok = (f.read(hd, 16) == 16) && hd[0] == 'L' && hd[1] == 'S' && hd[2] == 3 && hd[3] >= 1 && hd[3] <= 32 && hd[4] >= 8 && hd[4] <= 80 && hd[5] >= 8 && hd[5] <= 80 && hd[6] >= 4 && hd[7] >= 4 && hd[11] >= 1;
  const size_t n = ok ? (size_t)hd[4] * hd[5] * 4 : 0;
  if (ok && (size_t)f.size() != 16 + (size_t)hd[3] * n) ok = false;
  if (ok && ESP.getMaxAllocHeap() < n + 8192) ok = false;   // garde-fou : ne jamais entamer la marge du tas
  uint8_t *px = ok ? (uint8_t *)malloc(n) : nullptr;
  if (!px) { f.close(); return false; }
  s.sw = hd[4]; s.sh = hd[5]; s.lw = hd[6]; s.lh = hd[7]; s.draft = hd[8]; s.sub = hd[9]; s.yaw = hd[10] ? hd[10] : 3; s.nv = hd[3]; s.step = hd[11]; s.plunge = hd[12] ? hd[12] : 30; s.emerge = hd[13] ? hd[13] : 35; s.roll = (hd[14] & 1) != 0; s.wet = (hd[14] & 2) != 0; s.swim = (hd[14] & 4) != 0; s.swimV = hd[15] / 100.0f;
  s.px = px; s.f = f; s.cur = -1;
  if (!lqSetView(s, s.nv / 2)) { free(s.px); s.px = nullptr; s.f.close(); return false; }   // vue de profil (cap 0) au depart
  return true;
}

// echantillon bilineaire de la vue courante (couleurs premultipliees, 0..255 ; o = r, g, b, a) au point (fx, fy) en texels ; hors sprite = transparent
static void lqSample(const LqSpr &s, float fx, float fy, float *o)
{
  fx -= 0.5f; fy -= 0.5f;
  const int x0 = (int)floorf(fx), y0 = (int)floorf(fy); const float tx = fx - x0, ty = fy - y0;
  o[0] = o[1] = o[2] = o[3] = 0.0f;
  for (int j = 0; j < 2; j++) for (int i = 0; i < 2; i++) {
    const int x = x0 + i, y = y0 + j;
    if (x < 0 || y < 0 || x >= s.sw || y >= s.sh) continue;
    const float w = (i ? tx : 1.0f - tx) * (j ? ty : 1.0f - ty);
    const uint8_t *p = s.px + ((size_t)y * s.sw + x) * 4;
    o[0] += w * p[0]; o[1] += w * p[1]; o[2] += w * p[2]; o[3] += w * p[3];
  }
}
