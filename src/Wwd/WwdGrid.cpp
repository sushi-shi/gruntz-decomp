#include <rva.h>
#include <Mfc.h>

#include <Gruntz/WwdGrid.h>
#include <Wwd/WwdGridShell.h>
#include <Gruntz/WwdGridIter.h> // CWwdGridIter cursor - Start/Init/GetNext bodies live

VTBL(CWwdGrid, 0x001f0328); // ??_7CWwdGrid@@6B@ (6-slot CObject-derived vtable)
void operator delete(void* p);

RVA(0x001682a0, 0x46)
CWwdGridShell::~CWwdGridShell() {}

RVA_COMPGEN(0x00168c10, 0x46, ??1CWwdGrid@@UAE@XZ)

// ===========================================================================
// Homed from src/Stub/GapFunctions.cpp (matcher-5): the gap tool had merged the
// BucketHead `??_E` (0x191720, size 0x50) with the CWwdGrid span-ctor forwarder
// (0x191770, size 0x8d) into one 0xdd-byte span - now split.
// ===========================================================================
// 0x191720 = BucketHead's `vector deleting destructor' (??_E): the COMPILER-GENERATED
// vector deleting destructor for BucketHead, emitted from the `new BucketHead[m_cellCount]`
// in the ctor above (the array-cookie alloc + ehvec ctor/dtor pair). Not a hand-written
// method: cl auto-emits ??_EBucketHead@@QAEPAXI@Z; RVA_COMPGEN names it at this RVA so the
// delinker pairs the retail orphan (a zero-ref COMDAT; FreeBuckets inlines its own ehvec).
RVA_COMPGEN(0x00191720, 0x50, ??_EBucketHead@@QAEPAXI@Z)

RVA(0x00191770, 0x8d)
i32 CWwdGrid::Setup(RECT rect) {
    i32 cellW;
    if (rect.right > rect.left) {
        cellW = (rect.right - rect.left) / 10;
    } else {
        cellW = (rect.left - rect.right) / 10;
    }
    i32 cellH;
    if (rect.bottom > rect.top) {
        cellH = (rect.bottom - rect.top) / 10;
    } else {
        cellH = (rect.top - rect.bottom) / 10;
    }
    return Setup(rect, cellW, cellH);
}

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
                    WwdRegion* r =
                        static_cast<WwdRegion*>(m_buckets[idx].m_head); // list head -> typed node
                    while (r) {
                        i32 x = r->m_x;
                        WwdRegion* next = static_cast<WwdRegion*>(r->m_next);
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

RVA(0x00191a70, 0x57)
i32 CWwdGrid::Clear() {
    i32 nonEmpty = 0;
    for (i32 i = 0; i < m_cellCount; ++i) {
        WwdRegion* r = static_cast<WwdRegion*>(m_buckets[i].m_head);
        while (r) {
            m_buckets[i].Unlink(r);
            r->m_bucket = 0;
            ++nonEmpty;
            r = static_cast<WwdRegion*>(m_buckets[i].m_head);
        }
    }
    m_count = 0;
    return nonEmpty;
}

RVA(0x00191ad0, 0x34)
WwdRegion* CWwdGridIter::Start(CWwdGrid* grid, i32 remove) {
    // The grid's full bounds rect (minX,minY,maxX,maxY @ +0x28..+0x34) copied as a
    // contiguous 16-byte block - the four bounds ints ARE the query rect.
    WwdRect full = *reinterpret_cast<WwdRect*>(&grid->m_minX);
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
WwdRegion* CWwdGridIter::Init(CWwdGrid* grid, WwdRect rect, i32 remove) {
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
    m_next = static_cast<WwdRegion*>(grid->m_buckets[base].m_head);
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
WwdRegion* CWwdGridIter::GetNext() {
    WwdRegion* node;
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
        m_cur = static_cast<WwdRegion*>(m_grid->m_buckets[m_cell].m_head);
        if (m_cur == 0) {
            goto nextcell;
        }
    }
    if (m_cur == 0) {
        goto top;
    }
walk:
    m_next = static_cast<WwdRegion*>(m_cur->m_next);
    if (m_cur->m_x >= m_rect.a && m_cur->m_y >= m_rect.b && m_cur->m_x <= m_rect.c
        && m_cur->m_y <= m_rect.d) {
        if (m_remove) {
            m_grid->m_buckets[m_cell].Unlink(m_cur);
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

RVA(0x00191d10, 0x1)
BucketHead::~BucketHead() {}

// ===========================================================================
// 0x1915c0 - Setup: normalize the rect, compute log2 cell shifts + power-of-two
// cell sizes, derive the cols/rows/cell-count, allocate the bucket array
// (count cookie + vector-constructed 8-byte heads). /GX frame from the alloc EH.
// ===========================================================================
// @early-stop
// Remaining differences: (1) the log2/pow x87 path
// (fldln2/fld 2.0/fyl2x/fdiv/__ftol/__CIpow) has a non-steerable FP-stack
// schedule (docs/patterns/x87-fp-stack-schedule.md), and (2) the four rect-field
// stores fuse to a pointer block in retail depending on source scheduling.
RVA(0x001915c0, 0x15d)
i32 CWwdGrid::Setup(RECT rect, i32 cellW, i32 cellH) {
    m_count = 0;
    m_minX = rect.left;
    m_minY = rect.top;
    m_maxX = rect.right;
    m_maxY = rect.bottom;
    i32 lox = rect.left, hix = rect.right;
    if (rect.right < rect.left) {
        lox = rect.right;
        hix = rect.left;
    }
    i32 loy = rect.top, hiy = rect.bottom;
    if (rect.bottom < rect.top) {
        loy = rect.bottom;
        hiy = rect.top;
    }
    m_width = hix - lox;
    m_height = hiy - loy;
    m_shiftY = static_cast<i32>((log(static_cast<double>(cellW)) / log(2.0)));
    m_shiftX = static_cast<i32>((log(static_cast<double>(cellH)) / log(2.0)));
    m_cellH = static_cast<i32>(pow(2.0, static_cast<double>(m_shiftY)));
    m_cellW = static_cast<i32>(pow(2.0, static_cast<double>(m_shiftX)));
    m_cols = m_width / m_cellH + 1;
    m_rows = m_height / m_cellW + 1;
    m_cellCount = m_rows * m_cols;
    BucketHead* arr = new BucketHead[m_cellCount];
    m_buckets = arr;
    if (!arr) {
        return 0;
    }
    m_allocated = 1;
    return 1;
}
