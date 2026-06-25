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
