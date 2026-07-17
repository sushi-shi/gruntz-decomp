// StatusBarItem.cpp - labeling stand-in for the standalone complete-object ctor.
//
// CStatusBarItem is ONE class, defined canonically in <Gruntz/StatusBarItem.h>
// with an INLINE ctor (so the derived CSBI_RectOnly folds it - see SBI_RectOnly.cpp).
// Retail emits that same inline ctor out-of-line as a COMDAT (the complete-object
// ctor at 0x1005d0) wherever a bare CStatusBarItem is constructed. MSVC 5.0,
// however, inlines this tiny ctor at every instantiation we can synthesize, so it
// will not emit a labelable standalone ??0 from the canonical inline form.
//
// To keep the 0x1005d0 byte-match, this TU takes the canonical class from
// <Gruntz/StatusBarItem.h> but #defines SBI_ITEM_OWN_CTOR first: that flips the
// header's inline ctor to an out-of-line DECLARATION, so MSVC emits a standalone ??0
// this TU labels with the RVA-keyed body below. Same one class, out-of-line-ctor
// facet; a tooling workaround for the inline-ctor COMDAT, NOT a second class.
#include <rva.h>

#define SBI_ITEM_OWN_CTOR
#define SBI_DTOR_CHAIN // emit the retail inline base-dtor (stamp vptr + DtorStatus), so this
                       // TU's ??1/??_GCStatusBarItem match retail (0x100780/0x100620) and the
                       // SBI leaf TUs - not the empty-stub form (was a cross-obj DIVERGENT).
#include <Gruntz/StatusBarItem.h>

// ---------------------------------------------------------------------------
// CStatusBarItem::CStatusBarItem()
// Out-of-line complete-object ctor: zeroes m_4/m_8/m_24/m_28 after the vftable
// is installed. Byte-identical to the COMDAT copy of the header's inline ctor.
RVA(0x001005d0, 0x17)
CStatusBarItem::CStatusBarItem() {
    m_4 = 0;
    m_8 = 0;
    m_24 = 0;
    m_28 = 0;
}

// ~CStatusBarItem is now the SBI_DTOR_CHAIN inline body (stamp vftable + DtorStatus),
// so MSVC synthesizes the retail ??1CStatusBarItem (0x100780: mov [ecx],vptr; jmp
// DtorStatus) and ??_GCStatusBarItem (0x100620: stamp + call DtorStatus + delete),
// byte-matching the SBI leaf TUs - no longer the empty-stub COMDAT that diverged.
// This TU labels the cl-auto-generated ??_G at its retail RVA (it emits the vftable).
// (The fabricated `SbiVfunc0 { return 0; }` that used to sit here as a vftable anchor is
// GONE: slot 1 is the real 4-arg CStatusBarItem::SerializeFields, defined at its retail
// 0x10bfc0 in SBI_MenuItem.cpp. No anchor is needed - MSVC has no key-function rule, and
// the out-of-line ctor at 0x1005d0 below stamps the vptr, which is what references ??_7
// and forces the COMDAT out in this TU.)
// @rva-symbol: ??_GCStatusBarItem@@UAEPAXI@Z 0x00100620 0x24

// CStatusBarItem vtable RVA binding (moved from the deleted
// src/Stub/BoundaryLowerThunks.cpp vtable catalog).
VTBL(CStatusBarItem, 0x001eabcc);

// ---------------------------------------------------------------------------
// CStatusBarItem::Setup (0x100660, vtable slot 2): the base 10-arg config setter -
// inherited by CSBI_StatzTabGruntBar / CSBI_GruntMachine / CSBI_SideTab, overridden by
// CSBI_RectOnly (0xe86e0, which also marks m_4 = 1). Bails (returns 0) if the object id
// (a2) or owner (a1) is null; otherwise stores the eight live args into the base-region
// fields and returns 1 (the last two args are ABI-accepted but unused). The +0x14 sub-
// block is filled through a single base pointer (lea &m_14). __thiscall, ret 0x28.
// (re-homed from src/Stub/GapFunctions.cpp; attribution vtable-proven, ??_7CStatusBarItem+0x8.)
// @early-stop
// 90.0%: every field/store + control flow byte-faithful (the same body as the derived
// override CSBI_RectOnly::Setup @0xe86e0 minus its m_4 = 1 store). Retail materializes a
// base pointer for the +0x14 sub-block (lea edx,[ecx+0x14] -> [edx+4/8/c] stores) while
// MSVC5 folds it to ecx-relative disp8 stores with an alternating eax/edx pair. A pure
// instruction-selection/scheduling residual (docs/patterns/statement-schedule-faithful.md);
// direct, struct-member, and struct-pointer spellings all fold identically and the permuter
// finds no operand fix - same non-steerable coin-flip as its 0xe86e0 sibling. Final sweep.
// ---------------------------------------------------------------------------
RVA(0x00100660, 0x50)
i32 CStatusBarItem::Setup(i32 a1, i32 a2, i32 a3, i32 a4, SbiRect rc, i32 a9, i32 a10) {
    if (a2 == 0 || a1 == 0) {
        return 0;
    }
    m_2c = a1;
    m_24 = a2;
    m_10 = a4;
    m_rect14.m_0 = rc.m_0;
    m_rect14.m_4 = rc.m_4;
    m_rect14.m_8 = rc.m_8;
    m_rect14.m_c = rc.m_c;
    m_c = a3;
    return 1;
}
