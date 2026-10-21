#include "recon_all.h"

std::basic_ostringstream<char> &Log::getConditional(
    bool condition, bool isClient, LogLevel, LogLevel) {
    if (condition) {
        return get(LOG_SERVER);
    }
    if (!isClient) {
        return get(LOG_DEBUG);
    }
    return get(LOG_CLIENT);
}
