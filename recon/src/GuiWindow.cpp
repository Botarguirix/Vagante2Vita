// GuiWindow: 005ec3e0-005ec697. Constructor/add are inlined in EditorState::EditorState.
#include "recon_all.h"
GuiWindow::GuiWindow(float px, float py, float width, float height)
    : GuiElement(WINDOW,static_cast<int>(px),static_cast<int>(py),static_cast<int>(width),static_cast<int>(height)),
      elements(),hasBg(true) {}
GuiWindow::~GuiWindow() {
    // Destruction recovered from the scalar deleting wrapper @005ec330.
    for (std::size_t i=0;i<elements.size();++i) delete elements[i];
}
void GuiWindow::mouseAction(int px, int py, bool down, bool left) {
    for (std::size_t i=0;i<elements.size();++i) {
        auto *element=elements[i];
        if (element->contains(px,py)) element->mouseAction(px,py,down,left);
    }
}
void GuiWindow::step(int px, int py) {
    for (std::size_t i=0;i<elements.size();++i) elements[i]->step(px,py);
}
void GuiWindow::afterStep(bool left, bool right) {
    for (std::size_t i=0;i<elements.size();++i) elements[i]->afterStep(left,right);
}
void GuiWindow::draw(sf::RenderTarget &target) {
    if (hidden) return;
    if (hasBg) {
        sf::RectangleShape box({static_cast<float>(w),static_cast<float>(h)});
        box.setPosition(static_cast<float>(x),static_cast<float>(y));
        box.setOutlineThickness(1.f);
        box.setFillColor(sf::Color::Black);
        target.draw(box);
    }
    for (std::size_t i=0;i<elements.size();++i) elements[i]->draw(target);
}
void GuiWindow::setOffset(float px, float py) {
    // No assignment to this->offset in the original; only forward to children.
    for (std::size_t i=0;i<elements.size();++i) elements[i]->setOffset(px,py);
}
void GuiWindow::add(GuiElement *element) { elements.push_back(element); }
void GuiWindow::setHasBg(bool value) { hasBg=value; }
