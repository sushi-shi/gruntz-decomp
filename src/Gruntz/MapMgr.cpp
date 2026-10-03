#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/MapMgr.h>

#include <Globals.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/CoordPool.h>
#include <Gruntz/GameMode.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/MapCellInline.h>
#include <Gruntz/MapClipInline.h>
#include <Gruntz/SerialArchive.h>
#include <Io/FileMem.h>
#include <RectMacros.h>

#include <stdlib.h>
#include <string.h>

CBrickzNodePool::CBrickzNodePool() {
    RESET_MAP_ARRAY_STORAGE;
}

CBrickzNodePool::~CBrickzNodePool() {
    Free();
}

i32 CBrickzNodePool::Allocate(u32 count) {
    m_storage = new BrickzNode[count];
    if (m_storage == NULL) {
        return 0;
    }

    m_freeList = m_storage;
    BrickzNode* e = m_storage;
    m_count = count;
    e->m_openPrev = NULL;

    u32 i = 0;
    if (i < m_count) {
        BrickzNode* next = e + 1;
        do {
            if (e == m_freeList) {
                e->m_openPrev = NULL;
            } else {
                e->m_openPrev = e - 1;
            }
            e->m_openNext = next;
            ++e;
            ++next;
            ++i;
        } while (i < m_count);
    }
    m_freeList[m_count - 1].m_openNext = NULL;
    return 1;
}

void CBrickzNodePool::Free() {
    if (m_storage) {
        delete[] m_storage;
    }
    RESET_MAP_ARRAY_STORAGE;
}

CBrickzCellNodePool::CBrickzCellNodePool() {
    RESET_MAP_ARRAY_STORAGE;
}

CBrickzCellNodePool::~CBrickzCellNodePool() {
    Free();
}

i32 CBrickzCellNodePool::Allocate(u32 count) {
    m_storage = new BrickzCellNode[count];
    if (m_storage == NULL) {
        return 0;
    }

    m_freeList = m_storage;
    BrickzCellNode* e = m_storage;
    m_count = count;
    e->m_cellPrev = NULL;

    u32 i = 0;
    if (i < m_count) {
        BrickzCellNode* next = e + 1;
        do {
            if (e == m_freeList) {
                e->m_cellPrev = NULL;
            } else {
                e->m_cellPrev = e - 1;
            }
            e->m_searchNode = NULL;
            e->m_cellNext = next;
            ++e;
            ++next;
            ++i;
        } while (i < m_count);
    }
    m_freeList[m_count - 1].m_cellNext = NULL;
    return 1;
}

void CBrickzCellNodePool::Free() {
    if (m_storage) {
        delete[] m_storage;
    }
    RESET_MAP_ARRAY_STORAGE;
}

CMapMgr::CMapMgr() {
    m_cellPool = NULL;
    m_rows = NULL;
    m_width = 0;
    m_height = 0;
    m_openList = NULL;
    m_reserved1c = 0;
    m_edgeMask = 0;
    m_diagonalMask = 0;
    m_blockedMask = -1;
    m_dirty = true;
}

CMapMgr::~CMapMgr() {
    Reset();
}

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
    for (u32 i = 0; i < static_cast<u32>(height); i++) {
        m_rows[i] = &m_cellPool[i * width];
    }
    if (m_nodePool.Allocate(count * 5) == 0) {
        return 0;
    }
    if (m_cellNodePool.Allocate(count * 5) == 0) {
        return 0;
    }
    m_stepCb = callback;

    Clip(NULL);
    return 1;
}

void CMapMgr::Reset() {
    if (m_cellPool) {
        delete[] m_cellPool;
    }
    if (m_rows) {
        delete[] m_rows;
    }

    m_nodePool.Free();
    m_cellNodePool.Free();

    m_cellPool = NULL;
    m_rows = NULL;
    m_width = 0;
    m_height = 0;
    m_openList = NULL;
    m_reserved1c = 0;
}

i32 CMapMgr::FindPath(
    i32 startX,
    i32 startY,
    i32 goalX,
    i32 goalY,
    CPtrList* outPath,
    i32 blockedMask,
    i32 diagonalMask,
    i32 passableMask
) {
    if (!InSearchBounds(startX, startY)) {
        return 0;
    }
    if (!InSearchBounds(goalX, goalY)) {
        return 0;
    }
    m_passableMask = passableMask;
    m_diagonalMask = diagonalMask;
    m_blockedMask = blockedMask;
    i32 goalFlags = m_rows[goalY][goalX].m_flags;
    if ((blockedMask & goalFlags) != 0 && (passableMask & goalFlags) == 0) {
        return 0;
    }

    for (u32 i = 0; i < m_cellCount; i++) {
        m_cellPool[i].m_count = 0;
    }
    if (startX == goalX && startY == goalY) {
        return 1;
    }
    m_goal.m_x = goalX;
    m_start.m_x = startX;
    m_goal.m_y = goalY;
    m_start.m_y = startY;

    BrickzNode* seed = m_nodePool.Pop();
    if (seed == NULL) {
        return 0;
    }
    seed->m_col = startX;
    seed->m_row = startY;
    seed->m_gCost = 0;
    i32 deltaY = abs(m_goal.m_y - startY);
    i32 deltaX = abs(m_goal.m_x - startX);
    i32 h = (deltaY + deltaX) * 2;
    seed->m_hCost = h;
    seed->m_fCost = h;
    seed->m_openNext = NULL;
    seed->m_openPrev = NULL;
    seed->m_parent = NULL;
    InsertOpenNode(seed);
    (&m_rows[startY][startX])->m_count++;
    BrickzNode* node = NULL;
    while (m_openList != NULL) {
        node = PopBestOpenNode();
        BrickzCell* cell = &m_rows[node->m_row][node->m_col];
        cell->m_count--;
        if (node->m_col == m_goal.m_x && node->m_row == m_goal.m_y) {
            goto reached;
        }
        ExpandNeighbor(node, 0, 1, 2, 0);
        ExpandNeighbor(node, 1, 0, 2, 0);
        ExpandNeighbor(node, 0, -1, 2, 0);
        ExpandNeighbor(node, -1, 0, 2, 0);
        ExpandNeighbor(node, 1, 1, 3, 1);
        ExpandNeighbor(node, 1, -1, 3, 1);
        ExpandNeighbor(node, -1, -1, 3, 1);
        ExpandNeighbor(node, -1, 1, 3, 1);
        LinkClosedNode(node);
    }
    node = NULL;
    RecycleOpenNodes();
    RecycleClosedNodes();
    if (m_stepCb != NULL) {
        m_stepCb();
    }
    return 0;

reached:
    BrickzNode* p = node;
    while (p != NULL) {
        Coord position;
        Coord* slot = g_coordPool.PopCopy(*position.Set(p->m_col, p->m_row));

        outPath->AddHead(slot);
        p = p->m_parent;
    }
    if (m_stepCb != NULL) {
        m_stepCb();
    }
    m_nodePool.Push(node);
    RecycleOpenNodes();
    RecycleClosedNodes();
    return 1;
}

i32 CMapMgr::ExpandNeighbor(BrickzNode* node, i32 dx, i32 dy, i32 cost, i32 diagonal) {
    i32 ng = node->m_gCost + cost;
    i32 ncol = node->m_col + dx;
    i32 nrow = node->m_row + dy;
    if (!InSearchBounds(ncol, nrow)) {
        return 1;
    }
    BrickzCell* ncell = &m_rows[nrow][ncol];
    i32 nflags = ncell->m_flags;
    BrickzCell* cell = &m_rows[node->m_row][node->m_col];
    if ((m_edgeMask & nflags) != 0) {
        return 1;
    }
    if ((m_blockedMask & nflags) != 0 && (m_passableMask & nflags) == 0) {
        return 1;
    }
    if (diagonal != 0 && m_diagonalMask != 0) {
        BrickzCell *horizontalNeighbor, *verticalNeighbor;
        if (dx > 0 && dy > 0) {
            verticalNeighbor = cell + m_width;
            horizontalNeighbor = cell + 1;
        } else if (dx < 0 && dy > 0) {
            verticalNeighbor = cell + m_width;
            horizontalNeighbor = cell - 1;
        } else if (dx > 0 && dy < 0) {
            verticalNeighbor = cell - m_width;
            horizontalNeighbor = cell + 1;
        } else if (dx < 0 && dy < 0) {
            verticalNeighbor = cell - m_width;
            horizontalNeighbor = cell - 1;
        } else {
            goto relax;
        }
        if ((m_diagonalMask & horizontalNeighbor->m_flags) != 0
            || (m_diagonalMask & verticalNeighbor->m_flags) != 0) {
            return 1;
        }
    }
relax:
    BrickzNode* closed = NULL;
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
        open = FindOpenNode(ncol, nrow);
    } else {
        open = NULL;
    }
    if (open != NULL && ng >= open->m_gCost) {
        return 1;
    }
    if (open != NULL && ng < open->m_gCost) {
        if (closed != NULL) {
            UnlinkClosedNode(closed, 1);
        }
        UnlinkOpenNode(open);
        open->m_fCost = ng + open->m_hCost;
        open->m_parent = node;
        open->m_gCost = ng;
        InsertOpenNode(open);
        return 1;
    }
    if (closed != NULL && ng < closed->m_gCost) {
        UnlinkClosedNode(closed, 0);
        closed->m_parent = node;
        closed->m_gCost = ng;
        closed->m_fCost = closed->m_hCost + ng;
        InsertOpenNode(closed);
        ncell->m_count++;
        return 1;
    }
    if (closed != NULL) {
        UnlinkClosedNode(closed, 1);
    }
    if (open != NULL) {
        return 1;
    }
    BrickzNode* rec = m_nodePool.Pop();
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
    InsertOpenNode(rec);
    ncell->m_count++;
    return 1;
}

i32 CMapMgr::InsertOpenNode(BrickzNode* node) {
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

BrickzNode* CMapMgr::PopBestOpenNode() {
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

void CMapMgr::LinkClosedNode(BrickzNode* node) {
    BrickzCellNode** head = &m_rows[node->m_row][node->m_col].m_head;
    BrickzCellNode* slot = m_cellNodePool.Pop();
    BrickzCellNode* old = *head;
    if (old == NULL) {
        *head = slot;
        slot->m_cellPrev = NULL;
        slot->m_cellNext = NULL;
        slot->m_searchNode = node;
    } else {
        slot->m_cellPrev = old;
        slot->m_cellNext = (*head)->m_cellNext;
        *head = slot;
    }
    node->m_cellLink = slot;
}

BrickzNode* CMapMgr::FindOpenNode(i32 col, i32 row) {
    BrickzNode* p = m_openList;
    if (p == NULL) {
        return NULL;
    }
    do {
        if (p->m_col == col && p->m_row == row) {
            return p;
        }
        p = p->m_openNext;
    } while (p != NULL);
    return NULL;
}

BrickzNode* CMapMgr::FindClosedNode(i32 col, i32 row) {
    BrickzCellNode* n = m_rows[row][col].m_head;
    while (n != NULL) {
        BrickzNode* child = n->m_searchNode;
        if (child->m_col == col && child->m_row == row) {
            return n->m_searchNode;
        }
        n = n->m_cellNext;
    }
    return NULL;
}

void CMapMgr::RecycleOpenNodes() {
    BrickzNode* p = m_openList;
    if (p != NULL) {
        do {
            BrickzNode* cur = p;
            p = cur->m_openNext;
            m_nodePool.Push(cur);
        } while (p != NULL);
    }
    m_openList = NULL;
}

void CMapMgr::RecycleClosedNodes() {
    BrickzCell* cell = m_cellPool;
    for (u32 i = 0; i < m_width * m_height; i++) {
        BrickzCellNode* node = cell->m_head;
        while (node != NULL) {
            BrickzCellNode* cur = node;
            BrickzCellNode** link = &cur->m_cellNext;
            node = *link;
            BrickzNode* child = cur->m_searchNode;
            m_nodePool.Push(child);
            m_cellNodePool.Push(cur);
        }
        cell->m_head = NULL;
        cell++;
    }
}

void CMapMgr::UnlinkOpenNode(BrickzNode* node) {
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

void CMapMgr::UnlinkClosedNode(BrickzNode* node, i32 recycleSearchNode) {
    BrickzCellNode** head = &m_rows[node->m_row][node->m_col].m_head;
    BrickzCellNode* slot = node->m_cellLink;
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
    m_cellNodePool.Push(slot);
    if (recycleSearchNode != 0) {
        m_nodePool.Push(node);
    }
}

i32 CMapMgr::SerializeDispatch(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, i32 payload) {
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

i32 CMapMgr::Save(CFileMemBase* ar) {
    if (ar == NULL) {
        return 0;
    }
    ar->Write(&m_width, sizeof(m_width));
    ar->Write(&m_height, sizeof(m_height));
    ar->Write(&m_cellCount, sizeof(m_cellCount));
    ar->Write(&m_start, sizeof(m_start));
    ar->Write(&m_goal, sizeof(m_goal));
    ar->Write(&m_blockedMask, sizeof(m_blockedMask));
    ar->Write(&m_passableMask, sizeof(m_passableMask));
    ar->Write(&m_diagonalMask, sizeof(m_diagonalMask));
    ar->Write(&m_dirty, sizeof(m_dirty));
    ar->Write(&m_bounds.left, sizeof(m_bounds));
    ar->Write(&m_gridW, sizeof(m_gridW));
    ar->Write(&m_gridH, sizeof(m_gridH));
    for (u32 i = 0; i < m_width; i++) {
        for (u32 j = 0; j < m_height; j++) {
            ar->Write(&m_cellPool[j * m_width + i], sizeof(m_cellPool[j * m_width + i]));
        }
    }
    return 1;
}

i32 CMapMgr::Load(CFileMemBase* ar) {
    if (ar == NULL) {
        return 0;
    }
    ar->Read(&m_width, sizeof(m_width));
    ar->Read(&m_height, sizeof(m_height));
    ar->Read(&m_cellCount, sizeof(m_cellCount));
    ar->Read(&m_start, sizeof(m_start));
    ar->Read(&m_goal, sizeof(m_goal));
    ar->Read(&m_blockedMask, sizeof(m_blockedMask));
    ar->Read(&m_passableMask, sizeof(m_passableMask));
    ar->Read(&m_diagonalMask, sizeof(m_diagonalMask));
    ar->Read(&m_dirty, sizeof(m_dirty));
    ar->Read(&m_bounds.left, sizeof(m_bounds));
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

CRect g_versionRect(5, 453, 635, 478);
