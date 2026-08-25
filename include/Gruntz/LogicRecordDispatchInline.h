#ifndef GRUNTZ_LOGICRECORDDISPATCHINLINE_H
#define GRUNTZ_LOGICRECORDDISPATCHINLINE_H

#include <rva.h>

#include <DDrawMgr/LogicRecord.h>
#include <Gruntz/LogicEventDispatch.h>
#include <Ints.h>
#include <Wwd/WwdGameObjectFamily.h>

class CUserLogic;

inline void DispatchUnhandledLogicEvent(CUserLogic* sub) {
    DispatchLogicEvent(sub);
}

#endif // GRUNTZ_LOGICRECORDDISPATCHINLINE_H
