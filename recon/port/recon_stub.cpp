#include "recon_stub.h"

#include <cstdio>
#include <mutex>
#include <set>
#include <string>

namespace recon {
void unimplemented(const char *function) {
    static std::mutex lock;
    static std::set<std::string> reported;
    static std::FILE *log = std::fopen("recon.log", "w");
    std::lock_guard<std::mutex> guard(lock);
    if (!reported.insert(function).second) return;
    std::fprintf(stderr, "UNIMPLEMENTED %s\n", function);
    if (log) { std::fprintf(log, "UNIMPLEMENTED %s\n", function); std::fflush(log); }
}
}
