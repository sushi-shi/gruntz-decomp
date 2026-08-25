#ifndef GRUNTZ_IO_GAMESAVE_H
#define GRUNTZ_IO_GAMESAVE_H

#include <rva.h>

#include <Enums.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialObjectFactory.h>
#include <Ints.h>

class CGruntzMgr;
i32 SaveGame(CGruntzMgr* gameMgr, char* name);

extern i32 g_saveBuf[0x24];

extern i32 g_savedMenuCmd;

#endif // GRUNTZ_IO_GAMESAVE_H
