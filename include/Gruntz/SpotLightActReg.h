// SpotLightActReg.h - the SpotLightActReg.cpp TU's exported globals/functions.
#ifndef GRUNTZ_GRUNTZ_SPOTLIGHTACTREG_H
#define GRUNTZ_GRUNTZ_SPOTLIGHTACTREG_H

#include <Ints.h>

#include <Gruntz/ActReg.h> // CActReg (extern below)
extern CActReg g_actReg_646188; // 0x00246188


// TU-local thunk/table names this TU registers (moved from the .cpp; the
// addresses are ILT thunk VAs, reloc-masked at every use).
extern "C" void Handler_402414(); // 0x402414
extern "C" void Handler_4025db(); // 0x4025db

#endif // GRUNTZ_GRUNTZ_SPOTLIGHTACTREG_H
