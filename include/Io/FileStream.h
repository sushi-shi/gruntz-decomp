#ifndef SRC_IO_FILESTREAM_H
#define SRC_IO_FILESTREAM_H
#include <rva.h> // OVERRIDE macro (override under clang, no-op under MSVC 5.0)

#include <Mfc.h> // CObject, CString + <windows.h> (CreateFileA/ReadFile/...) FIRST

struct CFileIODispatch {
    // Slots 0-9 are MFC's own (CObject's five + CFile's early getters; the slot-10
    // Open @+0x28 pins the alignment). Never dispatched through this view - the
    // names document the real table, the void() signatures stay dispatch-only.
    virtual void GetRuntimeClass(); // [0] CObject
    virtual void Dtor();            // [1] scalar-deleting ~CFile
    virtual void Serialize();       // [2] CObject
    virtual void AssertValid();     // [3] CObject
    virtual void Dump();            // [4] CObject
    virtual void GetPosition();     // [5] CFile::GetPosition
    virtual void GetFileName();     // [6] CFile::GetFileName
    virtual void GetFileTitle();    // [7] CFile::GetFileTitle
    virtual void GetFilePath();     // [8] CFile::GetFilePath
    virtual void SetFilePath();     // [9] CFile::SetFilePath
    virtual i32 Open(const char* name, u32 flags, void* err); // +0x28 slot 10
    virtual void Duplicate();                                 // slot 11 (CFile::Duplicate)
    virtual LONG Seek(LONG off, i32 from);                    // +0x30 slot 12
    virtual void SetLength();                                 // slot 13 (CFile::SetLength)
    virtual i32 GetLength();                                  // +0x38 slot 14
    virtual i32 Read(void* buf, u32 n);                       // +0x3c slot 15
    virtual void Write(const void* buf, u32 n);               // +0x40 slot 16
    virtual void LockRange();                                 // slot 17 (CFile::LockRange)
    virtual void UnlockRange();                               // slot 18 (CFile::UnlockRange)
    virtual void Abort();                                     // slot 19 (CFile::Abort)
    virtual void Flush();                                     // slot 20 (CFile::Flush)
    virtual i32 Close(); // +0x54 slot 21 (CFile::Close; FileMem::Reset drives it)
};
SIZE_UNKNOWN(CFileIODispatch); // reinterpret dispatch view (never constructed)

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
SIZE_UNKNOWN(CFileLog); // data-less __thiscall host (never constructed)

#endif // SRC_IO_FILESTREAM_H
