#ifndef GRUNTZ_BOOTYSTATEACTIVATE_H
#define GRUNTZ_BOOTYSTATEACTIVATE_H

#include <rva.h>

#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Enums.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Ints.h>
#include <Wwd/WwdGameObjectFlags.h>

class CDDrawSurfaceMgr;

GZ_ENUM_BEGIN(SecretBonusTier)
    SECRET_BONUS_TIER_ONE = 1,
    SECRET_BONUS_TIER_TWO = 2,
    SECRET_BONUS_TIER_THREE = 3
GZ_ENUM_END(SecretBonusTier)

GZ_ENUM_CONST_BEGIN(BootyEffectCount)
    BOOTY_EXPLOSION_COUNT = 8
GZ_ENUM_CONST_END(BootyEffectCount)

i32 DrawTextToBackSurface(
    CDDrawSurfaceMgr* surfaceMgr,
    CString* text,
    RECT* box,
    i32 fontSel,
    i32 shadow,
    i32 r,
    i32 g,
    i32 b,
    i32 flag
);

inline CWwdSpriteObject* CreateSimpleAnimationSprite(i32 sortKey) {
    return g_gameReg->m_world->m_childGroup->CreateSprite(
        0,
        0,
        0,
        sortKey,
        "SimpleAnimation",
        WWD_GAME_OBJECT_FLAGS_SKIP_COLLISION_KEEP_ACTIVE
    );
}

#endif // GRUNTZ_BOOTYSTATEACTIVATE_H
