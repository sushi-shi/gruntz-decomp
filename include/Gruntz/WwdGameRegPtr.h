// WwdGameRegPtr.h - the ONE declaration of the global game-manager singleton pointer in
// the WwdGameReg view (?g_gameReg@@3PAUWwdGameReg@@A, VA 0x64556c - the retail mangled
// name's own type). Companion to <Gruntz/GameRegPtr.h> / <Gruntz/GameRegMfcPtr.h> (the
// CGameRegistry / CGruntzMgr views of the SAME object / SAME C symbol _g_gameReg).
//
// The WwdGameReg-view TUs #include this instead of re-`extern`-ing it per-TU; the full
// WwdGameReg shape comes from their existing <Gruntz/WwdGameReg.h> include. Kept extern
// "C" (single C symbol) so this view and the others coexist without forcing one C++ type.
// Opt-in: never pulled by a header another view's TU includes. Emits no code -> neutral.
#ifndef GRUNTZ_WWDGAMEREGPTR_H
#define GRUNTZ_WWDGAMEREGPTR_H

struct WwdGameReg;
extern "C" struct WwdGameReg* g_gameReg; // *0x24556c singleton (WwdGameReg view)

#endif // GRUNTZ_WWDGAMEREGPTR_H
