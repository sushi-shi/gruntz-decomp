#ifndef GRUNTZ_GRUNTZ_CATTRACT_H
#define GRUNTZ_GRUNTZ_CATTRACT_H

#include <Ints.h>

#include <DDrawMgr/DDSurface.h>
#include <Gruntz/GameStateId.h>
#include <Gruntz/State.h>
#include <Ints.h>

class CRezMgr;
struct SoundCue;

class CGruntzMgr;

class CAttract : public CState {
public:
    virtual i32 LoadGameAssetNamespaces(CGruntzMgr* mgr, i32 areaArg, i32 prevStateId)  ;

    virtual ~CAttract()  ;
    virtual void ReleaseResources()  ;

    virtual GameStateId Update()   {
        return GAMESTATE_ATTRACT;
    }
    virtual i32 Render()  ;
    virtual i32 RestoreDisplay()  ;
    virtual i32 OnPaint()  ;
    virtual i32 InputVirtual()  ;
    virtual i32 EnterState(GameStateId previousState)  ;
    virtual i32 LeaveState(GameStateId nextState)  ;
    virtual i32 OnKeyDown(i32, i32)  ;
    virtual i32 OnLButtonDown(i32, i32, i32)  ;

    u32 m_titleCountdownMs;
    SoundCue* m_titleCue;
    b32 m_titleCueEnabled;
};

extern b32 g_skipNextScreenEffect;
#endif
