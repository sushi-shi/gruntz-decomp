#include <Mfc.h>

#define SBI_RECTONLY_OWN_CTOR // this TU emits the out-of-line ??0CSBI_RectOnly COMDAT (0x101fa0)
#include <Gruntz/SBI_Image.h> // the frameless chain (CSBI_RectOnly : CStatusBarItem)
#include <Ints.h>
#include <rva.h>

RVA(0x00101fa0, 0x1b)
CSBI_RectOnly::CSBI_RectOnly() {
    m_kind = 1;
}

// ---------------------------------------------------------------------------
// CSBI_RectOnly::Setup - vtable slot 2 (0xe86e0). The 10-arg config setter (the
// last two args are accepted by the ABI but unused). Bails (returns 0) if either
// the object id (a2) or the owner (a1) is null; otherwise stores the eight live
// args into the base-region fields, marks m_4 = 1 (active) and returns 1.
// @early-stop
// 90.4%: every field/store and the control flow are correct, BUT retail writes the
// +0x14 sub-block through a `lea edx,[ecx+0x14]` base (strict sequential load-store)
// while MSVC5 here folds it to ecx-relative disp8 stores and interleaves the loads.
// A pure instruction-selection/scheduling residual (docs/patterns/
// statement-schedule-faithful.md) - direct, array, and struct-pointer spellings all
// fold the same; not steerable from C. Deferred to the final sweep.
RVA(0x000e86e0, 0x53)
i32 CSBI_RectOnly::Setup(
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
    m_enabled = 1;
    return 1;
}

RVA(0x000e8760, 0x1)
void CSBI_RectOnly::Reset() {}
RVA(0x000e8780, 0x8)
i32 CSBI_RectOnly::Refresh(i32) {
    return 1;
}
