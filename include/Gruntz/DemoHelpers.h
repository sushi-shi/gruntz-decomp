#ifndef GRUNTZ_GRUNTZ_DEMOHELPERS_H
#define GRUNTZ_GRUNTZ_DEMOHELPERS_H

#include <rva.h>

#include <Ints.h>

class CGameWorld;

// @identity-TODO
// The body and its thunk have no caller or data reference, and expose no allocation,
// RTTI, or mangled owner type; only the CGameWorld pointer at +0xc is proven.
class CDemoSetup {
public:
    i32 SetupDemoActors();
    char m_pad0[0xc];
    CGameWorld* m_world;
};

#endif // GRUNTZ_GRUNTZ_DEMOHELPERS_H
