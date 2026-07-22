// CreditsState.h - the CreditsState.cpp TU's exported globals/functions.
#ifndef GRUNTZ_GRUNTZ_CREDITSSTATE_H
#define GRUNTZ_GRUNTZ_CREDITSSTATE_H

#include <Ints.h>

extern i32 g_clipRegionEnabled; // 0x0022bf74


// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---
extern "C" i32 __stdcall Eng_SmackStep(void* handle, i32 frame);

#endif // GRUNTZ_GRUNTZ_CREDITSSTATE_H
