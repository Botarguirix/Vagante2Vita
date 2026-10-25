#include "port_lib_types.h"
#define private public
#include "AttackScriptDatabase.h"
#include "ResourceManager.h"
#include "ResourceArchive.h"
#undef private
#include "recon_all.h"
#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>
#include <cstdlib>
namespace {
using DB=AttackScriptDatabase;
bool near(float a,float b) { return std::abs(a-b)<.0002f; }
void point(sf::Vector2f a,float x,float y) { assert(near(a.x,x) && near(a.y,y)); }
}
int main() {
    DB database;
    DB::AttackScript script("defaults");
    assert(script.keyframes.empty() && !script.useAfterImages && !script.useKeyframeAimOrigin);
    assert(script.defaultProjectileType==-1 && script.maintainProjectileVelocity && !script.manualAttackStep);
    point(script.weilderSpriteAnchor,.5f,1.f); point(script.aimArmAnchor,.583f,.542f);
    DB::AttackScriptKeyframe frame;
    assert(frame.frames==1 && frame.weilderSpriteFrame==0 && frame.enabled && !frame.isChargeFrame && !frame.releaseProjectiles);
    point(frame.origin,0,0); point(frame.weaponGraphicalScale,1,1); assert(frame.weaponDurationFactor==1.f);
    DB::WeaponProperties weapon("defaults");
    assert(weapon.weaponSpriteSheet=="items/items-weapons.png" && weapon.weaponSpriteFrame==1);
    assert(weapon.weaponSpriteDimensions==sf::Vector2i(20,20));
    assert(weapon.perFrameDuration==5 && weapon.projectileDamageMin==1 && weapon.projectileDamageMax==2);
    assert(weapon.projectileKnockback==1.f && weapon.projectileVelocity==-1.f && weapon.defaultProjectileType==-1);
    DB::EffectCue cue;
    assert(cue.sfxFileName.empty() && cue.shakeAmount==5.f && cue.shakeTime==5 && cue.particleCount==1 && cue.particleBits==0);
    frame.origin={2,3}; frame.aimOrigin={4,5}; frame.originRotationDegrees=30;
    script.keyframes.push_back(frame);
    point(script.worldKeyFrameOrigin(0,false,90,0),2,3); // Aim is disabled for keyframe transform.
    assert(near(script.worldKeyFrameDegrees(0,false,90,0),30));
    point(script.worldAimOrigin(0,false,90,0),4,5);
    assert(near(script.worldAimDegrees(0,false,90,0),90));
    script.useKeyframeAimOrigin=true;
    point(script.worldKeyFrameOrigin(0,false,90,0),1,7);
    point(script.worldKeyFrameOrigin(0,true,90,0),-1,7);
    point(script.worldKeyFrameOrigin(0,false,90,90),-7,1);
    assert(near(script.worldKeyFrameDegrees(0,false,90,0),120));
    assert(near(script.worldKeyFrameDegrees(0,true,90,0),60));
    point(script.worldAimOrigin(8,true,90,90),-5,-4); // Modulo index, parent rotation before reflection.
    assert(near(script.worldAimDegrees(8,true,90,90),-180) || near(script.worldAimDegrees(8,true,90,90),180));
    DB::AttackShape shape; shape.enabled=false; shape.damageMin=shape.damageMax=0;
    shape.knockback=0; shape.stunDuration=0; shape.defaultHitSound=DB::MONEY_BAG;
    assert(shape.containsPoint(sf::Transform::Identity,{100,100})); // Empty polygon deliberately accepts.
    shape.vertices={{0,0},{2,0},{2,2},{0,2}};
    sf::Transform transform; transform.translate(10,20).scale(2,2);
    assert(shape.containsPoint(transform,{12,22}) && shape.containsPoint(transform,{10,20}));
    assert(!shape.containsPoint(transform,{9,20})); // enabled is ignored by containsPoint.
    assert((shape.vertexIndicesCloseToPoint(transform,{10.5f,20})==std::vector<unsigned>{0}));
    assert(shape.vertexIndicesCloseToPoint(transform,{10.5001f,20}).empty());
    sf::Transform reflection; reflection.scale(-1,1);
    assert(!shape.containsPoint(reflection,{-1,1})); // Original winding-sensitive algorithm.
    const std::string json=R"fixture({"attack-scripts":[{"weilder-sprite-name":"weilder-sprite-name.png","weilder-sprite-width":3,"weilder-sprite-height":4,"aim-arm-sprite-name":"aim-arm-sprite-name.png","aim-arm-sprite-width":6,"aim-arm-sprite-height":7,"weilder-overlay-sprite-name":"weilder-overlay-sprite-name.png","weilder-overlay-sprite-width":9,"weilder-overlay-sprite-height":10,"weilder-anchor-x":2.5,"weilder-anchor-y":2.75,"aim-arm-anchor-x":3,"aim-arm-anchor-y":3.25,"use-after-images":false,"use-aim-origin":true,"default-projectile-type":17,"bow-draw-frames":1,"manual-attack-step":false,"manual-attack-stop":true,"require-projectile-reload-animation":false,"maintain-projectile-velocity":true,"render-projectile-after-aim-arm":false,"use-separate-ammo":true,"is-overhead-swing":false,"name":"swing","keyframes":[{"enabled":true,"frames":3,"origin-x":0.75,"origin-y":1,"origin-degrees":1.25,"weapon-duration-factor":1.5,"aim-origin-x":1.75,"aim-origin-y":2,"weapon-gfx-scale-x":2.25,"weapon-gfx-scale-y":2.5,"projectile-mid-x":2.75,"projectile-mid-y":3,"release-projectiles":true,"weilder-sprite-frame":15,"shapes":[{"enabled":true,"damage-min":3,"damage-max":4,"knockback":1,"stun-duration":6,"default-hit-sound":1,"vertices":[{"x":0,"y":0},{"x":2,"y":0},{"x":2,"y":2},{"x":0,"y":2}]}],"effect-cues":[{"type":2,"frame-duration":3,"begin-at-frame-start":true,"only-execute-on-weapon-stop-swing":false,"only-execute-on-weapon-reload":true,"position-x":1.5,"position-y":1.75,"aabb-x":2,"aabb-y":2.25,"radius":2.5,"sfx-filename":"sfx-filename.png","shake-amount":3,"shake-time":14,"particle-type":15,"particle-count":16,"particle-bits":2147483649}],"isChargeFrame":true}]},{"weilder-sprite-name":"weilder-sprite-name.png","weilder-sprite-width":3,"weilder-sprite-height":4,"aim-arm-sprite-name":"aim-arm-sprite-name.png","aim-arm-sprite-width":6,"aim-arm-sprite-height":7,"weilder-overlay-sprite-name":"weilder-overlay-sprite-name.png","weilder-overlay-sprite-width":9,"weilder-overlay-sprite-height":10,"weilder-anchor-x":2.5,"weilder-anchor-y":2.75,"aim-arm-anchor-x":3,"aim-arm-anchor-y":3.25,"use-after-images":false,"use-aim-origin":true,"default-projectile-type":17,"bow-draw-frames":18,"manual-attack-step":false,"manual-attack-stop":true,"require-projectile-reload-animation":false,"maintain-projectile-velocity":true,"render-projectile-after-aim-arm":false,"use-separate-ammo":true,"is-overhead-swing":false,"name":"swing","keyframes":[{"enabled":true,"frames":3,"origin-x":0.75,"origin-y":1,"origin-degrees":1.25,"weapon-duration-factor":1.5,"aim-origin-x":1.75,"aim-origin-y":2,"weapon-gfx-scale-x":2.25,"weapon-gfx-scale-y":2.5,"projectile-mid-x":2.75,"projectile-mid-y":3,"release-projectiles":true,"weilder-sprite-frame":15,"shapes":[{"enabled":true,"damage-min":3,"damage-max":4,"knockback":1,"stun-duration":6,"default-hit-sound":1,"vertices":[{"x":0,"y":0},{"x":2,"y":0},{"x":2,"y":2},{"x":0,"y":2}]}],"effect-cues":[{"type":2,"frame-duration":3,"begin-at-frame-start":true,"only-execute-on-weapon-stop-swing":false,"only-execute-on-weapon-reload":true,"position-x":1.5,"position-y":1.75,"aabb-x":2,"aabb-y":2.25,"radius":2.5,"sfx-filename":"sfx-filename.png","shake-amount":3,"shake-time":14,"particle-type":15,"particle-count":16,"particle-bits":2147483649}],"isChargeFrame":true}]}],"weapon-properties":[{"sprite-sheet-name":"sprite-sheet-name.png","sprite-frame":1,"sprite-width":4,"sprite-height":5,"sprite-anchor-x":1.25,"sprite-anchor-y":1.5,"per-frame-duration":8,"wand-type-flags":9,"rod-type-flags":10,"projectile-damage-min":11,"projectile-damage-max":12,"projectile-knockback":3,"projectile-velocity":-1.25,"default-projectile-type":15,"type-flags":16,"heavy":false,"name":"sword","shapes":[{"enabled":true,"damage-min":3,"damage-max":4,"knockback":1,"stun-duration":6,"default-hit-sound":1,"vertices":[{"x":0,"y":0},{"x":2,"y":0},{"x":2,"y":2},{"x":0,"y":2}]}]},{"sprite-sheet-name":"sprite-sheet-name.png","sprite-frame":3,"sprite-width":4,"sprite-height":5,"sprite-anchor-x":1.25,"sprite-anchor-y":1.5,"per-frame-duration":8,"wand-type-flags":9,"rod-type-flags":10,"projectile-damage-min":11,"projectile-damage-max":12,"projectile-knockback":3,"projectile-velocity":-1.25,"default-projectile-type":15,"type-flags":16,"heavy":false,"name":"sword","shapes":[{"enabled":true,"damage-min":3,"damage-max":4,"knockback":1,"stun-duration":6,"default-hit-sound":1,"vertices":[{"x":0,"y":0},{"x":2,"y":0},{"x":2,"y":2},{"x":0,"y":2}]}]}]})fixture";
    ResourceManager::setConfig(nullptr);
    std::ofstream("attacks.json")<<json;
    _randEngine.seed(8123); const auto rng=_randEngine;
    database.loadScripts();
    assert(_randEngine==rng && globalAttackScriptDb.attackScripts.size()==1 && globalAttackScriptDb.weaponProperties.size()==1);
    assert(database.attackScriptExists("swing") && database.weaponExists("sword"));
    assert(!database.attackScriptExists("missing") && !database.weaponExists("missing"));
    assert(database.attackScripts.empty() && database.weaponProperties.empty()); // Global specialization.
    auto loaded=database.getAttack("swing");
    auto loadedWeapon=database.getWeapon("sword");
    assert(loaded.bowDrawFrames==18 && loadedWeapon.weaponSpriteFrame==3); // Last duplicate wins.
    assert(loaded.previewWeilderSprite=="weilder-sprite-name.png" && loaded.previewAimArmSprite=="aim-arm-sprite-name.png");
    assert(loaded.keyframes.size()==1 && loaded.keyframes[0].isChargeFrame);
    auto &loadedFrame=loaded.keyframes[0];
    assert(loadedFrame.frames==3 && loadedFrame.weaponDurationFactor==1.5f);
    point(loadedFrame.origin,0.75f,1.f);
    assert(loadedFrame.shapes.size()==1 && loadedFrame.effectCues.size()==1);
    assert(loadedFrame.shapes[0].damageMin==3 && loadedFrame.shapes[0].defaultHitSound==DB::BLADE);
    assert(loadedFrame.shapes[0].vertices[2]==sf::Vector2f(2,2));
    assert(loadedFrame.effectCues[0].type==DB::EffectCue::SFX && loadedFrame.effectCues[0].particleBits==0x80000001u);
    assert(loadedFrame.effectCues[0].sfxFileName=="sfx-filename.png");
    point(loadedFrame.aimOrigin,1.75f,2.f); point(loadedFrame.weaponGraphicalScale,2.25f,2.5f);
    point(loadedFrame.projectileMidOrigin,2.75f,3.f);
    assert(loadedFrame.enabled && loadedFrame.releaseProjectiles && loadedFrame.weilderSpriteFrame==15);
    assert(loadedFrame.originRotationDegrees==1.25f && loadedFrame.shapes[0].stunDuration==6);
    const auto &effect=loadedFrame.effectCues[0];
    point(effect.position,1.5f,1.75f); point(effect.aabbExtent,2.f,2.25f);
    assert(effect.radius==2.5f && effect.shakeAmount==3.f && effect.shakeTime==14);
    assert(effect.frameDuration==3 && effect.beginAtFrameStart && !effect.onlyExecuteOnWeaponStopSwing && effect.onlyExecuteOnWeaponReload);
    assert(effect.particleType==15 && effect.particleCount==16);
    assert(loadedWeapon.projectileVelocity==-1.25f && loadedWeapon.shapes.size()==1);
    assert(loadedWeapon.perFrameDuration==8 && loadedWeapon.wandTypeFlags==9 && loadedWeapon.rodTypeFlags==10);
    assert(loadedWeapon.typeFlags==16 && !loadedWeapon.isHeavy && loadedWeapon.defaultProjectileType==15);
    assert(loadedWeapon.projectileDamageMin==11 && loadedWeapon.projectileDamageMax==12 && loadedWeapon.projectileKnockback==3.f);
    assert(near(magVector2f({3,4}),5));
    loaded.keyframes[0].shapes[0].vertices[0].x=999;
    assert(database.getAttack("swing").keyframes[0].shapes[0].vertices[0].x==0); // Deep value copy.
    bool threw=false;
    try { database.getAttack("missing"); } catch(const std::out_of_range &) { threw=true; }
    assert(threw); threw=false;
    try { database.getWeapon("missing"); } catch(const std::out_of_range &) { threw=true; }
    assert(threw);
    auto legacy=json;
    const std::string optional=",\"isChargeFrame\":true";
    for(auto pos=legacy.find(optional); pos!=std::string::npos; pos=legacy.find(optional)) legacy.erase(pos,optional.size());
    std::ofstream("attacks.json")<<legacy; database.loadScripts();
    assert(!database.getAttack("swing").keyframes[0].isChargeFrame);
    std::ofstream("attacks.json")<<"{"; database.loadScripts();
    assert(database.attackScriptExists("swing")); // Parse failure preserves previous maps.
    Config config; ResourceManager::setConfig(&config);
    ResourceArchive archive("missing.vra",false);
    archive.loaded=true; archive.attackData=new char[json.size()+1];
    std::copy(json.begin(),json.end(),archive.attackData); archive.attackData[json.size()]=0;
    ResourceManager::archive=&archive;
    database.loadScripts(); assert(database.getAttack("swing").keyframes[0].isChargeFrame);
    globalAttackScriptDb.initialized=false; database.initialize();
    assert(globalAttackScriptDb.initialized && !database.initialized);
    archive.attackData[0]='!'; database.initialize(); // One-shot even if source changes.
    assert(database.attackScriptExists("swing"));
    database.loadScripts(); assert(database.attackScriptExists("swing"));
    delete[] archive.attackData; archive.attackData=nullptr;
    database.loadScripts(); assert(database.attackScriptExists("swing"));
    ResourceManager::archive=nullptr; ResourceManager::setConfig(nullptr);
    // Optional commercial fixture is supplied externally, never committed.
    if(const char *path=std::getenv("VAGANTE_ATTACK_FIXTURE")) {
        std::ifstream input(path); std::ofstream output("attacks.json"); output<<input.rdbuf(); output.close();
        database.loadScripts();
        assert(globalAttackScriptDb.attackScripts.size()==31 && globalAttackScriptDb.weaponProperties.size()==48);
        for(const auto &entry:globalAttackScriptDb.attackScripts) {
            assert(database.attackScriptExists(entry.first));
            for(const auto &f:entry.second.keyframes) {
                for(const auto &s:f.shapes) assert(std::isfinite(s.knockback));
                for(const auto &e:f.effectCues) assert(std::isfinite(e.radius));
            }
        }
        std::cout<<"Commercial attacks: "<<globalAttackScriptDb.attackScripts.size()<<", weapons: "<<globalAttackScriptDb.weaponProperties.size()<<"\n";
    }
    std::ofstream("attacks.json")<<"editor sentinel"; database.saveScripts();
    std::ifstream sentinel("attacks.json"); std::string line; std::getline(sentinel,line); assert(line=="editor sentinel");
    assert(_randEngine==rng);
    std::cout<<"AttackScriptDatabase: transforms/degrees, convex winding, radius, JSON/disk/archive, copies, duplicates, missing keys, initialization and editor exclusion passed\n";
}
