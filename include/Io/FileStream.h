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
// CFileIO - the KERNEL32 file-stream wrapper (CObject-derived; the base ctor
// stores the CObject vtable, the CFileIO ctor then overwrites it - two-phase
// construction, both stores reloc-masked). The implicit vptr at +0x00 keeps
// m_handle at +0x04.
// ---------------------------------------------------------------------------
SIZE_UNKNOWN(CFileIO);
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

    // Engine-label backlog stubs.
    void Stub_0bd3e0();
    void Stub_0e5550();
    void Stub_0e5700();
};

#endif // SRC_IO_FILESTREAM_H
