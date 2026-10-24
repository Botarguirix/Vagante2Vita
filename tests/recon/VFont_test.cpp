#include "port_lib_types.h"
#define private public
#include "VFont.h"
#include "ResourceManager.h"
#undef private
#include "recon_all.h"
#include <cassert>
#include <iostream>

int main() {
    sf::Image image;
    image.create(12, 18, sf::Color::Transparent);
    for (unsigned x : {0u, 3u, 7u}) image.setPixel(x, 0, sf::Color::Red);
    image.setPixel(11, 0, sf::Color::Blue);
    image.setPixel(0, 9, sf::Color::Red);
    image.setPixel(4, 9, sf::Color::Blue);
    image.setPixel(4, 2, sf::Color::White);
    image.setPixel(8, 2, sf::Color::Green);
    image.setPixel(1, 11, sf::Color::Blue);
    VFont font{};
    font.loadFromImage("font-fixture", image, 1);
    assert(font.loaded && font.characters.size() == 128 && font.characterHeight == 4);
    assert(font.characters[32].x == 0 && font.characters[32].y == 1 && font.characters[32].w == 3);
    assert(font.characters[33].x == 3 && font.characters[33].w == 4 && font.characters[33].h == 8);
    assert(font.characters[34].x == 7 && font.characters[35].y == 10);
    assert(font.characters[0].w == 8 && font.characters[0].h == 1);
    assert(font.getStringWidth("! \"", false) == 11);
    assert(font.getStringWidth("! \"", true) == 5);
    assert(font.getStringWidthWithoutSpaces("! \"") == 8);
    assert(font.getNumSpaces(" !  \" ") == 4);
    assert(font.getStringWidth("", false) == 0);
    assert(font.getStringWidth(std::string(1, char(255)), false) == 0);

    sf::RenderTexture target;
    assert(target.create(24, 16));
    target.clear(sf::Color::Transparent);
    font.drawString("!", 1, 1, 6, target, sf::Color::White, false);
    target.display(); // Texture is not published yet; draw must be a no-op.
    assert(!font.texture && target.getTexture().copyToImage().getPixel(2, 2).a == 0);
    auto *atlas = new sf::Texture;
    assert(atlas->loadFromImage(image));
    ResourceManager::textures.emplace("font-fixture", atlas);
    font.drawString("! \"", 1, 1, 6, target, sf::Color::White, false);
    target.display();
    auto pixels = target.getTexture().copyToImage();
    assert(font.texture == atlas && pixels.getPixel(2, 2) == sf::Color::White);
    assert(pixels.getPixel(13, 2) == sf::Color::Green);
    assert(pixels.getPixel(11, 2).a == 0);
    target.clear(sf::Color::Transparent);
    font.drawString("! \"", 1, 1, 6, target, sf::Color(128, 255, 255), true);
    target.display(); pixels = target.getTexture().copyToImage();
    assert(pixels.getPixel(2, 2) == sf::Color(128, 255, 255));
    assert(pixels.getPixel(11, 2) == sf::Color::Green && pixels.getPixel(13, 2).a == 0);
    assert(pixels.saveToFile("vfont-fixture.png"));
    const auto before = pixels;
    font.drawString(std::string(1, char(255)), 1, 1, 6, target, sf::Color::White, false);
    target.display();
    assert(target.getTexture().copyToImage().getPixel(2, 2) == before.getPixel(2, 2));
    VFont fromTexture{};
    fromTexture.loadFromTexture("other-name", atlas, -1);
    assert(fromTexture.loaded && fromTexture.texture == atlas && fromTexture.wBias == -1);
    fromTexture.loadFromTexture("missing", nullptr, 0);
    assert(!fromTexture.loaded && !fromTexture.texture);
    sf::Image invalid;
    invalid.create(2, 2, sf::Color::Transparent);
    VFont bad{};
    bad.loadFromImage("no markers", invalid, 0);
    assert(!bad.loaded);
    invalid = image;
    invalid.setPixel(4, 9, sf::Color::Transparent);
    bad.loadFromImage("unclosed", invalid, 0);
    assert(!bad.loaded);
    ResourceManager::textures.clear();
    delete atlas;
    std::cout << "VFont: marker parser, widths, spaces, tight advance, bias, lazy texture, tint and rendered pixels passed\n";
}
