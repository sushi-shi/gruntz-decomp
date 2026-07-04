// MovingLogic.cpp - CMovingLogic : CUserLogic, the moving-object motion-state
// logic base (parent of CProjectile). One dev TU, formerly split across
// MovingLogic{Ctor,Dtor,Update,Serial}.cpp. Methods in ascending retail-RVA
// order:
//   CMovingLogic::CMovingLogic       @0x013940 - standalone out-of-line ctor (COMDAT)
//   CMovingLogic::~CMovingLogic      @0x013bd0 - /GX leaf dtor (also the vftable anchor)
//   WriteCurve                       @0x16cdd0 - the bute-text curve writer
//   CMovingLogicBase::Serialize      @0x16e7f0 - the base-class bute round-trip
//   CMovingLogic::Update             @0x16ea90 - the per-frame scroll/position pump
//   CMovingLogic::Serialize          @0x16f4a0 - the derived bute round-trip
//
// All /GX (eh profile): the ctor's throwing CUserBaseLink base + the dtor's
// destructible +0x18 link force the EH frame; Update / WriteCurve / the two
// Serialize round-trips carry no destructible C++ object so /GX is a no-op for
// them (byte-verified under eh). Merged per docs/tu-topology-plan.md (Phase 1);
// the ctor 0x13940 (a standalone COMDAT copy) + the dtor 0x13bd0 (COMDAT-pooled)
// land in the game-code band while the run methods sit in the engine band - a
// COMDAT-placement artifact, not a separate dev TU.
//
// CMOVINGLOGIC_STANDALONE_CTOR drops <Gruntz/MovingLogic.h>'s inline no-arg ctor
// so this TU hangs the byte-exact out-of-line COMDAT copy (0x13940) instead - the
// engine emits both an inlined copy (folded into leaves) and this standalone.
#define CMOVINGLOGIC_STANDALONE_CTOR
#include <Gruntz/MovingLogic.h>
#include <Gruntz/MovingLogicSerial.h> // CButeText/CMovingLogicBase + the serialize helpers
#include <Globals.h>                  // Update: g_5f04f0 / g_motionNegHalf / g_645588
#include <rva.h>

// The standalone ctor. The +0x38..+0x10c motion ints are zeroed in retail's
// scheduled "column" order (all .a/low fields, then all .b/high fields), then the
// twelve coordinate bounds are seeded to the default [MIN,MAX] box.
// @early-stop
// 98.4% (vptr-stamp scheduling artifact, 6 residual bytes = ONE instruction):
// prologue, the /GX EH frame, the byte-exact field-zero "column" schedule, the
// min/max fan-out and the epilogue all match. Retail keeps the intermediate
// CUserLogic vptr stamp in the post-link slot and stamps the most-derived
// CMovingLogic vptr LATE (after the field init); cl here instead MOVES the
// CMovingLogic stamp into that early slot (DSE'ing the CUserLogic stamp) and emits
// no late stamp - because the min/max double loads clobber ecx, so cl cannot hoist
// the SEH-restore load into the post-link slot the way the byte-exact CPathHazard
// no-arg ctor (0x13170, no double init -> ecx free) does. A non-steerable cl
// scheduling/DSE micro-decision; logic complete. Deferred to the final sweep.
RVA(0x00013940, 0x1e1)
CMovingLogic::CMovingLogic() {
    m_78 = 0;
    m_80 = 0;
    m_88 = 0;
    m_60 = 0;
    m_68 = 0;
    m_70 = 0;
    m_48 = 0;
    m_50 = 0;
    m_58 = 0;
    m_38 = 0; // inherited CUserLogic field (re-zeroed in the no-arg path)
    m_40 = 0;
    m_f8 = 0;
    m_100 = 0;
    m_108 = 0;
    m_7c = 0;
    m_84 = 0;
    m_8c = 0;
    m_64 = 0;
    m_6c = 0;
    m_74 = 0;
    m_4c = 0;
    m_54 = 0;
    m_5c = 0;
    m_3c = 0; // inherited CUserLogic field
    m_44 = 0;
    m_fc = 0;
    m_104 = 0;
    m_10c = 0;
    m_f0 = 0;
    m_a8 = g_movingLogicMin;
    m_c0 = g_movingLogicMax;
    m_b0 = g_movingLogicMin;
    m_c8 = g_movingLogicMax;
    m_b8 = g_movingLogicMin;
    m_d0 = g_movingLogicMax;
    m_110 = g_movingLogicMax;
    m_118 = g_movingLogicMax;
    m_120 = g_movingLogicMax;
    m_128 = g_movingLogicMax;
    m_130 = g_movingLogicMax;
    m_138 = g_movingLogicMax;
}

// CMovingLogic::~CMovingLogic @0x00013bd0 - the most-derived vptr store is
// dead-eliminated at /O2, so the dtor folds the bare CUserLogic teardown: store
// the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the embedded
// ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The destructible
// link forces the /GX EH frame.
RVA(0x00013bd0, 0x44)
CMovingLogic::~CMovingLogic() {}

// 0x16cdd0 - WriteCurve(accum, curve): 29 doubles + 1 int, returning accum.
RVA(0x0016cdd0, 0x22f)
CButeText& WriteCurve(CButeText& accum, const CMotionState& c) {
    accum.AppendDouble(c.m_00);
    accum.AppendDouble(c.m_08);
    accum.AppendDouble(c.m_10);
    accum.AppendDouble(c.m_18);
    accum.AppendDouble(c.m_20);
    accum.AppendDouble(c.m_28);
    accum.AppendDouble(c.m_30);
    accum.AppendDouble(c.m_38);
    accum.AppendDouble(c.m_40);
    accum.AppendDouble(c.m_48);
    accum.AppendDouble(c.m_50);
    accum.AppendDouble(c.m_70);
    accum.AppendDouble(c.m_78);
    accum.AppendDouble(c.m_80);
    accum.AppendDouble(c.m_88);
    accum.AppendDouble(c.m_90);
    accum.AppendDouble(c.m_98);
    accum.AppendDouble(c.m_a0);
    accum.AppendDouble(c.m_a8);
    accum.AppendDouble(c.m_b0);
    accum.AppendInt(c.m_b8);
    accum.AppendDouble(c.m_c0);
    accum.AppendDouble(c.m_c8);
    accum.AppendDouble(c.m_d0);
    accum.AppendDouble(c.m_d8);
    accum.AppendDouble(c.m_e0);
    accum.AppendDouble(c.m_e8);
    accum.AppendDouble(c.m_f0);
    accum.AppendDouble(c.m_f8);
    accum.AppendDouble(c.m_100);
    return accum;
}

// ---------------------------------------------------------------------------
// 0x16e7f0 - CMovingLogicBase::Serialize(arc, mode, a3, a4): the base-class
// round-trip 0x16f4a0 chains to. mode 4 = write (append the name text, length-
// prefix it, write the three trailing ints + g_logicTypesRegistered); mode 7 =
// read (RezAlloc + read the text, parse the name back, read the four ints, then
// seed the back-pointers/m_14 from the context arg and stamp m_28=0x3e9).
// @early-stop
// Same virtual-base accumulator-temp wall as the derived Serialize 0x16f4a0
// (sibling, 66.6%): CButeWriteTemp/CButeReadTemp construct at the full object but
// the destruct helpers + the inlined text-length probe act on the vbase subobject
// (the `mov ecx,[esp+ecx+0x14]` vbase-offset lookup), which is not reproducible
// from a source-level temp. The read/write dispatch, the archive Read/Write
// virtual calls, RezAlloc/RezFree, WriteName/ReadName, the three trailing-int +
// g_logicTypesRegistered transfers and the read-mode back-pointer seeding are all
// byte-faithful. Logic complete; deferred to the final sweep with its sibling.
RVA(0x0016e7f0, 0x1cf)
i32 CMovingLogicBase::Serialize(CSerialArchive* arc, i32 mode, i32 a3, i32 a4) {
    if (arc == 0) {
        return 0;
    }
    if (mode == 4) {
        // WRITE: render the name to bute text, length-prefix it, append ints.
        char buf[0x100];
        CButeWriteTemp accum;
        accum.Ctor(buf, 0x100, 2, 1);
        WriteName(&accum, &m_18);
        i32 len = accum.Length();
        arc->Write(&len, 4);
        arc->Write(accum.GetBuffer(), len);
        arc->Write(&m_28, 4);
        arc->Write(&m_2c, 4);
        arc->Write(&g_logicTypesRegistered, 4);
        arc->Write(&m_30, 4);
        accum.m_vbase.DtorWriteB();
        accum.m_vbase.FuncB();
    } else if (mode == 7) {
        // READ: pull the length-prefixed text, parse the name back, read the ints.
        i32 len;
        arc->Read(&len, 4);
        void* buf = RezAlloc(len);
        arc->Read(buf, len);
        CButeReadTemp accum;
        accum.Ctor(buf, len, 1);
        ReadName(&accum, &m_18);
        RezFree(buf);
        arc->Read(&m_28, 4);
        arc->Read(&m_2c, 4);
        arc->Read(&g_logicTypesRegistered, 4);
        arc->Read(&m_30, 4);
        m_c = (void*)a4;
        m_10 = (void*)a4;
        m_14 = ((CMlSerialCtx*)a4)->m_7c;
        m_4 = 0;
        m_8 = 0;
        m_28 = 0x3e9;
        accum.m_vbase.DtorReadA();
        accum.m_vbase.FuncB();
    }
    return 1;
}

// The ms->units scale the elapsed-clock delta is multiplied by (0x5f04f0, a
// read-only .rdata double read via `fmul [mem]`); sits just before g_motionNegHalf.
// g_motionNegHalf (0x5f04f8, -0.5) comes via MotionState.h (canonical include).
// The running game clock g_645588 comes via <Gruntz/MovingLogic.h>.

// The lazily-built per-object worker at CGameObject+0x98 (the CAnimWorker the
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

// CMovingLogic is the shared canonical (<Gruntz/MovingLogic.h>): its +0x38
// CMotionState band is reached through Motion(); m_10 is the bound CGameObject,
// viewed here as the richer MlBoundObject (a scoped CGameObject reduced view - the
// engine-object consolidation, not CMovingLogic's - so its scroll/level fields are
// reachable). m_140/m_144/m_148/m_14c are the base's trailing ints.

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
    m_140 = (i32)Motion()->m_40;
    m_144 = (i32)Motion()->m_48;
    Motion()->Step((double)g_645588 * g_5f04f0 - Motion()->m_00);

    // Worker-driven scroll: fold the per-frame scroll deltas into the object's
    // screen position and re-seed the motion targets.
    if ((((MlBoundObject*)m_object)->m_8 & 0x10) && ((MlBoundObject*)m_object)->m_98 != 0) {
        ((MlBoundObject*)m_object)->m_5c += ((MlBoundObject*)m_object)->m_98->m_174;
        Motion()->m_40 = (double)((MlBoundObject*)m_object)->m_5c;
        ((MlBoundObject*)m_object)->m_60 += ((MlBoundObject*)m_object)->m_98->m_178;
        Motion()->m_48 = (double)((MlBoundObject*)m_object)->m_60;
    }

    // Clamp-scroll the level toward the new position.
    if (((MlBoundObject*)m_object)->m_e4 == 1) {
        m_148 = ((MlBoundObject*)m_object)
                    ->m_c->m_24->ClampScroll(
                        m_object,
                        (i32)Motion()->m_40,
                        ((MlBoundObject*)m_object)->m_60,
                        m_14c
                    );
        Motion()->m_30 = 0.0;
    } else {
        ((MlBoundObject*)m_object)->m_8 &= ~0x10;
        m_148 =
            ((MlBoundObject*)m_object)
                ->m_c->m_24->ClampScroll(m_object, (i32)Motion()->m_40, (i32)Motion()->m_48, m_14c);
    }

    // X arrival: if the object moved off the motion target, re-solve the X
    // arrival velocity and re-anchor the target.
    CMotionState* ms = Motion();
    i32 sx = ((MlBoundObject*)m_object)->m_5c;
    if ((i32)Motion()->m_40 != sx) {
        double d = (double)sx;
        ms->m_28 = ms->ArrivalVelX(d);
        double a0new = ms->m_a0 - (ms->m_40 - d);
        ms->m_40 = d;
        ms->m_a0 = a0new;
    }

    // Y arrival (symmetric).
    i32 sy = ((MlBoundObject*)m_object)->m_60;
    if ((i32)Motion()->m_48 != sy) {
        double d = (double)sy;
        ms->m_30 = ms->ArrivalVelY(d);
        double a8new = ms->m_a8 - (ms->m_48 - d);
        ms->m_48 = d;
        ms->m_a8 = a8new;
    }

    // Per-mode velocity fix-ups keyed off the ClampScroll result flags.
    if (((MlBoundObject*)m_object)->m_e4 != 7) {
        i32 f = m_148;
        if (f & 0x800000) {
            Motion()->m_30 = -Motion()->m_30;
            return;
        }
        if (f & 0x40000) {
            Motion()->m_88 = (double)m_140;
            Motion()->m_28 = Motion()->m_28 * g_motionNegHalf;
            return;
        }
        if (f & 0x80000) {
            Motion()->m_70 = (double)m_140;
            Motion()->m_28 = Motion()->m_28 * g_motionNegHalf;
        }
    }
}

// ---------------------------------------------------------------------------
// 0x16f4a0 - CMovingLogic::Serialize(arc, mode, a3, a4): bute-text round-trip of
// the curve block, then chain to the base. mode 4 = write (build text via
// WriteCurve, length-prefix it, write the four trailing ints), mode 7 = read
// (RezAlloc + read the text, ReadCurve it back, read the four ints). The CButeText
// accumulator is a virtual-base CButeMgr temp: the ctor takes the full object but
// the teardown helpers run on the vbase subobject (+0xc).
//
// @early-stop
// Virtual-base temp wall (docs/patterns family eh-dtor-inline-member-vtable-stamp-
// thisadjust): the accumulator's most-derived ctor (0x169700 read / 0x1698c0 write)
// constructs at the full object while the destruct helpers (0x1697c0/0x1699c0 then
// the shared 0x169d70) act on the vbase subobject at +0xc, and the write-path text
// length is computed by an inlined CString probe. These vbase this-adjusts + the
// inlined probe are not reproducible from a source-level temp; the read/write
// dispatch, the archive Read/Write virtual calls, RezAlloc/RezFree, WriteCurve
// (paired 100%) / ReadCurve, the four trailing-int transfers and the base chain
// are all byte-faithful. Logic complete; deferred to the final sweep.
RVA(0x0016f4a0, 0x1da)
i32 CMovingLogic::Serialize(CSerialArchive* arc, i32 mode, i32 a3, i32 a4) {
    if (arc == 0) {
        return 0;
    }
    if (mode == 4) {
        // WRITE: render the curve to bute text, length-prefix it, append ints.
        char buf[0x100];
        CButeWriteTemp accum;
        accum.Ctor(buf, 0x100, 2, 1);
        WriteCurve(*(CButeText*)&accum, *Motion());
        i32 len = accum.Length();
        arc->Write(&len, 4);
        arc->Write(accum.GetBuffer(), len);
        arc->Write(&m_140, 4);
        arc->Write(&m_144, 4);
        arc->Write(&m_148, 4);
        arc->Write(&m_14c, 4);
        accum.m_vbase.DtorWriteB();
        accum.m_vbase.FuncB();
    } else if (mode == 7) {
        // READ: pull the length-prefixed text, parse it back, read the ints.
        i32 len;
        arc->Read(&len, 4);
        void* buf = RezAlloc(len);
        arc->Read(buf, len);
        CButeReadTemp accum;
        accum.Ctor(buf, len, 1);
        ReadCurve(*(CButeText*)&accum, *Motion());
        RezFree(buf);
        arc->Read(&m_140, 4);
        arc->Read(&m_144, 4);
        arc->Read(&m_148, 4);
        arc->Read(&m_14c, 4);
        accum.m_vbase.DtorReadA();
        accum.m_vbase.FuncB();
    }
    return ((CMovingLogicBase*)this)->Serialize(arc, mode, a3, a4) != 0;
}

SIZE_UNKNOWN(MlBoundObject);
SIZE_UNKNOWN(MlHolder);
SIZE_UNKNOWN(MlLevel);
SIZE_UNKNOWN(MlScrollWorker);
SIZE_UNKNOWN(CButeReadTemp);
SIZE_UNKNOWN(CButeText);
SIZE_UNKNOWN(CButeVbaseTeardown);
SIZE_UNKNOWN(CButeWriteTemp);
SIZE_UNKNOWN(CMlSerialCtx);
SIZE_UNKNOWN(CMovingLogicBase);
