#include "playlist.h"

#include <sstream>

#include "log.h"

void Playlist::configure(const std::string &name, bool random) {
  if (name != name_) {
    name_ = name;
    lines_.clear();
    mtime_ = -1;
    seq_ = 0;
    lastRandom_ = -1;
  }
  random_ = random;
}

void Playlist::reloadIfChanged(const SdFs &fs) {
  if (name_.empty()) {
    lines_.clear();
    return;
  }
  const std::string path = "/playlists/" + name_;
  int64_t mt = fs.mtime(path);
  if (mt == mtime_ && !lines_.empty()) return;
  mtime_ = mt;
  lines_.clear();
  std::vector<uint8_t> data;
  if (!fs.readAll(path, data)) {
    logf("PLAYLIST", "%s absente", path.c_str());
    return;
  }
  std::istringstream in(std::string(data.begin(), data.end()));
  std::string line;
  while (std::getline(in, line)) {
    line = trim(line);
    // isValidPlaylistLine : non vide, ni '#' ni ';', et commence par '/'
    if (!line.empty() && line[0] == '/') lines_.push_back(line);
  }
  if (seq_ >= lines_.size()) seq_ = 0;
  logf("PLAYLIST", "%s : %zu lignes valides", path.c_str(), lines_.size());
}

int Playlist::count(const SdFs &fs) {
  reloadIfChanged(fs);
  return (int)lines_.size();
}

std::string Playlist::next(const SdFs &fs) {
  reloadIfChanged(fs);
  if (lines_.empty()) return "";
  if (random_) {
    int n = (int)lines_.size(), idx = lastRandom_;
    if (n > 1) {
      std::uniform_int_distribution<int> dist(0, n - 1);
      for (int t = 0; idx == lastRandom_ && t < 10; t++) idx = dist(rng_);
    } else {
      idx = 0;
    }
    lastRandom_ = idx;
    return lines_[idx];
  }
  if (seq_ >= lines_.size()) seq_ = 0;
  return lines_[seq_++];
}
