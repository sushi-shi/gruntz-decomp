#include <rva.h>
#include <Gruntz/RollingBall.h>
#include <Gruntz/SpotLight.h>
#include <Gruntz/KitchenSlime.h>
#include <Gruntz/PathHazard.h>
#include <Gruntz/RainCloud.h>
#include <Gruntz/Ufo.h>

#include <Gruntz/WorkerHandler.h> // shared Worker / Owner archetype + LOGIC_WORKER_PUMP

RVA(0x000af0a0, 0xf4)
i32 HandlerAF0A0(CGameObject* owner){LOGIC_WORKER_PUMP(CRollingBall)}

RVA(0x000af1e0, 0xf4)
i32 HandlerAF1E0(CGameObject* owner){LOGIC_WORKER_PUMP(CSpotLight)}

RVA(0x000af320, 0xf4)
i32 HandlerAF320(CGameObject* owner){LOGIC_WORKER_PUMP(CKitchenSlime)}

RVA(0x000af460, 0xf4)
i32 HandlerAF460(CGameObject* owner){LOGIC_WORKER_PUMP(CPathHazard)}

RVA(0x000af5a0, 0xf4)
i32 HandlerAF5A0(CGameObject* owner){LOGIC_WORKER_PUMP(CRainCloud)}

RVA(0x000af6e0, 0xf4)
i32 HandlerAF6E0(CGameObject* owner) {
    LOGIC_WORKER_PUMP(CUFO)
}
