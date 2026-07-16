// StatusBarUpdatersViews.h - engine-referent views the HUD status-bar updater TU
// (StatusBarUpdaters.cpp) drives. Only touched members/methods are load-bearing;
// every call through them is reloc-masked.
#ifndef GRUNTZ_STATUSBAR_UPDATERS_VIEWS_H
#define GRUNTZ_STATUSBAR_UPDATERS_VIEWS_H

#include <Ints.h>
#include <rva.h>
class DSoundCloneInst; // the pooled cue player (ConfigureItem @0x1360d0; Dsndmgr/DirectSoundMgr.h)
#include <Gruntz/Sprite.h> // CSprite (frame-data value) + CMapStringToOb
#include <Gruntz/SoundCue.h> // the ONE +0x28 status/cue holder (CSndHost)

// The status-bar item the named cue Lookup resolves IS the canonical LeafCue
// (<Gruntz/LeafCue.h>): the DSoundCloneInst ConfigureItem pushes into at m_10 (+0x10,
// == cueMgr), a draw-clock latch m_14 (+0x14, == drawClock) and a window width m_18
// (+0x18, == window). The former CStatusBarTab view was a duplicate of it, folded away.

// The +0x28 status-bar holder is the canonical CSndHost (SoundCue.h) - the former
// CStatusBarHolder/sprite-keyed copies were views of it (same +0x10 name->object map
// + the +0x30 emit gate; the cue TUs additionally read its +0x2c sound stream).

// (CRegHolder + CMapTileGrid are DISSOLVED, 2026-07-16: CRegHolder was
// g_gameReg->m_world == CState::m_c - the ONE CDDrawSurfaceMgr
// (<Gruntz/GameRegistry.h>): m_04==m_drawTarget, m_statusBar==m_28 (CSndHost), and
// m_tileHolder==m_24 (the canonical CGameLevel). CMapTileGrid was
// m_24->m_mainPlane - the canonical CLevelPlane/CDDrawWorkerHost
// (<DDrawMgr/DDrawWorkerHost.h>): m_cellState==m_tileGrid (+0x20),
// m_rowOffset==m_colOffsets (+0x24), the exact GetTileHandle table pair. The
// old "GameRegistry.h can't coexist with GruntzMgr.h" wall was stale -
// SplashState.cpp already included both. The tile-system notifier at registry
// +0x70 remains the canonical CTileGrid (<Gruntz/TileGrid.h>), Notify facet.)

// (The EngineLabelBacklog placeholder host + its CTabRec/CTabWidget/CDestructBlock/
// CStatzTabItem/CStatzTabSub/RegUnitTable satellite views are GONE, 2026-07-16:
// every body they hosted is a real CStatusBarMgr / CWarpStoneFly / CSBI_RectOnly
// method over the canonical layouts; the +0x68 'unit-record table' IS CTriggerMgr
// (its +0x1c slot array is m_grid), the +0x150 toggle item IS CSbiRect, and the
// +0x18c toggle sub IS CSbiStatObj (StatusBarMgr.h).)

// (CGrinderRect is DISSOLVED, 2026-07-16: the chip-grinder rect-target widget at
// CStatusBarMgr::m_extraNotify1 (+0x500) is a CSbiSlotPtr, and the +0x14..+0x20
// screen rect it stamps IS that class's m_rect14 - the same status-bar-item rect
// band CSbiRect carries as m_xLo..m_yHi (StatusBarMgr.h).)

// --- vtable catalog ---

#endif // GRUNTZ_STATUSBAR_UPDATERS_VIEWS_H
