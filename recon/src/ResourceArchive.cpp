#include "recon_all.h"

#include <array>
#include <cstdint>
#include <memory>
#include <zlib.h>
#if defined(__vita__)
#include <malloc.h>
#endif

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

// The archive is read from a forward-only zlib stream (no seeking), so lengths supplied by the
// file are bounded instead of compared with the remaining size; a short read still fails.
constexpr std::uint32_t kMaxArchiveName = 4096;
constexpr std::uint32_t kMaxArchiveData = 64u * 1024u * 1024u;

bool readArchiveName(std::istream &input, std::string &name) {
    std::uint32_t length = 0;
    if (!readArchiveInteger(input, length) || length > kMaxArchiveName) return false;
    name.resize(length);
    return length == 0 || bool(input.read(&name[0], length));
}

bool readArchiveData(std::istream &input, std::unique_ptr<char[]> &data,
                     std::uint32_t &size, bool terminate) {
    if (!readArchiveInteger(input, size) || size > kMaxArchiveData) return false;
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

namespace {
// Read-only streambuf that inflates data.vra on demand. The original inflated the whole archive
// into a stringstream first; on the Vita (256 MB heap) the ~124 MB buffer plus its growth and the
// per-resource copies run out of memory, so records are parsed straight from the zlib stream.
class InflateStreambuf : public std::streambuf {
public:
    explicit InflateStreambuf(std::istream &source) : source_(source) {
        ok_ = inflateInit(&stream_) == Z_OK;
    }
    ~InflateStreambuf() override { inflateEnd(&stream_); }
    int status() const { return status_; }

protected:
    int_type underflow() override {
        if (gptr() < egptr()) return traits_type::to_int_type(*gptr());
        if (!ok_ || finished_) return traits_type::eof();
        stream_.next_out = reinterpret_cast<Bytef *>(out_.data());
        stream_.avail_out = static_cast<uInt>(out_.size());
        while (stream_.avail_out == out_.size() && !finished_) {
            if (stream_.avail_in == 0) {
                source_.read(in_.data(), in_.size());
                stream_.avail_in = static_cast<uInt>(source_.gcount());
                stream_.next_in = reinterpret_cast<Bytef *>(in_.data());
                if (stream_.avail_in == 0) { status_ = Z_DATA_ERROR; finished_ = true; break; }
            }
            const int result = inflate(&stream_, Z_NO_FLUSH);
            if (result == Z_STREAM_END) finished_ = true;
            else if (result != Z_OK && result != Z_BUF_ERROR) { status_ = result; finished_ = true; break; }
        }
        const std::size_t produced = out_.size() - stream_.avail_out;
        if (produced == 0) return traits_type::eof();
        setg(out_.data(), out_.data(), out_.data() + produced);
        return traits_type::to_int_type(*gptr());
    }

private:
    std::istream &source_;
    z_stream stream_{};
    std::array<char, 0x4000> in_{};
    std::array<char, 0x10000> out_{};
    bool ok_ = false;
    bool finished_ = false;
    int status_ = Z_OK;
};
} // namespace

void ResourceArchive::load() {
    // ResourceArchive::load, 00739180. The counted resources are followed by three
    // uncounted text blobs, in room/palette/attack order (not map entries).
    std::ifstream compressed("data.vra", std::ios::binary);
    if (!compressed) {
        loaded = false;
        VLOG(LOG_ERROR) << "Unable to load data.vra";
        return;
    }

    InflateStreambuf inflater(compressed);
    std::istream expanded(&inflater);
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

    if (inflater.status() != Z_OK) {
        loaded = false;
        VLOG(LOG_ERROR) << "ResourceArchive::load: unable to decompress data.vra (" << inflater.status() << ")";
        return;
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
#if defined(__vita__)
    VLOG(LOG_INFO) << "Vita heap in use after data.vra: " << (mallinfo().uordblks / (1024 * 1024)) << " MB";
#endif
}

ResourceBlock *ResourceArchive::getMemoryBlock(std::string name) {
    const auto found = memoryBlocks.find(name);
    if (found != memoryBlocks.end()) {
#if defined(__vita__)
        // Vita memory budget (port change): unmanaged blocks are only read once while loading
        // (images, sounds, shaders), and the original frees them all at the end of loading with
        // cleanUpMemoryBlocks(false). Here each one is handed over and freed on the next request,
        // so compressed data does not pile up next to the decoded images.
        if (!found->second->isManaged()) {
            static std::unique_ptr<ResourceBlock> handedOver;
            handedOver.reset(found->second);
            memoryBlocks.erase(found);
            return handedOver.get();
        }
#endif
        return found->second;
    }
    VLOG(LOG_ERROR) << "ResourceArchive::getMemoryBlockUnable to find " << name << " in vagante archive!";
    return nullptr;
}

ResourceBlock *ResourceArchive::getManagedMemoryBlock(std::string name) {
    const auto found = memoryBlocks.find(name);
    if (found == memoryBlocks.end()) {
        VLOG(LOG_ERROR) << "ResourceArchive::getMemoryBlockUnable to find " << name << " in vagante archive!";
        return nullptr;
    }
    found->second->setManaged(true);
    return found->second;
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
