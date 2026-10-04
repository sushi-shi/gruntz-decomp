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

struct CGruntDirectionData {
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

    i32 Save(class CFileMemBase* ar);

    i32 Load(class CFileMemBase* ar);
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
    inline PickupType ResolveEquippedToolType(PickupType activePickupType) const;
    inline PickupType GetEquippedToolType() const;
    inline void CancelToolAnimationEffects();
    inline void UnregisterFromBoard(i32 exitedLevel);
    inline void SetBusyAndDeselect();

    PickupType GetPowerupType() const {
        return m_powerupType;
    }
    PickupType GetActivePickupType() const {
        return m_activePickupType;
    }
    PickupType GetSavedToolType() const {
        return m_savedToolType;
    }
    PickupType GetCarriedToyType() const {
        return m_carriedToyType;
    }
    PickupType GetBrickPickupType() const {
        return m_brickPickupType;
    }
    inline void BuildUnitSearchBox(RECT* box, i32 radius);
    inline Coord ScanCell();
    i32 GetPlayerIndex() const {
        return m_playerIndex;
    }
    i32 GetUnitIndex() const {
        return m_unitIndex;
    }
    i32 GetStamina() const {
        return m_stamina;
    }
    i32 GetToyTimePercent() const {
        return m_toyTime;
    }
    i32 GetHealth() const {
        return m_health;
    }
    void SetHealth(i32 health) {
        m_health = health;
    }
    b32 IsUnregisteredFromBoard() const {
        return m_cellRemovalNotified;
    }
    void MarkUnregisteredFromBoard() {
        m_cellRemovalNotified = true;
    }
    b32 IsSelected() const {
        return m_selected;
    }
    b32 IsInCombat() const {
        return m_inCombat;
    }
    b32 IsGuarding() const {
        return m_guarding;
    }
    b32 IsSpawnProtected() const {
        return m_spawnProtectionActive;
    }
    b32 IsEntranceCommitted() const {
        return m_entranceCommitted;
    }

    inline i32 GetScreenTileY() const;
    inline i32 GetScreenTileX() const;
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

    PickupType GetMoveIcon() const {
        return m_moveIcon;
    }

    Coord LastTilePx() {
        return m_lastTilePx;
    }

    BattlezTask GetBattlezTask() const {
        return m_battleState;
    }

    void SetBattlezTask(BattlezTask task) {
        m_battleState = task;
    }

    i32 GetRouteBlockedMask() const {
        return m_routeBlockedMask;
    }

    i32 GetRoutePassableMask() const {
        return m_routePassableMask;
    }

    void SetRouteBlockedMask(i32 mask) {
        m_routeBlockedMask = mask;
    }

    void SetRoutePassableMask(i32 mask) {
        m_routePassableMask = mask;
    }

    i32 GetDwell() const {
        return m_dwell;
    }

    void ResetDwell() {
        m_dwell = 0;
    }

    i32 GetTargetTeam() const {
        return m_targetTeam;
    }

    void SetTargetTeam(i32 team) {
        m_targetTeam = team;
    }

    GruntAiState GetAiState() const {
        return m_aiState;
    }

    void SetAiState(GruntAiState state) {
        m_aiState = state;
    }

    PickupType GetDefenderPickupType() const {
        return m_defenderPickupType;
    }

    void SetDefenderPickupType(PickupType type) {
        m_defenderPickupType = type;
    }

    i32 GetDefenderQueuePosition() const {
        return m_defenderQueuePosition;
    }

    void SetDefenderQueuePosition(i32 position) {
        m_defenderQueuePosition = position;
    }

    i32 GetDefenderRadius() const {
        return m_defenderRadius;
    }

    Coord DefenderPosition() const {
        return m_defenderPx;
    }

    Coord ArrivalCell() {
        return m_arrivalCell;
    }

    Coord ToolTargetTile() {
        return m_toolTargetTile;
    }

    i32 CreateHealthSprite();
    i32 CreateToySprite();
    i32 CreateStaminaSprite();
    i32 CreateToyTimeSprite();
    i32 CreateWingzTimeSprite();
    i32 CreatePowerupSprite(i32 powerupId);
    i32 CreateSelectedSprite();

    void ReadConfigFromButeMgr();
    i32 StartDeathMovement();
    void LoadAnimationSet(i32 toyMode, i32 mobileToy);

    i32 RectContains(i32 x, i32 y);

    void RecycleCoords();
    i32 IsInToyUseRange(i32 x, i32 y);
    void SetNeighbor(i32 playerIndex, i32 unitIndex);
    i32 CommitNeighbor(i32 targetPlayerIndex, i32 targetUnitIndex, i32 targetPxX, i32 targetPxY);
    CGrunt* FindGridNeighbor(i32 validate);

    i32 StepDumbChaserBehavior();

    i32 StepSmartChaserBehavior();
    i32 UpdateAttackIdleAnimation();

    i32 StepCompassMove();

    i32 BeginFreezeAnimation();

    i32 StartToolUseAnimation(i32 tileX, i32 tileY);

    i32 BuildGruntExitAnimation();

    i32 UpdateVehicleUseAnimation();

    i32 SetToobWaterMode(b32 isWater);

    i32 SetWingzEnabled(b32 enable);

    i32 CastSpell(i32 spellOverride);

    i32 UpdateDeathAnimation();
    i32 UpdateDecayFade();
    i32 UpdateToolUseAnimation();

    i32 StartDeath(GruntDeathType deathType, i32 killerPlayerIndex);

    i32 BeginPickupAnimation(
        PickupType type,
        i32 forced,
        i32 helpCueId,
        i32 pickupParam,
        i32 countStats
    );

    i32 BuildGruntLoseItemAnimation();

    i32 ApplyPickup(PickupType pickupType, i32 fresh, i32 scrollSpell, i32 defer);

    i32 ApplyPickupAndClearPending(PickupType pickupType);

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

    PickupType m_activePickupType;
    Coord m_entrancePx;
    Coord m_lastTilePx;
    Coord m_commitPx;
    // @identity-TODO: reserved members in this layout are save-streamed, with
    // some also reset during initialization; no gameplay read identifies their roles.
    i32 m_reserved18c;
    i32 m_toyVariantThreshold;
    PickupType m_brickPickupType;
    PickupType m_carriedToyType;
    PickupType m_savedToolType;
    PickupType m_pendingPickupType;
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
    b32 m_selected;
    Coord m_reserved1dc;
    b32 m_busy;
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
    b32 m_attackWindupActive;
    b32 m_attackQueued;
    b32 m_inCombat;
    i32 m_daFlag;
    b32 m_toyBreakStarted;
    b32 m_bombRunStarting;
    b32 m_arrivalActive;
    b32 m_toobWaterMode;
    b32 m_wingzEnabled;
    b32 m_freezeDelayDone;
    b32 m_freezeUnfrozen;
    b32 m_idleVariantActive;
    i32 m_arrivalFlags;
    i32 m_passableMask;
    i32 m_routeBlockedMask;
    i32 m_routePassableMask;
    PickupType m_powerupType;
    b32 m_entranceArmed;

    class CTriggerMgr* m_triggerMgr;
    i32 m_struckCount;

    ClockInterval m_struckTiming;
    ClockInterval m_holdTiming;
    Coord m_arrivalTargetPx;

    RECT m_reachRect;
    RECT m_reachExclusionRect;

    RECT m_toyUseRect;
    RECT m_toyUseExclusionRect;
    EnemyAiType m_aiType;
    GruntAiState m_aiState;
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
    i32 CoordsEmpty() const {
        return m_coordList.IsEmpty();
    }
    Coord* GetCoordAt(POSITION position) {
        return static_cast<Coord*>(m_coordList.GetAt(position));
    }
    Coord* GetNextCoord(POSITION& position) {
        return static_cast<Coord*>(m_coordList.GetNext(position));
    }
    Coord* GetHeadCoord() {
        return static_cast<Coord*>(m_coordList.GetAt(CoordHead()));
    }
    Coord* GetTailCoord() {
        return static_cast<Coord*>(m_coordList.GetAt(CoordTail()));
    }
    CPtrList* GetCoordList() {
        return &m_coordList;
    }
    POSITION AddHeadCoord(Coord* coord) {
        return m_coordList.AddHead(coord);
    }
    POSITION AddTailCoord(Coord* coord) {
        return m_coordList.AddTail(coord);
    }
    void AppendCoords(CPtrList& coords) {
        POSITION position = coords.GetHeadPosition();
        while (position != NULL) {
            AddTailCoord(static_cast<Coord*>(coords.GetNext(position)));
        }
    }
    void RemoveCoordAt(POSITION position) {
        m_coordList.RemoveAt(position);
    }
    Coord* RemoveHeadCoord() {
        return static_cast<Coord*>(m_coordList.RemoveHead());
    }
    Coord* RemoveTailCoord() {
        return static_cast<Coord*>(m_coordList.RemoveTail());
    }
    b32 IsDeathAnimationStarted() const {
        return m_deathAnimStarted;
    }

    CGruntDirectionData* FacingData() {
        GruntDirectionCell c = m_facing;
        return &m_directionData[3 * c.m_row + c.m_column];
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
    b32 m_spawnProtectionActive;
    b32 m_deathAnimStarted;
    b32 m_cellRemovalNotified;
    i32 m_killerPlayerIndex;
    i32 m_wandSpellOverride;
    i32 m_powerupDuration;
    i32 m_scrollSpell;
    i32 m_activeSpell;
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

    CAniElement* m_pickupAnimation;
    Coord m_reserved3dc;
    Coord m_toolTargetTile;
    i32 m_health;
    i32 m_stamina;
    i32 m_toyTime;
    i32 m_wingzTime;

    double m_moveSpeed;
    double m_movePosX;
    double m_movePosY;
    i32 m_reserved418;
    u32 m_timePerTile;
    b32 m_guarding;
    SoundBuffer* m_vehicleLoopSound;
    SoundBuffer* m_powerupLoopSound;
    i32 m_reserved42c;
    i32 m_reserved430;
    i32 m_startingItemId;
    i32 m_recordedFrameTick;
    GruntDirectionCell m_facing;
    CString m_frameSetName;
    CString m_deathFrameSetName;
    i32 m_arrivalPhase;
    b32 m_pendingTrigger;
    Coord m_pendingTriggerPx;
    b32 m_lowStaminaCued;
    b32 m_guardCommandPending;

    CGruntDirectionData m_directionData[9];

    ClockInterval m_toyTiming;
    ClockInterval m_idleDelayTiming;
    ClockInterval m_idleWindowTiming;
    ClockInterval m_entranceTiming;
    ClockInterval m_flashTiming;
    ClockInterval m_attackTiming;
    ClockInterval m_combatTiming;
    ClockInterval m_hudRetireTiming;
    ClockInterval m_wingzTiming;
    ClockInterval m_powerupTiming;
    ClockInterval m_shimmerTiming;
    ClockInterval m_walkVoiceTiming;
    i32 m_reserved8d0;

    CGrunt() : CMovingLogic(CUserLogic::INLINE_BASE) {}
    CGrunt(CGameObject* owner);

    void BuildImageSetNames(i32 toyMode, i32 mobileToy);
    void ResetIdleAnimation(i32 refreshFrame, i32 chooseIdleVariant, i32 playVoiceCue);

    i32 IsArrivalRerollPending() {
        return !m_arrivalRerollTiming.Expired();
    }

    b32 IsCombatTimeoutExpired() const {
        return m_combatTiming.Expired();
    }

    void StartHold(u32 durationMs) {
        m_holdTiming.Start(durationMs);
    }

    i32 IsHoldPending() {
        return !m_holdTiming.Expired();
    }

    void ResetArrivalReroll() {
        ResetIdleAnimation(1, 1, 0);
        m_arrivalRerollTiming.Clear();
        m_arrivalRerollTiming.Start(rand() % 30000 + 30000);
    }
    i32 UpdateIdleAnimation();
    void Deselect();
    i32 BuildEntranceAnimation(GruntEntranceMode mode);
    i32 UpdateEntranceAnimation();

    void SetEntrancePos(i32 clearArrivalState, i32 recycleRoute);

    void EnsureVehicleLoopSound(const char* key);
    i32 UpdateScrollUseAnimation();
    i32 Save(CFileMemBase* ar);

    i32 LoadStateRecord(CFileMemBase* ar);
    i32 Select();
    void StopVehicleLoopSound();
    void StopPowerupLoopSound();
    void ReapplyLoopSoundParams();
    void DestroyAnims();

    Coord GetTilePos();

    void EnsurePowerupLoopSound(const char* key);

    i32 CanShowStamina();
    Coord* EntranceTileOffset(Coord* out);
    void ComputeFacing(double dt);
    i32 StartAttackIdleAnimation();
    i32 StepAttackAction();

    void FaceTowardPixel(i32 x, i32 y);
    void SetFacing(i32 unused, GruntDirectionCell facing);
    void OnStruck(b32 wasHit);
    i32 StepPostGuardBehavior();
    i32 UpdateBombRunAnimation();

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
        PickupType attackerPowerupType
    );

    i32 ApplyCombatHitEffects(
        PickupType attackKind,
        i32 struckPose,
        i32 srcPlayerIndex,
        i32 srcUnitIndex,
        i32 srcPxX,
        i32 srcPxY,
        i32 fromProjectile,
        PickupType attackerPowerupType
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
    void ApplyPendingPickup();

    i32 StartWalkAnimation();

    i32 UpdatePickupAnimation();

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

    i32 UpdateMovingDeathAnimation();

    i32 UpdateFreezeAnimation();

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
    i32 UpdateWalkAnimation();
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
        PickupType attackerPowerupType
    );

    i32 TileSwitch(
        i32 col,
        i32 row,
        i32 arrivalPhase,
        i32 blockedMask,
        i32 clearEndpointFlags,
        i32 extraPassableMask
    );

    i32 SetCarriedToy(PickupType toyType);

    i32 Place(
        class CTriggerMgr* board,
        i32 playerIndex,
        i32 unitIndex,
        PickupType moveIcon,
        PickupType typeKind,
        i32 carriedToyType,
        EnemyAiType aiType,
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

bool SameGruntDirection(const GruntDirectionCell* a, const GruntDirectionCell* b);
bool DifferentGruntDirection(const GruntDirectionCell* a, const GruntDirectionCell* b);

static void GruntScratchTeardown();

#define STOP_GRUNT_LOOP_SOUNDS                                                                     \
    StopVehicleLoopSound();                                                                        \
    StopPowerupLoopSound()

#endif // SRC_GRUNTZ_GRUNT_H
