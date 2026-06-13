
#ifndef BitOrder
typedef uint8_t BitOrder; // Workaround: Adafruit_BusIO attend BitOrder (AVR) mais ESP32 le n'a pas
#endif

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <AnimatedGIF.h>
#include <SD.h>
#include <SPI.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "BluetoothSerial.h"
#include "esp_bt.h"
#include "esp_heap_caps.h"
#include "pngle.h"

// ==================================================
// CONSTANTES DE CONFIGURATION & LIMITES
// ==================================================

// Cache systèmes
#define SYS_CACHE_MAX 300
#define SYS_CACHE_FILE "/systems_cache.dat"

// Cache jeux (bigramme index)
#define GAMES_IDX_MAX 300
#define NB_IDX 703   // 1 + 26*27 entrees bigramme
#define GAMES_CACHE_FILE "/games_cache.bin"

// Limites et seuils de performance
#define SPRITE_LIMIT 800  // Max fichiers par système avant flag slowFlag='L'
#define BIGRAM_SLICE_MAX 8192  // Max allocation heap pour tranche bigramme
#define BIGRAM_FALLBACK_SIZE 2048  // Fallback si offset final non trouvé

// Timings (millisecondes)
#define MQTT_SEMAPHORE_WAIT_MS 100  // Mutex timeout pour commandes MQTT
#define REBOOT_DELAY_MS 1500  // Délai avant redémarrage
#define REBOOT_DISCONNECT_DELAY_MS 50  // Délai après déconnexion
#define SLEEP_DELAY_MS 100  // Délai lors d'attente ou arrêt

// Tailles et formats
#define RAW565_FRAME_WIDTH 128
#define RAW565_FRAME_HEIGHT 32
#define RAW565_FRAME_BYTES (RAW565_FRAME_WIDTH * RAW565_FRAME_HEIGHT * 2)  // 8192

// --------------------------------------------------
// Cache des _defaults par systeme
// --------------------------------------------------
static char (*sysCacheKeys)[32] = nullptr; // SYS_CACHE_MAX x 32 (heap)
static char *sysCacheVals = nullptr;       // SYS_CACHE_MAX (heap)
static char *sysCacheSlowVals = nullptr;   // SYS_CACHE_MAX (heap)
static int  sysCacheCount = 0;

char sysDefaultType(const String &sysName)
{
  for (int i = 0; i < sysCacheCount; i++)
    if (sysName == sysCacheKeys[i]) return sysCacheVals[i];
  return '?';
}

char sysDefaultSlowFlag(const String &sysName)
{
  for (int i = 0; i < sysCacheCount; i++)
    if (sysName == sysCacheKeys[i]) return sysCacheSlowVals[i];
  return 'N';
}

bool loadSysDefaultCache()
{
  File f = SD.open(SYS_CACHE_FILE, FILE_READ);
  if (!f) return false;
  sysCacheCount = 0;
  while (f.available() && sysCacheCount < SYS_CACHE_MAX)
  {
    String line = f.readStringUntil('\n'); line.trim();
    if (line.length() < 3) continue;
    char val = line.charAt(0);
    if (val != 'g' && val != 'p' && val != 'B') continue;

    // Format attendu:
    //   <val> <sysName> <slowFlag>
    // Avec compatibilité:
    //   <val> <sysName>              (slowFlag implicitement 'N')
    String rest = line.substring(2);
    rest.trim();

    int sp2 = rest.indexOf(' ');
    String sysName = (sp2 >= 0) ? rest.substring(0, sp2) : rest;

    char slow = 'N';
    if (sp2 >= 0)
    {
      String flag = rest.substring(sp2 + 1);
      flag.trim();
      if (flag.length() > 0) slow = flag.charAt(0);
    }

    strncpy(sysCacheKeys[sysCacheCount], sysName.c_str(), 31);
    sysCacheKeys[sysCacheCount][31] = '\0';
    sysCacheVals[sysCacheCount] = val;
    sysCacheSlowVals[sysCacheCount] = slow;
    sysCacheCount++;
  }
  f.close();
  Serial.println("[CACHE] charge: " + String(sysCacheCount) + " systemes");
  return sysCacheCount > 0;
}

void saveSysDefaultCache()
{
  SD.remove(SYS_CACHE_FILE);
  File f = SD.open(SYS_CACHE_FILE, FILE_WRITE);
  if (!f) return;
  for (int i = 0; i < sysCacheCount; i++)
  {
    f.print(sysCacheVals[i]); f.print(' ');
    f.print(sysCacheKeys[i]); f.print(' ');
    f.println(sysCacheSlowVals[i]);
  }
  f.close();
  Serial.println("[CACHE] sauvegarde: " + String(sysCacheCount) + " systemes");
}

static void countPngGifOverRec(const String &dirPath, int limit,
                                int &pngCount, int &gifCount,
                                bool &pngOver, bool &gifOver)
{
  if (pngOver && gifOver) return;

  File dir = SD.open(dirPath.c_str());
  if (!dir) return;
  if (!dir.isDirectory()) { dir.close(); return; }

  File entry = dir.openNextFile();
  while (entry)
  {
    if (pngOver && gifOver) { entry.close(); break; }

    String entryName = String(entry.name());
    if (entry.isDirectory())
    {
      String subPath = dirPath + "/" + entryName;
      entry.close();
      countPngGifOverRec(subPath, limit, pngCount, gifCount, pngOver, gifOver);
    }
    else
    {
      if (entryName.endsWith(".raw565"))
      {
        pngCount++;
        if (pngCount > limit) pngOver = true;
      }
      else if (entryName.endsWith(".raw565pack"))
      {
        gifCount++;
        if (gifCount > limit) gifOver = true;
      }
      entry.close();
    }

    entry = dir.openNextFile();
  }
  dir.close();
}

void buildSysDefaultCache()
{
  sysCacheCount = 0;
  File root = SD.open("/systems");
  if (!root) return;
  File entry = root.openNextFile();
  while (entry && sysCacheCount < SYS_CACHE_MAX)
  {
    if (entry.isDirectory())
    {
      String fullName = String(entry.name());
      int slash = fullName.lastIndexOf('/');
      String sysName = (slash >= 0) ? fullName.substring(slash + 1) : fullName;
      if (sysName == "_defaults") { entry.close(); entry = root.openNextFile(); continue; }
      String base = "/systems/_defaults/" + sysName;
      char val = '?';

      bool hasPack = SD.exists((base + ".raw565pack").c_str()) && SD.exists((base + ".meta").c_str());
      bool hasRaw  = SD.exists((base + ".raw565").c_str());

      // Règle:
      // - uniquement .raw565 => 'p'
      // - uniquement .raw565pack + .meta => 'g'
      // - les deux => 'B'
      if (hasPack && hasRaw) val = 'B';
      else if (hasPack)      val = 'g';
      else if (hasRaw)       val = 'p';
      strncpy(sysCacheKeys[sysCacheCount], sysName.c_str(), 31);
      sysCacheKeys[sysCacheCount][31] = '\0';
      sysCacheVals[sysCacheCount] = val;

      int pngCount = 0;
      int gifCount = 0;
      bool pngOver = false;
      bool gifOver = false;

      countPngGifOverRec("/systems/" + sysName, SPRITE_LIMIT, pngCount, gifCount, pngOver, gifOver);
      sysCacheSlowVals[sysCacheCount] = (pngOver || gifOver) ? 'L' : 'N';
      sysCacheCount++;
    }
    entry.close();
    entry = root.openNextFile();
  }
  root.close();
  Serial.println("[CACHE] " + String(sysCacheCount) + " systemes indexes");
  saveSysDefaultCache();
}

// --------------------------------------------------
// Cache des jeux — index bigramme 703 entrees
//
// Index 0       = '#'  (chiffres, tirets, etc.)
// Index 1       = 'A'  (jeux "a" + car. non-lettre)
// Index 2..27   = 'AA'..'AZ'
// Index 28      = 'B'
// ...
// Index 676     = 'Z'
// Index 677..702= 'ZA'..'ZZ'
// Total = 703 entrees (0..702)
//
// bigramTable est alloue dynamiquement en heap
// et libere avant drawPng pour liberer la RAM a pngle
// ==================================================

struct GamesSysIdx { char sysName[32]; uint32_t offset; };
static GamesSysIdx *gamesIdx = nullptr; // GAMES_IDX_MAX en heap
static int         gamesIdxCount  = 0;
static String      gamesCacheFile = GAMES_CACHE_FILE;

// Table bigramme — allouee dynamiquement, liberee avant affichage
static uint32_t *bigramTable      = nullptr; // NB_IDX x 4 bytes en heap
static String    bigramTableSys   = "";
static bool      bigramTableLoaded = false;

// Buffer tranche bigramme courante
static uint8_t  *bigramBuf          = nullptr;
static size_t    bigramBufSize      = 0;
static String    bigramBufKey       = "";
static uint32_t  bigramBufAbsOffset = 0;

void freeBigramBuffer()
{
  if (bigramBuf) { free(bigramBuf); bigramBuf = nullptr; }
  bigramBufSize = 0; bigramBufKey = ""; bigramBufAbsOffset = 0;
}

void freeBigramAll()
{
  freeBigramBuffer();
  if (bigramTable) { free(bigramTable); bigramTable = nullptr; }
  bigramTableSys    = "";
  bigramTableLoaded = false;
}

// Calcule l'index bigramme (0..702)
static int bigramIndex(const String &name)
{
  if (name.length() == 0) return 0;
  char c1 = (char)toupper((unsigned char)name.charAt(0));
  if (!isAlpha(c1)) return 0;
  int i1   = c1 - 'A';
  int base = 1 + i1 * 27;
  if (name.length() < 2) return base;
  char c2 = (char)toupper((unsigned char)name.charAt(1));
  if (!isAlpha(c2)) return base;
  return base + (c2 - 'A') + 1;
}

// Label lisible (ex: 343 -> "MR", 1 -> "A", 0 -> "#")
static String bigramLabel(int bi)
{
  if (bi == 0) return "#";
  int idx = bi - 1;
  int i1  = idx / 27;
  int i2  = idx % 27;
  char c1 = 'A' + i1;
  if (i2 == 0) return String(c1);
  return String(c1) + String((char)('A' + i2 - 1));
}

bool loadGamesIndex()
{
  File f = SD.open(gamesCacheFile.c_str(), FILE_READ);
  if (!f) return false;
  uint32_t nb = 0;
  f.read((uint8_t*)&nb, 4);
  if (nb == 0 || nb > (uint32_t)GAMES_IDX_MAX) { f.close(); return false; }
  gamesIdxCount = 0;
  for (uint32_t i = 0; i < nb && gamesIdxCount < GAMES_IDX_MAX; i++)
  {
    f.read((uint8_t*)gamesIdx[gamesIdxCount].sysName, 32);
    f.read((uint8_t*)&gamesIdx[gamesIdxCount].offset, 4);
    gamesIdxCount++;
  }
  f.close();
  Serial.println("[GCACHE] " + String(gamesIdxCount) + " systemes ("
                 + gamesCacheFile + ")");
  return gamesIdxCount > 0;
}

// Charge la table bigramme du systeme en heap (une seule lecture SD)
bool loadBigramTable(const String &sysName)
{
  if (bigramTableLoaded && bigramTableSys == sysName && bigramTable != nullptr)
    return true;

  uint32_t sysOffset = 0; bool found = false;
  for (int i = 0; i < gamesIdxCount; i++)
    if (sysName == gamesIdx[i].sysName) { sysOffset = gamesIdx[i].offset; found = true; break; }
  if (!found) return false;

  // Allouer si besoin
  if (!bigramTable)
  {
    bigramTable = (uint32_t*)malloc(NB_IDX * 4);
    if (!bigramTable) return false;
  }

  File f = SD.open(gamesCacheFile.c_str(), FILE_READ);
  if (!f) { free(bigramTable); bigramTable = nullptr; return false; }

  f.seek(sysOffset);
  size_t read = f.read((uint8_t*)bigramTable, NB_IDX * 4);
  f.close();

  if (read < (size_t)(NB_IDX * 4))
  {
    free(bigramTable); bigramTable = nullptr; return false;
  }

  bigramTableSys    = sysName;
  bigramTableLoaded = true;
  Serial.println("[GCACHE] table " + sysName + " (" + String(NB_IDX*4) + " bytes)");
  return true;
}

// Charge la tranche du bigramme en heap
// La table doit etre chargee (loadBigramTable)
bool preloadBigram(const String &sysName, const String &gameName)
{
  if (!loadBigramTable(sysName)) return false;

  int    bi  = bigramIndex(gameName);
  String key = sysName + "/" + bigramLabel(bi);
  if (bigramBufKey == key && bigramBuf != nullptr) return true;

  uint32_t bigramOffset = bigramTable[bi];
  if (bigramOffset == 0) return false;

  // Trouver l'offset suivant non nul dans la table (RAM)
  uint32_t nextOffset = 0;
  for (int nbi = bi + 1; nbi < NB_IDX; nbi++)
    if (bigramTable[nbi] != 0) { nextOffset = bigramTable[nbi]; break; }

  if (nextOffset == 0 || nextOffset <= bigramOffset)
  {
    for (int i = 0; i < gamesIdxCount - 1; i++)
      if (sysName == gamesIdx[i].sysName) { nextOffset = gamesIdx[i+1].offset; break; }
    if (nextOffset == 0 || nextOffset <= bigramOffset)
    {
      File f = SD.open(gamesCacheFile.c_str(), FILE_READ);
      if (f) { nextOffset = f.size(); f.close(); }
    }
  }

  size_t sliceSize = (nextOffset > bigramOffset) ? nextOffset - bigramOffset : 0;
  if (sliceSize == 0) return false;

  size_t maxAlloc = ESP.getMaxAllocHeap();
  if (sliceSize > maxAlloc / 2)
  {
    Serial.println("[GCACHE] tranche " + key + " trop grande ("
                   + String(sliceSize) + ") -> SD directe");
    return false;
  }

  freeBigramBuffer();
  bigramBuf = (uint8_t*)malloc(sliceSize);
  if (!bigramBuf) return false;

  File f = SD.open(gamesCacheFile.c_str(), FILE_READ);
  if (!f) { freeBigramBuffer(); return false; }
  f.seek(bigramOffset);
  f.read(bigramBuf, sliceSize);
  f.close();

  bigramBufSize      = sliceSize;
  bigramBufKey       = key;
  bigramBufAbsOffset = bigramOffset;

  Serial.println("[GCACHE] preload " + key + " (" + String(sliceSize)
                 + " bytes) free=" + String(ESP.getFreeHeap()));
  return true;
}

// Si le système est flag 'L' (lent), le cache bigram est inutile :
// les fichiers individuels existent mais on les cherche directement
// via SD.open() dans drawRaw565() / openGif().
// Retourner 'B' force la tentative des deux types.
static inline bool sysIsSlow(const String &sysName)
{
  char f = sysDefaultSlowFlag(sysName);
  return (f == 'L' || f == 'l');
}

char findInGamesCache(const String &sysName, const String &gameName)
{
  if (gamesIdxCount == 0) return '?';

  int    bi          = bigramIndex(gameName);
  String key         = sysName + "/" + bigramLabel(bi);
  String gameNameLow = gameName; gameNameLow.toLowerCase();

  // Recherche RAM
  if (bigramBufKey == key && bigramBuf != nullptr && bigramBufSize > 0)
  {
    uint8_t *ptr = bigramBuf;
    uint8_t *end = bigramBuf + bigramBufSize;
    char bestType = '?'; int bestLen = 0;

    while (ptr < end)
    {
      if (ptr + 1 >= end) break;
      char type = (char)*ptr; ptr++;
      uint8_t *nameStart = ptr;
      while (ptr < end && *ptr != 0) ptr++;
      if (ptr >= end) break;
      int nameLen = ptr - nameStart; ptr++;

      if (nameLen == (int)gameNameLow.length())
      {
        bool match = true;
        for (int i = 0; i < nameLen && match; i++)
          if (tolower((unsigned char)nameStart[i]) != (unsigned char)gameNameLow[i])
            match = false;
        if (match) return type;
      }
      if (nameLen < (int)gameNameLow.length() && nameLen > bestLen)
      {
        bool pfx = true;
        for (int i = 0; i < nameLen && pfx; i++)
          if (tolower((unsigned char)nameStart[i]) != (unsigned char)gameNameLow[i])
            pfx = false;
        if (pfx) { bestLen = nameLen; bestType = type; }
      }
    }
    return bestType;
  }

  // Fallback SD — lit une tranche en bloc puis parse en RAM
  if (!bigramTableLoaded || bigramTableSys != sysName)
    if (!loadBigramTable(sysName)) return '?';

  if (!bigramTable) return '?';
  uint32_t bigramOffset = bigramTable[bi];
  if (bigramOffset == 0) return '?';

  // Trouver la taille de la tranche (jusqu'au prochain bigramme non nul)
  uint32_t nextOffset = 0;
  for (int nbi = bi + 1; nbi < NB_IDX; nbi++)
    if (bigramTable[nbi] != 0) { nextOffset = bigramTable[nbi]; break; }
  if (nextOffset == 0 || nextOffset <= bigramOffset)
  {
    if (gamesIdxCount > 0) { nextOffset = gamesIdx[gamesIdxCount-1].offset; }
    else { nextOffset = bigramOffset + BIGRAM_FALLBACK_SIZE; } // fallback
  }

  uint32_t sliceSize = (nextOffset > bigramOffset) ? nextOffset - bigramOffset : BIGRAM_FALLBACK_SIZE;
  if (sliceSize > BIGRAM_SLICE_MAX) sliceSize = BIGRAM_SLICE_MAX; // sécurité heap

  File f = SD.open(gamesCacheFile.c_str(), FILE_READ);
  if (!f) return '?';
  f.seek(bigramOffset);

  // Lecture en bloc (évite des centaines de petits reads SPI)
  uint8_t *buf = (uint8_t*)malloc(sliceSize);
  if (!buf) { f.close(); return '?'; }
  size_t got = f.read(buf, sliceSize);
  f.close();

  if (got == 0) { free(buf); return '?'; }

  // Parse en RAM
  uint8_t *ptr = buf;
  uint8_t *end = buf + got;
  char bestType = '?'; int bestLen = 0;

  while (ptr < end)
  {
    if (ptr + 1 >= end) break;
    char type = (char)*ptr; ptr++;
    uint8_t *nameStart = ptr;
    while (ptr < end && *ptr != 0) ptr++;
    if (ptr >= end) break;
    int nameLen = ptr - nameStart; ptr++;

    if (nameLen == (int)gameNameLow.length())
    {
      bool match = true;
      for (int i = 0; i < nameLen && match; i++)
        if (tolower((unsigned char)nameStart[i]) != (unsigned char)gameNameLow[i])
          match = false;
      if (match) { free(buf); return type; }
    }
    if (nameLen < (int)gameNameLow.length() && nameLen > bestLen)
    {
      bool pfx = true;
      for (int i = 0; i < nameLen && pfx; i++)
        if (tolower((unsigned char)nameStart[i]) != (unsigned char)gameNameLow[i])
          pfx = false;
      if (pfx) { bestLen = nameLen; bestType = type; }
    }
  }

  free(buf);
  return bestType;
}

// --------------------------------------------------
// Pins & dimensions
// --------------------------------------------------
#define PANEL_RES_X 64
#define PANEL_RES_Y 32
#define PANEL_CHAIN 2

#define CLK_PIN 16
#define OE_PIN  15
#define LAT_PIN  4
#define A_PIN   33
#define B_PIN   32
#define C_PIN   22
#define D_PIN   17
#define E_PIN   -1

#define R1_PIN 25
#define G1_PIN 26
#define B1_PIN 27
#define R2_PIN 14
#define G2_PIN 12
#define B2_PIN 13

#define SD_CS_PIN  5
#define VSPI_MISO 19
#define VSPI_MOSI 23
#define VSPI_SCLK 18

#define MQTT_PORT         1883
#define MQTT_CLIENT  "esp32-marquee"
#define MQTT_RETRY_MS    15000
#define MQTT_START_DELAY_MS 12000

// --------------------------------------------------
// Globaux
// --------------------------------------------------
MatrixPanel_I2S_DMA *display = nullptr;
AnimatedGIF gif;
SPIClass spiSD(VSPI);
File gifFile;
File nextGifFile;
BluetoothSerial SerialBT;

enum DisplayMode { MODE_PLAYLIST, MODE_GIF, MODE_PNG, MODE_BLACK };
volatile DisplayMode currentMode = MODE_PLAYLIST;

bool   gifOpened      = false;
bool   pngDrawn       = false;
String currentPngPath = "";

// ------------------------------
// PNG async (pour systemes "L")
// ------------------------------
static uint16_t *pngAsyncFb = nullptr; // 16-bit RGB565 plein écran (largeur = PANEL_RES_X*PANEL_CHAIN, hauteur = PANEL_RES_Y)
static size_t    pngAsyncFbPixels = 0;

static TaskHandle_t asyncPngTaskHandle = nullptr;
static volatile bool asyncPngInProgress = false;
static volatile bool asyncPngReady = false;
static volatile bool asyncPngCancel = false;
static uint32_t asyncPngRequestId = 0;
static uint32_t asyncPngActiveRequestId = 0;
static String asyncPngPath = "";
static volatile bool currentPngAsyncWanted = false;
unsigned long asyncPngStartMs = 0;
String playlistName       = "";
String playlistSourcePath = "";
String playlistCachePath  = "";
String playlistSigPath    = "";
String playlistIdxPath    = "";
bool   playlistRandom     = true;
String imageFolder        = "";  // Vide : images directement dans systems/<sys>/

int gifCount        = 0;
int playIndex       = 0;
int lastRandomIndex = -1;

File seqPlaylistFile;
File idxFileHandle;

bool   requestNextGif = false;
bool   requestReboot  = false;
String nextGifPath    = "";

bool   wifiEnabled               = false;
String wifiSSID                  = "";
String wifiPassword              = "";
bool   wifiStaticEnabled         = false;
String wifiStaticIP              = "";
String wifiGateway               = "";
String wifiSubnet                = "";
String wifiDNS1                  = "";
String wifiDNS2                  = "";
unsigned long lastWifiReconnectAttempt = 0;

bool   bluetoothEnabled = false;
String bluetoothName    = "ESP32-GIF";
bool   showInfo         = true;

String recalboxIP     = "";
String mqttEventTopic = "marquee/event";
const unsigned long MQTT_OFFLINE_FALLBACK_MS = 60000;

WiFiClient   wifiClientMqtt;
PubSubClient mqttClient(wifiClientMqtt);
String       lastSysName = "";
String       displayedMaskSysName = "";

struct MqttCommand
{
  enum Type { CMD_NONE, CMD_STOP, CMD_DEFAULT, CMD_SYSTEM, CMD_GAME,
              CMD_STARTCLIP, CMD_RESUMESYS };
  Type   type;
  String arg;
  MqttCommand() : type(CMD_NONE), arg("") {}
  MqttCommand(Type t, const String &a) : type(t), arg(a) {}
};

SemaphoreHandle_t mqttCmdMutex   = nullptr;
MqttCommand       pendingCmd;
TaskHandle_t      mqttTaskHandle = nullptr;

#define MQTT_LOG_SIZE 10
struct MqttLogEntry { String topic; String msg; unsigned long ts; };
MqttLogEntry mqttLog[MQTT_LOG_SIZE];
int mqttLogHead  = 0;
int mqttLogCount = 0;

void mqttLogAdd(const String &topic, const String &msg)
{
  mqttLog[mqttLogHead] = { topic, msg, millis() };
  mqttLogHead = (mqttLogHead + 1) % MQTT_LOG_SIZE;
  if (mqttLogCount < MQTT_LOG_SIZE) mqttLogCount++;
}

// --------------------------------------------------
// Helpers
// --------------------------------------------------
String getPlaylistLabel()
{
  String label = playlistName;
  int slash = label.lastIndexOf('/'); if (slash >= 0) label = label.substring(slash + 1);
  int dot   = label.lastIndexOf('.'); if (dot > 0)   label = label.substring(0, dot);
  label.trim();
  if (label.length() == 0) label = "UNKNOWN";
  return label;
}

String fitLabel(String s, int maxChars)
{
  s.trim();
  if ((int)s.length() <= maxChars) return s;
  if (maxChars <= 3) return s.substring(0, maxChars);
  return s.substring(0, maxChars - 3) + "...";
}

String extractField(const String &msg, const String &key)
{
  int idx = msg.indexOf(key + "="); if (idx < 0) return "";
  int start = idx + key.length() + 1;
  int end   = msg.indexOf(' ', start); if (end < 0) end = msg.length();
  return msg.substring(start, end);
}

// --------------------------------------------------
// Affichage
// --------------------------------------------------
void showMessage(const String &line1, const String &line2, uint16_t color = 0xFFE0)
{
  display->clearScreen(); display->setTextWrap(false); display->setTextSize(1);
  display->setTextColor(color);
  display->setCursor(1, 6);  display->print(line1);
  display->setCursor(1, 18); display->print(line2);
}

void showPlaylistInfoScreen()
{
  display->clearScreen(); display->setTextWrap(false); display->setTextSize(1);
  display->setTextColor(display->color565(235, 235, 235));
  display->setCursor(1, 5);  display->print(fitLabel(getPlaylistLabel(), 10));
  display->setTextColor(display->color565(255, 210, 70));
  display->setCursor(1, 18); display->print(String(gifCount) + " GIFS");
}

void drawWifiIconSmall(int x, int y, uint16_t color)
{
  display->drawPixel(x+4,y+8,color); display->drawLine(x+2,y+6,x+6,y+6,color);
  display->drawPixel(x+1,y+4,color); display->drawPixel(x+7,y+4,color);
  display->drawLine(x+2,y+3,x+6,y+3,color);
  display->drawPixel(x+0,y+1,color); display->drawPixel(x+8,y+1,color);
  display->drawLine(x+1,y+0,x+7,y+0,color);
}

void drawBluetoothIconSmall(int x, int y, uint16_t color)
{
  display->drawLine(x+4,y+0,x+4,y+8,color);
  display->drawLine(x+4,y+4,x+7,y+1,color); display->drawLine(x+4,y+0,x+7,y+3,color);
  display->drawLine(x+4,y+4,x+7,y+7,color); display->drawLine(x+4,y+8,x+7,y+5,color);
  display->drawLine(x+1,y+2,x+4,y+4,color); display->drawLine(x+1,y+6,x+4,y+4,color);
}

void showWifiStatusScreen(const String &line1, const String &line2, uint16_t color)
{
  display->clearScreen(); display->setTextWrap(false); display->setTextSize(1);
  drawWifiIconSmall(3, 10, color); display->setTextColor(color);
  display->setCursor(18, 6);  display->print(line1);
  display->setCursor(18, 18); display->print(line2);
}

void showBluetoothStatusScreen(bool enabled)
{
  uint16_t color = enabled ? display->color565(80,170,255) : display->color565(255,0,0);
  display->clearScreen(); display->setTextWrap(false); display->setTextSize(1);
  drawBluetoothIconSmall(3,10,color); display->setTextColor(color);
  display->setCursor(18, 6);  display->print("BT");
  display->setCursor(18, 18); display->print(enabled ? "ON" : "OFF");
}

void drawHourglassTallFancy(int x, int y, int w, int h, uint8_t phase)
{
  uint16_t borderOuter=display->color565(60,90,130);
  uint16_t borderInner=display->color565(170,220,255);
  uint16_t capColor   =display->color565(110,150,200);
  uint16_t sandColor  =display->color565(255,210,70);
  uint16_t sandGlow   =display->color565(255,235,140);
  uint16_t shadowColor=display->color565(25,30,40);

  int cx=x+w/2, topY=y, botY=y+h-1, neckY=y+h/2;
  display->drawRect(x,y,w,h,borderOuter);
  display->drawRect(x+1,y+1,w-2,h-2,shadowColor);
  display->drawLine(x+3,topY+3,x+w-4,topY+3,capColor);
  display->drawLine(x+3,botY-3,x+w-4,botY-3,capColor);
  display->drawLine(x+4,topY+4,cx,neckY-1,borderInner);
  display->drawLine(x+w-5,topY+4,cx,neckY-1,borderInner);
  display->drawLine(cx,neckY+1,x+4,botY-4,borderInner);
  display->drawLine(cx,neckY+1,x+w-5,botY-4,borderInner);

  int topFill,bottomFill;
  switch(phase&7){
    case 0:topFill=10;bottomFill=2;break; case 1:topFill=9;bottomFill=3;break;
    case 2:topFill=8;bottomFill=4;break;  case 3:topFill=7;bottomFill=5;break;
    case 4:topFill=6;bottomFill=6;break;  case 5:topFill=5;bottomFill=7;break;
    case 6:topFill=4;bottomFill=8;break;  default:topFill=3;bottomFill=9;break;
  }
  int topBaseY=max(neckY-topFill,topY+5);
  display->fillTriangle(x+6,topBaseY,x+w-7,topBaseY,cx,neckY-2,sandColor);
  display->drawLine(x+7,topBaseY+1,x+w-8,topBaseY+1,sandGlow);
  int bottomApexY=min(neckY+bottomFill,botY-5);
  display->fillTriangle(x+6,botY-5,x+w-7,botY-5,cx,bottomApexY,sandColor);
  display->drawLine(x+7,botY-6,x+w-8,botY-6,sandGlow);

  uint8_t sm=phase&7;
  if(sm==0||sm==4){display->drawPixel(cx,neckY-1,sandGlow);display->drawPixel(cx,neckY,sandColor);display->drawPixel(cx,neckY+1,sandColor);display->drawPixel(cx,neckY+2,sandGlow);}
  else if(sm==1||sm==5){display->drawPixel(cx,neckY-1,sandGlow);display->drawPixel(cx,neckY,sandColor);display->drawPixel(cx,neckY+1,sandGlow);}
  else if(sm==2||sm==6){display->drawPixel(cx,neckY,sandColor);display->drawPixel(cx,neckY+1,sandGlow);}
  else{display->drawPixel(cx,neckY-1,sandColor);display->drawPixel(cx,neckY,sandGlow);display->drawPixel(cx,neckY+1,sandColor);}

  if((phase&1)==0) display->drawPixel(cx-1,bottomApexY+1,sandGlow);
  else             display->drawPixel(cx+1,bottomApexY+1,sandGlow);
}

void showLoadingHourglass(int count)
{
  static uint8_t frame=0; frame++;
  display->clearScreen(); display->setTextWrap(false); display->setTextSize(1);
  display->setTextColor(display->color565(235,235,235));
  display->setCursor(2,3);  display->print("GIFS");
  display->setTextColor(display->color565(255,210,70));
  display->setCursor(2,13); display->print(count);
  display->setTextColor(display->color565(150,200,255));
  display->setCursor(2,24); display->print(fitLabel(getPlaylistLabel(),7));
  drawHourglassTallFancy(44,1,18,30,frame);
}

// --------------------------------------------------
// Bluetooth
// --------------------------------------------------
void setupBluetoothFromConfig()
{
  if (showInfo) showBluetoothStatusScreen(bluetoothEnabled);
  delay(1200);
  if (!bluetoothEnabled) { btStop(); esp_bt_mem_release(ESP_BT_MODE_BTDM); return; }
  SerialBT.begin(bluetoothName);
}

// --------------------------------------------------
// PNG — libere le cache bigramme avant de decoder
// pour donner la RAM a pngle
// --------------------------------------------------
void pngleDrawCallback(pngle_t *pngle, uint32_t x, uint32_t y,
                       uint32_t w, uint32_t h, const uint8_t rgba[4])
{
  (void)pngle; (void)w; (void)h;
  if ((int)x >= (PANEL_RES_X * PANEL_CHAIN) || (int)y >= PANEL_RES_Y) return;
  display->drawPixel((int)x, (int)y, display->color565(rgba[0], rgba[1], rgba[2]));
}

static void logHeapCaps(const char *where)
{
  // largest free block in bytes (avoids confusion with total freeHeap)
  size_t largest8bit   = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
  size_t largestInt    = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
  size_t largestSpiRam = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);

  Serial.println(String("[HEAP] ") + where +
                 " freeHeap=" + String(ESP.getFreeHeap()) +
                 " maxAlloc=" + String(ESP.getMaxAllocHeap()) +
                 " largest8bit=" + String((uint32_t)largest8bit) +
                 " largestInternal=" + String((uint32_t)largestInt) +
                 " psramFound=" + String(psramFound() ? "1" : "0") +
                 " freePsram=" + String(ESP.getFreePsram()) +
                 " largestSpiram=" + String((uint32_t)largestSpiRam));
}

static const int RAW565_W = PANEL_RES_X * PANEL_CHAIN; // 128
static const int RAW565_H = PANEL_RES_Y;               // 32

static String pngToRaw565Path(const String &pngPath)
{
  if (pngPath.length() >= 4 && pngPath.endsWith(".png"))
    return pngPath.substring(0, pngPath.length() - 4) + ".raw565";
  return pngPath + ".raw565";
}

static uint16_t *raw565FullBuf = nullptr;

// Cache RAM du fallback /systems/_defaults/default.raw565 (8KB)
static uint16_t *defaultRaw565Buf = nullptr;
static bool defaultRaw565Cached = false;
static const char *DEFAULT_RAW565_PATH = "/systems/_defaults/default.raw565";

static bool ensureDefaultRaw565Cached()
{
  if (defaultRaw565Cached) return true;

  const size_t totalBytes = (size_t)RAW565_W * (size_t)RAW565_H * sizeof(uint16_t);
  if (!defaultRaw565Buf)
  {
    defaultRaw565Buf = (uint16_t*)malloc(totalBytes);
    if (!defaultRaw565Buf) return false;
  }

  File f = SD.open(DEFAULT_RAW565_PATH, FILE_READ);
  if (!f) return false;

  size_t gotAll = f.read((uint8_t*)defaultRaw565Buf, totalBytes);
  f.close();

  if (gotAll != totalBytes) return false;

  defaultRaw565Cached = true;
  return true;
}

static bool drawDefaultRaw565Cached()
{
  if (!ensureDefaultRaw565Cached()) return false;

  for (int y = 0; y < RAW565_H; y++)
    display->drawRGBBitmap(0, y, defaultRaw565Buf + (size_t)y * RAW565_W, RAW565_W, 1);

  return true;
}

static bool drawRaw565(const String &rawPath)
{
  File f = SD.open(rawPath.c_str(), FILE_READ);
  if (!f) return false;

  const size_t rowBytes   = (size_t)RAW565_W * sizeof(uint16_t);
  const size_t totalBytes = rowBytes * (size_t)RAW565_H;

  // 1 seul gros read au lieu de 32 reads: évite le jitter SD.
  if (!raw565FullBuf)
  {
    raw565FullBuf = (uint16_t*)malloc(totalBytes);
    if (!raw565FullBuf)
    {
      // fallback: ancien comportement (lecture ligne par ligne)
      uint16_t row[RAW565_W];
      for (int y = 0; y < RAW565_H; y++)
      {
        size_t got = f.read((uint8_t*)row, rowBytes);
        if (got != rowBytes)
        {
          f.close();
          return false;
        }
        display->drawRGBBitmap(0, y, row, RAW565_W, 1);
      }
      f.close();
      return true;
    }
  }

  size_t gotAll = f.read((uint8_t*)raw565FullBuf, totalBytes);
  if (gotAll != totalBytes)
  {
    f.close();
    return false;
  }

  for (int y = 0; y < RAW565_H; y++)
    display->drawRGBBitmap(0, y, raw565FullBuf + (size_t)y * RAW565_W, RAW565_W, 1);

  f.close();
  return true;
}

bool drawPng(const String &path)
{
  if (nextGifFile)   { nextGifFile.close();   nextGifFile   = File(); }
  if (idxFileHandle) { idxFileHandle.close();  idxFileHandle = File(); }

  // 1) Tentative rapide: afficher *.raw565 si présent
  String raw565Path = pngToRaw565Path(path);
  Serial.println("[PNG-RAW] drawRaw565 try raw565=" + raw565Path + " t=" + String(millis()));
  bool rawDrawn = drawRaw565(raw565Path);
  Serial.println(String("[PNG-RAW] drawRaw565 done raw565=") + raw565Path + " ok=" + (rawDrawn ? "1" : "0") + " t=" + String(millis()));

  if (rawDrawn)
  {
    Serial.println("[PNG-RAW] OK path=" + path + " raw565=" + raw565Path + " t=" + String(millis()));
    return true;
  }

  Serial.println("[PNG-RAW] MISSING raw565 path=" + path + " raw565=" + raw565Path);

  // 2) Déterminer si le système est "L" (lenteur) ou "N" (rapide)
  auto extractSysNameFromSystemsPath=[&](const String &p)->String{
    if(!p.startsWith("/systems/")) return "";
    if(p.startsWith("/systems/_defaults/")) return "";
    int s0 = String("/systems/").length();
    int slash = p.indexOf('/', s0);
    if(slash < 0) return "";
    return p.substring(s0, slash);
  };

  String sysName = extractSysNameFromSystemsPath(path);
  char slowFlag = sysDefaultSlowFlag(sysName);
  bool isSlow = (slowFlag == 'L' || slowFlag == 'l');

  Serial.println("[PNG-RAW] missing raw -> sysName=" + sysName + " slowFlag=" + String(slowFlag) + " isSlow=" + String(isSlow));

  // 3) fallback "toujours réactif" si système lent: on n'essaie pas de décoder PNG
  String defPng = "/systems/_defaults/default.png";
  String defRaw565Path = pngToRaw565Path(defPng);

  if (isSlow)
  {
    Serial.println("[PNG-RAW] slow system -> fallback default raw drawRaw565 start defRaw565=" + defRaw565Path + " t=" + String(millis()));
    if(drawDefaultRaw565Cached())
    {
      Serial.println("[PNG-RAW] FALLBACK " + defRaw565Path + " (slow skip png decode) for missing raw565 path=" + path + " t=" + String(millis()));
      return true;
    }
    Serial.println("[PNG-RAW] fallback default raw drawRaw565 failed defRaw565=" + defRaw565Path + " t=" + String(millis()));
    return false;
  }

  // 4) Système rapide: on tente de décoder le PNG (si SD.open échoue -> fallback default raw)
  freeBigramAll();

  if (nextGifFile)   { nextGifFile.close();   nextGifFile   = File(); }
  if (idxFileHandle) { idxFileHandle.close();  idxFileHandle = File(); }

  Serial.println("[PNG-RAW] fast system -> try png decode start path=" + path + " t=" + String(millis()));
  File f = SD.open(path.c_str(), FILE_READ);
  if (!f)
  {
    Serial.println("[PNG-RAW] fast system -> SD.open png failed, fallback default raw start defRaw565=" + defRaw565Path + " t=" + String(millis()));
    if(drawDefaultRaw565Cached())
    {
      Serial.println("[PNG-RAW] FALLBACK " + defRaw565Path + " (SD.open png failed) for path=" + path + " t=" + String(millis()));
      return true;
    }
    Serial.println("[PNG-RAW] fallback default raw drawRaw565 failed defRaw565=" + defRaw565Path + " t=" + String(millis()));
    return false;
  }

  pngle_t *pngle = pngle_new();
  if (!pngle)
  {
    f.close();
    Serial.println("[PNG-RAW] fast system -> pngle_new failed, fallback default raw start defRaw565=" + defRaw565Path + " t=" + String(millis()));
    if(drawDefaultRaw565Cached())
    {
      Serial.println("[PNG-RAW] FALLBACK " + defRaw565Path + " (pngle_new failed) for path=" + path + " t=" + String(millis()));
      return true;
    }
    Serial.println("[PNG-RAW] fallback default raw drawRaw565 failed defRaw565=" + defRaw565Path + " t=" + String(millis()));
    return false;
  }

  pngle_set_draw_callback(pngle, pngleDrawCallback);

  uint8_t buf[256];
  bool ok = true;
  while (f.available())
  {
    int len = f.read(buf, sizeof(buf));
    if (len <= 0) break;
    if (pngle_feed(pngle, buf, len) < 0) { ok = false; break; }
  }

  pngle_destroy(pngle);
  f.close();

  if (!ok)
  {
    Serial.println("[PNG-RAW] fast system -> png decode failed, fallback default raw start defRaw565=" + defRaw565Path + " t=" + String(millis()));
    if(drawDefaultRaw565Cached())
    {
      Serial.println("[PNG-RAW] FALLBACK " + defRaw565Path + " (png decode failed) for path=" + path + " t=" + String(millis()));
      return true;
    }
    Serial.println("[PNG-RAW] fallback default raw drawRaw565 failed defRaw565=" + defRaw565Path + " t=" + String(millis()));
    return false;
  }

  Serial.println("[PNG-RAW] fast system -> png decode OK path=" + path + " t=" + String(millis()));
  return true;
}

// --------------------------------------------------
// PNG async (systemes "L") -> decode en tâche vers buffer RGB565
// puis blit depuis loop()
// --------------------------------------------------
static const int PNG_ASYNC_FB_W = 64 * 2; // PANEL_RES_X * PANEL_CHAIN = 128
static const int PNG_ASYNC_FB_H = 32;     // PANEL_RES_Y = 32
static const int PNG_ASYNC_FB_PIXELS = PNG_ASYNC_FB_W * PNG_ASYNC_FB_H;

static inline void pngleAsyncDrawCallback(pngle_t *p, uint32_t x, uint32_t y,
                                           uint32_t w, uint32_t h, const uint8_t rgba[4])
{
  (void)p; (void)w; (void)h;
  if (!pngAsyncFb) return;
  if (x >= (uint32_t)PNG_ASYNC_FB_W || y >= (uint32_t)PNG_ASYNC_FB_H) return;
  // RGB565
  uint8_t r = rgba[0], g = rgba[1], b = rgba[2];
  uint16_t rgb565 = (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
  pngAsyncFb[y * PNG_ASYNC_FB_W + x] = rgb565;
}

static void blitPngAsyncFbToDisplay()
{
  if (!pngAsyncFb) return;
  // blit ligne par ligne (évite grosse allocation temporaire)
  for (int y = 0; y < PNG_ASYNC_FB_H; y++)
  {
    display->drawRGBBitmap(0, y, pngAsyncFb + (size_t)y * PNG_ASYNC_FB_W, PNG_ASYNC_FB_W, 1);
  }
}

static void pngAsyncDecodeTask(void *param)
{
  (void)param;

  uint32_t reqId = asyncPngActiveRequestId;
  String pathLocal = asyncPngPath;

  // reset
  asyncPngReady = false;
  asyncPngInProgress = true;

  Serial.println("[PNG-ASYNC] start reqId=" + String(reqId) + " path=" + pathLocal);
  Serial.println("[PNG-ASYNC] stackHighWater=" + String(uxTaskGetStackHighWaterMark(nullptr))
                 + " freeHeap=" + String(ESP.getFreeHeap()));

  // init buffer
  if (!pngAsyncFb)
  {
    pngAsyncFbPixels = PNG_ASYNC_FB_PIXELS;
    pngAsyncFb = (uint16_t*)malloc(pngAsyncFbPixels * sizeof(uint16_t));
  }

  if (!pngAsyncFb)
  {
    Serial.println("[PNG-ASYNC] malloc pngAsyncFb failed reqId=" + String(reqId)
                   + " freeHeap=" + String(ESP.getFreeHeap()));
    asyncPngInProgress = false;
    asyncPngReady = false;
    vTaskDelete(nullptr);
    return;
  }

  // Décodage
  File f = SD.open(pathLocal);
  if (!f)
  {
    Serial.println("[PNG-ASYNC] SD.open failed reqId=" + String(reqId) + " path=" + pathLocal);
    asyncPngInProgress = false;
    asyncPngReady = false;
    vTaskDelete(nullptr);
    return;
  }

  // IMPORTANT: libérer la RAM AVANT de créer pngle (comme drawPng)
  // drawPng ferme aussi les fichiers pour maximiser le heap.
  freeBigramAll();

  if (nextGifFile)   { nextGifFile.close();   nextGifFile = File(); }
  if (idxFileHandle) { idxFileHandle.close();  idxFileHandle = File(); }

  Serial.println("[PNG-ASYNC] before pngle_new freeHeap=" + String(ESP.getFreeHeap()));
  Serial.println("[PNG-ASYYNC] largest8bit=" + String((uint32_t)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)));
  pngle_t *pngle = pngle_new();
  if (!pngle)
  {
    Serial.println("[PNG-ASYNC] pngle_new failed reqId=" + String(reqId)
                   + " freeHeap=" + String(ESP.getFreeHeap())
                   + " largest8bit=" + String((uint32_t)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)));
    f.close();
    asyncPngInProgress = false;
    asyncPngReady = false;
    vTaskDelete(nullptr);
    return;
  }
  pngle_set_draw_callback(pngle, pngleAsyncDrawCallback);

  uint8_t buf[256];
  while (f.available())
  {
    if (asyncPngCancel || asyncPngActiveRequestId != reqId) break;
    int len = f.read(buf, sizeof(buf));
    if (len <= 0) break;
    if (pngle_feed(pngle, buf, len) < 0) break;
  }

  pngle_destroy(pngle);
  f.close();

  if (!asyncPngCancel && asyncPngActiveRequestId == reqId)
  {
    asyncPngReady = true;
    Serial.println("[PNG-ASYNC] ready reqId=" + String(reqId));
  }
  else
  {
    Serial.println("[PNG-ASYNC] not ready (cancel=" + String(asyncPngCancel ? "1" : "0")
                   + ", activeId=" + String(asyncPngActiveRequestId) + " reqId=" + String(reqId) + ")");
  }

  asyncPngInProgress = false;
  vTaskDelete(nullptr);
}

static void startAsyncPngDecodeIfNeeded(const String &path)
{
  // si on veut un PNG asynchrone mais déjà lancé pour le même path, on ne relance pas
  // (on utilise requestId pour simple tracking)
  if (asyncPngInProgress && asyncPngPath == path && asyncPngReady == false) return;

  // Annule l’éventuelle tâche précédente
  asyncPngCancel = true;
  delay(1);
  asyncPngCancel = false;

  asyncPngRequestId++;
  asyncPngActiveRequestId = asyncPngRequestId;
  asyncPngPath = path;
  asyncPngStartMs = millis();

  // IMPORTANT: éviter la fenêtre où loop() relance des tâches avant que la FreeRTOS task
  // ne passe à son premier instruction. On marque "in progress" dès maintenant.
  asyncPngReady = false;
  asyncPngInProgress = true;

  Serial.println("[PNG-ASYNC] scheduled reqId=" + String(asyncPngActiveRequestId)
                 + " inProgress=1 path=" + path);

  if (asyncPngTaskHandle) { asyncPngTaskHandle = nullptr; } // tâche gérée par vTaskDelete

  xTaskCreatePinnedToCore(pngAsyncDecodeTask, "pngAsyncDecode", 16384, nullptr, 1, &asyncPngTaskHandle, 1);
}

// --------------------------------------------------
// GIF callbacks
// --------------------------------------------------
void GIFDraw(GIFDRAW *pDraw)
{
  if (!display) return;
  uint8_t *s = pDraw->pPixels;
  int iWidth = pDraw->iWidth;
  if (iWidth > (PANEL_RES_X * PANEL_CHAIN)) iWidth = PANEL_RES_X * PANEL_CHAIN;
  int yOffset = (PANEL_RES_Y - pDraw->iHeight) / 2;
  int y = pDraw->iY + pDraw->y + yOffset;
  if (y < 0 || y >= PANEL_RES_Y) return;
  int xOffset = ((PANEL_RES_X * PANEL_CHAIN) - pDraw->iWidth) / 2;
  if (xOffset < 0) xOffset = 0;
  uint16_t usTemp[PANEL_RES_X * PANEL_CHAIN];
  for (int x = 0; x < iWidth; x++)
  {
    uint8_t idx = s[x];
    usTemp[x] = (idx == pDraw->ucTransparent && pDraw->ucHasTransparency)
                ? 0 : pDraw->pPalette[idx];
  }
  display->drawRGBBitmap(xOffset, y, usTemp, iWidth, 1);
}

void *GIFOpenFile(const char *fname, int32_t *pSize)
{
  if (nextGifFile && String(fname) == nextGifPath)
  { nextGifFile.seek(0); gifFile = nextGifFile; nextGifFile = File(); }
  else gifFile = SD.open(fname);
  if (!gifFile) return nullptr;
  *pSize = gifFile.size();
  return (void *)&gifFile;
}

void GIFCloseFile(void *pHandle) { File *f=(File*)pHandle; if(f) f->close(); }

int32_t GIFReadFile(GIFFILE *pFile, uint8_t *pBuf, int32_t len)
{
  File *f=(File*)pFile->fHandle; if(!f) return 0;
  int32_t toRead=len;
  if((pFile->iSize-pFile->iPos)<len) toRead=pFile->iSize-pFile->iPos;
  if(toRead<=0) return 0;
  int32_t n=f->read(pBuf,toRead); pFile->iPos=f->position(); return n;
}

int32_t GIFSeekFile(GIFFILE *pFile, int32_t position)
{
  File *f=(File*)pFile->fHandle; if(!f) return 0;
  f->seek(position); pFile->iPos=f->position(); return pFile->iPos;
}

static bool gifRawPackMode = false;
static uint32_t gifRawFrameIndex = 0;
static uint32_t gifRawFrameCount = 0;
static String gifRawPackPathCur = "";
static String gifRawMetaPathCur = "";

// Speed control for raw565pack playback:
// - 100 = normal speed
// - 50 = twice as fast
// - 25 = four times as fast
static const uint16_t GIF_RAW_PACK_SPEED_PERCENT = 50;
static const uint16_t GIF_RAW_PACK_MIN_DELAY_MS = 5;

static File gifRawPackFile;
static File gifRawMetaFile;
static const uint32_t RAW565_GIF_W = PANEL_RES_X * PANEL_CHAIN; // 128
static const uint32_t RAW565_GIF_H = PANEL_RES_Y;              // 32
static const uint32_t RAW565_GIF_FRAME_BYTES = RAW565_GIF_W * RAW565_GIF_H * 2;

static String gifToRaw565PackPath(const String &gifPath)
{
  if (gifPath.length() >= 4 && gifPath.endsWith(".gif"))
    return gifPath.substring(0, gifPath.length() - 4) + ".raw565pack";
  return gifPath + ".raw565pack";
}

static String gifToRaw565MetaPath(const String &gifPath)
{
  if (gifPath.length() >= 4 && gifPath.endsWith(".gif"))
    return gifPath.substring(0, gifPath.length() - 4) + ".meta";
  return gifPath + ".meta";
}

// Buffer cache pour tous les delays du meta file (lus en une fois a l'ouverture)
static uint16_t *gifRawDelayCache = nullptr;
static uint32_t  gifRawDelayCount = 0;

static void closeGifRawPackIfAny()
{
  if (gifRawPackFile) { gifRawPackFile.close(); }
  if (gifRawMetaFile) { gifRawMetaFile.close(); }
  if (gifRawDelayCache) { free(gifRawDelayCache); gifRawDelayCache = nullptr; }
  gifRawDelayCount = 0;
  gifRawPackFile = File();
  gifRawMetaFile = File();

  gifRawPackPathCur = "";
  gifRawMetaPathCur = "";
  gifRawFrameIndex = 0;
  gifRawFrameCount = 0;
  gifRawPackMode = false;
}

// Charge tous les delays du fichier meta en RAM a l'ouverture d'un raw565pack
static bool gifRawLoadMetaCache()
{
  if (gifRawDelayCache) free(gifRawDelayCache);
  gifRawDelayCache = nullptr;
  gifRawDelayCount = 0;
  if (!gifRawMetaFile) return false;
  size_t metaSize = gifRawMetaFile.size();
  if (metaSize < 2) return false;
  uint16_t count = metaSize / 2;
  gifRawDelayCache = (uint16_t*)malloc(metaSize);
  if (!gifRawDelayCache) return false;
  gifRawMetaFile.seek(0);
  size_t got = gifRawMetaFile.read((uint8_t*)gifRawDelayCache, metaSize);
  if (got < 2) { free(gifRawDelayCache); gifRawDelayCache = nullptr; return false; }
  gifRawDelayCount = count;
  return true;
}

static uint16_t gifRawReadDelayMs(uint32_t frameIndex)
{
  // Utiliser le cache RAM si disponible (plus de seek+read sur SD)
  if (gifRawDelayCache && frameIndex < gifRawDelayCount)
  {
    uint16_t ms = gifRawDelayCache[frameIndex];
    if (ms == 0) ms = GIF_RAW_PACK_MIN_DELAY_MS;
    ms = (uint16_t)((uint32_t)ms * GIF_RAW_PACK_SPEED_PERCENT / 100U);
    if (ms < GIF_RAW_PACK_MIN_DELAY_MS) ms = GIF_RAW_PACK_MIN_DELAY_MS;
    return ms;
  }
  // Fallback: lecture directe depuis le fichier meta
  if (!gifRawMetaFile) return 33;
  uint32_t metaPos = frameIndex * 2UL;
  gifRawMetaFile.seek(metaPos);
  uint16_t ms = 33;
  size_t got = gifRawMetaFile.read((uint8_t*)&ms, 2);
  if (got != 2) ms = 33;
  if (ms == 0) ms = GIF_RAW_PACK_MIN_DELAY_MS;
  ms = (uint16_t)((uint32_t)ms * GIF_RAW_PACK_SPEED_PERCENT / 100U);
  if (ms < GIF_RAW_PACK_MIN_DELAY_MS) ms = GIF_RAW_PACK_MIN_DELAY_MS;
  return ms;
}
// Buffer reusable pour la lecture bulk d'une frame raw565pack (8192 bytes)
// Partagé avec drawRaw565() via raw565FullBuf
static uint16_t *gifRawFrameBuf = nullptr;

static void drawGifRaw565Frame(uint32_t frameIndex)
{
  if (!gifRawPackFile) return;

  const size_t frameBytes = RAW565_GIF_FRAME_BYTES; // 128*32*2 = 8192

  uint32_t baseOff = frameIndex * frameBytes;
  gifRawPackFile.seek(baseOff);

  // Utiliser raw565FullBuf s'il est déjà alloué (drawRaw565 l'alloue si besoin)
  if (!gifRawFrameBuf)
  {
    // Essayer raw565FullBuf d'abord (partagé avec drawRaw565)
    if (raw565FullBuf)
    {
      gifRawFrameBuf = raw565FullBuf;
    }
    else
    {
      gifRawFrameBuf = (uint16_t*)malloc(frameBytes);
      if (!gifRawFrameBuf)
      {
        // Fallback: ancien comportement (lecture ligne par ligne)
        uint16_t row[RAW565_GIF_W];
        for (uint32_t y = 0; y < RAW565_GIF_H; y++)
        {
          size_t need = RAW565_GIF_W * 2UL;
          size_t got = gifRawPackFile.read((uint8_t*)row, need);
          if (got != need) break;
          display->drawRGBBitmap(0, (int)y, row, RAW565_GIF_W, 1);
        }
        return;
      }
    }
  }

  // 1 seul read bulk de toute la frame
  size_t gotAll = gifRawPackFile.read((uint8_t*)gifRawFrameBuf, frameBytes);
  if (gotAll != frameBytes) return;

  // Blit depuis RAM
  for (uint32_t y = 0; y < RAW565_GIF_H; y++)
    display->drawRGBBitmap(0, (int)y, gifRawFrameBuf + (size_t)y * RAW565_GIF_W, RAW565_GIF_W, 1);
}
static bool gifPlayFrameCompat(bool first, int *pDelayMs)
{
  if (gifRawPackMode)
  {
    if (first) gifRawFrameIndex = 0;
    if (gifRawFrameIndex >= gifRawFrameCount) return false;

    uint16_t ms = gifRawReadDelayMs(gifRawFrameIndex);
    *pDelayMs = (int)ms;

    drawGifRaw565Frame(gifRawFrameIndex);
    gifRawFrameIndex++;
    return true;
  }
  return gif.playFrame(first, pDelayMs);
}

static void gifResetCompat()
{
  if (gifRawPackMode)
  {
    gifRawFrameIndex = 0;
  }
  else
  {
    gif.reset();
  }
}

bool openGif(const String &path, bool clearBefore=true, bool skipProbe=false)
{
  if (!skipProbe)
  {
    File p = SD.open(path.c_str(), FILE_READ);
    if (!p) return false;
    p.close();
  }

  // On ferme l'état raw si on en avait un.
  closeGifRawPackIfAny();
  gifRawPackMode = false;
  gifOpened = false;

  // RULE:
  // - Si raw565pack + meta existent => on ouvre en raw565pack
  // - Sinon => fallback sur GIF standard (.gif)
  // - Cas spécifique masks _defaults: raw-only strict (pas de fallback GIF standard)
  bool isDefaults = (path.indexOf("/systems/_defaults/") >= 0);

  // -------- Raw565pack (si disponible) --------
  String rawPack = gifToRaw565PackPath(path);
  String metaPath = gifToRaw565MetaPath(path);

  {
    // Evite un pattern "probe" SD.exists()+SD.open() : on ouvre directement.
    if (clearBefore) display->clearScreen();

      gifRawPackFile = SD.open(rawPack.c_str(), FILE_READ);
      gifRawMetaFile = SD.open(metaPath.c_str(), FILE_READ);

      if (gifRawPackFile && gifRawMetaFile)
      {
        size_t packSize = gifRawPackFile.size();

        gif.close();
        gifOpened = true;
        gifRawPackMode = true;
        gifRawFrameIndex = 0;

        Serial.println("[GIF] open OK raw565pack req=" + path
                     + " rawPack=" + rawPack
                     + " meta=" + metaPath);

        gifRawPackPathCur = rawPack;
        gifRawMetaPathCur = metaPath;

        // Charger tous les delays en RAM pour eviter seek+read par frame
        gifRawLoadMetaCache();
        if (gifRawDelayCache) Serial.println("[GIF] meta delays en RAM: " + String(gifRawDelayCount) + " frames");

        gifRawFrameCount = (RAW565_GIF_FRAME_BYTES > 0) ? (packSize / RAW565_GIF_FRAME_BYTES) : 0;
        if (gifRawFrameCount == 0)
        {
          closeGifRawPackIfAny();
          gifOpened = false;
        }
        return gifOpened;
      }

      closeGifRawPackIfAny();
  }

  // raw-only strict: si ce sont des masks _defaults, on refuse sans fallback
  if (isDefaults)
  {
    gifRawPackMode = false;
    gif.close();
    gifOpened = false;
    return false;
  }

  // -------- Playlist => GIF standard --------
  if (clearBefore) display->clearScreen();
  gif.close();

  // AnimatedGIF::open signature:
  //   int open(const char *szFilename,
  //            GIF_OPEN_CALLBACK*,
  //            GIF_CLOSE_CALLBACK*,
  //            GIF_READ_CALLBACK*,
  //            GIF_SEEK_CALLBACK*,
  //            GIF_DRAW_CALLBACK*)
  // Ici on utilise nos callbacks SDFile via GIFOpenFile/GIFCloseFile/etc.
  gif.begin(LITTLE_ENDIAN_PIXELS);
  int rc = gif.open(path.c_str(), GIFOpenFile, GIFCloseFile, GIFReadFile, GIFSeekFile, GIFDraw);
  // AnimatedGIF::open / GIFInit() renvoie:
  //   1 = succès (GIFInit OK)
  //   0 = échec
  if (rc == 1)
  {
    gifRawPackMode = false;
    gifOpened = true;
    Serial.println("[GIF] open OK standard path=" + path + " rc=" + String(rc));
    return true;
  }

  gifRawPackMode = false;
  gifOpened = false;
  Serial.println("[GIF] open FAIL standard path=" + path + " rc=" + String(rc));
  return false;
}

// --------------------------------------------------
// openBestMedia
// --------------------------------------------------
DisplayMode openBestMedia(const String &basePath, const String &systemPath="")
{
  auto getSysName=[](const String &path)->String{
    if(!path.endsWith("/_default")) return "";
    int s2=path.lastIndexOf('/'); int s1=path.lastIndexOf('/',s2-1);
    if(s1<0) return ""; return path.substring(s1+1,s2);
  };
  auto getDP=[](const String &sysName)->String{return "/systems/_defaults/"+sysName;};
  auto openDP=[&](const String &sysName)->bool{
    String path=getDP(sysName)+".png";
    if(path==currentPngPath&&pngDrawn) return true;
    display->clearScreen();
    if(drawPng(path)){currentPngPath=path;pngDrawn=true;return true;}
    return false;
  };
  auto openDG=[&](const String &sysName)->bool{return openGif(getDP(sysName)+".gif", true, true);};

  bool isDefault=basePath.endsWith("/_default");
  if(!isDefault)
  {
    String path=basePath+".png";
    if(path==currentPngPath&&pngDrawn) return MODE_PNG;
    display->clearScreen();
    if(drawPng(path)){currentPngPath=path;pngDrawn=true;return MODE_PNG;}
    if(openGif(basePath+".gif", true, true)){pngDrawn=false;currentPngPath="";return MODE_GIF;}
  }
  else
  {
    String sysName=getSysName(basePath); char t=sysDefaultType(sysName);

    // Systemes _defaults : raw565 obligatoire, pas de raw565pack
    // drawPng() tente drawRaw565() puis fallback. Pas de openGif ici.
    if(drawPng(getDP(sysName)+".png")){currentPngPath=getDP(sysName)+".png";pngDrawn=true;return MODE_PNG;}
  }

  if(systemPath.length()>0)
  {
    String sysName=getSysName(systemPath); char t=sysDefaultType(sysName);

    // Ordre explicite selon le type cache:
    // p => PNG (raw565) d'abord
    // g => GIF d'abord
    // B => PNG d'abord puis GIF si échec
    if(t=='g')
    {
      if(openDG(sysName)){pngDrawn=false;currentPngPath="";return MODE_GIF;}
      String path=getDP(sysName)+".png";
      if(path==currentPngPath&&pngDrawn) return MODE_PNG;
      display->clearScreen();
      if(drawPng(path)){currentPngPath=path;pngDrawn=true;return MODE_PNG;}
    }
    else if(t=='p')
    {
      String path=getDP(sysName)+".png";
      if(path==currentPngPath&&pngDrawn) return MODE_PNG;
      display->clearScreen();
      if(drawPng(path)){currentPngPath=path;pngDrawn=true;return MODE_PNG;}
      if(openDG(sysName)){pngDrawn=false;currentPngPath="";return MODE_GIF;}
    }
    else // B ou autre
    {
      // B : on force raw565pack d'abord (openDG), même si raw565 existe
      if(openDG(sysName)){pngDrawn=false;currentPngPath="";return MODE_GIF;}

      String path=getDP(sysName)+".png";
      if(path==currentPngPath&&pngDrawn) return MODE_PNG;
      display->clearScreen();
      if(drawPng(path)){currentPngPath=path;pngDrawn=true;return MODE_PNG;}
    }
  }

  // Fallback final: forcer RAM default.raw565 (évite tout redécodage PNG en loop())
  gif.close(); gifOpened = false;
  pngDrawn = true;
  currentPngPath = "";
  display->clearScreen();
  if(drawDefaultRaw565Cached()) return MODE_PNG;

  // Si (exceptionnel) la RAM default.raw565 n'est pas disponible, rebasculer sur l'ancien fallback
  char defType=sysDefaultType("default");
  if(defType!='p'&&openDG("default")){pngDrawn=false;currentPngPath="";return MODE_GIF;}
  if(defType!='g'){
    String path=getDP("default")+".png";
    if(path==currentPngPath&&pngDrawn) return MODE_PNG;
    display->clearScreen();
    if(drawPng(path)){currentPngPath=path;pngDrawn=true;return MODE_PNG;}
  }

  if(!pngDrawn&&!gifOpened){display->clearScreen();currentPngPath="";}
  return MODE_BLACK;
}

// --------------------------------------------------
// Playlist
// --------------------------------------------------
String getNextGifSequential()
{
  if(!seqPlaylistFile){seqPlaylistFile=SD.open(playlistCachePath,FILE_READ);if(!seqPlaylistFile)return "";}
  if(!seqPlaylistFile.available()){seqPlaylistFile.seek(0);playIndex=0;}
  while(seqPlaylistFile.available())
  {
    String line=seqPlaylistFile.readStringUntil('\n');line.trim();
    if(line.length()>0){playIndex++;return line;}
  }
  return "";
}

String getNextGifRandom()
{
  if(gifCount<=0) return "";
  int idx=lastRandomIndex;
  if(gifCount>1){int t=0;while(idx==lastRandomIndex&&t<10){idx=random(0,gifCount);t++;}}
  else idx=0;
  lastRandomIndex=idx;
  if(!idxFileHandle){idxFileHandle=SD.open(playlistIdxPath,FILE_READ);if(!idxFileHandle)return getNextGifSequential();}
  idxFileHandle.seek((uint32_t)idx*4);
  uint32_t offset=0; idxFileHandle.read((uint8_t*)&offset,4);
  File cf=SD.open(playlistCachePath,FILE_READ); if(!cf) return "";
  cf.seek(offset); String line=cf.readStringUntil('\n'); cf.close(); line.trim();
  return line;
}

String getNextGif(){if(gifCount<=0)return "";return playlistRandom?getNextGifRandom():getNextGifSequential();}

void openNextGif()
{
  String next=(nextGifPath.length()>0)?nextGifPath:getNextGif(); nextGifPath="";
  if(next.length()==0||!openGif(next,false,true))
  {gifOpened=false;currentMode=MODE_BLACK;display->clearScreen();return;}
  currentMode=MODE_PLAYLIST; nextGifPath=getNextGif();
}

void resumePlaylist()
{
  gif.close(); gifOpened=false; currentPngPath=""; pngDrawn=false;
  if(nextGifFile){nextGifFile.close();nextGifFile=File();}
  nextGifPath=""; freeBigramAll();
  displayedMaskSysName="";
  if(gifCount>0){currentMode=MODE_PLAYLIST;openNextGif();}
  else{currentMode=MODE_BLACK;display->clearScreen();}
}

// --------------------------------------------------
// MQTT command processing
// --------------------------------------------------
bool hasPendingMqttCommand()
{
  if(mqttCmdMutex==nullptr) return false;
  if(xSemaphoreTake(mqttCmdMutex,0)!=pdTRUE) return false;
  bool has=(pendingCmd.type!=MqttCommand::CMD_NONE);
  xSemaphoreGive(mqttCmdMutex); return has;
}

void processPendingMqttCommand()
{
  if(mqttCmdMutex==nullptr) return;
  if(xSemaphoreTake(mqttCmdMutex,0)!=pdTRUE) return;
  MqttCommand cmd=pendingCmd; pendingCmd=MqttCommand(MqttCommand::CMD_NONE,"");
  xSemaphoreGive(mqttCmdMutex);
  if(cmd.type==MqttCommand::CMD_NONE) return;

  switch(cmd.type)
  {
  case MqttCommand::CMD_STOP:
    if(currentMode==MODE_PLAYLIST){Serial.println("[MQTT] stop ignored");break;}
    gif.close();gifOpened=false;currentPngPath="";pngDrawn=false;
    currentMode=MODE_BLACK;display->clearScreen();
    break;

  case MqttCommand::CMD_DEFAULT:
    resumePlaylist();
    break;

  case MqttCommand::CMD_SYSTEM:
    gif.close();gifOpened=false;pngDrawn=false;currentPngPath="";
    currentMode=MODE_BLACK;
    if(nextGifFile){nextGifFile.close();nextGifFile=File();nextGifPath="";}
    freeBigramAll();
    currentMode=openBestMedia("/systems/"+cmd.arg+"/_default");
    displayedMaskSysName=cmd.arg;
    break;

  case MqttCommand::CMD_GAME:
  {
    int slash=cmd.arg.indexOf('/');
    String sysName=(slash>=0)?cmd.arg.substring(0,slash):cmd.arg;
    String romName=(slash>=0)?cmd.arg.substring(slash+1):cmd.arg;
    String sysBase="/systems/"+sysName+"/_default";
    String gameBase="/systems/"+cmd.arg;
    if(imageFolder.length()>0)
      gameBase="/systems/"+sysName+"/"+imageFolder+"/"+romName;

    char slowFlag=sysDefaultSlowFlag(sysName);
    bool isSlow=(slowFlag=='L'||slowFlag=='l');
    bool needDrawMask = false;
    bool maskDrawn = false;

    // Debug général (doit s'afficher pour PS2 aussi)
    Serial.println("[CMD_GAME] enter sys=" + sysName
                   + " rom=" + romName
                   + " slowFlag=" + String(slowFlag)
                   + " isSlow=" + String(isSlow)
                   + " gameBase=" + gameBase);

    // FAST path (isSlow=0) : tenter directement le jeu en raw (drawPng gère *.raw via fallback)
    // Si le jeu n'existe pas (sur ta SD ps2 sans /systems/ps2), tomber sur default.png/default.raw des _defaults.
    if(!isSlow)
    {
      String gamePng = gameBase + ".png";
      String gameGif = gameBase + ".gif";
      char sysT = sysDefaultType(sysName);

      display->clearScreen();

      // Pour B : raw565pack d'abord (openGif sur .gif => .raw565pack+.meta)
      if(sysT == 'B')
      {
        if(openGif(gameGif, false, true))
        {
          pngDrawn = false;
          currentPngPath = "";
          currentMode = MODE_GIF;
          break;
        }
      }

      // Ordre standard : PNG d'abord
      if(drawPng(gamePng))
      {
        pngDrawn = true;
        currentPngPath = gamePng;
        currentMode = MODE_PNG;
        break;
      }

      // Puis GIF
      if(openGif(gameGif, false, true))
      {
        pngDrawn = false;
        currentPngPath = "";
        currentMode = MODE_GIF;
        break;
      }

      // fallback : default.png/default.raw
      String defPng = "/systems/_defaults/default.png";
      Serial.println("[CMD_GAME] fast fallback -> " + defPng);
      display->clearScreen();
      if(drawPng(defPng))
      {
        pngDrawn = true;
        currentPngPath = defPng;
        currentMode = MODE_PNG;
        break;
      }

      // si meme default ne passe pas, on retombe sur le mask
    }

    // Fermer l'animation precedente pour liberer l'etat, mais sans forcement clear l'ecran.
    gif.close();gifOpened=false;pngDrawn=false;currentPngPath="";

    if(isSlow)
    {
      needDrawMask = (displayedMaskSysName != sysName);
      // 1) afficher le mask d'attente si necessaire (sinon on le garde deja a l'ecran)
      if(needDrawMask)
      {
        String maskBase="/systems/_defaults/"+sysName;
        char maskType=sysDefaultType(sysName);

        if(maskType=='p')
        {
          String maskPng=maskBase+".png";
          display->clearScreen();
          if(drawPng(maskPng))
          {
            currentPngPath=maskPng;
            pngDrawn=true;
            currentMode=MODE_PNG;
            maskDrawn=true;
          }
          else
          {
            // PNG indisponible -> fallback GIF (au moins 1ère frame)
            String maskGif=maskBase+".gif";
            int fd=0;
            if(openGif(maskGif,true,true))
            {
              gif.playFrame(true,&fd);
              currentMode=MODE_GIF;
              pngDrawn=false;
              currentPngPath="";
              maskDrawn=true;
            }
            else
            {
              currentMode=MODE_BLACK;
            }
          }
        }
        else
        {
          // Mask obligatoire en raw565 (jamais raw565pack)
          String maskRaw565 = maskBase + ".raw565";
          if(drawRaw565(maskRaw565))
          {
            currentMode=MODE_BLACK; // on garde juste le contenu affiché du mask
            pngDrawn=false;
            currentPngPath="";
            maskDrawn=true;
          }
          else
          {
            currentMode=MODE_BLACK;
          }
        }

        if (maskDrawn)
        {
          displayedMaskSysName=sysName;
        }
        else if (needDrawMask)
        {
          displayedMaskSysName="";
        }
      }

      // 2) precharger le bigramme (table chargee une fois par systeme)
      preloadBigram(sysName, romName);
      char cached=findInGamesCache(sysName, romName);

      // DEBUG pour comprendre pourquoi le jeu n'est pas affiché (vs mask)
      Serial.println("[CMD_GAME] debug sys=" + sysName
                   + " rom=" + romName
                   + " cached=" + String(cached)
                   + " isSlow=" + String(isSlow)
                   + " gameBase=" + gameBase
                   + " slowFlag=" + String(slowFlag));

      // NE PAS clearScreen ici: le mask doit rester visible pendant le chargement.
      // En mode lent, on force le type d'affichage selon le flag système:
      // - sysType 'g'/'B' => raw565pack via openGif(...) (pas drawPng/raw565)
      // - sysType 'p'     => drawPng/raw565
      {
        char sysT = sysDefaultType(sysName);
        // Si le jeu n'est PAS dans le cache bigram (cached='?'), fallback RAM direct.
        // Ne PAS forcer 'g' (qui ferait openGif() lent sur dossier de 800+ fichiers).
        if(cached == '?')
        {
          Serial.println("[CMD_GAME] slow cached=? -> fallback default.raw565 RAM t=" + String(millis()));
          if(drawDefaultRaw565Cached())
          {
            // Garder displayedMaskSysName=sysName pour que loop() ne clearScreen pas
            // (voir MODE_PNG: if(displayedMaskSysName.length()==0) display->clearScreen())
            displayedMaskSysName = sysName;
            pngDrawn=true; currentPngPath=""; currentMode=MODE_BLACK;
            Serial.println("[CMD_GAME] slow cached=? fallback OK t=" + String(millis()));
            break;
          }
        }
        // cached present: force le type d'affichage selon le flag systeme
        char cachedBefore = cached;
        if(sysT=='g' || sysT=='B') cached = 'g';
        else if(sysT=='p') cached = 'p';
        Serial.println("[CMD_GAME] slow cached force sysType=" + String(sysT)
                       + " cachedBefore=" + String(cachedBefore)
                       + " cachedAfter=" + String(cached));
      }
      if(cached=='p')
      {
        String path=gameBase+".png";
        // Async pngle_new() échoue systématiquement en tâche => fallback synchrone
        Serial.println("[CMD_GAME] slow PNG fallback sync sys=" + sysName + " path=" + path);

        currentPngPath = path;
        currentPngAsyncWanted = false;
        asyncPngCancel = true;

        // Remplacer le mask par l'image PNG (garder le mask si affiche)
        // En LENT, on ne clear que si pas de mask actif.
        if (displayedMaskSysName.length() == 0)
        {
          Serial.println("[CMD_GAME] slow clearScreen (no mask) BEFORE t=" + String(millis()));
          display->clearScreen();
          Serial.println("[CMD_GAME] slow clearScreen AFTER t=" + String(millis()));
        }
        else
        {
          Serial.println("[CMD_GAME] slow keep mask on screen during load t=" + String(millis()));
        }

        Serial.println("[CMD_GAME] slow before drawPng t=" + String(millis())
                       + " maskLen=" + String(displayedMaskSysName.length()));
        bool okDraw = drawPng(path);
        Serial.println("[CMD_GAME] slow after drawPng ok=" + String(okDraw ? "1" : "0")
                       + " t=" + String(millis()));

        if(okDraw)
        {
          pngDrawn = true;
          displayedMaskSysName = "";
          currentMode = MODE_PNG;
        }
        else
        {
          // PNG indisponible -> fallback GIF
          pngDrawn = false;
          String gameGif = gameBase + ".gif";

          int fd = 0;
          if(openGif(gameGif, false, true))
          {
            gif.playFrame(true, &fd);
            displayedMaskSysName = "";
            currentMode = MODE_GIF;
          }
          else
          {
            currentMode = MODE_BLACK;
            display->clearScreen();
            displayedMaskSysName = "";
          }

          currentPngPath = "";
        }

        break;
      }
      else if(cached=='g')
      {
        // Tentative raw565pack via openGif (ne pas clearScreen, le mask reste visible)
        String gifPath=gameBase+".gif";
        if(openGif(gifPath,false,true))
        {
          currentMode=MODE_GIF;
          displayedMaskSysName="";
          loadBigramTable(sysName);
          break;
        }
        loadBigramTable(sysName);

        // raw565pack échoué → fallback immédiat: drawDefaultRaw565Cached() depuis RAM
        // (Évite probe3/ drawPng / drawRaw565 qui ferait un SD.open() sur 800+ fichiers
        //  dans un dossier FAT lent, prenant 5+ secondes.)
        Serial.println("[CMD_GAME] slow cached=g raw565pack fail -> fallback default.raw565 t=" + String(millis()));
        if(drawDefaultRaw565Cached())
        {
          pngDrawn=true;
          currentPngPath="";
          displayedMaskSysName="";
          currentMode=MODE_PNG;
          Serial.println("[CMD_GAME] slow cached=g fallback default.raw565 OK t=" + String(millis()));
          break;
        }
        // fallback mem epuise -> probe3 classique
      }

      // 3) fallback: tester existence PNG/GIF sans effacer l'ecran
      {
        String pngPath=gameBase+".png";

        unsigned long tProbeStart = millis();
        Serial.println("[CMD_GAME] probe3 start t=" + String(tProbeStart) + " png=" + pngPath);

        Serial.println("[CMD_GAME] probe3 calling drawPng (skip SD.exists) t=" + String(millis()));

        if(drawPng(pngPath))
        {
          currentPngPath=pngPath; pngDrawn=true; currentMode=MODE_PNG;
          displayedMaskSysName="";
          loadBigramTable(sysName);
          break;
        }
        else
        {
          Serial.println("[CMD_GAME] probe3 drawPng failed t=" + String(millis()));
        }
      }

      // Dernier recours (N classique) si tout echoue
      currentMode=openBestMedia(gameBase,sysBase);
      loadBigramTable(sysName);
      break;
    }

    // NORMAL: comportement actuel maintenu
    currentMode=MODE_BLACK;
    preloadBigram(sysName, romName);
    char cached=findInGamesCache(sysName, romName);

    if(cached=='p'){
      String path=gameBase+".png"; display->clearScreen();
      if(drawPng(path)){currentPngPath=path;pngDrawn=true;currentMode=MODE_PNG;break;}
      loadBigramTable(sysName);
    } else if(cached=='g'){
      if(openGif(gameBase+".gif")){pngDrawn=false;currentPngPath="";currentMode=MODE_GIF;break;}
    }
    currentMode=openBestMedia(gameBase,sysBase);
    loadBigramTable(sysName);
    break;
  }

  case MqttCommand::CMD_STARTCLIP:
    Serial.println("[MQTT] startgameclip -> playlist");
    resumePlaylist();
    break;

  case MqttCommand::CMD_RESUMESYS:
    Serial.println("[MQTT] resumesys -> "+cmd.arg);
    gif.close();gifOpened=false;pngDrawn=false;currentPngPath="";
    currentMode=MODE_BLACK;
    if(nextGifFile){nextGifFile.close();nextGifFile=File();nextGifPath="";}
    freeBigramAll();
    currentMode=openBestMedia("/systems/"+cmd.arg+"/_default");
    displayedMaskSysName=cmd.arg;
    break;

  default: break;
  }
}

// --------------------------------------------------
// MQTT callback
// --------------------------------------------------
void onMqttMessage(char *topic, byte *payload, unsigned int length)
{
  String t=String(topic); String msg="";
  for(unsigned int i=0;i<length;i++) msg+=(char)payload[i];
  msg.trim();
  Serial.println("[MQTT] "+t+" -> "+msg);
  mqttLogAdd(t,msg);

  if(mqttCmdMutex==nullptr) return;
  if(xSemaphoreTake(mqttCmdMutex,pdMS_TO_TICKS(10))!=pdTRUE) return;

  if     (t=="marquee/cmd/stop")    pendingCmd=MqttCommand(MqttCommand::CMD_STOP,"");
  else if(t=="marquee/cmd/default") pendingCmd=MqttCommand(MqttCommand::CMD_DEFAULT,"");
  else if(t=="marquee/cmd/system")  {lastSysName=msg;pendingCmd=MqttCommand(MqttCommand::CMD_SYSTEM,msg);}
  else if(t=="marquee/cmd/game")    pendingCmd=MqttCommand(MqttCommand::CMD_GAME,msg);
  else if(t==mqttEventTopic)
  {
    String ev=extractField(msg,"EVENT");
    String inGame=extractField(msg,"IN_GAME");
    String lastSys=extractField(msg,"LAST_SYS");
    Serial.println("[EVENT] ev="+ev+" in_game="+inGame+" sys="+lastSys);
    if(ev=="startgameclip"&&inGame=="0")
      pendingCmd=MqttCommand(MqttCommand::CMD_STARTCLIP,"");
    else if((ev=="stopgameclip"||ev=="wakeup"||ev=="systembrowsing")&&inGame=="0")
    {
      String sys=(lastSys.length()>0)?lastSys:lastSysName;
      if(sys.length()>0){lastSysName=sys;pendingCmd=MqttCommand(MqttCommand::CMD_RESUMESYS,sys);}
      else pendingCmd=MqttCommand(MqttCommand::CMD_DEFAULT,"");
    }
  }
  xSemaphoreGive(mqttCmdMutex);
}

// --------------------------------------------------
// MQTT task
// --------------------------------------------------
void mqttTask(void *param)
{
  (void)param;
  vTaskDelay(pdMS_TO_TICKS(MQTT_START_DELAY_MS));
  unsigned long lastMqttConnectedMs=millis();

  for(;;)
  {
    if(!wifiEnabled||recalboxIP.length()==0){vTaskDelay(pdMS_TO_TICKS(2000));continue;}
    if(WiFi.status()!=WL_CONNECTED){vTaskDelay(pdMS_TO_TICKS(1000));continue;}

    if(!mqttClient.connected())
    {
      Serial.println("[MQTT] connecting to "+recalboxIP);
      if(mqttClient.connect(MQTT_CLIENT))
      {
        Serial.println("[MQTT] connected");
        lastMqttConnectedMs=millis();
        mqttClient.subscribe("marquee/cmd/stop");
        mqttClient.subscribe("marquee/cmd/default");
        mqttClient.subscribe("marquee/cmd/system");
        mqttClient.subscribe("marquee/cmd/game");
        mqttClient.subscribe(mqttEventTopic.c_str());
        if (mqttCmdMutex != nullptr && xSemaphoreTake(mqttCmdMutex, pdMS_TO_TICKS(MQTT_SEMAPHORE_WAIT_MS)) == pdTRUE)
        {
          if(currentMode!=MODE_PLAYLIST&&gifCount>0)
            pendingCmd=MqttCommand(MqttCommand::CMD_DEFAULT,"");
          xSemaphoreGive(mqttCmdMutex);
        }
      }
      else
      {
        Serial.println("[MQTT] failed rc="+String(mqttClient.state()));
        unsigned long now=millis();
        if((now-lastMqttConnectedMs)>=MQTT_OFFLINE_FALLBACK_MS)
        {
          if(currentMode!=MODE_PLAYLIST&&gifCount>0)
          {
            Serial.println("[MQTT] injoignable -> reprise playlist");
            if (mqttCmdMutex != nullptr && xSemaphoreTake(mqttCmdMutex, pdMS_TO_TICKS(MQTT_SEMAPHORE_WAIT_MS)) == pdTRUE)
            {pendingCmd=MqttCommand(MqttCommand::CMD_DEFAULT,"");xSemaphoreGive(mqttCmdMutex);}
            lastMqttConnectedMs=now;
          }
        }
        vTaskDelay(pdMS_TO_TICKS(MQTT_RETRY_MS)); continue;
      }
    }
    else lastMqttConnectedMs=millis();

    mqttClient.loop();
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

// --------------------------------------------------
// WiFi
// --------------------------------------------------
bool parseIP(const String &s, IPAddress &ip)
{
  int a,b,c,d;
  if(sscanf(s.c_str(),"%d.%d.%d.%d",&a,&b,&c,&d)!=4) return false;
  if(a<0||a>255||b<0||b>255||c<0||c>255||d<0||d>255) return false;
  ip=IPAddress(a,b,c,d); return true;
}

bool applyStaticIP()
{
  if(!wifiStaticEnabled) return true;
  IPAddress localIP,gateway,subnet,dns1,dns2;
  if(!parseIP(wifiStaticIP,localIP)||!parseIP(wifiGateway,gateway)||!parseIP(wifiSubnet,subnet)) return false;
  bool h1=parseIP(wifiDNS1,dns1),h2=parseIP(wifiDNS2,dns2);
  if(h1&&h2) return WiFi.config(localIP,gateway,subnet,dns1,dns2);
  if(h1)     return WiFi.config(localIP,gateway,subnet,dns1);
  return WiFi.config(localIP,gateway,subnet);
}

void setupWiFiFromConfig()
{
  if(!wifiEnabled){WiFi.disconnect(true);WiFi.mode(WIFI_OFF);return;}
  if(wifiSSID.length()==0){
    if(showInfo)showWifiStatusScreen("NO WIFI","NO SSID",display->color565(255,0,0));
    delay(1200);WiFi.disconnect(true);WiFi.mode(WIFI_OFF);return;
  }
  WiFi.mode(WIFI_STA);WiFi.setSleep(false);WiFi.setAutoReconnect(true);
  if(!applyStaticIP()){if(showInfo)showWifiStatusScreen("WIFI","IP CFG ERR",display->color565(255,0,0));delay(1200);}
  WiFi.begin(wifiSSID.c_str(),wifiPassword.c_str());
  if(showInfo)showWifiStatusScreen("WIFI","CONNECT",display->color565(0,180,255));
  unsigned long start=millis();
  while(WiFi.status()!=WL_CONNECTED&&(millis()-start)<12000) delay(200);
  if(WiFi.status()==WL_CONNECTED)
  {
    String ip=WiFi.localIP().toString();
    if(showInfo)showWifiStatusScreen("WIFI OK",fitLabel(ip,14),display->color565(0,255,0));
    Serial.println("[WIFI] connected: "+ip);
    delay(1200);
    if(recalboxIP.length()>0){
      mqttClient.setServer(recalboxIP.c_str(),MQTT_PORT);
      mqttClient.setCallback(onMqttMessage);
      mqttClient.setKeepAlive(60);
      mqttClient.setSocketTimeout(30);
    }
  }
  else{
    if(showInfo)showWifiStatusScreen("NO WIFI","TIMEOUT",display->color565(255,0,0));
    Serial.println("[WIFI] failed");delay(1200);WiFi.disconnect(true);WiFi.mode(WIFI_OFF);
  }
}

void maintainWiFi()
{
  if(!wifiEnabled||wifiSSID.length()==0) return;
  if(WiFi.status()==WL_CONNECTED) return;
  unsigned long now=millis();
  if(now-lastWifiReconnectAttempt<5000) return;
  lastWifiReconnectAttempt=now;
  Serial.println("[WIFI] reconnect");
  showWifiStatusScreen("NO WIFI","RECONNECT",display->color565(255,128,0));
  delay(REBOOT_DELAY_MS); WiFi.disconnect(); delay(REBOOT_DISCONNECT_DELAY_MS);
  WiFi.begin(wifiSSID.c_str(),wifiPassword.c_str());

}

// --------------------------------------------------
// Config
// --------------------------------------------------
void loadConfig()
{
  File cfg=SD.open("/config.ini"); if(!cfg) return;
  while(cfg.available())
  {
    String line=cfg.readStringUntil('\n');line.trim();
    if(!line.length()||line[0]=='#'||line[0]==';') continue;
    int eq=line.indexOf('=');if(eq<0) continue;
    String key=line.substring(0,eq),value=line.substring(eq+1);
    key.trim();value.trim();key.toLowerCase();
    int cp=value.indexOf('#');if(cp>=0)value=value.substring(0,cp);
    cp=value.indexOf(';');   if(cp>=0)value=value.substring(0,cp);
    value.trim();

    if     (key=="playlist"            &&value.length()) playlistName     =value;
    else if(key=="wifi_enabled")                         wifiEnabled      =(value=="1");
    else if(key=="wifi_ssid")                            wifiSSID         =value;
    else if(key=="wifi_password")                        wifiPassword     =value;
    else if(key=="wifi_static_enabled")                  wifiStaticEnabled=(value=="1");
    else if(key=="wifi_static_ip")                       wifiStaticIP     =value;
    else if(key=="wifi_gateway")                         wifiGateway      =value;
    else if(key=="wifi_subnet")                          wifiSubnet       =value;
    else if(key=="wifi_dns1")                            wifiDNS1         =value;
    else if(key=="wifi_dns2")                            wifiDNS2         =value;
    else if(key=="bluetooth_enabled")                    bluetoothEnabled =(value=="1");
    else if(key=="bluetooth_name"     &&value.length())  bluetoothName    =value;
    else if(key=="recalbox_ip"        &&value.length())  recalboxIP       =value;
    else if(key=="random")                               playlistRandom   =(value!="0");
    else if(key=="info")                                 showInfo         =(value!="0");
    else if(key=="mqtt_event_topic"   &&value.length())  mqttEventTopic   =value;
  }
  cfg.close();

  // Images directement dans systems/<sys>/, pas de sous-dossier
  gamesCacheFile = "/games_cache.bin";
  Serial.println("[CACHE] fichier jeux: " + gamesCacheFile);

  if(!playlistName.length()) return;
  playlistSourcePath="/playlists/"+playlistName;
  String base=playlistName; int dot=base.lastIndexOf('.');if(dot>0)base=base.substring(0,dot);
  playlistCachePath="/playlists/"+base+".cache";
  playlistSigPath  ="/playlists/"+base+".sig";
  playlistIdxPath  ="/playlists/"+base+".idx";
}

bool isValidPlaylistLine(String line){line.trim();return line.length()&&line[0]!='#'&&line[0]!=';'&&line[0]=='/';}

uint32_t computeFileHash(const String &path)
{
  File f = SD.open(path, FILE_READ);
  if (!f) return 0;

  uint32_t h = 2166136261u;

  uint8_t buf[512];
  while (true)
  {
    size_t n = f.read(buf, sizeof(buf));
    if (n == 0) break;

    for (size_t i = 0; i < n; i++)
    {
      h ^= buf[i];
      h *= 16777619u;
    }
  }

  f.close();
  return h;
}

uint32_t readSavedSignature()
{
  File f=SD.open(playlistSigPath,FILE_READ);if(!f)return 0;
  String s=f.readStringUntil('\n');f.close();s.trim();
  return s.length()?(uint32_t)strtoul(s.c_str(),NULL,10):0;
}

bool writeSignature(uint32_t sig)
{
  if(SD.exists(playlistSigPath))SD.remove(playlistSigPath);
  File f=SD.open(playlistSigPath,FILE_WRITE);if(!f)return false;
  f.println(String(sig));f.close();return true;
}

int rebuildPlaylistCache()
{
  File src=SD.open(playlistSourcePath,FILE_READ);if(!src)return 0;
  if(SD.exists(playlistCachePath))SD.remove(playlistCachePath);
  File cache=SD.open(playlistCachePath,FILE_WRITE);if(!cache){src.close();return 0;}
  int n=0;showLoadingHourglass(0);
  while(src.available()){
    String line=src.readStringUntil('\n');line.trim();
    if(isValidPlaylistLine(line)){cache.println(line);n++;if((n%6)==0)showLoadingHourglass(n);}
    delay(0);
  }
  src.close();cache.close();showLoadingHourglass(n);return n;
}

int buildOffsetIndex()
{
  File cache=SD.open(playlistCachePath,FILE_READ);if(!cache)return 0;
  if(SD.exists(playlistIdxPath))SD.remove(playlistIdxPath);
  File idx=SD.open(playlistIdxPath,FILE_WRITE);
  int n=0;
  while(cache.available()){
    uint32_t pos=cache.position();
    String line=cache.readStringUntil('\n');line.trim();
    if(line.length()){if(idx)idx.write((uint8_t*)&pos,4);n++;}
    delay(0);
  }
  cache.close();if(idx)idx.close();
  if(idxFileHandle)idxFileHandle.close();
  idxFileHandle=SD.open(playlistIdxPath,FILE_READ);
  if(!playlistRandom){if(seqPlaylistFile)seqPlaylistFile.close();seqPlaylistFile=SD.open(playlistCachePath,FILE_READ);playIndex=0;}
  return n;
}


// --------------------------------------------------
// Splash screen — version au démarrage (info=1 uniquement)
// --------------------------------------------------
#define RETRO_VERSION "Raw565 Ed."

void showSplashScreen()
{
  display->clearScreen();
  display->setTextWrap(false);
  display->setTextSize(1);

  uint16_t red   = display->color565(255, 40,  40);
  uint16_t blue  = display->color565( 60, 130, 255);
  uint16_t green = display->color565( 50, 220,  80);
  uint16_t white = display->color565(235, 235, 235);

  // Panel = 2 x 64px = 128px de large, 32px de haut
  // Taille 1 = 6px par caractere
  // "RetroBoxLED" = 11 x 6 = 66px  → x = (128-66)/2 = 31
  // "v1.0.7"      =  6 x 6 = 36px  → x = (128-36)/2 = 46 (confirme)

  // Ligne 1 : RetroBoxLED centré, un seul setCursor puis print enchaînés
  display->setCursor(31, 8);
  display->setTextColor(red);   display->print("Recal");
  display->setTextColor(blue);  display->print("Box");
  display->setTextColor(green); display->print("DMD");

  // Ligne 2 : version centrée
  display->setCursor(33, 21);
  display->setTextColor(white);
  display->print(RETRO_VERSION);

  delay(2500);
  display->clearScreen();
}

// --------------------------------------------------
// Setup
// --------------------------------------------------
void setup()
{
  Serial.begin(115200); delay(1000); randomSeed(micros());

  // Heap allocations to reduce BSS (ESP32 DRAM linker limit on .dram0.bss)
  if (!sysCacheKeys)
  {
    sysCacheKeys = (char (*)[32])malloc(sizeof(char[32]) * SYS_CACHE_MAX);
    sysCacheVals = (char*)malloc(SYS_CACHE_MAX);
    sysCacheSlowVals = (char*)malloc(SYS_CACHE_MAX);
    gamesIdx = (GamesSysIdx*)malloc(sizeof(GamesSysIdx) * GAMES_IDX_MAX);
  }

  if (!sysCacheKeys || !sysCacheVals || !sysCacheSlowVals || !gamesIdx)
  {
    Serial.println("[MEM] heap alloc failed - halting");
    while (1) { delay(SLEEP_DELAY_MS); yield(); }
  }

  HUB75_I2S_CFG::i2s_pins pins={R1_PIN,G1_PIN,B1_PIN,R2_PIN,G2_PIN,B2_PIN,A_PIN,B_PIN,C_PIN,D_PIN,E_PIN,LAT_PIN,OE_PIN,CLK_PIN};
  HUB75_I2S_CFG mxconfig(PANEL_RES_X,PANEL_RES_Y,PANEL_CHAIN,pins);
  mxconfig.latch_blanking=4; mxconfig.i2sspeed=HUB75_I2S_CFG::HZ_10M;
  mxconfig.min_refresh_rate=60; mxconfig.clkphase=false; mxconfig.double_buff=false;

  display=new MatrixPanel_I2S_DMA(mxconfig);
  display->begin(); display->setBrightness8(120); display->clearScreen();

  spiSD.begin(VSPI_SCLK,VSPI_MISO,VSPI_MOSI,SD_CS_PIN);
  if(!SD.begin(SD_CS_PIN,spiSD)){
    showMessage("SD ERROR","NO CARD",display->color565(255,0,0));
    while (1) { delay(SLEEP_DELAY_MS); yield(); }
  }

  gif.begin(LITTLE_ENDIAN_PIXELS);

  {
    File cfg=SD.open("/config.ini");
    if(cfg){
      while(cfg.available()){
        String line=cfg.readStringUntil('\n');line.trim();
        if(line.startsWith("info=")||line.startsWith("info =")){
          String val=line.substring(line.indexOf('=')+1);val.trim();
          showInfo=(val!="0");break;
        }
      }
      cfg.close();
    }
  }

  showSplashScreen();  // Toujours affiché, indépendamment de info=

  // Charge le cache systèmes (systems_cache.dat). Si absent, on ne rescanner
  // que si l'utilisateur a info=1. Le script Python écrit déjà ce fichier.
  if(!loadSysDefaultCache()){
    if(showInfo)showMessage("MARQUEE","Indexation...",display->color565(100,100,255));
    buildSysDefaultCache();
  } else {
    Serial.println("[CACHE] utilise .dat existant, pas de scan recusrsif");
  }

  // Precharger le fallback default.raw565 en RAM des le setup
  // (pas au 1er appel de fallback). 8KB, heap dispo.
  ensureDefaultRaw565Cached();
  if (defaultRaw565Cached) Serial.println("[CACHE] default.raw565 en RAM");
  else                     Serial.println("[CACHE] default.raw565 absent");

  loadConfig();

  // Pour les systèmes flags 'L' (lents), games_cache.bin est court-circuité
  // dans findInGamesCache(). Inutile de le charger pour ces systèmes.
  // On le charge quand même pour les systèmes 'N' qui en ont besoin.
  if(!loadGamesIndex())
    Serial.println("[GCACHE] "+gamesCacheFile+" absent");
  else
    Serial.println("[GCACHE] OK - "+String(gamesIdxCount)+" systemes");

  if(!showInfo){
    String defBase="/systems/_defaults/default";
    if(openGif(defBase+".gif")){gifOpened=true;for(int i=0;i<5;i++){int fd=0;gif.playFrame(true,&fd);delay(fd>0?fd:33);}}
    else{drawPng(defBase+".png");delay(500);}
  }

  setupBluetoothFromConfig();
  setupWiFiFromConfig();

  mqttCmdMutex=xSemaphoreCreateMutex();
  pendingCmd=MqttCommand(MqttCommand::CMD_NONE,"");

  if(playlistName.length()==0){
    showMessage("NO PLAYLIST","config.ini",display->color565(255,128,0));
    delay(3000);display->clearScreen();currentMode=MODE_BLACK;
    goto start_mqtt_task;
  }

  {
    Serial.println("[PLAYLIST] sig compute start t=" + String(millis()));
    uint32_t curSig=computeFileHash(playlistSourcePath);
    Serial.println("[PLAYLIST] sig compute done t=" + String(millis()) + " curSig=" + String(curSig));

    Serial.println("[PLAYLIST] saved signature start t=" + String(millis()));
    uint32_t savSig=readSavedSignature();
    Serial.println("[PLAYLIST] saved signature done t=" + String(millis()) + " savSig=" + String(savSig));

    bool haveCache = false;
    if (curSig && curSig==savSig)
    {
      File cacheF = SD.open(playlistCachePath, FILE_READ);
      File idxF   = SD.open(playlistIdxPath,   FILE_READ);
      haveCache   = (cacheF && idxF);

      if (cacheF) cacheF.close();
      if (idxF)   idxF.close();
    }
    Serial.println("[PLAYLIST] cacheCheck haveCache=" + String(haveCache) + " t=" + String(millis()));

    if(!haveCache){
      Serial.println("[PLAYLIST] rebuildPlaylistCache start t=" + String(millis()));
      int rebuildN = rebuildPlaylistCache();
      Serial.println("[PLAYLIST] rebuildPlaylistCache done n=" + String(rebuildN) + " t=" + String(millis()));
      Serial.println("[PLAYLIST] writeSignature start t=" + String(millis()));
      (void)writeSignature(curSig);
      Serial.println("[PLAYLIST] writeSignature done t=" + String(millis()));

      Serial.println("[PLAYLIST] buildOffsetIndex start t=" + String(millis()));
      gifCount=buildOffsetIndex();
      Serial.println("[PLAYLIST] buildOffsetIndex done gifCount=" + String(gifCount) + " t=" + String(millis()));
    }
    else
    {
      // Index deja present: on ne le reconstruit pas.
      Serial.println("[PLAYLIST] use cached idx start t=" + String(millis()));
      if(idxFileHandle) idxFileHandle.close();
      idxFileHandle = SD.open(playlistIdxPath, FILE_READ);
      if(!idxFileHandle){
        Serial.println("[PLAYLIST] cached idx open failed -> rebuild");
        Serial.println("[PLAYLIST] buildOffsetIndex start t=" + String(millis()));
        gifCount=buildOffsetIndex();
        Serial.println("[PLAYLIST] buildOffsetIndex done gifCount=" + String(gifCount) + " t=" + String(millis()));
      } else {
        size_t idxSize = idxFileHandle.size();
        gifCount = (idxSize >= 4) ? (int)(idxSize / 4) : 0;
        if(!playlistRandom){
          if(seqPlaylistFile) seqPlaylistFile.close();
          seqPlaylistFile = SD.open(playlistCachePath, FILE_READ);
          playIndex = 0;
        }
        Serial.println("[PLAYLIST] cached idx ok gifCount=" + String(gifCount) + " t=" + String(millis()));
      }
    }

    Serial.println("[PLAYLIST] showPlaylistInfoScreen start t=" + String(millis()));
    showPlaylistInfoScreen(); delay(1300);
    Serial.println("[PLAYLIST] showPlaylistInfoScreen done t=" + String(millis()));
    if(gifCount==0){
      showMessage("PLAYLIST","EMPTY",display->color565(255,128,0));
      delay(2000);display->clearScreen();currentMode=MODE_BLACK;
      goto start_mqtt_task;
    }
    playIndex=0;lastRandomIndex=-1;currentMode=MODE_PLAYLIST;openNextGif();
  }

start_mqtt_task:
  if(wifiEnabled&&recalboxIP.length()>0)
    xTaskCreatePinnedToCore(mqttTask,"mqttTask",4096,NULL,1,&mqttTaskHandle,0);
}

// --------------------------------------------------
// Loop
// --------------------------------------------------
void loop()
{
  maintainWiFi(); processPendingMqttCommand();
  if(requestNextGif){requestNextGif=false;openNextGif();}
  if (requestReboot) { delay(SLEEP_DELAY_MS); ESP.restart(); }

  switch(currentMode)
  {
  case MODE_PLAYLIST:
    if(!gifOpened){display->clearScreen();currentMode=MODE_BLACK;break;}
    {
      int fd=0; bool frameOk=gifPlayFrameCompat(false,&fd);
      if(!frameOk){openNextGif();break;}
      if(fd<=0)fd=10;
      if(nextGifPath.length()==0)nextGifPath=getNextGif();
      unsigned long t=millis();
      while((long)(millis()-t)<fd){if(hasPendingMqttCommand())break;processPendingMqttCommand();delay(0);}
      if(nextGifPath.length()>0&&!nextGifFile)nextGifFile=SD.open(nextGifPath.c_str());
    }
    break;

  case MODE_GIF:
    if(!gifOpened){display->clearScreen();currentMode=MODE_BLACK;break;}
    {
      int fd=0; bool frameOk=gifPlayFrameCompat(false,&fd);
      if(!frameOk){gifResetCompat();break;}
      if(fd<=0)fd=10;
      unsigned long t=millis();
      while((long)(millis()-t)<fd){if(hasPendingMqttCommand())break;processPendingMqttCommand();delay(0);}
    }
    break;

  case MODE_PNG:
    if(currentPngPath.length()==0){
      // si un mask doit rester visible (LENT), on n'efface pas l'écran ici
      if(displayedMaskSysName.length()==0) display->clearScreen();
      currentMode=MODE_BLACK;break;
    }
    if(!pngDrawn)
    {
      if(currentPngAsyncWanted)
      {
        // Lancer le decode async si necessaire
        if(!asyncPngInProgress && !asyncPngReady)
        {
          // async PNG doit être déclenché uniquement depuis CMD_GAME (slow PNG),
          // on évite donc de relancer ici pour ne pas spammer des tâches.
        }
        // Fallback sécurité si l’async ne devient jamais prêt
        // Si l’async s’est terminée (échec ou abandon) sans devenir ready, fallback immédiatement
        else if(!asyncPngInProgress && !asyncPngReady)
        {
          Serial.println("[PNG-ASYNC] async ended -> fallback drawPng reqId=" + String(asyncPngActiveRequestId)
                         + " path=" + currentPngPath);
          asyncPngCancel = true;
          drawPng(currentPngPath);
          pngDrawn = true;
          currentPngAsyncWanted = false;
          asyncPngReady = false;
        }
        else if(!asyncPngReady && asyncPngStartMs > 0 && (millis() - asyncPngStartMs) > 1500UL)
        {
          Serial.println("[PNG-ASYNC] timeout(1500ms) fallback drawPng reqId=" + String(asyncPngActiveRequestId) + " path=" + currentPngPath);
          asyncPngCancel = true;
          drawPng(currentPngPath);
          pngDrawn = true;
          currentPngAsyncWanted = false;
          asyncPngReady = false;
        }

        // Si pret, blit et on repasse en etat PNG affiche
        if(asyncPngReady)
        {
          // Ne pas effacer le mask tant qu'il doit rester affiché : on recouvre directement en blit
          if(displayedMaskSysName.length()==0) display->clearScreen();
          blitPngAsyncFbToDisplay();
          asyncPngReady = false;
          currentPngAsyncWanted = false;
          pngDrawn = true;
        }
      }
      else
      {
        // Comportement normal (PNG "N")
        drawPng(currentPngPath);
        pngDrawn = true;
      }
    }
    {
      unsigned long t = millis();
      while ((long)(millis() - t) < 100) { if (hasPendingMqttCommand()) break; processPendingMqttCommand(); delay(1); }
    }
    break;

  case MODE_BLACK:
  default:
    {
      unsigned long t = millis();
      while ((long)(millis() - t) < 50) { if (hasPendingMqttCommand()) break; processPendingMqttCommand(); delay(1); }
    }
    break;
  }
}
