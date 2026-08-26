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

RVA(0x000af090, 0xf4)
i32 DispatchRollingBallLogic(CGameObject* owner){LOGIC_RECORD_DISPATCH(CRollingBall)}

RVA(0x000af1d0, 0xf4)
i32 DispatchSpotLightLogic(CGameObject* owner){LOGIC_RECORD_DISPATCH(CSpotLight)}

RVA(0x000af310, 0xf4)
i32 DispatchKitchenSlimeLogic(CGameObject* owner){LOGIC_RECORD_DISPATCH(CKitchenSlime)}

RVA(0x000af450, 0xf4)
i32 DispatchPathHazardLogic(CGameObject* owner){LOGIC_RECORD_DISPATCH(CPathHazard)}

RVA(0x000af590, 0xf4)
i32 DispatchRainCloudLogic(CGameObject* owner){LOGIC_RECORD_DISPATCH(CRainCloud)}

RVA(0x000af6d0, 0xf4)
i32 DispatchUFOLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CUFO)
}
