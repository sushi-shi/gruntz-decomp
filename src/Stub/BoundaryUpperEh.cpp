// BoundaryUpperEh.cpp - upper-half (RVA >= 0x133370) engine_boundary backlog,
// /GX EH-frame siblings of BoundaryUpper.cpp. These are manual-vtable "wap-object base
// worker" leaf destructors: stamp the most-derived vtable (cl prologue), tear down
// the owned member(s), then fold the base subobject (restamp the grand-base dtor
// vtable @0x5e8cb4). Modeled polymorphically (empty virtual base dtor + derived
// owned buffer) exactly like RezBufferObjectDtor.cpp (0x17f330, 100%): cl emits the
// /GX base-subobject unwind frame and the stamp-first prologue; the cl-emitted
// vtable symbols reloc-mask the retail manual vtables. Only OFFSETS + code shape
// are load-bearing.
#include <rva.h>
#include <DDrawMgr/DDrawWorkerHost.h>
#include <DDrawMgr/DDrawBlitParam.h>
#include <Gruntz/WwdGrid.h>
#include <Gruntz/BoundaryUpperViews.h>

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
SIZE_UNKNOWN(Sev17e240);
inline Sev17e240::~Sev17e240() {}
struct C17e240 : Sev17e240 {
    char* m_4; // +0x4
    virtual ~C17e240() OVERRIDE;
};
SIZE_UNKNOWN(C17e240);
RELOC_VTBL(C17e240, 0x001f0790); // vtable reloc-masks a bound datum (dtor-stamp verified)
RVA(0x0017e240, 0x51)
C17e240::~C17e240() {
    if (m_4) {
        RezFree(m_4);
    }
}

// ---------------------------------------------------------------------------
// 0x14fe30 - ~CShadeTableArray (IDENTITY: stamped vtable 0x5efb28 == ??_7CShadeTableArray,
// RTTI-confirmed; base folds ??_7CObject 0x5e8cb4). Free the +0x4 element buffer. STAYS a
// standalone placeholder here (NOT foldable onto the real CShadeTableArray in
// ShadeTableCache.cpp): the cache dtor ~CShadeTableCache (0x14de50, 100%) INLINES the array
// teardown (verified: it stamps 0x5efb28 + frees inline, does NOT call 0x14fe30). Defining
// CShadeTableArray::~CShadeTableArray out-of-line there makes 0x14de50 CALL it instead,
// cratering 0x14de50 100% -> ~61% (binary-proven this session). A class's dtor can be inline
// (for the owner's fold) XOR out-of-line (for 0x14fe30), not both - so this out-of-line ??1
// keeps its own placeholder class.
// ---------------------------------------------------------------------------
struct Sev14fe30 {
    virtual ~Sev14fe30();
};
SIZE_UNKNOWN(Sev14fe30);
inline Sev14fe30::~Sev14fe30() {}
struct C14fe30 : Sev14fe30 {
    char* m_4; // +0x4
    virtual ~C14fe30() OVERRIDE;
};
SIZE_UNKNOWN(C14fe30);
RELOC_VTBL(C14fe30, 0x001efb28); // vtable reloc-masks a bound datum (dtor-stamp verified)
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
SIZE_UNKNOWN(Sev161500);
inline Sev161500::~Sev161500() {}
struct C161500 : Sev161500 {
    char _4[0x14 - 0x4];
    char* m_14; // +0x14
    virtual ~C161500() OVERRIDE;
};
SIZE_UNKNOWN(C161500);
RELOC_VTBL(C161500, 0x001f0228); // vtable reloc-masks a bound datum (dtor-stamp verified)
RVA(0x00161500, 0x58)
C161500::~C161500() {
    if (m_14) {
        RezFree(m_14);
    }
    m_14 = 0;
}

// ---------------------------------------------------------------------------
// 0x138a50 - CGruntzSoundInnerZ::~CGruntzSoundInnerZ: RE-HOMED (folded onto the
// real class) into the gruntzsoundz unit (src/Dsndmgr/GruntzSoundZ.cpp), where the
// class + its VTBL already live. See that TU.
// ---------------------------------------------------------------------------

// (0x168c10 re-homed to src/Wwd/WwdGrid.cpp next to CWwdGrid - it is a second,
// un-COMDAT-folded copy of ~CWwdGrid @0x1682a0 (byte-identical, stamps 0x5f0328 +
// FreeBuckets); kept the distinct placeholder identity C168c10 since name-injectivity
// forbids two ~CWwdGrid. Both dtors + FreeBuckets stay 100% in that TU.)

// ---------------------------------------------------------------------------
// 0x15b6d0 - leaf dtor (stamp derived 0x5f0128, fold base ??_7CObject 0x5e8cb4). Member
// teardown 0x15c2c0 == CDDrawBlitParam::Reset_15c2c0; reset m_4/m_8/m_c (0x5f0128 unnamed;
// exact class unconfirmed).
// ---------------------------------------------------------------------------
struct Sev15b6d0 {
    virtual ~Sev15b6d0();
};
SIZE_UNKNOWN(Sev15b6d0);
inline Sev15b6d0::~Sev15b6d0() {}
struct C15b6d0 : Sev15b6d0 {
    i32 m_4; // +0x4
    i32 m_8; // +0x8
    i32 m_c; // +0xc
    virtual ~C15b6d0() OVERRIDE;
};
SIZE_UNKNOWN(C15b6d0);
RELOC_VTBL(C15b6d0, 0x001f0128); // vtable reloc-masks a bound datum (dtor-stamp verified)
RVA(0x0015b6d0, 0x5b)
C15b6d0::~C15b6d0() {
    ((CDDrawBlitParam*)this)->Reset_15c2c0();
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
SIZE_UNKNOWN(FaderBase);
struct C17f9f0 : FaderBase {
    virtual ~C17f9f0() OVERRIDE;
};
SIZE_UNKNOWN(C17f9f0);
RELOC_VTBL(C17f9f0, 0x001f0810); // vtable reloc-masks a bound datum (dtor-stamp verified)
RVA(0x0017f9f0, 0x4f)
C17f9f0::~C17f9f0() {
    ((B_17fc40*)this)->Free();
}
struct C180450 : FaderBase {
    virtual ~C180450() OVERRIDE;
    void SubFree(); // 0x180630
};
SIZE_UNKNOWN(C180450);
RELOC_VTBL(C180450, 0x001f0870); // vtable reloc-masks a bound datum (dtor-stamp verified)
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
SIZE_UNKNOWN(EmbedBase17e990);
inline EmbedBase17e990::~EmbedBase17e990() {}
struct EmbedSub17e990 : EmbedBase17e990 {
    char* m_4; // +0x4 (outer +0x5c)
    virtual ~EmbedSub17e990() OVERRIDE;
};
SIZE_UNKNOWN(EmbedSub17e990);
inline EmbedSub17e990::~EmbedSub17e990() {
    if (m_4) {
        RezFree(m_4);
    }
}
struct C17e990 : FaderBase {
    char _4[0x58 - 0x4];
    EmbedSub17e990 m_58; // +0x58
    virtual ~C17e990() OVERRIDE;
};
SIZE_UNKNOWN(C17e990);
RELOC_VTBL(C17e990, 0x001f07c0); // aliases CFaderMesh (dtor-stamp verified)
RVA(0x0017e990, 0x6b)
C17e990::~C17e990() {}

// (0x163a40 re-homed to src/Wwd/WwdSpatialMgr.cpp next to CWwdSpatialMgr::FreeGrids -
// it destructs a CWwdSpatialMgr (base subobject at +0x70, FreeGrids on this); kept the
// distinct placeholder identity C163a40 (most-derived vptr not at offset 0).)

// --- vtable catalog (reduced-view classes share their base vtable rva) ---

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---
