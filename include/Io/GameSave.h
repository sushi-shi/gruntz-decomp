#ifndef GRUNTZ_IO_GAMESAVE_H
#define GRUNTZ_IO_GAMESAVE_H

#include <Ints.h>
#include <rva.h>

// The save host IS the game-manager singleton: the sole caller
// (CSaveGame::Save @0xe4ea0) passes g_gameReg, and the one field the body reads,
// +0x30, is CGruntzMgr::m_world. (The ex `CGameSaveHost` 0x30-pad view is gone.)
class CGruntzMgr;

i32 SaveGame(CGruntzMgr* host, char* name);

// C linkage: the C++ array-global mangling diverges clang vs MSVC5 (the
// data-binding gotcha); the owner def in GameSave.cpp inherits it from here.
extern "C" i32 g_saveBuf[0x24]; // 0x229930  the 0x90-byte save header scratch

extern i32 g_savedMenuCmd; // 0x00213a9c (-1; the deferred menu WM_COMMAND)

// File-scope prototypes moved from the .cpp: an unqualified
// declaration at file scope has EXTERNAL linkage, so it belongs in
// the owner header.
i32 __cdecl SerialObjectFactory(void* ctx, void* ar, i32 mode, i32 typeId, void* payload);

#endif // GRUNTZ_IO_GAMESAVE_H
