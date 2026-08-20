#include <rva.h>

#include <Gruntz/BattlezMapConfig.h>

#include <Mfc.h>
#include <MfcNoInline.h>
#include <MfcWin.h>

#include <Bute/ButeMgr.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawWorkerHost.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/BattlezDifficulty.h>
#include <Gruntz/BattlezIntervalMs.h>
#include <Gruntz/BattlezRouteMaskPreset.h>
#include <Gruntz/BattlezTask.h>
#include <Gruntz/BrickTileId.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/EnemyAiType.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntAiState.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntPuddle.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GruntzPlayer.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/MapMgr.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/Play.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/StaminaPct.h>
#include <Gruntz/TileActionEvent.h>
#include <Gruntz/TileCollisionKind.h>
#include <Gruntz/TileTriggerContainer.h>
#include <Gruntz/TileTriggerLogic.h>
#include <Gruntz/TileTriggerSwitchLogic.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/UserLogic.h>
#include <Io/FileMem.h>
#include <Wap32/TileGeometry.h>
#include <Wap32/zBitVec.h>
#include <Wap32/ZVec.h>
#include <Wwd/WwdFile.h>

#include <limits.h>
#include <math.h>
#include <new>
#include <stdlib.h>
#include <string.h>

DATA(0x001e96ec)
const float g_diffScale = 0.01f;

DATA(0x0020ccc0)
i32 g_spawnCfg = 0x98f;
DATA(0x0022b6dc)
i32 g_stepRun;
DATA(0x0022b730)
i32 g_stepCol;
DATA(0x0022b734)
i32 g_stepRow;
DATA(0x0022b738)
i32 g_diffTier;

// The tenth `.CRT$XC` slot of this compiland (0x2085ec -> 0x0002d7e0, a bare
// `ret`). GruntDirectionCell's DEFAULT ctor is empty, and cl 5.0 emits one XC
// slot with a body of exactly `c3` for an array of such objects - the loop is
// deleted, so array and single object are byte-indistinguishable in .text and
// only the .bss extent separates them. 3 * 0xc = 0x24 = 0x0022b73c..0x0022b760
// exactly, and the slot is last, so the definition follows the scalars above.
// The next compiland's own cell block starts at 0x0022b760, which closes the
// extent from the far side. Nothing references it - like the nine singles above
// it is dead data that only its initializer touches - so the name claims only
// the family and that it is spare, not a purpose the evidence does not prove.
RVA_DYNINIT(0x0002d7c0, 0x5, s_gruntDirSpare)
RVA_DYNINIT(0x0002d7e0, 0x20, s_gruntDirSpare)
DATA(0x0022b73c)
static GruntDirectionCell s_gruntDirSpare[3];

static inline CGameObject* ListGetFirst(CDDrawChildGroup* list) {
    list->m_walkCursor = list->m_list.GetHeadPosition();
    if (list->m_walkCursor == NULL) {
        return 0;
    }
    return static_cast<CGameObject*>(list->m_list.GetNext(list->m_walkCursor));
}

static inline CGameObject* ListGetNext(CDDrawChildGroup* list) {
    if (list->m_walkCursor == NULL) {
        return 0;
    }
    return static_cast<CGameObject*>(list->m_list.GetNext(list->m_walkCursor));
}

// @early-stop
RVA(0x00024dc0, 0x158)
CBattlezMapConfig::CBattlezMapConfig()
    : m_routeClockLo(0), m_routeClockHi(0), m_routeWindowLo(0), m_routeWindowHi(0) {
    m_ownerId = 0;
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
// ebx and edi are transposed against retail (retail keeps the object cursor in
// ebx and the shared 0 in edi); that accounts for about two thirds of the
// residual. The rest is the g_diffScale multiply, which retail folds into
// `fmul m32` while cl preloads it with fld/fmulp.
RVA(0x00025020, 0x984)
i32 CBattlezMapConfig::LoadConfig(CGruntzMgr* mgr, i32 id, i32 diff) {

    m_gruntCreationTime = 0;
    m_spawnTimer = 0;
    m_spawnLastFire = 0;
    m_resourceCreationTime = 0;
    m_repickLastFire = 0;
    m_repickTimer = 0;
    m_ctx = mgr;
    m_ownerId = id;
    m_triggerMgr = mgr->m_cmdGrid;
    m_board = mgr->m_tileGrid;
    m_play = static_cast<CPlay*>(mgr->m_curState);
    m_cellQuery = m_play->m_beginMarker;
    m_active = 1;

    m_gruntCreationTime = g_buteMgr.GetDwordDef("Battlez", "GruntCreationTime", 10000);
    m_resourceCreationTime = g_buteMgr.GetDwordDef("Battlez", "ResourceCreationTime", 10000);
    m_gauntletzChance = g_buteMgr.GetDwordDef("Battlez", "GauntletzChance", 50);
    m_shovelzChance = g_buteMgr.GetDwordDef("Battlez", "ShovelzChance", 50);
    m_spyzChance = g_buteMgr.GetDwordDef("Battlez", "SpyzChance", 50);
    m_brickzChance = g_buteMgr.GetDwordDef("Battlez", "BrickzChance", 50);
    m_gooberzChance = g_buteMgr.GetDwordDef("Battlez", "GooberzChance", 50);
    m_gruntRatio = g_buteMgr.GetDwordDef("Battlez", "GruntRatio", 25);
    m_defenderChance = g_buteMgr.GetDwordDef("Battlez", "DefenderChance", 50);

    for (CGameObject* cur = ListGetFirst(mgr->m_world->m_childGroup); cur != NULL;
         cur = ListGetNext(mgr->m_world->m_childGroup)) {
        if (cur->m_animWorker->m_notify == &CreateGruntCreationPoint && cur->m_smarts == id) {
            CoordPoolNode* p = static_cast<CoordPoolNode*>(g_coordPool.m_freeHead);
            Coord* slot = 0;
            if (p->m_next != NULL) {
                slot = &p->m_coord;
                g_coordPool.m_freeHead = p->m_next;
            }
            slot->m_x = cur->m_screenX / 32;
            slot->m_y = cur->m_screenY / 32;
            m_candArray.SetAtGrow(m_candArray.GetSize(), slot);
        }
    }

    for (CGameObject* cur2 = ListGetFirst(mgr->m_world->m_childGroup); cur2 != NULL;
         cur2 = ListGetNext(mgr->m_world->m_childGroup)) {
        if (cur2->m_animWorker->m_notify == &CreateExitTrigger && cur2->m_smarts == id) {
            m_marker.m_x = cur2->m_screenX / 32;
            m_marker.m_y = cur2->m_screenY / 32;
            break;
        }
    }

    for (CGameObject* cur3 = ListGetFirst(mgr->m_world->m_childGroup); cur3 != NULL;
         cur3 = ListGetNext(mgr->m_world->m_childGroup)) {
        if (cur3->m_animWorker->m_notify == &CreateWayPoint && cur3->m_smarts == id) {
            CoordPoolNode* p = static_cast<CoordPoolNode*>(g_coordPool.m_freeHead);
            Coord* slot = 0;
            if (p->m_next != NULL) {
                slot = &p->m_coord;
                g_coordPool.m_freeHead = p->m_next;
            }
            slot->m_x = cur3->m_screenX >> TILE_SHIFT_PX;
            slot->m_y = cur3->m_screenY >> TILE_SHIFT_PX;
            m_attackWaypoints.SetAtGrow(m_attackWaypoints.GetSize(), slot);
            cur3->m_flags |= 0x10000;
        }
    }

    switch (static_cast<BattlezDifficulty>(diff)) {
        case BZDIFF_EASY: {
            g_buteMgr.GetIntDef("Battlez", "EasyDifficulty", 100);
            g_diffTier = 20;
            break;
        }
        case BZDIFF_NORMAL: {
            i32 r = g_buteMgr.GetIntDef("Battlez", "NormalDifficulty", 50);
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
            i32 r = g_buteMgr.GetIntDef("Battlez", "HardDifficulty", 25);
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
    m_idleBurnRandX = m_board->m_width / 3;
    m_idleBurnRandY = m_board->m_width / 3;
    m_assignedTargetMaxDistance = m_board->m_width >> 2;
    m_roundRobinTick = 0;

    m_toolzPct = g_buteMgr.GetInt("Battlez", "ToolzPercent");
    m_toyzPct = m_toolzPct + g_buteMgr.GetInt("Battlez", "ToyzPercent");
    m_brickzPct = m_toyzPct + g_buteMgr.GetInt("Battlez", "BrickzPercent");
    // The brick CDF: RedBrick through BlackBrick.
    m_redBrickPct = g_buteMgr.GetInt("Battlez", "RedBrick");
    m_blueBrickPct = m_redBrickPct + g_buteMgr.GetInt("Battlez", "BlueBrick");
    m_goldBrickPct = m_blueBrickPct + g_buteMgr.GetInt("Battlez", "GoldBrick");
    m_blackBrickPct = m_goldBrickPct + g_buteMgr.GetInt("Battlez", "BlackBrick");
    // The toy CDF: one running total per key, from BabyWalkerz to Yoyoz.
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

    // The tool CDF: Bombz through Wingz, the last entry being the grand total.
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

    m_routeClockLo = 0;
    m_routeWindowLo = 0;
    m_routeClockHi = 0;
    m_routeWindowHi = 0;
    return 1;
}

RVA(0x00025c20, 0x55)
i32 CBattlezMapConfig::StepAllRowSpawns() {
    if (g_gameReg->m_options[m_ownerId].m_humanControlled == 0
        && g_gameReg->m_options[m_ownerId].m_liveGate != 0) {
        for (i32 i = 0; i < m_candArray.GetSize(); i++) {
            this->StepRowSpawn(0);
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
            CoordPoolNode* node = g_coordPool.NodeOf(p);
            node->m_next = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = node;
        }
    }
    m_candArray.SetSize(0, -1);

    for (i = 0; i < m_attackWaypoints.GetSize(); i++) {
        CoordPoolNode* node = g_coordPool.NodeOf(m_attackWaypoints[i]);
        node->m_next = g_coordPool.m_freeHead;
        g_coordPool.m_freeHead = node;
    }
    m_attackWaypoints.SetSize(0, -1);

    m_reserved104.SetSize(0, -1);
    m_reserved118.SetSize(0, -1);
    m_reserved13c = 0;
}

// @early-stop
// Instruction count, branch sequence and branch targets all agree; the residue is
// call-setup scheduling - retail loads `ecx = &g_typeColl` BETWEEN the two argument
// loads at each of the seven GetNameRecord sites, and interleaves the three trailing
// `timer += g_frameDelta` reads differently.
RVA(0x00025d90, 0x580)
i32 CBattlezMapConfig::StepBoard() {
    if (m_active == 0) {
        return 1;
    }
    if (m_ctx->m_cmdGrid == NULL) {
        return 0;
    }
    if (m_spawnTimer - m_spawnLastFire > m_gruntCreationTime) {
        StepRowSpawn(1);
        m_spawnLastFire = m_spawnTimer;
    }

    i32 mn = BATTLEZ_QUEUE_POSITION_UNSET;
    CGrunt** row = &m_triggerMgr->m_grid[m_ownerId * BATTLEZ_UNIT_SLOT_COUNT];
    for (i32 s = BATTLEZ_UNIT_SLOT_COUNT; s != 0; s--) {
        CGrunt* u = *row;
        if (u != NULL && u->m_defenderState == AISTATE_RETURN && u->m_defenderQueuePosition < mn) {
            mn = u->m_defenderQueuePosition;
        }
        row++;
    }
    if (mn != 0 && mn != BATTLEZ_QUEUE_POSITION_UNSET) {
        for (i32 k = 0; k < BATTLEZ_UNIT_SLOT_COUNT; k++) {
            CGrunt* u = m_triggerMgr->m_grid[m_ownerId * BATTLEZ_UNIT_SLOT_COUNT + k];
            if (u != NULL && u->m_defenderState == AISTATE_RETURN) {
                u->m_defenderQueuePosition -= mn;
            }
        }
    }

    i32 forced = 0;
    CGrunt* forcedUnit = 0;
    if (m_repickTimer - m_repickLastFire > m_resourceCreationTime) {
        i32 r = rand() % 15;
        CGrunt* u = m_triggerMgr->m_grid[m_ownerId * 15 + r];
        forcedUnit = u;
        forced = 0;
        if (u != NULL && u->m_defenderState == AISTATE_RETURN && u->m_defenderQueuePosition == 0) {
            forced = 1;
        }
        // ONE if/else: retail enters the retask loop for `forced` OR for the 1-in-10
        // idle roll (two `j.. 0x25f31` into the same block), which is why `forced` is
        // still unknown inside it and the `unit = forcedUnit` override survives.
        if (!forced && rand() % 10 != 0) {
            i32 r2 = rand() % 15;
            CGrunt* u2 = m_triggerMgr->m_grid[m_ownerId * 15 + r2];
            if (u2 != NULL) {
                ChooseIdleBehavior(u2);
            }
        } else {
            for (i32 b = 0; b < 15; b++) {
                CGrunt* unit = m_triggerMgr->m_grid[m_ownerId * 15 + b];
                if (forced) {
                    unit = forcedUnit;
                }
                if (unit == NULL) {
                    continue;
                }
                CGameObject* lvl = unit->m_object;
                if (lvl->m_screenX != unit->m_lastTilePx.m_x) {
                    continue;
                }
                if (lvl->m_screenY != unit->m_lastTilePx.m_y) {
                    continue;
                }
                if (unit->m_entranceCommitted == 0) {
                    continue;
                }
                if (unit->m_deathAnimStarted != 0) {
                    continue;
                }
                if (unit->m_entranceActive != 0) {
                    continue;
                }
                if (unit->m_poweredUp != 0) {
                    continue;
                }
                bool eq;
                eq = (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_actKey)), "I") == 0);
                if (eq) {
                    continue;
                }
                eq = (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_actKey)), "G") == 0);
                if (eq) {
                    continue;
                }
                eq = (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_actKey)), "L") == 0);
                if (eq) {
                    continue;
                }
                eq = (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_actKey)), "P") == 0);
                if (eq) {
                    continue;
                }
                eq = (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_actKey)), "J") == 0);
                if (eq) {
                    continue;
                }
                eq = (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_actKey)), "C") == 0);
                if (eq) {
                    continue;
                }
                eq = (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_actKey)), "R") == 0);
                if (eq) {
                    continue;
                }
                if (unit->m_defenderState != AISTATE_RETURN) {
                    continue;
                }
                if (unit->m_defenderQueuePosition != 0) {
                    continue;
                }

                PickupType mode = unit->m_defenderPickupType;
                if (PathCrossesMarkedTile(unit) != 0) {
                    unit->m_defenderState = AISTATE_RETREAT;
                } else {
                    unit->m_defenderState = AISTATE_SEEK;
                }
                unit->LoadPickupSprites(unit->m_defenderPickupType, 1, 0, 0, 1);

                switch (mode) {
                    case PICKUP_WINGZ: {
                        if (unit->CoordCount() != 0) {
                            CoordNode* n = unit->CoordHead();
                            while (n != NULL) {
                                CoordNode* cur = n;
                                n = n->m_next;
                                if (cur->m_coord != NULL) {
                                    CoordPoolNode* node = g_coordPool.NodeOf(cur->m_coord);
                                    node->m_next = g_coordPool.m_freeHead;
                                    g_coordPool.m_freeHead = node;
                                }
                            }
                            unit->m_coordList.RemoveAll();
                        }
                        break;
                    }
                    case PICKUP_TOOB: {
                        if (unit->CoordCount() != 0) {
                            CoordNode* n = unit->CoordHead();
                            while (n != NULL) {
                                CoordNode* cur = n;
                                n = n->m_next;
                                if (cur->m_coord != NULL) {
                                    CoordPoolNode* node = g_coordPool.NodeOf(cur->m_coord);
                                    node->m_next = g_coordPool.m_freeHead;
                                    g_coordPool.m_freeHead = node;
                                }
                            }
                            unit->m_coordList.RemoveAll();
                        }
                        break;
                    }
                }

                // One decrement of THIS unit's queue position per still-returning
                // row-mate, floored at 0 (retail `dec eax; jns; xor eax,eax`), then
                // an immediate return - the row step and the clocks are skipped.
                for (i32 c = 0; c < 15; c++) {
                    CGrunt* mate = m_triggerMgr->m_grid[m_ownerId * 15 + c];
                    if (mate != NULL && mate->m_defenderState == AISTATE_RETURN) {
                        i32 q = unit->m_defenderQueuePosition - 1;
                        if (q < 0) {
                            q = 0;
                        }
                        unit->m_defenderQueuePosition = q;
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

// @early-stop
// Two residues: retail keeps the candidate index in ebp and the cursor on the
// stack (cl does the reverse), and its budget product is A*(B*C) - written that
// way cl folds A into an `fimul`, which is further from retail than (A*B)*C.
RVA(0x00026470, 0x29d)
i32 CBattlezMapConfig::StepRowSpawn(i32 allowReserved) {
    i32 occupied = 0;
    CGrunt** row = &m_triggerMgr->m_grid[m_ownerId * 15];
    for (i32 c = 15; c != 0; c--) {
        if (*row != NULL) {
            occupied++;
        }
        row++;
    }
    if (occupied >= m_ctx->m_options[m_ownerId].m_comboSel) {
        return 1;
    }
    i32 n = m_candArray.GetSize();
    if (n <= 0) {
        return 1;
    }
    Coord** cands = MfcPtrArrayData<Coord>(m_candArray);
    Coord* cand = 0;
    i32 i = 0;
    BrickzCell tileRec;
    for (;;) {
        cand = cands[i];
        i32 usable = 1;
        if (cand != NULL) {

            const i32* tilePtr = &m_board->m_rowInts[cand->m_y][cand->m_x * 7];
            memcpy(&tileRec, tilePtr, sizeof(tileRec));
            usable = 1;
            if (tileRec.m_flags & BRICKZ_CELL_OCCUPIED) {

                if (tileRec.m_occupantIdBytes[1] == m_ownerId) {
                    usable = 0;
                }
                if (allowReserved == 0) {
                    usable = 0;
                }
            }
            if (usable) {
                break;
            }
        }
        i++;
        if (i < m_candArray.GetSize()) {
            continue;
        }
        return 1;
    }
    Coord screen;
    m_ctx->m_world->m_level->m_mainPlane
        ->SnapToTileCenter(&screen, cand->m_x << TILE_SHIFT_PX, cand->m_y << TILE_SHIFT_PX);
    i32 cell;
    if (allowReserved != 0) {
        cell = m_ctx->m_cmdGrid->PlaceObject(
            m_ownerId,
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
            0
        );
    } else {
        cell = m_ctx->m_cmdGrid->PlaceObject(
            m_ownerId,
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
            0
        );
    }
    if (cell == -1) {
        return 0;
    }

    CGrunt* unit = m_ctx->m_cmdGrid->m_grid[cell + m_ownerId * TM_GRID_COLS];
    if (unit == NULL) {
        return 0;
    }

    i32 roll = rand() % 100;
    i32 freeCount = 0;
    CGrunt** r2 = &m_triggerMgr->m_grid[m_ownerId * 15];
    for (i32 k = 15; k != 0; k--) {
        CGrunt* g = *r2;
        if (g != NULL && g->m_battleState == BZTASK_UNASSIGNED) {
            freeCount++;
        }
        r2++;
    }
    i32 budget = static_cast<i32>(
        (static_cast<double>(m_ctx->m_options[m_ownerId].m_comboSel)
         * static_cast<double>(m_gruntRatio) * g_diffScale)
    );
    if (roll >= m_defenderChance || freeCount >= budget) {
        unit->m_battleState = BZTASK_ADVANCE;
    } else {
        unit->m_battleState = BZTASK_UNASSIGNED;
    }
    unit->m_arrivalState = AI_BATTLEZ_PATH;
    unit->m_defenderState = AISTATE_SEEK;
    unit->m_arrivalCell.m_x = -1;
    unit->m_unusedBattleCell.m_x = -1;
    unit->m_defenderPx.m_x = -1;
    unit->m_arrivalCell.m_y = -1;
    unit->m_unusedBattleCell.m_y = -1;
    unit->m_defenderPx.m_y = -1;
    unit->m_targetTeam = -1;
    unit->m_defenderPickupType = PICKUP_NONE;
    unit->m_defenderQueuePosition = 0;
    unit->m_dwell = 0;
    unit->m_blockedVoicePending = 1;
    return 1;
}

// @early-stop
// Branch sequence, ret count and the whole referent multiset AGREE (460/460
// branches, 8/8 rets, 15/15 ??0CRect@@QAE@HHHH@Z calls, reloc_multiset clean),
// and the labelled blocks come out in retail's order.  What is left is /O2
// register colouring at scale: retail carries `this` in ecx across the outer
// loop and reloads it in the latch, we reload it at the loop head, and retail's
// frame is one dword larger.
RVA(0x000267c0, 0x2850)
i32 CBattlezMapConfig::StepRowUnits() {
    m_roundRobinTick++;
    CGrunt* unit;
    i32 hit;
    char eq;
    i32 cell;
    Coord scratch;
    for (i32 i = 0; i < 15; i++) {
        unit = m_triggerMgr->m_grid[m_ownerId * 15 + i];
        if (unit != NULL) {
            if (static_cast<i64>(g_frameTime) - unit->m_holdAnchor64 < unit->m_holdWindow64) {
                return 1;
            }
        }
        if (unit != NULL) {
            if (unit->CoordCount() != 0) {
                Coord* hc = (unit->CoordHead())->m_coord;
                scratch.m_x = hc->m_x;
                scratch.m_x = m_board->m_width;
                scratch.m_y = hc->m_y;
            }
        }
        {
            {
                if (unit != NULL) {
                    if (static_cast<i64>(g_frameTime) - unit->m_arrivalReroll64
                        >= unit->m_arrivalRerollWindow64) {
                        RouteToNearbyPickup(unit);
                        if (unit->m_poweredUp != 0) {
                            eq =
                                (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_actKey)), "A")
                                 == 0);
                            if (eq) {
                                goto resetEntrance;
                            }
                        }
                        if (unit->CoordCount() != 0) {
                            Coord* ac = (unit->CoordHead())->m_coord;
                            i32 ax = ac->m_x;
                            i32 ay = ac->m_y;
                            Coord sp;
                            (static_cast<CUserLogic*>(unit))->GetScreenPos((&sp));
                            sp.m_x >>= 5;
                            sp.m_y >>= 5;
                            if (sp.m_x == ax && sp.m_y == ay) {
                                goto arriveHead;
                            }
                        }
                        {
                            PickupType st = unit->m_entranceReason;
                            if (st > PICKUP_EQUIPPABLE_LAST) {
                                st = unit->m_toolId;
                            }
                            if (st == PICKUP_BRICK && unit->m_battleState == BZTASK_UNASSIGNED) {
                                unit->m_battleState = BZTASK_CARRY_BRICK;
                                if (unit->CoordCount() != 0) {
                                    POSITION pos = unit->m_coordList.GetHeadPosition();
                                    if (pos != NULL) {
                                        do {
                                            Coord* d = static_cast<Coord*>(
                                                unit->CoordListOps()->NextData(pos)
                                            );
                                            if (d != NULL) {
                                                g_coordPool.Push(d);
                                            }
                                        } while (pos != NULL);
                                    }
                                    unit->m_coordList.RemoveAll();
                                }
                            }
                        }
                        if (unit->IsAtSavedScreenPos() != 0 && unit->m_entranceCommitted != 0
                            && unit->m_deathAnimStarted == 0 && unit->m_entranceActive == 0
                            && unit->m_poweredUp == 0) {
                            eq =
                                (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_actKey)), "I")
                                 == 0);
                            if (!eq) {
                                eq =
                                    (strcmp(
                                         (*g_typeColl.GetNameRecord(unit->m_objAux->m_actKey)),
                                         "G"
                                     )
                                     == 0);
                                if (!eq) {
                                    eq =
                                        (strcmp(
                                             (*g_typeColl.GetNameRecord(unit->m_objAux->m_actKey)),
                                             "L"
                                         )
                                         == 0);
                                    if (!eq) {
                                        eq =
                                            (strcmp(
                                                 (*g_typeColl.GetNameRecord(
                                                     unit->m_objAux->m_actKey
                                                 )),
                                                 "P"
                                             )
                                             == 0);
                                        if (!eq) {
                                            eq =
                                                (strcmp(
                                                     (*g_typeColl.GetNameRecord(
                                                         unit->m_objAux->m_actKey
                                                     )),
                                                     "J"
                                                 )
                                                 == 0);
                                            if (!eq) {
                                                eq =
                                                    (strcmp(
                                                         (*g_typeColl.GetNameRecord(
                                                             unit->m_objAux->m_actKey
                                                         )),
                                                         "C"
                                                     )
                                                     == 0);
                                                if (!eq) {
                                                    eq =
                                                        (strcmp(
                                                             (*g_typeColl.GetNameRecord(
                                                                 unit->m_objAux->m_actKey
                                                             )),
                                                             "R"
                                                         )
                                                         == 0);
                                                    if (!eq) {
                                                        PickupType st2 = unit->m_entranceReason;
                                                        if (st2 > PICKUP_EQUIPPABLE_LAST) {
                                                            st2 = unit->m_toolId;
                                                        }
                                                        if (st2 == PICKUP_BRICK
                                                            && unit->m_arrivalState == AI_DEFENDER
                                                            && unit->m_defenderState
                                                                   == AISTATE_BATTLEZ_ROUTE_TARGET) {
                                                            unit->LoadPickupSprites(
                                                                PICKUP_NONE,
                                                                1,
                                                                0,
                                                                0,
                                                                1
                                                            );
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
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
                            i32 tile;
                            if (static_cast<u32>(s2.m_x) < m_board->m_width
                                && static_cast<u32>(qy) < m_board->m_height) {
                                tile = m_board->m_rows[qy][s2.m_x].m_flags;
                            } else {
                                tile = 1;
                            }
                            if (!(tile & 4)) {
                                unit->m_arrivalCell.m_x = -1;
                                unit->m_battleState = BZTASK_ADVANCE;
                                unit->m_arrivalCell.m_y = -1;
                                if (unit->CoordCount() != 0) {
                                    POSITION pos = unit->m_coordList.GetHeadPosition();
                                    if (pos != NULL) {
                                        do {
                                            Coord* d = static_cast<Coord*>(
                                                unit->CoordListOps()->NextData(pos)
                                            );
                                            if (d != NULL) {
                                                g_coordPool.Push(d);
                                            }
                                        } while (pos != NULL);
                                    }
                                    unit->m_coordList.RemoveAll();
                                }
                                unit->m_routeMaskC = 0;
                                unit->m_defenderState = AISTATE_SEEK;
                            }
                        }
                        {
                            PickupType st = unit->m_entranceReason;
                            if (st > PICKUP_EQUIPPABLE_LAST) {
                                st = unit->m_toolId;
                            }
                            if (st != PICKUP_SPY && unit->m_battleState == BZTASK_CARRY_SPY) {
                                unit->m_arrivalCell.m_x = -1;
                                unit->m_battleState = BZTASK_ADVANCE;
                                unit->m_arrivalCell.m_y = -1;
                                if (unit->CoordCount() != 0) {
                                    POSITION pos = unit->m_coordList.GetHeadPosition();
                                    if (pos != NULL) {
                                        do {
                                            Coord* d = static_cast<Coord*>(
                                                unit->CoordListOps()->NextData(pos)
                                            );
                                            if (d != NULL) {
                                                g_coordPool.Push(d);
                                            }
                                        } while (pos != NULL);
                                    }
                                    unit->m_coordList.RemoveAll();
                                }
                                unit->m_routeMaskC = 0;
                                unit->m_defenderState = AISTATE_SEEK;
                            }
                        }
                        {
                            PickupType st = unit->m_entranceReason;
                            if (st > PICKUP_EQUIPPABLE_LAST) {
                                st = unit->m_toolId;
                            }
                            if (st == PICKUP_GOOBER) {
                                BattlezTask d8 = unit->m_battleState;
                                if (d8 != BZTASK_CARRY_GOOBER && d8 != BZTASK_ASSIGNED_TARGET) {
                                    if (unit->CoordCount() != 0) {
                                        POSITION pos = unit->m_coordList.GetHeadPosition();
                                        if (pos != NULL) {
                                            do {
                                                Coord* d = static_cast<Coord*>(
                                                    unit->CoordListOps()->NextData(pos)
                                                );
                                                if (d != NULL) {
                                                    g_coordPool.Push(d);
                                                }
                                            } while (pos != NULL);
                                        }
                                        unit->m_coordList.RemoveAll();
                                    }
                                    unit->m_arrivalCell.m_x = -1;
                                    unit->m_arrivalCell.m_y = -1;
                                    unit->m_battleState = BZTASK_CARRY_GOOBER;
                                }
                            }
                        }
                        {
                            PickupType st = unit->m_entranceReason;
                            if (st > PICKUP_EQUIPPABLE_LAST) {
                                st = unit->m_toolId;
                            }
                            if (st != PICKUP_GOOBER && unit->m_battleState == BZTASK_CARRY_GOOBER) {
                                unit->m_arrivalCell.m_x = -1;
                                unit->m_battleState = BZTASK_ADVANCE;
                                unit->m_arrivalCell.m_y = -1;
                                if (unit->CoordCount() != 0) {
                                    POSITION pos = unit->m_coordList.GetHeadPosition();
                                    if (pos != NULL) {
                                        do {
                                            Coord* d = static_cast<Coord*>(
                                                unit->CoordListOps()->NextData(pos)
                                            );
                                            if (d != NULL) {
                                                g_coordPool.Push(d);
                                            }
                                        } while (pos != NULL);
                                    }
                                    unit->m_coordList.RemoveAll();
                                }
                                unit->m_routeMaskC = 0;
                                unit->m_defenderState = AISTATE_SEEK;
                            }
                        }
                        if (unit->CoordCount() == 0) {
                            if (unit->m_defenderState == AISTATE_RETREAT) {
                                unit->m_defenderState = AISTATE_SEEK;
                            }
                        }
                        if (unit->m_defenderState == AISTATE_RETREAT) {
                            if (PathCrossesMarkedTile(unit) == 0) {
                                unit->m_defenderState = AISTATE_SEEK;
                            }
                        }
                        {
                            char ne;
                            ne =
                                (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_actKey)), "C")
                                 != 0);
                            if (ne) {
                                ne =
                                    (strcmp(
                                         (*g_typeColl.GetNameRecord(unit->m_objAux->m_actKey)),
                                         "R"
                                     )
                                     != 0);
                                if (ne) {
                                    ne =
                                        (strcmp(
                                             (*g_typeColl.GetNameRecord(unit->m_objAux->m_actKey)),
                                             "C"
                                         )
                                         != 0);
                                    if (ne) {
                                        ne =
                                            (strcmp(
                                                 (*g_typeColl.GetNameRecord(
                                                     unit->m_objAux->m_actKey
                                                 )),
                                                 "G"
                                             )
                                             != 0);
                                        if (ne) {
                                            ne =
                                                (strcmp(
                                                     (*g_typeColl.GetNameRecord(
                                                         unit->m_objAux->m_actKey
                                                     )),
                                                     "L"
                                                 )
                                                 != 0);
                                            if (ne) {
                                                ne =
                                                    (strcmp(
                                                         (*g_typeColl.GetNameRecord(
                                                             unit->m_objAux->m_actKey
                                                         )),
                                                         "P"
                                                     )
                                                     != 0);
                                                if (ne) {
                                                    ne =
                                                        (strcmp(
                                                             (*g_typeColl.GetNameRecord(
                                                                 unit->m_objAux->m_actKey
                                                             )),
                                                             "J"
                                                         )
                                                         != 0);
                                                    if (ne) {
                                                        if (unit->m_object->m_screenX
                                                                == unit->m_lastTilePx.m_x
                                                            && unit->m_object->m_screenY
                                                                   == unit->m_lastTilePx.m_y
                                                            && unit->m_entranceCommitted != 0
                                                            && unit->m_deathAnimStarted == 0
                                                            && unit->m_entranceActive == 0) {
                                                            RECT box;
                                                            Coord c1;
                                                            (static_cast<CUserLogic*>(unit))
                                                                ->GetScreenPos((&c1));
                                                            c1.m_y >>= 5;
                                                            c1.m_x >>= 5;
                                                            Coord c2;
                                                            (static_cast<CUserLogic*>(unit))
                                                                ->GetScreenPos((&c2));
                                                            c2.m_x >>= 5;
                                                            c2.m_y >>= 5;
                                                            Coord c3;
                                                            (static_cast<CUserLogic*>(unit))
                                                                ->GetScreenPos((&c3));
                                                            c3.m_y >>= 5;
                                                            c3.m_x >>= 5;
                                                            Coord c4;
                                                            (static_cast<CUserLogic*>(unit))
                                                                ->GetScreenPos((&c4));
                                                            c4.m_x >>= 5;
                                                            c4.m_y >>= 5;
                                                            box.left = c4.m_x - 4;
                                                            box.top = c3.m_y - 4;
                                                            box.right = c2.m_x + 4;
                                                            box.bottom = c1.m_y + 4;
                                                            Coord c5;
                                                            (static_cast<CUserLogic*>(unit))
                                                                ->GetScreenPos((&c5));
                                                            c5.m_x >>= 5;
                                                            c5.m_y >>= 5;
                                                            Coord c6;
                                                            (static_cast<CUserLogic*>(unit))
                                                                ->GetScreenPos((&c6));
                                                            c6.m_x >>= 5;
                                                            c6.m_y >>= 5;
                                                            Coord c7;
                                                            (static_cast<CUserLogic*>(unit))
                                                                ->GetScreenPos((&c7));
                                                            c7.m_y >>= 5;
                                                            c7.m_x >>= 5;
                                                            Coord c8;
                                                            (static_cast<CUserLogic*>(unit))
                                                                ->GetScreenPos((&c8));
                                                            c8.m_x >>= 5;
                                                            c8.m_y >>= 5;
                                                            i32 rowEnd = c5.m_y + 2;
                                                            i32 colEnd = c6.m_x + 2;
                                                            i32 rowBeg = c7.m_y - 1;
                                                            i32 colBeg = c8.m_x - 1;
                                                            CMapMgr* board = m_board;
                                                            CRect bounds(
                                                                0,
                                                                0,
                                                                board->m_width,
                                                                board->m_height
                                                            );
                                                            RECT clamp;
                                                            RECT* pb = &box;
                                                            if (pb != NULL) {
                                                                clamp.left = pb->left;
                                                                clamp.top = pb->top;
                                                                clamp.right = pb->right + 1;
                                                                clamp.bottom = pb->bottom + 1;
                                                            } else {
                                                                clamp = CRect(
                                                                    0,
                                                                    0,
                                                                    board->m_width,
                                                                    board->m_height
                                                                );
                                                            }
                                                            if (!IntersectRect(
                                                                    &board->m_bounds,
                                                                    &clamp,
                                                                    &bounds
                                                                )) {
                                                                board->m_bounds = clamp;
                                                            }
                                                            board->m_gridW = board->m_bounds.right
                                                                             - board->m_bounds.left;
                                                            board->m_gridH = board->m_bounds.bottom
                                                                             - board->m_bounds.top;
                                                            for (i32 row = rowBeg; row < rowEnd;
                                                                 row++) {
                                                                CMapMgr* b = m_board;
                                                                for (i32 col = colBeg; col < colEnd;
                                                                     col++) {
                                                                    if (static_cast<u32>(col)
                                                                            < b->m_width
                                                                        && static_cast<u32>(row)
                                                                               < b->m_height) {
                                                                        if (b->m_rows[row][col]
                                                                                .m_flags
                                                                            & 0x1000000) {
                                                                            goto perimSweep;
                                                                        }
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    reclampJoin: {
                        CMapMgr* bd = m_board;
                        CRect r1(0, 0, bd->m_width, bd->m_height);
                        RECT rc = CRect(0, 0, bd->m_width, bd->m_height);
                        RECT* rcDst = &bd->m_bounds;
                        if (!IntersectRect(rcDst, &rc, &r1)) {
                            *rcDst = rc;
                        }
                        bd->m_gridW = rcDst->right - rcDst->left;
                        bd->m_gridH = rcDst->bottom - rcDst->top;
                    }
                        {
                            i32 special = 1;
                            if (unit->m_object->m_screenX != unit->m_lastTilePx.m_x
                                || unit->m_object->m_screenY != unit->m_lastTilePx.m_y) {
                                special = 0;
                            }
                            if (unit->m_entranceCommitted == 0) {
                                special = 0;
                            }
                            if (unit->m_deathAnimStarted != 0) {
                                special = 0;
                            }
                            if (unit->m_entranceActive != 0) {
                                special = 0;
                            }
                            eq =
                                (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_actKey)), "I")
                                 == 0);
                            if (eq) {
                                special = 0;
                            }
                            eq =
                                (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_actKey)), "G")
                                 == 0);
                            if (eq) {
                                special = 0;
                            }
                            eq =
                                (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_actKey)), "L")
                                 == 0);
                            if (eq) {
                                special = 0;
                            }
                            eq =
                                (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_actKey)), "P")
                                 == 0);
                            if (eq) {
                                return 0;
                            }
                            eq =
                                (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_actKey)), "J")
                                 == 0);
                            if (eq) {
                                special = 0;
                            }
                            eq =
                                (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_actKey)), "C")
                                 == 0);
                            if (eq) {
                                special = 0;
                            }
                            eq =
                                (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_actKey)), "R")
                                 == 0);
                            if (eq) {
                                special = 0;
                            }
                            if (unit->m_gruntKind == GRUNT_GHOST) {
                                special = 0;
                            }
                            if (special != 0) {
                                if (unit->m_poweredUp != 0 && unit->m_neighborValid == 0
                                    && unit->m_combatActive == 0
                                    && unit->m_stamina >= STAMINA_FULL) {
                                    if (unit->FindGridNeighbor(0) != NULL) {
                                        return 1;
                                    }
                                }
                            }
                        }
                        if (unit->IsAtSavedScreenPos() != 0 && unit->m_entranceCommitted != 0
                            && unit->m_deathAnimStarted == 0 && unit->m_entranceActive == 0
                            && unit->m_poweredUp == 0) {
                            eq =
                                (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_actKey)), "I")
                                 == 0);
                            if (!eq) {
                                eq =
                                    (strcmp(
                                         (*g_typeColl.GetNameRecord(unit->m_objAux->m_actKey)),
                                         "G"
                                     )
                                     == 0);
                                if (!eq) {
                                    eq =
                                        (strcmp(
                                             (*g_typeColl.GetNameRecord(unit->m_objAux->m_actKey)),
                                             "L"
                                         )
                                         == 0);
                                    if (!eq) {
                                        eq =
                                            (strcmp(
                                                 (*g_typeColl.GetNameRecord(
                                                     unit->m_objAux->m_actKey
                                                 )),
                                                 "P"
                                             )
                                             == 0);
                                        if (!eq) {
                                            eq =
                                                (strcmp(
                                                     (*g_typeColl.GetNameRecord(
                                                         unit->m_objAux->m_actKey
                                                     )),
                                                     "J"
                                                 )
                                                 == 0);
                                            if (!eq) {
                                                eq =
                                                    (strcmp(
                                                         (*g_typeColl.GetNameRecord(
                                                             unit->m_objAux->m_actKey
                                                         )),
                                                         "C"
                                                     )
                                                     == 0);
                                                if (!eq) {
                                                    eq =
                                                        (strcmp(
                                                             (*g_typeColl.GetNameRecord(
                                                                 unit->m_objAux->m_actKey
                                                             )),
                                                             "R"
                                                         )
                                                         == 0);
                                                    if (!eq) {
                                                        for (i32 j = 0; j < 4; j++) {
                                                            if (j != m_ownerId) {
                                                                for (i32 k = 0; k < 15; k++) {
                                                                    CGrunt* other =
                                                                        m_triggerMgr
                                                                            ->m_grid[j * 15 + k];
                                                                    if (other != NULL) {
                                                                        if (unit->RectContains(
                                                                                other->m_object
                                                                                    ->m_screenX,
                                                                                other->m_object
                                                                                    ->m_screenY
                                                                            )
                                                                            != 0) {
                                                                            if (unit->m_gruntKind
                                                                                != PICKUP_GHOST) {
                                                                                if (other
                                                                                        ->m_poweredUp
                                                                                    == 0) {
                                                                                    if (HandleUnitContact(
                                                                                            unit,
                                                                                            other
                                                                                        )
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
                            }
                        }
                    }
                }
            }
        }
        hit = 0;
        if (unit != NULL) {
            if (static_cast<i64>(g_frameTime) - unit->m_arrivalReroll64
                >= unit->m_arrivalRerollWindow64) {
                BattlezTask d8 = unit->m_battleState;
                if (d8 != BZTASK_ASSIGNED_TARGET && d8 != BZTASK_SEEK_SWITCH) {
                    if (unit->m_entranceCommitted != 0 && unit->m_deathAnimStarted == 0
                        && unit->m_entranceActive == 0 && unit->m_poweredUp == 0) {
                        char ne;
                        ne =
                            (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_actKey)), "I")
                             != 0);
                        if (ne) {
                            ne =
                                (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_actKey)), "G")
                                 != 0);
                            if (ne) {
                                ne =
                                    (strcmp(
                                         (*g_typeColl.GetNameRecord(unit->m_objAux->m_actKey)),
                                         "L"
                                     )
                                     != 0);
                                if (ne) {
                                    ne =
                                        (strcmp(
                                             (*g_typeColl.GetNameRecord(unit->m_objAux->m_actKey)),
                                             "P"
                                         )
                                         != 0);
                                    if (ne) {
                                        ne =
                                            (strcmp(
                                                 (*g_typeColl.GetNameRecord(
                                                     unit->m_objAux->m_actKey
                                                 )),
                                                 "J"
                                             )
                                             != 0);
                                        if (ne) {
                                            ne =
                                                (strcmp(
                                                     (*g_typeColl.GetNameRecord(
                                                         unit->m_objAux->m_actKey
                                                     )),
                                                     "C"
                                                 )
                                                 != 0);
                                            if (ne) {
                                                ne =
                                                    (strcmp(
                                                         (*g_typeColl.GetNameRecord(
                                                             unit->m_objAux->m_actKey
                                                         )),
                                                         "R"
                                                     )
                                                     != 0);
                                                if (ne) {
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
                            }
                        }
                    }
                }
            }
        }
        if (unit != NULL) {
            if (unit->m_object->m_screenX == unit->m_lastTilePx.m_x
                && unit->m_object->m_screenY == unit->m_lastTilePx.m_y
                && unit->m_entranceCommitted != 0 && unit->m_deathAnimStarted == 0
                && unit->m_entranceActive == 0 && unit->m_poweredUp == 0) {
                eq = (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_actKey)), "I") == 0);
                if (!eq) {
                    eq = (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_actKey)), "G") == 0);
                    if (!eq) {
                        eq =
                            (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_actKey)), "L")
                             == 0);
                        if (!eq) {
                            eq =
                                (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_actKey)), "P")
                                 == 0);
                            if (!eq) {
                                eq =
                                    (strcmp(
                                         (*g_typeColl.GetNameRecord(unit->m_objAux->m_actKey)),
                                         "J"
                                     )
                                     == 0);
                                if (!eq) {
                                    eq =
                                        (strcmp(
                                             (*g_typeColl.GetNameRecord(unit->m_objAux->m_actKey)),
                                             "C"
                                         )
                                         == 0);
                                    if (!eq) {
                                        eq =
                                            (strcmp(
                                                 (*g_typeColl.GetNameRecord(
                                                     unit->m_objAux->m_actKey
                                                 )),
                                                 "R"
                                             )
                                             == 0);
                                        if (!eq) {
                                            if (static_cast<u32>(m_roundRobinTick) % 15
                                                == static_cast<u32>(i)) {
                                                {
                                                    PickupType st3 = unit->m_entranceReason;
                                                    if (st3 > PICKUP_EQUIPPABLE_LAST) {
                                                        st3 = unit->m_toolId;
                                                    }
                                                    if (st3 == PICKUP_WAND
                                                        && unit->m_health > 0x1a) {
                                                        if (rand() % g_diffTier == 0) {
                                                            i32 r = g_buteMgr.GetIntDef(
                                                                "Spellz",
                                                                "SpellRadius",
                                                                8
                                                            );
                                                            RECT spell;
                                                            i32 px = unit->m_object->m_screenX;
                                                            i32 py = unit->m_object->m_screenY;
                                                            spell.left = (px >> TILE_SHIFT_PX) - r;
                                                            spell.top = (py >> TILE_SHIFT_PX) - r;
                                                            spell.right = (px >> TILE_SHIFT_PX) + r;
                                                            spell.bottom =
                                                                (py >> TILE_SHIFT_PX) + r;
                                                            for (i32 j2 = 0; j2 < 4; j2++) {
                                                                if (j2 != m_ownerId) {
                                                                    for (i32 k2 = 0; k2 < 15;
                                                                         k2++) {
                                                                        CGrunt* o =
                                                                            m_triggerMgr->m_grid
                                                                                [j2 * 15 + k2];
                                                                        if (o != NULL) {
                                                                            POINT pt;
                                                                            pt.x = o->m_object
                                                                                       ->m_screenX
                                                                                   >> TILE_SHIFT_PX;
                                                                            pt.y = o->m_object
                                                                                       ->m_screenY
                                                                                   >> TILE_SHIFT_PX;
                                                                            if (PtInRect(&spell, pt)
                                                                                != 0) {
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
                                                if (unit->CoordCount() == 0
                                                    && unit->m_defenderState == AISTATE_COOLDOWN) {
                                                    unit->m_unusedBattleCell.m_x = -1;
                                                    unit->m_defenderState = AISTATE_SEEK;
                                                    unit->m_unusedBattleCell.m_y = -1;
                                                }
                                                {
                                                    char nd;
                                                    nd =
                                                        (strcmp(
                                                             (*g_typeColl.GetNameRecord(
                                                                 unit->m_objAux->m_actKey
                                                             )),
                                                             "D"
                                                         )
                                                         != 0);
                                                    if (nd) {
                                                        ResolveArrival(unit);
                                                    }
                                                }
                                                if (unit->m_object->m_screenX
                                                        == unit->m_lastTilePx.m_x
                                                    && unit->m_object->m_screenY
                                                           == unit->m_lastTilePx.m_y
                                                    && unit->m_entranceCommitted != 0
                                                    && unit->m_deathAnimStarted == 0
                                                    && unit->m_entranceActive == 0
                                                    && unit->m_poweredUp == 0) {
                                                    eq =
                                                        (strcmp(
                                                             (*g_typeColl.GetNameRecord(
                                                                 unit->m_objAux->m_actKey
                                                             )),
                                                             "I"
                                                         )
                                                         == 0);
                                                    if (!eq) {
                                                        eq =
                                                            (strcmp(
                                                                 (*g_typeColl.GetNameRecord(
                                                                     unit->m_objAux->m_actKey
                                                                 )),
                                                                 "G"
                                                             )
                                                             == 0);
                                                        if (!eq) {
                                                            eq =
                                                                (strcmp(
                                                                     (*g_typeColl.GetNameRecord(
                                                                         unit->m_objAux->m_actKey
                                                                     )),
                                                                     "L"
                                                                 )
                                                                 == 0);
                                                            if (!eq) {
                                                                eq =
                                                                    (strcmp(
                                                                         (*g_typeColl.GetNameRecord(
                                                                             unit->m_objAux
                                                                                 ->m_actKey
                                                                         )),
                                                                         "P"
                                                                     )
                                                                     == 0);
                                                                if (!eq) {
                                                                    eq =
                                                                        (strcmp(
                                                                             (*g_typeColl
                                                                                   .GetNameRecord(
                                                                                       unit->m_objAux
                                                                                           ->m_actKey
                                                                                   )),
                                                                             "J"
                                                                         )
                                                                         == 0);
                                                                    if (!eq) {
                                                                        eq =
                                                                            (strcmp(
                                                                                 (*g_typeColl
                                                                                       .GetNameRecord(
                                                                                           unit->m_objAux
                                                                                               ->m_actKey
                                                                                       )),
                                                                                 "C"
                                                                             )
                                                                             == 0);
                                                                        if (!eq) {
                                                                            eq =
                                                                                (strcmp(
                                                                                     (*g_typeColl.GetNameRecord(
                                                                                         unit->m_objAux
                                                                                             ->m_actKey
                                                                                     )),
                                                                                     "R"
                                                                                 )
                                                                                 == 0);
                                                                            if (!eq) {
                                                                                goto dispatch;
                                                                            }
                                                                        }
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        goto nexti;
    dispatch: {
        CMapMgr* bd2 = m_board;
        RECT a;
        a.left = 0;
        a.top = 0;
        a.right = bd2->m_width;
        a.bottom = bd2->m_height;
        RECT b2 = CRect(0, 0, bd2->m_width, bd2->m_height);
        RECT* b2Dst = &bd2->m_bounds;
        if (!IntersectRect(b2Dst, &b2, &a)) {
            *b2Dst = b2;
        }
        bd2->m_gridW = b2Dst->right - b2Dst->left;
        bd2->m_gridH = b2Dst->bottom - b2Dst->top;
        PickupType stX = unit->m_entranceReason;
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
                    Scan(unit);
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
        if (unit->CoordCount() != 0) {
            eq = (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_actKey)), "A") == 0);
            if (eq) {
                Coord* gc = (unit->CoordHead())->m_coord;
                i32 gx = gc->m_x;
                i32 gy = gc->m_y;
                i32 sx = unit->m_object->m_screenX >> TILE_SHIFT_PX;
                i32 sy = unit->m_object->m_screenY >> TILE_SHIFT_PX;
                // Retail 0x2874d-0x287c8.  The out-of-range arm is the ELSE of the
                // proximity test (`jge 0x287ca` twice), not a `cell & 2` target, and
                // the WINGZ gate is ONE ArrivalPickup select every predecessor jumps
                // to - not a split entranceReason/toolId pair with opposite polarity.
                if (abs(gx - sx) >= 2 || abs(gy - sy) >= 2) {
                    goto dropCoords;
                }
                {
                    cell = m_board->m_rows[gy][gx].m_flags;
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
                    if ((cell & 0x40) == 0) {
                        goto flagsArm;
                    }
                wingzGate: {
                    PickupType wp = unit->m_entranceReason;
                    if (wp > PICKUP_EQUIPPABLE_LAST) {
                        wp = unit->m_toolId;
                    }
                    if (wp != PICKUP_WINGZ) {
                        goto nexti;
                    }
                }
                    if ((cell & 2) == 0 && (cell & 0x100) == 0) {
                        goto nexti;
                    }
                    if ((cell & BRICKZ_CELL_OCCUPIED) == 0) {
                        goto tailArm2;
                    }
                    goto nexti;
                dropCoords:
                    if (unit->CoordCount() != 0) {
                        CoordNode* n = unit->CoordHead();
                        if (n != NULL) {
                            do {
                                CoordNode* cur = n;
                                n = n->m_next;
                                if (cur->m_coord != NULL) {
                                    CoordPoolNode* node = g_coordPool.NodeOf(cur->m_coord);
                                    node->m_next = g_coordPool.m_freeHead;
                                    g_coordPool.m_freeHead = node;
                                }
                            } while (n != NULL);
                        }
                        unit->m_coordList.RemoveAll();
                    }
                }
            }
        }
    }
    nexti:;
    }
    return 1;

resetEntrance: {
    i32 pw = unit->m_poweredUp;
    unit->m_neighborValid = 0;
    if (pw == 0) {
        return 1;
    }
    unit->m_entranceActive = 0;
    unit->m_combatActive = 0;
    unit->m_neighborValid = 0;
    unit->m_poweredUp = 0;
    unit->ResetEntranceAnimation(1, 0, 0);
    return 1;
}

arriveHead:
    if (unit->CoordCount() != 0) {
        POSITION pos = unit->m_coordList.GetHeadPosition();
        if (pos != NULL) {
            do {
                Coord* d = static_cast<Coord*>(unit->CoordListOps()->NextData(pos));
                if (d != NULL) {
                    g_coordPool.Push(d);
                }
            } while (pos != NULL);
        }
        unit->m_coordList.RemoveAll();
    }
    return 1;

perimSweep: {
    Coord q0;
    (static_cast<CUserLogic*>(unit))->GetScreenPos((&q0));
    i32 col = (q0.m_x >> TILE_SHIFT_PX) - 2;
    (static_cast<CUserLogic*>(unit))->GetScreenPos((&scratch));
    scratch.m_x >>= 5;
    scratch.m_y >>= 5;
    while (col < scratch.m_x + 3) {
        Coord qa;
        (static_cast<CUserLogic*>(unit))->GetScreenPos((&qa));
        qa.m_y >>= TILE_SHIFT_PX;
        qa.m_x >>= TILE_SHIFT_PX;
        i32 rt = qa.m_y - 2;
        if (static_cast<u32>(col) < m_board->m_width && static_cast<u32>(rt) < m_board->m_height) {
            if (unit->TileSwitch(col, rt, 0, 0x2000098b, 1, 0) != 0) {
                goto rowHitA;
            }
        }
        Coord qc;
        (static_cast<CUserLogic*>(unit))->GetScreenPos((&qc));
        qc.m_y >>= TILE_SHIFT_PX;
        qc.m_x >>= TILE_SHIFT_PX;
        i32 rb = qc.m_y + 2;
        if (static_cast<u32>(col) < m_board->m_width && static_cast<u32>(rb) < m_board->m_height) {
            if (unit->TileSwitch(col, rb, 0, 0x2000098b, 1, 0) != 0) {
                goto rowHitB;
            }
        }
        col++;
        (static_cast<CUserLogic*>(unit))->GetScreenPos((&scratch));
        scratch.m_x >>= 5;
        scratch.m_y >>= 5;
    }
    {
        Coord u0;
        (static_cast<CUserLogic*>(unit))->GetScreenPos((&u0));
        i32 row = (u0.m_y >> TILE_SHIFT_PX) - 2;
        (static_cast<CUserLogic*>(unit))->GetScreenPos((&scratch));
        scratch.m_x >>= 5;
        scratch.m_y >>= 5;
        while (row < scratch.m_y + 3) {
            Coord ua;
            (static_cast<CUserLogic*>(unit))->GetScreenPos((&ua));
            ua.m_x >>= 5;
            ua.m_y >>= 5;
            i32 xl = ua.m_x - 2;
            if (static_cast<u32>(xl) < m_board->m_width
                && static_cast<u32>(row) < m_board->m_height) {
                if (unit->TileSwitch(xl, row, 0, 0x2000098b, 1, 0) != 0) {
                    goto colHitA;
                }
            }
            Coord uc;
            (static_cast<CUserLogic*>(unit))->GetScreenPos((&uc));
            uc.m_y >>= 5;
            uc.m_x >>= 5;
            if (static_cast<u32>(uc.m_x + 2) < m_board->m_width
                && static_cast<u32>(row) < m_board->m_height) {

                if (unit->TileSwitch(xl, row, 0, 0x2000098b, 1, 0) != 0) {
                    goto colHitB;
                }
            }
            row++;
            (static_cast<CUserLogic*>(unit))->GetScreenPos((&scratch));
            scratch.m_x >>= 5;
            scratch.m_y >>= 5;
        }
    }
    {
        CMapMgr* fb = m_board;
        CRect f1(0, 0, fb->m_width, fb->m_height);
        RECT fc = CRect(0, 0, fb->m_width, fb->m_height);
        RECT* fcDst = &fb->m_bounds;
        if (!IntersectRect(fcDst, &fc, &f1)) {
            *fcDst = fc;
        }
        fb->m_gridW = fcDst->right - fcDst->left;
        fb->m_gridH = fcDst->bottom - fcDst->top;
        return 1;
    }
}

rowHitA: {
    unit->m_arrivalRerollLo = 0;
    unit->m_arrivalRerollWindowLo = 0;
    unit->m_arrivalRerollHi = 0;
    unit->m_arrivalRerollWindowHi = 0;
    unit->m_arrivalRerollWindowLo = 0x1f40;
    unit->m_arrivalRerollWindowHi = 0;
    unit->m_arrivalRerollLo = g_frameTime;
    unit->m_arrivalRerollHi = 0;
    CMapMgr* hb = m_board;
    CRect h1(0, 0, hb->m_width, hb->m_height);
    RECT hc = CRect(0, 0, hb->m_width, hb->m_height);
    RECT* hcDst = &hb->m_bounds;
    if (!IntersectRect(hcDst, &hc, &h1)) {
        *hcDst = hc;
    }
    hb->m_gridW = hcDst->right - hcDst->left;
    hb->m_gridH = hcDst->bottom - hcDst->top;
    return 1;
}

rowHitB: {
    unit->m_arrivalRerollLo = 0;
    unit->m_arrivalRerollWindowLo = 0;
    unit->m_arrivalRerollHi = 0;
    unit->m_arrivalRerollWindowHi = 0;
    unit->m_arrivalRerollWindowLo = 0x1f40;
    unit->m_arrivalRerollWindowHi = 0;
    unit->m_arrivalRerollLo = g_frameTime;
    unit->m_arrivalRerollHi = 0;
    CMapMgr* hb = m_board;
    CRect h1(0, 0, hb->m_width, hb->m_height);
    RECT hc = CRect(0, 0, hb->m_width, hb->m_height);
    RECT* hcDst = &hb->m_bounds;
    if (!IntersectRect(hcDst, &hc, &h1)) {
        *hcDst = hc;
    }
    hb->m_gridW = hcDst->right - hcDst->left;
    hb->m_gridH = hcDst->bottom - hcDst->top;
    return 1;
}

spellHit: {
    i32 hx = unit->m_lastTilePx.m_x;
    i32 hy = unit->m_lastTilePx.m_y;
    m_triggerMgr->ApplyTriggerA(unit->m_tileOwnerHi, unit->m_tileOwnerLo, hx, hy);
    return 1;
}

flagsArm: {
    i32 ok = 1;
    // Both gates load m_entranceReason ONCE and keep it live across the pair:
    // retail's first select is `cmp er,0x16 / mov p,er / jle / mov p,tool`
    // (a second register), the second overwrites er in place because it is dead.
    if (cell & 8) {
        PickupType er = unit->m_entranceReason;
        PickupType held = er;
        if (er > PICKUP_EQUIPPABLE_LAST) {
            held = unit->m_toolId;
        }
        if (held != PICKUP_TOOB) {
            PickupType held2 = er;
            if (er > PICKUP_EQUIPPABLE_LAST) {
                held2 = unit->m_toolId;
            }
            if (held2 != PICKUP_WINGZ) {
                ok = 0;
            }
        }
    }
    if (cell & 0x200) {
        PickupType er = unit->m_entranceReason;
        PickupType held = er;
        if (er > PICKUP_EQUIPPABLE_LAST) {
            held = unit->m_toolId;
        }
        if (held != PICKUP_TOOB) {
            PickupType held2 = er;
            if (er > PICKUP_EQUIPPABLE_LAST) {
                held2 = unit->m_toolId;
            }
            if (held2 != PICKUP_WINGZ) {
                ok = 0;
            }
        }
    }
    if (ok == 0) {
        return 1;
    }
    {
        Coord* tc = (unit->CoordTail())->m_coord;
        unit->m_entrancePx.m_x = (tc->m_x << TILE_SHIFT_PX) + TILE_HALF_PX;
        unit->m_entrancePx.m_y = (tc->m_y << TILE_SHIFT_PX) + TILE_HALF_PX;
        unit->StepEntranceReinit();
        return 1;
    }
}

tailArm2: {
    Coord* tc = (unit->CoordTail())->m_coord;
    unit->m_entrancePx.m_x = (tc->m_x << TILE_SHIFT_PX) + TILE_HALF_PX;
    unit->m_entrancePx.m_y = (tc->m_y << TILE_SHIFT_PX) + TILE_HALF_PX;
    unit->StepEntranceReinit();
    return 1;
}

colHitA: {
    unit->m_arrivalRerollLo = 0;
    unit->m_arrivalRerollWindowLo = 0;
    unit->m_arrivalRerollHi = 0;
    unit->m_arrivalRerollWindowHi = 0;
    unit->m_arrivalRerollWindowLo = 0x1f40;
    unit->m_arrivalRerollWindowHi = 0;
    unit->m_arrivalRerollLo = g_frameTime;
    unit->m_arrivalRerollHi = 0;
    CMapMgr* hb = m_board;
    CRect h1(0, 0, hb->m_width, hb->m_height);
    RECT hc = CRect(0, 0, hb->m_width, hb->m_height);
    RECT* hcDst = &hb->m_bounds;
    if (!IntersectRect(hcDst, &hc, &h1)) {
        *hcDst = hc;
    }
    hb->m_gridW = hcDst->right - hcDst->left;
    hb->m_gridH = hcDst->bottom - hcDst->top;
    return 1;
}

colHitB: {
    unit->m_arrivalRerollLo = 0;
    unit->m_arrivalRerollWindowLo = 0;
    unit->m_arrivalRerollHi = 0;
    unit->m_arrivalRerollWindowHi = 0;
    unit->m_arrivalRerollWindowLo = 0x1f40;
    unit->m_arrivalRerollWindowHi = 0;
    unit->m_arrivalRerollLo = g_frameTime;
    unit->m_arrivalRerollHi = 0;
    CMapMgr* hb = m_board;
    CRect h1(0, 0, hb->m_width, hb->m_height);
    RECT hc = CRect(0, 0, hb->m_width, hb->m_height);
    RECT* hcDst = &hb->m_bounds;
    if (!IntersectRect(hcDst, &hc, &h1)) {
        *hcDst = hc;
    }
    hb->m_gridW = hcDst->right - hcDst->left;
    hb->m_gridH = hcDst->bottom - hcDst->top;
    return 1;
}
}

RVA(0x00029a30, 0x10)
void*& CGruntCoordList::NextData(POSITION& pos) {

    return CPtrList::GetNext(pos);
}

RVA(0x00029a50, 0x15)
void CUserLogic::GetScreenPos(Coord* out) {
    CWwdGameObjectA* o = m_object;
    i32 y = o->m_screenY;
    i32 x = o->m_screenX;
    out->m_x = x;
    out->m_y = y;
}

RVA(0x00029a80, 0x29)
i32 CGrunt::IsAtSavedScreenPos() {
    CWwdGameObjectA* o = m_object;

    i32 sx = m_lastTilePx.m_x;
    if (o->m_screenX == sx && o->m_screenY == m_lastTilePx.m_y) {
        return 1;
    }
    return 0;
}

RVA(0x00029af0, 0x3b)

void __stdcall TileSwitch(CGrunt* g, i32 col, i32 row, i32 burnRandA, i32 burnRandB, i32 unused) {
    if (burnRandA) {
        rand();
    }
    if (burnRandB) {
        rand();
    }
    g->TileSwitch(col, row, 0, 0x9c7, 0, 0);
}

// @early-stop
RVA(0x00029b40, 0x813)
i32 CBattlezMapConfig::ValidateUnitPath(CGrunt* unit) {
    CPtrList* coordList = &unit->m_coordList;
    if (unit->CoordCount() == 0) {
        goto returnZero;
    }

    {
        Coord* c0 = unit->CoordHead()->m_coord;
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

        i32 tile0;
        if (static_cast<u32>(ux) < static_cast<u32>(m_board->m_width)
            && static_cast<u32>(uy) < static_cast<u32>(m_board->m_height)) {
            tile0 = (static_cast<BrickzCell*>(m_board->m_rows[uy]))[ux].m_flags;
        } else {
            tile0 = 1;
        }
        if (static_cast<u8>(tile0) == 1) {
            if (unit->CoordCount() == 0) {
                goto returnZero;
            }
            CoordNode* n = unit->CoordHead();
            while (n != NULL) {
                CoordNode* cur = n;
                n = n->m_next;
                if (cur->m_coord != NULL) {
                    g_coordPool.Push(cur->m_coord);
                }
            }
            coordList->RemoveAll();
            return 0;
        }

        BrickzCell scratchA;
        const BrickzCell* srcA;
        CoordNode* head = MfcNodeFromPosition<CoordNode>(coordList->GetHeadPosition());
        Coord* firstCoord = head->m_coord;
        if (static_cast<u32>(firstCoord->m_x) < static_cast<u32>(m_board->m_width)
            && static_cast<u32>(firstCoord->m_y) < static_cast<u32>(m_board->m_height)) {
            srcA = &(static_cast<BrickzCell*>(m_board->m_rows[firstCoord->m_y]))[firstCoord->m_x];
        } else {
            memset(&scratchA, 1, sizeof(scratchA));
            srcA = &scratchA;
        }
        if (coordList->GetCount() == 0) {
            goto returnZero;
        }
        Coord* pathHead = unit->CoordHead()->m_coord;
        i32 cx = pathHead->m_x;
        i32 cy = pathHead->m_y;
        (static_cast<CUserLogic*>(unit))->GetScreenPos((&pt));
        if (static_cast<u32>(cx) < static_cast<u32>(m_board->m_width)
            && static_cast<u32>(cy) < static_cast<u32>(m_board->m_height)) {
            srcA = &(static_cast<BrickzCell*>(m_board->m_rows[cy]))[cx];
        } else {
            memset(&scratchA, 1, sizeof(scratchA));
            srcA = &scratchA;
        }
        scratchA = *srcA;
        PickupType prim = unit->m_entranceReason;
        if (prim > PICKUP_EQUIPPABLE_LAST) {
            prim = unit->m_toolId;
        }

        Coord pt2;
        (static_cast<CUserLogic*>(unit))->GetScreenPos((&pt2));
        pt2.m_x >>= TILE_SHIFT_PX;
        pt2.m_y >>= TILE_SHIFT_PX;
        i32 sgy = pt2.m_y;
        (static_cast<CUserLogic*>(unit))->GetScreenPos((&pt));
        pt.m_x >>= TILE_SHIFT_PX;
        pt.m_y >>= TILE_SHIFT_PX;
        i32 sgx = pt.m_x;
        BrickzCell scratchB;
        const BrickzCell* srcB;
        if (static_cast<u32>(sgx) < static_cast<u32>(m_board->m_width)
            && static_cast<u32>(sgy) < static_cast<u32>(m_board->m_height)) {
            srcB = &(static_cast<BrickzCell*>(m_board->m_rows[sgy]))[sgx];
        } else {
            memset(&scratchB, 1, sizeof(scratchB));
            srcB = &scratchB;
        }
        scratchB = *srcB;

        if ((scratchB.m_flags & 0x4) && unit->m_battleState != BZTASK_SEEK_SWITCH) {
            (static_cast<CUserLogic*>(unit))->GetScreenPos((&pt));
            pt.m_x >>= TILE_SHIFT_PX;
            pt.m_y >>= TILE_SHIFT_PX;
            i32 rx = pt.m_x;
            (static_cast<CUserLogic*>(unit))->GetScreenPos((&pt2));
            pt2.m_x >>= TILE_SHIFT_PX;
            pt2.m_y >>= TILE_SHIFT_PX;
            i32 ry = pt2.m_y;
            CTileTriggerSwitchLogic* rec = m_cellQuery->FindChild((rx << 8) + ry, TRIGID_ANY);
            if (rec->m_typeId == TRIGID_SWITCH_2) {
                unit->m_defenderState = AISTATE_SEEK;
                if (unit->CoordCount() != 0) {
                    CoordNode* n = unit->CoordHead();
                    while (n != NULL) {
                        CoordNode* cur = n;
                        n = n->m_next;
                        if (cur->m_coord != NULL) {
                            g_coordPool.Push(cur->m_coord);
                        }
                    }
                    coordList->RemoveAll();
                }
                unit->m_battleState = BZTASK_SEEK_SWITCH;
                unit->m_dwell = 0;
                return 0;
            }
        }

        PickupType entranceMode = unit->m_entranceReason;
        if (entranceMode > PICKUP_EQUIPPABLE_LAST) {
            entranceMode = unit->m_toolId;
        }
        if (entranceMode == PICKUP_TIMEBOMB && unit->CoordCount() >= 2) {
            CoordNode* node = unit->CoordHead();
            Coord* ca = node->m_coord;
            CoordNode* nn = node->m_next;
            i32 ax = ca->m_x;
            Coord* cb = nn->m_coord;
            i32 ay = ca->m_y;
            i32 bx = cb->m_x;
            i32 by = cb->m_y;
            i32 tB;
            if (static_cast<u32>(bx) < static_cast<u32>(m_board->m_width)
                && static_cast<u32>(by) < static_cast<u32>(m_board->m_height)) {
                tB = (static_cast<BrickzCell*>(m_board->m_rows[by]))[bx].m_flags;
            } else {
                tB = 1;
            }
            if (tB & 0x20) {
                i32 tA2;
                if (static_cast<u32>(ax) < static_cast<u32>(m_board->m_width)
                    && static_cast<u32>(ay) < static_cast<u32>(m_board->m_height)) {
                    tA2 = (static_cast<BrickzCell*>(m_board->m_rows[ay]))[ax].m_flags;
                } else {
                    tA2 = 1;
                }
                if (!(tA2 & 0x2)) {
                    m_triggerMgr->ApplyTriggerA(
                        unit->m_tileOwnerHi,
                        unit->m_tileOwnerLo,
                        ax * 0x20 + 0x10,
                        ay * 0x20 + 0x10
                    );
                    return 0;
                }
            }
        }

        if ((scratchB.m_flags & 0x8000) && unit->m_defenderState == AISTATE_RETURN) {
            unit->m_defenderState = AISTATE_SEEK;
        }
        i32 sA = scratchA.m_flags;
        if ((sA & 0x8000) && prim == PICKUP_BRICK && unit->m_battleState == BZTASK_CARRY_BRICK) {
            m_triggerMgr->ApplyTriggerA(
                unit->m_tileOwnerHi,
                unit->m_tileOwnerLo,
                cx * 0x20 + 0x10,
                cy * 0x20 + 0x10
            );
            unit->m_defenderState = AISTATE_SEEK;
            if (unit->CoordCount() != 0) {
                CoordNode* n = unit->CoordHead();
                while (n != NULL) {
                    CoordNode* cur = n;
                    n = n->m_next;
                    if (cur->m_coord != NULL) {
                        g_coordPool.Push(cur->m_coord);
                    }
                }
                coordList->RemoveAll();
            }
            return 0;
        }
        // Retail re-tests `sA & 0x8000` here (the CSE'd `and` is re-`test`ed at
        // 0x29fbb), so the two guards are separate statements, not one nesting.
        if ((sA & 0x8000) && PathCrossesMarkedTile(unit) == 0
            && unit->m_defenderState == AISTATE_BATTLEZ_FINAL_ROUTE) {
            CoordNode* head = unit->CoordHead();
            if (head != NULL) {
                CoordNode* n = head->m_next;
                if (n != NULL) {
                    while (n != NULL) {
                        CoordNode* cur = n;
                        n = n->m_next;
                        if (cur->m_coord != NULL) {
                            CoordPoolNode* fn = g_coordPool.NodeOf(cur->m_coord);
                            fn->m_next = g_coordPool.m_freeHead;
                            g_coordPool.m_freeHead = fn;
                            coordList->RemoveAt(MfcPositionFromNode(cur));
                        }
                    }
                    return 1;
                }
            }
        }

        if (sA & 0x200) {
            PickupType p = unit->m_entranceReason;
            if (p > PICKUP_EQUIPPABLE_LAST) {
                p = unit->m_toolId;
            }
            if (p != PICKUP_WINGZ) {
                goto returnZero;
            }
        }
        if (sA & 0x8) {
            i32 hi = sA & 0x100;
            if (hi) {
                PickupType er = unit->m_entranceReason;
                PickupType p = er;
                if (er > PICKUP_EQUIPPABLE_LAST) {
                    p = unit->m_toolId;
                }
                if (p == PICKUP_WINGZ) {
                    return 1;
                }
                PickupType entranceMode2 = er;
                if (er > PICKUP_EQUIPPABLE_LAST) {
                    entranceMode2 = unit->m_toolId;
                }
                if (entranceMode2 == PICKUP_TOOB) {
                    return 1;
                }
            }
            i32 lo2 = sA & 0x2;
            if (lo2) {
                PickupType p = unit->m_entranceReason;
                if (p > PICKUP_EQUIPPABLE_LAST) {
                    p = unit->m_toolId;
                }
                if (p == PICKUP_WINGZ) {
                    return 1;
                }
            }
            if (PathToNearestGoal(unit, cx, cy) != 0) {
                return 1;
            }
            i32 sB = scratchB.m_flags;
            if ((sB & 0x200) || (sB & 0x8)) {
                goto returnZero;
            }
            if (hi && unit->m_defenderState != AISTATE_RETURN) {
                if (rand() % 5 == 0) {
                    EnterDefenderMode(unit, 0x16);
                } else {
                    EnterDefenderMode(unit, 0x12);
                }
            }
            if (lo2) {
                if (unit->m_defenderState == AISTATE_RETURN) {
                    goto returnZero;
                }
                EnterDefenderMode(unit, 0x16);
                return 0;
            }
            goto returnZero;
        }

        if ((sA & 0x20) && prim != PICKUP_GAUNTLETZ && prim != PICKUP_TIMEBOMB
            && prim != PICKUP_BOMB) {
            if (unit->m_defenderState == AISTATE_RETURN) {
                goto returnZero;
            }
            EnterDefenderMode(unit, 5);
            return 0;
        }
        if (sA & 0x40) {
            PickupType p = unit->m_entranceReason;
            if (p > PICKUP_EQUIPPABLE_LAST) {
                p = unit->m_toolId;
            }
            if (p != PICKUP_WINGZ) {
                if (prim == PICKUP_SHOVEL) {
                    goto returnZero;
                }
                if (unit->m_defenderState == AISTATE_RETURN) {
                    goto returnZero;
                }
                EnterDefenderMode(unit, 0xd);
                return 0;
            }
        }
        if (sA & 0x2) {
            PickupType p = unit->m_entranceReason;
            if (p > PICKUP_EQUIPPABLE_LAST) {
                p = unit->m_toolId;
            }
            if (p != PICKUP_WINGZ) {
                goto returnZero;
            }
        }
        if (sA & BRICKZ_CELL_OCCUPIED) {
            return RepathAroundBlockedTiles(unit);
        }
        PickupType pk = unit->m_entranceReason;
        if (pk > PICKUP_EQUIPPABLE_LAST) {
            pk = unit->m_toolId;
        }
        if (pk != PICKUP_GOOBER) {
            return 1;
        }

        POSITION opos = m_triggerMgr->m_baseList.GetHeadPosition();
        while (opos != NULL) {
            CGruntPuddle* cand = static_cast<CGruntPuddle*>(m_triggerMgr->m_baseList.GetNext(opos));
            if (cand->m_pending == 0) {
                i32 ox = cand->m_tileX;
                i32 oy = cand->m_tileY;
                if ((static_cast<CGrunt*>(unit))->RectContains(ox * 0x20 + 0x10, oy * 0x20 + 0x10)
                    != 0) {
                    m_triggerMgr->ApplyTriggerA(
                        unit->m_tileOwnerHi,
                        unit->m_tileOwnerLo,
                        ox * 0x20 + 0x10,
                        oy * 0x20 + 0x10
                    );
                    if (unit->CoordCount() != 0) {
                        CoordNode* n = unit->CoordHead();
                        while (n != NULL) {
                            CoordNode* cur = n;
                            n = n->m_next;
                            if (cur->m_coord != NULL) {
                                CoordPoolNode* fn = g_coordPool.NodeOf(cur->m_coord);
                                fn->m_next = g_coordPool.m_freeHead;
                                g_coordPool.m_freeHead = fn;
                            }
                        }
                        coordList->RemoveAll();
                    }
                    m_spawnTimer += static_cast<i32>((static_cast<u32>(m_gruntCreationTime) >> 2));
                    return 1;
                }
            }
        }
        return 1;
    }
recycleBail:
    if (unit->CoordCount() != 0) {
        CoordNode* n = unit->CoordHead();
        while (n != NULL) {
            CoordNode* cur = n;
            n = n->m_next;
            if (cur->m_coord != NULL) {
                g_coordPool.Push(cur->m_coord);
            }
        }
        coordList->RemoveAll();
    }
returnZero:
    return 0;
}

RVA(0x0002a570, 0x4c6)
i32 CBattlezMapConfig::RepathAroundBlockedTiles(CGrunt* unit) {
    CGruntCoordList* coordList = &unit->m_coordList;
    if (coordList->GetCount() == 0) {
        return 1;
    }
    CoordNode* node = MfcNodeFromPosition<CoordNode>(coordList->GetHeadPosition());
    Coord center;
    (static_cast<CUserLogic*>(unit))->GetScreenPos((&center));
    CMapMgr* board = m_board;
    center.m_y >>= TILE_SHIFT_PX;
    center.m_x >>= TILE_SHIFT_PX;
    {
        CRect bounds(0, 0, board->m_width, board->m_height);
        RECT box;
        box.left = center.m_x - 6;
        box.top = center.m_y - 6;
        box.right = center.m_x + 6;
        box.bottom = center.m_y + 6;

        // CMapMgr::Clip(&box) expanded in place.
        const RECT* src = &box;
        RECT a;
        if (src != NULL) {
            a = *src;
            a.right++;
            a.bottom++;
        } else {
            a = CRect(0, 0, board->m_width, board->m_height);
        }
        RECT* aDst = &board->m_bounds;
        if (!IntersectRect(aDst, &a, &bounds)) {
            *aDst = a;
        }
        board->m_gridW = aDst->right - aDst->left;
        board->m_gridH = aDst->bottom - aDst->top;
    }
    Coord* tailCoord = (unit->CoordTail())->m_coord;
    i32 tx = tailCoord->m_x;
    i32 ty = tailCoord->m_y;
    u32 iter = 0;
    while (node != NULL && iter < 3) {
        CoordNode* cur = node;
        node = node->m_next;
        Coord* coord = cur->m_coord;
        if (coord == NULL) {
            continue;
        }
        i32 x = coord->m_x;
        i32 y = coord->m_y;
        i32 tile = board->m_rowInts[y][x * 7];
        if ((tile & 1) != 0 && (x != tx || y != ty)) {
            continue;
        }
        CPtrList list(10);
        i32 flags = 0;
        // One load of m_entranceReason, kept live across all three gates: retail
        // guards on IT (`cmp er,0x16`) and lands the select in a second register.
        PickupType er = unit->m_entranceReason;
        PickupType prim = er;
        if (er > PICKUP_EQUIPPABLE_LAST) {
            prim = unit->m_toolId;
        }
        if (prim == PICKUP_TOOB) {
            flags = 0x100;
        }
        prim = er;
        if (er > PICKUP_EQUIPPABLE_LAST) {
            prim = unit->m_toolId;
        }
        if (prim == PICKUP_WINGZ) {
            flags = 0x942;
        }
        prim = er;
        if (er > PICKUP_EQUIPPABLE_LAST) {
            prim = unit->m_toolId;
        }
        if (prim == PICKUP_SPRING) {
            flags = 0x1000;
        }
        if (board->SearchEdge(
                center.m_x,
                center.m_y,
                coord->m_x,
                coord->m_y,
                &list,
                1,
                0x2000098f,
                flags
            ) != 0
            && list.GetCount() != 0) {
            Coord* head = static_cast<Coord*>(list.RemoveHead());
            if (head != NULL) {
                CoordPoolNode* n = g_coordPool.NodeOf(head);
                n->m_next = g_coordPool.m_freeHead;
                g_coordPool.m_freeHead = n;
            }
            if (list.GetCount() != 0) {
                while (node != NULL) {
                    CoordNode* remaining = node;
                    node = node->m_next;
                    // Spelled through the global at every use: the stores via `copy`
                    // may alias it, so retail RELOADS g_coordPool.m_freeHead for the
                    // unlink (3 refs at this site, not 2).
                    Coord* copy = NULL;
                    if (g_coordPool.m_freeHead->m_next != NULL) {
                        copy = &g_coordPool.m_freeHead->m_coord;
                        copy->m_x = remaining->m_coord->m_x;
                        copy->m_y = remaining->m_coord->m_y;
                        g_coordPool.m_freeHead = g_coordPool.m_freeHead->m_next;
                    }
                    list.AddTail(copy);
                }

                if (coordList->GetCount() != 0) {
                    CoordNode* p = unit->CoordHead();
                    while (p != NULL) {
                        CoordNode* c2 = p;
                        p = p->m_next;
                        if (c2->m_coord != NULL) {
                            g_coordPool.Push(c2->m_coord);
                        }
                    }
                    coordList->RemoveAll();
                }

                POSITION qp = list.GetHeadPosition();
                while (qp != NULL) {
                    Coord* c3 = static_cast<Coord*>(list.GetNext(qp));
                    if (c3 != NULL && (c3->m_x != center.m_x || c3->m_y != center.m_y)) {
                        coordList->AddTail(c3);
                    }
                }

                // Only ONE CRect ctor call here in retail (the other two Clip
                // expansions have two): the full-board rect is written field by
                // field, sharing the m_width/m_height loads with the ctor args.
                RECT hitFull;
                hitFull.left = 0;
                hitFull.top = 0;
                hitFull.right = board->m_width;
                hitFull.bottom = board->m_height;
                RECT hitBox = CRect(0, 0, board->m_width, board->m_height);
                RECT* hitBoxDst = &board->m_bounds;
                if (!IntersectRect(hitBoxDst, &hitBox, &hitFull)) {
                    *hitBoxDst = hitBox;
                }
                board->m_gridW = hitBoxDst->right - hitBoxDst->left;
                board->m_gridH = hitBoxDst->bottom - hitBoxDst->top;
                Coord* nt = (unit->CoordTail())->m_coord;
                unit->m_entrancePx.m_x = (nt->m_x << TILE_SHIFT_PX) + TILE_HALF_PX;
                unit->m_entrancePx.m_y = (nt->m_y << TILE_SHIFT_PX) + TILE_HALF_PX;
                return 1;
            }
        }
        iter++;
    }

    {
        CRect tailFull(0, 0, board->m_width, board->m_height);
        RECT tailBox = CRect(0, 0, board->m_width, board->m_height);
        RECT* tailBoxDst = &board->m_bounds;
        if (!IntersectRect(tailBoxDst, &tailBox, &tailFull)) {
            *tailBoxDst = tailBox;
        }
        board->m_gridW = tailBoxDst->right - tailBoxDst->left;
        board->m_gridH = tailBoxDst->bottom - tailBoxDst->top;
    }
    return 0;
}

// @early-stop
// Retail spills the strength-reduced grid byte offset and gives ebp to cx; cl does
// the reverse, so retail's frame carries one extra local (0x20 vs 0x1c).
RVA(0x0002ab80, 0x15e)
CGrunt* CBattlezMapConfig::FindIdleGruntInBox(i32 cx, i32 cy, i32 halfW, i32 halfH) {
    RECT rect;
    rect.left = cx - halfW;
    rect.top = cy - halfH;
    rect.right = cx + halfW;
    rect.bottom = cy + halfH;
    CGrunt* best = 0;
    i32 bestDist = INT_MAX;
    for (i32 band = 0; band < 4; band++) {
        if (band == m_ownerId) {
            continue;
        }
        for (i32 i = 0; i < 15; i++) {
            CGrunt* u = m_triggerMgr->m_grid[band * 15 + i];
            if (u == NULL) {
                continue;
            }
            if (u->m_entranceDropActive != 0) {
                continue;
            }
            CGameObject* lvl = u->m_object;
            POINT wpt;
            wpt.y = lvl->m_screenY >> TILE_SHIFT_PX;
            wpt.x = lvl->m_screenX >> TILE_SHIFT_PX;
            if (!PtInRect(&rect, wpt)) {
                continue;
            }
            i32 keep = 1;
            if (u->m_gruntKind == GRUNT_GHOST) {
                if (rand() % 100 > 5) {
                    keep = 0;
                }
            }
            if (keep == 0) {
                continue;
            }
            lvl = u->m_object;
            i32 dx = abs((lvl->m_screenX >> TILE_SHIFT_PX) - cx);
            i32 dy = abs((lvl->m_screenY >> TILE_SHIFT_PX) - cy);
            i32 dist = dx + dy;
            if (dist >= bestDist) {
                continue;
            }
            if (u->m_poweredUp != 0) {
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
    if (band == m_ownerId) {
        band++;
    }
    band = band % 4;
    i32 cell = rand() % 15;
    for (i32 i = 0; i < 15; i++) {
        CGrunt* u = m_triggerMgr->m_grid[band * 15 + i];
        if (u != NULL && u->m_entranceDropActive == 0) {
            return u;
        }
        cell = (cell + 1) % 15;
    }
    return 0;
}

RVA(0x0002ade0, 0x7)
void CBattlezMapConfig::Clear() {
    m_active = 0;
}

// @early-stop
// Frame 0x24 vs retail's 0x34 and 1048 bytes vs 1070 are ONE cause: retail runs out
// of registers here and we do not. Retail spills only box.top/box.bottom (to
// [esp+0x28]/[esp+0x30]) and keeps box.left/box.right in ebx/ebp, so `box` needs its
// own 16-byte slot; we forward all four fields and cl coalesces box's slot onto b's.
// Both sides still take box's address for the inlined Clip's `src != NULL` test
// (retail `lea edx,[esp+0x24]`, ours `lea ebx,[esp+0x24]`), so the address-take is
// not the difference. The ~22 missing bytes are that spill/reload traffic.
// The trailing `mov ecx,[esi+0xc]` does NOT prove a `m_board->Clip(0)` re-read: esi
// is reloaded from the `this` spill for the adjacent RouteUnitTo receiver, and with
// all four callee-saved registers live cl rematerialises m_board from it rather than
// spill a cached local. Spelling it `m_board->Clip(0)` measures 85.96 -> 81.31.
// Not fixable by making CMapMgr::Clip a header inline: cl 5.0 then expands BOTH
// sites (85.93 -> 77.41) and no obj emits the 0x2b340 COMDAT - see
// docs/patterns/inline-budget-emits-ool-comdat.md.
RVA(0x0002ae00, 0x42e)
i32 CBattlezMapConfig::HandleUnitContact(CGrunt* unit, CGrunt* tgt) {
    if (unit->m_entranceCommitted == 0) {
        return 0;
    }
    bool eq;
    eq = (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_actKey)), "J") == 0);
    if (eq) {
        return 0;
    }
    eq = (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_actKey)), "C") == 0);
    if (eq) {
        return 0;
    }
    eq = (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_actKey)), "R") == 0);
    if (eq) {
        return 0;
    }
    eq = (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_actKey)), "G") == 0);
    if (eq) {
        return 0;
    }
    eq = (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_actKey)), "L") == 0);
    if (eq) {
        return 0;
    }
    if (unit->m_gruntKind == GRUNT_GHOST) {
        return 0;
    }
    if (unit->m_entranceDropActive != 0) {
        return 0;
    }
    i32 roll = rand() % 4;
    if (tgt->m_vehiclePickupType != PICKUP_NONE && roll == 0) {
        CGameObject* ul = unit->m_object;
        if ((static_cast<CGrunt*>(tgt))->RectContainsGated(ul->m_screenX, ul->m_screenY) != 0) {
            if (tgt->m_vehiclePickupType == PICKUP_SCROLL) {
                CGameObject* tl = tgt->m_object;
                m_triggerMgr->ApplyTriggerB(
                    tgt->m_tileOwnerHi,
                    tgt->m_tileOwnerLo,
                    tl->m_screenX,
                    tl->m_screenY
                );
            } else {
                CGameObject* ul2 = unit->m_object;
                m_triggerMgr->ApplyTriggerB(
                    tgt->m_tileOwnerHi,
                    tgt->m_tileOwnerLo,
                    ul2->m_screenX,
                    ul2->m_screenY
                );
            }
            return 1;
        }
    }
    CGameObject* ul3 = unit->m_object;
    (static_cast<CGrunt*>(tgt))
        ->CommitNeighbor(unit->m_tileOwnerHi, unit->m_tileOwnerLo, ul3->m_screenX, ul3->m_screenY);
    PickupType prim = tgt->m_entranceReason;
    if (prim > PICKUP_EQUIPPABLE_LAST) {
        prim = tgt->m_toolId;
    }
    if (prim != PICKUP_TIMEBOMB) {
        return 1;
    }

    CGameObject* tl = tgt->m_object;
    i32 ycoord = (tl->m_screenY >> TILE_SHIFT_PX) + rand() % 10 - 5;
    i32 r2 = rand() % 10;
    CGameObject* tl2 = tgt->m_object;
    i32 left = (tl2->m_screenX >> TILE_SHIFT_PX) - 5;
    i32 xcoord = (tl->m_screenX >> TILE_SHIFT_PX) + r2 - 5;
    i32 right = (tl2->m_screenX >> TILE_SHIFT_PX) + 5;
    CMapMgr* board = m_board;
    i32 bottom = (tl2->m_screenY >> TILE_SHIFT_PX) + 5;
    i32 top = (tl2->m_screenY >> TILE_SHIFT_PX) - 5;
    RECT a;
    RECT box;
    RECT bounds;
    box.left = left;
    box.top = top;
    box.right = right;
    box.bottom = bottom;

    // CMapMgr::Clip(&box) expanded in place: retail calls the out-of-line Clip only
    // for the trailing NULL argument, so this arm is a copy of its body.
    const RECT* src = &box;
    bounds.left = 0;
    bounds.top = 0;
    bounds.right = board->m_width;
    bounds.bottom = board->m_height;
    if (src != NULL) {
        a = *src;
        a.right++;
        a.bottom++;
    } else {
        a = bounds;
    }
    RECT* aDst = &board->m_bounds;
    if (!IntersectRect(aDst, &a, &bounds)) {
        *aDst = a;
    }
    board->m_gridW = aDst->right - aDst->left;
    board->m_gridH = aDst->bottom - aDst->top;
    RouteUnitTo(tgt, xcoord, ycoord, 0x20000d87, 0, 0);
    board->Clip(static_cast<const RECT*>(0));
    return 1;
}

// @early-stop
// docs/patterns/duplicated-zero-constant-claims-the-fourth-callee-saved-register.md
// - exhausted there: 11 spellings, a 400-iteration hill-climb and a flat
// declaration-count window. Do not chase the prologue tells on their own.
RVA(0x0002b340, 0xaa)
void CMapMgr::Clip(const RECT* src) {
    RECT a, b;
    b.left = b.top = 0;
    b.right = m_width;
    b.bottom = m_height;
    if (src) {
        a = *src;
        a.right++;
        a.bottom++;
    } else {
        a = b;
    }
    RECT* dst = &m_bounds;
    if (!IntersectRect(dst, &a, &b)) {
        *dst = a;
    }
    m_gridW = dst->right - dst->left;
    m_gridH = dst->bottom - dst->top;
}
RVA(0x0002b420, 0x419)
i32 CBattlezMapConfig::Serialize(CFileMemBase* ar) {
    if (ar == NULL) {
        return 0;
    }
    ar->Write(&m_active, sizeof(m_active));
    ar->Write(&m_ownerId, sizeof(m_ownerId));
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

    n = m_attackWaypoints.GetSize();
    ar->Write(&n, sizeof(n));
    for (i = 0; i < n; i++) {
        ar->Write(m_attackWaypoints[i], 8);
    }

    n = m_candArray.GetSize();
    ar->Write(&n, sizeof(n));
    for (i = 0; i < n; i++) {
        ar->Write(m_candArray[i], 8);
    }
    return 1;
}

// @early-stop
RVA(0x0002b950, 0x513)
i32 CBattlezMapConfig::Deserialize(CFileMemBase* ar) {
    if (ar == NULL) {
        return 0;
    }
    ar->Read(&m_active, sizeof(m_active));
    ar->Read(&m_ownerId, sizeof(m_ownerId));
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
    m_reserved104.SetSize(0, -1);
    m_reserved104.SetSize(count, -1);
    for (i = 0; i < static_cast<u32>(count); i++) {
        ar->Read(&tmp, sizeof(tmp));
        m_reserved104[i] = tmp;
    }

    ar->Read(&count, sizeof(count));
    m_reserved118.SetSize(0, -1);
    m_reserved118.SetSize(count, -1);
    for (i = 0; i < static_cast<u32>(count); i++) {
        ar->Read(&tmp, sizeof(tmp));
        m_reserved118[i] = tmp;
    }

    for (i32 k = 0; k < 4; k++) {
        ar->Read(&m_reserved12c[k], sizeof(m_reserved12c[k]));
    }

    for (j = 0; j < m_attackWaypoints.GetSize(); j++) {
        Coord* q = static_cast<Coord*>(m_attackWaypoints[j]);
        if (q != NULL) {
            CoordPoolNode* node = g_coordPool.NodeOf(q);
            node->m_next = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = node;
        }
    }
    m_attackWaypoints.SetSize(0, -1);
    ar->Read(&count, sizeof(count));
    m_attackWaypoints.SetSize(count, -1);
    for (i = 0; i < static_cast<u32>(count); i++) {
        CoordPoolNode* node = g_coordPool.m_freeHead;
        Coord* payload = NULL;
        if (node->m_next != NULL) {
            payload = &node->m_coord;
            g_coordPool.m_freeHead = node->m_next;
        }
        ar->Read(payload, 8);
        m_attackWaypoints[i] = payload;
    }

    for (j = 0; j < m_candArray.GetSize(); j++) {
        Coord* q = static_cast<Coord*>(m_candArray[j]);
        if (q != NULL) {
            CoordPoolNode* node = g_coordPool.NodeOf(q);
            node->m_next = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = node;
        }
    }
    m_candArray.SetSize(0, -1);
    ar->Read(&count, sizeof(count));
    m_candArray.SetSize(count, -1);
    for (i = 0; i < static_cast<u32>(count); i++) {
        CoordPoolNode* node = g_coordPool.m_freeHead;
        Coord* payload = NULL;
        if (node->m_next != NULL) {
            payload = &node->m_coord;
            g_coordPool.m_freeHead = node->m_next;
        }
        ar->Read(payload, 8);
        m_candArray[i] = payload;
    }
    return 1;
}

RVA(0x0002bfc0, 0x8a)
i32 CBattlezMapConfig::SerializeState(CFileMemBase* objArg, SerialMode kindArg, LogicTypeId, i32) {
    CFileMemBase* obj = objArg;
    SerialMode kind = kindArg;
    switch (kind) {
        case SERIAL_SAVE:
            if (this->Serialize(obj) == 0) {
                return 0;
            }
            break;
        case SERIAL_LOAD:
            if (this->Deserialize(obj) == 0) {
                return 0;
            }
            break;
    }

    // retail materialises this+0x78 once, ahead of the dispatch, and reaches
    // the second timer as +8 off that cursor rather than off `this`.
    Clock64* p = m_routeTimers;
    switch (kind) {
        case SERIAL_SAVE:
            obj->Write(&p[0], sizeof(Clock64));
            obj->Write(&p[1], sizeof(Clock64));
            break;
        case SERIAL_LOAD:
            obj->Read(&p[0], sizeof(Clock64));
            obj->Read(&p[1], sizeof(Clock64));
            break;
    }
    return 1;
}

RVA(0x0002c080, 0x8)
i32 CBattlezMapConfig::AcceptAlways(CGrunt*) {
    return 1;
}

RVA(0x0002c0a0, 0x78)
i32 CBattlezMapConfig::EnterDefenderMode(CGrunt* unit, i32 value) {
    if (unit->m_defenderState == AISTATE_RETURN) {
        return 1;
    }
    m_claimTimer = 0;
    unit->m_defenderState = AISTATE_RETURN;
    unit->m_defenderPickupType = static_cast<PickupType>(value);
    CGrunt** units = m_triggerMgr->m_grid + m_ownerId * 15;
    i32 count = 0;
    for (i32 k = 0; k < 15; k++) {
        CGrunt* p = units[k];
        if (p != NULL && unit != p && p->m_defenderState == AISTATE_RETURN) {
            count++;
        }
    }
    unit->m_defenderQueuePosition = count;
    return 1;
}

// @early-stop
RVA(0x0002c140, 0x420)
i32 CBattlezMapConfig::RouteToNearbyPickup(CGrunt* unit) {
    if (unit->m_gruntKind != GRUNT_NORMAL) {
        return 0;
    }
    PickupType prim = unit->m_entranceReason;
    if (prim > PICKUP_EQUIPPABLE_LAST) {
        prim = unit->m_toolId;
    }
    if (prim != PICKUP_NONE) {
        return 0;
    }

    i32 bottom;
    i32 right;
    i32 top;
    i32 left;
    {
        Coord c1;
        unit->GetScreenPos(&c1);
        c1.m_x >>= TILE_SHIFT_PX;
        c1.m_y >>= TILE_SHIFT_PX;
        bottom = c1.m_y;
        Coord c2;
        unit->GetScreenPos(&c2);
        c2.m_x >>= TILE_SHIFT_PX;
        c2.m_y >>= TILE_SHIFT_PX;
        right = c2.m_x;
        Coord c3;
        unit->GetScreenPos(&c3);
        c3.m_x >>= TILE_SHIFT_PX;
        c3.m_y >>= TILE_SHIFT_PX;
        top = c3.m_y;
        Coord c4;
        unit->GetScreenPos(&c4);
        left = c4.m_x >> TILE_SHIFT_PX;
    }
    RECT box;
    box.left = left - 3;
    box.top = top - 3;
    box.right = right + 4;
    box.bottom = bottom + 4;
    // CMapMgr::Clip(&box) expanded; cl5 does not fold `&box != NULL`, so both
    // arms survive and the `+1` belongs to Clip's true arm, not to box.
    {
        const RECT* src = &box;
        CMapMgr* board = m_board;
        CRect b(0, 0, board->m_width, board->m_height);
        RECT a;
        if (src != NULL) {
            a = *src;
            a.right++;
            a.bottom++;
        } else {
            a = CRect(0, 0, board->m_width, board->m_height);
        }
        RECT* aDst = &board->m_bounds;
        if (!IntersectRect(aDst, &a, &b)) {
            *aDst = a;
        }
        board->m_gridW = aDst->right - aDst->left;
        board->m_gridH = aDst->bottom - aDst->top;
    }

    CDDrawChildGroup* coll = m_ctx->m_world->m_childGroup;
    coll->m_scanCursor = coll->m_list.GetHeadPosition();
    CGameObject* g = static_cast<CGameObject*>(coll->Drain());
    while (g != NULL) {
        if (g->m_animWorker->m_notify == &CreateInGameIcon
            && !HAS(g->m_stateFlags, SPRITE_STATE_HIDDEN)) {
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
            POINT wpt;
            wpt.x = gx;
            wpt.y = gy;
            if (PtInRect(&box, wpt)) {
                if (special != 0 && unit->m_gruntKind == GRUNT_NORMAL) {
                    if (RouteUnitTo(unit, gx, gy, 0x2000098b, 0, 0) != 0) {
                        // CMapMgr::Clip(NULL): the constant src folds, leaving
                        // only the else arm's second CRect construction.
                        CMapMgr* bd = m_board;
                        RECT b;
                        b.left = 0;
                        b.top = 0;
                        b.right = bd->m_width;
                        b.bottom = bd->m_height;
                        RECT a;
                        a = CRect(0, 0, bd->m_width, bd->m_height);
                        RECT* aDst = &bd->m_bounds;
                        if (!IntersectRect(aDst, &a, &b)) {
                            *aDst = a;
                        }
                        bd->m_gridW = aDst->right - aDst->left;
                        bd->m_gridH = aDst->bottom - aDst->top;
                        return 1;
                    }
                } else {
                    PickupType entranceMode = unit->m_entranceReason;
                    if (entranceMode > PICKUP_EQUIPPABLE_LAST) {
                        entranceMode = unit->m_toolId;
                    }
                    if (entranceMode == PICKUP_NONE) {
                        if (RouteUnitTo(unit, gx, gy, 0x2000098b, 0, 0) != 0) {
                            CMapMgr* bd = m_board;
                            CRect b(0, 0, bd->m_width, bd->m_height);
                            RECT a;
                            a = CRect(0, 0, bd->m_width, bd->m_height);
                            RECT* aDst = &bd->m_bounds;
                            if (!IntersectRect(aDst, &a, &b)) {
                                *aDst = a;
                            }
                            bd->m_gridW = aDst->right - aDst->left;
                            bd->m_gridH = aDst->bottom - aDst->top;
                            return 1;
                        }
                    }
                }
            }
        }

        // CDDrawChildGroup::Drain's first iteration, inlined by cl: the NULL arm
        // is its own `xor eax,eax; jmp` block behind a positive gate, and the
        // node is read with ONE GetNext (pNext first, then data), not GetAt+GetNext.
        CDDrawChildGroup* c = m_ctx->m_world->m_childGroup;
        if (c->m_scanCursor == NULL) {
            g = NULL;
        } else {
            CGameObject* pp = static_cast<CGameObject*>(c->m_list.GetNext(c->m_scanCursor));
            if (pp->GetClassId() == CLASSID_SERIALREF) {
                g = pp;
            } else {
                g = static_cast<CGameObject*>(c->Drain());
            }
        }
    }
    m_board->Clip(static_cast<const RECT*>(0));
    return 0;
}

#define ARR_RECYCLE(g)                                                                             \
    if ((g)->CoordCount() != 0) {                                                                  \
        CoordNode* nd = (g)->CoordHead();                                                          \
        while (nd != 0) {                                                                          \
            CoordNode* cur = nd;                                                                   \
            nd = nd->m_next;                                                                       \
            if (cur->m_coord != 0) {                                                               \
                g_coordPool.Push(cur->m_coord);                                                    \
            }                                                                                      \
        }                                                                                          \
        coordList->RemoveAll();                                                                    \
    }

// The engine's "which pickup is this grunt arriving with" select, spelled out at
// every use site in retail (cl re-evaluates it per site; it never CSEs).
static __inline PickupType ArrivalPickup(CGrunt* g) {
    PickupType p = g->m_entranceReason;
    if (p > PICKUP_EQUIPPABLE_LAST) {
        p = g->m_toolId;
    }
    return p;
}

// The same select against an entrance reason the caller already holds.  A block
// that asks more than once loads m_entranceReason ONCE and keeps it live, which
// is why retail's guard there reads `cmp er,0x16 / mov p,er / jle / mov p,tool`
// (a second register) instead of overwriting the loaded value in place.
static __inline PickupType ArrivalPickupOf(CGrunt* g, PickupType er) {
    PickupType p = er;
    if (er > PICKUP_EQUIPPABLE_LAST) {
        p = g->m_toolId;
    }
    return p;
}

static __inline i32 arrCell(CMapMgr* grid, i32 col, i32 row) {
    if (static_cast<u32>(col) < static_cast<u32>(grid->m_width)
        && static_cast<u32>(row) < static_cast<u32>(grid->m_height)) {
        return grid->m_rows[row][col].m_flags;
    }
    return 1;
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

// @early-stop
RVA(0x0002c690, 0xdb4)
i32 CBattlezMapConfig::ResolveArrival(CGrunt* g) {
    CPtrList* coordList = &g->m_coordList;
    if (RepathAroundBlockedTiles(g)) {
        return 1;
    }
    if (coordList->GetCount() == 0) {
        return 0;
    }

    CoordNode* head = MfcNodeFromPosition<CoordNode>(coordList->GetHeadPosition());
    Coord* fc = head->m_coord;
    i32 fcx = fc->m_x;
    i32 fcy = fc->m_y;

    Coord a;
    g->GetScreenTile(&a);
    i32 gy = a.m_y;
    i32 gx = a.m_x;
    Coord b;
    g->GetScreenTile(&b);
    i32 bx = b.m_x;

    BrickzCell dest;
    BrickzCell* dsrc;
    if (static_cast<u32>(bx) < static_cast<u32>(m_board->m_width)
        && static_cast<u32>(gy) < static_cast<u32>(m_board->m_height)) {
        dsrc = &m_board->m_rows[gy][bx];
    } else {
        memset(&dest, 1, 0x1c);
        dsrc = &dest;
    }
    dest = *dsrc;
    static_cast<void>(gx);

    i32 ownFlags;
    {
        BrickzCell own;
        BrickzCell* osrc;
        if (static_cast<u32>(fcx) < static_cast<u32>(m_board->m_width)
            && static_cast<u32>(fcy) < static_cast<u32>(m_board->m_height)) {
            osrc = &m_board->m_rows[fcy][fcx];
        } else {
            memset(&own, 1, 0x1c);
            osrc = &own;
        }
        own = *osrc;
        ownFlags = own.m_flags;
    }

    i32 maskFlags = ownFlags & BRICKZ_CELL_UNOCCUPIED_MASK;
    // Retail spells THIS one with the `<=` arm first: its two stores to the
    // `type` slot are ordered entranceReason-then-toolId behind a `jg`, which
    // is the arm order only the `<=` condition produces.
    PickupType type =
        (g->m_entranceReason <= PICKUP_EQUIPPABLE_LAST) ? g->m_entranceReason : g->m_toolId;

    if ((dest.m_flags & 0x400) && g->m_defenderState == AISTATE_RETURN
        && ArrivalPickup(g) != PICKUP_GRAVITYBOOTZ) {
        if (ownFlags & 0x4000) {
            {
                RECT box;
                g->GetScreenTile(&a);
                box.bottom = a.m_y + 2;
                g->GetScreenTile(&b);
                box.right = b.m_x + 2;
                box.top = (g->m_object->m_screenY >> TILE_SHIFT_PX) - 1;
                {
                    Coord c;
                    g->GetScreenPos(&c);
                    box.left = (c.m_x >> TILE_SHIFT_PX) - 1;
                }

                // CMapMgr::Clip(&box) expanded; cl5 keeps both arms of `&box != NULL`.
                {
                    const RECT* src = &box;
                    CMapMgr* board = m_board;
                    CRect clipFull(0, 0, board->m_width, board->m_height);
                    RECT clipBox;
                    if (src != NULL) {
                        clipBox = *src;
                        clipBox.right = clipBox.right + 1;
                        clipBox.bottom = clipBox.bottom + 1;
                    } else {
                        clipBox = CRect(0, 0, board->m_width, board->m_height);
                    }
                    RECT* clipBoxDst = &board->m_bounds;
                    if (!IntersectRect(clipBoxDst, &clipBox, &clipFull)) {
                        *clipBoxDst = clipBox;
                    }
                    board->m_gridW = clipBoxDst->right - clipBoxDst->left;
                    board->m_gridH = clipBoxDst->bottom - clipBoxDst->top;
                }
            }

            RECT scan = m_board->m_bounds;

            g->GetScreenTile(&a);
            i32 stepDy = a.m_y - fcy;
            i32 stepDx;
            {
                Coord c;
                g->GetScreenPos(&c);
                stepDx = (c.m_x >> TILE_SHIFT_PX) - fcx;
            }
            if (g->TileSwitch(stepDx, stepDy, 0, 0x20000983, 1, 0) == 0) {
                for (i32 scanRow = scan.top; scanRow < scan.bottom; scanRow++) {
                    BrickzCell* rowCell = &m_board->m_rows[scanRow][scan.left];
                    for (i32 scanCol = scan.left; scanCol < scan.right; scanCol++) {
                        CPtrList path(0xa);
                        if (!(rowCell->m_flags & BRICKZ_CELL_OCCUPIED)) {
                            CGameObject* lvl = g->m_object;
                            if (m_board->SearchEdge(
                                    lvl->m_screenX >> TILE_SHIFT_PX,
                                    lvl->m_screenY >> TILE_SHIFT_PX,
                                    scanCol,
                                    scanRow,
                                    &path,
                                    0,
                                    0x20004d03,
                                    0
                                ) != 0
                                && path.GetCount() != 0) {
                                Coord* head = static_cast<Coord*>(path.RemoveHead());
                                if (head != NULL) {
                                    CoordPoolNode* node = g_coordPool.NodeOf(head);
                                    node->m_next = g_coordPool.m_freeHead;
                                    g_coordPool.m_freeHead = node;
                                }
                                if (path.GetCount() != 0) {
                                    ARR_RECYCLE(g);
                                    POSITION qp = path.GetHeadPosition();
                                    while (qp != NULL) {
                                        Coord* step = static_cast<Coord*>(path.GetNext(qp));
                                        g->m_coordList.AddTail(step);
                                    }
                                    Coord* nt = (g->CoordTail())->m_coord;
                                    g->m_entrancePx.m_x = (nt->m_x << TILE_SHIFT_PX) + TILE_HALF_PX;
                                    g->m_entrancePx.m_y = (nt->m_y << TILE_SHIFT_PX) + TILE_HALF_PX;
                                    // CMapMgr::Clip(NULL): the constant src folds to the
                                    // else arm alone.
                                    CMapMgr* bd = m_board;
                                    RECT pathFull;
                                    pathFull.left = 0;
                                    pathFull.top = 0;
                                    pathFull.right = bd->m_width;
                                    pathFull.bottom = bd->m_height;
                                    RECT pathBox;
                                    pathBox = CRect(0, 0, bd->m_width, bd->m_height);
                                    RECT* pathBoxDst = &bd->m_bounds;
                                    if (!IntersectRect(pathBoxDst, &pathBox, &pathFull)) {
                                        *pathBoxDst = pathBox;
                                    }
                                    bd->m_gridW = pathBoxDst->right - pathBoxDst->left;
                                    bd->m_gridH = pathBoxDst->bottom - pathBoxDst->top;
                                    return 1;
                                }
                            }
                        }
                    }
                }
            }
        }

        // CMapMgr::Clip(NULL): the constant src folds to the else arm alone.
        {
            CMapMgr* bd = m_board;
            CRect b(0, 0, bd->m_width, bd->m_height);
            RECT a;
            a = CRect(0, 0, bd->m_width, bd->m_height);
            RECT* aDst = &bd->m_bounds;
            if (!IntersectRect(aDst, &a, &b)) {
                *aDst = a;
            }
            bd->m_gridW = aDst->right - aDst->left;
            bd->m_gridH = aDst->bottom - aDst->top;
        }
    }

    if ((dest.m_flags & 4) && g->m_battleState != BZTASK_SEEK_SWITCH) {
        Coord tp;
        i32 keyHi = g->m_object->m_screenX >> TILE_SHIFT_PX;
        g->GetScreenTile(&tp);
        i32 key = (keyHi << 8) + tp.m_y;
        CTileTriggerSwitchLogic* r = m_cellQuery->FindChild(key, TRIGID_ANY);
        if (r->m_typeId == TRIGID_SWITCH_2) {
            g->m_defenderState = AISTATE_SEEK;
            ARR_RECYCLE(g);
            g->m_battleState = BZTASK_SEEK_SWITCH;
            g->m_dwell = 0;
            return 0;
        }
    }

    if ((maskFlags & 0x8000) && type == PICKUP_BRICK && g->m_battleState == BZTASK_CARRY_BRICK) {
        m_triggerMgr->ApplyTriggerA(
            g->m_tileOwnerHi,
            g->m_tileOwnerLo,
            (fcx << TILE_SHIFT_PX) + TILE_HALF_PX,
            (fcy << TILE_SHIFT_PX) + TILE_HALF_PX
        );
        ARR_RECYCLE(g);
        return 0;
    }

    if ((maskFlags & 0x4000) && type == PICKUP_BRICK && g->m_battleState == BZTASK_CARRY_BRICK) {
        if (m_board->m_rows[fcy][fcx].m_typeCode != TILEKIND_GAUNTLET_BRICK_C) {
            m_triggerMgr->ApplyTriggerA(
                g->m_tileOwnerHi,
                g->m_tileOwnerLo,
                (fcx << TILE_SHIFT_PX) + TILE_HALF_PX,
                (fcy << TILE_SHIFT_PX) + TILE_HALF_PX
            );
            ARR_RECYCLE(g);
            return 0;
        }
        ARR_RECYCLE(g);
        return 0;
    }

    if (maskFlags & 0x200) {
        return 1;
    }

    if (maskFlags & 0x8) {
        if (PathToNearestGoal(g, fcx, fcy) != 0) {
            return 1;
        }
        EnterDefenderMode(g, 0x12);
    }

    if (maskFlags & 0x20) {
        PickupType er = g->m_entranceReason;
        if (ArrivalPickupOf(g, er) == PICKUP_BOMB || ArrivalPickupOf(g, er) == PICKUP_TIMEBOMB) {
            if (ArrivalPickupOf(g, er) == PICKUP_BOMB) {
                m_triggerMgr->ApplyTriggerA(
                    g->m_tileOwnerHi,
                    g->m_tileOwnerLo,
                    (fcx << TILE_SHIFT_PX) + TILE_HALF_PX,
                    (fcy << TILE_SHIFT_PX) + TILE_HALF_PX
                );
                return 1;
            }
            if (ArrivalPickupOf(g, er) == PICKUP_TIMEBOMB) {
                for (i32 row = fcy - 1; row < fcy + 2; row++) {
                    for (i32 col = fcx - 1; col < fcx + 2; col++) {
                        if (static_cast<u32>(col) < static_cast<u32>(m_board->m_width)
                            && static_cast<u32>(row) < static_cast<u32>(m_board->m_height)) {
                            i32 cf = arrCell(m_board, col, row);
                            if (cf & BRICKZ_BLOCKED_MASK) {
                                return 1;
                            }
                            i32 hitX = (col << TILE_SHIFT_PX) + TILE_HALF_PX;
                            i32 hitY = (row << TILE_SHIFT_PX) + TILE_HALF_PX;
                            if (g->RectContains(hitX, hitY) != 0) {
                                m_triggerMgr
                                    ->ApplyTriggerA(g->m_tileOwnerHi, g->m_tileOwnerLo, hitX, hitY);
                            }
                            return 1;
                        }
                    }
                }
            }
        }
    }

    if (maskFlags & 0x4000) {
        PickupType t =
            (g->m_entranceReason > PICKUP_EQUIPPABLE_LAST) ? g->m_toolId : g->m_entranceReason;
        if (t == PICKUP_SPY) {
            CTileActionEvent* r = m_cellQuery->FindActionByCellKey((fcx << 8) + fcy);
            if (r != NULL) {
                if (r->m_playerFlags[m_ownerId] != 0) {
                    ARR_RECYCLE(g);
                    ResolveTileClaim(g, fcx, fcy, 1);
                    return 1;
                }
                m_triggerMgr->ApplyTriggerA(
                    g->m_tileOwnerHi,
                    g->m_tileOwnerLo,
                    (fcx << TILE_SHIFT_PX) + TILE_HALF_PX,
                    (fcy << TILE_SHIFT_PX) + TILE_HALF_PX
                );
                return 1;
            }
        }
    }

    if (maskFlags & 0x8000) {
        PickupType t =
            (g->m_entranceReason > PICKUP_EQUIPPABLE_LAST) ? g->m_toolId : g->m_entranceReason;
        if (t == PICKUP_SPY) {
            ARR_RECYCLE(g);
            ResolveTileClaim(g, fcx, fcy, 1);
            return 1;
        }
    }

    if (maskFlags & 0x20) {
        PickupType t =
            (g->m_entranceReason > PICKUP_EQUIPPABLE_LAST) ? g->m_toolId : g->m_entranceReason;
        if (t == PICKUP_GAUNTLETZ) {
            if (maskFlags & 0x4000) {
                CTileActionEvent* r = m_cellQuery->FindActionByCellKey((fcx << 8) + fcy);
                if (r != NULL) {
                    BrickTileId k = static_cast<BrickTileId>(r->m_actionCode);
                    if (r->m_playerFlags[m_ownerId] != 0) {
                        if (k == BRICKTILE_GOLD_1 || k == BRICKTILE_GOLD_2_TOP
                            || k == BRICKTILE_GOLD_3_TOP) {
                            ResolveTileClaim(g, fcx, fcy, 0);
                        }
                    } else {
                        if (k == BRICKTILE_GOLD_1 || k == BRICKTILE_GOLD_2_TOP
                            || k == BRICKTILE_GOLD_3_TOP) {
                            m_cellQuery->SetCell(fcx, fcy, m_ownerId);
                        }
                    }
                }
            }
            m_triggerMgr->ApplyTriggerA(
                g->m_tileOwnerHi,
                g->m_tileOwnerLo,
                (fcx << TILE_SHIFT_PX) + TILE_HALF_PX,
                (fcy << TILE_SHIFT_PX) + TILE_HALF_PX
            );
            return 0;
        }
        if (t == PICKUP_TIMEBOMB || t == PICKUP_BOMB) {
            return 1;
        }
        i32 flag = 1;
        if (t == PICKUP_BRICK && (maskFlags & 0x4000)) {
            flag = 0;
        }
        if (t == PICKUP_SPY && (maskFlags & 0x4000)) {
            flag = 0;
        }
        if (flag == 0) {
            return 1;
        }
        EnterDefenderMode(g, 5);
        return 0;
    }

    if (maskFlags & 0x40) {
        PickupType er2 = g->m_entranceReason;
        if (ArrivalPickupOf(g, er2) != PICKUP_WINGZ) {
            if (ArrivalPickupOf(g, er2) == PICKUP_SHOVEL) {
                m_triggerMgr->ApplyTriggerA(
                    g->m_tileOwnerHi,
                    g->m_tileOwnerLo,
                    (fcx << TILE_SHIFT_PX) + TILE_HALF_PX,
                    (fcy << TILE_SHIFT_PX) + TILE_HALF_PX
                );
                return 0;
            }
            EnterDefenderMode(g, 0xd);
            return 0;
        }
    }

    PathToNearestCandidate(g, 0, 0, 0);
    if (PathCrossesMarkedTile(g) != 0) {
        return 1;
    }
    {
        PickupType t =
            (g->m_entranceReason > PICKUP_EQUIPPABLE_LAST) ? g->m_toolId : g->m_entranceReason;
        if (t == PICKUP_WINGZ) {
            return 1;
        }
    }
    {
        i32 oy = g->m_object->m_screenY >> TILE_SHIFT_PX;
        i32 ox = g->m_object->m_screenX >> TILE_SHIFT_PX;
        i32 row = rand() % 3 + oy - 1;
        i32 col = rand() % 3 + ox - 1;
        if (static_cast<u32>(col) >= static_cast<u32>(m_board->m_width)
            || static_cast<u32>(row) >= static_cast<u32>(m_board->m_height)) {
            return 1;
        }
        i32 c0 = arrCell(m_board, col, row);
        i32 c1 = arrCell(m_board, col, row) & 0x987;
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

#undef ARR_RECYCLE

// @early-stop
// cl strength-reduces the loop-carried col/row derivatives (col*0x1c, col<<8,
// row*4 decremented in slots, frame 0x68) where retail recomputes them from
// col/row each iteration (frame 0x60), and retail routes list2's dtor jmp INTO
// list3's `call ??1CPtrList` (one shared call instruction, receivers differ in
// ecx). The while spelling and the guard+tail-recursion spelling compile
// byte-identically - MSVC5 eliminates the self tail call BEFORE loop opts, so
// the source form cannot reach either decision.
RVA(0x0002d800, 0x605)
void CBattlezMapConfig::ClaimTilesAround(CGrunt* unit, i32 col, i32 row, i32 requireUnoccupied) {
    while (g_stepRun != 0) {

        i32 word = m_board->m_rows[row][col].m_flags;
        if (word & 0x8000) {
            CPtrList list(10);
            CGameObject* lvl = unit->m_object;
            if ((m_board)->SearchEdge(
                    lvl->m_screenX >> TILE_SHIFT_PX,
                    lvl->m_screenY >> TILE_SHIFT_PX,
                    col,
                    row,
                    &list,
                    1,
                    0x4903,
                    0
                )
                != 0) {
                CoordNode* head = MfcNodeFromPosition<CoordNode>(list.GetHeadPosition());
                g_stepRun = 0;
                g_stepCol = col;
                g_stepRow = row;
                if (head != NULL) {
                    CoordNode* n = head;
                    while (n != NULL) {
                        CoordNode* cur = n;
                        n = n->m_next;
                        CoordPoolNode* node = g_coordPool.NodeOf(cur->m_coord);
                        node->m_next = g_coordPool.m_freeHead;
                        g_coordPool.m_freeHead = node;
                    }
                }
                break;
            }
        }
        if (word & 0x4000) {
            CTileActionEvent* cell = m_cellQuery->FindActionByCellKey((col << 8) + row);
            if (requireUnoccupied != 0) {
                if (cell != NULL && cell->m_playerFlags[m_ownerId] == 0) {
                    CPtrList list2(10);
                    CGameObject* lvl = unit->m_object;
                    if ((m_board)->SearchEdge(
                            lvl->m_screenX >> TILE_SHIFT_PX,
                            lvl->m_screenY >> TILE_SHIFT_PX,
                            col,
                            row,
                            &list2,
                            1,
                            0x4003,
                            0
                        )
                        != 0) {
                        CoordNode* head = MfcNodeFromPosition<CoordNode>(list2.GetHeadPosition());
                        g_stepRun = 0;
                        g_stepCol = col;
                        g_stepRow = row;
                        if (head != NULL) {
                            CoordNode* n = head;
                            while (n != NULL) {
                                CoordNode* cur = n;
                                n = n->m_next;
                                CoordPoolNode* node = g_coordPool.NodeOf(cur->m_coord);
                                node->m_next = g_coordPool.m_freeHead;
                                g_coordPool.m_freeHead = node;
                            }
                        }
                    }
                }
            } else if (cell != NULL) {
                BrickTileId id = static_cast<BrickTileId>(cell->m_actionCode);
                i32 occ = cell->m_playerFlags[m_ownerId];
                // TWO separate ifs, not if/else-if: retail re-tests `occ`
                // (`test eax,eax; jne ladder` / `test eax,eax; mov edx,1; je done`)
                // and keeps two distinct `special = 1` stores.
                i32 special = 0;
                if (occ == 0) {
                    special = 1;
                }
                if (occ != 0) {
                    if (id == BRICKTILE_RED_1 || id == BRICKTILE_RED_2_TOP
                        || id == BRICKTILE_RED_3_TOP || id == BRICKTILE_BLACK_1
                        || id == BRICKTILE_BLACK_2_TOP || id == BRICKTILE_BLACK_3_TOP
                        || id == BRICKTILE_BLUE_1 || id == BRICKTILE_BLUE_2_TOP
                        || id == BRICKTILE_BLUE_3_TOP || id == BRICKTILE_BROWN_1
                        || id == BRICKTILE_BROWN_2 || id == BRICKTILE_BROWN_3) {
                        special = 1;
                    }
                }
                if (special != 0) {
                    CPtrList list3(10);
                    CGameObject* lvl = unit->m_object;
                    if ((m_board)->SearchEdge(
                            lvl->m_screenX >> TILE_SHIFT_PX,
                            lvl->m_screenY >> TILE_SHIFT_PX,
                            col,
                            row,
                            &list3,
                            1,
                            0x4003,
                            0
                        )
                        != 0) {
                        // Only this third block gates on the count: retail reads
                        // list3+0xc (m_nCount) and skips the whole store/recycle
                        // when it is 0, before it touches m_pNodeHead at +4.
                        if (list3.GetCount() != 0) {
                            CoordNode* head =
                                MfcNodeFromPosition<CoordNode>(list3.GetHeadPosition());
                            g_stepRun = 0;
                            g_stepCol = col;
                            g_stepRow = row;
                            if (head != NULL) {
                                CoordNode* n = head;
                                while (n != NULL) {
                                    CoordNode* cur = n;
                                    n = n->m_next;
                                    CoordPoolNode* node = g_coordPool.NodeOf(cur->m_coord);
                                    node->m_next = g_coordPool.m_freeHead;
                                    g_coordPool.m_freeHead = node;
                                }
                            }
                        }
                    }
                }
            }
        }

        m_board->m_rows[row][col].m_flags |= 0x20000;
        i32 cm = col - 1;
        i32 cp = col + 1;
        i32 rm = row - 1;
        i32 rp = row + 1;
        CMapMgr* b;
        i32 nw;

        b = m_board;
        if (static_cast<u32>(cm) < static_cast<u32>(b->m_width)) {
            nw = b->m_rows[row][cm].m_flags;
            if (!(nw & 0x20000)
                && ((nw & 0xc000) || b->m_rows[row][cm].m_typeCode == TILEKIND_AI_PATH_BLOCKER)) {
                ClaimTilesAround(unit, cm, row, requireUnoccupied);
            }
        }
        b = m_board;
        if (static_cast<u32>(cp) < static_cast<u32>(b->m_width)) {
            nw = b->m_rows[row][cp].m_flags;
            if (!(nw & 0x20000)
                && ((nw & 0xc000) || b->m_rows[row][cp].m_typeCode == TILEKIND_AI_PATH_BLOCKER)) {
                ClaimTilesAround(unit, cp, row, requireUnoccupied);
            }
        }
        b = m_board;
        if (static_cast<u32>(rm) < static_cast<u32>(b->m_width)) {
            nw = b->m_rows[rm][col].m_flags;
            if (!(nw & 0x20000)
                && ((nw & 0xc000) || b->m_rows[rm][col].m_typeCode == TILEKIND_AI_PATH_BLOCKER)) {
                ClaimTilesAround(unit, col, rm, requireUnoccupied);
            }
        }
        b = m_board;
        if (static_cast<u32>(rp) < static_cast<u32>(b->m_width)) {
            nw = b->m_rows[rp][col].m_flags;
            if (!(nw & 0x20000)
                && ((nw & 0xc000) || b->m_rows[rp][col].m_typeCode == TILEKIND_AI_PATH_BLOCKER)) {
                ClaimTilesAround(unit, col, rp, requireUnoccupied);
            }
        }
        b = m_board;
        if (static_cast<u32>(cp) < static_cast<u32>(b->m_width)
            && static_cast<u32>(rm) < static_cast<u32>(b->m_height)) {
            nw = b->m_rows[rm][cp].m_flags;
            if (!(nw & 0x20000)
                && ((nw & 0xc000) || b->m_rows[rm][cp].m_typeCode == TILEKIND_AI_PATH_BLOCKER)) {
                ClaimTilesAround(unit, cp, rm, requireUnoccupied);
            }
        }
        b = m_board;
        if (static_cast<u32>(cp) < static_cast<u32>(b->m_width)
            && static_cast<u32>(rp) < static_cast<u32>(b->m_height)) {
            nw = b->m_rows[rp][cp].m_flags;
            if (!(nw & 0x20000)
                && ((nw & 0xc000) || b->m_rows[rp][cp].m_typeCode == TILEKIND_AI_PATH_BLOCKER)) {
                ClaimTilesAround(unit, cp, rp, requireUnoccupied);
            }
        }
        b = m_board;
        if (static_cast<u32>(cm) < static_cast<u32>(b->m_width)
            && static_cast<u32>(rp) < static_cast<u32>(b->m_height)) {
            nw = b->m_rows[rp][cm].m_flags;
            if (!(nw & 0x20000)
                && ((nw & 0xc000) || b->m_rows[rp][cm].m_typeCode == TILEKIND_AI_PATH_BLOCKER)) {
                ClaimTilesAround(unit, cm, rp, requireUnoccupied);
            }
        }

        b = m_board;
        if (static_cast<u32>(cm) >= static_cast<u32>(b->m_width)
            || static_cast<u32>(rm) >= static_cast<u32>(b->m_height)) {
            break;
        }
        nw = b->m_rows[rm][cm].m_flags;
        if ((nw & 0x20000)
            || (!(nw & 0xc000) && b->m_rows[rm][cm].m_typeCode != TILEKIND_AI_PATH_BLOCKER)) {
            break;
        }
        row = rm;
        col = cm;
    }
}

// @early-stop
RVA(0x0002dfa0, 0x325)
i32 CBattlezMapConfig::ResolveTileClaim(CGrunt* unit, i32 col, i32 row, i32 requireUnoccupied) {
    g_stepRun = 1;

    i32 bottom;
    i32 right;
    i32 top;
    i32 left;
    {
        CGameObject* lvl = unit->m_object;
        bottom = lvl->m_screenY >> TILE_SHIFT_PX;
        // Same seed-the-last-probe idiom as RouteToNearbyEnemy: retail stores
        // probe 1's unused half into g2's slot before g2 is probed.
        Coord g0;
        Coord g1;
        Coord g2;
        (static_cast<CUserLogic*>(unit))->GetScreenTile(&g0);
        g2.m_y = g0.m_y;
        right = g0.m_x;
        (static_cast<CUserLogic*>(unit))->GetScreenTile(&g1);
        g2.m_x = g1.m_x;
        top = g1.m_y;
        (static_cast<CUserLogic*>(unit))->GetScreenTile(&g2);
        left = g2.m_x;
    }
    // The +-8 belongs to the RECT, not to the probes: retail's `add r,8` /
    // `add r,-8` pairs sit immediately before the CRect ctor call, not next to
    // the `sar r,5` that produced each edge.
    RECT box;
    box.left = left - 8;
    box.top = top - 8;
    box.right = right + 8;
    box.bottom = bottom + 8;
    // CMapMgr::Clip(&box) expanded: cl declines to inline the 170-byte body, but
    // it does NOT fold `&box != NULL`, so the else arm survives here.
    {
        const RECT* src = &box;
        CMapMgr* board = m_board;
        CRect b(0, 0, board->m_width, board->m_height);
        RECT a;
        if (src != NULL) {
            a = *src;
            a.right++;
            a.bottom++;
        } else {
            a = CRect(0, 0, board->m_width, board->m_height);
        }
        RECT* aDst = &board->m_bounds;
        if (!IntersectRect(aDst, &a, &b)) {
            *aDst = a;
        }
        board->m_gridW = aDst->right - aDst->left;
        board->m_gridH = aDst->bottom - aDst->top;
    }
    ClaimTilesAround(unit, col, row, requireUnoccupied);
    if (g_stepRun == 0) {
        i32 savedX = unit->m_entrancePx.m_x;
        i32 savedY = unit->m_entrancePx.m_y;
        i32 col = unit->m_entrancePx.m_x >> TILE_SHIFT_PX;
        i32 row = unit->m_entrancePx.m_y >> TILE_SHIFT_PX;
        u32 tile0;
        if (static_cast<u32>(col) < static_cast<u32>(m_board->m_width)
            && static_cast<u32>(row) < static_cast<u32>(m_board->m_height)) {
            tile0 = m_board->m_rowInts[row][col * 7];
        } else {
            tile0 = 1;
        }
        i32 flag = (tile0 >> 2) & 1;
        if (unit->CoordCount() != 0) {
            Coord* c = (unit->CoordTail())->m_coord;
            i32 cx = c->m_x;
            i32 cy = c->m_y;
            i32 tile1;
            if (static_cast<u32>(cx) < static_cast<u32>(m_board->m_width)
                && static_cast<u32>(cy) < static_cast<u32>(m_board->m_height)) {
                tile1 = m_board->m_rowInts[cy][cx * 7];
            } else {
                tile1 = 1;
            }
            if (tile1 & 4) {
                savedX = c->m_x;
                savedY = c->m_y;
                flag = 1;
            }
        }
        unit->TileSwitch(g_stepCol, g_stepRow, 0, 0x9c3, 1, 0);
        if (flag != 0) {
            unit->m_entrancePx.m_x = savedX;
            unit->m_entrancePx.m_y = savedY;
        }
    }

    RECT sweep = m_board->m_bounds;
    if (sweep.left < sweep.right) {
        i32 colOff = (sweep.left * 7) << 2;
        i32 w = sweep.right - sweep.left;
        do {
            for (i32 r = sweep.top; r < sweep.bottom; r++) {
                m_board->m_rowBytes[r][colOff + 2] &= 0xfd;
            }
            colOff += 0x1c;
        } while (--w != 0);
    }

    // CMapMgr::Clip(NULL) expanded: with a constant-NULL src cl folds the test
    // away and only the else arm - a struct copy of the board rect - survives.
    {
        CMapMgr* board = m_board;
        RECT b;
        b.left = 0;
        b.top = 0;
        b.right = board->m_width;
        b.bottom = board->m_height;
        RECT a;
        a = b;
        RECT* aDst = &board->m_bounds;
        if (!IntersectRect(aDst, &a, &b)) {
            *aDst = a;
        }
        board->m_gridW = aDst->right - aDst->left;
        board->m_gridH = aDst->bottom - aDst->top;
    }
    return 1;
}

RVA(0x0002e3a0, 0x7e1)
i32 CBattlezMapConfig::RouteToNearbyEnemy(CGrunt* unit) {

    i32 bottom;
    i32 right;
    i32 top;
    i32 left;
    {
        // Each probe seeds cD's other axis before cD is itself probed: retail
        // stores the unused half of probes 1..3 into the slot probe 4 later
        // overwrites (the same idiom GruntPhaseStep spells).
        Coord cA;
        Coord cB;
        Coord cC;
        Coord cD;
        unit->GetScreenTile(&cA);
        cD.m_x = cA.m_x;
        bottom = cA.m_y;
        unit->GetScreenTile(&cB);
        cD.m_y = cB.m_y;
        right = cB.m_x;
        unit->GetScreenTile(&cC);
        cD.m_x = cC.m_x;
        top = cC.m_y;
        unit->GetScreenTile(&cD);
        left = cD.m_x;
    }
    RECT box;
    box.left = left - 7;
    box.top = top - 7;
    box.right = right + 7;
    box.bottom = bottom + 7;

    CGrunt* best = 0;
    i32 bestDist = INT_MAX;
    for (i32 band = 0; band < 4; band++) {
        if (band == m_ownerId) {
            continue;
        }
        for (i32 i = 0; i < 15; i++) {
            CGrunt* u = m_triggerMgr->m_grid[band * 15 + i];
            if (u == NULL) {
                continue;
            }
            if (u->m_entranceCommitted == 0) {
                continue;
            }
            if (u->m_deathAnimStarted != 0) {
                continue;
            }
            if (u->m_entranceActive != 0) {
                continue;
            }
            if (u->m_poweredUp != 0) {
                continue;
            }
            bool ne;
            ne = strcmp((*g_typeColl.GetNameRecord(u->m_objAux->m_actKey)), "C") != 0;
            if (!ne) {
                continue;
            }
            ne = strcmp((*g_typeColl.GetNameRecord(u->m_objAux->m_actKey)), "R") != 0;
            if (!ne) {
                continue;
            }
            ne = strcmp((*g_typeColl.GetNameRecord(u->m_objAux->m_actKey)), "J") != 0;
            if (!ne) {
                continue;
            }
            ne = strcmp((*g_typeColl.GetNameRecord(u->m_objAux->m_actKey)), "G") != 0;
            if (!ne) {
                continue;
            }
            ne = strcmp((*g_typeColl.GetNameRecord(u->m_objAux->m_actKey)), "L") != 0;
            if (!ne) {
                continue;
            }
            if (u->m_gruntKind == GRUNT_GHOST) {
                continue;
            }
            Coord c;
            u->GetScreenTile(&c);
            POINT wpt;
            wpt.x = c.m_x;
            wpt.y = c.m_y;
            if (!PtInRect(&box, wpt)) {
                continue;
            }
            Coord unitPos1;
            unit->GetScreenTile(&unitPos1);
            Coord b1;
            u->GetScreenTile(&b1);
            i32 dx = abs(unitPos1.m_x - b1.m_x);
            Coord unitPos2;
            unit->GetScreenTile(&unitPos2);
            Coord b2;
            u->GetScreenTile(&b2);
            i32 dy = abs(unitPos2.m_y - b2.m_y);
            i32 dist = dx * dx + dy * dy;
            if (dist >= bestDist) {
                continue;
            }
            bestDist = dist;
            best = u;
        }
    }
    if (best != NULL) {
        if (static_cast<u32>(unit->m_dwell) > 0x64) {
            // CMapMgr::Clip(&box) expanded; cl5 keeps both arms of `&box != NULL`.
            {
                const RECT* src = &box;
                CMapMgr* board = m_board;
                CRect b(0, 0, board->m_width, board->m_height);
                RECT a;
                if (src != NULL) {
                    a = *src;
                    a.right++;
                    a.bottom++;
                } else {
                    a = CRect(0, 0, board->m_width, board->m_height);
                }
                RECT* aDst = &board->m_bounds;
                if (!IntersectRect(aDst, &a, &b)) {
                    *aDst = a;
                }
                board->m_gridW = aDst->right - aDst->left;
                board->m_gridH = aDst->bottom - aDst->top;
            }

            i32 flags = 0;
            PickupType prim = unit->m_entranceReason;
            PickupType t = prim;
            if (prim > PICKUP_EQUIPPABLE_LAST) {
                t = unit->m_toolId;
            }
            if (t == PICKUP_TOOB) {
                flags = 0x100;
            }
            t = prim;
            if (prim > PICKUP_EQUIPPABLE_LAST) {
                t = unit->m_toolId;
            }
            if (t == PICKUP_WINGZ) {
                flags = 0x942;
            }
            if (prim > PICKUP_EQUIPPABLE_LAST) {
                prim = unit->m_toolId;
            }
            if (prim == PICKUP_SPRING) {
                flags = 0x1000;
            }
            Coord bc;
            best->GetScreenTile(&bc);
            if (RouteUnitTo(unit, bc.m_x, bc.m_y, 0x1000d8f, flags, 1) != 0) {
                if (unit->m_defenderState != AISTATE_RETURN) {
                    unit->m_defenderState = AISTATE_SEEK;
                    unit->m_routeMaskC = 0;
                }
                if (unit->m_blockedVoicePending != 0) {
                    __int64 elapsed = static_cast<__int64>(g_frameTime) - m_routeClock.m_v;
                    if (elapsed >= m_routeWindow.m_v) {
                        unit->m_blockedVoicePending = 0;
                        CGameObject* lvl = unit->m_object;

                        RECT* hit = &g_gameReg->m_world->m_level->m_mainPlane->m_viewRect;
                        if (CGameLevel::PointInRect(hit, lvl->m_screenX, lvl->m_screenY)) {
                            g_gameReg->m_cueSink->SpawnVoiceDriver(unit, 0x366, -1, 0, -1, -1);
                        }
                        // Retail zeroes BOTH timers before re-arming - eight stores at 0x2e9e2
                        // (0x78/0x80/0x7c/0x84 = 0, then 0x80 = 0x1388 / 0x84 = 0, then the
                        // clock) - and we emitted only six.  The zero pass has to go through the
                        // ARRAY alias: written as `m_routeWindow.m_v = 0` cl proves the store
                        // dead against the 0x1388 that follows and drops it, and the re-arm has
                        // to stay two i32 halves for the same reason.
                        m_routeTimers[0].m_v = 0;
                        m_routeTimers[1].m_v = 0;
                        m_routeWindowLo = BLOCKED_VOICE_INTERVAL_MS;
                        m_routeWindowHi = 0;
                        m_routeClock.m_v = g_frameTime;
                    }
                }

                {
                    CMapMgr* board = m_board;
                    CRect gb(0, 0, board->m_width, board->m_height);
                    RECT grc;
                    grc = gb;
                    RECT* grcDst = &board->m_bounds;
                    if (!IntersectRect(grcDst, &grc, &gb)) {
                        *grcDst = grc;
                    }
                    board->m_gridW = grcDst->right - grcDst->left;
                    board->m_gridH = grcDst->bottom - grcDst->top;
                }
                unit->m_dwell = 0;
            } else {
                // CMapMgr::Clip(NULL): the constant src folds to the else arm alone.
                CMapMgr* board = m_board;
                CRect b(0, 0, board->m_width, board->m_height);
                RECT a;
                a = CRect(0, 0, board->m_width, board->m_height);
                RECT* aDst = &board->m_bounds;
                if (!IntersectRect(aDst, &a, &b)) {
                    *aDst = a;
                }
                board->m_gridW = aDst->right - aDst->left;
                board->m_gridH = aDst->bottom - aDst->top;
                unit->m_dwell = 0;
                return 0;
            }
        }
        return 1;
    }
    unit->m_blockedVoicePending = 1;
    return 0;
}

RVA(0x0002ed90, 0x5)
i32 CBattlezMapConfig::PathToNearbyUnit(CGrunt*) {
    return 0;
}

// @early-stop
RVA(0x0002edb0, 0x6b4)
i32 CBattlezMapConfig::PathToNearestCandidate(CGrunt* unit, i32 useArg, i32 ax, i32 ay) {
    if (unit->CoordCount() == 0) {
        return 0;
    }
    // The target cell is a Coord OBJECT, not two ints: retail keeps it in a
    // real 8-byte stack slot ([esp+0x14]/[esp+0x18]) written by a whole-object
    // copy on the found path, and leaves it uninitialised otherwise - it is
    // only read on the found == 1 path.
    Coord target;
    i32 found = 0;
    if (useArg == 0) {

        CoordNode* n = unit->CoordHead();
        while (n != NULL) {
            CoordNode* cur = n;
            n = n->m_next;
            Coord* c = cur->m_coord;
            if (c != NULL) {
                BrickzCell* row = m_board->m_rows[c->m_y];
                if (row[c->m_x].m_flags & 4) {
                    target = *c;
                    found = 1;
                    break;
                }
            }
        }

        // Everything from here to the join lives INSIDE the useArg == 0 arm:
        // retail's else arm (0x2ef2d) is entered only by the `jne` at the top
        // and falls straight into the FOURTH `found` gate at 0x2ef42, so the
        // three gates below are unreachable when the caller supplied the cell.
        // Each gate is a guarded BLOCK, not an early return: cl threads every
        // false edge onto the final `return 0`, which is what retail's three
        // `test esi,esi; je <ret0>` rows are.
        if (found != 0) {
            if (unit->m_defenderState == AISTATE_RETURN) {
                return 1;
            }
        }
        if (found != 0) {
            if (IsCoordOccupied(unit, target.m_x, target.m_y) != 0) {

                if (unit->CoordCount() != 0) {
                    CoordNode* n2 = unit->CoordHead();
                    while (n2 != NULL) {
                        CoordNode* cur = n2;
                        n2 = n2->m_next;
                        if (cur->m_coord != NULL) {
                            g_coordPool.Push(cur->m_coord);
                        }
                    }
                    unit->m_coordList.RemoveAll();
                }
                unit->m_defenderState = AISTATE_SEEK;
                return 1;
            }
        }
        if (found != 0 && PathCrossesMarkedTile(unit) != 0) {

            if (unit->CoordCount() != 0) {
                CoordNode* p = unit->CoordHead();
                Coord* c = p->m_coord;
                i32 word;
                CMapMgr* b = m_board;
                if (static_cast<u32>(c->m_x) < static_cast<u32>(b->m_width)
                    && static_cast<u32>(c->m_y) < static_cast<u32>(b->m_height)) {
                    word = b->m_rows[c->m_y][c->m_x].m_flags;
                } else {
                    word = 1;
                }
                if (!(word & BRICKZ_CELL_OCCUPIED)) {
                    return 1;
                }
            }
        }
    } else {
        target.m_x = ax;
        target.m_y = ay;
        found = 1;
    }
    if (found == 0) {
        return 0;
    }
    if (IsCoordOccupied(unit, target.m_x, target.m_y) != 0) {
        return 0;
    }

    // Bottom-tested count loop: retail's back-edge is `jl <top>` with the exit
    // as the fall-through, which a `break` on `scanned >= 15` inverts.
    i32 r = rand() % 15;
    for (i32 scanned = 0; scanned < 15; scanned++) {
        CGrunt* cand = m_triggerMgr->m_grid[m_ownerId * 15 + r];
        if (cand != NULL) {
            CGameObject* lvl = cand->m_object;
            if (lvl->m_screenX == cand->m_lastTilePx.m_x && lvl->m_screenY == cand->m_lastTilePx.m_y
                && cand->m_entranceCommitted != 0 && cand->m_deathAnimStarted == 0
                && cand->m_entranceActive == 0 && cand->m_poweredUp == 0) {
                bool eq;
                eq = (strcmp((*g_typeColl.GetNameRecord(cand->m_objAux->m_actKey)), "I") == 0);
                if (!eq) {
                    eq = (strcmp((*g_typeColl.GetNameRecord(cand->m_objAux->m_actKey)), "G") == 0);
                }
                if (!eq) {
                    eq = (strcmp((*g_typeColl.GetNameRecord(cand->m_objAux->m_actKey)), "L") == 0);
                }
                if (!eq) {
                    eq = (strcmp((*g_typeColl.GetNameRecord(cand->m_objAux->m_actKey)), "P") == 0);
                }
                if (!eq) {
                    eq = (strcmp((*g_typeColl.GetNameRecord(cand->m_objAux->m_actKey)), "J") == 0);
                }
                if (!eq) {
                    eq = (strcmp((*g_typeColl.GetNameRecord(cand->m_objAux->m_actKey)), "C") == 0);
                }
                if (!eq) {
                    eq = (strcmp((*g_typeColl.GetNameRecord(cand->m_objAux->m_actKey)), "R") == 0);
                }
                if (!eq && cand != unit && cand->m_defenderState != AISTATE_RETURN
                    && cand->m_defenderState != AISTATE_RETREAT) {
                    CGameObject* ul = unit->m_object;
                    CGameObject* cl = cand->m_object;
                    i32 dx = (ul->m_screenX >> TILE_SHIFT_PX) - (cl->m_screenX >> TILE_SHIFT_PX);
                    i32 dy = (ul->m_screenY >> TILE_SHIFT_PX) - (cl->m_screenY >> TILE_SHIFT_PX);
                    dx = abs(dx);
                    dy = abs(dy);
                    if (dx * dx + dy * dy <= 0x190) {

                        i32 flags = BATTLEZ_ROUTE_OTHER_TOOLS_TRIGGER;
                        PickupType cer = cand->m_entranceReason;
                        if (ArrivalPickupOf(cand, cer) == PICKUP_WINGZ) {
                            flags = BATTLEZ_ROUTE_OTHER_TOOLS_TRIGGER_WINGZ;
                        }
                        if (ArrivalPickupOf(cand, cer) == PICKUP_TOOB) {
                            flags |= BATTLEZ_ROUTE_TOOB_TRAVERSAL;
                        }
                        CPtrList list(10);
                        Coord oc;
                        (static_cast<CUserLogic*>(unit))->GetScreenPos((&oc));
                        CGameObject* dl = cand->m_object;
                        if ((m_board)->SearchEdge(
                                oc.m_x >> TILE_SHIFT_PX,
                                oc.m_y >> TILE_SHIFT_PX,
                                dl->m_screenX >> TILE_SHIFT_PX,
                                dl->m_screenY >> TILE_SHIFT_PX,
                                &list,
                                1,
                                0x98b,
                                flags
                            )
                            != 0) {
                            if (list.GetHeadPosition() != NULL) {
                                Coord* head = static_cast<Coord*>(list.RemoveHead());
                                if (head != NULL) {
                                    CoordPoolNode* node = g_coordPool.NodeOf(head);
                                    node->m_next = g_coordPool.m_freeHead;
                                    g_coordPool.m_freeHead = node;
                                }
                            }
                            if (list.GetHeadPosition() != NULL) {
                                if (cand->CoordCount() != 0) {
                                    CoordNode* cn = cand->CoordHead();
                                    while (cn != NULL) {
                                        CoordNode* cur = cn;
                                        cn = cn->m_next;
                                        if (cur->m_coord != NULL) {
                                            g_coordPool.Push(cur->m_coord);
                                        }
                                    }
                                    cand->m_coordList.RemoveAll();
                                }
                                if (unit->CoordCount() != 0) {
                                    CoordNode* nn = unit->CoordHead();
                                    while (nn != NULL) {
                                        CoordNode* cur = nn;
                                        nn = nn->m_next;
                                        if (cur->m_coord != NULL) {
                                            g_coordPool.Push(cur->m_coord);
                                        }
                                    }
                                    unit->m_coordList.RemoveAll();
                                }
                                POSITION pp = list.GetHeadPosition();
                                while (pp != NULL) {
                                    unit->m_coordList.AddTail(list.GetNext(pp));
                                }
                                cand->m_defenderState = AISTATE_SEEK;
                                unit->m_defenderState = AISTATE_RETREAT;
                            }
                            return 1;
                        }
                        return 0;
                    }
                }
            }
        }
        r = (r + 1) % 15;
    }
    return 0;
}

// @early-stop
RVA(0x0002f620, 0x871)
i32 CBattlezMapConfig::ChooseIdleBehavior(CGrunt* unit) {
    if (unit->m_entranceCommitted == 0) {
        return 0;
    }
    if (unit->m_deathAnimStarted != 0) {
        return 0;
    }
    if (unit->m_entranceActive != 0) {
        return 0;
    }
    if (unit->m_poweredUp != 0) {
        return 0;
    }

    bool eq;
    eq = (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_actKey)), "I") == 0);
    if (eq) {
        return 0;
    }

    CString* recs;
    CString* slot;
    i32 cnt;

    recs = g_typeColl.ScratchResolve(unit->m_objAux->m_actKey);
    slot = g_typeColl.Slots();
    cnt = g_typeColl.m_grown;
    while (cnt-- != 0) {
        if (slot != NULL) {
            slot->CString::CString();
        }
        slot++;
    }
    eq = (strcmp(*recs, "G") == 0);
    if (eq) {
        return 0;
    }

    recs = g_typeColl.ScratchResolve(unit->m_objAux->m_actKey);
    slot = g_typeColl.Slots();
    cnt = g_typeColl.m_grown;
    while (cnt-- != 0) {
        if (slot != NULL) {
            slot->CString::CString();
        }
        slot++;
    }
    eq = (strcmp(*recs, "L") == 0);
    if (eq) {
        return 0;
    }

    recs = g_typeColl.ScratchResolve(unit->m_objAux->m_actKey);
    slot = g_typeColl.Slots();
    cnt = g_typeColl.m_grown;
    while (cnt-- != 0) {
        if (slot != NULL) {
            slot->CString::CString();
        }
        slot++;
    }
    eq = (strcmp(*recs, "P") == 0);
    if (eq) {
        return 0;
    }

    recs = g_typeColl.ScratchResolve(unit->m_objAux->m_actKey);
    slot = g_typeColl.Slots();
    cnt = g_typeColl.m_grown;
    while (cnt-- != 0) {
        if (slot != NULL) {
            slot->CString::CString();
        }
        slot++;
    }
    eq = (strcmp(*recs, "J") == 0);
    if (eq) {
        return 0;
    }

    recs = g_typeColl.ScratchResolve(unit->m_objAux->m_actKey);
    slot = g_typeColl.Slots();
    cnt = g_typeColl.m_grown;
    while (cnt-- != 0) {
        if (slot != NULL) {
            slot->CString::CString();
        }
        slot++;
    }
    eq = (strcmp(*recs, "C") == 0);
    if (eq) {
        return 0;
    }

    recs = g_typeColl.ScratchResolve(unit->m_objAux->m_actKey);
    slot = g_typeColl.Slots();
    cnt = g_typeColl.m_grown;
    while (cnt-- != 0) {
        if (slot != NULL) {
            slot->CString::CString();
        }
        slot++;
    }
    eq = (strcmp(*recs, "R") == 0);
    if (eq) {
        return 0;
    }

    i32 bandPct = m_brickzPct;
    i32 band;
    if (bandPct == 0) {
        band = static_cast<i8>(rand());
        band &= 1;
    } else {
        band = rand() % bandPct;
    }
    band++;
    if (band <= m_toolzPct) {

        PickupType cur = unit->m_entranceReason;
        if (cur > PICKUP_EQUIPPABLE_LAST) {
            cur = unit->m_toolId;
        }
        if (cur != PICKUP_NONE) {
            return 1;
        }
        i32 rollPct = m_wingzPct;
        i32 roll;
        if (rollPct == 0) {
            roll = static_cast<i8>(rand());
            roll &= 1;
        } else {
            roll = rand() % rollPct;
        }
        roll++;
        // Every arm yields the PickupType id of the key its bucket accumulates;
        // 0x14 is not a droppable tool, so the chain skips straight to Wingz.
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
        // 0x14 has no Battlez bute key of its own; retail folds it onto Gauntletz.
        if (mode == PICKUP_WARPSTONE) {
            mode = PICKUP_GAUNTLETZ;
        }
        if (mode == PICKUP_BRICK) {

            CGrunt** row = &m_triggerMgr->m_grid[m_ownerId * 15];
            i32 nIdle = 0;
            for (i32 s = 15; s != 0; s--) {
                CGrunt* u = *row;
                if (u != NULL && u->m_battleState == BZTASK_CARRY_BRICK) {
                    nIdle++;
                }
                row++;
            }
            if (nIdle >= 2) {
                return 1;
            }
            for (i32 b = 0; b < 15; b++) {
                CGrunt* u = m_triggerMgr->m_grid[m_ownerId * 15 + b];
                if (u == NULL) {
                    continue;
                }
                if (u->m_battleState != BZTASK_UNASSIGNED) {
                    continue;
                }
                if (u->m_poweredUp != 0) {
                    continue;
                }
                (static_cast<CGrunt*>(u))->LoadPickupSprites(PICKUP_BRICK, 1, 0, 0, 1);
                u->m_battleState = BZTASK_CARRY_BRICK;
                if (u->CoordCount() != 0) {
                    CoordNode* n = u->CoordHead();
                    while (n != NULL) {
                        CoordNode* curn = n;
                        n = n->m_next;
                        if (curn->m_coord != NULL) {
                            CoordPoolNode* node = g_coordPool.NodeOf(curn->m_coord);
                            node->m_next = g_coordPool.m_freeHead;
                            g_coordPool.m_freeHead = node;
                        }
                    }
                    u->m_coordList.RemoveAll();
                }
            }
            return 1;
        }

        PickupType cur2 = unit->m_entranceReason;
        if (cur2 > PICKUP_EQUIPPABLE_LAST) {
            cur2 = unit->m_toolId;
        }
        if (cur2 == PICKUP_NONE) {
            (static_cast<CGrunt*>(unit))->LoadPickupSprites(mode, 1, 0, 0, 1);
            return 1;
        }
        // Retail 0x2f9xx: `cmp eax,0x12 / je <sunk>` sends the TOOB copy AWAY and
        // lets the WINGZ copy own the fall-through, which is the else-arm order.
        if (mode != PICKUP_TOOB) {
            if (mode == PICKUP_WINGZ) {
                if (unit->CoordCount() != 0) {
                    CoordNode* n = unit->CoordHead();
                    while (n != NULL) {
                        CoordNode* curn = n;
                        n = n->m_next;
                        if (curn->m_coord != NULL) {
                            CoordPoolNode* node = g_coordPool.NodeOf(curn->m_coord);
                            node->m_next = g_coordPool.m_freeHead;
                            g_coordPool.m_freeHead = node;
                        }
                    }
                    unit->m_coordList.RemoveAll();
                }
            }
        } else {
            if (unit->CoordCount() != 0) {
                CoordNode* n = unit->CoordHead();
                while (n != NULL) {
                    CoordNode* curn = n;
                    n = n->m_next;
                    if (curn->m_coord != NULL) {
                        CoordPoolNode* node = g_coordPool.NodeOf(curn->m_coord);
                        node->m_next = g_coordPool.m_freeHead;
                        g_coordPool.m_freeHead = node;
                    }
                }
                unit->m_coordList.RemoveAll();
            }
        }
    }

    if (band <= m_toyzPct) {

        i32 rollPct = m_yoyozPct;
        i32 roll;
        if (rollPct == 0) {
            roll = static_cast<i8>(rand());
            roll &= 1;
        } else {
            roll = rand() % rollPct;
        }
        roll++;
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
        (static_cast<CGrunt*>(unit))->LoadPickupSprites(mode, 1, 0, 0, 1);
        return 1;
    } else {

        i32 rollPct = m_blackBrickPct;
        i32 roll;
        if (rollPct == 0) {
            roll = static_cast<i8>(rand());
            roll &= 1;
        } else {
            roll = rand() % rollPct;
        }
        roll++;
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
            unit->m_entrancePickup = PICKUP_INVALID;
        }
        return 1;
    }
}

// @early-stop
RVA(0x000300c0, 0x190)
i32 CBattlezMapConfig::RouteUnitTo(
    CGrunt* unit,
    i32 gx,
    i32 gy,
    i32 maskA,
    i32 maskC,
    i32 clearFlag
) {
    CPtrList list(10);
    CGameObject* lvl = unit->m_object;
    if ((lvl->m_screenX >> TILE_SHIFT_PX) != gx || (lvl->m_screenY >> TILE_SHIFT_PX) != gy) {
        if ((m_board)->SearchEdge(
                lvl->m_screenX >> TILE_SHIFT_PX,
                lvl->m_screenY >> TILE_SHIFT_PX,
                gx,
                gy,
                &list,
                clearFlag,
                maskA,
                maskC
            )
            != 0) {
            if (list.GetCount() != 0) {
                Coord* head = static_cast<Coord*>(list.RemoveHead());
                if (head != NULL) {
                    CoordPoolNode* node = g_coordPool.NodeOf(head);
                    node->m_next = g_coordPool.m_freeHead;
                    g_coordPool.m_freeHead = node;
                }
                if (list.GetCount() != 0) {
                    if (unit->CoordCount() != 0) {
                        CoordNode* n = unit->CoordHead();
                        while (n != NULL) {
                            CoordNode* cur = n;
                            n = n->m_next;
                            if (cur->m_coord != NULL) {
                                g_coordPool.Push(cur->m_coord);
                            }
                        }
                        unit->m_coordList.RemoveAll();
                    }

                    POSITION pp = list.GetHeadPosition();
                    while (pp != NULL) {
                        Coord* cur = static_cast<Coord*>(list.GetNext(pp));
                        if (cur != NULL) {
                            unit->m_coordList.AddTail(cur);
                        }
                    }
                    list.RemoveAll();
                    Coord* tail = (unit->CoordTail())->m_coord;
                    i32 tailX = tail->m_x;
                    i32 tailY = tail->m_y;
                    unit->m_entrancePx.m_x = (tailX << TILE_SHIFT_PX) + TILE_HALF_PX;
                    unit->m_entrancePx.m_y = (tailY << TILE_SHIFT_PX) + TILE_HALF_PX;
                    return 1;
                }
            }
        }
    }
    return 0;
}

// @early-stop
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000302c0, 0x1ec)
i32 CBattlezMapConfig::RouteUnitToGoal(CGrunt* unit, Coord goal, i32 maskA, i32 maskC) {
    CPtrList list(10);
    Coord cur;
    CoordNode* n;
    CoordNode* p;
    Coord* head;
    CoordPoolNode* node;
    POSITION qp;

    (static_cast<CUserLogic*>(unit))->GetScreenPos((&cur));
    i32 gx = goal.m_x;
    i32 gy = goal.m_y;
    if ((cur.m_x >> TILE_SHIFT_PX) == gx) {
        (static_cast<CUserLogic*>(unit))->GetScreenPos((&goal));
        if ((goal.m_y >> TILE_SHIFT_PX) == gy) {
            goto fail;
        }
    }

    n = unit->CoordHead();
    while (n != NULL) {
        CoordNode* cur3 = n;
        n = n->m_next;
        Coord* coord = cur3->m_coord;
        if (coord != NULL && coord->m_x == gx && coord->m_y == gy) {
            break;
        }
    }

    if ((m_board)->SearchEdge(
            unit->m_object->m_screenX >> TILE_SHIFT_PX,
            unit->m_object->m_screenY >> TILE_SHIFT_PX,
            gx,
            gy,
            &list,
            0,
            maskA,
            maskC
        )
        == 0) {
        goto fail;
    }
    if (list.GetCount() == 0) {
        goto fail;
    }
    head = static_cast<Coord*>(list.RemoveHead());
    if (head != NULL) {
        node = g_coordPool.NodeOf(head);
        node->m_next = g_coordPool.m_freeHead;
        g_coordPool.m_freeHead = node;
    }
    if (list.GetCount() != 0) {
        if (n != NULL) {
            CoordNode* h = unit->CoordHead();
            if (h != NULL) {
                // Retail 0x303e0-0x303fa: the payload pointer is hoisted OUT of
                // this loop (`lea ecx,[unit+0x31c]`) and nothing in the body
                // advances h or n, so the `jne 0x303e6` back-edge only falls
                // through when the earlier search broke at the list HEAD.  Its
                // `test ecx,ecx` on a member address is also unfolded.  Both are
                // retail's, transcribed as-is.
                do {
                    CGruntCoordList* listPayload = &unit->m_coordList;
                    if (listPayload != NULL) {
                        node = g_coordPool.NodeOf(listPayload);
                        node->m_next = g_coordPool.m_freeHead;
                        g_coordPool.m_freeHead = node;
                    }
                } while (h != n);
            }
        }

        if (unit->CoordCount() != 0) {
            p = unit->CoordHead();
            while (p != NULL) {
                CoordNode* cur4 = p;
                p = p->m_next;
                if (cur4->m_coord != NULL) {
                    node = g_coordPool.NodeOf(cur4->m_coord);
                    node->m_next = g_coordPool.m_freeHead;
                    g_coordPool.m_freeHead = node;
                }
            }
            unit->m_coordList.RemoveAll();
        }

        qp = list.GetHeadPosition();
        while (qp != NULL) {
            Coord* cur5 = static_cast<Coord*>(list.GetNext(qp));
            if (cur5 != NULL) {
                unit->m_coordList.AddTail(cur5);
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
    if (unit->CoordCount() == 0) {
        return 0;
    }
    CoordNode* node = unit->CoordHead();
    if (node == NULL) {
        return 0;
    }
    BrickzCell** rows = ((m_board)->m_rows);
    while (node != NULL) {
        CoordNode* cur = node;
        node = node->m_next;
        Coord* c = cur->m_coord;
        i32 y = c->m_y;
        i32 x = c->m_x;
        if (rows[y][x].m_flags & 4) {
            return 1;
        }
    }
    return 0;
}

// @early-stop
// Register renaming only (edi/ebp and esi/edx swapped through the scan loop).
RVA(0x000305b0, 0x121)
i32 CBattlezMapConfig::IsCoordOccupied(CGrunt* selfUnit, i32 qx, i32 qy) {
    i32 i = 0;
    CGrunt** units = m_triggerMgr->m_grid + m_ownerId * 15;
    for (;;) {
        CGrunt* unit = *units;
        if (unit != NULL && unit != selfUnit && unit->m_battleState != BZTASK_SEEK_SWITCH) {

            if (unit->CoordCount() != 0) {
                CoordNode* node = unit->CoordHead();
                if (node != NULL) {
                    CMapMgr* board = m_board;
                    for (;;) {
                        CoordNode* cur = node;
                        node = node->m_next;
                        Coord* c = cur->m_coord;
                        i32 x = c->m_x;
                        i32 y = c->m_y;
                        i32 tile;
                        if (static_cast<u32>(x) < static_cast<u32>(board->m_width)
                            && static_cast<u32>(y) < static_cast<u32>(board->m_height)) {
                            tile = board->m_rowInts[y][x * 7];
                        } else {
                            tile = 1;
                        }
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
            CGameObject* lvl = unit->m_object;
            i32 currentX = lvl->m_screenX >> TILE_SHIFT_PX;
            i32 currentY = lvl->m_screenY >> TILE_SHIFT_PX;
            if (currentX == qx && currentY == qy) {
                return 1;
            }
        }
        i++;
        units++;
        if (i >= 15) {
            break;
        }
    }
    return 0;
}

// @early-stop
RVA(0x00030730, 0x1da)
i32 CBattlezMapConfig::ClaimCellFromRow(i32 cellX, i32 cellY, i32, i32) {
    if (m_active == 0) {
        return 0;
    }
    if (cellX == m_ownerId) {
        return 1;
    }
    CGrunt* src = m_triggerMgr->m_grid[cellX * 15 + cellY];
    if (src == NULL) {
        return 0;
    }
    if (src->m_gruntKind == GRUNT_GHOST) {
        return 0;
    }
    if (src->m_battleState == BZTASK_ADVANCE) {
        // Retail loads BOTH fields and spills m_y to a slot it never reads - the
        // extra 4 bytes of frame (0xc vs our 0x8).  A struct copy is what keeps
        // the second load alive.
        Coord sc = src->m_arrivalCell;
        if (sc.m_x != m_ownerId) {
            return 0;
        }
    }
    for (i32 i = 0; i < 15; i++) {
        CGrunt* u = m_triggerMgr->m_grid[m_ownerId * 15 + i];
        if (u == NULL) {
            continue;
        }
        i32 ok = 1;
        if (u->m_battleState == BZTASK_ASSIGNED_TARGET) {
            i32 ux = u->m_arrivalCell.m_x;
            i32 uy = u->m_arrivalCell.m_y;
            if (ux == cellX && uy == cellY) {
                ok = 0;
            }
        }
        if (u->m_battleState == BZTASK_ASSIGNED_TARGET) {
            i32 ux = u->m_arrivalCell.m_x;
            i32 uy = u->m_arrivalCell.m_y;
            if (!(ux == cellX && uy == cellY) && (rand() % 3) != 0) {
                ok = 0;
            }
        }
        if (ok == 0) {
            continue;
        }
        CGameObject* lvl = u->m_object;
        i32 lx = lvl->m_screenX >> TILE_SHIFT_PX;
        i32 ly = lvl->m_screenY >> TILE_SHIFT_PX;
        if (u->m_battleState == BZTASK_ADVANCE && u->m_targetTeam != -1) {
            CBattlezMapConfig* bundle = &m_ctx->m_options[u->m_targetTeam].m_battlezConfig;
            i32 dx = bundle->m_marker.m_x - lx;
            i32 dy = bundle->m_marker.m_y - ly;
            dx = abs(dx);
            dy = abs(dy);

            if (dx * dx + dy * dy <= 0x19) {
                ok = 0;
            }
        }
        if (ok == 0) {
            continue;
        }
        u->m_arrivalCell.m_x = cellX;
        u->m_battleState = BZTASK_ASSIGNED_TARGET;
        u->m_arrivalCell.m_y = cellY;
        u->m_defenderState = AISTATE_ATTACK;
        u->m_routeMaskA = 0xd87;
        u->m_routeMaskC = 0;
    }
    return 1;
}

// @early-stop
// ebx/ebp are transposed: retail parks the shared 0 in ebx and `occupied` in
// ebp, we do the reverse, and every store that uses either follows. Declaration
// order of `occupied`/`row` does not move it. The argument order to PlaceObject
// and the store order are both retail's now.
RVA(0x00030990, 0x11b)
i32 CBattlezMapConfig::TrySeedSpawnAt(i32 ax, i32 ay) {
    i32 occupied = 0;
    CGrunt** row = &m_triggerMgr->m_grid[m_ownerId * 15];
    for (i32 c = 15; c != 0; c--) {
        if (*row != NULL) {
            occupied++;
        }
        row++;
    }
    if (occupied >= m_ctx->m_options[m_ownerId].m_comboSel) {
        return 0;
    }
    i32 cell = m_triggerMgr->PlaceObject(
        m_ownerId,
        (ax << TILE_SHIFT_PX) + TILE_HALF_PX,
        (ay << TILE_SHIFT_PX) + TILE_HALF_PX,
        0x186a0,
        GRUNT_ENTRANCE_RESURRECT,
        IDX(m_ctx->m_options[m_ownerId].m_colorIndex),
        0,
        0,
        0x11,
        0,
        0,
        0,
        0
    );
    if (cell == -1) {
        return 0;
    }
    CGrunt* unit = m_ctx->m_cmdGrid->m_grid[cell + m_ownerId * TM_GRID_COLS];
    if (unit == NULL) {
        return 0;
    }
    unit->m_arrivalCell.m_x = -1;
    unit->m_unusedBattleCell.m_x = -1;
    unit->m_defenderPx.m_x = -1;
    unit->m_arrivalState = AI_BATTLEZ_PATH;
    unit->m_arrivalCell.m_y = -1;
    unit->m_targetTeam = -1;
    unit->m_unusedBattleCell.m_y = -1;
    unit->m_defenderState = AISTATE_SEEK;
    unit->m_defenderPx.m_y = -1;
    unit->m_defenderPickupType = PICKUP_NONE;
    unit->m_defenderQueuePosition = 0;
    unit->m_dwell = 0;
    unit->m_blockedVoicePending = 1;
    unit->m_battleState = BZTASK_ADVANCE;
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

// @early-stop
RVA(0x00030b20, 0x328)
i32 CBattlezMapConfig::PathToNearestGoal(CGrunt* unit, i32 col, i32 row) {
    CGameObject* lvl = unit->m_object;
    i32 goalX = lvl->m_screenX >> TILE_SHIFT_PX;
    i32 goalY = lvl->m_screenY >> TILE_SHIFT_PX;

    BrickzCell* tile = &(static_cast<BrickzCell*>((m_board)->m_rows[row]))[col];

    i32 bestX = col;
    i32 bestY = row;
    i32 bestDist = INT_MAX;

    CTileTriggerLogic* cell;

    if (tile->m_typeCode == TILEKIND_PYRAMID_LATCH_A) {
        cell = m_cellQuery->m_latchedLeaf;
    } else {
        cell = m_cellQuery->FindInLists12((col << 8) + row, TRIGID_ANY);
    }
    if (cell != NULL) {

        i32* p;
        for (p = cell->m_linkKeys; p - cell->m_linkKeys < 24; p++) {
            i32 node = *p;
            if (node == 0) {
                break;
            }
            CTileTriggerSwitchLogic* rec = m_cellQuery->FindChild(node, TRIGID_ANY);
            if (rec != NULL) {
                i32 cx = rec->m_tileX;
                i32 cy = rec->m_tileY;
                if (IsCoordOccupied(unit, cx, cy) != 0) {
                    return 1;
                }
            }
        }

        for (p = cell->m_linkKeys; p - cell->m_linkKeys < 24; p++) {
            i32 node = *p;
            if (node == 0) {
                break;
            }
            CTileTriggerSwitchLogic* rec = m_cellQuery->FindChild(node, TRIGID_ANY);
            if (rec != NULL) {
                i32 cx = rec->m_tileX;
                i32 cy = rec->m_tileY;
                i32 dx = cx - goalX;
                i32 dy = cy - goalY;
                dx = abs(dx);
                dy = abs(dy);
                i32 dist = dx * dx + dy * dy;
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
    PickupType er = unit->m_entranceReason;
    if (ArrivalPickupOf(unit, er) == PICKUP_WINGZ) {
        flags = BATTLEZ_ROUTE_ALL_TOOLS_WINGZ;
    }
    if (ArrivalPickupOf(unit, er) == PICKUP_TOOB) {
        flags |= BATTLEZ_ROUTE_TOOB_TRAVERSAL;
    }
    CGameObject* lvl2 = unit->m_object;
    if ((m_board)->SearchEdge(
            lvl2->m_screenX >> TILE_SHIFT_PX,
            lvl2->m_screenY >> TILE_SHIFT_PX,
            bestX,
            bestY,
            &list,
            1,
            0x98f,
            flags
        )
        != 0) {
        if (list.GetCount() != 0) {
            Coord* head = static_cast<Coord*>(list.RemoveHead());
            if (head != NULL) {
                CoordPoolNode* node = g_coordPool.NodeOf(head);
                node->m_next = g_coordPool.m_freeHead;
                g_coordPool.m_freeHead = node;
            }
            if (list.GetCount() != 0) {

                if (unit->CoordCount() != 0) {
                    CoordNode* n = unit->CoordHead();
                    while (n != NULL) {
                        CoordNode* cur = n;
                        n = n->m_next;
                        if (cur->m_coord != NULL) {
                            CoordPoolNode* fn = g_coordPool.NodeOf(cur->m_coord);
                            fn->m_next = g_coordPool.m_freeHead;
                            g_coordPool.m_freeHead = fn;
                        }
                    }
                    unit->m_coordList.RemoveAll();
                }

                POSITION pp = list.GetHeadPosition();
                while (pp != NULL) {
                    unit->m_coordList.AddTail(list.GetNext(pp));
                }
                Coord* tail = (unit->CoordTail())->m_coord;
                unit->m_entrancePx.m_x = (tail->m_x << TILE_SHIFT_PX) + TILE_HALF_PX;
                unit->m_entrancePx.m_y = (tail->m_y << TILE_SHIFT_PX) + TILE_HALF_PX;
                unit->m_defenderState = AISTATE_RETREAT;
                return 1;
            }
        }
    } else {
        PathToNearestCandidate(unit, 1, bestX, bestY);
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
        o->m_x = sx;
        o->m_y = sy;
        return o;
    }
    CGameObject* lvl = unit->m_object;
    i32 rx = lvl->m_screenX >> TILE_SHIFT_PX;
    i32 ry = lvl->m_screenY >> TILE_SHIFT_PX;
    CPtrArray* coords = &m_ctx->m_options[kind].m_battlezConfig.m_attackWaypoints;
    i32 count = coords->GetSize();
    if (count != 0) {
        i32 r = rand() % count;
        for (i32 k = 0; k < count; k++) {
            Coord** arr = MfcPtrArrayData<Coord>(*coords);
            CTriggerMgr* grid = m_triggerMgr;
            i32 cell = m_ownerId;
            Coord cand = *arr[r];
            i32 ok = 1;
            for (i32 j = 0; j < 15; j++) {
                CGrunt* u = grid->m_grid[cell * 15 + j];
                if (u != NULL && u->CoordCount() != 0) {
                    Coord node = *(u->CoordTail()->m_coord);
                    if (node.m_x == cand.m_x && node.m_y == cand.m_y) {
                        ok = 0;
                    }
                }
            }
            if (ok != 0) {
                *o = cand;
                return o;
            }
            r = (r + 1) % count;
        }
        r = rand() % count;
        Coord* cand = MfcPtrArrayData<Coord>(*coords)[r];
        rx = cand->m_x;
        ry = cand->m_y;
    }
    o->m_x = rx;
    o->m_y = ry;
    return o;
}

RVA(0x000310f0, 0x8d)
char* _zdvec::IndexToPtr(i32 i) {
    char* r;
    m_grown = 0;
    if (i >= m_lo && i <= m_hi) {
        r = m_base + (i - m_lo) * m_stride;
    } else if (GrowTo(i, 0)) {
        r = m_base + (i - m_lo) * m_stride;
    } else {
        char* msg = g_errOutOfMem;
        g_retAddrBreadcrumb = GetRetAddr();
        m_errSink->Set(this, msg, 0xc);
        r = m_spare;
    }
    char* slot = m_alloc;
    i32 n = m_grown;
    while (n-- != 0) {
        if (slot) {
            new (slot) CString();
        }
        slot += 4;
    }
    return r;
}

RVA(0x000311b0, 0x14)
void FreeNodePool::Push(void* p) {
    CoordPoolNode* node = NodeOf(p);
    node->m_next = m_freeHead;
    m_freeHead = node;
}

RVA(0x000311e0, 0x4c)
void CDDrawWorkerHost::SnapToTileCenter(Coord* out, i32 x, i32 y) {
    Coord result;
    i32 sx = m_shiftX;
    i32 sy = m_shiftY;
    result.m_x = x >> sx;
    result.m_y = y >> sy;
    result.m_x <<= sx;
    result.m_y <<= sy;
    result.m_x += m_tilePxW / 2;
    result.m_y += m_tilePxH / 2;
    *out = result;
}

RVA(0x00031250, 0x33)
CGameObject* CDDrawChildGroup::Drain() {
    for (;;) {
        if (m_scanCursor == NULL) {
            return 0;
        }
        CGameObject* data = static_cast<CGameObject*>(m_list.GetNext(m_scanCursor));
        if (data->GetClassId() == CLASSID_SERIALREF) {
            return data;
        }
    }
}

RVA(0x000312a0, 0x74)
char* _zvec::IndexToPtr(i32 idx) {
    char* r;
    m_grown = 0;
    if (idx >= m_lo && idx <= m_hi) {
        r = m_base + (idx - m_lo) * m_stride;
    } else if (GrowTo(idx, 0)) {
        r = m_base + (idx - m_lo) * m_stride;
    } else {
        char* msg = g_errOutOfMem;
        g_retAddrBreadcrumb = GetRetAddr();
        m_errSink->Set(this, msg, 0xc);
        r = m_spare;
    }
    return r;
}
