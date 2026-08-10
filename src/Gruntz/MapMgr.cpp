#include <rva.h>

#include <Gruntz/MapMgr.h>

#include <Mfc.h>

#include <Gruntz/Brickz.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/GameMode.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Io/FileMem.h>

#include <stdlib.h>
#include <string.h>

RVA(0x0009e700, 0xd)
CMapArrayA::CMapArrayA() {
    m_storage = NULL;
    m_freeList = NULL;
    m_count = 0;
}

// @early-stop
RVA(0x0009e740, 0x76)
i32 CMapArrayA::Allocate(u32 count) {
    m_storage = new BrickzNode[count];
    if (m_storage == NULL) {
        return 0;
    }

    m_freeList = m_storage;
    BrickzNode* e = m_storage;
    m_count = count;
    e->m_openPrev = NULL;

    BrickzNode* next = e + 1;
    for (u32 i = 0; i < m_count; ++i) {
        if (e == m_freeList) {
            e->m_openPrev = NULL;
        } else {
            e->m_openPrev = e - 1;
        }
        e->m_openNext = next;
        ++e;
        ++next;
    }
    m_freeList[m_count - 1].m_openNext = NULL;
    return 1;
}

RVA(0x0009e7e0, 0x29)
CMapArrayA::~CMapArrayA() {
    if (m_storage) {
        delete[] m_storage;
    }
    m_storage = NULL;
    m_freeList = NULL;
    m_count = 0;
}

RVA(0x0009e820, 0xd)
CMapArrayB::CMapArrayB() {
    m_storage = NULL;
    m_freeList = NULL;
    m_count = 0;
}

// @early-stop
RVA(0x0009e860, 0x7a)
i32 CMapArrayB::Allocate(u32 count) {
    m_storage = new BrickzCellNode[count];
    if (m_storage == NULL) {
        return 0;
    }

    m_freeList = m_storage;
    BrickzCellNode* e = m_storage;
    m_count = count;
    e->m_cellPrev = NULL;

    BrickzCellNode* next = e + 1;
    for (u32 i = 0; i < m_count; ++i) {
        if (e == m_freeList) {
            e->m_cellPrev = NULL;
        } else {
            e->m_cellPrev = e - 1;
        }
        e->m_searchNode = NULL;
        e->m_cellNext = next;
        ++e;
        ++next;
    }
    m_freeList[m_count - 1].m_cellNext = NULL;
    return 1;
}

RVA(0x0009e900, 0x28)
CMapArrayB::~CMapArrayB() {
    if (m_storage) {
        delete[] m_storage;
    }
    m_storage = NULL;
    m_freeList = NULL;
    m_count = 0;
}

RVA(0x0009e940, 0x73)
CMapMgr::CMapMgr() {
    m_cellPool = NULL;
    m_rows = NULL;
    m_width = 0;
    m_height = 0;
    m_openList = NULL;
    m_reserved1c = 0;
    m_edgeMask = 0;
    m_maskB = 0;
    m_maskA = -1;
    m_dirty = 1;
}

RVA(0x0009e9e0, 0x5d)
CMapMgr::~CMapMgr() {
    Reset();
}

// @early-stop
RVA(0x0009ea60, 0x168)
i32 CMapMgr::AllocGrid(i32 width, i32 height, void (*callback)()) {
    i32 count = height * width;
    m_width = width;
    m_height = height;
    m_cellCount = count;
    m_cellPool = new BrickzCell[count];
    if (m_cellPool == NULL) {
        return 0;
    }
    m_rows = new BrickzCell*[height];
    if (m_rows == NULL) {
        return 0;
    }
    memset(m_cellPool, 0, count * 0x1c);
    i32 stride = width;
    i32 off = 0;

    for (u32 i = 0; i < static_cast<u32>(height); i++) {
        m_rows[i] = m_cellPool + off;
        off += stride;
    }
    if (m_colA.Allocate(count * 5) == 0) {
        return 0;
    }
    if (m_colB.Allocate(count * 5) == 0) {
        return 0;
    }
    m_stepCb = callback;

    RECT a;
    RECT b;
    a.left = 0;
    a.top = 0;
    a.right = width;
    a.bottom = height;
    b.left = 0;
    b.top = 0;
    b.right = width;
    b.bottom = height;
    RECT* out = &m_bounds;
    if (!IntersectRect(out, &a, &b)) {
        *out = a;
    }
    m_gridW = out->right - out->left;
    m_gridH = out->bottom - out->top;
    return 1;
}

RVA(0x0009ec30, 0x4b)
void CMapMgr::Reset() {
    if (m_cellPool) {
        delete[] m_cellPool;
    }
    if (m_rows) {
        delete[] m_rows;
    }

    m_colA.~CMapArrayA();
    m_colB.~CMapArrayB();

    m_cellPool = NULL;
    m_rows = NULL;
    m_width = 0;
    m_height = 0;
    m_openList = NULL;
    m_reserved1c = 0;
}

// @early-stop
RVA(0x0009eca0, 0x2bd)
i32 CMapMgr::Search(i32 x1, i32 y1, i32 x2, i32 y2, void* list, i32 maskA, i32 maskB, i32 maskC) {
    i32 ox = m_bounds.left;
    if (static_cast<u32>((x1 - ox)) >= static_cast<u32>(m_gridW)) {
        return 0;
    }
    i32 oy = m_bounds.top;
    i32 hgt = m_gridH;
    if (static_cast<u32>((y1 - oy)) >= static_cast<u32>(hgt)) {
        return 0;
    }
    if (static_cast<u32>((x2 - ox)) >= static_cast<u32>(m_gridW)) {
        return 0;
    }
    if (static_cast<u32>((y2 - oy)) >= static_cast<u32>(hgt)) {
        return 0;
    }
    m_maskC = maskC;
    m_maskB = maskB;
    m_maskA = maskA;
    i32 flags = m_rows[y2][x2].m_flags;
    // Retail 0x9ed26: `test ecx,eax` (maskA & flags) `je` continue, then
    // `mov ecx,[esp+0x30]` (maskC) `test ecx,eax` `je 0x9eedc` -> return 0.  The
    // SECOND test bails when maskC does NOT hit, i.e. the goal is blocked by maskA
    // and the passable-override does not clear it - the same polarity CMapMgr::Expand
    // uses at 0x9f0a2 for a neighbour.
    if ((maskA & flags) != 0 && (maskC & flags) == 0) {
        return 0;
    }

    // Retail rotates a `for` (`cmp ecx,edx` against the zeroed i, `jbe`), not a
    // `!= 0` guard over a do-while (which lowers to `test`/`jne`).
    for (u32 i = 0; i < m_cellCount; i++) {
        m_cellPool[i].m_count = 0;
    }
    if (x1 == x2 && y1 == y2) {
        return 1;
    }
    m_goal.m_x = x2;
    m_start.m_x = x1;
    m_goal.m_y = y2;
    m_start.m_y = y1;

    // Retail 0x9ed7d nulls the SEED, not the free list: `mov ecx,[esi+0x30]` /
    // `mov eax,[ecx+0x14]` / `cmp eax,edx` / `jne` -> `xor ecx,ecx` (seed = NULL).
    // The free-list head is only rewritten on the taken path (0x9ed8b).  That keeps
    // the pool's last node as a permanent SENTINEL - the same invariant Expand
    // (0x9f2b6) and CellPush (0x9f48d) enforce.  Nulling m_freeList instead left
    // `seed` non-NULL, so the `seed == NULL` bail became dead: Search ran on the
    // sentinel while m_colA.m_freeList was NULL, and the next Expand / Drain /
    // ResetCells dereferenced that NULL head.
    BrickzNode* seed = m_colA.m_freeList;
    BrickzNode* slot = seed->m_openNext;
    if (slot == NULL) {
        seed = NULL;
    } else {
        m_colA.m_freeList = slot;
        slot->m_openPrev = NULL;
    }
    if (seed == NULL) {
        return 0;
    }
    seed->m_col = x1;
    seed->m_row = y1;
    seed->m_gCost = 0;
    i32 dx = abs(m_goal.m_y - y1);
    i32 dxx = abs(m_goal.m_x - x1);
    i32 h = (dx + dxx) * 2;
    seed->m_hCost = h;
    seed->m_fCost = h;
    seed->m_openNext = NULL;
    seed->m_openPrev = NULL;
    seed->m_parent = NULL;
    Insert(seed);
    (&m_rows[y1][x1])->m_count++;
    BrickzNode* node = 0;
    while (m_openList != NULL) {
        node = PopFront();
        (&m_rows[node->m_row][node->m_col])->m_count--;
        if (node->m_col == m_goal.m_x && node->m_row == m_goal.m_y) {
            goto reached;
        }
        Expand(node, 0, 1, 2, 0);
        Expand(node, 1, 0, 2, 0);
        Expand(node, 0, -1, 2, 0);
        Expand(node, -1, 0, 2, 0);
        Expand(node, 1, 1, 3, 1);
        Expand(node, 1, -1, 3, 1);
        Expand(node, -1, -1, 3, 1);
        Expand(node, -1, 1, 3, 1);
        CellPush(node);
    }
    node = NULL;
    Drain();
    ResetCells();
    if (m_stepCb != NULL) {
        m_stepCb();
    }
    return 0;

reached:
    BrickzNode* p = node;
    // `while`, not `do-while`: retail guards the loop with its own entry test
    // (block B25 at the `reached:` label carries a jcc past the body).
    while (p != NULL) {
        CoordPoolNode* rec = g_coordPool.m_freeHead;
        Coord* slot = 0;
        if (rec->m_next != NULL) {
            slot = &rec->m_coord;
            rec->m_coord.m_x = p->m_col;
            rec->m_coord.m_y = p->m_row;
            g_coordPool.m_freeHead = rec->m_next;
        }

        static_cast<CPtrList*>(list)->AddHead(slot);
        p = p->m_parent;
    }
    if (m_stepCb != NULL) {
        m_stepCb();
    }
    node->m_openPrev = NULL;
    node->m_openNext = m_colA.m_freeList;
    m_colA.m_freeList->m_openPrev = node;
    m_colA.m_freeList = node;
    Drain();
    ResetCells();
    return 1;
}

// @early-stop
RVA(0x0009f010, 0x2a1)
i32 CMapMgr::Expand(BrickzNode* node, i32 dx, i32 dy, i32 cost, i32 diag) {
    i32 ng = node->m_gCost + cost;
    i32 ncol = node->m_col + dx;
    i32 nrow = node->m_row + dy;
    BrickzNode* found0 = 0;
    if (static_cast<u32>((ncol - m_bounds.left)) >= static_cast<u32>(m_gridW)) {
        return 1;
    }
    if (static_cast<u32>((nrow - m_bounds.top)) >= static_cast<u32>(m_gridH)) {
        return 1;
    }
    BrickzCell* ncell = &m_rows[nrow][ncol];
    i32 nflags = ncell->m_flags;
    BrickzCell* cell = &m_rows[node->m_row][node->m_col];
    if ((m_edgeMask & nflags) != 0) {
        return 1;
    }
    if ((m_maskA & nflags) != 0 && (m_maskC & nflags) == 0) {
        return 1;
    }
    if (diag != 0 && m_maskB != 0) {
        BrickzCell *cellA, *cellB;
        if (dx > 0 && dy > 0) {
            cellB = cell + m_width;
            cellA = cell + 1;
        } else if (dx < 0 && dy > 0) {
            cellB = cell + m_width;
            cellA = cell - 1;
        } else if (dx > 0 && dy < 0) {
            cellB = cell - m_width;
            cellA = cell + 1;
        } else if (dx < 0 && dy < 0) {
            cellB = cell - m_width;
            cellA = cell - 1;
        } else {
            goto relax;
        }
        if ((m_maskB & cellA->m_flags) != 0 || (m_maskB & cellB->m_flags) != 0) {
            return 1;
        }
    }
relax:
    BrickzNode* closed = 0;
    BrickzCellNode* head = ncell->m_head;
    if (head != NULL) {
        closed = head->m_searchNode;
    }
    if (closed != NULL) {
        if (ng >= closed->m_gCost) {
            return 1;
        }
    }
    BrickzNode* open;
    if (ncell->m_count != 0) {
        open = Find(ncol, nrow);
    } else {
        open = found0;
    }
    if (open != NULL) {
        i32 og = open->m_gCost;
        if (ng >= og) {
            return 1;
        }
        if (open != NULL && ng < og) {
            if (closed != NULL) {
                CellPop(closed, 1);
            }
            Unlink(open);
            open->m_fCost = ng + open->m_hCost;
            open->m_parent = node;
            open->m_gCost = ng;
            Insert(open);
            return 1;
        }
    }
    // Two separate guards, not one nested block: retail tests `closed` at 0x9f1ed
    // (`test edi,edi / je 0x9f223`), again after the cost compare falls through
    // (0x9f215), so the CellPop(closed, 1) arm carries its OWN null test.
    if (closed != NULL && ng < closed->m_gCost) {
        CellPop(closed, 0);
        closed->m_parent = node;
        closed->m_gCost = ng;
        closed->m_fCost = closed->m_hCost + ng;
        Insert(closed);
        ncell->m_count++;
        return 1;
    }
    if (closed != NULL) {
        CellPop(closed, 1);
    }
    if (open != NULL) {
        return 1;
    }
    BrickzNode* rec = m_colA.m_freeList;
    BrickzNode* nx = rec->m_openNext;
    if (nx == NULL) {
        rec = NULL;
    } else {
        m_colA.m_freeList = nx;
        nx->m_openPrev = NULL;
    }
    if (rec == NULL) {
        return 0;
    }
    rec->m_col = ncol;
    rec->m_row = nrow;
    rec->m_gCost = ng;
    i32 hy = abs(m_goal.m_y - nrow);
    i32 hx = abs(m_goal.m_x - ncol);
    i32 h = (hy + hx) * 2;
    rec->m_parent = node;
    rec->m_hCost = h;
    rec->m_fCost = ng + h;
    rec->m_openNext = NULL;
    rec->m_openPrev = NULL;
    rec->m_cellLink = NULL;
    Insert(rec);
    ncell->m_count++;
    return 1;
}

RVA(0x0009f370, 0x8a)
i32 CMapMgr::Insert(BrickzNode* node) {
    BrickzNode* cur = m_openList;
    node->m_openPrev = NULL;
    node->m_openNext = NULL;
    if (cur == NULL) {
        m_openList = node;
        return 1;
    }
    i32 key = node->m_fCost;
    while (cur != NULL) {
        if (key < cur->m_fCost) {
            if (cur->m_openPrev != NULL) {
                node->m_openPrev = cur->m_openPrev;
                node->m_openNext = cur;
                cur->m_openPrev->m_openNext = node;
                cur->m_openPrev = node;
            } else {
                m_openList = node;
                node->m_openNext = cur;
                cur->m_openPrev = node;
            }
            return 1;
        }
        if (cur->m_openNext == NULL) {
            cur->m_openNext = node;
            node->m_openPrev = cur;
            return 1;
        }
        cur = cur->m_openNext;
    }
    return 1;
}

RVA(0x0009f430, 0x2a)
BrickzNode* CMapMgr::PopFront() {
    BrickzNode* head = m_openList;
    if (head != NULL) {
        BrickzNode* next = head->m_openNext;
        if (next != NULL) {
            m_openList = next;
            next->m_openPrev = NULL;
        } else {
            m_openList = NULL;
        }
        head->m_openNext = NULL;
        head->m_openPrev = NULL;
    }
    return head;
}

// @early-stop
RVA(0x0009f470, 0x62)
void CMapMgr::CellPush(BrickzNode* node) {
    BrickzCellNode** head = &m_rows[node->m_row][node->m_col].m_head;
    BrickzCellNode* slot = m_colB.m_freeList;
    BrickzCellNode* nx = slot->m_cellNext;
    if (nx == NULL) {
        slot = NULL;
    } else {
        m_colB.m_freeList = nx;
        nx->m_cellPrev = NULL;
    }
    BrickzCellNode* old = *head;
    if (old == NULL) {
        *head = slot;
        slot->m_cellPrev = NULL;
        slot->m_cellNext = NULL;
        slot->m_searchNode = node;
        node->m_cellLink = slot;
    } else {
        slot->m_cellPrev = old;
        slot->m_cellNext = (*head)->m_cellNext;
        *head = slot;
        node->m_cellLink = slot;
    }
}

RVA(0x0009f500, 0x24)
BrickzNode* CMapMgr::Find(i32 key1, i32 key2) {
    BrickzNode* p = m_openList;
    if (p == NULL) {
        return 0;
    }
    do {
        if (p->m_col == key1 && p->m_row == key2) {
            return p;
        }
        p = p->m_openNext;
    } while (p != NULL);
    return 0;
}

RVA(0x0009f540, 0x40)
BrickzNode* CMapMgr::FindCellNode(i32 col, i32 row) {
    BrickzCellNode* n = m_rows[row][col].m_head;
    while (n != NULL) {
        BrickzNode* child = n->m_searchNode;
        if (child->m_col == col && child->m_row == row) {
            return n->m_searchNode;
        }
        n = n->m_cellNext;
    }
    return 0;
}

RVA(0x0009f590, 0x2f)
void CMapMgr::Drain() {
    BrickzNode* p = m_openList;
    if (p != NULL) {
        do {
            BrickzNode* cur = p;
            p = cur->m_openNext;
            cur->m_openNext = m_colA.m_freeList;
            cur->m_openPrev = NULL;
            m_colA.m_freeList->m_openPrev = cur;
            m_colA.m_freeList = cur;
        } while (p != NULL);
    }
    m_openList = NULL;
}

// @early-stop
RVA(0x0009f5d0, 0x81)
void CMapMgr::ResetCells() {
    BrickzCell* cell = m_cellPool;
    for (u32 i = 0; i < m_height * m_width; i++) {
        BrickzCellNode* node = cell->m_head;
        while (node != NULL) {
            BrickzCellNode* cur = node;
            BrickzCellNode** link = &cur->m_cellNext;
            node = *link;
            BrickzNode* child = cur->m_searchNode;
            child->m_openNext = m_colA.m_freeList;
            child->m_openPrev = NULL;
            m_colA.m_freeList->m_openPrev = child;
            m_colA.m_freeList = child;
            cur->m_cellPrev = NULL;
            *link = m_colB.m_freeList;
            m_colB.m_freeList->m_cellPrev = cur;
            m_colB.m_freeList = cur;
        }
        cell->m_head = NULL;
        cell++;
    }
}

RVA(0x0009f690, 0x5d)
void CMapMgr::Unlink(BrickzNode* node) {
    if (node->m_openPrev != NULL && node->m_openNext != NULL) {
        node->m_openPrev->m_openNext = node->m_openNext;
        node->m_openNext->m_openPrev = node->m_openPrev;
    } else if (node->m_openPrev == NULL && node->m_openNext == NULL) {
        m_openList = NULL;
    } else if (node->m_openPrev == NULL && node->m_openNext != NULL) {
        BrickzNode* next = node->m_openNext;
        m_openList = next;
        next->m_openPrev = NULL;
    }
    if (node->m_openPrev != NULL && node->m_openNext == NULL) {
        node->m_openPrev->m_openNext = NULL;
    }
    node->m_openPrev = NULL;
    node->m_openNext = NULL;
}

// @early-stop
RVA(0x0009f710, 0xa7)
void CMapMgr::CellPop(BrickzNode* node, i32 flag) {
    BrickzCellNode** head = &m_rows[node->m_row][node->m_col].m_head;
    BrickzCellNode* slot = node->m_cellLink;
    // The same three-arm `&&` chain CMapMgr::Unlink uses, but with the arms in retail's
    // emission order: the FIRST body retail lays down is `*head = NULL` (0x9f73e), then
    // the two-sided unlink (0x9f74e), then the head-advance (0x9f761).
    if (slot->m_cellPrev == NULL && slot->m_cellNext == NULL) {
        *head = NULL;
    } else if (slot->m_cellPrev != NULL && slot->m_cellNext != NULL) {
        slot->m_cellPrev->m_cellNext = slot->m_cellNext;
        slot->m_cellNext->m_cellPrev = slot->m_cellPrev;
    } else if (slot->m_cellPrev == NULL) {
        BrickzCellNode* next = slot->m_cellNext;
        if (next != NULL) {
            *head = next;
            next->m_cellPrev = NULL;
        }
    }
    if (slot->m_cellPrev != NULL && slot->m_cellNext == NULL) {
        slot->m_cellPrev->m_cellNext = NULL;
    }
    node->m_openPrev = NULL;
    node->m_openNext = NULL;
    node->m_cellLink = NULL;
    slot->m_cellNext = m_colB.m_freeList;
    slot->m_cellPrev = NULL;
    m_colB.m_freeList->m_cellPrev = slot;
    m_colB.m_freeList = slot;
    if (flag != 0) {
        node->m_openPrev = NULL;
        node->m_openNext = m_colA.m_freeList;
        m_colA.m_freeList->m_openPrev = node;
        m_colA.m_freeList = node;
    }
}

RVA(0x0009f7f0, 0x3b)
i32 CMapMgr::Visit(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, i32 pObj) {
    if (ar == NULL) {
        return 0;
    }
    switch (mode) {
        case SERIAL_SAVE:
            if (Save(ar) == 0) {
                return 0;
            }
            break;
        case SERIAL_LOAD:
            if (Load(ar) == 0) {
                return 0;
            }
            break;
    }
    return 1;
}

RVA(0x0009f840, 0x110)
i32 CMapMgr::Save(CFileMemBase* ar) {
    if (ar == NULL) {
        return 0;
    }
    ar->Write(&m_width, sizeof(m_width));
    ar->Write(&m_height, sizeof(m_height));
    ar->Write(&m_cellCount, sizeof(m_cellCount));
    ar->Write(&m_start, sizeof(m_start));
    ar->Write(&m_goal, sizeof(m_goal));
    ar->Write(&m_maskA, sizeof(m_maskA));
    ar->Write(&m_maskC, sizeof(m_maskC));
    ar->Write(&m_maskB, sizeof(m_maskB));
    ar->Write(&m_dirty, sizeof(m_dirty));
    ar->Write(&m_bounds.left, 0x10);
    ar->Write(&m_gridW, sizeof(m_gridW));
    ar->Write(&m_gridH, sizeof(m_gridH));
    for (u32 i = 0; i < m_width; i++) {
        for (u32 j = 0; j < m_height; j++) {
            ar->Write(&m_cellPool[j * m_width + i], sizeof(m_cellPool[j * m_width + i]));
        }
    }
    return 1;
}

RVA(0x0009f9a0, 0x12e)
i32 CMapMgr::Load(CFileMemBase* ar) {
    if (ar == NULL) {
        return 0;
    }
    ar->Read(&m_width, sizeof(m_width));
    ar->Read(&m_height, sizeof(m_height));
    ar->Read(&m_cellCount, sizeof(m_cellCount));
    ar->Read(&m_start, sizeof(m_start));
    ar->Read(&m_goal, sizeof(m_goal));
    ar->Read(&m_maskA, sizeof(m_maskA));
    ar->Read(&m_maskC, sizeof(m_maskC));
    ar->Read(&m_maskB, sizeof(m_maskB));
    ar->Read(&m_dirty, sizeof(m_dirty));
    ar->Read(&m_bounds.left, 0x10);
    ar->Read(&m_gridW, sizeof(m_gridW));
    ar->Read(&m_gridH, sizeof(m_gridH));
    for (u32 i = 0; i < m_width; i++) {
        for (u32 j = 0; j < m_height; j++) {
            ar->Read(&m_cellPool[j * m_width + i], sizeof(m_cellPool[j * m_width + i]));
            m_cellPool[j * m_width + i].m_head = NULL;
        }
    }
    return 1;
}

// @identity-TODO SetVersionRect - thunk oracle: retail gave this NO incremental
// thunk, so it came from the static LIBRARY, while the rest of this TU
// (24 fns) was a link-line object. It belongs to another compiland.
RVA(0x0009fe10, 0x29)
void SetVersionRect() {
    g_versionRect.left = 5;
    g_versionRect.top = 0x1c5;
    g_versionRect.right = 0x27b;
    g_versionRect.bottom = 0x1de;
}
