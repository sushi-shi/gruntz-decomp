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
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GuardPoint.h>
#include <Gruntz/LogicRecordHandler.h>
#include <Gruntz/MenuSparkle.h>
#include <Gruntz/SimpleAnimation.h>
#include <Gruntz/SingleAnimation.h>
#include <Gruntz/SingleFrameMessage.h>
#include <Gruntz/WayPoint.h>

RVA(0x000a9a40, 0xf1)
i32 CreateAniCycle(CGameObject* owner){LOGIC_RECORD_DISPATCH(CAniCycle)}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000a9b80, 0xf1)
i32 CreateSingleFrameMessage(CGameObject* owner){LOGIC_RECORD_DISPATCH(CSingleFrameMessage)}

RVA(0x000a9cc0, 0xf1)
i32 CreateDoNothing(CGameObject* owner){LOGIC_RECORD_DISPATCH(CDoNothing)}

RVA(0x000a9e00, 0x10c)
i32 CreateDoNothingNormal(CGameObject* owner){LOGIC_RECORD_DISPATCH(CDoNothingNormal)}

RVA(0x000a9f60, 0xf1)
i32 CreateSimpleAnimation(CGameObject* owner){LOGIC_RECORD_DISPATCH(CSimpleAnimation)}

RVA(0x000aa0a0, 0xf1)
i32 CreateMenuSparkle(CGameObject* owner){LOGIC_RECORD_DISPATCH(CMenuSparkle)}

RVA(0x000aa1e0, 0xf1)
i32 CreateFrontCandy(CGameObject* owner){LOGIC_RECORD_DISPATCH(CFrontCandy)}

RVA(0x000aa320, 0xf1)
i32 CreateBehindCandy(CGameObject* owner){LOGIC_RECORD_DISPATCH(CBehindCandy)}

RVA(0x000aa460, 0xf1)
i32 CreateFrontCandyAni(CGameObject* owner){LOGIC_RECORD_DISPATCH(CFrontCandyAni)}

RVA(0x000aa5a0, 0xf1)
i32 CreateBehindCandyAni(CGameObject* owner){LOGIC_RECORD_DISPATCH(CBehindCandyAni)}

RVA(0x000aa6e0, 0xf1)
i32 CreateEyeCandy(CGameObject* owner){LOGIC_RECORD_DISPATCH(CEyeCandy)}

RVA(0x000aa820, 0xf1)
i32 CreateEyeCandyAni(CGameObject* owner){LOGIC_RECORD_DISPATCH(CEyeCandyAni)}

RVA(0x000aa960, 0xf1)
i32 CreateWayPoint(CGameObject* owner){LOGIC_RECORD_DISPATCH(CWayPoint)}

RVA(0x000aaaa0, 0xf1)
i32 CreateSingleAnimation(CGameObject* owner){LOGIC_RECORD_DISPATCH(CSingleAnimation)}

RVA(0x000aabe0, 0xf1)
i32 CreateGuardPoint(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CGuardPoint)
}
