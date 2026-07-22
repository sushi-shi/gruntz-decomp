// RollingBall.cpp - Gruntz CRollingBall::Update (C:\Proj\Gruntz). The per-tick
// movement/state update for the rolling-ball hazard (CRollingBall : CUserLogic,
// sizeof 0xa0). Recovered from the $SG string set (LEVEL_ROLLINGBALL_FALL/
// EXPLOSION/SINK*, LEVEL_ROLLINGBALL_NORTH/SOUTH/EAST/WEST, LEVEL_DEATHSPLASH,
// "RollingBallTimePerTile"/"Hazardz" CButeMgr key, "Particlez"/"GAME_WATER")
// which identify the ball's directional roll + sink/fall/explosion sound + the
// per-frame sub-tile position interpolation.
//
// Structure: an outer action switch on the tile's terrain id ([map]-4, range
// 0..0x70 -> jump table 0x4b0bbc) selects FALL / EXPLOSION / SINK / WATER /
// default; the SINK arm runs an inner switch on the sink-type id (0x98 base,
// 0..0x23 -> 0x4b0c68) and the move arm runs a direction switch (state +0x12c,
// 1..4) that picks NORTH/SOUTH/EAST/WEST and steps the integer tile coords
// (+0x78,+0x7c) by 0x20. The tail interpolates the double sub-tile position
// (+0x60/+0x68) toward the target at a per-tile time read from the bute file,
// then writes the snapped grid coords back into the logic object (+0x10).
//
// It is a /GX EH-framed routine: two CString diagnostic/scratch temps at
// [esp+0x10]/[esp+0x14] (the per-direction sound-name strings) give it the
// exception frame, so it lives in an `eh` unit.
//
// CARCASS doctrine: CRollingBall's own fields and the level/map/registry
// sub-objects are unmatched engine classes accessed by raw this+offset; every
// callee is an external reloc-masked __thiscall/__cdecl thunk; the LEVEL_*/GAME_*
// strings are $SG literals reloc-masked against the matched string symbols. Only
// the offsets / code bytes are load-bearing (campaign doctrine).
//
// @early-stop
// /GX branchy nested-jump-table state-machine wall. The whole body is
// reconstructed here and matches retail's logic; the byte-match of the descending
// /GX exception-state thread around the two CString temps + the three nested
// jump-table base relocs across this 2682-byte size is the documented
// megafunction wall (cf. the sibling /GX megafunctions TerrainTileLoader,
// MainMenuBuilder ~78%): MSVC5's jump-table base reloc typing + the EH-state
// numbering are not steerable from C source. Deferred to the final sweep
// (docs/patterns/jumptable-data-overlap.md; big-seh-fuzzy-desync.md;
// eh-state-numbering-base.md; o2-optimizer-bailout-framed.md).

#include <Gruntz/ActNameRegistry.h> // the shared activation-name registry archetype
#include <Gruntz/GameRegMfcPtr.h>   // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <Io/FileMem.h>    // the serialize stream (CSerialArchive == the real CFileMemBase)
#include <Gruntz/ActReg.h> // the shared CActReg coordinate-registry archetype
#include <Gruntz/AniAdvanceCursor.h> // CAniAdvanceCursor (m_38+0x1a0 sub-update Advance)
#include <Gruntz/RollingBall.h>      // CRollingBall : CUserLogic (+ the /GX CString temps)
#include <Gruntz/GameRegistry.h>     // the canonical *0x24556c singleton (g_gameReg typed)

#include <rva.h>
#include <Gruntz/GameLevel.h> // CTileImageSet (the tile-descriptor the resolver dispatches on)
#include <string.h>           // inline strcmp for the ctor's direction-name match
#include <Globals.h>
#include <Wap32/ZVec.h>
#include <Rez/FrameClock.h>    // g_frameTime/g_frameDelta/g_engineFrameDelta (frame-clock band)
#include <Gruntz/TriggerMgr.h> // CTriggerMgr - m_cmdGrid (m_rollingballWanted)

// g_rollingBallActReg (0x002461b0): CActReg - no provable static init (the type has no
// default ctor / is runtime-Init'd), so the datum is named by symbol.
DATA_SYMBOL(0x002461b0, 0x0, ?g_rollingBallActReg@@3UCActReg@@A)

VTBL(CRollingBall, 0x001e86fc);

static const double kMsPerSecond = 1000.0; // ms -> tiles/second divisor

void RbCacheFirst(
    void* anim,
    const char* name
); // 0x150540 = CGameObject::ApplyName (thiscall; free-fn shape kept with the parked carcass)
void RbApplyLookup(
    void* anim,
    const char* name,
    i32 z
);                                          // 0x1505b0 = CGameObject::ApplyLookupGeometry (ditto)
void RbStrAssign(void* str, const char* s); // 0x1b9e74 CString::operator=(LPCSTR)
i32 RbGetDwordDef(const char* sec, const char* key, i32 def); // 0x1721e0 CButeMgr::GetDwordDef
double RbCeil(double x);                                      // 0x120480 ceil
double RbFloor(double x);                                     // 0x120580 floor

i32 RbProbeRect(
    void* obj,
    i32 cx,
    i32 cy,
    RECT* rect,
    i32* outA,
    i32* outB,
    i32 z
);                                                           // 0x32ce (the object's m_area box)
void RbMarkRect(void* obj, i32 a, i32 b, i32 mode, i32 neg); // 0x2e96
void RbClearCell(void* obj, i32 a, i32 b, i32 z);            // 0x26df

static i32 VtblResolve(void* ent) {
    return static_cast<CTileImageSet*>(ent)->GetCollisionAt(0, 0);
}

// ===========================================================================
// CRollingBall::~CRollingBall  (0x012f80)
// ===========================================================================
// The leaf adds no destructible members, so its dtor folds the bare CUserLogic
// teardown: store the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link
// (the embedded ~EngStr call), store the CUserBase vptr (0x5e70b4). The
// destructible link forces the /GX EH frame. The empty body is enough.
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CRollingBall() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
RVA_COMPGEN(0x00012f80, 0x44, ??1CRollingBall@@UAE@XZ)

// CRollingBall::CRollingBall @0xaf820 - fold the shared CUserLogic(obj) init, bind
// the cycle geometry + "A" bute node, snap the bound object to the tile grid + seed
// the +0x74 layer key, match the ball's direction name
// (LEVEL_ROLLINGBALL_{NORTH,EAST,SOUTH,WEST}) into the travel vector + direction id,
// then read the per-tile time (RollingBallTimePerTile, +1000 in the windowed mode)
// into the per-frame speed and seed the timer block.
//
// @early-stop
// inline-strcmp + register-pinning + eh wall (docs/patterns/strcmp-eq-bool-local-setcc.md,
// zero-register-pinning.md, eh-ctor-vptr-restamp-position.md): body byte-faithful
// (the four unrolled inline-strcmp loops, the CString temp EH, the snap + layer key,
// the time-bonus gate + 32.0/time divide, the timer block). Residual is the strcmp
// result-reg alloc, the shared dy=0 store fold, and the /GX leaf-vptr re-stamp
// position. Not source-steerable (global regalloc/EH numbering).
RVA(0x000af820, 0x40d)
CRollingBall::CRollingBall(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_explodeStartLo = 0;
    m_explodeWindowLo = 0;
    m_explodeStartHi = 0;
    m_explodeWindowHi = 0;
    m_value = m_38->m_1a0.m_14;
    m_38->ApplyLookupGeometry("GAME_CYCLE100", 0);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_38->m_flags |= 0x2000002;

    CWwdGameObjectA* o = m_object;
    i32 snapX = (o->m_screenX & ~0x1f) + 0x10;
    i32 snapY = 0x10 + (o->m_screenY & ~0x1f);
    o->m_screenX = snapX;
    m_subX = static_cast<double>(snapX);
    o->m_screenY = snapY;
    m_subY = static_cast<double>(snapY);
    if (o->m_sortKey != 0x186a0 + snapY) {
        o->m_sortKey = snapY + 0x186a0;
        o->m_flags |= 0x20000;
    }

    CWwdGameObjectA* obj38 = m_38;
    if (obj38->m_194 != 0) {
        CString name;
        name = obj38->m_194 + 0x24;
        const char* s;
        s = static_cast<LPCTSTR>(name);
        if (strcmp(s, "LEVEL_ROLLINGBALL_NORTH") == 0) {
            o->m_12c = 1;
            m_stepDirX = 0;
            m_stepDirY = -1;
        } else if (strcmp(s, "LEVEL_ROLLINGBALL_EAST") == 0) {
            o->m_12c = 2;
            m_stepDirY = 0;
            m_stepDirX = 1;
        } else if (strcmp(s, "LEVEL_ROLLINGBALL_SOUTH") == 0) {
            o->m_12c = 3;
            m_stepDirY = 1;
            m_stepDirX = 0;
        } else if (strcmp(s, "LEVEL_ROLLINGBALL_WEST") == 0) {
            o->m_12c = 4;
            m_stepDirY = 0;
            m_stepDirX = -1;
        }
    }

    i32 time = o->m_7c->m_bc;
    if (time == 0) {
        time = g_buteMgr.GetDwordDef("Hazardz", "RollingBallTimePerTile", 1000);
    }
    CGruntzMgr* reg = g_gameReg;
    if (0 != reg->m_isEasyMode && reg->m_134 == 1 && o->m_124 != 1) {
        time += 1000;
    }
    m_explodeWindowLo = o->m_118;
    m_explodeWindowHi = 0;
    m_explodeStartLo = g_frameTime;
    m_explodeStartHi = 0;
    m_targetY = snapY;
    m_targetX = snapY;
    m_explodeLatch = 0;
    m_fallLatch = 0;
    m_moveSpeed = g_slimeSpeedNum / static_cast<double>(static_cast<i64>(static_cast<u32>(time)));
    o->m_area.left = 0;
    o->m_area.right = 0;
    o->m_area.top = 0;
    o->m_area.bottom = 0;
    m_moveDeltaHi = 0;
    m_moveDeltaLo = 0;
}

RVA(0x000afd60, 0x15)
void CRollingBall::InitActReg() {
    g_rollingBallActReg.Construct(2000, 2010);
}

RVA(0x000afde0, 0x102)
void CRollingBall::FireActivation(i32 id) {
    CRollingBallActEntry* e =
        reinterpret_cast<CRollingBallActEntry*>(g_rollingBallActReg.ResolveEntry(id));
    if (e->m_fn != 0) {
        (this
             ->*(reinterpret_cast<CRollingBallActEntry*>(g_rollingBallActReg.ResolveEntry(id)))
             ->m_fn)();
    }
}

// CRollingBall::RegisterActs @0x0aff40 - bind the per-frame handler (Update
// @0x0b0140) to the activation key "A" via the shared name registry. The SAME
// archetype as CBehindCandyAni::RegisterActs.
//
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): logic
// byte-faithful (every call/immediate/branch/offset + the `mov [entry],offset
// Update` handler store match retail); residual is the slot-vs-id callee-saved
// register choice cascading into the free-loop count materialization. Deferred.
RVA(0x000aff40, 0x18d)
void CRollingBall::RegisterActs() {
    i32 id = reinterpret_cast<i32>(g_buteTree.Find("A"));
    if (id == 0) {
        id = g_typeCounter;
        g_buteTree.Insert("A", reinterpret_cast<void*>(id));
        char* slot = ActNameLookup(id);
        i32 n = g_typeColl.m_grown;
        void** list = reinterpret_cast<void**>(g_typeColl.m_alloc);
        while (n-- != 0) {
            if (list != 0) {
                (reinterpret_cast<CString*>(list))->CString::~CString();
            }
            list++;
        }
        (reinterpret_cast<CString*>(slot))->operator=("A");
        g_typeCounter++;
    }
    (reinterpret_cast<CRollingBallActEntry*>(g_rollingBallActReg.ResolveEntry(id)))->m_fn =
        static_cast<i32 (CUserLogic::*)()>(&CRollingBall::Update);
}

// CRollingBall::Update - the per-tick rolling-ball state machine (__thiscall).
// @early-stop
// /GX branchy nested-jump-table megafunction wall (~38%): complete, correct
// reconstruction (prologue, explosion/fall latches, action/direction/sink switches,
// x87 interpolation tail). Two binary-proven structural fixes landed the prologue
// exactly (36.4%->38.0%): the m_38+0x1a0 sub-update is the real CAniAdvanceCursor::
// Advance(g_engineFrameDelta) __thiscall (was a fake free-fn RbSubUpdate + a dead
// g_engineFrameDelta load), and g_gameReg is re-loaded per use (retail does not cache the game
// registry in a callee-saved reg - ~15 fresh ds:0x64556c loads). Residual is the
// documented wall: MSVC5's constant-materialization (test eax,eax vs cmp [mem],ebx),
// the +8B frame spill count, the EH-state numbering + the three nested jump-table
// reloc-typings across 2682 B. See file header; final-sweep.
RVA(0x000b0140, 0xa7a)
i32 CRollingBall::Update() {
    m_38->m_1a0.Advance(g_engineFrameDelta);

    CWwdGameObjectA* anim = m_38;
    if (anim->m_1a0.m_28 != 0 && anim->m_1a0.m_20 == 0) {
        anim->m_flags |= 0x10000;
        return 0;
    }

    // The explosion latch (+0x80): the explosion sound + cell-clear fire once.
    if (m_explodeLatch == 0) {
        CWwdGameObjectA* logic = m_object;
        i32 lo = g_frameTime - m_explodeStartLo;
        i32 hi = 0 - m_explodeStartHi;
        i32 lim = m_explodeWindowHi;
        if (hi < lim || (hi == lim && static_cast<u32>(lo) < static_cast<u32>(m_explodeWindowLo))) {
            RbCacheFirst(m_38, "LEVEL_ROLLINGBALL_EXPLOSION");
            m_value = m_38->m_1a0.m_14;
            RbApplyLookup(m_38, "LEVEL_ROLLINGBALLEXPLOSION", 0);
            CTileGrid* map = g_gameReg->m_tileGrid;
            i32 cx = logic->m_screenX >> 5;
            i32 cy = logic->m_screenY >> 5;
            if (static_cast<u32>(cx) < map->m_width && static_cast<u32>(cy) < map->m_height) {
                i32** row = map->m_rowInts;
                i32 ix = cx * 7;
                row[cy][ix] &= 0xefffffff;
            }
        }
        m_explodeLatch = 1;
    }

    // The fall latch (+0x84): grid-side death flag + rect re-mark.
    if (m_fallLatch == 0) {
        CWwdGameObjectA* logic = m_object;
        i32 cx = logic->m_screenX;
        i32 cy = logic->m_screenY;
        if (cx < g_gameReg->m_viewOriginR && cx >= g_gameReg->m_viewOriginL
            && cy < g_gameReg->m_viewOriginB && cy >= g_gameReg->m_viewOriginT) {
            g_gameReg->m_cmdGrid->m_rollingballWanted = 1;
        }
        CWwdGameObjectA* logic2 = m_object;
        i32 outA, outB;
        if (RbProbeRect(
                g_gameReg->m_cmdGrid,
                logic2->m_screenX,
                logic2->m_screenY,
                &logic2->m_area,
                &outB,
                &outA,
                0
            )) {
            RbMarkRect(g_gameReg->m_cmdGrid, outA, outB, 2, -1);
        }
    }

    // ----- the sub-tile-snapped move + action switch -----
    CWwdGameObjectA* logic = m_object;
    if (logic->m_screenX == m_targetX && m_targetY == logic->m_screenY) {
        // arrived at the target cell: clear the cell, read its terrain id and
        // dispatch on the rolling-ball action.
        RbClearCell(g_gameReg->m_cmdGrid, m_targetY, m_targetX, 0);

        CTileGrid* map = g_gameReg->m_tileGrid;
        i32 cx = m_targetX >> 5;
        i32 cy = m_targetY >> 5;
        i32 terrain;
        if (static_cast<u32>(cx) < map->m_width && static_cast<u32>(cy) < map->m_height) {
            i32** row = map->m_rowInts;
            i32 ix = cx * 7;
            terrain = row[cy][ix];
        } else {
            terrain = 1;
        }

        if ((terrain & 0x939) != 0 || (terrain & 2) != 0) {
            CString fall;      // [esp+0x14]
            CString explosion; // [esp+0x10]
            // Resolve the action id from the level's main-plane tile graph - the
            // SAME walk GameLevel's collision macro does typed. DISASM NOTE: the old
            // raw-offset spelling read the attr grid off LVL+0x20; retail reads it
            // off the PLANE (+0x20 m_tileGrid, same edi as the +0x24 column read) -
            // a mis-based recon offset, fixed by the typed spelling.
            CGameLevel* lvl = g_gameReg->m_world->m_level;
            i32 ax = m_targetY >> 5;
            i32 ay = m_targetX >> 5;
            if (ax < 0) {
                ax = 0;
            } else {
                i32 w = lvl->m_mainPlane->m_gridW;
                if (ax >= w) {
                    ax = w - 1;
                }
            }
            if (ay < 0) {
                ay = 0;
            } else {
                i32 h = lvl->m_mainPlane->m_gridH;
                if (ay >= h) {
                    ay = h - 1;
                }
            }
            CLevelPlane* pl = lvl->m_mainPlane;
            i32 idx = pl->m_colOffsets[ay] + ax;
            i32 raw = pl->m_tileGrid[idx];
            i32 obj = 0;
            if (raw != static_cast<i32>(0xeeeeeeee) && raw != -1) {
                obj = VtblResolve(lvl->m_imageSets[raw & 0xffff]);
            }

            // The action switch (obj's terrain id, near-dense range): FALL,
            // EXPLOSION, SINK and their neighbours collapse to a few sound pairs.
            // Spelled as a natural switch so cl emits its own jump table.
            switch (obj) {
                case 0xa: // FALL group
                case 0x24:
                    RbStrAssign(&fall, "LEVEL_ROLLINGBALL_FALL");
                    RbStrAssign(&explosion, "LEVEL_ROLLINGBALLFALL");
                    break;
                case 0x10: // EXPLOSION
                    RbStrAssign(&fall, "LEVEL_ROLLINGBALL_EXPLOSION");
                    RbStrAssign(&explosion, "LEVEL_ROLLINGBALLEXPLOSION");
                    obj = 1;
                    break;
                default: // SINK and the rest collapse onto the sink temps
                    RbStrAssign(&fall, "LEVEL_ROLLINGBALL_SINK");
                    RbStrAssign(&explosion, "LEVEL_ROLLINGBALLSINKDEATH");
                    break;
            }
            RbCacheFirst(m_38, explosion);
            m_value = m_38->m_1a0.m_14;
            RbApplyLookup(m_38, fall, 0);
            if (obj == 4) {
                i32 t = RbGetDwordDef("Hazardz", "RollingBallTimePerTile", 0x3e8);
                m_moveSpeed = kMsPerSecond / static_cast<double>(t);
            }
        }
    }

    // ----- the direction sub-switch (state +0x12c -> NORTH/SOUTH/EAST/WEST) -----
    // m_subX/m_subY are doubles; the direction arms zero/seed them (and m_moveDelta)
    // as int pairs in this exact interleaved store order, so their halves are
    // addressed as ints via ((i32*)&member)[0/1] (matching retail's dword stores).
    (reinterpret_cast<i32*>(&m_subX))[0] = 0;
    (reinterpret_cast<i32*>(&m_subY))[0] = 0;
    (reinterpret_cast<i32*>(&m_subX))[1] = 0;
    (reinterpret_cast<i32*>(&m_subY))[1] = 0;
    CWwdGameObjectA* lg = m_object;
    switch (lg->m_12c) {
        case 1:
            m_subY = -*reinterpret_cast<double*>(&m_moveDeltaLo);
            m_targetX -= 0x20;
            m_stepDirX = -1;
            m_stepDirY = -1;
            break;
        case 2:
            (reinterpret_cast<i32*>(&m_subX))[0] = m_moveDeltaLo;
            (reinterpret_cast<i32*>(&m_subX))[1] = m_moveDeltaHi;
            m_targetX += 0x20;
            m_stepDirX = 1;
            m_stepDirY = 0;
            break;
        case 4:
            (reinterpret_cast<i32*>(&m_subY))[0] = m_moveDeltaLo;
            (reinterpret_cast<i32*>(&m_subY))[1] = m_moveDeltaHi;
            m_targetX += 0x20;
            m_stepDirX = 0;
            m_stepDirY = 1;
            break;
        case 3:
            m_subX = -*reinterpret_cast<double*>(&m_moveDeltaLo);
            m_targetX -= 0x20;
            m_stepDirX = -1;
            m_stepDirY = 0;
            break;
    }

    // ----- the x87 sub-tile interpolation tail -----
    CWwdGameObjectA* lg2 = m_object;
    m_subX = static_cast<double>(lg2->m_screenX) + m_subX;
    m_subY = static_cast<double>(lg2->m_screenY) + m_subY;
    m_moveDeltaLo = 0;
    m_moveDeltaHi = 0;

    double dt = static_cast<double>(g_frameDelta) * m_moveSpeed;
    i32 nx = m_targetX >> 5;
    if (m_stepDirX > 0) {
        double v = dt + m_subX;
        m_subX = v;
        nx = __ftol(RbCeil(v));
        if (nx >= (m_targetX >> 5)) {
            nx = m_targetX >> 5;
        }
    } else if (m_stepDirX < 0) {
        double v = m_subX - dt;
        m_subX = v;
        nx = __ftol(RbFloor(v));
        if (nx < (m_targetX >> 5)) {
            nx = m_targetX >> 5;
        }
    } else {
        nx = __ftol(RbFloor(m_subX));
    }

    i32 ny = m_targetY >> 5;
    if (m_stepDirY > 0) {
        double v = dt + m_subY;
        m_subY = v;
        ny = __ftol(RbCeil(v));
        if (ny >= (m_targetY >> 5)) {
            ny = m_targetY >> 5;
        }
    } else if (m_stepDirY < 0) {
        double v = m_subY - dt;
        m_subY = v;
        ny = __ftol(RbFloor(v));
        if (ny < (m_targetY >> 5)) {
            ny = m_targetY >> 5;
        }
    } else {
        ny = __ftol(RbFloor(m_subY));
    }

    CWwdGameObjectA* out = m_object;
    out->m_screenX = nx;
    out->m_screenY = ny;
    CWwdGameObjectA* out2 = m_object;
    i32 next = out2->m_screenY + 0x186a0;
    if (out2->m_sortKey != next) {
        out2->m_sortKey = next;
        out2->m_flags |= 0x20000;
    }
    return 0;
}

RVA(0x000b0fe0, 0x1ab)
i32 CRollingBall::SerializeMove(CGruntArchive* ar, i32 tag, i32 c, i32 d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    if (!Chain(ar, tag, c, reinterpret_cast<CGameObject*>(d))) {
        return 0;
    }

    // The explosion-timing pair (+0x88/+0x90), streamed through a walking pointer
    // that advances by 8 (retail hoists `lea ebp,[edi+0x88]` before the branch and
    // does `push ebp; call; add ebp,8; push ebp; call`). A two-case switch (not
    // if/else) floats the Write arm past the Read arm, matching retail's branch
    // polarity (docs/patterns/inline-switch-serialize-record-unroll.md +
    // pointer-walk-increment-in-for-update.md).
    i32* p = &m_explodeStartLo; // +0x88; the cursor walks the two i64 clock pairs (add 8)
    switch (tag) {
        case 4:
            ar->Write(p, 8);
            p += 2;
            ar->Write(p, 8);
            break;
        case 7:
            ar->Read(p, 8);
            p += 2;
            ar->Read(p, 8);
            break;
    }

    // The move/state field list (+0x58..+0x98).
    switch (tag) {
        case 4:
            ar->Write(&m_moveSpeed, 8);
            ar->Write(&m_subX, 8);
            ar->Write(&m_subY, 8);
            ar->Write(&m_stepDirX, 4);
            ar->Write(&m_stepDirY, 4);
            ar->Write(&m_targetX, 8);
            ar->Write(&m_explodeLatch, 4);
            ar->Write(&m_fallLatch, 4);
            ar->Write(&m_moveDeltaLo, 8);
            break;
        case 7:
            ar->Read(&m_moveSpeed, 8);
            ar->Read(&m_subX, 8);
            ar->Read(&m_subY, 8);
            ar->Read(&m_stepDirX, 4);
            ar->Read(&m_stepDirY, 4);
            ar->Read(&m_targetX, 8);
            ar->Read(&m_explodeLatch, 4);
            ar->Read(&m_fallLatch, 4);
            ar->Read(&m_moveDeltaLo, 8);
            break;
    }
    return 1;
}
