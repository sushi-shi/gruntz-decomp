// BoundaryUpperEh.cpp - upper-half (RVA >= 0x133370) engine_boundary backlog,
// /GX EH-frame siblings of BoundaryUpper.cpp. These are manual-vtable "severus
// worker" leaf destructors: stamp the most-derived vtable (cl prologue), tear down
// the owned member(s), then fold the base subobject (restamp the grand-base dtor
// vtable @0x5e8cb4). Modeled polymorphically (empty virtual base dtor + derived
// owned buffer) exactly like SeverusWorkerDtor.cpp (0x17f330, 100%): cl emits the
// /GX base-subobject unwind frame and the stamp-first prologue; the cl-emitted
// vtable symbols reloc-mask the retail manual vtables. Only OFFSETS + code shape
// are load-bearing.
#include <rva.h>

// The Rez heap free (0x1b9b82, __cdecl). C++ linkage (NOT extern "C") so MSVC5
// treats it as potentially-throwing and keeps the /GX base-subobject unwind frame.
void RezFree(void* p);

// ---------------------------------------------------------------------------
// 0x17e240 - CFader-area leaf dtor: stamp derived (0x5f0790), free the +0x4 heap
// buffer, restamp the base dtor vtable (0x5e8cb4).
// ---------------------------------------------------------------------------
struct Sev17e240 {
    virtual ~Sev17e240();
};
inline Sev17e240::~Sev17e240() {}
struct C17e240 : Sev17e240 {
    char* m_4; // +0x4
    ~C17e240();
};
RVA(0x0017e240, 0x51)
C17e240::~C17e240() {
    if (m_4) {
        RezFree(m_4);
    }
}

// ---------------------------------------------------------------------------
// 0x14fe30 - CShadeTableCache-area leaf dtor: stamp derived (0x5efb28), free the
// +0x4 heap buffer, restamp the base dtor vtable (0x5e8cb4).
// ---------------------------------------------------------------------------
struct Sev14fe30 {
    virtual ~Sev14fe30();
};
inline Sev14fe30::~Sev14fe30() {}
struct C14fe30 : Sev14fe30 {
    char* m_4; // +0x4
    ~C14fe30();
};
RVA(0x0014fe30, 0x51)
C14fe30::~C14fe30() {
    if (m_4) {
        RezFree(m_4);
    }
}

// ---------------------------------------------------------------------------
// 0x161500 - CImageSet3-area leaf dtor: stamp derived (0x5f0228), free the +0x14
// heap buffer and zero it, restamp the base dtor vtable (0x5e8cb4).
// ---------------------------------------------------------------------------
struct Sev161500 {
    virtual ~Sev161500();
};
inline Sev161500::~Sev161500() {}
struct C161500 : Sev161500 {
    char _4[0x14 - 0x4];
    char* m_14; // +0x14
    ~C161500();
};
RVA(0x00161500, 0x58)
C161500::~C161500() {
    if (m_14) {
        RezFree(m_14);
    }
    m_14 = 0;
}

// ---------------------------------------------------------------------------
// 0x138a50 - DSoundList inner-sound leaf dtor: stamp derived (0x5ef700), run the
// member teardown (0x138dd0), fold the base restamp (0x5e8cb4).
// ---------------------------------------------------------------------------
struct Sev138a50 {
    virtual ~Sev138a50();
};
inline Sev138a50::~Sev138a50() {}
struct C138a50 : Sev138a50 {
    ~C138a50();
    void SubTeardown(); // 0x138dd0
};
RVA(0x00138a50, 0x46)
C138a50::~C138a50() {
    SubTeardown();
}

// ---------------------------------------------------------------------------
// 0x168c10 - CAniRecord-area leaf dtor: stamp derived (0x5f0328), member teardown
// (0x191800), fold the base restamp (0x5e8cb4).
// ---------------------------------------------------------------------------
struct Sev168c10 {
    virtual ~Sev168c10();
};
inline Sev168c10::~Sev168c10() {}
struct C168c10 : Sev168c10 {
    ~C168c10();
    void SubTeardown(); // 0x191800
};
RVA(0x00168c10, 0x46)
C168c10::~C168c10() {
    SubTeardown();
}

// ---------------------------------------------------------------------------
// 0x15b6d0 - CWwdGameObject sub leaf dtor: stamp derived (0x5f0128), member
// teardown (0x15c2c0), reset m_4/m_8/m_c, fold the base restamp (0x5e8cb4).
// ---------------------------------------------------------------------------
struct Sev15b6d0 {
    virtual ~Sev15b6d0();
};
inline Sev15b6d0::~Sev15b6d0() {}
struct C15b6d0 : Sev15b6d0 {
    i32 m_4; // +0x4
    i32 m_8; // +0x8
    i32 m_c; // +0xc
    ~C15b6d0();
    void SubTeardown(); // 0x15c2c0
};
RVA(0x0015b6d0, 0x5b)
C15b6d0::~C15b6d0() {
    SubTeardown();
    m_4 = -1;
    m_8 = 0;
    m_c = 0;
}

// ---------------------------------------------------------------------------
// 0x17f9f0 / 0x180450 - CFxModeT3 / CFaderSine leaf dtors: stamp derived, run the
// member free, then CALL the shared out-of-line base dtor (0x17e4a0, which does the
// base restamp). The base is a real subobject with a non-inline dtor.
// ---------------------------------------------------------------------------
struct FaderBase {
    virtual ~FaderBase(); // out-of-line @0x17e4a0
};
struct C17f9f0 : FaderBase {
    ~C17f9f0();
    void SubFree(); // 0x17fc40
};
RVA(0x0017f9f0, 0x4f)
C17f9f0::~C17f9f0() {
    SubFree();
}
struct C180450 : FaderBase {
    ~C180450();
    void SubFree(); // 0x180630
};
RVA(0x00180450, 0x4f)
C180450::~C180450() {
    SubFree();
}

// ---------------------------------------------------------------------------
// 0x17e990 - CFaderSine leaf dtor with an embedded destructible sub-object at
// +0x58 (own vtable 0x5f07d8, owned buffer at +0x5c, grand-base 0x5e8cb4). Stamp
// the derived (0x5f07c0), destruct the +0x58 member, then CALL the shared base
// dtor (0x17e4a0). Members destruct before the base.
// ---------------------------------------------------------------------------
struct EmbedBase17e990 {
    virtual ~EmbedBase17e990();
};
inline EmbedBase17e990::~EmbedBase17e990() {}
struct EmbedSub17e990 : EmbedBase17e990 {
    char* m_4; // +0x4 (outer +0x5c)
    ~EmbedSub17e990();
};
inline EmbedSub17e990::~EmbedSub17e990() {
    if (m_4) {
        RezFree(m_4);
    }
}
struct C17e990 : FaderBase {
    char _4[0x58 - 0x4];
    EmbedSub17e990 m_58; // +0x58
    ~C17e990();
};
RVA(0x0017e990, 0x6b)
C17e990::~C17e990() {}

// ---------------------------------------------------------------------------
// 0x163a40 - CDDrawWorkerList-area dtor: the destructible base subobject lives at
// +0x70 (restamp 0x5e8cb4). Run the member teardown (0x1682f0), then the +0x70
// subobject folds. No most-derived vptr at offset 0.
// ---------------------------------------------------------------------------
struct Base163a40 {
    virtual ~Base163a40();
};
inline Base163a40::~Base163a40() {}
struct C163a40 {
    char _0[0x70];
    Base163a40 m_70;    // +0x70
    void SubTeardown(); // 0x1682f0
    ~C163a40();
};
RVA(0x00163a40, 0x41)
C163a40::~C163a40() {
    SubTeardown();
}
