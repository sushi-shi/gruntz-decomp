#ifndef SRC_IO_FILESTREAM_H
#define SRC_IO_FILESTREAM_H

#include <rva.h>

#include <Mfc.h>

class CFileLog {
public:
    void ReopenSharedFile(char* path);

    void OpenGruntzLog();
};

extern CFile g_gruntzLogFile;

#endif // SRC_IO_FILESTREAM_H
