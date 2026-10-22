#include "port_lib_types.h"
#define private public
#include "Input.h"
#undef private
#include "recon_all.h"
#include <cassert>

namespace {
using K = sf::Keyboard;
void testStateAndBinding() {
    Input input(nullptr);
    assert(input.controllers.size() == 4 && input.keys.size() == K::KeyCount);
    assert(input.controllers[0].enabled && !input.controllers[1].enabled);
    assert(input.actionNames[Input::ACTION_DOWN] == "Down");
    input.generateAndLoadDefaultProfiles();
    assert(input.inputProfiles.size() == 4 && input.inputProfiles[0].profileName == "Default Keyboard");
    assert(input.controllers[0].keyBindings[Input::ACTION_JUMP].key == K::S);
    auto &c = input.controllers[0];
    c.binding = false;
    input.keys.assign(K::KeyCount, false);
    input.keys[K::S] = true;
    input.stepController(c);
    assert(input.isHeld(Input::ACTION_JUMP, 0) && input.isPressed(Input::ACTION_JUMP, 0));
    input.stepController(c);
    assert(!input.isPressed(Input::ACTION_JUMP, 0));
    input.keys[K::S] = false;
    input.stepController(c);
    assert(input.isReleased(Input::ACTION_JUMP, 0));
    c.enabled = false;
    assert(!input.isReleased(Input::ACTION_JUMP, 0));
    c.enabled = true;

    input.keys[K::S] = true;
    c.binded[Input::ACTION_MELEE] = false;
    c.binding = true; c.currKey = K::Unknown; c.bindTimer = 0;
    input.stepBindController(c); // A different candidate must first become stable.
    assert(c.bindTimer == 0);
    for (int i = 0; i < 19; ++i) input.stepBindController(c);
    assert(!c.binded[Input::ACTION_MELEE] && c.bindTimer == 19);
    input.stepBindController(c);
    assert(c.binded[Input::ACTION_MELEE] && c.keyBindings[Input::ACTION_MELEE].key == K::S && c.justBinded);
    input.stepController(c);
    assert(!c.actions[Input::ACTION_MELEE] && c.justBinded);
    input.keys[K::S] = false;
    input.stepController(c);
    assert(!c.justBinded);
    input.keys[K::S] = true;
    input.stepController(c);
    assert(input.isPressed(Input::ACTION_MELEE, 0));
    input.cancelBinding(0);
    assert(!c.binding && c.currentlyBinding == Input::NUM_ACTIONS);
    assert(input.keysBoundCount.at(K::S) == 2);

    input.prevKeys.assign(K::KeyCount, false);
    assert(input.isKeyHeld(K::S) && input.isKeyPressed(K::S));
    assert(!input.isKeyHeld(K::Unknown) && !input.isKeyPressed(K::Unknown));
    input.keys[K::S] = false; input.prevKeys[K::S] = true;
    assert(input.isKeyReleased(K::S));
    input.setNetState(0x4001, 0);
    input.setNetState(0x0002, 1);
    assert(input.getNetState(0) == 0x4001);
    assert(input.getNetState(1) == 0x4001); // Slot-zero behavior confirmed in original assembly.

    // Exercise SFML polling; assert transitions from our prior snapshot, not
    // assumptions about which physical keys the user might currently be holding.
    const auto before = input.keys;
    input.step();
    assert(input.prevKeys == before);
}

void testDescriptorsAndGlyphs() {
    for (int i = 0; i < K::KeyCount; ++i) {
        const auto key = static_cast<K::Key>(i);
        const auto parsed = Input::getBindFromDescriptor(Input::getKeyName(key));
        assert(parsed.isKey && parsed.key == key && parsed.bound);
    }
    assert(Input::getKeyName(K::Unknown) == "Unknown");
    assert(Input::getKeyName(K::Semicolon) == "SemiColon");
    for (int action = 0; action < Input::NUM_ACTIONS; ++action)
        assert(Input::getActionFromHandle(Input::getActionHandle(static_cast<Input::Actions>(action))) == action);
    assert(Input::getActionHandle(Input::NUM_ACTIONS).empty());
    assert(Input::getActionFromHandle("missing") == Input::NUM_ACTIONS);
    const auto parts = Input::getDescriptors(" Z | [+0] || [-1] | [31] | ");
    assert(parts == std::vector<std::string>({"Z", "[+0]", "[-1]", "[31]"}));
    for (const auto &part : parts) {
        const auto bind = Input::getBindFromDescriptor(part);
        assert(Input::getDescriptorFromBind(bind, bind.isKey) == part);
    }
    assert(!Input::getBindFromDescriptor("").bound);
    assert(!Input::getBindFromDescriptor("[10").bound);
    assert(Input::getBindFromDescriptor("nonsense").key == K::Unknown);

    Input input(nullptr); input.generateAndLoadDefaultProfiles();
    auto &c = input.controllers[0]; c.usingJoystick = true; c.controlsType = CONTROLS_GENERIC_GAMEPAD;
    c.joystickBindings[Input::ACTION_JUMP] = ActionBind(10u);
    assert(input.getBindName(Input::ACTION_JUMP, 0, false) == "Button 10");
    c.usingSteamController = true;
    assert(input.getBindName(Input::ACTION_JUMP, 0, false) == "ERROR");
    assert(input.getSteamSprite(0, Input::ACTION_JUMP) == nullptr);
    c.usingSteamController = false;
    c.controlsType = CONTROLS_XBOX_360;
    assert(input.getBindName(Input::ACTION_JUMP, 0, false) == "A");
    assert(input.getXInputButtonSpriteId(0, Input::ACTION_JUMP, false) == 0);
    c.alternateJoystickBindings[Input::ACTION_JUMP] = c.joystickBindings[Input::ACTION_JUMP];
    assert(input.getBindName(Input::ACTION_JUMP, 0, true) == "-");
    c.joystickBindings[Input::ACTION_JUMP] = ActionBind(unsigned(sf::Joystick::Y), true);
    assert(input.getXInputBindName(c.joystickBindings[Input::ACTION_JUMP]) == "Down (L-Stk)");
    assert(input.getXInputButtonSpriteId(0, Input::ACTION_JUMP, false) == 22);
    assert(input.getPs4ButtonSpriteId(0, Input::ACTION_JUMP, false) == 21);
    c.binded[Input::ACTION_JUMP] = false; c.currentlyBinding = Input::ACTION_JUMP; c.bindingAlternate = false;
    c.bindTimer = 6;
    assert(input.getBindName(Input::ACTION_JUMP, 0, false) == "|||");
    c.bindTimer = 0;
    assert(input.getBindName(Input::ACTION_JUMP, 0, false) == "HOLD A BUTTON");
    c.usingJoystick = false;
    assert(input.getBindName(Input::ACTION_JUMP, 0, false) == "HOLD A KEY");

    c.usingJoystick = c.tryToUseJoystick = true; c.joystickVendorId = 0x54c; c.joystickProductId = 0x5c4;
    assert(input.getControlsType(c) == CONTROLS_PS4);
    c.joystickProductId = 0x268; assert(input.getControlsType(c) == CONTROLS_PS3);
    c.joystickVendorId = 0x45e; c.joystickProductId = 0x2d1;
    assert(input.getControlsType(c) == CONTROLS_XBOX_ONE);
    input.updateGamepadStatus();
    assert(!input.controllersDetected.empty() && input.controllersDetected[0].keyboard);
}

void testText() {
    Input input(nullptr);
    std::string text = "one two";
    input.setEnteringText(&text, 7, true);
    sf::Event::KeyEvent event{};
    event.code = K::Left; event.control = true;
    input.enteringTextKeyEvent(event, false); assert(input.textCursorIndexEnd == 7);
    input.enteringTextKeyEvent(event, true); assert(input.textCursorIndexEnd == 4 && input.textCursorIndexStart == 4);
    event.code = K::End; event.shift = true; event.control = false;
    input.enteringTextKeyEvent(event, true); assert(input.getSelectedText() == "two");
    input.inputChar(0xf1);
    assert(text == "one \xc3\xb1" && input.textCursorIndexEnd == 6);
    input.inputText(0x1f); input.inputText(0x80); input.inputText(0x9f);
    assert(text == "one \xc3\xb1");
    input.inputText('!'); assert(text == "one \xc3\xb1!");
    event = {}; event.code = K::Backspace;
    input.enteringTextKeyEvent(event, true); assert(text == "one \xc3\xb1");
    input.selectAllText(); input.deleteSelectedText(); assert(text.empty());
    input.inputText('A'); event.code = K::Enter; event.shift = true;
    input.enteringTextKeyEvent(event, true); assert(text == "A\n" && input.enteringText);
    event.shift = false;
    input.enteringTextKeyEvent(event, true);
    assert(!input.enteringText && input.enteredTextPressedReturn && input.skipInputThisFrame && !input.textBuffer);
    input.setEnteringText(&text, 0, false); input.setTextCursorToEnd(); assert(input.textCursorIndexStart == 2);
    input.stopEnteringTextIfBufferEqual(&text); assert(!input.enteringText);
    // A null window must not touch the system clipboard.
    input.tryCopy(); input.tryPaste();
}

void testIniAndProfiles() {
    Input original(nullptr); original.generateAndLoadDefaultProfiles();
    original.controllers[2].enabled = true;
    original.controllers[2].keyBindings[Input::ACTION_JUMP] = ActionBind(K::F7);
    original.controllers[2].joystickBindings[Input::ACTION_JUMP] = ActionBind(31u);
    original.controllers[2].alternateJoystickBindings[Input::ACTION_JUMP] = ActionBind(unsigned(sf::Joystick::PovY), false);
    assert(original.save());
    Input loaded(nullptr); assert(loaded.load());
    for (unsigned slot = 0; slot < 4; ++slot) {
        assert(loaded.controllers[slot].enabled == original.controllers[slot].enabled);
        for (int action = 0; action < Input::NUM_ACTIONS; ++action) {
            const auto &a = original.controllers[slot]; const auto &b = loaded.controllers[slot];
            assert(a.keyBindings[action].key == b.keyBindings[action].key);
            assert(Input::getDescriptorFromBind(a.joystickBindings[action], false) == Input::getDescriptorFromBind(b.joystickBindings[action], false));
            // A single saved joystick descriptor becomes both primary and
            // alternate in the original loader; the UI hides the duplicate.
            const auto &expectedAlternate = a.alternateJoystickBindings[action].bound ? a.alternateJoystickBindings[action] : a.joystickBindings[action];
            assert(Input::getDescriptorFromBind(expectedAlternate, false) == Input::getDescriptorFromBind(b.alternateJoystickBindings[action], false));
        }
    }
    InputProfile keyboard;
    original.controllers[0].copyToProfile(keyboard, true);
    keyboard.profileName = " My.Custom Profile ";
    assert(keyboard.saveProfile() && keyboard.filename == "profile_my_custom_profile.ini");
    InputProfile restored;
    assert(restored.loadProfile(std::filesystem::path(Config::CONFIG_DIR_PATH) / keyboard.filename));
    assert(restored.keyboard && restored.bindings.size() == 15 && restored.bindings[4].key == K::S);
    InputProfile gamepad;
    original.controllers[0].copyToProfile(gamepad, false); gamepad.profileName = "Pad";
    assert(gamepad.saveProfile());
    InputProfile restoredPad;
    assert(restoredPad.loadProfile(std::filesystem::path(Config::CONFIG_DIR_PATH) / gamepad.filename));
    loaded.controllers[1].loadFromProfile(restoredPad);
    assert(loaded.controllers[1].tryToUseJoystick && !loaded.controllers[1].alternateJoystickBindings[0].bound);
    loaded.loadProfiles();
    const auto count = loaded.inputProfiles.size(); loaded.loadProfiles();
    assert(count == loaded.inputProfiles.size() && count == 6);

    _randEngine.seed(1234);
    auto expected = _randEngine; for (int i = 0; i < 4; ++i) expected();
    InputProfile unnamed; original.controllers[0].copyToProfile(unnamed, true);
    assert(unnamed.saveProfile()); assert(_randEngine == expected);
    assert(unnamed.filename.substr(0, 8) == "profile_" && unnamed.filename.size() == 16);
    std::filesystem::remove(std::filesystem::path(Config::CONFIG_DIR_PATH) / unnamed.filename);

    std::ofstream(std::filesystem::path(Config::CONFIG_DIR_PATH) / "input.ini") << "[Controller 0]\nenabled=1\nenabled=0\n";
    assert(!loaded.load());
    const auto invalid = std::filesystem::path(Config::CONFIG_DIR_PATH) / "profile_invalid.ini";
    std::ofstream(invalid) << "name=Invalid\n[keyboard]\nleft=A\n[gamepad]\nleft=[0]\n";
    assert(!restored.loadProfile(invalid));
}

template<class T> void binary(std::ostream &out, const T &value) { out.write(reinterpret_cast<const char *>(&value), sizeof(value)); }
void testLegacy(int version) {
    {
        std::ofstream out("legacy.bin", std::ios::binary);
        binary(out, version); const auto chat = K::Enter; binary(out, chat);
        for (unsigned slot = 0; slot < 4; ++slot) {
            const bool enabled = slot == 1, no = false; const unsigned id = slot; const unsigned long long handle = 0;
            binary(out, enabled); binary(out, no); binary(out, id); binary(out, no); binary(out, no); binary(out, handle);
            for (int action = 0; action < 14; ++action) {
                if (version == 6 && action == Input::ACTION_WALK) continue;
                for (int alternate = 0; alternate < 2; ++alternate) {
                    const bool axisBind = alternate == 1; const auto axis = sf::Joystick::X; const unsigned button = 3;
                    binary(out, axisBind); binary(out, no); binary(out, axis); binary(out, button);
                }
                const auto key = K::F5; binary(out, key);
            }
        }
    }
    Input input(nullptr); estream file("legacy.bin", true, false);
    input.loadLegacyFormat(file);
    assert(file.file && file.tellg() == (version == 6 ? 1320 : 1416));
    assert(input.controllers[1].enabled && !input.controllers[0].enabled);
    for (const auto &c : input.controllers) {
        assert(c.keyBindings[Input::ACTION_JUMP].key == K::F5);
        assert(c.keyBindings[Input::ACTION_CHAT].key == K::Enter);
        assert(c.keyBindings[Input::ACTION_WALK].key == (version == 6 ? K::Down : K::F5));
        assert(!c.binding);
    }
}
}

int main() {
    const auto caseRoot = std::filesystem::current_path() /
        ("case-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(caseRoot);
    std::filesystem::current_path(caseRoot);
    testStateAndBinding();
    testDescriptorsAndGlyphs();
    testText();
    testIniAndProfiles();
    testLegacy(6); testLegacy(7);
    std::cout << "Input tests passed: snapshots, rebinding, descriptors, labels, text, INI/profiles, RNG and legacy 6/7\n";
}
