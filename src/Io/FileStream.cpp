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

// -------------------------------------------------------------------------
// The shared global file object at 0x646778 (bound as C646778 in
// BoundaryLowerThunks.cpp). Referenced here for its Open (0x1bf200 == the MFC
// CFile::Open) + close (0x1bf426 == CFile::Close); both calls are reloc-masked.
struct C646778 {
    BOOL Open(char* path, u32 flags, void* pError); // 0x1bf200
    void M1bf426();                                 // 0x1bf426
};
extern C646778 g_obj646778;

// CFileIO::ReopenSharedFile - reopen the shared file object around a close. Ignores
// `this`; the single stack arg is the path.
// @early-stop
// regalloc-tiebreak wall: both `path` (3x push arg) and `&g_obj646778` (3x ecx) are
// used 3 times; retail pins `path` in esi + re-materializes the global address as an
// immediate each call, cl pins the global address in esi + re-pushes path from the
// stack. Same code shape, opposite callee-saved pick; not source-steerable (~79%).
RVA(0x000bd3e0, 0x34)
void CFileIO::ReopenSharedFile(char* path) {
    g_obj646778.Open(path, 0x1000, 0);
    g_obj646778.M1bf426();
    g_obj646778.Open(path, 1, 0);
}

// Class-metadata (CFileIO / CFileIODispatch) lives atop their decls in FileStream.h.
