#include <rva.h>
#include <Ints.h>

#define SBI_OWN_RECTONLY_DTOR // this TU supplies the out-of-line ~CSBI_RectOnly
#include <Gruntz/SbiDtorChain.h>
// SBI_RectOnlyDtorEh.cpp - the /GX EH-framed CSBI_RectOnly scalar destructor
// (0x100700), split off SBI_RectOnlyEh.cpp (C:\Proj\Gruntz). MSVC5's /GX frames
// the dtor's base-subobject teardown walk; it cannot share the frameless flags.
// The split is matching-neutral (each function is RVA-keyed).
//
// 2-level case of docs/patterns/eh-dtor-multilevel-polymorphic-chain.md. It MUST
// live in its own TU because the shared base CSBI_RectOnly is the OUT-OF-LINE leaf
// here but the INLINE base of ~CSBI_ImageSet in SBI_ImageSetEh.cpp - one TU cannot
// define it both ways (hence the SBI_OWN_RECTONLY_DTOR guard above).

// Stamp ??_7CSBI_RectOnly, run DtorRect, then MSVC folds the base dtor (stamp
// ??_7CStatusBarItem + DtorStatus). The non-trivial base subobject supplies the
// /GX frame and the 0/-1 trylevel stamps.
RVA(0x00100700, 0x55)
CSBI_RectOnly::~CSBI_RectOnly() {
    DtorRect();
}

// -------------------------------------------------------------------------
// 0x100780 (spatially re-homed from src/Stub/BoundaryLowerThunks.cpp). A
// CStatusBarItem-base vptr restore: cl's implicit vptr-restore stamps the
// status-base vtable (0x5eabcc) then tail-jumps the base init/teardown (0x1d6b).
// Placeholder polymorphic class (the real CStatusBarItem dtor is modeled in
// StatusBarItem.cpp; this is a distinct restore, so its ??_7 reloc-masks by shape).
struct CStatusBaseSub100780 {
    void Base1d6b(); // 0x1d6b (reloc-masked)
    virtual ~CStatusBaseSub100780();
    virtual void VtSlotFill0(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill1(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill2(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill3(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill4(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill5(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill6(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill7(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill8(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill9(); // vtable-slot filler (real slot; declared-only)
};
SIZE_UNKNOWN(CStatusBaseSub100780);
RELOC_VTBL(
    CStatusBaseSub100780,
    0x001eabcc
); // vtable reloc-masks a bound datum (dtor-stamp verified)
RVA(0x00100780, 0xb)
CStatusBaseSub100780::~CStatusBaseSub100780() {
    Base1d6b();
}
