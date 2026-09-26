#ifndef GRUNTZ_COORDPOOL_H
#define GRUNTZ_COORDPOOL_H

#include <Mfc.h>

#include <Gruntz/CoordNode.h>
#include <Utils/FreeNodePool.h>

typedef FreeNodePool<Coord>::Node CoordPoolNode;
extern FreeNodePool<Coord> g_coordPool;

inline void RecycleHeadCoord(CPtrList& list) {
    Coord* head = static_cast<Coord*>(list.RemoveHead());
    if (head != NULL) {
        g_coordPool.Push(head);
    }
}

#endif // GRUNTZ_COORDPOOL_H
