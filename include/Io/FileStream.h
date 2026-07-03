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

// External NAFXCW error helpers (reloc-masked no-body calls). The names are
// placeholders pending the alias pass (MFC's real exports are
// AfxThrowFileException / CFileException::ThrowOsError / AfxFullPath).
extern "C" void __stdcall AfxThrowOsError(LONG lOsError, LPCSTR lpszName);
extern "C" void __stdcall AfxThrowFileError(i32 cause, LONG lOsError, LPCSTR lpszName);
extern "C" void __stdcall AfxFullPath(char* lpszPathOut, const char* lpszFileName);

// ---------------------------------------------------------------------------
// CFileIODispatch - a language-forced reinterpret VIEW over a CFileIO's vtable.
// CFileIO's CFile-family slots (Open/Seek/GetLength/Read/Write/GetStatus) are the
// KERNEL32 wrapper's virtuals, but modeling them as real C++ virtuals would make
// cl emit the whole 22-slot ??_7CFileIO (whose unmatched CFile slots point into
// NAFXCW), diverging the vtable. Instead we dispatch through this never-constructed
// dummy-virtual view (no vtable emitted): a call through it lowers to the retail
// `mov eax,[file]; call [eax+N]` with the __thiscall convention for free. Shared by
// CFileIO::GetLength (Seek-to-end) and CFileMem (drives its embedded m_file).
// (docs/patterns/dummy-virtual-slots.md.)
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
    virtual void v11();
    virtual LONG Seek(LONG off, i32 from); // +0x30 slot 12
    virtual void v13();
    virtual i32 GetLength();                    // +0x38 slot 14
    virtual i32 Read(void* buf, u32 n);         // +0x3c slot 15
    virtual void Write(const void* buf, u32 n); // +0x40 slot 16
    virtual void v17();
    virtual void v18();
    virtual void v19();
    virtual void v20();
    virtual i32 Status(); // +0x54 slot 21
};
SIZE_UNKNOWN(CFileIODispatch); // reinterpret dispatch view (never constructed)

// ---------------------------------------------------------------------------
// CFileIO - the KERNEL32 file-stream wrapper (CObject-derived; the base ctor
// stores the CObject vtable, the CFileIO ctor then overwrites it - two-phase
// construction, both stores reloc-masked). The implicit vptr at +0x00 keeps
// m_handle at +0x04.
// ---------------------------------------------------------------------------
SIZE(CFileIO, 0x10);       // vptr + m_handle + m_open + CString (embedded in CFileMem +0x10..+0x20)
VTBL(CFileIO, 0x001ed15c); // derived vtable stamp from ctor 0x1befd7
class CFileIO : public CObject {
public:
    CFileIO();
    CFileIO(HANDLE hFile);
    virtual ~CFileIO() OVERRIDE;

    BOOL Open(const char* lpszFileName, u32 nOpenFlags, void* pError);
    u32 Read(void* lpBuf, u32 nCount);
    void Write(const void* lpBuf, u32 nCount);
    LONG Seek(LONG lOff, i32 nFrom);
    LONG GetPosition();
    // GetLength: virtual Seek-to-end/restore. Declared (no body here) so external
    // callers (CFileImage) emit a reloc-masked `call rel32` to it.
    u32 GetLength();
    void Close();

    HANDLE m_handle; // +0x04
    i32 m_open;      // +0x08
    CString m_name;  // +0x0c

    // Reopen the shared global file object (0x646778) around a close: open(path,
    // 0x1000), close, open(path, 1). Static-like (ignores `this`).
    void ReopenSharedFile(char* path);
};

#endif // SRC_IO_FILESTREAM_H
