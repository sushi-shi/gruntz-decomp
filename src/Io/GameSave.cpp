#include <rva.h>
#include <Gruntz/SerialCounter.h>     // g_serialCounter
#include <string.h>                   // strlen / memset inline to repne scasb / rep stos
#include <DDrawMgr/DDrawSurfaceMgr.h> // canonical CDDrawSurfaceMgr (SnapshotChildren @0x156020)
#include <Io/GameSave.h>              // CGameSaveHost (the g_gameReg/CGruntzMgr save-host facet)

// The recursive snapshot run-callback handed to SnapshotChildren IS
// SerialObjectFactory @0xd2a0 (SerialObjectFactory.cpp); retail's /INCREMENTAL
// link routes the address-of through the ILT jmp-thunk 0x24e6 (reloc-masked).
i32 __cdecl SerialObjectFactory(void* ctx, void* ar, i32 mode, i32 typeId, void** ppObj);

DATA(0x00229930)     // C linkage inherited from GameSave.h's extern "C" decl (as g_mapCurve)
i32 g_saveBuf[0x24]; // the OWNER DEFINITION (zero-init, matching the retail datum)

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
    return mgr->SnapshotChildren(
               reinterpret_cast<HP_Callback>(&SerialObjectFactory),
               reinterpret_cast<i32>(name),
               "Gruntz Save Game",
               0
           )
           != 0;
}
