// SBI_RectOnlyBase.cpp - the thin RTTI CSBI_RectOnly intermediate's own retail obj
// (dossier #16, waveM-judgment). The per-class SBI item obj sequence in
// [0xe5ad0..0xe8733] ends with this class's slot-2 Setup body at 0xe86e0 (vtbl
// 0x1eab8c slot [2] thunk 0x2a2c); its other slot bodies/dtor are COMDAT-pool or
// 0x100xxx-band residents. Distinct from src/Gruntz/SBI_RectOnly.cpp, the 0x570
// status-bar HOST that wears the same class name (the known conflation - see
// SBI_RectOnly.h); THIS TU models the thin chain class of <Gruntz/SBI_Image.h>.
#include <Mfc.h>

#include <Gruntz/SBI_Image.h> // the frameless chain (CSBI_RectOnly : CStatusBarItem)
#include <Ints.h>
#include <rva.h>

// ---------------------------------------------------------------------------
// CSBI_RectOnly::CSBI_RectOnly (0x101fa0) - the thin sub-widget's ctor. Inlines the
// CStatusBarItem base ctor (the base's dead m_8=0 store is elided by /O2), stamps
// ??_7CSBI_RectOnly@@6B@ (0x5eab8c) and sets the subtype tag m_8 = 1.
//
// HOMED HERE BY THE SPLIT (2026-07-12): this ctor used to live in SBI_RectOnly.cpp,
// the 0x630 status-bar HOST's TU, because that TU's main class wore the CSBI_RectOnly
// name. It never belonged to the host - the host is non-polymorphic and stamps no
// vtable. See the banner of <Gruntz/StatusBarMgr.h>.
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
i32 CSBI_RectOnly::Setup(CStatusBarMgr* owner, CDDrawSurfaceMgr* host, i32 a3, i32 a4, SbiRect rc, i32 a9, i32 a10) {
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

// CSBI_RectOnly::DtorRect (0xe8760) - the empty 1-byte `ret` member-teardown the
// CHAIN-DTOR device (~CSBI_RectOnly) calls; an orphan COMDAT homed here in its owner
// class's band (this TU models CSBI_RectOnly), binding the dtor's CALL @0x100700+0x2c.
RVA(0x000e8760, 1)
void CSBI_RectOnly::DtorRect() {}
