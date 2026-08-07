#ifndef GRUNTZ_GRUNTZ_CDEMO_H
#define GRUNTZ_GRUNTZ_CDEMO_H

#include <rva.h>

#include <Gruntz/GameStateId.h>
#include <Gruntz/Play.h>

class CDemo : public CPlay {
public:
    CDemo() {}
    virtual ~CDemo() OVERRIDE;

    virtual i32 LoadGameAssetNamespaces(CGruntzMgr*, i32, i32) OVERRIDE;

    virtual void ReleaseResources() OVERRIDE;
    virtual GameStateId Update() OVERRIDE;
    virtual i32 Render() OVERRIDE;
    virtual i32 CompleteLevel() OVERRIDE;
    virtual i32 BuildWorldLevelPath(i32) OVERRIDE;

    i32 m_demoCountdown;
    i32 m_reserved524;
};
SIZE(0x528);

#endif // GRUNTZ_GRUNTZ_CDEMO_H
