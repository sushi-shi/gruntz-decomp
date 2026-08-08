#ifndef GRUNTZ_BOOTYSTATEACTIVATE_H
#define GRUNTZ_BOOTYSTATEACTIVATE_H

#include <rva.h>

#include <Mfc.h>

#include <Enums.h>
#include <Ints.h>

class CDDrawSurfaceMgr;

GZ_ENUM_BEGIN(SecretBonusTier)
    SECRET_BONUS_TIER_ONE = 1,
    SECRET_BONUS_TIER_TWO = 2,
    SECRET_BONUS_TIER_THREE = 3
GZ_ENUM_END(SecretBonusTier)

GZ_ENUM_CONST_BEGIN(BootyEffectCount)
    BOOTY_EXPLOSION_COUNT = 8
GZ_ENUM_CONST_END(BootyEffectCount)

i32 ShowHudMessageAlt(
    CDDrawSurfaceMgr* sink,
    CString* text,
    RECT* box,
    i32 fontSel,
    i32 b,
    i32 c,
    i32 d,
    i32 e,
    i32 f
);

#endif // GRUNTZ_BOOTYSTATEACTIVATE_H
