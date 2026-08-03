#ifndef GRUNTZ_CEYECANDY_H
#define GRUNTZ_CEYECANDY_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CEyeCandy : public CUserLogic, public CWapX {
public:
public:
    CEyeCandy() {}
    CEyeCandy(CGameObject* obj);

    RVA(0x0000fca0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_EYECANDY;
    }
    virtual i32 SerializeMove(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;
};
SIZE_UNKNOWN();

#endif // GRUNTZ_CEYECANDY_H
