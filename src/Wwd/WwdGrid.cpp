#include <rva.h>

#include <Mfc.h>

#include <Gruntz/WwdGrid.h>
#include <Gruntz/WwdGridIter.h>
#include <Wwd/WwdGridShell.h>

#include <math.h>

RVA(0x001682a0, 0x46)
CWwdGridShell::~CWwdGridShell() {}

RVA_COMPGEN(0x00168bf0, 0x1e, ??_GCWwdGrid@@UAEPAXI@Z)
RVA_COMPGEN(0x00168c10, 0x46, ??1CWwdGrid@@UAE@XZ)

RVA(0x001915c0, 0x15d)
i32 CWwdGrid::Setup(RECT rect, i32 cellW, i32 cellH) {
    m_count = 0;
    m_bounds.m_minX = rect.left;
    m_bounds.m_minY = rect.top;
    m_bounds.m_maxX = rect.right;
    m_bounds.m_maxY = rect.bottom;
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

        delete[] m_buckets;
        m_allocated = 0;
    }
}

RVA(0x00191840, 0x48)
i32 CWwdGrid::Add(WwdRegion* r) {
    i32 col = (r->m_y - m_bounds.m_minY) >> m_shiftX;
    i32 row = (r->m_x - m_bounds.m_minX) >> m_shiftY;
    BucketHead* bucket = m_buckets + (col * m_cols + row);
    r->m_bucket = bucket;
    bucket->InsertHead(r);
    ++m_count;
    return 1;
}

RVA(0x00191890, 0x24)
void CWwdGrid::Remove(WwdRegion* r) {
    r->m_bucket->Unlink(r);
    r->m_bucket = NULL;
    --m_count;
}

// @early-stop
RVA(0x001918c0, 0x1a2)

i32 CWwdGrid::Query(WwdRect q, i32 doRemove) {
    i32 fired = 0;
    if (q.m_minX > m_bounds.m_maxX) {
        return 0;
    }
    if (q.m_maxX < m_bounds.m_minX) {
        return 0;
    }
    if (q.m_minY > m_bounds.m_maxY) {
        return 0;
    }
    if (q.m_maxY < m_bounds.m_minY) {
        return 0;
    }
    if (q.m_minX < m_bounds.m_minX) {
        q.m_minX = m_bounds.m_minX;
    }
    if (q.m_maxX > m_bounds.m_maxX) {
        q.m_maxX = m_bounds.m_maxX;
    }
    if (q.m_minY < m_bounds.m_minY) {
        q.m_minY = m_bounds.m_minY;
    }
    if (q.m_maxY > m_bounds.m_maxY) {
        q.m_maxY = m_bounds.m_maxY;
    }
    WwdRect cell;
    cell.m_minY = (q.m_minY - m_bounds.m_minY) >> m_shiftX;
    cell.m_minX = (q.m_minX - m_bounds.m_minX) >> m_shiftY;
    cell.m_maxY = (q.m_maxY - m_bounds.m_minY) >> m_shiftX;
    cell.m_maxX = (q.m_maxX - m_bounds.m_minX) >> m_shiftY;
    i32 base = cell.m_minY * m_cols + cell.m_minX;
    if (cell.m_minY <= cell.m_maxY) {
        i32 colN = cell.m_maxY - cell.m_minY + 1;
        do {
            if (cell.m_minX <= cell.m_maxX) {
                i32 rowN = cell.m_maxX - cell.m_minX + 1;
                i32 idx = base;
                do {
                    WwdRegion* r = static_cast<WwdRegion*>(m_buckets[idx].m_head);
                    while (r) {
                        i32 x = r->m_x;
                        WwdRegion* next = static_cast<WwdRegion*>(r->m_next);
                        if (x >= q.m_minX && r->m_y >= q.m_minY && x <= q.m_maxX
                            && r->m_y <= q.m_maxY) {
                            if (doRemove) {
                                m_buckets[idx].Unlink(r);
                                r->m_bucket = NULL;
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
            r->m_bucket = NULL;
            ++nonEmpty;
            r = static_cast<WwdRegion*>(m_buckets[i].m_head);
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
    if (m_rect.m_minX > grid->m_bounds.m_maxX) {
        return 0;
    }
    if (m_rect.m_maxX < grid->m_bounds.m_minX) {
        return 0;
    }
    if (m_rect.m_minY > grid->m_bounds.m_maxY) {
        return 0;
    }
    if (m_rect.m_maxY < grid->m_bounds.m_minY) {
        return 0;
    }
    if (m_rect.m_minX < grid->m_bounds.m_minX) {
        m_rect.m_minX = grid->m_bounds.m_minX;
    }
    if (m_rect.m_maxX > grid->m_bounds.m_maxX) {
        m_rect.m_maxX = grid->m_bounds.m_maxX;
    }
    if (m_rect.m_minY < grid->m_bounds.m_minY) {
        m_rect.m_minY = grid->m_bounds.m_minY;
    }
    if (m_rect.m_maxY > grid->m_bounds.m_maxY) {
        m_rect.m_maxY = grid->m_bounds.m_maxY;
    }
    m_colStart = (m_rect.m_minY - grid->m_bounds.m_minY) >> grid->m_shiftX;
    m_rowStart = (m_rect.m_minX - grid->m_bounds.m_minX) >> grid->m_shiftY;
    m_colEnd = (m_rect.m_maxY - grid->m_bounds.m_minY) >> grid->m_shiftX;
    m_rowEnd = (m_rect.m_maxX - grid->m_bounds.m_minX) >> grid->m_shiftY;
    i32 base = m_colStart * grid->m_cols + m_rowStart;
    m_col = m_colStart;
    m_row = m_rowStart;
    m_rowBase = base;
    m_cell = base;
    m_next = static_cast<WwdRegion*>(grid->m_buckets[base].m_head);
    return GetNext();
}

// @early-stop
RVA(0x00191c30, 0xcc)
WwdRegion* CWwdGridIter::GetNext() {
    for (;;) {
        m_cur = m_next;
        while (m_cur == NULL) {
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
        }
        while (m_cur != NULL) {
            m_next = static_cast<WwdRegion*>(m_cur->m_next);
            if (m_cur->m_x < m_rect.m_minX || m_cur->m_y < m_rect.m_minY
                || m_cur->m_x > m_rect.m_maxX || m_cur->m_y > m_rect.m_maxY) {
                m_cur = m_next;
                continue;
            }
            if (m_remove) {
                m_grid->m_buckets[m_cell].Unlink(m_cur);
                m_cur->m_bucket = NULL;
                --m_grid->m_count;
            }
            return m_cur;
        }
    }
}

RVA(0x00191d10, 0x1)
BucketHead::~BucketHead() {}

// @early-stop
