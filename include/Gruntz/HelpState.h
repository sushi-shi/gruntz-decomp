#ifndef GRUNTZ_HELPSTATE_H
#define GRUNTZ_HELPSTATE_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/State.h>

class CHelpState : public CState {
public:
    virtual ~CHelpState() OVERRIDE;

    virtual i32 LoadGameAssetNamespaces(CGruntzMgr*, i32, i32) OVERRIDE;
    virtual void ReleaseResources() OVERRIDE;
    virtual GameStateId Update() OVERRIDE;
    virtual i32 Render() OVERRIDE;
    virtual i32 RestoreDisplay() OVERRIDE;
    virtual i32 InputVirtual() OVERRIDE;
    virtual i32 EnterState(i32) OVERRIDE;
    virtual i32 LeaveState(i32) OVERRIDE;
    virtual i32 OnKeyDown(i32, i32) OVERRIDE;
    virtual i32 OnLButtonDown(i32, i32, i32) OVERRIDE;

    char m_pad1b4[0x1b8 - 0x1b4];
};
SIZE(0x1b8);

extern char g_titleBuf[];
#endif // GRUNTZ_HELPSTATE_H
