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
struct CFileIODispatch {
    virtual void v0();
    virtual void v1();
    virtual void v2();
    virtual void v3();
    virtual void v4();
    virtual void v5();
    virtual void v6();
    virtual void v7();
    virtual void v8();
    virtual void v9();
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
// CFileIO - the KERNEL32 file-stream wrapper (CObject-derived; the base ctor
// stores the CObject vtable, the CFileIO ctor then overwrites it - two-phase
// construction, both stores reloc-masked). The implicit vptr at +0x00 keeps
// m_handle at +0x04.
// ---------------------------------------------------------------------------
SIZE(CFileIO, 0x10);       // vptr + m_handle + m_open + CString
VTBL(CFileIO, 0x001ed15c); // ??_7CFileIO@@6B@, stamped by the ctor 0x1befd7
// CFileIO IS MFC CFile (RTTI at 0x1ed15c == CFile): a faithful reconstruction of the
// statically-linked NAFXCW CFile. Modeled `: public CObject` (NOT `: public CFile`),
// with CFile's full 18-virtual interface declared HERE, because the class IS CFile -
// its ctor/dtor (0x1befd7/0x1bf121) INLINE the CObject-base vptr stamp with NO
// `call CFile::CFile`, so deriving from the real MFC CFile would wrongly emit a
// base-ctor call and break the 100% ctor/dtor. Declaring CFile's virtuals directly
// reproduces the 23-slot ??_7CFileIO vtable (CObject's 5 slots + CFile's 18) from
// cl's own emission: the reconstructed slots (dtor/GetPosition/Open/Seek/GetLength/
// Read/Write/Close) bind to our bodies; the un-reconstructed CFile-library slots
// (GetFileName/.../GetBufferPtr) are declared-only (external NAFXCW, reloc-masked).
// Slot order is CFile's declaration order (afx.h) - load-bearing.
class CFileIO : public CObject {
public:
    CFileIO();
    CFileIO(HANDLE hFile);
    virtual CRuntimeClass* GetRuntimeClass() const OVERRIDE; // [0] afx CObject slot 0
    virtual ~CFileIO() OVERRIDE; // [1] 0x1bf121 (overrides CObject dtor slot)

    // --- CFile virtual interface, vtable slots [5..22] in CFile declaration order --
    virtual LONG GetPosition();                                                   // [5]  0x1bf3dc
    virtual CString GetFileName() const;                                          // [6]  extern
    virtual CString GetFileTitle() const;                                         // [7]  extern
    virtual CString GetFilePath() const;                                          // [8]  extern
    virtual void SetFilePath(const char* lpszNewName);                            // [9]  extern
    virtual BOOL Open(const char* lpszFileName, u32 nOpenFlags, void* pError);    // [10] 0x1bf200
    virtual CFile* Duplicate() const;                                             // [11] extern
    virtual LONG Seek(LONG lOff, i32 nFrom);                                      // [12] 0x1bf3ad
    virtual void SetLength(u32 dwNewLen);                                         // [13] extern
    virtual u32 GetLength();                                                      // [14] 0x1bf505
    virtual u32 Read(void* lpBuf, u32 nCount);                                    // [15] 0x1bf328
    virtual void Write(const void* lpBuf, u32 nCount);                            // [16] 0x1bf362
    virtual void LockRange(u32 dwPos, u32 dwCount);                               // [17] extern
    virtual void UnlockRange(u32 dwPos, u32 dwCount);                             // [18] extern
    virtual void Abort();                                                         // [19] extern
    virtual void Flush();                                                         // [20] extern
    virtual void Close();                                                         // [21] 0x1bf426
    virtual u32 GetBufferPtr(u32 nCmd, u32 nCount, void** ppStart, void** ppMax); // [22] extern

    HANDLE m_handle; // +0x04  (CFile::m_hFile)
    i32 m_open;      // +0x08  (CFile::m_bCloseOnDelete)
    CString m_name;  // +0x0c  (CFile::m_strFileName)

    // Reopen the shared global file object (0x646778) around a close: open(path,
    // 0x1000), close, open(path, 1). Static-like (ignores `this`). Non-virtual.
    void ReopenSharedFile(char* path);
};

// --- vtable catalog ---

#endif // SRC_IO_FILESTREAM_H
