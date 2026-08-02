#ifndef GRUNTZ_GRUNTZ_GRUNTCOMBAT_H
#define GRUNTZ_GRUNTZ_GRUNTCOMBAT_H

#include <Ints.h>

#include <Gruntz/ActReg.h>

extern "C" i32 g_dirVec[9][4];
extern "C" const u8 g_hitTable[23][23];
extern "C" const float g_quarterScale;
extern "C" const float g_slopeTwo;
extern "C" const float g_slopeNegTwo;
extern "C" const double g_slopeHalf;
extern "C" const double g_slopeZero;

static inline void GruntScratchTeardown();

#endif // GRUNTZ_GRUNTZ_GRUNTCOMBAT_H
