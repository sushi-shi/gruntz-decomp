// FileStream.cpp - CFileIO, the engine's KERNEL32 file-I/O wrapper (MFC CFile
// work-alike). This class gates ALL engine file I/O (RezMgr, WwdFile, save/load).
//
// CFileIO IS the statically-linked NAFXCW MFC CFile (RTTI at 0x1ed15c == CFile):
// its ctor/dtor/Open/Read/Write/Seek/GetPosition/Close/GetLength ARE the MFC
// library bodies (retail RVAs 0x1befd7..0x1bf505), shared by the whole CFile
// family (CMemFile/CMirrorFile call ??0CFile/??1CFile; CRecentFileList/CDocument
// drive the profile+path helpers). Per library policy those bodies are NOT
// hand-reconstructed here - they are carved to config/library_labels.csv (NAFXCW,
// HIGH) and excluded from the match denominator; callers reach them through the
// class's declared-only virtual interface in <Io/FileStream.h> (reloc-masked).
//
// The class MODEL (SIZE/VTBL + CFile's 18-slot virtual interface) stays in the
// header so every caller TU keeps `mov eax,[file]; call [eax+N]` dispatch; only
// the genuine engine method below (ReopenSharedFile) has a reconstructed body.
#include <Io/FileStream.h>
#include <rva.h>

// The engine's ONE static MFC CFile instance at 0x646778: a concrete CFileIO global.
// Canonical DATA pin now lives here (CFileIO's own TU); referenced for its Open
// (0x1bf200 == CFile::Open) + Close (0x1bf426 == CFile::Close); on this concrete-typed
// global both devirtualize to direct, reloc-masked calls.
DATA(0x00246778)
extern CFileIO g_obj646778;

// CFileLog::ReopenSharedFile - reopen the shared file object around a close. Ignores
// `this`; the single stack arg is the path.
// @early-stop
// regalloc-tiebreak wall: both `path` (3x push arg) and `&g_obj646778` (3x ecx) are
// used 3 times; retail pins `path` in esi + re-materializes the global address as an
// immediate each call, cl pins the global address in esi + re-pushes path from the
// stack. Same code shape, opposite callee-saved pick; not source-steerable (~79%).
RVA(0x000bd3e0, 0x34)
void CFileLog::ReopenSharedFile(char* path) {
    g_obj646778.Open(path, 0x1000, 0);
    g_obj646778.Close();
    g_obj646778.Open(path, 1, 0);
}

// -------------------------------------------------------------------------
// 0x0bd430 (spatially re-homed from src/Stub/BoundaryLowerThunks.cpp). Tail-
// forward CFile::Close (0x1bf426) onto the static CFileIO global, devirtualized
// on the concrete-typed global.
RVA(0x000bd430, 0xa)
void CloseFileIOGlobal() {
    g_obj646778.Close();
}

// -------------------------------------------------------------------------
// CFileLog::OpenGruntzLog (0x0bd450, re-homed from src/Stub/BoundaryLowerMethods.cpp):
// close the shared global file then reopen it on the fixed "c:\gruntz.log" debug path.
// The two calls resolve (via ILT thunks 0x3625/0x1983) to the real CloseFileIOGlobal
// (0xbd430) + CFileIO::ReopenSharedFile (0xbd3e0) above; `this` is forwarded to
// ReopenSharedFile, which ignores it. __thiscall, no args.
RVA(0x000bd450, 0x16)
void CFileLog::OpenGruntzLog() {
    CloseFileIOGlobal();
    ReopenSharedFile("c:\\gruntz.log");
}

// Class-metadata (CFileIO / CFileIODispatch) lives atop their decls in FileStream.h.
