#ifndef GRUNTZ_CSECRETTELEPORTERTRIGGER_H
#define GRUNTZ_CSECRETTELEPORTERTRIGGER_H

#include <rva.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CSecretTeleporterTrigger : public CUserLogic, public CWapX {
public:
    RVA(0x00010a10, 0x47)
    virtual i32 SerializeMove(CFileMemBase* a, SerialMode b, LogicTypeId c, CGameObject* d)
        OVERRIDE {
        if (!CUserLogic::SerializeMove(a, b, c, d)) {
            return 0;
        }
        return Chain(a, b, c, d) != 0;
    }
    RVA(0x000109f0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_SECRETTELEPORTERTRIGGER;
    }

public:
    CSecretTeleporterTrigger() {}
    CSecretTeleporterTrigger(CGameObject* obj);

    static void RegisterActs();

    virtual void FireActivation(i32 id) OVERRIDE;

    i32 SpawnTeleporter();
};
SIZE(0x54);

#endif // GRUNTZ_CSECRETTELEPORTERTRIGGER_H
