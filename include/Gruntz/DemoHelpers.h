#ifndef GRUNTZ_GRUNTZ_DEMOHELPERS_H
#define GRUNTZ_GRUNTZ_DEMOHELPERS_H

#include <Ints.h>

#include <Ints.h>

class CDDrawSurfaceMgr;

class CDemoSetup {
public:
    i32 SetupDemoActors();
    char m_pad0[0xc];
    CDDrawSurfaceMgr* m_world;
};

#endif
