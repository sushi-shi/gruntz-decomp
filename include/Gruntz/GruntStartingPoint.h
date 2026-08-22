#ifndef GRUNTZ_CGRUNTSTARTINGPOINT_H
#define GRUNTZ_CGRUNTSTARTINGPOINT_H

#include <rva.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CGruntStartingPoint : public CUserLogic, public CWapX {
public:
    RVA(0x000105d0, 0x47)
    virtual i32 SerializeMove(CFileMemBase* ar, SerialMode tag, LogicTypeId c, CGameObject* d)
        OVERRIDE{SERIALIZE_USER_LOGIC_AND_CHAIN(ar, tag, c, d)} RVA(0x000105b0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_GRUNTSTARTINGPOINT;
    }

public:
    CGruntStartingPoint() {}
    CGruntStartingPoint(CGameObject* obj);

    virtual void FireActivation(i32 id) OVERRIDE;

    i32 Idle();
};

#endif // GRUNTZ_CGRUNTSTARTINGPOINT_H
