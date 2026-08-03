#define USERLOGIC_OOL_CTOR

#include <rva.h>

#include <Gruntz/AniCycle.h>
#include <Gruntz/BehindCandy.h>
#include <Gruntz/BehindCandyAni.h>
#include <Gruntz/DoNothing.h>
#include <Gruntz/DoNothingNormal.h>
#include <Gruntz/EyeCandy.h>
#include <Gruntz/EyeCandyAni.h>
#include <Gruntz/FrontCandy.h>
#include <Gruntz/FrontCandyAni.h>
#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/GuardPoint.h>
#include <Gruntz/MenuSparkle.h>
#include <Gruntz/SimpleAnimation.h>
#include <Gruntz/SingleAnimation.h>
#include <Gruntz/SingleFrameMessage.h>
#include <Gruntz/WayPoint.h>
#include <Gruntz/WorkerHandler.h>

RVA(0x000a9a40, 0xf1)
i32 CreateAniCycle(CGameObject* owner){LOGIC_WORKER_PUMP(CAniCycle)}

RVA(0x000a9b80, 0xf1)
i32 CreateSingleFrameMessage(CGameObject* owner){LOGIC_WORKER_PUMP(CSingleFrameMessage)}

RVA(0x000a9cc0, 0xf1)
i32 CreateDoNothing(CGameObject* owner){LOGIC_WORKER_PUMP(CDoNothing)}

// @early-stop
RVA(0x000a9e00, 0x10c)
i32 CreateDoNothingNormal(CGameObject* owner){LOGIC_WORKER_PUMP(CDoNothingNormal)}

RVA(0x000a9f60, 0xf1)
i32 CreateSimpleAnimation(CGameObject* owner){LOGIC_WORKER_PUMP(CSimpleAnimation)}

RVA(0x000aa0a0, 0xf1)
i32 CreateMenuSparkle(CGameObject* owner){LOGIC_WORKER_PUMP(CMenuSparkle)}

RVA(0x000aa1e0, 0xf1)
i32 CreateFrontCandy(CGameObject* owner){LOGIC_WORKER_PUMP(CFrontCandy)}

RVA(0x000aa320, 0xf1)
i32 CreateBehindCandy(CGameObject* owner){LOGIC_WORKER_PUMP(CBehindCandy)}

RVA(0x000aa460, 0xf1)
i32 CreateFrontCandyAni(CGameObject* owner){LOGIC_WORKER_PUMP(CFrontCandyAni)}

RVA(0x000aa5a0, 0xf1)
i32 CreateBehindCandyAni(CGameObject* owner){LOGIC_WORKER_PUMP(CBehindCandyAni)}

RVA(0x000aa6e0, 0xf1)
i32 CreateEyeCandy(CGameObject* owner){LOGIC_WORKER_PUMP(CEyeCandy)}

RVA(0x000aa820, 0xf1)
i32 CreateEyeCandyAni(CGameObject* owner){LOGIC_WORKER_PUMP(CEyeCandyAni)}

RVA(0x000aa960, 0xf1)
i32 CreateWayPoint(CGameObject* owner){LOGIC_WORKER_PUMP(CWayPoint)}

RVA(0x000aaaa0, 0xf1)
i32 CreateSingleAnimation(CGameObject* owner){LOGIC_WORKER_PUMP(CSingleAnimation)}

RVA(0x000aabe0, 0xf1)
i32 CreateGuardPoint(CGameObject* owner) {
    LOGIC_WORKER_PUMP(CGuardPoint)
}
