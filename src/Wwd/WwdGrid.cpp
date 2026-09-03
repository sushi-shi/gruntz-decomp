#include <rva.h>

#include <Mfc.h>

#include <Gruntz/WwdGrid.h>
#include <Gruntz/WwdGridIter.h>
#include <Wwd/WwdGridShell.h>

#include <math.h>
#include <string.h>

// @early-stop
RVA(0x001915c0, 0x15d)
i32 CWwdGrid::Setup(RECT rect, i32 cellW, i32 cellH) {
    m_count = 0;
    m_bounds.Init(rect.left, rect.top, rect.right, rect.bottom);
    CRect normalized = rect;
    normalized.NormalizeRect();
    m_extent = normalized.Size();
    m_cellShift = CSize(
        static_cast<i32>((log(static_cast<double>(cellW)) / log(2.0))),
        static_cast<i32>((log(static_cast<double>(cellH)) / log(2.0)))
    );
    m_cellSize = CSize(
        static_cast<i32>(
            pow(DATA_COMPGEN(0x001f0ab0, 2.0), static_cast<double>(m_cellShift.cx))
            ),
            static_cast<i32>(pow(2.0, static_cast<double>(m_cellShift.cy)))
    );
    m_gridSize = CSize(m_extent.cx / m_cellSize.cx + 1, m_extent.cy / m_cellSize.cy + 1);
    m_cellCount = m_gridSize.cy * m_gridSize.cx;
    BucketHead* arr = new BucketHead[m_cellCount];
    m_buckets = arr;
    if (arr == NULL) {
        return 0;
    }
    m_allocated = true;
    return 1;
}
RVA_COMPGEN(0x00191720, 0x50, ??_EBucketHead@@QAEPAXI@Z)

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00191770, 0x8d)
i32 CWwdGrid::Setup(RECT rect) {
    CRect normalized = rect;
    normalized.NormalizeRect();
    return Setup(rect, normalized.Width() / 10, normalized.Height() / 10);
}

RVA(0x00191800, 0x39)
void CWwdGrid::FreeBuckets() {
    if (m_allocated) {

        delete[] m_buckets;
        m_allocated = false;
    }
}

RVA(0x00191840, 0x48)
i32 CWwdGrid::Add(WwdRegion* r) {
    Coord cell(
        (r->m_position.m_x - m_bounds.m_minX) >> m_cellShift.cx,
        (r->m_position.m_y - m_bounds.m_minY) >> m_cellShift.cy
    );
    BucketHead* bucket = m_buckets + (cell.m_y * m_gridSize.cx + cell.m_x);
    r->m_bucket = bucket;
    bucket->InsertFirst(r);
    ++m_count;
    return 1;
}

RVA(0x00191890, 0x24)
void CWwdGrid::Remove(WwdRegion* r) {
    r->m_bucket->Delete(r);
    r->m_bucket = NULL;
    --m_count;
}

// @early-stop
RVA(0x001918c0, 0x1a2)

i32 CWwdGrid::Query(WwdRect q, i32 doRemove) {
    i32 fired = 0;
    if (!q.Intersects(m_bounds)) {
        return 0;
    }
    q.Intersect(m_bounds);
    WwdRect cell;
    cell.Init(
        (q.m_minX - m_bounds.m_minX) >> m_cellShift.cx,
        (q.m_minY - m_bounds.m_minY) >> m_cellShift.cy,
        (q.m_maxX - m_bounds.m_minX) >> m_cellShift.cx,
        (q.m_maxY - m_bounds.m_minY) >> m_cellShift.cy
    );
    i32 base = cell.m_minY * m_gridSize.cx + cell.m_minX;
    if (cell.m_minY <= cell.m_maxY) {
        i32 rowCount = cell.m_maxY - cell.m_minY + 1;
        do {
            if (cell.m_minX <= cell.m_maxX) {
                i32 colCount = cell.m_maxX - cell.m_minX + 1;
                i32 idx = base;
                do {
                    WwdRegion* r = static_cast<WwdRegion*>(m_buckets[idx].GetFirst());
                    while (r) {
                        WwdRegion* next = static_cast<WwdRegion*>(r->Next());
                        if (q.Contains(r)) {
                            if (doRemove) {
                                m_buckets[idx].Delete(r);
                                r->m_bucket = NULL;
                                --m_count;
                            }
                            OnFound(r);
                            ++fired;
                        }
                        r = next;
                    }
                    ++idx;
                } while (--colCount);
            }
            base += m_gridSize.cx;
        } while (--rowCount);
    }
    return fired;
}

RVA(0x00191a70, 0x57)
i32 CWwdGrid::Clear() {
    i32 nonEmpty = 0;
    for (i32 i = 0; i < m_cellCount; ++i) {
        WwdRegion* r = static_cast<WwdRegion*>(m_buckets[i].GetFirst());
        while (r) {
            m_buckets[i].Delete(r);
            r->m_bucket = NULL;
            ++nonEmpty;
            r = static_cast<WwdRegion*>(m_buckets[i].GetFirst());
        }
    }
    m_count = 0;
    return nonEmpty;
}

RVA(0x00191ad0, 0x34)
WwdRegion* CWwdGridIter::Start(CWwdGrid* grid, i32 remove) {

    WwdRect full = grid->m_bounds;
    return Init(grid, full, remove);
}

// @early-stop
RVA(0x00191b10, 0x111)
WwdRegion* CWwdGridIter::Init(CWwdGrid* grid, WwdRect rect, i32 remove) {
    m_grid = grid;
    m_rect = rect;
    m_remove = remove;
    if (!m_rect.Intersects(grid->m_bounds)) {
        return NULL;
    }
    m_rect.Intersect(grid->m_bounds);
    m_rowStart = (m_rect.m_minY - grid->m_bounds.m_minY) >> grid->m_cellShift.cy;
    m_colStart = (m_rect.m_minX - grid->m_bounds.m_minX) >> grid->m_cellShift.cx;
    m_rowEnd = (m_rect.m_maxY - grid->m_bounds.m_minY) >> grid->m_cellShift.cy;
    m_colEnd = (m_rect.m_maxX - grid->m_bounds.m_minX) >> grid->m_cellShift.cx;
    i32 base = m_rowStart * grid->m_gridSize.cx + m_colStart;
    m_row = m_rowStart;
    m_col = m_colStart;
    m_rowBase = base;
    m_cell = base;
    m_next = static_cast<WwdRegion*>(grid->m_buckets[base].GetFirst());
    return GetNext();
}

// @early-stop
RVA(0x00191c30, 0xcc)
WwdRegion* CWwdGridIter::GetNext() {
    for (;;) {
        m_cur = m_next;
        while (m_cur == NULL) {
            if (m_col < m_colEnd) {
                ++m_cell;
                ++m_col;
            } else {
                if (m_row >= m_rowEnd) {
                    return NULL;
                }
                m_rowBase += m_grid->m_gridSize.cx;
                m_cell = m_rowBase;
                m_col = m_colStart;
                ++m_row;
            }
            m_cur = static_cast<WwdRegion*>(m_grid->m_buckets[m_cell].GetFirst());
        }
        while (m_cur != NULL) {
            m_next = static_cast<WwdRegion*>(m_cur->Next());
            if (!m_rect.Contains(m_cur)) {
                // Unreachable era bug: a rejected region repeats without advancing m_cur.
                continue;
            }
            if (m_remove) {
                m_grid->m_buckets[m_cell].Delete(m_cur);
                m_cur->m_bucket = NULL;
                --m_grid->m_count;
            }
            return m_cur;
        }
    }
}

RVA_COMPGEN(0x00191d00, 0x10, ??0BucketHead@@QAE@XZ)
RVA(0x00191d10, 0x1)
BucketHead::~BucketHead() {}
