// CMovingLogicSerial.cpp - CMovingLogic's bute-text curve writer (C:\Proj\Gruntz).
//
//   0x16cdd0  WriteCurve - stream the 0x108-byte movement-curve block
//                          (CMovingLogic+0x38) field-by-field into a CButeText
//                          accumulator, returning it. Driven by CMovingLogic's
//                          bute Serialize (vtbl slot 0x16f4a0) in the text-write
//                          mode. A flat __cdecl sequence of formatted appends;
//                          the AppendDouble/AppendInt callees are reloc-masked
//                          engine externs.
#include <Gruntz/CMovingLogicSerial.h>

// 0x16cdd0 - WriteCurve(accum, curve): 29 doubles + 1 int, returning accum.
RVA(0x0016cdd0, 0x22f)
CButeText& WriteCurve(CButeText& accum, const CMovingLogicCurve& c) {
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
i32 CMovingLogic::Serialize(CMlSerialArchive* arc, i32 mode, i32 a3, i32 a4) {
    if (arc == 0) {
        return 0;
    }
    if (mode == 4) {
        // WRITE: render the curve to bute text, length-prefix it, append ints.
        char buf[0x100];
        CButeWriteTemp accum;
        accum.Ctor(buf, 0x100, 2, 1);
        WriteCurve(*(CButeText*)&accum, m_38);
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
        ReadCurve(*(CButeText*)&accum, m_38);
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
