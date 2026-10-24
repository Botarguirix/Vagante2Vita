// Bitmap font parsing and rendering, recovered from VFont.cpp.
#include "recon_all.h"
#include <algorithm>
#include <utility>
#define VLOG(level) if ((level) > Log::reportingLevel()) ; else Log().get(level)

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

void VFont::loadFromTexture(const std::string &name, sf::Texture *source, int bias) {
    // Inline-only convenience API: retain the supplied texture without owning it.
    if (!source) { loaded = false; texture = nullptr; return; }
    loadFromImage(name, source->copyToImage(), bias);
    texture = source;
}
int VFont::getStringWidth(std::string text, bool tight) {
    int width = 0;
    for (const unsigned char character : text) {
        if (character < characters.size()) width += -2 + characters[character].w + (tight ^ 1) * 2;
    }
    return width;
}
int VFont::getStringWidthWithoutSpaces(std::string text) {
    text.erase(std::remove(text.begin(), text.end(), ' '), text.end());
    return getStringWidth(std::move(text), false);
}
int VFont::getNumSpaces(std::string text) {
    return static_cast<int>(std::count(text.begin(), text.end(), ' '));
}
void VFont::drawString(std::string text, float x, float y, float spaceAdvance,
                       sf::RenderTarget &target, sf::Color color, bool tight) {
    if (!texture) {
        texture = ResourceManager::getTexture(sourceImageFileName, false); // CL=0 @007ec08d.
        if (!texture) return;
    }
    sf::Sprite sprite(*texture);
    sprite.setColor(color);
    sprite.setPosition(x, y);
    for (const unsigned char character : text) {
        // getStringWidth uses unsigned bytes. Use the same range here instead
        // of reproducing the original signed-char out-of-bounds access >=128.
        if (character >= characters.size()) continue;
        const auto &info = characters[character];
        sprite.setTextureRect({info.x, info.y, info.w, info.h});
        target.draw(sprite);
        const float advance = character == ' ' ? spaceAdvance
            : static_cast<float>(tight ? info.w - 2 : info.w) + static_cast<float>(wBias);
        sprite.move(advance, 0);
    }
}
