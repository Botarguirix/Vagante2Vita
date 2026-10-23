// Audio resource loading and streaming dependencies recovered from vagante.exe.
#include "recon_all.h"
#include <algorithm>
#include <memory>
#define VLOG(level) if ((level) > Log::reportingLevel()) ; else Log().get(level)

ESoundBuffer::ESoundBuffer()
    : sourceRandomPitchModifier(0), sourceVolumeModifier(1),
      sourceDefaultAttenuation(0), hasCustomDefaultAttenuation(false),
      filename("UNDEFINED"), playedThisFrame(false) {}
ESoundBuffer::~ESoundBuffer() = default;
void ESoundBuffer::setDefaultAttenuation(float value) {
    sourceDefaultAttenuation = value;
    hasCustomDefaultAttenuation = true;
}

// sf::Music streams its source rather than copying it. Archive blocks used here
// remain managed until cleanUp destroys music and then the archive.
bool ResourceManager::loadSound(std::string name, float pitch, float volume) {
    if (archive && !archive->isLoaded()) {
        std::lock_guard<std::mutex> lock(mutexes[M_ARCHIVE]);
        archive->addResourceLocationDescriptor(name, ResourceArchive::RESOURCETYPE_SOUNDBUFFER);
    }
    if (createArchiveOnly) return true;
    auto sound = std::make_unique<ESoundBuffer>();
    sound->filename = name;
    bool ok;
    if (archive && archive->isLoaded() && config && config->loadFromArchive) {
        auto *block = archive->getMemoryBlock(sfxPath + name);
        ok = block && sound->loadFromMemory(block->getData(), block->getSize());
    } else ok = sound->loadFromFile(sfxPath + name);
    if (!ok) { VLOG(LOG_ERROR) << "Error loading sound: " << name; return false; }
    sound->sourceRandomPitchModifier = pitch;
    sound->sourceVolumeModifier = volume;
    std::lock_guard<std::mutex> lock(mutexes[M_SOUNDBUFFERS]);
    if (soundBuffers.count(name)) {
        VLOG(LOG_INFO) << name << " has already been loaded!";
        return false;
    }
    soundBuffers.emplace(name, sound.release());
    soundBufferNames.push_back(name);
    return true;
}

bool ResourceManager::loadMusic(std::string name) {
    if (archive && !archive->isLoaded()) {
        std::lock_guard<std::mutex> lock(mutexes[M_ARCHIVE]);
        archive->addResourceLocationDescriptor(name, ResourceArchive::RESOURCETYPE_MUSIC);
    }
    if (createArchiveOnly) return true;
    {
        std::lock_guard<std::mutex> lock(mutexes[M_MUSIC]);
        if (music.count(name)) return false;
    }
    std::unique_ptr<EMusic> stream;
    {
        std::lock_guard<std::mutex> lock(mutexes[M_SFX]);
        stream = std::make_unique<EMusic>();
        stream->filename = name;
    }
    ResourceBlock *block = nullptr;
    const bool fromArchive = archive && archive->isLoaded() && config && config->loadFromArchive;
    if (fromArchive) block = archive->getManagedMemoryBlock(musicPath + name);
    bool ok;
    {
        std::lock_guard<std::mutex> lock(mutexes[M_SFX]);
        ok = fromArchive ? block && stream->openFromMemory(block->getData(), block->getSize())
                         : stream->openFromFile(musicPath + name);
    }
    if (!ok) { VLOG(LOG_ERROR) << "Error loading music: " << name; return false; }
    stream->setLoop(true);
    stream->setVolume(musicVolume);
    std::lock_guard<std::mutex> lock(mutexes[M_MUSIC]);
    return music.emplace(name, stream.release()).second;
}

EMusic::EMusic()
    : m_bIntroHasBeenPlayed(false), m_nLoopPointSampleLeft(0),
      m_nLoopPointSampleRight(0), m_nCurrentSample(0), child(nullptr),
      parent(nullptr), childPercent(0) {}
EMusic::~EMusic() { sf::Music::stop(); }
unsigned int EMusic::timeToSample(sf::Time time) {
    return static_cast<unsigned int>(time.asSeconds() * static_cast<float>(getSampleRate()) * getChannelCount());
}
sf::Time EMusic::sampleToTime(unsigned int sample) {
    if (!getSampleRate() || !getChannelCount()) return sf::Time::Zero;
    return sf::seconds(static_cast<float>(sample) / getSampleRate() / getChannelCount());
}
bool EMusic::openFromFile(const std::string &path) {
    if (!sf::Music::openFromFile(path)) return false;
    m_nLoopPointSampleLeft = timeToSample(sf::Time::Zero);
    m_nLoopPointSampleRight = timeToSample(getDuration());
    return true;
}
bool EMusic::openFromMemory(const void *data, unsigned int size) {
    if (!sf::Music::openFromMemory(data, size)) return false;
    m_nLoopPointSampleLeft = timeToSample(sf::Time::Zero);
    m_nLoopPointSampleRight = timeToSample(getDuration());
    return true;
}
void EMusic::setLoopPoint(sf::Time time) {
    m_nLoopPointSampleLeft = timeToSample(time);
    // SFML 2.5 Music::onLoop seeks its file directly, bypassing onSeek (the
    // original SFML fork called onSeek). Configure the same loop in its API.
    sf::Music::setLoopPoints(sf::Music::TimeSpan(time, getDuration() - time));
}
bool EMusic::onGetData(sf::SoundStream::Chunk &chunk) {
    if (sf::Music::onGetData(chunk)) {
        const auto next = m_nCurrentSample + chunk.sampleCount;
        if (next <= m_nLoopPointSampleRight) {
            m_nCurrentSample = static_cast<unsigned int>(next);
            return true;
        }
        chunk.sampleCount = m_nLoopPointSampleRight - m_nCurrentSample;
        while (chunk.sampleCount % getChannelCount()) ++chunk.sampleCount;
        m_nCurrentSample = m_nLoopPointSampleLeft;
    } else if (getLoop()) {
        // Keep the original counter synchronized with SFML 2.5's direct seek.
        m_nCurrentSample = m_nLoopPointSampleLeft;
    }
    return false;
}
void EMusic::onSeek(sf::Time time) {
    if (!m_bIntroHasBeenPlayed) {
        sf::Music::onSeek(time);
        m_bIntroHasBeenPlayed = true;
    } else {
        m_nCurrentSample = m_nLoopPointSampleLeft + timeToSample(time);
        sf::Music::onSeek(sampleToTime(m_nCurrentSample));
    }
    if (child && child->getPlayingOffset() != getPlayingOffset())
        child->setPlayingOffset(getPlayingOffset());
}
void EMusic::play() {
    if (getStatus() == sf::SoundSource::Paused || getStatus() == sf::SoundSource::Stopped) {
        if (getStatus() == sf::SoundSource::Stopped) {
            m_bIntroHasBeenPlayed = false;
            m_nCurrentSample = 0;
        }
        sf::Music::play();
    }
    if (child) {
        child->play();
        if (child->getPlayingOffset() != getPlayingOffset()) child->setPlayingOffset(getPlayingOffset());
    }
}
void EMusic::stop() { sf::Music::stop(); if (child) child->stop(); }
void EMusic::pause() { sf::Music::pause(); if (child) child->pause(); }
bool EMusic::hasChild() { return child != nullptr; }
bool EMusic::hasParent() { return parent != nullptr; }
float EMusic::getChildPercent() { return childPercent; }
void EMusic::setChild(EMusic *value) { child = value; if (child) child->parent = this; }
float EMusic::getVolume() const {
    return sf::Music::getVolume() + (child ? child->getVolume() : 0.0f);
}
void EMusic::setChildPercent(float value) {
    const float volume = getVolume();
    childPercent = std::clamp(value, 0.0f, 1.0f);
    setVolume(volume);
}
void EMusic::setVolume(float value) {
    if (child) {
        sf::Music::setVolume((1.0f - childPercent) * value);
        child->setVolume(value * childPercent);
    } else sf::Music::setVolume(value);
}

EMusic *ResourceManager::getMusicPointer(const std::string &name, bool lockRequired) {
    std::unique_lock<std::mutex> lock(mutexes[M_MUSIC], std::defer_lock);
    if (lockRequired && !lock.try_lock()) return nullptr;
    const auto it = music.find(name);
    return it == music.end() ? nullptr : it->second;
}
void ResourceManager::setMusicLoopPoint(std::string name, float seconds) {
    if (auto *stream = getMusicPointer(name, !initialized.load())) stream->setLoopPoint(sf::seconds(seconds));
}
std::map<std::string, EMusic *> &ResourceManager::getMusicMap() { return music; }
void ResourceManager::stopAllMusic() { for (auto &entry : music) entry.second->stop(); }
void ResourceManager::setMusicVolume(float value) {
    if (value == musicVolume) return;
    musicVolume = value;
    for (auto &entry : music) if (!entry.second->hasParent()) entry.second->setVolume(value);
}
void ResourceManager::setGlobalSfxVolume(float value) {
    sfxVolume = value;
    for (auto &sound : playingSounds) sound->setVolume(sfxVolumeFactor * sfxVolume);
}
void ResourceManager::setSfxFactor(float value) {
    sfxVolumeFactor = value;
    for (auto &sound : playingSounds) sound->setVolume(sfxVolume * sfxVolumeFactor);
}

void ResourceManager::playMusic(const std::string &name, bool transition, bool restart) {
    auto *stream = getMusicPointer(name, !initialized.load());
    if (!stream) {
        VLOG(initialized.load() ? LOG_ERROR : LOG_WARNING) << "ResourceManager::playMusic Music " << name << " is not loaded.";
        return;
    }
    if (transition) { targetMusic = stream; targetRestart = restart; }
    else {
        currMusic = stream;
        if (restart) stream->stop();
        currMusic->setVolume(musicVolume);
        currMusic->play();
        currMusicFade = 1;
    }
    fadingOutMusic = false;
}
void ResourceManager::setSpecificMusicVolume(const std::string &name, float value) {
    if (auto *stream = getMusicPointer(name, !initialized.load())) stream->setVolume(value);
    else { VLOG(initialized.load() ? LOG_ERROR : LOG_WARNING) << "Music " << name << " is not loaded."; }
}
void ResourceManager::setMusicChildPercent(const std::string &name, float value) {
    if (auto *stream = getMusicPointer(name, !initialized.load())) stream->setChildPercent(value);
    else { VLOG(initialized.load() ? LOG_ERROR : LOG_WARNING) << "Music " << name << " is not loaded."; }
}
void ResourceManager::updateMusicTransition() {
    for (auto &entry : soundBuffers) entry.second->playedThisFrame = false;
    for (auto &sound : playingSounds) sound->updatePanAndVolume();
    if (!currMusic) {
        if (targetMusic) { currMusic = targetMusic; targetMusic = nullptr; currMusic->play(); }
    } else if (fadingOutMusic) {
        if (currMusicFade > 0) {
            currMusicFade = std::max(0.0f, currMusicFade - 0.04f);
            currMusic->setVolume(musicVolume * currMusicFade);
            if (currMusicFade == 0) { currMusic->pause(); currMusic = nullptr; }
            if (altMusic) altMusic->setVolume(musicVolume * currMusicFade);
        }
    } else if (targetMusic) {
        if (currMusicFade <= 0) {
            currMusic->pause();
            currMusic = targetMusic;
            if (targetRestart) {
                targetRestart = false;
                currMusicFade = 1;
                currMusic->stop();
                currMusic->setVolume(musicVolume * currMusicFade);
            }
            currMusic->play();
            targetMusic = nullptr;
        } else {
            currMusicFade = std::max(0.0f, currMusicFade - 0.04f);
            currMusic->setVolume(musicVolume * currMusicFade);
        }
    } else if (currMusicFade < 1) {
        currMusicFade = std::min(1.0f, currMusicFade + 0.04f);
        currMusic->setVolume(musicVolume * currMusicFade);
    }
}
