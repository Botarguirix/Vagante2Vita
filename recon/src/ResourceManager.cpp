#include "recon_all.h"

std::string ResourceManager::getLanguageString(LanguageType language) {
    static constexpr const char *codes[] = {
        "en", "de", "fr", "es", "ja", "ko", "zh", "ru", "pt", "pl", "tr", "th", "it", "cs", "oth"};
    const auto index = static_cast<unsigned int>(language);
    return index < NUM_LANGUAGES ? codes[index] : "";
}
