#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <locale>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace recon {

// Portable replacement for the message-catalog facet installed by Boost.Locale.
// Catalogs remain external game data; no translated text is compiled into the port.
class MessageCatalog : public std::locale::facet {
public:
    inline static std::locale::id id;
    std::map<std::string, std::string> messages;

    explicit MessageCatalog(const std::filesystem::path &path) {
        std::ifstream input(path, std::ios::binary);
        if (!input) return;
        const std::vector<unsigned char> data((std::istreambuf_iterator<char>(input)), {});
        if (data.size() < 28) return;
        const auto little = [&](std::size_t offset) {
            return std::uint32_t(data[offset]) | (std::uint32_t(data[offset + 1]) << 8) |
                   (std::uint32_t(data[offset + 2]) << 16) | (std::uint32_t(data[offset + 3]) << 24);
        };
        const auto magic = little(0);
        if (magic != 0x950412de && magic != 0xde120495) return;
        const auto word = [&](std::size_t offset) {
            const auto value = little(offset);
            if (magic == 0x950412de) return value;
            return (value >> 24) | ((value >> 8) & 0xff00) |
                   ((value << 8) & 0xff0000) | (value << 24);
        };
        const std::size_t count = word(8), originals = word(12), translations = word(16);
        if (count > data.size() / 8 || originals > data.size() - count * 8 ||
            translations > data.size() - count * 8) return;
        for (std::size_t index = 0; index < count; ++index) {
            const std::size_t keyLength = word(originals + index * 8);
            const std::size_t keyOffset = word(originals + index * 8 + 4);
            const std::size_t valueLength = word(translations + index * 8);
            const std::size_t valueOffset = word(translations + index * 8 + 4);
            if (keyOffset > data.size() || keyLength > data.size() - keyOffset ||
                valueOffset > data.size() || valueLength > data.size() - valueOffset) continue;
            std::string key(reinterpret_cast<const char *>(data.data() + keyOffset), keyLength);
            std::string value(reinterpret_cast<const char *>(data.data() + valueOffset), valueLength);
            // A singular lookup uses the first entry of a plural record as gettext does.
            key.resize(key.find('\0') == std::string::npos ? key.size() : key.find('\0'));
            value.resize(value.find('\0') == std::string::npos ? value.size() : value.find('\0'));
            if (!key.empty() && !value.empty()) messages.emplace(std::move(key), std::move(value));
        }
    }
};

inline void installMessageLocale(const std::filesystem::path &root, const std::string &code) {
    std::locale base = std::locale::classic();
    try { base = std::locale((code + ".UTF-8").c_str()); }
    catch (const std::runtime_error &) { /* The host may not install this locale. */ }
    auto catalog = root / code / "LC_MESSAGES" / (code + ".mo");
    if (!std::filesystem::exists(catalog))
        catalog = root / (code + ".UTF-8") / "LC_MESSAGES" / (code + ".mo");
    std::locale::global(std::locale(base, new MessageCatalog(catalog)));
}

inline std::string translate(const std::string &text) {
    const std::locale locale;
    if (!std::has_facet<MessageCatalog>(locale)) return text;
    const auto &messages = std::use_facet<MessageCatalog>(locale).messages;
    const auto found = messages.find(text);
    return found == messages.end() ? text : found->second;
}

} // namespace recon
