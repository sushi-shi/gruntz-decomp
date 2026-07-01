// SBI_StatzTabGruntBar.h - Gruntz CSBI_StatzTabGruntBar (C:\Proj\Gruntz).
// RTTI .?AVCSBI_StatzTabGruntBar@@; a sibling leaf of the SBI family
//   CSBI_StatzTabGruntBar : CStatusBarItem  (RTTI hierarchy:
//   {CSBI_StatzTabGruntBar, CStatusBarItem}).
// Vtable @0x5eace4 (RTTI meta 0x5f4ed0). The /GX-framed scalar destructor (0x104b00)
// lives in SBI_StatzTabGruntBarEh.cpp.
//
// This item is the per-grunt "Statz" tab: each Update it samples the selected
// grunt's record (health/state/abilities/selection/timer) from the game registry,
// and when any of the five tracked values changes it resolves a glyph for it and
// flags the item dirty (return 1) so the next vfunc-10 redraw repaints. Modeled with
// the SBI family's manual-vtable-stamp device (no real `virtual`); sibling/engine
// callees are ILT/reloc-masked.
//
// Fields are placeholders; the offsets + code bytes are the load-bearing fact, the
// mangled (?<method>@CSBI_StatzTabGruntBar@@...) name is layout-independent.
#ifndef SBI_STATZTABGRUNTBAR_H
#define SBI_STATZTABGRUNTBAR_H

#include <Ints.h>
#include <rva.h>

// ---------------------------------------------------------------------------
// Shared engine views (modeled minimally; only the touched members/methods are
// load-bearing; every call/datum through them is reloc-masked).

// The selected grunt's record (an element of the registry's unit table at
// WwdGameReg+0x68). Update reads stat fields out of it; the deep offsets (0x170/
// 0x194/0x198/0x19c ability block, 0x1d8 alive gate, 0x3ec health) are the
// load-bearing facts.
struct CStatzGruntRec {
    char m_pad0[0x170];
    i32 m_170; // +0x170  ability level
    char m_pad174[0x194 - 0x174];
    i32 m_194; // +0x194  ability sub-value
    i32 m_198; // +0x198  override badge (non-zero => use directly)
    i32 m_19c; // +0x19c  ability cap (used when m_170 > 0x16)
    char m_pad1a0[0x1d8 - 0x1a0];
    i32 m_1d8; // +0x1d8  alive/active gate (0 => skip the timer block)
    char m_pad1dc[0x3ec - 0x1dc];
    i32 m_3ec; // +0x3ec  health
};
SIZE_UNKNOWN(CStatzGruntRec);

// The CTriggerMgr-style selection host: the unit-table base (WwdGameReg+0x68)
// doubles as the selection-list owner. SelectionListFind(key, y) resolves the
// selection glyph (__thiscall, ret 8).
struct CStatzSelHost {
    i32 SelectionListFind(i32 key, i32 y); // 0x7d2a0
};
SIZE_UNKNOWN(CStatzSelHost);

// The game-registry singleton (?g_gameReg@@3PAUWwdGameReg@@A @ VA 0x64556c). Only
// the +0x68 unit-table base the Statz tab samples is modeled (rows of 15 dwords).
struct CStatzGameReg {
    char m_pad0[0x68];
    CStatzSelHost* m_68; // +0x68  unit/record table base + selection host
};
SIZE_UNKNOWN(CStatzGameReg);

// A polymorphic view of `this` used only for the self-virtual slot-0x28 redraw
// dispatch (CStatusBarItem vtable slot 10): 10 leading slots + Refresh at index 10
// (byte 0x28). Declared (never defined) so no ??_7 is emitted here;
// `((CStatzSelf*)this)->Refresh()` lowers to the exact
// mov eax,[this]; mov ecx,this; call [eax+0x28] __thiscall dispatch.
class CStatzSelf {
public:
    virtual void v0();
    virtual void v4();
    virtual void v8();
    virtual void vc();
    virtual void v10();
    virtual void v14();
    virtual void v18();
    virtual void v1c();
    virtual void v20();
    virtual void v24();
    virtual void Refresh(); // +0x28 (slot 10)
};
SIZE_UNKNOWN(CStatzSelf);

// The glyph map a changed value is resolved through (this+0x74 for the first four
// values, this+0x68 for the timer value): a [m_64..m_68]-gated table at m_14.
struct CStatzGlyphMap {
    char m_pad0[0x14];
    i32* m_14; // +0x14  glyph table
    char m_pad18[0x64 - 0x18];
    i32 m_64; // +0x64  glyph-index range lo gate
    i32 m_68; // +0x68  glyph-index range hi gate
};
SIZE_UNKNOWN(CStatzGlyphMap);

// ---------------------------------------------------------------------------
// CSBI_StatzTabGruntBar - the per-grunt stat tab. Derives directly from
// CStatusBarItem (vtable @0x5eace4).
class CSBI_StatzTabGruntBar {
public:
    void Reset();      // 0xea470  drop the five tracked values (also the dtor teardown)
    i32 Poll(i32 arg); // 0xea4b0  Update + conditional vfunc-10 redraw (arg unused)
    i32 Update();      // 0xea6c0  resample the grunt and latch any changed value

    // ----- layout (placeholders; offsets are the load-bearing fact) -----
    char m_pad0[0x30];
    i32 m_30; // +0x30  status glyph
    i32 m_34; // +0x34  status glyph (latched)
    i32 m_38; // +0x38  status value (tracked)
    i32 m_3c; // +0x3c  ability glyph
    i32 m_40; // +0x40  ability glyph (latched)
    i32 m_44; // +0x44  ability value (tracked)
    i32 m_48; // +0x48  override glyph
    i32 m_4c; // +0x4c  override glyph (latched)
    i32 m_50; // +0x50  override value (tracked)
    i32 m_54; // +0x54  selection key (0 => skip selection sample)
    i32 m_58; // +0x58  selection glyph (latched)
    i32 m_5c; // +0x5c  selection value (tracked)
    i32 m_60; // +0x60  unit-table column index (== SelectionListFind y)
    i32 m_64; // +0x64  unit-table row index   (== SelectionListFind key)
    i32 m_68; // +0x68  timer glyph map pointer (CStatzGlyphMap*)
    i32 m_6c; // +0x6c  timer glyph (latched)
    i32 m_70; // +0x70  timer value (tracked)
    i32 m_74; // +0x74  glyph map pointer (CStatzGlyphMap*) for the first four values
    i32 m_78; // +0x78  timer anchor lo (g_645588 at last bump)
    i32 m_7c; // +0x7c  timer anchor hi
    i32 m_80; // +0x80  timer window lo
    i32 m_84; // +0x84  timer window hi
};
SIZE_UNKNOWN(CSBI_StatzTabGruntBar);

#endif // SBI_STATZTABGRUNTBAR_H
