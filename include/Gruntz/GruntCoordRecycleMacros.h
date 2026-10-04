#ifndef GRUNTZ_GRUNTCOORDRECYCLEMACROS_H
#define GRUNTZ_GRUNTCOORDRECYCLEMACROS_H

#include <Gruntz/GruntCoordInline.h>

#define RECYCLE_GRUNT_COORDS(grunt) (grunt)->RecycleCoords();
#define RECYCLE_GRUNT_COORDS_VIA_NEXTDATA(grunt) (grunt)->RecycleCoords();

#define RECYCLE_HEAD_COORD(items) { Coord* head = takeFront(items); if (head) g_coordPool.Push(head); }

#endif
