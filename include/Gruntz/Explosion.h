#ifndef GRUNTZ_CEXPLOSION_H
#define GRUNTZ_CEXPLOSION_H

#include <rva.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CExplosion : public CUserLogic, public CWapX {
public:
    RVA(0x00012e20, 0x47)
    virtual i32 SerializeMove(CFileMemBase* ar, SerialMode tag, LogicTypeId c, CGameObject* d)
        OVERRIDE {
        if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
            return 0;
        }
        return Chain(ar, tag, c, d) != 0;
    }
    RVA(0x00012e00, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_EXPLOSION;
    }

public:
    CExplosion() {}
    CExplosion(CGameObject* obj);

    virtual void FireActivation(i32 id) OVERRIDE;

    i32 Update();
};
SIZE(0x54);

SIZE_UNKNOWN();

#endif // GRUNTZ_CEXPLOSION_H
