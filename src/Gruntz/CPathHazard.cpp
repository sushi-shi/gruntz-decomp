// CPathHazard.cpp - the path-following hazard game object (C:\Proj\Gruntz).
//
// Four trace-discovered CPathHazard methods, defined in ascending retail-RVA
// order:
//   ~CPathHazard   @0x013340 - the /GX leaf dtor (folds the CUserLogic teardown).
//   Tick          @0x0b4020 - the per-frame movement-integrator driver (vslot 16).
//   BeginLeg      @0x0b47e0 - seed the unit vector toward the waypoint (vslot 19).
//   ForwardTick   @0x0b5070 - a thin non-virtual forwarder to vslot 16 (Tick).
//
// CPathHazard : CUserLogic (the base hierarchy comes from <Gruntz/UserLogic.h>);
// the hazard reads the bound CGameObject (m_10/m_38) as CPathObj. Only offsets /
// code bytes are load-bearing; names are placeholders for the recovered engine
// identities.
#include <Gruntz/CPathHazard.h>

// sqrt inlines to fsqrt at /O2; the (int)double casts lower to __ftol.
extern "C" double sqrt(double);

// The typed view of `this`'s vtable the Tick driver / ForwardTick dispatch
// through. The slots are the hazard's own __thiscall virtuals invoked indirectly
// (`mov ecx,this; call [edx+NN]`): slot 16 (+0x40) Tick (ForwardTick's tail
// target), slot 18 (+0x48) the "arrived" handler, slot 19 (+0x4c) BeginLeg, slot
// 20 (+0x50) the per-frame hit test. Each slot is a pointer-to-member-function of
// CPathHazard (single inheritance -> 4-byte code pointer; CPathHazard is complete
// in the header above, pmf-complete-class-4byte), so `(this->*slot)(...)` emits
// the `mov ecx,this; call [slot]` virtual-dispatch shape with no cast.
typedef void (CPathHazard::*PathArriveFn)();
typedef i32 (CPathHazard::*PathBeginFn)();
typedef i32 (CPathHazard::*PathHitFn)(i32, i32);
struct CPathHazardVtbl {
    char s0[0x40];
    PathArriveFn Tick; // +0x40  slot 16
    char s44[0x48 - 0x44];
    PathArriveFn Arrive; // +0x48  slot 18
    PathBeginFn BeginLeg; // +0x4c  slot 19 (== 0xb47e0)
    PathHitFn HitTest;    // +0x50  slot 20
};

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
class CLightningHazard : public CUserLogic {
public:
    i32 SiblingTick();         // 0x0b43f0 (virtual slot 16 override)
    i32 ArmStrike(i32, i32);   // 0x0b4640 (arm the strike window + kill cue)
    ~CLightningHazard(); // 0x013280 (folds the CUserLogic teardown)

    char m_pad40[0x108 - 0x40];
    i64 m_108; // +0x108 leg deadline (i64: m_108/m_10c)
    i64 m_110; // +0x110 leg window   (i64: m_110/m_114)
    i32 m_118; // +0x118 strike-armed gate
    char m_pad11c[0x120 - 0x11c];
    i64 m_120; // +0x120 strike deadline (i64)
    i64 m_128; // +0x128 strike window  (i64)
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
extern i32 g_strikeClock; // 0x645588 (the draw-clock; also g_pathLegTag)
DATA(0x00245598)
extern i32 g_strikeThresh; // 0x645598 (compared to 0x64)

// g_gameReg's +0x78 ref-index table (the strike sprite-frame selectors) and the
// +0x118/+0x134 window-mode gates, plus the bound-object's +0x144 query rect.
struct CLightObj {
    char m_pad00[0x4c];
    i32 m_4c; // +0x4c  sprite-ref handle
    i32 m_50; // +0x50  state (set to 7)
    char m_pad54[0x58 - 0x54];
    i32 m_58; // +0x58  active flag (set to 1)
    i32 m_5c, m_60; // +0x5c/+0x60 screen position
    char m_pad64[0x144 - 0x64];
    i32 m_144; // +0x144 query rect base
    char m_pad148[0x198 - 0x148];
    CPathLayer* m_198; // +0x198 layer descriptor
};
// The positional-sound emitter ArmStrike's "LEVEL_CLOUDHAZARDKILL" cue resolves
// to: m_10 the play host (Play 0x1360d0), m_14 the last-play clock, m_18 the
// cooldown interval (an unsigned `(now - m_14) >= m_18` gate).
struct CSndPlayHost {
    void Play(i32 tag, i32 a, i32 b, i32 c); // FUN_001360d0 __thiscall
};
struct CSndEmitter {
    char m_pad00[0x10];
    CSndPlayHost* m_10; // +0x10
    u32 m_14;           // +0x14 last-play clock
    u32 m_18;           // +0x18 cooldown interval
};
// The finder embedded at CSndHost+0x10 (out-param Find, 0x1b8438), the gate at +0x30.
struct CSndFinder {
    void Find(char* szName, CSndEmitter** out); // FUN_001b8438 __thiscall
};
struct CSndHost { // reg->m_30->m_28
    char m_pad00[0x10];
    CSndFinder m_10;          // +0x10 embedded
    char m_pad11[0x30 - 0x11]; // -> +0x30
    i32 m_30;                 // +0x30 gate (must be 0 to emit)
};
struct CSndSubMgr { // reg->m_30
    char m_pad00[0x28];
    CSndHost* m_28; // +0x28
};

struct CLightGameReg {
    char m_pad00[0x30];
    CSndSubMgr* m_30; // +0x30 sound sub-mgr (positional-sound emit)
    char m_pad34[0x68 - 0x34];
    CPathCueGate* m_68; // +0x68 visibility/cue gate
    char m_pad6c[0x78 - 0x6c];
    i32* m_78; // +0x78 ref-index/selector table
    char m_pad7c[0x118 - 0x7c];
    i32 m_118; // +0x118 has-window flag
    char m_pad11c[0x134 - 0x11c];
    i32 m_134; // +0x134 mode discriminator
    char m_pad138[0x13c - 0x138];
    i32 m_13c; // +0x13c visible-rect left
    i32 m_140; // +0x140 visible-rect top
    i32 m_144; // +0x144 visible-rect right
    i32 m_148; // +0x148 visible-rect bottom
};
DATA(0x0024556c)
extern CLightGameReg* g_lightGameReg;

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

// CPathHazard::~CPathHazard @0x013340 - the leaf adds no destructible members
// beyond CUserLogic, so its dtor folds the bare CUserLogic teardown: store the
// CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the embedded
// ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The destructible
// link forces the /GX EH frame. Byte-identical in shape to ~CTimeBomb
// (0x012a70) / ~CKitchenSlime (0x013100); the empty body is enough for cl.
RVA(0x00013340, 0x44)
CPathHazard::~CPathHazard() {}

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

    CPathObj* obj = (CPathObj*)m_10;
    // The probe rect (a 4-int local) the on-screen query tests, computed
    // unconditionally: {left, top, right, bottom} around the bound object's
    // screen position, inset by the layer base (m_198->m_18/m_1c, re-read each
    // component as retail does) and a 7px margin.
    i32 rect[4];
    rect[0] = obj->m_5c - obj->m_198->m_18 + 7;
    rect[2] = obj->m_198->m_18 + obj->m_5c - 7;
    rect[1] = obj->m_60 - obj->m_198->m_1c + 7;
    rect[3] = obj->m_198->m_1c + obj->m_60 - 7;

    CPathGameReg* reg = g_pathGameReg;
    if (reg->m_118 == 0 || reg->m_134 != 1) {
        i32 outA, outB;
        CPathEntity* ent =
            (CPathEntity*)reg->m_68->QueryAt(obj->m_5c, obj->m_60, &obj->m_144, &outA, &outB, rect);
        if (ent != 0 && ent->m_258 != 0x38) {
            if (g_pathGameReg->m_134 != 1 || outA != 0) {
                CPathHazardVtbl* vt = *(CPathHazardVtbl**)this;
                if ((this->*(vt->HitTest))(outA, outB) == 0) {
                    return 0;
                }
            }
        }
    }

    CPathObj* m10 = (CPathObj*)m_10;
    i32 wx = m_fc;
    if (m10->m_5c == wx) {
        i32 wy = m_100;
        if (m10->m_60 == wy) {
            // Arrived at the waypoint tile.
            m_60 = (double)wx;
            m_68 = (double)wy;
            CPathHazardVtbl* vt = *(CPathHazardVtbl**)this;
            (this->*(vt->Arrive))();
            i32 segs = ((CPathObj*)m_10)->m_120;
            if (segs > 0) {
                m_110 = segs;
                m_114 = 0;
                m_108 = g_pathLegTag;
                m_10c = 0;
                m_30 = (void*)m_14->m_1c;
                m_14->m_1c = g_buteTree.Find(g_iconBute);
                return 0;
            }
            (this->*(vt->BeginLeg))();
            return 0;
        }
    }

    // Not arrived: integrate the sub-pixel movement vector toward the waypoint.
    double step = (double)(i64)(u64)(u32)g_pathStepSeed * m_58;
    m_60 = m_60 + step * m_70;
    m_68 = m_68 + (double)(u32)g_pathStepSeed * m_78 * m_58;
    i32 newX = (i32)(m_80 + m_60);
    i32 newY = (i32)(m_88 + m_68);

    if (m_70 > g_pathZero) {
        if (newX > m_fc) {
            newX = m_fc;
        }
    } else if (m_70 < g_pathZero) {
        if (newX < m_fc) {
            newX = m_fc;
        }
    }

    if (m_78 > g_pathZero) {
        if (newY > m_100) {
            newY = m_100;
        }
    } else if (m_78 < g_pathZero) {
        if (newY < m_100) {
            newY = m_100;
        }
    }

    ((CPathObj*)m_10)->m_5c = newX;
    ((CPathObj*)m_10)->m_60 = newY;
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
    if (m_118 != 0) {
        i32 sel = 5;
        i64 elapsed = (i64)(u32)g_strikeClock - m_120;
        if (elapsed >= m_128) {
            m_118 = 0;
        } else if (g_strikeThresh < 0x64) {
            sel = 0;
        }
        CLightObj* o = (CLightObj*)m_10;
        o->m_58 = 1;
        o->m_50 = 7;
        o->m_4c = g_lightGameReg->m_78[sel + 5]; // [m_78 + sel*4 + 0x14]
    }

    ((CPathSubMgr*)((char*)m_38 + 0x1a0))->Advance(g_pathTick);

    CLightObj* obj = (CLightObj*)m_10;
    i32 rect[4];
    rect[0] = obj->m_5c - obj->m_198->m_18 + 7;
    rect[2] = obj->m_198->m_18 + obj->m_5c - 7;
    rect[1] = obj->m_60 - obj->m_198->m_1c + 7;
    rect[3] = obj->m_198->m_1c + obj->m_60 - 7;

    CLightGameReg* reg = g_lightGameReg;
    if (reg->m_118 != 0 && reg->m_134 == 1) {
        // window mode, skip the query
    } else {
        i32 outA, outB;
        CPathEntity* ent =
            (CPathEntity*)reg->m_68->QueryAt(obj->m_5c, obj->m_60, &obj->m_144, &outA, &outB, rect);
        if (ent != 0 && ent->m_258 != 0x38) {
            if (g_lightGameReg->m_134 != 1 || outA != 0) {
                CLightVtbl* vt = *(CLightVtbl**)this;
                if ((this->*(vt->HitTest))(outA, outB) == 0) {
                    return 0;
                }
            }
        }
    }

    i64 legElapsed = (i64)(u32)g_strikeClock - m_108;
    if (legElapsed >= m_110) {
        CLightObj* o = (CLightObj*)m_10;
        o->m_58 = 1;
        o->m_50 = 7;
        o->m_4c = g_lightGameReg->m_78[0xa]; // [m_78 + 0x28]
        CLightVtbl* vt = *(CLightVtbl**)this;
        (this->*(vt->BeginLeg))();
        m_30 = (void*)m_14->m_1c;
        m_14->m_1c = g_buteTree.Find("A");
        m_118 = 0;
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
    m_118 = 1;
    m_128 = (i64)(u32)g_buteMgr.GetDwordDef("Hazardz", "RainCloudFlashTime", 0x7d0);
    m_120 = (i64)(u32)g_strikeClock;
    g_lightGameReg->m_68->Strike(a, b, 9, -1);

    CLightObj* obj = (CLightObj*)m_10;
    CLightGameReg* reg = g_lightGameReg;
    i32 y = obj->m_60;
    i32 x = obj->m_5c;
    if (x < reg->m_144 && x >= reg->m_13c && y < reg->m_148 && y >= reg->m_140) {
        CSndHost* host = reg->m_30->m_28;
        if (host->m_30 == 0) {
            CSndEmitter* out = 0;
            host->m_10.Find("LEVEL_CLOUDHAZARDKILL", &out);
            if (out != 0) {
                i32 enabled = g_sndEnabled;
                i32 tag = g_sndCueTag;
                if (enabled != 0) {
                    u32 now = g_killCueClock;
                    if ((u32)(now - out->m_14) >= out->m_18) {
                        out->m_14 = now;
                        out->m_10->Play(tag, 0, 0, 0);
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
    CPathObj* obj = (CPathObj*)m_10;
    i32 idx = m_f8;
    i32 wx = PATH_WAYPOINTS(this)[idx].x;
    m_fc = wx;
    i32 wy = PATH_WAYPOINTS(this)[idx].y;
    m_100 = wy;

    double dx = (double)m_fc - (double)obj->m_5c;
    double dy = (double)m_100 - (double)obj->m_60;
    double len = sqrt(dx * dx + dy * dy);
    double ux = dx / len;
    double uy = dy / len;

    m_58 = g_pathOne / ((double)obj->m_7c->m_bc * g_pathTimeScale);
    m_60 = (double)obj->m_5c;
    m_68 = (double)obj->m_60;
    m_70 = ux;
    m_78 = uy;

    if (ux > g_pathZero) {
        m_80 = 0.5;
    } else if (ux < g_pathZero) {
        m_80 = -0.5;
    } else {
        m_80 = 0.0;
    }

    if (uy > g_pathZero) {
        m_88 = 0.5;
    } else if (uy < g_pathZero) {
        m_88 = -0.5;
    } else {
        m_88 = 0.0;
    }
    return 1;
}

// CPathHazard::ForwardTick @0x0b5070 - a thin non-virtual forwarder that
// tail-jumps to the hazard's own virtual slot 16 (Tick): `mov eax,[ecx]; jmp
// [eax+0x40]`. The tail-call through the typed vtable emits the jmp directly.
RVA(0x000b5070, 0x5)
void CPathHazard::ForwardTick() {
    CPathHazardVtbl* vt = *(CPathHazardVtbl**)this;
    (this->*(vt->Tick))();
}
