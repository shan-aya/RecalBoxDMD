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

std::string i18nLang(const std::string &lang);  // « fr » / « en » / « es », defaut « fr »
