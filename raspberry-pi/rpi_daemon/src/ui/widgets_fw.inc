// ============================================================================
// GENERE par tools/port_widgets.py depuis RecalBox_DMD.ino (FW_VERSION_NUM 382).
// NE PAS EDITER A LA MAIN : relancer le script. Widgets du decor (voir l'en-tete du script).
// ============================================================================
// ---- donnees des widgets Recalbox (CMD=rbw) et leur analyse (firmware, lignes 5369-5415) ----
struct RbwData {
  bool ch, hl; uint8_t nTop, nRw;
  char chName[32]; int days, rank, ggs;
  char tn[3][12]; uint32_t ts[3]; int8_t tm[3];
  char rk[4][8], rn[4][24], rc[4][48];
  char hs[8]; int temp, mem, disk;
};
RbwData g_rbw;
static String rbwField(const String &s, char sep, int idx)
{
  int start = 0;
  for (int i = 0; i < idx; i++) { const int p = s.indexOf(sep, start); if (p < 0) return String(""); start = p + 1; }
  const int e = s.indexOf(sep, start);
  return (e < 0) ? s.substring(start) : s.substring(start, e);
}
static void rbwCopy(char *d, size_t n, const String &s) { strncpy(d, s.c_str(), n - 1); d[n - 1] = 0; }
static void rbwParse(const String &arg)
{
  if (arg.length() < 1) return;
  const char t = arg[0];
  if (t == 'C') {
    const String nm = rbwField(arg, '|', 1);
    if (nm.length() == 0) { g_rbw.ch = false; Serial.println("[RBW] aucun challenge en cours"); return; }
    rbwCopy(g_rbw.chName, sizeof(g_rbw.chName), nm);
    g_rbw.days = rbwField(arg, '|', 2).toInt(); g_rbw.rank = rbwField(arg, '|', 3).toInt(); g_rbw.ggs = rbwField(arg, '|', 4).toInt();
    const String tops = rbwField(arg, '|', 5); g_rbw.nTop = 0;
    for (int i = 0; i < 3; i++) {
      const String e = rbwField(tops, ';', i); if (e.length() == 0) break;
      rbwCopy(g_rbw.tn[i], sizeof(g_rbw.tn[i]), rbwField(e, ':', 0));
      g_rbw.ts[i] = (uint32_t)strtoul(rbwField(e, ':', 1).c_str(), nullptr, 10); g_rbw.tm[i] = (int8_t)rbwField(e, ':', 2).toInt(); g_rbw.nTop++;
    }
    g_rbw.ch = true;
    Serial.println("[RBW] challenge : " + String(g_rbw.chName) + " j=" + String(g_rbw.days) + " rang=" + String(g_rbw.rank) + " gg=" + String(g_rbw.ggs) + " top=" + String(g_rbw.nTop));
  } else if (t == 'R') {
    g_rbw.nRw = 0;
    for (int i = 0; i < 4; i++) {
      const String e = rbwField(arg, '|', i + 1); if (e.length() == 0) break;
      rbwCopy(g_rbw.rk[i], sizeof(g_rbw.rk[i]), rbwField(e, ':', 0)); rbwCopy(g_rbw.rn[i], sizeof(g_rbw.rn[i]), rbwField(e, ':', 1)); rbwCopy(g_rbw.rc[i], sizeof(g_rbw.rc[i]), rbwField(e, ':', 2)); g_rbw.nRw++;
    }
    Serial.println("[RBW] recompenses : " + String(g_rbw.nRw));
  } else if (t == 'H') {
    rbwCopy(g_rbw.hs, sizeof(g_rbw.hs), rbwField(arg, '|', 1));
    g_rbw.temp = rbwField(arg, '|', 2).toInt(); g_rbw.mem = rbwField(arg, '|', 3).toInt(); g_rbw.disk = rbwField(arg, '|', 4).toInt();
    g_rbw.hl = (strlen(g_rbw.hs) >= 6);
    Serial.println("[RBW] sante : " + String(g_rbw.hs) + " temp=" + String(g_rbw.temp) + " mem=" + String(g_rbw.mem) + " disk=" + String(g_rbw.disk));
  }
}

// ---- date + ephemeride (firmware, lignes 13383-13532) ----
static int decoIsoWeek(const struct tm &t)
{
  const int y = t.tm_year + 1900;
  const int wd = (t.tm_wday + 6) % 7;   // lundi = 0
  int w = (t.tm_yday - wd + 10) / 7;
  auto weeksIn = [](int yy) {
    int p  = (yy + yy / 4 - yy / 100 + yy / 400) % 7;
    int yp = yy - 1;
    int pp = (yp + yp / 4 - yp / 100 + yp / 400) % 7;
    return (p == 4 || pp == 3) ? 53 : 52;
  };
  if (w < 1) return weeksIn(y - 1);
  if (w > weeksIn(y)) return 1;
  return w;
}

// Jours feries francais (retourne nullptr si le jour n'en est pas un). Paques : algorithme de Meeus/Jones/Butcher.
static const char *decoHolidayFr(const struct tm &t)
{
  const int d = t.tm_mday, mo = t.tm_mon + 1, y = t.tm_year + 1900;
  if (d == 1  && mo == 1)  return "JOUR DE L'AN";
  if (d == 1  && mo == 5)  return "FETE DU TRAVAIL";
  if (d == 8  && mo == 5)  return "VICTOIRE 1945";
  if (d == 14 && mo == 7)  return "FETE NATIONALE";
  if (d == 15 && mo == 8)  return "ASSOMPTION";
  if (d == 1  && mo == 11) return "TOUSSAINT";
  if (d == 11 && mo == 11) return "ARMISTICE";
  if (d == 25 && mo == 12) return "NOEL";
  const int a = y % 19, b = y / 100, c = y % 100, dd = b / 4, e = b % 4, f = (b + 8) / 25, g = (b - f + 1) / 3;
  const int h = (19 * a + b - dd - g + 15) % 30, i = c / 4, k = c % 4, l = (32 + 2 * e + 2 * i - h - k) % 7;
  const int m = (a + 11 * h + 22 * l) / 451;
  const int em = (h + l - 7 * m + 114) / 31, ed = ((h + l - 7 * m + 114) % 31) + 1;   // Paques
  static const struct { int off; const char *name; } moving[] = { {1, "LUNDI DE PAQUES"}, {39, "ASCENSION"}, {50, "LUNDI DE PENTECOTE"} };
  for (unsigned n = 0; n < sizeof(moving) / sizeof(moving[0]); n++) {
    struct tm x = {};
    x.tm_year = t.tm_year; x.tm_mon = em - 1; x.tm_mday = ed + moving[n].off; x.tm_hour = 12;
    mktime(&x);   // normalise le jour (debordement de mois)
    if (x.tm_mon == t.tm_mon && x.tm_mday == t.tm_mday) return moving[n].name;
  }
  return nullptr;
}

static void decoDrawDate(const struct tm &t)
{
  const bool en = (uiLanguage == "en"), es = (uiLanguage == "es");
  static const char *dFr[] = {"DIM", "LUN", "MAR", "MER", "JEU", "VEN", "SAM"};
  static const char *dEn[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
  static const char *dEs[] = {"DOM", "LUN", "MAR", "MIE", "JUE", "VIE", "SAB"};
  static const char *mFr[] = {"JAN", "FEV", "MAR", "AVR", "MAI", "JUN", "JUL", "AOU", "SEP", "OCT", "NOV", "DEC"};
  static const char *mEn[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
  static const char *mEs[] = {"ENE", "FEB", "MAR", "ABR", "MAY", "JUN", "JUL", "AGO", "SEP", "OCT", "NOV", "DIC"};
  const char *wd = (en ? dEn : es ? dEs : dFr)[t.tm_wday % 7];
  const char *mo = (en ? mEn : es ? mEs : mFr)[t.tm_mon % 12];
  char dayBuf[4]; snprintf(dayBuf, sizeof(dayBuf), "%d", t.tm_mday);
  display->clearScreen();
  display->setTextWrap(false);
  display->setTextSize(2);
  const int w1 = (3 + 1 + (int)strlen(dayBuf) + 1 + 3) * 12;
  int x = (128 - w1) / 2; if (x < 0) x = 0;
  display->setCursor(x, 1);
  display->setTextColor(0xFFE0); display->print(wd); display->print(' ');
  display->setTextColor(0xFFFF); display->print(dayBuf); display->print(' ');
  display->setTextColor(0x07FF); display->print(mo);
  display->setTextSize(1);
  char line2[28];
  const char *hol = (!en && !es) ? decoHolidayFr(t) : nullptr;
  if (hol) { snprintf(line2, sizeof(line2), "%s", hol); display->setTextColor(0x07E0); }
  else     { snprintf(line2, sizeof(line2), "%s%d - %d", en ? "WK" : "S", decoIsoWeek(t), t.tm_year + 1900); display->setTextColor(display->color565(200, 200, 200)); }
  int x2 = (128 - (int)strlen(line2) * 6) / 2; if (x2 < 0) x2 = 0;
  display->setCursor(x2, 22);
  display->print(line2);
}

// -- Widget EPHEMERIDE (id 2) : soleil (lever / coucher / duree du jour) et lune (phase, illumination).
// Equation du lever du soleil (formules de l'almanach, precision ~ 1-2 min) ; longitude positive a l'est.
// Retourne false si le soleil ne se leve / ne se couche pas ce jour-la (jour ou nuit polaire : *polarDay dit lequel).
static bool decoSunTimes(double lat, double lon, time_t now, time_t &rise, time_t &set, bool &polarDay)
{
  const double R = M_PI / 180.0;
  const double jd = (double)now / 86400.0 + 2440587.5;
  const double n  = ceil(jd - 2451545.0 + 0.0008);
  const double js = n - lon / 360.0;
  const double M  = fmod(357.5291 + 0.98560028 * js, 360.0);
  const double C  = 1.9148 * sin(M * R) + 0.0200 * sin(2 * M * R) + 0.0003 * sin(3 * M * R);
  const double lam = fmod(M + C + 180.0 + 102.9372, 360.0);
  const double jt  = 2451545.0 + js + 0.0053 * sin(M * R) - 0.0069 * sin(2 * lam * R);
  const double sd  = sin(lam * R) * sin(23.4397 * R);
  const double cd  = cos(asin(sd));
  const double cw  = (sin(-0.833 * R) - sin(lat * R) * sd) / (cos(lat * R) * cd);
  if (cw <= -1.0) { polarDay = true;  return false; }
  if (cw >=  1.0) { polarDay = false; return false; }
  const double w = acos(cw) / R;
  rise = (time_t)((jt - w / 360.0 - 2440587.5) * 86400.0);
  set  = (time_t)((jt + w / 360.0 - 2440587.5) * 86400.0);
  return true;
}

// Disque lunaire : partie eclairee a droite si la lune croit (hemisphere nord), inversee dans l'hemisphere sud.
static void decoDrawMoon(int cx, int cy, int rad, double age, bool south)
{
  const double SYN = 29.530588853;
  const double th = 2.0 * M_PI * age / SYN;
  const bool waxing = age < SYN / 2.0;
  const uint16_t lit = display->color565(255, 240, 180), dark = display->color565(30, 30, 40);
  for (int dy = -rad; dy <= rad; dy++) {
    const int w = (int)(sqrt((double)(rad * rad - dy * dy)) + 0.5);
    const int t = (int)(w * cos(th) + (cos(th) >= 0 ? 0.5 : -0.5));
    for (int dx = -w; dx <= w; dx++) {
      bool on = waxing ? (dx >= t) : (dx <= -t);
      if (south) on = waxing ? (dx <= -t) : (dx >= t);
      display->drawPixel(cx + dx, cy + dy, on ? lit : dark);
    }
  }
}

// Dessine l'ephemeride ; retourne false si elle ne peut pas etre calculee (position non definie).
static bool decoDrawEphemeris()
{
  if (decoLat == 0.0f && decoLon == 0.0f) return false;
  time_t now; time(&now);
  struct tm tl; localtime_r(&now, &tl);
  if (tl.tm_year < 100) return false;
  const bool en = (uiLanguage == "en"), es = (uiLanguage == "es");
  const char *sRise = en ? "SUNRISE" : es ? "SALIDA" : "LEVER";
  const char *sSet  = en ? "SUNSET"  : es ? "OCASO"  : "COUCHER";
  const char *sMoon = en ? "MOON"    : es ? "LUNA"   : "LUNE";
  time_t rise = 0, set = 0; bool polarDay = false;
  const bool ok = decoSunTimes(decoLat, decoLon, now, rise, set, polarDay);
  const double SYN = 29.530588853;
  double age = fmod((double)now / 86400.0 + 2440587.5 - 2451550.1, SYN); if (age < 0) age += SYN;
  const int illum = (int)(((1.0 - cos(2.0 * M_PI * age / SYN)) / 2.0) * 100.0 + 0.5);
  display->clearScreen();
  display->setTextWrap(false);
  display->setTextSize(1);
  decoDrawMoon(15, 16, 13, age, decoLat < 0);
  char b[28];
  String pl = decoPlace; if (pl.length() > 15) pl = pl.substring(0, 15); pl.toUpperCase();
  display->setTextColor(display->color565(150, 150, 150)); display->setCursor(34, 0);  display->print(pl);
  if (ok) {
    struct tm a, c; localtime_r(&rise, &a); localtime_r(&set, &c);
    const int dl = (int)((set - rise) / 60);
    display->setTextColor(0xFFE0);                              display->setCursor(34, 8);  snprintf(b, sizeof(b), "%s %02d:%02d", sRise, a.tm_hour, a.tm_min); display->print(b);
    display->setTextColor(display->color565(255, 140, 0));      display->setCursor(34, 16); snprintf(b, sizeof(b), "%s %02d:%02d", sSet, c.tm_hour, c.tm_min);   display->print(b);
    display->setTextColor(0x07FF);                              display->setCursor(34, 24); snprintf(b, sizeof(b), "%dH%02d %s %d%%", dl / 60, dl % 60, sMoon, illum); display->print(b);
  } else {
    display->setTextColor(0xFFE0);                              display->setCursor(34, 8);  display->print(polarDay ? (en ? "POLAR DAY" : es ? "DIA POLAR" : "JOUR POLAIRE") : (en ? "POLAR NIGHT" : es ? "NOCHE POLAR" : "NUIT POLAIRE"));
    display->setTextColor(0x07FF);                              display->setCursor(34, 24); snprintf(b, sizeof(b), "%s %d%%", sMoon, illum); display->print(b);
  }
  return true;
}

// ---- compte a rebours : calcul de l'evenement (firmware, lignes 13552-13584) ----
static long decoJdn(int y, int m, int d)   // jours depuis 1970-01-01 (Howard Hinnant)
{
  y -= (m <= 2);
  const long era = (y >= 0 ? y : y - 399) / 400;
  const unsigned yoe = (unsigned)(y - era * 400);
  const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097L + (long)doe - 719468L;
}
static bool decoLeap(int y) { return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0; }

static bool decoPickEvent(int &idx, long &days, int &ey, int &em, int &ed)
{
  time_t now; time(&now); struct tm t; localtime_r(&now, &t);
  if (t.tm_year < 100) return false;
  const long today = decoJdn(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
  long best = -1; idx = -1;
  for (int i = 0; i < DECO_EV_COUNT; i++) {
    if (!decoEvDate[i] || !decoEvOn[i]) continue;   // v353 : desactive = ignore
    int y = (int)(decoEvDate[i] / 10000), m = (int)((decoEvDate[i] / 100) % 100), d = (int)(decoEvDate[i] % 100);
    if (decoEvYearly[i]) {
      y = t.tm_year + 1900;
      int dd = (m == 2 && d == 29 && !decoLeap(y)) ? 28 : d;
      if (decoJdn(y, m, dd) < today) { y++; dd = (m == 2 && d == 29 && !decoLeap(y)) ? 28 : d; }
      d = dd;
    }
    const long diff = decoJdn(y, m, d) - today;
    if (diff < 0) continue;
    if (best < 0 || diff < best) { best = diff; idx = i; days = diff; ey = y; em = m; ed = d; }
  }
  return idx >= 0;
}


// ---- ambiances (firmware, lignes 13668-14263) ----
// -- Ambiances : plasma, pluie, etoiles, vie (aucune allocation dynamique : tampons statiques) --
static bool decoAmbPlasma(unsigned long durMs, bool preview)
{
  uint16_t *pal = s_amb.pal;
  decoPlasmaInit(pal);   // v292 : parametres aleatoires a chaque lancement, motif non repetitif
  if (decoAmbCol[1][0] >= 0) decoPlasmaPalette(pal, decoAmbCol[1][0]);   // v311 : couleur choisie = palette monochrome
  const unsigned long t0 = millis();
  while (millis() - t0 < durMs) {
    if (decoStop(preview)) return false;
    decoPlasmaStep();
    for (int y = 0; y < 32; y++)
      for (int x = 0; x < 128; x++) display->drawPixel(x, y, decoPlasmaAt(x, y, pal));
    delay(1);
  }
  return true;
}
static bool decoAmbRain(unsigned long durMs, bool preview)
{
  int16_t *head = s_amb.rain.head; uint8_t *len = s_amb.rain.len, *spd = s_amb.rain.spd;
  display->clearScreen();
  for (int x = 0; x < 128; x++) { head[x] = -(int)random(0, 40); len[x] = (uint8_t)random(6, 14); spd[x] = (uint8_t)random(1, 3); }
  int rR = 0, rG = 200, rB = 40; if (decoAmbCol[2][0] >= 0) { rR = (decoAmbCol[2][0] >> 16) & 255; rG = (decoAmbCol[2][0] >> 8) & 255; rB = decoAmbCol[2][0] & 255; }   // v311
  const uint16_t rHead = display->color565(rR + (255 - rR) * 3 / 4, rG + (255 - rG) * 3 / 4, rB + (255 - rB) * 3 / 4), rMid = display->color565(rR, rG, rB), rTail = display->color565(rR * 55 / 100, rG * 55 / 100, rB * 55 / 100);
  const unsigned long t0 = millis(); unsigned long last = 0; unsigned tick = 0;
  while (millis() - t0 < durMs) {
    if (decoStop(preview)) return false;
    if (millis() - last < 45) { delay(2); continue; }
    last = millis(); tick++;
    for (int x = 0; x < 128; x++) {
      if (spd[x] == 1 && (tick & 1)) continue;   // colonne lente : une image sur deux
      const int y = ++head[x];
      if (y >= 0 && y < 32)       display->drawPixel(x, y, rHead);
      if (y - 1 >= 0 && y - 1 < 32) display->drawPixel(x, y - 1, rMid);
      if (y - 3 >= 0 && y - 3 < 32) display->drawPixel(x, y - 3, rTail);
      const int e = y - len[x];
      if (e >= 0 && e < 32)       display->drawPixel(x, e, 0);
      if (e > 32) { head[x] = -(int)random(0, 30); len[x] = (uint8_t)random(6, 14); spd[x] = (uint8_t)random(1, 3); }
    }
  }
  return true;
}

static bool decoAmbStars(unsigned long durMs, bool preview)
{
  typedef AmbStar Star;
  Star *st = s_amb.st;
  display->clearScreen();
  auto spawn = [](Star &s) { s.x = random(-640, 640) / 10.0f; s.y = random(-160, 160) / 10.0f; s.z = random(30, 100) / 10.0f; s.px = -1; s.py = -1; };
  for (int i = 0; i < 48; i++) spawn(st[i]);
  int sR = 255, sG = 255, sB = 255; if (decoAmbCol[3][0] >= 0) { sR = (decoAmbCol[3][0] >> 16) & 255; sG = (decoAmbCol[3][0] >> 8) & 255; sB = decoAmbCol[3][0] & 255; }   // v311
  const unsigned long t0 = millis(); unsigned long last = 0;
  while (millis() - t0 < durMs) {
    if (decoStop(preview)) return false;
    if (millis() - last < 35) { delay(2); continue; }
    last = millis();
    for (int i = 0; i < 48; i++) {
      Star &s = st[i];
      if (s.px >= 0) display->drawPixel(s.px, s.py, 0);
      s.z -= 0.12f;
      if (s.z <= 0.4f) { spawn(s); continue; }
      const int px = 64 + (int)(s.x / s.z * 8.0f), py = 16 + (int)(s.y / s.z * 8.0f);
      if (px < 0 || px >= 128 || py < 0 || py >= 32) { spawn(s); continue; }
      int b = (int)(255.0f * (1.0f - s.z / 10.0f)); if (b < 40) b = 40;
      display->drawPixel(px, py, display->color565(sR * b / 255, sG * b / 255, sB * b / 255));
      s.px = (int16_t)px; s.py = (int16_t)py;
    }
  }
  return true;
}

static bool decoAmbLife(unsigned long durMs, bool preview)
{
  uint8_t (*g)[512] = s_amb.life;
  int cur = 0;
  auto get = [&](int b, int x, int y) -> int { x &= 127; y &= 31; const int i = y * 128 + x; return (g[b][i >> 3] >> (i & 7)) & 1; };
  auto put = [&](int b, int x, int y, int v) { const int i = y * 128 + x; if (v) g[b][i >> 3] |= (uint8_t)(1 << (i & 7)); else g[b][i >> 3] &= (uint8_t)~(1 << (i & 7)); };
  const uint16_t lifeC = (decoAmbCol[4][0] >= 0) ? display->color565((decoAmbCol[4][0] >> 16) & 255, (decoAmbCol[4][0] >> 8) & 255, decoAmbCol[4][0] & 255) : display->color565(0, 190, 60);   // v311
  auto reseed = [&]() {
    display->clearScreen();
    memset(g[cur], 0, 512);
    for (int y = 0; y < 32; y++) for (int x = 0; x < 128; x++) if (random(0, 100) < 28) { put(cur, x, y, 1); display->drawPixel(x, y, lifeC); }
  };
  reseed();
  const unsigned long t0 = millis(); unsigned long last = 0; int gens = 0;
  while (millis() - t0 < durMs) {
    if (decoStop(preview)) return false;
    if (millis() - last < 90) { delay(2); continue; }
    last = millis(); gens++;
    const int nx = 1 - cur; int changes = 0;
    for (int y = 0; y < 32; y++)
      for (int x = 0; x < 128; x++) {
        const int n = get(cur, x - 1, y - 1) + get(cur, x, y - 1) + get(cur, x + 1, y - 1) + get(cur, x - 1, y) + get(cur, x + 1, y) + get(cur, x - 1, y + 1) + get(cur, x, y + 1) + get(cur, x + 1, y + 1);
        const int old = get(cur, x, y), v = old ? (n == 2 || n == 3) : (n == 3);
        put(nx, x, y, v);
        if (v != old) { display->drawPixel(x, y, v ? lifeC : 0); changes++; }
      }
    cur = nx;
    if (changes < 6 || gens >= 300) { gens = 0; reseed(); }   // population figee ou trop ancienne : nouvelle graine
  }
  return true;
}

// -- LIQUIDE : "machine a vagues" (Dolphin Wave / La Vague). Le DMD est un bac ferme, plein d'eau transparente (invisible), contenant une nappe de LIQUIDE COLORE plus dense que l'eau.
// A chaque lancement on tire un TYPE de liquide (eau teintee, huile, sirop, miel, cire fondue, mercure) : chacun injecte dans le moteur sa couleur, sa viscosite (trainee, lissage, amortissement
// des ondelettes), son agitation de surface, la presence de gouttelettes, ET sa maniere d'etre incline (amplitude, vitesse, pauses) : le miel coule lentement donc forte pente mais bascules tres lentes,
// le mercure et l'eau sont vifs, etc. Modele : eau peu profonde 1D (Saint-Venant) dans le repere du bac, gravite inclinee, viscosite artificielle sur les fronts ; PAR-DESSUS, un champ d'ondelettes de surface
// (equation d'onde amortie, excitee par l'ecoulement, les fronts et les impacts) ; gouttelettes balistiques aux parois pour les liquides fluides. --
struct LiquidType { const char *name; uint8_t rgb[3]; float drag, nu, rdamp, rexc; uint8_t ampMin, ampMax; uint16_t spdMin, spdMax; uint8_t holdMin, holdMax, dMin, dMax; bool drops; };
static const LiquidType LQT[6] = {
  // nom            couleur          trainee  lissage ond.amort ond.exc  incl.(deg)  vitesse (x1e-4 rad/img)  pause (img)  profondeur  gouttes
  {"eau teintee",   {0, 205, 255},   0.002f,  0.012f, 0.012f,   1.00f,   9, 13,      80, 200,                 0, 30,       8, 14,      true},
  {"huile",         {0, 120, 255},   0.020f,  0.030f, 0.050f,   0.60f,   10, 15,     50, 200,                 0, 40,       9, 16,      true},
  {"sirop",         {0, 215, 120},   0.080f,  0.060f, 0.100f,   0.30f,   12, 18,     40, 120,                 10, 60,      9, 16,      false},
  {"miel",          {255, 165, 0},   0.550f,  0.150f, 0.300f,   0.05f,   18, 26,      6, 20,                  40, 120,     9, 15,      false},
  {"cire fondue",   {255, 85, 170},  0.250f,  0.100f, 0.200f,   0.10f,   16, 22,     15, 50,                  30, 90,      9, 15,      false},
  {"mercure",       {215, 225, 235}, 0.001f,  0.010f, 0.010f,   1.20f,   8, 12,     100, 300,                 0, 20,       8, 13,      true}};
static bool decoAmbLiquid(unsigned long durMs, bool preview)
{
  // v377 -- LIQUIDE DANS UN BAC : le bac (sol + deux parois) est pose sur une barre triangulaire dont la pointe est au centre bas de la dalle ; il BASCULE autour de ce pivot,
  // toujours a l'inclinaison maximale du type de liquide, a une vitesse tiree au hasard a chaque bascule. Le liquide (meme physique qu'avant, dans le repere du bac) reste
  // horizontal a l'ecran ; sa surface et l'impact sur les parois restent visibles. Rendu par inversion : chaque pixel de l'ecran est converti dans le repere du bac.
  const int W = 128, H = 32, NW = 124, SUB = 6, ND = 48;   // v379 : 48 gouttelettes (10 avant) pour les grosses eclaboussures   // v378 : bac = largeur de la dalle moins l'epaisseur des deux parois (2 x 2 px)
  const float PX = 64.0f, PY = 26.5f, FL = 2.0f, WT = 2.0f, WH = 42.0f;
  struct LiqBuf { float h[NW], hn[NW], q[NW - 1], r[NW], rv[NW], sf[NW]; float dpx[ND], dpy[ND], dvx[ND], dvy[ND]; bool dal[ND]; };
  LiqBuf *lb = (LiqBuf *)malloc(sizeof(LiqBuf));
  if (!lb) { Serial.println("[DECO] liquide : tas insuffisant, widget saute"); return true; }
  struct LiqFree { void *p; ~LiqFree() { free(p); } } lbFree = { lb };
  float (&h)[NW] = lb->h; float (&hn)[NW] = lb->hn; float (&q)[NW - 1] = lb->q; float (&r)[NW] = lb->r; float (&rv)[NW] = lb->rv; float (&sf)[NW] = lb->sf;
  float (&dpx)[ND] = lb->dpx; float (&dpy)[ND] = lb->dpy; float (&dvx)[ND] = lb->dvx; float (&dvy)[ND] = lb->dvy; bool (&dal)[ND] = lb->dal;
  static int s_lastLq = -1;
  int ti; do { ti = (int)random(0, 6); } while (ti == s_lastLq); s_lastLq = ti;
  const LiquidType &L = LQT[ti];
  const float jit = random(85, 120) / 100.0f;
  const float hm = (float)random(L.dMin, L.dMax + 1);
  float ampDeg = random(L.ampMin * 10, L.ampMax * 10 + 1) / 10.0f; if (ampDeg > 16.0f) ampDeg = 16.0f;
  const float amp = ampDeg * (float)M_PI / 180.0f;
  const unsigned long phMs = (unsigned long)random(5000, 10001);   // v379 : DUREE DE PHASE fixee au demarrage (5 a 10 s), la meme pour toutes les bascules
  const float adh = L.drops ? 0.0f : fminf(1.0f, 0.4f + L.drag * 1.5f);   // v379 : ADHERENCE aux parois des liquides peu fluides (sirop 0,5 / cire 0,8 / miel 1)
  float wetH[2] = {0, 0};   // hauteur du film laisse sur chaque paroi
  const float dt = 1.0f / SUB, g = 0.40f, drag = L.drag * jit, nu = L.nu * jit, rdamp = L.rdamp, rk = 0.30f, rexc = L.rexc;
  uint8_t oc[3] = {L.rgb[0], L.rgb[1], L.rgb[2]};
  if (decoAmbCol[5][0] >= 0) { oc[0] = (decoAmbCol[5][0] >> 16) & 255; oc[1] = (decoAmbCol[5][0] >> 8) & 255; oc[2] = decoAmbCol[5][0] & 255; }
  Serial.println(String("[DECO] liquide (bac) : ") + L.name + ", profondeur " + String(hm, 0) + " px, bascule max " + String(ampDeg, 1) + " deg, phase " + String(phMs / 1000.0f, 1) + " s, heap=" + String(ESP.getFreeHeap()));
  for (int i = 0; i < NW; i++) { h[i] = hm; r[i] = 0; rv[i] = 0; sf[i] = hm; }
  for (int i = 0; i < NW - 1; i++) q[i] = 0;
  for (int i = 0; i < ND; i++) dal[i] = false;
  display->clearScreen();
  const float wR = 95, wG = 105, wB = 130, hR = 160, hG = 170, hB = 200;   // bac : couleur de base et liseré clair cote interieur
  const uint16_t triC = display->color565(70, 55, 40);
  const uint16_t dropC = display->color565(min(255, oc[0] + 110), min(255, oc[1] + 110), 255);
  float th = 0, tgt = random(0, 2) ? amp : -amp, spd = random(4, 101) / 10000.0f; int cdL = 0, cdR = 0;
  unsigned long phT0 = millis();
  const unsigned long t0 = millis(); unsigned long last = 0;
  while (millis() - t0 < durMs) {
    if (decoStop(preview)) return false;
    if (millis() - last < 33) { delay(2); continue; }
    last = millis();
    // bascule : chaque phase dure EXACTEMENT phMs ; a chaque phase le sens s'inverse et la vitesse est tiree au hasard (tres lente = bascule interrompue avant le maximum,
    // rapide = maximum atteint puis pause jusqu'a la fin de la phase) ; la phase suivante repart de l'angle atteint
    if (millis() - phT0 >= phMs) { tgt = -tgt; spd = random(4, 101) / 10000.0f; phT0 = millis(); }
    if (fabsf(th - tgt) >= 0.0001f) {
      const float dd = tgt - th; float ma = fabsf(dd * 0.06f);
      if (ma > spd) ma = spd; else if (ma < 0.25f * spd) ma = 0.25f * spd;
      if (ma >= fabsf(dd)) th = tgt; else th += (dd > 0 ? ma : -ma);
    }
    const float cs = cosf(th), sn = sinf(th);
    const float gx = sn, gy = cs;
    for (int s = 0; s < SUB; s++) {
      for (int i = 0; i < NW - 1; i++) {
        const float hf = (h[i] + h[i + 1]) * 0.5f;
        float qq = q[i] + dt * (-gy * g * hf * (h[i + 1] - h[i]) + g * gx * hf) - dt * drag * q[i];
        if (qq > 0) { const float m = 0.4f * h[i] / dt; if (qq > m) qq = m; } else { const float m = -0.4f * h[i + 1] / dt; if (qq < m) qq = m; }
        if (adh > 0.0f && (i < 4 || i > NW - 6)) qq *= 1.0f - 0.12f * adh;   // v379 : le liquide visqueux colle aux parois
        q[i] = qq;
      }
      for (int i = 0; i < NW; i++) {
        const float ql = i > 0 ? q[i - 1] : 0.0f, qr = i < NW - 1 ? q[i] : 0.0f;
        h[i] += dt * (ql - qr);
      }
      for (int i = 0; i < NW; i++) {
        const float a = i > 0 ? h[i - 1] : h[i], b = i < NW - 1 ? h[i + 1] : h[i];
        float nl = nu + 0.4f * fabsf(b - a); if (nl > 2.0f) nl = 2.0f;
        float v = h[i] + nl * dt * (a - 2 * h[i] + b);
        hn[i] = v < 0 ? 0 : (v > WH - 2.0f ? WH - 2.0f : v);
      }
      memcpy(h, hn, sizeof(h));
    }
    // ondelettes de surface : equation d'onde amortie, excitee par l'ecoulement et les fronts, impulsion forte aux parois
    for (int x = 0; x < NW; x++) {
      const float qa = fabsf(q[x < NW - 1 ? x : NW - 2]), gr = fabsf((x < NW - 1 ? h[x + 1] : h[x]) - (x > 0 ? h[x - 1] : h[x]));
      if (h[x] > 0.5f) rv[x] += random(-1000, 1001) / 1000.0f * rexc * (0.02f + 0.02f * qa + 0.08f * gr);
    }
    for (int it = 0; it < 2; it++) {
      for (int x = 0; x < NW; x++) {
        const float a = x > 0 ? r[x - 1] : r[x], b = x < NW - 1 ? r[x + 1] : r[x];
        rv[x] += rk * (a + b - 2 * r[x]) - rdamp * rv[x] - 0.02f * r[x];
      }
      for (int x = 0; x < NW; x++) { r[x] += rv[x]; if (r[x] > 2.5f) r[x] = 2.5f; else if (r[x] < -2.5f) r[x] = -2.5f; }
    }
    // impacts sur les parois du bac : choc dans les ondelettes, gerbe de gouttelettes pour les liquides fluides (lancees dans le repere du bac, retombent a l'ecran)
    if (cdL > 0) cdL--;
    if (cdR > 0) cdR--;
    const float qL = q[0], qR = q[NW - 2];
    for (int side = 0; side < 2; side++) {
      const float qi = side ? qR : -qL;
      int &cd = side ? cdR : cdL;
      if (qi > 1.2f && cd == 0) {   // v381 : seuil 2,5 -> 1,2 (les bascules lentes tirees au hasard ne donnaient presque jamais d'impact)
        cd = 4;
        if (L.drops) {
          int n = 4 + (int)(qi * 4.0f); if (n > ND) n = ND;
          const float sp = 0.8f + 0.25f * qi;
          for (int k = 0; k < n; k++) {
            for (int d = 0; d < ND; d++) if (!dal[d]) {
              const int xs = side ? NW - 2 - (int)random(0, 4) : 1 + (int)random(0, 4);
              const float u0 = xs + 0.5f - NW * 0.5f, v0 = h[xs] + random(0, 100) / 100.0f * 2.0f;
              const float vxs = (side ? -1.0f : 1.0f) * (0.3f + random(0, 100) / 100.0f * 0.9f) * sp * 0.8f, vys = (0.35f + random(0, 100) / 100.0f) * sp;   // v381 : lancees dans le repere de l'ECRAN (vers le haut et vers l'interieur), pas dans celui du bac penche (elles partaient hors de la dalle)
              dal[d] = true; dpx[d] = PX + u0 * cs + v0 * sn; dpy[d] = PY - (-u0 * sn + v0 * cs);
              dvx[d] = vxs; dvy[d] = -vys; break;
            }
          }
        }
        for (int k = 0; k < 4; k++) rv[side ? NW - 1 - k : k] += 0.35f * rexc * (4 - k) / 4.0f * qi * 0.2f;
      }
    }
    // surface de chaque colonne du bac = nappe + ondelettes
    for (int x = 0; x < NW; x++) sf[x] = (h[x] > 0.15f) ? h[x] + (h[x] > 0.4f ? r[x] : 0.0f) : 0.0f;
    if (adh > 0.0f) {   // v379 : menisque (le liquide remonte contre la paroi) puis film laisse sur la paroi, qui ne s'ecoule que lentement
      static const float MEN[3] = {2.0f, 1.2f, 0.6f};
      for (int k = 0; k < 3; k++) { if (sf[k] > 0.4f) sf[k] += adh * MEN[k]; if (sf[NW - 1 - k] > 0.4f) sf[NW - 1 - k] += adh * MEN[k]; }
      const float lv[2] = {sf[0], sf[NW - 1]};
      for (int sd = 0; sd < 2; sd++) {
        if (lv[sd] > wetH[sd]) wetH[sd] = lv[sd];
        else { wetH[sd] -= 0.004f + 0.05f * (1.0f - adh); if (wetH[sd] < lv[sd]) wetH[sd] = lv[sd]; }
      }
    }
    // dessin complet : chaque pixel de l'ecran est converti dans le repere du bac (rotation inverse autour du pivot)
    for (int y = 0; y < H; y++) {
      const float dy = PY - (y + 0.5f);
      for (int x = 0; x < W; x++) {
        const float dx = (x + 0.5f) - PX;
        const float u = dx * cs - dy * sn, v = dx * sn + dy * cs;
        const float fu = u + NW * 0.5f;
        uint16_t col = 0;
        // v378 : contour du bac LISSE (couverture de chaque pixel par le rectangle exterieur moins le rectangle interieur, calculee dans le repere du bac)
        float cin = fu < NW - fu ? fu : NW - fu; if (v < cin) cin = v;
        float cout = (fu + WT) < (NW + WT - fu) ? (fu + WT) : (NW + WT - fu); if (v + FL < cout) cout = v + FL; if (WH - v < cout) cout = WH - v;
        const float covIn = cin <= -0.5f ? 0.0f : (cin >= 0.5f ? 1.0f : cin + 0.5f);
        const float covOut = cout <= -0.5f ? 0.0f : (cout >= 0.5f ? 1.0f : cout + 0.5f);
        float rr = 0, gg = 0, bb = 0;
        if (covIn > 0.0f) {
          float fi = fu - 0.5f; if (fi < 0) fi = 0; if (fi > NW - 1.001f) fi = NW - 1.001f;
          const int i0 = (int)fi; const float tt = fi - i0, hs = sf[i0] + (sf[i0 + 1] - sf[i0]) * tt;
          const float d = hs - v;
          if (hs > 0.15f && d > -0.5f) {
            const float cov = d > 0.5f ? 1.0f : d + 0.5f;
            float dep = d / 22.0f; if (dep > 1.0f) dep = 1.0f;
            const float k = 1.0f - 0.35f * dep;
            rr = oc[0] * k; gg = oc[1] * k; bb = oc[2] * k;
            if (d < 1.0f) { rr = rr * 0.55f + 255 * 0.45f; gg = gg * 0.55f + 255 * 0.45f; bb = bb * 0.55f + 255 * 0.45f; }
            rr *= cov; gg *= cov; bb *= cov;
          }
          rr *= covIn; gg *= covIn; bb *= covIn;
        }
        const float wc = covOut * (1.0f - covIn);
        if (wc > 0.0f) {
          float hi = 1.2f + cin; if (hi < 0) hi = 0; if (hi > 1) hi = 1;
          float wr = wR + (hR - wR) * hi, wg = wG + (hG - wG) * hi, wb = wB + (hB - wB) * hi;
          if (adh > 0.0f && v > 0.0f && cin > -1.2f) {   // film de liquide colle a la paroi (jusqu'a la hauteur mouillee)
            const int sd = (fu < NW * 0.5f) ? 0 : 1;
            if (v < wetH[sd]) { const float fa = adh * 0.75f * (1.0f - 0.5f * v / (wetH[sd] + 0.001f)); wr += (oc[0] - wr) * fa; wg += (oc[1] - wg) * fa; wb += (oc[2] - wb) * fa; }
          }
          rr += wr * wc; gg += wg * wc; bb += wb * wc;
        }
        if (rr > 255) rr = 255;
        if (gg > 255) gg = 255;
        if (bb > 255) bb = 255;
        col = display->color565((uint8_t)rr, (uint8_t)gg, (uint8_t)bb);
        if (col == 0 && dy < 0 && fabsf(dx) <= -dy * 0.6f) col = triC;
        display->drawPixel(x, y, col);
      }
    }
    // gouttelettes : trajectoire balistique a l'ecran, reabsorbees par la nappe, perdues contre une paroi ou hors de l'ecran
    for (int i = 0; i < ND; i++) if (dal[i]) {
      dvy[i] += 0.16f; dpx[i] += dvx[i]; dpy[i] += dvy[i];
      if (dpx[i] < -40 || dpx[i] >= W + 40 || dpy[i] >= H + 8) { dal[i] = false; continue; }   // v380 : les gouttes peuvent naitre hors de la dalle (paroi basse du bac) et y rentrer
      const float ddx = dpx[i] - PX, ddy = PY - dpy[i];
      const float u = ddx * cs - ddy * sn, v = ddx * sn + ddy * cs, fu = u + NW * 0.5f;
      if (fu < 1 || fu >= NW - 1) { dal[i] = false; continue; }
      if (v < 0 || (dvy[i] > 0 && v < sf[(int)fu])) { dal[i] = false; rv[(int)fu] += 0.25f * rexc; continue; }   // v380 : reabsorbee seulement en retombant
      if (dpy[i] >= 0 && dpx[i] >= 0 && dpx[i] < W) display->drawPixel((int)dpx[i], (int)dpy[i], dropC);
    }
  }
  return true;
}

// -- SABLE DE COULEURS : coulees successives d'un point fixe (leger balancement), une couleur par coulee, buttes qui s'empilent --
static bool decoAmbSand(unsigned long durMs, bool preview)
{
  uint8_t *g = s_amb.sand; memset(g, 0, 2048);   // 4 bits par cellule : 0 = vide, 1..6 = couleur
  auto get = [&](int x, int y) -> int { const int i = y * 128 + x; const uint8_t b = g[i >> 1]; return (i & 1) ? (b >> 4) : (b & 15); };
  auto put = [&](int x, int y, int c) { const int i = y * 128 + x; uint8_t &b = g[i >> 1]; b = (i & 1) ? (uint8_t)((b & 0x0F) | (c << 4)) : (uint8_t)((b & 0xF0) | c); };
  const uint16_t col[7] = {0, display->color565(238, 205, 120), display->color565(205, 95, 55), display->color565(240, 232, 205),
                           display->color565(60, 170, 160), display->color565(150, 90, 175), display->color565(140, 98, 60)};
  display->clearScreen();
  const unsigned long t0 = millis(); unsigned long last = 0; unsigned frame = 0; int count = 0, blockedRun = 0;   // v296 : blockedRun = grains perdus d'affilee (haut bloque)
  int x0 = 64, colorIdx = 0, remain = 0, rate = 1, idle = 0; float amp = 3, freq = 0.7f, tp = 0;
  auto newPour = [&]() {   // nouvelle coulee : point fixe distinct, couleur differente, amplitude / duree / debit aleatoires
    int nx = x0;
    for (int tries = 0; tries < 8; tries++) { nx = (int)random(16, 112); if (abs(nx - x0) >= 24) break; }
    x0 = nx;
    int c; do { c = (int)random(1, 7); } while (c == colorIdx); colorIdx = c;
    amp = random(10, 90) / 10.0f; freq = random(40, 120) / 100.0f; remain = (int)random(110, 381); rate = (int)random(1, 4); tp = 0;
  };
  newPour();
  while (millis() - t0 < durMs) {
    if (decoStop(preview)) return false;
    if (millis() - last < 22) { delay(2); continue; }
    last = millis(); frame++;
    if (remain > 0) {
      for (int k = 0; k < rate && remain > 0; k++) {
        const int sx = x0 + (int)lroundf(amp * sinf(2.0f * (float)M_PI * freq * tp)); tp += 0.012f;
        if (sx >= 0 && sx < 128 && get(sx, 0) == 0) { put(sx, 0, colorIdx); display->drawPixel(sx, 0, col[colorIdx]); remain--; count++; blockedRun = 0; }
        else { remain--; blockedRun++; }   // case occupee : le grain est perdu (evite une boucle si la colonne est pleine)
      }
      if (remain == 0) idle = (int)random(4, 30);
    } else if (--idle <= 0) newPour();
    const bool ltr = (frame & 1);
    for (int y = 30; y >= 0; y--) {
      for (int xi = 0; xi < 128; xi++) {
        const int x = ltr ? xi : 127 - xi; const int c = get(x, y);
        if (!c) continue;
        int nx = x;
        if (get(x, y + 1) != 0) {
          const int dd = random(0, 2) ? 1 : -1;
          if (x + dd >= 0 && x + dd < 128 && get(x + dd, y + 1) == 0) nx = x + dd;
          else if (x - dd >= 0 && x - dd < 128 && get(x - dd, y + 1) == 0) nx = x - dd;
          else continue;
        }
        put(x, y, 0); display->drawPixel(x, y, 0); put(nx, y + 1, c); display->drawPixel(nx, y + 1, col[c]);
      }
    }
    if (count >= 3891 || blockedRun >= 400) {   // v296 : ecran REMPLI a >= 95 % (ou haut bloque) : balayage vers le bas puis nouvelles buttes
      for (int y = 0; y < 32; y++) { for (int x = 0; x < 128; x++) display->drawPixel(x, y, 0); if (decoStop(preview)) return false; delay(18); }
      memset(g, 0, 2048); count = 0; blockedRun = 0; newPour();
    }
  }
  return true;
}
// -- SABLE 2 (strates) : inspire du "Moving Sand Art". Une reserve de sable en ZONES de couleur (bandes epaisses, a frontieres ondulees), tres epaisse (elle depasse au-dessus
// de l'ecran), repose sur une etagere invisible percee de 1 a 3 breches d'1 pixel. Le sable glisse en pente douce vers la breche la plus proche (entonnoirs qui creusent les
// strates), tombe en filets fins et edifie en bas un cone par breche, qui garde l'ordre des couleurs. Quand une breche est bouchee par son cone, on tente d'en ouvrir de
// nouvelles (0 a 3 au hasard, ailleurs). Toutes bouchees et rien d'ajoute (ou reserve vide) : balayage, nouvelle palette, nouvelle reserve. La grille fait 64 lignes : les 32 du haut sont hors ecran. --
static bool decoAmbSand2(unsigned long durMs, bool preview)
{
  const int W = 128, H = 32, GH = 64, GMAX = 8;
  if (ESP.getMaxAllocHeap() < 4096 + 8000) { Serial.println("[DECO] sable2 : tas insuffisant"); return true; }
  uint8_t *g = (uint8_t *)malloc(W * GH / 2);
  if (!g) return true;
  memset(g, 0, W * GH / 2);   // 4 bits par cellule : 0 = vide, 1..4 = couleur de la palette ; ligne gy visible a l'ecran en y = gy - 32
  auto get = [&](int x, int y) -> int { const int i = y * W + x; const uint8_t b = g[i >> 1]; return (i & 1) ? (b >> 4) : (b & 15); };
  auto put = [&](int x, int y, int c) { const int i = y * W + x; uint8_t &b = g[i >> 1]; b = (i & 1) ? (uint8_t)((b & 0x0F) | (c << 4)) : (uint8_t)((b & 0xF0) | c); };
  static const uint8_t PAIR[6][2][3] = {{{245, 245, 245}, {60, 60, 70}}, {{0, 80, 255}, {255, 30, 30}}, {{255, 220, 0}, {130, 0, 200}},
                                        {{0, 200, 70}, {255, 0, 140}}, {{0, 220, 255}, {255, 110, 0}}, {{255, 255, 255}, {0, 90, 255}}};   // paires TRES contrastees
  uint16_t col[5]; col[0] = 0;
  int SY = 37, ng = 1, xgs[GMAX], blk[GMAX], quiet = 0, nMoves = 0; bool dead[GMAX], lifted = false, done = false; unsigned frame = 0, hold = 0;
  unsigned long cycleT0 = 0, liftT = 0; bool cycleOpen = true;
  unsigned gcnt[GMAX] = {0}; int cbase = 0; unsigned gAge[GMAX] = {0}, gLife[GMAX] = {0}; bool hv[GMAX][5] = {{false}}; float gPh[GMAX] = {0}, gSp[GMAX] = {0}, curK[GMAX] = {0};   // v372 : poids PAR COULEUR ET PAR BRECHE (hv, true = lourd / amas)   // v371 : courant lateral et phase des nuages PROPRES a chaque breche   // v369 : grains passes par chaque breche (la couleur d'un grain est decidee a la breche)
  uint8_t bandCol[200], hgt[128], near[128], hp[128], slideP[128]; bool isGap[128]; float curPh = 0, curSp = 0.03f;   // v306 : irregularite : probabilite de glissement PAR COLONNE (sable plus ou moins collant) et courant lateral lent dans l'eau
  auto dr = [&](int x, int gy, int c) { if (gy >= 32) display->drawPixel(x, gy - 32, c ? col[c] : 0); };
  auto coneCol = [&](int xg) -> int {   // v369 : couche de couleur suivante du cone de CETTE breche : la hauteur d'un cone croit comme sqrt(grains) -> bandes fines et regulieres qui suivent la pente (chevrons)
    int k = 0; for (int j = 0; j < ng; j++) if (xgs[j] == xg) { k = j; break; }
    int lv = cbase + (int)sqrtf((float)(gcnt[k]++)); if (lv > 199) lv = 199; return bandCol[lv];
  };
  auto recomputeNear = [&]() {   // breche ACTIVE la plus proche de chaque colonne (255 = aucune)
    for (int x = 0; x < W; x++) { int b = 255, bd = 999; for (int k = 0; k < ng; k++) if (!dead[k]) { const int dd = abs(x - xgs[k]); if (dd < bd) { bd = dd; b = k; } } near[x] = (uint8_t)b; }
  };
  auto addGaps = [&](int n) {   // v375 : ouvre jusqu'a n breches ailleurs (pas a moins de 16 px d'une autre, sans cone sous l'etagere), 6 ACTIVES AU MAXIMUM ; reutilise l'emplacement d'une breche terminee
    for (int a = 0; a < n; a++) {
      int active = 0, slot = -1; for (int j = 0; j < ng; j++) { if (!dead[j]) active++; else if (slot < 0) slot = j; }
      if (active >= 6) break;
      if (slot < 0) { if (ng >= GMAX) break; slot = ng; }
      for (int tries = 0; tries < 40; tries++) {
        const int c = (int)random(8, 120); bool ok = (get(c, SY) == 0 && get(c, SY + 1) == 0 && get(c, SY - 1) != 0);
        for (int j = 0; j < ng && ok; j++) if (j != slot && abs(c - xgs[j]) < 16) ok = false;
        if (ok) {
          if (slot < ng) isGap[xgs[slot]] = false; else ng++;
          xgs[slot] = c; blk[slot] = 0; dead[slot] = false; isGap[c] = true; gAge[slot] = 0; gLife[slot] = (unsigned)random(300, 1201);   // duree de vie aleatoire : 10 a 40 s environ
          { const bool r = random(0, 2); hv[slot][1] = r; hv[slot][2] = !r; hv[slot][3] = random(0, 2); hv[slot][4] = random(0, 2); }
          gPh[slot] = random(0, 628) / 100.0f; gSp[slot] = (random(0, 2) ? 1 : -1) * random(50, 101) / 100.0f; /* courant constant pendant la vie de la breche */
          break;
        }
      }
    }
    recomputeNear();
  };
  auto newReserve = [&]() {
    const uint8_t (*pl)[3] = PAIR[random(0, 6)];   // DEUX couleurs seulement, tranchees
    col[1] = decoAmbCol[7][0] >= 0 ? display->color565((decoAmbCol[7][0] >> 16) & 255, (decoAmbCol[7][0] >> 8) & 255, decoAmbCol[7][0] & 255) : display->color565(pl[0][0], pl[0][1], pl[0][2]);   // v311 : couleurs choisies
    col[2] = decoAmbCol[7][1] >= 0 ? display->color565((decoAmbCol[7][1] >> 16) & 255, (decoAmbCol[7][1] >> 8) & 255, decoAmbCol[7][1] & 255) : display->color565(pl[1][0], pl[1][1], pl[1][2]); { const int r1 = (col[1] >> 11) & 31, g1 = (col[1] >> 5) & 63, b1 = col[1] & 31, r2 = (col[2] >> 11) & 31, g2 = (col[2] >> 5) & 63, b2 = col[2] & 31;   // v368 : 2 tons de plus : melange (liseré) et teinte claire
      col[3] = (uint16_t)((((r1 + r2) / 2) << 11) | (((g1 + g2) / 2) << 5) | ((b1 + b2) / 2));
      col[4] = (uint16_t)((((r1 + 31) / 2) << 11) | (((g1 + 63) / 2) << 5) | ((b1 + 31) / 2)); }
    SY = 30;   // v371 : etagere 2 rangees au-dessus de l'ecran (les pointes des cones peuvent depasser le bord haut sans boucher) ; v370 : reserve HORS ECRAN (au-dessus du DMD) : les filets sortent par le bord haut de la dalle
    for (int x = 0; x < W; x++) isGap[x] = false;
    memset(g, 0, W * GH / 2);
    { int k = 0, cc = (int)random(1, 3); while (k < 200) {   // v368 : STRATES FINES, d'epaisseur irreguliere (ton principal 3-7 rangees, contraste 2-4, liseres de 1-2) comme dans la vraie machine a sable
        const int r = random(0, 100); const int tone = (r < 45) ? 1 : (r < 70) ? 2 : (r < 88) ? 3 : 4; cc = tone == cc ? ((tone % 4) + 1) : tone;
        const int th = (cc == 1) ? (int)random(4, 10) : (cc == 2) ? (int)random(3, 7) : (int)random(1, 3);
        for (int j = 0; j < th && k < 200; j++) bandCol[k++] = (uint8_t)cc; } }
    const int T = 27;   // v371 : reserve calibree 27 x 128 = 3456 grains = 84 % de la dalle (v370 : 13 rangees = 40 %, trop de vide) : une fois TOUT tombe il reste du vide en haut ; v306 : 32 rangees
    // : reserve de 32 rangees x 128 = 4096 grains = EXACTEMENT le nombre de pixels de la dalle (remplissage final visee : 100 %)
    const float p1 = random(0, 628) / 100.0f, p2 = random(0, 628) / 100.0f, a1 = random(20, 45) / 10.0f, a2 = random(5, 12) / 10.0f, bt = random(80, 150) / 10.0f; const int off = (int)random(0, 4);
    for (int gy = SY - T; gy < SY; gy++) for (int x = 0; x < W; x++) {   // zones : bandes de 8 a 15 lignes, frontieres ondulees douces
      const float v = gy + a1 * sinf(x * 0.045f + p1) + a2 * sinf(x * 0.13f + p2) + 40.0f;
      int vi = (int)v; if (vi < 0) vi = 0; if (vi > 199) vi = 199;
      const int c = bandCol[vi];
      put(x, gy, c); dr(x, gy, c);
    }
    { const float q1 = random(0, 628) / 100.0f, q2 = random(0, 628) / 100.0f; for (int x = 0; x < W; x++) slideP[x] = (uint8_t)(25 + 60 * (0.5f + 0.25f * sinf(x * 0.06f + q1) + 0.25f * sinf(x * 0.17f + q2))); }
    curPh = random(0, 628) / 100.0f; curSp = random(15, 40) / 1000.0f;
    for (int j = 0; j < GMAX; j++) gcnt[j] = 0; cbase = (int)random(0, 60);
    ng = 0; lifted = false; done = false; quiet = 0; hold = 0; cycleT0 = millis(); addGaps(1);
    const int extra = (int)random(2, 4);   // v371 : de 3 a 4 breches au depart
    addGaps(extra);
  };
  display->clearScreen();
  newReserve();
  const unsigned long t0 = millis(); unsigned long last = 0;
  while (millis() - t0 < durMs || (cycleOpen && millis() - t0 < 150000UL)) {   // v306e : la duree est un MINIMUM, on finit toujours le paysage en cours
    if (decoStop(preview)) { free(g); return false; }
    if (millis() - last < 30) { delay(2); continue; }
    last = millis(); frame++;
    if (hold) {   // paysage termine : on le laisse admirer, puis balayage et nouvelle reserve
      if (--hold == 0) {
        cycleOpen = false;
        if (millis() - t0 >= durMs) break;   // duree minimale atteinte et paysage termine : fin de l'ambiance
        for (int y = 0; y < H; y++) { for (int x = 0; x < W; x++) display->drawPixel(x, y, 0); if (decoStop(preview)) { free(g); return false; } delay(18); }
        newReserve(); cycleOpen = true;
      }
      continue;
    }
    nMoves = 0;
    if (!lifted && !done) {
      // 1) derive en pente douce : le grain de surface glisse d'une colonne vers la breche active la plus proche si la colonne voisine (cote breche) est plus basse
      for (int x = 0; x < W; x++) { int t = 0; while (t < SY && get(x, t) == 0) t++; hgt[x] = (uint8_t)(SY - t); }
      for (int d = 1; d < 128; d++) for (int k = 0; k < ng; k++) for (int side = 0; side < 2; side++) {
        if (dead[k]) continue;
        const int x = side ? xgs[k] + d : xgs[k] - d, s = side ? -1 : 1;
        if (x < 0 || x >= W || near[x] != k || hgt[x] == 0 || random(0, 100) >= 40) continue;
        const int xt = x + s, ty = SY - hgt[x];
        if (xt == xgs[k]) {   // dans la breche : il tombe
          if (get(xt, SY) == 0) { const int c = get(x, ty); put(x, ty, 0); dr(x, ty, 0); put(xt, SY, c); dr(xt, SY, c); hgt[x]--; nMoves++; }
        } else if (hgt[x] > hgt[xt] && SY - hgt[xt] - 1 >= 0) {
          const int c = get(x, ty), ny = SY - hgt[xt] - 1;
          put(x, ty, 0); dr(x, ty, 0); put(xt, ny, c); dr(xt, ny, c); hgt[x]--; hgt[xt]++; nMoves++;
        }
      }
    }
    // 2) chute / eboulement ; sous l'etagere seules les breches sont franchissables (jusqu'a ce que l'etagere soit levee)
    for (int k = 0; k < ng; k++) curK[k] = gSp[k];   // v374 : constant (v373 : sinus + amplitude modulee = grains legers repartis de part et d'autre du cone, pyramides)   // v371 : un courant par breche (sens et periode differents : plus de symetrie)
    const float cur = 0;   // courant lateral lent (-1..1) : fait deriver le filet, le cone n'est plus centre ni regulier
    const int passes = lifted ? 2 : 1;
    for (int pass = 0; pass < passes; pass++) {
      const bool ltr = ((frame + pass) & 1);
      for (int y = GH - 2; y >= 0; y--) for (int xi = 0; xi < W; xi++) {
        const int x = ltr ? xi : W - 1 - xi, c = get(x, y);
        if (!c) continue;
        int nx = x, ny = y + 1;
        const bool cross = (!lifted && y == SY - 1), water = (lifted || y >= SY - 1);   // sous l'etagere : le sable chute DANS L'EAU (plus lent, flotte et oscille)
        if (get(x, y + 1) != 0 || (cross && !isGap[x])) {   // pose : glisse en pente, lentement (l'eau freine l'eboulement)
          if (random(0, 100) >= (lifted ? 35 + (int)(slideP[x] * 0.6f) : (int)slideP[x])) continue;   // sable plus ou moins collant selon la colonne
          const int dd = random(0, 2) ? 1 : -1; bool moved = false;
          for (int k = 0; k < 2 && !moved; k++) {
            const int tx = x + (k ? -dd : dd);
            if (tx >= 0 && tx < W && get(tx, y + 1) == 0 && (!cross || isGap[tx]) && (lifted || slideP[x] >= 20 || ny + 2 >= GH || get(tx, y + 2) == 0)) { nx = tx; moved = true; }   // v369 : colonne collante = pente plus raide (cones irreguliers)
          }
          if (!moved) continue;
          { const int dxs = nx - x; for (int r = 0; r < 6; r++) {   // le grain peut continuer a rouler sur la pente (eboulis, pente irreguliere)
              if (random(0, 100) >= 50) break; const int tx2 = nx + dxs; if (tx2 < 0 || tx2 >= W || ny + 1 >= GH || get(nx, ny + 1) == 0 || get(tx2, ny + 1) != 0) break; nx = tx2; ny++; } }
        } else if (water) {   // chute libre : couleur LEGERE (grains seuls : lente, tres sensible au courant de sa breche = nuage) ou LOURDE (amas : plus rapide, presque verticale) ; poids propre a chaque couleur et a chaque breche, donc une strate se comporte d'un bloc
          const int kk = (near[x] == 255) ? 0 : near[x]; const float cu = curK[kk];
          const bool heavy = hv[kk][c];
          if (random(0, 100) >= (heavy ? 30 : 18)) continue;   // v373 : chute plus lente (v372 : 58 / 40)
          { const int r = random(0, 100); int dir = 0; const int base = heavy ? 3 : 25, sens = heavy ? 30 : 95;   // v373 : derive nettement plus forte (v372 : 1/6 et 12/55) : le filet balaie une zone, collines arrondies
            if (r < base) dir = random(0, 2) ? 1 : -1; else if (r < base + (int)(sens * fabsf(cu))) dir = cu > 0 ? 1 : -1;
            if (dir) { const int tx = x + dir; if (tx >= 0 && tx < W && get(tx, y + 1) == 0 && get(tx, y) == 0 && (!cross || isGap[tx])) nx = tx; } }
        }
        const int c2 = c;   // v372 : la couleur vient de la reserve (plus de recoloration a la breche)   // v369 : passage de la breche = recoloration
        put(x, y, 0); dr(x, y, 0); put(nx, ny, c2); dr(nx, ny, c2); nMoves++;
      }
    }
    if (!lifted) {   // v376 : GLISSEMENT DE STRATE : un grain LOURD pose sur un grain LEGER entraine une plaque de 1 a 3 grains vers le bas de la pente (gravite), l'ordre des couleurs est conserve
      for (int x = 0; x < W; x++) { int h = 0; while (h < GH && get(x, GH - 1 - h) != 0) h++; hp[x] = (uint8_t)h; }
      for (int x = 0; x < W; x++) {
        if (hp[x] < 3 || random(0, 100) >= 12) continue;
        const int ty0 = GH - hp[x], kk = (near[x] == 255) ? 0 : near[x];
        if (ty0 < SY + 1) continue;
        if (!hv[kk][get(x, ty0)] || hv[kk][get(x, ty0 + 1)]) continue;
        const int nslab = 1 + (int)random(0, 3);
        int bestE = -1, bestDrop = 0;
        for (int sg = 0; sg < 2; sg++) {
          const int d = sg ? 1 : -1; int e = x, st = 0;
          while (st < 40) { const int n2 = e + d; if (n2 < 0 || n2 >= W || hp[n2] >= hp[e]) break; e = n2; st++; }
          const int drop = (int)hp[x] - (int)hp[e]; if (e != x && drop >= 2 && drop > bestDrop) { bestDrop = drop; bestE = e; }
        }
        if (bestE < 0) continue;
        int n = nslab < bestDrop / 2 ? nslab : bestDrop / 2; if (n < 1) continue;
        int cs[3];
        for (int s = 0; s < n; s++) { const int sy = GH - hp[x]; cs[s] = get(x, sy); put(x, sy, 0); dr(x, sy, 0); hp[x]--; }
        for (int s = n - 1; s >= 0; s--) { const int ty = GH - 1 - hp[bestE]; if (ty < 0) break; put(bestE, ty, cs[s]); dr(bestE, ty, cs[s]); hp[bestE]++; }
        nMoves++;
      }
    }
    if (lifted) {   // 3) nivellement : le grain de sommet d'une colonne DESCEND la pente (jusqu'au minimum local, 48 colonnes max) si la chute est >= 2 : les pyramides se resorbent, le sable s'etale en paysage plein
      for (int x = 0; x < W; x++) { int h = 0; while (h < GH && get(x, GH - 1 - h) != 0) h++; hp[x] = (uint8_t)h; }
      for (int xi = 0; xi < W; xi++) {
        const int x = (frame & 1) ? xi : W - 1 - xi;
        if (hp[x] < 2 || random(0, 100) >= 45) continue;
        int bestE = -1, bestDrop = 1;
        for (int sg = 0; sg < 2; sg++) {
          const int d = sg ? 1 : -1; int e = x, st = 0;
          while (st < 48) { const int n2 = e + d; if (n2 < 0 || n2 >= W || hp[n2] >= hp[e]) break; e = n2; st++; }
          const int drop = (int)hp[x] - (int)hp[e]; if (e != x && drop >= 2 && drop > bestDrop) { bestDrop = drop; bestE = e; }
        }
        if (bestE < 0) continue;
        const int sy = GH - hp[x], ty = GH - 1 - hp[bestE]; if (ty < 0) continue;
        const int c = get(x, sy); put(x, sy, 0); dr(x, sy, 0); put(bestE, ty, c); dr(bestE, ty, c); hp[x]--; hp[bestE]++; nMoves++;
      }
    }    // 4) fin de cycle : breche bouchee par son cone -> morte (+ 0 a 3 nouvelles breches) ; plus aucune active (ou reserve vide) -> l'etagere LEVE : toute la reserve s'effondre
    //    sur les cones puis se nivelle ; quand plus rien ne bouge : paysage plein (mesure du remplissage), pause, balayage
    if (!done) {
      bool changed = false; int deaths = 0;
      for (int k = 0; k < ng; k++) {
        if (dead[k]) continue;
        const int xg = xgs[k];
        if (get(xg, SY + 1) != 0 && xg > 0 && xg < W - 1 && get(xg - 1, SY + 2) != 0 && get(xg, SY + 2) != 0 && get(xg + 1, SY + 2) != 0) blk[k]++; else blk[k] = 0;
        if (blk[k] > 50) { dead[k] = true; changed = true; deaths++; }   // bouchee par son cone
        else if (++gAge[k] > gLife[k]) { dead[k] = true; isGap[xg] = false; changed = true; deaths++; }   // v375 : fin de vie (duree aleatoire)
      }
      if (changed) { recomputeNear(); for (int d = 0; d < deaths; d++) addGaps((int)random(0, 4));   // v375 : 0 a 3 nouvelles breches a chaque breche terminee
        int al = 0; for (int k = 0; k < ng; k++) if (!dead[k]) al++;
        int rest = 0; for (int x = 0; x < W; x++) rest += hgt[x];
        if (al == 0 && rest > 40) addGaps(1); }   // jamais sans breche tant qu'il reste du sable
      int alive = 0; for (int k = 0; k < ng; k++) if (!dead[k]) alive++;
      int left = 0; for (int x = 0; x < W; x++) left += hgt[x];
      if (alive == 0 || left <= 40) { done = true; quiet = 0; liftT = millis(); }   // v370 : reserve vidée : PAS d'effondrement, on laisse seulement finir de tomber
    } else {
      quiet = (nMoves == 0) ? quiet + 1 : 0;
      if (quiet >= 20 || millis() - liftT > 90000UL) {
        int fill = 0; for (int y = 32; y < GH; y++) for (int x = 0; x < W; x++) if (get(x, y)) fill++;
        Serial.println("[DECO] sable2 cycle : chute " + String((liftT - cycleT0) / 1000.0f, 1) + " s + effondrement/nivellement " + String((millis() - liftT) / 1000.0f, 1) + " s = " + String((millis() - cycleT0) / 1000.0f, 1) + " s, remplissage " + String(fill * 100.0f / 4096.0f, 1) + " %");
        hold = 130;   // ~4 s de contemplation
      }
    }
  }
  free(g);
  return true;
}// -- LAVA LAMP (ambiance 8) : couleur de l'HUILE et couleur de la CIRE au choix (page Widgets) ou tirees au hasard ; la temperature de l'huile monte pendant l'ambiance (cire de plus en plus elastique,
// excroissances de plus en plus hautes). Voir le bloc LAVA LAMP plus haut. --
static bool decoAmbLava(unsigned long durMs, bool preview)
{
  LavaSt *L = (LavaSt *)s_amb.sand;
  static const uint8_t OILS[6][3] = {{8, 20, 80}, {52, 8, 90}, {0, 60, 62}, {80, 30, 8}, {14, 44, 18}, {10, 10, 44}};      // bleu nuit, violet, sarcelle, brun, vert sombre, indigo
  static const uint8_t WAXS[6][3] = {{255, 110, 0}, {255, 60, 150}, {255, 225, 40}, {0, 225, 255}, {120, 255, 60}, {255, 60, 40}};   // orange, rose, jaune, cyan, vert, rouge
  uint8_t oil[3], wax[3];
  if (decoAmbCol[8][0] >= 0) { oil[0] = (decoAmbCol[8][0] >> 16) & 255; oil[1] = (decoAmbCol[8][0] >> 8) & 255; oil[2] = decoAmbCol[8][0] & 255; } else memcpy(oil, OILS[random(0, 6)], 3);
  if (decoAmbCol[8][1] >= 0) { wax[0] = (decoAmbCol[8][1] >> 16) & 255; wax[1] = (decoAmbCol[8][1] >> 8) & 255; wax[2] = decoAmbCol[8][1] & 255; } else memcpy(wax, WAXS[random(0, 6)], 3);
  lavaInit(L, oil, wax);
  Serial.println(String("[DECO] lava lamp : huile ") + oil[0] + "," + oil[1] + "," + oil[2] + " cire " + wax[0] + "," + wax[1] + "," + wax[2] + " heap=" + String(ESP.getFreeHeap()));
  display->clearScreen();
  const unsigned long t0 = millis(); unsigned long last = 0;
  const float ramp = (float)((durMs * 3UL / 4UL) < 45000UL ? (durMs * 3UL / 4UL) : 45000UL) / 1000.0f;   // la temperature monte de 0 % (cire figee) a 100 % sur ~75 % de la duree (45 s max)
  uint8_t lv[128];
  while (millis() - t0 < durMs) {
    if (decoStop(preview)) return false;
    if (millis() - last < 33) { delay(2); continue; }
    last = millis();
    const float t = (millis() - t0) / 1000.0f, tg = (t >= ramp || ramp < 0.5f) ? 1.0f : ((t / ramp < 0.1f) ? 0.0f : (t / ramp - 0.1f) / 0.9f);   // v306 : depart a FROID (0 %), puis chauffe progressive
    lavaStep(L, tg, t); lavaBuild(L, tg, t);
    for (int y = 0; y < 32; y++) { lavaRow(y, lv); for (int x = 0; x < 128; x++) display->drawPixel(x, y, lavaColor(L, x, y, lv[x])); }
  }
  return true;
}static const char *decoAmbName(int s) { return s == 1 ? "plasma" : s == 2 ? "pluie" : s == 3 ? "etoiles" : s == 4 ? "vie" : s == 5 ? "liquide" : s == 6 ? "sable" : s == 7 ? "sable2" : "lava"; }

static bool decoRunAmbiance(bool preview)
{
  int s = decoAmbStyle;
  static int s_lastAmb = 0;
  if (s < 1 || s > 8) { do { s = random(1, 9); } while (s == s_lastAmb); }
  s_lastAmb = s;
  const unsigned long dur = (unsigned long)decoDur[4] * 1000UL;
  Serial.println(String("[DECO] widget ambiance ") + decoAmbName(s) + " (" + String(decoDur[4]) + " s) heap=" + String(ESP.getFreeHeap()));
  bool ok = (s == 1) ? decoAmbPlasma(dur, preview) : (s == 2) ? decoAmbRain(dur, preview) : (s == 3) ? decoAmbStars(dur, preview) : (s == 4) ? decoAmbLife(dur, preview) : (s == 5) ? decoAmbLiquid(dur, preview) : (s == 6) ? decoAmbSand(dur, preview) : (s == 7) ? decoAmbSand2(dur, preview) : decoAmbLava(dur, preview);
  display->clearScreen();
  return ok;
}


// ---- meteo : donnees, requete Open-Meteo, classes (firmware, lignes 14267-14330) ----
struct DecoWx { bool valid; float temp, tmax, tmin; int code; bool day; unsigned long fetchedMs; float lat, lon; };   // v353 : lat/lon = lieu des valeurs gardees
static DecoWx s_wx = {false, 0, 0, 0, 0, true, 0, 0, 0};
static unsigned long s_wxLastTry = 0; static bool s_wxTried = false;

static bool decoJsonNum(const char *from, const char *key, double &out)
{
  const char *p = strstr(from, key);
  if (!p) return false;
  p += strlen(key);
  while (*p == ' ' || *p == '[') p++;
  char *e; out = strtod(p, &e);
  return e != p;
}

static bool decoWeatherFetch()
{
  s_wxTried = true; s_wxLastTry = millis();
  if (WiFi.status() != WL_CONNECTED) { Serial.println("[WX] pas de Wi-Fi"); return false; }
  const uint32_t maxA = ESP.getMaxAllocHeap();
  if (maxA < 12000) { Serial.println("[WX] tas insuffisant maxalloc=" + String(maxA)); return false; }
  const unsigned long t0 = millis();
  WiFiClient c;
  if (!c.connect("api.open-meteo.com", 80, 3000)) { Serial.println("[WX] connexion impossible (" + String(millis() - t0) + " ms)"); return false; }
  char req[420];   // v277 : 300 octets tronquaient la requete (307 o necessaires)
  const int rl = snprintf(req, sizeof(req),
           "GET /v1/forecast?latitude=%.4f&longitude=%.4f&current=temperature_2m,weather_code,is_day&daily=temperature_2m_max,temperature_2m_min&timezone=auto&forecast_days=1 HTTP/1.1\r\n"
           "Host: api.open-meteo.com\r\nUser-Agent: RecalBoxDMD\r\nAccept: application/json\r\nAccept-Encoding: identity\r\nConnection: close\r\n\r\n",
           decoLat, decoLon);
  if (rl <= 0 || rl >= (int)sizeof(req)) { Serial.println("[WX] requete tronquee (" + String(rl) + " o)"); c.stop(); return false; }
  c.print(req);
  char buf[1100]; size_t n = 0;   // v339 : plus statique (1,1 Ko de statique en moins)
  while (millis() - t0 < 3500 && (c.connected() || c.available()) && n < sizeof(buf) - 1) {
    const int a = c.available();
    if (a > 0) { const int r = c.read((uint8_t *)buf + n, (a < (int)(sizeof(buf) - 1 - n)) ? a : (int)(sizeof(buf) - 1 - n)); if (r > 0) n += (size_t)r; }
    else { delay(5); yield(); }
  }
  buf[n] = 0; c.stop();
  const char *cur = strstr(buf, "\"current\":{"), *dai = strstr(buf, "\"daily\":{");
  double t = 0, wc = 0, id = 1, mx = 0, mn = 0;
  if (!cur || !dai || !decoJsonNum(cur, "\"temperature_2m\":", t) || !decoJsonNum(cur, "\"weather_code\":", wc) || !decoJsonNum(cur, "\"is_day\":", id)
      || !decoJsonNum(dai, "\"temperature_2m_max\":", mx) || !decoJsonNum(dai, "\"temperature_2m_min\":", mn)) {
    Serial.println("[WX] reponse inexploitable (" + String((unsigned)n) + " o, " + String(millis() - t0) + " ms)");
    return false;
  }
  s_wx.temp = (float)t; s_wx.code = (int)wc; s_wx.day = (id != 0); s_wx.tmax = (float)mx; s_wx.tmin = (float)mn; s_wx.fetchedMs = millis(); s_wx.lat = decoLat; s_wx.lon = decoLon; s_wx.valid = true;
  Serial.println("[WX] OK " + String(t, 1) + " C code=" + String((int)wc) + " " + String(mn, 0) + "/" + String(mx, 0) + " (" + String((unsigned)n) + " o, " + String(millis() - t0) + " ms, maxalloc avant=" + String(maxA) + " apres=" + String(ESP.getMaxAllocHeap()) + ")");
  return true;
}

// classes : 0 clair, 1 eclaircies, 2 couvert, 3 brume, 4 bruine, 5 pluie, 6 neige, 7 averses, 8 orage
static int decoWxClass(int code)
{
  if (code == 0) return 0;
  if (code == 1 || code == 2) return 1;
  if (code == 45 || code == 48) return 3;
  if (code >= 51 && code <= 57) return 4;
  if (code >= 61 && code <= 67) return 5;
  if (code >= 71 && code <= 77) return 6;
  if (code >= 80 && code <= 82) return 7;
  if (code == 85 || code == 86) return 6;
  if (code >= 95) return 8;
  return 2;
}


// ---- ciel anime : degrades, astres, nuages, pluie (firmware, lignes 14923-15024) ----
struct SkyKey { float h; uint8_t t[3], b[3]; };
static const SkyKey SKYK[9] = {{0.0f, {5, 8, 30}, {14, 20, 50}}, {4.5f, {5, 8, 30}, {14, 20, 50}}, {6.5f, {40, 50, 110}, {230, 120, 70}}, {9.0f, {50, 120, 220}, {140, 200, 250}},
  {14.0f, {30, 110, 230}, {120, 190, 245}}, {18.0f, {30, 110, 230}, {120, 190, 245}}, {19.8f, {50, 40, 110}, {240, 110, 60}}, {21.5f, {5, 8, 30}, {14, 20, 50}}, {24.0f, {5, 8, 30}, {14, 20, 50}}};
static inline uint8_t skyMix(uint8_t a, uint8_t b, float t) { return (uint8_t)(a + (b - a) * t); }
static void skyCols(float h, uint8_t *top, uint8_t *bot)
{
  for (int i = 0; i < 8; i++) if (h >= SKYK[i].h && h <= SKYK[i + 1].h) {
    const float t = (h - SKYK[i].h) / (SKYK[i + 1].h - SKYK[i].h);
    for (int c = 0; c < 3; c++) { top[c] = skyMix(SKYK[i].t[c], SKYK[i + 1].t[c], t); bot[c] = skyMix(SKYK[i].b[c], SKYK[i + 1].b[c], t); }
    return;
  }
  for (int c = 0; c < 3; c++) { top[c] = SKYK[0].t[c]; bot[c] = SKYK[0].b[c]; }
}
// v318 -- le ciel suit les heures REELLES de lever / coucher du soleil (ephemeride) : le modele du ciel est cale sur lever 6 h / coucher 21 h, l'heure reelle y est ramenee par une
// fonction lineaire par morceaux (jour : [lever, coucher] -> [6, 21] ; nuit : [coucher, lever] -> [21, 30]). Sans position (page Lieu) : modele tel quel.
static float s_skyRise = 6.0f, s_skySet = 21.0f;
static float skyEffHour(float h)
{
  while (h >= 24.0f) h -= 24.0f;
  const float r = s_skyRise, s = s_skySet;
  if (h >= r && h <= s) return 6.0f + (h - r) / (s - r) * 15.0f;
  const float n = (h > s) ? (h - s) : (h + 24.0f - s), nl = r + 24.0f - s;
  float e = 21.0f + n / nl * 9.0f; if (e >= 24.0f) e -= 24.0f;
  return e;
}
static void skyCloud(Adafruit_GFX &g, int x, int y, uint16_t col)
{
  g.fillCircle(x + 4, y + 5, 4, col); g.fillCircle(x + 11, y + 3, 5, col); g.fillCircle(x + 19, y + 5, 4, col); g.fillRect(x + 4, y + 5, 16, 4, col);
}
// cls : classe meteo 0..8 (voir decoWxClass) ; hour : heure locale decimale ; f : numero d'image
static void skyDraw(Adafruit_GFX &g, float hour, int cls, int f)
{
  uint8_t top[3], bot[3]; skyCols(hour, top, bot);
  static const float KG[9] = {0.0f, 0.2f, 0.55f, 0.5f, 0.55f, 0.65f, 0.55f, 0.65f, 0.8f};
  const uint8_t grey[3] = {95, 100, 115};
  for (int c = 0; c < 3; c++) { top[c] = skyMix(top[c], grey[c], KG[cls]); bot[c] = skyMix(bot[c], grey[c], KG[cls] * 0.8f); }
  if (cls == 8) { const uint8_t d1[3] = {10, 10, 24}, d2[3] = {30, 30, 50}; for (int c = 0; c < 3; c++) { top[c] = skyMix(top[c], d1[c], 0.5f); bot[c] = skyMix(bot[c], d2[c], 0.4f); } }
  const bool flash = (cls == 8 && (f % 60) < 3);
  for (int y = 0; y < 32; y++) {
    uint8_t c[3]; for (int k = 0; k < 3; k++) { c[k] = (uint8_t)(skyMix(top[k], bot[k], y / 31.0f) * 0.55f); }
    if (flash) { c[0] = skyMix(c[0], 200, 0.28f); c[1] = skyMix(c[1], 200, 0.28f); c[2] = skyMix(c[2], 255, 0.28f); }
    g.drawFastHLine(0, y, 128, display->color565(c[0], c[1], c[2]));
  }
  const bool night = (hour < 5.5f || hour >= 21.0f);
  if ((cls == 0 || cls == 1) && night) {   // etoiles qui scintillent
    for (int i = 0; i < 16; i++) if (((f / 5) + i * 3) % 7 < 5) g.drawPixel((i * 53 + 7) % 126 + 1, (i * 29 + 3) % 18 + 1, (i & 1) ? display->color565(190, 190, 220) : display->color565(110, 110, 150));
  }
  if (cls == 0 || cls == 1) {   // astre : soleil de 6 h a 21 h, lune la nuit
    if (!night) {
      float u = (hour - 6.0f) / 15.0f; if (u < 0) u = 0; if (u > 1) u = 1;
      const int x = (int)(8 + u * 112), y = (int)(24 - 20 * sinf((float)M_PI * u));
      // v307 : soleil ANIME : halo qui respire, rayons qui tournent lentement et dont la longueur ondule, coeur qui scintille
      const float pu = 0.5f + 0.5f * sinf(f * 0.11f), sh = 0.5f + 0.5f * sinf(f * 0.37f + 1.3f);
      g.fillCircle(x, y, 7 + (int)(pu * 2.0f), display->color565(46, 40, 10)); g.fillCircle(x, y, 5, display->color565(78, 68, 18));
      for (int a = 0; a < 12; a++) { const float an = a * (float)M_PI / 6.0f + f * 0.02f, r0 = 6.0f, r1 = 8.5f + 2.5f * sinf(f * 0.16f + a * 1.7f) + pu; g.drawLine(x + (int)lroundf(r0 * cosf(an)), y + (int)lroundf(r0 * sinf(an)), x + (int)lroundf(r1 * cosf(an)), y + (int)lroundf(r1 * sinf(an)), display->color565(150, 130, 34)); }
      g.fillCircle(x, y, 3, display->color565((uint8_t)(190 + 20 * sh), (uint8_t)(170 + 25 * sh), (uint8_t)(50 + 25 * sh)));
    } else {
      const float u = fmodf(hour + 3.0f, 24.0f) / 8.5f;
      const int x = (int)(10 + u * 108), y = (int)(20 - 14 * sinf((float)M_PI * u));
      g.drawCircle(x, y, 6 + (int)(1.5f * sinf(f * 0.09f) + 1.5f), display->color565(40, 40, 58));   // v307 : halo de lune qui respire
      g.fillCircle(x, y, 4, display->color565(190, 186, 155)); g.fillCircle(x + 2, y - 2, 3, display->color565((uint8_t)(top[0] * 0.55f * 0.55f), (uint8_t)(top[1] * 0.55f * 0.55f), (uint8_t)(top[2] * 0.55f * 0.55f)));
    }
  }
  if (cls == 1 || (cls >= 2 && cls != 3)) {   // nuages qui derivent
    const bool dark = (cls == 4 || cls == 5 || cls == 7 || cls == 8); const int n = (cls == 1) ? 2 : 5;
    const float k = (night ? 0.75f : 0.75f) * (dark ? 0.6f : 1.0f);
    const uint16_t col = night ? display->color565((uint8_t)(60 * k), (uint8_t)(65 * k), (uint8_t)(85 * k)) : display->color565((uint8_t)(170 * k), (uint8_t)(175 * k), (uint8_t)(190 * k));
    for (int i = 0; i < n; i++) { const int x = (int)fmodf(i * 61 + f * (0.35f + 0.1f * i), 190.0f) - 30, y = 1 + (i * 7) % 9; skyCloud(g, x, y, col); }
  }
  if (cls == 3) {   // brume : bandes qui ondulent
    for (int i = 0; i < 6; i++) {
      const int off = (int)(8 * sinf(f * 0.05f + i)), y = 3 + i * 5; uint8_t c[3]; for (int k = 0; k < 3; k++) c[k] = (uint8_t)(skyMix(top[k], bot[k], y / 31.0f) * 0.55f);
      const uint16_t bc = display->color565(skyMix(c[0], 190, 0.35f), skyMix(c[1], 190, 0.35f), skyMix(c[2], 200, 0.35f));
      for (int x = 0; x < 128; x++) if (((x + off + 400) % 40) < 28) g.drawPixel(x, y, bc);
    }
  }
  if (cls == 4 || cls == 5 || cls == 7 || cls == 8) {   // pluie / bruine / averses / orage
    const int n = (cls == 4) ? 14 : (cls == 5) ? 28 : (cls == 7) ? 22 : 34, sp = (cls == 4) ? 1 : (cls == 7) ? 2 : 3;
    const uint16_t dc = display->color565(110, 150, 255);
    for (int i = 0; i < n; i++) { const int x = (i * 37 + (f * sp) / 3) % 128, y = (f * sp + i * 11) % 44 - 8; if (cls == 4) g.drawPixel(x, y, dc); else g.drawLine(x, y, x - 1, y + 3, dc); }
  }
  if (cls == 6) {   // neige
    for (int i = 0; i < 40; i++) g.drawPixel((int)fmodf(i * 31 + 4 * sinf(f * 0.08f + i) + 128.0f, 128.0f), (f / 2 + i * 7) % 40 - 6, display->color565(235, 235, 245));
  }
  if (cls == 8 && flash) { const uint16_t y = 0xFFF0; g.drawLine(60, 0, 55, 9, y); g.drawLine(55, 9, 61, 9, y); g.drawLine(61, 9, 54, 20, y); }
}
static void skyText(Adafruit_GFX &g, int x, int y, int size, uint16_t col, const char *t)   // texte avec ombre portee noire
{
  g.setTextWrap(false); g.setTextSize(size);
  g.setTextColor(0); g.setCursor(x + 1, y + 1); g.print(t);
  g.setTextColor(col); g.setCursor(x, y); g.print(t);
}
// v306 -- texte dans une FENETRE [x0, x1[ avec decalage off (px) : seuls les caracteres entierement dans la fenetre sont dessines (defilement ping-pong d'un nom trop long)
static void skyTextWin(Adafruit_GFX &g, int x0, int x1, int off, int y, uint16_t col, const char *t)
{
  g.setTextWrap(false); g.setTextSize(1);
  for (int i = 0; t[i]; i++) { const int px = x0 - off + i * 6; if (px < x0 || px + 5 > x1) continue; char b[2] = {t[i], 0}; g.setTextColor(0); g.setCursor(px + 1, y + 1); g.print(b); g.setTextColor(col); g.setCursor(px, y); g.print(b); }
}
// Boucle d'animation : fg(canvas, image, ctx) dessine l'avant-plan par-dessus le ciel. Retour : 1 termine, 0 interrompu, -1 pas assez de tas (l'appelant se rabat sur l'affichage fixe).
// v310 -- le ciel est dessine par BANDES (1, 2 ou 4 selon le tas disponible) : en lecture de playlist le tas est bien plus bas qu'en apercu (GIF ouvert), et le canevas
// de 8 Ko d'avant echouait -> retour a l'ancien affichage sans ciel (constate par l'utilisateur). Une bande de 2 Ko suffit. L'avant-plan est redessine une fois par bande :
// tout etat qui avance (particules...) ne doit evoluer que pendant la bande 0 (g_skyPass == 0).

// ---- avant-plans : meteo, date, compte a rebours, ephemeride (firmware, lignes 15083-15252) ----
static int decoSkyClass() { return (s_wx.valid && (millis() - s_wx.fetchedMs) <= 3UL * 3600UL * 1000UL) ? decoWxClass(s_wx.code) : 0; }

// ---- widget METEO sur le ciel ----
static const char *decoWxLabel(int cls)
{
  static const char *cFr[] = {"CLAIR", "ECLAIRCIES", "COUVERT", "BRUME", "BRUINE", "PLUIE", "NEIGE", "AVERSES", "ORAGE"};
  static const char *cEn[] = {"CLEAR", "PARTLY", "CLOUDY", "FOG", "DRIZZLE", "RAIN", "SNOW", "SHOWERS", "STORM"};
  static const char *cEs[] = {"DESPEJADO", "NUBES", "CUBIERTO", "NIEBLA", "LLOVIZNA", "LLUVIA", "NIEVE", "CHUBASCOS", "TORMENTA"};
  return (uiLanguage == "en" ? cEn : uiLanguage == "es" ? cEs : cFr)[cls];
}
static void skyWxFg(Adafruit_GFX &g, int f, void *ctx)
{
  (void)ctx;
  const bool ok = s_wx.valid && (millis() - s_wx.fetchedMs) <= 3UL * 3600UL * 1000UL;   // meteo injoignable : on garde le ciel clair (soleil le jour, lune la nuit) sans chiffres
  const int cls = ok ? decoWxClass(s_wx.code) : 0;
  const char *lb = ok ? decoWxLabel(cls) : (uiLanguage == "en" ? "OFFLINE" : uiLanguage == "es" ? "SIN DATOS" : "HORS LIGNE");
  String pl = decoPlace; pl.toUpperCase();
  const int winW = 125 - (int)strlen(lb) * 6 - 3 - 14, plW = (int)pl.length() * 6;   // fenetre du nom : on garde 14 px d'air avant l'etat du ciel
  int off = 0;
  if (plW > winW && winW > 12) { const int span = plW - winW, hold = 14, cyc = 2 * (span * 2 + hold); int t = f % cyc; if (t < hold) off = 0; else if (t < hold + span * 2) off = (t - hold) / 2; else if (t < 2 * hold + span * 2) off = span; else off = span - (t - 2 * hold - span * 2) / 2; if (off < 0) off = 0; }   // ping-pong : pause, aller (1 px / 2 images), pause, retour
  skyTextWin(g, 3, 3 + (plW > winW ? winW : plW), off, 1, display->color565(235, 235, 245), pl.c_str());
  skyText(g, 125 - (int)strlen(lb) * 6, 1, 1, 0xFFFF, lb);
  char tb[8]; if (ok) snprintf(tb, sizeof(tb), "%d", (int)lroundf(s_wx.temp)); else snprintf(tb, sizeof(tb), "--");
  const uint16_t ty = display->color565(255, 240, 120);
  skyText(g, 3, 11, 3, ty, tb);
  const int dx = 3 + (int)strlen(tb) * 18 + 2;
  g.drawCircle(dx + 2, 14, 2, 0); g.drawCircle(dx + 1, 13, 2, ty);
  skyText(g, dx + 6, 11, 2, ty, "C");
  if (!ok) return;
  // mini / maxi avec fleches : fleche bas (cyan) = mini, fleche haut (orange) = maxi
  char mn[6], mx[6]; snprintf(mn, sizeof(mn), "%d", (int)lroundf(s_wx.tmin)); snprintf(mx, sizeof(mx), "%d", (int)lroundf(s_wx.tmax));
  int sz = 2; auto width = [&](int s2) { return 7 + 2 + (int)strlen(mn) * 6 * s2 + 6 + 7 + 2 + (int)strlen(mx) * 6 * s2; };
  if (125 - width(2) < dx + 6 + 12 + 4) sz = 1;
  int x = 125 - width(sz); const int ty0 = (sz == 2) ? 17 : 23, ah = (sz == 2) ? 14 : 7, ay = ty0 + (ah - 6) / 2;
  const uint16_t cc = display->color565(140, 240, 255), co = display->color565(255, 170, 80);
  g.fillTriangle(x + 1, ay + 1, x + 7, ay + 1, x + 4, ay + 7, 0); g.fillTriangle(x, ay, x + 6, ay, x + 3, ay + 6, cc);   // fleche bas + ombre
  skyText(g, x + 9, ty0, sz, cc, mn); x += 9 + (int)strlen(mn) * 6 * sz + 6;
  g.fillTriangle(x + 4, ay + 1, x + 1, ay + 7, x + 7, ay + 7, 0); g.fillTriangle(x + 3, ay, x, ay + 6, x + 6, ay + 6, co);      // fleche haut + ombre
  skyText(g, x + 9, ty0, sz, co, mx);
}

// v306 -- nombre (taille 2, 14 px de haut) ecrase verticalement (echelle sf, ancre en haut) : effet « page de calendrier qui se tourne »
static void skyNumScaled(Adafruit_GFX &g, int x, int y, const char *txt, uint16_t col, float sf)
{
  GFXcanvas1 cv(24, 14); if (!cv.getBuffer()) return;
  cv.fillScreen(0); cv.setTextWrap(false); cv.setTextSize(2); cv.setTextColor(1); cv.setCursor(0, 0); cv.print(txt);
  const uint8_t *b = cv.getBuffer(); const int dh = (int)(14 * sf + 0.5f);
  for (int dy = 0; dy < dh; dy++) { int sy = (int)(dy / sf); if (sy > 13) sy = 13; for (int sx = 0; sx < 24; sx++) if (b[sy * 3 + (sx >> 3)] & (0x80 >> (sx & 7))) g.drawPixel(x + sx, y + dy, col); }
}// ---- widget DATE : carte calendrier blanche sur le ciel ----
struct SkyDateCtx { struct tm t; };
static void skyDateFg(Adafruit_GFX &g, int f, void *ctx)
{
  const struct tm &t = ((SkyDateCtx *)ctx)->t;
  const bool en = (uiLanguage == "en"), es = (uiLanguage == "es");
  static const char *dFr[] = {"DIM", "LUN", "MAR", "MER", "JEU", "VEN", "SAM"}; static const char *dEn[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"}; static const char *dEs[] = {"DOM", "LUN", "MAR", "MIE", "JUE", "VIE", "SAB"};
  static const char *mFr[] = {"JAN", "FEV", "MAR", "AVR", "MAI", "JUN", "JUL", "AOU", "SEP", "OCT", "NOV", "DEC"}; static const char *mEn[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"}; static const char *mEs[] = {"ENE", "FEB", "MAR", "ABR", "MAY", "JUN", "JUL", "AGO", "SEP", "OCT", "NOV", "DIC"};
  const char *wd = (en ? dEn : es ? dEs : dFr)[t.tm_wday % 7], *mo = (en ? mEn : es ? mEs : mFr)[t.tm_mon % 12];
  const char *hol = (!en && !es) ? decoHolidayFr(t) : nullptr;
  uint16_t band = display->color565(210, 35, 35), dayc = 0;
  if (hol) { band = display->color565(0, 150, 60); dayc = band; }
  else if (t.tm_wday == 0) dayc = display->color565(215, 30, 30);   // dimanche : date en rouge
  char dayBuf[4]; snprintf(dayBuf, sizeof(dayBuf), "%d", t.tm_mday);
  g.fillRect(2, 2, 32, 28, display->color565(245, 245, 240)); g.fillRect(2, 2, 32, 9, band);
  g.fillRect(9, 0, 2, 3, display->color565(230, 230, 235)); g.fillRect(26, 0, 2, 3, display->color565(230, 230, 235));
  g.setTextWrap(false); g.setTextSize(1); g.setTextColor(0xFFFF); g.setCursor(2 + (32 - (int)strlen(mo) * 6) / 2, 3); g.print(mo);
  // v306 : les pages se tournent depuis le 1er du mois jusqu'a aujourd'hui (rapide puis de plus en plus lent sur les 4 derniers jours) ; le dimanche des pages tournees est rouge
  const int today = t.tm_mday, slow = today > 4 ? 4 : today, fast = today - slow; int dShow = today; float sf = 1.0f;
  if (f < fast) { dShow = f + 1; sf = 0.55f; }
  else { int ff = f - fast, acc = 0; for (int k = 0; k < slow; k++) { const int dur = 2 + k; if (ff < acc + dur) { dShow = fast + k + 1; const int kk = ff - acc; sf = (kk == 0) ? 0.5f : (kk == 1 ? 0.8f : 1.0f); break; } acc += dur; } }
  if (dShow != today) { const int wdS = ((t.tm_wday - (today - dShow)) % 7 + 7) % 7; dayc = (wdS == 0) ? display->color565(215, 30, 30) : 0; snprintf(dayBuf, sizeof(dayBuf), "%d", dShow); }
  const int dx0 = 2 + (32 - (int)strlen(dayBuf) * 12) / 2;
  if (sf < 1.0f) skyNumScaled(g, dx0, 13, dayBuf, dayc, sf); else { g.setTextSize(2); g.setTextColor(dayc); g.setCursor(dx0, 13); g.print(dayBuf); }
  char wk[8], yr[8]; snprintf(wk, sizeof(wk), "%s%d", en ? "WK" : "S", decoIsoWeek(t)); snprintf(yr, sizeof(yr), "%d", t.tm_year + 1900);   // v306 : semaine et annee dans la colonne de droite (avant : espace vide)
  skyText(g, 42, hol ? 2 : 5, 3, 0xFFFF, wd);
  skyText(g, 125 - (int)strlen(wk) * 6, hol ? 4 : 8, 1, display->color565(230, 230, 240), wk);
  skyText(g, 125 - (int)strlen(yr) * 6, hol ? 13 : 17, 1, display->color565(190, 215, 255), yr);
  if (hol) { char line2[24]; snprintf(line2, sizeof(line2), "%.14s", hol); skyText(g, 42, 24, 1, display->color565(120, 255, 150), line2); }
}
// ---- v303 : COMPTE A REBOURS, EPHEMERIDE et STATISTIQUES sur le ciel ----
// texte 5x7 a l'echelle `size` avec un pas de `adv` pixels (pas serre : « AUJOURD'HUI » tient en 121 px au lieu de 132) et ombre portee
static void skyTextTight(Adafruit_GFX &g, int x, int y, int size, int adv, uint16_t col, const char *t)
{
  g.setTextWrap(false);
  for (int pass = 0; pass < 2; pass++) { int xx = x; for (const char *p = t; *p; p++) { g.drawChar(xx + (pass ? 0 : 1), y + (pass ? 0 : 1), (unsigned char)*p, pass ? col : 0, pass ? col : 0, size); xx += adv; } }
}
static void skyTri(Adafruit_GFX &g, int x, int y, bool up, uint16_t col)   // petit triangle 7 x 6 (haut / bas) avec ombre
{
  if (up) { g.fillTriangle(x + 4, y + 1, x + 1, y + 7, x + 7, y + 7, 0); g.fillTriangle(x + 3, y, x, y + 6, x + 6, y + 6, col); }
  else    { g.fillTriangle(x + 1, y + 1, x + 7, y + 1, x + 4, y + 7, 0); g.fillTriangle(x, y, x + 6, y, x + 3, y + 6, col); }
}
// -- compte a rebours --
struct SkyCdCtx { long days; char name[44]; char ds[8]; };   // v353 : nom jusqu'a 40 car. (defile au-dela de 14)
struct CdPt { float x, y, vx, vy; uint8_t life; uint16_t col; };
static CdPt *s_cdPts = nullptr; static int s_cdNextBurst = 8;   // v339 : alloue au 1er affichage, libere par decoScratchRelease() (0,8 Ko de statique en moins)
static void skyCountFg(Adafruit_GFX &g, int f, void *ctx)
{
  const SkyCdCtx *c = (const SkyCdCtx *)ctx; const bool en = (uiLanguage == "en"), es = (uiLanguage == "es");
  if (!s_cdPts) s_cdPts = (CdPt *)calloc(40, sizeof(CdPt));
  if (!s_cdPts) return;
  if (f == 0 && g_skyPass == 0) { for (int i = 0; i < 40; i++) s_cdPts[i].life = 0; s_cdNextBurst = 8; }
  { const int nl = (int)strlen(c->name), plW = nl * 6, winW = 84;   // v353 : 14 car. visibles ; au-dela le nom defile (ping-pong, comme le lieu de la meteo)
    if (plW <= winW) skyText(g, 3, 1, 1, display->color565(240, 240, 245), c->name);
    else { const int span = plW - winW, hold = 14, cyc = 2 * (span * 2 + hold); int t = f % cyc, off; if (t < hold) off = 0; else if (t < hold + span * 2) off = (t - hold) / 2; else if (t < 2 * hold + span * 2) off = span; else off = span - (t - 2 * hold - span * 2) / 2; if (off < 0) off = 0; skyTextWin(g, 3, 3 + winW, off, 1, display->color565(240, 240, 245), c->name); } }
  skyText(g, 125 - 5 * 6, 1, 1, display->color565(190, 190, 205), c->ds);
  if (c->days <= 1) {
    const char *w = (c->days == 0) ? (en ? "TODAY" : es ? "HOY" : "AUJOURD'HUI") : (en ? "TOMORROW" : es ? "MANANA" : "DEMAIN");
    const int len = (int)strlen(w), adv = (len * 12 > 124) ? 11 : 12, x = (128 - len * adv) / 2;
    skyTextTight(g, x < 0 ? 0 : x, 13, 2, adv, c->days == 0 ? display->color565(0, 255, 120) : 0xFFE0, w);
    if (f >= s_cdNextBurst && g_skyPass == 0) {   // salve de feu d'artifice : 14 particules (bande 0 seulement)
      s_cdNextBurst = f + (int)random(10, 20);
      const float bx = random(14, 114), by = random(4, 22); const uint16_t cc = display->color565((uint8_t)random(120, 256), (uint8_t)random(60, 256), (uint8_t)random(60, 256));
      int placed = 0;
      for (int i = 0; i < 40 && placed < 14; i++) if (!s_cdPts[i].life) { const float an = placed * 6.2832f / 14.0f + random(0, 40) / 100.0f, sp = 0.9f + random(0, 90) / 100.0f; s_cdPts[i] = {bx, by, cosf(an) * sp, sinf(an) * sp * 0.8f, (uint8_t)random(12, 22), cc}; placed++; }
    }
  } else {
    float p = f / 20.0f; if (p > 1.0f) p = 1.0f; const float e = 1.0f - (1.0f - p) * (1.0f - p) * (1.0f - p);   // le nombre « defile » de 0 a sa valeur, freinage progressif
    char nbF[8]; snprintf(nbF, sizeof(nbF), "%ld", c->days);
    const char *lb = en ? "DAYS" : es ? "DIAS" : "JOURS";
    const int wn = (int)strlen(nbF) * 18, wl = (int)strlen(lb) * 12, total = wn + 6 + wl; int x0 = (128 - total) / 2; if (x0 < 0) x0 = 0;
    char nb[8]; snprintf(nb, sizeof(nb), "%ld", (long)lroundf(c->days * e));
    skyText(g, x0 + wn - (int)strlen(nb) * 18, 10, 3, 0xFFE0, nb);
    const float br = 0.5f + 0.5f * sinf(f * 0.15f);   // l'unite respire
    skyText(g, x0 + wn + 6, 16, 2, display->color565((uint8_t)(120 + 135 * br), 255, 255), lb);
    if (p >= 1.0f) for (int k = 0; k < 3; k++) { const int sx = x0 + (int)random(0, wn), sy = 10 + (int)random(0, 22); g.drawPixel(sx, sy, 0xFFFF); if (random(0, 3) == 0) { g.drawPixel(sx + 1, sy, 0xFFFF); g.drawPixel(sx - 1, sy, 0xFFFF); g.drawPixel(sx, sy + 1, 0xFFFF); g.drawPixel(sx, sy - 1, 0xFFFF); } }
  }
  for (int i = 0; i < 40; i++) if (s_cdPts[i].life) {
    CdPt &q = s_cdPts[i]; if (g_skyPass == 0) { q.x += q.vx; q.y += q.vy; q.vy += 0.07f; q.life--; }
    const int px = (int)lroundf(q.x), py = (int)lroundf(q.y);
    if (px >= 0 && px < 128 && py >= 0 && py < 32) g.drawPixel(px, py, q.life < 5 ? display->color565(90, 90, 90) : q.col);
  }
}
// -- ephemeride --
struct SkyEphCtx { bool ok, polarDay, south; struct tm a, c; int dl, illum; double age; char pl[16]; };
static bool decoEphCompute(void *ep)   // void * : prototypes generes avant les struct
{
  SkyEphCtx &e = *(SkyEphCtx *)ep;
  if (decoLat == 0.0f && decoLon == 0.0f) return false;
  time_t now; time(&now); struct tm tl; localtime_r(&now, &tl); if (tl.tm_year < 100) return false;
  time_t rise = 0, set = 0; e.polarDay = false; e.ok = decoSunTimes(decoLat, decoLon, now, rise, set, e.polarDay);
  const double SYN = 29.530588853; e.age = fmod((double)now / 86400.0 + 2440587.5 - 2451550.1, SYN); if (e.age < 0) e.age += SYN;
  e.illum = (int)(((1.0 - cos(2.0 * M_PI * e.age / SYN)) / 2.0) * 100.0 + 0.5); e.south = (decoLat < 0);
  if (e.ok) { localtime_r(&rise, &e.a); localtime_r(&set, &e.c); e.dl = (int)((set - rise) / 60); }
  String pl = decoPlace; if (pl.length() > 15) pl = pl.substring(0, 15); pl.toUpperCase(); strncpy(e.pl, pl.c_str(), sizeof(e.pl) - 1); e.pl[sizeof(e.pl) - 1] = 0;
  return true;
}
static void skyEphFg(Adafruit_GFX &g, int f, void *ctx)
{
  const SkyEphCtx *e = (const SkyEphCtx *)ctx; const bool en = (uiLanguage == "en"), es = (uiLanguage == "es");
  const char *sRise = en ? "SUNRISE" : es ? "SALIDA" : "LEVER", *sSet = en ? "SUNSET" : es ? "OCASO" : "COUCHER", *sMoon = en ? "MOON" : es ? "LUNA" : "LUNE";
  const float pu = 0.5f + 0.5f * sinf(f * 0.1f);   // halo qui respire autour de la lune
  g.drawCircle(15, 16, 15, display->color565((uint8_t)(40 + 50 * pu), (uint8_t)(38 + 45 * pu), (uint8_t)(20 + 25 * pu)));
  const double SYN = 29.530588853, th = 2.0 * M_PI * e->age / SYN; const bool waxing = e->age < SYN / 2.0;
  const uint16_t lit = display->color565(255, 240, 180), dark = display->color565(30, 30, 40);
  for (int dy = -13; dy <= 13; dy++) {
    const int w = (int)(sqrt((double)(13 * 13 - dy * dy)) + 0.5), t = (int)(w * cos(th) + (cos(th) >= 0 ? 0.5 : -0.5));
    for (int dx = -w; dx <= w; dx++) { bool on = waxing ? (dx >= t) : (dx <= -t); if (e->south) on = waxing ? (dx <= -t) : (dx >= t); g.drawPixel(15 + dx, 16 + dy, on ? lit : dark); }
  }
  char b[28];
  skyText(g, 36, 1, 1, display->color565(235, 235, 245), e->pl);
  if (e->ok) {
    skyTri(g, 36, 10, true, 0xFFE0);
    snprintf(b, sizeof(b), "%s %02d:%02d", sRise, e->a.tm_hour, e->a.tm_min); skyText(g, 46, 9, 1, 0xFFE0, b);
    skyTri(g, 36, 18, false, display->color565(255, 150, 40));
    snprintf(b, sizeof(b), "%s %02d:%02d", sSet, e->c.tm_hour, e->c.tm_min); skyText(g, 46, 17, 1, display->color565(255, 150, 40), b);
    snprintf(b, sizeof(b), "%dH%02d  %s %d%%", e->dl / 60, e->dl % 60, sMoon, e->illum); skyText(g, 36, 25, 1, display->color565(140, 240, 255), b);
  } else {
    skyText(g, 36, 12, 1, 0xFFE0, e->polarDay ? (en ? "POLAR DAY" : es ? "DIA POLAR" : "JOUR POLAIRE") : (en ? "POLAR NIGHT" : es ? "NOCHE POLAR" : "NUIT POLAIRE"));
    snprintf(b, sizeof(b), "%s %d%%", sMoon, e->illum); skyText(g, 36, 25, 1, display->color565(140, 240, 255), b);
  }
}

// ---- avant-plans des widgets Recalbox : challenge, recompenses, sante (firmware, lignes 15286-15382) ----
// ---- v315 : widgets RECALBOX (8 challenge, 9 recompenses, 10 sante) sur le ciel ----
// decalage (px) d'un texte trop long dans une fenetre : pause, defilement, pause, retour
static int rbwPing(int f, int over)
{
  if (over <= 0) return 0;
  const int t = (f / 2) % (2 * (over + 10));
  if (t < over + 10) return t < over ? t : over;
  const int t2 = t - (over + 10); return over - (t2 < over ? t2 : over);
}
static void rbwTextFit(Adafruit_GFX &g, int x0, int x1, int f, int y, uint16_t col, const char *t)
{
  const int over = (int)strlen(t) * 6 - (x1 - x0);
  skyTextWin(g, x0, x1, rbwPing(f, over), y, col, t);
}
static void skyRbwChFg(Adafruit_GFX &g, int f, void *ctx)
{
  (void)ctx; const bool en = (uiLanguage == "en"), es = (uiLanguage == "es");
  char b[24];
  if (g_rbw.days >= 0) { snprintf(b, sizeof(b), "%d%s", g_rbw.days, en ? "D" : "J"); skyText(g, 125 - (int)strlen(b) * 6, 1, 1, g_rbw.days <= 2 ? display->color565(255, 120, 80) : display->color565(190, 215, 255), b); }
  rbwTextFit(g, 3, 98, f, 1, display->color565(240, 240, 245), g_rbw.chName);
  const bool pageA = ((f / 60) % 2) == 0 && g_rbw.nTop > 0;   // 3 s le classement, 3 s votre position
  if (pageA) {
    const uint16_t med[3] = { 0xFFE0, display->color565(200, 200, 215), display->color565(205, 127, 50) };
    for (int i = 0; i < g_rbw.nTop; i++) {
      const int y = 9 + i * 8;
      b[0] = (char)('1' + i); b[1] = 0; skyText(g, 3, y, 1, med[i], b);
      skyText(g, 11, y, 1, 0xFFFF, g_rbw.tn[i]);
      snprintf(b, sizeof(b), "%lu", (unsigned long)g_rbw.ts[i]); skyText(g, 114 - (int)strlen(b) * 6, y, 1, display->color565(140, 240, 255), b);
      if (g_rbw.tm[i] > 0) skyTri(g, 117, y - 1, true, display->color565(0, 230, 100));
      else if (g_rbw.tm[i] < 0) skyTri(g, 117, y - 1, false, display->color565(255, 70, 70));
      else g.drawFastHLine(118, y + 3, 5, display->color565(170, 170, 170));
    }
  } else if (g_rbw.rank > 0) {
    snprintf(b, sizeof(b), "#%d", g_rbw.rank); skyText(g, 4, 11, 3, 0xFFE0, b);
    skyText(g, 74, 11, 1, display->color565(190, 215, 255), "GG");
    snprintf(b, sizeof(b), "%d", g_rbw.ggs); skyText(g, 74, 20, 2, display->color565(0, 230, 100), b);
  } else {
    skyText(g, 4, 15, 2, display->color565(190, 215, 255), en ? "UNRANKED" : es ? "SIN PUESTO" : "NON CLASSE");
  }
}
// v324 -- recompenses : chaque page dure au moins 8 s (2 s de pause, defilement du texte trop long a 25 px/s, 3 s de pause finale) au lieu de 3 s ; la duree du widget couvre toutes les pages
static int rbwRwOver(int i) { const int a = (int)strlen(g_rbw.rn[i]) * 6 - 93, c = (int)strlen(g_rbw.rc[i]) * 6 - 93; const int o = a > c ? a : c; return o > 0 ? o : 0; }
static int rbwRwPageFrames(int i) { const int pf = 40 + rbwRwOver(i) + 60; return pf < 160 ? 160 : pf; }   // v329 : defilement 1 px / image (50 px/s) au lieu de 25 px/s
static int rbwRwTotalFrames() { int t = 0; for (int i = 0; i < g_rbw.nRw; i++) t += rbwRwPageFrames(i); return t < 1 ? 1 : t; }
static void rbwFitLocal(Adafruit_GFX &g, int x0, int x1, int fl, int y, uint16_t col, const char *t)
{
  const int over = (int)strlen(t) * 6 - (x1 - x0); int off = 0;
  if (over > 0) { off = fl - 40; if (off < 0) off = 0; if (off > over) off = over; }
  skyTextWin(g, x0, x1, off, y, col, t);
}
static void skyRbwRwFg(Adafruit_GFX &g, int f, void *ctx)
{
  (void)ctx; const bool en = (uiLanguage == "en"), es = (uiLanguage == "es");
  int ff = f % rbwRwTotalFrames(), i = 0;
  while (i < g_rbw.nRw - 1 && ff >= rbwRwPageFrames(i)) { ff -= rbwRwPageFrames(i); i++; }
  const int fl = ff;   // image locale a la page
  char b[16];
  skyText(g, 3, 1, 1, 0xFFE0, en ? "TO WIN" : es ? "A GANAR" : "A GAGNER");
  snprintf(b, sizeof(b), "%d/%d", i + 1, (int)g_rbw.nRw); skyText(g, 125 - (int)strlen(b) * 6, 1, 1, display->color565(190, 215, 255), b);
  const int bob = (int)lroundf(sinf(f * 0.2f) * 1.0f);
  const char *k = g_rbw.rk[i];
  if (!strcmp(k, "BADGE")) {   // medaille et rubans
    g.fillTriangle(8, 20 + bob, 15, 20 + bob, 9, 31, display->color565(200, 40, 40)); g.fillTriangle(13, 20 + bob, 20, 20 + bob, 19, 31, display->color565(200, 40, 40));
    g.fillCircle(14, 17 + bob, 8, 0); g.fillCircle(14, 16 + bob, 8, 0xFFE0); g.fillCircle(14, 16 + bob, 5, display->color565(255, 150, 0));
    g.drawPixel(14, 16 + bob, 0xFFFF); g.drawPixel(13, 16 + bob, 0xFFFF); g.drawPixel(15, 16 + bob, 0xFFFF); g.drawPixel(14, 15 + bob, 0xFFFF); g.drawPixel(14, 17 + bob, 0xFFFF);
  } else if (!strcmp(k, "AVATAR")) {   // tete et epaules
    g.fillCircle(15, 38 + bob, 12, 0); g.fillCircle(14, 37 + bob, 12, display->color565(60, 120, 255));
    g.fillCircle(15, 17 + bob, 6, 0); g.fillCircle(14, 16 + bob, 6, display->color565(255, 210, 160));
  } else {   // titre : banniere etoilee
    g.fillRect(6, 11 + bob, 18, 12, 0); g.fillRect(5, 10 + bob, 18, 12, display->color565(150, 70, 220));
    g.fillTriangle(6, 22 + bob, 24, 22 + bob, 15, 30, 0); g.fillTriangle(5, 21 + bob, 23, 21 + bob, 14, 29, display->color565(150, 70, 220));
    g.drawPixel(14, 14 + bob, 0xFFE0); g.drawPixel(13, 15 + bob, 0xFFE0); g.drawPixel(14, 15 + bob, 0xFFFF); g.drawPixel(15, 15 + bob, 0xFFE0); g.drawPixel(14, 16 + bob, 0xFFE0); g.drawPixel(12, 17 + bob, 0xFFE0); g.drawPixel(16, 17 + bob, 0xFFE0);
  }
  rbwFitLocal(g, 32, 125, fl, 11, 0xFFE0, g_rbw.rn[i]);
  rbwFitLocal(g, 32, 125, fl, 21, display->color565(140, 240, 255), g_rbw.rc[i]);
}
static void skyRbwHlFg(Adafruit_GFX &g, int f, void *ctx)
{
  (void)ctx; const bool en = (uiLanguage == "en"), es = (uiLanguage == "es");
  const char *lab[6] = { "TEMP", en ? "PWR" : "ALIM", en ? "CLK" : es ? "RELO" : "HORL", en ? "NET" : es ? "RED" : "RES", es ? "DISC" : "DISK", "MEM" };
  for (int i = 0; i < 6; i++) {
    const int cx = 3 + (i % 3) * 42, cy = 2 + (i / 3) * 10, st = g_rbw.hs[i] - '0';
    uint16_t c = display->color565(0, 220, 80);
    if (st == 1) c = ((f / 6) % 2) ? display->color565(255, 170, 0) : display->color565(120, 80, 0);
    else if (st >= 2) c = ((f / 4) % 2) ? display->color565(255, 40, 40) : display->color565(110, 10, 10);
    g.fillCircle(cx + 4, cy + 4, 4, 0); g.fillCircle(cx + 3, cy + 3, 3, c);
    skyText(g, cx + 10, cy, 1, 0xFFFF, lab[i]);
  }
  char b[28];
  const uint16_t ct = display->color565(140, 240, 255);
  if (g_rbw.temp >= 0) snprintf(b, sizeof(b), "%dC", g_rbw.temp); else snprintf(b, sizeof(b), "--");
  skyText(g, 3, 24, 1, g_rbw.hs[0] > '0' ? display->color565(255, 150, 60) : ct, b);
  if (g_rbw.mem >= 0) snprintf(b, sizeof(b), "RAM %d%%", g_rbw.mem); else snprintf(b, sizeof(b), "RAM --");
  skyText(g, 38, 24, 1, g_rbw.hs[5] > '0' ? display->color565(255, 150, 60) : ct, b);
  if (g_rbw.disk >= 0) snprintf(b, sizeof(b), "SD %d%%", g_rbw.disk); else snprintf(b, sizeof(b), "SD --");
  skyText(g, 86, 24, 1, g_rbw.hs[4] > '0' ? display->color565(255, 150, 60) : ct, b);
}

// ---- citations : themes d'origine embarques (firmware, lignes 14448-14511) ----
// Themes d'origine embarques (copies sur la SD par decoQuotesRestore()). Contenu d'ESSAI : le jeu complet suit apres relecture.
static const char QT_FR_BISCUIT[] PROGMEM =
  "Un petit pas aujourd'hui vaut mieux qu'un grand projet demain.\n"
  "La patience est la cle qui ouvre toutes les portes.\n"
  "Celui qui sourit a la difficulte l'a deja a moitie vaincue.\n"
  "Le meilleur moment pour commencer, c'est maintenant.\n"
  "Une bonne nouvelle viendra de l'endroit ou vous ne l'attendiez pas.\n"
  "Ne comptez pas les jours : faites que les jours comptent.\n"
  "Votre prochaine partie sera la meilleure.\n"
  "Le bonheur se cache dans les petites choses du quotidien.\n"
  "Qui sait ecouter entend bien plus que celui qui parle.\n"
  "Une porte se ferme, un niveau secret s'ouvre.\n";
static const char QT_FR_BLAGUES[] PROGMEM =
  "Pourquoi les plongeurs plongent-ils toujours en arriere ? | Parce que sinon ils tomberaient dans le bateau.\n"
  "Que dit un escargot quand il croise une limace ? | Regarde le nudiste !\n"
  "Quel est le comble pour un joueur de jeux video ? | De perdre sa vie sur un coup de tete.\n"
  "Pourquoi les fantomes ne mentent-ils jamais ? | Parce qu'on voit a travers eux.\n"
  "Quel est le fruit prefere des robots ? | La pomme... d'Adam.\n";
static const char QT_EN_FORTUNE[] PROGMEM =
  "A small step today beats a big plan tomorrow.\n"
  "Patience is the key that opens every door.\n"
  "The best time to start is right now. | Proverb\n";
static const char QT_ES_FORTUNE[] PROGMEM =
  "Un pequeno paso hoy vale mas que un gran plan manana.\n"
  "La paciencia es la llave que abre todas las puertas.\n"
  "El mejor momento para empezar es ahora. | Refran\n";

static const char QT_FR_QUIZ[] PROGMEM =
  "Quel herisson bleu est la mascotte de Sega ? | SONIC / TAILS / KNUCKLES | 1\n"
  "Quel jeu d'arcade de 1980 met en scene un gobeur de pastilles jaune ? | GALAXIAN / PAC-MAN / DIG DUG | 2\n"
  "En quelle annee est sorti Space Invaders (Taito) ? | 1976 / 1977 / 1978 | 3\n"
  "Quel jeu de tennis d'Atari (1972) a popularise le jeu video ? | BREAKOUT / PONG / ASTEROIDS | 2\n"
  "Comment s'appellent les pieces du jeu Tetris ? | DOMINOS / TETROMINOS / PENTOMINOS | 2\n"
  "Quel editeur japonais a cree Street Fighter II ? | CAPCOM / SNK / KONAMI | 1\n"
  "Quelle console portable de Nintendo est sortie en 1989 ? | GAME BOY / GAME GEAR / LYNX | 1\n"
  "Combien de bits possede la Super Nintendo ? | 8 BITS / 16 BITS / 32 BITS | 2\n"
  "Quel personnage porte l'epee dans The Legend of Zelda ? | ZELDA / LINK / GANON | 2\n"
  "Quel studio a cree le jeu Donkey Kong en 1981 ? | NINTENDO / SEGA / ATARI | 1\n";
static const char QT_EN_QUIZ[] PROGMEM =
  "Which blue hedgehog is the mascot of Sega? | SONIC / TAILS / MARIO | 1\n"
  "Which 1972 Atari game made video games popular? | BREAKOUT / PONG / ASTEROIDS | 2\n"
  "How many bits does the Super Nintendo have? | 8 BITS / 16 BITS / 32 BITS | 2\n";
static const char QT_ES_QUIZ[] PROGMEM =
  "Que erizo azul es la mascota de Sega? | SONIC / TAILS / MARIO | 1\n"
  "Cuantos bits tiene la Super Nintendo? | 8 BITS / 16 BITS / 32 BITS | 2\n"
  "En que ano salio Space Invaders? | 1976 / 1977 / 1978 | 3\n";static const char QT_FR_CELEBRES[] PROGMEM =
  "Il y a un spectacle plus grand que la mer, c'est le ciel ; il y a un spectacle plus grand que le ciel, c'est l'interieur de l'ame. | Victor Hugo, Les Miserables\n"
  "Le bon sens est la chose du monde la mieux partagee : car chacun pense en etre si bien pourvu, que ceux meme qui sont les plus difficiles a contenter en toute autre chose n'ont point coutume d'en desirer plus qu'ils n'en ont. | Rene Descartes, Discours de la methode\n"
  "L'homme n'est qu'un roseau, le plus faible de la nature, mais c'est un roseau pensant. Il ne faut pas que l'univers entier s'arme pour l'ecraser. | Blaise Pascal, Pensees\n"
  "L'homme est ne libre, et partout il est dans les fers. Tel se croit le maitre des autres, qui ne laisse pas d'etre plus esclave qu'eux. | Jean-Jacques Rousseau, Du contrat social\n"
  "Patience et longueur de temps font plus que force ni que rage. | Jean de La Fontaine, Le Lion et le Rat\n"
  "La plus grande chose du monde, c'est de savoir etre a soi. Il faut se reserver une arriere-boutique toute notre, toute franche, en laquelle nous establissons notre vraye liberte. | Michel de Montaigne, Essais\n"
  "Il faut manger pour vivre, et non pas vivre pour manger. | Moliere, L'Avare\n"
  "Science sans conscience n'est que ruine de l'ame. | Francois Rabelais, Pantagruel\n"
  "Je me presse de rire de tout, de peur d'etre oblige d'en pleurer. | Beaumarchais, Le Barbier de Seville\n"
  "La raison du plus fort est toujours la meilleure : nous l'allons montrer tout a l'heure. | Jean de La Fontaine, Le Loup et l'Agneau\n";
static const char QT_EN_FAMOUS[] PROGMEM =
  "To be, or not to be, that is the question: whether 'tis nobler in the mind to suffer the slings and arrows of outrageous fortune, or to take arms against a sea of troubles and by opposing end them. | William Shakespeare, Hamlet\n"
  "All the world's a stage, and all the men and women merely players; they have their exits and their entrances, and one man in his time plays many parts. | William Shakespeare, As You Like It\n"
  "Four score and seven years ago our fathers brought forth on this continent, a new nation, conceived in Liberty, and dedicated to the proposition that all men are created equal. | Abraham Lincoln, Gettysburg Address\n";
static const char QT_ES_CELEBRES[] PROGMEM =
  "En un lugar de la Mancha, de cuyo nombre no quiero acordarme, no ha mucho tiempo que vivia un hidalgo de los de lanza en astillero, adarga antigua, rocin flaco y galgo corredor. | Miguel de Cervantes, Don Quijote\n"
  "La libertad, Sancho, es uno de los mas preciosos dones que a los hombres dieron los cielos; con ella no pueden igualarse los tesoros que encierra la tierra ni el mar encubre. | Miguel de Cervantes, Don Quijote\n"
  "Que toda la vida es sueno, y los suenos, suenos son. | Calderon de la Barca, La vida es sueno\n";

// ---- citations : etat, table des themes (firmware, lignes 14512-14525) ----
char g_quoteThemeName[26] = "";   // v304 -- theme de citations tire (choix du fond d'image)
const uint16_t *g_qBg = nullptr;   // fond de la citation en cours (nullptr = noir)
uint16_t g_qCq = 0, g_qCa = 0xFFFF, g_qShCol = 0; bool g_qShadow = true, g_qOutline = false; uint8_t g_qDim = 55; int g_qX0 = 0, g_qX1 = 128, g_qY0 = 0, g_qY1 = 32;   // v312 : + fenetre verticale (evite les bordures claires du fond)   // v308 : fenetre de texte propre au fond (ex. devinettes : seulement la partie jaune)   // v306d : ombre (couleur) et luminosite du fond (% ; 55 par defaut : les fonds clairs nuisaient a la lisibilite)   // v305 -- couleurs du texte (question / reponse) et ombre portee, propres au fond (fichier <fond>.col)
static bool g_quoteQuiz = false;   // v285 : theme « quiz » en cours (pause de reflexion plus longue avant la reponse)

struct DecoQuoteSet { const char *lang; const char *name; const char *data; };
static const DecoQuoteSet DECO_QUOTE_SETS[] = {
  {"fr", "biscuit-chinois", QT_FR_BISCUIT}, {"fr", "blagues", QT_FR_BLAGUES},
  {"fr", "quiz-retrogaming", QT_FR_QUIZ},   {"en", "fortune-cookie", QT_EN_FORTUNE},  {"es", "galleta-fortuna", QT_ES_FORTUNE},
  {"en", "retro-quiz", QT_EN_QUIZ},         {"es", "quiz-retrogaming", QT_ES_QUIZ},
  {"fr", "citations-celebres", QT_FR_CELEBRES}, {"en", "famous-quotes", QT_EN_FAMOUS}, {"es", "citas-celebres", QT_ES_CELEBRES}};

static bool decoQuoteLangOk(const String &l) { return l == "fr" || l == "en" || l == "es"; }


// ---- citations : UTF-8 -> ASCII (firmware, lignes 14575-14593) ----
// UTF-8 -> ASCII (la police n'a pas d'accents) : sur place.
static void decoAscii(char *s)
{
  char *w = s;
  for (const unsigned char *r = (const unsigned char *)s; *r; ) {
    const unsigned char c = *r;
    if (c < 0x80) { *w++ = (char)c; r++; }
    else if (c == 0xC3 && r[1]) {   // lettres latines accentuees (U+00C0..U+00FF)
      static const char MAP[] = "AAAAAAACEEEEIIIIDNOOOOOxOUUUUYTsaaaaaaaceeeeiiiidnooooo/ouuuuyty";
      const unsigned char k = r[1];
      *w++ = (k >= 0x80 && k <= 0xBF) ? MAP[k - 0x80] : '?'; r += 2;
    }
    else if (c == 0xC5 && r[1] == 0x93) { *w++ = 'o'; *w++ = 'e'; r += 2; }   // oe
    else if (c == 0xE2 && r[1] == 0x80 && r[2]) { *w++ = (r[2] == 0x99 || r[2] == 0x98) ? '\'' : (r[2] == 0x93 || r[2] == 0x94) ? '-' : '"'; r += 3; }   // apostrophes, tirets, guillemets
    else { *w++ = '?'; r++; while ((*r & 0xC0) == 0x80) r++; }
  }
  *w = 0;
}


// ---- citations : vitesse de defilement (firmware, lignes 14635-14639) ----
// Affichage : defilement vertical d'un texte (+ suite). POLICE ET STYLE DE L'ECRAN DESCRIPTION DES JEUX (drawScoreScreen) :
// TomThumb 3x5, tout en MAJUSCULES, or (255,200,0), lignes CENTREES (avances reelles tomThumbCharAdvance()), pas de ligne de 8 px
// (4 lignes dans 32 px). Le texte est dessine UNE fois dans un GFXcanvas1 (1 bit) puis une fenetre de 32 lignes est recopiee point a
// point a chaque image (voir v282). La suite (auteur / reponse) est en blanc.
#define QUOTE_STEP_MS 100   // 1 pixel toutes les 100 ms (~10 px/s, ~1 s par ligne) : a ajuster apres essai sur la dalle

// ---- QCM : decoupe de texte TomThumb (firmware, lignes 14640-14668) ----
// ============ v287 -- QCM (quiz a choix multiples) ============
// Decoupe un texte en <= maxLines lignes de <= maxPx pixels (avances TomThumb), retourne le nombre de lignes, ou -1 si ca ne tient pas.
static int decoWrapTT(const char *t, char (*lines)[40], uint8_t *widths, int maxLines, int maxPx)
{
  const int len = (int)strlen(t); int i = 0, n = 0;
  while (i < len) {
    while (i < len && t[i] == ' ') i++;
    if (i >= len) break;
    if (n >= maxLines) return -1;
    int j = i, last = -1, w = 0;
    for (; j < len && j - i < 38; j++) { const int a = tomThumbCharAdvance(t[j]); if (w + a > maxPx) break; w += a; if (t[j] == ' ') last = j; }
    int cut = (j >= len) ? len : ((last > i) ? last : j);
    int l = cut - i; memcpy(lines[n], t + i, l); lines[n][l] = 0;
    while (l > 0 && lines[n][l - 1] == ' ') lines[n][--l] = 0;
    int lw = 0; for (int k = 0; k < l; k++) lw += tomThumbCharAdvance(lines[n][k]);
    widths[n] = (uint8_t)(lw > 255 ? 255 : lw); n++;
    i = cut;
  }
  return n;
}

static int decoTTWidth(const char *s) { int w = 0; for (; *s; s++) w += tomThumbCharAdvance(*s); return w; }

// ==================================================================
// v304 -- FONDS D'IMAGE DES CITATIONS : /widgets/quotes/bg/<fond>.raw565 (128 x 32, 8 192 octets, uint16 RGB565 natifs, meme format que les images du DMD).
// Choix du fichier : <theme>.raw565 si present ; sinon fond generique selon le nom du theme (« quiz » -> quiz.raw565, avec « retro » : quiz-retrogaming.raw565 d'abord ;
// biscuit / fortune / galleta -> biscuit-chinois ; celebre / famous / citas -> citations-celebres ; blague / joke / chiste -> blagues ; devinette / riddle / adivin -> devinettes).
// Aucun fichier = fond noir comme avant. Le fond est lu UNE fois par citation dans le tampon raw565FullBuf (8 Ko, deja alloue par drawRaw565) ; texte avec ombre portee.
// ==================================================================

// ---- citations : fond, texte, QCM, defilement (firmware, lignes 14710-14905) ----
static void decoQuoteBgLoadImpl();
static void decoQuoteBgLoad() { decoQuoteBgLoadImpl(); Serial.println(String("[DECO] fond citation : theme=") + g_quoteThemeName + (g_qBg ? " -> fond charge" : " -> aucun fond (noir)") + ", heap=" + String(ESP.getMaxAllocHeap())); }
static void decoQuoteBgLoadImpl()
{
  g_qBg = nullptr; g_qCq = display->color565(255, 200, 0); g_qCa = 0xFFFF; g_qShadow = true; g_qShCol = 0; g_qDim = 80; g_qX0 = 0; g_qX1 = 128; g_qY0 = 0; g_qY1 = 32; g_qOutline = true;   // v309 : CONTOUR noir complet des lettres par defaut, fond plus lumineux (80 %)
  // v314 : en lecture de playlist le tas est bas, mais le tampon raw565FullBuf (8 Ko) EXISTE deja (image brute des GIF raw565pack) : on le reutilise, la garde ne porte que sur l'ouverture du fichier ; sinon il faut pouvoir l'allouer
  if (ESP.getMaxAllocHeap() < (raw565FullBuf ? 4500 : 8192 + 4500)) { Serial.println("[DECO] fond citation : tas insuffisant (maxalloc=" + String(ESP.getMaxAllocHeap()) + ")"); return; }
  String n = g_quoteThemeName; n.toLowerCase();
  if (decoQuoteBgTry(g_quoteThemeName)) return;
  if (n.indexOf("quiz") >= 0) { if (n.indexOf("retro") >= 0 && decoQuoteBgTry("quiz-retrogaming")) return; decoQuoteBgTry("quiz"); return; }
  if (n.indexOf("biscuit") >= 0 || n.indexOf("fortune") >= 0 || n.indexOf("galleta") >= 0) { decoQuoteBgTry("biscuit-chinois"); return; }
  if (n.indexOf("celebre") >= 0 || n.indexOf("famous") >= 0 || n.indexOf("citas") >= 0) { decoQuoteBgTry("citations-celebres"); return; }
  if (n.indexOf("blague") >= 0 || n.indexOf("joke") >= 0 || n.indexOf("chiste") >= 0) { decoQuoteBgTry("blagues"); return; }
  if (n.indexOf("devinette") >= 0 || n.indexOf("riddle") >= 0 || n.indexOf("adivin") >= 0) { decoQuoteBgTry("devinettes"); return; }
}
static inline uint16_t decoQBgPx(int x, int y)   // pixel du fond, assombri (g_qDim %)
{
  if (!g_qBg) return 0;
  const uint16_t c = g_qBg[y * 128 + x]; if (g_qDim >= 100) return c;
  const uint32_t r = ((c >> 11) & 31) * g_qDim / 100, gg = ((c >> 5) & 63) * g_qDim / 100, b = (c & 31) * g_qDim / 100;
  return (uint16_t)((r << 11) | (gg << 5) | b);
}
static void decoQBgRect(int x, int y, int w, int h)   // repeint une zone avec le fond (ou en noir)
{
  for (int yy = y; yy < y + h; yy++) { if (yy < 0 || yy >= 32) continue; for (int xx = x; xx < x + w; xx++) if (xx >= 0 && xx < 128) display->drawPixel(xx, yy, decoQBgPx(xx, yy)); }
}
static void decoQText(int x, int y, uint16_t col, const char *s)   // texte TomThumb avec ombre portee noire quand il y a un fond
{
  if (g_qBg && g_qShadow) { display->setTextColor(g_qShCol); if (g_qOutline) { for (int oy = -1; oy <= 1; oy++) for (int ox = -1; ox <= 1; ox++) if (ox || oy) { display->setCursor(x + ox, y + oy); display->print(s); } } else { display->setCursor(x + 1, y + 1); display->print(s); } }
  display->setTextColor(col); display->setCursor(x, y); display->print(s);
}
// Retourne 1 = termine, 0 = interrompu, -1 = pas un QCM valide (l'appelant retombe sur l'affichage « question | reponse »).
static int decoDrawQcm(const char *qIn, const char *sIn, bool preview)
{
  char S[80], Q[240], ql[3][40]; uint8_t qw[3];   // v340 : plus statiques
  char al[3][2][40]; uint8_t aw[3][2]; int an[3], bx[3], bw[3], bh[3];   // v340 : lignes de chaque reponse (1 ou 2)
  strncpy(S, sIn, sizeof(S) - 1); S[sizeof(S) - 1] = 0;
  char *bar = strstr(S, " | "); if (!bar) return -1;
  *bar = 0; const int correct = atoi(bar + 3);
  char *ch[3]; int nc = 0; char *p = S;
  while (nc < 3) { ch[nc++] = p; char *sep = strstr(p, " / "); if (!sep) break; *sep = 0; p = sep + 3; }
  if (nc < 2 || correct < 1 || correct > nc) return -1;
  for (char *c = S; c < S + sizeof(S); c++) *c = (char)toupper((unsigned char)*c);
  strncpy(Q, qIn, sizeof(Q) - 1); Q[sizeof(Q) - 1] = 0; for (char *c = Q; *c; c++) *c = (char)toupper((unsigned char)*c);
  const int nql = decoWrapTT(Q, ql, qw, 3, 124);
  if (nql < 1) return -1;

  const uint16_t white = 0xFFFF, red = display->color565(255, 50, 50), green = display->color565(0, 230, 70), grey = display->color565(55, 55, 55), blueBar = display->color565(0, 110, 255), gold = display->color565(255, 200, 0);
  const uint16_t cc[3] = {red, white, green}, c2[2] = {red, green};
  const int zw = 128 / nc, cy = nql * 6 + 3;   // zone de chaque choix ; ordonnee du haut de la ligne des choix
  decoQuoteBgLoad();   // v304 : fond d'image du theme (ou noir)
  if (g_qBg) decoQBgRect(0, 0, 128, 32); else display->clearScreen();
  display->setTextWrap(false); display->setFont(&TomThumb);
  for (int i = 0; i < nql; i++) decoQText((128 - qw[i]) / 2, i * 6 + 5, white, ql[i]);
  // v340 -- une reponse trop longue passe sur 2 LIGNES (si la question tient sur 2 lignes au plus : sinon pas la place) au lieu d'etre tronquee ; plus de place nulle part = tronquee comme avant
  auto drawCh = [&](int k, uint16_t col) { for (int l = 0; l < an[k]; l++) decoQText(k * zw + (zw - aw[k][l]) / 2, cy + 5 + l * 6, col, al[k][l]); };
  for (int k = 0; k < nc; k++) {
    an[k] = 0;
    if (nql <= 2 && decoTTWidth(ch[k]) > zw - 2) {
      char tmp[80]; strncpy(tmp, ch[k], sizeof(tmp) - 1); tmp[sizeof(tmp) - 1] = 0;
      // v350 : la DERNIERE reponse sur 2 lignes est plus etroite de 8 px (le chiffre du decompte occupe x 119-127, y 25-31)
      int r = decoWrapTT(tmp, al[k], aw[k], 2, (k == nc - 1) ? zw - 10 : zw - 2);
      if (r < 0 && k == nc - 1) r = decoWrapTT(tmp, al[k], aw[k], 2, zw - 2);
      if (r >= 1 && r <= 2) an[k] = r;
    }
    if (an[k] == 0) {   // 1 ligne, tronquee a la largeur de sa zone (comportement d'avant)
      while (strlen(ch[k]) > 1 && decoTTWidth(ch[k]) > zw - 2) ch[k][strlen(ch[k]) - 1] = 0;
      strncpy(al[k][0], ch[k], sizeof(al[k][0]) - 1); al[k][0][sizeof(al[k][0]) - 1] = 0; aw[k][0] = (uint8_t)decoTTWidth(al[k][0]); an[k] = 1;
    }
    int mn = 128, mx = 0;
    for (int l = 0; l < an[k]; l++) { const int x = k * zw + (zw - aw[k][l]) / 2; if (x < mn) mn = x; if (x + aw[k][l] > mx) mx = x + aw[k][l]; }
    bx[k] = mn; bw[k] = mx - mn; bh[k] = 7 + (an[k] - 1) * 6;
    drawCh(k, nc == 2 ? c2[k] : cc[k]);
  }
  Serial.println("[DECO] qcm reponses : lignes=" + String(an[0]) + "/" + String(an[1]) + "/" + String(nc > 2 ? an[2] : 0) + " largeurs=" + String(aw[0][0]) + "," + String(aw[0][1]) + " " + String(aw[1][0]) + "," + String(aw[1][1]) + " " + String(nc > 2 ? aw[2][0] : 0) + "," + String(nc > 2 ? aw[2][1] : 0) + " zone=" + String(zw) + " | " + String(al[0][0]) + (an[0] > 1 ? String(" / ") + al[0][1] : String("")) + " ; " + String(al[1][0]) + (an[1] > 1 ? String(" / ") + al[1][1] : String("")) + " ; " + String(nc > 2 ? al[2][0] : "") + ((nc > 2 && an[2] > 1) ? String(" / ") + al[2][1] : String("")));
  Serial.println("[DECO] qcm : " + String(nql) + " ligne(s), " + String(nc) + " choix, bonne reponse " + String(correct) + ", heap=" + String(ESP.getFreeHeap()));
  // decompte : barre (118 px) + chiffre des secondes
  const unsigned long totalMs = (nql >= 3 ? 9000UL : 7000UL), t0 = millis();
  int lastW = 118, lastSec = -1; bool red3 = false;
  display->fillRect(0, 28, 118, 3, blueBar);
  while (millis() - t0 < totalMs) {
    if (decoStop(preview)) { display->setFont(NULL); return 0; }
    const unsigned long el = millis() - t0, left = totalMs - el;
    const int w = (int)(118UL * left / totalMs);
    if (w < lastW) decoQBgRect(w, 28, lastW - w, 3);
    lastW = w;
    if (left <= 3000UL && !red3) { red3 = true; display->fillRect(0, 28, w > 0 ? w : 0, 3, red); }
    const int sec = (int)((left + 999UL) / 1000UL);
    if (sec != lastSec) { lastSec = sec; decoQBgRect(119, 25, 9, 7); char sb[6]; snprintf(sb, sizeof(sb), "%d", sec); decoQText(122, 31, red3 ? red : gold, sb); }
    delay(40);
  }
  // reponse : les mauvais choix s'eteignent, le bon clignote puis reste souligne
  decoQBgRect(0, 25, 128, 7);
  const int a = correct - 1;
  for (int k = 0; k < nc; k++) if (k != a) { decoQBgRect(bx[k], cy, bw[k] + 2, bh[k]); drawCh(k, grey); }
  for (int b = 0; b < 6; b++) {
    decoQBgRect(bx[a], cy, bw[a] + 2, bh[a] + 2);
    if (b % 2 == 0) { drawCh(a, green); display->drawFastHLine(bx[a], cy + bh[a], bw[a], green); }
    const unsigned long e = millis() + 250UL; while (millis() < e) { if (decoStop(preview)) { display->setFont(NULL); return 0; } delay(20); }
  }
  decoQBgRect(bx[a], cy, bw[a] + 2, bh[a] + 2); drawCh(a, green); display->drawFastHLine(bx[a], cy + bh[a], bw[a], green);   // v313 : la bonne reponse RESTE affichee (soulignee) apres le clignotement
  const unsigned long e2 = millis() + 4500UL; while (millis() < e2) { if (decoStop(preview)) { display->setFont(NULL); return 0; } delay(20); }
  display->setFont(NULL);
  return 1;
}
// v286 -- icones du quiz (11x11) : bulle « ? » et coche, une chaine de 11 caracteres par ligne ('#' = allume)
static const char *const QUIZ_ICON_Q[11] = {".#########.", "#.........#", "#...###...#", "#..#...#..#", "#......#..#", "#.....#...#", "#....#....#", "#.........#", "#....#....#", ".#########.", "..##......."};
static const char *const QUIZ_ICON_OK[11] = {"..........#", ".........##", "........##.", "#......##..", "##....##...", ".##..##....", "..####.....", "...##......", "...........", "...........", "..........."};
static bool decoDrawQuote(const char *qIn, const char *suiteIn, bool preview)
{
  const int PITCH = 8;
  const bool quiz = g_quoteQuiz;
  decoQuoteBgLoad();   // v308 : fond d'image du theme (et sa fenetre de texte) AVANT la mise en lignes
  const int XO = quiz ? 14 : g_qX0, XE = g_qX1, WMAX = quiz ? 110 : (g_qX1 - g_qX0 - (g_qX0 > 0 || g_qX1 < 128 ? 2 : 4));   // quiz : 14 px reserves a gauche pour l'icone ; fond a fenetre : on reste dans la partie utile
  char Q[240], S[80], lines[16][40]; uint8_t widths[16]; int n = 0;   // v339 : plus statiques (1 Ko de statique en moins)
  strncpy(Q, qIn, sizeof(Q) - 1); Q[sizeof(Q) - 1] = 0; for (char *c = Q; *c; c++) *c = (char)toupper((unsigned char)*c);
  strncpy(S, suiteIn ? suiteIn : "", sizeof(S) - 1); S[sizeof(S) - 1] = 0; for (char *c = S; *c; c++) *c = (char)toupper((unsigned char)*c);
  auto wrap = [&](const char *t) {
    const int len = (int)strlen(t); int i = 0;
    while (i < len && n < 16) {
      while (i < len && t[i] == ' ') i++;
      if (i >= len) break;
      int j = i, last = -1, w = 0;
      for (; j < len && j - i < 38; j++) { const int a = tomThumbCharAdvance(t[j]); if (w + a > WMAX) break; w += a; if (t[j] == ' ') last = j; }
      int cut = (j >= len) ? len : ((last > i) ? last : j);
      int l = cut - i; memcpy(lines[n], t + i, l); lines[n][l] = 0;
      while (l > 0 && lines[n][l - 1] == ' ') lines[n][--l] = 0;
      int lw = 0; for (int k = 0; k < l; k++) lw += tomThumbCharAdvance(lines[n][k]);
      widths[n] = (uint8_t)(lw > 255 ? 255 : lw); n++;
      i = cut;
    }
  };
  wrap(Q);
  const int nq = n;                                             // lignes du texte
  const bool answer = (S[0] && Q[strlen(Q) - 1] == '?');        // devinette : reponse revelee apres une pause
  if (S[0] && n < 15) {
    lines[n][0] = 0; widths[n] = 0; n++;                        // ligne vide
    if (answer) wrap(S); else { char a[90]; snprintf(a, sizeof(a), "- %s", S); wrap(a); }
  }
  if (n == 0) return true;
  if (ESP.getMaxAllocHeap() < 8000) { Serial.println("[DECO] citation : tas insuffisant"); return true; }
  GFXcanvas1 cvs(128, n * PITCH);
  const uint8_t *buf = cvs.getBuffer();
  if (!buf) { Serial.println("[DECO] citation : canvas non alloue"); return true; }
  cvs.fillScreen(0); cvs.setTextWrap(false); cvs.setFont(&TomThumb); cvs.setTextColor(1);
  for (int i = 0; i < n; i++) { cvs.setCursor(XO + ((XE - XO) - widths[i]) / 2, i * PITCH + 5); cvs.print(lines[i]); }
  const int H = n * PITCH;
  const int VY0 = g_qY0, VY1 = g_qY1, VH = VY1 - VY0;   // v312 : fenetre verticale du texte (lignes de la dalle)
  const bool scrolls = H > VH;
  const int maxOff = scrolls ? (H - VH) : 0;
  const uint16_t cq = g_qCq, ca = g_qCa;   // v305 : couleurs du fond (or / blanc par defaut)
  Serial.println("[DECO] citation (TomThumb) : " + String(n) + " lignes (" + String(nq) + " de texte" + (answer ? ", reponse" : "") + "), " + (scrolls ? String("defile ") + String(maxOff) + " px" : String("fixe")) + ", heap=" + String(ESP.getFreeHeap()));
  int shownRows = H;   // lignes de pixels visibles (la reponse d'une devinette est masquee au debut)
  auto blit = [&](int off) {
    const int base = scrolls ? 0 : -((VH - H) / 2);
    for (int y = 0; y < 32; y++) {
      if (y < VY0 || y >= VY1) { for (int x = 0; x < 128; x++) display->drawPixel(x, y, decoQBgPx(x, y)); continue; }
      const int sy = base + off + (y - VY0);
      if (sy < 0 || sy >= H || sy >= shownRows) { for (int x = 0; x < 128; x++) display->drawPixel(x, y, decoQBgPx(x, y)); continue; }
      const uint16_t col = (sy / PITCH > nq) ? ca : cq;
      const uint8_t *row = buf + (size_t)sy * 16, *prv = (sy > 0 && sy - 1 < shownRows) ? buf + (size_t)(sy - 1) * 16 : nullptr, *nxt = (sy + 1 < H && sy + 1 < shownRows) ? buf + (size_t)(sy + 1) * 16 : nullptr;
      for (int x = 0; x < 128; x++) {
        uint16_t c;
        if (row[x >> 3] & (0x80 >> (x & 7))) c = col;
        else if (g_qBg && g_qShadow && g_qOutline && ((prv && ((x > 0 && (prv[(x - 1) >> 3] & (0x80 >> ((x - 1) & 7)))) || (prv[x >> 3] & (0x80 >> (x & 7))) || (x < 127 && (prv[(x + 1) >> 3] & (0x80 >> ((x + 1) & 7)))))) || (nxt && ((x > 0 && (nxt[(x - 1) >> 3] & (0x80 >> ((x - 1) & 7)))) || (nxt[x >> 3] & (0x80 >> (x & 7))) || (x < 127 && (nxt[(x + 1) >> 3] & (0x80 >> ((x + 1) & 7)))))) || (x > 0 && (row[(x - 1) >> 3] & (0x80 >> ((x - 1) & 7)))) || (x < 127 && (row[(x + 1) >> 3] & (0x80 >> ((x + 1) & 7)))))) c = g_qShCol;   // contour complet (8 voisins)
        else if (g_qBg && g_qShadow && !g_qOutline && prv && x > 0 && (prv[(x - 1) >> 3] & (0x80 >> ((x - 1) & 7)))) c = g_qShCol;   // ombre portee (texte decale de 1,1)
        else c = decoQBgPx(x, y);
        display->drawPixel(x, y, c);
      }
    }
  };
  auto icon = [&]() {
    if (!quiz) return;
    const char *const *ic = (shownRows < H) ? QUIZ_ICON_Q : QUIZ_ICON_OK;
    const uint16_t c = (shownRows < H) ? display->color565(0, 200, 255) : display->color565(0, 255, 120);
    for (int y = 0; y < 11; y++) for (int x = 0; x < 11; x++) display->drawPixel(x, y, (ic[y][x] == '#') ? c : decoQBgPx(x, y));
  };
  auto waitMs = [&](unsigned long ms) -> bool { const unsigned long e = millis() + ms; while (millis() < e) { if (decoStop(preview)) return false; delay(20); } return true; };
  int off = 0;
  if (answer) shownRows = nq * PITCH;
  blit(off); icon();
  unsigned long qWait = scrolls ? 3000UL : (4000UL + (unsigned long)strlen(Q) * 40UL);
  if (answer && g_quoteQuiz) qWait = 6000UL + (unsigned long)strlen(Q) * 40UL;   // v285 : temps de reflexion du quiz
  if (!waitMs(qWait)) return false;
  if (answer) { shownRows = H; blit(off); icon(); if (!waitMs(g_quoteQuiz ? 5000UL : (scrolls ? 1500UL : 4000UL))) return false; }
  if (scrolls) {
    for (off = 1; off <= maxOff; off++) {
      const unsigned long t0 = millis();
      if (decoStop(preview)) return false;
      blit(off); icon();
      while (millis() - t0 < QUOTE_STEP_MS) { if (decoStop(preview)) return false; delay(2); }
    }
    if (!waitMs(3000UL)) return false;
  }
  return true;
}

