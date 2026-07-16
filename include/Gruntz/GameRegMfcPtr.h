// GameRegMfcPtr.h - the ONE declaration of the global game-manager singleton pointer in
// the MFC CGruntzMgr view (?g_gameReg, VA 0x64556c). Companion to <Gruntz/GameRegPtr.h>
// (the Win32-safe CGameRegistry view of the SAME object / SAME C symbol _g_gameReg).
//
// The CGruntzMgr-view TUs (they dereference g_gameReg for MFC methods/members) #include
// this instead of re-`extern`-ing it per-TU; the full CGruntzMgr shape comes from their
// existing <Gruntz/GruntzMgr.h> include. Kept extern "C" (single C symbol) so this view
// and the CGameRegistry view coexist without forcing one C++ type. Opt-in: never pulled
// by a header a CGameRegistry-view TU includes, so the two views never collide in one TU.
// Emits no code, pulls no other header -> matching-neutral.
#ifndef GRUNTZ_GAMEREGMFCPTR_H
#define GRUNTZ_GAMEREGMFCPTR_H

class CGruntzMgr;
extern "C" CGruntzMgr* g_gameReg; // *0x24556c singleton (MFC/CGruntzMgr view)

#endif // GRUNTZ_GAMEREGMFCPTR_H
