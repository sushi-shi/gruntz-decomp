// MovingLogicSerial.h - the bute-text serialization helpers of CMovingLogic
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
#include <Gruntz/MovingLogic.h> // the canonical CMovingLogic + CMotionState (+0x38 curve)
#include <rva.h>

// The bute-text accumulator is a CRT ostream (an ostrstream - its virtual base
// `ios` is the vbase-teardown wall the CBute*Temp helpers below reproduce). The
// formatted appends WriteCurve streams are the LIBRARY ostream::operator<<
// overloads, not game code: int (0x191d20, ??6ostream@@QAEAAV0@H@Z) and double
// (0x191df0, ??6ostream@@QAEAAV0@N@Z), both LIBCIMT and bound in
// config/library_labels.csv - reloc-masked, not reconstructed (game-not-CRT).
class ostream {
public:
    ostream& operator<<(i32 v);    // ??6ostream@@QAEAAV0@H@Z  0x191d20
    ostream& operator<<(double v); // ??6ostream@@QAEAAV0@N@Z  0x191df0
};
// The read side's accumulator is the matching istream (istrstream). ReadCurve is
// declared-only (reloc-masked); a forward decl suffices for its reference param.
class istream;

// CMovingLogic's movement-curve coefficient block (CMovingLogic+0x38) IS the
// CMotionState kinematic band (<Gruntz/MotionState.h>): the writer streams its
// doubles [0x00..0x50], [0x70..0xb0], the int at 0xb8, then [0xc0..0x100]. The
// 0x58/0x60/0x68 doubles and the 0xbc region are part of the block but not
// streamed. (Formerly the reduced view `CMovingLogicCurve`; unified onto the one
// real subobject.)

// ---------------------------------------------------------------------------
// CMovingLogic::Serialize (0x16f4a0) support. The class persists its 0x108-byte
// curve block via a bute-text round-trip through a CButeText accumulator, then
// chains to its base Serialize. The accumulator is a virtual-base CButeMgr temp:
// the most-derived ctor takes the full object while the teardown helpers act on
// the vbase subobject (+0xc) - that vbase this-adjust is a documented MSVC5 wall,
// so the construct/teardown are modeled here as reloc-masked helper views.

// The serialize/archive stream: the shared WAP32 stream interface (Read @ +0x2c /
// Write @ +0x30), a real declared-only virtual class.
#include <Gruntz/SerialArchive.h>

// RezAlloc/RezFree (0x1b9b46 / 0x1b9b82) - the engine block allocator.
#include <Rez/RezAlloc.h> // RezAlloc/RezFree (the global allocator pair)

// ReadCurve (0x16d000): parse an istream accumulator back into the curve block.
void ReadCurve(istream& accum, CMotionState& c); // 0x16d000

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

// The bute-text name transfer helpers (reloc-masked __cdecl externs): append the
// CString member's text to the accumulator / parse it back. Same role as
// WriteCurve/ReadCurve but for the name string at +0x18.
void WriteName(void* accum, void* pstr); // 0x193080
void ReadName(void* accum, void* pstr);  // 0x193140

// The persisted "logic types registered" cell (.data 0x6bf674) the base Serialize
// streams alongside the three trailing ints.
extern i32 g_logicTypesRegistered; // 0x6bf674 (?g_logicTypesRegistered@@3HA)

// The read-mode context arg: m_14 is seeded from ctx->m_7c.
struct CMlSerialCtx {
    char _00[0x7c];
    i32 m_7c; // +0x7c
};

// CMovingLogic's base class providing the chained Serialize (0x16e7f0): persist
// the name string + three trailing ints + g_logicTypesRegistered, and on read
// seed the back-pointers (m_c/m_10) and m_14 from the context arg.
class CMovingLogicBase {
public:
    i32 Serialize(CSerialArchive* arc, i32 mode, i32 a3, i32 a4); // 0x16e7f0

    void* _vptr;     // +0x00
    i32 m_4;         // +0x04
    i32 m_8;         // +0x08
    void* m_c;       // +0x0c
    void* m_10;      // +0x10
    i32 m_14;        // +0x14
    char m_18[0x10]; // +0x18  name CString (helpers take &m_18)
    i32 m_28;        // +0x28
    i32 m_2c;        // +0x2c
    i32 m_30;        // +0x30
};

// CMovingLogic itself is the shared canonical (<Gruntz/MovingLogic.h>): its
// Serialize (0x16f4a0) streams the +0x38 CMotionState curve (reached via Motion())
// and the four trailing ints, then chains to CMovingLogicBase::Serialize.

#endif // GRUNTZ_CMOVINGLOGICSERIAL_H
