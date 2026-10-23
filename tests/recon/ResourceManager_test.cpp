#include "port_lib_types.h"
// Inspect the reconstructed state without modifying the generated PDB contract.
#define private public
#include "ResourceManager.h"
#undef private
#include "recon_all.h"

#include <cassert>
#include <cstdlib>
#include <future>
#include <zlib.h>

namespace {
void integer(std::string &out, unsigned value, int bytes = 4) {
    for (int i = 0; i < bytes; ++i) out.push_back(char(value >> (8 * i)));
}
void record(std::string &out, const std::string &name, const std::string &data) {
    integer(out, name.size()); out += name;
    integer(out, data.size()); out += data;
}
void writeTestArchive() {
    sf::Image pixels;
    pixels.create(2, 2, sf::Color::Red);
    pixels.setPixel(1, 0, sf::Color::Green);
    pixels.setPixel(0, 1, sf::Color::Blue);
    pixels.setPixel(1, 1, sf::Color::White);
    assert(pixels.saveToFile("loading-test.png"));
    std::ifstream image("loading-test.png", std::ios::binary);
    const std::string png((std::istreambuf_iterator<char>(image)), {});
    std::string data;
    integer(data, 8); integer(data, 1, 2);
    record(data, "gfx/gui/loading.png", png);
    record(data, "rooms.json", "{}");
    record(data, "palettes.json", "{}");
    record(data, "attacks.json", "{}");
    std::string compressed(compressBound(data.size()), '\0');
    uLongf size = compressed.size();
    assert(compress2(reinterpret_cast<Bytef *>(compressed.data()), &size,
                     reinterpret_cast<const Bytef *>(data.data()), data.size(), Z_DEFAULT_COMPRESSION) == Z_OK);
    std::ofstream("data.vra", std::ios::binary).write(compressed.data(), size);
}

void releaseRetainedImages() {
    // cleanUp intentionally leaves these CPU images in the original. The fixture
    // releases them separately so repeated independent cases don't leak memory.
    for (auto *map : {&ResourceManager::images, &ResourceManager::imagesMasks,
                      &ResourceManager::imagesMasksOutline, &ResourceManager::imagesPaletteSwaps}) {
        for (auto &entry : *map) delete entry.second;
        map->clear();
    }
}

void testLoadingTextureAndAtlas() {
    Config config;
    ResourceManager::setConfig(&config);
    writeTestArchive();
    ResourceManager::archive = new ResourceArchive("data.vra", false);
    ResourceManager::archive->load();
    assert(ResourceManager::isArchiveLoaded());
    assert(ResourceManager::getRoomData() && ResourceManager::getAttackData() && ResourceManager::getPaletteData());
    assert(!ResourceManager::getNewLoadingTexture());
    assert(ResourceManager::loadImage("gui/loading.png", false, false, false));
    const auto *texture = ResourceManager::getNewLoadingTexture();
    assert(texture && texture->getSize() == sf::Vector2u(2, 2));
    assert(ResourceManager::getNewLoadingTexture() == texture);
    assert(texture->copyToImage().getPixel(1, 0) == sf::Color::Green);

    ResourceManager::mutexes[M_TEXTURES].lock();
    auto blocked = std::async(std::launch::async, [] { return ResourceManager::getNewLoadingTexture(); });
    assert(!blocked.get()); // Nonblocking lookup while a worker owns the map.
    ResourceManager::mutexes[M_TEXTURES].unlock();

    ResourceManager::ProcessImageJob tile{2, 2, 0, texture, nullptr};
    assert(ResourceManager::processImage("gui/loading.png", &tile, false) == 0);
    const auto border = tile.postTextureCached->getTexture().copyToImage();
    assert(border.getSize() == sf::Vector2u(4, 4));
    for (unsigned y = 0; y < 4; ++y) for (unsigned x = 0; x < 4; ++x) {
        const auto original = texture->copyToImage();
        assert(border.getPixel(x, y) == original.getPixel(x < 2 ? 0 : 1, y < 2 ? 0 : 1));
    }
    assert(ResourceManager::getTexture("@gui/loading.png", false) == &tile.postTextureCached->getTexture());
    ResourceManager::cleanUp();
    releaseRetainedImages();
    ResourceManager::setConfig(nullptr);
    assert(!ResourceManager::isArchiveLoaded() && ResourceManager::textures.empty());
}

void testProgressCancellationAndIncrementalFinish() {
    ResourceManager::tasksCompleted.store(3);
    ResourceManager::totalTasks = 8;
    assert(ResourceManager::initTaskProgress() == 0.375f);
    ResourceManager::numThreads = 2;
    ResourceManager::threadsCompleted.store(1);
    assert(!ResourceManager::initThreadsAreDone());
    ResourceManager::threadsCompleted.store(2);
    assert(ResourceManager::initThreadsAreDone());

    ResourceManager::queueTask(ResourceManager::TASK_LOAD_IMAGE, "cancel-image", 0, 1);
    ResourceManager::queueTask(ResourceManager::TASK_LOAD_SOUND, "cancel-sound", 0, 1);
    ResourceManager::queueTask(ResourceManager::TASK_PROCESS_IMAGE, "cancel-process", 2, 2);
    assert(!ResourceManager::gfxTasks.empty() && !ResourceManager::sfxTasks.empty());
    ResourceManager::cancelInitialization();
    assert(ResourceManager::gfxTasks.empty() && ResourceManager::sfxTasks.empty() && ResourceManager::processTasks.empty());
    assert(ResourceManager::tasksCompleted.load() == 3); // Cancellation does not fake completion.

    writeTestArchive();
    ResourceManager::archive = new ResourceArchive("data.vra", false);
    ResourceManager::archive->load();
    ResourceManager::initialized.store(false);
    sf::Image pixels;
    pixels.create(2, 34, sf::Color::Cyan);
    sf::Texture atlas;
    assert(atlas.loadFromImage(pixels));
    ResourceManager::processImageJobs["incremental.png"] = {2, 2, 0, &atlas, nullptr};
    ResourceManager::finishInitialization(true);
    assert(!ResourceManager::isInitialized());
    assert(ResourceManager::processImageJobs.at("incremental.png").tilesProcessed == 16);
    ResourceManager::finishInitialization(true);
    assert(ResourceManager::isInitialized() && ResourceManager::processImageJobs.empty());
    assert(ResourceManager::getTexture("@incremental.png", false)->getSize() == sf::Vector2u(4, 68));
    ResourceManager::cleanUp();
}

void testInitialize(bool threads) {
    Config config;
    ResourceManager::setConfig(&config);
    assert(ResourceManager::getConfig() == &config);
    ResourceManager::initialized.store(false);
    writeTestArchive();
    // Block 2 now queues the complete game, while this synthetic archive holds
    // only one image. Exercise both scheduling modes in descriptor-only mode;
    // ResourceManagerLoading_test covers complete loading with the owned VRA.
    ResourceManager::initialize(true, true, threads);
    ResourceManager::finishInitialization(false);
    assert(ResourceManager::isInitialized() && !ResourceManager::isArchiveLoaded());
    assert(ResourceManager::taskThreads.empty());
    if (threads) {
        assert(ResourceManager::initThreadsAreDone());
        assert(ResourceManager::initTaskProgress() == 1.f);
    }
    ResourceManager::cleanUp();
    releaseRetainedImages();
    ResourceManager::setConfig(nullptr);
}

void testExternalUserTexture() {
    const char *path = std::getenv("VAGANTE_VRA");
    if (!path || !*path) return;
    Config config;
    ResourceManager::setConfig(&config);
    const auto previous = std::filesystem::current_path();
    std::filesystem::current_path(std::filesystem::absolute(path).parent_path());
    ResourceManager::initialized.store(false);
    ResourceManager::initialize(true, false, false);
    assert(ResourceManager::isArchiveLoaded());
    assert(ResourceManager::loadImage("gui/loading.png", false, false, false));
    const auto *loading = ResourceManager::getNewLoadingTexture();
    assert(loading && loading->getSize().x > 0 && loading->getSize().y > 0);
    std::cout << "External data.vra -> SFML loading texture: " << loading->getSize().x << "x" << loading->getSize().y << '\n';
    ResourceManager::finishInitialization(false);
    assert(ResourceManager::isInitialized());
    assert(ResourceManager::fonts.count("Tiny_04b.ttf") == 1);
    assert(ResourceManager::fonts.count("NotoSansCJKkr-Regular.otf") == 1);
    // Request a new glyph after unmanaged bytes have been freed. FreeType must
    // still have valid backing storage for both archive-loaded fonts.
    for (const auto &font : ResourceManager::fonts) {
        assert(font.second->getGlyph('A', 12, false).advance > 0);
    }
    assert(ResourceManager::shaders.count("postprocessing.frag") == 1);
    ResourceManager::cleanUp();
    releaseRetainedImages();
    ResourceManager::setConfig(nullptr);
    std::filesystem::current_path(previous);
}
}

int main() {
    assert(ResourceManager::mutexes.size() == NUM_MUTEXES);
    testLoadingTextureAndAtlas();
    testProgressCancellationAndIncrementalFinish();
    testInitialize(false);
    testInitialize(true);
    testExternalUserTexture();
    std::cout << "ResourceManager tests passed\n";
}
