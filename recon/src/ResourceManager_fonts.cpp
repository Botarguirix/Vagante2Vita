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
