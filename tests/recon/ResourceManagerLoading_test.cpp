#include "port_lib_types.h"
#define private public
#define protected public
#include "ResourceManager.h"
#include "EMusic.h"
#undef protected
#undef private
#include "recon_all.h"
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <future>
#include <iostream>

int main() {
    const char *source = std::getenv("VAGANTE_TEST_ARCHIVE");
    if (!source) { std::cerr << "Set VAGANTE_TEST_ARCHIVE to the owned data.vra\n"; return 2; }
    const auto root = std::filesystem::current_path() /
        ("run-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(root);
    std::filesystem::current_path(root);
    Log::init(LOG_DEBUG4);
    std::error_code linkError;
    std::filesystem::create_hard_link(source, "data.vra", linkError);
    if (linkError) std::filesystem::copy_file(source, "data.vra");
    Config config;
    config.language = LANG_EN;
    ResourceManager::setConfig(&config);
    PaletteDatabase::PaletteGroup paletteGroup;
    paletteGroup.name = "group";
    paletteGroup.defaultPalette.colors = {sf::Color::Red, sf::Color::Blue};
    PaletteDatabase::Palette palette;
    palette.name = "alternative";
    palette.colors = {sf::Color::Green, sf::Color::White};
    sf::Image original;
    original.create(2, 1, sf::Color(255, 0, 0, 123));
    original.setPixel(1, 0, sf::Color::Transparent);
    auto *input = new ResourceManager::ImageResource();
    input->image = original;
    ResourceManager::images.emplace("fixture", input);
    assert(ResourceManager::processPaletteSwap("fixture", paletteGroup, palette));
    auto *mapped = ResourceManager::imagesPaletteSwaps.at("fixturegroupalternative");
    assert(mapped->image.getPixel(0, 0) == sf::Color(0, 255, 0, 123));
    assert(mapped->image.getPixel(1, 0) == sf::Color::Transparent);
    assert(input->image.getPixel(0, 0) == sf::Color(255, 0, 0, 123));
    auto highlighted = original;
    assert(palette.generatePaletteSwap(highlighted, paletteGroup.defaultPalette, 0));
    assert(highlighted.getPixel(0, 0) == sf::Color(255, 0, 255, 123));
    PaletteDatabase::Palette invalid;
    invalid.colors.resize(3);
    assert(!invalid.generatePaletteSwap(highlighted, paletteGroup.defaultPalette, 0xffffffffu));
    delete input;
    ResourceManager::images.clear();
    delete mapped;
    ResourceManager::imagesPaletteSwaps.clear();
    // A processor must not burn its retry budget while its input is already
    // dequeued by another worker but has not finished loading.
    ResourceManager::queueTask(ResourceManager::TASK_LOAD_IMAGE, "dependency", 0, 1);
    ResourceManager::queueTask(ResourceManager::TASK_IMAGE_REPEAT, "dependency", 0, 1);
    auto primary = std::unique_ptr<ResourceManager::Task>(ResourceManager::getNextTask(true));
    assert(primary && primary->type == ResourceManager::TASK_LOAD_IMAGE);
    std::promise<void> workerEntered;
    auto processor = std::async(std::launch::async, [&] {
        workerEntered.set_value();
        return std::unique_ptr<ResourceManager::Task>(ResourceManager::getNextTask(false));
    });
    workerEntered.get_future().wait();
    assert(processor.wait_for(std::chrono::milliseconds(50)) == std::future_status::timeout);
    primary.reset();
    auto next = std::unique_ptr<ResourceManager::Task>(ResourceManager::getNextTask(true));
    auto other = processor.get();
    assert(bool(next) != bool(other));
    assert((next ? next : other)->type == ResourceManager::TASK_IMAGE_REPEAT);
    ResourceManager::queueLoadTextureTasks();
    ResourceManager::queueLoadSoundTasks();
    unsigned imageTasks = 0, musicTasks = 0, soundTasks = 0;
    for (auto q = ResourceManager::gfxTasks; !q.empty(); q.pop())
        if (q.front().type >= ResourceManager::TASK_LOAD_IMAGE && q.front().type <= ResourceManager::TASK_LOAD_IMAGE_ALL_MASKS) ++imageTasks;
    for (auto q = ResourceManager::sfxTasks; !q.empty(); q.pop()) {
        musicTasks += q.front().type == ResourceManager::TASK_LOAD_MUSIC;
        soundTasks += q.front().type == ResourceManager::TASK_LOAD_SOUND;
    }
    assert(imageTasks == 2395 && musicTasks == 32 && soundTasks == 272);
    assert(ResourceManager::gfxTasks.front().path == "gui/loading.png");
    ResourceManager::cancelInitialization();

    sf::RenderWindow window(sf::VideoMode(640, 240), "ResourceManager: data.vra loading test");
    window.setFramerateLimit(120);
    std::ofstream samples("progress.csv");
    samples << "frame,completed,total,progress\n";
    auto initialization = std::async(std::launch::async, [] { ResourceManager::initialize(true, false, true); });
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::minutes(3);
    bool intermediate = false, textureSeen = false;
    unsigned frame = 0, snapshots = 0;
    while (initialization.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready ||
           !ResourceManager::initThreadsAreDone()) {
        assert(std::chrono::steady_clock::now() < deadline);
        sf::Event event;
        while (window.pollEvent(event)) {}
        // totalTasks is published when initialize returns. Avoid reading its
        // plain int concurrently with initialization in this test harness.
        const bool started = initialization.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready;
        const float progress = started ? ResourceManager::initTaskProgress() : 0.0f;
        intermediate |= progress > 0 && progress < 1;
        samples << frame++ << ',' << ResourceManager::tasksCompleted.load() << ','
                << (started ? ResourceManager::totalTasks : 0) << ',' << progress << '\n';
        window.clear(sf::Color::Black);
        if (const auto *texture = ResourceManager::getNewLoadingTexture()) {
            textureSeen = true;
            sf::Sprite sprite(*texture);
            const auto size = texture->getSize();
            sprite.setTextureRect(sf::IntRect(0, 0, static_cast<int>(size.x * progress), size.y));
            sprite.setPosition(20, 80);
            sprite.setScale(600.0f / size.x, 2);
            sprite.setTextureRect(sf::IntRect(0, 0, size.x, size.y));
            sprite.setColor(sf::Color(255, 255, 255, 50));
            window.draw(sprite);
            sprite.setTextureRect(sf::IntRect(0, 0, static_cast<int>(size.x * progress), size.y));
            sprite.setColor(sf::Color::White);
            window.draw(sprite);
            if (progress > 0 && progress < 1 && snapshots < 3 && progress >= 0.2f * (snapshots + 1)) {
                sf::Texture capture;
                assert(capture.create(640, 240));
                capture.update(window);
                assert(capture.copyToImage().saveToFile("loading-" + std::to_string(++snapshots) + ".png"));
            }
        }
        window.display();
    }
    initialization.get();
    samples << frame << ',' << ResourceManager::tasksCompleted.load() << ',' << ResourceManager::totalTasks << ',' << ResourceManager::initTaskProgress() << '\n';
    samples.flush();
    std::cout << "Completed " << ResourceManager::tasksCompleted.load() << '/' << ResourceManager::totalTasks
              << "; images=" << ResourceManager::images.size() << " bitmap fonts=" << ResourceManager::vFonts.size() << std::endl;
    assert(intermediate && textureSeen);
    assert(ResourceManager::initTaskProgress() == 1.0f);
    assert(ResourceManager::images.size() == 2393);
    assert(ResourceManager::images.at("monster/last_boss/environment/fog-texture.png")->repeat);
    assert(ResourceManager::soundBuffers.size() == 272 && ResourceManager::music.size() == 32);
    assert(ResourceManager::fonts.size() == 2 && ResourceManager::vFonts.size() == 3);
    assert(ResourceManager::getDefaultFont() && ResourceManager::getGuaranteedUnicodeFont());
    for (const auto &entry : ResourceManager::soundBuffers) assert(entry.second->getSampleCount() > 0);
    for (const auto &entry : ResourceManager::music) assert(entry.second->getDuration() > sf::Time::Zero);
    for (const auto &entry : ResourceManager::vFonts) assert(entry.second->loaded && entry.second->characterHeight > 0);
    ResourceManager::finishInitialization(false);
    assert(ResourceManager::isInitialized());
    assert(ResourceManager::getTexture("gui/loading.png", false));
    assert(ResourceManager::getTexture("@tiles-background.png", false));
    assert(ResourceManager::getTextureMask("classes/knight/1/stand.png"));
    assert(ResourceManager::getTextureOutlineMask("classes/knight/1/stand.png"));
    assert(ResourceManager::getVFont("font/font1.png"));
    auto *parent = ResourceManager::getMusicMap().at("tower_theme.ogg");
    auto *child = ResourceManager::getMusicMap().at("tower_theme_drums.ogg");
    assert(parent->hasChild() && child->hasParent());
    assert(std::abs(ResourceManager::music.at("title_theme.ogg")->getLoopPoints().offset.asSeconds() - 46.085f) < 0.001f);
    ResourceManager::setMusicVolume(40);
    ResourceManager::setMusicChildPercent("tower_theme.ogg", 0.25f);
    assert(std::abs(parent->sf::Music::getVolume() - 30) < 0.001f);
    assert(std::abs(child->sf::Music::getVolume() - 10) < 0.001f);
    auto &block = *ResourceManager::archive->getMemoryBlock("sfx/tower_theme.ogg");
    assert(block.isManaged()); // finishInitialization must retain streamed bytes.
    assert(ResourceManager::getGuaranteedUnicodeFont()->getGlyph('A', 12, false).advance > 0);
    const auto name = ResourceManager::soundBufferNames.front();
    ResourceManager::setGlobalSfxVolume(40);
    ResourceManager::setSfxFactor(0.5f);
    _randEngine.seed(123);
    auto reference = _randEngine;
    const float random = static_cast<float>(reference()) / 4294967296.0f;
    auto sound = ResourceManager::playSound(name, false);
    assert(sound && sound->isGlobal() && sound->getVolume() == 20);
    assert(std::abs(sound->getPitch() - ((random - 0.5f) * ResourceManager::soundBuffers.at(name)->sourceRandomPitchModifier + 1)) < 0.00001f);
    assert(_randEngine() == reference());
    assert(!ResourceManager::playSound(name, false)); // one-shot duplicate this frame
    ResourceManager::updateMusicTransition();
    assert(ResourceManager::playSound(name, 10, 20, false));
    ResourceManager::setGlobalSfxVolume(30);
    assert(sound->getVolume() == 15);
    ResourceManager::setSfxFactor(1);
    assert(sound->getVolume() == 30);
    assert(ResourceManager::getIndexOfSound(name) == 0);
    assert(ResourceManager::getIndexOfSound("missing") == 272);
    assert(!ResourceManager::loadSound("missing.wav", 0, 1));
    assert(!ResourceManager::loadMusic("missing.ogg"));
    sound.reset();
    ResourceManager::cleanUp();
    window.close();
    std::cout << "PASS: 2395 image tasks, 2393 images, 272 sounds, 32 music, 2 fonts, 3 bitmap fonts; progress frames=" << frame << " snapshots=" << snapshots << "\n";
}
