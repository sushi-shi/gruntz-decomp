#ifndef GRUNTZ_CPARTICLEZ_H
#define GRUNTZ_CPARTICLEZ_H

#include <rva.h>

#include <DDrawMgr/DDrawChildGroup.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/UserLogic.h>
#include <Wwd/WwdGameObjectFamily.h>

class CParticlez : public CUserLogic, public CWapX {
public:
public:
    CParticlez() {}
    CParticlez(CGameObject* obj);

    RVA(0x00012cd0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_PARTICLEZ;
    }
    RVA(0x00012cf0, 0x47)
    virtual i32
    SerializeDispatch(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, CGameObject* object)
        OVERRIDE {
        SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE(ar, mode, typeId, object)
    }
    virtual void FireActivation(i32 id) OVERRIDE;

    static void RegisterActs();
    i32 Update();
};

inline void CreateParticlez(
    CDDrawChildGroup* group,
    i32 x,
    i32 y,
    const char* imageSetName,
    const char* animationName
) {
    CWwdSpriteObject* sprite = group->CreateSprite(
        0,
        x,
        y,
        SORTKEY_ACTOR_BEHIND,
        "Particlez",
        WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
    );
    if (sprite != NULL) {
        sprite->SetImageSetByName(imageSetName);
        sprite->SetAnimationByName(animationName, 0);
    }
}

#endif // GRUNTZ_CPARTICLEZ_H
