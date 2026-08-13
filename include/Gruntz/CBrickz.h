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
    virtual i32 SerializeMove(CFileMemBase* a, SerialMode b, LogicTypeId c, CGameObject* d)
        OVERRIDE {
        if (!CUserLogic::SerializeMove(a, b, c, d)) {
            return 0;
        }
        return Chain(a, b, c, d) != 0;
    }

    virtual void FireActivation(i32 id) OVERRIDE;
    static void RegisterActs();
    i32 Trigger();

    i32 LoadAttributes(i32 a, i32 b);
};

#endif // GRUNTZ_CBRICKZ_H
