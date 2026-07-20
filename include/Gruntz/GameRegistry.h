// GameRegistry.h - the one canonical shape of the global game-manager singleton
// (?g_gameReg@@3PAUWwdGameReg@@A, the WwdGameReg* at RVA 0x24556c / VA 0x64556c).
//
// SINGLETON IDENTITY (verified): the object at *0x24556c is the RTTI-confirmed
// CGruntzMgr (vftable ??_7CGruntzMgr@@6B@ @0x5e9b64), `new`'d by CGruntzApp::
// InitializeGameManager (@0x080a20, push 0xa30). CGameRegistry (this struct) and
// CGruntzMgr (<Gruntz/GruntzMgr.h>) are TWO VIEWS OF ONE OBJECT, proven by shared
// method RVAs (CGameRegistry::Ack == CGruntzMgr::ReportError, both @0x08dc60) and
// coincident slot meanings (+0x48 m_sound, m_2c m_curState, m_8c m_modeW: the
// mode-width cmp [reg+0x8c],0x280 (640) in RestoreVideoMode 0x08ddd0).
//
// WHY TWO HEADERS (a NECESSARY split, not a mistake): CGruntzMgr is an MFC class
// (`: public CGameMgr`, CString/CByteArray members) so <Gruntz/GruntzMgr.h>
// pulls <Mfc.h>/afx. THIS header is included by ~60 TUs, many of which are pure-
// Win32 (they `#include <Win32.h>` -> windows.h). afx forbids a prior windows.h
// (`C1189: MFC apps must not #include <windows.h>`), so this canonical view MUST
// stay MFC-free (a plain struct over <Ints.h>+<CTileGrid.h>). The two views cannot
// live in one header without a build break; the field DESCRIPTIONS below are kept
// reconciled with GruntzMgr's descriptive names so there is one agreed layout.
// See docs/vtable-conversion-log.md ("0x24556c dual-view: MFC/Win32 wall").
//
// This object was previously modeled ~20 different ways across the tree (CGameReg,
// WwdGameReg, WwdGameRegZ, CObjDropReg, CGmGameReg, TgcGameReg, ... one bespoke
// partial "view" struct per TU). The USER PRINCIPLE is: different layouts = a
// mistake; there is ONE real object. The leaf SCALAR fields (the ints below) are
// provably consistent across every TU's disasm and are named here.
//
// The SINGLE-TYPE sub-object pointers are typed here so their consumers reach them
// WITHOUT a per-site cast: m_2c (CState* current game-state), m_30 (the resource
// manager - CDDrawSurfaceMgr, the retail CDDrawSurfaceMgr: draw target + sprite factory
// + image registry + view + sound/anim), m_60 (cue sink), m_70 (tile grid). Sub-
// object TYPES defined in <Gruntz/ResMgr.h>/<Wwd/WwdFile.h> are forward-
// declared (not included) to keep this ~60-TU-wide header light.
//
// RESOLVED: +0x74 is CSpriteRefTable* (<Gruntz/SpriteRefTable.h>) - one object,
// RTTI/teardown-proven (Close tears it down via CSpriteRefTable::Reset @0xe2290; the
// GetByIndex/LoadSprite consumer facets are its GetSel/LoadSprite methods, all cast-free).
// RESOLVED: +0x68 is CTriggerMgr* (see the m_cmdGrid fwd-decl block below) - the ~10
// per-TU downcasts were all views of the ONE non-polymorphic CTriggerMgr, proven by
// shared method RVAs (HitTestCell 0x75af0, CellDispatch 0x6bcb0) + the +0x1c cell grid.
// The slots at 0x54/0x58/0x6c/0x78/0x7c are SUSPECT / UN-RECOVERED, NOT
// confirmed-authentic. Today each TU downcasts the void* to a different concrete type
// (+0x7c: a "draw object" in GameMode vs GruntPickupStats
// in the pickup loader). That per-TU divergence is a RED FLAG per no-sane-dev-test: a
// real CGruntzMgr field has ONE type, so "a different type per TU" almost always means
// the real single type - or the common base the mode objects derive - was never
// recovered, and the void*-plus-downcast is a reconstruction ARTIFACT, not a genuine
// per-mode union. These stay void* ONLY because the real type isn't recovered yet. The
// fix (do NOT defend the void*): trace what the retail CGruntzMgr ctor + per-mode init
// actually store at each offset - a single type, or a real base to derive - then type
// it here so consumers reach it cast-free, exactly like m_curState/m_world/m_tileGrid.
#ifndef GRUNTZ_GRUNTZ_CGAMEREGISTRY_H
#define GRUNTZ_GRUNTZ_CGAMEREGISTRY_H

#include <Ints.h>
// +0x30->+0x28 sound/anim cue registry (CSndHost). Included (not fwd-declared) as the
// lightweight Ints.h-only SoundCue.h: a full def does NOT count toward this ~60-TU
// header's file-scope forward-decl budget, so typing m_28 CSndHost* costs 0 fwd decls
// (see docs/patterns/header-fwd-decl-count-regalloc-butterfly.md - the include-as-lever).
#include <Gruntz/SoundCue.h>

class CDDrawChildGroup; // +0x30 -> +0x08 the object collection / sprite factory (CreateSprite)
namespace Utils {
    class RegistryHelper; // the +0x38 settings writer (<Utils/RegistryHelper.h>)
}
class CGruntSpawnConfig;
typedef CGruntSpawnConfig CGruntCueSink;    // +0x60 on-screen cue receiver; Grunt.h completes it (or, in
                        // the pure-Win32 grunt-step TUs that can't pull Grunt.h,
                        // completed locally with just the 0x4039f4 6-arg cue - a
                        // data-less method handle, so layout-neutral, no cross-cast)
class CState;           // +0x2c current game-state; CState.h completes it
class CWorldSoundSet;   // +0x54 active-level input/spatial-sound object (WorldSoundSet.h)
// +0x68 world command/trigger grid. RESOLVED -> CTriggerMgr (<Gruntz/TriggerMgr.h>):
// the ~10 per-TU downcasts (CTeleIconTable/CTriggerSink/CSbIconSet/CSlimeCueGate/
// CPathCueGate/TgcRegion/MgrObj/RbCmdGrid/...) are all VIEWS of the ONE non-polymorphic
// CTriggerMgr. Proven by shared non-virtual method RVAs: the 5-arg Probe/HitTestCell
// == CTriggerMgr::HitTestCell @0x75af0, the 4-arg ScrollTo/Strike/RbMarkRect ==
// CTriggerMgr::CellDispatch @0x6bcb0 (both FID-tagged ?...@CTriggerMgr@@), and the
// +0x1c 15-column grid every array-consumer indexes == CTriggerMgr::m_grid. Forward-
// declared (not included) to keep this ~60-TU header MFC-free (TriggerMgr.h pulls
// <Mfc.h>); consumers include TriggerMgr.h to reach methods cast-free.
class CTriggerMgr;
class CBattlezData; // +0x7c the HUD/score accumulator (BattlezData.h completes it)
// Sub-objects of the +0x30 resource manager, defined in <Gruntz/ResMgr.h> /
// <Wwd/WwdFile.h> (CPlaneRender); forward-declared here so consumers reach them typed
// (no per-site cast) without pulling those headers into this ~60-TU-wide view.
struct CDDrawSubMgrPages; // +0x30->+0x04 active draw surface (m_drawContext at +0x14)
// The image/name registry IS the canonical CDDrawWorkerRegistry
// (<DDrawMgr/DDrawWorkerRegistry.h>, real polymorphic; ex CWorkerVtableView).
class CDDrawWorkerRegistry;
typedef CDDrawWorkerRegistry CImageRegistry;
// +0x74 sprite/animation reference table (<Gruntz/SpriteRefTable.h>): GetSel(i,bAlt)
// resolves a kind slot to its sprite/frame pointer; LoadSprite(desc,flag) loads by
// descriptor. `new`'d in the game bootstrap (0x83450), torn down by CGruntzMgr::Close
// (CSpriteRefTable::Reset @0xe2290). Forward-declared to keep this ~60-TU view light.
class CSpriteRefTable;
// +0x78 per-frame light-FX / shade-table pump (<Gruntz/LightFxMgr.h>): Push(imgSet,
// anchor, slot) applies a chosen shade table; m_tables[10] (+0x14) is the effect->table
// array consumers index. `new`'d in the bootstrap (0x83450), torn down by Close
// (CLightFxMgr::Reset @0x9dc80). Forward-declared to keep this ~60-TU view light.
class CLightFxMgr;
// The +0x30 game resource/level manager (the retail "CDDrawSurfaceMgr"/world holder) IS the
// canonical CDDrawSurfaceMgr - one definition, included below. [FOLDED 2026-07-16:
// the game-side "CSpriteFactoryHolder" view (this header) and the loader-side
// "CDDrawSurfaceMgr" view (ResMgr.h) were the settled 2026-07-13 identity's two remaining
// shadows; the view members map 1:1 onto the canonical (renamed to the union of
// both sides' best names: m_drawTarget/m_childGroup/m_workerList/m_imageRegistry/
// m_workerCache/m_level/m_soundRegistry/m_animRegistry/m_hWnd/m_flags/m_lastError/
// m_callback).
// The old "must not emit two vtables" deferral is void: one class, one ??_7.
// Its +0x24 level object is the canonical CGameLevel (the ex `CGameViewport` facet:
// "PushView" was ?VisitVisible@CGameLevel@@ @0x15dc90, "SetClipRect"
// ?BuildAllPlanes@CGameLevel@@ @0x15da80; +0x10 is m_planeCtx, +0x5c m_mainPlane).]
#include <DDrawMgr/DDrawSurfaceMgr.h>

// The tile occupancy grid (*g_gameReg+0x70) is CTileGrid, in
// <Gruntz/TileGrid.h>. CGrunt::LoadEntranceConfig stamps the grunt's footprint
// into the cell occupied by (m_10->m_5c>>5, m_10->m_60>>5): sets/clears bit
// 0x20 in cell byte+3 and writes a packed (m_1ec<<8)|m_1f0 owner word into cell[1].
#include <Gruntz/TileGrid.h>

// One per-player focus/registry slot: the element of the +0x150 array (4 records
// of 0x238 bytes; == the retail CGruntzMgr m_options[4]). It is ONE real record
// whose fields are REUSED per game-mode - a same-struct field overlay, not a set
// of distinct types (unlike the +0x68/+0x7c object slots which hold genuinely
// different classes). The fields named here are the offsets the game-mode
// consumers read: the two arrival/active gates (+0x14, +0x20), the join/done/
// cleared round-state trio (+0x24/+0x28/+0x2c), the per-mode id/sound/key word at
// +0x0c, and the snapped focus position (+0x220/+0x224). The role of m_0c varies
// by mode (sound id in battlez, entity id in the exit trigger, a key pointer in
// the sprite loader) so it is a plain i32 the pointer-consumer reinterprets.
// (CFocusSlot is FOLDED: the +0x150 array element IS GruntzPlayer - the 6-way
// conflation's last open name (<Gruntz/GruntzPlayer.h>, MFC-side, holds the
// CString the Win32 view never could). Consumers access the array at the REAL
// type via g_gameReg (CGruntzMgr::m_options[4]).)

// PHANTOM PURGE (this batch): BuildLevelRezPath / LogError / RunModalDialog / GetRect /
// EnterModalUI are GONE from this view. Every one was a mangled name (?X@CGameRegistry@@..)
// that no obj and no .LIB can ever define - while the RVA each call actually targets is a
// REAL CGruntzMgr method (0x93d40 / 0x8ef10 / 0x90260 / 0x8e3a0 / 0x8ef10, read off the
// call sites' rel32). LogError turned out to BE EnterModalUI (one function, two fake
// names). Their callers (savegame, loadgamemenu, customworlddialog, drawdebugstats,
// groupops) now declare the singleton at its REAL type, CGruntzMgr* - which is legal in
// any TU (GruntzMgr.h already includes THIS header, and extern "C" gives the pointer one
// C symbol whatever type a TU picks). The rest of this view's methods are the same defect
// and go the same way as their callers convert; see the matcher report for the blocker
// (the sub-object views - CWorldZ vs CDDrawSurfaceMgr, CGruntzMapMgr vs CTileGrid -
// must be folded first for the field-heavy TUs).
class CShadeTableCache; // +0x50 shade-table cache (<DDrawMgr/ShadeTableCache.h>)
// THE STRUCT IS GONE (2026-07-20): every consumer reads the singleton at its real
// type (CGruntzMgr, <Gruntz/GruntzMgr.h>); the Win32-safe field mirror had no live
// user left. This header remains as the shared fwd-decl/type hub for its ~97
// includers (deleting it wholesale is a separate include-diet pass - the fwd-decl
// COUNT is /O2 type-table state, see header-fwd-decl-count-regalloc-butterfly).


#endif // GRUNTZ_GRUNTZ_CGAMEREGISTRY_H
