// RezSync.h
#ifndef GRUNTZ_REZ_REZSYNC_H_H
#define GRUNTZ_REZ_REZSYNC_H_H

#include <Ints.h>
class CGruntSpawnConfig;

// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---
extern i32 g_dlgVal_6451a4, g_dlgVal_645268, g_dlgVal_64526c, g_dlgVal_6452a8;
extern i32 g_dlgVal_6452d0, g_dlgVal_6452d4, g_dlgVal_645538, g_dlgVal_645558;
extern i32 g_dlgVal_64555c, g_dlgVal_645560, g_dlgVal_645564, g_dlgVal_645568;
// 0x645578 IS the CGruntSpawnConfig singleton - PROVEN in this very function:
// RezAlloc(0x28) (its exact size), then ->Init(CGruntzMgr*), then RezFree on the
// failure path. Typed, so the `(CGruntSpawnConfig*)` cast at the Init call is gone.
extern "C" char* StrUpr(char*); // 0x18d330
extern "C" void cb_403193();
// The bute parse-error sink handed to CButeMgr::SetErrCallback. Its ARITY is proven
// from the body the 0x1bc2 ILT thunk jumps to (RVA 0x119320, 0x15 B):
//   mov ecx,[0x64556c]; test ecx,ecx; je ret; mov eax,[esp+4]; push eax; call <sink>
// - one __cdecl stack argument, the message string, forwarded to a global sink when
// one is installed. The ex `void cb_401bc2()` decl had the WRONG arity, which is why
// the call site needed a reinterpret_cast to ErrCallback. Still declared-only: the
// 0x119320 body is not reconstructed yet (@identity-TODO: its owning TU).
extern "C" void ButeParseErrorSink(const char* msg);

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
