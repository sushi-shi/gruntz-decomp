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
    virtual i32 Vslot15() OVERRIDE;
    virtual i32 BuildWorldLevelPath(i32) OVERRIDE;

    char m_pad51c[0x520 - 0x51c];
    i32 m_520;
    i32 m_524;
};
SIZE_UNKNOWN();

extern "C" const i32 g_rotTableA_60d008[27];
extern "C" const i32 g_rotTableB_60d078[27];

#endif // GRUNTZ_GRUNTZ_CDEMO_H
