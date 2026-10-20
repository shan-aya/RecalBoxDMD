#include "sdfs.h"

#include <dirent.h>
#include <sys/stat.h>

#include <algorithm>
#include <cctype>
#include <cstdio>

std::string toLower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); });
  return s;
}

std::string trim(const std::string &s) {
  size_t a = 0, b = s.size();
  while (a < b && std::isspace((unsigned char)s[a])) a++;
  while (b > a && std::isspace((unsigned char)s[b - 1])) b--;
  return s.substr(a, b - a);
}

SdFs::SdFs(std::string root) : root_(std::move(root)) {
  while (root_.size() > 1 && root_.back() == '/') root_.pop_back();
}

const SdFs::DirCache &SdFs::dirCache(const std::string &realDir, bool forceRefresh) const {
  auto now = std::chrono::steady_clock::now();
  auto it = dirs_.find(realDir);
  if (it != dirs_.end() && !forceRefresh) return it->second;
  DirCache dc;
  dc.at = now;
  if (DIR *d = opendir(realDir.c_str())) {
    while (dirent *e = readdir(d)) {
      std::string n = e->d_name;
      if (n == "." || n == "..") continue;
      dc.lower2real.emplace(toLower(n), n);  // en cas de doublons de casse, le premier gagne
    }
    closedir(d);
  }
  return dirs_[realDir] = std::move(dc);
}

std::string SdFs::resolve(const std::string &sdPath) const {
  std::string cur = root_;
  size_t i = 0;
  while (i < sdPath.size()) {
    while (i < sdPath.size() && sdPath[i] == '/') i++;
    size_t j = sdPath.find('/', i);
    if (j == std::string::npos) j = sdPath.size();
    std::string comp = sdPath.substr(i, j - i);
    i = j;
    if (comp.empty() || comp == ".") continue;
    if (comp == "..") return "";  // jamais de sortie de la racine
    struct stat st;
    std::string cand = cur + "/" + comp;
    if (stat(cand.c_str(), &st) == 0) {
      cur = cand;
      continue;
    }
    // Pas de correspondance exacte : recherche insensible a la casse (listing en cache, rafraichi une fois si rate)
    std::string low = toLower(comp);
    const DirCache *dc = &dirCache(cur, false);
    auto f = dc->lower2real.find(low);
    if (f == dc->lower2real.end()) {
      // le cache a pu vieillir (fichier ajoute depuis) : un seul rafraichissement, au plus toutes les 2 s
      if (std::chrono::steady_clock::now() - dc->at > std::chrono::seconds(2)) {
        dc = &dirCache(cur, true);
        f = dc->lower2real.find(low);
      }
      if (f == dc->lower2real.end()) return "";
    }
    cur = cur + "/" + f->second;
  }
  return cur;
}

bool SdFs::isDir(const std::string &sdPath) const {
  std::string r = resolve(sdPath);
  struct stat st;
  return !r.empty() && stat(r.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

int64_t SdFs::fileSize(const std::string &sdPath) const {
  std::string r = resolve(sdPath);
  struct stat st;
  if (r.empty() || stat(r.c_str(), &st) != 0 || !S_ISREG(st.st_mode)) return -1;
  return (int64_t)st.st_size;
}

int64_t SdFs::mtime(const std::string &sdPath) const {
  std::string r = resolve(sdPath);
  struct stat st;
  if (r.empty() || stat(r.c_str(), &st) != 0) return 0;
  return (int64_t)st.st_mtime;
}

bool SdFs::readAll(const std::string &sdPath, std::vector<uint8_t> &out, size_t maxBytes) const {
  std::string r = resolve(sdPath);
  if (r.empty()) return false;
  FILE *f = std::fopen(r.c_str(), "rb");
  if (!f) return false;
  std::fseek(f, 0, SEEK_END);
  long sz = std::ftell(f);
  std::fseek(f, 0, SEEK_SET);
  if (sz < 0) {
    std::fclose(f);
    return false;
  }
  size_t want = (maxBytes > 0 && (size_t)sz > maxBytes) ? maxBytes : (size_t)sz;
  out.resize(want);
  size_t got = want ? std::fread(out.data(), 1, want, f) : 0;
  std::fclose(f);
  out.resize(got);
  return got == want;
}

bool SdFs::readAt(const std::string &sdPath, uint64_t off, void *buf, size_t n) const {
  std::string r = resolve(sdPath);
  if (r.empty()) return false;
  FILE *f = std::fopen(r.c_str(), "rb");
  if (!f) return false;
  bool ok = std::fseek(f, (long)off, SEEK_SET) == 0 && std::fread(buf, 1, n, f) == n;
  std::fclose(f);
  return ok;
}

std::vector<std::string> SdFs::list(const std::string &sdDir) const {
  std::vector<std::string> out;
  std::string r = resolve(sdDir);
  if (r.empty()) return out;
  if (DIR *d = opendir(r.c_str())) {
    while (dirent *e = readdir(d)) {
      std::string n = e->d_name;
      if (n != "." && n != "..") out.push_back(n);
    }
    closedir(d);
  }
  return out;
}
