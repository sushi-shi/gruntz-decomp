#ifndef GRUNTZ_CGRUNTHEALTHSPRITE_H
#define GRUNTZ_CGRUNTHEALTHSPRITE_H

#include <rva.h>

#include <Gruntz/GruntIdentity.h>
#include <Gruntz/GruntIndicatorSprite.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CGrunt;

class CGruntHealthSprite : public CUserLogic, public CWapX {
public:
public:
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;
    RVA(0x00011f60, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_GRUNTHEALTHSPRITE;
    }
    virtual void FireActivation(i32 id) OVERRIDE;
    static void RegisterActs();

    i32 HealthUpdate();
    i32 BindToGrunt(i32 playerIndex, i32 unitIndex, i32 displayedValue);

    virtual i32 GetDisplayedValue(CGrunt* grunt);
    CGruntHealthSprite();
    CGruntHealthSprite(CUserLogic::EInlineBase) {}
    CGruntHealthSprite(CGameObject* obj);

    GruntIdentity m_gruntIdentity;
    i32 m_displayedValue;
    i32 m_yOffset;
};

#endif // GRUNTZ_CGRUNTHEALTHSPRITE_H
