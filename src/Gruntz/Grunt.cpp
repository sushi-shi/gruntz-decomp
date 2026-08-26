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
#include <Gruntz/GruntCoordRecycleMacros.h>
#include <Gruntz/GruntDeathType.h>
#include <Gruntz/GruntEntranceArrival.h>
#include <Gruntz/GruntEntranceMove.h>
#include <Gruntz/GruntHealthSprite.h>
#include <Gruntz/GruntIdentity.h>
#include <Gruntz/GruntMovementInline.h>
#include <Gruntz/GruntMovementMacros.h>
#include <Gruntz/GruntPoweredStateMacros.h>
#include <Gruntz/GruntPowerupSprite.h>
#include <Gruntz/GruntSelectedSprite.h>
#include <Gruntz/GruntSpriteMacros.h>
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
#include <Gruntz/RockNeighborMask.h>
#include <Gruntz/SbiMenuItemState.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialRecords.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SortKeyMacros.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/StaminaPct.h>
#include <Gruntz/State.h>
#include <Gruntz/StatusBarDock.h>
#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/StatusBarTab.h>
#include <Gruntz/TileCoordMacros.h>
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
    m_wwdObject->m_hitTypeFlags = 0x3d1;
    SetObjectFlags(WWD_GAME_OBJECT_FLAGS_CULL_SOUND_COLLIDE);
    m_wwdObject->m_collMask |= 0x103f;
    m_wwdObject->m_attackTypeMask = 1;
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
    RECT reach;
    reach.left = -1;
    reach.top = -1;
    reach.right = 1;
    reach.bottom = 1;
    m_reachRect = reach;
    RECT zero;
    zero.left = 0;
    zero.top = 0;
    zero.right = 0;
    zero.bottom = 0;
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
    m_unusedBattleCell.m_x = -1;
    m_unusedBattleCell.m_y = -1;
    m_arrivalNotified = false;
    m_defenderState = AISTATE_SEEK;
    m_battleState = BZTASK_UNASSIGNED;
    {
        CWwdSpriteObject* h = m_object;
        i32 lim = h->m_screenY + 0x186a0;
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

    m_timePerTile = g_buteMgr.GetDwordDef(
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

        i32 x = AT(m_poseToy, GRUNT_TOY1)->m_records.GetSize();
        LOAD_POSE(AT(m_poseToy, GRUNT_TOY2), s_pose_TOY2);
        i32 y = AT(m_poseToy, GRUNT_TOY2)->m_records.GetSize();

        if (x < y) {
            double blend =
                DATA_COMPGEN(0x001e9748, 100.0) / (static_cast<double>(y) / x - DATA_COMPGEN(0x001e9740, -1.0)) - g_slopeNegHalf;
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
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0004a780, 0x1ec)
GruntDirectionCell* MotionEntity::Classify(MotionEntity* other, char exact) {
    if (other == NULL) {
        return &g_gruntMoveDirCenter;
    }
    i32 horizontalDelta = static_cast<i32>((other->m_positionX - m_positionX));
    double otherY = other->m_positionY;
    i32 verticalDelta = static_cast<i32>((m_positionY - otherY));
    if (horizontalDelta == 0) {
        if (verticalDelta > 0) {
            return &g_gruntMoveDirNorth;
        }
        if (verticalDelta < 0) {
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
    double ratio = static_cast<double>(verticalDelta) / static_cast<double>(horizontalDelta);

    if (verticalDelta >= 0 && horizontalDelta > 0) {
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
    if (verticalDelta >= 0 && horizontalDelta < 0) {
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
    if (verticalDelta <= 0 && horizontalDelta > 0) {
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
    CGrunt* tgt = m_triggerMgr->FindAtPixel(m_object->m_screenX, m_object->m_screenY);
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
            i32 row = facing.row;
            i32 column = facing.column;
            i32 index = 3 * row + column;

            const char* nm = m_cells[index].IdleName().GetBuffer(0);
            SetImageFrameByName(nm, frame);
        }
        goto store;
    }

walk:

    SwitchAnimation(m_poseWalk);
    {
        i32 row = facing.row;
        i32 column = facing.column;
        i32 index = 3 * row + column;

        const char* nm = m_cells[index].WalkName().GetBuffer(0);
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
        m_arrivalFlags &= 0xe7fbfbfd;
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
    Coord center;
    Coord* point =
        center.Set((col << TILE_SHIFT_PX) + TILE_HALF_PX, (row << TILE_SHIFT_PX) + TILE_HALF_PX);
    return StepArrivalDrop(
        point->m_x,
        point->m_y,
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
    i32 lastX, lastY, tileX, tileY;
    i32 passableMask, cnt, headFlags, lastFlags, hit;
    i32 reinit;
    i32 nudged;
    RockNeighborMask free4;
    i32 step, acc, err, walkX, walkY, blocked;
    i32 saved[3][3];
    i32 sx, sy;
    bool eq;

    m_pendingTrigger = false;
    eq = ANIMATION_ACT_DIFFERS("D");
    if (!eq && pxX == m_entrancePx.m_x && pxY == m_entrancePx.m_y) {
        goto commitPhase;
    }

    RecycleGruntCoords(this);
    lastX = m_lastTilePx.m_x >> TILE_SHIFT_PX;
    lastY = m_lastTilePx.m_y >> TILE_SHIFT_PX;
    tileX = pxX >> TILE_SHIFT_PX;
    tileY = pxY >> TILE_SHIFT_PX;
    if (blockedMask == -1) {
        blockedMask = m_arrivalFlags;
    }
    m_arrivalTargetPx.m_x = pxX;
    m_arrivalTargetPx.m_y = pxY;
    passableMask = extraPassableMask | m_passableMask;
    if (g_gameReg->m_tileGrid->FindPathWithEndpointOverrides(
            lastX,
            lastY,
            tileX,
            tileY,
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
        lastFlags = g_gameReg->m_tileGrid->CellFlagsAt(lastX, lastY);
        if ((lastFlags & 0x80) != 0) {
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
            if (m_object->m_screenX == m_lastTilePx.m_x
                && m_lastTilePx.m_y == m_object->m_screenY) {
                FaceTowardTile(tileX, tileY);
            }
            return 0;
        }
        if (m_arrivalState == AI_BATTLEZ_PATH) {
            reinit = 0;
            goto commitEntrance;
        }
        {

            CPtrList probe(10);
            if (g_gameReg->m_tileGrid->FindPathWithEndpointOverrides(
                    lastX,
                    lastY,
                    tileX,
                    tileY,
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
        m_entrancePx.m_x = pxX;
        m_entrancePx.m_y = pxY;
        if (reinit != 0) {
            StepEntranceReinit();
        }
    commitPhase:
        m_arrivalPhase = arrivalPhase;
        return 1;
    }

    nudged = 0;

    CMapMgr* grid = g_gameReg->m_tileGrid;
    if (grid->m_rowInts[tileY][tileX * 7 + 4] != IDX(TILEKIND_GIANT_ROCK)) {
        goto nudgeDone;
    }
    free4 = (grid->m_rowInts[tileY + 1][tileX * 7 + 4] == IDX(TILEKIND_GIANT_ROCK)) ? ROCKADJ_BELOW
                                                                                    : ROCKADJ_NONE;
    free4 |= (grid->m_rowInts[tileY - 1][tileX * 7 + 4] == IDX(TILEKIND_GIANT_ROCK)) ? ROCKADJ_ABOVE
                                                                                     : ROCKADJ_NONE;
    free4 |= (grid->m_rowInts[tileY][tileX * 7 + 11] == IDX(TILEKIND_GIANT_ROCK)) ? ROCKADJ_RIGHT
                                                                                  : ROCKADJ_NONE;
    free4 |= (grid->m_rowInts[tileY][tileX * 7 - 3] == IDX(TILEKIND_GIANT_ROCK)) ? ROCKADJ_LEFT
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
            saved[sx - tileX + 1][sy - tileY + 1] = grid->m_rowInts[sy][sx * 7 + 7];
            grid->m_rowInts[sy][sx * 7 + 7] = 0;
        }
    }
    grid = g_gameReg->m_tileGrid;
    if (grid->FindPathWithEndpointOverrides(
            lastX,
            lastY,
            tileX,
            tileY,
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
                pxX = tail->m_x * TILE_SIZE_PX + TILE_HALF_PX;
                pxY = tail->m_y * TILE_SIZE_PX + TILE_HALF_PX;
            }
        }
    }
    for (sy = tileY - 1; sy < tileY + 2; sy++) {
        for (sx = tileX - 1; sx < tileX + 2; sx++) {
            grid->m_rowInts[sy][sx * 7 + 7] = saved[sx - tileX + 1][sy - tileY + 1];
        }
    }
    if (0 != nudged) {
        if (CoordCount() == 1 && arrivalPhase == IDX(PICKUP_BOOMERANG)
            && m_entranceReason == PICKUP_GAUNTLETZ) {
            m_triggerMgr->UseEquippedToolAt(m_playerIndex, m_unitIndex, pxX, pxY);
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
    if (AI_NONE != m_arrivalState) {
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
        CMapMgr* lineGrid = g_gameReg->m_tileGrid;
        acc = lastY << 16;
        sx = lastX;
        if (tileX - lastX > 0) {
            while (blocked == 0) {
                sy = acc >> 16;
                err = lineGrid->CellFlagsAt(sx, sy);
                if ((blockedMask & err) != 0 && (m_passableMask & err) == 0) {
                    blocked = 1;
                } else {
                    walkX = sx;
                    walkY = sy;
                }
                acc += step;
                sx++;
            }
        } else {
            while (blocked == 0) {
                sy = acc >> 16;
                err = lineGrid->CellFlagsAt(sx, sy);
                if ((blockedMask & err) != 0 && (m_passableMask & err) == 0) {
                    blocked = 1;
                } else {
                    walkX = sx;
                    walkY = sy;
                }
                acc += step;
                sx--;
            }
        }
    } else {
        step = ((tileX - lastX) << 16) / abs(tileY - lastY);
        CMapMgr* lineGrid = g_gameReg->m_tileGrid;
        acc = lastX << 16;
        sy = lastY;
        if (tileY - lastY > 0) {
            while (blocked == 0) {
                sx = acc >> 16;
                err = lineGrid->CellFlagsAt(sx, sy);
                if ((blockedMask & err) != 0 && (m_passableMask & err) == 0) {
                    blocked = 1;
                } else {
                    walkY = sy;
                    walkX = sx;
                }
                acc += step;
                sy++;
            }
        } else {
            while (blocked == 0) {
                sx = acc >> 16;
                err = lineGrid->CellFlagsAt(sx, sy);
                if ((blockedMask & err) != 0 && (m_passableMask & err) == 0) {
                    blocked = 1;
                } else {
                    walkY = sy;
                    walkX = sx;
                }
                acc += step;
                sy--;
            }
        }
    }
    if (walkX != lastX || lastY != walkY) {
        goto reProbe;
    }
reCommit:
    SetEntrancePos(1, 1);
    if (m_arrivalPending == false) {
        return 0;
    }
    m_arrivalPhase = arrivalPhase;
    return arrivalPhase != 0;

reProbe:
    pxX = walkX * TILE_SIZE_PX + TILE_HALF_PX;
    pxY = walkY * TILE_SIZE_PX + TILE_HALF_PX;
    clearEndpointFlags = 1;
    if (g_gameReg->m_tileGrid->FindPathWithEndpointOverrides(
            lastX,
            lastY,
            walkX,
            walkY,
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
        FaceTowardTile(walkX, walkY);
    }
    if (m_arrivalPending == false) {
        return 0;
    }
    m_arrivalPhase = arrivalPhase;
    return arrivalPhase != 0;
}

RVA(0x0004c170, 0xbe7)
i32 CGrunt::StepGruntMovement() {
    i32 coordX, coordY;
    i32 gtX, gtY;
    i32 recRow, recColumn;
    GruntDirection recDirection;
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
        if (entX == lastX && m_lastTilePx.m_y == entY) {
            goto label_ret1;
        }
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
            recColumn = g_gruntMoveDirSouthEast.column;
            recRow = g_gruntMoveDirSouthEast.row;
            recDirection = g_gruntMoveDirSouthEast.direction;
        } else if (coordY == gtY) {
            recColumn = g_gruntMoveDirEast.column;
            recDirection = g_gruntMoveDirEast.direction;
            recRow = g_gruntMoveDirEast.row;
        } else {
            recColumn = g_gruntMoveDirNorthEast.column;
            recDirection = g_gruntMoveDirNorthEast.direction;
            recRow = g_gruntMoveDirNorthEast.row;
        }
    } else if (coordX < gtX) {
        if (coordY > gtY) {
            recColumn = g_gruntMoveDirSouthWest.column;
            recRow = g_gruntMoveDirSouthWest.row;
            recDirection = g_gruntMoveDirSouthWest.direction;
        } else if (coordY == gtY) {
            recColumn = g_gruntMoveDirWest.column;
            recRow = g_gruntMoveDirWest.row;
            recDirection = g_gruntMoveDirWest.direction;
        } else {
            recColumn = g_gruntMoveDirNorthWest.column;
            recRow = g_gruntMoveDirNorthWest.row;
            recDirection = g_gruntMoveDirNorthWest.direction;
        }
    } else {
        if (coordY < gtY) {
            recRow = g_gruntMoveDirNorth.row;
            recColumn = g_gruntMoveDirNorth.column;
            recDirection = g_gruntMoveDirNorth.direction;
        } else {
            recColumn = g_gruntMoveDirSouth.column;
            recRow = g_gruntMoveDirSouth.row;
            recDirection = g_gruntMoveDirSouth.direction;
        }
    }
    rec.row = recRow;
    rec.column = recColumn;
    rec.direction = recDirection;

    SET_TILE_CENTER_PIXEL_PAIR_Y_FIRST(tgtPxY, tgtPxX, coordY, coordX)
    bd = g_gameReg->m_tileGrid;
    tgtTileX = tgtPxX >> TILE_SHIFT_PX;
    tgtTileY = tgtPxY >> TILE_SHIFT_PX;
    flagHead = bd->CellFlagsAt(tgtTileX, tgtTileY);

    {
        EnemyAiType st = m_arrivalState;
        i32 blockMove = 1;
        if (st == AI_OBJECTGUARD) {
            if (((m_defenderPx.m_x ^ tgtPxX) & 0xffffffe0) == 0
                && ((m_defenderPx.m_y ^ tgtPxY) & 0xffffffe0) == 0) {
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
        i32 ltx = m_lastTilePx.m_x >> TILE_SHIFT_PX;
        i32 lty = m_lastTilePx.m_y >> TILE_SHIFT_PX;
        i32 lastFlag = bd->CellFlagsAt(ltx, lty);
        if (!(lastFlag & 0x80)) {
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
                node->m_x = tgtTileX;
                node->m_y = tgtTileY;
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
                i32 cx = co->m_x;
                i32 cy = co->m_y;
                SET_TILE_CENTER_PIXEL_PAIR(tgtPxX, tgtPxY, cx, cy)
                i32 gx = m_object->m_screenX >> TILE_SHIFT_PX;
                i32 gy = m_object->m_screenY >> TILE_SHIFT_PX;
                if (cx > gx) {
                    if (cy > gy) {
                        recRow = g_gruntMoveDirSouthEast.row;
                        recColumn = g_gruntMoveDirSouthEast.column;
                        recDirection = g_gruntMoveDirSouthEast.direction;
                    } else if (cy == gy) {
                        recRow = g_gruntMoveDirEast.row;
                        recColumn = g_gruntMoveDirEast.column;
                        recDirection = g_gruntMoveDirEast.direction;
                    } else {
                        recRow = g_gruntMoveDirNorthEast.row;
                        recColumn = g_gruntMoveDirNorthEast.column;
                        recDirection = g_gruntMoveDirNorthEast.direction;
                    }
                } else if (cx < gx) {
                    if (cy > gy) {
                        recRow = g_gruntMoveDirSouthWest.row;
                        recColumn = g_gruntMoveDirSouthWest.column;
                        recDirection = g_gruntMoveDirSouthWest.direction;
                    } else if (gy == cy) {
                        recRow = g_gruntMoveDirWest.row;
                        recColumn = g_gruntMoveDirWest.column;
                        recDirection = g_gruntMoveDirWest.direction;
                    } else {
                        recRow = g_gruntMoveDirNorthWest.row;
                        recColumn = g_gruntMoveDirNorthWest.column;
                        recDirection = g_gruntMoveDirNorthWest.direction;
                    }
                } else {
                    if (cy < gy) {
                        recRow = g_gruntMoveDirNorth.row;
                        recColumn = g_gruntMoveDirNorth.column;
                        recDirection = g_gruntMoveDirNorth.direction;
                    } else {
                        recRow = g_gruntMoveDirSouth.row;
                        recColumn = g_gruntMoveDirSouth.column;
                        recDirection = g_gruntMoveDirSouth.direction;
                    }
                }
                rec.row = recRow;
                rec.column = recColumn;
                rec.direction = recDirection;
                CGruntzMapMgr* bd = g_gameReg->m_tileGrid;
                if (bd->m_rowInts[cy][cx * 7] & BRICKZ_CELL_OCCUPIED) {
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

    if ((flagHead & BRICKZ_CELL_OCCUPIED) && !(flagHead & 0x80)) {
        i32 owner;
        if (static_cast<u32>(tgtTileX) < static_cast<u32>(bd->m_width)
            && static_cast<u32>(tgtTileY) < static_cast<u32>(bd->m_height)) {
            owner = bd->m_rowInts[tgtTileY][tgtTileX * 7 + 1];
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
    if (flagHead & 0x80) {
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
        CGruntzMapMgr* bd = g_gameReg->m_tileGrid;
        i32 beyondFlag = bd->CellFlagsAt(btx, bty);
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
        CCueRect* rr = &g_gameReg->m_world->m_level->m_mainPlane->m_planeViewRect;
        if (CGameLevel::PointInRect(rr, hudX, hudY)) {
            g_gameReg->m_voiceManager->PlayGruntVoiceCue(this, 8, -1, -1, -1);
        }
        tgtPxX = beyondPxX;
        tgtPxY = beyondPxY;
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
    if (mask & BRICKZ_CELL_OCCUPIED) {
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
    } else if (dx < 0 && dy > 0) {
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
    } else if (dx > 0 && dy < 0) {
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
    } else if (dx < 0 && dy < 0) {
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
        i32 lastTileX = m_lastTilePx.m_x >> TILE_SHIFT_PX;
        i32 lastTileY = m_lastTilePx.m_y >> TILE_SHIFT_PX;
        CGruntzMapMgr* bdl = g_gameReg->m_tileGrid;

        bdl->m_rows[lastTileY][lastTileX].m_flagBytes[3] &= 0xdf;
        bdl->m_rows[lastTileY][lastTileX].m_occupantId = -1;

        tgtTileX = tgtPxX >> TILE_SHIFT_PX;
        tgtTileY = tgtPxY >> TILE_SHIFT_PX;
        CGruntzMapMgr* bd2 = g_gameReg->m_tileGrid;
        bd2->m_rows[tgtTileY][tgtTileX].m_flags |= BRICKZ_CELL_OCCUPIED;
        bd2->m_rows[tgtTileY][tgtTileX].m_occupantId =
            (m_playerIndex << GRUNT_IDENTITY_PLAYER_SHIFT) | m_unitIndex;

        m_lastTilePx.m_x = tgtPxX;
        m_lastTilePx.m_y = tgtPxY;
        ComputeFacing(1.0);
    }
    m_arrivalPending = true;
    if (reason12) {
        if (flagHead & 0x100) {
            if (m_coordToggle != false) {
                goto label_ret1;
            }
        } else {
            if (m_coordToggle == false) {
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
        m_object->m_screenX,
        m_object->m_screenY - 0x19,
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
        m_object->m_screenX,
        m_object->m_screenY - 0x19,
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
        m_object->m_screenX,
        m_object->m_screenY - 0x20,
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
        m_object->m_screenX,
        m_object->m_screenY - 0x20,
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
        m_object->m_screenX,
        m_object->m_screenY - 0x26,
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
        m_object->m_screenX,
        m_object->m_screenY,
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
        m_object->m_screenX,
        m_object->m_screenY,
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
    m_arrivalTargetPx.m_x = -1;
    m_arrivalTargetPx.m_y = -1;
    m_defenderPx.m_x = -1;
    m_defenderPx.m_y = -1;
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
    m_arrivalCell.m_x = -1;
    m_arrivalCell.m_y = -1;
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
        m_object->m_extent.left = (m_lastTilePx.m_x >> TILE_SHIFT_PX) - span->left;
        m_object->m_extent.right = span->right + (m_lastTilePx.m_x >> TILE_SHIFT_PX);
        m_object->m_extent.top = (m_lastTilePx.m_y >> TILE_SHIFT_PX) - span->top;
        m_object->m_extent.bottom = span->bottom + (m_lastTilePx.m_y >> TILE_SHIFT_PX);
    }
    RECT reach;
    CopyRect(&reach, &m_object->m_extent);
    if (reach.right - reach.left == 0 && reach.top - reach.bottom == 0) {
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
    i32 tx = m_lastTilePx.m_x >> TILE_SHIFT_PX;
    i32 ty = m_lastTilePx.m_y >> TILE_SHIFT_PX;
    plane->m_rowInts[ty][tx * 7] |= BRICKZ_CELL_OCCUPIED;
    plane->m_rowInts[ty][tx * 7 + 1] = (m_playerIndex << GRUNT_IDENTITY_PLAYER_SHIFT) | m_unitIndex;
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
                DECLARE_TILE_CENTER_PIXEL_PAIR(px, py, defenderQueuePosition, defenderPickupType)
                m_defenderPx.m_x = px;
                m_defenderPx.m_y = py;
                StepArrivalDrop(px, py - 0x20, 0, -1, 1, 0);
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
            i32 r = g_buteMgr.GetIntDef(m_animSetName, "ToolAA", 1);
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
                m_arrivalFlags |= 0x10;
            }
            m_passableMask = 0;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_BOMB: {
            m_animSetName = "BOMBGRUNT";
            i32 r = g_buteMgr.GetIntDef(m_animSetName, "ToolAA", 1);
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
                m_arrivalFlags |= 0x10;
            }
            m_passableMask = 0;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_BOOMERANG: {
            m_animSetName = "BOOMERANGGRUNT";
            i32 r = g_buteMgr.GetIntDef(m_animSetName, "ToolAA", 1);
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
                m_arrivalFlags |= 0x10;
            }
            m_passableMask = 0;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_BRICK: {
            m_animSetName = "BRICKGRUNT";
            i32 r = g_buteMgr.GetIntDef(m_animSetName, "ToolAA", 1);
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
                m_arrivalFlags |= 0x10;
            }
            m_passableMask = 0;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_CLUB: {
            m_animSetName = "CLUBGRUNT";
            i32 r = g_buteMgr.GetIntDef(m_animSetName, "ToolAA", 1);
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
                m_arrivalFlags |= 0x10;
            }
            m_passableMask = 0;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_GAUNTLETZ: {
            m_animSetName = "GAUNTLETZGRUNT";
            i32 r = g_buteMgr.GetIntDef(m_animSetName, "ToolAA", 1);
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
                m_arrivalFlags |= 0x10;
            }
            m_passableMask = 0;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_GLOVEZ: {
            m_animSetName = "GLOVEZGRUNT";
            i32 r = g_buteMgr.GetIntDef(m_animSetName, "ToolAA", 1);
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
                m_arrivalFlags |= 0x10;
            }
            m_passableMask = 0;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_GOOBER: {
            m_animSetName = "GOOBERGRUNT";
            i32 r = g_buteMgr.GetIntDef(m_animSetName, "ToolAA", 1);
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
                m_arrivalFlags |= 0x10;
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
            i32 r = g_buteMgr.GetIntDef(m_animSetName, "ToolAA", 1);
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
                m_arrivalFlags |= 0x10;
            }
            m_passableMask = 0x400;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_GUNHAT: {
            m_animSetName = "GUNHATGRUNT";
            i32 r = g_buteMgr.GetIntDef(m_animSetName, "ToolAA", 1);
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
                m_arrivalFlags |= 0x10;
            }
            m_passableMask = 0;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_NERFGUN: {
            m_animSetName = "NERFGUNGRUNT";
            i32 r = g_buteMgr.GetIntDef(m_animSetName, "ToolAA", 1);
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
                m_arrivalFlags |= 0x10;
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
            i32 r = g_buteMgr.GetIntDef(m_animSetName, "ToolAA", 1);
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
                m_arrivalFlags |= 0x10;
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
            i32 r = g_buteMgr.GetIntDef(m_animSetName, "ToolAA", 1);
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
                m_arrivalFlags |= 0x10;
            }
            m_passableMask = 0;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_SHOVEL: {
            m_animSetName = "SHOVELGRUNT";
            i32 r = g_buteMgr.GetIntDef(m_animSetName, "ToolAA", 1);
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
                m_arrivalFlags |= 0x10;
            }
            m_passableMask = 0;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_SPRING: {
            m_animSetName = "SPRINGGRUNT";
            i32 r = g_buteMgr.GetIntDef(m_animSetName, "ToolAA", 1);
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
                m_arrivalFlags |= 0x10;
            }
            m_passableMask = 0x1000;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_SPY: {
            m_animSetName = "SPYGRUNT";
            i32 r = g_buteMgr.GetIntDef(m_animSetName, "ToolAA", 1);
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
                m_arrivalFlags |= 0x10;
            }
            m_passableMask = 0;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_SWORD: {
            m_animSetName = "SWORDGRUNT";
            i32 r = g_buteMgr.GetIntDef(m_animSetName, "ToolAA", 1);
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
                m_arrivalFlags |= 0x10;
            }
            m_passableMask = 0;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_TIMEBOMB: {
            m_animSetName = "TIMEBOMBGRUNT";
            i32 r = g_buteMgr.GetIntDef(m_animSetName, "ToolAA", 1);
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
                m_arrivalFlags |= 0x10;
            }
            m_passableMask = 0;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_TOOB: {
            m_animSetName = "TOOBGRUNT";
            i32 r = g_buteMgr.GetIntDef(m_animSetName, "ToolAA", 1);
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
                m_arrivalFlags |= 0x10;
            }
            m_passableMask = 0x100;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_WAND: {
            m_animSetName = "WANDGRUNT";
            i32 r = g_buteMgr.GetIntDef(m_animSetName, "ToolAA", 1);
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
                m_arrivalFlags |= 0x10;
            }
            m_passableMask = 0;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_WARPSTONE: {
            m_animSetName = "WARPSTONEGRUNT";
            i32 r = g_buteMgr.GetIntDef(m_animSetName, "ToolAA", 1);
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
            i32 r = g_buteMgr.GetIntDef(m_animSetName, "ToolAA", 1);
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
                m_arrivalFlags |= 0x10;
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
            i32 r = g_buteMgr.GetIntDef(m_animSetName, "ToolAA", 1);
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
                m_arrivalFlags |= 0x10;
            }
            if (m_arrivalState == AI_DEFENDER) {
                m_defenderRadius = 1;
            }
            m_passableMask = 0xd02;
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
                m_arrivalFlags |= 0x10;
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
                m_arrivalFlags |= 0x10;
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
                m_arrivalFlags |= 0x10;
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
                m_arrivalFlags |= 0x10;
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
                m_arrivalFlags |= 0x10;
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
                m_arrivalFlags |= 0x10;
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
                m_arrivalFlags |= 0x10;
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
                m_arrivalFlags |= 0x10;
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
                m_arrivalFlags |= 0x10;
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
                m_arrivalFlags |= 0x10;
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
                m_arrivalFlags |= 0x10;
            }
            m_passableMask = 0;
            m_gruntKind = GRUNT_CONVERSION;
            m_convertTimeLo = g_buteMgr.GetDwordDef("Powerupz", "ConversionTime", 0x1f4);
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
            StopPowerupLoopSound();
            EnsurePowerupLoopSound("GAME_DEATHTOUCHLOOP");
            break;
        }
        case PICKUP_GHOST: {
            m_gruntKind = GRUNT_GHOST;
            i32 t = g_buteMgr.GetIntDef("Powerupz", "GruntGhostTransparencyOn", 0xe0);
            SET_DRAW_FILL_FRACTION(m_object, SHADE_PAL_ALPHA_16, t);
            if (m_powerupDuration == 0) {
                m_powerupDuration = g_buteMgr.GetDwordDef("Powerupz", "GhostTime", 0x4e20);
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
                m_powerupDuration =
                    g_buteMgr.GetDwordDef("Powerupz", "InvulnerabilityTime", 0x4e20);
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
                m_powerupDuration = g_buteMgr.GetDwordDef("Powerupz", "ReactiveArmorTime", 0x4e20);
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
                m_powerupDuration = g_buteMgr.GetDwordDef("Powerupz", "RoidzTime", 0x4e20);
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
            i32 mins = g_buteMgr.GetIntDef("Powerupz", "StopwatchMinutes", 1);
            i32 secs = g_buteMgr.GetIntDef("Powerupz", "StopwatchSeconds", 0);
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
            GruntDirectionCell cell = m_entranceCell;
            SetImageFrameByName(
                m_cells[cell.row * 3 + cell.column].m_names[1].GetBuffer(0),
                handle
            );
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
                GruntDirectionCell cell2 = m_entranceCell;
                SetImageSetByName(m_cells[cell2.row * 3 + cell2.column].m_names[2].GetBuffer(0));
                SwitchAnimation(m_poseWalk);
            } else {
                ResetEntranceAnimation(1, 0, 0);
                if (m_arrivalPending == false) {
                    m_triggerMgr->WireTileSwitchLogic(this, m_lastTilePx.m_x, m_lastTilePx.m_y);
                }
            }
        }
        i32 col = m_lastTilePx.m_x >> TILE_SHIFT_PX;
        i32 row = m_lastTilePx.m_y >> TILE_SHIFT_PX;
        TileCollisionKind tk = g_gameReg->m_tileGrid->m_rows[row][col].m_typeCode;
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
        m_triggerMgr->ReinitGroup(m_object->m_screenX, m_object->m_screenY);
    }
    return 1;
fail:
    return 0;
}
