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
    virtual i32 Vslot06() OVERRIDE;
    virtual i32 InputVirtual() OVERRIDE;
    virtual i32 Vslot09(i32) OVERRIDE;
    virtual i32 FrameSlot28(i32) OVERRIDE;
    virtual i32 Vslot0c(i32, i32) OVERRIDE;
    virtual i32 Vslot0e(i32, i32, i32) OVERRIDE;

    char m_pad1b4[0x1b8 - 0x1b4];
};
SIZE(0x1b8);

extern char g_titleBuf[];
#endif // GRUNTZ_HELPSTATE_H
