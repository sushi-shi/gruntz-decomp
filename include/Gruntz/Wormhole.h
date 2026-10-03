#ifndef GRUNTZ_CWORMHOLE_H
#define GRUNTZ_CWORMHOLE_H

#include <Ints.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/LogicFnTable.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CWormhole : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*)  ;

    virtual LogicTypeId GetTypeTag()   {
        return LOGIC_WORMHOLE;
    }

public:
    virtual void FireActivation(i32 id)  ;

    CWormhole() {}
    CWormhole(CGameObject* obj);
    i32 SpawnPartners();
};

#endif
