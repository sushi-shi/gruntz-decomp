#ifndef GRUNTZ_COORDPOOL_H
#define GRUNTZ_COORDPOOL_H

#include <Gruntz/CoordNode.h>
#include <Utils/FreeNodePool.h>

typedef FreeNodePool<Coord>::Node CoordPoolNode;
extern FreeNodePool<Coord> g_coordPool;

#endif // GRUNTZ_COORDPOOL_H
