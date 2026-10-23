// bootanim.h -- animation de demarrage du demon (parite avec playBootAnimation() du firmware ESP32, v361/v367).
#pragma once
#include <functional>
#include <string>

#include "config.h"
#include "display.h"
#include "sdfs.h"

// Joue l'animation de demarrage : 6,5 s, 25 images/s, titre « RecalBoxDMD » + ligne `edition` de 2 s a 4,5 s.
// Fichier choisi comme sur l'ESP32 : /gifs/_boot/boot_<fete>.raw565pack si la date tombe dans la fenetre d'une fete (boot_seasons.h),
// sinon /gifs/_boot/boot_anim.raw565pack, sinon /boot_anim.raw565pack ; absent ou invalide = rien (le demon demarre sans animation).
// Date : cle « fake_date=AAAAMMJJ » de config.ini (essais) > horloge systeme si elle est valide > cle « last_date= ».
// `stop` : arret immediat demande (SIGTERM). Renvoie true si l'animation a ete jouee.
bool playBootAnimation(Display &d, const SdFs &fs, const Config &cfg, const std::string &edition, const std::function<bool()> &stop);