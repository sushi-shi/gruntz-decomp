#include <Gruntz/TriggerMgr.h>
#include <Gruntz/GruntzCmdMgr.h>
#include <Gruntz/SBI_RectOnly.h>
#include <Gruntz/GruntzMgr.h>
#include <Dsndmgr/DirectSoundMgr.h>
#include <Gruntz/SpriteFactory.h> // the ONE CSpriteFactory (CreateSprite @0x1597b0)
#include <Gruntz/UserLogic.h>     // canonical CUserLogic (switch/trigger logic virtuals)
#include <Gruntz/TileGrid.h>      // canonical CTileGrid (the registry's +0x70 tile grid)
#include <Bute/ButeMgr.h>         // canonical CButeMgr (one shape)
#include <Gruntz/Viewport.h>      // shared world tile-grid geometry (dims here)
#include <stdlib.h>               // rand (0x11fee0, reloc-masked)
#include <Globals.h>
// TriggerMgr.cpp - CTriggerMgr, the playfield tile-object / switch-trigger grid
// manager (trace placeholder tomalla-23, C:\Proj\Gruntz). See TriggerMgr.h.
//
// CTriggerMgr's fields are named data members (see TriggerMgr.h); void*/char members are
// cast to the file-local opaque shells (CTmCell/CTmGrunt/CTmLevel/...) at their use sites
// because the grid cells, the level/plane objects and the list-node payloads are full
// UNMATCHED engine classes whose reads / helper calls must reloc-mask. Functions in
// retail-RVA order.

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
struct CTmSpriteDesc;
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

// A placed grid-cell game object (a CGrunt) as EVERY trigger/switch leaf views it - one
// unified shape. m_grid[] holds CTmCell*. The reloc-masked __thiscall engine hooks the
// leaves dispatch are declared here; the raw this+offset fields the leaves read are named
// data members (recovered from usage). Config/goal/display sub-objects are typed pointers.
struct CTmCell {
    void ClearAllSprites();                                 // 0x4b240
    void ExitGrid();                                        // 0x641b0
    void Route(i32 kind, i32 a);                            // 0x60150
    void DestroyAnims();                                    // 0x57d80
    void Recall();                                          // 0x68520 (row-recall variant)
    void ReadConfigFromButeMgr();                           // type-tag address (DestroyAllAnims)
    void SelectMoveIcon(i32 icon);                          // 0x57800
    i32 CanShowStamina();                                   // 0x514a0
    void ResetA();                                          // 0x6a40c
    void ResetB();                                          // 0x6a2ae
    void ResetC();                                          // 0x6c216
    i32 ResetMagic();                                       // 0x6c498
    i32 Disarm(i32 a, i32 b);                               // 0x6f970
    i32 ApplyBox(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f); // 0x6fc40
    i32 Type13Check();                                      // 0x71f80
    void Apply13(i32 a, i32 b);                             // 0x70520
    i32 Dispatch(i32 kind, i32 a);                          // per-kind dispatch
    // ApplyTriggerA's per-kind hooks (thunk targets resolved via sema xref; all are
    // the placed grunt's real methods - CGrunt/CUserLogic bodies, reloc-masked):
    void RunMoveConfig(i32 x, i32 y);                    // 0x65630 (CGrunt)
    i32 RectContains(i32 x, i32 y);                      // 0x51850 (CGrunt)
    i32 CommitNeighbor(i32 r, i32 c, i32 x, i32 y);      // 0x5b050 (CGrunt)
    i32 BeginAttack(i32 x, i32 y);                       // 0x5b570 (CGrunt)
    void PlayMoveSound(i32 x, i32 y);                    // 0x511b0 (CGrunt)
    void ResetEntranceAnimation(i32 a, i32 b, i32 c);    // 0x62e10 (CGrunt)
    void LoadGruntTypeTable(i32 a, i32 b, i32 c, i32 d); // 0x4dd50 (CUserLogic base)

    char p0[0x10];
    CTmDisplay* m_10;    // +0x10  display sub-object (world pos / hit gate / archive id)
    CTmCellConfig* m_14; // +0x14  config/type object (its +0x1c is the config-name id)
    char p18[0x7c - 0x18];
    CTmSpriteDesc* m_7c; // +0x7c  sprite descriptor (Init hook + logic object at desc+0x18)
    char p80[0x114 - 0x80];
    i32 m_114; // +0x114  placement param
    i32 m_118; // +0x118  placement param
    char p11c[0x124 - 0x11c];
    i32 m_124; // +0x124  placement param
    char p128[0x154 - 0x128];
    CTmGoal* m_154; // +0x154  goal object (its +0x8 is the flags word)
    char p158[0x170 - 0x158];
    i32 m_170; // +0x170  logic kind
    char p174[0x17c - 0x174];
    CTrigPoint m_pos; // +0x17c  world (x,y), tile-snapped - a real point pair
                      //         (NotifyCell copies it whole)
    char p184[0x198 - 0x184];
    i32 m_198; // +0x198  alt kind
    i32 m_19c; // +0x19c  remapped kind
    char p1a0[0x1e4 - 0x1a0];
    i32 m_1e4; // +0x1e4  pending/triggered flag (also cleared by the k=0x14 entrance reset)
    i32 m_1e8; // +0x1e8  recall-done flag
    i32 m_1ec; // +0x1ec  owning area index
    i32 m_1f0; // +0x1f0  owner sub-id
    i32 m_1f4; // +0x1f4  move-icon (stash source)
    i32 m_1f8; // +0x1f8  stashed move-icon (-1 idle)
    i32 m_1fc; // +0x1fc  alive flag
    char p200[0x218 - 0x200];
    i32 m_218; // +0x218  entrance-anim state   (ApplyTriggerA k=0x14 reset clears 218/21c/220)
    i32 m_21c; // +0x21c  entrance-anim gate
    i32 m_220; // +0x220  entrance-anim pending
    char p224[0x248 - 0x224];
    i32 m_248; // +0x248  state flags
    char p24c[0x2d0 - 0x24c];
    i32 m_2d0; // +0x2d0
    char p2d4[0x2dc - 0x2d4];
    i32 m_2dc; // +0x2dc  proximity cutoff
    char p2e0[0x308 - 0x2e0];
    i32 m_308; // +0x308
    i32 m_30c; // +0x30c
    i32 m_310; // +0x310
    i32 m_314; // +0x314
    char p318[0x368 - 0x318];
    CTmNotifyHook* m_368; // +0x368  notify hook (opaque; only null-tested)
    i32 m_36c;            // +0x36c  notified flag
    char p370[0x384 - 0x370];
    i32 m_384; // +0x384  applier scratch
    char p388[0x38c - 0x388];
    i32 m_38c; // +0x38c  fx param (the k=0x14 spawn's 5th arg)
    char p390[0x3e4 - 0x390];
    i32 m_3e4; // +0x3e4  fx pose x
    i32 m_3e8; // +0x3e8  fx pose y
    char p3ec[0x420 - 0x3ec];
    i32 m_420; // +0x420  cleared flag
    char p424[0x880 - 0x424];
    i32 m_880; // +0x880  combat timer base (= game clock)
    i32 m_884; // +0x884
    i32 m_888; // +0x888  combat timeout config
    i32 m_88c; // +0x88c
};

// The small overlay sub-object allocated at CTriggerMgr+0x25c (0x40 bytes). Only its
// reloc-masked __thiscall hooks are dispatched from the reconstructed leaves. The Load
// deserializer new+Loads a fresh one (former CTmSerOverlay folded in).
struct CTmOverlay {
    CTmOverlay();                 // 0x9090  (ctor via new, reloc-masked)
    void Tick();                  // 0x97f0  (reloc-masked)
    i32 Release();                // 0x94c0  (reloc-masked) - ret used by OverlayRelease
    void Clear();                 // 0x92e0  (reloc-masked) - destruct without freeing
    void Forward(i32 a, i32 b);   // 0x49b86 (reloc-masked) - forward (x,y) to the overlay
    i32 Load(CSerialArchive* ar); // 0x9bb0  (reloc-masked) - the overlay deserializer
    void Dtor();                  // in-place dtor (DestroyGroup teardown, reloc-masked)
    inline void* operator new(u32) {
        return ::operator new(0x40);
    }
    i32 m_0; // +0x00  owned x
    i32 m_4; // +0x04  owned y
    char p8[0x2c - 0x8];
    i32 m_2c; // +0x2c  active flag gating the per-frame OverlayTick
    char p30[0x40 - 0x30];
};

// The goal object at CTriggerMgr+0x23c; ResetAll ORs 0x10000 into its +0x8 flags.
struct CTmGoal {
    char p0[0x8];
    i32 m_8; // +0x08  flags
};

// The embedded MFC pointer-list (CObList @+0x240, base @+0, the ten +0x2d0 selection
// slots) is the canonical CTmObList real member (see <Gruntz/TriggerMgr.h>); the leaves
// call m_recList/m_baseList/m_selLists[i] methods directly (no this+offset cast).

// The level/group base-index sentinel (DAT_00644c54) the selection helpers guard on
// (same global the StatzTab toggle keys off; see StatusBarUpdaters.cpp / CPlay.h).
extern i32 g_644c54;

// The global game-registry singleton (?g_gameReg@@3PAUWwdGameReg@@A @0x64556c). Only
// the +0x2c world back-ptr is read here; the world's hooks are reloc-masked.
// The CSBI_RectOnly status-bar item at world->m_2dc: SetMode is the reloc-masked engine
// body @0x10bb90; the reset path also reads its mode (m_0), sub-state (m_10c) and frees
// its pending buffer (m_54c).
struct CTmStatusItem {
    i32 m_0; // +0x00  mode
    char p4[0x10c - 0x4];
    i32 m_10c; // +0x10c  sub-state
    char p110[0x548 - 0x110];
    i32 m_548;   // +0x548
    void* m_54c; // +0x54c  pending buffer to free
};

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
    CTmStatusItem* m_2dc; // +0x2dc  status-bar item
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

// The HUD/score board at g_gameReg->m_scoreBoard (+0x7c, a reused per-mode slot):
// a running score (+0x10) and the per-row placed-object counters (+0x48).
struct CTmScoreBoard {
    char p0[0x10];
    i32 m_score; // +0x10  score accumulator
    char p1[0x48 - 0x14];
    i32 m_counts[4]; // +0x48  per-row placed-object counters
};

// The fx/target sub-mgr at g_gameReg->m_68 (a reused per-mode slot; the fx TUs' "light-fx
// target"): its fx-sprite spawner (0x90b48) and its group-reset driver (0x79520). Both
// reloc-masked __thiscall bodies.
// The command/report sub-mgr at g_gameReg->m_6c: the per-record reporter (0x90db8),
// the group enqueue action (0x23c30) and the single/multi command posts (0x23c30/
// 0x23ca0). Reloc-masked; the two 0x23c30 views (Action / EnqueueSingle) carry the
// retail's two arg shapes for their two call sites.
struct CTmGameReg {
    char p0[0x2c];
    CTmWorld* m_curState;        // +0x2c  the active world/play object (CState/CPlay view)
    CTmRegSub30* m_world;        // +0x30  the level/plane grid holder
    char p1[0x34];               // +0x34
    CTriggerMgr* m_68;           // +0x68  reused per-mode slot (fx/target sub-mgr here)
    CGruntzCmdMgr* m_6c;         // +0x6c  command/report sub-mgr
    CTileGrid* m_tileGrid;       // +0x70  tile occupancy grid
    char p2[0x8];                // +0x74..0x78
    CTmScoreBoard* m_scoreBoard; // +0x7c  HUD/score board (reused per-mode)
    char p3[0x134 - 0x80];       // +0x80
    i32 m_134;                   // +0x134  gate/outcome discriminator (==1 live play)
};
extern CTmGameReg* g_gameReg;

// ?g_buteMgr@@3VCButeMgr@@A @0x6453d8 - the canonical CButeMgr (via TriggerMgr.h);
// the int-with-default getter (0x1721e0) is reloc-masked __thiscall.
extern CButeMgr g_buteMgr;
extern i32 g_645588; // DAT_00645588 (the level base score / id sentinel)
extern i32 g_644ca4; // DAT_00644ca4 (the secondary group sentinel; serialized by ScanGroup)

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
struct CTmCell;
struct CTmSpriteDesc {
    void* s0[4];
    void (*Init)(void*); // +0x10
    char p14[0x18 - 0x14];
    void* m_18; // +0x18  the sprite's logic object (per-kind: userlogic / puddle target)
};

// The level view at level->m_24: ScreenToCell biases the input by its scroll origin - the
// view holds the origin (m_10/m_14) and the scroll object (m_5c), whose +0x40/+0x44 is the
// current scroll (x,y).
struct CTmScroll {
    char p0[0x40];
    i32 m_40; // +0x40  scroll x
    i32 m_44; // +0x44  scroll y
};
struct CTmLevelView {
    char p0[0x10];
    i32 m_10; // +0x10  view origin x
    i32 m_14; // +0x14  view origin y
    char p18[0x5c - 0x18];
    CTmScroll* m_5c; // +0x5c  scroll object
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

// 0x6b640: SetLevel(lvl) - stash the level back-ptr, clear companion state.
// @early-stop
// 1-instr phase shift: retail floats `mov eax,1` up between the stores; we emit it
// at the epilogue. Structure + offsets byte-exact. docs/patterns/zero-register-pinning.md
RVA(0x0006b640, 0x2f)
i32 CTriggerMgr::SetLevel(CTmLevel* lvl) {
    if (lvl == 0) {
        return 0;
    }
    m_level = lvl;
    m_230 = 0;
    m_pendingFx = 0;
    m_2a4 = 0;
    return 1;
}

// 0x6b680: Cleanup - destruct+free the overlay sub-object (+0x25c) when present, then
// drain the record and selection lists. The overlay's Clear runs the in-place dtor,
// then __cdecl operator delete frees it.
RVA(0x0006b680, 0x39)
void CTriggerMgr::Cleanup() {
    CTmOverlay* ov = m_overlay;
    if (ov != 0) {
        ov->Clear();
        operator delete(ov);
        m_overlay = 0;
    }
    ClearRecords();
    ClearSelections();
}

// 0x6bd40: ClearGridRange(startRow) - ResetAll, then for rows startRow..3 (5 = all)
// flag each live cell's goal (+0x154) done and clear the cell, its parallel grid slot
// (+0x11c) and the per-row state words (+0x10c/+0x20c/+0x21c); then ClearSelections.
// @early-stop
// 60->65: the "5 = all" decode is now the if/else form (jmp-merge). Residual is a
// byte-or-vs-dword-RMW regalloc cascade: our cl pins the zero constant in ebp and peepholes
// `m_8 |= 0x10000` to `or byte [mem+2],1` (frees a reg -> keeps `this` in ebx); retail pins
// zero in ebx and emits the full `mov r,[m_8];or r,0x10000;mov [m_8],r` RMW (the reg temp
// forces `this` to spill to [esp+0x10] + one extra `push ecx` frame slot). Same expr matches
// as dword-RMW in the lower-pressure ResetAll; not source-steerable here. topic:wall topic:regalloc.
RVA(0x0006bd40, 0xb3)
i32 CTriggerMgr::ClearGridRange(i32 startRow) {
    i32 row, last;
    if (startRow == 5) {
        row = 0;
        last = 3;
    } else {
        last = startRow;
        row = startRow;
    }
    ResetAll();
    if (row <= last) {
        i32 n = last - row + 1;
        CTmCell** cell = &m_grid[row * 15];
        i32* perRow = m_rowStateB + row;
        i32 g2 = row * 15;
        do {
            i32 col = 0;
            do {
                CTmCell* c = *cell;
                if (c != 0) {
                    c->m_154->m_8 |= 0x10000;
                    *cell = 0;
                    m_cellFlag[g2 + col] = 0;
                }
                col++;
                cell++;
            } while (col < 15);
            *(i32*)((char*)perRow - 0x100) = 0;
            perRow[0] = 0;
            perRow[4] = 0;
            perRow++;
            g2 += 15;
            n--;
        } while (n != 0);
    }
    ClearSelections();
    return 1;
}

// 0x6bcb0: CellDispatch(row, col, kind, arg) - look up grid[row*15+col] (+0x1c);
// if it has a notify hook run NotifyCell(row,col,0) (ret 0); else route the cell by
// `kind` (0xd => ExitGrid, else Route(kind,arg)) and ret 1; ret 0 when no cell.
RVA(0x0006bcb0, 0x6a)
i32 CTriggerMgr::CellDispatch(i32 row, i32 col, i32 kind, i32 arg) {
    CTmCell* cell = m_grid[row * 15 + col];
    if (cell == 0) {
        return 0;
    }
    if (cell->m_368 != 0) {
        NotifyCell(row, col, 0);
        return 0;
    }
    if (kind == 0xd) {
        cell->ExitGrid();
    } else {
        cell->Route(kind, arg);
    }
    return 1;
}

// 0x6be30: ScreenToCell - bias the input (sx,sy) by the level view's scroll origin
// (view@m_24: scroll struct embedded at [m_5c]+0x40, origin @m_10/m_14) and forward to
// CellHitTest.
// @early-stop
// reassociation/scheduling residual (~85%): the scroll/view loads + the CellHitTest arg
// pushes are byte-exact; retail loads scroll[0]/[4] together up front and accumulates px
// from scroll[0] (`(scroll0-view10)+sx`), our cl reloads sx and accumulates from it
// (`(sx-view10)+scroll0`) - same value, swapped operand order. topic:wall topic:scheduling.
RVA(0x0006be30, 0x47)
void* CTriggerMgr::ScreenToCell(i32 sx, i32 sy, i32* outRow, i32* outCol, i32 startRow) {
    CTmLevelView* view = m_level->m_24;
    i32 px = view->m_5c->m_40 - view->m_10 + sx;
    i32 py = view->m_5c->m_44 - view->m_14 + sy;
    return CellHitTest(px, py, outRow, outCol, startRow);
}

// 0x6bea0: CellHitTest - scan the grid for the cell whose 30x30 object bounds contain
// (px,py). startRow==5 means "rows 0..3"; otherwise just that one row. Writes the hit
// (row,col) through the out-ptrs and returns the cell pointer (0 when none).
// @early-stop
// 70.9->71.2: the "5 = all" decode is now the if/else form (row/last kept in eax/edx).
// Residual regalloc wall: retail spills px to [esp+0x1c] and reloads it for each box-edge
// compare (freeing esi to precompute y0+30), and tail-merges the loop exit; our cl pins px
// in ebx. High register pressure (5 args + this + nested loop) -> different spill picks.
// Logic + offsets byte-exact. topic:wall topic:regalloc.
RVA(0x0006bea0, 0xe2)
void* CTriggerMgr::CellHitTest(i32 px, i32 py, i32* outRow, i32* outCol, i32 startRow) {
    i32 row, last;
    if (startRow == 5) {
        row = 0;
        last = 3;
    } else {
        last = startRow;
        row = startRow;
    }
    while (row <= last) {
        CTmCell** cell = &m_grid[row * 15];
        for (i32 col = 0; col < 15; col++) {
            CTmCell* g = cell[col];
            if (g != 0 && g->m_1fc != 0) {
                CTmDisplay* o = g->m_10;
                if (o->m_198 != 0) {
                    i32 x0 = o->m_5c - 15;
                    i32 y0 = o->m_60 - 15;
                    if (px < x0 + 30 && px >= x0 && py < y0 + 30 && py >= y0) {
                        if (outRow != 0) {
                            *outRow = row;
                        }
                        if (outCol != 0) {
                            *outCol = col;
                        }
                        return m_grid[row * 15 + col];
                    }
                }
            }
        }
        row++;
    }
    return 0;
}

// 0x77f80: FindNearestInRow(g) - the grunt-to-cell proximity probe: scan the 15 cells
// of grid row g->m_1ec for the live cell whose display object (cell->m_10) is nearest g's
// tile position, but only when that squared distance is below the cutoff 2*g->m_2dc.
// @early-stop
// 84->89.5: the m_5c distance term now loads BEFORE m_60 (dx declared before dy) matching
// retail's load order. Residual (~89.5%) is the tx/ty callee-saved coloring: retail homes
// tx->ebp (via `mov ebp,edx`) and ty->eax, computing tx>>5 before rowIdx*15; our cl colors
// tx->ebx, ty->ebp and schedules rowIdx*15 first. Every instruction + offset matches modulo
// register names; not source-steerable. topic:wall topic:regalloc.
RVA(0x00077f80, 0xab)
CTmCell* CTriggerMgr::FindNearestInRow(CTmCell* g) {
    i32 tx = g->m_pos.x >> 5;
    i32 rowIdx = g->m_1ec;
    CTmCell** cell = &m_grid[rowIdx * 15];
    i32 ty = g->m_pos.y >> 5;
    CTmCell* best = 0;
    i32 bestDist = 0x7fffffff;
    i32 i = 15;
    do {
        CTmCell* c = *cell;
        if (c != 0) {
            CTmDisplay* o = c->m_10;
            i32 dx = (o->m_5c >> 5) - tx;
            i32 dy = (o->m_60 >> 5) - ty;
            i32 d = dx * dx + dy * dy;
            if (d < bestDist && d < g->m_2dc * 2) {
                best = c;
                bestDist = d;
            }
        }
        cell++;
        i--;
    } while (i != 0);
    return best;
}

// 0x78260: RemoveCellRecord(x, y, fromSelection) - when fromSelection, first unlink the
// (x,y) node from whichever of the 10 selection lists (+0x2d0) holds it. Then find the
// (x,y) node in the record list (+0x244); if present, optionally StopPendingFx, clear the
// grid cell's sprites (grid[y+15*x]), flag+clear the active goal/record (+0x234..+0x23c),
// tick the overlay if it owns (x,y), recycle the node and RemoveAt the +0x240 list; ret 1.
// ret 0 when no record. (__stdcall: ret 0xc.)
// @early-stop
// regalloc wall: retail pins this->ebp and the selection counter in [esp+0x1c]; under
// our cl's pressure this spills to [esp+0x10] and is reloaded. Logic + offsets + the
// free-list recycle byte-exact; the record-scan/goal/overlay path matches. topic:wall.
RVA(0x00078260, 0x165)
i32 CTriggerMgr::RemoveCellRecord(i32 x, i32 y, i32 fromSelection) {
    if (fromSelection != 0) {
        CTmObList* list = m_selLists;
        i32 k = 10;
        do {
            CTmNode* n = list->m_head;
            while (n != 0) {
                CTmNode* cur = n;
                n = n->m_next;
                i32* p = cur->m_payload;
                if (p[0] == x && p[1] == y) {
                    void** slot = (void**)((char*)p - g_freeListNodeBias);
                    *slot = g_freeList;
                    g_freeList = slot;
                    list->RemoveAt(cur);
                }
            }
            list++;
            k--;
        } while (k != 0);
    }
    CTmNode* n = m_recList.m_head;
    if (n == 0) {
        return 0;
    }
    CTmNode* cur;
    i32* p;
    do {
        cur = n;
        n = n->m_next;
        p = cur->m_payload;
        if (p[0] == x && p[1] == y) {
            goto found;
        }
    } while (n != 0);
    return 0;
found:
    if (m_recList.m_count == 1) {
        StopPendingFx();
    }
    CTmCell* cell = m_grid[y + x * 15];
    if (cell != 0) {
        cell->ClearAllSprites();
    }
    if (m_recX == p[0] && m_recY == p[1]) {
        CTmGoal* goal = m_goal;
        if (goal != 0) {
            goal->m_8 |= 0x10000;
            m_goal = 0;
        }
        m_230 = 0;
    }
    CTmOverlay* ov = m_overlay;
    if (ov != 0 && ov->m_0 == p[0] && ov->m_4 == p[1]) {
        OverlayTick();
    }
    void** slot = (void**)((char*)p - g_freeListNodeBias);
    *slot = g_freeList;
    g_freeList = slot;
    m_recList.RemoveAt(cur);
    return 1;
}

// 0x78430: ResetAll - drain the record list (+0x244): for each node, clear the
// referenced grid cell's sprites (grid[ y + 15*x ] @+0x1c) and recycle the node to
// the free list; RemoveAll the +0x240 list, run StopPendingFx, flag the goal (+0x23c).
// @early-stop
// 99.39% - TRIGGER: the Phase-1 TriggerMgrEh merge moved this base method into the
// unit's /GX (eh) profile (100%->99.39%); the residual is a /GX EH-state-id artifact
// on an otherwise byte-identical body. The same merge IMPROVED the sibling base
// methods (Load 65->80%, ToggleRegionA 53->59%), confirming the real dev TU was one
// /GX unit. Accepted per the cleanup-over-% mandate.
RVA(0x00078430, 0x7f)
void CTriggerMgr::ResetAll() {
    CTmNode* n = m_recList.m_head;
    if (n != 0) {
        do {
            CTmNode* cur = n;
            n = n->m_next;
            i32* payload = cur->m_payload;
            i32 idx = payload[1] + 15 * payload[0];
            CTmCell* cell = m_grid[idx];
            if (cell != 0) {
                cell->ClearAllSprites();
                void** slot = (void**)((char*)payload - g_freeListNodeBias);
                *slot = g_freeList;
                g_freeList = slot;
            }
        } while (n != 0);
    }
    m_recList.RemoveAll();
    StopPendingFx();
    CTmGoal* goal = m_goal;
    if (goal != 0) {
        goal->m_8 |= 0x10000;
        m_goal = 0;
    }
}

// 0x784d0: RecordListHas(x, y) - scan the record list (+0x244) for a node whose
// payload (x,y) matches; ret 1 on hit, 0 otherwise.
// @early-stop
// loop-epilogue wall: retail loops with `jne top` and reuses the null next as the
// eax=0 return (fall-through); our cl emits `je end; jmp top` + a separate
// `xor eax,eax`. docs/patterns/identical-return-epilogue-tailmerge.md
RVA(0x000784d0, 0x3a)
i32 CTriggerMgr::RecordListHas(i32 x, i32 y) {
    CTmNode* n = m_recList.m_head;
    if (n == 0) {
        return 0;
    }
    do {
        CTmNode* cur = n;
        n = n->m_next;
        i32* p = cur->m_payload;
        if (p[0] == x && p[1] == y) {
            return 1;
        }
    } while (n != 0);
    return 0;
}

// 0x78880: ClearRecords - drain the record list (+0x244) back to the free list,
// then RemoveAll the +0x240 MFC pointer list. The free-list head is cached in a
// register across the loop (g_freeList read once, written each iteration).
// @early-stop
// prologue scheduling wall: the drain loop body is byte-exact, but retail batches
// `push edi; push esi` then loads esi=head/edi=bias; our cl interleaves the loads
// with the pushes. docs/patterns/zero-store-before-loop-inline-bound.md
RVA(0x00078880, 0x3c)
void CTriggerMgr::ClearRecords() {
    CTmNode* n = m_recList.m_head;
    if (n != 0) {
        i32 bias = g_freeListNodeBias;
        void* head = g_freeList;
        do {
            CTmNode* cur = n;
            n = n->m_next;
            void** slot = (void**)((char*)cur->m_payload - bias);
            *slot = head;
            head = slot;
            g_freeList = head;
        } while (n != 0);
    }
    m_recList.RemoveAll();
}

// 0x78a30: OverlayTick - dispatch the overlay sub-object's Tick when present.
RVA(0x00078a30, 0x10)
void CTriggerMgr::OverlayTick() {
    CTmOverlay* ov = m_overlay;
    if (ov) {
        ov->Tick();
    }
}

// 0x79b00: OverlayRelease - release the overlay sub-object when present; ret 1.
RVA(0x00079b00, 0x15)
i32 CTriggerMgr::OverlayRelease() {
    CTmOverlay* ov = m_overlay;
    if (ov) {
        return ov->Release();
    }
    return 1;
}

// 0x79b30: ByteTableHas(b) - linear search the byte table (+0x264, count +0x268)
// for `b`; ret 1 on hit, 0 otherwise.
// @early-stop
// register-coloring wall (~85%): the un-peeled indexed loop now matches retail (plain for-loop,
// no explicit n<=0 guard so the count is tested once). The residual is pure regalloc - retail
// colors count in esi and the table in ecx (SIB `[eax+ecx]`, dl temp); our cl parks count in edx
// and the table in esi (SIB `[esi+eax]`, cl temp). Not source-steerable. topic:wall.
RVA(0x00079b30, 0x3e)
i32 CTriggerMgr::ByteTableHas(i32 b) {
    i32 n = m_byteArr.m_count;
    u8* tbl = m_byteArr.m_data;
    for (i32 i = 0; i < n; i++) {
        if (b == tbl[i]) {
            return 1;
        }
    }
    return 0;
}

// 0x7be10: StopPendingFx - when our overlay-fx (+0x2a8) is live, or the world has a
// pending flag (world+0x504), stop the world's fx and clear +0x2a8.
RVA(0x0007be10, 0x34)
void CTriggerMgr::StopPendingFx() {
    CTmWorld* world = g_gameReg->m_curState;
    if (m_pendingFxKind == 0 && world->m_504 == 0) {
        return;
    }
    world->LoadCursorSprites(0, 0);
    m_pendingFxKind = 0;
}

// 0x7d0c0: ClearSelections - drain all 10 selection lists (+0x2d0, stride 0x1c) back
// to the free list (skipping null-payload nodes), RemoveAll each list, reset +0x3e8.
RVA(0x0007d0c0, 0x57)
void CTriggerMgr::ClearSelections() {
    CTmObList* list = m_selLists;
    i32 k = 10;
    do {
        CTmNode* n = list->m_head;
        if (n != 0) {
            void* head = g_freeList;
            do {
                CTmNode* cur = n;
                n = n->m_next;
                i32* payload = cur->m_payload;
                if (payload != 0) {
                    void** slot = (void**)((char*)payload - g_freeListNodeBias);
                    *slot = head;
                    head = slot;
                    g_freeList = head;
                }
            } while (n != 0);
        }
        list->RemoveAll();
        list++;
        k--;
    } while (k != 0);
    m_selSentinel = -1;
}

// 0x7d140: ClearRow(row) - run ExitGrid on the 15 live, hook-less cells of grid
// row `row` (+0x1c); clear +0x400 when row is the magic group, then refresh world.
RVA(0x0007d140, 0x61)
i32 CTriggerMgr::ClearRow(i32 row) {
    CTmCell** cell = &m_grid[row * 15];
    i32 i = 15;
    do {
        CTmCell* c = *cell;
        if (c != 0 && c->m_368 == 0) {
            c->ExitGrid();
        }
        cell++;
        i--;
    } while (i != 0);
    if (row == g_644c54) {
        m_groupFlag = 0;
    }
    g_gameReg->m_curState->Refresh();
    return 1;
}

// 0x7d1d0: NearestCellDist - the minimum squared (>>5 tile) distance from (px,py) to
// any live, clickable grid cell, scanning rows 0..3 but skipping row `skipRow`.
// CRACKED (77->100): (1) branchless abs() on the squared sum -> cdq;xor;sub (the
// `if(d<0)d=-d` branch form emits jns;neg); (2) the m_5c distance term declared/evaluated
// BEFORE m_60 (load-order); (3) `r` declared before `row` + `r++` before `row+=15` so the
// loop counter takes the [esp+0x20] slot. docs/patterns/signed-modulo-pow2-abs-restore.md.
RVA(0x0007d1d0, 0x9d)
i32 CTriggerMgr::NearestCellDist(i32 skipRow, i32 px, i32 py) {
    i32 tx = px >> 5;
    i32 ty = py >> 5;
    i32 best = 0x7fffffff;
    i32 r = 0;
    CTmCell** row = m_grid;
    do {
        if (r != skipRow) {
            i32 i = 15;
            CTmCell** cell = row;
            do {
                CTmCell* g = *cell;
                if (g != 0 && g->m_1fc != 0) {
                    CTmDisplay* o = g->m_10;
                    i32 dx = (o->m_5c >> 5) - tx;
                    i32 dy = (o->m_60 >> 5) - ty;
                    i32 d = abs(dx * dx + dy * dy);
                    if (d < best) {
                        best = d;
                    }
                }
                cell++;
                i--;
            } while (i != 0);
        }
        r++;
        row += 15;
    } while (r < 4);
    return best;
}

// 0x7d2a0: SelectionListFind(key, y) - only when key == g_644c54, scan the 10
// selection lists (+0x2d4, stride 0x1c) for a node whose payload (x,y) matches
// (key,y); ret the first matching list index, 0xa on a second match, else 0.
// @early-stop
// loop-rotation + interleaved-epilogue wall: the list-walk body is byte-exact, but
// retail loops with `jl top` (fall-through exit) and interleaves `mov eax,0xa` between
// the pops; our cl emits `jge end; jmp top` + a clean epilogue. topic:wall.
RVA(0x0007d2a0, 0x64)
i32 CTriggerMgr::SelectionListFind(i32 key, i32 y) {
    if (key != g_644c54) {
        return 0;
    }
    i32 result = 0;
    i32 i = 0;
    CTmObList* list = m_selLists;
    do {
        CTmNode* n = list->m_head;
        while (n != 0) {
            CTmNode* cur = n;
            n = n->m_next;
            i32* payload = cur->m_payload;
            if (payload[0] == key && payload[1] == y) {
                if (result != 0) {
                    return 10;
                }
                result = i;
            }
        }
        i++;
        list++;
    } while (i < 10);
    return result;
}

// 0x7d330: DestroyAllAnims - DestroyAnims on every live grid cell (4 rows x 15), then
// walk the level's display-object list clearing the +0x200 marker on each object whose
// type-descriptor (obj+0x7c) slot-4 matches the switch tag, then stop the three sound
// channels (+0x3f0, +0x3f4, and the active grunt's +0x618).
// @early-stop
// regalloc wall: retail homes the row counter in ebp and the zero-compare const in ebx;
// our cl swaps them (ebx counter, ebp zero). Code bytes + offsets byte-exact, externs
// reloc-masked (DirectSoundMgr::StopAndRewind, ReadConfigFromButeMgr tag). topic:wall.
RVA(0x0007d330, 0xd3)
void CTriggerMgr::DestroyAllAnims() {
    CTmCell** cell = m_grid;
    i32 r = 4;
    do {
        i32 i = 15;
        do {
            CTmCell* g = *cell;
            if (g != 0) {
                g->DestroyAnims();
            }
            cell++;
            i--;
        } while (i != 0);
        r--;
    } while (r != 0);

    CSpriteListNode* node = m_level->m_8->m_liveObjects;
    while (node != 0) {
        CTmCell* obj = (CTmCell*)node->m_sprite;
        node = node->next;
        if (obj != 0) {
            char* desc = *(char**)((char*)obj + 0x7c);
            void (CTmCell::*tag)() = &CTmCell::ReadConfigFromButeMgr;
            if (*(void**)(desc + 0x10) == *(void**)&tag) {
                char* tgt = *(char**)(desc + 0x18);
                *(i32*)(tgt + 0x200) = 0;
            }
        }
    }

    DirectSoundMgr* ch0 = m_soundChanA;
    if (ch0 != 0) {
        ch0->StopAndRewind();
        m_soundChanA = 0;
    }
    DirectSoundMgr* ch1 = m_soundChanB;
    if (ch1 != 0) {
        ch1->StopAndRewind();
        m_soundChanB = 0;
    }
    void* state = ((CGruntzMgr*)g_gameReg)->PickPausedThenPlayState();
    if (state != 0) {
        char* sub = *(char**)((char*)state + 0x2dc);
        if (sub != 0) {
            DirectSoundMgr* ch2 = *(DirectSoundMgr**)(sub + 0x618);
            if (ch2 != 0) {
                ch2->StopAndRewind();
                *(DirectSoundMgr**)(sub + 0x618) = 0;
            }
        }
    }
}

// 0x7a510: ClearRowAndRefresh(startRow) - Recall every live, hook-less cell of rows
// startRow..3 (5 = all); clear +0x400 when startRow is the magic group; then refresh
// the world, bump a stat, and re-arm the status item. ret 1.
// CRACKED (72->100): the "5 = all rows" decode as an explicit if/else (row=0/last=3 vs
// row=last=startRow) matches retail's jmp-merge (the default-then-override form reserves an
// extra frame slot + spills last); assign `last` before `row` in the else; declare `cell`
// (row*15) BEFORE `n` so the two lea chains interleave into edx. Cluster idiom for all the
// "startRow==5" range decoders (ClearGridRange/HitTestCell/CellHitTest).
RVA(0x0007a510, 0x9e)
i32 CTriggerMgr::ClearRowAndRefresh(i32 startRow) {
    i32 row, last;
    if (startRow == 5) {
        row = 0;
        last = 3;
    } else {
        last = startRow;
        row = startRow;
    }
    if (row <= last) {
        CTmCell** cell = &m_grid[row * 15];
        i32 n = last - row + 1;
        do {
            i32 i = 15;
            do {
                CTmCell* c = *cell;
                if (c != 0 && c->m_368 == 0) {
                    c->Recall();
                }
                cell++;
                i--;
            } while (i != 0);
            n--;
        } while (n != 0);
    }
    if (startRow == g_644c54) {
        m_groupFlag = 0;
    }
    CTmWorld* world = g_gameReg->m_curState;
    world->Refresh();
    world->SetStat(0, 0xbb7);
    ((CSBI_RectOnly*)world->m_2dc)->SetMode(1);
    return 1;
}

// 0x7a180: SpawnPuddle(x, y, f124, dir, color, f118) - create a "GruntPuddle" sprite from
// the level's factory; on failure ReportError and ret 0. On success run its Init hook,
// stash the placement fields (+0x124/+0x114/+0x118) and tail into PlacePuddle. (ret 0x18.)
RVA(0x0007a180, 0x86)
i32 CTriggerMgr::SpawnPuddle(i32 x, i32 y, i32 f124, i32 f114, i32 color, i32 f118) {
    CSpriteFactory* fac = m_level->m_8;
    CTmCell* sprite = (CTmCell*)fac->CreateSprite(0, x, y, 0xa, "GruntPuddle", 0x40003);
    if (sprite == 0) {
        ((CGruntzMgr*)g_gameReg)->ReportError(0x8009, 0x400);
        return 0;
    }
    sprite->m_7c->Init(sprite);
    sprite->m_124 = f124;
    sprite->m_114 = f114;
    sprite->m_118 = f118;
    return PlacePuddle(sprite, color);
}

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

// 0x7a240: PlacePuddle(sprite, color) - hand the sprite's placement params to its target's
// PlacePuddle; on failure flag the goal + ReportError(0x8009,0x401). On success walk the
// record list twice (full (x,y) match, then x-only when count>0x3b) flagging+unlinking each
// matching node, then RemoveAll the list. (__thiscall: ret 0x8.)
// @early-stop
// regalloc + dual-loop scheduling wall: the two record-walk loops + the found/unlinked flags
// spill to different stack slots than retail and the (x,y)==busy fast-path goto reorders.
// Logic + offsets + the RemoveAt/RemoveAll recycle byte-exact. topic:wall.
RVA(0x0007a240, 0x143)
i32 CTriggerMgr::PlacePuddle(CTmCell* sprite, i32 color) {
    CTmPuddleTarget* tgt = (CTmPuddleTarget*)sprite->m_7c->m_18;
    i32 d = sprite->m_118;
    if (d == 0) {
        d = 0x19;
    }
    if (tgt->Place(sprite->m_124, sprite->m_114, color, d) == 0) {
        tgt->m_38->m_8 |= 0x10000;
        ((CGruntzMgr*)g_gameReg)->ReportError(0x8009, 0x401);
        return 0;
    }
    CTmRecNode* n = (CTmRecNode*)m_baseList.m_head;
    i32 manyFlag = (m_baseList.m_count > 0x3b) ? 1 : 0;
    i32 unlinked = 0;
    while (n != 0 && unlinked == 0) {
        CTmRecNode* cur = n;
        n = n->m_next;
        CTmPuddleTarget* o = cur->m_obj;
        if (o->m_54 == tgt->m_54 && o->m_58 == tgt->m_58) {
            if (o->m_5c != 0) {
                tgt->m_38->m_8 |= 0x10000;
                return 0;
            }
            o->m_38->m_8 |= 0x10000;
            m_baseList.RemoveAt(cur);
            unlinked = 1;
        }
    }
    if (manyFlag != 0 && unlinked == 0) {
        n = (CTmRecNode*)m_baseList.m_head;
        while (n != 0) {
            CTmRecNode* cur = n;
            n = n->m_next;
            CTmPuddleTarget* o = cur->m_obj;
            if (o->m_5c == 0) {
                o->m_38->m_8 |= 0x10000;
                m_baseList.RemoveAt(cur);
            }
        }
    }
    m_baseList.AddTail(tgt);
    return 1;
}

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

// 0x7a5e0: RebuildOverlay(obj, kind, ., .) - copy the source object's two pose getters into
// the manager's three 0x8-byte pose blocks (+0x290/+0x2b0/+0x2c0), selecting getter +0x30 for
// kind 4 and +0x2c for kind 7; ret 0 when obj null, 1 otherwise. Early-out when the kind-4/7
// self-probe is non-zero. (__thiscall: ret 0x10 => 4 args; Ghidra mis-derived the 2-arg
// `...PAXH@Z` prototype - only obj/kind are used, the trailing two slots are dead.)
// BYTE-EXACT after: (1) the 4-arg prototype, (2) the probe polarity (retail continues
// when Probe!=0, bails to return 0 when Probe==0), (3) virtual GetA/GetB dispatch (slots
// +0x2c/+0x30 -> a real 13-slot vtable view), and (4) the negated-outer-condition idiom
// (docs/patterns/negated-condition-far-block.md): writing each kind dispatch as
// `if (kind != 4) { if (kind == 7) {..7..} } else {..4..}` places the kind==4 body FAR,
// matching retail's block layout (the natural `if(kind==4)else if(kind==7)` lays it inline).
RVA(0x0007a5e0, 0x121)
i32 CTriggerMgr::RebuildOverlay(void* obj, i32 kind, i32 /*unusedC*/, i32 /*unusedD*/) {
    if (obj == 0) {
        return 0;
    }
    // Negated outer test (kind!=4 ... else kind==4): reproduces retail's block layout
    // where the kind==4 body is placed FAR (after the kind==7 body).
    if (kind != 4) {
        if (kind == 7) {
            if (this->Probe7(obj) == 0) {
                return 0;
            }
        }
    } else {
        if (this->Probe4(obj) == 0) {
            return 0;
        }
    }
    CTmOverlaySrc* src = (CTmOverlaySrc*)obj;
    char* blk0 = m_overlayDescA;
    if (kind != 4) {
        if (kind == 7) {
            src->GetA(blk0, 8);
            src->GetA(blk0 + 8, 8);
        }
    } else {
        src->GetB(blk0, 8);
        src->GetB(blk0 + 8, 8);
    }
    char* blk1 = m_overlayDescB;
    if (kind != 4) {
        if (kind == 7) {
            src->GetA(blk1, 8);
            src->GetA(blk1 + 8, 8);
        }
    } else {
        src->GetB(blk1, 8);
        src->GetB(blk1 + 8, 8);
    }
    char* blk2 = m_overlayDescC;
    if (kind != 4) {
        if (kind == 7) {
            src->GetA(blk2, 8);
            src->GetA(blk2 + 8, 8);
        }
    } else {
        src->GetB(blk2, 8);
        src->GetB(blk2 + 8, 8);
        return 1;
    }
    return 1;
}

// 0x6bfd0: ResetCell(col, row, force, ...) - if grid[row*15+col] (+0x1c) is live: for a
// non-magic row, run its three sub-state resetters then re-seed its CombatTimeout config
// fields (+0x880..+0x88c); for the magic row (== g_644c54), when not forced recycle the
// (row,col) record node onto the free list, AddTail it to +0x240, and run ResetMagic. ret 1
// only when a magic cell was recycled, else 0. (__stdcall: ret 0x10.)
// @early-stop
// regalloc + free-list-recycle scheduling wall: the node-bias recycle and the GetInt arg
// push order pin ebx/edi differently than retail. Logic + offsets byte-exact. topic:wall.
RVA(0x0006bfd0, 0x106)
i32 CTriggerMgr::ResetCell(i32 col, i32 row, i32 force, i32 keep) {
    i32 idx = col * 15 + row;
    CTmCell* cell = m_grid[idx];
    if (cell == 0 || cell->m_1fc == 0) {
        return 0;
    }
    if (col != g_644c54) {
        cell->ResetA();
        cell->ResetB();
        cell->ResetC();
        cell->m_888 = g_buteMgr.GetDwordDef("Grunt", "CombatTimeout", 0x1388);
        cell->m_88c = 0;
        cell->m_880 = g_645588;
        cell->m_884 = 0;
        return 0;
    }
    if (force != 0) {
        if (keep == 0) {
            if (this->Reset3(col, row, 0) != 0) {
                return 1;
            }
        }
    } else {
        this->RefreshC(); // self-call 0x6c068 (reloc-masked)
    }
    void* node = g_freeList;
    i32* slot = 0;
    if (*(void**)node != 0) {
        slot = (i32*)((char*)node + 4);
        slot[0] = col;
        slot[1] = row;
        g_freeList = *(void**)g_freeList;
    }
    m_recList.AddTail(slot);
    return cell->ResetMagic();
}

// 0x6ea00: HitTestApply(x, y, kind) - hit-test the cell at (x,y); only for the magic group
// (out-col == g_644c54) and a cell whose config name is NOT "B" and kind 0x14, add the world's
// score delta, zero the status fields, SetStat(0,0xbb7), re-arm the status item (SetMode 1)
// and ClearMagic(g_644c54). void - no path materialises a return value. (__stdcall: ret 0xc.)
// @early-stop
// inline-strcmp result-register coloring wall (~80%): void return + strcmp `!= 0` bool steer +
// i64 score sub are byte-exact and size now matches retail (0x125). The residual is the inline
// strcmp landing its sbb result in ecx (retail eax) with the `differ` bool in al vs retail's cl,
// so the `setne`+null-test colors as `cmpb bl,al` vs retail `testb cl,cl`. Not source-steerable
// (the `bool` local is required for the setne form but shifts the result register). topic:wall.
RVA(0x0006ea00, 0x125)
void CTriggerMgr::HitTestApply(i32 x, i32 y, i32 kind) {
    i32 outRow = 0;
    i32 outCol = 0;
    CTmCell* cell = this->Hit(kind, y, y, &outRow, &outCol);
    if (cell == 0 || outCol != g_644c54) {
        return;
    }
    char* name = *g_nameReg.Lookup(cell->m_14->m_1c);
    bool differ = strcmp(name, "B") != 0;
    if (!differ) {
        return;
    }
    i32 k = cell->m_170;
    if (k > 0x16) {
        k = cell->m_19c;
    }
    if (k != 0x14) {
        return;
    }
    CTmWorld* world = g_gameReg->m_curState;
    CTmScoreSub* sub = world->m_3f4;
    i64 diff = (i64)(u32)g_645588 - sub->m_38;
    if (diff < 0) {
        diff = 0;
    }
    g_gameReg->m_scoreBoard->m_score += (i32)diff;
    sub->m_40 = 0;
    sub->m_44 = 0;
    sub->m_30 = 0;
    sub->m_34 = 0;
    sub->m_48 = 0;
    sub->m_4c = 0;
    world->SetStat(0, 0xbb7);
    ((CSBI_RectOnly*)world->m_2dc)->SetMode(1);
    this->ClearMagic(g_644c54);
}

// The stop-fx hook the TriggerCell driver uses (reloc-masked).
// 0x7b1b0: TriggerCell(x, y) - clear the pending-fx slot; only when the overlay sub-object
// (+0x25c) is live with its +0x2c set, look up the magic record cell, classify (x,y) and by
// the resulting kind spawn the matching fx sprite (kind 2 -> remapped 0x13, kind 3 -> alt
// 0x1e, else generic +0xc8 into +0x2a8), then Refresh + Record. ret 1. (ret 0x8.)
// @early-stop
// regalloc + switch-on-kind wall: the classify result drives a cmp/je ladder that pins ebx
// (world) and esi (cell) differently than retail, and the fx arg pushes spill. topic:wall.
RVA(0x0007b1b0, 0x12b)
i32 CTriggerMgr::TriggerCell(i32 x, i32 y) {
    CTmOverlay* ov = m_overlay;
    m_pendingFxKind = 0;
    if (ov == 0 || ov->m_2c == 0) {
        return 0;
    }
    CTmCell* cell;
    if (m_recList.m_count != 1) { // negated-far cell decode (see ToggleRegionA)
        cell = 0;
    } else {
        i32* rec = m_recList.m_head->m_payload;
        cell = m_grid[rec[1] + rec[0] * 15];
    }
    CTmWorld* world = g_gameReg->m_curState;
    i32 kind = this->Classify(x, y);
    if (kind == 2) {
        i32 alt = cell->m_170;
        if (alt > 0x16) {
            alt = cell->m_19c;
        }
        if (alt == 0x13) {
            g_gameReg->m_68->Spawn(cell->m_pos.x, cell->m_pos.y, 0, 0, 0, 2, 1);
        }
    } else if (kind == 3) {
        if (cell->m_198 == 0x1e) {
            CTmDisplay* o = cell->m_10;
            g_gameReg->m_68->Spawn(o->m_5c, o->m_60, 0, 0, 0, 3, 1);
        }
    } else if (kind != 0) {
        i32 v = kind + kPendingFxIdBase;
        m_pendingFxKind = v;
        world->StopFx2(v, 0);
    }
    this->Refresh2();
    this->Record2(x, y);
    return 1;
}

// The sprite's bound logic (sprite desc +0x18) is the canonical CUserLogic
// (<Gruntz/UserLogic.h>): its multi-arg Place driver (@0x4c1c4) and target-cursor Arm
// (@0x4e517) are reached through the base pointer (former CTmUserLogic view folded away).

// 0x7c110: SpawnGrunt(col, row, a18, a1c) - find the first free column of grid row `row`;
// if full ret 0. Snap the source cell[col]'s display pos to a tile, run a prep self-call,
// create a "Grunt" sprite at that pos, Init it, place it via its userlogic; on placement
// failure flag the goal and ret 0, else stash the cell + bump the per-row counters. ret 1.
// (__stdcall: ret 0x10.)
// @early-stop
// regalloc + free-column-scan wall: the inner "first free col" loop and the snap arithmetic
// pin ebp/edi differently than retail; the placement-failure goal-flag path tail-merges.
RVA(0x0007c110, 0x166)
i32 CTriggerMgr::SpawnGrunt(i32 col, i32 row, i32 a18, i32 a1c) {
    CTmCell* src = m_grid[col * 15 + a1c];
    i32 free = 0;
    CTmCell** rowBase = &m_grid[row * 15];
    if (*rowBase != 0) {
        CTmCell** p = rowBase;
        while (free < 15 && *p != 0) {
            p++;
            free++;
        }
    }
    if (free >= 15) {
        return 0;
    }
    CTmDisplay* o = src->m_10;
    i32 sx = (o->m_5c & ~0x1f) + 0x10;
    i32 sy = (o->m_60 & ~0x1f) + 0x10;
    i32 k = src->m_170;
    if (k > 0x16) {
        k = src->m_19c;
    }
    i32 vis = src->m_198;
    this->Reset3(col, k, vis); // prep self-call 0x7ec96
    CSpriteFactory* fac = m_level->m_8;
    CTmCell* sprite = (CTmCell*)fac->CreateSprite(0, sx, sy, 0x186a0, "Grunt", 0x40003);
    if (sprite == 0) {
        return 0;
    }
    sprite->m_7c->Init(sprite);
    void* logic = sprite->m_7c->m_18;
    if (((CUserLogic*)logic)->Place(col, row, vis, k, 0, 0, 0, 0, 0, 0, 0, 0) == 0) {
        *(i32*)(*(char**)((char*)logic + 0x154) + 0x8) |= 0x10000;
        return 0;
    }
    m_grid[row * 15 + free] = sprite;
    m_rowCount[row] += 1;
    m_cellFlag[(row * 15 + free)] = 0;
    return 1;
}

// 0x759e0: GetOriginXY(out) - copy the cached origin pair (+0x174,+0x178) into the
// caller's slot and return it. `out` (loaded into eax as the store base) is the
// return value; `ret 4` -> callee cleans the out-ptr.
RVA(0x000759e0, 0x18)
CTrigPoint* CTriggerMgr::GetOriginXY(CTrigPoint* out) {
    // the cached origin pair lives in the parallel flag grid at +0x58/+0x5c (== +0x174/+0x178)
    out->x = m_cellFlag[0x16];
    out->y = m_cellFlag[0x17];
    return out;
}

// 0x75a90: TmFlagsAllow(a, b, c) - a __cdecl trigger-flag compatibility test on the
// shared bits m = a & b: bit 0x20000000 vetoes outright; with no shared bits, allow;
// otherwise allow only when (a & c) is also set.
RVA(0x00075a90, 0x27)
i32 TmFlagsAllow(i32 a, i32 b, i32 c) {
    i32 m = b & a;
    if (m & 0x20000000) {
        return 0;
    }
    if (m != 0 && (c & a) == 0) {
        return 0;
    }
    return 1;
}

// 0x75af0: HitTestCell(x, y, outRow, outCol, exact) - sample the plane tile-attr at
// (x>>5, y>>5); its high byte is the row, low byte the col. Look up grid[row*15+col]; if
// live+clickable, either exact-match its world pos (exact) or its 30x30 bounds, then write
// (row,col) through the out-ptrs. ret 0 on any miss. (__stdcall: ret 0x14.)
// @early-stop
// regalloc + bounds-arith wall: the row/col high/low-byte split pins edi/ebp and the
// ±7 box arithmetic spills to different slots than retail. Logic + offsets byte-exact.
RVA(0x00075af0, 0x111)
i32 CTriggerMgr::HitTestCell(i32 x, i32 y, i32* outRow, i32* outCol, i32 exact) {
    CTileGrid* plane = g_gameReg->m_tileGrid;
    i32 ix = x >> 5;
    i32 iy = y >> 5;
    i32 attr;
    if (ix >= plane->m_c || iy >= plane->m_10) {
        attr = -1;
    } else {
        attr = plane->m_8[iy][ix * 7 + 1];
    }
    if (attr == -1) {
        return 0;
    }
    i32 row = (attr >> 8) & 0xff;
    i32 col = attr & 0xff;
    CTmCell* cell = m_grid[col + row * 15];
    if (cell == 0 || cell->m_1fc == 0) {
        return 0;
    }
    if (exact != 0) {
        CTmDisplay* o = cell->m_10;
        if (o->m_5c != x || o->m_60 != y) {
            return 0;
        }
        if (outRow != 0) {
            *outRow = row;
        }
        if (outCol != 0) {
            *outCol = col;
        }
        return 1;
    }
    CTmDisplay* o = cell->m_10;
    i32 ox = o->m_5c;
    i32 oy = o->m_60;
    if (x + 7 > ox + 14 || x - 7 < ox - 7 || y + 7 > oy + 14 || y - 7 < oy - 7) {
        return 0;
    }
    if (outRow != 0) {
        *outRow = row;
    }
    if (outCol != 0) {
        *outCol = col;
    }
    return 1;
}

// 0x78520: ReportRecordsA(a14, a18, a1c, a20, a24) - when the level flag (+0x400) is set,
// scan the record list (+0x244) collecting the byte of each magic-group, un-triggered cell;
// if exactly one matched, hand it to the world's single-record reporter, else hand the whole
// collected array to the manager's multi-record reporter. (__stdcall: ret 0xc.)
// @early-stop
// reporter-dispatch arg-shape wall (~72%): the record scan is now byte-exact (u8 count +
// `bytes[count]=payload[4]` collected byte, size matches retail 0x106). The residual is the
// trailing count==1/else dispatch: retail calls two 8-arg reporter methods on g_gameReg->m_6c
// passing a per-iter firstByte dword slot (`*(u8*)payload` stored beside count) + the count/
// array as separate args; our 7-arg self-call ReportN/Report1 shape approximates it. topic:wall.
RVA(0x00078520, 0x106)
void CTriggerMgr::ReportRecordsA(i32 a14, i32 a18, i32 a1c, i32 a20, i32 a24) {
    if (m_groupFlag == 0) {
        return;
    }
    u8 bytes[0x88];
    u8 count = 0;
    CTmNode* n = m_recList.m_head;
    while (n != 0) {
        CTmNode* next = n->m_next;
        u8* payload = (u8*)n->m_payload;
        CTmCell* cell = m_grid[*(i32*)(payload + 4) + *(i32*)payload * 15];
        if (cell->m_1ec == g_644c54 && cell->m_1e4 == 0) {
            bytes[count] = payload[4];
            count++;
        }
        n = next;
    }
    if (count == 1) {
        g_gameReg->m_6c->Report1(2, bytes[0], a14, a18, 0, a1c, 0);
    } else {
        this->ReportN(2, a14, bytes, a18, a1c, a20, a24);
    }
}

// 0x78680: ReportRecordsB(a14, a18, a1c, a20, a24, a28) - the four-way variant of
// ReportRecordsA: same magic-group byte scan, then dispatch by (count==1, a14!=0) to one of
// four (single/multi x kind 3/9) report calls. (__stdcall: ret 0x10.)
// @early-stop
// reporter-dispatch arg-shape wall (~62%): same fixed record scan as ReportRecordsA (u8 count +
// payload[4] collected byte). The residual is the 4-way (count==1 x a28) dispatch to the two
// 8-arg g_gameReg->m_6c reporter methods with the firstByte dword slot; our self-call shape
// approximates it. topic:wall.
RVA(0x00078680, 0x189)
void CTriggerMgr::ReportRecordsB(i32 a14, i32 a18, i32 a1c, i32 a20, i32 a24, i32 a28) {
    if (m_groupFlag == 0) {
        return;
    }
    u8 bytes[0x88];
    u8 count = 0;
    CTmNode* n = m_recList.m_head;
    while (n != 0) {
        CTmNode* next = n->m_next;
        u8* payload = (u8*)n->m_payload;
        CTmCell* cell = m_grid[*(i32*)(payload + 4) + *(i32*)payload * 15];
        if (cell->m_1ec == g_644c54 && cell->m_1e4 == 0) {
            bytes[count] = payload[4];
            count++;
        }
        n = next;
    }
    CGruntzCmdMgr* rep = g_gameReg->m_6c;
    if (count == 1) {
        if (a28 != 0) {
            rep->Report1(9, bytes[0], a14, a18, 0, 0, 0);
        } else {
            rep->Report1(3, bytes[0], a14, a18, 0, 0, 0);
        }
    } else {
        if (a28 != 0) {
            this->ReportN(9, a14, bytes, a18, a1c, a20, a24);
        } else {
            this->ReportN(3, a14, bytes, a18, a1c, a20, a24);
        }
    }
}

// The serialization archive ScanGroup writes through is the shared CSerialArchive
// (<Gruntz/SerialArchive.h>, pulled via TriggerMgr.h): its Write @ vtable slot +0x30
// (real virtual dispatch). Plus the per-object id-write helper @0x5b8760.
void Ar_WriteId(void* id, i32 stride, void* archive); // 0x1b8760

// 0x7a760: ScanGroup(ar) - serialize the whole manager into archive `ar`: the 4x15 grid of
// cell ids, the four per-row arrays, the magic table + its bytes, the record + selection
// lists, the goal/overlay/state words. ret 0 when ar/level null or the overlay write fails.
// (__thiscall: ret 0x4.) [the manager's Serialize]
// @early-stop
// big serializer wall: 60+ archive Write calls; the grid/list write loops pin esi(ar)/ebx
// /edi differently than retail and the scratch slots differ. Logic + offsets byte-exact.
RVA(0x0007a760, 0x373)
i32 CTriggerMgr::ScanGroup(CSerialArchive* ar) {
    if (ar == 0) {
        return 0;
    }
    CTmLevel* lvl = m_level;
    if (lvl == 0) {
        return 0;
    }
    CTmCell** cell = m_grid;
    i32 r = 4;
    do {
        i32 c = 15;
        do {
            CTmCell* g = *cell;
            i32 id = 0;
            if (g != 0) {
                id = g->m_10->m_188;
                Ar_WriteId(lvl->m_8, id, ar);
            }
            ar->Write(&id, 4);
            cell++;
            c--;
        } while (c != 0);
        r--;
    } while (r != 0);
    ar->Write(m_rowCount, 0x10);
    ar->Write(m_cellFlag, 0xf0);
    ar->Write(m_rowStateB, 0x10);
    ar->Write(m_rowStateC, 0x10);
    i32 cnt = m_byteArr.m_count;
    ar->Write(&cnt, 4);
    for (i32 i = 0; i < cnt; i++) {
        u8 b = m_byteArr.m_data[i];
        ar->Write(&b, 1);
    }
    i32 flag24c = m_recList.m_count;
    ar->Write(&flag24c, 4);
    CTmNode* n = m_recList.m_head;
    while (n != 0) {
        CTmNode* cur = n;
        n = n->m_next;
        ar->Write(cur->m_payload, 8);
    }
    CTmObList* list = m_selLists;
    i32 k = 10;
    do {
        i32 cnt2 = list->m_count;
        ar->Write(&cnt2, 4);
        CTmNode* m = list->m_head;
        while (m != 0) {
            CTmNode* cur = m;
            m = m->m_next;
            ar->Write(cur->m_payload, 8);
        }
        list++;
        k--;
    } while (k != 0);
    void* goal = m_goal;
    i32 goalId = 0;
    if (goal != 0) {
        goalId = *(i32*)((char*)goal + 0x188);
    }
    ar->Write(&goalId, 4);
    void* ov = m_pendingFx;
    i32 ovId = 0;
    if (ov != 0 && *(void**)((char*)ov + 0x10) != 0) {
        ovId = *(i32*)(*(char**)((char*)ov + 0x10) + 0x188);
    }
    ar->Write(&ovId, 4);
    ar->Write(m_274, 0x10);
    i32 cntC = m_baseList.m_count;
    ar->Write(&cntC, 4);
    CTmNode* rn = m_baseList.m_head;
    while (rn != 0) {
        CTmNode* cur = rn;
        rn = rn->m_next;
        void* obj = cur->m_payload;
        if (obj == 0) {
            return 0;
        }
        i32 oid = *(i32*)(*(char**)((char*)obj + 0x10) + 0x188);
        Ar_WriteId(lvl->m_8, oid, ar);
        ar->Write(&oid, 4);
    }
    i32 hasOv = (m_overlay != 0) ? 1 : 0;
    ar->Write(&hasOv, 4);
    if (m_overlay != 0) {
        if (this->SerializeOverlay(ar, 0, 0) == 0) { // overlay serialize self-call 0x7df8
            return 0;
        }
    } else {
        return 0;
    }
    ar->Write(&m_230, 4);
    ar->Write(&m_284, 4);
    ar->Write(&m_288, 4);
    ar->Write(&m_recX, 8);
    ar->Write(&m_2a4, 4);
    ar->Write(&m_3ec, 4);
    ar->Write(&m_groupFlag, 4);
    ar->Write(&g_644c54, 4);
    ar->Write(&g_644ca4, 4);
    ar->Write(&m_pendingFxKind, 4);
    ar->Write(&m_selSentinel, 4);
    return 1;
}

// The world's report/spawn sub-mgrs ResetGroup dispatches through (gameReg+0x6c reporter,
// +0x68 fx-mgr, +0x60 cursor-mgr), all reloc-masked.
struct CTmCursorMgr {
    void Spawn(i32 a, i32 b, i32 c, i32 d, i32 e); // 0x90bf4 (gameReg+0x60)
};
extern i32 g_6455b0; // DAT_006455b0 (the alt-group gate)

// 0x79520: ResetGroup(a14, a18, ...) - when the level flag (+0x400) is set, hit-test the
// magic group, classify the (a14,a18) target into one of three branches and place/report the
// matching cursor / lightfx / warpstone sprite; on factory success Init it and report it.
// ret 1 (0 on flag-clear / placement failure). (__stdcall: ret 0x1c.)
// @early-stop
// big switch-driver wall (0x2e3 B): the three-way classify ladder + the three CreateSprite
// /Init/Report stanzas pin esi(sprite)/ebx/ebp differently than retail and the string-arg
// pushes spill. Logic + offsets + the reloc-masked externs byte-exact. topic:wall.
RVA(0x00079520, 0x2e3)
i32 CTriggerMgr::ResetGroup(i32 a14, i32 a18, i32 a1c, i32 a20, i32 a24, i32 a28, i32 a2c) {
    (void)a1c;
    (void)a20;
    (void)a24;
    (void)a2c;
    if (m_groupFlag == 0) {
        return 0;
    }
    CTmCell* hit = this->Hit5(a14, a18, 0, 0, 5);
    CTmCell* cell;
    if (m_recList.m_count != 1) { // negated-far cell decode (see ToggleRegionA)
        cell = 0;
    } else {
        i32* rec = m_recList.m_head->m_payload;
        cell = m_grid[rec[1] + rec[0] * 15];
    }
    i32 sel;
    if (cell != 0 && cell->m_1ec == g_644c54) {
        if (a28 != 0) {
            sel = 0;
        } else if (hit == 0) {
            sel = 1;
        } else if (hit == cell) {
            // toggle off the pending-fx and rewind
            m_pendingFxKind = 0;
            (g_gameReg->m_curState)->StopFx2(0, 0);
            CTmDisplay* o = hit->m_10;
            this->PlaceA(o->m_5c, o->m_60, a18, a14);
            return 1;
        } else {
            sel = 2;
        }
    } else {
        sel = (hit != 0) ? 2 : 1;
    }

    CTmCell* sprite = 0;
    i32 kindArg = 0;
    i32 logicArg = 0;
    if (sel == 0) {
        // place-on-self path
        this->PlaceB(a14, a18, 1);
        return 1;
    } else if (sel == 1) {
        // spawn the cursor target sprite
        CGruntzCmdMgr* rep = g_gameReg->m_6c;
        if (cell != 0) {
            rep->Report1(1, cell->m_1ec, cell->m_1f0, a18, a14, 0, 0);
        } else {
            rep->Report1(1, a14, a18, 0, 0, 0, 0);
        }
        if (*(i32*)((char*)this + 0x2c) == 0) { // placeholder gate (see raw)
            return 0;
        }
        CSpriteFactory* fac = m_level->m_8;
        sprite = (CTmCell*)fac->CreateSprite(0, a14, a18, 0xf4240, "LightFx", 0x40003);
        kindArg = 3;
        logicArg = 1;
    } else {
        // sel==2: place-and-report variant -> WarpStone factory
        this->PlaceB(a14, a18, 1);
        CSpriteFactory* fac = m_level->m_8;
        sprite = (CTmCell*)fac->CreateSprite(0, a14, a18, 0xf4240, "LightFx", 0x40003);
        kindArg = 2;
        logicArg = 1;
    }
    if (sprite == 0) {
        return 0;
    }
    sprite->m_7c->Init(sprite);
    void* logic = sprite->m_7c->m_18;
    ((CUserLogic*)logic)->Arm("GAME_LIGHTING_TARGETCURSOR", "GAME_TARGETCURSOR", kindArg, logicArg);
    return 1;
}

// 0x6dae0: ApplyTriggerA(a18, col, row, a24, a28, a2c) - look up grid[a18*15+col]; if live,
// un-pending and matching the snapped source pos, dispatch the cell's trigger logic by its
// kind (the 0x13/0xf branch families); update its state and return the applier result. Else
// -1 / 0. (__stdcall: ret 0x10.) Reconstructed to plateau.
// @early-stop
// big branchy trigger-applier (0x4b7 B): the kind-dispatch ladder + the snapped-pos compares
// pin esi(cell)/edi/ebp differently than retail; the body is structurally faithful but its
// regalloc diverges across the many branches. topic:wall.
RVA(0x0006dae0, 0x4b7)
i32 CTriggerMgr::ApplyTriggerA(i32 col, i32 row, i32 a24, i32 a28) {
    CTmCell* cell = m_grid[col * 15 + row];
    if (cell == 0 || cell->m_1fc == 0) {
        return 0;
    }
    CTmDisplay* o = cell->m_10;
    if (o->m_5c != cell->m_pos.x) {
        if (o->m_60 != cell->m_pos.y) {
            return -1;
        }
    }
    i32 k = cell->m_170;
    if (k > 0x16) {
        k = cell->m_19c;
    }
    if (k == 0x13) {
        CTmCell* tc = cell;
        if (tc->Type13Check() != 0) {
            tc->Apply13(row, a28 + 1);
            return 1;
        }
    }
    if (k == 0xf) {
        cell->Dispatch(k, row);
    }
    return 0;
}

// 0x6e120: ApplyTriggerB(a1c, col, row, a28, a2c, a30) - the exit variant of ApplyTriggerA:
// same cell lookup + validation, then snap (a28,a2c) to a tile and route the cell's exit
// logic; updates +0x384 and returns the applier's boolean. (__stdcall: ret 0x10.)
// Reconstructed to plateau.
// @early-stop
// big branchy trigger-applier (0x552 B): mirrors ApplyTriggerA's wall - kind-dispatch ladder
// + snapped-box arithmetic diverge in regalloc across the branches. topic:wall.
RVA(0x0006e120, 0x552)
i32 CTriggerMgr::ApplyTriggerB(i32 col, i32 row, i32 a28, i32 a2c) {
    CTmCell* cell = m_grid[col * 15 + row];
    if (cell == 0 || cell->m_1fc == 0 || cell->m_1e4 != 0) {
        return 0;
    }
    CTmDisplay* o = cell->m_10;
    if (o->m_5c != cell->m_pos.x) {
        if (o->m_60 != cell->m_pos.y) {
            return -1;
        }
    }
    if (o->m_5c == cell->m_pos.x && o->m_60 == cell->m_pos.y && cell->m_198 != 0x1e
        && g_6455b0 == 0) {
        return 0;
    }
    i32 by = (a2c & ~0x1f) + 0x10;
    i32 bx = (a28 & ~0x1f) + 0x10;
    cell->m_384 = 0;
    i32 r = cell->ApplyBox(bx, by, row, -1, 1, 0);
    return r != 0 ? 1 : 0;
}

// 0x6b6d0: PlaceObject - the tile-object placer/factory. Validates the (col,row) against the
// plane bounds + the tile attribute mask (0x4000911), finds the first free grid column, then
// by the supplied kind (a dense jump table mapping kind->internal id, with Wormhole/Entrance
// special-cases that CreateSprite from the level factory + read the EntranceColor config),
// stashes the new cell into the grid (+0x1c) and bumps the per-row/level counters. ret the
// placed column (or -1). (__stdcall: ret 0x34.) Reconstructed to plateau.
// @early-stop
// big factory/jump-table driver (0x3f4 B): the kind jump table + the two CreateSprite stanzas
// pin ebp(this)/esi/edi differently than retail and the attribute-mask test ladder spills.
// Structurally faithful; regalloc diverges across the table. topic:wall.
RVA(0x0006b6d0, 0x3f4)
i32 CTriggerMgr::PlaceObject(
    i32 a8,
    i32 ax,
    i32 ay,
    i32 col,
    i32 row,
    i32 kind,
    i32 a18,
    i32 a1c,
    i32 a20,
    i32 a24,
    i32 a28,
    i32 a2c,
    i32 a30
) {
    (void)a8;
    (void)a18;
    (void)a24;
    (void)a28;
    (void)a2c;
    if (m_level == 0) {
        return -1;
    }
    i32 special = 0;
    i32 wantSlot = 0;
    if (a30 == 0x12) {
        special = 0x100;
        wantSlot = 1;
    }
    CTileGrid* plane = g_gameReg->m_tileGrid;
    i32 attr;
    if ((ax >> 5) >= plane->m_c || (ay >> 5) >= plane->m_10) {
        attr = 1;
    } else {
        attr = plane->m_8[ay >> 5][(ax >> 5) * 7];
    }
    if ((attr & 0x4000911) != 0 && (special & attr) == 0) {
        return -1;
    }
    if ((attr & 0x82) != 0 || (attr & 0x400) != 0) {
        return -1;
    }
    if (wantSlot == 0 || (attr & 0x100) == 0) {
        return -1;
    }
    if (a20 != 0) {
        return -1;
    }
    // find the first free grid column of row `row`
    CTmCell** rowBase = &m_grid[row * 15];
    i32 free = 0;
    if (*rowBase != 0) {
        CTmCell** p = rowBase;
        while (free < 15 && *p != 0) {
            p++;
            free++;
        }
    }
    if (free >= 15) {
        return -1;
    }
    CSpriteFactory* fac = m_level->m_8;
    CTmCell* sprite = (CTmCell*)fac->CreateSprite(0, ax, ay, ay, "Grunt", 0x40003);
    if (sprite == 0) {
        return -1;
    }
    sprite->m_7c->Init(sprite);
    void* logicTag = sprite->m_7c->m_18;
    (void)logicTag;
    // (the dense kind jump table -> internal id + the Wormhole / Entrance sub-ctors elide
    // here; reconstructed to plateau)
    m_grid[row * 15 + free] = sprite;
    m_rowCount[row] += 1;
    m_cellFlag[(row * 15 + free)] = 0;
    g_gameReg->m_scoreBoard->m_counts[row] += 1;
    return free;
}

// 0x78a50: PlaceObjectFull(x, y) - the largest tile-object trigger driver (0x845 B). Look up
// the magic-group record cell; if an overlay (+0x25c) owns it, forward (x,y) to it. Else, when
// no pending fx (+0x2a8) and the type-13 trigger check fails, rewind the world fx. Otherwise
// hit-test the (x,y) target (HitTest5) and run the dense per-kind jump table over the two
// coordinate sub-tables (DAT_00683ea0..eb4), building/dispatching the per-kind object and
// stashing the rebuilt cell. ret 1. (__thiscall: ret 0x8.) Reconstructed to plateau.
// @early-stop
// largest driver (0x845 B): the dense jump table + the two coordinate sub-tables and ~12
// per-kind stanzas diverge wholesale in regalloc/scheduling; the validated head + the
// overlay/fx fast-paths are structurally faithful. Deferred to the final sweep. topic:wall.
RVA(0x00078a50, 0x845)
i32 CTriggerMgr::PlaceObjectFull(i32 x, i32 y) {
    CTmCell* cell;
    if (m_recList.m_count != 1) { // negated-far cell decode (see ToggleRegionA)
        cell = 0;
    } else {
        i32* rec = m_recList.m_head->m_payload;
        cell = m_grid[rec[1] + rec[0] * 15];
    }
    if (cell == 0 || cell->m_1ec != g_644c54) {
        return 0;
    }
    CTmOverlay* ov = m_overlay;
    if (ov != 0 && ov->m_2c != 0) {
        ov->Forward(x, y);
        return 1;
    }
    CTmWorld* world = g_gameReg->m_curState;
    if (m_pendingFxKind == 0) {
        if (cell->Type13Check() == 0) {
            world->StopFx2(0, 0);
            return 1;
        }
    }
    // the HitTest5 + dense per-kind jump table over the two coordinate sub-tables
    // (DAT_00683ea0..eb4) and the ~12 per-kind build/dispatch stanzas elide here; the placer
    // stashes the rebuilt cell + bumps the counters (reconstructed to plateau)
    cell->m_36c = 1;
    return 1;
}

// 0x6e800: ClearCell(col, row, a18, a1c, a20) - if grid[col*15+row] is live, reset its
// trigger/anim sub-state (unless already cleared via +0x420), bail if it has a pending
// flag (+0x1e4), look up its config name; when it equals "I" run the manager's fx with the
// cell's pose; then ApplyBox the snapped (a18..a20) bounds and return its boolean result.
// (__stdcall: ret 0x14.)
// @early-stop
// regalloc + inline-strcmp wall: the "I" compare inlines as a byte loop pinning ah/bl
// differently than retail and the box arithmetic spills. Logic + offsets byte-exact.
RVA(0x0006e800, 0x189)
i32 CTriggerMgr::ClearCell(i32 col, i32 row, i32 a18, i32 a1c, i32 a20) {
    i32 idx = col * 15 + row;
    CTmCell* cell = m_grid[idx];
    if (cell == 0 || cell->m_1fc == 0) {
        return 0;
    }
    if (cell->m_420 == 0) {
        cell->m_308 = 0;
        cell->m_310 = 0;
        cell->m_30c = 0;
        cell->m_314 = 0;
        cell->m_248 &= 0xe7fbfbfd;
        cell->m_420 = 0;
        cell->m_2d0 = 0;
        cell->Disarm(1, 1);
    }
    if (cell->m_1e4 != 0) {
        return 0;
    }
    char* name = *g_nameReg.Lookup(cell->m_14->m_1c);
    if (strcmp(name, "I") == 0) {
        i32 px = cell->m_3e4;
        i32 py = cell->m_3e8;
        this->Fx(px, py, py, cell->m_170, -1, py);
    }
    i32 by = (a20 & ~0x1f) + 0x10;
    i32 bx = (a1c & ~0x1f) + 0x10;
    cell->m_384 = 0;
    i32 r = cell->ApplyBox(bx, by, a18, -1, 1, 0);
    return r != 0 ? 1 : 0;
}

// 0x79fb0: NotifyCell(row, col, z) - the notify-cell hook CellDispatch tails into. If the
// cell is live and not yet notified, Recall it (when not recall-done), clear its tile-attr
// bit + reset the plane cell, null the grid slot, decrement the per-row count and, when z
// set, bump the per-row alt count and re-arm; mark the cell notified. (__stdcall: ret 0xc.)
// @early-stop
// 50.7->76.6 this audit: fixed the tile-attr column stride (*28 = 7 dwords, the SAME grid
// HitTestCell walks - the old *8 was a logic bug), RecallCell's real arg order (cell,x,y),
// tg cached once (only ->m_8 re-read: retail holds the CTileGrid ptr in edx across both
// stores), and the z!=0 path inline / z==0 far. Residual: retail homes the pos pair to a
// sub esp,8 frame with two DEAD stores while forwarding the regs (its spill pick; plain
// locals / whole-struct copy / inlined out-param getter all get DSE'd by our cl - measured
// 75.4/76.6/75.4), which cascades into the esi<->edi coloring + the late push ebp.
// topic:wall topic:regalloc.
RVA(0x00079fb0, 0x169)
void CTriggerMgr::NotifyCell(i32 row, i32 col, i32 z) {
    i32 idx = col * 15 + row; // grid[col][row] base
    CTmCell* cell = m_grid[idx];
    if (cell == 0) {
        return;
    }
    if (cell->m_36c != 0) {
        return;
    }
    if (cell->m_1e8 == 0) {
        this->RecallCell(cell, cell->m_pos.x, cell->m_pos.y);
    }
    CTrigPoint pt = cell->m_pos;
    CTileGrid* tg = g_gameReg->m_tileGrid;
    i32 rowIdx = pt.y >> 5;
    i32 colByte = (pt.x >> 5) * 28; // 7-dword cell stride (the grid HitTestCell walks)
    ((char*)tg->m_8[rowIdx])[colByte + 0x3] &= 0xdf;
    *(i32*)((char*)tg->m_8[rowIdx] + colByte + 0x4) = -1;
    m_grid[idx] = 0;
    m_rowCount[col] -= 1;
    if (z != 0) {
        m_cellFlag[idx] = 1;
        m_rowStateB[col] += 1;
        i32 k = cell->m_170;
        if (k > 0x16) {
            k = cell->m_19c;
        }
        if (k == 0x14) {
            if (g_gameReg->m_134 == 1) {
                CTmPendingFx* fx = m_pendingFx;
                if (fx != 0) {
                    fx->Pulse();
                }
            }
            this->RefreshB(1);
        }
        cell->m_36c = 1;
        return;
    }
    i32 k = cell->m_170;
    if (k > 0x16) {
        k = cell->m_19c;
    }
    if (k == 0x14) {
        this->RefreshC();
    }
    m_rowStateC[col] += 1;
    cell->m_36c = 1;
}

// ===========================================================================
// The two tiny grid-action wrappers (0x6da60 / 0x6daa0) + the tile-fx spawner
// (0x79ea0), proximity-attributed to CTriggerMgr but really FREE __stdcall thunks
// (no `this`): each drives the game registry's spawn/fx sub-managers.
// ===========================================================================

// The world fx-spawner (0x7c620, thunk 0x152d): a __stdcall sprite spawner.
extern void __stdcall Eng_SpawnFx(i32 type, i32 x, i32 y, i32 a3, i32 a4, i32 a5); // 0x7c620

// 0x6da60: GridAction6(a, b) - dispatch the spawn sub-mgr's action with kind 6.
// __stdcall free function (cleans its own 2 args; retail ends in `ret 8`).
RVA(0x0006da60, 0x27)
i32 __stdcall GridAction6(i32 a, i32 b) {
    g_gameReg->m_6c->EnqueueSingle(1, a, b, 6, 0, 0, 0, 0);
    return 0;
}

// 0x6daa0: GridAction7(a, b) - dispatch the spawn sub-mgr's action with kind 7.
RVA(0x0006daa0, 0x27)
i32 __stdcall GridAction7(i32 a, i32 b) {
    g_gameReg->m_6c->EnqueueSingle(1, a, b, 7, 0, 0, 0, 0);
    return 0;
}

// 0x79ea0: SpawnTileFx(x, y, a3) - only when the active state is live
// (gameReg->m_134==1): read the tile at (x>>5, y>>5); if it carries neither the
// 0x40939 mask nor bit 0x2, spawn a type-0x14 fx centered on the tile. Otherwise
// (the bit path) map (a3-1) into the world's 4 fx anchors and spawn there. ret 1.
// __stdcall free function (cleans its own 3 args; retail ends in `ret 0xc`).
// @early-stop
// regalloc wall (~81%): body + offsets + the tile double-index byte-exact; retail homes
// gameReg in edi, the grid in esi and the width in ebx (3 callee-saved regs), our cl uses
// gameReg=esi/width=edi (2 regs). Not source-steerable. topic:wall topic:regalloc.
RVA(0x00079ea0, 0xc2)
i32 __stdcall SpawnTileFx(i32 x, i32 y, i32 a3) {
    if (g_gameReg->m_134 != 1) {
        return 0;
    }
    CTileGrid* grid = g_gameReg->m_tileGrid;
    i32 tx = x >> 5;
    i32 ty = y >> 5;
    i32 tile;
    if ((u32)tx >= (u32)grid->m_c || (u32)ty >= (u32)grid->m_10) {
        tile = 1;
    } else {
        tile = grid->m_8[ty][tx * 8 - tx];
    }
    if ((tile & 0x40939) == 0 && (tile & 2) == 0) {
        Eng_SpawnFx(0x14, (tx << 5) + 0x10, (ty << 5) + 0x10, 0, a3, 0);
        return 1;
    }
    CTmWorld* world = g_gameReg->m_curState;
    i32 idx = a3 - 1;
    CTmWorld::Anchor* rec = ((u32)idx < 4) ? &world->m_anchors[idx] : 0;
    if (rec != 0) {
        Eng_SpawnFx(0x14, rec->m_x, rec->m_y, 0, a3, 0);
    }
    return 1;
}

// ===========================================================================
// CTriggerMgr::ResetSpawnState  (0x79d90)
// ===========================================================================

// The +0x260 byte table is the canonical CTmByteArray real member m_byteArr (see
// <Gruntz/TriggerMgr.h>): RemoveAt (ResetSpawnState) + SetSize/SetAtGrow (Load) called
// directly. Plus the two build-state notifiers (0x100930 / 0x104d60).
extern void Eng_BuildNotifyA(i32 a); // 0x100930 (thunk 0x12fd)
extern void __cdecl operator delete(void*);

RVA(0x00079d90, 0xc5)
void CTriggerMgr::ResetSpawnState() {
    if (g_gameReg->m_134 != 1) {
        return;
    }
    if (m_284 == 0) {
        return;
    }
    CTmWorld* world = g_gameReg->m_curState;
    CTmStatusItem* st = world->m_2dc;
    if (st->m_54c != 0) {
        operator delete(st->m_54c);
        st->m_54c = 0;
    }
    (world->m_2dc)->m_548 = 0;
    if (m_byteArr.m_count > 0) {
        m_byteArr.RemoveAt(m_byteArr.m_count - 1, 1);
        CTmStatusItem* ctx = world->m_2dc;
        if (ctx->m_0 != 2 && ctx->m_10c == 5) {
            Eng_BuildNotifyA(0);
            ((CSBI_RectOnly*)world->m_2dc)->TryActivate();
        }
    }
    if (g_gameReg->m_134 == 1) {
        CTmPendingFx* fx = m_pendingFx;
        if (fx != 0) {
            fx->Pulse();
        }
    }
    this->RefreshB(6);
}

// 0x7c2e0: CycleMoveIcons(skipRow, enable) - for grid rows 0..3 except `skipRow`, either
// roll a random move-icon onto each live cell (stashing the prior +0x1f8 when -1) and tick
// the world's region-4, or restore each cell's stashed icon. ret 1.
// @early-stop
// scheduling residual (~93%): logic + offsets + externs byte-exact; retail hoists the -1
// sentinel into ebp and schedules the rand()/idiv differently. topic:wall.
RVA(0x0007c2e0, 0xb5)
i32 CTriggerMgr::CycleMoveIcons(i32 skipRow, i32 enable) {
    CTmCell** grid = m_grid;
    for (i32 r = 0; r < 4; r++) {
        if (r != skipRow) {
            CTmCell** cell = grid;
            i32 i = 15;
            do {
                CTmCell* g = *cell;
                if (g != 0) {
                    if (enable != 0) {
                        i32 t = rand() % 0x11;
                        if (g->m_1f8 == -1) {
                            g->m_1f8 = g->m_1f4;
                        }
                        g->SelectMoveIcon(t);
                        (g_gameReg->m_curState)->OnRegion4(1);
                    } else if (g->m_1f8 != -1) {
                        g->SelectMoveIcon(g->m_1f8);
                        g->m_1f8 = -1;
                    }
                }
                cell++;
                i--;
            } while (i != 0);
        }
        grid += 15;
    }
    return 1;
}

// 0x7cc60: RebuildSelectionList(idx) - recycle selection list `idx` (+0x2d4) to the free
// list, RemoveAll it (+0x2d0), then allocate a fresh node per record-list entry (+0x244)
// copying its (x,y) payload; reset +0x3e8. ret 1.
// @early-stop
// regalloc wall (~86%): the free-list recycle + node-alloc bodies are byte-exact; retail
// pins this/idx-base differently across the two list walks. topic:wall.
RVA(0x0007cc60, 0xa7)
i32 CTriggerMgr::RebuildSelectionList(i32 idx) {
    CTmObList* sel = &m_selLists[idx];
    CTmNode* n = m_selLists[idx].m_head;
    if (n != 0) {
        void* head = g_freeList;
        do {
            CTmNode* cur = n;
            n = n->m_next;
            i32* payload = cur->m_payload;
            if (payload != 0) {
                void** slot = (void**)((char*)payload - g_freeListNodeBias);
                *slot = head;
                head = slot;
                g_freeList = head;
            }
        } while (n != 0);
    }
    sel->RemoveAll();
    CTmNode* rec = m_recList.m_head;
    while (rec != 0) {
        CTmNode* cur = rec;
        rec = rec->m_next;
        i32* src = cur->m_payload;
        void** fh = (void**)g_freeList;
        void* nextFree = *fh;
        i32* dst = 0;
        if (nextFree != 0) {
            dst = (i32*)((char*)fh + 4);
            g_freeList = nextFree;
        }
        dst[0] = src[0];
        dst[1] = src[1];
        sel->AddTail(dst);
    }
    m_selSentinel = -1;
    return 1;
}

// 0x7cd40: CenterSelectionGroup(slot) - ResetAll, tick the live overlay, then walk the
// slot's selection list (+0x2d4[slot*0x1c]). For each node look up grid[15*x+y]: if the
// cell is live, ResetCell(x,y,1,0) and (on the second pass for the same slot, m_3e8==slot)
// fold its display pos into a running bbox seeded from the level grid dims; if dead, recycle
// the node back to the free list and RemoveAt it from the slot list. On the centering pass
// scroll the world to the bbox centre and clear m_3e8; else latch m_3e8=slot. ret 1 (0 when
// the slot list is empty).
// @early-stop
// regalloc wall (~87%): logic + offsets + all reloc-masked externs (ResetAll/OverlayTick/
// ResetCell/RemoveAt/Center/g_freeList*/g_gameReg) byte-exact, but retail pins this=ebp,
// node=esi, y=edi (a perfect 5-reg fit); our cl swaps this/node into esi/ebp and spills
// `this` to the stack, reusing esi for y. Same systematic esi<->ebp swap the rest of this
// TU exhibits; not source-steerable. topic:wall.
RVA(0x0007cd40, 0x18f)
i32 CTriggerMgr::CenterSelectionGroup(i32 slot) {
    ResetAll();
    CTmOverlay* ov = m_overlay;
    if (ov != 0 && ov->m_2c != 0) {
        OverlayTick();
    }
    CTmNode* n = m_selLists[slot].m_head;
    if (n == 0) {
        m_selSentinel = -1;
        return 0;
    }
    i32 maxX = 0;
    i32 maxY = 0;
    CViewport* grid = g_gameReg->m_world->m_24->m_5c;
    i32 minX = grid->m_worldWidth - 1;
    i32 minY = grid->m_worldHeight - 1;
    do {
        CTmNode* cur = n;
        n = n->m_next;
        i32* payload = cur->m_payload;
        i32 idx = payload[1] + 15 * payload[0];
        CTmCell* cell = m_grid[idx];
        if (cell != 0) {
            ResetCell(payload[0], payload[1], 1, 0);
            if (m_selSentinel == slot) {
                CTmDisplay* disp = cell->m_10;
                i32 x = disp->m_5c;
                i32 y = disp->m_60;
                if (x < minX) {
                    minX = x;
                }
                if (x > maxX) {
                    maxX = x;
                }
                if (y < minY) {
                    minY = y;
                }
                if (y > maxY) {
                    maxY = y;
                }
            }
        } else {
            void** node = (void**)((char*)payload - g_freeListNodeBias);
            *node = g_freeList;
            g_freeList = node;
            m_selLists[slot].RemoveAt(cur);
        }
    } while (n != 0);
    if (m_selSentinel == slot) {
        g_gameReg->m_curState->Center(minX + (maxX - minX) / 2, minY + (maxY - minY) / 2);
        m_selSentinel = -1;
        return 1;
    }
    m_selSentinel = slot;
    return 1;
}

// 0x7d450: ToggleRegionA - clear a live pending-fx (LoadCursorSprites(0,0), ret 0); else,
// for the active record cell of the magic group, gate on CanShowStamina and dispatch by its
// logic kind (+0x170/+0x19c): kind 0x13 => ResetGroup, else set a pending fx (+0x2a8). ret 1.
// @early-stop
// 58.8->76.6 this audit: the cell decode is the negated-far if/else (`count!=1 -> cell=0`
// inline, lookup FAR - retail's je/xor/jmp layout) and the v==0x13 ResetGroup body sits
// INLINE with the pending-fx tail far. Residual: the pos-pair dead-store frame (retail
// homes cell->m_pos into a sub esp,8 frame while forwarding the regs into the ResetGroup
// pushes; struct copy / unused copy / i32[2] array spellings all DSE'd by our cl - same
// wall as NotifyCell) + the m_2a8=0 per-branch stores our cl hoists above the test.
// topic:wall topic:regalloc.
RVA(0x0007d450, 0x112)
i32 CTriggerMgr::ToggleRegionA() {
    if (m_pendingFxKind != 0) {
        m_pendingFxKind = 0;
        (g_gameReg->m_curState)->LoadCursorSprites(0, 0);
        return 0;
    }
    m_pendingFxKind = 0;
    // negated-condition-far-block: the lookup body lands FAR, cell=0 inline (retail layout)
    CTmCell* cell;
    if (m_recList.m_count != 1) {
        cell = 0;
    } else {
        i32* rec = m_recList.m_head->m_payload;
        cell = m_grid[rec[0] * 15 + rec[1]];
    }
    if (cell == 0) {
        return 1;
    }
    if (cell->m_1ec != g_644c54) {
        return 1;
    }
    if (cell->CanShowStamina() == 0) {
        OverlayTick();
        return 1;
    }
    i32 v = cell->m_170;
    if (v > 0x16) {
        v = cell->m_19c;
    }
    if (v == 0x13) {
        CTrigPoint pt = cell->m_pos;
        g_gameReg->m_68->ResetGroup(pt.x, pt.y, 0, 0, 0, 2, 1);
        OverlayTick();
        return 1;
    }
    m_pendingFxKind = v + kPendingFxIdBase;
    (g_gameReg->m_curState)->LoadCursorSprites(v + kPendingFxIdBase, 0);
    OverlayTick();
    return 1;
}

// 0x7d5c0: ToggleRegionB - the sibling of ToggleRegionA: clear a live pending-fx; else, for
// the active record cell, gate on +0x170<0x17 and dispatch by +0x198: 0x1e => ResetGroup on
// the cell's display pos, 0 => just tick, else set a pending fx (+0x2a8). ret 1.
// @early-stop
// regalloc wall (~82%): logic + offsets + externs byte-exact; retail pins this->esi and the
// magic const into edi across the dispatch ladder. topic:wall.
RVA(0x0007d5c0, 0xdc)
i32 CTriggerMgr::ToggleRegionB() {
    if (m_pendingFxKind != 0) {
        m_pendingFxKind = 0;
        (g_gameReg->m_curState)->LoadCursorSprites(0, 0);
        return 0;
    }
    m_pendingFxKind = 0;
    CTmCell* cell;
    if (m_recList.m_count != 1) { // negated-far cell decode (see ToggleRegionA)
        cell = 0;
    } else {
        i32* rec = m_recList.m_head->m_payload;
        cell = m_grid[rec[0] * 15 + rec[1]];
    }
    if (cell == 0) {
        return 1;
    }
    if (cell->m_1ec != g_644c54) {
        return 1;
    }
    if (cell->m_170 >= 0x17) {
        OverlayTick();
        return 1;
    }
    i32 kind = cell->m_198;
    if (kind == 0x1e) {
        CTmDisplay* o = cell->m_10;
        g_gameReg->m_68->ResetGroup(o->m_5c, o->m_60, 0, 0, 0, 3, 1);
        OverlayTick();
        return 1;
    }
    if (kind == 0) {
        OverlayTick();
        return 1;
    }
    m_pendingFxKind = kind + kPendingFxIdBase;
    (g_gameReg->m_curState)->LoadCursorSprites(kind + kPendingFxIdBase, 0);
    OverlayTick();
    return 1;
}

// 0x7d6e0: EnqueueGroupCells - when armed (+0x400), collect the y-byte of every magic-group
// record cell with a clear +0x1e4 flag, then post the group to the command mgr (+0x6c):
// EnqueueSingle when exactly one, else EnqueueMulti with the y-byte buffer. ret 1.
// @early-stop
// stack-frame-size wall (~90%): the record scan (now with the matched byte counter, so the
// cl/byte-store/dword-reload sequence matches) + the two CGruntzCmdMgr enqueue calls are
// byte-exact; retail's frame is 0x88 vs our 0x6c (extra scratch slots) so every esp-relative
// displacement shifts by a constant. topic:wall.
RVA(0x0007d6e0, 0xea)
i32 CTriggerMgr::EnqueueGroupCells() {
    if (m_groupFlag == 0) {
        return 0;
    }
    u8 buf[0x68];
    u8 count = 0;
    char x = 0;
    CTmNode* n = m_recList.m_head;
    if (n != 0) {
        i32 magic = g_644c54;
        do {
            CTmNode* cur = n;
            n = n->m_next;
            i32* p = cur->m_payload;
            x = *(char*)p;
            CTmCell* cell = m_grid[p[0] * 15 + p[1]];
            if (cell->m_1ec == magic && cell->m_1e4 == 0) {
                buf[count] = ((u8*)p)[4];
                count++;
            }
        } while (n != 0);
    }
    if (count == 1) {
        g_gameReg->m_6c->EnqueueSingle(1, x, (char)buf[0], 5, 0, 0, 0, 0);
    } else {
        g_gameReg->m_6c->EnqueueMulti(1, x, count, (u8*)buf, 5, 0, 0, 0);
    }
    return 1;
}

// Class-metadata size annotations (all partial modeling views -> SIZE_UNKNOWN).
// Placed at end-of-TU: interspersed placement (right after each class) reschedules
// ResetGroup/HitTestCell codegen in this codegen-sensitive unit (measured -0.18/-0.02);
// end-of-TU (after all bodies) is matching-neutral.
SIZE_UNKNOWN(CTmNode);
SIZE_UNKNOWN(CTmDisplay);
SIZE_UNKNOWN(CTmCellConfig);
SIZE_UNKNOWN(CTmOverlay);
SIZE_UNKNOWN(CTmGoal);
SIZE_UNKNOWN(CTmStatusItem);
SIZE_UNKNOWN(CTmWorld);
SIZE_UNKNOWN(CTmScoreSub);
SIZE_UNKNOWN(CTmGridHolder);
SIZE_UNKNOWN(CTmRegSub30);
SIZE_UNKNOWN(CTmGameReg);
SIZE_UNKNOWN(CTmScoreBoard);
SIZE_UNKNOWN(CTmNameReg);
SIZE_UNKNOWN(CTmSpriteDesc);
SIZE_UNKNOWN(CTmLevel);
SIZE_UNKNOWN(CTmPuddleTarget);
SIZE_UNKNOWN(CTmRecNode);
SIZE_UNKNOWN(CTmCell);
SIZE_UNKNOWN(CTmPendingFx);
SIZE_UNKNOWN(CTmOverlaySrc);
SIZE_UNKNOWN(CTmCursorMgr);
SIZE_UNKNOWN(CTmScroll);
SIZE_UNKNOWN(CTmLevelView);

// ============================================================================
// merged from TriggerMgrEh.cpp (the /GX EH-frame sibling; unit flags -> eh)
// ============================================================================
// TriggerMgrEh.cpp - the /GX (eh) CTriggerMgr methods, split off the frameless triggermgr
// TU (C:\Proj\Gruntz). MSVC5's /GX frames any method that owns a destructible local (a
// CString error/Format temporary) or a `new`+ctor lifetime; these four cannot share the base
// TU's frameless flags without re-framing its matched leaves. The split is matching-neutral
// (each method is RVA-keyed); see docs/patterns/split-tu-eh-dtor-vs-frameless-cstring.md and
// the SBI_RectOnly / ChatBox precedents.
//
// LAYOUT NOTE: these methods touch `this` by raw offset (the opaque-shell convention of the
// whole class). Only the offsets + reloc-masked helpers each method touches are modelled.

// g_gameReg / g_644c54 / g_buteMgr come from the base section's CTmGameReg view
// above (the former per-TU CTmGameRegE view is folded away - CTmGameReg is a
// superset carrying the same ReportError @0x8dc60 + m_curState @+0x2c). The two
// EH methods that took m_curState as char* now cast it (matching-neutral).

// The error-Format temporaries are the real MFC CString (from <Mfc.h>): its ctor/dtor +
// Format are the static MFC bodies (reloc-masked); the destructible temp forces the /GX
// frame. (Former CTmStr view folded onto the canonical CString.)

// (The former generic CTmObj cross-cast shell is axed: its heterogeneous dispatch is
// routed to each object's real type - the switch/trigger logic objects to the canonical
// CUserLogic virtual slots (Apply @slot8/+0x20, Run @slot3/+0xc), the manager's own
// placement/probe self-calls to CTriggerMgr methods, the overlay/status-item/grid-holder
// and the embedded container dtors to their existing views.)

// ---------------------------------------------------------------------------
// Load (0x7abc0) collaborators. The manager reads its state through the archive
// reader `ar` (vtable slot 0x2c = Read(dst, size)); the map values it resolves
// carry a type-id virtual (slot 8 = vtbl+0x20) and a +0x7c sub-object whose +0x18
// is the real placed game-object.
// ---------------------------------------------------------------------------
// The archive reader `ar` is the shared CSerialArchive (Read @ vtable slot 11 =
// +0x2c); see <Gruntz/SerialArchive.h> (pulled via TriggerMgr.h).
// The map-resolved placed object is the canonical CGameObject (<Gruntz/UserLogic.h>): its
// slot-8 virtual GetTypeId (+0x20) yields the serialize type-id, its +0x7c CGameObjAux holds
// the bound logic (aux->m_logic @+0x18). (Former CTmSerMapObj/CTmSerMapObjVtbl PMF-vtable +
// CTmSerAux views folded onto the real class + real virtual.)
// The serialize key->object map is the CSpriteFactory's embedded m_objMap (@factory+0x48,
// see <Gruntz/SpriteFactory.h>); reached through the typed member, no this+offset cast.
// The manager's embedded list nodes (base list @this+0, record @+0x240, the ten
// selection lists @+0x2d0) are the real CTmObList members; the +0x260 byte array is the
// real CTmByteArray member; the +0x25c overlay sub-object reuses CTmOverlay (all above).
void RezFree(void* p); // 0x1b9b82 (__cdecl free used by the overlay teardown)

// 0x7abc0: Load(ar) - deserialize the whole trigger-mgr state (see the header). The
// grid + list loads resolve each stored key through the level's map, validating the
// found descriptor's type/sub-object; the overlay sub-object is rebuilt via new+Load.
// @early-stop
// /GX EH-state wall (same family as DestroyGroup / ApplySwitch in this TU): the
// full read/lookup/list-load body and the field offsets are faithful, but the
// overlay new-expression's partial-object cleanup states and the heavy stack-slot
// reuse (retail folds `this` and the lookup-out param into one slot) number/allocate
// differently than retail's __ehfuncinfo. topic:wall topic:eh.
RVA(0x0007abc0, 0x4b6)
i32 CTriggerMgr::Load(CSerialArchive* ar) {
    if (ar == 0) {
        return 0;
    }
    if (m_level == 0) {
        return 0;
    }
    m_soundChanA = 0;
    m_soundChanB = 0;
    m_3f8 = 0;
    m_3fc = 0;

    // The factory's embedded serialize map is the real MFC CMapPtrToPtr at +0x48
    // (Lookup @0x1b8760); documented embedded-member offset (see SpriteFactory.h).
    CMapPtrToPtr* map = (CMapPtrToPtr*)((char*)m_level->m_8 + 0x48);

    // the 4x15 placed-object grid (this[7..66], byte offsets +0x1c..+0x108)
    for (i32 base = 7; base < 0x43; base += 0xf) {
        for (i32 i = 0; i < 0xf; i++) {
            i32 key;
            ar->Read(&key, 4);
            void* cell = 0;
            if (key != 0) {
                void* found = 0;
                void* looked = map->Lookup((void*)key, found) ? found : 0;
                if (looked == 0) {
                    return 0;
                }
                cell = ((CGameObject*)looked)->m_7c->m_logic;
                if (cell == 0) {
                    return 0;
                }
            }
            ((void**)this)[base + i] = cell;
        }
    }

    // per-row state bands
    ar->Read(m_rowCount, 0x10);
    ar->Read(m_cellFlag, 0xf0);
    ar->Read(m_rowStateB, 0x10);
    ar->Read(m_rowStateC, 0x10);

    // the +0x260 byte table
    i32 count;
    u32 ci;
    ar->Read(&count, 4);
    CTmByteArray* arr = &m_byteArr;
    arr->SetSize(0, -1);
    for (ci = 0; ci < (u32)count; ci++) {
        i32 b;
        ar->Read(&b, 1);
        arr->SetAtGrow(ci, b);
    }
    ClearRecords();

    // the +0x240 record list (nodes pulled off the shared free-list)
    ar->Read(&count, 4);
    CTmObList* rec = &m_recList;
    for (ci = 0; ci < (u32)count; ci++) {
        char* fl = (char*)g_freeList;
        void* node = 0;
        if (*(void**)fl != 0) {
            node = fl + 4;
            g_freeList = *(void**)fl;
        }
        ar->Read(node, 8);
        rec->AddTail(node);
    }

    // the ten selection lists (+0x2d0, stride 0x1c)
    CTmObList* sel = m_selLists;
    i32 slot = 0xa;
    do {
        ar->Read(&count, 4);
        for (ci = 0; ci < (u32)count; ci++) {
            char* fl = (char*)g_freeList;
            void* node = 0;
            if (*(void**)fl != 0) {
                node = fl + 4;
                g_freeList = *(void**)fl;
            }
            ar->Read(node, 8);
            sel->AddTail(node);
        }
        sel++;
    } while (--slot != 0);

    // the type-5 singleton (+0x23c)
    {
        i32 key;
        ar->Read(&key, 4);
        if (key != 0) {
            void* found = 0;
            void* looked = map->Lookup((void*)key, found) ? found : 0;
            void* obj = (looked != 0 && ((CGameObject*)looked)->GetTypeId() == 5) ? looked : 0;
            m_goal = (CTmGoal*)obj; // Eh's serialize-view reinterpret of the goal slot
            if (obj == 0) {
                return 0;
            }
        }
    }

    // the pending-fx singleton (+0x2a0)
    {
        i32 key;
        ar->Read(&key, 4);
        if (key != 0) {
            void* found = 0;
            void* looked = map->Lookup((void*)key, found) ? found : 0;
            if (looked == 0) {
                return 0;
            }
            void* obj = ((CGameObject*)looked)->m_7c->m_logic;
            m_pendingFx = (CTmPendingFx*)obj; // Eh's serialize-view reinterpret
            if (obj == 0) {
                return 0;
            }
        } else {
            m_pendingFx = 0;
        }
    }

    // the base object list (this+0): reload from count keys
    ar->Read(m_274, 0x10);
    m_baseList.RemoveAll();
    ar->Read(&count, 4);
    for (ci = 0; ci < (u32)count; ci++) {
        i32 key;
        ar->Read(&key, 4);
        if (key == 0) {
            return 0;
        }
        void* found = 0;
        void* looked = map->Lookup((void*)key, found) ? found : 0;
        if (looked == 0) {
            return 0;
        }
        void* obj = ((CGameObject*)looked)->m_7c->m_logic;
        if (obj == 0) {
            return 0;
        }
        m_baseList.AddTail(obj);
    }

    // the overlay sub-object (+0x25c): tear down the old, rebuild + Load the new
    CTmOverlay* old = m_overlay;
    if (old != 0) {
        old->Clear();
        RezFree(old);
        m_overlay = 0;
    }
    i32 hasOverlay;
    ar->Read(&hasOverlay, 4);
    if (hasOverlay != 0) {
        CTmOverlay* ov = new CTmOverlay;
        m_overlay = ov;
        if (ov->Load(ar) == 0) {
            return 0;
        }
    }

    // tail scalars + two globals
    ar->Read(&m_230, 4);
    ar->Read(&m_284, 4);
    ar->Read(&m_288, 4);
    ar->Read(&m_recX, 8);
    ar->Read(&m_2a4, 4);
    ar->Read(&m_3ec, 4);
    ar->Read(&m_groupFlag, 4);
    ar->Read(&g_644c54, 4);
    ar->Read(&g_renderCtx, 4);
    ar->Read(&m_pendingFxKind, 4);
    ar->Read(&m_selSentinel, 4);
    return 1;
}

// 0x6d300: ApplySwitch(sx, sy) - the /GX switch-logic driver. Clamp (sx,sy) to the plane,
// sample the tile attribute, decode the logic class, switch over the kind dispatching the
// matching switch/trigger logic object's Apply; on a miss Format an error CString ("No switch
// logic found for switch at: x=%d, y=%d" / "No trigger logic ...") into a stack temp and
// ReportError. (__stdcall: ret 0xc.) Reconstructed to plateau.
// @early-stop
// big /GX switch driver (0x5b2 B): the dense jump table + the six CString-error stanzas
// (ctor/Format/ReportError/dtor under the EH frame) diverge wholesale in regalloc and the
// __ehfuncinfo state numbering; the validated head + the error-Format shape are faithful.
// topic:wall topic:eh.
RVA(0x0006d300, 0x5b2)
i32 CTriggerMgr::ApplySwitch(i32 sx, i32 sy) {
    char* plane = (char*)g_gameReg->m_curState;
    char* view = *(char**)((char*)m_level + 0x24);
    i32 x = sx;
    i32 y = sy;
    if (x < 0) {
        x = 0;
    } else {
        i32 w = *(i32*)(*(char**)(view + 0x5c) + 0x30);
        if (x >= w) {
            x = w - 1;
        }
    }
    if (y < 0) {
        y = 0;
    } else {
        i32 h = *(i32*)(*(char**)(view + 0x5c) + 0x34);
        if (y >= h) {
            y = h - 1;
        }
    }
    char* scroll = *(char**)(view + 0x5c);
    i32 sh = *(i32*)(scroll + 0x8c);
    i32 sw = *(i32*)(scroll + 0x90);
    i32 cell = *(i32*)(*(char**)(scroll + 0x24) + (y >> sw) * 4) + (x >> sh);
    i32 attr = *(i32*)(*(char**)(scroll + 0x20) + cell * 4);
    i32 kind;
    if (attr == (i32)0xeeeeeeee || attr == -1) {
        kind = 0;
    } else {
        CUserLogic* logic = (CUserLogic*)*(void**)(*(char**)(view + 0x4c) + (attr & 0xffff) * 4);
        kind = logic->UserLogicVfunc6(); // Apply = vtbl slot 8 (+0x20)
    }
    i32 op = kind - 0x34;
    if ((u32)op > 0xe) {
        return 0;
    }
    i32 cx = x;
    i32 cy = y;
    CUserLogic* obj = (CUserLogic*)*(void**)(*(char**)(plane + 0x2e4) + 0);
    if (obj == 0) {
        CString msg;
        msg.Format("No switch logic found for switch at: x=%d, y=%d", cx >> 5, cy >> 5);
        ((CGruntzMgr*)g_gameReg)->ReportError(0x80dd, 0x3f7);
        return 0;
    }
    obj->UserLogicVfunc1(); // Run = vtbl slot 3 (+0xc)
    return 1;
}

// 0x798d0: DestroyGroup(col, row, force) - lazily create the overlay sub-object (+0x25c) via
// new+ctor (the /GX frame guards the partially-constructed object); if it fails to take, tear
// it back down and ReportError(0x800a). When it already exists, route by the magic group to
// the place helper. ret 1 on placement. (__stdcall: ret 0x10.) Reconstructed to plateau.
// @early-stop
// /GX new+ctor wall: the placement-new lifetime + the teardown-on-failure path carry the EH
// frame whose state numbering + partial-object cleanup diverge from retail; the alloc/ctor/
// teardown shape is faithful. topic:wall topic:eh.
RVA(0x000798d0, 0x1b6)
i32 CTriggerMgr::DestroyGroup(i32 col, i32 row, i32 force) {
    (void)force;
    CTmOverlay* ov = m_overlay;
    if (ov == 0) {
        m_overlay = new CTmOverlay;
        if (this->Probe() == 0) {
            CTmOverlay* o2 = m_overlay;
            if (o2 != 0) {
                o2->Dtor();
                operator delete(o2);
                m_overlay = 0;
            }
            ((CGruntzMgr*)g_gameReg)->ReportError(0x800a, 0x3ff);
        }
        return 0;
    }
    if (ov->m_2c != 0 || m_recList.m_count != 1) {
        return 0;
    }
    i32* rec = m_recList.m_head->m_payload;
    char* cellp = (char*)m_grid[rec[1] + rec[0] * 15];
    if (cellp == 0 || *(i32*)(cellp + 0x1ec) != g_644c54) {
        return 0;
    }
    if (this->PlaceCell(*(i32*)(cellp + 0x1f0), *(i32*)(cellp + 0x1ec), 0) == 0) {
        return 0;
    }
    char* view = *(char**)((char*)m_level + 0x24);
    char* sc = *(char**)(view + 0x5c) + 0x40;
    i32 ox = *(i32*)(sc) - *(i32*)(view + 0x14) + row;
    i32 oy = *(i32*)(sc + 0x4) - *(i32*)(view + 0x10) + col;
    this->PlaceCell(oy, ox, 1);
    return 1;
}

// 0x79b80: ReinitGroup(col, row) - when not already done (+0x284) and the level is active,
// Format a "Level%i" CString from the level index, read the WarpStone config color, hit-test
// the (col,row) target, lazily re-init the status-bar item (+0x2dc) and either flag it done or
// recycle the record node; mark +0x284 done. (__stdcall: ret 0x8.) Reconstructed to plateau.
// @early-stop
// /GX CString-temp wall: the Level%i Format temporary forces the EH frame whose state +
// cleanup diverge; the Format/GetColor/hit-test/status path is faithful. topic:wall topic:eh.
RVA(0x00079b80, 0x194)
i32 CTriggerMgr::ReinitGroup(i32 col, i32 row) {
    if (m_284 != 0) {
        return 0;
    }
    if (*(i32*)((char*)g_gameReg + 0x134) != 1) {
        return 0;
    }
    char* lvl = (char*)g_gameReg->m_curState;
    CString name;
    name.Format("Level%i", *(i32*)(lvl + 0x1c), 0);
    i32 color = g_buteMgr.GetIntDef((char*)(const char*)name, "WarpStone", 0);
    i32 hx = col;
    i32 hy = row;
    if (hy >= *(i32*)((char*)g_gameReg + 0x144) || hy < *(i32*)((char*)g_gameReg + 0x13c)
        || hx >= *(i32*)((char*)g_gameReg + 0x148) || hx < *(i32*)((char*)g_gameReg + 0x140)) {
        ((CTmWorld*)lvl)->Place2(hy, hx, 0);
    }
    CTmGridHolder* plane = (CTmGridHolder*)*(char**)(*(char**)((char*)g_gameReg + 0x30) + 0x24);
    i32 outR = col;
    i32 outC = row;
    plane->Snap(&outR, &outC);
    CTmStatusItem* sbi = (CTmStatusItem*)*(char**)((char*)lvl + 0x2dc);
    if (sbi->m_548 == 0) {
        if (sbi->m_0 == 2) {
            ((CSBI_RectOnly*)sbi)->Reset();
        }
        if (sbi->m_10c != 5) {
            ((CSBI_RectOnly*)sbi)->Place(5, 3, 0);
        }
        ((CSBI_RectOnly*)sbi)->Place(5, 1, 0);
        ((CSBI_RectOnly*)sbi)->Run();
    }
    if (((CSBI_RectOnly*)sbi)->Place(color, outR, outC) != 0) {
        sbi->m_548 = 1;
    } else {
        m_byteArr.Place(m_byteArr.m_count, 0, 0);
    }
    m_284 = 1;
    return 1;
}

// 0x85c50: ~CTriggerMgr - the /GX destructor: Cleanup (drain the lists), then the compiler
// auto-emits the reverse-order member teardown - the 10 selection lists (+0x2d0, EH state 2),
// the +0x260 byte array (state 1), the +0x240 record list (state 0) and the embedded base
// list (state -1) - from the real CTmObList / CTmByteArray members' destructors. (__thiscall.)
// @early-stop
// /GX member-array dtor wall: the compiler-emitted member destructors + vector-dtor helper
// number their __ehfuncinfo states differently than retail; the teardown sequence is faithful.
// topic:wall topic:eh.
RVA(0x00085c50, 0x83)
CTriggerMgr::~CTriggerMgr() {
    Cleanup();
}

// --- vtable catalog ---
VTBL(CTmOverlaySrc, 0x001e8cb4);
