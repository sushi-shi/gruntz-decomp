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

#define RECYCLE_HEAD_COORD(list)                                                                   \
    {                                                                                              \
        Coord* head = static_cast<Coord*>((list).RemoveHead());                                    \
        if (head != NULL) {                                                                        \
            g_coordPool.Push(head);                                                                \
        }                                                                                          \
    }

#endif // GRUNTZ_GRUNTCOORDRECYCLEMACROS_H
