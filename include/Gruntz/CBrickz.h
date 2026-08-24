#ifndef GRUNTZ_CBRICKZ_H
#define GRUNTZ_CBRICKZ_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CBrickz : public CUserLogic, public CWapX {
public:
public:
    CBrickz() {}
    CBrickz(CGameObject* obj);

    RVA(0x00011300, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_BRICKZ;
    }
    RVA(0x00011320, 0x47)
    virtual i32
    SerializeMove(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, CGameObject* object)
        OVERRIDE {
        SERIALIZE_USER_LOGIC_AND_CHAIN(ar, mode, typeId, object)
    }

    virtual void FireActivation(i32 id) OVERRIDE;
    static void RegisterActs();
    i32 Trigger();

    i32 LoadAttributes(i32 a, i32 b);
};

#endif // GRUNTZ_CBRICKZ_H
