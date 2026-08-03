#ifndef GRUNTZ_CSIMPLEANIMATION_H
#define GRUNTZ_CSIMPLEANIMATION_H

#include <rva.h>

#include <Gruntz/LogicFnTable.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CSimpleAnimation : public CUserLogic, public CWapX {
public:
    RVA(0x0000f930, 0x47)
    virtual i32 SerializeMove(CFileMemBase* ar, SerialMode tag, LogicTypeId c, CGameObject* d)
        OVERRIDE {
        if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
            return 0;
        }
        return Chain(ar, tag, c, d) != 0;
    }
    RVA(0x0000f910, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_SIMPLEANIMATION;
    }

public:
    CSimpleAnimation() {}
    CSimpleAnimation(CGameObject* obj);
    i32 AdvanceAnim();

    virtual void FireActivation(i32 id) OVERRIDE;
};
SIZE(0x54);

#endif // GRUNTZ_CSIMPLEANIMATION_H
