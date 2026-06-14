// FileStream.cpp - CFileIO, the engine's KERNEL32 file-I/O wrapper (MFC CFile
// work-alike). This class gates ALL engine file I/O (RezMgr, WwdFile, save/load).
//
// Functions matched in this TU (RVAs in GRUNTZ.EXE); 9/10 BYTE-EXACT:
//   CFileIO::CFileIO()            @ 0x1befd7  (64 B,  thiscall ret)    - ctor
//   CFileIO::CFileIO(HANDLE)      @ 0x1bf033  (68 B,  thiscall ret 4)  - adopt-handle ctor
//   CFileIO::`scalar deleting dtor` @ 0x1bf017 (28 B, thiscall ret 4)  - ??_G
//   CFileIO::~CFileIO()           @ 0x1bf121  (78 B,  thiscall ret)    - dtor
//   CFileIO::Read                 @ 0x1bf328  (58 B,  thiscall ret 8)  - ReadFile
//   CFileIO::Write                @ 0x1bf362  (75 B,  thiscall ret 8)  - WriteFile
//   CFileIO::Seek                 @ 0x1bf3ad  (47 B,  thiscall ret 8)  - SetFilePointer
//   CFileIO::GetPosition         @ 0x1bf3dc  (41 B,  thiscall ret)    - SetFilePointer
//   CFileIO::Close                @ 0x1bf426  (65 B,  thiscall ret)    - CloseHandle
//   CFileIO::Open                 @ 0x1bf200  (296 B, thiscall ret c)  - CreateFileA
//                                   (95.5%: one share-switch case-body layout
//                                    coin-flip remains; logic/control-flow exact)
//
// All KERNEL32 calls go through the IAT (FF15 [slot]); the NAFXCW helpers
// (CObject base vtable + CString ctor/dtor/assign/Empty, the throw helpers, the
// path canonicalizer) are modeled as external/no-body callees so their reloc
// operands are masked in objdiff. The CString member (a class with a dtor) makes
// the ctor/dtor carry a C++ EH frame (MSVC5 __EH_prolog + FuncInfo, fs:0) -> /GX.
//
// BUILT WITH /O1 (optimize for SIZE), NOT the locked /O2: this is MFC-derived
// (CFile) code; MFC ships /O1, which pushes memory operands directly and keeps
// ebp frames for address-taken locals (Read/Write/Open) - behaviors /O2 (favor
// speed) drops. /O2 plateaus these in the 60s-80s%; /O1 takes them byte-exact.
#include "FileStream.h"

// ---------------------------------------------------------------------------
// CFileIO::CFileIO()
// Two-phase MFC construction: base CObject vtable, the CString member's ctor,
// m_handle = -1 (INVALID_HANDLE_VALUE), then the final CFileIO vtable; m_open
// stays 0. The CString ctor under EH installs the unwind frame.
//
// @address: 0x1befd7
// @size:    0x40
// ---------------------------------------------------------------------------
CFileIO::CFileIO()
{
    m_handle = (HANDLE)-1;
    m_open = 0;
}

// ---------------------------------------------------------------------------
// CFileIO::`scalar deleting destructor' (compiler-generated thunk; no body).
//
// @address: 0x1bf017
// @size:    0x1c
// @symbol:  ??_GCFileIO@@UAEPAXI@Z
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// CFileIO::~CFileIO()
// Closes the handle if we own one (m_handle != -1 && m_open), then the CString
// member dtor runs and the vtable is restored to base on unwind.
//
// @address: 0x1bf121
// @size:    0x4e
// ---------------------------------------------------------------------------
CFileIO::~CFileIO()
{
    if (m_handle != (HANDLE)-1 && m_open != 0)
        Close();
}

// ---------------------------------------------------------------------------
// CFileIO::CFileIO(HANDLE)
// Adopts an existing OS handle: m_open = 0 (we did NOT open it), m_handle = h.
// Same two-phase vtable + CString-member construction + EH frame as the default
// ctor. Used by the handle-duplicating clone path (engine fn @0x1bf16f).
//
// @address: 0x1bf033
// @size:    0x44
// ---------------------------------------------------------------------------
CFileIO::CFileIO(HANDLE hFile)
{
    m_open = 0;
    m_handle = hFile;
}

// ---------------------------------------------------------------------------
// CFileIO::Read
// ReadFile(m_handle, buf, n, &n, NULL); throws on failure; returns count read.
// nCount==0 short-circuits to 0 before touching the handle.
//
// @address: 0x1bf328
// @size:    0x3a
// ---------------------------------------------------------------------------
unsigned int CFileIO::Read(void *lpBuf, unsigned int nCount)
{
    if (nCount == 0)
        return 0;

    // The lpNumberOfBytesRead out-param is &nCount (the parameter slot);
    // taking the address of a local/parameter forces an ebp frame at /O1.
    if (!ReadFile(m_handle, lpBuf, nCount, (LPDWORD)&nCount, 0))
        AfxThrowOsError((LONG)GetLastError(), 0);

    return nCount;
}

// ---------------------------------------------------------------------------
// CFileIO::Write
// WriteFile(m_handle, buf, n, &n, NULL); throws OS error on failure, and a
// generic "disk full"/short-write error if fewer than n bytes were written.
// nCount==0 short-circuits (no-op).
//
// @address: 0x1bf362
// @size:    0x4b
// ---------------------------------------------------------------------------
void CFileIO::Write(const void *lpBuf, unsigned int nCount)
{
    DWORD nWritten;

    if (nCount == 0)
        return;

    if (!WriteFile(m_handle, (LPVOID)lpBuf, nCount, &nWritten, 0))
        AfxThrowOsError((LONG)GetLastError(), (const char *)m_name);

    if (nWritten != nCount)
        AfxThrowFileError(0xd /*diskFull*/, (LONG)-1, (const char *)m_name);
}

// ---------------------------------------------------------------------------
// CFileIO::Seek
// SetFilePointer(m_handle, off, NULL, from); throws on -1; returns new pos.
//
// @address: 0x1bf3ad
// @size:    0x2f
// ---------------------------------------------------------------------------
LONG CFileIO::Seek(LONG lOff, int nFrom)
{
    LONG pos = (LONG)SetFilePointer(m_handle, lOff, 0, (DWORD)nFrom);
    if (pos == -1)
        AfxThrowOsError((LONG)GetLastError(), 0);
    return pos;
}

// ---------------------------------------------------------------------------
// CFileIO::GetPosition
// SetFilePointer(m_handle, 0, NULL, FILE_CURRENT); throws on -1; returns the
// current file position.
//
// @address: 0x1bf3dc
// @size:    0x29
// ---------------------------------------------------------------------------
LONG CFileIO::GetPosition()
{
    LONG pos = (LONG)SetFilePointer(m_handle, 0, 0, 1 /*FILE_CURRENT*/);
    if (pos == -1)
        AfxThrowOsError((LONG)GetLastError(), 0);
    return pos;
}

// ---------------------------------------------------------------------------
// CFileIO::Close
// CloseHandle(m_handle) if open; reset m_handle = -1, m_open = 0, empty m_name;
// then throw the OS error if CloseHandle failed.
//
// @address: 0x1bf426
// @size:    0x41
// ---------------------------------------------------------------------------
void CFileIO::Close()
{
    BOOL failed = 0;
    if (m_handle != (HANDLE)-1)
        failed = (CloseHandle(m_handle) == 0);

    m_handle = (HANDLE)-1;
    m_open = 0;
    m_name.Empty();

    if (failed)
        AfxThrowOsError((LONG)GetLastError(), 0);
}

// The CFileException the failure path fills in: m_cause@+0x8, m_lOsError@+0xc,
// m_strFileName (CString)@+0x10. The os-error -> exception-cause mapper is the
// NAFXCW static helper at 0x1c1a71 (external no-body callee, reloc-masked).
struct CFileExceptionLite {
    char      pad0[8];   // +0x00  (CObject vtable + base)
    int       m_cause;   // +0x08
    LONG      m_lOsError;// +0x0c
    AfxString m_strFileName; // +0x10
};
extern "C" int __stdcall AfxOsErrorToException(LONG lOsError);

// A minimal SECURITY_ATTRIBUTES (we don't pull in <windows.h>).
struct SecurityAttributes {
    DWORD  nLength;              // +0x00 (== 0xc)
    void  *lpSecurityDescriptor;// +0x04
    BOOL   bInheritHandle;      // +0x08
};

// ---------------------------------------------------------------------------
// CFileIO::Open
// CreateFileA() with MFC nOpenFlags -> access/share/disposition translation;
// stores the HANDLE; on failure fills the CFileException* (pError) if non-null
// and returns FALSE. Returns nonzero on success.
//
// @address: 0x1bf200
// @size:    0x128
// ---------------------------------------------------------------------------
BOOL CFileIO::Open(const char *lpszFileName, unsigned int nOpenFlags, void *pError)
{
    char szPath[0x104];  // MAX_PATH (260) - frame is 0x110 with the SA local

    m_open = 0;
    m_handle = (HANDLE)-1;
    nOpenFlags &= 0xFFFF7FFF;       // clear typeText/typeBinary bit
    m_name.Empty();

    AfxFullPath(szPath, lpszFileName);
    m_name = szPath;

    DWORD dwAccess;
    switch (nOpenFlags & 3) {
    case 0:  dwAccess = 0x80000000; break;  // GENERIC_READ
    case 1:  dwAccess = 0x40000000; break;  // GENERIC_WRITE
    case 2:  dwAccess = 0xC0000000; break;  // GENERIC_READ|GENERIC_WRITE
    }

    DWORD dwShare;
    switch (nOpenFlags & 0x70) {
    case 0x00:
    case 0x10: dwShare = 0; break;
    case 0x20: dwShare = 1; break;          // FILE_SHARE_READ
    case 0x30: dwShare = 2; break;          // FILE_SHARE_WRITE
    case 0x40: dwShare = 3; break;          // FILE_SHARE_READ|WRITE
    default:   dwShare = (DWORD)lpszFileName; break;
    }

    SecurityAttributes sa;
    sa.nLength = 0xc;
    sa.lpSecurityDescriptor = 0;
    sa.bInheritHandle = ((~nOpenFlags) >> 7) & 1;  // !(nOpenFlags & modeNoInherit)

    DWORD dwCreate;
    if (nOpenFlags & 0x1000) {              // modeCreate
        dwCreate = (nOpenFlags & 0x2000) ? 4 : 2;  // OPEN_ALWAYS : CREATE_ALWAYS
    } else {
        dwCreate = 3;                       // OPEN_EXISTING
    }

    HANDLE h = CreateFileA(lpszFileName, dwAccess, dwShare,
                           (LPSECURITY_ATTRIBUTES)&sa, dwCreate, 0x80, 0);
    if (h == (HANDLE)-1) {
        CFileExceptionLite *pe = (CFileExceptionLite *)pError;
        if (pe != 0) {
            pe->m_lOsError = (LONG)GetLastError();
            pe->m_cause = AfxOsErrorToException(pe->m_lOsError);
            pe->m_strFileName = lpszFileName;
        }
        return 0;
    }

    m_handle = h;
    m_open = 1;
    return 1;
}
