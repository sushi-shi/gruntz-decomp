#ifndef SRC_IO_FILESTREAM_H
#define SRC_IO_FILESTREAM_H
#include <rva.h> // OVERRIDE macro (override under clang, no-op under MSVC 5.0)

#include <Mfc.h> // CObject, CString + <windows.h> (CreateFileA/ReadFile/...) FIRST

typedef CFile CFileIO;

class CFileLog {
public:
    // Reopen the shared file object (0x646778) around a close: open(path, 0x1000), close,
    // open(path, 1). `this` unused.
    void ReopenSharedFile(char* path); // 0x0bd3e0
    // Close the shared global file, then reopen it on the fixed "c:\gruntz.log" debug path.
    // `this` is forwarded to ReopenSharedFile, which ignores it. __thiscall, no args.
    void OpenGruntzLog(); // 0x0bd450
};
SIZE_UNKNOWN(); // data-less __thiscall host (never constructed)

#endif // SRC_IO_FILESTREAM_H
