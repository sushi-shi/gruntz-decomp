// FileStream.h - CFileIO, the engine's KERNEL32 file-I/O wrapper (a CObject-
// derived MFC CFile work-alike that gates ALL engine file I/O: RezMgr, WwdFile,
// the save/load path). Minimal Monolith-faithful reconstruction sufficient to
// byte-match the leaf I/O methods. Field names are placeholders (m_<hexoffset>);
// only the OFFSETS + code bytes are load-bearing (campaign doctrine).
//
//   +0x00  m_vtbl   : vtable pointer (CObject-derived, two-phase ctor).
//   +0x04  m_handle : Win32 HANDLE (INVALID_HANDLE_VALUE = -1 when closed).
//   +0x08  m_open   : owns-handle flag (1 when this object opened the handle).
//   +0x0c  m_name   : the file name (MFC CString).
#ifndef SRC_IO_FILESTREAM_H
#define SRC_IO_FILESTREAM_H
#include <rva.h> // OVERRIDE macro (override under clang, no-op under MSVC 5.0)

#include <Mfc.h> // CObject, CString + <windows.h> (CreateFileA/ReadFile/...) FIRST

// ---------------------------------------------------------------------------
// CFileIODispatch - a language-forced reinterpret VIEW over a CFileIO's vtable,
// used to FORCE a virtual dispatch on a CONCRETE embedded CFileIO. CFileIO now
// declares its CFile-family slots as real virtuals (below), so a call through a
// CFileIO* (unknown dynamic type) already lowers to `mov eax,[file]; call [eax+N]`
// - CFileIO::GetLength uses that directly. But CFileMem drives its embedded
// `CFileIO m_file` MEMBER, whose dynamic type cl knows exactly, so `m_file.Open()`
// would DEVIRTUALIZE to a direct call. Dispatching through this never-constructed
// dummy-virtual view (cl can't reason about its dynamic type) keeps the retail
// virtual dispatch. Kept for CFileMem. (docs/patterns/dummy-virtual-slots.md.)
// DISSOLUTION TESTED 2026-07-19: `CFile* f = &m_file; f->Open(..)` held bytes at
// CFileMem::Open, but converting ALL sites + deleting this view cost -4 exact
// (deterministic) - the shim is LOAD-BEARING for the other dispatch sites; keep.
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

// ---------------------------------------------------------------------------
// CFileIO IS the statically-linked NAFXCW MFC CFile - the RTTI at 0x1ed15c IS CFile, and
// its ctor/dtor/Open/Seek/GetPosition/GetLength/Read/Write/Close ARE the library bodies at
// 0x1befd7..0x1bf505. So it is not "a CFile work-alike": it is CFile, and it is now spelled
// that way. `CFileIO` survives ONLY as an alias, so the ~20 consumer TUs read unchanged.
//
// WHY THIS HAD TO CHANGE (assert_relocs --fake-targets): the old model re-DECLARED CFile's
// whole interface on a class of its own name and left every body undefined "because the
// library owns them". That is fine for objdiff (relocs are masked) but it is a LINK BREAK:
// the calls mangled as ?Open@CFileIO@@UAEHPBDIPAX@Z / ??0CFileIO@@QAE@XZ / ??1CFileIO@@UAE@XZ,
// and NAFXCW.LIB defines ?Open@CFile@@... / ??0CFile@@... - 42 guaranteed `unresolved external
// symbol`s. Aliasing the REAL class makes every one of them resolve, and is codegen-identical
// because it is the same class: same 0x10 layout, same 23-slot vtable, same slot order.
// (Exactly the CTmObList->CPtrList / CTmByteArray->CByteArray fold.)
//
// No SIZE()/VTBL() annotation any more: the layout and the ??_7 belong to the library now.
typedef CFile CFileIO;

// The two GENUINE engine helpers that live in this TU (0xbd3e0 / 0xbd450). They drive the
// shared global file object and IGNORE `this` entirely, so their host carries no data and no
// vtable - only the __thiscall convention and the mangled name are load-bearing, and nothing
// outside FileStream.cpp calls them. They used to hang off CFileIO; they cannot hang off the
// real CFile (that is MFC's class), so they get a data-less host of their own.
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

// --- vtable catalog ---

#endif // SRC_IO_FILESTREAM_H
