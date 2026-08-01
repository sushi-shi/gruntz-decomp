#ifndef GRUNTZ_GRUNTZ_DEMOHELPERS_H
#define GRUNTZ_GRUNTZ_DEMOHELPERS_H

#include <Ints.h>
#include <rva.h>

class CDDrawSurfaceMgr;

// @identity-TODO
// The body and its thunk have no caller or data reference, and expose no allocation,
// RTTI, or mangled owner type; only the CDDrawSurfaceMgr pointer at +0xc is proven.
class CDemoSetup {
public:
    i32 SetupDemoActors();
    char m_pad0[0xc];
    CDDrawSurfaceMgr* m_c;
};
SIZE_UNKNOWN();

struct Orient3 {
    i32 m_0, m_4, m_8;
    void StepA(i32 count);
    void StepB(i32 count);
};
SIZE_UNKNOWN();

#endif // GRUNTZ_GRUNTZ_DEMOHELPERS_H
