#ifndef GRUNTZ_PORTABLE_TEST
#include <StdAfx.h>
#endif
#include <Io/File.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#include <io.h>
#else
#include <unistd.h>
#endif
#include <vector>

namespace io {
bool absolutePath(const std::string& path, std::string& result) {
    if (path.empty() || path.find('\0') != std::string::npos) return false;
#ifdef _WIN32
    char* resolved = _fullpath(NULL, path.c_str(), 0);
    if (!resolved) return false;
    result = resolved;
    free(resolved);
    return true;
#else
    if (path[0] == '/') { result = path; return true; }
    for (size_t capacity = 256; capacity <= 1024 * 1024; capacity *= 2) {
        std::vector<char> directory(capacity);
        if (getcwd(&directory[0], capacity)) {
            result = std::string(&directory[0]) + "/" + path;
            return true;
        }
        if (errno != ERANGE) return false;
    }
    return false;
#endif
}

bool replaceFile(const std::string& source, const std::string& target) {
    if (source.empty() || target.empty() || source.find('\0') != std::string::npos
        || target.find('\0') != std::string::npos) return false;
#ifdef _WIN32
    return MoveFileExA(source.c_str(), target.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    return rename(source.c_str(), target.c_str()) == 0;
#endif
}

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
    if (!m_file) m_error = errno == ENOENT ? NotFound : OpenFailed;
    return good();
}
bool File::createSibling(const std::string& target, std::string& createdPath) {
    if (m_file && !finish()) return false;
    m_error = NoError;
    createdPath.erase();
    if (target.empty() || target.find('\0') != std::string::npos) {
        m_error = OpenFailed;
        return false;
    }
    for (unsigned int index = 0; index < 1024; ++index) {
        char suffix[24];
        sprintf(suffix, ".stage-%u", index);
        std::string candidate = target + suffix;
#ifdef _WIN32
        const int descriptor = _open(candidate.c_str(), _O_CREAT | _O_EXCL | _O_RDWR | _O_BINARY,
                                     _S_IREAD | _S_IWRITE);
#else
        const int descriptor = ::open(candidate.c_str(), O_CREAT | O_EXCL | O_RDWR, 0600);
#endif
        if (descriptor < 0) {
            if (errno == EEXIST) continue;
            m_error = OpenFailed;
            return false;
        }
#ifdef _WIN32
        m_file = _fdopen(descriptor, "w+b");
#else
        m_file = fdopen(descriptor, "w+b");
#endif
        if (!m_file) {
#ifdef _WIN32
            _close(descriptor);
#else
            ::close(descriptor);
#endif
            remove(candidate.c_str());
            m_error = OpenFailed;
            return false;
        }
        createdPath.swap(candidate);
        return true;
    }
    m_error = OpenFailed;
    return false;
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
