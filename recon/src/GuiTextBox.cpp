// GuiTextBox 005e9bf0-005ea8aa, with inlined Input operations through its public API.
#include "recon_all.h"
#include <cmath>
#include <algorithm>
#define VLOG(level) if ((level) > Log::reportingLevel()) ; else Log().get(level)
GuiTextBox::GuiTextBox(Input &source, float px, float py, float width, float)
    : GuiElement(TEXTBOX,static_cast<int>(px),static_cast<int>(py),static_cast<int>(width),10),
      focus(false),useAboveLabel(false),input(source),doAction(false),bgLabel(),label(),
      frame(0),bound(false),boundBuffer(nullptr) {}
GuiTextBox::~GuiTextBox() { input.stopEnteringTextIfBufferEqual(boundBuffer); }
void GuiTextBox::bindString(std::string *buffer) {
    bound=true;
    boundBuffer=buffer;
    // The original dereferences buffer for this diagnostic. Null is tolerated
    // here so a malformed binding cannot crash simply because logging is on.
    VLOG(LOG_DEBUG) << "GuiTextBox::bindString(" << (buffer ? *buffer : std::string()) << ")";
}
void GuiTextBox::startEnteringText(bool newLines) {
    focus=true;
    if (bound) input.setEnteringText(boundBuffer,boundBuffer ? static_cast<int>(boundBuffer->size()) : 0,newLines);
}
void GuiTextBox::stopEnteringText() { input.stopEnteringTextIfBufferEqual(boundBuffer); }
void GuiTextBox::mouseAction(int, int, bool down, bool left) {
    if (hidden || disabled) return;
    VLOG(LOG_DEBUG) << down << " . " << left;
    if (down && left) {
        startEnteringText(true);
        VLOG(LOG_DEBUG) << "Set entering text to true.";
    } else { VLOG(LOG_DEBUG) << "Some other mouse action"; }
}
void GuiTextBox::step(int, int) {
    ++frame;
    if (focus && !input.enteringText) {
        VLOG(LOG_DEBUG) << "!enteringTExt.... focus = falsee doaction = true";
        focus=false;
        doAction=true;
    }
}
void GuiTextBox::draw(sf::RenderTarget &target) {
    if (hidden) return;
    // Unlike GuiButton, this compiled draw uses x/y without offset or alphaModifier.
    sf::RectangleShape box({static_cast<float>(w),static_cast<float>(h)});
    box.setPosition(static_cast<float>(x),static_cast<float>(y));
    box.setOutlineThickness(.5f);
    box.setOutlineColor(disabled ? disabledColor : (hover ? hoverColor : sf::Color(200,200,200)));
    box.setFillColor(sf::Color(0,0,0,200));
    target.draw(box);
    auto *font=ResourceManager::getGuaranteedUnicodeFont();
    if (!font) return;
    sf::Text text(sf::String::fromUtf8(label.begin(),label.end()),*font,16);
    text.setScale(.5f,.5f);
    text.setFillColor(disabled ? disabledColor : sf::Color::White);
    if (!useAboveLabel) text.setPosition(x-text.getLocalBounds().width*.5f-7.f,y+h*.5f-6.f);
    else text.setPosition(static_cast<float>(x),static_cast<float>(y-8)-3.f);
    target.draw(text);
    if (bound && boundBuffer) {
        text.setString(sf::String::fromUtf8(boundBuffer->begin(),boundBuffer->end()));
        text.setPosition(static_cast<float>(x+3),static_cast<float>(h/2+y-6));
        target.draw(text);
    }
    if (!focus) return;
    auto start=text.findCharacterPos(input.textCursorIndexStart);
    auto end=text.findCharacterPos(input.textCursorIndexEnd);
    const auto alpha=static_cast<sf::Uint8>(static_cast<int>(std::sin(static_cast<float>(frame)*.05f)*50.f+110.f));
    const auto selection=[&](sf::Vector2f position, sf::Vector2f size) {
        sf::RectangleShape highlight(size);
        highlight.setPosition(position);
        highlight.setFillColor(sf::Color(255,255,255,alpha));
        target.draw(highlight);
    };
    if (start.y==end.y) {
        selection({start.x-1.f,start.y},{end.x-start.x+1.f,9.f});
    } else {
        if (end.y<start.y) std::swap(start,end);
        const float width=text.getLocalBounds().width*.5f;
        const float tx=text.getPosition().x;
        selection({start.x-1.f,start.y},{width-(start.x-tx)+1.f,9.f});
        selection({tx-1.f,end.y},{end.x-tx+1.f,9.f});
        const int middle=static_cast<int>(std::round((end.y-start.y)*.125f))-1;
        for (int line=0,dy=8;line<middle;++line,dy+=8)
            selection({tx,start.y+dy},{width,9.f});
    }
}
// Small field wrappers were inlined, with no separate bodies in the unit index.
void GuiTextBox::setBgLabel(std::string text) { bgLabel=std::move(text); }
void GuiTextBox::setLabel(std::string text) { label=std::move(text); }
bool GuiTextBox::pollAction() { const bool result=doAction;doAction=false;return result; }
std::string GuiTextBox::getString() { return bound && boundBuffer ? *boundBuffer : std::string(); }
