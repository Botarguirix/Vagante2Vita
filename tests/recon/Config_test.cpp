#include "recon_all.h"
#include "localization.h"

#include <cassert>
#include <cstring>

namespace {
void writeIni(const std::string &text) {
    std::filesystem::create_directories("config");
    std::ofstream("config/config.ini") << text;
}

void writeCatalog() {
    // Minimal GNU .mo file: one original and one translated message.
    const std::uint32_t words[] = {0x950412de, 0, 1, 28, 36, 0, 0, 5, 44, 4, 50};
    std::filesystem::create_directories("locales/es/LC_MESSAGES");
    std::ofstream output("locales/es/LC_MESSAGES/es.mo", std::ios::binary);
    output.write(reinterpret_cast<const char *>(words), sizeof(words));
    output.write("Hello\0Hola\0", 11);
}
}

int main() {
    Config defaults;
    std::uint32_t zoom = 0;
    std::memcpy(&zoom, &defaults.viewZoom, sizeof(zoom));
    assert(zoom == 0x3fc5d174);
    assert(defaults.musicVolume == 50.f && defaults.sfxVolume == 50.f);
    assert(defaults.windowSize == sf::Vector2i(1280, 720));
    assert(defaults.particlesEnabled && defaults.useVertexArrays && defaults.sendStatistics);
    assert(!defaults.fullScreen && !defaults.postProcessingEnabled && !defaults.doLanguageChange);
    assert(defaults.language == NUM_LANGUAGES && defaults.gameModes.size() == 1);
    assert(defaults.quickCastEnabled.size() == 4 && defaults.quickSwapEnabled.size() == 4);
    assert(Config::FILE_FORMAT_VERSION == 35 && Config::CONFIG_FILE_NAME == "config.ini");

    writeIni("");
    assert(defaults.loadIni());
    assert(defaults.windowSize == sf::Vector2i(1024, 768));
    assert(defaults.language == LANG_EN && !defaults.doLanguageChange);

    writeCatalog();
    writeIni("[game]\nlanguage=es\nparticlesEnabled=false\n[input]\nsupportDirectInput=1\n"
             "[audio]\nmusicVolume=12.25\nsfxVolume=31.5\n[graphics]\nwindowSizeX=800\nwindowSizeY=600\n"
             "gamma=invalid\nviewZoom=2\nminimapScale=1.25\nverticalSync=0\n"
             "[player1]\nquickCast=true\n[player3]\nquickSwap=1\n");
    Config loaded;
    assert(loaded.loadIni());
    assert(loaded.language == LANG_ES && !loaded.doLanguageChange);
    assert(recon::translate("Hello") == "Hola" && recon::translate("Missing") == "Missing");
    assert(loaded.windowSize == sf::Vector2i(800, 600));
    assert(!loaded.particlesEnabled && loaded.supportDirectInput && EJoystick::updateDirectInput);
    assert(loaded.musicVolume == 12.25f && loaded.sfxVolume == 31.5f);
    assert(loaded.gamma == 1.f && loaded.viewZoom == 2.f && loaded.minimapScale == 1.25f);
    assert(!loaded.verticalSync && loaded.quickCastEnabled[1] && loaded.quickSwapEnabled[3]);
    assert(loaded.saveIni());
    Config roundTrip;
    assert(roundTrip.loadIni());
    assert(roundTrip.language == LANG_ES && roundTrip.musicVolume == loaded.musicVolume);
    assert(roundTrip.quickCastEnabled == loaded.quickCastEnabled);
    assert(roundTrip.quickSwapEnabled == loaded.quickSwapEnabled);
    loaded.setLanguage(LANG_EN);
    assert(loaded.doLanguageChange && recon::translate("Hello") == "Hello");

    writeIni("[graphics]\nwindowSizeX=800\nwindowSizeX=900\n");
    assert(!roundTrip.loadIni());
    writeIni("[graphics\nwindowSizeX=800\n");
    assert(!roundTrip.loadIni());

    {
        estream created("stream-test.bin", false, false);
        assert(created.is_open());
        const int value = 0x12345678;
        created.file.write(reinterpret_cast<const char *>(&value), sizeof(value));
        created.file.write("abc", 3);
        created.file.flush();
        created.file.seekg(0);
        int read = 0;
        created >> read;
        assert(read == value && created.tellg() == 4);
        created.skip(1);
        char tail[2] = {};
        created.readInto(tail, 2);
        assert(tail[0] == 'b' && tail[1] == 'c');
        created.close();
    }
    {
        estream existing("stream-test.bin", true, true);
        assert(existing.is_open());
        existing.close();
        assert(!std::filesystem::exists("stream-test.bin"));
    }
    std::cout << "Config: constructor, INI defaults/round-trip/rejection, localization and estream passed\n";
    return 0;
}
