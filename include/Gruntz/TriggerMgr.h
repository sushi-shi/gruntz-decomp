// TriggerMgr.h - CTriggerMgr, the playfield tile-object / switch-trigger grid
// manager (trace placeholder ClassUnknown_23, C:\Proj\Gruntz). NON-POLYMORPHIC
// (no RTTI, no vtable): a plain __thiscall manager that owns a 15-column grid of
// placed game-object pointers (base +0x1c, element stride 4), a per-cell undo /
// record list (+0x244), a 10-entry array of selection lists (+0x2d0, stride 0x1c),
// an MFC pointer list (+0x240), and a small allocated overlay sub-object (+0x25c,
// 0x40 bytes). It reads the global game registry (g_gameReg @0x64556c) for the
// active level/plane and drives the per-tile switch/trigger logic.
//
// Only the offsets the reconstructed methods touch are pinned; the rest is opaque
// padding. Members are accessed by raw this+offset (a deliberate, naming-independent
// choice for this externally-coupled manager - the grid cells and the level/plane
// objects are full UNMATCHED engine classes, modeled as opaque shells so their
// member reads / calls reloc-mask).
#ifndef SRC_GRUNTZ_TRIGGERMGR_H
#define SRC_GRUNTZ_TRIGGERMGR_H
#include <rva.h>
#include <Mfc.h> // CPtrList and the MFC list helpers (reloc-masked)

// The global free-list of recycled list nodes (an intrusive singly-linked stack)
// and its node-bias constant, shared across the manager's list churn. Stored as
// raw void*/int (the engine's real ?g_freeList@@3PAXA / ?g_freeListNodeBias@@3HA).
extern void* g_freeList;       // ?g_freeList@@3PAXA        @0x645544
extern int g_freeListNodeBias; // ?g_freeListNodeBias@@3HA  @0x64554c

// operator new / operator delete (static CRT, reloc-masked).
void* operator new(unsigned int);
void operator delete(void*);

class CTriggerMgr {
public:
    // --- the small reconstructed leaf interface (retail-RVA order) -------------
    // 0x6b640: store the supplied object at +0x22c and clear three companion
    // state words; returns 1 (0 when arg is null).
    int SetLevel(void* lvl);

    // 0x78a30: forward to the overlay sub-object's helper when present, else ret.
    void OverlayTick();

    // 0x79b00: forward-and-free the overlay sub-object when present; ret 1.
    int OverlayRelease();

    // 0x79b30: linear search of the byte table (+0x264, count +0x268) for `b`;
    // ret 1 on hit else 0.
    int ByteTableHas(int b);

    // 0x78430: ResetAll - drain the record list (+0x244), clearing each referenced
    // grid cell's sprites and recycling the node; RemoveAll the +0x240 list, run
    // StopPendingFx, then flag the goal (+0x23c).
    void ResetAll();

    // 0x784d0: scan the record list (+0x244) for a node whose payload (x,y)
    // matches; ret 1 on hit else 0.
    int RecordListHas(int x, int y);

    // 0x7d140: ClearRow(row) - for the 15 cells of grid row `row` (+0x1c), run each
    // live cell's ExitGrid (unless it has a notify hook); clear +0x400 when row is the
    // magic group, then refresh the world. ret 1.
    int ClearRow(int row);

    // 0x7d2a0: only when `key` equals the magic kind (g_644c54), scan the 10 selection
    // lists (+0x2d4, stride 0x1c) for a node payload matching (key,y); ret the list
    // index of the first match (0xa when a second match exists) or 0.
    int SelectionListFind(int key, int y);

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
    int CellDispatch(int row, int col, int kind, int arg);

    // 0x79fb0: the notify-cell hook CellDispatch tails into when a cell has a +0x368
    // hook. UNRECONSTRUCTED (still a stub); declared so CellDispatch's reloc-masked
    // self-call mangles onto this class. No body in this TU.
    void NotifyCell(int row, int col, int z);

    // 0x6bea0: scan the 15-row x N-col cell grid for the cell whose object bounds
    // contain (px,py); writes the found (row,col) through the out-ptrs and returns
    // the cell pointer (0 when none). (__stdcall: ret 0x14.)
    void* CellHitTest(int row0, int px, int py, int* outRow, int* outCol);
};

#endif
