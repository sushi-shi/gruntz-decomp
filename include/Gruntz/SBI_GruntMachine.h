// SBI_GruntMachine.h - Gruntz CSBI_GruntMachine (C:\Proj\Gruntz).
// RTTI .?AVCSBI_GruntMachine@@; a sibling leaf of the SBI family
//   CSBI_GruntMachine : CStatusBarItem  (RTTI hierarchy: {CSBI_GruntMachine,
//   CStatusBarItem}). Vtable @0x5eadbc (RTTI meta 0x5f4fa0). The /GX-framed scalar
// destructor (0x104ce0) lives in SBI_GruntMachineEh.cpp.
//
// The "grunt machine" status item: a short two-frame anim driven by a countdown.
// SetFrames primes the two frame indices and arms the countdown (m_28=2); each
// Render tick decrements the countdown, resolves the two indexed frame records
// through the config record's gated frame table, and blits up to three frames (the
// standalone handle m_44, plus the two resolved records) at the base origin offset
// by each record's own draw delta. Modeled with the SBI family's manual-vtable-stamp
// device (no real `virtual`); sibling/engine callees are ILT/vtable-reloc-masked.
//
// Fields are placeholders; the offsets + code bytes are the load-bearing fact, the
// mangled (?<method>@CSBI_GruntMachine@@...) name is layout-independent.
#ifndef SBI_GRUNTMACHINE_H
#define SBI_GRUNTMACHINE_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/StatusBarItem.h> // CStatusBarItem base
#include <Gruntz/SbRect.h>        // BuildResourceTabStatusBar's by-value geometry rect

// BuildResourceTabStatusBar's owner/config-host pair (pointers only - fwd-decl).
// `class` vs `struct` is load-bearing for the mangling; both match their real defs.
class CStatusBarMgr;
class CDDrawSurfaceMgr;

#include <Image/CImage.h> // the canonical frame-record class (CImage::RenderFrame @0x153790)

// ---------------------------------------------------------------------------
// Shared engine views (modeled minimally; only the touched members/methods are
// load-bearing; every call/datum through them is reloc-masked).

// The frame record (an element of the config record's m_14 frame table, and the
// type of the standalone m_44 handle) is the RTTI-confirmed CImage: a draw-offset
// pair at m_18/m_1c, drawn by CImage::RenderFrame (0x153790, __thiscall). Modeled
// by the shared <Image/CImage.h> definition; every call through it is reloc-masked.

// The resolved config record (CSBI_GruntMachine::m_30): a frame-index range gate at
// m_64/m_68 and a frame table at m_14 (an array of CImage*). Same CSbiConfigRecord
// shape as CSbiConfigRecord (<Gruntz/SbiConfig.h>).
struct CGmConfig {
    char m_pad0[0x14];
    CImage** m_14; // +0x14  frame table (array of frame-record pointers)
    char m_pad18[0x64 - 0x18];
    i32 m_64; // +0x64  frame-index range lo gate (idx < m_64 => reject)
    i32 m_68; // +0x68  frame-index range hi gate (idx > m_68 => reject)
};
SIZE_UNKNOWN(CGmConfig);

// The active surface context Render passes into RenderFrame is reached through the
// canonical resource manager: g_gameReg->m_world (CDDrawSurfaceMgr) ->
// m_drawTarget (CDDrawSubMgrPages, +0x04) -> m_drawContext (+0x14). Modeled by the shared
// <Gruntz/GameRegistry.h> + <Gruntz/ResMgr.h> types (see SBI_GruntMachine.cpp); no
// per-TU game-manager facet is kept.

// ---------------------------------------------------------------------------
// CSBI_GruntMachine - the grunt-machine status-bar item. Real RTTI base is
// CStatusBarItem (vtable @0x5eadbc); kept FLAT (frameless method-view) because the
// render methods read base-region storage (m_14/m_18/m_28) under machine-specific
// names that CStatusBarItem models as the m_rect14 aggregate.
class CSBI_GruntMachine : public CStatusBarItem {
public:
    // tag 9 (the Resource-tab MACHINE widget). Built through BuildResourceTabStatusBar.
    CSBI_GruntMachine() {
        m_8 = 9;
        m_34 = 0;
        m_3c = 0;
        m_44 = 0;
        m_30 = 0;
    }
    // Real vtable shape (sema class: vtbl@0x1eadbc, 11 slots; overrides 0/1/3/4/5).
    // The out-of-line ~ (0x104ce0, calls Reset) lives in SBI_GruntMachine.cpp via the
    // CHAIN-DTOR device (see StatusBarItem.h).
    virtual ~CSBI_GruntMachine() OVERRIDE; // slot 0
    virtual i32 SbiVfunc0() OVERRIDE;      // slot 1
    virtual void SbiSlot3() OVERRIDE;      // slot 3 (the Reset below)
    virtual void SbiSlot4() OVERRIDE;      // slot 4
    virtual void SbiSlot5() OVERRIDE;      // slot 5 (the Render below)

    // 0xe8a70: the machine widget's own "configure" (the Resource tab's MACHINE item is
    // built through this, not through the CSBI_Image SetupImage slot - CSBI_GruntMachine
    // derives straight from CStatusBarItem and has no slot 11). `owner`/`host` are the
    // same pair SetupImage takes (owner -> m_2c, config host -> m_24, deref'd at +0x10).
    // Was defined as `CSbTab::BuildResourceTabStatusBar` - a view that CONFLATED this
    // class with CSBI_StatzTabGruntBar - while the caller referenced it on the fabricated
    // CSbConfigItem base, so the call resolved to NO definition (an unresolved external).
    i32 BuildResourceTabStatusBar(
        CStatusBarMgr* owner,
        CDDrawSurfaceMgr* host,
        i32 p3,
        i32 p4,
        SbRect g,
        const char* key,
        i32 idxA,
        i32 idxB
    ); // 0xe8a70

    // vtable slot 3 (0xe8c70): drop the standalone frame handle + the two resolved
    // frame records (also reached by the destructor as the member teardown). Out-of-line
    // (retail emits it as a standalone .text fn / vtable slot).
    void Reset(); // 0xe8c70
    // 0xe8dc0 (__thiscall, ret 8): prime the two frame indices (each gated by != -1)
    // and arm the countdown (m_28 = 2). Out-of-line.
    void SetFrames(i32 idxA, i32 idxB); // 0xe8dc0
    // vtable slot 5 (0xe8cb0): the per-frame render of the machine's frames.
    i32 Render(i32 z);

    // ----- own fields (after CStatusBarItem @0x30, which now owns m_2c); base draw
    // origins reuse m_rect14.m_0/m_4 (@0x14/0x18), the frame countdown reuses m_28.
    CGmConfig* m_30; // +0x30  resolved config record (frame table host)
    CImage* m_34;    // +0x34  resolved frame for index m_38
    i32 m_38;        // +0x38  frame index A (resolved into m_34)
    CImage* m_3c;    // +0x3c  resolved frame for index m_40
    i32 m_40;        // +0x40  frame index B (resolved into m_3c)
    CImage* m_44;    // +0x44  standalone frame handle (blitted directly)
};
SIZE_UNKNOWN(CSBI_GruntMachine);
VTBL(CSBI_GruntMachine, 0x001eadbc); // vtable_names -> code (RTTI game class)

#endif // SBI_GRUNTMACHINE_H
