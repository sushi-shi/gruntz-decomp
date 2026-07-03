// FileGetStatus.cpp - CFile::GetStatus (RVA 0x1c152f), the statically-linked
// NAFXCW (MFC) member. Modeled with the REAL MFC CFile / CFileStatus / CTime from
// <Mfc.h> (was a hand-rolled CFile/CFileStatus/CTime shim): fills a CFileStatus
// from the open handle - zero it, copy the cached path, then (if the handle is
// live) read the file times/size/attribute, converting each FILETIME to a CTime.
//
// The retail store order (verified via dump_target 0x1c152f) is m_ctime@+0,
// m_atime@+8, m_mtime@+4 - exactly the MFC CFileStatus layout, so the real struct
// reproduces the offsets. The prior hand-rolled shim mislabeled access/modify and
// swapped their stores; the real MFC source is byte-faithful. Compiled /ln (favor
// size, the `o1` profile) because NAFXCW ships /ln: memory-operand pushes + the
// pinned -1 in ebx are /O2-framed misses.
//
// @early-stop
// ~95%. Residual is (1) the 4 IAT import calls (ff15 [__imp_X] vs the raw absolute
// [0x6c40xx]) reloc-masked, (2) the 3 CTime-ctor relocs named mangled here vs the
// delinker's generic "CTime", and (3) a tail-merge block-order flip in the
// GetFileAttributes m_attribute=0 merge (je =0-first vs jne =al-first) not
// source-steerable at /ln. Code bytes otherwise byte-identical.
#include <rva.h>

#include <Mfc.h> // CFile, CFileStatus, CTime, CString + <windows.h>

// Retail CALLs memset (0x121380); force the out-of-line form (not the intrinsic
// `rep stosl`, which would steal edi from `this` and ebx from the pinned -1).
extern "C" void* memset(void* dst, int c, unsigned int n);
#pragma function(memset)

RVA(0x001c152f, 0xda)
BOOL CFile::GetStatus(CFileStatus& rStatus) const {
    memset(&rStatus, 0, sizeof(CFileStatus));
    lstrcpyn(rStatus.m_szFullName, m_strFileName, _MAX_PATH);
    if (m_hFile != (UINT)-1) {
        FILETIME ftCreate, ftAccess, ftModify;
        if (!GetFileTime((HANDLE)m_hFile, &ftCreate, &ftAccess, &ftModify)) {
            return FALSE;
        }
        DWORD dwSize = GetFileSize((HANDLE)m_hFile, NULL);
        rStatus.m_size = (LONG)dwSize;
        if (dwSize == (DWORD)-1) {
            return FALSE;
        }
        if (m_strFileName.GetLength() != 0) {
            DWORD dwAttr = GetFileAttributes(m_strFileName);
            if (dwAttr == (DWORD)-1) {
                rStatus.m_attribute = 0;
            } else {
                rStatus.m_attribute = (BYTE)dwAttr;
            }
        } else {
            rStatus.m_attribute = 0;
        }
        rStatus.m_ctime = CTime(ftCreate, -1);
        rStatus.m_atime = CTime(ftAccess, -1);
        rStatus.m_mtime = CTime(ftModify, -1);
        if (rStatus.m_ctime.GetTime() == 0) {
            rStatus.m_ctime = rStatus.m_mtime;
        }
        if (rStatus.m_atime.GetTime() == 0) {
            rStatus.m_atime = rStatus.m_mtime;
        }
    }
    return TRUE;
}
