#ifndef GRUNTZ_GRUNTZ_DEMOHELPERS_H
#define GRUNTZ_GRUNTZ_DEMOHELPERS_H

#include <rva.h>

#include <Ints.h>

class CDDrawSurfaceMgr;

// @identity-TODO
// The body and its thunk have no caller or data reference, and expose no allocation,
// RTTI, or mangled owner type; only the CDDrawSurfaceMgr pointer at +0xc is proven.
class CDemoSetup {
public:
    i32 SetupDemoActors();
    char m_pad0[0xc];
    CDDrawSurfaceMgr* m_world;
};
SIZE_UNKNOWN();

#endif // GRUNTZ_GRUNTZ_DEMOHELPERS_H
