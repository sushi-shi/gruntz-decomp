#include <rva.h>

#include <Gruntz/GruntCombat.h>

#include <Mfc.h>

#include <Bute/ButeMgr.h>
#include <Bute/ButeTree.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSubMgrLeaf.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Dsndmgr/DirectSoundMgr.h>
#include <Enums.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniElement.h>
#include <Gruntz/ArrivalFlagsPreset.h>
#include <Gruntz/BattlezMapConfig.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/EnemyAiType.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameModeId.h>
#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntAiState.h>
#include <Gruntz/GruntDeathType.h>
#include <Gruntz/GruntDirection.h>
#include <Gruntz/GruntEntranceArrival.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Gruntz/GruntzMapMgr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/HealthPct.h>
#include <Gruntz/LeafCue.h>
#include <Gruntz/LightFx.h>
#include <Gruntz/MovingLogicSerial.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialRecords.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SoundCue.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/SpellzEffect.h>
#include <Gruntz/StaminaPct.h>
#include <Gruntz/TraitorMode.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/WorkerHandler.h>
#include <Ints.h>
#include <Rez/FrameClock.h>
#include <Utils/MapTyped.h>
#include <Wap32/Object.h>
#include <Wap32/Rect.h>
#include <Wap32/TileGeometry.h>
#include <Wwd/AnimWorkerAct.h>

#include <math.h>
#include <new>
#include <stdlib.h>
#include <string.h>

#pragma intrinsic(strcmp, sqrt)

static const char s_GRUNTZ_[] = "GRUNTZ_";
DATA(0x0020dd40)
static const char s__LOSEITEM[] = "_LOSEITEM";
DATA(0x0020a680)
static const char s_SingleAnimation[] = "SingleAnimation";

// damage% by [entrance reason][attack kind]; retail .rdata 23x23 matrix
DATA(0x001e9788)
const u8 g_hitTable[23][23] = {
    {5, 100, 30, 20, 40, 25, 5, 10, 15, 50, 5, 40, 5, 30, 25, 20, 50, 100, 10, 0, 100, 100, 10},
    {100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
     100, 100, 100, 100, 100, 100, 100, 0,   100, 100, 100},
    {5, 100, 30, 20, 40, 25, 5, 10, 15, 50, 5, 40, 5, 30, 25, 20, 50, 100, 10, 0, 100, 100, 10},
    {5, 100, 30, 20, 40, 25, 5, 10, 15, 50, 5, 40, 5, 30, 25, 20, 50, 100, 10, 0, 100, 100, 10},
    {5, 100, 30, 20, 40, 25, 5, 10, 15, 50, 5, 40, 5, 30, 25, 20, 50, 100, 10, 0, 100, 100, 10},
    {5, 100, 30, 20, 40, 25, 5, 10, 15, 50, 5, 40, 5, 30, 25, 20, 50, 100, 10, 0, 100, 100, 10},
    {5, 100, 30, 20, 40, 25, 5, 10, 15, 50, 5, 40, 5, 30, 25, 20, 50, 100, 10, 0, 100, 100, 10},
    {5, 100, 30, 20, 40, 25, 5, 10, 15, 50, 5, 40, 5, 30, 25, 20, 50, 100, 10, 0, 100, 100, 10},
    {5, 100, 30, 20, 40, 25, 5, 10, 15, 50, 5, 40, 5, 30, 25, 20, 50, 100, 10, 0, 100, 100, 10},
    {5, 100, 15, 20, 40, 25, 5, 10, 15, 25, 0, 20, 5, 30, 25, 20, 50, 100, 10, 0, 100, 50, 5},
    {5, 100, 30, 20, 40, 25, 5, 10, 15, 50, 5, 40, 5, 30, 25, 20, 50, 100, 10, 0, 100, 100, 10},
    {5, 100, 30, 20, 40, 25, 5, 10, 15, 50, 5, 40, 5, 30, 25, 20, 50, 100, 10, 0, 100, 100, 10},
    {0, 100, 30, 10, 20, 10, 0, 5, 5, 50, 5, 40, 0, 15, 10, 10, 25, 100, 5, 0, 100, 100, 10},
    {5, 100, 30, 20, 40, 25, 5, 10, 15, 50, 5, 40, 5, 30, 25, 20, 50, 100, 10, 0, 100, 100, 10},
    {5, 100, 30, 20, 40, 25, 5, 10, 15, 50, 5, 40, 5, 30, 25, 20, 50, 100, 10, 0, 100, 100, 10},
    {5, 100, 30, 20, 40, 25, 5, 10, 15, 50, 5, 40, 5, 30, 25, 20, 50, 100, 10, 0, 100, 100, 10},
    {5, 100, 30, 20, 40, 25, 5, 10, 15, 50, 5, 40, 5, 30, 25, 20, 50, 100, 10, 0, 100, 100, 10},
    {5, 100, 30, 20, 40, 25, 5, 10, 15, 50, 5, 40, 5, 30, 25, 20, 50, 100, 10, 0, 100, 100, 10},
    {5, 100, 30, 20, 40, 25, 5, 10, 15, 50, 5, 40, 5, 30, 25, 20, 50, 100, 10, 0, 100, 100, 10},
    {5, 100, 30, 20, 40, 25, 5, 10, 15, 50, 5, 40, 5, 30, 25, 20, 50, 100, 10, 0, 100, 100, 10},
    {5, 100, 30, 20, 40, 25, 5, 10, 15, 50, 5, 40, 5, 30, 25, 20, 50, 100, 10, 0, 100, 100, 10},
    {5, 100, 30, 20, 40, 25, 5, 10, 15, 50, 5, 40, 5, 30, 25, 20, 50, 100, 10, 0, 100, 100, 10},
    {5, 100, 30, 20, 40, 25, 5, 10, 15, 50, 5, 40, 5, 30, 25, 20, 50, 100, 10, 0, 100, 100, 10},
};

// zero-init; filled at runtime by the (unreconstructed) fn in gap 0x58f3c-0x59230
DATA(0x00244970)
i32 g_dirVec[9][4];

DATA(0x00244ab0)
GruntDirectionCell g_gruntDirNorth = GruntDirectionCell(0, 1, 1);
DATA(0x00244ae0)
GruntDirectionCell g_gruntDirNorthEast = GruntDirectionCell(0, 2, 2);
DATA(0x00244aa0)
GruntDirectionCell g_gruntDirEast = GruntDirectionCell(1, 2, 3);
DATA(0x00244b28)
GruntDirectionCell g_gruntDirSouthEast = GruntDirectionCell(2, 2, 4);
DATA(0x00244ac0)
GruntDirectionCell g_gruntDirSouth = GruntDirectionCell(2, 1, 5);
DATA(0x00244b48)
GruntDirectionCell g_gruntDirSouthWest = GruntDirectionCell(2, 0, 6);
DATA(0x00244ad0)
GruntDirectionCell g_gruntDirWest = GruntDirectionCell(1, 0, 7);
DATA(0x00244b18)
GruntDirectionCell g_gruntDirNorthWest = GruntDirectionCell(0, 0, 8);
DATA(0x00244b38)
GruntDirectionCell g_gruntDirCenter = GruntDirectionCell(1, 1, 0);

DATA(0x0020d7fc)
char s_codeH[] = "H";

DATA(0x0020d2e8)
char s_codeF[] = "F";

template<> DATA(0x00244af0)
CActReg CActRegPool<CGrunt>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

static char s_TimePerTile[] = "TimePerTile";
static char s_Grunt[] = "Grunt";
static char s_EntranceSafeTime[] = "EntranceSafeTime";
static char s_IdleDelay[] = "IdleDelay";
static char s_PlayerDefenderRadius[] = "PlayerDefenderRadius";
DATA(0x0020df84)
static char s_CombatTimeout[] = "CombatTimeout";

DATA(0x0020dd30)
static const char s_GAME_ATTACK[] = "GAME_ATTACK";
static char s_Spellz[] = "Spellz";
DATA(0x0020dcf8)
static char s_FreezeRadius[] = "FreezeRadius";
DATA(0x0020dce8)
static char s_HealthRadius[] = "HealthRadius";
DATA(0x0020dcd0)
static char s_RessurectionRadius[] = "RessurectionRadius";
DATA(0x0020dcc0)
static char s_ToyzRadius[] = "ToyzRadius";
DATA(0x0020dcac)
static char s_TeleportRadius[] = "TeleportRadius";
DATA(0x0020dc78)
static char s_RollingBallzSpeed[] = "RollingBallzSpeed";
DATA(0x0020dc64)
static char s_RollingBallzTime[] = "RollingBallzTime";

DATA(0x0020df6c)
static const char s_CONVERSIONHIT[] = "GAME_CONVERSIONHIT";
DATA(0x0020df54)
static const char s_DEATHTOUCHHIT[] = "GAME_DEATHTOUCHHIT";
DATA(0x0020de1c)
static const char s_IMPACTMM1[] = "GRUNTZ_NORMALGRUNT_IMPACTMM1";
DATA(0x0020dd8c)
static const char s_IMPACTMM2[] = "GRUNTZ_NORMALGRUNT_IMPACTMM2";
DATA(0x0020df30)
static const char s_IMPACTMM3[] = "GRUNTZ_NORMALGRUNT_IMPACTMM3";
DATA(0x0020df0c)
static const char s_IMPACTMM4[] = "GRUNTZ_NORMALGRUNT_IMPACTMM4";
DATA(0x0020ddf8)
static const char s_IMPACTWM1[] = "GRUNTZ_NORMALGRUNT_IMPACTWM1";
DATA(0x0020ddb0)
static const char s_IMPACTWM2[] = "GRUNTZ_NORMALGRUNT_IMPACTWM2";
DATA(0x0020dd68)
static const char s_IMPACTWM3[] = "GRUNTZ_NORMALGRUNT_IMPACTWM3";
DATA(0x0020ddd4)
static const char s_BLOCKBODY1[] = "GRUNTZ_NORMALGRUNT_BLOCKBODY1";
DATA(0x0020de40)
static const char s_BLOCKBODY2[] = "GRUNTZ_NORMALGRUNT_BLOCKBODY2";
DATA(0x0020dee4)
static const char s_BLOCKMETAL1[] = "GRUNTZ_NORMALGRUNT_BLOCKMETAL1";
DATA(0x0020deb8)
static const char s_SPRING2[] = "GRUNTZ_SPRINGGRUNT_SPRINGGRUNTS2S1";
DATA(0x0020de8c)
static const char s_SPRING1[] = "GRUNTZ_SPRINGGRUNT_SPRINGGRUNTS1S1";
DATA(0x0020de64)
static const char s_TOOBZ[] = "GRUNTZ_TOOBGRUNT_TOOBZGRUNTUI1B";
static const char s_typeO[] = "O";
DATA(0x0020dd4c)
static const char s_knockKey[] = "KnockBackTimePerTile";
static const char s_gruntSec[] = "Grunt";

static inline void GruntScratchTeardown() {
    CString* slot = (g_typeColl.Slots());
    i32 cnt = g_typeColl.m_grown;
    while (cnt != 0) {
        if (slot != NULL) {
            slot->~CString();
        }
        slot++;
        cnt--;
    }
}

#define LK(key)                                                                                    \
    do {                                                                                           \
        LeafCue* out = 0;                                                                          \
        MapLookup(g_gameReg->m_world->m_soundRegistry->m_cues, (key), out);                        \
        cue = out;                                                                                 \
    } while (0)

#define SETDIR(k, nx, ny)                                                                          \
    do {                                                                                           \
        this->m_entranceCell.row = g_dirVec[k][0];                                                 \
        this->m_entranceCell.column = g_dirVec[k][1];                                              \
        this->m_entranceCell.direction = g_dirVec[k][2];                                           \
        newX = (nx);                                                                               \
        newY = (ny);                                                                               \
    } while (0)

// @early-stop
RVA(0x00056f80, 0xb0)
void CGrunt::EntranceTileOffset(i32* out) {
    i32 x = m_lastTilePx.m_x;
    i32 y = m_lastTilePx.m_y;
    switch (m_entranceCell.direction) {
        case DIR_NORTH:
            y -= 0x20;
            break;
        case DIR_NORTHEAST:
            x += 0x20;
            y -= 0x20;
            break;
        case DIR_EAST:
            x += 0x20;
            break;
        case DIR_SOUTHEAST:
            x += 0x20;
            y += 0x20;
            break;
        case DIR_SOUTH:
            y += 0x20;
            break;
        case DIR_SOUTHWEST:
            x -= 0x20;
            y += 0x20;
            break;
        case DIR_WEST:
            x -= 0x20;
            break;
        case DIR_NORTHWEST:
            x -= 0x20;
            y -= 0x20;
            break;
    }
    out[0] = x;
    out[1] = y;
}

#define SCAN_BOUNDS(grid)                                                                          \
    {                                                                                              \
        CRect rb(0, 0, (grid)->m_width, (grid)->m_height);                                         \
        RECT ra;                                                                                   \
        ra = CRect(0, 0, (grid)->m_width, (grid)->m_height);                                       \
        if (!IntersectRect(&(grid)->m_bounds, &ra, &rb)) {                                         \
            (grid)->m_bounds = ra;                                                                 \
        }                                                                                          \
        (grid)->m_gridW = (grid)->m_bounds.right - (grid)->m_bounds.left;                          \
        (grid)->m_gridH = (grid)->m_bounds.bottom - (grid)->m_bounds.top;                          \
    }

#define SCAN_BOUNDS_PLAINCLIP(grid)                                                                \
    {                                                                                              \
        RECT rb;                                                                                   \
        rb.left = 0;                                                                               \
        rb.top = 0;                                                                                \
        rb.right = (grid)->m_width;                                                                \
        rb.bottom = (grid)->m_height;                                                              \
        RECT ra;                                                                                   \
        ra = CRect(0, 0, (grid)->m_width, (grid)->m_height);                                       \
        if (!IntersectRect(&(grid)->m_bounds, &ra, &rb)) {                                         \
            (grid)->m_bounds = ra;                                                                 \
        }                                                                                          \
        (grid)->m_gridW = (grid)->m_bounds.right - (grid)->m_bounds.left;                          \
        (grid)->m_gridH = (grid)->m_bounds.bottom - (grid)->m_bounds.top;                          \
    }

#define FREELIST_PUSH(elem)                                                                        \
    {                                                                                              \
        CoordPoolNode* node = g_coordPool.NodeOf((elem));                                          \
        node->m_next = g_coordPool.m_freeHead;                                                     \
        g_coordPool.m_freeHead = node;                                                             \
    }

RVA(0x00057060, 0x72)
void CGrunt::ComputeFacing(double dt) {
    CWwdGameObjectA* h = m_object;
    double dx = static_cast<double>(m_lastTilePx.m_x) - static_cast<double>(h->m_screenX);
    double dy = static_cast<double>(m_lastTilePx.m_y) - static_cast<double>(h->m_screenY);

    m_moveSpeed =
        (sqrt(dx * dx + dy * dy) / static_cast<double>(static_cast<u32>(m_timePerTile))) * dt;
    m_movePosX = static_cast<double>(h->m_screenX);
    m_movePosY = static_cast<double>(h->m_screenY);
}

static inline CString* ActNameSlots() {
    return g_typeColl.Slots();
}

#define BIND_ACT_644AF0(id, handler)                                                               \
    {                                                                                              \
        GruntActPmf _p;                                                                            \
        _p.m_pmf = (handler);                                                                      \
        *CActRegPool<CGrunt>::s_table.Resolve(id) = _p.m_h;                                        \
    }

#define REGISTER_KEY_644AF0(key, handler)                                                          \
    {                                                                                              \
        i32 id = ActFindId(key);                                                                   \
        if (id == 0) {                                                                             \
            ActInsertId(key, g_typeCounter);                                                       \
            id = g_typeCounter;                                                                    \
            /* Keep the global counter as the name-slot lookup's direct argument; */               \
            /* using id changes MSVC's two-consumer CSE and register allocation. */                \
            /* See docs/patterns/act-registrar-counter-cse-and-freeloop.md. */                     \
            CString* slot = g_typeColl.ScratchResolve(g_typeCounter);                              \
            i32 n = g_typeColl.m_grown;                                                            \
            CString* list = ActNameSlots();                                                        \
            while (n-- != 0) {                                                                     \
                if (list != 0) {                                                                   \
                    list->CString::~CString();                                                     \
                }                                                                                  \
                list++;                                                                            \
            }                                                                                      \
            *slot = (key);                                                                         \
            g_typeCounter++;                                                                       \
        }                                                                                          \
        BIND_ACT_644AF0(id, handler);                                                              \
    }

#define REGISTER_KEY_644AF0_DERIVED(key, handler)                                                  \
    {                                                                                              \
        i32 id = ActFindId(key);                                                                   \
        if (id == 0) {                                                                             \
            ActInsertId(key, g_typeCounter);                                                       \
            id = g_typeCounter;                                                                    \
            *g_typeColl.SlotOf(id) = (key);                                                        \
            g_typeCounter++;                                                                       \
        }                                                                                          \
        BIND_ACT_644AF0(id, handler);                                                              \
    }

// @early-stop
RVA(0x00057100, 0x577)
i32 CGrunt::LoadGruntAbilityTuning(i32 forced) {
    // `forced` arrives from m_moveVariant, which also carries a raw cue-variant
    // index, so the spell domain is entered here.
    SpellzEffect idx = static_cast<SpellzEffect>(forced);
    if (forced == 0) {
        i32 m = 3;
        if (g_gameReg->m_gameMode != GAMEMODE_SINGLE) {
            m = 6;
        }
        if (m == 0) {
            idx = static_cast<SpellzEffect>(rand() & 1);
        } else {
            idx = static_cast<SpellzEffect>(rand() % m + 1);
        }
    }

    CDDrawSubMgrLeafScan* slot =
        (static_cast<CDDrawSurfaceMgr*>(m_animWorker->m_ownerCtx))->m_soundRegistry;
    if (slot->m_emitGate == 0) {
        LeafCue* sout = 0;
        MapLookup(slot->m_cues, s_GAME_ATTACK, sout);
        if (sout != NULL) {

            sout->PlayIfElapsed(g_sndCueTag, 0, 0, 0);
        }
    }

    switch (idx) {
        case SPELLZ_FREEZE: {
            CGameObject* spr = g_gameReg->m_world->m_childGroup->CreateSprite(
                0,
                m_lastTilePx.m_x,
                m_lastTilePx.m_y,
                SORTKEY_OVERLAY,
                "LightFx",
                0x40003
            );
            spr->m_animWorker->m_notify(spr);
            (static_cast<CLightFx*>(spr->m_animWorker->m_logic))
                ->Activate("GAME_LIGHTING_FLASH", "GAME_FLASH", 9, 1);
            return m_tileMgr->CombatCue(
                m_lastTilePx.m_x,
                m_lastTilePx.m_y,
                g_buteMgr.GetIntDef(s_Spellz, s_FreezeRadius, 8),
                4,
                -1
            );
        }
        case SPELLZ_HEALTH: {
            CGameObject* spr = g_gameReg->m_world->m_childGroup->CreateSprite(
                0,
                m_lastTilePx.m_x,
                m_lastTilePx.m_y,
                SORTKEY_OVERLAY,
                "LightFx",
                0x40003
            );
            spr->m_animWorker->m_notify(spr);
            (static_cast<CLightFx*>(spr->m_animWorker->m_logic))
                ->Activate("GAME_LIGHTING_FLASH", "GAME_FLASH", 2, 1);
            return m_tileMgr->CombatCue(
                m_lastTilePx.m_x,
                m_lastTilePx.m_y,
                g_buteMgr.GetIntDef(s_Spellz, s_HealthRadius, 8),
                3,
                -1
            );
        }
        case SPELLZ_RESURRECTION: {
            CGameObject* spr = g_gameReg->m_world->m_childGroup->CreateSprite(
                0,
                m_lastTilePx.m_x,
                m_lastTilePx.m_y,
                SORTKEY_OVERLAY,
                "LightFx",
                0x40003
            );
            spr->m_animWorker->m_notify(spr);
            (static_cast<CLightFx*>(spr->m_animWorker->m_logic))
                ->Activate("GAME_LIGHTING_FLASH", "GAME_FLASH", 8, 1);
            return m_tileMgr->LoadGruntResurrectTuning(
                m_lastTilePx.m_x,
                m_lastTilePx.m_y,
                g_buteMgr.GetIntDef(s_Spellz, s_RessurectionRadius, 8)
            );
        }
        case SPELLZ_TOYZ: {
            CGameObject* spr = g_gameReg->m_world->m_childGroup->CreateSprite(
                0,
                m_lastTilePx.m_x,
                m_lastTilePx.m_y,
                SORTKEY_OVERLAY,
                "LightFx",
                0x40003
            );
            spr->m_animWorker->m_notify(spr);
            (static_cast<CLightFx*>(spr->m_animWorker->m_logic))
                ->Activate("GAME_LIGHTING_FLASH", "GAME_FLASH", 7, 1);
            return m_tileMgr->CombatCue(
                m_lastTilePx.m_x,
                m_lastTilePx.m_y,
                g_buteMgr.GetIntDef(s_Spellz, s_ToyzRadius, 8),
                5,
                -1
            );
        }
        case SPELLZ_TELEPORT: {
            CGameObject* spr = g_gameReg->m_world->m_childGroup->CreateSprite(
                0,
                m_lastTilePx.m_x,
                m_lastTilePx.m_y,
                SORTKEY_OVERLAY,
                "LightFx",
                0x40003
            );
            spr->m_animWorker->m_notify(spr);
            (static_cast<CLightFx*>(spr->m_animWorker->m_logic))
                ->Activate("GAME_LIGHTING_FLASH", "GAME_FLASH", 3, 1);
            return m_tileMgr->CombatCue(
                m_lastTilePx.m_x,
                m_lastTilePx.m_y,
                g_buteMgr.GetIntDef(s_Spellz, s_TeleportRadius, 8),
                2,
                -1
            );
        }
        case SPELLZ_ROLLINGBALL: {
            CWwdGameObjectA* n = g_gameReg->m_world->m_childGroup->CreateSprite(
                0,
                m_lastTilePx.m_x,
                m_lastTilePx.m_y - 0x20,
                0,
                "RollingBall",
                0x40003
            );
            n->ApplyName("LEVEL_ROLLINGBALL_NORTH");
            AnimWorkerObj* ni = n->m_animWorker;
            ni->m_speed =
                static_cast<i32>(g_buteMgr.GetDwordDef(s_Spellz, s_RollingBallzSpeed, 0x3e8));
            n->m_smarts = 0;
            n->m_points =
                static_cast<i32>(g_buteMgr.GetDwordDef(s_Spellz, s_RollingBallzTime, 0x3e8));

            CWwdGameObjectA* e = g_gameReg->m_world->m_childGroup->CreateSprite(
                0,
                m_lastTilePx.m_x + 0x20,
                m_lastTilePx.m_y,
                0,
                "RollingBall",
                0x40003
            );
            e->ApplyName("LEVEL_ROLLINGBALL_EAST");
            AnimWorkerObj* ei = e->m_animWorker;
            ei->m_speed =
                static_cast<i32>(g_buteMgr.GetDwordDef(s_Spellz, s_RollingBallzSpeed, 0x3e8));
            e->m_smarts = 0;
            e->m_points =
                static_cast<i32>(g_buteMgr.GetDwordDef(s_Spellz, s_RollingBallzTime, 0x3e8));

            CWwdGameObjectA* s = g_gameReg->m_world->m_childGroup->CreateSprite(
                0,
                m_lastTilePx.m_x,
                m_lastTilePx.m_y + 0x20,
                0,
                "RollingBall",
                0x40003
            );
            s->ApplyName("LEVEL_ROLLINGBALL_SOUTH");
            AnimWorkerObj* si = s->m_animWorker;
            si->m_speed =
                static_cast<i32>(g_buteMgr.GetDwordDef(s_Spellz, s_RollingBallzSpeed, 0x3e8));
            s->m_smarts = 0;
            s->m_points =
                static_cast<i32>(g_buteMgr.GetDwordDef(s_Spellz, s_RollingBallzTime, 0x3e8));

            CWwdGameObjectA* w = g_gameReg->m_world->m_childGroup->CreateSprite(
                0,
                m_lastTilePx.m_x - 0x20,
                m_lastTilePx.m_y,
                0,
                "RollingBall",
                0x40003
            );
            w->ApplyName("LEVEL_ROLLINGBALL_WEST");
            AnimWorkerObj* wi = w->m_animWorker;
            wi->m_speed =
                static_cast<i32>(g_buteMgr.GetDwordDef(s_Spellz, s_RollingBallzSpeed, 0x3e8));
            w->m_smarts = 0;
            w->m_points =
                static_cast<i32>(g_buteMgr.GetDwordDef(s_Spellz, s_RollingBallzTime, 0x3e8));
            return 1;
        }
        default:
            return 0;
    }
}

RVA(0x00057800, 0x64)
void CGrunt::SelectMoveIcon(i32 a) {
    if (m_moveIcon == a) {
        return;
    }
    m_moveIcon = static_cast<PickupType>(a);
    if (a < 0 || a >= IDX(PICKUP_TIMEBOMB)) {
        m_moveIcon = PICKUP_NONE;
    }
    CShadeTable* sel =
        g_gameReg->m_spriteFactory->GetSel(m_moveIcon, m_entranceReason >= PICKUP_TOYZ_FIRST);
    CWwdGameObjectA* h = m_object;
    h->m_drawActive = 1;
    h->m_drawFillCmd = SHADE_PAL_16;
    h->m_drawFillArg = sel;
}

RVA(0x00057890, 0x19c)
i32 CGrunt::BuildGruntLoseItemAnimation() {
    FinishActiveAction();
    PickupType reason = m_entranceReason;
    if (reason != PICKUP_TOOB && reason != PICKUP_WINGZ && reason != PICKUP_SPRING) {
        return 0;
    }

    CWwdGameObjectA* spr = g_gameReg->m_world->m_childGroup->CreateSprite(
        0,
        m_object->m_screenX,
        m_object->m_screenY,
        SORTKEY_ACTOR,
        s_SingleAnimation,
        0x40003
    );
    spr->ApplyName(s_GRUNTZ_ + m_animSetName + s__LOSEITEM);
    spr->ApplyLookupGeometry(s_GRUNTZ_ + m_animSetName + s__LOSEITEM, 0);

    CGruntzMgr* g = g_gameReg;
    i32 y = m_object->m_screenY;
    i32 x = m_object->m_screenX;
    CCueRect* rc = &g->m_world->m_level->m_mainPlane->m_viewRect;
    if (x < rc->right && x >= rc->left && y < rc->bottom && y >= rc->top) {
        g->m_cueSink->LoadGruntSpawnConfig(this, 0xe, -1, -1, -1);
    }

    LoadGruntTypeTable(PICKUP_NONE, 1, 0, 1);
    m_entranceActive = 0;
    return 1;
}

// @early-stop
RVA(0x00057aa0, 0x9b)
i32 CGrunt::TryPowerupAtTile() {
    PickupType reason = m_entranceReason;
    if (reason <= PICKUP_NONE || reason >= PICKUP_TOYZ_FIRST) {
        return 0;
    }
    CWwdGameObjectA* h = m_object;
    i32 mx = h->m_screenX;
    i32 my = h->m_screenY;
    CGruntzMapMgr* b = g_gameReg->m_tileGrid;
    i32 px = (mx & ~TILE_MASK_PX) + TILE_HALF_PX;
    i32 py = (my & ~TILE_MASK_PX) + TILE_HALF_PX;
    i32 tx = px >> TILE_SHIFT_PX;
    i32 ty = py >> TILE_SHIFT_PX;
    i32 flags;
    if (static_cast<u32>(tx) >= static_cast<u32>(b->m_width)
        || static_cast<u32>(ty) >= static_cast<u32>(b->m_height)) {
        flags = 1;
    } else {
        flags = b->m_rowInts[ty][tx * 7];
    }
    if ((flags & BRICKZ_BLOCKED_MASK) || (flags & 2)) {
        return 0;
    }
    m_tileMgr->LoadPowerupIconSprites(reason, px, py, 0, 1, 0);
    return 1;
}

// @early-stop
RVA(0x00057b70, 0x77)
void CGrunt::EnsureStruckSlot(const char* key) {
    DirectSoundMgr*& sample = m_struckSlotSound;
    if (sample != NULL) {
        return;
    }
    if (g_gameReg->m_soundEnabled == 0) {
        return;
    }
    void* entry_ob = 0;
    g_gameReg->m_world->m_soundRegistry->m_cues.Lookup(key, entry_ob);
    LeafCue* entry = static_cast<LeafCue*>(entry_ob);
    if (entry == NULL) {
        return;
    }
    if (entry->m_sound == NULL) {
        return;
    }
    sample = static_cast<DirectSoundMgr*>(entry->m_sound->GetItem());
    if (sample == NULL) {
        return;
    }
    sample->ApplyAndPlay(g_gameReg->m_soundVolume, 0, 0, 1);
}

RVA(0x00057c10, 0x1e)
void CGrunt::StopStruckSlotSound() {
    DirectSoundMgr* p = m_struckSlotSound;
    if (p) {
        p->StopAndRewind();
        m_struckSlotSound = NULL;
    }
}

RVA(0x00057c40, 0x71)
void CGrunt::EnsureStruckVoice(const char* key) {
    DirectSoundMgr*& sample = m_struckVoiceSound;
    if (sample != NULL) {
        return;
    }
    void* entry_ob = 0;
    g_gameReg->m_world->m_soundRegistry->m_cues.Lookup(key, entry_ob);
    LeafCue* entry = static_cast<LeafCue*>(entry_ob);
    if (entry == NULL) {
        return;
    }
    if (entry->m_sound == NULL) {
        return;
    }
    sample = static_cast<DirectSoundMgr*>(entry->m_sound->GetItem());
    if (sample == NULL) {
        return;
    }
    sample->ApplyAndPlay(g_gameReg->m_soundVolume, 0, 0, 1);
}

RVA(0x00057ce0, 0x1e)
void CGrunt::StopStruckVoiceSound() {
    DirectSoundMgr* p = m_struckVoiceSound;
    if (p) {
        p->StopAndRewind();
        m_struckVoiceSound = NULL;
    }
}

RVA(0x00057d10, 0x4e)
void CGrunt::ReapplyVoiceParams() {
    if (g_gameReg->m_soundEnabled == 0) {
        return;
    }
    DirectSoundMgr* a = m_struckSlotSound;
    if (a != NULL) {
        a->ApplyAndPlay(g_gameReg->m_soundVolume, 0, 0, 1);
    }
    DirectSoundMgr* b = m_struckVoiceSound;
    if (b != NULL) {
        b->ApplyAndPlay(g_gameReg->m_soundVolume, 0, 0, 1);
    }
}

RVA(0x00057d80, 0x11)
void CGrunt::DestroyAnims() {
    StopStruckSlotSound();
    StopStruckVoiceSound();
}

// @early-stop
RVA(0x00057db0, 0x8f8)
i32 CGrunt::PathScan() {
    CMapMgr* grid = g_gameReg->m_tileGrid;

    CPtrList* coordz = &m_coordList;
    if (CoordCount() == 0) {
        return 1;
    }

    POSITION node = coordz->GetHeadPosition();

    i32 col5 = m_object->m_screenX >> TILE_SHIFT_PX;
    i32 row5 = m_object->m_screenY >> TILE_SHIFT_PX;

    {
        RECT gb;
        gb.left = 0;
        gb.top = 0;
        gb.right = grid->m_width;
        gb.bottom = grid->m_height;
        RECT rs;
        rs.left = col5 - 2;
        rs.top = row5 - 2;
        rs.right = col5 + 2;
        rs.bottom = row5 + 2;
        RECT box;
        const RECT* pr = &rs;
        if (pr != NULL) {

            box = *pr;
            box.right++;
            box.bottom++;
        } else {
            box = CRect(0, 0, grid->m_width, grid->m_height);
        }
        if (!IntersectRect(&grid->m_bounds, &box, &gb)) {
            grid->m_bounds = box;
        }
        grid->m_gridW = grid->m_bounds.right - grid->m_bounds.left;
        grid->m_gridH = grid->m_bounds.bottom - grid->m_bounds.top;
    }

    CoordNode* tail = CoordTail();
    i32 tcol = tail->m_coord->m_x;
    i32 trow = tail->m_coord->m_y;
    i32 hits = 0;

    while (node != NULL) {
        Coord* co = static_cast<Coord*>(coordz->GetNext(node));
        if (co != NULL) {

            if ((grid->m_rows[co->m_y][co->m_x].m_flagBytes[3] & 0x20) == 0
                || (co->m_x == tcol && co->m_y == trow)) {

                CPtrList s(0xa);
                i32 res = grid->SearchEdge(
                    col5,
                    row5,
                    co->m_x,
                    co->m_y,
                    &s,
                    1,
                    m_arrivalFlags | 0x20000000,
                    m_passableMask
                );
                if (res != 0) {
                    if (s.GetCount() != 0) {

                        while (node != NULL) {
                            Coord* src = static_cast<Coord*>(coordz->GetNext(node));
                            Coord* fresh = 0;
                            CoordPoolNode* free = g_coordPool.m_freeHead;
                            if (free->m_next != NULL) {
                                fresh = &free->m_coord;
                                fresh->m_x = src->m_x;
                                fresh->m_y = src->m_y;
                                g_coordPool.m_freeHead = g_coordPool.m_freeHead->m_next;
                            }
                            s.AddTail(fresh);
                        }

                        if (CoordCount() != 0) {
                            POSITION pos = m_coordList.GetHeadPosition();
                            if (pos != NULL) {
                                do {
                                    void* d = m_coordList.GetNext(pos);
                                    if (d != NULL) {
                                        g_coordPool.Push(d);
                                    }
                                } while (pos != NULL);
                            }
                            coordz->RemoveAll();
                        }

                        POSITION p = s.GetHeadPosition();
                        if (p != NULL) {
                            do {
                                Coord* d = static_cast<Coord*>(s.GetNext(p));
                                if (d != NULL) {
                                    if (d->m_x != col5 || d->m_y != row5) {
                                        coordz->AddTail(d);
                                    }
                                }
                            } while (p != NULL);
                        }
                        void* elem = s.RemoveHead();
                        if (elem != NULL) {
                            FREELIST_PUSH(elem);
                        }
                        s.RemoveAll();
                        SCAN_BOUNDS_PLAINCLIP(grid);
                        return 1;
                    }
                } else {
                    hits++;
                }
            }
        }

        if (hits == 5) {
            SCAN_BOUNDS(grid);
            break;
        }
    }

    SCAN_BOUNDS(grid);

    RECT nb;
    nb.left = tcol - 4;
    nb.top = trow - 4;
    nb.right = tcol + 4;
    nb.bottom = trow + 4;
    if (col5 < nb.right && col5 >= nb.left && row5 < nb.bottom && row5 >= nb.top) {

        CRect rb(0, 0, grid->m_width, grid->m_height);
        RECT ra;
        const RECT* pn = &nb;
        if (pn != NULL) {
            ra = *pn;
            ra.right++;
            ra.bottom++;
        } else {
            ra = CRect(0, 0, grid->m_width, grid->m_height);
        }
        if (!IntersectRect(&grid->m_bounds, &ra, &rb)) {
            grid->m_bounds = ra;
        }
        grid->m_gridW = grid->m_bounds.right - grid->m_bounds.left;
        grid->m_gridH = grid->m_bounds.bottom - grid->m_bounds.top;

        for (i32 dy = -1; dy < 2; dy++) {
            for (i32 dx = -1; dx < 2; dx++) {
                if (dy == 0 && dx == 0) {
                    continue;
                }
                i32 rr = trow + dy;
                i32 cc = tcol + dx;

                i32 cf;
                if (static_cast<u32>(cc) < static_cast<u32>(grid->m_width)
                    && static_cast<u32>(rr) < static_cast<u32>(grid->m_height)) {
                    cf = ((grid->m_rowInts[rr]))[cc * 7];
                } else {
                    cf = 1;
                }
                i32 mf = (m_arrivalFlags | 0x20040002) & cf;
                if (mf & 0x20000000) {
                    continue;
                }
                if (mf != 0 && (m_passableMask & cf) == 0) {
                    continue;
                }
                CPtrList s(0xa);
                i32 res = grid->SearchEdge(
                    col5,
                    row5,
                    cc,
                    rr,
                    &s,
                    0,
                    m_arrivalFlags | 0x20040002,
                    m_passableMask
                );
                if (res != 0) {

                    if (s.GetCount() == 0) {
                        grid->Clip(0);
                        return 0;
                    }
                    void* elem = s.RemoveHead();
                    if (elem != NULL) {
                        FREELIST_PUSH(elem);
                    }
                    if (s.GetCount() == 0) {
                        grid->Clip(0);
                        return 0;
                    }

                    if (CoordCount() != 0) {
                        POSITION pos = m_coordList.GetHeadPosition();
                        if (pos != NULL) {
                            do {
                                void* d = static_cast<CGruntCoordList*>(coordz)->NextData(pos);
                                if (d != NULL) {
                                    g_coordPool.Push(d);
                                }
                            } while (pos != NULL);
                        }
                        coordz->RemoveAll();
                    }

                    POSITION p = s.GetHeadPosition();
                    if (p != NULL) {
                        do {
                            coordz->AddTail(s.GetNext(p));
                        } while (p != NULL);
                    }
                    s.RemoveAll();

                    if (grid->SearchEdge(cc, rr, tcol, trow, &s, 1, m_arrivalFlags, m_passableMask)
                        != 0) {
                        if (s.GetCount() != 0) {
                            void* e2 = s.RemoveHead();
                            if (e2 != NULL) {
                                FREELIST_PUSH(e2);
                            }
                            if (s.GetCount() != 0) {
                                POSITION q = s.GetHeadPosition();
                                if (q != NULL) {
                                    do {
                                        coordz->AddTail(s.GetNext(q));
                                    } while (q != NULL);
                                }
                                s.RemoveAll();
                            }
                        }
                    }
                    SCAN_BOUNDS(grid);
                    return 1;
                }
            }
        }
    }
    grid->Clip(0);
    return 0;
}

// @early-stop
RVA(0x000588f0, 0x1ea)
void CGrunt::OnStruck(i32 wasHit) {
    m_struckTimerLo = 0xfa0;
    m_struckTimerHi = 0;
    m_struckClockLo = static_cast<i32>(g_frameTime);
    m_struckClockHi = 0;
    i32 c = ++m_struckCount;

    if (wasHit == 0) {
        if (m_gruntKind == GRUNT_GHOST) {
            return;
        }
        i32 x = m_object->m_screenX;
        i32 y = m_object->m_screenY;
        if (c < 5) {
            CGruntzMgr* g = g_gameReg;
            const RECT* vr = &g->m_world->m_level->m_mainPlane->m_viewRect;
            if (x < vr->right && x >= vr->left && y < vr->bottom && y >= vr->top) {
                g->m_cueSink->SpawnVoiceDriver(this, 0x370, -1, 0, -1, -1);
            }
            return;
        }
        CGruntzMgr* g = g_gameReg;
        const RECT* vr = &g->m_world->m_level->m_mainPlane->m_viewRect;
        if (x < vr->right && x >= vr->left && y < vr->bottom && y >= vr->top) {
            g->m_cueSink->SpawnVoiceDriver(this, 0x371, -1, 0, -1, -1);
        } else {
            m_struckCount = 0;
        }
        return;
    }

    if (c < 5) {
        i32 x = m_object->m_screenX;
        i32 y = m_object->m_screenY;
        CGruntzMgr* g = g_gameReg;
        const RECT* vr = &g->m_world->m_level->m_mainPlane->m_viewRect;
        if (x < vr->right && x >= vr->left && y < vr->bottom && y >= vr->top) {
            g->m_cueSink->SpawnVoiceDriver(this, 0x320, -1, 0, -1, -1);
        }
        return;
    }
    if (c < 0xa) {
        i32 x = m_object->m_screenX;
        i32 y = m_object->m_screenY;
        CGruntzMgr* g = g_gameReg;
        const RECT* vr = &g->m_world->m_level->m_mainPlane->m_viewRect;
        if (x < vr->right && x >= vr->left && y < vr->bottom && y >= vr->top) {
            g->m_cueSink->SpawnVoiceDriver(this, 0x321, -1, 0, -1, -1);
        }
        return;
    }
    {
        i32 x = m_object->m_screenX;
        i32 y = m_object->m_screenY;
        m_struckCount = 0;
        CGruntzMgr* g = g_gameReg;
        const RECT* vr = &g->m_world->m_level->m_mainPlane->m_viewRect;
        if (x < vr->right && x >= vr->left && y < vr->bottom && y >= vr->top) {
            g->m_cueSink->SpawnVoiceDriver(this, 0x322, -1, 0, -1, -1);
        }
    }
}

// @early-stop
RVA(0x00059230, 0x40d)
i32 CGrunt::ArrivalRecycle(i32 a, i32 b, i32 mode, i32 d, i32 e) {
    if (mode == 0) {
        switch (m_arrivalState) {
            case AI_SMARTCHASER:
                m_arrivalCell.m_x = d;
                m_arrivalCell.m_y = e;
                break;
            case AI_DUMBCHASER:
            case AI_DEFENDER:
                m_arrivalCell.m_x = d;
                m_arrivalCell.m_y = e;
                m_defenderState = AISTATE_ATTACK;
                break;
            case AI_POSTGUARD:
                m_arrivalCell.m_x = d;
                m_arrivalCell.m_y = e;
                m_defenderState = AISTATE_ATTACK;
                break;
            case AI_HITANDRUNNER:
            case AI_OBJECTGUARD:
                m_arrivalCell.m_x = d;
                m_arrivalCell.m_y = e;
                m_defenderState = AISTATE_ATTACK;
                break;
            case AI_BATTLEZ_PATH:
                m_arrivalCell.m_x = d;
                m_arrivalCell.m_y = e;
                break;
            default:
                break;
        }

        i32 phase = m_arrivalPhase;
        if ((phase == 3 || phase == 2) && m_arrivalActive != 0) {
            CGrunt* occ = m_tileMgr->m_grid[m_arrivalCell.m_x * TM_GRID_COLS + m_arrivalCell.m_y];
            if (occ != NULL) {
                CGameObject* inner = occ->m_object;
                i32 yMasked = (inner->m_screenY & ~TILE_MASK_PX) + TILE_HALF_PX;
                i32 xMasked = (inner->m_screenX & ~TILE_MASK_PX) + TILE_HALF_PX;
                i32 hit;
                if (phase == 3) {
                    hit = RectContains(xMasked, yMasked);
                } else {
                    hit = RectContainsGated(xMasked, yMasked);
                }
                if (hit != 0) {
                    BuildEntranceAnimation(0);
                }
                if (phase == 3) {
                    m_tileMgr->ApplyTriggerB(
                        m_tileOwnerHi,
                        m_tileOwnerLo,
                        inner->m_screenX,
                        inner->m_screenY
                    );
                } else {
                    m_tileMgr->ApplyTriggerA(
                        m_tileOwnerHi,
                        m_tileOwnerLo,
                        inner->m_screenX,
                        inner->m_screenY
                    );
                }
            }
        }
        return 1;
    }

    PlayMoveSound(a, b);

    char* nm0 = *g_typeColl.GetNameRecord(m_objAux->m_actKey);
    bool eqH = (strcmp(nm0, s_codeH) == 0);
    if (eqH) {
        return 1;
    }
    {
        i32 coord = m_objAux->ActKey();
        g_typeColl.m_grown = 0;
        CString* rec;
        if (coord < g_typeColl.m_lo || coord > g_typeColl.m_hi) {
            if (g_typeColl.GrowTo(coord, 0) != NULL) {
                rec = g_typeColl.Elem(coord);
            } else {
                g_typeColl.Report(g_errOutOfMem, 0xc);
                rec = g_typeColl.Scratch();
            }
        } else {
            rec = g_typeColl.Elem(coord);
        }
        GruntScratchTeardown();
        static_cast<void>(rec);
    }
    char* nm1 = *g_typeColl.GetNameRecord(m_objAux->m_actKey);
    bool eqF = (strcmp(nm1, s_codeF) == 0);
    if (eqF) {
        return 1;
    }
    {
        i32 coord = m_objAux->ActKey();
        g_typeColl.m_grown = 0;
        CString* rec;
        if (coord < g_typeColl.m_lo || coord > g_typeColl.m_hi) {
            if (g_typeColl.GrowTo(coord, 0) != NULL) {
                rec = g_typeColl.Elem(coord);
            } else {
                char* msg = g_errOutOfMem;
                g_retAddrBreadcrumb = GetRetAddr();
                g_typeColl.m_errSink->Set(&g_typeColl, msg, 0xc);
                rec = g_typeColl.Scratch();
            }
        } else {
            rec = g_typeColl.Elem(coord);
        }
        GruntScratchTeardown();
        static_cast<void>(rec);
    }
    char* nm2 = *g_typeColl.GetNameRecord(m_objAux->m_actKey);
    bool eqO = (strcmp(nm2, s_codeO) == 0);
    if (eqO) {
        return 1;
    }
    ResetGeometry();
    return 1;
}

// @early-stop
RVA(0x000597a0, 0x1400)
i32 CGrunt::LoadGruntCombatAnimations(
    PickupType attackKind,
    i32 struckPose,
    i32 srcRow,
    i32 srcCol,
    i32 srcPxX,
    i32 srcPxY,
    i32 fromProjectile,
    PickupType attackerGruntKind
) {
    if (this->m_gruntKind == GRUNT_INVULNERABLE && this->m_entranceReason != PICKUP_BOMB) {
        return 1;
    }

    if (attackerGruntKind == GRUNT_CONVERSION) {
        CGrunt* enemy = m_tileMgr->m_grid[srcRow * TM_GRID_COLS + srcCol];
        if (enemy != NULL
            && m_tileMgr->SpawnGrunt(
                   this->m_tileOwnerHi,
                   this->m_tileOwnerLo,
                   srcRow,
                   enemy->m_moveIcon
               ) != 0) {
            i32 h = enemy->m_health + 0x19;
            if (h >= HEALTH_FULL) {
                h = HEALTH_FULL;
            }
            enemy->m_health = h;

            CDDrawSubMgrLeafScan* host =
                (static_cast<CDDrawSurfaceMgr*>(m_animWorker->m_ownerCtx))->m_soundRegistry;
            if (host->m_emitGate == 0) {
                LeafCue* cc = static_cast<LeafCue*>(host->Lookup(s_CONVERSIONHIT));
                if (cc != NULL) {
                    cc->PlayIfElapsed(g_sndCueTag, 0, 0, 0);
                }
            }
            return 0;
        }
    }

    i32 hit = AT(AT(g_hitTable, this->m_entranceReason), attackKind);
    CGruntzMgr* reg = g_gameReg;
    if (reg->m_isEasyMode != 0 && reg->m_gameMode == GAMEMODE_SINGLE
        && this->m_tileOwnerHi == g_curPlayer) {
        i32 t = hit / 2;
        hit = t + t % 5;
    }

    if (attackerGruntKind == GRUNT_DEATHTOUCH) {
        hit = 0x64;
    } else if (this->m_gruntKind == GRUNT_REACTIVEARMOR) {
        hit = static_cast<i32>((static_cast<float>(hit) * g_quarterScale));
        if (fromProjectile == 0) {
            CGrunt* enemy = m_tileMgr->m_grid[srcRow * TM_GRID_COLS + srcCol];
            if (enemy != NULL && enemy->m_entranceCommitted != 0) {
                i32 nh = enemy->m_health - hit * 3;
                if (nh < 0) {
                    nh = 0;
                }
                enemy->m_health = nh;
                if (nh <= 0) {
                    m_tileMgr->CellDispatch(srcRow, srcCol, DEATH_NORMAL, -1);
                }
            }
        }
    }

    i32 nh = this->m_health - hit;
    if (nh < 0) {
        nh = 0;
    }
    this->m_health = nh;
    if (this->m_entranceReason == PICKUP_BOMB) {
        m_tileMgr->CellDispatch(this->m_tileOwnerHi, this->m_tileOwnerLo, DEATH_NORMAL, srcRow);
        return 0;
    }
    if (nh <= 0) {
        this->m_entranceCommitted = 0;
        this->m_killerSlot = srcRow;
    }

    LeafCue* cue = 0;
    i32 vx = this->m_object->m_screenX;
    i32 vy = this->m_object->m_screenY;
    if (vx < reg->m_viewBounds.right && vx >= reg->m_viewBounds.left
        && vy < reg->m_viewBounds.bottom && vy >= reg->m_viewBounds.top) {
        if (attackerGruntKind == GRUNT_DEATHTOUCH) {
            LK(s_DEATHTOUCHHIT);
            goto L_cue;
        }
        if (attackKind == PICKUP_GLOVEZ || attackKind == PICKUP_NERFGUN
            || attackKind == PICKUP_WINGZ) {
            if (this->m_entranceReason == PICKUP_GRAVITYBOOTZ) {
                LK(s_BLOCKBODY2);
            } else {
                LK(s_IMPACTMM2);
            }
            goto L_cue;
        }
        if (this->m_entranceReason == PICKUP_GUNHAT) {
            if (attackKind == PICKUP_GAUNTLETZ || attackKind == PICKUP_SHOVEL
                || attackKind == PICKUP_SPRING || attackKind == PICKUP_CLUB) {
                LK(s_IMPACTMM4);
            } else {
                LK(s_IMPACTMM3);
            }
            goto L_cue;
        }
        if (this->m_entranceReason == PICKUP_SHIELD) {
            LK(s_BLOCKMETAL1);
            goto L_cue;
        }
        if (this->m_entranceReason == PICKUP_SPRING) {
            if (struckPose == 1) {
                LK(s_SPRING2);
            } else {
                LK(s_SPRING1);
            }
            goto L_cue;
        }
        if (this->m_entranceReason == PICKUP_TOOB && this->m_coordToggle != 0) {
            LK(s_TOOBZ);
            goto L_cue;
        }
        switch (attackKind) {
            case PICKUP_NONE:
                if (struckPose == 0) {
                    LK(s_BLOCKBODY2);
                } else {
                    LK(s_IMPACTMM1);
                }
                break;
            case PICKUP_BOOMERANG:
                LK(s_IMPACTMM1);
                break;
            case PICKUP_BRICK:
                if (struckPose == 0) {
                    LK(s_BLOCKBODY2);
                } else {
                    LK(s_IMPACTMM4);
                }
                break;
            case PICKUP_CLUB:
                if (struckPose == 0) {
                    LK(s_BLOCKBODY2);
                } else {
                    LK(s_IMPACTMM4);
                }
                break;
            case PICKUP_GAUNTLETZ:
                if (struckPose == 0) {
                    LK(s_BLOCKBODY2);
                } else {
                    LK(s_IMPACTMM3);
                }
                break;
            case PICKUP_GOOBER:
                if (struckPose == 0) {
                    LK(s_BLOCKBODY2);
                } else {
                    LK(s_IMPACTWM1);
                }
                break;
            case PICKUP_GRAVITYBOOTZ:
                if (struckPose == 0) {
                    LK(s_BLOCKBODY1);
                } else {
                    LK(s_IMPACTMM1);
                }
                break;
            case PICKUP_GUNHAT:
                LK(s_IMPACTWM2);
                break;
            case PICKUP_ROCK:
                LK(s_IMPACTMM2);
                break;
            case PICKUP_SHIELD:
                if (struckPose == 0) {
                    LK(s_BLOCKBODY1);
                } else {
                    LK(s_IMPACTMM4);
                }
                break;
            case PICKUP_SHOVEL:
                if (struckPose == 0) {
                    LK(s_BLOCKMETAL1);
                } else {
                    LK(s_IMPACTMM4);
                }
                break;
            case PICKUP_SPRING:
                if (struckPose == 0) {
                    LK(s_BLOCKBODY2);
                } else {
                    LK(s_IMPACTWM3);
                }
                break;
            case PICKUP_SPY:
                if (struckPose == 0) {
                    LK(s_BLOCKBODY2);
                } else {
                    LK(s_IMPACTMM1);
                }
                break;
            case PICKUP_SWORD:
                if (struckPose == 0) {
                    LK(s_BLOCKBODY2);
                } else {
                    LK(s_IMPACTMM3);
                }
                break;
            case PICKUP_TOOB:
                if (struckPose == 0) {
                    LK(s_BLOCKBODY2);
                } else {
                    LK(s_IMPACTMM1);
                }
                break;
            case PICKUP_WAND:
                if (struckPose == 0) {
                    LK(s_BLOCKBODY2);
                } else {
                    LK(s_IMPACTMM1);
                }
                break;
            case PICKUP_WARPSTONE:
                LK(s_IMPACTWM2);
                break;
            case PICKUP_WELDER:
                LK(s_IMPACTWM2);
                break;
            default:
                LK(s_IMPACTMM3);
                break;
        }

    L_cue:

        if (cue != NULL && g_sndEnabled != 0) {
            i32 clk = g_killCueClock;
            if (static_cast<u32>((clk - cue->m_lastPlayTime))
                >= static_cast<u32>(cue->m_replayDelay)) {
                cue->m_lastPlayTime = clk;
                cue->m_sound->ConfigureItem(g_sndCueTag, 0, 0, 0);
            }
        }
    }

    if (!(attackKind == PICKUP_GLOVEZ || attackKind == PICKUP_NERFGUN
          || attackKind == PICKUP_WINGZ)) {
        if (attackKind != PICKUP_WELDER) {
            return 1;
        }
        if (this->m_health > 0) {
            return 1;
        }
        m_tileMgr->CellDispatch(this->m_tileOwnerHi, this->m_tileOwnerLo, DEATH_BURN, srcRow);
        return 0;
    }

    if (this->m_entranceReason == PICKUP_GRAVITYBOOTZ) {
        return 1;
    }

    CString* typeRec = g_typeColl.ScratchResolve(this->m_objAux->m_actKey);
    if (g_typeColl.m_grown != 0) {
        CString* p = g_typeColl.Slots();
        i32 n = g_typeColl.m_grown;
        do {
            if (p != NULL) {
                new (p) CString();
            }
            p++;
        } while (--n != 0);
    }
    if (strcmp(*typeRec, s_typeO) == 0) {
        return 1;
    }

    i32 dy = srcPxY - this->m_object->m_screenY;
    i32 dx = srcPxX - this->m_object->m_screenX;
    i32 newX;
    i32 newY;
    if (attackKind == PICKUP_WINGZ) {
        switch (rand() % 8 - 1) {
            case 0:
                SETDIR(8, this->m_lastTilePx.m_x + 0x20, this->m_lastTilePx.m_y - 0x20);
                break;
            case 1:
                SETDIR(3, this->m_lastTilePx.m_x + 0x20, this->m_lastTilePx.m_y);
                break;
            case 2:
                SETDIR(5, this->m_lastTilePx.m_x + 0x20, this->m_lastTilePx.m_y + 0x20);
                break;
            case 3:
                SETDIR(1, this->m_lastTilePx.m_x, this->m_lastTilePx.m_y + 0x20);
                break;
            case 4:
                SETDIR(4, this->m_lastTilePx.m_x - 0x20, this->m_lastTilePx.m_y + 0x20);
                break;
            case 5:
                SETDIR(0, this->m_lastTilePx.m_x - 0x20, this->m_lastTilePx.m_y);
                break;
            case 6:
                SETDIR(6, this->m_lastTilePx.m_x - 0x20, this->m_lastTilePx.m_y - 0x20);
                break;
            default:
                SETDIR(2, this->m_lastTilePx.m_x, this->m_lastTilePx.m_y - 0x20);
                break;
        }
    } else if (dx == 0) {
        if (srcPxY > this->m_object->m_screenY) {
            SETDIR(2, this->m_lastTilePx.m_x, this->m_lastTilePx.m_y - 0x20);
        } else if (srcPxY < this->m_object->m_screenY) {
            SETDIR(1, this->m_lastTilePx.m_x, this->m_lastTilePx.m_y + 0x20);
        } else {
            goto L_moveDone;
        }
    } else {
        float slope = static_cast<float>(dy) / dx;
        if (slope > g_slopeTwo || slope < g_slopeNegTwo) {
            if (srcPxY > this->m_object->m_screenY) {
                SETDIR(2, this->m_lastTilePx.m_x, this->m_lastTilePx.m_y - 0x20);
            } else {
                SETDIR(1, this->m_lastTilePx.m_x, this->m_lastTilePx.m_y + 0x20);
            }
        } else if (slope > g_slopeHalf || slope < g_slopeZero) {
            if (slope > g_slopeHalf) {
                if (srcPxX > this->m_object->m_screenX) {
                    SETDIR(6, this->m_lastTilePx.m_x - 0x20, this->m_lastTilePx.m_y - 0x20);
                } else {
                    SETDIR(5, this->m_lastTilePx.m_x + 0x20, this->m_lastTilePx.m_y + 0x20);
                }
            } else if (slope < g_slopeZero) {
                if (srcPxX > this->m_object->m_screenX) {
                    SETDIR(4, this->m_lastTilePx.m_x - 0x20, this->m_lastTilePx.m_y + 0x20);
                } else {
                    SETDIR(8, this->m_lastTilePx.m_x + 0x20, this->m_lastTilePx.m_y - 0x20);
                }
            } else {
                goto L_moveDone;
            }
        } else {
            if (srcPxX > this->m_object->m_screenX) {
                SETDIR(0, this->m_lastTilePx.m_x - 0x20, this->m_lastTilePx.m_y);
            } else {
                SETDIR(3, this->m_lastTilePx.m_x + 0x20, this->m_lastTilePx.m_y);
            }
        }
    }

    {
        i32 flags = this->m_arrivalFlags | 0x20000000;
        CMapMgr* grid = static_cast<CMapMgr*>(g_gameReg->m_tileGrid);
        i32 nyt = newY >> TILE_SHIFT_PX;
        i32 nxt = newX >> TILE_SHIFT_PX;
        i32 oxt = this->m_lastTilePx.m_x >> TILE_SHIFT_PX;
        i32 oyt = this->m_lastTilePx.m_y >> TILE_SHIFT_PX;
        if (!(oxt == nxt && oyt == nyt)) {
            if (static_cast<u32>(nxt) >= static_cast<u32>(grid->m_width)) {
                return 1;
            }
            if (static_cast<u32>(nyt) >= static_cast<u32>(grid->m_height)) {
                return 1;
            }
            BrickzCell* cell = &grid->m_rows[nyt][nxt];
            i32 t = flags & cell->m_flags;
            if (t & 0x20000000) {
                return 1;
            }
            if (t != 0 && (cell->m_flags & (this->m_passableMask | 0x18000482)) == 0) {
                return 1;
            }
            BrickzCell* ocell = &grid->m_rows[oyt][oxt];
            i32 dxt = nxt - oxt;
            i32 dyt = nyt - oyt;
            if (dxt != 0 && dyt != 0) {
                i32 w = grid->m_width;
                if (dxt > 0) {
                    if (dyt > 0) {
                        if (((ocell + 1)->m_flags & 0x2000) || ((ocell + w)->m_flags & 0x2000)
                            || ((cell - 1)->m_flags & 0x2000) || ((cell - w)->m_flags & 0x2000)) {
                            return 1;
                        }
                    } else {
                        if (((ocell + 1)->m_flags & 0x2000) || ((ocell - w)->m_flags & 0x2000)
                            || ((cell - 1)->m_flags & 0x2000) || ((cell + w)->m_flags & 0x2000)) {
                            return 1;
                        }
                    }
                } else {
                    if (dyt > 0) {
                        if (((ocell - 1)->m_flags & 0x2000) || ((ocell + w)->m_flags & 0x2000)
                            || ((cell + 1)->m_flags & 0x2000) || ((cell - w)->m_flags & 0x2000)) {
                            return 1;
                        }
                    } else {
                        if (((ocell - 1)->m_flags & 0x2000) || ((ocell - w)->m_flags & 0x2000)
                            || ((cell + 1)->m_flags & 0x2000) || ((cell + w)->m_flags & 0x2000)) {
                            return 1;
                        }
                    }
                }
            }
        }

        if (this->m_arrivalPending == 0) {
            m_tileMgr->ApplySwitch(this, this->m_lastTilePx.m_x, this->m_lastTilePx.m_y);
        }
        CMapMgr* g2 = static_cast<CMapMgr*>(g_gameReg->m_tileGrid);
        i32 ox = this->m_lastTilePx.m_x >> TILE_SHIFT_PX;
        i32 oy = this->m_lastTilePx.m_y >> TILE_SHIFT_PX;
        g2->m_rows[oy][ox].m_flagBytes[3] &= 0xdf;
        g2->m_rows[oy][ox].m_occupantId = -1;
        g2->m_rows[nyt][nxt].m_flagBytes[3] |= 0x20;
        g2->m_rows[nyt][nxt].m_occupantId = (this->m_tileOwnerHi << 8) | this->m_tileOwnerLo;

        if (m_coordList.GetCount() != 0) {
            Coord* node = 0;
            i32 rx = this->m_lastTilePx.m_x >> TILE_SHIFT_PX;
            i32 ry = this->m_lastTilePx.m_y >> TILE_SHIFT_PX;
            if (g_coordPool.m_freeHead->m_next != NULL) {
                node = &g_coordPool.m_freeHead->m_coord;
                node->m_x = rx;
                node->m_y = ry;
                g_coordPool.m_freeHead = g_coordPool.m_freeHead->m_next;
            }
            m_coordList.AddHead(node);
        }

        this->m_lastTilePx.m_x = newX;
        this->m_lastTilePx.m_y = newY;
        this->m_prevAnimSetNode = this->m_objAux->m_actKey;
        this->m_objAux->m_actKey = ActFindId(s_typeO);
        double ddx = static_cast<double>(newX) - this->m_object->m_screenX;
        double ddy = static_cast<double>(newY) - this->m_object->m_screenY;
        double dist = sqrt(ddx * ddx + ddy * ddy);
        u32 kb = g_buteMgr.GetDwordDef(s_gruntSec, s_knockKey, 200);
        m_moveSpeed = dist / static_cast<double>(kb);
        m_movePosX = static_cast<double>((this->m_object->m_screenX));
        m_movePosY = static_cast<double>((this->m_object->m_screenY));

        if (m_coordList.GetCount() != 0) {
            POSITION pos = m_coordList.GetHeadPosition();
            if (pos != NULL) {
                do {
                    Coord* data = static_cast<Coord*>(m_coordList.GetNext(pos));
                    if (data != NULL) {

                        CoordPoolNode* slot = g_coordPool.NodeOf(data);
                        slot->m_next = g_coordPool.m_freeHead;
                        g_coordPool.m_freeHead = slot;
                    }
                } while (pos != NULL);
            }
            m_coordList.RemoveAll();
        }
        this->m_arrivalPending = 0;
    }

L_moveDone:
    return 1;
}

// @early-stop
RVA(0x0005b050, 0x40b)
i32 CGrunt::CommitNeighbor(i32 a, i32 b, i32 c, i32 d) {
    if (a == m_tileOwnerHi && g_traitorMode == 0) {
        return 0;
    }
    PickupType reason = m_entranceReason;
    if (reason == PICKUP_WARPSTONE || reason == PICKUP_WAND) {
        return 0;
    }
    {
        CGruntzMapMgr* bd = g_gameReg->m_tileGrid;
        i32 tx = m_lastTilePx.m_x >> TILE_SHIFT_PX;
        i32 ty = m_lastTilePx.m_y >> TILE_SHIFT_PX;
        i32 flags;
        if (static_cast<u32>(tx) >= static_cast<u32>(bd->m_width)
            || static_cast<u32>(ty) >= static_cast<u32>(bd->m_height)) {
            flags = 1;
        } else {
            flags = bd->m_rowInts[ty][tx * 7];
        }
        if (flags & 0x80) {
            return 0;
        }
    }

    CreateHealthSprite();
    m_combatTimeoutLo = static_cast<i32>(g_buteMgr.GetDwordDef(s_Grunt, s_CombatTimeout, 0x1388));
    m_combatTimeoutHi = 0;
    m_combatClockLo = static_cast<i32>(g_frameTime);
    m_combatClockHi = 0;
    m_neighborScanEnabled = 1;

    CGrunt* nb = m_tileMgr->m_grid[a * TM_GRID_COLS + b];
    if (nb == NULL || nb->m_entranceCommitted == 0 || m_entranceCommitted == 0) {
        return 0;
    }

    bool eq;
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), s_codeF) == 0);
    if (eq) {
        return 0;
    }

    i32 flag = 0;
    PickupType v = m_entranceReason;
    if (v > PICKUP_EQUIPPABLE_LAST) {
        v = m_toolId;
    }
    if (v == PICKUP_BOMB) {
        flag = IDX(v);
    }
    if (flag != 0) {
        RunMoveConfig(c >> TILE_SHIFT_PX, d >> TILE_SHIFT_PX);
        return 1;
    }

    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), "I") == 0);
    if (eq) {
        m_tileMgr->LoadTileArrivalFx(
            m_tileOwnerHi,
            m_tileOwnerLo,
            m_moveTile.m_x,
            m_moveTile.m_y,
            m_entranceReason,
            -1
        );
    } else {
        eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), s_codeN) == 0);
        if (eq) {
            i32 lastX = m_lastTilePx.m_x;
            i32 lastY = m_lastTilePx.m_y;
            i32 px = (m_object->m_screenX & ~TILE_MASK_PX) + TILE_HALF_PX;
            i32 py = (m_object->m_screenY & ~TILE_MASK_PX) + TILE_HALF_PX;
            i32 redo = 1;
            if (px != lastX || py != lastY) {
                if (IsDropReady(1)) {
                    m_coordToggle = (m_coordToggle == 0);
                    redo = 0;
                }
            }
            SnapToLastTile(1);
            if (redo) {
                m_prevAnimSetNode = m_objAux->m_actKey;
                m_objAux->m_actKey = ActFindId(s_codeD);
                SetupTubeAnim(m_coordToggle);
            }
        }
    }

    if (m_arrivalPending != 0) {
        m_tileMgr->WireTileSwitchLogic(this, m_object->m_screenX, m_object->m_screenY);
        m_arrivalPending = 0;
    }
    m_poweredUp = 1;
    nb->CreateHealthSprite();
    nb->m_combatTimeoutLo =
        static_cast<i32>(g_buteMgr.GetDwordDef(s_Grunt, s_CombatTimeout, 0x1388));
    nb->m_combatTimeoutHi = 0;
    nb->m_combatClockLo = static_cast<i32>(g_frameTime);
    nb->m_combatClockHi = 0;
    ArrivalRecycle(c, d, 1, a, b);
    m_neighborCell.m_x = a;
    m_neighborCell.m_y = b;
    m_attackTargetPx.m_x = c;
    m_attackTargetPx.m_y = d;
    if (m_stamina < STAMINA_FULL || m_entranceActive != 0) {
        m_neighborValid = 1;
        return 1;
    }
    m_neighborValid = 0;
    nb->ArrivalRecycle(m_object->m_screenX, m_object->m_screenY, 0, m_tileOwnerHi, m_tileOwnerLo);
    RearmAttackAnim(a, b);
    return 1;
}

// @early-stop
RVA(0x0005b570, 0x12b)
i32 CGrunt::BeginAttack(i32 a, i32 b) {
    if (m_entranceCommitted == 0) {
        goto fail;
    }
    {

        CString* rec = g_typeColl.ScratchResolve(m_objAux->m_actKey);
        GruntScratchTeardown();
        bool eq = (strcmp(*rec, s_codeF) == 0);
        if (eq) {
            goto fail;
        }
    }
    if (m_stamina < STAMINA_FULL) {
        goto fail;
    }

    PlayMoveSound(a, b);
    m_poweredUp = 1;
    m_combatActive = 1;
    CreateHealthSprite();

    m_combatTimeoutLo = static_cast<i32>(g_buteMgr.GetDwordDef(s_Grunt, s_CombatTimeout, 0x1388));
    m_combatTimeoutHi = 0;
    m_combatClockLo = static_cast<i32>(g_frameTime);
    m_combatClockHi = 0;
    m_neighborScanEnabled = 1;
    m_attackTargetPx.m_x = a;
    m_attackTargetPx.m_y = b;
    RearmAttackAnim2();
    return 1;
fail:
    return 0;
}

RVA(0x0005b6f0, 0xb5)
CGrunt* CGrunt::FindGridNeighbor(i32 validate) {
    if (m_neighborCell.m_x == -1) {
        return 0;
    }
    if (m_neighborCell.m_y == -1) {
        return 0;
    }

    CGrunt* n = m_tileMgr->m_grid[m_neighborCell.m_x * TM_GRID_COLS + m_neighborCell.m_y];
    if (n != NULL && n->m_entranceCommitted != 0) {
        if (validate != 0) {
            if (n->m_object->m_screenX != n->m_lastTilePx.m_x) {
                return 0;
            }
            if (n->m_object->m_screenY != n->m_lastTilePx.m_y) {
                return 0;
            }
        }
        if (RectContains(n->m_object->m_screenX, n->m_object->m_screenY)) {
            CommitNeighbor(
                m_neighborCell.m_x,
                m_neighborCell.m_y,
                n->m_object->m_screenX,
                n->m_object->m_screenY
            );
            return n;
        }
    }

    m_neighborValid = 0;
    return 0;
}

RVA(0x0005b7e0, 0x23)
CObject* CDDrawSubMgrLeafScan::Lookup(const char* key) {
    void* val = 0;
    m_cues.Lookup(key, val);
    return static_cast<CObject*>(val);
}

RVA(0x0005baf0, 0xf4)
i32 CreateGrunt(CGameObject* owner) {
    AnimWorkerObj* rec = owner->m_animWorker;
    switch (static_cast<u32>(rec->ActKey())) {
        case ACT_UNINITIALISED: {
            rec->SetActKey(ACT_LIVE);
            CUserLogic* sub = new CGrunt(owner);
            sub->Activate();
            rec->m_logic = sub;
            break;
        }
        case ACT_OBJECT_REMOVED:
            rec->m_logic->OnObjectRemoved();
            break;
        case ACT_LEAVE_ACTIVE_REGION:
            rec->m_logic->OnLeaveActiveRegion();
            break;
        case ACT_PREPARE_SAVE:
            rec->m_logic->PrepareSave();
            break;
        case ACT_AFTER_LOAD_REFERENCES:
            rec->m_logic->AfterLoadReferences();
            break;
        case ACT_AFTER_LOAD:
            rec->m_logic->AfterLoad();
            break;
        case ACT_AFTER_SAVE:
            rec->m_logic->AfterSave();
            break;
        case ACT_LIVE:
            break;
        default:
            Worker_DefaultPump(rec->m_logic);
            break;
    }
    return 1;
}

RVA(0x0005bcd0, 0x102)
void CGrunt::FireActivation(i32 id) {
    CActHandler* e = CActRegPool<CGrunt>::s_table.ResolveEntry(id);
    if (*e != 0) {
        (this->*(*CActRegPool<CGrunt>::s_table.ResolveEntry(id)))();
    }
}

// @early-stop
// Only the last (DERIVED) entry differs: retail pushes the operator= argument
// before evaluating SlotOf(); cl emits the object expression first. 3 bytes.
RVA(0x0005be30, 0x9e5)
void RegisterGruntActions() {
    REGISTER_KEY_644AF0("A", &CGrunt::ResolveEntranceArrival);
    REGISTER_KEY_644AF0("B", &CGrunt::StepWarpExit);
    REGISTER_KEY_644AF0("C", &CGrunt::LoadGruntDecayConfig);
    REGISTER_KEY_644AF0(s_codeD, &CGrunt::StepArrivalReroll);
    REGISTER_KEY_644AF0("E", &CGrunt::UpdateGruntStatus);
    REGISTER_KEY_644AF0(s_codeF, &CGrunt::StepAttackAction);
    REGISTER_KEY_644AF0("G", &CGrunt::StepEntranceRelatchA);
    REGISTER_KEY_644AF0(s_codeH, &CGrunt::StepArrivalCommitA);
    REGISTER_KEY_644AF0("I", &CGrunt::LoadWandGruntItemConfig);
    REGISTER_KEY_644AF0("J", &CGrunt::RunEntranceMove);
    REGISTER_KEY_644AF0(s_codeK, &CGrunt::LoadEntranceConfig);
    REGISTER_KEY_644AF0("L", &CGrunt::LoadVehicleGruntAnimations);
    REGISTER_KEY_644AF0(s_codeM, &CGrunt::RearmEntranceDrop);
    REGISTER_KEY_644AF0(s_codeN, &CGrunt::StepEntranceRelatchB);
    REGISTER_KEY_644AF0(s_codeO, &CGrunt::StepArrivalCommitB);
    REGISTER_KEY_644AF0("P", &CGrunt::UpdateEntranceAnim);
    REGISTER_KEY_644AF0(s_codeQ, &CGrunt::LoadFreezeSpellAssets);
    REGISTER_KEY_644AF0("R", &CGrunt::LoadGruntDecayConfig2);
    REGISTER_KEY_644AF0_DERIVED(k_60df94, &CGrunt::FinishEntranceMove);
}

RVA(0x0005caa0, 0x5e4)
void CGrunt::Activate() {
    double diag = sqrt(2.0);

    double s = 1.0 / diag;

    m_cells[3 * g_gruntDirNorth.row + g_gruntDirNorth.column].m_motion.m_direction.x = 0.0;
    m_cells[3 * g_gruntDirNorth.row + g_gruntDirNorth.column].m_motion.m_direction.y = -1.0;
    m_cells[3 * g_gruntDirNorth.row + g_gruntDirNorth.column].m_motion.m_step.x = 0.0;
    m_cells[3 * g_gruntDirNorth.row + g_gruntDirNorth.column].m_motion.m_step.y = -0.5;

    m_cells[3 * g_gruntDirNorthEast.row + g_gruntDirNorthEast.column].m_motion.m_direction.x = s;
    m_cells[3 * g_gruntDirNorthEast.row + g_gruntDirNorthEast.column].m_motion.m_direction.y =
        -1.0 / diag;
    m_cells[3 * g_gruntDirNorthEast.row + g_gruntDirNorthEast.column].m_motion.m_step.x = 0.5;
    m_cells[3 * g_gruntDirNorthEast.row + g_gruntDirNorthEast.column].m_motion.m_step.y = -0.5;

    m_cells[3 * g_gruntDirEast.row + g_gruntDirEast.column].m_motion.m_direction.x = 1.0;
    m_cells[3 * g_gruntDirEast.row + g_gruntDirEast.column].m_motion.m_direction.y = 0.0;
    m_cells[3 * g_gruntDirEast.row + g_gruntDirEast.column].m_motion.m_step.x = 0.5;
    m_cells[3 * g_gruntDirEast.row + g_gruntDirEast.column].m_motion.m_step.y = 0.0;

    m_cells[3 * g_gruntDirSouthEast.row + g_gruntDirSouthEast.column].m_motion.m_direction.x = s;
    m_cells[3 * g_gruntDirSouthEast.row + g_gruntDirSouthEast.column].m_motion.m_direction.y = s;
    m_cells[3 * g_gruntDirSouthEast.row + g_gruntDirSouthEast.column].m_motion.m_step.x = 0.5;
    m_cells[3 * g_gruntDirSouthEast.row + g_gruntDirSouthEast.column].m_motion.m_step.y = 0.5;

    m_cells[3 * g_gruntDirSouth.row + g_gruntDirSouth.column].m_motion.m_direction.x = 0.0;
    m_cells[3 * g_gruntDirSouth.row + g_gruntDirSouth.column].m_motion.m_direction.y = 1.0;
    m_cells[3 * g_gruntDirSouth.row + g_gruntDirSouth.column].m_motion.m_step.x = 0.0;
    m_cells[3 * g_gruntDirSouth.row + g_gruntDirSouth.column].m_motion.m_step.y = 0.5;

    m_cells[3 * g_gruntDirSouthWest.row + g_gruntDirSouthWest.column].m_motion.m_direction.x =
        -1.0 / diag;
    m_cells[3 * g_gruntDirSouthWest.row + g_gruntDirSouthWest.column].m_motion.m_direction.y = s;
    m_cells[3 * g_gruntDirSouthWest.row + g_gruntDirSouthWest.column].m_motion.m_step.x = -0.5;
    m_cells[3 * g_gruntDirSouthWest.row + g_gruntDirSouthWest.column].m_motion.m_step.y = 0.5;

    m_cells[3 * g_gruntDirWest.row + g_gruntDirWest.column].m_motion.m_direction.x = -1.0;
    m_cells[3 * g_gruntDirWest.row + g_gruntDirWest.column].m_motion.m_direction.y = 0.0;
    m_cells[3 * g_gruntDirWest.row + g_gruntDirWest.column].m_motion.m_step.x = -0.5;
    m_cells[3 * g_gruntDirWest.row + g_gruntDirWest.column].m_motion.m_step.y = 0.0;

    m_cells[3 * g_gruntDirNorthWest.row + g_gruntDirNorthWest.column].m_motion.m_direction.x =
        -1.0 / diag;
    m_cells[3 * g_gruntDirNorthWest.row + g_gruntDirNorthWest.column].m_motion.m_direction.y =
        -1.0 / diag;
    m_cells[3 * g_gruntDirNorthWest.row + g_gruntDirNorthWest.column].m_motion.m_step.x = -0.5;
    m_cells[3 * g_gruntDirNorthWest.row + g_gruntDirNorthWest.column].m_motion.m_step.y = -0.5;

    m_cells[3 * g_gruntDirCenter.row + g_gruntDirCenter.column].m_motion.m_direction.x = 0.0;
    m_cells[3 * g_gruntDirCenter.row + g_gruntDirCenter.column].m_motion.m_direction.y = 0.0;
    m_cells[3 * g_gruntDirCenter.row + g_gruntDirCenter.column].m_motion.m_step.x = 0.0;
    m_cells[3 * g_gruntDirCenter.row + g_gruntDirCenter.column].m_motion.m_step.y = 0.0;

    CWwdGameObjectA* h = m_object;
    i32 px = h->m_screenX;
    m_commitPx.m_x = px;
    m_lastTilePx.m_x = px;
    m_entrancePx.m_x = px;
    i32 py = h->m_screenY;
    m_commitPx.m_y = py;
    m_lastTilePx.m_y = py;
    m_entrancePx.m_y = py;
    m_reserved1dc.m_x = 0;
    m_reserved1dc.m_y = 0;
    m_health = HEALTH_FULL;
    m_stamina = STAMINA_FULL;
    m_toyTime = 0;
    m_wingzTime = 0;
    m_entranceActive = 0;
    m_arrivalPending = 0;
    m_arrivalState = 0;
    m_poweredUp = 0;
    m_resetApplied = 0;
    m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
    m_passableMask = 0;
    m_deathAnimStarted = 0;
    m_tileClaimed = 0;
}

#undef REGISTER_KEY_644AF0

DATA(0x001e999c)
const float g_quarterScale = 0.25f;

DATA(0x001e99a0)
const float g_slopeTwo = 2.0f;
DATA(0x001e99a4)
const float g_slopeNegTwo = -2.0f;
DATA(0x001e99a8)
const double g_slopeHalf = 0.5;
DATA(0x001e99b0)
const double g_slopeZero = 0.0;
