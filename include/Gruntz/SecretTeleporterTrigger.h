#ifndef GRUNTZ_CSECRETTELEPORTERTRIGGER_H
#define GRUNTZ_CSECRETTELEPORTERTRIGGER_H

#include <Ints.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CSecretTeleporterTrigger : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*)  ;

    virtual LogicTypeId GetTypeTag()   {
        return LOGIC_SECRETTELEPORTERTRIGGER;
    }

public:
    CSecretTeleporterTrigger() {}
    CSecretTeleporterTrigger(CGameObject* obj);

    static void RegisterActs();

    virtual void FireActivation(i32 id)  ;

    i32 SpawnTeleporter();
};

#endif
