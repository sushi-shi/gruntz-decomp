#ifndef GRUNTZ_RANDOMEXTENTPOINT_H
#define GRUNTZ_RANDOMEXTENTPOINT_H

#include <Globals.h>
#include <Gruntz/CoordNode.h>

#include <stdlib.h>

template<class TObject>
inline void SelectRandomExtentPoint(TObject* object, Coord* position, Coord* span) {
    *position = Coord(object->m_extent.left, object->m_extent.top);
    Coord farCorner(object->m_extent.right, object->m_extent.bottom);
    *span = (farCorner - *position).GetAbs();
    if (span->m_x != 0) {
        position->m_x += rand() % span->m_x;
    }
    if (span->m_y != 0) {
        position->m_y += rand() % span->m_y;
    }
}

#endif // GRUNTZ_RANDOMEXTENTPOINT_H
