// MovingLogicSerial.cpp - CMovingLogic's bute-text curve writer (C:\Proj\Gruntz).
//
//   0x16cdd0  WriteCurve - stream the 0x108-byte movement-curve block
//                          (CMovingLogic+0x38) field-by-field into a CButeText
//                          accumulator, returning it. Driven by CMovingLogic's
//                          bute Serialize (vtbl slot 0x16f4a0) in the text-write
//                          mode. A flat __cdecl sequence of formatted appends;
//                          the AppendDouble/AppendInt callees are reloc-masked
//                          engine externs.
#include <Gruntz/MovingLogicSerial.h>

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

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
#include <rva.h>
SIZE_UNKNOWN(CButeReadTemp);
SIZE_UNKNOWN(CButeText);
SIZE_UNKNOWN(CButeVbaseTeardown);
SIZE_UNKNOWN(CButeWriteTemp);
SIZE_UNKNOWN(CMlSerialCtx);
SIZE_UNKNOWN(CMovingLogicBase);
