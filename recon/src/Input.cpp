// PDB contract; Ghidra Input unit, 005f8cc0..00608700.
#include "recon_all.h"
#include "localization.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <stdexcept>

#define VLOG(level) if ((level) > Log::reportingLevel()) ; else Log().get(level)

const unsigned int Input::MAX_PLAYERS = 4;
const int Input::BIND_TIME = 20;
const int Input::INPUT_FILE_FORMAT_VERSION = 7;
const std::vector<std::string> Input::ACTION_HANDLES = {
    "left", "right", "down", "up", "jump", "melee", "range", "skill",
    "swap", "walk", "playermenu", "pause", "interact", "emote", "chat"};

namespace {
std::string trimInput(const std::string &s) {
    const auto first = s.find_first_not_of(" \t\r\n");
    return first == std::string::npos ? "" : s.substr(first, s.find_last_not_of(" \t\r\n") - first + 1);
}
struct InputIni {
    using Section = std::map<std::string, std::string>;
    std::map<std::string, Section> sections;
    void read(const std::filesystem::path &path) {
        std::ifstream in(path);
        if (!in) throw std::runtime_error("Cannot open input INI");
        std::string line, section;
        while (std::getline(in, line)) {
            line = trimInput(line);
            if (line.empty() || line[0] == ';' || line[0] == '#') continue;
            if (line[0] == '[') {
                if (line.back() != ']') throw std::runtime_error("Malformed input section");
                section = trimInput(line.substr(1, line.size() - 2));
                if (sections.count(section)) throw std::runtime_error("Duplicate input section");
                sections[section];
            } else {
                const auto equal = line.find('=');
                if (equal == std::string::npos) throw std::runtime_error("Malformed input entry");
                auto key = trimInput(line.substr(0, equal));
                if (key.empty() || !sections[section].emplace(key, trimInput(line.substr(equal + 1))).second)
                    throw std::runtime_error("Duplicate input entry");
            }
        }
    }
    std::string get(const std::string &section, const std::string &key, const std::string &fallback = "") const {
        const auto s = sections.find(section);
        if (s == sections.end()) return fallback;
        const auto k = s->second.find(key);
        return k == s->second.end() ? fallback : k->second;
    }
    template<class T> T number(const std::string &section, const std::string &key, T fallback) const {
        std::istringstream in(get(section, key));
        T value{};
        if (!(in >> value) || !(in >> std::ws).eof()) return fallback;
        return value;
    }
    bool boolean(const std::string &section, const std::string &key, bool fallback) const {
        const auto value = get(section, key);
        if (value == "true" || value == "1") return true;
        if (value == "false" || value == "0") return false;
        return fallback;
    }
};
std::string localizedInput(const std::string &s, const char *context = nullptr) {
    if (context) {
        const auto contextual = std::string(context) + '\004' + s;
        const auto translated = recon::translate(contextual);
        if (translated != contextual) return translated;
    }
    return recon::translate(s);
}
std::string inputFormat(const std::string &s, unsigned value) {
    auto result = localizedInput(s, "Gamepad Binds");
    const auto token = result.find("{1}");
    if (token != std::string::npos) result.replace(token, 3, std::to_string(value));
    return result;
}
std::string controllerSection(unsigned slot) { return "Controller " + std::to_string(slot); }
std::string randomProfileFilename() {
    // Game lambda 00604970. MSVC's uniform_int(0, INT_MAX) at 0041cce0
    // consumes one MT19937 draw and takes its remainder modulo 0x80000000.
    const std::string consonants = "bcdfghjklmnprstvwxyz";
    std::string word = "_";
    for (int i = 1; i < 5; ++i) {
        const char previous = word.back();
        const std::string next = previous == 's' ? "aeiouyt" : previous == 'u' ? "bcdefghjklmnprstvwxyz"
            : (previous == '_' || std::string("aeio").find(previous) != std::string::npos) ? consonants : "aeiou";
        const std::uint32_t draw = _randEngine() & 0x7fffffffu;
        word += next[draw % next.size()];
    }
    return "profile" + word + ".ini";
}
bool sfJoyButton(unsigned id, unsigned button) {
    return id < sf::Joystick::Count && button < sf::Joystick::ButtonCount && sf::Joystick::isButtonPressed(id, button);
}
float sfJoyAxis(unsigned id, sf::Joystick::Axis axis) {
    return id < sf::Joystick::Count && unsigned(axis) < sf::Joystick::AxisCount ? sf::Joystick::getAxisPosition(id, axis) : 0.f;
}
bool heldJoystickBind(const ControlScheme &scheme, const ActionBind &bind) {
    if (!bind.joyUsingAxis) return sfJoyButton(scheme.joystickId, bind.joyButton);
    const float value = sfJoyAxis(scheme.joystickId, bind.joyAxis);
    const float threshold = scheme.specialL2R2 && (bind.joyAxis == sf::Joystick::Z || bind.joyAxis == sf::Joystick::R) ? 0.f : 50.f;
    return bind.joyAxisPositive ? value > threshold : value < -threshold;
}
template<class T> void readInputValue(estream &stream, T &value) {
    stream.file.read(reinterpret_cast<char *>(&value), sizeof(value));
}
}

ActionBind::ActionBind() : isKey(false), key(sf::Keyboard::A), joyButton(0), joyAxis(sf::Joystick::Z),
    joyAxisPositive(false), joyUsingAxis(false), bound(false) {}
ActionBind::ActionBind(sf::Keyboard::Key value) : ActionBind() { isKey = true; key = value; bound = true; }
ActionBind::ActionBind(unsigned int button) : ActionBind() { joyButton = button; bound = true; }
ActionBind::ActionBind(unsigned int axis, bool positive) : ActionBind() {
    joyAxis = static_cast<sf::Joystick::Axis>(axis); joyAxisPositive = positive; joyUsingAxis = true; bound = true;
}
InputProfile::InputProfile() : keyboard(false), isDefault(false) {}
Input::ControllerIdentifier::ControllerIdentifier(std::string value)
    : keyboard(false), steamController(false), joystickId(-1), steamControllerHandle(0), name(std::move(value)) {}

Input::Input(sf::Window *) : steamControllerActionHandles{}, steamControllerActionSetHandle(0),
    joystickDetected(false), enteringText(false), enteredTextPressedReturn(false), skipInputThisFrame(false),
    someConnectEvent(false), window(nullptr), textBuffer(nullptr), allowNewLines(false),
    textCursorIndexStart(0), textCursorIndexEnd(0), steamControllerInitialized(false), controllerSpritesInitialized(false) {
    VLOG(LOG_DEBUG) << "Input::Input";
    actionNames.resize(NUM_ACTIONS, "UNDEFINED");
    updateActionNames();
    keys.resize(sf::Keyboard::KeyCount, false);
    prevKeys.resize(sf::Keyboard::KeyCount, false);
    reset(-1);
    controllersDetected.clear();
}
Input::~Input() = default;

void Input::reset(int) {
    // The optimized body resets all four slots regardless of the retained int argument.
    someConnectEvent = false;
    joystickDetected = false;
    for (unsigned i = 0; i < MAX_PLAYERS; ++i) if (sf::Joystick::isConnected(i)) { joystickDetected = true; break; }
    controllerSwitchedTimers.resize(MAX_PLAYERS, 0.f);
    keyboardSwitchedTimers.resize(MAX_PLAYERS, 0.f);
    controllers.clear();
    controllers.resize(MAX_PLAYERS, ControlScheme{});
    for (unsigned i = 0; i < MAX_PLAYERS; ++i) {
        auto &c = controllers[i];
        c.slot = i; c.enabled = i == 0;
        c.currentlyBinding = 0; c.bindingAlternate = false;
        c.keyBindings.resize(NUM_ACTIONS, ActionBind(sf::Keyboard::F12));
        c.joystickBindings.resize(NUM_ACTIONS);
        c.alternateJoystickBindings.resize(NUM_ACTIONS);
        c.binded.assign(NUM_ACTIONS, false);
        c.actions.assign(NUM_ACTIONS, false);
        c.prevActions.assign(NUM_ACTIONS, false);
        c.bindTimer = 0; c.binding = true; c.controllerChooseTimer = 0;
    }
    enteringText = false;
    if (window) window->setKeyRepeatEnabled(false);
}

void Input::updateActionNames() {
    static const char *names[] = {"Left", "Right", "Down", "Up", "Jump", "Melee Attack", "Ranged Attack",
        "Cast Spell", "Swap/Cancel", "Walk", "Menu", "Pause", "Interact", "Emote (Multiplayer Only)",
        "In-Game Chat (Multiplayer Only)"};
    actionNames.resize(NUM_ACTIONS, "UNDEFINED");
    for (int i = 0; i < NUM_ACTIONS; ++i) actionNames[i] = localizedInput(names[i]);
}
std::string Input::getActionHandle(Actions action) {
    if (unsigned(action) < NUM_ACTIONS) return ACTION_HANDLES[action];
    VLOG(LOG_ERROR) << "Input::getActionHandle Cannot find handle for action: " << action;
    return "";
}
Input::Actions Input::getActionFromHandle(std::string handle) {
    const auto found = std::find(ACTION_HANDLES.begin(), ACTION_HANDLES.end(), handle);
    return found == ACTION_HANDLES.end() ? NUM_ACTIONS : static_cast<Actions>(found - ACTION_HANDLES.begin());
}
bool Input::isHeld(Actions action, unsigned slot) {
    return controllers[slot].enabled && controllers[slot].actions[action];
}
bool Input::isPressed(Actions action, unsigned slot) {
    return controllers[slot].enabled && controllers[slot].actions[action] && !controllers[slot].prevActions[action];
}
bool Input::isReleased(Actions action, unsigned slot) {
    return controllers[slot].enabled && !controllers[slot].actions[action] && controllers[slot].prevActions[action];
}
bool Input::isAnyHeld(Actions action) { for (unsigned i = 0; i < controllers.size(); ++i) if (isHeld(action, i)) return true; return false; }
bool Input::isAnyPressed(Actions action) { for (unsigned i = 0; i < controllers.size(); ++i) if (isPressed(action, i)) return true; return false; }
bool Input::isAnyReleased(Actions action) { for (unsigned i = 0; i < controllers.size(); ++i) if (isReleased(action, i)) return true; return false; }
bool Input::isBound(Actions action, unsigned slot) { return controllers[slot].binded[action]; }
bool Input::isKeyHeld(sf::Keyboard::Key key) { return unsigned(key) < keys.size() && keys[key]; }
bool Input::isKeyPressed(sf::Keyboard::Key key) { return isKeyHeld(key) && !prevKeys[key]; }
bool Input::isKeyReleased(sf::Keyboard::Key key) { return unsigned(key) < keys.size() && !keys[key] && prevKeys[key]; }
unsigned short Input::getNetState(unsigned int) {
    // Original 005fa040 serializes slot zero; the PDB argument is unused.
    unsigned short state = 0;
    for (int i = 0; i < NUM_ACTIONS; ++i) if (controllers[0].actions[i]) state |= 1u << i;
    return state;
}
void Input::setNetState(unsigned short state, unsigned slot) {
    controllers[slot].prevActions = controllers[slot].actions;
    for (int i = 0; i < NUM_ACTIONS; ++i) controllers[slot].actions[i] = (state & (1u << i)) != 0;
}
void Input::setWindow(sf::Window *value) { window = value; }
void Input::step() {
    keys.swap(prevKeys);
    for (int i = 0; i < sf::Keyboard::KeyCount; ++i) keys[i] = sf::Keyboard::isKeyPressed(static_cast<sf::Keyboard::Key>(i));
    for (auto &c : controllers) {
        if (c.binding) stepBindController(c);
        else if (c.enabled) stepController(c);
    }
}
void Input::stepController(ControlScheme &c) {
    c.actions.swap(c.prevActions);
    bool releasedBinding = true;
    if (!c.usingJoystick) {
        for (int i = 0; i < NUM_ACTIONS; ++i) {
            const auto key = c.keyBindings[i].key;
            if (unsigned(key) >= keys.size()) {
                VLOG(LOG_ERROR) << "Input::stepController keys index out of bounds: " << int(key) << ".";
                continue;
            }
            c.actions[i] = keys[key];
            if (c.justBinded && key == c.lastBindedKeyEvent && c.actions[i]) {
                releasedBinding = false; c.actions[i] = false;
            }
        }
    } else if (!c.usingSteamController) {
        for (int i = 0; i < NUM_ACTIONS; ++i) {
            auto &bind = c.joystickBindings[i];
            c.actions[i] = heldJoystickBind(c, bind);
            if (!c.actions[i] && i < 4 && c.alternateJoystickBindings[i].bound)
                c.actions[i] = heldJoystickBind(c, c.alternateJoystickBindings[i]);
            if (c.justBinded && joyActionsEqual(bind, c.lastBindedAction) && c.actions[i]) {
                releasedBinding = false; c.actions[i] = false;
            }
        }
    } else {
        // Steam controller polling is excluded; ordinary controllers use SFML.
        std::fill(c.actions.begin(), c.actions.end(), false);
    }
    if (c.justBinded && releasedBinding) c.justBinded = false;
    if (c.tryToUseJoystick != c.usingJoystick) setUsingJoystick(c, c.tryToUseJoystick);
}
bool Input::joyActionsEqual(ActionBind &a, ActionBind &b) {
    return a.joyUsingAxis == b.joyUsingAxis && (a.joyUsingAxis
        ? a.joyAxis == b.joyAxis && a.joyAxisPositive == b.joyAxisPositive : a.joyButton == b.joyButton);
}

void Input::countUsedKeys() {
    keysBoundCount.clear();
    for (const auto &c : controllers) if (c.enabled && !c.usingJoystick)
        for (const auto &bind : c.keyBindings) ++keysBoundCount[bind.key];
}
bool Input::isKeyBoundToSomething(sf::Keyboard::Key key) {
    const auto found = keysBoundCount.find(key); return found != keysBoundCount.end() && found->second > 0;
}
void Input::bindKey(ControlScheme &c, Actions action, sf::Keyboard::Key key) {
    c.keyBindings[action] = ActionBind(key); c.binded[action] = true;
}
void Input::cancelBinding(int slot) {
    debugStates.clear(); auto &c = controllers[slot]; c.binding = false;
    std::fill(c.binded.begin(), c.binded.end(), true); c.currentlyBinding = NUM_ACTIONS; countUsedKeys();
}
void Input::stepBindController(ControlScheme &c) {
    for (int action = 0; action < NUM_ACTIONS; ++action) {
        if (c.binded[action]) continue;
        c.currentlyBinding = action;
        if (!c.usingJoystick) {
            const auto previous = c.currKey;
            unsigned held = 0;
            for (int key = 0; key < sf::Keyboard::KeyCount; ++key) if (keys[key]) { ++held; c.currKey = static_cast<sf::Keyboard::Key>(key); }
            if (held == 0) c.justBinded = false;
            if (held == 1 && c.currKey == previous && !c.justBinded) {
                if (c.bindTimer < BIND_TIME && ++c.bindTimer == BIND_TIME) {
                    bindKey(c, static_cast<Actions>(action), c.currKey);
                    c.bindTimer = 0; c.lastBindedKeyEvent = c.currKey; c.justBinded = true;
                }
            } else c.bindTimer = 0;
        } else {
            debugStates.clear();
            debugStates.push_back("-DEBUG JOYSTICK BINDING-");
            debugStates.push_back("Product[" + std::to_string(c.joystickProductId) + "] / Vendor: [" + std::to_string(c.joystickVendorId) + "]");
            const auto previousButton = c.currButton;
            const auto previousAxis = c.currAxis;
            unsigned held = 0;
            for (unsigned button = 0; button < 32; ++button) if (sfJoyButton(c.joystickId, button)) {
                ++held; c.currButton = button; debugStates.push_back("Button Held: " + std::to_string(button));
            }
            if (c.validAxes.empty()) for (unsigned a = 0; a < 8; ++a) c.validAxes.push_back(static_cast<sf::Joystick::Axis>(a));
            c.bindingAxis = false;
            for (unsigned i = 0; i < c.validAxes.size(); ++i) {
                const auto axis = c.validAxes[i]; const float value = sfJoyAxis(c.joystickId, axis);
                const bool special = c.specialL2R2 && (i == 2 || i == 3);
                if ((special && value > 0.f) || (!special && std::abs(value) > 40.f)) {
                    ++held; c.bindingAxis = true; c.currAxis = axis; c.currAxisPositive = special || value > 0.f;
                    std::ostringstream line;
                    line << "Axis Held: " << int(axis) << (special ? " positive at position: " : " negative at position: ") << value;
                    debugStates.push_back(line.str());
                }
            }
            if (held == 0) c.justBinded = false;
            if (held == 1 && (previousButton == c.currButton || previousAxis == c.currAxis) && !c.justBinded) {
                if (c.bindTimer < BIND_TIME && ++c.bindTimer == BIND_TIME) {
                    ActionBind bind(c.currButton);
                    bind.joyAxis = c.currAxis; bind.joyAxisPositive = c.currAxisPositive; bind.joyUsingAxis = c.bindingAxis;
                    (c.bindingAlternate ? c.alternateJoystickBindings : c.joystickBindings)[action] = bind;
                    c.binded[action] = true; c.lastBindedAction = bind; c.justBinded = true; c.bindTimer = 0;
                }
            } else c.bindTimer = 0;
        }
        return;
    }
    c.binding = false; debugStates.clear(); countUsedKeys();
}

void ControlScheme::copyToProfile(InputProfile &profile, bool keyboard) {
    profile.bindings.clear(); profile.secondaryBindings.clear(); profile.keyboard = keyboard;
    profile.bindings = keyboard ? keyBindings : joystickBindings;
    profile.secondaryBindings = keyboard ? keyBindings : alternateJoystickBindings;
}
void ControlScheme::loadFromProfile(InputProfile &profile) {
    tryToUseJoystick = !profile.keyboard;
    for (unsigned i = 0; i < profile.bindings.size(); ++i) {
        binded[i] = true;
        if (profile.keyboard) keyBindings[i] = ActionBind(profile.bindings[i].key);
        else {
            joystickBindings[i] = profile.bindings[i];
            // Disk profiles store one bind per action. The original loader does
            // not populate secondaryBindings; avoid reading past its empty vector.
            alternateJoystickBindings[i] = i < profile.secondaryBindings.size() ? profile.secondaryBindings[i] : ActionBind();
        }
    }
}

void Input::loadDefaults(int) {
    static const sf::Keyboard::Key keyboard[] = {sf::Keyboard::Left, sf::Keyboard::Right, sf::Keyboard::Down,
        sf::Keyboard::Up, sf::Keyboard::Z, sf::Keyboard::X, sf::Keyboard::C, sf::Keyboard::V, sf::Keyboard::A,
        sf::Keyboard::Down, sf::Keyboard::Space, sf::Keyboard::Escape, sf::Keyboard::LShift, sf::Keyboard::LControl, sf::Keyboard::Y};
    static const unsigned buttons[] = {2, 3, 1, 0, 10, 12, 11, 13, 14, 1, 4, 5, 15, 8, 7};
    for (auto &c : controllers) {
        std::fill(c.binded.begin(), c.binded.end(), true); c.binding = false; c.usingJoystick = false; c.usingSteamController = false;
        for (int i = 0; i < NUM_ACTIONS; ++i) { c.keyBindings[i] = ActionBind(keyboard[i]); c.joystickBindings[i] = ActionBind(buttons[i]); }
        c.alternateJoystickBindings[ACTION_LEFT] = ActionBind(unsigned(sf::Joystick::X), false);
        c.alternateJoystickBindings[ACTION_RIGHT] = ActionBind(unsigned(sf::Joystick::X), true);
        c.alternateJoystickBindings[ACTION_UP] = ActionBind(unsigned(sf::Joystick::Y), true);
        c.alternateJoystickBindings[ACTION_DOWN] = ActionBind(unsigned(sf::Joystick::Y), false);
        c.lastBindedKeyEvent = sf::Keyboard::Unknown; c.lastBindedAction.joyButton = 99;
        c.lastBindedAction.joyUsingAxis = false; c.justBinded = false;
    }
    countUsedKeys();
}

void Input::generateAndLoadDefaultProfiles() {
    const sf::Keyboard::Key keyboard[3][NUM_ACTIONS] = {
        {sf::Keyboard::Left,sf::Keyboard::Right,sf::Keyboard::Down,sf::Keyboard::Up,sf::Keyboard::S,sf::Keyboard::D,sf::Keyboard::F,sf::Keyboard::A,sf::Keyboard::W,sf::Keyboard::Down,sf::Keyboard::Space,sf::Keyboard::Escape,sf::Keyboard::E,sf::Keyboard::Tab,sf::Keyboard::Enter},
        {sf::Keyboard::Left,sf::Keyboard::Right,sf::Keyboard::Down,sf::Keyboard::Up,sf::Keyboard::Z,sf::Keyboard::X,sf::Keyboard::C,sf::Keyboard::V,sf::Keyboard::A,sf::Keyboard::Down,sf::Keyboard::Space,sf::Keyboard::Escape,sf::Keyboard::LShift,sf::Keyboard::LControl,sf::Keyboard::Enter},
        {sf::Keyboard::S,sf::Keyboard::F,sf::Keyboard::D,sf::Keyboard::E,sf::Keyboard::Space,sf::Keyboard::J,sf::Keyboard::K,sf::Keyboard::L,sf::Keyboard::A,sf::Keyboard::D,sf::Keyboard::H,sf::Keyboard::G,sf::Keyboard::Semicolon,sf::Keyboard::R,sf::Keyboard::T}};
    const char *names[] = {"Default Keyboard", "Classic Keyboard", "Keyboard Home-Row"};
    for (int profile = 0; profile < 3; ++profile) {
        InputProfile p; p.isDefault = true; p.keyboard = true; p.profileName = names[profile];
        p.bindings.resize(NUM_ACTIONS); p.secondaryBindings.resize(NUM_ACTIONS);
        for (int i = 0; i < NUM_ACTIONS; ++i) p.bindings[i].key = keyboard[profile][i];
        inputProfiles.push_back(p);
    }
    InputProfile gamepad; gamepad.isDefault = true; gamepad.profileName = "Default (XInput)";
    gamepad.bindings.resize(NUM_ACTIONS); gamepad.secondaryBindings.resize(NUM_ACTIONS);
    const unsigned buttons[] = {2,3,1,0,10,12,11,13,14,1,4,5,15,8,7};
    for (int i = 0; i < NUM_ACTIONS; ++i) gamepad.bindings[i] = ActionBind(buttons[i]);
    gamepad.secondaryBindings[0] = ActionBind(unsigned(sf::Joystick::X), false);
    gamepad.secondaryBindings[1] = ActionBind(unsigned(sf::Joystick::X), true);
    gamepad.secondaryBindings[3] = ActionBind(unsigned(sf::Joystick::Y), true);
    gamepad.secondaryBindings[2] = ActionBind(unsigned(sf::Joystick::Y), false);
    inputProfiles.push_back(gamepad);
    for (auto &c : controllers) {
        for (auto &p : inputProfiles) if (p.isDefault && !p.keyboard) { c.loadFromProfile(p); break; }
        for (auto &p : inputProfiles) if (p.isDefault && p.keyboard) { c.loadFromProfile(p); break; }
    }
}

std::string Input::getKeyName(const sf::Keyboard::Key key) {
    static const char *names[] = {
        "A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P","Q","R","S","T","U","V","W","X","Y","Z",
        "Num0","Num1","Num2","Num3","Num4","Num5","Num6","Num7","Num8","Num9",
        "Escape","LControl","LShift","LAlt","LSystem","RControl","RShift","RAlt","RSystem","Menu",
        "LBracket","RBracket","SemiColon","Comma","Period","Quote","Slash","BackSlash","Tilde","Equal","Dash",
        "Space","Return","BackSpace","Tab","PageUp","PageDown","End","Home","Insert","Delete","Add","Subtract","Multiply","Divide",
        "Left","Right","Up","Down","Numpad0","Numpad1","Numpad2","Numpad3","Numpad4","Numpad5","Numpad6","Numpad7","Numpad8","Numpad9",
        "F1","F2","F3","F4","F5","F6","F7","F8","F9","F10","F11","F12","F13","F14","F15","Pause"};
    return unsigned(key) < std::size(names) ? names[key] : "Unknown";
}
sf::Keyboard::Key Input::getKeyFromName(std::string name) {
    for (int i = 0; i < sf::Keyboard::KeyCount; ++i) if (getKeyName(static_cast<sf::Keyboard::Key>(i)) == name) return static_cast<sf::Keyboard::Key>(i);
    return sf::Keyboard::Unknown;
}
std::vector<std::string> Input::getDescriptors(std::string descriptors) {
    std::vector<std::string> result;
    for (;;) {
        const auto split = descriptors.find('|');
        const auto part = trimInput(descriptors.substr(0, split));
        if (!part.empty()) result.push_back(part);
        if (split == std::string::npos) return result;
        descriptors.erase(0, split + 1);
    }
}
std::string Input::getDescriptorFromBind(const ActionBind &bind, bool keyboard) {
    if (keyboard) return getKeyName(bind.key);
    return "[" + (bind.joyUsingAxis ? std::string(bind.joyAxisPositive ? "+" : "-") + std::to_string(int(bind.joyAxis))
                                  : std::to_string(bind.joyButton)) + "]";
}
ActionBind Input::getBindFromDescriptor(std::string descriptor) {
    ActionBind bind;
    if (descriptor.empty()) {
        VLOG(LOG_ERROR) << "Input::getBindFromDescriptor Cannot create bind from empty descriptor.";
        return bind;
    }
    // Actual cmpb immediates at 006022b9/006022cb/006023c2/006023d0:
    // '[', ']', '+', '-'. Ghidra mislabels these byte comparisons as bools.
    if (descriptor.size() > 2 && descriptor.front() == '[') {
        if (descriptor.back() != ']') {
            VLOG(LOG_ERROR) << "Input::getBindFromDescriptor Missing closing bracket ] on gamepad bind descriptor.";
            return bind;
        }
        descriptor = descriptor.substr(1, descriptor.size() - 2);
        bind.bound = true;
        if (descriptor[0] == '+' || descriptor[0] == '-') {
            bind.joyUsingAxis = true; bind.joyAxisPositive = descriptor[0] == '+';
            unsigned axis = unsigned(sf::Joystick::Z);
            std::istringstream(descriptor.substr(1)) >> axis;
            bind.joyAxis = static_cast<sf::Joystick::Axis>(axis);
        } else std::istringstream(descriptor) >> bind.joyButton;
        return bind;
    }
    return ActionBind(getKeyFromName(descriptor));
}

ControlsType Input::getControlsType(ControlScheme &c) {
    if (c.usingSteamController) return CONTROLS_STEAM;
    if (!c.usingJoystick) return CONTROLS_KEYBOARD;
    if (c.tryToUseJoystick) {
        if (c.joystickVendorId == 0x45e && (c.joystickProductId == 0x2d1 || c.joystickProductId == 0x2ff)) return CONTROLS_XBOX_ONE;
        // SFML identification replaces the original raw xinput device flag.
        if (c.joystickVendorId == 0x45e) return CONTROLS_XBOX_360;
        if (c.joystickVendorId == 0x54c && c.joystickProductId == 0x5c4) return CONTROLS_PS4;
        if (c.joystickVendorId == 0x54c && c.joystickProductId == 0x268) return CONTROLS_PS3;
    }
    return CONTROLS_GENERIC_GAMEPAD;
}
bool Input::isUsingXInputController(ControlScheme &c) { return c.controlsType == CONTROLS_XBOX_360 || c.controlsType == CONTROLS_XBOX_ONE; }
bool Input::isUsingXboxOneController(ControlScheme &c) { return c.controlsType == CONTROLS_XBOX_ONE; }
bool Input::isUsingPs3Controller(ControlScheme &c) { return c.controlsType == CONTROLS_PS3; }
bool Input::isUsingPs4Controller(ControlScheme &c) { return c.controlsType == CONTROLS_PS4; }
bool Input::isUsingKeyboard(ControlScheme &c) { return !c.usingJoystick; }
bool Input::isJoystickConnected(int id) { return unsigned(id) < sf::Joystick::Count && sf::Joystick::isConnected(id); }
void Input::setUsingJoystick(ControlScheme &c, bool use) {
    const bool previous = c.usingJoystick;
    c.usingJoystick = use; c.tryToUseJoystick = use;
    if (use) {
        if (!isJoystickConnected(c.joystickId)) c.usingJoystick = false;
        else {
            const auto id = sf::Joystick::getIdentification(c.joystickId);
            c.joystickName = id.name.toAnsiString(); c.joystickVendorId = id.vendorId; c.joystickProductId = id.productId;
            c.controlsType = getControlsType(c);
            c.usingSteamController = false; // Steam discovery is excluded.
            c.specialL2R2 = false; c.validAxes.clear();
            for (unsigned axis = 0; axis < sf::Joystick::AxisCount; ++axis) c.validAxes.push_back(static_cast<sf::Joystick::Axis>(axis));
        }
    }
    c.controlsType = getControlsType(c);
    if (previous != c.usingJoystick) {
        (c.usingJoystick ? controllerSwitchedTimers : keyboardSwitchedTimers)[c.slot] = 1.f;
        (c.usingJoystick ? keyboardSwitchedTimers : controllerSwitchedTimers)[c.slot] = 0.f;
    }
}
void Input::updateGamepadStatus() {
    sf::Joystick::update();
    joystickDetected = false; controllersDetected.clear();
    ControllerIdentifier keyboard("Keyboard"); keyboard.keyboard = true; controllersDetected.push_back(keyboard);
    // Steam discovery is excluded; SFML owns ordinary joystick enumeration.
    for (unsigned id = 0; id < sf::Joystick::Count; ++id) if (sf::Joystick::isConnected(id)) {
        joystickDetected = true;
        ControllerIdentifier controller(sf::Joystick::getIdentification(id).name.toAnsiString());
        controller.joystickId = id; controllersDetected.push_back(controller);
    }
    for (auto &c : controllers) setUsingJoystick(c, c.tryToUseJoystick);
}
void Input::joystickConnectEvent(unsigned id) {
    for (auto &c : controllers) if (c.usingJoystick && c.binding && c.joystickId == id) c.binding = false;
    someConnectEvent = true; VLOG(LOG_DEBUG) << "Input::joystickConnectEvent: " << id;
    updateGamepadStatus();
}
void Input::joystickDisconnectEvent(unsigned id) {
    for (auto &c : controllers) if (c.usingJoystick && c.binding && c.joystickId == id) c.binding = false;
    someConnectEvent = true; VLOG(LOG_DEBUG) << "Input::joystickDisconnectEvent: " << id;
    updateGamepadStatus();
}
void Input::tryToUseAController() {
    auto &c = controllers[0];
    // Steam preference/discovery is excluded. Preserve the non-Steam fallback.
    if (!c.usingSteamController && !c.usingJoystick) {
        for (const auto &device : controllersDetected) if (!device.keyboard && !device.steamController) {
            controllerSwitchedTimers[c.slot] = 1.f; keyboardSwitchedTimers[c.slot] = 0.f;
            c.enabled = true; c.usingJoystick = true; c.tryToUseJoystick = true; c.joystickId = device.joystickId;
            const auto id = sf::Joystick::getIdentification(c.joystickId);
            c.joystickName = id.name.toAnsiString(); c.joystickVendorId = id.vendorId; c.joystickProductId = id.productId;
            c.controlsType = getControlsType(c); return;
        }
    }
}
void Input::initSteamController() { /* Steam controller API excluded. */ }
Sprite *Input::getSteamSprite(unsigned, Actions) { /* Steam glyph API excluded. */ return nullptr; }

std::string Input::getXInputBindName(ActionBind &bind) {
    // Historical name: this translates labels only; it does not call XInput.
    if (bind.joyUsingAxis) {
        switch (bind.joyAxis) {
        case sf::Joystick::X: return localizedInput(bind.joyAxisPositive ? "Right (L-Stk)" : "Left (L-Stk)");
        case sf::Joystick::Y: return localizedInput(bind.joyAxisPositive ? "Down (L-Stk)" : "Up (L-Stk)");
        case sf::Joystick::R: return localizedInput(bind.joyAxisPositive ? "Down (R-Stk)" : "Up (R-Stk)");
        case sf::Joystick::U: return localizedInput(bind.joyAxisPositive ? "Right (R-Stk)" : "Left (R-Stk)");
        default: return "?";
        }
    }
    static const char *names[] = {"Up (D-pad)", "Down (D-pad)", "Left (D-pad)", "Right (D-pad)",
        "Start", "Back", "Left Thumb", "Right Thumb", "Left Shoulder", "Right Shoulder", "A", "B", "X", "Y", "Left Trigger", "Right Trigger"};
    return bind.joyButton < std::size(names) ? localizedInput(names[bind.joyButton]) : "?";
}
std::string Input::getBindName(Actions action, unsigned slot, bool alternate) {
    auto &c = controllers[slot];
    if (c.enabled && !c.binded[action] && c.currentlyBinding == action && alternate == c.bindingAlternate) {
        if (c.bindTimer != 0) return std::string(c.bindTimer / 2, '|');
        return localizedInput(c.usingJoystick ? "HOLD A BUTTON" : "HOLD A KEY");
    }
    if (!c.usingJoystick) return alternate ? "-" : getKeyName(c.keyBindings[action].key);
    if (c.usingSteamController) return localizedInput("ERROR"); // Original fallback when Steam is unavailable.
    auto &bind = (alternate ? c.alternateJoystickBindings : c.joystickBindings)[action];
    if (!bind.bound) return "";
    if (alternate && joyActionsEqual(c.joystickBindings[action], bind)) return "-";
    if (isUsingXInputController(c)) return getXInputBindName(bind);
    if (!bind.joyUsingAxis) return inputFormat("Button {1}", bind.joyButton);
    return inputFormat(bind.joyAxisPositive ? "Axis {1}+" : "Axis {1}-", unsigned(bind.joyAxis));
}

void Input::setTextCursorToEnd() {
    if (textBuffer) textCursorIndexStart = textCursorIndexEnd = int(textBuffer->size());
}
void Input::setEnteringText(std::string *buffer, int position, bool newLines) {
    allowNewLines = newLines; textBuffer = buffer; enteringText = buffer != nullptr;
    if (buffer) textCursorIndexStart = textCursorIndexEnd = position;
    if (window) window->setKeyRepeatEnabled(enteringText);
    enteredTextPressedReturn = false;
}
void Input::stopEnteringText() { enteringText = false; if (window) window->setKeyRepeatEnabled(false); textBuffer = nullptr; }
void Input::stopEnteringTextIfBufferEqual(std::string *buffer) { if (textBuffer == buffer) stopEnteringText(); }
void Input::clearTextBuffer() { if (textBuffer) { textBuffer->clear(); textCursorIndexStart = textCursorIndexEnd = 0; } }
std::string Input::getTextBuffer() { return textBuffer ? *textBuffer : ""; }
std::string Input::getSelectedText() {
    if (!textBuffer) return "";
    const auto range = std::minmax(textCursorIndexStart, textCursorIndexEnd);
    return textBuffer->substr(range.first, range.second - range.first);
}
void Input::deleteSelectedText() {
    if (!textBuffer || textCursorIndexStart == textCursorIndexEnd) {
        VLOG(LOG_DEBUG) << "Input::deleteSelectedText Nothing to delete.";
        return;
    }
    const int start = std::min(textCursorIndexStart, textCursorIndexEnd);
    const int end = std::max(textCursorIndexStart, textCursorIndexEnd);
    textBuffer->erase(start, end - start); textCursorIndexStart = textCursorIndexEnd = start;
}
void Input::selectAllText() { if (textBuffer) { textCursorIndexStart = 0; textCursorIndexEnd = int(textBuffer->size()); } }
void Input::insertText(std::string text) {
    if (!textBuffer) return;
    textBuffer->insert(textCursorIndexEnd, text);
    textCursorIndexStart += int(text.size()); textCursorIndexEnd += int(text.size());
}
void Input::inputChar(unsigned codepoint) {
    if (!textBuffer) return;
    if (textCursorIndexStart != textCursorIndexEnd) deleteSelectedText();
    if (codepoint == 0) return; // The original inserts the UTF-8 C string up to NUL.
    const auto bytes = sf::String(static_cast<sf::Uint32>(codepoint)).toUtf8();
    insertText(std::string(bytes.begin(), bytes.end()));
}
void Input::inputText(unsigned codepoint) { if (codepoint > 31 && (codepoint < 128 || codepoint > 159)) inputChar(codepoint); }
void Input::inputNewLine() { if (allowNewLines) inputChar(10); }
void Input::enteringTextKeyEvent(sf::Event::KeyEvent event, bool pressed) {
    if (!pressed || !textBuffer) return;
    using K = sf::Keyboard;
    if (event.code == K::Backspace || event.code == K::Delete) {
        int start = std::min(textCursorIndexStart, textCursorIndexEnd), end = std::max(textCursorIndexStart, textCursorIndexEnd);
        if (start == end) {
            if (event.code == K::Backspace && start != 0) --start;
            else if (event.code == K::Delete && end != int(textBuffer->size())) ++end;
        }
        textBuffer->erase(start, end - start); textCursorIndexStart = textCursorIndexEnd = start;
    } else if (event.code == K::Left || event.code == K::Right) {
        if (event.code == K::Left && textCursorIndexEnd > 0) {
            --textCursorIndexEnd;
            if (event.control) while (textCursorIndexEnd > 0 && (*textBuffer)[textCursorIndexEnd - 1] != ' ') --textCursorIndexEnd;
        } else if (event.code == K::Right && textCursorIndexEnd < int(textBuffer->size())) {
            ++textCursorIndexEnd;
            if (event.control) while (textCursorIndexEnd < int(textBuffer->size()) && (*textBuffer)[textCursorIndexEnd] != ' ') ++textCursorIndexEnd;
        }
        if (!event.shift) textCursorIndexStart = textCursorIndexEnd;
    } else if (event.code == K::Home || event.code == K::End) {
        textCursorIndexEnd = event.code == K::Home ? 0 : int(textBuffer->size());
        if (!event.shift) textCursorIndexStart = textCursorIndexEnd;
    } else if (event.code == K::Enter) {
        if (event.shift && allowNewLines) inputChar(10);
        else { stopEnteringText(); enteredTextPressedReturn = true; skipInputThisFrame = true; }
    } else if (event.control) {
        if (event.code == K::C) tryCopy();
        else if (event.code == K::V) tryPaste();
        else if (event.code == K::X) { tryCopy(); deleteSelectedText(); }
        else if (event.code == K::A) selectAllText();
    }
}
void Input::tryCopy() {
    if (!enteringText || !textBuffer || !window) return;
    const auto selection = getSelectedText();
    if (!selection.empty()) sf::Clipboard::setString(sf::String::fromUtf8(selection.begin(), selection.end()));
}
void Input::tryPaste() {
    if (!enteringText || !window) return;
    const auto bytes = sf::Clipboard::getString().toUtf8();
    // Original paste inserts at the active cursor without deleting the selection.
    insertText(std::string(bytes.begin(), bytes.end()));
}

bool Input::load() {
    generateAndLoadDefaultProfiles();
    try {
        const auto dir = std::filesystem::path(Config::CONFIG_DIR_PATH);
        if (!std::filesystem::is_directory(dir) || !std::filesystem::is_regular_file(dir / "input.ini")) return false;
        InputIni ini; ini.read(dir / "input.ini");
        for (unsigned i = 0; i < controllers.size(); ++i) {
            auto &c = controllers[i]; const auto section = controllerSection(i);
            if (!ini.sections.count(section)) throw std::runtime_error("No such controller node");
            c.enabled = ini.boolean(section, "enabled", c.enabled);
            c.tryToUseJoystick = ini.boolean(section, "tryToUseJoystick", c.tryToUseJoystick);
            c.joystickId = ini.number<unsigned>(section, "joystickId", c.joystickId);
            c.usingSteamController = ini.boolean(section, "usingSteamController", c.usingSteamController);
            c.steamControllerHandle = ini.number<unsigned long long>(section, "steamControllerHandle", c.steamControllerHandle);
            for (int action = 0; action < NUM_ACTIONS; ++action) {
                c.joystickBindings[action].bound = false; c.alternateJoystickBindings[action].bound = false; c.binded[action] = true;
                for (const auto &descriptor : getDescriptors(ini.get(section, getActionHandle(static_cast<Actions>(action))))) {
                    auto bind = getBindFromDescriptor(descriptor);
                    if (bind.isKey) c.keyBindings[action] = bind;
                    else {
                        if (!c.joystickBindings[action].bound) {
                            c.joystickBindings[action] = bind; c.alternateJoystickBindings[action] = bind;
                        } else c.alternateJoystickBindings[action] = bind;
                        (c.joystickBindings[action].bound ? c.alternateJoystickBindings : c.joystickBindings)[action].bound = true;
                    }
                }
            }
        }
        updateActionNames(); return true;
    } catch (const std::exception &error) { VLOG(LOG_ERROR) << "Input::load: " << error.what(); return false; }
}
bool Input::save() {
    try {
        std::filesystem::create_directories(Config::CONFIG_DIR_PATH);
        std::ofstream out(std::filesystem::path(Config::CONFIG_DIR_PATH) / "input.ini");
        if (!out) return false;
        out << std::boolalpha;
        for (unsigned i = 0; i < controllers.size(); ++i) {
            const auto &c = controllers[i];
            out << '[' << controllerSection(i) << "]\nenabled=" << c.enabled << "\ntryToUseJoystick=" << c.tryToUseJoystick << '\n';
            if (c.tryToUseJoystick) out << "joystickId=" << c.joystickId << '\n';
            out << "usingSteamController=" << c.usingSteamController << '\n';
            if (c.usingSteamController) out << "steamControllerHandle=" << c.steamControllerHandle << '\n';
            for (int action = 0; action < NUM_ACTIONS; ++action) {
                std::vector<std::string> descriptors;
                if (c.joystickBindings[action].bound) descriptors.push_back(getDescriptorFromBind(c.joystickBindings[action], false));
                if (c.alternateJoystickBindings[action].bound && !joyActionsEqual(controllers[i].joystickBindings[action], controllers[i].alternateJoystickBindings[action]))
                    descriptors.push_back(getDescriptorFromBind(c.alternateJoystickBindings[action], false));
                if (c.keyBindings[action].bound) descriptors.push_back(getDescriptorFromBind(c.keyBindings[action], true));
                out << getActionHandle(static_cast<Actions>(action)) << '=';
                for (unsigned d = 0; d < descriptors.size(); ++d) { if (d) out << " | "; out << descriptors[d]; }
                if (descriptors.empty()) { VLOG(LOG_ERROR) << "Input::save No input binds bound, cannot get descriptor"; }
                out << '\n';
            }
        }
        return bool(out);
    } catch (const std::exception &error) { VLOG(LOG_ERROR) << "Input::save: " << error.what(); return false; }
}
void Input::saveProfiles() { for (auto &profile : inputProfiles) if (!profile.isDefault) profile.saveProfile(); }
void Input::loadProfiles() {
    const auto dir = std::filesystem::path(Config::CONFIG_DIR_PATH);
    if (!std::filesystem::is_directory(dir)) return;
    // Preserve filesystem iterator order, as the original Boost iterator did.
    for (const auto &entry : std::filesystem::directory_iterator(dir)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".ini" || entry.path().filename().string().find("profile") == std::string::npos) continue;
        const auto filename = entry.path().filename().string();
        auto existing = std::find_if(inputProfiles.begin(), inputProfiles.end(), [&](const InputProfile &p) { return p.filename == filename; });
        if (existing == inputProfiles.end()) { inputProfiles.emplace_back(); existing = inputProfiles.end() - 1; }
        existing->loadProfile(entry.path());
    }
}
std::vector<int> Input::getInputProfileIndices(bool keyboard) {
    std::vector<int> indices;
    for (unsigned i = 0; i < inputProfiles.size(); ++i) if (inputProfiles[i].keyboard == keyboard) indices.push_back(i);
    return indices;
}

bool InputProfile::saveProfile() {
    try {
        std::filesystem::create_directories(Config::CONFIG_DIR_PATH);
        if (bindings.size() > Input::NUM_ACTIONS) return false;
        if (filename.empty()) {
            filename = trimInput(profileName);
            for (auto &c : filename) { c = char(std::tolower(static_cast<unsigned char>(c))); if (c == ' ' || c == '.') c = '_'; }
            if (!filename.empty()) filename = "profile_" + filename + ".ini";
            else do { filename = randomProfileFilename(); } while (std::filesystem::exists(std::filesystem::path(Config::CONFIG_DIR_PATH) / filename));
        }
        std::ofstream out(std::filesystem::path(Config::CONFIG_DIR_PATH) / filename);
        if (!out) return false;
        out << "name=" << profileName << '\n' << '[' << (keyboard ? "keyboard" : "gamepad") << "]\n";
        for (unsigned i = 0; i < bindings.size(); ++i)
            out << Input::getActionHandle(static_cast<Input::Actions>(i)) << '=' << Input::getDescriptorFromBind(bindings[i], keyboard) << '\n';
        return bool(out);
    } catch (const std::exception &error) { VLOG(LOG_ERROR) << "InputProfile::saveProfile: " << error.what(); return false; }
}
bool InputProfile::loadProfile(boost::filesystem::path path) {
    try {
        if (!std::filesystem::is_directory(Config::CONFIG_DIR_PATH) || !std::filesystem::is_regular_file(path)) return false;
        filename = path.filename().string();
        InputIni ini; ini.read(path);
        profileName = ini.get("", "name", "unnamed profile");
        const bool hasKeyboard = ini.sections.count("keyboard"), hasGamepad = ini.sections.count("gamepad");
        if (hasKeyboard == hasGamepad) { VLOG(LOG_ERROR) << "InputProfile::loadProfile Invalid profile " << filename; return false; }
        keyboard = hasKeyboard; bindings.clear(); bindings.resize(Input::NUM_ACTIONS);
        for (int i = 0; i < Input::NUM_ACTIONS; ++i) {
            const auto descriptor = ini.get(keyboard ? "keyboard" : "gamepad", Input::getActionHandle(static_cast<Input::Actions>(i)));
            if (descriptor.empty()) { VLOG(LOG_ERROR) << "InputProfile::loadProfile Bind undefined in profile " << filename; return false; }
            bindings[i] = Input::getBindFromDescriptor(descriptor);
        }
        return true;
    } catch (const std::exception &error) { VLOG(LOG_ERROR) << "InputProfile::loadProfile: " << error.what(); return false; }
}

void Input::loadLegacyFormat(estream &stream) {
    loadDefaults(-1);
    int version = 0; readInputValue(stream, version);
    if (version != 6 && version != 7) {
        VLOG(LOG_DEBUG) << "Failed to upgrade old Input config file from " << version << " to 7";
        return;
    }
    sf::Keyboard::Key chat = sf::Keyboard::A; readInputValue(stream, chat);
    for (auto &c : controllers) {
        readInputValue(stream, c.enabled);
        bool use = false; readInputValue(stream, use); setUsingJoystick(c, use);
        readInputValue(stream, c.joystickId);
        bool unused = false; readInputValue(stream, unused);
        readInputValue(stream, c.usingSteamController); readInputValue(stream, c.steamControllerHandle);
        bool valid = !c.usingJoystick || c.joystickId < 8 && isJoystickConnected(c.joystickId);
        for (int i = 0; i < NUM_ACTIONS; ++i) {
            if (i == ACTION_CHAT) { c.keyBindings[i] = ActionBind(chat); continue; }
            if (version == 6 && i == ACTION_WALK) continue;
            auto &bind = c.joystickBindings[i]; auto &alt = c.alternateJoystickBindings[i];
            readInputValue(stream, bind.joyUsingAxis); readInputValue(stream, bind.joyAxisPositive);
            sf::Joystick::Axis axis = sf::Joystick::X; readInputValue(stream, axis);
            if (unsigned(axis) >= 8) { bind.joyAxis = sf::Joystick::X; valid = false; } else bind.joyAxis = axis;
            readInputValue(stream, bind.joyButton);
            readInputValue(stream, alt.joyUsingAxis); readInputValue(stream, alt.joyAxisPositive);
            readInputValue(stream, axis);
            if (unsigned(axis) >= 8) { alt.joyAxis = bind.joyAxis; valid = false; } else alt.joyAxis = axis;
            readInputValue(stream, alt.joyButton);
            if (c.usingJoystick && (c.joystickId > 7 || bind.joyButton > 31)) valid = false;
            sf::Keyboard::Key key = sf::Keyboard::A; readInputValue(stream, key); c.keyBindings[i] = ActionBind(key);
        }
        if (c.usingJoystick && !valid) c.usingJoystick = false;
        c.binding = false;
    }
}

void Input::initControllerSprites() {
    const char *paths[] = {"gui/controllers/xbox_32.png", "gui/controllers/keyboard_32.png",
        "gui/controllers/xboxone_32.png", "gui/controllers/ps_32.png", "gui/controllers/ps4_32.png",
        "gui/controllers/steam_32.png", "gui/controllers/generic.png"};
    Sprite *sprites[] = {&xbox360Sprite, &keyboardSprite, &xboxOneSprite, &ps3Sprite, &ps4Sprite, &steamSprite, &genericSprite};
    for (unsigned i = 0; i < std::size(sprites); ++i)
        *sprites[i] = Sprite(std::weak_ptr<Entity>{}, paths[i], 32, 32, -1, 0, 0.f, 0.f, false, false);
    for (auto *sprite : sprites) sprite->setScale(0.5f);
    controllerSpritesInitialized = true;
}
int Input::getXInputButtonSpriteId(unsigned slot, Actions action, bool alternate) {
    const auto &bind = (alternate ? controllers[slot].alternateJoystickBindings : controllers[slot].joystickBindings)[action];
    if (!bind.joyUsingAxis) {
        static const int frames[] = {17,16,15,18,6,7,12,13,8,9,0,1,2,3,10,11};
        return bind.joyButton < std::size(frames) ? frames[bind.joyButton] : -1;
    }
    switch (bind.joyAxis) {
    case sf::Joystick::X: return bind.joyAxisPositive ? 23 : 20;
    case sf::Joystick::Y: return 21 + int(bind.joyAxisPositive);
    case sf::Joystick::Z: return bind.joyAxisPositive ? 28 : 25;
    case sf::Joystick::R: return 26 + int(bind.joyAxisPositive);
    default: return -1;
    }
}
int Input::getXboxButtonSpriteId(unsigned slot, Actions action, bool alternate) { return getXInputButtonSpriteId(slot, action, alternate); }
int Input::getXboxOneButtonSpriteId(unsigned slot, Actions action, bool alternate) { return getXInputButtonSpriteId(slot, action, alternate); }
int Input::getPs4ButtonSpriteId(unsigned slot, Actions action, bool alternate) {
    const auto &bind = (alternate ? controllers[slot].alternateJoystickBindings : controllers[slot].joystickBindings)[action];
    if (!bind.joyUsingAxis) {
        static const int frames[] = {2,0,1,3,8,9,10,11,6,5,12,13,7,4};
        return bind.joyButton < std::size(frames) ? frames[bind.joyButton] : -1;
    }
    switch (bind.joyAxis) {
    case sf::Joystick::X: return bind.joyAxisPositive ? 23 : 20;
    case sf::Joystick::Y: return 21 + int(!bind.joyAxisPositive);
    case sf::Joystick::Z: return bind.joyAxisPositive ? 28 : 25;
    case sf::Joystick::R: return 26 + int(!bind.joyAxisPositive);
    case sf::Joystick::PovX: return bind.joyAxisPositive ? 18 : 15;
    case sf::Joystick::PovY: return 16 + int(bind.joyAxisPositive);
    default: return -1;
    }
}
int Input::getKeySpriteId(unsigned slot, Actions action) { return int(controllers[slot].keyBindings[action].key); }
Sprite *Input::getGenericSprite(unsigned slot, Actions action) {
    const auto &bind = controllers[slot].joystickBindings[action];
    const unsigned frame = bind.joyUsingAxis ? unsigned(!bind.joyAxisPositive) + 12 + unsigned(bind.joyAxis) * 2 : bind.joyButton;
    genericSprite.setFrame(frame); return &genericSprite;
}
Sprite *Input::getControllerButtonSprite(unsigned slot, Actions action, bool alternate) {
    if (!controllerSpritesInitialized) initControllerSprites();
    if (slot >= controllers.size()) {
        VLOG(LOG_ERROR) << "Input::getControllerButtonSprite invalid playerId: " << slot;
        return nullptr;
    }
    int frame;
    switch (controllers[slot].controlsType) {
    case CONTROLS_KEYBOARD:
        frame = getKeySpriteId(slot, action);
        if (frame >= 0) { keyboardSprite.setFrame(frame); return &keyboardSprite; }
        return nullptr;
    case CONTROLS_XBOX_360:
    case CONTROLS_XBOX_ONE:
        frame = getXInputButtonSpriteId(slot, action, alternate);
        if (frame >= 0) { xbox360Sprite.setFrame(frame); return &xbox360Sprite; }
        return nullptr;
    case CONTROLS_PS4:
        frame = getPs4ButtonSpriteId(slot, action, alternate);
        if (frame >= 0) { ps4Sprite.setFrame(frame); return &ps4Sprite; }
        return nullptr;
    case CONTROLS_STEAM: return getSteamSprite(slot, action);
    default: return getGenericSprite(slot, action);
    }
}
