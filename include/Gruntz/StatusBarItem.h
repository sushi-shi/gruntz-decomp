// StatusBarItem.h - Gruntz status-bar item base class (C:\Proj\Gruntz).
// Reconstruction sufficient to byte-match the small constructor. Field names
// are placeholders; the offsets are the load-bearing fact the match proves.
#ifndef STATUSBARITEM_H
#define STATUSBARITEM_H

#include <Ints.h>
#include <rva.h>

// ONE canonical CStatusBarItem, TWO dtor spellings selected per-TU by macro (the
// CHAIN-DTOR device below): by default the dtor is DECLARED-only, so ctor-side/
// builder TUs fold the tiny inline ctor with no /GX frame; a merged one-file-per-
// class SBI TU #defines SBI_DTOR_CHAIN and gets the retail INLINE DtorStatus dtor
// body, so its leaf's out-of-line ~CSBI_X folds the multilevel destructor chain +
// /GX EH frame. This device superseded the old frameless-vs-<Gruntz/SbiDtorChain.h>
// two-view split for the collapsed TUs (the *Eh.cpp collapse proved both spellings
// co-host in one TU byte-exactly - dtors 100%, ctor-side fns unchanged);
// SbiDtorChain.h remains only for the still-split dtor TUs (sbi_rectonlydtor_eh,
// sbi_statztabarrow_eh). (StatusBarItem.cpp + StatusBarGameMenu.cpp add two further
// LABELING-DEVICE redefs whose class NAME must stay "CStatusBarItem" so their
// ctor-call symbols pair with the 0x1005d0 ctor.)
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
    SbiRect() {}
    SbiRect(i32 l, i32 t, i32 r, i32 b) : m_0(l), m_4(t), m_8(r), m_c(b) {}
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
    // Args 5..8 are ONE by-value SbRect, and the CALLER proves it: BuildStatusBarTabs
    // (0xffde0) materializes them with `sub esp,0x10; mov ecx,esp; mov [ecx],<value>;
    // mov [ecx+4],..; mov [ecx+8],..; mov [ecx+0xc],..` - the struct is built DIRECTLY in
    // the outgoing arg frame from freshly-computed values, never from a named local.
    // Callee-side the two spellings are ABI-identical (10 dwords, same `ret`), so only the
    // call site can tell them apart. The sibling builder view <Gruntz/SbiTabzDialogViews.h>
    // already had it right - its slot-11 Setup takes `SbiRect rc` by value and its call
    // sites pass an INLINE TEMPORARY, `SbRect(cx - 0x5e, cy - 0x3c, ...)`. That temporary
    // is the whole trick: a named local makes cl materialize the struct and copy it; an
    // inline temporary makes it build the struct in place, which is what retail does.
    virtual i32 Setup(i32 a1, i32 a2, i32 a3, i32 a4, SbiRect rc, i32 a9, i32 a10); // slot 2
    virtual void SbiSlot3();                                                       // slot 3
    virtual void SbiSlot4(); // slot 4
    virtual void SbiSlot5(); // slot 5
    // slots 6..9 (0x100530/0x100550/0x100570/0x100590): base defaults - each is
    // `xor eax,eax; ret 0xc` => i32-return, 3 stack args, `return 0`. No SBI leaf
    // overrides them. Out-of-line default bodies in SBI_RectOnly.cpp.
    virtual i32 SbiSlot6(i32, i32, i32); // slot 6  0x100530
    virtual i32 SbiSlot7(i32, i32, i32); // slot 7  0x100550
    virtual i32 SbiSlot8(i32, i32, i32); // slot 8  0x100570
    virtual i32 SbiSlot9(i32, i32, i32); // slot 9  0x100590
    virtual void SetSubtype();           // slot 10

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

    // Member teardown run by the inline destructor of the CHAIN-DTOR device below
    // (reloc-masked extern; the retail standalone body is 0x10bfa0).
    void DtorStatus(); // 0x10bfa0

    // slot-1 base leg (vtbl slot [1] thunk 0x1848 -> 0x10bfc0): serialize the six
    // base-region fields (m_4..m_rect14, m_28). Re-attributed from CSBI_MenuItem
    // (dossier #16); the body TU is SBI_MenuItem.cpp (its retail obj neighborhood).
    i32 SerializeFields(void* ar, i32 kind, i32 a, i32 b); // 0x10bfc0
};
SIZE_UNKNOWN(CStatusBarItem);

// ---------------------------------------------------------------------------
// CHAIN-DTOR device (opt-in): a merged /GX leaf TU (the original one-file-per-class
// SBI TUs, rebuilt by the *Eh.cpp collapse) #defines SBI_DTOR_CHAIN before its first
// include; the base destructors then get their retail INLINE bodies, so the leaf's
// out-of-line ~CSBI_X folds the whole subobject teardown walk (per-level ??_7
// re-stamps + Dtor* calls) behind one /GX SEH frame with descending trylevels.
// Every other TU (macro undefined) sees today's declared-only dtor - preprocessor-
// identical, so this device is output-neutral outside the merged TUs. It supersedes
// <Gruntz/SbiDtorChain.h> for the collapsed TUs; the SBI_OWN_*_DTOR guards mirror
// that header's device (the one TU that owns a level's out-of-line dtor suppresses
// the inline body). See docs/patterns/eh-dtor-multilevel-polymorphic-chain.md.
#if defined(SBI_DTOR_CHAIN) && !defined(SBI_ITEM_OWN_DTOR)
inline CStatusBarItem::~CStatusBarItem() {
    DtorStatus();
}
#endif

#endif // STATUSBARITEM_H
