#ifndef GRUNTZ_IO_BYTES_H
#define GRUNTZ_IO_BYTES_H
#include <stddef.h>
#include <vector>
namespace io {
class Input {
public:
    virtual ~Input() {}
    virtual size_t read(void* data, size_t count) = 0;
    virtual size_t size() = 0;
    virtual long position() = 0;
    virtual bool good() const = 0;
};
class Output {
public:
    virtual ~Output() {}
    virtual bool write(const void* data, size_t count) = 0;
    virtual bool good() const = 0;
};
class MemoryInput : public Input {
public:
    MemoryInput(const void* bytes, size_t length);
    virtual size_t read(void* data, size_t count);
    virtual size_t size() { return m_bytes.size(); }
    virtual long position() { return static_cast<long>(m_offset); }
    virtual bool good() const { return m_good; }
private:
    std::vector<unsigned char> m_bytes;
    size_t m_offset;
    bool m_good;
};
class MemoryOutput : public Output {
public:
    MemoryOutput() : m_good(true) {}
    virtual bool write(const void* data, size_t count);
    virtual bool good() const { return m_good; }
    const std::vector<unsigned char>& bytes() const { return m_bytes; }
private:
    std::vector<unsigned char> m_bytes;
    bool m_good;
};
class BinaryReader {
public:
    explicit BinaryReader(Input& source) : m_source(source) {}
    bool bytes(void* data, size_t count);
    bool u32(unsigned int& value);
    size_t remaining();
private:
    Input& m_source;
};
class BinaryWriter {
public:
    explicit BinaryWriter(Output& target) : m_target(target) {}
    bool bytes(const void* data, size_t count) { return m_target.write(data, count); }
    bool u32(unsigned int value);
private:
    Output& m_target;
};
bool readSizedBytes(Input& source, std::vector<unsigned char>& bytes);
bool writeSizedBytes(Output& target, const void* bytes, size_t count);
}
#endif
