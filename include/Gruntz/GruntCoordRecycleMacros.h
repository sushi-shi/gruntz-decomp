#ifndef GRUNTZ_GRUNTCOORDRECYCLEMACROS_H
#define GRUNTZ_GRUNTCOORDRECYCLEMACROS_H

#include <Gruntz/GruntCoordInline.h>

// These macros preserve nested call boundaries in StepBoard, StepRowUnits
// and ApplyPickup.
#define RECYCLE_GRUNT_COORDS(grunt)                                                                \
    {                                                                                              \
        POSITION node = (grunt)->CoordHead();                                                      \
        while (node != NULL) {                                                                     \
            POSITION current = node;                                                               \
            (grunt)->GetNextCoord(node);                                                           \
            if ((grunt)->GetCoordAt(current) != NULL) {                                            \
                g_coordPool.Push((grunt)->GetCoordAt(current));                                    \
            }                                                                                      \
        }                                                                                          \
        (grunt)->m_coordList.RemoveAll();                                                          \
    }

#define RECYCLE_GRUNT_COORDS_VIA_NEXTDATA(grunt)                                                   \
    {                                                                                              \
        POSITION position = (grunt)->m_coordList.GetHeadPosition();                                \
        while (position != NULL) {                                                                 \
            Coord* coord = (grunt)->GetNextCoord(position);                                        \
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
