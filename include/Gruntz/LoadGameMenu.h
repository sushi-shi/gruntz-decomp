// LoadGameMenu.h - the LoadGameMenu.cpp TU's exported globals/functions.
#ifndef GRUNTZ_GRUNTZ_LOADGAMEMENU_H
#define GRUNTZ_GRUNTZ_LOADGAMEMENU_H

#include <Ints.h>


// TU-local thunk/table names this TU registers (moved from the .cpp; the
// addresses are ILT thunk VAs, reloc-masked at every use).
extern "C" void LoadDeleteDlgProc(); // 0x121c (GAME_DELETE)
extern "C" void LoadInfoDlgProc();   // 0x1e3d (GAME_INFO)

#endif // GRUNTZ_GRUNTZ_LOADGAMEMENU_H
