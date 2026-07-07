// GameModeObjLifecycle.cpp - level lifecycle methods on the big in-game game-mode
// object (C:\Proj\Gruntz), the same physical class as CPlayLevelLoad
// (LoadLevelByMode.cpp) and the EngineLabelBacklog status-bar object
// (StatusBarUpdaters.cpp): a >0x520-byte PLAY-state object holding the cursor
// sprite set (+0x2f4..+0x2fc/+0x504), a worker sub-object (+0x2dc), a 4-record
// CPtrArray table of placed objects (+0x3a8, 0x14 stride), the 4 team/color
// permutation slots, and the level/init state words.
//
// These were the `StatusBarItem` placeholder cluster in src/Stub/Discovered.cpp;
// they are NOT CStatusBarItem (a ~0x2c-byte HUD item) methods - they are lifecycle
// steps on this big object, reached __thiscall from LoadLevelByMode (0xca200),
// CPlay::CPlay_0d1b60 (0xd1b60), the CGruntzMgr dispatch region (0x86xxx), and
// UpdateStatusBarTabHighlight (0xfe910). The fourth method of the cluster
// (RandomizeStartColors @0xd9290, a 679-B /GX EH function) stays stubbed in
// src/Stub/Discovered.cpp for the final sweep.
//
// Members are accessed by raw this+offset (the naming-independent choice the
// siblings use); only offsets / code bytes are load-bearing.
#include <Mfc.h> // real MFC CPtrArray (the placed-object table's RemoveAt @0x1b5200)
#include <rva.h>
#include <Gruntz/SBI_RectOnly.h>

// ---------------------------------------------------------------------------
// Shared singletons (named so their DIR32 operands reloc-mask; anchored in the
// sibling TUs). g_gameReg is the WwdGameReg manager singleton (*0x64556c); its
// +0x134 is the game mode (1 single / 2 multi), +0x70 the tile/map notifier.
// ---------------------------------------------------------------------------
struct WwdGameRegZ;
extern WwdGameRegZ* g_gameReg; // ?g_gameReg@@3PAUWwdGameRegZ@@A @0x64556c
extern i32 g_dat645588;        // ?g_dat645588@@3HA (the running game clock)

// MFC heap free-list internals the CPtrArray::RemoveAt path threads (named so
// their DIR32 operands reloc-mask; CRT/MFC-owned globals).
extern void* g_freeList;       // ?g_freeList@@3PAXA @0x645544
extern i32 g_freeListNodeBias; // ?g_freeListNodeBias@@3HA @0x64554c

// ---------------------------------------------------------------------------
// Engine helpers reached through reloc-masked __thiscall ILT thunks (no body).
// ---------------------------------------------------------------------------
// The +0x2dc worker sub-object: its teardown @0x50b210 is a no-arg __thiscall
// (this in ecx); modeled as a method so `mov ecx,worker; call` falls out with no
// stack cleanup. External (no body) so the `call rel32` reloc-masks.
// CPtrArray::RemoveAt @0x1b5200 and CMapPtrToPtr::Lookup @0x1b8760 - MFC; modeled
// as methods so the __thiscall `mov ecx,this; call` falls out reloc-masked.
struct CPlacedObj; // defined below
// The +0x3a8 placed-object table (0x14 stride) is an MFC CPtrArray; RemoveAt @0x1b5200 is
// CPtrArray::RemoveAt. Field-view kept (MFC hides m_pData); the call casts to CPtrArray.
class CPlacedArray {
public:
    CPlacedObj** m_pData; // +0x00  the element pointer table
    i32 m_nSize;          // +0x04  live element count
};
class CCellMap {
public:
    i32 Lookup(void* key, void** out); // 0x1b8760  (this = grid->m_30->m_8 + 0x48)
};

// The placed map object: m_0/m_4 are its grid (x,y) cell coordinates.
struct CPlacedObj {
    i32 m_0; // +0x00  cell X
    i32 m_4; // +0x04  cell Y
};

// The tile/map grid notifier reached at g_gameReg->m_tileGrid. m_8 is the row table
// (a per-row cell array), m_c/m_10 the (width,height) bounds; each cell is 0x1c
// bytes (x*7 dwords) with the occupant object at +0x8 and the flags word at +0x0.
struct CMapGrid {
    char m_pad00[0x8];
    i32** m_8; // +0x08  row table: m_8[y] -> the row's cell array
    i32 m_c;   // +0x0c  grid width
    i32 m_10;  // +0x10  grid height
};

// g_gameReg view used here: +0x70 the map grid, +0x30 the resource holder whose
// m_8 carries the cell->object map (queried at +0x48).
struct GmReg {
    char m_pad00[0x30];
    struct M30 {
        char m_pad00[0x8];
        char* m_8; // +0x08  the holder the cell map hangs at (+0x48)
    }* m_world;    // +0x30
    char m_pad34[0x68 - 0x34];
    struct M68 {
        char m_pad00[0x2a8];
        i32 m_2a8; // +0x2a8  a pending-flag the teardown clears
    }* m_68;       // +0x68
    char m_pad6c[0x70 - 0x6c];
    CMapGrid* m_tileGrid; // +0x70  the tile/map grid
};

// ---------------------------------------------------------------------------
// CGameModeObj - the big PLAY-state game-mode object (raw this+offset access).
// ---------------------------------------------------------------------------
class CGameModeObj {
public:
    // LoadCursorSprites @0xd0120: __thiscall, caches the cursor sprite set,
    // comparing the request against the latched (+0x2f8,+0x504) pair. External
    // (no body here) so the `call rel32` reloc-masks.
    i32 LoadCursorSprites(i32 frame, i32 flag);
    i32 SetCursorFrame(i32 frame); // 0x0d1b30
    i32 ReleaseLevelOverlay(i32);  // 0x0d6560
    i32 ClearPlacedObjects();      // 0x0da030
    i32 FlushPendingOps();         // 0x0da2d0
    void Sub17a8(i32 a);           // (thiscall, 1 arg)  reloc-masked ILT thunk
    void Sub35da(i32 a, i32 b);    // (thiscall, 2 args) reloc-masked ILT thunk
    // RandomizeStartColors (0x0d9290) - the 679-B /GX EH method - stays stubbed in
    // src/Stub/Discovered.cpp for the final sweep (don't half-do a big EH fn).
    char m_pad[4];
};

#define I32(p, off) (*(i32*)((char*)(p) + (off)))
#define PTR(p, off) (*(void**)((char*)(p) + (off)))

// ===========================================================================
// CGameModeObj::SetCursorFrame - cache the cursor sprite for `frame` and latch
// it as the active cursor (+0x2f4). Returns 1.
RVA(0x000d1b30, 0x20)
i32 CGameModeObj::SetCursorFrame(i32 frame) {
    LoadCursorSprites(frame, 0);
    I32(this, 0x2f4) = frame;
    return 1;
}

// ===========================================================================
// CGameModeObj::ReleaseLevelOverlay - if the level overlay (+0x4fc) is live,
// tear down its worker (+0x2dc), clear the flag, and (unless multiplayer,
// g_gameReg->m_134 == 2) publish the level's start clock (+0x1cc) to the running
// game clock. When the overlay was already down, return immediately. Returns 1.
// The single stack arg is unused.
RVA(0x000d6560, 0x45)
i32 CGameModeObj::ReleaseLevelOverlay(i32) {
    if (I32(this, 0x4fc) != 0) {
        CSBI_RectOnly* worker = (CSBI_RectOnly*)PTR(this, 0x2dc);
        I32(this, 0x4fc) = 0;
        worker->ExitMode();
        if (I32(g_gameReg, 0x134) != 2) {
            g_dat645588 = I32(this, 0x1cc);
        }
    }
    return 1;
}

// Typed view of the manager singleton for the grid walk (same address/symbol).
static GmReg* gmReg() {
    return (GmReg*)g_gameReg;
}

// ===========================================================================
// @early-stop
// 361-B map-grid free-list walk. The control flow (the 4 placed-object record
// blocks at +0x3a8, the per-block element walk with the restart flag, the grid
// (x,y) occupant lookup with the x*7-dword cell stride, the cell-map class
// query, the cell-flag clear + CPtrArray::RemoveAt + free-list node return) is a
// faithful reconstruction of retail's logic. The residual is the dual-exit
// regalloc/scheduling wall: MSVC5 pins blockIdx/ebx/edi/esi across the nested
// loops and threads the restart `edi` flag through the block advance in a way
// the C source can't steer (zero-register-pinning.md). Deferred to the final sweep.
//
// CGameModeObj::ClearPlacedObjects - sweep the 4 placed-object record blocks; for
// each still-occupied grid cell whose map entry is gone (or not type 0x14), clear
// the cell, remove the object from its record array, and free the node. Returns
// the block index on an early-out, else -1.
RVA(0x000da030, 0x169)
i32 CGameModeObj::ClearPlacedObjects() {
    CPlacedArray* recs = (CPlacedArray*)((char*)this + 0x3a8);
    for (i32 blockIdx = 0; blockIdx < 4; ++blockIdx) {
        CPlacedArray* rec = (CPlacedArray*)((char*)recs + blockIdx * 0x14);
        i32 i = 0;
        i32 restart = 0;
        while (i < rec->m_nSize) {
            CPlacedObj* obj = rec->m_pData[i];
            CMapGrid* grid = gmReg()->m_tileGrid;
            i32* cellObj = 0;
            if ((u32)obj->m_0 < (u32)grid->m_c && (u32)obj->m_4 < (u32)grid->m_10) {
                i32 stride = obj->m_0 * 7;
                i32* row = grid->m_8[obj->m_4];
                cellObj = (i32*)row[stride + 2];
            }
            if (cellObj == 0) {
                restart = 1;
                break;
            }
            void* out = 0;
            CCellMap* map = (CCellMap*)(gmReg()->m_world->m_8 + 0x48);
            i32* result = cellObj;
            if (map->Lookup(cellObj, &out)) {
                result = (i32*)out;
            }
            if (result == 0) {
                // cell vacated: clear the cell's occupant + flag bit and unlink.
                CMapGrid* g = gmReg()->m_tileGrid;
                if ((u32)obj->m_0 < (u32)g->m_c && (u32)obj->m_4 < (u32)g->m_10) {
                    i32 stride = obj->m_0 * 7;
                    i32* row = g->m_8[obj->m_4];
                    row[stride + 2] = 0;
                    i32* row2 = g->m_8[obj->m_4];
                    row2[stride] &= 0xfffbffff;
                }
                ((CPtrArray*)rec)->RemoveAt(i, 1);
                // return the placed-object node to the MFC free list (the node
                // header sits g_freeListNodeBias bytes before the payload).
                void* node = (char*)obj - g_freeListNodeBias;
                *(void**)node = g_freeList;
                g_freeList = node;
                return -1;
            }
            if (I32(result, 0x124) != 0x14) {
                restart = 1;
            }
            ++i;
            if (restart) {
                break;
            }
        }
        if (i >= rec->m_nSize && !restart) {
            if (i > 0) {
                return blockIdx;
            }
            restart = 1;
        }
        (void)restart;
    }
    return -1;
}

// ===========================================================================
// CGameModeObj::FlushPendingOps (0x0da2d0) - if the object hasn't already been
// finalized (+0x4f0), flush the two deferred worker operations gated by +0x368
// and +0x36c (each running a worker step + a this-side sub-step), then clear the
// registry's m_68 pending flag (+0x2a8) and run the final sub-step. Returns 1 if
// any of the three was pending, else 0. Migrated from engine_boundary
// (CGameModeObj: reads the +0x2dc worker, +0x2f4 cursor sprite, +0x368/+0x36c).
RVA(0x000da2d0, 0xa5)
i32 CGameModeObj::FlushPendingOps() {
    if (I32(this, 0x4f0) != 0) {
        return 0;
    }
    i32 changed = 0;
    if (I32(this, 0x368) != 0) {
        CSBI_RectOnly* worker = (CSBI_RectOnly*)PTR(this, 0x2dc);
        I32(this, 0x368) = 0;
        worker->CommitSlot(0);
        Sub17a8(0);
        changed = 1;
    }
    if (I32(this, 0x36c) != 0) {
        i32 spr = I32(this, 0x2f4);
        CSBI_RectOnly* worker = (CSBI_RectOnly*)PTR(this, 0x2dc);
        I32(this, 0x36c) = 0;
        worker->EnterHlRow(0, spr);
        Sub17a8(0);
        changed = 1;
    }
    GmReg::M68* o = ((GmReg*)g_gameReg)->m_68;
    if (I32(o, 0x2a8) != 0) {
        changed = 1;
    }
    I32(o, 0x2a8) = 0;
    Sub35da(0, 0);
    return changed;
}

SIZE_UNKNOWN(CPlacedArray);
SIZE_UNKNOWN(CCellMap);
SIZE_UNKNOWN(CPlacedObj);
SIZE_UNKNOWN(CMapGrid);
SIZE_UNKNOWN(GmReg);
SIZE_UNKNOWN(CGameModeObj);
