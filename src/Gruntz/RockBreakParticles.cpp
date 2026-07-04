// RockBreakParticles.cpp - BuildRockBreakParticles (0x7b440), a __thiscall
// (this; cx, cy, r, a4) (ret 0x10) per-tile "rock break" pass over a radius of
// tiles centered on (cx>>5, cy>>5). It is the direct sibling of
// CGruntResurrector::LoadGruntResurrectTuning (src/Stub/Backlog.cpp): a doubly-
// nested tile scan that reads each cell's type off the level grid and, by type,
// either drops a "LEVEL_ROCKBREAK" Particlez eye-candy sprite (+ rate-limited
// sound cue, the BootyState::vfunc_9 sound-chain idiom) for a broken-rock tile
// (0x1e/0x1f), resurrects/retires the giant-rock logic object for a giant-rock
// tile (0x21, with the "No giant rock logic found around" CString diagnostic),
// or reconciles the sub-logic for a hazard tile (0x97/0x98/0x99).
//
// The CString diagnostic temp gives it the /GX exception frame -> `eh` unit.
// CARCASS doctrine: every callee is a reloc-masked external __thiscall; the grid /
// logic-registry / sprite-factory / sound classes are unmatched engine classes
// accessed by raw this+offset; strings are $SG literals reloc-masked against the
// matched symbols. Only offsets / code bytes are load-bearing.
#include <Mfc.h>   // MFC CString (default ctor 0x1b9b93 / dtor 0x1b9cde)
#include <Win32.h> // PtInRect / RECT / POINT

#include <Gruntz/SpriteFactory.h> // the ONE CSpriteFactory (CreateSprite @0x1597b0)
#include <Gruntz/UserLogic.h>     // CGameObject (the created sprite)
#include <rva.h>

// FUN_001b2cf5 __cdecl: format into a CString (the LoadBootyCheatState FormatStr).
void FormatStr(CString* out, const char* fmt, ...);

// The eye-candy sprite the factory returns is the shared CGameObject (ApplyName
// @0x150540 / ApplyLookupGeometry @0x1505b0); the factory is the canonical
// CSpriteFactory (m_22c->m_8; <Gruntz/SpriteFactory.h>).

// The rate-limited sound cue (the shared Booty* sound-chain idiom).
struct RockSndPlayer {
    void Play(i32 token, i32, i32, i32); // FUN_001360d0 __thiscall
};
struct RockSndEntry {
    char m_pad00[0x10];
    RockSndPlayer* m_10; // +0x10
    u32 m_14;            // +0x14  last-played stamp
    u32 m_18;            // +0x18  interval
};
struct RockSndTable {
    void Find(const char* name, RockSndEntry** out); // FUN_001b8438 __thiscall
};
struct RockSndSet { // this->m_22c->m_28
    char m_pad00[0x10];
    RockSndTable m_10; // +0x10
    char m_pad11[0x30 - 0x11];
    i32 m_30; // +0x30  active guard
};

// The level tile grid (board->m_5c): a sparse row-offset cell store.
struct RockGrid {
    char m_pad00[0x20];
    i32* m_20; // +0x20  cell array (row-offset + col)
    i32* m_24; // +0x24  per-row base index
    i32 m_28;  // +0x28  storage cols
    i32 m_2c;  // +0x2c  storage rows
    i32 m_30;  // +0x30  active cols
    i32 m_34;  // +0x34  active rows
};
// A cell's type object (board->m_4c[cell & 0xffff]). This is a FOREIGN engine
// class: its ??_7 and the intermediate slots 0..7 are unreconstructed engine code,
// so the only honest model is the ONE dispatched slot. GetType is the __thiscall
// virtual at vtable byte offset +0x20, modeled as a 4-byte PMF loaded from the
// vtable (`char m_pad00[0x20]` documents the un-recovered slots) so `o->GetType(0,0)`
// emits `mov edx,[ecx]; call [edx+0x20]`. The class is COMPLETE before the T::*
// typedef to keep the PMF 4 bytes (docs/patterns/pmf-complete-class-4byte.md).
struct RockCellVtbl;
struct RockCellObj {
    RockCellVtbl* m_vptr; // +0x00
    i32 GetType(i32 a, i32 b);
};
typedef i32 (RockCellObj::*RockGetTypeFn)(i32, i32);
struct RockCellVtbl {
    char m_pad00[0x20];
    RockGetTypeFn GetType; // +0x20
};
inline i32 RockCellObj::GetType(i32 a, i32 b) {
    return (this->*(m_vptr->GetType))(a, b);
}
struct RockBoard { // this->m_22c->m_24 (and g_mgrSettings->m_world->m_24)
    char m_pad00[0x4c];
    RockCellObj** m_4c; // +0x4c  cell type-object table
    char m_pad50[0x5c - 0x50];
    RockGrid* m_5c; // +0x5c
};
struct RockMapHost { // this->m_22c (== g_mgrSettings->m_world)
    char m_pad00[0x8];
    CSpriteFactory* m_8; // +0x08
    char m_pad0c[0x24 - 0xc];
    RockBoard* m_24;  // +0x24
    RockSndSet* m_28; // +0x28
};

// The per-cell "logic object" the registry hands out.
struct RockLogicObj {
    void Retire();      // FUN_00003af8 __thiscall
    void SetType(i32);  // FUN_00001a00 __thiscall
    i32 Reconcile(i32); // FUN_00003adf __thiscall
};
struct RockLogicMgr {                         // g_mgrSettings->m_curState->m_2e4
    RockLogicObj* FindAt(i32 x, i32 y);       // FUN @ 0x377e __thiscall
    RockLogicObj* FindCell(i32 cellId);       // FUN @ 0x2838 __thiscall
    RockLogicObj* Acquire(i32 cellId, i32 f); // FUN @ 0x21df __thiscall
    void Release(RockLogicObj* o);            // FUN @ 0x2581 __thiscall
    void Reap(RockLogicObj* o);               // FUN @ 0x3aa8 __thiscall
};
struct RockSettingsRoot { // g_mgrSettings->m_curState
    char m_pad00[0x2e4];
    RockLogicMgr* m_2e4; // +0x2e4
};
struct RockNotify {                        // g_mgrSettings->m_tileGrid
    void OnCellSet(i32 x, i32 y, i32 val); // FUN @ 0x33f0 __thiscall
};
struct RockMgr { // g_mgrSettings (*0x64556c), this method's typed alias
    char m_pad00[0x2c];
    RockSettingsRoot* m_curState; // +0x2c
    RockMapHost* m_world;         // +0x30  write-grid host
    char m_pad34[0x70 - 0x34];
    RockNotify* m_tileGrid; // +0x70
    char m_pad74[0x13c - 0x74];
    RECT m_13c;                    // +0x13c  visible rect
    void Log(const char* s);       // FUN @ 0x417e __thiscall
    void Pump(i32 msg, i32 param); // FUN @ 0x346d __thiscall
};
DATA(0x0024556c)
extern "C" RockMgr* g_mgrSettings; // _g_mgrSettings
DATA(0x0021ab20)
extern i32 g_sndEnabled; // ?g_sndEnabled@@3HA
DATA(0x0021ab24)
extern i32 g_sndCueTag; // ?g_sndCueTag@@3HA
DATA(0x002bf3c0)
extern "C" u32 g_killCueClock; // _g_killCueClock (wrap-safe draw clock)

class CRockBreakMgr {
public:
    void Prepare(i32 cx, i32 cy, i32 r, i32, i32); // FUN @ 0x400c __thiscall
    i32 BuildRockBreakParticles(i32 cx, i32 cy, i32 r, i32 a4);

    char m_pad00[0x22c];
    RockMapHost* m_22c; // +0x22c
};

// @source: string-xref
// @early-stop
// regalloc/loop-strength-reduction wall: the logic - the M400c prep call, the
// radius rect, the doubly-nested tile scan, the col/row clamp, the cell-type
// dispatch (0x1e/0x1f rock-break + Particlez sprite + rate-limited sound,
// 0x21 giant-rock resurrect + "No giant rock logic found" CString diagnostic,
// 0x97/0x98/0x99 hazard reconcile), the PtInRect gate and the return-1/return-0
// tails - is byte-faithful (instruction selection, calls, constants, strings). The
// residual is the induction-variable set: retail carries pixelX/pixelY, the packed
// (tileX<<8) cell base and the loop bounds as spilled strength-reduced accumulators
// on a 0x1c frame, and pins the two loop counters (tileX->ebp, tileY->edi) with a
// specific spill coloring the /O2 recompile re-derives differently, cascading a
// +N [esp+N] shift across the body. Not source-steerable (see
// docs/patterns/loop-invariant-multiply-strength-reduce-vs-memreread.md +
// zero-register-pinning.md). Logic complete.
RVA(0x0007b440, 0x3f0)
i32 CRockBreakMgr::BuildRockBreakParticles(i32 cx, i32 cy, i32 r, i32 a4) {
    Prepare(cx, cy, r, 6, a4);

    RockSettingsRoot* root = g_mgrSettings->m_curState;
    i32 tileCx = cx >> 5;
    i32 tileCy = cy >> 5;
    i32 hiX = tileCx + r;
    for (i32 tx = tileCx - r; tx <= hiX; tx++) {
        i32 pxX = (tx << 5) + 0x10;
        for (i32 ty = tileCy - r; ty <= tileCy + r; ty++) {
            i32 pxY = (ty << 5) + 0x10;
            if (pxX < 0x10 || pxY < 0x10) {
                continue;
            }
            RockBoard* board = m_22c->m_24;
            RockGrid* grid = board->m_5c;
            if (tx >= grid->m_30 || ty >= grid->m_34) {
                continue;
            }
            i32 col;
            if (pxX < 0x10) {
                col = 0;
            } else if (tx >= grid->m_28) {
                col = grid->m_28 - 1;
            } else {
                col = tx;
            }
            i32 row = (ty >= grid->m_2c) ? grid->m_2c - 1 : ty;
            i32 cell = grid->m_20[grid->m_24[row] + col];
            i32 type;
            if (cell == (i32)0xeeeeeeee || cell == -1) {
                type = 0;
            } else {
                RockCellObj* o = board->m_4c[cell & 0xffff];
                type = o->GetType(0, 0);
            }

            if (type != 0x1e && type != 0x1f) {
                if (type == 0x21) {
                    RockLogicObj* gr = root->m_2e4->FindAt(tx, ty);
                    if (gr == 0) {
                        CString msg;
                        FormatStr(&msg, "No giant rock logic found around: x=%d, y=%d", cx, cy);
                        g_mgrSettings->Log(msg);
                        g_mgrSettings->Pump(0x80dd, 0x403);
                        return 0;
                    }
                    gr->Retire();
                    root->m_2e4->Release(gr);
                    continue;
                }
                if (type != 0x97 && type != 0x98 && type != 0x99) {
                    continue;
                }
                RockLogicObj* o = root->m_2e4->FindCell(ty + (tx << 8));
                if (o->Reconcile(0)) {
                    root->m_2e4->Reap(o);
                }
                continue;
            }

            // type == 0x1e || type == 0x1f: rock-break marker + particle
            RockLogicObj* lo = root->m_2e4->Acquire(ty + (tx << 8), 0x1a);
            if (lo != 0) {
                lo->SetType(type);
                root->m_2e4->Release(lo);
            } else {
                RockGrid* wg = g_mgrSettings->m_world->m_24->m_5c;
                i32 off = wg->m_24[ty];
                if (type == 0x1e) {
                    wg->m_20[off + tx] = 0x5a;
                    g_mgrSettings->m_tileGrid->OnCellSet(tx, ty, 0x5a);
                } else {
                    wg->m_20[off + tx] = 0x5b;
                    g_mgrSettings->m_tileGrid->OnCellSet(tx, ty, 0x5b);
                }
            }

            POINT pt;
            pt.x = pxX;
            pt.y = pxY;
            if (!PtInRect(&g_mgrSettings->m_13c, pt)) {
                continue;
            }
            CGameObject* spr =
                m_22c->m_8->CreateSprite(0, pxX, pxY, 0xcf84f, "Particlez", 0x40003);
            if (spr == 0) {
                continue;
            }
            spr->ApplyName("LEVEL_ROCKBREAK");
            spr->ApplyLookupGeometry("LEVEL_ROCKBREAK", 0);

            RockSndSet* set = m_22c->m_28;
            if (set->m_30 == 0) {
                RockSndEntry* e = 0;
                set->m_10.Find("LEVEL_ROCKBREAK", &e);
                if (e != 0 && g_sndEnabled != 0) {
                    u32 now = g_killCueClock;
                    if (now - e->m_14 >= e->m_18) {
                        e->m_14 = now;
                        e->m_10->Play(g_sndCueTag, 0, 0, 0);
                    }
                }
            }
        }
    }
    return 1;
}
SIZE_UNKNOWN(CRockBreakMgr);
SIZE_UNKNOWN(RockBoard);
SIZE_UNKNOWN(RockCellObj);
SIZE_UNKNOWN(RockCellVtbl);
SIZE_UNKNOWN(RockGrid);
SIZE_UNKNOWN(RockLogicMgr);
SIZE_UNKNOWN(RockLogicObj);
SIZE_UNKNOWN(RockMapHost);
SIZE_UNKNOWN(RockMgr);
SIZE_UNKNOWN(RockNotify);
SIZE_UNKNOWN(RockSettingsRoot);
SIZE_UNKNOWN(RockSndEntry);
SIZE_UNKNOWN(RockSndPlayer);
SIZE_UNKNOWN(RockSndSet);
SIZE_UNKNOWN(RockSndTable);
