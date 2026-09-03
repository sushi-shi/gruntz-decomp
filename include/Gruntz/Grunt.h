#ifndef SRC_GRUNTZ_GRUNT_H
#define SRC_GRUNTZ_GRUNT_H

#include <rva.h>

#include <Mfc.h>

#include <Clock64.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <Enums.h>
#include <Gruntz/ActReg.h>
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
#include <Utils/MfcTyped.h>

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

class FreeNodePool;

class SoundSample;

class SoundBuffer;

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

class CGruntCoordList : public CPtrList {
public:
    void*& NextData(POSITION& pos);
};

class CGruntPuddle;

class CArchive;

struct GruntCellMotion {
    DoubleVector2 m_direction;
    DoubleVector2 m_step;
};

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

    GruntCellMotion m_motion;
    CGruntCellRec();
    ~CGruntCellRec();

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

    void FaceTowardTile(Coord tile) {
        FaceTowardTile(tile.m_x, tile.m_y);
    }

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

    union {
        i64 m_struckClock64;
        struct {
            i32 m_struckClockLo;
            i32 m_struckClockHi;
        };
    };
    union {
        i64 m_struckTimer64;
        struct {
            i32 m_struckTimerLo;
            i32 m_struckTimerHi;
        };
    };

    union {
        struct {
            union {
                i64 m_holdAnchor64;
                struct {
                    i32 m_holdAnchorLo;
                    i32 m_holdAnchorHi;
                };
            };
            union {
                i64 m_holdWindow64;
                struct {
                    i32 m_holdWindowLo;
                    i32 m_holdWindowHi;
                };
            };
        };
        CPairRecord m_holdTiming;
    };
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

    union {
        struct {
            union {
                i64 m_arrivalReroll64;
                struct {
                    i32 m_arrivalRerollLo;
                    i32 m_arrivalRerollHi;
                };
            };
            union {
                i64 m_arrivalRerollWindow64;
                struct {
                    i32 m_arrivalRerollWindowLo;
                    i32 m_arrivalRerollWindowHi;
                };
            };
        };
        CPairRecord m_arrivalRerollTiming;
    };
    b32 m_hasExtent;

    CPtrList m_coordList;
    CPtrList m_payloads;

    CoordNode* CoordHead() const {
        return MfcNodeFromPosition<CoordNode>(m_coordList.GetHeadPosition());
    }
    CGruntCoordList* CoordListOps() {
        return static_cast<CGruntCoordList*>(&m_coordList);
    }
    CoordNode* CoordTail() const {
        return MfcNodeFromPosition<CoordNode>(m_coordList.GetTailPosition());
    }
    i32 CoordCount() const {
        return m_coordList.GetCount();
    }
    CGruntCellRec* EntranceCell() {
        return &m_cells[3 * m_entranceCell.row + m_entranceCell.column];
    }
    i32 PayloadCount() const {
        return m_payloads.GetCount();
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
    char m_pad3fc[0x400 - 0x3fc];

    double m_moveSpeed;
    DoubleVector2 m_movePosition;
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

    union {
        i64 m_toyClock;
        struct {
            i32 m_toyClockLo, m_toyClockHi;
        };
    };
    union {
        i64 m_toyDuration;
        struct {
            i32 m_toyDurationLo, m_toyDurationHi;
        };
    };
    union {
        i64 m_idleAnchor;
        struct {
            i32 m_idleAnchorLo, m_idleAnchorHi;
        };
    };
    union {
        i64 m_idleDelay;
        struct {
            i32 m_idleDelayLo, m_idleDelayHi;
        };
    };
    union {
        i64 m_idleTimer;
        struct {
            i32 m_idleTimerLo, m_idleTimerHi;
        };
    };
    union {
        i64 m_idleWindow;
        struct {
            i32 m_idleWindowLo, m_idleWindowHi;
        };
    };

    union {
        i64 m_entranceClock64;
        struct {
            i32 m_entranceClockLo;
            i32 m_entranceClockHi;
        };
    };
    union {
        i64 m_entranceSafeTime64;
        struct {
            i32 m_entranceSafeTimeLo;
            i32 m_entranceSafeTimeHi;
        };
    };

    union {
        i64 m_flashClock64;
        struct {
            i32 m_flashClockLo;
            i32 m_flashClockHi;
        };
    };
    union {
        i64 m_flashWindow64;
        struct {
            i32 m_flashWindowLo;
            i32 m_flashWindowHi;
        };
    };

    union {
        i64 m_attackClock64;
        struct {
            i32 m_attackClockLo;
            i32 m_attackClockHi;
        };
    };
    union {
        i64 m_attackDowntime64;
        struct {
            i32 m_attackDowntimeLo;
            i32 m_attackDowntimeHi;
        };
    };

    union {
        i64 m_combatClock64;
        struct {
            i32 m_combatClockLo;
            i32 m_combatClockHi;
        };
    };
    union {
        i64 m_combatTimeout64;
        struct {
            i32 m_combatTimeoutLo;
            i32 m_combatTimeoutHi;
        };
    };

    union {
        i64 m_hudRetireClock64;
        struct {
            i32 m_hudRetireClockLo;
            i32 m_hudRetireClockHi;
        };
    };
    union {
        i64 m_hudRetireWindow64;
        struct {
            i32 m_hudRetireWindowLo;
            i32 m_hudRetireWindowHi;
        };
    };
    union {
        struct {
            union {
                i64 m_wingzClock64;
                struct {
                    i32 m_wingzClockLo;
                    i32 m_wingzClockHi;
                };
            };
            union {
                i64 m_wingzDuration64;
                struct {
                    i32 m_wingzDurationLo;
                    i32 m_wingzDurationHi;
                };
            };
        };
        CPairRecord m_wingzTiming;
    };

    union {
        struct {
            union {
                i64 m_convertClock64;
                struct {
                    i32 m_convertClockLo;
                    i32 m_convertClockHi;
                };
            };
            union {
                i64 m_convertTime64;
                struct {
                    i32 m_convertTimeLo;
                    i32 m_convertTimeHi;
                };
            };
        };
        CPairRecord m_conversionTiming;
    };

    union {
        struct {
            union {
                i64 m_shimmerClock64;
                struct {
                    i32 m_shimmerClockLo;
                    i32 m_shimmerClockHi;
                };
            };
            union {
                i64 m_shimmerWindow64;
                struct {
                    i32 m_shimmerWindowLo;
                    i32 m_shimmerWindowHi;
                };
            };
        };
        CPairRecord m_shimmerTiming;
    };

    union {
        struct {
            union {
                Clock64 m_arrivalVoiceClock;
                struct {
                    i32 m_arrivalVoiceClockLo;
                    i32 m_arrivalVoiceClockHi;
                };
            };
            union {
                Clock64 m_arrivalVoiceWindow;
                struct {
                    i32 m_arrivalVoiceWindowLo;
                    i32 m_arrivalVoiceWindowHi;
                };
            };
        };
        CPairRecord m_arrivalVoiceTiming;
    };
    i32 m_reserved8d0;

    CGrunt()
        : CMovingLogic(CUserLogic::INLINE_BASE),
          m_struckClock64(0),
          m_struckTimer64(0),
          m_holdAnchor64(0),
          m_holdWindow64(0),
          m_arrivalReroll64(0),
          m_arrivalRerollWindow64(0),
          m_toyClock(0),
          m_toyDuration(0),
          m_idleAnchor(0),
          m_idleDelay(0),
          m_idleTimer(0),
          m_idleWindow(0),
          m_entranceClock64(0),
          m_entranceSafeTime64(0),
          m_flashClock64(0),
          m_flashWindow64(0),
          m_attackClock64(0),
          m_attackDowntime64(0),
          m_combatClock64(0),
          m_combatTimeout64(0),
          m_hudRetireClock64(0),
          m_hudRetireWindow64(0),
          m_wingzClock64(0),
          m_wingzDuration64(0),
          m_convertClock64(0),
          m_convertTime64(0),
          m_shimmerClock64(0),
          m_shimmerWindow64(0) {
        m_arrivalVoiceClock.m_v = 0;
        m_arrivalVoiceWindow.m_v = 0;
    }
    CGrunt(CGameObject* owner);

    void LoadCellAnimNames(i32 kind, i32 directionOnly);
    void ResetEntranceAnimation(i32 refreshFrame, i32 chooseIdleVariant, i32 playVoiceCue);

    i32 IsArrivalRerollPending() {
        return static_cast<i64>(g_frameTime) - m_arrivalReroll64 < m_arrivalRerollWindow64;
    }

    i32 IsHoldPending() {
        return static_cast<i64>(g_frameTime) - m_holdAnchor64 < m_holdWindow64;
    }

    void ResetArrivalReroll() {
        ResetEntranceAnimation(1, 1, 0);
        m_arrivalRerollLo = 0;
        m_arrivalRerollWindowLo = 0;
        m_arrivalRerollHi = 0;
        m_arrivalRerollWindowHi = 0;
        m_arrivalRerollWindowLo = rand() % 30000 + 30000;
        m_arrivalRerollWindowHi = 0;
        m_arrivalRerollLo = static_cast<i32>(g_frameTime);
        m_arrivalRerollHi = 0;
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

    i32 StepEntranceReinit();

    i32 RunEntranceMove();

    i32 StepWarpExit();

    i32 IsDropReady(i32 clearArrivalState = 0);

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

union GruntActPmf {
    i32 (CGrunt::*m_pmf)();
    struct {
        CActHandler m_h;
        i32 m_adjust;
    };
};

typedef i32 (CGrunt::*GruntActHandler)();

bool SameCellTag(const GruntDirectionCell* a, const GruntDirectionCell* b);
bool DifferentCellTag(const GruntDirectionCell* a, const GruntDirectionCell* b);

static void GruntScratchTeardown();

#define STOP_GRUNT_LOOP_SOUNDS                                                                     \
    StopVehicleLoopSound();                                                                        \
    StopPowerupLoopSound()

#endif // SRC_GRUNTZ_GRUNT_H
