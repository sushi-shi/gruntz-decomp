// FileGetStatus.cpp - CFile::GetStatus (RVA 0x1c152f).
//
// Fills a CFileStatus from the open handle: zero it, copy the path, then (if the
// handle is live) read the file times, size and attribute byte, converting each
// FILETIME to a CTime time_t. Part of the 0x1bf*-0x1d5* registry/COM helper module
// (framed profile). Field names are placeholders; only offsets + code bytes are
// load-bearing.
#include <rva.h>

#include <Win32.h>

extern "C" void* memset(void* dst, i32 c, u32 n);

// MFC CTime: built from a FILETIME; the time_t is its first dword.
struct CTime {
    i32 m_time;                          // +0x00
    CTime(const FILETIME* ft, i32 nDST); // 0x1b3065
};

struct CFileStatus {
    i32 m_0;          // +0x00 create time
    i32 m_4;          // +0x04 access time
    i32 m_8;          // +0x08 modify time
    u32 m_c;          // +0x0c size
    u8 m_10;          // +0x10 attribute
    char m_11;        // +0x11
    char m_12[0x104]; // +0x12 full path
};

class CFile {
public:
    i32 GetStatus(CFileStatus* status);
    char m_pad00[0x4];
    HANDLE m_4; // +0x04 handle
    char m_pad08[0xc - 0x8];
    char* m_c; // +0x0c full-path CString (length at m_c[-8])
};

// @early-stop
// framed module (/O2 /Oy-) regalloc/stack-slot wall: the body + every Win32/CTime
// call match retail, but the optimizer reuses the dead inbound-arg slot ([ebp+8])
// as the CTime scratch and pins the -1 constant in ebx - state that can't be forced
// from source. Same residual as ConfigStore.cpp. docs/patterns/
// o2-optimizer-bailout-framed.md. Final sweep.
RVA(0x001c152f, 0xda)
i32 CFile::GetStatus(CFileStatus* status) {
    memset(status, 0, 0x118);
    lstrcpynA(status->m_12, m_c, 0x104);
    if (m_4 != INVALID_HANDLE_VALUE) {
        FILETIME create, access, modify;
        if (GetFileTime(m_4, &create, &access, &modify)) {
            DWORD size = GetFileSize(m_4, 0);
            status->m_c = size;
            if (size != 0xffffffff) {
                if (*(i32*)(m_c - 8) != 0) {
                    DWORD attr = GetFileAttributesA(m_c);
                    if (attr != 0xffffffff) {
                        status->m_10 = (u8)attr;
                    } else {
                        status->m_10 = 0;
                    }
                } else {
                    status->m_10 = 0;
                }
                status->m_0 = CTime(&create, -1).m_time;
                status->m_8 = CTime(&modify, -1).m_time;
                i32 a = CTime(&access, -1).m_time;
                status->m_4 = a;
                if (status->m_0 == 0) {
                    status->m_0 = a;
                }
                if (status->m_8 == 0) {
                    status->m_8 = status->m_4;
                }
                return 1;
            }
            return 0;
        }
    }
    return 1;
}
