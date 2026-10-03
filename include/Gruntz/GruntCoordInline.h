#ifndef GRUNTZ_GRUNTCOORDINLINE_H
#define GRUNTZ_GRUNTCOORDINLINE_H

#include <Gruntz/CoordPool.h>
#include <Gruntz/Grunt.h>

inline void CGrunt::RecycleCoords() {
    if (CoordCount() == 0) {
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

#endif
