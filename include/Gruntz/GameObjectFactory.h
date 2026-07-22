#ifndef GRUNTZ_GRUNTZ_GAMEOBJECTFACTORY_H
#define GRUNTZ_GRUNTZ_GAMEOBJECTFACTORY_H

#include <Ints.h>

class CDDrawSurfaceMgr; // the world/surface manager (<DDrawMgr/DDrawSurfaceMgr.h>)
struct CGameObject;     // the registrants' arg (<Wwd/WwdGameObjectFamily.h>)

void RegisterGameObjectTypes(CDDrawSurfaceMgr* ctx); // 0x00a3b0


// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---
extern "C" i32 CreateAniCycle(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateDoNothingNormal(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateDoNothing(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateSimpleAnimation(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateMenuSparkle(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateFrontCandy(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateBehindCandy(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateFrontCandyAni(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateBehindCandyAni(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateEyeCandy(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateEyeCandyAni(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateGrunt(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateGlobalAmbientSound(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateAmbientSound(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateAmbientPosSound(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateSpotAmbientSound(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateActionArea(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateStatusBarSprite(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateParticlez(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateExplosion(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateGruntSelectedSprite(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateGruntHealthSprite(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateGruntStaminaSprite(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateGruntToySprite(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateGruntToyTimeSprite(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateGruntWingzTimeSprite(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateGruntPowerupSprite(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateToyPeek(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateTileTriggerSwitch(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateTileTrigger(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateTileSecretTrigger(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateBrickz(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateTileTriggerTransition(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateGruntStartingPoint(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateGruntCreationPoint(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateFortressFlag(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateExitTrigger(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateGiantRock(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateCoveredPowerup(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateInGameIcon(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateInGameText(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateWormhole(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateGruntPuddle(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateRollingBall(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateObjectDropper(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateDroppedObject(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateDroppedObjectShadow(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateCheckpointTrigger(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateTeleporter(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateSecretTeleporterTrigger(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateSecretLevelTrigger(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateProjectile(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateBoomerang(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateStaticHazard(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateToobSpikez(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateTimeBomb(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateSpotLight(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateKitchenSlime(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateSingleAnimation(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateWayPoint(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateWarlord(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreatePathHazard(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateRainCloud(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateUFO(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateGruntVoice(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateWarpStonePad(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateGuardPoint(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateVoiceTrigger(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateLevelTime(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateCursorSnapSprite(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateLightFx(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateDemoMover(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
extern "C" i32 CreateDemoSign(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    // (The five remaining follow-up registrars are now bound as real static-member
    // calls via <Gruntz/ObjTypeRegistrars.h>: their home TUs declare them `static`
    // (SAXXZ), so the retail this-less `call rel32` binds to the exact RVA without a
    // `mov ecx`. See the call sites below - CProjActObj::RegisterType (0x8240),
    // CTileTriggerTransition::RegisterActs (0x10fe70), CToobSpikez::RegisterActs
    // (0x1149c0), CTimeBomb::RegisterActs (0xe1990), CVoiceTrigger::RegisterActs
    // (0x11a500).)

extern "C" i32 LogicHitFactory(CGameObject* obj);    // GameObjNotifyFn ABI
extern "C" i32 LogicAttackFactory(CGameObject* obj); // GameObjNotifyFn ABI
extern "C" i32 LogicBumpFactory(CGameObject* obj);   // GameObjNotifyFn ABI

#endif // GRUNTZ_GRUNTZ_GAMEOBJECTFACTORY_H
