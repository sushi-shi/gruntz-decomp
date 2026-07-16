// GameSave.h - the top-level Gruntz save-game entry (C:\Proj\Gruntz).
#ifndef GRUNTZ_IO_GAMESAVE_H
#define GRUNTZ_IO_GAMESAVE_H

#include <Ints.h>
#include <rva.h>

class CDDrawSurfaceMgr; // <DDrawMgr/DDrawSurfaceMgr.h> - the bound surface manager (+0x30)

// The game-state host SaveGame is handed. PROVEN identity: CSaveGame::Save (0xe4ea0)
// passes ds:0x64556c == g_gameReg (the CGruntzMgr singleton) as arg1 (0xe4f83:
// `mov eax,ds:0x64556c; push eax; call SaveGame`), so this IS the CGruntzMgr facet -
// its +0x30 is the CDDrawSurfaceMgr. Kept a MINIMAL typed view (not folded onto the
// full CGruntzMgr) so the standalone save entry stays decoupled from the fat game-mgr
// header; only +0x30 is touched. The opaque head is the game-mgr region.
struct CGameSaveHost {
    char m_head[0x30];
    CDDrawSurfaceMgr* m_surfaceMgr; // +0x30  bound surface manager
};

// SaveGame(host, name) - reset the serialize counter + scratch buffer, then drive the
// recursive surface-mgr serializer (CDDrawSurfaceMgr::SnapshotChildren). 0x0000d170.
i32 SaveGame(CGameSaveHost* host, char* name);

#endif // GRUNTZ_IO_GAMESAVE_H
