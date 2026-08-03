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
    virtual i32 SerializeMove(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;

    virtual void FireActivation(i32 id) OVERRIDE;
    static void RegisterActs();
    i32 Trigger();

    i32 LoadAttributes(i32 a, i32 b);
};
SIZE(0x54);

#endif // GRUNTZ_CBRICKZ_H
