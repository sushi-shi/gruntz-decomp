#ifndef GRUNTZ_WWD_WWDGRIDSHELL_H
#define GRUNTZ_WWD_WWDGRIDSHELL_H

#include <rva.h>

#include <Gruntz/WwdGrid.h>
#include <Ints.h>

struct WwdRegion;

// @identity-TODO: original class spelling is unavailable.
struct CObjectActivationGrid : public CWwdGrid {
    virtual void OnFound(WwdRegion* r) OVERRIDE;
};

#endif // GRUNTZ_WWD_WWDGRIDSHELL_H
