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
#include <MakeRect.h>

#include <stdlib.h>
#include <string.h>

RVA_DYNINIT(0x0009fb20, 0x5, s_gruntDirNorth)
RVA_DYNINIT(0x0009fb40, 0x1a, s_gruntDirNorth)
RVA_DYNINIT(0x0009fb70, 0x5, s_gruntDirNorthEast)
RVA_DYNINIT(0x0009fb90, 0x1a, s_gruntDirNorthEast)
RVA_DYNINIT(0x0009fbc0, 0x5, s_gruntDirEast)
RVA_DYNINIT(0x0009fbe0, 0x1f, s_gruntDirEast)
RVA_DYNINIT(0x0009fc10, 0x5, s_gruntDirSouthEast)
RVA_DYNINIT(0x0009fc30, 0x1a, s_gruntDirSouthEast)
RVA_DYNINIT(0x0009fc60, 0x5, s_gruntDirSouth)
RVA_DYNINIT(0x0009fc80, 0x1f, s_gruntDirSouth)
RVA_DYNINIT(0x0009fcb0, 0x5, s_gruntDirSouthWest)
RVA_DYNINIT(0x0009fcd0, 0x1f, s_gruntDirSouthWest)
RVA_DYNINIT(0x0009fd00, 0x5, s_gruntDirWest)
RVA_DYNINIT(0x0009fd20, 0x1f, s_gruntDirWest)
RVA_DYNINIT(0x0009fd50, 0x5, s_gruntDirNorthWest)
RVA_DYNINIT(0x0009fd70, 0x17, s_gruntDirNorthWest)
RVA_DYNINIT(0x0009fda0, 0x5, s_gruntDirCenter)
RVA_DYNINIT(0x0009fdc0, 0x1a, s_gruntDirCenter)

#define RESET_MAP_ARRAY_STORAGE                                                                    \
    m_storage = NULL;                                                                              \
    m_freeList = NULL;                                                                             \
    m_count = 0

RVA(0x0009e700, 0xd)
CBrickzNodePool::CBrickzNodePool() {
    RESET_MAP_ARRAY_STORAGE;
}

RVA(0x0009e720, 0x5)
CBrickzNodePool::~CBrickzNodePool() {
    Free();
}

// @early-stop
RVA(0x0009e740, 0x76)
i32 CBrickzNodePool::Allocate(u32 count) {
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
void CBrickzNodePool::Free() {
    if (m_storage) {
        delete[] m_storage;
    }
    RESET_MAP_ARRAY_STORAGE;
}

RVA(0x0009e820, 0xd)
CBrickzCellNodePool::CBrickzCellNodePool() {
    RESET_MAP_ARRAY_STORAGE;
}

RVA(0x0009e840, 0x5)
CBrickzCellNodePool::~CBrickzCellNodePool() {
    Free();
}

// @early-stop
RVA(0x0009e860, 0x7a)
i32 CBrickzCellNodePool::Allocate(u32 count) {
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
void CBrickzCellNodePool::Free() {
    if (m_storage) {
        delete[] m_storage;
    }
    RESET_MAP_ARRAY_STORAGE;
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
    m_diagonalMask = 0;
    m_blockedMask = -1;
    m_dirty = true;
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
    memset(m_cellPool, 0, count * sizeof(BrickzCell));
    i32 stride = width;
    i32 off = 0;

    for (u32 i = 0; i < static_cast<u32>(height); i++) {
        m_rows[i] = m_cellPool + off;
        off += stride;
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

RVA(0x0009ec30, 0x4b)
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

// @early-stop
RVA(0x0009eca0, 0x2bd)
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
    Coord start(startX, startY);
    Coord goal(goalX, goalY);
    Coord bounds(m_bounds.left, m_bounds.top);
    if (static_cast<u32>(start.m_x - bounds.m_x) >= static_cast<u32>(m_gridSize.cx)) {
        return 0;
    }
    i32 gridHeight = m_gridSize.cy;
    if (static_cast<u32>(start.m_y - bounds.m_y) >= static_cast<u32>(gridHeight)) {
        return 0;
    }
    if (static_cast<u32>(goal.m_x - bounds.m_x) >= static_cast<u32>(m_gridSize.cx)) {
        return 0;
    }
    if (static_cast<u32>(goal.m_y - bounds.m_y) >= static_cast<u32>(gridHeight)) {
        return 0;
    }
    m_passableMask = passableMask;
    m_diagonalMask = diagonalMask;
    m_blockedMask = blockedMask;
    i32 goalFlags = m_rows[goal.m_y][goal.m_x].m_flags;
    if ((blockedMask & goalFlags) != 0 && (passableMask & goalFlags) == 0) {
        return 0;
    }

    for (u32 i = 0; i < m_cellCount; i++) {
        m_cellPool[i].m_count = 0;
    }
    if (start == goal) {
        return 1;
    }
    m_goal = goal;
    m_start = start;

    BrickzNode* seed = m_nodePool.m_freeList;
    BrickzNode* slot = seed->m_openNext;
    if (slot == NULL) {
        seed = NULL;
    } else {
        m_nodePool.m_freeList = slot;
        slot->m_openPrev = NULL;
    }
    if (seed == NULL) {
        return 0;
    }
    seed->m_col = start.m_x;
    seed->m_row = start.m_y;
    seed->m_gCost = 0;
    Coord delta = goal - start;
    Coord distance = delta.GetAbs();
    i32 h = (distance.m_y + distance.m_x) * 2;
    seed->m_hCost = h;
    seed->m_fCost = h;
    seed->m_openNext = NULL;
    seed->m_openPrev = NULL;
    seed->m_parent = NULL;
    InsertOpenNode(seed);
    (&m_rows[start.m_y][start.m_x])->m_count++;
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
        CoordPoolNode* rec = g_coordPool.m_freeHead;
        Coord cell(p->m_col, p->m_row);
        Coord* slot = NULL;
        if (rec->m_next != NULL) {
            slot = &rec->m_coord;
            *slot = cell;
            g_coordPool.m_freeHead = g_coordPool.m_freeHead->m_next;
        }

        outPath->AddHead(slot);
        p = p->m_parent;
    }
    if (m_stepCb != NULL) {
        m_stepCb();
    }
    node->m_openPrev = NULL;
    node->m_openNext = m_nodePool.m_freeList;
    m_nodePool.m_freeList->m_openPrev = node;
    m_nodePool.m_freeList = node;
    RecycleOpenNodes();
    RecycleClosedNodes();
    return 1;
}

RVA(0x0009f010, 0x2a1)
i32 CMapMgr::ExpandNeighbor(BrickzNode* node, i32 dx, i32 dy, i32 cost, i32 diagonal) {
    i32 ng = node->m_gCost + cost;
    Coord neighbor(node->m_col + dx, node->m_row + dy);
    if (static_cast<u32>((neighbor.m_x - m_bounds.left)) >= static_cast<u32>(m_gridSize.cx)) {
        return 1;
    }
    if (static_cast<u32>((neighbor.m_y - m_bounds.top)) >= static_cast<u32>(m_gridSize.cy)) {
        return 1;
    }
    BrickzCell* ncell = &m_rows[neighbor.m_y][neighbor.m_x];
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
        open = FindOpenNode(neighbor.m_x, neighbor.m_y);
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
    BrickzNode* rec = m_nodePool.m_freeList;
    BrickzNode* nx = rec->m_openNext;
    if (nx == NULL) {
        rec = NULL;
    } else {
        m_nodePool.m_freeList = nx;
        nx->m_openPrev = NULL;
    }
    if (rec == NULL) {
        return 0;
    }
    rec->m_col = neighbor.m_x;
    rec->m_row = neighbor.m_y;
    rec->m_gCost = ng;
    Coord delta = m_goal - neighbor;
    Coord distance = delta.GetAbs();
    i32 h = (distance.m_x + distance.m_y) * 2;
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

RVA(0x0009f370, 0x8a)
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

RVA(0x0009f430, 0x2a)
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

static inline BrickzCellNode* PopFreeCellNode(BrickzCellNode*& freeList) {
    BrickzCellNode* node = freeList;
    BrickzCellNode* next = node->m_cellNext;
    if (next == NULL) {
        return NULL;
    }
    freeList = next;
    next->m_cellPrev = NULL;
    return node;
}

// @early-stop
RVA(0x0009f470, 0x62)
void CMapMgr::LinkClosedNode(BrickzNode* node) {
    BrickzCellNode** head = &m_rows[node->m_row][node->m_col].m_head;
    BrickzCellNode* slot = PopFreeCellNode(m_cellNodePool.m_freeList);
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

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0009f540, 0x40)
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

RVA(0x0009f590, 0x2f)
void CMapMgr::RecycleOpenNodes() {
    BrickzNode* p = m_openList;
    if (p != NULL) {
        do {
            BrickzNode* cur = p;
            p = cur->m_openNext;
            cur->m_openNext = m_nodePool.m_freeList;
            cur->m_openPrev = NULL;
            m_nodePool.m_freeList->m_openPrev = cur;
            m_nodePool.m_freeList = cur;
        } while (p != NULL);
    }
    m_openList = NULL;
}

// @early-stop
RVA(0x0009f5d0, 0x81)
void CMapMgr::RecycleClosedNodes() {
    BrickzCell* cell = m_cellPool;
    for (u32 i = 0; i < m_height * m_width; i++) {
        BrickzCellNode* node = cell->m_head;
        while (node != NULL) {
            BrickzCellNode* cur = node;
            BrickzCellNode** link = &cur->m_cellNext;
            node = *link;
            BrickzNode* child = cur->m_searchNode;
            child->m_openNext = m_nodePool.m_freeList;
            child->m_openPrev = NULL;
            m_nodePool.m_freeList->m_openPrev = child;
            m_nodePool.m_freeList = child;
            cur->m_cellPrev = NULL;
            *link = m_cellNodePool.m_freeList;
            m_cellNodePool.m_freeList->m_cellPrev = cur;
            m_cellNodePool.m_freeList = cur;
        }
        cell->m_head = NULL;
        cell++;
    }
}

RVA(0x0009f690, 0x5d)
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

RVA(0x0009f710, 0xa7)
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
    slot->m_cellNext = m_cellNodePool.m_freeList;
    slot->m_cellPrev = NULL;
    m_cellNodePool.m_freeList->m_cellPrev = slot;
    m_cellNodePool.m_freeList = slot;
    if (recycleSearchNode != 0) {
        node->m_openNext = m_nodePool.m_freeList;
        node->m_openPrev = NULL;
        m_nodePool.m_freeList->m_openPrev = node;
        m_nodePool.m_freeList = node;
    }
}

RVA(0x0009f7f0, 0x3b)
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
    ar->Write(&m_blockedMask, sizeof(m_blockedMask));
    ar->Write(&m_passableMask, sizeof(m_passableMask));
    ar->Write(&m_diagonalMask, sizeof(m_diagonalMask));
    ar->Write(&m_dirty, sizeof(m_dirty));
    ar->Write(&m_bounds.left, sizeof(m_bounds));
    ar->Write(&m_gridSize.cx, sizeof(m_gridSize.cx));
    ar->Write(&m_gridSize.cy, sizeof(m_gridSize.cy));
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
    ar->Read(&m_blockedMask, sizeof(m_blockedMask));
    ar->Read(&m_passableMask, sizeof(m_passableMask));
    ar->Read(&m_diagonalMask, sizeof(m_diagonalMask));
    ar->Read(&m_dirty, sizeof(m_dirty));
    ar->Read(&m_bounds.left, sizeof(m_bounds));
    ar->Read(&m_gridSize.cx, sizeof(m_gridSize.cx));
    ar->Read(&m_gridSize.cy, sizeof(m_gridSize.cy));
    for (u32 i = 0; i < m_width; i++) {
        for (u32 j = 0; j < m_height; j++) {
            ar->Read(&m_cellPool[j * m_width + i], sizeof(m_cellPool[j * m_width + i]));
            m_cellPool[j * m_width + i].m_head = NULL;
        }
    }
    return 1;
}

RVA_DYNINIT(0x0009fdf0, 0x5, g_versionRect)
RVA_DYNINIT(0x0009fe10, 0x29, g_versionRect)
DATA(0x00245cc8)
CRect g_versionRect(5, 453, 635, 478);
