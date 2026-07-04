#ifndef IO_SHAREDFILEOBJ_H
#define IO_SHAREDFILEOBJ_H
#include <Ints.h>
#include <rva.h>
// NOTE: BOOL comes from the including TU's <Mfc.h>/<Win32.h> (both consumers --
// FileStream.cpp via <Io/FileStream.h>, BoundaryLowerThunks.cpp -- pull it first).

// The shared global file object at 0x646778: the engine's one static MFC CFile
// instance (CFileIO == the NAFXCW CFile; see FileStream.cpp). Its methods are the
// CFile library bodies (Open 0x1bf200, Close 0x1bf426, teardown 0x1befd7), reloc-
// masked at every call.  Unified here -- it was independently redefined in
// FileStream.cpp + BoundaryLowerThunks.cpp.  The canonical DATA(0x00246778) binding
// stays owned by BoundaryLowerThunks.cpp; each TU re-declares the extern locally.
SIZE_UNKNOWN(C646778);
struct C646778 {
    BOOL Open(char* path, u32 flags, void* pError); // 0x1bf200 (CFile::Open)
    void M1befd7();                                 // 0x1befd7 (CFile-family teardown)
    void M1bf426();                                 // 0x1bf426 (CFile::Close)
};

#endif // IO_SHAREDFILEOBJ_H
