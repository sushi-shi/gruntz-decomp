// CursorSnapActReg.h - the CursorSnapActReg.cpp TU's exported globals/functions.
#ifndef GRUNTZ_GRUNTZ_CURSORSNAPACTREG_H
#define GRUNTZ_GRUNTZ_CURSORSNAPACTREG_H

#include <Ints.h>

#include <Gruntz/ActReg.h>                  // CLogicActTable (extern below)
extern CLogicActTable g_logicActReg_62bfa0; // 0x0022bfa0

// TU-local thunk/table names this TU registers (moved from the .cpp; the
// addresses are ILT thunk VAs, reloc-masked at every use).
extern "C" void CursorSnapAct();

#endif // GRUNTZ_GRUNTZ_CURSORSNAPACTREG_H
