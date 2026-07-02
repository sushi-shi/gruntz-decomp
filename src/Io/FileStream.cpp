// FileStream.cpp - CFileIO, the engine's KERNEL32 file-I/O wrapper (MFC CFile
// work-alike). This class gates ALL engine file I/O (RezMgr, WwdFile, save/load).
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
#include <Io/FileStream.h>
#include <rva.h>

// ---------------------------------------------------------------------------
// CFileIO::CFileIO()
// Two-phase MFC construction: base CObject vtable, the CString member's ctor,
// m_handle = -1 (INVALID_HANDLE_VALUE), then the final CFileIO vtable; m_open
// stays 0. The CString ctor under EH installs the unwind frame.
RVA(0x001befd7, 0x40)
CFileIO::CFileIO() {
    m_handle = (HANDLE)-1;
    m_open = 0;
}

// ---------------------------------------------------------------------------
// CFileIO::`scalar deleting destructor' (compiler-generated thunk; no body, so
// it cannot carry an RVA() attribute - pin it by mangled name directly).
// @rva-symbol: ??_GCFileIO@@UAEPAXI@Z 0x001bf017 0x1c

// ---------------------------------------------------------------------------
// CFileIO::~CFileIO()
// Closes the handle if we own one (m_handle != -1 && m_open), then the CString
// member dtor runs and the vtable is restored to base on unwind.
RVA(0x001bf121, 0x4e)
CFileIO::~CFileIO() {
    if (m_handle != (HANDLE)-1 && m_open != 0) {
        Close();
    }
}

// ---------------------------------------------------------------------------
// CFileIO::CFileIO(HANDLE)
// Adopts an existing OS handle: m_open = 0 (we did NOT open it), m_handle = h.
// Same two-phase vtable + CString-member construction + EH frame as the default
// ctor. Used by the handle-duplicating clone path (engine fn).
RVA(0x001bf033, 0x44)
CFileIO::CFileIO(HANDLE hFile) {
    m_open = 0;
    m_handle = hFile;
}

// ---------------------------------------------------------------------------
// CFileIO::Read
// ReadFile(m_handle, buf, n, &n, NULL); throws on failure; returns count read.
// nCount==0 short-circuits to 0 before touching the handle.
RVA(0x001bf328, 0x3a)
u32 CFileIO::Read(void* lpBuf, u32 nCount) {
    if (nCount == 0) {
        return 0;
    }

    // The lpNumberOfBytesRead out-param is &nCount (the parameter slot);
    // taking the address of a local/parameter forces an ebp frame at /O1.
    if (!ReadFile(m_handle, lpBuf, nCount, (LPDWORD)&nCount, 0)) {
        AfxThrowOsError((LONG)GetLastError(), 0);
    }

    return nCount;
}

// ---------------------------------------------------------------------------
// CFileIO::Write
// WriteFile(m_handle, buf, n, &n, NULL); throws OS error on failure, and a
// generic "disk full"/short-write error if fewer than n bytes were written.
// nCount==0 short-circuits (no-op).
RVA(0x001bf362, 0x4b)
void CFileIO::Write(const void* lpBuf, u32 nCount) {
    DWORD nWritten;

    if (nCount == 0) {
        return;
    }

    if (!WriteFile(m_handle, (LPVOID)lpBuf, nCount, &nWritten, 0)) {
        AfxThrowOsError((LONG)GetLastError(), (const char*)m_name);
    }

    if (nWritten != nCount) {
        AfxThrowFileError(0xd /*diskFull*/, (LONG)-1, (const char*)m_name);
    }
}

// ---------------------------------------------------------------------------
// CFileIO::Seek
// SetFilePointer(m_handle, off, NULL, from); throws on -1; returns new pos.
RVA(0x001bf3ad, 0x2f)
LONG CFileIO::Seek(LONG lOff, i32 nFrom) {
    LONG pos = (LONG)SetFilePointer(m_handle, lOff, 0, (DWORD)nFrom);
    if (pos == -1) {
        AfxThrowOsError((LONG)GetLastError(), 0);
    }
    return pos;
}

// ---------------------------------------------------------------------------
// CFileIO::GetPosition
// SetFilePointer(m_handle, 0, NULL, FILE_CURRENT); throws on -1; returns the
// current file position.
RVA(0x001bf3dc, 0x29)
LONG CFileIO::GetPosition() {
    LONG pos = (LONG)SetFilePointer(m_handle, 0, 0, 1 /*FILE_CURRENT*/);
    if (pos == -1) {
        AfxThrowOsError((LONG)GetLastError(), 0);
    }
    return pos;
}

// ---------------------------------------------------------------------------
// CFileIO::Close
// CloseHandle(m_handle) if open; reset m_handle = -1, m_open = 0, empty m_name;
// then throw the OS error if CloseHandle failed.
RVA(0x001bf426, 0x41)
void CFileIO::Close() {
    BOOL failed = 0;
    if (m_handle != (HANDLE)-1) {
        failed = (CloseHandle(m_handle) == 0);
    }

    m_handle = (HANDLE)-1;
    m_open = 0;
    m_name.Empty();

    if (failed) {
        AfxThrowOsError((LONG)GetLastError(), 0);
    }
}

// The CFileException the failure path fills in: m_cause@+0x8, m_lOsError@+0xc,
// m_strFileName (CString)@+0x10. The os-error -> exception-cause mapper is the
// NAFXCW static helper (external no-body callee, reloc-masked).
struct CFileExceptionLite {
    char pad0[8];          // +0x00  (CObject vtable + base)
    i32 m_cause;           // +0x08
    LONG m_lOsError;       // +0x0c
    CString m_strFileName; // +0x10
};
extern "C" i32 __stdcall AfxOsErrorToException(LONG lOsError);

// A minimal SECURITY_ATTRIBUTES (we don't pull in <windows.h>).
struct SecurityAttributes {
    DWORD nLength;              // +0x00 (== 0xc)
    void* lpSecurityDescriptor; // +0x04
    BOOL bInheritHandle;        // +0x08
};

// ---------------------------------------------------------------------------
// CFileIO::Open
// CreateFileA() with MFC nOpenFlags -> access/share/disposition translation;
// stores the HANDLE; on failure fills the CFileException* (pError) if non-null
// and returns FALSE. Returns nonzero on success.
RVA(0x001bf200, 0x128)
BOOL CFileIO::Open(const char* lpszFileName, u32 nOpenFlags, void* pError) {
    char szPath[0x104]; // MAX_PATH (260) - frame is 0x110 with the SA local

    m_open = 0;
    m_handle = (HANDLE)-1;
    nOpenFlags &= 0xFFFF7FFF; // clear typeText/typeBinary bit
    m_name.Empty();

    AfxFullPath(szPath, lpszFileName);
    m_name = szPath;

    DWORD dwAccess;
    switch (nOpenFlags & 3) {
        case 0:
            dwAccess = 0x80000000;
            break; // GENERIC_READ
        case 1:
            dwAccess = 0x40000000;
            break; // GENERIC_WRITE
        case 2:
            dwAccess = 0xC0000000;
            break; // GENERIC_READ|GENERIC_WRITE
    }

    DWORD dwShare;
    switch (nOpenFlags & 0x70) {
        case 0x00:
        case 0x10:
            dwShare = 0;
            break;
        case 0x20:
            dwShare = 1;
            break; // FILE_SHARE_READ
        case 0x30:
            dwShare = 2;
            break; // FILE_SHARE_WRITE
        case 0x40:
            dwShare = 3;
            break; // FILE_SHARE_READ|WRITE
        default:
            dwShare = (DWORD)lpszFileName;
            break;
    }

    SecurityAttributes sa;
    sa.nLength = 0xc;
    sa.lpSecurityDescriptor = 0;
    sa.bInheritHandle = ((~nOpenFlags) >> 7) & 1; // !(nOpenFlags & modeNoInherit)

    DWORD dwCreate;
    if (nOpenFlags & 0x1000) {                    // modeCreate
        dwCreate = (nOpenFlags & 0x2000) ? 4 : 2; // OPEN_ALWAYS : CREATE_ALWAYS
    } else {
        dwCreate = 3; // OPEN_EXISTING
    }

    HANDLE h =
        CreateFileA(lpszFileName, dwAccess, dwShare, (LPSECURITY_ATTRIBUTES)&sa, dwCreate, 0x80, 0);
    if (h == (HANDLE)-1) {
        CFileExceptionLite* pe = (CFileExceptionLite*)pError;
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

// ---------------------------------------------------------------------------
// CFileIO::GetLength
// Snapshot the current position (Seek 0,current), seek to the end for the length
// (Seek 0,end), then restore (Seek cur,begin); return the length. Seek is the
// CFile-family virtual at vtable slot 12 (+0x30) - dispatched through a small
// placeholder-virtual view (slots 0-11 fix the offset) so each Seek reloads the
// vptr (matching the retail virtual call) without disturbing CFileIO's own
// reloc-masked vtable.
class CFileIODispatch {
public:
    virtual void Slot0();
    virtual void Slot1();
    virtual void Slot2();
    virtual void Slot3();
    virtual void Slot4();
    virtual void Slot5();
    virtual void Slot6();
    virtual void Slot7();
    virtual void Slot8();
    virtual void Slot9();
    virtual void Slot10();
    virtual void Slot11();
    virtual LONG Seek(LONG off, i32 from); // +0x30  slot 12
};
RVA(0x001bf505, 0x2d)
u32 CFileIO::GetLength() {
    CFileIODispatch* f = (CFileIODispatch*)this;
    LONG cur = f->Seek(0, 1 /*current*/);
    LONG len = f->Seek(0, 2 /*end*/);
    f->Seek(cur, 0 /*begin*/);
    return (u32)len;
}

// -------------------------------------------------------------------------
// Engine-label backlog stubs.
// -------------------------------------------------------------------------
// The shared global file object at 0x646778 (bound as C646778 in
// BoundaryLowerThunks.cpp). Referenced here for its Open (0x1bf200 == CFileIO::Open)
// + close (0x1bf426); both calls are reloc-masked rel32.
struct C646778 {
    BOOL Open(char* path, u32 flags, void* pError); // 0x1bf200
    void M1bf426();                                 // 0x1bf426
};
extern C646778 g_obj646778;

// CFileIO::Stub_0bd3e0 - reopen the shared file object around a close. Ignores
// `this`; the single stack arg is the path.
// @early-stop
// regalloc-tiebreak wall: both `path` (3x push arg) and `&g_obj646778` (3x ecx) are
// used 3 times; retail pins `path` in esi + re-materializes the global address as an
// immediate each call, cl pins the global address in esi + re-pushes path from the
// stack. Same code shape, opposite callee-saved pick; not source-steerable (~79%).
RVA(0x000bd3e0, 0x34)
void CFileIO::Stub_0bd3e0(char* path) {
    g_obj646778.Open(path, 0x1000, 0);
    g_obj646778.M1bf426();
    g_obj646778.Open(path, 1, 0);
}

// ===========================================================================
// Class-metadata annotations (EOF-hosted: FileStream.h is pulled into /O1+/O2
// consumers incl. Image, and this /O1 TU is byte-exact-sensitive, so keep every
// completeness typedef after the last function body).
// ===========================================================================
SIZE_UNKNOWN(CFileExceptionLite);
SIZE(SecurityAttributes, 0xc); // complete Win32 SECURITY_ATTRIBUTES (nLength=0xc)
SIZE_UNKNOWN(CFileIODispatch); // CFile Seek slot-dispatch view (no emitted vtable)
