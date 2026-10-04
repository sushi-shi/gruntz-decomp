#include <Rez/ArchiveData.h>
#include <Io/File.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>

static void word(std::vector<unsigned char>& bytes, size_t offset, unsigned int value) {
    assert(offset + 4 <= bytes.size());
    for (int i = 0; i < 4; ++i) bytes[offset + i] = static_cast<unsigned char>(value >> (8 * i));
}
static void append(std::vector<unsigned char>& bytes, unsigned int value) {
    const size_t at = bytes.size(); bytes.resize(at + 4); word(bytes, at, value);
}
static void text(std::vector<unsigned char>& bytes, const char* value) {
    bytes.insert(bytes.end(), value, value + std::strlen(value) + 1);
}
static std::vector<unsigned char> fixture() {
    std::vector<unsigned char> bytes(176, 0);
    bytes[0] = bytes[62] = bytes[124] = 13;
    bytes[1] = bytes[63] = bytes[125] = 10; bytes[126] = 26; bytes[167] = 1;
    word(bytes, 127, 1); word(bytes, 131, 176);
    std::memcpy(&bytes[168], "payload!", 8);
    append(bytes, 0); append(bytes, 168); append(bytes, 8); append(bytes, 42);
    append(bytes, 7); append(bytes, 0x504944); append(bytes, 2);
    text(bytes, "item"); text(bytes, "comment"); append(bytes, 123); append(bytes, 456);
    word(bytes, 135, bytes.size() - 176);
    return bytes;
}
static rez::DecodeError decode(const std::vector<unsigned char>& bytes, rez::Archive& result,
    const rez::Limits& limits = rez::Limits()) {
    io::MemoryInput input(bytes.empty() ? NULL : &bytes[0], bytes.size());
    return rez::decode(input, result, limits);
}
static void invalid(const std::vector<unsigned char>& bytes) {
    rez::Archive previous;
    previous.time = 12345;
    assert(decode(bytes, previous) != rez::Decoded);
    assert(previous.time == 12345 && previous.directories.empty());
}
// Short reads must be rejected even when a source advertises sufficient length.
class ShortSource : public io::RandomInput {
public:
    explicit ShortSource(size_t length) : m_length(length) {}
    size_t size() { return m_length; }
    bool readAt(size_t, void*, size_t) { return false; }
private:
    size_t m_length;
};
int main(int argc, char** argv) {
    const std::vector<unsigned char> valid = fixture();
    rez::Archive archive;
    assert(decode(valid, archive) == rez::Decoded);
    assert(archive.directories.size() == 1 && archive.directories[0].resources.size() == 1);
    const rez::Resource& item = archive.directories[0].resources[0];
    assert(item.name == "item" && item.comment == "comment" && item.size == 8);
    assert(item.keys.size() == 2 && item.keys[0] == 123 && item.keys[1] == 456);
    io::MemoryInput memory(&valid[0], valid.size());
    char payload[8];
    assert(memory.readAt(item.offset, payload, item.size));
    assert(std::memcmp(payload, "payload!", 8) == 0);
    assert(!memory.readAt(valid.size(), payload, 1));
    for (size_t length = 0; length < valid.size(); ++length) {
        invalid(std::vector<unsigned char>(valid.begin(), valid.begin() + length));
    }
    // Truncation inside the directory itself, with a consistent outer span.
    for (size_t length = 1; length < valid.size() - 176; ++length) {
        std::vector<unsigned char> bad = valid;
        word(bad, 135, length);
        invalid(bad);
    }
    const size_t fields[] = {127, 131, 135, 176, 180, 184, 200};
    for (size_t i = 0; i < sizeof(fields) / sizeof(fields[0]); ++i) {
        std::vector<unsigned char> bad = valid;
        word(bad, fields[i], 0xffffffffU); invalid(bad);
    }
    {
        std::vector<unsigned char> bad = valid;
        for (size_t i = 204; i < bad.size(); ++i) bad[i] = 'x';
        invalid(bad);
    }
    // A root containing one directory entry, whose body aliases the root.
    std::vector<unsigned char> tree(valid.begin(), valid.begin() + 176);
    append(tree, 1); append(tree, 176); append(tree, 18); append(tree, 0); text(tree, "d");
    word(tree, 135, 18); invalid(tree);
    word(tree, 180, tree.size()); word(tree, 184, 0);
    assert(decode(tree, archive) == rez::Decoded);
    assert(archive.directories.size() == 2 && archive.directories[1].parent == 0);
    rez::Limits limits; limits.depth = 0;
    assert(decode(tree, archive, limits) == rez::LimitExceeded);
    limits = rez::Limits(); limits.directoryBytes = 1;
    assert(decode(valid, archive, limits) == rez::LimitExceeded);
    limits = rez::Limits(); limits.metadataBytes = 1;
    assert(decode(valid, archive, limits) == rez::LimitExceeded);
    limits = rez::Limits(); limits.entries = 0;
    assert(decode(valid, archive, limits) == rez::LimitExceeded);
    ShortSource shortSource(valid.size());
    assert(rez::decode(shortSource, archive) == rez::ShortRead);
    assert(io::containsRange(8, 8, 0) && !io::containsRange(8, 9, 0));
    assert(!io::containsRange(8, 1, static_cast<size_t>(-1)));
    // Arbitrary malformed records exercise decoder bounds with a fixed seed.
    unsigned int random = 123;
    for (size_t trial = 0; trial < 3000; ++trial) {
        std::vector<unsigned char> bytes = valid;
        for (size_t mutation = 0; mutation < 5; ++mutation) {
            random = random * 1664525U + 1013904223U;
            const size_t at = random % bytes.size();
            random = random * 1664525U + 1013904223U;
            bytes[at] = static_cast<unsigned char>(random >> 24);
        }
        rez::Archive result;
        decode(bytes, result);
    }
    for (int i = 1; i < argc; ++i) {
        io::File file;
        assert(file.open(argv[i], io::ReadOnly));
        const rez::DecodeError error = rez::decode(file, archive);
        if (error != rez::Decoded) std::fprintf(stderr, "Decode error %d: %s\n", error, argv[i]);
        assert(error == rez::Decoded);
        size_t count = 0;
        for (size_t d = 0; d < archive.directories.size(); ++d) {
            count += archive.directories[d].resources.size();
            for (size_t r = 0; r < archive.directories[d].resources.size(); ++r) {
                const rez::Resource& resource = archive.directories[d].resources[r];
                if (resource.size) {
                    unsigned char byte;
                    assert(file.readAt(resource.offset, &byte, 1));
                    assert(file.readAt(resource.offset + resource.size - 1, &byte, 1));
                }
            }
        }
        std::printf("Read %lu directories, %lu members from %s\n",
            static_cast<unsigned long>(archive.directories.size()), static_cast<unsigned long>(count), argv[i]);
    }
    std::puts("REZ bounds, graph and transactional decode tests passed.");
}
