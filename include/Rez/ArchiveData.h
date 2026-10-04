#ifndef GRUNTZ_REZ_ARCHIVEDATA_H
#define GRUNTZ_REZ_ARCHIVEDATA_H
#include <Io/Bytes.h>
#include <string>
#include <vector>
namespace rez {
enum DecodeError { Decoded, ShortRead, InvalidHeader, InvalidRange, InvalidRecord,
    InvalidTree, LimitExceeded };
struct Limits {
    Limits() : directoryBytes(16 * 1024 * 1024), metadataBytes(64 * 1024 * 1024),
        entries(1000000), depth(256) {}
    size_t directoryBytes;
    size_t metadataBytes;
    size_t entries;
    size_t depth;
};
struct Resource {
    unsigned int offset, size, time, id, type;
    std::string name, comment;
    std::vector<unsigned int> keys;
};
struct Directory {
    unsigned int offset, size, time;
    size_t parent, depth;
    std::string name;
    std::vector<Resource> resources;
};
struct Archive {
    Archive() : time(0), nextWrite(0), largestKeys(0), largestDirectory(0),
        largestName(0), largestComment(0), sorted(false) {}
    unsigned int time, nextWrite, largestKeys, largestDirectory, largestName, largestComment;
    bool sorted;
    // Parent precedes child. The root is index zero and has parent zero.
    std::vector<Directory> directories;
    void swap(Archive& other);
};
// Decodes the complete directory graph before replacing result. No source or
// byte-buffer pointers escape this call. Payloads remain in the source.
DecodeError decode(io::RandomInput& source, Archive& result, const Limits& limits = Limits());
// Bounded cursor over one owned directory buffer, borrowed only while decoding.
class DirectoryReader {
public:
    explicit DirectoryReader(const std::vector<unsigned char>& bytes) : m_bytes(bytes), m_offset(0) {}
    bool word(unsigned int& result);
    bool text(std::string& result);
    size_t remaining() const { return m_bytes.size() - m_offset; }
private:
    const std::vector<unsigned char>& m_bytes;
    size_t m_offset;
};
}
#endif
