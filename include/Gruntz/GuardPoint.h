#ifndef GRUNTZ_CGUARDPOINT_H
#define GRUNTZ_CGUARDPOINT_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CGuardPoint : public CUserLogic, public CWapX {
public:
public:
    RVA(0x00010350, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_GUARDPOINT;
    }
    virtual i32 SerializeMove(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;
    CGuardPoint() {}
    CGuardPoint(CGameObject* obj);
};
SIZE(0x54);

#endif // GRUNTZ_CGUARDPOINT_H
