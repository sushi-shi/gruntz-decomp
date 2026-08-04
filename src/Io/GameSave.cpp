#include <rva.h>

#include <Io/GameSave.h>

#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Enums.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialCounter.h>

#include <string.h>

DATA(0x00229930)
i32 g_saveBuf[0x24];

RVA(0x0000d170, 0x74)
i32 SaveGame(CGruntzMgr* host, char* name) {
    if (host == NULL) {
        return 0;
    }
    if (name == NULL) {
        return 0;
    }
    if (strlen(name) == 0) {
        return 0;
    }
    g_serialCounter = 0;
    memset(g_saveBuf, 0, 0x90);
    g_saveBuf[0] = 1;
    CDDrawSurfaceMgr* mgr = host->m_world;
    if (mgr == NULL) {
        return 0;
    }
    // Filter seeded with 0, NOT LOGIC_NONE(-1): a full save keeps every child,
    // and 0 is not a member of the LogicTypeId domain (same idiom as
    // CGameObject::WriteSnapshot's typeTag).
    return mgr->SnapshotChildren(
               &SerialObjectFactory,
               name,
               "Gruntz Save Game",
               static_cast<LogicTypeId>(0)
           )
           != 0;
}
