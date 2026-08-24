#ifndef GRUNTZ_SPLASHSTATE_H
#define GRUNTZ_SPLASHSTATE_H

#include <rva.h>

#include <Gruntz/GameStateId.h>
#include <Gruntz/State.h>

class CSplashState : public CState {
public:
    CSplashState() {
        m_reserved1b4 = 0;
    }

    virtual ~CSplashState() OVERRIDE;

    virtual i32 LoadGameAssetNamespaces(CGruntzMgr* mgr, i32 areaArg, i32 prevStateId) OVERRIDE;
    virtual void ReleaseResources() OVERRIDE;

    virtual GameStateId Update() OVERRIDE;
    virtual i32 Render() OVERRIDE;
    virtual i32 RestoreDisplay() OVERRIDE;
    virtual i32 InputVirtual() OVERRIDE;
    virtual i32 EnterState(GameStateId) OVERRIDE;
    virtual i32 LeaveState(GameStateId) OVERRIDE;
    virtual i32 OnKeyDown(i32, i32) OVERRIDE;
    virtual i32 OnLButtonDown(i32, i32, i32) OVERRIDE;

    i32 m_reserved1b4;
    i32 m_splashCountdownMs;
};

#endif // GRUNTZ_SPLASHSTATE_H
