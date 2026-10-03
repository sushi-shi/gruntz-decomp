#ifndef GRUNTZ_SPLASHSTATE_H
#define GRUNTZ_SPLASHSTATE_H

#include <Ints.h>

#include <Gruntz/GameStateId.h>
#include <Gruntz/State.h>

class CSplashState : public CState {
    inline b32 IsAdvanceRequested();

public:
    CSplashState() {
        m_reserved1b4 = 0;
    }

    virtual ~CSplashState()  ;

    virtual i32 LoadGameAssetNamespaces(CGruntzMgr* mgr, i32 areaArg, i32 prevStateId)  ;
    virtual void ReleaseResources()  ;

    virtual GameStateId Update()  ;
    virtual i32 Render()  ;
    virtual i32 RestoreDisplay()  ;
    virtual i32 InputVirtual()  ;
    virtual i32 EnterState(GameStateId previousState)  ;
    virtual i32 LeaveState(GameStateId nextState)  ;
    virtual i32 OnKeyDown(i32, i32)  ;
    virtual i32 OnLButtonDown(i32, i32, i32)  ;

    i32 m_reserved1b4;
    i32 m_splashCountdownMs;
};

#endif
