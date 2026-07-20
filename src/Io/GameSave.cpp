#include <rva.h>
#include <Gruntz/SerialCounter.h>     // g_serialCounter
#include <string.h>                   // strlen / memset inline to repne scasb / rep stos
#include <DDrawMgr/DDrawSurfaceMgr.h> // canonical CDDrawSurfaceMgr (SnapshotChildren @0x156020)
#include <Io/GameSave.h>              // CGameSaveHost (the g_gameReg/CGruntzMgr save-host facet)

// The recursive snapshot run-callback handed to SnapshotChildren; modeled NO-body
// so the call/address-of reloc-mask. Its type is the canonical HP_Callback.
// reloc-fidelity: the real body is SerialObjectFactory @0xd2a0
// (?SerialObjectFactory@@YAHPAX0HHPAPAX@Z, SerialObjectFactory.cpp), but retail's
// /INCREMENTAL link routes this address-of through the ILT jmp-THUNK 0x24e6 (jmp
// 0xd2a0), so the DIR32 stored here is 0x24e6, NOT 0xd2a0. Bind the address-taken
// symbol to the THUNK rva (same idiom as GruntzApp's _ErrorDialogProcThunk @0x33c8):
// the DIR32 target IS the thunk, so have==want==0x24e6 -> CORRECT (no MISBOUND).
// @data-symbol: ?SaveRunCallback@@YAHPAX0HHH@Z 0x000024e6
extern i32 __cdecl
SaveRunCallback(void* mgr, void* ser, i32 mode, i32, i32); // ILT thunk 0x24e6 -> 0xd2a0

DATA(0x00229930) // extern "C": C++ array-global mangling diverges clang vs MSVC5 (as g_mapCurve)
extern "C" i32 g_saveBuf[0x24]; // 0x229930

RVA(0x0000d170, 0x74)
i32 SaveGame(CGameSaveHost* host, char* name) {
    if (host == 0) {
        return 0;
    }
    if (name == 0) {
        return 0;
    }
    if (strlen(name) == 0) {
        return 0;
    }
    g_serialCounter = 0;
    memset(g_saveBuf, 0, 0x90);
    g_saveBuf[0] = 1;
    CDDrawSurfaceMgr* mgr = host->m_surfaceMgr;
    if (mgr == 0) {
        return 0;
    }
    return mgr->SnapshotChildren(SaveRunCallback, reinterpret_cast<i32>(name), "Gruntz Save Game", 0) != 0;
}

SIZE_UNKNOWN(CGameSaveHost);
