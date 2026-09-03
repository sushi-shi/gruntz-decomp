#include <rva.h>

#include <Gruntz/Grunt.h>

#include <Mfc.h>
#include <MfcNoInline.h>
#include <MfcWin.h>

#include <Bute/ButeMgr.h>
#include <Bute/ButeTree.h>
#include <DDrawMgr/AniAdvance.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Dsndmgr/SoundBuffer.h>
#include <Enums.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AniElement.h>
#include <Gruntz/AnimationRegistry.h>
#include <Gruntz/ArrivalFlagsPreset.h>
#include <Gruntz/BattlezMapConfig.h>
#include <Gruntz/BattlezTask.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/CurPlayer.h>
#include <Gruntz/DirectionClassify.h>
#include <Gruntz/EnemyAiType.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/FreeNodePoolInline.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameModeId.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GameStats.h>
#include <Gruntz/GruntAiState.h>
#include <Gruntz/GruntDeathType.h>
#include <Gruntz/GruntEntranceArrival.h>
#include <Gruntz/GruntEntranceMove.h>
#include <Gruntz/GruntHealthSprite.h>
#include <Gruntz/GruntIdentity.h>
#include <Gruntz/GruntMovementInline.h>
#include <Gruntz/GruntPowerupSprite.h>
#include <Gruntz/GruntSelectedSprite.h>
#include <Gruntz/GruntToySprite.h>
#include <Gruntz/GruntzCommandId.h>
#include <Gruntz/GruntzMapMgr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/HealthPct.h>
#include <Gruntz/ImageSets.h>
#include <Gruntz/InGameIcon.h>
#include <Gruntz/MapCellFlags.h>
#include <Gruntz/MovingLogicSerial.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/Play.h>
#include <Gruntz/Projectile.h>
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
#include <Gruntz/VoiceManager.h>
#include <Ints.h>
#include <MakeRect.h>
#include <Pix16.h>
#include <Rez/FrameClock.h>
#include <Rez/RezArchiveDir.h>
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

DATA(0x001e9750)
const double g_slopeNegHalf = -0.5;
DATA(0x001e9758)
const double g_slopePosHalf = 0.5;
DATA(0x001e9760)
const double g_slopePosTwo = 2.0;
DATA(0x001e9768)
const double g_slopeNegTwo = -2.0;

i32 g_movingSeed;

DATA(0x0020d424)
static char s_d48_BREAK[] = "_BREAK";
DATA(0x0020d42c)
static char s_d48_SOUTHEAST[] = "_SOUTHEAST";
DATA(0x0020d43c)
static char s_d48_SOUTH[] = "_SOUTH";
DATA(0x0020d444)
static char s_d48_SOUTHWEST[] = "_SOUTHWEST";
DATA(0x0020d454)
static char s_d48_EAST[] = "_EAST";
DATA(0x0020d45c)
static char s_d48_WEST[] = "_WEST";
DATA(0x0020d464)
static char s_d48_NORTHEAST[] = "_NORTHEAST";
DATA(0x0020d474)
static char s_d48_NORTH[] = "_NORTH";
DATA(0x0020d47c)
static char s_d48_NORTHWEST[] = "_NORTHWEST";
DATA(0x0020d48c)
static char s_d48_SOUTHEAST_ITEM[] = "_SOUTHEAST_ITEM";
DATA(0x0020d4a0)
static char s_d48_SOUTH_ITEM[] = "_SOUTH_ITEM";
DATA(0x0020d4b0)
static char s_d48_SOUTHWEST_ITEM[] = "_SOUTHWEST_ITEM";
DATA(0x0020d4c4)
static char s_d48_EAST_ITEM[] = "_EAST_ITEM";
DATA(0x0020d4d4)
static char s_d48_WEST_ITEM[] = "_WEST_ITEM";
DATA(0x0020d4e4)
static char s_d48_NORTHEAST_ITEM[] = "_NORTHEAST_ITEM";
DATA(0x0020d4f8)
static char s_d48_NORTH_ITEM[] = "_NORTH_ITEM";
DATA(0x0020d508)
static char s_d48_NORTHWEST_ITEM[] = "_NORTHWEST_ITEM";
DATA(0x0020d51c)
static char s_d48_SOUTHEAST_IDLE[] = "_SOUTHEAST_IDLE";
DATA(0x0020d530)
static char s_d48_SOUTH_IDLE[] = "_SOUTH_IDLE";
DATA(0x0020d540)
static char s_d48_SOUTHWEST_IDLE[] = "_SOUTHWEST_IDLE";
DATA(0x0020d554)
static char s_d48_EAST_IDLE[] = "_EAST_IDLE";
DATA(0x0020d564)
static char s_d48_WEST_IDLE[] = "_WEST_IDLE";
DATA(0x0020d574)
static char s_d48_NORTHEAST_IDLE[] = "_NORTHEAST_IDLE";
DATA(0x0020d588)
static char s_d48_NORTH_IDLE[] = "_NORTH_IDLE";
DATA(0x0020d598)
static char s_d48_NORTHWEST_IDLE[] = "_NORTHWEST_IDLE";
DATA(0x0020d5ac)
static char s_d48_SOUTHEAST_ATTACK[] = "_SOUTHEAST_ATTACK";
DATA(0x0020d5c4)
static char s_d48_SOUTH_ATTACK[] = "_SOUTH_ATTACK";
DATA(0x0020d5d4)
static char s_d48_SOUTHWEST_ATTACK[] = "_SOUTHWEST_ATTACK";
DATA(0x0020d5ec)
static char s_d48_EAST_ATTACK[] = "_EAST_ATTACK";
DATA(0x0020d5fc)
static char s_d48_WEST_ATTACK[] = "_WEST_ATTACK";
DATA(0x0020d60c)
static char s_d48_NORTHEAST_ATTACK[] = "_NORTHEAST_ATTACK";
DATA(0x0020d624)
static char s_d48_NORTH_ATTACK[] = "_NORTH_ATTACK";
DATA(0x0020d634)
static char s_d48_NORTHWEST_ATTACK[] = "_NORTHWEST_ATTACK";
DATA(0x0020d64c)
static char s_d48_SOUTHEAST_STRUCK[] = "_SOUTHEAST_STRUCK";
DATA(0x0020d664)
static char s_d48_SOUTH_STRUCK[] = "_SOUTH_STRUCK";
DATA(0x0020d674)
static char s_d48_SOUTHWEST_STRUCK[] = "_SOUTHWEST_STRUCK";
DATA(0x0020d68c)
static char s_d48_EAST_STRUCK[] = "_EAST_STRUCK";
DATA(0x0020d69c)
static char s_d48_WEST_STRUCK[] = "_WEST_STRUCK";
DATA(0x0020d6ac)
static char s_d48_NORTHEAST_STRUCK[] = "_NORTHEAST_STRUCK";
DATA(0x0020d6c4)
static char s_d48_NORTH_STRUCK[] = "_NORTH_STRUCK";
DATA(0x0020d6d4)
static char s_d48_NORTHWEST_STRUCK[] = "_NORTHWEST_STRUCK";
DATA(0x0020d6ec)
static char s_d48_SOUTHEAST_WALK[] = "_SOUTHEAST_WALK";
DATA(0x0020d700)
static char s_d48_SOUTH_WALK[] = "_SOUTH_WALK";
DATA(0x0020d710)
static char s_d48_SOUTHWEST_WALK[] = "_SOUTHWEST_WALK";
DATA(0x0020d724)
static char s_d48_EAST_WALK[] = "_EAST_WALK";
DATA(0x0020d734)
static char s_d48_WEST_WALK[] = "_WEST_WALK";
DATA(0x0020d744)
static char s_d48_NORTHEAST_WALK[] = "_NORTHEAST_WALK";
DATA(0x0020d758)
static char s_d48_NORTH_WALK[] = "_NORTH_WALK";
DATA(0x0020d768)
static char s_d48_NORTHWEST_WALK[] = "_NORTHWEST_WALK";
DATA(0x0020d77c)
static char s_pose_TOYBREAK[] = "_TOY-BREAK";
DATA(0x0020d78c)
static char s_pose_TOY2[] = "_TOY2";
DATA(0x0020d794)
static char s_pose_TOY1[] = "_TOY1";
DATA(0x0020d79c)
static char s_pose_ITEM2[] = "_ITEM2";
DATA(0x0020d7a4)
static char s_pose_ITEM[] = "_ITEM";
DATA(0x0020d7ac)
static char s_pose_IDLE5[] = "_IDLE5";
DATA(0x0020d7b4)
static char s_pose_STRUCK2[] = "_STRUCK2";
DATA(0x0020d7c0)
static char s_pose_STRUCK1[] = "_STRUCK1";
DATA(0x0020d7cc)
static char s_pose_ATTACKIDLE[] = "_ATTACK-IDLE";
DATA(0x0020d7dc)
static char s_pose_ATTACK2[] = "_ATTACK2";
DATA(0x0020d7e8)
static char s_pose_ATTACK1[] = "_ATTACK1";
DATA(0x002455b0)
b32 g_traitorMode;

static inline CAniElement* FindAnimElement(CMapStringToPtr& map, LPCTSTR key) {
    CAniElement* out = NULL;
    MapLookup(map, key, out);
    return out;
}

#define LOAD_POSE(dst, sfx)                                                                        \
    ((dst) = FindAnimElement(                                                                      \
         m_wwdObject->OwnerMgr()->m_animRegistry->m_animations,                                    \
         "GRUNTZ_" + m_animSetName + (sfx)                                                         \
     ))

RVA_COMPGEN(0x0000f2c0, 0x1e, ??_GCGrunt@@UAEPAXI@Z)
RVA(0x0000f2f0, 0xc8)
CGrunt::~CGrunt() {
    OnObjectRemoved();
}

RVA(0x0000f400, 0x1b)
CGruntCellRec::CGruntCellRec() {}

RVA(0x0000f430, 0x10)
CGruntCellRec::~CGruntCellRec() {}

// @early-stop
RVA(0x00047a10, 0x770)
CGrunt::CGrunt(CGameObject* owner)
    : CMovingLogic(owner, CMovingLogic::GRUNT_SCALE),
      CWapX(owner),
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
    m_entranceCell = g_gruntMoveDirSouth;
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
    m_arrived = false;
    m_wwdObject->m_objectType = WWD_OBJECT_TYPE_GRUNT;
    m_wwdObject->m_hitTypeFlags = WWD_GRUNT_HIT_TYPE_FLAGS;
    SetObjectFlags(WWD_GAME_OBJECT_FLAGS_CULL_SOUND_COLLIDE);
    m_wwdObject->m_collMask |= WWD_GRUNT_COLLISION_MASK;
    m_wwdObject->m_attackTypeMask = WWD_GRUNT_ATTACK_TYPE_MASK;
    m_playerIndex = -1;
    m_unitIndex = -1;
    m_neighborPlayerIndex = -1;
    m_warpstoneAnchorIndex = 0;
    m_entranceReason = PICKUP_NONE;
    m_vehiclePickupType = PICKUP_NONE;
    m_brickPickupType = PICKUP_NONE;
    m_gruntKind = GRUNT_NORMAL;
    m_toolId = PICKUP_NONE;
    m_animSetName = "NORMALGRUNT";
    m_neighborUnitIndex = -1;
    m_entranceCommitted = true;
    m_healthSprite = NULL;
    m_staminaSprite = NULL;
    m_toyTimeSprite = NULL;
    m_wingzTimeSprite = NULL;
    m_selectedSprite = NULL;
    m_toySprite = NULL;
    m_powerupSprite = NULL;
    m_reserved210 = 0;
    m_combatActive = false;
    m_neighborValid = false;
    m_arrivalActive = false;
    m_coordToggle = false;
    m_wingzEnabled = false;
    m_vehicleLoopSound = NULL;
    m_powerupLoopSound = NULL;
    CRect reach = MakeRect(-1, -1, 1, 1);
    m_reachRect = reach;
    CRect zero = MakeRect(0, 0, 0, 0);
    m_reachExclusionRect = zero;
    m_vehicleContactRect = zero;
    m_vehicleContactExclusionRect = zero;

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
    m_unusedBattleCell.Set(-1, -1);
    m_arrivalNotified = false;
    m_defenderState = AISTATE_SEEK;
    m_battleState = BZTASK_UNASSIGNED;
    {
        CWwdSpriteObject* h = m_object;
        i32 lim = h->m_screenPosition.m_y + 0x186a0;
        if (h->m_sortKey != lim) {
            h->m_sortKey = lim;
            h->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_SORT_PENDING);
        }
    }
    m_blockedVoicePending = true;
}

RVA(0x00048360, 0x7e)
void CGrunt::OnObjectRemoved() {
    if (CoordCount() != 0) {

        POSITION pos = m_coordList.GetHeadPosition();
        while (pos != NULL) {
            Coord* buf = static_cast<Coord*>(m_coordList.GetNext(pos));
            if (buf) {
                CoordPoolNode* slot = g_coordPool.NodeOf(buf);
                slot->m_next = g_coordPool.m_freeHead;
                g_coordPool.m_freeHead = slot;
            }
        }
        m_coordList.RemoveAll();
    }

    while (true) {
        i32 n = PayloadCount();
        i32* head = (n == 0) ? NULL : static_cast<i32*>(m_payloads.GetHead());
        if (head == NULL) {
            return;
        }
        if (n == 0) {
            continue;
        }
        i32* p = static_cast<i32*>(m_payloads.RemoveHead());
        delete[] p;
    }
}

RVA(0x00048400, 0x47)
void CGrunt::ReadConfigFromButeMgr() {
    m_reserved18c = 0;
    m_reserved418 = 0;

    m_timePerTile = g_buteMgr.GetDword(
        const_cast<char*>(static_cast<const char*>(m_animSetName)),
        "TimePerTile",
        1000
    );

    if (m_gruntKind == GRUNT_SUPERSPEED) {
        m_timePerTile >>= 1;
    }
}

RVA(0x00048470, 0x131b)
void CGrunt::LoadCellAnimNames(i32 kind, i32 dirOnly) {
    if (kind == 0) {
        m_cells[0].WalkName() = "GRUNTZ_" + m_animSetName + s_d48_NORTHWEST_WALK;
        m_cells[1].WalkName() = "GRUNTZ_" + m_animSetName + s_d48_NORTH_WALK;
        m_cells[2].WalkName() = "GRUNTZ_" + m_animSetName + s_d48_NORTHEAST_WALK;
        m_cells[3].WalkName() = "GRUNTZ_" + m_animSetName + s_d48_WEST_WALK;
        m_cells[4].WalkName() = "GRUNTZ_" + m_animSetName + s_d48_NORTH_WALK;
        m_cells[5].WalkName() = "GRUNTZ_" + m_animSetName + s_d48_EAST_WALK;
        m_cells[6].WalkName() = "GRUNTZ_" + m_animSetName + s_d48_SOUTHWEST_WALK;
        m_cells[7].WalkName() = "GRUNTZ_" + m_animSetName + s_d48_SOUTH_WALK;
        m_cells[8].WalkName() = "GRUNTZ_" + m_animSetName + s_d48_SOUTHEAST_WALK;
        m_cells[0].StruckName() = "GRUNTZ_" + m_animSetName + s_d48_NORTHWEST_STRUCK;
        m_cells[1].StruckName() = "GRUNTZ_" + m_animSetName + s_d48_NORTH_STRUCK;
        m_cells[2].StruckName() = "GRUNTZ_" + m_animSetName + s_d48_NORTHEAST_STRUCK;
        m_cells[3].StruckName() = "GRUNTZ_" + m_animSetName + s_d48_WEST_STRUCK;
        m_cells[4].StruckName() = "GRUNTZ_" + m_animSetName + s_d48_NORTH_STRUCK;
        m_cells[5].StruckName() = "GRUNTZ_" + m_animSetName + s_d48_EAST_STRUCK;
        m_cells[6].StruckName() = "GRUNTZ_" + m_animSetName + s_d48_SOUTHWEST_STRUCK;
        m_cells[7].StruckName() = "GRUNTZ_" + m_animSetName + s_d48_SOUTH_STRUCK;
        m_cells[8].StruckName() = "GRUNTZ_" + m_animSetName + s_d48_SOUTHEAST_STRUCK;
        m_cells[0].AttackName() = "GRUNTZ_" + m_animSetName + s_d48_NORTHWEST_ATTACK;
        m_cells[1].AttackName() = "GRUNTZ_" + m_animSetName + s_d48_NORTH_ATTACK;
        m_cells[2].AttackName() = "GRUNTZ_" + m_animSetName + s_d48_NORTHEAST_ATTACK;
        m_cells[3].AttackName() = "GRUNTZ_" + m_animSetName + s_d48_WEST_ATTACK;
        m_cells[4].AttackName() = "GRUNTZ_" + m_animSetName + s_d48_NORTH_ATTACK;
        m_cells[5].AttackName() = "GRUNTZ_" + m_animSetName + s_d48_EAST_ATTACK;
        m_cells[6].AttackName() = "GRUNTZ_" + m_animSetName + s_d48_SOUTHWEST_ATTACK;
        m_cells[7].AttackName() = "GRUNTZ_" + m_animSetName + s_d48_SOUTH_ATTACK;
        m_cells[8].AttackName() = "GRUNTZ_" + m_animSetName + s_d48_SOUTHEAST_ATTACK;
        m_cells[0].IdleName() = "GRUNTZ_" + m_animSetName + s_d48_NORTHWEST_IDLE;
        m_cells[1].IdleName() = "GRUNTZ_" + m_animSetName + s_d48_NORTH_IDLE;
        m_cells[2].IdleName() = "GRUNTZ_" + m_animSetName + s_d48_NORTHEAST_IDLE;
        m_cells[3].IdleName() = "GRUNTZ_" + m_animSetName + s_d48_WEST_IDLE;
        m_cells[4].IdleName() = "GRUNTZ_" + m_animSetName + s_d48_NORTH_IDLE;
        m_cells[5].IdleName() = "GRUNTZ_" + m_animSetName + s_d48_EAST_IDLE;
        m_cells[6].IdleName() = "GRUNTZ_" + m_animSetName + s_d48_SOUTHWEST_IDLE;
        m_cells[7].IdleName() = "GRUNTZ_" + m_animSetName + s_d48_SOUTH_IDLE;
        m_cells[8].IdleName() = "GRUNTZ_" + m_animSetName + s_d48_SOUTHEAST_IDLE;
        m_cells[0].ItemName() = "GRUNTZ_" + m_animSetName + s_d48_NORTHWEST_ITEM;
        m_cells[1].ItemName() = "GRUNTZ_" + m_animSetName + s_d48_NORTH_ITEM;
        m_cells[2].ItemName() = "GRUNTZ_" + m_animSetName + s_d48_NORTHEAST_ITEM;
        m_cells[3].ItemName() = "GRUNTZ_" + m_animSetName + s_d48_WEST_ITEM;
        m_cells[4].ItemName() = "GRUNTZ_" + m_animSetName + s_d48_NORTH_ITEM;
        m_cells[5].ItemName() = "GRUNTZ_" + m_animSetName + s_d48_EAST_ITEM;
        m_cells[6].ItemName() = "GRUNTZ_" + m_animSetName + s_d48_SOUTHWEST_ITEM;
        m_cells[7].ItemName() = "GRUNTZ_" + m_animSetName + s_d48_SOUTH_ITEM;
        m_cells[8].ItemName() = "GRUNTZ_" + m_animSetName + s_d48_SOUTHEAST_ITEM;
        m_deathFrameSetName = "GRUNTZ_" + m_animSetName + "_DEATH";
    } else if (dirOnly != 0) {
        m_cells[0].WalkName() = "GRUNTZ_" + m_animSetName + s_d48_NORTHWEST;
        m_cells[1].WalkName() = "GRUNTZ_" + m_animSetName + s_d48_NORTH;
        m_cells[2].WalkName() = "GRUNTZ_" + m_animSetName + s_d48_NORTHEAST;
        m_cells[3].WalkName() = "GRUNTZ_" + m_animSetName + s_d48_WEST;
        m_cells[4].WalkName() = "GRUNTZ_" + m_animSetName + s_d48_NORTH;
        m_cells[5].WalkName() = "GRUNTZ_" + m_animSetName + s_d48_EAST;
        m_cells[6].WalkName() = "GRUNTZ_" + m_animSetName + s_d48_SOUTHWEST;
        m_cells[7].WalkName() = "GRUNTZ_" + m_animSetName + s_d48_SOUTH;
        m_cells[8].WalkName() = "GRUNTZ_" + m_animSetName + s_d48_SOUTHEAST;
        m_frameSetName = "GRUNTZ_" + m_animSetName + s_d48_BREAK;
    } else {
        m_frameSetName = "GRUNTZ_" + m_animSetName;
    }
    CShadeTable* sel = g_gameReg->m_spriteFactory->GetSel(IDX(m_moveIcon), kind);
    CWwdSpriteObject* h = m_object;
    ShadeMode fillCmd = h->m_drawFillCmd;

    SET_DRAW_FILL_SPLIT(m_object, h, fillCmd, sel);
}

RVA(0x00049c60, 0x8d1)
void CGrunt::LoadAnimNameTable(i32 kind, i32 toyOnly) {
    if (kind == 0) {
        LOAD_POSE(m_poseWalk, "_WALK");
        LOAD_POSE(AT(m_poseAttack, GRUNT_ATTACK1), s_pose_ATTACK1);
        LOAD_POSE(AT(m_poseAttack, GRUNT_ATTACK2), s_pose_ATTACK2);
        LOAD_POSE(m_poseAttackIdle, s_pose_ATTACKIDLE);
        LOAD_POSE(AT(m_poseStruck, GRUNT_STRUCK1), s_pose_STRUCK1);
        LOAD_POSE(AT(m_poseStruck, GRUNT_STRUCK2), s_pose_STRUCK2);
        LOAD_POSE(AT(m_poseIdle, GRUNT_IDLE1), "_IDLE1");
        LOAD_POSE(AT(m_poseIdle, GRUNT_IDLE2), "_IDLE2");
        LOAD_POSE(AT(m_poseIdle, GRUNT_IDLE3), "_IDLE3");
        LOAD_POSE(AT(m_poseIdle, GRUNT_IDLE4), "_IDLE4");
        LOAD_POSE(AT(m_poseIdle, GRUNT_IDLE5), s_pose_IDLE5);
        LOAD_POSE(AT(m_poseItem, GRUNT_ITEM1), s_pose_ITEM);
        LOAD_POSE(AT(m_poseItem, GRUNT_ITEM2), s_pose_ITEM2);
        LOAD_POSE(m_poseDeath, "_DEATH");
        return;
    }

    if (toyOnly != 0) {
        LOAD_POSE(m_poseWalk, "_WALK");
    } else {
        LOAD_POSE(AT(m_poseToy, GRUNT_TOY1), s_pose_TOY1);

        i32 toy1FrameCount = AT(m_poseToy, GRUNT_TOY1)->m_records.GetSize();
        LOAD_POSE(AT(m_poseToy, GRUNT_TOY2), s_pose_TOY2);
        i32 toy2FrameCount = AT(m_poseToy, GRUNT_TOY2)->m_records.GetSize();

        if (toy1FrameCount < toy2FrameCount) {
            double blend =
                DATA_COMPGEN(0x001e9748, 100.0) / (static_cast<double>(toy2FrameCount) / toy1FrameCount - DATA_COMPGEN(0x001e9740, -1.0)) - g_slopeNegHalf;
            i32 pct = static_cast<i32>(blend);
            m_toyBlendPct = 100 - pct;
        } else {
            m_toyBlendPct = static_cast<i32>(
                100.0 / (static_cast<double>(toy1FrameCount) / toy2FrameCount - -1.0)
                - g_slopeNegHalf
            );
        }
    }

    LOAD_POSE(AT(m_poseToy, GRUNT_TOY_BREAK), s_pose_TOYBREAK);
}

#undef LOAD_POSE

// @early-stop
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0004a780, 0x1ec)
GruntDirectionCell* MotionEntity::Classify(MotionEntity* other, char exact) {
    if (other == NULL) {
        return &g_gruntMoveDirCenter;
    }
    DoubleVector2 delta = other->m_position - m_position;
    delta.y = -delta.y;
    Coord integerDelta = delta.ToCoord();
    if (integerDelta.m_x == 0) {
        if (integerDelta.m_y > 0) {
            return &g_gruntMoveDirNorth;
        }
        if (integerDelta.m_y < 0) {
            return &g_gruntMoveDirSouth;
        }
        return &g_gruntMoveDirCenter;
    }

    char onCell = exact;
    if (onCell) {
        onCell = (m_position.ToCoord() == m_gridPosition) ? 1 : 0;
    }
    double ratio = static_cast<double>(integerDelta.m_y) / static_cast<double>(integerDelta.m_x);

    if (integerDelta.m_y >= 0 && integerDelta.m_x > 0) {
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
    if (integerDelta.m_y >= 0 && integerDelta.m_x < 0) {
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
    if (integerDelta.m_y <= 0 && integerDelta.m_x > 0) {
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

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0004a9f0, 0x1aa)
i32 CGrunt::IntersectsTileObjectAxes() {
    CGrunt* tgt =
        m_triggerMgr->FindAtPixel(m_object->m_screenPosition.m_x, m_object->m_screenPosition.m_y);
    if (tgt == NULL) {
        return 0;
    }
    CRect r = tgt->m_wwdObject->m_area;
    CGameObject* th = tgt->m_object;
    r.OffsetRect(th->m_screenPosition.m_x, th->m_screenPosition.m_y);

    Coord center = m_object->ScreenPos();
    CPoint b(center.m_x, center.m_y - 0x3e8);
    CPoint a(center.m_x, center.m_y + 0x3e8);
    if (RectSegProbe(&r, &b, &a)) {
        return 1;
    }

    b = CPoint(center.m_x - 0x3e8, center.m_y);
    a = CPoint(center.m_x + 0x3e8, center.m_y);
    if (RectSegProbe(&r, &b, &a)) {
        return 1;
    }

    b = CPoint(center.m_x - 0x3e8, center.m_y - 0x3e8);
    a = CPoint(center.m_x + 0x3e8, center.m_y + 0x3e8);
    if (RectSegProbe(&r, &b, &a)) {
        return 1;
    }

    b = CPoint(center.m_x - 0x3e8, center.m_y + 0x3e8);
    a = CPoint(center.m_x + 0x3e8, center.m_y - 0x3e8);
    return RectSegProbe(&r, &b, &a) != 0;
}

// @early-stop
RVA(0x0004ac10, 0x402)
void CGrunt::SetFacing(i32 unused, GruntDirectionCell facing) {
    static_cast<void>(unused);
    if (SameCellTag(&m_entranceCell, &facing)) {
        return;
    }

    bool eq;
    eq = ANIMATION_ACT_EQUALS("F");
    if (eq) {
        return;
    }
    bool ne;
    ne = ANIMATION_ACT_DIFFERS("D");
    if (ne) {
        eq = ANIMATION_ACT_EQUALS("A");
        if (!eq) {
            eq = ANIMATION_ACT_EQUALS("K");
            if (!eq) {
                eq = ANIMATION_ACT_EQUALS("E");
                if (eq) {

                    SwitchAnimation(m_poseAttackIdle);
                    {
                        CAniElement* desc = m_wwdObject->m_animationCursor.m_animation;
                        CAniRecordView* elem =
                            desc->m_records.GetSize() > 0
                                ? static_cast<CAniRecordView*>(desc->m_records.GetAt(0))
                                : NULL;
                        i32 frame = elem->m_param;
                        const char* nm = EntranceCell()->AttackName().GetBuffer(0);
                        SetImageFrameByName(nm, frame);
                    }
                    goto store;
                }
                eq = ANIMATION_ACT_EQUALS("I");
                if (!eq) {
                    eq = ANIMATION_ACT_EQUALS("M");
                    if (!eq) {
                        goto walk;
                    }
                }

                m_entranceCell = facing;
                SwitchAnimation(AT(m_poseIdle, GRUNT_IDLE2));
                ResetEntranceAnimation(1, 0, 0);
                return;
            }
        }

        SwitchAnimationAndMaybeAdvance(AT(m_poseIdle, GRUNT_IDLE1), 0);
        {
            CAniElement* desc = m_wwdObject->m_animationCursor.m_animation;
            CAniRecordView* elem = desc->m_records.GetSize() > 0
                                       ? static_cast<CAniRecordView*>(desc->m_records.GetAt(0))
                                       : NULL;
            i32 frame = elem->m_param;
            const char* nm = m_cells[3 * facing.row + facing.column].IdleName().GetBuffer(0);
            SetImageFrameByName(nm, frame);
        }
        goto store;
    }

walk:

    SwitchAnimation(m_poseWalk);
    {
        const char* nm = m_cells[3 * facing.row + facing.column].WalkName().GetBuffer(0);
        SetImageSetByName(nm);
    }

store:
    m_entranceCell = facing;
}

// @early-stop
RVA(0x0004b130, 0xc8)
i32 CGrunt::CommitArrival() {
    if (m_arrived != false) {
        return 1;
    }

    if (m_tileClaimed != false && g_gameReg->m_gameMode == GAMEMODE_MULTIPLAYER) {
        m_triggerMgr->EnqueueGuardEnd(m_playerIndex, m_unitIndex);
    } else if (m_tileClaimed != false) {
        m_arrivalReroll64 = 0;
        m_arrivalRerollWindow64 = 0;
        m_tileClaimed = false;
        m_arrivalState = AI_NONE;
        m_arrivalFlags &= ~IDX(
            CELL_FLAG_SPECIAL | CELL_FLAG_SPIKES | CELL_FLAG_IN_GAME_ICON | CELL_FLAG_STATIC_HAZARD
            | CELL_FLAG_ROLLING_BALL
        );
        SetEntrancePos(1, 1);
    }
    CreateSelectedSprite();
    CreateHealthSprite();
    CreateToySprite();
    CreateStaminaSprite();
    CreateToyTimeSprite();
    CreateWingzTimeSprite();
    m_arrived = true;
    return 1;
}

RVA(0x0004b240, 0xaa)
void CGrunt::ClearAllSprites() {
    if (m_selectedSprite) {
        m_selectedSprite->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);
        m_selectedSprite = NULL;
    }
    if (m_healthSprite) {
        m_healthSprite->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);
        m_healthSprite = NULL;
    }
    if (m_toySprite) {
        m_toySprite->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);
        m_toySprite = NULL;
    }
    if (m_entranceCommitted == false) {
        if (m_staminaSprite) {
            m_staminaSprite->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);
            m_staminaSprite = NULL;
        }
        if (m_toyTimeSprite) {
            m_toyTimeSprite->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);
            m_toyTimeSprite = NULL;
        }
        if (m_wingzTimeSprite) {
            m_wingzTimeSprite->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);
            m_wingzTimeSprite = NULL;
        }
    }
    m_arrived = false;
}

RVA(0x0004b320, 0x34)
i32 CGrunt::TileSwitch(
    i32 col,
    i32 row,
    i32 arrivalPhase,
    i32 blockedMask,
    i32 clearEndpointFlags,
    i32 extraPassableMask
) {
    Coord center(col, row);
    TileCenter(&center);
    return StepArrivalDrop(
        center.m_x,
        center.m_y,
        arrivalPhase,
        blockedMask,
        clearEndpointFlags,
        extraPassableMask
    );
}

RVA(0x0004b370, 0xb30)
i32 CGrunt::StepArrivalDrop(
    i32 pxX,
    i32 pxY,
    i32 arrivalPhase,
    i32 blockedMask,
    i32 clearEndpointFlags,
    i32 extraPassableMask
) {
    CoordNode* n;
    CoordNode* cur;
    Coord* tail;
    POSITION pos;
    Coord lastTile;
    Coord tile;
    i32 passableMask, cnt, headFlags, lastFlags, hit;
    i32 reinit;
    i32 nudged;
    RockNeighborMask free4;
    i32 step, acc, err, blocked;
    Coord walk;
    Coord delta;
    Coord distance;
    Coord pixel;
    i32 saved[3][3];
    Coord scan;
    bool eq;

    pixel.Set(pxX, pxY);
    m_pendingTrigger = false;
    eq = ANIMATION_ACT_DIFFERS("D");
    if (!eq && pixel == m_entrancePx) {
        goto commitPhase;
    }

    RecycleGruntCoords(this);
    lastTile = m_lastTilePx;
    ScreenTile(&lastTile);
    tile = pixel;
    ScreenTile(&tile);
    if (blockedMask == -1) {
        blockedMask = m_arrivalFlags;
    }
    m_arrivalTargetPx = pixel;
    passableMask = extraPassableMask | m_passableMask;
    if (g_gameReg->m_tileGrid->FindPathWithEndpointOverrides(
            lastTile.m_x,
            lastTile.m_y,
            tile.m_x,
            tile.m_y,
            &m_coordList,
            clearEndpointFlags,
            blockedMask,
            passableMask
        )
        != 0) {
        if (CoordCount() != 0) {
            PushFreeNode(&g_coordPool, m_coordList.RemoveHead());
        }
    pathGate:
        reinit = 1;
        cnt = CoordCount();
        if (cnt == 0) {
            goto commitEntrance;
        }
        tail = CoordHead()->m_coord;
        headFlags = g_gameReg->m_tileGrid->CellFlagsAt(tail->m_x, tail->m_y);
        lastFlags = g_gameReg->m_tileGrid->CellFlagsAt(lastTile.m_x, lastTile.m_y);
        if ((lastFlags & IDX(CELL_FLAG_ARROW)) != 0) {
            goto commitEntrance;
        }
        if ((headFlags & BRICKZ_CELL_OCCUPIED) == 0) {
            hit = headFlags & blockedMask;
            if ((hit & BRICKZ_CELL_OCCUPIED) == 0) {
                if (hit == 0) {
                    goto commitEntrance;
                }
                if ((headFlags & passableMask) != 0) {
                    goto commitEntrance;
                }
            }
        }
        if (cnt == 1 && m_arrivalPending == false) {

            SetEntrancePos(1, 1);
            if (m_object->ScreenPos() == m_lastTilePx) {
                FaceTowardTile(tile);
            }
        returnZero:
            return 0;
        }
        if (m_arrivalState == AI_BATTLEZ_PATH) {
            reinit = 0;
            goto commitEntrance;
        }
        {

            CPtrList probe(10);
            if (g_gameReg->m_tileGrid->FindPathWithEndpointOverrides(
                    lastTile.m_x,
                    lastTile.m_y,
                    tile.m_x,
                    tile.m_y,
                    &probe,
                    clearEndpointFlags,
                    blockedMask | BRICKZ_CELL_OCCUPIED,
                    passableMask
                ) != 0
                && probe.GetCount() != 0) {
                if (probe.GetCount() <= cnt + 3) {
                    PushFreeNode(&g_coordPool, probe.RemoveHead());
                    if (CoordCount() != 0) {
                        n = CoordHead();
                        while (NULL != n) {
                            cur = n;
                            n = n->m_next;
                            if (cur->m_coord != NULL) {
                                PushFreeNode(&g_coordPool, cur->m_coord);
                            }
                        }
                        m_coordList.RemoveAll();
                    }
                    pos = probe.GetHeadPosition();
                    while (pos != NULL) {
                        m_coordList.AddTail(probe.GetNext(pos));
                    }
                } else {
                    pos = probe.GetHeadPosition();
                    while (pos != NULL) {
                        PushFreeNode(&g_coordPool, probe.GetNext(pos));
                    }
                }
                probe.RemoveAll();
            }
        }
    commitEntrance:
        m_entrancePx = pixel;
        if (reinit != 0) {
            StepEntranceReinit();
        }
    commitPhase:
        m_arrivalPhase = arrivalPhase;
    returnOne:
        return 1;
    }

    nudged = 0;

    CMapMgr* grid = g_gameReg->m_tileGrid;
    if (grid->m_rows[tile.m_y][tile.m_x].m_typeCode != TILEKIND_GIANT_ROCK) {
        goto nudgeDone;
    }
    free4 = (grid->m_rows[tile.m_y + 1][tile.m_x].m_typeCode == TILEKIND_GIANT_ROCK) ? ROCKADJ_BELOW
                                                                                     : ROCKADJ_NONE;
    free4 |= (grid->m_rows[tile.m_y - 1][tile.m_x].m_typeCode == TILEKIND_GIANT_ROCK)
                 ? ROCKADJ_ABOVE
                 : ROCKADJ_NONE;
    free4 |= (grid->m_rows[tile.m_y][tile.m_x + 1].m_typeCode == TILEKIND_GIANT_ROCK)
                 ? ROCKADJ_RIGHT
                 : ROCKADJ_NONE;
    free4 |= (grid->m_rows[tile.m_y][tile.m_x - 1].m_typeCode == TILEKIND_GIANT_ROCK)
                 ? ROCKADJ_LEFT
                 : ROCKADJ_NONE;
    switch (free4) {
        case ROCKADJ_RIGHT | ROCKADJ_BELOW:
            tile.m_x++;
            tile.m_y++;
            break;
        case ROCKADJ_RIGHT | ROCKADJ_ABOVE:
            tile.m_x++;
            tile.m_y--;
            break;
        case ROCKADJ_RIGHT | ROCKADJ_ABOVE | ROCKADJ_BELOW:
            tile.m_x++;
            break;
        case ROCKADJ_LEFT | ROCKADJ_BELOW:
            tile.m_x--;
            tile.m_y++;
            break;
        case ROCKADJ_LEFT | ROCKADJ_ABOVE:
            tile.m_x--;
            tile.m_y--;
            break;
        case ROCKADJ_LEFT | ROCKADJ_ABOVE | ROCKADJ_BELOW:
            tile.m_x--;
            break;
        case ROCKADJ_LEFT | ROCKADJ_RIGHT | ROCKADJ_BELOW:
            tile.m_y++;
            break;
        case ROCKADJ_LEFT | ROCKADJ_RIGHT | ROCKADJ_ABOVE:
            tile.m_y--;
            break;
        default:
            break;
    }

    for (scan.m_y = tile.m_y - 1; scan.m_y < tile.m_y + 2; scan.m_y++) {
        for (scan.m_x = tile.m_x - 1; scan.m_x < tile.m_x + 2; scan.m_x++) {
            saved[scan.m_x - tile.m_x + 1][scan.m_y - tile.m_y + 1] =
                grid->m_rows[scan.m_y][scan.m_x + 1].m_flags;
            grid->m_rows[scan.m_y][scan.m_x + 1].m_flags = 0;
        }
    }
    grid = g_gameReg->m_tileGrid;
    if (grid->FindPathWithEndpointOverrides(
            lastTile.m_x,
            lastTile.m_y,
            tile.m_x,
            tile.m_y,
            &m_coordList,
            clearEndpointFlags,
            blockedMask,
            passableMask
        ) != 0
        && CoordCount() != 0) {
        PushFreeNode(&g_coordPool, m_coordList.RemoveHead());
        if (CoordCount() != 0) {
            PushFreeNode(&g_coordPool, m_coordList.RemoveTail());
            if (CoordCount() != 0) {
                nudged = 1;
                tail = CoordTail()->m_coord;
                pixel = *tail;
                TileCenter(&pixel);
            }
        }
    }
    for (scan.m_y = tile.m_y - 1; scan.m_y < tile.m_y + 2; scan.m_y++) {
        for (scan.m_x = tile.m_x - 1; scan.m_x < tile.m_x + 2; scan.m_x++) {
            grid->m_rows[scan.m_y][scan.m_x + 1].m_flags =
                saved[scan.m_x - tile.m_x + 1][scan.m_y - tile.m_y + 1];
        }
    }
    if (0 != nudged) {
        if (CoordCount() == 1 && arrivalPhase == IDX(PICKUP_BOOMERANG)
            && m_entranceReason == PICKUP_GAUNTLETZ) {
            m_triggerMgr->UseEquippedToolAt(m_playerIndex, m_unitIndex, pixel.m_x, pixel.m_y);
            SetEntrancePos(1, 1);
            return 1;
        }
        m_arrivalTargetPx = pixel;
    }
nudgeDone:
    if (nudged != 0) {
        goto pathGate;
    }
    if (AI_NONE != m_arrivalState) {
        SetEntrancePos(1, 1);
        return 0;
    }
    if (lastTile == tile) {
        goto reCommit;
    }

    blocked = 0;
    walk = tile;
    delta = tile - lastTile;
    distance = delta.GetAbs();
    if (distance.m_x > distance.m_y) {
        step = (delta.m_y << 16) / distance.m_x;
        CMapMgr* lineGrid = g_gameReg->m_tileGrid;
        acc = lastTile.m_y << 16;
        scan.m_x = lastTile.m_x;
        if (delta.m_x > 0) {
            while (blocked == 0) {
                scan.m_y = acc >> 16;
                err = lineGrid->CellFlagsAt(scan.m_x, scan.m_y);
                if ((blockedMask & err) != 0 && (m_passableMask & err) == 0) {
                    blocked = 1;
                } else {
                    walk = scan;
                }
                acc += step;
                scan.m_x++;
            }
        } else {
            while (blocked == 0) {
                scan.m_y = acc >> 16;
                err = lineGrid->CellFlagsAt(scan.m_x, scan.m_y);
                if ((blockedMask & err) != 0 && (m_passableMask & err) == 0) {
                    blocked = 1;
                } else {
                    walk = scan;
                }
                acc += step;
                scan.m_x--;
            }
        }
    } else {
        step = (delta.m_x << 16) / distance.m_y;
        CMapMgr* lineGrid = g_gameReg->m_tileGrid;
        acc = lastTile.m_x << 16;
        scan.m_y = lastTile.m_y;
        if (delta.m_y > 0) {
            while (blocked == 0) {
                scan.m_x = acc >> 16;
                err = lineGrid->CellFlagsAt(scan.m_x, scan.m_y);
                if ((blockedMask & err) != 0 && (m_passableMask & err) == 0) {
                    blocked = 1;
                } else {
                    walk = scan;
                }
                acc += step;
                scan.m_y++;
            }
        } else {
            while (blocked == 0) {
                scan.m_x = acc >> 16;
                err = lineGrid->CellFlagsAt(scan.m_x, scan.m_y);
                if ((blockedMask & err) != 0 && (m_passableMask & err) == 0) {
                    blocked = 1;
                } else {
                    walk = scan;
                }
                acc += step;
                scan.m_y--;
            }
        }
    }
    if (walk != lastTile) {
        goto reProbe;
    }
reCommit:
    SetEntrancePos(1, 1);
    if (m_arrivalPending == false) {
        return 0;
    }
    m_arrivalPhase = arrivalPhase;
    if (arrivalPhase != 0) {
        goto returnOne;
    }
    goto returnZero;

reProbe:
    pixel = walk;
    TileCenter(&pixel);
    clearEndpointFlags = 1;
    if (g_gameReg->m_tileGrid->FindPathWithEndpointOverrides(
            lastTile.m_x,
            lastTile.m_y,
            walk.m_x,
            walk.m_y,
            &m_coordList,
            clearEndpointFlags,
            blockedMask,
            passableMask
        )
        != 0) {
        if (CoordCount() != 0) {
            PushFreeNode(&g_coordPool, m_coordList.RemoveHead());
        }
        goto pathGate;
    }
    SetEntrancePos(1, 1);
    if (IsGruntAtSavedScreenPos(this) != 0) {
        FaceTowardTile(walk);
    }
    if (m_arrivalPending == false) {
        return 0;
    }
    m_arrivalPhase = arrivalPhase;
    if (arrivalPhase != 0) {
        goto returnOne;
    }
    goto returnZero;
}

#include <Gruntz/GruntCoordRecycleMacros.h>
#include <Gruntz/GruntMovementMacros.h>
#include <Gruntz/SortKeyMacros.h>

RVA(0x0004c170, 0xbe7)
i32 CGrunt::StepGruntMovement() {
    Coord coord;
    Coord currentTile;
    Coord targetPixel;
    Coord targetTile;
    GruntDirectionCell rec;
    i32 flagHead;
    i32 reason12, reason16, reason0e;
    CGruntzMapMgr* bd;

    if (m_entrancePx == m_lastTilePx) {
        goto label_ret1;
    }
    if (m_arrivalState == AI_BATTLEZ_PATH) {
        CBattlezMapConfig* slot = &g_gameReg->m_players[m_playerIndex].m_battlezConfig;
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
        coord = *co;
        CoordPoolNode* p = g_coordPool.NodeOf(co);
        p->m_next = g_coordPool.m_freeHead;
        g_coordPool.m_freeHead = p;
    } else {
        Coord* co = CoordHead()->m_coord;
        coord = *co;
    }

    currentTile = m_object->ScreenPos();
    ScreenTile(&currentTile);
    if (coord.m_x > currentTile.m_x) {
        if (coord.m_y > currentTile.m_y) {
            rec = g_gruntMoveDirSouthEast;
        } else if (coord.m_y == currentTile.m_y) {
            rec = g_gruntMoveDirEast;
        } else {
            rec = g_gruntMoveDirNorthEast;
        }
    } else if (coord.m_x < currentTile.m_x) {
        if (coord.m_y > currentTile.m_y) {
            rec = g_gruntMoveDirSouthWest;
        } else if (coord.m_y == currentTile.m_y) {
            rec = g_gruntMoveDirWest;
        } else {
            rec = g_gruntMoveDirNorthWest;
        }
    } else {
        if (coord.m_y < currentTile.m_y) {
            rec = g_gruntMoveDirNorth;
        } else {
            rec = g_gruntMoveDirSouth;
        }
    }

    targetPixel = coord;
    TileCenter(&targetPixel);
    bd = g_gameReg->m_tileGrid;
    targetTile = targetPixel;
    ScreenTile(&targetTile);
    flagHead = bd->CellFlagsAt(targetTile.m_x, targetTile.m_y);

    {
        EnemyAiType st = m_arrivalState;
        i32 blockMove = 1;
        if (st == AI_OBJECTGUARD) {
            Coord defenderTile = m_defenderPx;
            ScreenTile(&defenderTile);
            if (defenderTile == targetTile) {
                blockMove = 0;
            }
        }
        if (blockMove != 0 && !(flagHead & BRICKZ_CELL_OCCUPIED)) {
            i32 mask = m_arrivalFlags & flagHead;
            if (!(mask & BRICKZ_CELL_OCCUPIED)) {
                if (mask == 0) {
                    goto label_4c6e4;
                }
                if (flagHead & m_passableMask) {
                    goto label_4c6e4;
                }
            }
        }
    }
    if (m_entranceActive == false) {
        Coord lastTile = m_lastTilePx;
        ScreenTile(&lastTile);
        i32 lastFlag = bd->CellFlagsAt(lastTile.m_x, lastTile.m_y);
        if (!(lastFlag & IDX(CELL_FLAG_ARROW))) {
            if (m_arrivalState == AI_BATTLEZ_PATH) {
                goto label_4cb2a;
            }
            if (CoordCount() == 0) {
                goto label_4cb2a;
            }
            {
                i32 mask = m_arrivalFlags & flagHead;
                if (mask & BRICKZ_CELL_OCCUPIED) {
                    goto label_4cb2a;
                }
                if (mask != 0 && !(flagHead & m_passableMask)) {
                    goto label_4cb2a;
                }
            }
            if (!(flagHead & BRICKZ_CELL_OCCUPIED)) {
                goto label_4c6e4;
            }
            {
                Coord* node = NULL;
                CoordPoolNode* head = g_coordPool.m_freeHead;
                if (head->m_next != NULL) {
                    node = &head->m_coord;
                    g_coordPool.m_freeHead = head->m_next;
                }
                *node = targetTile;
                m_coordList.AddHead(node);
            }
            if (PathScan() == 0) {
                SetFacing(0x3e8, rec);
                SetEntrancePos(1, 0);
                return 0;
            }

            if (CoordCount() == 0) {
                goto label_4cb2a;
            }
            {
                Coord* co = CoordHead()->m_coord;
                coord = *co;
                targetPixel = coord;
                TileCenter(&targetPixel);
                currentTile = m_object->ScreenPos();
                ScreenTile(&currentTile);
                if (coord.m_x > currentTile.m_x) {
                    if (coord.m_y > currentTile.m_y) {
                        rec = g_gruntMoveDirSouthEast;
                    } else if (coord.m_y == currentTile.m_y) {
                        rec = g_gruntMoveDirEast;
                    } else {
                        rec = g_gruntMoveDirNorthEast;
                    }
                } else if (coord.m_x < currentTile.m_x) {
                    if (coord.m_y > currentTile.m_y) {
                        rec = g_gruntMoveDirSouthWest;
                    } else if (currentTile.m_y == coord.m_y) {
                        rec = g_gruntMoveDirWest;
                    } else {
                        rec = g_gruntMoveDirNorthWest;
                    }
                } else {
                    if (coord.m_y < currentTile.m_y) {
                        rec = g_gruntMoveDirNorth;
                    } else {
                        rec = g_gruntMoveDirSouth;
                    }
                }
                CGruntzMapMgr* bd = g_gameReg->m_tileGrid;
                if (bd->m_rows[coord.m_y][coord.m_x].m_flags & BRICKZ_CELL_OCCUPIED) {
                    SetFacing(0x3e8, rec);
                    SetEntrancePos(1, 0);
                    return 0;
                }
                Coord* co2 = static_cast<Coord*>(m_coordList.RemoveHead());
                CoordPoolNode* p = g_coordPool.NodeOf(co2);
                p->m_next = g_coordPool.m_freeHead;
                g_coordPool.m_freeHead = p;
                goto label_4c6e4;
            }
        }
    }

    if ((flagHead & BRICKZ_CELL_OCCUPIED) && !(flagHead & IDX(CELL_FLAG_ARROW))) {
        i32 owner;
        if (static_cast<u32>(targetTile.m_x) < static_cast<u32>(bd->m_width)
            && static_cast<u32>(targetTile.m_y) < static_cast<u32>(bd->m_height)) {
            owner = bd->m_rows[targetTile.m_y][targetTile.m_x].m_occupantId;
        } else {
            owner = -1;
        }
        m_triggerMgr->StartUnitDeath(
            (owner >> GRUNT_IDENTITY_PLAYER_SHIFT) & GRUNT_IDENTITY_COMPONENT_MASK,
            owner & GRUNT_IDENTITY_COMPONENT_MASK,
            DEATH_SQUASH,
            m_playerIndex
        );
    }

label_4c6e4:
    if (m_arrivalState == AI_BATTLEZ_PATH && CoordCount() != 0) {
        Coord* co = static_cast<Coord*>(m_coordList.RemoveHead());
        CoordPoolNode* p = g_coordPool.NodeOf(co);
        p->m_next = g_coordPool.m_freeHead;
        g_coordPool.m_freeHead = p;
    }
    if (flagHead & IDX(CELL_FLAG_ARROW)) {
        m_entranceActive = true;
    } else {
        CString* r = g_typeColl.ScratchResolve(m_logicRecord->m_eventCode);
        ActNameConstructGrownSlots();
        bool ne;
        ne = (strcmp(*r, "L") != 0);
        if (ne) {
            m_entranceActive = false;
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

    if (!(flagHead & IDX(CELL_FLAG_SPIKES | CELL_FLAG_WATER_DIAGONAL_PASSAGE))) {
        if (!(flagHead & IDX(CELL_FLAG_SPECIAL))) {
            goto label_4cb4b;
        }
    }
    if (targetPixel == m_entrancePx) {
        if ((flagHead & BRICKZ_BLOCKED_MASK) == 0) {
            goto label_4c92b;
        }
        goto label_4cb2a;
    }
    {
        Coord beyondPixel = targetPixel * 2 - m_lastTilePx;
        Coord beyondTile = beyondPixel;
        ScreenTile(&beyondTile);
        CGruntzMapMgr* bd = g_gameReg->m_tileGrid;
        i32 beyondFlag = bd->CellFlagsAt(beyondTile.m_x, beyondTile.m_y);
        if (beyondFlag
            & (BRICKZ_CELL_OCCUPIED
               | IDX(
                   CELL_FLAG_SOLID | CELL_FLAG_BRIDGE | CELL_FLAG_GRUNT_ENTRANCE_AREA
                   | CELL_FLAG_DESTRUCTIBLE_ROCK | CELL_FLAG_WATER | CELL_FLAG_SINK_HAZARD
               ))) {
            goto label_4cb2a;
        }
        if (CoordCount() != 0 && m_arrivalState != AI_BATTLEZ_PATH) {
            Coord* co = static_cast<Coord*>(m_coordList.RemoveHead());
            if (*co == beyondTile) {
                CoordPoolNode* p = g_coordPool.NodeOf(co);
                p->m_next = g_coordPool.m_freeHead;
                g_coordPool.m_freeHead = p;
            } else {
                m_coordList.AddHead(co);
            }
        }
        Coord hud = m_object->ScreenPos();
        CCueRect* rr = &g_gameReg->m_world->m_level->m_mainPlane->m_planeViewRect;
        if (::PtInRect(rr, hud.m_x, hud.m_y)) {
            g_gameReg->m_voiceManager->PlayGruntVoiceCue(this, 8, -1, -1, -1);
        }
        targetPixel = beyondPixel;
    }

label_4c92b: {
    Coord lastTile = m_lastTilePx;
    ScreenTile(&lastTile);
    targetTile = targetPixel;
    ScreenTile(&targetTile);
    CGruntzMapMgr* bd = g_gameReg->m_tileGrid;
    if (lastTile == targetTile) {
        goto label_4cb4b;
    }
    i32 xbound = bd->m_width;
    if (static_cast<u32>(targetTile.m_x) >= static_cast<u32>(xbound)) {
        goto label_4cb2a;
    }
    if (static_cast<u32>(targetTile.m_y) >= static_cast<u32>(bd->m_height)) {
        goto label_4cb2a;
    }
    BrickzCell** rowtable = bd->m_rows;
    BrickzCell* tgtT = &rowtable[targetTile.m_y][targetTile.m_x];
    i32 tgtFlag = tgtT->m_flags;
    i32 mask = m_arrivalFlags & tgtFlag;
    if (mask & BRICKZ_CELL_OCCUPIED) {
        goto label_4cb2a;
    }
    if (mask != 0 && !(tgtFlag & m_passableMask)) {
        goto label_4cb2a;
    }
    BrickzCell* lastT = &rowtable[lastTile.m_y][lastTile.m_x];
    Coord delta = targetTile - lastTile;
    if (delta.m_x == 0) {
        goto label_4cb4b;
    }
    if (delta.m_y == 0) {
        goto label_4cb4b;
    }

    if (delta.m_x > 0 && delta.m_y > 0) {
        if ((lastT + 1)->m_flags & BRICKZ_CELL_ROUTE_MASKB) {
            goto label_4cb2a;
        }
        if ((lastT + xbound)->m_flags & BRICKZ_CELL_ROUTE_MASKB) {
            goto label_4cb2a;
        }
        if ((tgtT - 1)->m_flags & BRICKZ_CELL_ROUTE_MASKB) {
            goto label_4cb2a;
        }
        if (!((tgtT - xbound)->m_flags & BRICKZ_CELL_ROUTE_MASKB)) {
            goto label_4cb4b;
        }
        goto label_4cb2a;
    } else if (delta.m_x < 0 && delta.m_y > 0) {
        if ((lastT - 1)->m_flags & BRICKZ_CELL_ROUTE_MASKB) {
            goto label_4cb2a;
        }
        if ((lastT + xbound)->m_flags & BRICKZ_CELL_ROUTE_MASKB) {
            goto label_4cb2a;
        }
        if ((tgtT + 1)->m_flags & BRICKZ_CELL_ROUTE_MASKB) {
            goto label_4cb2a;
        }
        if (!((tgtT - xbound)->m_flags & BRICKZ_CELL_ROUTE_MASKB)) {
            goto label_4cb4b;
        }
        goto label_4cb2a;
    } else if (delta.m_x > 0 && delta.m_y < 0) {
        if ((lastT + 1)->m_flags & BRICKZ_CELL_ROUTE_MASKB) {
            goto label_4cb2a;
        }
        if ((lastT - xbound)->m_flags & BRICKZ_CELL_ROUTE_MASKB) {
            goto label_4cb2a;
        }
        if ((tgtT - 1)->m_flags & BRICKZ_CELL_ROUTE_MASKB) {
            goto label_4cb2a;
        }
        if (!((tgtT + xbound)->m_flags & BRICKZ_CELL_ROUTE_MASKB)) {
            goto label_4cb4b;
        }
        goto label_4cb2a;
    } else if (delta.m_x < 0 && delta.m_y < 0) {
        if ((lastT - 1)->m_flags & BRICKZ_CELL_ROUTE_MASKB) {
            goto label_4cb2a;
        }
        if ((lastT - xbound)->m_flags & BRICKZ_CELL_ROUTE_MASKB) {
            goto label_4cb2a;
        }
        if ((tgtT + 1)->m_flags & BRICKZ_CELL_ROUTE_MASKB) {
            goto label_4cb2a;
        }
        if (!((tgtT + xbound)->m_flags & BRICKZ_CELL_ROUTE_MASKB)) {
            goto label_4cb4b;
        }
        goto label_4cb2a;
    }
    goto label_4cb4b;
}

label_4cb2a:
    SetFacing(0x3e8, rec);
    SetEntrancePos(1, 1);
    return 0;

label_4cb4b:
    m_reserved210 = 0;
    m_triggerMgr->ApplySwitch(this, m_lastTilePx.m_x, m_lastTilePx.m_y);
    m_coordRetryCount = 0;
    SetFacing(0x3e8, rec);
    {
        m_commitPx = m_lastTilePx;
        Coord lastTile = m_lastTilePx;
        ScreenTile(&lastTile);
        CGruntzMapMgr* bdl = g_gameReg->m_tileGrid;

        bdl->m_rows[lastTile.m_y][lastTile.m_x].m_flags &= BRICKZ_CELL_UNOCCUPIED_MASK;
        bdl->m_rows[lastTile.m_y][lastTile.m_x].m_occupantId = -1;

        targetTile = targetPixel;
        ScreenTile(&targetTile);
        CGruntzMapMgr* bd2 = g_gameReg->m_tileGrid;
        bd2->m_rows[targetTile.m_y][targetTile.m_x].m_flags |= BRICKZ_CELL_OCCUPIED;
        bd2->m_rows[targetTile.m_y][targetTile.m_x].m_occupantId =
            (m_playerIndex << GRUNT_IDENTITY_PLAYER_SHIFT) | m_unitIndex;

        m_lastTilePx = targetPixel;
        ComputeFacing(1.0);
    }
    m_arrivalPending = true;
    if (reason12) {
        if (flagHead & IDX(CELL_FLAG_WATER)) {
            if (m_coordToggle != false) {
                goto label_ret1;
            }
        } else {
            if (m_coordToggle == false) {
                goto label_ret1;
            }
        }
        RunMoveConfig(targetTile.m_x, targetTile.m_y);
        return 1;
    }
    if (reason16) {
        if (!(flagHead
              & IDX(
                  CELL_FLAG_SPECIAL | CELL_FLAG_WATER | CELL_FLAG_SPIKES | CELL_FLAG_SINK_HAZARD
              ))) {
            goto label_ret1;
        }
        if (m_wingzEnabled != false) {
            goto label_ret1;
        }
        LoadWingzGruntSprites(true);
        return 1;
    }
    if (reason0e) {
        SwitchAnimation(m_poseWalk);
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
void CGrunt::SetEntrancePos(i32 clearArrivalState, i32 recycleRoute) {
    m_reserved210 = 0;
    m_entrancePx = m_lastTilePx;
    if (clearArrivalState) {
        m_arrivalPhase = 0;
        m_arrivalActive = false;
    }
    if (recycleRoute && m_arrivalState != AI_BATTLEZ_PATH && CoordCount() != 0) {
        RECYCLE_GRUNT_COORDS_EXPANDED(this)
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
        m_object->m_screenPosition.m_x,
        m_object->m_screenPosition.m_y - 0x19,
        SORTKEY_GRUNT_HUD,
        "GruntHealthSprite",
        WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
    );
    m_healthSprite->m_logicRecord->m_dispatch(m_healthSprite);

    CLogicRecord* inner = m_healthSprite->m_logicRecord;
    CGruntHealthSprite* reg = static_cast<CGruntHealthSprite*>(inner->m_userLogic);
    if (!reg->BindToGrunt(m_playerIndex, m_unitIndex, m_health)) {
        reg->SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
        m_healthSprite = NULL;
        return 0;
    }
    return 1;
}

// @early-stop
RVA(0x0004d220, 0x9c)
i32 CGrunt::CreateToySprite() {
    if (m_toySprite) {
        return 0;
    }

    m_toySprite = g_gameReg->m_world->m_childGroup->CreateSprite(
        0,
        m_object->m_screenPosition.m_x,
        m_object->m_screenPosition.m_y - 0x19,
        SORTKEY_GRUNT_HUD,
        "GruntToySprite",
        WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
    );
    m_toySprite->m_logicRecord->m_dispatch(m_toySprite);

    CGruntToySprite* reg = static_cast<CGruntToySprite*>(m_toySprite->m_logicRecord->m_userLogic);
    if (!reg->BindToGrunt(m_playerIndex, m_unitIndex)) {
        reg->SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
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
        m_object->m_screenPosition.m_x,
        m_object->m_screenPosition.m_y - 0x20,
        SORTKEY_GRUNT_HUD,
        "GruntStaminaSprite",
        WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
    );
    m_staminaSprite->m_logicRecord->m_dispatch(m_staminaSprite);

    CLogicRecord* inner = m_staminaSprite->m_logicRecord;
    CGruntHealthSprite* reg = static_cast<CGruntHealthSprite*>(inner->m_userLogic);
    if (!reg->BindToGrunt(m_playerIndex, m_unitIndex, m_stamina)) {
        reg->SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
        m_staminaSprite = NULL;
        return 0;
    }
    return 1;
}

#include <Gruntz/GruntPoweredStateMacros.h>
#include <Gruntz/GruntSpriteMacros.h>

// @early-stop
RVA(0x0004d3e0, 0xf5)
i32 CGrunt::CreateToyTimeSprite() {
    if (m_toyTimeSprite || m_toyTime == 0) {
        return 0;
    }

    HIDE_AND_CLEAR_GRUNT_SPRITE(m_staminaSprite)
    HIDE_AND_CLEAR_GRUNT_SPRITE(m_wingzTimeSprite)

    m_toyTimeSprite = g_gameReg->m_world->m_childGroup->CreateSprite(
        0,
        m_object->m_screenPosition.m_x,
        m_object->m_screenPosition.m_y - 0x20,
        SORTKEY_GRUNT_HUD,
        "GruntToyTimeSprite",
        WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
    );
    m_toyTimeSprite->m_logicRecord->m_dispatch(m_toyTimeSprite);

    CLogicRecord* inner = m_toyTimeSprite->m_logicRecord;
    CGruntHealthSprite* reg = static_cast<CGruntHealthSprite*>(inner->m_userLogic);
    if (!reg->BindToGrunt(m_playerIndex, m_unitIndex, m_toyTime)) {
        reg->SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
        m_toyTimeSprite = NULL;
        return 0;
    }
    return 1;
}

// @early-stop
RVA(0x0004d520, 0xe3)
i32 CGrunt::CreateWingzTimeSprite() {
    if (m_wingzTimeSprite || m_wingzEnabled == false || m_wingzTime == 0) {
        return 0;
    }

    HIDE_AND_CLEAR_GRUNT_SPRITE(m_toyTimeSprite)

    m_wingzTimeSprite = g_gameReg->m_world->m_childGroup->CreateSprite(
        0,
        m_object->m_screenPosition.m_x,
        m_object->m_screenPosition.m_y - 0x26,
        SORTKEY_GRUNT_HUD,
        "GruntWingzTimeSprite",
        WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
    );
    m_wingzTimeSprite->m_logicRecord->m_dispatch(m_wingzTimeSprite);

    CLogicRecord* inner = m_wingzTimeSprite->m_logicRecord;
    CGruntHealthSprite* reg = static_cast<CGruntHealthSprite*>(inner->m_userLogic);
    if (!reg->BindToGrunt(m_playerIndex, m_unitIndex, m_wingzTime)) {
        reg->SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
        m_wingzTimeSprite = NULL;
        return 0;
    }
    return 1;
}

// @early-stop
RVA(0x0004d650, 0xa1)
i32 CGrunt::CreatePowerupSprite(i32 powerupId) {
    if (m_powerupSprite) {
        return 0;
    }

    m_powerupSprite = g_gameReg->m_world->m_childGroup->CreateSprite(
        0,
        m_object->m_screenPosition.m_x,
        m_object->m_screenPosition.m_y,
        0x15,
        "GruntPowerupSprite",
        WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
    );
    m_powerupSprite->m_logicRecord->m_dispatch(m_powerupSprite);

    CLogicRecord* inner = m_powerupSprite->m_logicRecord;
    CGruntPowerupSprite* reg = static_cast<CGruntPowerupSprite*>(inner->m_userLogic);
    if (!reg->BindToGrunt(m_playerIndex, m_unitIndex, powerupId)) {
        reg->SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
        m_powerupSprite = NULL;
        return 0;
    }
    return 1;
}

// @early-stop
RVA(0x0004d730, 0x96)
i32 CGrunt::CreateSelectedSprite() {
    if (m_selectedSprite) {
        return 0;
    }

    m_selectedSprite = g_gameReg->m_world->m_childGroup->CreateSprite(
        0,
        m_object->m_screenPosition.m_x,
        m_object->m_screenPosition.m_y,
        0x14,
        "GruntSelectedSprite",
        WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
    );
    m_selectedSprite->m_logicRecord->m_dispatch(m_selectedSprite);

    CGruntSelectedSprite* reg =
        static_cast<CGruntSelectedSprite*>(m_selectedSprite->m_logicRecord->m_userLogic);
    if (!reg->BindToGrunt(m_playerIndex, m_unitIndex)) {
        reg->SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
        m_selectedSprite = NULL;
        return 0;
    }
    return 1;
}

RVA(0x0004d800, 0x440)
i32 CGrunt::Place(
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
) {
    if (kind != AI_NONE) {
        if (kind != AI_BATTLEZ_PATH) {
            m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
        } else {
            m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
        }
    } else {
        m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
        if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
            m_arrivalFlags = ARRIVAL_FLAGS_PLAYER_SINGLE;
        }
    }
    m_arrivalTargetPx.Set(-1, -1);
    m_defenderPx.Set(-1, -1);
    m_powerupDuration = 0;
    m_blockedVoicePending = true;
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
    m_playerIndex = playerIndex;
    m_defenderQueuePosition = defenderQueuePosition;
    m_unitIndex = unitIndex;
    m_arrivalCell.Set(-1, -1);
    m_defenderPickupType = static_cast<PickupType>(defenderPickupType);
    m_defenderRadius = defenderRadiusMinusOne + 1;
    m_arrivalRerollLo = 0;
    m_arrivalRerollWindowLo = 0;
    m_arrivalRerollHi = 0;
    m_arrivalRerollWindowHi = 0;
    m_holdAnchorLo = 0;
    m_holdWindowLo = 0;
    m_holdAnchorHi = 0;
    m_holdWindowHi = 0;
    m_moveIcon = moveIcon;
    m_triggerMgr = board;
    m_daFlag = 1;
    m_arrivalPhase = 0;
    m_toolConfigured = true;
    m_tileClaimed = false;
    m_neighborScanEnabled = true;
    m_tileMoveCommitted = false;
    m_entranceArmed = false;
    m_entranceDropActive = false;
    m_deathType = DEATH_NONE;
    m_pendingTrigger = false;
    m_cellRemovalNotified = false;
    m_killerPlayerIndex = -1;
    m_passableMask = 0;
    m_savedMoveIcon = -1;
    m_lowStaminaCued = false;
    m_targetTeam = -1;
    LoadVehicleGruntSprites(static_cast<PickupType>(vehicleKind));
    LoadGruntTypeTable(typeKind, 1, 0, 0);
    if (span != NULL) {
        Coord tile = m_lastTilePx;
        ScreenTile(&tile);
        m_object->m_extent = MakeRect(
            tile.m_x - span->left,
            tile.m_y - span->top,
            tile.m_x + span->right,
            tile.m_y + span->bottom
        );
    }
    CRect reach = m_object->m_extent;
    if (reach.Width() == 0 && reach.Height() == 0) {
        m_hasExtent = false;
    } else {
        m_hasExtent = true;
    }
    if (m_moveIcon < PICKUP_NONE || m_moveIcon >= PICKUP_MOVEICON_END) {
        m_moveIcon = PICKUP_NONE;
    }
    CShadeTable* shade = g_gameReg->m_spriteFactory->GetSel(IDX(m_moveIcon), 0);
    if (shade == NULL) {
        shade = g_gameReg->m_spriteFactory->GetSel(1, 0);
    }
    SET_DRAW_FILL_ARG_FIRST(m_object, SHADE_PAL_16, shade);
    if (entranceMode != GRUNT_ENTRANCE_NONE) {
        BuildEntranceAnimation(entranceMode);
        return 1;
    }

    CGruntzMapMgr* plane = g_gameReg->m_tileGrid;
    Coord tile = m_lastTilePx;
    ScreenTile(&tile);
    plane->m_rows[tile.m_y][tile.m_x].m_flags |= BRICKZ_CELL_OCCUPIED;
    plane->m_rows[tile.m_y][tile.m_x].m_occupantId =
        (m_playerIndex << GRUNT_IDENTITY_PLAYER_SHIFT) | m_unitIndex;
    m_entranceActive = false;
    ReadConfigFromButeMgr();
    LoadCellAnimNames(0, 0);
    LoadAnimNameTable(0, 0);
    ResetEntranceAnimation(1, 0, 0);
    switch (kind) {
        case AI_POSTGUARD:
            m_defenderPx = m_lastTilePx;
            break;
        case AI_OBJECTGUARD:
            if (defenderQueuePosition == 0 && defenderPickupType == 0) {
                m_defenderPx = m_lastTilePx;
                m_arrivalState = AI_POSTGUARD;
            } else {
                Coord defenderTile(defenderQueuePosition, defenderPickupType);
                TileCenter(&defenderTile);
                m_defenderPx = defenderTile;
                StepArrivalDrop(defenderTile.m_x, defenderTile.m_y - 0x20, 0, -1, 1, 0);
            }
            break;
        case AI_DEFENDER:
        case AI_BOMBER:
            m_defenderPx = m_lastTilePx;
            break;
    }
    return 1;
}

// @early-stop
RVA(0x0004dd50, 0x2400)
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
        if (m_entranceActive != false) {
            goto fail;
        }
        eq = (strcmp((*g_typeColl.GetNameRecord(m_logicRecord->m_eventCode)), "A") != 0);
        if (eq) {
            eq = (strcmp((*g_typeColl.GetNameRecord(m_logicRecord->m_eventCode)), "D") != 0);
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
    if (m_coordToggle != false) {
        goto fail;
    }
    if (kind != PICKUP_WINGZ) {
        m_wingzEnabled = false;
        m_wingzDurationLo = 0;
        m_wingzDurationHi = 0;
        HIDE_AND_CLEAR_GRUNT_SPRITE(m_wingzTimeSprite)
    }
    fresh = 0;
    defer = 0;
    if (m_entranceReason < PICKUP_EQUIPPABLE_END) {
        m_toolId = m_entranceReason;
    }
    switch (kind) {
        case PICKUP_NONE: {
            m_animSetName = "NORMALGRUNT";
            i32 r = g_buteMgr.GetInt(m_animSetName, "ToolAA", 1);
            m_reachRect = MakeRect(-r, -r, r, r);
            m_reachExclusionRect = MakeRect(0, 0, 0, 0);
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
                m_arrivalFlags |= IDX(CELL_FLAG_GRUNT_ENTRANCE_AREA);
            }
            m_passableMask = 0;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_BOMB: {
            m_animSetName = "BOMBGRUNT";
            i32 r = g_buteMgr.GetInt(m_animSetName, "ToolAA", 1);
            m_reachRect = MakeRect(-r, -r, r, r);
            m_reachExclusionRect = MakeRect(0, 0, 0, 0);
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
                m_arrivalFlags |= IDX(CELL_FLAG_GRUNT_ENTRANCE_AREA);
            }
            m_passableMask = 0;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_BOOMERANG: {
            m_animSetName = "BOOMERANGGRUNT";
            i32 r = g_buteMgr.GetInt(m_animSetName, "ToolAA", 1);
            m_reachRect = MakeRect(-r, -r, r, r);
            m_reachExclusionRect = MakeRect(0, 0, 0, 0);
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
            if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
                m_arrivalFlags |= IDX(CELL_FLAG_GRUNT_ENTRANCE_AREA);
            }
            m_passableMask = 0;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_BRICK: {
            m_animSetName = "BRICKGRUNT";
            i32 r = g_buteMgr.GetInt(m_animSetName, "ToolAA", 1);
            m_reachRect = MakeRect(-r, -r, r, r);
            m_reachExclusionRect = MakeRect(0, 0, 0, 0);
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
                m_arrivalFlags |= IDX(CELL_FLAG_GRUNT_ENTRANCE_AREA);
            }
            m_passableMask = 0;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_CLUB: {
            m_animSetName = "CLUBGRUNT";
            i32 r = g_buteMgr.GetInt(m_animSetName, "ToolAA", 1);
            m_reachRect = MakeRect(-r, -r, r, r);
            m_reachExclusionRect = MakeRect(0, 0, 0, 0);
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
                m_arrivalFlags |= IDX(CELL_FLAG_GRUNT_ENTRANCE_AREA);
            }
            m_passableMask = 0;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_GAUNTLETZ: {
            m_animSetName = "GAUNTLETZGRUNT";
            i32 r = g_buteMgr.GetInt(m_animSetName, "ToolAA", 1);
            m_reachRect = MakeRect(-r, -r, r, r);
            m_reachExclusionRect = MakeRect(0, 0, 0, 0);
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
                m_arrivalFlags |= IDX(CELL_FLAG_GRUNT_ENTRANCE_AREA);
            }
            m_passableMask = 0;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_GLOVEZ: {
            m_animSetName = "GLOVEZGRUNT";
            i32 r = g_buteMgr.GetInt(m_animSetName, "ToolAA", 1);
            m_reachRect = MakeRect(-r, -r, r, r);
            m_reachExclusionRect = MakeRect(0, 0, 0, 0);
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
                m_arrivalFlags |= IDX(CELL_FLAG_GRUNT_ENTRANCE_AREA);
            }
            m_passableMask = 0;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_GOOBER: {
            m_animSetName = "GOOBERGRUNT";
            i32 r = g_buteMgr.GetInt(m_animSetName, "ToolAA", 1);
            m_reachRect = MakeRect(-r, -r, r, r);
            m_reachExclusionRect = MakeRect(0, 0, 0, 0);
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
                m_arrivalFlags |= IDX(CELL_FLAG_GRUNT_ENTRANCE_AREA);
            }
            m_passableMask = 0;
            m_toolConfigured = true;
            if (m_arrivalState == AI_BATTLEZ_PATH) {
                if (m_battleState != BZTASK_ADVANCE) {
                    if (this->CoordCount() != 0) {
                        RECYCLE_GRUNT_COORDS(this)
                    }
                    for (;;) {
                        i32* h;
                        if (m_payloads.GetCount() != 0) {
                            h = static_cast<i32*>(m_payloads.GetHead());
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
                    i32* mem = new i32[0xb];
                    i32* payload;
                    if (mem != NULL) {
                        memset(mem, 0, 0x2c);
                        payload = mem;
                    } else {
                        payload = NULL;
                    }
                    payload[0] = 9;
                    m_payloads.AddHead(payload);
                }
            }
            break;
        }
        case PICKUP_GRAVITYBOOTZ: {
            m_animSetName = "GRAVITYBOOTZGRUNT";
            i32 r = g_buteMgr.GetInt(m_animSetName, "ToolAA", 1);
            m_reachRect = MakeRect(-r, -r, r, r);
            m_reachExclusionRect = MakeRect(0, 0, 0, 0);
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
                m_arrivalFlags |= IDX(CELL_FLAG_GRUNT_ENTRANCE_AREA);
            }
            m_passableMask = IDX(CELL_FLAG_SPIKES);
            m_toolConfigured = true;
            break;
        }
        case PICKUP_GUNHAT: {
            m_animSetName = "GUNHATGRUNT";
            i32 r = g_buteMgr.GetInt(m_animSetName, "ToolAA", 1);
            m_reachRect = MakeRect(-r, -r, r, r);
            m_reachExclusionRect = MakeRect(0, 0, 0, 0);
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
            if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
                m_arrivalFlags |= IDX(CELL_FLAG_GRUNT_ENTRANCE_AREA);
            }
            m_passableMask = 0;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_NERFGUN: {
            m_animSetName = "NERFGUNGRUNT";
            i32 r = g_buteMgr.GetInt(m_animSetName, "ToolAA", 1);
            m_reachRect = MakeRect(-r, -r, r, r);
            m_reachExclusionRect = MakeRect(0, 0, 0, 0);
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
                m_arrivalFlags |= IDX(CELL_FLAG_GRUNT_ENTRANCE_AREA);
            }
            if (m_arrivalState == AI_DEFENDER) {
                m_defenderRadius = 1;
            }
            m_passableMask = 0;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_ROCK: {
            m_animSetName = "ROCKGRUNT";
            i32 r = g_buteMgr.GetInt(m_animSetName, "ToolAA", 1);
            m_reachRect = MakeRect(-r, -r, r, r);
            m_reachExclusionRect = MakeRect(0, 0, 0, 0);
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
                m_arrivalFlags |= IDX(CELL_FLAG_GRUNT_ENTRANCE_AREA);
            }
            if (m_arrivalState == AI_DEFENDER) {
                m_defenderRadius = 1;
            }
            m_passableMask = 0;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_SHIELD: {
            m_animSetName = "SHIELDGRUNT";
            i32 r = g_buteMgr.GetInt(m_animSetName, "ToolAA", 1);
            m_reachRect = MakeRect(-r, -r, r, r);
            m_reachExclusionRect = MakeRect(0, 0, 0, 0);
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
                m_arrivalFlags |= IDX(CELL_FLAG_GRUNT_ENTRANCE_AREA);
            }
            m_passableMask = 0;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_SHOVEL: {
            m_animSetName = "SHOVELGRUNT";
            i32 r = g_buteMgr.GetInt(m_animSetName, "ToolAA", 1);
            m_reachRect = MakeRect(-r, -r, r, r);
            m_reachExclusionRect = MakeRect(0, 0, 0, 0);
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
                m_arrivalFlags |= IDX(CELL_FLAG_GRUNT_ENTRANCE_AREA);
            }
            m_passableMask = 0;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_SPRING: {
            m_animSetName = "SPRINGGRUNT";
            i32 r = g_buteMgr.GetInt(m_animSetName, "ToolAA", 1);
            m_reachRect = MakeRect(-r, -r, r, r);
            m_reachExclusionRect = MakeRect(0, 0, 0, 0);
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
                m_arrivalFlags |= IDX(CELL_FLAG_GRUNT_ENTRANCE_AREA);
            }
            m_passableMask = IDX(CELL_FLAG_WATER_DIAGONAL_PASSAGE);
            m_toolConfigured = true;
            break;
        }
        case PICKUP_SPY: {
            m_animSetName = "SPYGRUNT";
            i32 r = g_buteMgr.GetInt(m_animSetName, "ToolAA", 1);
            m_reachRect = MakeRect(-r, -r, r, r);
            m_reachExclusionRect = MakeRect(0, 0, 0, 0);
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
                m_arrivalFlags |= IDX(CELL_FLAG_GRUNT_ENTRANCE_AREA);
            }
            m_passableMask = 0;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_SWORD: {
            m_animSetName = "SWORDGRUNT";
            i32 r = g_buteMgr.GetInt(m_animSetName, "ToolAA", 1);
            m_reachRect = MakeRect(-r, -r, r, r);
            m_reachExclusionRect = MakeRect(0, 0, 0, 0);
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
                m_arrivalFlags |= IDX(CELL_FLAG_GRUNT_ENTRANCE_AREA);
            }
            m_passableMask = 0;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_TIMEBOMB: {
            m_animSetName = "TIMEBOMBGRUNT";
            i32 r = g_buteMgr.GetInt(m_animSetName, "ToolAA", 1);
            m_reachRect = MakeRect(-r, -r, r, r);
            m_reachExclusionRect = MakeRect(0, 0, 0, 0);
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
                m_arrivalFlags |= IDX(CELL_FLAG_GRUNT_ENTRANCE_AREA);
            }
            m_passableMask = 0;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_TOOB: {
            m_animSetName = "TOOBGRUNT";
            i32 r = g_buteMgr.GetInt(m_animSetName, "ToolAA", 1);
            m_reachRect = MakeRect(-r, -r, r, r);
            m_coordToggle = false;
            m_reachExclusionRect = MakeRect(0, 0, 0, 0);
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
                m_arrivalFlags |= IDX(CELL_FLAG_GRUNT_ENTRANCE_AREA);
            }
            m_passableMask = IDX(CELL_FLAG_WATER);
            m_toolConfigured = true;
            break;
        }
        case PICKUP_WAND: {
            m_animSetName = "WANDGRUNT";
            i32 r = g_buteMgr.GetInt(m_animSetName, "ToolAA", 1);
            m_reachRect = MakeRect(-r, -r, r, r);
            m_reachExclusionRect = MakeRect(0, 0, 0, 0);
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
                m_arrivalFlags |= IDX(CELL_FLAG_GRUNT_ENTRANCE_AREA);
            }
            m_passableMask = 0;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_WARPSTONE: {
            m_animSetName = "WARPSTONEGRUNT";
            i32 r = g_buteMgr.GetInt(m_animSetName, "ToolAA", 1);
            m_reachRect = MakeRect(-r, -r, r, r);
            m_reachExclusionRect = MakeRect(0, 0, 0, 0);
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            m_passableMask = 0;
            m_toolConfigured = false;
            break;
        }
        case PICKUP_WELDER: {
            m_animSetName = "WELDERGRUNT";
            i32 r = g_buteMgr.GetInt(m_animSetName, "ToolAA", 1);
            m_reachRect = MakeRect(-r, -r, r, r);
            m_reachExclusionRect = MakeRect(0, 0, 0, 0);
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
                m_arrivalFlags |= IDX(CELL_FLAG_GRUNT_ENTRANCE_AREA);
            }
            if (m_arrivalState == AI_DEFENDER) {
                m_defenderRadius = 1;
            }
            m_passableMask = 0;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_WINGZ: {
            m_animSetName = "WINGZGRUNT";
            i32 r = g_buteMgr.GetInt(m_animSetName, "ToolAA", 1);
            m_reachRect = MakeRect(-r, -r, r, r);
            m_reachExclusionRect = MakeRect(0, 0, 0, 0);
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
                m_arrivalFlags |= IDX(CELL_FLAG_GRUNT_ENTRANCE_AREA);
            }
            if (m_arrivalState == AI_DEFENDER) {
                m_defenderRadius = 1;
            }
            m_passableMask =
                IDX(CELL_FLAG_SPECIAL | CELL_FLAG_WATER | CELL_FLAG_SPIKES | CELL_FLAG_SINK_HAZARD);
            m_wingzEnabled = false;
            m_wingzTime = 0x64;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_BABYWALKER: {
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
                m_arrivalFlags |= IDX(CELL_FLAG_GRUNT_ENTRANCE_AREA);
            }
            m_passableMask = 0;
            m_animSetName = "BABYWALKERGRUNT";
            CString* rec = g_typeColl.ScratchResolve(m_logicRecord->m_eventCode);
            ActNameConstructGrownSlots();
            eq = (strcmp(*rec, "D") == 0);
            if (eq) {
                ConsiderArrival(0);
            }
            fresh = 1;
            defer = 1;
            break;
        }
        case PICKUP_BEACHBALL: {
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
                m_arrivalFlags |= IDX(CELL_FLAG_GRUNT_ENTRANCE_AREA);
            }
            m_passableMask = 0;
            m_animSetName = "BEACHBALLGRUNT";
            CString* rec = g_typeColl.ScratchResolve(m_logicRecord->m_eventCode);
            ActNameConstructGrownSlots();
            eq = (strcmp(*rec, "D") == 0);
            if (eq) {
                ConsiderArrival(0);
            }
            fresh = 1;
            break;
        }
        case PICKUP_BIGWHEEL: {
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
                m_arrivalFlags |= IDX(CELL_FLAG_GRUNT_ENTRANCE_AREA);
            }
            m_passableMask = 0;
            m_animSetName = "BIGWHEELGRUNT";
            CString* rec = g_typeColl.ScratchResolve(m_logicRecord->m_eventCode);
            ActNameConstructGrownSlots();
            eq = (strcmp(*rec, "D") == 0);
            if (eq) {
                ConsiderArrival(0);
            }
            fresh = 1;
            defer = 1;
            break;
        }
        case PICKUP_GOKART: {
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
                m_arrivalFlags |= IDX(CELL_FLAG_GRUNT_ENTRANCE_AREA);
            }
            m_passableMask = 0;
            m_animSetName = "GOKARTGRUNT";
            CString* rec = g_typeColl.ScratchResolve(m_logicRecord->m_eventCode);
            ActNameConstructGrownSlots();
            eq = (strcmp(*rec, "D") == 0);
            if (eq) {
                ConsiderArrival(0);
            }
            fresh = 1;
            defer = 1;
            break;
        }
        case PICKUP_JACKINTHEBOX: {
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
                m_arrivalFlags |= IDX(CELL_FLAG_GRUNT_ENTRANCE_AREA);
            }
            m_passableMask = 0;
            m_animSetName = "JACKINTHEBOXGRUNT";
            CString* rec = g_typeColl.ScratchResolve(m_logicRecord->m_eventCode);
            ActNameConstructGrownSlots();
            eq = (strcmp(*rec, "D") == 0);
            if (eq) {
                ConsiderArrival(0);
            }
            fresh = 1;
            break;
        }
        case PICKUP_JUMPROPE: {
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
                m_arrivalFlags |= IDX(CELL_FLAG_GRUNT_ENTRANCE_AREA);
            }
            m_passableMask = 0;
            m_animSetName = "JUMPROPEGRUNT";
            CString* rec = g_typeColl.ScratchResolve(m_logicRecord->m_eventCode);
            ActNameConstructGrownSlots();
            eq = (strcmp(*rec, "D") == 0);
            if (eq) {
                ConsiderArrival(0);
            }
            fresh = 1;
            break;
        }
        case PICKUP_POGOSTICK: {
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
                m_arrivalFlags |= IDX(CELL_FLAG_GRUNT_ENTRANCE_AREA);
            }
            m_passableMask = 0;
            m_animSetName = "POGOSTICKGRUNT";
            CString* rec = g_typeColl.ScratchResolve(m_logicRecord->m_eventCode);
            ActNameConstructGrownSlots();
            eq = (strcmp(*rec, "D") == 0);
            if (eq) {
                ConsiderArrival(0);
            }
            fresh = 1;
            defer = 1;
            break;
        }
        case PICKUP_SCROLL: {
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
                m_arrivalFlags |= IDX(CELL_FLAG_GRUNT_ENTRANCE_AREA);
            }
            m_moveVariant = variant;
            m_passableMask = 0;
            m_animSetName = "SCROLLGRUNT";
            CString* rec = g_typeColl.ScratchResolve(m_logicRecord->m_eventCode);
            ActNameConstructGrownSlots();
            eq = (strcmp(*rec, "D") == 0);
            if (eq) {
                ConsiderArrival(0);
            }
            fresh = 1;
            break;
        }
        case PICKUP_SQUEAKTOY: {
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
                m_arrivalFlags |= IDX(CELL_FLAG_GRUNT_ENTRANCE_AREA);
            }
            m_passableMask = 0;
            m_animSetName = "SQUEAKTOYGRUNT";
            CString* rec = g_typeColl.ScratchResolve(m_logicRecord->m_eventCode);
            ActNameConstructGrownSlots();
            eq = (strcmp(*rec, "D") == 0);
            if (eq) {
                ConsiderArrival(0);
            }
            fresh = 1;
            break;
        }
        case PICKUP_YOYO: {
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
                m_arrivalFlags |= IDX(CELL_FLAG_GRUNT_ENTRANCE_AREA);
            }
            m_passableMask = 0;
            m_animSetName = "YOYOGRUNT";
            CString* rec = g_typeColl.ScratchResolve(m_logicRecord->m_eventCode);
            ActNameConstructGrownSlots();
            eq = (strcmp(*rec, "D") == 0);
            if (eq) {
                ConsiderArrival(0);
            }
            fresh = 1;
            break;
        }
        case PICKUP_HEALTH1: {
            i32 h = g_buteMgr.GetInt("Powerupz", "Health1", 0x19) + m_health;
            if (h >= HEALTH_FULL) {
                h = HEALTH_FULL;
            }
            m_health = h;
            return 1;
        }
        case PICKUP_HEALTH2: {
            i32 h = g_buteMgr.GetInt("Powerupz", "Health2", 0x19) + m_health;
            if (h >= HEALTH_FULL) {
                h = HEALTH_FULL;
            }
            m_health = h;
            return 1;
        }
        case PICKUP_HEALTH3: {
            i32 h = g_buteMgr.GetInt("Powerupz", "Health3", 0x19) + m_health;
            if (h >= HEALTH_FULL) {
                h = HEALTH_FULL;
            }
            m_health = h;
            return 1;
        }
        case PICKUP_CONVERSION: {
            m_toolId = m_entranceReason;
            m_reachRect = MakeRect(-1, -1, 1, 1);
            m_reachExclusionRect = MakeRect(0, 0, 0, 0);
            fresh = 0;
            m_animSetName = "HAREKRISHNAGRUNT";
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
                m_arrivalFlags |= IDX(CELL_FLAG_GRUNT_ENTRANCE_AREA);
            }
            m_passableMask = 0;
            m_gruntKind = GRUNT_CONVERSION;
            m_convertTimeLo = g_buteMgr.GetDword("Powerupz", "ConversionTime", 0x1f4);
            m_convertTimeHi = 0;
            m_convertClockLo = g_frameTime;
            m_convertClockHi = 0;
            StopPowerupLoopSound();
            EnsurePowerupLoopSound("GAME_CONVERSIONLOOP");
            break;
        }
        case PICKUP_DEATHTOUCH: {
            m_toolId = m_entranceReason;
            m_reachRect = MakeRect(-1, -1, 1, 1);
            m_reachExclusionRect = MakeRect(0, 0, 0, 0);
            fresh = 0;
            m_animSetName = "REAPERGRUNT";
            if (m_arrivalState == AI_NONE) {
                m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
            } else if (m_arrivalState == AI_BATTLEZ_PATH) {
                m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
            } else {
                m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
            }
            if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
                m_arrivalFlags |= IDX(CELL_FLAG_GRUNT_ENTRANCE_AREA);
            }
            m_passableMask = 0;
            m_gruntKind = GRUNT_DEATHTOUCH;
            if (m_powerupDuration == 0) {
                m_powerupDuration = g_buteMgr.GetDword("Powerupz", "DeathTouchTime", 0x4e20);
            }
            m_convertTimeLo = m_powerupDuration;
            m_convertTimeHi = 0;
            m_convertClockLo = g_frameTime;
            m_convertClockHi = 0;
            m_shimmerWindowLo = 0;
            m_shimmerWindowHi = 0;
            StopPowerupLoopSound();
            EnsurePowerupLoopSound("GAME_DEATHTOUCHLOOP");
            break;
        }
        case PICKUP_GHOST: {
            m_gruntKind = GRUNT_GHOST;
            i32 t = g_buteMgr.GetInt("Powerupz", "GruntGhostTransparencyOn", 0xe0);
            SET_DRAW_FILL_FRACTION(m_object, SHADE_PAL_ALPHA_16, t);
            if (m_powerupDuration == 0) {
                m_powerupDuration = g_buteMgr.GetDword("Powerupz", "GhostTime", 0x4e20);
            }
            m_convertTimeLo = m_powerupDuration;
            m_convertTimeHi = 0;
            m_convertClockLo = g_frameTime;
            m_convertClockHi = 0;
            m_shimmerWindowLo = 0;
            m_shimmerWindowHi = 0;
            StopPowerupLoopSound();
            EnsurePowerupLoopSound("GAME_GHOSTLOOP");
            return 1;
        }
        case PICKUP_INVULNERABILITY: {
            m_gruntKind = GRUNT_INVULNERABLE;
            if (m_powerupDuration == 0) {
                m_powerupDuration = g_buteMgr.GetDword("Powerupz", "InvulnerabilityTime", 0x4e20);
            }
            m_convertTimeLo = m_powerupDuration;
            m_convertTimeHi = 0;
            m_convertClockLo = g_frameTime;
            m_convertClockHi = 0;
            m_shimmerWindowLo = 0;
            m_shimmerWindowHi = 0;
            StopPowerupLoopSound();
            EnsurePowerupLoopSound("GAME_INVULNERABILITYLOOP");
            return 1;
        }
        case PICKUP_REACTIVEARMOR: {
            m_gruntKind = GRUNT_REACTIVEARMOR;
            CreatePowerupSprite(3);
            if (m_powerupDuration == 0) {
                m_powerupDuration = g_buteMgr.GetDword("Powerupz", "ReactiveArmorTime", 0x4e20);
            }
            m_convertTimeLo = m_powerupDuration;
            m_convertTimeHi = 0;
            m_convertClockLo = g_frameTime;
            m_convertClockHi = 0;
            m_shimmerWindowLo = 0;
            m_shimmerWindowHi = 0;
            StopPowerupLoopSound();
            EnsurePowerupLoopSound("GAME_REACTIVEARMORLOOP");
            return 1;
        }
        case PICKUP_ROIDZ: {
            m_gruntKind = GRUNT_ROIDZ;
            CreatePowerupSprite(1);
            if (m_powerupDuration == 0) {
                m_powerupDuration = g_buteMgr.GetDword("Powerupz", "RoidzTime", 0x4e20);
            }
            m_convertTimeLo = m_powerupDuration;
            m_convertTimeHi = 0;
            m_convertClockLo = g_frameTime;
            m_convertClockHi = 0;
            m_shimmerWindowLo = 0;
            m_shimmerWindowHi = 0;
            StopPowerupLoopSound();
            EnsurePowerupLoopSound("GAME_ROIDZLOOP");
            return 1;
        }
        case PICKUP_SUPERSPEED: {
            m_gruntKind = GRUNT_SUPERSPEED;
            CreatePowerupSprite(2);
            if (m_powerupDuration == 0) {
                m_powerupDuration = g_buteMgr.GetDword("Powerupz", "SuperSpeedTime", 0x4e20);
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
            StopPowerupLoopSound();
            EnsurePowerupLoopSound("GAME_SUPERSPEEDLOOP");
            return 1;
        }
        case PICKUP_MEGAPHONE: {
            CPlay* play = static_cast<CPlay*>(g_gameReg->m_curState);
            CStatusBarMgr* sb = play->m_statusBar;
            if (sb->m_hlBusy == false) {
                if (sb->m_position == STATUSBAR_HIDDEN) {
                    sb->RestoreStatusBar();
                }
                if (sb->m_activeTab != TAB_RESOURCE) {
                    sb->SetTabState(SBICMD_TAB_RESOURCE, MENUITEM_SELECTED);
                }
                sb->Deactivate();
            }
            play->m_statusBar->UpdateRezMachineWakeStatusBar();
            return 1;
        }
        case PICKUP_RANDOMCOLORZ: {
            m_triggerMgr->CycleMoveIcons(m_playerIndex, true);
            return 1;
        }
        case PICKUP_SCREENSHAKE: {
            if (m_playerIndex == g_curPlayer) {
                return 1;
            }
            (static_cast<CPlay*>(g_gameReg->m_curState))->SetMonitorCurse(true);
            return 1;
        }
        case PICKUP_BLACKSCREEN: {
            if (m_playerIndex == g_curPlayer) {
                return 1;
            }
            (static_cast<CPlay*>(g_gameReg->m_curState))->SetDarknessCurse(true);
            return 1;
        }
        case PICKUP_MINICAM: {
            if (m_playerIndex == g_curPlayer) {
                return 1;
            }
            (static_cast<CPlay*>(g_gameReg->m_curState))->SetTinyViewportCurse(true);
            return 1;
        }
        case PICKUP_W:
        case PICKUP_A:
        case PICKUP_R:
        case PICKUP_P: {
            g_gameReg->m_gameStats->m_warpLetterFound = true;
            return 1;
        }
        case PICKUP_HELPBOX: {
            (static_cast<CPlay*>(g_gameReg->m_curState))->PostActionCue(m_helpCueId);
            return 1;
        }
        case PICKUP_COIN: {
            g_gameReg->m_gameStats->m_coinsCollected++;
            return 1;
        }
        case PICKUP_STOPWATCH: {
            CPlay* play = static_cast<CPlay*>(g_gameReg->m_curState);
            if (play->m_levelTimer == NULL) {
                return 1;
            }
            i32 mins = g_buteMgr.GetInt("Powerupz", "StopwatchMinutes", 1);
            i32 secs = g_buteMgr.GetInt("Powerupz", "StopwatchSeconds", 0);
            if (g_gameReg->m_isEasyMode != false && g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
                secs += secs;
                mins += mins;
                if (secs > 0x3b) {
                    mins++;
                    secs -= 0x3c;
                }
            }
            play->m_levelTimer->AddTime(mins, secs);
            return 1;
        }
        default: {
            m_reachRect = MakeRect(-1, -1, 1, 1);
            m_reachExclusionRect = MakeRect(0, 0, 0, 0);
            fresh = 0;
            m_animSetName = "NORMALGRUNT";
            break;
        }
    }

    {
        CPlay* play = static_cast<CPlay*>(g_gameReg->m_curState);
        if (kind == PICKUP_TOOB) {
            play->BuildGruntTypeNameTable(PICKUP_TOOB, 1, 1, NULL);
        } else {
            play->BuildAssetNamespacePrefixes(m_animSetName, 1, 1, NULL);
        }
    }
    m_entranceReason = kind;
    ReadConfigFromButeMgr();
    LoadCellAnimNames(fresh, defer);
    LoadAnimNameTable(fresh, defer);
    if (fresh == 0) {
        CString* rec;
        {
            i32 key = m_logicRecord->EventCode();
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
            ActNameConstructGrownSlots();
        }

        eq = (strcmp(*rec, "H") == 0);
        if (eq) {
            CAniElement* el = m_wwdObject->m_animationCursor.m_animation;
            CAniRecordView* first;
            if (el->m_records.GetSize() > 0) {
                first = static_cast<CAniRecordView*>(el->m_records[0]);
            } else {
                first = NULL;
            }
            i32 handle = first->m_param;
            SetImageFrameByName(EntranceCell()->m_names[1].GetBuffer(0), handle);
        } else {
            if (m_poweredUp != false && m_neighborValid == false) {
                RESET_GRUNT_POWERED_STATE(this)
            }
            CString* rec2;
            {
                i32 key2 = m_logicRecord->EventCode();
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
                ActNameConstructGrownSlots();
            }

            eq = (strcmp(*rec2, "D") == 0);
            if (eq) {
                SetImageSetByName(EntranceCell()->m_names[2].GetBuffer(0));
                SwitchAnimation(m_poseWalk);
            } else {
                ResetEntranceAnimation(1, 0, 0);
                if (m_arrivalPending == false) {
                    m_triggerMgr->WireTileSwitchLogic(this, m_lastTilePx.m_x, m_lastTilePx.m_y);
                }
            }
        }
        Coord tile = m_lastTilePx;
        ScreenTile(&tile);
        TileCollisionKind tk = g_gameReg->m_tileGrid->m_rows[tile.m_y][tile.m_x].m_typeCode;
        if (tk == TILEKIND_CHECKPOINT || tk == TILEKIND_CHECKPOINT_UP) {
            if (GRUNT_AT_SAVED_SCREEN_POS(this)) {
                m_triggerMgr->ApplySwitch(this, m_lastTilePx.m_x, m_lastTilePx.m_y);
                m_triggerMgr->WireTileSwitchLogic(this, m_lastTilePx.m_x, m_lastTilePx.m_y);
            }
        }
    } else {
        UpdateArrival(defer, 1);
    }
    if (m_arrived != false) {
        if (m_playerIndex == g_curPlayer) {
            m_triggerMgr->StopPendingFx();
        }
    }
    if (kind == PICKUP_WARPSTONE) {
        m_triggerMgr->ReinitGroup(m_object->m_screenPosition.m_x, m_object->m_screenPosition.m_y);
    }
    return 1;
fail:
    return 0;
}
