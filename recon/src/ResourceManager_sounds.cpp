#include "recon_all.h"
#include <algorithm>
#include <cmath>
#define VLOG(level) if ((level) > Log::reportingLevel()) ; else Log().get(level)

ESound::ESound(bool positional, const ESoundBuffer &buffer)
    : sf::Sound(buffer), e_buffer(buffer), e_global(!positional), e_volume(100),
      e_attenuation(buffer.hasCustomDefaultAttenuation ? buffer.sourceDefaultAttenuation : 1.5f),
      e_specificVolumeFactor(1), e_spatialVolumeFactor(1), e_gamePosition(0, 0) { ++DEBUG_COUNT; }
ESound::~ESound() { --DEBUG_COUNT; }
void ESound::setVolume(float value) {
    e_volume = value;
    sf::Sound::setVolume(std::clamp(e_spatialVolumeFactor * e_specificVolumeFactor * e_volume * e_buffer.sourceVolumeModifier, 0.0f, 100.0f));
}
float ESound::getVolume() { return e_volume; }
void ESound::setSpecificVolume(float value) { e_specificVolumeFactor = value; setVolume(e_volume); }
float ESound::getSpecificVolume() { return e_specificVolumeFactor; }
void ESound::setAttenuation(float value) { e_attenuation = value; }
bool ESound::isGlobal() { return e_global; }
void ESound::setGamePosition(float x, float y) { e_gamePosition = {x, y}; }
void ESound::setGamePosition(sf::Vector2f value) { e_gamePosition = value; }
const sf::Vector2f &ESound::getGamePosition() { return e_gamePosition; }
std::string ESound::debugGetSoundName() { return e_buffer.filename; }
void ESound::updatePanAndVolume() {
    if (e_global || getStatus() == sf::SoundSource::Stopped) return;
    const float dx = e_gamePosition.x - LISTENER.x;
    const float dy = e_gamePosition.y - LISTENER.y;
    const float distance = std::sqrt(dy * dy + dx * dx);
    e_spatialVolumeFactor = distance < 40.0f ? 1.0f : distance > 300.0f ? 0.0f
        : 40.0f / ((distance - 40.0f) * e_attenuation + 40.0f);
    float pan = std::abs(dx) < 64.0f ? 0.0f : std::min(1.0f, (std::abs(dx) - 64.0f) * 0.006666666828095913f);
    if (dx < 0) pan = -pan;
    // The original SFML fork has setPan. Standard SFML 2.5 uses a relative
    // unit-circle source with distance attenuation disabled for this mapping.
    sf::Sound::setAttenuation(0);
    sf::Sound::setPosition(pan, 0, -std::sqrt(std::max(0.0f, 1.0f - pan * pan)));
    sf::Sound::setVolume(std::clamp((e_buffer.sourceVolumeModifier * e_volume) *
                                   (e_specificVolumeFactor * e_spatialVolumeFactor), 0.0f, 100.0f));
}

std::shared_ptr<ESound> ResourceManager::playSound(std::string name, bool loop) {
    return playSoundPriv(std::move(name), false, 0, 0, loop);
}
std::shared_ptr<ESound> ResourceManager::playSound(std::string name, float x, float y, bool loop) {
    return playSoundPriv(std::move(name), true, x, y, loop);
}
std::vector<std::shared_ptr<ESound>> &ResourceManager::debugGetPlayingSounds() { return playingSounds; }
std::string ResourceManager::getSoundNameByIndex(unsigned short index) {
    return index < soundBufferNames.size() ? soundBufferNames[index] : "";
}
unsigned short ResourceManager::getIndexOfSound(std::string name) {
    const auto it = std::find(soundBufferNames.begin(), soundBufferNames.end(), name);
    return static_cast<unsigned short>(it - soundBufferNames.begin());
}
std::shared_ptr<ESound> ResourceManager::playSoundIndex(unsigned short index, bool positional, float x, float y, bool loop) {
    return playSoundPriv(getSoundNameByIndex(index), positional, x, y, loop);
}
std::shared_ptr<ESound> ResourceManager::playSoundPriv(std::string name, bool positional, float x, float y, bool loop) {
    // The original dedicated-server thread guard is excluded with networking.
    playingSounds.erase(std::remove_if(playingSounds.begin(), playingSounds.end(), [](const auto &sound) {
        return sound.use_count() == 1 && sound->getStatus() == sf::SoundSource::Stopped;
    }), playingSounds.end());
    if (muted) return {};
    // The executable tests initialized (not its inverse) here.
    std::unique_lock<std::mutex> lock(mutexes[M_SOUNDBUFFERS], std::defer_lock);
    if (initialized.load() && !lock.try_lock()) return {};
    const auto it = soundBuffers.find(name);
    if (it == soundBuffers.end() || !it->second) {
        if (!initialized.load()) { VLOG(LOG_ERROR) << "Could not find SFX: " << name; }
        return {};
    }
    auto &buffer = *it->second;
    if ((buffer.playedThisFrame && !loop) || playingSounds.size() >= 201) return {};
    buffer.playedThisFrame = true;
    auto sound = std::make_shared<ESound>(positional, buffer);
    // MSVC generate_canonical<float> at 0041c980 consumes exactly one MT draw,
    // converts that draw to float, then divides by 2^32 (no extra distribution).
    const float random = static_cast<float>(_randEngine()) / 4294967296.0f;
    sound->setPitch((random - 0.5f) * buffer.sourceRandomPitchModifier + 1.0f);
    sound->setVolume(sfxVolumeFactor * sfxVolume);
    sound->setRelativeToListener(true);
    if (positional) sound->setGamePosition(x, y);
    sound->setLoop(loop);
    sound->play();
    sound->updatePanAndVolume();
    playingSounds.push_back(sound);
    return sound;
}
