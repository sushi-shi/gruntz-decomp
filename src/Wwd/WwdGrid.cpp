#include <rva.h>
// <Mfc.h> for the /GX EH frame helpers + CObject-style base. The grid's bucket
// list helpers, the engine allocator, and the vector ctor/dtor iterators are all
// reloc-masked engine externs (no bodies here).
#include <Mfc.h>

#include <Gruntz/WwdGrid.h>
#include <Wwd/SubWidget168080.h> // the sibling 0x44 grid (its dtor 0x1682a0 lives in this obj)
#include <Gruntz/WwdGridIter.h> // CWwdGridIter cursor - Start/Init/GetNext bodies live
                                // here (0x191ad0..0x191c30, same obj as CWwdGrid); shared
                                // with WwdSpatialMgr.cpp (its GetFirst/GetNext API driver)

// --- reloc-masked engine externs -------------------------------------------

// operator delete (0x1b9b82, ??3@YAXPAX@Z): the engine Rez heap free IS the global
// operator delete (FID-verified library label).
//
// (There used to be a hand-rolled `void __stdcall Tm_DestroyArray(...)` declared here as
// a stand-in for the "un-spellable" ??_M vector-dtor iterator. It IS spellable: retail's
// FreeBuckets is a plain `delete[] m_buckets` - cl reads the count cookie at [p-4], calls
// ??_M(p, sizeof, count, &~BucketHead), then operator delete(p-4), which is byte-for-byte
// what 0x191800 does. ?Tm_DestroyArray@@YGXPAXHIP6AXXZ@Z was a symbol NOTHING defines,
// while ??_M@YGXPAXIHP6EX0@Z@Z is right there in the CRT libs. Writing the real construct
// makes cl emit the real call.)
void operator delete(void* p);
extern "C" double log(double);
extern "C" double pow(double, double);

// ===========================================================================
// 0x1682a0 - ~CWwdGrid: stamp own vtable, free the bucket array; the base dtor
// (folded inline) then restores the parent vtable. /GX frame from the base
// subobject teardown.
// ===========================================================================
// Real polymorphic now: cl emits the implicit ??_7CWwdGrid own-vptr stamp in the
// ENTRY state (stamp-first, == retail), then FreeBuckets, then the ~CObject
// grand-base re-stamp folds in. /GX frame from the destructible base subobject.
// (eh-dtor-implicit-vptr-stamp-first.md.)
// SubWidget_168080::~SubWidget_168080 @0x1682a0 - the SIBLING grid class's dtor (its own
// 6-slot vtable ??_7SubWidget_168080 @0x1f0310; class in <Wwd/SubWidget168080.h>). Same
// shape as ~CWwdGrid: stamp own vptr, free the bucket array, fold the CObject grand-base.
// IDENTITY (vtable-owner probe): ??_7 @0x1f0310 slot 1 -> the sdd 0x168280 -> THIS body.
// It was misbound as CWwdGrid::~CWwdGrid, which pushed the REAL ~CWwdGrid (0x168c10) onto
// a fake "second COMDAT copy" placeholder (C168c10) - an impossible story, since MSVC5
// keeps exactly ONE COMDAT per mangled name. Two grids, two vtables, two dtors.
// The FreeBuckets cast is the honest residue of the sibling's still-unmodelled layout
// (it runs the same bucket-array teardown on the same offsets).
RVA(0x001682a0, 0x46)
SubWidget_168080::~SubWidget_168080() {
    ((CWwdGrid*)this)->FreeBuckets();
}

// 0x168c10 - the REAL CWwdGrid::~CWwdGrid: stamp ??_7CWwdGrid (0x5f0328), free the bucket
// array, then the ~CObject grand-base re-stamp folds in. /GX frame from the destructible
// base subobject. IDENTITY (vtable-owner probe): ??_7CWwdGrid @0x1f0328 slot 1 -> the sdd
// 0x168bf0 -> THIS body. (eh-dtor-implicit-vptr-stamp-first.md.)
RVA(0x00168c10, 0x46)
CWwdGrid::~CWwdGrid() {
    FreeBuckets();
}

// ===========================================================================
// Homed from src/Stub/GapFunctions.cpp (matcher-5): the gap tool had merged the
// BucketHead `??_E` (0x191720, size 0x50) with the CWwdGrid span-ctor forwarder
// (0x191770, size 0x8d) into one 0xdd-byte span - now split.
// ===========================================================================
// 0x191720 = BucketHead's `vector deleting destructor' (??_E): the COMPILER-GENERATED
// vector deleting destructor for BucketHead, emitted from the `new BucketHead[m_cellCount]`
// in the ctor above (the array-cookie alloc + ehvec ctor/dtor pair). Not a hand-written
// method: cl auto-emits ??_EBucketHead@@QAEPAXI@Z; @rva-symbol names it at this RVA so the
// delinker pairs the retail orphan (a zero-ref COMDAT; FreeBuckets inlines its own ehvec).
// @rva-symbol: ??_EBucketHead@@QAEPAXI@Z 0x00191720 0x50

// @early-stop
// 0x191770 = a __thiscall(this; x0,y0,x1,y1) helper that derives the cell sizes
// (cellW = |x1-x0|/5, cellH = |y1-y0|/5 via the 0x66666667 /5 magic-divide) and
// tail-calls CWwdGrid::CWwdGrid(x0,y0,x1,y1,cellW,cellH) @0x1915c0 on `this` (a
// re-init / 4-arg ctor overload). Body parked: invoking a ctor as a function on an
// existing object (and CWwdGrid being abstract - the pure OnFound - forbids placement
// new) is not expressible in MSVC5 C++, so the delegating call cannot be regenerated.
RVA(0x00191770, 0x8d)
i32 Gap_191770(void) {
    return 0;
}

// ===========================================================================
// 0x191800 - FreeBuckets: if allocated, run the vector dtor over the node array
// and release the backing block; then clear the alloc-OK flag.
// ===========================================================================
RVA(0x00191800, 0x39)
void CWwdGrid::FreeBuckets() {
    if (m_allocated) {
        // cl lowers this to retail's exact sequence: null-check, read the array-cookie
        // count at [m_buckets-4], ??_M(m_buckets, 8, count, &~BucketHead), then
        // operator delete(m_buckets-4).
        delete[] m_buckets;
        m_allocated = 0;
    }
}

// ===========================================================================
// 0x191840 - Add(region): compute the region's cell index from its pixel
// position, cache the owning bucket, link it in, bump the count.
// ===========================================================================
RVA(0x00191840, 0x48)
i32 CWwdGrid::Add(WwdRegion* r) {
    i32 col = (r->m_y - m_minY) >> m_shiftX;
    i32 row = (r->m_x - m_minX) >> m_shiftY;
    BucketHead* bucket = m_buckets + (col * m_cols + row);
    r->m_bucket = bucket;
    bucket->InsertHead(r); // DSoundList::InsertHead @0x1390e0 (r upcasts to DSoundLink*)
    ++m_count;
    return 1;
}

// ===========================================================================
// 0x191890 - Remove(region): unlink from its cached bucket, clear it, dec count.
// ===========================================================================
RVA(0x00191890, 0x24)
void CWwdGrid::Remove(WwdRegion* r) {
    r->m_bucket->Unlink(r); // DSoundList::Unlink @0x1391e0
    r->m_bucket = 0;
    --m_count;
}

// ===========================================================================
// 0x1918c0 - Query(x0,y0,x1,y1,doRemove): walk the cells overlapping the rect;
// for each region truly inside the rect, fire the virtual callback (and, when
// doRemove, unlink it first). Returns how many fired.
// ===========================================================================
// @early-stop
// 99.7% - entropy tail: body byte-identical bar a `lea ebp,[ecx*8+0]` vs
// `[8*ecx]` addressing-mode encoding + one [esp] reload-pair load-order swap
// (scheduling). No source diff closes it. (Wrapping the loop in `if(colA<=colB)`
// instead of an early `return fired` was the real fix: 95%->99.7%, killing the
// duplicate early-return epilogue - retail jg's to the shared exit.)
RVA(0x001918c0, 0x1a2)
i32 CWwdGrid::Query(i32 a0, i32 a1, i32 a2, i32 a3, i32 doRemove) {
    i32 fired = 0;
    if (a0 > m_maxX) {
        return 0;
    }
    if (a2 < m_minX) {
        return 0;
    }
    if (a1 > m_maxY) {
        return 0;
    }
    if (a3 < m_minY) {
        return 0;
    }
    if (a0 < m_minX) {
        a0 = m_minX;
    }
    if (a2 > m_maxX) {
        a2 = m_maxX;
    }
    if (a1 < m_minY) {
        a1 = m_minY;
    }
    if (a3 > m_maxY) {
        a3 = m_maxY;
    }
    i32 colA = (a1 - m_minY) >> m_shiftX;
    i32 rowA = (a0 - m_minX) >> m_shiftY;
    i32 colB = (a3 - m_minY) >> m_shiftX;
    i32 rowB = (a2 - m_minX) >> m_shiftY;
    i32 base = colA * m_cols + rowA;
    if (colA <= colB) {
        i32 colN = colB - colA + 1;
        do {
            if (rowA <= rowB) {
                i32 rowN = rowB - rowA + 1;
                i32 idx = base;
                do {
                    WwdRegion* r = (WwdRegion*)m_buckets[idx].m_head; // list head -> typed node
                    while (r) {
                        i32 x = r->m_x;
                        WwdRegion* next = (WwdRegion*)r->m_next;
                        if (x >= a0 && r->m_y >= a1 && x <= a2 && r->m_y <= a3) {
                            if (doRemove) {
                                m_buckets[idx].Unlink(r);
                                r->m_bucket = 0;
                                --m_count;
                            }
                            OnFound(r);
                            ++fired;
                        }
                        r = next;
                    }
                    ++idx;
                } while (--rowN);
            }
            base += m_cols;
        } while (--colN);
    }
    return fired;
}

// ===========================================================================
// 0x191a70 - Clear: unlink and reset every bucket's list, return how many cells
// held something; zero the count.
// ===========================================================================
RVA(0x00191a70, 0x57)
i32 CWwdGrid::Clear() {
    i32 nonEmpty = 0;
    for (i32 i = 0; i < m_cellCount; ++i) {
        WwdRegion* r = (WwdRegion*)m_buckets[i].m_head;
        while (r) {
            m_buckets[i].Unlink(r);
            r->m_bucket = 0;
            ++nonEmpty;
            r = (WwdRegion*)m_buckets[i].m_head;
        }
    }
    m_count = 0;
    return nonEmpty;
}

// ===========================================================================
// CWwdGridIter cursor methods (the tomalla-67 cluster) - re-homed here from
// WwdSpatialMgr.cpp (matcher-2 D6 drain): 0x191ad0..0x191c30 sit in THIS obj's
// contiguous .text run (right after Clear @0x191a70), not the CWwdSpatialMgr obj
// (0x1682f0..). The cursor walks every grid cell overlapping the query rect,
// visiting each node truly inside it (optionally unlinking it). It reads the
// canonical CWwdGrid's bounds/shift/cols/bucket fields directly; the buckets are
// the canonical BucketHead ({head,tail} DSoundList) so the node reads cast through
// (WwdGridNode*) exactly as CWwdGrid::Query/Clear do.
// ===========================================================================

// 0x191ad0 - Start(grid, remove): seed the cursor over the grid's ENTIRE bounds
// rect (grid->minX..maxY passed by value) and return the first in-rect node.
RVA(0x00191ad0, 0x34)
WwdGridNode* CWwdGridIter::Start(CWwdGrid* grid, i32 remove) {
    // The grid's full bounds rect (minX,minY,maxX,maxY @ +0x28..+0x34) copied as a
    // contiguous 16-byte block - the four bounds ints ARE the query rect.
    WwdRect full = *(WwdRect*)&grid->m_minX;
    return Init(grid, full, remove);
}

// 0x191b10 - Init(grid, rect, remove): cache the grid + clamped query rect, fail
// fast if the rect is fully outside the grid, derive the cell-range corners and
// the live cell-walk counters, prime the cursor at the first cell head, then
// advance to the first in-rect node.
// @early-stop
// ~95.5% - imul regalloc wall: body byte-identical (the rect block-copy + all 8
// clamp guards + the cell-range corners match), but the final
// base=colStart*cols+rowStart keeps colStart in a different reg than retail
// (retail imul edi,ecx + m_col=ecx between imul/add; recompile imul ecx,esi),
// a 3-instr operand-register choice. Not source-steerable (see
// docs/patterns/zero-register-pinning.md / statement-schedule-faithful.md).
RVA(0x00191b10, 0x111)
WwdGridNode* CWwdGridIter::Init(CWwdGrid* grid, WwdRect rect, i32 remove) {
    m_grid = grid;
    m_rect = rect;
    m_remove = remove;
    if (m_rect.a > grid->m_maxX) {
        return 0;
    }
    if (m_rect.c < grid->m_minX) {
        return 0;
    }
    if (m_rect.b > grid->m_maxY) {
        return 0;
    }
    if (m_rect.d < grid->m_minY) {
        return 0;
    }
    if (m_rect.a < grid->m_minX) {
        m_rect.a = grid->m_minX;
    }
    if (m_rect.c > grid->m_maxX) {
        m_rect.c = grid->m_maxX;
    }
    if (m_rect.b < grid->m_minY) {
        m_rect.b = grid->m_minY;
    }
    if (m_rect.d > grid->m_maxY) {
        m_rect.d = grid->m_maxY;
    }
    m_colStart = (m_rect.b - grid->m_minY) >> grid->m_shiftX;
    m_rowStart = (m_rect.a - grid->m_minX) >> grid->m_shiftY;
    m_colEnd = (m_rect.d - grid->m_minY) >> grid->m_shiftX;
    m_rowEnd = (m_rect.c - grid->m_minX) >> grid->m_shiftY;
    i32 base = m_colStart * grid->m_cols + m_rowStart;
    m_col = m_colStart;
    m_row = m_rowStart;
    m_rowBase = base;
    m_cell = base;
    m_next = (WwdGridNode*)grid->m_buckets[base].m_head;
    return GetNext();
}

// 0x191c30 - GetNext(): resume the cell walk. Faithful goto transcription of the
// retail cursor: reload the scan node, advance cells until a non-empty bucket,
// then walk the bucket testing each node against the query rect; on a hit,
// optionally unlink it and return it.
// @early-stop
// ~93.7% - LICM/regalloc wall: the cell-advance block + the whole control flow
// are byte-identical, but cl hoists the loop-invariant query bound m_rect.a
// (+0x10) into a callee-saved register (extra push ebx) where retail reloads it
// from memory each iteration; this cascades the walk's m_y0 reg (ebx vs edi) and
// the remove-block reg assignment. Not source-steerable (member-bound LICM
// choice; see docs/patterns/zero-register-pinning.md).
RVA(0x00191c30, 0xcc)
WwdGridNode* CWwdGridIter::GetNext() {
    WwdGridNode* node;
top:
    node = m_next;
    m_cur = node;
    if (node == 0) {
    nextcell:
        if (m_row < m_rowEnd) {
            ++m_cell;
            ++m_row;
        } else {
            if (m_col >= m_colEnd) {
                return 0;
            }
            m_rowBase += m_grid->m_cols;
            m_cell = m_rowBase;
            m_row = m_rowStart;
            ++m_col;
        }
        m_cur = (WwdGridNode*)m_grid->m_buckets[m_cell].m_head;
        if (m_cur == 0) {
            goto nextcell;
        }
    }
    if (m_cur == 0) {
        goto top;
    }
walk:
    m_next = m_cur->m_next;
    if (m_cur->m_x >= m_rect.a && m_cur->m_y >= m_rect.b && m_cur->m_x <= m_rect.c
        && m_cur->m_y <= m_rect.d) {
        if (m_remove) {
            m_grid->m_buckets[m_cell].Unlink((DSoundLink*)m_cur);
            m_cur->m_bucket = 0;
            --m_grid->m_count;
        }
        return m_cur;
    }
    if (m_cur != 0) {
        goto walk;
    }
    goto top;
}

// BucketHead::~BucketHead (0x191d10): the empty destructor - a bare 1-byte `ret` (a
// {head,tail} DSoundList has trivial teardown). OUT-OF-LINE on purpose (declared, not
// defined, in WwdGrid.h): that is what makes cl emit the real ??_M vector-dtor call in
// FreeBuckets instead of proving the loop away. It was modelled as a free function
// `BucketHead_Dtor` whose address FreeBuckets passed by hand.
// (Ghidra's __fpclear FID label was a 1-byte-`ret` false hit.)
RVA(0x00191d10, 1)
BucketHead::~BucketHead() {}

// ===========================================================================
// 0x1915c0 - ctor: normalize the rect, compute log2 cell shifts + power-of-two
// cell sizes, derive the cols/rows/cell-count, allocate the bucket array
// (count cookie + vector-constructed 8-byte heads). /GX frame from the alloc EH.
// ===========================================================================
// @early-stop
// ~74%: three stacked walls, all logic byte-faithful. (1) the log2/pow x87 path
// (fldln2/fld 2.0/fyl2x/fdiv/__ftol/__CIpow) has a non-steerable FP-stack
// schedule (docs/patterns/x87-fp-stack-schedule.md). (2) the real polymorphic
// CObject subobject shifts the /GX __ehfuncinfo state-id base
// (docs/patterns/eh-state-numbering-base.md) - the trade that took the dtor to
// 100% (real-virtual implicit stamp-first). (3) the 4 rect-field stores fuse to a
// `lea edx,[esi+0x28]` pointer-block in retail but stay direct member stores here
// (regalloc choice). The implicit ctor vptr-stamp is now compiler-emitted (the
// abstract base's __purecall vtable), matching retail's polymorphic construction.
RVA(0x001915c0, 0x15d)
CWwdGrid::CWwdGrid(i32 x0, i32 y0, i32 x1, i32 y1, i32 cellW, i32 cellH) {
    m_count = 0;
    m_minX = x0;
    m_minY = y0;
    m_maxX = x1;
    m_maxY = y1;
    i32 lox = x0, hix = x1;
    if (x1 < x0) {
        lox = x1;
        hix = x0;
    }
    i32 loy = y0, hiy = y1;
    if (y1 < y0) {
        loy = y1;
        hiy = y0;
    }
    m_width = hix - lox;
    m_height = hiy - loy;
    m_shiftY = (i32)(log((double)cellW) / log(2.0));
    m_shiftX = (i32)(log((double)cellH) / log(2.0));
    m_cellH = (i32)pow(2.0, (double)m_shiftY);
    m_cellW = (i32)pow(2.0, (double)m_shiftX);
    m_cols = m_width / m_cellH + 1;
    m_rows = m_height / m_cellW + 1;
    m_cellCount = m_rows * m_cols;
    BucketHead* arr = new BucketHead[m_cellCount];
    m_buckets = arr;
    if (arr) {
        m_allocated = 1;
    }
}
