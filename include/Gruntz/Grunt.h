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
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/MovingLogic.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialCounter.h>
#include <Gruntz/SerialRecords.h>
#include <Gruntz/SpriteRefTable.h>
#include <Gruntz/String.h>
#include <Gruntz/UserBaseLink.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/WwdGameReg.h>
#include <Ints.h>

class CAniElement;

class FreeNodePool;

class CDDrawSubMgrLeaf;

class DSoundCloneInst;

class DirectSoundMgr;

typedef struct tagRECT CCueRect;
SIZE_UNKNOWN();

class CGruntSpawnConfig;

CString __stdcall operator+(const char* lhs, const CString& rhs);
CString __stdcall operator+(const CString& lhs, const char* rhs);

extern i32 g_movingSeed;

class CGrunt;

struct CTriRecord {
    CTriRecord() {}
    CTriRecord(i32 row_, i32 column_, i32 direction_)
        : row(row_), column(column_), direction(direction_) {}

    i32 Serialize(CFileMemBase* ar, SerialMode tag, LogicTypeId c, CGameObject* d);

    i32 row;
    i32 column;
    i32 direction;
};
SIZE(0xc);

struct GruntDirectionCell : public CTriRecord {
    GruntDirectionCell() {}
    GruntDirectionCell(i32 row_, i32 column_, i32 direction_)
        : CTriRecord(row_, column_, direction_) {}

    void RotateClockwise(i32 steps);
    void RotateCounterclockwise(i32 steps);
};
SIZE(0xc);

extern GruntDirectionCell g_gruntDirNorth;
extern GruntDirectionCell g_gruntDirNorthEast;
extern GruntDirectionCell g_gruntDirEast;
extern GruntDirectionCell g_gruntDirSouthEast;
extern GruntDirectionCell g_gruntDirSouth;
extern GruntDirectionCell g_gruntDirSouthWest;
extern GruntDirectionCell g_gruntDirWest;
extern GruntDirectionCell g_gruntDirNorthWest;
extern GruntDirectionCell g_gruntDirCenter;

extern FreeNodePool g_coordPool;

class CGruntCoordList : public CPtrList {
public:
    void*& NextData(POSITION& pos);
};
SIZE_UNKNOWN();

class CGruntPuddle;

class CArchive;

struct CGruntCellRec {
    GZ_ENUM_BEGIN(NameSlot)
        NAME_ATTACK,
        NAME_STRUCK,
        NAME_WALK,
        NAME_IDLE,
        NAME_ITEM,
        NAME_COUNT
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

    struct {
        DoubleVector2 m_direction;
        DoubleVector2 m_step;
    } m_motion;
    CGruntCellRec();
    ~CGruntCellRec();

    i32 SerializeStrings(class CFileMemBase* ar);

    i32 DeserializeStrings(class CFileMemBase* ar);
};
SIZE(0x68);
extern GruntDirectionCell g_gruntMoveDirNorth;
extern GruntDirectionCell g_gruntMoveDirNorthEast;
extern GruntDirectionCell g_gruntMoveDirEast;
extern GruntDirectionCell g_gruntMoveDirSouthEast;
extern GruntDirectionCell g_gruntMoveDirSouth;
extern GruntDirectionCell g_gruntMoveDirSouthWest;
extern GruntDirectionCell g_gruntMoveDirWest;
extern GruntDirectionCell g_gruntMoveDirNorthWest;
extern GruntDirectionCell g_gruntMoveDirCenter;

extern const double g_movingLogicMin;
extern const double g_movingLogicMax;
extern u32 g_defaultZ;
extern u32 g_gruntSpawnClock;
extern "C" u32 g_frameTime;

class CProjectile;

union CoordPos {
    POSITION m_pos;
    CoordNode* m_node;
};

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
    SerializeMove(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, CGameObject* pObj)
        OVERRIDE;
    RVA(0x0000f2a0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_GRUNT;
    }

    virtual void XferName(char* name) OVERRIDE;

    virtual void FireActivation(i32 id) OVERRIDE;

    virtual void Activate() OVERRIDE;
    virtual i32 RecordFrameTick() OVERRIDE;
    virtual i32 StepAttackFire() OVERRIDE;

    virtual void OnObjectRemoved() OVERRIDE;
    virtual void AdvanceMotion() OVERRIDE;

    i32 CreateHealthSprite();
    i32 CreateToySprite();
    i32 CreateStaminaSprite();
    i32 CreateToyTimeSprite();
    i32 CreateWingzTimeSprite();
    i32 CreatePowerupSprite(i32 a);
    i32 CreateSelectedSprite();

    void ReadConfigFromButeMgr();
    i32 LoadGruntMovingDeathConfig();
    void LoadAnimNameTable(i32 a, i32 b);

    i32 RectContains(i32 x, i32 y);

    void RecycleCoords();
    i32 RectContainsGated(i32 x, i32 y);
    i32 CommitNeighbor(i32 a, i32 b, i32 c, i32 d);
    CGrunt* FindGridNeighbor(i32 validate);

    i32 ChargeStep();

    i32 ScanNearestTarget();
    i32 UpdateGruntStatus();

    i32 StepCompassMove();

    i32 StepArrivalCommit();

    void RunMoveConfig(i32 a, i32 b);

    i32 BuildGruntExitAnimation();

    i32 LoadVehicleGruntAnimations();

    i32 SetupTubeAnim(i32 isWater);

    i32 LoadWingzGruntSprites(i32 enable);

    i32 LoadGruntAbilityTuning(i32 forced);

    i32 LoadGruntDecayConfig();
    i32 LoadGruntDecayConfig2();
    i32 LoadWandGruntItemConfig();

    i32 LoadGruntDeathAnimations(GruntDeathType deathType, i32 killerSlot);

    i32 LoadPickupSprites(PickupType type, i32 forced, i32 helpCueId, i32 unused, i32 countStats);

    i32 BuildGruntLoseItemAnimation();

    i32 LoadGruntTypeTable(PickupType a, i32 b, i32 c, i32 d);

    i32 LoadTypeTableClearMove(i32 typeId);

    void PlayMoveSoundAtTile(i32 tx, i32 ty);
    void SnapToLastTile(i32 a);
    i32 ClaimSwitchTile();
    void SetArrivalTarget(i32 a, i32 b, i32 c, i32 d);
    void ConsiderArrival(i32 a);
    void SelectMoveIcon(i32 a);
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
    // NOT a movement mode despite the name it was reconstructed under: every
    // read range-dispatches it by PickupType band (>=0x17 Toyz, >=0x22 Brickz,
    // >=0x32 PowerUpz) and casts it, and the only non-sentinel write is the
    // PickupType handed to CGrunt::LoadPickup. -1 is PICKUP_INVALID.
    PickupType m_entrancePickup;
    i32 m_helpCueId;
    i32 m_reserved1a8;
    i32 m_reserved1ac;
    i32 m_reserved1b0;
    i32 m_reserved1b4;
    CWwdGameObjectA* m_selectedSprite;
    CWwdGameObjectA* m_toySprite;
    CString m_animSetName;
    CWwdGameObjectA* m_healthSprite;
    CWwdGameObjectA* m_staminaSprite;
    CWwdGameObjectA* m_toyTimeSprite;
    CWwdGameObjectA* m_wingzTimeSprite;
    CWwdGameObjectA* m_powerupSprite;
    i32 m_arrived;
    Coord m_reserved1dc;
    i32 m_entranceActive;
    i32 m_arrivalPending;
    i32 m_tileOwnerHi;
    i32 m_tileOwnerLo;
    PickupType m_moveIcon;
    i32 m_savedMoveIcon;
    i32 m_entranceCommitted;
    Coord m_neighborCell;
    Coord m_attackTargetPx;
    i32 m_reserved210;
    i32 m_struckPose;
    i32 m_combatActive;
    i32 m_neighborValid;
    i32 m_poweredUp;
    i32 m_daFlag; // "da=" in retail CRC line; role unrecovered
    i32 m_entranceStamped;
    i32 m_bombRunActive;
    i32 m_arrivalActive;
    i32 m_coordToggle;
    i32 m_wingzEnabled;
    i32 m_freezeDelayDone;
    i32 m_freezeUnfrozen;
    i32 m_resetApplied;
    i32 m_arrivalFlags;
    i32 m_passableMask;
    i32 m_routeMaskA;
    i32 m_routeMaskC;
    PickupType m_gruntKind;
    i32 m_entranceArmed;

    class CTriggerMgr* m_tileMgr;
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

    RECT m_toyRectA;
    RECT m_toyRectB;
    i32 m_arrivalState;
    i32 m_defenderState;
    i32 m_battleState;
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
    i32 m_hasExtent;

    CPtrList m_coordList;
    CPtrList m_payloads;

    CoordNode* CoordHead() const {
        CoordPos p;
        p.m_pos = m_coordList.GetHeadPosition();
        return p.m_node;
    }
    CGruntCoordList* CoordListOps() {
        return static_cast<CGruntCoordList*>(&m_coordList);
    }
    CoordNode* CoordTail() const {
        CoordPos p;
        p.m_pos = m_coordList.GetTailPosition();
        return p.m_node;
    }
    i32 CoordCount() const {
        return m_coordList.GetCount();
    }
    i32 PayloadCount() const {
        return m_payloads.GetCount();
    }

    i32 m_toolConfigured; // set on every tool (re)config; never read
    i32 m_neighborScanEnabled;
    i32 m_tileMoveCommitted;
    GruntDeathType m_deathType;
    i32 m_entranceDropActive;
    i32 m_deathAnimStarted;
    i32 m_cellRemovalNotified;
    i32 m_killerSlot;
    i32 m_moveVariantOverride;
    i32 m_powerupDuration;
    i32 m_moveKind;
    i32 m_moveVariant;
    i32 m_coordRetryCount;
    i32 m_toyTileIndex;
    i32 m_warpstoneAnchorIndex;
    i32 m_blockedVoicePending;

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
    double m_movePosX;
    double m_movePosY;
    i32 m_reserved418;
    u32 m_timePerTile;
    i32 m_tileClaimed;
    DirectSoundMgr* m_struckSlotSound;
    DirectSoundMgr* m_struckVoiceSound;
    i32 m_reserved42c;
    i32 m_reserved430;
    i32 m_startingItemId;
    i32 m_recordedFrameTick;
    GruntDirectionCell m_entranceCell;
    CString m_frameSetName;
    CString m_deathFrameSetName;
    i32 m_arrivalPhase;
    i32 m_pendingTrigger;
    Coord m_pendingTriggerPx;
    i32 m_lowStaminaCued;
    i32 m_arrivalNotified;

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

    CGrunt() {}
    CGrunt(void* owner);

    void LoadCellAnimNames(i32 a, i32 b);
    void ResetEntranceAnimation(i32 a, i32 b, i32 c);
    i32 ResolveEntranceArrival();
    void ClearAllSprites();
    void BuildEntranceAnimation(i32 mode);
    i32 LoadEntranceConfig();

    void SetEntrancePos(i32 a, i32 b);

    void EnsureStruckSlot(const char* key);
    i32 UpdateEntranceAnim();
    void ApplyMoveKind(i32 v);
    i32 Save(CFileMemBase* ar);

    i32 LoadStateRecord(CFileMemBase* ar);
    i32 CommitArrival();
    void StopStruckSlotSound();
    void StopStruckVoiceSound();
    void ReapplyVoiceParams();
    void DestroyAnims();

    Coord* GetTilePos(Coord* out);

    void EnsureStruckVoice(const char* key);

    void ArrivalClaim(i32 a, i32 b);
    void ArrivalHook0();
    void ArrivalHook1();
    void ArrivalHook2();
    void ArrivalHook3();
    void ArrivalHook4();
    void ArrivalHook5();
    i32 CanShowStamina();
    void EntranceTileOffset(i32* out);
    void ComputeFacing(double dt);
    i32 ResetGeometry();
    i32 StepAttackAction();

    void PlayMoveSound(i32 x, i32 y);
    void PlaySound(i32 range, GruntDirectionCell rec);
    void OnStruck(i32 wasHit);
    i32 ResolveArrivalNeighbor();
    i32 RearmEntranceDrop();

    i32 ArrivalRecycle(i32 a, i32 b, i32 mode, i32 d, i32 e);

    i32 LoadGruntCombatAnimations(
        PickupType attackKind,
        i32 struckPose,
        i32 srcRow,
        i32 srcCol,
        i32 srcPxX,
        i32 srcPxY,
        i32 fromProjectile,
        PickupType attackerGruntKind
    );

    i32 UpdateArrival(i32 walking, i32 commit);

    i32 StepArrivalDrop(i32 pxX, i32 pxY, i32 arrivalPhase, i32 maskA, i32 clearFlag, i32 maskCIn);
    i32 StepGruntMovement();
    i32 TryTeleportToCell(i32 tileX, i32 tileY, i32 useSecretColor, i32 spawnWormhole);

    i32 FinishActiveAction();

    i32 StepEntranceReinit();

    i32 RunEntranceMove();

    i32 StepWarpExit();

    i32 IsDropReady(i32 a = 0);

    i32 BeginAttack(i32 a, i32 b);

    i32 RearmAttackAnim(i32 col, i32 row);

    i32 RearmAttackAnim2();

    i32 GruntInRadius(i32 col, i32 row);

    i32 StepPeerTracking();

    i32 FinishEntranceMove();

    i32 LoadFreezeSpellAssets();

    i32 ResolveArrivalReposition();

    i32 StepArrivalDefense();

    i32 StepArrivalDefenseLean();

    i32 StepArrivalDefenseAlt();

    i32 PhaseStep();

    i32 UpdateArrival();
    i32 SeekTarget();
    i32 WanderStep();
    i32 StepBrickLayerBehavior();
    i32 StepGooSuckerBehavior();
    i32 StepDiggerBehavior();

    i32 StartBombGruntRun();

    virtual void FinalizeStep(char* name) OVERRIDE;

    i32 StepEntranceRelatchA();
    i32 StepArrivalReroll();
    i32 StepArrivalCommitA();
    i32 StepArrivalCommitB();
    i32 StepEntranceRelatchB();

    i32 StepCombatReaction(
        PickupType attackKind,
        i32 struckPose,
        i32 srcRow,
        i32 srcCol,
        i32 srcPxX,
        i32 srcPxY,
        i32 fromProjectile,
        PickupType attackerGruntKind
    );

    i32 TileSwitch(i32 col, i32 row, i32 arrivalPhase, i32 maskA, i32 clearFlag, i32 maskCIn);

    i32 LoadVehicleGruntSprites(PickupType kind);

    i32 Place(
        class CTriggerMgr* board,
        i32 col,
        i32 row,
        PickupType moveIcon,
        PickupType typeKind,
        i32 vehicleKind,
        i32 kind,
        i32 a8,
        i32 a9,
        i32 a10,
        RECT* span,
        i32 entranceMode
    );

    i32 ArrivalReticleScan();
};
SIZE(0x8d8);

union NotifyWord {
    GameObjNotifyFn m_fn;
    void (CGrunt::*m_method)();
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
SIZE(0x4);

bool SameCellTag(const GruntDirectionCell* a, const GruntDirectionCell* b);

void GruntRecycleCoords(CGrunt* g);
void __stdcall TileSwitch(CGrunt* g, i32 col, i32 row, i32 burnRandA, i32 burnRandB, i32 unused);

extern char s_codeD[];
extern char s_codeF[];
extern char s_codeH[];
extern char s_codeK[];
extern char s_codeM[];
extern char s_codeN[];
extern char s_codeO[];
extern char s_codeQ[];

static void GruntScratchTeardown();

#endif // SRC_GRUNTZ_GRUNT_H
