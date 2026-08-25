#ifndef GRUNTZ_GRUNTCOORDRECYCLEMACROS_H
#define GRUNTZ_GRUNTCOORDRECYCLEMACROS_H

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

#define RECYCLE_GRUNT_COORDS_EXPANDED(grunt)                                                       \
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

#define RECYCLE_GRUNT_COORDS_VIA_NEXTDATA(grunt)                                                   \
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
