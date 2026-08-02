#ifndef GRUNTZ_CGRUNTSELECTEDSPRITE_H
#define GRUNTZ_CGRUNTSELECTEDSPRITE_H

#include <rva.h>

#include <Gruntz/CoordNode.h>
#include <Gruntz/GruntIndicatorSprite.h>

class CGruntSelectedSprite : public CUserLogic, public CWapX {
public:
public:
    virtual i32 SerializeMove(CFileMemBase*, i32, i32, CGameObject*) OVERRIDE;
    RVA(0x00011e30, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_GRUNTSELECTEDSPRITE;
    }
    CGruntSelectedSprite() {}
    CGruntSelectedSprite(CGameObject* obj);

    virtual void FireActivation(i32 id) OVERRIDE;
    static void RegisterActs();

    i32 SetCell(i32 x, i32 y);
    i32 Update();
    Coord m_cell;
};
SIZE_UNKNOWN();

#endif // GRUNTZ_CGRUNTSELECTEDSPRITE_H
