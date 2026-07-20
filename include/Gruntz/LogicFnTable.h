// LogicFnTable.h - the per-logic-class command-dispatch table, the single shared
// shape of the zDArray<int (CUserLogic::*)(void)> registry every per-class logic
// module builds (RTTI ULogicFnTable / .?AV?$_zdvec@P8CUserLogic@@AEHXZ@@). Each
// concrete dispatch global (g_wormholeDispatch, g_simpleAnimDispatch, the icon
// action/state tables, the g_logicDispatch_* set) keeps its own DATA-pinned symbol
// but shares this one definition. Constructed over the index band [0x7d0, 0x7da]
// through the zDArray<T> ctor at 0x408710 (reached via the 0x15 static-init thunks
// / the 0x3742 ILT thunk); the ctor call reloc-masks.
#ifndef GRUNTZ_GRUNTZ_LOGICFNTABLE_H
#define GRUNTZ_GRUNTZ_LOGICFNTABLE_H

#include <Gruntz/ActReg.h> // CLogicActTable (the SAME per-logic-class dispatch-table shell)

// LogicFnTable IS CLogicActTable - two names had grown for the one dispatch-table
// shell (zDArray + the CActReg resolve surface). One class, both spellings.
typedef CLogicActTable LogicFnTable;

// The eye-candy per-coordinate dispatch table (0x646060; DATA home + construction in
// src/Gruntz/LogicDispatchInit.cpp). Declared here so FrontCandyAni references it from
// this owner header, not a per-TU extern.
extern LogicFnTable g_eyeCandyDispatch;

#endif // GRUNTZ_GRUNTZ_LOGICFNTABLE_H
