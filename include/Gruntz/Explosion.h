#ifndef GRUNTZ_CEXPLOSION_H
#define GRUNTZ_CEXPLOSION_H

#include <rva.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CExplosion : public CUserLogic, public CWapX {
public:
    RVA(0x00012e30, 0x47)
    virtual i32
    SerializeDispatch(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, CGameObject* object)
        OVERRIDE{
            SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE(ar, mode, typeId, object)
        } RVA(0x00012e10, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_EXPLOSION;
    }

public:
    CExplosion() {}
    CExplosion(CGameObject* obj);

    virtual void FireActivation(i32 id) OVERRIDE;

    i32 Update();
};

#endif // GRUNTZ_CEXPLOSION_H
