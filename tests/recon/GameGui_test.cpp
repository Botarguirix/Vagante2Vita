#include "port_lib_types.h"
#define private public
#include "GameGui.h"
#include "ResourceManager.h"
#undef private
#include "recon_all.h"
#include <cassert>
#include <cmath>
#include <iostream>

int main() {
    Config config;
    ResourceManager::setConfig(&config);
    sf::Image image;
    image.create(416, 16, sf::Color::White);
    sf::Texture texture;
    assert(texture.loadFromImage(image));
    for (const char *name : {"font/mono$paced.png", "gui/gold_banner.png", "gui/gold_coin.png",
                             "gui/icon_gamepad.png", "gui/icon_keyboard.png"})
        ResourceManager::textures[name] = &texture;
    sf::Font font;
    assert(font.loadFromFile("C:/Windows/Fonts/arial.ttf"));
    ResourceManager::fonts["Tiny_04b.ttf"] = &font;
    ResourceManager::fonts["NotoSansCJKkr-Regular.otf"] = &font;
    sf::RenderTexture target;
    assert(target.create(640, 360));
    _randEngine.seed(123);
    auto expected = _randEngine;
    GameGui gui;
    assert(_randEngine == expected && gui.frame == 0 && gui.goldPieces.empty());
    assert(gui.goldFontSprite.getSize() == sf::Vector2i(16, 16));
    assert(gui.goldBannerSprite.getSize() == sf::Vector2i(64, 16));
    assert(gui.timerPosition == -10 && gui.menuPosition == sf::Vector2f(0, 0) && !gui.showTimer);
    sf::View view(sf::FloatRect(12, 30, 800, 480));
    target.setView(view);
    gui.resized(target);
    assert(gui.menuPosition == sf::Vector2f(80, 60));
    assert(target.getView().getCenter() == view.getCenter());
    target.setView(target.getDefaultView());
    auto pixels = [&]() {
        target.display(); auto result = target.getTexture().copyToImage();
        return std::vector<sf::Uint8>(result.getPixelsPtr(), result.getPixelsPtr() + 640 * 360 * 4);
    };
    auto changed = [](const auto &a, const auto &b) { return a != b; };
    target.clear(sf::Color::Transparent);
    const auto blank = pixels();
    gui.drawGameVersion(target, false, &config);
    const auto offline = pixels();
    assert(changed(blank, offline));
    target.clear(sf::Color::Transparent);
    gui.drawGameVersion(target, true, &config);
    assert(changed(offline, pixels()));
    gui.timerPosition = 10;
    target.clear(sf::Color::Transparent); gui.drawTime(target, 0); auto zero = pixels();
    target.clear(sf::Color::Transparent); gui.drawTime(target, 59); assert(zero == pixels());
    target.clear(sf::Color::Transparent); gui.drawTime(target, 60); assert(changed(zero, pixels()));
    target.clear(sf::Color::Transparent); gui.drawFpsCounter(target, 40, 2); auto low = pixels();
    target.clear(sf::Color::Transparent); gui.drawFpsCounter(target, 41, 2); auto medium = pixels();
    target.clear(sf::Color::Transparent); gui.drawFpsCounter(target, 51, 2);
    assert(changed(low, medium) && changed(medium, pixels()));
    std::vector<float> pads{.5f, 0, .1f, .9f}, keys{0, .5f, .8f, .1f};
    gui.drawControllerUpdates(target, pads, keys);
    assert(gui.gamepadSprite.getPosition().x == 560 && gui.keyboardSprite.getPosition().x == 400);
    assert(_randEngine == expected);
    gui.drawExperienceBar(target, 1, 50, false);
    expected();
    assert(_randEngine == expected && gui.displayExperience == .5f);
    assert(std::abs(gui.experienceEffectCounter - (.5f - .03f) * .89f) < 1e-6f);
    gui.drawExperienceBar(target, 1, 0, false);
    assert(gui.displayExperience == 0 && gui.experienceEffectCounter == 0 && _randEngine == expected);
    gui.displayExperience = 99.75f;
    gui.drawExperienceBar(target, 1, 100, true);
    expected();
    assert(gui.displayExperience == 100 && std::abs(gui.experienceShineEffectCounter - 2.7f) < 1e-6f && _randEngine == expected);
    gui.drawGoldDisplay(target, 100, 10);
    assert(gui.goldBannerSprite.getPosition() == sf::Vector2f(574, 10));
    assert(gui.goldCoinSprite.getPosition() == sf::Vector2f(579, 7));
    assert(gui.goldFontSprite.getFrame() == 16 && gui.goldFontSprite.getPosition().y == 22);
    assert(_randEngine == expected);
    target.clear(sf::Color::White);
    gui.drawTransitionCircle(target, 128, nullptr); target.display();
    auto transition = target.getTexture().copyToImage();
    assert(transition.getPixel(320, 180) == sf::Color::White && transition.getPixel(0, 180) == sf::Color::Black);
    target.clear(sf::Color::White);
    gui.drawTransitionCircle(target, 256, nullptr); target.display();
    assert(target.getTexture().copyToImage().getPixel(0, 180) == sf::Color::White);
    std::vector<int> values;
    std::vector<unsigned __int64> times;
    gui.drawNetGraph(target, values, values, values, values, times, 0);
    assert(_randEngine == expected);
    ResourceManager::fonts.clear(); ResourceManager::textures.clear();
    std::cout << "GameGui: constructor, resized, rendered labels, counters, gold, transitions and RNG passed\n";
}
