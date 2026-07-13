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

#include <Gruntz/TriggerMgr.h> // CTriggerMgr + CTrigPoint (+ <Mfc.h> CObList/CByteArray)

#include <Gruntz/GruntzCmdMgr.h>  // CGruntzCmdMgr (the +0x6c command/report sub-mgr)
#include <Gruntz/SBI_RectOnly.h>  // CSBI_RectOnly (the world's +0x2dc status-bar item)
#include <Gruntz/SpriteFactory.h> // the ONE CSpriteFactory (CreateSprite @0x1597b0)
#include <Gruntz/TileGrid.h>      // canonical CTileGrid (the registry's +0x70 tile grid)
#include <Bute/ButeMgr.h>         // canonical CButeMgr (one shape)
#include <Gruntz/Viewport.h>      // shared world tile-grid geometry (dims here)
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

// The embedded MFC pointer-list (CObList @+0x240, base @+0, the ten +0x2d0 selection
// slots) is the real MFC CObList member (see <Gruntz/TriggerMgr.h>); the leaves
// call m_recList/m_baseList/m_selLists[i] methods directly (no this+offset cast).

// The level/group base-index sentinel (DAT_00644c54) the selection helpers guard on
// (same global the StatzTab toggle keys off; see StatusBarUpdaters.cpp / CPlay.h).
extern "C" i32 g_curPlayer;

// The global game-registry singleton (?g_gameReg@@3PAUWwdGameReg@@A @0x64556c). Only
// the +0x2c world back-ptr is read here; the world's hooks are reloc-masked.
// The status-bar item at world->m_2dc is the real CSBI_RectOnly (<Gruntz/SBI_RectOnly.h>,
// included above): SetMode @0x10bb90, TryActivate/Reset/Place/Run; the reset path reads its
// offset-0 subtype tag (*(i32*)), sub-state (m_activeTab @0x10c), busy flag (m_hlBusy @0x548)
// and frees its retab notifier (m_retabNotify @0x54c). The former CTmStatusItem flat view is
// gone - m_2dc is typed CSBI_RectOnly* so the method calls need no per-site cast.

// The booty/score sub-object at world->m_3f4 (booty & trigger modes): a running i64 score
// tally (m_38) plus the per-column status counters HitTestApply zeroes.
struct CTmScoreSub {
    char p0[0x30];
    i32 m_30; // +0x30
    i32 m_34; // +0x34
    i64 m_38; // +0x38  score tally (i64)
    i32 m_40; // +0x40
    i32 m_44; // +0x44
    i32 m_48; // +0x48
    i32 m_4c; // +0x4c
};

// The active game-state (g_gameReg->m_curState, a CPlay/CState) as the leaves view it: one
// unified shape. LoadCursorSprites (== the retail's StopFx, 0xd0120) loads/clears the pending
// cursor fx; the rest are the world refresh / stat / scroll / fx hooks. Reloc-masked.
struct CTmWorld {
    void LoadCursorSprites(i32 kind, i32 flag); // 0xd0120
    void Refresh();                             // 0xda2d0
    void SetStat(i32 a, i32 b);                 // 0xd9240
    void Center(i32 cx, i32 cy);                // 0xd5f00 (scroll-center on a tile)
    void StopFx2(i32 a, i32 b);                 // 0xd0b3a
    i32 OnRegion4(i32 z);                       // 0xd8bc0
    void Place2(i32 a, i32 b, i32 c);           // ReinitGroup state place (reloc-masked)
    char p0[0x2dc];
    CSBI_RectOnly* m_2dc; // +0x2dc  status-bar item (real class, no per-site cast)
    char p2e0[0x384 - 0x2e0];
    struct Anchor {
        i32 m_x;
        i32 m_y;
    } m_anchors[4]; // +0x384  fx anchors (stride 8)
    char p3a4[0x3f4 - 0x3a4];
    CTmScoreSub* m_3f4; // +0x3f4  booty/score sub-object
    char p3f8[0x504 - 0x3f8];
    i32 m_504; // +0x504  pending-fx flag (only null-tested)
};
// The level/plane grid the active-selection center reads its dims from: the chain
// g_gameReg->m_world->m_24->m_5c lands on the shared CViewport
// (<Gruntz/Viewport.h>) whose m_worldWidth/m_worldHeight are the (cols,rows) read here.
struct CTmGridHolder {
    void Snap(i32* outR, i32* outC); // ReinitGroup snap-to-cell (reloc-masked)
    char p0[0x5c];
    CViewport* m_5c; // +0x5c  the grid object
};
struct CTmRegSub30 {
    char p0[0x24];
    CTmGridHolder* m_24; // +0x24
};
// The tile occupancy grid at g_gameReg->m_tileGrid (+0x70) is the canonical CTileGrid
// (<Gruntz/TileGrid.h>): a row-pointer table (m_8), width (m_c), height (m_10).
// The fx/target sub-mgr at g_gameReg->m_68 (a reused per-mode slot; the fx TUs' "light-fx
// target"): its fx-sprite spawner (0x90b48) and its group-reset driver (0x79520). Both
// reloc-masked __thiscall bodies.

// ?g_buteMgr@@3VCButeMgr@@A @0x6453d8 - the canonical CButeMgr (via TriggerMgr.h);
// the int-with-default getter (0x1721e0) is reloc-masked __thiscall.
extern CButeMgr g_buteMgr;
extern "C" u32 g_645588; // DAT_00645588 (the level base score / id sentinel)
extern i32 g_644ca4;     // DAT_00644ca4 (the secondary group sentinel; serialized by ScanGroup)

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
//  <Gruntz/UserLogic.h> CGameObject and CGameObjAux - which already model exactly this:
//  "spr->m_7c->Init(spr) on the fresh CSpriteFactory::CreateSprite result", and
//  CGameObjAux::m_logic as the bound logic leaf. CreateSprite RETURNS CGameObject*, so the
//  (CTmCell*) casts on its result were wrong outright: the deleted CTmCell view had
//  conflated the SPRITE with the LOGIC the sprite carries.)
// The level view at level->m_24: ScreenToCell biases the input by its scroll origin - the
// view holds the origin (m_10/m_14) and the scroll object (m_5c), whose +0x40/+0x44 is the
// current scroll (x,y).
struct CTmLevelView {
    char p0[0x10];
    i32 m_10; // +0x10  view origin x
    i32 m_14; // +0x14  view origin y
    char p18[0x4c - 0x18];
    void** m_4c; // +0x4c  tile-class object table (cell id -> type object; PlaceObjectFull)
    char p50[0x5c - 0x50];
    CViewport* m_5c; // +0x5c  the plane viewport (real CViewport: tile grid + edge/scroll origin)
};

// The level object stored at CTriggerMgr+0x22c (set by SetLevel): its +0x8 is the sprite
// factory the spawners create from, +0x24 the level view.
struct CTmLevel {
    char p0[0x8];
    CSpriteFactory* m_8; // +0x08  sprite/object factory + live-object list holder
    char pc[0x24 - 0xc];
    CTmLevelView* m_24; // +0x24  the level view (scroll origin)
};

// The level's display-object list (level->m_8->m_liveObjects, the canonical
// CSpriteListNode chain): each node carries the next ptr @+0 and the bound object
// @+8. The object's type is identified by a fixed entry in its descriptor
// (obj+0x7c) slot-4 (+0x10) matching the CGrunt::ReadConfigFromButeMgr method
// address; on a match, +0x18 names the target whose +0x200 channel marker is cleared.
// The grid-cell object's ReadConfigFromButeMgr method address is the retail's type tag
// (DestroyAllAnims compares a level-list object's descriptor slot-4 against it, reloc-
// masked DIR32); &CTmCell::ReadConfigFromButeMgr carries that reloc.

// The puddle's placement target (sprite desc +0x18): a CUserLogic-ish object whose
// PlacePuddle(a,b,c,d) does the real placement (reloc-masked @0x9c3f0) and whose +0x38
// goal carries the +0x8 flags ORed with 0x10000 on failure. The (x,y,z) the record-list
// walk matches against live at +0x54/+0x58/+0x5c.
struct CTmPuddleTarget {
    i32 Place(i32 a, i32 b, i32 c, i32 d); // 0x9c3f0
    char p0[0x38];
    CTmGoal* m_38; // +0x38  goal object (its +0x8 is the flags word)
    char p1[0x54 - 0x3c];
    i32 m_54; // +0x54  match x
    i32 m_58; // +0x58  match y
    i32 m_5c; // +0x5c  busy flag
};
// The cell record nodes PlacePuddle walks: this+0x4 is the intrusive CPtrList head node,
// this+0xc its count. Each node carries the next ptr @+0 and the placed-object @+0x8.
struct CTmRecNode {
    CTmRecNode* m_next;     // +0x00
    char p0[0x4];           // +0x04
    CTmPuddleTarget* m_obj; // +0x08  placed object (the puddle target shape)
};

// The pending-fx sub-object at CTriggerMgr+0x2a0; its Pulse() is the reloc-masked thiscall.
struct CTmPendingFx {
    void Pulse(); // reloc-masked
};

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
extern i32 g_6455b0; // DAT_006455b0 (the alt-group gate)

// Class-metadata size annotations (all partial modeling views -> SIZE_UNKNOWN).
// Placed at end-of-TU: interspersed placement (right after each class) reschedules
// ResetGroup/HitTestCell codegen in this codegen-sensitive unit (measured -0.18/-0.02);
// end-of-TU (after all bodies) is matching-neutral.
SIZE_UNKNOWN(CTmNode);
SIZE_UNKNOWN(CTmDisplay);
SIZE_UNKNOWN(CTmCellConfig);
SIZE_UNKNOWN(CTmOverlay);
SIZE_UNKNOWN(CTmGoal);
SIZE_UNKNOWN(CTmWorld);
SIZE_UNKNOWN(CTmScoreSub);
SIZE_UNKNOWN(CTmGridHolder);
SIZE_UNKNOWN(CTmRegSub30);
SIZE_UNKNOWN(CTmNameReg);
SIZE_UNKNOWN(CTmLevel);
SIZE_UNKNOWN(CTmPuddleTarget);
SIZE_UNKNOWN(CTmRecNode);
SIZE_UNKNOWN(CTmCell);
SIZE_UNKNOWN(CTmPendingFx);
SIZE_UNKNOWN(CTmOverlaySrc);
SIZE_UNKNOWN(CTmCursorMgr);
SIZE_UNKNOWN(CTmScroll);
SIZE_UNKNOWN(CTmLevelView);

#endif // GRUNTZ_TRIGGERMGR_VIEWS_H
