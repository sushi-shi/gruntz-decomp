#ifndef GRUNTZ_WWD_WWDGRIDSHELL_H
#define GRUNTZ_WWD_WWDGRIDSHELL_H

#include <Ints.h>
#include <Gruntz/WwdGrid.h>
#include <rva.h>

struct WwdRegion;

struct CWwdGridShell : public CWwdGrid {
    virtual ~CWwdGridShell() OVERRIDE;
    virtual void OnFound(WwdRegion* r) OVERRIDE;
    CWwdGridShell() {}
};
SIZE(0x44);

#endif // GRUNTZ_WWD_WWDGRIDSHELL_H
