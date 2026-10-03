#ifndef GRUNTZ_CBEHINDCANDY_H
#define GRUNTZ_CBEHINDCANDY_H

#include <Ints.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CBehindCandy : public CUserLogic, public CWapX {
public:
public:
    CBehindCandy() : CUserLogic(CUserLogic::INLINE_BASE) {}
    CBehindCandy(CGameObject* obj);

    virtual LogicTypeId GetTypeTag()   {
        return LOGIC_BEHINDCANDY;
    }
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*)  ;
};

#endif
