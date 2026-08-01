#ifndef GRUNTZ_GRUNTZ_GRUNTCOMBAT_H
#define GRUNTZ_GRUNTZ_GRUNTCOMBAT_H

#include <Ints.h>

#include <Gruntz/ActReg.h>

extern "C" i32 g_dirVec[9][4];
extern "C" unsigned char g_hitTable[];
extern "C" float g_dtScale;
extern "C" float g_tanC0;
extern "C" float g_tanC1;
extern "C" double g_tanC2;
extern "C" double g_tanC3;

static inline void GruntScratchTeardown();

#endif // GRUNTZ_GRUNTZ_GRUNTCOMBAT_H
