// TileLogicPump.h - the TileLogicPump.cpp TU's exported globals/functions.
#ifndef GRUNTZ_GRUNTZ_TILELOGICPUMP_H
#define GRUNTZ_GRUNTZ_TILELOGICPUMP_H

#include <Ints.h>

#include <Gruntz/ActReg.h> // CActReg (extern below)
extern CActReg g_brickzActReg; // 0x0024e7c0

extern CActReg g_checkpointActReg; // 0x0024e7e8

extern CActReg g_tileTriggerActReg; // 0x0024e810

extern CActReg g_tileTriggerSwitchActReg; // 0x0024e798

extern CActReg g_warpStonePadActReg; // 0x0024e6a0


// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---
extern CActReg g_tileActReg;

#endif // GRUNTZ_GRUNTZ_TILELOGICPUMP_H
