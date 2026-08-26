#ifndef GRUNTZ_CBEHINDCANDYANI_H
#define GRUNTZ_CBEHINDCANDYANI_H

#include <rva.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CBehindCandyAni : public CUserLogic, public CWapX {
public:
public:
    CBehindCandyAni() : CUserLogic(CUserLogic::INLINE_BASE) {}
    CBehindCandyAni(CGameObject* obj);

    virtual void FireActivation(i32 id) OVERRIDE;

    static void RegisterActs();
    i32 AdvanceAnim();

    RVA(0x00010040, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_BEHINDCANDYANI;
    }
    RVA(0x00010060, 0x47)
    virtual i32
    SerializeDispatch(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, CGameObject* object)
        OVERRIDE {
        SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE(ar, mode, typeId, object)
    }
};

#endif // GRUNTZ_CBEHINDCANDYANI_H
