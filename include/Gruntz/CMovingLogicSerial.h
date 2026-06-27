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

// ---------------------------------------------------------------------------
// CMovingLogic::Serialize (0x16f4a0) support. The class persists its 0x108-byte
// curve block via a bute-text round-trip through a CButeText accumulator, then
// chains to its base Serialize. The accumulator is a virtual-base CButeMgr temp:
// the most-derived ctor takes the full object while the teardown helpers act on
// the vbase subobject (+0xc) - that vbase this-adjust is a documented MSVC5 wall,
// so the construct/teardown are modeled here as reloc-masked helper views.

// The serialize/archive stream (same shape as CSerialSub34's): a polymorphic
// CArchive-like whose vtable holds Read @ +0x2c and Write @ +0x30.
struct CMlSerialArchiveVtbl;
class CMlSerialArchive {
public:
    CMlSerialArchiveVtbl* vptr; // +0x00
    void Read(void* buf, i32 len);
    void Write(const void* buf, i32 len);
};
typedef void (CMlSerialArchive::*CMlSerialIoFn)(void*, i32);
struct CMlSerialArchiveVtbl {
    char _00[0x2c];
    CMlSerialIoFn Read;  // [0x2c]
    CMlSerialIoFn Write; // [0x30]
};
inline void CMlSerialArchive::Read(void* buf, i32 len) {
    (this->*(vptr->Read))(buf, len);
}
inline void CMlSerialArchive::Write(const void* buf, i32 len) {
    (this->*(vptr->Write))((void*)buf, len);
}

// RezAlloc/RezFree (0x1b9b46 / 0x1b9b82) - the engine block allocator.
void* RezAlloc(i32 size); // 0x1b9b46
void RezFree(void* p);    // 0x1b9b82

// ReadCurve (0x16d000): parse a CButeText accumulator back into a curve block.
void ReadCurve(CButeText& accum, CMovingLogicCurve& c); // 0x16d000

// The vbase-subobject teardown helpers (reloc-masked, the +0xc this used by the
// retail destruct path).
class CButeVbaseTeardown {
public:
    void DtorReadA();  // 0x1697c0  (read-temp derived teardown)
    void DtorWriteB(); // 0x1699c0 (write-temp derived teardown)
    void FuncB();      // 0x169d70 (shared vbase teardown)
};

// The two accumulator temps. Construct via their reloc-masked ctors (different
// signatures); the teardown helpers act on the vbase subobject at +0xc.
class CButeReadTemp {
public:
    void Ctor(void* buf, i32 len, i32 flag); // 0x169700
    char _00[0x0c];
    CButeVbaseTeardown m_vbase; // +0x0c
    char _10[0x130];
};
class CButeWriteTemp {
public:
    void Ctor(void* buf, i32 cap, i32 a, i32 b); // 0x1698c0
    i32 Length();                                // inlined buffer-length probe
    char* GetBuffer();                           // 0x1692b0
    char _00[0x0c];
    CButeVbaseTeardown m_vbase; // +0x0c
    char _10[0x130];
};

// CMovingLogic's base class providing the chained Serialize (0x16e7f0). Modeled
// as a standalone view; the derived Serialize forwards `this` unchanged.
class CMovingLogicBase {
public:
    i32 Serialize(CMlSerialArchive* arc, i32 mode, i32 a3, i32 a4); // 0x16e7f0
};

// CMovingLogic: vtable + the 0x108-byte curve at +0x38 + four trailing ints.
class CMovingLogic {
public:
    i32 Serialize(CMlSerialArchive* arc, i32 mode, i32 a3, i32 a4); // 0x16f4a0

    void* _vptr;            // +0x00
    char _04[0x34];         // +0x04
    CMovingLogicCurve m_38; // +0x38 (0x108 bytes -> ends at +0x140)
    i32 m_140, m_144, m_148, m_14c;
};

#endif // GRUNTZ_CMOVINGLOGICSERIAL_H
