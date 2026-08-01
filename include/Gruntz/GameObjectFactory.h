#ifndef GRUNTZ_GRUNTZ_GAMEOBJECTFACTORY_H
#define GRUNTZ_GRUNTZ_GAMEOBJECTFACTORY_H

#include <Ints.h>

class CDDrawSurfaceMgr;
struct CGameObject;

void RegisterGameObjectTypes(CDDrawSurfaceMgr* ctx);

extern "C" i32 CreateAniCycle(CGameObject* obj);
extern "C" i32 CreateDoNothingNormal(CGameObject* obj);
extern "C" i32 CreateDoNothing(CGameObject* obj);
extern "C" i32 CreateSimpleAnimation(CGameObject* obj);
extern "C" i32 CreateMenuSparkle(CGameObject* obj);
extern "C" i32 CreateFrontCandy(CGameObject* obj);
extern "C" i32 CreateBehindCandy(CGameObject* obj);
extern "C" i32 CreateFrontCandyAni(CGameObject* obj);
extern "C" i32 CreateBehindCandyAni(CGameObject* obj);
extern "C" i32 CreateEyeCandy(CGameObject* obj);
extern "C" i32 CreateEyeCandyAni(CGameObject* obj);
extern "C" i32 CreateGrunt(CGameObject* obj);
extern "C" i32 CreateGlobalAmbientSound(CGameObject* obj);
extern "C" i32 CreateAmbientSound(CGameObject* obj);
extern "C" i32 CreateAmbientPosSound(CGameObject* obj);
extern "C" i32 CreateSpotAmbientSound(CGameObject* obj);
extern "C" i32 CreateActionArea(CGameObject* obj);
extern "C" i32 CreateStatusBarSprite(CGameObject* obj);
extern "C" i32 CreateParticlez(CGameObject* obj);
extern "C" i32 CreateExplosion(CGameObject* obj);
extern "C" i32 CreateGruntSelectedSprite(CGameObject* obj);
extern "C" i32 CreateGruntHealthSprite(CGameObject* obj);
extern "C" i32 CreateGruntStaminaSprite(CGameObject* obj);
extern "C" i32 CreateGruntToySprite(CGameObject* obj);
extern "C" i32 CreateGruntToyTimeSprite(CGameObject* obj);
extern "C" i32 CreateGruntWingzTimeSprite(CGameObject* obj);
extern "C" i32 CreateGruntPowerupSprite(CGameObject* obj);
extern "C" i32 CreateToyPeek(CGameObject* obj);
extern "C" i32 CreateTileTriggerSwitch(CGameObject* obj);
extern "C" i32 CreateTileTrigger(CGameObject* obj);
extern "C" i32 CreateTileSecretTrigger(CGameObject* obj);
extern "C" i32 CreateBrickz(CGameObject* obj);
extern "C" i32 CreateTileTriggerTransition(CGameObject* obj);
extern "C" i32 CreateGruntStartingPoint(CGameObject* obj);
extern "C" i32 CreateGruntCreationPoint(CGameObject* obj);
extern "C" i32 CreateFortressFlag(CGameObject* obj);
extern "C" i32 CreateExitTrigger(CGameObject* obj);
extern "C" i32 CreateGiantRock(CGameObject* obj);
extern "C" i32 CreateCoveredPowerup(CGameObject* obj);
extern "C" i32 CreateInGameIcon(CGameObject* obj);
extern "C" i32 CreateInGameText(CGameObject* obj);
extern "C" i32 CreateWormhole(CGameObject* obj);
extern "C" i32 CreateGruntPuddle(CGameObject* obj);
extern "C" i32 CreateRollingBall(CGameObject* obj);
extern "C" i32 CreateObjectDropper(CGameObject* obj);
extern "C" i32 CreateDroppedObject(CGameObject* obj);
extern "C" i32 CreateDroppedObjectShadow(CGameObject* obj);
extern "C" i32 CreateCheckpointTrigger(CGameObject* obj);
extern "C" i32 CreateTeleporter(CGameObject* obj);
extern "C" i32 CreateSecretTeleporterTrigger(CGameObject* obj);
extern "C" i32 CreateSecretLevelTrigger(CGameObject* obj);
extern "C" i32 CreateProjectile(CGameObject* obj);
extern "C" i32 CreateBoomerang(CGameObject* obj);
extern "C" i32 CreateStaticHazard(CGameObject* obj);
extern "C" i32 CreateToobSpikez(CGameObject* obj);
extern "C" i32 CreateTimeBomb(CGameObject* obj);
extern "C" i32 CreateSpotLight(CGameObject* obj);
extern "C" i32 CreateKitchenSlime(CGameObject* obj);
extern "C" i32 CreateSingleAnimation(CGameObject* obj);
extern "C" i32 CreateWayPoint(CGameObject* obj);
extern "C" i32 CreateWarlord(CGameObject* obj);
extern "C" i32 CreatePathHazard(CGameObject* obj);
extern "C" i32 CreateRainCloud(CGameObject* obj);
extern "C" i32 CreateUFO(CGameObject* obj);
extern "C" i32 CreateGruntVoice(CGameObject* obj);
extern "C" i32 CreateWarpStonePad(CGameObject* obj);
extern "C" i32 CreateGuardPoint(CGameObject* obj);
extern "C" i32 CreateVoiceTrigger(CGameObject* obj);
extern "C" i32 CreateLevelTime(CGameObject* obj);
extern "C" i32 CreateCursorSnapSprite(CGameObject* obj);
extern "C" i32 CreateLightFx(CGameObject* obj);
extern "C" i32 CreateDemoMover(CGameObject* obj);
extern "C" i32 CreateDemoSign(CGameObject* obj);

extern "C" i32 LogicHitFactory(CGameObject* obj);
extern "C" i32 LogicAttackFactory(CGameObject* obj);
extern "C" i32 LogicBumpFactory(CGameObject* obj);

#endif // GRUNTZ_GRUNTZ_GAMEOBJECTFACTORY_H
