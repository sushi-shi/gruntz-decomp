#ifndef GRUNTZ_CWORMHOLE_H
#define GRUNTZ_CWORMHOLE_H

#include <rva.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/LogicFnTable.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CWormhole : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;
    RVA(0x00010930, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_WORMHOLE;
    }

public:
    virtual void FireActivation(i32 id) OVERRIDE;

    CWormhole() {}
    CWormhole(CGameObject* obj);
    i32 SpawnPartners();
};
SIZE_UNKNOWN();

typedef i32 (CUserLogic::*CActHandler)();

#endif // GRUNTZ_CWORMHOLE_H
