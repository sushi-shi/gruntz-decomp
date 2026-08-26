#ifndef GRUNTZ_CEYECANDY_H
#define GRUNTZ_CEYECANDY_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CEyeCandy : public CUserLogic, public CWapX {
public:
public:
    CEyeCandy() : CUserLogic(CUserLogic::INLINE_BASE) {}
    CEyeCandy(CGameObject* obj);

    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_EYECANDY;
    }
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;
};

#endif // GRUNTZ_CEYECANDY_H
