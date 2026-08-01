#include <Gruntz/GameObjectFactory.h> // the real Create* registrant decls (ex thunk-shaped aliases)
#include <Gruntz/Play.h> // complete CPlay (the m_10 spawn-info holder is the play state)
#include <Gruntz/TileTriggerLogic.h>
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GruntzPlayer.h>
#include <Io/FileMem.h> // the serialize stream (CFileMemBase == the real CFileMemBase)
#include <Gruntz/TileTriggerSwitchLogic.h>
#include <Gruntz/TileActionEvent.h> // CTileActionEvent - FindByField0C's real return type
#include <Gruntz/UserLogic.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/TypeKeyColl.h> // g_typeColl (anim registry)
#include <Gruntz/Brickz.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Wwd/WwdFile.h>
#include <Gruntz/GameLevel.h> // canonical CGameLevel/CDDrawWorkerHost (m_world->m_level visible rect)
#include <rva.h>

#include <Gruntz/CoordNode.h>    // the shared coord-list node
#include <Gruntz/FreeNodePool.h> // canonical coord free-pool (g_coordPool)
#include <Gruntz/BattlezMapConfig.h>
#include <Gruntz/TriggerMgr.h>        // the ONE CTriggerMgr (the local dup class is gone)
#include <Gruntz/GruntPuddle.h>       // CGruntPuddle (the m_baseList spawn-candidate element)
#include <Gruntz/MapMgr.h>            // CMapMgr == CMapMgr (the board / tile grid)
#include <DDrawMgr/DDrawChildGroup.h> // the level's game-object collection (ex CQueueDrainHost view)
#include <Wap32/zBitVec.h>            // zErrHandling (the zvec error-report target)
#include <Gruntz/ActReg.h>
#include <Gruntz/LevelInfo.h> // the canonical CLevelInfo (LoadConfig arg1)
#include <Bute/ButeMgr.h>     // CButeMgr (LoadConfig reads the g_buteMgr singleton)
#include <Gruntz/GameRegistry.h>
#include <Gruntz/SerialArchive.h> // the shared CFileMemBase stream (Read @+0x2c / Write @+0x30)

#include <stdlib.h> // rand (0x11fee0, grid-scan neighbour pick); abs (branchless cdq/xor/sub)
#include <math.h>   // sqrt (CBattlezMapConfig::Step board-distance)
#pragma intrinsic(sqrt)
#include <string.h>     // strcmp (anim-name dispatch -> inline sbb/sbb byte compare)
#include <new>          // placement new (CRect in-place ctor)
#include <Wap32/Rect.h> // canonical CRect (0x29ac0 direct-store ctor, was local QuadIntRecord)
#include <Gruntz/TileTriggerContainer.h> // canonical CTileTriggerContainer (FindInLists12 0x116f20)

#include <Gruntz/FreeNodePool.h> // the coord-node pool object @0x645540

DATA(0x001e96ec)
const float g_diffScale = 0.01f; // 0x5e96ec

DATA(0x0020ccc0)
i32 g_spawnCfg = 0x98f; // .data (map-config seed 2447)
DATA(0x0022b6dc)
i32 g_stepRun;
DATA(0x0022b730)
i32 g_stepCol;
DATA(0x0022b734)
i32 g_stepRow;
DATA(0x0022b738)
i32 g_diffTier;
DATA(0x0022b7ec)
i32 g_spawnState;

// CPtrArray is MFC's UNTYPED void* array; naming the element type is the one cast its
// API leaves no way to avoid. One seam for this TU rather than a cast at every walk
// (and kept LOCAL - adding it to a shared header shifts every including TU's codegen).
static inline Coord** CoordArrayData(CPtrArray& a) {
    union {
        void** m_untyped;
        Coord** m_typed;
    } band;
    band.m_untyped = a.GetData();
    return band.m_typed;
}

static inline CGameObject* ListGetFirst(CDDrawChildGroup* list) {
    list->m_walkCursor = list->m_list.GetHeadPosition();
    if (list->m_walkCursor == 0) {
        return 0;
    }
    return static_cast<CGameObject*>(list->m_list.GetNext(list->m_walkCursor));
}

static inline CGameObject* ListGetNext(CDDrawChildGroup* list) {
    if (list->m_walkCursor == 0) {
        return 0;
    }
    return static_cast<CGameObject*>(list->m_list.GetNext(list->m_walkCursor));
}

// ===========================================================================
// CBattlezMapConfig::CBattlezMapConfig  @0x024dc0
// Member-constructs the four arrays (CPtrArray x2, CDWordArray x2) - the /GX
// compiler frames the ctor and advances the EH try-level after each constructed
// member - then seeds the scalar config block. Returns `this`.
// ===========================================================================
// @early-stop
RVA(0x00024dc0, 0x158)
CBattlezMapConfig::CBattlezMapConfig()
    : m_scratch78(0), m_scratch7c(0), m_scratch80(0), m_scratch84(0) {
    m_curCell = 0;
    m_01c = 1;
    m_020 = 0x40;
    m_024 = 0x40;
    m_028 = 0x40;
    m_08c = 5;
    m_090 = 5;
    m_02c = 0x32;
    m_094 = 8;
    m_098 = 8;
    m_0ac = 8;
    m_0b0 = 8;
    m_spawnPct = 0x32;
    m_reserveBudget = 0x3e8;
    m_moveBudget = 0x3e8;
    m_088 = 0x32;
    m_0a8 = 0x32;
    m_spawnInterval = 0;
    m_repickInterval = 0;
    m_spawnLastFire = 0;
    m_repickLastFire = 0;
    m_repickTimer = 0;
    m_spawnTimer = 0;
    m_repathBudget = 0xbb8;
    m_0cc = 0xbb8;
    m_13c = 0;
    m_140 = 0;
    m_09c = 0x7d0;
    m_0a0 = 0x7d0;
    m_0a4 = 6;
    m_0b8 = 0x7d0;
    m_0c0 = 0xa;
    m_0c8 = 0x7530;
    m_budgetMul = 0x19;
}

RVA(0x00024f80, 0x7d)
CBattlezMapConfig::~CBattlezMapConfig() {
    FreeArrays();
}

RVA(0x00025020, 0x984)
i32 CBattlezMapConfig::LoadConfig(CGruntzMgr* mgr, i32 id, i32 diff) {
    // --- prologue: zero the scratch fields, copy the level-info handles. ---
    m_gruntCreationTime = 0;
    m_4c = 0;
    m_50 = 0;
    m_resourceCreationTime = 0;
    m_58 = 0;
    m_5c = 0;
    m_levelInfo = mgr;
    m_ownerId = id;
    m_8 = mgr->m_cmdGrid;
    m_dims = mgr->m_tileGrid;
    m_10 = static_cast<CPlay*>(mgr->m_curState);
    m_14 = m_10->m_beginMarker; // the play state's begin-marker sink (CTileTriggerContainer)
    m_0 = 1;

    // --- the [Battlez] creation-rate / chance block. ---
    m_gruntCreationTime = g_buteMgr.GetDwordDef("Battlez", "GruntCreationTime", 10000);
    m_resourceCreationTime = g_buteMgr.GetDwordDef("Battlez", "ResourceCreationTime", 10000);
    m_gauntletzChance = g_buteMgr.GetDwordDef("Battlez", "GauntletzChance", 50);
    m_shovelzChance = g_buteMgr.GetDwordDef("Battlez", "ShovelzChance", 50);
    m_spyzChance = g_buteMgr.GetDwordDef("Battlez", "SpyzChance", 50);
    m_brickzChance = g_buteMgr.GetDwordDef("Battlez", "BrickzChance", 50);
    m_gooberzChance = g_buteMgr.GetDwordDef("Battlez", "GooberzChance", 50);
    m_gruntRatio = g_buteMgr.GetDwordDef("Battlez", "GruntRatio", 25);
    m_defenderChance = g_buteMgr.GetDwordDef("Battlez", "DefenderChance", 50);

    // --- loop 1: append EVERY type-1 start marker to the +0xdc array. The marker
    //     coords are scaled by signed /32 (round-toward-zero) into a freelist pair.
    //     The list is re-derived (mgr->m_world->m_childGroup) and advanced via the GetNext
    //     cursor idiom on every step. ---
    for (CGameObject* cur = ListGetFirst(mgr->m_world->m_childGroup); cur != 0;
         cur = ListGetNext(mgr->m_world->m_childGroup)) {
        if (cur->m_7c->m_notify == &CreateGruntCreationPoint && cur->m_124 == id) {
            CoordPoolNode* p = static_cast<CoordPoolNode*>(g_coordPool.m_freeHead);
            i32* slot = 0;
            if (p->m_next != 0) {
                slot = &p->m_coord.m_x;
                g_coordPool.m_freeHead = p->m_next;
            }
            slot[0] = cur->m_screenX / 32;
            slot[1] = cur->m_screenY / 32;
            SetAtGrow(m_candArray.GetSize(), slot);
        }
    }

    // --- loop 2: find the FIRST type-2 marker, stamp m_markerX/m_markerY with its /32 coords,
    //     and stop (fall straight into loop 3). ---
    for (CGameObject* cur2 = ListGetFirst(mgr->m_world->m_childGroup); cur2 != 0;
         cur2 = ListGetNext(mgr->m_world->m_childGroup)) {
        if (cur2->m_7c->m_notify == &CreateExitTrigger && cur2->m_124 == id) {
            m_markerX = cur2->m_screenX / 32;
            m_markerY = cur2->m_screenY / 32;
            break;
        }
    }

    // --- loop 3: append EVERY type-3 marker to the +0xf0 array, scaled by >>5
    //     (arithmetic floor), and set bit 0x10000 in the matched object's flags. ---
    for (CGameObject* cur3 = ListGetFirst(mgr->m_world->m_childGroup); cur3 != 0;
         cur3 = ListGetNext(mgr->m_world->m_childGroup)) {
        if (cur3->m_7c->m_notify == &CreateWayPoint && cur3->m_124 == id) {
            CoordPoolNode* p = static_cast<CoordPoolNode*>(g_coordPool.m_freeHead);
            i32* slot = 0;
            if (p->m_next != 0) {
                slot = &p->m_coord.m_x;
                g_coordPool.m_freeHead = p->m_next;
            }
            slot[0] = cur3->m_screenX >> 5;
            slot[1] = cur3->m_screenY >> 5;
            SetAtGrow(m_0f0.GetSize(), slot);
            cur3->m_flags |= 0x10000;
        }
    }

    // --- per-difficulty rescale of the two creation-time fields. ---
    switch (diff) {
        case 0: { // Easy
            g_buteMgr.GetIntDef("Battlez", "EasyDifficulty", 100);
            g_diffTier = 20;
            break;
        }
        case 1: { // Normal
            i32 r = g_buteMgr.GetIntDef("Battlez", "NormalDifficulty", 50);
            g_diffTier = 10;
            m_gruntCreationTime = static_cast<i32>(
                (static_cast<double>(r)
                 * (static_cast<double>(static_cast<i64>(m_gruntCreationTime)) * g_diffScale))
            );
            m_resourceCreationTime = static_cast<i32>(
                (static_cast<double>(r)
                 * (static_cast<double>(static_cast<i64>(m_resourceCreationTime)) * g_diffScale))
            );
            break;
        }
        case 2: { // Hard
            i32 r = g_buteMgr.GetIntDef("Battlez", "HardDifficulty", 25);
            g_diffTier = 5;
            m_gruntCreationTime = static_cast<i32>(
                (static_cast<double>(r)
                 * (static_cast<double>(static_cast<i64>(m_gruntCreationTime)) * g_diffScale))
            );
            m_resourceCreationTime = static_cast<i32>(
                (static_cast<double>(r)
                 * (static_cast<double>(static_cast<i64>(m_resourceCreationTime)) * g_diffScale))
            );
            break;
        }
        default:
            break;
    }

    // --- mid-block scalar seeds. ---
    m_50 = 0;
    m_14c = 0;
    {
        i32 rv = rand();
        m_144 = ((rv % 4) + 5) * 125 * 8;
    }
    m_148 = 0;
    m_8c = 6;
    m_90 = 6;
    m_94 = 6;
    m_98 = 6;
    m_a4 = 8;
    m_ac = m_dims->m_width / 3;
    m_b0 = m_dims->m_width / 3;
    m_c0 = m_dims->m_width >> 2;
    m_140 = 0;

    // --- the per-item spawn-budget running totals (each = prev_total + GetInt). ---
    m_toolzPct = g_buteMgr.GetInt("Battlez", "ToolzPercent");
    m_toyzPct = m_toolzPct + g_buteMgr.GetInt("Battlez", "ToyzPercent");
    m_brickzPct = m_toyzPct + g_buteMgr.GetInt("Battlez", "BrickzPercent");
    m_redBrickPct = g_buteMgr.GetInt("Battlez", "RedBrick");
    m_blueBrickPct = m_redBrickPct + g_buteMgr.GetInt("Battlez", "BlueBrick");
    m_goldBrickPct = g_buteMgr.GetInt("Battlez", "GoldBrick");
    m_blackBrickPct = m_goldBrickPct + g_buteMgr.GetInt("Battlez", "BlackBrick");
    m_babyWalkerzPct = g_buteMgr.GetInt("Battlez", "BabyWalkerz");
    m_beachBallzPct = m_babyWalkerzPct + g_buteMgr.GetInt("Battlez", "BeachBallz");
    m_bigWheelzPct = g_buteMgr.GetInt("Battlez", "BigWheelz");
    m_goKartzPct = m_bigWheelzPct + g_buteMgr.GetInt("Battlez", "GoKartz");
    m_jackInTheBoxzPct = g_buteMgr.GetInt("Battlez", "JackInTheBoxz");
    m_jumpRopezPct = m_jackInTheBoxzPct + g_buteMgr.GetInt("Battlez", "JumpRopez");
    m_pogoStickzPct = g_buteMgr.GetInt("Battlez", "PogoStickz");
    m_scrollzPct = m_pogoStickzPct + g_buteMgr.GetInt("Battlez", "Scrollz");
    m_squeakToyzPct = g_buteMgr.GetInt("Battlez", "SqueakToyz");
    m_yoyozPct = m_squeakToyzPct + g_buteMgr.GetInt("Battlez", "Yoyoz");
    m_bombzPct = g_buteMgr.GetInt("Battlez", "Bombz");
    m_boomerangzPct = m_bombzPct + g_buteMgr.GetInt("Battlez", "Boomerangz");
    g_buteMgr.GetInt("Battlez", "Brickz");
    m_clubzPct = m_boomerangzPct + g_buteMgr.GetInt("Battlez", "Clubz");
    m_gauntletzPct = g_buteMgr.GetInt("Battlez", "Gauntletz");
    m_glovezPct = m_gauntletzPct + g_buteMgr.GetInt("Battlez", "Glovez");
    m_gooberzPct = g_buteMgr.GetInt("Battlez", "Gooberz");
    m_gravityBootzPct = m_gooberzPct + g_buteMgr.GetInt("Battlez", "GravityBootz");
    m_gunHatzPct = g_buteMgr.GetInt("Battlez", "GunHatz");
    m_nerfGunzPct = m_gunHatzPct + g_buteMgr.GetInt("Battlez", "NerfGunz");
    m_rockzPct = g_buteMgr.GetInt("Battlez", "Rockz");
    m_shieldzPct = m_rockzPct + g_buteMgr.GetInt("Battlez", "Shieldz");
    m_shovelzPct = g_buteMgr.GetInt("Battlez", "Shovelz");
    m_springzPct = m_shovelzPct + g_buteMgr.GetInt("Battlez", "Springz");
    m_spyzPct = g_buteMgr.GetInt("Battlez", "Spyz");
    m_swordzPct = m_spyzPct + g_buteMgr.GetInt("Battlez", "Swordz");
    m_timeBombzPct = g_buteMgr.GetInt("Battlez", "TimeBombz");
    m_toobzPct = m_timeBombzPct + g_buteMgr.GetInt("Battlez", "Toobz");
    m_wandzPct = g_buteMgr.GetInt("Battlez", "Wandz");
    m_welderzPct = m_wandzPct + g_buteMgr.GetInt("Battlez", "Welderz");
    m_wingzPct = g_buteMgr.GetInt("Battlez", "Wingz");
    m_1e4 = m_wingzPct + g_buteMgr.GetInt("Battlez", "Wingz");

    // --- epilogue: clear the +0x78..+0x84 block, return 1. ---
    m_78 = 0;
    m_80 = 0;
    m_7c = 0;
    m_84 = 0;
    return 1;
}

RVA(0x00025c20, 0x55)
i32 CBattlezMapConfig::StepAllRowSpawns() {
    if (g_gameReg->m_options[m_curCell].m_014 == 0
        && g_gameReg->m_options[m_curCell].m_liveGate != 0) {
        for (i32 i = 0; i < m_candArray.GetSize(); i++) {
            this->StepRowSpawn(0); // @0x26470 (the per-element refresh sibling)
        }
    }
    return 1;
}

RVA(0x00025ca0, 0xbf)
void CBattlezMapConfig::FreeArrays() {
    i32 i;
    for (i = 0; i < m_candArray.GetSize(); i++) {
        void* p = m_candArray[i];
        if (p != 0) {
            CoordPoolNode* node = g_coordPool.NodeOf(p);
            node->m_next = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = node;
        }
    }
    m_candArray.SetSize(0, -1);

    for (i = 0; i < m_0f0.GetSize(); i++) {
        CoordPoolNode* node = g_coordPool.NodeOf(m_0f0[i]);
        node->m_next = g_coordPool.m_freeHead;
        g_coordPool.m_freeHead = node;
    }
    m_0f0.SetSize(0, -1);

    m_104.SetSize(0, -1);
    m_118.SetSize(0, -1);
    m_13c = 0;
}

// ===========================================================================
// CBattlezMapConfig::StepBoard  @0x025d90
// The per-tick board step. Run the two timers (claim/spawn budget via
// StepRowSpawn, and a periodic re-pick), level off mode-3 units' countdowns,
// then scan the current cell-row for the one eligible unit (passes the cached-
// cell + clear-flags guards and is NOT one of the I/G/L/P/J/C/R type codes) whose
// countdown reached 0, transition it (state 0/5 + SetState), and on a 0x12/0x16
// mode recycle its coord nodes onto g_coordPool.m_freeHead. Decrement every mode-3 unit's
// countdown and advance the bundle's timers by g_frameDelta. Returns 1.
// ===========================================================================
// @early-stop
RVA(0x00025d90, 0x580)
i32 CBattlezMapConfig::StepBoard() {
    if (m_active == 0) {
        return 1;
    }
    if (m_ctx->m_cmdGrid == 0) {
        return 0;
    }
    if (m_spawnTimer - m_spawnLastFire > m_spawnInterval) {
        StepRowSpawn(1);
        m_spawnLastFire = m_spawnTimer;
    }
    // Level off the mode-3 countdowns: find the minimum, subtract it from each.
    i32 mn = 0x10;
    CGrunt** row = &m_triggerMgr->m_grid[m_curCell * 15];
    for (i32 s = 15; s != 0; s--) {
        CGrunt* u = *row;
        if (u != 0 && u->m_defenderState == 3 && u->m_2e0 < mn) {
            mn = u->m_2e0;
        }
        row++;
    }
    if (mn != 0 && mn != 0x10) {
        for (i32 k = 0; k < 15; k++) {
            CGrunt* u = m_triggerMgr->m_grid[m_curCell * 15 + k];
            if (u != 0 && u->m_defenderState == 3) {
                u->m_2e0 -= mn;
            }
        }
    }
    // The periodic re-pick: every so often pick a random unit; if it is a
    // ready mode-3 it becomes the forced first candidate of the scan, otherwise
    // (2/3 chance) kick its idle behaviour.
    i32 forced = 0;
    CGrunt* forcedUnit = 0;
    if (m_repickTimer - m_repickLastFire > m_repickInterval) {
        i32 r = rand() % 15;
        CGrunt* u = m_triggerMgr->m_grid[m_curCell * 15 + r];
        forcedUnit = u;
        forced = 0;
        if (u != 0 && u->m_defenderState == 3 && u->m_2e0 == 0) {
            forced = 1;
        }
        if (!forced) {
            if (rand() % 10 != 0) {
                i32 r2 = rand() % 15;
                CGrunt* u2 = m_triggerMgr->m_grid[m_curCell * 15 + r2];
                if (u2 != 0) {
                    ChooseIdleBehavior(u2);
                }
            }
        }
        if (!forced) {
            m_repickLastFire = m_repickTimer;
        } else {
            // The eligibility scan: walk the row (the forced unit overrides slot 0).
            for (i32 b = 0; b < 15; b++) {
                CGrunt* unit = m_triggerMgr->m_grid[m_curCell * 15 + b];
                if (forced) {
                    unit = forcedUnit;
                }
                if (unit == 0) {
                    continue;
                }
                CGameObject* lvl = unit->m_object;
                if (lvl->m_screenX != unit->m_lastTilePxX) {
                    continue;
                }
                if (lvl->m_screenY != unit->m_lastTilePxY) {
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
                i32 eq;
                eq = (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_1c)), "I") == 0);
                if (eq) {
                    continue;
                }
                eq = (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_1c)), "G") == 0);
                if (eq) {
                    continue;
                }
                eq = (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_1c)), "L") == 0);
                if (eq) {
                    continue;
                }
                eq = (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_1c)), "P") == 0);
                if (eq) {
                    continue;
                }
                eq = (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_1c)), "J") == 0);
                if (eq) {
                    continue;
                }
                eq = (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_1c)), "C") == 0);
                if (eq) {
                    continue;
                }
                eq = (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_1c)), "R") == 0);
                if (eq) {
                    continue;
                }
                if (unit->m_defenderState != 3) {
                    continue;
                }
                if (unit->m_2e0 != 0) {
                    continue;
                }
                // Eligible: transition + (mode 0x12/0x16) recycle its coord nodes.
                i32 mode = unit->m_2e4;
                if (PathCrossesMarkedTile(unit) != 0) {
                    unit->m_defenderState = 5;
                } else {
                    unit->m_defenderState = 0;
                }
                (static_cast<CGrunt*>(unit))->LoadPickupSprites(unit->m_2e4, 0, 0, 1, 1);
                // retail 0x2623e: TWO separate guarded recycles (0x26283 for mode
                // 0x12, 0x26248 for 0x16), each `if (CoordCount()) { walk; RemoveAll; }`
                // - cl tail-merges only the shared RemoveAll at 0x262bc, so the source
                // really does spell the block twice.
                if (mode == 0x12) {
                    if (unit->CoordCount() != 0) {
                        CoordNode* n = unit->CoordHead();
                        while (n != 0) {
                            CoordNode* cur = n;
                            n = n->m_next;
                            if (cur->m_coord != 0) {
                                CoordPoolNode* node = g_coordPool.NodeOf(cur->m_coord);
                                node->m_next = g_coordPool.m_freeHead;
                                g_coordPool.m_freeHead = node;
                            }
                        }
                        unit->m_31c.RemoveAll();
                    }
                } else if (mode == 0x16) {
                    if (unit->CoordCount() != 0) {
                        CoordNode* n = unit->CoordHead();
                        while (n != 0) {
                            CoordNode* cur = n;
                            n = n->m_next;
                            if (cur->m_coord != 0) {
                                CoordPoolNode* node = g_coordPool.NodeOf(cur->m_coord);
                                node->m_next = g_coordPool.m_freeHead;
                                g_coordPool.m_freeHead = node;
                            }
                        }
                        unit->m_31c.RemoveAll();
                    }
                }
                break;
            }
            m_repickLastFire = m_repickTimer;
        }
    }
    StepRowUnits();
    m_spawnTimer += g_frameDelta;
    m_repickTimer += g_frameDelta;
    m_claimTimer += g_frameDelta;
    return 1;
}

// ===========================================================================
// CBattlezMapConfig::StepRowSpawn  @0x026470
// Spawn/claim decision for the current cell-row: if the row is already at/over
// its per-level unit budget (rec->m_378) return early; otherwise scan the first
// CPtrArray (m_candArray) of candidate coords, skip ones whose tile carries the
// 0x20000000 "reserved" bit (unless they map to this row), map the first usable
// candidate to a screen cell via WorldToScreen + ProbeCell, and if that cell
// holds a unit, seed it as a fresh spawn (mode 4 / state 0x11 / a -1 coord block)
// gated by a g_diffScale-scaled budget compare. Returns 1.
// ===========================================================================
// @early-stop
RVA(0x00026470, 0x29d)
i32 CBattlezMapConfig::StepRowSpawn(i32 allowReserved) {
    CGrunt** row = &m_triggerMgr->m_grid[m_curCell * 15];
    i32 occupied = 0;
    for (i32 c = 15; c != 0; c--) {
        if (*row != 0) {
            occupied++;
        }
        row++;
    }
    if (occupied >= m_ctx->m_options[m_curCell].m_comboSel) {
        return 1;
    }
    i32 n = m_candArray.GetSize();
    if (n <= 0) {
        return 1;
    }
    Coord** cands = CoordArrayData(m_candArray);
    Coord* cand = 0;
    i32 i = 0;
    BrickzCell tileRec;
    for (;;) {
        cand = cands[i];
        i32 usable = 1;
        if (cand != 0) {
            // retail @0x264fa computes the cell address on the m_rowInts arm - `x*7` then
            // `*4` (`shl edi,3; sub edi,eax; lea esi,[ecx+edi*4]`), not an x*0x1c stride -
            // and copies the whole 0x1c-byte cell with `mov ecx,7; rep movsd`.
            const i32* tilePtr = &m_board->m_rowInts[cand->m_y][cand->m_x * 7];
            memcpy(&tileRec, tilePtr, sizeof(tileRec));
            usable = 1;
            if (tileRec.m_0 & 0x20000000) {
                // retail 0x26517 `cmp eax,ecx / jne +2 / xor esi,esi`: usable is cleared
                // when the reserved cell's owner byte EQUALS m_curCell.
                if (tileRec.m_4Bytes[1] == m_curCell) {
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
        if (i >= m_candArray.GetSize()) {
            return 1;
        }
    }
    Coord screen;
    m_ctx->m_world->m_level->m_mainPlane->SnapToTileCenter(&screen, cand->m_x << 5, cand->m_y << 5);
    i32 cell;
    if (allowReserved != 0) {
        cell = m_ctx->m_cmdGrid->PlaceObject(
            m_curCell,
            screen.m_x,
            screen.m_y,
            0x186a0,
            2,
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
            m_curCell,
            screen.m_x,
            screen.m_y,
            0x186a0,
            0,
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
    // retail 0x26470: `lea ecx,[curCell+curCell*2]; lea edx,[cell+ecx*4]; add ecx,edx`
    // == cell + curCell*15, then `[grid+ecx*4+0x1c]` - the +0x1c board, not a +0 pun.
    CGrunt* unit = m_ctx->m_cmdGrid->m_grid[cell + m_curCell * TM_GRID_COLS];
    if (unit == 0) {
        return 0;
    }
    // retail parks this roll in the SAME stack slot as the (now dead) argument -
    // [esp+0x38] at 0x26620 - which is why the two used to be one conflated local.
    i32 roll = rand() % 100;
    i32 freeCount = 0;
    CGrunt** r2 = &m_triggerMgr->m_grid[m_curCell * 15];
    for (i32 k = 15; k != 0; k--) {
        CGrunt* g = *r2;
        if (g != 0 && g->m_2d8 == 0) {
            freeCount++;
        }
        r2++;
    }
    i32 budget = static_cast<i32>(
        (static_cast<double>(m_ctx->m_options[m_curCell].m_comboSel)
         * static_cast<double>(m_budgetMul) * g_diffScale)
    );
    if (roll >= m_spawnPct || freeCount >= budget) {
        unit->m_2d8 = 4;
    } else {
        unit->m_2d8 = 0;
    }
    unit->m_arrivalState = 0x11;
    unit->m_defenderState = 0;
    unit->m_arrivalCol = -1;
    unit->m_2f8 = -1;
    unit->m_defenderX = -1;
    unit->m_arrivalRow = -1;
    unit->m_2fc = -1;
    unit->m_defenderY = -1;
    unit->m_2e8 = -1;
    unit->m_2e4 = 0;
    unit->m_2e0 = 0;
    unit->m_dwell = 0;
    unit->m_390 = 1;
    return 1;
}

// ===========================================================================
// CBattlezMapConfig::StepRowUnits  @0x267c0  (10269 B)
// The master per-tick unit dispatcher over the current cell-row (called from
// StepBoard every tick). For each of the 15 row slots: four eligibility phases
// (each re-gated on the unit and the m_278/m_308 64-bit time windows), the
// I/G/L/P/J/C/R type-code chains, the m_2d8 state jump-table dispatch into the
// sibling state-machine methods, plus cold tail arms (entrance reset, head-coord
// arrival recycle, perimeter TileSwitch sweeps, Spellz radius trigger, tile-flag
// commit via StepEntranceReinit). Faithful oddities kept: the dead head-coord
// Coord writes (via the address-taken function-scope scratch Coord), the second
// column-sweep TileSwitch passing xl where the bounds check tests x+2 (retail
// copy-paste), the final-reclamp return 0 on intersect, and the `hit` local left
// unset on early phase exits.
// ===========================================================================
// @early-stop
RVA(0x000267c0, 0x2850)
i32 CBattlezMapConfig::StepRowUnits() {
    m_140++;
    CGrunt* unit;
    i32 hit;
    char eq;
    i32 cell;
    Coord scratch;
    for (i32 i = 0; i < 15; i++) {
        unit = m_triggerMgr->m_grid[m_curCell * 15 + i];
        if (unit != 0) {
            if (static_cast<i64>(static_cast<u32>(g_frameTime)) - unit->m_holdAnchor64
                < unit->m_holdWindow64) {
                return 1;
            }
        }
        if (unit != 0) {
            if (unit->CoordCount() != 0) {
                Coord* hc = (unit->CoordHead())->m_coord;
                scratch.m_x = hc->m_x;
                scratch.m_x = m_board->m_width;
                scratch.m_y = hc->m_y;
            }
        }
        {
            {
                if (unit != 0) {
                    if (static_cast<i64>(static_cast<u32>(g_frameTime)) - unit->m_arrivalReroll64
                        >= unit->m_arrivalRerollWindow64) {
                        winapi_02c140_IntersectRect_PtInRect(unit);
                        if (unit->m_poweredUp != 0) {
                            eq =
                                (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_1c)), "A")
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
                            i32 st = unit->m_entranceReason;
                            if (st > 0x16) {
                                st = unit->m_toolId;
                            }
                            if (st == 3 && unit->m_2d8 == 0) {
                                unit->m_2d8 = 0xa;
                                if (unit->CoordCount() != 0) {
                                    POSITION pos = unit->m_31c.GetHeadPosition();
                                    if (pos != 0) {
                                        do {
                                            void* d = unit->CoordListOps()->NextData(pos);
                                            if (d != 0) {
                                                g_coordPool.Push(d);
                                            }
                                        } while (pos != 0);
                                    }
                                    unit->m_31c.RemoveAll();
                                }
                            }
                        }
                        if (unit->IsAtSavedScreenPos() != 0 && unit->m_entranceCommitted != 0
                            && unit->m_deathAnimStarted == 0 && unit->m_entranceActive == 0
                            && unit->m_poweredUp == 0) {
                            eq =
                                (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_1c)), "I")
                                 == 0);
                            if (!eq) {
                                eq =
                                    (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_1c)), "G")
                                     == 0);
                                if (!eq) {
                                    eq =
                                        (strcmp(
                                             (*g_typeColl.GetNameRecord(unit->m_objAux->m_1c)),
                                             "L"
                                         )
                                         == 0);
                                    if (!eq) {
                                        eq =
                                            (strcmp(
                                                 (*g_typeColl.GetNameRecord(unit->m_objAux->m_1c)),
                                                 "P"
                                             )
                                             == 0);
                                        if (!eq) {
                                            eq =
                                                (strcmp(
                                                     (*g_typeColl.GetNameRecord(
                                                         unit->m_objAux->m_1c
                                                     )),
                                                     "J"
                                                 )
                                                 == 0);
                                            if (!eq) {
                                                eq =
                                                    (strcmp(
                                                         (*g_typeColl.GetNameRecord(
                                                             unit->m_objAux->m_1c
                                                         )),
                                                         "C"
                                                     )
                                                     == 0);
                                                if (!eq) {
                                                    eq =
                                                        (strcmp(
                                                             (*g_typeColl.GetNameRecord(
                                                                 unit->m_objAux->m_1c
                                                             )),
                                                             "R"
                                                         )
                                                         == 0);
                                                    if (!eq) {
                                                        i32 st2 = unit->m_entranceReason;
                                                        if (st2 > 0x16) {
                                                            st2 = unit->m_toolId;
                                                        }
                                                        if (st2 == 3 && unit->m_arrivalState == 4
                                                            && unit->m_defenderState == 6) {
                                                            unit->LoadPickupSprites(0, 1, 0, 0, 1);
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        if (unit->m_2d8 == 0xb) {
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
                                tile = m_board->m_rows[qy][s2.m_x].m_0;
                            } else {
                                tile = 1;
                            }
                            if (!(tile & 4)) {
                                unit->m_arrivalCol = -1;
                                unit->m_2d8 = 4;
                                unit->m_arrivalRow = -1;
                                if (unit->CoordCount() != 0) {
                                    POSITION pos = unit->m_31c.GetHeadPosition();
                                    if (pos != 0) {
                                        do {
                                            void* d = unit->CoordListOps()->NextData(pos);
                                            if (d != 0) {
                                                g_coordPool.Push(d);
                                            }
                                        } while (pos != 0);
                                    }
                                    unit->m_31c.RemoveAll();
                                }
                                unit->m_254 = 0;
                                unit->m_defenderState = 0;
                            }
                        }
                        {
                            i32 st = unit->m_entranceReason;
                            if (st > 0x16) {
                                st = unit->m_toolId;
                            }
                            if (st != 0xf && unit->m_2d8 == 9) {
                                unit->m_arrivalCol = -1;
                                unit->m_2d8 = 4;
                                unit->m_arrivalRow = -1;
                                if (unit->CoordCount() != 0) {
                                    POSITION pos = unit->m_31c.GetHeadPosition();
                                    if (pos != 0) {
                                        do {
                                            void* d = unit->CoordListOps()->NextData(pos);
                                            if (d != 0) {
                                                g_coordPool.Push(d);
                                            }
                                        } while (pos != 0);
                                    }
                                    unit->m_31c.RemoveAll();
                                }
                                unit->m_254 = 0;
                                unit->m_defenderState = 0;
                            }
                        }
                        {
                            i32 st = unit->m_entranceReason;
                            if (st > 0x16) {
                                st = unit->m_toolId;
                            }
                            if (st == 7) {
                                i32 d8 = unit->m_2d8;
                                if (d8 != 6 && d8 != 3) {
                                    if (unit->CoordCount() != 0) {
                                        POSITION pos = unit->m_31c.GetHeadPosition();
                                        if (pos != 0) {
                                            do {
                                                void* d = unit->CoordListOps()->NextData(pos);
                                                if (d != 0) {
                                                    g_coordPool.Push(d);
                                                }
                                            } while (pos != 0);
                                        }
                                        unit->m_31c.RemoveAll();
                                    }
                                    unit->m_arrivalCol = -1;
                                    unit->m_arrivalRow = -1;
                                    unit->m_2d8 = 6;
                                }
                            }
                        }
                        {
                            i32 st = unit->m_entranceReason;
                            if (st > 0x16) {
                                st = unit->m_toolId;
                            }
                            if (st != 7 && unit->m_2d8 == 6) {
                                unit->m_arrivalCol = -1;
                                unit->m_2d8 = 4;
                                unit->m_arrivalRow = -1;
                                if (unit->CoordCount() != 0) {
                                    POSITION pos = unit->m_31c.GetHeadPosition();
                                    if (pos != 0) {
                                        do {
                                            void* d = unit->CoordListOps()->NextData(pos);
                                            if (d != 0) {
                                                g_coordPool.Push(d);
                                            }
                                        } while (pos != 0);
                                    }
                                    unit->m_31c.RemoveAll();
                                }
                                unit->m_254 = 0;
                                unit->m_defenderState = 0;
                            }
                        }
                        if (unit->CoordCount() == 0) {
                            if (unit->m_defenderState == 5) {
                                unit->m_defenderState = 0;
                            }
                        }
                        if (unit->m_defenderState == 5) {
                            if (PathCrossesMarkedTile(unit) == 0) {
                                unit->m_defenderState = 0;
                            }
                        }
                        {
                            char ne;
                            ne =
                                (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_1c)), "C")
                                 != 0);
                            if (ne) {
                                ne =
                                    (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_1c)), "R")
                                     != 0);
                                if (ne) {
                                    ne =
                                        (strcmp(
                                             (*g_typeColl.GetNameRecord(unit->m_objAux->m_1c)),
                                             "C"
                                         )
                                         != 0);
                                    if (ne) {
                                        ne =
                                            (strcmp(
                                                 (*g_typeColl.GetNameRecord(unit->m_objAux->m_1c)),
                                                 "G"
                                             )
                                             != 0);
                                        if (ne) {
                                            ne =
                                                (strcmp(
                                                     (*g_typeColl.GetNameRecord(
                                                         unit->m_objAux->m_1c
                                                     )),
                                                     "L"
                                                 )
                                                 != 0);
                                            if (ne) {
                                                ne =
                                                    (strcmp(
                                                         (*g_typeColl.GetNameRecord(
                                                             unit->m_objAux->m_1c
                                                         )),
                                                         "P"
                                                     )
                                                     != 0);
                                                if (ne) {
                                                    ne =
                                                        (strcmp(
                                                             (*g_typeColl.GetNameRecord(
                                                                 unit->m_objAux->m_1c
                                                             )),
                                                             "J"
                                                         )
                                                         != 0);
                                                    if (ne) {
                                                        if (unit->m_object->m_screenX
                                                                == unit->m_lastTilePxX
                                                            && unit->m_object->m_screenY
                                                                   == unit->m_lastTilePxY
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
                                                            RECT bounds;
                                                            static_cast<RECT*>(new (&bounds) CRect(
                                                                0,
                                                                0,
                                                                board->m_width,
                                                                board->m_height
                                                            ));
                                                            RECT clamp;
                                                            RECT* pb = &box;
                                                            if (pb != 0) {
                                                                clamp.left = pb->left;
                                                                clamp.top = pb->top;
                                                                clamp.right = pb->right + 1;
                                                                clamp.bottom = pb->bottom + 1;
                                                            } else {
                                                                RECT tmp;
                                                                RECT* q = static_cast<RECT*>(
                                                                    new (&tmp) CRect(
                                                                        0,
                                                                        0,
                                                                        board->m_width,
                                                                        board->m_height
                                                                    )
                                                                );
                                                                clamp.left = q->left;
                                                                clamp.top = q->top;
                                                                clamp.right = q->right;
                                                                clamp.bottom = q->bottom;
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
                                                                        if (b->m_rows[row][col].m_0
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
                        RECT r1;
                        static_cast<RECT*>(new (&r1) CRect(0, 0, bd->m_width, bd->m_height));
                        RECT r2;
                        RECT* boardRect =
                            static_cast<RECT*>(new (&r2) CRect(0, 0, bd->m_width, bd->m_height));
                        RECT rc;
                        rc.left = boardRect->left;
                        rc.top = boardRect->top;
                        rc.right = boardRect->right;
                        rc.bottom = boardRect->bottom;
                        if (!IntersectRect(&bd->m_bounds, &rc, &r1)) {
                            bd->m_bounds = rc;
                        }
                        bd->m_gridW = bd->m_bounds.right - bd->m_bounds.left;
                        bd->m_gridH = bd->m_bounds.bottom - bd->m_bounds.top;
                    }
                        {
                            i32 special = 1;
                            if (unit->m_lastTilePxX != unit->m_object->m_screenX
                                || unit->m_object->m_screenY != unit->m_lastTilePxY) {
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
                                (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_1c)), "I")
                                 == 0);
                            if (eq) {
                                special = 0;
                            }
                            eq =
                                (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_1c)), "G")
                                 == 0);
                            if (eq) {
                                special = 0;
                            }
                            eq =
                                (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_1c)), "L")
                                 == 0);
                            if (eq) {
                                special = 0;
                            }
                            eq =
                                (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_1c)), "P")
                                 == 0);
                            if (eq) {
                                return 0;
                            }
                            eq =
                                (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_1c)), "J")
                                 == 0);
                            if (eq) {
                                special = 0;
                            }
                            eq =
                                (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_1c)), "C")
                                 == 0);
                            if (eq) {
                                special = 0;
                            }
                            eq =
                                (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_1c)), "R")
                                 == 0);
                            if (eq) {
                                special = 0;
                            }
                            if (unit->m_gruntKind == 0x36) {
                                special = 0;
                            }
                            if (special != 0) {
                                if (unit->m_poweredUp != 0 && unit->m_neighborValid == 0
                                    && unit->m_combatActive == 0 && unit->m_stamina >= 0x64) {
                                    if (unit->FindGridNeighbor(0) != 0) {
                                        return 1;
                                    }
                                }
                            }
                        }
                        if (unit->IsAtSavedScreenPos() != 0 && unit->m_entranceCommitted != 0
                            && unit->m_deathAnimStarted == 0 && unit->m_entranceActive == 0
                            && unit->m_poweredUp == 0) {
                            eq =
                                (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_1c)), "I")
                                 == 0);
                            if (!eq) {
                                eq =
                                    (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_1c)), "G")
                                     == 0);
                                if (!eq) {
                                    eq =
                                        (strcmp(
                                             (*g_typeColl.GetNameRecord(unit->m_objAux->m_1c)),
                                             "L"
                                         )
                                         == 0);
                                    if (!eq) {
                                        eq =
                                            (strcmp(
                                                 (*g_typeColl.GetNameRecord(unit->m_objAux->m_1c)),
                                                 "P"
                                             )
                                             == 0);
                                        if (!eq) {
                                            eq =
                                                (strcmp(
                                                     (*g_typeColl.GetNameRecord(
                                                         unit->m_objAux->m_1c
                                                     )),
                                                     "J"
                                                 )
                                                 == 0);
                                            if (!eq) {
                                                eq =
                                                    (strcmp(
                                                         (*g_typeColl.GetNameRecord(
                                                             unit->m_objAux->m_1c
                                                         )),
                                                         "C"
                                                     )
                                                     == 0);
                                                if (!eq) {
                                                    eq =
                                                        (strcmp(
                                                             (*g_typeColl.GetNameRecord(
                                                                 unit->m_objAux->m_1c
                                                             )),
                                                             "R"
                                                         )
                                                         == 0);
                                                    if (!eq) {
                                                        for (i32 j = 0; j < 4; j++) {
                                                            if (j != m_curCell) {
                                                                for (i32 k = 0; k < 15; k++) {
                                                                    CGrunt* other =
                                                                        m_triggerMgr
                                                                            ->m_grid[j * 15 + k];
                                                                    if (other != 0) {
                                                                        if (unit->RectContains(
                                                                                other->m_object
                                                                                    ->m_screenX,
                                                                                other->m_object
                                                                                    ->m_screenY
                                                                            )
                                                                            != 0) {
                                                                            if (unit->m_gruntKind
                                                                                != 0x36) {
                                                                                if (other
                                                                                        ->m_poweredUp
                                                                                    == 0) {
                                                                                    if (winapi_02ae00_IntersectRect(
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
        if (unit != 0) {
            if (static_cast<i64>(static_cast<u32>(g_frameTime)) - unit->m_arrivalReroll64
                >= unit->m_arrivalRerollWindow64) {
                i32 d8 = unit->m_2d8;
                if (d8 != 3 && d8 != 0xb) {
                    if (unit->m_entranceCommitted != 0 && unit->m_deathAnimStarted == 0
                        && unit->m_entranceActive == 0 && unit->m_poweredUp == 0) {
                        char ne;
                        ne = (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_1c)), "I") != 0);
                        if (ne) {
                            ne =
                                (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_1c)), "G")
                                 != 0);
                            if (ne) {
                                ne =
                                    (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_1c)), "L")
                                     != 0);
                                if (ne) {
                                    ne =
                                        (strcmp(
                                             (*g_typeColl.GetNameRecord(unit->m_objAux->m_1c)),
                                             "P"
                                         )
                                         != 0);
                                    if (ne) {
                                        ne =
                                            (strcmp(
                                                 (*g_typeColl.GetNameRecord(unit->m_objAux->m_1c)),
                                                 "J"
                                             )
                                             != 0);
                                        if (ne) {
                                            ne =
                                                (strcmp(
                                                     (*g_typeColl.GetNameRecord(
                                                         unit->m_objAux->m_1c
                                                     )),
                                                     "C"
                                                 )
                                                 != 0);
                                            if (ne) {
                                                ne =
                                                    (strcmp(
                                                         (*g_typeColl.GetNameRecord(
                                                             unit->m_objAux->m_1c
                                                         )),
                                                         "R"
                                                     )
                                                     != 0);
                                                if (ne) {
                                                    if (unit->m_2d8 != 0) {
                                                        if (winapi_02e3a0_PtInRect(unit) != 0) {
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
        if (unit != 0) {
            if (unit->m_object->m_screenX == unit->m_lastTilePxX
                && unit->m_lastTilePxY == unit->m_object->m_screenY
                && unit->m_entranceCommitted != 0 && unit->m_deathAnimStarted == 0
                && unit->m_entranceActive == 0 && unit->m_poweredUp == 0) {
                eq = (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_1c)), "I") == 0);
                if (!eq) {
                    eq = (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_1c)), "G") == 0);
                    if (!eq) {
                        eq = (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_1c)), "L") == 0);
                        if (!eq) {
                            eq =
                                (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_1c)), "P")
                                 == 0);
                            if (!eq) {
                                eq =
                                    (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_1c)), "J")
                                     == 0);
                                if (!eq) {
                                    eq =
                                        (strcmp(
                                             (*g_typeColl.GetNameRecord(unit->m_objAux->m_1c)),
                                             "C"
                                         )
                                         == 0);
                                    if (!eq) {
                                        eq =
                                            (strcmp(
                                                 (*g_typeColl.GetNameRecord(unit->m_objAux->m_1c)),
                                                 "R"
                                             )
                                             == 0);
                                        if (!eq) {
                                            if (static_cast<u32>(m_140) % 15
                                                == static_cast<u32>(i)) {
                                                {
                                                    i32 st3 = unit->m_entranceReason;
                                                    if (st3 > 0x16) {
                                                        st3 = unit->m_toolId;
                                                    }
                                                    if (st3 == 0x13 && unit->m_health > 0x1a) {
                                                        if (rand() % g_diffTier == 0) {
                                                            i32 r = g_buteMgr.GetIntDef(
                                                                "Spellz",
                                                                "SpellRadius",
                                                                8
                                                            );
                                                            RECT spell;
                                                            i32 px = unit->m_object->m_screenX;
                                                            i32 py = unit->m_object->m_screenY;
                                                            spell.left = (px >> 5) - r;
                                                            spell.top = (py >> 5) - r;
                                                            spell.right = (px >> 5) + r;
                                                            spell.bottom = (py >> 5) + r;
                                                            for (i32 j2 = 0; j2 < 4; j2++) {
                                                                if (j2 != m_curCell) {
                                                                    for (i32 k2 = 0; k2 < 15;
                                                                         k2++) {
                                                                        CGrunt* o =
                                                                            m_triggerMgr->m_grid
                                                                                [j2 * 15 + k2];
                                                                        if (o != 0) {
                                                                            POINT pt;
                                                                            pt.x = o->m_object
                                                                                       ->m_screenX
                                                                                   >> 5;
                                                                            pt.y = o->m_object
                                                                                       ->m_screenY
                                                                                   >> 5;
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
                                                    && unit->m_defenderState == 4) {
                                                    unit->m_2f8 = -1;
                                                    unit->m_defenderState = 0;
                                                    unit->m_2fc = -1;
                                                }
                                                {
                                                    char nd;
                                                    nd =
                                                        (strcmp(
                                                             (*g_typeColl.GetNameRecord(
                                                                 unit->m_objAux->m_1c
                                                             )),
                                                             "D"
                                                         )
                                                         != 0);
                                                    if (nd) {
                                                        ResolveArrival(unit);
                                                    }
                                                }
                                                if (unit->m_object->m_screenX == unit->m_lastTilePxX
                                                    && unit->m_object->m_screenY
                                                           == unit->m_lastTilePxY
                                                    && unit->m_entranceCommitted != 0
                                                    && unit->m_deathAnimStarted == 0
                                                    && unit->m_entranceActive == 0
                                                    && unit->m_poweredUp == 0) {
                                                    eq =
                                                        (strcmp(
                                                             (*g_typeColl.GetNameRecord(
                                                                 unit->m_objAux->m_1c
                                                             )),
                                                             "I"
                                                         )
                                                         == 0);
                                                    if (!eq) {
                                                        eq =
                                                            (strcmp(
                                                                 (*g_typeColl.GetNameRecord(
                                                                     unit->m_objAux->m_1c
                                                                 )),
                                                                 "G"
                                                             )
                                                             == 0);
                                                        if (!eq) {
                                                            eq =
                                                                (strcmp(
                                                                     (*g_typeColl.GetNameRecord(
                                                                         unit->m_objAux->m_1c
                                                                     )),
                                                                     "L"
                                                                 )
                                                                 == 0);
                                                            if (!eq) {
                                                                eq =
                                                                    (strcmp(
                                                                         (*g_typeColl.GetNameRecord(
                                                                             unit->m_objAux->m_1c
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
                                                                                           ->m_1c
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
                                                                                               ->m_1c
                                                                                       )),
                                                                                 "C"
                                                                             )
                                                                             == 0);
                                                                        if (!eq) {
                                                                            eq =
                                                                                (strcmp(
                                                                                     (*g_typeColl.GetNameRecord(
                                                                                         unit->m_objAux
                                                                                             ->m_1c
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
        RECT t2;
        RECT* q2 = static_cast<RECT*>(new (&t2) CRect(0, 0, bd2->m_width, bd2->m_height));
        RECT b2;
        b2.left = q2->left;
        b2.top = q2->top;
        b2.right = q2->right;
        b2.bottom = q2->bottom;
        if (!IntersectRect(&bd2->m_bounds, &b2, &a)) {
            bd2->m_bounds = b2;
        }
        bd2->m_gridW = bd2->m_bounds.right - bd2->m_bounds.left;
        bd2->m_gridH = bd2->m_bounds.bottom - bd2->m_bounds.top;
        i32 stX = unit->m_entranceReason;
        if (hit == 0) {
            switch (unit->m_2d8) {
                case 0: {
                    Step33520(unit);
                    break;
                }
                case 2: {
                    Step(unit);
                    break;
                }
                case 3: {
                    winapi_031ca0_IntersectRect(unit);
                    break;
                }
                case 4: {
                    winapi_032060_IntersectRect(unit);
                    break;
                }
                case 6: {
                    RepathToFreeCell(unit);
                    break;
                }
                case 7: {
                    CheckQueuedSpawnTile(unit);
                    break;
                }
                case 9: {
                    RetargetIdleUnit(unit);
                    break;
                }
                case 0xb: {
                    Scan(unit);
                    break;
                }
                case 0xa: {
                    ScanRegion(unit);
                    break;
                }
                default:
                    break;
            }
        }
        if (unit->CoordCount() != 0) {
            eq = (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_1c)), "A") == 0);
            if (eq) {
                Coord* gc = (unit->CoordHead())->m_coord;
                i32 gx = gc->m_x;
                i32 gy = gc->m_y;
                i32 sx = unit->m_object->m_screenX >> 5;
                i32 sy = unit->m_object->m_screenY >> 5;
                if (abs(gx - sx) < 2 && abs(gy - sy) < 2) {
                    cell = m_board->m_rows[gy][gx].m_0;
                    i32 f = unit->m_arrivalFlags & cell;
                    if (f & 0x20000000) {
                        goto LB;
                    }
                    if (f == 0) {
                        goto LA;
                    }
                    if ((cell & unit->m_24c) == 0) {
                        goto LB;
                    }
                    if (cell & 0x20000000) {
                        goto LB;
                    }
                    if ((cell & 0x40) == 0) {
                        goto flagsArm;
                    }
                LA:
                    if (unit->m_entranceReason <= 0x16) {
                        if (unit->m_entranceReason == 0x16) {
                            goto nexti;
                        }
                        goto LC;
                    }
                LB:
                    if (unit->m_toolId != 0x16) {
                        goto nexti;
                    }
                    if (cell & 2) {
                        goto LR;
                    }
                    if ((cell & 0x100) == 0) {
                        goto nexti;
                    }
                LC:
                    if ((cell & 0x20000000) == 0) {
                        goto tailArm2;
                    }
                    goto nexti;
                LR:
                    if (unit->CoordCount() != 0) {
                        CoordNode* n = unit->CoordHead();
                        if (n != 0) {
                            do {
                                CoordNode* cur = n;
                                n = n->m_next;
                                if (cur->m_coord != 0) {
                                    CoordPoolNode* node = g_coordPool.NodeOf(cur->m_coord);
                                    node->m_next = g_coordPool.m_freeHead;
                                    g_coordPool.m_freeHead = node;
                                }
                            } while (n != 0);
                        }
                        unit->m_31c.RemoveAll();
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
        POSITION pos = unit->m_31c.GetHeadPosition();
        if (pos != 0) {
            do {
                void* d = unit->CoordListOps()->NextData(pos);
                if (d != 0) {
                    g_coordPool.Push(d);
                }
            } while (pos != 0);
        }
        unit->m_31c.RemoveAll();
    }
    return 1;

perimSweep: {
    Coord q0;
    (static_cast<CUserLogic*>(unit))->GetScreenPos((&q0));
    i32 col = (q0.m_x >> 5) - 2;
    (static_cast<CUserLogic*>(unit))->GetScreenPos((&scratch));
    scratch.m_x >>= 5;
    scratch.m_y >>= 5;
    while (col < scratch.m_x + 3) {
        Coord qa;
        (static_cast<CUserLogic*>(unit))->GetScreenPos((&qa));
        i32 rt = (qa.m_y >> 5) - 2;
        if (static_cast<u32>(col) < m_board->m_width && static_cast<u32>(rt) < m_board->m_height) {
            if (unit->TileSwitch(col, rt, 0, 0x2000098b, 1, 0) != 0) {
                goto rowHitA;
            }
        }
        Coord qc;
        (static_cast<CUserLogic*>(unit))->GetScreenPos((&qc));
        i32 rb = (qc.m_y >> 5) + 2;
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
        i32 row = (u0.m_y >> 5) - 2;
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
                // Retail copy-paste: the bounds check tests x+2 but the call passes xl.
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
        RECT f1;
        static_cast<RECT*>(new (&f1) CRect(0, 0, fb->m_width, fb->m_height));
        RECT f2;
        RECT* fq = static_cast<RECT*>(new (&f2) CRect(0, 0, fb->m_width, fb->m_height));
        RECT fc;
        fc.left = fq->left;
        fc.top = fq->top;
        fc.right = fq->right;
        fc.bottom = fq->bottom;
        if (IntersectRect(&fb->m_bounds, &fc, &f1) != 0) {
            return 0;
        }
        fb->m_bounds = fc;
        fb->m_gridW = fb->m_bounds.right - fb->m_bounds.left;
        fb->m_gridH = fb->m_bounds.bottom - fb->m_bounds.top;
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
    RECT h1;
    static_cast<RECT*>(new (&h1) CRect(0, 0, hb->m_width, hb->m_height));
    RECT h2;
    RECT* hq = static_cast<RECT*>(new (&h2) CRect(0, 0, hb->m_width, hb->m_height));
    RECT hc;
    hc.left = hq->left;
    hc.top = hq->top;
    hc.right = hq->right;
    hc.bottom = hq->bottom;
    if (!IntersectRect(&hb->m_bounds, &hc, &h1)) {
        hb->m_bounds = hc;
    }
    hb->m_gridW = hb->m_bounds.right - hb->m_bounds.left;
    hb->m_gridH = hb->m_bounds.bottom - hb->m_bounds.top;
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
    RECT h1;
    static_cast<RECT*>(new (&h1) CRect(0, 0, hb->m_width, hb->m_height));
    RECT h2;
    RECT* hq = static_cast<RECT*>(new (&h2) CRect(0, 0, hb->m_width, hb->m_height));
    RECT hc;
    hc.left = hq->left;
    hc.top = hq->top;
    hc.right = hq->right;
    hc.bottom = hq->bottom;
    if (!IntersectRect(&hb->m_bounds, &hc, &h1)) {
        hb->m_bounds = hc;
    }
    hb->m_gridW = hb->m_bounds.right - hb->m_bounds.left;
    hb->m_gridH = hb->m_bounds.bottom - hb->m_bounds.top;
    return 1;
}

spellHit: {
    i32 hx = unit->m_lastTilePxX;
    i32 hy = unit->m_lastTilePxY;
    m_triggerMgr->ApplyTriggerA(unit->m_tileOwnerHi, unit->m_tileOwnerLo, hx, hy);
    return 1;
}

flagsArm: {
    i32 ok = 1;
    if (cell & 8) {
        if ((unit->m_entranceReason > 0x16 ? unit->m_toolId : unit->m_entranceReason) != 0x12
            && (unit->m_entranceReason > 0x16 ? unit->m_toolId : unit->m_entranceReason) != 0x16) {
            ok = 0;
        }
    }
    if (cell & 0x200) {
        if ((unit->m_entranceReason > 0x16 ? unit->m_toolId : unit->m_entranceReason) != 0x12
            && (unit->m_entranceReason > 0x16 ? unit->m_toolId : unit->m_entranceReason) != 0x16) {
            ok = 0;
        }
    }
    if (ok == 0) {
        return 1;
    }
    {
        Coord* tc = (unit->CoordTail())->m_coord;
        unit->m_entrancePxX = (tc->m_x << 5) + 0x10;
        unit->m_entrancePxY = (tc->m_y << 5) + 0x10;
        unit->StepEntranceReinit();
        return 1;
    }
}

tailArm2: {
    Coord* tc = (unit->CoordTail())->m_coord;
    unit->m_entrancePxX = (tc->m_x << 5) + 0x10;
    unit->m_entrancePxY = (tc->m_y << 5) + 0x10;
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
    RECT h1;
    static_cast<RECT*>(new (&h1) CRect(0, 0, hb->m_width, hb->m_height));
    RECT h2;
    RECT* hq = static_cast<RECT*>(new (&h2) CRect(0, 0, hb->m_width, hb->m_height));
    RECT hc;
    hc.left = hq->left;
    hc.top = hq->top;
    hc.right = hq->right;
    hc.bottom = hq->bottom;
    if (!IntersectRect(&hb->m_bounds, &hc, &h1)) {
        hb->m_bounds = hc;
    }
    hb->m_gridW = hb->m_bounds.right - hb->m_bounds.left;
    hb->m_gridH = hb->m_bounds.bottom - hb->m_bounds.top;
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
    RECT h1;
    static_cast<RECT*>(new (&h1) CRect(0, 0, hb->m_width, hb->m_height));
    RECT h2;
    RECT* hq = static_cast<RECT*>(new (&h2) CRect(0, 0, hb->m_width, hb->m_height));
    RECT hc;
    hc.left = hq->left;
    hc.top = hq->top;
    hc.right = hq->right;
    hc.bottom = hq->bottom;
    if (!IntersectRect(&hb->m_bounds, &hc, &h1)) {
        hb->m_bounds = hc;
    }
    hb->m_gridW = hb->m_bounds.right - hb->m_bounds.left;
    hb->m_gridH = hb->m_bounds.bottom - hb->m_bounds.top;
    return 1;
}
}

RVA(0x00029a30, 0x10)
void*& CGruntCoordList::NextData(POSITION& pos) {
    // Literally CPtrList::GetNext, kept OUT-OF-LINE: `cur = pos; pos = cur->pNext;
    // return cur->data`. Inlining MFC's accessor into this one body reproduces the
    // 0x10-byte retail function exactly and retires the two raw-node puns.
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
i32 CUserLogic::IsAtSavedScreenPos() {
    CWwdGameObjectA* o = m_object;
    // every caller is a grunt; the saved pos lives in the CGrunt spine (+0x17c/+0x180)
    CGrunt* g = static_cast<CGrunt*>(this);
    i32 sx = g->m_lastTilePxX;
    if (o->m_screenX == sx && o->m_screenY == g->m_lastTilePxY) {
        return 1;
    }
    return 0;
}

// @early-stop
RVA(0x00029b40, 0x813)
i32 CBattlezMapConfig::ValidateUnitPath(CGrunt* unit) {
    CPtrList* coordList = &unit->m_31c;
    if (unit->CoordCount() == 0) {
        return 0;
    }
    // --- geometry-drift head: recycle-and-bail if the unit's stored first coord and
    //     its live screen cell have drifted >= 2 cells apart. ---
    Coord* c0 = unit->CoordHead()->m_coord;
    i32 ux = c0->m_x;
    i32 uy = c0->m_y;
    Coord pt;
    (static_cast<CUserLogic*>(unit))->GetScreenPos((&pt));
    i32 gx = pt.m_x >> 5;
    (static_cast<CUserLogic*>(unit))->GetScreenPos((&pt));
    i32 gy = pt.m_y >> 5;
    if (abs(ux - gx) >= 2) {
        goto recycleBail;
    }
    if (abs(uy - gy) >= 2) {
        goto recycleBail;
    }
    {
        CMapMgr* board = m_board;
        // tile0: is the unit's own coord cell blocked (low flag byte == 1)?
        i32 tile0;
        if (static_cast<u32>(ux) < static_cast<u32>(board->m_width)
            && static_cast<u32>(uy) < static_cast<u32>(board->m_height)) {
            tile0 = (static_cast<BrickzCell*>(board->m_rows[uy]))[ux].m_0;
        } else {
            tile0 = 1;
        }
        if (static_cast<u8>(tile0) == 1) {
            if (unit->CoordCount() == 0) {
                return 0;
            }
            CoordNode* n = unit->CoordHead();
            while (n != 0) {
                CoordNode* cur = n;
                n = n->m_next;
                if (cur->m_coord != 0) {
                    g_coordPool.Push(cur->m_coord);
                }
            }
            coordList->RemoveAll();
            return 0;
        }
        // --- scratchA: the 7-dword tile record under the unit's STORED coord (the
        //     main flags word). Retail copies the record to a stack scratch (rep movs),
        //     defaulting to all-0x01 (memset) when out of bounds. This forces the 0x58
        //     EH frame; the flag dispatch reads the scratch. ---
        i32 cx = c0->m_x;
        i32 cy = c0->m_y;
        (static_cast<CUserLogic*>(unit))->GetScreenPos((&pt));
        BrickzCell scratchA;
        const BrickzCell* srcA;
        if (static_cast<u32>(cx) < static_cast<u32>(board->m_width)
            && static_cast<u32>(cy) < static_cast<u32>(board->m_height)) {
            srcA = &(static_cast<BrickzCell*>(board->m_rows[cy]))[cx];
        } else {
            memset(&scratchA, 1, sizeof(scratchA));
            srcA = &scratchA;
        }
        if (unit->CoordCount() == 0) {
            return 0;
        }
        scratchA = *srcA;
        i32 prim = unit->m_entranceReason;
        if (prim > 0x16) {
            prim = unit->m_toolId;
        }
        // --- scratchB: the tile record under the unit's LIVE screen cell (secondary
        //     gate). Retail reads the y from the first probe and the x from the second. ---
        Coord pt2;
        (static_cast<CUserLogic*>(unit))->GetScreenPos((&pt2));
        i32 sgy = pt2.m_y >> 5;
        (static_cast<CUserLogic*>(unit))->GetScreenPos((&pt));
        i32 sgx = pt.m_x >> 5;
        BrickzCell scratchB;
        const BrickzCell* srcB;
        if (static_cast<u32>(sgx) < static_cast<u32>(board->m_width)
            && static_cast<u32>(sgy) < static_cast<u32>(board->m_height)) {
            srcB = &(static_cast<BrickzCell*>(board->m_rows[sgy]))[sgx];
        } else {
            memset(&scratchB, 1, sizeof(scratchB));
            srcB = &scratchB;
        }
        scratchB = *srcB;
        // --- reroute arm: scratchB blocked-bit (0x4) + unit not already in mode 0xb.
        //     If the live cell resolves to a kind-2 cell record, drop the unit's path
        //     and latch it into mode 0xb. ---
        if ((scratchB.m_0 & 0x4) && unit->m_2d8 != 0xb) {
            (static_cast<CUserLogic*>(unit))->GetScreenPos((&pt));
            i32 rx = pt.m_x >> 5;
            (static_cast<CUserLogic*>(unit))->GetScreenPos((&pt2));
            i32 ry = pt2.m_y >> 5;
            CTileTriggerSwitchLogic* rec = m_cellQuery->FindChild((rx << 8) + ry, 0);
            if (rec->m_typeId == 2) {
                unit->m_defenderState = 0;
                if (unit->CoordCount() != 0) {
                    CoordNode* n = unit->CoordHead();
                    while (n != 0) {
                        CoordNode* cur = n;
                        n = n->m_next;
                        if (cur->m_coord != 0) {
                            g_coordPool.Push(cur->m_coord);
                        }
                    }
                    coordList->RemoveAll();
                }
                unit->m_2d8 = 0xb;
                unit->m_dwell = 0;
                return 0;
            }
        }
        // --- prim==0x11 arm: with two occupied coords, if the second's tile is a
        //     0x20 cell and the first's is not a 0x2 cell, fire the coord trigger. ---
        i32 entranceMode = unit->m_entranceReason;
        if (entranceMode > 0x16) {
            entranceMode = unit->m_toolId;
        }
        if (entranceMode == 0x11 && unit->CoordCount() >= 2) {
            CoordNode* node = unit->CoordHead();
            Coord* ca = node->m_coord;
            CoordNode* nn = node->m_next;
            i32 ax = ca->m_x;
            Coord* cb = nn->m_coord;
            i32 ay = ca->m_y;
            i32 bx = cb->m_x;
            i32 by = cb->m_y;
            i32 tB;
            if (static_cast<u32>(bx) < static_cast<u32>(board->m_width)
                && static_cast<u32>(by) < static_cast<u32>(board->m_height)) {
                tB = (static_cast<BrickzCell*>(board->m_rows[by]))[bx].m_0;
            } else {
                tB = 1;
            }
            if (tB & 0x20) {
                i32 tA2;
                if (static_cast<u32>(ax) < static_cast<u32>(board->m_width)
                    && static_cast<u32>(ay) < static_cast<u32>(board->m_height)) {
                    tA2 = (static_cast<BrickzCell*>(board->m_rows[ay]))[ax].m_0;
                } else {
                    tA2 = 1;
                }
                if (!(tA2 & 0x2)) {
                    m_triggerMgr->ApplyTriggerB(
                        unit->m_tileOwnerHi,
                        unit->m_tileOwnerLo,
                        ax * 0x20 + 0x10,
                        ay * 0x20 + 0x10
                    );
                    return 0;
                }
            }
        }
        // --- 0x8000 arms: scratchB 0x8000 clears an in-progress state-3; scratchA
        //     0x8000 gates a commit (mode 0xa) or a PathCrossesMarkedTile hand-off. ---
        if ((scratchB.m_0 & 0x8000) && unit->m_defenderState == 3) {
            unit->m_defenderState = 0;
        }
        i32 sA = scratchA.m_0;
        if (sA & 0x8000) {
            if (prim == 3 && unit->m_2d8 == 0xa) {
                m_triggerMgr->ApplyTriggerB(
                    unit->m_tileOwnerHi,
                    unit->m_tileOwnerLo,
                    cx * 0x20 + 0x10,
                    cy * 0x20 + 0x10
                );
                unit->m_defenderState = 0;
                if (unit->CoordCount() != 0) {
                    CoordNode* n = unit->CoordHead();
                    while (n != 0) {
                        CoordNode* cur = n;
                        n = n->m_next;
                        if (cur->m_coord != 0) {
                            g_coordPool.Push(cur->m_coord);
                        }
                    }
                    coordList->RemoveAll();
                }
                return 0;
            }
            if (PathCrossesMarkedTile(unit) == 0 && unit->m_defenderState == 7) {
                CoordNode* head = unit->CoordHead();
                if (head != 0) {
                    CoordNode* n = head->m_next;
                    if (n != 0) {
                        while (n != 0) {
                            CoordNode* cur = n;
                            n = n->m_next;
                            if (cur->m_coord != 0) {
                                CoordPoolNode* fn = g_coordPool.NodeOf(cur->m_coord);
                                fn->m_next = g_coordPool.m_freeHead;
                                g_coordPool.m_freeHead = fn;
                                CoordPos cp;
                                cp.m_node = cur;
                                coordList->RemoveAt(cp.m_pos);
                            }
                        }
                        return 1;
                    }
                }
            }
        }
        // --- 0x2a024: main scratchA bit dispatch ---
        if (sA & 0x200) {
            i32 p = unit->m_entranceReason;
            if (p > 0x16) {
                p = unit->m_toolId;
            }
            if (p != 0x16) {
                return 0;
            }
        }
        if (sA & 0x8) {
            i32 hi = sA & 0x100;
            if (hi) {
                i32 p = unit->m_entranceReason;
                if (p > 0x16) {
                    p = unit->m_toolId;
                }
                if (p == 0x16) {
                    return 1;
                }
                i32 entranceMode2 = unit->m_entranceReason;
                if (entranceMode2 > 0x16) {
                    entranceMode2 = unit->m_toolId;
                }
                if (entranceMode2 == 0x12) {
                    return 1;
                }
            }
            i32 lo2 = sA & 0x2;
            if (lo2) {
                i32 p = unit->m_entranceReason;
                if (p > 0x16) {
                    p = unit->m_toolId;
                }
                if (p == 0x16) {
                    return 1;
                }
            }
            if (PathToNearestGoal(unit, cx, cy) != 0) {
                return 1;
            }
            i32 sB = scratchB.m_0;
            if ((sB & 0x200) || (sB & 0x8)) {
                return 0;
            }
            if (hi && unit->m_defenderState != 3) {
                i32 pick = (rand() % 5) != 0 ? 0x12 : 0x16;
                EnterDefenderMode(unit, pick);
            }
            if (lo2) {
                if (unit->m_defenderState == 3) {
                    return 0;
                }
                EnterDefenderMode(unit, 0x16);
            }
            return 0;
        }
        // --- sA & 0x8 == 0 ---
        if ((sA & 0x20) && prim != 5 && prim != 0x11 && prim != 1) {
            if (unit->m_defenderState == 3) {
                return 0;
            }
            EnterDefenderMode(unit, 5);
            return 0;
        }
        if (sA & 0x40) {
            i32 p = unit->m_entranceReason;
            if (p > 0x16) {
                p = unit->m_toolId;
            }
            if (p != 0x16) {
                if (prim == 0xd) {
                    return 0;
                }
                if (unit->m_defenderState == 3) {
                    return 0;
                }
                EnterDefenderMode(unit, 0xd);
                return 0;
            }
        }
        if (sA & 0x2) {
            i32 p = unit->m_entranceReason;
            if (p > 0x16) {
                p = unit->m_toolId;
            }
            if (p == 0x16) {
                return 0;
            }
        }
        if (sA & 0x20000000) {
            RepathAroundBlockedTiles(unit);
            return 0;
        }
        i32 pk = unit->m_entranceReason;
        if (pk > 0x16) {
            pk = unit->m_toolId;
        }
        if (pk != 0x7) {
            return 1;
        }
        // --- kind-7 arm: scan the grid candidate list for an unoccupied cell that
        //     falls inside the unit's reach rect; on a hit, fire the trigger, recycle
        //     the unit's path, advance the spawn timer, and return 1. ---
        POSITION opos = m_triggerMgr->m_baseList.GetHeadPosition();
        while (opos != 0) {
            CGruntPuddle* cand = static_cast<CGruntPuddle*>(m_triggerMgr->m_baseList.GetNext(opos));
            if (cand->m_pending == 0) {
                i32 ox = cand->m_tileX;
                i32 oy = cand->m_tileY;
                if ((static_cast<CGrunt*>(unit))->RectContains(ox * 0x20 + 0x10, oy * 0x20 + 0x10)
                    != 0) {
                    m_triggerMgr->ApplyTriggerB(
                        unit->m_tileOwnerHi,
                        unit->m_tileOwnerLo,
                        ox * 0x20 + 0x10,
                        oy * 0x20 + 0x10
                    );
                    if (unit->CoordCount() != 0) {
                        CoordNode* n = unit->CoordHead();
                        while (n != 0) {
                            CoordNode* cur = n;
                            n = n->m_next;
                            if (cur->m_coord != 0) {
                                CoordPoolNode* fn = g_coordPool.NodeOf(cur->m_coord);
                                fn->m_next = g_coordPool.m_freeHead;
                                g_coordPool.m_freeHead = fn;
                            }
                        }
                        coordList->RemoveAll();
                    }
                    m_spawnTimer += static_cast<i32>((static_cast<u32>(m_spawnInterval) >> 2));
                    return 1;
                }
            }
        }
        return 1;
    }
recycleBail:
    if (unit->CoordCount() == 0) {
        return 0;
    }
    {
        CoordNode* n = unit->CoordHead();
        while (n != 0) {
            CoordNode* cur = n;
            n = n->m_next;
            if (cur->m_coord != 0) {
                g_coordPool.Push(cur->m_coord);
            }
        }
        coordList->RemoveAll();
    }
    return 0;
}

// ===========================================================================
// CBattlezMapConfig::RepathAroundBlockedTiles  @0x02a570  (/GX EH frame)
// The reserved-tile scatter reroute. For a unit that holds occupied coords, clamp
// the board dirty-rect to a 13x13 box around its screen coord (IntersectRect copy-
// back), then scan up to three of its coord-list nodes for one on a blocked (bit 0)
// tile (or its own tail coord); for such a node build the FindPath flag word from
// the unit's 0x12/0x16/0xe anim modes and ask CMapMgr::FindPath (flags 0x2000098f) for
// a route into a local CPtrList. On a route: recycle the route head + the unit's old
// coords onto g_coordPool.m_freeHead/g_coordPool, empty its coord list, AddTail the new route,
// re-clamp the board dirty-rect, stamp the unit's packed coord from the new tail, and
// return 1. Exhausting the three nodes re-clamps the board dirty-rect and returns 0.
// ===========================================================================
// @early-stop
RVA(0x0002a570, 0x4c6)
i32 CBattlezMapConfig::RepathAroundBlockedTiles(CGrunt* unit) {
    if (unit->CoordCount() == 0) {
        return 1;
    }
    void* pos = unit->CoordHead();
    Coord center;
    (static_cast<CUserLogic*>(unit))->GetScreenPos((&center));
    CMapMgr* board = m_board;
    i32 cx = center.m_x >> 5;
    i32 cy = center.m_y >> 5;
    RECT bounds;
    static_cast<RECT*>(new (&bounds) CRect(0, 0, board->m_width, board->m_height));
    RECT box;
    box.left = cx - 6;
    box.top = cy - 6;
    box.right = (cx + 6) + 1;
    box.bottom = (cy + 6) + 1;
    if (!IntersectRect(&board->m_bounds, &box, &bounds)) {
        board->m_bounds = box;
    }
    board->m_gridW = board->m_bounds.right - board->m_bounds.left;
    board->m_gridH = board->m_bounds.bottom - board->m_bounds.top;
    Coord* tailCoord = (unit->CoordTail())->m_coord;
    i32 tx = tailCoord->m_x;
    i32 ty = tailCoord->m_y;
    i32 iter = 0;
    CoordNode* node = *static_cast<CoordNode**>(pos);
    while (node != 0 && iter < 3) {
        CoordNode* cur = node;
        node = node->m_next;
        Coord* coord = cur->m_coord;
        if (coord == 0) {
            continue;
        }
        i32 x = coord->m_x;
        i32 y = coord->m_y;
        i32 tile = board->m_rowInts[y][x * 7];
        i32 proceed = 1;
        if (tile & 1) {
            if (x != tx || y != ty) {
                proceed = 0;
            }
        }
        if (proceed == 0) {
            continue;
        }
        CPtrList list(10);
        i32 flags = 0;
        i32 prim = unit->m_entranceReason;
        if (prim > 0x16) {
            prim = unit->m_toolId;
        }
        if (prim == 0x12) {
            flags = 0x100;
        }
        prim = unit->m_entranceReason;
        if (prim > 0x16) {
            prim = unit->m_toolId;
        }
        if (prim == 0x16) {
            flags = 0x942;
        }
        prim = unit->m_entranceReason;
        if (prim > 0x16) {
            prim = unit->m_toolId;
        }
        if (prim == 0xe) {
            flags = 0x1000;
        }
        if (board->SearchEdge(cx, cy, coord->m_x, coord->m_y, &list, 1, 0x2000098f, flags) != 0
            && list.GetCount() != 0) {
            void* head = list.RemoveHead();
            if (head != 0) {
                CoordPoolNode* n = g_coordPool.NodeOf(head);
                n->m_next = g_coordPool.m_freeHead;
                g_coordPool.m_freeHead = n;
            }
            if (list.GetCount() != 0) {
                // Recycle the unit's current path coords onto g_coordPool, empty it.
                if (unit->CoordCount() != 0) {
                    CoordNode* p = unit->CoordHead();
                    while (p != 0) {
                        CoordNode* c2 = p;
                        p = p->m_next;
                        if (c2->m_coord != 0) {
                            g_coordPool.Push(c2->m_coord);
                        }
                    }
                    unit->m_31c.RemoveAll();
                }
                // AddTail every route node's coord onto the unit's coord list.
                POSITION qp = list.GetHeadPosition();
                while (qp != 0) {
                    Coord* c3 = static_cast<Coord*>(list.GetNext(qp));
                    if (c3 != 0) {
                        unit->m_31c.AddTail(c3);
                    }
                }
                // Re-clamp the board dirty-rect to the board bounds.
                RECT b1;
                static_cast<RECT*>(new (&b1) CRect(0, 0, board->m_width, board->m_height));
                RECT b2;
                RECT* boardRect =
                    static_cast<RECT*>(new (&b2) CRect(0, 0, board->m_width, board->m_height));
                RECT rc;
                rc.left = boardRect->left;
                rc.top = boardRect->top;
                rc.right = boardRect->right;
                rc.bottom = boardRect->bottom;
                if (!IntersectRect(&board->m_bounds, &rc, &b1)) {
                    board->m_bounds = rc;
                }
                board->m_gridW = board->m_bounds.right - board->m_bounds.left;
                board->m_gridH = board->m_bounds.bottom - board->m_bounds.top;
                Coord* nt = (unit->CoordTail())->m_coord;
                unit->m_entrancePxX = (nt->m_x << 5) + 0x10;
                unit->m_entrancePxY = (nt->m_y << 5) + 0x10;
                list.RemoveAll();
                return 1;
            }
        }
        iter++;
        list.RemoveAll();
    }
    // No route: re-clamp the board dirty-rect to the board bounds.
    RECT f1;
    static_cast<RECT*>(new (&f1) CRect(0, 0, board->m_width, board->m_height));
    RECT f2;
    RECT* pf = static_cast<RECT*>(new (&f2) CRect(0, 0, board->m_width, board->m_height));
    RECT fc;
    fc.left = pf->left;
    fc.top = pf->top;
    fc.right = pf->right;
    fc.bottom = pf->bottom;
    if (!IntersectRect(&board->m_bounds, &fc, &f1)) {
        board->m_bounds = fc;
    }
    board->m_gridW = board->m_bounds.right - board->m_bounds.left;
    board->m_gridH = board->m_bounds.bottom - board->m_bounds.top;
    return 0;
}

// ===========================================================================
// CBattlezMapConfig::FindIdleGruntInBox  @0x02ab80
// Build a RECT centered at (cx,cy) with half-extents (halfW,halfH); scan the
// four cell-bands (15 units each, skipping the current cell band m_curCell) for the
// nearest idle (m_364==0) unit whose grid coord is inside the rect. On a kind-0x36
// unit, keep it only with a ~5% roll. Track the manhattan-nearest; return it (0
// on none). __thiscall(cx,cy,halfW,halfH).
// ===========================================================================
// @early-stop
RVA(0x0002ab80, 0x15e)
CGrunt* CBattlezMapConfig::FindIdleGruntInBox(i32 cx, i32 cy, i32 halfW, i32 halfH) {
    RECT rect;
    rect.left = cx - halfW;
    rect.right = cx + halfW;
    rect.top = cy - halfH;
    rect.bottom = cy + halfH;
    CGrunt* best = 0;
    i32 bestDist = 0x7fffffff;
    for (i32 band = 0; band < 4; band++) {
        if (band == m_curCell) {
            continue;
        }
        for (i32 i = 0; i < 15; i++) {
            CGrunt* u = m_triggerMgr->m_grid[band * 15 + i];
            if (u == 0) {
                continue;
            }
            if (u->m_entranceDropActive != 0) {
                continue;
            }
            CGameObject* lvl = u->m_object;
            POINT wpt;
            wpt.x = lvl->m_screenX >> 5;
            wpt.y = lvl->m_screenY >> 5;
            if (!PtInRect(&rect, wpt)) {
                continue;
            }
            i32 keep = 1;
            if (u->m_gruntKind == 0x36) {
                if (rand() % 100 > 5) {
                    keep = 0;
                }
            }
            if (keep == 0) {
                continue;
            }
            lvl = u->m_object;
            i32 dx = abs((lvl->m_screenX >> 5) - cx);
            i32 dy = abs((lvl->m_screenY >> 5) - cy);
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

// ===========================================================================
// CBattlezMapConfig::PickRandomIdleUnit  @0x02ad40
// Pick a random idle unit from one of the four cell-bands: roll a band [0..3]
// (avoiding the current cell index m_curCell by bumping past it), a random start cell
// [0..14], then scan the band's 15 units from there (cell index wrapping mod 15),
// returning the first non-null unit whose +0x364 "busy" slot is clear (0 on miss).
// The arg is unused. (__thiscall, ret 0x4.)
// ===========================================================================
// @early-stop
RVA(0x0002ad40, 0x71)
void* CBattlezMapConfig::PickRandomIdleUnit(i32) {
    i32 band = rand() % 4;
    if (m_curCell == band) {
        band++;
    }
    band = band % 4;
    i32 cell = rand() % 15;
    CGrunt** row = &m_triggerMgr->m_grid[band * 15];
    for (i32 i = 0; i < 15; i++) {
        CGrunt* u = *row;
        if (u != 0 && u->m_entranceDropActive == 0) {
            return u;
        }
        cell = (cell + 1) % 15;
        row++;
    }
    return 0;
}

// The gated point-in-rect test on a unit (RVA 0x051a20, RectContainsGated): a
// __thiscall taking the other unit's level coord. External, reloc-masked.
// The neighbour-commit hook on a unit (RVA 0x05b050, CommitNeighbor): a __thiscall
// taking (packedA, packedB, coordX, coordY). External, reloc-masked.
// ===========================================================================
// CBattlezMapConfig::winapi_02ae00_IntersectRect  @0x02ae00
// Coord hand-off from `unit` to the target `tgt`. Reject unless `unit` is eligible
// (m_1fc set, its anim name is none of J/C/R/G/L, m_gruntKind != 0x36, m_364 clear). Then,
// when tgt is armed (m_198 != 0) with a 1/4 roll and tgt gates in `unit`'s level
// coord (RectContainsGated), fire the grid trigger (ApplyTriggerB) at tgt's coord
// (m_198==0x1e) or unit's coord and return. Otherwise commit the neighbour
// (CommitNeighbor) and, when tgt's prim anim is 0x11, clamp the board dirty-rect to
// an 11x11 box around tgt (IntersectRect copy-back) and re-path tgt to a random
// nearby cell (RouteUnitTo, flags 0x20000d87). Returns 1 (0 on the eligibility rejects).
// ===========================================================================
RVA(0x0002ade0, 0x7)
void CBattlezMapConfig::Clear() {
    m_active = 0;
}

RVA(0x0002ae00, 0x42e)
i32 CBattlezMapConfig::winapi_02ae00_IntersectRect(CGrunt* unit, CGrunt* tgt) {
    if (unit->m_entranceCommitted == 0) {
        return 0;
    }
    bool eq;
    eq = (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_1c)), "J") == 0);
    if (eq) {
        return 0;
    }
    eq = (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_1c)), "C") == 0);
    if (eq) {
        return 0;
    }
    eq = (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_1c)), "R") == 0);
    if (eq) {
        return 0;
    }
    eq = (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_1c)), "G") == 0);
    if (eq) {
        return 0;
    }
    eq = (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_1c)), "L") == 0);
    if (eq) {
        return 0;
    }
    if (unit->m_gruntKind == 0x36) {
        return 0;
    }
    if (unit->m_entranceDropActive != 0) {
        return 0;
    }
    i32 roll = rand() % 4;
    if (tgt->m_198 != 0 && roll == 0) {
        CGameObject* ul = unit->m_object;
        if ((static_cast<CGrunt*>(tgt))->RectContainsGated(ul->m_screenX, ul->m_screenY) != 0) {
            if (tgt->m_198 == 0x1e) {
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
    i32 prim = tgt->m_entranceReason;
    if (prim > 0x16) {
        prim = tgt->m_toolId;
    }
    if (prim != 0x11) {
        return 1;
    }
    // Clamp the board dirty-rect to an 11x11 box around tgt, then re-path tgt to a
    // random nearby cell.
    CGameObject* tl = tgt->m_object;
    i32 ycoord = (tl->m_screenY >> 5) + rand() % 10 - 5;
    i32 r2 = rand() % 10;
    CGameObject* tl2 = tgt->m_object;
    i32 left = (tl2->m_screenX >> 5) - 5;
    i32 xcoord = (tl->m_screenX >> 5) + r2 - 5;
    i32 right = (tl2->m_screenX >> 5) + 5;
    CMapMgr* board = m_board;
    i32 bottom = (tl2->m_screenY >> 5) + 5;
    i32 top = (tl2->m_screenY >> 5) - 5;
    RECT box;
    box.left = left;
    box.top = top;
    box.right = right + 1;
    box.bottom = bottom + 1;
    RECT bounds;
    bounds.left = 0;
    bounds.top = 0;
    bounds.right = board->m_width;
    bounds.bottom = board->m_height;
    if (!IntersectRect(&board->m_bounds, &box, &bounds)) {
        board->m_bounds = box;
    }
    board->m_gridW = board->m_bounds.right - board->m_bounds.left;
    board->m_gridH = board->m_bounds.bottom - board->m_bounds.top;
    RouteUnitTo(tgt, xcoord, ycoord, 0x20000d87, 0, 0);
    board->Clip(static_cast<const RECT*>(0));
    return 1;
}

RVA(0x0002b420, 0x419)
i32 CBattlezMapConfig::Serialize(void* arArg) {
    CFileMemBase* ar = static_cast<CFileMemBase*>(arArg);
    if (ar == 0) {
        return 0;
    }
    ar->Write(&m_active, 4);
    ar->Write(&m_curCell, 4);
    ar->Write(&m_01c, 4);
    ar->Write(&m_020, 4);
    ar->Write(&m_024, 4);
    ar->Write(&m_028, 4);
    ar->Write(&m_02c, 4);
    ar->Write(&m_spawnPct, 4);
    ar->Write(&m_034, 4);
    ar->Write(&m_038, 4);
    ar->Write(&m_03c, 4);
    ar->Write(&m_040, 4);
    ar->Write(&m_044, 4);
    ar->Write(&m_spawnInterval, 4);
    ar->Write(&m_repickInterval, 4);
    ar->Write(&m_spawnLastFire, 4);
    ar->Write(&m_repickLastFire, 4);
    ar->Write(&m_spawnTimer, 4);
    ar->Write(&m_repickTimer, 4);
    ar->Write(&m_060, 4);
    ar->Write(&m_064, 4);
    ar->Write(&m_068, 4);
    ar->Write(&m_06c, 4);
    ar->Write(&m_070, 4);
    ar->Write(&m_budgetMul, 4);
    ar->Write(&m_088, 4);
    ar->Write(&m_08c, 4);
    ar->Write(&m_090, 4);
    ar->Write(&m_094, 4);
    ar->Write(&m_098, 4);
    ar->Write(&m_09c, 4);
    ar->Write(&m_0a0, 4);
    ar->Write(&m_0a4, 4);
    ar->Write(&m_0a8, 4);
    ar->Write(&m_0ac, 4);
    ar->Write(&m_0b0, 4);
    ar->Write(&m_reserveBudget, 4);
    ar->Write(&m_0b8, 4);
    ar->Write(&m_moveBudget, 4);
    ar->Write(&m_0c0, 4);
    ar->Write(&m_repathBudget, 4);
    ar->Write(&m_0c8, 4);
    ar->Write(&m_0cc, 4);
    ar->Write(&m_0d0, 8);
    ar->Write(&m_0d8, 4);
    ar->Write(&m_13c, 4);
    ar->Write(&m_140, 4);
    ar->Write(&m_144, 4);
    ar->Write(&m_claimTimer, 4);
    ar->Write(&m_14c, 4);

    u32 i;
    u32 n = m_104.GetSize();
    ar->Write(&n, 4);
    for (i = 0; i < n; i++) {
        DWORD v = m_104[i];
        ar->Write(&v, 4);
    }

    n = m_118.GetSize();
    ar->Write(&n, 4);
    for (i = 0; i < n; i++) {
        DWORD v = m_118[i];
        ar->Write(&v, 4);
    }

    i32* p = &m_12c;
    for (i32 k = 0; k < 4; k++) {
        ar->Write(p, 4);
        p++;
    }

    n = m_0f0.GetSize();
    ar->Write(&n, 4);
    for (i = 0; i < n; i++) {
        ar->Write(m_0f0[i], 8);
    }

    n = m_candArray.GetSize();
    ar->Write(&n, 4);
    for (i = 0; i < n; i++) {
        ar->Write(m_candArray[i], 8);
    }
    return 1;
}

// ===========================================================================
// CBattlezMapConfig::Deserialize  @0x02b950
// The read mirror of Serialize: pull every config scalar back out of the
// archive through its Read(buf,count) vtable slot (+0x2c), then the four growable
// arrays + the inline 4-dword block. The two CDWordArrays read a count then that
// many dwords; the two CPtrArrays first recycle their current element nodes onto
// g_coordPool.m_freeHead, resize, then allocate one fresh node per element (8-byte payload).
// ===========================================================================
// @early-stop
RVA(0x0002b950, 0x513)
i32 CBattlezMapConfig::Deserialize(void* arArg) {
    CFileMemBase* ar = static_cast<CFileMemBase*>(arArg);
    if (ar == 0) {
        return 0;
    }
    ar->Read(&m_active, 4);
    ar->Read(&m_curCell, 4);
    ar->Read(&m_01c, 4);
    ar->Read(&m_020, 4);
    ar->Read(&m_024, 4);
    ar->Read(&m_028, 4);
    ar->Read(&m_02c, 4);
    ar->Read(&m_spawnPct, 4);
    ar->Read(&m_034, 4);
    ar->Read(&m_038, 4);
    ar->Read(&m_03c, 4);
    ar->Read(&m_040, 4);
    ar->Read(&m_044, 4);
    ar->Read(&m_spawnInterval, 4);
    ar->Read(&m_repickInterval, 4);
    ar->Read(&m_spawnLastFire, 4);
    ar->Read(&m_repickLastFire, 4);
    ar->Read(&m_spawnTimer, 4);
    ar->Read(&m_repickTimer, 4);
    ar->Read(&m_060, 4);
    ar->Read(&m_064, 4);
    ar->Read(&m_068, 4);
    ar->Read(&m_06c, 4);
    ar->Read(&m_070, 4);
    ar->Read(&m_budgetMul, 4);
    ar->Read(&m_088, 4);
    ar->Read(&m_08c, 4);
    ar->Read(&m_090, 4);
    ar->Read(&m_094, 4);
    ar->Read(&m_098, 4);
    ar->Read(&m_09c, 4);
    ar->Read(&m_0a0, 4);
    ar->Read(&m_0a4, 4);
    ar->Read(&m_0a8, 4);
    ar->Read(&m_0ac, 4);
    ar->Read(&m_0b0, 4);
    ar->Read(&m_reserveBudget, 4);
    ar->Read(&m_0b8, 4);
    ar->Read(&m_moveBudget, 4);
    ar->Read(&m_0c0, 4);
    ar->Read(&m_repathBudget, 4);
    ar->Read(&m_0c8, 4);
    ar->Read(&m_0cc, 4);
    ar->Read(&m_0d0, 8);
    ar->Read(&m_0d8, 4);
    ar->Read(&m_13c, 4);
    ar->Read(&m_140, 4);
    ar->Read(&m_144, 4);
    ar->Read(&m_claimTimer, 4);
    ar->Read(&m_14c, 4);

    u32 i;
    i32 j;
    int count;
    DWORD tmp;

    ar->Read(&count, 4);
    m_104.SetSize(0, -1);
    m_104.SetSize(count, -1);
    for (i = 0; i < static_cast<u32>(count); i++) {
        ar->Read(&tmp, 4);
        m_104[i] = tmp;
    }

    ar->Read(&count, 4);
    m_118.SetSize(0, -1);
    m_118.SetSize(count, -1);
    for (i = 0; i < static_cast<u32>(count); i++) {
        ar->Read(&tmp, 4);
        m_118[i] = tmp;
    }

    i32* p = &m_12c;
    for (i32 k = 0; k < 4; k++) {
        ar->Read(p, 4);
        p++;
    }

    for (j = 0; j < m_0f0.GetSize(); j++) {
        void* q = m_0f0[j];
        if (q != 0) {
            CoordPoolNode* node = g_coordPool.NodeOf(q);
            node->m_next = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = node;
        }
    }
    m_0f0.SetSize(0, -1);
    ar->Read(&count, 4);
    m_0f0.SetSize(count, -1);
    for (i = 0; i < static_cast<u32>(count); i++) {
        CoordPoolNode* node = g_coordPool.m_freeHead;
        void* payload = 0;
        if (node->m_next != 0) {
            payload = &node->m_coord;
            g_coordPool.m_freeHead = node->m_next;
        }
        ar->Read(payload, 8);
        m_0f0[i] = payload;
    }

    for (j = 0; j < m_candArray.GetSize(); j++) {
        void* q = m_candArray[j];
        if (q != 0) {
            CoordPoolNode* node = g_coordPool.NodeOf(q);
            node->m_next = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = node;
        }
    }
    m_candArray.SetSize(0, -1);
    ar->Read(&count, 4);
    m_candArray.SetSize(count, -1);
    for (i = 0; i < static_cast<u32>(count); i++) {
        CoordPoolNode* node = g_coordPool.m_freeHead;
        void* payload = 0;
        if (node->m_next != 0) {
            payload = &node->m_coord;
            g_coordPool.m_freeHead = node->m_next;
        }
        ar->Read(payload, 8);
        m_candArray[i] = payload;
    }
    return 1;
}

RVA(0x0002bfc0, 0x8a)
i32 CBattlezMapConfig::SerializeState(CFileMemBase* objArg, i32 kindArg, i32, i32) {
    CFileMemBase* obj = objArg;
    i32 kind = kindArg;
    switch (kind) {
        case 4:
            if (this->Serialize(obj) == 0) { // kind-4 validator @0x2b420
                return 0;
            }
            break;
        case 7:
            if (this->Deserialize(obj) == 0) { // kind-7 validator @0x2b950
                return 0;
            }
            break;
    }
    // the two 8-byte scratch pairs (+0x78/+0x7c and +0x80/+0x84) stream as themselves;
    // Read/Write take a void* so no cast is needed.
    switch (kind) {
        case 4:
            obj->Write(&m_scratch78, 8);
            obj->Write(&m_scratch80, 8);
            break;
        case 7:
            obj->Read(&m_scratch78, 8);
            obj->Read(&m_scratch80, 8);
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
    if (unit->m_defenderState == 3) {
        return 1;
    }
    m_claimTimer = 0;
    unit->m_defenderState = 3;
    unit->m_2e4 = value;
    CGrunt** units = m_triggerMgr->m_grid + m_curCell * 15;
    i32 count = 0;
    for (i32 k = 0; k < 15; k++) {
        CGrunt* p = units[k];
        if (p != 0 && unit != p && p->m_defenderState == 3) {
            count++;
        }
    }
    unit->m_2e0 = count;
    return 1;
}

// ===========================================================================
// CBattlezMapConfig::winapi_02c140_IntersectRect_PtInRect  @0x02c140
// For an idle unit (m_gruntKind==0, prim anim clear) clamp the board dirty-rect to an 8x8
// box centered on the unit (four GetCoord corner reads + the IntersectRect copy-back
// idiom), then iterate the scene collection (m_ctx->m_world->m_childGroup) for a kind-matching
// (m_7c handler == 0x40288d), non-flagged (m_40&1), in-box unit; on the first such
// unit re-path this unit toward it (RouteUnitTo, flags 0x2000098b), re-clamp the
// dirty-rect, and return 1. Exhausting the collection tails into board Clip + return 0.
// ===========================================================================
// @early-stop
RVA(0x0002c140, 0x420)
i32 CBattlezMapConfig::winapi_02c140_IntersectRect_PtInRect(CGrunt* unit) {
    if (unit->m_gruntKind != 0) {
        return 0;
    }
    i32 prim = unit->m_entranceReason;
    if (prim > 0x16) {
        prim = unit->m_toolId;
    }
    if (prim != 0) {
        return 0;
    }
    // Build an 8x8 box around the unit (four GetCoord corner reads).
    RECT box;
    Coord c1;
    (static_cast<CUserLogic*>(unit))->GetScreenPos((&c1));
    box.bottom = (c1.m_y >> 5) + 4;
    Coord c2;
    (static_cast<CUserLogic*>(unit))->GetScreenPos((&c2));
    box.right = (c2.m_x >> 5) + 4;
    Coord c3;
    (static_cast<CUserLogic*>(unit))->GetScreenPos((&c3));
    box.top = (c3.m_y >> 5) - 3;
    Coord c4;
    (static_cast<CUserLogic*>(unit))->GetScreenPos((&c4));
    box.left = (c4.m_x >> 5) - 3;
    CMapMgr* board = m_board;
    RECT bounds;
    static_cast<RECT*>(new (&bounds) CRect(0, 0, board->m_width, board->m_height));
    RECT clamp;
    clamp.left = box.left;
    clamp.top = box.top;
    clamp.right = box.right + 1;
    clamp.bottom = box.bottom + 1;
    if (!IntersectRect(&board->m_bounds, &clamp, &bounds)) {
        board->m_bounds = clamp;
    }
    board->m_gridW = board->m_bounds.right - board->m_bounds.left;
    board->m_gridH = board->m_bounds.bottom - board->m_bounds.top;
    // Iterate the scene collection for kind-matching units inside the box.
    CDDrawChildGroup* coll = m_ctx->m_world->m_childGroup;
    coll->m_scanCursor = coll->m_list.GetHeadPosition();
    CGameObject* g = static_cast<CGameObject*>(coll->Drain());
    while (g != 0) {
        if (g->m_7c->m_notify == &CreateInGameIcon && (g->m_stateFlags & 1) == 0) {
            i32 special = 0;
            switch (g->m_124) {
                case 0x33:
                case 0x34:
                case 0x35:
                case 0x36:
                case 0x37:
                case 0x38:
                case 0x39:
                case 0x3a:
                case 0x3b:
                case 0x3c:
                case 0x3d:
                case 0x3e:
                case 0x3f:
                case 0x40:
                    special = 1;
                    break;
            }
            i32 gx = g->m_screenX >> 5;
            i32 gy = g->m_screenY >> 5;
            POINT wpt;
            wpt.x = gx;
            wpt.y = gy;
            if (PtInRect(&box, wpt)) {
                if (special != 0 && unit->m_gruntKind == 0) {
                    if (RouteUnitTo(unit, gx, gy, 0x2000098b, 0, 0) != 0) {
                        CMapMgr* bd = m_board;
                        RECT mb;
                        mb.left = 0;
                        mb.top = 0;
                        mb.right = bd->m_width;
                        mb.bottom = bd->m_height;
                        RECT tmp;
                        RECT* p =
                            static_cast<RECT*>(new (&tmp) CRect(0, 0, bd->m_width, bd->m_height));
                        RECT bx;
                        bx.left = p->left;
                        bx.top = p->top;
                        bx.right = p->right;
                        bx.bottom = p->bottom;
                        if (!IntersectRect(&bd->m_bounds, &bx, &mb)) {
                            bd->m_bounds = bx;
                        }
                        bd->m_gridW = bd->m_bounds.right - bd->m_bounds.left;
                        bd->m_gridH = bd->m_bounds.bottom - bd->m_bounds.top;
                        return 1;
                    }
                } else {
                    i32 entranceMode = unit->m_entranceReason;
                    if (entranceMode > 0x16) {
                        entranceMode = unit->m_toolId;
                    }
                    if (entranceMode == 0) {
                        if (RouteUnitTo(unit, gx, gy, 0x2000098b, 0, 0) != 0) {
                            CMapMgr* bd = m_board;
                            RECT r1;
                            static_cast<RECT*>(new (&r1) CRect(0, 0, bd->m_width, bd->m_height));
                            RECT r2;
                            RECT* p2r =
                                static_cast<RECT*>(new (&r2)
                                                       CRect(0, 0, bd->m_width, bd->m_height));
                            RECT rc;
                            rc.left = p2r->left;
                            rc.top = p2r->top;
                            rc.right = p2r->right;
                            rc.bottom = p2r->bottom;
                            if (!IntersectRect(&bd->m_bounds, &rc, &r1)) {
                                bd->m_bounds = rc;
                            }
                            bd->m_gridW = bd->m_bounds.right - bd->m_bounds.left;
                            bd->m_gridH = bd->m_bounds.bottom - bd->m_bounds.top;
                            return 1;
                        }
                    }
                }
            }
        }
        // Back-edge: the inlined first GetNext pop, tail-continuing via the helper.
        CDDrawChildGroup* c = m_ctx->m_world->m_childGroup;
        g = 0;
        if (c->m_scanCursor != 0) {
            // the inlined Drain copy: retail reads the cursor twice here (peek then
            // advance), unlike Drain itself - GetAt + GetNext, measured +0.4%
            CGameObject* pp = static_cast<CGameObject*>(c->m_list.GetAt(c->m_scanCursor));
            c->m_list.GetNext(c->m_scanCursor);
            if (pp->GetClassId() == CLASSID_SERIALREF) {
                g = pp;
            } else {
                g = static_cast<CGameObject*>(c->Drain());
            }
        }
    }
    board->Clip(static_cast<const RECT*>(0));
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
        (g)->m_31c.RemoveAll();                                                                    \
    }

static __inline i32 arrCell(CMapMgr* grid, i32 col, i32 row) {
    if (static_cast<u32>(col) < static_cast<u32>(grid->m_width)
        && static_cast<u32>(row) < static_cast<u32>(grid->m_height)) {
        return grid->m_rows[row][col].m_0;
    }
    return 1;
}

// @early-stop
RVA(0x0002c690, 0xdb4)
i32 CBattlezMapConfig::ResolveArrival(CGrunt* g) {
    if (RepathAroundBlockedTiles(g)) {
        return 1;
    }
    if (g->CoordCount() == 0) {
        return 0;
    }

    Coord* fc = g->CoordHead()->m_coord;
    i32 fcx = fc->m_x; // grunt head coord x (long-lived)
    i32 fcy = fc->m_y; // grunt head coord y

    Coord a;
    g->GetTilePos(&a);
    i32 gy = a.m_y >> 5;
    i32 gx = a.m_x >> 5;
    Coord b;
    g->GetTilePos(&b);
    i32 bx = b.m_x >> 5;

    // destination cell = grid[gy][bx]; OOB fills the dest buffer itself (self-copy).
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

    // own cell = grid[fcy][fcx], the grunt's head pending coordinate.
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

    i32 maskFlags = own.m_0 & 0xdfffffff;
    i32 type = (g->m_entranceReason > 0x16) ? g->m_toolId : g->m_entranceReason;

    // ---- door (dest.flags & 0x400, m_2d4==3, type!=8) ----
    if ((dest.m_0 & 0x400) && g->m_defenderState == 3 && type != 8) {
        if (own.m_0 & 0x4000) {
            // door-open transform: build a search rect from the grunt pos, then a per-cell
            // CString-EH loop recycling edge nodes onto g_coordPool.m_freeHead. (Byte-walled: retail
            // emits a dead stack-address null-recheck our MSVC5 DCEs - see @early-stop.)
            Coord da;
            g->GetTilePos(&da);
            for (i32 drow = m_board->m_bounds.top; drow < m_board->m_bounds.bottom; drow++) {
                for (i32 dcol = m_board->m_bounds.left; dcol < m_board->m_bounds.right; dcol++) {
                    CPtrList cs(0xa); // a real MFC CPtrList (the stack instance forces /GX)
                    if (!(m_board->m_rows[drow][dcol].m_0 & 0x20000000)) {
                        void* h = cs.RemoveHead();
                        if (h != 0) {
                            CoordPoolNode* node = g_coordPool.NodeOf(h);
                            node->m_next = g_coordPool.m_freeHead;
                            g_coordPool.m_freeHead = node;
                        }
                    }
                }
            }
        }
        // recompute the grid clip region (IntersectRect with the full-grid rect)
        CRect full(0, 0, m_board->m_width, m_board->m_height);
        CRect corners(0, 0, m_board->m_width, m_board->m_height);
        RECT tmp;
        tmp.left = corners.left;
        tmp.top = corners.top;
        tmp.right = corners.right;
        tmp.bottom = corners.bottom;
        if (!IntersectRect(
                &m_board->m_bounds,
                static_cast<RECT*>(&tmp),
                static_cast<RECT*>(&corners)
            )) {
            m_board->m_bounds = tmp;
        }
        m_board->m_gridW = m_board->m_bounds.right - m_board->m_bounds.left;
        m_board->m_gridH = m_board->m_bounds.bottom - m_board->m_bounds.top;
    }

    // ---- door body flag (dest.flags & 4) ----
    if ((dest.m_0 & 4) && g->m_2d8 != 0xb) {
        Coord tp;
        i32 keyHi = g->m_object->m_screenX >> 5;
        g->GetTilePos(&tp);
        i32 key = (keyHi << 8) + (tp.m_y >> 5);
        static_cast<void>((tp.m_x >> 5));
        CTileTriggerSwitchLogic* r = m_cellQuery->FindChild(key, 0);
        if (r->m_typeId == 2) {
            g->m_defenderState = 0;
            ARR_RECYCLE(g);
            g->m_2d8 = 0xb;
            g->m_dwell = 0;
            return 0;
        }
    }

    // ---- 0x8000 gate, type 3 ----
    if ((maskFlags & 0x8000) && type == 3 && g->m_2d8 == 0xa) {
        m_triggerMgr->ApplyTriggerA(
            g->m_tileOwnerHi,
            g->m_tileOwnerLo,
            (fcx << 5) + 0x10,
            (fcy << 5) + 0x10
        );
        ARR_RECYCLE(g);
        return 0;
    }

    // ---- 0x4000 gate, type 3 ----
    if ((maskFlags & 0x4000) && type == 3 && g->m_2d8 == 0xa) {
        if (m_board->m_rows[fcy][fcx].m_10 != 0x99) {
            m_triggerMgr->ApplyTriggerA(
                g->m_tileOwnerHi,
                g->m_tileOwnerLo,
                (fcx << 5) + 0x10,
                (fcy << 5) + 0x10
            );
        }
        ARR_RECYCLE(g);
        return 0;
    }

    // ---- 0x200 -> done ----
    if (maskFlags & 0x200) {
        return 1;
    }

    // ---- 0x8 -> probe/effect ----
    if (maskFlags & 0x8) {
        if (PathToNearestGoal(g, fcx, fcy) != 0) {
            return 1;
        }
        EnterDefenderMode(g, 0x12);
    }

    // ---- 0x20 -> type dispatch ----
    if (maskFlags & 0x20) {
        i32 t = (g->m_entranceReason > 0x16) ? g->m_toolId : g->m_entranceReason;
        if (t == 1 || t == 0x11) {
            if (t == 1) {
                m_triggerMgr->ApplyTriggerA(
                    g->m_tileOwnerHi,
                    g->m_tileOwnerLo,
                    (fcx << 5) + 0x10,
                    (fcy << 5) + 0x10
                );
                return 1;
            }
            // t == 0x11: scan the 3x3 neighbourhood for the first in-bounds cell
            for (i32 row = fcy - 1; row < fcy + 2; row++) {
                for (i32 col = fcx - 1; col < fcx + 2; col++) {
                    if (static_cast<u32>(col) < static_cast<u32>(m_board->m_width)
                        && static_cast<u32>(row) < static_cast<u32>(m_board->m_height)) {
                        i32 cf = arrCell(m_board, col, row);
                        if (cf & 0x939) {
                            return 1;
                        }
                        if (g->RectContains((col << 5) + 0x10, (row << 5) + 0x10) != 0) {
                            m_triggerMgr->ApplyTriggerA(
                                g->m_tileOwnerHi,
                                g->m_tileOwnerLo,
                                (col << 5) + 0x10,
                                (row << 5) + 0x10
                            );
                        }
                        return 1;
                    }
                }
            }
        }
    }

    // ---- 0x4000 path, type 0xf (teleport) ----
    if (maskFlags & 0x4000) {
        i32 t = (g->m_entranceReason > 0x16) ? g->m_toolId : g->m_entranceReason;
        if (t == 0xf) {
            CTileActionEvent* r = m_cellQuery->FindByField0C((fcx << 8) + fcy);
            if (r != 0) {
                if (r->m_playerFlags[m_curCell] != 0) {
                    ARR_RECYCLE(g);
                    winapi_02dfa0_IntersectRect(g, fcx, fcy, 1);
                    return 1;
                }
                m_triggerMgr->ApplyTriggerA(
                    g->m_tileOwnerHi,
                    g->m_tileOwnerLo,
                    (fcx << 5) + 0x10,
                    (fcy << 5) + 0x10
                );
                return 1;
            }
        }
    }

    // ---- 0x8000 path, type 0xf ----
    if (maskFlags & 0x8000) {
        i32 t = (g->m_entranceReason > 0x16) ? g->m_toolId : g->m_entranceReason;
        if (t == 0xf) {
            ARR_RECYCLE(g);
            winapi_02dfa0_IntersectRect(g, fcx, fcy, 1);
            return 1;
        }
    }

    // ---- 0x20 path, type 5 ----
    if (maskFlags & 0x20) {
        i32 t = (g->m_entranceReason > 0x16) ? g->m_toolId : g->m_entranceReason;
        if (t == 5) {
            if (maskFlags & 0x4000) {
                CTileActionEvent* r = m_cellQuery->FindByField0C((fcx << 8) + fcy);
                if (r != 0) {
                    i32 k = r->m_actionCode;
                    if (r->m_playerFlags[m_curCell] != 0) {
                        if (k == 0x13e || k == 0x140 || k == 0x143) {
                            winapi_02dfa0_IntersectRect(g, fcx, fcy, 0);
                        }
                    } else {
                        if (k == 0x13e || k == 0x140 || k == 0x143) {
                            m_cellQuery->SetCell(fcx, fcy, m_curCell);
                        }
                    }
                }
            }
            m_triggerMgr->ApplyTriggerA(
                g->m_tileOwnerHi,
                g->m_tileOwnerLo,
                (fcx << 5) + 0x10,
                (fcy << 5) + 0x10
            );
            return 0;
        }
        if (t == 0x11 || t == 1) {
            return 1;
        }
        i32 flag = 1;
        if (t == 3 && (maskFlags & 0x4000)) {
            flag = 0;
        }
        if (t == 0xf && (maskFlags & 0x4000)) {
            flag = 0;
        }
        if (flag == 0) {
            return 1;
        }
        EnterDefenderMode(g, 5);
        return 0;
    }

    // ---- 0x40 path ----
    if (maskFlags & 0x40) {
        i32 t = (g->m_entranceReason > 0x16) ? g->m_toolId : g->m_entranceReason;
        if (t != 0x16) {
            i32 t2 = (g->m_entranceReason > 0x16) ? g->m_toolId : g->m_entranceReason;
            if (t2 == 0xd) {
                m_triggerMgr->ApplyTriggerA(
                    g->m_tileOwnerHi,
                    g->m_tileOwnerLo,
                    (fcx << 5) + 0x10,
                    (fcy << 5) + 0x10
                );
                return 0;
            }
            EnterDefenderMode(g, 0xd);
            return 0;
        }
    }

    // ---- neighbour pick fallback ----
    PathToNearestCandidate(g, 0, 0, 0);
    if (PathCrossesMarkedTile(g) != 0) {
        return 1;
    }
    {
        i32 t = (g->m_entranceReason > 0x16) ? g->m_toolId : g->m_entranceReason;
        if (t == 0x16) {
            return 1;
        }
    }
    {
        i32 oy = g->m_object->m_screenY >> 5;
        i32 ox = g->m_object->m_screenX >> 5;
        i32 row = rand() % 3 + oy - 1;
        i32 col = rand() % 3 + ox - 1;
        if (static_cast<u32>(col) >= static_cast<u32>(m_board->m_width)
            || static_cast<u32>(row) >= static_cast<u32>(m_board->m_height)) {
            return 1;
        }
        i32 c0 = arrCell(m_board, col, row);
        i32 c1 = arrCell(m_board, col, row);
        if ((c1 & 0x987) & 0x20000000) {
            return 1;
        }
        if (c1 & 0x987) {
            return 1;
        }
        if (c0 & 0x20000000) {
            return 1;
        }
        g->TileSwitch(
            col,
            row,
            0x987,
            0,
            1,
            0
        ); // @0x4b320 (thiscall spelling: retail's dead mov ecx)
    }
    return 1;
}

#undef ARR_RECYCLE

// @early-stop
RVA(0x0002d800, 0x605)
void CBattlezMapConfig::ClaimTilesAround(CGrunt* unit, i32 col, i32 row, i32 requireUnoccupied) {
    while (g_stepRun != 0) {
        // Retail holds NO tile pointer across the body: it CSEs only the byte offset
        // col*28 ([esp+0x10]) and re-derives m_board->m_rows[row] at each use.
        i32 word = m_board->m_rows[row][col].m_0;
        if (word & 0x8000) {
            CPtrList list(10);
            CGameObject* lvl = unit->m_object;
            if ((m_board)->SearchEdge(
                    lvl->m_screenX >> 5,
                    lvl->m_screenY >> 5,
                    col,
                    row,
                    &list,
                    1,
                    0x4903,
                    0
                )
                != 0) {
                void* head = list.GetHeadPosition();
                g_stepRun = 0;
                g_stepCol = col;
                g_stepRow = row;
                if (head != 0) {
                    CoordNode* n = static_cast<CoordNode*>(head);
                    while (n != 0) {
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
            CTileActionEvent* cell = m_cellQuery->FindByField0C((col << 8) + row);
            if (requireUnoccupied != 0) {
                if (cell != 0 && cell->m_playerFlags[m_curCell] == 0) {
                    CPtrList list2(10);
                    CGameObject* lvl = unit->m_object;
                    if ((m_board)->SearchEdge(
                            lvl->m_screenX >> 5,
                            lvl->m_screenY >> 5,
                            col,
                            row,
                            &list2,
                            1,
                            0x4003,
                            0
                        )
                        != 0) {
                        void* head = list2.GetHeadPosition();
                        g_stepRun = 0;
                        g_stepCol = col;
                        g_stepRow = row;
                        if (head != 0) {
                            CoordNode* n = static_cast<CoordNode*>(head);
                            while (n != 0) {
                                CoordNode* cur = n;
                                n = n->m_next;
                                CoordPoolNode* node = g_coordPool.NodeOf(cur->m_coord);
                                node->m_next = g_coordPool.m_freeHead;
                                g_coordPool.m_freeHead = node;
                            }
                        }
                    }
                }
            } else if (cell != 0) {
                i32 id = cell->m_actionCode;
                i32 special = 0;
                i32 occ = cell->m_playerFlags[m_curCell];
                if (occ == 0) {
                    special = 1;
                } else if (id == 0x132 || id == 0x134 || id == 0x137 || id == 0x144 || id == 0x146
                           || id == 0x149 || id == 0x138 || id == 0x13a || id == 0x13d
                           || id == 0x12f || id == 0x130 || id == 0x131) {
                    special = 1;
                }
                if (special != 0) {
                    CPtrList list3(10);
                    CGameObject* lvl = unit->m_object;
                    if ((m_board)->SearchEdge(
                            lvl->m_screenX >> 5,
                            lvl->m_screenY >> 5,
                            col,
                            row,
                            &list3,
                            1,
                            0x4003,
                            0
                        )
                        != 0) {
                        void* head = list3.GetHeadPosition();
                        g_stepRun = 0;
                        g_stepCol = col;
                        g_stepRow = row;
                        if (head != 0) {
                            CoordNode* n = static_cast<CoordNode*>(head);
                            while (n != 0) {
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
        // Mark this tile visited, then recurse into the 8 neighbours. Each block:
        // in bounds + not visited (0x20000) + passable (0xc0000 set or anim 0x9a).
        m_board->m_rows[row][col].m_0 |= 0x20000;
        i32 cm = col - 1;
        i32 cp = col + 1;
        i32 rm = row - 1;
        i32 rp = row + 1;
        CMapMgr* b;
        BrickzCell* nt;
        i32 nw;

        b = m_board;
        if (static_cast<u32>(cm) < static_cast<u32>(b->m_width)) {
            nt = &b->m_rows[row][cm];
            nw = nt->m_0;
            if (!(nw & 0x20000) && ((nw & 0xc000) || nt->m_10 == 0x9a)) {
                ClaimTilesAround(unit, cm, row, requireUnoccupied);
            }
        }
        b = m_board;
        if (static_cast<u32>(cp) < static_cast<u32>(b->m_width)) {
            nt = &b->m_rows[row][cp];
            nw = nt->m_0;
            if (!(nw & 0x20000) && ((nw & 0xc000) || nt->m_10 == 0x9a)) {
                ClaimTilesAround(unit, cp, row, requireUnoccupied);
            }
        }
        b = m_board;
        if (static_cast<u32>(rm) < static_cast<u32>(b->m_width)) {
            nt = &b->m_rows[rm][col];
            nw = nt->m_0;
            if (!(nw & 0x20000) && ((nw & 0xc000) || nt->m_10 == 0x9a)) {
                ClaimTilesAround(unit, col, rm, requireUnoccupied);
            }
        }
        b = m_board;
        if (static_cast<u32>(rp) < static_cast<u32>(b->m_width)) {
            nt = &b->m_rows[rp][col];
            nw = nt->m_0;
            if (!(nw & 0x20000) && ((nw & 0xc000) || nt->m_10 == 0x9a)) {
                ClaimTilesAround(unit, col, rp, requireUnoccupied);
            }
        }
        b = m_board;
        if (static_cast<u32>(cp) < static_cast<u32>(b->m_width)
            && static_cast<u32>(rm) < static_cast<u32>(b->m_height)) {
            nt = &b->m_rows[rm][cp];
            nw = nt->m_0;
            if (!(nw & 0x20000) && ((nw & 0xc000) || nt->m_10 == 0x9a)) {
                ClaimTilesAround(unit, cp, rm, requireUnoccupied);
            }
        }
        b = m_board;
        if (static_cast<u32>(cp) < static_cast<u32>(b->m_width)
            && static_cast<u32>(rp) < static_cast<u32>(b->m_height)) {
            nt = &b->m_rows[rp][cp];
            nw = nt->m_0;
            if (!(nw & 0x20000) && ((nw & 0xc000) || nt->m_10 == 0x9a)) {
                ClaimTilesAround(unit, cp, rp, requireUnoccupied);
            }
        }
        b = m_board;
        if (static_cast<u32>(cm) < static_cast<u32>(b->m_width)
            && static_cast<u32>(rp) < static_cast<u32>(b->m_height)) {
            nt = &b->m_rows[rp][cm];
            nw = nt->m_0;
            if (!(nw & 0x20000) && ((nw & 0xc000) || nt->m_10 == 0x9a)) {
                ClaimTilesAround(unit, cm, rp, requireUnoccupied);
            }
        }
        // The 8th neighbour (cm,rm) is NOT a call: retail tail-iterates it. Its guards
        // jump straight to the epilogue (0x2ddf0) and the back-edge at 0x2dd99 sets
        // ebp=cm / edi=rm before re-testing g_stepRun, so failing a guard here RETURNS.
        b = m_board;
        if (static_cast<u32>(cm) >= static_cast<u32>(b->m_width)
            || static_cast<u32>(rm) >= static_cast<u32>(b->m_height)) {
            break;
        }
        nt = &b->m_rows[rm][cm];
        nw = nt->m_0;
        if ((nw & 0x20000) || (!(nw & 0xc000) && nt->m_10 != 0x9a)) {
            break;
        }
        row = rm;
        col = cm;
    }
    // No `return` anywhere: retail never zeroes eax - every exit falls straight into the
    // 0x2ddf0 epilogue with whatever eax holds (g_stepRun on the two loop-test paths).
}

// ===========================================================================
// CBattlezMapConfig::winapi_02dfa0_IntersectRect  @0x02dfa0
// The flood-fill launcher. Arm g_stepRun, build a 17x17 box around the unit's
// (>>5) coord (three GetCoord reads for the corners), clamp the board dirty-rect
// to that box intersected with the board bounds (the IntersectRect copy-back
// idiom), then run the recursive flood-fill (ClaimTilesAround). If it committed
// (g_stepRun cleared), read the tile under the unit (and, when it has a live coord
// list, the tile under its tail coord); when a blocked (bit 0x4) tile is seen,
// stamp the unit's packed coord and place it at the committed cell (Method_4b320,
// g_stepCol/g_stepRow, flags 0x9c3). Finally clear the 0x2 bit across the dirty
// region and re-clamp the board dirty-rect to the board bounds.
// ===========================================================================
// @early-stop
RVA(0x0002dfa0, 0x325)
i32 CBattlezMapConfig::winapi_02dfa0_IntersectRect(
    CGrunt* unit,
    i32 col,
    i32 row,
    i32 requireUnoccupied
) {
    g_stepRun = 1;
    // Build a 17x17 box (corner reads via three GetCoords).
    CGameObject* lvl = unit->m_object;
    i32 bottom = (lvl->m_screenY >> 5) + 8;
    Coord g0;
    (static_cast<CUserLogic*>(unit))->GetScreenPos((&g0));
    i32 right = (g0.m_x >> 5) + 8;
    Coord g1;
    (static_cast<CUserLogic*>(unit))->GetScreenPos((&g1));
    i32 top = (g1.m_y >> 5) - 8;
    Coord g2;
    (static_cast<CUserLogic*>(unit))->GetScreenPos((&g2));
    i32 left = (g2.m_x >> 5) - 8;
    CMapMgr* board = m_board;
    RECT bounds;
    static_cast<RECT*>(new (&bounds) CRect(0, 0, board->m_width, board->m_height));
    RECT box;
    box.left = left;
    box.top = top;
    box.right = right + 1;
    box.bottom = bottom + 1;
    if (!IntersectRect(&board->m_bounds, &box, &bounds)) {
        board->m_bounds = box;
    }
    board->m_gridW = board->m_bounds.right - board->m_bounds.left;
    board->m_gridH = board->m_bounds.bottom - board->m_bounds.top;
    ClaimTilesAround(unit, col, row, requireUnoccupied);
    if (g_stepRun == 0) {
        i32 savedX = unit->m_entrancePxX;
        i32 savedY = unit->m_entrancePxY;
        i32 col = unit->m_entrancePxX >> 5;
        i32 row = unit->m_entrancePxY >> 5;
        i32 tile0;
        if (static_cast<u32>(col) < static_cast<u32>(board->m_width)
            && static_cast<u32>(row) < static_cast<u32>(board->m_height)) {
            tile0 = board->m_rowInts[row][col * 7];
        } else {
            tile0 = 1;
        }
        i32 flag = (tile0 >> 2) & 1;
        if (unit->CoordCount() != 0) {
            Coord* c = (unit->CoordTail())->m_coord;
            i32 cx = c->m_x;
            i32 cy = c->m_y;
            i32 tile1;
            if (static_cast<u32>(cx) < static_cast<u32>(board->m_width)
                && static_cast<u32>(cy) < static_cast<u32>(board->m_height)) {
                tile1 = board->m_rowInts[cy][cx * 7];
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
            unit->m_entrancePxX = savedX;
            unit->m_entrancePxY = savedY;
        }
    }
    // Clear bit 0x2 across the board dirty region.
    i32 dl = board->m_bounds.left;
    i32 dt = board->m_bounds.top;
    i32 dr = board->m_bounds.right;
    i32 db = board->m_bounds.bottom;
    if (dl < dr) {
        i32 colOff = (dl * 7) << 2;
        for (i32 w = dr - dl; w != 0; w--) {
            for (i32 r = dt; r < db; r++) {
                board->m_rowBytes[r][colOff + 2] &= 0xfd;
            }
            colOff += 0x1c;
        }
    }
    // Re-clamp the board dirty-rect to the board bounds (inline rect init).
    RECT fa;
    fa.left = 0;
    fa.top = 0;
    fa.right = board->m_width;
    fa.bottom = board->m_height;
    RECT fb;
    fb.left = 0;
    fb.top = 0;
    fb.right = board->m_width;
    fb.bottom = board->m_height;
    if (!IntersectRect(&board->m_bounds, &fa, &fb)) {
        board->m_bounds = fa;
    }
    board->m_gridW = board->m_bounds.right - board->m_bounds.left;
    board->m_gridH = board->m_bounds.bottom - board->m_bounds.top;
    return 1;
}

// ===========================================================================
// CBattlezMapConfig::winapi_02e3a0_PtInRect  @0x02e3a0
// The nearest-idle-neighbour retarget. Build a 15x15 box (half-extent 7) around
// the arg unit's screen coord (4 GetCoord corners), then scan the four cell-bands
// (15 units each, skipping the current band m_curCell) for the eligible (m_1fc set,
// m_368/m_bandADiv/m_220 clear, anim name not C/R/J/G/L, m_gruntKind != 0x36) unit whose
// grid coord is inside the box, keeping the manhattan-distance-squared nearest.
// If one is found (and the arg unit's m_dwell cooldown > 0x64), clamp the board
// dirty-rect to that box, build the FindPath flag word from the unit's 0x12/0x16/
// 0xe anim modes, and re-path the unit toward it (RouteUnitTo, flags 0x1000d8f).
// On a route, debounce the m_390 latch (a g_frameTime window against m_scratch78..m_084,
// firing the scene hit when the unit's level coord is on-screen), re-clamp the
// board dirty-rect, and return 1. No candidate latches m_390 and returns 0.
// ===========================================================================
// @early-stop
RVA(0x0002e3a0, 0x7e1)
i32 CBattlezMapConfig::winapi_02e3a0_PtInRect(CGrunt* unit) {
    // Four GetCoord corners -> a 15x15 box (half-extent 7) around the unit.
    RECT box;
    Coord cA;
    (static_cast<CUserLogic*>(unit))->GetScreenPos((&cA));
    cA.m_x >>= 5;
    cA.m_y >>= 5;
    box.bottom = cA.m_y + 7;
    Coord cB;
    (static_cast<CUserLogic*>(unit))->GetScreenPos((&cB));
    cB.m_x >>= 5;
    cB.m_y >>= 5;
    box.right = cB.m_x + 7;
    Coord cC;
    (static_cast<CUserLogic*>(unit))->GetScreenPos((&cC));
    cC.m_x >>= 5;
    cC.m_y >>= 5;
    box.top = cC.m_y - 7;
    Coord cD;
    (static_cast<CUserLogic*>(unit))->GetScreenPos((&cD));
    box.left = (cD.m_x >> 5) - 7;

    CGrunt* best = 0;
    i32 bestDist = 0x7fffffff;
    for (i32 band = 0; band < 4; band++) {
        if (band == m_curCell) {
            continue;
        }
        for (i32 i = 0; i < 15; i++) {
            CGrunt* u = m_triggerMgr->m_grid[band * 15 + i];
            if (u == 0) {
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
            ne = strcmp((*g_typeColl.GetNameRecord(u->m_objAux->m_1c)), "C") != 0;
            if (!ne) {
                continue;
            }
            ne = strcmp((*g_typeColl.GetNameRecord(u->m_objAux->m_1c)), "R") != 0;
            if (!ne) {
                continue;
            }
            ne = strcmp((*g_typeColl.GetNameRecord(u->m_objAux->m_1c)), "J") != 0;
            if (!ne) {
                continue;
            }
            ne = strcmp((*g_typeColl.GetNameRecord(u->m_objAux->m_1c)), "G") != 0;
            if (!ne) {
                continue;
            }
            ne = strcmp((*g_typeColl.GetNameRecord(u->m_objAux->m_1c)), "L") != 0;
            if (!ne) {
                continue;
            }
            if (u->m_gruntKind == 0x36) {
                continue;
            }
            Coord c;
            (static_cast<CUserLogic*>(u))->GetScreenPos((&c));
            POINT wpt;
            wpt.x = c.m_x >> 5;
            wpt.y = c.m_y >> 5;
            if (!PtInRect(&box, wpt)) {
                continue;
            }
            Coord unitPos1;
            (static_cast<CUserLogic*>(unit))->GetScreenPos((&unitPos1));
            Coord b1;
            (static_cast<CUserLogic*>(u))->GetScreenPos((&b1));
            i32 dx = abs((unitPos1.m_x >> 5) - (b1.m_x >> 5));
            Coord unitPos2;
            (static_cast<CUserLogic*>(unit))->GetScreenPos((&unitPos2));
            Coord b2;
            (static_cast<CUserLogic*>(u))->GetScreenPos((&b2));
            i32 dy = abs((unitPos2.m_y >> 5) - (b2.m_y >> 5));
            i32 dist = dx * dx + dy * dy;
            if (dist >= bestDist) {
                continue;
            }
            bestDist = dist;
            best = u;
        }
    }
    if (best == 0) {
        unit->m_390 = 1;
        return 0;
    }
    if (static_cast<u32>(unit->m_dwell) <= 0x64) {
        return 1;
    }
    CMapMgr* board = m_board;
    RECT bounds;
    static_cast<RECT*>(new (&bounds) CRect(0, 0, board->m_width, board->m_height));
    RECT* boxp = &box;
    RECT rc;
    if (boxp != 0) {
        rc.left = box.left;
        rc.top = box.top;
        rc.right = box.right + 1;
        rc.bottom = box.bottom + 1;
    } else {
        RECT r0;
        RECT* boardRect =
            static_cast<RECT*>(new (&r0) CRect(0, 0, board->m_width, board->m_height));
        rc.left = boardRect->left;
        rc.top = boardRect->top;
        rc.right = boardRect->right;
        rc.bottom = boardRect->bottom;
    }
    if (!IntersectRect(&board->m_bounds, &rc, &bounds)) {
        board->m_bounds = rc;
    }
    board->m_gridW = board->m_bounds.right - board->m_bounds.left;
    board->m_gridH = board->m_bounds.bottom - board->m_bounds.top;
    // FindPath flag word from the unit's 0x12 / 0x16 / 0xe anim modes.
    i32 flags = 0;
    i32 prim = unit->m_entranceReason;
    i32 t = prim;
    if (prim > 0x16) {
        t = unit->m_toolId;
    }
    if (t == 0x12) {
        flags = 0x100;
    }
    t = prim;
    if (prim > 0x16) {
        t = unit->m_toolId;
    }
    if (t == 0x16) {
        flags = 0x942;
    }
    if (prim > 0x16) {
        prim = unit->m_toolId;
    }
    if (prim == 0xe) {
        flags = 0x1000;
    }
    Coord bc;
    (static_cast<CUserLogic*>(best))->GetScreenPos((&bc));
    if (RouteUnitTo(unit, bc.m_x >> 5, bc.m_y >> 5, 0x1000d8f, flags, 1) == 0) {
        // Re-path failed: re-clamp the board dirty-rect, clear the cooldown, ret 0.
        RECT fb;
        fb.left = 0;
        fb.top = 0;
        RECT fr;
        RECT* fp = static_cast<RECT*>(new (&fr) CRect(0, 0, board->m_width, board->m_height));
        fb.right = board->m_width;
        fb.bottom = board->m_height;
        RECT frc;
        frc.left = fp->left;
        frc.top = fp->top;
        frc.right = fp->right;
        frc.bottom = fp->bottom;
        if (!IntersectRect(&board->m_bounds, &frc, &fb)) {
            board->m_bounds = frc;
        }
        board->m_gridW = board->m_bounds.right - board->m_bounds.left;
        board->m_gridH = board->m_bounds.bottom - board->m_bounds.top;
        unit->m_dwell = 0;
        return 0;
    }
    if (unit->m_defenderState != 3) {
        unit->m_defenderState = 0;
        unit->m_254 = 0;
    }
    if (unit->m_390 != 0) {
        __int64 elapsed = static_cast<__int64>(static_cast<u32>(g_frameTime)) - m_routeClock.m_v;
        if (elapsed >= m_routeWindow.m_v) {
            unit->m_390 = 0;
            CGameObject* lvl = unit->m_object;
            // On-screen test against the main plane's tile origin/extent quad
            // (+0x40..+0x4c), overlaid as a RECT (the sanctioned int-quad read).
            RECT* hit = &g_gameReg->m_world->m_level->m_mainPlane->m_viewRect;
            if (lvl->m_screenX < hit->right && lvl->m_screenX >= hit->left
                && lvl->m_screenY < hit->bottom && lvl->m_screenY >= hit->top) {
                g_gameReg->m_cueSink->SpawnVoiceDriver(unit, 0x366, -1, 0, -1, -1);
            }
            m_routeClock.m_v = 0;
            m_scratch80 = 0x1388;
            m_scratch84 = 0;
            m_scratch78 = g_frameTime;
            m_scratch7c = 0;
        }
    }
    // Re-clamp the board dirty-rect to the board bounds, clear the cooldown, ret 1.
    RECT gb;
    static_cast<RECT*>(new (&gb) CRect(0, 0, board->m_width, board->m_height));
    RECT gr2;
    RECT* gp = static_cast<RECT*>(new (&gr2) CRect(0, 0, board->m_width, board->m_height));
    RECT grc;
    grc.left = gp->left;
    grc.top = gp->top;
    grc.right = gp->right;
    grc.bottom = gp->bottom;
    if (!IntersectRect(&board->m_bounds, &grc, &gb)) {
        board->m_bounds = grc;
    }
    board->m_gridW = board->m_bounds.right - board->m_bounds.left;
    board->m_gridH = board->m_bounds.bottom - board->m_bounds.top;
    unit->m_dwell = 0;
    return 1;
}

// ===========================================================================
// CBattlezMapConfig::PathToNearestCandidate  @0x02edb0  (/GX EH frame)
// Reroute a unit toward a target cell. The target is (arg2,arg3) when `useArg` is
// set, else the first of the unit's occupied coords that lands on a blocked (bit
// 0x4) tile. If the unit already collides there (IsCoordOccupied) recycle its path +
// clear state; if its path is blocked (PathCrossesMarkedTile) honour the reserved-tile
// bit. Otherwise scan the current cell-row for the nearest eligible unit (passing
// the cached-cell + clear-flag guards and NOT an I/G/L/P/J/C/R type code) within
// distance 0x190, build the FindPath flags from its 0x16/0x12 anim modes, ask
// CMapMgr::FindPath for a route, and swap that unit's path onto this one (recycle old
// coords onto g_coordPool, AddTail the new, set state 5). Returns 1 on a reroute.
// ===========================================================================
RVA(0x0002ed90, 0x5)
i32 CBattlezMapConfig::PathToNearbyUnit(CGrunt*) {
    return 0;
}

RVA(0x0002edb0, 0x6b4)
i32 CBattlezMapConfig::PathToNearestCandidate(CGrunt* unit, i32 useArg, i32 ax, i32 ay) {
    if (unit->CoordCount() == 0) {
        return 0;
    }
    i32 tx = 0;
    i32 ty = 0;
    i32 found = 0;
    if (useArg != 0) {
        tx = ax;
        ty = ay;
        found = 1;
    } else {
        // Find the unit's first occupied coord that sits on a blocked tile.
        CoordNode* n = unit->CoordHead();
        while (n != 0) {
            CoordNode* cur = n;
            n = n->m_next;
            Coord* c = cur->m_coord;
            if (c != 0) {
                BrickzCell* row = m_board->m_rows[c->m_y];
                if (row[c->m_x].m_0 & 4) {
                    tx = c->m_x;
                    ty = c->m_y;
                    found = 1;
                    break;
                }
            }
        }
    }
    if (found == 0) {
        return 0;
    }
    if (unit->m_defenderState == 3) {
        return 1;
    }
    if (found == 0) {
        return 0;
    }
    if (IsCoordOccupied(unit, tx, ty) != 0) {
        // Already colliding there: recycle the unit's path + reset state.
        if (unit->CoordCount() != 0) {
            CoordNode* n = unit->CoordHead();
            while (n != 0) {
                CoordNode* cur = n;
                n = n->m_next;
                if (cur->m_coord != 0) {
                    CoordPoolNode* node = g_coordPool.NodeOf(cur->m_coord);
                    node->m_next = g_coordPool.m_freeHead;
                    g_coordPool.m_freeHead = node;
                }
            }
            unit->m_31c.RemoveAll();
        }
        unit->m_defenderState = 0;
        return 1;
    }
    if (found == 0) {
        return 0;
    }
    if (PathCrossesMarkedTile(unit) != 0) {
        // Path is blocked: a reserved-tile bit on the first path coord aborts.
        if (unit->CoordCount() != 0) {
            CoordNode* p = unit->CoordHead();
            Coord* c = p->m_coord;
            i32 word;
            CMapMgr* b = m_board;
            if (static_cast<u32>(c->m_x) < static_cast<u32>(b->m_width)
                && static_cast<u32>(c->m_y) < static_cast<u32>(b->m_height)) {
                word = b->m_rows[c->m_y][c->m_x].m_0;
            } else {
                word = 1;
            }
            if (word & 0x20000000) {
                return 0;
            }
            return 1;
        }
    }
    if (IsCoordOccupied(unit, tx, ty) != 0) {
        return 0;
    }
    // Scan the current cell-row from a random start for the nearest eligible unit.
    i32 r = rand() % 15;
    i32 scanned = 0;
    for (;;) {
        CGrunt* cand = m_triggerMgr->m_grid[m_curCell * 15 + r];
        if (cand != 0) {
            CGameObject* lvl = cand->m_object;
            if (lvl->m_screenX == cand->m_lastTilePxX && lvl->m_screenY == cand->m_lastTilePxY
                && cand->m_entranceCommitted != 0 && cand->m_deathAnimStarted == 0
                && cand->m_entranceActive == 0 && cand->m_poweredUp == 0) {
                bool eq;
                eq = (strcmp((*g_typeColl.GetNameRecord(cand->m_objAux->m_1c)), "I") == 0);
                if (!eq) {
                    eq = (strcmp((*g_typeColl.GetNameRecord(cand->m_objAux->m_1c)), "G") == 0);
                }
                if (!eq) {
                    eq = (strcmp((*g_typeColl.GetNameRecord(cand->m_objAux->m_1c)), "L") == 0);
                }
                if (!eq) {
                    eq = (strcmp((*g_typeColl.GetNameRecord(cand->m_objAux->m_1c)), "P") == 0);
                }
                if (!eq) {
                    eq = (strcmp((*g_typeColl.GetNameRecord(cand->m_objAux->m_1c)), "J") == 0);
                }
                if (!eq) {
                    eq = (strcmp((*g_typeColl.GetNameRecord(cand->m_objAux->m_1c)), "C") == 0);
                }
                if (!eq) {
                    eq = (strcmp((*g_typeColl.GetNameRecord(cand->m_objAux->m_1c)), "R") == 0);
                }
                if (!eq && cand != unit && cand->m_defenderState != 3
                    && cand->m_defenderState != 5) {
                    CGameObject* ul = unit->m_object;
                    CGameObject* cl = cand->m_object;
                    i32 dx = (ul->m_screenX >> 5) - (cl->m_screenX >> 5);
                    i32 dy = (ul->m_screenY >> 5) - (cl->m_screenY >> 5);
                    dx = abs(dx);
                    dy = abs(dy);
                    if (dx * dx + dy * dy <= 0x190) {
                        // Found a donor: build the FindPath flags + swap its path.
                        i32 flags = 0x4020;
                        i32 sec = unit->m_entranceReason;
                        if (sec > 0x16) {
                            sec = unit->m_toolId;
                        }
                        if (sec == 0x16) {
                            flags = 0x4962;
                        }
                        i32 prim = unit->m_entranceReason;
                        if (prim > 0x16) {
                            prim = unit->m_toolId;
                        }
                        if (prim == 0x12) {
                            flags |= 0x100;
                        }
                        CPtrList list(10);
                        Coord oc;
                        (static_cast<CUserLogic*>(unit))->GetScreenPos((&oc));
                        CGameObject* dl = cand->m_object;
                        if ((m_board)->SearchEdge(
                                oc.m_x >> 5,
                                oc.m_y >> 5,
                                dl->m_screenX >> 5,
                                dl->m_screenY >> 5,
                                &list,
                                1,
                                0x98b,
                                flags
                            )
                            != 0) {
                            if (list.GetHeadPosition() != 0) {
                                // Recycle the unit's old path coords onto g_coordPool.
                                void* head = list.RemoveHead();
                                if (head != 0) {
                                    CoordPoolNode* node = g_coordPool.NodeOf(head);
                                    node->m_next = g_coordPool.m_freeHead;
                                    g_coordPool.m_freeHead = node;
                                }
                                if (unit->CoordCount() != 0) {
                                    CoordNode* nn = unit->CoordHead();
                                    while (nn != 0) {
                                        CoordNode* cur = nn;
                                        nn = nn->m_next;
                                        if (cur->m_coord != 0) {
                                            CoordPoolNode* fn = g_coordPool.NodeOf(cur->m_coord);
                                            fn->m_next = g_coordPool.m_freeHead;
                                            g_coordPool.m_freeHead = fn;
                                        }
                                    }
                                    unit->m_31c.RemoveAll();
                                }
                                POSITION pp = list.GetHeadPosition();
                                while (pp != 0) {
                                    unit->m_31c.AddTail(list.GetNext(pp));
                                }
                                cand->m_defenderState = 0;
                                unit->m_defenderState = 5;
                            }
                            list.RemoveAll();
                            return 1;
                        }
                        list.RemoveAll();
                        return 0;
                    }
                }
            }
        }
        r = (r + 1) % 15;
        scanned++;
        if (scanned >= 15) {
            break;
        }
    }
    return 0;
}

// ===========================================================================
// CBattlezMapConfig::ChooseIdleBehavior  @0x02f620
// The grunt idle-behaviour chooser (the cluster's largest method). Gate the unit
// on the four clear-flag guards, then reject the I/G/L/P/J/C/R type codes (I via
// GetRecord, the rest via the scratch-teardown GetRecords). For an eligible unit,
// roll a [1..N] band selector against m_bandSplitA/m_bandSplitB to pick one of three behaviour
// bands; within each band roll a second value against an ascending probability-
// threshold table to choose an anim/state index, then apply it via SetState - the
// mode==3 arm instead reseeds idle units in the current cell-row, and the 0x12/
// 0x16 modes recycle the unit's occupied-coord nodes onto g_coordPool.m_freeHead. Returns 1.
// ===========================================================================
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
    // I (resolved directly via GetRecord). The compare result is materialized as a
    // setcc'd bool (the `bool eq` local, not the inline neg/sbb form) - see
    // docs/patterns/strcmp-eq-bool-local-setcc.md.
    bool eq;
    eq = (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_1c)), "I") == 0);
    if (eq) {
        return 0;
    }
    // G / L / P / J / C / R (each via GetRecords, with the scratch CString teardown).
    CString* recs;
    CString* slot;
    i32 cnt;

    recs = g_typeColl.ScratchResolve(unit->m_objAux->m_1c);
    slot = g_typeColl.Slots();
    cnt = g_typeColl.m_grown;
    while (cnt != 0) {
        if (slot != 0) {
            slot->~CString();
        }
        slot++;
        cnt--;
    }
    eq = (strcmp(*recs, "G") == 0);
    if (eq) {
        return 0;
    }

    recs = g_typeColl.ScratchResolve(unit->m_objAux->m_1c);
    slot = g_typeColl.Slots();
    cnt = g_typeColl.m_grown;
    while (cnt != 0) {
        if (slot != 0) {
            slot->~CString();
        }
        slot++;
        cnt--;
    }
    eq = (strcmp(*recs, "L") == 0);
    if (eq) {
        return 0;
    }

    recs = g_typeColl.ScratchResolve(unit->m_objAux->m_1c);
    slot = g_typeColl.Slots();
    cnt = g_typeColl.m_grown;
    while (cnt != 0) {
        if (slot != 0) {
            slot->~CString();
        }
        slot++;
        cnt--;
    }
    eq = (strcmp(*recs, "P") == 0);
    if (eq) {
        return 0;
    }

    recs = g_typeColl.ScratchResolve(unit->m_objAux->m_1c);
    slot = g_typeColl.Slots();
    cnt = g_typeColl.m_grown;
    while (cnt != 0) {
        if (slot != 0) {
            slot->~CString();
        }
        slot++;
        cnt--;
    }
    eq = (strcmp(*recs, "J") == 0);
    if (eq) {
        return 0;
    }

    recs = g_typeColl.ScratchResolve(unit->m_objAux->m_1c);
    slot = g_typeColl.Slots();
    cnt = g_typeColl.m_grown;
    while (cnt != 0) {
        if (slot != 0) {
            slot->~CString();
        }
        slot++;
        cnt--;
    }
    eq = (strcmp(*recs, "C") == 0);
    if (eq) {
        return 0;
    }

    recs = g_typeColl.ScratchResolve(unit->m_objAux->m_1c);
    slot = g_typeColl.Slots();
    cnt = g_typeColl.m_grown;
    while (cnt != 0) {
        if (slot != 0) {
            slot->~CString();
        }
        slot++;
        cnt--;
    }
    eq = (strcmp(*recs, "R") == 0);
    if (eq) {
        return 0;
    }

    // Pick the behaviour band: roll a [1..m_bandDiv] value (or a coin when m_bandDiv == 0).
    i32 band;
    if (m_bandDiv == 0) {
        band = rand() & 1;
    } else {
        band = rand() % m_bandDiv + 1;
    }
    if (band <= m_bandSplitA) {
        // Band A: the unit must currently be idle (m_entranceReason/m_toolId clear).
        i32 cur = unit->m_entranceReason;
        if (cur > 0x16) {
            cur = unit->m_toolId;
        }
        if (cur != 0) {
            return 1;
        }
        i32 roll;
        if (m_bandADiv == 0) {
            roll = rand() & 1;
        } else {
            roll = rand() % m_bandADiv + 1;
        }
        i32 mode;
        if (roll <= m_bandAThresh[0]) {
            mode = 1;
        } else if (roll <= m_bandAThresh[1]) {
            mode = 2;
        } else if (roll <= m_bandAThresh[2]) {
            mode = 3;
        } else if (roll <= m_bandAThresh[3]) {
            mode = 4;
        } else if (roll <= m_bandAThresh[4]) {
            mode = 5;
        } else if (roll <= m_bandAThresh[5]) {
            mode = 6;
        } else if (roll <= m_bandAThresh[6]) {
            mode = 7;
        } else if (roll <= m_bandAThresh[7]) {
            mode = 8;
        } else if (roll <= m_bandAThresh[8]) {
            mode = 9;
        } else if (roll <= m_bandAThresh[9]) {
            mode = 0xa;
        } else if (roll <= m_bandAThresh[10]) {
            mode = 0xb;
        } else if (roll <= m_bandAThresh[11]) {
            mode = 0xc;
        } else if (roll <= m_bandAThresh[12]) {
            mode = 0xd;
        } else if (roll <= m_bandAThresh[13]) {
            mode = 0xe;
        } else if (roll <= m_bandAThresh[14]) {
            mode = 0xf;
        } else if (roll <= m_bandAThresh[15]) {
            mode = 0x10;
        } else if (roll <= m_bandAThresh[16]) {
            mode = 0x11;
        } else if (roll <= m_bandAThresh[17]) {
            mode = 0x12;
        } else if (roll <= m_bandAThresh[18]) {
            mode = 0x13;
        } else if (roll <= m_bandAThresh[19]) {
            mode = 0x15;
        } else {
            mode = 0x16;
        }
        if (mode == 0x14) {
            mode = 5;
        }
        if (mode == 3) {
            // Reseed: count idle units in the current cell-row; bail if 2+ already.
            CGrunt** row = &m_triggerMgr->m_grid[m_curCell * 15];
            i32 nIdle = 0;
            for (i32 s = 15; s != 0; s--) {
                CGrunt* u = *row;
                if (u != 0 && u->m_2d8 == 3) {
                    nIdle++;
                }
                row++;
            }
            if (nIdle >= 2) {
                return 1;
            }
            for (i32 b = 0; b < 15; b++) {
                CGrunt* u = m_triggerMgr->m_grid[m_curCell * 15 + b];
                if (u == 0) {
                    continue;
                }
                if (u->m_2d8 != 0) {
                    continue;
                }
                if (u->m_poweredUp != 0) {
                    continue;
                }
                (static_cast<CGrunt*>(u))->LoadPickupSprites(3, 1, 0, 0, 1);
                u->m_2d8 = 3;
                if (u->CoordCount() != 0) {
                    CoordNode* n = u->CoordHead();
                    while (n != 0) {
                        CoordNode* curn = n;
                        n = n->m_next;
                        if (curn->m_coord != 0) {
                            CoordPoolNode* node = g_coordPool.NodeOf(curn->m_coord);
                            node->m_next = g_coordPool.m_freeHead;
                            g_coordPool.m_freeHead = node;
                        }
                    }
                    u->m_31c.RemoveAll();
                }
            }
            return 1;
        }
        // Non-3 band-A mode: if the unit is idle, apply directly; otherwise recycle
        // the unit's coord nodes for the 0x12 / 0x16 modes.
        i32 cur2 = unit->m_entranceReason;
        if (cur2 > 0x16) {
            cur2 = unit->m_toolId;
        }
        if (cur2 == 0) {
            (static_cast<CGrunt*>(unit))->LoadPickupSprites(mode, 1, 0, 0, 1);
            return 1;
        }
        if (mode == 0x12) {
            if (unit->CoordCount() != 0) {
                CoordNode* n = unit->CoordHead();
                while (n != 0) {
                    CoordNode* curn = n;
                    n = n->m_next;
                    if (curn->m_coord != 0) {
                        CoordPoolNode* node = g_coordPool.NodeOf(curn->m_coord);
                        node->m_next = g_coordPool.m_freeHead;
                        g_coordPool.m_freeHead = node;
                    }
                }
                unit->m_31c.RemoveAll();
            }
        } else if (mode == 0x16) {
            if (unit->CoordCount() != 0) {
                CoordNode* n = unit->CoordHead();
                while (n != 0) {
                    CoordNode* curn = n;
                    n = n->m_next;
                    if (curn->m_coord != 0) {
                        CoordPoolNode* node = g_coordPool.NodeOf(curn->m_coord);
                        node->m_next = g_coordPool.m_freeHead;
                        g_coordPool.m_freeHead = node;
                    }
                }
                unit->m_31c.RemoveAll();
            }
        }
        return 1;
    } else if (band <= m_bandSplitB) {
        // Band B: a higher anim index (0x17..0x1f) chosen against m_bandBThresh[0..8].
        i32 roll;
        if (m_bandBDiv == 0) {
            roll = rand() & 1;
        } else {
            roll = rand() % m_bandBDiv + 1;
        }
        i32 mode;
        if (roll <= m_bandBThresh[0]) {
            mode = 0x17;
        } else if (roll <= m_bandBThresh[1]) {
            mode = 0x18;
        } else if (roll <= m_bandBThresh[2]) {
            mode = 0x19;
        } else if (roll <= m_bandBThresh[3]) {
            mode = 0x1a;
        } else if (roll <= m_bandBThresh[4]) {
            mode = 0x1b;
        } else if (roll <= m_bandBThresh[5]) {
            mode = 0x1c;
        } else if (roll <= m_bandBThresh[6]) {
            mode = 0x1d;
        } else if (roll <= m_bandBThresh[7]) {
            mode = 0x1e;
        } else {
            mode = (roll > m_bandBThresh[8]) + 0x1f;
        }
        (static_cast<CGrunt*>(unit))->LoadPickupSprites(mode, 1, 0, 0, 1);
        return 1;
    } else {
        // Band C: the rarest anim band (0x23..0x26) chosen against m_bandCThresh[0..2].
        i32 roll;
        if (m_bandCDiv == 0) {
            roll = rand() & 1;
        } else {
            roll = rand() % m_bandCDiv + 1;
        }
        i32 mode;
        if (roll <= m_bandCThresh[0]) {
            mode = 0x23;
        } else if (roll <= m_bandCThresh[1]) {
            mode = 0x24;
        } else if (roll <= m_bandCThresh[2]) {
            mode = 0x25;
        } else {
            mode = 0x26;
        }
        if (mode >= 0x22) {
            unit->m_194 = mode;
            unit->m_moveMode = -1;
        }
        return 1;
    }
}

// ===========================================================================
// CBattlezMapConfig::RouteUnitTo  @0x0300c0  (/GX EH frame)
// Re-path `unit` to (gx,gy): if it is already there (its level geometry's
// (>>5) coord equals the goal) succeed trivially; otherwise ask the board's
// A* (FindPath) for a route into a local CPtrList, then swap the unit's path:
// recycle each old coord node onto the coord pool, empty the unit's path list,
// AddTail every new path node onto it, set the unit's packed coord from the
// new tail, and destruct the local list. Returns 1 on a route, 0 otherwise.
// ===========================================================================
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
    if ((lvl->m_screenX >> 5) == gx && (lvl->m_screenY >> 5) == gy) {
        return 0;
    }
    if ((m_board)->SearchEdge(
            lvl->m_screenX >> 5,
            lvl->m_screenY >> 5,
            gx,
            gy,
            &list,
            clearFlag,
            maskA,
            maskC
        )
        == 0) {
        return 0;
    }
    if (list.GetCount() == 0) {
        return 0;
    }
    void* head = list.RemoveHead();
    if (head != 0) {
        CoordPoolNode* node = g_coordPool.NodeOf(head);
        node->m_next = g_coordPool.m_freeHead;
        g_coordPool.m_freeHead = node;
    }
    if (list.GetCount() == 0) {
        return 0;
    }
    // Recycle the unit's current path-coord nodes onto the coord pool, empty its
    // path list.
    if (unit->CoordCount() != 0) {
        CoordNode* n = unit->CoordHead();
        while (n != 0) {
            CoordNode* cur = n;
            n = n->m_next;
            if (cur->m_coord != 0) {
                g_coordPool.Push(cur->m_coord);
            }
        }
        unit->m_31c.RemoveAll();
    }
    // AddTail every new path node's coord onto the unit's path list.
    POSITION pp = list.GetHeadPosition();
    while (pp != 0) {
        Coord* cur = static_cast<Coord*>(list.GetNext(pp));
        if (cur != 0) {
            unit->m_31c.AddTail(cur);
        }
    }
    list.RemoveAll();
    Coord* tail = (unit->CoordTail())->m_coord;
    unit->m_entrancePxX = (tail->m_x << 5) + 0x10;
    unit->m_entrancePxY = (tail->m_y << 5) + 0x10;
    return 1;
}

// ===========================================================================
// CBattlezMapConfig::RouteUnitToGoal  @0x0302c0  (/GX EH frame)
// Re-path `unit` to (gx, gy) - the GetCoord-fronted twin of RouteUnitTo. If the
// unit is already at the goal (its GetCoord (>>5) == (gx, gy)) bail; scan its path
// for a node already on the goal; ask the board's A* (FindPath) for a route into a
// local CPtrList; recycle the route's head + (when the goal was already queued) the
// path-list base + the unit's existing coord nodes onto g_coordPool.m_freeHead; then AddTail
// every new route node onto the unit's path list. Returns 1 on a route, 0 otherwise.
// ===========================================================================
// @early-stop
RVA(0x000302c0, 0x1ec)
i32 CBattlezMapConfig::RouteUnitToGoal(CGrunt* unit, i32 gx, i32 gy, i32 maskA, i32 maskC) {
    CPtrList list(10);
    Coord cur;
    (static_cast<CUserLogic*>(unit))->GetScreenPos((&cur));
    if ((cur.m_x >> 5) == gx) {
        Coord cur2;
        (static_cast<CUserLogic*>(unit))->GetScreenPos((&cur2));
        if ((cur2.m_y >> 5) == gy) {
            return 0;
        }
    }
    // Scan the unit's path for a node already on the goal (match = the node after it).
    CoordNode* match = 0;
    CoordNode* n = unit->CoordHead();
    while (n != 0) {
        CoordNode* cur3 = n;
        n = n->m_next;
        Coord* coord = cur3->m_coord;
        if (coord != 0 && coord->m_x == gx && coord->m_y == gy) {
            match = n;
            break;
        }
    }
    CGameObject* lvl = unit->m_object;
    // BUG FIX 2026-07-29 (found by naming): this passed `a5, a5` and dropped a4.
    // Retail @0x30351 pushes [esp+0x54] then, one push later, [esp+0x54] again - i.e.
    // arg5 (maskC) then arg4 (maskA), the same (maskA, maskC) pair RouteUnitTo passes.
    // The old spelling sent maskC into both slots and never read maskA at all.
    if ((m_board)
            ->SearchEdge(lvl->m_screenX >> 5, lvl->m_screenY >> 5, gx, gy, &list, 0, maskA, maskC)
        == 0) {
        return 0;
    }
    if (list.GetCount() == 0) {
        return 0;
    }
    void* head = list.RemoveHead();
    if (head != 0) {
        CoordPoolNode* node = g_coordPool.NodeOf(head);
        node->m_next = g_coordPool.m_freeHead;
        g_coordPool.m_freeHead = node;
    }
    if (list.GetCount() == 0) {
        return 0;
    }
    // The matched-path-segment recycle (degenerate in retail).
    if (match != 0 && unit->CoordHead() != 0) {
        CoordPoolNode* node = g_coordPool.NodeOf(&unit->m_31c);
        node->m_next = g_coordPool.m_freeHead;
        g_coordPool.m_freeHead = node;
    }
    // Recycle the unit's existing coord nodes onto g_coordPool.m_freeHead, then empty its path.
    if (unit->CoordCount() != 0) {
        CoordNode* p = unit->CoordHead();
        while (p != 0) {
            CoordNode* cur4 = p;
            p = p->m_next;
            if (cur4->m_coord != 0) {
                CoordPoolNode* node = g_coordPool.NodeOf(cur4->m_coord);
                node->m_next = g_coordPool.m_freeHead;
                g_coordPool.m_freeHead = node;
            }
        }
        unit->m_31c.RemoveAll();
    }
    // AddTail every new route node's coord onto the unit's path list.
    POSITION qp = list.GetHeadPosition();
    while (qp != 0) {
        Coord* cur5 = static_cast<Coord*>(list.GetNext(qp));
        if (cur5 != 0) {
            unit->m_31c.AddTail(cur5);
        }
    }
    list.RemoveAll();
    return 1;
}

RVA(0x00030530, 0x56)
i32 CBattlezMapConfig::PathCrossesMarkedTile(CGrunt* unit) {
    if (unit->CoordCount() == 0) {
        return 0;
    }
    CoordNode* node = unit->CoordHead();
    if (node == 0) {
        return 0;
    }
    BrickzCell** rows = ((m_board)->m_rows);
    while (node != 0) {
        CoordNode* cur = node;
        node = node->m_next;
        Coord* c = cur->m_coord;
        i32 y = c->m_y;
        i32 x = c->m_x;
        if (rows[y][x].m_0 & 4) {
            return 1;
        }
    }
    return 0;
}

// ===========================================================================
// CBattlezMapConfig::IsCoordOccupied  @0x0305b0
// Scan the current cell-row for any OTHER unit that occupies coordinate
// (arg1, arg2): either via a "blocked tile" hit on the unit's occupied-coord
// list, via the unit's own packed coord (m_entrancePxX/m_entrancePxY >> 5), or via its level
// geometry (m_object->m_5c/m_60 >> 5). Returns 1 on the first hit, else 0.
// ===========================================================================
// @early-stop
RVA(0x000305b0, 0x121)
i32 CBattlezMapConfig::IsCoordOccupied(CGrunt* selfUnit, i32 qx, i32 qy) {
    CGrunt** units = m_triggerMgr->m_grid + m_curCell * 15;
    for (i32 i = 0; i < 15; i++) {
        CGrunt* unit = units[i];
        if (unit == 0) {
            continue;
        }
        if (unit == selfUnit) {
            continue;
        }
        if (unit->m_2d8 == 0xb) {
            continue;
        }
        // retail 0x30600: only TWO guards (count, head) then straight into the body -
        // the walk is a do/while, not a `while` (which would peel a third test).
        if (unit->CoordCount() != 0 && unit->CoordHead() != 0) {
            CMapMgr* board = m_board;
            CoordNode* node = unit->CoordHead();
            do {
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
            } while (node != 0);
        }
        if ((unit->m_entrancePxX >> 5) == qx && (unit->m_entrancePxY >> 5) == qy) {
            return 1;
        }
        CGameObject* lvl = unit->m_object;
        if ((lvl->m_screenX >> 5) == qx && (lvl->m_screenY >> 5) == qy) {
            return 1;
        }
    }
    return 0;
}

// ===========================================================================
// CBattlezMapConfig::ClaimCellFromRow  @0x030730
// Cell-claim scan: for the (cellX,cellY) source unit, walk the 15 unit slots of
// the CURRENT cell-row (m_curCell) and, for each candidate whose mode is 3 (or a
// 2/3-of-the-time random pick) and whose per-level record lands FARTHER than
// distance 0x19 from the candidate's geometry, claim it - mark mode 3 / state 2, stamp the
// target coord (cellX,cellY) and seed m_250 = 0xd87.
// ===========================================================================
// @early-stop
RVA(0x00030730, 0x1da)
i32 CBattlezMapConfig::ClaimCellFromRow(i32 cellX, i32 cellY, i32, i32) {
    if (m_active == 0) {
        return 0;
    }
    if (cellX == m_curCell) {
        return 1;
    }
    CGrunt* src = m_triggerMgr->m_grid[cellX * 15 + cellY];
    if (src == 0) {
        return 0;
    }
    if (src->m_gruntKind == 0x36) {
        return 0;
    }
    if (src->m_2d8 == 4) {
        i32 sx = src->m_arrivalCol;
        i32 sy = src->m_arrivalRow;
        // NOT-equal: retail 0x307ae is `cmp ecx,edx / je 0x307bc`, i.e. it skips the
        // `return 0` when the arrival column MATCHES m_curCell (edx is m_curCell -
        // loaded at 0x3074e and compared against cellX at 0x30755). A mode-4 source
        // unit that is already heading somewhere ELSE stops the claim.
        if (sx != m_curCell) {
            return 0;
        }
    }
    for (i32 i = 0; i < 15; i++) {
        CGrunt* u = m_triggerMgr->m_grid[m_curCell * 15 + i];
        if (u == 0) {
            continue;
        }
        i32 ok = 1;
        if (u->m_2d8 == 3) {
            i32 ux = u->m_arrivalCol;
            i32 uy = u->m_arrivalRow;
            if (ux == cellX && uy == cellY) {
                ok = 0;
            }
        }
        if (u->m_2d8 == 3) {
            i32 ux = u->m_arrivalCol;
            i32 uy = u->m_arrivalRow;
            if (!(ux == cellX && uy == cellY) && (rand() % 3) != 0) {
                ok = 0;
            }
        }
        if (ok == 0) {
            continue;
        }
        CGameObject* lvl = u->m_object;
        i32 lx = lvl->m_screenX >> 5;
        i32 ly = lvl->m_screenY >> 5;
        if (u->m_2d8 == 4 && u->m_2e8 != -1) {
            CBattlezMapConfig* bundle = &m_ctx->m_options[u->m_2e8].m_038;
            i32 dx = bundle->m_markerX - lx;
            i32 dy = bundle->m_markerY - ly;
            dx = abs(dx);
            dy = abs(dy);
            // <=, not >: retail 0x30899 is `cmp edx,0x19 / jg`, so the `xor ebp,ebp`
            // that clears `ok` runs when the squared distance is WITHIN 0x19. The
            // candidate is rejected for being too CLOSE to its own level-record
            // marker, not for being too far.
            if (dx * dx + dy * dy <= 0x19) {
                ok = 0;
            }
        }
        if (ok == 0) {
            continue;
        }
        u->m_arrivalCol = cellX;
        u->m_2d8 = 3;
        u->m_arrivalRow = cellY;
        u->m_defenderState = 2;
        u->m_250 = 0xd87;
        u->m_254 = 0;
    }
    return 1;
}

// ===========================================================================
// CBattlezMapConfig::TrySeedSpawnAt  @0x030990
// Try to seed a fresh spawn unit at a screen cell. Count the occupied units in the
// current cell-row; if that count is at/over the per-level record's budget
// (rec->m_378) bail. Otherwise probe the screen cell mapped from (arg1,arg2) via the
// grid's SpawnProbe (using rec->m_158 as the kind tag); if it resolves to a unit
// slot, seed it as a fresh mode-4 spawn (state 0x11, -1 coord block). Returns 1 on a
// seeded spawn, 0 otherwise.
// ===========================================================================
// @early-stop
RVA(0x00030990, 0x11b)
i32 CBattlezMapConfig::TrySeedSpawnAt(i32 ax, i32 ay) {
    CGrunt** row = &m_triggerMgr->m_grid[m_curCell * 15];
    i32 occupied = 0;
    for (i32 c = 15; c != 0; c--) {
        if (*row != 0) {
            occupied++;
        }
        row++;
    }
    if (occupied >= m_ctx->m_options[m_curCell].m_comboSel) {
        return 0;
    }
    i32 cell = m_triggerMgr->PlaceObject(
        m_curCell,
        (ay << 5) + 0x10,
        (ax << 5) + 0x10,
        0x186a0,
        3,
        m_ctx->m_options[m_curCell].m_008,
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
    CGrunt* unit = m_ctx->m_cmdGrid->m_grid[cell + m_curCell * TM_GRID_COLS];
    if (unit == 0) {
        return 0;
    }
    unit->m_arrivalCol = -1;
    unit->m_2f8 = -1;
    unit->m_defenderX = -1;
    unit->m_arrivalState = 0x11;
    unit->m_arrivalRow = -1;
    unit->m_2e8 = -1;
    unit->m_2fc = -1;
    unit->m_defenderState = 0;
    unit->m_defenderY = -1;
    unit->m_2e4 = 0;
    unit->m_2e0 = 0;
    unit->m_dwell = 0;
    unit->m_390 = 1;
    unit->m_2d8 = 4;
    return 1;
}

// ===========================================================================
// CBattlezMapConfig::PathToNearestGoal  @0x030b20  (/GX EH frame)
// Best-fit reroute: locate the cell record for (col,row) - directly when its tile
// dword[4] == 0x67, else via m_ctx->QueryA - then scan its 24-entry sub-cell
// pointer block for the candidate, not colliding with `unit` (IsCoordOccupied),
// nearest (min squared-distance) to the unit's level coord. If one is found and is
// reachable, build the FindPath flag word from the unit's 0x16/0x12 anim modes,
// ask CMapMgr::FindPath for a route into a local CPtrList, then swap the unit's path
// (recycle old coord nodes onto g_coordPool.m_freeHead, AddTail the new ones, stamp the packed
// target coord + state 5). Returns 1 on a reroute, 0 otherwise.
// ===========================================================================
// @early-stop
RVA(0x00030b20, 0x328)
i32 CBattlezMapConfig::PathToNearestGoal(CGrunt* unit, i32 col, i32 row) {
    CGameObject* lvl = unit->m_object;
    i32 goalX = lvl->m_screenX >> 5;
    i32 goalY = lvl->m_screenY >> 5;
    // The cell record for (col,row): a direct table slot when its tile marker is
    // 0x67, else resolved through QueryA on the packed coordinate.
    BrickzCell* tile = &(static_cast<BrickzCell*>((m_board)->m_rows[row]))[col];
    // BOTH arms run on m_cellQuery (+0x14), not m_ctx (+0x04): retail loads
    // `mov edx,[ebp+0x14] / mov edi,[edx+0x70]` and `mov ecx,[ebp+0x14] / call
    // FindInLists12`. The old m_ctx reading survived only because CGruntzMgr::
    // m_tileGrid sits at +0x70 too, so the trailing displacement matched by accident
    // while the base slot was wrong; +0x70 here is CTileTriggerContainer::
    // m_latchedLeaf, already the CTileTriggerLogic* this needs.
    CTileTriggerLogic* cell;
    // Both arms run on the +0x14 tile-trigger container, NOT +0x04 (retail 0x30b20:
    // `mov edx,[ebp+0x14]; mov edi,[edx+0x70]` and `mov ecx,[ebp+0x14]; call
    // FindInLists12`). +0x70 is AddLogic's id-0x15 latch, which is what tile marker
    // 0x67 selects - so the ex "ctx+0x70 IS the board" @identity-TODO was a wrong decode.
    if (tile->m_10 == 0x67) {
        cell = m_cellQuery->m_latchedLeaf;
    } else {
        cell = m_cellQuery->FindInLists12((col << 8) + row, 0);
    }
    i32 bestX = col;
    i32 bestY = col;
    i32 bestDist = 0x7fffffff;
    if (cell != 0) {
        // First pass: any sub-cell that already collides with `unit` aborts.
        i32 s;
        for (s = 0; s < 24; s++) {
            i32 node = cell->m_block[s];
            if (node != 0) {
                CTileTriggerSwitchLogic* rec = m_cellQuery->FindChild(node, 0);
                if (rec != 0) {
                    i32 cx = rec->m_08;    // the record's tile coords (the m_tileX/m_tileY
                    i32 cy = rec->m_key0c; // slots of the logic-record family)
                    if (IsCoordOccupied(unit, cx, cy) != 0) {
                        return 1;
                    }
                }
            }
        }
        // Second pass: keep the nearest non-colliding sub-cell.
        for (s = 0; s < 24; s++) {
            i32 node = cell->m_block[s];
            if (node != 0) {
                CTileTriggerSwitchLogic* rec = m_cellQuery->FindChild(node, 0);
                if (rec != 0) {
                    i32 cx = rec->m_08;
                    i32 cy = rec->m_key0c;
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
    }
    if (bestDist == 0x7fffffff) {
        return 0;
    }
    if (IsCoordOccupied(unit, bestX, bestY) != 0) {
        return 0;
    }
    CPtrList list(10);
    // The FindPath flag word: 0x60 base, + 0x900/0x100 bits from the unit's
    // 0x16 / 0x12 anim modes (primary m_entranceReason, or secondary m_toolId when m_entranceReason > 0x16).
    i32 flags = 0x60;
    i32 sec = unit->m_entranceReason;
    if (sec > 0x16) {
        sec = unit->m_toolId;
    }
    if (sec == 0x16) {
        flags = 0x962;
    }
    i32 prim = unit->m_entranceReason;
    if (prim > 0x16) {
        prim = unit->m_toolId;
    }
    if (prim == 0x12) {
        flags |= 0x100;
    }
    CGameObject* lvl2 = unit->m_object;
    if ((m_board)->SearchEdge(
            lvl2->m_screenX >> 5,
            lvl2->m_screenY >> 5,
            bestX,
            bestY,
            &list,
            1,
            0x98f,
            flags
        )
        == 0) {
        // No route: hand off to the sibling coord state machine and bail.
        PathToNearestCandidate(unit, 1, bestX, bestY);
        return 0;
    }
    if (list.GetCount() == 0) {
        return 0;
    }
    void* head = list.RemoveHead();
    if (head != 0) {
        CoordPoolNode* node = g_coordPool.NodeOf(head);
        node->m_next = g_coordPool.m_freeHead;
        g_coordPool.m_freeHead = node;
    }
    if (list.GetCount() == 0) {
        return 0;
    }
    // Recycle the unit's current path-coord nodes onto g_coordPool.m_freeHead, empty its list.
    if (unit->CoordCount() != 0) {
        CoordNode* n = unit->CoordHead();
        while (n != 0) {
            CoordNode* cur = n;
            n = n->m_next;
            if (cur->m_coord != 0) {
                CoordPoolNode* fn = g_coordPool.NodeOf(cur->m_coord);
                fn->m_next = g_coordPool.m_freeHead;
                g_coordPool.m_freeHead = fn;
            }
        }
        unit->m_31c.RemoveAll();
    }
    // AddTail every new path node's coord onto the unit's path list.
    POSITION pp = list.GetHeadPosition();
    while (pp != 0) {
        unit->m_31c.AddTail(list.GetNext(pp));
    }
    Coord* tail = (unit->CoordTail())->m_coord;
    unit->m_entrancePxX = (tail->m_x << 5) + 0x10;
    unit->m_entrancePxY = (tail->m_y << 5) + 0x10;
    unit->m_defenderState = 5;
    return 1;
}

// ===========================================================================
// CBattlezMapConfig::PickSpawnCoord  @0x030f20
// Pick a spawn coordinate for `unit` from the per-level record's candidate list
// (index `kind`, 0..3): start at a random candidate and walk forward (mod count)
// looking for one not already occupied by any unit in the current cell-row; on
// success write it to `out`. Out-of-range `kind` or empty list falls back to the
// unit's own (>>5) geometry. Returns `out`.
// ===========================================================================
// @early-stop
RVA(0x00030f20, 0x16d)
void* CBattlezMapConfig::PickSpawnCoord(void* out, CGrunt* unit, i32 kind) {
    Coord* o = static_cast<Coord*>(out);
    if (kind < 0 || kind >= 4) {
        CGameObject* lvl = unit->m_object;
        o->m_x = lvl->m_screenX >> 5;
        o->m_y = lvl->m_screenY >> 5;
        return o;
    }
    CPtrArray* coords = &m_ctx->m_options[kind].m_038.m_0f0; // the loop-3 start-coord array
    CGameObject* lvl = unit->m_object;
    i32 rx = lvl->m_screenX >> 5;
    i32 ry = lvl->m_screenY >> 5;
    i32 count = coords->GetSize();
    if (count != 0) {
        i32 r = rand() % count;
        i32 k = 0;
        if (count > 0) {
            Coord** arr = CoordArrayData(*coords);
            CTriggerMgr* grid = m_triggerMgr;
            i32 cell = m_curCell;
            for (;;) {
                Coord* cand = arr[r];
                i32 cx = cand->m_x;
                i32 cy = cand->m_y;
                i32 ok = 1;
                CGrunt** row = &grid->m_grid[cell * 15];
                for (i32 j = 15; j != 0; j--) {
                    CGrunt* u = *row;
                    if (u != 0 && u->CoordCount() != 0) {
                        Coord* node = u->CoordTail()->m_coord;
                        if (node->m_x == cx && node->m_y == cy) {
                            ok = 0;
                        }
                    }
                    row++;
                }
                if (ok != 0) {
                    o->m_x = cx;
                    o->m_y = cy;
                    return o;
                }
                r = (r + 1) % count;
                k++;
                if (k >= count) {
                    break;
                }
            }
        }
        r = rand() % count;
        Coord* cand = CoordArrayData(*coords)[r];
        rx = cand->m_x;
        ry = cand->m_y;
    }
    o->m_x = rx;
    o->m_y = ry;
    return o;
}

#define MOVE_RECYCLE(g)                                                                            \
    {                                                                                              \
        CoordNode* nd = (g)->CoordHead();                                                          \
        while (nd != 0) {                                                                          \
            CoordNode* cur = nd;                                                                   \
            nd = nd->m_next;                                                                       \
            if (cur->m_coord != 0) {                                                               \
                g_coordPool.Push(cur->m_coord);                                                    \
            }                                                                                      \
        }                                                                                          \
        (g)->m_31c.RemoveAll();                                                                    \
    }

// @early-stop
RVA(0x00031610, 0x501)
i32 CBattlezMapConfig::Step(CGrunt* g) {
    if (g->CoordCount() == 0) {
        if (g->m_defenderState == 2) {
            goto inflight;
        }

        // ---- fresh: re-query the move grid for the target tile ----
        i32 W = m_board->m_width;
        i32 H = m_board->m_height;
        Coord c0;
        g->GetScreenPos((&c0));
        c0.m_x >>= 5;
        c0.m_y >>= 5;
        CGrunt* nb = FindIdleGruntInBox(
            c0.m_x,
            c0.m_y,
            static_cast<i32>((static_cast<u32>(W) / 3)),
            static_cast<i32>((static_cast<u32>(H) / 3))
        );
        if (nb != 0) {
            Coord c1;
            nb->GetScreenPos((&c1));
            c1.m_x >>= 5;
            c1.m_y >>= 5;
            if (g->TileSwitch(c1.m_x, c1.m_y, 0xd87, 0, 1, 0) == 0) {
                return 1;
            }
            g->m_arrivalCol = nb->m_tileOwnerHi;
            g->m_arrivalRow = nb->m_tileOwnerLo;
            g->m_defenderState = 2;
            g->m_dwell = 0;
            AcceptAlways(g);
            return 1;
        }
        // nb == 0: replan / drain
        if (static_cast<u32>(g->m_dwell) > static_cast<u32>(m_0b8)) {
            Coord here;
            g->GetScreenPos((&here));
            ::TileSwitch(g, here.m_x >> 5, here.m_y >> 5, m_ac, m_b0, -1);
            if (g->CoordCount() > m_98 + m_94 && g->CoordCount() != 0) {
                POSITION pos = g->m_31c.GetHeadPosition();
                if (pos != 0) {
                    do {
                        void* d = g->CoordListOps()->NextData(pos);
                        if (d != 0) {
                            g_coordPool.Push(d);
                        }
                    } while (pos != 0);
                }
                g->m_31c.RemoveAll();
            }
            g->m_dwell = 0;
        }
        return 1;
    }

    // m_328 != 0
    if (g->m_defenderState != 2) {
        return 1;
    }
inflight: {
    // ---- in-flight: advance / reroute along the path ----
    i32 col = g->m_arrivalCol;
    i32 row = g->m_arrivalRow;
    CGrunt* cur = m_8->m_grid[15 * col + row];
    i32 W = m_board->m_width;
    i32 H = m_board->m_height;
    Coord c0;
    g->GetScreenPos((&c0));
    c0.m_x >>= 5;
    c0.m_y >>= 5;
    CGrunt* nb = FindIdleGruntInBox(
        c0.m_x,
        c0.m_y,
        static_cast<i32>((static_cast<u32>(W) / 3)),
        static_cast<i32>((static_cast<u32>(H) / 3))
    );

    if (cur == 0) {
        goto L_clear;
    }
    if (nb != 0 && cur != nb) {
        if (g->CoordCount() != 0) {
            MOVE_RECYCLE(g);
        }
        g->m_arrivalCol = nb->m_tileOwnerHi;
        g->m_arrivalRow = nb->m_tileOwnerLo;
        g->m_defenderState = 2;
        g->m_dwell = 0;
        {
            CGameObject* s = static_cast<CGameObject*>(nb->m_object);
            if (g->TileSwitch(s->m_screenX >> 5, s->m_screenY >> 5, 0xd87, 0, 0, 0) == 0) {
                return 1;
            }
        }
        cur = nb; // loc34
    }
    // L_900
    if (cur == 0) {
        goto L_clear;
    }
    {
        CGameObject* s = cur->m_object;
        if (g->RectContains(s->m_screenX, s->m_screenY) != 0) {
            // arrived on this tile: latch the move.  retail 0x3191e recycles the
            // tracked coordz FIRST (guard + walk + RemoveAll, tail-merged with the
            // 0x318a3 copy at 0x262bc-style shared exit) - this block was missing.
            if (g->CoordCount() != 0) {
                MOVE_RECYCLE(g);
            }
            g->m_arrivalCol = -1;
            g->m_arrivalRow = -1;
            winapi_02ae00_IntersectRect(g, cur);
            g->m_defenderState = 0;
            return 1;
        }
    }
    // 3198f: not arrived - reroute by board distance
    if (static_cast<u32>(g->m_dwell) <= static_cast<u32>(m_reserveBudget)) {
        return 1;
    }
    {
        Coord here;
        g->GetScreenPos((&here));
        i32 x5 = here.m_x >> 5;
        i32 y5 = here.m_y >> 5;
        Coord nbpos;
        cur->GetTilePos((&nbpos));
        i32 dx = nbpos.m_x - x5;
        i32 dy = nbpos.m_y - y5;
        i32 adx = dx < 0 ? -dx : dx;
        i32 ady = dy < 0 ? -dy : dy;
        i32 dist = static_cast<i32>(sqrt(static_cast<double>((adx * adx + ady * ady))));
        if (dist > m_c0) {
            if (g->CoordCount() != 0) {
                MOVE_RECYCLE(g);
            }
            goto L_clearAt;
        }
        if (g->CoordCount() != 0) {
            MOVE_RECYCLE(g);
        }
        CGameObject* s = cur->m_object;
        if (g->TileSwitch(s->m_screenX >> 5, s->m_screenY >> 5, 0xd87, 0, 0, 0) != 0) {
            g->m_dwell = 0;
            return 1;
        }
    }
L_clearAt:
    g->m_arrivalCol = -1;
    g->m_arrivalRow = -1;
    g->m_defenderState = 0;
    g->m_dwell = 0;
    return 1;

L_clear:
    g->m_arrivalCol = -1;
    g->m_defenderState = 0;
    g->m_arrivalRow = -1;
    return 1;
}
}
#undef MOVE_RECYCLE

// CGrunt::GetTilePos (0x31c70) - write the HUD tile coords (m_10->m_5c/m_60 >> 5)
// into the caller's {x,y} out slot and return it. Its retail body's birth position is
// inside this TU's 0x29a30 interval; a tiny leaf, likely COMDAT-at-usage emitted by this obj.
// @early-stop
RVA(0x00031c70, 0x1d)
Coord* CGrunt::GetTilePos(Coord* out) {
    CWwdGameObjectA* h = m_object;
    i32 x = h->m_screenX >> 5;
    i32 y = h->m_screenY >> 5;
    out->m_x = x;
    out->m_y = y;
    return out;
}

// ===========================================================================
// CBattlezMapConfig::winapi_031ca0_IntersectRect  @0x031ca0
// The queued-unit arrival resolver. For a unit with a live target cell
// (m_arrivalCol/m_arrivalRow != -1) locate the unit at that cell (grid[m_arrivalCol][m_arrivalRow]); if it is
// gone, reset the unit (mode 4 / -1 coords) recycling its path onto g_coordPool.m_freeHead.
// If the target cell is already occupied (CGrunt::Occupied on the target's
// level coord), recycle the unit's path onto g_coordPool, clear the target coord
// and hand off to winapi_02ae00. Otherwise clamp the board dirty-rect to the board
// bounds (the CRect / IntersectRect copy-back idiom) and, once the unit's idle
// timer passes 0x1f4, place it at the target's level (>>5) coord (Method_4b320,
// flags m_250). A dangling target (m_arrivalCol/m_arrivalRow == -1) resets via g_coordPool.
// ===========================================================================
// @early-stop
RVA(0x00031ca0, 0x2f2)
i32 CBattlezMapConfig::winapi_031ca0_IntersectRect(CGrunt* unit) {
    i32 tx = unit->m_arrivalCol;
    i32 ty = unit->m_arrivalRow;
    if (tx != -1 && ty != -1) {
        CGrunt* target = m_triggerMgr->m_grid[tx * 15 + ty];
        if (target != 0) {
            CGameObject* lvl = target->m_object;
            if ((static_cast<CGrunt*>(unit))->RectContains(lvl->m_screenX, lvl->m_screenY) != 0) {
                if (unit->CoordCount() != 0) {
                    POSITION pos = unit->m_31c.GetHeadPosition();
                    while (pos != 0) {
                        void* coord = unit->CoordListOps()->NextData(pos);
                        if (coord != 0) {
                            g_coordPool.Push(coord);
                        }
                    }
                    unit->m_31c.RemoveAll();
                }
                unit->m_arrivalCol = -1;
                unit->m_arrivalRow = -1;
                winapi_02ae00_IntersectRect(unit, target);
                return 1;
            }
            // Clamp the board dirty-rect to (0,0,w,h): the CRect / IntersectRect
            // copy-back idiom (shared with GruntPathScan's SCAN_BOUNDS).
            CMapMgr* board = m_board;
            RECT r1;
            static_cast<RECT*>(new (&r1) CRect(0, 0, board->m_width, board->m_height));
            RECT r2;
            RECT* boardRect =
                static_cast<RECT*>(new (&r2) CRect(0, 0, board->m_width, board->m_height));
            RECT rc;
            rc.left = boardRect->left;
            rc.top = boardRect->top;
            rc.right = boardRect->right;
            rc.bottom = boardRect->bottom;
            if (!IntersectRect(&board->m_bounds, &rc, &r1)) {
                board->m_bounds = rc;
            }
            board->m_gridW = board->m_bounds.right - board->m_bounds.left;
            board->m_gridH = board->m_bounds.bottom - board->m_bounds.top;
            if (static_cast<u32>(unit->m_dwell) > 0x1f4 && unit->CoordCount() == 0) {
                i32 flags = unit->m_250;
                unit->m_254 = 0x4268;
                CGameObject* tl = target->m_object;
                unit->TileSwitch(tl->m_screenX >> 5, tl->m_screenY >> 5, 0, flags, 0, 0x4268);
                unit->m_dwell = 0;
            }
            return 1;
        }
        // The target unit is gone: reset it (mode 4 / -1 coords), recycle its path
        // onto g_coordPool.m_freeHead.
        unit->m_arrivalCol = -1;
        unit->m_arrivalRow = -1;
        unit->m_defenderX = -1;
        unit->m_defenderState = 0;
        unit->m_2d8 = 4;
        unit->m_defenderY = -1;
        if (unit->CoordCount() != 0) {
            CoordNode* n = unit->CoordHead();
            if (n != 0) {
                do {
                    CoordNode* cur = n;
                    n = n->m_next;
                    void* coord = cur->m_coord;
                    if (coord != 0) {
                        CoordPoolNode* slot = g_coordPool.NodeOf(coord);
                        slot->m_next = g_coordPool.m_freeHead;
                        g_coordPool.m_freeHead = slot;
                    }
                } while (n != 0);
            }
            unit->m_31c.RemoveAll();
        }
        return 1;
    }
    // A dangling target coord (m_arrivalCol/m_arrivalRow == -1): reset, recycle onto g_coordPool.
    unit->m_arrivalCol = -1;
    unit->m_arrivalRow = -1;
    unit->m_defenderX = -1;
    unit->m_defenderState = 0;
    unit->m_2d8 = 4;
    unit->m_defenderY = -1;
    if (unit->CoordCount() != 0) {
        POSITION pos = unit->m_31c.GetHeadPosition();
        while (pos != 0) {
            void* coord = unit->CoordListOps()->NextData(pos);
            if (coord != 0) {
                g_coordPool.Push(coord);
            }
        }
        unit->m_31c.RemoveAll();
    }
    return 1;
}

// ===========================================================================
// CBattlezMapConfig::winapi_032060_IntersectRect  @0x032060
// The per-unit spawn-path state machine, keyed on the unit's m_defenderState mode. First
// resolve the target band (m_2e8): pick a fresh random one (avoiding the current
// band m_curCell, requiring the record's +0x170 ready / +0x174 clear) when unset, or
// re-validate the stored one (recycling the unit's coords + resetting on an invalid
// record). Then, for a unit that holds no coords (m_coordCount == 0), dispatch on m_defenderState:
//   0 -> seed the goal (m_defenderX/m_defenderY) from the band record or a PickSpawnCoord re-route,
//        keeping the nearer of the current vs stored goal, and advance to mode 6;
//   6 -> if the idle timer (m_dwell) exceeds m_moveBudget, measure the distance to the goal:
//        arrive (mode 7) within 4 tiles, else re-place toward it (GridUnitSpawn::Place,
//        flag word from the 0x12/0x16/0xe anim modes) and, on failure, walk the m_254
//        state code to its next value;
//   7 -> clamp the board dirty-rect to the board bounds and place at the band's queued
//        point (Place, flags 0x987).
// A unit that DOES hold coords (m_coordCount != 0) only advances mode 6 -> 7 once within range,
// recycling its coords onto g_coordPool.m_freeHead. Returns 1.
// ===========================================================================
// @early-stop
RVA(0x00032060, 0x7bd)
i32 CBattlezMapConfig::winapi_032060_IntersectRect(CGrunt* unit) {
    if (unit->m_defenderState == 3) {
        return 1;
    }
    i32 band = unit->m_2e8;
    if (band == -1) {
        band = rand() % 4;
        if (band == m_curCell) {
            band++;
        }
        band = band % 4;
        if (m_ctx->m_options[band].m_clearedRound != 0) {
            return 1;
        }
        if (m_ctx->m_options[band].m_liveGate == 0) {
            return 1;
        }
        unit->m_2e8 = band;
        unit->m_defenderX = -1;
        unit->m_defenderY = -1;
    } else {
        if (m_ctx->m_options[band].m_clearedRound != 0 || m_ctx->m_options[band].m_liveGate == 0) {
            // Invalid record: recycle the unit's coords onto g_coordPool, reset state.
            if (unit->CoordCount() != 0) {
                POSITION pos = unit->m_31c.GetHeadPosition();
                if (pos != 0) {
                    do {
                        void* coord = unit->CoordListOps()->NextData(pos);
                        if (coord != 0) {
                            g_coordPool.Push(coord);
                        }
                    } while (pos != 0);
                }
                unit->m_31c.RemoveAll();
            }
            unit->m_arrivalCol = -1;
            unit->m_arrivalRow = -1;
            unit->m_defenderX = -1;
            unit->m_2e8 = -1;
            unit->m_defenderY = -1;
            unit->m_defenderState = 0;
            unit->m_250 = g_spawnCfg;
            unit->m_254 = g_spawnState;
            return 1;
        }
    }
    band = unit->m_2e8;
    CBattlezMapConfig* bundle = &m_ctx->m_options[band].m_038;
    i32 rx = bundle->m_markerX;
    i32 ry = bundle->m_markerY;
    if (unit->CoordCount() != 0) {
        if (unit->m_defenderState != 6) {
            return 1;
        }
        i32 gx = unit->m_defenderX;
        i32 gy = unit->m_defenderY;
        if (gx == -1 || gy == -1) {
            // Reset the goal: recycle the unit's coords onto g_coordPool.m_freeHead.
            unit->m_defenderState = 0;
            if (unit->CoordCount() != 0) {
                CoordNode* n = unit->CoordHead();
                while (n != 0) {
                    CoordNode* cur = n;
                    n = n->m_next;
                    if (cur->m_coord != 0) {
                        CoordPoolNode* node = g_coordPool.NodeOf(cur->m_coord);
                        node->m_next = g_coordPool.m_freeHead;
                        g_coordPool.m_freeHead = node;
                    }
                }
                unit->m_31c.RemoveAll();
            }
            unit->m_defenderX = -1;
            unit->m_defenderY = -1;
            return 1;
        }
        CGameObject* lvl = unit->m_object;
        i32 dx = abs(gx - (lvl->m_screenX >> 5));
        i32 dy = abs(gy - (lvl->m_screenY >> 5));
        if (dx * dx + dy * dy > 0x10) {
            return 1;
        }
        CoordNode* n = unit->CoordHead();
        while (n != 0) {
            CoordNode* cur = n;
            n = n->m_next;
            if (cur->m_coord != 0) {
                CoordPoolNode* node = g_coordPool.NodeOf(cur->m_coord);
                node->m_next = g_coordPool.m_freeHead;
                g_coordPool.m_freeHead = node;
            }
        }
        unit->m_31c.RemoveAll();
        unit->m_defenderState = 7;
        unit->m_250 = g_spawnCfg;
        unit->m_254 = 0x248;
        return 1;
    }
    if (unit->m_defenderState == 0) {
        unit->m_250 = g_spawnCfg;
        unit->m_254 = g_spawnState;
        i32 gx = unit->m_defenderX;
        if (gx == -1) {
            i32 x, y;
            // +0xf8 is m_0f0 (a CPtrArray at +0x0f0) plus 8 == MFC's m_nSize
            if (bundle->m_0f0.GetSize() != 0) {
                Coord out;
                Coord* r = static_cast<Coord*>(PickSpawnCoord(&out, unit, band));
                x = r->m_x;
                y = r->m_y;
            } else {
                x = rx;
                y = ry;
            }
            unit->m_defenderX = x;
            unit->m_defenderY = y;
            unit->m_defenderState = 6;
            return 1;
        }
        i32 gy = unit->m_defenderY;
        Coord c1;
        (static_cast<CUserLogic*>(unit))->GetScreenPos((&c1));
        i32 dxA = abs(rx - (c1.m_x >> 5));
        Coord c2;
        (static_cast<CUserLogic*>(unit))->GetScreenPos((&c2));
        i32 dyA = abs(ry - (c2.m_y >> 5));
        i32 distA = dxA * dxA + dyA * dyA;
        i32 dxB = abs(rx - gx);
        i32 dyB = abs(ry - gy);
        i32 distB = dxB * dxB + dyB * dyB;
        if (distA > distB) {
            unit->m_defenderState = 6;
        }
        return 1;
    }
    if (unit->m_defenderState == 6) {
        if (static_cast<u32>(unit->m_dwell) <= static_cast<u32>(m_moveBudget)) {
            return 1;
        }
        i32 gx = unit->m_defenderX;
        i32 gy = unit->m_defenderY;
        if (gx == -1 || gy == -1) {
            // Reset the goal: recycle the unit's coords onto g_coordPool.
            unit->m_defenderState = 0;
            if (unit->CoordCount() != 0) {
                CoordNode* n = unit->CoordHead();
                while (n != 0) {
                    CoordNode* cur = n;
                    n = n->m_next;
                    if (cur->m_coord != 0) {
                        g_coordPool.Push(cur->m_coord);
                    }
                }
                unit->m_31c.RemoveAll();
            }
            unit->m_defenderX = -1;
            unit->m_defenderY = -1;
            return 1;
        }
        CGameObject* lvl = unit->m_object;
        i32 dx = abs(gx - (lvl->m_screenX >> 5));
        i32 dy = abs(gy - (lvl->m_screenY >> 5));
        if (dx * dx + dy * dy <= 0x10) {
            unit->m_defenderState = 7;
            unit->m_250 = g_spawnCfg;
            unit->m_254 = 0x248;
            return 1;
        }
        i32 prim = unit->m_entranceReason;
        i32 cfg = unit->m_250;
        i32 flags = unit->m_254;
        i32 t = prim;
        if (prim > 0x16) {
            t = unit->m_toolId;
        }
        if (t == 0x12) {
            flags |= 0x100;
        } else {
            t = prim;
            if (prim > 0x16) {
                t = unit->m_toolId;
            }
            if (t == 0xe) {
                flags |= 0x1000;
            } else {
                if (prim > 0x16) {
                    prim = unit->m_toolId;
                }
                if (prim == 0x16) {
                    flags |= 0x942;
                }
            }
        }
        if (unit->TileSwitch(gx, gy, 0, cfg, 0, flags) != 0) {
            unit->m_250 = g_spawnCfg;
            unit->m_254 = g_spawnState;
            unit->m_dwell = 0;
            return 1;
        }
        i32 st = unit->m_254;
        if (st == g_spawnState) {
            unit->m_254 = 0x40;
        } else if (st == 0x40) {
            unit->m_254 = 0x248;
        } else if (st == 0x248) {
            unit->m_254 = 0x20;
        } else if (st == 0x20) {
            unit->m_254 = 0x228;
        } else if (st == 0x228) {
            unit->m_254 = 0x268;
        } else if (st == 0x268) {
            unit->m_254 = 0x4268;
        }
        unit->m_dwell = 0;
        return 1;
    }
    if (unit->m_defenderState != 7) {
        return 1;
    }
    CMapMgr* board = m_board;
    RECT box2;
    box2.left = 0;
    box2.top = 0;
    RECT bounds;
    RECT* bp = static_cast<RECT*>(new (&bounds) CRect(0, 0, board->m_width, board->m_height));
    box2.right = board->m_width;
    box2.bottom = board->m_height;
    RECT rc;
    rc.left = bp->left;
    rc.top = bp->top;
    rc.right = bp->right;
    rc.bottom = bp->bottom;
    if (!IntersectRect(&board->m_bounds, &rc, &box2)) {
        board->m_bounds = rc;
    }
    board->m_gridW = board->m_bounds.right - board->m_bounds.left;
    board->m_gridH = board->m_bounds.bottom - board->m_bounds.top;
    i32 prim = unit->m_entranceReason;
    i32 flags = unit->m_254;
    i32 t = prim;
    if (prim > 0x16) {
        t = unit->m_toolId;
    }
    if (t == 0x12) {
        flags |= 0x100;
    } else {
        t = prim;
        if (prim > 0x16) {
            t = unit->m_toolId;
        }
        if (t == 0xe) {
            flags |= 0x1000;
        } else {
            if (prim > 0x16) {
                prim = unit->m_toolId;
            }
            if (prim == 0x16) {
                flags |= 0x942;
            }
        }
    }
    if (unit->TileSwitch(rx, ry, 0, 0x987, 1, flags) != 0) {
        unit->m_250 = g_spawnCfg;
        unit->m_254 = g_spawnState;
        unit->m_dwell = 0;
        return 1;
    }
    unit->m_dwell = 0;
    unit->m_254 = 0x4268;
    return 1;
}

// ===========================================================================
// CGrunt::RecycleCoords  @0x0343f0  (__thiscall on a CGrunt). Recycle each occupied-coord node's payload onto g_coordPool.m_freeHead (head
// cached in a register across the loop, written each iteration), then tail into the
// +0x31c CPtrList's RemoveAll. Skips everything when the list's count is zero.
// ===========================================================================
RVA(0x000343f0, 0x47)
void CGrunt::RecycleCoords() {
    if (CoordCount() == 0) {
        return;
    }
    CoordNode* n = CoordHead();
    if (n != 0) {
        do {
            CoordNode* cur = n;
            n = n->m_next;
            void* coord = cur->m_coord;
            if (coord != 0) {
                // no cached local: retail stores the CACHED reg back to g_freeList
                // (`mov [eax],esi; mov esi,eax; mov ds:g_freeList,esi`); a source-level
                // `head` local makes cl store the fresh reg instead.
                CoordPoolNode* slot = g_coordPool.NodeOf(coord);
                slot->m_next = g_coordPool.m_freeHead;
                g_coordPool.m_freeHead = slot;
            }
        } while (n != 0);
    }
    m_31c.RemoveAll();
}

// ===========================================================================
// CBattlezMapConfig::CanPlaySpecialAnim  @0x034460
// Anim-name gate: a unit is eligible for a "special" anim only when it sits on
// its cached cell (lvl coord == m_lastTilePxX/m_lastTilePxY) and a block of state flags is clear.
// Then resolve the unit's anim name and reject the simple type codes (I/G/L/J/C)
// outright; for the remaining codes, run the second resolver (which fills the
// g_typeColl.m_alloc CString array, torn down each call) and either map an in-range
// candidate index directly or grow/report a slot, returning whether the final
// resolved name differs from the "P" code.
// ===========================================================================
// @early-stop
RVA(0x00034460, 0x3fc)
i32 CBattlezMapConfig::CanPlaySpecialAnim(CGrunt* unit) {
    if (unit == 0) {
        return 0;
    }
    CGameObject* lvl = unit->m_object;
    if (lvl->m_screenX != unit->m_lastTilePxX) {
        return 0;
    }
    if (lvl->m_screenY != unit->m_lastTilePxY) {
        return 0;
    }
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
    // Simple type codes resolved directly (GetRecord): I / G / L. The compare
    // result is materialized as a bool (setcc form) - see
    // docs/patterns/return-bool-via-local-setcc.md.
    i32 eq;
    eq = (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_1c)), "I") == 0);
    if (eq) {
        return 0;
    }
    eq = (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_1c)), "G") == 0);
    if (eq) {
        return 0;
    }
    eq = (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_1c)), "L") == 0);
    if (eq) {
        return 0;
    }
    // The remaining codes resolve through GetRecords (which fills the scratch
    // CString array torn down after each call): P / J / C.
    CString* recs;
    CString* slot;
    i32 cnt;

    recs = g_typeColl.ScratchResolve(unit->m_objAux->m_1c);
    slot = g_typeColl.Slots();
    cnt = g_typeColl.m_grown;
    while (cnt != 0) {
        if (slot != 0) {
            slot->~CString();
        }
        slot++;
        cnt--;
    }
    eq = (strcmp(*recs, "P") == 0);
    if (eq) {
        return 0;
    }

    recs = g_typeColl.ScratchResolve(unit->m_objAux->m_1c);
    slot = g_typeColl.Slots();
    cnt = g_typeColl.m_grown;
    while (cnt != 0) {
        if (slot != 0) {
            slot->~CString();
        }
        slot++;
        cnt--;
    }
    eq = (strcmp(*recs, "J") == 0);
    if (eq) {
        return 0;
    }

    recs = g_typeColl.ScratchResolve(unit->m_objAux->m_1c);
    slot = g_typeColl.Slots();
    cnt = g_typeColl.m_grown;
    while (cnt != 0) {
        if (slot != 0) {
            slot->~CString();
        }
        slot++;
        cnt--;
    }
    eq = (strcmp(*recs, "C") == 0);
    if (eq) {
        return 0;
    }

    // Map the candidate index, or grow/report a fresh slot.
    i32 ci = unit->m_objAux->ActKey();
    CString* sel;
    g_typeColl.m_grown = 0;
    if (ci >= g_typeColl.m_lo && ci <= g_typeColl.m_hi) {
        sel = g_typeColl.Elem(ci);
    } else if (g_typeColl.GrowTo(ci, 0) != 0) {
        sel = g_typeColl.Elem(ci);
    } else {
        g_typeColl.Report(g_errOutOfMem, 0xc);
        sel = g_typeColl.Scratch();
    }

    // Tear down the scratch again, then compare the selected name to "R".
    slot = g_typeColl.Slots();
    cnt = g_typeColl.m_grown;
    while (cnt != 0) {
        if (slot != 0) {
            slot->~CString();
        }
        slot++;
        cnt--;
    }
    return strcmp(*sel, "R") != 0;
}

RVA(0x00034960, 0x24)
void zErrHandling::Report(void* sentinel, i32 code) {
    g_retAddrBreadcrumb = GetRetAddr();
    m_errSink->Set(this, sentinel, code);
}

// ===========================================================================
// CBattlezMapConfig::CheckQueuedSpawnTile  @0x034c70
// The queued-unit board-tile resolver. For a unit with no live coord list
// (m_coordCount==0): look up its target tile (board->m_rows[m_arrivalRow][m_arrivalCol]); if the tile
// carries the 0x20 "reserved" flag, only place (Method_4b320, flags 0xd87) when the
// per-level budget (m_dwell) exceeds this->m_reserveBudget - on a successful place clear m_dwell,
// otherwise fall to the "give up" path; if the tile is free, give up directly. The
// give-up path marks the unit mode 4, recycles its coord nodes (onto the coord pool
// for the reserved-tile branch, onto g_coordPool.m_freeHead for the free-tile branch), empties
// its coord list, and resets its target coord (-1,-1) + state. Returns 1.
// ===========================================================================
// @early-stop
RVA(0x00034c70, 0x133)
i32 CBattlezMapConfig::CheckQueuedSpawnTile(CGrunt* unit) {
    if (unit->CoordCount() != 0) {
        return 1;
    }
    i32 x = unit->m_arrivalCol;
    i32 y = unit->m_arrivalRow;
    BrickzCell* tile = &(static_cast<BrickzCell*>((m_board)->m_rows[y]))[x];
    if (tile->m_0 & 0x20) {
        if (static_cast<u32>(unit->m_dwell) <= static_cast<u32>(m_reserveBudget)) {
            return 1;
        }
        if (unit->TileSwitch(unit->m_arrivalCol, unit->m_arrivalRow, 0, 0xd87, 0, 0) != 0) {
            unit->m_dwell = 0;
            return 1;
        }
        unit->m_2d8 = 4;
        // retail 0x34d2e re-reads CoordCount here (TileSwitch above can have
        // re-populated the list) - the guard was missing.
        if (unit->CoordCount() != 0) {
            CoordNode* n = unit->CoordHead();
            while (n != 0) {
                CoordNode* cur = n;
                n = n->m_next;
                if (cur->m_coord != 0) {
                    g_coordPool.Push(cur->m_coord);
                }
            }
            unit->m_31c.RemoveAll();
        }
    } else {
        unit->m_2d8 = 4;
        if (unit->CoordCount() != 0) {
            CoordNode* n = unit->CoordHead();
            while (n != 0) {
                CoordNode* cur = n;
                n = n->m_next;
                if (cur->m_coord != 0) {
                    CoordPoolNode* slot = g_coordPool.NodeOf(cur->m_coord);
                    slot->m_next = g_coordPool.m_freeHead;
                    g_coordPool.m_freeHead = slot;
                }
            }
            unit->m_31c.RemoveAll();
        }
    }
    unit->m_arrivalCol = -1;
    unit->m_arrivalRow = -1;
    unit->m_defenderState = 0;
    unit->m_dwell = 0;
    return 1;
}

// ===========================================================================
// CBattlezMapConfig::RepathToFreeCell  @0x0350d0
// Periodic re-path of `unit` toward the nearest free candidate cell. Gate on the
// unit's m_dwell timer exceeding the bundle's m_repathBudget budget; otherwise walk the grid
// object's candidate list (head at m_triggerMgr->m_4), and among the unoccupied candidates
// (sub->m_occupied == 0, and not already exactly on the unit's level coord) keep the one
// nearest (min squared distance) to the unit's level (>>5) coordinate. If one is
// found, re-path the unit to it via RouteUnitTo (flags 0xd87). Clear m_dwell and
// return 1.
// ===========================================================================
// @early-stop
RVA(0x000350d0, 0xfa)
i32 CBattlezMapConfig::RepathToFreeCell(CGrunt* unit) {
    if (static_cast<u32>(unit->m_dwell) > static_cast<u32>(m_repathBudget)) {
        CGruntPuddle* best = 0;
        i32 bestDist = 0x7fffffff;
        POSITION pos = m_triggerMgr->m_baseList.GetHeadPosition();
        while (pos != 0) {
            CGruntPuddle* cand = static_cast<CGruntPuddle*>(m_triggerMgr->m_baseList.GetNext(pos));
            if (cand->m_pending == 0) {
                CGameObject* lvl = unit->m_object;
                i32 lx = lvl->m_screenX >> 5;
                i32 ly = lvl->m_screenY >> 5;
                if (cand->m_tileX != lx || cand->m_tileY != ly) {
                    i32 dx = cand->m_tileX - lx;
                    dx = abs(dx);
                    i32 dy = cand->m_tileY - ly;
                    dy = abs(dy);
                    i32 dist = dx * dx + dy * dy;
                    if (dist < bestDist) {
                        bestDist = dist;
                        best = cand;
                    }
                }
            }
        }
        if (best != 0) {
            RouteUnitTo(unit, best->m_tileX, best->m_tileY, 0xd87, 0, 0);
        }
        unit->m_dwell = 0;
    }
    return 1;
}

RVA(0x00035210, 0x4f)
i32 CBattlezMapConfig::ProbeUnoccupiedAt(i32 x, i32 y) {
    CPtrList& lst = m_ctx->m_cmdGrid->m_baseList;
    POSITION pos = lst.GetHeadPosition();
    while (pos != 0) {
        CGruntPuddle* cand = static_cast<CGruntPuddle*>(lst.GetNext(pos));
        if (cand != 0 && cand->m_tileX == x && cand->m_tileY == y && cand->m_pending == 0) {
            return 1;
        }
    }
    return 0;
}

RVA(0x00035550, 0x52)
i32 CBattlezMapConfig::ForcePlaceFromReserve(CGrunt* unit) {
    if (unit->CoordCount() != 0) {
        return 1;
    }
    if (static_cast<u32>(unit->m_dwell) <= static_cast<u32>(m_reserveBudget)) {
        return 1;
    }
    unit->TileSwitch(
        unit->m_arrivalCol,
        unit->m_arrivalRow,
        0,
        0xd87,
        0,
        0
    ); // @0x4b320 (thiscall: retail's dead mov ecx)
    unit->m_dwell = 0;
    return 1;
}

// ===========================================================================
// CBattlezMapConfig::RetargetIdleUnit  @0x0358a0  (__thiscall ret 4 => 1 CGrunt* arg)
// The idle-unit policy step: when the unit holds no occupied coords it either
// retargets to a random band (m_arrivalCol == -1, idle timer past m_moveBudget) or re-places at its
// band's default coord (timer past 0x7d0); when it DOES hold coords it despawns
// (recycling them onto g_coordPool) if both band slots are clear, else keeps the unit
// only when it is within 6 tiles of a band candidate (recycling onto g_coordPool.m_freeHead).
// m_ctx indexes the per-band records at stride 0x238; the +0x150/+0x188 sub-objects'
// candidate vectors live at +0xf4 (array) / +0xf8 (count) / +0xd0,+0xd4 (default coord).
// ===========================================================================
// The unit-side place/probe (thunk 0x1640, __thiscall, 6 args) and the bundle's
// per-unit commit (thunk 0x42e1, __thiscall on `this`, 1 arg). Reloc-masked externs.
// @early-stop
RVA(0x000358a0, 0x2d6)
i32 CBattlezMapConfig::RetargetIdleUnit(CGrunt* unit) {
    GruntzPlayer* recA = 0;
    CBattlezMapConfig* cfgB = 0;
    i32 cell = unit->m_arrivalCol;
    if (cell >= 0 && cell < 4) {
        recA = &m_ctx->m_options[cell];
        cfgB = &m_ctx->m_options[cell].m_038;
    }
    if (unit->CoordCount() == 0) {
        if (cell == -1) {
            if (static_cast<u32>(unit->m_dwell) <= static_cast<u32>(m_moveBudget)) {
                return 1;
            }
            i32 r = rand() % 4;
            if (r == m_curCell) {
                r++;
            }
            i32 band = r % 4;
            CBattlezMapConfig* b = &m_ctx->m_options[band].m_038;
            i32 cnt = b->m_0f0.GetSize();
            i32 x = b->m_markerX;
            i32 y = b->m_markerY;
            if (cnt != 0) {
                Coord** arr = CoordArrayData(b->m_0f0); // the CPtrArray band
                Coord* pair = arr[rand() % cnt];
                x = pair->m_x;
                y = pair->m_y;
            }
            if (unit->TileSwitch(x, y, 0, 0x9cf, 0, 0x4020) != 0) {
                unit->m_arrivalCol = band;
                unit->m_arrivalRow = 0;
                AcceptAlways(unit);
            }
            unit->m_dwell = 0;
            return 1;
        }
        CBattlezMapConfig* recB = &m_ctx->m_options[cell].m_038;
        if (recB == 0) {
            return 1;
        }
        if (static_cast<u32>(unit->m_dwell) <= 0x7d0) {
            return 1;
        }
        // NB the old spelling was `*(i32*)(recB + 0xd4)` on a CBattlezMapConfig* - that
        // is pointer arithmetic in WHOLE STRUCTS, i.e. 0xd4 * sizeof(*recB) bytes out.
        // +0xd0/+0xd4 are the declared m_markerX / m_markerY.
        i32 y = recB->m_markerY;
        i32 x = recB->m_markerX;
        unit->TileSwitch(x, y, 0, 0x987, 0, 0x4068);
        unit->m_dwell = 0;
        return 1;
    }
    if (recA == 0 || cfgB == 0) {
        unit->m_arrivalCol = -1;
        unit->m_arrivalRow = -1;
        return 1;
    }
    if (recA->m_014 == 0 && cfgB->m_active == 0) {
        CoordNode* n = unit->CoordHead();
        while (n != 0) {
            CoordNode* cur = n;
            n = n->m_next;
            if (cur->m_coord != 0) {
                g_coordPool.Push(cur->m_coord);
            }
        }
        unit->m_31c.RemoveAll();
        unit->m_arrivalCol = -1;
        unit->m_arrivalRow = -1;
        return 1;
    }
    i32 saved = unit->m_arrivalCol;
    static_cast<void>(saved);
    if (unit->m_arrivalRow == 1) {
        return 1;
    }
    CGameObject* lvl = unit->m_object;
    i32 px = lvl->m_screenX >> 5;
    i32 py = lvl->m_screenY >> 5;
    i32 nearBand = 0;
    // +0xf0 is the config's CPtrArray: +0xf4 is its data, +0xf8 its count
    i32 cnt2 = cfgB->m_0f0.GetSize();
    if (cnt2 > 0) {
        Coord** vec = CoordArrayData(cfgB->m_0f0);
        for (i32 j = cnt2; j > 0; j--) {
            Coord* pair = *vec;
            i32 dy = abs(pair->m_y - py);
            i32 dx = abs(pair->m_x - px);
            if (dx + dy <= 6) {
                nearBand = 1;
            }
            vec++;
        }
    }
    if (nearBand == 0) {
        return 1;
    }
    unit->m_arrivalCol = unit->m_arrivalCol;
    unit->m_arrivalRow = 1;
    if (unit->CoordCount() == 0) {
        return 1;
    }
    CoordNode* n = unit->CoordHead();
    while (n != 0) {
        CoordNode* cur = n;
        n = n->m_next;
        if (cur->m_coord != 0) {
            CoordPoolNode* slot = g_coordPool.NodeOf(cur->m_coord);
            slot->m_next = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = slot;
        }
    }
    unit->m_31c.RemoveAll();
    return 1;
}
