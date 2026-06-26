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
