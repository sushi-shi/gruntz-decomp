#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/GameObjectLogicTypes.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/KitchenSlime.h>
#include <Gruntz/LogicRecordHandler.h>
#include <Gruntz/PathHazard.h>
#include <Gruntz/RainCloud.h>
#include <Gruntz/RollingBall.h>
#include <Gruntz/SpotLight.h>
#include <Gruntz/Ufo.h>

i32 DispatchRollingBallLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CRollingBall)
}

i32 DispatchSpotLightLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CSpotLight)
}

i32 DispatchKitchenSlimeLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CKitchenSlime)
}

i32 DispatchPathHazardLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CPathHazard)
}

i32 DispatchRainCloudLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CRainCloud)
}

i32 DispatchUFOLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CUFO)
}
