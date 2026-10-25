#include "recon_all.h"
#include <fstream>
#include <stdexcept>
#include <rapidjson/istreamwrapper.h>
#define VLOG(level) if ((level) > Log::reportingLevel()) ; else Log().get(level)
namespace {
using DB = AttackScriptDatabase;
using Json = rapidjson::Value;
template<class Tag, typename Tag::Type Member> struct GetterAccess {
    friend typename Tag::Type attackConfigGetter(Tag) { return Member; }
};
struct ResourceConfig { using Type = Config *(*)(); friend Type attackConfigGetter(ResourceConfig); };
template struct GetterAccess<ResourceConfig, &ResourceManager::getConfig>;
const Json &field(const Json &v,const char *key) {
    if (!v.IsObject() || !v.HasMember(key)) throw std::runtime_error(key);
    return v[key];
}
const Json &array(const Json &v,const char *key) {
    const auto &result=field(v,key);
    if (!result.IsArray()) throw std::runtime_error(key);
    return result;
}
std::string string(const Json &v,const char *key) {
    const auto &result=field(v,key);
    if (!result.IsString()) throw std::runtime_error(key);
    return result.GetString();
}
bool boolean(const Json &v,const char *key) {
    const auto &result=field(v,key);
    if (!result.IsBool()) throw std::runtime_error(key);
    return result.GetBool();
}
std::int64_t integer(const Json &v,const char *key) {
    const auto &result=field(v,key);
    if (!result.IsInt64() && !result.IsUint64()) throw std::runtime_error(key);
    return result.IsInt64() ? result.GetInt64() : static_cast<std::int64_t>(result.GetUint64());
}
float number(const Json &v,const char *key) {
    const auto &result=field(v,key);
    if (!result.IsNumber()) throw std::runtime_error(key);
    return static_cast<float>(result.GetDouble());
}
// Reader lambda 4eb003... @00412af0: preserve shape/vertex order.
std::vector<DB::AttackShape> readShapes(const Json &values) {
    std::vector<DB::AttackShape> shapes;
    for (const auto &v : values.GetArray()) {
        DB::AttackShape s;
        s.enabled = boolean(v,"enabled");
        s.damageMin = integer(v,"damage-min");
        s.damageMax = integer(v,"damage-max");
        s.knockback = number(v,"knockback");
        s.stunDuration = integer(v,"stun-duration");
        s.defaultHitSound = static_cast<DB::AttackHitSound>(integer(v,"default-hit-sound"));
        for (const auto &vertex : array(v,"vertices").GetArray()) {
            const float x=number(vertex,"x");
            const float y=number(vertex,"y");
            s.vertices.emplace_back(x,y);
        }
        shapes.push_back(s);
    }
    return shapes;
}
// Reader lambda 78e4bb... @00412680.
std::vector<DB::EffectCue> readEffects(const Json &values) {
    std::vector<DB::EffectCue> cues;
    for (const auto &v : values.GetArray()) {
        DB::EffectCue e;
        e.type = static_cast<DB::EffectCue::Type>(integer(v,"type"));
        e.frameDuration = integer(v,"frame-duration");
        e.beginAtFrameStart = boolean(v,"begin-at-frame-start");
        e.onlyExecuteOnWeaponStopSwing = boolean(v,"only-execute-on-weapon-stop-swing");
        e.onlyExecuteOnWeaponReload = boolean(v,"only-execute-on-weapon-reload");
        e.position.x = number(v,"position-x");
        e.position.y = number(v,"position-y");
        e.aabbExtent.x = number(v,"aabb-x");
        e.aabbExtent.y = number(v,"aabb-y");
        e.radius = number(v,"radius");
        e.sfxFileName = string(v,"sfx-filename");
        e.shakeAmount = number(v,"shake-amount");
        e.shakeTime = integer(v,"shake-time");
        e.particleType = integer(v,"particle-type");
        e.particleCount = integer(v,"particle-count");
        e.particleBits = integer(v,"particle-bits");
        cues.push_back(e);
    }
    return cues;
}
DB::AttackScript readAttack(const Json &v) {
    DB::AttackScript a(string(v,"name"));
    a.previewWeilderSprite = string(v,"weilder-sprite-name");
    a.previewWeilderSpriteDimensions.x = integer(v,"weilder-sprite-width");
    a.previewWeilderSpriteDimensions.y = integer(v,"weilder-sprite-height");
    a.previewAimArmSprite = string(v,"aim-arm-sprite-name");
    a.previewAimArmSpriteDimensions.x = integer(v,"aim-arm-sprite-width");
    a.previewAimArmSpriteDimensions.y = integer(v,"aim-arm-sprite-height");
    a.previewWeilderOverlaySprite = string(v,"weilder-overlay-sprite-name");
    a.previewWeilderOverlaySpriteDimensions.x = integer(v,"weilder-overlay-sprite-width");
    a.previewWeilderOverlaySpriteDimensions.y = integer(v,"weilder-overlay-sprite-height");
    a.weilderSpriteAnchor.x = number(v,"weilder-anchor-x");
    a.weilderSpriteAnchor.y = number(v,"weilder-anchor-y");
    a.aimArmAnchor.x = number(v,"aim-arm-anchor-x");
    a.aimArmAnchor.y = number(v,"aim-arm-anchor-y");
    a.useAfterImages = boolean(v,"use-after-images");
    a.useKeyframeAimOrigin = boolean(v,"use-aim-origin");
    a.defaultProjectileType = integer(v,"default-projectile-type");
    a.bowDrawFrames = integer(v,"bow-draw-frames");
    a.manualAttackStep = boolean(v,"manual-attack-step");
    a.manualAttackStop = boolean(v,"manual-attack-stop");
    a.requireProjectileReloadAnimation = boolean(v,"require-projectile-reload-animation");
    a.maintainProjectileVelocity = boolean(v,"maintain-projectile-velocity");
    a.renderProjectileAfterAimArm = boolean(v,"render-projectile-after-aim-arm");
    a.useSeparateAmmo = boolean(v,"use-separate-ammo");
    a.isOverheadSwing = boolean(v,"is-overhead-swing");
    for (const auto &frame : array(v,"keyframes").GetArray()) {
        DB::AttackScriptKeyframe k;
        if (frame.HasMember("isChargeFrame")) k.isChargeFrame=boolean(frame,"isChargeFrame");
        k.enabled = boolean(frame,"enabled");
        k.frames = integer(frame,"frames");
        k.origin.x = number(frame,"origin-x");
        k.origin.y = number(frame,"origin-y");
        k.originRotationDegrees = number(frame,"origin-degrees");
        k.weaponDurationFactor = number(frame,"weapon-duration-factor");
        k.aimOrigin.x = number(frame,"aim-origin-x");
        k.aimOrigin.y = number(frame,"aim-origin-y");
        k.weaponGraphicalScale.x = number(frame,"weapon-gfx-scale-x");
        k.weaponGraphicalScale.y = number(frame,"weapon-gfx-scale-y");
        k.projectileMidOrigin.x = number(frame,"projectile-mid-x");
        k.projectileMidOrigin.y = number(frame,"projectile-mid-y");
        k.releaseProjectiles = boolean(frame,"release-projectiles");
        k.weilderSpriteFrame = integer(frame,"weilder-sprite-frame");
        k.shapes=readShapes(array(frame,"shapes"));
        k.effectCues=readEffects(array(frame,"effect-cues"));
        a.keyframes.push_back(k);
    }
    return a;
}
DB::WeaponProperties readWeapon(const Json &v) {
    DB::WeaponProperties w(string(v,"name"));
    w.weaponSpriteSheet = string(v,"sprite-sheet-name");
    w.weaponSpriteFrame = integer(v,"sprite-frame");
    w.weaponSpriteDimensions.x = integer(v,"sprite-width");
    w.weaponSpriteDimensions.y = integer(v,"sprite-height");
    w.weaponSpriteAnchor.x = number(v,"sprite-anchor-x");
    w.weaponSpriteAnchor.y = number(v,"sprite-anchor-y");
    w.perFrameDuration = integer(v,"per-frame-duration");
    w.wandTypeFlags = integer(v,"wand-type-flags");
    w.rodTypeFlags = integer(v,"rod-type-flags");
    w.projectileDamageMin = integer(v,"projectile-damage-min");
    w.projectileDamageMax = integer(v,"projectile-damage-max");
    w.projectileKnockback = number(v,"projectile-knockback");
    w.projectileVelocity = number(v,"projectile-velocity");
    w.defaultProjectileType = integer(v,"default-projectile-type");
    w.typeFlags = integer(v,"type-flags");
    w.isHeavy = boolean(v,"heavy");
    w.shapes=readShapes(array(v,"shapes"));
    return w;
}
}

void AttackScriptDatabase::loadScripts() {
    rapidjson::Document document;
    if (attackConfigGetter(ResourceConfig{})() && ResourceManager::isArchiveLoaded()) {
        const char *data=ResourceManager::getAttackData();
        if (!data) { VLOG(LOG_ERROR) << "AttackScriptDatabase::loadScripts: No attack data found!"; return; }
        VLOG(LOG_INFO) << "AttackScriptDatabase::loadScripts: Got attack data!";
        document.Parse(data);
        if (document.HasParseError()) { VLOG(LOG_ERROR) << "AttackScriptDatabase::loadScripts: Error parsing attacks.json in data.vra!"; return; }
    } else {
        std::ifstream input("attacks.json");
        if (!input) { VLOG(LOG_ERROR) << "AttackScriptDatabase::loadScripts: Failed to open attacks.json! Does it exist?"; return; }
        rapidjson::IStreamWrapper stream(input);
        document.ParseStream(stream);
        if (document.HasParseError()) { VLOG(LOG_ERROR) << "AttackScriptDatabase::loadScripts: Error parsing attacks.json!"; return; }
    }
    // Malformed external data gets a diagnostic instead of RapidJSON assertions.
    try {
        const auto &attacks=array(document,"attack-scripts");
        const auto &weapons=array(document,"weapon-properties");
        globalAttackScriptDb.attackScripts.clear();
        for (const auto &value : attacks.GetArray()) {
            auto attack=readAttack(value);
            const auto name=attack.attackScriptName;
            globalAttackScriptDb.attackScripts.insert_or_assign(name,attack);
        }
        globalAttackScriptDb.weaponProperties.clear();
        for (const auto &value : weapons.GetArray()) {
            auto weapon=readWeapon(value);
            const auto name=weapon.weaponName;
            globalAttackScriptDb.weaponProperties.insert_or_assign(name,weapon);
        }
    } catch (const std::runtime_error &error) {
        VLOG(LOG_ERROR) << "AttackScriptDatabase::loadScripts: Invalid field: " << error.what();
    }
}
