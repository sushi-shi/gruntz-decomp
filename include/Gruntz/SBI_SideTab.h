// SBI_SideTab.h - Gruntz CSBI_SideTab (C:\Proj\Gruntz), the FRAMELESS method view.
// The side-tab status-bar item + the engine-referent views its slot-3/4/5 methods
// drive. Moved here from the per-TU inline defs in SBI_SideTab.cpp so each shape
// carries a single shared definition (matching-neutral: offsets + code bytes are
// the load-bearing fact; the mangled ?<method>@CSBI_SideTab@@ names are
// layout-independent).
//
// TWO-VIEW SPLIT: this is the FRAMELESS method view (derives CStatusBarItem, models
// the leaf fields under side-tab names). SBI_SideTabBuild.cpp models the SAME retail
// class as its CTOR/builder view - a standalone polymorphic 0x5c child whose real
// virtual dtor lets `new`/`delete` auto-stamp ??_7CSBI_SideTab@@6B@ - kept in that
// TU's own header. One MSVC5 spelling emits only one of those shapes, so the two
// CSBI_SideTab views stay split and are NEVER co-included (see the two-view-split
// note atop <Gruntz/StatusBarItem.h>).
#ifndef GRUNTZ_SBI_SIDETAB_H
#define GRUNTZ_SBI_SIDETAB_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/ResMgr.h>        // canonical g_gameReg->m_world (CResMgr + draw chain)
#include <Gruntz/StatusBarItem.h> // canonical frameless CStatusBarItem base
#include <Image/CImage.h>         // the m_30/m_34 frame handles ARE CImage (RenderFrame @0x153790)

// A sampled grunt record (an element of the registry unit table at g_gameReg+0x68).
// Only the stat fields BuildHandle reads are modeled.
struct CSideTabGruntRec {
    char m_pad0[0x170];
    i32 m_170; // +0x170  ability level
    char m_pad174[0x198 - 0x174];
    i32 m_198; // +0x198  override badge
    i32 m_19c; // +0x19c  ability cap (used when level > 0x16)
    char m_pad1a0[0x3ec - 0x1a0];
    i32 m_3ec; // +0x3ec  health
};
SIZE_UNKNOWN(CSideTabGruntRec);

// The fallback notified (m_2c) when the sampled unit slot is empty (__thiscall, 1 arg).
struct CSideTabFallback {};
SIZE_UNKNOWN(CSideTabFallback);

// The per-frame unit-record table (g_gameReg->m_68): a flat array of grunt-record
// pointers at +0x1c indexed by (col + 15*row).
struct CSideTabUnitTable {
    char m_pad0[0x1c];
    CSideTabGruntRec* m_units[1]; // +0x1c  per-cell grunt-record pointers
};
SIZE_UNKNOWN(CSideTabUnitTable);

// The g_gameReg singleton (?g_gameReg@@3PAUWwdGameReg@@A @ VA 0x64556c) viewed by the
// SideTab paths: m_30 is the canonical resource manager (CResMgr), m_68 the per-frame
// unit-record table the sampled grunt record is indexed out of. Both slots are typed
// here so the render/glyph and sampling paths reach them with no reinterpret cast.
struct CSideTabGameReg {
    char m_pad00[0x30];
    CResMgr* m_world; // +0x30  resource manager
    char m_pad34[0x68 - 0x34];
    CSideTabUnitTable* m_68; // +0x68  per-frame unit-record table
};
SIZE_UNKNOWN(CSideTabGameReg);

// CSBI_SideTab - the side-tab status-bar item. Derives directly from
// CStatusBarItem. Fields are placeholders; the offsets + code bytes are the
// load-bearing fact, the mangled (?<method>@CSBI_SideTab@@...) name is
// layout-independent.
class CSBI_SideTab : public CStatusBarItem {
public:
    virtual ~CSBI_SideTab() OVERRIDE; // slot 0
    virtual i32 SbiVfunc0() OVERRIDE; // slot 1
    virtual void SbiSlot3() OVERRIDE; // slot 3
    virtual void SbiSlot4() OVERRIDE; // slot 4
    virtual void SbiSlot5() OVERRIDE; // slot 5
    void Reset();                     // vslot 3 (0xe9800)  drop the two frame handles
    i32 Refresh(i32 unused);          // vslot 4 (0xe9820)  rebuild the +0x58 draw gate (ret int 0)
    i32 Render(i32 z);                // vslot 5 (0xe99c0)  draw the two side frames
    i32 BuildHandle();                // 0xe9850  sibling: build the +0x58 draw gate

    // base region m_0..0x2b comes from CStatusBarItem; leaf fields start at +0x2c.
    CSideTabFallback* m_2c; // +0x2c  empty-slot fallback notify target
    CImage* m_30;           // +0x30  top frame handle
    CImage* m_34;           // +0x34  bottom frame handle (resolved glyph)
    i32 m_38;               // +0x38  tracked sampled value
    i32 m_3c;               // +0x3c  unit-table row index (stride 15)
    i32 m_40;               // +0x40  unit-table column index
    i32 m_44;               // +0x44  sample mode (0 idle / 2 ability / 3 badge / 1 health)
    i32 m_48;               // +0x48  draw x
    i32 m_4c;               // +0x4c  draw y
    i32 m_50;               // +0x50  bottom-frame y delta
    char m_pad54[0x58 - 0x54];
    i32 m_58; // +0x58  draw gate (0 => not built)
};
SIZE_UNKNOWN(CSBI_SideTab);
VTBL(CSBI_SideTab, 0x001eae3c); // vtable_names -> code (RTTI game class)

#endif // GRUNTZ_SBI_SIDETAB_H
