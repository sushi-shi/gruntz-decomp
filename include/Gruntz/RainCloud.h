#ifndef GRUNTZ_CRAINCLOUD_H
#define GRUNTZ_CRAINCLOUD_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/PathHazard.h>
#include <Gruntz/SerialArchive.h>

class CRainCloud : public CPathHazard {
public:
    virtual i32 SerializeMove(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;
    RVA(0x000132f0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_RAINCLOUD;
    }
    CRainCloud() {}
    CRainCloud(CGameObject* obj);

    virtual i32 Tick() OVERRIDE;
    virtual i32 HitTest(i32 playerIndex, i32 unitIndex) OVERRIDE;
};

#endif // GRUNTZ_CRAINCLOUD_H
