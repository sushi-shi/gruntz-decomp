#ifndef GRUNTZ_CGUARDPOINT_H
#define GRUNTZ_CGUARDPOINT_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CGuardPoint : public CUserLogic, public CWapX {
public:
public:
    RVA(0x00010360, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_GUARDPOINT;
    }
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;
    CGuardPoint() {}
    CGuardPoint(CGameObject* obj);
};

#endif // GRUNTZ_CGUARDPOINT_H
