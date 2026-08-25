#include <rva.h>

#include <Gruntz/GameObjectLogicTypes.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/KitchenSlime.h>
#include <Gruntz/LogicRecordHandler.h>
#include <Gruntz/PathHazard.h>
#include <Gruntz/RainCloud.h>
#include <Gruntz/RollingBall.h>
#include <Gruntz/SpotLight.h>
#include <Gruntz/Ufo.h>

RVA(0x000af0a0, 0xf4)
i32 DispatchRollingBallLogic(CGameObject* owner){LOGIC_RECORD_DISPATCH(CRollingBall)}

RVA(0x000af1e0, 0xf4)
i32 DispatchSpotLightLogic(CGameObject* owner){LOGIC_RECORD_DISPATCH(CSpotLight)}

RVA(0x000af320, 0xf4)
i32 DispatchKitchenSlimeLogic(CGameObject* owner){LOGIC_RECORD_DISPATCH(CKitchenSlime)}

RVA(0x000af460, 0xf4)
i32 DispatchPathHazardLogic(CGameObject* owner){LOGIC_RECORD_DISPATCH(CPathHazard)}

RVA(0x000af5a0, 0xf4)
i32 DispatchRainCloudLogic(CGameObject* owner){LOGIC_RECORD_DISPATCH(CRainCloud)}

RVA(0x000af6e0, 0xf4)
i32 DispatchUFOLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CUFO)
}
