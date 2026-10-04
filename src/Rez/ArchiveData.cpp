#ifndef GRUNTZ_PORTABLE_TEST
#include <StdAfx.h>
#endif
#include <Rez/ArchiveData.h>
#include <map>
#include <algorithm>
#include <string.h>
namespace rez {
static const size_t headerBytes = 168;
static unsigned int wordAt(const unsigned char* p) {
    return static_cast<unsigned int>(p[0]) | (static_cast<unsigned int>(p[1]) << 8)
        | (static_cast<unsigned int>(p[2]) << 16) | (static_cast<unsigned int>(p[3]) << 24);
}
bool DirectoryReader::word(unsigned int& result) {
    if (remaining() < 4) return false;
    result = wordAt(&m_bytes[m_offset]); m_offset += 4; return true;
}
bool DirectoryReader::text(std::string& result) {
    if (!remaining()) return false;
    const char* start = reinterpret_cast<const char*>(&m_bytes[m_offset]);
    const char* end = static_cast<const char*>(memchr(start, 0, remaining()));
    if (!end) return false;
    result.assign(start, end - start);
    m_offset += end - start + 1;
    return true;
}
void Archive::swap(Archive& other) {
    std::swap(time, other.time); std::swap(nextWrite, other.nextWrite);
    std::swap(largestKeys, other.largestKeys); std::swap(largestDirectory, other.largestDirectory);
    std::swap(largestName, other.largestName); std::swap(largestComment, other.largestComment);
    std::swap(sorted, other.sorted); directories.swap(other.directories);
}
static bool validSpan(size_t length, unsigned int offset, unsigned int size) {
    return io::containsRange(length, offset, size) && (!size || offset >= headerBytes);
}
DecodeError decode(io::RandomInput& source, Archive& result, const Limits& limits) {
    const size_t length = source.size();
    unsigned char header[headerBytes];
    if (length < headerBytes || !source.readAt(0, header, sizeof(header))) return ShortRead;
    if (header[0] != 13 || header[1] != 10 || header[62] != 13 || header[63] != 10
        || header[124] != 13 || header[125] != 10 || header[126] != 26
        || wordAt(header + 127) != 1 || header[167] > 1) return InvalidHeader;
    Archive parsed;
    Directory root;
    root.offset = wordAt(header + 131); root.size = wordAt(header + 135);
    root.time = wordAt(header + 139); root.parent = 0; root.depth = 0;
    parsed.nextWrite = wordAt(header + 143); parsed.time = wordAt(header + 147);
    parsed.largestKeys = wordAt(header + 151); parsed.largestDirectory = wordAt(header + 155);
    parsed.largestName = wordAt(header + 159); parsed.largestComment = wordAt(header + 163);
    parsed.sorted = header[167] != 0;
    parsed.directories.push_back(root);
    std::map<unsigned int, unsigned int> spans;
    size_t metadataBytes = 0;
    size_t entryCount = 0;
    for (size_t index = 0; index < parsed.directories.size(); ++index) {
        // Copy: appending child directories can reallocate the vector.
        const Directory current = parsed.directories[index];
        if (!validSpan(length, current.offset, current.size)) return InvalidRange;
        if (current.depth > limits.depth || current.size > limits.directoryBytes
            || !io::containsRange(limits.metadataBytes, metadataBytes, current.size)) return LimitExceeded;
        metadataBytes += current.size;
        if (current.size) {
            std::map<unsigned int, unsigned int>::iterator next = spans.lower_bound(current.offset);
            if (next != spans.end() && current.size > next->first - current.offset) return InvalidTree;
            if (next != spans.begin()) {
                --next;
                if (current.offset - next->first < next->second) return InvalidTree;
            }
            spans[current.offset] = current.size;
        }
        std::vector<unsigned char> bytes(current.size);
        if (current.size && !source.readAt(current.offset, &bytes[0], current.size)) return ShortRead;
        DirectoryReader reader(bytes);
        std::vector<Resource> resources;
        while (reader.remaining()) {
            if (entryCount >= limits.entries) return LimitExceeded;
            ++entryCount;
            unsigned int kind;
            if (!reader.word(kind)) return InvalidRecord;
            if (kind == 1) {
                Directory child;
                child.parent = index; child.depth = current.depth + 1;
                if (!reader.word(child.offset) || !reader.word(child.size) || !reader.word(child.time)
                    || !reader.text(child.name) || child.name.empty()) return InvalidRecord;
                parsed.directories.push_back(child);
            } else if (kind == 0) {
                Resource item;
                unsigned int count;
                if (!reader.word(item.offset) || !reader.word(item.size) || !reader.word(item.time)
                    || !reader.word(item.id) || !reader.word(item.type) || !reader.word(count)
                    || !reader.text(item.name) || item.name.empty() || !reader.text(item.comment)) return InvalidRecord;
                if (!validSpan(length, item.offset, item.size)) return InvalidRange;
                if (count > reader.remaining() / 4) return InvalidRecord;
                item.keys.resize(count);
                for (size_t k = 0; k < count; ++k) {
                    if (!reader.word(item.keys[k])) return InvalidRecord;
                }
                resources.push_back(item);
            } else return InvalidRecord;
        }
        parsed.directories[index].resources.swap(resources);
    }
    result.swap(parsed);
    return Decoded;
}
}
