#ifndef GRUNTZ_GRUNTZ_CATTRACT_H
#define GRUNTZ_GRUNTZ_CATTRACT_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/State.h>

#include <DDrawMgr/DDSurface.h>

extern "C" i32 g_attractStateCount;

class CSymParser;

class DirectSoundMgr;

class CGruntzMgr;

class CAttract : public CState {
public:
    virtual i32 LoadGameAssetNamespaces(CGruntzMgr* a, i32 b, i32 mode) OVERRIDE;

    virtual ~CAttract() OVERRIDE;
    virtual void ReleaseResources() OVERRIDE;

    virtual GameStateId Update() OVERRIDE;
    virtual i32 Render() OVERRIDE;
    virtual i32 RestoreDisplay() OVERRIDE;
    virtual i32 OnPaint() OVERRIDE;
    virtual i32 InputVirtual() OVERRIDE;
    virtual i32 EnterState(i32) OVERRIDE;
    virtual i32 LeaveState(i32 arg) OVERRIDE;
    virtual i32 OnKeyDown(i32, i32) OVERRIDE;
    virtual i32 OnLButtonDown(i32, i32, i32) OVERRIDE;

    u32 m_idleTimer;
    struct LeafCue* m_host;
    i32 m_activeFlag;
};
SIZE(0x1c0);
SIZE(0x1c0);

extern i32 g_suppress_64e360;
#endif // GRUNTZ_GRUNTZ_CATTRACT_H
