#ifndef SRC_GRUNTZ_TRIGGERMGR_H
#define SRC_GRUNTZ_TRIGGERMGR_H
#include <rva.h>
#include <Mfc.h>                  // CPtrList and the MFC list helpers (reloc-masked)
#include <Gruntz/SerialArchive.h> // CFileMemBase - the Load serializer's stream (Read @ +0x2c)

#include <Gruntz/FreeNodePool.h> // the coord-node pool object @0x645540
extern FreeNodePool g_coordPool;

void operator delete(void*);

typedef enum TmGridDim {
    TM_GRID_COLS = 15, // cells per row (the m_grid/m_cellFlag row stride)
    TM_GRID_ROWS = 4,  // rows (m_rowCount/m_rowStateB/m_rowStateC are per-row)
} TmGridDim;

#include <Gruntz/CoordNode.h>

class CGrunt;
struct CGameObject; // <Gruntz/UserLogic.h> - what CDDrawChildGroup::CreateSprite returns

class CDDrawSurfaceMgr;
class DirectSoundMgr; // Dsndmgr/DirectSoundMgr.h (StopAndRewind)
struct CTmNode;
struct CTmRecNode;
struct CTmOverlay;     // the allocated overlay sub-object (+0x25c); completed in each TU
class CWwdGameObjectA; // <Wwd/WwdGameObjectFamily.h> - the goal camera sprite's real class
class CActionOptionsMenuBar;

// The ELEMENT type of the base object list (m_baseList, +0x000): the battlez spawn
// machine's "grid candidate" - a grid (x,y) at +0x54/+0x58 and an occupied flag at
// +0x5c. Retail proof (ProbeUnoccupiedAt @0x35210, byte-exact): `[[this+4]+0x68]` (the
// level's trigger mgr) `-> [+0x4]` (the list's head slot) then, per node, `[node+8]`
// and `cmp [elem+0x54],x / cmp [elem+0x58],y / mov [elem+0x5c]`.
// @identity-TODO: the RTTI class is NOT recovered - the element carries no vptr store
// on any path this TU sees, and it is NOT a CGrunt (whose +0x54 is a CString body) nor
// a CGameObject (whose +0x54..+0x5c are the draw-fill triple). It was BattlezMapConfig
// .cpp's local `GridCand`; promoted here (onto the list that owns it) rather than kept
// as a per-TU view. Its cells are plain MFC CPtrList nodes (no separate node type).
//
// MERGED VIEWS (2026-07-14): the ex-`CTmPuddleTarget` (TriggerMgrViews.h) and the
// ex-`ResGrunt` (TriggerMgr.cpp resurrect pass) were BOTH this same baseList element:
// identical +0x54/+0x58 grid pair + +0x5c occupied gate, plus the +0x38 bound-object
// slot (leaf->m_38->m_8 |= 0x10000 "released" - the CTileLogic-tail bound-object idiom),
// the +0x68 grunt-type index and the +0x6c spawn-host word the resurrect pass feeds to
// PlaceObject. Union of the three views' knowledge, one shape.
// RESOLVED IDENTITY (ex the `CTmCandidate` @identity-TODO view): the baseList
// element IS the CGruntPuddle logic leaf (<Gruntz/GruntPuddle.h>). Proof:
//   * PlacePuddle's placement driver on the element is ?Place@CGruntPuddle@@QAEHHHHH@Z
//     @0x40c30 (the retail rel32 - the view's "@0x9c3f0" note was stale/wrong);
//   * the view's +0x54/+0x58 grid pair + +0x5c gate are exactly CGruntPuddle's
//     m_tileX/m_tileY/m_pending, its +0x38 bound object is the inherited
//     TILE_LOGIC_TAIL m_38 (CGameObject*, whose m_flags takes the 0x10000 released
//     bit), and +0x68/+0x6c are the Place() a0/a1 snapshots the resurrect pass
//     reads back (m_gruntType -> PlaceObject type, m_placeIndex -> PlaceObject a6);
//   * PlacePuddle AddTail()s the SAME object it Place()d, closing the element type.
// Game semantics agree: a dead grunt leaves a goo puddle; the spawn/resurrect
// scans walk the placed puddles. Consumers include GruntPuddle.h for the members.
class CGruntPuddle;

class CTriggerMgr {
public:
    void* Spawn(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f, i32 g);
    // 0x7abc0: Load(ar) - deserialize the whole trigger-mgr state from the reader
    // (the 4x15 placed-object grid, per-row state bands, byte table, record list,
    // ten selection lists, base object list, overlay sub-object + tail scalars).
    // /GX; lives in the eh sibling TU. ret 1, or 0 on any missing referent.
    i32 Load(CFileMemBase* ar);

    // --- the small reconstructed leaf interface (retail-RVA order) -------------
    // 0x759e0: copy the cached origin pair (+0x174,+0x178) into the caller's
    // out-slot and return it (ret 4 -> callee cleans the out-ptr arg).
    Coord* GetOriginXY(Coord* out); // 0x759e0

    // 0x6b640: store the supplied world holder at +0x22c, clear m_armed + m_pendingFx
    // and raise m_countdownActive; returns 1 (0 when arg is null).
    i32 SetLevel(CDDrawSurfaceMgr* lvl); // 0x6b640

    // 0x788d0 (ILT 0x1398): centre the active plane's scroll origin on the selected
    // record cell's bound object (parallax-scaled unless the plane is origin-fixed).
    // CMulti::PumpB fires it when m_armed. (Ex-CSnd788d0 "sound-emitter" view.)
    i32 ScrollToActiveRecord();

    // 0x78a30: forward to the overlay sub-object's helper when present, else ret.
    void OverlayTick();
    // (The ex-`AddGrunt` 13-arg decl is GONE: ILT 0x40bb lands on ?PlaceObject@
    // CTriggerMgr@@ @0x6b6d0 - it was a phantom alias; Play.cpp calls PlaceObject.
    // The ex-`Reset1b48a6` decl is GONE the same way: 0x1b48a6 is the MFC
    // ?RemoveAll@CPtrList@@ on the +0 m_baseList.)

    // 0x79b00: forward-and-free the overlay sub-object when present; ret 1.
    i32 OverlayRelease();

    // 0x79b30: linear search of the byte table (+0x264, count +0x268) for `b`;
    // ret 1 on hit else 0.
    i32 ByteTableHas(i32 b);

    // 0x78430: ResetAll - drain the record list (+0x244), clearing each referenced
    // grid cell's sprites and recycling the node; RemoveAll the +0x240 list, run
    // StopPendingFx, then flag the goal (+0x23c).
    void ResetAll();

    // 0x784d0: scan the record list (+0x244) for a node whose payload (x,y)
    // matches; ret 1 on hit else 0.
    i32 RecordListHas(i32 x, i32 y);

    // 0x7d140: ClearRow(row) - for the 15 cells of grid row `row` (+0x1c), run each
    // live cell's ExitGrid (unless it has a notify hook); clear +0x400 when row is the
    // magic group, then refresh the world. ret 1.
    i32 ClearRow(i32 row);

    // 0x7d2a0: only when `key` equals the magic kind (g_curPlayer), scan the 10 selection
    // lists (+0x2d4, stride 0x1c) for a node payload matching (key,y); ret the list
    // index of the first match (0xa when a second match exists) or 0.
    i32 SelectionListFind(i32 key, i32 y);

    // 0x7be10: when a pending overlay-fx (+0x2a8) is live, or the world has its own
    // pending flag (world+0x504), stop the world's fx and clear +0x2a8.
    void StopPendingFx();

    // 0x7d0c0: empty all 10 selection lists (+0x2d0, stride 0x1c) back to the free
    // list and reset the +0x3e8 sentinel to -1.
    void ClearSelections();

    // 0x78880: drain the record list (+0x244) back to the free list, then RemoveAll
    // the +0x240 MFC pointer list.
    void ClearRecords();

    // 0x6bc20: scan the cell grid (+0x1c) for the cell pointer == `obj` (startRow, or
    // rows 0..3 when startRow==5) and dispatch it via CellDispatch(row,col,kind,arg);
    // ret 0 when `obj` is not placed. (callee-cleans: ret 0x10.)
    i32 DispatchCellForObject(CGrunt* obj, i32 startRow, i32 kind, i32 arg);

    // 0x6bcb0: grid-cell dispatch - looks up cell[row*15+col] (+0x1c) and, if it has
    // a +0x368 hook, runs this->NotifyCell(row,col,0) (ret 0); else routes by `kind`
    // to two cell helpers (ret 1). ret 0 when no cell. (callee-cleans: ret 0x10.)
    i32 CellDispatch(i32 row, i32 col, i32 kind, i32 arg);

    // 0x79fb0: the notify-cell hook CellDispatch tails into when a cell has a +0x368
    // hook. UNRECONSTRUCTED (still a stub); declared so CellDispatch's reloc-masked
    // self-call mangles onto this class. No body in this TU.
    void NotifyCell(i32 row, i32 col, i32 z);

    // 0x6bea0: scan the cell grid (startRow, or rows 0..3 when startRow==5) for the
    // cell whose 30x30 object bounds contain (px,py); writes the found (row,col)
    // through the out-ptrs and returns the cell pointer (0 when none). (ret 0x14.)
    void* CellHitTest(i32 px, i32 py, i32* outRow, i32* outCol, i32 startRow);

    // 0x6be30: ScreenToCell - translate (sx,sy) by the level view's scroll origin and
    // forward to CellHitTest(startRow). (__stdcall: ret 0x14.)
    void* ScreenToCell(i32 sx, i32 sy, i32* outRow, i32* outCol, i32 startRow);

    // 0x6b680: Cleanup - destruct+free the overlay sub-object (+0x25c) when present,
    // then drain the record (+0x244) and selection (+0x2d0) lists.
    void Cleanup();

    // 0x7d1d0: NearestCellDist - the minimum squared (>>5) distance from (px,py) to any
    // live, clickable grid cell, skipping grid row `skipRow`. (__stdcall: ret 0xc.)
    i32 NearestCellDist(i32 skipRow, i32 px, i32 py);

    // 0x7d330: DestroyAllAnims - DestroyAnims on every live grid cell (4x15), clear the
    // +0x200 marker on every list object of the level's matching type, and stop the
    // three sound channels (+0x3f0, +0x3f4, and the active grunt's +0x618).
    void DestroyAllAnims();

    // 0x6bd40: ClearGridRange(startRow) - ResetAll, then for rows startRow..3 flag each
    // live cell's goal (+0x154) done, null the cell + its parallel grid slot (+0x11c)
    // and per-row state (+0x10c/+0x20c/+0x21c); finally ClearSelections. ret 1.
    i32 ClearGridRange(i32 startRow);

    // 0x7a510: ClearRowAndRefresh(startRow) - Recall every live, hook-less cell of rows
    // startRow..3; clear +0x400 when startRow is the magic group; refresh the world,
    // bump a world stat, and re-arm the status item. ret 1.
    i32 ClearRowAndRefresh(i32 startRow);

    // 0x77f80: FindNearestInRow(g) - scan the grid row g->m_tileOwnerHi (15 cells) for the live
    // cell whose display object is nearest g's tile pos (g->m_17c/m_180 >> 5), within the
    // squared cutoff 2*g->m_defenderRadius; ret the nearest cell pointer or 0. (__thiscall: ret 4.)
    CGrunt* FindNearestInRow(CGrunt* g);

    // 0x78260: RemoveCellRecord(x,y,fromSelection) - unlink the (x,y) node from the
    // selection lists (when fromSelection) and from the record list, clearing the cell's
    // sprites / goal / overlay along the way. ret 1 if a record was removed. (ret 0xc.)
    i32 RemoveCellRecord(i32 x, i32 y, i32 fromSelection);

    // 0x7a180: SpawnPuddle(x,y,...) - create+init a "GruntPuddle" sprite, stash its
    // placement fields, then PlacePuddle it. ret 0 on factory failure. (ret 0x18.)
    i32 SpawnPuddle(i32 x, i32 y, i32 f124, i32 f114, i32 color, i32 f118);

    // 0x7a240: PlacePuddle(sprite, color) - place the puddle via its CUserLogic and, on
    // success, flag/remove the matching record + selection nodes. ret 1 on success.
    i32 PlacePuddle(CGameObject* sprite, i32 color);

    // ---- the remaining reconstructed methods (retail-RVA order) ----------------

    // 0x6b6d0: PlaceObject - the big tile-object placer/factory switch: validate the
    // (col,row,kind), look up the level type-table entry, dispatch the per-kind
    // sub-ctor (CreateSprite + Init for Wormhole/Entrance variants, a jump table by
    // kind), stash the new cell into the grid (+0x1c) and bump the per-row/level
    // counters. (__stdcall: ret 0x34.)
    i32 PlaceObject(
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
    );

    // 0x6bfd0: ResetCell(col, row, force) - if grid[col*15+row] (+0x1c) is a live cell,
    // either reset its switch/trigger sub-state (force / non-magic) or, when it is the
    // magic group, recycle the (row,col) record node and unlink it from the +0x240 list.
    i32 ResetCell(i32 col, i32 row, i32 force, i32 keep);

    // 0x78960: LoadCameraSprite - the SBI_RectOnly cursor-place path fires this after
    // latching the placed (row,col). Declared-only (reloc-masked; Ghidra mis-attributes
    // it to EngineLabelBacklog, but it is dispatched thiscall on this trigger mgr).
    i32 LoadCameraSprite();

    // 0x6d300: ApplySwitch - the /GX switch-logic driver. Clamp (sx,sy) to the plane,
    // sample the tile attribute, switch on the logic class (CDDraw tag - 0x34), dispatch
    // the matching switch/trigger logic object; on a miss, Format an error CString
    // ("No switch/trigger logic found for switch ...") and ReportError. (lives in the
    // eh sibling TU.) THREE args - retail ends `ret 0xc`, and every caller (GruntSteps/
    // Grunt.cpp/GruntCombat, ex `ApplyTileSwitch`/`ApplySwitch(this,x,y)` view aliases)
    // pushes (grunt, x, y). The 2-arg spelling was a shape defect (ret 8).
    i32 ApplySwitch(CGrunt* g, i32 sx, i32 sy);

    // 0x6dae0 / 0x6e120: the two big tile-trigger appliers (apply-on-enter / apply-on-
    // exit). Look up the cell, walk its trigger/switch sub-objects, dispatch each logic
    // and update the cell state. (__stdcall: ret 0x1c / 0x1c.) Reconstructed to plateau.
    i32 ApplyTriggerA(i32 col, i32 row, i32 a24, i32 a28);
    i32 ApplyTriggerB(i32 col, i32 row, i32 a28, i32 a2c);

    // 0x6e800: ClearCell(col, row, ...) - reset a cell's animation/trigger sub-state and
    // string-compare its config name, then re-init. (__stdcall: ret 0x14.)
    i32 ClearCell(i32 col, i32 row, i32 a18, i32 a1c, i32 a20);

    // 0x6ea00: HitTestApply(x, y, kind) - hit-test then, when the magic kind, compare the
    // config string and adjust the world score + status item. (__stdcall: ret 0xc.)
    void HitTestApply(i32 x, i32 y, i32 kind);

    // 0x75af0: HitTestCell(x, y, outRow, outCol, exact) - sample the tile-attr index, map
    // it to (row,col), bounds-test the cell object, write (row,col). (ret 0x14.)
    i32 HitTestCell(i32 x, i32 y, i32* outRow, i32* outCol, i32 exact);

    // 0x75c60: FindGruntAt(px, py, span, outCol, outRow, src) - scan the tile cells
    // around a pixel point (bounded by the tile-span rect, or an explicit source rect)
    // through the tile grid's packed owner word into m_grid; return the first live cell
    // (m_1fc) whose 15x15 display box hits the rect, reporting its (col,row). (ret 0x18.)
    CGrunt* FindGruntAt(i32 px, i32 py, RECT* span, i32* outCol, i32* outRow, RECT* src);

    // 0x78520 / 0x78680: the two record-table reporters - scan the record list (+0x244)
    // for nodes of the magic group, collect their bytes, then call a world report helper
    // with one of two messages depending on the count. (__stdcall: ret 0xc / ret 0x10.)
    void ReportRecordsA(i32 tag, i32 gx, i32 gy);
    void ReportRecordsB(i32 tag, i32 gx, i32 gy, i32 flag);

    // 0x78a50: PlaceObjectFull - the largest tile-object placer/switch driver (0x845 B,
    // a dense jump table over the logic kind, with two coordinate sub-tables). Builds the
    // object, dispatches by kind, stashes the cell. Reconstructed to plateau. (ret 0x18.)
    i32 PlaceObjectFull(i32 x, i32 y);

    // 0x79520: ResetGroup - drain the magic-group cells, clearing each cell's sub-state and
    // recycling its record node, then refresh. Reconstructed to plateau. (__thiscall.)
    i32 ResetGroup(i32 a14, i32 a18, i32 a1c, i32 a20, i32 a24, i32 a28, i32 a2c);

    // 0x798d0: DestroyGroup - /GX destruct of a cell group (member CString temporaries on
    // teardown). Reconstructed to plateau (eh sibling TU).
    i32 DestroyGroup(i32 a1, i32 a2, i32 a3, i32 a4);

    // 0x79b80: ReinitGroup - /GX re-init driver with a CString config-name temporary (eh
    // sibling TU). Reconstructed to plateau.
    i32 ReinitGroup(i32 col, i32 row);

    // 0x79fb0: NotifyCell(row, col, z) - the notify-cell hook CellDispatch tails into.
    // Recall the cell's sub-objects, clear its tile-attr bits, decrement the per-row count,
    // and re-arm. (__stdcall: ret 0xc.) (was a stub; now reconstructed.)

    // 0x7a5e0: serialize the trigger grid's timer blocks and overlay records.
    // kind 4 writes, kind 7 reads. (__thiscall: ret 0x10.)
    i32 Serialize(CFileMemBase* ar, i32 kind, i32 unusedC, i32 unusedD);

    // 0x7a760: ScanGroup - the magic-group scanner/applier; for each live cell of the
    // group, dispatch its logic and tally. Reconstructed to plateau. (__thiscall.)
    i32 ScanGroup(CFileMemBase* ar);

    // 0x7b1b0: TriggerCell(x, y) - look up the (x,y) cell, switch on its logic kind and
    // spawn the matching fx sprite (+0x2a8), then refresh + record. (ret 0x8.)
    i32 TriggerCell(i32 x, i32 y);

    // 0x7c110: SpawnGrunt(col, row, a18) - create a "Grunt" sprite at the cell's snapped
    // world pos, Init it, place it via its userlogic; on success stash the cell. ret 1.
    // (__stdcall: ret 0x10.)
    i32 SpawnGrunt(i32 col, i32 row, i32 a18, i32 a1c);

    // 0x79d90: ResetSpawnState - when the active game state is live (gameReg->m_134==1)
    // and this->m_284 is set, free the world status item's pending buffer (+0x54c),
    // clear +0x548, drop the last entry of the +0x260 array, optionally re-fire the
    // build-state notifier, re-pulse the pending-fx (+0x2a0), and LoadFinishLevelSprite(6). (__thiscall.)
    void ResetSpawnState();

    // 0x7c2e0: CycleMoveIcons(skipRow, enable) - for rows 0..3 except `skipRow`, either
    // (enable) roll a random move-icon onto each live cell and OnRegion4, or (else) restore
    // each cell's stashed icon (+0x1f8). ret 1. (__stdcall: ret 8.)
    i32 CycleMoveIcons(i32 skipRow, i32 enable);

    // 0x7cc60: RebuildSelectionList(idx) - recycle selection list `idx` (+0x2d4) back to the
    // free list, RemoveAll it, then copy the record list (+0x244) into it as fresh nodes;
    // reset +0x3e8. ret 1. (__thiscall: ret 4.)
    i32 RebuildSelectionList(i32 idx);

    // 0x7cd40: CenterSelectionGroup(slot) - ResetAll + overlay tick, then walk the slot's
    // selection list (+0x2d4[slot*0x1c]), ResetCell each live cell and recycle dead nodes.
    // On the second pass for the same slot (m_3e8==slot) accumulate the cell bbox and scroll-
    // center the world on it (then m_3e8=-1); else latch m_3e8=slot. ret 1 (0 when the slot
    // list is empty). (__thiscall: ret 4.)
    i32 CenterSelectionGroup(i32 slot);

    // 0x7d450 / 0x7d5c0: the two region-toggle handlers. If a pending fx (+0x2a8) is live,
    // clear it and LoadCursorSprites(0,0); else look up the active record cell and, by its
    // logic kind (+0x170/+0x19c/+0x198), either ResetGroup, set a pending fx, or just tick
    // the overlay. ret 1.
    i32 ToggleRegionA();
    i32 ToggleRegionB();

    // 0x7d6e0: EnqueueGroupCells - collect the y-bytes of every magic-group record cell with
    // a clear +0x1e4 flag, then post the group to the command mgr (+0x6c): EnqueueSingle when
    // exactly one, else EnqueueMulti. ret 1 (0 when +0x400 is clear). (__thiscall.)
    i32 EnqueueGroupCells();

    // 0x78060: HudRect (TriggerMgr.cpp) - the combat-region scan CPlay's
    // DispatchHudClick/PostHudRect drive on the world's +0x68 slot (== this mgr;
    // transform the world rect via m_level's view, then (re)arm combat state on
    // every grunt slot whose 30x30 box hits it.
    void HudRect(RECT r, i32 flag);

    // 0x6eb80: the per-frame goo-well / win-condition grid step (thiscall(clock)).
    // Body in GooWellMgr.cpp. (The ex-`CGooWellMgr` view there was another fake name
    // for THIS class - every field overlaid: m_playerFlag[4]@+0x10c==m_rowCount,
    // m_overlay@+0x25c, m_phase@+0x288, the +0x290..+0x2c8 i64 timer pairs, the
    // +0x3f0..+0x3fc loop channels. Folded 2026-07-14.)
    i32 LoadTeleporterGooConfig(i32 clock);

    // 0x7c3d0: the finish-level / round-end state driver (TriggerMgr.cpp): a 6-way
    // switch on `state` that latches m_phase + the countdown pair, plays the
    // GAME\FINISHLEVEL cue on entering state 1, and resolves the pending-fx grunt's
    // death anim on state 3. The goo-well update fires it as its "Notify" (states
    // 2..5) on this same object - the ex-`CFinishLevelState` and ex-`CGooWellMgr`
    // views were both THIS class (offsets 0x22c/0x288/0x290/0x2a0/0x3ec/0x400).
    void LoadFinishLevelSprite(i32 state);

    // 0x7be60: the resurrect-radius pass (called by CGrunt::LoadGruntAbilityTuning
    // @0x57100 via ILT 0x1fff): walk the baseList candidates inside the (cx,cy,r)
    // tile rect and PlaceObject/board-place each un-occupied one, flagging its bound
    // object + RemoveAt'ing the node. Ex-`CGruntResurrector` view: its `Resurrect`
    // was ?PlaceObject@CTriggerMgr@@ @0x6b6d0 (ILT 0x40bb) and its `Notify` the MFC
    // ?RemoveAt@CPtrList@@ @0x1b4ac7 on the +0 baseList - this class, proven twice.
    i32 LoadGruntResurrectTuning(i32 cx, i32 cy, i32 r);

    // 0x77df0: FindNearestEnemy(g) - scan all live, non-kind-0x36 grid cells
    // SKIPPING g's own row (m_tileOwnerHi), pick the nearest to g's tile; null it
    // unless inside g's +/-(m_reachRadius+m_defenderRadius+1) tile box. Body in
    // BrickzCellFlags_077790.cpp (ex `Grid_77df0::FindNearest` - the receiver at
    // every call site is the grunt's +0x260 board == THIS class; the identity the
    // old @identity-TODO said "does not crack" cracks from that settled slot type).
    CGrunt* FindNearestEnemy(CGrunt* g);

    // 0x7cf40: centre the view on the selection group's bounding-box midpoint, then
    // (single selection) re-arm the record latch. Ex-`CGroupSel` view (TriggerMgr.cpp
    // + GameKeyHandler.cpp): its +0x1c grid / +0x230..+0x238 latch / +0x244 list head
    // / +0x24c count are m_grid / m_armed-m_recX-m_recY / m_recList's head+count, and
    // its `TrySelect`/`Commit` were RecordListHas @0x784d0 / LoadCameraSprite @0x78960
    // (ILT 0x33aa / 0x3d1e) - both already reconstructed on THIS class.
    i32 CenterOnGroup(i32 doSelect);

    // CGrunt::m_tileMgr (+0x260) IS this class - every dispatch site loads
    // ecx=[grunt+0x260] and its thunk lands in this class's method band; the view's
    // ClaimTile/ReleaseTile/LookupTile/SetTileState4/ArrivalNotify6/... were alias
    // names for ResetCell/RecordListHas/HitTestCell/CellDispatch/LoadTileArrivalFx.
    // The four rows below are the targets the view carried that had NO decl here yet
    // (each verified by resolving the caller's ILT thunk to its body):
    // 0x75e90 (thunk 0x3945; 6 args, ret 0x18): the tile-arrival effect/switch
    // driver (LEVEL_DIRT/GAME_DIRT/LEVEL_GAUNTLETROCK1 tile fx by reason). The
    // grunt anim-dispatch machines fire it on settled arrival (ex ArrivalNotify6 ==
    // ex Load6, one body). Body in TerrainTileLoader.cpp (the ex-`CTerrainTileLoader`
    // placeholder class was a view of THIS one: its +0x22c level holder IS m_level).
    i32 LoadTileArrivalFx(i32 ownerHi, i32 ownerLo, i32 tileX, i32 tileY, i32 reason, i32 sel);
    // (SpawnTileFx @0x79ea0 is a free __stdcall function, not a CTriggerMgr method -
    //  declared at namespace scope below the class.)
    // 0x7b930: the 5-arg combat-area cue (radius scan over m_grid applying the
    // per-cell tier effect); body in TriggerMgr.cpp (ex ?CombatCue@CGruntTileMgr@@).
    i32 CombatCue(i32 x, i32 y, i32 radius, i32 tier, i32 flag); // 0x7b930 (ret 0x14)
    // 0x7b440: the rock-break particle spawner (radius tile scan; body in
    // TriggerMgr.cpp). Ex ?BuildRockBreakParticles@CRockBreakMgr@@ - that class was
    // a placeholder view of THIS one: its only member m_22c (the world holder) IS
    // m_level, its Prepare thunk 0x400c IS CombatCue @0x7b930, it was "reached
    // through the registry's +0x68 slot" (m_cmdGrid == this class) and the body
    i32 BuildRockBreakParticles(i32 cx, i32 cy, i32 r, i32 a4); // 0x7b440
    // 0x6e7e0: the HUD/pixel grunt probe (5-byte always-0 stub); body in
    // TriggerMgrGrid.cpp (ex ?FindAtPixel@CGruntTileMgr@@).
    CGrunt* FindAtPixel(i32 x, i32 y); // 0x6e7e0

    // 0x6c130: the settled-move tile-switch/plate commit (__thiscall ret 0xc; body in
    // TriggerMgrGrid.cpp - by first-link contiguity it sits between ResetCell and
    // ApplySwitch inside this class's own obj). Ex the `CTileWireLogic` .cpp-local view,
    // (CGrunt::m_tileMgr == THIS class), and the view's only own member, m_level@+0x22c,
    // IS m_level (identical level->plane clamp walk as the sibling ApplySwitch). The
    // "m_triggerContainer @+0x2e4" the view carried was NOT on this class at all: retail
    // reads it off the spilled g_gameReg->m_curState (CPlay::m_beginMarker, the
    // CTileTriggerContainer) - the +0x2e4 coincidence with m_selLists[0]'s CPlex slot
    // was a mis-based read in the ~10% reconstruction.
    i32 WireTileSwitchLogic(CGrunt* g, i32 x, i32 y); // 0x6c130

    // 0x85c50: ~CTriggerMgr - the /GX destructor (drains the lists, destructs the member
    // list arrays). Reconstructed to plateau (eh sibling TU).
    ~CTriggerMgr();

    // --- self-called helpers the reconstructed leaves dispatch on `this` -----------------
    // Still-UNRECONSTRUCTED CTriggerMgr methods (no body / RVA here). Reset3/RefreshB
    // cover several retail RVAs of the same shape (all masked).
    i32 Reset3(i32 a, i32 b, i32 c);
    // (ReportObjectAt is GONE - its thunk 0x3030 jumps to 0x6e120, which IS
    // ApplyTriggerB; CGrunt::StepPeerTracking calls the real name.)
    CGrunt* Hit(i32 arg, i32 a, i32 b, i32* outRow, i32* outCol);
    void ReportN(i32 a, i32 b, u8* bytes, i32 c, i32 d, i32 e, i32 f);
    CGrunt* Hit5(i32 a, i32 b, i32 c, i32 d, i32 e);
    i32 PlaceB(i32 a, i32 b, i32 c);
    void Fx(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f);

    // 0x7c620: LoadPowerupIconSprites(type, geoB, geoA, m130, warpIdx, m120) - the big
    // in-game-icon loader (PickupType jump-table switch selecting the GAME_INGAMEICONZ_*
    // sprite-set key). Called on the +0x68 registry m_cmdGrid by the tile effect loaders
    // (TileTriggerSwitchLogic, GruntCombat) and the grunt ProbeMoveTile thunk 0x152d.
    // Body in TriggerMgr.cpp (the ex-`FireCommand` decl here and the ex-
    // ?LoadPowerupIconSprites@EngineLabelBacklog def were TWO NAMES for this one method;
    // unified 2026-07-16).
    i32 LoadPowerupIconSprites(i32 type, i32 geoB, i32 geoA, i32 m130, i32 warpIdx, i32 m120);

    // 0x7b330: the brick-break explosion-sprite loader (x, y, id, kind), the sibling of
    // LoadPowerupIconSprites fired on this same +0x68 registry m_cmdGrid by
    // CTileActionEvent::Process (effect 0x144) and the K/x cheat keys. Body in
    // TriggerMgr.cpp (was ?LoadExplosionSprites@EngineLabelBacklog - the fake host's
    // m_factoryHolder @+0x22c IS m_level).
    i32 LoadExplosionSprites(i32 x, i32 y, i32 id, i32 kind);

    // 0x7a3f0: the lazy "GAME_TOYBOX" in-game-icon loader (bails when an icon already
    // sits on the tile). Body in TriggerMgr.cpp (ex ?LoadToyBoxIcon@EngineLabelBacklog).
    i32 LoadToyBoxIcon(i32 x, i32 y, i32 a3, i32 a4, i32 a5);

    // 0x46b6d0: the screen-coord -> cell-index probe the battlez spawn machine fires on
    // this grid (two arg shapes at the same body; Ghidra leaves it class-unattributed).
    // Declared-only (reloc-masked). These two are all that BattlezMapConfig.cpp's
    // DUPLICATE `class CTriggerMgr` added - a second, divergent definition of THIS class
    // inside a .cpp (its "m_objListHead @+0x04" is m_baseList's list head, its
    i32 Probe(
        i32 cell,
        i32 sx,
        i32 sy,
        i32 a3,
        i32 a4,
        i32 a5,
        i32 a6,
        i32 a7,
        i32 a8,
        i32 a9,
        i32 a10,
        i32 a11,
        i32 a12
    );
    i32 ProbeCell(i32 a0, i32 a1, void* a2, i32 a3, void* a4, i32 a5, i32 a6, i32 a7, i32 a8);

    // --- data layout (recovered from the raw this+offset field reads across both TUs) ---
    // The three embedded MFC containers (base CPtrList @0, record CPtrList @0x240, byte-table
    // CByteArray @0x260) and the ten-slot selection-list array (@0x2d0, stride 0x1c) are the
    // REAL MFC classes - the leaves call their methods directly and ~CTriggerMgr auto-emits
    // their member teardown: 2 scalar ??1CObList CALLs (m_baseList/m_recList) + the
    // m_selLists[10] array teardown, whose __ehvec_dtor takes ??1CObList as a function
    // POINTER (that is ~CTriggerMgr's DATA reloc), + 1 ??1CByteArray CALL. No casts remain.
    CPtrList m_baseList;  // +0x000  base object-list (holds CTmRecNode payloads)
    CGrunt* m_grid[0x3c]; // +0x01c  the 4x15 placed grid-object cells (stride 4)
    i32 m_rowCount[4];    // +0x10c  per-row placed count (bumped/serialized 0x10 B)
    i32 m_cellFlag[0x3c]; // +0x11c  parallel 4x15 per-cell flag grid; also holds the
                          //         cached origin pair at +0x58/+0x5c (GetOriginXY, raw)
    i32 m_rowStateB[4];   // +0x20c  per-row state band B
    i32 m_rowStateC[4];   // +0x21c  per-row state band C
    // +0x22c  the active world/resource holder (SetLevel). Renamed m_level->m_world:
    // it is the CDDrawSurfaceMgr WORLD HOLDER, not the board - consumers reach the real
    // CGameLevel via m_world->m_level (and the child-group factory via m_world->m_childGroup).
    CDDrawSurfaceMgr* m_world;
    // +0x230: the multiplayer armed gate (ex-CMultiSub68 view's m_armed) ==
    // the companion state word cleared by SetLevel; serialized at 0x1339/0x1545.
    i32 m_armed;                      // +0x230
    i32 m_recX;                       // +0x234  active-record x
    i32 m_recY;                       // +0x238  active-record y
    CWwdGameObjectA* m_goal;          // +0x23c  the "DoNothing" camera-sprite goal object
                                      //         (LoadCameraSprite; released via m_flags|=0x10000)
    CPtrList m_recList;               // +0x240  record list (per-cell undo/record nodes)
    CActionOptionsMenuBar* m_overlay; // +0x25c  the allocated overlay sub-object (0x40 B)
    CByteArray m_byteArr;             // +0x260  byte-table array
    char m_274[0x10];                 // +0x274  serialized 16-byte region
    i32 m_284;                        // +0x284  build/reinit gate flag
    // +0x288: the round phase (0 = running, 1 = finishing, 2 = finished). PROVEN by the
    // two state drivers on this class: LoadTeleporterGooConfig (0x6eb80, the goo-well
    // update) and LoadFinishLevelSprite (0x7c3d0, the finish-level 6-state switch).
    i32 m_phase;       // +0x288  round phase (serialized)
    char _pad28c[0x4]; // +0x28c
    // +0x290..+0x2c8: the three 64-bit timer pairs (base tick, window). PROVEN i64 by
    // 0x6eb80's 64-bit subtract/compare chains ((i64)g_frameTime - base >= window). The
    // finish-level driver writes them as (lo, hi=0) pairs - spell those stores as u32
    // zero-extends. Serialize (0x7a5e0) snapshots the same three pairs in 8-byte
    // strides through the overlay source's GetA/GetB getters. The first pair is
    // multi-role (same storage, mode-dependent client): the battlez match-over
    // countdown (0x6eb80), the finish-level cue timer (0x7c3d0, base=g_frameTime,
    // window=cueDuration+500 or 3000), and the warlord fort-battle cue window
    // (CWarlord::AdvanceMovingAnim, window=0x3e8).
    i64 m_timerBase;   // +0x290  timer pair 0: base tick
    i64 m_timerWindow; // +0x298  timer pair 0: window/length
    // +0x2a0: the pending-fx GRUNT (the spawned fx sprite's bound logic). Ex-CTmPendingFx
    // view; its `Pulse()` was ?ResolveDeathAnimation@CGrunt@@QAEHXZ @0x455f0 all along
    // (ILT 0x3a1c at both call sites), and the deserializer stores m_7c->m_logic here.
    CGrunt* m_pendingFx;     // +0x2a0  pending-fx grunt logic
    i32 m_countdownActive;   // +0x2a4  countdown armed gate (serialized; ex m_2a4)
    i32 m_pendingFxKind;     // +0x2a8  active pending overlay-fx kind
    char _pad2ac[0x4];       // +0x2ac
    i64 m_gooTimerBase;      // +0x2b0  goo respawn timer base ("TimePerGoo")
    i64 m_gooInterval;       // +0x2b8  goo respawn interval
    i64 m_resourceTimerBase; // +0x2c0  resource respawn timer base ("TimePerResource")
    i64 m_resourceInterval;  // +0x2c8  resource respawn interval
    CPtrList m_selLists[10]; // +0x2d0  ten selection lists (stride 0x1c)
    i32 m_selSentinel;       // +0x3e8  selection-group latch (-1 when idle)
    i32 m_3ec;               // +0x3ec  serialized scalar (LoadFinishLevelSprite: last state)
    // +0x3f0..+0x3fc: the two looping-sound channels + their per-frame wanted flags.
    // Names PROVEN by the goo-well update's lookup keys: "LEVEL_ROLLINGBALL" plays
    // into +0x3f0 gated by +0x3f8, "GAME_TELEPORTLOOP" into +0x3f4 gated by +0x3fc.
    DirectSoundMgr* m_rollingballLoop; // +0x3f0  LEVEL_ROLLINGBALL loop handle
    DirectSoundMgr* m_teleportLoop;    // +0x3f4  GAME_TELEPORTLOOP loop handle
    i32 m_rollingballWanted;           // +0x3f8  rollingball wanted this frame
    i32 m_teleportWanted;              // +0x3fc  teleportloop wanted this frame
    i32 m_groupFlag;                   // +0x400  magic-group active flag
};
SIZE_UNKNOWN();

// 0x6da60 / 0x6daa0 (thunks 0x275c / 0x2c48): free __stdcall command
// helpers. Each forwards (1, hi, lo, N, 0,0,0,0) to CGruntzCmdMgr and never
// receives or reads a CTriggerMgr receiver.
void __stdcall GridAction6(i32 hi, i32 lo);
void __stdcall GridAction7(i32 hi, i32 lo);

i32 __stdcall SpawnTileFx(i32 px, i32 py, i32 kind);

extern "C" void IconClassInitB(); // 0x402bad
extern "C" void IconClassInitA(); // 0x40288d

// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---
extern void __stdcall Eng_SpawnFx(i32 type, i32 x, i32 y, i32 a3, i32 a4, i32 a5); // 0x7c620

extern i32 g_groupSentinel;
#endif
