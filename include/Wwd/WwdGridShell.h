#ifndef GRUNTZ_WWD_WWDGRIDSHELL_H
#define GRUNTZ_WWD_WWDGRIDSHELL_H

#include <Ints.h>

#include <Gruntz/WwdGrid.h>
#include <Ints.h>

struct WwdRegion;

struct CWwdGridShell : public CWwdGrid {
    virtual void OnFound(WwdRegion* r)  ;
};

#endif
