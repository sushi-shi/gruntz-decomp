#ifndef GRUNTZ_GRUNTZ_LOGICFNTABLE_H
#define GRUNTZ_GRUNTZ_LOGICFNTABLE_H

#include <Gruntz/ActReg.h> // CLogicActTable (the SAME per-logic-class dispatch-table shell)
#include <rva.h>

typedef CLogicActTable LogicFnTable;
SIZE_UNKNOWN();

extern LogicFnTable g_eyeCandyDispatch;

#endif // GRUNTZ_GRUNTZ_LOGICFNTABLE_H
