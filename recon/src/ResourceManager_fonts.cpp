#include "recon_all.h"
#include <memory>
#define VLOG(level) if ((level) > Log::reportingLevel()) ; else Log().get(level)

sf::Font *ResourceManager::getDefaultFont() {
    if (config->language != LANG_EN && config->language != LANG_KO) return getGuaranteedUnicodeFont();
    std::unique_lock<std::mutex> lock(mutexes[M_FONTS], std::defer_lock);
    if (!initialized.load() && !lock.try_lock()) return nullptr;
    const auto it = fonts.find("Tiny_04b.ttf");
    return it == fonts.end() ? nullptr : it->second;
}
sf::Font *ResourceManager::getGuaranteedUnicodeFont() {
    std::unique_lock<std::mutex> lock(mutexes[M_FONTS], std::defer_lock);
    if (!initialized.load() && !lock.try_lock()) return nullptr;
    const auto it = fonts.find("NotoSansCJKkr-Regular.otf");
    return it == fonts.end() ? nullptr : it->second;
}
void ResourceManager::loadVFont(const sf::Image &image, std::string name, int bias) {
    VFont *font = nullptr;
    {
        std::lock_guard<std::mutex> lock(mutexes[M_VFONTS]);
        const auto it = vFonts.find(name);
        if (it != vFonts.end()) font = it->second;
    }
    if (font) {
        font->loadFromImage(name, image, bias);
        if (!font->loaded) { VLOG(LOG_WARNING) << "ResourceManager::loadVFont: failed to reload VFont " << name; }
        return;
    }
    auto created = std::make_unique<VFont>();
    created->characterHeight = -1;
    created->loadFromImage(name, image, bias);
    if (!created->loaded) { VLOG(LOG_ERROR) << "ResourceManager::loadVFont: failed to load VFont " << name; return; }
    std::lock_guard<std::mutex> lock(mutexes[M_VFONTS]);
    if (vFonts.emplace(name, created.get()).second) created.release();
}
VFont *ResourceManager::getVFont(std::string name) {
    std::unique_lock<std::mutex> lock(mutexes[M_VFONTS], std::defer_lock);
    if (!initialized.load() && !lock.try_lock()) return nullptr;
    const auto it = vFonts.find(name);
    return it == vFonts.end() ? nullptr : it->second;
}

// Only the bitmap parser needed by loading belongs to this block; drawing and
// text measurement remain in the separate VFont task.
VFont::CharacterInfo::CharacterInfo() : x(0), y(0), w(8), h(1) {}
void VFont::loadFromImage(const std::string &name, const sf::Image &image, int bias) {
    loaded = false;
    sourceImageFileName = name;
    wBias = bias;
    characters.assign(128, CharacterInfo());
    characterHeight = 0;
    const auto size = image.getSize();
    // The original trusts the marker format. Bound checks also reject corrupt
    // images instead of reading outside sf::Image.
    while (static_cast<unsigned>(characterHeight + 1) < size.y) {
        const auto pixel = image.getPixel(0, characterHeight + 1);
        if (pixel.r == 255 && pixel.b != 255 && pixel.g != 255) break;
        ++characterHeight;
    }
    if (!size.x || static_cast<unsigned>(characterHeight + 1) >= size.y) return;
    unsigned x = 0, y = 0, startX = 0;
    int startY = 0, width = 0;
    unsigned character = 32;
    bool open = false;
    do {
        const auto pixel = image.getPixel(x, y);
        const bool red = pixel.r == 255 && pixel.b != 255;
        const bool blue = pixel.r != 255 && pixel.b == 255;
        if (red || blue) {
            if (!open) {
                startX = x; width = 0; startY = y + 1; open = true;
            } else {
                auto &info = characters[character++];
                info.x = startX; info.y = startY; info.w = width; info.h = characterHeight;
                width = 0; startX = x; startY = y + 1; open = pixel.r == 255;
            }
        }
        if (open) ++width;
        if (++x >= size.x) { x = 0; y += 1 + characterHeight; }
    } while (y < size.y && character != 123);
    loaded = !open;
    if (!loaded) { VLOG(LOG_WARNING) << "VFont::loadFromImage: Last character not closed properly, missing blue pixel."; }
    characterHeight -= 4;
}
