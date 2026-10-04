#include <StdAfx.h>

#include <rva.h>

#include <Gruntz/Grunt.h>

#include <Bute/ButeMgr.h>
#include <DDrawMgr/AniAdvance.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Dsndmgr/SoundBuffer.h>
#include <Enums.h>
#include <Globals.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/ActRegistry.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AniElement.h>
#include <Gruntz/AniElementInline.h>
#include <Gruntz/AnimationRegistry.h>
#include <Gruntz/ArrivalFlagsPreset.h>
#include <Gruntz/BattlezMapConfig.h>
#include <Gruntz/BattlezTask.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/CoordPool.h>
#include <Gruntz/CurPlayer.h>
#include <Gruntz/DirectionClassify.h>
#include <Gruntz/EnemyAiType.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameModeId.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GameStats.h>
#include <Gruntz/GruntAiState.h>
#include <Gruntz/GruntConfigMacros.h>
#include <Gruntz/GruntCoordRecycleMacros.h>
#include <Gruntz/GruntDeathType.h>
#include <Gruntz/GruntDirectionInline.h>
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
#include <Gruntz/MapCellInline.h>
#include <Gruntz/MapTraversalInline.h>
#include <Gruntz/MovingLogicSerial.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/Play.h>
#include <Gruntz/Projectile.h>
#include <Gruntz/ResolveNodeInline.h>
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
#include <RectMacros.h>
#include <Rez/FrameClock.h>
#include <Rez/RezArchiveDir.h>
#include <Rez/RezTypeTag.h>
#include <Utils/MapTyped.h>
#include <Wap32/CoordUnset.h>
#include <Wap32/Object.h>
#include <Wap32/TileGeometry.h>
#include <Wap32/Wap32.h>
#include <Wwd/MoveMode.h>
#include <Wwd/WwdFile.h>
#include <Wwd/WwdObjectType.h>
#include <ZTools/BitVec.h>

#include <math.h>
#include <new>
#include <stdlib.h>
#include <string.h>

DATA(0x001e9750)
const double g_slopeNegHalf = -0.5;
DATA(0x001e9758)
const double g_slopePosHalf = 0.5;
DATA(0x001e9760)
const double s_slopePosTwo = 2.0;
DATA(0x001e9768)
const double s_slopeNegTwo = -2.0;

i32 g_movingSeed;

DATA(0x0020d424)
static char s_gruntAnimSuffix_BREAK[] = "_BREAK";
DATA(0x0020d42c)
static char s_gruntAnimSuffix_SOUTHEAST[] = "_SOUTHEAST";
DATA(0x0020d43c)
static char s_gruntAnimSuffix_SOUTH[] = "_SOUTH";
DATA(0x0020d444)
static char s_gruntAnimSuffix_SOUTHWEST[] = "_SOUTHWEST";
DATA(0x0020d454)
static char s_gruntAnimSuffix_EAST[] = "_EAST";
DATA(0x0020d45c)
static char s_gruntAnimSuffix_WEST[] = "_WEST";
DATA(0x0020d464)
static char s_gruntAnimSuffix_NORTHEAST[] = "_NORTHEAST";
DATA(0x0020d474)
static char s_gruntAnimSuffix_NORTH[] = "_NORTH";
DATA(0x0020d47c)
static char s_gruntAnimSuffix_NORTHWEST[] = "_NORTHWEST";
DATA(0x0020d48c)
static char s_gruntAnimSuffix_SOUTHEAST_ITEM[] = "_SOUTHEAST_ITEM";
DATA(0x0020d4a0)
static char s_gruntAnimSuffix_SOUTH_ITEM[] = "_SOUTH_ITEM";
DATA(0x0020d4b0)
static char s_gruntAnimSuffix_SOUTHWEST_ITEM[] = "_SOUTHWEST_ITEM";
DATA(0x0020d4c4)
static char s_gruntAnimSuffix_EAST_ITEM[] = "_EAST_ITEM";
DATA(0x0020d4d4)
static char s_gruntAnimSuffix_WEST_ITEM[] = "_WEST_ITEM";
DATA(0x0020d4e4)
static char s_gruntAnimSuffix_NORTHEAST_ITEM[] = "_NORTHEAST_ITEM";
DATA(0x0020d4f8)
static char s_gruntAnimSuffix_NORTH_ITEM[] = "_NORTH_ITEM";
DATA(0x0020d508)
static char s_gruntAnimSuffix_NORTHWEST_ITEM[] = "_NORTHWEST_ITEM";
DATA(0x0020d51c)
static char s_gruntAnimSuffix_SOUTHEAST_IDLE[] = "_SOUTHEAST_IDLE";
DATA(0x0020d530)
static char s_gruntAnimSuffix_SOUTH_IDLE[] = "_SOUTH_IDLE";
DATA(0x0020d540)
static char s_gruntAnimSuffix_SOUTHWEST_IDLE[] = "_SOUTHWEST_IDLE";
DATA(0x0020d554)
static char s_gruntAnimSuffix_EAST_IDLE[] = "_EAST_IDLE";
DATA(0x0020d564)
static char s_gruntAnimSuffix_WEST_IDLE[] = "_WEST_IDLE";
DATA(0x0020d574)
static char s_gruntAnimSuffix_NORTHEAST_IDLE[] = "_NORTHEAST_IDLE";
DATA(0x0020d588)
static char s_gruntAnimSuffix_NORTH_IDLE[] = "_NORTH_IDLE";
DATA(0x0020d598)
static char s_gruntAnimSuffix_NORTHWEST_IDLE[] = "_NORTHWEST_IDLE";
DATA(0x0020d5ac)
static char s_gruntAnimSuffix_SOUTHEAST_ATTACK[] = "_SOUTHEAST_ATTACK";
DATA(0x0020d5c4)
static char s_gruntAnimSuffix_SOUTH_ATTACK[] = "_SOUTH_ATTACK";
DATA(0x0020d5d4)
static char s_gruntAnimSuffix_SOUTHWEST_ATTACK[] = "_SOUTHWEST_ATTACK";
DATA(0x0020d5ec)
static char s_gruntAnimSuffix_EAST_ATTACK[] = "_EAST_ATTACK";
DATA(0x0020d5fc)
static char s_gruntAnimSuffix_WEST_ATTACK[] = "_WEST_ATTACK";
DATA(0x0020d60c)
static char s_gruntAnimSuffix_NORTHEAST_ATTACK[] = "_NORTHEAST_ATTACK";
DATA(0x0020d624)
static char s_gruntAnimSuffix_NORTH_ATTACK[] = "_NORTH_ATTACK";
DATA(0x0020d634)
static char s_gruntAnimSuffix_NORTHWEST_ATTACK[] = "_NORTHWEST_ATTACK";
DATA(0x0020d64c)
static char s_gruntAnimSuffix_SOUTHEAST_STRUCK[] = "_SOUTHEAST_STRUCK";
DATA(0x0020d664)
static char s_gruntAnimSuffix_SOUTH_STRUCK[] = "_SOUTH_STRUCK";
DATA(0x0020d674)
static char s_gruntAnimSuffix_SOUTHWEST_STRUCK[] = "_SOUTHWEST_STRUCK";
DATA(0x0020d68c)
static char s_gruntAnimSuffix_EAST_STRUCK[] = "_EAST_STRUCK";
DATA(0x0020d69c)
static char s_gruntAnimSuffix_WEST_STRUCK[] = "_WEST_STRUCK";
DATA(0x0020d6ac)
static char s_gruntAnimSuffix_NORTHEAST_STRUCK[] = "_NORTHEAST_STRUCK";
DATA(0x0020d6c4)
static char s_gruntAnimSuffix_NORTH_STRUCK[] = "_NORTH_STRUCK";
DATA(0x0020d6d4)
static char s_gruntAnimSuffix_NORTHWEST_STRUCK[] = "_NORTHWEST_STRUCK";
DATA(0x0020d6ec)
static char s_gruntAnimSuffix_SOUTHEAST_WALK[] = "_SOUTHEAST_WALK";
DATA(0x0020d700)
static char s_gruntAnimSuffix_SOUTH_WALK[] = "_SOUTH_WALK";
DATA(0x0020d710)
static char s_gruntAnimSuffix_SOUTHWEST_WALK[] = "_SOUTHWEST_WALK";
DATA(0x0020d724)
static char s_gruntAnimSuffix_EAST_WALK[] = "_EAST_WALK";
DATA(0x0020d734)
static char s_gruntAnimSuffix_WEST_WALK[] = "_WEST_WALK";
DATA(0x0020d744)
static char s_gruntAnimSuffix_NORTHEAST_WALK[] = "_NORTHEAST_WALK";
DATA(0x0020d758)
static char s_gruntAnimSuffix_NORTH_WALK[] = "_NORTH_WALK";
DATA(0x0020d768)
static char s_gruntAnimSuffix_NORTHWEST_WALK[] = "_NORTHWEST_WALK";
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

RVA_COMPGEN(0x0000f2c0, 0x1e, ??_GCGrunt@@UAEPAXI@Z)
RVA(0x0000f2f0, 0xc8)
CGrunt::~CGrunt() {
    OnObjectRemoved();
}

RVA_COMPGEN(0x0000f400, 0x1b, ??0CGruntCellRec@@QAE@XZ)

RVA_COMPGEN(0x0000f430, 0x10, ??1CGruntCellRec@@QAE@XZ)

RVA(0x00047a10, 0x770)
CGrunt::CGrunt(CGameObject* owner) : CMovingLogic(owner, CMovingLogic::GRUNT_SCALE), CWapX(owner) {
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
    m_pickupAnimation = NULL;
    m_selected = false;
    m_wwdObject->m_objectType = WWD_OBJECT_TYPE_GRUNT;
    m_wwdObject->m_hitTypeFlags = 0x3d1;
    SetObjectFlags(WWD_GAME_OBJECT_FLAGS_CULL_SOUND_COLLIDE);
    m_wwdObject->m_collMask |= 0x103f;
    m_wwdObject->m_attackTypeMask = 1;
    m_playerIndex = -1;
    m_unitIndex = -1;
    m_neighborPlayerIndex = -1;
    m_neighborUnitIndex = -1;
    m_warpstoneAnchorIndex = 0;
    m_activePickupType = PICKUP_NONE;
    m_carriedToyType = PICKUP_NONE;
    m_brickPickupType = PICKUP_NONE;
    m_gruntKind = GRUNT_NORMAL;
    m_savedToolType = PICKUP_NONE;
    m_animSetName = "NORMALGRUNT";
    m_entranceCommitted = true;
    m_healthSprite = NULL;
    m_staminaSprite = NULL;
    m_toyTimeSprite = NULL;
    m_wingzTimeSprite = NULL;
    m_selectedSprite = NULL;
    m_toySprite = NULL;
    m_powerupSprite = NULL;
    m_reserved210 = 0;
    m_attackWindupActive = false;
    m_attackQueued = false;
    m_arrivalActive = false;
    m_coordToggle = false;
    m_wingzEnabled = false;
    m_vehicleLoopSound = NULL;
    m_powerupLoopSound = NULL;
    RECT reach;
    SET_RECT_COMPONENTS(reach, -1, -1, 1, 1);
    m_reachRect = reach;
    RECT zero;
    SET_RECT_COMPONENTS(zero, 0, 0, 0, 0);
    m_reachExclusionRect = zero;
    m_toyUseRect = zero;
    m_toyUseExclusionRect = zero;

    m_toyTiming.Clear();
    m_idleDelayTiming.Clear();
    m_idleWindowTiming.Clear();
    m_entranceTiming.Clear();
    m_flashTiming.Clear();
    m_attackTiming.Clear();
    m_combatTiming.Clear();
    m_hudRetireTiming.Clear();
    m_wingzTiming.Clear();
    m_conversionTiming.Clear();
    m_shimmerTiming.Clear();
    m_walkVoiceTiming.Clear();
    m_arrivalRerollTiming.Clear();
    m_unusedBattleCell.Set(-1, -1);
    m_arrivalNotified = false;
    m_aiState = AISTATE_SEEK;
    m_battleState = BZTASK_UNASSIGNED;
    {
        CWwdSpriteObject* h = m_object;
        i32 lim = h->m_screenY + 0x186a0;
        SET_SORT_KEY_IF_CHANGED(h, lim);
    }
    m_blockedVoicePending = true;
}

RVA(0x00048360, 0x7e)
void CGrunt::OnObjectRemoved() {
    this->RecycleCoords();

    DeleteAllPayloads();
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
        m_cells[0].WalkName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_NORTHWEST_WALK;
        m_cells[1].WalkName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_NORTH_WALK;
        m_cells[2].WalkName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_NORTHEAST_WALK;
        m_cells[3].WalkName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_WEST_WALK;
        m_cells[4].WalkName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_NORTH_WALK;
        m_cells[5].WalkName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_EAST_WALK;
        m_cells[6].WalkName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_SOUTHWEST_WALK;
        m_cells[7].WalkName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_SOUTH_WALK;
        m_cells[8].WalkName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_SOUTHEAST_WALK;
        m_cells[0].StruckName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_NORTHWEST_STRUCK;
        m_cells[1].StruckName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_NORTH_STRUCK;
        m_cells[2].StruckName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_NORTHEAST_STRUCK;
        m_cells[3].StruckName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_WEST_STRUCK;
        m_cells[4].StruckName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_NORTH_STRUCK;
        m_cells[5].StruckName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_EAST_STRUCK;
        m_cells[6].StruckName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_SOUTHWEST_STRUCK;
        m_cells[7].StruckName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_SOUTH_STRUCK;
        m_cells[8].StruckName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_SOUTHEAST_STRUCK;
        m_cells[0].AttackName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_NORTHWEST_ATTACK;
        m_cells[1].AttackName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_NORTH_ATTACK;
        m_cells[2].AttackName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_NORTHEAST_ATTACK;
        m_cells[3].AttackName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_WEST_ATTACK;
        m_cells[4].AttackName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_NORTH_ATTACK;
        m_cells[5].AttackName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_EAST_ATTACK;
        m_cells[6].AttackName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_SOUTHWEST_ATTACK;
        m_cells[7].AttackName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_SOUTH_ATTACK;
        m_cells[8].AttackName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_SOUTHEAST_ATTACK;
        m_cells[0].IdleName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_NORTHWEST_IDLE;
        m_cells[1].IdleName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_NORTH_IDLE;
        m_cells[2].IdleName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_NORTHEAST_IDLE;
        m_cells[3].IdleName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_WEST_IDLE;
        m_cells[4].IdleName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_NORTH_IDLE;
        m_cells[5].IdleName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_EAST_IDLE;
        m_cells[6].IdleName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_SOUTHWEST_IDLE;
        m_cells[7].IdleName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_SOUTH_IDLE;
        m_cells[8].IdleName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_SOUTHEAST_IDLE;
        m_cells[0].ItemName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_NORTHWEST_ITEM;
        m_cells[1].ItemName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_NORTH_ITEM;
        m_cells[2].ItemName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_NORTHEAST_ITEM;
        m_cells[3].ItemName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_WEST_ITEM;
        m_cells[4].ItemName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_NORTH_ITEM;
        m_cells[5].ItemName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_EAST_ITEM;
        m_cells[6].ItemName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_SOUTHWEST_ITEM;
        m_cells[7].ItemName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_SOUTH_ITEM;
        m_cells[8].ItemName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_SOUTHEAST_ITEM;
        m_deathFrameSetName = "GRUNTZ_" + m_animSetName + "_DEATH";
    } else if (dirOnly != 0) {
        m_cells[0].WalkName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_NORTHWEST;
        m_cells[1].WalkName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_NORTH;
        m_cells[2].WalkName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_NORTHEAST;
        m_cells[3].WalkName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_WEST;
        m_cells[4].WalkName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_NORTH;
        m_cells[5].WalkName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_EAST;
        m_cells[6].WalkName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_SOUTHWEST;
        m_cells[7].WalkName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_SOUTH;
        m_cells[8].WalkName() = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_SOUTHEAST;
        m_frameSetName = "GRUNTZ_" + m_animSetName + s_gruntAnimSuffix_BREAK;
    } else {
        m_frameSetName = "GRUNTZ_" + m_animSetName;
    }
    CShadeTable* sel = g_gameReg->SpriteTable()->GetSel(IDX(m_moveIcon), kind);
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
        if (ratio <= s_slopePosTwo) {
            return &g_gruntMoveDirNorthEast;
        }
        return &g_gruntMoveDirNorth;
    }
    if (verticalDelta >= 0 && horizontalDelta < 0) {
        if (onCell) {
            return &g_gruntMoveDirNorthWest;
        }
        if (ratio <= s_slopeNegTwo) {
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
        if (ratio <= s_slopeNegTwo) {
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
    if (ratio <= s_slopePosTwo) {
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

    SET_POINT_COMPONENTS(b, m_object->m_screenX, m_object->m_screenY - 0x3e8);
    SET_POINT_COMPONENTS(a, m_object->m_screenX, m_object->m_screenY + 0x3e8);
    if (RectSegProbe(&r, &b, &a)) {
        return 1;
    }

    SET_POINT_COMPONENTS(b, m_object->m_screenX - 0x3e8, m_object->m_screenY);
    SET_POINT_COMPONENTS(a, m_object->m_screenX + 0x3e8, m_object->m_screenY);
    if (RectSegProbe(&r, &b, &a)) {
        return 1;
    }

    SET_POINT_COMPONENTS(b, m_object->m_screenX - 0x3e8, m_object->m_screenY - 0x3e8);
    SET_POINT_COMPONENTS(a, m_object->m_screenX + 0x3e8, m_object->m_screenY + 0x3e8);
    if (RectSegProbe(&r, &b, &a)) {
        return 1;
    }

    SET_POINT_COMPONENTS(b, m_object->m_screenX - 0x3e8, m_object->m_screenY + 0x3e8);
    SET_POINT_COMPONENTS(a, m_object->m_screenX + 0x3e8, m_object->m_screenY - 0x3e8);
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
    eq = IsAnimationAct("F");
    if (eq) {
        return;
    }
    bool ne;
    ne = IsNotAnimationAct("D");
    if (ne) {
        eq = IsAnimationAct("A");
        if (!eq) {
            eq = IsAnimationAct("K");
            if (!eq) {
                eq = IsAnimationAct("E");
                if (eq) {

                    SwitchAnimation(m_poseAttackIdle);
                    {
                        DECLARE_CURRENT_ANIMATION_FRAME(frame, desc, elem)
                        const char* nm = EntranceCell()->AttackName().GetBuffer(0);
                        SetImageFrameByName(nm, frame);
                    }
                    goto store;
                }
                eq = IsAnimationAct("I");
                if (!eq) {
                    eq = IsAnimationAct("M");
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
            DECLARE_CURRENT_ANIMATION_FRAME(frame, desc, elem)
            i32 row = facing.m_row;
            i32 column = facing.m_column;
            i32 index = 3 * row + column;

            const char* nm = m_cells[index].IdleName().GetBuffer(0);
            SetImageFrameByName(nm, frame);
        }
        goto store;
    }

walk:

    SwitchAnimation(m_poseWalk);
    {
        i32 row = facing.m_row;
        i32 column = facing.m_column;
        i32 index = 3 * row + column;

        const char* nm = m_cells[index].WalkName().GetBuffer(0);
        SetImageSetByName(nm);
    }

store:
    m_entranceCell = facing;
}

// @early-stop
RVA(0x0004b130, 0xc8)
i32 CGrunt::Select() {
    if (m_selected != false) {
        return 1;
    }

    if (m_tileClaimed != false && g_gameReg->GetGameMode() == GAMEMODE_MULTIPLAYER) {
        m_triggerMgr->EnqueueGuardEnd(m_playerIndex, m_unitIndex);
    } else if (m_tileClaimed != false) {
        END_GUARD(this);
    }
    CreateSelectedSprite();
    CreateHealthSprite();
    CreateToySprite();
    CreateStaminaSprite();
    CreateToyTimeSprite();
    CreateWingzTimeSprite();
    m_selected = true;
    return 1;
}

RVA(0x0004b240, 0xaa)
void CGrunt::Deselect() {
    if (m_selectedSprite) {
        m_selectedSprite->AddFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
        m_selectedSprite = NULL;
    }
    if (m_healthSprite) {
        m_healthSprite->AddFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
        m_healthSprite = NULL;
    }
    if (m_toySprite) {
        m_toySprite->AddFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
        m_toySprite = NULL;
    }
    if (m_entranceCommitted == false) {
        if (m_staminaSprite) {
            m_staminaSprite->AddFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
            m_staminaSprite = NULL;
        }
        if (m_toyTimeSprite) {
            m_toyTimeSprite->AddFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
            m_toyTimeSprite = NULL;
        }
        if (m_wingzTimeSprite) {
            m_wingzTimeSprite->AddFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
            m_wingzTimeSprite = NULL;
        }
    }
    m_selected = false;
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
    Coord* tail;
    POSITION pos;
    Coord lastTile, targetTile;
    i32 passableMask, cnt, headFlags, lastFlags, hit;
    i32 reinit;
    i32 nudged;
    RockNeighborMask free4;
    i32 step, acc, err, walkX, walkY, blocked;
    i32 saved[3][3];
    i32 sx, sy;
    bool eq;

    m_pendingTrigger = false;
    eq = IsNotAnimationAct("D");
    if (!eq && pxX == m_entrancePx.m_x && pxY == m_entrancePx.m_y) {
        goto commitPhase;
    }

    this->RecycleCoords();
    lastTile = ScreenTile(LastTilePx());
    targetTile.Set(pxX, pxY);
    ScreenTile(&targetTile);
    if (blockedMask == -1) {
        blockedMask = m_arrivalFlags;
    }
    m_arrivalTargetPx.Set(pxX, pxY);
    passableMask = extraPassableMask | m_passableMask;
    if (g_gameReg->GetTileGrid()->FindPathWithEndpointOverrides(
            lastTile.m_x,
            lastTile.m_y,
            targetTile.m_x,
            targetTile.m_y,
            GetCoordList(),
            clearEndpointFlags,
            blockedMask,
            passableMask
        )
        != 0) {
        if (!CoordsEmpty()) {
            g_coordPool.Push(RemoveHeadCoord());
        }
    pathGate:
        reinit = 1;
        cnt = CoordCount();
        if (cnt == 0) {
            goto commitEntrance;
        }
        tail = GetHeadCoord();
        headFlags = g_gameReg->GetTileGrid()->CellFlagsAt(tail->m_x, tail->m_y);
        lastFlags = g_gameReg->GetTileGrid()->CellFlagsAt(lastTile.m_x, lastTile.m_y);
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
                FaceTowardTile(targetTile.m_x, targetTile.m_y);
            }
            return 0;
        }
        if (m_aiType == AI_BATTLEZ_PATH) {
            reinit = 0;
            goto commitEntrance;
        }
        {

            CPtrList probe(10);
            if (g_gameReg->GetTileGrid()->FindPathWithEndpointOverrides(
                    lastTile.m_x,
                    lastTile.m_y,
                    targetTile.m_x,
                    targetTile.m_y,
                    &probe,
                    clearEndpointFlags,
                    blockedMask | BRICKZ_CELL_OCCUPIED,
                    passableMask
                ) != 0
                && !probe.IsEmpty()) {
                if (probe.GetCount() <= cnt + 3) {
                    g_coordPool.Push(probe.RemoveHead());
                    this->RecycleCoords();
                    pos = probe.GetHeadPosition();
                    while (pos != NULL) {
                        AddTailCoord(static_cast<Coord*>(probe.GetNext(pos)));
                    }
                } else {
                    RecycleCoordList(probe);
                }
                probe.RemoveAll();
            }
        }
    commitEntrance:
        m_entrancePx.Set(pxX, pxY);
        if (reinit != 0) {
            StepEntranceReinit();
        }
    commitPhase:
        m_arrivalPhase = arrivalPhase;
        return 1;
    }

    nudged = 0;

    CMapMgr* grid = g_gameReg->GetTileGrid();
    if (grid->CellTypeAt(targetTile.m_x, targetTile.m_y) != TILEKIND_GIANT_ROCK) {
        goto nudgeDone;
    }
    free4 = (grid->CellTypeAt(targetTile.m_x, targetTile.m_y + 1) == TILEKIND_GIANT_ROCK)
                ? ROCKADJ_BELOW
                : ROCKADJ_NONE;
    free4 |= (grid->CellTypeAt(targetTile.m_x, targetTile.m_y - 1) == TILEKIND_GIANT_ROCK)
                 ? ROCKADJ_ABOVE
                 : ROCKADJ_NONE;
    free4 |= (grid->CellTypeAt(targetTile.m_x + 1, targetTile.m_y) == TILEKIND_GIANT_ROCK)
                 ? ROCKADJ_RIGHT
                 : ROCKADJ_NONE;
    free4 |= (grid->CellTypeAt(targetTile.m_x - 1, targetTile.m_y) == TILEKIND_GIANT_ROCK)
                 ? ROCKADJ_LEFT
                 : ROCKADJ_NONE;
    switch (free4) {
        case ROCKADJ_RIGHT | ROCKADJ_BELOW:
            targetTile.m_x++;
            targetTile.m_y++;
            break;
        case ROCKADJ_RIGHT | ROCKADJ_ABOVE:
            targetTile.m_x++;
            targetTile.m_y--;
            break;
        case ROCKADJ_RIGHT | ROCKADJ_ABOVE | ROCKADJ_BELOW:
            targetTile.m_x++;
            break;
        case ROCKADJ_LEFT | ROCKADJ_BELOW:
            targetTile.m_x--;
            targetTile.m_y++;
            break;
        case ROCKADJ_LEFT | ROCKADJ_ABOVE:
            targetTile.m_x--;
            targetTile.m_y--;
            break;
        case ROCKADJ_LEFT | ROCKADJ_ABOVE | ROCKADJ_BELOW:
            targetTile.m_x--;
            break;
        case ROCKADJ_LEFT | ROCKADJ_RIGHT | ROCKADJ_BELOW:
            targetTile.m_y++;
            break;
        case ROCKADJ_LEFT | ROCKADJ_RIGHT | ROCKADJ_ABOVE:
            targetTile.m_y--;
            break;
        default:
            break;
    }

    for (sy = targetTile.m_y - 1; sy < targetTile.m_y + 2; sy++) {
        for (sx = targetTile.m_x - 1; sx < targetTile.m_x + 2; sx++) {
            saved[sx - targetTile.m_x + 1][sy - targetTile.m_y + 1] =
                grid->CellFlagsAtUnchecked(sx, sy);
            grid->CellFlagsAtUnchecked(sx, sy) = 0;
        }
    }
    grid = g_gameReg->GetTileGrid();
    if (grid->FindPathWithEndpointOverrides(
            lastTile.m_x,
            lastTile.m_y,
            targetTile.m_x,
            targetTile.m_y,
            GetCoordList(),
            clearEndpointFlags,
            blockedMask,
            passableMask
        ) != 0
        && !CoordsEmpty()) {
        g_coordPool.Push(RemoveHeadCoord());
        if (!CoordsEmpty()) {
            g_coordPool.Push(RemoveTailCoord());
            if (!CoordsEmpty()) {
                nudged = 1;
                tail = GetTailCoord();
                pxX = tail->m_x * TILE_SIZE_PX + TILE_HALF_PX;
                pxY = tail->m_y * TILE_SIZE_PX + TILE_HALF_PX;
            }
        }
    }
    for (sy = targetTile.m_y - 1; sy < targetTile.m_y + 2; sy++) {
        for (sx = targetTile.m_x - 1; sx < targetTile.m_x + 2; sx++) {
            grid->CellFlagsAtUnchecked(sx, sy) =
                saved[sx - targetTile.m_x + 1][sy - targetTile.m_y + 1];
        }
    }
    if (0 != nudged) {
        if (CoordCount() == 1 && arrivalPhase == IDX(PICKUP_BOOMERANG)
            && m_activePickupType == PICKUP_GAUNTLETZ) {
            m_triggerMgr->UseEquippedToolAt(m_playerIndex, m_unitIndex, pxX, pxY);
            SetEntrancePos(1, 1);
            return 1;
        }
    }
    m_arrivalTargetPx.Set(pxX, pxY);
nudgeDone:
    if (nudged != 0) {
        goto pathGate;
    }
    if (AI_NONE != m_aiType) {
        SetEntrancePos(1, 1);
        return 0;
    }
    if (lastTile == targetTile) {
        goto reCommit;
    }

    blocked = 0;
    walkX = targetTile.m_x;
    walkY = targetTile.m_y;
    if (abs(targetTile.m_x - lastTile.m_x) > abs(targetTile.m_y - lastTile.m_y)) {
        step = ((targetTile.m_y - lastTile.m_y) << 16) / abs(targetTile.m_x - lastTile.m_x);
        CMapMgr* lineGrid = g_gameReg->GetTileGrid();
        acc = lastTile.m_y << 16;
        sx = lastTile.m_x;
        if (targetTile.m_x - lastTile.m_x > 0) {
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
        step = ((targetTile.m_x - lastTile.m_x) << 16) / abs(targetTile.m_y - lastTile.m_y);
        CMapMgr* lineGrid = g_gameReg->GetTileGrid();
        acc = lastTile.m_x << 16;
        sy = lastTile.m_y;
        if (targetTile.m_y - lastTile.m_y > 0) {
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
    if (walkX != lastTile.m_x || lastTile.m_y != walkY) {
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
    if (g_gameReg->GetTileGrid()->FindPathWithEndpointOverrides(
            lastTile.m_x,
            lastTile.m_y,
            walkX,
            walkY,
            GetCoordList(),
            clearEndpointFlags,
            blockedMask,
            passableMask
        )
        != 0) {
        if (!CoordsEmpty()) {
            g_coordPool.Push(RemoveHeadCoord());
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
    Coord destination;
    Coord currentTile;
    GruntDirectionCell moveDirection;
    Coord targetPixel;
    i32 destinationFlags;
    i32 usingToob, usingWingz, usingSpring;
    i32 targetTileX, targetTileY;
    CGruntzMapMgr* tileGrid;

    {
        i32 destinationX = m_entrancePx.m_x;
        i32 lastTileX = m_lastTilePx.m_x;
        i32 destinationY = m_entrancePx.m_y;
        if (destinationX == lastTileX && m_lastTilePx.m_y == destinationY) {
            return 1;
        }
    }
    if (m_aiType == AI_BATTLEZ_PATH) {
        CBattlezMapConfig* slot = g_gameReg->m_players[m_playerIndex].GetBattlezConfig();
        if (slot != NULL && slot->ValidateUnitPath(this) == 0) {
            SetEntrancePos(1, 1);
            return 0;
        }
    }
    if (CoordsEmpty()) {
        goto stopWithoutPath;
    }
    if (m_aiType != AI_BATTLEZ_PATH) {
        Coord* pathCoord = RemoveHeadCoord();
        destination = *pathCoord;
        g_coordPool.Push(pathCoord);
    } else {
        Coord* pathCoord = GetHeadCoord();
        destination = *pathCoord;
    }

    currentTile = ScreenTile(this);
    moveDirection = MovementDirection(currentTile, destination);

    targetPixel.Set(
        (destination.m_x << TILE_SHIFT_PX) + TILE_HALF_PX,
        (destination.m_y << TILE_SHIFT_PX) + TILE_HALF_PX
    );
    tileGrid = g_gameReg->m_tileGrid;
    targetTileX = targetPixel.m_x >> TILE_SHIFT_PX;
    targetTileY = targetPixel.m_y >> TILE_SHIFT_PX;
    destinationFlags = tileGrid->CellFlagsAt(targetTileX, targetTileY);

    {
        EnemyAiType st = m_aiType;
        i32 blockMove = 1;
        if (st == AI_OBJECTGUARD) {
            if (((m_defenderPx.m_x ^ targetPixel.m_x) & 0xffffffe0) == 0
                && ((m_defenderPx.m_y ^ targetPixel.m_y) & 0xffffffe0) == 0) {
                blockMove = 0;
            }
        }
        if (blockMove != 0 && !(destinationFlags & BRICKZ_CELL_OCCUPIED)) {
            i32 mask = m_arrivalFlags & destinationFlags;
            if (!(mask & BRICKZ_CELL_OCCUPIED)) {
                if (mask == 0) {
                    goto prepareTraversal;
                }
                if (destinationFlags & m_passableMask) {
                    goto prepareTraversal;
                }
            }
        }
    }
    if (m_entranceActive == false) {
        i32 lastTileX = m_lastTilePx.m_x >> TILE_SHIFT_PX;
        i32 lastTileY = m_lastTilePx.m_y >> TILE_SHIFT_PX;
        i32 lastCellFlags = tileGrid->CellFlagsAt(lastTileX, lastTileY);
        if (!(lastCellFlags & 0x80)) {
            if (m_aiType == AI_BATTLEZ_PATH) {
                goto stopBlockedMovement;
            }
            if (CoordsEmpty()) {
                goto stopBlockedMovement;
            }
            {
                i32 mask = m_arrivalFlags & destinationFlags;
                if (mask & BRICKZ_CELL_OCCUPIED) {
                    goto stopBlockedMovement;
                }
                if (mask != 0 && !(destinationFlags & m_passableMask)) {
                    goto stopBlockedMovement;
                }
            }
            if (!(destinationFlags & BRICKZ_CELL_OCCUPIED)) {
                goto prepareTraversal;
            }
            {
                Coord* node = g_coordPool.Pop();
                node->Set(targetTileX, targetTileY);
                AddHeadCoord(node);
            }
            if (PathScan() == 0) {
                SetFacing(0x3e8, moveDirection);
                SetEntrancePos(1, 0);
                return 0;
            }

            if (CoordsEmpty()) {
                goto stopBlockedMovement;
            }
            {
                Coord* pathCoord = GetHeadCoord();
                i32 cx = pathCoord->m_x;
                i32 cy = pathCoord->m_y;
                targetPixel.Set(
                    (cx << TILE_SHIFT_PX) + TILE_HALF_PX,
                    (cy << TILE_SHIFT_PX) + TILE_HALF_PX
                );
                Coord current = ScreenTile(this);
                moveDirection = MovementDirection(current, *pathCoord);
                CGruntzMapMgr* tileGrid = g_gameReg->m_tileGrid;
                if (tileGrid->CellFlagsAtUnchecked(cx, cy) & BRICKZ_CELL_OCCUPIED) {
                    SetFacing(0x3e8, moveDirection);
                    SetEntrancePos(1, 0);
                    return 0;
                }
                Coord* consumedCoord = RemoveHeadCoord();
                g_coordPool.Push(consumedCoord);
                goto prepareTraversal;
            }
        }
    }

    if ((destinationFlags & BRICKZ_CELL_OCCUPIED) && !(destinationFlags & 0x80)) {
        i32 owner = tileGrid->OccupantAt(targetTileX, targetTileY);
        m_triggerMgr->StartUnitDeath(
            (owner >> GRUNT_IDENTITY_PLAYER_SHIFT) & GRUNT_IDENTITY_COMPONENT_MASK,
            owner & GRUNT_IDENTITY_COMPONENT_MASK,
            DEATH_SQUASH,
            m_playerIndex
        );
    }

prepareTraversal:
    if (m_aiType == AI_BATTLEZ_PATH && !CoordsEmpty()) {
        Coord* pathCoord = RemoveHeadCoord();
        g_coordPool.Push(pathCoord);
    }
    if (destinationFlags & 0x80) {
        m_entranceActive = true;
    } else {
        if (IsNotAnimationAct("L")) {
            m_entranceActive = false;
        }
    }

    usingToob = 0;
    usingWingz = 0;
    usingSpring = 0;
    if (m_activePickupType == PICKUP_TOOB) {
        usingToob = 1;
    } else if (m_activePickupType == PICKUP_WINGZ) {
        usingWingz = 1;
    } else if (m_activePickupType == PICKUP_SPRING) {
        usingSpring = 1;
    }
    if (usingSpring == 0) {
        goto commitMovement;
    }

    if (!(destinationFlags & 0x1400)) {
        if (!(destinationFlags & 0x2)) {
            goto commitMovement;
        }
    }
    if (targetPixel.m_x == m_entrancePx.m_x && targetPixel.m_y == m_entrancePx.m_y) {
        if ((destinationFlags & BRICKZ_BLOCKED_MASK) == 0) {
            goto validateSpringStep;
        }
        goto stopBlockedMovement;
    }
    {
        Coord beyondPixel;
        beyondPixel.Set(
            targetPixel.m_x * 2 - m_lastTilePx.m_x,
            targetPixel.m_y * 2 - m_lastTilePx.m_y
        );
        i32 beyondTileX = beyondPixel.m_x >> TILE_SHIFT_PX;
        i32 beyondTileY = beyondPixel.m_y >> TILE_SHIFT_PX;
        CGruntzMapMgr* tileGrid = g_gameReg->m_tileGrid;
        i32 beyondCellFlags = tileGrid->CellFlagsAt(beyondTileX, beyondTileY);
        if (beyondCellFlags & 0x20000939) {
            goto stopBlockedMovement;
        }
        if (!CoordsEmpty() && m_aiType != AI_BATTLEZ_PATH) {
            Coord* pathCoord = RemoveHeadCoord();
            if (pathCoord->m_x == beyondTileX && pathCoord->m_y == beyondTileY) {
                g_coordPool.Push(pathCoord);
            } else {
                AddHeadCoord(pathCoord);
            }
        }
        PLAY_GRUNT_CUE_IN_VIEW(8);
        targetPixel = beyondPixel;
    }

validateSpringStep: {
    i32 lastTileX = m_lastTilePx.m_x >> TILE_SHIFT_PX;
    targetTileX = targetPixel.m_x >> TILE_SHIFT_PX;
    i32 lastTileY = m_lastTilePx.m_y >> TILE_SHIFT_PX;
    targetTileY = targetPixel.m_y >> TILE_SHIFT_PX;
    CGruntzMapMgr* tileGrid = g_gameReg->m_tileGrid;
    if (tileGrid->CanStepBetween(
            lastTileX,
            lastTileY,
            targetTileX,
            targetTileY,
            m_arrivalFlags,
            m_passableMask
        )
        == 0) {
        goto stopBlockedMovement;
    }
    goto commitMovement;
}

stopBlockedMovement:
    SetFacing(0x3e8, moveDirection);
    SetEntrancePos(1, 1);
    return 0;

commitMovement:
    m_reserved210 = 0;
    m_triggerMgr->ApplySwitch(this, m_lastTilePx.m_x, m_lastTilePx.m_y);
    m_coordRetryCount = 0;
    SetFacing(0x3e8, moveDirection);
    {
        m_commitPx = m_lastTilePx;
        g_gameReg->GetTileGrid()->ReleaseCellOccupancy(
            m_lastTilePx.m_x >> TILE_SHIFT_PX,
            m_lastTilePx.m_y >> TILE_SHIFT_PX
        );

        targetTileX = targetPixel.m_x >> TILE_SHIFT_PX;
        targetTileY = targetPixel.m_y >> TILE_SHIFT_PX;
        CGruntzMapMgr* occupancyGrid = g_gameReg->m_tileGrid;
        occupancyGrid->AcquireCellOccupancy(targetTileX, targetTileY, m_playerIndex, m_unitIndex);

        m_lastTilePx = targetPixel;
        ComputeFacing(1.0);
    }
    m_arrivalPending = true;
    if (usingToob) {
        if (destinationFlags & 0x100) {
            if (m_coordToggle != false) {
                goto movementStarted;
            }
        } else {
            if (m_coordToggle == false) {
                return 1;
            }
        }
        RunMoveConfig(targetTileX, targetTileY);
        return 1;
    }
    if (usingWingz) {
        if (!(destinationFlags & 0xd02)) {
            return 1;
        }
        if (m_wingzEnabled != false) {
            goto movementStarted;
        }
        LoadWingzGruntSprites(true);
        return 1;
    }
    if (usingSpring) {
        SwitchAnimation(m_poseWalk);
        return 1;
    }
    return 1;

stopWithoutPath:
    SetEntrancePos(1, 1);
    return 0;

movementStarted:
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
    if (recycleRoute && m_aiType != AI_BATTLEZ_PATH && !CoordsEmpty()) {
        this->RecycleCoords();
    }
}

// @early-stop
RVA(0x0004d130, 0xb5)
i32 CGrunt::CreateHealthSprite() {
    if (m_healthSprite || m_health <= 0) {
        return 0;
    }

    m_healthSprite = g_gameReg->World()->ChildGroup()->CreateSprite(
        0,
        m_object->m_screenX,
        m_object->m_screenY - 0x19,
        SORTKEY_GRUNT_HUD,
        "GruntHealthSprite",
        WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
    );
    m_healthSprite->GetLogicRecord()->Dispatch(m_healthSprite);

    CLogicRecord* inner = m_healthSprite->GetLogicRecord();
    CGruntHealthSprite* reg = static_cast<CGruntHealthSprite*>(inner->UserLogic());
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

    m_toySprite = g_gameReg->World()->ChildGroup()->CreateSprite(
        0,
        m_object->m_screenX,
        m_object->m_screenY - 0x19,
        SORTKEY_GRUNT_HUD,
        "GruntToySprite",
        WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
    );
    m_toySprite->GetLogicRecord()->Dispatch(m_toySprite);

    CGruntToySprite* reg =
        static_cast<CGruntToySprite*>(m_toySprite->GetLogicRecord()->UserLogic());
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

    m_staminaSprite = g_gameReg->World()->ChildGroup()->CreateSprite(
        0,
        m_object->m_screenX,
        m_object->m_screenY - 0x20,
        SORTKEY_GRUNT_HUD,
        "GruntStaminaSprite",
        WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
    );
    m_staminaSprite->GetLogicRecord()->Dispatch(m_staminaSprite);

    CLogicRecord* inner = m_staminaSprite->GetLogicRecord();
    CGruntHealthSprite* reg = static_cast<CGruntHealthSprite*>(inner->UserLogic());
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

    m_toyTimeSprite = g_gameReg->World()->ChildGroup()->CreateSprite(
        0,
        m_object->m_screenX,
        m_object->m_screenY - 0x20,
        SORTKEY_GRUNT_HUD,
        "GruntToyTimeSprite",
        WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
    );
    m_toyTimeSprite->GetLogicRecord()->Dispatch(m_toyTimeSprite);

    CLogicRecord* inner = m_toyTimeSprite->GetLogicRecord();
    CGruntHealthSprite* reg = static_cast<CGruntHealthSprite*>(inner->UserLogic());
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

    m_wingzTimeSprite = g_gameReg->World()->ChildGroup()->CreateSprite(
        0,
        m_object->m_screenX,
        m_object->m_screenY - 0x26,
        SORTKEY_GRUNT_HUD,
        "GruntWingzTimeSprite",
        WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
    );
    m_wingzTimeSprite->GetLogicRecord()->Dispatch(m_wingzTimeSprite);

    CLogicRecord* inner = m_wingzTimeSprite->GetLogicRecord();
    CGruntHealthSprite* reg = static_cast<CGruntHealthSprite*>(inner->UserLogic());
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

    m_powerupSprite = g_gameReg->World()->ChildGroup()->CreateSprite(
        0,
        m_object->m_screenX,
        m_object->m_screenY,
        0x15,
        "GruntPowerupSprite",
        WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
    );
    m_powerupSprite->GetLogicRecord()->Dispatch(m_powerupSprite);

    CLogicRecord* inner = m_powerupSprite->GetLogicRecord();
    CGruntPowerupSprite* reg = static_cast<CGruntPowerupSprite*>(inner->UserLogic());
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

    m_selectedSprite = g_gameReg->World()->ChildGroup()->CreateSprite(
        0,
        m_object->m_screenX,
        m_object->m_screenY,
        0x14,
        "GruntSelectedSprite",
        WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
    );
    m_selectedSprite->GetLogicRecord()->Dispatch(m_selectedSprite);

    CGruntSelectedSprite* reg =
        static_cast<CGruntSelectedSprite*>(m_selectedSprite->GetLogicRecord()->UserLogic());
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
    i32 carriedToyType,
    EnemyAiType aiType,
    i32 defenderRadiusMinusOne,
    i32 defenderQueuePosition,
    i32 defenderPickupType,
    RECT* span,
    GruntEntranceMode entranceMode
) {
    if (aiType != AI_NONE) {
        if (aiType != AI_BATTLEZ_PATH) {
            m_arrivalFlags = ARRIVAL_FLAGS_ENEMY;
        } else {
            m_arrivalFlags = ARRIVAL_FLAGS_BATTLEZ;
        }
    } else {
        m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
        if (g_gameReg->GetGameMode() == GAMEMODE_QUESTZ) {
            m_arrivalFlags = ARRIVAL_FLAGS_PLAYER_SINGLE;
        }
    }
    m_arrivalTargetPx.Set(-1, -1);
    m_defenderPx.Set(-1, -1);
    m_powerupDuration = 0;
    m_blockedVoicePending = true;
    m_struckCount = 0;
    m_toyTileIndex = 0;
    m_pendingPickupType = PICKUP_INVALID;
    m_coordRetryCount = 0;
    m_moveKind = 0;
    m_moveVariantOverride = 0;
    m_moveVariant = 0;
    m_helpCueId = 0;
    m_aiType = aiType;
    m_brickPickupType = PICKUP_BROWNBRICK;
    m_playerIndex = playerIndex;
    m_defenderQueuePosition = defenderQueuePosition;
    m_unitIndex = unitIndex;
    m_arrivalCell.Set(-1, -1);
    m_defenderPickupType = static_cast<PickupType>(defenderPickupType);
    m_defenderRadius = defenderRadiusMinusOne + 1;
    m_arrivalRerollTiming.Clear();
    m_holdTiming.Clear();
    m_moveIcon = moveIcon;
    m_triggerMgr = board;
    m_daFlag = 1;
    m_arrivalPhase = 0;
    m_toolConfigured = true;
    m_tileClaimed = false;
    m_neighborScanEnabled = true;
    m_tileMoveCommitted = false;
    m_entranceArmed = false;
    m_spawnProtectionActive = false;
    m_deathType = DEATH_NONE;
    m_pendingTrigger = false;
    m_cellRemovalNotified = false;
    m_killerPlayerIndex = -1;
    m_passableMask = 0;
    m_savedMoveIcon = -1;
    m_lowStaminaCued = false;
    m_targetTeam = -1;
    SetCarriedToy(static_cast<PickupType>(carriedToyType));
    LoadGruntTypeTable(typeKind, 1, 0, 0);
    if (span != NULL) {
        SET_RECT_XY_EXTENTS(
            m_object->m_extent,
            (m_lastTilePx.m_x >> TILE_SHIFT_PX) - span->left,
            span->right + (m_lastTilePx.m_x >> TILE_SHIFT_PX),
            (m_lastTilePx.m_y >> TILE_SHIFT_PX) - span->top,
            span->bottom + (m_lastTilePx.m_y >> TILE_SHIFT_PX)
        );
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
    m_object->SetDrawFill(SHADE_PAL_16, shade);
    if (entranceMode != GRUNT_ENTRANCE_NONE) {
        BuildEntranceAnimation(entranceMode);
        return 1;
    }

    g_gameReg->GetTileGrid()->AcquireCellOccupancy(
        m_lastTilePx.m_x >> TILE_SHIFT_PX,
        m_lastTilePx.m_y >> TILE_SHIFT_PX,
        m_playerIndex,
        m_unitIndex
    );
    m_entranceActive = false;
    ReadConfigFromButeMgr();
    LoadCellAnimNames(0, 0);
    LoadAnimNameTable(0, 0);
    ResetEntranceAnimation(1, 0, 0);
    switch (aiType) {
        case AI_POSTGUARD:
            m_defenderPx = m_lastTilePx;
            break;
        case AI_OBJECTGUARD:
            if (defenderQueuePosition == 0 && defenderPickupType == 0) {
                m_defenderPx = m_lastTilePx;
                m_aiType = AI_POSTGUARD;
            } else {
                Coord defender;
                m_defenderPx = *defender.Set(
                    (defenderQueuePosition << TILE_SHIFT_PX) + TILE_HALF_PX,
                    (defenderPickupType << TILE_SHIFT_PX) + TILE_HALF_PX
                );
                StepArrivalDrop(defender.m_x, defender.m_y - TILE_SIZE_PX, 0, -1, 1, 0);
            }
            break;
        case AI_DEFENDER:
        case AI_BOMBER:
            m_defenderPx = m_lastTilePx;
            break;
    }
    return 1;
}

RVA(0x0004dd50, 0x2400)
i32 CGrunt::LoadGruntTypeTable(PickupType kind, i32 fresh, i32 variant, i32 defer) {
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
        if (IsNotAnimationAct("A")) {
            if (IsNotAnimationAct("D")) {
                goto fail;
            }
        }
    }
    if (m_activePickupType == kind) {
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
            if (m_activePickupType == kind) {
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
        m_wingzTiming.m_intervalLo = 0;
        m_wingzTiming.m_intervalHi = 0;
        HIDE_AND_CLEAR_GRUNT_SPRITE(m_wingzTimeSprite)
    }
    fresh = 0;
    defer = 0;
    if (m_activePickupType < PICKUP_EQUIPPABLE_END) {
        m_savedToolType = m_activePickupType;
    }
    switch (kind) {
        case PICKUP_NONE: {
            m_animSetName = "NORMALGRUNT";
            LOAD_GRUNT_TOOL_REACH()
            m_reachExclusionRect = MakeRect(0, 0, 0, 0);
            ResetArrivalFlags(this);
            MarkQuestzArrival(this);
            m_passableMask = 0;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_BOMB: {
            m_animSetName = "BOMBGRUNT";
            LOAD_GRUNT_TOOL_REACH()
            m_reachExclusionRect = MakeRect(0, 0, 0, 0);
            ResetArrivalFlags(this);
            MarkQuestzArrival(this);
            m_passableMask = 0;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_BOOMERANG: {
            m_animSetName = "BOOMERANGGRUNT";
            LOAD_GRUNT_TOOL_REACH()
            m_reachExclusionRect = MakeRect(0, 0, 0, 0);
            ResetArrivalFlags(this);
            if (m_aiType == AI_DEFENDER) {
                m_defenderRadius = 1;
            }
            MarkQuestzArrival(this);
            m_passableMask = 0;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_BRICK: {
            m_animSetName = "BRICKGRUNT";
            LOAD_GRUNT_TOOL_REACH()
            m_reachExclusionRect = MakeRect(0, 0, 0, 0);
            ResetArrivalFlags(this);
            MarkQuestzArrival(this);
            m_passableMask = 0;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_CLUB: {
            m_animSetName = "CLUBGRUNT";
            LOAD_GRUNT_TOOL_REACH()
            m_reachExclusionRect = MakeRect(0, 0, 0, 0);
            ResetArrivalFlags(this);
            MarkQuestzArrival(this);
            m_passableMask = 0;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_GAUNTLETZ: {
            m_animSetName = "GAUNTLETZGRUNT";
            LOAD_GRUNT_TOOL_REACH()
            m_reachExclusionRect = MakeRect(0, 0, 0, 0);
            ResetArrivalFlags(this);
            MarkQuestzArrival(this);
            m_passableMask = 0;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_GLOVEZ: {
            m_animSetName = "GLOVEZGRUNT";
            LOAD_GRUNT_TOOL_REACH()
            m_reachExclusionRect = MakeRect(0, 0, 0, 0);
            ResetArrivalFlags(this);
            MarkQuestzArrival(this);
            m_passableMask = 0;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_GOOBER: {
            m_animSetName = "GOOBERGRUNT";
            LOAD_GRUNT_TOOL_REACH()
            m_reachExclusionRect = MakeRect(0, 0, 0, 0);
            ResetArrivalFlags(this);
            MarkQuestzArrival(this);
            m_passableMask = 0;
            m_toolConfigured = true;
            if (m_aiType == AI_BATTLEZ_PATH) {
                if (m_battleState != BZTASK_ADVANCE) {
                    if (!this->CoordsEmpty()) {
                        RECYCLE_GRUNT_COORDS_VIA_NEXTDATA(this)
                    }
                    DeleteAllPayloads();
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
            LOAD_GRUNT_TOOL_REACH()
            m_reachExclusionRect = MakeRect(0, 0, 0, 0);
            ResetArrivalFlags(this);
            MarkQuestzArrival(this);
            m_passableMask = 0x400;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_GUNHAT: {
            m_animSetName = "GUNHATGRUNT";
            LOAD_GRUNT_TOOL_REACH()
            m_reachExclusionRect = MakeRect(0, 0, 0, 0);
            ResetArrivalFlags(this);
            if (m_aiType == AI_DEFENDER) {
                m_defenderRadius = 1;
            }
            MarkQuestzArrival(this);
            m_passableMask = 0;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_NERFGUN: {
            m_animSetName = "NERFGUNGRUNT";
            LOAD_GRUNT_TOOL_REACH()
            m_reachExclusionRect = MakeRect(0, 0, 0, 0);
            ResetArrivalFlags(this);
            MarkQuestzArrival(this);
            if (m_aiType == AI_DEFENDER) {
                m_defenderRadius = 1;
            }
            m_passableMask = 0;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_ROCK: {
            m_animSetName = "ROCKGRUNT";
            LOAD_GRUNT_TOOL_REACH()
            m_reachExclusionRect = MakeRect(0, 0, 0, 0);
            ResetArrivalFlags(this);
            MarkQuestzArrival(this);
            if (m_aiType == AI_DEFENDER) {
                m_defenderRadius = 1;
            }
            m_passableMask = 0;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_SHIELD: {
            m_animSetName = "SHIELDGRUNT";
            LOAD_GRUNT_TOOL_REACH()
            m_reachExclusionRect = MakeRect(0, 0, 0, 0);
            ResetArrivalFlags(this);
            MarkQuestzArrival(this);
            m_passableMask = 0;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_SHOVEL: {
            m_animSetName = "SHOVELGRUNT";
            LOAD_GRUNT_TOOL_REACH()
            m_reachExclusionRect = MakeRect(0, 0, 0, 0);
            ResetArrivalFlags(this);
            MarkQuestzArrival(this);
            m_passableMask = 0;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_SPRING: {
            m_animSetName = "SPRINGGRUNT";
            LOAD_GRUNT_TOOL_REACH()
            m_reachExclusionRect = MakeRect(0, 0, 0, 0);
            ResetArrivalFlags(this);
            MarkQuestzArrival(this);
            m_passableMask = 0x1000;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_SPY: {
            m_animSetName = "SPYGRUNT";
            LOAD_GRUNT_TOOL_REACH()
            m_reachExclusionRect = MakeRect(0, 0, 0, 0);
            ResetArrivalFlags(this);
            MarkQuestzArrival(this);
            m_passableMask = 0;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_SWORD: {
            m_animSetName = "SWORDGRUNT";
            LOAD_GRUNT_TOOL_REACH()
            m_reachExclusionRect = MakeRect(0, 0, 0, 0);
            ResetArrivalFlags(this);
            MarkQuestzArrival(this);
            m_passableMask = 0;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_TIMEBOMB: {
            m_animSetName = "TIMEBOMBGRUNT";
            LOAD_GRUNT_TOOL_REACH()
            m_reachExclusionRect = MakeRect(0, 0, 0, 0);
            ResetArrivalFlags(this);
            MarkQuestzArrival(this);
            m_passableMask = 0;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_TOOB: {
            m_animSetName = "TOOBGRUNT";
            LOAD_GRUNT_TOOL_REACH()
            m_coordToggle = false;
            m_reachExclusionRect = MakeRect(0, 0, 0, 0);
            ResetArrivalFlags(this);
            MarkQuestzArrival(this);
            m_passableMask = 0x100;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_WAND: {
            m_animSetName = "WANDGRUNT";
            LOAD_GRUNT_TOOL_REACH()
            m_reachExclusionRect = MakeRect(0, 0, 0, 0);
            ResetArrivalFlags(this);
            MarkQuestzArrival(this);
            m_passableMask = 0;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_WARPSTONE: {
            m_animSetName = "WARPSTONEGRUNT";
            LOAD_GRUNT_TOOL_REACH()
            m_reachExclusionRect = MakeRect(0, 0, 0, 0);
            ResetArrivalFlags(this);
            m_passableMask = 0;
            m_toolConfigured = false;
            break;
        }
        case PICKUP_WELDER: {
            m_animSetName = "WELDERGRUNT";
            LOAD_GRUNT_TOOL_REACH()
            m_reachExclusionRect = MakeRect(0, 0, 0, 0);
            ResetArrivalFlags(this);
            MarkQuestzArrival(this);
            if (m_aiType == AI_DEFENDER) {
                m_defenderRadius = 1;
            }
            m_passableMask = 0;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_WINGZ: {
            m_animSetName = "WINGZGRUNT";
            LOAD_GRUNT_TOOL_REACH()
            m_reachExclusionRect = MakeRect(0, 0, 0, 0);
            ResetArrivalFlags(this);
            MarkQuestzArrival(this);
            if (m_aiType == AI_DEFENDER) {
                m_defenderRadius = 1;
            }
            m_passableMask = 0xd02;
            m_wingzEnabled = false;
            m_wingzTime = 0x64;
            m_toolConfigured = true;
            break;
        }
        case PICKUP_BABYWALKER: {
            ResetArrivalFlags(this);
            MarkQuestzArrival(this);
            m_passableMask = 0;
            m_animSetName = "BABYWALKERGRUNT";
            if (IsAnimationAct("D")) {
                ConsiderArrival(0);
            }
            fresh = 1;
            defer = 1;
            break;
        }
        case PICKUP_BEACHBALL: {
            ResetArrivalFlags(this);
            MarkQuestzArrival(this);
            m_passableMask = 0;
            m_animSetName = "BEACHBALLGRUNT";
            if (IsAnimationAct("D")) {
                ConsiderArrival(0);
            }
            fresh = 1;
            break;
        }
        case PICKUP_BIGWHEEL: {
            ResetArrivalFlags(this);
            MarkQuestzArrival(this);
            m_passableMask = 0;
            m_animSetName = "BIGWHEELGRUNT";
            if (IsAnimationAct("D")) {
                ConsiderArrival(0);
            }
            fresh = 1;
            defer = 1;
            break;
        }
        case PICKUP_GOKART: {
            ResetArrivalFlags(this);
            MarkQuestzArrival(this);
            m_passableMask = 0;
            m_animSetName = "GOKARTGRUNT";
            if (IsAnimationAct("D")) {
                ConsiderArrival(0);
            }
            fresh = 1;
            defer = 1;
            break;
        }
        case PICKUP_JACKINTHEBOX: {
            ResetArrivalFlags(this);
            MarkQuestzArrival(this);
            m_passableMask = 0;
            m_animSetName = "JACKINTHEBOXGRUNT";
            if (IsAnimationAct("D")) {
                ConsiderArrival(0);
            }
            fresh = 1;
            break;
        }
        case PICKUP_JUMPROPE: {
            ResetArrivalFlags(this);
            MarkQuestzArrival(this);
            m_passableMask = 0;
            m_animSetName = "JUMPROPEGRUNT";
            if (IsAnimationAct("D")) {
                ConsiderArrival(0);
            }
            fresh = 1;
            break;
        }
        case PICKUP_POGOSTICK: {
            ResetArrivalFlags(this);
            MarkQuestzArrival(this);
            m_passableMask = 0;
            m_animSetName = "POGOSTICKGRUNT";
            if (IsAnimationAct("D")) {
                ConsiderArrival(0);
            }
            fresh = 1;
            defer = 1;
            break;
        }
        case PICKUP_SCROLL: {
            ResetArrivalFlags(this);
            MarkQuestzArrival(this);
            m_moveVariant = variant;
            m_passableMask = 0;
            m_animSetName = "SCROLLGRUNT";
            if (IsAnimationAct("D")) {
                ConsiderArrival(0);
            }
            fresh = 1;
            break;
        }
        case PICKUP_SQUEAKTOY: {
            ResetArrivalFlags(this);
            MarkQuestzArrival(this);
            m_passableMask = 0;
            m_animSetName = "SQUEAKTOYGRUNT";
            if (IsAnimationAct("D")) {
                ConsiderArrival(0);
            }
            fresh = 1;
            break;
        }
        case PICKUP_YOYO: {
            ResetArrivalFlags(this);
            MarkQuestzArrival(this);
            m_passableMask = 0;
            m_animSetName = "YOYOGRUNT";
            if (IsAnimationAct("D")) {
                ConsiderArrival(0);
            }
            fresh = 1;
            break;
        }
        case PICKUP_HEALTH1: {
            i32 h = g_buteMgr.GetInt("Powerupz", "Health1", 0x19) + m_health;
            m_health = min(h, HEALTH_FULL);
            return 1;
        }
        case PICKUP_HEALTH2: {
            i32 h = g_buteMgr.GetInt("Powerupz", "Health2", 0x19) + m_health;
            m_health = min(h, HEALTH_FULL);
            return 1;
        }
        case PICKUP_HEALTH3: {
            i32 h = g_buteMgr.GetInt("Powerupz", "Health3", 0x19) + m_health;
            m_health = min(h, HEALTH_FULL);
            return 1;
        }
        case PICKUP_CONVERSION: {
            m_savedToolType = m_activePickupType;
            m_reachRect = MakeRect(-1, -1, 1, 1);
            m_reachExclusionRect = MakeRect(0, 0, 0, 0);
            fresh = 0;
            m_animSetName = "HAREKRISHNAGRUNT";
            ResetArrivalFlags(this);
            MarkQuestzArrival(this);
            m_passableMask = 0;
            m_gruntKind = GRUNT_CONVERSION;
            m_conversionTiming.m_interval = g_buteMgr.GetDword("Powerupz", "ConversionTime", 0x1f4);
            m_conversionTiming.m_start = g_frameTime;
            StopPowerupLoopSound();
            EnsurePowerupLoopSound("GAME_CONVERSIONLOOP");
            break;
        }
        case PICKUP_DEATHTOUCH: {
            m_savedToolType = m_activePickupType;
            m_reachRect = MakeRect(-1, -1, 1, 1);
            m_reachExclusionRect = MakeRect(0, 0, 0, 0);
            fresh = 0;
            m_animSetName = "REAPERGRUNT";
            ResetArrivalFlags(this);
            MarkQuestzArrival(this);
            m_passableMask = 0;
            m_gruntKind = GRUNT_DEATHTOUCH;
            if (m_powerupDuration == 0) {
                m_powerupDuration = g_buteMgr.GetDword("Powerupz", "DeathTouchTime", 0x4e20);
            }
            m_conversionTiming.m_interval = static_cast<u32>(m_powerupDuration);
            m_conversionTiming.m_start = g_frameTime;
            m_shimmerTiming.m_intervalLo = 0;
            m_shimmerTiming.m_intervalHi = 0;
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
            m_conversionTiming.m_interval = static_cast<u32>(m_powerupDuration);
            m_conversionTiming.m_start = g_frameTime;
            m_shimmerTiming.m_intervalLo = 0;
            m_shimmerTiming.m_intervalHi = 0;
            StopPowerupLoopSound();
            EnsurePowerupLoopSound("GAME_GHOSTLOOP");
            return 1;
        }
        case PICKUP_INVULNERABILITY: {
            m_gruntKind = GRUNT_INVULNERABLE;
            if (m_powerupDuration == 0) {
                m_powerupDuration = g_buteMgr.GetDword("Powerupz", "InvulnerabilityTime", 0x4e20);
            }
            m_conversionTiming.m_interval = static_cast<u32>(m_powerupDuration);
            m_conversionTiming.m_start = g_frameTime;
            m_shimmerTiming.m_intervalLo = 0;
            m_shimmerTiming.m_intervalHi = 0;
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
            m_conversionTiming.m_interval = static_cast<u32>(m_powerupDuration);
            m_conversionTiming.m_start = g_frameTime;
            m_shimmerTiming.m_intervalLo = 0;
            m_shimmerTiming.m_intervalHi = 0;
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
            m_conversionTiming.m_interval = static_cast<u32>(m_powerupDuration);
            m_conversionTiming.m_start = g_frameTime;
            m_shimmerTiming.m_intervalLo = 0;
            m_shimmerTiming.m_intervalHi = 0;
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
            m_conversionTiming.m_interval = static_cast<u32>(m_powerupDuration);
            m_conversionTiming.m_start = g_frameTime;
            m_shimmerTiming.m_intervalLo = 0;
            m_shimmerTiming.m_intervalHi = 0;
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
                if (sb->GetState() == STATUSBAR_HIDDEN) {
                    sb->RestoreStatusBar();
                }
                if (sb->GetActiveTab() != TAB_RESOURCE) {
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
    m_activePickupType = kind;
    ReadConfigFromButeMgr();
    LoadCellAnimNames(fresh, defer);
    LoadAnimNameTable(fresh, defer);
    if (fresh == 0) {
        if (IsAnimationAct("H")) {
            DECLARE_CURRENT_ANIMATION_FRAME(handle, el, first)
            SetImageFrameByName(EntranceCell()->StruckName().GetBuffer(0), handle);
        } else {
            if (m_inCombat != false && m_attackQueued == false) {
                RESET_GRUNT_COMBAT_STATE(this)
            }
            if (IsAnimationAct("D")) {
                SetImageSetByName(EntranceCell()->WalkName().GetBuffer(0));
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
    if (m_selected != false) {
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
