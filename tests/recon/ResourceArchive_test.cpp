#include "recon_all.h"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <zlib.h>

namespace {
void appendInteger(std::string &data, std::uint32_t value, unsigned int bytes = 4) {
    for (unsigned int i = 0; i < bytes; ++i) data.push_back(char(value >> (i * 8)));
}

void appendRecord(std::string &data, const std::string &name, const std::string &bytes) {
    appendInteger(data, name.size());
    data += name;
    appendInteger(data, bytes.size());
    data += bytes;
}

std::string makeArchive() {
    std::string data;
    appendInteger(data, 8);
    appendInteger(data, 4, 2);
    appendRecord(data, "gfx/example.png", std::string("png\0bytes", 9));
    appendRecord(data, "music/example.ogg", "streamed music");
    appendRecord(data, "duplicate", "old");
    appendRecord(data, "duplicate", "new");
    // These are uncounted and kept separately from the resource map.
    appendRecord(data, "rooms.json", "{\"rooms\":[]}");
    appendRecord(data, "palettes.json", "{\"palettes\":[]}");
    appendRecord(data, "attacks.json", "{\"attacks\":[]}");
    return data;
}

void writeArchive(const std::string &data) {
    std::string compressed(compressBound(data.size()), '\0');
    uLongf size = compressed.size();
    assert(compress2(reinterpret_cast<Bytef *>(&compressed[0]), &size,
                     reinterpret_cast<const Bytef *>(data.data()), data.size(), Z_DEFAULT_COMPRESSION) == Z_OK);
    std::ofstream output("data.vra", std::ios::binary);
    output.write(compressed.data(), size);
}

void testSyntheticArchive() {
    writeArchive(makeArchive());
    ResourceArchive archive("data.vra", false);
    assert(!archive.isLoaded());
    archive.load();
    assert(archive.isLoaded());
    auto *image = archive.getMemoryBlock("gfx/example.png");
    assert(image && image->getSize() == 9 && !image->isManaged());
    assert(std::memcmp(image->getData(), "png\0bytes", 9) == 0);
    auto *lastDuplicate = archive.getMemoryBlock("duplicate");
    assert(lastDuplicate && lastDuplicate->getSize() == 3);
    assert(std::memcmp(lastDuplicate->getData(), "new", 3) == 0);
    assert(std::strcmp(archive.roomData, "{\"rooms\":[]}") == 0);
    assert(std::strcmp(archive.paletteData, "{\"palettes\":[]}") == 0);
    assert(std::strcmp(archive.attackData, "{\"attacks\":[]}") == 0);
    assert(!archive.getMemoryBlock("rooms.json"));

    auto *music = archive.getManagedMemoryBlock("music/example.ogg");
    assert(music && music->isManaged());
    archive.cleanUpMemoryBlocks(false);
    assert(!archive.getMemoryBlock("gfx/example.png"));
    assert(archive.getMemoryBlock("music/example.ogg") == music);
    assert(std::strcmp(archive.roomData, "{\"rooms\":[]}") == 0);
    archive.cleanUpMemoryBlocks(true);
    assert(!archive.getMemoryBlock("music/example.ogg"));

    // Reload creates a fresh set after the previously read blocks were released.
    archive.load();
    assert(archive.isLoaded() && archive.getMemoryBlock("gfx/example.png"));
}

void testMalformedArchives() {
    auto data = makeArchive();
    data.resize(data.size() - 1);
    writeArchive(data);
    ResourceArchive truncated("data.vra", false);
    truncated.load();
    assert(!truncated.isLoaded() && !truncated.roomData);
    assert(!truncated.getMemoryBlock("gfx/example.png"));

    std::ofstream("data.vra", std::ios::binary) << "not a zlib stream";
    ResourceArchive invalid("data.vra", false);
    invalid.load();
    assert(!invalid.isLoaded());

    std::filesystem::remove("data.vra");
    ResourceArchive missing("data.vra", false);
    missing.load();
    assert(!missing.isLoaded());
}

void testUserArchive() {
    const auto *archivePath = std::getenv("VAGANTE_VRA");
    if (!archivePath || !*archivePath) return;
    const auto path = std::filesystem::absolute(archivePath);
    assert(path.filename() == "data.vra" && std::filesystem::exists(path));
    const auto previous = std::filesystem::current_path();
    std::filesystem::current_path(path.parent_path());
    {
        ResourceArchive archive("data.vra", false);
        archive.load();
        assert(archive.isLoaded());
        auto *loading = archive.getMemoryBlock("gfx/gui/loading.png");
        const unsigned char signature[] = {137, 'P', 'N', 'G', 13, 10, 26, 10};
        assert(loading && loading->getSize() >= sizeof(signature));
        assert(std::memcmp(loading->getData(), signature, sizeof(signature)) == 0);
        assert(archive.roomData && *archive.roomData);
        assert(archive.paletteData && *archive.paletteData);
        assert(archive.attackData && *archive.attackData);
        // The music buffer must outlive the initial release of ordinary resources.
        auto *managed = archive.getManagedMemoryBlock("gfx/gui/loading.png");
        archive.cleanUpMemoryBlocks(false);
        assert(archive.getMemoryBlock("gfx/gui/loading.png") == managed);
    }
    std::filesystem::current_path(previous);
    std::cout << "External user data.vra: loaded resources and three text blobs\n";
}
}

int main() {
    testSyntheticArchive();
    testMalformedArchives();
    testUserArchive();
    std::cout << "ResourceArchive tests passed\n";
}
