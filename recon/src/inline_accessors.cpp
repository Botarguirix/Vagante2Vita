// Inline accessors that the original compiler expanded at every call site, so they have no
// function of their own in vagante.exe and do not appear in any unit's function list.
// Their bodies are inferred from the expanded code at the call sites (noted per function).
#include "recon_all.h"

// --- MainMenu (Client::step @ 0055e500: on ConfigGui::doRunCredits it stores fadingOutTarget = 3
//     and fadingOut = true)
void MainMenu::startCredits() {
    fadingOutTarget = 3;
    fadingOut = true;
}

// --- NetMenu (Client::step reads open directly)
bool NetMenu::isOpen() { return open; }

// NetMenu::setOpen @ 006b9240 (a real function, kept here beside isOpen).
void NetMenu::setOpen(bool value) {
    if (open == value) return;
    customWorld = false;
    selectingCustomWorld = false;
    overlayDialogFrames = 0;
    open = value;
    if (value) {
        loadingMusicFade = 1.0f;
        ResourceManager::setSpecificMusicVolume("title_theme.ogg", 1.0f);
        animateInTimer = 20;
        refresh(true);
        selectPos.x = 1;
        selectPos.y = 13;
        for (GuiElement *element : guiElements) element->setAlpha(0.0f);
    } else {
        loadingSubtext = "...";
        if (friendBrowser) friendBrowser->clearFriends();
    }
}

// NetMenu::up/down (Client::handleInput @ 00550ea0): a fresh press restarts the key-repeat delay
// (30 frames), a held direction only keeps the flag set. Indices follow Input::Actions (UP 3, DOWN 2
// in the action enum; the menu's own direction slots are 1 for up and 3 for down).
void NetMenu::up(bool held) {
    if (animateInTimer != 0 || selectingCustomWorld) return;
    directionHeld[1] = true;
    if (!held) framesHoldingDirection[1] = std::pair<int, int>(0, 30);
}

void NetMenu::down(bool held) {
    if (animateInTimer != 0 || selectingCustomWorld) return;
    directionHeld[3] = true;
    if (!held) framesHoldingDirection[3] = std::pair<int, int>(0, 30);
}

// --- ConfigGui (Client::step tests "open || closingTimer > 0" before stepping it, and on leaving
//     the progression screen stores screen = 0 and switchTimer = 10)
bool ConfigGui::isOpen() { return open || closingTimer > 0; }
ConfigGui::ConfigScreen ConfigGui::getScreen() { return screen; }

void ConfigGui::setScreen(ConfigGui::ConfigScreen value) {
    screen = value;
    switchTimer = 10;
}

// Client::handleInput forwards confirm presses to the hovered element once the open/close
// animation has finished.
void ConfigGui::actionPress() {
    if (openingTimer == 0 && closingTimer == 0 && hoveredElement) hoveredElement->action(true);
}

void ConfigGui::actionRelease() {
    if (openingTimer == 0 && closingTimer == 0 && hoveredElement) hoveredElement->action(false);
}

bool ConfigGui::doRunCredits() {
    const bool run = runCredits;
    runCredits = false;
    return run;
}

// --- Credits (Client::step resets the credits with this exact sequence in two places:
//     (false, false) after they end and (true, true) when started from the main menu)
void Credits::setTriggered(bool value, bool skipEnemies) {
    state = CREDITS_PROGRAMMING;
    triggered = value;
    initializeCurrentState();
    creditsOver = false;
    fadeColor = sf::Color::Black;
    fadeFrames = 0;
    fadeFramesTotal = 120;
    fadingOut = false;
    skipBranchEnemies = true;
    skipAllEnemies = skipEnemies;
}

void Credits::setTriggeredFromMainMenu() { setTriggered(true, true); }

// --- Sprite (GuiCursor::draw @ 005e6220: speed == 0 gives (frame + 1) / numFrames, otherwise
//     (speed * frame + timer) / (numFrames * speed))
float Sprite::percentComplete() {
    if (speed == 0) return static_cast<float>(frame + 1) / static_cast<float>(numFrames);
    return static_cast<float>(speed * frame + timer) / static_cast<float>(numFrames * speed);
}

// --- PlayerCustomizeMenu (Client::step copies both vectors straight into Client::newGame)
std::vector<Player::PlayerClass> PlayerCustomizeMenu::getClasses() { return playerClasses; }
std::vector<int> PlayerCustomizeMenu::getPalettes() { return playerPalettes; }

// --- Editors (Client::draw reads the flags directly; the editors are developer tools and stay null)
bool LevelEditor::isEditing() { return editing; }
bool WeaponEditor::isEditing() const { return m_isEditing; }
bool PaletteEditor::isEditing() const { return m_isEditing; }

// --- GameEngine
Level *GameEngine::getLevel() { return currLevel; }
std::vector<std::shared_ptr<Player>> &GameEngine::getPlayers() { return players; }
std::vector<std::shared_ptr<PlayerMenu>> &GameEngine::getPlayerMenus() { return playerMenus; }

// --- resized() of the menus: expanded inside Client::resized @ 00567750. Each fits the 640x360
//     design area into the window (ConfigGui ignores pixel-perfect scaling and centers on 256,180).
namespace {
float menuScale(const sf::Vector2u &size, bool perfectPixels) {
    float scale = std::min(static_cast<float>(size.x) / 640.0f, static_cast<float>(size.y) / 360.0f);
    if (perfectPixels) scale = static_cast<float>(static_cast<int>(scale));
    return scale;
}
void fitView(sf::View &view, const sf::Vector2u &size, float scale) {
    const float inverse = 1.0f / scale;
    view.setSize(static_cast<float>(static_cast<int>(static_cast<float>(size.x) * inverse)),
                 static_cast<float>(static_cast<int>(static_cast<float>(size.y) * inverse)));
}
}

void ConfigGui::resized() {
    fitView(view, window.getSize(), menuScale(window.getSize(), false));
    view.setCenter(256.0f, 180.0f);
    window.setView(view);
}

void MainMenu::resized() {
    fitView(view, window.getSize(), menuScale(window.getSize(), configGui.getConfig().forcePerfectPixelScaling));
    window.setView(view);
}

void Credits::resized(sf::RenderWindow &target) {
    fitView(view, target.getSize(), menuScale(target.getSize(), configGui.getConfig().forcePerfectPixelScaling));
    target.setView(view);
}