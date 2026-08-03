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
    RVA(0x0000fcc0, 0x47)
    virtual i32 SerializeMove(CFileMemBase* ar, SerialMode tag, LogicTypeId c, CGameObject* d)
        OVERRIDE {
        if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
            return 0;
        }
        return Chain(ar, tag, c, d) != 0;
    }
};
SIZE_UNKNOWN();

#endif // GRUNTZ_CEYECANDY_H
