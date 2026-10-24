#include "port_lib_types.h"
#define private public
#include "ResourceManager.h"
#undef private
#include "recon_all.h"
#include <cassert>
#include <iostream>
struct Child : GuiElement {
    int id;std::vector<int> &events;int &deaths;
    GuiWindow *appendTo=nullptr;
    Child(int number,std::vector<int> &log,int &dead) : GuiElement(BUTTON,10,10,10,10),id(number),events(log),deaths(dead) {}
    ~Child() override { ++deaths; }
    void mouseAction(int x,int y,bool a,bool b) override { assert(x==10 && y==10 && a && !b);events.push_back(id); }
    void step(int x,int y) override { assert(x==4 && y==5);events.push_back(id+10);if(appendTo){appendTo->add(new Child(3,events,deaths));appendTo=nullptr;} }
    void afterStep(bool a,bool b) override { assert(!a && b);events.push_back(id+20); }
    void draw(sf::RenderTarget &) override { events.push_back(id+30); }
};
int main() {
    std::vector<int> events;int deaths=0;
    sf::RenderTexture target;assert(target.create(64,64));
    {
        GuiWindow window(2.9f,3.9f,40.9f,30.9f);
        assert(window.type==GuiElement::WINDOW && window.getX()==2 && window.getY()==3 && window.getW()==40 && window.getH()==30);
        auto *a=new Child(1,events,deaths),*b=new Child(2,events,deaths);window.add(a);window.add(b);
        window.mouseAction(10,10,true,false);assert((events==std::vector<int>{1,2}));events.clear();
        b->setHidden(true);window.mouseAction(10,10,true,false);assert((events==std::vector<int>{1}));events.clear();
        window.mouseAction(20,10,true,false);assert(events.empty());
        a->disable();window.mouseAction(10,10,true,false);assert((events==std::vector<int>{1}));events.clear();
        window.setHidden(true);window.step(4,5);window.afterStep(false,true);assert((events==std::vector<int>{11,12,21,22}));events.clear();
        window.mouseAction(10,10,true,false);assert((events==std::vector<int>{1}));events.clear();
        window.setOffset(30,40);assert(window.getOffset()==sf::Vector2f(0,0) && a->getOffset()==sf::Vector2f(30,40) && b->getOffset()==sf::Vector2f(30,40));
        target.clear(sf::Color::Magenta);window.draw(target);target.display();assert(events.empty() && target.getTexture().copyToImage().getPixel(8,8)==sf::Color::Magenta);
        window.setHidden(false);window.draw(target);target.display();assert((events==std::vector<int>{31,32}) && target.getTexture().copyToImage().getPixel(8,8)==sf::Color::Black);events.clear();
        window.setHasBg(false);target.clear(sf::Color::Magenta);window.draw(target);target.display();assert(target.getTexture().copyToImage().getPixel(8,8)==sf::Color::Magenta);events.clear();
        a->appendTo=&window;window.step(4,5);assert((events==std::vector<int>{11,12,13}));events.clear();
    }
    assert(deaths==3);
    {
        GuiWindow parent(0,0,10,10);auto *nested=new GuiWindow(0,0,10,10);auto *child=new Child(1,events,deaths);
        nested->add(child);parent.add(nested);parent.setOffset(7,8);assert(child->getOffset()==sf::Vector2f(7,8) && nested->getOffset()==sf::Vector2f(0,0));
    }
    assert(deaths==4);
    // Exercise the translated controls together, including the font path that
    // the isolated atlas test deliberately runs without a loaded font.
    Config config;ResourceManager::setConfig(&config);
    sf::Font font;assert(font.loadFromFile("C:/Windows/Fonts/arial.ttf"));
    ResourceManager::fonts["Tiny_04b.ttf"]=&font;
    ResourceManager::fonts["NotoSansCJKkr-Regular.otf"]=&font;
    ResourceManager::muted=true;
    Input input(nullptr);std::string value="abc";
    {
        GuiWindow window(0,0,64,64);
        auto *button=new GuiButton("go",2,3,30,20,"tip",false);
        auto *text=new GuiTextBox(input,2,30,50,10);
        text->bindString(&value);text->setLabel("value");
        window.add(button);window.add(text);
        window.mouseAction(3,4,true,true);window.mouseAction(3,4,false,true);
        assert(button->pollAction() && !button->pollAction());
        button->hover=true;window.draw(target);
        button->disable();window.draw(target);button->setUseBackground(true);window.draw(target);
        button->enable();window.draw(target);button->action(true);window.draw(target);
        window.mouseAction(3,31,true,true);assert(input.enteringText);
        input.inputChar('d');assert(value=="abcd");window.draw(target);
    }
    assert(!input.enteringText);
    ResourceManager::muted=false;ResourceManager::fonts.clear();
    std::cout<<"GuiWindow: dispatch/order, bounds, hidden, render, forwarding, mutation during step and owned destruction passed\n";
}
