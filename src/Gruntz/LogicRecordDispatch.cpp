#include <rva.h>

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

RVA(0x000de8a0, 0xf4)
i32 DispatchProjectileLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CProjectile)
}

RVA(0x000de9e0, 0xf4)
i32 DispatchBoomerangLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CBoomerang)
}

RVA(0x000deb20, 0xf1)
i32 DispatchTimeBombLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CTimeBomb)
}
