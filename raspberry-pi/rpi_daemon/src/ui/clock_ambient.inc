// ============================================================================
// GENERE par tools/port_clock_ambient.py depuis RecalBox_DMD.ino (FW_VERSION_NUM 382, lignes 12105-12449).
// NE PAS EDITER A LA MAIN : relancer le script. Themes d'horloge animes : Plasma, Life, Stars, Rain, Lava.
// ============================================================================
struct LavaBlob { float x, y, vx, vy, r, T, ax; bool att, alive; };   // v309 : r = taille (masse ~ r^2), fusions et reapparitions
struct LavaSt { LavaBlob b[7]; uint8_t oil[3], wax[3]; float pool, budT; float F[8][32]; };   // v382 : F = champ de temperature de l'huile (cellules de 4 x 4 px, rangee 0 = haut)
static_assert(sizeof(LavaSt) <= 2048, "LavaSt depasse s_amb.sand");
struct LavaBall { float x, y, R, R2; };

struct AmbRain { int16_t head[128]; uint8_t len[128], spd[128]; };
struct AmbStar { float x, y, z; int16_t px, py; };
struct ClkScratch { uint8_t mask[512]; uint8_t outl[512]; uint8_t state[1024]; };   // masque des chiffres, halo (1 px) et etat du fond
union AmbScratch {   // 2 048 o au total
  uint16_t pal[256];                                   // plasma (ambiance)
  AmbRain  rain;                                       // pluie (ambiance)
  AmbStar  st[48];                                     // etoiles (ambiance)
  uint8_t  life[2][512];                               // jeu de la vie (ambiance, 1 bit par cellule)
  uint8_t  sand[2048];                                 // sable (4 bits par cellule, 6 couleurs)
  ClkScratch clk;                                      // themes d'horloge ambiants
};
static AmbScratch s_amb;
static uint8_t s_sin8[256]; static bool s_sinInit = false;
static void decoSinInit() { if (!s_sinInit) { for (int i = 0; i < 256; i++) s_sin8[i] = (uint8_t)(127.5 + 127.5 * sin(i * 2.0 * M_PI / 256.0)); s_sinInit = true; } }
static uint16_t decoRainbow(uint8_t h)
{
  const uint8_t region = h / 43, rem = (uint8_t)((h - region * 43) * 6);
  uint8_t r, g, b;
  switch (region) {
    case 0:  r = 255;       g = rem;       b = 0;         break;
    case 1:  r = 255 - rem; g = 255;       b = 0;         break;
    case 2:  r = 0;         g = 255;       b = rem;       break;
    case 3:  r = 0;         g = 255 - rem; b = 255;       break;
    case 4:  r = rem;       g = 0;         b = 255;       break;
    default: r = 255;       g = 0;         b = 255 - rem; break;
  }
  return display->color565(r / 2, g / 2, b / 2);
}

// Plasma NON REPETITIF : 4 termes (x, y, diagonale, distance a un centre mobile) dont les phases avancent a des vitesses
// aleatoires (tirees a chaque lancement) et incommensurables, palette qui tourne a vitesse propre : le motif ne revient jamais
// a l'identique (avant : 3 termes a vitesses entieres -> boucle exacte de 256 images, « on dirait un gif qui boucle »).
struct PlasmaSt { uint32_t p[4], inc[4], pc[2], incc[2], pal, incp; int kx, ky, kd, cx, cy; };
static PlasmaSt s_pl;
static void decoPlasmaInit(uint16_t *pal)
{
  decoSinInit();
  for (int i = 0; i < 256; i++) pal[i] = decoRainbow((uint8_t)i);
  for (int i = 0; i < 4; i++) { s_pl.p[i] = (uint32_t)random(0, 65536); s_pl.inc[i] = (uint32_t)random(120, 520); }
  for (int i = 0; i < 2; i++) { s_pl.pc[i] = (uint32_t)random(0, 65536); s_pl.incc[i] = (uint32_t)random(40, 190); }
  s_pl.pal = (uint32_t)random(0, 65536); s_pl.incp = (uint32_t)random(30, 110);
  s_pl.kx = (int)random(2, 5); s_pl.ky = (int)random(4, 8); s_pl.kd = (int)random(1, 4); s_pl.cx = 64; s_pl.cy = 16;
}
static void decoPlasmaStep()
{
  for (int i = 0; i < 4; i++) s_pl.p[i] += s_pl.inc[i];
  for (int i = 0; i < 2; i++) s_pl.pc[i] += s_pl.incc[i];
  s_pl.pal += s_pl.incp;
  s_pl.cx = 64 + ((int)s_sin8[(s_pl.pc[0] >> 8) & 255] - 128) * 56 / 128;
  s_pl.cy = 16 + ((int)s_sin8[(s_pl.pc[1] >> 8) & 255] - 128) * 14 / 128;
}
// v294 -- palette monochrome (degrade sombre -> teinte -> blanc) si une couleur d'animation est choisie, sinon l'arc-en-ciel
static void decoPlasmaPalette(uint16_t *pal, int32_t color)
{
  if (color < 0) { for (int i = 0; i < 256; i++) pal[i] = decoRainbow((uint8_t)i); return; }
  const int r = (color >> 16) & 255, g = (color >> 8) & 255, b = color & 255;
  for (int i = 0; i < 256; i++) {
    const int f = s_sin8[i];                           // 0..255
    const int w = (f > 215) ? (f - 215) * 3 : 0;       // crete : tire vers le blanc
    int rr = r * f / 255 + w, gg = g * f / 255 + w, bb = b * f / 255 + w; if (rr > 255) rr = 255; if (gg > 255) gg = 255; if (bb > 255) bb = 255;
    pal[i] = display->color565((uint8_t)(rr / 2), (uint8_t)(gg / 2), (uint8_t)(bb / 2));
  }
}
static inline uint16_t decoPlasmaAt(int x, int y, const uint16_t *pal)
{
  const int v1 = s_sin8[(x * s_pl.kx + (int)(s_pl.p[0] >> 8)) & 255];
  const int v2 = s_sin8[(y * s_pl.ky + (int)(s_pl.p[1] >> 8)) & 255];
  const int v3 = s_sin8[((x + y) * s_pl.kd + (int)(s_pl.p[2] >> 8)) & 255];
  const int ddx = x - s_pl.cx, ddy = (y - s_pl.cy) * 3;
  const int v4 = s_sin8[((int)sqrtf((float)(ddx * ddx + ddy * ddy)) * 3 + (int)(s_pl.p[3] >> 8)) & 255];
  return pal[(((v1 + v2 + v3 + v4) >> 2) + (int)(s_pl.pal >> 8)) & 255];
}

// ---- themes d'horloge ambiants (ids RETRO_THEME_COUNT .. +3) ----
static inline bool clkBit(const uint8_t *a, int x, int y) { return (a[y * 16 + (x >> 3)] & (0x80 >> (x & 7))) != 0; }
static void clkSetBit(uint8_t *a, int x, int y) { if (x >= 0 && x < 128 && y >= 0 && y < 32) a[y * 16 + (x >> 3)] |= (uint8_t)(0x80 >> (x & 7)); }
static void clkBuildMask(int h, int m)
{
  uint8_t *mk = s_amb.clk.mask, *ol = s_amb.clk.outl; memset(mk, 0, 512); memset(ol, 0, 512);
  const uint8_t lx[5] = {6, 30, 54, 78, 102};
  const char buf[5] = {(char)('0' + h / 10), (char)('0' + h % 10), ':', (char)('0' + m / 10), (char)('0' + m % 10)};
  for (int ci = 0; ci < 5; ci++) {
    int idx = -1; for (int i = 0; i < 11; i++) if (font_chars[i] == buf[ci]) { idx = i; break; }
    if (idx < 0) continue;
    uint8_t rows[7]; memcpy_P(rows, font5x7[idx], 7);
    for (int row = 0; row < 7; row++) for (int col = 0; col < 5; col++) if (rows[row] & (0x10 >> col))
      for (int dy = 0; dy < 4; dy++) for (int dx = 0; dx < 4; dx++) clkSetBit(mk, lx[ci] + col * 4 + dx, 2 + row * 4 + dy);
  }
  for (int y = 0; y < 32; y++) for (int x = 0; x < 128; x++) if (clkBit(mk, x, y))
    for (int dy = -1; dy <= 1; dy++) for (int dx = -1; dx <= 1; dx++) clkSetBit(ol, x + dx, y + dy);   // halo noir de 1 px autour des chiffres
}
static uint16_t s_clkDigit = 0xFFFF;   // couleur des chiffres du theme en cours
static inline void clkPx(int x, int y, uint16_t c)   // ecrit un pixel de fond en respectant les chiffres (blanc) et leur halo (noir)
{
  if (x < 0 || x >= 128 || y < 0 || y >= 32) return;
  display->drawPixel(x, y, clkBit(s_amb.clk.mask, x, y) ? s_clkDigit : (clkBit(s_amb.clk.outl, x, y) ? 0 : c));
}

// ==================================================================
// v300 -- LAVA LAMP (ambiance 8 et theme d'horloge 14) : un bassin de cire au fond d'un bac d'huile coloree ; des blobs de cire, chauffes en bas, montent en etirant une
// « queue » (chapelet de perles = excroissance elastique) puis se pincent, refroidissent en haut et retombent se fondre dans le bassin. TEMPERATURE GLOBALE tg (0 -> 1) :
// plus l'huile chauffe, plus la cire est molle et elastique (flottabilite plus forte, excroissances plus hautes avant rupture, pincements plus frequents). Rendu : champ de metaballs
// calcule ligne par ligne (aucun tampon d'image) ; huile = fond degrade, cire = degrade bord -> coeur -> blanc.
// ==================================================================
static LavaBall *s_lvBalls = nullptr; static int s_lvN = 0;   // v339 : alloue au 1er lavaBuild(), libere par decoScratchRelease() (2,7 Ko de statique en moins)
static void lavaInit(void *Lv, const uint8_t *oil, const uint8_t *wax)   // void * : l'IDE Arduino genere les prototypes avant les structures
{
  LavaSt *L = (LavaSt *)Lv;
  memcpy(L->oil, oil, 3); memcpy(L->wax, wax, 3); L->pool = 0; L->budT = 0; memset(L->F, 0, sizeof(L->F));
  for (int i = 0; i < 7; i++) { LavaBlob &b = L->b[i]; b.r = random(22, 64) / 10.0f; b.ax = 10.0f + i * 18.0f + random(-30, 30) / 10.0f; b.x = b.ax; b.y = 29.0f; b.vx = 0; b.vy = 0; b.T = random(2, 15) / 100.0f; b.att = true; b.alive = true; }
}
// v309 -- MASSE : la taille r d'un blob (masse ~ r^2) change : deux blobs libres qui se touchent FUSIONNENT (masses additionnees, quantite de mouvement et temperature moyennees) ;
// un gros blob MONTE plus lentement (portance / traînee ~ r^-0,8) mais REDESCEND plus vite (poids ~ r^0,8) ; un blob qui retombe dans le bassin s'y FOND (le bassin grossit) ;
// de nouveaux petits blobs naissent ensuite du bassin chaud (le bassin maigrit d'autant).
// v382 -- POINT CHAUD : au lieu d'une temperature uniforme, un champ 2D (32 x 8 cellules de 4 px) : une resistance chauffante au centre du fond injecte de la chaleur (gaussienne de ~14 px) ;
// la chaleur diffuse vers les bords et vers le haut (2 pas de diffusion par image), l'huile perd un peu de chaleur partout et davantage a la surface. Le centre chauffe donc bien avant les bords.
static void lavaField(void *Lv)
{
  LavaSt *L = (LavaSt *)Lv; float N[8][32];
  for (int s = 0; s < 2; s++) {
    for (int j = 0; j < 8; j++) for (int i = 0; i < 32; i++) {
      const float c = L->F[j][i], l = L->F[j][i > 0 ? i - 1 : 0], r = L->F[j][i < 31 ? i + 1 : 31], u = L->F[j > 0 ? j - 1 : 0][i], d = L->F[j < 7 ? j + 1 : 7][i];
      float v = c + 0.18f * (l + r + u + d - 4.0f * c) - 0.0006f * c;
      if (j == 0) v -= 0.005f * c;
      if (j == 7) { const float e = (i - 15.5f) / 3.5f; v += 0.03f * expf(-e * e); }
      N[j][i] = v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
    }
    memcpy(L->F, N, sizeof(N));
  }
}
static float lavaTemp(const void *Lv, float x, float y)   // temperature locale (0..1), interpolation bilineaire
{
  const LavaSt *L = (const LavaSt *)Lv;
  float fx = x * 0.25f - 0.5f, fy = y * 0.25f - 0.5f;
  if (fx < 0.0f) fx = 0.0f; if (fx > 30.999f) fx = 30.999f; if (fy < 0.0f) fy = 0.0f; if (fy > 6.999f) fy = 6.999f;
  const int i = (int)fx, j = (int)fy; const float a = fx - i, b = fy - j;
  return (L->F[j][i] * (1.0f - a) + L->F[j][i + 1] * a) * (1.0f - b) + (L->F[j + 1][i] * (1.0f - a) + L->F[j + 1][i + 1] * a) * b;
}
static void lavaStep(void *Lv, float tg, float t)
{
  LavaSt *L = (LavaSt *)Lv;
  (void)tg;   // v382 : la temperature n'est plus globale : champ local lavaField()
  lavaField(L);
  for (int i = 0; i < 7; i++) {
    LavaBlob &b = L->b[i]; if (!b.alive) continue;
    const float tl = lavaTemp(L, b.att ? b.ax : b.x, b.att ? 30.0f : b.y);   // v382 : temperature LOCALE (ancrage dans le bassin pour une excroissance, position du blob sinon)
    const float Lmax = 8.0f + 20.0f * tl;
    const float act = (tl - 0.15f) / 0.5f < 0.0f ? 0.0f : ((tl - 0.15f) / 0.5f > 1.0f ? 1.0f : (tl - 0.15f) / 0.5f);   // huile froide = cire figee, fonte progressive
    const float rr = b.r / 4.0f, inv = 1.0f / rr, up = powf(rr, -0.8f), down = powf(rr, 0.8f);
    if (b.y > 24.0f) b.T += 0.010f * (0.1f + tl) * (0.2f + tl) * inv; else if (b.y < 9.0f) b.T -= 0.014f * inv; else b.T -= 0.0012f * inv;   // chauffe en bas, refroidit en haut : un gros blob a plus d'inertie thermique
    if (b.T > 1) b.T = 1; if (b.T < 0) b.T = 0;
    const float bf = b.T - 0.45f, K = 0.0066f * (0.4f + 1.1f * tl) * act;
    float ay = (bf > 0) ? -bf * K * up : -bf * 0.35f * K * down;   // montee : plus lente si gros ; descente : plus rapide si gros
    float ax = 0.0025f * sinf(0.35f * t + i * 1.7f) * act;
    if (b.att) {   // excroissance : rappel elastique vers l'ancrage dans le bassin
      const float dx = b.ax - b.x, dy = 33.0f - b.y, d = sqrtf(dx * dx + dy * dy) + 1e-3f, st = d - 3.0f;
      if (st > 0) { ax += 0.00034f * st * dx / d; ay += 0.00034f * st * dy / d; }
      if (d > Lmax || (b.T > 0.9f && tl > 0.45f && d > 7.0f && random(0, 1000) < 2)) b.att = false;       // la queue se pince : le blob se detache
    } else {
      for (int j = 0; j < 7; j++) if (j != i && L->b[j].alive && !L->b[j].att) { const LavaBlob &o = L->b[j]; const float dx = o.x - b.x, dy = o.y - b.y, d = sqrtf(dx * dx + dy * dy) + 1e-3f; if (d < 11.0f) { ax += 0.0004f * dx / d; ay += 0.0004f * dy / d; } }
      if (b.y > 27.5f && b.vy >= 0) { L->pool += b.r * b.r * 0.06f; if (L->pool > 3.5f) L->pool = 3.5f; b.alive = false; continue; }   // retombe : se FOND dans le bassin (qui grossit)
    }
    const float dmp = 0.82f + 0.14f * tl;   // cire froide = tres visqueuse (amortissement fort), plus fluide en chauffant
    b.vx = (b.vx + ax) * dmp; b.vy = (b.vy + ay) * dmp; b.x += b.vx; b.y += b.vy;
    if (b.x < 4) { b.x = 4; b.vx = fabsf(b.vx) * 0.5f; } else if (b.x > 124) { b.x = 124; b.vx = -fabsf(b.vx) * 0.5f; }
    if (b.y < 3) { b.y = 3; b.vy = fabsf(b.vy) * 0.3f; } else if (b.y > 30) { b.y = 30; b.vy = -fabsf(b.vy) * 0.3f; }
  }
  for (int i = 0; i < 7; i++) for (int j = i + 1; j < 7; j++) {   // fusions : deux blobs libres qui se touchent
    LavaBlob &p = L->b[i], &q = L->b[j];
    if (!p.alive || !q.alive || p.att || q.att) continue;
    const float dx = q.x - p.x, dy = q.y - p.y;
    if (dx * dx + dy * dy < 0.5625f * (p.r + q.r) * (p.r + q.r)) {   // distance < 0,75 (r1 + r2)
      const float mp = p.r * p.r, mq = q.r * q.r, m = mp + mq;
      p.x = (p.x * mp + q.x * mq) / m; p.y = (p.y * mp + q.y * mq) / m; p.vx = (p.vx * mp + q.vx * mq) / m; p.vy = (p.vy * mp + q.vy * mq) / m; p.T = (p.T * mp + q.T * mq) / m;
      p.r = sqrtf(m); if (p.r > 9.0f) p.r = 9.0f; q.alive = false;
    }
  }
  if (--L->budT <= 0) {   // naissance d'un petit blob depuis le bassin CHAUD (toutes les 6 a 15 s) : seulement la ou le bassin est assez chaud (v382 : pres du point chaud d'abord)
    L->budT = (float)random(200, 500);
    float bx = -1.0f; for (int k = 0; k < 10; k++) { const float cx = (float)random(10, 118); if (lavaTemp(L, cx, 30.0f) > 0.3f) { bx = cx; break; } }
    if (bx >= 0.0f) for (int i = 0; i < 7; i++) if (!L->b[i].alive) {
      LavaBlob &b = L->b[i]; b.r = random(22, 40) / 10.0f; b.ax = bx; b.x = b.ax; b.y = 29.0f; b.vx = 0; b.vy = 0; b.T = random(10, 30) / 100.0f; b.att = true; b.alive = true;
      L->pool -= b.r * b.r * 0.06f; if (L->pool < 0) L->pool = 0; break;
    }
  }
}static void lavaAdd(float x, float y, float rv) { if (s_lvBalls && s_lvN < 170) { LavaBall &q = s_lvBalls[s_lvN++]; q.R = rv * 1.57f; q.R2 = q.R * q.R; q.x = x; q.y = y; } }
static void lavaBuild(const void *Lv, float tg, float t)
{
  const LavaSt *L = (const LavaSt *)Lv;
  (void)tg;
  if (!s_lvBalls) s_lvBalls = (LavaBall *)malloc(170 * sizeof(LavaBall));
  s_lvN = 0; if (!s_lvBalls) return;   // pas de tas : champ vide plutot qu'un plantage
  for (int xk = 0; xk <= 128; xk += 10) lavaAdd((float)xk, 34.2f - 1.2f * lavaTemp(L, (float)xk, 30.0f) * sinf(0.7f * t + xk * 0.15f), 7.0f + L->pool * 0.5f);   // bassin de cire (ondule quand il chauffe)
  for (int i = 0; i < 7; i++) {
    const LavaBlob &b = L->b[i]; if (!b.alive) continue;
    const float tl = lavaTemp(L, b.x, b.y), ta = lavaTemp(L, b.ax, 30.0f);   // v382 : temperature locale du blob / de son ancrage
    { const float rr = b.r * (0.95f + 0.25f * tl), vs = sqrtf(b.vx * b.vx + b.vy * b.vy);   // v306c : blobs DEFORMABLES : tete + 2 lobes qui tournent lentement, etirement le long du mouvement, respiration
      const float br = 1.0f + 0.16f * sinf(t * 0.6f + i * 2.1f) * (0.3f + tl), ang = t * 0.28f + i * 1.9f, lob = rr * (0.45f + 0.35f * tl);
      lavaAdd(b.x, b.y, rr * br); lavaAdd(b.x, b.y, rr * br);
      lavaAdd(b.x + cosf(ang) * lob, b.y + sinf(ang) * lob * 0.8f, rr * 0.62f); lavaAdd(b.x - cosf(ang * 0.8f + 1.0f) * lob, b.y - sinf(ang * 0.8f + 1.0f) * lob * 0.8f, rr * 0.55f);
      if (!b.att && vs > 0.015f) { const float k = (vs * 55.0f > rr * 1.4f) ? rr * 1.4f : vs * 55.0f; lavaAdd(b.x - b.vx / vs * k, b.y - b.vy / vs * k, rr * 0.7f); lavaAdd(b.x + b.vx / vs * k * 0.5f, b.y + b.vy / vs * k * 0.5f, rr * 0.6f); } }
    if (!b.att) continue;
    const float dx = b.x - b.ax, dy = b.y - 33.0f, d = sqrtf(dx * dx + dy * dy);
    if (d <= 5.0f) continue;
    int n = (int)(d / 3.0f); if (n > 9) n = 9;
    const float pinch = 1.0f - 0.30f * (d / (8.0f + 20.0f * ta) > 1.0f ? 1.0f : d / (8.0f + 20.0f * ta));
    for (int k = 1; k <= n; k++) {   // perles de la queue : plus fines vers le milieu, ondulantes
      const float u = k / (n + 1.0f);
      lavaAdd(b.ax + dx * u + sinf(u * 3.14159f) * 1.2f * sinf(t * 0.9f + i), 33.0f + dy * u, b.r * (0.42f + 0.40f * (1.0f - u)) * pinch);
    }
  }
}
static void lavaRow(int y, uint8_t *lv)   // champ de la ligne y (x 0..127) -> lv[x] = champ x 100 (0..255)
{
  float f[128]; memset(f, 0, sizeof(f));
  const float yy = y + 0.5f;
  for (int k = 0; k < s_lvN; k++) {
    const LavaBall &q = s_lvBalls[k];
    const float dy = yy - q.y, dy2 = dy * dy;
    if (dy2 >= q.R2) continue;
    int x0 = (int)(q.x - q.R); if (x0 < 0) x0 = 0; int x1 = (int)(q.x + q.R) + 1; if (x1 > 127) x1 = 127;
    for (int x = x0; x <= x1; x++) { const float dx = x + 0.5f - q.x, d2 = dx * dx + dy2; if (d2 < q.R2) { const float w = 1.0f - d2 / q.R2; f[x] += w * w; } }
  }
  for (int x = 0; x < 128; x++) { const int v = (int)(f[x] * 100.0f); lv[x] = (uint8_t)(v > 255 ? 255 : v); }
}
static uint16_t lavaColor(const void *Lv, int x, int y, int f)
{
  const LavaSt *L = (const LavaSt *)Lv;
  const int ok = 30 + (y * 55) / 31;   // huile : degrade, sombre en haut, plus lumineux en bas (%)
  int r = L->oil[0] * ok / 100, g = L->oil[1] * ok / 100, b = L->oil[2] * ok / 100;
  { const int hg = (int)(L->F[y >> 2][x >> 2] * 28.0f);   // v382 : halo de chaleur : l'huile chaude tire vers la couleur de la cire (28 % au maximum)
    r += (L->wax[0] - r) * hg / 100; g += (L->wax[1] - g) * hg / 100; b += (L->wax[2] - b) * hg / 100; }
  if (f >= 32) {
    const int a = (f < 44) ? (f - 32) * 100 / 12 : 100;                  // bord anticrenele
    int t = (f > 44) ? ((f - 44) > 100 ? 100 : (f - 44)) : 0;            // coeur 0..100
    const int wh = (t > 70) ? (t - 70) * 2 : 0;                          // coeur tire vers le blanc
    int wr = L->wax[0] * (55 + 45 * t / 100) / 100 + wh, wg = L->wax[1] * (55 + 45 * t / 100) / 100 + wh, wb = L->wax[2] * (55 + 45 * t / 100) / 100 + wh;
    if (wr > 255) wr = 255; if (wg > 255) wg = 255; if (wb > 255) wb = 255;
    r = (r * (100 - a) + wr * a) / 100; g = (g * (100 - a) + wg * a) / 100; b = (b * (100 - a) + wb * a) / 100;
  }
  return display->color565((uint8_t)r, (uint8_t)g, (uint8_t)b);
}
// couleur d'huile « complementaire sombre » d'une cire (theme d'horloge : une seule couleur d'animation)
static void lavaOilFromWax(const uint8_t *wax, uint8_t *oil) { for (int i = 0; i < 3; i++) oil[i] = (uint8_t)((255 - wax[i]) * 30 / 100 + 6); }
static void decoClockAmbient(int kind, int h, int m, unsigned long ms)
{
  static unsigned long lastMs = 0xFFFFFFFFUL, lastStep = 0; static int lastKind = -1, lastMin = -1, lastHour = -1;
  static unsigned gens = 0;
  uint8_t *st = s_amb.clk.state;
  uint16_t *pal = (uint16_t *)st;
  AmbRain *rn = (AmbRain *)st; AmbStar *stars = (AmbStar *)st; uint8_t (*life)[512] = (uint8_t (*)[512])st;
  static int lifeCur = 0; static unsigned long lavaT0 = 0;
  static uint16_t lifeCol = 0, rainHead = 0, rainMid = 0, rainTail = 0, lastVer = 0xFFFF; static uint8_t aR = 255, aG = 255, aB = 255;   // v294 : couleurs de l'animation en cours
  const bool fresh = (ms < lastMs) || (kind != lastKind);
  const bool timeChanged = (h != lastHour) || (m != lastMin);
  lastMs = ms;
  auto lget = [&](int b, int x, int y) -> int { x &= 127; y &= 31; const int i = y * 128 + x; return (life[b][i >> 3] >> (i & 7)) & 1; };
  auto lput = [&](int b, int x, int y, int v) { const int i = y * 128 + x; if (v) life[b][i >> 3] |= (uint8_t)(1 << (i & 7)); else life[b][i >> 3] &= (uint8_t)~(1 << (i & 7)); };
  auto style = [&]() {   // v294 : couleurs d'animation et de chiffres (personnalisees ou d'origine)
    static const uint8_t DDEF[5][3] = {{255, 255, 255}, {255, 255, 255}, {255, 230, 120}, {200, 255, 200}, {255, 255, 255}};
    static const uint8_t ADEF[5][3] = {{255, 64, 160}, {0, 190, 60}, {255, 255, 255}, {0, 200, 40}, {255, 110, 0}};
    const int32_t ac = clockAnimCol[kind], dc = clockDigitCol[kind];
    uint8_t dr = DDEF[kind][0], dg = DDEF[kind][1], db = DDEF[kind][2];
    if (dc >= 0) { dr = (dc >> 16) & 255; dg = (dc >> 8) & 255; db = dc & 255; }
    s_clkDigit = display->color565(dr, dg, db);
    aR = ADEF[kind][0]; aG = ADEF[kind][1]; aB = ADEF[kind][2];
    if (ac >= 0) { aR = (ac >> 16) & 255; aG = (ac >> 8) & 255; aB = ac & 255; }
    lifeCol  = display->color565(aR, aG, aB);
    rainHead = display->color565(aR + (255 - aR) * 3 / 4, aG + (255 - aG) * 3 / 4, aB + (255 - aB) * 3 / 4);
    rainMid  = display->color565(aR, aG, aB);
    rainTail = display->color565(aR * 55 / 100, aG * 55 / 100, aB * 55 / 100);
    if (kind == 0) decoPlasmaPalette(pal, ac);
    if (kind == 4) { LavaSt *LL = (LavaSt *)st; LL->wax[0] = aR; LL->wax[1] = aG; LL->wax[2] = aB; lavaOilFromWax(LL->wax, LL->oil); }   // v300 : couleur d'animation = cire, huile = complementaire sombre
  };
  auto repaint = [&]() {   // fond complet + chiffres (au demarrage et a chaque minute)
    for (int y = 0; y < 32; y++) for (int x = 0; x < 128; x++) {
      uint16_t c = 0; if (kind == 1 && lget(lifeCur, x, y)) c = lifeCol;
      clkPx(x, y, c);
    }
  };
  if (fresh) {
    lastKind = kind; lastHour = h; lastMin = m; lastStep = 0; gens = 0;
    clkBuildMask(h, m);
    lastVer = g_clkColorVer;
    display->clearScreen();
    if (kind == 0) decoPlasmaInit(pal);
    else if (kind == 1) { lifeCur = 0; memset(life[0], 0, 512); for (int y = 0; y < 32; y++) for (int x = 0; x < 128; x++) if (random(0, 100) < 28) lput(0, x, y, 1); }
    else if (kind == 2) { for (int i = 0; i < 48; i++) { stars[i].x = random(-640, 640) / 10.0f; stars[i].y = random(-160, 160) / 10.0f; stars[i].z = random(30, 100) / 10.0f; stars[i].px = -1; stars[i].py = -1; } }
    else if (kind == 4) { static const uint8_t o0[3] = {40, 10, 80}, w0[3] = {255, 110, 0}; lavaInit((LavaSt *)st, o0, w0); lavaT0 = ms; }
    else { for (int x = 0; x < 128; x++) { rn->head[x] = -(int)random(0, 40); rn->len[x] = (uint8_t)random(6, 14); rn->spd[x] = (uint8_t)random(1, 3); } }
    style();
    repaint();
    return;
  }
  if (g_clkColorVer != lastVer) { lastVer = g_clkColorVer; style(); display->clearScreen(); repaint(); }   // v294 : changement de couleur en direct
  if (timeChanged) { lastHour = h; lastMin = m; clkBuildMask(h, m); display->clearScreen(); repaint(); }
  if (kind == 0) {   // plasma : toute l'image est reecrite a chaque image (chiffres et halo compris : un seul ecriture par pixel)
    if (ms - lastStep < 33) return; lastStep = ms;
    decoPlasmaStep();
    for (int y = 0; y < 32; y++) for (int x = 0; x < 128; x++) clkPx(x, y, decoPlasmaAt(x, y, pal));
  } else if (kind == 1) {   // jeu de la vie : une generation toutes les 120 ms ; seuls les changements sont dessines
    if (ms - lastStep < 120) return; lastStep = ms; gens++;
    const int nx = 1 - lifeCur; int changes = 0;
    for (int y = 0; y < 32; y++) for (int x = 0; x < 128; x++) {
      const int n = lget(lifeCur, x - 1, y - 1) + lget(lifeCur, x, y - 1) + lget(lifeCur, x + 1, y - 1) + lget(lifeCur, x - 1, y) + lget(lifeCur, x + 1, y) + lget(lifeCur, x - 1, y + 1) + lget(lifeCur, x, y + 1) + lget(lifeCur, x + 1, y + 1);
      const int old = lget(lifeCur, x, y), v = old ? (n == 2 || n == 3) : (n == 3);
      lput(nx, x, y, v);
      if (v != old) { clkPx(x, y, v ? lifeCol : 0); changes++; }
    }
    lifeCur = nx;
    if (changes < 6 || gens >= 400) { gens = 0; memset(life[lifeCur], 0, 512); for (int y = 0; y < 32; y++) for (int x = 0; x < 128; x++) if (random(0, 100) < 28) lput(lifeCur, x, y, 1); display->clearScreen(); repaint(); }
  } else if (kind == 2) {   // etoiles : filent derriere les chiffres
    if (ms - lastStep < 35) return; lastStep = ms;
    for (int i = 0; i < 48; i++) {
      AmbStar &s = stars[i];
      if (s.px >= 0) clkPx(s.px, s.py, 0);
      s.z -= 0.12f;
      bool respawn = (s.z <= 0.4f);
      int px = 0, py = 0;
      if (!respawn) { px = 64 + (int)(s.x / s.z * 8.0f); py = 16 + (int)(s.y / s.z * 8.0f); respawn = (px < 0 || px >= 128 || py < 0 || py >= 32); }
      if (respawn) { s.x = random(-640, 640) / 10.0f; s.y = random(-160, 160) / 10.0f; s.z = random(30, 100) / 10.0f; s.px = -1; s.py = -1; continue; }
      int b = (int)(255.0f * (1.0f - s.z / 10.0f)); if (b < 40) b = 40;
      clkPx(px, py, display->color565(aR * b / 255, aG * b / 255, aB * b / 255)); s.px = (int16_t)px; s.py = (int16_t)py;
    }
  } else if (kind == 4) {   // lava lamp : toute l'image est reecrite a chaque image (un seul ecriture par pixel) ; temperature : 15 % -> 100 % en 40 s
    if (ms - lastStep < 33) return; lastStep = ms;
    LavaSt *LL = (LavaSt *)st; const float t = (ms - lavaT0) / 1000.0f, tg = (t < 6.0f) ? 0.0f : ((t - 6.0f) > 40.0f ? 1.0f : (t - 6.0f) / 40.0f);
    lavaStep(LL, tg, t); lavaBuild(LL, tg, t);
    uint8_t lv[128];
    for (int y = 0; y < 32; y++) { lavaRow(y, lv); for (int x = 0; x < 128; x++) clkPx(x, y, lavaColor(LL, x, y, lv[x])); }
  } else {   // pluie : traînees vertes derriere les chiffres
    if (ms - lastStep < 45) return; lastStep = ms; static unsigned tick = 0; tick++;
    for (int x = 0; x < 128; x++) {
      if (rn->spd[x] == 1 && (tick & 1)) continue;
      const int y = ++rn->head[x];
      if (y >= 0 && y < 32)         clkPx(x, y, rainHead);
      if (y - 1 >= 0 && y - 1 < 32) clkPx(x, y - 1, rainMid);
      if (y - 3 >= 0 && y - 3 < 32) clkPx(x, y - 3, rainTail);
      const int e = y - rn->len[x];
      if (e >= 0 && e < 32) clkPx(x, e, 0);
      if (e > 32) { rn->head[x] = -(int)random(0, 30); rn->len[x] = (uint8_t)random(6, 14); rn->spd[x] = (uint8_t)random(1, 3); }
    }
  }
}

