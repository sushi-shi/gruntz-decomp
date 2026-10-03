#ifndef GRUNTZ_CRAINCLOUD_H
#define GRUNTZ_CRAINCLOUD_H

#include <Ints.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/PathHazard.h>
#include <Gruntz/SerialArchive.h>

class CRainCloud : public CPathHazard {
public:
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*)  ;

    virtual LogicTypeId GetTypeTag()   {
        return LOGIC_RAINCLOUD;
    }
    CRainCloud() {}
    CRainCloud(CGameObject* obj);

    virtual i32 Tick()  ;
    virtual i32 HitTest(i32 playerIndex, i32 unitIndex)  ;
};

#endif
