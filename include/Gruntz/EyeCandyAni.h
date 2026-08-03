#ifndef GRUNTZ_CEYECANDYANI_H
#define GRUNTZ_CEYECANDYANI_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CEyeCandyAni : public CUserLogic, public CWapX {
public:
public:
    CEyeCandyAni() {}
    CEyeCandyAni(CGameObject* obj);

    RVA(0x0000ff00, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_EYECANDYANI;
    }
    RVA(0x0000ff20, 0x47)
    virtual i32 SerializeMove(CFileMemBase* ar, SerialMode tag, LogicTypeId c, CGameObject* d)
        OVERRIDE {
        if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
            return 0;
        }
        return Chain(ar, tag, c, d) != 0;
    }

    virtual void FireActivation(i32 id) OVERRIDE;

    static void RegisterActs();

    i32 AdvanceAnim();
};
SIZE(0x54);

#endif // GRUNTZ_CEYECANDYANI_H
