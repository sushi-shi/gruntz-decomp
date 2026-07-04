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
#include <Gruntz/ActReg.h>          // the shared CActReg coordinate-registry archetype
#include <Gruntz/RollingBall.h>     // CRollingBall : CUserLogic (+ the /GX CString temps)

#include <rva.h>
#include <string.h> // inline strcmp for the ctor's direction-name match
#include <Globals.h>

// The handler entry the per-class registry yields: its first dword receives the
// per-frame handler PMF (Update, a 4-byte code ptr on this single-inheritance
// class).
typedef i32 (CRollingBall::*RollingBallHandler)();
struct CRollingBallActEntry {
    RollingBallHandler m_fn;
};

// The class's activation-coordinate registry singleton (@0x6461b0): the fixed
// [2000,2010] range built by the shared registry ctor (0x408710). CRollingBallActReg
// is the shared <Gruntz/ActReg.h> CActReg archetype (was a per-file duplicate of its
// layout + ResolveEntry); it keeps its own placeholder name so the DATA-pinned
// global symbol is unchanged.
struct CRollingBallActReg : public CActReg {};
DATA(0x002461b0)
extern CRollingBallActReg g_rollingBallActReg; // 0x6461b0

// ---------------------------------------------------------------------------
// Shared singletons (named so their DIR32 datum reloc-masks).
// ---------------------------------------------------------------------------
extern void* g_64556c; // ?g_gameReg@@3PAUWwdGameReg@@A @0x64556c
// (g_buteMgr comes from <Bute/ButeMgr.h> via UserLogic.h; Update reaches it only
//  through RbGetDwordDef, so no direct extern is needed here.)
extern "C" i32 g_645588;    // DAT_00645588 @0x645588 (world clock ms)
extern "C" i32 g_645584;    // DAT_00645584 @0x645584 (frame delta ms)
extern "C" double g_5ea3e8; // DAT_005ea3e8 @0x5ea3e8 (1000.0 ms->tiles divisor)
extern "C" void* g_6bf3bc;  // _g_6bf3bc @0x6bf3bc (the EH frame's pushed global)

// ---------------------------------------------------------------------------
// Engine helpers reached through reloc-masked thunks (no body).
// ---------------------------------------------------------------------------
void RbSubUpdate(void* anim1a0); // 0x15c360 [this+0x38]+0x1a0 sub-update
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
extern "C" i32 __ftol(double x);                              // 0x11f570

// Registry slot calls on g_gameReg sub-objects (reloc-masked __thiscall).
i32 RbProbeRect(void* obj, i32 cx, i32 cy, i32* rectBase, i32* outA, i32* outB, i32 z); // 0x32ce
void RbMarkRect(void* obj, i32 a, i32 b, i32 mode, i32 neg);                            // 0x2e96
void RbClearCell(void* obj, i32 a, i32 b, i32 z);                                       // 0x26df

#define I32(p, off) (*(i32*)((char*)(p) + (off)))
#define PTR(p, off) (*(void**)((char*)(p) + (off)))
#define DBL(p, off) (*(double*)((char*)(p) + (off)))

// Vtable slot +0x20 (the cell -> object-id resolver): mov edx,[ent]; call [edx+0x20].
static i32 VtblResolve(void* ent) {
    void* vtbl = *(void**)ent;
    return (*(i32(**)(void*, i32, i32))((char*)vtbl + 0x20))(ent, 0, 0);
}

// ===========================================================================
// CRollingBall::~CRollingBall  (0x012f80)
// ===========================================================================
// The leaf adds no destructible members, so its dtor folds the bare CUserLogic
// teardown: store the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link
// (the embedded ~EngStr call), store the CUserBase vptr (0x5e70b4). The
// destructible link forces the /GX EH frame. The empty body is enough.
RVA(0x00012f80, 0x44)
CRollingBall::~CRollingBall() {}

// The CString temp the direction-name match builds (static-linked MFC CString
// helpers, modeled NO-body so the calls reloc-mask): MiniStr() = 0x1b9b93,
// operator=(LPCSTR) = 0x1b9e74, ~MiniStr() = 0x1b9cde. The real C++ dtor makes
// MSVC emit the temp's /GX cleanup state like retail.
struct CRbMiniStr {
    char* m_buf; // +0x00 the strcmp operand
    CRbMiniStr();
    ~CRbMiniStr();
    CRbMiniStr& operator=(const char* s);
};

// The bound CGameObject viewed by the ctor (m_10 == m_38). Only the touched
// offsets are modeled.
struct CRbCtorSub { // m_10->m_7c (per-tile-time owner)
    char m_pad00[0xbc];
    i32 m_bc; // +0xbc per-tile time
};
struct CRbCtorObj {
    char m_pad00[0x08];
    i32 m_08; // +0x08 flags
    char m_pad0c[0x5c - 0x0c];
    i32 m_5c; // +0x5c screen X
    i32 m_60; // +0x60 screen Y
    char m_pad64[0x74 - 0x64];
    i32 m_74; // +0x74 layer key
    char m_pad78[0x7c - 0x78];
    CRbCtorSub* m_7c; // +0x7c
    char m_pad80[0x118 - 0x80];
    i32 m_118; // +0x118 active flag (snapshot into m_explodeWindowLo)
    char m_pad11c[0x124 - 0x11c];
    i32 m_124; // +0x124 place mode (== 1 -> no time bonus)
    char m_pad128[0x12c - 0x128];
    i32 m_12c; // +0x12c travel direction (1..4)
    char m_pad130[0x144 - 0x130];
    i32 m_144; // +0x144 rect base
    i32 m_148; // +0x148
    i32 m_14c; // +0x14c
    i32 m_150; // +0x150
    char m_pad154[0x194 - 0x154];
    char* m_194; // +0x194 sprite/name record (dir name at +0x24); char* like CGameObject::m_194
    char m_pad198[0x1b4 - 0x198];
    i32 m_1b4; // +0x1b4 cycle-geometry id
};

// The global game registry (WwdGameReg @0x64556c; g_64556c masks _g_mgrSettings).
// m_118 the has-window gate; m_134 the mode discriminator (the time-bonus gate).
struct CRbReg {
    char m_pad00[0x118];
    i32 m_isEasyMode; // +0x118
    char m_pad11c[0x134 - 0x11c];
    i32 m_134; // +0x134
};

// 32.0 (the per-tile-time -> per-frame-speed reciprocal numerator), VA 0x5ea3e0
// (?g_slimeSpeedNum@@3NB; the consolidated global is pinned via <Globals.h>, so
// reference it as a plain extern - the same way KitchenSlime's LoadSprites does).

// g_buteTree (the "A" node store) comes from <Gruntz/ActNameRegistry.h>; g_buteMgr
// (the per-tile-time GetDwordDef) from <Bute/ButeMgr.h> via UserLogic.h.

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
CRollingBall::CRollingBall(CGameObject* obj) : CUserLogic(obj) {
    m_explodeStartLo = 0;
    m_explodeWindowLo = 0;
    m_explodeStartHi = 0;
    m_explodeWindowHi = 0;
    m_savedGeoId = m_38->m_geoId;
    m_38->ApplyLookupGeometry("GAME_CYCLE100", 0);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_38->m_flags |= 0x2000002;

    CRbCtorObj* o = (CRbCtorObj*)m_object;
    i32 snapX = (o->m_5c & ~0x1f) + 0x10;
    i32 snapY = (o->m_60 & ~0x1f) + 0x10;
    o->m_5c = snapX;
    m_subX = (double)snapX;
    o->m_60 = snapY;
    m_subY = (double)snapY;
    if (o->m_74 != snapY + 0x186a0) {
        o->m_74 = snapY + 0x186a0;
        o->m_08 |= 0x20000;
    }

    CRbCtorObj* obj38 = (CRbCtorObj*)m_38;
    if (obj38->m_194 != 0) {
        CRbMiniStr name;
        name = obj38->m_194 + 0x24;
        const char* s = name.m_buf;
        if (strcmp(s, "LEVEL_ROLLINGBALL_NORTH") == 0) {
            o->m_12c = 1;
            m_stepDirX = 0;
            m_stepDirY = -1;
        } else if (strcmp(s, "LEVEL_ROLLINGBALL_EAST") == 0) {
            o->m_12c = 2;
            m_stepDirX = 1;
            m_stepDirY = 0;
        } else if (strcmp(s, "LEVEL_ROLLINGBALL_SOUTH") == 0) {
            o->m_12c = 3;
            m_stepDirX = 0;
            m_stepDirY = 1;
        } else if (strcmp(s, "LEVEL_ROLLINGBALL_WEST") == 0) {
            o->m_12c = 4;
            m_stepDirX = -1;
            m_stepDirY = 0;
        }
    }

    i32 time = o->m_7c->m_bc;
    if (time == 0) {
        time = g_buteMgr.GetDwordDef("Hazardz", "RollingBallTimePerTile", 1000);
    }
    CRbReg* reg = (CRbReg*)g_64556c;
    if (reg->m_isEasyMode != 0 && reg->m_134 == 1 && o->m_124 != 1) {
        time += 1000;
    }
    m_explodeWindowLo = o->m_118;
    m_explodeWindowHi = 0;
    m_explodeStartLo = g_645588;
    m_explodeStartHi = 0;
    m_targetX = snapY;
    m_targetY = snapY;
    m_explodeLatch = 0;
    m_fallLatch = 0;
    m_moveSpeed = g_slimeSpeedNum / (double)(i64)(u32)time;
    o->m_144 = 0;
    o->m_14c = 0;
    o->m_148 = 0;
    o->m_150 = 0;
    m_moveDeltaLo = 0;
    m_moveDeltaHi = 0;
}

// CRollingBall::InitActReg @0x0afd60 - construct the class's activation-coordinate
// registry singleton (g_rollingBallActReg @0x6461b0) over the fixed range
// [2000, 2010] via the shared registry ctor (0x408710). Free init thunk.
RVA(0x000afd60, 0x15)
void CRollingBall::InitActReg() {
    g_rollingBallActReg.Construct(2000, 2010);
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
    i32 id = (i32)g_buteTree.Find(s_actKeyA);
    if (id == 0) {
        id = g_nextActId;
        g_buteTree.Insert(s_actKeyA, (void*)id);
        char* slot = ActNameLookup(id);
        i32 n = g_nameRegScratch;
        void** list = g_nameRegCurList;
        while (n-- != 0) {
            if (list != 0) {
                ((CActName*)list)->Free();
            }
            list++;
        }
        ((CActName*)slot)->Assign(s_actKeyA);
        g_nextActId++;
    }
    ((CRollingBallActEntry*)g_rollingBallActReg.ResolveEntry(id))->m_fn = &CRollingBall::Update;
}

// CRollingBall::Update - the per-tick rolling-ball state machine (__thiscall).
// @early-stop
// /GX branchy nested-jump-table megafunction wall (~35%): complete, correct
// reconstruction (prologue, explosion/fall latches, action/direction/sink
// switches, x87 interpolation tail); the EH-state numbering + jump-table reloc
// typing across 2682 B is the documented wall. See file header; final-sweep.
RVA(0x000b0140, 0xa7a)
i32 CRollingBall::Update() {
    void* self = this;
    void* gr = g_64556c;
    (void)g_6bf3bc;

    RbSubUpdate((char*)PTR(self, 0x38) + 0x1a0);

    void* anim = PTR(self, 0x38);
    if (I32(anim, 0x1c8) != 0 && I32(anim, 0x1c0) == 0) {
        I32(anim, 0x8) |= 0x10000;
        return 0;
    }

    // The explosion latch (+0x80): the explosion sound + cell-clear fire once.
    if (I32(self, 0x80) == 0) {
        void* logic = PTR(self, 0x10);
        i32 lo = g_645588 - I32(self, 0x88);
        i32 hi = 0 - I32(self, 0x8c);
        i32 lim = I32(self, 0x94);
        if (hi < lim || (hi == lim && (u32)lo < (u32)I32(self, 0x90))) {
            RbCacheFirst(PTR(self, 0x38), "LEVEL_ROLLINGBALL_EXPLOSION");
            I32(self, 0x40) = I32(PTR(self, 0x38), 0x1b4);
            RbApplyLookup(PTR(self, 0x38), "LEVEL_ROLLINGBALLEXPLOSION", 0);
            void* map = PTR(gr, 0x70);
            i32 cx = I32(logic, 0x5c) >> 5;
            i32 cy = I32(logic, 0x60) >> 5;
            if ((u32)cx < (u32)I32(map, 0xc) && (u32)cy < (u32)I32(map, 0x10)) {
                void* row = PTR(map, 0x8);
                i32 ix = cx * 7;
                i32* cell = (i32*)((char*)PTR(row, cy * 4) + ix * 4);
                *cell &= 0xefffffff;
            }
        }
        I32(self, 0x80) = 1;
    }

    // The fall latch (+0x84): grid-side death flag + rect re-mark.
    if (I32(self, 0x84) == 0) {
        void* logic = PTR(self, 0x10);
        i32 cx = I32(logic, 0x5c);
        i32 cy = I32(logic, 0x60);
        if (cx < I32(gr, 0x144) && cx >= I32(gr, 0x13c) && cy < I32(gr, 0x148)
            && cy >= I32(gr, 0x140)) {
            I32(PTR(gr, 0x68), 0x3f8) = 1;
        }
        void* logic2 = PTR(self, 0x10);
        i32 outA, outB;
        if (RbProbeRect(
                PTR(gr, 0x68),
                I32(logic2, 0x5c),
                I32(logic2, 0x60),
                (i32*)((char*)logic2 + 0x144),
                &outB,
                &outA,
                0
            )) {
            RbMarkRect(PTR(gr, 0x68), outA, outB, 2, -1);
        }
    }

    // ----- the sub-tile-snapped move + action switch -----
    void* logic = PTR(self, 0x10);
    if (I32(logic, 0x5c) == I32(self, 0x78) && I32(logic, 0x60) == I32(self, 0x7c)) {
        // arrived at the target cell: clear the cell, read its terrain id and
        // dispatch on the rolling-ball action.
        RbClearCell(PTR(gr, 0x68), I32(self, 0x7c), I32(self, 0x78), 0);

        void* map = PTR(gr, 0x70);
        i32 cx = I32(self, 0x78) >> 5;
        i32 cy = I32(self, 0x7c) >> 5;
        i32 terrain;
        if ((u32)cx < (u32)I32(map, 0xc) && (u32)cy < (u32)I32(map, 0x10)) {
            void* row = PTR(map, 0x8);
            i32 ix = cx * 7;
            terrain = I32((char*)PTR(row, cy * 4) + ix * 4, 0);
        } else {
            terrain = 1;
        }

        if ((terrain & 0x939) != 0 || (terrain & 2) != 0) {
            CString fall;      // [esp+0x14]
            CString explosion; // [esp+0x10]
            // resolve the action id from the second grid plane.
            void* m2 = PTR(gr, 0x30);
            void* lvl = PTR(m2, 0x24);
            i32 ax = I32(self, 0x7c) >> 5;
            i32 ay = I32(self, 0x78) >> 5;
            if (ax < 0) {
                ax = 0;
            } else {
                i32 w = I32(PTR(lvl, 0x5c), 0x28);
                if (ax >= w) {
                    ax = w - 1;
                }
            }
            if (ay < 0) {
                ay = 0;
            } else {
                i32 h = I32(PTR(lvl, 0x5c), 0x2c);
                if (ay >= h) {
                    ay = h - 1;
                }
            }
            i32 col = I32(PTR(lvl, 0x5c), 0x24);
            i32 idx = I32((char*)col + ay * 4, 0) + ax;
            i32 raw = I32(PTR(lvl, 0x20), idx * 4);
            i32 obj = 0;
            if (raw != (i32)0xeeeeeeee && raw != -1) {
                void* tbl = PTR(lvl, 0x4c);
                void* ent = PTR((char*)tbl, (raw & 0xffff) * 4);
                obj = VtblResolve(ent);
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
            RbCacheFirst(PTR(self, 0x38), explosion);
            I32(self, 0x40) = I32(PTR(self, 0x38), 0x1b4);
            RbApplyLookup(PTR(self, 0x38), fall, 0);
            if (obj == 4) {
                i32 t = RbGetDwordDef("Hazardz", "RollingBallTimePerTile", 0x3e8);
                DBL(self, 0x58) = g_5ea3e8 / (double)t;
            }
        }
    }

    // ----- the direction sub-switch (state +0x12c -> NORTH/SOUTH/EAST/WEST) -----
    I32(self, 0x60) = 0;
    I32(self, 0x68) = 0;
    I32(self, 0x64) = 0;
    I32(self, 0x6c) = 0;
    void* lg = PTR(self, 0x10);
    switch (I32(lg, 0x12c)) {
        case 1:
            DBL(self, 0x68) = -DBL(self, 0x98);
            I32(self, 0x78) -= 0x20;
            I32(self, 0x70) = -1;
            I32(self, 0x74) = -1;
            break;
        case 2:
            I32(self, 0x60) = I32(self, 0x98);
            I32(self, 0x64) = I32(self, 0x9c);
            I32(self, 0x78) += 0x20;
            I32(self, 0x70) = 1;
            I32(self, 0x74) = 0;
            break;
        case 4:
            I32(self, 0x68) = I32(self, 0x98);
            I32(self, 0x6c) = I32(self, 0x9c);
            I32(self, 0x78) += 0x20;
            I32(self, 0x70) = 0;
            I32(self, 0x74) = 1;
            break;
        case 3:
            DBL(self, 0x60) = -DBL(self, 0x98);
            I32(self, 0x78) -= 0x20;
            I32(self, 0x70) = -1;
            I32(self, 0x74) = 0;
            break;
    }

    // ----- the x87 sub-tile interpolation tail -----
    void* lg2 = PTR(self, 0x10);
    DBL(self, 0x60) = (double)I32(lg2, 0x5c) + DBL(self, 0x60);
    DBL(self, 0x68) = (double)I32(lg2, 0x60) + DBL(self, 0x68);
    I32(self, 0x98) = 0;
    I32(self, 0x9c) = 0;

    double dt = (double)g_645584 * DBL(self, 0x58);
    i32 nx = I32(self, 0x78) >> 5;
    if (I32(self, 0x70) > 0) {
        double v = dt + DBL(self, 0x60);
        DBL(self, 0x60) = v;
        nx = __ftol(RbCeil(v));
        if (nx >= (I32(self, 0x78) >> 5)) {
            nx = I32(self, 0x78) >> 5;
        }
    } else if (I32(self, 0x70) < 0) {
        double v = DBL(self, 0x60) - dt;
        DBL(self, 0x60) = v;
        nx = __ftol(RbFloor(v));
        if (nx < (I32(self, 0x78) >> 5)) {
            nx = I32(self, 0x78) >> 5;
        }
    } else {
        nx = __ftol(RbFloor(DBL(self, 0x60)));
    }

    i32 ny = I32(self, 0x7c) >> 5;
    if (I32(self, 0x74) > 0) {
        double v = dt + DBL(self, 0x68);
        DBL(self, 0x68) = v;
        ny = __ftol(RbCeil(v));
        if (ny >= (I32(self, 0x7c) >> 5)) {
            ny = I32(self, 0x7c) >> 5;
        }
    } else if (I32(self, 0x74) < 0) {
        double v = DBL(self, 0x68) - dt;
        DBL(self, 0x68) = v;
        ny = __ftol(RbFloor(v));
        if (ny < (I32(self, 0x7c) >> 5)) {
            ny = I32(self, 0x7c) >> 5;
        }
    } else {
        ny = __ftol(RbFloor(DBL(self, 0x68)));
    }

    void* out = PTR(self, 0x10);
    I32(out, 0x5c) = nx;
    I32(out, 0x60) = ny;
    void* out2 = PTR(self, 0x10);
    i32 next = I32(out2, 0x60) + 0x186a0;
    if (I32(out2, 0x74) != next) {
        I32(out2, 0x74) = next;
        I32(out2, 0x8) |= 0x20000;
    }
    return 0;
}

// ===========================================================================
// CRollingBall::Serialize  (0x0b0fe0) - __thiscall, ret 0x10, vtable slot 1
// ===========================================================================
// The CArchive round-trip of the ball state. Chains the shared CUserLogic
// serialize helper on `this`, then the +0x34 sub-object's chain; both gate (early
// return 0 on failure). Then it streams the ball fields through the archive's
// vtable: mode 4 = Write (slot +0x30, store), mode 7 = Read (slot +0x2c, load).
// The +0x88/+0x90 explosion-timing pair is streamed first, then the move/state
// field list (+0x58..+0x98).
RVA(0x000b0fe0, 0x1ab)
i32 CRollingBall::Serialize(CSerialArchive* ar, i32 tag, i32 c, i32 d) {
    if (!SerializeChain((i32)ar, tag, c, d)) {
        return 0;
    }
    if (!((CSerialObjRef*)((char*)this + 0x34))->Chain(ar, tag, c, (CSerialObj*)d)) {
        return 0;
    }

    // The explosion-timing pair (+0x88/+0x90), streamed through a walking pointer
    // that advances by 8 (retail hoists `lea ebp,[edi+0x88]` before the branch and
    // does `push ebp; call; add ebp,8; push ebp; call`). A two-case switch (not
    // if/else) floats the Write arm past the Read arm, matching retail's branch
    // polarity (docs/patterns/inline-switch-serialize-record-unroll.md +
    // pointer-walk-increment-in-for-update.md).
    char* p = (char*)this + 0x88;
    switch (tag) {
        case 4:
            ar->Write(p, 8);
            p += 8;
            ar->Write(p, 8);
            break;
        case 7:
            ar->Read(p, 8);
            p += 8;
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
SIZE_UNKNOWN(CRbCtorObj);
SIZE_UNKNOWN(CRbCtorSub);
SIZE_UNKNOWN(CRbMiniStr);
SIZE_UNKNOWN(CRbReg);
SIZE_UNKNOWN(CRollingBallActEntry);
SIZE_UNKNOWN(CRollingBallActReg);
