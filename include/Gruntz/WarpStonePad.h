#ifndef GRUNTZ_CWARPSTONEPAD_H
#define GRUNTZ_CWARPSTONEPAD_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CWarpStonePad : public CUserLogic, public CWapX {
    RVA(0x00010f20, 0x47)
    virtual i32
    SerializeMove(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, CGameObject* pObj)
        OVERRIDE{SERIALIZE_USER_LOGIC_AND_CHAIN(ar, mode, typeId, pObj)}

    RVA(0x00010f00, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_WARPSTONEPAD;
    }

public:
public:
    CWarpStonePad() {}
    CWarpStonePad(CGameObject* obj);

    virtual void FireActivation(i32 id) OVERRIDE;
    static void RegisterActs();
    i32 AdvanceAnim();
};

#endif // GRUNTZ_CWARPSTONEPAD_H
