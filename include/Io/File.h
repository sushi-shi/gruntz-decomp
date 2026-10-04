#ifndef GRUNTZ_IO_FILE_H
#define GRUNTZ_IO_FILE_H
#include <stdio.h>
#include <string>
#include <stddef.h>
#include <Io/Bytes.h>

namespace io {
bool absolutePath(const std::string& path, std::string& result);
bool replaceFile(const std::string& source, const std::string& target);
enum Access { ReadOnly, Replace, Update };
enum Origin { Start, Current, End };
enum Error { NoError, NotOpen, NotFound, OpenFailed, ReadFailed, WriteFailed, SeekFailed, TooLarge, CloseFailed };

class File : public Input, public Output {
public:
    File();
    virtual ~File();
    bool open(const std::string& path, Access access);
    bool open(const char* path, Access access);
    bool finish();
    bool flush();
    size_t read(void* data, size_t size);
    bool write(const void* data, size_t size);
    bool seek(long offset, Origin origin);
    long position();
    size_t size();
    bool good() const { return m_error == NoError && m_file != NULL; }
    Error error() const { return m_error; }
    // Borrowed only by legacy decoders that require a native file handle.
    FILE* nativeFile() const { return m_file; }
private:
    File(const File&);
    File& operator=(const File&);
    FILE* m_file;
    Error m_error;
};
}
#endif
