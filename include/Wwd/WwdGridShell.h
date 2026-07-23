#ifndef GRUNTZ_WWD_WWDGRIDSHELL_H
#define GRUNTZ_WWD_WWDGRIDSHELL_H

#include <Ints.h>
#include <Gruntz/WwdGrid.h> // the abstract CWwdGrid base
#include <rva.h>

struct WwdRegion; // the grid bucket node (<Gruntz/WwdGridIter.h>)

// The concrete grid: override the abstract base's OnFound slot and forward the
// found object to the world's sorted child group. No additional fields.
struct CWwdGridShell : public CWwdGrid {
    virtual ~CWwdGridShell() OVERRIDE;           // [1] +0x04; ??_G 0x168280, ??1 0x1682a0
    virtual void OnFound(WwdRegion* r) OVERRIDE; // [5] 0x168060 (GameLevelMove.cpp)
    CWwdGridShell() {
        m_allocated = 0;
    }
};
SIZE(0x44);

#endif // GRUNTZ_WWD_WWDGRIDSHELL_H
