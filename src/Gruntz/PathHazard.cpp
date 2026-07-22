#include <Gruntz/GruntzMgr.h> // complete CGruntzMgr
#include <Gruntz/PathHazard.h>
#include <Gruntz/SoundState.h> // g_sndEnabled/g_sndCueTag
#include <Gruntz/RainCloud.h>  // CRainCloud (its dtor 0x13340 lives in this obj)
#include <Gruntz/ActReg.h>     // CActReg coordinate registry (ResolveEntry) for RunAct
#include <Gruntz/LeafCue.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/LightFxMgr.h> // CLightFxMgr (g_gameReg->m_logicPump @+0x78; m_tables[])
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SoundCue.h> // the shared positional-sound cue subsystem
#include <Gruntz/TriggerMgr.h> // canonical CTriggerMgr (m_cmdGrid): FindGruntAt @0x75c60, CellDispatch @0x6bcb0

#include <math.h>           // sqrt - inlines to fsqrt at /O2; the (int)double casts lower to __ftol
#include <Rez/FrameClock.h> // g_timer200 (strike/leg deadline threshold)
#include <Image/CImage.h> // the +0x198 cached frame (ex CGameObjLayer view)



#include <Gruntz/PathHazardActReg.h> // g_actReg_646250 (ex .cpp extern)
RVA(0x00013170, 0x7b)
CPathHazard::CPathHazard() {
    m_legDeadline = 0;
    m_legWindow = 0;
    m_strikeDeadline = 0;
    m_strikeWindow = 0;
}

// CPathHazard::~CPathHazard @0x013280 - the leaf adds no destructible members beyond
// CUserLogic, so its dtor folds the bare CUserLogic teardown: store the CUserLogic vptr
// (0x5e705c), inline-destruct the +0x18 link (the embedded ~EngStr call 0x16d2a0), store
// the CUserBase vptr (0x5e70b4). The destructible link forces the /GX EH frame.
// IDENTITY (vtable-owner probe): ??_7CPathHazard @0x1e7394 slot 0 -> ILT thunk -> the
// scalar-deleting dtor 0x13250 -> THIS body. It was misbound as ~CLightningHazard, while
// ~CPathHazard was misbound to 0x13340 (which is really ~CRainCloud - see below): the
// whole family was shifted by one, because N byte-identical empty leaf dtors were being
// assigned by proximity instead of by the vtable that dispatches them.
#include <rva.h>
RVA_COMPGEN(0x00013280, 0x44, ??1CPathHazard@@UAE@XZ)

// (~CRainCloud x13340 is IMPLICIT and its COMDAT is emitted by raincloud.obj - the
// TU that constructs it - so its RVA_COMPGEN pin lives in RainCloud.cpp, not here.)

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
CPathHazard::CPathHazard(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_legDeadline = 0;
    m_legWindow = 0;
    m_strikeDeadline = 0;
    m_strikeWindow = 0;

    m_38->m_flags |= 0x2000002;

    CWwdGameObjectA* o = m_object;
    i32 snapX = (o->m_screenX & ~0x1f) + 0x10;
    i32 snapY = (o->m_screenY & ~0x1f) + 0x10;
    o->m_screenX = snapX;
    m_posX = static_cast<double>(snapX);
    o->m_screenY = snapY;
    m_posY = static_cast<double>(snapY);
    if (o->m_sortKey != 0xcf850) {
        o->m_sortKey = 0xcf850;
        o->m_flags |= 0x20000;
    }

    m_wp[0].x = o->m_screenX;
    m_wp[0].y = o->m_screenY;
    m_wp[1].x = (o->m_extent.left << 5) + 0x10;
    m_wp[1].y = (o->m_extent.top << 5) + 0x10;
    m_wp[2].x = (o->m_extent.right << 5) + 0x10;
    m_wp[2].y = (o->m_extent.bottom << 5) + 0x10;
    m_wp[3].x = (o->m_area.left << 5) + 0x10;
    m_wp[3].y = (o->m_area.top << 5) + 0x10;
    m_wp[4].x = (o->m_area.right << 5) + 0x10;
    m_wp[4].y = (o->m_area.bottom << 5) + 0x10;
    m_wp[5].x = (o->m_switchRect.left << 5) + 0x10;
    m_wp[5].y = (o->m_switchRect.top << 5) + 0x10;
    m_wp[6].x = (o->m_switchRect.right << 5) + 0x10;
    m_wp[6].y = (o->m_switchRect.bottom << 5) + 0x10;
    m_wp[7].x = (o->m_clip.left << 5) + 0x10;
    m_wp[7].y = (o->m_clip.top << 5) + 0x10;
    m_wp[8].x = (o->m_clip.right << 5) + 0x10;
    m_wp[8].y = (o->m_clip.bottom << 5) + 0x10;
    m_wp[9].x = (o->m_7c->m_switchRectA.left << 5) + 0x10;
    m_wp[9].y = (o->m_7c->m_switchRectA.top << 5) + 0x10;
    m_wp[10].x = (o->m_7c->m_switchRectA.right << 5) + 0x10;
    m_wp[10].y = (o->m_7c->m_switchRectA.bottom << 5) + 0x10;
    m_wp[11].x = (o->m_7c->m_switchRectB.left << 5) + 0x10;
    m_wp[11].y = (o->m_7c->m_switchRectB.top << 5) + 0x10;
    m_wp[12].x = (o->m_7c->m_switchRectB.right << 5) + 0x10;
    m_wp[12].y = (o->m_7c->m_switchRectB.bottom << 5) + 0x10;

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
        m_value = m_38->m_1a0.m_14;
        m_38->ApplyLookupGeometry("GAME_CYCLE100", 0);
    }
}

RVA(0x000b3b60, 0x102)
void CPathHazard::FireActivation(i32 id) {
    CPathHazardActEntry* e = reinterpret_cast<CPathHazardActEntry*>(g_actReg_646250.ResolveEntry(id));
    if (e->m_fn != 0) {
        (this->*(reinterpret_cast<CPathHazardActEntry*>(g_actReg_646250.ResolveEntry(id)))->m_fn)();
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
    m_38->m_1a0.Advance(g_engineFrameDelta);

    CWwdGameObjectA* obj = m_object;
    // The probe rect (a 4-int local) the on-screen query tests, computed
    // unconditionally: {left, top, right, bottom} around the bound object's
    // screen position, inset by the layer base (m_198->m_18/m_1c, re-read each
    // component as retail does) and a 7px margin.
    i32 rect[4];
    rect[0] = obj->m_screenX - obj->m_layer->m_anchorX + 7;
    rect[2] = obj->m_layer->m_anchorX + obj->m_screenX - 7;
    rect[1] = obj->m_screenY - obj->m_layer->m_anchorY + 7;
    rect[3] = obj->m_layer->m_anchorY + obj->m_screenY - 7;

    CGruntzMgr* reg = g_gameReg;
    if (reg->m_isEasyMode == 0 || reg->m_134 != 1) {
        i32 outA, outB;
        CPathEntity* ent = reinterpret_cast<CPathEntity*>(reg->m_cmdGrid->FindGruntAt(
            obj->m_screenX,
            obj->m_screenY,
            &obj->m_area,
            &outA,
            &outB,
            reinterpret_cast<RECT*>(rect)
        ));
        if (ent != 0 && ent->m_258 != 0x38) {
            if (g_gameReg->m_134 != 1 || outA != 0) {
                if (this->HitTest(outA, outB) == 0) { // virtual slot 20 (+0x50)
                    return 0;
                }
            }
        }
    }

    CWwdGameObjectA* m10 = m_object;
    i32 wx = m_wpX;
    if (m10->m_screenX == wx) {
        i32 wy = m_wpY;
        if (m10->m_screenY == wy) {
            // Arrived at the waypoint tile.
            m_posX = static_cast<double>(wx);
            m_posY = static_cast<double>(wy);
            this->Arrive(); // virtual slot 18 (+0x48)
            i32 segs = m_object->m_120;
            if (segs > 0) {
                m_legWindow = segs;
                m_legDeadline =
                    static_cast<u32>(g_frameTime); // the running game clock seeds the leg deadline
                m_prevAnimSetNode = m_objAux->m_1c;
                m_objAux->m_1c = g_buteTree.Find("B");
                return 0;
            }
            this->BeginLeg(); // virtual slot 19 (+0x4c)
            return 0;
        }
    }

    // Not arrived: integrate the sub-pixel movement vector toward the waypoint.
    double step =
        static_cast<double>(static_cast<i64>(static_cast<u64>(static_cast<u32>(g_frameDelta))))
        * m_speed;
    m_posX = m_posX + step * m_unitX;
    m_posY = m_posY + static_cast<double>(static_cast<u32>(g_frameDelta)) * m_unitY * m_speed;
    i32 newX = static_cast<i32>((m_roundBiasX + m_posX));
    i32 newY = static_cast<i32>((m_roundBiasY + m_posY));

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

// @early-stop
// 98.94%: every opcode/offset/branch is byte-identical. The lone residual is a
// load-order coin-flip in the sprite-write tail - retail reads g_gameReg->m_78
// (edx) before m_object (reusing eax for the sprite ptr); cl loads m_object first
// (into ecx) and pins the sprite there. A pure allocator choice on the
// [this+0x10] load; no source reorder flips it.
// HOMED here (matcher-6, tu-partition): the earlier "BOUNDARY COMDAT - not homeable"
// verdict read the 8-byte CUFO::Tick @0xb4330 as the lower neighbour, but that body is
// itself an outlier of Ufo.cpp. The real bracket is THIS TU on BOTH sides -
// CPathHazard::Tick @0xb4020 below and CPathHazard::SiblingTick @0xb43f0 above - and
// 0xb4350 lies inside this obj's own contiguous 0xb35a0..0xb5075 run, which a
// compiland's .text contribution must be. vtable_hierarchy independently tags this
// slot-16 override's ORIGIN as CPathHazard, and this TU already owns CRainCloud's dtor
// (0x13340) and includes <Gruntz/RainCloud.h>. (Ufo.cpp 0xb4330..0xb4fb7 and
// RainCloud.cpp 0xb49b0 are also entirely inside this run - i.e. this one retail obj
// holds CPathHazard plus its CUFO/CRainCloud leaves - so they want folding here too;
// left for a follow-up, they are not orphan TUs.)
RVA(0x000b4350, 0x7e)
i32 CRainCloud::Tick() {
    if (m_strikeArmed != 0) {
        i32 idx = 5;
        if (static_cast<i64>(static_cast<u32>(g_frameTime)) - m_strikeDeadline < m_strikeWindow) {
            if (static_cast<u32>(g_timer200) >= 0x64) {
                idx = 0;
            }
        } else {
            m_strikeArmed = 0;
        }
        i32 frame = reinterpret_cast<i32>(g_gameReg->m_logicPump->m_tables[idx]);
        CWwdGameObjectA* spr = m_object;
        spr->m_drawActive = 1;
        spr->m_drawFillArg = frame;
        spr->m_drawFillCmd = 7;
    }
    CPathHazard::Tick(); // the base chain (thunk 0x2914 -> 0xb4020), result unused
    return 0;
}

// CPathHazard::SiblingTick @0x0b43f0 (virtual slot 17) - the timed
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
// documented tails: (1) the TU models g_gameReg as ?g_gameReg while the retail
// obj names it _g_mgrSettings, so its repeated DIR32 data relocs stay fuzzy (a
// TU-wide rename, the matcher.md reloc-naming artifact); (2) the dual signed-i64
// `>=` window compares lay the expire/check branches in a different order than
// retail's hi-dword `jg/jl; cmp lo; jae` materialization. Logic correct; deferred.
RVA(0x000b43f0, 0x1c7)
i32 CPathHazard::SiblingTick() {
    if (m_strikeArmed != 0) {
        i32 sel = 5;
        i64 elapsed = static_cast<i64>(static_cast<u32>(g_frameTime)) - m_strikeDeadline;
        if (elapsed >= m_strikeWindow) {
            m_strikeArmed = 0;
        } else if (g_timer200 < 0x64) {
            sel = 0;
        }
        CWwdGameObjectA* o = m_object;
        o->m_drawActive = 1;
        o->m_drawFillCmd = 7;
        o->m_drawFillArg = reinterpret_cast<i32>(g_gameReg->m_logicPump->m_tables[sel]); // [m_78 + sel*4 + 0x14]
    }

    m_38->m_1a0.Advance(g_engineFrameDelta);

    CWwdGameObjectA* obj = m_object;
    i32 rect[4];
    rect[0] = obj->m_screenX - obj->m_layer->m_anchorX + 7;
    rect[2] = obj->m_layer->m_anchorX + obj->m_screenX - 7;
    rect[1] = obj->m_screenY - obj->m_layer->m_anchorY + 7;
    rect[3] = obj->m_layer->m_anchorY + obj->m_screenY - 7;

    CGruntzMgr* reg = g_gameReg;
    if (reg->m_isEasyMode != 0 && reg->m_134 == 1) {
        // window mode, skip the query
    } else {
        i32 outA, outB;
        CPathEntity* ent = reinterpret_cast<CPathEntity*>(reg->m_cmdGrid->FindGruntAt(
            obj->m_screenX,
            obj->m_screenY,
            &obj->m_area,
            &outA,
            &outB,
            reinterpret_cast<RECT*>(rect)
        ));
        if (ent != 0 && ent->m_258 != 0x38) {
            if (g_gameReg->m_134 != 1 || outA != 0) {
                if (this->HitTest(outA, outB) == 0) {
                    return 0;
                }
            }
        }
    }

    i64 legElapsed = static_cast<i64>(static_cast<u32>(g_frameTime)) - m_legDeadline;
    if (legElapsed >= m_legWindow) {
        CWwdGameObjectA* o = m_object;
        o->m_drawActive = 1;
        o->m_drawFillCmd = 7;
        o->m_drawFillArg = reinterpret_cast<i32>(g_gameReg->m_logicPump->m_tables[5]); // [m_78 + 0x28]
        this->BeginLeg();
        m_prevAnimSetNode = m_objAux->m_1c;
        m_objAux->m_1c = g_buteTree.Find("A");
        m_strikeArmed = 0;
    }
    return 0;
}

// (0xb4640 - CRainCloud::HitTest, the slot-20 strike-arm override - moved to
// its owner RainCloud.cpp.)

RVA(0x000b47a0, 0x27)
i32 CPathHazard::Arrive() {
    i32 next = m_wpIndex + 1;
    m_wpIndex = next;
    if (next >= m_wpCount) {
        m_wpIndex = 0;
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
    CWwdGameObjectA* obj = m_object;
    i32 idx = m_wpIndex;
    i32 wx = m_wp[idx].x;
    m_wpX = wx;
    i32 wy = m_wp[idx].y;
    m_wpY = wy;

    double dx = static_cast<double>(m_wpX) - static_cast<double>(obj->m_screenX);
    double dy = static_cast<double>(m_wpY) - static_cast<double>(obj->m_screenY);
    double len = sqrt(dx * dx + dy * dy);
    double ux = dx / len;
    double uy = dy / len;

    m_speed = g_pathOne / (static_cast<double>(obj->m_7c->m_bc) * g_pathTimeScale);
    m_posX = static_cast<double>(obj->m_screenX);
    m_posY = static_cast<double>(obj->m_screenY);
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

RVA(0x000b5070, 0x5)
void CPathHazard::ForwardTick() {
    Tick(); // virtual slot 16 (+0x40); tail-jump `mov eax,[ecx]; jmp [eax+0x40]`
}

#include <rva.h>
VTBL(CPathHazard, 0x001e7394); // vtable_names -> code (RTTI game class)
