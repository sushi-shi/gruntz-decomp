// MenuState.h
#ifndef GRUNTZ_GRUNTZ_MENUSTATE_H_H
#define GRUNTZ_GRUNTZ_MENUSTATE_H_H

#include <Ints.h>
#include <Gruntz/GameMode.h> // g_versionRect (the version-string layout rect)

// --- C-linkage carriers for the TU's extern-C definitions (the defs
// inherit the linkage from these decls; the .cpp wrappers are gone) ---
extern "C" tagRECT g_versionRect; // .bss (def in MenuState.cpp)


// File-scope prototypes moved from the .cpp (external linkage
// belongs in the owner header).
void ShowHudMessage(
    CDDrawSurfaceMgr* sink,
    CString* text,
    RECT* box,
    i32 fontSel,
    i32 b,
    i32 c,
    i32 d,
    i32 e,
    i32 f
); // 0x1154b0

#endif // GRUNTZ_GRUNTZ_MENUSTATE_H_H
