#ifndef GRUNTZ_CWARLORD_H
#define GRUNTZ_CWARLORD_H

#include <rva.h>

#include <Mfc.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

struct WarlordTimer {
    i64 m_start;
    i64 m_window;

    WarlordTimer() : m_start(0), m_window(0) {}
};

class CWarlord : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;
    RVA(0x000107b0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_WARLORD;
    }

public:
    CWarlord() {}
    CWarlord(CGameObject* obj);

    virtual void FireActivation(i32 id) OVERRIDE;

    i32 FinishIdleAnimation();

    i32 FinishBattlecryAnimation();

    i32 FinishJoyAnimation();

    i32 UpdateMovingState();
    i32 UpdatePanicState();

    i32 BuildFortSplashParticles();

    i32 NotifyFortUnderAttack();

    i32 ResolveMovingAnimation();
    i32 ResolveDeathAnimation();
    i32 ResolveJoyAnimation();
    i32 ResolveIdleAnimation();
    i32 ResolveBattlecryAnimation();

    CString m_warlordName;

    CAniElement* m_idleAnims[4];
    CAniElement* m_battlecryAnims[3];
    CAniElement* m_animJoy;
    CAniElement* m_animDeath;
    CAniElement* m_animMoving;
    CAniElement* m_animPanic;
    char m_pad84[0x88 - 0x84];

    WarlordTimer m_cooldownTimer;
    WarlordTimer m_notifyTimer;
    b32 m_deathStarted;

    i32 m_ownerTag;
};

#endif // GRUNTZ_CWARLORD_H
