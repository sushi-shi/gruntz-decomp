// StatusBarUpdatersViews.h - engine-referent views the HUD status-bar updater TU
// (StatusBarUpdaters.cpp) drives, plus the EngineLabelBacklog host placeholder the
// updater bodies are defined on. Moved here from the per-TU inline defs so each
// shape carries a single shared definition (matching-neutral: only touched
// members/methods are load-bearing; every call through them is reloc-masked).
#ifndef GRUNTZ_STATUSBAR_UPDATERS_VIEWS_H
#define GRUNTZ_STATUSBAR_UPDATERS_VIEWS_H

#include <Ints.h>
#include <rva.h>
#include <Dsndmgr/DirectSoundMgr.h> // DSoundCloneInst (ConfigureItem @0x1360d0)
#include <Gruntz/Sprite.h>      // CSprite (frame-data value) + CMapStringToOb
#include <Gruntz/SoundCue.h> // the ONE +0x28 status/cue holder (CSndHost; folds the former CStatusBarHolder)

// The status-bar item the named cue Lookup resolves IS the canonical LeafCue
// (<Gruntz/LeafCue.h>): the DSoundCloneInst ConfigureItem pushes into at m_10 (+0x10,
// == cueMgr), a draw-clock latch m_14 (+0x14, == drawClock) and a window width m_18
// (+0x18, == window). The former CStatusBarTab view was a duplicate of it, folded away.

// The +0x28 status-bar holder is the canonical CSndHost (SoundCue.h) - the former
// CStatusBarHolder/sprite-keyed copies were views of it (same +0x10 name->object map
// + the +0x30 emit gate; the cue TUs additionally read its +0x2c sound stream).

// The map render grid reached via m_30->m_tileHolder->m_grid (two parallel tables: a cell
// state table at +0x20 and a row-offset table at +0x24). Distinct object from the
// registry +0x70 tile occupancy grid (CTileGrid, <Gruntz/TileGrid.h>).
SIZE_UNKNOWN(CMapTileGrid);
struct CMapTileGrid {
    char m_pad00[0x20];
    i32* m_cellState; // +0x20  cell-state table
    i32* m_rowOffset; // +0x24  row-offset table
};
// The tile-system notifier at registry +0x70 is the canonical CTileGrid
// (<Gruntz/TileGrid.h>), viewed through its Notify facet.
// The registry's world/resource holder (g_gameReg->m_world == CState::m_c, the ONE
// CSpriteFactoryHolder): the DDraw front/back pages (+0x04, read only by the level-
// preview driver), the tile-grid holder (+0x24 -> +0x5c grid) and the status-bar
// holder (+0x28). LevelPreview's former PreviewMgr was a second view of it - folded
// here. (Deeper fold onto the canonical CSpriteFactoryHolder / CSndHost pending.)
struct CDDrawSubMgrPages; // +0x04 DDraw front/back page pair (LevelPreview render facet)
struct CRegHolder {
    char m_pad00[0x04];
    CDDrawSubMgrPages* m_04; // +0x04  DDraw front/back pages (preview surface)
    char m_pad08[0x24 - 0x08];
    struct M24 {
        char m_pad00[0x5c];
        CMapTileGrid* m_grid;
    }* m_tileHolder;       // +0x24 -> +0x5c grid
    CSndHost* m_statusBar; // +0x28  the +0x28 cue/status holder (canonical CSndHost)
};
SIZE_UNKNOWN(CRegHolder);

// EngineLabelBacklog - the placeholder class the backlog stubs hang off (modeled
// as a class so the member-fn mangling falls out, mirroring IconLoaders.cpp).
// The status-bar updaters live on the big game-mode object: a 5-slot array of
// 0x18-byte tab records at +0x220 with a parallel pointer array at +0x204.
struct CTabRec {
    i32 m_state;   // +0x00  state (1 active, 2 finished)
    i32 m_frame;   // +0x04  last animation frame index
    u32 m_startLo; // +0x08  start-clock lo
    u32 m_startHi; // +0x0c  start-clock hi
    char m_pad10[0x18 - 0x10];
};
SIZE_UNKNOWN(CTabRec);

// The parallel widget objects with a thiscall virtual at vtbl+0x30 (set-frame,
// slot 12). Modeled with a padded virtual interface so `call [vtbl+0x30]` falls
// out as a __thiscall (this=ecx, one stack arg).
struct CTabWidget {
    virtual void s00();
    virtual void s01();
    virtual void s02();
    virtual void s03();
    virtual void s04();
    virtual void s05();
    virtual void s06();
    virtual void s07();
    virtual void s08();
    virtual void s09();
    virtual void s0a();
    virtual void s0b();
    virtual void SetFrame(i32 frame); // slot 12 (+0x30)
    virtual void VtSlotFill0();       // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill1();       // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill2();       // vtable-slot filler (real slot; declared-only)
};
SIZE_UNKNOWN(CTabWidget);

// The destruct-button countdown block on the game-mode object (overlaid at
// +0x558): a 3-state machine (0 idle / 1 warning-down / 2 warning-up) with a
// frame counter, a 64-bit retrigger clock, the warning delay, and the widget.
struct CDestructBlock {
    i32 m_state;          // +0x558  state (0/1/2)
    i32 m_frame;          // +0x55c  frame counter
    u32 m_retriggerLo;    // +0x560  retrigger-clock lo
    u32 m_retriggerHi;    // +0x564  retrigger-clock hi
    u32 m_delayLo;        // +0x568  warning delay lo
    u32 m_delayHi;        // +0x56c  warning delay hi (always 0)
    CTabWidget* m_widget; // +0x570  the warning widget
};
SIZE_UNKNOWN(CDestructBlock);

// The chip-grinder rect-target widget at m_500: its +0x14..+0x20 screen rect is
// stamped each step from the scroll origin (m_10/m_14) and the grinder extents.
struct CGrinderRect {
    char m_pad00[0x14];
    i32 m_left;   // +0x14  left
    i32 m_top;    // +0x18  top
    i32 m_right;  // +0x1c  right
    i32 m_bottom; // +0x20  bottom
};
SIZE_UNKNOWN(CGrinderRect);

// The per-tab toggle item reached through this[idx*4 + 0x150]: SetField0 stamps
// the toggle value (+0x44), m_4 is its active flag.
struct CStatzTabItem {
    char m_pad00[0x4];
    i32 m_active; // +0x04  active flag
    char m_pad08[0x44 - 0x08];
    i32 m_toggleValue; // +0x44  toggle value
};
SIZE_UNKNOWN(CStatzTabItem);

class EngineLabelBacklog {
public:
    // (UpdateGruntOven/ChipGrinder/DestructButton StatusBar + ChipGrinderFinishStep
    // folded off this fake host onto their real owner CSBI_RectOnly - see SBI_RectOnly.cpp.)
    i32 LoadStatzTabToggleSprite(i32 value, i32 idx);
    // (LoadSwitchDownSprite/UpSprite @0x110570/0x1106b0 re-homed onto their real owner
    // CTileTriggerSwitchLogic::SwitchDown/SwitchUp - see StatusBarUpdaters.cpp / TileTriggerSwitchLogic.h.)
    i32 UpdateWarpStoneStatusBar(i32 a0, i32 phase, i32 srcX, i32 srcY);
    i32 LoadExplosionSprites(i32 a, i32 b, i32 c, i32 d); // 0x... (time-bomb tile move-at)

    // The switch tile-trigger object: m_switchTileX/m_switchTileY are its grid
    // coords, m_switchState its down(1)/up(0) flag.
    char m_pad00[0x8];
    i32 m_switchTileX; // +0x08  tile X
    i32 m_switchTileY; // +0x0c  tile Y
    char m_pad10[0x14 - 0x10];
    i32 m_switchState; // +0x14  down/up state
    char m_pad18[0x204 - 0x18];
    CTabWidget* m_slots[5]; // +0x204  the 5 grunt-oven tab widgets
    char m_pad218[0x220 - 0x218];
    CTabRec m_tabs[5]; // +0x220  the 5 cooking tabs
    char m_pad298[0x558 - 0x298];
    CDestructBlock m_destruct; // +0x558  the destruct-button block
};

// The toggle's sub-helper reached through this[idx*4 + 0x18c] (thunk_FUN_004ea170,
// external/reloc-masked); __thiscall, two args (this->m_state and the active flag).
struct CStatzTabSub {
    void Toggle(i32 stateId, i32 on);
};
SIZE_UNKNOWN(CStatzTabSub);

// The registry's per-frame unit-record table (g_gameReg->m_68, poly slot): a flat
// array of grunt-record pointers at +0x1c indexed by (idx + 15*player).
struct RegUnitTable {
    char m_pad0[0x1c];
    void* m_slots[1]; // +0x1c  per-cell record pointers
};
SIZE_UNKNOWN(RegUnitTable);

// --- vtable catalog ---

#endif // GRUNTZ_STATUSBAR_UPDATERS_VIEWS_H
