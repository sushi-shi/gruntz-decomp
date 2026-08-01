#ifndef GRUNTZ_SPLASHSTATE_H
#define GRUNTZ_SPLASHSTATE_H

#include <Gruntz/State.h>
#include <rva.h>

class CSplashState : public CState {
public:
    CSplashState() {
        m_1b4 = 0;
    }

    virtual ~CSplashState() OVERRIDE;

    virtual i32 LoadGameAssetNamespaces(CGruntzMgr* a, i32 b, i32 c) OVERRIDE;
    virtual void ReleaseResources() OVERRIDE;

    virtual GameStateId Update() OVERRIDE;
    virtual i32 Render() OVERRIDE;
    virtual i32 RestoreDisplay() OVERRIDE;
    virtual i32 InputVirtual() OVERRIDE;
    virtual i32 EnterState(i32) OVERRIDE;
    virtual i32 LeaveState(i32) OVERRIDE;
    virtual i32 OnKeyDown(i32, i32) OVERRIDE;
    virtual i32 OnLButtonDown(i32, i32, i32) OVERRIDE;

    i32 m_1b4;
    i32 m_1b8;
};
SIZE_UNKNOWN();
SIZE_UNKNOWN();

#endif // GRUNTZ_SPLASHSTATE_H
