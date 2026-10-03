#ifndef GRUNTZ_COORDPOOL_H
#define GRUNTZ_COORDPOOL_H

#include <Gruntz/CoordNode.h>
#include <Utils/FreeNodePool.h>

typedef FreeNodePool<Coord>::Node CoordPoolNode;
extern FreeNodePool<Coord> g_coordPool;

// Return payloads to the pool; the caller still owns the list nodes.
inline void RecycleCoordList(CPtrList& list) {
    POSITION node = list.GetHeadPosition();
    if (node != NULL) {
        do {
            g_coordPool.Push(static_cast<Coord*>(list.GetNext(node)));
        } while (node != NULL);
    }
}

#endif // GRUNTZ_COORDPOOL_H
