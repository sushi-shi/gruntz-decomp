#include <StdAfx.h>

#include <rva.h>

#include <Gruntz/BattlezMapConfig.h>

#include <Bute/ButeMgr.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawChildGroupScanInline.h>
#include <DDrawMgr/DDrawWorkerHost.h>
#include <Globals.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/BattlezDifficulty.h>
#include <Gruntz/BattlezGruntInline.h>
#include <Gruntz/BattlezIntervalMs.h>
#include <Gruntz/BattlezRouteMaskPreset.h>
#include <Gruntz/BattlezTask.h>
#include <Gruntz/BrickTileId.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/CoordPool.h>
#include <Gruntz/EnemyAiType.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameObjectLogicTypes.h>
#include <Gruntz/GameRand.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntAiState.h>
#include <Gruntz/GruntCoordRecycleMacros.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntMovementInline.h>
#include <Gruntz/GruntMovementMacros.h>
#include <Gruntz/GruntPickupInline.h>
#include <Gruntz/GruntPoweredStateMacros.h>
#include <Gruntz/GruntPuddle.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GruntzPlayer.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/MapCellFlags.h>
#include <Gruntz/MapMgr.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/Play.h>
#include <Gruntz/ScanGridMacros.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialRecords.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/StaminaPct.h>
#include <Gruntz/TileActionEvent.h>
#include <Gruntz/TileCollisionKind.h>
#include <Gruntz/TileCoordMacros.h>
#include <Gruntz/TileTriggerContainer.h>
#include <Gruntz/TileTriggerLogic.h>
#include <Gruntz/TileTriggerSwitchLogic.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/VoiceManager.h>
#include <Io/FileMem.h>
#include <Lith/BDefs.h>
#include <RectMacros.h>
#include <Wap32/TileGeometry.h>
#include <Wwd/WwdFile.h>
#include <ZTools/BitVec.h>
#include <ZTools/ZDArray.h>

#include <limits.h>
#include <math.h>
#include <new>
#include <stdlib.h>
#include <string.h>

DATA(0x001e96ec)
const float g_diffScale = 0.01f;

DATA(0x0020ccc0)
i32 g_battlezRouteBlockedMask = 0x98f;
DATA(0x0022b6dc)
b32 g_stepRun;
DATA(0x0022b730)
i32 g_stepCol;
DATA(0x0022b734)
i32 g_stepRow;
DATA(0x0022b738)
i32 g_diffTier;

RVA_DYNINIT(0x0002d7c0, 0x5, s_gruntDirSpare)
RVA_DYNINIT(0x0002d7e0, 0x20, s_gruntDirSpare)
DATA(0x0022b73c)
static GruntDirectionCell s_gruntDirSpare[3];

// @early-stop
RVA(0x00024dc0, 0x158)
CBattlezMapConfig::CBattlezMapConfig() {
    m_playerIndex = 0;
    m_reserved01c = 1;
    m_reserved020 = 0x40;
    m_reserved024 = 0x40;
    m_reserved028 = 0x40;
    m_defenderSearchRadiusX = 5;
    m_defenderSearchRadiusY = 5;
    m_reserved02c = 0x32;
    m_idleRouteLimitX = 8;
    m_idleRouteLimitY = 8;
    m_idleBurnRandX = 8;
    m_idleBurnRandY = 8;
    m_defenderChance = 0x32;
    m_reserveBudget = 0x3e8;
    m_moveBudget = 0x3e8;
    m_reserved088 = 0x32;
    m_reserved0a8 = 0x32;
    m_gruntCreationTime = 0;
    m_resourceCreationTime = 0;
    m_spawnLastFire = 0;
    m_repickLastFire = 0;
    m_repickTimer = 0;
    m_spawnTimer = 0;
    m_repathBudget = 0xbb8;
    m_nearbyRouteSearchDelay = 0xbb8;
    m_reserved13c = 0;
    m_roundRobinTick = 0;
    m_reserved09c = 0x7d0;
    m_idleAttackWaypointDelay = 0x7d0;
    m_defenderTargetMaxDistance = 6;
    m_idleRerouteDelay = 0x7d0;
    m_assignedTargetMaxDistance = 0xa;
    m_inactiveTargetRerouteDelay = 0x7530;
    m_gruntRatio = 0x19;
}

RVA(0x00024f80, 0x7d)
CBattlezMapConfig::~CBattlezMapConfig() {
    FreeArrays();
}

// @early-stop
RVA(0x00025020, 0x984)
i32 CBattlezMapConfig::LoadConfig(CGruntzMgr* mgr, i32 playerIndex, BattlezDifficulty difficulty) {

    m_gruntCreationTime = 0;
    m_spawnTimer = 0;
    m_spawnLastFire = 0;
    m_resourceCreationTime = 0;
    m_repickLastFire = 0;
    m_repickTimer = 0;
    m_ctx = mgr;
    m_playerIndex = playerIndex;
    m_triggerMgr = mgr->GetTriggerMgr();
    m_board = mgr->GetTileGrid();
    m_play = static_cast<CPlay*>(mgr->m_curState);
    m_cellQuery = m_play->GetTileTriggers();
    m_active = true;

    m_gruntCreationTime = g_buteMgr.GetDword("Battlez", "GruntCreationTime", 10000);
    m_resourceCreationTime = g_buteMgr.GetDword("Battlez", "ResourceCreationTime", 10000);
    m_gauntletzChance = g_buteMgr.GetDword("Battlez", "GauntletzChance", 50);
    m_shovelzChance = g_buteMgr.GetDword("Battlez", "ShovelzChance", 50);
    m_spyzChance = g_buteMgr.GetDword("Battlez", "SpyzChance", 50);
    m_brickzChance = g_buteMgr.GetDword("Battlez", "BrickzChance", 50);
    m_gooberzChance = g_buteMgr.GetDword("Battlez", "GooberzChance", 50);
    m_gruntRatio = g_buteMgr.GetDword("Battlez", "GruntRatio", 25);
    m_defenderChance = g_buteMgr.GetDword("Battlez", "DefenderChance", 50);

    for (CGameObject* cur = mgr->m_world->ChildGroup()->FirstChild(); cur != NULL;
         cur = mgr->m_world->ChildGroup()->NextChild()) {
        if (cur->GetLogicRecord()->GetDispatch() == &DispatchGruntCreationPointLogic
            && cur->m_smarts == playerIndex) {
            Coord* slot = g_coordPool.Pop();
            slot->m_x = cur->m_screenX / TILE_SIZE_PX;
            slot->m_y = cur->m_screenY / TILE_SIZE_PX;
            m_candArray.Add(slot);
        }
    }

    for (CGameObject* cur2 = mgr->m_world->ChildGroup()->FirstChild(); cur2 != NULL;
         cur2 = mgr->m_world->ChildGroup()->NextChild()) {
        if (cur2->GetLogicRecord()->GetDispatch() == &DispatchExitTriggerLogic
            && cur2->m_smarts == playerIndex) {
            m_marker.m_x = cur2->m_screenX / TILE_SIZE_PX;
            m_marker.m_y = cur2->m_screenY / TILE_SIZE_PX;
            break;
        }
    }

    for (CGameObject* cur3 = mgr->m_world->ChildGroup()->FirstChild(); cur3 != NULL;
         cur3 = mgr->m_world->ChildGroup()->NextChild()) {
        if (cur3->GetLogicRecord()->GetDispatch() == &DispatchWayPointLogic
            && cur3->m_smarts == playerIndex) {
            Coord* slot = g_coordPool.Pop();
            slot->m_x = cur3->m_screenX >> TILE_SHIFT_PX;
            slot->m_y = cur3->m_screenY >> TILE_SHIFT_PX;
            m_attackWaypoints.Add(slot);
            cur3->AddFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
        }
    }

    switch (difficulty) {
        case BZDIFF_EASY: {
            g_buteMgr.GetInt("Battlez", "EasyDifficulty", 100);
            g_diffTier = 20;
            break;
        }
        case BZDIFF_NORMAL: {
            i32 r = g_buteMgr.GetInt("Battlez", "NormalDifficulty", 50);
            g_diffTier = 10;
            m_gruntCreationTime = static_cast<i32>(
                (static_cast<double>(r)
                 * (static_cast<double>(static_cast<u32>(m_gruntCreationTime)) * g_diffScale))
            );
            m_resourceCreationTime = static_cast<i32>(
                (static_cast<double>(r)
                 * (static_cast<double>(static_cast<u32>(m_resourceCreationTime)) * g_diffScale))
            );
            break;
        }
        case BZDIFF_HARD: {
            i32 r = g_buteMgr.GetInt("Battlez", "HardDifficulty", 25);
            g_diffTier = 5;
            m_gruntCreationTime = static_cast<i32>(
                (static_cast<double>(r)
                 * (static_cast<double>(static_cast<u32>(m_gruntCreationTime)) * g_diffScale))
            );
            m_resourceCreationTime = static_cast<i32>(
                (static_cast<double>(r)
                 * (static_cast<double>(static_cast<u32>(m_resourceCreationTime)) * g_diffScale))
            );
            break;
        }
        default:
            break;
    }

    m_spawnLastFire = 0;
    m_reserved14c = 0;
    {
        i32 rv = rand();
        m_reserved144 = ((rv % 4) + 5) * 125 * 8;
    }
    m_claimTimer = 0;
    m_defenderSearchRadiusX = 6;
    m_defenderSearchRadiusY = 6;
    m_idleRouteLimitX = 6;
    m_idleRouteLimitY = 6;
    m_defenderTargetMaxDistance = 8;
    m_idleBurnRandX = m_board->GetWidth() / 3;
    m_idleBurnRandY = m_board->GetWidth() / 3;
    m_assignedTargetMaxDistance = m_board->GetWidth() >> 2;
    m_roundRobinTick = 0;

    m_toolzPct = g_buteMgr.GetInt("Battlez", "ToolzPercent");
    m_toyzPct = m_toolzPct + g_buteMgr.GetInt("Battlez", "ToyzPercent");
    m_brickzPct = m_toyzPct + g_buteMgr.GetInt("Battlez", "BrickzPercent");
    m_redBrickPct = g_buteMgr.GetInt("Battlez", "RedBrick");
    m_blueBrickPct = m_redBrickPct + g_buteMgr.GetInt("Battlez", "BlueBrick");
    m_goldBrickPct = m_blueBrickPct + g_buteMgr.GetInt("Battlez", "GoldBrick");
    m_blackBrickPct = m_goldBrickPct + g_buteMgr.GetInt("Battlez", "BlackBrick");
    m_babyWalkerzPct = g_buteMgr.GetInt("Battlez", "BabyWalkerz");
    m_beachBallzPct = m_babyWalkerzPct + g_buteMgr.GetInt("Battlez", "BeachBallz");
    m_bigWheelzPct = m_beachBallzPct + g_buteMgr.GetInt("Battlez", "BigWheelz");
    m_goKartzPct = m_bigWheelzPct + g_buteMgr.GetInt("Battlez", "GoKartz");
    m_jackInTheBoxzPct = m_goKartzPct + g_buteMgr.GetInt("Battlez", "JackInTheBoxz");
    m_jumpRopezPct = m_jackInTheBoxzPct + g_buteMgr.GetInt("Battlez", "JumpRopez");
    m_pogoStickzPct = m_jumpRopezPct + g_buteMgr.GetInt("Battlez", "PogoStickz");
    m_scrollzPct = m_pogoStickzPct + g_buteMgr.GetInt("Battlez", "Scrollz");
    m_squeakToyzPct = m_scrollzPct + g_buteMgr.GetInt("Battlez", "SqueakToyz");
    m_yoyozPct = m_squeakToyzPct + g_buteMgr.GetInt("Battlez", "Yoyoz");

    m_bombzPct = g_buteMgr.GetInt("Battlez", "Bombz");
    m_boomerangzPct = m_bombzPct + g_buteMgr.GetInt("Battlez", "Boomerangz");
    m_toolBrickzPct = m_boomerangzPct + g_buteMgr.GetInt("Battlez", "Brickz");
    m_clubzPct = m_toolBrickzPct + g_buteMgr.GetInt("Battlez", "Clubz");
    m_gauntletzPct = m_clubzPct + g_buteMgr.GetInt("Battlez", "Gauntletz");
    m_glovezPct = m_gauntletzPct + g_buteMgr.GetInt("Battlez", "Glovez");
    m_gooberzPct = m_glovezPct + g_buteMgr.GetInt("Battlez", "Gooberz");
    m_gravityBootzPct = m_gooberzPct + g_buteMgr.GetInt("Battlez", "GravityBootz");
    m_gunHatzPct = m_gravityBootzPct + g_buteMgr.GetInt("Battlez", "GunHatz");
    m_nerfGunzPct = m_gunHatzPct + g_buteMgr.GetInt("Battlez", "NerfGunz");
    m_rockzPct = m_nerfGunzPct + g_buteMgr.GetInt("Battlez", "Rockz");
    m_shieldzPct = m_rockzPct + g_buteMgr.GetInt("Battlez", "Shieldz");
    m_shovelzPct = m_shieldzPct + g_buteMgr.GetInt("Battlez", "Shovelz");
    m_springzPct = m_shovelzPct + g_buteMgr.GetInt("Battlez", "Springz");
    m_spyzPct = m_springzPct + g_buteMgr.GetInt("Battlez", "Spyz");
    m_swordzPct = m_spyzPct + g_buteMgr.GetInt("Battlez", "Swordz");
    m_timeBombzPct = m_swordzPct + g_buteMgr.GetInt("Battlez", "TimeBombz");
    m_toobzPct = m_timeBombzPct + g_buteMgr.GetInt("Battlez", "Toobz");
    m_wandzPct = m_toobzPct + g_buteMgr.GetInt("Battlez", "Wandz");
    m_welderzPct = m_wandzPct + g_buteMgr.GetInt("Battlez", "Welderz");
    m_wingzPct = m_welderzPct + g_buteMgr.GetInt("Battlez", "Wingz");

    // Keep the timer reset in the accumulator caller's store sequence.
    m_routeTiming.m_start = 0;
    m_routeTiming.m_interval = 0;
    return 1;
}

RVA(0x00025c20, 0x55)
i32 CBattlezMapConfig::StepAllRowSpawns() {
    if (g_gameReg->m_players[m_playerIndex].IsHumanControlled() == false
        && g_gameReg->m_players[m_playerIndex].IsActive() != false) {
        for (i32 i = 0; i < m_candArray.GetSize(); i++) {
            this->StepRowSpawn(false);
        }
    }
    return 1;
}

RVA(0x00025ca0, 0xbf)
void CBattlezMapConfig::FreeArrays() {
    i32 i;
    for (i = 0; i < m_candArray.GetSize(); i++) {
        Coord* p = static_cast<Coord*>(m_candArray[i]);
        if (p != NULL) {
            g_coordPool.Push(p);
        }
    }
    m_candArray.RemoveAll();

    for (i = 0; i < GetAttackWaypointCount(); i++) {
        g_coordPool.Push(CoordAt(i));
    }
    m_attackWaypoints.RemoveAll();

    m_reserved104.RemoveAll();
    m_reserved118.RemoveAll();
    m_reserved13c = 0;
}

// @early-stop
RVA(0x00025d90, 0x580)
i32 CBattlezMapConfig::StepBoard() {
    if (m_active == false) {
        return 1;
    }
    if (m_ctx->GetTriggerMgr() == NULL) {
        return 0;
    }
    if (m_spawnTimer - m_spawnLastFire > m_gruntCreationTime) {
        StepRowSpawn(true);
        m_spawnLastFire = m_spawnTimer;
    }

    i32 mn = BATTLEZ_QUEUE_POSITION_UNSET;
    CGrunt** units = m_triggerMgr->PlayerUnits(m_playerIndex);
    for (i32 s = TM_UNITS_PER_PLAYER; s != 0; s--) {
        CGrunt* u = *units;
        if (u != NULL && u->GetAiState() == AISTATE_RETURN && u->GetDefenderQueuePosition() < mn) {
            mn = u->GetDefenderQueuePosition();
        }
        units++;
    }
    if (mn != 0 && mn != BATTLEZ_QUEUE_POSITION_UNSET) {
        for (i32 k = 0; k < TM_UNITS_PER_PLAYER; k++) {
            CGrunt* u = m_triggerMgr->UnitAt(m_playerIndex, k);
            if (u != NULL && u->GetAiState() == AISTATE_RETURN) {
                u->SetDefenderQueuePosition(u->GetDefenderQueuePosition() - mn);
            }
        }
    }

    i32 forced = 0;
    CGrunt* forcedUnit = NULL;
    if (m_repickTimer - m_repickLastFire > m_resourceCreationTime) {
        i32 r = rand() % TM_UNITS_PER_PLAYER;
        CGrunt* u = m_triggerMgr->UnitAt(m_playerIndex, r);
        forcedUnit = u;
        forced = 0;
        if (u != NULL && u->GetAiState() == AISTATE_RETURN && u->GetDefenderQueuePosition() == 0) {
            forced = 1;
        }
        if (!forced && rand() % 10 != 0) {
            i32 r2 = rand() % TM_UNITS_PER_PLAYER;
            CGrunt* u2 = m_triggerMgr->UnitAt(m_playerIndex, r2);
            if (u2 != NULL) {
                ChooseIdleBehavior(u2);
            }
        } else {
            for (i32 b = 0; b < TM_UNITS_PER_PLAYER; b++) {
                CGrunt* unit = m_triggerMgr->UnitAt(m_playerIndex, b);
                if (forced) {
                    unit = forcedUnit;
                }
                if (unit == NULL) {
                    continue;
                }
                CGameObject* lvl = unit->m_object;
                if (!(GRUNT_OBJECT_AT_SAVED_SCREEN_POS(lvl, unit))) {
                    continue;
                }
                if (unit->IsEntranceCommitted() == false) {
                    continue;
                }
                if (unit->IsDeathAnimationStarted() != false) {
                    continue;
                }
                if (unit->m_entranceActive != false) {
                    continue;
                }
                if (unit->IsInCombat() != false) {
                    continue;
                }
                bool eq;
                eq = unit->IsAnimationAct("I");
                if (eq) {
                    continue;
                }
                eq = unit->IsAnimationAct("G");
                if (eq) {
                    continue;
                }
                eq = unit->IsAnimationAct("L");
                if (eq) {
                    continue;
                }
                eq = unit->IsAnimationAct("P");
                if (eq) {
                    continue;
                }
                eq = unit->IsAnimationAct("J");
                if (eq) {
                    continue;
                }
                eq = unit->IsAnimationAct("C");
                if (eq) {
                    continue;
                }
                eq = unit->IsAnimationAct("R");
                if (eq) {
                    continue;
                }
                if (unit->GetAiState() != AISTATE_RETURN) {
                    continue;
                }
                if (unit->GetDefenderQueuePosition() != 0) {
                    continue;
                }

                PickupType mode = unit->GetDefenderPickupType();
                if (PathCrossesMarkedTile(unit) != 0) {
                    unit->SetAiState(AISTATE_RETREAT);
                } else {
                    unit->SetAiState(AISTATE_SEEK);
                }
                unit->BeginPickupAnimation(unit->GetDefenderPickupType(), 1, 0, 0, 1);

                switch (mode) {
                    case PICKUP_WINGZ: {
                        if (!unit->CoordsEmpty()) {
                            RECYCLE_GRUNT_COORDS(unit)
                        }
                        break;
                    }
                    case PICKUP_TOOB: {
                        if (!unit->CoordsEmpty()) {
                            RECYCLE_GRUNT_COORDS(unit)
                        }
                        break;
                    }
                }

                for (i32 c = 0; c < TM_UNITS_PER_PLAYER; c++) {
                    CGrunt* mate = m_triggerMgr->UnitAt(m_playerIndex, c);
                    if (mate != NULL && mate->GetAiState() == AISTATE_RETURN) {
                        i32 q = unit->GetDefenderQueuePosition() - 1;
                        if (q < 0) {
                            q = 0;
                        }
                        unit->SetDefenderQueuePosition(q);
                    }
                }
                return 1;
            }
        }
        m_repickLastFire = m_repickTimer;
    }
    StepRowUnits();
    m_spawnTimer += g_frameDelta;
    m_repickTimer += g_frameDelta;
    m_claimTimer += g_frameDelta;
    return 1;
}

RVA(0x00026470, 0x29d)
i32 CBattlezMapConfig::StepRowSpawn(b32 allowReserved) {
    i32 occupied = 0;
    CGrunt** units = m_triggerMgr->PlayerUnits(m_playerIndex);
    for (i32 unitsRemaining = TM_UNITS_PER_PLAYER; unitsRemaining != 0; unitsRemaining--) {
        if (*units != NULL) {
            occupied++;
        }
        units++;
    }
    if (occupied >= m_ctx->m_players[m_playerIndex].GetMaxGruntz()) {
        return 1;
    }
    i32 i = 0;
    Coord* cand = NULL;
    BrickzCell tileRec;
    for (; i < m_candArray.GetSize(); i++) {
        cand = static_cast<Coord*>(m_candArray.GetAt(i));
        if (cand != NULL) {

            tileRec = m_board->m_rows[cand->m_y][cand->m_x];
            b32 usable = true;
            if (tileRec.m_flags & BRICKZ_CELL_OCCUPIED) {

                if (tileRec.m_occupantIdBytes[1] == m_playerIndex) {
                    usable = false;
                }
                if (allowReserved == false) {
                    usable = false;
                }
            }
            if (usable) {
                goto candidateFound;
            }
        }
    }
    return 1;

candidateFound:
    Coord screen;
    m_ctx->m_world->GetLevel()->m_mainPlane->SnapToTileCenter(
        &screen,
        cand->m_x << TILE_SHIFT_PX,
        cand->m_y << TILE_SHIFT_PX
    );
    i32 cell;
    if (allowReserved != false) {
        cell = m_ctx->GetTriggerMgr()->PlaceObject(
            m_playerIndex,
            screen.m_x,
            screen.m_y,
            0x186a0,
            GRUNT_ENTRANCE_DROP,
            g_groupSentinel,
            0,
            0,
            0,
            0,
            0,
            0,
            NULL
        );
    } else {
        cell = m_ctx->GetTriggerMgr()->PlaceObject(
            m_playerIndex,
            screen.m_x,
            screen.m_y,
            0x186a0,
            GRUNT_ENTRANCE_NONE,
            g_groupSentinel,
            0,
            0,
            0,
            0,
            0,
            0,
            NULL
        );
    }
    if (cell == -1) {
        return 0;
    }

    CGrunt* unit = m_ctx->GetTriggerMgr()->UnitAt(m_playerIndex, cell);
    if (unit == NULL) {
        return 0;
    }

    i32 roll = GetRandom(0, 99);
    i32 freeCount = 0;
    CGrunt** r2 = m_triggerMgr->PlayerUnits(m_playerIndex);
    for (i32 k = TM_UNITS_PER_PLAYER; k != 0; k--) {
        CGrunt* g = *r2;
        if (g != NULL && g->GetBattlezTask() == BZTASK_UNASSIGNED) {
            freeCount++;
        }
        r2++;
    }
    i32 budget = static_cast<i32>(
        (static_cast<double>(m_ctx->m_players[m_playerIndex].GetMaxGruntz())
         * static_cast<double>(m_gruntRatio) * g_diffScale)
    );
    if (roll >= m_defenderChance || freeCount >= budget) {
        unit->SetBattlezTask(BZTASK_ADVANCE);
    } else {
        unit->SetBattlezTask(BZTASK_UNASSIGNED);
    }
    unit->m_aiType = AI_BATTLEZ_PATH;
    unit->SetAiState(AISTATE_SEEK);
    UNSET_COORD(unit->m_arrivalCell);
    UNSET_COORD(unit->m_unusedBattleCell);
    UNSET_COORD(unit->m_defenderPx);
    unit->SetTargetTeam(-1);
    unit->SetDefenderPickupType(PICKUP_NONE);
    unit->SetDefenderQueuePosition(0);
    unit->ResetDwell();
    unit->m_blockedVoicePending = true;
    return 1;
}

RVA(0x000267c0, 0x2850)
i32 CBattlezMapConfig::StepRowUnits() {
    m_roundRobinTick++;
    CGrunt* unit;
    i32 hit;
    char eq;
    i32 cell;
    Coord scratch;
    for (i32 i = 0; i < TM_UNITS_PER_PLAYER; i++) {
        unit = m_triggerMgr->UnitAt(m_playerIndex, i);
        if (unit != NULL) {
            if (unit->IsHoldPending()) {
                return 1;
            }
        }
        if (unit != NULL) {
            if (!unit->CoordsEmpty()) {
                Coord* hc = unit->GetHeadCoord();
                scratch.m_x = hc->m_x;
                scratch.m_x = m_board->m_width;
                scratch.m_y = hc->m_y;
            }
        }
        {
            {
                if (unit != NULL) {
                    if (!unit->IsArrivalRerollPending()) {
                        RouteToNearbyPickup(unit);
                        if (unit->IsInCombat() != false) {
                            eq = unit->IsAnimationAct("A");
                            if (eq) {
                                goto resetEntrance;
                            }
                        }
                        if (!unit->CoordsEmpty()) {
                            Coord* ac = unit->GetHeadCoord();
                            i32 ax = ac->m_x;
                            i32 ay = ac->m_y;
                            Coord sp;
                            (static_cast<CUserLogic*>(unit))->GetScreenTile((&sp));
                            if (sp.m_x == ax && sp.m_y == ay) {
                                goto arriveHead;
                            }
                        }
                        {
                            PickupType st = unit->GetEquippedToolType();
                            if (st == PICKUP_BRICK && unit->m_battleState == BZTASK_UNASSIGNED) {
                                unit->m_battleState = BZTASK_CARRY_BRICK;
                                if (!unit->CoordsEmpty()) {
                                    RECYCLE_GRUNT_COORDS_VIA_NEXTDATA(unit)
                                }
                            }
                        }
                        if (unit->IsAtSavedScreenPos() != 0 && unit->IsEntranceCommitted() != false
                            && unit->IsDeathAnimationStarted() == false
                            && unit->m_entranceActive == false && unit->IsInCombat() == false) {
                            if (BattlezActDiffersFromIGLPJCR(unit)) {
                                PickupType st2 = unit->GetEquippedToolType();
                                if (st2 == PICKUP_BRICK && unit->m_aiType == AI_DEFENDER
                                    && unit->m_aiState == AISTATE_BATTLEZ_ROUTE_TARGET) {
                                    unit->BeginPickupAnimation(PICKUP_NONE, 1, 0, 0, 1);
                                }
                            }
                        }
                        if (unit->m_battleState == BZTASK_SEEK_SWITCH) {
                            Coord s1;
                            (static_cast<CUserLogic*>(unit))->GetScreenPos((&s1));
                            s1.m_x >>= 5;
                            i32 qx = s1.m_x;
                            s1.m_y >>= 5;
                            i32 qy = s1.m_y;
                            Coord s2;
                            (static_cast<CUserLogic*>(unit))->GetScreenPos((&s2));
                            s2.m_y >>= 5;
                            s2.m_x >>= 5;
                            i32 tile = m_board->CellFlagsAt(s2.m_x, qy);
                            if (!(tile & 4)) {
                                UNSET_COORD(unit->m_arrivalCell);
                                unit->m_battleState = BZTASK_ADVANCE;
                                if (!unit->CoordsEmpty()) {
                                    RECYCLE_GRUNT_COORDS_VIA_NEXTDATA(unit)
                                }
                                unit->SetRoutePassableMask(0);
                                unit->m_aiState = AISTATE_SEEK;
                            }
                        }
                        {
                            PickupType st = unit->GetEquippedToolType();
                            if (st != PICKUP_SPY && unit->m_battleState == BZTASK_CARRY_SPY) {
                                UNSET_COORD(unit->m_arrivalCell);
                                unit->m_battleState = BZTASK_ADVANCE;
                                if (!unit->CoordsEmpty()) {
                                    RECYCLE_GRUNT_COORDS_VIA_NEXTDATA(unit)
                                }
                                unit->SetRoutePassableMask(0);
                                unit->m_aiState = AISTATE_SEEK;
                            }
                        }
                        {
                            PickupType st = unit->GetEquippedToolType();
                            if (st == PICKUP_GOOBER) {
                                BattlezTask battleTask = unit->m_battleState;
                                if (battleTask != BZTASK_CARRY_GOOBER
                                    && battleTask != BZTASK_ASSIGNED_TARGET) {
                                    if (!unit->CoordsEmpty()) {
                                        RECYCLE_GRUNT_COORDS_VIA_NEXTDATA(unit)
                                    }
                                    UNSET_COORD(unit->m_arrivalCell);
                                    unit->m_battleState = BZTASK_CARRY_GOOBER;
                                }
                            }
                        }
                        {
                            PickupType st = unit->GetEquippedToolType();
                            if (st != PICKUP_GOOBER && unit->m_battleState == BZTASK_CARRY_GOOBER) {
                                UNSET_COORD(unit->m_arrivalCell);
                                unit->m_battleState = BZTASK_ADVANCE;
                                if (!unit->CoordsEmpty()) {
                                    RECYCLE_GRUNT_COORDS_VIA_NEXTDATA(unit)
                                }
                                unit->SetRoutePassableMask(0);
                                unit->m_aiState = AISTATE_SEEK;
                            }
                        }
                        if (unit->CoordsEmpty()) {
                            if (unit->m_aiState == AISTATE_RETREAT) {
                                unit->m_aiState = AISTATE_SEEK;
                            }
                        }
                        if (unit->m_aiState == AISTATE_RETREAT) {
                            if (PathCrossesMarkedTile(unit) == 0) {
                                unit->m_aiState = AISTATE_SEEK;
                            }
                        }
                        {
                            if (BattlezActDiffersFromCRCGLPJ(unit)) {
                                if (unit->m_object->m_screenX == unit->m_lastTilePx.m_x
                                    && unit->m_object->m_screenY == unit->m_lastTilePx.m_y
                                    && unit->IsEntranceCommitted() != false
                                    && unit->IsDeathAnimationStarted() == false
                                    && unit->m_entranceActive == false) {
                                    RECT box;
                                    unit->BuildUnitSearchBox(&box, 4);
                                    Coord c5;
                                    (static_cast<CUserLogic*>(unit))->GetScreenTile((&c5));
                                    Coord c6;
                                    (static_cast<CUserLogic*>(unit))->GetScreenTile((&c6));
                                    Coord c7;
                                    (static_cast<CUserLogic*>(unit))->GetScreenPos((&c7));
                                    c7.m_y >>= 5;
                                    c7.m_x >>= 5;
                                    Coord c8;
                                    (static_cast<CUserLogic*>(unit))->GetScreenTile((&c8));
                                    i32 rowEnd = c5.m_y + 2;
                                    i32 colEnd = c6.m_x + 2;
                                    i32 rowBeg = c7.m_y - 1;
                                    i32 colBeg = c8.m_x - 1;
                                    CMapMgr* board = m_board;
                                    board->Clip(&box);
                                    for (i32 row = rowBeg; row < rowEnd; row++) {
                                        CMapMgr* b = m_board;
                                        for (i32 col = colBeg; col < colEnd; col++) {
                                            if (static_cast<u32>(col) < b->m_width
                                                && static_cast<u32>(row) < b->m_height) {
                                                if (b->CellFlagsAtUnchecked(col, row)
                                                    & IDX(CELL_FLAG_TIME_BOMB)) {
                                                    goto perimSweep;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    reclampJoin: {
                        CMapMgr* bd = m_board;
                        bd->Clip(NULL);
                    }
                        {
                            i32 special = 1;
                            if (GRUNT_NOT_AT_SAVED_SCREEN_POS(unit)) {
                                special = 0;
                            }
                            if (unit->IsEntranceCommitted() == false) {
                                special = 0;
                            }
                            if (unit->IsDeathAnimationStarted() != false) {
                                special = 0;
                            }
                            if (unit->m_entranceActive != false) {
                                special = 0;
                            }
                            if (!UpdateBattlezSpecialEligibility(unit, special)) {
                                return 0;
                            }
                            if (unit->GetPowerupType() == GRUNT_GHOST) {
                                special = 0;
                            }
                            if (special != 0) {
                                if (unit->IsInCombat() != false && unit->m_attackQueued == false
                                    && unit->m_attackWindupActive == false
                                    && unit->m_stamina >= STAMINA_FULL) {
                                    if (unit->FindGridNeighbor(0) != NULL) {
                                        return 1;
                                    }
                                }
                            }
                        }
                        if (unit->IsAtSavedScreenPos() != 0 && unit->IsEntranceCommitted() != false
                            && unit->IsDeathAnimationStarted() == false
                            && unit->m_entranceActive == false && unit->IsInCombat() == false) {
                            if (BattlezActDiffersFromIGLPJCR(unit)) {
                                for (i32 j = 0; j < 4; j++) {
                                    if (j != m_playerIndex) {
                                        for (i32 k = 0; k < TM_UNITS_PER_PLAYER; k++) {
                                            CGrunt* other = m_triggerMgr->UnitAt(j, k);
                                            if (other != NULL) {
                                                if (unit->RectContains(
                                                        other->m_object->m_screenX,
                                                        other->m_object->m_screenY
                                                    )
                                                    != 0) {
                                                    if (unit->GetPowerupType() != PICKUP_GHOST) {
                                                        if (other->IsInCombat() == false) {
                                                            if (HandleUnitContact(unit, other)
                                                                != 0) {
                                                                return 1;
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                                hit = 0;
                            }
                        }
                    }
                }
            }
        }
        hit = 0;
        if (unit != NULL) {
            if (!unit->IsArrivalRerollPending()) {
                BattlezTask battleTask = unit->m_battleState;
                if (battleTask != BZTASK_ASSIGNED_TARGET && battleTask != BZTASK_SEEK_SWITCH) {
                    if (unit->IsEntranceCommitted() != false
                        && unit->IsDeathAnimationStarted() == false
                        && unit->m_entranceActive == false && unit->IsInCombat() == false) {
                        if (BattlezActDiffersFromIGLPJCR(unit)) {
                            if (unit->m_battleState != BZTASK_UNASSIGNED) {
                                if (RouteToNearbyEnemy(unit) != 0) {
                                    hit = 1;
                                }
                            }
                        }
                    }
                }
            }
        }
        if (unit != NULL) {
            if (GRUNT_AT_SAVED_SCREEN_POS(unit) && unit->IsEntranceCommitted() != false
                && unit->IsDeathAnimationStarted() == false && unit->m_entranceActive == false
                && unit->IsInCombat() == false) {
                if (BattlezActDiffersFromIGLPJCR(unit)) {
                    if (static_cast<u32>(m_roundRobinTick) % TM_UNITS_PER_PLAYER
                        == static_cast<u32>(i)) {
                        {
                            PickupType st3 = unit->GetEquippedToolType();
                            if (st3 == PICKUP_WAND && unit->m_health > 0x1a) {
                                if (rand() % g_diffTier == 0) {
                                    i32 r = g_buteMgr.GetInt("Spellz", "SpellRadius", 8);
                                    RECT spell;
                                    i32 px = unit->m_object->m_screenX;
                                    i32 py = unit->m_object->m_screenY;
                                    SET_RECT_COMPONENTS(
                                        spell,
                                        (px >> TILE_SHIFT_PX) - r,
                                        (py >> TILE_SHIFT_PX) - r,
                                        (px >> TILE_SHIFT_PX) + r,
                                        (py >> TILE_SHIFT_PX) + r
                                    );
                                    for (i32 j2 = 0; j2 < 4; j2++) {
                                        if (j2 != m_playerIndex) {
                                            for (i32 k2 = 0; k2 < TM_UNITS_PER_PLAYER; k2++) {
                                                CGrunt* o = m_triggerMgr->UnitAt(j2, k2);
                                                if (o != NULL) {
                                                    POINT pt;
                                                    SET_POINT_COMPONENTS(
                                                        pt,
                                                        o->m_object->m_screenX >> TILE_SHIFT_PX,
                                                        o->m_object->m_screenY >> TILE_SHIFT_PX
                                                    );
                                                    if (PtInRect(&spell, pt) != false) {
                                                        goto spellHit;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        if (PathToNearbyUnit(unit) != 0) {
                            return 1;
                        }
                        if (unit->CoordsEmpty() && unit->m_aiState == AISTATE_COOLDOWN) {
                            UNSET_COORD(unit->m_unusedBattleCell);
                            unit->m_aiState = AISTATE_SEEK;
                        }
                        {
                            char nd;
                            nd = unit->IsNotAnimationAct("D");
                            if (nd) {
                                ResolveArrival(unit);
                            }
                        }
                        if (unit->m_object->m_screenX == unit->m_lastTilePx.m_x
                            && unit->m_object->m_screenY == unit->m_lastTilePx.m_y
                            && unit->IsEntranceCommitted() != false
                            && unit->IsDeathAnimationStarted() == false
                            && unit->m_entranceActive == false && unit->IsInCombat() == false) {
                            if (BattlezActDiffersFromIGLPJCR(unit)) {
                                goto dispatch;
                            }
                        }
                    }
                }
            }
        }
        continue;
    dispatch: {
        CMapMgr* bd2 = m_board;
        bd2->Clip(NULL);
        PickupType stX = unit->m_activePickupType;
        if (hit == 0) {
            switch (unit->m_battleState) {
                case BZTASK_UNASSIGNED: {
                    StepDefenderUnit(unit);
                    break;
                }
                case BZTASK_STEP: {
                    Step(unit);
                    break;
                }
                case BZTASK_ASSIGNED_TARGET: {
                    TrackAssignedEnemy(unit);
                    break;
                }
                case BZTASK_ADVANCE: {
                    AdvanceToEnemyBase(unit);
                    break;
                }
                case BZTASK_CARRY_GOOBER: {
                    RepathToFreeCell(unit);
                    break;
                }
                case BZTASK_CHECK_QUEUED_SPAWN: {
                    CheckQueuedSpawnTile(unit);
                    break;
                }
                case BZTASK_CARRY_SPY: {
                    RetargetIdleUnit(unit);
                    break;
                }
                case BZTASK_SEEK_SWITCH: {
                    RerouteSwitchSeeker(unit);
                    break;
                }
                case BZTASK_CARRY_BRICK: {
                    ScanRegion(unit);
                    break;
                }
                default:
                    break;
            }
        }
        if (!unit->CoordsEmpty()) {
            eq = unit->IsAnimationAct("A");
            if (eq) {
                Coord* gc = unit->GetHeadCoord();
                i32 gx = gc->m_x;
                i32 gy = gc->m_y;
                i32 sx = unit->m_object->m_screenX >> TILE_SHIFT_PX;
                i32 sy = unit->m_object->m_screenY >> TILE_SHIFT_PX;
                if (abs(gx - sx) >= 2 || abs(gy - sy) >= 2) {
                    goto dropCoords;
                }
                {
                    cell = m_board->CellFlagsAtUnchecked(gx, gy);
                    i32 f;
                    f = unit->m_arrivalFlags & cell;
                    if (f & BRICKZ_CELL_OCCUPIED) {
                        goto wingzGate;
                    }
                    if (f != 0 && (cell & unit->m_passableMask) == 0) {
                        goto wingzGate;
                    }
                    if (cell & BRICKZ_CELL_OCCUPIED) {
                        goto wingzGate;
                    }
                    if ((cell & IDX(CELL_FLAG_REVEALED_POWERUP)) == 0) {
                        goto flagsArm;
                    }
                wingzGate: {
                    PickupType wp = unit->GetEquippedToolType();
                    if (wp != PICKUP_WINGZ) {
                        continue;
                    }
                }
                    if ((cell & IDX(CELL_FLAG_SPECIAL)) == 0 && (cell & 0x100) == 0) {
                        continue;
                    }
                    if ((cell & BRICKZ_CELL_OCCUPIED) == 0) {
                        goto tailArm2;
                    }
                    continue;
                dropCoords:
                    if (!unit->CoordsEmpty()) {
                        RECYCLE_GRUNT_COORDS(unit)
                    }
                }
            }
        }
    }
    }
    return 1;

resetEntrance: {
    b32 pw = unit->m_inCombat;
    unit->m_attackQueued = false;
    if (pw == false) {
        return 1;
    }
    RESET_GRUNT_COMBAT_STATE(unit)
    return 1;
}

arriveHead:
    if (!unit->CoordsEmpty()) {
        RECYCLE_GRUNT_COORDS_VIA_NEXTDATA(unit)
    }
    return 1;

perimSweep: {
    Coord q0;
    (static_cast<CUserLogic*>(unit))->GetScreenPos((&q0));
    i32 col = (q0.m_x >> TILE_SHIFT_PX) - 2;
    (static_cast<CUserLogic*>(unit))->GetScreenTile((&scratch));
    while (col < scratch.m_x + 3) {
        Coord qa;
        GET_SCREEN_TILE_Y_FIRST(static_cast<CUserLogic*>(unit), qa)
        i32 rt = qa.m_y - 2;
        if (static_cast<u32>(col) < m_board->m_width && static_cast<u32>(rt) < m_board->m_height) {
            if (unit->TileSwitch(col, rt, 0, 0x2000098b, 1, 0) != 0) {
                goto topRowProbeHit;
            }
        }
        Coord qc;
        GET_SCREEN_TILE_Y_FIRST(static_cast<CUserLogic*>(unit), qc)
        i32 rb = qc.m_y + 2;
        if (static_cast<u32>(col) < m_board->m_width && static_cast<u32>(rb) < m_board->m_height) {
            if (unit->TileSwitch(col, rb, 0, 0x2000098b, 1, 0) != 0) {
                goto bottomRowProbeHit;
            }
        }
        col++;
        (static_cast<CUserLogic*>(unit))->GetScreenTile((&scratch));
    }
    {
        Coord u0;
        (static_cast<CUserLogic*>(unit))->GetScreenPos((&u0));
        i32 row = (u0.m_y >> TILE_SHIFT_PX) - 2;
        (static_cast<CUserLogic*>(unit))->GetScreenTile((&scratch));
        while (row < scratch.m_y + 3) {
            Coord ua;
            (static_cast<CUserLogic*>(unit))->GetScreenTile((&ua));
            i32 xl = ua.m_x - 2;
            if (static_cast<u32>(xl) < m_board->m_width
                && static_cast<u32>(row) < m_board->m_height) {
                if (unit->TileSwitch(xl, row, 0, 0x2000098b, 1, 0) != 0) {
                    goto firstColumnProbeHit;
                }
            }
            Coord uc;
            (static_cast<CUserLogic*>(unit))->GetScreenPos((&uc));
            uc.m_y >>= 5;
            uc.m_x >>= 5;
            if (static_cast<u32>(uc.m_x + 2) < m_board->m_width
                && static_cast<u32>(row) < m_board->m_height) {

                if (unit->TileSwitch(xl, row, 0, 0x2000098b, 1, 0) != 0) {
                    goto secondColumnProbeHit;
                }
            }
            row++;
            (static_cast<CUserLogic*>(unit))->GetScreenTile((&scratch));
        }
    }
    {
        CMapMgr* fb = m_board;
        fb->Clip(NULL);
        return 1;
    }
}

// These four reset/start expansions share the caller's nested inline population.
topRowProbeHit: {
    unit->m_arrivalRerollTiming.m_startLo = 0;
    unit->m_arrivalRerollTiming.m_intervalLo = 0;
    unit->m_arrivalRerollTiming.m_startHi = 0;
    unit->m_arrivalRerollTiming.m_intervalHi = 0;
    unit->m_arrivalRerollTiming.m_intervalLo = 0x1f40;
    unit->m_arrivalRerollTiming.m_intervalHi = 0;
    unit->m_arrivalRerollTiming.m_startLo = g_frameTime;
    unit->m_arrivalRerollTiming.m_startHi = 0;
    CMapMgr* hb = m_board;
    hb->Clip(NULL);
    return 1;
}

bottomRowProbeHit: {
    unit->m_arrivalRerollTiming.m_startLo = 0;
    unit->m_arrivalRerollTiming.m_intervalLo = 0;
    unit->m_arrivalRerollTiming.m_startHi = 0;
    unit->m_arrivalRerollTiming.m_intervalHi = 0;
    unit->m_arrivalRerollTiming.m_intervalLo = 0x1f40;
    unit->m_arrivalRerollTiming.m_intervalHi = 0;
    unit->m_arrivalRerollTiming.m_startLo = g_frameTime;
    unit->m_arrivalRerollTiming.m_startHi = 0;
    CMapMgr* hb = m_board;
    hb->Clip(NULL);
    return 1;
}

spellHit: {
    i32 hx = unit->m_lastTilePx.m_x;
    i32 hy = unit->m_lastTilePx.m_y;
    m_triggerMgr->UseEquippedToolAt(unit->m_playerIndex, unit->m_unitIndex, hx, hy);
    return 1;
}

flagsArm: {
    b32 ok = true;
    if (cell & 8) {
        PickupType er = unit->m_activePickupType;
        PickupType held = unit->ResolveEquippedToolType(er);
        if (held != PICKUP_TOOB) {
            PickupType held2 = unit->ResolveEquippedToolType(er);
            if (held2 != PICKUP_WINGZ) {
                ok = false;
            }
        }
    }
    if (cell & 0x200) {
        PickupType er = unit->m_activePickupType;
        PickupType held = unit->ResolveEquippedToolType(er);
        if (held != PICKUP_TOOB) {
            PickupType held2 = unit->ResolveEquippedToolType(er);
            if (held2 != PICKUP_WINGZ) {
                ok = false;
            }
        }
    }
    if (ok == false) {
        return 1;
    }
    {
        Coord* tc = unit->GetTailCoord();
        SET_TILE_CENTER_PIXEL_PAIR(unit->m_entrancePx.m_x, unit->m_entrancePx.m_y, tc->m_x, tc->m_y)
        unit->StepEntranceReinit();
        return 1;
    }
}

tailArm2: {
    Coord* tc = unit->GetTailCoord();
    SET_TILE_CENTER_PIXEL_PAIR(unit->m_entrancePx.m_x, unit->m_entrancePx.m_y, tc->m_x, tc->m_y)
    unit->StepEntranceReinit();
    return 1;
}

firstColumnProbeHit: {
    unit->m_arrivalRerollTiming.m_startLo = 0;
    unit->m_arrivalRerollTiming.m_intervalLo = 0;
    unit->m_arrivalRerollTiming.m_startHi = 0;
    unit->m_arrivalRerollTiming.m_intervalHi = 0;
    unit->m_arrivalRerollTiming.m_intervalLo = 0x1f40;
    unit->m_arrivalRerollTiming.m_intervalHi = 0;
    unit->m_arrivalRerollTiming.m_startLo = g_frameTime;
    unit->m_arrivalRerollTiming.m_startHi = 0;
    CMapMgr* hb = m_board;
    hb->Clip(NULL);
    return 1;
}

secondColumnProbeHit: {
    unit->m_arrivalRerollTiming.m_startLo = 0;
    unit->m_arrivalRerollTiming.m_intervalLo = 0;
    unit->m_arrivalRerollTiming.m_startHi = 0;
    unit->m_arrivalRerollTiming.m_intervalHi = 0;
    unit->m_arrivalRerollTiming.m_intervalLo = 0x1f40;
    unit->m_arrivalRerollTiming.m_intervalHi = 0;
    unit->m_arrivalRerollTiming.m_startLo = g_frameTime;
    unit->m_arrivalRerollTiming.m_startHi = 0;
    CMapMgr* hb = m_board;
    hb->Clip(NULL);
    return 1;
}
}

RVA(0x00029a50, 0x15)
void CUserLogic::GetScreenPos(Coord* out) {
    CWwdSpriteObject* o = m_object;
    i32 y = o->m_screenY;
    i32 x = o->m_screenX;
    out->Set(x, y);
}

RVA(0x00029a80, 0x29)
i32 CGrunt::IsAtSavedScreenPos() {
    return IsGruntAtSavedScreenPos(this);
}

RVA(0x00029af0, 0x3b)
void CBattlezMapConfig::RerouteIdleUnit(
    CGrunt* unit,
    i32 col,
    i32 row,
    i32 burnFirstRandom,
    i32 burnSecondRandom,
    i32 unused
) {
    if (burnFirstRandom) {
        rand();
    }
    if (burnSecondRandom) {
        rand();
    }
    unit->TileSwitch(col, row, 0, 0x9c7, 0, 0);
}

RVA(0x00029b40, 0x813)
i32 CBattlezMapConfig::ValidateUnitPath(CGrunt* unit) {
    CPtrList* coordList = unit->GetCoordList();
    if (unit->CoordsEmpty()) {
        goto returnZero;
    }

    {
        Coord* c0 = unit->GetHeadCoord();
        i32 ux = c0->m_x;
        i32 uy = c0->m_y;
        Coord pt;
        (static_cast<CUserLogic*>(unit))->GetScreenPos((&pt));
        i32 gx = pt.m_x >> TILE_SHIFT_PX;
        (static_cast<CUserLogic*>(unit))->GetScreenPos((&pt));
        i32 gy = pt.m_y >> TILE_SHIFT_PX;
        i32 dx = abs(ux - gx);
        i32 dy = abs(uy - gy);
        if (dx >= 2 || dy >= 2) {
            goto recycleBail;
        }

        i32 tile0 = m_board->CellFlagsAt(ux, uy);
        if (static_cast<u8>(tile0) == 1) {
            unit->RecycleCoords();
            return 0;
        }

        POSITION head = coordList->GetHeadPosition();
        Coord* firstCoord = unit->GetCoordAt(head);
        BrickzCell pathHeadCell = m_board->CellAt(firstCoord->m_x, firstCoord->m_y);
        if (unit->CoordsEmpty()) {
            goto returnZero;
        }
        Coord* pathHead = unit->GetHeadCoord();
        i32 cx = pathHead->m_x;
        i32 cy = pathHead->m_y;
        (static_cast<CUserLogic*>(unit))->GetScreenPos((&pt));
        pathHeadCell = m_board->CellAt(cx, cy);
        PickupType prim = EQUIPPED_TOOL_TERNARY_LE(unit);

        Coord pt2;
        (static_cast<CUserLogic*>(unit))->GetScreenTile((&pt2));
        i32 sgy = pt2.m_y;
        (static_cast<CUserLogic*>(unit))->GetScreenTile((&pt));
        i32 sgx = pt.m_x;
        BrickzCell currentCell = m_board->CellAt(sgx, sgy);

        if ((currentCell.m_flags & 0x4) && unit->GetBattlezTask() != BZTASK_SEEK_SWITCH) {
            (static_cast<CUserLogic*>(unit))->GetScreenTile((&pt));
            i32 rx = pt.m_x;
            (static_cast<CUserLogic*>(unit))->GetScreenTile((&pt2));
            i32 ry = pt2.m_y;
            CTileTriggerSwitchLogic* rec =
                m_cellQuery->FindSwitchLogic(CellKey(rx, ry), TRIGID_ANY);
            if (rec->GetType() == TRIGID_SWITCH_2) {
                unit->SetAiState(AISTATE_SEEK);
                unit->RecycleCoords();
                unit->SetBattlezTask(BZTASK_SEEK_SWITCH);
                unit->ResetDwell();
                return 0;
            }
        }

        PickupType entranceMode = unit->GetEquippedToolType();
        if (entranceMode == PICKUP_TIMEBOMB && unit->CoordCount() >= 2) {
            POSITION node = unit->CoordHead();
            Coord* ca = unit->GetCoordAt(node);
            POSITION nn = node;
            unit->GetNextCoord(nn);
            i32 ax = ca->m_x;
            Coord* cb = unit->GetCoordAt(nn);
            i32 ay = ca->m_y;
            i32 bx = cb->m_x;
            i32 by = cb->m_y;
            i32 secondCellFlags = m_board->CellFlagsAt(bx, by);
            if (secondCellFlags & 0x20) {
                i32 firstCellFlags = m_board->CellFlagsAt(ax, ay);
                if (!(firstCellFlags & 0x2)) {
                    m_triggerMgr->UseEquippedToolAt(
                        unit->GetPlayerIndex(),
                        unit->GetUnitIndex(),
                        ax * 0x20 + 0x10,
                        ay * 0x20 + 0x10
                    );
                    return 0;
                }
            }
        }

        if ((currentCell.m_flags & IDX(CELL_FLAG_HIDDEN_POWERUP))
            && unit->GetAiState() == AISTATE_RETURN) {
            unit->SetAiState(AISTATE_SEEK);
        }
        i32 pathHeadFlags = pathHeadCell.m_flags;
        if ((pathHeadFlags & IDX(CELL_FLAG_HIDDEN_POWERUP)) && prim == PICKUP_BRICK
            && unit->GetBattlezTask() == BZTASK_CARRY_BRICK) {
            m_triggerMgr->UseEquippedToolAt(
                unit->GetPlayerIndex(),
                unit->GetUnitIndex(),
                cx * 0x20 + 0x10,
                cy * 0x20 + 0x10
            );
            unit->SetAiState(AISTATE_SEEK);
            unit->RecycleCoords();
            return 0;
        }
        if ((pathHeadFlags & IDX(CELL_FLAG_HIDDEN_POWERUP)) && PathCrossesMarkedTile(unit) == 0
            && unit->GetAiState() == AISTATE_BATTLEZ_FINAL_ROUTE) {
            POSITION head = unit->CoordHead();
            if (head != NULL) {
                POSITION n = head;
                unit->GetNextCoord(n);
                if (n != NULL) {
                    while (n != NULL) {
                        POSITION cur = n;
                        unit->GetNextCoord(n);
                        if (unit->GetCoordAt(cur) != NULL) {
                            g_coordPool.Push(unit->GetCoordAt(cur));
                            unit->RemoveCoordAt(cur);
                        }
                    }
                    return 1;
                }
            }
        }

        if (pathHeadFlags & 0x200) {
            PickupType p = unit->GetEquippedToolType();
            if (p != PICKUP_WINGZ) {
                goto returnZero;
            }
        }
        if (pathHeadFlags & 0x8) {
            i32 wingzOrToobGate = pathHeadFlags & 0x100;
            if (wingzOrToobGate) {
                PickupType er = unit->m_activePickupType;
                PickupType p = unit->ResolveEquippedToolType(er);
                if (p == PICKUP_WINGZ) {
                    goto returnOne;
                }
                PickupType entranceMode2 = unit->ResolveEquippedToolType(er);
                if (entranceMode2 == PICKUP_TOOB) {
                    return 1;
                }
            }
            i32 wingzGate = pathHeadFlags & IDX(CELL_FLAG_SPECIAL);
            if (wingzGate) {
                PickupType p = unit->GetEquippedToolType();
                if (p == PICKUP_WINGZ) {
                    return 1;
                }
            }
            if (PathToNearestGoal(unit, cx, cy) != 0) {
                goto returnOne;
            }
            i32 currentCellFlags = currentCell.m_flags;
            if ((currentCellFlags & 0x200) || (currentCellFlags & 0x8)) {
                goto returnZero;
            }
            if (wingzOrToobGate && unit->GetAiState() != AISTATE_RETURN) {
                if (rand() % 5) {
                    EnterDefenderMode(unit, 0x12);
                } else {
                    EnterDefenderMode(unit, 0x16);
                }
            }
            if (wingzGate) {
                if (unit->GetAiState() == AISTATE_RETURN) {
                    goto returnZero;
                }
                EnterDefenderMode(unit, 0x16);
                return 0;
            }
            goto returnZero;
        }

        if ((pathHeadFlags & 0x20) && prim != PICKUP_GAUNTLETZ && prim != PICKUP_TIMEBOMB
            && prim != PICKUP_BOMB) {
            if (unit->GetAiState() == AISTATE_RETURN) {
                goto returnZero;
            }
            EnterDefenderMode(unit, 5);
            return 0;
        }
        if (pathHeadFlags & IDX(CELL_FLAG_REVEALED_POWERUP)) {
            PickupType p = unit->GetEquippedToolType();
            if (p != PICKUP_WINGZ) {
                if (prim == PICKUP_SHOVEL) {
                    goto returnZero;
                }
                if (unit->GetAiState() == AISTATE_RETURN) {
                    goto returnZero;
                }
                EnterDefenderMode(unit, 0xd);
                return 0;
            }
        }
        if (pathHeadFlags & IDX(CELL_FLAG_SPECIAL)) {
            PickupType p = unit->GetEquippedToolType();
            if (p != PICKUP_WINGZ) {
                goto returnZero;
            }
        }
        if (pathHeadFlags & BRICKZ_CELL_OCCUPIED) {
            return RepathAroundBlockedTiles(unit);
        }
        PickupType pk = unit->GetEquippedToolType();
        if (pk == PICKUP_GOOBER) {
            POSITION opos = m_triggerMgr->GetPuddleHeadPosition();
            while (opos != NULL) {
                CGruntPuddle* cand = m_triggerMgr->GetNextPuddle(opos);
                if (cand->IsPending() == false) {
                    i32 ox = cand->GetTileX();
                    i32 oy = cand->GetTileY();
                    if ((static_cast<CGrunt*>(unit))
                            ->RectContains(ox * 0x20 + 0x10, oy * 0x20 + 0x10)
                        != 0) {
                        m_triggerMgr->UseEquippedToolAt(
                            unit->GetPlayerIndex(),
                            unit->GetUnitIndex(),
                            ox * 0x20 + 0x10,
                            oy * 0x20 + 0x10
                        );
                        unit->RecycleCoords();
                        m_spawnTimer +=
                            static_cast<i32>((static_cast<u32>(m_gruntCreationTime) >> 2));
                        return 1;
                    }
                }
            }
            return 1;
        }
        goto returnOne;
    }
returnOne:
    return 1;
recycleBail:
    unit->RecycleCoords();
returnZero:
    return 0;
}

RVA(0x0002a570, 0x4c6)
i32 CBattlezMapConfig::RepathAroundBlockedTiles(CGrunt* unit) {
    CPtrList* coordList = unit->GetCoordList();
    if (coordList->IsEmpty()) {
        return 1;
    }
    POSITION node = coordList->GetHeadPosition();
    Coord center = unit->ScanCell();
    CMapMgr* board = m_board;
    {
        CRect box(center.m_x - 6, center.m_y - 6, center.m_x + 6, center.m_y + 6);
        board->Clip(&box);
    }
    Coord tail = *unit->GetTailCoord();
    u32 iter = 0;
    coordList->GetNext(node);
    while (node != NULL && iter < 3) {
        Coord* coord = static_cast<Coord*>(coordList->GetNext(node));
        if (coord == NULL) {
            continue;
        }
        i32 x = coord->m_x;
        i32 y = coord->m_y;
        if ((m_board->CellFlagsAtUnchecked(x, y) & 1) != 0 && (x != tail.m_x || y != tail.m_y)) {
            continue;
        }
        CPtrList list(10);
        i32 flags = 0;
        PickupType er = unit->m_activePickupType;
        if (unit->ResolveEquippedToolType(er) == PICKUP_TOOB) {
            flags = BATTLEZ_ROUTE_TOOB_TRAVERSAL;
        }
        if (unit->ResolveEquippedToolType(er) == PICKUP_WINGZ) {
            flags = BATTLEZ_ROUTE_WINGZ_TRAVERSAL;
        }
        if (unit->ResolveEquippedToolType(er) == PICKUP_SPRING) {
            flags = BATTLEZ_ROUTE_SPRING_TRAVERSAL;
        }
        if (m_board->FindPathWithEndpointOverrides(
                center.m_x,
                center.m_y,
                coord->m_x,
                coord->m_y,
                &list,
                1,
                0x2000098f,
                flags
            ) != 0
            && !list.IsEmpty()) {
            RECYCLE_HEAD_COORD(list)
            if (!list.IsEmpty()) {
                while (node != NULL) {
                    Coord* remaining = static_cast<Coord*>(coordList->GetNext(node));
                    list.AddTail(g_coordPool.PopCopy(*remaining));
                }

                unit->RecycleCoords();

                POSITION qp = list.GetHeadPosition();
                while (qp != NULL) {
                    Coord* c3 = static_cast<Coord*>(list.GetNext(qp));
                    if (c3 != NULL && (*c3 != center)) {
                        coordList->AddTail(c3);
                    }
                }

                m_board->Clip(NULL);
                Coord* nt = unit->GetTailCoord();
                Coord entrance;
                SET_TILE_CENTER_PIXEL_PAIR(entrance.m_x, entrance.m_y, nt->m_x, nt->m_y)
                unit->m_entrancePx = entrance;
                return 1;
            }
        }
        iter++;
    }

    {
        m_board->Clip(NULL);
    }
    return 0;
}

RVA(0x0002ab80, 0x15e)
CGrunt* CBattlezMapConfig::FindIdleGruntInBox(i32 cx, i32 cy, i32 halfW, i32 halfH) {
    RECT rect;
    SET_RECT_COMPONENTS(rect, cx - halfW, cy - halfH, cx + halfW, cy + halfH);
    CGrunt* best = NULL;
    i32 bestDist = INT_MAX;
    for (i32 band = 0; band < 4; band++) {
        if (band == m_playerIndex) {
            continue;
        }
        for (i32 i = 0; i < TM_UNITS_PER_PLAYER; i++) {
            CGrunt* u = m_triggerMgr->UnitAt(band, i);
            if (u == NULL) {
                continue;
            }
            if (u->IsSpawnProtected() != false) {
                continue;
            }
            Coord tile = ScreenTile(u);
            POINT wpt;
            wpt.y = tile.m_y;
            wpt.x = tile.m_x;
            if (!PtInRect(&rect, wpt)) {
                continue;
            }
            i32 keep = 1;
            if (u->GetPowerupType() == GRUNT_GHOST) {
                if (GetRandom(0, 99) > 5) {
                    keep = 0;
                }
            }
            if (keep == 0) {
                continue;
            }
            i32 dx = abs(u->GetScreenTileX() - cx);
            i32 dy = abs(u->GetScreenTileY() - cy);
            i32 dist = dx + dy;
            if (dist >= bestDist) {
                continue;
            }
            if (u->IsInCombat() != false) {
                rand();
            }
            best = u;
            bestDist = dist;
        }
    }
    return best;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0002ad40, 0x71)
CGrunt* CBattlezMapConfig::PickRandomIdleUnit(i32) {
    i32 band = rand() % 4;
    if (band == m_playerIndex) {
        band++;
    }
    band = band % 4;
    i32 cell = rand() % TM_UNITS_PER_PLAYER;
    for (i32 i = 0; i < TM_UNITS_PER_PLAYER; i++) {
        CGrunt* u = m_triggerMgr->UnitAt(band, i);
        if (u != NULL && u->IsSpawnProtected() == false) {
            return u;
        }
        cell = (cell + 1) % TM_UNITS_PER_PLAYER;
    }
    return NULL;
}

RVA(0x0002ade0, 0x7)
void CBattlezMapConfig::Clear() {
    m_active = false;
}

RVA(0x0002ae00, 0x42e)
i32 CBattlezMapConfig::HandleUnitContact(CGrunt* actor, CGrunt* other) {
    if (other->IsEntranceCommitted() == false) {
        return 0;
    }
    if (other->IsAnimationAct("J")) {
        return 0;
    }
    if (other->IsAnimationAct("C")) {
        return 0;
    }
    if (other->IsAnimationAct("R")) {
        return 0;
    }
    if (other->IsAnimationAct("G")) {
        return 0;
    }
    if (other->IsAnimationAct("L")) {
        return 0;
    }
    if (other->GetPowerupType() == GRUNT_GHOST) {
        return 0;
    }
    if (other->IsSpawnProtected() != false) {
        return 0;
    }
    i32 roll = rand() % 4;
    if (actor->GetCarriedToyType() != PICKUP_NONE && roll == 0) {
        CGameObject* ul = other->m_object;
        if ((static_cast<CGrunt*>(actor))->IsInToyUseRange(ul->m_screenX, ul->m_screenY) != 0) {
            if (actor->GetCarriedToyType() == PICKUP_SCROLL) {
                CGameObject* tl = actor->m_object;
                m_triggerMgr->UseToyAt(
                    actor->GetPlayerIndex(),
                    actor->GetUnitIndex(),
                    tl->m_screenX,
                    tl->m_screenY
                );
            } else {
                CGameObject* ul2 = other->m_object;
                m_triggerMgr->UseToyAt(
                    actor->GetPlayerIndex(),
                    actor->GetUnitIndex(),
                    ul2->m_screenX,
                    ul2->m_screenY
                );
            }
            return 1;
        }
    }
    CGameObject* ul3 = other->m_object;
    (static_cast<CGrunt*>(actor))
        ->CommitNeighbor(
            other->GetPlayerIndex(),
            other->GetUnitIndex(),
            ul3->m_screenX,
            ul3->m_screenY
        );
    PickupType prim = actor->GetEquippedToolType();
    if (prim != PICKUP_TIMEBOMB) {
        return 1;
    }

    i32 ycoord = actor->GetScreenTileY();
    i32 xcoord = actor->GetScreenTileX();
    ycoord += rand() % 10 - 5;
    i32 r2 = rand() % 10;
    RECT box;
    box.left = actor->GetScreenTileX() - 5;
    xcoord += r2 - 5;
    box.right = actor->GetScreenTileX() + 5;
    CMapMgr* board = m_board;
    box.bottom = actor->GetScreenTileY() + 5;
    box.top = actor->GetScreenTileY() - 5;

    board->Clip(&box);
    RouteUnitTo(actor, xcoord, ycoord, 0x20000d87, 0, 0);
    m_board->Clip(static_cast<const RECT*>(0));
    return 1;
}

RVA(0x0002b420, 0x419)
i32 CBattlezMapConfig::Serialize(CFileMemBase* ar) {
    if (ar == NULL) {
        return 0;
    }
    ar->Write(&m_active, sizeof(m_active));
    ar->Write(&m_playerIndex, sizeof(m_playerIndex));
    ar->Write(&m_reserved01c, sizeof(m_reserved01c));
    ar->Write(&m_reserved020, sizeof(m_reserved020));
    ar->Write(&m_reserved024, sizeof(m_reserved024));
    ar->Write(&m_reserved028, sizeof(m_reserved028));
    ar->Write(&m_reserved02c, sizeof(m_reserved02c));
    ar->Write(&m_defenderChance, sizeof(m_defenderChance));
    ar->Write(&m_reserved034, sizeof(m_reserved034));
    ar->Write(&m_reserved038, sizeof(m_reserved038));
    ar->Write(&m_reserved03c, sizeof(m_reserved03c));
    ar->Write(&m_reserved040, sizeof(m_reserved040));
    ar->Write(&m_reserved044, sizeof(m_reserved044));
    ar->Write(&m_gruntCreationTime, sizeof(m_gruntCreationTime));
    ar->Write(&m_resourceCreationTime, sizeof(m_resourceCreationTime));
    ar->Write(&m_spawnLastFire, sizeof(m_spawnLastFire));
    ar->Write(&m_repickLastFire, sizeof(m_repickLastFire));
    ar->Write(&m_spawnTimer, sizeof(m_spawnTimer));
    ar->Write(&m_repickTimer, sizeof(m_repickTimer));
    ar->Write(&m_gauntletzChance, sizeof(m_gauntletzChance));
    ar->Write(&m_shovelzChance, sizeof(m_shovelzChance));
    ar->Write(&m_spyzChance, sizeof(m_spyzChance));
    ar->Write(&m_brickzChance, sizeof(m_brickzChance));
    ar->Write(&m_gooberzChance, sizeof(m_gooberzChance));
    ar->Write(&m_gruntRatio, sizeof(m_gruntRatio));
    ar->Write(&m_reserved088, sizeof(m_reserved088));
    ar->Write(&m_defenderSearchRadiusX, sizeof(m_defenderSearchRadiusX));
    ar->Write(&m_defenderSearchRadiusY, sizeof(m_defenderSearchRadiusY));
    ar->Write(&m_idleRouteLimitX, sizeof(m_idleRouteLimitX));
    ar->Write(&m_idleRouteLimitY, sizeof(m_idleRouteLimitY));
    ar->Write(&m_reserved09c, sizeof(m_reserved09c));
    ar->Write(&m_idleAttackWaypointDelay, sizeof(m_idleAttackWaypointDelay));
    ar->Write(&m_defenderTargetMaxDistance, sizeof(m_defenderTargetMaxDistance));
    ar->Write(&m_reserved0a8, sizeof(m_reserved0a8));
    ar->Write(&m_idleBurnRandX, sizeof(m_idleBurnRandX));
    ar->Write(&m_idleBurnRandY, sizeof(m_idleBurnRandY));
    ar->Write(&m_reserveBudget, sizeof(m_reserveBudget));
    ar->Write(&m_idleRerouteDelay, sizeof(m_idleRerouteDelay));
    ar->Write(&m_moveBudget, sizeof(m_moveBudget));
    ar->Write(&m_assignedTargetMaxDistance, sizeof(m_assignedTargetMaxDistance));
    ar->Write(&m_repathBudget, sizeof(m_repathBudget));
    ar->Write(&m_inactiveTargetRerouteDelay, sizeof(m_inactiveTargetRerouteDelay));
    ar->Write(&m_nearbyRouteSearchDelay, sizeof(m_nearbyRouteSearchDelay));
    ar->Write(&m_marker, sizeof(m_marker));
    ar->Write(&m_reserved0d8, sizeof(m_reserved0d8));
    ar->Write(&m_reserved13c, sizeof(m_reserved13c));
    ar->Write(&m_roundRobinTick, sizeof(m_roundRobinTick));
    ar->Write(&m_reserved144, sizeof(m_reserved144));
    ar->Write(&m_claimTimer, sizeof(m_claimTimer));
    ar->Write(&m_reserved14c, sizeof(m_reserved14c));

    u32 i;
    u32 n = m_reserved104.GetSize();
    ar->Write(&n, sizeof(n));
    for (i = 0; i < n; i++) {
        DWORD v = m_reserved104[i];
        ar->Write(&v, sizeof(v));
    }

    n = m_reserved118.GetSize();
    ar->Write(&n, sizeof(n));
    for (i = 0; i < n; i++) {
        DWORD v = m_reserved118[i];
        ar->Write(&v, sizeof(v));
    }

    for (i32 k = 0; k < 4; k++) {
        ar->Write(&m_reserved12c[k], sizeof(m_reserved12c[k]));
    }

    n = GetAttackWaypointCount();
    ar->Write(&n, sizeof(n));
    for (i = 0; i < n; i++) {
        ar->Write(CoordAt(i), 8);
    }

    n = m_candArray.GetSize();
    ar->Write(&n, sizeof(n));
    for (i = 0; i < n; i++) {
        ar->Write(m_candArray[i], 8);
    }
    return 1;
}

RVA(0x0002b950, 0x513)
i32 CBattlezMapConfig::Deserialize(CFileMemBase* ar) {
    if (ar == NULL) {
        return 0;
    }
    ar->Read(&m_active, sizeof(m_active));
    ar->Read(&m_playerIndex, sizeof(m_playerIndex));
    ar->Read(&m_reserved01c, sizeof(m_reserved01c));
    ar->Read(&m_reserved020, sizeof(m_reserved020));
    ar->Read(&m_reserved024, sizeof(m_reserved024));
    ar->Read(&m_reserved028, sizeof(m_reserved028));
    ar->Read(&m_reserved02c, sizeof(m_reserved02c));
    ar->Read(&m_defenderChance, sizeof(m_defenderChance));
    ar->Read(&m_reserved034, sizeof(m_reserved034));
    ar->Read(&m_reserved038, sizeof(m_reserved038));
    ar->Read(&m_reserved03c, sizeof(m_reserved03c));
    ar->Read(&m_reserved040, sizeof(m_reserved040));
    ar->Read(&m_reserved044, sizeof(m_reserved044));
    ar->Read(&m_gruntCreationTime, sizeof(m_gruntCreationTime));
    ar->Read(&m_resourceCreationTime, sizeof(m_resourceCreationTime));
    ar->Read(&m_spawnLastFire, sizeof(m_spawnLastFire));
    ar->Read(&m_repickLastFire, sizeof(m_repickLastFire));
    ar->Read(&m_spawnTimer, sizeof(m_spawnTimer));
    ar->Read(&m_repickTimer, sizeof(m_repickTimer));
    ar->Read(&m_gauntletzChance, sizeof(m_gauntletzChance));
    ar->Read(&m_shovelzChance, sizeof(m_shovelzChance));
    ar->Read(&m_spyzChance, sizeof(m_spyzChance));
    ar->Read(&m_brickzChance, sizeof(m_brickzChance));
    ar->Read(&m_gooberzChance, sizeof(m_gooberzChance));
    ar->Read(&m_gruntRatio, sizeof(m_gruntRatio));
    ar->Read(&m_reserved088, sizeof(m_reserved088));
    ar->Read(&m_defenderSearchRadiusX, sizeof(m_defenderSearchRadiusX));
    ar->Read(&m_defenderSearchRadiusY, sizeof(m_defenderSearchRadiusY));
    ar->Read(&m_idleRouteLimitX, sizeof(m_idleRouteLimitX));
    ar->Read(&m_idleRouteLimitY, sizeof(m_idleRouteLimitY));
    ar->Read(&m_reserved09c, sizeof(m_reserved09c));
    ar->Read(&m_idleAttackWaypointDelay, sizeof(m_idleAttackWaypointDelay));
    ar->Read(&m_defenderTargetMaxDistance, sizeof(m_defenderTargetMaxDistance));
    ar->Read(&m_reserved0a8, sizeof(m_reserved0a8));
    ar->Read(&m_idleBurnRandX, sizeof(m_idleBurnRandX));
    ar->Read(&m_idleBurnRandY, sizeof(m_idleBurnRandY));
    ar->Read(&m_reserveBudget, sizeof(m_reserveBudget));
    ar->Read(&m_idleRerouteDelay, sizeof(m_idleRerouteDelay));
    ar->Read(&m_moveBudget, sizeof(m_moveBudget));
    ar->Read(&m_assignedTargetMaxDistance, sizeof(m_assignedTargetMaxDistance));
    ar->Read(&m_repathBudget, sizeof(m_repathBudget));
    ar->Read(&m_inactiveTargetRerouteDelay, sizeof(m_inactiveTargetRerouteDelay));
    ar->Read(&m_nearbyRouteSearchDelay, sizeof(m_nearbyRouteSearchDelay));
    ar->Read(&m_marker, sizeof(m_marker));
    ar->Read(&m_reserved0d8, sizeof(m_reserved0d8));
    ar->Read(&m_reserved13c, sizeof(m_reserved13c));
    ar->Read(&m_roundRobinTick, sizeof(m_roundRobinTick));
    ar->Read(&m_reserved144, sizeof(m_reserved144));
    ar->Read(&m_claimTimer, sizeof(m_claimTimer));
    ar->Read(&m_reserved14c, sizeof(m_reserved14c));

    u32 i;
    i32 j;
    int count;
    DWORD tmp;

    ar->Read(&count, sizeof(count));
    m_reserved104.RemoveAll();
    m_reserved104.SetSize(count, -1);
    for (i = 0; i < static_cast<u32>(count); i++) {
        ar->Read(&tmp, sizeof(tmp));
        m_reserved104[i] = tmp;
    }

    ar->Read(&count, sizeof(count));
    m_reserved118.RemoveAll();
    m_reserved118.SetSize(count, -1);
    for (i = 0; i < static_cast<u32>(count); i++) {
        ar->Read(&tmp, sizeof(tmp));
        m_reserved118[i] = tmp;
    }

    for (i32 k = 0; k < 4; k++) {
        ar->Read(&m_reserved12c[k], sizeof(m_reserved12c[k]));
    }

    for (j = 0; j < GetAttackWaypointCount(); j++) {
        Coord* q = CoordAt(j);
        if (q != NULL) {
            g_coordPool.Push(q);
        }
    }
    m_attackWaypoints.RemoveAll();
    ar->Read(&count, sizeof(count));
    m_attackWaypoints.SetSize(count, -1);
    for (i = 0; i < static_cast<u32>(count); i++) {
        Coord* payload = g_coordPool.Pop();
        ar->Read(payload, 8);
        m_attackWaypoints[i] = payload;
    }

    for (j = 0; j < m_candArray.GetSize(); j++) {
        Coord* q = static_cast<Coord*>(m_candArray[j]);
        if (q != NULL) {
            g_coordPool.Push(q);
        }
    }
    m_candArray.RemoveAll();
    ar->Read(&count, sizeof(count));
    m_candArray.SetSize(count, -1);
    for (i = 0; i < static_cast<u32>(count); i++) {
        Coord* payload = g_coordPool.Pop();
        ar->Read(payload, 8);
        m_candArray[i] = payload;
    }
    return 1;
}

RVA(0x0002bfc0, 0x8a)
i32 CBattlezMapConfig::SerializeState(CFileMemBase* arArg, SerialMode modeArg, LogicTypeId, i32) {
    CFileMemBase* ar = arArg;
    SerialMode mode = modeArg;
    switch (mode) {
        case SERIAL_SAVE:
            if (this->Serialize(ar) == 0) {
                return 0;
            }
            break;
        case SERIAL_LOAD:
            if (this->Deserialize(ar) == 0) {
                return 0;
            }
            break;
    }

    SerializeClockPair(ar, mode, &m_routeTiming);
    return 1;
}

RVA(0x0002c080, 0x8)
i32 CBattlezMapConfig::AcceptAlways(CGrunt*) {
    return 1;
}

RVA(0x0002c0a0, 0x78)
i32 CBattlezMapConfig::EnterDefenderMode(CGrunt* unit, i32 value) {
    if (unit->GetAiState() == AISTATE_RETURN) {
        return 1;
    }
    m_claimTimer = 0;
    unit->SetAiState(AISTATE_RETURN);
    unit->SetDefenderPickupType(static_cast<PickupType>(value));
    CGrunt** units = m_triggerMgr->PlayerUnits(m_playerIndex);
    i32 count = 0;
    for (i32 k = 0; k < TM_UNITS_PER_PLAYER; k++) {
        CGrunt* p = units[k];
        if (p != NULL && unit != p && p->GetAiState() == AISTATE_RETURN) {
            count++;
        }
    }
    unit->SetDefenderQueuePosition(count);
    return 1;
}

RVA(0x0002c140, 0x420)
i32 CBattlezMapConfig::RouteToNearbyPickup(CGrunt* unit) {
    if (unit->GetPowerupType() != GRUNT_NORMAL) {
        return 0;
    }
    PickupType prim = unit->GetEquippedToolType();
    if (prim != PICKUP_NONE) {
        return 0;
    }

    CRect box(
        unit->ScanCell().m_x - 3,
        unit->ScanCell().m_y - 3,
        unit->ScanCell().m_x + 4,
        unit->ScanCell().m_y + 4
    );
    {
        CMapMgr* board = m_board;
        board->Clip(&box);
    }

    CDDrawChildGroup* coll = m_ctx->m_world->ChildGroup();
    CGameObject* g = coll->FirstSerialChild();
    while (g != NULL) {
        if (g->GetLogicRecord()->GetDispatch() == &DispatchInGameIconLogic && !g->IsHidden()) {
            i32 special = 0;

            switch (static_cast<PickupType>(g->m_smarts)) {
                case PICKUP_HEALTH1:
                    special = 1;
                    break;
                case PICKUP_HEALTH2:
                    special = 1;
                    break;
                case PICKUP_HEALTH3:
                    special = 1;
                    break;
                case PICKUP_GHOST:
                    special = 1;
                    break;
                case PICKUP_SUPERSPEED:
                    special = 1;
                    break;
                case PICKUP_INVULNERABILITY:
                    special = 1;
                    break;
                case PICKUP_CONVERSION:
                    special = 1;
                    break;
                case PICKUP_DEATHTOUCH:
                    special = 1;
                    break;
                case PICKUP_ROIDZ:
                    special = 1;
                    break;
                case PICKUP_REACTIVEARMOR:
                    special = 1;
                    break;
                case PICKUP_RANDOMCOLORZ:
                    special = 1;
                    break;
                case PICKUP_SCREENSHAKE:
                    special = 1;
                    break;
                case PICKUP_BLACKSCREEN:
                    special = 1;
                    break;
                case PICKUP_MINICAM:
                    special = 1;
                    break;
            }
            i32 gx = g->m_screenX >> TILE_SHIFT_PX;
            i32 gy = g->m_screenY >> TILE_SHIFT_PX;
            CPoint wpt(gx, gy);
            if (box.PtInRect(wpt)) {
                if (special != 0 && unit->GetPowerupType() == GRUNT_NORMAL) {
                    if (RouteUnitTo(unit, gx, gy, 0x2000098b, 0, 0) != 0) {
                        CMapMgr* bd = m_board;
                        bd->Clip(NULL);
                        return 1;
                    }
                } else {
                    PickupType entranceMode = unit->GetEquippedToolType();
                    if (entranceMode == PICKUP_NONE) {
                        if (RouteUnitTo(unit, gx, gy, 0x2000098b, 0, 0) != 0) {
                            CMapMgr* bd = m_board;
                            bd->Clip(NULL);
                            return 1;
                        }
                    }
                }
            }
        }

        g = m_ctx->m_world->ChildGroup()->Drain();
    }
    m_board->Clip(static_cast<const RECT*>(0));
    return 0;
}

// @identity-TODO BattlezMapConfigAcceptAlwaysArg - the surviving external
// thunk and `ret 4` prove one callee-popped dword, but no use survives to prove
// the original symbol name or whether this was a member.
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0002c670, 0x8)
i32 __stdcall BattlezMapConfigAcceptAlwaysArg(i32) {
    return 1;
}

RVA(0x0002c690, 0xdb4)
i32 CBattlezMapConfig::ResolveArrival(CGrunt* g) {
    CPtrList* coordList = g->GetCoordList();
    if (RepathAroundBlockedTiles(g)) {
        return 1;
    }
    if (coordList->IsEmpty()) {
        return 0;
    }

    Coord first = *static_cast<Coord*>(coordList->GetHead());

    Coord a;
    Coord b;
    BrickzCell dest = m_board->CellAt(g->ScanCell().m_x, g->ScanCell().m_y);
    i32 ownFlags = m_board->CellAt(first.m_x, first.m_y).m_flags;

    i32 maskFlags = ownFlags & BRICKZ_CELL_UNOCCUPIED_MASK;
    PickupType type = EQUIPPED_TOOL_TERNARY_LE(g);

    if ((dest.m_flags & 0x400) && g->GetAiState() == AISTATE_RETURN
        && g->GetEquippedToolType() != PICKUP_GRAVITYBOOTZ) {
        if (ownFlags & 0x4000) {
            {
                RECT box;
                g->GetScreenTile(&a);
                box.bottom = a.m_y + 2;
                g->GetScreenTile(&b);
                box.right = b.m_x + 2;
                box.top = g->GetScreenTileY() - 1;
                {
                    Coord c;
                    g->GetScreenPos(&c);
                    box.left = (c.m_x >> TILE_SHIFT_PX) - 1;
                }

                {
                    CMapMgr* board = m_board;
                    board->Clip(&box);
                }
            }

            RECT scan = m_board->GetSearchBounds();

            g->GetScreenTile(&a);
            i32 stepDy = a.m_y - first.m_y;
            i32 stepDx;
            {
                Coord c;
                g->GetScreenPos(&c);
                stepDx = (c.m_x >> TILE_SHIFT_PX) - first.m_x;
            }
            if (g->TileSwitch(stepDx, stepDy, 0, 0x20000983, 1, 0) == 0) {
                for (i32 scanRow = scan.top; scanRow < scan.bottom; scanRow++) {
                    BrickzCell* rowCell = &m_board->m_rows[scanRow][scan.left];
                    for (i32 scanCol = scan.left; scanCol < scan.right; scanCol++) {
                        CPtrList path(0xa);
                        if (!(rowCell->m_flags & BRICKZ_CELL_OCCUPIED)) {
                            if (m_board->FindPathWithEndpointOverrides(
                                    g->GetScreenTileX(),
                                    g->GetScreenTileY(),
                                    scanCol,
                                    scanRow,
                                    &path,
                                    0,
                                    0x20004d03,
                                    0
                                ) != 0
                                && !path.IsEmpty()) {
                                RECYCLE_HEAD_COORD(path)
                                if (!path.IsEmpty()) {
                                    g->RecycleCoords();
                                    POSITION qp = path.GetHeadPosition();
                                    while (qp != NULL) {
                                        Coord* step = static_cast<Coord*>(path.GetNext(qp));
                                        g->AddTailCoord(step);
                                    }
                                    Coord* nt = g->GetTailCoord();
                                    SET_TILE_CENTER_PIXEL_PAIR(
                                        g->m_entrancePx.m_x,
                                        g->m_entrancePx.m_y,
                                        nt->m_x,
                                        nt->m_y
                                    )
                                    CMapMgr* bd = m_board;
                                    bd->Clip(NULL);
                                    return 1;
                                }
                            }
                        }
                    }
                }
            }
        }

        {
            CMapMgr* bd = m_board;
            bd->Clip(NULL);
        }
    }

    if ((dest.m_flags & 4) && g->GetBattlezTask() != BZTASK_SEEK_SWITCH) {
        Coord tp;
        i32 keyHi = g->GetScreenTileX();
        g->GetScreenTile(&tp);
        i32 key = CellKey(keyHi, tp.m_y);
        CTileTriggerSwitchLogic* r = m_cellQuery->FindSwitchLogic(key, TRIGID_ANY);
        if (r->GetType() == TRIGID_SWITCH_2) {
            g->SetAiState(AISTATE_SEEK);
            g->RecycleCoords();
            g->SetBattlezTask(BZTASK_SEEK_SWITCH);
            g->ResetDwell();
            return 0;
        }
    }

    if ((maskFlags & IDX(CELL_FLAG_HIDDEN_POWERUP)) && type == PICKUP_BRICK
        && g->GetBattlezTask() == BZTASK_CARRY_BRICK) {
        m_triggerMgr->UseEquippedToolAt(
            g->GetPlayerIndex(),
            g->GetUnitIndex(),
            (first.m_x << TILE_SHIFT_PX) + TILE_HALF_PX,
            (first.m_y << TILE_SHIFT_PX) + TILE_HALF_PX
        );
        g->RecycleCoords();
        return 0;
    }

    if ((maskFlags & IDX(CELL_FLAG_GAUNTLET_BRICK)) && type == PICKUP_BRICK
        && g->GetBattlezTask() == BZTASK_CARRY_BRICK) {
        if (m_board->CellTypeAt(first.m_x, first.m_y) != TILEKIND_GAUNTLET_BRICK_C) {
            m_triggerMgr->UseEquippedToolAt(
                g->GetPlayerIndex(),
                g->GetUnitIndex(),
                (first.m_x << TILE_SHIFT_PX) + TILE_HALF_PX,
                (first.m_y << TILE_SHIFT_PX) + TILE_HALF_PX
            );
            g->RecycleCoords();
            return 0;
        }
        g->RecycleCoords();
        return 0;
    }

    if (maskFlags & 0x200) {
        return 1;
    }

    if (maskFlags & 0x8) {
        if (PathToNearestGoal(g, first.m_x, first.m_y) != 0) {
            return 1;
        }
        EnterDefenderMode(g, 0x12);
    }

    if (maskFlags & 0x20) {
        PickupType er = g->m_activePickupType;
        if (g->ResolveEquippedToolType(er) == PICKUP_BOMB
            || g->ResolveEquippedToolType(er) == PICKUP_TIMEBOMB) {
            if (g->ResolveEquippedToolType(er) == PICKUP_BOMB) {
                m_triggerMgr->UseEquippedToolAt(
                    g->GetPlayerIndex(),
                    g->GetUnitIndex(),
                    (first.m_x << TILE_SHIFT_PX) + TILE_HALF_PX,
                    (first.m_y << TILE_SHIFT_PX) + TILE_HALF_PX
                );
                return 1;
            }
            if (g->ResolveEquippedToolType(er) == PICKUP_TIMEBOMB) {
                for (i32 row = first.m_y - 1; row < first.m_y + 2; row++) {
                    for (i32 col = first.m_x - 1; col < first.m_x + 2; col++) {
                        if (static_cast<u32>(col) < static_cast<u32>(m_board->GetWidth())
                            && static_cast<u32>(row) < static_cast<u32>(m_board->GetHeight())) {
                            i32 cf = m_board->CellFlagsAt(col, row);
                            if (cf & BRICKZ_BLOCKED_MASK) {
                                return 1;
                            }
                            DECLARE_TILE_CENTER_PIXEL_PAIR(hitX, hitY, col, row)
                            if (g->RectContains(hitX, hitY) != 0) {
                                m_triggerMgr->UseEquippedToolAt(
                                    g->GetPlayerIndex(),
                                    g->GetUnitIndex(),
                                    hitX,
                                    hitY
                                );
                            }
                            return 1;
                        }
                    }
                }
            }
        }
    }

    if (maskFlags & IDX(CELL_FLAG_GAUNTLET_BRICK)) {
        PickupType t = EQUIPPED_TOOL_TERNARY_GT(g);
        if (t == PICKUP_SPY) {
            CTileActionEvent* r = m_cellQuery->FindActionByCellKey(CellKey(first.m_x, first.m_y));
            if (r != NULL) {
                if (r->GetPlayerFlags(m_playerIndex) != 0) {
                    g->RecycleCoords();
                    ResolveTileClaim(g, first.m_x, first.m_y, 1);
                    return 1;
                }
                m_triggerMgr->UseEquippedToolAt(
                    g->GetPlayerIndex(),
                    g->GetUnitIndex(),
                    (first.m_x << TILE_SHIFT_PX) + TILE_HALF_PX,
                    (first.m_y << TILE_SHIFT_PX) + TILE_HALF_PX
                );
                return 1;
            }
        }
    }

    if (maskFlags & IDX(CELL_FLAG_HIDDEN_POWERUP)) {
        PickupType t = EQUIPPED_TOOL_TERNARY_GT(g);
        if (t == PICKUP_SPY) {
            g->RecycleCoords();
            ResolveTileClaim(g, first.m_x, first.m_y, 1);
            return 1;
        }
    }

    if (maskFlags & 0x20) {
        PickupType t = EQUIPPED_TOOL_TERNARY_GT(g);
        if (t == PICKUP_GAUNTLETZ) {
            if (maskFlags & IDX(CELL_FLAG_GAUNTLET_BRICK)) {
                CTileActionEvent* r =
                    m_cellQuery->FindActionByCellKey(CellKey(first.m_x, first.m_y));
                if (r != NULL) {
                    BrickTileId k = r->GetActionCode();
                    if (r->GetPlayerFlags(m_playerIndex) != 0) {
                        if (k == BRICKTILE_GOLD_1 || k == BRICKTILE_GOLD_2_TOP
                            || k == BRICKTILE_GOLD_3_TOP) {
                            ResolveTileClaim(g, first.m_x, first.m_y, 0);
                        }
                    } else {
                        if (k == BRICKTILE_GOLD_1 || k == BRICKTILE_GOLD_2_TOP
                            || k == BRICKTILE_GOLD_3_TOP) {
                            m_play->GetTileTriggers()->SetCell(first.m_x, first.m_y, m_playerIndex);
                        }
                    }
                }
            }
            m_triggerMgr->UseEquippedToolAt(
                g->GetPlayerIndex(),
                g->GetUnitIndex(),
                (first.m_x << TILE_SHIFT_PX) + TILE_HALF_PX,
                (first.m_y << TILE_SHIFT_PX) + TILE_HALF_PX
            );
            return 0;
        }
        if (t == PICKUP_TIMEBOMB || t == PICKUP_BOMB) {
            return 1;
        }
        b32 flag = true;
        if (t == PICKUP_BRICK && (maskFlags & IDX(CELL_FLAG_GAUNTLET_BRICK))) {
            flag = false;
        }
        if (t == PICKUP_SPY && (maskFlags & IDX(CELL_FLAG_GAUNTLET_BRICK))) {
            flag = false;
        }
        if (flag == false) {
            return 1;
        }
        EnterDefenderMode(g, 5);
        return 0;
    }

    if (maskFlags & IDX(CELL_FLAG_REVEALED_POWERUP)) {
        PickupType er2 = g->m_activePickupType;
        if (g->ResolveEquippedToolType(er2) != PICKUP_WINGZ) {
            if (g->ResolveEquippedToolType(er2) == PICKUP_SHOVEL) {
                m_triggerMgr->UseEquippedToolAt(
                    g->GetPlayerIndex(),
                    g->GetUnitIndex(),
                    (first.m_x << TILE_SHIFT_PX) + TILE_HALF_PX,
                    (first.m_y << TILE_SHIFT_PX) + TILE_HALF_PX
                );
                return 0;
            }
            EnterDefenderMode(g, 0xd);
            return 0;
        }
    }

    PathToNearestCandidate(g, false, 0, 0);
    if (PathCrossesMarkedTile(g) != 0) {
        return 1;
    }
    {
        PickupType t = EQUIPPED_TOOL_TERNARY_GT(g);
        if (t == PICKUP_WINGZ) {
            return 1;
        }
    }
    {
        i32 oy = g->GetScreenTileY();
        i32 ox = g->GetScreenTileX();
        i32 row = rand() % 3 + oy - 1;
        i32 col = rand() % 3 + ox - 1;
        if (static_cast<u32>(col) >= static_cast<u32>(m_board->GetWidth())
            || static_cast<u32>(row) >= static_cast<u32>(m_board->GetHeight())) {
            return 1;
        }
        i32 c0 = m_board->CellFlagsAt(col, row);
        i32 c1 = m_board->CellFlagsAt(col, row) & 0x987;
        if (c1 & BRICKZ_CELL_OCCUPIED) {
            return 1;
        }
        if (c1) {
            return 1;
        }
        if (c0 & BRICKZ_CELL_OCCUPIED) {
            return 1;
        }
        g->TileSwitch(col, row, 0, 0x987, 1, 0);
    }
    return 1;
}

RVA(0x0002d800, 0x605)
void CBattlezMapConfig::ClaimTilesAround(CGrunt* unit, i32 col, i32 row, i32 requireUnoccupied) {
    if (g_stepRun == false) {
        return;
    }
    i32 word = m_board->CellFlagsAtUnchecked(col, row);
    if (word & IDX(CELL_FLAG_HIDDEN_POWERUP)) {
        CPtrList list(10);
        Coord start = ScreenTile(unit);
        if ((m_board)
                ->FindPathWithEndpointOverrides(start.m_x, start.m_y, col, row, &list, 1, 0x4903, 0)
            != 0) {
            g_stepRun = false;
            g_stepCol = col;
            g_stepRow = row;
            RecycleCoordList(list);
            return;
        }
    }
    if (word & IDX(CELL_FLAG_GAUNTLET_BRICK)) {
        CTileActionEvent* cell = m_cellQuery->FindActionByCellKey(CellKey(col, row));
        if (requireUnoccupied != 0) {
            if (cell != NULL && cell->GetPlayerFlags(m_playerIndex) == 0) {
                CPtrList list2(10);
                Coord start = ScreenTile(unit);
                if ((m_board)->FindPathWithEndpointOverrides(
                        start.m_x,
                        start.m_y,
                        col,
                        row,
                        &list2,
                        1,
                        0x4003,
                        0
                    )
                    != 0) {
                    g_stepRun = false;
                    g_stepCol = col;
                    g_stepRow = row;
                    RecycleCoordList(list2);
                }
            }
        } else if (cell != NULL) {
            BrickTileId id = cell->GetActionCode();
            i32 occ = cell->GetPlayerFlags(m_playerIndex);
            i32 special = 0;
            if (occ == 0) {
                special = 1;
            }
            if (occ != 0) {
                if (id == BRICKTILE_RED_1 || id == BRICKTILE_RED_2_TOP || id == BRICKTILE_RED_3_TOP
                    || id == BRICKTILE_BLACK_1 || id == BRICKTILE_BLACK_2_TOP
                    || id == BRICKTILE_BLACK_3_TOP || id == BRICKTILE_BLUE_1
                    || id == BRICKTILE_BLUE_2_TOP || id == BRICKTILE_BLUE_3_TOP
                    || id == BRICKTILE_BROWN_1 || id == BRICKTILE_BROWN_2
                    || id == BRICKTILE_BROWN_3) {
                    special = 1;
                }
            }
            if (special != 0) {
                CPtrList list3(10);
                Coord start = ScreenTile(unit);
                if ((m_board)->FindPathWithEndpointOverrides(
                        start.m_x,
                        start.m_y,
                        col,
                        row,
                        &list3,
                        1,
                        0x4003,
                        0
                    )
                    != 0) {
                    if (!list3.IsEmpty()) {
                        g_stepRun = false;
                        g_stepCol = col;
                        g_stepRow = row;
                        RecycleCoordList(list3);
                    }
                }
            }
        }
    }

    m_board->CellFlagsAtUnchecked(col, row) |= IDX(CELL_FLAG_CLAIM_VISITED);
    i32 cm = col - 1;
    i32 cp = col + 1;
    i32 rm = row - 1;
    i32 rp = row + 1;
    CMapMgr* b;
    i32 nw;

    b = m_board;
    if (static_cast<u32>(cm) < static_cast<u32>(b->m_width)) {
        nw = b->CellFlagsAtUnchecked(cm, row);
        if (!(nw & IDX(CELL_FLAG_CLAIM_VISITED))
            && ((nw & IDX(CELL_FLAG_GAUNTLET_BRICK | CELL_FLAG_HIDDEN_POWERUP))
                || b->CellTypeAt(cm, row) == TILEKIND_AI_PATH_BLOCKER)) {
            ClaimTilesAround(unit, cm, row, requireUnoccupied);
        }
    }
    b = m_board;
    if (static_cast<u32>(cp) < static_cast<u32>(b->m_width)) {
        nw = b->CellFlagsAtUnchecked(cp, row);
        if (!(nw & IDX(CELL_FLAG_CLAIM_VISITED))
            && ((nw & IDX(CELL_FLAG_GAUNTLET_BRICK | CELL_FLAG_HIDDEN_POWERUP))
                || b->CellTypeAt(cp, row) == TILEKIND_AI_PATH_BLOCKER)) {
            ClaimTilesAround(unit, cp, row, requireUnoccupied);
        }
    }
    b = m_board;
    if (static_cast<u32>(rm) < static_cast<u32>(b->m_width)) {
        nw = b->CellFlagsAtUnchecked(col, rm);
        if (!(nw & IDX(CELL_FLAG_CLAIM_VISITED))
            && ((nw & IDX(CELL_FLAG_GAUNTLET_BRICK | CELL_FLAG_HIDDEN_POWERUP))
                || b->CellTypeAt(col, rm) == TILEKIND_AI_PATH_BLOCKER)) {
            ClaimTilesAround(unit, col, rm, requireUnoccupied);
        }
    }
    b = m_board;
    if (static_cast<u32>(rp) < static_cast<u32>(b->m_width)) {
        nw = b->CellFlagsAtUnchecked(col, rp);
        if (!(nw & IDX(CELL_FLAG_CLAIM_VISITED))
            && ((nw & IDX(CELL_FLAG_GAUNTLET_BRICK | CELL_FLAG_HIDDEN_POWERUP))
                || b->CellTypeAt(col, rp) == TILEKIND_AI_PATH_BLOCKER)) {
            ClaimTilesAround(unit, col, rp, requireUnoccupied);
        }
    }
    b = m_board;
    if (static_cast<u32>(cp) < static_cast<u32>(b->m_width)
        && static_cast<u32>(rm) < static_cast<u32>(b->m_height)) {
        nw = b->CellFlagsAtUnchecked(cp, rm);
        if (!(nw & IDX(CELL_FLAG_CLAIM_VISITED))
            && ((nw & IDX(CELL_FLAG_GAUNTLET_BRICK | CELL_FLAG_HIDDEN_POWERUP))
                || b->CellTypeAt(cp, rm) == TILEKIND_AI_PATH_BLOCKER)) {
            ClaimTilesAround(unit, cp, rm, requireUnoccupied);
        }
    }
    b = m_board;
    if (static_cast<u32>(cp) < static_cast<u32>(b->m_width)
        && static_cast<u32>(rp) < static_cast<u32>(b->m_height)) {
        nw = b->CellFlagsAtUnchecked(cp, rp);
        if (!(nw & IDX(CELL_FLAG_CLAIM_VISITED))
            && ((nw & IDX(CELL_FLAG_GAUNTLET_BRICK | CELL_FLAG_HIDDEN_POWERUP))
                || b->CellTypeAt(cp, rp) == TILEKIND_AI_PATH_BLOCKER)) {
            ClaimTilesAround(unit, cp, rp, requireUnoccupied);
        }
    }
    b = m_board;
    if (static_cast<u32>(cm) < static_cast<u32>(b->m_width)
        && static_cast<u32>(rp) < static_cast<u32>(b->m_height)) {
        nw = b->CellFlagsAtUnchecked(cm, rp);
        if (!(nw & IDX(CELL_FLAG_CLAIM_VISITED))
            && ((nw & IDX(CELL_FLAG_GAUNTLET_BRICK | CELL_FLAG_HIDDEN_POWERUP))
                || b->CellTypeAt(cm, rp) == TILEKIND_AI_PATH_BLOCKER)) {
            ClaimTilesAround(unit, cm, rp, requireUnoccupied);
        }
    }

    b = m_board;
    if (static_cast<u32>(cm) < static_cast<u32>(b->m_width)
        && static_cast<u32>(rm) < static_cast<u32>(b->m_height)) {
        nw = b->CellFlagsAtUnchecked(cm, rm);
        if (!(nw & IDX(CELL_FLAG_CLAIM_VISITED))
            && ((nw & IDX(CELL_FLAG_GAUNTLET_BRICK | CELL_FLAG_HIDDEN_POWERUP))
                || b->CellTypeAt(cm, rm) == TILEKIND_AI_PATH_BLOCKER)) {
            ClaimTilesAround(unit, cm, rm, requireUnoccupied);
        }
    }
}

RVA(0x0002dfa0, 0x325)
i32 CBattlezMapConfig::ResolveTileClaim(CGrunt* unit, i32 col, i32 row, i32 requireUnoccupied) {
    g_stepRun = true;

    i32 bottom;
    i32 right;
    i32 top;
    i32 left;
    {
        bottom = unit->GetScreenTileY();
        Coord g0;
        Coord g1;
        Coord g2;
        unit->GetScreenTile(&g0);
        g2.m_y = g0.m_y;
        right = g0.m_x;
        unit->GetScreenTile(&g1);
        g2.m_x = g1.m_x;
        top = g1.m_y;
        unit->GetScreenTile(&g2);
        left = g2.m_x;
    }
    RECT box;
    SET_RECT_COMPONENTS(box, left - 8, top - 8, right + 8, bottom + 8);
    {
        CMapMgr* board = m_board;
        board->Clip(&box);
    }
    ClaimTilesAround(unit, col, row, requireUnoccupied);
    if (g_stepRun == false) {
        Coord saved = unit->EntrancePx();
        i32 col = saved.m_x >> TILE_SHIFT_PX;
        i32 row = saved.m_y >> TILE_SHIFT_PX;
        u32 tile0 = m_board->CellFlagsAt(col, row);
        b32 flag = ((tile0 >> 2) & 1) != 0;
        if (!unit->CoordsEmpty()) {
            Coord* c = unit->GetTailCoord();
            i32 cx = c->m_x;
            i32 cy = c->m_y;
            i32 tile1 = m_board->CellFlagsAt(cx, cy);
            if (tile1 & 4) {
                saved = *c;
                flag = true;
            }
        }
        unit->TileSwitch(g_stepCol, g_stepRow, 0, 0x9c3, 1, 0);
        if (flag != false) {
            unit->m_entrancePx = saved;
        }
    }

    RECT sweep = m_board->GetSearchBounds();
    for (i32 c = sweep.left; c < sweep.right; c++) {
        for (i32 r = sweep.top; r < sweep.bottom; r++) {
            m_board->CellFlagsAtUnchecked(c, r) &= ~IDX(CELL_FLAG_CLAIM_VISITED);
        }
    }

    {
        m_board->Clip(NULL);
    }
    return 1;
}

RVA(0x0002e3a0, 0x7e1)
i32 CBattlezMapConfig::RouteToNearbyEnemy(CGrunt* unit) {
    CRect box(
        unit->ScanCell().m_x - 7,
        unit->ScanCell().m_y - 7,
        unit->ScanCell().m_x + 7,
        unit->ScanCell().m_y + 7
    );

    CGrunt* best = NULL;
    i32 bestDist = INT_MAX;
    for (i32 band = 0; band < 4; band++) {
        if (band == m_playerIndex) {
            continue;
        }
        for (i32 i = 0; i < TM_UNITS_PER_PLAYER; i++) {
            CGrunt* u = m_triggerMgr->UnitAt(band, i);
            if (u == NULL) {
                continue;
            }
            if (u->m_entranceCommitted == false) {
                continue;
            }
            if (u->m_deathAnimStarted != false) {
                continue;
            }
            if (u->m_entranceActive != false) {
                continue;
            }
            if (u->m_inCombat != false) {
                continue;
            }
            if (!u->IsNotAnimationAct("C")) {
                continue;
            }
            if (!u->IsNotAnimationAct("R")) {
                continue;
            }
            if (!u->IsNotAnimationAct("J")) {
                continue;
            }
            if (!u->IsNotAnimationAct("G")) {
                continue;
            }
            if (!u->IsNotAnimationAct("L")) {
                continue;
            }
            if (u->m_powerupType == GRUNT_GHOST) {
                continue;
            }
            Coord c;
            u->GetScreenTile(&c);
            CPoint wpt(c.m_x, c.m_y);
            if (!box.PtInRect(wpt)) {
                continue;
            }
            Coord unitPos1;
            unit->GetScreenTile(&unitPos1);
            Coord candidatePos1;
            u->GetScreenTile(&candidatePos1);
            i32 dx = abs(unitPos1.m_x - candidatePos1.m_x);
            Coord unitPos2;
            unit->GetScreenTile(&unitPos2);
            Coord candidatePos2;
            u->GetScreenTile(&candidatePos2);
            i32 dy = abs(unitPos2.m_y - candidatePos2.m_y);
            i32 dist = SquaredDistance(dx, dy);
            if (dist >= bestDist) {
                continue;
            }
            bestDist = dist;
            best = u;
        }
    }
    if (best != NULL) {
        if (static_cast<u32>(unit->GetDwell()) > 0x64) {
            m_board->Clip(&box);

            i32 flags = 0;
            PickupType prim = unit->m_activePickupType;
            PickupType t = unit->ResolveEquippedToolType(prim);
            if (t == PICKUP_TOOB) {
                flags = 0x100;
            }
            t = prim;
            if (prim > PICKUP_EQUIPPABLE_LAST) {
                t = unit->m_savedToolType;
            }
            if (t == PICKUP_WINGZ) {
                flags = 0x942;
            }
            if (prim > PICKUP_EQUIPPABLE_LAST) {
                prim = unit->m_savedToolType;
            }
            if (prim == PICKUP_SPRING) {
                flags = 0x1000;
            }
            Coord bc;
            best->GetScreenTile(&bc);
            if (RouteUnitTo(unit, bc.m_x, bc.m_y, 0x1000d8f, flags, 1) != 0) {
                if (unit->GetAiState() != AISTATE_RETURN) {
                    unit->SetAiState(AISTATE_SEEK);
                    unit->SetRoutePassableMask(0);
                }
                if (unit->m_blockedVoicePending != false) {
                    __int64 elapsed = static_cast<__int64>(g_frameTime) - m_routeTiming.m_start;
                    if (elapsed >= m_routeTiming.m_interval) {
                        unit->m_blockedVoicePending = false;
                        CGameObject* lvl = unit->m_object;

                        RECT* hit = g_gameReg->m_world->GetLevel()->m_mainPlane->GetPlaneViewRect();
                        if (::PtInRect(hit, lvl->m_screenX, lvl->m_screenY)) {
                            g_gameReg->VoiceMgr()->PlayVoice(unit, 0x366, -1, 0, -1, -1);
                        }
                        m_routeTiming.Clear();
                        m_routeTiming.m_intervalLo = BLOCKED_VOICE_INTERVAL_MS;
                        m_routeTiming.m_intervalHi = 0;
                        m_routeTiming.m_start = g_frameTime;
                    }
                }

                m_board->Clip(NULL);
                unit->ResetDwell();
            } else {
                m_board->Clip(NULL);
                unit->ResetDwell();
                return 0;
            }
        }
        return 1;
    }
    unit->m_blockedVoicePending = true;
    return 0;
}

RVA(0x0002ed90, 0x5)
i32 CBattlezMapConfig::PathToNearbyUnit(CGrunt*) {
    return 0;
}

RVA(0x0002edb0, 0x6b4)
i32 CBattlezMapConfig::PathToNearestCandidate(CGrunt* unit, b32 useArg, i32 ax, i32 ay) {
    if (unit->CoordsEmpty()) {
        return 0;
    }
    Coord target;
    POSITION n = unit->CoordHead();
    b32 found = false;
    if (useArg == false) {

        while (n != NULL) {
            Coord* c = unit->GetNextCoord(n);
            if (c != NULL) {
                BrickzCell* row = m_board->m_rows[c->m_y];
                if (row[c->m_x].m_flags & 4) {
                    found = true;
                    target = *c;
                    break;
                }
            }
        }

        if (found != false) {
            if (unit->GetAiState() == AISTATE_RETURN) {
                return 1;
            }
        }
        if (found != false) {
            if (IsCoordOccupied(unit, target.m_x, target.m_y) != 0) {

                unit->RecycleCoords();
                unit->SetAiState(AISTATE_SEEK);
                return 1;
            }
        }
        if (found != false && PathCrossesMarkedTile(unit) != 0) {

            if (!unit->CoordsEmpty()) {
                POSITION p = unit->CoordHead();
                Coord* c = unit->GetCoordAt(p);
                CMapMgr* b = m_board;
                i32 word = b->CellFlagsAt(c->m_x, c->m_y);
                if (!(word & BRICKZ_CELL_OCCUPIED)) {
                    return 1;
                }
            }
        }
    } else {
        target.Set(ax, ay);
        found = true;
    }
    if (found == false) {
        return 0;
    }
    if (IsCoordOccupied(unit, target.m_x, target.m_y) != 0) {
        return 0;
    }

    i32 r = rand() % TM_UNITS_PER_PLAYER;
    for (i32 scanned = 0; scanned < TM_UNITS_PER_PLAYER; scanned++) {
        CGrunt* cand = m_triggerMgr->UnitAt(m_playerIndex, r);
        if (cand != NULL) {
            if (IsGruntAtSavedScreenPos(cand) && cand->IsEntranceCommitted() != false
                && cand->IsDeathAnimationStarted() == false && cand->m_entranceActive == false
                && cand->IsInCombat() == false) {
                if (!cand->IsAnimationAct("I") && !cand->IsAnimationAct("G")
                    && !cand->IsAnimationAct("L") && !cand->IsAnimationAct("P")
                    && !cand->IsAnimationAct("J") && !cand->IsAnimationAct("C")
                    && !cand->IsAnimationAct("R") && cand != unit
                    && cand->GetAiState() != AISTATE_RETURN
                    && cand->GetAiState() != AISTATE_RETREAT) {
                    i32 dx = abs(cand->GetScreenTileX() - unit->GetScreenTileX());
                    i32 dy = abs(cand->GetScreenTileY() - unit->GetScreenTileY());
                    if (SquaredDistance(dx, dy) <= 0x190) {

                        i32 flags = BATTLEZ_ROUTE_OTHER_TOOLS_TRIGGER;
                        PickupType entranceReason = unit->m_activePickupType;
                        if (unit->ResolveEquippedToolType(entranceReason) == PICKUP_WINGZ) {
                            flags = BATTLEZ_ROUTE_OTHER_TOOLS_TRIGGER_WINGZ;
                        }
                        if (unit->ResolveEquippedToolType(entranceReason) == PICKUP_TOOB) {
                            flags |= BATTLEZ_ROUTE_TOOB_TRAVERSAL;
                        }
                        CPtrList list(10);
                        Coord oc;
                        GET_SCREEN_TILE_Y_FIRST(static_cast<CUserLogic*>(cand), oc)
                        if ((m_board)->FindPathWithEndpointOverrides(
                                oc.m_x,
                                oc.m_y,
                                target.m_x,
                                target.m_y,
                                &list,
                                1,
                                0x98b,
                                flags
                            )
                            != 0) {
                            if (list.GetHeadPosition() != NULL) {
                                RECYCLE_HEAD_COORD(list)
                            }
                            if (list.GetHeadPosition() != NULL) {
                                unit->RecycleCoords();
                                cand->RecycleCoords();
                                POSITION pp = list.GetHeadPosition();
                                while (pp != NULL) {
                                    cand->AddTailCoord(static_cast<Coord*>(list.GetNext(pp)));
                                }
                                unit->SetAiState(AISTATE_SEEK);
                                cand->SetAiState(AISTATE_RETREAT);
                            }
                            return 1;
                        }
                        return 0;
                    }
                }
            }
        }
        r = (r + 1) % TM_UNITS_PER_PLAYER;
    }
    return 0;
}

RVA(0x0002f620, 0x871)
i32 CBattlezMapConfig::ChooseIdleBehavior(CGrunt* unit) {
    if (unit->IsEntranceCommitted() == false) {
        return 0;
    }
    if (unit->IsDeathAnimationStarted() != false) {
        return 0;
    }
    if (unit->m_entranceActive != false) {
        return 0;
    }
    if (unit->IsInCombat() != false) {
        return 0;
    }

    if (unit->GetAnimationActName() == "I") {
        return 0;
    }
    if (unit->GetAnimationActName() == "G") {
        return 0;
    }
    if (unit->GetAnimationActName() == "L") {
        return 0;
    }
    if (unit->GetAnimationActName() == "P") {
        return 0;
    }
    if (unit->GetAnimationActName() == "J") {
        return 0;
    }
    if (unit->GetAnimationActName() == "C") {
        return 0;
    }
    if (unit->GetAnimationActName() == "R") {
        return 0;
    }

    i32 band = GetRandom(1, m_brickzPct);
    if (band <= m_toolzPct) {

        PickupType cur = unit->GetEquippedToolType();
        if (cur != PICKUP_NONE) {
            return 1;
        }
        i32 roll = GetRandom(1, m_wingzPct);
        PickupType mode = PICKUP_WINGZ;
        if (roll <= m_bombzPct) {
            mode = PICKUP_BOMB;
        } else if (roll <= m_boomerangzPct) {
            mode = PICKUP_BOOMERANG;
        } else if (roll <= m_toolBrickzPct) {
            mode = PICKUP_BRICK;
        } else if (roll <= m_clubzPct) {
            mode = PICKUP_CLUB;
        } else if (roll <= m_gauntletzPct) {
            mode = PICKUP_GAUNTLETZ;
        } else if (roll <= m_glovezPct) {
            mode = PICKUP_GLOVEZ;
        } else if (roll <= m_gooberzPct) {
            mode = PICKUP_GOOBER;
        } else if (roll <= m_gravityBootzPct) {
            mode = PICKUP_GRAVITYBOOTZ;
        } else if (roll <= m_gunHatzPct) {
            mode = PICKUP_GUNHAT;
        } else if (roll <= m_nerfGunzPct) {
            mode = PICKUP_NERFGUN;
        } else if (roll <= m_rockzPct) {
            mode = PICKUP_ROCK;
        } else if (roll <= m_shieldzPct) {
            mode = PICKUP_SHIELD;
        } else if (roll <= m_shovelzPct) {
            mode = PICKUP_SHOVEL;
        } else if (roll <= m_springzPct) {
            mode = PICKUP_SPRING;
        } else if (roll <= m_spyzPct) {
            mode = PICKUP_SPY;
        } else if (roll <= m_swordzPct) {
            mode = PICKUP_SWORD;
        } else if (roll <= m_timeBombzPct) {
            mode = PICKUP_TIMEBOMB;
        } else if (roll <= m_toobzPct) {
            mode = PICKUP_TOOB;
        } else if (roll <= m_wandzPct) {
            mode = PICKUP_WAND;
        } else if (roll <= m_welderzPct) {
            mode = PICKUP_WELDER;
        }
        if (mode == PICKUP_WARPSTONE) {
            mode = PICKUP_GAUNTLETZ;
        }
        if (mode == PICKUP_BRICK) {

            CGrunt** units = m_triggerMgr->PlayerUnits(m_playerIndex);
            i32 nIdle = 0;
            for (i32 s = TM_UNITS_PER_PLAYER; s != 0; s--) {
                CGrunt* u = *units;
                if (u != NULL && u->GetBattlezTask() == BZTASK_CARRY_BRICK) {
                    nIdle++;
                }
                units++;
            }
            if (nIdle >= 2) {
                return 1;
            }
            for (i32 b = 0; b < TM_UNITS_PER_PLAYER; b++) {
                CGrunt* u = m_triggerMgr->UnitAt(m_playerIndex, b);
                if (u == NULL) {
                    continue;
                }
                if (u->GetBattlezTask() != BZTASK_UNASSIGNED) {
                    continue;
                }
                if (u->IsInCombat() != false) {
                    continue;
                }
                u->BeginPickupAnimation(PICKUP_BRICK, 1, 0, 0, 1);
                u->SetBattlezTask(BZTASK_CARRY_BRICK);
                u->RecycleCoords();
            }
            return 1;
        }

        PickupType cur2 = unit->GetEquippedToolType();
        if (cur2 == PICKUP_NONE) {
            unit->BeginPickupAnimation(mode, 1, 0, 0, 1);
            return 1;
        }
        if (mode != PICKUP_TOOB) {
            if (mode == PICKUP_WINGZ) {
                unit->RecycleCoords();
            }
        } else {
            unit->RecycleCoords();
        }
    }

    if (band <= m_toyzPct) {

        i32 roll = GetRandom(1, m_yoyozPct);
        PickupType mode;
        if (roll <= m_babyWalkerzPct) {
            mode = PICKUP_BABYWALKER;
        } else if (roll <= m_beachBallzPct) {
            mode = PICKUP_BEACHBALL;
        } else if (roll <= m_bigWheelzPct) {
            mode = PICKUP_BIGWHEEL;
        } else if (roll <= m_goKartzPct) {
            mode = PICKUP_GOKART;
        } else if (roll <= m_jackInTheBoxzPct) {
            mode = PICKUP_JACKINTHEBOX;
        } else if (roll <= m_jumpRopezPct) {
            mode = PICKUP_JUMPROPE;
        } else if (roll <= m_pogoStickzPct) {
            mode = PICKUP_POGOSTICK;
        } else if (roll <= m_scrollzPct) {
            mode = PICKUP_SCROLL;
        } else {
            mode = roll > m_squeakToyzPct ? PICKUP_YOYO : PICKUP_SQUEAKTOY;
        }
        unit->BeginPickupAnimation(mode, 1, 0, 0, 1);
        return 1;
    } else {

        i32 roll = GetRandom(1, m_blackBrickPct);
        PickupType mode = PICKUP_BLACKBRICK;
        if (roll <= m_redBrickPct) {
            mode = PICKUP_REDBRICK;
        } else if (roll <= m_blueBrickPct) {
            mode = PICKUP_BLUEBRICK;
        } else if (roll <= m_goldBrickPct) {
            mode = PICKUP_GOLDBRICK;
        }
        if (mode >= PICKUP_BRICKZ_FIRST) {
            unit->m_brickPickupType = mode;
            unit->m_pendingPickupType = PICKUP_INVALID;
        }
        return 1;
    }
}

RVA(0x000300c0, 0x190)
i32 CBattlezMapConfig::RouteUnitTo(
    CGrunt* unit,
    i32 goalCol,
    i32 goalRow,
    i32 blockedMask,
    i32 passableMask,
    i32 clearEndpointFlags
) {
    CPtrList list(10);
    CGameObject* lvl = unit->m_object;
    i32 screenX = lvl->m_screenX;
    if (unit->GetScreenTileX() != goalCol || unit->GetScreenTileY() != goalRow) {
        if ((m_board)->FindPathWithEndpointOverrides(
                screenX >> TILE_SHIFT_PX,
                lvl->m_screenY >> TILE_SHIFT_PX,
                goalCol,
                goalRow,
                &list,
                clearEndpointFlags,
                blockedMask,
                passableMask
            )
            != 0) {
            if (!list.IsEmpty()) {
                RECYCLE_HEAD_COORD(list)
                if (!list.IsEmpty()) {
                    unit->RecycleCoords();

                    POSITION pp = list.GetHeadPosition();
                    while (pp != NULL) {
                        Coord* cur = static_cast<Coord*>(list.GetNext(pp));
                        if (cur != NULL) {
                            unit->AddTailCoord(cur);
                        }
                    }
                    list.RemoveAll();
                    Coord* tail = unit->GetTailCoord();
                    i32 tailX = tail->m_x;
                    i32 tailY = tail->m_y;
                    unit->m_entrancePx.Set(
                        (tailX << TILE_SHIFT_PX) + TILE_HALF_PX,
                        (tailY << TILE_SHIFT_PX) + TILE_HALF_PX
                    );
                    return 1;
                }
            }
        }
    }
    return 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000302c0, 0x1ec)
i32 CBattlezMapConfig::RouteUnitToGoal(
    CGrunt* unit,
    Coord goal,
    i32 blockedMask,
    i32 passableMask
) {
    CPtrList list(10);
    Coord cur;
    POSITION n;
    Coord* head;
    POSITION qp;

    if (unit->ScanCell().m_x == goal.m_x) {
        if (unit->ScanCell().m_y == goal.m_y) {
            goto fail;
        }
    }

    n = unit->CoordHead();
    while (n != NULL) {
        Coord* coord = unit->GetNextCoord(n);
        if (coord != NULL && *coord == goal) {
            break;
        }
    }

    cur = ScreenTile(unit);
    if ((m_board)->FindPathWithEndpointOverrides(
            cur.m_x,
            cur.m_y,
            goal.m_x,
            goal.m_y,
            &list,
            0,
            blockedMask,
            passableMask
        )
        == 0) {
        goto fail;
    }
    if (list.IsEmpty()) {
        goto fail;
    }
    head = static_cast<Coord*>(list.RemoveHead());
    if (head != NULL) {
        g_coordPool.Push(head);
    }
    if (!list.IsEmpty()) {
        if (n != NULL) {
            POSITION h = unit->CoordHead();
            if (h != NULL) {
                do {
                    CPtrList* listPayload = unit->GetCoordList();
                    if (listPayload != NULL) {
                        g_coordPool.Push(listPayload);
                    }
                } while (h != n);
            }
        }

        unit->RecycleCoords();

        qp = list.GetHeadPosition();
        while (qp != NULL) {
            Coord* cur5 = static_cast<Coord*>(list.GetNext(qp));
            if (cur5 != NULL) {
                unit->AddTailCoord(cur5);
            }
        }
        list.RemoveAll();
        return 1;
    }
fail:
    return 0;
}

RVA(0x00030530, 0x56)
i32 CBattlezMapConfig::PathCrossesMarkedTile(CGrunt* unit) {
    if (unit->CoordsEmpty()) {
        return 0;
    }
    POSITION node = unit->CoordHead();
    if (node == NULL) {
        return 0;
    }
    BrickzCell** rows = ((m_board)->m_rows);
    while (node != NULL) {
        Coord* c = unit->GetNextCoord(node);
        i32 y = c->m_y;
        i32 x = c->m_x;
        if (rows[y][x].m_flags & 4) {
            return 1;
        }
    }
    return 0;
}

// @early-stop
RVA(0x000305b0, 0x121)
i32 CBattlezMapConfig::IsCoordOccupied(CGrunt* selfUnit, i32 qx, i32 qy) {
    i32 i = 0;
    CGrunt** units = m_triggerMgr->PlayerUnits(m_playerIndex);
    for (;;) {
        CGrunt* unit = *units;
        if (unit != NULL && unit != selfUnit && unit->GetBattlezTask() != BZTASK_SEEK_SWITCH) {

            if (!unit->CoordsEmpty()) {
                POSITION node = unit->CoordHead();
                if (node != NULL) {
                    CMapMgr* board = m_board;
                    for (;;) {
                        Coord* c = unit->GetNextCoord(node);
                        i32 x = c->m_x;
                        i32 y = c->m_y;
                        i32 tile = board->CellFlagsAt(x, y);
                        if ((tile & 4) && x == qx && y == qy) {
                            return 1;
                        }
                        if (node == NULL) {
                            break;
                        }
                    }
                }
            }
            i32 entranceX = unit->m_entrancePx.m_x >> TILE_SHIFT_PX;
            i32 entranceY = unit->m_entrancePx.m_y >> TILE_SHIFT_PX;
            if (entranceX == qx && entranceY == qy) {
                return 1;
            }
            Coord current = ScreenTile(unit);
            if (current.m_x == qx && current.m_y == qy) {
                return 1;
            }
        }
        i++;
        units++;
        if (i >= TM_UNITS_PER_PLAYER) {
            break;
        }
    }
    return 0;
}

// @early-stop
RVA(0x00030730, 0x1da)
i32 CBattlezMapConfig::ClaimCellFromRow(i32 targetPlayer, i32 targetUnit, i32, i32) {
    if (m_active == false) {
        return 0;
    }
    if (targetPlayer == m_playerIndex) {
        return 1;
    }
    CGrunt* src = m_triggerMgr->UnitAt(targetPlayer, targetUnit);
    if (src == NULL) {
        return 0;
    }
    if (src->GetPowerupType() == GRUNT_GHOST) {
        return 0;
    }
    if (src->GetBattlezTask() == BZTASK_ADVANCE) {
        if (src->ArrivalCell().m_x != m_playerIndex) {
            return 0;
        }
    }
    for (i32 i = 0; i < TM_UNITS_PER_PLAYER; i++) {
        CGrunt* u = m_triggerMgr->UnitAt(m_playerIndex, i);
        if (u == NULL) {
            continue;
        }
        b32 ok = true;
        if (u->GetBattlezTask() == BZTASK_ASSIGNED_TARGET) {
            Coord arrival = u->ArrivalCell();
            if (arrival.m_x == targetPlayer && arrival.m_y == targetUnit) {
                ok = false;
            }
        }
        if (u->GetBattlezTask() == BZTASK_ASSIGNED_TARGET) {
            Coord arrival = u->ArrivalCell();
            if (!(arrival.m_x == targetPlayer && arrival.m_y == targetUnit) && (rand() % 3) != 0) {
                ok = false;
            }
        }
        if (ok == false) {
            continue;
        }
        Coord current = ScreenTile(u);
        if (u->GetBattlezTask() == BZTASK_ADVANCE && u->GetTargetTeam() != -1) {
            Coord marker = m_ctx->m_players[u->GetTargetTeam()].GetBattlezConfig()->GetBaseTile();
            i32 dx = marker.m_x - current.m_x;
            i32 dy = marker.m_y - current.m_y;
            dx = abs(dx);
            dy = abs(dy);

            if (SquaredDistance(dx, dy) <= 0x19) {
                ok = false;
            }
        }
        if (ok == false) {
            continue;
        }
        u->m_arrivalCell.m_x = targetPlayer;
        u->SetBattlezTask(BZTASK_ASSIGNED_TARGET);
        u->m_arrivalCell.m_y = targetUnit;
        u->SetAiState(AISTATE_ATTACK);
        u->SetRouteBlockedMask(0xd87);
        u->SetRoutePassableMask(0);
    }
    return 1;
}

RVA(0x00030990, 0x11b)
i32 CBattlezMapConfig::TrySeedSpawnAt(i32 ax, i32 ay) {
    i32 occupied = 0;
    CGrunt** units = m_triggerMgr->PlayerUnits(m_playerIndex);
    for (i32 unitsRemaining = TM_UNITS_PER_PLAYER; unitsRemaining != 0; unitsRemaining--) {
        if (*units != NULL) {
            occupied++;
        }
        units++;
    }
    if (occupied >= m_ctx->m_players[m_playerIndex].GetMaxGruntz()) {
        return 0;
    }
    i32 cell = m_triggerMgr->PlaceObject(
        m_playerIndex,
        (ax << TILE_SHIFT_PX) + TILE_HALF_PX,
        (ay << TILE_SHIFT_PX) + TILE_HALF_PX,
        0x186a0,
        GRUNT_ENTRANCE_RESURRECT,
        IDX(m_ctx->m_players[m_playerIndex].GetColor()),
        0,
        0,
        0x11,
        0,
        0,
        0,
        NULL
    );
    if (cell == -1) {
        return 0;
    }
    CGrunt* unit = m_ctx->GetTriggerMgr()->UnitAt(m_playerIndex, cell);
    if (unit == NULL) {
        return 0;
    }
    unit->m_aiType = AI_BATTLEZ_PATH;
    UNSET_COORD(unit->m_arrivalCell);
    unit->SetTargetTeam(-1);
    UNSET_COORD(unit->m_unusedBattleCell);
    unit->SetAiState(AISTATE_SEEK);
    UNSET_COORD(unit->m_defenderPx);
    unit->SetDefenderPickupType(PICKUP_NONE);
    unit->SetDefenderQueuePosition(0);
    unit->ResetDwell();
    unit->m_blockedVoicePending = true;
    unit->SetBattlezTask(BZTASK_ADVANCE);
    return 1;
}

// @identity-TODO BattlezMapConfigAcceptAlwaysSixArgs - the surviving external
// thunk and `ret 24` prove six callee-popped dwords, but no use survives to
// distinguish a static callback from a six-argument member.
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00030b00, 0x8)
i32 __stdcall BattlezMapConfigAcceptAlwaysSixArgs(i32, i32, i32, i32, i32, i32) {
    return 1;
}

RVA(0x00030b20, 0x328)
i32 CBattlezMapConfig::PathToNearestGoal(CGrunt* unit, i32 col, i32 row) {
    i32 bestDist = INT_MAX;
    i32 bestX = col;
    i32 bestY = row;
    Coord goal = ScreenTile(unit);

    CTileTriggerLogic* cell;

    if (m_board->CellTypeAt(col, row) == TILEKIND_PYRAMID_LATCH_A) {
        cell = m_cellQuery->m_latchedLeaf;
    } else {
        cell = m_cellQuery->FindLogic(CellKey(col, row), TRIGID_ANY);
    }
    if (cell != NULL) {

        const i32* p;
        for (p = cell->GetLinkKeys(); p - cell->GetLinkKeys() < 24; p++) {
            i32 node = *p;
            if (node == 0) {
                break;
            }
            CTileTriggerSwitchLogic* rec = m_cellQuery->FindSwitchLogic(node, TRIGID_ANY);
            if (rec != NULL) {
                i32 cx = rec->GetTileX();
                i32 cy = rec->GetTileY();
                if (IsCoordOccupied(unit, cx, cy) != 0) {
                    return 1;
                }
            }
        }

        for (p = cell->GetLinkKeys(); p - cell->GetLinkKeys() < 24; p++) {
            i32 node = *p;
            if (node == 0) {
                break;
            }
            CTileTriggerSwitchLogic* rec = m_cellQuery->FindSwitchLogic(node, TRIGID_ANY);
            if (rec != NULL) {
                i32 cx = rec->GetTileX();
                i32 cy = rec->GetTileY();
                i32 dx = cx - goal.m_x;
                i32 dy = cy - goal.m_y;
                dx = abs(dx);
                dy = abs(dy);
                i32 dist = SquaredDistance(dx, dy);
                if (dist < bestDist) {
                    bestX = cx;
                    bestY = cy;
                    bestDist = dist;
                }
            }
        }
    }
    if (bestDist == INT_MAX) {
        return 0;
    }
    if (IsCoordOccupied(unit, bestX, bestY) != 0) {
        return 0;
    }
    CPtrList list(10);

    i32 flags = BATTLEZ_ROUTE_ALL_TOOLS;
    PickupType er = unit->m_activePickupType;
    if (unit->ResolveEquippedToolType(er) == PICKUP_WINGZ) {
        flags = BATTLEZ_ROUTE_ALL_TOOLS_WINGZ;
    }
    if (unit->ResolveEquippedToolType(er) == PICKUP_TOOB) {
        flags |= BATTLEZ_ROUTE_TOOB_TRAVERSAL;
    }
    Coord start = ScreenTile(unit);
    if ((m_board)->FindPathWithEndpointOverrides(
            start.m_x,
            start.m_y,
            bestX,
            bestY,
            &list,
            1,
            0x98f,
            flags
        )
        != 0) {
        if (!list.IsEmpty()) {
            RECYCLE_HEAD_COORD(list)
            if (!list.IsEmpty()) {

                unit->RecycleCoords();

                POSITION pp = list.GetHeadPosition();
                while (pp != NULL) {
                    unit->AddTailCoord(static_cast<Coord*>(list.GetNext(pp)));
                }
                Coord* tail = unit->GetTailCoord();
                unit->m_entrancePx.Set(
                    (tail->m_x << TILE_SHIFT_PX) + TILE_HALF_PX,
                    (tail->m_y << TILE_SHIFT_PX) + TILE_HALF_PX
                );
                unit->SetAiState(AISTATE_RETREAT);
                return 1;
            }
        }
    } else {
        PathToNearestCandidate(unit, true, bestX, bestY);
    }
    return 0;
}

// @early-stop
RVA(0x00030f20, 0x16d)
Coord* CBattlezMapConfig::PickSpawnCoord(Coord* o, CGrunt* unit, i32 kind) {
    if (kind < 0 || kind >= 4) {
        CGameObject* lvl = unit->m_object;
        i32 sx = lvl->m_screenX >> TILE_SHIFT_PX;
        i32 sy = lvl->m_screenY >> TILE_SHIFT_PX;
        o->Set(sx, sy);
        return o;
    }
    CGameObject* lvl = unit->m_object;
    i32 rx = lvl->m_screenX >> TILE_SHIFT_PX;
    i32 ry = lvl->m_screenY >> TILE_SHIFT_PX;
    CPtrArray* coords = &m_ctx->m_players[kind].GetBattlezConfig()->m_attackWaypoints;
    i32 count = coords->GetSize();
    if (count != 0) {
        i32 r = rand() % count;
        for (i32 k = 0; k < count; k++) {
            CTriggerMgr* grid = m_triggerMgr;
            i32 cell = m_playerIndex;
            Coord cand = *static_cast<Coord*>(coords->GetAt(r));
            b32 ok = true;
            for (i32 j = 0; j < TM_UNITS_PER_PLAYER; j++) {
                CGrunt* u = grid->UnitAt(cell, j);
                if (u != NULL && !u->CoordsEmpty()) {
                    Coord node = *u->GetTailCoord();
                    if (node == cand) {
                        ok = false;
                    }
                }
            }
            if (ok != false) {
                *o = cand;
                return o;
            }
            r = (r + 1) % count;
        }
        r = rand() % count;
        Coord* cand = static_cast<Coord*>(coords->GetAt(r));
        rx = cand->m_x;
        ry = cand->m_y;
    }
    o->Set(rx, ry);
    return o;
}

template CString& zDArray<CString>::operator[](i32 i);
RVA_COMPGEN(0x000310f0, 0x8d, ??A?$zDArray@VCString@@@@QAEAAVCString@@H@Z)

template void FreeNodePool<Coord>::Push(void* p);
RVA_COMPGEN(0x000311b0, 0x14, ?Push@?$FreeNodePool@UCoord@@@@QAEXPAX@Z)

RVA(0x000311e0, 0x4c)
void CDDrawWorkerHost::SnapToTileCenter(Coord* out, i32 x, i32 y) {
    Coord result;
    i32 sx = m_shiftX;
    i32 sy = m_shiftY;
    result.Set(x >> sx, y >> sy);
    result.m_x <<= sx;
    result.m_y <<= sy;
    result.m_x += m_tileWidthPx / 2;
    result.m_y += m_tileHeightPx / 2;
    *out = result;
}

RVA_COMPGEN(0x000312a0, 0x74, ?get@_zdvec@@IAEPAXH@Z)
