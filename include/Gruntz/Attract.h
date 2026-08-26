#ifndef GRUNTZ_GRUNTZ_CATTRACT_H
#define GRUNTZ_GRUNTZ_CATTRACT_H

#include <rva.h>

#include <DDrawMgr/DDSurface.h>
#include <Gruntz/GameStateId.h>
#include <Gruntz/State.h>
#include <Ints.h>

class CRezArchive;
struct SoundCue;

class CGruntzMgr;

class CAttract : public CState {
public:
    virtual i32 LoadGameAssetNamespaces(CGruntzMgr* mgr, i32 areaArg, i32 prevStateId) OVERRIDE;

    virtual ~CAttract() OVERRIDE;
    virtual void ReleaseResources() OVERRIDE;

    RVA(0x0008cc60, 0x6)
    virtual GameStateId Update() OVERRIDE {
        return GAMESTATE_ATTRACT;
    }
    virtual i32 Render() OVERRIDE;
    virtual i32 RestoreDisplay() OVERRIDE;
    virtual i32 OnPaint() OVERRIDE;
    virtual i32 InputVirtual() OVERRIDE;
    virtual i32 EnterState(GameStateId previousState) OVERRIDE;
    virtual i32 LeaveState(GameStateId nextState) OVERRIDE;
    virtual i32 OnKeyDown(i32, i32) OVERRIDE;
    virtual i32 OnLButtonDown(i32, i32, i32) OVERRIDE;

    u32 m_titleCountdownMs;
    SoundCue* m_titleCue;
    b32 m_titleCueEnabled;
};

extern b32 g_skipNextScreenEffect;
#endif // GRUNTZ_GRUNTZ_CATTRACT_H
