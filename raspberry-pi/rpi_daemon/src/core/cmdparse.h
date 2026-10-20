// cmdparse.h -- analyse d'un message « CMD=<nom> ARG=<reste> » (extractField + indexOf("ARG=") du firmware) :
// CMD = mot simple jusqu'au prochain espace ; ARG = tout ce qui suit le premier « ARG= » jusqu'a la fin (espaces et
// '|' compris).
#pragma once
#include <string>

#include "sdfs.h"

struct ParsedCmd {
  std::string cmd, arg;
};

inline ParsedCmd parseCommand(const std::string &raw) {
  const std::string msg = trim(raw);
  ParsedCmd p;
  size_t idx = msg.find("CMD=");
  if (idx != std::string::npos) {
    size_t start = idx + 4;
    size_t end = msg.find(' ', start);
    if (end == std::string::npos) end = msg.size();
    p.cmd = msg.substr(start, end - start);
  }
  size_t a = msg.find("ARG=");
  if (a != std::string::npos) p.arg = msg.substr(a + 4);
  return p;
}
