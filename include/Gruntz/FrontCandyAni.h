#ifndef GRUNTZ_CFRONTCANDYANI_H
#define GRUNTZ_CFRONTCANDYANI_H

#include <rva.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CFrontCandyAni : public CUserLogic, public CWapX {
public:
    RVA(0x0000fdf0, 0x47)
    virtual i32 SerializeMove(CFileMemBase* ar, SerialMode tag, LogicTypeId c, CGameObject* d)
        OVERRIDE {
        if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
            return 0;
        }
        return Chain(ar, tag, c, d) != 0;
    }
    RVA(0x0000fdd0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_FRONTCANDYANI;
    }

public:
    CFrontCandyAni() {}
    CFrontCandyAni(CGameObject* obj);

    virtual void FireActivation(i32 id) OVERRIDE;

    static void RegisterActs();
    i32 AdvanceAnim();
};
SIZE_UNKNOWN();

#endif // GRUNTZ_CFRONTCANDYANI_H
