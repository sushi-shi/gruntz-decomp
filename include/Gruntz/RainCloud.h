#ifndef GRUNTZ_CRAINCLOUD_H
#define GRUNTZ_CRAINCLOUD_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/PathHazard.h>
#include <Gruntz/SerialArchive.h>

class CRainCloud : public CPathHazard {
public:
    virtual i32 SerializeMove(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;
    virtual LogicTypeId GetTypeTag() OVERRIDE;
    CRainCloud() {}
    CRainCloud(CGameObject* obj);

    virtual i32 Tick() OVERRIDE;
    virtual i32 HitTest(i32, i32) OVERRIDE;
};

#endif // GRUNTZ_CRAINCLOUD_H
