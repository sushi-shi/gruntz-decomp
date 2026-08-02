#ifndef GRUNTZ_CGRUNTTOYSPRITE_H
#define GRUNTZ_CGRUNTTOYSPRITE_H

#include <rva.h>

#include <Gruntz/GruntIndicatorSprite.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/GruntIndicatorSprite.h>

class CGruntToySprite : public CUserLogic, public CWapX {
public:
public:
    virtual i32 SerializeMove(CFileMemBase*, i32, i32, CGameObject*) OVERRIDE;
    RVA(0x00012260, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_GRUNTTOYSPRITE;
    }
    CGruntToySprite() {}
    CGruntToySprite(CGameObject* obj);

    virtual void FireActivation(i32 id) OVERRIDE;
    static void RegisterActs();

    i32 SetCell(i32 x, i32 y);
    i32 Update();

    Coord m_cell;
    i32 m_lastLayer;
};
SIZE(0x60);

#endif // GRUNTZ_CGRUNTTOYSPRITE_H
