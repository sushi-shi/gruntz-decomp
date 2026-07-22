// RezSync.h
#ifndef GRUNTZ_REZ_REZSYNC_H_H
#define GRUNTZ_REZ_REZSYNC_H_H

#include <Ints.h>
#include <Bute/ButeStore.h> // zPTree (a zPTree typedef - not fwd-declarable)

class CGruntSpawnConfig;

// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---
extern i32 g_dlgVal_6451a4, g_dlgVal_645268, g_dlgVal_64526c, g_dlgVal_6452a8;
extern i32 g_dlgVal_6452d0, g_dlgVal_6452d4, g_dlgVal_645538, g_dlgVal_645558;
extern i32 g_dlgVal_64555c, g_dlgVal_645560, g_dlgVal_645564, g_dlgVal_645568;
    // @identity-TODO: 0x645570 has TWO readings and they contradict - here it is the
    // CSpawnOwner* handed to CGruntSpawnConfig::Init, in GruntzMgr it is read as a
    // DirectInputMgr2* ("attract host"). Typing it either way would be a guess, so it
    // stays void* and the `(CSpawnOwner*)` cast at the call site STAYS: the cast is
    // telling the truth - the type above it is still unresolved. Settle the conflation
    // (one object, two class names) and the cast falls out on its own.
    // 0x645578 IS the CGruntSpawnConfig singleton - PROVEN in this very function:
    // RezAlloc(0x28) (its exact size), then ->Init(CSpawnOwner*), then RezFree on the
    // failure path. Typed, so the `(CGruntSpawnConfig*)` cast at the Init call is gone.
extern "C" char* StrUpr(char*); // 0x18d330
extern "C" void cb_403193();
extern "C" void cb_401bc2();
extern char g_lab504358[]; // 0x504358
extern char g_lab545854[]; // 0x545854
extern zPTree g_store6453f0, g_store64544c; // == g_buteMgr.m_tree / .m_tree74

// --- C-linkage carriers for the TU's extern-C definitions (the defs
// inherit the linkage from these decls; the .cpp wrappers are gone) ---
extern "C" i32 g_attractStateCount;

extern "C" i32 g_disableAudio;
extern "C" i32 g_disableSound;
extern "C" i32 g_disableMusic;
extern "C" i32 g_disableJoystick;
extern "C" i32 g_disableSoundFonts;
extern "C" i32 g_disableDirectVideo;
extern "C" i32 g_disableHqMovie;
extern "C" i32 g_enableTriple;
extern "C" i32 g_enableHiColor;
extern "C" i32 g_enableTrueColor;
extern "C" i32 g_enableEmulation;
#endif // GRUNTZ_REZ_REZSYNC_H_H
