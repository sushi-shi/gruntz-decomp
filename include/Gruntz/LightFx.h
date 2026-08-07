#ifndef GRUNTZ_GRUNTZ_CLIGHTFX_H
#define GRUNTZ_GRUNTZ_CLIGHTFX_H

#include <rva.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CLightFx : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;
    RVA(0x000123e0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_LIGHTFX;
    }

public:
    CLightFx() {}
    CLightFx(CGameObject* obj);

    virtual void FireActivation(i32 id) OVERRIDE;

    static void RegisterActs();

    i32 AdvanceAnim();

    void Activate(const char* spec, const char* effect, i32 anchorA, i32 anchorB);

    i32 RebindNode();

    i32 m_anchorA;
    i32 m_anchorB;
};
SIZE(0x5c);

#endif // GRUNTZ_GRUNTZ_CLIGHTFX_H
