#ifndef GRUNTZ_IO_GAMESAVE_H
#define GRUNTZ_IO_GAMESAVE_H

#include <Ints.h>
#include <rva.h>

class CGruntzMgr;

i32 SaveGame(CGruntzMgr* host, char* name);

extern "C" i32 g_saveBuf[0x24];

extern i32 g_savedMenuCmd;

i32 __cdecl SerialObjectFactory(void* ctx, void* ar, i32 mode, i32 typeId, void* payload);

#endif // GRUNTZ_IO_GAMESAVE_H
