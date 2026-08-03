#ifndef GRUNTZ_CTOOBSPIKEZ_H
#define GRUNTZ_CTOOBSPIKEZ_H

#include <rva.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CToobSpikez : public CUserLogic, public CWapX {
public:
public:
    CToobSpikez() {}
    CToobSpikez(CGameObject* obj);

    i32 AdvanceAnim();

    RVA(0x00012ba0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_TOOBSPIKEZ;
    }
    virtual i32 SerializeMove(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;
    virtual void FireActivation(i32 id) OVERRIDE;
    static void RegisterActs();
};
SIZE(0x54);

#endif // GRUNTZ_CTOOBSPIKEZ_H
