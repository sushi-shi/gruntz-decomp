// GameRegPtr.h - the ONE declaration of the global game-manager singleton pointer, in
// the Win32-safe CGameRegistry view (?g_gameReg, the WwdGameReg*/CGameRegistry* at RVA
// 0x24556c / VA 0x64556c). See <Gruntz/GameRegistry.h> for the full singleton shape and
// the dual-view (CGruntzMgr MFC view) rationale.
//
// The g_gameReg symbol is DATA()-bound to RVA 0x24556c in its owner TU src/Gruntz/
// GruntzMgr.cpp; this header is the decl-only consumer view. Kept extern "C" (one C
// symbol _g_gameReg)
// so the MFC-view TUs can still see it as CGruntzMgr* / WwdGameReg* through their own
// headers without this Win32-safe view forcing a type on them. Consumers that use the
// CGameRegistry* view #include this instead of re-`extern`-ing it per-TU. Emits no code
// and pulls no other header -> matching-neutral.
#ifndef GRUNTZ_GAMEREGPTR_H
#define GRUNTZ_GAMEREGPTR_H

struct CGameRegistry;
extern "C" struct CGameRegistry* g_gameReg; // *0x24556c singleton (Win32/CGameRegistry view)

#endif // GRUNTZ_GAMEREGPTR_H
