// Reconstructed from GuiElement 005e64f0-005e65f0 and inlined accessors in GUI callers.
#include "recon_all.h"
const sf::Color GuiElement::hoverColor(90, 255, 255, 255); // initializer 00402ac0
const sf::Color GuiElement::disabledColor(100, 100, 100, 255); // initializer 00402ae0
GuiElement::GuiElement(GuiElementType kind, int px, int py, int width, int height)
    : hover(false), type(kind), cursorOffset(0.f, 11.f), tooltip(), hidden(false),
      pressed(false), disabled(false), x(px), y(py), w(width), h(height),
      offset(0.f, 0.f), alphaModifier(1.f) {}
GuiElement::~GuiElement() = default;
void GuiElement::mouseAction(int, int, bool, bool) {}
void GuiElement::action(bool) {}
void GuiElement::step(int, int) {}
void GuiElement::afterStep(bool, bool) {}
void GuiElement::setOffset(float px, float py) { offset = {px, py}; }
// These small accessors were inlined in the original. contains is expanded in
// GuiWindow::mouseAction: hidden excludes hits, offset does not affect bounds.
bool GuiElement::contains(int px, int py) { return !hidden && x <= px && y <= py && px < x+w && py < y+h; }
void GuiElement::setAlpha(float value) { alphaModifier=value; }
void GuiElement::setTooltip(std::string value) { tooltip=std::move(value); }
void GuiElement::setHidden(bool value) { hidden=value; }
void GuiElement::setEnabled(bool value) { disabled=!value; }
void GuiElement::disable() { disabled=true; }
void GuiElement::enable() { disabled=false; }
bool GuiElement::isHidden() { return hidden; }
bool GuiElement::isEnabled() { return !disabled; }
void GuiElement::setX(int value) { x=value; }
void GuiElement::setY(int value) { y=value; }
void GuiElement::setPosition(int px, int py) { x=px; y=py; }
sf::Vector2<float> &GuiElement::getOffset() { return offset; }
int GuiElement::getX() { return x; }
int GuiElement::getY() { return y; }
int GuiElement::getW() { return w; }
int GuiElement::getH() { return h; }
std::string GuiElement::getToolTip() { return tooltip; }
bool GuiElement::isPressed() { return pressed; }
