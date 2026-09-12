// ============================================
// udp_freeze_isolation.ino -- sketch minimal de diagnostic
// ============================================
// Etape 1 du plan de chasse au bug "gel de reception UDP" (voir
// DECISIONS.md, projet RecalBox_DMD). But UNIQUE : trancher si le gel
// vient du firmware principal (interaction avec l'affichage HUB75/DMA,
// heap, SD, etc.) ou de la plateforme elle-meme (ESP32/lwIP/AP), en
// reproduisant EXACTEMENT le meme reseau/AP/port UDP mais SANS AUCUNE
// autre logique (pas de GIF, pas de display, pas de SD, pas de heap
// lourd).
//
// A flasher sur un 2e module ESP32 (jamais utilise pour cette
// investigation), place physiquement pres du DMD de production.
//
// Comportement : se connecte au meme WiFi, ecoute UDP sur le meme port
// (5005), repond "PONG:<compteur>" a chaque paquet recu, log serial
// minimal (timestamp + compteur + ecart depuis le dernier paquet). Meme
// detection d'episode [UDPGAP] que le firmware principal (v190), pour
// pouvoir comparer directement frequence/duree des 2 cote a cote.
//
// SSID/mot de passe injectes automatiquement depuis la config du DMD de
// prod (recuperes via son endpoint web /load) -- meme reseau exact, pas
// de simple "meme SSID tape a la main" qui pourrait diverger.
//
// v2 -- etape 1b du plan : resultat de la v1 (WiFiUDP seul, sans aucune
// autre logique) sur 21 minutes de ping continu (5s) -- 240/240 paquets
// recus, ZERO coupure. Ecarte donc la plateforme ESP32/WiFi/lwIP/AP comme
// cause -- le bug vit dans le firmware principal. Prochain suspect le
// plus probable, teste ICI : le driver d'affichage HUB75/DMA (interruptions
// haute frequence pouvant affamer la tache WiFi interne par intermittence).
// Ajoute la MEME config HUB75 que le firmware principal (pins/resolution
// identiques, copiees de RecalBox_DMD.ino) + un rafraichissement continu
// simple (pas de SD/GIF/logique complexe) pour isoler CE SEUL sous-systeme.
// ============================================

#include <WiFi.h>
#include <WiFiUdp.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

// Meme config exacte que RecalBox_DMD.ino (pins/resolution) -- voir ses
// #define PANEL_RES_X/Y, PANEL_CHAIN, xxx_PIN et son mxconfig au boot.
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

MatrixPanel_I2S_DMA *display = nullptr;

const char *WIFI_SSID = "shan";
const char *WIFI_PASSWORD = "shanankh";
const uint16_t UDP_PORT = 5005;
const unsigned long GAP_THRESHOLD_MS = 15000UL;

WiFiUDP udp;
unsigned long lastPacketMs = 0;
unsigned long gapStartMs = 0;
uint32_t packetCount = 0;

void setup()
{
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println("=== udp_freeze_isolation -- sketch minimal ===");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("[BOOT] connexion WiFi");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000)
  {
    delay(250);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("[BOOT] connecte, IP=" + WiFi.localIP().toString());
  }
  else
  {
    Serial.println("[BOOT] ECHEC connexion WiFi -- reboot dans 5s");
    delay(5000);
    ESP.restart();
  }

  udp.begin(UDP_PORT);
  Serial.println("[BOOT] UDP en ecoute sur le port " + String(UDP_PORT));
  lastPacketMs = millis();

  // v2 -- meme init HUB75 que RecalBox_DMD.ino (memes parametres mxconfig).
  HUB75_I2S_CFG::i2s_pins pins={R1_PIN,G1_PIN,B1_PIN,R2_PIN,G2_PIN,B2_PIN,A_PIN,B_PIN,C_PIN,D_PIN,E_PIN,LAT_PIN,OE_PIN,CLK_PIN};
  HUB75_I2S_CFG mxconfig(PANEL_RES_X,PANEL_RES_Y,PANEL_CHAIN,pins);
  mxconfig.latch_blanking=4; mxconfig.i2sspeed=HUB75_I2S_CFG::HZ_10M;
  mxconfig.min_refresh_rate=60; mxconfig.clkphase=false; mxconfig.double_buff=false;
  display=new MatrixPanel_I2S_DMA(mxconfig);
  display->begin();
  display->setBrightness8(80);
  display->clearScreen();
  Serial.println("[BOOT] HUB75 display initialise");
}

void loop()
{
  int packetSize = udp.parsePacket();
  if (packetSize > 0)
  {
    char buf[64];
    int len = udp.read(buf, sizeof(buf) - 1);
    if (len > 0) buf[len] = '\0'; else buf[0] = '\0';
    packetCount++;
    unsigned long now = millis();

    if (gapStartMs != 0)
    {
      unsigned long dureeMs = now - gapStartMs;
      Serial.println("[UDPGAP] fin coupure, duree=" + String(dureeMs) + "ms");
      gapStartMs = 0;
    }

    lastPacketMs = now;
    Serial.println("[RX] #" + String(packetCount) + " len=" + String(len)
                   + " content=" + String(buf) + " t=" + String(now));

    // reponse PONG:compteur, meme esprit que le vrai firmware
    IPAddress remoteIp = udp.remoteIP();
    uint16_t remotePort = udp.remotePort();
    String reply = "PONG:" + String(packetCount);
    udp.beginPacket(remoteIp, remotePort);
    udp.write((const uint8_t *)reply.c_str(), reply.length());
    udp.endPacket();
  }

  unsigned long now = millis();
  if (gapStartMs == 0 && (now - lastPacketMs) >= GAP_THRESHOLD_MS)
  {
    gapStartMs = lastPacketMs;
    Serial.println("[UDPGAP] debut coupure (dernier paquet vu il y a "
                   + String(now - lastPacketMs) + "ms)");
  }

  // heartbeat minimal, meme esprit que [LOOPDIAG] mais volontairement
  // tres leger -- ce sketch ne doit RIEN faire d'autre que WiFi/UDP.
  static unsigned long lastHeartbeatMs = 0;
  if (now - lastHeartbeatMs >= 5000)
  {
    lastHeartbeatMs = now;
    Serial.println("[HB] t=" + String(now) + " wifiStatus=" + String((int)WiFi.status())
                   + " rssi=" + String(WiFi.RSSI()) + " freeHeap=" + String(ESP.getFreeHeap()));
  }

  // v2 -- rafraichissement HUB75 continu, simple mais REEL (pas juste
  // display->begin() puis rien) -- meme esprit que le firmware principal
  // qui redessine en continu (playlist/GIF), sans dependance SD/GIF ici.
  // Balayage d'un pixel + couleur qui cycle, ~30 fois/s.
  static unsigned long lastDrawMs = 0;
  static int animX = 0;
  static uint8_t animHue = 0;
  if (millis() - lastDrawMs >= 33)
  {
    lastDrawMs = millis();
    display->clearScreen();
    uint16_t color = display->color565(animHue, 255 - animHue, (animHue * 2) % 255);
    for (int y = 0; y < PANEL_RES_Y; y++) display->drawPixel(animX, y, color);
    animX = (animX + 1) % (PANEL_RES_X * PANEL_CHAIN);
    animHue = (animHue + 2) % 255;
  }

  // BUG REEL trouve au 1er flash : boucle sans aucun delay()/yield() ->
  // la tache IDLE0 n'est jamais programmee -> task watchdog declenche un
  // abort()/reboot toutes les ~9.5s. delay(1) suffit (udp.parsePacket()
  // reste non bloquant, aucun impact sur la reactivite mesuree).
  delay(1);
}
