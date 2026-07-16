// TriggerMgrViews.h - the shared reconstruction views of the CTriggerMgr TU
// family (TriggerMgrGrid.cpp / TriggerMgrHitTest.cpp / TriggerMgr.cpp - the one
// retail `triggermgr` conflation split into its three original objs, see the
// per-file headers). The CTm* shapes below are the unit-wide placed-cell /
// level / world / registry views every leaf shares; they were TriggerMgr.cpp
// file-locals before the split. All are acknowledged reconstruction scaffolding
// over UNMATCHED engine classes (the fold worklist is in each struct's comment);
// only offsets + code bytes are load-bearing.
//
// The 0x64556c singleton is the REAL ::CGruntzMgr (its extern is declared per-TU). The
// former CTmGameReg / CTmScoreBoard fake views are DELETED: +0x6c is the real
// CGruntzCmdMgr, +0x70 the real CGruntzMapMgr, +0x7c the real CBattlezData (whose m_score/
// m_counts names were migrated off CTmScoreBoard). TileGridCommand.h no longer declares
// g_gameReg, so each TU picks the type it needs.

#ifndef GRUNTZ_TRIGGERMGR_VIEWS_H
#define GRUNTZ_TRIGGERMGR_VIEWS_H

#include <Gruntz/TriggerMgr.h> // CTriggerMgr + CTrigPoint (+ <Mfc.h> CPtrList/CByteArray)

#include <Gruntz/GameRegistry.h> // CSpriteFactoryHolder - m_level's REAL class (ex CTmLevel)
#include <Gruntz/GameLevel.h> // CGameLevel - the holder's +0x24 level (ex CTmLevelView/CTmGridHolder)
#include <Gruntz/GruntzCmdMgr.h>  // CGruntzCmdMgr (the +0x6c command/report sub-mgr)
#include <Gruntz/SoundCue.h>      // CSndHost (a typedef - never fwd-declare it): holder m_28
#include <Gruntz/StatusBarMgr.h>  // CStatusBarMgr (the world's +0x2dc status-bar item)
#include <Gruntz/SpriteFactory.h> // the ONE CSpriteFactory (CreateSprite @0x1597b0)
#include <Gruntz/TileGrid.h>      // canonical CTileGrid (the registry's +0x70 tile grid)
#include <Bute/ButeMgr.h>         // canonical CButeMgr (one shape)
#include <Wwd/WwdFile.h>          // CPlaneRender - the canonical plane (dims here)
#include <rva.h>

// The pending-fx sprite-id base: a cell's logic kind maps to its pending overlay-fx sprite
// id as (kind + kPendingFxIdBase), latched into m_pendingFxKind and handed to the world.
enum {
    kPendingFxIdBase = 0xc8
};

// A list node: { CTmNode* m_next; ; (x,y)* m_payload }. The payload is an (x,y)
// pair at +0/+4. Opaque otherwise. These are the record-list / selection-list nodes.
struct CTmNode {
    CTmNode* m_next; // +0x00
    i32 m_4;         // +0x04
    i32* m_payload;  // +0x08  -> { x@+0, y@+4 }
};

// A grid cell's config/type sub-object (cell->m_14): its +0x1c is the config-name id the
// name registry maps to a string. And the goal object (cell->m_154 / the manager's goal),
// whose +0x8 flags word gets the 0x10000 done-bit; full CTmGoal is defined below.
struct CTmGoal;
struct CTmNotifyHook; // a cell's opaque +0x368 notify hook (only null-tested)
struct CTmCellConfig {
    char p0[0x1c];
    i32 m_1c; // +0x1c  config-name id
};

// The display sub-object hung at a grid cell's +0x10: the world position (m_5c/m_60),
// the archive id (m_188) and the clickable/hit gate (m_198). Reached as cell->m_10.
struct CTmDisplay {
    char p0[0x5c];
    i32 m_5c; // +0x5c  world x
    i32 m_60; // +0x60  world y
    char p64[0x188 - 0x64];
    i32 m_188; // +0x188  archive/serialize id
    char p18c[0x198 - 0x18c];
    i32 m_198; // +0x198  clickable/hit gate
};

// (The CTmCell view is GONE. It was a second model of ::CGrunt - identity long proven -
//  and it is now a typedef in <Gruntz/TriggerMgr.h>, so every method the leaves dispatch
//  on a placed grid grunt binds to the REAL ?X@CGrunt@@ body instead of a phantom
//  ?X@CTmCell@@ that nothing on earth defines. What blocked this fold was believed to be a
//  "+0x120 phantom-gap layout bug" in CGrunt; that bug does not exist - see the note in
//  <Gruntz/TriggerMgr.h>. The view's proven-but-unnamed offsets (0x114/0x118/0x124,
//  0x884/0x888/0x88c) were migrated onto CGrunt, and its sub-object views folded onto
//  CGrunt's real typed members.)

// The goal object at CTriggerMgr+0x23c; ResetAll ORs 0x10000 into its +0x8 flags.

// The embedded MFC pointer-list (CPtrList @+0x240, base @+0, the ten +0x2d0 selection
// slots) is the real MFC CPtrList member (see <Gruntz/TriggerMgr.h>); the leaves
// call m_recList/m_baseList/m_selLists[i] methods directly (no this+offset cast).

// The level/group base-index sentinel (DAT_00644c54) the selection helpers guard on
// (same global the StatzTab toggle keys off; see StatusBarUpdaters.cpp / CPlay.h).
#include <Gruntz/CurPlayer.h> // g_curPlayer

// The global game-registry singleton (?g_gameReg@@3PAUWwdGameReg@@A @0x64556c). Only
// the +0x2c world back-ptr is read here; the world's hooks are reloc-masked.
// The status-bar item at world->m_2dc is the real CStatusBarMgr (<Gruntz/StatusBarMgr.h>,
// included above): SetMode @0x10bb90, TryActivate/Reset/Place/Run; the reset path reads its
// offset-0 subtype tag (*(i32*)), sub-state (m_activeTab @0x10c), busy flag (m_hlBusy @0x548)
// and frees its retab notifier (m_retabNotify @0x54c). The former CTmStatusItem flat view is
// gone - m_2dc is typed CStatusBarMgr* so the method calls need no per-site cast.

// (The `CTmWorld` FIELD view of g_gameReg->m_curState and its `CTmScoreSub` m_3f4
//  sub-object view are DISSOLVED, 2026-07-15: m_curState IS the canonical CPlay
//  (<Gruntz/Play.h>) - its methods were already folded (LoadCursorSprites/ArmSnapshot/
//  ResetGoals/OnRegion4/FlushPendingOps), and its FIELDS map member-for-member:
//  m_2dc==CPlay::m_guts (CStatusBarMgr), +0x384==CPlay::m_anchors[4], m_3f4==CPlay::
//  m_frameMarker (CTimer - the "score sub" was the frame-marker timer: m_30/m_34==
//  m_accumLo/Hi, m_38:m_3c==the i64 start stamp, m_48/m_4c==m_running/m_currentMs),
//  m_504==CPlay::m_dragEndNotify. Consumers cast m_curState to CPlay* and use the real
//  members cast-free.)
// (CTmGridHolder / CTmRegSub30 are GONE - they were g_gameReg->m_world (the canonical
//  CSpriteFactoryHolder) and its +0x24 CGameLevel; the "+0x5c grid object" is
//  m_mainPlane, and the ReinitGroup "Snap(&r,&c)" was ?WrapCoord@CDDrawWorkerHost@@
//  @0xa000 (thunk 0x295a) on that plane - already reconstructed in WwdFile.cpp.)
// The tile occupancy grid at g_gameReg->m_tileGrid (+0x70) is the canonical CTileGrid
// (<Gruntz/TileGrid.h>): a row-pointer table (m_8), width (m_c), height (m_10).
// The fx/target sub-mgr at g_gameReg->m_68 (a reused per-mode slot; the fx TUs' "light-fx
// target"): its fx-sprite spawner (0x90b48) and its group-reset driver (0x79520). Both
// reloc-masked __thiscall bodies.

// ?g_buteMgr@@3VCButeMgr@@A @0x6453d8 - the canonical CButeMgr (via TriggerMgr.h);
// the int-with-default getter (0x1721e0) is reloc-masked __thiscall.
extern CButeMgr g_buteMgr;
extern "C" u32 g_frameTime; // DAT_00645588 (the level base score / id sentinel)
// (g_644ca4 moved to its only user, TriggerMgr.cpp, where it can carry the DATA()
//  binding a header cannot: a DATA() in a header is ignored by the label pass, so
//  declaring it here bound it to NO retail address at all.)

// The DAT_006bf650 config-name registry (its method maps a sprite-type id to a config-name
// string; @0x6bf66c/@0x6bf670 are its node array + count). Reloc-masked.
struct CTmNameReg {
    char** Lookup(i32 id); // 0x46e0c0
};
extern CTmNameReg g_nameReg; // 0x6bf650
void Str_Free(void* node);   // CString teardown, 0x1b9b93

// A DirectSound channel helper (?StopAndRewind@DirectSoundMgr, @0x135380, __thiscall,
// reloc-masked); DestroyAllAnims rewinds three channels.

// The level's sprite factory (level->m_8) is the canonical CSpriteFactory
// (<Gruntz/SpriteFactory.h>): CreateSprite (@0x1597b0, reloc-masked) builds a sprite
// from a config key, and the factory owns the live display-object list at +0x14
// (m_liveObjects / CSpriteListNode). The created sprite is the shared CGameObject,
// cast to this TU's placed-object view CTmCell (the unit-wide B-view; its full fold
// onto CGameObject is deferred). The sprite carries a descriptor at +0x7c whose
// slot-4 (+0x10) is an Init thunk run on the fresh sprite.
// (CTmSprite / CTmSpriteDesc are GONE. They were duplicates of the canonical
//  <Gruntz/UserLogic.h> CGameObject and AnimWorkerObj - which already model exactly this:
//  "spr->m_7c->Init(spr) on the fresh CSpriteFactory::CreateSprite result", and
//  AnimWorkerObj::m_logic as the bound logic leaf. CreateSprite RETURNS CGameObject*, so the
//  (CTmCell*) casts on its result were wrong outright: the deleted CTmCell view had
//  conflated the SPRITE with the LOGIC the sprite carries.)
// (CTmLevel / CTmLevelView are GONE - DISSOLVED 2026-07-15 onto the canonicals.
//  CTmLevel WAS the world holder CSpriteFactoryHolder (<Gruntz/GameRegistry.h>): all
//  three members land on it at identical offsets AND names (m_8 CSpriteFactory* /
//  m_24 the level / m_28 CSndHost*). CTmLevelView WAS the canonical CGameLevel
//  (<Gruntz/GameLevel.h>): its "m_10/m_14 view origin" is m_planeCtx.minX/minY (the
//  LevelCoordRect at +0x10), its "+0x4c tile-class table" is m_imageSets' CObArray
//  data pointer (reach it via GetAt), and its m_5c is m_mainPlane (CLevelPlane ==
//  CPlaneRender == CDDrawWorkerHost - one typedef, so no cast). CTriggerMgr::m_level
//  is typed CSpriteFactoryHolder* in <Gruntz/TriggerMgr.h>.)

// The level's display-object list (level->m_8->m_liveObjects, the canonical
// CSpriteListNode chain): each node carries the next ptr @+0 and the bound object
// @+8. The object's type is identified by a fixed entry in its descriptor
// (obj+0x7c) slot-4 (+0x10) matching the CGrunt::ReadConfigFromButeMgr method
// address; on a match, +0x18 names the target whose +0x200 channel marker is cleared.
// The grid-cell object's ReadConfigFromButeMgr method address is the retail's type tag
// (DestroyAllAnims compares a level-list object's descriptor slot-4 against it, reloc-
// masked DIR32); &CTmCell::ReadConfigFromButeMgr carries that reloc.

// (CTmPuddleTarget is GONE - it was a SECOND view of the baseList element the canonical
//  CTmCandidate (<Gruntz/TriggerMgr.h>) already models: same +0x38 bound/goal object,
//  same +0x54/+0x58 grid pair, same +0x5c occupied gate. Its Place @0x9c3f0 moved there.)
// The cell record nodes PlacePuddle walks: this+0x4 is the intrusive CPtrList head node,
// this+0xc its count. Each node carries the next ptr @+0 and the placed-object @+0x8.
// (These are MFC CPtrList CNodes - next/prev/data - viewed with a typed data slot.)
struct CTmRecNode {
    CTmRecNode* m_next;  // +0x00
    char p0[0x4];        // +0x04
    CTmCandidate* m_obj; // +0x08  placed object (the baseList candidate element)
};

// (CTmPendingFx is GONE - the +0x2a0 object is the pending-fx GRUNT: its `Pulse()` was
//  ?ResolveDeathAnimation@CGrunt@@QAEHXZ @0x455f0 through ILT 0x3a1c at both call sites,
//  and the deserializer stores the looked-up sprite's m_7c->m_logic there. The member is
//  typed CTmCell* (== CGrunt) in <Gruntz/TriggerMgr.h>.)

// The overlay snapshot source `obj`: a sprite whose +0x2c / +0x30 vtable slots are the
// 8-byte field getters RebuildOverlay copies into the manager's three pose blocks. Modeled
// polymorphic so `src->GetA/GetB` lower to the retail `mov eax,[esi]; call [eax+0x2c/0x30]`
// virtual dispatch (GetA at slot 11=+0x2c, GetB at slot 12=+0x30); slots 0..10 are unused
// placeholders that only pin the vtable layout.
//
// NOT reparentable to a named sprite base: `obj` arrives as the opaque first arg (a0) of
// CGruntzMgr::BroadcastCmd (0x93460), a broadcast-command payload whose concrete class is
// not determinable from the reconstructed set - GetA/GetB are a generic 2-getter sprite
// interface, not a modeled shared base. Kept as a documented genuine interface view (the
// task's "else leave"); RebuildOverlay is byte-exact (100%) so a speculative retype would
// only risk it.
struct CTmOverlaySrc {
    virtual void vf00();
    virtual void vf01();
    virtual void vf02();
    virtual void vf03();
    virtual void vf04();
    virtual void vf05();
    virtual void vf06();
    virtual void vf07();
    virtual void vf08();
    virtual void vf09();
    virtual void vf10();
    virtual void GetA(void* dst, i32 n); // [11] vtbl +0x2c
    virtual void GetB(void* dst, i32 n); // [12] vtbl +0x30
};

// The world's report/spawn sub-mgrs ResetGroup dispatches through (gameReg+0x6c reporter,
// +0x68 fx-mgr, +0x60 cursor-mgr), all reloc-masked.
struct CTmCursorMgr {
    void Spawn(i32 a, i32 b, i32 c, i32 d, i32 e); // 0x90bf4 (gameReg+0x60)
};

// --- the megafn (FUN_6f2f0) leaf helpers, @identity-TODO (orphan COMDATs whose only
// caller is the ~21 KB unreconstructed megafunction; owner unrecovered). Homed here from
// TriggerMgrHitTest.cpp - their shapes belong in the family scaffolding header; the
// method bodies stay in that .cpp. ---

// 0x75a10: a 2-field setter (CPoint/CSize-style) that fills m_0/m_4 and returns this.
struct CPairXY {
    i32 m_0;
    i32 m_4;
    CPairXY* Set(i32 a, i32 b); // 0x75a10
};

// 0x75a40: a 2D grid lookup - bounds-check (x, y) against width/height, then return the
// first dword of the (0x1c-byte-stride) cell at rows[y][x]; out of bounds returns 1. The
// m_8/m_c/m_10 trio + the 0x1c cell stride are the SAME shape as canonical CTileGrid (a
// likely CTileGrid method); kept as the view pending the megafn's reconstruction, because
// respelling the [y][x] walk onto CTileGrid's i32* rows changes scaled-index codegen.
struct CGridCell {
    i32 m_0;
    char _pad[0x1c - 4];
};
struct CGridLookup {
    char _00[8];
    CGridCell** m_8;          // +0x08  rows
    i32 m_c;                  // +0x0c  width
    i32 m_10;                 // +0x10  height
    i32 Lookup(i32 x, i32 y); // 0x75a40
};

#include <Gruntz/TraitorMode.h> // g_traitorMode (DAT_006455b0, the alt-group gate)

// Class-metadata size annotations (all partial modeling views -> SIZE_UNKNOWN).
// Placed at end-of-TU: interspersed placement (right after each class) reschedules
// ResetGroup/HitTestCell codegen in this codegen-sensitive unit (measured -0.18/-0.02);
// end-of-TU (after all bodies) is matching-neutral.
SIZE_UNKNOWN(CTmNode);
SIZE_UNKNOWN(CTmDisplay);
SIZE_UNKNOWN(CTmCellConfig);
SIZE_UNKNOWN(CTmOverlay);
SIZE_UNKNOWN(CTmGoal);
SIZE_UNKNOWN(CTmNameReg);
SIZE_UNKNOWN(CTmRecNode);
SIZE_UNKNOWN(CTmCell);
SIZE_UNKNOWN(CTmOverlaySrc);
SIZE_UNKNOWN(CTmCursorMgr);
SIZE_UNKNOWN(CPairXY);
SIZE_UNKNOWN(CGridCell);
SIZE_UNKNOWN(CGridLookup);

#endif // GRUNTZ_TRIGGERMGR_VIEWS_H
