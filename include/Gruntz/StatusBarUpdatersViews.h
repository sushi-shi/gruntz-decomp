// StatusBarUpdatersViews.h - engine-referent views the HUD status-bar updater TU
// (StatusBarUpdaters.cpp) drives. Moved here from the per-TU inline defs so each
// shape carries a single shared definition (matching-neutral: only touched
// members/methods are load-bearing; every call through them is reloc-masked).
#ifndef GRUNTZ_STATUSBAR_UPDATERS_VIEWS_H
#define GRUNTZ_STATUSBAR_UPDATERS_VIEWS_H

#include <Ints.h>
#include <rva.h>
class DSoundCloneInst; // the pooled cue player (ConfigureItem @0x1360d0; Dsndmgr/DirectSoundMgr.h)
#include <Gruntz/Sprite.h> // CSprite (frame-data value) + CMapStringToOb
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

// (The EngineLabelBacklog placeholder host + its CTabRec/CTabWidget/CDestructBlock/
// CStatzTabItem/CStatzTabSub/RegUnitTable satellite views are GONE, 2026-07-16:
// every body they hosted is a real CStatusBarMgr / CWarpStoneFly / CSBI_RectOnly
// method over the canonical layouts; the +0x68 'unit-record table' IS CTriggerMgr
// (its +0x1c slot array is m_grid), the +0x150 toggle item IS CSbiRect, and the
// +0x18c toggle sub IS CSbiStatObj (StatusBarMgr.h).)

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


// --- vtable catalog ---

#endif // GRUNTZ_STATUSBAR_UPDATERS_VIEWS_H
