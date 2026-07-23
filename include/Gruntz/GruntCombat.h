// GruntCombat.h - the GruntCombat.cpp TU's exported globals/functions.
#ifndef GRUNTZ_GRUNTZ_GRUNTCOMBAT_H
#define GRUNTZ_GRUNTZ_GRUNTCOMBAT_H

#include <Ints.h>

#include <Gruntz/ActReg.h> // CActReg (extern below)

// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---
extern "C" i32 g_dirVec[9][4];         // DAT_00644970
extern "C" unsigned char g_hitTable[]; // DAT_005e9788
extern "C" float g_dtScale;            // DAT_005e999c death-touch duration scale
extern "C" float g_tanC0;              // DAT_005e99a0
extern "C" float g_tanC1;              // DAT_005e99a4
extern "C" double g_tanC2;             // DAT_005e99a8
extern "C" double g_tanC3;             // DAT_005e99b0

#endif // GRUNTZ_GRUNTZ_GRUNTCOMBAT_H
