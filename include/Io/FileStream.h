#ifndef SRC_IO_FILESTREAM_H
#define SRC_IO_FILESTREAM_H
#include <Io/File.h>

#include <Ints.h>

class CFileLog {
public:
    bool ReopenSharedFile(const std::string& path);

    bool OpenGruntzLog();
    i32 IsLoggingEnabled();
};

extern io::File g_gruntzLogFile;

#endif
