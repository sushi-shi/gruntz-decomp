// SpotLightActReg.h - the SpotLightActReg.cpp TU's exported globals/functions.
#ifndef GRUNTZ_GRUNTZ_SPOTLIGHTACTREG_H
#define GRUNTZ_GRUNTZ_SPOTLIGHTACTREG_H

#include <Ints.h>

#include <Gruntz/ActReg.h> // CActReg (extern below)

// TU-local thunk/table names this TU registers (moved from the .cpp; the
// addresses are ILT thunk VAs, reloc-masked at every use).
extern "C" void SpotLightActB(); // 0x402414
extern "C" void SpotLightActA(); // 0x4025db

#endif // GRUNTZ_GRUNTZ_SPOTLIGHTACTREG_H
