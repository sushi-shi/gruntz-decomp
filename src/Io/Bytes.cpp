#ifndef GRUNTZ_PORTABLE_TEST
#include <StdAfx.h>
#endif
#include <Io/Bytes.h>
#include <string.h>
#include <limits.h>
#if UINT_MAX != 0xffffffffU
#error Binary formats require a 32-bit unsigned int
#endif
namespace io {
MemoryInput::MemoryInput(const void* bytes, size_t length) : m_offset(0), m_good(true) {
    if ((!bytes && length) || length > 0x7fffffffU) { m_good = false; return; }
    if (length) {
        const unsigned char* start = static_cast<const unsigned char*>(bytes);
        m_bytes.assign(start, start + length);
    }
}
size_t MemoryInput::read(void* data, size_t count) {
    if (!m_good) return 0;
    if (!data && count) { m_good = false; return 0; }
    size_t amount = m_bytes.size() - m_offset;
    if (amount > count) amount = count;
    if (amount) memcpy(data, &m_bytes[m_offset], amount);
    m_offset += amount;
    if (amount != count) m_good = false;
    return amount;
}
bool MemoryOutput::write(const void* data, size_t count) {
    if (!m_good) return false;
    if ((!data && count) || count > 0x7fffffffU - m_bytes.size()) {
        m_good = false; return false;
    }
    // Snapshot the input first: callers may append part of this buffer to itself.
    if (count) {
        const unsigned char* start = static_cast<const unsigned char*>(data);
        const std::vector<unsigned char> copy(start, start + count);
        m_bytes.insert(m_bytes.end(), copy.begin(), copy.end());
    }
    return true;
}
bool BinaryReader::bytes(void* data, size_t count) {
    return m_source.good() && m_source.read(data, count) == count && m_source.good();
}
bool BinaryReader::u32(unsigned int& value) {
    unsigned char bytes[4];
    if (!this->bytes(bytes, sizeof(bytes))) return false;
    value = static_cast<unsigned int>(bytes[0]) | (static_cast<unsigned int>(bytes[1]) << 8)
        | (static_cast<unsigned int>(bytes[2]) << 16) | (static_cast<unsigned int>(bytes[3]) << 24);
    return true;
}
size_t BinaryReader::remaining() {
    const long offset = m_source.position();
    const size_t length = m_source.size();
    if (!m_source.good() || offset < 0 || static_cast<size_t>(offset) > length) return 0;
    return length - static_cast<size_t>(offset);
}
bool BinaryWriter::u32(unsigned int value) {
    unsigned char bytes[4];
    for (int i = 0; i < 4; ++i) bytes[i] = static_cast<unsigned char>(value >> (i * 8));
    return m_target.write(bytes, sizeof(bytes));
}
bool readSizedBytes(Input& source, std::vector<unsigned char>& bytes) {
    BinaryReader reader(source);
    unsigned int count;
    if (!reader.u32(count) || count > reader.remaining() || !source.good()) return false;
    std::vector<unsigned char> data(count);
    if (!reader.bytes(count ? &data[0] : NULL, count)) return false;
    bytes.swap(data);
    return true;
}
bool writeSizedBytes(Output& target, const void* bytes, size_t count) {
    if (count > 0x7fffffffU) return false;
    BinaryWriter writer(target);
    return writer.u32(static_cast<unsigned int>(count)) && writer.bytes(bytes, count);
}
}
