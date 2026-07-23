// RbEffect.h - RbEffect, the rock-break per-frame effect leaf (Update @0x476b0).
//
// @identity-TODO: the owning class is unrecovered - an orphan COMDAT with no
// caller/new-site/RTTI/vtable-dispatch (all applicable techniques dead-end). It
// drives an effect sprite (m_38, a CGameObject with a CAniAdvanceCursor @+0x1a0)
// over a target CGameObject (m_10). Defined in src/Gruntz/RockBreakEffectUpdate.cpp.
//
// Field names are placeholders; only offsets + code bytes are load-bearing.
#ifndef GRUNTZ_RBEFFECT_H
#define GRUNTZ_RBEFFECT_H

#include <rva.h>
#include <Ints.h>

struct CGameObject; // (struct tag matches <Gruntz/UserLogic.h>)

struct RbEffect {
    char p0[0x10];
    CWwdGameObjectA* m_10; // +0x10  target object
    char p14[0x38 - 0x14];
    CWwdGameObjectA* m_38; // +0x38  effect sprite (a CreateSprite product - the A kind)
    i32 Update();          // 0x476b0
};
SIZE_UNKNOWN();

#endif // GRUNTZ_RBEFFECT_H
