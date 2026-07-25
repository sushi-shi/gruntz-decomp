// UserLogicCtorEmit.h - the UserLogicCtorEmit TU's external declarations.
#ifndef GRUNTZ_USERLOGICCTOREMIT_H
#define GRUNTZ_USERLOGICCTOREMIT_H

#include <Mfc.h> // afx.h FIRST (umbrella for any Win32 types below)
#include <Ints.h>
#include <rva.h>

struct CGameObject;

extern "C" i32 LogicAttackFactory(
    CGameObject* obj
); // GameObjNotifyFn ABI (CreateWorker registrant) // 0x56e4d0
extern "C" i32 LogicBumpFactory(
    CGameObject* obj
); // GameObjNotifyFn ABI (CreateWorker registrant)   // 0x56e4e0

#endif // GRUNTZ_USERLOGICCTOREMIT_H
