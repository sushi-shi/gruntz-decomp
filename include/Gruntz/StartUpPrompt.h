// StartUpPrompt.h - the StartUpPrompt.cpp TU's exported globals/functions.
#ifndef GRUNTZ_GRUNTZ_STARTUPPROMPT_H
#define GRUNTZ_GRUNTZ_STARTUPPROMPT_H

#include <Ints.h>
#include <Gruntz/GruntzMgr.h> // ex Globals.h

extern HINSTANCE g_appResHandle; // 0x00251618


// --- C-linkage carriers for the TU's extern-C definitions (the defs
// inherit the linkage from these decls; the .cpp wrappers are gone) ---
extern "C" i32 g_cdPromptResult;

#endif // GRUNTZ_GRUNTZ_STARTUPPROMPT_H
