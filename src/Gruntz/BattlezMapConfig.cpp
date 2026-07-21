#include <Gruntz/Play.h> // complete CPlay (the m_10 spawn-info holder is the play state)
#include <Gruntz/TileTriggerLogic.h>
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GruntzPlayer.h>
#include <Io/FileMem.h> // the serialize stream (CSerialArchive == the real CFileMemBase)
#include <Gruntz/TileTriggerSwitchLogic.h>
#include <Gruntz/TileActionEvent.h> // CTileActionEvent - FindByField0C's real return type
#include <Gruntz/UserLogic.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/TypeKeyColl.h> // g_typeColl (anim registry)
#include <Gruntz/Brickz.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Wwd/WwdFile.h>
#include <Gruntz/GameLevel.h> // canonical CGameLevel/CLevelPlane (m_world->m_level visible rect)
#include <rva.h>

#include <Gruntz/CoordNode.h>    // the shared coord-list node
#include <Gruntz/FreeNodePool.h> // canonical coord free-pool (g_coordPool)
#include <Gruntz/BattlezMapConfig.h>
#include <Gruntz/TriggerMgr.h>     // the ONE CTriggerMgr (the local dup class is gone)
#include <Gruntz/GruntPuddle.h>    // CGruntPuddle (the m_baseList spawn-candidate element)
#include <Gruntz/MapMgr.h>         // CBrickzGrid == CMapMgr (the board / tile grid)
#include <DDrawMgr/DDrawChildGroup.h> // the level's game-object collection (ex CQueueDrainHost view)
#include <Wap32/zBitVec.h>         // zErrHandling (the zvec error-report target)
#include <Gruntz/ActReg.h>
#include <Gruntz/LevelInfo.h>      // the canonical CLevelInfo (LoadConfig arg1)
#include <Bute/ButeMgr.h>          // CButeMgr (LoadConfig reads the g_buteMgr singleton)
#include <Gruntz/GameRegistry.h>
#include <Gruntz/SerialArchive.h> // the shared CSerialArchive stream (Read @+0x2c / Write @+0x30)
#include <Globals.h>

#include <stdlib.h> // rand (0x11fee0, grid-scan neighbour pick); abs (branchless cdq/xor/sub)
#include <math.h>   // sqrt (CBattlezMapConfig::Step board-distance)
#pragma intrinsic(sqrt)
#include <string.h>     // strcmp (anim-name dispatch -> inline sbb/sbb byte compare)
#include <new>          // placement new (CRect in-place ctor)
#include <Wap32/Rect.h> // canonical CRect (0x29ac0 direct-store ctor, was local QuadIntRecord)
#include <Gruntz/TileTriggerContainer.h> // canonical CTileTriggerContainer (FindInLists12 0x116f20)

void* __stdcall ListNodeAdvance(void** pos);

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

extern "C" void __stdcall SetAtGrow(i32 arrayHandle, void* node);

extern "C" {
    void* CreateGruntCreationPoint(); // ILT thunk 0x17e4 (GameObjectFactory.cpp)
    void* CreateExitTrigger();        // ILT thunk 0x192e
    void* CreateWayPoint();           // ILT thunk 0x1087
}

static inline CGameObject* ListGetFirst(CDDrawChildGroup* list) {
    CDDrawGroupNode* n = reinterpret_cast<CDDrawGroupNode*>(list->m_list.GetHeadPosition());
    list->m_walkCursor = n;
    if (n == 0) {
        return 0;
    }
    list->m_walkCursor = n->m_next;
    return n->m_obj;
}

static inline CGameObject* ListGetNext(CDDrawChildGroup* list) {
    CDDrawGroupNode* n = list->m_walkCursor;
    if (n == 0) {
        return 0;
    }
    list->m_walkCursor = n->m_next;
    return n->m_obj;
}

// ===========================================================================
// CBattlezMapConfig::CBattlezMapConfig  @0x024dc0
// Member-constructs the four arrays (CPtrArray x2, CDWordArray x2) - the /GX
// compiler frames the ctor and advances the EH try-level after each constructed
// member - then seeds the scalar config block. Returns `this`.
// ===========================================================================
// @early-stop
// 93.3% - const-materialize/scheduling wall (docs/patterns/const-materialize-into-reg-vs-immediate.md).
// The body assignment order ALREADY matches retail's store order exactly (verified
// against --target); the two residuals are pure MSVC5 scheduling coin-flips:
//   (1) retail holds 0x7d0 in edx AND 0xbb8 in eax simultaneously (materializes
//       `mov edx,0x7d0` early, before the intervening =0 stores), reusing edx for the
//       three 0x7d0 stores (m_09c/0a0/0b8); our cl finishes the 0xbb8 stores then
//       reuses eax (`mov eax,0x7d0`) - same values/offsets, one register differs.
//   (2) the /GX member-init-list zero stores emit 78,7c,80,84 (declaration order)
//       but retail schedules them 78,80,7c,84 across the array-ctor calls.
// Neither is source-steerable (reordering the init list is a no-op - VC5 emits
// declaration order; reordering declarations would break the offsets). Final sweep.
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
        if (cur->m_7c->m_notify == reinterpret_cast<GameObjNotifyFn>(&CreateGruntCreationPoint) && cur->m_124 == id) {
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
        if (cur2->m_7c->m_notify == reinterpret_cast<GameObjNotifyFn>(&CreateExitTrigger) && cur2->m_124 == id) {
            m_markerX = cur2->m_screenX / 32;
            m_markerY = cur2->m_screenY / 32;
            break;
        }
    }

    // --- loop 3: append EVERY type-3 marker to the +0xf0 array, scaled by >>5
    //     (arithmetic floor), and set bit 0x10000 in the matched object's flags. ---
    for (CGameObject* cur3 = ListGetFirst(mgr->m_world->m_childGroup); cur3 != 0;
         cur3 = ListGetNext(mgr->m_world->m_childGroup)) {
        if (cur3->m_7c->m_notify == reinterpret_cast<GameObjNotifyFn>(&CreateWayPoint) && cur3->m_124 == id) {
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
            m_gruntCreationTime =
                static_cast<i32>((static_cast<double>(r) * (static_cast<double>(static_cast<i64>(m_gruntCreationTime)) * g_diffScale)));
            m_resourceCreationTime =
                static_cast<i32>((static_cast<double>(r) * (static_cast<double>(static_cast<i64>(m_resourceCreationTime)) * g_diffScale)));
            break;
        }
        case 2: { // Hard
            i32 r = g_buteMgr.GetIntDef("Battlez", "HardDifficulty", 25);
            g_diffTier = 5;
            m_gruntCreationTime =
                static_cast<i32>((static_cast<double>(r) * (static_cast<double>(static_cast<i64>(m_gruntCreationTime)) * g_diffScale)));
            m_resourceCreationTime =
                static_cast<i32>((static_cast<double>(r) * (static_cast<double>(static_cast<i64>(m_resourceCreationTime)) * g_diffScale)));
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
i32 CBattlezMapConfig::Method_025c20() {
    if (g_gameReg->m_options[m_curCell].m_014 == 0
        && g_gameReg->m_options[m_curCell].m_liveGate != 0) {
        for (i32 i = 0; i < m_candArray.GetSize(); i++) {
            this->Method_026470(0); // @0x26470 (the per-element refresh sibling)
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
// CBattlezMapConfig::Method_025d90  @0x025d90
// The per-tick board step. Run the two timers (claim/spawn budget via
// Method_026470, and a periodic re-pick), level off mode-3 units' countdowns,
// then scan the current cell-row for the one eligible unit (passes the cached-
// cell + clear-flags guards and is NOT one of the I/G/L/P/J/C/R type codes) whose
// countdown reached 0, transition it (state 0/5 + SetState), and on a 0x12/0x16
// mode recycle its coord nodes onto g_coordPool.m_freeHead. Decrement every mode-3 unit's
// countdown and advance the bundle's timers by g_frameDelta. Returns 1.
// ===========================================================================
// @early-stop
// large-state-machine plateau: the timer/budget head, the I/G/L/P/J/C/R anim-name
// dispatch (shared with Method_034460), the eligibility guards, the state
// transition + g_coordPool.m_freeHead recycle, and the post-loop countdown decrement are all
// reconstructed. Residual is the regalloc across the three 15-slot scans + the
// chosen-unit override local, and the foreign unit/level chains modeled by raw
// offset. Deferred to the final sweep.
RVA(0x00025d90, 0x580)
i32 CBattlezMapConfig::Method_025d90() {
    if (m_active == 0) {
        return 1;
    }
    if (m_ctx->m_cmdGrid == 0) {
        return 0;
    }
    if (m_spawnTimer - m_spawnLastFire > m_spawnInterval) {
        Method_026470(1);
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
                    Method_02f620(reinterpret_cast<i32>(u2));
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
                i32 idx = reinterpret_cast<i32>(unit->m_objAux->m_1c);
                i32 eq;
                eq = (strcmp((*g_typeColl.GetNameRecord(reinterpret_cast<void*>((idx)))), "I") == 0);
                if (eq) {
                    continue;
                }
                eq = (strcmp((*g_typeColl.GetNameRecord(static_cast<void*>((unit->m_objAux->m_1c)))), "G") == 0);
                if (eq) {
                    continue;
                }
                eq = (strcmp((*g_typeColl.GetNameRecord(static_cast<void*>((unit->m_objAux->m_1c)))), "L") == 0);
                if (eq) {
                    continue;
                }
                eq = (strcmp((*g_typeColl.GetNameRecord(static_cast<void*>((unit->m_objAux->m_1c)))), "P") == 0);
                if (eq) {
                    continue;
                }
                eq = (strcmp((*g_typeColl.GetNameRecord(static_cast<void*>((unit->m_objAux->m_1c)))), "J") == 0);
                if (eq) {
                    continue;
                }
                eq = (strcmp((*g_typeColl.GetNameRecord(static_cast<void*>((unit->m_objAux->m_1c)))), "C") == 0);
                if (eq) {
                    continue;
                }
                eq = (strcmp((*g_typeColl.GetNameRecord(static_cast<void*>((unit->m_objAux->m_1c)))), "R") == 0);
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
                if (Method_030530(reinterpret_cast<i32>(unit)) != 0) {
                    unit->m_defenderState = 5;
                } else {
                    unit->m_defenderState = 0;
                }
                (static_cast<CGrunt*>(unit))->LoadPickupSprites(unit->m_2e4, 0, 0, 1, 1);
                if (mode == 0x12 || (mode == 0x16 && unit->CoordCount() != 0)) {
                    GruntCoordNode* n = unit->CoordHead();
                    while (n != 0) {
                        GruntCoordNode* cur = n;
                        n = n->m_next;
                        if (cur->m_coord != 0) {
                            CoordPoolNode* node = g_coordPool.NodeOf(cur->m_coord);
                            node->m_next = g_coordPool.m_freeHead;
                            g_coordPool.m_freeHead = node;
                        }
                    }
                    unit->m_31c.RemoveAll();
                }
                break;
            }
            m_repickLastFire = m_repickTimer;
        }
    }
    winapi_0267c0_IntersectRect_PtInRect();
    m_spawnTimer += g_frameDelta;
    m_repickTimer += g_frameDelta;
    m_claimTimer += g_frameDelta;
    return 1;
}

// ===========================================================================
// CBattlezMapConfig::Method_026470  @0x026470
// Spawn/claim decision for the current cell-row: if the row is already at/over
// its per-level unit budget (rec->m_378) return early; otherwise scan the first
// CPtrArray (m_candArray) of candidate coords, skip ones whose tile carries the
// 0x20000000 "reserved" bit (unless they map to this row), map the first usable
// candidate to a screen cell via WorldToScreen + ProbeCell, and if that cell
// holds a unit, seed it as a fresh spawn (mode 4 / state 0x11 / a -1 coord block)
// gated by a g_diffScale-scaled budget compare. Returns 1.
// ===========================================================================
// @early-stop
// deep-chain regalloc plateau (~82%): logic + the grid/threshold scans, the
// WorldToScreen/ProbeCell/float-budget math, and the full spawn-field block are
// byte-exact in shape. Residual is pure register allocation: retail pins the row
// count in edx and the candidate index in ebp where MSVC5 here picks esi/eax, and
// the choice cascades through the two 15-slot scans' operands. The foreign render/
// level chains (m_ctx->m_world->m_24->m_5c) are modeled by raw offset. Final sweep.
RVA(0x00026470, 0x29d)
i32 CBattlezMapConfig::Method_026470(i32) {
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
    Coord** cands = reinterpret_cast<Coord**>(m_candArray.GetData());
    Coord* cand = 0;
    i32 i = 0;
    i32 tileRec[7];
    i32 slot38;
    for (;;) {
        cand = cands[i];
        i32 usable = 1;
        if (cand != 0) {
            i32* tilePtr = reinterpret_cast<i32*>(&(static_cast<BrickzCell*>((m_board)->m_rows[cand->m_y]))[cand->m_x]);
            for (i32 t = 0; t < 7; t++) {
                tileRec[t] = tilePtr[t];
            }
            usable = 1;
            if (tileRec[0] & 0x20000000) {
                if (static_cast<u8>(tileRec[1]) != static_cast<u8>(m_curCell)) {
                    usable = 0;
                }
                if (slot38 == 0) {
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
    m_ctx->m_world->m_level->m_mainPlane->SnapToTileCenter(reinterpret_cast<i32*>(&screen), cand->m_x << 5, cand->m_y << 5);
    i32 cell;
    if (slot38 != 0) {
        cell = m_ctx->m_cmdGrid->ProbeCell(
            m_curCell,
            screen.m_x,
            reinterpret_cast<void*>(0x186a0),
            2,
            reinterpret_cast<void*>(g_groupSentinel),
            0,
            0,
            0,
            0
        );
    } else {
        cell = m_ctx->m_cmdGrid->ProbeCell(
            m_curCell,
            screen.m_x,
            reinterpret_cast<void*>(0x186a0),
            0,
            reinterpret_cast<void*>(g_groupSentinel),
            0,
            0,
            0,
            0
        );
    }
    if (cell == -1) {
        return 0;
    }
    CGrunt* unit = (reinterpret_cast<CGrunt**>((m_ctx->m_cmdGrid)))[cell * 3 + m_curCell * 3];
    if (unit == 0) {
        return 0;
    }
    slot38 = rand() % 100;
    i32 freeCount = 0;
    CGrunt** r2 = &m_triggerMgr->m_grid[m_curCell * 15];
    for (i32 k = 15; k != 0; k--) {
        CGrunt* g = *r2;
        if (g != 0 && g->m_2d8 == 0) {
            freeCount++;
        }
        r2++;
    }
    i32 budget = static_cast<i32>((static_cast<double>(m_ctx->m_options[m_curCell].m_comboSel)
                       * static_cast<double>(m_budgetMul) * g_diffScale));
    if (slot38 >= m_spawnPct || freeCount >= budget) {
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
// CBattlezMapConfig::winapi_0267c0_IntersectRect_PtInRect  @0x267c0  (10269 B)
// The engine's single biggest function and #1 weighted match drag. TRIAGED: this is
// a GENUINE reconstruction gap (a bare `return 0` scoring 0.06%), NOT a scoring
// artifact - there is no reconstruction here yet to be mis-scored.
//
// STRUCTURE (mapped from the retail disasm; reconstructable, just very large): the
// master per-unit AI dispatch tick. `esi` is the CGrunt (the same CGrunt-family
// object Method_029b40 drives - reads m_1fc/m_220/m_1e4/m_368 eligibility guards,
// m_objAux->m_1c type key, m_308/30c/314/310/320/328/280 geometry). The body is:
//   * head (0x267c0..0x2690b): a screen-rect / board-rect intersect + PtInRect-style
//     geometry gate (the IntersectRect/PtInRect winapi imports at PTR_..006c4568/6c
//     that heuristically named this fn) + a first type-name probe.
//   * the CORE is a long, highly REGULAR chain of eligibility-gated type-dispatch
//     arms, each of the shape:
//        if (unit->m_entranceCommitted == 0) goto handler;            // + m_368/m_1e4/m_220 guards
//        name = *g_typeColl.IndexToPtr(unit->m_14->m_1c);   // zDArray lookup
//        if (strcmp(name, "<CODE>") == 0) goto handler;     // MSVC5-inlined strcmp
//     over ~9 distinct type-code string constants (?s_codeA / ?s_codeJ / s_codeI /
//     s_codeG / s_codeL / s_codeP / k_60bebc / k_60cc90 / k_60cc94), each arm then
//     branching into a per-type behaviour (coord recycle via g_coordPool, GetScreenPos
//     geometry, CRect clamps, CButeMgr::GetIntDef config reads, and hand-offs to the
//     sibling state-machine methods in this TU).
// callees (all named, reloc-masked): winapi_02c140/02ae00/02e3a0/031ca0/032060 (the
//   IntersectRect/PtInRect family), GetScreenPos, IndexToPtr (zDArray, x52),
//   CRect (x15), ListNodeAdvance, g_coordPool.Push, CPtrList::RemoveAll, RectContains,
//   FindGridNeighbor, ResolveArrival, Step/Step33520, Scan/ScanRegion32ce0,
//   Method_030530/02ed90/0350d0/034c70/0358a0, CGrunt::TileSwitch, ApplyTriggerA,
//   ResetEntranceAnimation, StepEntranceReinit, rand, CButeMgr::GetIntDef.
// ===========================================================================
// @early-stop
// deferred - SIZE wall, not an idiom/regalloc wall. A faithful reconstruction is
// tractable (the arm pattern is regular; every callee + type is already modeled - the
// CGrunt/g_typeColl/CBrickzGrid views used by Method_029b40 above cover it), but at
// 10269 B it is a dedicated multi-session leaf-first job, not a single-matcher batch.
// objdiff scores the WHOLE function's alignment, so any sub-complete partial (even a
// 1-2 KB faithful head) still scores ~0 and diverges its own regalloc - reconstructing
// a fraction buys no weighted credit and risks banking a wrong guess, so the honest
// state is the stub + this structural map. TOP-PRIORITY final-sweep target.
// @confidence: low
// @source: winapi:IntersectRect;PtInRect
// @stub
RVA(0x000267c0, 0x281d)
i32 CBattlezMapConfig::winapi_0267c0_IntersectRect_PtInRect() {
    return 0;
}

RVA(0x00029a30, 0x10)
void* __stdcall ListNodeAdvance(void** it) {
    char* cur = static_cast<char*>(*it);
    *it = *reinterpret_cast<void**>(cur);
    return cur + 8;
}

RVA(0x00029a50, 0x15)
void CUserLogic::GetScreenPos(ScreenPoint* out) {
    CWwdGameObjectA* o = m_object;
    i32 y = o->m_screenY;
    i32 x = o->m_screenX;
    out->m_x = x;
    out->m_y = y;
}

RVA(0x00029a80, 0x29)
i32 CUserLogic::IsAtSavedScreenPos() {
    CWwdGameObjectA* o = m_object;
    i32 sx = *reinterpret_cast<i32*>((reinterpret_cast<char*>(this) + 0x17c));
    if (o->m_screenX == sx && o->m_screenY == *reinterpret_cast<i32*>((reinterpret_cast<char*>(this) + 0x180))) {
        return 1;
    }
    return 0;
}

// @early-stop
// two-scratch reconstruction plateau (23.8% loose one-tile approx -> 58.4% faithful).
// The whole control-flow graph, the two tile-record stack copies (rep stos/movs), the
// arm bit dispatch, and the per-arm recyclers (g_coordPool.Push vs raw g_coordPool.m_freeHead +
// RemoveAt/RemoveAll) are byte-faithful; the permuter confirms 58.4% is the operand-
// permutation ceiling for this shape. Residual is genuine codegen/regalloc: (1) frame
// 0x5c vs retail 0x58 - one extra spill slot MSVC picks for the head; (2) the tile-
// fetch regions carry retail artifacts no clean C++ reproduces (a dead redundant
// GetScreenPos before scratchA, and cross-call register reuse - scratchB reads x from
// one screen probe and y from the other); (3) the head keeps this in [esp+0x8] pre-
// push where MSVC5 sinks it post-push; (4) deep arm regalloc. Final sweep / permuter.
RVA(0x00029b40, 0x813)
i32 CBattlezMapConfig::Method_029b40(i32 unitArg) {
    CGrunt* unit = reinterpret_cast<CGrunt*>(unitArg);
    CPtrList* coordList = &unit->m_31c;
    if (unit->CoordCount() == 0) {
        return 0;
    }
    // --- geometry-drift head: recycle-and-bail if the unit's stored first coord and
    //     its live screen cell have drifted >= 2 cells apart. ---
    GruntCoord* c0 = unit->CoordHead()->m_coord;
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
        CBrickzGrid* board = m_board;
        // tile0: is the unit's own coord cell blocked (low flag byte == 1)?
        i32 tile0;
        if (static_cast<u32>(ux) < static_cast<u32>(board->m_width) && static_cast<u32>(uy) < static_cast<u32>(board->m_height)) {
            tile0 = (static_cast<BrickzCell*>(board->m_rows[uy]))[ux].m_0;
        } else {
            tile0 = 1;
        }
        if (static_cast<u8>(tile0) == 1) {
            if (unit->CoordCount() == 0) {
                return 0;
            }
            GruntCoordNode* n = unit->CoordHead();
            while (n != 0) {
                GruntCoordNode* cur = n;
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
        if (static_cast<u32>(cx) < static_cast<u32>(board->m_width) && static_cast<u32>(cy) < static_cast<u32>(board->m_height)) {
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
            prim = unit->m_19c;
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
        if (static_cast<u32>(sgx) < static_cast<u32>(board->m_width) && static_cast<u32>(sgy) < static_cast<u32>(board->m_height)) {
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
                    GruntCoordNode* n = unit->CoordHead();
                    while (n != 0) {
                        GruntCoordNode* cur = n;
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
        i32 p11 = unit->m_entranceReason;
        if (p11 > 0x16) {
            p11 = unit->m_19c;
        }
        if (p11 == 0x11 && unit->CoordCount() >= 2) {
            GruntCoordNode* node = unit->CoordHead();
            GruntCoord* ca = node->m_coord;
            GruntCoordNode* nn = node->m_next;
            i32 ax = ca->m_x;
            GruntCoord* cb = nn->m_coord;
            i32 ay = ca->m_y;
            i32 bx = cb->m_x;
            i32 by = cb->m_y;
            i32 tB;
            if (static_cast<u32>(bx) < static_cast<u32>(board->m_width) && static_cast<u32>(by) < static_cast<u32>(board->m_height)) {
                tB = (static_cast<BrickzCell*>(board->m_rows[by]))[bx].m_0;
            } else {
                tB = 1;
            }
            if (tB & 0x20) {
                i32 tA2;
                if (static_cast<u32>(ax) < static_cast<u32>(board->m_width) && static_cast<u32>(ay) < static_cast<u32>(board->m_height)) {
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
        //     0x8000 gates a commit (mode 0xa) or a Method_030530 hand-off. ---
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
                    GruntCoordNode* n = unit->CoordHead();
                    while (n != 0) {
                        GruntCoordNode* cur = n;
                        n = n->m_next;
                        if (cur->m_coord != 0) {
                            g_coordPool.Push(cur->m_coord);
                        }
                    }
                    coordList->RemoveAll();
                }
                return 0;
            }
            if (Method_030530(reinterpret_cast<i32>(unit)) == 0 && unit->m_defenderState == 7) {
                GruntCoordNode* head = unit->CoordHead();
                if (head != 0) {
                    GruntCoordNode* n = head->m_next;
                    if (n != 0) {
                        while (n != 0) {
                            GruntCoordNode* cur = n;
                            n = n->m_next;
                            if (cur->m_coord != 0) {
                                CoordPoolNode* fn = g_coordPool.NodeOf(cur->m_coord);
                                fn->m_next = g_coordPool.m_freeHead;
                                g_coordPool.m_freeHead = fn;
                                coordList->RemoveAt(reinterpret_cast<POSITION>(cur));
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
                p = unit->m_19c;
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
                    p = unit->m_19c;
                }
                if (p == 0x16) {
                    return 1;
                }
                i32 p2 = unit->m_entranceReason;
                if (p2 > 0x16) {
                    p2 = unit->m_19c;
                }
                if (p2 == 0x12) {
                    return 1;
                }
            }
            i32 lo2 = sA & 0x2;
            if (lo2) {
                i32 p = unit->m_entranceReason;
                if (p > 0x16) {
                    p = unit->m_19c;
                }
                if (p == 0x16) {
                    return 1;
                }
            }
            if (Method_030b20(reinterpret_cast<i32>(unit), cx, cy) != 0) {
                return 1;
            }
            i32 sB = scratchB.m_0;
            if ((sB & 0x200) || (sB & 0x8)) {
                return 0;
            }
            if (hi && unit->m_defenderState != 3) {
                i32 pick = (rand() % 5) != 0 ? 0x12 : 0x16;
                Method_02c0a0(reinterpret_cast<i32>(unit), pick);
            }
            if (lo2) {
                if (unit->m_defenderState == 3) {
                    return 0;
                }
                Method_02c0a0(reinterpret_cast<i32>(unit), 0x16);
            }
            return 0;
        }
        // --- sA & 0x8 == 0 ---
        if ((sA & 0x20) && prim != 5 && prim != 0x11 && prim != 1) {
            if (unit->m_defenderState == 3) {
                return 0;
            }
            Method_02c0a0(reinterpret_cast<i32>(unit), 5);
            return 0;
        }
        if (sA & 0x40) {
            i32 p = unit->m_entranceReason;
            if (p > 0x16) {
                p = unit->m_19c;
            }
            if (p != 0x16) {
                if (prim == 0xd) {
                    return 0;
                }
                if (unit->m_defenderState == 3) {
                    return 0;
                }
                Method_02c0a0(reinterpret_cast<i32>(unit), 0xd);
                return 0;
            }
        }
        if (sA & 0x2) {
            i32 p = unit->m_entranceReason;
            if (p > 0x16) {
                p = unit->m_19c;
            }
            if (p == 0x16) {
                return 0;
            }
        }
        if (sA & 0x20000000) {
            winapi_02a570_IntersectRect(reinterpret_cast<i32>(unit));
            return 0;
        }
        i32 pk = unit->m_entranceReason;
        if (pk > 0x16) {
            pk = unit->m_19c;
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
                if ((static_cast<CGrunt*>(unit))->RectContains(ox * 0x20 + 0x10, oy * 0x20 + 0x10) != 0) {
                    m_triggerMgr->ApplyTriggerB(
                        unit->m_tileOwnerHi,
                        unit->m_tileOwnerLo,
                        ox * 0x20 + 0x10,
                        oy * 0x20 + 0x10
                    );
                    if (unit->CoordCount() != 0) {
                        GruntCoordNode* n = unit->CoordHead();
                        while (n != 0) {
                            GruntCoordNode* cur = n;
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
        GruntCoordNode* n = unit->CoordHead();
        while (n != 0) {
            GruntCoordNode* cur = n;
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
// CBattlezMapConfig::winapi_02a570_IntersectRect  @0x02a570  (/GX EH frame)
// The reserved-tile scatter reroute. For a unit that holds occupied coords, clamp
// the board dirty-rect to a 13x13 box around its screen coord (IntersectRect copy-
// back), then scan up to three of its coord-list nodes for one on a blocked (bit 0)
// tile (or its own tail coord); for such a node build the FindPath flag word from
// the unit's 0x12/0x16/0xe anim modes and ask CBrickzGrid::FindPath (flags 0x2000098f) for
// a route into a local CPtrList. On a route: recycle the route head + the unit's old
// coords onto g_coordPool.m_freeHead/g_coordPool, empty its coord list, AddTail the new route,
// re-clamp the board dirty-rect, stamp the unit's packed coord from the new tail, and
// return 1. Exhausting the three nodes re-clamps the board dirty-rect and returns 0.
// ===========================================================================
// @early-stop
// EH-frame + FindPath reroute plateau: the 13x13 box clamp, the 3-node blocked-tile
// scan, the 0x12/0x16/0xe FindPath-flag build, CPtrList(10)/FindPath, the g_coordPool.m_freeHead +
// g_coordPool recycles, the AddTail path-swap, and both dirty-rect re-clamps are
// reconstructed in shape + order (same family as Method_030b20 / Method_0302c0).
// Residual is the /GX cond-temp EH state machine (shared `je <unwind>` cleanup vs
// cl's per-return duplication), the deep-loop regalloc across the CPtrList walks, and
// the dead maybe-null box branch retail emits (shared with winapi_02c140/02dfa0).
// Foreign unit/board chains modeled by raw offset. Deferred to the final sweep.
RVA(0x0002a570, 0x4c6)
i32 CBattlezMapConfig::winapi_02a570_IntersectRect(i32 unitArg) {
    CGrunt* unit = reinterpret_cast<CGrunt*>(unitArg);
    if (unit->CoordCount() == 0) {
        return 1;
    }
    void* pos = unit->CoordHead();
    Coord center;
    (static_cast<CUserLogic*>(unit))->GetScreenPos((&center));
    CBrickzGrid* board = m_board;
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
    GruntCoord* tailCoord = (unit->CoordTail())->m_coord;
    i32 tx = tailCoord->m_x;
    i32 ty = tailCoord->m_y;
    i32 iter = 0;
    GruntCoordNode* node = *static_cast<GruntCoordNode**>(pos);
    while (node != 0 && iter < 3) {
        GruntCoordNode* cur = node;
        node = node->m_next;
        GruntCoord* coord = cur->m_coord;
        if (coord == 0) {
            continue;
        }
        i32 x = coord->m_x;
        i32 y = coord->m_y;
        i32 tile = (reinterpret_cast<i32*>(board->m_rows[y]))[x * 7];
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
            prim = unit->m_19c;
        }
        if (prim == 0x12) {
            flags = 0x100;
        }
        prim = unit->m_entranceReason;
        if (prim > 0x16) {
            prim = unit->m_19c;
        }
        if (prim == 0x16) {
            flags = 0x942;
        }
        prim = unit->m_entranceReason;
        if (prim > 0x16) {
            prim = unit->m_19c;
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
                    GruntCoordNode* p = unit->CoordHead();
                    while (p != 0) {
                        GruntCoordNode* c2 = p;
                        p = p->m_next;
                        if (c2->m_coord != 0) {
                            g_coordPool.Push(c2->m_coord);
                        }
                    }
                    unit->m_31c.RemoveAll();
                }
                // AddTail every route node's coord onto the unit's coord list.
                GruntCoordNode* q = reinterpret_cast<GruntCoordNode*>(list.GetHeadPosition());
                while (q != 0) {
                    GruntCoordNode* c3 = q;
                    q = q->m_next;
                    if (c3->m_coord != 0) {
                        unit->m_31c.AddTail(c3->m_coord);
                    }
                }
                // Re-clamp the board dirty-rect to the board bounds.
                RECT b1;
                static_cast<RECT*>(new (&b1) CRect(0, 0, board->m_width, board->m_height));
                RECT b2;
                RECT* p2 = static_cast<RECT*>(new (&b2) CRect(0, 0, board->m_width, board->m_height));
                RECT rc;
                rc.left = p2->left;
                rc.top = p2->top;
                rc.right = p2->right;
                rc.bottom = p2->bottom;
                if (!IntersectRect(&board->m_bounds, &rc, &b1)) {
                    board->m_bounds = rc;
                }
                board->m_gridW = board->m_bounds.right - board->m_bounds.left;
                board->m_gridH = board->m_bounds.bottom - board->m_bounds.top;
                GruntCoord* nt = (unit->CoordTail())->m_coord;
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
// CBattlezMapConfig::winapi_02ab80_PtInRect  @0x02ab80
// Build a RECT centered at (cx,cy) with half-extents (halfW,halfH); scan the
// four cell-bands (15 units each, skipping the current cell band m_curCell) for the
// nearest idle (m_364==0) unit whose grid coord is inside the rect. On a kind-0x36
// unit, keep it only with a ~5% roll. Track the manhattan-nearest; return it (0
// on none). __thiscall(cx,cy,halfW,halfH).
// ===========================================================================
// @early-stop
// regalloc/spill-choice wall: retail keeps arg0 (cx) live in ebp across the loop
// and spills the band-base induction var to a stack slot (frame 0x20); this
// reconstruction keeps the band-base in ebp and spills cx (frame 0x1c). Same live
// set, opposite recolor -> the [esp+N] offsets shift and cascade. Logic + offsets
// byte-exact otherwise (77.9%). Not source-steerable; deferred to the final sweep.
RVA(0x0002ab80, 0x15e)
i32 CBattlezMapConfig::winapi_02ab80_PtInRect(i32 cx, i32 cy, i32 halfW, i32 halfH) {
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
    return reinterpret_cast<i32>(best);
}

// ===========================================================================
// CBattlezMapConfig::Method_02ad40  @0x02ad40
// Pick a random idle unit from one of the four cell-bands: roll a band [0..3]
// (avoiding the current cell index m_curCell by bumping past it), a random start cell
// [0..14], then scan the band's 15 units from there (cell index wrapping mod 15),
// returning the first non-null unit whose +0x364 "busy" slot is clear (0 on miss).
// The arg is unused. (__thiscall, ret 0x4.)
// ===========================================================================
// @early-stop
// regalloc wall: the double rand()%4 band-pick (with the m_curCell skip; the compare
// is `m_curCell == band` so the m_curCell load schedules early like retail), the
// rand()%15 start, and the 15-cell scan (incl. the dead running-cell-index recompute
// via idiv 15) are reconstructed in shape, but retail pins the row walker in esi / the
// m_364 temp + the 15 const in edi where MSVC5 here swaps them (edi walker, esi
// counter), and the swap cascades through the small body. Logic + offsets correct;
// the residual reg-swap is not source-steerable (permuter-confirmed). Final sweep.
RVA(0x0002ad40, 0x71)
void* CBattlezMapConfig::Method_02ad40(i32) {
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
// nearby cell (Method_0300c0, flags 0x20000d87). Returns 1 (0 on the eligibility rejects).
// ===========================================================================
// @early-stop
// string-dispatch + box-clamp plateau: the five inline-strcmp J/C/R/G/L rejects (the
// bool-local setcc form, docs/patterns/strcmp-eq-bool-local-setcc.md), the rand()%4
// gate, all three reloc-masked helper calls (RectContainsGated / ApplyTriggerB /
// CommitNeighbor), the prim==0x11 gate, and the box build + IntersectRect clamp +
// Method_0300c0 re-path are reconstructed in shape + order. Residual is the box-tail
// stack-slot schedule (the rand-offset dest coords + the dead maybe-null box branch
// retail emits, shared with winapi_02c140/02dfa0) and the foreign unit/level chains
// modeled by raw offset. Deferred to the final sweep.
// Clear_02ade0 (0x2ade0) - clear the active flag.
RVA(0x0002ade0, 0x7)
void CBattlezMapConfig::Clear_02ade0() {
    m_active = 0;
}

RVA(0x0002ae00, 0x42e)
i32 CBattlezMapConfig::winapi_02ae00_IntersectRect(i32 unitArg, i32 targetArg) {
    CGrunt* unit = reinterpret_cast<CGrunt*>(unitArg);
    if (unit->m_entranceCommitted == 0) {
        return 0;
    }
    bool eq;
    eq = (strcmp((*g_typeColl.GetNameRecord(static_cast<void*>((unit->m_objAux->m_1c)))), "J") == 0);
    if (eq) {
        return 0;
    }
    eq = (strcmp((*g_typeColl.GetNameRecord(static_cast<void*>((unit->m_objAux->m_1c)))), "C") == 0);
    if (eq) {
        return 0;
    }
    eq = (strcmp((*g_typeColl.GetNameRecord(static_cast<void*>((unit->m_objAux->m_1c)))), "R") == 0);
    if (eq) {
        return 0;
    }
    eq = (strcmp((*g_typeColl.GetNameRecord(static_cast<void*>((unit->m_objAux->m_1c)))), "G") == 0);
    if (eq) {
        return 0;
    }
    eq = (strcmp((*g_typeColl.GetNameRecord(static_cast<void*>((unit->m_objAux->m_1c)))), "L") == 0);
    if (eq) {
        return 0;
    }
    if (unit->m_gruntKind == 0x36) {
        return 0;
    }
    if (unit->m_entranceDropActive != 0) {
        return 0;
    }
    CGrunt* tgt = reinterpret_cast<CGrunt*>(targetArg);
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
        prim = tgt->m_19c;
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
    CBrickzGrid* board = m_board;
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
    Method_0300c0(targetArg, xcoord, ycoord, 0x20000d87, 0, 0);
    board->Clip(static_cast<const RECT*>(0));
    return 1;
}

RVA(0x0002b420, 0x419)
i32 CBattlezMapConfig::Serialize_02b420(void* arArg) {
    CSerialArchive* ar = static_cast<CSerialArchive*>(arArg);
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
// CBattlezMapConfig::Deserialize_02b950  @0x02b950
// The read mirror of Serialize_02b420: pull every config scalar back out of the
// archive through its Read(buf,count) vtable slot (+0x2c), then the four growable
// arrays + the inline 4-dword block. The two CDWordArrays read a count then that
// many dwords; the two CPtrArrays first recycle their current element nodes onto
// g_coordPool.m_freeHead, resize, then allocate one fresh node per element (8-byte payload).
// ===========================================================================
// @early-stop
// 99.5% - regalloc/scheduling wall in the two freelist-pop alloc loops. Retail
// emits `mov edx,ecx; lea ebx,[eax+4]; mov g_coordPool.m_freeHead,edx` (store the popped
// *node via an edx copy, AFTER the payload lea) and picks ecx for the m_pData
// reload; every source spelling tried lowers to the equivalent direct
// `mov g_coordPool.m_freeHead,ecx` (no copy) + eax for m_pData. The two forms are pure
// register-scheduling noise - proven by CTriggerMgr's own two structurally
// identical alloc loops compiling to BOTH (0x7ad40 direct-ecx vs 0x7ad9b
// edx-copy). ~8 residual bytes across 1299; all logic byte-exact otherwise.
RVA(0x0002b950, 0x513)
i32 CBattlezMapConfig::Deserialize_02b950(void* arArg) {
    CSerialArchive* ar = static_cast<CSerialArchive*>(arArg);
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
i32 CBattlezMapConfig::Method_02bfc0(i32 objArg, void* kindArg, i32, i32) {
    CSerialArchive* obj = reinterpret_cast<CSerialArchive*>(objArg);
    i32 kind = static_cast<i32>(reinterpret_cast<i32>(kindArg));
    switch (kind) {
        case 4:
            if (this->Serialize_02b420(obj) == 0) { // kind-4 validator @0x2b420
                return 0;
            }
            break;
        case 7:
            if (this->Deserialize_02b950(obj) == 0) { // kind-7 validator @0x2b950
                return 0;
            }
            break;
    }
    char* scratch = reinterpret_cast<char*>(&m_scratch78);
    switch (kind) {
        case 4:
            obj->Write(scratch, 8);
            scratch += 8;
            obj->Write(scratch, 8);
            break;
        case 7:
            obj->Read(scratch, 8);
            scratch += 8;
            obj->Read(scratch, 8);
            break;
    }
    return 1;
}

RVA(0x0002c080, 0x8)
i32 CBattlezMapConfig::Method_02c080(i32) {
    return 1;
}

RVA(0x0002c0a0, 0x78)
i32 CBattlezMapConfig::Method_02c0a0(i32 unitArg, i32 value) {
    CGrunt* unit = reinterpret_cast<CGrunt*>(unitArg);
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

extern "C" void Handler_0040288d(void);

// ===========================================================================
// CBattlezMapConfig::winapi_02c140_IntersectRect_PtInRect  @0x02c140
// For an idle unit (m_gruntKind==0, prim anim clear) clamp the board dirty-rect to an 8x8
// box centered on the unit (four GetCoord corner reads + the IntersectRect copy-back
// idiom), then iterate the scene collection (m_ctx->m_world->m_childGroup) for a kind-matching
// (m_7c handler == 0x40288d), non-flagged (m_40&1), in-box unit; on the first such
// unit re-path this unit toward it (Method_0300c0, flags 0x2000098b), re-clamp the
// dirty-rect, and return 1. Exhausting the collection tails into board Clip + return 0.
// ===========================================================================
// @early-stop
// iterator + reloc + stack-slot plateau: the box build, the IntersectRect clamp, the
// PtInRect gate, the all-same-target anim switch (0x33..0x40 jump table - table data is
// a delinker scoring artifact, docs/patterns/switch-jumptable-separate-comdat.md), the
// class-identity handler compare (reloc-masked immediate), both Method_0300c0 arms + the
// two re-clamp variants are reconstructed in shape + order. Two residuals: (1) the
// loop back-edge - retail inlines the FIRST GetNext pop + tail-calls the helper where a
// natural call re-emits it; (2) the dead maybe-null box branch retail emits (shared with
// winapi_02dfa0). Foreign scene/board chains modeled by raw offset. Deferred to the final sweep.
RVA(0x0002c140, 0x3e7)
i32 CBattlezMapConfig::winapi_02c140_IntersectRect_PtInRect(i32 unitArg) {
    CGrunt* unit = reinterpret_cast<CGrunt*>(unitArg);
    if (unit->m_gruntKind != 0) {
        return 0;
    }
    i32 prim = unit->m_entranceReason;
    if (prim > 0x16) {
        prim = unit->m_19c;
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
    CBrickzGrid* board = m_board;
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
    coll->m_scanCursor = reinterpret_cast<CDDrawGroupNode*>(coll->m_list.GetHeadPosition());
    CGameObject* g = static_cast<CGameObject*>(coll->Drain_031250());
    while (g != 0) {
        if (g->m_7c->m_notify == reinterpret_cast<GameObjNotifyFn>(Handler_0040288d) && (g->m_stateFlags & 1) == 0) {
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
                    if (Method_0300c0(unitArg, gx, gy, 0x2000098b, 0, 0) != 0) {
                        CBrickzGrid* bd = m_board;
                        RECT mb;
                        mb.left = 0;
                        mb.top = 0;
                        mb.right = bd->m_width;
                        mb.bottom = bd->m_height;
                        RECT tmp;
                        RECT* p = static_cast<RECT*>(new (&tmp) CRect(0, 0, bd->m_width, bd->m_height));
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
                    i32 p2 = unit->m_entranceReason;
                    if (p2 > 0x16) {
                        p2 = unit->m_19c;
                    }
                    if (p2 == 0) {
                        if (Method_0300c0(unitArg, gx, gy, 0x2000098b, 0, 0) != 0) {
                            CBrickzGrid* bd = m_board;
                            RECT r1;
                            static_cast<RECT*>(new (&r1) CRect(0, 0, bd->m_width, bd->m_height));
                            RECT r2;
                            RECT* p2r = static_cast<RECT*>(new (&r2) CRect(0, 0, bd->m_width, bd->m_height));
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
            CDDrawGroupNode* nd = c->m_scanCursor;
            c->m_scanCursor = nd->m_next;
            CGameObject* pp = nd->m_obj;
            if (pp->GetClassId() == CLASSID_SERIALREF) {
                g = pp;
            } else {
                g = static_cast<CGameObject*>(c->Drain_031250());
            }
        }
    }
    board->Clip(static_cast<const RECT*>(0));
    return 0;
}

#define ARR_RECYCLE(g)                                                                             \
    if ((g)->CoordCount() != 0) {                                                                  \
        GruntCoordNode* nd = (g)->CoordHead();                                                     \
        while (nd != 0) {                                                                          \
            GruntCoordNode* cur = nd;                                                              \
            nd = nd->m_next;                                                                       \
            if (cur->m_coord != 0) {                                                               \
                g_coordPool.Push(cur->m_coord);                                                    \
            }                                                                                      \
        }                                                                                          \
        (g)->m_31c.RemoveAll();                                                                    \
    }

static __inline i32 arrCell(CBrickzGrid* grid, i32 col, i32 row) {
    if (static_cast<u32>(col) < static_cast<u32>(grid->m_width) && static_cast<u32>(row) < static_cast<u32>(grid->m_height)) {
        return grid->m_rows[row][col].m_0;
    }
    return 1;
}

// @early-stop
// The whole COMMON path is complete + logic-
// correct (verified vs `sema disasm --diff`): Gate1a14(g) gate; the list-cached m_328 latch
// (esi=&g->m_31c, read [esi+0xc]); the double-GetTilePos dest cell grid[gy][bx] AND the own
// cell grid[fcy][fcx] (fcy = coord->y, was wrongly fcx twice) with the 0x01010101 OOB self-
// fill; maskFlags = own.m_0 & ~0x20000000; type = (m_170>0x16?m_19c:m_170); and ALL the
// flag/type handlers in retail order - door(0x400)/doorbody(0x4,FindChild==2)/0x8000-t3/
// 0x4000-t3(!=0x99)/0x200/0x8(Probe+Effect0x12)/0x20(t1 Move, t0x11 3x3 scan)/0x4000-tf/
// 0x8000-tf(FindByField0C+Impact/Move)/0x20-t5(SetCell/Impact/Move)/0x40(td Move / Effect0xd)/
// the neighbour-pick fallback (SelfImpact+Ready, rand()%3 origin scan + Trigger1640). Every
// callee owner corrected: this==g->m_10 (grid=this->m_c, Move via this->m_8, Find via this->
// m_14, Effect/Probe/Impact/Ready on this; origin g->m_object->m_screenX/m_60). The CBattlezMapConfig view had
// a missing char _00[8] (all this-> offsets were 8 low) - fixed.
//
// Residual is TWO walls, both proven with `sema disasm --base/--target`:
//   (1) REGALLOC SWAP (dominant, ~unclimbable): retail colours g->ebp and SPILLS this to
//       [esp+0x10]; our MSVC5 colours this->ebp and g->edi. Identical instruction stream +
//       logic, but every g-member/this-member ref uses the opposite base register (ebp<->edi),
//       so the modrm bytes differ throughout. Same allocator wall documented for
//       CBattlezMapConfig::Step (GruntMoveStep.cpp) - "no source spelling reassigns the callee-saved
//       register". Routing the board through g->m_10 DOES flip g->ebp but adds an [ebp+0x10]
//       reload per cluster (base grows), netting -0.8% - not worth it.
//   (2) DOOR-OPEN transform (off the common path, own recheck-DCE wall): retail's 0x1ad..0x46e
//       block is 3x GetTilePos + QuadIntRecord + IntersectRect + a per-cell CString-EH nested
//       loop (SearchEdge 0x20f4 / RemoveHead / g_coordPool.m_freeHead push) + a 0x2d31b cleanup tail. It
//       contains a dead stack-address null-recheck (`lea edx,[esp+0x38]; test edx,edx; je`) our
//       stronger MSVC5 DCE eliminates (cf. docs/patterns/dead-unreachable-recheck-block-dce.md),
//       so it can't reach byte-exact regardless. Reconstructed as a structural CString-EH loop
//       (forces the /GX frame); the exact 700-B transform + tail are a dedicated final-sweep job.
RVA(0x0002c690, 0xdb4)
i32 CBattlezMapConfig::ResolveArrival(CGrunt* g) {
    if (Gate1a14(g)) {
        return 1;
    }
    if (g->CoordCount() == 0) {
        return 0;
    }

    GruntCoord* fc = g->CoordHead()->m_coord;
    i32 fcx = fc->m_x; // grunt head coord x (long-lived)
    i32 fcy = fc->m_y; // grunt head coord y

    GruntTilePos a;
    g->GetTilePos(&a);
    i32 gy = a.m_y >> 5;
    i32 gx = a.m_x >> 5;
    GruntTilePos b;
    g->GetTilePos(&b);
    i32 bx = b.m_x >> 5;

    // destination cell = grid[gy][bx]; OOB fills the dest buffer itself (self-copy).
    BrickzCell dest;
    BrickzCell* dsrc;
    if (static_cast<u32>(bx) < static_cast<u32>(m_board->m_width) && static_cast<u32>(gy) < static_cast<u32>(m_board->m_height)) {
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
    if (static_cast<u32>(fcx) < static_cast<u32>(m_board->m_width) && static_cast<u32>(fcy) < static_cast<u32>(m_board->m_height)) {
        osrc = &m_board->m_rows[fcy][fcx];
    } else {
        memset(&own, 1, 0x1c);
        osrc = &own;
    }
    own = *osrc;

    i32 maskFlags = own.m_0 & 0xdfffffff;
    i32 type = (g->m_entranceReason > 0x16) ? g->m_19c : g->m_entranceReason;

    // ---- door (dest.flags & 0x400, m_2d4==3, type!=8) ----
    if ((dest.m_0 & 0x400) && g->m_defenderState == 3 && type != 8) {
        if (own.m_0 & 0x4000) {
            // door-open transform: build a search rect from the grunt pos, then a per-cell
            // CString-EH loop recycling edge nodes onto g_coordPool.m_freeHead. (Byte-walled: retail
            // emits a dead stack-address null-recheck our MSVC5 DCEs - see @early-stop.)
            GruntTilePos da;
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
        if (!IntersectRect(&m_board->m_bounds, static_cast<RECT*>(&tmp), static_cast<RECT*>(&corners))) {
            m_board->m_bounds = tmp;
        }
        m_board->m_gridW = m_board->m_bounds.right - m_board->m_bounds.left;
        m_board->m_gridH = m_board->m_bounds.bottom - m_board->m_bounds.top;
    }

    // ---- door body flag (dest.flags & 4) ----
    if ((dest.m_0 & 4) && g->m_2d8 != 0xb) {
        GruntTilePos tp;
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
        if (Probe1a4b(g, fcx, fcy) != 0) {
            return 1;
        }
        Effect374c(g, 0x12);
    }

    // ---- 0x20 -> type dispatch ----
    if (maskFlags & 0x20) {
        i32 t = (g->m_entranceReason > 0x16) ? g->m_19c : g->m_entranceReason;
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
                    if (static_cast<u32>(col) < static_cast<u32>(m_board->m_width) && static_cast<u32>(row) < static_cast<u32>(m_board->m_height)) {
                        i32 cf = arrCell(m_board, col, row);
                        if (cf & 0x939) {
                            return 1;
                        }
                        if (g->IsInCombatRange((col << 5) + 0x10, (row << 5) + 0x10) != 0) {
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
        i32 t = (g->m_entranceReason > 0x16) ? g->m_19c : g->m_entranceReason;
        if (t == 0xf) {
            CTileActionEvent* r = m_cellQuery->FindByField0C((fcx << 8) + fcy);
            if (r != 0) {
                if (r->m_playerFlags[m_curCell] != 0) {
                    ARR_RECYCLE(g);
                    Impact25e5(g, fcx, fcy, 1);
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
        i32 t = (g->m_entranceReason > 0x16) ? g->m_19c : g->m_entranceReason;
        if (t == 0xf) {
            ARR_RECYCLE(g);
            Impact25e5(g, fcx, fcy, 1);
            return 1;
        }
    }

    // ---- 0x20 path, type 5 ----
    if (maskFlags & 0x20) {
        i32 t = (g->m_entranceReason > 0x16) ? g->m_19c : g->m_entranceReason;
        if (t == 5) {
            if (maskFlags & 0x4000) {
                CTileActionEvent* r = m_cellQuery->FindByField0C((fcx << 8) + fcy);
                if (r != 0) {
                    i32 k = r->m_actionCode;
                    if (r->m_playerFlags[m_curCell] != 0) {
                        if (k == 0x13e || k == 0x140 || k == 0x143) {
                            Impact25e5(g, fcx, fcy, 0);
                        }
                    } else {
                        if (k == 0x13e || k == 0x140 || k == 0x143) {
                            (static_cast<CTileTriggerContainer*>(m_cellQuery))->SetCell(fcx, fcy, m_curCell);
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
        Effect374c(g, 5);
        return 0;
    }

    // ---- 0x40 path ----
    if (maskFlags & 0x40) {
        i32 t = (g->m_entranceReason > 0x16) ? g->m_19c : g->m_entranceReason;
        if (t != 0x16) {
            i32 t2 = (g->m_entranceReason > 0x16) ? g->m_19c : g->m_entranceReason;
            if (t2 == 0xd) {
                m_triggerMgr->ApplyTriggerA(
                    g->m_tileOwnerHi,
                    g->m_tileOwnerLo,
                    (fcx << 5) + 0x10,
                    (fcy << 5) + 0x10
                );
                return 0;
            }
            Effect374c(g, 0xd);
            return 0;
        }
    }

    // ---- neighbour pick fallback ----
    SelfImpact2b58(g, 0, 0, 0);
    if (Ready27ed(g) != 0) {
        return 1;
    }
    {
        i32 t = (g->m_entranceReason > 0x16) ? g->m_19c : g->m_entranceReason;
        if (t == 0x16) {
            return 1;
        }
    }
    {
        i32 oy = g->m_object->m_screenY >> 5;
        i32 ox = g->m_object->m_screenX >> 5;
        i32 row = rand() % 3 + oy - 1;
        i32 col = rand() % 3 + ox - 1;
        if (static_cast<u32>(col) >= static_cast<u32>(m_board->m_width) || static_cast<u32>(row) >= static_cast<u32>(m_board->m_height)) {
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
// register-coloring wall (logic byte-shaped & complete). All arms reconstructed:
// 0x8000/0x4000 tile-bit tests (test bh), the arg-`unit`->m_object corner reads, the
// a5-gated occupancy branch, the special anim-id set, the arm1/2/3 commits (g_stepRun/
// Col/Row + g_coordPool recycle), the 0x20000 RMW visited-mark, the 8-neighbour
// self-recursion, and single ~CPtrList-per-list scope teardown. Residual: retail
// colours word->ebx, col->ebp, row->edi and spills tileOff@[esp+0x10]; MSVC here
// keeps tileOff in a callee-saved reg and spills word, so the whole body's stack
// offsets shift +0x20 (frame 0x40 vs 0x60) and every reg operand diverges. Not
// source-steerable; a permuter target for the final sweep.
RVA(0x0002d800, 0x605)
i32 CBattlezMapConfig::Method_02d800(i32 a4, i32 col, i32 row, i32 a5) {
    if (g_stepRun == 0) {
        return 0;
    }
    for (;;) {
        i32 tileOff = ((col * 7) << 2);
        i32 word = *reinterpret_cast<i32*>((reinterpret_cast<char*>(m_board->m_rows[row]) + tileOff));
        if (word & 0x8000) {
            CPtrList list(10);
            CGameObject* lvl = (reinterpret_cast<CGrunt*>(a4))->m_object;
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
                    GruntCoordNode* n = static_cast<GruntCoordNode*>(head);
                    while (n != 0) {
                        GruntCoordNode* cur = n;
                        n = n->m_next;
                        CoordPoolNode* node = g_coordPool.NodeOf(cur->m_coord);
                        node->m_next = g_coordPool.m_freeHead;
                        g_coordPool.m_freeHead = node;
                    }
                }
                return 0;
            }
        }
        if (word & 0x4000) {
            void* cell = m_cellQuery->FindByField0C((col << 8) + row);
            if (a5 != 0) {
                if (cell == 0) {
                    break;
                }
                if (*reinterpret_cast<i32*>((reinterpret_cast<char*>(cell) + m_curCell * 4 + 0x18)) != 0) {
                    break;
                }
                CPtrList list2(10);
                CGameObject* lvl = (reinterpret_cast<CGrunt*>(a4))->m_object;
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
                        GruntCoordNode* n = static_cast<GruntCoordNode*>(head);
                        while (n != 0) {
                            GruntCoordNode* cur = n;
                            n = n->m_next;
                            CoordPoolNode* node = g_coordPool.NodeOf(cur->m_coord);
                            node->m_next = g_coordPool.m_freeHead;
                            g_coordPool.m_freeHead = node;
                        }
                    }
                }
                break;
            }
            if (cell == 0) {
                break;
            }
            i32 id = *static_cast<i32*>(cell);
            i32 special = 0;
            i32 occ = *reinterpret_cast<i32*>((reinterpret_cast<char*>(cell) + m_curCell * 4 + 0x18));
            if (occ == 0) {
                special = 1;
            } else if (id == 0x132 || id == 0x134 || id == 0x137 || id == 0x144 || id == 0x146
                       || id == 0x149 || id == 0x138 || id == 0x13a || id == 0x13d || id == 0x12f
                       || id == 0x130 || id == 0x131) {
                special = 1;
            }
            if (special == 0) {
                break;
            }
            CPtrList list3(10);
            CGameObject* lvl = (reinterpret_cast<CGrunt*>(a4))->m_object;
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
                    GruntCoordNode* n = static_cast<GruntCoordNode*>(head);
                    while (n != 0) {
                        GruntCoordNode* cur = n;
                        n = n->m_next;
                        CoordPoolNode* node = g_coordPool.NodeOf(cur->m_coord);
                        node->m_next = g_coordPool.m_freeHead;
                        g_coordPool.m_freeHead = node;
                    }
                }
            }
            break;
        }
        // Mark this tile visited, then recurse into the 8 neighbours. Each block:
        // in bounds + not visited (0x20000) + passable (0xc0000 set or anim 0x9a).
        *reinterpret_cast<i32*>((reinterpret_cast<char*>(m_board->m_rows[row]) + tileOff)) |= 0x20000;
        i32 cm = col - 1;
        i32 cp = col + 1;
        i32 rm = row - 1;
        i32 rp = row + 1;
        CBrickzGrid* b;
        i32* nt;
        i32 nw;

        b = m_board;
        if (static_cast<u32>(cm) < static_cast<u32>(b->m_width)) {
            nt = reinterpret_cast<i32*>((reinterpret_cast<char*>(b->m_rows[row]) + ((cm * 7) << 2)));
            nw = *nt;
            if (!(nw & 0x20000) && ((nw & 0xc000) || nt[4] == 0x9a)) {
                Method_02d800(a4, cm, row, a5);
            }
        }
        b = m_board;
        if (static_cast<u32>(cp) < static_cast<u32>(b->m_width)) {
            nt = reinterpret_cast<i32*>((reinterpret_cast<char*>(b->m_rows[row]) + ((cp * 7) << 2)));
            nw = *nt;
            if (!(nw & 0x20000) && ((nw & 0xc000) || nt[4] == 0x9a)) {
                Method_02d800(a4, cp, row, a5);
            }
        }
        b = m_board;
        if (static_cast<u32>(rm) < static_cast<u32>(b->m_width)) {
            nt = reinterpret_cast<i32*>((reinterpret_cast<char*>(b->m_rows[rm]) + ((col * 7) << 2)));
            nw = *nt;
            if (!(nw & 0x20000) && ((nw & 0xc000) || nt[4] == 0x9a)) {
                Method_02d800(a4, col, rm, a5);
            }
        }
        b = m_board;
        if (static_cast<u32>(rp) < static_cast<u32>(b->m_width)) {
            nt = reinterpret_cast<i32*>((reinterpret_cast<char*>(b->m_rows[rp]) + ((col * 7) << 2)));
            nw = *nt;
            if (!(nw & 0x20000) && ((nw & 0xc000) || nt[4] == 0x9a)) {
                Method_02d800(a4, col, rp, a5);
            }
        }
        b = m_board;
        if (static_cast<u32>(cp) < static_cast<u32>(b->m_width) && static_cast<u32>(rm) < static_cast<u32>(b->m_height)) {
            nt = reinterpret_cast<i32*>((reinterpret_cast<char*>(b->m_rows[rm]) + ((cp * 7) << 2)));
            nw = *nt;
            if (!(nw & 0x20000) && ((nw & 0xc000) || nt[4] == 0x9a)) {
                Method_02d800(a4, cp, rm, a5);
            }
        }
        b = m_board;
        if (static_cast<u32>(cp) < static_cast<u32>(b->m_width) && static_cast<u32>(rp) < static_cast<u32>(b->m_height)) {
            nt = reinterpret_cast<i32*>((reinterpret_cast<char*>(b->m_rows[rp]) + ((cp * 7) << 2)));
            nw = *nt;
            if (!(nw & 0x20000) && ((nw & 0xc000) || nt[4] == 0x9a)) {
                Method_02d800(a4, cp, rp, a5);
            }
        }
        b = m_board;
        if (static_cast<u32>(cm) < static_cast<u32>(b->m_width) && static_cast<u32>(rp) < static_cast<u32>(b->m_height)) {
            nt = reinterpret_cast<i32*>((reinterpret_cast<char*>(b->m_rows[rp]) + ((cm * 7) << 2)));
            nw = *nt;
            if (!(nw & 0x20000) && ((nw & 0xc000) || nt[4] == 0x9a)) {
                Method_02d800(a4, cm, rp, a5);
            }
        }
        b = m_board;
        if (static_cast<u32>(cm) < static_cast<u32>(b->m_width) && static_cast<u32>(rm) < static_cast<u32>(b->m_height)) {
            nt = reinterpret_cast<i32*>((reinterpret_cast<char*>(b->m_rows[rm]) + ((cm * 7) << 2)));
            nw = *nt;
            if (!(nw & 0x20000) && ((nw & 0xc000) || nt[4] == 0x9a)) {
                Method_02d800(a4, cm, rm, a5);
            }
        }
        if (g_stepRun == 0) {
            break;
        }
    }
    return 0;
}

// ===========================================================================
// CBattlezMapConfig::winapi_02dfa0_IntersectRect  @0x02dfa0
// The flood-fill launcher. Arm g_stepRun, build a 17x17 box around the unit's
// (>>5) coord (three GetCoord reads for the corners), clamp the board dirty-rect
// to that box intersected with the board bounds (the IntersectRect copy-back
// idiom), then run the recursive flood-fill (Method_02d800). If it committed
// (g_stepRun cleared), read the tile under the unit (and, when it has a live coord
// list, the tile under its tail coord); when a blocked (bit 0x4) tile is seen,
// stamp the unit's packed coord and place it at the committed cell (Method_4b320,
// g_stepCol/g_stepRow, flags 0x9c3). Finally clear the 0x2 bit across the dirty
// region and re-clamp the board dirty-rect to the board bounds.
// ===========================================================================
// @early-stop
// flood-fill-driver stack-slot plateau: logic + every call (the three GetCoords,
// IntersectRect x2, Method_02d800, Method_4b320) is reconstructed in shape + order,
// and the box/clamp/tile-read/clear-loop arithmetic is byte-shaped. Residual is the
// documented overlapping stack-slot schedule of the box + the two dirty-rect
// clamps (shared with GruntPathScan's SCAN_BOUNDS + 031ca0) and the dead
// maybe-null box branch retail emits; foreign unit/board chains modeled by raw
// offset. Deferred to the final sweep.
RVA(0x0002dfa0, 0x325)
i32 CBattlezMapConfig::winapi_02dfa0_IntersectRect(i32 unitArg, i32 a1, i32 a2, i32 a3) {
    CGrunt* unit = reinterpret_cast<CGrunt*>(unitArg);
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
    CBrickzGrid* board = m_board;
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
    Method_02d800(unitArg, a1, a2, a3);
    if (g_stepRun == 0) {
        i32 savedX = unit->m_entrancePxX;
        i32 savedY = unit->m_entrancePxY;
        i32 col = unit->m_entrancePxX >> 5;
        i32 row = unit->m_entrancePxY >> 5;
        i32 tile0;
        if (static_cast<u32>(col) < static_cast<u32>(board->m_width) && static_cast<u32>(row) < static_cast<u32>(board->m_height)) {
            tile0 = (reinterpret_cast<i32*>(board->m_rows[row]))[col * 7];
        } else {
            tile0 = 1;
        }
        i32 flag = (tile0 >> 2) & 1;
        if (unit->CoordCount() != 0) {
            GruntCoord* c = (unit->CoordTail())->m_coord;
            i32 cx = c->m_x;
            i32 cy = c->m_y;
            i32 tile1;
            if (static_cast<u32>(cx) < static_cast<u32>(board->m_width) && static_cast<u32>(cy) < static_cast<u32>(board->m_height)) {
                tile1 = (reinterpret_cast<i32*>(board->m_rows[cy]))[cx * 7];
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
                (reinterpret_cast<u8*>(board->m_rows[r]))[colOff + 2] &= 0xfd;
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
// 0xe anim modes, and re-path the unit toward it (Method_0300c0, flags 0x1000d8f).
// On a route, debounce the m_390 latch (a g_frameTime window against m_scratch78..m_084,
// firing the scene hit when the unit's level coord is on-screen), re-clamp the
// board dirty-rect, and return 1. No candidate latches m_390 and returns 0.
// ===========================================================================
// @early-stop
// box-stack-slot + EH/regalloc plateau (same family as winapi_02a570/02dfa0): the
// 4-corner box build, the band scan with the five inline-strcmp C/R/J/G/L rejects
// (setne bool form) + PtInRect + dist^2 min-keep, the box clamp with the dead
// maybe-null branch retail emits, the 0x12/0x16/0xe FindPath-flag build, the
// Method_0300c0 re-path, the m_390 64-bit-timer debounce + scene-hit, and both
// dirty-rect re-clamps are reconstructed in shape + order. Residual is the
// compiler's stack colouring of the 6 transient Coord/box slots (the >>5 corners
// alias the later dist temporaries) + the /GX cond-temp EH state; foreign
// unit/board/g_gameReg chains modeled by raw offset. Not source-steerable.
RVA(0x0002e3a0, 0x7e1)
i32 CBattlezMapConfig::winapi_02e3a0_PtInRect(i32 unitArg) {
    CGrunt* unit = reinterpret_cast<CGrunt*>(unitArg);
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
            ne = strcmp((*g_typeColl.GetNameRecord(static_cast<void*>((u->m_objAux->m_1c)))), "C") != 0;
            if (!ne) {
                continue;
            }
            ne = strcmp((*g_typeColl.GetNameRecord(static_cast<void*>((u->m_objAux->m_1c)))), "R") != 0;
            if (!ne) {
                continue;
            }
            ne = strcmp((*g_typeColl.GetNameRecord(static_cast<void*>((u->m_objAux->m_1c)))), "J") != 0;
            if (!ne) {
                continue;
            }
            ne = strcmp((*g_typeColl.GetNameRecord(static_cast<void*>((u->m_objAux->m_1c)))), "G") != 0;
            if (!ne) {
                continue;
            }
            ne = strcmp((*g_typeColl.GetNameRecord(static_cast<void*>((u->m_objAux->m_1c)))), "L") != 0;
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
            Coord a1;
            (static_cast<CUserLogic*>(unit))->GetScreenPos((&a1));
            Coord b1;
            (static_cast<CUserLogic*>(u))->GetScreenPos((&b1));
            i32 dx = abs((a1.m_x >> 5) - (b1.m_x >> 5));
            Coord a2;
            (static_cast<CUserLogic*>(unit))->GetScreenPos((&a2));
            Coord b2;
            (static_cast<CUserLogic*>(u))->GetScreenPos((&b2));
            i32 dy = abs((a2.m_y >> 5) - (b2.m_y >> 5));
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
    CBrickzGrid* board = m_board;
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
        RECT* p0 = static_cast<RECT*>(new (&r0) CRect(0, 0, board->m_width, board->m_height));
        rc.left = p0->left;
        rc.top = p0->top;
        rc.right = p0->right;
        rc.bottom = p0->bottom;
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
        t = unit->m_19c;
    }
    if (t == 0x12) {
        flags = 0x100;
    }
    t = prim;
    if (prim > 0x16) {
        t = unit->m_19c;
    }
    if (t == 0x16) {
        flags = 0x942;
    }
    if (prim > 0x16) {
        prim = unit->m_19c;
    }
    if (prim == 0xe) {
        flags = 0x1000;
    }
    Coord bc;
    (static_cast<CUserLogic*>(best))->GetScreenPos((&bc));
    if (Method_0300c0(reinterpret_cast<i32>(unit), bc.m_x >> 5, bc.m_y >> 5, 0x1000d8f, flags, 1) == 0) {
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
        __int64 elapsed = static_cast<__int64>(static_cast<u32>(g_frameTime)) - *reinterpret_cast<__int64*>(&m_scratch78);
        if (elapsed >= *reinterpret_cast<__int64*>(&m_scratch80)) {
            unit->m_390 = 0;
            CGameObject* lvl = unit->m_object;
            // On-screen test against the main plane's tile origin/extent quad
            // (+0x40..+0x4c), overlaid as a RECT (the sanctioned int-quad read).
            RECT* hit = reinterpret_cast<RECT*>(&g_gameReg->m_world->m_level->m_mainPlane->m_originX);
            if (lvl->m_screenX < hit->right && lvl->m_screenX >= hit->left
                && lvl->m_screenY < hit->bottom && lvl->m_screenY >= hit->top) {
                (static_cast<CGruntSpawnConfig*>(static_cast<void*>(g_gameReg->m_cueSink)))
                    ->SpawnVoiceDriver(reinterpret_cast<i32>(unit), 0x366, -1, 0, -1, -1);
            }
            *reinterpret_cast<__int64*>(&m_scratch78) = 0;
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
// CBattlezMapConfig::Method_02edb0  @0x02edb0  (/GX EH frame)
// Reroute a unit toward a target cell. The target is (arg2,arg3) when `useArg` is
// set, else the first of the unit's occupied coords that lands on a blocked (bit
// 0x4) tile. If the unit already collides there (Method_0305b0) recycle its path +
// clear state; if its path is blocked (Method_030530) honour the reserved-tile
// bit. Otherwise scan the current cell-row for the nearest eligible unit (passing
// the cached-cell + clear-flag guards and NOT an I/G/L/P/J/C/R type code) within
// distance 0x190, build the FindPath flags from its 0x16/0x12 anim modes, ask
// CBrickzGrid::FindPath for a route, and swap that unit's path onto this one (recycle old
// coords onto g_coordPool, AddTail the new, set state 5). Returns 1 on a reroute.
// ===========================================================================
// @early-stop
// resolver + EH + regalloc plateau: the coord-scan head, the Method_0305b0 collision
// + Method_030530 block checks, the seven-way I/G/L/P/J/C/R GetRecord setcc dispatch
// (docs/patterns/strcmp-eq-bool-local-setcc.md), the distance<=0x190 best scan, the
// 0x16/0x12 flag build, CPtrList(10)/GetCoord/FindPath, and the g_coordPool/g_coordPool.m_freeHead
// path-swap are reconstructed in shape + order. Residual is the 15-slot scan regalloc
// (retail pins the slot index in [esp+0x4c] and the candidate in ebp) plus the /GX
// cleanup epilogue funnel; foreign chains modeled by raw offset. Final sweep.
// Method_02ed90 (0x2ed90) - always returns 0.
RVA(0x0002ed90, 0x5)
i32 CBattlezMapConfig::Method_02ed90(i32) {
    return 0;
}

RVA(0x0002edb0, 0x6b4)
i32 CBattlezMapConfig::Method_02edb0(i32 unitArg, i32 useArg, i32 ax, i32 ay) {
    CGrunt* unit = reinterpret_cast<CGrunt*>(unitArg);
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
        GruntCoordNode* n = unit->CoordHead();
        while (n != 0) {
            GruntCoordNode* cur = n;
            n = n->m_next;
            GruntCoord* c = cur->m_coord;
            if (c != 0) {
                BrickzCell* row = static_cast<BrickzCell*>((m_board)->m_rows[c->m_y]);
                if ((reinterpret_cast<i32*>(&row[c->m_x]))[0] & 4) {
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
    if (Method_0305b0(unitArg, tx, ty) != 0) {
        // Already colliding there: recycle the unit's path + reset state.
        if (unit->CoordCount() != 0) {
            GruntCoordNode* n = unit->CoordHead();
            while (n != 0) {
                GruntCoordNode* cur = n;
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
    if (Method_030530(unitArg) != 0) {
        // Path is blocked: a reserved-tile bit on the first path coord aborts.
        if (unit->CoordCount() != 0) {
            GruntCoordNode* p = unit->CoordHead();
            GruntCoord* c = p->m_coord;
            i32 word;
            CBrickzGrid* b = m_board;
            if (static_cast<u32>(c->m_x) < static_cast<u32>(b->m_width) && static_cast<u32>(c->m_y) < static_cast<u32>(b->m_height)) {
                word = (reinterpret_cast<i32*>(&(static_cast<BrickzCell*>(b->m_rows[c->m_y]))[c->m_x]))[0];
            } else {
                word = 1;
            }
            if (word & 0x20000000) {
                return 0;
            }
            return 1;
        }
    }
    if (Method_0305b0(unitArg, tx, ty) != 0) {
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
                eq =
                    (strcmp(
                         (*g_typeColl.GetNameRecord(reinterpret_cast<void*>((*reinterpret_cast<i32*>((reinterpret_cast<char*>(cand->m_objAux)
                                                                    + 0x1c)))))),
                         "I"
                     )
                     == 0);
                if (!eq) {
                    eq =
                        (strcmp(
                             (*g_typeColl.GetNameRecord(reinterpret_cast<void*>((*reinterpret_cast<i32*>((reinterpret_cast<char*>(cand->m_objAux)
                                                                        + 0x1c)))))),
                             "G"
                         )
                         == 0);
                }
                if (!eq) {
                    eq =
                        (strcmp(
                             (*g_typeColl.GetNameRecord(reinterpret_cast<void*>((*reinterpret_cast<i32*>((reinterpret_cast<char*>(cand->m_objAux)
                                                                        + 0x1c)))))),
                             "L"
                         )
                         == 0);
                }
                if (!eq) {
                    eq =
                        (strcmp(
                             (*g_typeColl.GetNameRecord(reinterpret_cast<void*>((*reinterpret_cast<i32*>((reinterpret_cast<char*>(cand->m_objAux)
                                                                        + 0x1c)))))),
                             "P"
                         )
                         == 0);
                }
                if (!eq) {
                    eq =
                        (strcmp(
                             (*g_typeColl.GetNameRecord(reinterpret_cast<void*>((*reinterpret_cast<i32*>((reinterpret_cast<char*>(cand->m_objAux)
                                                                        + 0x1c)))))),
                             "J"
                         )
                         == 0);
                }
                if (!eq) {
                    eq =
                        (strcmp(
                             (*g_typeColl.GetNameRecord(reinterpret_cast<void*>((*reinterpret_cast<i32*>((reinterpret_cast<char*>(cand->m_objAux)
                                                                        + 0x1c)))))),
                             "C"
                         )
                         == 0);
                }
                if (!eq) {
                    eq =
                        (strcmp(
                             (*g_typeColl.GetNameRecord(reinterpret_cast<void*>((*reinterpret_cast<i32*>((reinterpret_cast<char*>(cand->m_objAux)
                                                                        + 0x1c)))))),
                             "R"
                         )
                         == 0);
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
                            sec = unit->m_19c;
                        }
                        if (sec == 0x16) {
                            flags = 0x4962;
                        }
                        i32 prim = unit->m_entranceReason;
                        if (prim > 0x16) {
                            prim = unit->m_19c;
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
                                    GruntCoordNode* nn = unit->CoordHead();
                                    while (nn != 0) {
                                        GruntCoordNode* cur = nn;
                                        nn = nn->m_next;
                                        if (cur->m_coord != 0) {
                                            CoordPoolNode* fn = g_coordPool.NodeOf(cur->m_coord);
                                            fn->m_next = g_coordPool.m_freeHead;
                                            g_coordPool.m_freeHead = fn;
                                        }
                                    }
                                    unit->m_31c.RemoveAll();
                                }
                                GruntCoordNode* p = reinterpret_cast<GruntCoordNode*>(list.GetHeadPosition());
                                while (p != 0) {
                                    GruntCoordNode* cur = p;
                                    p = p->m_next;
                                    unit->m_31c.AddTail(cur->m_coord);
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
// CBattlezMapConfig::Method_02f620  @0x02f620
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
// large-state-machine plateau (~49%): the four guards, the seven-way I/G/L/P/J/C/R
// anim-name dispatch (the inline-strcmp setcc form via `bool eq`, see
// docs/patterns/strcmp-eq-bool-local-setcc.md), the three banded threshold-table
// cascades, all three SetState arms, the mode-3 row reseed loop, and the 0x12/0x16
// g_coordPool.m_freeHead recycle are reconstructed in shape + order, and the prologue/setcc
// strcmp byte stream now matches retail. Two coupled residuals: (1) the scratch
// CString teardown loop - retail copies the count, decrements, tests the original,
// and recovers the trip via `lea edi,[eax+1]` where MSVC5 here just `mov edi,eax`s
// the count (a loop-strength-reduction idiom no source spelling reproduces, shared
// with Method_034460); (2) the threshold-cascade regalloc (retail pins the rolled
// value in edx, the band divisors in esi). Deferred to the final sweep.
RVA(0x0002f620, 0x871)
i32 CBattlezMapConfig::Method_02f620(i32 unitArg) {
    CGrunt* unit = reinterpret_cast<CGrunt*>(unitArg);
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
    eq = (strcmp((*g_typeColl.GetNameRecord(static_cast<void*>((unit->m_objAux->m_1c)))), "I") == 0);
    if (eq) {
        return 0;
    }
    // G / L / P / J / C / R (each via GetRecords, with the scratch CString teardown).
    CAnimNameRecord* recs;
    CString* slot;
    i32 cnt;

    recs = g_typeColl.GetNameRecords(static_cast<void*>((unit->m_objAux->m_1c)));
    slot = reinterpret_cast<CString*>(g_typeColl.m_alloc);
    cnt = g_typeColl.m_grown;
    while (cnt != 0) {
        if (slot != 0) {
            slot->~CString();
        }
        slot++;
        cnt--;
    }
    eq = (strcmp(recs->m_name, "G") == 0);
    if (eq) {
        return 0;
    }

    recs = g_typeColl.GetNameRecords(static_cast<void*>((unit->m_objAux->m_1c)));
    slot = reinterpret_cast<CString*>(g_typeColl.m_alloc);
    cnt = g_typeColl.m_grown;
    while (cnt != 0) {
        if (slot != 0) {
            slot->~CString();
        }
        slot++;
        cnt--;
    }
    eq = (strcmp(recs->m_name, "L") == 0);
    if (eq) {
        return 0;
    }

    recs = g_typeColl.GetNameRecords(static_cast<void*>((unit->m_objAux->m_1c)));
    slot = reinterpret_cast<CString*>(g_typeColl.m_alloc);
    cnt = g_typeColl.m_grown;
    while (cnt != 0) {
        if (slot != 0) {
            slot->~CString();
        }
        slot++;
        cnt--;
    }
    eq = (strcmp(recs->m_name, "P") == 0);
    if (eq) {
        return 0;
    }

    recs = g_typeColl.GetNameRecords(static_cast<void*>((unit->m_objAux->m_1c)));
    slot = reinterpret_cast<CString*>(g_typeColl.m_alloc);
    cnt = g_typeColl.m_grown;
    while (cnt != 0) {
        if (slot != 0) {
            slot->~CString();
        }
        slot++;
        cnt--;
    }
    eq = (strcmp(recs->m_name, "J") == 0);
    if (eq) {
        return 0;
    }

    recs = g_typeColl.GetNameRecords(static_cast<void*>((unit->m_objAux->m_1c)));
    slot = reinterpret_cast<CString*>(g_typeColl.m_alloc);
    cnt = g_typeColl.m_grown;
    while (cnt != 0) {
        if (slot != 0) {
            slot->~CString();
        }
        slot++;
        cnt--;
    }
    eq = (strcmp(recs->m_name, "C") == 0);
    if (eq) {
        return 0;
    }

    recs = g_typeColl.GetNameRecords(static_cast<void*>((unit->m_objAux->m_1c)));
    slot = reinterpret_cast<CString*>(g_typeColl.m_alloc);
    cnt = g_typeColl.m_grown;
    while (cnt != 0) {
        if (slot != 0) {
            slot->~CString();
        }
        slot++;
        cnt--;
    }
    eq = (strcmp(recs->m_name, "R") == 0);
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
        // Band A: the unit must currently be idle (m_entranceReason/m_19c clear).
        i32 cur = unit->m_entranceReason;
        if (cur > 0x16) {
            cur = unit->m_19c;
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
                    GruntCoordNode* n = u->CoordHead();
                    while (n != 0) {
                        GruntCoordNode* curn = n;
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
            cur2 = unit->m_19c;
        }
        if (cur2 == 0) {
            (static_cast<CGrunt*>(unit))->LoadPickupSprites(mode, 1, 0, 0, 1);
            return 1;
        }
        if (mode == 0x12) {
            if (unit->CoordCount() != 0) {
                GruntCoordNode* n = unit->CoordHead();
                while (n != 0) {
                    GruntCoordNode* curn = n;
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
                GruntCoordNode* n = unit->CoordHead();
                while (n != 0) {
                    GruntCoordNode* curn = n;
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
// CBattlezMapConfig::Method_0300c0  @0x0300c0  (/GX EH frame)
// Re-path `unit` to (gx,gy): if it is already there (its level geometry's
// (>>5) coord equals the goal) succeed trivially; otherwise ask the board's
// A* (FindPath) for a route into a local CPtrList, then swap the unit's path:
// recycle each old coord node onto the coord pool, empty the unit's path list,
// AddTail every new path node onto it, set the unit's packed coord from the
// new tail, and destruct the local list. Returns 1 on a route, 0 otherwise.
// ===========================================================================
// @early-stop
// EH-frame + regalloc plateau (~63%): logic + every call (FindPath, RemoveHead,
// the two CPtrList walks, the g_coordPool/g_coordPool.m_freeHead recycles) is byte-exact and
// in the right order. Two coupled walls: (1) retail pins `unit` in ebp and arg2
// in edi, loading arg3 lazily between the two head compares, where MSVC5 here
// pins `unit` in ebx and reads arg3 early; (2) retail funnels all `return 0`
// paths into ONE shared /GX cleanup epilogue (je <shared>) where MSVC5 duplicates
// the ~CPtrList/xor/jmp at each early return. No steerable source spelling closes
// either. Deferred to the final sweep.
RVA(0x000300c0, 0x190)
i32 CBattlezMapConfig::Method_0300c0(i32 unitArg, i32 gx, i32 gy, i32 a4, i32 a5, i32 a6) {
    CPtrList list(10);
    CGrunt* unit = reinterpret_cast<CGrunt*>(unitArg);
    CGameObject* lvl = unit->m_object;
    if ((lvl->m_screenX >> 5) == gx && (lvl->m_screenY >> 5) == gy) {
        return 0;
    }
    if ((m_board)->SearchEdge(lvl->m_screenX >> 5, lvl->m_screenY >> 5, gx, gy, &list, a6, a4, a5)
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
        GruntCoordNode* n = unit->CoordHead();
        while (n != 0) {
            GruntCoordNode* cur = n;
            n = n->m_next;
            if (cur->m_coord != 0) {
                g_coordPool.Push(cur->m_coord);
            }
        }
        unit->m_31c.RemoveAll();
    }
    // AddTail every new path node's coord onto the unit's path list.
    GruntCoordNode* p = reinterpret_cast<GruntCoordNode*>(list.GetHeadPosition());
    while (p != 0) {
        GruntCoordNode* cur = p;
        p = p->m_next;
        if (cur->m_coord != 0) {
            unit->m_31c.AddTail(cur->m_coord);
        }
    }
    list.RemoveAll();
    GruntCoord* tail = (unit->CoordTail())->m_coord;
    unit->m_entrancePxX = (tail->m_x << 5) + 0x10;
    unit->m_entrancePxY = (tail->m_y << 5) + 0x10;
    return 1;
}

// ===========================================================================
// CBattlezMapConfig::Method_0302c0  @0x0302c0  (/GX EH frame)
// Re-path `unit` to (gx, gy) - the GetCoord-fronted twin of Method_0300c0. If the
// unit is already at the goal (its GetCoord (>>5) == (gx, gy)) bail; scan its path
// for a node already on the goal; ask the board's A* (FindPath) for a route into a
// local CPtrList; recycle the route's head + (when the goal was already queued) the
// path-list base + the unit's existing coord nodes onto g_coordPool.m_freeHead; then AddTail
// every new route node onto the unit's path list. Returns 1 on a route, 0 otherwise.
// ===========================================================================
// @early-stop
// EH-frame + regalloc plateau: logic + every call (the two GetCoords, FindPath,
// RemoveHead, the g_coordPool.m_freeHead recycles, AddTail, the ~CPtrList unwind) is reconstructed
// in shape + order. Two walls: (1) the /GX cond-temp EH state machine (shared
// `je <unwind>` cleanup vs cl's per-return duplication, same as Method_0300c0); (2)
// the matched-node g_coordPool.m_freeHead recycle in the middle compiles to a degenerate
// loop-invariant `do/while` in retail (the path-segment recycle) that no source
// spelling reproduces. Foreign unit chains modeled by raw offset. Final sweep.
RVA(0x000302c0, 0x1ec)
i32 CBattlezMapConfig::Method_0302c0(i32 unitArg, i32 gx, i32 gy, i32 a4, i32 a5) {
    CPtrList list(10);
    CGrunt* unit = reinterpret_cast<CGrunt*>(unitArg);
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
    GruntCoordNode* match = 0;
    GruntCoordNode* n = unit->CoordHead();
    while (n != 0) {
        GruntCoordNode* cur3 = n;
        n = n->m_next;
        GruntCoord* coord = cur3->m_coord;
        if (coord != 0 && coord->m_x == gx && coord->m_y == gy) {
            match = n;
            break;
        }
    }
    CGameObject* lvl = unit->m_object;
    if ((m_board)->SearchEdge(lvl->m_screenX >> 5, lvl->m_screenY >> 5, gx, gy, &list, 0, a5, a5)
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
        GruntCoordNode* p = unit->CoordHead();
        while (p != 0) {
            GruntCoordNode* cur4 = p;
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
    GruntCoordNode* q = reinterpret_cast<GruntCoordNode*>(list.GetHeadPosition());
    while (q != 0) {
        GruntCoordNode* cur5 = q;
        q = q->m_next;
        if (cur5->m_coord != 0) {
            unit->m_31c.AddTail(cur5->m_coord);
        }
    }
    list.RemoveAll();
    return 1;
}

RVA(0x00030530, 0x56)
i32 CBattlezMapConfig::Method_030530(i32 unitArg) {
    CGrunt* unit = reinterpret_cast<CGrunt*>(unitArg);
    if (unit->CoordCount() == 0) {
        return 0;
    }
    GruntCoordNode* node = unit->CoordHead();
    if (node == 0) {
        return 0;
    }
    BrickzCell** rows = ((m_board)->m_rows);
    while (node != 0) {
        GruntCoordNode* cur = node;
        node = node->m_next;
        GruntCoord* c = cur->m_coord;
        i32 y = c->m_y;
        i32 x = c->m_x;
        if (rows[y][x].m_0 & 4) {
            return 1;
        }
    }
    return 0;
}

// ===========================================================================
// CBattlezMapConfig::Method_0305b0  @0x0305b0
// Scan the current cell-row for any OTHER unit that occupies coordinate
// (arg1, arg2): either via a "blocked tile" hit on the unit's occupied-coord
// list, via the unit's own packed coord (m_entrancePxX/m_entrancePxY >> 5), or via its level
// geometry (m_object->m_5c/m_60 >> 5). Returns 1 on the first hit, else 0.
// ===========================================================================
// @early-stop
// regalloc wall (~46%): logic byte-exact at the head (prologue + the three
// early-out compares match). Retail spills `this` to a stack slot and keeps it
// in esi (reloading inside the inner loop), and orders the two stack locals
// (counter / cell-ptr) opposite to MSVC5's choice here; we keep `this` live in
// ebx. The divergence cascades through every register operand. No steerable
// spelling found. Deferred to the final sweep.
RVA(0x000305b0, 0x121)
i32 CBattlezMapConfig::Method_0305b0(i32 selfUnit, i32 qx, i32 qy) {
    CGrunt** units = m_triggerMgr->m_grid + m_curCell * 15;
    for (i32 i = 0; i < 15; i++) {
        CGrunt* unit = units[i];
        if (unit == 0) {
            continue;
        }
        if (unit == reinterpret_cast<CGrunt*>(selfUnit)) {
            continue;
        }
        if (unit->m_2d8 == 0xb) {
            continue;
        }
        if (unit->CoordCount() != 0 && unit->CoordHead() != 0) {
            CBrickzGrid* board = m_board;
            GruntCoordNode* node = unit->CoordHead();
            while (node != 0) {
                GruntCoordNode* cur = node;
                node = node->m_next;
                GruntCoord* c = cur->m_coord;
                i32 x = c->m_x;
                i32 y = c->m_y;
                i32 tile;
                if (static_cast<u32>(x) < static_cast<u32>(board->m_width) && static_cast<u32>(y) < static_cast<u32>(board->m_height)) {
                    tile = (reinterpret_cast<i32*>(board->m_rows[y]))[x * 7];
                } else {
                    tile = 1;
                }
                if ((tile & 4) && x == qx && y == qy) {
                    return 1;
                }
            }
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
// CBattlezMapConfig::Method_030730  @0x030730
// Cell-claim scan: for the (cellX,cellY) source unit, walk the 15 unit slots of
// the CURRENT cell-row (m_curCell) and, for each candidate whose mode is 3 (or a
// 2/3-of-the-time random pick) and whose per-level record lands within distance
// 0x19 of the candidate's geometry, claim it - mark mode 3 / state 2, stamp the
// target coord (cellX,cellY) and seed m_250 = 0xd87.
// ===========================================================================
// @early-stop
// regalloc wall (~88%): logic byte-exact. Retail pins `this` in edi and SPILLS a
// copy to [esp+0x10] so it can reuse edi as scratch for this->m_ctx (mov edi,
// [edi+0x4]) in the distance block, reloading it after; MSVC5 here keeps `this`
// live in one callee-saved reg + loads m_ctx into a fresh one, reserving 0x8 of
// locals (no spill slot) vs retail's 0xc. Cascades through the cellX/cellY
// reg-vs-memory operand choice. No steerable spelling found; final sweep.
RVA(0x00030730, 0x1da)
i32 CBattlezMapConfig::Method_030730(i32 cellX, i32 cellY, i32, i32) {
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
        if (sx == m_curCell) {
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
            if (dx * dx + dy * dy > 0x19) {
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
// CBattlezMapConfig::Method_030990  @0x030990
// Try to seed a fresh spawn unit at a screen cell. Count the occupied units in the
// current cell-row; if that count is at/over the per-level record's budget
// (rec->m_378) bail. Otherwise probe the screen cell mapped from (arg1,arg2) via the
// grid's SpawnProbe (using rec->m_158 as the kind tag); if it resolves to a unit
// slot, seed it as a fresh mode-4 spawn (state 0x11, -1 coord block). Returns 1 on a
// seeded spawn, 0 otherwise.
// ===========================================================================
// @early-stop
// zero-register-pinning wall (~94.6%): structure byte-exact - the occupied-count
// loop, the rec->m_378 budget gate, the 13-arg SpawnProbe call (rec->m_158 tag +
// the two shifted coords), the cell->unit index, and the full mode-4 spawn seed are
// all reproduced in shape + order. Retail pins the occupied counter in ebp and the
// zero/null constant in ebx; MSVC5 here swaps the two (counter in ebx, zero in ebp),
// which cascades through every push-0, the budget cmp, and the seed's `=0` stores +
// reschedules the -1 block. No source lever forces the pinning under /O2 (see
// docs/patterns/zero-register-pinning.md). Deferred to the final sweep.
RVA(0x00030990, 0x11b)
i32 CBattlezMapConfig::Method_030990(i32 ax, i32 ay) {
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
    i32 cell = m_triggerMgr->Probe(
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
    CGrunt* unit = m_ctx->m_cmdGrid->m_grid[cell + m_curCell * 15];
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
// CBattlezMapConfig::Method_030b20  @0x030b20  (/GX EH frame)
// Best-fit reroute: locate the cell record for (col,row) - directly when its tile
// dword[4] == 0x67, else via m_ctx->QueryA - then scan its 24-entry sub-cell
// pointer block for the candidate, not colliding with `unit` (Method_0305b0),
// nearest (min squared-distance) to the unit's level coord. If one is found and is
// reachable, build the FindPath flag word from the unit's 0x16/0x12 anim modes,
// ask CBrickzGrid::FindPath for a route into a local CPtrList, then swap the unit's path
// (recycle old coord nodes onto g_coordPool.m_freeHead, AddTail the new ones, stamp the packed
// target coord + state 5). Returns 1 on a reroute, 0 otherwise.
// ===========================================================================
// @early-stop
// EH-frame + regalloc plateau (~69%): logic + every call (QueryA/QueryB,
// Method_0305b0, the 0x16/0x12 flag build, CPtrList(10)/FindPath, the g_coordPool.m_freeHead
// recycle + AddTail path-swap, ~CPtrList) is reconstructed in shape + order. Residual
// is the head's instruction scheduling (retail interleaves the goalX/goalY >>5 with
// the tile lookup and pins the cell base in edi where MSVC5 here computes the goal
// upfront and spills) plus the /GX cleanup epilogue funnel; the foreign cell/level
// chains are modeled by raw offset. Deferred to the final sweep.
RVA(0x00030b20, 0x328)
i32 CBattlezMapConfig::Method_030b20(i32 unitArg, i32 col, i32 row) {
    CGrunt* unit = reinterpret_cast<CGrunt*>(unitArg);
    CGameObject* lvl = unit->m_object;
    i32 goalX = lvl->m_screenX >> 5;
    i32 goalY = lvl->m_screenY >> 5;
    // The cell record for (col,row): a direct table slot when its tile marker is
    // 0x67, else resolved through QueryA on the packed coordinate.
    BrickzCell* tile = &(static_cast<BrickzCell*>((m_board)->m_rows[row]))[col];
    char* cell;
    if (*reinterpret_cast<i32*>((reinterpret_cast<char*>(tile) + 0x10)) == 0x67) {
        cell = reinterpret_cast<char*>(m_ctx->m_tileGrid); // ctx+0x70 IS the board (== this->m_board)
    } else {
        cell = reinterpret_cast<char*>((reinterpret_cast<CTileTriggerContainer*>(m_ctx))->FindInLists12((col << 8) + row, 0));
    }
    i32 bestX = col;
    i32 bestY = col;
    i32 bestDist = 0x7fffffff;
    if (cell != 0) {
        // First pass: any sub-cell that already collides with `unit` aborts.
        char** scan = reinterpret_cast<char**>((cell + 0x3c));
        while (static_cast<i32>(((reinterpret_cast<char*>(scan) - cell - 0x3c) & ~3)) < 0x60) {
            void* node = *scan;
            if (node != 0) {
                void* rec = m_cellQuery->FindChild(reinterpret_cast<i32>(node), 0);
                if (rec != 0) {
                    i32 cx = *reinterpret_cast<i32*>((reinterpret_cast<char*>(rec) + 0x8));
                    i32 cy = *reinterpret_cast<i32*>((reinterpret_cast<char*>(rec) + 0xc));
                    if (Method_0305b0(unitArg, cx, cy) != 0) {
                        return 1;
                    }
                }
            }
            scan++;
        }
        // Second pass: keep the nearest non-colliding sub-cell.
        char** scan2 = reinterpret_cast<char**>((cell + 0x3c));
        while (static_cast<i32>(((reinterpret_cast<char*>(scan2) - cell - 0x3c) & ~3)) < 0x60) {
            void* node = *scan2;
            if (node != 0) {
                void* rec = m_cellQuery->FindChild(reinterpret_cast<i32>(node), 0);
                if (rec != 0) {
                    i32 cx = *reinterpret_cast<i32*>((reinterpret_cast<char*>(rec) + 0x8));
                    i32 cy = *reinterpret_cast<i32*>((reinterpret_cast<char*>(rec) + 0xc));
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
            scan2++;
        }
    }
    if (bestDist == 0x7fffffff) {
        return 0;
    }
    if (Method_0305b0(unitArg, bestX, bestY) != 0) {
        return 0;
    }
    CPtrList list(10);
    // The FindPath flag word: 0x60 base, + 0x900/0x100 bits from the unit's
    // 0x16 / 0x12 anim modes (primary m_entranceReason, or secondary m_19c when m_entranceReason > 0x16).
    i32 flags = 0x60;
    i32 sec = unit->m_entranceReason;
    if (sec > 0x16) {
        sec = unit->m_19c;
    }
    if (sec == 0x16) {
        flags = 0x962;
    }
    i32 prim = unit->m_entranceReason;
    if (prim > 0x16) {
        prim = unit->m_19c;
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
        Method_02edb0(unitArg, 1, bestX, bestY);
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
        GruntCoordNode* n = unit->CoordHead();
        while (n != 0) {
            GruntCoordNode* cur = n;
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
    GruntCoordNode* p = reinterpret_cast<GruntCoordNode*>(list.GetHeadPosition());
    while (p != 0) {
        GruntCoordNode* cur = p;
        p = p->m_next;
        unit->m_31c.AddTail(cur->m_coord);
    }
    GruntCoord* tail = (unit->CoordTail())->m_coord;
    unit->m_entrancePxX = (tail->m_x << 5) + 0x10;
    unit->m_entrancePxY = (tail->m_y << 5) + 0x10;
    unit->m_defenderState = 5;
    return 1;
}

// ===========================================================================
// CBattlezMapConfig::Method_030f20  @0x030f20
// Pick a spawn coordinate for `unit` from the per-level record's candidate list
// (index `kind`, 0..3): start at a random candidate and walk forward (mod count)
// looking for one not already occupied by any unit in the current cell-row; on
// success write it to `out`. Out-of-range `kind` or empty list falls back to the
// unit's own (>>5) geometry. Returns `out`.
// ===========================================================================
// @early-stop
// regalloc wall (~58%): logic byte-exact (count==0 / final-rand tail-merge, the
// found-in-loop early return, the 15-slot collision scan all reproduced). Retail
// re-reads `unit` from the stack arg and spills the candidate count to a stack
// slot; MSVC5 here caches `unit` and pins count in edi, which cascades the inner
// collision loop's register operands (load-then-test vs memory-compare on
// u->CoordCount(), cand coord regs). No steerable spelling found; final sweep.
RVA(0x00030f20, 0x16d)
void* CBattlezMapConfig::Method_030f20(void* out, i32 unitArg, i32 kind) {
    Coord* o = static_cast<Coord*>(out);
    CGrunt* unit = reinterpret_cast<CGrunt*>(unitArg);
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
            Coord** arr = reinterpret_cast<Coord**>(coords->GetData());
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
                        i32* node = reinterpret_cast<i32*>(u->CoordTail()->m_coord);
                        if (node[0] == cx && node[1] == cy) {
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
        Coord* cand = reinterpret_cast<Coord**>(coords->GetData())[r];
        rx = cand->m_x;
        ry = cand->m_y;
    }
    o->m_x = rx;
    o->m_y = ry;
    return o;
}

#define MOVE_RECYCLE(g)                                                                            \
    {                                                                                              \
        GruntCoordNode* nd = (g)->CoordHead();                                                     \
        while (nd != 0) {                                                                          \
            GruntCoordNode* cur = nd;                                                              \
            nd = nd->m_next;                                                                       \
            if (cur->m_coord != 0) {                                                               \
                g_coordPool.Push(static_cast<void*>(cur->m_coord));                                           \
            }                                                                                      \
        }                                                                                          \
        (g)->m_31c.RemoveAll();                                                                    \
    }

// @early-stop
// The block order matters: my source laid the in-flight path first, but retail lays the
// FRESH path as the fall-through.
// Fixes applied, each verified against llvm-objdump -dr:
//   * block order: wrap fresh in `if(m_328==0){...}` so cl emits `jne handle328` and
//     falls into fresh (was `if(m_328!=0)goto` which cl inverted to fall into the short
//     handle328) - the single change that moved 18->69;
//   * board distance is real `(int)sqrt((double)(adx*adx+ady*ady))` inlined -> retail's
//     `fild [sum]; fsqrt; call __ftol` (the fake identity isqrt both mis-computed AND
//     dropped the [esp] spill that sizes the frame to 0x20);
//   * W/H read raw before GetTilePos, /3 divided after (deferred-division);
//   * `c0.m_x/c0.m_y/c1.m_x/c1.m_y >>= 5` in place (retail stores the shifted coords back);
//   * m_2ec vs m_0b8/m_reserveBudget are UNSIGNED compares (jbe, not jle).
// Residual ~28% is a genuine register-COLORING cascade: retail colors `this`(mover)->edi
// and `g`(arg)->esi; this cl colors `this`->ebx, freeing one reg so it spills/reloads
// fewer temps (base 352 insns vs retail 395 - retail re-materializes push-0/or-1 and
// reloads spills that this cl keeps in the extra reg). No source spelling reassigns the
// callee-saved `this` register. Final-sweep candidate.
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
        CGrunt* nb = QueryTile4098(c0.m_x, c0.m_y, static_cast<i32>((static_cast<u32>(W) / 3)), static_cast<i32>((static_cast<u32>(H) / 3)));
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
            Commit42e1(g);
            return 1;
        }
        // nb == 0: replan / drain
        if (static_cast<u32>(g->m_dwell) > static_cast<u32>(m_0b8)) {
            Coord here;
            g->GetScreenPos((&here));
            Plan293c(g, here.m_x >> 5, here.m_y >> 5, m_ac, m_b0, -1);
            if (g->CoordCount() > m_98 + m_94 && g->CoordCount() != 0) {
                GruntCoordNode* nd = g->CoordHead();
                if (nd != 0) {
                    do {
                        void* r = ListNodeAdvance(reinterpret_cast<void**>(&nd));
                        if (*static_cast<i32*>(r) != 0) {
                            g_coordPool.Push(reinterpret_cast<void*>((*static_cast<i32*>(r))));
                        }
                    } while (nd != 0);
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
    CGrunt* nb = QueryTile4098(c0.m_x, c0.m_y, static_cast<i32>((static_cast<u32>(W) / 3)), static_cast<i32>((static_cast<u32>(H) / 3)));

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
            // arrived on this tile: latch the move
            g->m_arrivalCol = -1;
            g->m_arrivalRow = -1;
            Finish3e4f(g, cur);
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
// return-pointer regalloc wall (~58.9%): logic byte-faithful, but retail keeps `out`
// in edx across the two stores and materializes the return via a trailing `mov eax,edx`,
// where our cl pins `out` in eax and elides that move (cascading the m_5c/m_60 register
// pair). Permuter found no closing spelling (operand-order invariant). Emits at 0x31c70.
RVA(0x00031c70, 0x1d)
GruntTilePos* CGrunt::GetTilePos(GruntTilePos* out) {
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
// 80.6% - head regalloc wall: logic + every call (CGrunt::Occupied, the
// CoordListWalk/g_coordPool + raw-walk/g_coordPool.m_freeHead recycles, IntersectRect, the
// Method_4b320 place, winapi_02ae00) is byte-exact in shape + order (the whole body
// matches). Residual is the m_arrivalCol/m_arrivalRow head: retail keeps the -1 as an immediate
// (cmp eax,0xffffffff) and spills tx/ty to [esp+0x10]/[esp+0x14], where MSVC5 here
// hoists -1 into edi (cmp eax,edi) and keeps tx/ty in registers - the shared-const
// / spill recolor cascades ~0x40 head bytes. Not source-steerable; final sweep.
RVA(0x00031ca0, 0x2f2)
i32 CBattlezMapConfig::winapi_031ca0_IntersectRect(i32 unitArg) {
    CGrunt* unit = reinterpret_cast<CGrunt*>(unitArg);
    i32 tx = unit->m_arrivalCol;
    i32 ty = unit->m_arrivalRow;
    if (tx != -1 && ty != -1) {
        CGrunt* target = m_triggerMgr->m_grid[tx * 15 + ty];
        if (target != 0) {
            CGameObject* lvl = target->m_object;
            if ((static_cast<CGrunt*>(unit))->RectContains(lvl->m_screenX, lvl->m_screenY) != 0) {
                if (unit->CoordCount() != 0) {
                    void* pos = unit->CoordHead();
                    while (pos != 0) {
                        void* coord = *static_cast<void**>(ListNodeAdvance(&pos));
                        if (coord != 0) {
                            g_coordPool.Push(coord);
                        }
                    }
                    unit->m_31c.RemoveAll();
                }
                unit->m_arrivalCol = -1;
                unit->m_arrivalRow = -1;
                winapi_02ae00_IntersectRect(unitArg, reinterpret_cast<i32>(target));
                return 1;
            }
            // Clamp the board dirty-rect to (0,0,w,h): the CRect / IntersectRect
            // copy-back idiom (shared with GruntPathScan's SCAN_BOUNDS).
            CBrickzGrid* board = m_board;
            RECT r1;
            static_cast<RECT*>(new (&r1) CRect(0, 0, board->m_width, board->m_height));
            RECT r2;
            RECT* p2 = static_cast<RECT*>(new (&r2) CRect(0, 0, board->m_width, board->m_height));
            RECT rc;
            rc.left = p2->left;
            rc.top = p2->top;
            rc.right = p2->right;
            rc.bottom = p2->bottom;
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
            GruntCoordNode* n = unit->CoordHead();
            if (n != 0) {
                CoordPoolNode* head = g_coordPool.m_freeHead;
                do {
                    GruntCoordNode* cur = n;
                    n = n->m_next;
                    void* coord = cur->m_coord;
                    if (coord != 0) {
                        CoordPoolNode* slot = g_coordPool.NodeOf(coord);
                        slot->m_next = head;
                        head = slot;
                        g_coordPool.m_freeHead = head;
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
        void* pos = unit->CoordHead();
        while (pos != 0) {
            void* coord = *static_cast<void**>(ListNodeAdvance(&pos));
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
//   0 -> seed the goal (m_defenderX/m_defenderY) from the band record or a Method_030f20 re-route,
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
// large no-EH state-machine plateau (same family as winapi_02e3a0): the m_2e8 band-pick
// (signed rand()%4 with the m_curCell skip), the m_defenderState 0/6/7 dispatch with all three re-place
// arms + the m_254 state-code walk, the box clamp, both FindPath-flag else-if chains, and
// all four coord recyclers (g_coordPool via CoordListWalk::Advance / g_coordPool.m_freeHead inline) are
// reconstructed in shape + order. Residual is the register-relative record-address regalloc
// (cl strength-reduces the band*0x238 lea-chain + folds the +0x170/+0x188/+0x258 sub-offsets
// differently per arm, the documented Method_0358a0 record-address wall) + the box-stack-slot
// schedule; foreign board/record chains modeled by raw offset. Not source-steerable.
RVA(0x00032060, 0x7bd)
i32 CBattlezMapConfig::winapi_032060_IntersectRect(i32 unitArg) {
    CGrunt* unit = reinterpret_cast<CGrunt*>(unitArg);
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
                void* pos = unit->CoordHead();
                if (pos != 0) {
                    do {
                        void* coord = *static_cast<void**>(ListNodeAdvance(&pos));
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
    char* edge = reinterpret_cast<char*>(bundle); // bundle-relative cursor (offset reads below)
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
                GruntCoordNode* n = unit->CoordHead();
                while (n != 0) {
                    GruntCoordNode* cur = n;
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
        GruntCoordNode* n = unit->CoordHead();
        while (n != 0) {
            GruntCoordNode* cur = n;
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
            if (*reinterpret_cast<i32*>((edge + 0xf8)) != 0) {
                Coord out;
                Coord* r = static_cast<Coord*>(Method_030f20(&out, reinterpret_cast<i32>(unit), band));
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
                GruntCoordNode* n = unit->CoordHead();
                while (n != 0) {
                    GruntCoordNode* cur = n;
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
            t = unit->m_19c;
        }
        if (t == 0x12) {
            flags |= 0x100;
        } else {
            t = prim;
            if (prim > 0x16) {
                t = unit->m_19c;
            }
            if (t == 0xe) {
                flags |= 0x1000;
            } else {
                if (prim > 0x16) {
                    prim = unit->m_19c;
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
    CBrickzGrid* board = m_board;
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
        t = unit->m_19c;
    }
    if (t == 0x12) {
        flags |= 0x100;
    } else {
        t = prim;
        if (prim > 0x16) {
            t = unit->m_19c;
        }
        if (t == 0xe) {
            flags |= 0x1000;
        } else {
            if (prim > 0x16) {
                prim = unit->m_19c;
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
// @early-stop
// 99.78% - the SAME freelist-store register-scheduling coin-flip as Deserialize_02b950's
// recycle loops: retail's `g_coordPool.m_freeHead = head` store reads esi (head's callee-saved home,
// which also holds slot after `head=slot`); our cl folds it to `mov g_coordPool.m_freeHead,eax`
// (slot's register). 1-byte residual (89 35 vs a3), pure operand selection - proven a
// coin-flip by CTriggerMgr's twin alloc loops (0x7ad40 direct vs 0x7ad9b copy). All
// logic byte-exact. Deferred to the final sweep.
RVA(0x000343f0, 0x47)
void CGrunt::RecycleCoords() {
    if (CoordCount() == 0) {
        return;
    }
    GruntCoordNode* n = CoordHead();
    if (n != 0) {
        CoordPoolNode* head = g_coordPool.m_freeHead;
        do {
            GruntCoordNode* cur = n;
            n = n->m_next;
            void* coord = cur->m_coord;
            if (coord != 0) {
                CoordPoolNode* slot = g_coordPool.NodeOf(coord);
                slot->m_next = head;
                head = slot;
                g_coordPool.m_freeHead = head;
            }
        } while (n != 0);
    }
    m_31c.RemoveAll();
}

// ===========================================================================
// CBattlezMapConfig::Method_034460  @0x034460
// Anim-name gate: a unit is eligible for a "special" anim only when it sits on
// its cached cell (lvl coord == m_lastTilePxX/m_lastTilePxY) and a block of state flags is clear.
// Then resolve the unit's anim name and reject the simple type codes (I/G/L/J/C)
// outright; for the remaining codes, run the second resolver (which fills the
// g_typeColl.m_alloc CString array, torn down each call) and either map an in-range
// candidate index directly or Probe/Reserve a slot, returning whether the final
// resolved name differs from the "P" code.
// ===========================================================================
// @early-stop
// resolver-cluster plateau: the eligibility guards + the five inline-strcmp type
// rejects (I/G/L/J/C) are byte-exact; the second-resolver tail (GetRecords +
// g_typeColl.m_alloc teardown loop, the candidate-bounds map, Probe/Reserve) is
// reconstructed but its global-scratch regalloc and the imul/bounds arithmetic
// diverge from retail's. Deferred to the final sweep.
RVA(0x00034460, 0x3fc)
i32 CBattlezMapConfig::Method_034460(i32 unitArg) {
    CGrunt* unit = reinterpret_cast<CGrunt*>(unitArg);
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
    eq = (strcmp((*g_typeColl.GetNameRecord(static_cast<void*>((unit->m_objAux->m_1c)))), "I") == 0);
    if (eq) {
        return 0;
    }
    eq = (strcmp((*g_typeColl.GetNameRecord(static_cast<void*>((unit->m_objAux->m_1c)))), "G") == 0);
    if (eq) {
        return 0;
    }
    eq = (strcmp((*g_typeColl.GetNameRecord(static_cast<void*>((unit->m_objAux->m_1c)))), "L") == 0);
    if (eq) {
        return 0;
    }
    // The remaining codes resolve through GetRecords (which fills the scratch
    // CString array torn down after each call): P / J / C.
    CAnimNameRecord* recs;
    CString* slot;
    i32 cnt;

    recs = g_typeColl.GetNameRecords(static_cast<void*>((unit->m_objAux->m_1c)));
    slot = reinterpret_cast<CString*>(g_typeColl.m_alloc);
    cnt = g_typeColl.m_grown;
    while (cnt != 0) {
        if (slot != 0) {
            slot->~CString();
        }
        slot++;
        cnt--;
    }
    eq = (strcmp(recs->m_name, "P") == 0);
    if (eq) {
        return 0;
    }

    recs = g_typeColl.GetNameRecords(static_cast<void*>((unit->m_objAux->m_1c)));
    slot = reinterpret_cast<CString*>(g_typeColl.m_alloc);
    cnt = g_typeColl.m_grown;
    while (cnt != 0) {
        if (slot != 0) {
            slot->~CString();
        }
        slot++;
        cnt--;
    }
    eq = (strcmp(recs->m_name, "J") == 0);
    if (eq) {
        return 0;
    }

    recs = g_typeColl.GetNameRecords(static_cast<void*>((unit->m_objAux->m_1c)));
    slot = reinterpret_cast<CString*>(g_typeColl.m_alloc);
    cnt = g_typeColl.m_grown;
    while (cnt != 0) {
        if (slot != 0) {
            slot->~CString();
        }
        slot++;
        cnt--;
    }
    eq = (strcmp(recs->m_name, "C") == 0);
    if (eq) {
        return 0;
    }

    // Map the candidate index, or Probe/Reserve a fresh slot.
    i32 ci = reinterpret_cast<i32>(unit->m_objAux->m_1c);
    char* sel;
    g_typeColl.m_grown = 0;
    if (ci >= g_typeColl.m_lo && ci <= g_typeColl.m_hi) {
        sel = g_typeColl.m_base + (ci - g_typeColl.m_lo) * g_typeColl.m_stride;
    } else if (g_typeColl.Probe(ci, 0) != 0) {
        sel = g_typeColl.m_base + (ci - g_typeColl.m_lo) * g_typeColl.m_stride;
    } else {
        g_typeColl.Reserve(static_cast<CAnimNameRecord*>(g_projActCache), 0xc);
        sel = g_typeColl.m_spare;
    }

    // Tear down the scratch again, then compare the selected name to "R".
    slot = reinterpret_cast<CString*>(g_typeColl.m_alloc);
    cnt = g_typeColl.m_grown;
    while (cnt != 0) {
        if (slot != 0) {
            slot->~CString();
        }
        slot++;
        cnt--;
    }
    return strcmp((reinterpret_cast<CAnimNameRecord*>(sel))->m_name, "R") != 0;
}

RVA(0x00034960, 0x24)
void zErrHandling::Report(i32 sentinel, i32 code) {
    g_retAddrBreadcrumb = GetRetAddr();
    m_errSink->Set(static_cast<void*>(this), sentinel, code);
}

// ===========================================================================
// CBattlezMapConfig::Method_034c70  @0x034c70
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
// deep-chain regalloc plateau (~72%): the board-tile lookup, the budget gate, the
// Method_4b320 spawn, both coord-recycle loops (coord-pool vs g_coordPool.m_freeHead) and the
// reset block are reconstructed in shape + order. The `m_dwell <= m_reserveBudget`
// budget gate was a SIGNEDNESS bug (retail jbe, we emitted jle) - now cast to u32
// (68->72). Residual: retail pins the unit in edi / the zero const in ebx (zero-register
// pin) and the tile-index math (m_arrivalCol*7) spills to different stack slots than
// MSVC5 here. Foreign unit/board chains modeled by raw offset. Deferred to final sweep.
RVA(0x00034c70, 0x133)
i32 CBattlezMapConfig::Method_034c70(i32 unitArg) {
    CGrunt* unit = reinterpret_cast<CGrunt*>(unitArg);
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
        {
            GruntCoordNode* n = unit->CoordHead();
            while (n != 0) {
                GruntCoordNode* cur = n;
                n = n->m_next;
                if (cur->m_coord != 0) {
                    g_coordPool.Push(cur->m_coord);
                }
            }
        }
        unit->m_31c.RemoveAll();
    } else {
        unit->m_2d8 = 4;
        if (unit->CoordCount() != 0) {
            GruntCoordNode* n = unit->CoordHead();
            while (n != 0) {
                GruntCoordNode* cur = n;
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
// CBattlezMapConfig::Method_0350d0  @0x0350d0
// Periodic re-path of `unit` toward the nearest free candidate cell. Gate on the
// unit's m_dwell timer exceeding the bundle's m_repathBudget budget; otherwise walk the grid
// object's candidate list (head at m_triggerMgr->m_4), and among the unoccupied candidates
// (sub->m_occupied == 0, and not already exactly on the unit's level coord) keep the one
// nearest (min squared distance) to the unit's level (>>5) coordinate. If one is
// found, re-path the unit to it via Method_0300c0 (flags 0xd87). Clear m_dwell and
// return 1.
// ===========================================================================
// @early-stop
// regalloc/spill wall (~78%): logic byte-exact. Inverting the gate to `if (dwell >
// budget) { body }` + one shared `return 1` put the body inline (retail's jbe layout,
// 73->78). Residual: retail spills BOTH the list iterator and bestDist to stack locals
// (frame 0x10, reloading `arg1` from its stack slot each iteration), where MSVC5 here
// keeps the iterator in ebp and bestDist in ecx (frame 0x8, no reload) - the higher-
// spill-pressure choice. No source lever forces the spill under /O2; the divergence
// cascades through every loop register operand. Final sweep.
RVA(0x000350d0, 0xfa)
i32 CBattlezMapConfig::Method_0350d0(i32 unitArg) {
    CGrunt* unit = reinterpret_cast<CGrunt*>(unitArg);
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
            Method_0300c0(unitArg, best->m_tileX, best->m_tileY, 0xd87, 0, 0);
        }
        unit->m_dwell = 0;
    }
    return 1;
}

RVA(0x00035210, 0x4f)
i32 CBattlezMapConfig::Method_035210(i32 x, i32 y) {
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
i32 CBattlezMapConfig::Method_035550(i32 unitArg) {
    CGrunt* unit = reinterpret_cast<CGrunt*>(unitArg);
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
// CBattlezMapConfig::Method_0358a0  @0x0358a0  (__thiscall ret 4 => 1 CGrunt* arg)
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
// 0x2d6 (726 B) no-EH grid policy step: the body reproduces all four arms (random-band
// retarget, fixed-band re-place, despawn-recycle, near-band keep) incl. the signed
// rand()%4 / idiv rand()%cnt modulo idioms and both coord recyclers (g_coordPool vs
// g_coordPool.m_freeHead). The plateau is the documented register-relative record-address regalloc
// wall (cl strength-reduces the idx*0x238 lea-chain + folds the band sub-object offsets
// differently across the four arms) and the dead saved-m_arrivalCol reload; logic complete.
RVA(0x000358a0, 0x2d6)
i32 CBattlezMapConfig::Method_0358a0(i32 unitArg) {
    CGrunt* unit = reinterpret_cast<CGrunt*>(unitArg);
    char* recA = 0;
    char* recB0 = 0;
    i32 cell = unit->m_arrivalCol;
    if (cell >= 0 && cell < 4) {
        recA = reinterpret_cast<char*>(&m_ctx->m_options[cell]);
        recB0 = reinterpret_cast<char*>(&m_ctx->m_options[cell].m_038);
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
                Coord** arr = reinterpret_cast<Coord**>(b->m_0f0.GetData()); // the CPtrArray band
                Coord* pair = arr[rand() % cnt];
                x = pair->m_x;
                y = pair->m_y;
            }
            if (unit->TileSwitch(x, y, 0, 0x9cf, 0, 0x4020) != 0) {
                unit->m_arrivalCol = band;
                unit->m_arrivalRow = 0;
                Method_02c080(reinterpret_cast<i32>(unit));
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
        i32 y = *reinterpret_cast<i32*>((recB + 0xd4));
        i32 x = *reinterpret_cast<i32*>((recB + 0xd0));
        unit->TileSwitch(x, y, 0, 0x987, 0, 0x4068);
        unit->m_dwell = 0;
        return 1;
    }
    if (recA == 0 || recB0 == 0) {
        unit->m_arrivalCol = -1;
        unit->m_arrivalRow = -1;
        return 1;
    }
    if (*reinterpret_cast<i32*>((recA + 0x14)) == 0 && *reinterpret_cast<i32*>(recB0) == 0) {
        GruntCoordNode* n = unit->CoordHead();
        while (n != 0) {
            GruntCoordNode* cur = n;
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
    i32 cnt2 = *reinterpret_cast<i32*>((recB0 + 0xf8));
    if (cnt2 > 0) {
        Coord** vec = *reinterpret_cast<Coord***>((recB0 + 0xf4));
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
    GruntCoordNode* n = unit->CoordHead();
    while (n != 0) {
        GruntCoordNode* cur = n;
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
SIZE_UNKNOWN(CAnimNameRecord);
