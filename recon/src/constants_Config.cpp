#include "recon_all.h"

// Format comparison in Config::load (00577110), and dynamic initializers
// 00402a00, 00402a20, 00402a40. These override the generator's TODO defaults.
const int Config::FILE_FORMAT_VERSION = 35;
const std::string Config::LEGACY_CONFIG_FILE_NAME = "vagante.cfg";
const std::string Config::CONFIG_DIR_PATH = "./config";
const std::string Config::CONFIG_FILE_NAME = "config.ini";
