// PathHazard.cpp - the path-following hazard game object (C:\Proj\Gruntz).
//
// Four trace-discovered CPathHazard methods, defined in ascending retail-RVA
// order:
//   ~CPathHazard   @0x013340 - the /GX leaf dtor (folds the CUserLogic teardown).
//   Tick          @0x0b4020 - the per-frame movement-integrator driver (vslot 16).
//   BeginLeg      @0x0b47e0 - seed the unit vector toward the waypoint (vslot 19).
//   ForwardTick   @0x0b5070 - a thin non-virtual forwarder to vslot 16 (Tick).
//
// CPathHazard : CUserLogic (the base hierarchy comes from <Gruntz/UserLogic.h>);
// the hazard reads the bound CGameObject (m_10/m_38) directly. Only offsets /
// code bytes are load-bearing; names are placeholders for the recovered engine
// identities.
#include <Gruntz/PathHazard.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/LightFxMgr.h> // CLightFxMgr (g_lightGameReg->m_logicPump @+0x78; m_tables[])
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SoundCue.h> // the shared positional-sound cue subsystem

#include <math.h> // sqrt - inlines to fsqrt at /O2; the (int)double casts lower to __ftol

// CPathHazard's own Tick / ForwardTick now dispatch its added virtuals directly
// (`this->Tick/Arrive/BeginLeg/HitTest()`): CUserLogic is modeled at its full 16
// slots (UserLogic.h), so CPathHazard's five added virtuals land at their true
// retail slots 16..20 (+0x40 Tick, +0x48 Arrive, +0x4c BeginLeg, +0x50 HitTest) -
// the former base-vtable slot-count wall (14-slot base -> Tick mis-placed at slot
// 14) is lifted, and the manual CPathHazardVtbl offset-view is gone. (CLightningHazard
// below still reads the same slots RAW via CLightVtbl: it cannot derive CPathHazard -
// its /GX leaf dtor must fold the bare-CUserLogic teardown, which an out-of-line
// CPathHazard base dtor would block - so its raw view is retained.)

// ---------------------------------------------------------------------------
// A sibling timed/striking path-hazard (the CRainCloud/CUFO family; proximity-
// attributed to CPathHazard). It shares CPathHazard's layout + vtable (Tick reads
// the same slots) and the bare-CUserLogic leaf dtor, adding a strike-window timer
// pair: m_118 the strike-armed gate, the (m_120,m_124) i64 strike deadline and
// (m_128,m_12c) i64 window. Only offsets / code bytes are load-bearing.
// ---------------------------------------------------------------------------
// Derives from CUserLogic directly (NOT CPathHazard, whose dtor is out-of-line):
// the leaf dtor must fold the bare CUserLogic teardown inline to match 0x13280,
// which an out-of-line CPathHazard base dtor would block. It shares CPathHazard's
// vtable/layout only at the byte level (Tick reads the vtable raw).
class CLightningHazard : public CTileLogic {
public:
    i32 SiblingTick();                    // 0x0b43f0 (virtual slot 16 override)
    i32 ArmStrike(i32, i32);              // 0x0b4640 (arm the strike window + kill cue)
    virtual ~CLightningHazard() OVERRIDE; // 0x013280 (folds the CUserLogic teardown)

    char m_pad40[0x108 - 0x40];
    i64 m_legDeadline; // +0x108 leg start-clock deadline (i64)
    i64 m_legWindow;   // +0x110 leg window duration      (i64)
    i32 m_strikeArmed; // +0x118 strike-armed gate
    char m_pad11c[0x120 - 0x11c];
    i64 m_strikeDeadline; // +0x120 strike start-clock deadline (i64)
    i64 m_strikeWindow;   // +0x128 strike window duration      (i64)
};

// The sibling's vtable view (its own slot PMFs; same slot offsets as CPathHazard).
typedef i32 (CLightningHazard::*LightBeginFn)();
typedef i32 (CLightningHazard::*LightHitFn)(i32, i32);
struct CLightVtbl {
    char s0[0x4c];
    LightBeginFn BeginLeg; // +0x4c  slot 19
    LightHitFn HitTest;    // +0x50  slot 20
};

// The strike-clock + threshold globals the timer windows poll.
DATA(0x00245588)
extern i32 g_strikeClock;  // 0x645588 (the draw-clock; also g_pathLegTag)
extern i32 g_strikeThresh; // 0x645598 (compared to 0x64)

// The sibling hazard reads its bound CGameObject (m_10) directly: the draw trio
// (+0x4c sprite-ref / +0x50 state / +0x58 active), screen pos (+0x5c/+0x60), the
// +0x144 query-rect base and the +0x198 layer descriptor - all on CGameObject.
// g_gameReg's +0x78 ref-index table holds the strike sprite-frame selectors; its
// +0x118/+0x134 are the window-mode gates.
// The positional-sound cue idiom (shared with the menu-select handler, see
// <Gruntz/SoundCue.h>): ArmStrike looks up "LEVEL_CLOUDHAZARDKILL" -> an emitter
// (m_10 the CSoundCueMgr play-object, m_14 last-play clock, m_18 cooldown), then
// plays it through the cue manager when the per-emitter cooldown has elapsed.

DATA(0x0024556c)
extern CGameRegistry* g_lightGameReg;

// Strike config globals: the bute window source + the sound-enable flag / cue tag
// pair the positional emit polls, plus the kill-cue clock.
extern CButeMgr g_buteMgr; // ?g_buteMgr@@3VCButeMgr@@A (butemgr unit)
DATA(0x0021ab20)
extern i32 g_sndEnabled; // 0x61ab20 (sound-enable flag)
DATA(0x0021ab24)
extern i32 g_sndCueTag; // 0x61ab24 (cue tag)
DATA(0x002bf3c0)
extern "C" u32 g_killCueClock; // 0x6bf3c0

// The "A" bute key the new-leg re-bind looks up (DAT_0060a454 $SG literal).
extern CButeTree g_buteTree;

// CLightningHazard::~ @0x013280 - byte-identical to ~CPathHazard (the bare
// CUserLogic leaf teardown); the empty body is enough for cl. (Distinct EH
// handler funclet from 0x13340, but that is reloc-masked.)
RVA(0x00013280, 0x44)
CLightningHazard::~CLightningHazard() {}

// CPathHazard::GetTypeTag @0x0132f0 - return the class's logic-type id. The same
// 6-byte `mov eax,<id>; ret` virtual archetype as CBehindCandy::GetTypeTag
// (0x00fb70); sits between CLightningHazard::~ and the [scalar,plain] CPathHazard
// dtor pair, the canonical [GetTypeTag][scalar-dtor][plain-dtor] per-class layout.
RVA(0x000132f0, 0x6)
LogicTypeId CPathHazard::GetTypeTag() {
    return LOGIC_PATHHAZARD; // 0x425
}

// CPathHazard::~CPathHazard @0x013340 - the leaf adds no destructible members
// beyond CUserLogic, so its dtor folds the bare CUserLogic teardown: store the
// CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the embedded
// ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The destructible
// link forces the /GX EH frame. Byte-identical in shape to ~CTimeBomb
// (0x012a70) / ~CKitchenSlime (0x013100); the empty body is enough for cl.
RVA(0x00013340, 0x44)
CPathHazard::~CPathHazard() {}

// The bound CGameObject viewed by the ctor: it reads the screen position (m_5c/
// m_60), the layer key (m_74), the flags (m_08), and the raw waypoint coordinate
// arrays (the 12 ints at +0x134, the 4 ints at +0x64, and the 8 ints at
// m_7c->+0xf0). The hazard scales each tile coordinate to a pixel centre
// (coord*0x20 + 0x10). Only the touched offsets are modeled.
struct CPathCtorSub { // m_object->m_7c (the per-tile-time + extra waypoint owner)
    char m_pad00[0xbc];
    i32 m_bc; // +0xbc per-tile time (0 -> read PathHazardTimePerTile)
    char m_padc0[0xf0 - 0xc0];
    i32 m_f0, m_f4, m_f8, m_fc, m_100, m_104, m_108, m_10c; // +0xf0..0x10c (8 coords)
};
struct CPathCtorObj {
    char m_pad00[0x08];
    i32 m_08; // +0x08 flags
    char m_pad0c[0x5c - 0x0c];
    i32 m_5c, m_60;             // +0x5c, +0x60 screen position (wp[0])
    i32 m_64, m_68, m_6c, m_70; // +0x64..0x70 (4 coords)
    i32 m_74;                   // +0x74 layer key
    char m_pad78[0x7c - 0x78];
    CPathCtorSub* m_7c; // +0x7c
    char m_pad80[0x134 - 0x80];
    i32 m_134, m_138, m_13c, m_140, m_144, m_148, m_14c, m_150, m_154, m_158, m_15c,
        m_160; // +0x134..0x160 (12 coords)
};

// CPathHazard::CPathHazard @0xb35a0 - fold the shared CUserLogic(obj) init, then
// build the hazard's waypoint path: snap the bound object's screen position to the
// tile grid (the m_60/m_68 doubles + the m_74 layer key), then scale the raw
// per-tile waypoint coordinates (the 12 ints at obj+0x134, the 4 at obj+0x64, the
// 8 at obj->m_7c+0xf0) to pixel centres (coord*0x20 + 0x10) into the +0x90
// waypoint array (wp[0] is the unscaled start). Find the path length (the first
// waypoint equal to the (0x10,0x10) sentinel), seed the per-tile time
// (PathHazardTimePerTile when unset), start the first leg, and on success bind the
// "A" bute node + cycle geometry (else hide the object).
//
// @early-stop
// register-pinning/eh-ctor-vptr-restamp wall (docs/patterns/zero-register-pinning.md,
// eh-ctor-vptr-restamp-position.md): body byte-faithful (the i64 zeroing, the 24
// scaled waypoint copies with 0x10 pinned in ebx, the sentinel search loop, the
// bute/geometry tail all match retail). Residual is the /GX leaf-vptr re-stamp
// position + retail's walking-pointer reuse of the copy's ecx in the search loop
// (a regalloc artifact, not source-steerable).
RVA(0x000b35a0, 0x401)
CPathHazard::CPathHazard(CGameObject* obj) : CTileLogic(obj) {
    *(i64*)&m_legTag = 0;
    *(i64*)&m_legSegs = 0;
    *(i64*)&m_strikeDeadline = 0;
    *(i64*)&m_strikeWindow = 0;

    m_38->m_flags |= 0x2000002;

    CPathCtorObj* o = (CPathCtorObj*)m_object;
    i32 snapX = (o->m_5c & ~0x1f) + 0x10;
    i32 snapY = (o->m_60 & ~0x1f) + 0x10;
    o->m_5c = snapX;
    m_posX = (double)snapX;
    o->m_60 = snapY;
    m_posY = (double)snapY;
    if (o->m_74 != 0xcf850) {
        o->m_74 = 0xcf850;
        o->m_08 |= 0x20000;
    }

    m_wp[0].x = o->m_5c;
    m_wp[0].y = o->m_60;
    m_wp[1].x = (o->m_134 << 5) + 0x10;
    m_wp[1].y = (o->m_138 << 5) + 0x10;
    m_wp[2].x = (o->m_13c << 5) + 0x10;
    m_wp[2].y = (o->m_140 << 5) + 0x10;
    m_wp[3].x = (o->m_144 << 5) + 0x10;
    m_wp[3].y = (o->m_148 << 5) + 0x10;
    m_wp[4].x = (o->m_14c << 5) + 0x10;
    m_wp[4].y = (o->m_150 << 5) + 0x10;
    m_wp[5].x = (o->m_154 << 5) + 0x10;
    m_wp[5].y = (o->m_158 << 5) + 0x10;
    m_wp[6].x = (o->m_15c << 5) + 0x10;
    m_wp[6].y = (o->m_160 << 5) + 0x10;
    m_wp[7].x = (o->m_64 << 5) + 0x10;
    m_wp[7].y = (o->m_68 << 5) + 0x10;
    m_wp[8].x = (o->m_6c << 5) + 0x10;
    m_wp[8].y = (o->m_70 << 5) + 0x10;
    m_wp[9].x = (o->m_7c->m_f0 << 5) + 0x10;
    m_wp[9].y = (o->m_7c->m_f4 << 5) + 0x10;
    m_wp[10].x = (o->m_7c->m_f8 << 5) + 0x10;
    m_wp[10].y = (o->m_7c->m_fc << 5) + 0x10;
    m_wp[11].x = (o->m_7c->m_100 << 5) + 0x10;
    m_wp[11].y = (o->m_7c->m_104 << 5) + 0x10;
    m_wp[12].x = (o->m_7c->m_108 << 5) + 0x10;
    m_wp[12].y = (o->m_7c->m_10c << 5) + 0x10;

    i32 i = 1;
    i32 found = 0;
    while (found == 0) {
        if (m_wp[i].x == 0x10 && m_wp[i].y == 0x10) {
            found = 1;
        } else {
            i++;
        }
        if (i >= 13) {
            break;
        }
    }
    m_wpCount = i;
    m_wpIndex = 0;

    if (o->m_7c->m_bc == 0) {
        o->m_7c->m_bc = g_buteMgr.GetDwordDef("Hazardz", "PathHazardTimePerTile", 1000);
    }

    if (StartPath() == 0) {
        m_38->m_flags |= 0x10000;
    } else {
        m_prevAnimSetNode = m_objAux->m_1c;
        m_objAux->m_1c = g_buteTree.Find("A");
        m_savedGeoId = m_38->m_geoId;
        m_38->ApplyLookupGeometry("GAME_CYCLE100", 0);
    }
}

// CPathHazard::Tick @0x0b4020 (virtual slot 16) - the per-frame driver. Advance
// the +0x1a0 sub-mgr; run the on-screen visibility/hit gate (unless the registry
// is in the no-window mode); if the bound object has reached the current
// waypoint tile, snap the doubles, fire the "arrived" handler, and either start
// the next leg (when no segments remain, vslot 19) or re-bind the leg's "B" bute
// node; otherwise integrate the sub-pixel movement vector into m_60/m_68 and
// write the snapped, waypoint-clamped tile position back to the bound object.
// @early-stop
// x87 FP movement-integrator wall (docs/patterns/x87-fp-stack-schedule.md): the
// integer scaffolding (the visibility gate, the arrival/leg blocks) is byte-exact;
// the residual is the x87 stack-slot scheduling of the dx/dy integrator (the same
// wall CKitchenSlime::Tick carries). Logic byte-for-byte correct.
RVA(0x000b4020, 0x26c)
i32 CPathHazard::Tick() {
    ((CPathSubMgr*)((char*)m_38 + 0x1a0))->Advance(g_pathTick);

    CGameObject* obj = m_object;
    // The probe rect (a 4-int local) the on-screen query tests, computed
    // unconditionally: {left, top, right, bottom} around the bound object's
    // screen position, inset by the layer base (m_198->m_18/m_1c, re-read each
    // component as retail does) and a 7px margin.
    i32 rect[4];
    rect[0] = obj->m_screenX - obj->m_layer->m_baseX + 7;
    rect[2] = obj->m_layer->m_baseX + obj->m_screenX - 7;
    rect[1] = obj->m_screenY - obj->m_layer->m_1c + 7;
    rect[3] = obj->m_layer->m_1c + obj->m_screenY - 7;

    CGameRegistry* reg = g_pathGameReg;
    if (reg->m_isEasyMode == 0 || reg->m_134 != 1) {
        i32 outA, outB;
        CPathEntity* ent =
            (CPathEntity*)((CPathCueGate*)reg->m_cmdGrid)
                ->QueryAt(obj->m_screenX, obj->m_screenY, &obj->m_areaL, &outA, &outB, rect);
        if (ent != 0 && ent->m_258 != 0x38) {
            if (g_pathGameReg->m_134 != 1 || outA != 0) {
                if (this->HitTest(outA, outB) == 0) { // virtual slot 20 (+0x50)
                    return 0;
                }
            }
        }
    }

    CGameObject* m10 = m_object;
    i32 wx = m_wpX;
    if (m10->m_screenX == wx) {
        i32 wy = m_wpY;
        if (m10->m_screenY == wy) {
            // Arrived at the waypoint tile.
            m_posX = (double)wx;
            m_posY = (double)wy;
            this->Arrive(); // virtual slot 18 (+0x48)
            i32 segs = m_object->m_120;
            if (segs > 0) {
                m_legSegs = segs;
                m_legSegsHi = 0;
                m_legTag = g_pathLegTag;
                m_legTagHi = 0;
                m_prevAnimSetNode = m_objAux->m_1c;
                m_objAux->m_1c = g_buteTree.Find(g_iconBute);
                return 0;
            }
            this->BeginLeg(); // virtual slot 19 (+0x4c)
            return 0;
        }
    }

    // Not arrived: integrate the sub-pixel movement vector toward the waypoint.
    double step = (double)(i64)(u64)(u32)g_pathStepSeed * m_speed;
    m_posX = m_posX + step * m_unitX;
    m_posY = m_posY + (double)(u32)g_pathStepSeed * m_unitY * m_speed;
    i32 newX = (i32)(m_roundBiasX + m_posX);
    i32 newY = (i32)(m_roundBiasY + m_posY);

    if (m_unitX > g_pathZero) {
        if (newX > m_wpX) {
            newX = m_wpX;
        }
    } else if (m_unitX < g_pathZero) {
        if (newX < m_wpX) {
            newX = m_wpX;
        }
    }

    if (m_unitY > g_pathZero) {
        if (newY > m_wpY) {
            newY = m_wpY;
        }
    } else if (m_unitY < g_pathZero) {
        if (newY < m_wpY) {
            newY = m_wpY;
        }
    }

    m_object->m_screenX = newX;
    m_object->m_screenY = newY;
    return 0;
}

// CLightningHazard::SiblingTick @0x0b43f0 (virtual slot 16 override) - the timed
// striking hazard's per-frame driver. When armed (m_118), check the strike window:
// if the i64 (clock - deadline) is past the window OR the strike-threshold gate
// expired, disarm and pick the "spent" sprite frame; otherwise pick the active
// frame (selector index 5 vs 0); seed the bound object's draw state (active=1,
// state=7, sprite-ref from g_gameReg->m_78). Then advance the +0x1a0 sub-mgr, run
// the on-screen visibility/hit gate, and on arrival fire BeginLeg + re-bind the
// "A" bute node. Integer-only; returns 0.
// @early-stop
// ~86%: the integer scaffolding (the i64 strike-window compares, the visibility
// gate, the arrival/BeginLeg + bute re-bind) is byte-correct. The residual is two
// documented tails: (1) the TU models g_gameReg as ?g_lightGameReg while the retail
// obj names it _g_mgrSettings, so its repeated DIR32 data relocs stay fuzzy (a
// TU-wide rename, the matcher.md reloc-naming artifact); (2) the dual signed-i64
// `>=` window compares lay the expire/check branches in a different order than
// retail's hi-dword `jg/jl; cmp lo; jae` materialization. Logic correct; deferred.
RVA(0x000b43f0, 0x1c7)
i32 CLightningHazard::SiblingTick() {
    if (m_strikeArmed != 0) {
        i32 sel = 5;
        i64 elapsed = (i64)(u32)g_strikeClock - m_strikeDeadline;
        if (elapsed >= m_strikeWindow) {
            m_strikeArmed = 0;
        } else if (g_strikeThresh < 0x64) {
            sel = 0;
        }
        CGameObject* o = m_object;
        o->m_drawActive = 1;
        o->m_drawFillCmd = 7;
        o->m_drawFillArg = (i32)g_lightGameReg->m_logicPump->m_tables[sel]; // [m_78 + sel*4 + 0x14]
    }

    ((CPathSubMgr*)((char*)m_38 + 0x1a0))->Advance(g_pathTick);

    CGameObject* obj = m_object;
    i32 rect[4];
    rect[0] = obj->m_screenX - obj->m_layer->m_baseX + 7;
    rect[2] = obj->m_layer->m_baseX + obj->m_screenX - 7;
    rect[1] = obj->m_screenY - obj->m_layer->m_1c + 7;
    rect[3] = obj->m_layer->m_1c + obj->m_screenY - 7;

    CGameRegistry* reg = g_lightGameReg;
    if (reg->m_isEasyMode != 0 && reg->m_134 == 1) {
        // window mode, skip the query
    } else {
        i32 outA, outB;
        CPathEntity* ent =
            (CPathEntity*)((CPathCueGate*)reg->m_cmdGrid)
                ->QueryAt(obj->m_screenX, obj->m_screenY, &obj->m_areaL, &outA, &outB, rect);
        if (ent != 0 && ent->m_258 != 0x38) {
            if (g_lightGameReg->m_134 != 1 || outA != 0) {
                CLightVtbl* vt = *(CLightVtbl**)this;
                if ((this->*(vt->HitTest))(outA, outB) == 0) {
                    return 0;
                }
            }
        }
    }

    i64 legElapsed = (i64)(u32)g_strikeClock - m_legDeadline;
    if (legElapsed >= m_legWindow) {
        CGameObject* o = m_object;
        o->m_drawActive = 1;
        o->m_drawFillCmd = 7;
        o->m_drawFillArg = (i32)g_lightGameReg->m_logicPump->m_tables[5]; // [m_78 + 0x28]
        CLightVtbl* vt = *(CLightVtbl**)this;
        (this->*(vt->BeginLeg))();
        m_prevAnimSetNode = m_objAux->m_1c;
        m_objAux->m_1c = g_buteTree.Find("A");
        m_strikeArmed = 0;
    }
    return 0;
}

// CLightningHazard::ArmStrike @0x0b4640 - arm the strike-window timer (deadline =
// now, window = bute RainCloudFlashTime), fire the cue gate, and play the
// "LEVEL_CLOUDHAZARDKILL" positional sound on the bound object when it is on-screen
// and the per-emitter cooldown has elapsed.  Integer-only; returns 1.  __thiscall,
// 2 args.
// @early-stop
// ~95%: code bytes byte-exact; residual is the same TU-wide reloc-naming artifact
// SiblingTick carries (the obj names g_lightGameReg as _g_mgrSettings and
// g_strikeClock as _g_645588). Logic byte-for-byte correct.
RVA(0x000b4640, 0x104)
i32 CLightningHazard::ArmStrike(i32 a, i32 b) {
    m_strikeArmed = 1;
    m_strikeWindow = (i64)(u32)g_buteMgr.GetDwordDef("Hazardz", "RainCloudFlashTime", 0x7d0);
    m_strikeDeadline = (i64)(u32)g_strikeClock;
    ((CPathCueGate*)g_lightGameReg->m_cmdGrid)->Strike(a, b, 9, -1);

    CGameObject* obj = m_object;
    CGameRegistry* reg = g_lightGameReg;
    i32 y = obj->m_screenY;
    i32 x = obj->m_screenX;
    if (x < reg->m_viewOriginR && x >= reg->m_viewOriginL && y < reg->m_viewOriginB
        && y >= reg->m_viewOriginT) {
        CSndHost* host = (CSndHost*)reg->m_world->m_28;
        if (host->m_30 == 0) {
            CSndEmitter* out = 0;
            host->m_10.Lookup("LEVEL_CLOUDHAZARDKILL", &out);
            if (out != 0) {
                i32 enabled = g_sndEnabled;
                i32 tag = g_sndCueTag;
                if (enabled != 0) {
                    u32 now = g_killCueClock;
                    if ((u32)(now - out->m_14) >= out->m_18) {
                        out->m_14 = now;
                        out->m_10->ConfigureItem(tag, 0, 0, 0);
                    }
                }
            }
        }
    }
    return 1;
}

// CPathHazard::BeginLeg @0x0b47e0 (virtual slot 19) - compute the unit vector
// toward the current waypoint (m_f8) and seed the per-leg movement state: the
// per-frame speed (m_58 = 1 / (m_7c->m_bc / 32)), the doubled current position
// (m_60/m_68), the unit-vector components (m_70/m_78) and their half-step sign
// biases (m_80/m_88 = sign * 0.5). Returns 1.
// @early-stop
// x87 FP normalize wall (docs/patterns/x87-fp-stack-schedule.md): the unit-vector
// sqrt-divide chain + the sign tests are the documented stack-scheduling residual;
// every offset / immediate / branch matches retail. Logic byte-for-byte correct.
RVA(0x000b47e0, 0x170)
i32 CPathHazard::BeginLeg() {
    CGameObject* obj = m_object;
    i32 idx = m_wpIndex;
    i32 wx = m_wp[idx].x;
    m_wpX = wx;
    i32 wy = m_wp[idx].y;
    m_wpY = wy;

    double dx = (double)m_wpX - (double)obj->m_screenX;
    double dy = (double)m_wpY - (double)obj->m_screenY;
    double len = sqrt(dx * dx + dy * dy);
    double ux = dx / len;
    double uy = dy / len;

    m_speed = g_pathOne / ((double)obj->m_7c->m_bc * g_pathTimeScale);
    m_posX = (double)obj->m_screenX;
    m_posY = (double)obj->m_screenY;
    m_unitX = ux;
    m_unitY = uy;

    if (ux > g_pathZero) {
        m_roundBiasX = 0.5;
    } else if (ux < g_pathZero) {
        m_roundBiasX = -0.5;
    } else {
        m_roundBiasX = 0.0;
    }

    if (uy > g_pathZero) {
        m_roundBiasY = 0.5;
    } else if (uy < g_pathZero) {
        m_roundBiasY = -0.5;
    } else {
        m_roundBiasY = 0.0;
    }
    return 1;
}

// CPathHazard::ForwardTick @0x0b5070 - a thin non-virtual forwarder that
// tail-jumps to the hazard's own virtual slot 16 (Tick): `mov eax,[ecx]; jmp
// [eax+0x40]`. The tail-call through the typed vtable emits the jmp directly.
RVA(0x000b5070, 0x5)
void CPathHazard::ForwardTick() {
    Tick(); // virtual slot 16 (+0x40); tail-jump `mov eax,[ecx]; jmp [eax+0x40]`
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
#include <rva.h>
SIZE_UNKNOWN(CGameRegistry);
SIZE_UNKNOWN(CLightVtbl);
SIZE_UNKNOWN(CLightningHazard);
SIZE_UNKNOWN(CPathCtorObj);
SIZE_UNKNOWN(CPathCtorSub);
SIZE_UNKNOWN(CPathCueGate);
SIZE_UNKNOWN(CPathEntity);
SIZE_UNKNOWN(CPathSubMgr);
SIZE_UNKNOWN(CPathWaypoint);
