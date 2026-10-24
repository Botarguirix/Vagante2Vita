#include "recon_all.h"
#include "recon_lazy.h"
#if defined(__vita__)
#include <malloc.h>
#endif

#include <algorithm>
#include <memory>

#define VLOG(level) if ((level) > Log::reportingLevel()) ; else Log().get(level)

namespace {
template<class Map> void deleteResources(Map &resources) {
    for (auto &entry : resources) delete entry.second;
    resources.clear();
}
}

// initialize @0073c290: the first two arguments arrive in CL/DL in the
// optimized executable; Ghidra's apparent param_1 is the third argument.
void ResourceManager::initialize(bool loadArchive, bool createOnly, bool useThreads) {
    gfxTaskChecksum = 0;
    createArchiveOnly = createOnly;
    if (!archive) archive = new ResourceArchive("data.vra", false);
    if (loadArchive && !createOnly) archive->load();

    queueLoadSoundTasks();
    queueTask(TASK_LOAD_FONT, "Tiny_04b.ttf", 0.0f, 1.0f);
    queueTask(TASK_LOAD_FONT, "NotoSansCJKkr-Regular.otf", 1.0f, 1.0f);
    queueLoadTextureTasks();
    queueTask(TASK_GENERATE_ERROR_TEXTURE, "", 0.0f, 1.0f);
    queueTask(TASK_LOAD_ROOMS, "", 0.0f, 1.0f);
    queueTask(TASK_LOAD_ATTACK_AND_WEAPON_SCRIPTS, "", 0.0f, 1.0f);
    queueTask(TASK_LOAD_TEXTURE_PALETTE_META_SCRIPT, "", 0.0f, 1.0f);
    threadsCompleted.store(0);
    tasksCompleted.store(0);
    if (useThreads) {
        totalTasks = static_cast<int>(gfxTasks.size() + sfxTasks.size() + processTasks.size());
        numThreads = std::max(1, static_cast<int>(std::thread::hardware_concurrency()));
        VLOG(LOG_DEBUG) << "ResourceManager::initialize Starting G+S+P[ "
                        << gfxTasks.size() << " + " << sfxTasks.size() << " + "
                        << processTasks.size() << " ] tasks across " << numThreads << " thread(s).";
        for (int i = 0; i < numThreads; ++i) taskThreads.emplace_back(runTasks, (i & 1) == 0);
    } else {
        runTasks(false);
    }
}

void ResourceManager::finishInitialization(bool incremental) {
#if defined(__vita__)
    VLOG(LOG_INFO) << "Vita heap in use before finishInitialization: " << (mallinfo().uordblks / (1024 * 1024)) << " MB";
#endif
    for (auto &thread : taskThreads) thread.join();
    taskThreads.clear();
    for (auto &entry : processImageJobs) {
        const int remaining = processImage(entry.first, &entry.second, incremental);
        if (remaining < 0) {
            VLOG(LOG_ERROR) << "ResourceManager::finishInitialization: failed to process image \""
                            << entry.first << "\"";
        } else if (incremental && remaining > 0) {
            return;
        }
    }
    processImageJobs.clear();
    if (!getShaderPointer("postprocessing.frag")) {
        VLOG(LOG_ERROR) << "ResourceManager::finishInitialization: Could not load postprocessing.frag";
    }
    if (archive && (!config || config->loadFromArchive) && !archive->isLoaded()) createNewVra();
    std::sort(soundBufferNames.begin(), soundBufferNames.end());
    if (archive) archive->cleanUpMemoryBlocks(false);
    if (cache && !archive->isLoaded()) {
        cache->createCache();
        cache->cleanUpMemoryBlocks(false);
    }
    initialized.store(true);
    VLOG(LOG_INFO) << "ResourceManager::finishInitialization: DONE!";
}

bool ResourceManager::initThreadsAreDone() {
    return numThreads > 0 && threadsCompleted.load() >= numThreads;
}

float ResourceManager::initTaskProgress() {
    return static_cast<float>(tasksCompleted.load()) / static_cast<float>(totalTasks);
}

bool ResourceManager::isInitialized() { return initialized.load(); }
void ResourceManager::setConfig(Config * const value) { config = value; }
Config *ResourceManager::getConfig() { return config; }
bool ResourceManager::isArchiveLoaded() { return archive && archive->isLoaded(); }
char *ResourceManager::getRoomData() { return archive ? archive->roomData : nullptr; }
char *ResourceManager::getAttackData() { return archive ? archive->attackData : nullptr; }
char *ResourceManager::getPaletteData() { return archive ? archive->paletteData : nullptr; }
RoomDatabase *ResourceManager::getBaseRoomDatabase() { return roomDatabase; }

const sf::Texture *ResourceManager::getNewLoadingTexture() {
    std::unique_lock<std::mutex> textureLock(mutexes[M_TEXTURES], std::try_to_lock);
    if (!textureLock) return nullptr;
    const auto texture = textures.find("gui/loading.png");
    if (texture != textures.end()) return texture->second;
    recon::lazy::prepare("gui/loading.png", images, mutexes[M_IMAGES],
                         [](bool s, bool m, bool o) { loadImage("gui/loading.png", s, m, o); });
    std::unique_lock<std::mutex> imageLock(mutexes[M_IMAGES], std::try_to_lock);
    if (!imageLock) return nullptr;
    const auto image = images.find("gui/loading.png");
    return image == images.end() ? nullptr : uploadTexture("gui/loading.png", image, &images, &textures);
}

void ResourceManager::cancelInitialization() {
    VLOG(LOG_DEBUG) << "ResourceManager::cancelInitialization Removing all initialization tasks in task queue...";
    {
        std::lock_guard<std::mutex> lock(mutexes[M_TASKS]);
        while (!gfxTasks.empty()) gfxTasks.pop();
        while (!sfxTasks.empty()) sfxTasks.pop();
        while (!processTasks.empty()) processTasks.pop();
    }
    VLOG(LOG_DEBUG) << "ResourceManager::cancelInitialization Task queues cleared.";
}

void ResourceManager::cleanUp() {
    stopAllMusic();
    for (auto &thread : taskThreads) thread.join();
    taskThreads.clear();
    totalTasks = 0;
    numThreads = 0;
    delete errorTexture;
    errorTexture = nullptr;
    deleteResources(textures);
    deleteResources(processedTextures);
    deleteResources(textureMasks);
    deleteResources(texturePaletteSwaps);
    deleteResources(fonts);
    deleteResources(vFonts);
    for (const auto &sound : playingSounds) sound->stop();
    playingSounds.clear();
    deleteResources(soundBuffers);
    deleteResources(music);
    deleteResources(shaders);
    shaderTypes.clear();
    delete archive;
    archive = nullptr;
    // The original does not reset initialized or own cache/roomDatabase here.
}

const sf::Texture *ResourceManager::getTexture(std::string name, bool texturesAlreadyLocked) {
    const bool lockRequired = !initialized.load();
    if (!name.empty() && name[0] == '@') {
        const auto found = processedTextures.find(name);
        return found == processedTextures.end() ? nullptr : &found->second->getTexture();
    }
    std::unique_lock<std::mutex> textureLock(mutexes[M_TEXTURES], std::defer_lock);
    if (lockRequired && !texturesAlreadyLocked && !textureLock.try_lock()) return nullptr;
    const auto found = textures.find(name);
    if (found != textures.end()) return found->second;
    recon::lazy::prepare(name, images, mutexes[M_IMAGES], [&](bool s, bool m, bool o) { loadImage(name, s, m, o); });
    std::unique_lock<std::mutex> imageLock(mutexes[M_IMAGES], std::defer_lock);
    if (lockRequired && !imageLock.try_lock()) return nullptr;
    const auto image = images.find(name);
    if (image != images.end()) {
        sf::Texture *texture = uploadTexture(name.c_str(), image, &images, &textures);
        if (recon::lazy::enabled()) {
            if (!imageLock.owns_lock()) imageLock.lock();
            recon::lazy::release(name, images);  // the GPU copy is all the game draws from
        }
        return texture;
    }
    if (lockRequired) return nullptr;
    VLOG(LOG_ERROR) << "ResourceManager::getTexture: Unable to get texture: \"" << name << "\"";
    return errorTexture;
}

sf::Shader *ResourceManager::getShaderPointer(std::string name) {
    auto found = shaders.find(name);
    if (found == shaders.end()) {
        loadShader(name, sf::Shader::Fragment); // ECX=2 at 00750c0a.
        found = shaders.find(name);
        if (found == shaders.end()) {
            if (!createArchiveOnly) { VLOG(LOG_ERROR) << "Failed to lazy load shader!"; }
            return nullptr;
        }
    }
    return found->second;
}

bool ResourceManager::loadShader(std::string name, sf::Shader::Type type) {
    if (archive && !archive->isLoaded()) {
        std::lock_guard<std::mutex> lock(mutexes[M_ARCHIVE]);
        archive->addResourceLocationDescriptor(name, ResourceArchive::RESOURCETYPE_SHADER);
    }
    if (createArchiveOnly) return true;
    VLOG(LOG_INFO) << "ResourceManager::loadShader: Loading shader: " << name;
    if (shaders.find(name) != shaders.end()) {
        VLOG(LOG_INFO) << "ResourceManager::loadShader: " << name
                       << " has already been loaded! Ignoring request to load it again...";
        return false;
    }
    auto shader = std::make_unique<sf::Shader>();
    bool ok = true;
    if (archive && archive->isLoaded()) {
        auto *block = archive->getMemoryBlock(shaderPath + name);
        if (!block) {
            VLOG(LOG_ERROR) << "ResourceManager::loadShader: Could not find memory block for shader: " << name;
            ok = false; // Avoid the original's null dereference on a corrupt/missing record.
        } else {
            ok = shader->loadFromMemory(std::string(block->getData(), block->getSize()), type);
        }
    } else if (config) {
        ok = shader->loadFromFile(shaderPath + name, type);
    }
    if (!ok) {
        VLOG(LOG_ERROR) << "ResourceManager::loadShader: Could not load shader: " << name;
        shader.reset();
    }
    if (!shader) {
        VLOG(LOG_ERROR) << "ResourceManager::loadShader: shader is nullptr and will not be added to the database: " << name;
        return false;
    }
    VLOG(LOG_INFO) << "ResourceManager::loadShader: Shader " << name << " successfully loaded!";
    shader->setUniform("texture", sf::Shader::CurrentTexture);
    shaders.emplace(name, shader.release());
    shaderTypes[name] = type;
    return true;
}

std::string ResourceManager::getLanguageString(LanguageType language) {
    static constexpr const char *codes[] = {
        "en", "de", "fr", "es", "ja", "ko", "zh", "ru", "pt", "pl", "tr", "th", "it", "cs", "oth"};
    const auto index = static_cast<unsigned int>(language);
    return index < NUM_LANGUAGES ? codes[index] : "";
}
