// Dynamic initializers 004050f0..00405170, 00405610 from the original executable.
#include "recon_all.h"

const std::string ResourceManager::gfxPath = "gfx/";
const std::string ResourceManager::sfxPath = "sfx/";
const std::string ResourceManager::musicPath = "sfx/";
const std::string ResourceManager::fontPath = "gfx/font/";
const std::string ResourceManager::shaderPath = "gfx/shaders/";
std::vector<std::mutex> ResourceManager::mutexes(NUM_MUTEXES);
