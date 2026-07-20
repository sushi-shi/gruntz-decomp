#ifndef GRUNTZ_WWD_WWDGRIDSHELL_H
#define GRUNTZ_WWD_WWDGRIDSHELL_H

#include <Ints.h>
#include <Wap32/Object.h> // CObject grand-base (slots 0/2/3/4)
#include <rva.h>
#include <Win32.h> // RECT (Setup's by-value arg)

struct CWwdGridShell : public CObject {
    virtual ~CWwdGridShell() OVERRIDE; // [1] +0x04; ??_G 0x168280, ??1 0x1682a0
    virtual void InsertSorted();         // [5] 0x168060: forwards (arg->m_18, 1) through
                                         //     m_c->m_8 to CDDrawChildGroup::InsertSorted_159e40
                                         //     (rel32 target byte-computed)
    i32 m_4;                              // +0x04
    char m_pad8[0x44 - 8];
    i32 Setup(RECT rc, i32 a, i32 b); // 0x1915c0 (reloc-masked)
    CWwdGridShell() {
        m_4 = 0; // cl auto-stamps &??_7CWwdGridShell first
    }
};
SIZE(CWwdGridShell, 0x44);
VTBL(CWwdGridShell, 0x001f0310); // ??_7CWwdGridShell (was g_subVtbl_5f0310)

#endif // GRUNTZ_WWD_WWDGRIDSHELL_H
