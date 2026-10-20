// log.h -- journal minimal sur stderr : "[  12345 ms] [TAG] message" (les tags reprennent ceux du firmware).
#pragma once
#include <chrono>
#include <cstdarg>
#include <cstdint>
#include <cstdio>

inline uint64_t nowMs() {
  using namespace std::chrono;
  return (uint64_t)duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

inline void logf(const char *tag, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
inline void logf(const char *tag, const char *fmt, ...) {
  char buf[512];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  std::fprintf(stderr, "[%8llu ms] [%s] %s\n", (unsigned long long)nowMs(), tag, buf);
}
