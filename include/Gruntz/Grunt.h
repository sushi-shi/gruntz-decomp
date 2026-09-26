#ifndef SRC_GRUNTZ_GRUNT_H
#define SRC_GRUNTZ_GRUNT_H

#include <rva.h>

#include <DDrawMgr/DDrawChildGroup.h>
#include <Enums.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/ClockInterval.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/CurPlayer.h>
#include <Gruntz/DoubleVector.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GruntDeathType.h>
#include <Gruntz/GruntDirection.h>
#include <Gruntz/GruntEntranceMode.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/MovingLogic.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialCounter.h>
#include <Gruntz/SerialRecords.h>
#include <Gruntz/SpriteRefTable.h>
#include <Gruntz/String.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/WwdGameReg.h>
#include <Ints.h>
#include <Rez/FrameClock.h>

#include <stdlib.h>

GZ_ENUM_FORWARD(BattlezTask);
GZ_ENUM_FORWARD(EnemyAiType);
GZ_ENUM_FORWARD(GruntAiState);

GZ_ENUM_CONST_BEGIN(GruntCombatScan)
    GRUNT_COMBAT_FULL_SCAN_HITS = 5
GZ_ENUM_CONST_END(GruntCombatScan)

GZ_ENUM_CONST_BEGIN(GruntIdleVariant)
    GRUNT_IDLE_VARIANT_PRIMARY = 1,
    GRUNT_IDLE_VARIANT_SECONDARY = 2
GZ_ENUM_CONST_END(GruntIdleVariant)

class CAniElement;

class SoundSample;

class SoundBuffer;

struct SoundCue;

typedef struct tagRECT CCueRect;

class CVoiceManager;

CString __stdcall operator+(const char* lhs, const CString& rhs);
CString __stdcall operator+(const CString& lhs, const char* rhs);

extern i32 g_movingSeed;
extern const double g_slopeNegHalf;
extern const double g_slopePosHalf;

class CGrunt;

GZ_ENUM_CONST_BEGIN(GruntDirectionGrid)
    GRUNT_DIRECTION_GRID_LOW = 0,
    GRUNT_DIRECTION_GRID_CENTER = 1,
    GRUNT_DIRECTION_GRID_HIGH = 2,
    GRUNT_DIRECTION_GRID_WIDTH = 3
GZ_ENUM_CONST_END(GruntDirectionGrid)

GZ_ENUM_CONST_BEGIN(GruntArrivalTag)
    ARRIVAL_TAG_NONE = 0,
    ARRIVAL_TAG_TRIGGER_A = 2,
    ARRIVAL_TAG_TRIGGER_B = 3
GZ_ENUM_CONST_END(GruntArrivalTag)

extern GruntDirectionCell g_gruntDirNorth;
extern GruntDirectionCell g_gruntDirNorthEast;
extern GruntDirectionCell g_gruntDirEast;
extern GruntDirectionCell g_gruntDirSouthEast;
extern GruntDirectionCell g_gruntDirSouth;
extern GruntDirectionCell g_gruntDirSouthWest;
extern GruntDirectionCell g_gruntDirWest;
extern GruntDirectionCell g_gruntDirNorthWest;
extern GruntDirectionCell g_gruntDirCenter;

class CGruntPuddle;

class CArchive;

struct CGruntCellRec {
    GZ_ENUM_BEGIN(NameSlot)
        NAME_ATTACK = 0,
        NAME_STRUCK = 1,
        NAME_WALK = 2,
        NAME_IDLE = 3,
        NAME_ITEM = 4,
        NAME_COUNT = 5
    GZ_ENUM_END(NameSlot)

    CString AT(m_names, NAME_COUNT);

    CString& AttackName() {
        return AT(m_names, NAME_ATTACK);
    }
    CString& StruckName() {
        return AT(m_names, NAME_STRUCK);
    }
    CString& WalkName() {
        return AT(m_names, NAME_WALK);
    }
    CString& IdleName() {
        return AT(m_names, NAME_IDLE);
    }
    CString& ItemName() {
        return AT(m_names, NAME_ITEM);
    }

    RECT m_rects[3];

    struct Motion {
        DoubleVector2 m_direction;
        DoubleVector2 m_step;
    } m_motion;

    i32 SerializeStrings(class CFileMemBase* ar);

    i32 DeserializeStrings(class CFileMemBase* ar);
};
extern GruntDirectionCell g_gruntMoveDirNorth;
extern GruntDirectionCell g_gruntMoveDirNorthEast;
extern GruntDirectionCell g_gruntMoveDirEast;
extern GruntDirectionCell g_gruntMoveDirSouthEast;
extern GruntDirectionCell g_gruntMoveDirSouth;
extern GruntDirectionCell g_gruntMoveDirSouthWest;
extern GruntDirectionCell g_gruntMoveDirWest;
extern GruntDirectionCell g_gruntMoveDirNorthWest;
extern GruntDirectionCell g_gruntMoveDirCenter;

extern u32 g_gruntSpawnClock;

class CProjectile;

GZ_ENUM_BEGIN(GruntAttackPose)
    GRUNT_ATTACK1 = 0,
    GRUNT_ATTACK2 = 1
GZ_ENUM_END(GruntAttackPose)

GZ_ENUM_BEGIN(GruntStruckPose)
    GRUNT_STRUCK1 = 0,
    GRUNT_STRUCK2 = 1
GZ_ENUM_END(GruntStruckPose)

GZ_ENUM_BEGIN(GruntIdlePose)
    GRUNT_IDLE1 = 0,
    GRUNT_IDLE2 = 1,
    GRUNT_IDLE3 = 2,
    GRUNT_IDLE4 = 3,
    GRUNT_IDLE5 = 4
GZ_ENUM_END(GruntIdlePose)

GZ_ENUM_BEGIN(GruntToyPose)
    GRUNT_TOY1 = 0,
    GRUNT_TOY2 = 1,
    GRUNT_TOY_BREAK = 2
GZ_ENUM_END(GruntToyPose)

GZ_ENUM_BEGIN(GruntItemPose)
    GRUNT_ITEM1 = 0,
    GRUNT_ITEM2 = 1
GZ_ENUM_END(GruntItemPose)

class CGrunt : public CMovingLogic, public CWapX {
public:
    inline i32 CanCommitMove(i32 moveX, i32 moveY, i32 sourceX, i32 sourceY) const;
    inline i32 AddBattlezTraversalFlags(i32 flags) const;
    inline PickupType ArrivalPickupOf(PickupType entranceReason) const;
    inline PickupType ArrivalPickup() const;
    inline void BuildUnitSearchBox(RECT* box, i32 radius);
    inline Coord ScanCell();
    inline i32 GetScreenTileY() const;
    inline i32 GetScreenTileX() const;
    inline void MirrorAcrossArrival();
    virtual ~CGrunt() OVERRIDE;
    virtual i32
    SerializeDispatch(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, CGameObject* object)
        OVERRIDE;
    RVA(0x0000f2a0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_GRUNT;
    }

    virtual void StepBehavior(char* animationActName) OVERRIDE;

    virtual void FireActivation(i32 id) OVERRIDE;

    virtual void Activate() OVERRIDE;
    virtual i32 RecordFrameTick() OVERRIDE;
    virtual i32 StepAttackFire() OVERRIDE;

    virtual void OnObjectRemoved() OVERRIDE;
    virtual void AdvanceMotion() OVERRIDE;

    i32 IsAtSavedScreenPos();

    RVA(0x000759e0, 0x18)
    Coord EntrancePx() {
        return m_entrancePx;
    }

    Coord LastTilePx() {
        return m_lastTilePx;
    }

    Coord ArrivalCell() {
        return m_arrivalCell;
    }

    Coord MoveTile() {
        return m_moveTile;
    }

    i32 CreateHealthSprite();
    i32 CreateToySprite();
    i32 CreateStaminaSprite();
    i32 CreateToyTimeSprite();
    i32 CreateWingzTimeSprite();
    i32 CreatePowerupSprite(i32 powerupId);
    i32 CreateSelectedSprite();

    void ReadConfigFromButeMgr();
    i32 LoadGruntMovingDeathConfig();
    void LoadAnimNameTable(i32 kind, i32 toyOnly);

    i32 RectContains(i32 x, i32 y);

    void RecycleCoords();
    i32 VehicleContactContains(i32 x, i32 y);
    i32 CommitNeighbor(i32 targetPlayerIndex, i32 targetUnitIndex, i32 targetPxX, i32 targetPxY);
    CGrunt* FindGridNeighbor(i32 validate);

    i32 StepDumbChaserBehavior();

    i32 StepSmartChaserBehavior();
    i32 UpdateGruntStatus();

    i32 StepCompassMove();

    i32 StepArrivalCommit();

    i32 RunMoveConfig(i32 tileX, i32 tileY);

    i32 BuildGruntExitAnimation();

    i32 LoadVehicleGruntAnimations();

    i32 SetupTubeAnim(b32 isWater);

    i32 LoadWingzGruntSprites(b32 enable);

    i32 LoadGruntAbilityTuning(i32 forced);

    i32 UpdateDeathAnimation();
    i32 UpdateDecayFade();
    i32 LoadWandGruntItemConfig();

    i32 LoadGruntDeathAnimations(GruntDeathType deathType, i32 killerPlayerIndex);

    i32
    LoadPickupSprites(PickupType type, i32 forced, i32 helpCueId, i32 pickupParam, i32 countStats);

    i32 BuildGruntLoseItemAnimation();

    i32 LoadGruntTypeTable(PickupType kind, i32 fresh, i32 variant, i32 defer);

    i32 LoadTypeTableClearMove(PickupType typeId);

    void FaceTowardTile(i32 tileX, i32 tileY);
    void SnapToLastTile(i32 clearArrivalState);
    i32 ClaimSwitchTile();
    i32 SetArrivalTarget(i32 targetPlayerIndex, i32 targetUnitIndex, i32 targetPxX, i32 targetPxY);
    void ConsiderArrival(i32 clearArrivalState);
    void SelectMoveIcon(i32 moveIconId);
    i32 TryPowerupAtTile();

    i32 PathScan();

    i32 IntersectsTileObjectAxes();

    i32 RectSegProbe(RECT* r, POINT* e1, POINT* e2);

    PickupType m_entranceReason;
    Coord m_entrancePx;
    Coord m_lastTilePx;
    Coord m_commitPx;
    i32 m_reserved18c;
    i32 m_toyBlendPct;
    PickupType m_brickPickupType;
    PickupType m_vehiclePickupType;
    PickupType m_toolId;
    PickupType m_entrancePickup;
    i32 m_helpCueId;
    i32 m_reserved1a8;
    i32 m_reserved1ac;
    i32 m_reserved1b0;
    i32 m_reserved1b4;
    CWwdSpriteObject* m_selectedSprite;
    CWwdSpriteObject* m_toySprite;
    CString m_animSetName;
    CWwdSpriteObject* m_healthSprite;
    CWwdSpriteObject* m_staminaSprite;
    CWwdSpriteObject* m_toyTimeSprite;
    CWwdSpriteObject* m_wingzTimeSprite;
    CWwdSpriteObject* m_powerupSprite;
    b32 m_arrived;
    Coord m_reserved1dc;
    b32 m_entranceActive;
    b32 m_arrivalPending;
    i32 m_playerIndex;
    i32 m_unitIndex;
    PickupType m_moveIcon;
    i32 m_savedMoveIcon;
    b32 m_entranceCommitted;
    i32 m_neighborPlayerIndex;
    i32 m_neighborUnitIndex;
    Coord m_attackTargetPx;
    i32 m_reserved210;
    i32 m_struckPose;
    b32 m_combatActive;
    b32 m_neighborValid;
    b32 m_poweredUp;
    i32 m_daFlag;
    b32 m_entranceStamped;
    b32 m_bombRunActive;
    b32 m_arrivalActive;
    b32 m_coordToggle;
    b32 m_wingzEnabled;
    b32 m_freezeDelayDone;
    b32 m_freezeUnfrozen;
    b32 m_resetApplied;
    i32 m_arrivalFlags;
    i32 m_passableMask;
    i32 m_routeBlockedMask;
    i32 m_routePassableMask;
    PickupType m_gruntKind;
    b32 m_entranceArmed;

    class CTriggerMgr* m_triggerMgr;
    i32 m_struckCount;

    ClockInterval m_struckTiming;
    ClockInterval m_holdTiming;
    Coord m_arrivalTargetPx;

    RECT m_reachRect;
    RECT m_reachExclusionRect;

    RECT m_vehicleContactRect;
    RECT m_vehicleContactExclusionRect;
    EnemyAiType m_arrivalState;
    GruntAiState m_defenderState;
    BattlezTask m_battleState;
    i32 m_defenderRadius;
    i32 m_defenderQueuePosition;
    PickupType m_defenderPickupType;
    i32 m_targetTeam;
    i32 m_dwell;
    Coord m_arrivalCell;
    Coord m_unusedBattleCell; // invalidated with arrival/defender cells; never read
    Coord m_defenderPx;

    ClockInterval m_arrivalRerollTiming;
    b32 m_hasExtent;

    CPtrList m_coordList;
    CPtrList m_payloads;

    POSITION CoordHead() const {
        return m_coordList.GetHeadPosition();
    }
    POSITION CoordTail() const {
        return m_coordList.GetTailPosition();
    }
    i32 CoordCount() const {
        return m_coordList.GetCount();
    }
    Coord* GetHeadCoord() {
        return static_cast<Coord*>(m_coordList.GetAt(CoordHead()));
    }
    Coord* GetTailCoord() {
        return static_cast<Coord*>(m_coordList.GetAt(CoordTail()));
    }
    CGruntCellRec* EntranceCell() {
        GruntDirectionCell c = m_entranceCell;
        return &m_cells[3 * c.m_row + c.m_column];
    }
    i32 PayloadCount() const {
        return m_payloads.GetCount();
    }
    i32* HeadPayload() {
        return PayloadCount() == 0 ? NULL : static_cast<i32*>(m_payloads.GetHead());
    }
    void DeleteHeadPayload() {
        if (PayloadCount() != 0) {
            delete[] static_cast<i32*>(m_payloads.RemoveHead());
        }
    }
    void DeleteAllPayloads() {
        while (HeadPayload() != NULL) {
            DeleteHeadPayload();
        }
    }

    b32 m_toolConfigured; // set on every tool (re)config; never read
    b32 m_neighborScanEnabled;
    b32 m_tileMoveCommitted;
    GruntDeathType m_deathType;
    b32 m_entranceDropActive;
    b32 m_deathAnimStarted;
    b32 m_cellRemovalNotified;
    i32 m_killerPlayerIndex;
    i32 m_moveVariantOverride;
    i32 m_powerupDuration;
    i32 m_moveKind;
    i32 m_moveVariant;
    i32 m_coordRetryCount;
    u32 m_toyTileIndex;
    i32 m_warpstoneAnchorIndex;
    b32 m_blockedVoicePending;

    CAniElement* m_poseWalk;
    CAniElement* m_poseAttack[2];
    CAniElement* m_poseAttackIdle;
    CAniElement* m_poseStruck[2];
    CAniElement* m_poseIdle[5];
    CAniElement* m_poseDeath;
    CAniElement* m_poseToy[3];
    CAniElement* m_poseItem[2];

    CAniElement* m_pickupGeoSrc;
    Coord m_reserved3dc;
    Coord m_moveTile;
    i32 m_health;
    i32 m_stamina;
    i32 m_toyTime;
    i32 m_wingzTime;

    double m_moveSpeed;
    double m_movePosX;
    double m_movePosY;
    i32 m_reserved418;
    u32 m_timePerTile;
    b32 m_tileClaimed;
    SoundBuffer* m_vehicleLoopSound;
    SoundBuffer* m_powerupLoopSound;
    i32 m_reserved42c;
    i32 m_reserved430;
    i32 m_startingItemId;
    i32 m_recordedFrameTick;
    GruntDirectionCell m_entranceCell;
    CString m_frameSetName;
    CString m_deathFrameSetName;
    i32 m_arrivalPhase;
    b32 m_pendingTrigger;
    Coord m_pendingTriggerPx;
    b32 m_lowStaminaCued;
    b32 m_arrivalNotified;

    CGruntCellRec m_cells[9];

    ClockInterval m_toyTiming;
    ClockInterval m_idleDelayTiming;
    ClockInterval m_idleWindowTiming;
    ClockInterval m_entranceTiming;
    ClockInterval m_flashTiming;
    ClockInterval m_attackTiming;
    ClockInterval m_combatTiming;
    ClockInterval m_hudRetireTiming;
    ClockInterval m_wingzTiming;
    ClockInterval m_conversionTiming;
    ClockInterval m_shimmerTiming;
    ClockInterval m_arrivalVoiceTiming;
    i32 m_reserved8d0;

    CGrunt() : CMovingLogic(CUserLogic::INLINE_BASE) {}
    CGrunt(CGameObject* owner);

    void LoadCellAnimNames(i32 kind, i32 directionOnly);
    void ResetEntranceAnimation(i32 refreshFrame, i32 chooseIdleVariant, i32 playVoiceCue);

    i32 IsArrivalRerollPending() {
        return !m_arrivalRerollTiming.Expired();
    }

    i32 IsHoldPending() {
        return !m_holdTiming.Expired();
    }

    void ResetArrivalReroll() {
        ResetEntranceAnimation(1, 1, 0);
        m_arrivalRerollTiming.m_startLo = 0;
        m_arrivalRerollTiming.m_intervalLo = 0;
        m_arrivalRerollTiming.m_startHi = 0;
        m_arrivalRerollTiming.m_intervalHi = 0;
        m_arrivalRerollTiming.Start(rand() % 30000 + 30000);
    }
    i32 ResolveEntranceArrival();
    void ClearAllSprites();
    i32 BuildEntranceAnimation(GruntEntranceMode mode);
    i32 LoadEntranceConfig();

    void SetEntrancePos(i32 clearArrivalState, i32 recycleRoute);

    void EnsureVehicleLoopSound(const char* key);
    i32 UpdateEntranceAnim();
    i32 Save(CFileMemBase* ar);

    i32 LoadStateRecord(CFileMemBase* ar);
    i32 CommitArrival();
    void StopVehicleLoopSound();
    void StopPowerupLoopSound();
    void ReapplyLoopSoundParams();
    void DestroyAnims();

    Coord GetTilePos();

    void EnsurePowerupLoopSound(const char* key);

    i32 CanShowStamina();
    i32* EntranceTileOffset(i32* out);
    void ComputeFacing(double dt);
    i32 ResetGeometry();
    i32 StepAttackAction();

    void FaceTowardPixel(i32 x, i32 y);
    void SetFacing(i32 unused, GruntDirectionCell facing);
    void OnStruck(b32 wasHit);
    i32 StepPostGuardBehavior();
    i32 RearmEntranceDrop();

    i32 HandleCombatContact(
        i32 otherPxX,
        i32 otherPxY,
        b32 isAttacker,
        i32 otherPlayerIndex,
        i32 otherUnitIndex
    );

    inline void SelectCombatHitCue(
        CGruntzMgr* reg,
        SoundCue*& cue,
        PickupType attackKind,
        i32 struckPose,
        PickupType attackerGruntKind
    );

    i32 LoadGruntCombatAnimations(
        PickupType attackKind,
        i32 struckPose,
        i32 srcPlayerIndex,
        i32 srcUnitIndex,
        i32 srcPxX,
        i32 srcPxY,
        i32 fromProjectile,
        PickupType attackerGruntKind
    );

    i32 UpdateArrival(i32 walking, i32 commit);

    i32 StepArrivalDrop(
        i32 pxX,
        i32 pxY,
        i32 arrivalPhase,
        i32 blockedMask,
        i32 clearEndpointFlags,
        i32 extraPassableMask
    );
    i32 StepGruntMovement();
    i32 TryTeleportToCell(i32 tileX, i32 tileY, b32 useSecretColor, b32 spawnWormhole);

    i32 FinishActiveAction();

    void RestoreToolAfterToyUse(i32 defer);

    void RestorePreviousAppearance();
    void ApplyEntrancePickup();
    void ResolveEntranceOccupant();

    i32 StepEntranceReinit();

    i32 RunEntranceMove();

    i32 StepWarpExit();

    i32 IsDropReady(i32 clearArrivalState = 0);

    void SettleTubeMove();
    void SettleKnockback();
    bool SettleActiveKnockback();

    i32 BeginAttack(i32 targetPxX, i32 targetPxY);

    i32 StartNeighborAttackAnimation(i32 targetPlayerIndex, i32 targetUnitIndex);

    i32 StartRangedAttackAnimation();

    i32 GruntInRadius(i32 playerIndex, i32 unitIndex);

    i32 StepToyerBehavior();

    i32 FinishEntranceMove();

    i32 LoadFreezeSpellAssets();

    i32 StepBomberBehavior();

    i32 StepScrollGruntBehavior();

    i32 StepMagicWandGruntBehavior();

    i32 StepObjectGuardBehavior();

    i32 StepTimeBomberBehavior();

    i32 StepGauntletGruntBehavior();
    i32 StepToolThiefBehavior();
    i32 StepHitAndRunnerBehavior();
    i32 StepBrickLayerBehavior();
    i32 StepGooSuckerBehavior();
    i32 StepDiggerBehavior();

    i32 StartBombGruntRun();

    virtual void FinalizeStep(char* name) OVERRIDE;

    i32 UpdateToyUseAnimation();
    i32 StepArrivalReroll();
    i32 FinishStruckAnimation();
    i32 FinishKnockbackAnimation();
    i32 FinishToobMoveAnimation();

    i32 StepCombatReaction(
        PickupType attackKind,
        i32 struckPose,
        i32 srcPlayerIndex,
        i32 srcUnitIndex,
        i32 srcPxX,
        i32 srcPxY,
        i32 fromProjectile,
        PickupType attackerGruntKind
    );

    i32 TileSwitch(
        i32 col,
        i32 row,
        i32 arrivalPhase,
        i32 blockedMask,
        i32 clearEndpointFlags,
        i32 extraPassableMask
    );

    i32 LoadVehicleGruntSprites(PickupType kind);

    i32 Place(
        class CTriggerMgr* board,
        i32 playerIndex,
        i32 unitIndex,
        PickupType moveIcon,
        PickupType typeKind,
        i32 vehicleKind,
        EnemyAiType kind,
        i32 defenderRadiusMinusOne,
        i32 defenderQueuePosition,
        i32 defenderPickupType,
        RECT* span,
        GruntEntranceMode entranceMode
    );
    i32 StepDefenderBehavior();
};

union LogicDispatchWord {
    LogicRecordDispatchFn m_dispatch;
    void (CGrunt::*m_gruntMethod)();
    u32 m_bits;
};

typedef i32 (CGrunt::*GruntActHandler)();

bool SameCellTag(const GruntDirectionCell* a, const GruntDirectionCell* b);
bool DifferentCellTag(const GruntDirectionCell* a, const GruntDirectionCell* b);

static void GruntScratchTeardown();

#define STOP_GRUNT_LOOP_SOUNDS                                                                     \
    StopVehicleLoopSound();                                                                        \
    StopPowerupLoopSound()

#endif // SRC_GRUNTZ_GRUNT_H
