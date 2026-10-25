// GameGui.cpp, PDB contract and Ghidra 005cd300-005ceeff.
#include "recon_all.h"
#include <iomanip>
#include <sstream>

GameGui::GameGui()
    : frame(0),
      goldFontSprite({}, "font/mono$paced.png", 16, 16, -1, 0, 0, 0, false, false),
      goldBannerSprite({}, "gui/gold_banner.png", 64, 16, -1, 0, 0, 0, false, false),
      goldCoinSprite({}, "gui/gold_coin.png", 16, 16, -1, 0, 0, 0, false, false),
      gamepadSprite({}, "gui/icon_gamepad.png", -1, -1, -1, 0, 0, 0, false, false),
      keyboardSprite({}, "gui/icon_keyboard.png", -1, -1, -1, 0, 0, 0, false, false),
      shakePurse(0), spawnedGoldTimer(0), justPickedUpGoldTimer(0), prevGoldReserve(0),
      displayExperience(0), experienceEffectCounter(0), experienceShineEffectCounter(0),
      timerPosition(-10), hideTimerDelay(0), menuPosition(0, 0), showTimer(false) {
    goldFontSprite.setOrigin(0, 0);
    goldBannerSprite.setOrigin(0, 0);
    goldCoinSprite.setOrigin(0, 0);
}
GameGui::~GameGui() = default;

void GameGui::resized(sf::RenderTarget &target) {
    // 00814110 returns sf::View::getSize, despite Ghidra's RenderTarget label.
    const sf::View view = target.getView();
    menuPosition.x = (view.getSize().x - 640.f) * .5f;
    menuPosition.y = (view.getSize().y - 360.f) * .5f;
}

void GameGui::drawGameVersion(sf::RenderTarget &target, bool online, Config *config) {
    auto *font = ResourceManager::getDefaultFont();
    if (!font) return;
    sf::Text text(GAME_VERSION + PATCH_VERSION + (online ? " Online" : ""), *font, 16);
    text.setScale(.5f, .5f);
    text.setFillColor({255, 255, 255, 20});
    text.setPosition(3, 0);
    target.draw(text);
    text.move(text.getLocalBounds().width * .5f + 3.f, 0);
    text.setFillColor({150, 150, 255, 22});
    text.setString(ResourceManager::getLanguageString(config->language));
    target.draw(text);
}

void GameGui::drawTime(sf::RenderTarget &target, unsigned ticks) {
    std::ostringstream stream;
    const unsigned minutes = ticks / 60 / 60, seconds = ticks / 60 % 60;
    stream << std::setfill('0') << std::setw(2) << minutes << ':' << std::setw(2) << seconds;
    auto *font = ResourceManager::getDefaultFont();
    if (!font) return;
    sf::Text text(stream.str(), *font, 16);
    text.setScale(.5f, .5f);
    text.setFillColor({170, 170, 170});
    text.setPosition(target.getView().getSize().x * .5f - text.getLocalBounds().width * .25f,
                     timerPosition);
    target.draw(text);
}

void GameGui::drawFpsCounter(sf::RenderTarget &target, int fps, int updates) {
    auto *font = ResourceManager::getDefaultFont();
    if (!font) return;
    sf::Text text(std::to_string(fps), *font, 16);
    text.setScale(.5f, .5f);
    text.setFillColor(fps < 41 ? sf::Color(255, 0, 0, 50) :
                      fps < 51 ? sf::Color(255, 255, 0, 50) : sf::Color(0, 255, 0, 50));
    text.setPosition(target.getView().getSize().x - text.getGlobalBounds().width - 1.f, -2.f);
    target.draw(text);
    sf::Text updateText(std::to_string(updates), *font, 16);
    updateText.setScale(.5f, .5f);
    updateText.setPosition(text.getPosition().x - updateText.getGlobalBounds().width - 2.f, -2.f);
    updateText.setFillColor({255, 255, 255, 40});
    target.draw(updateText);
}

void GameGui::drawNetGraph(sf::RenderTarget &, std::vector<int> &, std::vector<int> &,
                         std::vector<int> &, std::vector<int> &,
                         std::vector<unsigned __int64> &, int) {
    // Network diagnostics are intentionally excluded from the port.
}
