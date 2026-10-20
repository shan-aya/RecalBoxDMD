// score.cpp -- portage de drawScoreScreen() (RecalBox_DMD.ino v110-v157). Les couleurs, positions et regles de coupe sont
// celles du firmware, validees a l'oeil par l'utilisateur au fil des versions : ne rien « ameliorer » ici sans le dire.
#include "score.h"

#include <cctype>

#include "TomThumb.h"

namespace {
const int RAW565_W = DMD_W;  // 128

void drawShadowed(Display &d, int x, int y, const std::string &s, uint16_t main) {
  // ombre portee noire d'1 px derriere le texte (lisibilite sur fond de LED)
  d.setTextColor(Display::color565(0, 0, 0));
  d.setCursor((int16_t)(x + 1), (int16_t)(y + 1));
  d.print(s);
  d.setTextColor(main);
  d.setCursor((int16_t)x, (int16_t)y);
  d.print(s);
}

// avance reelle (xAdvance) d'un caractere dans TomThumb ; 4 px hors de la plage 0x20-0x7E
uint8_t tomThumbCharAdvance(char c) {
  const uint8_t uc = (uint8_t)c;
  if (uc < 0x20 || uc > 0x7E) return 4;
  return TomThumbGlyphs[uc - 0x20].xAdvance;
}

std::string upper(std::string s) {
  for (char &c : s) c = (char)std::toupper((unsigned char)c);
  return s;
}
}  // namespace

void drawScoreScreen(Display &d, const std::string &payload) {
  d.clearScreen();
  d.setTextWrap(false);
  d.setFont(nullptr);  // police classique 5x7 au depart, quel que soit l'etat laisse par l'appel precedent
  const uint16_t gold = Display::color565(255, 200, 0);        // valeurs / scores
  const uint16_t white = Display::color565(235, 235, 235);     // libelles / noms / texte
  const uint16_t titleColor = Display::color565(90, 90, 255);  // titres (bleu-violet sature)
  const uint16_t rank1Color = Display::color565(255, 90, 40);  // rang 1 du hi-score

  const size_t firstSep = payload.find('|');
  const std::string title = (firstSep == std::string::npos) ? payload : payload.substr(0, firstSep);
  const std::string rest = (firstSep == std::string::npos) ? std::string() : payload.substr(firstSep + 1);

  // --- page 1 du hi-score : titre, rang 1 en TAILLE 2 sur une ligne, rang 2 en taille normale (8+16+8 = 32 px) ---
  if (title == "HI-SCORE" && rest.compare(0, 2, "1 ") == 0) {
    int y = 0;
    d.setTextSize(1);
    {
      int tx = (RAW565_W - (int)title.size() * 6) / 2;
      if (tx < 0) tx = 0;
      drawShadowed(d, tx, y, title, titleColor);
    }
    y += 8;
    size_t start = 0;
    int rankIdx = 0;
    while (start <= rest.size() && rankIdx < 2) {
      const size_t sep = rest.find('|', start);
      const std::string line = (sep == std::string::npos) ? rest.substr(start) : rest.substr(start, sep - start);
      const size_t sp = line.rfind(' ');
      const std::string name = (sp != std::string::npos && sp > 0) ? line.substr(0, sp) : line;
      const std::string scoreVal = (sp != std::string::npos && sp > 0) ? line.substr(sp + 1) : std::string();
      if (rankIdx == 0) {
        std::string bigName = name;
        const size_t firstSpace = name.find(' ');
        if (firstSpace != std::string::npos && firstSpace > 0) bigName = name.substr(firstSpace + 1);  // retire le numero de rang
        d.setTextSize(2);
        drawShadowed(d, 1, y, bigName, white);
        int sx = RAW565_W - (int)scoreVal.size() * 12 - 1;  // score aligne a droite
        const int nameEnd = 1 + (int)bigName.size() * 12 + 6;
        if (sx < nameEnd) sx = nameEnd;
        drawShadowed(d, sx, y, scoreVal, rank1Color);
        y += 16;
      } else {
        d.setTextSize(1);
        drawShadowed(d, 1, y, name, white);
        if (!scoreVal.empty()) {
          int sx = RAW565_W - (int)scoreVal.size() * 6 - 1;
          const int nameEnd = 1 + (int)name.size() * 6 + 4;
          if (sx < nameEnd) sx = nameEnd;
          drawShadowed(d, sx, y, scoreVal, gold);
        }
        y += 8;
      }
      rankIdx++;
      if (sep == std::string::npos) break;
      start = sep + 1;
    }
    return;
  }

  // --- rendu generique : jusqu'a 4 lignes de 8 px ---
  d.setTextSize(1);
  int y = 0;
  size_t start = 0;
  int lineIdx = 0;
  const int maxLines = 4;
  const bool isInfoScreen = (title == "INFOS");
  const bool isDescriptionScreen = (title == "DESCRIPTION");
  while (start <= payload.size() && lineIdx < maxLines) {
    const size_t sep = payload.find('|', start);
    const std::string line = (sep == std::string::npos) ? payload.substr(start) : payload.substr(start, sep - start);
    if (lineIdx == 0) {
      // titre centre ; chaine vide (page de continuation) : on consomme juste la ligne
      if (!line.empty()) {
        int tx = (RAW565_W - (int)line.size() * 6) / 2;
        if (tx < 0) tx = 0;
        drawShadowed(d, tx, y, line, titleColor);
      }
    } else if (isInfoScreen) {
      // libelle (avec « : ») en blanc, valeur en or
      const size_t cp = line.find(':');
      if (cp != std::string::npos && cp > 0) {
        const std::string label = line.substr(0, cp + 1);
        const std::string value = line.substr(cp + 1);
        drawShadowed(d, 1, y, label, white);
        drawShadowed(d, 1 + (int)label.size() * 6 + 4, y, value, gold);
      } else {
        drawShadowed(d, 1, y, line, white);
      }
    } else if (isDescriptionScreen) {
      // texte libre en TomThumb, MAJUSCULES, or, centre ; espacement = avance native de la police ; police GFX = ligne de base (+5)
      d.setFont(&TomThumb);
      const std::string up = upper(line);
      int lineWidth = 0;
      for (char ch : up) lineWidth += tomThumbCharAdvance(ch);
      int cx = (RAW565_W - lineWidth) / 2;
      if (cx < 0) cx = 0;
      for (char ch : up) {
        drawShadowed(d, cx, y + 5, std::string(1, ch), gold);
        cx += tomThumbCharAdvance(ch);
      }
      d.setFont(nullptr);
    } else {
      // rang : coupe sur le DERNIER espace, nom en blanc a gauche, score en or ALIGNE A DROITE
      const size_t sp = line.rfind(' ');
      if (sp != std::string::npos && sp > 0) {
        const std::string name = line.substr(0, sp);
        const std::string scoreVal = line.substr(sp + 1);
        drawShadowed(d, 1, y, name, white);
        int sx = RAW565_W - (int)scoreVal.size() * 6 - 1;
        const int nameEnd = 1 + (int)name.size() * 6 + 4;
        if (sx < nameEnd) sx = nameEnd;
        drawShadowed(d, sx, y, scoreVal, gold);
      } else {
        drawShadowed(d, 1, y, line, white);
      }
    }
    y += 8;
    lineIdx++;
    if (sep == std::string::npos) break;
    start = sep + 1;
  }
}
