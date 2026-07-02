// BoundaryUpper2Eh.cpp - upper-half (RVA >= 0x133370) engine_boundary backlog,
// /GX EH-frame siblings of BoundaryUpper2.cpp. /GX destructors whose destructible
// base/member subobjects force the MSVC unwind frame. Only OFFSETS + code shape are
// load-bearing.
#include <Ints.h>
#include <rva.h>

// The Rez heap free (0x1b9b82, __cdecl). C++ linkage (NOT extern "C") so MSVC5
// treats it as potentially-throwing and keeps the /GX base-subobject unwind frame.
void RezFree(void* p);

// ---------------------------------------------------------------------------
// CRezDir base subobject (out-of-line dtor @0x13c520) shared by the two /GX
// destructors below. Modeled polymorphically so cl emits the base-subobject unwind
// frame + the most-derived vptr stamp; the emitted vtables reloc-mask retail's.
// ---------------------------------------------------------------------------
struct RezDirBase {
    virtual ~RezDirBase(); // 0x13c520
};
SIZE_UNKNOWN(RezDirBase);

// ---------------------------------------------------------------------------
// 0x13c9b0 - CRezDir /GX dtor: drain the two child lists (m_14, m_20) by repeatedly
// scalar-deleting the head (vtbl slot 1, arg 1), poison m_1c/m_10 with the purecall
// vftable, then fold the base subobject. __thiscall.
// ---------------------------------------------------------------------------
struct RezListNode {
    virtual void v0();
    virtual void Delete(i32); // slot 1 (+0x4)
};
SIZE_UNKNOWN(RezListNode);
DATA(0x001ef760)
extern void* g_purecallVtbl; // 0x5ef760 (PTR___purecall)
struct CRezDir13c9b0 : RezDirBase {
    i32 _4[(0x10 - 0x4) / 4];
    void* m_10;        // +0x10
    RezListNode* m_14; // +0x14
    i32 _18;           // +0x18
    void* m_1c;        // +0x1c
    RezListNode* m_20; // +0x20
    ~CRezDir13c9b0();
};
SIZE_UNKNOWN(CRezDir13c9b0);
RVA(0x0013c9b0, 0x7f)
CRezDir13c9b0::~CRezDir13c9b0() {
    while (m_14) {
        m_14->Delete(1);
    }
    while (m_20) {
        m_20->Delete(1);
    }
    m_1c = (void*)&g_purecallVtbl;
    m_10 = (void*)&g_purecallVtbl;
}

// ---------------------------------------------------------------------------
// 0x13cb80 - CRezDir-area /GX dtor: optional child cleanup (m_14 -> 0x13ce70),
// release the +0x10 buffer, detach via the +0x18 owner's +0x1c sub (0x1852e0), then
// fold the base subobject. __thiscall.
// ---------------------------------------------------------------------------
struct RezSub1c {
    void Detach(void* owner); // 0x1852e0
};
SIZE_UNKNOWN(RezSub1c);
struct RezOwner18 {
    i32 _0[0x1c / 4];
    RezSub1c m_1c; // +0x1c
};
SIZE_UNKNOWN(RezOwner18);
struct CRezDir13cb80 : RezDirBase {
    i32 _4[(0x10 - 0x4) / 4];
    void* m_10;       // +0x10
    i32 m_14;         // +0x14
    RezOwner18* m_18; // +0x18
    void Cleanup14(); // 0x13ce70
    ~CRezDir13cb80();
};
SIZE_UNKNOWN(CRezDir13cb80);
RVA(0x0013cb80, 0x72)
CRezDir13cb80::~CRezDir13cb80() {
    if (m_14) {
        Cleanup14();
    }
    if (m_10) {
        RezFree(m_10);
    }
    m_18->m_1c.Detach(this);
}

// ---------------------------------------------------------------------------
// 0x1333b0 - CInputDevBase's standalone /GX base-subobject destructor (the middle
// level of the DirectInput device chain): stamp base vftable B @0x5ef680, ReleaseBase,
// stamp grand-base C @0x5ef670, BaseDtorC. Kept manual-vptr here: the LEAF dtors
// (keyboard 0x133300 / mouse 0x1334f0 / joystick 0x133460) are now modeled real-
// polymorphic in src/DinMgr2/DirectInputMgr2.cpp (cl inlines this base unwind into
// each), but this standalone base dtor can't be bound here without dup'ing DinMgr2's
// inline CInputDevBase (its emitted COMDAT is unbound). Grand-base ~G @0x133370 is
// matched standalone in BoundaryUpper2.cpp.
// ---------------------------------------------------------------------------
extern void* g_deviceConfigVtblB; // 0x5ef680
extern void* g_deviceConfigVtblC; // 0x5ef670
struct DevCfgChain {
    void* m_vptr;
    void ReleaseBase(); // 0x1342b0
    void BaseDtorC();   // 0x134d50
    void DtorD1();
};
SIZE_UNKNOWN(DevCfgChain);
// @early-stop
// eh-dtor-needs-base-subobject wall (docs/patterns/eh-dtor-needs-base-subobject.md):
// body is byte-correct (stamp B / ReleaseBase / stamp C / BaseDtorC) but retail wraps
// it in a /GX frame with [esp+0x10] try-level stamps (0 / -1) from real base-subobject
// dtors, unreachable under this manual-vptr shape. DROPPED 42.7% -> 34.2% by an
// UNRELATED change: the device chain's base/root vtables 0x5ef680/0x5ef670 are now
// named ??_7CInputDevBase / ??_7CInputDevRoot (cl-emitted via VTBL in DirectInputMgr2.cpp),
// so this manual `&g_deviceConfigVtblB/C` stamp reloc-name-mismatches them. Binding a
// real ~CInputDevBase here would dup DinMgr2's inline base dtor - deferred to a final
// sweep that reunifies the whole chain in one TU.
RVA(0x001333b0, 0x55)
void DevCfgChain::DtorD1() {
    m_vptr = &g_deviceConfigVtblB;
    ReleaseBase();
    m_vptr = &g_deviceConfigVtblC;
    BaseDtorC();
}

// ---------------------------------------------------------------------------
// 0x17e7c0 - CFxModeT3 /GX constructor: run the base ctor (0x17e7b0) + the +0x24
// CString member ctor, init the descriptor fields, then assign the empty string to
// the +0x24 CString. The destructible CString member forces the /GX frame; returns
// `this`. __thiscall.
// ---------------------------------------------------------------------------
extern "C" char g_emptyString[]; // 0x6293f4
struct CStr17e7c0 {
    char* p;
    CStr17e7c0();                // 0x1b9b93
    void operator=(const char*); // 0x1b9e74
    ~CStr17e7c0();
};
SIZE_UNKNOWN(CStr17e7c0);
struct CFxBase17e7c0 {
    CFxBase17e7c0(); // 0x17e7b0
};
SIZE_UNKNOWN(CFxBase17e7c0);
struct CFxModeT3 : CFxBase17e7c0 {
    i32 m_0;
    i32 m_4;
    i32 m_8;
    i32 m_c;
    i32 m_10;
    i32 m_14;
    i32 m_18;
    i32 m_1c;
    i32 m_20;
    CStr17e7c0 m_24; // +0x24
    i32 m_28;        // +0x28
    CFxModeT3();
};
RVA(0x0017e7c0, 0x7a)
CFxModeT3::CFxModeT3() {
    m_0 = 1;
    m_4 = 0;
    m_8 = 0;
    m_c = 0;
    m_10 = 0x32;
    m_14 = 1;
    m_18 = 1;
    m_1c = 0;
    m_20 = 0;
    m_24 = g_emptyString;
    m_28 = 0;
}
