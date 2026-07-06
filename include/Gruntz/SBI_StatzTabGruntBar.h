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
#include <Gruntz/StatusBarItem.h> // canonical frameless CStatusBarItem base (real RTTI base)

// ---------------------------------------------------------------------------
// Shared engine views (modeled minimally; only the touched members/methods are
// load-bearing; every call/datum through them is reloc-masked).

// The selected grunt's record (an element of the registry's unit table at
// WwdGameReg+0x68). Update reads stat fields out of it; the deep offsets (0x170/
// 0x194/0x198/0x19c ability block, 0x1d8 alive gate, 0x3ec health) are the
// load-bearing facts.
struct CStatzGruntRec {
    char m_pad0[0x170];
    i32 m_abilityLevel; // +0x170  ability level
    char m_pad174[0x194 - 0x174];
    i32 m_abilitySub; // +0x194  ability sub-value
    i32 m_badge;      // +0x198  override badge (non-zero => use directly)
    i32 m_abilityCap; // +0x19c  ability cap (used when m_abilityLevel > 0x16)
    char m_pad1a0[0x1d8 - 0x1a0];
    i32 m_alive; // +0x1d8  alive/active gate (0 => skip the timer block)
    char m_pad1dc[0x3ec - 0x1dc];
    i32 m_health; // +0x3ec  health
};
SIZE_UNKNOWN(CStatzGruntRec);

// The CTriggerMgr-style selection host: the unit-table base (WwdGameReg+0x68)
// doubles as the selection-list owner. SelectionListFind(key, y) resolves the
// selection glyph (__thiscall, ret 8).
struct CStatzSelHost {};
SIZE_UNKNOWN(CStatzSelHost);

// The game-registry singleton (?g_gameReg@@3PAUWwdGameReg@@A @ VA 0x64556c). Only
// the +0x68 unit-table base the Statz tab samples is modeled (rows of 15 dwords).
struct CStatzGameReg {
    char m_pad0[0x68];
    CStatzSelHost* m_unitTable; // +0x68  unit/record table base + selection host
};
SIZE_UNKNOWN(CStatzGameReg);

// A polymorphic view of `this` used only for the self-virtual slot-0x28 redraw
// dispatch (CStatusBarItem vtable slot 10): 10 leading slots + Refresh at index 10
// (byte 0x28). Declared (never defined) so no ??_7 is emitted here;
// `((CStatzSelf*)this)->Refresh()` lowers to the exact
// mov eax,[this]; mov ecx,this; call [eax+0x28] __thiscall dispatch.
class CStatzSelf {
public:
    virtual void Destroy();     // slot 0  scalar-deleting dtor
    virtual void Serialize();   // slot 1
    virtual void Setup();       // slot 2
    virtual void ClearFrame();  // slot 3
    virtual void Poll();        // slot 4
    virtual void Tick();        // slot 5
    virtual void HitHandlerA(); // slot 6
    virtual void HitHandlerB(); // slot 7
    virtual void HitHandlerC(); // slot 8
    virtual void HitHandlerD(); // slot 9
    virtual void Refresh();     // +0x28 (slot 10)
};
SIZE_UNKNOWN(CStatzSelf);

// The glyph map a changed value is resolved through (m_glyphMap for the first four
// values, m_timerGlyphMap for the timer value): a [m_minIndex..m_maxIndex]-gated
// table at m_glyphs.
struct CStatzGlyphMap {
    char m_pad0[0x14];
    i32* m_glyphs; // +0x14  glyph table
    char m_pad18[0x64 - 0x18];
    i32 m_minIndex; // +0x64  glyph-index range lo gate
    i32 m_maxIndex; // +0x68  glyph-index range hi gate
};
SIZE_UNKNOWN(CStatzGlyphMap);

// ---------------------------------------------------------------------------
// CSBI_StatzTabGruntBar - the per-grunt stat tab. Derives directly from
// CStatusBarItem (vtable @0x5eace4).
class CSBI_StatzTabGruntBar : public CStatusBarItem {
public:
    void Reset();      // 0xea470  drop the five tracked values (also the dtor teardown)
    i32 Poll(i32 arg); // 0xea4b0  Update + conditional vfunc-10 redraw (arg unused)
    i32 Update();      // 0xea6c0  resample the grunt and latch any changed value

    // ----- layout (offsets are the load-bearing fact) -----
    // base region m_0..0x2b comes from CStatusBarItem; leaf fields start at +0x30.
    char m_pad2c[0x30 - 0x2c];
    i32 m_statusGlyph;               // +0x30  status glyph
    i32 m_statusGlyphLatched;        // +0x34  status glyph (resolved by Update)
    i32 m_statusValue;               // +0x38  status value (tracked)
    i32 m_abilityGlyph;              // +0x3c  ability glyph
    i32 m_abilityGlyphLatched;       // +0x40  ability glyph (resolved by Update)
    i32 m_abilityValue;              // +0x44  ability value (tracked)
    i32 m_overrideGlyph;             // +0x48  override glyph
    i32 m_overrideGlyphLatched;      // +0x4c  override glyph (resolved by Update)
    i32 m_overrideValue;             // +0x50  override value (tracked)
    i32 m_selectKey;                 // +0x54  selection key (0 => skip selection sample)
    i32 m_selectGlyph;               // +0x58  selection glyph (resolved by Update)
    i32 m_selectValue;               // +0x5c  selection value (tracked)
    i32 m_unitRow;                   // +0x60  unit-table row index (stride 15 records)
    i32 m_unitCol;                   // +0x64  unit-table column index (within the 15-dword record)
    CStatzGlyphMap* m_timerGlyphMap; // +0x68  timer glyph map
    i32 m_timerGlyph;                // +0x6c  timer glyph (resolved by Update)
    i32 m_timerValue;                // +0x70  timer value (tracked)
    CStatzGlyphMap* m_glyphMap;      // +0x74  glyph map for the first four values
    i32 m_timerAnchorLo;             // +0x78  timer anchor lo (g_645588 at last bump)
    i32 m_timerAnchorHi;             // +0x7c  timer anchor hi
    i32 m_timerWindowLo;             // +0x80  timer window lo
    i32 m_timerWindowHi;             // +0x84  timer window hi
};
SIZE_UNKNOWN(CSBI_StatzTabGruntBar);

#endif // SBI_STATZTABGRUNTBAR_H
