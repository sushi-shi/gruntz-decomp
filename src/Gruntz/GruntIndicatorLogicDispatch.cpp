#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/GameObjectLogicTypes.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntHealthSprite.h>
#include <Gruntz/GruntPowerupSprite.h>
#include <Gruntz/GruntSelectedSprite.h>
#include <Gruntz/GruntStaminaSprite.h>
#include <Gruntz/GruntToySprite.h>
#include <Gruntz/GruntToyTimeSprite.h>
#include <Gruntz/GruntWingzTimeSprite.h>
#include <Gruntz/LogicEventDispatch.h>
#include <Gruntz/LogicRecordHandler.h>
#include <Gruntz/UserLogic.h>
#include <Wwd/LogicRecordEvent.h>

i32 DispatchGruntSelectedSpriteLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CGruntSelectedSprite)
}

i32 DispatchGruntHealthSpriteLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CGruntHealthSprite)
}

i32 DispatchGruntToySpriteLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CGruntToySprite)
}

i32 DispatchGruntStaminaSpriteLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CGruntStaminaSprite)
}

i32 DispatchGruntToyTimeSpriteLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CGruntToyTimeSprite)
}

i32 DispatchGruntWingzTimeSpriteLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CGruntWingzTimeSprite)
}

i32 DispatchGruntPowerupSpriteLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CGruntPowerupSprite)
}
