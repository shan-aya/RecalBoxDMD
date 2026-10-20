#include "netutil.h"

#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>

#include "log.h"

std::string runArgs(const std::vector<std::string> &argv, int timeoutMs, int *rc) {
  std::string out;
  if (rc) *rc = -2;
  if (argv.empty()) return out;
  int pfd[2];
  if (pipe(pfd) != 0) return out;
  const pid_t pid = fork();
  if (pid < 0) {
    close(pfd[0]);
    close(pfd[1]);
    return out;
  }
  if (pid == 0) {
    dup2(pfd[1], 1);
    dup2(pfd[1], 2);
    close(pfd[0]);
    close(pfd[1]);
    const int dn = open("/dev/null", O_RDONLY);
    if (dn >= 0) dup2(dn, 0);
    std::vector<char *> av;
    for (const std::string &a : argv) av.push_back(const_cast<char *>(a.c_str()));
    av.push_back(nullptr);
    execvp(av[0], av.data());
    _exit(127);
  }
  close(pfd[1]);
  const auto t0 = std::chrono::steady_clock::now();
  bool killed = false;
  for (;;) {
    int wait = 200;
    if (timeoutMs > 0) {
      const long el = (long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();
      if (el >= timeoutMs) {
        kill(pid, SIGKILL);
        killed = true;
        break;
      }
    }
    pollfd p{pfd[0], POLLIN, 0};
    const int r = poll(&p, 1, wait);
    if (r > 0 && (p.revents & (POLLIN | POLLHUP))) {
      char b[1024];
      const ssize_t n = read(pfd[0], b, sizeof(b));
      if (n <= 0) break;
      out.append(b, (size_t)n);
    }
  }
  close(pfd[0]);
  int st = 0;
  waitpid(pid, &st, 0);
  if (rc) *rc = killed ? -1 : (WIFEXITED(st) ? WEXITSTATUS(st) : -2);
  return out;
}

std::string runShell(const std::string &cmd) {
  std::string out;
  FILE *p = popen(cmd.c_str(), "r");
  if (!p) return out;
  char b[512];
  size_t n;
  while ((n = fread(b, 1, sizeof(b), p)) > 0) out.append(b, n);
  pclose(p);
  return out;
}

NetInfo netInfo() {
  NetInfo ni;
  const std::string route = runShell("ip -4 route show default 2>/dev/null");
  {
    std::istringstream in(route);
    std::string w, prev;
    while (in >> w) {
      if (prev == "via") ni.gw = w;
      if (prev == "dev") ni.dev = w;
      prev = w;
    }
  }
  if (!ni.dev.empty()) {
    const std::string addr = runShell("ip -4 -o addr show dev " + ni.dev + " 2>/dev/null");
    const size_t p = addr.find("inet ");
    if (p != std::string::npos) {
      const std::string cidr = addr.substr(p + 5, addr.find(' ', p + 5) - p - 5);
      const size_t sl = cidr.find('/');
      if (sl != std::string::npos) {
        ni.ip = cidr.substr(0, sl);
        const int bits = std::atoi(cidr.c_str() + sl + 1);
        const uint32_t m = bits <= 0 ? 0 : (bits >= 32 ? 0xFFFFFFFFu : (0xFFFFFFFFu << (32 - bits)));
        char b[20];
        std::snprintf(b, sizeof(b), "%u.%u.%u.%u", (m >> 24) & 255, (m >> 16) & 255, (m >> 8) & 255, m & 255);
        ni.mask = b;
      }
    }
  }
  std::ifstream rc("/etc/resolv.conf");
  std::string line;
  while (std::getline(rc, line))
    if (line.compare(0, 11, "nameserver ") == 0) {
      ni.dns = line.substr(11);
      while (!ni.dns.empty() && (ni.dns.back() == ' ' || ni.dns.back() == '\r')) ni.dns.pop_back();
      break;
    }
  return ni;
}

bool hasDefaultRoute() { return !runShell("ip -4 route show default 2>/dev/null").empty(); }

std::string i18nLang(const std::string &lang) { return (lang == "en" || lang == "es") ? lang : "fr"; }
