#ifndef GRUNTZ_GRUNTCOORDINLINE_H
#define GRUNTZ_GRUNTCOORDINLINE_H

#include <list>
struct Coord;

#include <Gruntz/CoordPool.h>
#include <Gruntz/Grunt.h>

inline void CGrunt::RecycleCoords() {
    if (CoordCount() == 0) {
        return;
    }
    std::list<Coord*>::iterator n = CoordHead();
    if (n != m_coordList.end()) {
        do {
            Coord* coord = static_cast<Coord*>(*(n++));
            if (coord != NULL) {
                g_coordPool.Push(coord);
            }
        } while (n != m_coordList.end());
    }
    m_coordList.clear();
}

#endif
