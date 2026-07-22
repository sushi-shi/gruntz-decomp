#ifndef GRUNTZ_IO_GAMESAVE_H
#define GRUNTZ_IO_GAMESAVE_H

#include <Ints.h>
#include <rva.h>

class CDDrawSurfaceMgr; // <DDrawMgr/DDrawSurfaceMgr.h> - the bound surface manager (+0x30)

struct CGameSaveHost {
    char m_head[0x30];
    CDDrawSurfaceMgr* m_surfaceMgr; // +0x30  bound surface manager
};
SIZE_UNKNOWN();

i32 SaveGame(CGameSaveHost* host, char* name);

// C linkage: the C++ array-global mangling diverges clang vs MSVC5 (the
// data-binding gotcha); the owner def in GameSave.cpp inherits it from here.
extern "C" i32 g_saveBuf[0x24]; // 0x229930  the 0x90-byte save header scratch

extern i32 g_savedMenuCmd; // 0x00213a9c (-1; the deferred menu WM_COMMAND)


// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---
extern i32 __cdecl
SaveRunCallback(void* mgr, void* ser, i32 mode, i32, i32); // ILT thunk 0x24e6 -> 0xd2a0

#endif // GRUNTZ_IO_GAMESAVE_H
