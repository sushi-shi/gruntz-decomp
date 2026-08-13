#ifndef GRUNTZ_CSTATUSBARSPRITE_H
#define GRUNTZ_CSTATUSBARSPRITE_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CStatusBarSprite : public CUserLogic, public CWapX {
public:
    RVA(0x00011ae0, 0x47)
    virtual i32
    SerializeMove(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, CGameObject* pObj)
        OVERRIDE {
        if (!CUserLogic::SerializeMove(ar, mode, typeId, pObj)) {
            return 0;
        }
        return Chain(ar, mode, typeId, pObj) != 0;
    }
    RVA(0x00011ac0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_STATUSBARSPRITE;
    }

public:
    CStatusBarSprite() {}
    CStatusBarSprite(CGameObject* obj);
    virtual void FireActivation(i32 id) OVERRIDE;
    static void RegisterActs();
    i32 AdvanceAnim();
};

#endif // GRUNTZ_CSTATUSBARSPRITE_H
