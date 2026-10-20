// widgets.h -- widgets du decor (portage du firmware v266-v353) : date, ephemeride, compte a rebours, ambiances, meteo, citations,
// widgets Recalbox. Le code d'affichage vient du firmware, extrait tel quel (tools/port_widgets.py -> widgets_fw.inc) ; ici : les
// reglages (config.ini), l'interruption et l'entretien du reseau pendant qu'un widget tourne.
//
// Un widget est BLOQUANT (comme dans le firmware : une boucle d'animation de decoDur[id] secondes). Il s'arrete des qu'une commande
// attend dans la file (hook `commandPending`) et entretient la presence UDP pendant ce temps (hook `service`).
#pragma once
#include <cstdint>
#include <functional>
#include <string>

#include "config.h"
#include "display.h"
#include "sdfs.h"

class Widgets {
 public:
  void attach(Display &d, const SdFs &fs);  // la SD : citations (/widgets/quotes/<langue>/*.txt) et fonds d'image (/widgets/quotes/bg/)
  // Reglages : lieu (DECO_LAT/LON/PLACE), evenements du compte a rebours (DECO_EV_n), ambiances (DECO_AMB, DECO_AMBC_n_a/b, DECO_LAVA_*),
  // theme de citations (DECO_QTHEME) ; langue = `language`. `dur` : duree en secondes de chaque widget (index = identifiant).
  void configure(const Config &cfg, const uint8_t dur[11]);

  // CMD=rbw : donnees des widgets Recalbox (challenge C, recompenses R, sante H), appelee par la boucle principale
  void feedRbw(const std::string &arg);

  // Citations : ecrit les themes d'origine de la langue (« fr », « en », « es ») sur la SD ; nombre de fichiers, -1 si langue inconnue
  int restoreQuotes(const std::string &lang);

  void setHooks(std::function<bool()> commandPending, std::function<void()> service);

  // Outils de test (CMD=widget_preview) : duree d'un widget et style d'ambiance forces.
  void setDuration(int id, int sec);
  void setAmbStyle(int style);

  // Apercu depuis la page web : « <id>[:<style>[:<duree>[:<themes>[:<couleur a>[:<couleur b>]]]]][@lat,lon] » ; reglages TEMPORAIRES (jamais gardes), bloquant.
  void previewSpec(const std::string &spec);

  // Joue le widget `id` (1..10). false = interrompu (la sequence doit s'arreter) ; true = termine, ou saute faute de donnees.
  bool run(int id, bool preview = false);
};
