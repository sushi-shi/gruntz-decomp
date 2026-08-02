#ifndef GRUNTZ_CBEHINDCANDYANI_H
#define GRUNTZ_CBEHINDCANDYANI_H

#include <rva.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/UserLogic.h>

class CBehindCandyAni : public CUserLogic, public CWapX {
public:
public:
    CBehindCandyAni() {}
    CBehindCandyAni(CGameObject* obj);

    virtual void FireActivation(i32 id) OVERRIDE;

    static void RegisterActs();
    i32 AdvanceAnim();

    RVA(0x00010030, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_BEHINDCANDYANI;
    }
    virtual i32 SerializeMove(CFileMemBase*, i32, i32, CGameObject*) OVERRIDE;
};
SIZE(0x54);

#endif // GRUNTZ_CBEHINDCANDYANI_H
