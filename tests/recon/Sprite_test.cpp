#include "port_lib_types.h"
#define private public
#include "Sprite.h"
#include "SpriteEntityInterface.h"
#include "ResourceManager.h"
#undef private
#include "recon_all.h"
#include <cassert>
#include <iostream>

struct SpriteOwnerFixture : Entity {
    SpriteOwnerFixture() : Entity(nullptr, Entity::TYPE_DOODAD, 0, 0, 1, 1) {}
    void step() override {}
    void draw(sf::RenderTarget &) override {}
    void doCollision(const std::shared_ptr<Entity> &) override {}
};

int main() {
    sf::Image atlas;
    atlas.create(4, 4, sf::Color::Red);
    for (unsigned y = 0; y < 4; ++y) for (unsigned x = 0; x < 4; ++x)
        atlas.setPixel(x, y, y < 2 ? (x < 2 ? sf::Color::Red : sf::Color::Green)
                                  : (x < 2 ? sf::Color::Blue : sf::Color::White));
    auto *texture = new sf::Texture;
    assert(texture->loadFromImage(atlas));
    ResourceManager::textures.emplace("fixture-atlas", texture);
    Sprite empty;
    assert(empty.isEmpty && empty.getFrame() == 0 && empty.getNumFrames() == 1);
    assert(empty.getOrigin() == sf::Vector2f(0.5f, 0.5f));
    Sprite animation({}, "fixture-atlas", 2, 2, -1, 2, 3, 4, true, false);
    assert(animation.getNumFrames() == 4 && animation.cols == 2 && animation.rows == 2);
    assert(animation.getTexture() == texture && animation.getOrigin() == sf::Vector2f(1, 1));
    animation.setPosition(10, 20);
    assert(animation.getPosition() == sf::Vector2f(10, 20));
    assert(animation.getPositionNoOffset() == sf::Vector2f(7, 16));
    animation.setScale(-2, -3);
    animation.setFlipped(true);
    assert(animation.getScale() == sf::Vector2f(2, -3));
    assert(static_cast<const sf::Transformable &>(animation).getScale().x == -2);
    animation.setPosition(sf::Vector2f(10, 20));
    assert(animation.getPositionNoOffset() == sf::Vector2f(13, 16));
    assert(animation.getPosition() == sf::Vector2f(10, 20));
    animation.setFlipped(false);

    animation.step({0, 0}, {1, 1}); animation.step({0, 0}, {1, 1});
    assert(animation.getFrame() == 0 && animation.getTime() == 2);
    animation.step({0, 0}, {1, 1});
    assert(animation.getFrame() == 1 && animation.getTime() == 0);
    assert(animation.getTextureRect() == sf::IntRect(2, 0, 2, 2));
    animation.setFrozen(true);
    animation.step({5, 6}, {0.5f, 2});
    assert(animation.getTime() == 0 && animation.getTextureRect() == sf::IntRect(2, 0, 2, 2));
    animation.setFrozen(false);
    animation.step({5, 6}, {0.5f, 2});
    assert(animation.getFrame() == 1 && animation.getTextureRect() == sf::IntRect(7, 6, 1, 4));
    animation.setFrame(99);
    assert(animation.getFrame() == 3 && animation.getTime() == 0);
    animation.setFrame(-1);
    assert(animation.getFrame() == 0);
    animation.setLoop(false); animation.setSpeed(0);
    for (int i = 0; i < 4; ++i) animation.step({0, 0}, {1, 1});
    assert(animation.getFrame() == 3 && animation.isFinished());
    animation.setReverse(true); animation.reset();
    assert(!animation.isFinished() && animation.getFrame() == 3 && animation.getTime() == 0);
    for (int i = 0; i < 4; ++i) animation.step({0, 0}, {1, 1});
    assert(animation.getFrame() == 0 && animation.isFinished());
    animation.setReverse(false); animation.setLoop(true); animation.setSpeed(2);
    animation.setResetDelay(3); animation.reset();
    for (int i = 0; i < 4; ++i) animation.step({0, 0}, {1, 1});
    assert(animation.getFrame() == 0 && animation.getTime() == 4);
    animation.step({0, 0}, {1, 1});
    assert(animation.getFrame() == 1 && animation.getTime() == 0);

    animation.setColor(sf::Color(12, 34, 56, 78));
    animation.setAlpha(90); animation.setColorComponents(1, 2, 3);
    assert(animation.getColor(3) == sf::Color(1, 2, 3, 90));
    animation.setColor(sf::Color::Yellow, 2);
    animation.setTime(7);
    animation.m_paletteGroupName = "not copied";
    Sprite copy(animation);
    assert(copy.getTime() == 0 && copy.getFrame() == animation.getFrame());
    assert(copy.getColor(2) == sf::Color::Yellow && copy.m_paletteGroupName.empty());
    Sprite withFlags(animation, true, true);
    assert(withFlags.getFrame() == animation.getFrame() && withFlags.getTime() == 0);
    copy = std::move(copy);
    assert(copy.getFrame() == animation.getFrame());

    sf::RenderTexture target;
    assert(target.create(4, 4));
    Sprite visible({}, "fixture-atlas", 2, 2, 4, 0, 0, 0, true, false);
    visible.setOrigin(0, 0); visible.setPositionNoOffset(0, 0); visible.setScale(2);
    const sf::Color expected[] = {sf::Color::Red, sf::Color::Green, sf::Color::Blue, sf::Color::White};
    for (int f = 0; f < 4; ++f) {
        visible.setFrame(f); target.clear(sf::Color::Transparent);
        visible.draw(target, sf::RenderStates::Default); target.display();
        assert(target.getTexture().copyToImage().getPixel(1, 1) == expected[f]);
    }
    assert(target.getTexture().copyToImage().saveToFile("sprite-fixture.png"));
    auto *processed = new sf::RenderTexture;
    assert(processed->create(8, 8));
    ResourceManager::processedTextures.emplace("@fixture", processed);
    Sprite padded({}, "@fixture", 2, 2, -1, 0, 0, 0, true, false);
    assert(padded.tileProcessed && padded.cols == 2 && padded.rows == 2);
    assert(padded.getNumFrames() == 16); // Original counts before changing to the padded grid.
    padded.setFrame(1);
    assert(padded.getTextureRect() == sf::IntRect(5, 1, 2, 2));
    padded.step({0, 0}, {1, 1});
    assert(padded.getTextureRect() == sf::IntRect(0, 2, 2, 2));

    auto entity = std::make_shared<SpriteOwnerFixture>();
    {
        Sprite owned(entity, "fixture-atlas", 2, 2, 4, 0, 0, 0, true, false);
        assert(entity->entitySprites.size() == 1);
        Sprite clone(owned, false, false);
        assert(entity->entitySprites.size() == 2);
        Sprite relocated;
        relocated = std::move(clone);
        assert(clone.m_ownerEntity.expired() && entity->entitySprites.size() == 2);
        assert(std::find(entity->entitySprites.begin(), entity->entitySprites.end(), &relocated) != entity->entitySprites.end());
        entity->SpriteEntityInterface::setPalette("group", "palette");
        assert(owned.m_paletteGroupName == "group" && relocated.m_paletteName == "palette");
    }
    assert(entity->entitySprites.empty());
    std::cout << "Sprite: animation boundaries, reverse/delay/freeze, transforms, copy/move, owner lifecycle and rendered atlas passed\n";
    ResourceManager::textures.clear(); ResourceManager::processedTextures.clear();
    delete texture; delete processed;
}
