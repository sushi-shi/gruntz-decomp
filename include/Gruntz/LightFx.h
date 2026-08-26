#ifndef GRUNTZ_GRUNTZ_CLIGHTFX_H
#define GRUNTZ_GRUNTZ_CLIGHTFX_H

#include <rva.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>
#include <Ints.h>

class CLightFx : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;
    RVA(0x000123f0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_LIGHTFX;
    }

public:
    CLightFx() {}
    CLightFx(CGameObject* obj);

    virtual void FireActivation(i32 id) OVERRIDE;

    static void RegisterActs();

    i32 AdvanceAnim();

    void Activate(
        const char* imageSetName,
        const char* animationName,
        i32 shadeTableIndex,
        b32 deleteWhenComplete
    );

    i32 RebindNode();

    i32 m_shadeTableIndex;
    b32 m_deleteWhenComplete;
};

#endif // GRUNTZ_GRUNTZ_CLIGHTFX_H
