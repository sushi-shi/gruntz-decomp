// Teleporter.cpp - the teleporter tile-logic game object (C:\Proj\Gruntz).
//
// Two trace-discovered CTeleporter methods, defined in ascending retail-RVA order:
//   ~CTeleporter @0x010dd0 - the /GX leaf dtor (folds the CUserLogic teardown).
//   Begin        @0x0419e0 - the anim-driven one-shot geometry/bute bring-up.
//
// CTeleporter : CUserLogic (RTTI .?AVCTeleporter@@). Only offsets / code bytes
// are load-bearing; names are placeholders for the recovered engine identities.
#include <Gruntz/Teleporter.h>
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/Play.h>
#include <Gruntz/ActReg.h> // shared activation-registrar archetype (CTeleporterActReg)
#include <Gruntz/GameRegistry.h>
#include <Gruntz/SpriteFactory.h> // the ONE CSpriteFactory (CreateSprite @0x1597b0)
#include <Gruntz/SerialObjRef.h>  // CSerialObjRef (+0x34 chain) + CSerialArchive (Read/Write)
#include <Globals.h>

// The bound / spawned visual object the tick reads + re-seeds IS the engine
// CGameObject (the inherited CUserLogic m_10/m_38 both point at it; ApplyLookup-
// Geometry @0x1505b0, screen x/y @+0x5c/+0x60, tile col/row @+0x11c/+0x120, the
// +0x7c aux carrying the per-tile clock at +0xbc, the +0x1a0 anim sub-mgr's
// idle/active flags at +0x1c0/+0x1c8). All those offsets live in the shared
// CGameObject / CGameObjAux (<Gruntz/UserLogic.h>), so this TU uses them directly
// - no per-TU visual/bound-owner "view" and no cast off m_10/m_38.

// ===========================================================================
// CTeleporter::Update (0x41aa0) collaborators (modeled NO-body / by offset so
// the calls + field loads reloc-mask against the engine symbols).
//
// The CTele* structs below (record / scroller / icon-table / sel-holder / mgr-sub)
// are the SANCTIONED per-TU views of the 0x64556c multi-view game-manager singleton's
// void* sub-object slots (m_2c/m_68/m_7c) - see <Gruntz/GameRegistry.h>: those slots
// hold a *different concrete type per TU*, so each TU casts them to its own local view
// at the deref site (a real struct-pointer-to-sub-object cast, not an artifact). Kept
// by that design; the singleton itself is owned by CGruntzMgr (classifier scope).
// ===========================================================================

// The logic record HitTestCell returns / the registry's active cell entry; its
// m_10 is the bound CGameObject. StepAnimDispatchA (0x52fb0 via the 0x2f3b thunk).
// The camera/scroll sub-mgr at mgr->m_curState (ResetGoals 0xd5f00 via the 0x2e28 thunk).
// The sprite factory reached as mgr->m_world->m_8 (CreateSprite 0x1597b0) is the
// canonical CSpriteFactory (<Gruntz/SpriteFactory.h>). g_gameReg->m_world is already the
// real CSpriteFactoryHolder (<Gruntz/GameRegistry.h>) whose m_8 IS that CSpriteFactory* -
// no local holder/factory view, and the result is the created CGameObject.

// The selection holder at mgr->m_68->m_244 (its +0x8 -> the {row,col} index pair).
struct CTeleSelHolder {
    char m_pad00[0x8];
    i32* m_8; // +0x08  {a,b} index pair
};
// The icon/logic record table reached as mgr->m_68: HitTestCell resolves the
// record under (x,y); the record array sits at +0x1c (stride 4).
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

// CTeleporter::GetTypeTag (0x00010d80) is now an inline member in the class header.

// CTeleporter::~CTeleporter @0x010dd0 - the leaf adds no destructible members
// beyond CUserLogic, so its dtor folds the bare CUserLogic teardown: store the
// CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the embedded
// ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The destructible
// link forces the /GX EH frame. Byte-identical in shape to ~CGruntPuddle
// (0x010d10) / ~CTimeBomb (0x012a70); the empty body is enough for cl.
RVA(0x00010dd0, 0x44)
CTeleporter::~CTeleporter() {}

// CTeleporter::Serialize @0x041350 - slot-1 (SerializeMove) override. Chain the
// shared CUserLogic serialize helper + the +0x34 sub-object's chain (either bailing
// out on failure), then round-trip the leaf state: the two i64 arm-clock/interval
// snapshots (+0x58/+0x60, walked through one hoisted base ptr; read-inline/write-else
// block layout) then the two i32 fields m_armed/+0x54 + m_tickHandled/+0x68 (a
// switch), and on the post-load tag 8 re-apply the config through LoadColors (its
// body is COMDAT-ICF-folded with CWormhole::LoadColors at 0x411f0). Same archetype
// as CGruntPuddle::Serialize. Byte-exact.
RVA(0x00041350, 0xee)
i32 CTeleporter::Serialize(CSerialArchive* ar, i32 tag, i32 c, i32 d) {
    if (!SerializeChain((i32)ar, tag, c, d)) {
        return 0;
    }
    if (!((CSerialObjRef*)((char*)this + 0x34))->Chain(ar, tag, c, (CSerialObj*)d)) {
        return 0;
    }
    // The two i64 snapshots (+0x58 arm-clock, +0x60 interval) round-trip through one
    // hoisted base pointer that walks +8 (retail: lea ebx,[this+0x58] then add ebx,8).
    i32* p = &m_armClockLo;
    if (tag != 4) {
        if (tag == 7) {
            ar->Read(p, 8);
            ar->Read(p + 2, 8);
        }
    } else {
        ar->Write(p, 8);
        ar->Write(p + 2, 8);
    }
    switch (tag) {
        case 4:
            ar->Write(&m_armed, 4);
            ar->Write(&m_tickHandled, 4);
            break;
        case 7:
            ar->Read(&m_armed, 4);
            ar->Read(&m_tickHandled, 4);
            break;
        case 8:
            LoadColors();
            break;
    }
    return 1;
}

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
    ((CZDArrayDerived*)&g_teleporterActReg)->Construct(2000, 2010);
}

// CTeleporter::FireActivation @0x041520 - look the activation coordinate up in
// g_teleporterActReg; if the resolved entry carries a registered handler, resolve
// it again and dispatch it __thiscall on `this`. Same archetype as
// CParticlez::FireActivation (double ResolveEntry + dispatch).
RVA(0x00041520, 0x102)
void CTeleporter::FireActivation(i32 coord) {
    CTeleporterActEntry* e = (CTeleporterActEntry*)g_teleporterActReg.ResolveEntry(coord);
    if (e->m_fn != 0) {
        CTeleporterActEntry* e2 = (CTeleporterActEntry*)g_teleporterActReg.ResolveEntry(coord);
        (this->*(e2->m_fn))();
    }
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
    ((CAniAdvanceCursor*)((char*)m_38 + 0x1a0))->Advance_15c360(g_6bf3bc);

    if (((CTeleAnimSink*)((char*)m_38 + 0x1a0))->m_28 == 0) {
        return 0;
    }
    if (((CTeleAnimSink*)((char*)m_38 + 0x1a0))->m_20 != 0) {
        return 0;
    }

    m_intervalLo = m_object->m_7c->m_bc;
    m_intervalHi = 0;
    m_armClockLo = g_645588;
    m_armClockHi = 0;
    m_savedGeoId = m_38->m_geoId;
    m_object->ApplyLookupGeometry(g_teleporterGeoKey, 0);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find(g_iconBute);
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
    ((CAniAdvanceCursor*)((char*)m_38 + 0x1a0))->Advance_15c360(g_6bf3bc);
    CGameObject* a = m_38;
    if (a->m_1c8 != 0 && a->m_1c0 == 0) {
        if (m_object->m_124 == 1) {
            a->m_flags |= 0x10000;
        } else {
            a->m_stateFlags |= 1;
        }
        return 0;
    }

    CGameRegistry* mgr;
    if (m_tickHandled == 0) {
        CGameObject* o = m_object;
        mgr = g_gameReg;
        i32 y = o->m_screenY;
        i32 x = o->m_screenX;
        if (x < mgr->m_viewOriginR && x >= mgr->m_viewOriginL && y < mgr->m_viewOriginB
            && y >= mgr->m_viewOriginT) {
            ((CTriggerMgr*)mgr->m_cmdGrid)->m_3fc = 1;
        }
    }
    mgr = g_gameReg;
    if (m_armed == 0) {
        return 0;
    }

    CGameObject* o = m_object;
    if (o->m_7c->m_bc != 0) {
        i64 delta = (i64)(u32)g_645588 - *(i64*)&m_armClockLo;
        if (delta >= *(i64*)&m_intervalLo) {
            m_savedGeoId = m_38->m_geoId;
            m_38->ApplyLookupGeometry(g_teleporterCloseKey, 0);
            m_object->m_7c->m_bc = 0;
            m_tickHandled = 1;
            return 0;
        }
    }

    i32 outA;
    i32 outB;
    CGrunt* found = (CGrunt*)((CTriggerMgr*)mgr->m_cmdGrid)
                        ->HitTestCell(o->m_screenX, o->m_screenY, &outB, &outA, 1);
    if (found == 0) {
        return 0;
    }

    if (m_object->m_124 == 2) {
        found->StepAnimDispatchA(m_object->m_164, m_object->m_168, 1, 1);
        ((CTeleMgrSub*)g_gameReg->m_scoreHud)->m_28++;
        m_savedGeoId = m_38->m_geoId;
        m_38->ApplyLookupGeometry(g_teleporterCloseKey, 0);
        CGameObject* s = m_object;
        CGameObject* spawned = g_gameReg->m_world->m_8->CreateSprite(
            0,
            s->m_11c * 32 + 16,
            s->m_120 * 32 + 16,
            0,
            g_teleporterSpawnKey,
            0x40003
        );
        if (spawned != 0) {
            spawned->m_124 = 1;
            spawned->m_placeMode = m_object->m_placeMode;
            spawned->m_164 = m_object->m_114;
            spawned->m_168 = m_object->m_118;
            spawned->m_7c->m_bc = 0;
        }
    } else {
        CGameObject* s = m_object;
        CGameObject* spawned = g_gameReg->m_world->m_8->CreateSprite(
            0,
            s->m_164 * 32 + 16,
            s->m_168 * 32 + 16,
            0,
            g_wormholeSpawnKey,
            0x40003
        );
        spawned->m_164 = m_object->m_screenX;
        spawned->m_168 = m_object->m_screenY;
        spawned->m_124 = m_object->m_placeMode;
        found->StepAnimDispatchA(m_object->m_164, m_object->m_168, 0, 0);
        m_savedGeoId = m_38->m_geoId;
        m_38->ApplyLookupGeometry(g_teleporterCloseKey, 0);
    }

    m_armed = 0;
    m_tickHandled = 1;
    mgr = g_gameReg;
    CGrunt* current;
    if (((CTriggerMgr*)mgr->m_cmdGrid)->m_recList.m_count != 1) {
        current = 0;
    } else {
        i32* pair = ((CTeleSelHolder*)((CTriggerMgr*)mgr->m_cmdGrid)->m_recList.m_head)->m_8;
        i32 row = pair[0];
        i32 col = pair[1];
        current = ((CGrunt**)((char*)(CTriggerMgr*)mgr->m_cmdGrid + 0x1c))[row * 15 + col];
    }
    if (found == current && outB == g_curPlayer) {
        CGameObject* g = found->m_object;
        ((CPlay*)mgr->m_curState)->ResetGoals(g->m_screenX, g->m_screenY);
    }
    return 0;
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
#include <rva.h>
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>
SIZE_UNKNOWN(CTeleAnimSink);
SIZE_UNKNOWN(CTeleIconTable);
SIZE_UNKNOWN(CTeleMgrSub);
SIZE_UNKNOWN(CTeleScroller);
SIZE_UNKNOWN(CTeleSelHolder);
VTBL(CTeleporter, 0x001e80cc); // vtable_names -> code (RTTI game class)
SIZE_UNKNOWN(CTeleporterActReg);
