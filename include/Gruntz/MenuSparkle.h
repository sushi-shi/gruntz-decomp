#ifndef GRUNTZ_CMENUSPARKLE_H
#define GRUNTZ_CMENUSPARKLE_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/ActReg.h>

class CMenuSparkle : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CFileMemBase*, i32, i32, CGameObject*) OVERRIDE;
    RVA(0x00010160, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_MENUSPARKLE;
    }

public:
    CMenuSparkle() {}
    CMenuSparkle(CGameObject* obj);

    virtual void FireActivation(i32 id) OVERRIDE;

    i32 AdvanceAnim();
};
SIZE_UNKNOWN();

#endif // GRUNTZ_CMENUSPARKLE_H
