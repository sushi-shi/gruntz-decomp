// GruntVoiceActReg.h - the GruntVoiceActReg.cpp TU's exported globals/functions.
#ifndef GRUNTZ_GRUNTZ_GRUNTVOICEACTREG_H
#define GRUNTZ_GRUNTZ_GRUNTVOICEACTREG_H

#include <Ints.h>

#include <Gruntz/ActReg.h> // CActReg (extern below)
extern CActReg g_actReg_6514d8; // 0x002514d8

// TU-local thunk/table names this TU registers (moved from the .cpp; the
// addresses are ILT thunk VAs, reloc-masked at every use).
extern "C" void GruntVoiceActB(); // 0x402dd8
extern "C" void GruntVoiceActA(); // 0x4037bf

#endif // GRUNTZ_GRUNTZ_GRUNTVOICEACTREG_H
