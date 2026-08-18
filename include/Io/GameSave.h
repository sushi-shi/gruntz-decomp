#ifndef GRUNTZ_IO_GAMESAVE_H
#define GRUNTZ_IO_GAMESAVE_H

#include <rva.h>

#include <Enums.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Ints.h>

class CGruntzMgr;
class CDDrawSurfaceMgr;
class CFileMemBase;

i32 SaveGame(CGruntzMgr* host, char* name);

extern i32 g_saveBuf[0x24];

extern i32 g_savedMenuCmd;

i32 __cdecl SerialObjectFactory(
    CDDrawSurfaceMgr* ctx,
    CFileMemBase* archive,
    SerialMode mode,
    LogicTypeId typeId,
    void* payload
);

#endif // GRUNTZ_IO_GAMESAVE_H
