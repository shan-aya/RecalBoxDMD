// pngdecode.h -- decodage PNG (libpng) pour le repli de drawPng() : meme comportement que pngle dans le firmware.
// Le firmware dessine chaque pixel tel quel (RGB, canal alpha IGNORE) aux coordonnees x,y de l'image, sans aucune mise
// a l'echelle : ce qui depasse 128x32 est rogne (coin haut-gauche conserve). Sur Pi 3B : ~1,6 ms pour un 128x32.
#pragma once
#include <cstdint>
#include <vector>

// Decode `file` (contenu d'un .png) en RGBA 8 bits ; false si ce n'est pas un PNG valide.
bool decodePngRgba(const std::vector<uint8_t> &file, std::vector<uint8_t> &rgba, int &w, int &h);
