// score.h -- ecran de score plein cadre (CMD=score) : portage de drawScoreScreen() du firmware.
// Payload = lignes separees par '|' (4 lignes de 8 px maximum) : « HI-SCORE|1 MAA 283200|2 CAP 30000 »,
// « INFOS|Dev: ...|Editeur: ... », « DESCRIPTION|... » (police compacte TomThumb, majuscules, centre), « SUCCES|nom », etc.
// Dessine dans le framebuffer (efface d'abord l'ecran) ; ne touche a aucun etat d'animation.
#pragma once
#include <string>

#include "display.h"

void drawScoreScreen(Display &d, const std::string &payload);
