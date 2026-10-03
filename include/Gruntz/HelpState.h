#ifndef GRUNTZ_HELPSTATE_H
#define GRUNTZ_HELPSTATE_H

#include <Ints.h>

#include <Gruntz/GameStateId.h>
#include <Gruntz/State.h>
#include <Ints.h>

class CHelpState : public CState {
public:
    virtual ~CHelpState()  ;

    virtual i32 LoadGameAssetNamespaces(CGruntzMgr*, i32, i32)  ;
    virtual void ReleaseResources()  ;
    virtual GameStateId Update()  ;
    virtual i32 Render()  ;
    virtual i32 RestoreDisplay()  ;
    virtual i32 InputVirtual()  ;
    virtual i32 EnterState(GameStateId previousState)  ;
    virtual i32 LeaveState(GameStateId nextState)  ;
    virtual i32 OnKeyDown(i32, i32)  ;
    virtual i32 OnLButtonDown(i32, i32, i32)  ;

    char m_pad1b4[0x1b8 - 0x1b4];
};

extern char g_titleBuf[];
#endif
