#ifndef GRUNTZ_CANICYCLE_H
#define GRUNTZ_CANICYCLE_H

#include <rva.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CAniCycle : public CUserLogic, public CWapX {
public:
public:
    CAniCycle() : CUserLogic(CUserLogic::INLINE_BASE) {}
    CAniCycle(CGameObject* obj);

    RVA(0x0000f450, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_ANICYCLE;
    }
    RVA(0x0000f470, 0x47)
    virtual i32
    SerializeMove(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, CGameObject* object)
        OVERRIDE {
        SERIALIZE_USER_LOGIC_AND_CHAIN(ar, mode, typeId, object)
    }

    virtual void FireActivation(i32 id) OVERRIDE;

    static void RegisterActs();

    i32 AdvanceAnim();
};

#endif // GRUNTZ_CANICYCLE_H
