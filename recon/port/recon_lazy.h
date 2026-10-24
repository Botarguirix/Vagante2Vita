// On-demand image loading (port change, enabled by RECON_LAZY_IMAGES; default on the PS Vita).
//
// The original decodes every PNG at startup and keeps the RGBA pixels, plus a full copy per
// palette variant, in RAM for the whole session (>300 MB on PC); the Vita heap is 256 MB.
// With RECON_LAZY_IMAGES:
//  - ResourceManager::loadImage only registers the image (its compressed PNG stays pinned);
//  - the first request for a texture (plain, mask or outline) decodes it, uploads it and frees
//    the CPU pixels;
//  - palette groups are recorded and each palette variant is generated on first use.
// ResourceManager's maps are private, so the member functions pass them in.
#pragma once

#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace recon {
namespace lazy {

constexpr bool enabled() {
#if defined(RECON_LAZY_IMAGES)
    return true;
#else
    return false;
#endif
}

struct Entry {
    bool smooth = false, mask = false, outline = false, repeat = false;
};

inline std::mutex &registryLock() { static std::mutex m; return m; }
inline std::map<std::string, Entry> &registry() { static std::map<std::string, Entry> r; return r; }
inline bool &decoding() { static thread_local bool d = false; return d; }

// Top of loadImage: true means "registered, do not decode now".
inline bool deferLoad(const std::string &path, bool smooth, bool mask, bool outline) {
    if (!enabled() || decoding()) return false;
    std::lock_guard<std::mutex> guard(registryLock());
    Entry &entry = registry()[path];
    entry.smooth |= smooth;
    entry.mask |= mask;
    entry.outline |= outline;
    return true;
}

inline bool find(const std::string &path, Entry &out) {
    if (!enabled()) return false;
    std::lock_guard<std::mutex> guard(registryLock());
    const auto found = registry().find(path);
    if (found == registry().end()) return false;
    out = found->second;
    return true;
}

inline void setRepeat(const std::string &path) {
    std::lock_guard<std::mutex> guard(registryLock());
    const auto found = registry().find(path);
    if (found != registry().end()) found->second.repeat = true;
}

// Decodes a registered image if it is not in the CPU map yet. Call without holding imageMutex.
// decode(smooth, mask, outline) runs the original loadImage body.
template<class ImageMap, class Decode>
void prepare(const std::string &path, ImageMap &images, std::mutex &imageMutex, Decode decode) {
    Entry entry;
    if (!find(path, entry)) return;
    {
        std::lock_guard<std::mutex> guard(imageMutex);
        if (images.count(path)) return;
    }
    decoding() = true;
    decode(entry.smooth, entry.mask, entry.outline);
    decoding() = false;
    if (entry.repeat) {
        std::lock_guard<std::mutex> guard(imageMutex);
        const auto found = images.find(path);
        if (found != images.end()) found->second->repeat = true;
    }
}

// After a texture exists for path: frees the CPU copy in one map (caller holds its mutex).
template<class ImageMap>
void release(const std::string &path, ImageMap &images) {
    if (!enabled()) return;
    Entry entry;
    if (!find(path, entry)) return;
    const auto found = images.find(path);
    if (found == images.end()) return;
    delete found->second;
    images.erase(found);
}

// Palette groups recorded instead of generating every variant at startup.
struct PaletteRequest { std::string texture, group, palette; };
inline std::map<std::string, PaletteRequest> &paletteRequests() {
    static std::map<std::string, PaletteRequest> r;
    return r;
}

}  // namespace lazy
}  // namespace recon
