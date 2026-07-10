// StatusBarItem.h - Gruntz status-bar item base class (C:\Proj\Gruntz).
// Reconstruction sufficient to byte-match the small constructor. Field names
// are placeholders; the offsets are the load-bearing fact the match proves.
#ifndef STATUSBARITEM_H
#define STATUSBARITEM_H

#include <Ints.h>
#include <rva.h>

// TWO-VIEW SPLIT (genuinely cannot merge to one C++ spelling): this header is the
// FRAMELESS view - the small 2-virtual inline-ctor base that the ctor-side/builder
// TUs (SBI_RectOnly.cpp, SBI_Image.cpp, CStatusBarMgr.h) use so MSVC5 FOLDS the tiny
// ctor at each instantiation with no /GX frame. <Gruntz/SbiDtorChain.h> is the CHAIN
// view - the full 11-virtual 0x60-byte grand-base with an INLINE DtorStatus dtor that
// the *Eh.cpp dtor TUs use so the multilevel destructor chain + /GX EH frame emits.
// The two are NEVER co-included (verified) precisely because one class cannot be both:
// a class carrying the inline DtorStatus dtor + 11 virtuals would force the ctor-side
// TUs to inline that dtor (a destructible-base /GX frame retail does not have there),
// and a 2-virtual declared-dtor class cannot emit the dtor chain. Same retail class,
// two deliberate reconstruction views. (StatusBarItem.cpp + StatusBarGameMenu.cpp add
// two further LABELING-DEVICE redefs whose class NAME must stay "CStatusBarItem" so
// their ctor-call symbols pair with the 0x1005d0 ctor.)
//
// ---------------------------------------------------------------------------
// CStatusBarItem - base of the SBI_* family. One class, one definition.
//
// The ctor is INLINE: the derived CSBI_RectOnly ctor folds it (zeroing
// m_4/m_24/m_28; the base's m_8=0 store is then dead, since CSBI_RectOnly sets
// m_8=1, and the optimiser drops it). MSVC 5.0 only folds a base ctor that is
// visible inline, so inline is the load-bearing choice the match pins.
//
// Retail also has a standalone complete-object ctor (one that *does* zero all
// four fields, at its own RVA 0x1005d0): MSVC's out-of-line COMDAT copy of this
// same inline ctor. It is NOT a second class. MSVC 5.0 inlines this tiny ctor at
// every instantiation we can synthesize, so the canonical inline form cannot emit
// a labelable standalone ??0 to diff against it. To keep that byte-match,
// src/Gruntz/StatusBarItem.cpp is a stand-in TU that locally redeclares the class
// with an out-of-line ctor purely as a labeling device for the COMDAT - a tooling
// workaround, not a second class the developers wrote.
// ---------------------------------------------------------------------------
// The +0x14 sub-block CSBI_RectOnly::Setup fills (a RECT-like 4-int record).
struct SbiRect {
    i32 m_0; // +0x00 (rel +0x14)
    i32 m_4; // +0x04 (rel +0x18)
    i32 m_8; // +0x08 (rel +0x1c)
    i32 m_c; // +0x0c (rel +0x20)
};
SIZE_UNKNOWN(SbiRect);

class CStatusBarItem {
public:
    // The ctor is INLINE in every builder/ctor-side TU (so the derived CSBI_RectOnly
    // folds it). The standalone complete-object ctor COMDAT (0x1005d0) is labeled by
    // src/Gruntz/StatusBarItem.cpp, which #defines SBI_ITEM_OWN_CTOR to take the
    // out-of-line declaration + supply the RVA-keyed body (mirrors SbiDtorChain.h's
    // SBI_OWN_*_DTOR device); NOT a second class.
#ifdef SBI_ITEM_OWN_CTOR
    CStatusBarItem();
#else
    CStatusBarItem() {
        m_4 = 0;
        m_8 = 0;
        m_24 = 0;
        m_28 = 0;
    }
#endif
    virtual ~CStatusBarItem();
    virtual i32 SbiVfunc0(); // slot 1
    // vtable slot 2 (0x100660): the base 10-arg setup - bails (returns 0) if the object
    // id (a2) or owner (a1) is null, else stores the eight live args into the base-region
    // fields (the last two args are ABI-accepted but unused). CSBI_RectOnly overrides it
    // (0xe86e0) to additionally mark m_4 = 1 (active).
    virtual i32
    Setup(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7, i32 a8, i32 a9, i32 a10); // slot 2
    virtual void SbiSlot3();   // slot 3
    virtual void SbiSlot4();   // slot 4
    virtual void SbiSlot5();   // slot 5
    virtual void SbiSlot6();   // slot 6
    virtual void SbiSlot7();   // slot 7
    virtual void SbiSlot8();   // slot 8
    virtual void SbiSlot9();   // slot 9
    virtual void SetSubtype(); // slot 10

    i32 m_4;  // +0x04
    i32 m_8;  // +0x08
    i32 m_c;  // +0x0c  Setup arg3
    i32 m_10; // +0x10  Setup arg4
    // +0x14..0x20: a 4-int sub-block (a RECT-like record) that Setup fills through
    // a single base pointer (lea &m_14; [+0]/[+4]/[+8]/[+c]).
    SbiRect m_rect14; // +0x14  Setup args 5..8
    i32 m_24;         // +0x24  Setup arg2
    i32 m_28;         // +0x28
    i32 m_2c;         // +0x2c  Setup arg1 (owner/id target)
};
SIZE_UNKNOWN(CStatusBarItem);

#endif // STATUSBARITEM_H
