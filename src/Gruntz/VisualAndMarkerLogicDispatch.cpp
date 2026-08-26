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
#include <Gruntz/GameObjectLogicTypes.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GuardPoint.h>
#include <Gruntz/LogicRecordHandler.h>
#include <Gruntz/MenuSparkle.h>
#include <Gruntz/SimpleAnimation.h>
#include <Gruntz/SingleAnimation.h>
#include <Gruntz/SingleFrameMessage.h>
#include <Gruntz/WayPoint.h>

RVA(0x000a9a30, 0xf1)
i32 DispatchAniCycleLogic(CGameObject* owner){LOGIC_RECORD_DISPATCH(CAniCycle)}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000a9b70, 0xf1)
i32 DispatchSingleFrameMessageLogic(CGameObject* owner){LOGIC_RECORD_DISPATCH(CSingleFrameMessage)}

RVA(0x000a9cb0, 0xf1)
i32 DispatchDoNothingLogic(CGameObject* owner){LOGIC_RECORD_DISPATCH(CDoNothing)}

RVA(0x000a9df0, 0x10c)
i32 DispatchDoNothingNormalLogic(CGameObject* owner){LOGIC_RECORD_DISPATCH(CDoNothingNormal)}

RVA(0x000a9f50, 0xf1)
i32 DispatchSimpleAnimationLogic(CGameObject* owner){LOGIC_RECORD_DISPATCH(CSimpleAnimation)}

RVA(0x000aa090, 0xf1)
i32 DispatchMenuSparkleLogic(CGameObject* owner){LOGIC_RECORD_DISPATCH(CMenuSparkle)}

RVA(0x000aa1d0, 0xf1)
i32 DispatchFrontCandyLogic(CGameObject* owner){LOGIC_RECORD_DISPATCH(CFrontCandy)}

RVA(0x000aa310, 0xf1)
i32 DispatchBehindCandyLogic(CGameObject* owner){LOGIC_RECORD_DISPATCH(CBehindCandy)}

RVA(0x000aa450, 0xf1)
i32 DispatchFrontCandyAniLogic(CGameObject* owner){LOGIC_RECORD_DISPATCH(CFrontCandyAni)}

RVA(0x000aa590, 0xf1)
i32 DispatchBehindCandyAniLogic(CGameObject* owner){LOGIC_RECORD_DISPATCH(CBehindCandyAni)}

RVA(0x000aa6d0, 0xf1)
i32 DispatchEyeCandyLogic(CGameObject* owner){LOGIC_RECORD_DISPATCH(CEyeCandy)}

RVA(0x000aa810, 0xf1)
i32 DispatchEyeCandyAniLogic(CGameObject* owner){LOGIC_RECORD_DISPATCH(CEyeCandyAni)}

RVA(0x000aa950, 0xf1)
i32 DispatchWayPointLogic(CGameObject* owner){LOGIC_RECORD_DISPATCH(CWayPoint)}

RVA(0x000aaa90, 0xf1)
i32 DispatchSingleAnimationLogic(CGameObject* owner){LOGIC_RECORD_DISPATCH(CSingleAnimation)}

RVA(0x000aabd0, 0xf1)
i32 DispatchGuardPointLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CGuardPoint)
}
