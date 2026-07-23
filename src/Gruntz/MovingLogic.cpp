#define CMOVINGLOGIC_STANDALONE_CTOR
#include <Gruntz/MovingLogic.h>
#include <Io/FileMem.h> // the serialize stream (CFileMemBase == the real CFileMemBase)
#include <strstrea.h>   // REAL CRT ostrstream/istrstream (the serialize accumulator temps)
#include <Gruntz/MovingLogicSerial.h> // the serialize helpers (WriteName/ReadName/ReadCurve)
#include <Gruntz/GameLevel.h>         // CGameLevel::MoveToward (the level hop in Update)
#include <DDrawMgr/DDrawSurfaceMgr.h> // m_object->m_0c (the world root; m_level hop)
#include <rva.h>
#include <Gruntz/MotionState.h> // ex Globals.h

DATA(0x001f04f8)
const double g_motionNegHalf = -0.5;

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

RVA(0x00013bb0, 0x4)
LogicTypeId CMovingLogic::GetTypeTag() {
    return LOGIC_NONE;
}

RVA(0x00013bd0, 0x44)
CMovingLogic::~CMovingLogic() {}

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
// TU-partition /GX wall (sibling 0x16f4a0 same): the accumulators are REAL CRT
// ostrstream/istrstream stack temps now (<strstrea.h>; the ctor+vbase-flag, the
// inlined pcount() vbdisp probe, str()->?str@strstreambuf and the scope-end
// ~ostrstream+~ios pair all byte-match retail under /GX-). The ONLY residue: this
// unit is compiled /GX (its claimed 0x139xx ctor/dtor band needs EH frames) while
// retail's 0x16cxxx-0x16fxxx band has NO EH frames - two retail TUs conflated in
// one unit, so cl wraps the temps in an EH frame retail lacks. Fix = split the
// movinglogic TU at the band boundary (docs/exe-map partition work), not source.
RVA(0x0016e7f0, 0x1cf)
i32 CUserLogic::SerializeMove(CFileMemBase* arc, i32 mode, i32 a3, i32 a4) {
    if (arc == 0) {
        return 0;
    }
    if (mode == 4) {
        // WRITE: render the name to bute text, length-prefix it, append ints.
        char buf[0x100];
        ostrstream accum(buf, 0x100); // ??0ostrstream(buf, 0x100, ios::out=2) + vbase flag
        WriteName(&accum, &m_link);
        i32 len = accum.pcount(); // the inlined vbase rdbuf()->out_waiting() probe
        arc->Write(&len, 4);
        arc->Write(accum.str(), len); // inline forward -> ?str@strstreambuf (0x1692b0)
        arc->Write(&m_28, 4);
        arc->Write(&m_2c, 4);
        arc->Write(&g_logicTypesRegistered, 4);
        arc->Write(&m_prevAnimSetNode, 4);
        // scope-end: cl emits ~ostrstream (0x1699c0) then the ~ios vbase (0x169d70)
    } else if (mode == 7) {
        // READ: pull the length-prefixed text, parse the name back, read the ints.
        i32 len;
        arc->Read(&len, 4);
        void* buf = RezAlloc(len);
        arc->Read(buf, len);
        istrstream accum(static_cast<char*>(buf), len); // ??0istrstream + vbase flag
        ReadName(&accum, &m_link);
        RezFree(buf);
        arc->Read(&m_28, 4);
        arc->Read(&m_2c, 4);
        arc->Read(&g_logicTypesRegistered, 4);
        arc->Read(&m_prevAnimSetNode, 4);
        m_0c = reinterpret_cast<CGameObject*>(a4);
        m_object = reinterpret_cast<CWwdGameObjectA*>(a4);
        m_objAux = (reinterpret_cast<CGameObject*>(a4))->m_7c;
        m_deferredCallback = 0;
        m_gatedCallback = 0;
        m_28 = 0x3e9;
        // scope-end: cl emits ~istrstream (0x1697c0) then the ~ios vbase (0x169d70)
    }
    return 1;
}

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
        m_148 = m_object->OwnerMgr()->m_level->MoveToward(
            m_object,
            static_cast<i32>(Motion()->m_40),
            m_object->m_screenY,
            m_14c
        );
        Motion()->m_30 = 0.0;
    } else {
        m_object->m_flags &= ~0x10;
        m_148 = m_object->OwnerMgr()->m_level->MoveToward(
            m_object,
            static_cast<i32>(Motion()->m_40),
            static_cast<i32>(Motion()->m_48),
            m_14c
        );
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
// (RezAlloc + read the text, ReadCurve it back, read the four ints). The
// accumulators are CRT ostrstream/istrstream stack temps (<strstrea.h>).
//
// @early-stop
// TU-partition /GX wall (see 0x16e7f0 above): real ostrstream/istrstream temps
// byte-match retail under /GX-; this unit builds /GX for its 0x139xx ctor/dtor
// band, so cl adds an EH frame retail lacks here. Split the TU to fix.
RVA(0x0016f4a0, 0x1da)
i32 CMovingLogic::SerializeMove(CFileMemBase* arc, i32 mode, i32 a3, i32 a4) {
    if (arc == 0) {
        return 0;
    }
    if (mode == 4) {
        // WRITE: render the curve to bute text, length-prefix it, append ints.
        char buf[0x100];
        ostrstream accum(buf, 0x100); // ??0ostrstream(buf, 0x100, ios::out=2) + vbase flag
        WriteCurve(accum, *Motion());
        i32 len = accum.pcount(); // the inlined vbase rdbuf()->out_waiting() probe
        arc->Write(&len, 4);
        arc->Write(accum.str(), len); // inline forward -> ?str@strstreambuf (0x1692b0)
        arc->Write(&m_140, 4);
        arc->Write(&m_144, 4);
        arc->Write(&m_148, 4);
        arc->Write(&m_14c, 4);
        // scope-end: cl emits ~ostrstream (0x1699c0) then the ~ios vbase (0x169d70)
    } else if (mode == 7) {
        // READ: pull the length-prefixed text, parse it back, read the ints.
        i32 len;
        arc->Read(&len, 4);
        void* buf = RezAlloc(len);
        arc->Read(buf, len);
        istrstream accum(static_cast<char*>(buf), len); // ??0istrstream + vbase flag
        ReadCurve(accum, *Motion());
        RezFree(buf);
        arc->Read(&m_140, 4);
        arc->Read(&m_144, 4);
        arc->Read(&m_148, 4);
        arc->Read(&m_14c, 4);
        // scope-end: cl emits ~istrstream (0x1697c0) then the ~ios vbase (0x169d70)
    }
    return CUserLogic::SerializeMove(arc, mode, a3, a4) != 0;
}
