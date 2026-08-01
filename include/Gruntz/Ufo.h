#ifndef GRUNTZ_CUFO_H
#define GRUNTZ_CUFO_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/PathHazard.h>

class CUFO : public CPathHazard {
public:
    virtual i32 SerializeMove(CFileMemBase*, i32, i32, CGameObject*) OVERRIDE;

    RVA(0x000133b0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_UFO;
    }
    CUFO() {}
    CUFO(CGameObject* obj);

    virtual i32 Tick() OVERRIDE;
};
SIZE(0x130);

#endif // GRUNTZ_CUFO_H
