#ifndef GRUNTZ_PORTABLE_TEST
#include <StdAfx.h>
#endif
#include <Io/File.h>

namespace io {
File::File() : m_file(NULL), m_error(NoError) {}
File::~File() { finish(); }
bool File::open(const char* path, Access access) {
    const std::string name = path ? path : "";
    return open(name, access);
}
bool File::open(const std::string& path, Access access) {
    if (m_file && !finish()) return false;
    m_error = NoError;
    if (path.empty() || path.find('\0') != std::string::npos) {
        m_error = OpenFailed;
        return false;
    }
    const char* mode = access == ReadOnly ? "rb" : access == Replace ? "w+b" : "r+b";
    m_file = fopen(path.c_str(), mode);
    if (!m_file) m_error = OpenFailed;
    return good();
}
bool File::finish() {
    if (m_file) {
        if (fclose(m_file) != 0 && m_error == NoError) m_error = CloseFailed;
        m_file = NULL;
    }
    return m_error == NoError;
}
bool File::flush() {
    if (!m_file && m_error == NoError) m_error = NotOpen;
    if (!good()) return false;
    if (fflush(m_file) != 0) m_error = WriteFailed;
    return good();
}
size_t File::read(void* data, size_t count) {
    if (!m_file && m_error == NoError) m_error = NotOpen;
    if (!good()) return 0;
    if (count && !data) { m_error = ReadFailed; return 0; }
    const size_t n = count ? fread(data, 1, count, m_file) : 0;
    if (n != count) m_error = ReadFailed;
    return n;
}
bool File::write(const void* data, size_t count) {
    if (!m_file && m_error == NoError) m_error = NotOpen;
    if (!good()) return false;
    if ((count && !data) || (count && fwrite(data, 1, count, m_file) != count))
        m_error = WriteFailed;
    return good();
}
bool File::seek(long offset, Origin origin) {
    if (!m_file && m_error == NoError) m_error = NotOpen;
    if (!good()) return false;
    const int base = origin == Start ? SEEK_SET : origin == Current ? SEEK_CUR : SEEK_END;
    if (fseek(m_file, offset, base) != 0) m_error = SeekFailed;
    return good();
}
long File::position() {
    if (!m_file && m_error == NoError) m_error = NotOpen;
    if (!good()) return -1;
    const long pos = ftell(m_file);
    if (pos < 0) m_error = SeekFailed;
    if (pos > 0x7fffffffL) m_error = TooLarge;
    return good() ? pos : -1;
}
size_t File::size() {
    const long old = position();
    if (old < 0 || !seek(0, End)) return 0;
    const long length = position();
    if (length < 0 || !seek(old, Start)) return 0;
    return static_cast<size_t>(length);
}
}
