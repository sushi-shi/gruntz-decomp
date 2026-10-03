#ifndef SRC_IO_FILESTREAM_H
#define SRC_IO_FILESTREAM_H

#include <Ints.h>

class CFileLog {
public:
    void ReopenSharedFile(char* path);

    void OpenGruntzLog();
    i32 IsLoggingEnabled();
};

extern CFile g_gruntzLogFile;

#endif
