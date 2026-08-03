#ifndef GRUNTZ_CLEVELTIME_H
#define GRUNTZ_CLEVELTIME_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CLevelTime : public CUserLogic, public CWapX {
public:
    RVA(0x000119b0, 0x47)
    virtual i32
    SerializeMove(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, CGameObject* pObj)
        OVERRIDE {
        if (!CUserLogic::SerializeMove(ar, mode, typeId, pObj)) {
            return 0;
        }
        return Chain(ar, mode, typeId, pObj) != 0;
    }
    RVA(0x00011990, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_LEVELTIME;
    }

public:
    CLevelTime() {}

    CLevelTime(CGameObject* obj);
};
SIZE(0x54);

#endif // GRUNTZ_CLEVELTIME_H
