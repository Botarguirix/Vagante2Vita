// GuiCursor.cpp: PDB contract + Ghidra 005e5fe0-005e64ff.
#include "recon_all.h"
#include <cmath>

// GuiCursor::GuiCursor @ 005e5fe0. Sprite arguments recovered from the call site
// (tools/call_args.py): 18x18 frames, 5 ticks per frame, looping.
GuiCursor::GuiCursor(float startX, float startY)
    : x(startX), y(startY), alphaModifier(1.0f), target(nullptr),
      selectHoverSprite(std::weak_ptr<Entity>(), "generic_select.png", 18, 18, -1, 5, 0.0f, 0.0f, true, false),
      selectGrabSprite(std::weak_ptr<Entity>(), "grab_confirm.png", 18, 18, -1, 5, 0.0f, 0.0f, true, false) {}

GuiCursor::~GuiCursor() {}

GuiElement *GuiCursor::getTarget() const { return target; }
void GuiCursor::setTarget(GuiElement *element) { target = element; }
void GuiCursor::setAlpha(float alpha) { alphaModifier = alpha; }

// GuiCursor::step @ 005e6150: animate both sprites and ease halfway towards the target each tick.
void GuiCursor::step() {
    selectHoverSprite.step(sf::Vector2<int>(0, 0), sf::Vector2<float>(1.0f, 1.0f));
    selectGrabSprite.step(sf::Vector2<int>(0, 0), sf::Vector2<float>(1.0f, 1.0f));
    if (target) {
        x += (static_cast<float>(target->getX()) + target->cursorOffset.x - x) * 0.5f;
        y += (static_cast<float>(target->getY()) + target->cursorOffset.y - y) * 0.5f;
    }
}

// GuiCursor::draw @ 005e6220: the bob always follows selectHoverSprite's animation, also while
// the grab sprite is shown (as in the original).
void GuiCursor::draw(sf::RenderTarget &target_) {
    if (!target) return;
    const float bob = std::cos(selectHoverSprite.percentComplete() * 2.0f * 3.14159265f) * 0.5f;
    Sprite &sprite = target->isPressed() ? selectGrabSprite : selectHoverSprite;
    sprite.setPosition(x, y + bob);
    sprite.draw(target_, sf::RenderStates::Default);
}
