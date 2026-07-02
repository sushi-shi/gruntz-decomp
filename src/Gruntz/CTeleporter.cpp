// CTeleporter.cpp - the teleporter tile-logic game object (C:\Proj\Gruntz).
//
// Two trace-discovered CTeleporter methods, defined in ascending retail-RVA order:
//   ~CTeleporter @0x010dd0 - the /GX leaf dtor (folds the CUserLogic teardown).
//   Begin        @0x0419e0 - the anim-driven one-shot geometry/bute bring-up.
//
// CTeleporter : CUserLogic (RTTI .?AVCTeleporter@@). Only offsets / code bytes
// are load-bearing; names are placeholders for the recovered engine identities.
#include <Gruntz/CTeleporter.h>
#include <Gruntz/ActReg.h>             // shared activation-registrar archetype (CTeleporterActReg)
#include <Gruntz/CTeleSpriteFactory.h> // shared teleporter HUD-sprite factory
#include <Gruntz/CGameRegistry.h>

// The bound CGameObject viewed through m_10 by the bring-up: its +0x7c sub-object
// carries the per-tile-time at +0xbc (the SAME shape CPathHazard reads as
// CPathObj). Only the touched offsets are modeled; it overlays CGameObject.
struct CTeleBoundOwner {
    char m_pad00[0x7c];
    CTeleBoundOwner* m_7c; // +0x7c  per-tile-time owner
    char m_pad80[0xbc - 0x80];
    i32 m_bc; // +0xbc  per-tile time
};

// ===========================================================================
// CTeleporter::Update (0x41aa0) collaborators (modeled NO-body / by offset so
// the calls + field loads reloc-mask against the engine symbols).
// ===========================================================================

// The bound / spawned visual object (CGameObject) the tick reads + re-seeds.
struct CTeleVisualAux {
    char m_pad00[0xbc];
    i32 m_bc; // +0xbc  per-tile time
};
struct CTeleVisual {
    void ApplyLookupGeometry(const char* key, i32 flag); // 0x1505b0
    char m_pad00[0x8];
    i32 m_8; // +0x08  render-dirty flags
    char m_pad0c[0x40 - 0xc];
    i32 m_40; // +0x40  geometry flag
    char m_pad44[0x5c - 0x44];
    i32 m_5c; // +0x5c  screen X
    i32 m_60; // +0x60  screen Y
    char m_pad64[0x7c - 0x64];
    CTeleVisualAux* m_7c; // +0x7c
    char m_pad80[0x114 - 0x80];
    i32 m_114; // +0x114
    i32 m_118; // +0x118
    i32 m_11c; // +0x11c  tile column (Teleporter spawn)
    i32 m_120; // +0x120  tile row (Teleporter spawn)
    i32 m_124; // +0x124  command/mode (==1, ==2)
    i32 m_128; // +0x128
    char m_pad12c[0x164 - 0x12c];
    i32 m_164; // +0x164
    i32 m_168; // +0x168
    char m_pad16c[0x1b4 - 0x16c];
    i32 m_1b4; // +0x1b4  geometry id snapshot
    char m_pad1b8[0x1c0 - 0x1b8];
    i32 m_1c0; // +0x1c0  anim active flag  (sink+0x20)
    char m_pad1c4[0x1c8 - 0x1c4];
    i32 m_1c8; // +0x1c8  anim idle flag    (sink+0x28)
};

// The logic record HitTestCell returns / the registry's active cell entry; its
// m_10 is the bound visual. StepAnimDispatchA (0x52fb0 via the 0x2f3b thunk).
struct CTeleRecord {
    void StepAnimDispatchA(i32 a, i32 b, i32 c, i32 d); // 0x52fb0
    char m_pad00[0x10];
    CTeleVisual* m_10; // +0x10
};

// The camera/scroll sub-mgr at mgr->m_2c (ResetGoals 0xd5f00 via the 0x2e28 thunk).
struct CTeleScroller {
    void ResetGoals(i32 x, i32 y); // 0xd5f00
};
// The sprite factory reached as mgr->m_30->m_8 (CreateSprite 0x1597b0) is the shared
// <Gruntz/CTeleSpriteFactory.h> class; its result is cast to CTeleVisual*.
struct CTeleFactoryHolder {
    char m_pad00[0x8];
    CTeleSpriteFactory* m_8; // +0x08
};
// The selection holder at mgr->m_68->m_244 (its +0x8 -> the {row,col} index pair).
struct CTeleSelHolder {
    char m_pad00[0x8];
    i32* m_8; // +0x08  {a,b} index pair
};
// The icon/logic record table reached as mgr->m_68: HitTestCell resolves the
// record under (x,y); the record array sits at +0x1c (stride 4).
struct CTeleIconTable {
    CTeleRecord* HitTestCell(i32 x, i32 y, i32* o0, i32* o1, i32 f); // 0x75af0 (thunk 0x35f3)
    char m_pad00[0x244];
    CTeleSelHolder* m_244; // +0x244
    char m_pad248[0x24c - 0x248];
    i32 m_24c; // +0x24c  has-active gate (==1)
    char m_pad250[0x3fc - 0x250];
    i32 m_3fc; // +0x3fc  on-screen flag
};
// A camera/index sub-object at mgr->m_7c (its +0x28 is bumped on a teleport).
struct CTeleMgrSub {
    char m_pad00[0x28];
    i32 m_28; // +0x28
};

// The game-manager singleton (CGameRegistry* @ 0x64556c). Only the tick's offsets are
// modeled here; uses the shared ?g_gameReg@@3PAUCGameReg@@A name so the load
// reloc-masks against the already-bound symbol.
DATA(0x0024556c)
extern CGameRegistry* g_gameReg;

// The current local player index (g_644c54) the warp gates on.
DATA(0x00244c54)
extern i32 g_curPlayer;

// The spawn keys + the close-geometry key (.rdata constants).
extern char g_teleporterSpawnKey[]; // "Teleporter" @ 0x60a72c
extern char g_wormholeSpawnKey[];   // "Wormhole" @ 0x60a7ac
extern char g_teleporterCloseKey[]; // "GAME_TELEPORTERCLOSE" @ 0x60d1fc

// CTeleporter::~CTeleporter @0x010dd0 - the leaf adds no destructible members
// beyond CUserLogic, so its dtor folds the bare CUserLogic teardown: store the
// CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the embedded
// ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The destructible
// link forces the /GX EH frame. Byte-identical in shape to ~CGruntPuddle
// (0x010d10) / ~CTimeBomb (0x012a70); the empty body is enough for cl.
RVA(0x00010dd0, 0x44)
CTeleporter::~CTeleporter() {}

// The class's activation-coordinate registry singleton (@0x6446b0), built over the
// fixed [2000, 2010] range by the shared registry ctor (0x408710). CTeleporterActReg
// is the shared <Gruntz/ActReg.h> CActReg-derived alias; only Construct is used here.
DATA(0x002446b0)
extern CTeleporterActReg g_teleporterActReg; // 0x6446b0

// CTeleporter::InitActReg @0x0414a0 - construct the class's activation-coordinate
// registry singleton (g_teleporterActReg @0x6446b0) over the fixed range
// [2000, 2010] via the shared registry ctor (0x408710). Free init thunk.
RVA(0x000414a0, 0x15)
void CTeleporter::InitActReg() {
    g_teleporterActReg.Construct(2000, 2010);
}

// CTeleporter::Begin @0x0419e0 - advance the +0x1a0 anim sub-mgr to the current
// draw-delta; once it reports idle (m_28==0 && m_20!=0), run the one-shot
// finalize: snapshot the bound object's per-tile-time / running-clock / bound
// geometry into the leaf fields, apply the "GAME_TELEPORTER" lookup-geometry to
// the bound object, and swap the +0x14 sub-object's "B" bute node. The finalize
// block is the SAME archetype as CGruntPuddle::Place's tail. Returns 0.
//
// @early-stop
// inverse register-pinning wall (~88%, docs/patterns/zero-register-pinning.md):
// every offset / immediate / branch target / call arg / field store is byte-faithful,
// but our MSVC enregisters the reloaded m_38+0x1a0 pointer + the m_7c->m_bc /
// g_645588 values in callee-saved edi (extra push edi/pop edi; folds +0x1a0 into the
// +0x1c8/+0x1c0 field offsets instead of re-adding 0x1a0; reuses eax for the m_1b4
// read) where retail re-reads from memory each time. The SAME coin-flip
// CGruntPuddle::Place / CPlay::ApplyGameOptions carry; no source lever flips it.
RVA(0x000419e0, 0x81)
i32 CTeleporter::Begin() {
    ((CTeleAnimSink*)((char*)m_38 + 0x1a0))->Advance(g_6bf3bc);

    if (((CTeleAnimSink*)((char*)m_38 + 0x1a0))->m_28 == 0) {
        return 0;
    }
    if (((CTeleAnimSink*)((char*)m_38 + 0x1a0))->m_20 != 0) {
        return 0;
    }

    m_60 = ((CTeleBoundOwner*)m_10)->m_7c->m_bc;
    m_64 = 0;
    m_58 = g_645588;
    m_5c = 0;
    m_40 = m_38->m_1b4;
    m_10->ApplyLookupGeometry(g_teleporterGeoKey, 0);
    m_30 = m_14->m_1c;
    m_14->m_1c = g_buteTree.Find(g_iconBute);
    return 0;
}

// CTeleporter::Update @0x41aa0 - the per-frame teleporter tick. Advance the anim
// sub-mgr; if it just went idle, flag the bound object dirty and bail. Otherwise
// poll the on-screen render flag, and when armed (m_54 set) test the cell under
// the bound object for a grunt: on a hit, step its anim, spawn the per-mode
// ("Teleporter"/"Wormhole") sprite at the grunt's tile, close the gate geometry
// and - if that grunt is the registry's active local cell - scroll the camera to
// it. Returns 0.
//
// @early-stop
// ~99% - the whole 786-byte body is byte-identical to retail except a regalloc
// coin-flip in the final ~7-instruction tail (the m_24c active-cell gate):
// retail reuses the now-dead ebx (which held the constant 1 for the earlier
// cmp/m_68 store) to hold `col`, so `row*15 + col` accumulates into edx and the
// two ResetGoals reads land in eax/ecx/edx; MSVC here keeps `col` in ecx (the
// pointer-chain register), so the accumulator + the outB/curPlayer + g->m_5c/m_60
// registers shift by one. Same zero-register-pinning coin-flip
// (docs/patterns/zero-register-pinning.md) CGruntPuddle::Place / CTeleporter::Begin
// carry; not source-steerable. Logic + every offset/branch/call-arg byte-faithful.
RVA(0x00041aa0, 0x312)
i32 CTeleporter::Update() {
    ((CTeleAnimSink*)((char*)m_38 + 0x1a0))->Advance(g_6bf3bc);
    CTeleVisual* a = (CTeleVisual*)m_38;
    if (a->m_1c8 != 0 && a->m_1c0 == 0) {
        if (((CTeleVisual*)m_10)->m_124 == 1) {
            a->m_8 |= 0x10000;
        } else {
            a->m_40 |= 1;
        }
        return 0;
    }

    CGameRegistry* mgr;
    if (m_68 == 0) {
        CTeleVisual* o = (CTeleVisual*)m_10;
        mgr = g_gameReg;
        i32 y = o->m_60;
        i32 x = o->m_5c;
        if (x < mgr->m_144 && x >= mgr->m_13c && y < mgr->m_148 && y >= mgr->m_140) {
            ((CTeleIconTable*)mgr->m_68)->m_3fc = 1;
        }
    }
    mgr = g_gameReg;
    if (m_54 == 0) {
        return 0;
    }

    CTeleVisual* o = (CTeleVisual*)m_10;
    if (o->m_7c->m_bc != 0) {
        i64 delta = (i64)(u32)g_645588 - *(i64*)&m_58;
        if (delta >= *(i64*)&m_60) {
            m_40 = m_38->m_1b4;
            m_38->ApplyLookupGeometry(g_teleporterCloseKey, 0);
            ((CTeleVisual*)m_10)->m_7c->m_bc = 0;
            m_68 = 1;
            return 0;
        }
    }

    i32 outA;
    i32 outB;
    CTeleRecord* found =
        ((CTeleIconTable*)mgr->m_68)->HitTestCell(o->m_5c, o->m_60, &outB, &outA, 1);
    if (found == 0) {
        return 0;
    }

    if (((CTeleVisual*)m_10)->m_124 == 2) {
        found->StepAnimDispatchA(((CTeleVisual*)m_10)->m_164, ((CTeleVisual*)m_10)->m_168, 1, 1);
        ((CTeleMgrSub*)g_gameReg->m_7c)->m_28++;
        m_40 = m_38->m_1b4;
        m_38->ApplyLookupGeometry(g_teleporterCloseKey, 0);
        CTeleVisual* s = (CTeleVisual*)m_10;
        CTeleVisual* spawned = (CTeleVisual*)((CTeleFactoryHolder*)g_gameReg->m_30)
                                   ->m_8->CreateSprite(
                                       0,
                                       s->m_11c * 32 + 16,
                                       s->m_120 * 32 + 16,
                                       0,
                                       g_teleporterSpawnKey,
                                       0x40003
                                   );
        if (spawned != 0) {
            spawned->m_124 = 1;
            spawned->m_128 = ((CTeleVisual*)m_10)->m_128;
            spawned->m_164 = ((CTeleVisual*)m_10)->m_114;
            spawned->m_168 = ((CTeleVisual*)m_10)->m_118;
            spawned->m_7c->m_bc = 0;
        }
    } else {
        CTeleVisual* s = (CTeleVisual*)m_10;
        CTeleVisual* spawned = (CTeleVisual*)((CTeleFactoryHolder*)g_gameReg->m_30)
                                   ->m_8->CreateSprite(
                                       0,
                                       s->m_164 * 32 + 16,
                                       s->m_168 * 32 + 16,
                                       0,
                                       g_wormholeSpawnKey,
                                       0x40003
                                   );
        spawned->m_164 = ((CTeleVisual*)m_10)->m_5c;
        spawned->m_168 = ((CTeleVisual*)m_10)->m_60;
        spawned->m_124 = ((CTeleVisual*)m_10)->m_128;
        found->StepAnimDispatchA(((CTeleVisual*)m_10)->m_164, ((CTeleVisual*)m_10)->m_168, 0, 0);
        m_40 = m_38->m_1b4;
        m_38->ApplyLookupGeometry(g_teleporterCloseKey, 0);
    }

    m_54 = 0;
    m_68 = 1;
    mgr = g_gameReg;
    CTeleRecord* current;
    if (((CTeleIconTable*)mgr->m_68)->m_24c != 1) {
        current = 0;
    } else {
        i32* pair = ((CTeleIconTable*)mgr->m_68)->m_244->m_8;
        i32 row = pair[0];
        i32 col = pair[1];
        current = ((CTeleRecord**)((char*)(CTeleIconTable*)mgr->m_68 + 0x1c))[row * 15 + col];
    }
    if (found == current && outB == g_curPlayer) {
        CTeleVisual* g = found->m_10;
        ((CTeleScroller*)mgr->m_2c)->ResetGoals(g->m_5c, g->m_60);
    }
    return 0;
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
#include <rva.h>
SIZE_UNKNOWN(CTeleAnimSink);
SIZE_UNKNOWN(CTeleBoundOwner);
SIZE_UNKNOWN(CTeleFactoryHolder);
SIZE_UNKNOWN(CTeleIconTable);
SIZE_UNKNOWN(CTeleMgrSub);
SIZE_UNKNOWN(CTeleRecord);
SIZE_UNKNOWN(CTeleScroller);
SIZE_UNKNOWN(CTeleSelHolder);
SIZE_UNKNOWN(CTeleSpriteFactory);
SIZE_UNKNOWN(CTeleVisual);
SIZE_UNKNOWN(CTeleVisualAux);
SIZE_UNKNOWN(CTeleporter);
SIZE_UNKNOWN(CTeleporterActReg);
