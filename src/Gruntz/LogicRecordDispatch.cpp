#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/Boomerang.h>
#include <Gruntz/GameObjectLogicTypes.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/LogicEventDispatch.h>
#include <Gruntz/LogicRecordHandler.h>
#include <Gruntz/Projectile.h>
#include <Gruntz/StaticHazard.h>
#include <Gruntz/TimeBomb.h>
#include <Gruntz/UserLogic.h>
#include <Ints.h>

i32 DispatchProjectileLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CProjectile)
}

i32 DispatchBoomerangLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CBoomerang)
}

i32 DispatchTimeBombLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CTimeBomb)
}
