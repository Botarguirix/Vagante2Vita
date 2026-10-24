// Sprite.cpp: PDB contract + Ghidra 007b86d0-007ba381.
#include "recon_all.h"
#include <algorithm>
#include <cmath>
#include <utility>
#define VLOG(level) if ((level) > Log::reportingLevel()) ; else Log().get(level)

namespace {
// The PDB generator did not recover Sprite's friendship with its owner
// interface. Explicit instantiation permits naming the private member here
// without changing the generated declaration, its layout or its access labels.
template<class Tag, typename Tag::Type Member> struct OwnerAccess {
    friend typename Tag::Type ownerMethod(Tag) { return Member; }
};
struct AddSprite { using Type = void (SpriteEntityInterface::*)(Sprite *); friend Type ownerMethod(AddSprite); };
struct RemoveSprite { using Type = void (SpriteEntityInterface::*)(const Sprite *); friend Type ownerMethod(RemoveSprite); };
template struct OwnerAccess<AddSprite, &SpriteEntityInterface::addSprite>;
template struct OwnerAccess<RemoveSprite, &SpriteEntityInterface::removeSprite>;
void registerSprite(const std::shared_ptr<Entity> &entity, Sprite *sprite) {
    (entity.get()->*ownerMethod(AddSprite{}))(sprite);
}
void unregisterSprite(const std::shared_ptr<Entity> &entity, const Sprite *sprite) {
    (entity.get()->*ownerMethod(RemoveSprite{}))(sprite);
}
}

Sprite::Sprite()
    : flipped(false), isEmpty(true), pTexture(nullptr), textureRect(0, 0, 1, 1),
      w(0), h(0), frame(0), numFrames(1), timer(0), speed(0), resetDelay(0),
      offset(0, 0), reverse(false), frozen(false), loop(false), done(false),
      rows(1), cols(1), tileProcessed(false), textureIsMask(false), textureIsMaskOutline(false) {
    setOrigin(0.5f, 0.5f);
}
Sprite::Sprite(std::weak_ptr<Entity> owner, const std::string &name,
               int width, int height, int frames, int ticks, float dx, float dy,
               bool looping, bool mask)
    : flipped(false), isEmpty(false), pTexture(nullptr), textureRect(0, 0, width, height),
      w(width), h(height), frame(0), numFrames(frames), timer(0), speed(ticks), resetDelay(0),
      offset(dx, dy), reverse(false), frozen(false), loop(looping), done(false),
      rows(1), cols(1), tileProcessed(false), textureIsMask(mask), textureIsMaskOutline(false),
      m_textureName(name) {
    setOrigin(w * 0.5f, h * 0.5f);
    lazyLoadTexture();
    if (auto entity = owner.lock()) { m_ownerEntity = owner; registerSprite(entity, this); }
}
Sprite::Sprite(const Sprite &other)
    : flipped(other.flipped), isEmpty(other.isEmpty), pTexture(other.pTexture),
      textureRect(other.textureRect), w(other.w), h(other.h), frame(other.frame),
      numFrames(other.numFrames), timer(other.timer), speed(other.speed), resetDelay(other.resetDelay),
      offset(other.offset), reverse(other.reverse), frozen(other.frozen), loop(other.loop), done(other.done),
      rows(other.rows), cols(other.cols), tileProcessed(other.tileProcessed),
      textureIsMask(other.textureIsMask), textureIsMaskOutline(other.textureIsMaskOutline),
      m_textureName(other.m_textureName) {
    setRotation(other.getRotation());
    setScale(other.getScale());
    setPosition(other.getPosition());
    setOrigin(other.getOrigin());
    setFrame(frame); // Also resets the copied timer, even when frame is unchanged.
    updatePositions();
    updateTexCoords();
    for (unsigned i = 0; i < 4; ++i) vertices[i].color = other.vertices[i].color;
    // The original leaves both palette names empty, then lets the owner apply them.
    if (auto entity = other.m_ownerEntity.lock()) {
        m_ownerEntity = other.m_ownerEntity;
        registerSprite(entity, this);
    }
}
Sprite::Sprite(const Sprite &other, bool, bool) : Sprite(other) {
    // Both bools are unused in the compiled body @007b8f70.
    setRotation(other.getRotation());
    setScale(other.getScale());
    setPosition(other.getPosition());
    setOrigin(other.getOrigin());
    if (auto entity = other.m_ownerEntity.lock()) {
        m_ownerEntity = other.m_ownerEntity;
        registerSprite(entity, this);
    }
}
Sprite::~Sprite() { if (auto entity = m_ownerEntity.lock()) unregisterSprite(entity, this); }
Sprite &Sprite::operator=(Sprite &&other) {
    if (this == &other) return *this;
    flipped = other.flipped; isEmpty = other.isEmpty; pTexture = other.pTexture;
    textureRect = other.textureRect; w = other.w; h = other.h;
    frame = other.frame; numFrames = other.numFrames; timer = other.timer;
    speed = other.speed; resetDelay = other.resetDelay; offset = other.offset;
    reverse = other.reverse; frozen = other.frozen; loop = other.loop; done = other.done;
    rows = other.rows; cols = other.cols; tileProcessed = other.tileProcessed;
    textureIsMask = other.textureIsMask; textureIsMaskOutline = other.textureIsMaskOutline;
    m_textureName = other.m_textureName;
    m_paletteGroupName = other.m_paletteGroupName; m_paletteName = other.m_paletteName;
    m_ownerEntity = other.m_ownerEntity;
    setRotation(other.getRotation()); setScale(other.getScale());
    setPosition(other.getPosition()); setOrigin(other.getOrigin());
    setFrame(frame); updatePositions(); updateTexCoords();
    for (unsigned i = 0; i < 4; ++i) vertices[i].color = other.vertices[i].color;
    if (auto entity = m_ownerEntity.lock()) {
        unregisterSprite(entity, &other);
        other.m_ownerEntity.reset();
        registerSprite(entity, this);
    }
    return *this;
}
Sprite &Sprite::operator=(Sprite &other) {
    // No compiled lvalue body is present; compose the recovered copy and move
    // operations instead of transferring the lvalue's owner registration.
    if (this != &other) { Sprite copy(other); *this = std::move(copy); }
    return *this;
}

void Sprite::step(const sf::Vector2i &displacement, const sf::Vector2f &sizeFactor) {
    if (frozen) return;
    ++timer;
    if (speed < timer) {
        if (frame == 0 && timer < resetDelay + speed) return;
        frame += reverse ? -1 : 1;
        if (frame < 0) {
            if (loop) frame = numFrames - 1;
            else { frame = 0; done = true; }
        }
        if (numFrames <= frame) {
            if (loop) frame = 0;
            else { frame = numFrames - 1; done = true; }
        }
        timer = 0;
    } else if (displacement == sf::Vector2i() && sizeFactor == sf::Vector2f(1, 1)) return;
    // step/reset use the unpadded grid, unlike setFrame. Preserve that distinction.
    setTextureRect({w * (frame % cols) + displacement.x, h * (frame / cols) + displacement.y,
                    static_cast<int>(w * sizeFactor.x), static_cast<int>(h * sizeFactor.y)});
}
void Sprite::reset() {
    frame = reverse ? numFrames - 1 : 0;
    timer = 0; done = false;
    setTextureRect({w * (frame % cols), h * (frame / cols), w, h});
}
void Sprite::setFrame(int value) {
    timer = 0;
    if (frame == value) return;
    frame = std::max(0, std::min(value, numFrames - 1));
    textureRect.left = tileProcessed ? (w + 2) * (frame % cols) + 1 : w * (frame % cols);
    textureRect.top = tileProcessed ? (h + 2) * (frame / cols) + 1 : h * (frame / cols);
    updateTexCoords();
}
void Sprite::setTextureRect(const sf::IntRect &rect) {
    if (rect == textureRect) return;
    textureRect = rect;
    updatePositions(); updateTexCoords();
}
void Sprite::updatePositions() {
    vertices[1].position = {0, static_cast<float>(textureRect.height)};
    vertices[2].position = {static_cast<float>(textureRect.width), 0};
    vertices[3].position = {static_cast<float>(textureRect.width), static_cast<float>(textureRect.height)};
}
void Sprite::updateTexCoords() {
    const float left = static_cast<float>(textureRect.left), top = static_cast<float>(textureRect.top);
    const float right = static_cast<float>(textureRect.width) + left;
    const float bottom = static_cast<float>(textureRect.height) + top;
    vertices[0].texCoords = {left, top}; vertices[1].texCoords = {left, bottom};
    vertices[2].texCoords = {right, top}; vertices[3].texCoords = {right, bottom};
}
void Sprite::setTexture(const sf::Texture * const texture, bool resetRect) {
    if (!texture) return;
    if (resetRect || (!pTexture && textureRect == sf::IntRect())) {
        const auto size = texture->getSize();
        setTextureRect({0, 0, static_cast<int>(size.x), static_cast<int>(size.y)});
    }
    pTexture = texture;
}
void Sprite::lazyLoadTexture() {
    if (pTexture) return;
    const auto origin = getOrigin();
    const float originX = origin.x / static_cast<float>(w ? w : 1);
    const float originY = origin.y / static_cast<float>(h ? h : 1);
    if (textureIsMask) setTexture(ResourceManager::getTextureMask(m_textureName), false);
    else if (textureIsMaskOutline) setTexture(ResourceManager::getTextureOutlineMask(m_textureName), false);
    else if (!m_paletteGroupName.empty() && !m_paletteName.empty()) {
        const auto *texture = ResourceManager::getTextureAltPalette(m_textureName, m_paletteGroupName, m_paletteName);
        if (texture) setTexture(texture, false);
        else { VLOG(LOG_WARNING) << "Sprite::lazyLoadTexture: palette swap not found! {" << m_textureName << ',' << m_paletteGroupName << ',' << m_paletteName << '}'; }
    } else setTexture(ResourceManager::getTexture(m_textureName, false), false);
    if (!pTexture) return;
    const auto size = pTexture->getSize();
    if (static_cast<int>(size.x) < w || w < 1) { w = size.x; textureRect.width = w; }
    if (static_cast<int>(size.y) < h || h < 1) { h = size.y; textureRect.height = h; }
    setOrigin(w * originX, h * originY);
    rows = size.y / h; cols = size.x / w;
    if (numFrames == -1) numFrames = rows * cols;
    if (!m_textureName.empty() && m_textureName.front() == '@') {
        tileProcessed = true;
        rows = size.y / (h + 2u); cols = size.x / (w + 2u);
    }
    updatePositions(); updateTexCoords();
}
void Sprite::draw(sf::RenderTarget &target, sf::RenderStates states) {
    lazyLoadTexture();
    states.texture = pTexture;
    if (!states.texture) return;
    states.transform *= getTransform();
    target.draw(vertices, 4, sf::TriangleStrip, states);
}

void Sprite::setPalette(const std::string &group, const std::string &name) {
    pTexture = nullptr; m_paletteGroupName = group; m_paletteName = name;
}
void Sprite::setFlipped(bool value) {
    if (flipped == value) return;
    flipped = value;
    const auto scale = sf::Transformable::getScale();
    setScale(value ? -std::abs(scale.x) : std::abs(scale.x), scale.y);
}
bool Sprite::getFlipped() const { return flipped; }
void Sprite::setPosition(const sf::Vector2f &position) { setPosition(position.x, position.y); }
void Sprite::setPosition(float x, float y) { sf::Transformable::setPosition(flipped ? offset.x + x : x - offset.x, y - offset.y); }
void Sprite::setPositionNoOffset(float x, float y) { sf::Transformable::setPosition(x, y); }
sf::Vector2f Sprite::getPosition() const {
    const auto &position = sf::Transformable::getPosition();
    return {flipped ? position.x - offset.x : offset.x + position.x, position.y + offset.y};
}
sf::Vector2f Sprite::getPositionNoOffset() const { return sf::Transformable::getPosition(); }
void Sprite::setScale(float value) { setScale(value, value); }
void Sprite::setScale(const sf::Vector2f &value) { setScale(value.x, value.y); }
void Sprite::setScale(float x, float y) { sf::Transformable::setScale(flipped ? -std::abs(x) : std::abs(x), y); }
sf::Vector2f Sprite::getScale() const { const auto &s = sf::Transformable::getScale(); return {std::abs(s.x), s.y}; }
void Sprite::setFrozen(bool value) { frozen = value; }
bool Sprite::getFrozen() const { return frozen; }
void Sprite::setReverse(bool value) { reverse = value; }
bool Sprite::getReverse() const { return reverse; }
int Sprite::getFrame() const { return frame; }
int Sprite::getTime() { return timer; }
void Sprite::setTime(int value) { timer = value; }
int Sprite::getSpeed() { return speed; }
void Sprite::setSpeed(int value) { speed = value; }
void Sprite::setResetDelay(int value) { resetDelay = value; }
int Sprite::getNumFrames() const { return numFrames; }
sf::Vector2i Sprite::getSize() const { return {w, h}; }
sf::IntRect Sprite::getTextureRect() const { return textureRect; }
sf::FloatRect Sprite::getLocalBounds() const { return {0, 0, static_cast<float>(textureRect.width), static_cast<float>(textureRect.height)}; }
bool Sprite::isFinished() const { return done; }
void Sprite::setDone(bool value) { done = value; }
void Sprite::useProcessedTileOffsets(bool value) { tileProcessed = value; }
bool Sprite::getLoop() const { return loop; }
void Sprite::setLoop(bool value) { loop = value; }
const sf::Vector2f &Sprite::getOffset() const { return offset; }
void Sprite::setOffset(float x, float y) { offset = {x, y}; }
std::string Sprite::getTextureName() { return m_textureName; }
const sf::Texture *Sprite::getTexture() const { return pTexture; }
void Sprite::setAlpha(unsigned char alpha) { for (auto &v : vertices) v.color.a = alpha; }
void Sprite::setColorComponents(unsigned char r, unsigned char g, unsigned char b) {
    for (auto &v : vertices) { v.color.r = r; v.color.g = g; v.color.b = b; }
}
void Sprite::setColor(sf::Color color, unsigned int index) { vertices[index].color = color; }
void Sprite::setColor(sf::Color color) { for (auto &v : vertices) v.color = color; }
const sf::Color &Sprite::getColor(unsigned int index) const { return vertices[index].color; }
const sf::Color &Sprite::getColor() const { return vertices[0].color; }

// Owner registration dependencies, 005a6cc0/005a6d80 and inline erase.
void SpriteEntityInterface::addSprite(Sprite * const sprite) {
    if (std::find(entitySprites.begin(), entitySprites.end(), sprite) != entitySprites.end()) return;
    entitySprites.push_back(sprite);
    if (!m_paletteGroupName.empty() && !m_paletteName.empty()) sprite->setPalette(m_paletteGroupName, m_paletteName);
}
void SpriteEntityInterface::removeSprite(const Sprite * const sprite) {
    const auto it = std::find(entitySprites.begin(), entitySprites.end(), sprite);
    if (it != entitySprites.end()) entitySprites.erase(it);
}
void SpriteEntityInterface::setPalette(const std::string &group, const std::string &name) {
    for (auto *sprite : entitySprites) sprite->setPalette(group, name);
    m_paletteGroupName = group; m_paletteName = name;
}
