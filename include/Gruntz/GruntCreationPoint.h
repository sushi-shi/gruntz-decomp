#ifndef GRUNTZ_CGRUNTCREATIONPOINT_H
#define GRUNTZ_CGRUNTCREATIONPOINT_H

#include <rva.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CGruntCreationPoint : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;
    RVA(0x000106e0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_GRUNTCREATIONPOINT;
    }

public:
    CGruntCreationPoint() {}
    CGruntCreationPoint(CGameObject* obj);

    static void RegisterActs();

    virtual void FireActivation(i32 id) OVERRIDE;
    i32 AdvanceAnim();
};
SIZE(0x54);

#endif // GRUNTZ_CGRUNTCREATIONPOINT_H
