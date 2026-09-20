#ifndef GRUNTZ_GRUNTCOORDRECYCLEMACROS_H
#define GRUNTZ_GRUNTCOORDRECYCLEMACROS_H

#define RECYCLE_GRUNT_COORDS(grunt)                                                                \
    {                                                                                              \
        POSITION node = (grunt)->CoordHead();                                                      \
        while (node != NULL) {                                                                     \
            POSITION current = node;                                                               \
            (grunt)->m_coordList.GetNext(node);                                                    \
            if (static_cast<Coord*>((grunt)->m_coordList.GetAt(current)) != NULL) {                \
                g_coordPool.Push(static_cast<Coord*>((grunt)->m_coordList.GetAt(current)));        \
            }                                                                                      \
        }                                                                                          \
        (grunt)->m_coordList.RemoveAll();                                                          \
    }

#define RECYCLE_GRUNT_COORDS_EXPANDED(grunt)                                                       \
    {                                                                                              \
        POSITION node = (grunt)->CoordHead();                                                      \
        while (node != NULL) {                                                                     \
            POSITION current = node;                                                               \
            (grunt)->m_coordList.GetNext(node);                                                    \
            if (static_cast<Coord*>((grunt)->m_coordList.GetAt(current)) != NULL) {                \
                CoordPoolNode* slot =                                                              \
                    g_coordPool.NodeOf(static_cast<Coord*>((grunt)->m_coordList.GetAt(current)));  \
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
            Coord* coord = static_cast<Coord*>((grunt)->m_coordList.GetNext(position));            \
            if (coord != NULL) {                                                                   \
                g_coordPool.Push(coord);                                                           \
            }                                                                                      \
        }                                                                                          \
        (grunt)->m_coordList.RemoveAll();                                                          \
    }

#endif // GRUNTZ_GRUNTCOORDRECYCLEMACROS_H
