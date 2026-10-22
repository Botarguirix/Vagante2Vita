#include "recon_all.h"

#include <array>
#include <cstdint>
#include <memory>
#include <zlib.h>

#define VLOG(level) if ((level) > Log::reportingLevel()) ; else Log().get(level)

namespace {
// Archive integers are little endian, independently of the host's byte order.
bool readArchiveInteger(std::istream &input, std::uint32_t &value, unsigned int bytes = 4) {
    unsigned char buffer[4]{};
    if (!input.read(reinterpret_cast<char *>(buffer), bytes)) return false;
    value = 0;
    for (unsigned int i = 0; i < bytes; ++i) value |= std::uint32_t(buffer[i]) << (8 * i);
    return true;
}

bool readArchiveName(std::istream &input, std::string &name) {
    std::uint32_t length = 0;
    if (!readArchiveInteger(input, length)) return false;
    // Check the remaining bytes before allocating a length supplied by the file.
    const auto start = input.tellg();
    input.seekg(0, std::ios::end);
    const auto end = input.tellg();
    input.seekg(start);
    if (start < 0 || end < start || std::uint64_t(end - start) < length) return false;
    name.resize(length);
    return length == 0 || bool(input.read(&name[0], length));
}

bool readArchiveData(std::istream &input, std::unique_ptr<char[]> &data,
                     std::uint32_t &size, bool terminate) {
    if (!readArchiveInteger(input, size)) return false;
    const auto start = input.tellg();
    input.seekg(0, std::ios::end);
    const auto end = input.tellg();
    input.seekg(start);
    if (start < 0 || end < start || std::uint64_t(end - start) < size) return false;
    data = std::make_unique<char[]>(std::size_t(size) + (terminate ? 1 : 0));
    if (size != 0 && !input.read(data.get(), size)) return false;
    if (terminate) data[size] = '\0';
    return true;
}
}

ResourceBlock::ResourceBlock(char *bytes, int byteCount)
    : data(bytes), size(static_cast<unsigned int>(byteCount)), managed(false) {}

ResourceBlock::~ResourceBlock() {
    delete[] data;
    data = nullptr;
}

unsigned int ResourceBlock::getSize() { return size; }
char *ResourceBlock::getData() { return data; }
void ResourceBlock::setManaged(bool value) { managed = value; }
bool ResourceBlock::isManaged() { return managed; }

ResourceArchive::ResourceArchive(const char *, bool)
    : loaded(false), checksum(0), roomData(nullptr), attackData(nullptr), paletteData(nullptr) {
    // The constructor expanded in ResourceManager::initialize only initializes fields.
    // Its PDB signature retains two arguments, but the archive path is fixed by load().
}

ResourceArchive::~ResourceArchive() {
    cleanUpMemoryBlocks(true);
    delete[] roomData;
    delete[] attackData;
    delete[] paletteData;
}

void ResourceArchive::load() {
    // ResourceArchive::load, 00739180. The counted resources are followed by three
    // uncounted text blobs, in room/palette/attack order (not map entries).
    std::ifstream compressed("data.vra", std::ios::binary);
    if (!compressed) {
        loaded = false;
        VLOG(LOG_ERROR) << "Unable to load data.vra";
        return;
    }

    std::stringstream expanded(std::ios::in | std::ios::out | std::ios::binary);
    const auto result = inflateFile(compressed, expanded);
    if (result != Z_OK) {
        loaded = false;
        VLOG(LOG_ERROR) << "ResourceArchive::load: unable to decompress data.vra (" << result << ")";
        return;
    }
    expanded.seekg(0);
    std::uint32_t version = 0;
    std::uint32_t count = 0;
    if (!readArchiveInteger(expanded, version) || !readArchiveInteger(expanded, count, 2)) {
        loaded = false;
        VLOG(LOG_ERROR) << "ResourceArchive::load: truncated data.vra header";
        return;
    }
    // The original reads the version word but does not reject a different value.
    (void)version;

    // Commit the newly read blocks together. A damaged archive must not expose
    // partially initialized data or leak allocations made before the failing record.
    std::map<std::string, std::unique_ptr<ResourceBlock>> blocks;
    std::unique_ptr<char[]> rooms, palettes, attacks;
    auto malformed = [&]() {
        loaded = false;
        VLOG(LOG_ERROR) << "ResourceArchive::load: truncated data.vra record";
    };
    for (std::uint32_t i = 0; i < count; ++i) {
        std::string name;
        std::unique_ptr<char[]> data;
        std::uint32_t size = 0;
        if (!readArchiveName(expanded, name) || !readArchiveData(expanded, data, size, false)) {
            malformed();
            return;
        }
        // As in the original map assignment, the last occurrence of a path wins.
        blocks[name] = std::make_unique<ResourceBlock>(data.release(), static_cast<int>(size));
    }
    for (auto *text : {&rooms, &palettes, &attacks}) {
        std::string name;
        std::uint32_t size = 0;
        if (!readArchiveName(expanded, name) || !readArchiveData(expanded, *text, size, true)) {
            malformed();
            return;
        }
    }

    cleanUpMemoryBlocks(true);
    delete[] roomData;
    delete[] paletteData;
    delete[] attackData;
    for (auto &entry : blocks) memoryBlocks[entry.first] = entry.second.release();
    roomData = rooms.release();
    paletteData = palettes.release();
    attackData = attacks.release();
    loaded = true;
    VLOG(LOG_DEBUG) << "ResourceArchive::load: Successfully loaded data.vra resource indices into memory.";
}

ResourceBlock *ResourceArchive::getMemoryBlock(std::string name) {
    const auto found = memoryBlocks.find(name);
    if (found != memoryBlocks.end()) return found->second;
    VLOG(LOG_ERROR) << "ResourceArchive::getMemoryBlockUnable to find " << name << " in vagante archive!";
    return nullptr;
}

ResourceBlock *ResourceArchive::getManagedMemoryBlock(std::string name) {
    auto *block = getMemoryBlock(std::move(name));
    if (block) block->setManaged(true);
    return block;
}

bool ResourceArchive::isLoaded() const { return loaded; }

void ResourceArchive::addResourceLocationDescriptor(std::string path, ResourceType type) {
    resourceLocationDescriptor.emplace_back(std::move(path), type);
}

void ResourceArchive::cleanUpMemoryBlocks(bool includeManaged) {
    for (auto block = memoryBlocks.begin(); block != memoryBlocks.end();) {
        if (includeManaged || !block->second->isManaged()) {
            delete block->second;
            block = memoryBlocks.erase(block);
        } else {
            ++block;
        }
    }
}

int ResourceArchive::inflateFile(std::istream &input, std::ostream &output) {
    // ResourceArchive::inflateFile, 0073ae50: zlib streaming with 0x4000-byte buffers.
    std::array<unsigned char, 0x4000> in{};
    std::array<unsigned char, 0x4000> out{};
    z_stream stream{};
    int result = inflateInit(&stream);
    if (result != Z_OK) return result;
    do {
        input.read(reinterpret_cast<char *>(in.data()), in.size());
        if (input.bad()) {
            inflateEnd(&stream);
            return Z_ERRNO;
        }
        stream.avail_in = static_cast<uInt>(input.gcount());
        if (stream.avail_in == 0) break;
        stream.next_in = in.data();
        do {
            stream.avail_out = out.size();
            stream.next_out = out.data();
            result = inflate(&stream, Z_NO_FLUSH);
            if (result == Z_NEED_DICT) result = Z_DATA_ERROR;
            if (result == Z_DATA_ERROR || result == Z_MEM_ERROR || result == Z_STREAM_ERROR) {
                inflateEnd(&stream);
                return result;
            }
            output.write(reinterpret_cast<const char *>(out.data()), out.size() - stream.avail_out);
            if (!output) {
                inflateEnd(&stream);
                return Z_ERRNO;
            }
        } while (stream.avail_out == 0);
    } while (result != Z_STREAM_END);
    inflateEnd(&stream);
    return result == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}
