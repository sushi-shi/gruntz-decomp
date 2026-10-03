#include <StdAfx.h>

#include <Ints.h>

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

i32 DispatchAniCycleLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CAniCycle)
}

i32 DispatchSingleFrameMessageLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CSingleFrameMessage)
}

i32 DispatchDoNothingLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CDoNothing)
}

i32 DispatchDoNothingNormalLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CDoNothingNormal)
}

i32 DispatchSimpleAnimationLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CSimpleAnimation)
}

i32 DispatchMenuSparkleLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CMenuSparkle)
}

i32 DispatchFrontCandyLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CFrontCandy)
}

i32 DispatchBehindCandyLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CBehindCandy)
}

i32 DispatchFrontCandyAniLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CFrontCandyAni)
}

i32 DispatchBehindCandyAniLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CBehindCandyAni)
}

i32 DispatchEyeCandyLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CEyeCandy)
}

i32 DispatchEyeCandyAniLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CEyeCandyAni)
}

i32 DispatchWayPointLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CWayPoint)
}

i32 DispatchSingleAnimationLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CSingleAnimation)
}

i32 DispatchGuardPointLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CGuardPoint)
}
