#include <StdAfx.h>

#include <Ints.h>

#include <Io/GameSave.h>

#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Enums.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialCounter.h>

#include <string.h>

i32 g_saveBuf[0x24];

i32 SaveGame(CGruntzMgr* gameMgr, io::Output& target) {
    if (gameMgr == NULL) {
        return 0;
    }
    g_serialCounter = 0;
    memset(g_saveBuf, 0, 0x90);
    g_saveBuf[0] = 1;
    CDDrawSurfaceMgr* world = gameMgr->m_world;
    if (world == NULL) {
        return 0;
    }
    return world
               ->SnapshotChildren(&GameSerializationCallback, target, "Gruntz Save Game", LOGIC_UNSET)
           != LOGIC_UNSET;
}
