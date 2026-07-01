#include <rva.h>
#include <Stub/MallocConstructors.h>
// MallocConstructors.cpp - constructor bodies for MallocConstructors.h (RVA-labeled
// so they delink/diff against retail).
//
// Reconstructed in place (owning class not yet attributed):
//   0x001736a0 / 0x00174cb0 / 0x00173c60 / 0x00174730 - the op-new + payload-copy
//     "boxed value" ctor family (store a tag/handle at +0x00, heap-copy a source
//     payload into a fresh operator-new'd block at +0x04, return this).
//   0x00174d00 - a CButeNode-family node ctor (base ctor 0x16dff0 + two re-stamped
//     vftables).

// ===========================================================================
// op-new + copy-construct "boxed value" ctor family.
// operator new @0x1b9b46 (MSVC5 CRT global op-new); the payload copy is a memberwise
// dword copy (<=4 dwords) or rep-movs (larger). 0x1736a0's payload is an MFC CString
// (throwing copy-ctor 0x1b9ba3 -> a /GX new-expression EH frame + delete-on-unwind).
// ===========================================================================
void* operator new(u32);
void operator delete(void*);

SIZE_UNKNOWN(MallocPayload16);
struct MallocPayload16 {
    i32 a, b, c, d;
}; // 16-byte POD (memberwise 4-dword copy)
SIZE_UNKNOWN(MallocPayload24);
struct MallocPayload24 {
    i32 a, b, c, d, e, f;
}; // 24-byte POD (rep-movs 6-dword copy)
SIZE_UNKNOWN(MallocStr);
struct MallocStr {                   // 4-byte value with a throwing copy-ctor (MFC CString)
    char* m_pchData;                 // +0x00
    MallocStr(const MallocStr& src); // 0x1b9ba3 (external, reloc-masked)
};

// 0x1736a0: store a handle at +0x00, box a CString copy at +0x04.
SIZE_UNKNOWN(BoxedStr);
class BoxedStr {
public:
    BoxedStr(i32 handle, const MallocStr& src);
    i32 m_00;
    MallocStr* m_04;
};
// 0x174cb0 / 0x173c60: box a 16-byte payload copy.
SIZE_UNKNOWN(Boxed16a);
class Boxed16a {
public:
    Boxed16a(i32 handle, const MallocPayload16* src);
    i32 m_00;
    MallocPayload16* m_04;
};
SIZE_UNKNOWN(Boxed16b);
class Boxed16b {
public:
    Boxed16b(i32 handle, const MallocPayload16* src);
    i32 m_00;
    MallocPayload16* m_04;
};
// 0x174730: box a 24-byte payload copy.
SIZE_UNKNOWN(Boxed24);
class Boxed24 {
public:
    Boxed24(i32 handle, const MallocPayload24* src);
    i32 m_00;
    MallocPayload24* m_04;
};

// ===========================================================================
// 0x174d00: a CButeNode-family node ctor. Base-constructs via the CButeNodeBase ctor
// (0x16dff0, descriptor @0x574df0 + kind arg), then re-stamps its two most-derived
// vftables (primary @0x5f051c at +0x00, +0x08 sub-object vftable @0x5f0518). No
// op-new, no EH frame (clean leaf). Manual stamps per the zPTree family convention
// (cf. CButeSectionCtor.cpp / CButeNodeBase.cpp).
// ===========================================================================
extern u8 g_node174df0Tag; // 0x574df0  kind descriptor (in .text)
DATA(0x001f0518)
extern void* g_node174dSubVtbl; // 0x5f0518  +0x08 sub-object vftable
DATA(0x001f051c)
extern void* g_node174dVtbl; // 0x5f051c  node primary vftable

SIZE_UNKNOWN(Node174d00);
struct Node174d00 {
    void CtorBase(void* desc, i32 kind); // 0x16dff0 CButeNodeBase ctor (external)
    Node174d00* Construct(i32 kind);     // 0x174d00

    void* m_vptr;       // +0x00  node primary vftable (manual stamp)
    char m_pad04[0x04]; // +0x04
    void* m_subVptr;    // +0x08  sub-object vftable (manual stamp)
};

// ===========================================================================
// Stub bodies (unattributed operator-new sites; empty ctors pending reconstruction).
// ===========================================================================
RVA(0x0015b390, 0x128)
MallocCtor_15b390::MallocCtor_15b390() {}

RVA(0x00192390, 0xef)
MallocCtor_192390::MallocCtor_192390() {}

RVA(0x00169ad0, 0xef)
MallocCtor_169ad0::MallocCtor_169ad0() {}

RVA(0x00169fb0, 0xec)
MallocCtor_169fb0::MallocCtor_169fb0() {}

RVA(0x001698c0, 0xdf)
MallocCtor_1698c0::MallocCtor_1698c0() {}

RVA(0x0013aa10, 0xdc)
MallocCtor_13aa10::MallocCtor_13aa10() {}

RVA(0x00169700, 0xba)
MallocCtor_169700::MallocCtor_169700() {}

RVA(0x0016c570, 0x9d)
MallocCtor_16c570::MallocCtor_16c570() {}

RVA(0x0013cac0, 0x9b)
MallocCtor_13cac0::MallocCtor_13cac0() {}

RVA(0x0016b510, 0x92)
MallocCtor_16b510::MallocCtor_16b510() {}

RVA(0x00136180, 0x86)
MallocCtor_136180::MallocCtor_136180() {}

RVA(0x0016bfa0, 0x85)
MallocCtor_16bfa0::MallocCtor_16bfa0() {}

RVA(0x00139bf0, 0x71)
MallocCtor_139bf0::MallocCtor_139bf0() {}

RVA(0x00184960, 0x70)
MallocCtor_184960::MallocCtor_184960() {}

RVA(0x00139c80, 0x6c)
MallocCtor_139c80::MallocCtor_139c80() {}

// ===========================================================================
// Reconstructed boxed-value / node ctors (retail-RVA order).
// ===========================================================================
RVA(0x001736a0, 0x5f)
BoxedStr::BoxedStr(i32 handle, const MallocStr& src) {
    m_00 = handle;
    m_04 = new MallocStr(src);
}

RVA(0x00174cb0, 0x49)
Boxed16a::Boxed16a(i32 handle, const MallocPayload16* src) {
    m_00 = handle;
    m_04 = new MallocPayload16(*src);
}

RVA(0x00173c60, 0x49)
Boxed16b::Boxed16b(i32 handle, const MallocPayload16* src) {
    m_00 = handle;
    m_04 = new MallocPayload16(*src);
}

RVA(0x00174730, 0x3c)
Boxed24::Boxed24(i32 handle, const MallocPayload24* src) {
    m_00 = handle;
    m_04 = new MallocPayload24(*src);
}

RVA(0x00168e70, 0x27)
MallocCtor_168e70::MallocCtor_168e70() {}

RVA(0x00174d00, 0x25)
Node174d00* Node174d00::Construct(i32 kind) {
    CtorBase(&g_node174df0Tag, kind);
    m_vptr = &g_node174dVtbl;
    m_subVptr = &g_node174dSubVtbl;
    return this;
}
