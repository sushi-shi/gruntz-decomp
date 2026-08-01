#include <Mfc.h>
#include <Ints.h>
#include <Clock64.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/LogicTypeId.h>

#include <Gruntz/CoordNode.h>
#include <rva.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <Gruntz/UserBaseLink.h>
#include <Gruntz/SpriteRefTable.h>
#include <Gruntz/WwdGameReg.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/MovingLogic.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/String.h>
#include <Gruntz/CurPlayer.h>
#include <Gruntz/SerialCounter.h>
#ifndef SRC_GRUNTZ_GRUNT_H
#define SRC_GRUNTZ_GRUNT_H

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

extern "C" i32 GruntRand();

class CGrunt;

class CGruntCell {
public:
};
SIZE_UNKNOWN();

struct GruntDirectionCell {
    GruntDirectionCell() {}
    GruntDirectionCell(i32 row_, i32 column_, i32 direction_)
        : row(row_), column(column_), direction(direction_) {}

    i32 row;
    i32 column;
    i32 direction;
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

struct GruntSoundEntry;

struct GruntSoundEntry {
    char m_pad0[0x10];
    DSoundCloneInst* m_10;
};
SIZE_UNKNOWN();

extern FreeNodePool g_coordPool;

class CGruntCoordList : public CPtrList {
public:
    void*& NextData(POSITION& pos);
};
SIZE_UNKNOWN();

struct CAnimSetNode {
    char m_pad0[0xc];
    i32 m_c;
    i32 m_10;
};
SIZE_UNKNOWN();

class CGruntPuddle;

class CArchive;

class CGruntSub {
public:
};
SIZE_UNKNOWN();

struct CGruntCellRec {
    enum NameSlot {
        NAME_ATTACK,
        NAME_STRUCK,
        NAME_WALK,
        NAME_IDLE,
        NAME_ITEM,
        NAME_COUNT
    };

    CString m_names[NAME_COUNT];

    CString& AttackName() {
        return m_names[NAME_ATTACK];
    }
    CString& StruckName() {
        return m_names[NAME_STRUCK];
    }
    CString& WalkName() {
        return m_names[NAME_WALK];
    }
    CString& IdleName() {
        return m_names[NAME_IDLE];
    }
    CString& ItemName() {
        return m_names[NAME_ITEM];
    }

    i32 m_14;
    i32 m_18;
    i32 m_1c;
    i32 m_20;
    i32 m_24;
    i32 m_28;
    i32 m_2c;
    i32 m_30;
    i32 m_34;
    char m_pad38[0x40 - 0x38];
    i32 m_40;
    i32 m_44;

    double m_dirX;
    double m_dirY;
    double m_stepX;
    double m_stepY;
    CGruntCellRec();
    ~CGruntCellRec();

    i32 SerializeStrings(class CFileMemBase* ar);

    i32 DeserializeStrings(class CFileMemBase* ar);
};
SIZE(0x68);
struct GruntStrSub {
    void CtorImpl();
    void Dtor();
    GruntStrSub() {
        CtorImpl();
    }
    ~GruntStrSub() {
        Dtor();
    }
};
SIZE_UNKNOWN();

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

typedef enum GruntAttackPose {
    GRUNT_ATTACK1 = 0,
    GRUNT_ATTACK2 = 1,
} GruntAttackPose;

typedef enum GruntStruckPose {
    GRUNT_STRUCK1 = 0,
    GRUNT_STRUCK2 = 1,
} GruntStruckPose;

typedef enum GruntIdlePose {
    GRUNT_IDLE1 = 0,
    GRUNT_IDLE2 = 1,
    GRUNT_IDLE3 = 2,
    GRUNT_IDLE4 = 3,
    GRUNT_IDLE5 = 4,
} GruntIdlePose;

typedef enum GruntToyPose {
    GRUNT_TOY1 = 0,
    GRUNT_TOY2 = 1,
    GRUNT_TOY_BREAK = 2,
} GruntToyPose;

typedef enum GruntItemPose {
    GRUNT_ITEM1 = 0,
    GRUNT_ITEM2 = 1,
} GruntItemPose;

class CGrunt : public CMovingLogic, public CWapX {
public:
    virtual ~CGrunt() OVERRIDE;
    virtual i32 SerializeMove(CFileMemBase* ar, i32 mode, i32 typeId, CGameObject* pObj) OVERRIDE;
    RVA(0x0000f2a0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_GRUNT;
    }

    virtual void XferName(char* name) OVERRIDE;

    virtual void FireActivation(i32 id) OVERRIDE;

    virtual void Activate() OVERRIDE;
    virtual i32 UserLogicVfunc6() OVERRIDE;
    virtual i32 StepAttackFire() OVERRIDE;

    virtual void UserLogicVfunc9() OVERRIDE;
    virtual void MovingSlot16() OVERRIDE;

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

    i32 LoadGruntDeathAnimations(i32 deathType, i32 killerSlot);

    i32 LoadPickupSprites(i32 type, i32 forced, i32 a3, i32 unused, i32 countStats);

    i32 BuildGruntLoseItemAnimation();

    i32 LoadGruntTypeTable(i32 a, i32 b, i32 c, i32 d);

    i32 LoadTypeTableClearMove(i32 typeId);

    void PlayMoveSoundAtTile(i32 tx, i32 ty);
    void SnapToLastTile(i32 a);
    i32 ClaimSwitchTile();
    void SetArrivalTarget(i32 a, i32 b, i32 c, i32 d);
    void ConsiderArrival(i32 a);
    void SelectMoveIcon(i32 a);
    i32 TryPowerupAtTile();

    i32 PathScan();

    i32 winapi_04a9f0_CopyRect_OffsetRect();

    i32 RectSegProbe(RECT* r, POINT* e1, POINT* e2);

    i32 m_entranceReason;
    i32 m_entrancePxX;
    i32 m_entrancePxY;
    i32 m_lastTilePxX;
    i32 m_lastTilePxY;
    i32 m_commitPxX;
    i32 m_commitPxY;
    i32 m_18c;
    i32 m_toyBlendPct;
    i32 m_194;
    i32 m_198;
    i32 m_toolId;
    i32 m_moveMode;
    i32 m_1a4;
    i32 m_1a8;
    i32 m_1ac;
    i32 m_1b0;
    i32 m_1b4;
    CWwdGameObjectA* m_selectedSprite;
    CWwdGameObjectA* m_toySprite;
    CString m_animSetName;
    CWwdGameObjectA* m_healthSprite;
    CWwdGameObjectA* m_staminaSprite;
    CWwdGameObjectA* m_toyTimeSprite;
    CWwdGameObjectA* m_wingzTimeSprite;
    CWwdGameObjectA* m_powerupSprite;
    i32 m_arrived;
    i32 m_1dc;
    i32 m_1e0;
    i32 m_entranceActive;
    i32 m_arrivalPending;
    i32 m_tileOwnerHi;
    i32 m_tileOwnerLo;
    i32 m_1f4_moveIcon;
    i32 m_1f8;
    i32 m_entranceCommitted;
    i32 m_neighborCol;
    i32 m_neighborRow;
    i32 m_208;
    i32 m_20c;
    i32 m_210;
    i32 m_214;
    i32 m_combatActive;
    i32 m_neighborValid;
    i32 m_poweredUp;
    i32 m_224;
    i32 m_entranceStamped;
    i32 m_22c;
    i32 m_arrivalActive;
    i32 m_coordToggle;
    i32 m_wingzEnabled;
    i32 m_freezeDelayDone;
    i32 m_freezeUnfrozen;
    i32 m_resetApplied;
    i32 m_arrivalFlags;
    i32 m_24c;
    i32 m_250;
    i32 m_254;
    i32 m_gruntKind;
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
        i64 m_holdAnchor64;
        struct {
            i32 m_278;
            i32 m_27c;
        };
    };
    union {
        i64 m_holdWindow64;
        struct {
            i32 m_280;
            i32 m_284;
        };
    };
    i32 m_288;
    i32 m_28c;

    i32 m_reachRectLeft;
    i32 m_reachRectTop;
    i32 m_reachRadius;
    i32 m_reachRectBottom;
    i32 m_2a0;
    i32 m_2a4;
    i32 m_2a8;
    i32 m_2ac;

    RECT m_toyRectA;
    RECT m_toyRectB;
    i32 m_arrivalState;
    i32 m_defenderState;
    i32 m_2d8;
    i32 m_defenderRadius;
    i32 m_2e0;
    i32 m_2e4;
    i32 m_2e8;
    i32 m_dwell;
    i32 m_arrivalCol;
    i32 m_arrivalRow;
    i32 m_2f8;
    i32 m_2fc;
    i32 m_defenderX;
    i32 m_defenderY;

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
    i32 m_318;

    CPtrList m_31c;
    CPtrList m_338;

    CoordNode* CoordHead() const {
        CoordPos p;
        p.m_pos = m_31c.GetHeadPosition();
        return p.m_node;
    }
    CGruntCoordList* CoordListOps() {
        return static_cast<CGruntCoordList*>(&m_31c);
    }
    CoordNode* CoordTail() const {
        CoordPos p;
        p.m_pos = m_31c.GetTailPosition();
        return p.m_node;
    }
    i32 CoordCount() const {
        return m_31c.GetCount();
    }
    i32 PayloadCount() const {
        return m_338.GetCount();
    }

    i32 m_354;
    i32 m_358;
    i32 m_35c;
    i32 m_deathType;
    i32 m_entranceDropActive;
    i32 m_deathAnimStarted;
    i32 m_36c;
    i32 m_370;
    i32 m_374;
    i32 m_378;
    i32 m_moveKind;
    i32 m_moveVariant;
    i32 m_coordRetryCount;
    i32 m_toyTileIndex;
    i32 m_38c;
    i32 m_390;

    CAniElement* m_poseWalk;
    CAniElement* m_poseAttack[2];
    CAniElement* m_poseAttackIdle;
    CAniElement* m_poseStruck[2];
    CAniElement* m_poseIdle[5];
    CAniElement* m_poseDeath;
    CAniElement* m_poseToy[3];
    CAniElement* m_poseItem[2];

    CAniElement* m_pickupGeoSrc;
    i32 m_3dc;
    i32 m_3e0;
    i32 m_moveTileX;
    i32 m_moveTileY;
    i32 m_health;
    i32 m_stamina;
    i32 m_toyTime;
    i32 m_wingzTime;
    char m_pad3fc[0x400 - 0x3fc];

    double m_moveSpeed;
    double m_408;
    double m_410;
    i32 m_418;
    u32 m_timePerTile;
    i32 m_tileClaimed;
    DirectSoundMgr* m_struckSlotSound;
    DirectSoundMgr* m_struckVoiceSound;
    i32 m_42c;
    i32 m_430;
    i32 m_434;
    i32 m_438;
    GruntDirectionCell m_entranceCell;
    CString m_448;
    CString m_44c;
    i32 m_arrivalPhase;
    i32 m_454;
    i32 m_458;
    i32 m_45c;
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

    union {
        Clock64 m_struckClock;
        struct {
            i32 m_8c0;
            i32 m_8c4;
        };
    };
    i32 m_8c8;
    i32 m_8cc;
    i32 m_8d0;
    i32 m_8d4;

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
    void ClearSubA();
    void ClearSubB();
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
    i32 DispatchVtbl24();

    void PlayMoveSound(i32 x, i32 y);
    void PlaySound(i32 range, GruntDirectionCell rec);
    void OnStruck(i32 wasHit);
    i32 ResolveArrivalNeighbor();
    i32 RearmEntranceDrop();

    i32 ArrivalRecycle(i32 a, i32 b, i32 mode, i32 d, i32 e);

    i32 LoadGruntCombatAnimations(
        i32 attackKind,
        i32 struckPose,
        i32 srcRow,
        i32 srcCol,
        i32 srcPxX,
        i32 srcPxY,
        i32 fromProjectile,
        i32 attackerGruntKind
    );

    i32 UpdateArrival(i32 walking, i32 commit);

    i32 StepArrivalDrop(i32 pxX, i32 pxY, i32 arrivalPhase, i32 maskA, i32 clearFlag, i32 maskCIn);
    i32 StepGruntMovement();
    i32 StepAnimDispatchA(i32 a, i32 b, i32 c, i32 d);

    i32 StepAnimDispatchB();

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
    i32 ArrivalScanA();
    i32 ArrivalScanB();
    i32 ArrivalScanC();

    i32 StartBombGruntRun();

    virtual void FinalizeStep(char* name) OVERRIDE;

    i32 StepEntranceRelatchA();
    i32 StepArrivalReroll();
    i32 StepArrivalCommitA();
    i32 StepArrivalCommitB();
    i32 StepEntranceRelatchB();

    i32 StepCombatReaction(
        i32 attackKind,
        i32 struckPose,
        i32 srcRow,
        i32 srcCol,
        i32 srcPxX,
        i32 srcPxY,
        i32 fromProjectile,
        i32 attackerGruntKind
    );

    i32 TileSwitch(i32 col, i32 row, i32 arrivalPhase, i32 maskA, i32 clearFlag, i32 maskCIn);

    i32 LoadVehicleGruntSprites(i32 kind);

    i32 Place(
        class CTriggerMgr* board,
        i32 col,
        i32 row,
        i32 moveIcon,
        i32 typeKind,
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
    void* m_addr;
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

extern "C" i32 CellTargetable(i32 col, i32 row);
extern "C" i32 GameRand();

extern "C" void __stdcall GruntCue(CGrunt* g, i32 code, i32 a, i32 b, i32 c, i32 d);
extern "C" i32 PickupCheck(i32 a, i32 b, i32 c, i32 d, i32 e);

static void GruntScratchTeardown();

#endif // SRC_GRUNTZ_GRUNT_H
