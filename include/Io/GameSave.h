#ifndef GRUNTZ_IO_GAMESAVE_H
#define GRUNTZ_IO_GAMESAVE_H

#include <Ints.h>
#include <rva.h>

class CDDrawSurfaceMgr; // <DDrawMgr/DDrawSurfaceMgr.h> - the bound surface manager (+0x30)

struct CGameSaveHost {
    char m_head[0x30];
    CDDrawSurfaceMgr* m_surfaceMgr; // +0x30  bound surface manager
};

i32 SaveGame(CGameSaveHost* host, char* name);

#endif // GRUNTZ_IO_GAMESAVE_H
