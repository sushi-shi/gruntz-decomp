#ifndef GRUNTZ_GRUNTZ_CDEMO_H
#define GRUNTZ_GRUNTZ_CDEMO_H

#include <rva.h>

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

    char m_pad51c[0x520 - 0x51c];
    i32 m_demoCountdown;
    i32 m_reserved524;
};
SIZE(0x528);

extern "C" const i32 g_directionClockwiseTable[27];
extern "C" const i32 g_directionCounterclockwiseTable[27];

#endif // GRUNTZ_GRUNTZ_CDEMO_H
