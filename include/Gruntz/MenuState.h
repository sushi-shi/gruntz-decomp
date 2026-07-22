// MenuState.h
#ifndef GRUNTZ_GRUNTZ_MENUSTATE_H_H
#define GRUNTZ_GRUNTZ_MENUSTATE_H_H

#include <Ints.h>
#include <Gruntz/GameMode.h> // CGMVerRect (the version-string layout rect)

// --- C-linkage carriers for the TU's extern-C definitions (the defs
// inherit the linkage from these decls; the .cpp wrappers are gone) ---
extern "C" CGMVerRect g_versionRect; // .bss (def in MenuState.cpp)

#endif // GRUNTZ_GRUNTZ_MENUSTATE_H_H
