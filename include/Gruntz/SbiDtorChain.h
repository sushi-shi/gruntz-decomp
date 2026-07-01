// SbiDtorChain.h - the ONE polymorphic destructor-chain view of the status-bar
// item family (C:\Proj\Gruntz). Shared by every SBI_*Eh.cpp TU.
//
// The retail SBI hierarchy is single-inheritance:
//   CStatusBarItem
//     -> CSBI_RectOnly
//          -> CSBI_Image
//               -> CSBI_ImageSet
//                    -> CSBI_ImageSetAni
// with the leaves (CSBI_WellGoo / CSBI_MenuItem / CSBI_SideTab /
// CSBI_GruntMachine / CSBI_StatzTabGruntBar / CSBI_WarlordHead /
// CSBI_StatzTabArrow) each derived from one of those levels.
//
// Every level has a non-trivial (inline) virtual destructor that calls its member
// teardown helper; MSVC5 folds the base-subobject teardown walk into the
// most-derived dtor and - because the base subobjects are non-trivial - emits the
// full /GX SEH frame (push -1/handler/fs:0 + descending trylevel stamps) plus the
// per-level vptr re-stamps. The ??_7 vftables auto-derive on the target (RTTI;
// config/vtable_names.csv), replacing the transitional manual
// `*(void**)this = &g_vtbl_*` stamps.
//
// A class is INLINE-base in most TUs but the OUT-OF-LINE leaf in exactly one (its
// own scalar-dtor TU). That TU #defines the matching SBI_OWN_*_DTOR macro before
// including this header, suppressing the header's inline body so it can supply the
// RVA-keyed out-of-line definition. Every other TU gets the inline base body.
//
// Field names/types are placeholders; only the offsets + total 0x60 size are
// load-bearing (the dtor proper never reads a size, but the size is kept identical
// to retail's allocation). See docs/patterns/eh-dtor-multilevel-polymorphic-chain.md.
#ifndef GRUNTZ_SBIDTORCHAIN_H
#define GRUNTZ_SBIDTORCHAIN_H

#include <Ints.h>
#include <rva.h>

// CStatusBarItem grand-base (vtable 0x5eabcc, 11 slots = vdtor + 10 virtuals).
// The +0x24 config-host pointer and +0x34 owned surface are named because
// ~CSBI_WellGoo (the one leaf whose teardown reads base storage) touches them;
// everything else in [0x04,0x60) is opaque base-region storage.
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
    void DtorStatus(); // 0x10bfa0  CStatusBarItem member teardown (reloc-masked)
    char m_pad4[0x24 - 0x04];
    void* m_configHost; // +0x24  config host (CSBI_WellGoo: the surface-pool owner)
    char m_pad28[0x34 - 0x28];
    void* m_ownedSurface; // +0x34  owned draw surface freed by ~CSBI_WellGoo
    char m_pad38[0x60 - 0x38];
};
inline CStatusBarItem::~CStatusBarItem() {
    DtorStatus();
}

// CSBI_RectOnly (vtable 0x5eab8c, 11 slots; overrides the vdtor).
struct CSBI_RectOnly : CStatusBarItem {
    virtual ~CSBI_RectOnly();
    void DtorRect(); // 0xe8760  CSBI_RectOnly member teardown (reloc-masked)
};
#ifndef SBI_OWN_RECTONLY_DTOR
inline CSBI_RectOnly::~CSBI_RectOnly() {
    DtorRect();
}
#endif

// CSBI_Image (vtable 0x5eac0c, 12 slots = vdtor + 10 + 1 new).
struct CSBI_Image : CSBI_RectOnly {
    virtual ~CSBI_Image();
    virtual void Imf1();
    void DtorImage(); // 0xe6d90  CSBI_Image member teardown (reloc-masked)
};
#ifndef SBI_OWN_IMAGE_DTOR
inline CSBI_Image::~CSBI_Image() {
    DtorImage();
}
#endif

// CSBI_ImageSet (vtable 0x5eac4c, 13 slots = vdtor + 10 + 1 + 1).
struct CSBI_ImageSet : CSBI_Image {
    virtual ~CSBI_ImageSet();
    virtual void Isf1();
    void DtorImageSet(); // CSBI_ImageSet member teardown (reloc-masked)
};
#ifndef SBI_OWN_IMAGESET_DTOR
inline CSBI_ImageSet::~CSBI_ImageSet() {
    DtorImageSet();
}
#endif

// CSBI_ImageSetAni (14 slots = vdtor + 10 + 1 + 1 + 1).
struct CSBI_ImageSetAni : CSBI_ImageSet {
    virtual ~CSBI_ImageSetAni();
    virtual void Ianf1();
    virtual void Ianf2();
    void DtorImageSetAni(); // CSBI_ImageSetAni member teardown (reloc-masked)
};
#ifndef SBI_OWN_IMAGESETANI_DTOR
inline CSBI_ImageSetAni::~CSBI_ImageSetAni() {
    DtorImageSetAni();
}
#endif

#endif // GRUNTZ_SBIDTORCHAIN_H
