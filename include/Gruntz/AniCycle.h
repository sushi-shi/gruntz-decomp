#ifndef GRUNTZ_CANICYCLE_H
#define GRUNTZ_CANICYCLE_H

#include <rva.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CAniCycle : public CUserLogic, public CWapX {
public:
public:
    CAniCycle() {}
    CAniCycle(CGameObject* obj);

    RVA(0x0000f450, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_ANICYCLE;
    }
    virtual i32 SerializeMove(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;

    virtual void FireActivation(i32 id) OVERRIDE;

    static void RegisterActs();

    i32 AdvanceAnim();
};
SIZE(0x54);

#endif // GRUNTZ_CANICYCLE_H
