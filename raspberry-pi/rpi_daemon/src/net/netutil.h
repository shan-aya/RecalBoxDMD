// netutil.h -- outils reseau du demon : lancer une commande (sans shell quand des donnees viennent du reseau) et lire la connexion actuelle.
#pragma once
#include <string>
#include <vector>

// Lance argv[0] avec ses arguments (execvp : AUCUN shell, donc pas d'injection par un SSID ou un mot de passe). Renvoie stdout+stderr ;
// rc = code de sortie (-1 : delai depasse, processus tue). timeoutMs <= 0 : sans limite.
std::string runArgs(const std::vector<std::string> &argv, int timeoutMs = 20000, int *rc = nullptr);
// commande de lecture via le shell (chaine fixe, sans donnee exterieure)
std::string runShell(const std::string &cmd);

struct NetInfo {
  std::string dev, ip, gw, mask, dns;  // connexion par defaut du Pi (route par defaut) ; vides si aucune
};
NetInfo netInfo();
bool hasDefaultRoute();  // une connectivite IP existe-t-elle (Wi-Fi ou Ethernet) ?

// Lien Wi-Fi courant (mode diagnostic) : `iw dev <iface> link` (SSID, BSSID, frequence, signal), repli /proc/net/wireless pour le seul signal.
struct WifiLink {
  bool up = false;
  std::string iface, ssid, bssid;
  int freq = 0, channel = 0, rssi = -127;
};
WifiLink wifiLink();
long memAvailableBytes();  // MemAvailable de /proc/meminfo (octets), -1 si illisible

std::string i18nLang(const std::string &lang);  // « fr » / « en » / « es », defaut « fr »
