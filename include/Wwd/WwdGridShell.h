#ifndef GRUNTZ_WWD_WWDGRIDSHELL_H
#define GRUNTZ_WWD_WWDGRIDSHELL_H

#include <Ints.h>
#include <Gruntz/WwdGrid.h> // the abstract CWwdGrid base
#include <rva.h>

struct WwdRegion; // the grid bucket node (<Gruntz/WwdGridIter.h>)

// The CONCRETE grid: derives the abstract CWwdGrid, overriding the __purecall
// OnFound slot (forward (r->m_object, 1) to the world's CDDrawChildGroup::
// InsertSorted via OwnerMgr()->m_childGroup). Same 0x44 layout - no own fields.
struct CWwdGridShell : public CWwdGrid {
    virtual ~CWwdGridShell() OVERRIDE;           // [1] +0x04; ??_G 0x168280, ??1 0x1682a0
    virtual void OnFound(WwdRegion* r) OVERRIDE; // [5] 0x168060 (GameLevelMove.cpp)
    // (the ex-"Setup" @0x1915c0 was ??0CWwdGrid run as a re-init on this raw
    // object - the two-phase construction is spelled placement-new at the Init site)
    CWwdGridShell() {
        m_allocated = 0; // cl auto-stamps &??_7CWwdGridShell first
    }
};
SIZE(0x44);
VTBL(CWwdGridShell, 0x001f0310); // ??_7CWwdGridShell (was g_subVtbl_5f0310)

#endif // GRUNTZ_WWD_WWDGRIDSHELL_H
