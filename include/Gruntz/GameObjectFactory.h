#ifndef GRUNTZ_GRUNTZ_GAMEOBJECTFACTORY_H
#define GRUNTZ_GRUNTZ_GAMEOBJECTFACTORY_H

#include <Ints.h>

class CDDrawSurfaceMgr;
struct CGameObject;

void RegisterGameObjectTypes(CDDrawSurfaceMgr* ctx);

i32 CreateAniCycle(CGameObject* obj);
i32 CreateDoNothingNormal(CGameObject* obj);
i32 CreateDoNothing(CGameObject* obj);
i32 CreateSimpleAnimation(CGameObject* obj);
i32 CreateMenuSparkle(CGameObject* obj);
i32 CreateFrontCandy(CGameObject* obj);
i32 CreateBehindCandy(CGameObject* obj);
i32 CreateFrontCandyAni(CGameObject* obj);
i32 CreateBehindCandyAni(CGameObject* obj);
i32 CreateEyeCandy(CGameObject* obj);
i32 CreateEyeCandyAni(CGameObject* obj);
i32 CreateGrunt(CGameObject* obj);
i32 CreateGlobalAmbientSound(CGameObject* obj);
i32 CreateAmbientSound(CGameObject* obj);
i32 CreateAmbientPosSound(CGameObject* obj);
i32 CreateSpotAmbientSound(CGameObject* obj);
i32 CreateActionArea(CGameObject* obj);
i32 CreateStatusBarSprite(CGameObject* obj);
i32 CreateParticlez(CGameObject* obj);
i32 CreateExplosion(CGameObject* obj);
i32 CreateGruntSelectedSprite(CGameObject* obj);
i32 CreateGruntHealthSprite(CGameObject* obj);
i32 CreateGruntStaminaSprite(CGameObject* obj);
i32 CreateGruntToySprite(CGameObject* obj);
i32 CreateGruntToyTimeSprite(CGameObject* obj);
i32 CreateGruntWingzTimeSprite(CGameObject* obj);
i32 CreateGruntPowerupSprite(CGameObject* obj);
i32 CreateToyPeek(CGameObject* obj);
i32 CreateTileTriggerSwitch(CGameObject* obj);
i32 CreateTileTrigger(CGameObject* obj);
i32 CreateTileSecretTrigger(CGameObject* obj);
i32 CreateBrickz(CGameObject* obj);
i32 CreateTileTriggerTransition(CGameObject* obj);
i32 CreateGruntStartingPoint(CGameObject* obj);
i32 CreateGruntCreationPoint(CGameObject* obj);
i32 CreateFortressFlag(CGameObject* obj);
i32 CreateExitTrigger(CGameObject* obj);
i32 CreateGiantRock(CGameObject* obj);
i32 CreateCoveredPowerup(CGameObject* obj);
i32 CreateInGameIcon(CGameObject* obj);
i32 CreateInGameText(CGameObject* obj);
i32 CreateWormhole(CGameObject* obj);
i32 CreateGruntPuddle(CGameObject* obj);
i32 CreateRollingBall(CGameObject* obj);
i32 CreateObjectDropper(CGameObject* obj);
i32 CreateDroppedObject(CGameObject* obj);
i32 CreateDroppedObjectShadow(CGameObject* obj);
i32 CreateCheckpointTrigger(CGameObject* obj);
i32 CreateTeleporter(CGameObject* obj);
i32 CreateSecretTeleporterTrigger(CGameObject* obj);
i32 CreateSecretLevelTrigger(CGameObject* obj);
i32 CreateProjectile(CGameObject* obj);
i32 CreateBoomerang(CGameObject* obj);
i32 CreateToobSpikez(CGameObject* obj);
i32 CreateTimeBomb(CGameObject* obj);
i32 CreateSpotLight(CGameObject* obj);
i32 CreateKitchenSlime(CGameObject* obj);
i32 CreateSingleAnimation(CGameObject* obj);
i32 CreateWayPoint(CGameObject* obj);
i32 CreateWarlord(CGameObject* obj);
i32 CreatePathHazard(CGameObject* obj);
i32 CreateRainCloud(CGameObject* obj);
i32 CreateUFO(CGameObject* obj);
i32 CreateGruntVoice(CGameObject* obj);
i32 CreateWarpStonePad(CGameObject* obj);
i32 CreateGuardPoint(CGameObject* obj);
i32 CreateVoiceTrigger(CGameObject* obj);
i32 CreateLevelTime(CGameObject* obj);
i32 CreateCursorSnapSprite(CGameObject* obj);
i32 CreateLightFx(CGameObject* obj);
i32 CreateDemoMover(CGameObject* obj);
i32 CreateDemoSign(CGameObject* obj);

i32 LogicHitFactory(CGameObject* obj);
i32 LogicAttackFactory(CGameObject* obj);
i32 LogicBumpFactory(CGameObject* obj);

#endif // GRUNTZ_GRUNTZ_GAMEOBJECTFACTORY_H
