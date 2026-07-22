// DDrawSubMgr.h - the DDrawSubMgr.cpp TU's exported globals/functions.
#ifndef GRUNTZ_DDRAWMGR_DDRAWSUBMGR_H
#define GRUNTZ_DDRAWMGR_DDRAWSUBMGR_H

#include <Ints.h>

extern float g_sndPanScale; // 0x001eff2c (0.01f; def in DDrawSubMgr.cpp)

// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---
extern "C" char g_emptyString[]; // 0x2293f4

#endif // GRUNTZ_DDRAWMGR_DDRAWSUBMGR_H
