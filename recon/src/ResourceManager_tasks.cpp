// ResourceManager task dispatch and CPU image/font loading, recovered from
// vagante.exe 1.10.1 (0074ab70, 0074b440, 0074ed50-007503c0, 00751d90).
#include "ResourceManager.h"
#include "ResourceArchive.h"
#include "ResourceBlock.h"
#include "ResourceMutexes.h"
#include "Config.h"
#include "ESoundBuffer.h"
#include "EMusic.h"
#include "RoomDatabase.h"
#include "AttackScriptDatabase.h"
#include "Log.h"
#include "LogLevel.h"

#include <cstdint>
#include <memory>
#include <utility>

#define VLOG(level) if ((level) > Log::reportingLevel()) ; else Log().get(level)

extern AttackScriptDatabase globalAttackScriptDb;
extern PaletteDatabase globalPaletteDb;

namespace {

std::uint32_t taskStringHash(const std::string &text) {
    std::uint32_t hash = 0x811c9dc5u;
    for (const unsigned char byte : text) hash = (hash ^ byte) * 0x1000193u;
    return hash;
}

// Existing image entries are updated in place: users of an ImageResource keep
// their pointer when a texture is reloaded.
void storeImage(std::map<std::string, ResourceManager::ImageResource *> &map,
                const std::string &path,
                std::unique_ptr<ResourceManager::ImageResource> image) {
    const auto found = map.find(path);
    if (found == map.end()) map.emplace(path, image.release());
    else *found->second = *image;
}

}  // namespace

ResourceManager::Task::Task(TaskType taskType, std::string firstPath,
                            std::string secondPath, std::string thirdPath,
                            float firstAttribute, float secondAttribute)
    : type(taskType), path(std::move(firstPath)), path2(std::move(secondPath)),
      path3(std::move(thirdPath)), attribs(firstAttribute),
      attribs2(secondAttribute), attempts(1) {}

// The other constructor found in Ghidra is the compiler-generated copy
// constructor. The PDB header leaves it implicit, as do we.

void ResourceManager::_lock(ResourceMutexes index) { mutexes[index].lock(); }
bool ResourceManager::_try_lock(ResourceMutexes index) { return mutexes[index].try_lock(); }
void ResourceManager::_unlock(ResourceMutexes index) { mutexes[index].unlock(); }

void ResourceManager::queueTask(TaskType type, std::string path,
                                float attribs, float attribs2) {
    queueTask(type, std::move(path), "", "", attribs, attribs2);
}

void ResourceManager::queueTask(TaskType type, std::string path,
                                std::string path2, std::string path3,
                                float attribs, float attribs2) {
    std::lock_guard<std::mutex> lock(mutexes[M_TASKS]);
    switch (type) {
    case TASK_PROCESS_PALETTE_GROUP:
    case TASK_PROCESS_VFONT:
    case TASK_PROCESS_VFONT_MINUS:
    case TASK_PROCESS_IMAGE:
    case TASK_SET_MUSIC_CHILD:
    case TASK_IMAGE_REPEAT:
        processTasks.emplace(type, path, path2, path3, attribs, attribs2);
        break;
    case TASK_LOAD_SOUND:
    case TASK_LOAD_MUSIC:
        sfxTasks.emplace(type, path, path2, path3, attribs, attribs2);
        break;
    default:
        gfxTasks.emplace(type, path, path2, path3, attribs, attribs2);
        gfxTaskChecksum += taskStringHash(path) + taskStringHash(path2) + taskStringHash(path3);
        break;
    }
}

ResourceManager::Task *ResourceManager::getNextTask(bool preferGraphics) {
    std::lock_guard<std::mutex> lock(mutexes[M_TASKS]);
    auto *first = preferGraphics ? &gfxTasks : &sfxTasks;
    auto *second = preferGraphics ? &sfxTasks : &gfxTasks;
    for (auto *queue : {first, second, &processTasks}) {
        if (!queue->empty()) {
            auto task = std::make_unique<Task>(queue->front());
            queue->pop();
            return task.release();
        }
    }
    return nullptr;
}

bool ResourceManager::requeueTask(Task *task) {
    if (task->attempts < 5) {
        Task retry(task->type, task->path, task->path2, task->path3,
                   task->attribs, task->attribs2);
        retry.attempts = task->attempts + 1;
        std::lock_guard<std::mutex> lock(mutexes[M_TASKS]);
        processTasks.push(retry);
        return true;
    }
    VLOG(LOG_ERROR) << "ResourceManager::requeueTask Failed to execute task: "
                    << task->type << ", " << task->path << ", " << task->path2
                    << ", " << task->attribs << ", " << task->attribs2
                    << ". Attempted " << 5 << " times.";
    return false;
}

bool ResourceManager::loadImage(std::string path, bool smooth,
                                bool makeMask, bool makeOutline) {
    if (archive && !archive->isLoaded()) {
        std::lock_guard<std::mutex> lock(mutexes[M_ARCHIVE]);
        archive->addResourceLocationDescriptor(path, ResourceArchive::RESOURCETYPE_TEXTURE);
    }
    if (createArchiveOnly) return true;

    auto resource = std::make_unique<ImageResource>();
    if (archive && archive->isLoaded() && config && config->loadFromArchive) {
        ResourceBlock *block = archive->getMemoryBlock(gfxPath + path);
        if (!block) return false;
        if (!resource->image.loadFromMemory(block->getData(), block->getSize())) {
            VLOG(LOG_ERROR) << "ResourceManager::loadImage Error loading Image from ResourceBlock: " << path;
            return false;
        }
    } else if (!resource->image.loadFromFile(gfxPath + path)) {
        VLOG(LOG_ERROR) << "Error loading Texture from FileSystem: " << path;
        return false;
    }
    resource->smooth = smooth;
    resource->repeat = false;

    std::unique_ptr<ImageResource> outline;
    std::unique_ptr<ImageResource> mask;
    const sf::Vector2u size = resource->image.getSize();
    if (makeOutline) {
        outline = std::make_unique<ImageResource>(*resource);
        for (unsigned int x = 0; x < size.x; ++x) {
            for (unsigned int y = 0; y < size.y; ++y) {
                sf::Color color = resource->image.getPixel(x, y);
                if (color.a && color.r == 0 && color.g == 0 && color.b == 0) {
                    color.r = color.g = color.b = 255;
                    outline->image.setPixel(x, y, color);
                } else outline->image.setPixel(x, y, sf::Color::Transparent);
            }
        }
    }
    if (makeMask) {
        mask = std::make_unique<ImageResource>(*resource);
        for (unsigned int x = 0; x < size.x; ++x) {
            for (unsigned int y = 0; y < size.y; ++y) {
                sf::Color color = resource->image.getPixel(x, y);
                if (color.a) {
                    color.r = color.g = color.b = 255;
                    mask->image.setPixel(x, y, color);
                }
            }
        }
    }
    std::lock_guard<std::mutex> lock(mutexes[M_IMAGES]);
    storeImage(images, path, std::move(resource));
    if (outline) storeImage(imagesMasksOutline, path, std::move(outline));
    if (mask) storeImage(imagesMasks, path, std::move(mask));
    return true;
}

bool ResourceManager::loadFont(std::string path, bool smooth) {
    if (archive && !archive->isLoaded()) {
        std::lock_guard<std::mutex> lock(mutexes[M_ARCHIVE]);
        archive->addResourceLocationDescriptor(path, ResourceArchive::RESOURCETYPE_FONT);
    }
    if (createArchiveOnly) return true;
    {
        std::lock_guard<std::mutex> lock(mutexes[M_FONTS]);
        if (fonts.find(path) != fonts.end()) {
            VLOG(LOG_INFO) << path << " has already been loaded! Ignoring request to load it again...";
            return false;
        }
    }
    auto font = std::make_unique<sf::Font>();
    if (archive && archive->isLoaded() && config && config->loadFromArchive) {
        ResourceBlock *block;
        {
            std::lock_guard<std::mutex> lock(mutexes[M_ARCHIVE]);
            block = archive->getManagedMemoryBlock(fontPath + path);
        }
        // Unlike decoded Images, SFML fonts retain a reference to their source
        // bytes. Managed blocks survive archive->cleanUpMemoryBlocks(false).
        if (!block || !font->loadFromMemory(block->getData(), block->getSize())) {
            VLOG(LOG_ERROR) << "Error loading font: " << path;
            return false;
        }
    } else if (!font->loadFromFile(fontPath + path)) {
        VLOG(LOG_ERROR) << "Error loading font: " << path;
        return false;
    }
    if (config) {
        for (const unsigned int size : {8u, 12u, 16u}) {
            const_cast<sf::Texture &>(font->getTexture(size)).setSmooth(smooth);
        }
    }
    std::lock_guard<std::mutex> lock(mutexes[M_FONTS]);
    const auto result = fonts.emplace(path, font.get());
    if (result.second) font.release();
    return true;
}

sf::Texture *ResourceManager::uploadTexture(
    const char *const path,
    std::map<std::string, ImageResource *>::iterator image,
    std::map<std::string, ImageResource *> *const source,
    std::map<std::string, sf::Texture *> *const destination) {
    (void)source;  // The original retains the CPU image; the iterator supplies it.
    auto texture = std::make_unique<sf::Texture>();
    if (!texture->loadFromImage(image->second->image)) {
        VLOG(LOG_ERROR) << "ResourceManager::uploadTexture: failed to upload image '" << path << "' to the GPU!";
        return nullptr;
    }
    if (image->second->repeat) texture->setRepeated(true);
    if (image->second->smooth) texture->setSmooth(true);
    const auto result = destination->emplace(path, texture.get());
    if (!result.second) {
        VLOG(LOG_ERROR) << "ResourceManager::uploadTexture: texture '" << path << "' was already uploaded!!!";
        return result.first->second;
    }
    return texture.release();
}

void ResourceManager::runTasks(bool preferGraphics) {
    for (std::unique_ptr<Task> task(getNextTask(preferGraphics)); task;
         task.reset(getNextTask(preferGraphics))) {
        bool completed = true;
        switch (task->type) {
        case TASK_LOAD_FONT:
            loadFont(task->path, task->attribs != 0.0f);
            break;
        case TASK_LOAD_IMAGE:
        case TASK_LOAD_IMAGE_AND_OUTLINE_MASK:
        case TASK_LOAD_IMAGE_ALL_MASKS:
            if (!loadImage(task->path, false,
                           task->type != TASK_LOAD_IMAGE_AND_OUTLINE_MASK,
                           task->type != TASK_LOAD_IMAGE)) {
                requeueTask(task.get());
                completed = false;
            }
            break;
        case TASK_PROCESS_IMAGE:
            if (!createArchiveOnly) {
                const ProcessImageJob job{static_cast<int>(task->attribs),
                                          static_cast<int>(task->attribs2), 0, nullptr, nullptr};
                std::lock_guard<std::mutex> lock(mutexes[M_IMAGES_PROCESS_JOBS]);
                processImageJobs.emplace(task->path, job);
            }
            break;
        case TASK_PROCESS_PALETTE_GROUP:
            if (!createArchiveOnly) {
                const auto group = globalPaletteDb.getPaletteGroup(task->path);
                for (const auto &textureName : group.textureNames) {
                    for (const auto &palette : group.palettes) {
                        if (!processPaletteSwap(textureName, group, palette.second)) {
                            requeueTask(task.get());
                            completed = false;
                            break;
                        }
                    }
                    if (!completed) break;
                }
            }
            break;
        case TASK_IMAGE_REPEAT:
            if (!createArchiveOnly) {
                std::lock_guard<std::mutex> imagesLock(mutexes[M_IMAGES]);
                const auto found = images.find(task->path);
                if (found == images.end()) {
                    requeueTask(task.get());
                    completed = false;
                } else {
                    found->second->repeat = true;
                    std::lock_guard<std::mutex> texturesLock(mutexes[M_TEXTURES]);
                    const auto texture = textures.find(task->path);
                    if (texture != textures.end()) texture->second->setRepeated(true);
                }
            }
            break;
        case TASK_LOAD_SOUND:
            loadSound(task->path, task->attribs, task->attribs2);
            break;
        case TASK_LOAD_MUSIC:
            loadMusic(task->path);
            if (!createArchiveOnly && task->attribs != 0.0f) setMusicLoopPoint(task->path, task->attribs);
            break;
        case TASK_SET_MUSIC_CHILD:
            if (!createArchiveOnly) {
                EMusic *parent;
                EMusic *child;
                {
                    std::lock_guard<std::mutex> lock(mutexes[M_MUSIC]);
                    const auto first = music.find(task->path);
                    const auto second = music.find(task->path2);
                    parent = first == music.end() ? nullptr : first->second;
                    child = second == music.end() ? nullptr : second->second;
                }
                if (!parent || !child) {
                    requeueTask(task.get());
                    completed = false;
                } else parent->setChild(child);
            }
            break;
        case TASK_SET_DEFAULT_ATTENUATION:
            if (!createArchiveOnly) {
                std::lock_guard<std::mutex> lock(mutexes[M_SOUNDBUFFERS]);
                const auto found = soundBuffers.find(task->path);
                if (found == soundBuffers.end()) {
                    requeueTask(task.get());
                    completed = false;
                } else found->second->setDefaultAttenuation(task->attribs);
            }
            break;
        case TASK_PROCESS_VFONT:
        case TASK_PROCESS_VFONT_MINUS:
            if (!createArchiveOnly) {
                ImageResource *image;
                {
                    std::lock_guard<std::mutex> lock(mutexes[M_IMAGES]);
                    const auto found = images.find(task->path);
                    image = found == images.end() ? nullptr : found->second;
                }
                if (!image) {
                    requeueTask(task.get());
                    completed = false;
                } else loadVFont(image->image, task->path,
                                 task->type == TASK_PROCESS_VFONT_MINUS ? -1 : 0);
            }
            break;
        case TASK_GENERATE_ERROR_TEXTURE:
            // Original jump-table entry at 0074faf8 points straight to the
            // tasksCompleted increment at 0074fa50.
            break;
        case TASK_LOAD_ROOMS:
            roomDatabase = new RoomDatabase(true, boost::filesystem::path("rooms.json"));
            break;
        case TASK_LOAD_ATTACK_AND_WEAPON_SCRIPTS:
            globalAttackScriptDb.initialize();
            break;
        case TASK_LOAD_TEXTURE_PALETTE_META_SCRIPT:
            {
                std::lock_guard<std::mutex> lock(mutexes[M_DATABASE_PALETTE]);
                globalPaletteDb.initialize();
            }
            for (const auto &name : globalPaletteDb.getPaletteGroupList()) {
                queueTask(TASK_PROCESS_PALETTE_GROUP, name, 0.0f, 1.0f);
            }
            break;
        default:
            VLOG(LOG_ERROR) << "ResourceManager::runTasks Unknown task type: " << task->type;
            sf::sleep(sf::seconds(0.5f));
            break;
        }
        if (completed) ++tasksCompleted;
    }
    ++threadsCompleted;
}
