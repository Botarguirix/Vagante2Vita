// GuiButton 005e4b70-005e58bc; inherited offsets recovered from machine code.
#include "recon_all.h"
#include <cmath>
GuiButton::GuiButton(std::string text, int px, int py, int width, int height,
                     std::string tip, bool background)
    : GuiElement(BUTTON, px, py, width, height), centered(true), isATab(false),
      fgColor(sf::Color::White), name(std::move(text)), hasBackground(background),
      toggleState(false), doAction(false), rightClick(false),
      buttonSprite({}, "@gui/gui_button_nine_slice.png", 8, 8, -1, 0, 0, 0, false, false),
      tabSprite({}, "gui/gui_tabs.png", 8, 16, -1, 0, 0, 0, false, false), hoverTime(0) {
    buttonSprite.setOrigin(0, 0);
    tabSprite.setOrigin(0, 0);
    tooltip=std::move(tip);
}
GuiButton::~GuiButton() = default;
void GuiButton::action(bool down) {
    if (disabled) return;
    if (down) { pressed=true; return; }
    if (pressed) doAction=true;
    pressed=false;
}
void GuiButton::mouseAction(int, int, bool down, bool left) {
    if (disabled || hidden) return;
    if (down) { pressed=true; return; }
    if (pressed) { doAction=true; rightClick=!left; }
    pressed=false;
}
void GuiButton::step(int, int) {
    if (hidden) { pressed=false; return; }
    if (hover) ++hoverTime;
    else hoverTime=0;
}
void GuiButton::afterStep(bool left, bool right) {
    if (pressed && (!hover || (!left && !right))) { pressed=false; doAction=true; }
}
bool GuiButton::pollAction() {
    const bool result=doAction;
    if (result) ResourceManager::playSound("menu/menu_use_button.wav", false);
    doAction=false;
    return result;
}
void GuiButton::draw(sf::RenderTarget &target) {
    if (hidden) return;
    const auto alpha=static_cast<sf::Uint8>(static_cast<unsigned>(alphaModifier*255.f));
    const float px=x+offset.x, py=y+offset.y;
    if (!hasBackground) {
        const float thickness=pressed ? 2.f : 1.f;
        sf::RectangleShape box({w-2*thickness,h-2*thickness});
        box.setOutlineThickness(thickness);
        box.setPosition(px+thickness,py+thickness);
        auto color=disabled ? disabledColor : (hover ? hoverColor : sf::Color(200,200,200));
        color.a=alpha;
        box.setOutlineColor(color);
        box.setFillColor(sf::Color(0,toggleState ? 66 : 0,0,alpha));
        target.draw(box);
    } else {
        auto color=disabled ? sf::Color(100,100,100) : (hover ? sf::Color::White : sf::Color(200,200,200));
        color.a=alpha;
        buttonSprite.setColor(color);
        const int columns=static_cast<int>(std::ceil(w*.125f));
        const int rows=static_cast<int>(std::ceil(h*.125f));
        int tile=0;
        for (int row=0;row<rows;++row) {
            for (int column=0;column<columns;++column) {
                buttonSprite.setFrame((pressed ? 9 : 0)+tile);
                buttonSprite.setPosition(px+column*8.f,row==rows-1 ? py+h-8.f : py+row*8.f);
                buttonSprite.draw(target,sf::RenderStates::Default);
                if (!disabled && hover) {
                    buttonSprite.setAlpha(static_cast<sf::Uint8>(static_cast<int>(alphaModifier*50.f)));
                    buttonSprite.draw(target,sf::RenderStates(sf::BlendAdd));
                    buttonSprite.setColor(color);
                }
                if (column>=columns-2 || column==0) {
                    ++tile;
                    if (tile==3) { if (rows==2) tile=6; }
                    else if (tile==6 && row<rows-2) tile=3;
                }
            }
        }
    }
    auto *font=ResourceManager::getDefaultFont();
    if (!font) return;
    sf::Text text(sf::String::fromUtf8(name.begin(),name.end()),*font,16);
    text.setScale(.5f,.5f);
    float tx=px+1.f, ty=py+h*.5f-6.f;
    if (centered) tx+=w*.5f-text.getLocalBounds().width*.25f;
    if (pressed) { ++tx; ++ty; }
    text.setPosition(tx,ty);
    if (hasBackground) {
        text.move(0,.5f);
        text.setOutlineColor(sf::Color(18,13,10,static_cast<sf::Uint8>(static_cast<int>(alphaModifier*200.f))));
        text.setOutlineThickness(2.f);
    }
    sf::Color color;
    auto textAlpha=alpha;
    if (!disabled) {
        if (!hasBackground) color=fgColor;
        else if (!hover) { color=sf::Color(186,201,179); textAlpha=static_cast<sf::Uint8>(static_cast<int>(alpha*.5f)); }
        else color=sf::Color(196,211,189);
    } else if (!hasBackground) color=disabledColor;
    else { color=sf::Color(86,86,86); textAlpha=static_cast<sf::Uint8>(static_cast<int>(alpha*.5f)); }
    color.a=textAlpha;
    text.setFillColor(color);
    target.draw(text);
}
// Field accessors expanded in original GUI callers, not independent indexed bodies.
void GuiButton::setUseBackground(bool value) { hasBackground=value; }
void GuiButton::updateName(std::string value) { name=std::move(value); }
bool GuiButton::wasRightClick() { return rightClick; }
void GuiButton::setToggleState(bool value) { toggleState=value; }
bool GuiButton::getToggleState() { return toggleState; }
std::string GuiButton::getName() { return name; }
