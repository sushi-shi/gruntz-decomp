#ifndef SBI_STATZTABGRUNTBAR_H
#define SBI_STATZTABGRUNTBAR_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/StatusBarItem.h> // canonical frameless CStatusBarItem base (real RTTI base)
#include <Gruntz/SbRect.h>        // BuildMultiplayerTabStatusBar's by-value geometry rect
#include <Image/CImage.h>         // the glyph handles ARE CImage (RenderFrame @0x153790)

class CStatusBarMgr;
class CDDrawSurfaceMgr;

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
SIZE_UNKNOWN();

struct CStatzSelHost {};
SIZE_UNKNOWN();

struct CStatzDrawable {
    char m_pad0[0x14];
    void* m_14; // +0x14  render context (RenderFrame arg0)
};
SIZE_UNKNOWN();
struct CStatzGameMgr {
    char m_pad0[0x04];
    CStatzDrawable* m_4; // +0x04  active drawable
};
SIZE_UNKNOWN();

struct CStatzGameReg {
    char m_pad0[0x30];
    CStatzGameMgr* m_30; // +0x30  active game-mode/renderer
    char m_pad34[0x68 - 0x34];
    CStatzSelHost* m_unitTable; // +0x68  unit/record table base + selection host
};
SIZE_UNKNOWN();

class CDDrawWorker; // CSprite IS CDDrawWorker (<DDrawMgr/DDrawWorker.h>); the
typedef CDDrawWorker CSprite; // typedef repeats Sprite.h's - identical, so legal,

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
        m_kind = 6;
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
    CSprite* m_timerGlyphMap; // +0x68  timer glyph map (a frame-data CSprite)
    CImage* m_timerGlyph;     // +0x6c  timer glyph (resolved by Update)
    i32 m_timerValue;         // +0x70  timer value (tracked)
    CSprite* m_glyphMap;      // +0x74  glyph map for the first four values (a CSprite)
    i32 m_timerAnchorLo;             // +0x78  timer anchor lo (g_frameTime at last bump)
    i32 m_timerAnchorHi;             // +0x7c  timer anchor hi
    i32 m_timerWindowLo;             // +0x80  timer window lo
    i32 m_timerWindowHi;             // +0x84  timer window hi
};
SIZE_UNKNOWN();

VTBL(CSBI_StatzTabGruntBar, 0x001eace4);

#endif // SBI_STATZTABGRUNTBAR_H
