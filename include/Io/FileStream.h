// FileStream.h - CFileIO, the engine's KERNEL32 file-I/O wrapper class (the
// MFC CFile work-alike that gates ALL engine file I/O: RezMgr, WwdFile, the
// save/load path). Minimal Monolith-faithful reconstruction sufficient to
// byte-match the leaf I/O methods. Field names are placeholders (m_<hexoffset>);
// only the OFFSETS + code bytes are load-bearing (campaign doctrine).
//
//   +0x00  m_vtbl   : vtable pointer (CObject-derived, two-phase ctor).
//   +0x04  m_handle : Win32 HANDLE (INVALID_HANDLE_VALUE = -1 when closed).
//   +0x08  m_open   : owns-handle flag (1 when this object opened the handle).
//   +0x0c  m_name   : the file name (an MFC CString - a single char* pointer
//                     whose ctor/dtor/assign/Empty are NAFXCW library calls,
//                     modeled here as an external string type so the calls are
//                     reloc-masked in objdiff).
#ifndef SRC_IO_FILESTREAM_H
#define SRC_IO_FILESTREAM_H

// ---------------------------------------------------------------------------
// Minimal Win32 surface - we do NOT pull in <windows.h> (keep the visible
// symbol set small; the compiler hashes it, so entropy follows header churn).
// Only the KERNEL32 imports the matched methods call, declared as a
// __declspec(dllimport) __stdcall block (reproduces the FF15 [IAT] form).
// ---------------------------------------------------------------------------
typedef int                 BOOL;
typedef unsigned long       DWORD;
typedef void *              HANDLE;
typedef const char *        LPCSTR;
typedef void *              LPVOID;
typedef DWORD *             LPDWORD;
typedef long                LONG;

struct _SECURITY_ATTRIBUTES;
typedef struct _SECURITY_ATTRIBUTES *LPSECURITY_ATTRIBUTES;
struct _OVERLAPPED;
typedef struct _OVERLAPPED *LPOVERLAPPED;

extern "C" {
__declspec(dllimport) HANDLE __stdcall CreateFileA(
    LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
__declspec(dllimport) BOOL __stdcall ReadFile(
    HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead,
    LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped);
__declspec(dllimport) BOOL __stdcall WriteFile(
    HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToWrite,
    LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);
__declspec(dllimport) DWORD __stdcall SetFilePointer(
    HANDLE hFile, LONG lDistanceToMove, LONG *lpDistanceToMoveHigh,
    DWORD dwMoveMethod);
__declspec(dllimport) BOOL __stdcall CloseHandle(HANDLE hObject);
__declspec(dllimport) DWORD __stdcall GetLastError(void);
}

// ---------------------------------------------------------------------------
// External NAFXCW helpers, modeled with NO body so their `call rel32`
// displacements are reloc-masked in objdiff (the "external no-body callee"
// idiom). Calling-convention/arg-shape pinned from the disasm.
//   - CString: a 4-byte CString (single char* @+0). Its ctor/dtor/Empty/
//     operator= are the engine's CString helpers.
//   - ThrowOsError(lOsError, pszName): CFileException::ThrowOsError.
//   - ThrowGenericError(cause, lOsError, pszName): the 3-arg throw.
//   - AfxFullPath(dst, src): the long-path canonicalizer.
// ---------------------------------------------------------------------------
#include <incs/CString.h>

extern "C" void __stdcall AfxThrowOsError(LONG lOsError, LPCSTR lpszName);
extern "C" void __stdcall AfxThrowFileError(int cause, LONG lOsError, LPCSTR lpszName);
extern "C" void __stdcall AfxFullPath(char *lpszPathOut, const char *lpszFileName);

// ---------------------------------------------------------------------------
// CFileIO - the KERNEL32 file-stream wrapper.
// ---------------------------------------------------------------------------
// CObject - the MFC root base. Polymorphic (its own vtable), no data
// members, so CFileIO's first field (m_handle) stays at +0x04. The base ctor
// stores the CObject vtable; the CFileIO ctor then overwrites it with the
// derived vtable (two-phase construction, both stores reloc-masked).
class CObject {
public:
    CObject() {}
    virtual ~CObject() {}
};

class CFileIO : public CObject {
public:
    CFileIO();
    CFileIO(HANDLE hFile);
    virtual ~CFileIO();

    BOOL  Open(const char *lpszFileName, unsigned int nOpenFlags, void *pError);
    unsigned int Read(void *lpBuf, unsigned int nCount);
    void  Write(const void *lpBuf, unsigned int nCount);
    LONG  Seek(LONG lOff, int nFrom);
    LONG  GetPosition();
    // GetLength: virtual Seek-to-end/restore. Declared (no body here)
    // so external callers (CFileImage) emit a reloc-masked `call rel32` to it.
    unsigned int GetLength();
    void  Close();

    // The vtable pointer at +0x00 is implicit (virtual ~CFileIO()); do NOT
    // declare it as an explicit member or it shifts every field by 4.
    HANDLE       m_handle;   // +0x04
    int          m_open;     // +0x08
    CString    m_name;     // +0x0c

    // Engine-label backlog stubs.
    void Stub_0bd3e0();
    void Stub_0e5550();
    void Stub_0e5700();
};

#endif // SRC_IO_FILESTREAM_H
