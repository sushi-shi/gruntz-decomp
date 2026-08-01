#ifndef SRC_GRUNTZ_BATTLEZMAPCONFIG_H
#define SRC_GRUNTZ_BATTLEZMAPCONFIG_H

#include <Gruntz/MapMgr.h>
#include <rva.h>
#include <Clock64.h>
#include <Mfc.h>

class CTriggerMgr;
class CTileTriggerSwitchLogic;
struct Coord;
class CTileTriggerContainer;
class CGrunt;
class CGruntzMgr;
class CLevelInfo;

class CBattlezMapConfig {
public:
    i32 LoadConfig(CGruntzMgr* mgr, i32 id, i32 diff);

    CBattlezMapConfig();
    ~CBattlezMapConfig();
    void FreeArrays();
    i32 StepAllRowSpawns();
    void Clear();
    i32 EnterDefenderMode(CGrunt*, i32);
    i32 PathCrossesMarkedTile(CGrunt*);
    i32 IsCoordOccupied(CGrunt*, i32, i32);
    i32 SerializeState(CFileMemBase*, i32, i32, i32);
    i32 PathToNearbyUnit(CGrunt*);
    i32 Serialize(void*);
    i32 Deserialize(void*);
    i32 ClaimCellFromRow(i32, i32, i32, i32);
    i32 TrySeedSpawnAt(i32, i32);
    i32 RepathToFreeCell(CGrunt*);
    i32 ProbeUnoccupiedAt(i32, i32);
    i32 ForcePlaceFromReserve(CGrunt*);
    void* PickSpawnCoord(void*, CGrunt*, i32);

    i32 RouteUnitTo(CGrunt* unit, i32 gx, i32 gy, i32 maskA, i32 maskC, i32 clearFlag);

    i32 RouteUnitToGoal(CGrunt* unit, i32 gx, i32 gy, i32 maskA, i32 maskC);
    i32 StepRowSpawn(i32 allowReserved);
    i32 CanPlaySpecialAnim(CGrunt*);
    i32 StepBoard();
    i32 ChooseIdleBehavior(CGrunt*);
    i32 ValidateUnitPath(CGrunt*);

    void ClaimTilesAround(CGrunt* unit, i32 col, i32 row, i32 requireUnoccupied);
    i32 PathToNearestCandidate(CGrunt*, i32, i32, i32);
    i32 PathToNearestGoal(CGrunt*, i32, i32);
    void* PickRandomIdleUnit(i32);
    i32 AcceptAlways(CGrunt*);
    i32 CheckQueuedSpawnTile(CGrunt*);
    i32 RetargetIdleUnit(CGrunt*);
    i32 StepRowUnits();
    i32 RepathAroundBlockedTiles(CGrunt*);
    CGrunt* FindIdleGruntInBox(i32 cx, i32 cy, i32 halfW, i32 halfH);
    i32 winapi_02ae00_IntersectRect(CGrunt*, CGrunt*);
    i32 winapi_02c140_IntersectRect_PtInRect(CGrunt*);

    i32 winapi_02dfa0_IntersectRect(CGrunt* unit, i32 col, i32 row, i32 requireUnoccupied);
    i32 winapi_02e3a0_PtInRect(CGrunt*);
    i32 winapi_031ca0_IntersectRect(CGrunt*);
    i32 winapi_032060_IntersectRect(CGrunt*);

    i32 ResolveArrival(CGrunt* g);
    i32 Step(CGrunt* g);
    i32 Step33520(CGrunt* g);

    i32 ScanRegion(CGrunt* g);

    i32 Scan(CGrunt* g);

    union {

        struct {
            i32 m_active;
            CGruntzMgr* m_ctx;
            CTriggerMgr* m_triggerMgr;
            CMapMgr* m_board;
            i32 m_010;
            CTileTriggerContainer* m_cellQuery;

            i32 m_curCell;
            i32 m_01c;
            i32 m_020;
            i32 m_024;
            i32 m_028;
            i32 m_02c;
            i32 m_spawnPct;
            i32 m_034;
            i32 m_038;
            i32 m_03c;
            i32 m_040;
            i32 m_044;
            i32 m_spawnInterval;
            i32 m_spawnTimer;
            i32 m_spawnLastFire;
            i32 m_repickInterval;
            i32 m_repickLastFire;
            i32 m_repickTimer;
            i32 m_060;
            i32 m_064;
            i32 m_068;
            i32 m_06c;
            i32 m_070;
            i32 m_budgetMul;

            union {
                Clock64 m_routeClock;
                struct {
                    i32 m_scratch78;
                    i32 m_scratch7c;
                };
            };
            union {
                Clock64 m_routeWindow;
                struct {
                    i32 m_scratch80;
                    i32 m_scratch84;
                };
            };
            i32 m_088;
            i32 m_08c;
            i32 m_090;
            i32 m_094;
            i32 m_098;
            i32 m_09c;
            i32 m_0a0;
            i32 m_0a4;
            i32 m_0a8;
            i32 m_0ac;
            i32 m_0b0;
            i32 m_reserveBudget;
            i32 m_0b8;
            i32 m_moveBudget;
            i32 m_0c0;
            i32 m_repathBudget;
            i32 m_0c8;
            i32 m_0cc;
            i32 m_0d0;
            i32 m_0d4;
            i32 m_0d8;
        };

        struct {
            i32 m_0;
            CGruntzMgr* m_levelInfo;

            CTriggerMgr* m_8;
            CMapMgr* m_dims;
            class CPlay* m_10;

            CTileTriggerContainer* m_14;
            i32 m_ownerId;
            char m_pad1c[0x30 - 0x1c];
            DWORD m_defenderChance;
            char m_pad34[0x48 - 0x34];
            DWORD m_gruntCreationTime;
            DWORD m_4c;
            i32 m_50;
            DWORD m_resourceCreationTime;
            i32 m_58;
            i32 m_5c;
            DWORD m_gauntletzChance;
            DWORD m_shovelzChance;
            DWORD m_spyzChance;
            DWORD m_brickzChance;
            DWORD m_gooberzChance;
            DWORD m_gruntRatio;
            i32 m_78;
            i32 m_7c;
            i32 m_80;
            i32 m_84;
            char m_pad88[0x8c - 0x88];
            i32 m_8c;
            i32 m_90;
            i32 m_94;
            i32 m_98;
            char m_pad9c[0xa4 - 0x9c];
            i32 m_a4;
            char m_pada8[0xac - 0xa8];
            u32 m_ac;
            u32 m_b0;
            char m_padb4[0xc0 - 0xb4];
            u32 m_c0;
            char m_padc4[0xd0 - 0xc4];
            i32 m_markerX;
            i32 m_markerY;
            char m_padd8[0xdc - 0xd8];
        };
    };

    CPtrArray m_candArray;
    CPtrArray m_0f0;

    Coord* CoordAt(i32 index) {
        return static_cast<Coord*>(m_0f0.GetAt(index));
    }
    CDWordArray m_104;
    CDWordArray m_118;

    union {

        struct {
            i32 m_12c;
            i32 m_130;
            i32 m_134;
            i32 m_138;
            i32 m_13c;
            i32 m_140;
            i32 m_144;
            i32 m_claimTimer;
            i32 m_14c;

            i32 m_bandSplitA;
            i32 m_bandSplitB;
            i32 m_bandDiv;
            i32 m_bandCThresh[3];
            i32 m_bandCDiv;
            i32 m_bandBThresh[9];
            i32 m_bandBDiv;
            i32 m_bandAThresh[20];
            i32 m_bandADiv;
        };

        struct {
            char m_padTail12c[0x140 - 0x12c];
            i32 m_140b;
            i32 m_144b;
            i32 m_148;
            i32 m_14cb;
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
            i32 m_1e4;
        };
    };
};
SIZE(0x1e8);

extern "C" void __stdcall SetAtGrow(i32 arrayHandle, void* node);

extern const float g_diffScale;
extern i32 g_stepRun;
extern i32 g_stepCol;
extern i32 g_stepRow;
extern i32 g_diffTier;
#endif // SRC_GRUNTZ_BATTLEZMAPCONFIG_H
