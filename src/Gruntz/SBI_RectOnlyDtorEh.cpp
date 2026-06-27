#include <rva.h>
#include <Ints.h>
// SBI_RectOnlyDtorEh.cpp - the /GX EH-framed CSBI_RectOnly scalar destructor
// (0x100700), split off SBI_RectOnlyEh.cpp (C:\Proj\Gruntz). MSVC5's /GX frames
// the dtor's base-subobject teardown walk; it cannot share the frameless flags.
// The split is matching-neutral (each function is RVA-keyed).
//
// REAL polymorphic hierarchy:  CSBI_RectOnly : CStatusBarItem.  The grand-base
// CStatusBarItem has a non-trivial (inline) virtual dtor that calls its member
// teardown helper; MSVC folds the base dtor into the most-derived ~CSBI_RectOnly
// and - because the base subobject is non-trivial - emits the full /GX SEH frame
// (push -1/handler/fs:0 + the 0/-1 trylevel stamps) plus the per-level vptr
// re-stamps.  The ??_7CStatusBarItem / ??_7CSBI_RectOnly vftables auto-derive on
// the target (RTTI; config/vtable_names.csv), replacing the transitional manual
// `*(void**)this = &g_vtbl_*` stamps the old non-polymorphic model used.
//
// This is the 2-level case of docs/patterns/eh-dtor-multilevel-polymorphic-chain.md;
// it MUST live in its own TU because the shared base CSBI_RectOnly is the OUT-OF-LINE
// leaf here but the INLINE base of ~CSBI_ImageSet in SBI_ImageSetEh.cpp - one TU
// cannot define it both ways.

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

// CSBI_RectOnly most-derived (vtable 0x5eab8c, 11 slots; overrides the vdtor).
struct CSBI_RectOnly : CStatusBarItem {
    virtual ~CSBI_RectOnly();
    void DtorRect(); // 0xe8760  CSBI_RectOnly member teardown (reloc-masked)
};

// Stamp ??_7CSBI_RectOnly, run DtorRect, then MSVC folds the base dtor (stamp
// ??_7CStatusBarItem + DtorStatus). The non-trivial base subobject supplies the
// /GX frame and the 0/-1 trylevel stamps.
RVA(0x00100700, 0x55)
CSBI_RectOnly::~CSBI_RectOnly() {
    DtorRect();
}
