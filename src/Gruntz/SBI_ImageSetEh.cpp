#include <rva.h>
#include <Ints.h>
// SBI_ImageSetEh.cpp - the /GX EH-framed CSBI_ImageSet scalar destructor
// (0x102000), split off SBI_RectOnlyEh.cpp (C:\Proj\Gruntz). MSVC5's /GX frames
// the dtor's base-subobject teardown walk; the split is matching-neutral
// (each function is RVA-keyed).
//
// REAL polymorphic hierarchy:
//   CSBI_ImageSet : CSBI_Image : CSBI_RectOnly : CStatusBarItem.
// Each base subobject has a non-trivial (inline) virtual dtor calling its member
// teardown helper, so MSVC folds the three base teardowns into the most-derived
// ~CSBI_ImageSet and emits the full /GX SEH frame (push -1/handler/fs:0 + the
// 0/1/2/-1 trylevel stamps) plus the per-level vptr re-stamps. The four ??_7
// vftables (CSBI_ImageSet / CSBI_Image / CSBI_RectOnly / CStatusBarItem) auto-
// derive on the target (RTTI; config/vtable_names.csv), replacing the transitional
// manual `*(void**)this = &g_vtbl_*` stamps the old non-polymorphic model used.
//
// 4-level case of docs/patterns/eh-dtor-multilevel-polymorphic-chain.md. It lives
// in its own TU (not SBI_RectOnlyDtorEh.cpp) because the shared base CSBI_RectOnly
// is the INLINE base here but the OUT-OF-LINE leaf there.

// CStatusBarItem grand-base (vtable 0x5eabcc, 11 slots = vdtor + 10 virtuals).
struct CStatusBarItem {
    virtual ~CStatusBarItem();
    virtual void Sf1();
    virtual void Sf2();
    virtual void Sf3();
    virtual void Sf4();
    virtual void Sf5();
    virtual void Sf6();
    virtual void Sf7();
    virtual void Sf8();
    virtual void Sf9();
    virtual void Sf10();
    void DtorStatus(); // 0x10bfa0  CStatusBarItem base teardown (reloc-masked)
    char m_pad[0x60 - 0x04];
};
inline CStatusBarItem::~CStatusBarItem() { DtorStatus(); }

// CSBI_RectOnly base (vtable 0x5eab8c, 11 slots; overrides the vdtor).
struct CSBI_RectOnly : CStatusBarItem {
    virtual ~CSBI_RectOnly();
    void DtorRect(); // 0xe8760  CSBI_RectOnly base teardown (reloc-masked)
};
inline CSBI_RectOnly::~CSBI_RectOnly() { DtorRect(); }

// CSBI_Image base (vtable 0x5eac0c, 12 slots = vdtor + 10 + 1 new).
struct CSBI_Image : CSBI_RectOnly {
    virtual ~CSBI_Image();
    virtual void Imf1();
    void DtorImage(); // 0xe6d90  CSBI_Image base teardown (reloc-masked)
};
inline CSBI_Image::~CSBI_Image() { DtorImage(); }

// CSBI_ImageSet most-derived (vtable 0x5eac4c, 13 slots = vdtor + 10 + 1 + 1).
struct CSBI_ImageSet : CSBI_Image {
    virtual ~CSBI_ImageSet();
    virtual void Isf1();
    void DtorImageSet(); // most-derived (ImageSet) member teardown (reloc-masked)
};

// Stamp ??_7CSBI_ImageSet, run DtorImageSet, then MSVC folds the three base dtors
// (CSBI_Image, CSBI_RectOnly, CStatusBarItem) in. The non-trivial base subobjects
// supply the /GX frame and the 0/1/2/-1 trylevel stamps.
RVA(0x00102000, 0x7f)
CSBI_ImageSet::~CSBI_ImageSet() {
    DtorImageSet();
}
