#ifndef GRUNTZ_CSECRETTELEPORTERTRIGGER_H
#define GRUNTZ_CSECRETTELEPORTERTRIGGER_H

#include <rva.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CSecretTeleporterTrigger : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;
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
