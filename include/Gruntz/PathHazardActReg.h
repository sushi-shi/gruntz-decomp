// PathHazardActReg.h - the PathHazardActReg.cpp TU's exported globals/functions.
#ifndef GRUNTZ_GRUNTZ_PATHHAZARDACTREG_H
#define GRUNTZ_GRUNTZ_PATHHAZARDACTREG_H

#include <Ints.h>

#include <Gruntz/ActReg.h> // CActReg (extern below)
extern CActReg g_actReg_646250; // 0x00246250


// TU-local thunk/table names this TU registers (moved from the .cpp; the
// addresses are ILT thunk VAs, reloc-masked at every use).
extern "C" void Handler_402252(); // 0x402252
extern "C" void Handler_4021d5(); // 0x4021d5

#endif // GRUNTZ_GRUNTZ_PATHHAZARDACTREG_H
