#include "recon_all.h"
#include <cassert>
#include <iostream>
struct Probe : GuiElement {
    using GuiElement::GuiElement;
    void draw(sf::RenderTarget &) override {}
    float alpha() { return alphaModifier; }
    sf::Color highlight() { return hoverColor; }
    sf::Color muted() { return disabledColor; }
};
int main() {
    Probe p(GuiElement::BUTTON,10,20,30,40);
    assert(!p.hover && !p.isHidden() && p.isEnabled() && !p.isPressed());
    assert(p.cursorOffset==sf::Vector2f(0,11) && p.alpha()==1 && p.getToolTip().empty());
    assert(p.contains(10,20) && p.contains(39,59) && !p.contains(40,20) && !p.contains(10,60));
    p.setOffset(100,200);assert(p.getOffset()==sf::Vector2f(100,200) && p.contains(10,20));
    p.setHidden(true);assert(!p.contains(10,20));p.setHidden(false);
    p.disable();assert(!p.isEnabled());p.enable();assert(p.isEnabled());p.setEnabled(false);assert(!p.isEnabled());
    p.setPosition(2,3);p.setX(4);p.setY(5);assert(p.getX()==4 && p.getY()==5 && p.getW()==30 && p.getH()==40);
    p.setTooltip("tip");p.setAlpha(.25f);assert(p.getToolTip()=="tip" && p.alpha()==.25f);
    p.action(true);p.mouseAction(0,0,true,true);p.step(0,0);p.afterStep(true,true);assert(!p.isPressed());
    assert(p.highlight()==sf::Color(90,255,255) && p.muted()==sf::Color(100,100,100));
    std::cout<<"GuiElement: construction, bounds, accessors, no-op virtuals and recovered colors passed\n";
}
