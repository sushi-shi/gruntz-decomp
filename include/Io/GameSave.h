#ifndef GRUNTZ_IO_GAMESAVE_H
#define GRUNTZ_IO_GAMESAVE_H

#include <Ints.h>
#include <Io/Bytes.h>

#include <Enums.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialObjectFactory.h>
#include <Ints.h>
#include <Io/Bytes.h>

class CGruntzMgr;
i32 SaveGame(CGruntzMgr* gameMgr, io::Output& target);

extern i32 g_saveBuf[0x24];

extern i32 g_savedMenuCmd;

#endif
