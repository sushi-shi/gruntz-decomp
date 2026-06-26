// CMovingLogicSerial.h - the bute-text serialization helpers of CMovingLogic
// (RTTI .?AVCMovingLogic@@; C:\Proj\Gruntz). CMovingLogic carries a fixed
// 0x108-byte coefficient block at this+0x38 (a movement curve: 29 doubles in
// three runs + one int), which the class's bute Serialize (vtbl slot @0x16f4a0)
// streams field-by-field into a CButeText accumulator.
//
// Owner recovered by RTTI: the writer 0x16cdd0 is called only from the
// CMovingLogic vtable slot at 0x16f4a0 (vtable 0x5e87ac -> COL 0x5f3d08 ->
// .?AVCMovingLogic@@). Field names are placeholders; only offsets + emitted
// bytes are load-bearing (campaign doctrine).
#ifndef GRUNTZ_CMOVINGLOGICSERIAL_H
#define GRUNTZ_CMOVINGLOGICSERIAL_H

#include <Ints.h>
#include <rva.h>

// The bute-text accumulator (an MFC-CString-backed value text). Its formatted
// appends are reloc-masked engine externs (no body): AppendDouble (0x191df0)
// and AppendInt (0x191d20) each format one value and return the accumulator.
class CButeText {
public:
    CButeText& AppendInt(i32 v);       // 0x191d20
    CButeText& AppendDouble(double v); // 0x191df0
};

// CMovingLogic's movement-curve coefficient block (CMovingLogic+0x38). Written
// runs: [0x00..0x50] 11 doubles, [0x70..0xb0] 9 doubles, the int at 0xb8, then
// [0xc0..0x100] 9 doubles. The 0x58/0x60/0x68 doubles and the 0xbc int are part
// of the block but not streamed.
struct CMovingLogicCurve {
    double m_00, m_08, m_10, m_18, m_20, m_28, m_30, m_38, m_40, m_48, m_50;
    double m_58, m_60, m_68; // not streamed
    double m_70, m_78, m_80, m_88, m_90, m_98, m_a0, m_a8, m_b0;
    i32 m_b8;
    i32 m_bc; // not streamed
    double m_c0, m_c8, m_d0, m_d8, m_e0, m_e8, m_f0, m_f8, m_100;
};

#endif // GRUNTZ_CMOVINGLOGICSERIAL_H
