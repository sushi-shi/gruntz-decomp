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
    RVA(0x00012bc0, 0x47)
    virtual i32 SerializeMove(CFileMemBase* a, SerialMode b, LogicTypeId c, CGameObject* d)
        OVERRIDE {
        if (!CUserLogic::SerializeMove(a, b, c, d)) {
            return 0;
        }
        return Chain(a, b, c, d) != 0;
    }
    virtual void FireActivation(i32 id) OVERRIDE;
    static void RegisterActs();
};

#endif // GRUNTZ_CTOOBSPIKEZ_H
