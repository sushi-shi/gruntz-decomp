// MovingLogicUpdate.cpp - CMovingLogic::Update (RVA 0x16ea90), the per-frame
// scroll/position pump of the moving-object family (C:\Proj\Gruntz).
//
// This is the class's real 564-byte __thiscall per-tick update (this=ecx->esi):
// snapshot the integer X/Y positions, advance the embedded CMotionState (+0x38)
// by the elapsed clock delta, fold in any worker-driven scroll deltas from the
// bound CGameObject (+0x10), clamp-scroll the level toward the new position via
// CGameLevel::ClampScroll (0x15de40), re-solve the arrival velocities against the
// object's screen position (CMotionState::ArrivalVelX/Y), and finally apply the
// per-mode velocity fix-ups keyed off the ClampScroll result flags (m_148).
//
// The +0x38 subobject is the SAME CMotionState modeled in Gruntz/MotionState.h;
// it is reused here (methods Step/ArrivalVelX/ArrivalVelY reloc-mask; the double
// fields are read/written directly). Trailing ints m_140/m_144/m_148/m_14c live
// after the 0x108-byte motion block. Kept in a dedicated unit so it can reuse the
// method-carrying CMotionState without perturbing CMovingLogicSerial's plain
// CMovingLogicCurve view of the same +0x38 region.
#include <Gruntz/MotionState.h>
#include <rva.h>

// The ms->units scale the elapsed-clock delta is multiplied by (0x5f04f0, a
// read-only .rdata double read via `fmul [mem]`); sits just before g_motionNegHalf.
extern const double g_5f04f0;

// The shared -0.5 easing constant (0x5f04f8; owner Globals.cpp) used by the tail
// velocity fix-ups. Declared via MotionState.h (g_motionNegHalf).

// The running game clock (low 32 bits of the engine ms counter, 0x645588; owner
// Projectile.cpp). Read unsigned -> the (double) conversion is the {lo,0} fild.
extern "C" u32 g_645588;

// The lazily-built per-object worker at CGameObject+0x98 (the CSiriusWorker the
// UserLogic family builds); only its two per-frame scroll deltas are touched.
struct MlScrollWorker {
    char _00[0x174];
    i32 m_174; // +0x174  X scroll delta
    i32 m_178; // +0x178  Y scroll delta
};

// The level the bound object clamp-scrolls: reached as obj->m_c->m_24. Its
// per-frame scroll clamp is CGameLevel::ClampScroll (0x15de40); modeled here as a
// bodyless external so the direct call reloc-masks.
struct MlLevel {
    i32 ClampScroll(void* target, i32 a1, i32 a2, i32 a3); // 0x15de40
};
struct MlHolder {
    char _00[0x24];
    MlLevel* m_24; // +0x24
};

// The bound CGameObject at CMovingLogic+0x10 (only the fields this update reads).
struct MlBoundObject {
    char _00[0x08];
    i32 m_8;       // +0x08  flags (bit 0x10 = worker-scroll active)
    MlHolder* m_c; // +0x0c  level holder
    char _10[0x5c - 0x10];
    i32 m_5c; // +0x5c  screen X
    i32 m_60; // +0x60  screen Y
    char _64[0x98 - 0x64];
    MlScrollWorker* m_98; // +0x98  lazily-built scroll worker
    char _9c[0xe4 - 0x9c];
    i32 m_e4; // +0xe4  scroll mode (1 = follow, 7 = free)
};

// CMovingLogic: vptr + the CMotionState kinematic block at +0x38 + four trailing
// ints. Only the layout + Update are needed here.
class CMovingLogic {
public:
    void Update(); // 0x16ea90

    char _00[0x10];      // +0x00  vptr + spare
    MlBoundObject* m_10; // +0x10
    char _14[0x38 - 0x14];
    CMotionState m_38; // +0x38  (0x108 bytes -> ends at +0x140)
    i32 m_140;         // +0x140  latched (int)m_38.m_40
    i32 m_144;         // +0x144  latched (int)m_38.m_48
    i32 m_148;         // +0x148  ClampScroll result flags
    i32 m_14c;         // +0x14c  ClampScroll mode arg
};

// ---------------------------------------------------------------------------
// @early-stop
// Complete reconstruction (~92%). Logic + control flow + every member store are
// byte-faithful; the residual is three documented codegen walls verified by
// llvm-objdump -dr base vs target (0x16ea90):
//   1. regalloc (dominant): in the worker-scroll block MSVC pins the bound object
//      m_10 in a CALLEE-SAVED reg (edi) and carries it across into the ClampScroll
//      block (one live range), whereas retail re-fetches m_10 into SCRATCH (eax/edx)
//      per use and reloads a fresh edi in the ClampScroll block (two live ranges).
//      A reload-vs-reuse coin-flip; not source-steerable (using m_10-> directly
//      already yields the aliasing reloads; the reg CLASS choice is the allocator's).
//   2. x87 latency-fill: the two (int)m_38.m_40 / (int)m_38.m_48 snapshots let cl
//      hoist the second `fldl m_48` into the __ftol return-latency gap ahead of the
//      `mov m_140` store; retail keeps store-then-load. A scheduler fill, not steerable.
//   3. const-fld order: `m_28 *= g_motionNegHalf` emits `fld g_motionNegHalf; fmul
//      m_28` because g_motionNegHalf is a `const double` global (cl loads the const
//      operand first) vs retail `fld m_28; fmul g_motionNegHalf`; operand-swap in
//      source does not change it (docs/patterns/x87-fp-stack-schedule.md).
RVA(0x0016ea90, 0x234)
void CMovingLogic::Update() {
    // Snapshot the integer positions, then step the kinematic state by the
    // elapsed clock delta.
    m_140 = (i32)m_38.m_40;
    m_144 = (i32)m_38.m_48;
    m_38.Step((double)g_645588 * g_5f04f0 - m_38.m_00);

    // Worker-driven scroll: fold the per-frame scroll deltas into the object's
    // screen position and re-seed the motion targets.
    if ((m_10->m_8 & 0x10) && m_10->m_98 != 0) {
        m_10->m_5c += m_10->m_98->m_174;
        m_38.m_40 = (double)m_10->m_5c;
        m_10->m_60 += m_10->m_98->m_178;
        m_38.m_48 = (double)m_10->m_60;
    }

    // Clamp-scroll the level toward the new position.
    if (m_10->m_e4 == 1) {
        m_148 = m_10->m_c->m_24->ClampScroll(m_10, (i32)m_38.m_40, m_10->m_60, m_14c);
        m_38.m_30 = 0.0;
    } else {
        m_10->m_8 &= ~0x10;
        m_148 = m_10->m_c->m_24->ClampScroll(m_10, (i32)m_38.m_40, (i32)m_38.m_48, m_14c);
    }

    // X arrival: if the object moved off the motion target, re-solve the X
    // arrival velocity and re-anchor the target.
    CMotionState* ms = &m_38;
    i32 sx = m_10->m_5c;
    if ((i32)m_38.m_40 != sx) {
        double d = (double)sx;
        ms->m_28 = ms->ArrivalVelX(d);
        double a0new = ms->m_a0 - (ms->m_40 - d);
        ms->m_40 = d;
        ms->m_a0 = a0new;
    }

    // Y arrival (symmetric).
    i32 sy = m_10->m_60;
    if ((i32)m_38.m_48 != sy) {
        double d = (double)sy;
        ms->m_30 = ms->ArrivalVelY(d);
        double a8new = ms->m_a8 - (ms->m_48 - d);
        ms->m_48 = d;
        ms->m_a8 = a8new;
    }

    // Per-mode velocity fix-ups keyed off the ClampScroll result flags.
    if (m_10->m_e4 != 7) {
        i32 f = m_148;
        if (f & 0x800000) {
            m_38.m_30 = -m_38.m_30;
            return;
        }
        if (f & 0x40000) {
            m_38.m_88 = (double)m_140;
            m_38.m_28 = m_38.m_28 * g_motionNegHalf;
            return;
        }
        if (f & 0x80000) {
            m_38.m_70 = (double)m_140;
            m_38.m_28 = m_38.m_28 * g_motionNegHalf;
        }
    }
}

SIZE_UNKNOWN(MlBoundObject);
SIZE_UNKNOWN(MlHolder);
SIZE_UNKNOWN(MlLevel);
SIZE_UNKNOWN(MlScrollWorker);
