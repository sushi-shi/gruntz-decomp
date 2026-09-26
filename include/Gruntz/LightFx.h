#ifndef GRUNTZ_GRUNTZ_CLIGHTFX_H
#define GRUNTZ_GRUNTZ_CLIGHTFX_H

#include <rva.h>

#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/LogicRecord.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>
#include <Ints.h>
#include <Wwd/WwdGameObjectFamily.h>

class CLightFx : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;
    RVA(0x000123e0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_LIGHTFX;
    }

public:
    CLightFx() {}
    CLightFx(CGameObject* obj);

    virtual void FireActivation(i32 id) OVERRIDE;

    static void RegisterActs();

    i32 AdvanceAnim();

    void Activate(
        const char* imageSetName,
        const char* animationName,
        i32 shadeTableIndex,
        b32 deleteWhenComplete
    );

    i32 RebindNode();

    i32 m_shadeTableIndex;
    b32 m_deleteWhenComplete;
};

inline void CreateLightFx(
    CDDrawChildGroup* group,
    i32 x,
    i32 y,
    i32 sortKey,
    const char* imageSetName,
    const char* animationName,
    i32 shadeTableIndex,
    b32 deleteWhenComplete
) {
    CWwdSpriteObject* sprite =
        group->CreateSprite(0, x, y, sortKey, "LightFx", WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE);
    sprite->m_logicRecord->m_dispatch(sprite);
    static_cast<CLightFx*>(sprite->m_logicRecord->m_userLogic)
        ->Activate(imageSetName, animationName, shadeTableIndex, deleteWhenComplete);
}

#endif // GRUNTZ_GRUNTZ_CLIGHTFX_H
