// FilePathInfo.cpp - the Win32 file-path/time-info utility family (graduated from
// src/Stub/Backlog.cpp). Three __stdcall free helpers that canonicalize a path
// and snapshot/restore a file's timestamps + attributes:
//
//   CanonicalizePath   (0x1bf8f8) - GetFullPathName + volume-info-driven case
//                                   normalization; FindFirstFile fixes the file
//                                   part's on-disk casing. /GX (an MFC CString
//                                   temp holds the extracted volume root).
//   GetFileTimeInfo    (0x1c1609) - canonicalize into out->path, FindFirstFile,
//                                   snapshot size + attrs + the three file times
//                                   (converted to engine CTime via 0x1b3065).
//   RestoreFileTimeInfo(0x1c176a) - clear the read-only bit, reopen, SetFileTime
//                                   from the snapshot, restore the attributes.
//
// Every callee is a reloc-masked external (the CString ctor/dtor, the engine
// CTime<->FILETIME converters 0x1b3065/0x1c16bb, the error thunk 0x1c18b7, and
// the sibling ExtractRoot 0x1bf9d1); the Win32 imports come from the real headers.
// Only offsets / code bytes are load-bearing (campaign doctrine).

#include <Mfc.h> // CString (the /GX volume-root temp) + Win32 (GetFullPathName etc.)

#include <rva.h>

// The file-info snapshot record (GetFileTimeInfo out-param / RestoreFileTimeInfo
// in-param). The three times are engine CTime values (a packed time_t); the path
// buffer at +0x12 receives the canonicalized full path.
struct FileInfoRec {
    i32 m_0;            // +0x00  creation time  (CTime)
    i32 m_4;            // +0x04  last-write time (CTime)
    i32 m_8;            // +0x08  last-access time (CTime)
    u32 m_c;            // +0x0c  file size (low DWORD)
    u8 m_10;            // +0x10  attributes & 0x7f
    u8 m_11;            // +0x11  (pad)
    char m_path[0x104]; // +0x12  canonicalized full path
};

// A 4-byte scratch CTime built from a FILETIME. FromFileTime (0x1b3065,
// __thiscall) converts ft to local system time then constructs the CTime into
// m_value, returning &m_value (so the caller reads the value with a single deref).
struct EngCTimeTmp {
    i32 m_value;
    i32* FromFileTime(const FILETIME* ft, i32 dst); // 0x1b3065 __thiscall
};

// The sibling volume-root extractor: fills `out` (a CString) with the drive/root
// of `path`. FUN 0x1bf9d1, __stdcall.
void __stdcall ExtractRoot(char* path, CString* out);
// CTime -> FILETIME converter (0x1c16bb, __cdecl): decompose the CTime at *ct into
// a SYSTEMTIME and produce the UTC FILETIME in *out.
void __cdecl CTimeToFileTime(const i32* ct, FILETIME* out);
// The shared Win32-error thunk (0x1c18b7, __stdcall): if err != 0, format + log it.
void __stdcall FileErrThunk(i32 err, i32 ctx);

// @source: import:FindFirstFileA
// CanonicalizePath - resolve `path` to a full path in `buf` (MAX_PATH), then fix
// the on-disk casing: uppercase when the volume does not preserve case, and copy
// the FindFirstFile-reported name over the file part otherwise.
// @early-stop
// __EH_prolog wall (~46%): logic complete + all externs/imports named. Retail
// establishes its /GX C++ EH frame with the shared out-of-line helper
// (`mov eax,OFFSET handler; call __EH_prolog` -> ebp frame, `sub esp,0x14c`), but
// THIS env's MSVC5 build ALWAYS INLINES that prologue (`push -1; push handler;
// mov eax,fs:0; push eax; mov fs:0,esp; sub esp,0x148`) - NO base object in the
// whole tree emits `call __EH_prolog`, so it is a toolchain-version wall. The
// inline-vs-helper frame reserves the EH slots differently, +4-shifting every
// [ebp-N] local and diverging both epilogues. Not source-steerable. Sibling of
// ConfigStoreRead::GetString (same framed 0x1bf* module, same wall).
RVA(0x001bf8f8, 0xd9)
i32 __stdcall CanonicalizePath(char* buf, char* path) {
    char* filePart;
    if (GetFullPathNameA(path, 0x104, buf, &filePart) == 0) {
        lstrcpynA(buf, path, 0x104);
        return 0;
    }

    CString root;
    ExtractRoot(buf, &root);

    DWORD maxComp;
    DWORD fsFlags;
    if (!GetVolumeInformationA(root, 0, 0, 0, &maxComp, &fsFlags, 0, 0)) {
        return 0;
    }

    if ((fsFlags & 2) == 0) {
        CharUpperA(buf);
    }
    if ((fsFlags & 4) == 0) {
        WIN32_FIND_DATAA fd;
        HANDLE h = FindFirstFileA(path, &fd);
        if (h != INVALID_HANDLE_VALUE) {
            FindClose(h);
            lstrcpyA(filePart, fd.cFileName);
        }
    }
    return 1;
}

// @source: import:FindFirstFileA
// GetFileTimeInfo - canonicalize `path` into out->m_path, then snapshot the
// file's size, attributes and the three file times (each converted to an engine
// CTime). A zero creation/access time falls back to the write time.
// @early-stop
// regalloc wall (~38%): logic complete + all callees named. `path` (arg1) is live
// across the CanonicalizePath call and then re-used by FindFirstFile; retail spills
// it to its home slot and RE-READS `[ebp+8]` at both uses (2 callee-saves esi/edi),
// while this env's cl PINS `path` to callee-saved ebx (an extra `push ebx`/`pop ebx`
// + `mov ebx,[ebp+8]`), a 3rd callee-save whose recolor cascades through the body
// and all three epilogues. The zeroing store also differs (`mov byte[edi],0` vs
// retail's peephole `and [edi],al` reusing the known-zero al). Not source-steerable.
RVA(0x001c1609, 0xb2)
i32 __stdcall GetFileTimeInfo(char* path, FileInfoRec* out) {
    if (!CanonicalizePath(out->m_path, path)) {
        out->m_path[0] = 0;
        return 0;
    }

    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(path, &fd);
    if (h == INVALID_HANDLE_VALUE) {
        return 0;
    }
    FindClose(h);

    out->m_10 = (u8)(fd.dwFileAttributes & 0x7f);
    out->m_c = fd.nFileSizeLow;

    EngCTimeTmp t;
    out->m_0 = *t.FromFileTime(&fd.ftCreationTime, -1);
    out->m_8 = *t.FromFileTime(&fd.ftLastAccessTime, -1);
    out->m_4 = *t.FromFileTime(&fd.ftLastWriteTime, -1);
    if (out->m_0 == 0) {
        out->m_0 = out->m_4;
    }
    if (out->m_8 == 0) {
        out->m_8 = out->m_4;
    }
    return 1;
}

// @source: import:CreateFileA
// RestoreFileTimeInfo - reapply the snapshot in `in` to `path`: clear the
// read-only bit if it blocks the write, reopen the file, SetFileTime from the
// three (present) times, then restore the recorded attributes.
// @early-stop
// import-CSE/regalloc wall (~77%): logic complete + all callees named; the guard
// structure, the CTimeToFileTime conversions and the CreateFile/SetFileTime/
// CloseHandle chain are byte-faithful modulo reloc names. GetLastError is called 6x:
// retail CSEs its IAT slot into ebx (`mov ebx,ds:GetLastError; call ebx` x6) and
// keeps curAttr in a stack slot [ebp-4], while this env's cl does NOT hoist the
// import load (`call ds:[__imp__GetLastError]` each time) and instead pins curAttr
// in ebx - an inverted register/CSE assignment. The `ctx` 0-arg also differs
// (immediate `push 0` vs retail's `push <known-zero reg>`). Not source-steerable.
RVA(0x001c176a, 0x12d)
void __stdcall RestoreFileTimeInfo(char* path, FileInfoRec* in) {
    FILETIME* creationFt = 0;
    FILETIME* accessFt = 0;

    DWORD curAttr = GetFileAttributesA(path);
    if (curAttr == 0xffffffff) {
        FileErrThunk(GetLastError(), 0);
    }

    if ((u32)in->m_10 != curAttr && (curAttr & 1)) {
        if (!SetFileAttributesA(path, in->m_10)) {
            FileErrThunk(GetLastError(), 0);
        }
    }

    if (in->m_4 != 0) {
        FILETIME writeFt;
        FILETIME accessTmp;
        FILETIME creationTmp;
        CTimeToFileTime(&in->m_4, &writeFt);

        if (in->m_8 != 0) {
            CTimeToFileTime(&in->m_8, &accessTmp);
            accessFt = &accessTmp;
        }
        if (in->m_0 != 0) {
            CTimeToFileTime(&in->m_0, &creationTmp);
            creationFt = &creationTmp;
        }

        HANDLE h = CreateFileA(path, 0xc0000000, 1, 0, 3, 0x80, 0);
        if (h == INVALID_HANDLE_VALUE) {
            FileErrThunk(GetLastError(), 0);
        }
        if (!SetFileTime(h, creationFt, accessFt, &writeFt)) {
            FileErrThunk(GetLastError(), 0);
        }
        if (!CloseHandle(h)) {
            FileErrThunk(GetLastError(), 0);
        }
    }

    if ((u32)in->m_10 != curAttr && (curAttr & 1) == 0) {
        if (!SetFileAttributesA(path, in->m_10)) {
            FileErrThunk(GetLastError(), 0);
        }
    }
}
