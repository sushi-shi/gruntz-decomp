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
    virtual i32 SerializeMove(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;
    virtual void FireActivation(i32 id) OVERRIDE;

    static void RegisterActs();
    i32 Update();
};
SIZE_UNKNOWN();

#endif // GRUNTZ_CPARTICLEZ_H
