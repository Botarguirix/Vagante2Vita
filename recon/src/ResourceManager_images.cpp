#include "recon_all.h"
#include "recon_lazy.h"
#include <algorithm>
#include <memory>
#define VLOG(level) if ((level) > Log::reportingLevel()) ; else Log().get(level)

bool PaletteDatabase::Palette::generatePaletteSwap(sf::Image &image, const Palette &base,
                                                   unsigned int highlighted) const {
    if (base.colors.size() < colors.size()) {
        VLOG(LOG_ERROR) << "PaletteDatabase::Palette::generatePaletteSwap: Could not generate palette swap, palette size mismatch!";
        return false;
    }
    if (base.colors.size() != colors.size()) {
        VLOG(LOG_ERROR) << "PaletteDatabase::Palette::generatePaletteSwap: WARNING! palette doesn't fully map to all default palette colors!";
    }
    const auto size = image.getSize();
    for (unsigned x = 0; x < size.x; ++x) for (unsigned y = 0; y < size.y; ++y) {
        const auto pixel = image.getPixel(x, y);
        if (!pixel.a) continue;
        bool found = false;
        for (unsigned index = 0; index < base.colors.size(); ++index) {
            const auto &color = base.colors[index];
            if (color.r != pixel.r || color.g != pixel.g || color.b != pixel.b) continue;
            found = true;
            if (index < colors.size()) {
                const auto replacement = index == highlighted ? sf::Color::Magenta : colors[index];
                image.setPixel(x, y, sf::Color(replacement.r, replacement.g, replacement.b, pixel.a));
                break;
            }
        }
        if (!found) {
            VLOG(LOG_ERROR) << "PaletteDatabase::Palette::generatePaletteSwap: Could not find pixel in default palette! x=" << x << " y=" << y;
            return false;
        }
    }
    return true;
}

bool ResourceManager::processPaletteSwap(const std::string &name,
                                         const PaletteDatabase::PaletteGroup &group,
                                         const PaletteDatabase::Palette &palette) {
    const std::string key = name + group.name + palette.name;
    if (cache && archive && archive->isLoaded()) cache->getMemoryBlock(key);
    recon::lazy::prepare(name, images, mutexes[M_IMAGES], [&](bool s, bool m, bool o) { loadImage(name, s, m, o); });
    std::unique_ptr<ImageResource> image;
    {
        std::lock_guard<std::mutex> lock(mutexes[M_IMAGES]);
        const auto source = images.find(name);
        if (source == images.end()) {
            VLOG(LOG_ERROR) << "ResourceManager::processPaletteSwap: could not find image \"" << name << '"';
            return false;
        }
        image = std::make_unique<ImageResource>(*source->second);
    }
    if (!palette.generatePaletteSwap(image->image, group.defaultPalette, 0xffffffffu)) {
        VLOG(LOG_ERROR) << "ResourceManager::processPaletteSwap: failed to generate palette swap \"" << name << "\"->\"" << group.defaultPalette.name << "\"->\"" << palette.name << '"';
        return false;
    }
    {
        std::lock_guard<std::mutex> lock(mutexes[M_IMAGES_PALETTES]);
        if (imagesPaletteSwaps.emplace(key, image.get()).second) image.release();
    }
    if (cache && archive && !archive->isLoaded()) {
        std::lock_guard<std::mutex> lock(mutexes[M_CACHE]);
        cache->addResourceLocationDescriptor(key, ResourceArchive::RESOURCETYPE_TEXTURE_PALETTE);
    }
    return true;
}

const sf::Texture *ResourceManager::getTextureMask(std::string name) {
    const bool loading = !initialized.load();
    std::unique_lock<std::mutex> textureLock(mutexes[M_TEXTURES_MASKS], std::defer_lock);
    if (loading && !textureLock.try_lock()) return nullptr;
    const auto texture = textureMasks.find(name);
    if (texture != textureMasks.end()) return texture->second;
    recon::lazy::prepare(name, imagesMasks, mutexes[M_IMAGES_MASKS], [&](bool s, bool m, bool o) { loadImage(name, s, m, o); });
    std::unique_lock<std::mutex> imageLock(mutexes[M_IMAGES_MASKS], std::defer_lock);
    if (loading && !imageLock.try_lock()) return nullptr;
    const auto image = imagesMasks.find(name);
    if (image != imagesMasks.end()) {
        sf::Texture *texture = uploadTexture(name.c_str(), image, &imagesMasks, &textureMasks);
        if (recon::lazy::enabled()) { if (!imageLock.owns_lock()) imageLock.lock(); recon::lazy::release(name, imagesMasks); }
        return texture;
    }
    if (loading) return nullptr;
    VLOG(LOG_ERROR) << "ResourceManager::getTextureMask: Unable to get texture: \"" << name << '"';
    return errorTexture;
}
const sf::Texture *ResourceManager::getTextureOutlineMask(std::string name) {
    const bool loading = !initialized.load();
    std::unique_lock<std::mutex> textureLock(mutexes[M_TEXTURES_OUTLINES], std::defer_lock);
    if (loading && !textureLock.try_lock()) return nullptr;
    const auto texture = textureOutlineMasks.find(name);
    if (texture != textureOutlineMasks.end()) return texture->second;
    recon::lazy::prepare(name, imagesMasksOutline, mutexes[M_IMAGES_OUTLINES], [&](bool s, bool m, bool o) { loadImage(name, s, m, o); });
    std::unique_lock<std::mutex> imageLock(mutexes[M_IMAGES_OUTLINES], std::defer_lock);
    if (loading && !imageLock.try_lock()) return nullptr;
    const auto image = imagesMasksOutline.find(name);
    if (image != imagesMasksOutline.end()) {
        sf::Texture *texture = uploadTexture(name.c_str(), image, &imagesMasksOutline, &textureOutlineMasks);
        if (recon::lazy::enabled()) { if (!imageLock.owns_lock()) imageLock.lock(); recon::lazy::release(name, imagesMasksOutline); }
        return texture;
    }
    if (loading) return nullptr;
    VLOG(LOG_ERROR) << "ResourceManager::getTextureOutlineMask: Unable to get texture: \"" << name << '"';
    return errorTexture;
}

// ResourceManager::getTextureAltPalette @ 0074d370: the texture of one palette variant
// (key texture + group + palette, as built by processPaletteSwap), uploaded on first request.
// With RECON_LAZY_IMAGES the variant itself is generated here, on first use.
const sf::Texture *ResourceManager::getTextureAltPalette(const std::string &texture, const std::string &group,
                                                         const std::string &palette) {
    const PaletteDatabase::PaletteGroup paletteGroup = globalPaletteDb.getPaletteGroup(group);
    const auto variant = paletteGroup.palettes.find(palette);
    if (variant == paletteGroup.palettes.end()) return getTexture(texture, false);
    const std::string key = texture + paletteGroup.name + variant->second.name;
    {
        std::lock_guard<std::mutex> lock(mutexes[M_TEXTURES_PALETTES]);
        const auto found = texturePaletteSwaps.find(key);
        if (found != texturePaletteSwaps.end()) return found->second;
    }
    bool generated;
    {
        std::lock_guard<std::mutex> lock(mutexes[M_IMAGES_PALETTES]);
        generated = imagesPaletteSwaps.count(key) != 0;
    }
    if (!generated && recon::lazy::enabled() && !processPaletteSwap(texture, paletteGroup, variant->second))
        return getTexture(texture, false);
    std::lock_guard<std::mutex> textureLock(mutexes[M_TEXTURES_PALETTES]);
    std::lock_guard<std::mutex> imageLock(mutexes[M_IMAGES_PALETTES]);
    const auto image = imagesPaletteSwaps.find(key);
    if (image == imagesPaletteSwaps.end()) return getTexture(texture, false);
    sf::Texture *uploaded = uploadTexture(key.c_str(), image, &imagesPaletteSwaps, &texturePaletteSwaps);
    if (recon::lazy::enabled()) {
        delete image->second;  // the GPU copy is all the game draws from
        imagesPaletteSwaps.erase(image);
    }
    return uploaded;
}

