#ifndef GRUNTZ_GRUNTZ_CDEMO_H
#define GRUNTZ_GRUNTZ_CDEMO_H

#include <Ints.h>

#include <Gruntz/GameStateId.h>
#include <Gruntz/Play.h>

class CDemo : public CPlay {
public:
    virtual ~CDemo()  ;

    virtual i32 LoadGameAssetNamespaces(CGruntzMgr*, i32, i32)  ;

    virtual void ReleaseResources()  ;
    virtual GameStateId Update()  ;
    virtual i32 Render()  ;
    virtual i32 CompleteLevel()  ;
    virtual i32 BuildWorldLevelPath(i32)  ;

    i32 m_demoCountdown;
};

#endif
