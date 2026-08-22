#ifndef GRUNTZ_GRUNTCOORDRECYCLEMACROS_H
#define GRUNTZ_GRUNTCOORDRECYCLEMACROS_H

#define RECYCLE_GRUNT_COORDS_IF_ANY(grunt)                                                         \
    if ((grunt)->CoordCount() != 0) {                                                              \
        CoordNode* node = (grunt)->CoordHead();                                                    \
        while (node != NULL) {                                                                     \
            CoordNode* current = node;                                                             \
            node = node->m_next;                                                                   \
            if (current->m_coord != NULL) {                                                        \
                g_coordPool.Push(current->m_coord);                                                \
            }                                                                                      \
        }                                                                                          \
        (grunt)->m_coordList.RemoveAll();                                                          \
    }

#define RECYCLE_GRUNT_COORDS_INLINE_PUSH_IF_ANY(grunt)                                             \
    if ((grunt)->CoordCount() != 0) {                                                              \
        CoordNode* node = (grunt)->CoordHead();                                                    \
        while (node != NULL) {                                                                     \
            CoordNode* current = node;                                                             \
            node = node->m_next;                                                                   \
            if (current->m_coord != NULL) {                                                        \
                PushFreeNode(&g_coordPool, current->m_coord);                                      \
            }                                                                                      \
        }                                                                                          \
        (grunt)->m_coordList.RemoveAll();                                                          \
    }

#define RECYCLE_GRUNT_COORDS(grunt)                                                                \
    {                                                                                              \
        CoordNode* node = (grunt)->CoordHead();                                                    \
        while (node != NULL) {                                                                     \
            CoordNode* current = node;                                                             \
            node = node->m_next;                                                                   \
            if (current->m_coord != NULL) {                                                        \
                g_coordPool.Push(current->m_coord);                                                \
            }                                                                                      \
        }                                                                                          \
        (grunt)->m_coordList.RemoveAll();                                                          \
    }

#define RECYCLE_GRUNT_COORDS_INLINE_PUSH(grunt)                                                    \
    {                                                                                              \
        CoordNode* node = (grunt)->CoordHead();                                                    \
        while (node != NULL) {                                                                     \
            CoordNode* current = node;                                                             \
            node = node->m_next;                                                                   \
            if (current->m_coord != NULL) {                                                        \
                PushFreeNode(&g_coordPool, current->m_coord);                                      \
            }                                                                                      \
        }                                                                                          \
        (grunt)->m_coordList.RemoveAll();                                                          \
    }

#define RECYCLE_GRUNT_COORDS_INLINE_POOL_IF_ANY(grunt)                                             \
    if ((grunt)->CoordCount() != 0) {                                                              \
        CoordNode* node = (grunt)->CoordHead();                                                    \
        while (node != NULL) {                                                                     \
            CoordNode* current = node;                                                             \
            node = node->m_next;                                                                   \
            if (current->m_coord != NULL) {                                                        \
                CoordPoolNode* slot = g_coordPool.NodeOf(current->m_coord);                        \
                slot->m_next = g_coordPool.m_freeHead;                                             \
                g_coordPool.m_freeHead = slot;                                                     \
            }                                                                                      \
        }                                                                                          \
        (grunt)->m_coordList.RemoveAll();                                                          \
    }

#define RECYCLE_GRUNT_COORDS_INLINE_POOL(grunt)                                                    \
    {                                                                                              \
        CoordNode* node = (grunt)->CoordHead();                                                    \
        while (node != NULL) {                                                                     \
            CoordNode* current = node;                                                             \
            node = node->m_next;                                                                   \
            if (current->m_coord != NULL) {                                                        \
                CoordPoolNode* slot = g_coordPool.NodeOf(current->m_coord);                        \
                slot->m_next = g_coordPool.m_freeHead;                                             \
                g_coordPool.m_freeHead = slot;                                                     \
            }                                                                                      \
        }                                                                                          \
        (grunt)->m_coordList.RemoveAll();                                                          \
    }

#define RECYCLE_GRUNT_COORDS_POSITION(grunt)                                                       \
    {                                                                                              \
        POSITION position = (grunt)->m_coordList.GetHeadPosition();                                \
        while (position != NULL) {                                                                 \
            Coord* coord = static_cast<Coord*>((grunt)->m_coordList.GetNext(position));            \
            if (coord != NULL) {                                                                   \
                g_coordPool.Push(coord);                                                           \
            }                                                                                      \
        }                                                                                          \
        (grunt)->m_coordList.RemoveAll();                                                          \
    }

#define RECYCLE_GRUNT_COORDS_POSITION_INLINE_PUSH(grunt)                                           \
    {                                                                                              \
        POSITION position = (grunt)->m_coordList.GetHeadPosition();                                \
        while (position != NULL) {                                                                 \
            Coord* coord = static_cast<Coord*>((grunt)->m_coordList.GetNext(position));            \
            if (coord != NULL) {                                                                   \
                PushFreeNode(&g_coordPool, coord);                                                 \
            }                                                                                      \
        }                                                                                          \
        (grunt)->m_coordList.RemoveAll();                                                          \
    }

#define RECYCLE_GRUNT_COORDS_POSITION_INLINE_POOL(grunt)                                           \
    {                                                                                              \
        POSITION position = (grunt)->m_coordList.GetHeadPosition();                                \
        while (position != NULL) {                                                                 \
            Coord* coord = static_cast<Coord*>((grunt)->m_coordList.GetNext(position));            \
            if (coord != NULL) {                                                                   \
                CoordPoolNode* slot = g_coordPool.NodeOf(coord);                                   \
                slot->m_next = g_coordPool.m_freeHead;                                             \
                g_coordPool.m_freeHead = slot;                                                     \
            }                                                                                      \
        }                                                                                          \
        (grunt)->m_coordList.RemoveAll();                                                          \
    }

#define RECYCLE_GRUNT_COORDS_OPS_POSITION(grunt)                                                   \
    {                                                                                              \
        POSITION position = (grunt)->m_coordList.GetHeadPosition();                                \
        while (position != NULL) {                                                                 \
            Coord* coord = static_cast<Coord*>((grunt)->CoordListOps()->NextData(position));       \
            if (coord != NULL) {                                                                   \
                g_coordPool.Push(coord);                                                           \
            }                                                                                      \
        }                                                                                          \
        (grunt)->m_coordList.RemoveAll();                                                          \
    }

#endif // GRUNTZ_GRUNTCOORDRECYCLEMACROS_H
