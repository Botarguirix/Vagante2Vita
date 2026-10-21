// Config.cpp: PDB contract and Ghidra functions 00576f60..0057a350.
#include "recon_all.h"
#include "localization.h"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <iomanip>
#include <limits>
#include <set>

#define VLOG(level) if ((level) > Log::reportingLevel()) ; else Log().get(level)

namespace {

std::string trimIni(const std::string &value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    return value.substr(first, value.find_last_not_of(" \t\r\n") - first + 1);
}

// The original used Boost.PropertyTree's INI reader. Keep its sections, duplicate
// rejection and optional typed values, without depending on Boost internals.
struct ConfigIni {
    std::map<std::string, std::string> values;
    std::vector<std::string> order;

    void read(const std::filesystem::path &path) {
        std::ifstream input(path);
        if (!input) throw std::runtime_error("cannot open INI file");
        std::string section, line;
        std::set<std::string> sections;
        while (std::getline(input, line)) {
            line = trimIni(line);
            if (line.empty() || line.front() == ';' || line.front() == '#') continue;
            if (line.front() == '[') {
                const auto end = line.find(']');
                if (end == std::string::npos) throw std::runtime_error("unmatched '['");
                section = trimIni(line.substr(1, end - 1));
                if (!sections.insert(section).second) throw std::runtime_error("duplicate section");
                continue;
            }
            const auto equals = line.find('=');
            if (equals == std::string::npos) throw std::runtime_error("missing '='");
            const auto key = trimIni(line.substr(0, equals));
            const auto full = section.empty() ? key : section + '.' + key;
            if (!values.emplace(full, trimIni(line.substr(equals + 1))).second)
                throw std::runtime_error("duplicate key");
        }
    }

    template<class T> T get(const std::string &key, T fallback) const {
        const auto found = values.find(key);
        if (found == values.end()) return fallback;
        std::istringstream input(found->second);
        T result;
        if (!(input >> result)) return fallback;
        input >> std::ws;
        return input.eof() ? result : fallback;
    }

    bool get(const std::string &key, bool fallback) const {
        const auto found = values.find(key);
        if (found == values.end()) return fallback;
        if (found->second == "1" || found->second == "true") return true;
        if (found->second == "0" || found->second == "false") return false;
        return fallback;
    }

    std::string get(const std::string &key, const std::string &fallback) const {
        const auto found = values.find(key);
        return found == values.end() ? fallback : found->second;
    }

    template<class T> void put(const std::string &key, const T &value) {
        std::ostringstream output;
        output << std::setprecision(std::numeric_limits<float>::max_digits10) << value;
        if (values.emplace(key, output.str()).second) order.push_back(key);
        else values[key] = output.str();
    }

    void write(const std::filesystem::path &path) const {
        std::ofstream output(path);
        if (!output) throw std::runtime_error("cannot write INI file");
        std::vector<std::string> sections;
        for (const auto &full : order) {
            const auto dot = full.find('.');
            const auto section = dot == std::string::npos ? std::string() : full.substr(0, dot);
            if (std::find(sections.begin(), sections.end(), section) == sections.end()) sections.push_back(section);
        }
        for (const auto &section : sections) {
            if (!section.empty()) output << '[' << section << "]\n";
            for (const auto &full : order) {
                const auto dot = full.find('.');
                const auto keySection = dot == std::string::npos ? std::string() : full.substr(0, dot);
                if (keySection != section) continue;
                output << (dot == std::string::npos ? full : full.substr(dot + 1))
                       << '=' << values.at(full) << '\n';
            }
        }
        if (!output) throw std::runtime_error("INI write failed");
    }
};

template<class T> void legacyRead(estream &input, T &value) {
    input.file.read(reinterpret_cast<char *>(&value), sizeof(value));
}

} // namespace

Config::Config()
    : musicVolume(50.f), sfxVolume(50.f), viewZoom(1.5454545f),
      particlesEnabled(true), useVertexArrays(true), sendStatistics(true), loadFromArchive(true),
      fullScreen(false), enableMenuToolTips(true), enableInGameTooltips(true), verticalSync(true),
      cumulativeCombatText(false), steamBigPicture(false), windowSize(1280, 720),
      doResized(false), postProcessingEnabled(false), doLanguageChange(false), usingUgcLocalization(false),
      gamma(1.f), debugLevelGen(false), drawNetInformation(true), forcePerfectPixelScaling(false),
      firstTimePlaying(false), supportDirectInput(false), minimapScale(1.f), language(NUM_LANGUAGES),
      gameModes(1, false), quickCastEnabled(4, false), quickSwapEnabled(4, false) {}

Config::~Config() = default;

bool Config::loadUpgrade(int version, int, Input &input, estream &stream) {
    if (version != 34) return false;
    legacyRead(stream, musicVolume);
    legacyRead(stream, sfxVolume);
    legacyRead(stream, particlesEnabled);
    legacyRead(stream, loadFromArchive);
    legacyRead(stream, fullScreen);
    legacyRead(stream, windowSize.x);
    legacyRead(stream, windowSize.y);
    legacyRead(stream, viewZoom);
    legacyRead(stream, gamma);
    legacyRead(stream, postProcessingEnabled);
    legacyRead(stream, useVertexArrays);
    legacyRead(stream, enableMenuToolTips);
    legacyRead(stream, verticalSync);
    legacyRead(stream, enableInGameTooltips);
    legacyRead(stream, cumulativeCombatText);
    legacyRead(stream, drawNetInformation);
    legacyRead(stream, forcePerfectPixelScaling);
    legacyRead(stream, sendStatistics);
    legacyRead(stream, supportDirectInput);
    EJoystick::updateDirectInput = supportDirectInput;
    int loadedLanguage = 0;
    legacyRead(stream, loadedLanguage);
    setLanguage(static_cast<LanguageType>(loadedLanguage));
    input.loadLegacyFormat(stream);
    ResourceManager::setMusicVolume(musicVolume);
    ResourceManager::setGlobalSfxVolume(sfxVolume);
    return true;
}

bool Config::load(Input &input) {
    const auto path = std::filesystem::path(CONFIG_DIR_PATH) / CONFIG_FILE_NAME;
    if (!std::filesystem::exists(path)) {
        VLOG(LOG_INFO) << "Config::load" << CONFIG_DIR_PATH << '/' << CONFIG_FILE_NAME
                       << " not found, checking for legacy config file...";
        estream legacy(LEGACY_CONFIG_FILE_NAME, true, true);
        if (!legacy.is_open()) {
            VLOG(LOG_INFO) << "Config::load Legacy config file '" << LEGACY_CONFIG_FILE_NAME << "' not found.";
        } else {
            int version = 0;
            legacyRead(legacy, version);
            bool upgraded = false;
            if (version == FILE_FORMAT_VERSION) {
                VLOG(LOG_ERROR) << "Config::load Legacy config file version impossibly matches current version.";
            } else if (loadUpgrade(version, FILE_FORMAT_VERSION, input, legacy)) {
                VLOG(LOG_INFO) << "Config::load Successfully loaded legacy configuration file.";
                upgraded = true;
            } else {
                VLOG(LOG_INFO) << "Config::load Could not load legacy configuration file with version: " << version;
            }
            legacy.close();
            if (upgraded) return true;
        }
    }
    const bool loaded = loadIni();
    if (!loaded) {
        VLOG(LOG_INFO) << " Could not open " << CONFIG_FILE_NAME << ". Using defaults...";
        setLanguage(LANG_EN);
    }
    return loaded;
}

bool Config::save() { return saveIni(); }

bool Config::loadIni() {
    try {
        std::filesystem::create_directories(CONFIG_DIR_PATH);
        if (!std::filesystem::is_directory(CONFIG_DIR_PATH)) {
            VLOG(LOG_ERROR) << "Config::loadIni Unable to create path: " << CONFIG_DIR_PATH;
            return false;
        }
        const auto path = std::filesystem::path(CONFIG_DIR_PATH) / CONFIG_FILE_NAME;
        if (!std::filesystem::is_regular_file(path)) {
            VLOG(LOG_ERROR) << "Config::loadIni Not a regular file: " << std::quoted(path.string());
            return false;
        }
        ConfigIni ini;
        ini.read(path);
        enableMenuToolTips = ini.get("game.enableMenuToolTips", true);
        enableInGameTooltips = ini.get("game.enableInGameTooltips", true);
        drawNetInformation = ini.get("game.drawNetInformation", true);
        cumulativeCombatText = ini.get("game.cumulativeCombatText", false);
        particlesEnabled = ini.get("game.particlesEnabled", true);
        const auto code = ini.get("game.language", std::string("en"));
        LanguageType selected = LANG_EN;
        bool known = false;
        for (unsigned int index = 0; index < NUM_LANGUAGES; ++index) {
            const auto candidate = static_cast<LanguageType>(index);
            if (ResourceManager::getLanguageString(candidate) == code) {
                selected = candidate;
                known = true;
                break;
            }
        }
        if (!known) { VLOG(LOG_ERROR) << "Unknown language: " << code << ". Defaulting to english"; }
        setLanguage(selected);
        supportDirectInput = ini.get("input.supportDirectInput", false);
        musicVolume = ini.get("audio.musicVolume", 50.f);
        sfxVolume = ini.get("audio.sfxVolume", 50.f);
        loadFromArchive = ini.get("developer.loadFromArchive", true);
        fullScreen = ini.get("graphics.fullScreen", false);
        windowSize.x = ini.get("graphics.windowSizeX", 1024);
        windowSize.y = ini.get("graphics.windowSizeY", 768);
        verticalSync = ini.get("graphics.verticalSync", true);
        viewZoom = ini.get("graphics.viewZoom", 1.5454545f);
        gamma = ini.get("graphics.gamma", 1.f);
        postProcessingEnabled = ini.get("graphics.postProcessingEnabled", false);
        useVertexArrays = ini.get("graphics.useVertexArrays", true);
        forcePerfectPixelScaling = ini.get("graphics.forcePerfectPixelScaling", false);
        minimapScale = ini.get("graphics.minimapScale", 1.f);
        gameModes[0] = ini.get("gamemode.hardmode", false) && Stats::isUnlocked("hard_mode");
        for (std::size_t index = 0; index < quickCastEnabled.size(); ++index)
            quickCastEnabled[index] = ini.get("player" + std::to_string(index) + ".quickCast", false);
        for (std::size_t index = 0; index < quickSwapEnabled.size(); ++index)
            quickSwapEnabled[index] = ini.get("player" + std::to_string(index) + ".quickSwap", false);
        ResourceManager::setMusicVolume(musicVolume);
        ResourceManager::setGlobalSfxVolume(sfxVolume);
        EJoystick::updateDirectInput = supportDirectInput;
        return true;
    } catch (const std::exception &error) {
        VLOG(LOG_ERROR) << "Config::loadIni " << error.what();
        return false;
    }
}

bool Config::saveIni() {
    try {
        std::filesystem::create_directories(CONFIG_DIR_PATH);
        if (!std::filesystem::is_directory(CONFIG_DIR_PATH)) {
            VLOG(LOG_ERROR) << "Config::saveIni Unable to create directory: " << CONFIG_DIR_PATH;
            return false;
        }
        ConfigIni ini;
        ini.put("game.enableMenuToolTips", enableMenuToolTips);
        ini.put("game.enableInGameTooltips", enableInGameTooltips);
        ini.put("game.drawNetInformation", drawNetInformation);
        ini.put("game.cumulativeCombatText", cumulativeCombatText);
        ini.put("game.particlesEnabled", particlesEnabled);
        if (!usingUgcLocalization) ini.put("game.language", ResourceManager::getLanguageString(language));
        ini.put("input.supportDirectInput", supportDirectInput);
        ini.put("audio.musicVolume", musicVolume);
        ini.put("audio.sfxVolume", sfxVolume);
        ini.put("developer.loadFromArchive", loadFromArchive);
        ini.put("graphics.fullScreen", fullScreen);
        ini.put("graphics.windowSizeX", windowSize.x);
        ini.put("graphics.windowSizeY", windowSize.y);
        ini.put("graphics.verticalSync", verticalSync);
        ini.put("graphics.viewZoom", viewZoom);
        ini.put("graphics.gamma", gamma);
        ini.put("graphics.postProcessingEnabled", postProcessingEnabled);
        ini.put("graphics.useVertexArrays", useVertexArrays);
        ini.put("graphics.forcePerfectPixelScaling", forcePerfectPixelScaling);
        ini.put("graphics.minimapScale", minimapScale);
        ini.put("gamemode.hardmode", bool(gameModes[0]));
        for (std::size_t index = 0; index < quickCastEnabled.size(); ++index)
            ini.put("player" + std::to_string(index) + ".quickCast", bool(quickCastEnabled[index]));
        for (std::size_t index = 0; index < quickSwapEnabled.size(); ++index)
            ini.put("player" + std::to_string(index) + ".quickSwap", bool(quickSwapEnabled[index]));
        const auto path = std::filesystem::path(CONFIG_DIR_PATH) / CONFIG_FILE_NAME;
        ini.write(path);
        VLOG(LOG_DEBUG) << "Config::saveIni Successfully saved " << path.string();
        return true;
    } catch (const std::exception &error) {
        VLOG(LOG_ERROR) << "Config::saveIni " << error.what();
        return false;
    }
}

void Config::updateGamma() {
    if (!postProcessingEnabled) return;
    if (auto shader = ResourceManager::getShaderPointer("postprocessing.frag")) {
        shader->setUniform("gamma", gamma);
    } else {
        VLOG(LOG_ERROR) << "Config::updateGamma: postprocessing shader is null, cannot set gamma!";
    }
}

void Config::updateWindowSize(sf::RenderWindow &window) {
    const auto desktop = sf::VideoMode::getDesktopMode();
    VLOG(LOG_INFO) << "Current Desktop VideoMode: (" << desktop.width << ", " << desktop.height
                   << ", " << desktop.bitsPerPixel << ')';
    if (windowSize.x > static_cast<int>(desktop.width)) windowSize.x = desktop.width;
    if (windowSize.y > static_cast<int>(desktop.height)) windowSize.y = desktop.height;
    auto mode = sf::VideoMode(windowSize.x, windowSize.y, 32);
    auto style = sf::Style::Default;
    if (fullScreen) {
        mode = sf::VideoMode::getDesktopMode();
        style = sf::Style::None; // Original fullscreen is a borderless desktop window (style 0).
        VLOG(LOG_DEBUG) << "Config::updateWindowSize VideoMode is: " << mode.width << ", "
                        << mode.height << ", " << mode.bitsPerPixel;
        VLOG(LOG_DEBUG) << "Config::updateWindowSize: windowName=Vagante windowStyle=0x" << std::hex << style;
    }
    window.create(mode, "Vagante", style, sf::ContextSettings(0, 0, 0, 1, 1));
    // The original's embedded sfml_icon is commercial data, not part of the port.
    // A user's external icon.png supplies the same SFML operation when present.
    if (std::filesystem::is_regular_file("icon.png")) {
        sf::Image icon;
        if (icon.loadFromFile("icon.png")) window.setIcon(icon.getSize().x, icon.getSize().y, icon.getPixelsPtr());
    }
    window.setKeyRepeatEnabled(false);
    window.setVerticalSyncEnabled(verticalSync);
}

void Config::setLanguage(LanguageType selected) {
    if (language == selected) return;
    if (language != NUM_LANGUAGES) doLanguageChange = true;
    language = selected;
    VLOG(LOG_DEBUG) << "Config::setLanguage Set language to "
                    << ResourceManager::getLanguageString(language) << " - " << language;
    if (selected < LANG_OTHER)
        recon::installMessageLocale("./locales/", ResourceManager::getLanguageString(selected));
}

void Config::setWorkshopLocalization(boost::filesystem::path, std::string) {
    // Steam Workshop is intentionally omitted in this offline port.
}

estream::estream(std::string filePath, bool existingOnly, bool removeAfter)
    : file(filePath, std::ios::in | std::ios::out | std::ios::binary),
      path(std::move(filePath)), deleteAfter(removeAfter) {
    if (file || existingOnly) return;
    file.clear();
    file.open(path, std::ios::out | std::ios::binary);
    if (file) {
        if (!removeAfter) { VLOG(LOG_INFO) << "Successfully created file:" << path; }
    } else {
        VLOG(LOG_ERROR) << std::strerror(errno);
        VLOG(LOG_ERROR) << "Could not create file:" << path;
    }
    file.close();
    file.clear();
    file.open(path, std::ios::in | std::ios::out | std::ios::binary);
}

void estream::close() {
    file.close();
    if (deleteAfter) std::remove(path.c_str());
}

bool estream::is_open() { return file.is_open(); }
void estream::skip(unsigned int count) { file.seekg(count, std::ios::cur); }
long long estream::tellg() { return static_cast<long long>(file.tellg()); }
void estream::readInto(char *destination, unsigned int count) { file.read(destination, count); }

estream &operator>>(estream &input, int &value) {
    input.file.read(reinterpret_cast<char *>(&value), sizeof(value));
    return input;
}
