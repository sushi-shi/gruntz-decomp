#include <rva.h>

#define SBI_ITEM_OWN_CTOR
#define SBI_DTOR_CHAIN // emit the retail inline base-dtor (stamp vptr + Reset), so this
#include <Gruntz/StatusBarItem.h>

RVA(0x001005b0, 0x8)
void CStatusBarItem::SetSubtype() {
    m_28 = 2;
}

RVA(0x001005d0, 0x17)
CStatusBarItem::CStatusBarItem() {
    m_enabled = 0;
    m_kind = 0;
    m_24 = 0;
    m_28 = 0;
}

// ~CStatusBarItem is now the SBI_DTOR_CHAIN inline body (stamp vftable + Reset),
// so MSVC synthesizes the retail ??1CStatusBarItem (0x100780: mov [ecx],vptr; jmp
// Reset) and ??_GCStatusBarItem (0x100620: stamp + call Reset + delete),
// byte-matching the SBI leaf TUs - no longer the empty-stub COMDAT that diverged.
// This TU labels the cl-auto-generated ??_G at its retail RVA (it emits the vftable).
// (The fabricated `SbiVfunc0 { return 0; }` that used to sit here as a vftable anchor is
// GONE: slot 1 is the real 4-arg CStatusBarItem::SerializeFields, defined at its retail
// 0x10bfc0 in SBI_MenuItem.cpp. No anchor is needed - MSVC has no key-function rule, and
// the out-of-line ctor at 0x1005d0 below stamps the vptr, which is what references ??_7
// and forces the COMDAT out in this TU.)
RVA(0x00100600, 0x8)
i32 CStatusBarItem::Refresh(i32) {
    return 1;
}

RVA_COMPGEN(0x00100620, 0x24, ??_GCStatusBarItem@@UAEPAXI@Z)

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
i32 CStatusBarItem::Setup(
    CStatusBarMgr* owner,
    CDDrawSurfaceMgr* host,
    i32 a3,
    i32 a4,
    SbiRect rc,
    i32 a9,
    i32 a10
) {
    if (host == 0 || owner == 0) {
        return 0;
    }
    m_2c = owner;
    m_24 = host;
    m_tab = a4;
    m_rect14.m_0 = rc.m_0;
    m_rect14.m_4 = rc.m_4;
    m_rect14.m_8 = rc.m_8;
    m_rect14.m_c = rc.m_c;
    m_cmd = a3;
    return 1;
}
