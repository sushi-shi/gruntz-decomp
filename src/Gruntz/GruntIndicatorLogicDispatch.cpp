#include <StdAfx.h>

#include <rva.h>

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

RVA(0x0007db20, 0xf1)
i32 DispatchGruntSelectedSpriteLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CGruntSelectedSprite)
}

RVA(0x0007dc60, 0xf1)
i32 DispatchGruntHealthSpriteLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CGruntHealthSprite)
}

RVA(0x0007dda0, 0xf1)
i32 DispatchGruntToySpriteLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CGruntToySprite)
}

RVA(0x0007dee0, 0xf1)
i32 DispatchGruntStaminaSpriteLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CGruntStaminaSprite)
}

RVA(0x0007e020, 0xf1)
i32 DispatchGruntToyTimeSpriteLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CGruntToyTimeSprite)
}

RVA(0x0007e160, 0xf1)
i32 DispatchGruntWingzTimeSpriteLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CGruntWingzTimeSprite)
}

RVA(0x0007e2a0, 0xf1)
i32 DispatchGruntPowerupSpriteLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CGruntPowerupSprite)
}
