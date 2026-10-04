#ifndef GRUNTZ_GRUNTCOORDINLINE_H
#define GRUNTZ_GRUNTCOORDINLINE_H

#include <Gruntz/CoordPool.h>
#include <Gruntz/Grunt.h>

inline void CGrunt::RecycleHeadCoord() {
    Coord* coord = RemoveHeadCoord();
    g_coordPool.Push(coord);
}

RVA(0x000343f0, 0x47)
inline void CGrunt::RecycleCoords() {
    if (CoordsEmpty()) {
        return;
    }
    POSITION n = CoordHead();
    if (n != NULL) {
        do {
            Coord* coord = static_cast<Coord*>(m_coordList.GetNext(n));
            if (coord != NULL) {
                g_coordPool.Push(coord);
            }
        } while (n != NULL);
    }
    m_coordList.RemoveAll();
}

#endif // GRUNTZ_GRUNTCOORDINLINE_H
