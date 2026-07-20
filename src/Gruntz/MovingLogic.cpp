// MovingLogic.cpp - CMovingLogic : CUserLogic, the moving-object motion-state
// logic base (parent of CProjectile). One dev TU, formerly split across
// MovingLogic{Ctor,Dtor,Update,Serial}.cpp. Methods in ascending retail-RVA
// order:
//   CMovingLogic::CMovingLogic       @0x013940 - standalone out-of-line ctor (COMDAT)
//   CMovingLogic::~CMovingLogic      @0x013bd0 - /GX leaf dtor (also the vftable anchor)
//   WriteCurve                       @0x16cdd0 - the bute-text curve writer
//   CUserLogic::SerializeMove        @0x16e7f0 - the base-class bute round-trip (slot 1)
//   CMovingLogic::MovingSlot16             @0x16ea90 - the per-frame scroll/position pump
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
#include <Io/FileMem.h> // the serialize stream (CSerialArchive == the real CFileMemBase)
#include <Gruntz/MovingLogicSerial.h> // CButeText accumulators + the serialize helpers
#include <Gruntz/GameLevel.h>         // CGameLevel::MoveToward (the level hop in Update)
#include <DDrawMgr/DDrawSurfaceMgr.h> // m_object->m_0c (the world root; m_level hop)
#include <Globals.h>                  // Update: g_motionTimeScale / g_motionNegHalf / g_frameTime
#include <rva.h>

// The per-tick time scale (owner-TU def; VA 0x5f04f0). Update: g_frameTime * g_motionTimeScale.
DATA(0x001f04f0)
const double g_motionTimeScale = 0.001; // 0x5f04f0

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
// @interleaver CMovingLogic - own-class out-of-line COMDAT (0x13xxx leaf-ctor pool, run
// methods sit in the engine band 0x16cdd0+); RVA-placement artifact per the header note.
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
    m_band38 = 0; // the CMotionState band's dword 0 (flat overlay zero-init)
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
    m_band3c = 0; // the band's dword 1
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

// (0x16be60 is NOT a game method: it is the CRT iostream insertion operator
// ostream::operator<<(const char*) - its callees are ostream::opfx (0x16bd10) /
// writepad (0x16c2d0) / osfx (0x16bd90), and its siblings ostream::operator<<(int)
// @0x191d20 / (double) @0x191df0 are library-carved. Carved to config/library_labels.csv
// (LIBCIMT, ??6ostream@@QAEAAV0@PBD@Z), not reconstructed - game-not-CRT policy.)

// 0x16cdd0 - WriteCurve(accum, curve): stream 29 doubles + 1 int into the ostream
// accumulator (the library ostream::operator<< overloads), returning accum.
RVA(0x0016cdd0, 0x22f)
ostream& WriteCurve(ostream& accum, const CMotionState& c) {
    accum << c.m_00;
    accum << c.m_08;
    accum << c.m_10;
    accum << c.m_18;
    accum << c.m_20;
    accum << c.m_28;
    accum << c.m_30;
    accum << c.m_38;
    accum << c.m_40;
    accum << c.m_48;
    accum << c.m_50;
    accum << c.m_70;
    accum << c.m_78;
    accum << c.m_80;
    accum << c.m_88;
    accum << c.m_90;
    accum << c.m_98;
    accum << c.m_a0;
    accum << c.m_a8;
    accum << c.m_b0;
    accum << c.m_b8; // int (?g_b8 is i32) -> operator<<(int) 0x191d20
    accum << c.m_c0;
    accum << c.m_c8;
    accum << c.m_d0;
    accum << c.m_d8;
    accum << c.m_e0;
    accum << c.m_e8;
    accum << c.m_f0;
    accum << c.m_f8;
    accum << c.m_100;
    return accum;
}

// ---------------------------------------------------------------------------
// 0x16e7f0 - CUserLogic::SerializeMove(arc, mode, a3, a4): the class's OWN vtable
// slot-1 override (vtbl 0x1e705c[1], `override` of CUserBase's slot 1 per RTTI;
// CGruntVoice : CUserLogic tags the same slot `inherited`, so CUserLogic defines
// it). Was bound here under the fake `CMovingLogicBase::Serialize` - see the
// dissolution note in <Gruntz/MovingLogicSerial.h>. The base-class round-trip
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
i32 CUserLogic::SerializeMove(CGruntArchive* arc, i32 mode, i32 a3, i32 a4) {
    if (arc == 0) {
        return 0;
    }
    if (mode == 4) {
        // WRITE: render the name to bute text, length-prefix it, append ints.
        char buf[0x100];
        CButeWriteTemp accum;
        accum.Ctor(buf, 0x100, 2, 1);
        WriteName(&accum, &m_link);
        i32 len = accum.Length();
        arc->Write(&len, 4);
        arc->Write(accum.GetBuffer(), len);
        arc->Write(&m_28, 4);
        arc->Write(&m_2c, 4);
        arc->Write(&g_logicTypesRegistered, 4);
        arc->Write(&m_prevAnimSetNode, 4);
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
        ReadName(&accum, &m_link);
        RezFree(buf);
        arc->Read(&m_28, 4);
        arc->Read(&m_2c, 4);
        arc->Read(&g_logicTypesRegistered, 4);
        arc->Read(&m_prevAnimSetNode, 4);
        m_0c = reinterpret_cast<CGameObject*>(a4);
        m_object = reinterpret_cast<CWwdGameObjectA*>(a4);
        m_objAux = (reinterpret_cast<CGameObject*>(a4))->m_7c;
        m_04 = 0;
        m_08 = 0;
        m_28 = 0x3e9;
        accum.m_vbase.DtorReadA();
        accum.m_vbase.FuncB();
    }
    return 1;
}

// The ms->units scale the elapsed-clock delta is multiplied by (0x5f04f0, a
// read-only .rdata double read via `fmul [mem]`); sits just before g_motionNegHalf.
// g_motionNegHalf (0x5f04f8, -0.5) comes via MotionState.h (canonical include).
// The running game clock g_frameTime comes via <Gruntz/MovingLogic.h>.

// The bound object is the canonical CGameObject (UserLogic.h, via MovingLogic.h):
// the former MlBoundObject/MlScrollWorker/MlHolder/MlLevel reduced views are
// COLLAPSED into it - m_flags bit4 = riding, m_carrier = the latched carrier
// object (its m_deltaX/m_deltaY are the per-frame ride deltas; CGameLevel::
// StepAxisAlt latches it), m_moveMode drives the level's DispatchMove. The level
// hop is m_object->m_0c (CDDrawSurfaceMgr) -> m_level -> MoveToward (0x15de40); the
// m_0c cast is language-forced (the base stores the owner as a generic i32).

// CMovingLogic is the shared canonical (<Gruntz/MovingLogic.h>): its +0x38
// CMotionState band is reached through Motion(); m_10 is the bound CGameObject.
// m_140/m_144/m_148/m_14c are the base's trailing ints.

// ---------------------------------------------------------------------------
// @early-stop
// Complete reconstruction (~92%). Logic + control flow + every member store are
// byte-faithful; the residual is three documented codegen walls verified by
// llvm-objdump -dr base vs target (0x16ea90):
//   1. regalloc (dominant): in the worker-scroll block MSVC pins the bound object
//      m_10 in a CALLEE-SAVED reg (edi) and carries it across into the MoveToward
//      block (one live range), whereas retail re-fetches m_10 into SCRATCH (eax/edx)
//      per use and reloads a fresh edi in the MoveToward block (two live ranges).
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
void CMovingLogic::MovingSlot16() {
    // Snapshot the integer positions, then step the kinematic state by the
    // elapsed clock delta.
    m_140 = static_cast<i32>(Motion()->m_40);
    m_144 = static_cast<i32>(Motion()->m_48);
    Motion()->Step(static_cast<double>(g_frameTime) * g_motionTimeScale - Motion()->m_00);

    // Carrier ride: while riding (flags bit4 + a latched carrier), fold the
    // carrier's per-frame deltas into the object's position and re-seed the
    // motion targets.
    if ((m_object->m_flags & 0x10) && m_object->m_carrier != 0) {
        m_object->m_screenX += m_object->m_carrier->m_deltaX;
        Motion()->m_40 = static_cast<double>(m_object->m_screenX);
        m_object->m_screenY += m_object->m_carrier->m_deltaY;
        Motion()->m_48 = static_cast<double>(m_object->m_screenY);
    }

    // Drive the level's move resolver toward the new position.
    if (m_object->m_moveMode == 1) {
        m_148 = m_object->OwnerMgr()->m_level
                    ->MoveToward(m_object, static_cast<i32>(Motion()->m_40), m_object->m_screenY, m_14c);
        Motion()->m_30 = 0.0;
    } else {
        m_object->m_flags &= ~0x10;
        m_148 = m_object->OwnerMgr()->m_level
                    ->MoveToward(m_object, static_cast<i32>(Motion()->m_40), static_cast<i32>(Motion()->m_48), m_14c);
    }

    // X arrival: if the object moved off the motion target, re-solve the X
    // arrival velocity and re-anchor the target.
    CMotionState* ms = Motion();
    i32 sx = m_object->m_screenX;
    if (static_cast<i32>(Motion()->m_40) != sx) {
        double d = static_cast<double>(sx);
        ms->m_28 = ms->ArrivalVelX(d);
        double a0new = ms->m_a0 - (ms->m_40 - d);
        ms->m_40 = d;
        ms->m_a0 = a0new;
    }

    // Y arrival (symmetric).
    i32 sy = m_object->m_screenY;
    if (static_cast<i32>(Motion()->m_48) != sy) {
        double d = static_cast<double>(sy);
        ms->m_30 = ms->ArrivalVelY(d);
        double a8new = ms->m_a8 - (ms->m_48 - d);
        ms->m_48 = d;
        ms->m_a8 = a8new;
    }

    // Per-mode velocity fix-ups keyed off the MoveToward result flags.
    if (m_object->m_moveMode != 7) {
        i32 f = m_148;
        if (f & 0x800000) {
            Motion()->m_30 = -Motion()->m_30;
            return;
        }
        if (f & 0x40000) {
            Motion()->m_88 = static_cast<double>(m_140);
            Motion()->m_28 = Motion()->m_28 * g_motionNegHalf;
            return;
        }
        if (f & 0x80000) {
            Motion()->m_70 = static_cast<double>(m_140);
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
i32 CMovingLogic::SerializeMove(CGruntArchive* arc, i32 mode, i32 a3, i32 a4) {
    if (arc == 0) {
        return 0;
    }
    if (mode == 4) {
        // WRITE: render the curve to bute text, length-prefix it, append ints.
        char buf[0x100];
        CButeWriteTemp accum;
        accum.Ctor(buf, 0x100, 2, 1);
        WriteCurve(*reinterpret_cast<ostream*>(&accum), *Motion());
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
        ReadCurve(*reinterpret_cast<istream*>(&accum), *Motion());
        RezFree(buf);
        arc->Read(&m_140, 4);
        arc->Read(&m_144, 4);
        arc->Read(&m_148, 4);
        arc->Read(&m_14c, 4);
        accum.m_vbase.DtorReadA();
        accum.m_vbase.FuncB();
    }
    return CUserLogic::SerializeMove(arc, mode, a3, a4) != 0;
}

SIZE_UNKNOWN(CButeReadTemp);
SIZE_UNKNOWN(CButeVbaseTeardown);
SIZE_UNKNOWN(CButeWriteTemp);
SIZE_UNKNOWN(ostream); // CRT ostream (declared-only; library operator<< appends)
