#include "port_lib_types.h"
#define private public
#define protected public
#include "GuiTextBox.h"
#include "Input.h"
#include "ResourceManager.h"
#undef protected
#undef private
#include "recon_all.h"
#include <cassert>
#include <iostream>
int main() {
    Input input(nullptr); std::string value="abc";
    GuiTextBox box(input,30.9f,20.1f,60.8f,99.f);
    assert(box.getX()==30 && box.getY()==20 && box.getW()==60 && box.getH()==10 && !box.focus && !box.bound);
    box.startEnteringText(false);assert(box.focus && !input.enteringText);box.step(0,0);assert(!box.focus && box.pollAction() && !box.pollAction());
    box.bindString(&value);box.startEnteringText(false);assert(input.enteringText && input.textCursorIndexStart==3 && input.textCursorIndexEnd==3 && !input.allowNewLines);
    input.inputChar('d');assert(value=="abcd" && box.getString()==value);
    box.stopEnteringText();assert(!input.enteringText && box.focus);box.step(0,0);assert(!box.focus && box.pollAction());
    box.setHidden(true);box.mouseAction(0,0,true,true);assert(!box.focus);box.setHidden(false);
    box.disable();box.mouseAction(0,0,true,true);assert(!box.focus);box.enable();
    box.mouseAction(0,0,true,false);assert(!box.focus);box.mouseAction(0,0,true,true);assert(box.focus && input.enteringText && input.allowNewLines);
    std::string other="other"; input.setEnteringText(&other,2,false);box.stopEnteringText();assert(input.enteringText && input.textBuffer==&other);
    { GuiTextBox temporary(input,0,0,10,10);temporary.bindString(&other); }
    assert(!input.enteringText && input.textBuffer==nullptr);
    box.startEnteringText(true);assert(input.textBuffer==&value);
    sf::Font font;assert(font.loadFromFile("C:/Windows/Fonts/arial.ttf"));
    ResourceManager::fonts["NotoSansCJKkr-Regular.otf"]=&font;
    sf::RenderTexture target;assert(target.create(160,100));
    box.setLabel("Name");box.setBgLabel("unused");box.setOffset(40,40);box.setAlpha(0);
    target.clear(sf::Color::Magenta);box.draw(target);target.display();
    assert(target.getTexture().copyToImage().getPixel(80,25)!=sf::Color::Magenta);
    box.useAboveLabel=true;box.disable();box.hover=true;box.draw(target);
    value="abc\ndef\nghi";input.textCursorIndexStart=0;input.textCursorIndexEnd=10;box.draw(target);
    std::swap(input.textCursorIndexStart,input.textCursorIndexEnd);box.draw(target);
    input.textCursorIndexEnd=input.textCursorIndexStart;box.draw(target);
    box.setHidden(true);target.clear(sf::Color::Magenta);box.draw(target);target.display();assert(target.getTexture().copyToImage().getPixel(80,25)==sf::Color::Magenta);
    box.bindString(nullptr);box.startEnteringText(false);assert(!input.enteringText);
    ResourceManager::fonts.clear();
    std::cout<<"GuiTextBox: binding, input ownership, focus/actions, destructor, rendering and multiline selection passed\n";
}
