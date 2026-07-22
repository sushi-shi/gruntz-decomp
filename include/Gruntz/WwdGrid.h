#ifndef GRUNTZ_WWDGRID_H
#define GRUNTZ_WWDGRID_H

#include <Ints.h>
#include <Wap32/Object.h>           // CObject - the shared engine grand-base
#include <Dsndmgr/SoundVoiceList.h> // the engine's ONE {head,tail} intrusive-list
#include <Gruntz/WwdGridIter.h>     // WwdRegion (the node) + the rect cursor
#include <rva.h>

struct BucketHead;

struct BucketHead : DSoundList { // {m_head,m_tail} + InsertHead/Unlink inherited
    BucketHead() {
        m_head = 0;
        m_tail = 0;
    }
    // DECLARED here, DEFINED out-of-line in WwdGrid.cpp (0x191d10, a bare `ret`). That is
    // load-bearing: with an inline `~BucketHead() {}` cl sees the teardown is a no-op and
    // elides the vector-dtor loop from `delete[]` entirely - no ??_M call. Retail's
    // FreeBuckets DOES call ??_M, so the dtor was opaque to it, i.e. out-of-line.
    ~BucketHead();
};
SIZE_UNKNOWN();

class CWwdGrid : public CObject {
public:
    // ctor: build the grid over rect (x0,y0,x1,y1) with cell sizes cellW/cellH.
    // (the empty default ctor exists for the concrete CWwdGridShell's two-phase
    // construction; its base ??_7 stamp dead-store-elides under the derived stamp)
    CWwdGrid() {}
    CWwdGrid(i32 x0, i32 y0, i32 x1, i32 y1, i32 cellW, i32 cellH);
    virtual ~CWwdGrid() OVERRIDE;
    // slot 5 (vtbl+0x14): __purecall in the retail 0x1f0328 vtable - the class is
    // abstract THERE. Declared non-pure (and defined nowhere) so CWwdSpatialMgr::Init
    // can run this ctor over the raw CWwdGridShell via placement-new, exactly as
    // retail's caller TU must have (its Init calls ??0CWwdGrid on the shell storage).
    // Only the vtable DATUM's slot-5 reloc differs (ours names OnFound, retail
    // __purecall) - a data-phase footnote, not a code byte.
    virtual void OnFound(WwdRegion* r);

    void FreeBuckets();
    i32 Add(WwdRegion* r);
    void Remove(WwdRegion* r);
    i32 Query(i32 x0, i32 y0, i32 x1, i32 y1, i32 doRemove);
    i32 Clear();

    i32 m_allocated;       // +0x04  buckets-allocated flag
    i32 m_count;           // +0x08  live object count
    i32 m_cols;            // +0x0c  width/cellH + 1
    i32 m_rows;            // +0x10  height/cellW + 1
    i32 m_shiftY;          // +0x14  log2(cellW)
    i32 m_shiftX;          // +0x18  log2(cellH)
    i32 m_cellCount;       // +0x1c  numCells = cols*rows
    i32 m_width;           // +0x20
    i32 m_height;          // +0x24
    i32 m_minX;            // +0x28
    i32 m_minY;            // +0x2c
    i32 m_maxX;            // +0x30
    i32 m_maxY;            // +0x34
    i32 m_cellH;           // +0x38  2^shiftY
    i32 m_cellW;           // +0x3c  2^shiftX
    BucketHead* m_buckets; // +0x40
};
SIZE(0x44);

// TU-local thunk/table names this TU registers (moved from the .cpp; the
// addresses are ILT thunk VAs, reloc-masked at every use).
extern "C" double log(double);

// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---
extern "C" double pow(double, double);

#endif // GRUNTZ_WWDGRID_H
