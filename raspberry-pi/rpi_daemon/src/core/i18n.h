// i18n.h -- texte a trois langues (fr / en / es) selon la cle « language » de config.ini (Config::uiLanguage). v3.75 : ecrans du DMD et messages du serveur web.
#pragma once
#include <string>

inline const char *L3(const std::string &lang, const char *fr, const char *en, const char *es) { return lang == "en" ? en : lang == "es" ? es : fr; }
