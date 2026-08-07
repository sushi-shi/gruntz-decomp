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

extern "C" u32 g_engineFrameDelta;

extern "C" u32 g_frameTime;

class CWarlord : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;
    RVA(0x000107a0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_WARLORD;
    }

public:
    CWarlord() {
        m_cooldownStamp = 0;
        m_cooldownWindow = 0;
        m_timer2Stamp = 0;
        m_timer2Window = 0;
    }
    CWarlord(CGameObject* obj);

    virtual void FireActivation(i32 id) OVERRIDE;

    i32 RearmMoving();

    i32 RearmMoving2();

    i32 AdvanceMovingAnim();

    i32 LoadAttributes();
    i32 LoadAttributes2();

    i32 BuildFortSplashParticles();

    i32 NotifyFortUnderAttack();

    i32 ResolveMovingAnimation();
    i32 ResolveDeathAnimation();
    i32 RaiseBattleAlert();
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

    i64 m_cooldownStamp;
    i64 m_cooldownWindow;
    i64 m_timer2Stamp;
    i64 m_timer2Window;
    i32 m_deathStarted;

    i32 m_ownerTag;
};
SIZE(0xb0);

#endif // GRUNTZ_CWARLORD_H
