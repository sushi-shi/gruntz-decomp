// SBI_ImageSet.h - Gruntz CSBI_ImageSet (C:\Proj\Gruntz), the FRAMELESS method view.
// RTTI .?AVCSBI_ImageSet@@; most-derived of the SBI image chain
//   CSBI_ImageSet : CSBI_Image : CSBI_RectOnly : CStatusBarItem   (vtable 0x5eac4c).
//
// One canonical spelling of the class's non-dtor methods: the slot-1 Serialize
// (0xe74f0, defined in SBI_ImageSet.cpp) + its base BaseSerialize (0xe6e40). This
// is the FRAMELESS view (size 0x3c). Its deliberate counterparts (SAME retail
// class, never co-included - see the two-view-split note atop SbiDtorChain.h /
// StatusBarItem.h):
//   * <Gruntz/SbiDtorChain.h>  - the CHAIN view: the polymorphic /GX destructor
//     chain (grand-base 0x60) the *Eh.cpp dtor TUs fold.
//   * the builder TUs (CStatusBarMgr.cpp / StatusBarGameMenu.cpp /
//     SBI_TabzDialogEh.cpp) model the concrete widget as a tiny tag-4 ctor over
//     their local frameless status-bar-item base.
// Field names are placeholders; the offsets + total 0x3c size are the load-bearing
// fact (fields are name-independent at /O2).
#ifndef GRUNTZ_SBI_IMAGESET_H
#define GRUNTZ_SBI_IMAGESET_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/SBI_Image.h> // canonical frameless CSBI_Image base (real RTTI base)

struct CSprite; // full def in <Gruntz/Sprite.h>; only a CSprite* member is needed here

// The serialization stream (Serialize arg1): a real polymorphic object with
// ReadBytes at vtable slot 0x2c (index 0xb) and WriteBytes at slot 0x30 (index
// 0xc), both __thiscall (buf, len). Modeled as virtuals so a call lowers to
// `mov edx,[s]; call [edx+0x2c|0x30]` with ecx=this (no explicit-self push).
struct CImageSetStream {
    virtual void Vf0();
    virtual void Vf1();
    virtual void Vf2();
    virtual void Vf3();
    virtual void Vf4();
    virtual void Vf5();
    virtual void Vf6();
    virtual void Vf7();
    virtual void Vf8();
    virtual void Vf9();
    virtual void Vfa();
    virtual void ReadBytes(void* buf, i32 len);  // slot 0x2c
    virtual void WriteBytes(void* buf, i32 len); // slot 0x30
};
SIZE_UNKNOWN(CImageSetStream);

// CSBI_ImageSet - adds the slot-1 serialize override (save/load of the config id +
// name) on top of CSBI_Image (its real RTTI base). Offsets are load-bearing.
class CSBI_ImageSet : public CSBI_Image {
public:
    virtual ~CSBI_ImageSet() OVERRIDE; // slot 0
    virtual i32 SbiVfunc0() OVERRIDE;  // slot 1
    virtual void SbiSlot3() OVERRIDE;  // slot 3
    virtual void SbiSlot4() OVERRIDE;  // slot 4
    virtual void SbiSlot5() OVERRIDE;  // slot 5
    virtual i32 SetupImage(i32, CSbiConfigHost*, i32, i32, i32, i32, i32, i32, i32, i32, i32)
        OVERRIDE;                                                    // slot 11
    virtual void SbiSlot12();                                        // slot 12 (new)
    i32 Serialize(CImageSetStream* s, i32 mode, i32 a3, i32 a4);     // vslot 1 (0xe74f0)
    i32 BaseSerialize(CImageSetStream* s, i32 mode, i32 a3, i32 a4); // 0xe6e40 base slot 1
    // Member teardown run by the CHAIN-DTOR device (see StatusBarItem.h).
    void DtorImageSet(); // slot-3 teardown (reloc-masked extern)

    CSprite* m_34; // +0x34  resolved config record (the image registry's CSprite)
    i32 m_38;      // +0x38  serialized config id (4 bytes)
};
SIZE(CSBI_ImageSet, 0x3c);

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---
VTBL(CSBI_ImageSet, 0x001eac4c);

// CHAIN-DTOR device (see StatusBarItem.h): inline base-dtor body for the merged
// /GX leaf TUs; SBI_OWN_IMAGESET_DTOR marks the TU that owns the out-of-line ??1
// (SBI_ImageSet.cpp, RVA 0x102000).
#if defined(SBI_DTOR_CHAIN) && !defined(SBI_OWN_IMAGESET_DTOR)
inline CSBI_ImageSet::~CSBI_ImageSet() {
    DtorImageSet();
}
#endif

#endif // GRUNTZ_SBI_IMAGESET_H
