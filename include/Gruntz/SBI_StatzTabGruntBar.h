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
#include <Gruntz/SbRect.h>        // BuildMultiplayerTabStatusBar's by-value geometry rect
#include <Image/CImage.h>         // the glyph handles ARE CImage (RenderFrame @0x153790)

// BuildMultiplayerTabStatusBar's owner/config-host pair (pointers only - fwd-decl).
class CStatusBarMgr;
class CDDrawSurfaceMgr;

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

// The active drawable reached via g_gameReg->m_30->m_4: its +0x14 is the render
// context (the RenderFrame arg the Blit passes through). Same chain SBI_WellGoo's
// Tick uses (g_gameReg->m_30->m_4->m_14).
struct CStatzDrawable {
    char m_pad0[0x14];
    void* m_14; // +0x14  render context (RenderFrame arg0)
};
struct CStatzGameMgr {
    char m_pad0[0x04];
    CStatzDrawable* m_4; // +0x04  active drawable
};
SIZE_UNKNOWN(CStatzDrawable);
SIZE_UNKNOWN(CStatzGameMgr);

// The game-registry singleton (?g_gameReg@@3PAUWwdGameReg@@A @ VA 0x64556c). The
// +0x30 render-mgr chain (Blit's target) and the +0x68 unit-table base the Statz
// tab samples are modeled (rows of 15 dwords).
struct CStatzGameReg {
    char m_pad0[0x30];
    CStatzGameMgr* m_30; // +0x30  active game-mode/renderer
    char m_pad34[0x68 - 0x34];
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
// NOTE (proven, fold pending): this is the REAL CSprite (<Gruntz/Sprite.h>) - identical
// field for field. CSprite's m_frames CObArray starts at +0x10, so its m_pData lands at
// +0x14 == m_glyphs; its m_name is +0x24..0x64 == m_name below; its m_firstFrame/
// m_lastFrame are +0x64/+0x68 == m_minIndex/m_maxIndex. SBI_Image.cpp already writes this
// same (name -> Lookup -> gated index -> frame) idiom against the real CSprite. The
// identical `CRegTypeTable` in <Gruntz/SerialRecView.h> is the same view again.
// Dissolving all three onto CSprite is follow-up work (it also touches the
// CEventLoadRec/CTriggerLoadRec TUs that share SerialRecView.h).
struct CStatzGlyphMap {
    char m_pad0[0x14];
    CImage** m_glyphs; // +0x14  glyph table (frame handles)  [== CSprite::m_frames.m_pData]
    char m_pad18[0x24 - 0x18];
    // +0x24 registry name, extent 0x24..0x64 (bounded by m_minIndex): the key the glyph
    // map was Lookup'd under, which the slot-1 serialize's mode-4 leg strcpy's out.
    // Same offset AND same extent as CSprite::m_name.
    char m_name[0x64 - 0x24];
    i32 m_minIndex; // +0x64  glyph-index range lo gate  [== CSprite::m_firstFrame]
    i32 m_maxIndex; // +0x68  glyph-index range hi gate  [== CSprite::m_lastFrame]
};
SIZE_UNKNOWN(CStatzGlyphMap);

// ---------------------------------------------------------------------------
// CSBI_StatzTabGruntBar - the per-grunt stat tab. Derives directly from
// CStatusBarItem (vtable @0x5eace4).
class CSBI_StatzTabGruntBar : public CStatusBarItem {
public:
    // tag 6 (the Statz/Multiplayer per-grunt stat bar). The four tracked values start
    // "unset" (-1) so the first Update latches them. Store order preserved from the
    // retail new-site ctor fold.
    CSBI_StatzTabGruntBar() {
        m_timerAnchorLo = 0;
        m_timerWindowLo = 0;
        m_timerAnchorHi = 0;
        m_timerWindowHi = 0;
        m_8 = 6;
        m_statusGlyphLatched = 0;
        m_abilityGlyphLatched = 0;
        m_overrideGlyphLatched = 0;
        m_selectGlyph = 0;
        m_glyphMap = 0;
        m_statusGlyph = 0;
        m_abilityGlyph = 0;
        m_overrideGlyph = 0;
        m_selectKey = 0;
        m_overrideValue = -1;
        m_abilityValue = -1;
        m_statusValue = -1;
        m_selectValue = 0;
        m_timerGlyphMap = 0;
        m_timerValue = -1;
        m_timerGlyph = 0;
    }
    virtual ~CSBI_StatzTabGruntBar() OVERRIDE; // slot 0
    // slot 1 (vtbl 0x1eace4 thunk 0x11e0 -> 0xea990): the stat-bar serialize. Mode 7
    // resolves each glyph through the registry (name + gated index); mode 4 writes each
    // back by reverse-lookup; both tail-chain CStatusBarItem::SerializeFields.
    //
    // 0xea990 used to be claimed as `CGruntStateRec::Load` - a .cpp-local placeholder
    // class in GruntStateRec.cpp. It is THIS class's slot-1 body, proven four ways:
    //  1. ??_7CSBI_StatzTabGruntBar (0x1eace4) slot 1 -> thunk 0x11e0 -> 0xea990 (direct).
    //  2. the body tail-chains `call 0x1848` (CStatusBarItem::SerializeFields) with
    //     `mov ecx,ebp` = its OWN this -> a qualified base call -> `this` IS a
    //     CStatusBarItem. A free record-loader could not make that call.
    //  3. RVA band: 0xea990 sits inside this class's run (0xea1f0/0xea470/0xea4b0/
    //     0xea4e0/0xea6c0).
    //  4. the view's own shape: `char m_pad00[0x30]` (exactly the CStatusBarItem base
    //     subobject) then 19 fields whose ptr/int pattern matches this class's
    //     m_statusGlyph..m_glyphMap at every single offset.
    virtual i32 SerializeFields(CSerialArchive* s, i32 mode, i32 a2, i32 a3) OVERRIDE; // 0xea990
    virtual void SbiSlot3() OVERRIDE;          // slot 3
    virtual void SbiSlot4() OVERRIDE;          // slot 4
    virtual void SbiSlot5() OVERRIDE;          // slot 5

    // 0xea1f0: the stat bar's own "configure" (it derives straight from CStatusBarItem,
    // so there is no slot-11 SetupImage to override). Same owner/config-host pair as
    // SetupImage. Was `CSbTab::BuildMultiplayerTabStatusBar` - a view CONFLATING this
    // class with CSBI_GruntMachine - while the caller referenced it on the fabricated
    // CSbConfigItem base, so the call resolved to NO definition.
    i32 BuildMultiplayerTabStatusBar(
        CStatusBarMgr* owner,
        CDDrawSurfaceMgr* host,
        i32 p3,
        i32 p4,
        SbRect g,
        const char* key,
        i32 p10,
        i32 p11,
        i32 selMode
    ); // 0xea1f0

    void Reset();      // 0xea470  drop the five tracked values (also the dtor teardown)
    i32 Poll(i32 arg); // 0xea4b0  Update + conditional vfunc-10 redraw (arg unused)
    i32 Blit();        // 0xea4e0  draw the tab's background + value glyphs (slot +0x14)
    i32 Update();      // 0xea6c0  resample the grunt and latch any changed value

    // ----- layout (offsets are the load-bearing fact) -----
    // base region m_0..0x2f comes from CStatusBarItem (0x30, incl. m_2c); leaf fields
    // start at +0x30.
    CImage* m_statusGlyph;           // +0x30  status background glyph
    CImage* m_statusGlyphLatched;    // +0x34  status value glyph (resolved by Update)
    i32 m_statusValue;               // +0x38  status value (tracked)
    CImage* m_abilityGlyph;          // +0x3c  ability background glyph
    CImage* m_abilityGlyphLatched;   // +0x40  ability value glyph (resolved by Update)
    i32 m_abilityValue;              // +0x44  ability value (tracked)
    CImage* m_overrideGlyph;         // +0x48  override background glyph
    CImage* m_overrideGlyphLatched;  // +0x4c  override value glyph (resolved by Update)
    i32 m_overrideValue;             // +0x50  override value (tracked)
    CImage* m_selectKey;             // +0x54  selection background glyph (0 => no selection)
    CImage* m_selectGlyph;           // +0x58  selection value glyph (resolved by Update)
    i32 m_selectValue;               // +0x5c  selection value (tracked)
    i32 m_unitRow;                   // +0x60  unit-table row index (stride 15 records)
    i32 m_unitCol;                   // +0x64  unit-table column index (within the 15-dword record)
    CStatzGlyphMap* m_timerGlyphMap; // +0x68  timer glyph map
    CImage* m_timerGlyph;            // +0x6c  timer glyph (resolved by Update)
    i32 m_timerValue;                // +0x70  timer value (tracked)
    CStatzGlyphMap* m_glyphMap;      // +0x74  glyph map for the first four values
    i32 m_timerAnchorLo;             // +0x78  timer anchor lo (g_frameTime at last bump)
    i32 m_timerAnchorHi;             // +0x7c  timer anchor hi
    i32 m_timerWindowLo;             // +0x80  timer window lo
    i32 m_timerWindowHi;             // +0x84  timer window hi
};
SIZE_UNKNOWN(CSBI_StatzTabGruntBar);

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---
VTBL(CSBI_StatzTabGruntBar, 0x001eace4);

#endif // SBI_STATZTABGRUNTBAR_H
