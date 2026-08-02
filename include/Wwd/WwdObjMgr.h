#ifndef GRUNTZ_WWD_WWDOBJMGR_H
#define GRUNTZ_WWD_WWDOBJMGR_H

#include <rva.h>

#include <Ints.h>

struct CGameObject;

extern i32 g_wwdObjIdCounter;

i32 __stdcall BoxesOverlap(CGameObject* areaObj, CGameObject* switchObj);

#endif // GRUNTZ_WWD_WWDOBJMGR_H
