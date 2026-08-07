#include <rva.h>

#include <Gruntz/Grunt.h>

#include <Mfc.h>
#include <MfcNoInline.h>
#include <MfcWin.h>

#include <AddrWord.h>
#include <Bute/ButeMgr.h>
#include <Bute/ButeTree.h>
#include <Bute/SymTab.h>
#include <DDrawMgr/AniAdvance.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSubMgrLeaf.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Dsndmgr/DirectSoundMgr.h>
#include <Enums.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AniElement.h>
#include <Gruntz/ArrivalFlagsPreset.h>
#include <Gruntz/BattlezData.h>
#include <Gruntz/BattlezMapConfig.h>
#include <Gruntz/BattlezTask.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/CurPlayer.h>
#include <Gruntz/DirectionClassify.h>
#include <Gruntz/EnemyAiType.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameModeId.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntAiState.h>
#include <Gruntz/GruntDeathType.h>
#include <Gruntz/GruntEntranceArrival.h>
#include <Gruntz/GruntEntranceMove.h>
#include <Gruntz/GruntHealthSprite.h>
#include <Gruntz/GruntPowerupSprite.h>
#include <Gruntz/GruntSelectedSprite.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Gruntz/GruntToySprite.h>
#include <Gruntz/GruntzCommandId.h>
#include <Gruntz/GruntzMapMgr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/HealthPct.h>
#include <Gruntz/ImageSets.h>
#include <Gruntz/InGameIcon.h>
#include <Gruntz/MovingLogicSerial.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/Play.h>
#include <Gruntz/Projectile.h>
#include <Gruntz/Random.h>
#include <Gruntz/RockNeighborMask.h>
#include <Gruntz/SbiMenuItemState.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialRecords.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/StaminaPct.h>
#include <Gruntz/State.h>
#include <Gruntz/StatusBarDock.h>
#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/StatusBarTab.h>
#include <Gruntz/Timer.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/UserLogic.h>
#include <Ints.h>
#include <Pix16.h>
#include <Rez/FrameClock.h>
#include <Rez/RezTypeTag.h>
#include <Utils/MapTyped.h>
#include <Wap32/CoordUnset.h>
#include <Wap32/Object.h>
#include <Wap32/TileGeometry.h>
#include <Wap32/Wap32.h>
#include <Wap32/zBitVec.h>
#include <Wwd/MoveMode.h>
#include <Wwd/WwdFile.h>
#include <Wwd/WwdObjectType.h>

#include <math.h>
#include <new>
#include <stdlib.h>
#include <string.h>

DATA(0x001e9a68)
double s_fpZero = 0.0;

static __inline void GruntPosScratchTeardown() {
    CString* slot = (g_typeColl.Slots());
    i32 cnt = g_typeColl.m_grown;
    while (cnt--) {
        if (slot != NULL) {
            slot->~CString();
        }
        slot++;
    }
}

DATA(0x001e9750)
const double g_slopeNegHalf = -0.5;

DATA(0x001e9758)
const double g_slopePosHalf = 0.5;

DATA(0x001e9760)
const double g_slopePosTwo = 2.0;

DATA(0x001e9768)
const double g_slopeNegTwo = -2.0;

// @early-stop

DATA(0x0020a930)
static const char s_GruntHealthSprite[] = "GruntHealthSprite";
DATA(0x0020a904)
static const char s_GruntToySprite[] = "GruntToySprite";
DATA(0x0020a918)
static const char s_GruntStaminaSprite[] = "GruntStaminaSprite";
DATA(0x0020a8ec)
static const char s_GruntToyTimeSprite[] = "GruntToyTimeSprite";
DATA(0x0020a8d0)
static const char s_GruntWingzTimeSprite[] = "GruntWingzTimeSprite";
DATA(0x0020a8b8)
static const char s_GruntPowerupSprite[] = "GruntPowerupSprite";
DATA(0x0020a948)
static const char s_GruntSelectedSprite[] = "GruntSelectedSprite";

static const char s_GRUNTZ_[] = "GRUNTZ_";

DATA(0x0020cc98)
static char s_codeL[] = "L";

i32 g_movingSeed;

DATA(0x0020d414)
static char s_TimePerTile[] = "TimePerTile";
DATA(0x0020a9ec)
static char s_Grunt[] = "Grunt";
DATA(0x0020df98)
static char s_EntranceSafeTime[] = "EntranceSafeTime";
static char s_IdleDelay[] = "IdleDelay";
static char s_PlayerDefenderRadius[] = "PlayerDefenderRadius";
static char s_CombatTimeout[] = "CombatTimeout";

static char s_Spellz[] = "Spellz";
static char s_FreezeDelay[] = "FreezeDelay";

static char s_BOMBGRUNT[] = "BOMBGRUNT";
static char s_RunningTimePerTile[] = "RunningTimePerTile";

DATA(0x0020dfd0)
static char s_FadeTransparency[] = "FadeTransparency";
DATA(0x0020dfc0)
static char s_SafeFlashTime[] = "SafeFlashTime";
DATA(0x0020dfac)
static char s_AccelerateFlash[] = "AccelerateFlash";
DATA(0x0020d9b4)
static char s_Powerupz[] = "Powerupz";
DATA(0x0020d974)
static char s_ConversionTime[] = "ConversionTime";
DATA(0x0020d900)
static char s_GruntGhostTransparencyOn[] = "GruntGhostTransparencyOn";

DATA(0x0020a454)
static char s_codeA[] = "A";
static char s_codeE[] = "E";
static char s_codeI[] = "I";

DATA(0x002455b0)
i32 g_traitorMode;

DATA(0x0020bb64)
static const char s_pose_WALK[] = "_WALK";
DATA(0x0020d7e8)
static const char s_pose_ATTACK1[] = "_ATTACK1";
DATA(0x0020d7dc)
static const char s_pose_ATTACK2[] = "_ATTACK2";
DATA(0x0020d7cc)
static const char s_pose_ATTACKIDLE[] = "_ATTACK-IDLE";
DATA(0x0020d7c0)
static const char s_pose_STRUCK1[] = "_STRUCK1";
DATA(0x0020d7b4)
static const char s_pose_STRUCK2[] = "_STRUCK2";
static const char s_pose_IDLE1[] = "_IDLE1";
static const char s_pose_IDLE2[] = "_IDLE2";
static const char s_pose_IDLE3[] = "_IDLE3";
static const char s_pose_IDLE4[] = "_IDLE4";
DATA(0x0020d7ac)
static const char s_pose_IDLE5[] = "_IDLE5";
DATA(0x0020d7a4)
static const char s_pose_ITEM[] = "_ITEM";
DATA(0x0020d79c)
static const char s_pose_ITEM2[] = "_ITEM2";
static const char s_pose_DEATH[] = "_DEATH";
DATA(0x0020d794)
static const char s_pose_TOY1[] = "_TOY1";
DATA(0x0020d78c)
static const char s_pose_TOY2[] = "_TOY2";
DATA(0x0020d77c)
static const char s_pose_TOYBREAK[] = "_TOY-BREAK";

static inline CAniElement* FindAnimElement(CMapStringToPtr& map, LPCTSTR key) {
    CAniElement* out = 0;
    MapLookup(map, key, out);
    return out;
}

#define LOAD_POSE(dst, sfx)                                                                        \
    ((dst) = FindAnimElement(                                                                      \
         m_wwdObject->OwnerMgr()->m_animRegistry->m_animations,                                    \
         "GRUNTZ_" + m_animSetName + (sfx)                                                         \
     ))

static __inline void GruntScratchTeardown() {
    CString* slot = (g_typeColl.Slots());
    i32 cnt = g_typeColl.m_grown;
    while (cnt--) {
        if (slot != NULL) {
            slot->~CString();
        }
        slot++;
    }
}

// @early-stop
RVA_COMPGEN(0x0000f2c0, 0x1e, ??_GCGrunt@@UAEPAXI@Z)
RVA(0x0000f2f0, 0xc8)
CGrunt::~CGrunt() {
    OnObjectRemoved();
}

RVA(0x0000f400, 0x1b)
CGruntCellRec::CGruntCellRec() {}

RVA(0x0000f430, 0x10)
CGruntCellRec::~CGruntCellRec() {}

DATA(0x0020d404)
static const char s_NORMALGRUNT[] = "NORMALGRUNT";

// @early-stop

RVA(0x00047a10, 0x770)
CGrunt::CGrunt(void* owner)
    : CMovingLogic(static_cast<CGameObject*>(owner)),
      CWapX(static_cast<CGameObject*>(owner)),
      m_struckClockLo(0),
      m_struckTimerLo(0),
      m_struckClockHi(0),
      m_struckTimerHi(0),
      m_holdAnchorLo(0),
      m_holdWindowLo(0),
      m_holdAnchorHi(0),
      m_holdWindowHi(0),
      m_arrivalRerollLo(0),
      m_arrivalRerollWindowLo(0),
      m_arrivalRerollHi(0),
      m_arrivalRerollWindowHi(0),
      m_toyClockLo(0),
      m_toyDurationLo(0),
      m_toyClockHi(0),
      m_toyDurationHi(0),
      m_idleAnchorLo(0),
      m_idleDelayLo(0),
      m_idleAnchorHi(0),
      m_idleDelayHi(0),
      m_idleTimerLo(0),
      m_idleWindowLo(0),
      m_idleTimerHi(0),
      m_idleWindowHi(0),
      m_entranceClockLo(0),
      m_entranceSafeTimeLo(0),
      m_entranceClockHi(0),
      m_entranceSafeTimeHi(0),
      m_flashClockLo(0),
      m_flashWindowLo(0),
      m_flashClockHi(0),
      m_flashWindowHi(0),
      m_attackClockLo(0),
      m_attackDowntimeLo(0),
      m_attackClockHi(0),
      m_attackDowntimeHi(0),
      m_combatClockLo(0),
      m_combatTimeoutLo(0),
      m_combatClockHi(0),
      m_combatTimeoutHi(0),
      m_hudRetireClockLo(0),
      m_hudRetireWindowLo(0),
      m_hudRetireClockHi(0),
      m_hudRetireWindowHi(0),
      m_wingzClockLo(0),
      m_wingzDurationLo(0),
      m_wingzClockHi(0),
      m_wingzDurationHi(0),
      m_convertClockLo(0),
      m_convertTimeLo(0),
      m_convertClockHi(0),
      m_convertTimeHi(0),
      m_shimmerClockLo(0),
      m_shimmerWindowLo(0),
      m_shimmerClockHi(0),
      m_shimmerWindowHi(0),
      m_arrivalVoiceClockLo(0),
      m_arrivalVoiceWindowLo(0),
      m_arrivalVoiceClockHi(0),
      m_arrivalVoiceWindowHi(0) {
    m_entranceCell.row = g_gruntMoveDirSouth.row;
    m_entranceCell.column = g_gruntMoveDirSouth.column;
    m_entranceCell.direction = g_gruntMoveDirSouth.direction;
    m_startingItemId = m_object->m_powerup;
    m_recordedFrameTick = g_frameTicks;
    m_object->m_moveMode = MOVE_GROUNDED;
    m_reserved430 = 0;
    m_reserved42c = 0;

    m_poseWalk = NULL;
    memset(m_poseAttack, 0, sizeof(m_poseAttack));
    m_poseAttackIdle = NULL;
    memset(m_poseStruck, 0, sizeof(m_poseStruck));
    memset(m_poseIdle, 0, sizeof(m_poseIdle));
    memset(m_poseItem, 0, sizeof(m_poseItem));
    m_poseDeath = NULL;
    memset(m_poseToy, 0, sizeof(m_poseToy));
    m_pickupGeoSrc = NULL;
    m_arrived = 0;
    m_wwdObject->m_objectType = WWD_OBJECT_TYPE_GRUNT;
    m_wwdObject->m_hitTypeFlags = 0x3d1;
    m_wwdObject->m_flags |= 0x2000100;
    m_wwdObject->m_collMask |= 0x103f;
    m_wwdObject->m_attackTypeMask = 1;
    m_tileOwnerHi = -1;
    m_tileOwnerLo = -1;
    m_neighborCell.m_x = -1;
    m_warpstoneAnchorIndex = 0;
    m_entranceReason = PICKUP_NONE;
    m_vehiclePickupType = PICKUP_NONE;
    m_brickPickupType = PICKUP_NONE;
    m_gruntKind = GRUNT_NORMAL;
    m_toolId = PICKUP_NONE;
    m_animSetName = s_NORMALGRUNT;
    m_neighborCell.m_y = -1;
    m_entranceCommitted = 1;
    m_healthSprite = NULL;
    m_reachRect.left = -1;
    m_staminaSprite = NULL;
    m_toyTimeSprite = NULL;
    m_wingzTimeSprite = NULL;
    m_selectedSprite = NULL;
    m_toySprite = NULL;
    m_powerupSprite = NULL;
    m_reserved210 = 0;
    m_combatActive = 0;
    m_neighborValid = 0;
    m_arrivalActive = 0;
    m_coordToggle = 0;
    m_wingzEnabled = 0;
    m_tileClaimed = 0;
    m_struckVoiceSound = NULL;
    m_reachRect.top = -1;
    m_reachRect.right = 1;
    m_reachRect.bottom = 1;
    m_reachExclusionRect.left = 0;
    m_reachExclusionRect.top = 0;
    m_reachExclusionRect.right = 0;
    m_reachExclusionRect.bottom = 0;
    m_toyRectA.left = 0;
    m_toyRectA.top = 0;
    m_toyRectA.right = 0;
    m_toyRectA.bottom = 0;
    m_toyRectB.left = 0;
    m_toyRectB.top = 0;
    m_toyRectB.right = 0;
    m_toyRectB.bottom = 0;

    m_toyClockLo = 0;
    m_toyDurationLo = 0;
    m_toyClockHi = 0;
    m_toyDurationHi = 0;
    m_idleAnchorLo = 0;
    m_idleDelayLo = 0;
    m_idleAnchorHi = 0;
    m_idleDelayHi = 0;
    m_idleTimerLo = 0;
    m_idleWindowLo = 0;
    m_idleTimerHi = 0;
    m_idleWindowHi = 0;
    m_entranceClockLo = 0;
    m_entranceSafeTimeLo = 0;
    m_entranceClockHi = 0;
    m_entranceSafeTimeHi = 0;
    m_flashClockLo = 0;
    m_flashWindowLo = 0;
    m_flashClockHi = 0;
    m_flashWindowHi = 0;
    m_attackClockLo = 0;
    m_attackDowntimeLo = 0;
    m_attackClockHi = 0;
    m_attackDowntimeHi = 0;
    m_combatClockLo = 0;
    m_combatTimeoutLo = 0;
    m_combatClockHi = 0;
    m_combatTimeoutHi = 0;
    m_hudRetireClockLo = 0;
    m_hudRetireWindowLo = 0;
    m_hudRetireClockHi = 0;
    m_hudRetireWindowHi = 0;
    m_wingzClockLo = 0;
    m_wingzDurationLo = 0;
    m_wingzClockHi = 0;
    m_wingzDurationHi = 0;
    m_convertClockLo = 0;
    m_convertTimeLo = 0;
    m_convertClockHi = 0;
    m_convertTimeHi = 0;
    m_shimmerClockLo = 0;
    m_shimmerWindowLo = 0;
    m_shimmerClockHi = 0;
    m_shimmerWindowHi = 0;
    m_arrivalVoiceClockLo = 0;
    m_arrivalVoiceWindowLo = 0;
    m_arrivalVoiceClockHi = 0;
    m_arrivalVoiceWindowHi = 0;
    m_arrivalRerollLo = 0;
    m_arrivalRerollWindowLo = 0;
    m_arrivalRerollHi = 0;
    m_arrivalRerollWindowHi = 0;
    m_unusedBattleCell.m_x = -1;
    m_unusedBattleCell.m_y = -1;
    m_arrivalNotified = 0;
    m_defenderState = AISTATE_SEEK;
    m_battleState = BZTASK_UNASSIGNED;
    {
        CWwdGameObjectA* h = m_object;
        i32 lim = h->m_screenY + 0x186a0;
        if (h->m_sortKey != lim) {
            h->m_sortKey = lim;
            h->m_flags |= 0x20000;
        }
    }
    m_blockedVoicePending = 1;
}

RVA(0x00048360, 0x7e)
void CGrunt::OnObjectRemoved() {
    if (CoordCount() != 0) {

        POSITION pos = m_coordList.GetHeadPosition();
        while (pos != NULL) {
            void* buf = m_coordList.GetNext(pos);
            if (buf) {
                CoordPoolNode* slot = g_coordPool.NodeOf(buf);
                slot->m_next = g_coordPool.m_freeHead;
                g_coordPool.m_freeHead = slot;
            }
        }
        m_coordList.RemoveAll();
    }

    while (1) {
        i32 n = PayloadCount();
        void* head = (n == 0) ? 0 : m_payloads.GetHead();
        if (head == NULL) {
            return;
        }
        if (n == 0) {
            continue;
        }
        void* p = m_payloads.RemoveHead();
        delete[] static_cast<i32*>(p);
    }
}

RVA(0x00048400, 0x47)
void CGrunt::ReadConfigFromButeMgr() {
    m_reserved18c = 0;
    m_reserved418 = 0;

    m_timePerTile = g_buteMgr.GetDwordDef(
        const_cast<char*>(static_cast<const char*>(m_animSetName)),
        s_TimePerTile,
        1000
    );

    if (m_gruntKind == GRUNT_SUPERSPEED) {
        m_timePerTile >>= 1;
    }
}

DATA(0x0020d768)
static const char s_d48_NORTHWEST_WALK[] = "_NORTHWEST_WALK";
DATA(0x0020d758)
static const char s_d48_NORTH_WALK[] = "_NORTH_WALK";
DATA(0x0020d744)
static const char s_d48_NORTHEAST_WALK[] = "_NORTHEAST_WALK";
DATA(0x0020d734)
static const char s_d48_WEST_WALK[] = "_WEST_WALK";
DATA(0x0020d724)
static const char s_d48_EAST_WALK[] = "_EAST_WALK";
DATA(0x0020d710)
static const char s_d48_SOUTHWEST_WALK[] = "_SOUTHWEST_WALK";
DATA(0x0020d700)
static const char s_d48_SOUTH_WALK[] = "_SOUTH_WALK";
DATA(0x0020d6ec)
static const char s_d48_SOUTHEAST_WALK[] = "_SOUTHEAST_WALK";
DATA(0x0020d6d4)
static const char s_d48_NORTHWEST_STRUCK[] = "_NORTHWEST_STRUCK";
DATA(0x0020d6c4)
static const char s_d48_NORTH_STRUCK[] = "_NORTH_STRUCK";
DATA(0x0020d6ac)
static const char s_d48_NORTHEAST_STRUCK[] = "_NORTHEAST_STRUCK";
DATA(0x0020d69c)
static const char s_d48_WEST_STRUCK[] = "_WEST_STRUCK";
DATA(0x0020d68c)
static const char s_d48_EAST_STRUCK[] = "_EAST_STRUCK";
DATA(0x0020d674)
static const char s_d48_SOUTHWEST_STRUCK[] = "_SOUTHWEST_STRUCK";
DATA(0x0020d664)
static const char s_d48_SOUTH_STRUCK[] = "_SOUTH_STRUCK";
DATA(0x0020d64c)
static const char s_d48_SOUTHEAST_STRUCK[] = "_SOUTHEAST_STRUCK";
DATA(0x0020d634)
static const char s_d48_NORTHWEST_ATTACK[] = "_NORTHWEST_ATTACK";
DATA(0x0020d624)
static const char s_d48_NORTH_ATTACK[] = "_NORTH_ATTACK";
DATA(0x0020d60c)
static const char s_d48_NORTHEAST_ATTACK[] = "_NORTHEAST_ATTACK";
DATA(0x0020d5fc)
static const char s_d48_WEST_ATTACK[] = "_WEST_ATTACK";
DATA(0x0020d5ec)
static const char s_d48_EAST_ATTACK[] = "_EAST_ATTACK";
DATA(0x0020d5d4)
static const char s_d48_SOUTHWEST_ATTACK[] = "_SOUTHWEST_ATTACK";
DATA(0x0020d5c4)
static const char s_d48_SOUTH_ATTACK[] = "_SOUTH_ATTACK";
DATA(0x0020d5ac)
static const char s_d48_SOUTHEAST_ATTACK[] = "_SOUTHEAST_ATTACK";
DATA(0x0020d598)
static const char s_d48_NORTHWEST_IDLE[] = "_NORTHWEST_IDLE";
DATA(0x0020d588)
static const char s_d48_NORTH_IDLE[] = "_NORTH_IDLE";
DATA(0x0020d574)
static const char s_d48_NORTHEAST_IDLE[] = "_NORTHEAST_IDLE";
DATA(0x0020d564)
static const char s_d48_WEST_IDLE[] = "_WEST_IDLE";
DATA(0x0020d554)
static const char s_d48_EAST_IDLE[] = "_EAST_IDLE";
DATA(0x0020d540)
static const char s_d48_SOUTHWEST_IDLE[] = "_SOUTHWEST_IDLE";
DATA(0x0020d530)
static const char s_d48_SOUTH_IDLE[] = "_SOUTH_IDLE";
DATA(0x0020d51c)
static const char s_d48_SOUTHEAST_IDLE[] = "_SOUTHEAST_IDLE";
DATA(0x0020d508)
static const char s_d48_NORTHWEST_ITEM[] = "_NORTHWEST_ITEM";
DATA(0x0020d4f8)
static const char s_d48_NORTH_ITEM[] = "_NORTH_ITEM";
DATA(0x0020d4e4)
static const char s_d48_NORTHEAST_ITEM[] = "_NORTHEAST_ITEM";
DATA(0x0020d4d4)
static const char s_d48_WEST_ITEM[] = "_WEST_ITEM";
DATA(0x0020d4c4)
static const char s_d48_EAST_ITEM[] = "_EAST_ITEM";
DATA(0x0020d4b0)
static const char s_d48_SOUTHWEST_ITEM[] = "_SOUTHWEST_ITEM";
DATA(0x0020d4a0)
static const char s_d48_SOUTH_ITEM[] = "_SOUTH_ITEM";
DATA(0x0020d48c)
static const char s_d48_SOUTHEAST_ITEM[] = "_SOUTHEAST_ITEM";
static const char s_d48_DEATH[] = "_DEATH";
DATA(0x0020d47c)
static const char s_d48_NORTHWEST[] = "_NORTHWEST";
DATA(0x0020d474)
static const char s_d48_NORTH[] = "_NORTH";
DATA(0x0020d464)
static const char s_d48_NORTHEAST[] = "_NORTHEAST";
DATA(0x0020d45c)
static const char s_d48_WEST[] = "_WEST";
DATA(0x0020d454)
static const char s_d48_EAST[] = "_EAST";
DATA(0x0020d444)
static const char s_d48_SOUTHWEST[] = "_SOUTHWEST";
DATA(0x0020d43c)
static const char s_d48_SOUTH[] = "_SOUTH";
DATA(0x0020d42c)
static const char s_d48_SOUTHEAST[] = "_SOUTHEAST";
DATA(0x0020d424)
static const char s_d48_BREAK[] = "_BREAK";

DATA(0x001e9738)
double g_val_1e9738;

RVA(0x00048470, 0x131b)
void CGrunt::LoadCellAnimNames(i32 kind, i32 dirOnly) {
    if (kind == 0) {
        m_cells[0].WalkName() = s_GRUNTZ_ + m_animSetName + s_d48_NORTHWEST_WALK;
        m_cells[1].WalkName() = s_GRUNTZ_ + m_animSetName + s_d48_NORTH_WALK;
        m_cells[2].WalkName() = s_GRUNTZ_ + m_animSetName + s_d48_NORTHEAST_WALK;
        m_cells[3].WalkName() = s_GRUNTZ_ + m_animSetName + s_d48_WEST_WALK;
        m_cells[4].WalkName() = s_GRUNTZ_ + m_animSetName + s_d48_NORTH_WALK;
        m_cells[5].WalkName() = s_GRUNTZ_ + m_animSetName + s_d48_EAST_WALK;
        m_cells[6].WalkName() = s_GRUNTZ_ + m_animSetName + s_d48_SOUTHWEST_WALK;
        m_cells[7].WalkName() = s_GRUNTZ_ + m_animSetName + s_d48_SOUTH_WALK;
        m_cells[8].WalkName() = s_GRUNTZ_ + m_animSetName + s_d48_SOUTHEAST_WALK;
        m_cells[0].StruckName() = s_GRUNTZ_ + m_animSetName + s_d48_NORTHWEST_STRUCK;
        m_cells[1].StruckName() = s_GRUNTZ_ + m_animSetName + s_d48_NORTH_STRUCK;
        m_cells[2].StruckName() = s_GRUNTZ_ + m_animSetName + s_d48_NORTHEAST_STRUCK;
        m_cells[3].StruckName() = s_GRUNTZ_ + m_animSetName + s_d48_WEST_STRUCK;
        m_cells[4].StruckName() = s_GRUNTZ_ + m_animSetName + s_d48_NORTH_STRUCK;
        m_cells[5].StruckName() = s_GRUNTZ_ + m_animSetName + s_d48_EAST_STRUCK;
        m_cells[6].StruckName() = s_GRUNTZ_ + m_animSetName + s_d48_SOUTHWEST_STRUCK;
        m_cells[7].StruckName() = s_GRUNTZ_ + m_animSetName + s_d48_SOUTH_STRUCK;
        m_cells[8].StruckName() = s_GRUNTZ_ + m_animSetName + s_d48_SOUTHEAST_STRUCK;
        m_cells[0].AttackName() = s_GRUNTZ_ + m_animSetName + s_d48_NORTHWEST_ATTACK;
        m_cells[1].AttackName() = s_GRUNTZ_ + m_animSetName + s_d48_NORTH_ATTACK;
        m_cells[2].AttackName() = s_GRUNTZ_ + m_animSetName + s_d48_NORTHEAST_ATTACK;
        m_cells[3].AttackName() = s_GRUNTZ_ + m_animSetName + s_d48_WEST_ATTACK;
        m_cells[4].AttackName() = s_GRUNTZ_ + m_animSetName + s_d48_NORTH_ATTACK;
        m_cells[5].AttackName() = s_GRUNTZ_ + m_animSetName + s_d48_EAST_ATTACK;
        m_cells[6].AttackName() = s_GRUNTZ_ + m_animSetName + s_d48_SOUTHWEST_ATTACK;
        m_cells[7].AttackName() = s_GRUNTZ_ + m_animSetName + s_d48_SOUTH_ATTACK;
        m_cells[8].AttackName() = s_GRUNTZ_ + m_animSetName + s_d48_SOUTHEAST_ATTACK;
        m_cells[0].IdleName() = s_GRUNTZ_ + m_animSetName + s_d48_NORTHWEST_IDLE;
        m_cells[1].IdleName() = s_GRUNTZ_ + m_animSetName + s_d48_NORTH_IDLE;
        m_cells[2].IdleName() = s_GRUNTZ_ + m_animSetName + s_d48_NORTHEAST_IDLE;
        m_cells[3].IdleName() = s_GRUNTZ_ + m_animSetName + s_d48_WEST_IDLE;
        m_cells[4].IdleName() = s_GRUNTZ_ + m_animSetName + s_d48_NORTH_IDLE;
        m_cells[5].IdleName() = s_GRUNTZ_ + m_animSetName + s_d48_EAST_IDLE;
        m_cells[6].IdleName() = s_GRUNTZ_ + m_animSetName + s_d48_SOUTHWEST_IDLE;
        m_cells[7].IdleName() = s_GRUNTZ_ + m_animSetName + s_d48_SOUTH_IDLE;
        m_cells[8].IdleName() = s_GRUNTZ_ + m_animSetName + s_d48_SOUTHEAST_IDLE;
        m_cells[0].ItemName() = s_GRUNTZ_ + m_animSetName + s_d48_NORTHWEST_ITEM;
        m_cells[1].ItemName() = s_GRUNTZ_ + m_animSetName + s_d48_NORTH_ITEM;
        m_cells[2].ItemName() = s_GRUNTZ_ + m_animSetName + s_d48_NORTHEAST_ITEM;
        m_cells[3].ItemName() = s_GRUNTZ_ + m_animSetName + s_d48_WEST_ITEM;
        m_cells[4].ItemName() = s_GRUNTZ_ + m_animSetName + s_d48_NORTH_ITEM;
        m_cells[5].ItemName() = s_GRUNTZ_ + m_animSetName + s_d48_EAST_ITEM;
        m_cells[6].ItemName() = s_GRUNTZ_ + m_animSetName + s_d48_SOUTHWEST_ITEM;
        m_cells[7].ItemName() = s_GRUNTZ_ + m_animSetName + s_d48_SOUTH_ITEM;
        m_cells[8].ItemName() = s_GRUNTZ_ + m_animSetName + s_d48_SOUTHEAST_ITEM;
        m_deathFrameSetName = s_GRUNTZ_ + m_animSetName + s_d48_DEATH;
    } else if (dirOnly != 0) {
        m_cells[0].WalkName() = s_GRUNTZ_ + m_animSetName + s_d48_NORTHWEST;
        m_cells[1].WalkName() = s_GRUNTZ_ + m_animSetName + s_d48_NORTH;
        m_cells[2].WalkName() = s_GRUNTZ_ + m_animSetName + s_d48_NORTHEAST;
        m_cells[3].WalkName() = s_GRUNTZ_ + m_animSetName + s_d48_WEST;
        m_cells[4].WalkName() = s_GRUNTZ_ + m_animSetName + s_d48_NORTH;
        m_cells[5].WalkName() = s_GRUNTZ_ + m_animSetName + s_d48_EAST;
        m_cells[6].WalkName() = s_GRUNTZ_ + m_animSetName + s_d48_SOUTHWEST;
        m_cells[7].WalkName() = s_GRUNTZ_ + m_animSetName + s_d48_SOUTH;
        m_cells[8].WalkName() = s_GRUNTZ_ + m_animSetName + s_d48_SOUTHEAST;
        m_frameSetName = s_GRUNTZ_ + m_animSetName + s_d48_BREAK;
    } else {
        m_frameSetName = s_GRUNTZ_ + m_animSetName;
    }
    CShadeTable* sel = g_gameReg->m_spriteFactory->GetSel(IDX(m_moveIcon), kind);
    CWwdGameObjectA* h = m_object;
    ShadeMode fillCmd = h->m_drawFillCmd;

    m_object->m_drawActive = 1;
    h->m_drawFillCmd = fillCmd;
    h->m_drawFillArg = sel;
}

RVA(0x00049c60, 0x8d1)
void CGrunt::LoadAnimNameTable(i32 kind, i32 toyOnly) {
    if (kind == 0) {
        LOAD_POSE(m_poseWalk, s_pose_WALK);
        LOAD_POSE(AT(m_poseAttack, GRUNT_ATTACK1), s_pose_ATTACK1);
        LOAD_POSE(AT(m_poseAttack, GRUNT_ATTACK2), s_pose_ATTACK2);
        LOAD_POSE(m_poseAttackIdle, s_pose_ATTACKIDLE);
        LOAD_POSE(AT(m_poseStruck, GRUNT_STRUCK1), s_pose_STRUCK1);
        LOAD_POSE(AT(m_poseStruck, GRUNT_STRUCK2), s_pose_STRUCK2);
        LOAD_POSE(AT(m_poseIdle, GRUNT_IDLE1), s_pose_IDLE1);
        LOAD_POSE(AT(m_poseIdle, GRUNT_IDLE2), s_pose_IDLE2);
        LOAD_POSE(AT(m_poseIdle, GRUNT_IDLE3), s_pose_IDLE3);
        LOAD_POSE(AT(m_poseIdle, GRUNT_IDLE4), s_pose_IDLE4);
        LOAD_POSE(AT(m_poseIdle, GRUNT_IDLE5), s_pose_IDLE5);
        LOAD_POSE(AT(m_poseItem, GRUNT_ITEM1), s_pose_ITEM);
        LOAD_POSE(AT(m_poseItem, GRUNT_ITEM2), s_pose_ITEM2);
        LOAD_POSE(m_poseDeath, s_pose_DEATH);
        return;
    }

    if (toyOnly != 0) {
        LOAD_POSE(m_poseWalk, s_pose_WALK);
    } else {
        LOAD_POSE(AT(m_poseToy, GRUNT_TOY1), s_pose_TOY1);

        i32 x = AT(m_poseToy, GRUNT_TOY1)->m_records.GetSize();
        LOAD_POSE(AT(m_poseToy, GRUNT_TOY2), s_pose_TOY2);
        i32 y = AT(m_poseToy, GRUNT_TOY2)->m_records.GetSize();

        if (x < y) {
            double blend =
                DATA_COMPGEN(0x001e9748, fp_1e9748, 100.0) / (static_cast<double>(y) / x - DATA_COMPGEN(0x001e9740, fp_1e9740, -1.0)) - g_slopeNegHalf;
            i32 pct = static_cast<i32>(blend);
            m_toyBlendPct = 100 - pct;
        } else {
            m_toyBlendPct =
                static_cast<i32>((100.0 / (static_cast<double>(x) / y - -1.0) - g_slopeNegHalf));
        }
    }

    LOAD_POSE(AT(m_poseToy, GRUNT_TOY_BREAK), s_pose_TOYBREAK);
}

#undef LOAD_POSE

// @early-stop
// residue is the FP shape of the second difference (retail loads both operands
// and fxch/fsubp where cl folds one into fsubr) plus the ebx/edi swap that
// follows from it.
RVA(0x0004a780, 0x1ec)
GruntDirectionCell* MotionEntity::Classify(MotionEntity* other, char exact) {
    if (other == NULL) {
        return &g_gruntMoveDirCenter;
    }
    i32 dy = static_cast<i32>((other->m_positionX - m_positionX));
    double otherY = other->m_positionY;
    i32 dx = static_cast<i32>((m_positionY - otherY));
    if (dy == 0) {
        if (dx > 0) {
            return &g_gruntMoveDirNorth;
        }
        if (dx < 0) {
            return &g_gruntMoveDirSouth;
        }
        return &g_gruntMoveDirCenter;
    }

    char onCell = exact;
    if (onCell) {
        onCell =
            (static_cast<i32>(m_positionX) == m_gridX && static_cast<i32>(m_positionY) == m_gridY)
                ? 1
                : 0;
    }
    double ratio = static_cast<double>(dx) / static_cast<double>(dy);

    if (dx >= 0 && dy > 0) {
        if (onCell) {
            return &g_gruntMoveDirNorthEast;
        }
        if (ratio <= g_slopePosHalf) {
            return &g_gruntMoveDirEast;
        }
        if (ratio <= g_slopePosTwo) {
            return &g_gruntMoveDirNorthEast;
        }
        return &g_gruntMoveDirNorth;
    }
    if (dx >= 0) {
        if (onCell) {
            return &g_gruntMoveDirNorthWest;
        }
        if (ratio <= g_slopeNegTwo) {
            return &g_gruntMoveDirNorth;
        }
        if (ratio <= g_slopeNegHalf) {
            return &g_gruntMoveDirNorthWest;
        }
        return &g_gruntMoveDirWest;
    }
    if (dy > 0) {
        if (onCell) {
            return &g_gruntMoveDirSouthEast;
        }
        if (ratio <= g_slopeNegTwo) {
            return &g_gruntMoveDirSouth;
        }
        if (ratio <= g_slopeNegHalf) {
            return &g_gruntMoveDirSouthEast;
        }
        return &g_gruntMoveDirEast;
    }

    if (onCell) {
        return &g_gruntMoveDirSouthWest;
    }
    if (ratio <= g_slopePosHalf) {
        return &g_gruntMoveDirWest;
    }
    if (ratio <= g_slopePosTwo) {
        return &g_gruntMoveDirSouthWest;
    }
    return &g_gruntMoveDirSouth;
}

RVA(0x0004a9f0, 0x1aa)
i32 CGrunt::IntersectsTileObjectAxes() {
    CGrunt* tgt = m_tileMgr->FindAtPixel(m_object->m_screenX, m_object->m_screenY);
    if (tgt == NULL) {
        return 0;
    }
    RECT r;
    CopyRect(&r, &tgt->m_wwdObject->m_area);
    CGameObject* th = tgt->m_object;
    OffsetRect(&r, th->m_screenX, th->m_screenY);

    POINT a, b;

    b.x = m_object->m_screenX;
    b.y = m_object->m_screenY - 0x3e8;
    a.x = m_object->m_screenX;
    a.y = m_object->m_screenY + 0x3e8;
    if (RectSegProbe(&r, &b, &a)) {
        return 1;
    }

    b.x = m_object->m_screenX - 0x3e8;
    b.y = m_object->m_screenY;
    a.x = m_object->m_screenX + 0x3e8;
    a.y = m_object->m_screenY;
    if (RectSegProbe(&r, &b, &a)) {
        return 1;
    }

    b.x = m_object->m_screenX - 0x3e8;
    b.y = m_object->m_screenY - 0x3e8;
    a.x = m_object->m_screenX + 0x3e8;
    a.y = m_object->m_screenY + 0x3e8;
    if (RectSegProbe(&r, &b, &a)) {
        return 1;
    }

    b.x = m_object->m_screenX - 0x3e8;
    b.y = m_object->m_screenY + 0x3e8;
    a.x = m_object->m_screenX + 0x3e8;
    a.y = m_object->m_screenY - 0x3e8;
    return RectSegProbe(&r, &b, &a) != 0;
}

// @early-stop
RVA(0x0004ac10, 0x402)
void CGrunt::PlaySound(i32 range, GruntDirectionCell rec) {
    static_cast<void>(range);
    if (SameCellTag(&m_entranceCell, &rec)) {
        return;
    }

    bool eq;
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), s_codeF) == 0);
    if (eq) {
        return;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), s_codeD) == 0);
    if (eq) {
        goto walk;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), s_codeA) == 0);
    if (eq) {
        goto idle;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), s_codeK) == 0);
    if (eq) {
        goto idle;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), s_codeE) == 0);
    if (eq) {

        m_value = m_wwdObject->m_animCursor.m_animation;
        m_wwdObject->m_animCursor.Setup(m_poseAttackIdle);
        {
            CAniElement* desc = m_wwdObject->m_animCursor.m_animation;
            CAniRecordView* elem = desc->m_records.GetSize() > 0
                                       ? static_cast<CAniRecordView*>(desc->m_records.GetAt(0))
                                       : 0;
            i32 frame = elem->m_param;
            const char* nm = EntranceCell()->AttackName().GetBuffer(0);
            m_wwdObject->ApplyLookupSprite(nm, frame);
        }
        goto store;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), s_codeI) == 0);
    if (eq) {
        goto codeI;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), s_codeM) == 0);
    if (!eq) {
        goto walk;
    }

codeI:

    m_entranceCell.row = rec.row;
    m_entranceCell.column = rec.column;
    m_entranceCell.direction = rec.direction;
    m_value = m_wwdObject->m_animCursor.m_animation;
    m_wwdObject->m_animCursor.Setup(AT(m_poseIdle, GRUNT_IDLE2));
    ResetEntranceAnimation(1, 0, 0);
    return;

idle:

    m_value = m_wwdObject->m_animCursor.m_animation;
    m_wwdObject->ApplyGeometryDirect(AT(m_poseIdle, GRUNT_IDLE1), 0);
    {
        CAniElement* desc = m_wwdObject->m_animCursor.m_animation;
        CAniRecordView* elem = desc->m_records.GetSize() > 0
                                   ? static_cast<CAniRecordView*>(desc->m_records.GetAt(0))
                                   : 0;
        i32 frame = elem->m_param;
        i32 row = rec.row;
        i32 column = rec.column;
        i32 index = 3 * row + column;

        const char* nm = m_cells[index].IdleName().GetBuffer(0);
        m_wwdObject->ApplyLookupSprite(nm, frame);
    }
    goto store;

walk:

    m_value = m_wwdObject->m_animCursor.m_animation;
    m_wwdObject->m_animCursor.Setup(m_poseWalk);
    {
        i32 row = rec.row;
        i32 column = rec.column;
        i32 index = 3 * row + column;

        const char* nm = m_cells[index].WalkName().GetBuffer(0);
        m_wwdObject->ApplyName(nm);
    }

store:
    m_entranceCell.row = rec.row;
    m_entranceCell.column = rec.column;
    m_entranceCell.direction = rec.direction;
}

// @early-stop
RVA(0x0004b130, 0xc8)
i32 CGrunt::CommitArrival() {
    if (m_arrived != 0) {
        return 1;
    }

    if (m_tileClaimed != 0 && g_gameReg->m_gameMode == GAMEMODE_MULTIPLAYER) {
        m_tileMgr->GridAction7(m_tileOwnerHi, m_tileOwnerLo);
    } else if (m_tileClaimed != 0) {
        m_arrivalRerollLo = 0;
        m_arrivalRerollWindowLo = 0;
        m_arrivalRerollHi = 0;
        m_arrivalRerollWindowHi = 0;
        m_tileClaimed = 0;
        m_arrivalState = AI_NONE;
        m_arrivalFlags &= 0xe7fbfbfd;
        SetEntrancePos(1, 1);
    }
    CreateSelectedSprite();
    CreateHealthSprite();
    CreateToySprite();
    CreateStaminaSprite();
    CreateToyTimeSprite();
    CreateWingzTimeSprite();
    m_arrived = 1;
    return 1;
}

RVA(0x0004b240, 0xaa)
void CGrunt::ClearAllSprites() {
    if (m_selectedSprite) {
        m_selectedSprite->m_flags |= 0x10000;
        m_selectedSprite = NULL;
    }
    if (m_healthSprite) {
        m_healthSprite->m_flags |= 0x10000;
        m_healthSprite = NULL;
    }
    if (m_toySprite) {
        m_toySprite->m_flags |= 0x10000;
        m_toySprite = NULL;
    }
    if (m_entranceCommitted == 0) {
        if (m_staminaSprite) {
            m_staminaSprite->m_flags |= 0x10000;
            m_staminaSprite = NULL;
        }
        if (m_toyTimeSprite) {
            m_toyTimeSprite->m_flags |= 0x10000;
            m_toyTimeSprite = NULL;
        }
        if (m_wingzTimeSprite) {
            m_wingzTimeSprite->m_flags |= 0x10000;
            m_wingzTimeSprite = NULL;
        }
    }
    m_arrived = 0;
}

// @early-stop
// @early-stop
// regalloc wall: retail shuttles the four pass-through args through esi
// (push esi/pop esi) so col+row stay live from the first load; cl keeps only
// two scratch regs and materialises px/py last. 49 permuter variants exhausted.
RVA(0x0004b320, 0x34)
i32 CGrunt::TileSwitch(i32 col, i32 row, i32 arrivalPhase, i32 maskA, i32 clearFlag, i32 maskCIn) {
    i32 px = col * 0x20 + 0x10;
    i32 py = row * 0x20 + 0x10;
    return StepArrivalDrop(px, py, arrivalPhase, maskA, clearFlag, maskCIn);
}

// @early-stop
// objdiff pairs the symbol but scores 0; retail also carries one more
// 4-byte local, so every parameter offset in our frame is short by four.
RVA(0x0004b370, 0xb30)
i32 CGrunt::StepArrivalDrop(
    i32 pxX,
    i32 pxY,
    i32 arrivalPhase,
    i32 maskA,
    i32 clearFlag,
    i32 maskCIn
) {
    CoordNode* n;
    CoordNode* cur;
    CoordPoolNode* pooled;
    Coord* tail;
    POSITION pos;
    i32 lastX, lastY, tileX, tileY;
    i32 maskC, cnt, headFlags, lastFlags, hit;
    i32 reinit;
    i32 nudged;
    RockNeighborMask free4;
    i32 step, acc, err, walkX, walkY, blocked;
    i32 saved[3][3];
    i32 sx, sy;
    bool eq;

    m_pendingTrigger = 0;
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), s_codeD) != 0);
    if (!eq && pxX == m_entrancePx.m_x && pxY == m_entrancePx.m_y) {
        goto commitPhase;
    }

    if (CoordCount() != 0) {
        n = CoordHead();
        while (n != NULL) {
            cur = n;
            n = n->m_next;
            if (cur->m_coord != NULL) {
                g_coordPool.Push(cur->m_coord);
            }
        }
        m_coordList.RemoveAll();
    }
    lastX = m_lastTilePx.m_x >> TILE_SHIFT_PX;
    lastY = m_lastTilePx.m_y >> TILE_SHIFT_PX;
    tileX = pxX >> TILE_SHIFT_PX;
    tileY = pxY >> TILE_SHIFT_PX;
    if (maskA == -1) {
        maskA = m_arrivalFlags;
    }
    m_arrivalTargetPx.m_x = pxX;
    m_arrivalTargetPx.m_y = pxY;
    maskC = maskCIn | m_passableMask;
    if (g_gameReg->m_tileGrid
            ->SearchEdge(lastX, lastY, tileX, tileY, &m_coordList, clearFlag, maskA, maskC)
        == 0) {
        goto nudgeTarget;
    }
dropHead:
    if (CoordCount() != 0) {
        pooled = g_coordPool.NodeOf(m_coordList.RemoveHead());
        pooled->m_next = g_coordPool.m_freeHead;
        g_coordPool.m_freeHead = pooled;
    }
pathGate:
    reinit = 1;
    cnt = CoordCount();
    if (cnt == 0) {
        goto commitEntrance;
    }
    tail = CoordHead()->m_coord;
    headFlags = (static_cast<u32>(tail->m_x) >= g_gameReg->m_tileGrid->m_width
                 || static_cast<u32>(tail->m_y) >= g_gameReg->m_tileGrid->m_height)
                    ? 1
                    : g_gameReg->m_tileGrid->m_rowInts[tail->m_y][tail->m_x * 7];
    lastFlags = (static_cast<u32>(lastX) >= g_gameReg->m_tileGrid->m_width
                 || static_cast<u32>(lastY) >= g_gameReg->m_tileGrid->m_height)
                    ? 1
                    : g_gameReg->m_tileGrid->m_rowInts[lastY][lastX * 7];
    if ((lastFlags & 0x80) != 0) {
        goto commitEntrance;
    }
    if ((headFlags & 0x20000000) == 0) {
        hit = headFlags & maskA;
        if ((hit & 0x20000000) == 0) {
            if (hit == 0) {
                goto commitEntrance;
            }
            if ((maskC & headFlags) != 0) {
                goto commitEntrance;
            }
        }
    }
    if (cnt == 1 && m_arrivalPending == 0) {

        SetEntrancePos(1, 1);
        if (m_object->m_screenX == m_lastTilePx.m_x && m_object->m_screenY == m_lastTilePx.m_y) {
            PlayMoveSoundAtTile(tileX, tileY);
        }
        return 0;
    }
    if (m_arrivalState == AI_BATTLEZ_PATH) {
        reinit = 0;
        goto commitEntrance;
    }
    {

        CPtrList probe(10);
        if (g_gameReg->m_tileGrid->SearchEdge(
                lastX,
                lastY,
                tileX,
                tileY,
                &probe,
                clearFlag,
                maskA | 0x20000000,
                maskC
            ) != 0
            && probe.GetCount() != 0) {
            if (probe.GetCount() > cnt + 3) {
                pos = probe.GetHeadPosition();
                while (pos != NULL) {
                    pooled = g_coordPool.NodeOf(probe.GetNext(pos));
                    pooled->m_next = g_coordPool.m_freeHead;
                    g_coordPool.m_freeHead = pooled;
                }
            } else {
                pooled = g_coordPool.NodeOf(probe.RemoveHead());
                pooled->m_next = g_coordPool.m_freeHead;
                g_coordPool.m_freeHead = pooled;
                if (CoordCount() != 0) {
                    n = CoordHead();
                    while (n != NULL) {
                        cur = n;
                        n = n->m_next;
                        if (cur->m_coord != NULL) {
                            pooled = g_coordPool.NodeOf(cur->m_coord);
                            pooled->m_next = g_coordPool.m_freeHead;
                            g_coordPool.m_freeHead = pooled;
                        }
                    }
                    m_coordList.RemoveAll();
                }
                pos = probe.GetHeadPosition();
                while (pos != NULL) {
                    m_coordList.AddTail(probe.GetNext(pos));
                }
            }
            probe.RemoveAll();
        }
    }
commitEntrance:
    m_entrancePx.m_x = pxX;
    m_entrancePx.m_y = pxY;
    if (reinit != 0) {
        StepEntranceReinit();
    }
commitPhase:
    m_arrivalPhase = arrivalPhase;
    return 1;

nudgeTarget:
    nudged = 0;

    if (g_gameReg->m_tileGrid->m_rowInts[tileY][tileX * 7 + 4] != IDX(TILEKIND_GIANT_ROCK)) {
        goto nudgeDone;
    }
    free4 = (g_gameReg->m_tileGrid->m_rowInts[tileY + 1][tileX * 7 + 4] == IDX(TILEKIND_GIANT_ROCK))
                ? ROCKADJ_BELOW
                : ROCKADJ_NONE;
    free4 |=
        (g_gameReg->m_tileGrid->m_rowInts[tileY - 1][tileX * 7 + 4] == IDX(TILEKIND_GIANT_ROCK))
            ? ROCKADJ_ABOVE
            : ROCKADJ_NONE;
    free4 |= (g_gameReg->m_tileGrid->m_rowInts[tileY][tileX * 7 + 11] == IDX(TILEKIND_GIANT_ROCK))
                 ? ROCKADJ_RIGHT
                 : ROCKADJ_NONE;
    free4 |= (g_gameReg->m_tileGrid->m_rowInts[tileY][tileX * 7 - 3] == IDX(TILEKIND_GIANT_ROCK))
                 ? ROCKADJ_LEFT
                 : ROCKADJ_NONE;
    switch (free4) {
        case ROCKADJ_RIGHT | ROCKADJ_BELOW:
            tileX++;
            tileY++;
            break;
        case ROCKADJ_RIGHT | ROCKADJ_ABOVE:
            tileX++;
            tileY--;
            break;
        case ROCKADJ_RIGHT | ROCKADJ_ABOVE | ROCKADJ_BELOW:
            tileX++;
            break;
        case ROCKADJ_LEFT | ROCKADJ_BELOW:
            tileX--;
            tileY++;
            break;
        case ROCKADJ_LEFT | ROCKADJ_ABOVE:
            tileX--;
            tileY--;
            break;
        case ROCKADJ_LEFT | ROCKADJ_ABOVE | ROCKADJ_BELOW:
            tileX--;
            break;
        case ROCKADJ_LEFT | ROCKADJ_RIGHT | ROCKADJ_BELOW:
            tileY++;
            break;
        case ROCKADJ_LEFT | ROCKADJ_RIGHT | ROCKADJ_ABOVE:
            tileY--;
            break;
        default:
            break;
    }

    for (sy = tileY - 1; sy < tileY + 2; sy++) {
        for (sx = tileX - 1; sx < tileX + 2; sx++) {
            saved[sx - tileX + 1][sy - tileY + 1] =
                g_gameReg->m_tileGrid->m_rowInts[sy][sx * 7 + 7];
            g_gameReg->m_tileGrid->m_rowInts[sy][sx * 7 + 7] = 0;
        }
    }
    if (g_gameReg->m_tileGrid
                ->SearchEdge(lastX, lastY, tileX, tileY, &m_coordList, clearFlag, maskA, maskC)
            != 0
        && CoordCount() != 0) {
        pooled = g_coordPool.NodeOf(m_coordList.RemoveHead());
        pooled->m_next = g_coordPool.m_freeHead;
        g_coordPool.m_freeHead = pooled;
        if (CoordCount() != 0) {
            pooled = g_coordPool.NodeOf(m_coordList.RemoveTail());
            pooled->m_next = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = pooled;
            if (CoordCount() != 0) {
                nudged = 1;
                tail = CoordTail()->m_coord;
                pxX = tail->m_x * 32 + 0x10;
                pxY = tail->m_y * 32 + 0x10;
            }
        }
    }
    for (sy = tileY - 1; sy < tileY + 2; sy++) {
        for (sx = tileX - 1; sx < tileX + 2; sx++) {
            g_gameReg->m_tileGrid->m_rowInts[sy][sx * 7 + 7] =
                saved[sx - tileX + 1][sy - tileY + 1];
        }
    }
    if (nudged != 0) {
        if (CoordCount() == 1 && arrivalPhase == IDX(PICKUP_BOOMERANG)
            && m_entranceReason == PICKUP_GAUNTLETZ) {
            m_tileMgr->ApplyTriggerA(m_tileOwnerHi, m_tileOwnerLo, pxX, pxY);
            SetEntrancePos(1, 1);
            return 1;
        }
        m_arrivalTargetPx.m_x = pxX;
        m_arrivalTargetPx.m_y = pxY;
    }
nudgeDone:
    if (nudged != 0) {
        goto pathGate;
    }
    if (m_arrivalState != AI_NONE) {
        SetEntrancePos(1, 1);
        return 0;
    }
    if (lastX == tileX && lastY == tileY) {
        goto reCommit;
    }

    blocked = 0;
    walkX = tileX;
    walkY = tileY;
    if (abs(tileX - lastX) > abs(tileY - lastY)) {
        step = ((tileY - lastY) << 16) / abs(tileX - lastX);
        acc = lastY << 16;
        sx = lastX;
        while (blocked == 0) {
            sy = acc >> 16;
            err = (static_cast<u32>(sx) >= g_gameReg->m_tileGrid->m_width
                   || static_cast<u32>(sy) >= g_gameReg->m_tileGrid->m_height)
                      ? 1
                      : g_gameReg->m_tileGrid->m_rowInts[sy][sx * 7];
            if ((maskA & err) != 0 && (m_passableMask & err) == 0) {
                blocked = 1;
            } else {
                walkX = sx;
                walkY = sy;
                acc += step;
                sx += (tileX > lastX) ? 1 : -1;
            }
        }
    } else {
        step = ((tileX - lastX) << 16) / abs(tileY - lastY);
        acc = lastX << 16;
        sy = lastY;
        while (blocked == 0) {
            sx = acc >> 16;
            err = (static_cast<u32>(sx) >= g_gameReg->m_tileGrid->m_width
                   || static_cast<u32>(sy) >= g_gameReg->m_tileGrid->m_height)
                      ? 1
                      : g_gameReg->m_tileGrid->m_rowInts[sy][sx * 7];
            if ((maskA & err) != 0 && (m_passableMask & err) == 0) {
                blocked = 1;
            } else {
                walkX = sx;
                walkY = sy;
                acc += step;
                sy += (tileY > lastY) ? 1 : -1;
            }
        }
    }
    if (lastX != walkX || lastY != walkY) {
        goto reProbe;
    }
reCommit:
    SetEntrancePos(1, 1);
    if (m_arrivalPending == 0) {
        return 0;
    }
    m_arrivalPhase = arrivalPhase;
    return 1;

reProbe:
    pxX = walkX * 32 + 0x10;
    pxY = walkY * 32 + 0x10;
    if (g_gameReg->m_tileGrid->SearchEdge(walkX, walkY, lastX, lastY, &m_coordList, 1, maskA, maskC)
        != 0) {
        goto dropHead;
    }
    SetEntrancePos(1, 1);
    if (m_object->m_screenX == m_lastTilePx.m_x && m_object->m_screenY == m_lastTilePx.m_y) {
        PlayMoveSoundAtTile(walkX, walkY);
    }
    if (m_arrivalPending == 0) {
        return 0;
    }
    m_arrivalPhase = arrivalPhase;
    return 1;
}

// @early-stop
RVA(0x0004c170, 0xbe7)
i32 CGrunt::StepGruntMovement() {
    i32 coordX, coordY;
    i32 gtX, gtY;
    GruntDirectionCell rec;
    i32 tgtPxX, tgtPxY;
    i32 flagHead;
    i32 reason12, reason16, reason0e;
    i32 tgtTileX, tgtTileY;
    CGruntzMapMgr* bd;

    {
        i32 entX = m_entrancePx.m_x;
        i32 lastX = m_lastTilePx.m_x;
        i32 entY = m_entrancePx.m_y;
        if (lastX == entX && m_lastTilePx.m_y == entY) {
            goto label_ret1;
        }
    }
    if (m_arrivalState == AI_BATTLEZ_PATH) {
        CBattlezMapConfig* slot = &g_gameReg->m_options[m_tileOwnerHi].m_battlezConfig;
        if (slot != NULL && slot->ValidateUnitPath(this) == 0) {
            SetEntrancePos(1, 1);
            return 0;
        }
    }
    if (CoordCount() == 0) {
        goto label_dropRet0;
    }
    if (m_arrivalState != AI_BATTLEZ_PATH) {
        Coord* co = static_cast<Coord*>(m_coordList.RemoveHead());
        coordX = co->m_x;
        coordY = co->m_y;
        CoordPoolNode* p = g_coordPool.NodeOf(co);
        p->m_next = g_coordPool.m_freeHead;
        g_coordPool.m_freeHead = p;
    } else {
        Coord* co = CoordHead()->m_coord;
        coordX = co->m_x;
        coordY = co->m_y;
    }

    gtX = m_object->m_screenX >> TILE_SHIFT_PX;
    gtY = m_object->m_screenY >> TILE_SHIFT_PX;
    if (coordX > gtX) {
        if (coordY > gtY) {
            rec = g_gruntMoveDirSouthEast;
        } else if (coordY == gtY) {
            rec = g_gruntMoveDirEast;
        } else {
            rec = g_gruntMoveDirNorthEast;
        }
    } else if (coordX < gtX) {
        if (coordY > gtY) {
            rec = g_gruntMoveDirSouthWest;
        } else if (coordY == gtY) {
            rec = g_gruntMoveDirWest;
        } else {
            rec = g_gruntMoveDirNorthWest;
        }
    } else {
        if (coordY < gtY) {
            rec = g_gruntMoveDirNorth;
        } else {
            rec = g_gruntMoveDirSouth;
        }
    }

    tgtPxY = (coordY << TILE_SHIFT_PX) + TILE_HALF_PX;
    tgtPxX = (coordX << TILE_SHIFT_PX) + TILE_HALF_PX;
    bd = g_gameReg->m_tileGrid;
    tgtTileX = tgtPxX >> TILE_SHIFT_PX;
    tgtTileY = tgtPxY >> TILE_SHIFT_PX;
    if (static_cast<u32>(tgtTileX) < static_cast<u32>(bd->m_width)
        && static_cast<u32>(tgtTileY) < static_cast<u32>(bd->m_height)) {
        flagHead = bd->m_rowInts[tgtTileY][tgtTileX * 7];
    } else {
        flagHead = 1;
    }

    {
        EnemyAiType st = m_arrivalState;
        i32 blockMove = 1;
        if (st == AI_OBJECTGUARD) {
            if (((m_defenderPx.m_x ^ tgtPxX) & 0xffffffe0) == 0
                && ((m_defenderPx.m_y ^ tgtPxY) & 0xffffffe0) == 0) {
                blockMove = 0;
            }
        }
        if (blockMove != 0 && !(flagHead & 0x20000000)) {
            i32 mask = m_arrivalFlags & flagHead;
            if (!(mask & 0x20000000)) {
                if (mask == 0) {
                    goto label_4c6e4;
                }
                if (flagHead & m_passableMask) {
                    goto label_4c6e4;
                }
            }
        }
    }
    if (m_entranceActive != 0) {
        goto label_4c68b;
    }
    {
        i32 lastFlag;
        i32 ltx = m_lastTilePx.m_x >> TILE_SHIFT_PX;
        i32 lty = m_lastTilePx.m_y >> TILE_SHIFT_PX;
        if (static_cast<u32>(ltx) < static_cast<u32>(bd->m_width)
            && static_cast<u32>(lty) < static_cast<u32>(bd->m_height)) {
            lastFlag = bd->m_rowInts[lty][ltx * 7];
        } else {
            lastFlag = 1;
        }
        if (lastFlag & 0x80) {
            goto label_4c68b;
        }
    }
    if (m_arrivalState == AI_BATTLEZ_PATH) {
        goto label_4cb2a;
    }
    if (CoordCount() == 0) {
        goto label_4cb2a;
    }
    {
        i32 mask = m_arrivalFlags & flagHead;
        if (mask & 0x20000000) {
            goto label_4cb2a;
        }
        if (mask != 0 && !(flagHead & m_passableMask)) {
            goto label_4cb2a;
        }
    }
    if (!(flagHead & 0x20000000)) {
        goto label_4c6e4;
    }
    {
        void* node = 0;
        CoordPoolNode* head = g_coordPool.m_freeHead;
        if (head->m_next != NULL) {
            node = &head->m_coord;
            g_coordPool.m_freeHead = head->m_next;
        }
        (static_cast<i32*>(node))[0] = tgtTileX;
        (static_cast<i32*>(node))[1] = tgtTileY;
        m_coordList.AddHead(node);
    }
    if (PathScan() == 0) {
        PlaySound(0x3e8, rec);
        SetEntrancePos(1, 0);
        return 0;
    }

    if (CoordCount() == 0) {
        goto label_4cb2a;
    }
    {
        Coord* co = CoordHead()->m_coord;
        i32 cx = co->m_x;
        i32 cy = co->m_y;
        tgtPxX = (cx << TILE_SHIFT_PX) + TILE_HALF_PX;
        tgtPxY = (cy << TILE_SHIFT_PX) + TILE_HALF_PX;
        i32 gx = m_object->m_screenX >> TILE_SHIFT_PX;
        i32 gy = m_object->m_screenY >> TILE_SHIFT_PX;
        if (cx > gx) {
            if (cy > gy) {
                rec = g_gruntMoveDirSouthEast;
            } else if (cy == gy) {
                rec = g_gruntMoveDirEast;
            } else {
                rec = g_gruntMoveDirNorthEast;
            }
        } else if (cx < gx) {
            if (cy > gy) {
                rec = g_gruntMoveDirSouthWest;
            } else if (cy == gy) {
                rec = g_gruntMoveDirWest;
            } else {
                rec = g_gruntMoveDirNorthWest;
            }
        } else {
            if (cy < gy) {
                rec = g_gruntMoveDirNorth;
            } else {
                rec = g_gruntMoveDirSouth;
            }
        }
        CGruntzMapMgr* bd = g_gameReg->m_tileGrid;
        if (bd->m_rowInts[cy][cx * 7] & 0x20000000) {
            PlaySound(0x3e8, rec);
            SetEntrancePos(1, 0);
            return 0;
        }
        Coord* co2 = static_cast<Coord*>(m_coordList.RemoveHead());
        CoordPoolNode* p = g_coordPool.NodeOf(co2);
        p->m_next = g_coordPool.m_freeHead;
        g_coordPool.m_freeHead = p;
        goto label_4c6e4;
    }

label_4c68b:
    if ((flagHead & 0x20000000) && !(flagHead & 0x80)) {
        i32 owner;
        if (static_cast<u32>(tgtTileX) < static_cast<u32>(bd->m_width)
            && static_cast<u32>(tgtTileY) < static_cast<u32>(bd->m_height)) {
            owner = bd->m_rowInts[tgtTileY][tgtTileX * 7 + 1];
        } else {
            owner = -1;
        }
        m_tileMgr->CellDispatch((owner >> 8) & 0xff, owner & 0xff, DEATH_SQUASH, m_tileOwnerHi);
    }

label_4c6e4:
    if (m_arrivalState == AI_BATTLEZ_PATH && CoordCount() != 0) {
        Coord* co = static_cast<Coord*>(m_coordList.RemoveHead());
        CoordPoolNode* p = g_coordPool.NodeOf(co);
        p->m_next = g_coordPool.m_freeHead;
        g_coordPool.m_freeHead = p;
    }
    if (flagHead & 0x80) {
        m_entranceActive = 1;
    } else {
        CString* r = g_typeColl.ScratchResolve(m_objAux->m_actKey);
        GruntScratchTeardown();
        bool ne;
        ne = (strcmp(*r, "L") != 0);
        if (ne) {
            m_entranceActive = 0;
        }
    }

    reason12 = 0;
    reason16 = 0;
    reason0e = 0;
    if (m_entranceReason == PICKUP_TOOB) {
        reason12 = 1;
    } else if (m_entranceReason == PICKUP_WINGZ) {
        reason16 = 1;
    } else if (m_entranceReason == PICKUP_SPRING) {
        reason0e = 1;
    }
    if (reason0e == 0) {
        goto label_4cb4b;
    }

    if (!(flagHead & 0x1400)) {
        if (!(flagHead & 0x2)) {
            goto label_4cb4b;
        }
    }
    if (tgtPxX == m_entrancePx.m_x && tgtPxY == m_entrancePx.m_y) {
        if ((flagHead & BRICKZ_BLOCKED_MASK) == 0) {
            goto label_4c92b;
        }
        goto label_4cb2a;
    }
    {
        i32 beyondPxX = tgtPxX * 2 - m_lastTilePx.m_x;
        i32 beyondPxY = tgtPxY * 2 - m_lastTilePx.m_y;
        i32 btx = beyondPxX >> TILE_SHIFT_PX;
        i32 bty = beyondPxY >> TILE_SHIFT_PX;
        i32 beyondFlag;
        CGruntzMapMgr* bd = g_gameReg->m_tileGrid;
        if (static_cast<u32>(btx) < static_cast<u32>(bd->m_width)
            && static_cast<u32>(bty) < static_cast<u32>(bd->m_height)) {
            beyondFlag = bd->m_rowInts[bty][btx * 7];
        } else {
            beyondFlag = 1;
        }
        if (beyondFlag & 0x20000939) {
            goto label_4cb2a;
        }
        if (CoordCount() != 0 && m_arrivalState != AI_BATTLEZ_PATH) {
            Coord* co = static_cast<Coord*>(m_coordList.RemoveHead());
            if (co->m_x == btx && co->m_y == bty) {
                CoordPoolNode* p = g_coordPool.NodeOf(co);
                p->m_next = g_coordPool.m_freeHead;
                g_coordPool.m_freeHead = p;
            } else {
                m_coordList.AddHead(co);
            }
        }
        i32 hudY = m_object->m_screenY;
        i32 hudX = m_object->m_screenX;
        CCueRect* rr = &g_gameReg->m_world->m_level->m_mainPlane->m_viewRect;
        if (hudX < rr->right && hudX >= rr->left && hudY < rr->bottom && hudY >= rr->top) {
            g_gameReg->m_cueSink->LoadGruntSpawnConfig(this, 8, -1, -1, -1);
        }
        tgtPxY = beyondPxY;
        tgtPxX = beyondPxX;
    }

label_4c92b: {
    i32 lastTileX = m_lastTilePx.m_x >> TILE_SHIFT_PX;
    tgtTileX = tgtPxX >> TILE_SHIFT_PX;
    i32 lastTileY = m_lastTilePx.m_y >> TILE_SHIFT_PX;
    tgtTileY = tgtPxY >> TILE_SHIFT_PX;
    CGruntzMapMgr* bd = g_gameReg->m_tileGrid;
    if (lastTileX == tgtTileX && lastTileY == tgtTileY) {
        goto label_4cb4b;
    }
    i32 xbound = bd->m_width;
    if (static_cast<u32>(tgtTileX) >= static_cast<u32>(xbound)) {
        goto label_4cb2a;
    }
    if (static_cast<u32>(tgtTileY) >= static_cast<u32>(bd->m_height)) {
        goto label_4cb2a;
    }
    BrickzCell** rowtable = bd->m_rows;
    BrickzCell* tgtT = &rowtable[tgtTileY][tgtTileX];
    i32 tgtFlag = tgtT->m_flags;
    i32 mask = m_arrivalFlags & tgtFlag;
    if (mask & 0x20000000) {
        goto label_4cb2a;
    }
    if (mask != 0 && !(tgtFlag & m_passableMask)) {
        goto label_4cb2a;
    }
    BrickzCell* lastT = &rowtable[lastTileY][lastTileX];
    i32 dx = tgtTileX - lastTileX;
    i32 dy = tgtTileY - lastTileY;
    if (dx == 0) {
        goto label_4cb4b;
    }
    if (dy == 0) {
        goto label_4cb4b;
    }

    if (dx > 0 && dy > 0) {
        if ((lastT + 1)->m_flags & 0x2000) {
            goto label_4cb2a;
        }
        if ((lastT + xbound)->m_flags & 0x2000) {
            goto label_4cb2a;
        }
        if ((tgtT - 1)->m_flags & 0x2000) {
            goto label_4cb2a;
        }
        if (!((tgtT - xbound)->m_flags & 0x2000)) {
            goto label_4cb4b;
        }
        goto label_4cb2a;
    } else if (dx < 0 && dy > 0) {
        if ((lastT - 1)->m_flags & 0x2000) {
            goto label_4cb2a;
        }
        if ((lastT + xbound)->m_flags & 0x2000) {
            goto label_4cb2a;
        }
        if ((tgtT + 1)->m_flags & 0x2000) {
            goto label_4cb2a;
        }
        if (!((tgtT - xbound)->m_flags & 0x2000)) {
            goto label_4cb4b;
        }
        goto label_4cb2a;
    } else if (dx > 0 && dy < 0) {
        if ((lastT + 1)->m_flags & 0x2000) {
            goto label_4cb2a;
        }
        if ((lastT - xbound)->m_flags & 0x2000) {
            goto label_4cb2a;
        }
        if ((tgtT - 1)->m_flags & 0x2000) {
            goto label_4cb2a;
        }
        if (!((tgtT + xbound)->m_flags & 0x2000)) {
            goto label_4cb4b;
        }
        goto label_4cb2a;
    } else if (dx < 0 && dy < 0) {
        if ((lastT - 1)->m_flags & 0x2000) {
            goto label_4cb2a;
        }
        if ((lastT - xbound)->m_flags & 0x2000) {
            goto label_4cb2a;
        }
        if ((tgtT + 1)->m_flags & 0x2000) {
            goto label_4cb2a;
        }
        if (!((tgtT + xbound)->m_flags & 0x2000)) {
            goto label_4cb4b;
        }
        goto label_4cb2a;
    }
    goto label_4cb4b;
}

label_4cb2a:
    PlaySound(0x3e8, rec);
    goto label_dropRet0;

label_4cb4b:
    m_reserved210 = 0;
    m_tileMgr->ApplySwitch(this, m_lastTilePx.m_x, m_lastTilePx.m_y);
    m_coordRetryCount = 0;
    PlaySound(0x3e8, rec);
    {
        m_commitPx.m_x = m_lastTilePx.m_x;
        m_commitPx.m_y = m_lastTilePx.m_y;
        i32 lastTileX = m_lastTilePx.m_x >> TILE_SHIFT_PX;
        i32 lastTileY = m_lastTilePx.m_y >> TILE_SHIFT_PX;
        CGruntzMapMgr* bdl = g_gameReg->m_tileGrid;

        bdl->m_rows[lastTileY][lastTileX].m_flagBytes[3] &= 0xdf;
        bdl->m_rows[lastTileY][lastTileX].m_occupantId = -1;

        tgtTileX = tgtPxX >> TILE_SHIFT_PX;
        tgtTileY = tgtPxY >> TILE_SHIFT_PX;
        CGruntzMapMgr* bd2 = g_gameReg->m_tileGrid;
        bd2->m_rows[tgtTileY][tgtTileX].m_flags |= 0x20000000;
        bd2->m_rows[tgtTileY][tgtTileX].m_occupantId = (m_tileOwnerHi << 8) | m_tileOwnerLo;

        m_lastTilePx.m_x = rec.row;
        m_lastTilePx.m_y = rec.column;
        ComputeFacing(1.0);
    }
    m_arrivalPending = 1;
    if (reason12) {
        if (flagHead & 0x100) {
            if (m_coordToggle != 0) {
                goto label_ret1;
            }
        } else {
            if (m_coordToggle == 0) {
                goto label_ret1;
            }
        }
        RunMoveConfig(tgtTileX, tgtTileY);
        return 1;
    }
    if (reason16) {
        if (!(flagHead & 0xd02)) {
            goto label_ret1;
        }
        if (m_wingzEnabled != 0) {
            goto label_ret1;
        }
        LoadWingzGruntSprites(1);
        return 1;
    }
    if (reason0e) {
        m_value = m_wwdObject->m_animCursor.m_animation;
        m_wwdObject->m_animCursor.Setup(m_poseWalk);
        return 1;
    }
    goto label_ret1;

label_dropRet0:
    SetEntrancePos(1, 1);
    return 0;

label_ret1:
    return 1;
}

RVA(0x0004d060, 0x98)
void CGrunt::SetEntrancePos(i32 a, i32 b) {
    m_reserved210 = 0;
    m_entrancePx.m_x = m_lastTilePx.m_x;
    m_entrancePx.m_y = m_lastTilePx.m_y;
    if (a) {
        m_arrivalPhase = 0;
        m_arrivalActive = 0;
    }
    if (b && m_arrivalState != AI_BATTLEZ_PATH && CoordCount() != 0) {

        POSITION pos = m_coordList.GetHeadPosition();
        while (pos != NULL) {
            void* buf = m_coordList.GetNext(pos);
            if (buf) {
                CoordPoolNode* slot = g_coordPool.NodeOf(buf);
                slot->m_next = g_coordPool.m_freeHead;
                g_coordPool.m_freeHead = slot;
            }
        }
        m_coordList.RemoveAll();
    }
}

// @early-stop
RVA(0x0004d130, 0xb5)
i32 CGrunt::CreateHealthSprite() {
    if (m_healthSprite || m_health <= 0) {
        return 0;
    }

    m_healthSprite = g_gameReg->m_world->m_childGroup->CreateSprite(
        0,
        m_object->m_screenX,
        m_object->m_screenY - 0x19,
        SORTKEY_GRUNT_HUD,
        s_GruntHealthSprite,
        0x40003
    );
    m_healthSprite->m_animWorker->m_notify(m_healthSprite);

    AnimWorkerObj* inner = m_healthSprite->m_animWorker;
    CGruntHealthSprite* reg = static_cast<CGruntHealthSprite*>(inner->m_logic);
    if (!reg->SetHealthGlyph(m_tileOwnerHi, m_tileOwnerLo, m_health)) {
        reg->m_wwdObject->m_flags |= 0x10000;
        m_healthSprite = NULL;
        return 0;
    }
    return 1;
}

// @early-stop
// 2-arg sibling of the SetHealthGlyph family: retail defers the m_tileOwnerHi
// load past the reg=inner->m_logic step and reuses eax, cl hoists it into edx.
// 600 AST + TU-state variants moved none of it.
RVA(0x0004d220, 0x9c)
i32 CGrunt::CreateToySprite() {
    if (m_toySprite) {
        return 0;
    }

    m_toySprite = g_gameReg->m_world->m_childGroup->CreateSprite(
        0,
        m_object->m_screenX,
        m_object->m_screenY - 0x19,
        SORTKEY_GRUNT_HUD,
        s_GruntToySprite,
        0x40003
    );
    m_toySprite->m_animWorker->m_notify(m_toySprite);

    AnimWorkerObj* inner = m_toySprite->m_animWorker;
    CGruntToySprite* reg = static_cast<CGruntToySprite*>(inner->m_logic);
    if (!reg->SetCell(m_tileOwnerHi, m_tileOwnerLo)) {
        reg->m_wwdObject->m_flags |= 0x10000;
        m_toySprite = NULL;
        return 0;
    }
    return 1;
}

// @early-stop
RVA(0x0004d2f0, 0xb4)
i32 CGrunt::CreateStaminaSprite() {
    if (m_staminaSprite || m_stamina == STAMINA_FULL) {
        return 0;
    }

    m_staminaSprite = g_gameReg->m_world->m_childGroup->CreateSprite(
        0,
        m_object->m_screenX,
        m_object->m_screenY - 0x20,
        SORTKEY_GRUNT_HUD,
        s_GruntStaminaSprite,
        0x40003
    );
    m_staminaSprite->m_animWorker->m_notify(m_staminaSprite);

    AnimWorkerObj* inner = m_staminaSprite->m_animWorker;
    CGruntHealthSprite* reg = static_cast<CGruntHealthSprite*>(inner->m_logic);
    if (!reg->SetHealthGlyph(m_tileOwnerHi, m_tileOwnerLo, m_stamina)) {
        reg->m_wwdObject->m_flags |= 0x10000;
        m_staminaSprite = NULL;
        return 0;
    }
    return 1;
}

// @early-stop
RVA(0x0004d3e0, 0xf5)
i32 CGrunt::CreateToyTimeSprite() {
    if (m_toyTimeSprite || m_toyTime == 0) {
        return 0;
    }

    if (m_staminaSprite) {
        m_staminaSprite->m_flags |= 0x10000;
        m_staminaSprite = NULL;
    }
    if (m_wingzTimeSprite) {
        m_wingzTimeSprite->m_flags |= 0x10000;
        m_wingzTimeSprite = NULL;
    }

    m_toyTimeSprite = g_gameReg->m_world->m_childGroup->CreateSprite(
        0,
        m_object->m_screenX,
        m_object->m_screenY - 0x20,
        SORTKEY_GRUNT_HUD,
        s_GruntToyTimeSprite,
        0x40003
    );
    m_toyTimeSprite->m_animWorker->m_notify(m_toyTimeSprite);

    AnimWorkerObj* inner = m_toyTimeSprite->m_animWorker;
    CGruntHealthSprite* reg = static_cast<CGruntHealthSprite*>(inner->m_logic);
    if (!reg->SetHealthGlyph(m_tileOwnerHi, m_tileOwnerLo, m_toyTime)) {
        reg->m_wwdObject->m_flags |= 0x10000;
        m_toyTimeSprite = NULL;
        return 0;
    }
    return 1;
}

// @early-stop
RVA(0x0004d520, 0xe3)
i32 CGrunt::CreateWingzTimeSprite() {
    if (m_wingzTimeSprite || m_wingzEnabled == 0 || m_wingzTime == 0) {
        return 0;
    }

    if (m_toyTimeSprite) {
        m_toyTimeSprite->m_flags |= 0x10000;
        m_toyTimeSprite = NULL;
    }

    m_wingzTimeSprite = g_gameReg->m_world->m_childGroup->CreateSprite(
        0,
        m_object->m_screenX,
        m_object->m_screenY - 0x26,
        SORTKEY_GRUNT_HUD,
        s_GruntWingzTimeSprite,
        0x40003
    );
    m_wingzTimeSprite->m_animWorker->m_notify(m_wingzTimeSprite);

    AnimWorkerObj* inner = m_wingzTimeSprite->m_animWorker;
    CGruntHealthSprite* reg = static_cast<CGruntHealthSprite*>(inner->m_logic);
    if (!reg->SetHealthGlyph(m_tileOwnerHi, m_tileOwnerLo, m_wingzTime)) {
        reg->m_wwdObject->m_flags |= 0x10000;
        m_wingzTimeSprite = NULL;
        return 0;
    }
    return 1;
}

// @early-stop
// four bytes: ecx and edx are swapped for the a / m_tileOwnerLo argument pair.
RVA(0x0004d650, 0xa1)
i32 CGrunt::CreatePowerupSprite(i32 a) {
    if (m_powerupSprite) {
        return 0;
    }

    m_powerupSprite = g_gameReg->m_world->m_childGroup->CreateSprite(
        0,
        m_object->m_screenX,
        m_object->m_screenY,
        0x15,
        s_GruntPowerupSprite,
        0x40003
    );
    m_powerupSprite->m_animWorker->m_notify(m_powerupSprite);

    AnimWorkerObj* inner = m_powerupSprite->m_animWorker;
    CGruntPowerupSprite* reg = static_cast<CGruntPowerupSprite*>(inner->m_logic);
    if (!reg->SetCell(m_tileOwnerHi, m_tileOwnerLo, a)) {
        reg->m_wwdObject->m_flags |= 0x10000;
        m_powerupSprite = NULL;
        return 0;
    }
    return 1;
}

// @early-stop
// same wall as CreateToySprite: retail defers the m_tileOwnerHi load past the
// reg=inner->m_logic step and reuses eax, cl hoists it into edx.
RVA(0x0004d730, 0x96)
i32 CGrunt::CreateSelectedSprite() {
    if (m_selectedSprite) {
        return 0;
    }

    m_selectedSprite = g_gameReg->m_world->m_childGroup->CreateSprite(
        0,
        m_object->m_screenX,
        m_object->m_screenY,
        0x14,
        s_GruntSelectedSprite,
        0x40003
    );
    m_selectedSprite->m_animWorker->m_notify(m_selectedSprite);

    AnimWorkerObj* inner = m_selectedSprite->m_animWorker;
    CGruntSelectedSprite* reg = static_cast<CGruntSelectedSprite*>(inner->m_logic);
    if (!reg->SetCell(m_tileOwnerHi, m_tileOwnerLo)) {
        reg->m_wwdObject->m_flags |= 0x10000;
        m_selectedSprite = NULL;
        return 0;
    }
    return 1;
}

// @early-stop
RVA(0x0004d800, 0x440)
i32 CGrunt::Place(
    class CTriggerMgr* board,
    i32 col,
    i32 row,
    PickupType moveIcon,
    PickupType typeKind,
    i32 vehicleKind,
    EnemyAiType kind,
    i32 a8,
    i32 a9,
    i32 a10,
    RECT* span,
    GruntEntranceMode entranceMode
) {
    if (kind != AI_NONE) {
        if (kind != AI_BATTLEZ_PATH) {
            m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
        } else {
            m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
        }
    } else {
        m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
        if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
            m_arrivalFlags = ARRIVAL_FLAGS_PLAYER_SINGLE;
        }
    }
    m_arrivalTargetPx.m_x = -1;
    m_arrivalTargetPx.m_y = -1;
    m_defenderPx.m_x = -1;
    m_defenderPx.m_y = -1;
    m_powerupDuration = 0;
    m_blockedVoicePending = 1;
    m_struckCount = 0;
    m_toyTileIndex = 0;
    m_entrancePickup = PICKUP_INVALID;
    m_coordRetryCount = 0;
    m_moveKind = 0;
    m_moveVariantOverride = 0;
    m_moveVariant = 0;
    m_helpCueId = 0;
    m_arrivalState = kind;
    m_brickPickupType = PICKUP_BROWNBRICK;
    m_tileOwnerHi = col;
    m_defenderQueuePosition = a9;
    m_tileOwnerLo = row;
    m_arrivalCell.m_x = -1;
    m_arrivalCell.m_y = -1;
    m_defenderPickupType = static_cast<PickupType>(a10);
    m_defenderRadius = a8 + 1;
    m_arrivalRerollLo = 0;
    m_arrivalRerollWindowLo = 0;
    m_arrivalRerollHi = 0;
    m_arrivalRerollWindowHi = 0;
    m_holdAnchorLo = 0;
    m_holdWindowLo = 0;
    m_holdAnchorHi = 0;
    m_holdWindowHi = 0;
    m_moveIcon = moveIcon;
    m_tileMgr = board;
    m_daFlag = 1;
    m_arrivalPhase = 0;
    m_toolConfigured = 1;
    m_tileClaimed = 0;
    m_neighborScanEnabled = 1;
    m_tileMoveCommitted = 0;
    m_entranceArmed = 0;
    m_entranceDropActive = 0;
    m_deathType = DEATH_NONE;
    m_pendingTrigger = 0;
    m_cellRemovalNotified = 0;
    m_killerSlot = -1;
    m_passableMask = 0;
    m_savedMoveIcon = -1;
    m_lowStaminaCued = 0;
    m_targetTeam = -1;
    LoadVehicleGruntSprites(static_cast<PickupType>(vehicleKind));
    LoadGruntTypeTable(typeKind, 1, 0, 0);
    if (span != NULL) {
        m_object->m_extent.left = (m_lastTilePx.m_x >> TILE_SHIFT_PX) - span->left;
        m_object->m_extent.right = span->right + (m_lastTilePx.m_x >> TILE_SHIFT_PX);
        m_object->m_extent.top = (m_lastTilePx.m_y >> TILE_SHIFT_PX) - span->top;
        m_object->m_extent.bottom = span->bottom + (m_lastTilePx.m_y >> TILE_SHIFT_PX);
    }
    RECT reach;
    CopyRect(&reach, &m_object->m_extent);
    if (reach.right - reach.left == 0 && reach.top - reach.bottom == 0) {
        m_hasExtent = 0;
    } else {
        m_hasExtent = 1;
    }
    if (m_moveIcon < PICKUP_NONE || m_moveIcon >= PICKUP_MOVEICON_END) {
        m_moveIcon = PICKUP_NONE;
    }
    CShadeTable* shade = g_gameReg->m_spriteFactory->GetSel(IDX(m_moveIcon), 0);
    if (shade == NULL) {
        shade = g_gameReg->m_spriteFactory->GetSel(1, 0);
    }
    m_object->m_drawFillArg = shade;
    m_object->m_drawActive = 1;
    m_object->m_drawFillCmd = SHADE_PAL_16;
    if (entranceMode != GRUNT_ENTRANCE_NONE) {
        BuildEntranceAnimation(entranceMode);
        return 1;
    }

    CGruntzMapMgr* plane = g_gameReg->m_tileGrid;
    i32 tx = m_lastTilePx.m_x >> TILE_SHIFT_PX;
    i32 ty = m_lastTilePx.m_y >> TILE_SHIFT_PX;
    plane->m_rowInts[ty][tx * 7] |= 0x20000000;
    plane->m_rowInts[ty][tx * 7 + 1] = (m_tileOwnerHi << 8) | m_tileOwnerLo;
    m_entranceActive = 0;
    ReadConfigFromButeMgr();
    LoadCellAnimNames(0, 0);
    LoadAnimNameTable(0, 0);
    ResetEntranceAnimation(1, 0, 0);
    // `kind` is the EnemyAiType. These four are the types that own a post: the
    // guards keep theirs where they spawned, and the Object Guard reads its
    // guarded address out of the WWD X Min / Y Min pair (a9, a10), degenerating
    // to a Post Guard when the level gives it none.
    switch (kind) {
        case AI_POSTGUARD:
            m_defenderPx.m_x = m_lastTilePx.m_x;
            m_defenderPx.m_y = m_lastTilePx.m_y;
            break;
        case AI_OBJECTGUARD:
            if (a9 == 0 && a10 == 0) {
                m_defenderPx.m_x = m_lastTilePx.m_x;
                m_defenderPx.m_y = m_lastTilePx.m_y;
                m_arrivalState = AI_POSTGUARD;
            } else {
                i32 px = (a9 << TILE_SHIFT_PX) + TILE_HALF_PX;
                i32 py = (a10 << TILE_SHIFT_PX) + TILE_HALF_PX;
                m_defenderPx.m_x = px;
                m_defenderPx.m_y = py;
                StepArrivalDrop(px, py - 0x20, 0, -1, 1, 0);
            }
            break;
        case AI_DEFENDER:
        case AI_BOMBER:
            m_defenderPx.m_x = m_lastTilePx.m_x;
            m_defenderPx.m_y = m_lastTilePx.m_y;
            break;
    }
    return 1;
}

static inline void ConstructGrownSlots() {
    CString* slot = g_typeColl.Slots();
    i32 n = g_typeColl.m_grown;
    while (n-- != 0) {
        if (slot) {
            new (slot) CString();
        }
        slot++;
    }
}

// @early-stop
RVA(0x0004dd50, 0x2880)
i32 CGrunt::LoadGruntTypeTable(PickupType kind, i32 fresh, i32 variant, i32 defer) {
    char eq;
    if (kind == PICKUP_INVALID) {
        goto fail;
    }
    if (m_gruntKind == GRUNT_CONVERSION) {
        goto fail;
    }
    if (m_gruntKind == GRUNT_DEATHTOUCH) {
        goto fail;
    }
    if (fresh == 0) {
        if (m_entranceActive != 0) {
            goto fail;
        }
        eq = (strcmp((*g_typeColl.GetNameRecord(m_objAux->m_actKey)), "A") != 0);
        if (eq) {
            eq = (strcmp((*g_typeColl.GetNameRecord(m_objAux->m_actKey)), "D") != 0);
            if (eq) {
                goto fail;
            }
        }
    }
    if (m_entranceReason == kind) {
        if (kind != PICKUP_WINGZ) {
            return 1;
        }
        m_wingzTime = 0x64;
        LoadWingzGruntSprites(m_wingzEnabled);
        return 1;
    }
    if (defer == 0) {
        if (FinishActiveAction() != 0) {
            if (m_gruntKind == GRUNT_CONVERSION) {
                goto fail;
            }
            if (m_gruntKind == GRUNT_DEATHTOUCH) {
                goto fail;
            }
            if (m_entranceReason == kind) {
                if (kind != PICKUP_WINGZ) {
                    return 1;
                }
                m_wingzTime = 0x64;
                LoadWingzGruntSprites(m_wingzEnabled);
                return 1;
            }
        }
    }
    if (m_coordToggle != 0) {
        goto fail;
    }
    if (kind != PICKUP_WINGZ) {
        m_wingzEnabled = 0;
        m_wingzDurationLo = 0;
        m_wingzDurationHi = 0;
        if (m_wingzTimeSprite != NULL) {
            m_wingzTimeSprite->m_flags |= 0x10000;
            m_wingzTimeSprite = NULL;
        }
    }
    fresh = 0;
    defer = 0;
    if (m_entranceReason < PICKUP_EQUIPPABLE_END) {
        m_toolId = m_entranceReason;
    }
    switch (kind) {
        case PICKUP_NONE: {
            m_animSetName = "NORMALGRUNT";
            i32 r = g_buteMgr.GetIntDef(m_animSetName, "ToolAA", 1);
            m_reachRect.left = -r;
            m_reachRect.top = -r;
            m_reachRect.right = r;
            m_reachRect.bottom = r;
            m_reachExclusionRect.left = 0;
            m_reachExclusionRect.top = 0;
            m_reachExclusionRect.right = 0;
            m_reachExclusionRect.bottom = 0;
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
                m_arrivalFlags |= 0x10;
            }
            m_passableMask = 0;
            m_toolConfigured = 1;
            break;
        }
        case PICKUP_BOMB: {
            m_animSetName = "BOMBGRUNT";
            i32 r = g_buteMgr.GetIntDef(m_animSetName, "ToolAA", 1);
            m_reachRect.left = -r;
            m_reachRect.top = -r;
            m_reachRect.right = r;
            m_reachRect.bottom = r;
            m_reachExclusionRect.left = 0;
            m_reachExclusionRect.top = 0;
            m_reachExclusionRect.right = 0;
            m_reachExclusionRect.bottom = 0;
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
                m_arrivalFlags |= 0x10;
            }
            m_passableMask = 0;
            m_toolConfigured = 1;
            break;
        }
        case PICKUP_BOOMERANG: {
            m_animSetName = "BOOMERANGGRUNT";
            i32 r = g_buteMgr.GetIntDef(m_animSetName, "ToolAA", 1);
            m_reachRect.left = -r;
            m_reachRect.top = -r;
            m_reachRect.right = r;
            m_reachRect.bottom = r;
            m_reachExclusionRect.left = 0;
            m_reachExclusionRect.top = 0;
            m_reachExclusionRect.right = 0;
            m_reachExclusionRect.bottom = 0;
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (m_arrivalState == AI_DEFENDER) {
                m_defenderRadius = 1;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
                m_arrivalFlags |= 0x10;
            }
            m_passableMask = 0;
            m_toolConfigured = 1;
            break;
        }
        case PICKUP_BRICK: {
            m_animSetName = "BRICKGRUNT";
            i32 r = g_buteMgr.GetIntDef(m_animSetName, "ToolAA", 1);
            m_reachRect.left = -r;
            m_reachRect.top = -r;
            m_reachRect.right = r;
            m_reachRect.bottom = r;
            m_reachExclusionRect.left = 0;
            m_reachExclusionRect.top = 0;
            m_reachExclusionRect.right = 0;
            m_reachExclusionRect.bottom = 0;
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
                m_arrivalFlags |= 0x10;
            }
            m_passableMask = 0;
            m_toolConfigured = 1;
            break;
        }
        case PICKUP_CLUB: {
            m_animSetName = "CLUBGRUNT";
            i32 r = g_buteMgr.GetIntDef(m_animSetName, "ToolAA", 1);
            m_reachRect.left = -r;
            m_reachRect.top = -r;
            m_reachRect.right = r;
            m_reachRect.bottom = r;
            m_reachExclusionRect.left = 0;
            m_reachExclusionRect.top = 0;
            m_reachExclusionRect.right = 0;
            m_reachExclusionRect.bottom = 0;
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
                m_arrivalFlags |= 0x10;
            }
            m_passableMask = 0;
            m_toolConfigured = 1;
            break;
        }
        case PICKUP_GAUNTLETZ: {
            m_animSetName = "GAUNTLETZGRUNT";
            i32 r = g_buteMgr.GetIntDef(m_animSetName, "ToolAA", 1);
            m_reachRect.left = -r;
            m_reachRect.top = -r;
            m_reachRect.right = r;
            m_reachRect.bottom = r;
            m_reachExclusionRect.left = 0;
            m_reachExclusionRect.top = 0;
            m_reachExclusionRect.right = 0;
            m_reachExclusionRect.bottom = 0;
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
                m_arrivalFlags |= 0x10;
            }
            m_passableMask = 0;
            m_toolConfigured = 1;
            break;
        }
        case PICKUP_GLOVEZ: {
            m_animSetName = "GLOVEZGRUNT";
            i32 r = g_buteMgr.GetIntDef(m_animSetName, "ToolAA", 1);
            m_reachRect.left = -r;
            m_reachRect.top = -r;
            m_reachRect.right = r;
            m_reachRect.bottom = r;
            m_reachExclusionRect.left = 0;
            m_reachExclusionRect.top = 0;
            m_reachExclusionRect.right = 0;
            m_reachExclusionRect.bottom = 0;
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
                m_arrivalFlags |= 0x10;
            }
            m_passableMask = 0;
            m_toolConfigured = 1;
            break;
        }
        case PICKUP_GOOBER: {
            m_animSetName = "GOOBERGRUNT";
            i32 r = g_buteMgr.GetIntDef(m_animSetName, "ToolAA", 1);
            m_reachRect.left = -r;
            m_reachRect.top = -r;
            m_reachRect.right = r;
            m_reachRect.bottom = r;
            m_reachExclusionRect.left = 0;
            m_reachExclusionRect.top = 0;
            m_reachExclusionRect.right = 0;
            m_reachExclusionRect.bottom = 0;
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
                m_arrivalFlags |= 0x10;
            }
            m_passableMask = 0;
            m_toolConfigured = 1;
            if (m_arrivalState == AI_BATTLEZ_PATH) {
                if (m_battleState != BZTASK_ADVANCE) {
                    if (CoordCount() != 0) {
                        CoordNode* p = CoordHead();
                        while (p != NULL) {
                            CoordNode* c = p;
                            p = p->m_next;
                            if (c->m_coord != NULL) {
                                g_coordPool.Push(c->m_coord);
                            }
                        }
                        m_coordList.RemoveAll();
                    }
                    for (;;) {
                        void* h;
                        if (m_payloads.GetCount() != 0) {
                            h = m_payloads.GetHead();
                        } else {
                            h = NULL;
                        }
                        if (h == NULL) {
                            break;
                        }
                        if (m_payloads.GetCount() != 0) {
                            delete[] static_cast<i32*>(m_payloads.RemoveHead());
                        }
                    }
                    i32* payload = new i32[0xb];
                    if (payload != NULL) {
                        memset(payload, 0, 0x2c);
                    }
                    payload[0] = 9;
                    m_payloads.AddHead(payload);
                }
            }
            break;
        }
        case PICKUP_GRAVITYBOOTZ: {
            m_animSetName = "GRAVITYBOOTZGRUNT";
            i32 r = g_buteMgr.GetIntDef(m_animSetName, "ToolAA", 1);
            m_reachRect.left = -r;
            m_reachRect.top = -r;
            m_reachRect.right = r;
            m_reachRect.bottom = r;
            m_reachExclusionRect.left = 0;
            m_reachExclusionRect.top = 0;
            m_reachExclusionRect.right = 0;
            m_reachExclusionRect.bottom = 0;
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
                m_arrivalFlags |= 0x10;
            }
            m_passableMask = 0x400;
            m_toolConfigured = 1;
            break;
        }
        case PICKUP_GUNHAT: {
            m_animSetName = "GUNHATGRUNT";
            i32 r = g_buteMgr.GetIntDef(m_animSetName, "ToolAA", 1);
            m_reachRect.left = -r;
            m_reachRect.top = -r;
            m_reachRect.right = r;
            m_reachRect.bottom = r;
            m_reachExclusionRect.left = 0;
            m_reachExclusionRect.top = 0;
            m_reachExclusionRect.right = 0;
            m_reachExclusionRect.bottom = 0;
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
                m_arrivalFlags |= 0x10;
            }
            if (m_arrivalState == AI_DEFENDER) {
                m_defenderRadius = 1;
            }
            m_passableMask = 0;
            m_toolConfigured = 1;
            break;
        }
        case PICKUP_NERFGUN: {
            m_animSetName = "NERFGUNGRUNT";
            i32 r = g_buteMgr.GetIntDef(m_animSetName, "ToolAA", 1);
            m_reachRect.left = -r;
            m_reachRect.top = -r;
            m_reachRect.right = r;
            m_reachRect.bottom = r;
            m_reachExclusionRect.left = 0;
            m_reachExclusionRect.top = 0;
            m_reachExclusionRect.right = 0;
            m_reachExclusionRect.bottom = 0;
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
                m_arrivalFlags |= 0x10;
            }
            if (m_arrivalState == AI_DEFENDER) {
                m_defenderRadius = 1;
            }
            m_passableMask = 0;
            m_toolConfigured = 1;
            break;
        }
        case PICKUP_ROCK: {
            m_animSetName = "ROCKGRUNT";
            i32 r = g_buteMgr.GetIntDef(m_animSetName, "ToolAA", 1);
            m_reachRect.left = -r;
            m_reachRect.top = -r;
            m_reachRect.right = r;
            m_reachRect.bottom = r;
            m_reachExclusionRect.left = 0;
            m_reachExclusionRect.top = 0;
            m_reachExclusionRect.right = 0;
            m_reachExclusionRect.bottom = 0;
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
                m_arrivalFlags |= 0x10;
            }
            if (m_arrivalState == AI_DEFENDER) {
                m_defenderRadius = 1;
            }
            m_passableMask = 0;
            m_toolConfigured = 1;
            break;
        }
        case PICKUP_SHIELD: {
            m_animSetName = "SHIELDGRUNT";
            i32 r = g_buteMgr.GetIntDef(m_animSetName, "ToolAA", 1);
            m_reachRect.left = -r;
            m_reachRect.top = -r;
            m_reachRect.right = r;
            m_reachRect.bottom = r;
            m_reachExclusionRect.left = 0;
            m_reachExclusionRect.top = 0;
            m_reachExclusionRect.right = 0;
            m_reachExclusionRect.bottom = 0;
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
                m_arrivalFlags |= 0x10;
            }
            m_passableMask = 0;
            m_toolConfigured = 1;
            break;
        }
        case PICKUP_SHOVEL: {
            m_animSetName = "SHOVELGRUNT";
            i32 r = g_buteMgr.GetIntDef(m_animSetName, "ToolAA", 1);
            m_reachRect.left = -r;
            m_reachRect.top = -r;
            m_reachRect.right = r;
            m_reachRect.bottom = r;
            m_reachExclusionRect.left = 0;
            m_reachExclusionRect.top = 0;
            m_reachExclusionRect.right = 0;
            m_reachExclusionRect.bottom = 0;
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
                m_arrivalFlags |= 0x10;
            }
            m_passableMask = 0;
            m_toolConfigured = 1;
            break;
        }
        case PICKUP_SPRING: {
            m_animSetName = "SPRINGGRUNT";
            i32 r = g_buteMgr.GetIntDef(m_animSetName, "ToolAA", 1);
            m_reachRect.left = -r;
            m_reachRect.top = -r;
            m_reachRect.right = r;
            m_reachRect.bottom = r;
            m_reachExclusionRect.left = 0;
            m_reachExclusionRect.top = 0;
            m_reachExclusionRect.right = 0;
            m_reachExclusionRect.bottom = 0;
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
                m_arrivalFlags |= 0x10;
            }
            m_passableMask = 0x1000;
            m_toolConfigured = 1;
            break;
        }
        case PICKUP_SPY: {
            m_animSetName = "SPYGRUNT";
            i32 r = g_buteMgr.GetIntDef(m_animSetName, "ToolAA", 1);
            m_reachRect.left = -r;
            m_reachRect.top = -r;
            m_reachRect.right = r;
            m_reachRect.bottom = r;
            m_reachExclusionRect.left = 0;
            m_reachExclusionRect.top = 0;
            m_reachExclusionRect.right = 0;
            m_reachExclusionRect.bottom = 0;
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
                m_arrivalFlags |= 0x10;
            }
            m_passableMask = 0;
            m_toolConfigured = 1;
            break;
        }
        case PICKUP_SWORD: {
            m_animSetName = "SWORDGRUNT";
            i32 r = g_buteMgr.GetIntDef(m_animSetName, "ToolAA", 1);
            m_reachRect.left = -r;
            m_reachRect.top = -r;
            m_reachRect.right = r;
            m_reachRect.bottom = r;
            m_reachExclusionRect.left = 0;
            m_reachExclusionRect.top = 0;
            m_reachExclusionRect.right = 0;
            m_reachExclusionRect.bottom = 0;
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
                m_arrivalFlags |= 0x10;
            }
            m_passableMask = 0;
            m_toolConfigured = 1;
            break;
        }
        case PICKUP_TIMEBOMB: {
            m_animSetName = "TIMEBOMBGRUNT";
            i32 r = g_buteMgr.GetIntDef(m_animSetName, "ToolAA", 1);
            m_reachRect.left = -r;
            m_reachRect.top = -r;
            m_reachRect.right = r;
            m_reachRect.bottom = r;
            m_reachExclusionRect.left = 0;
            m_reachExclusionRect.top = 0;
            m_reachExclusionRect.right = 0;
            m_reachExclusionRect.bottom = 0;
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
                m_arrivalFlags |= 0x10;
            }
            m_passableMask = 0;
            m_toolConfigured = 1;
            break;
        }
        case PICKUP_TOOB: {
            m_animSetName = "TOOBGRUNT";
            i32 r = g_buteMgr.GetIntDef(m_animSetName, "ToolAA", 1);
            m_reachRect.left = -r;
            m_reachRect.top = -r;
            m_coordToggle = 0;
            m_reachRect.right = r;
            m_reachRect.bottom = r;
            m_reachExclusionRect.left = 0;
            m_reachExclusionRect.top = 0;
            m_reachExclusionRect.right = 0;
            m_reachExclusionRect.bottom = 0;
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
                m_arrivalFlags |= 0x10;
            }
            m_passableMask = 0x100;
            m_toolConfigured = 1;
            break;
        }
        case PICKUP_WAND: {
            m_animSetName = "WANDGRUNT";
            i32 r = g_buteMgr.GetIntDef(m_animSetName, "ToolAA", 1);
            m_reachRect.left = -r;
            m_reachRect.top = -r;
            m_reachRect.right = r;
            m_reachRect.bottom = r;
            m_reachExclusionRect.left = 0;
            m_reachExclusionRect.top = 0;
            m_reachExclusionRect.right = 0;
            m_reachExclusionRect.bottom = 0;
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
                m_arrivalFlags |= 0x10;
            }
            m_passableMask = 0;
            m_toolConfigured = 1;
            break;
        }
        case PICKUP_WARPSTONE: {
            m_animSetName = "WARPSTONEGRUNT";
            i32 r = g_buteMgr.GetIntDef(m_animSetName, "ToolAA", 1);
            m_reachRect.left = -r;
            m_reachRect.top = -r;
            m_reachRect.right = r;
            m_reachRect.bottom = r;
            m_reachExclusionRect.left = 0;
            m_reachExclusionRect.top = 0;
            m_reachExclusionRect.right = 0;
            m_reachExclusionRect.bottom = 0;
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
                m_arrivalFlags |= 0x10;
            }
            m_passableMask = 0;
            m_toolConfigured = 0;
            break;
        }
        case PICKUP_WELDER: {
            m_animSetName = "WELDERGRUNT";
            i32 r = g_buteMgr.GetIntDef(m_animSetName, "ToolAA", 1);
            m_reachRect.left = -r;
            m_reachRect.top = -r;
            m_reachRect.right = r;
            m_reachRect.bottom = r;
            m_reachExclusionRect.left = 0;
            m_reachExclusionRect.top = 0;
            m_reachExclusionRect.right = 0;
            m_reachExclusionRect.bottom = 0;
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
                m_arrivalFlags |= 0x10;
            }
            if (m_arrivalState == AI_DEFENDER) {
                m_defenderRadius = 1;
            }
            m_passableMask = 0;
            m_toolConfigured = 1;
            break;
        }
        case PICKUP_WINGZ: {
            m_animSetName = "WINGZGRUNT";
            i32 r = g_buteMgr.GetIntDef(m_animSetName, "ToolAA", 1);
            m_reachRect.left = -r;
            m_reachRect.top = -r;
            m_reachRect.right = r;
            m_reachRect.bottom = r;
            m_reachExclusionRect.left = 0;
            m_reachExclusionRect.top = 0;
            m_reachExclusionRect.right = 0;
            m_reachExclusionRect.bottom = 0;
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
                m_arrivalFlags |= 0x10;
            }
            if (m_arrivalState == AI_DEFENDER) {
                m_defenderRadius = 1;
            }
            m_passableMask = 0xd02;
            m_wingzEnabled = 0;
            m_wingzTime = 0x64;
            m_toolConfigured = 1;
            break;
        }
        case PICKUP_BABYWALKER: {
            if (m_arrivalState == AI_DUMBCHASER) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
                m_arrivalFlags |= 0x10;
            }
            m_passableMask = 1;
            m_animSetName = "BABYWALKERGRUNT";
            const char* rec = *g_typeColl.ScratchResolve(m_objAux->m_actKey);
            ConstructGrownSlots();
            eq = (strcmp(rec, "D") == 0);
            if (eq) {
                ConsiderArrival(0);
                fresh = 1;
                defer = 1;
            }
            break;
        }
        case PICKUP_BEACHBALL: {
            if (m_arrivalState == AI_DUMBCHASER) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
                m_arrivalFlags |= 0x10;
            }
            m_passableMask = 1;
            m_animSetName = "BEACHBALLGRUNT";
            const char* rec = *g_typeColl.ScratchResolve(m_objAux->m_actKey);
            ConstructGrownSlots();
            eq = (strcmp(rec, "D") == 0);
            if (eq) {
                ConsiderArrival(0);
                fresh = 1;
            }
            break;
        }
        case PICKUP_BIGWHEEL: {
            if (m_arrivalState == AI_DUMBCHASER) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
                m_arrivalFlags |= 0x10;
            }
            m_passableMask = 1;
            m_animSetName = "BIGWHEELGRUNT";
            const char* rec = *g_typeColl.ScratchResolve(m_objAux->m_actKey);
            ConstructGrownSlots();
            eq = (strcmp(rec, "D") == 0);
            if (eq) {
                ConsiderArrival(0);
                fresh = 1;
                defer = 1;
            }
            break;
        }
        case PICKUP_GOKART: {
            if (m_arrivalState == AI_DUMBCHASER) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
                m_arrivalFlags |= 0x10;
            }
            m_passableMask = 1;
            m_animSetName = "GOKARTGRUNT";
            const char* rec = *g_typeColl.ScratchResolve(m_objAux->m_actKey);
            ConstructGrownSlots();
            eq = (strcmp(rec, "D") == 0);
            if (eq) {
                ConsiderArrival(0);
                fresh = 1;
            }
            break;
        }
        case PICKUP_JACKINTHEBOX: {
            if (m_arrivalState == AI_DUMBCHASER) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
                m_arrivalFlags |= 0x10;
            }
            m_passableMask = 1;
            m_animSetName = "JACKINTHEBOXGRUNT";
            const char* rec = *g_typeColl.ScratchResolve(m_objAux->m_actKey);
            ConstructGrownSlots();
            eq = (strcmp(rec, "D") == 0);
            if (eq) {
                ConsiderArrival(0);
                fresh = 1;
            }
            break;
        }
        case PICKUP_JUMPROPE: {
            if (m_arrivalState == AI_DUMBCHASER) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
                m_arrivalFlags |= 0x10;
            }
            m_passableMask = 1;
            m_animSetName = "JUMPROPEGRUNT";
            const char* rec = *g_typeColl.ScratchResolve(m_objAux->m_actKey);
            ConstructGrownSlots();
            eq = (strcmp(rec, "D") == 0);
            if (eq) {
                ConsiderArrival(0);
                fresh = 1;
                defer = 1;
            }
            break;
        }
        case PICKUP_POGOSTICK: {
            if (m_arrivalState == AI_DUMBCHASER) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
                m_arrivalFlags |= 0x10;
            }
            m_passableMask = 1;
            m_animSetName = "POGOSTICKGRUNT";
            const char* rec = *g_typeColl.ScratchResolve(m_objAux->m_actKey);
            ConstructGrownSlots();
            eq = (strcmp(rec, "D") == 0);
            if (eq) {
                ConsiderArrival(0);
                fresh = 1;
                defer = 1;
            }
            break;
        }
        case PICKUP_SCROLL: {
            if (m_arrivalState == AI_DUMBCHASER) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
                m_arrivalFlags |= 0x10;
            }
            m_moveVariant = variant;
            m_passableMask = 1;
            m_animSetName = "SCROLLGRUNT";
            const char* rec = *g_typeColl.ScratchResolve(m_objAux->m_actKey);
            ConstructGrownSlots();
            eq = (strcmp(rec, "D") == 0);
            if (eq) {
                ConsiderArrival(0);
                fresh = 1;
            }
            break;
        }
        case PICKUP_SQUEAKTOY: {
            if (m_arrivalState == AI_DUMBCHASER) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
                m_arrivalFlags |= 0x10;
            }
            m_passableMask = 1;
            m_animSetName = "SQUEAKTOYGRUNT";
            const char* rec = *g_typeColl.ScratchResolve(m_objAux->m_actKey);
            ConstructGrownSlots();
            eq = (strcmp(rec, "D") == 0);
            if (eq) {
                ConsiderArrival(0);
                fresh = 1;
            }
            break;
        }
        case PICKUP_YOYO: {
            if (m_arrivalState == AI_DUMBCHASER) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
                m_arrivalFlags |= 0x10;
            }
            m_passableMask = 1;
            m_animSetName = "YOYOGRUNT";
            const char* rec = *g_typeColl.ScratchResolve(m_objAux->m_actKey);
            ConstructGrownSlots();
            eq = (strcmp(rec, "D") == 0);
            if (eq) {
                ConsiderArrival(0);
                fresh = 1;
            }
            break;
        }
        case PICKUP_HEALTH1: {
            i32 h = g_buteMgr.GetIntDef("Powerupz", "Health1", 0x19) + m_health;
            if (h >= HEALTH_FULL) {
                h = HEALTH_FULL;
            }
            m_health = h;
            return 1;
        }
        case PICKUP_HEALTH2: {
            i32 h = g_buteMgr.GetIntDef("Powerupz", "Health2", 0x19) + m_health;
            if (h >= HEALTH_FULL) {
                h = HEALTH_FULL;
            }
            m_health = h;
            return 1;
        }
        case PICKUP_HEALTH3: {
            i32 h = g_buteMgr.GetIntDef("Powerupz", "Health3", 0x19) + m_health;
            if (h >= HEALTH_FULL) {
                h = HEALTH_FULL;
            }
            m_health = h;
            return 1;
        }
        case PICKUP_CONVERSION: {
            m_toolId = m_entranceReason;
            m_reachRect.left = -1;
            m_reachRect.top = -1;
            m_reachRect.right = 1;
            m_reachRect.bottom = 1;
            m_reachExclusionRect.left = 0;
            m_reachExclusionRect.top = 0;
            m_reachExclusionRect.right = 0;
            m_reachExclusionRect.bottom = 0;
            fresh = 0;
            m_animSetName = "HAREKRISHNAGRUNT";
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
                m_arrivalFlags |= 0x10;
            }
            m_passableMask = 0;
            m_gruntKind = GRUNT_CONVERSION;
            m_convertTimeLo = g_buteMgr.GetDwordDef("Powerupz", "ConversionTime", 0x1f4);
            m_convertTimeHi = 0;
            m_convertClockLo = g_frameTime;
            m_convertClockHi = 0;
            StopStruckVoiceSound();
            EnsureStruckVoice("GAME_CONVERSIONLOOP");
            break;
        }
        case PICKUP_DEATHTOUCH: {
            m_toolId = m_entranceReason;
            m_reachRect.left = -1;
            m_reachRect.top = -1;
            m_reachRect.right = 1;
            m_reachRect.bottom = 1;
            m_reachExclusionRect.left = 0;
            m_reachExclusionRect.top = 0;
            m_reachExclusionRect.right = 0;
            m_reachExclusionRect.bottom = 0;
            fresh = 0;
            m_animSetName = "REAPERGRUNT";
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
                m_arrivalFlags |= 0x10;
            }
            m_passableMask = 0;
            m_gruntKind = GRUNT_DEATHTOUCH;
            if (m_powerupDuration == 0) {
                m_powerupDuration = g_buteMgr.GetDwordDef("Powerupz", "DeathTouchTime", 0x4e20);
            }
            m_convertTimeLo = m_powerupDuration;
            m_convertTimeHi = 0;
            m_convertClockLo = g_frameTime;
            m_convertClockHi = 0;
            m_shimmerWindowLo = 0;
            m_shimmerWindowHi = 0;
            StopStruckVoiceSound();
            EnsureStruckVoice("GAME_DEATHTOUCHLOOP");
            break;
        }
        case PICKUP_GHOST: {
            m_gruntKind = GRUNT_GHOST;
            i32 t = g_buteMgr.GetIntDef("Powerupz", "GruntGhostTransparencyOn", 0xe0);
            m_object->m_drawActive = 1;
            m_object->m_drawFillCmd = SHADE_PAL_ALPHA_16;
            m_object->m_fillFraction = t;
            if (m_powerupDuration == 0) {
                m_powerupDuration = g_buteMgr.GetDwordDef("Powerupz", "GhostTime", 0x4e20);
            }
            m_convertTimeLo = m_powerupDuration;
            m_convertTimeHi = 0;
            m_convertClockLo = g_frameTime;
            m_convertClockHi = 0;
            m_shimmerWindowLo = 0;
            m_shimmerWindowHi = 0;
            StopStruckVoiceSound();
            EnsureStruckVoice("GAME_GHOSTLOOP");
            return 1;
        }
        case PICKUP_INVULNERABILITY: {
            m_gruntKind = GRUNT_INVULNERABLE;
            if (m_powerupDuration == 0) {
                m_powerupDuration =
                    g_buteMgr.GetDwordDef("Powerupz", "InvulnerabilityTime", 0x4e20);
            }
            m_convertTimeLo = m_powerupDuration;
            m_convertTimeHi = 0;
            m_convertClockLo = g_frameTime;
            m_convertClockHi = 0;
            m_shimmerWindowLo = 0;
            m_shimmerWindowHi = 0;
            StopStruckVoiceSound();
            EnsureStruckVoice("GAME_INVULNERABILITYLOOP");
            return 1;
        }
        case PICKUP_REACTIVEARMOR: {
            m_gruntKind = GRUNT_REACTIVEARMOR;
            CreatePowerupSprite(3);
            if (m_powerupDuration == 0) {
                m_powerupDuration = g_buteMgr.GetDwordDef("Powerupz", "ReactiveArmorTime", 0x4e20);
            }
            m_convertTimeLo = m_powerupDuration;
            m_convertTimeHi = 0;
            m_convertClockLo = g_frameTime;
            m_convertClockHi = 0;
            m_shimmerWindowLo = 0;
            m_shimmerWindowHi = 0;
            StopStruckVoiceSound();
            EnsureStruckVoice("GAME_REACTIVEARMORLOOP");
            return 1;
        }
        case PICKUP_ROIDZ: {
            m_gruntKind = GRUNT_ROIDZ;
            CreatePowerupSprite(1);
            if (m_powerupDuration == 0) {
                m_powerupDuration = g_buteMgr.GetDwordDef("Powerupz", "RoidzTime", 0x4e20);
            }
            m_convertTimeLo = m_powerupDuration;
            m_convertTimeHi = 0;
            m_convertClockLo = g_frameTime;
            m_convertClockHi = 0;
            m_shimmerWindowLo = 0;
            m_shimmerWindowHi = 0;
            StopStruckVoiceSound();
            EnsureStruckVoice("GAME_ROIDZLOOP");
            return 1;
        }
        case PICKUP_SUPERSPEED: {
            m_gruntKind = GRUNT_SUPERSPEED;
            CreatePowerupSprite(2);
            if (m_powerupDuration == 0) {
                m_powerupDuration = g_buteMgr.GetDwordDef("Powerupz", "SuperSpeedTime", 0x4e20);
            }
            m_convertTimeLo = m_powerupDuration;
            m_convertTimeHi = 0;
            m_convertClockLo = g_frameTime;
            m_convertClockHi = 0;
            m_shimmerWindowLo = 0;
            m_shimmerWindowHi = 0;
            ReadConfigFromButeMgr();
            LoadCellAnimNames(0, 0);
            LoadAnimNameTable(0, 0);
            StopStruckVoiceSound();
            EnsureStruckVoice("GAME_SUPERSPEEDLOOP");
            return 1;
        }
        case PICKUP_MEGAPHONE: {
            CPlay* play = static_cast<CPlay*>(g_gameReg->m_curState);
            CStatusBarMgr* sb = play->m_guts;
            if (sb->m_hlBusy == 0) {
                if (sb->m_position == STATUSBAR_HIDDEN) {
                    sb->RefreshState();
                }
                if (sb->m_activeTab != TAB_RESOURCE) {
                    sb->SetTabState(SBICMD_TAB_RESOURCE, MENUITEM_SELECTED);
                }
                sb->Deactivate();
                play->m_guts->UpdateRezMachineWakeStatusBar();
                return 1;
            }
            m_tileMgr->CycleMoveIcons(m_tileOwnerHi, 1);
            return 1;
        }
        case PICKUP_RANDOMCOLORZ: {
            if (m_tileOwnerHi == g_curPlayer) {
                return 1;
            }
            (static_cast<CPlay*>(g_gameReg->m_curState))->SetMonitorCurse(1);
            return 1;
        }
        case PICKUP_SCREENSHAKE: {
            if (m_tileOwnerHi == g_curPlayer) {
                return 1;
            }
            (static_cast<CPlay*>(g_gameReg->m_curState))->SetDarknessCurse(1);
            return 1;
        }
        case PICKUP_BLACKSCREEN: {
            if (m_tileOwnerHi == g_curPlayer) {
                return 1;
            }
            (static_cast<CPlay*>(g_gameReg->m_curState))->SetTinyViewportCurse(1);
            return 1;
        }
        case PICKUP_MINICAM: {
            if (m_tileOwnerHi == g_curPlayer) {
                return 1;
            }
            (static_cast<CPlay*>(g_gameReg->m_curState))->SetTinyViewportCurse(1);
            return 1;
        }
        case PICKUP_W:
        case PICKUP_A:
        case PICKUP_R:
        case PICKUP_P: {
            g_gameReg->m_scoreHud->m_scoreValue = 1;
            return 1;
        }
        case PICKUP_HELPBOX: {
            (static_cast<CPlay*>(g_gameReg->m_curState))->PostActionCue(m_helpCueId);
            return 1;
        }
        case PICKUP_COIN: {
            g_gameReg->m_scoreHud->m_coinsCollected++;
            return 1;
        }
        case PICKUP_STOPWATCH: {
            CPlay* play = static_cast<CPlay*>(g_gameReg->m_curState);
            if (play->m_frameMarker == NULL) {
                return 1;
            }
            i32 mins = g_buteMgr.GetIntDef("Powerupz", "StopwatchMinutes", 1);
            i32 secs = g_buteMgr.GetIntDef("Powerupz", "StopwatchSeconds", 0);
            if (g_gameReg->m_isEasyMode != 0 && g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
                secs += secs;
                mins += mins;
                if (secs > 0x3b) {
                    mins++;
                    secs -= 0x3c;
                }
            }
            play->m_frameMarker->AddTime(mins, secs);
            return 1;
        }
        default: {
            m_reachRect.left = -1;
            m_reachRect.top = -1;
            m_reachRect.right = 1;
            m_reachRect.bottom = 1;
            m_reachExclusionRect.left = 0;
            m_reachExclusionRect.top = 0;
            m_reachExclusionRect.right = 0;
            m_reachExclusionRect.bottom = 0;
            fresh = 0;
            m_animSetName = "NORMALGRUNT";
            break;
        }
    }

    {
        CPlay* play = static_cast<CPlay*>(g_gameReg->m_curState);
        if (kind == PICKUP_TOOB) {
            play->BuildGruntTypeNameTable(PICKUP_TOOB, 1, 1, 0);
        } else {
            play->BuildAssetNamespacePrefixes(m_animSetName, 1, 1, 0);
        }
    }
    m_entranceReason = kind;
    ReadConfigFromButeMgr();
    LoadCellAnimNames(fresh, defer);
    LoadAnimNameTable(fresh, defer);
    if (fresh == 0) {
        CString* rec;
        {
            i32 key = m_objAux->ActKey();
            g_typeColl.m_grown = 0;
            if (key >= g_typeColl.m_lo && key <= g_typeColl.m_hi) {
                rec = g_typeColl.Elem(key);
            } else if ((static_cast<_zvec*>(&g_typeColl))->GrowTo(key, 0) != NULL) {
                rec = g_typeColl.Elem(key);
            } else {
                char* msg = g_errOutOfMem;
                g_retAddrBreadcrumb = GetRetAddr();
                g_typeColl.m_errSink->Set(&g_typeColl, msg, 0xc);
                rec = g_typeColl.Scratch();
            }
            ConstructGrownSlots();
        }

        eq = (strcmp(*rec, "H") == 0);
        if (eq) {
            CAniElement* el = m_wwdObject->m_animCursor.m_animation;
            CAniRecordView* first;
            if (el->m_records.GetSize() > 0) {
                first = static_cast<CAniRecordView*>(el->m_records[0]);
            } else {
                first = NULL;
            }
            i32 handle = first->m_param;
            GruntDirectionCell cell = m_entranceCell;
            m_wwdObject->ApplyLookupSprite(
                m_cells[cell.row * 3 + cell.column].m_names[1].GetBuffer(0),
                handle
            );
        } else {
            if (m_poweredUp != 0 && m_neighborValid == 0) {
                m_entranceActive = 0;
                m_combatActive = 0;
                m_neighborValid = 0;
                m_poweredUp = 0;
                ResetEntranceAnimation(1, 0, 0);
            }
            CString* rec2;
            {
                i32 key2 = m_objAux->ActKey();
                g_typeColl.m_grown = 0;
                if (key2 >= g_typeColl.m_lo && key2 <= g_typeColl.m_hi) {
                    rec2 = g_typeColl.Elem(key2);
                } else if ((static_cast<_zvec*>(&g_typeColl))->GrowTo(key2, 0) != NULL) {
                    rec2 = g_typeColl.Elem(key2);
                } else {
                    char* msg2 = g_errOutOfMem;
                    g_retAddrBreadcrumb = GetRetAddr();
                    g_typeColl.m_errSink->Set(&g_typeColl, msg2, 0xc);
                    rec2 = g_typeColl.Scratch();
                }
                ConstructGrownSlots();
            }

            eq = (strcmp(*rec2, "D") == 0);
            if (eq) {
                GruntDirectionCell cell2 = m_entranceCell;
                m_wwdObject->ApplyName(
                    m_cells[cell2.row * 3 + cell2.column].m_names[2].GetBuffer(0)
                );
                m_value = m_wwdObject->m_animCursor.m_animation;
                m_wwdObject->m_animCursor.Setup(m_poseWalk);
            } else {
                ResetEntranceAnimation(1, 0, 0);
                if (m_arrivalPending == 0) {
                    m_tileMgr->WireTileSwitchLogic(this, m_lastTilePx.m_x, m_lastTilePx.m_y);
                    i32 col = m_lastTilePx.m_x >> TILE_SHIFT_PX;
                    i32 row = m_lastTilePx.m_y >> TILE_SHIFT_PX;
                    TileCollisionKind tk = g_gameReg->m_tileGrid->m_rows[row][col].m_typeCode;
                    if (tk == TILEKIND_CHECKPOINT) {
                        UpdateArrival(col, row);
                    } else if (tk == TILEKIND_CHECKPOINT_UP) {
                        if (m_object->m_screenX == m_lastTilePx.m_x
                            && m_object->m_screenY == m_lastTilePx.m_y) {
                            m_tileMgr->ApplySwitch(this, m_lastTilePx.m_x, m_lastTilePx.m_y);
                            m_tileMgr
                                ->WireTileSwitchLogic(this, m_lastTilePx.m_x, m_lastTilePx.m_y);
                        }
                    }
                }
            }
        }
    }
    if (m_arrived != 0) {
        if (m_tileOwnerHi == g_curPlayer) {
            m_tileMgr->StopPendingFx();
        }
    }
    if (kind == PICKUP_WARPSTONE) {
        m_tileMgr->ReinitGroup(m_object->m_screenX, m_object->m_screenY);
    }
    return 1;
fail:
    return 0;
}

// @early-stop
RVA(0x0005d210, 0x1554)
void CGrunt::XferName(char*) {
    if (static_cast<i64>(g_frameTime) - m_struckClock64 >= m_struckTimer64) {
        m_struckCount = 0;
    }
    m_dwell += g_frameDelta;

    if (m_entranceDropActive != 0) {
        bool differs = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), s_codeA) != 0);
        if (differs) {
            differs = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), s_codeK) != 0);
            if (differs) {
                goto dropExpire;
            }
        }

        if (static_cast<i64>(g_frameTime) - m_entranceClock64 >= m_entranceSafeTime64) {
        dropExpire: {
            CWwdGameObjectA* obj = m_object;
            m_entranceDropActive = 0;
            obj->m_drawActive = 1;
            obj->m_drawFillCmd = SHADE_PAL_16;
        }
            m_entranceSafeTimeLo = 0;
            m_entranceSafeTimeHi = 0;
        } else if (static_cast<i64>(g_frameTime) - m_flashClock64 >= m_flashWindow64) {
            CWwdGameObjectA* obj = m_object;
            if (obj->m_drawFillCmd == SHADE_PAL_ALPHA_16) {
                obj->m_drawActive = 1;
                obj->m_drawFillCmd = SHADE_PAL_16;
            } else {
                i32 fade = g_buteMgr.GetIntDef(s_Grunt, s_FadeTransparency, 0xc0);
                CWwdGameObjectA* o2 = m_object;
                o2->m_drawActive = 1;
                o2->m_drawFillCmd = SHADE_PAL_ALPHA_16;
                o2->m_fillFraction = fade;
            }
            i32 flash = g_buteMgr.GetIntDef(s_Grunt, s_SafeFlashTime, 0x32);
            if (g_buteMgr.GetIntDef(s_Grunt, s_AccelerateFlash, 0) == 1) {
                i64 el = static_cast<i64>(g_frameTime) - m_entranceClock64;
                u32 elapsed = (el < 0 ? 0 : static_cast<u32>(el));

                double span =
                    static_cast<double>(g_buteMgr.GetDwordDef(s_Grunt, s_EntranceSafeTime, 0x1388));
                double frac = static_cast<double>(elapsed) / span - 1.0;
                flash = static_cast<i32>(frac * frac * DATA_COMPGEN(0x001e9a40, fp_1e9a40, 750.0));
            }
            if (flash < 0x1e) {
                flash = 0x1e;
            }
            m_flashWindowLo = flash;
            m_flashWindowHi = 0;
            m_flashClockLo = static_cast<i32>(g_frameTime);
            m_flashClockHi = 0;
        }
    }

    if (m_deathAnimStarted != 0) {
        return;
    }

    {
        CWwdGameObjectA* obj = m_object;
        if (obj->m_screenX != m_lastTilePx.m_x || obj->m_screenY != m_lastTilePx.m_y) {
            goto afterTile;
        }
    }
    {
        CGruntzMgr* reg = g_gameReg;
        CMapMgr* grid = reg->m_tileGrid;
        i32 tx = m_lastTilePx.m_x >> TILE_SHIFT_PX;
        i32 ty = m_lastTilePx.m_y >> TILE_SHIFT_PX;
        i32 cellObj;
        if (static_cast<u32>(tx) >= static_cast<u32>(grid->m_width)
            || static_cast<u32>(ty) >= static_cast<u32>(grid->m_height)) {
            cellObj = 0;
        } else {

            cellObj = ((grid->m_rowInts[ty]))[tx * 7 + 2];
        }
        if (cellObj != 0) {
            CGameObject* found = 0;
            CGameObject* result = 0;
            if (MapLookupById(reg->m_world->m_childGroup->m_map48, cellObj, found)) {
                result = found;
            }
            if (result == NULL) {
                grid = g_gameReg->m_tileGrid;
                if (static_cast<u32>(tx) < static_cast<u32>(grid->m_width)
                    && static_cast<u32>(ty) < static_cast<u32>(grid->m_height)) {
                    ((grid->m_rowInts[ty]))[tx * 7 + 2] = 0;
                    ((grid->m_rowInts[ty]))[tx * 7] &= ~0x40000;
                }
            } else {

                CInGameIcon* icon = static_cast<CInGameIcon*>(result->m_animWorker->m_logic);
                icon->PlaceAt(m_tileOwnerHi, m_tileOwnerLo);
            }
        }

        i32 onMoveTile = 0;
        i32 onWingzTile = 0;
        {
            PickupType reason = m_entranceReason;
            if (reason == PICKUP_TOOB) {
                onMoveTile = 1;
            } else if (reason == PICKUP_WINGZ) {
                onWingzTile = 1;
            }
        }

        CGruntzMgr* reg2 = g_gameReg;
        i32 flags;
        {
            CMapMgr* bd = reg2->m_tileGrid;
            if (static_cast<u32>(tx) >= static_cast<u32>(bd->m_width)
                || static_cast<u32>(ty) >= static_cast<u32>(bd->m_height)) {
                flags = 1;
            } else {
                flags = ((bd->m_rowInts[ty]))[tx * 7];
            }
        }
        if (flags & 0x100000) {
            reg2->m_options[0]
                .m_battlezConfig.ClaimCellFromRow(m_tileOwnerHi, m_tileOwnerLo, tx, ty);
        } else if (flags & 0x200000) {
            reg2->m_options[1]
                .m_battlezConfig.ClaimCellFromRow(m_tileOwnerHi, m_tileOwnerLo, tx, ty);
        } else if (flags & 0x400000) {
            reg2->m_options[2]
                .m_battlezConfig.ClaimCellFromRow(m_tileOwnerHi, m_tileOwnerLo, tx, ty);
        } else if (flags & 0x800000) {
            reg2->m_options[3]
                .m_battlezConfig.ClaimCellFromRow(m_tileOwnerHi, m_tileOwnerLo, tx, ty);
        }

        if (onWingzTile != 0) {
            if (flags & 0xd02) {
                if (m_wingzEnabled == 0) {
                    LoadWingzGruntSprites(1);
                    return;
                }
            } else if (m_wingzEnabled != 0) {
                LoadWingzGruntSprites(0);
                return;
            }
        } else if (onMoveTile != 0) {
            if (flags & 0x100) {
                if (m_coordToggle == 0) {
                    RunMoveConfig(
                        m_lastTilePx.m_x >> TILE_SHIFT_PX,
                        m_lastTilePx.m_y >> TILE_SHIFT_PX
                    );
                    return;
                }
            } else if (m_coordToggle != 0) {
                RunMoveConfig(m_lastTilePx.m_x >> TILE_SHIFT_PX, m_lastTilePx.m_y >> TILE_SHIFT_PX);
                return;
            }
        }

        if ((flags & 2) == 0 || m_wingzEnabled != 0) {
            i32 mask = m_arrivalFlags & flags;
            if (!(mask & 0x20000000)) {
                if (mask == 0) {
                    goto tileKindDone;
                }
                if (flags & m_passableMask) {
                    goto tileKindDone;
                }
            }
        }

        {

            i32 ptx = m_lastTilePx.m_x >> TILE_SHIFT_PX;
            i32 pty = m_lastTilePx.m_y >> TILE_SHIFT_PX;
            CGameLevel* level = g_gameReg->m_world->m_level;
            i32 cx = ptx;
            if (cx < 0) {
                cx = 0;
            } else if (cx >= level->m_mainPlane->m_gridW) {
                cx = level->m_mainPlane->m_gridW - 1;
            }
            i32 cy = pty;
            if (cy < 0) {
                cy = 0;
            } else if (cy >= level->m_mainPlane->m_gridH) {
                cy = level->m_mainPlane->m_gridH - 1;
            }
            i32 raw = level->m_mainPlane->m_tileGrid[level->m_mainPlane->m_colOffsets[cy] + cx];
            TileCollisionKind kind;
            if (raw == UNINIT_FILL || raw == -1) {
                kind = TILEKIND_PASSABLE;
            } else {
                CTileImageSet* ts =
                    static_cast<CTileImageSet*>(level->m_imageSets.GetAt(raw & 0xffff));
                kind = ts->GetCollisionAt(0, 0);
            }

            i32 gate = 1;
            GruntDeathType hazard;
            switch (kind) {
                case TILEKIND_REVEALED_POWERUP:
                    hazard = DEATH_HOLE;
                    break;
                case TILEKIND_WATER:
                case TILEKIND_WATERBRIDGE_UP:
                case TILEKIND_TOGGLEWATERBRIDGE_UP:
                    hazard = DEATH_SINK;
                    break;
                case TILEKIND_SPIKES:
                    // Unrecovered: a tile COLUMN into a death-cause slot is not a
                    // real conversion, and gate=0 keeps it from CellDispatch - but
                    // dropping the store changes .text, so the assignment is real.
                    hazard = static_cast<GruntDeathType>(cx);
                    gate = 0;
                    break;
                default: {
                    CMapMgr* bd = g_gameReg->m_tileGrid;
                    i32 cellId;
                    if (static_cast<u32>(ptx) >= static_cast<u32>(bd->m_width)
                        || static_cast<u32>(pty) >= static_cast<u32>(bd->m_height)) {
                        cellId = 0;
                    } else {
                        cellId = ((bd->m_rowInts[pty]))[ptx * 7 + 3];
                    }
                    if (cellId == -1) {
                        hazard = g_areaPitDeath;
                    } else {
                        hazard = DEATH_EXPLODE;
                        if ((flags & 0x80) != 0 || (flags & BRICKZ_BLOCKED_MASK) == 0) {
                            gate = 0;
                        }
                    }
                    break;
                }
                case TILEKIND_SPECIAL:
                case TILEKIND_DEATHBRIDGE_UP:
                case TILEKIND_TOGGLEDEATHBRIDGE_UP:
                    hazard = g_areaPitDeath;
                    break;
            }
            if (gate != 0) {
                m_tileMgr->CellDispatch(m_tileOwnerHi, m_tileOwnerLo, hazard, -1);
                return;
            }
        }

    tileKindDone:
        if (flags & 0x400) {
            PickupType reason = m_entranceReason;
            PickupType pose = reason;
            if (reason > PICKUP_EQUIPPABLE_LAST) {
                pose = m_toolId;
            }
            if (pose == PICKUP_GRAVITYBOOTZ) {
                goto afterTile;
            }
            if (reason == PICKUP_SPRING || reason == PICKUP_TOOB) {
                BuildGruntLoseItemAnimation();
            }
            if (m_wingzEnabled != 0) {
                goto afterTile;
            }
            if (m_gruntKind == GRUNT_INVULNERABLE) {
                goto afterTile;
            }
            if (m_entranceReason == PICKUP_BOMB) {
                bool nameDiffers =
                    (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), s_codeM) != 0);
                if (!nameDiffers) {
                    goto afterTile;
                }
            }
            if (static_cast<i64>(g_frameTime) - m_entranceClock64 < m_entranceSafeTime64) {
                goto afterTile;
            }
            {
                CGruntzMgr* reg3 = g_gameReg;
                i32 hp;

                if (reg3->m_isEasyMode != 0 && reg3->m_gameMode == GAMEMODE_SINGLE) {
                    i32 bite = m_health - 5;
                    hp = (bite < 0) ? 0 : bite;
                } else {
                    i32 bite = m_health - 0xa;
                    hp = (bite < 0) ? 0 : bite;
                }
                m_health = hp;
                if (hp <= 0) {
                    ConsiderArrival(1);
                    m_tileMgr->CellDispatch(m_tileOwnerHi, m_tileOwnerLo, DEATH_NORMAL, -1);
                    return;
                }
            }
            {
                CWwdGameObjectA* obj = m_object;
                CGruntzMgr* reg3 = g_gameReg;
                i32 sy = obj->m_screenY;
                i32 sx = obj->m_screenX;
                const RECT* vr = &reg3->m_world->m_level->m_mainPlane->m_viewRect;
                if (sx < vr->right && sx >= vr->left && sy < vr->bottom && sy >= vr->top) {
                    reg3->m_cueSink->SpawnVoiceDriver(this, 0x348, -1, 0, -1, -1);
                }
            }
            m_entranceSafeTimeLo = 0x3e8;
            m_entranceSafeTimeHi = 0;
            m_entranceClockLo = static_cast<i32>(g_frameTime);
            m_entranceClockHi = 0;
        } else if (flags & 0x2000000) {
            if (m_entranceReason == PICKUP_TOOB) {
                CString* node = g_typeColl.ScratchResolve(m_objAux->m_actKey);
                CString* slot = g_typeColl.Slots();
                i32 n = g_typeColl.m_grown;
                while (n-- != 0) {
                    if (slot != NULL) {
                        slot->CString::CString();
                    }
                    slot++;
                }
                bool nameDiffers = (strcmp(*node, s_codeN) != 0);
                if (nameDiffers) {
                    BuildGruntLoseItemAnimation();
                }
            }
        }
    }

afterTile:
    if (m_entranceActive == 0 && m_entranceCommitted != 0) {
        CWwdGameObjectA* obj = m_object;
        i32 sx = obj->m_screenX;
        if (sx != m_lastTilePx.m_x) {
            goto afterArrival;
        }
        i32 sy = obj->m_screenY;
        if (sy != m_lastTilePx.m_y) {
            goto afterArrival;
        }
        {
            i32 hp = m_health;
            if (hp <= 5 && hp > 0
                && (m_arrivalState == AI_SMARTCHASER || m_arrivalState == AI_HITANDRUNNER)) {
                if (static_cast<u32>(m_dwell) > DWELL_SEEK_PATH_MS) {

                    i32 baseRow = sy >> TILE_SHIFT_PX;
                    i32 baseCol = sx >> TILE_SHIFT_PX;
                    i32 wanderRow = rand() % 6 + baseRow - 3;
                    i32 wanderCol = rand() % 6 + baseCol - 3;
                    TileSwitch(wanderCol, wanderRow, 0, m_arrivalFlags, 0, 0);
                    m_dwell = 0;
                }
                goto afterArrival;
            }
        }
        {

            i32 col5 = m_defenderPx.m_x >> TILE_SHIFT_PX;
            i32 row5 = m_defenderPx.m_y >> TILE_SHIFT_PX;
            i32 reach = m_defenderRadius + m_reachRect.right;
            CMapMgr* grid = g_gameReg->m_tileGrid;

            RECT gb;
            RECT rs;
            RECT box;
            rs.left = col5 - reach;
            rs.top = row5 - reach;
            rs.right = reach + col5 + 1;
            rs.bottom = reach + row5 + 1;
            gb.left = 0;
            gb.top = 0;
            gb.right = grid->m_width;
            gb.bottom = grid->m_height;
            const RECT* pr = &rs;
            if (pr != NULL) {
                box = *pr;
                box.right++;
                box.bottom++;
            } else {
                box = CRect(0, 0, grid->m_width, grid->m_height);
            }

            RECT* bounds = &grid->m_bounds;
            if (!IntersectRect(bounds, &box, &gb)) {
                *bounds = box;
            }
            grid->m_gridW = bounds->right - bounds->left;
            grid->m_gridH = bounds->bottom - bounds->top;
        }
        if (m_arrivalState != AI_NONE) {
            if (static_cast<i64>(g_frameTime) - m_holdAnchor64 >= m_holdWindow64) {
                switch (m_arrivalState) {
                    case AI_DUMBCHASER:
                        ChargeStep();
                        break;
                    case AI_SMARTCHASER:
                        ScanNearestTarget();
                        break;
                    case AI_HITANDRUNNER:
                        WanderStep();
                        break;
                    case AI_DEFENDER:
                        ArrivalReticleScan();
                        break;
                    case AI_POSTGUARD:
                        ResolveArrivalNeighbor();
                        break;
                    case AI_OBJECTGUARD:
                        StepArrivalDefenseAlt();
                        break;
                    case AI_BOMBER:
                        ResolveArrivalReposition();
                        break;
                    case AI_BRICKLAYER:
                        StepBrickLayerBehavior();
                        break;
                    case AI_GOOSUCKER:
                        StepGooSuckerBehavior();
                        break;
                    case AI_DIGGER:
                        StepDiggerBehavior();
                        break;
                    case AI_GAUNTLETZGRUNT:
                        UpdateArrival();
                        break;
                    case AI_TIMEBOMBER:
                        PhaseStep();
                        break;
                    case AI_TOOLTHIEF:
                        SeekTarget();
                        break;
                    case AI_TOYER:
                        StepPeerTracking();
                        break;
                    case AI_MAGICWANDGRUNT:
                        StepArrivalDefenseLean();
                        break;
                    case AI_SCROLLGRUNT:
                        StepArrivalDefense();
                        break;
                }
            }
        } else if (m_poweredUp != 0 && m_neighborValid == 0 && m_combatActive == 0
                   && m_stamina >= STAMINA_FULL && m_neighborScanEnabled != 0) {
            FindGridNeighbor(0);
        }
        {

            CMapMgr* grid = g_gameReg->m_tileGrid;
            RECT ra;
            RECT rb;
            rb.left = 0;
            rb.top = 0;
            rb.right = grid->m_width;
            rb.bottom = grid->m_height;
            ra = CRect(0, 0, grid->m_width, grid->m_height);
            RECT* bounds = &grid->m_bounds;
            if (!IntersectRect(bounds, &ra, &rb)) {
                *bounds = ra;
            }
            grid->m_gridW = bounds->right - bounds->left;
            grid->m_gridH = bounds->bottom - bounds->top;
        }
    }

afterArrival:
    if (m_toyTime > 0) {
        i64 left = m_toyDuration - static_cast<i64>(g_frameTime) + m_toyClock;
        m_toyTime = static_cast<i32>(
            static_cast<double>((left < 0 ? 0 : static_cast<u32>(left)))
                / static_cast<double>(static_cast<u32>(m_toyDurationLo)) * g_wingzScale
            - g_wingzBias
        );
        i64 left2 = m_toyDuration - static_cast<i64>(g_frameTime) + m_toyClock;
        if (static_cast<u32>((left2 < 0 ? 0 : static_cast<u32>(left2))) == 0) {
            m_toyTime = 0;
            if (m_toyTimeSprite != NULL) {
                m_toyTimeSprite->m_flags |= 0x10000;
                m_toyTimeSprite = NULL;
            }
        }
    }

    if (m_stamina < STAMINA_FULL) {
        i64 left = m_attackDowntime64 + m_attackClock64 - static_cast<i64>(g_frameTime);
        if (static_cast<u32>((left < 0 ? 0 : static_cast<u32>(left))) == 0) {
            m_stamina = STAMINA_FULL;
        } else {
            i64 spent = static_cast<i64>(g_frameTime) - m_attackClock64;
            m_stamina = static_cast<i32>(
                static_cast<double>((spent < 0 ? 0 : static_cast<u32>(spent)))
                    / static_cast<double>(static_cast<u32>(m_attackDowntimeLo)) * g_wingzScale
                - g_wingzBias
            );
        }
        if (m_stamina == STAMINA_FULL) {
            if (m_staminaSprite != NULL) {
                m_staminaSprite->m_flags |= 0x10000;
                m_staminaSprite = NULL;
            }
        }
    }

    if (m_wingzEnabled != 0) {
        i64 left = m_wingzDuration64 - static_cast<i64>(g_frameTime) + m_wingzClock64;
        m_wingzTime = static_cast<i32>(
            static_cast<double>((left < 0 ? 0 : static_cast<u32>(left)))
            * DATA_COMPGEN(0x001e9a58, fp_1e9a58, 0.01) - g_wingzBias
            );
        i64 left2 = m_wingzDuration64 - static_cast<i64>(g_frameTime) + m_wingzClock64;
        if (static_cast<u32>((left2 < 0 ? 0 : static_cast<u32>(left2))) == 0) {
            ConsiderArrival(1);
            m_wingzTime = 0;
            LoadWingzGruntSprites(0);
            BuildGruntLoseItemAnimation();
        }
    }

    if (m_arrivalState == AI_BATTLEZ_PATH) {
        if (m_poweredUp != 0 && m_stamina >= STAMINA_FULL) {
            bool eq;
            {
                CString* node = g_typeColl.ScratchResolve(m_objAux->m_actKey);
                CString* slot = g_typeColl.Slots();
                i32 n = g_typeColl.m_grown;
                while (n-- != 0) {
                    if (slot != NULL) {
                        slot->CString::CString();
                    }
                    slot++;
                }
                eq = (strcmp(*node, s_codeE) == 0);
            }
            if (!eq) {
                CString* node = g_typeColl.ScratchResolve(m_objAux->m_actKey);
                CString* slot = g_typeColl.Slots();
                i32 n = g_typeColl.m_grown;
                while (n-- != 0) {
                    if (slot != NULL) {
                        slot->CString::CString();
                    }
                    slot++;
                }
                eq = (strcmp(*node, s_codeA) == 0);
            }
            if (eq) {
                if (m_poweredUp != 0 && m_neighborValid == 0) {
                    m_entranceActive = 0;
                    m_combatActive = 0;
                    m_neighborValid = 0;
                    m_poweredUp = 0;
                    ResetEntranceAnimation(1, 0, 0);
                }
            }
        }
    } else {
        if (static_cast<i64>(g_frameTime) - m_combatClock64 >= m_combatTimeout64) {
            if (m_poweredUp != 0 && m_neighborValid == 0) {
                m_entranceActive = 0;
                m_combatActive = 0;
                m_neighborValid = 0;
                m_poweredUp = 0;
                ResetEntranceAnimation(1, 0, 0);
            }
            if (m_arrived == 0
                && static_cast<i64>(g_frameTime) - m_hudRetireClock64 >= m_hudRetireWindow64) {
                if (m_healthSprite != NULL) {
                    m_healthSprite->m_flags |= 0x10000;
                    m_healthSprite = NULL;
                }
                if (m_toySprite != NULL) {
                    m_toySprite->m_flags |= 0x10000;
                    m_toySprite = NULL;
                }
                if (m_staminaSprite != NULL) {
                    m_staminaSprite->m_flags |= 0x10000;
                    m_staminaSprite = NULL;
                }
            }
        }
    }

kindDispatch:
    if (m_gruntKind != GRUNT_NORMAL) {
        if (m_gruntKind == GRUNT_CONVERSION) {

            if (static_cast<i64>(g_frameTime) - m_convertClock64 < m_convertTime64) {
                return;
            }
            i32 bite = m_health - 5;
            i32 hp = (bite < 0) ? 0 : bite;
            m_health = hp;
            if (hp <= 0) {
                ConsiderArrival(1);
                m_tileMgr->CellDispatch(m_tileOwnerHi, m_tileOwnerLo, DEATH_NORMAL, -1);
                return;
            }
            m_convertTimeLo =
                static_cast<i32>(g_buteMgr.GetDwordDef(s_Powerupz, s_ConversionTime, 0x1f4));
            m_convertTimeHi = 0;
            m_convertClockLo = static_cast<i32>(g_frameTime);
            m_convertClockHi = 0;
            return;
        }
        if (m_gruntKind == GRUNT_INVULNERABLE) {

            if (static_cast<i64>(g_frameTime) - m_shimmerClock64 >= m_shimmerWindow64) {
                i32 pick = rand() % 16;
                if (pick == IDX(m_moveIcon)) {
                    pick = 0x10;
                }
                CShadeTable* sel =
                    g_gameReg->m_spriteFactory->GetSel(pick, m_entranceReason >= PICKUP_TOYZ_FIRST);
                CWwdGameObjectA* obj = m_object;
                ShadeMode cmd = obj->m_drawFillCmd;
                obj->m_drawActive = 1;
                obj->m_drawFillCmd = cmd;
                obj->m_drawFillArg = sel;
            }
        }
        i64 left = m_convertTime64 + m_convertClock64 - static_cast<i64>(g_frameTime);
        i32 leftMs = (left < 0 ? 0 : static_cast<i32>(left));
        if (leftMs <= 0xbb8) {
            if (m_gruntKind == GRUNT_GHOST) {

                i64 rem = m_convertTime64 + m_convertClock64 - static_cast<i64>(g_frameTime);
                u32 remMs = (rem < 0 ? 0 : static_cast<u32>(rem));
                double topaque = static_cast<double>(
                    g_buteMgr.GetIntDef(s_Powerupz, s_GruntGhostTransparencyOn, 0x100)
                );
                i32 frac = static_cast<i32>(
                    topaque * static_cast<double>(remMs)
                    * DATA_COMPGEN(0x001e9a60, fp_1e9a60, 0.0003333333333333333)
                    );
                CWwdGameObjectA* obj = m_object;
                obj->m_drawActive = 1;
                obj->m_drawFillCmd = SHADE_PAL_ALPHA_16;
                obj->m_fillFraction = frac;
            } else {
                CWwdGameObjectA* obj = m_object;
                if (!HAS(obj->m_stateFlags, SPRITE_STATE_FLASHING)) {
                    obj->m_flashInterval = 0x7d;
                    obj->m_flashCountdown = 0;
                    m_object->m_stateFlags |= SPRITE_STATE_FLASHING;
                }
            }
            if (leftMs == 0) {
                switch (m_gruntKind) {
                    case PICKUP_GHOST: {
                        CWwdGameObjectA* obj = m_object;
                        m_gruntKind = GRUNT_NORMAL;
                        obj->m_drawActive = 1;
                        obj->m_drawFillCmd = SHADE_PAL_16;
                        break;
                    }
                    case PICKUP_INVULNERABILITY:
                        m_gruntKind = GRUNT_NORMAL;
                        break;
                    case PICKUP_SUPERSPEED:
                    case PICKUP_ROIDZ:
                    case PICKUP_REACTIVEARMOR: {
                        CWwdGameObjectA* ps = m_powerupSprite;
                        m_gruntKind = GRUNT_NORMAL;
                        if (ps != NULL) {
                            ps->m_flags |= 0x10000;
                            m_powerupSprite = NULL;
                        }
                        break;
                    }
                    case PICKUP_DEATHTOUCH: {
                        CWwdGameObjectA* ps = m_powerupSprite;
                        m_gruntKind = GRUNT_NORMAL;
                        if (ps != NULL) {
                            ps->m_flags |= 0x10000;
                            m_powerupSprite = NULL;
                        }
                        PickupType typeId = m_toolId;
                        m_entranceReason = PICKUP_INVALID;
                        LoadGruntTypeTable(typeId, 1, 0, 0);
                        break;
                    }
                }
                m_object->m_stateFlags &= ~SPRITE_STATE_FLASHING;
                ReadConfigFromButeMgr();
                PickupType reason = m_entranceReason;
                i32 vehicle = (reason >= PICKUP_TOYZ_FIRST) ? 1 : 0;
                i32 variant = 0;
                if (vehicle != 0) {
                    switch (reason) {
                        case PICKUP_BABYWALKER:
                        case PICKUP_BIGWHEEL:
                        case PICKUP_GOKART:
                        case PICKUP_POGOSTICK:
                            variant = 1;
                            break;
                    }
                }
                LoadCellAnimNames(vehicle, variant);
                LoadAnimNameTable(vehicle, variant);
            }
        }
    }

    if (m_pendingTrigger != 0 && m_stamina >= STAMINA_FULL) {
        m_tileMgr->ApplyTriggerA(
            m_tileOwnerHi,
            m_tileOwnerLo,
            m_pendingTriggerPx.m_x,
            m_pendingTriggerPx.m_y
        );
        m_pendingTrigger = 0;
    }
}

// @early-stop
RVA(0x0005ecd0, 0x4f3)
void CGrunt::FinalizeStep(char* name) {
    CUserLogic::FinalizeStep(name);
    AdvanceMotion();
    if (m_struckSlotSound != NULL) {
        bool neL = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), "L") != 0);
        if (neL) {
            bool neG = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), "G") != 0);
            if (neG) {
                StopStruckSlotSound();
            }
        }
    }
    if (m_struckVoiceSound != NULL) {
        if (m_gruntKind == GRUNT_NORMAL) {
            StopStruckVoiceSound();
        } else {
            CGruntzMgr* g = g_gameReg;
            i32 y = m_object->m_screenY;
            i32 x = m_object->m_screenX;
            if (!(x < g->m_viewBounds.right && x >= g->m_viewBounds.left
                  && y < g->m_viewBounds.bottom && y >= g->m_viewBounds.top)) {
                StopStruckVoiceSound();
            }
        }
    }
    bool eqO = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), s_codeO) == 0);
    if (eqO) {

        if (m_object->m_screenX == m_lastTilePx.m_x && m_object->m_screenY == m_lastTilePx.m_y) {
            return;
        }
        GruntDirectionCell c = m_entranceCell;
        i32 row = c.row;
        if (row == GRUNT_DIRECTION_GRID_LOW) {
            row = GRUNT_DIRECTION_GRID_HIGH;
        } else if (row == GRUNT_DIRECTION_GRID_HIGH) {
            row = GRUNT_DIRECTION_GRID_LOW;
        }
        i32 column = c.column;
        if (column == GRUNT_DIRECTION_GRID_LOW) {
            column = GRUNT_DIRECTION_GRID_HIGH;
        } else if (column == GRUNT_DIRECTION_GRID_HIGH) {
            column = GRUNT_DIRECTION_GRID_LOW;
        }
        i32 base = GRUNT_DIRECTION_GRID_WIDTH * row + column;
        double d48 = m_cells[base].m_motion.m_direction.x;
        double d50 = m_cells[base].m_motion.m_direction.y;
        m_movePosX = static_cast<double>(g_frameDelta) * d48 * m_moveSpeed + m_movePosX;
        m_movePosY = static_cast<double>(g_frameDelta) * d50 * m_moveSpeed + m_movePosY;
        i32 nx = static_cast<i32>((m_cells[base].m_motion.m_step.x + m_movePosX));
        i32 ny = static_cast<i32>((m_cells[base].m_motion.m_step.y + m_movePosY));
        if ((d48 > s_fpZero && nx > m_lastTilePx.m_x)
            || (d48 < s_fpZero && nx < m_lastTilePx.m_x)) {
            nx = m_lastTilePx.m_x;
        }
        if ((d50 > s_fpZero && ny > m_lastTilePx.m_y)
            || (d50 < s_fpZero && ny < m_lastTilePx.m_y)) {
            ny = m_lastTilePx.m_y;
        }
        m_object->m_screenX = nx;
        m_object->m_screenY = ny;
        CWwdGameObjectA* h = m_object;
        i32 v = h->m_screenY + 0x186a0;
        if (h->m_sortKey != v) {
            h->m_sortKey = v;
            h->m_flags |= 0x20000;
        }
        return;
    }

    CString* rec = g_typeColl.ScratchResolve(m_objAux->m_actKey);
    GruntPosScratchTeardown();
    bool eqPos = (strcmp(*rec, k_60df94) == 0);
    if (eqPos) {
        if (m_object->m_screenX == m_lastTilePx.m_x && m_object->m_screenY == m_lastTilePx.m_y) {
            return;
        }
        double d48 = EntranceCell()->m_motion.m_direction.x;
        double d50 = EntranceCell()->m_motion.m_direction.y;
        m_movePosX = static_cast<double>(g_frameDelta) * d48 * m_moveSpeed + m_movePosX;
        m_movePosY = static_cast<double>(g_frameDelta) * d50 * m_moveSpeed + m_movePosY;
        i32 nx = static_cast<i32>((EntranceCell()->m_motion.m_step.x + m_movePosX));
        i32 ny = static_cast<i32>((EntranceCell()->m_motion.m_step.y + m_movePosY));
        if (d48 > s_fpZero) {
            if (nx > m_lastTilePx.m_x) {
                nx = m_lastTilePx.m_x;
            }
        } else if (d48 < s_fpZero && nx < m_lastTilePx.m_x) {
            nx = m_lastTilePx.m_x;
        }
        if (d50 > s_fpZero) {
            if (ny > m_lastTilePx.m_y) {
                ny = m_lastTilePx.m_y;
            }
        } else if (d50 < s_fpZero && ny < m_lastTilePx.m_y) {
            ny = m_lastTilePx.m_y;
        }
        m_object->m_screenX = nx;
        m_object->m_screenY = ny;
    }
    return;
}

// @early-stop

RVA(0x0005f310, 0xb5e)
void CGrunt::AdvanceMotion() {
    if (m_arrivalState != AI_BATTLEZ_PATH) {
        bool eq;
        eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), "A") == 0);
        if (eq && CoordCount() != 0) {
            CoordNode* head = CoordHead();
            Coord* co = head->m_coord;
            i32 fl = g_gameReg->m_tileGrid->m_rowInts[co->m_y][co->m_x * 7];
            i32 mask = m_arrivalFlags & fl;
            if (!(fl & 0x20000000) && !(mask & 0x20000000)
                && (mask == 0 || (m_arrivalNotified & fl) != 0)) {
                m_entrancePx.m_x = (co->m_x << TILE_SHIFT_PX) + TILE_HALF_PX;
                m_entrancePx.m_y = (co->m_y << TILE_SHIFT_PX) + TILE_HALF_PX;
                m_coordRetryCount = 0;
                StepEntranceReinit();
            } else if (m_coordRetryCount <= 5) {
                if (PathScan() != 0) {
                    Coord* h2 = (CoordHead())->m_coord;
                    m_entrancePx.m_x = (h2->m_x << TILE_SHIFT_PX) + TILE_HALF_PX;
                    m_entrancePx.m_y = (h2->m_y << TILE_SHIFT_PX) + TILE_HALF_PX;
                    if (CoordCount() != 0) {
                        Coord* h3 = (CoordHead())->m_coord;
                        i32 fl2 = g_gameReg->m_tileGrid->m_rowInts[h3->m_y][h3->m_x * 7];
                        if (!(fl2 & 0x20000000)) {
                            m_coordRetryCount = 0;
                            StepEntranceReinit();
                        }
                    }
                } else {
                    (m_coordRetryCount)++;
                }
            }
        }
    }

    CString* code = g_typeColl.ScratchResolve(m_objAux->m_actKey);
    GruntScratchTeardown();
    bool different = strcmp(*code, s_codeD);
    if (different) {
        code = g_typeColl.ScratchResolve(m_objAux->m_actKey);
        GruntScratchTeardown();
        different = strcmp(*code, s_codeN);
        if (different) {
            code = g_typeColl.ScratchResolve(m_objAux->m_actKey);
            GruntScratchTeardown();
            different = strcmp(*code, s_codeL);
            if (different) {
                code = g_typeColl.ScratchResolve(m_objAux->m_actKey);
                GruntScratchTeardown();
                different = strcmp(*code, s_codeM);
                if (different) {
                    return;
                }
                if (m_bombRunActive != 0) {
                    return;
                }
            } else if (m_entranceStamped != 0) {
                return;
            }
        }
    }
    if (m_object->m_screenX == m_lastTilePx.m_x && m_object->m_screenY == m_lastTilePx.m_y) {
        if (m_arrivalPending != 0) {
            m_tileMgr->WireTileSwitchLogic(this, m_lastTilePx.m_x, m_lastTilePx.m_y);
            m_arrivalPending = 0;

            if (m_arrivalPhase != ARRIVAL_TAG_NONE) {
                i32 result = -1;
                if (m_arrivalPhase == ARRIVAL_TAG_TRIGGER_A) {
                    if (m_coordToggle == 0) {
                        result = m_tileMgr->ApplyTriggerA(
                            m_tileOwnerHi,
                            m_tileOwnerLo,
                            m_arrivalTargetPx.m_x,
                            m_arrivalTargetPx.m_y
                        );
                    } else {
                        CGrunt* other =
                            m_tileMgr->m_grid[m_arrivalCell.m_x * TM_GRID_COLS + m_arrivalCell.m_y];
                        if (other == NULL) {
                            result = 0;
                        } else {
                            i32 x = (other->m_object->m_screenX & ~TILE_MASK_PX) + TILE_HALF_PX;
                            i32 y = (other->m_object->m_screenY & ~TILE_MASK_PX) + TILE_HALF_PX;
                            if (m_defenderPx.m_x != x || m_defenderPx.m_y != y) {
                                m_defenderPx.m_x = x;
                                m_defenderPx.m_y = y;
                                if (StepArrivalDrop(x, y, ARRIVAL_TAG_TRIGGER_A, -1, 1, 0) == 0) {
                                    m_arrivalPhase = ARRIVAL_TAG_NONE;
                                }
                            }

                            i32 targetX;
                            i32 targetY;
                            if (RectContains(x, y) == 0) {
                                if (RectContains(other->m_lastTilePx.m_x, other->m_lastTilePx.m_y)
                                    == 0) {
                                    targetX = m_arrivalTargetPx.m_x;
                                    targetY = m_arrivalTargetPx.m_y;
                                } else {
                                    SnapToLastTile(0);
                                    targetX = other->m_lastTilePx.m_x;
                                    targetY = other->m_lastTilePx.m_y;
                                }
                            } else {
                                targetX = x;
                                targetY = y;
                            }
                            result =
                                m_tileMgr
                                    ->ApplyTriggerA(m_tileOwnerHi, m_tileOwnerLo, targetX, targetY);
                        }
                    }
                } else if (m_arrivalPhase == ARRIVAL_TAG_TRIGGER_B) {
                    if (m_coordToggle == 0) {
                        result = m_tileMgr->ApplyTriggerB(
                            m_tileOwnerHi,
                            m_tileOwnerLo,
                            m_arrivalTargetPx.m_x,
                            m_arrivalTargetPx.m_y
                        );
                    } else {
                        CGrunt* other =
                            m_tileMgr->m_grid[m_arrivalCell.m_x * TM_GRID_COLS + m_arrivalCell.m_y];
                        if (other == NULL) {
                            result = 0;
                        } else {
                            i32 x = (other->m_object->m_screenX & ~TILE_MASK_PX) + TILE_HALF_PX;
                            i32 y = (other->m_object->m_screenY & ~TILE_MASK_PX) + TILE_HALF_PX;
                            if (m_defenderPx.m_x != x || m_defenderPx.m_y != y) {
                                m_defenderPx.m_x = x;
                                m_defenderPx.m_y = y;
                                if (StepArrivalDrop(x, y, ARRIVAL_TAG_TRIGGER_B, -1, 1, 0) == 0) {
                                    m_arrivalPhase = ARRIVAL_TAG_NONE;
                                }
                            }

                            i32 targetX;
                            i32 targetY;
                            if (RectContainsGated(x, y) == 0) {
                                if (RectContainsGated(
                                        other->m_lastTilePx.m_x,
                                        other->m_lastTilePx.m_y
                                    )
                                    == 0) {
                                    targetX = m_arrivalTargetPx.m_x;
                                    targetY = m_arrivalTargetPx.m_y;
                                } else {
                                    SnapToLastTile(0);
                                    targetX = other->m_lastTilePx.m_x;
                                    targetY = other->m_lastTilePx.m_y;
                                }
                            } else {
                                targetX = x;
                                targetY = y;
                            }
                            result =
                                m_tileMgr
                                    ->ApplyTriggerB(m_tileOwnerHi, m_tileOwnerLo, targetX, targetY);
                        }
                    }
                }

                if (result == 1) {
                    SetEntrancePos(1, 1);
                    return;
                }
                if (result == 0) {
                    m_arrivalPhase = 0;
                }
            }
        }

        const char* name = *g_typeColl.GetNameRecord(m_objAux->m_actKey);
        if (strcmp(name, s_codeN) == 0) {
            return;
        }
        name = *g_typeColl.GetNameRecord(m_objAux->m_actKey);
        if (strcmp(name, s_codeL) == 0) {
            if (StepCompassMove() != 0) {
                return;
            }
            m_idleAnchor = 0;
            return;
        }
        name = *g_typeColl.GetNameRecord(m_objAux->m_actKey);
        if (strcmp(name, s_codeM) == 0) {
            if (ClaimSwitchTile() != 0) {
                return;
            }
            m_tileMgr->CellDispatch(m_tileOwnerHi, m_tileOwnerLo, DEATH_NORMAL, -1);
            return;
        }
        if (m_lastTilePx.m_x == m_entrancePx.m_x && m_lastTilePx.m_y == m_entrancePx.m_y) {
            m_arrivalPhase = 0;
            ResetEntranceAnimation(1, 0, 0);
            return;
        }
        if (StepGruntMovement() == 0) {
            return;
        }
    }

    CGruntCellRec* cell = &m_cells[3 * m_entranceCell.row + m_entranceCell.column];
    double dirX = cell->m_motion.m_direction.x;
    double dirY = cell->m_motion.m_direction.y;
    m_movePosX = static_cast<double>(g_frameDelta) * dirX * m_moveSpeed + m_movePosX;
    m_movePosY = static_cast<double>(g_frameDelta) * dirY * m_moveSpeed + m_movePosY;
    i32 x = static_cast<i32>(cell->m_motion.m_step.x + m_movePosX);
    i32 y = static_cast<i32>(cell->m_motion.m_step.y + m_movePosY);
    if ((dirX > 0.0 && x > m_lastTilePx.m_x) || (dirX < 0.0 && x < m_lastTilePx.m_x)) {
        x = m_lastTilePx.m_x;
    }
    if ((dirY > 0.0 && y > m_lastTilePx.m_y) || (dirY < 0.0 && y < m_lastTilePx.m_y)) {
        y = m_lastTilePx.m_y;
    }
    m_object->m_screenX = x;
    m_object->m_screenY = y;
    i32 sortKey = y + 0x186a0;
    CWwdGameObjectA* o = m_object;
    if (o->m_sortKey != sortKey) {
        o->m_sortKey = sortKey;
        o->m_flags |= 0x20000;
    }
}
