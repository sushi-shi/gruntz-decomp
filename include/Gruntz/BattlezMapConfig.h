#ifndef SRC_GRUNTZ_BATTLEZMAPCONFIG_H
#define SRC_GRUNTZ_BATTLEZMAPCONFIG_H

#include <rva.h>

#include <Mfc.h>

#include <Clock64.h>
#include <Gruntz/BattlezDifficulty.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/MapMgr.h>
#include <Gruntz/SerialArchive.h>
#include <Utils/MfcTyped.h>

class CTriggerMgr;
class CTileTriggerSwitchLogic;
struct Coord;
class CTileTriggerContainer;
class CGrunt;
class CGruntzMgr;
class CPlay;

class CBattlezMapConfig {
public:
    i32 LoadConfig(CGruntzMgr* mgr, i32 playerIndex, BattlezDifficulty difficulty);

    CBattlezMapConfig();
    ~CBattlezMapConfig();
    void FreeArrays();
    i32 StepAllRowSpawns();
    void Clear();
    i32 EnterDefenderMode(CGrunt*, i32);
    i32 PathCrossesMarkedTile(CGrunt*);
    i32 IsCoordOccupied(CGrunt*, i32, i32);
    i32 SerializeState(CFileMemBase*, SerialMode, LogicTypeId, i32);
    i32 PathToNearbyUnit(CGrunt*);
    i32 Serialize(CFileMemBase*);
    i32 Deserialize(CFileMemBase*);
    i32 ClaimCellFromRow(i32, i32, i32, i32);
    i32 TrySeedSpawnAt(i32, i32);
    i32 RepathToFreeCell(CGrunt*);
    i32 ProbeUnoccupiedAt(i32, i32);
    i32 ForcePlaceFromReserve(CGrunt*);
    Coord* PickSpawnCoord(Coord*, CGrunt*, i32);

    i32 RouteUnitTo(
        CGrunt* unit,
        i32 goalCol,
        i32 goalRow,
        i32 blockedMask,
        i32 passableMask,
        i32 clearEndpointFlags
    );

    i32 RouteUnitToGoal(CGrunt* unit, Coord goal, i32 blockedMask, i32 passableMask);
    i32 StepRowSpawn(b32 allowReserved);
    i32 CanPlaySpecialAnim(CGrunt*);
    i32 StepBoard();
    i32 ChooseIdleBehavior(CGrunt*);

    void RerouteIdleUnit(
        CGrunt* unit,
        i32 col,
        i32 row,
        i32 burnFirstRandom,
        i32 burnSecondRandom,
        i32 unused
    );

    i32 ValidateUnitPath(CGrunt*);

    void ClaimTilesAround(CGrunt* unit, i32 col, i32 row, i32 requireUnoccupied);
    i32 PathToNearestCandidate(CGrunt*, b32, i32, i32);
    i32 PathToNearestGoal(CGrunt*, i32, i32);
    CGrunt* PickRandomIdleUnit(i32);
    i32 AcceptAlways(CGrunt*);
    i32 CheckQueuedSpawnTile(CGrunt*);
    i32 RetargetIdleUnit(CGrunt*);
    i32 StepRowUnits();
    i32 RepathAroundBlockedTiles(CGrunt*);
    CGrunt* FindIdleGruntInBox(i32 cx, i32 cy, i32 halfW, i32 halfH);
    i32 HandleUnitContact(CGrunt* actor, CGrunt* other);
    i32 RouteToNearbyPickup(CGrunt*);

    i32 ResolveTileClaim(CGrunt* unit, i32 col, i32 row, i32 requireUnoccupied);
    i32 RouteToNearbyEnemy(CGrunt*);
    i32 TrackAssignedEnemy(CGrunt*);
    i32 AdvanceToEnemyBase(CGrunt*);

    i32 ResolveArrival(CGrunt* g);
    i32 Step(CGrunt* g);
    i32 StepDefenderUnit(CGrunt* grunt);

    i32 ScanRegion(CGrunt* g);

    i32 RerouteSwitchSeeker(CGrunt* grunt);

    b32 m_active;
    CGruntzMgr* m_ctx;
    CTriggerMgr* m_triggerMgr;
    CMapMgr* m_board;
    CPlay* m_play;
    CTileTriggerContainer* m_cellQuery;

    i32 m_playerIndex;
    i32 m_reserved01c;
    i32 m_reserved020;
    i32 m_reserved024;
    i32 m_reserved028;
    i32 m_reserved02c;
    i32 m_defenderChance;
    i32 m_reserved034;
    i32 m_reserved038;
    i32 m_reserved03c;
    i32 m_reserved040;
    i32 m_reserved044;
    u32 m_gruntCreationTime;
    u32 m_spawnTimer;
    u32 m_spawnLastFire;
    u32 m_resourceCreationTime;
    u32 m_repickLastFire;
    u32 m_repickTimer;
    i32 m_gauntletzChance;
    i32 m_shovelzChance;
    i32 m_spyzChance;
    i32 m_brickzChance;
    i32 m_gooberzChance;
    u32 m_gruntRatio;

    union {
        Clock64 m_routeTimers[2];
        struct {
            Clock64 m_routeClock;
            Clock64 m_routeWindow;
        };
        struct {
            i32 m_routeClockLo;
            i32 m_routeClockHi;
            i32 m_routeWindowLo;
            i32 m_routeWindowHi;
        };
    };
    i32 m_reserved088;
    i32 m_defenderSearchRadiusX;
    i32 m_defenderSearchRadiusY;
    i32 m_idleRouteLimitX;
    i32 m_idleRouteLimitY;
    i32 m_reserved09c;
    i32 m_idleAttackWaypointDelay;
    i32 m_defenderTargetMaxDistance;
    i32 m_reserved0a8;
    i32 m_idleBurnRandX;
    i32 m_idleBurnRandY;
    i32 m_reserveBudget;
    i32 m_idleRerouteDelay;
    i32 m_moveBudget;
    i32 m_assignedTargetMaxDistance;
    i32 m_repathBudget;
    i32 m_inactiveTargetRerouteDelay;
    i32 m_nearbyRouteSearchDelay;
    Coord m_marker;
    i32 m_reserved0d8;

    CPtrArray m_candArray;
    CPtrArray m_attackWaypoints;

    Coord* CoordAt(i32 index) {
        return static_cast<Coord*>(m_attackWaypoints.GetAt(index));
    }
    CDWordArray m_reserved104;
    CDWordArray m_reserved118;

    i32 m_reserved12c[4];
    i32 m_reserved13c;
    i32 m_roundRobinTick;
    i32 m_reserved144;
    i32 m_claimTimer;
    i32 m_reserved14c;
    i32 m_toolzPct;
    i32 m_toyzPct;
    i32 m_brickzPct;
    i32 m_redBrickPct;
    i32 m_blueBrickPct;
    i32 m_goldBrickPct;
    i32 m_blackBrickPct;
    i32 m_babyWalkerzPct;
    i32 m_beachBallzPct;
    i32 m_bigWheelzPct;
    i32 m_goKartzPct;
    i32 m_jackInTheBoxzPct;
    i32 m_jumpRopezPct;
    i32 m_pogoStickzPct;
    i32 m_scrollzPct;
    i32 m_squeakToyzPct;
    i32 m_yoyozPct;
    i32 m_bombzPct;
    i32 m_boomerangzPct;
    i32 m_toolBrickzPct;
    i32 m_clubzPct;
    i32 m_gauntletzPct;
    i32 m_glovezPct;
    i32 m_gooberzPct;
    i32 m_gravityBootzPct;
    i32 m_gunHatzPct;
    i32 m_nerfGunzPct;
    i32 m_rockzPct;
    i32 m_shieldzPct;
    i32 m_shovelzPct;
    i32 m_springzPct;
    i32 m_spyzPct;
    i32 m_swordzPct;
    i32 m_timeBombzPct;
    i32 m_toobzPct;
    i32 m_wandzPct;
    i32 m_welderzPct;
    i32 m_wingzPct;
};

extern const float g_diffScale;
extern b32 g_stepRun;
extern i32 g_battlezRouteBlockedMask;
extern i32 g_stepCol;
extern i32 g_stepRow;
extern i32 g_diffTier;
#endif // SRC_GRUNTZ_BATTLEZMAPCONFIG_H
