// VehicleGruntSprites.cpp - LoadVehicleGruntSprites (graduated from
// src/Stub/Backlog.cpp). A __thiscall method on a grunt-command game object: it
// records the requested vehicle-grunt kind, seeds two 4-int region blocks, picks
// the matching sprite-set name (a 10-way jump table over kinds 0x17..0x20), asks
// the game registry to register the named sprite set, then - if the grunt's tile
// is an 'A'/'B' action tile at its current cell - fires the two follow-up object
// registrations. A destructible MFC CString temp gives it the /GX exception frame.
//
// IDENTITY recovered by string-xref (the BABYWALKER/BEACHBALL/.../YOYO "*GRUNT"
// set names) + g_mgrSettings. Only offsets / code bytes are load-bearing; the
// owning class, registry sub-objects and every callee are unmatched engine code
// reached by raw this+offset / reloc-masked external __thiscall thunks.

#include <Mfc.h> // CString temp (/GX)

#include <rva.h>

#include <Gruntz/TileGrid.h>   // the registry +0x70 tile occupancy grid
#include <Gruntz/GruntzMgr.h>  // canonical MFC-side g_gameReg singleton view (CGruntzMgr)
#include <Gruntz/PickupType.h> // the shared object/pickup/grunt-kind type id space

// The game registry singleton (*0x24556c) - the canonical MFC-side CGruntzMgr view.
// Its +0x2c slot registers a named sprite set (cast to CSpriteSetReg); its +0x70
// tile grid (m_cmdNotify, cast to CTileGrid) has m_8 = row-pointer array, each cell
// 0x1c bytes with the tile code at +0x10, indexed by the >>5 tile coords.
struct CSpriteSetReg {
    void Register(CString* name, i32 a, i32 b, i32 c); // 0x2bc1 __thiscall
};
DATA(0x0024556c)
extern CGruntzMgr* g_gameReg;

// The grunt-command object's follow-up registrar (this->m_260).
struct CGruntCmdObj;
struct CGruntRegistrar {
    void RegisterA(CGruntCmdObj* obj, i32 x, i32 y); // 0x26df __thiscall
    void RegisterB(CGruntCmdObj* obj, i32 x, i32 y); // 0x3dfa __thiscall
};
// The grunt's current-tile anchor (this->m_10): m_5c/m_60 are its committed coords.
struct CGruntAnchor {
    char m_pad0[0x5c];
    i32 m_5c; // +0x5c
    i32 m_60; // +0x60
};

struct CGruntCmdObj {
    i32 LoadVehicleGruntSprites(i32 kind);

    char m_pad0[0x10];
    CGruntAnchor* m_10; // +0x10
    char m_pad14[0x17c - 0x14];
    i32 m_17c; // +0x17c  tile X (pixels)
    i32 m_180; // +0x180  tile Y (pixels)
    char m_pad184[0x198 - 0x184];
    i32 m_198; // +0x198  requested kind
    char m_pad19c[0x1a0 - 0x19c];
    i32 m_1a0; // +0x1a0
    char m_pad1a4[0x260 - 0x1a4];
    CGruntRegistrar* m_260; // +0x260
    char m_pad264[0x2b0 - 0x264];
    i32 m_region0[4]; // +0x2b0
    i32 m_region1[4]; // +0x2c0
};

// The toy/vehicle-grunt kind LoadVehicleGruntSprites dispatches on (kind, 0x17..0x20)
// is the toy band of PickupType, the shared object/pickup/grunt-kind id space in
// <Gruntz/PickupType.h>. Each name is confirmed by its case's "<NAME>GRUNT" sprite key;
// the ids are byte-verified to equal PickupType's toys (BABYWALKER=0x17..YOYO=0x20).
// Same immediates as the bare labels -> naming is matching-neutral.

// @early-stop
// ~37%: COMPLETE + correct (prologue/dispatch/jump-table/common CString-tail/
// tile-A-B gate + the two registrations all model retail). Residual is a /O2
// constant-hoisting regalloc wall, NOT a source bug: the 10 switch arms share the
// {-1,-1,1,1}/{0,0,0,0} region constants, so this cl hoists -1/1/0 into callee-
// saved ebx/ebp/edi above the CString ctor (one esi-relative store per field),
// whereas retail re-materializes them per-arm (lea ebx,[esi+0x2b0]; or eax,-1; mov
// [ebx],eax ...) keeping kind/this in edi/esi. Same /O2 /Oy /GX frame; pointer-vs-
// index spelling is normalized away (verified identical bytes both ways), so the
// hoist is a cl build-8034 heuristic difference. Deferred to the final sweep.
RVA(0x00050ce0, 0x399)
i32 CGruntCmdObj::LoadVehicleGruntSprites(i32 kind) {
    m_198 = kind;
    m_1a0 = -1;

    CString name;
    i32* r0;
    switch (kind) {
        case PICKUP_BABYWALKER:
            r0 = m_region0;
            r0[0] = -1;
            r0[1] = -1;
            r0[2] = 1;
            r0[3] = 1;
            r0 = m_region1;
            r0[0] = 0;
            r0[1] = 0;
            r0[2] = 0;
            r0[3] = 0;
            name = "BABYWALKERGRUNT";
            break;
        case PICKUP_BEACHBALL:
            r0 = m_region0;
            r0[0] = -1;
            r0[1] = -1;
            r0[2] = 1;
            r0[3] = 1;
            r0 = m_region1;
            r0[0] = 0;
            r0[1] = 0;
            r0[2] = 0;
            r0[3] = 0;
            name = "BEACHBALLGRUNT";
            break;
        case PICKUP_BIGWHEEL:
            r0 = m_region0;
            r0[0] = -1;
            r0[1] = -1;
            r0[2] = 1;
            r0[3] = 1;
            r0 = m_region1;
            r0[0] = 0;
            r0[1] = 0;
            r0[2] = 0;
            r0[3] = 0;
            name = "BIGWHEELGRUNT";
            break;
        case PICKUP_GOKART:
            r0 = m_region0;
            r0[0] = -1;
            r0[1] = -1;
            r0[2] = 1;
            r0[3] = 1;
            r0 = m_region1;
            r0[0] = 0;
            r0[1] = 0;
            r0[2] = 0;
            r0[3] = 0;
            name = "GOKARTGRUNT";
            break;
        case PICKUP_JACKINTHEBOX:
            r0 = m_region0;
            r0[0] = -1;
            r0[1] = -1;
            r0[2] = 1;
            r0[3] = 1;
            r0 = m_region1;
            r0[0] = 0;
            r0[1] = 0;
            r0[2] = 0;
            r0[3] = 0;
            name = "JACKINTHEBOXGRUNT";
            break;
        case PICKUP_JUMPROPE:
            r0 = m_region0;
            r0[0] = -1;
            r0[1] = -1;
            r0[2] = 1;
            r0[3] = 1;
            r0 = m_region1;
            r0[0] = 0;
            r0[1] = 0;
            r0[2] = 0;
            r0[3] = 0;
            name = "JUMPROPEGRUNT";
            break;
        case PICKUP_POGOSTICK:
            r0 = m_region0;
            r0[0] = -1;
            r0[1] = -1;
            r0[2] = 1;
            r0[3] = 1;
            r0 = m_region1;
            r0[0] = 0;
            r0[1] = 0;
            r0[2] = 0;
            r0[3] = 0;
            name = "POGOSTICKGRUNT";
            break;
        case PICKUP_SCROLL:
            r0 = m_region0;
            r0[0] = -1;
            r0[1] = -1;
            r0[2] = 1;
            r0[3] = 1;
            r0 = m_region1;
            r0[0] = 0;
            r0[1] = 0;
            r0[2] = 0;
            r0[3] = 0;
            name = "SCROLLGRUNT";
            break;
        case PICKUP_SQUEAKTOY:
            r0 = m_region0;
            r0[0] = -1;
            r0[1] = -1;
            r0[2] = 1;
            r0[3] = 1;
            r0 = m_region1;
            r0[0] = 0;
            r0[1] = 0;
            r0[2] = 0;
            r0[3] = 0;
            name = "SQUEAKTOYGRUNT";
            break;
        case PICKUP_YOYO:
            r0 = m_region0;
            r0[0] = -1;
            r0[1] = -1;
            r0[2] = 1;
            r0[3] = 1;
            r0 = m_region1;
            r0[0] = 0;
            r0[1] = 0;
            r0[2] = 0;
            r0[3] = 0;
            name = "YOYOGRUNT";
            break;
        default:
            break;
    }

    ((CSpriteSetReg*)g_gameReg->m_curState)->Register(&name, 1, 1, 0);

    i32 code = ((i32*)((CTileGrid*)g_gameReg->m_cmdNotify)->m_8[m_180 >> 5])[(m_17c >> 5) * 7 + 4];
    if (code == 0x41 || code == 0x42) {
        if (m_10->m_5c == m_17c && m_10->m_60 == m_180) {
            m_260->RegisterA(this, m_17c, m_180);
            m_260->RegisterB(this, m_17c, m_180);
        }
    }
    return 1;
}
SIZE_UNKNOWN(CGruntAnchor);
SIZE_UNKNOWN(CGruntCmdObj);
SIZE_UNKNOWN(CGruntRegistrar);
SIZE_UNKNOWN(CSpriteSetReg);
SIZE_UNKNOWN(CTileCell);
