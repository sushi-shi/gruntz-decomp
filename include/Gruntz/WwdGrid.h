#ifndef GRUNTZ_WWDGRID_H
#define GRUNTZ_WWDGRID_H

#include <Ints.h>
#include <Wap32/Object.h>           // CObject - the shared engine grand-base
#include <Dsndmgr/SoundVoiceList.h> // the engine's ONE {head,tail} intrusive-list
#include <Gruntz/WwdGridIter.h>     // WwdRegion (the node) + the rect cursor
#include <rva.h>

struct BucketHead;

struct BucketHead : DSoundList { // {m_head,m_tail} + InsertHead/Unlink inherited
    // The empty chain comes from the DSoundList base ctor; a body store would sit on
    // the wrong side of a stamp if the class ever gains one.
    BucketHead() {}
    // DECLARED here, DEFINED out-of-line in WwdGrid.cpp (0x191d10, a bare `ret`). That is
    // load-bearing: with an inline `~BucketHead() {}` cl sees the teardown is a no-op and
    // elides the vector-dtor loop from `delete[]` entirely - no ??_M call. Retail's
    // FreeBuckets DOES call ??_M, so the dtor was opaque to it, i.e. out-of-line.
    ~BucketHead();
};
SIZE_UNKNOWN();

class CWwdGrid : public CObject {
public:
    CWwdGrid() {
        m_allocated = 0;
    }
    virtual ~CWwdGrid() OVERRIDE {
        FreeBuckets();
    }
    virtual void OnFound(WwdRegion* r) = 0; // [5] retail slot = __purecall

    i32 Setup(RECT rect, i32 cellW, i32 cellH);
    i32 Setup(RECT rect);

    void FreeBuckets();
    i32 Add(WwdRegion* r);
    void Remove(WwdRegion* r);
    i32 Query(WwdRect q, i32 doRemove); // by value: retail's caller builds the 16-byte arg block
    i32 Clear();

    i32 m_allocated; // +0x04  buckets-allocated flag
    i32 m_count;     // +0x08  live object count
    i32 m_cols;      // +0x0c  width/cellH + 1
    i32 m_rows;      // +0x10  height/cellW + 1
    i32 m_shiftY;    // +0x14  log2(cellW)
    i32 m_shiftX;    // +0x18  log2(cellH)
    i32 m_cellCount; // +0x1c  numCells = cols*rows
    i32 m_width;     // +0x20
    i32 m_height;    // +0x24
    // +0x28..+0x37: the grid's full bounds. These four adjacent ints ARE one
    // WwdRect - CWwdGridIter::Start @0x191ad0 block-copies them straight into
    // its `WwdRect rect` by-value argument - so they are declared as one.
    WwdRect m_bounds;      // +0x28  m_minX/m_minY/m_maxX/m_maxY
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
