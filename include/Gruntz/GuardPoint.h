#ifndef GRUNTZ_CGUARDPOINT_H
#define GRUNTZ_CGUARDPOINT_H

#include <Ints.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CGuardPoint : public CUserLogic, public CWapX {
public:
public:

    virtual LogicTypeId GetTypeTag()   {
        return LOGIC_GUARDPOINT;
    }
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*)  ;
    CGuardPoint() {}
    CGuardPoint(CGameObject* obj);
};

#endif
