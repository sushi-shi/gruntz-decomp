#ifndef GRUNTZ_CTOOBSPIKEZ_H
#define GRUNTZ_CTOOBSPIKEZ_H

#include <rva.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CToobSpikez : public CUserLogic, public CWapX {
public:
public:
    CToobSpikez() {}
    CToobSpikez(CGameObject* obj);

    i32 AdvanceAnim();

    RVA(0x00012ba0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_TOOBSPIKEZ;
    }
    RVA(0x00012bc0, 0x47)
    virtual i32
    SerializeMove(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, CGameObject* object)
        OVERRIDE {
        SERIALIZE_USER_LOGIC_AND_CHAIN(ar, mode, typeId, object)
    }
    virtual void FireActivation(i32 id) OVERRIDE;
    static void RegisterActs();
};

#endif // GRUNTZ_CTOOBSPIKEZ_H
