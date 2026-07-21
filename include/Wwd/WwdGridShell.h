#ifndef GRUNTZ_WWD_WWDGRIDSHELL_H
#define GRUNTZ_WWD_WWDGRIDSHELL_H

#include <Ints.h>
#include <Wap32/Object.h> // CObject grand-base (slots 0/2/3/4)
#include <rva.h>

struct WwdRegion; // the grid bucket node (<Gruntz/WwdGridIter.h>)

struct CWwdGridShell : public CObject {
    virtual ~CWwdGridShell() OVERRIDE; // [1] +0x04; ??_G 0x168280, ??1 0x1682a0
    // [5] 0x168060 (GameLevelMove.cpp): the concrete impl of the slot the abstract
    // CWwdGrid leaves __purecall - forward (r->m_object, 1) to the world's
    // CDDrawChildGroup::InsertSorted via OwnerMgr()->m_childGroup.
    virtual void OnFound(WwdRegion* r);
    i32 m_4; // +0x04
    char m_pad8[0x44 - 8];
    // (the ex-"Setup" @0x1915c0 was ??0CWwdGrid run as a re-init on this raw
    // object - the two-phase construction is spelled placement-new at the Init site)
    CWwdGridShell() {
        m_4 = 0; // cl auto-stamps &??_7CWwdGridShell first
    }
};
SIZE(CWwdGridShell, 0x44);
VTBL(CWwdGridShell, 0x001f0310); // ??_7CWwdGridShell (was g_subVtbl_5f0310)

#endif // GRUNTZ_WWD_WWDGRIDSHELL_H
