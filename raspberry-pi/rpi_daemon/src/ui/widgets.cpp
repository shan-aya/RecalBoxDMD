// widgets.cpp -- voir widgets.h. Le code d'affichage est celui du firmware (widgets_fw.inc, genere) compile derriere compat/Arduino.h.
#include "widgets.h"

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <thread>

#include "log.h"

// Le code du firmware n'est pas ecrit pour nos options de compilation : on coupe ses avertissements, pas les notres.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#pragma GCC diagnostic ignored "-Wparentheses"
#pragma GCC diagnostic ignored "-Wtype-limits"
#pragma GCC diagnostic ignored "-Wformat-truncation"
#pragma GCC diagnostic ignored "-Wdangling-else"

#include <filesystem>

#include "Arduino.h"
#include "TomThumb.h"
#include "sdfs.h"
#include "widget_bg_data.h"   // fonds d'image des widgets embarques dans le firmware (ecrits sur la SD s'ils manquent)

// avance reelle (xAdvance) d'un caractere TomThumb ; 4 px hors de la plage 0x20-0x7E (meme fonction que score.cpp)
static uint8_t tomThumbCharAdvance(char c) {
  const uint8_t uc = (uint8_t)c;
  if (uc < 0x20 || uc > 0x7E) return 4;
  return TomThumbGlyphs[uc - 0x20].xAdvance;
}

// ----------------------------------------------------------------------------------------------------------------- etat (RecalBox_DMD.ino)
extern Display *display;                     // defini dans clock.cpp (la « matrice » du firmware)
extern int32_t clockAnimCol[5], clockDigitCol[5];
extern uint16_t g_clkColorVer;
extern bool clockNeonCustomColor;             // globaux attendus par clock_themes.h (definis dans clock.cpp)
extern uint8_t clockNeonR, clockNeonG, clockNeonB;

String uiLanguage = "fr";
uint8_t decoDur[11] = {60, 10, 15, 10, 60, 15, 8, 1, 15, 30, 15};   // 0 horloge, 1 date, 2 ephemeride, 3 compte a rebours, 4 ambiance, 5 meteo, 6 stats, 7 citations, 8-10 Recalbox
int32_t decoAmbCol[9][2] = {{-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}};
float decoLat = 0.0f, decoLon = 0.0f;        // position de l'ephemeride (0,0 = non definie)
String decoPlace = "";
#define DECO_EV_COUNT 10
String decoEvName[DECO_EV_COUNT];
uint32_t decoEvDate[DECO_EV_COUNT] = {0};
uint8_t decoEvYearly[DECO_EV_COUNT] = {0};
uint8_t decoEvOn[DECO_EV_COUNT] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
uint8_t decoAmbStyle = 0;                    // 0 = aleatoire, 1 plasma, 2 pluie, 3 etoiles, 4 vie, 5 liquide, 6 sable, 7 sable 2, 8 lava lamp
String decoQuoteTheme = "";
int g_skyPass = 0;                           // numero de bande du ciel (une seule ici : toujours 0)

namespace {
std::function<bool()> g_cmdPending;
std::function<void()> g_service;
uint64_t g_lastFlushMs = 0;

// Presente l'image en cours (le firmware dessine directement sur la matrice DMA ; ici il faut un show()). Au plus un tous les 4 ms.
void flushDisplay() {
  const uint64_t n = nowMs();
  if (display && n - g_lastFlushMs >= 4) {
    g_lastFlushMs = n;
    display->show();
  }
}
}  // namespace

void delay(unsigned long ms) {
  flushDisplay();
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

#include "clock_themes.h"    // (copie corrigee, voir CMakeLists.txt) : police des chiffres utilisee par clock_ambient.inc
#include "clock_ambient.inc"   // plasma, lave... (deja utilises par les themes d'horloge ; static, donc sans conflit avec clock.cpp)

// Interruption commune des widgets : true = il faut s'arreter (une commande attend dans la file). Entretient le reseau (HELLO / PING).
static bool decoStop(bool preview) {
  flushDisplay();
  if (g_service) g_service();
  if (g_cmdPending && g_cmdPending()) return true;
  return false;
}
static bool decoWait(unsigned long durMs, bool preview) {
  const unsigned long t0 = millis();
  while (millis() - t0 < durMs) {
    if (decoStop(preview)) return false;
    delay(20);
  }
  return true;
}

static const uint16_t *g_skyBgImg = nullptr;   // fond image des widgets Recalbox (a la place du ciel)
static const SdFs *g_fs = nullptr;
static const int RAW565_W = 128, RAW565_H = 32;
static uint16_t g_rawBuf[128 * 32];             // raw565FullBuf du firmware : tampon d'une image 128 x 32 (fond de citation / de widget)
static uint16_t *raw565FullBuf = g_rawBuf;
static bool decoQuoteBgTry(const char *name);   // definis plus bas (SD)
static bool decoBgEnsure(const char *file);

#include "widgets_fw.inc"

// ----------------------------------------------------------------------------------------------------------------- SD : citations et fonds d'image
// (le firmware lit / ecrit la SD avec l'API Arduino ; ici SdFs, chemins insensibles a la casse comme la FAT)
static bool sdRead(const std::string &p, std::vector<uint8_t> &out) { return g_fs && g_fs->readAll(p, out); }
static bool sdWrite(const std::string &p, const uint8_t *d, size_t n) {
  if (!g_fs) return false;
  const std::string full = g_fs->root() + p;
  std::error_code ec;
  std::filesystem::create_directories(std::filesystem::path(full).parent_path(), ec);
  FILE *f = std::fopen(full.c_str(), "wb");
  if (!f) return false;
  const bool ok = std::fwrite(d, 1, n, f) == n;
  std::fclose(f);
  return ok;
}

// nom de theme sur : minuscules, [a-z0-9_-], espaces -> '-', 24 caracteres max (sert de nom de fichier) ; listes « a,b,c » (24 au maximum)
static String quoteSlug(const String &in) {
  String o;
  for (unsigned i = 0; i < in.length() && o.length() < 24; i++) {
    char c = in[i];
    if (c >= 'A' && c <= 'Z') c += 32;
    if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_') o += c;
    else if (c == ' ') o += '-';
  }
  return o;
}
static String quoteSlugList(const String &in) {
  String out;
  int start = 0, cnt = 0;
  while (start <= (int)in.length() && cnt < 24) {
    const int c = in.indexOf(',', start);
    String tok = quoteSlug((c < 0) ? in.substring(start) : in.substring(start, c));
    if (tok.length()) {
      if (out.length()) out += ",";
      out += tok;
      cnt++;
    }
    if (c < 0) break;
    start = c + 1;
  }
  return out;
}
static bool quoteListHas(const String &list, const char *name) {
  int start = 0;
  while (start <= (int)list.length()) {
    const int c = list.indexOf(',', start);
    const String tok = (c < 0) ? list.substring(start) : list.substring(start, c);
    if (tok == name) return true;
    if (c < 0) break;
    start = c + 1;
  }
  return false;
}

// ecrit /widgets/quotes/bg/<fichier> depuis le firmware s'il est ABSENT de la SD (jamais d'ecrasement) ; true si le fichier existe ensuite
static bool decoBgEnsure(const char *file) {
  char path[72];
  snprintf(path, sizeof(path), "/widgets/quotes/bg/%s", file);
  if (g_fs && g_fs->exists(path)) return true;
  for (unsigned i = 0; i < sizeof(DECO_BG_ASSETS) / sizeof(DECO_BG_ASSETS[0]); i++) {
    if (strcmp(DECO_BG_ASSETS[i].file, file) != 0) continue;
    const bool ok = sdWrite(path, DECO_BG_ASSETS[i].data, DECO_BG_ASSETS[i].len);
    Serial.println(String("[DECO] fond ") + file + (ok ? " ecrit sur la SD (embarque dans le firmware)" : " : ECHEC d'ecriture"));
    return ok;
  }
  return false;
}

// themes d'origine de la langue : ecrase les fichiers du meme nom ; retourne le nombre de fichiers ecrits
static int decoQuotesRestore(const String &lang) {
  if (!decoQuoteLangOk(lang)) return -1;
  int n = 0;
  for (unsigned i = 0; i < sizeof(DECO_QUOTE_SETS) / sizeof(DECO_QUOTE_SETS[0]); i++) {
    if (lang != DECO_QUOTE_SETS[i].lang) continue;
    const String p = "/widgets/quotes/" + lang + "/" + DECO_QUOTE_SETS[i].name + ".txt";
    if (sdWrite(p.c_str(), (const uint8_t *)DECO_QUOTE_SETS[i].data, strlen(DECO_QUOTE_SETS[i].data))) n++;
  }
  Serial.println("[QUOTES] themes d'origine restaures (" + lang + ") : " + String(n));
  return n;
}

// liste des themes (noms sans .txt) de la langue : nombre (max `maxN`), noms dans names[i] (24 car. max)
static int decoQuoteList(const String &lang, char names[][26], int *sizes, int maxN) {
  int n = 0;
  if (!g_fs) return 0;
  const std::string d = std::string("/widgets/quotes/") + lang.c_str();
  for (const std::string &e : g_fs->list(d)) {
    if (n >= maxN) break;
    if (e.size() > 4 && e.size() <= 29 && e.compare(e.size() - 4, 4, ".txt") == 0) {
      const std::string nm = e.substr(0, e.size() - 4);
      strncpy(names[n], nm.c_str(), 25);
      names[n][25] = 0;
      sizes[n] = (int)g_fs->fileSize(d + "/" + e);
      n++;
    }
  }
  return n;
}

// tire une citation au hasard : text (<= 239 car.) et suite (<= 79 car.) ; false s'il n'y a rien a afficher
static bool decoQuotePick(char *text, size_t tn, char *suite, size_t sn) {
  const String lang = decoQuoteLangOk(uiLanguage) ? uiLanguage : String("fr");
  char names[24][26];
  int sizes[24];
  int n = decoQuoteList(lang, names, sizes, 24);
  if (n == 0) { decoQuotesRestore(lang); n = decoQuoteList(lang, names, sizes, 24); }   // 1er lancement : themes d'origine
  if (n == 0) return false;
  int cand[24], nc = 0;   // themes coches (liste separee par des virgules) ; aucun valide = tous
  if (decoQuoteTheme.length()) for (int i = 0; i < n; i++) if (quoteListHas(decoQuoteTheme, names[i])) cand[nc++] = i;
  const int t = (nc > 0) ? cand[random(0, nc)] : (int)random(0, n);   // plusieurs coches : tirage au hasard entre eux
  g_quoteQuiz = (strstr(names[t], "quiz") != nullptr);
  strncpy(g_quoteThemeName, names[t], sizeof(g_quoteThemeName) - 1);
  g_quoteThemeName[sizeof(g_quoteThemeName) - 1] = 0;   // pour le fond
  const String path = "/widgets/quotes/" + lang + "/" + names[t] + ".txt";
  std::vector<uint8_t> data;
  if (!sdRead(path.c_str(), data)) return false;
  const uint32_t sz = (uint32_t)data.size();
  char line[330];
  bool ok = false;
  for (int attempt = 0; attempt < 6 && !ok && sz > 0; attempt++) {
    uint32_t pos = (sz > 1) ? (uint32_t)random(0, (long)sz) : 0;   // une ligne par saut aleatoire : rien n'est charge d'avance
    if (pos > 0) { while (pos < sz && data[pos++] != '\n') {} }    // fin de la ligne coupee
    if (pos >= sz) pos = 0;                                         // fin du fichier : retour au debut
    size_t l = 0;
    while (pos < sz) {
      const int ch = data[pos++];
      if (ch == '\n') break;
      if (ch != '\r' && l < sizeof(line) - 1) line[l++] = (char)ch;
    }
    line[l] = 0;
    char *p = line;
    while (*p == ' ') p++;
    if (*p == 0 || *p == '#') continue;
    decoAscii(p);
    char *bar = strstr(p, " | ");
    suite[0] = 0;
    if (bar) { *bar = 0; strncpy(suite, bar + 3, sn - 1); suite[sn - 1] = 0; }
    strncpy(text, p, tn - 1);
    text[tn - 1] = 0;
    ok = (text[0] != 0);
  }
  return ok;
}

// fond d'une citation : <fond>.raw565 (8 192 octets) lu dans raw565FullBuf, puis <fond>.col (couleurs, ombre, luminosite, fenetre de texte)
static bool decoQuoteBgTry(const char *name) {
  char path[64];
  snprintf(path, sizeof(path), "/widgets/quotes/bg/%s.raw565", name);
  { char fn[40]; snprintf(fn, sizeof(fn), "%s.raw565", name); decoBgEnsure(fn); }   // fond d'origine absent de la SD -> ecrit depuis le firmware
  const size_t total = (size_t)RAW565_W * RAW565_H * sizeof(uint16_t);
  std::vector<uint8_t> raw;
  bool ok = false;
  if (sdRead(path, raw) && raw.size() == total) { memcpy(raw565FullBuf, raw.data(), total); ok = true; }
  if (ok) {
    g_qBg = raw565FullBuf;
    // couleurs propres au fond : <fond>.col = « RRGGBB RRGGBB [RRGGBB|-] [luminosite] [x0 x1 y0 y1] » (question, reponse, ombre ; « - » = pas d'ombre)
    snprintf(path, sizeof(path), "/widgets/quotes/bg/%s.col", name);
    { char fn[40]; snprintf(fn, sizeof(fn), "%s.col", name); decoBgEnsure(fn); }
    std::vector<uint8_t> cf;
    if (sdRead(path, cf) && !cf.empty()) {
      char b[40];
      const int n = (int)(cf.size() < sizeof(b) - 1 ? cf.size() : sizeof(b) - 1);
      memcpy(b, cf.data(), (size_t)n);
      b[n] = 0;
      unsigned q = 0, a = 0; int dm = -1, wx0 = -1, wx1 = -1, wy0 = -1, wy1 = -1; char sh[8] = "";
      if (sscanf(b, "%x %x %7s %d %d %d %d %d", &q, &a, sh, &dm, &wx0, &wx1, &wy0, &wy1) >= 2) {
        g_qCq = display->color565((q >> 16) & 255, (q >> 8) & 255, q & 255);
        g_qCa = display->color565((a >> 16) & 255, (a >> 8) & 255, a & 255);
        g_qShadow = (sh[0] != '-');
        if (g_qShadow && sh[0]) {
          g_qOutline = (sh[0] == 'o' || sh[0] == 'O');
          const unsigned sc = strtoul(g_qOutline ? sh + 1 : sh, nullptr, 16);
          g_qShCol = display->color565((sc >> 16) & 255, (sc >> 8) & 255, sc & 255);
        }
        if (dm >= 10 && dm <= 100) g_qDim = (uint8_t)dm;
        if (wx0 >= 0 && wx1 > wx0 + 30 && wx1 <= 128) { g_qX0 = wx0; g_qX1 = wx1; }
        if (wy0 >= 0 && wy1 > wy0 + 16 && wy1 <= 32) { g_qY0 = wy0; g_qY1 = wy1; }
      }
    }
  }
  return ok;
}

// fond image des widgets Recalbox a la place du ciel : /widgets/quotes/bg/rb-challenge | rb-rewards | rb-health .raw565 assombri a 55 % (texte lisible) ; absent = ciel
static bool decoRbBgLoad(const char *name) {
  g_skyBgImg = nullptr;
  { char fn[40]; snprintf(fn, sizeof(fn), "%s.raw565", name); decoBgEnsure(fn); }
  char bgPath[64];
  snprintf(bgPath, sizeof(bgPath), "/widgets/quotes/bg/%s.raw565", name);
  const size_t total = (size_t)RAW565_W * RAW565_H * sizeof(uint16_t);
  std::vector<uint8_t> raw;
  if (!sdRead(bgPath, raw) || raw.size() != total) { Serial.println(String("[DECO] fond Recalbox : ") + bgPath + " absent -> ciel"); return false; }
  memcpy(raw565FullBuf, raw.data(), total);
  for (size_t i = 0; i < (size_t)RAW565_W * RAW565_H; i++) {
    const uint16_t v = raw565FullBuf[i];
    const uint16_t r = ((v >> 11) & 31) * 55 / 100, g = ((v >> 5) & 63) * 55 / 100, b = (v & 31) * 55 / 100;
    raw565FullBuf[i] = (uint16_t)((r << 11) | (g << 5) | b);
  }
  g_skyBgImg = raw565FullBuf;
  return true;
}
// Boucle d'animation du ciel : fg(canevas, image, ctx) dessine l'avant-plan par-dessus le ciel. 1 termine, 0 interrompu.
// Le firmware dessine par BANDES de 1 a 4 Ko faute de tas ; le Pi a la place d'un canevas complet 128 x 32 (une seule bande).
static int decoSkyRun(unsigned long durMs, bool preview, int cls, void (*fg)(Adafruit_GFX &, int, void *), void *ctx) {
  static Display canvas(nullptr);
  time_t now0;
  time(&now0);
  struct tm t0t;
  localtime_r(&now0, &t0t);
  const float h0 = t0t.tm_hour + t0t.tm_min / 60.0f + t0t.tm_sec / 3600.0f;
  s_skyRise = 6.0f;
  s_skySet = 21.0f;
  if (!(decoLat == 0.0f && decoLon == 0.0f) && t0t.tm_year >= 100) {  // le ciel suit les heures reelles de lever / coucher du soleil
    time_t rs = 0, st = 0;
    bool pd = false;
    if (decoSunTimes(decoLat, decoLon, now0, rs, st, pd)) {
      struct tm ta, tb;
      localtime_r(&rs, &ta);
      localtime_r(&st, &tb);
      const float r = ta.tm_hour + ta.tm_min / 60.0f, s = tb.tm_hour + tb.tm_min / 60.0f;
      if (s > r + 4.0f) {
        s_skyRise = r;
        s_skySet = s;
      }
    }
  }
  const unsigned long t0 = millis();
  unsigned long last = 0;
  int f = 0;
  while (millis() - t0 < durMs) {
    if (decoStop(preview)) return 0;
    if (millis() - last < 50) {
      delay(2);
      continue;
    }
    last = millis();
    g_skyPass = 0;
    if (g_skyBgImg) canvas.drawRGBBitmap(0, 0, g_skyBgImg, 128, 32);
    else skyDraw(canvas, skyEffHour(h0 + (millis() - t0) / 3600000.0f), cls, f);
    fg(canvas, f, ctx);
    display->drawRGBBitmap(0, 0, canvas.framebuffer(), 128, 32);
    display->show();
    g_lastFlushMs = nowMs();
    f++;
  }
  return 1;
}

static void decoScratchRelease() {
  s_lvN = 0;
  if (s_lvBalls) {
    LavaBall *p = s_lvBalls;
    s_lvBalls = nullptr;
    free(p);
  }
  if (s_cdPts) {
    CdPt *p = s_cdPts;
    s_cdPts = nullptr;
    free(p);
  }
}

// Execute un widget (decoRunWidgetImpl du firmware). false = interrompu, true = termine ou saute faute de donnees.
static bool decoRunWidgetImpl(int id, bool preview) {
  auto skyOrSkip = [&](const char *name, unsigned long ms, int cls, void (*fg)(Adafruit_GFX &, int, void *), void *ctx) -> bool {
    const int r = decoSkyRun(ms, preview, cls, fg, ctx);
    display->clearScreen();
    display->show();
    return r == 1;
  };
  if (id == 1) {
    time_t now;
    struct tm ti;
    time(&now);
    localtime_r(&now, &ti);
    if (ti.tm_year < 100) { Serial.println("[DECO] date : heure non synchronisee, widget saute"); return true; }
    Serial.println("[DECO] widget date (" + String((int)decoDur[1]) + " s)");
    SkyDateCtx dc;
    dc.t = ti;
    return skyOrSkip("date", (unsigned long)decoDur[1] * 1000UL, decoSkyClass(), skyDateFg, &dc);
  }
  if (id == 2) {
    static SkyEphCtx ec;
    if (!decoEphCompute(&ec)) { Serial.println("[DECO] ephemeride : position ou heure non definie, widget saute"); return true; }
    Serial.println("[DECO] widget ephemeride (" + String((int)decoDur[2]) + " s)");
    return skyOrSkip("ephemeride", (unsigned long)decoDur[2] * 1000UL, decoSkyClass(), skyEphFg, &ec);
  }
  if (id == 3) {
    int idx, ey, em, ed;
    long days;
    if (!decoPickEvent(idx, days, ey, em, ed)) { Serial.println("[DECO] compte a rebours : aucun evenement a venir (ou heure non definie), widget saute"); return true; }
    static SkyCdCtx cc;
    cc.days = days;
    String nm = decoEvName[idx];
    nm.toUpperCase();
    if (nm.length() > 40) nm = nm.substring(0, 40);
    strncpy(cc.name, nm.c_str(), sizeof(cc.name) - 1);
    cc.name[sizeof(cc.name) - 1] = 0;
    if (uiLanguage == "en") snprintf(cc.ds, sizeof(cc.ds), "%02d/%02d", em, ed);
    else snprintf(cc.ds, sizeof(cc.ds), "%02d/%02d", ed, em);
    Serial.println("[DECO] widget compte a rebours (" + String((int)decoDur[3]) + " s)");
    return skyOrSkip("compte a rebours", (unsigned long)decoDur[3] * 1000UL, decoSkyClass(), skyCountFg, &cc);
  }
  if (id == 4) return decoRunAmbiance(preview);
  if (id == 5) {
    if (decoLat == 0.0f && decoLon == 0.0f) { Serial.println("[DECO] meteo : position non definie (page Lieu), widget saute"); return true; }
    const bool moved = s_wx.valid && (fabsf(s_wx.lat - decoLat) > 0.005f || fabsf(s_wx.lon - decoLon) > 0.005f);   // autre lieu = valeurs gardees inutilisables
    if (moved) s_wx.valid = false;
    const bool stale = !s_wx.valid || (millis() - s_wx.fetchedMs) > 30UL * 60000UL;
    if (stale && (preview || moved || !s_wxTried || (millis() - s_wxLastTry) > 120000UL)) {   // apres un echec : pas de nouvel essai avant 2 min (sauf apercu)
      display->clearScreen();
      display->setTextWrap(false);
      display->setTextSize(1);
      display->setTextColor(display->color565(150, 150, 150));
      display->setCursor(40, 12);
      display->print("METEO...");
      display->show();
      decoWeatherFetch();
    }
    Serial.println("[DECO] widget meteo (" + String((int)decoDur[5]) + " s)");
    return skyOrSkip("meteo", (unsigned long)decoDur[5] * 1000UL, decoSkyClass(), skyWxFg, nullptr);
  }
  if (id == 6) return true;  // widget « Statistiques Recalbox » retire du firmware : identifiant reserve, jamais joue
  if (id == 7) {
    char qt[240], qs[80];
    const int cnt = decoDur[7] < 1 ? 1 : (decoDur[7] > 10 ? 10 : decoDur[7]);   // la « duree » du widget = nombre de citations d'affilee (1 par defaut)
    for (int k = 0; k < cnt; k++) {
      if (!decoQuotePick(qt, sizeof(qt), qs, sizeof(qs))) { Serial.println("[DECO] citations : aucun theme / aucune citation, widget saute"); return true; }
      if (qs[0]) {  // QCM
        const int r = decoDrawQcm(qt, qs, preview);
        if (r >= 0) {
          display->setFont(NULL);
          display->clearScreen();
          if (r != 1) return false;
          continue;
        }
      }
      const bool done = decoDrawQuote(qt, qs, preview);
      display->clearScreen();
      if (!done) return false;
    }
    return true;
  }
  if (id == 8) {
    if (!g_rbw.ch) { Serial.println("[DECO] challenge Recalbox : aucune donnee (script RB absent ou pas de challenge), widget saute"); return true; }
    Serial.println("[DECO] widget challenge Recalbox (" + String((int)decoDur[8]) + " s)");
    decoRbBgLoad("rb-challenge");
    const bool r8 = skyOrSkip("challenge", (unsigned long)decoDur[8] * 1000UL, decoSkyClass(), skyRbwChFg, nullptr);
    g_skyBgImg = nullptr;
    return r8;
  }
  if (id == 9) {
    if (g_rbw.nRw == 0) { Serial.println("[DECO] recompenses Recalbox : aucune donnee, widget saute"); return true; }
    Serial.println("[DECO] widget recompenses Recalbox (" + String((int)decoDur[9]) + " s)");
    unsigned long rwMs = (unsigned long)decoDur[9] * 1000UL;
    const unsigned long rwNeed = (unsigned long)rbwRwTotalFrames() * 50UL;
    if (rwMs < rwNeed) rwMs = rwNeed;  // toutes les recompenses sont vues au moins une fois
    decoRbBgLoad("rb-rewards");
    const bool r9 = skyOrSkip("recompenses", rwMs, decoSkyClass(), skyRbwRwFg, nullptr);
    g_skyBgImg = nullptr;
    return r9;
  }
  if (id == 10) {
    if (!g_rbw.hl) { Serial.println("[DECO] sante Recalbox : aucune donnee, widget saute"); return true; }
    Serial.println("[DECO] widget sante Recalbox (" + String((int)decoDur[10]) + " s)");
    decoRbBgLoad("rb-health");
    const bool r10 = skyOrSkip("sante", (unsigned long)decoDur[10] * 1000UL, decoSkyClass(), skyRbwHlFg, nullptr);
    g_skyBgImg = nullptr;
    return r10;
  }
  return true;  // identifiant inconnu : saute
}

static bool decoRunWidget(int id, bool preview) {
  const bool r = decoRunWidgetImpl(id, preview);
  decoScratchRelease();
  return r;
}

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------------------------------------------------- API
namespace {
int32_t hexColor(const std::string &s) {  // « #RRGGBB » -> 0xRRGGBB, sinon -1 (automatique)
  if (s.size() != 7 || s[0] != '#') return -1;
  return (int32_t)(std::strtoul(s.c_str() + 1, nullptr, 16) & 0xFFFFFF);
}
}  // namespace

void Widgets::attach(Display &d, const SdFs &fs) {
  display = &d;
  g_fs = &fs;
}

void Widgets::feedRbw(const std::string &arg) { rbwParse(String(arg)); }

void Widgets::previewSpec(const std::string &specIn) {
  std::string wa = specIn, ll;
  const size_t at = wa.find('@');
  if (at != std::string::npos) { ll = wa.substr(at + 1); wa = wa.substr(0, at); }   // lieu affiche dans la page (brouillon non enregistre)
  std::vector<std::string> f;
  {
    size_t start = 0;
    for (int k = 0; k < 5; k++) {   // id, style, duree, themes, couleur a ; le reste (couleur b) est la 6e partie
      const size_t c = wa.find(':', start);
      if (c == std::string::npos) break;
      f.push_back(wa.substr(start, c - start));
      start = c + 1;
    }
    f.push_back(wa.substr(start));
  }
  const int wid = std::atoi(f[0].c_str());
  const int pStyle = f.size() > 1 ? std::atoi(f[1].c_str()) : -1;
  const int pDur = f.size() > 2 ? std::atoi(f[2].c_str()) : 0;
  const bool hasTheme = f.size() > 4;
  const std::string theme = f.size() > 3 ? f[3] : "";
  const std::string lo = f.size() > 4 ? f[4] : "", lw = f.size() > 5 ? f[5] : "";
  if (wid < 1 || wid > 10) return;
  const uint8_t oldDur = decoDur[wid], oldAmb = decoAmbStyle;
  const String oldQ = decoQuoteTheme;
  if (pDur >= 1 && pDur <= 120) decoDur[wid] = (uint8_t)pDur;
  if (wid == 4 && pStyle >= 0 && pStyle <= 8) decoAmbStyle = (uint8_t)pStyle;
  if (wid == 7 && hasTheme) decoQuoteTheme = quoteSlugList(String(theme));
  const int pSt = (pStyle >= 1 && pStyle <= 8) ? pStyle : 0;
  const int32_t oldCa = decoAmbCol[pSt][0], oldCb = decoAmbCol[pSt][1];
  if (wid == 4 && pSt > 0 && (!lo.empty() || !lw.empty())) {  // couleurs du style apercu (vide / « - » = automatique)
    decoAmbCol[pSt][0] = hexColor(lo);
    decoAmbCol[pSt][1] = hexColor(lw);
  }
  const float oldLa = decoLat, oldLo = decoLon;
  const size_t cm = ll.find(',');
  if (cm != std::string::npos && cm > 0 && (wid == 2 || wid == 5)) {
    decoLat = (float)std::atof(ll.substr(0, cm).c_str());
    decoLon = (float)std::atof(ll.substr(cm + 1).c_str());
  }
  decoRunWidget(wid, true);
  decoLat = oldLa;
  decoLon = oldLo;
  decoDur[wid] = oldDur;
  decoAmbStyle = oldAmb;
  decoQuoteTheme = oldQ;
  decoAmbCol[pSt][0] = oldCa;
  decoAmbCol[pSt][1] = oldCb;
}

int Widgets::restoreQuotes(const std::string &lang) { return decoQuotesRestore(String(lang)); }

void Widgets::setHooks(std::function<bool()> commandPending, std::function<void()> service) {
  g_cmdPending = std::move(commandPending);
  g_service = std::move(service);
}

void Widgets::configure(const Config &cfg, const uint8_t dur[11]) {
  for (int i = 0; i < 11; i++) decoDur[i] = dur[i];
  uiLanguage = cfg.uiLanguage.c_str();
  // anciennes cles DECO_* encore lues (le firmware les renomme en WIDGET_* : meme valeur ; la plus recente gagne)
  auto get = [&](const std::string &base, const std::string &def = "") -> std::string {
    return cfg.has("widget_" + base) ? cfg.get("widget_" + base, def) : cfg.get("deco_" + base, def);
  };
  auto has = [&](const std::string &base) { return cfg.has("widget_" + base) || cfg.has("deco_" + base); };
  decoLat = (float)std::atof(get("lat", "0").c_str());
  decoLon = (float)std::atof(get("lon", "0").c_str());
  std::string place = get("place");
  if (place.size() > 24) place.resize(24);
  decoPlace = place.c_str();
  decoPlace.trim();
  const int amb = std::atoi(get("amb", "0").c_str());
  decoAmbStyle = (uint8_t)((amb >= 0 && amb <= 8) ? amb : 0);
  for (int n = 1; n <= 8; n++)
    for (int k = 0; k < 2; k++) {
      const std::string base = std::string("ambc_") + char('0' + n) + "_" + (k ? "b" : "a");
      if (has(base)) decoAmbCol[n][k] = hexColor(get(base));
    }
  if (has("lava_oil")) decoAmbCol[8][0] = hexColor(get("lava_oil"));
  if (has("lava_wax")) decoAmbCol[8][1] = hexColor(get("lava_wax"));
  for (int id = 1; id <= DECO_EV_COUNT; id++) {  // DECO_EV_n = nom|AAAAMMJJ|annuel[|actif]
    const std::string v = get("ev_" + std::to_string(id));
    const size_t a = v.find('|');
    const size_t b = (a == std::string::npos) ? std::string::npos : v.find('|', a + 1);
    if (a == std::string::npos || b == std::string::npos || a == 0) continue;
    decoEvName[id - 1] = v.substr(0, a).c_str();
    decoEvDate[id - 1] = (uint32_t)std::strtoul(v.substr(a + 1, b - a - 1).c_str(), nullptr, 10);
    decoEvYearly[id - 1] = (std::atoi(v.substr(b + 1).c_str()) != 0) ? 1 : 0;
    const size_t c4 = v.find('|', b + 1);
    decoEvOn[id - 1] = (c4 != std::string::npos && std::atoi(v.substr(c4 + 1).c_str()) == 0) ? 0 : 1;
  }
  decoQuoteTheme = quoteSlugList(String(get("qtheme")));
  logf("DECO", "lieu=%s (%.4f, %.4f), ambiance=%d, langue=%s", decoPlace.c_str(), decoLat, decoLon, decoAmbStyle, cfg.uiLanguage.c_str());
}

void Widgets::setDuration(int id, int sec) {
  if (id >= 0 && id < 11 && sec >= 1 && sec <= 120) decoDur[id] = (uint8_t)sec;
}
void Widgets::setAmbStyle(int style) { decoAmbStyle = (uint8_t)((style >= 0 && style <= 8) ? style : 0); }

bool Widgets::run(int id, bool preview) { return decoRunWidget(id, preview); }
