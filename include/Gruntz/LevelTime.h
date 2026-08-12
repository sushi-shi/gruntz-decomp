#ifndef GRUNTZ_CLEVELTIME_H
#define GRUNTZ_CLEVELTIME_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CLevelTime : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;
    RVA(0x00011990, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_LEVELTIME;
    }

public:
    CLevelTime() {}

    CLevelTime(CGameObject* obj);
};

#endif // GRUNTZ_CLEVELTIME_H
