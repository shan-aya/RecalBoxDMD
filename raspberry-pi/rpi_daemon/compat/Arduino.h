// compat/Arduino.h -- couche de compatibilite MINIMALE pour compiler tels quels, sur le Pi, des fichiers du firmware
// (clock_themes.h, themes d'horloge et widgets extraits du .ino). Seuls les identifiants reellement utilises y figurent.
// Le dossier compat/ n'est mis sur le chemin d'inclusion que pour les fichiers qui en ont besoin (voir CMakeLists.txt).
#pragma once
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <random>
#include <string>
#include <type_traits>
#include <vector>

#include "display.h"
#include "log.h"

#define PROGMEM
#define pgm_read_byte(addr) (*(const uint8_t *)(addr))
#define pgm_read_word(addr) (*(const uint16_t *)(addr))
#define memcpy_P memcpy
#define strlen_P strlen

// millis() : 32 bits non signe comme sur l'ESP32 (le code du firmware compare des differences d'entiers non signes)
inline unsigned long millis() { return (unsigned long)(uint32_t)nowMs(); }

inline std::mt19937 &arduinoRng() {
  static std::mt19937 rng{std::random_device{}()};
  return rng;
}
// random(a, b) d'Arduino : entier long dans [a, b[
inline long random(long a, long b) {
  if (b <= a) return a;
  return a + (long)(arduinoRng()() % (uint32_t)(b - a));
}
inline long random(long b) { return random(0, b); }

// attente : definie par le code qui l'utilise (elle presente aussi l'image en cours) ; yield() ne fait rien (le Pi n'a pas de FreeRTOS a nourrir)
void delay(unsigned long ms);
inline void yield() {}

template <class A, class B>
inline typename std::common_type<A, B>::type min(A a, B b) {
  typedef typename std::common_type<A, B>::type T;
  return ((T)b < (T)a) ? (T)b : (T)a;
}
template <class A, class B>
inline typename std::common_type<A, B>::type max(A a, B b) {
  typedef typename std::common_type<A, B>::type T;
  return ((T)a < (T)b) ? (T)b : (T)a;
}

// ----------------------------------------------------------------------------------------------------------------- String
// Sous-ensemble de String d'Arduino (semantique identique : indexOf -1 si absent, toInt tolerant, substring borne...).
class String {
 public:
  String() {}
  String(const char *s) : s_(s ? s : "") {}
  String(const std::string &s) : s_(s) {}
  String(char c) : s_(1, c) {}
  String(int v) : s_(std::to_string(v)) {}
  String(unsigned v) : s_(std::to_string(v)) {}
  String(long v) : s_(std::to_string(v)) {}
  String(unsigned long v) : s_(std::to_string(v)) {}
  String(double v, unsigned dec = 2) { char b[64]; std::snprintf(b, sizeof(b), "%.*f", (int)dec, v); s_ = b; }
  String(float v, unsigned dec = 2) { char b[64]; std::snprintf(b, sizeof(b), "%.*f", (int)dec, (double)v); s_ = b; }

  operator std::string() const { return s_; }
  unsigned length() const { return (unsigned)s_.size(); }
  const char *c_str() const { return s_.c_str(); }
  bool reserve(unsigned) { return true; }
  char charAt(unsigned i) const { return i < s_.size() ? s_[i] : 0; }
  char operator[](unsigned i) const { return charAt(i); }
  char &operator[](unsigned i) { return s_[i]; }
  String substring(unsigned from, unsigned to = 0xFFFFFFFFu) const {
    if (to == 0xFFFFFFFFu || to > s_.size()) to = (unsigned)s_.size();
    if (from > to) { unsigned t = from; from = to; to = t; }
    if (from >= s_.size()) return String();
    return String(s_.substr(from, to - from));
  }
  int indexOf(char c, unsigned from = 0) const { size_t p = s_.find(c, from); return p == std::string::npos ? -1 : (int)p; }
  int indexOf(const String &t, unsigned from = 0) const { size_t p = s_.find(t.s_, from); return p == std::string::npos ? -1 : (int)p; }
  int lastIndexOf(char c) const { size_t p = s_.rfind(c); return p == std::string::npos ? -1 : (int)p; }
  int lastIndexOf(char c, unsigned from) const { size_t p = s_.rfind(c, from); return p == std::string::npos ? -1 : (int)p; }
  bool startsWith(const String &t) const { return s_.compare(0, t.s_.size(), t.s_) == 0; }
  bool endsWith(const String &t) const { return s_.size() >= t.s_.size() && s_.compare(s_.size() - t.s_.size(), t.s_.size(), t.s_) == 0; }
  void trim() {
    size_t a = s_.find_first_not_of(" \t\r\n"), b = s_.find_last_not_of(" \t\r\n");
    s_ = (a == std::string::npos) ? std::string() : s_.substr(a, b - a + 1);
  }
  void toLowerCase() { for (char &c : s_) if (c >= 'A' && c <= 'Z') c = (char)(c + 32); }
  void toUpperCase() { for (char &c : s_) if (c >= 'a' && c <= 'z') c = (char)(c - 32); }
  long toInt() const { return std::strtol(s_.c_str(), nullptr, 10); }
  float toFloat() const { return (float)std::strtod(s_.c_str(), nullptr); }
  double toDouble() const { return std::strtod(s_.c_str(), nullptr); }
  void replace(const String &from, const String &to) {
    if (from.s_.empty()) return;
    size_t p = 0;
    while ((p = s_.find(from.s_, p)) != std::string::npos) { s_.replace(p, from.s_.size(), to.s_); p += to.s_.size(); }
  }
  void remove(unsigned idx, unsigned count = 0xFFFFFFFFu) { if (idx < s_.size()) s_.erase(idx, count); }
  bool concat(const String &t) { s_ += t.s_; return true; }
  bool equals(const String &t) const { return s_ == t.s_; }
  bool equalsIgnoreCase(const String &t) const {
    if (s_.size() != t.s_.size()) return false;
    for (size_t i = 0; i < s_.size(); i++) if (std::tolower((unsigned char)s_[i]) != std::tolower((unsigned char)t.s_[i])) return false;
    return true;
  }
  int compareTo(const String &t) const { return s_.compare(t.s_); }
  String &operator+=(const String &t) { s_ += t.s_; return *this; }
  String &operator+=(char c) { s_ += c; return *this; }
  bool operator==(const String &t) const { return s_ == t.s_; }
  bool operator==(const char *t) const { return s_ == (t ? t : ""); }
  bool operator!=(const String &t) const { return s_ != t.s_; }
  bool operator!=(const char *t) const { return s_ != (t ? t : ""); }
  bool operator<(const String &t) const { return s_ < t.s_; }
  const std::string &str() const { return s_; }

 private:
  std::string s_;
};
inline String operator+(const String &a, const String &b) { return String(a.str() + b.str()); }
inline String operator+(const String &a, const char *b) { return String(a.str() + (b ? b : "")); }
inline String operator+(const char *a, const String &b) { return String(std::string(a ? a : "") + b.str()); }
template <class T, class = typename std::enable_if<std::is_arithmetic<T>::value>::type>
inline String operator+(const String &a, T v) { return a + String(v); }

// ----------------------------------------------------------------------------------------------------------------- Serial / ESP / WiFi
// Les lignes du firmware portent deja leur etiquette (« [DECO] ... ») : on les journalise telles quelles.
struct SerialClass {
  void println() {}
  void println(const String &s) { logf("FW", "%s", s.c_str()); }
  void println(const char *s) { logf("FW", "%s", s); }
  void print(const String &s) { logf("FW", "%s", s.c_str()); }
  void printf(const char *fmt, ...) __attribute__((format(printf, 2, 3))) {
    char b[400];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(b, sizeof(b), fmt, ap);
    va_end(ap);
    logf("FW", "%s", b);
  }
};
inline SerialClass Serial;

// Pas de tas a economiser sur le Pi : les gardes « assez de memoire ? » du firmware passent toujours.
struct EspClass {
  uint32_t getMaxAllocHeap() const { return 16u << 20; }
  uint32_t getFreeHeap() const { return 16u << 20; }
};
inline EspClass ESP;

enum { WL_CONNECTED = 3 };
struct WiFiClass {
  int status() const { return WL_CONNECTED; }  // la connectivite reelle est testee par connect()
};
inline WiFiClass WiFi;

// Client TCP minimal (sockets POSIX) : connect avec delai, print, available/read, connected, stop.
class WiFiClient {
 public:
  ~WiFiClient() { stop(); }
  bool connect(const char *host, uint16_t port, int timeoutMs = 3000) {
    stop();
    addrinfo hints{}, *res = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host, std::to_string(port).c_str(), &hints, &res) != 0 || !res) return false;
    fd_ = ::socket(res->ai_family, res->ai_socktype, 0);
    if (fd_ < 0) { freeaddrinfo(res); return false; }
    ::fcntl(fd_, F_SETFL, ::fcntl(fd_, F_GETFL, 0) | O_NONBLOCK);
    const int r = ::connect(fd_, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);
    if (r != 0) {
      pollfd p{fd_, POLLOUT, 0};
      if (poll(&p, 1, timeoutMs) <= 0) { stop(); return false; }
      int err = 0;
      socklen_t el = sizeof(err);
      getsockopt(fd_, SOL_SOCKET, SO_ERROR, &err, &el);
      if (err != 0) { stop(); return false; }
    }
    return true;
  }
  size_t print(const char *s) {
    if (fd_ < 0) return 0;
    const ssize_t w = ::send(fd_, s, std::strlen(s), MSG_NOSIGNAL);
    return w > 0 ? (size_t)w : 0;
  }
  int available() {
    if (fd_ < 0) return 0;
    int n = 0;
    ioctl(fd_, FIONREAD, &n);
    return n;
  }
  bool connected() {
    if (fd_ < 0) return false;
    pollfd p{fd_, POLLIN, 0};
    if (poll(&p, 1, 0) > 0 && (p.revents & (POLLIN | POLLHUP | POLLERR))) {
      if (available() == 0) { stop(); return false; }  // lisible mais 0 octet = le serveur a ferme
    }
    return true;
  }
  int read(uint8_t *buf, size_t len) { return fd_ < 0 ? -1 : (int)::recv(fd_, buf, len, MSG_DONTWAIT); }
  void stop() {
    if (fd_ >= 0) { ::close(fd_); fd_ = -1; }
  }

 private:
  int fd_ = -1;
};

// ----------------------------------------------------------------------------------------------------------------- canevas
// Adafruit_GFX / GFXcanvas du firmware : tout est un Display (meme API de dessin). Un canevas est un Display sans backend.
using Adafruit_GFX = Display;

// canevas 1 bit : getBuffer() renvoie un tampon VIVANT (1 bit par pixel, MSB en premier, lignes de (largeur+7)/8 octets), comme celui d'Adafruit :
// le firmware l'obtient AVANT de dessiner. Le dessin se fait dans un Display hors ecran, recopie dans le tampon apres chaque operation.
class GFXcanvas1 {
 public:
  GFXcanvas1(int w, int h) : w_(w), h_(h), cv_(w, h), bits_((size_t)((w + 7) / 8) * (size_t)h, 0) {}
  const uint8_t *getBuffer() const { return bits_.data(); }
  void fillScreen(uint16_t c) { cv_.fillScreen(c); sync(); }
  void setTextWrap(bool w) { cv_.setTextWrap(w); }
  void setTextSize(uint8_t s) { cv_.setTextSize(s); }
  void setTextColor(uint16_t c) { cv_.setTextColor(c); }
  void setCursor(int16_t x, int16_t y) { cv_.setCursor(x, y); }
  void setFont(const GFXfont *f) { cv_.setFont(f); }
  size_t print(const char *s) { const size_t n = cv_.print(s); sync(); return n; }

 private:
  void sync() {
    const int rb = (w_ + 7) / 8;
    std::fill(bits_.begin(), bits_.end(), 0);
    const uint16_t *fb = cv_.framebuffer();
    for (int y = 0; y < h_; y++)
      for (int x = 0; x < w_; x++)
        if (fb[(size_t)y * w_ + x]) bits_[(size_t)y * rb + (x >> 3)] |= (uint8_t)(0x80 >> (x & 7));
  }
  int w_, h_;
  Display cv_;
  std::vector<uint8_t> bits_;
};