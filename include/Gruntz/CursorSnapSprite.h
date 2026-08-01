#ifndef GRUNTZ_CCURSORSNAPSPRITE_H
#define GRUNTZ_CCURSORSNAPSPRITE_H

#include <rva.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CCursorSnapSprite : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CFileMemBase*, i32, i32, CGameObject*) OVERRIDE;
    RVA(0x00011860, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_CURSORSNAPSPRITE;
    }

public:
    CCursorSnapSprite() {}
    CCursorSnapSprite(CGameObject* obj);

    virtual void FireActivation(i32 id) OVERRIDE;

    i32 AdvanceAnim();
};
SIZE(0x54);

SIZE_UNKNOWN();

#endif // GRUNTZ_CCURSORSNAPSPRITE_H
