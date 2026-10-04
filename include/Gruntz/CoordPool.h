#ifndef GRUNTZ_COORDPOOL_H
#define GRUNTZ_COORDPOOL_H

#include <list>
struct Coord;

#include <Gruntz/CoordNode.h>
#include <Utils/FreeNodePool.h>

typedef FreeNodePool<Coord>::Node CoordPoolNode;
extern FreeNodePool<Coord> g_coordPool;

inline void RecycleCoordList(std::list<Coord*>& list) {
    std::list<Coord*>::iterator node = list.begin();
    if (node != list.end()) {
        do {
            g_coordPool.Push(static_cast<Coord*>(*(node++)));
        } while (node != list.end());
    }
}

#endif
