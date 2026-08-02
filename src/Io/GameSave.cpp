#include <rva.h>

#include <Io/GameSave.h>

#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/SerialCounter.h>

#include <string.h>

DATA(0x00229930)
i32 g_saveBuf[0x24];

RVA(0x0000d170, 0x74)
i32 SaveGame(CGruntzMgr* host, char* name) {
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
    CDDrawSurfaceMgr* mgr = host->m_world;
    if (mgr == 0) {
        return 0;
    }
    return mgr->SnapshotChildren(&SerialObjectFactory, name, "Gruntz Save Game", 0) != 0;
}
