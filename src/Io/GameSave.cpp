// GameSave.cpp - the top-level Gruntz save-game entry (C:\Proj\Gruntz).
//
// SaveGame validates the host + name, resets the serialize sequence counter and
// the snapshot scratch buffer, then drives the recursive surface-mgr serializer
// (CDDrawSurfaceMgr::SnapshotChildren) over the "Gruntz Save Game" stream. Field
// names are placeholders; only OFFSETS + code bytes matter.
#include <rva.h>
#include <string.h> // strlen / memset inline to repne scasb / rep stos

// The recursive snapshot run-callback signature + the surface-mgr serializer it is
// handed to. Both modeled NO-body so the call/address-of reloc-mask.
typedef i32(__cdecl* SnapRunCallback)(void* mgr, void* ser, i32 mode, i32, i32);
extern i32 __cdecl SaveRunCallback(void* mgr, void* ser, i32 mode, i32, i32); // 0x24e6 thunk

struct CDDrawSurfaceMgr {
    i32 SnapshotChildren(SnapRunCallback cb, i32 arg1, char* name, i32 arg3); // 0x156020
};

// The game-state host being saved; its +0x30 is the bound surface manager. Modeled
// as a view (only +0x30 is touched here); the opaque head is the game-mgr region.
struct CGameSaveHost {
    char m_head[0x30];
    CDDrawSurfaceMgr* m_surfaceMgr; // +0x30  bound surface manager
};

// The 0x24-dword snapshot scratch buffer (zeroed each save) + the serialize
// sequence counter (shared with Grunt.cpp's per-record counter).
extern i32 g_saveBuf[0x24]; // VA 0x629930
DATA(0x00229ad0)
extern i32 g_serialCounter; // VA 0x629ad0

// SaveGame(host, name) - bail on a null host/name or an empty name; reset the
// sequence counter + scratch buffer (buf[0]=1 marks it live); then, when the host
// has a bound surface mgr, run the recursive snapshot and return whether it succeeded.
RVA(0x0000d170, 0x74)
i32 SaveGame(CGameSaveHost* host, char* name) {
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
    CDDrawSurfaceMgr* mgr = host->m_surfaceMgr;
    if (mgr == 0) {
        return 0;
    }
    return mgr->SnapshotChildren(SaveRunCallback, (i32)name, "Gruntz Save Game", 0) != 0;
}

SIZE_UNKNOWN(CGameSaveHost);
