// TriggerMgr.h - CTriggerMgr, the playfield tile-object / switch-trigger grid
// manager (trace placeholder tomalla-23, C:\Proj\Gruntz). NON-POLYMORPHIC
// (no RTTI, no vtable): a plain __thiscall manager that owns a 15-column grid of
// placed game-object pointers (base +0x1c, element stride 4), a per-cell undo /
// record list (+0x244), a 10-entry array of selection lists (+0x2d0, stride 0x1c),
// an MFC pointer list (+0x240), and a small allocated overlay sub-object (+0x25c,
// 0x40 bytes). It reads the global game registry (g_gameReg @0x64556c) for the
// active level/plane and drives the per-tile switch/trigger logic.
//
// The offsets the reconstructed methods touch are named data members below (recovered
// from how every method reads/writes them); the gaps are opaque padding. The grid cells
// and the level/plane objects are still full UNMATCHED engine classes, so member fields
// typed as void*/char are cast to the file-local opaque shells at their use sites (that
// keeps those reads / calls reloc-masked). The three embedded MFC containers (base CObList
// @0, record CObList @0x240, byte-table CByteArray @0x260) still have their base address
// reinterpreted through the file-local list/array helpers for the reloc-masked engine
// method bodies - their head/count scalars ARE broken out as named members.
#ifndef SRC_GRUNTZ_TRIGGERMGR_H
#define SRC_GRUNTZ_TRIGGERMGR_H
#include <rva.h>
#include <Mfc.h> // CPtrList and the MFC list helpers (reloc-masked)

// The global free-list of recycled list nodes (an intrusive singly-linked stack)
// and its node-bias constant, shared across the manager's list churn. Stored as
// raw void*/int (the engine's real ?g_freeList@@3PAXA / ?g_freeListNodeBias@@3HA).
extern void* g_freeList;       // ?g_freeList@@3PAXA        @0x645544
extern i32 g_freeListNodeBias; // ?g_freeListNodeBias@@3HA  @0x64554c

// operator delete (static CRT, reloc-masked); operator new comes from <Mfc.h>.
void operator delete(void*);

// The (x,y) pair the +0x174/+0x178 origin accessor writes out.
struct CTrigPoint {
    i32 x; // +0x00
    i32 y; // +0x04
};
SIZE_UNKNOWN(CTrigPoint);

// The archive reader the Load serializer pulls fields through (vtable slot 0x2c =
// Read(dst, size)); defined fully in the eh sibling TU.
struct CTmSerReader;

// The placed grid-cell game object (a CGrunt). The full unified shape is a file-local
// view in each TU; the grid stores pointers to it, so an incomplete decl suffices here.
// Same for the level object, sound-channel helper and the two intrusive list-node shapes
// the record/base lists hang off - each TU completes them as it needs.
struct CTmCell;
struct CTmLevel;
struct CTmSoundChan;
struct CTmNode;
struct CTmRecNode;
struct CTmArchive; // the serialize archive ScanGroup writes through (eh sibling completes it)

class CTriggerMgr {
public:
    // 0x7abc0: Load(ar) - deserialize the whole trigger-mgr state from the reader
    // (the 4x15 placed-object grid, per-row state bands, byte table, record list,
    // ten selection lists, base object list, overlay sub-object + tail scalars).
    // /GX; lives in the eh sibling TU. ret 1, or 0 on any missing referent.
    i32 Load(CTmSerReader* ar);

    // --- the small reconstructed leaf interface (retail-RVA order) -------------
    // 0x759e0: copy the cached origin pair (+0x174,+0x178) into the caller's
    // out-slot and return it (ret 4 -> callee cleans the out-ptr arg).
    CTrigPoint* GetOriginXY(CTrigPoint* out);

    // 0x6b640: store the supplied object at +0x22c and clear three companion
    // state words; returns 1 (0 when arg is null).
    i32 SetLevel(CTmLevel* lvl);

    // 0x78a30: forward to the overlay sub-object's helper when present, else ret.
    void OverlayTick();

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

    // 0x7d2a0: only when `key` equals the magic kind (g_644c54), scan the 10 selection
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

    // 0x77f80: FindNearestInRow(g) - scan the grid row g->m_1ec (15 cells) for the live
    // cell whose display object is nearest g's tile pos (g->m_17c/m_180 >> 5), within the
    // squared cutoff 2*g->m_2dc; ret the nearest cell pointer or 0. (__thiscall: ret 4.)
    CTmCell* FindNearestInRow(CTmCell* g);

    // 0x78260: RemoveCellRecord(x,y,fromSelection) - unlink the (x,y) node from the
    // selection lists (when fromSelection) and from the record list, clearing the cell's
    // sprites / goal / overlay along the way. ret 1 if a record was removed. (ret 0xc.)
    i32 RemoveCellRecord(i32 x, i32 y, i32 fromSelection);

    // 0x7a180: SpawnPuddle(x,y,...) - create+init a "GruntPuddle" sprite, stash its
    // placement fields, then PlacePuddle it. ret 0 on factory failure. (ret 0x18.)
    i32 SpawnPuddle(i32 x, i32 y, i32 f124, i32 f114, i32 color, i32 f118);

    // 0x7a240: PlacePuddle(sprite, color) - place the puddle via its CUserLogic and, on
    // success, flag/remove the matching record + selection nodes. ret 1 on success.
    i32 PlacePuddle(CTmCell* sprite, i32 color);

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

    // 0x6d300: ApplySwitch - the /GX switch-logic driver. Clamp (sx,sy) to the plane,
    // sample the tile attribute, switch on the logic class (CDDraw tag - 0x34), dispatch
    // the matching switch/trigger logic object; on a miss, Format an error CString
    // ("No switch/trigger logic found for switch ...") and ReportError. (lives in the
    // eh sibling TU; __stdcall ret 0xc.)
    i32 ApplySwitch(i32 sx, i32 sy);

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

    // 0x78520 / 0x78680: the two record-table reporters - scan the record list (+0x244)
    // for nodes of the magic group, collect their bytes, then call a world report helper
    // with one of two messages depending on the count. (__stdcall: ret 0xc / ret 0x10.)
    void ReportRecordsA(i32 a14, i32 a18, i32 a1c, i32 a20, i32 a24);
    void ReportRecordsB(i32 a14, i32 a18, i32 a1c, i32 a20, i32 a24, i32 a28);

    // 0x78a50: PlaceObjectFull - the largest tile-object placer/switch driver (0x845 B,
    // a dense jump table over the logic kind, with two coordinate sub-tables). Builds the
    // object, dispatches by kind, stashes the cell. Reconstructed to plateau. (ret 0x18.)
    i32 PlaceObjectFull(i32 x, i32 y);

    // 0x79520: ResetGroup - drain the magic-group cells, clearing each cell's sub-state and
    // recycling its record node, then refresh. Reconstructed to plateau. (__thiscall.)
    i32 ResetGroup(i32 a14, i32 a18, i32 a1c, i32 a20, i32 a24, i32 a28, i32 a2c);

    // 0x798d0: DestroyGroup - /GX destruct of a cell group (member CString temporaries on
    // teardown). Reconstructed to plateau (eh sibling TU).
    i32 DestroyGroup(i32 col, i32 row, i32 force);

    // 0x79b80: ReinitGroup - /GX re-init driver with a CString config-name temporary (eh
    // sibling TU). Reconstructed to plateau.
    i32 ReinitGroup(i32 col, i32 row);

    // 0x79fb0: NotifyCell(row, col, z) - the notify-cell hook CellDispatch tails into.
    // Recall the cell's sub-objects, clear its tile-attr bits, decrement the per-row count,
    // and re-arm. (__stdcall: ret 0xc.) (was a stub; now reconstructed.)

    // 0x7a5e0: RebuildOverlay(kind) - for the overlay sub-object (+0x290..+0x2c0 in 0x8
    // strides), copy two descriptor fields via its vtable getter/setter per kind. ret 1.
    // (__thiscall: ret 0x10.)
    i32 RebuildOverlay(void* obj, i32 kind, i32 unusedC, i32 unusedD);

    // 0x7a760: ScanGroup - the magic-group scanner/applier; for each live cell of the
    // group, dispatch its logic and tally. Reconstructed to plateau. (__thiscall.)
    i32 ScanGroup(CTmArchive* ar);

    // 0x7df8: the overlay-serialize self-call ScanGroup tails into (writes the overlay into
    // the archive). Still-UNRECONSTRUCTED; declared so the reloc-masked self-call is clean.
    i32 SerializeOverlay(CTmArchive* ar, i32 b, i32 c);

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
    // build-state notifier, re-pulse the pending-fx (+0x2a0), and RefreshB(6). (__thiscall.)
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

    // 0x85c50: ~CTriggerMgr - the /GX destructor (drains the lists, destructs the member
    // list arrays). Reconstructed to plateau (eh sibling TU).
    ~CTriggerMgr();

    // --- self-called helpers the reconstructed leaves dispatch on `this` -----------------
    // Still-UNRECONSTRUCTED CTriggerMgr methods (no body / RVA here); declared so the leaves'
    // reloc-masked self-calls mangle onto this class with no `((CTmSelf*)this)` cast. Reset3/
    // RefreshB cover several retail RVAs of the same shape (all masked).
    i32 Reset3(i32 a, i32 b, i32 c);
    void RecallCell(i32 x, i32 y, CTmCell* cell);
    void RefreshB(i32 a);
    void RefreshC();
    i32 Probe4(void* obj);
    i32 Probe7(void* obj);
    CTmCell* Hit(i32 arg, i32 a, i32 b, i32* outRow, i32* outCol);
    void ClearMagic(i32 key);
    i32 Classify(i32 x, i32 y);
    void Refresh2();
    void Record2(i32 x, i32 y);
    void ReportN(i32 a, i32 b, u8* bytes, i32 c, i32 d, i32 e, i32 f);
    CTmCell* Hit5(i32 a, i32 b, i32 c, i32 d, i32 e);
    i32 PlaceA(i32 a, i32 b, i32 c, i32 d);
    i32 PlaceB(i32 a, i32 b, i32 c);
    void Fx(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f);

    // --- data layout (recovered from the raw this+offset field reads across both TUs) ---
    // The three embedded MFC containers (base CObList @0, record CObList @0x240, byte-table
    // CByteArray @0x260) and the ten-slot selection-list array (@0x2d0, stride 0x1c) are
    // modeled as raw byte regions (with their head/count scalars broken out); the reconstructed
    // methods still reinterpret those regions through the file-local CTmPtrList / CTmObArray
    // helpers to drive the reloc-masked engine list bodies. Everything is a plain this+offset
    // field, so naming is byte-neutral.
    char _pad0[0x4];            // +0x000  base list vptr
    CTmRecNode* m_objListHead;  // +0x004  base object-list head node
    char _pad8[0x4];            // +0x008  base list tail
    i32 m_objListCount;         // +0x00c  base object-list count
    char _pad10[0xc];           // +0x010  base list free/blocks/blocksize
    CTmCell* m_grid[0x3c];      // +0x01c  the 4x15 placed grid-object cells (stride 4)
    i32 m_rowCount[4];          // +0x10c  per-row placed count (bumped/serialized 0x10 B)
    i32 m_cellFlag[0x3c];       // +0x11c  parallel 4x15 per-cell flag grid; also holds the
                                //         cached origin pair at +0x58/+0x5c (GetOriginXY, raw)
    i32 m_rowStateB[4];         // +0x20c  per-row state band B
    i32 m_rowStateC[4];         // +0x21c  per-row state band C
    CTmLevel* m_level;          // +0x22c  the active level object (SetLevel)
    i32 m_230;                  // +0x230  companion state word (cleared by SetLevel)
    i32 m_recX;                 // +0x234  active-record x
    i32 m_recY;                 // +0x238  active-record y
    void* m_goal;               // +0x23c  the goal object (CTmGoal)
    char _pad240[0x4];          // +0x240  record list vptr
    CTmNode* m_recHead;         // +0x244  record-list head node
    char _pad248[0x4];          // +0x248  record list tail
    i32 m_recCount;             // +0x24c  record-list count
    char _pad250[0xc];          // +0x250  record list free/blocks/blocksize
    void* m_overlay;            // +0x25c  the allocated overlay sub-object (0x40 B)
    char _pad260[0x4];          // +0x260  byte-table array vptr
    u8* m_byteData;             // +0x264  byte-table data pointer
    i32 m_byteCount;            // +0x268  byte-table count
    char _pad26c[0x8];          // +0x26c  byte-table maxsize/growby
    char m_274[0x10];           // +0x274  serialized 16-byte region
    i32 m_284;                  // +0x284  build/reinit gate flag
    i32 m_288;                  // +0x288  serialized scalar
    char _pad28c[0x4];          // +0x28c
    char m_overlayDescA[0x10];  // +0x290  overlay descriptor block 0
    void* m_pendingFx;          // +0x2a0  pending-fx sub-object (CTmPendingFx)
    i32 m_2a4;                  // +0x2a4  serialized scalar
    i32 m_pendingFxKind;        // +0x2a8  active pending overlay-fx kind
    char _pad2ac[0x4];          // +0x2ac
    char m_overlayDescB[0x10];  // +0x2b0  overlay descriptor block 1
    char m_overlayDescC[0x10];  // +0x2c0  overlay descriptor block 2
    char m_selLists[0x118];     // +0x2d0  ten selection lists (CObList, stride 0x1c)
    i32 m_selSentinel;          // +0x3e8  selection-group latch (-1 when idle)
    i32 m_3ec;                  // +0x3ec  serialized scalar
    CTmSoundChan* m_soundChanA; // +0x3f0  DirectSound channel A
    CTmSoundChan* m_soundChanB; // +0x3f4  DirectSound channel B
    i32 m_3f8;                  // +0x3f8
    i32 m_3fc;                  // +0x3fc
    i32 m_groupFlag;            // +0x400  magic-group active flag
};
SIZE_UNKNOWN(CTriggerMgr);

#endif
