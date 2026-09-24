#ifndef GRUNTZ_LOGICEVENTDISPATCH_H
#define GRUNTZ_LOGICEVENTDISPATCH_H

#include <rva.h>

#include <Ints.h>

class CUserLogic;

i32 DispatchLogicEvent(CUserLogic* logic);

inline void DispatchUnhandledLogicEvent(CUserLogic* sub) {
    DispatchLogicEvent(sub);
}

#endif // GRUNTZ_LOGICEVENTDISPATCH_H
