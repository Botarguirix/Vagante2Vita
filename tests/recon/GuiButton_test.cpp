#include "port_lib_types.h"
#define private public
#define protected public
#include "GuiButton.h"
#include "ResourceManager.h"
#undef protected
#undef private
#include "recon_all.h"
#include <cassert>
#include <iostream>
int main() {
    Config config; ResourceManager::setConfig(&config);
    sf::Image atlas;atlas.create(24,48,sf::Color::Red);
    sf::Texture texture;assert(texture.loadFromImage(atlas));
    sf::RenderTexture processed;assert(processed.create(30,60));processed.clear(sf::Color::Red);processed.display();
    ResourceManager::processedTextures["@gui/gui_button_nine_slice.png"]=&processed;
    ResourceManager::textures["gui/gui_tabs.png"]=&texture;
    GuiButton b("go",2,3,24,24,"tip",false);
    assert(b.type==GuiElement::BUTTON && b.getX()==2 && b.getY()==3 && b.getToolTip()=="tip");
    assert(b.centered && !b.isATab && !b.getToggleState() && b.hoverTime==0);
    b.action(false);assert(!b.pollAction());b.action(true);assert(b.isPressed() && !b.pollAction());
    b.action(false);assert(!b.isPressed() && b.doAction);b.doAction=false;
    b.disable();b.action(true);assert(!b.isPressed());b.enable();
    b.mouseAction(0,0,true,false);b.mouseAction(0,0,false,false);assert(b.doAction && b.wasRightClick());b.doAction=false;
    b.mouseAction(0,0,true,true);b.mouseAction(0,0,false,true);assert(b.doAction && !b.wasRightClick());b.doAction=false;
    b.hover=true;b.step(0,0);assert(b.hoverTime==1);b.action(true);b.afterStep(true,false);assert(b.isPressed());
    b.hover=false;b.afterStep(true,false);assert(!b.isPressed() && b.doAction);b.doAction=false;
    b.step(0,0);assert(b.hoverTime==0);b.setHidden(true);b.mouseAction(0,0,true,true);assert(!b.isPressed());
    b.setHidden(false);b.action(true);b.hover=true;b.afterStep(false,false);assert(!b.isPressed() && b.doAction);b.doAction=false;
    sf::RenderTexture target;assert(target.create(64,64));
    b.setToggleState(true);b.setAlpha(1);target.clear(sf::Color::Magenta);b.draw(target);target.display();
    assert(target.getTexture().copyToImage().getPixel(10,10)==sf::Color(0,66,0));
    b.setHidden(true);target.clear(sf::Color::Magenta);b.draw(target);target.display();assert(target.getTexture().copyToImage().getPixel(10,10)==sf::Color::Magenta);
    b.setHidden(false);b.setUseBackground(true);b.hover=false;b.draw(target);assert(b.buttonSprite.getFrame()==8);
    b.action(true);b.draw(target);assert(b.buttonSprite.getFrame()==17);
    b.hover=true;b.setAlpha(.5f);b.draw(target);assert(b.buttonSprite.getColor().a==127);
    b.updateName("new");assert(b.getName()=="new");
    ResourceManager::textures.clear();ResourceManager::processedTextures.clear();
    std::cout<<"GuiButton: press/release, hidden/disabled, both mouse buttons, hover, afterStep, outline/atlas rendering passed\n";
}
