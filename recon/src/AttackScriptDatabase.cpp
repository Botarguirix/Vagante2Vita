#include "recon_all.h"
#include <cmath>
#define VLOG(level) if ((level) > Log::reportingLevel()) ; else Log().get(level)

float magVector2f(const sf::Vector2f &value) {
    // 004196e0: mulss x*x; mulss y*y; addss; sqrtss (not hypot/double pow).
    return std::sqrt(value.x*value.x + value.y*value.y);
}

AttackScriptDatabase::AttackScriptDatabase() : initialized(false) {}
// Inline initialization recovered from ResourceManager::runTasks @0074f707ff.
void AttackScriptDatabase::initialize() {
    if (!globalAttackScriptDb.initialized) {
        loadScripts();
        globalAttackScriptDb.initialized=true;
    }
}

// Scalar members not assigned by the original inline construction remain unset;
// the JSON readers assign every one before inserting the record.
AttackScriptDatabase::AttackShape::AttackShape() {}
AttackScriptDatabase::EffectCue::EffectCue()
    : shakeAmount(5.f), shakeTime(5), particleType(0), particleCount(1), particleBits(0) {}

AttackScriptDatabase::WeaponProperties::WeaponProperties(const std::string &name)
    : weaponName(name), weaponSpriteSheet("items/items-weapons.png"), weaponSpriteFrame(1),
      weaponSpriteDimensions(20,20), weaponSpriteAnchor(0.f,.5f), perFrameDuration(5),
      wandTypeFlags(0), rodTypeFlags(0), projectileDamageMin(1), projectileDamageMax(2),
      projectileKnockback(1.f), projectileVelocity(-1.f), defaultProjectileType(-1),
      typeFlags(0), isHeavy(false) {}

AttackScriptDatabase::AttackScriptKeyframe::AttackScriptKeyframe()
    : frames(1), weilderSpriteFrame(0), originRotationDegrees(0.f),
      weaponGraphicalScale(1.f,1.f), weaponDurationFactor(1.f), enabled(true),
      isChargeFrame(false), releaseProjectiles(false) {}

AttackScriptDatabase::AttackScript::AttackScript(const std::string &name)
    : attackScriptName(name), weilderSpriteAnchor(.5f,1.f), aimArmAnchor(.583f,.542f),
      useAfterImages(false), useKeyframeAimOrigin(false), defaultProjectileType(-1), bowDrawFrames(0),
      manualAttackStep(false), manualAttackStop(false), requireProjectileReloadAnimation(false),
      maintainProjectileVelocity(true), renderProjectileAfterAimArm(false), useSeparateAmmo(false),
      isOverheadSwing(false) {}

sf::Transform AttackScriptDatabase::AttackScript::keyFrameToWorldTransform(
    unsigned index, bool flipped, float aimDegrees, float parentDegrees) const {
    if (index >= keyframes.size()) {
        VLOG(LOG_ERROR) << "AttackScriptDatabase::AttackScript::keyFrameToWorldTransform: invalid keyframe! keyFrameIndex=" << index;
        if (keyframes.empty()) return {}; // Original modulo-zero is invalid external data.
        index %= keyframes.size();
    }
    sf::Transform transform;
    transform.rotate(parentDegrees).scale(flipped ? -1.f : 1.f,1.f);
    if (useKeyframeAimOrigin) transform.translate(keyframes[index].aimOrigin).rotate(aimDegrees);
    transform.translate(keyframes[index].origin).rotate(keyframes[index].originRotationDegrees);
    return transform;
}
sf::Transform AttackScriptDatabase::AttackScript::aimToWorldTransform(
    unsigned index, bool flipped, float aimDegrees, float parentDegrees) const {
    if (index >= keyframes.size()) {
        VLOG(LOG_ERROR) << "AttackScriptDatabase::AttackScript::aimToWorldTransform: invalid keyframe! keyFrameIndex=" << index;
        if (keyframes.empty()) return {};
        index %= keyframes.size();
    }
    sf::Transform transform;
    transform.rotate(parentDegrees).scale(flipped ? -1.f : 1.f,1.f);
    transform.translate(keyframes[index].aimOrigin).rotate(aimDegrees);
    return transform;
}
sf::Vector2f AttackScriptDatabase::AttackScript::worldKeyFrameOrigin(unsigned i,bool f,float a,float p) const {
    return keyFrameToWorldTransform(i,f,a,p).transformPoint(0.f,0.f);
}
sf::Vector2f AttackScriptDatabase::AttackScript::worldAimOrigin(unsigned i,bool f,float a,float p) const {
    return aimToWorldTransform(i,f,a,p).transformPoint(0.f,0.f);
}
namespace {
float transformDegrees(const sf::Transform &transform) {
    const auto origin = transform.transformPoint(0.f,0.f);
    const auto axis = transform.transformPoint(1.f,0.f) - origin;
    // 0040f402/0040f4d2: mulss [00b20598], omitted by Ghidra's return reconstruction.
    return (axis.x != 0.f || axis.y != 0.f ? std::atan2(axis.y,axis.x) : 0.f) * 57.2957763671875f;
}
}
float AttackScriptDatabase::AttackScript::worldKeyFrameDegrees(unsigned i,bool f,float a,float p) const {
    return transformDegrees(keyFrameToWorldTransform(i,f,a,p));
}
float AttackScriptDatabase::AttackScript::worldAimDegrees(unsigned i,bool f,float a,float p) const {
    return transformDegrees(aimToWorldTransform(i,f,a,p));
}
bool AttackScriptDatabase::AttackShape::containsPoint(const sf::Transform &transform,const sf::Vector2f &point) const {
    std::vector<sf::Vector2f> world;
    for (const auto &vertex : vertices) world.push_back(transform.transformPoint(vertex));
    for (unsigned i=0; i<world.size(); ++i) {
        const auto &a=world[i], &b=world[(i+1)%world.size()];
        if ((point.y-a.y)*(b.x-a.x) - (b.y-a.y)*(point.x-a.x) < 0.f) return false;
    }
    return true; // Includes the original empty polygon and boundary behavior.
}
std::vector<unsigned> AttackScriptDatabase::AttackShape::vertexIndicesCloseToPoint(
    const sf::Transform &transform,const sf::Vector2f &point) const {
    std::vector<sf::Vector2f> world;
    for (const auto &vertex : vertices) world.push_back(transform.transformPoint(vertex));
    std::vector<unsigned> indices;
    for (unsigned i=0; i<world.size(); ++i) {
        const auto delta=point-world[i];
        // magVector2f @004196e0: single-precision x*x, y*y, sum, sqrt.
        if (magVector2f(delta) <= .5f) indices.push_back(i);
    }
    return indices;
}
bool AttackScriptDatabase::attackScriptExists(const std::string &name) const {
    if (globalAttackScriptDb.attackScripts.count(name)) return true;
    VLOG(LOG_DEBUG) << "AttackScriptDatabase::attackScriptExists: WARNING - \"" << name << "\" doesn't exist!";
    return false;
}
bool AttackScriptDatabase::weaponExists(const std::string &name) const {
    if (globalAttackScriptDb.weaponProperties.count(name)) return true;
    VLOG(LOG_DEBUG) << "AttackScriptDatabase::weaponExists: WARNING - \"" << name << "\" doesn't exist!";
    return false;
}
AttackScriptDatabase::AttackScript AttackScriptDatabase::getAttack(const std::string &name) const {
    return globalAttackScriptDb.attackScripts.at(name);
}
AttackScriptDatabase::WeaponProperties AttackScriptDatabase::getWeapon(const std::string &name) const {
    return globalAttackScriptDb.weaponProperties.at(name);
}
void AttackScriptDatabase::saveScripts() { /* Attack editor serialization is excluded from the port. */ }
