#ifndef GRUNTZ_CRAINCLOUD_H
#define GRUNTZ_CRAINCLOUD_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/PathHazard.h>

class CRainCloud : public CPathHazard {
public:
    virtual i32 SerializeMove(CFileMemBase*, i32, i32, CGameObject*) OVERRIDE;
    virtual LogicTypeId GetTypeTag() OVERRIDE;
    CRainCloud() {}
    CRainCloud(CGameObject* obj);

    virtual i32 Tick() OVERRIDE;
    virtual i32 HitTest(i32, i32) OVERRIDE;
};
SIZE(0x130);

#endif // GRUNTZ_CRAINCLOUD_H
