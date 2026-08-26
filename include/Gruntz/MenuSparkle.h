#ifndef GRUNTZ_CMENUSPARKLE_H
#define GRUNTZ_CMENUSPARKLE_H

#include <rva.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CMenuSparkle : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;
    RVA(0x00010170, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_MENUSPARKLE;
    }

public:
    CMenuSparkle() {}
    CMenuSparkle(CGameObject* obj);

    virtual void FireActivation(i32 id) OVERRIDE;

    i32 AdvanceAnim();
};

#endif // GRUNTZ_CMENUSPARKLE_H
