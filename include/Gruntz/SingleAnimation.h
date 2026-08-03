#ifndef GRUNTZ_CSINGLEANIMATION_H
#define GRUNTZ_CSINGLEANIMATION_H

#include <rva.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CSingleAnimation : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;
    RVA(0x00010480, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_SINGLEANIMATION;
    }

public:
    CSingleAnimation() {}
    CSingleAnimation(CGameObject* obj);
    virtual void FireActivation(i32 id) OVERRIDE;
    static void RegisterActs();

    i32 AdvanceAnim();
};
SIZE(0x54);

#endif // GRUNTZ_CSINGLEANIMATION_H
