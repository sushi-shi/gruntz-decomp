#ifndef GRUNTZ_CPARTICLEZ_H
#define GRUNTZ_CPARTICLEZ_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CParticlez : public CUserLogic, public CWapX {
public:
public:
    CParticlez() {}
    CParticlez(CGameObject* obj);

    RVA(0x00012cd0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_PARTICLEZ;
    }
    RVA(0x00012cf0, 0x47)
    virtual i32 SerializeMove(CFileMemBase* ar, SerialMode tag, LogicTypeId c, CGameObject* d)
        OVERRIDE {
        if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
            return 0;
        }
        return Chain(ar, tag, c, d) != 0;
    }
    virtual void FireActivation(i32 id) OVERRIDE;

    static void RegisterActs();
    i32 Update();
};

#endif // GRUNTZ_CPARTICLEZ_H
