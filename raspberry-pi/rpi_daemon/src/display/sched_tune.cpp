#include "sched_tune.h"

#include <dirent.h>
#include <sched.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "log.h"

namespace {
constexpr int DISPLAY_PRIO = 90;  // thread de rafraichissement (hzeller le met a 99)
constexpr int KERNEL_PRIO = 91;   // taches du noyau du coeur 3 : toujours AU-DESSUS de l'affichage
constexpr int DISPLAY_CPU = 3;

struct Task {
  int tid;
  int pid;
  int ppid;
  std::string comm;
};

bool readFile(const std::string &path, std::string &out) {
  FILE *f = std::fopen(path.c_str(), "r");
  if (!f) return false;
  char buf[512];
  size_t n = std::fread(buf, 1, sizeof(buf) - 1, f);
  std::fclose(f);
  buf[n] = 0;
  out = buf;
  return true;
}

// ppid d'un processus depuis /proc/<pid>/stat : "pid (comm) S ppid ..." (comm peut contenir espaces et parentheses)
int readPpid(int pid) {
  std::string s;
  if (!readFile("/proc/" + std::to_string(pid) + "/stat", s)) return -1;
  size_t p = s.rfind(')');
  if (p == std::string::npos || p + 4 > s.size()) return -1;
  int ppid = -1;
  char st;
  if (std::sscanf(s.c_str() + p + 1, " %c %d", &st, &ppid) != 2) return -1;
  return ppid;
}

std::vector<Task> listTasks() {
  std::vector<Task> v;
  DIR *d = opendir("/proc");
  if (!d) return v;
  while (dirent *e = readdir(d)) {
    if (e->d_name[0] < '0' || e->d_name[0] > '9') continue;
    int pid = std::atoi(e->d_name);
    int ppid = readPpid(pid);
    std::string tdir = std::string("/proc/") + e->d_name + "/task";
    if (DIR *t = opendir(tdir.c_str())) {
      while (dirent *te = readdir(t)) {
        if (te->d_name[0] < '0' || te->d_name[0] > '9') continue;
        std::string comm;
        if (!readFile(tdir + "/" + te->d_name + "/comm", comm)) continue;
        while (!comm.empty() && (comm.back() == '\n' || comm.back() == '\r')) comm.pop_back();
        v.push_back({std::atoi(te->d_name), pid, ppid, comm});
      }
      closedir(t);
    }
  }
  closedir(d);
  return v;
}

bool isCore3Kernel(const std::string &c) { return c.rfind("kworker/3:", 0) == 0 || c == "ksoftirqd/3"; }
bool isIrqThread(const std::string &c) { return c.rfind("irq/", 0) == 0; }

bool setFifo(int tid, int prio) {
  sched_param sp{};
  sp.sched_priority = prio;
  return sched_setscheduler(tid, SCHED_FIFO, &sp) == 0;
}
}  // namespace

struct SchedTune::Impl {
  std::thread th;
  std::atomic<bool> stop{false};
  std::unordered_map<int, std::pair<int, int>> boosted;  // tid -> (politique, priorite) d'origine, pour la restauration
  std::unordered_set<int> pinned;       // tids ecartes du coeur 3
  int displayTid = -1;
  int nBoost = 0, nPin = 0;

  void applyOnce(bool first) {
    const pid_t self = getpid();
    cpu_set_t away;
    CPU_ZERO(&away);
    for (int c = 0; c < 4; c++)
      if (c != DISPLAY_CPU) CPU_SET(c, &away);

    for (const Task &t : listTasks()) {
      // 1) thread de rafraichissement : le seul thread FIFO 99 de NOTRE processus
      if (t.pid == self && displayTid < 0) {
        sched_param sp{};
        if (sched_getscheduler(t.tid) == SCHED_FIFO && sched_getparam(t.tid, &sp) == 0 && sp.sched_priority >= 90) {
          if (setFifo(t.tid, DISPLAY_PRIO)) {
            displayTid = t.tid;
            logf("SCHED", "thread de rafraichissement %d : FIFO %d -> %d", t.tid, sp.sched_priority, DISPLAY_PRIO);
          }
          continue;
        }
      }
      // 2) taches du noyau (ppid 2) : hors coeur 3 si possible
      if ((t.ppid == 2 || t.pid == 2) && !pinned.count(t.tid)) {
        if (sched_setaffinity(t.tid, sizeof(away), &away) == 0) {
          pinned.insert(t.tid);
          nPin++;
        }
      }
      // 3) taches du noyau du coeur 3 et threads d'interruption : AU-DESSUS de l'affichage
      if ((isCore3Kernel(t.comm) || isIrqThread(t.comm)) && !boosted.count(t.tid)) {
        const int oldPol = sched_getscheduler(t.tid);
        sched_param old{};
        sched_getparam(t.tid, &old);
        if (setFifo(t.tid, KERNEL_PRIO)) {
          boosted[t.tid] = {oldPol < 0 ? SCHED_OTHER : oldPol, old.sched_priority};
          nBoost++;
        }
      }
    }
    if (first) logf("SCHED", "%d threads du noyau ecartes du coeur 3, %d passes en FIFO %d", nPin, nBoost, KERNEL_PRIO);
  }
};

void SchedTune::start() {
  if (impl_) return;
  if (geteuid() != 0) {
    logf("SCHED", "pas root : reglage d'ordonnancement ignore (le WiFi risque de se figer pendant l'affichage)");
    return;
  }
  impl_ = new Impl();
  impl_->applyOnce(true);
  Impl *im = impl_;
  im->th = std::thread([im] {
    // thread de fond sur les coeurs 0-2 (jamais sur le coeur d'affichage)
    cpu_set_t away;
    CPU_ZERO(&away);
    for (int c = 0; c < 4; c++)
      if (c != DISPLAY_CPU) CPU_SET(c, &away);
    sched_setaffinity(0, sizeof(away), &away);
    while (!im->stop) {
      for (int i = 0; i < 20 && !im->stop; i++) std::this_thread::sleep_for(std::chrono::milliseconds(100));
      if (!im->stop) im->applyOnce(false);  // de nouveaux kworker/3:N apparaissent a la demande
    }
  });
}

void SchedTune::stop() {
  if (!impl_) return;
  impl_->stop = true;
  if (impl_->th.joinable()) impl_->th.join();
  // retour a l'etat normal : les taches du noyau reprennent leur ordonnancement d'origine
  for (const auto &kv : impl_->boosted) {
    sched_param sp{};
    sp.sched_priority = kv.second.second;
    sched_setscheduler(kv.first, kv.second.first, &sp);  // priorite d'origine (ex. irq/* en FIFO 50)
  }
  delete impl_;
  impl_ = nullptr;
}

SchedTune::~SchedTune() { stop(); }
