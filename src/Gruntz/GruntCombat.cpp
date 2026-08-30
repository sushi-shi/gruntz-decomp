#include <rva.h>

#include <Gruntz/GruntCombat.h>

#include <Mfc.h>
#include <MfcNoInline.h>
#include <MfcWin.h>

#include <Bute/ButeMgr.h>
#include <Bute/ButeTree.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/LogicRecordRegistryFindInline.h>
#include <Dsndmgr/SoundBuffer.h>
#include <Enums.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniElement.h>
#include <Gruntz/AnimationRegistry.h>
#include <Gruntz/ArrivalFlagsPreset.h>
#include <Gruntz/BattlezMapConfig.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/EnemyAiType.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameModeId.h>
#include <Gruntz/GameObjectLogicTypes.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntAiState.h>
#include <Gruntz/GruntCombatClockInline.h>
#include <Gruntz/GruntCombatDirection.h>
#include <Gruntz/GruntCoordRecycleMacros.h>
#include <Gruntz/GruntDeathType.h>
#include <Gruntz/GruntDirection.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntEntranceArrival.h>
#include <Gruntz/GruntEntranceMove.h>
#include <Gruntz/GruntIdentity.h>
#include <Gruntz/GruntMovementMacros.h>
#include <Gruntz/GruntPoweredStateMacros.h>
#include <Gruntz/GruntSpriteMacros.h>
#include <Gruntz/GruntzMapMgr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/HealthPct.h>
#include <Gruntz/InGameIcon.h>
#include <Gruntz/LightFx.h>
#include <Gruntz/LogicRecordHandler.h>
#include <Gruntz/LogicTypeTableInline.h>
#include <Gruntz/MapCellFlags.h>
#include <Gruntz/MotionState.h>
#include <Gruntz/MovingLogicSerial.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/Play.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialRecords.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SortKeyMacros.h>
#include <Gruntz/SoundCue.h>
#include <Gruntz/SoundCueRegistry.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/SpellzEffect.h>
#include <Gruntz/StaminaPct.h>
#include <Gruntz/TileCoordMacros.h>
#include <Gruntz/TraitorMode.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/VoiceManager.h>
#include <Ints.h>
#include <Io/FileMem.h>
#include <Lith/BDefs.h>
#include <Rez/FrameClock.h>
#include <Utils/MapTyped.h>
#include <Wap32/Object.h>
#include <Wap32/TileGeometry.h>
#include <Wwd/LogicRecordEvent.h>
#include <Wwd/WwdGameObjectFamily.h>
#include <Wwd/WwdSpriteAnimationInline.h>

#include <math.h>
#include <new>
#include <stdlib.h>
#include <string.h>

RVA_DYNINIT(0x00058f60, 0x5, s_gruntDirNorth)
RVA_DYNINIT(0x00058f80, 0x1a, s_gruntDirNorth)
RVA_DYNINIT(0x00058fb0, 0x5, s_gruntDirNorthEast)
RVA_DYNINIT(0x00058fd0, 0x1a, s_gruntDirNorthEast)
RVA_DYNINIT(0x00059000, 0x5, s_gruntDirEast)
RVA_DYNINIT(0x00059020, 0x1f, s_gruntDirEast)
RVA_DYNINIT(0x00059050, 0x5, s_gruntDirSouthEast)
RVA_DYNINIT(0x00059070, 0x1a, s_gruntDirSouthEast)
RVA_DYNINIT(0x000590a0, 0x5, s_gruntDirSouth)
RVA_DYNINIT(0x000590c0, 0x1f, s_gruntDirSouth)
RVA_DYNINIT(0x000590f0, 0x5, s_gruntDirSouthWest)
RVA_DYNINIT(0x00059110, 0x1f, s_gruntDirSouthWest)
RVA_DYNINIT(0x00059140, 0x5, s_gruntDirWest)
RVA_DYNINIT(0x00059160, 0x1f, s_gruntDirWest)
RVA_DYNINIT(0x00059190, 0x5, s_gruntDirNorthWest)
RVA_DYNINIT(0x000591b0, 0x17, s_gruntDirNorthWest)
RVA_DYNINIT(0x000591e0, 0x5, s_gruntDirCenter)
RVA_DYNINIT(0x00059200, 0x1a, s_gruntDirCenter)

DATA(0x0020dc64)
static char s_RollingBallzTime[] = "RollingBallzTime";
DATA(0x0020dc78)
static char s_RollingBallzSpeed[] = "RollingBallzSpeed";
DATA(0x0020dcac)
static char s_TeleportRadius[] = "TeleportRadius";
DATA(0x0020dcc0)
static char s_ToyzRadius[] = "ToyzRadius";
DATA(0x0020dcd0)
static char s_RessurectionRadius[] = "RessurectionRadius";
DATA(0x0020dce8)
static char s_HealthRadius[] = "HealthRadius";
DATA(0x0020dcf8)
static char s_FreezeRadius[] = "FreezeRadius";
DATA(0x0020dd30)
static char s_GAME_ATTACK[] = "GAME_ATTACK";
DATA(0x0020dd40)
static char s__LOSEITEM[] = "_LOSEITEM";
DATA(0x0020dd4c)
static char s_knockKey[] = "KnockBackTimePerTile";
DATA(0x0020dd68)
static char s_IMPACTWM3[] = "GRUNTZ_NORMALGRUNT_IMPACTWM3";
DATA(0x0020dd8c)
static char s_IMPACTMM2[] = "GRUNTZ_NORMALGRUNT_IMPACTMM2";
DATA(0x0020ddb0)
static char s_IMPACTWM2[] = "GRUNTZ_NORMALGRUNT_IMPACTWM2";
DATA(0x0020ddd4)
static char s_BLOCKBODY1[] = "GRUNTZ_NORMALGRUNT_BLOCKBODY1";
DATA(0x0020ddf8)
static char s_IMPACTWM1[] = "GRUNTZ_NORMALGRUNT_IMPACTWM1";
DATA(0x0020de1c)
static char s_IMPACTMM1[] = "GRUNTZ_NORMALGRUNT_IMPACTMM1";
DATA(0x0020de40)
static char s_BLOCKBODY2[] = "GRUNTZ_NORMALGRUNT_BLOCKBODY2";
DATA(0x0020de64)
static char s_TOOBZ[] = "GRUNTZ_TOOBGRUNT_TOOBZGRUNTUI1B";
DATA(0x0020de8c)
static char s_SPRING1[] = "GRUNTZ_SPRINGGRUNT_SPRINGGRUNTS1S1";
DATA(0x0020deb8)
static char s_SPRING2[] = "GRUNTZ_SPRINGGRUNT_SPRINGGRUNTS2S1";
DATA(0x0020dee4)
static char s_BLOCKMETAL1[] = "GRUNTZ_NORMALGRUNT_BLOCKMETAL1";
DATA(0x0020df0c)
static char s_IMPACTMM4[] = "GRUNTZ_NORMALGRUNT_IMPACTMM4";
DATA(0x0020df54)
static char s_DEATHTOUCHHIT[] = "GAME_DEATHTOUCHHIT";
DATA(0x0020df6c)
static char s_CONVERSIONHIT[] = "GAME_CONVERSIONHIT";
DATA(0x0020dfac)
static char s_AccelerateFlash[] = "AccelerateFlash";
DATA(0x0020dfc0)
static char s_SafeFlashTime[] = "SafeFlashTime";
DATA(0x0020dfd0)
static char s_FadeTransparency[] = "FadeTransparency";

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

RVA_DYNINIT(0x0005b8c0, 0x5, g_gruntDirEast)
RVA_DYNINIT(0x0005b8e0, 0x1f, g_gruntDirEast)
DATA(0x00244aa0)
GruntDirectionCell g_gruntDirEast = GruntDirectionCell(1, 2, DIR_EAST);
RVA_DYNINIT(0x0005b820, 0x5, g_gruntDirNorth)
RVA_DYNINIT(0x0005b840, 0x1a, g_gruntDirNorth)
DATA(0x00244ab0)
GruntDirectionCell g_gruntDirNorth = GruntDirectionCell(0, 1, DIR_NORTH);
RVA_DYNINIT(0x0005b960, 0x5, g_gruntDirSouth)
RVA_DYNINIT(0x0005b980, 0x1f, g_gruntDirSouth)
DATA(0x00244ac0)
GruntDirectionCell g_gruntDirSouth = GruntDirectionCell(2, 1, DIR_SOUTH);
RVA_DYNINIT(0x0005ba00, 0x5, g_gruntDirWest)
RVA_DYNINIT(0x0005ba20, 0x1f, g_gruntDirWest)
DATA(0x00244ad0)
GruntDirectionCell g_gruntDirWest = GruntDirectionCell(1, 0, DIR_WEST);
RVA_DYNINIT(0x0005b870, 0x5, g_gruntDirNorthEast)
RVA_DYNINIT(0x0005b890, 0x1a, g_gruntDirNorthEast)
DATA(0x00244ae0)
GruntDirectionCell g_gruntDirNorthEast = GruntDirectionCell(0, 2, DIR_NORTHEAST);
RVA_DYNINIT(0x0005ba50, 0x5, g_gruntDirNorthWest)
RVA_DYNINIT(0x0005ba70, 0x17, g_gruntDirNorthWest)
DATA(0x00244b18)
GruntDirectionCell g_gruntDirNorthWest = GruntDirectionCell(0, 0, DIR_NORTHWEST);
RVA_DYNINIT(0x0005b910, 0x5, g_gruntDirSouthEast)
RVA_DYNINIT(0x0005b930, 0x1a, g_gruntDirSouthEast)
DATA(0x00244b28)
GruntDirectionCell g_gruntDirSouthEast = GruntDirectionCell(2, 2, DIR_SOUTHEAST);
RVA_DYNINIT(0x0005baa0, 0x5, g_gruntDirCenter)
RVA_DYNINIT(0x0005bac0, 0x1a, g_gruntDirCenter)
DATA(0x00244b38)
GruntDirectionCell g_gruntDirCenter = GruntDirectionCell(1, 1, DIR_CENTER);
RVA_DYNINIT(0x0005b9b0, 0x5, g_gruntDirSouthWest)
RVA_DYNINIT(0x0005b9d0, 0x1f, g_gruntDirSouthWest)
DATA(0x00244b48)
GruntDirectionCell g_gruntDirSouthWest = GruntDirectionCell(2, 0, DIR_SOUTHWEST);

RVA_DYNINIT(0x0005bc30, 0xa, CActRegPool<CGrunt>::s_table)
RVA_DYNINIT(0x0005bc50, 0x15, CActRegPool<CGrunt>::s_table)
RVA_DYNINIT(0x0005bc80, 0xe, CActRegPool<CGrunt>::s_table)
RVA_DYNINIT(0x0005bca0, 0x1f, CActRegPool<CGrunt>::s_table)
template<> DATA(0x00244af0)
CActReg CActRegPool<CGrunt>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

#define LK(key)                                                                                    \
    do {                                                                                           \
        SoundCue* out = NULL;                                                                      \
        MapLookup(reg->m_world->m_soundRegistry->m_cues, (key), out);                              \
        cue = out;                                                                                 \
    } while (0)

#define SETDIR(cell, nx, ny)                                                                       \
    do {                                                                                           \
        newPos.m_y = (ny);                                                                         \
        newPos.m_x = (nx);                                                                         \
        this->m_entranceCell = (cell);                                                             \
    } while (0)

// @early-stop
RVA(0x00056f80, 0xb0)
i32* CGrunt::EntranceTileOffset(i32* out) {
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
    return out;
}

#define SCAN_BOUNDS(grid)                                                                          \
    {                                                                                              \
        CRect rb(0, 0, (grid)->m_width, (grid)->m_height);                                         \
        RECT ra;                                                                                   \
        ra = CRect(0, 0, (grid)->m_width, (grid)->m_height);                                       \
        RECT* rd = &(grid)->m_bounds;                                                              \
        if (!IntersectRect(rd, &ra, &rb)) {                                                        \
            *rd = ra;                                                                              \
        }                                                                                          \
        (grid)->m_gridW = rd->right - rd->left;                                                    \
        (grid)->m_gridH = rd->bottom - rd->top;                                                    \
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
        RECT* rd = &(grid)->m_bounds;                                                              \
        if (!IntersectRect(rd, &ra, &rb)) {                                                        \
            *rd = ra;                                                                              \
        }                                                                                          \
        (grid)->m_gridW = rd->right - rd->left;                                                    \
        (grid)->m_gridH = rd->bottom - rd->top;                                                    \
    }

#define FREELIST_PUSH(elem)                                                                        \
    {                                                                                              \
        CoordPoolNode* node = g_coordPool.NodeOf((elem));                                          \
        node->m_next = g_coordPool.m_freeHead;                                                     \
        g_coordPool.m_freeHead = node;                                                             \
    }

RVA(0x00057060, 0x72)
void CGrunt::ComputeFacing(double dt) {
    CWwdSpriteObject* h = m_object;
    double dx = static_cast<double>(m_lastTilePx.m_x) - static_cast<double>(h->m_screenX);
    double dy = static_cast<double>(m_lastTilePx.m_y) - static_cast<double>(h->m_screenY);

    m_moveSpeed = (sqrt(SQR(dx) + SQR(dy)) / static_cast<double>(m_timePerTile)) * dt;
    m_movePosX = static_cast<double>(h->m_screenX);
    m_movePosY = static_cast<double>(h->m_screenY);
}

#define BIND_ACT_644AF0_RAW(id, handler)                                                           \
    {                                                                                              \
        GruntActPmf _p;                                                                            \
        _p.m_pmf = (handler);                                                                      \
        /* The stored generic CUserLogic PMF is reached through retail's raw */                    \
        /* _zvec accessor; the typed view exists only at this ABI seam. */                         \
        *CActReg::AsElem(CActRegPool<CGrunt>::s_table._zvec::IndexToPtr(id)) = _p.m_h;             \
    }

#define BIND_ACT_644AF0_TYPED(id, handler)                                                         \
    {                                                                                              \
        GruntActPmf _p;                                                                            \
        _p.m_pmf = (handler);                                                                      \
        *CActRegPool<CGrunt>::s_table.Resolve(id) = _p.m_h;                                        \
    }

#define REGISTER_KEY_644AF0_IMPL(key, handler, bind)                                               \
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
                    list->CString::CString();                                                      \
                }                                                                                  \
                list++;                                                                            \
            }                                                                                      \
            *slot = (key);                                                                         \
            g_typeCounter++;                                                                       \
        }                                                                                          \
        bind(id, handler);                                                                         \
    }

#define REGISTER_KEY_644AF0(key, handler)                                                          \
    REGISTER_KEY_644AF0_IMPL(key, handler, BIND_ACT_644AF0_RAW)

#define REGISTER_KEY_644AF0_TYPED(key, handler)                                                    \
    REGISTER_KEY_644AF0_IMPL(key, handler, BIND_ACT_644AF0_TYPED)

#define REGISTER_KEY_644AF0_DERIVED(key, handler)                                                  \
    {                                                                                              \
        i32 id = ActFindId(key);                                                                   \
        if (id == 0) {                                                                             \
            ActInsertId(key, g_typeCounter);                                                       \
            id = g_typeCounter;                                                                    \
            *g_typeColl.SlotOf(id) = (key);                                                        \
            g_typeCounter++;                                                                       \
        }                                                                                          \
        BIND_ACT_644AF0_TYPED(id, handler);                                                        \
    }

RVA(0x00057100, 0x590)
i32 CGrunt::LoadGruntAbilityTuning(i32 forced) {
    SpellzEffect idx = static_cast<SpellzEffect>(forced);
    if (forced == 0) {
        i32 m = 3;
        if (g_gameReg->m_gameMode != GAMEMODE_QUESTZ) {
            m = 6;
        }
        if (m == 0) {
            i32 coin = static_cast<char>(rand());
            idx = static_cast<SpellzEffect>(coin & 1);
        } else {
            idx = static_cast<SpellzEffect>(rand() % m + 1);
        }
    }

    SoundCueRegistry* slot =
        (static_cast<CDDrawSurfaceMgr*>(m_ownerLogicRecord->m_ownerCtx))->m_soundRegistry;
    if (slot->m_silentMode == false) {
        SoundCue* sout = NULL;
        MapLookup(slot->m_cues, s_GAME_ATTACK, sout);
        if (sout != NULL) {

            sout->PlayIfElapsed(g_soundVolumePercent, 0, 0, false);
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
                WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
            );
            spr->m_logicRecord->m_dispatch(spr);
            (static_cast<CLightFx*>(spr->m_logicRecord->m_userLogic))
                ->Activate("GAME_LIGHTING_FLASH", "GAME_FLASH", 9, true);
            return m_triggerMgr->ApplyGruntAreaEffect(
                m_lastTilePx.m_x,
                m_lastTilePx.m_y,
                g_buteMgr.GetInt("Spellz", s_FreezeRadius, 8),
                GRUNT_AREA_EFFECT_FREEZE,
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
                WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
            );
            spr->m_logicRecord->m_dispatch(spr);
            (static_cast<CLightFx*>(spr->m_logicRecord->m_userLogic))
                ->Activate("GAME_LIGHTING_FLASH", "GAME_FLASH", 2, true);
            return m_triggerMgr->ApplyGruntAreaEffect(
                m_lastTilePx.m_x,
                m_lastTilePx.m_y,
                g_buteMgr.GetInt("Spellz", s_HealthRadius, 8),
                GRUNT_AREA_EFFECT_HEAL,
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
                WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
            );
            spr->m_logicRecord->m_dispatch(spr);
            (static_cast<CLightFx*>(spr->m_logicRecord->m_userLogic))
                ->Activate("GAME_LIGHTING_FLASH", "GAME_FLASH", 8, true);
            return m_triggerMgr->LoadGruntResurrectTuning(
                m_lastTilePx.m_x,
                m_lastTilePx.m_y,
                g_buteMgr.GetInt("Spellz", s_RessurectionRadius, 8)
            );
        }
        case SPELLZ_TOYZ: {
            CGameObject* spr = g_gameReg->m_world->m_childGroup->CreateSprite(
                0,
                m_lastTilePx.m_x,
                m_lastTilePx.m_y,
                SORTKEY_OVERLAY,
                "LightFx",
                WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
            );
            spr->m_logicRecord->m_dispatch(spr);
            (static_cast<CLightFx*>(spr->m_logicRecord->m_userLogic))
                ->Activate("GAME_LIGHTING_FLASH", "GAME_FLASH", 7, true);
            return m_triggerMgr->ApplyGruntAreaEffect(
                m_lastTilePx.m_x,
                m_lastTilePx.m_y,
                g_buteMgr.GetInt("Spellz", s_ToyzRadius, 8),
                GRUNT_AREA_EFFECT_GIVE_TOY,
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
                WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
            );
            spr->m_logicRecord->m_dispatch(spr);
            (static_cast<CLightFx*>(spr->m_logicRecord->m_userLogic))
                ->Activate("GAME_LIGHTING_FLASH", "GAME_FLASH", 3, true);
            return m_triggerMgr->ApplyGruntAreaEffect(
                m_lastTilePx.m_x,
                m_lastTilePx.m_y,
                g_buteMgr.GetInt("Spellz", s_TeleportRadius, 8),
                GRUNT_AREA_EFFECT_TELEPORT,
                -1
            );
        }
        case SPELLZ_ROLLINGBALL: {
            CGameObject* spr = g_gameReg->m_world->m_childGroup->CreateSprite(
                0,
                m_lastTilePx.m_x,
                m_lastTilePx.m_y,
                SORTKEY_OVERLAY,
                "LightFx",
                WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
            );
            spr->m_logicRecord->m_dispatch(spr);
            (static_cast<CLightFx*>(spr->m_logicRecord->m_userLogic))
                ->Activate("GAME_LIGHTING_FLASH", "GAME_FLASH", 1, true);

            CWwdSpriteObject* n = g_gameReg->m_world->m_childGroup->CreateSprite(
                0,
                m_lastTilePx.m_x,
                m_lastTilePx.m_y - 0x20,
                0,
                "RollingBall",
                WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
            );
            n->SetImageSetByName("LEVEL_ROLLINGBALL_NORTH");
            CLogicRecord* ni = n->m_logicRecord;
            ni->m_speed =
                static_cast<i32>(g_buteMgr.GetDword("Spellz", s_RollingBallzSpeed, 0x3e8));
            n->m_smarts = 0;
            n->m_points = static_cast<i32>(g_buteMgr.GetDword("Spellz", s_RollingBallzTime, 0x3e8));

            CWwdSpriteObject* e = g_gameReg->m_world->m_childGroup->CreateSprite(
                0,
                m_lastTilePx.m_x + 0x20,
                m_lastTilePx.m_y,
                0,
                "RollingBall",
                WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
            );
            e->SetImageSetByName("LEVEL_ROLLINGBALL_EAST");
            CLogicRecord* ei = e->m_logicRecord;
            ei->m_speed =
                static_cast<i32>(g_buteMgr.GetDword("Spellz", s_RollingBallzSpeed, 0x3e8));
            e->m_smarts = 0;
            e->m_points = static_cast<i32>(g_buteMgr.GetDword("Spellz", s_RollingBallzTime, 0x3e8));

            CWwdSpriteObject* s = g_gameReg->m_world->m_childGroup->CreateSprite(
                0,
                m_lastTilePx.m_x,
                m_lastTilePx.m_y + 0x20,
                0,
                "RollingBall",
                WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
            );
            s->SetImageSetByName("LEVEL_ROLLINGBALL_SOUTH");
            CLogicRecord* si = s->m_logicRecord;
            si->m_speed =
                static_cast<i32>(g_buteMgr.GetDword("Spellz", s_RollingBallzSpeed, 0x3e8));
            s->m_smarts = 0;
            s->m_points = static_cast<i32>(g_buteMgr.GetDword("Spellz", s_RollingBallzTime, 0x3e8));

            CWwdSpriteObject* w = g_gameReg->m_world->m_childGroup->CreateSprite(
                0,
                m_lastTilePx.m_x - 0x20,
                m_lastTilePx.m_y,
                0,
                "RollingBall",
                WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
            );
            w->SetImageSetByName("LEVEL_ROLLINGBALL_WEST");
            CLogicRecord* wi = w->m_logicRecord;
            wi->m_speed =
                static_cast<i32>(g_buteMgr.GetDword("Spellz", s_RollingBallzSpeed, 0x3e8));
            w->m_smarts = 0;
            w->m_points = static_cast<i32>(g_buteMgr.GetDword("Spellz", s_RollingBallzTime, 0x3e8));
            return 1;
        }
        default:
            return 0;
    }
}

RVA(0x00057800, 0x64)
void CGrunt::SelectMoveIcon(i32 moveIconId) {
    if (IDX(m_moveIcon) == moveIconId) {
        return;
    }
    m_moveIcon = static_cast<PickupType>(moveIconId);
    if (moveIconId < 0 || moveIconId >= IDX(PICKUP_TIMEBOMB)) {
        m_moveIcon = PICKUP_NONE;
    }
    CShadeTable* sel =
        g_gameReg->m_spriteFactory->GetSel(IDX(m_moveIcon), m_entranceReason >= PICKUP_TOYZ_FIRST);
    CWwdSpriteObject* h = m_object;
    SET_DRAW_FILL(h, SHADE_PAL_16, sel);
}

RVA(0x00057890, 0x19c)
i32 CGrunt::BuildGruntLoseItemAnimation() {
    FinishActiveAction();
    PickupType reason = m_entranceReason;
    if (reason != PICKUP_TOOB && reason != PICKUP_WINGZ && reason != PICKUP_SPRING) {
        return 0;
    }

    CWwdSpriteObject* spr = g_gameReg->m_world->m_childGroup->CreateSprite(
        0,
        m_object->m_screenX,
        m_object->m_screenY,
        SORTKEY_ACTOR,
        "SingleAnimation",
        WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
    );
    spr->SetImageSetByName("GRUNTZ_" + m_animSetName + s__LOSEITEM);
    spr->SetAnimationByName("GRUNTZ_" + m_animSetName + s__LOSEITEM, 0);

    CGruntzMgr* g = g_gameReg;
    i32 y = m_object->m_screenY;
    i32 x = m_object->m_screenX;
    CCueRect* rc = &g->m_world->m_level->m_mainPlane->m_planeViewRect;
    if (::PtInRect(rc, x, y)) {
        g->m_voiceManager->PlayGruntVoiceCue(this, 0xe, -1, -1, -1);
    }

    LoadGruntTypeTable(PICKUP_NONE, 1, 0, 1);
    m_entranceActive = false;
    return 1;
}

RVA(0x00057aa0, 0x9b)
i32 CGrunt::TryPowerupAtTile() {
    PickupType reason = m_entranceReason;
    if (reason <= PICKUP_NONE || reason >= PICKUP_TOYZ_FIRST) {
        return 0;
    }
    CWwdSpriteObject* h = m_object;
    i32 mx = h->m_screenX;
    i32 my = h->m_screenY;
    i32 px = (mx & ~TILE_MASK_PX) + TILE_HALF_PX;
    i32 py = (my & ~TILE_MASK_PX) + TILE_HALF_PX;
    i32 tx = px >> TILE_SHIFT_PX;
    i32 ty = py >> TILE_SHIFT_PX;
    CGruntzMapMgr* b = g_gameReg->m_tileGrid;
    i32 flags;
    if (static_cast<u32>(tx) >= static_cast<u32>(b->m_width)
        || static_cast<u32>(ty) >= static_cast<u32>(b->m_height)) {
        flags = 1;
    } else {
        flags = b->m_rowInts[ty][tx * 7];
    }
    if ((flags & BRICKZ_BLOCKED_MASK) || (flags & IDX(CELL_FLAG_SPECIAL))) {
        return 0;
    }
    m_triggerMgr->SpawnPowerupIcon(reason, px, py, 0, 1, 0);
    return 1;
}

RVA(0x00057b70, 0x77)
void CGrunt::EnsureVehicleLoopSound(const char* key) {
    SoundBuffer*& sound = m_vehicleLoopSound;
    if (sound != NULL) {
        return;
    }
    if (g_gameReg->m_soundEnabled == false) {
        return;
    }
    CDDrawSurfaceMgr* world = g_gameReg->m_world;
    SoundCue* cue = NULL;
    MapLookup(world->m_soundRegistry->m_cues, key, cue);
    if (cue == NULL) {
        return;
    }
    if (cue->m_sound == NULL) {
        return;
    }
    sound = static_cast<SoundBuffer*>(cue->m_sound->AcquireInstance());
    if (sound == NULL) {
        return;
    }
    sound->ApplyAndPlay(g_gameReg->m_soundVolume, 0, 0, true);
}

RVA(0x00057c10, 0x1e)
void CGrunt::StopVehicleLoopSound() {
    SoundBuffer* sound = m_vehicleLoopSound;
    if (sound) {
        sound->StopAndRewind();
        m_vehicleLoopSound = NULL;
    }
}

RVA(0x00057c40, 0x71)
void CGrunt::EnsurePowerupLoopSound(const char* key) {
    SoundBuffer*& sound = m_powerupLoopSound;
    if (sound != NULL) {
        return;
    }
    SoundCue* cue = NULL;
    MapLookup(g_gameReg->m_world->m_soundRegistry->m_cues, key, cue);
    if (cue == NULL) {
        return;
    }
    if (cue->m_sound == NULL) {
        return;
    }
    sound = static_cast<SoundBuffer*>(cue->m_sound->AcquireInstance());
    if (sound == NULL) {
        return;
    }
    sound->ApplyAndPlay(g_gameReg->m_soundVolume, 0, 0, true);
}

RVA(0x00057ce0, 0x1e)
void CGrunt::StopPowerupLoopSound() {
    SoundBuffer* sound = m_powerupLoopSound;
    if (sound) {
        sound->StopAndRewind();
        m_powerupLoopSound = NULL;
    }
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00057d10, 0x4e)
void CGrunt::ReapplyLoopSoundParams() {
    if (g_gameReg->m_soundEnabled == false) {
        return;
    }
    SoundBuffer* vehicleSound = m_vehicleLoopSound;
    if (vehicleSound != NULL) {
        vehicleSound->ApplyAndPlay(g_gameReg->m_soundVolume, 0, 0, true);
    }
    SoundBuffer* powerupSound = m_powerupLoopSound;
    if (powerupSound != NULL) {
        powerupSound->ApplyAndPlay(g_gameReg->m_soundVolume, 0, 0, true);
    }
}

RVA(0x00057d80, 0x11)
void CGrunt::DestroyAnims() {
    STOP_GRUNT_LOOP_SOUNDS;
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

    Coord start;
    start.m_x = m_object->m_screenX >> TILE_SHIFT_PX;
    start.m_y = m_object->m_screenY >> TILE_SHIFT_PX;

    {
        RECT gb;
        gb.left = 0;
        gb.top = 0;
        gb.right = grid->m_width;
        gb.bottom = grid->m_height;
        RECT rs;
        rs.left = start.m_x - 2;
        rs.top = start.m_y - 2;
        rs.right = start.m_x + 2;
        rs.bottom = start.m_y + 2;
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
    Coord target = *tail->m_coord;
    i32 hits = 0;

    while (node != NULL) {
        Coord* co = static_cast<Coord*>(coordz->GetNext(node));
        if (co != NULL) {

            if ((grid->m_rows[co->m_y][co->m_x].m_flagBytes[3] & 0x20) == 0
                || (co->m_x == target.m_x && co->m_y == target.m_y)) {

                CPtrList s(0xa);
                i32 res = grid->FindPathWithEndpointOverrides(
                    start.m_x,
                    start.m_y,
                    co->m_x,
                    co->m_y,
                    &s,
                    1,
                    m_arrivalFlags | BRICKZ_CELL_OCCUPIED,
                    m_passableMask
                );
                if (res != 0) {
                    if (s.GetCount() != 0) {

                        while (node != NULL) {
                            Coord* src = static_cast<Coord*>(coordz->GetNext(node));
                            Coord* fresh = NULL;
                            CoordPoolNode* free = g_coordPool.m_freeHead;
                            if (free->m_next != NULL) {
                                fresh = &free->m_coord;
                                *fresh = *src;
                                g_coordPool.m_freeHead = g_coordPool.m_freeHead->m_next;
                            }
                            s.AddTail(fresh);
                        }

                        if (CoordCount() != 0) {
                            POSITION pos = m_coordList.GetHeadPosition();
                            if (pos != NULL) {
                                do {
                                    Coord* d = static_cast<Coord*>(m_coordList.GetNext(pos));
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
                                    if (d->m_x != start.m_x || d->m_y != start.m_y) {
                                        coordz->AddTail(d);
                                    }
                                }
                            } while (p != NULL);
                        }
                        Coord* elem = static_cast<Coord*>(s.RemoveHead());
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

        if (hits == GRUNT_COMBAT_FULL_SCAN_HITS) {
            SCAN_BOUNDS(grid);
            break;
        }
    }

    SCAN_BOUNDS(grid);

    RECT nb;
    nb.left = target.m_x - 4;
    nb.top = target.m_y - 4;
    nb.right = target.m_x + 4;
    nb.bottom = target.m_y + 4;
    if (::PtInRect(&nb, start.m_x, start.m_y)) {

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
        RECT* raDst = &grid->m_bounds;
        if (!IntersectRect(raDst, &ra, &rb)) {
            *raDst = ra;
        }
        grid->m_gridW = raDst->right - raDst->left;
        grid->m_gridH = raDst->bottom - raDst->top;

        for (i32 dy = -1; dy < 2; dy++) {
            for (i32 dx = -1; dx < 2; dx++) {
                if (dy == 0 && dx == 0) {
                    continue;
                }
                i32 rr = target.m_y + dy;
                i32 cc = target.m_x + dx;

                i32 cf = grid->CellFlagsAt(cc, rr);
                i32 mf = (m_arrivalFlags | 0x20040002) & cf;
                if (mf & BRICKZ_CELL_OCCUPIED) {
                    continue;
                }
                if (mf != 0 && (m_passableMask & cf) == 0) {
                    continue;
                }
                CPtrList s(0xa);
                i32 res = grid->FindPathWithEndpointOverrides(
                    start.m_x,
                    start.m_y,
                    cc,
                    rr,
                    &s,
                    0,
                    m_arrivalFlags | 0x20040002,
                    m_passableMask
                );
                if (res != 0) {

                    if (s.GetCount() != 0) {
                        Coord* elem = static_cast<Coord*>(s.RemoveHead());
                        if (elem != NULL) {
                            FREELIST_PUSH(elem);
                        }
                        if (s.GetCount() != 0) {

                            if (CoordCount() != 0) {
                                POSITION pos = m_coordList.GetHeadPosition();
                                if (pos != NULL) {
                                    do {
                                        Coord* d = static_cast<Coord*>(
                                            static_cast<CGruntCoordList*>(coordz)->NextData(pos)
                                        );
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

                            if (grid->FindPathWithEndpointOverrides(
                                    cc,
                                    rr,
                                    target.m_x,
                                    target.m_y,
                                    &s,
                                    1,
                                    m_arrivalFlags,
                                    m_passableMask
                                )
                                != 0) {
                                if (s.GetCount() != 0) {
                                    Coord* e2 = static_cast<Coord*>(s.RemoveHead());
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
                    grid->Clip(NULL);
                    return 0;
                }
            }
        }
    }
    grid->Clip(NULL);
    return 0;
}

// @early-stop
RVA(0x000588f0, 0x1ea)
void CGrunt::OnStruck(b32 wasHit) {
    m_struckTimerLo = 0xfa0;
    m_struckTimerHi = 0;
    m_struckClockLo = static_cast<i32>(g_frameTime);
    m_struckClockHi = 0;
    i32 c = ++m_struckCount;

    if (wasHit == false) {
        if (m_gruntKind == GRUNT_GHOST) {
            return;
        }
        i32 y = m_object->m_screenY;
        i32 x = m_object->m_screenX;
        if (c < 5) {
            CGruntzMgr* g = g_gameReg;
            const RECT* vr = &g->m_world->m_level->m_mainPlane->m_planeViewRect;
            if (::PtInRect(vr, x, y)) {
                g->m_voiceManager->PlayVoice(this, 0x370, -1, 0, -1, -1);
            }
            return;
        }
        CGruntzMgr* g = g_gameReg;
        const RECT* vr = &g->m_world->m_level->m_mainPlane->m_planeViewRect;
        if (::PtInRect(vr, x, y)) {
            g->m_voiceManager->PlayVoice(this, 0x371, -1, 0, -1, -1);
        }
        m_struckCount = 0;
        return;
    }

    if (c < 5) {
        i32 y = m_object->m_screenY;
        i32 x = m_object->m_screenX;
        CGruntzMgr* g = g_gameReg;
        const RECT* vr = &g->m_world->m_level->m_mainPlane->m_planeViewRect;
        if (::PtInRect(vr, x, y)) {
            g->m_voiceManager->PlayVoice(this, 0x320, -1, 0, -1, -1);
        }
        return;
    }
    if (c < 0xa) {
        i32 y = m_object->m_screenY;
        i32 x = m_object->m_screenX;
        CGruntzMgr* g = g_gameReg;
        const RECT* vr = &g->m_world->m_level->m_mainPlane->m_planeViewRect;
        if (::PtInRect(vr, x, y)) {
            g->m_voiceManager->PlayVoice(this, 0x321, -1, 0, -1, -1);
        }
        return;
    }
    {
        i32 y = m_object->m_screenY;
        i32 x = m_object->m_screenX;
        m_struckCount = 0;
        CGruntzMgr* g = g_gameReg;
        const RECT* vr = &g->m_world->m_level->m_mainPlane->m_planeViewRect;
        if (::PtInRect(vr, x, y)) {
            g->m_voiceManager->PlayVoice(this, 0x322, -1, 0, -1, -1);
        }
    }
}

RVA(0x00058b60, 0x2d)
void CWwdSpriteObject::SetAnimation(CAniElement* animation, i32 advanceImmediately){
    SET_ANIMATION_AND_MAYBE_ADVANCE(this, animation, advanceImmediately)
}

RVA(0x00058bc0, 0xa1)
i32 CMotionState::SetParams(
    double posX,
    double posY,
    double posZ,
    double velX,
    double velY,
    double velZ,
    double accelX,
    double accelY,
    double accelZ,
    double clock,
    double dt
) {
    m_position.Init(posX, posY, posZ);
    m_velocity.Init(velX, velY, velZ);
    m_acceleration.Init(accelX, accelY, accelZ);
    m_time = clock;
    m_deltaTime = dt;
    return 1;
}

RVA(0x00058ca0, 0x19)
void CMotionState::SetZ(double z) {
    m_maxStep.Init(z, z, z);
}

RVA(0x00058cd0, 0x195)
CUserLogic::CUserLogic(CGameObject* obj) {
    USERLOGIC_ATTACH_TO_OBJECT(obj);
}

RVA(0x00058ee0, 0x5c)
i32 CPairRecord::Serialize(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    switch (mode) {
        case SERIAL_SAVE:
            ar->Write(&m_start, sizeof(m_start));
            ar->Write(&m_duration, sizeof(m_duration));
            break;
        case SERIAL_LOAD:
            ar->Read(&m_start, sizeof(m_start));
            ar->Read(&m_duration, sizeof(m_duration));
            break;
    }
    return 1;
}

// @early-stop
RVA(0x00059230, 0x450)
i32 CGrunt::HandleCombatContact(
    i32 otherPxX,
    i32 otherPxY,
    b32 isAttacker,
    i32 otherPlayerIndex,
    i32 otherUnitIndex
) {
    if (isAttacker == false) {
        switch (m_arrivalState) {
            case AI_NONE:
                break;
            case AI_SMARTCHASER:
                m_arrivalCell.m_x = otherPlayerIndex;
                m_arrivalCell.m_y = otherUnitIndex;
                break;
            case AI_DUMBCHASER:
            case AI_DEFENDER:
                m_arrivalCell.m_x = otherPlayerIndex;
                m_arrivalCell.m_y = otherUnitIndex;
                m_defenderState = AISTATE_ATTACK;
                break;
            case AI_POSTGUARD:
                m_arrivalCell.m_x = otherPlayerIndex;
                m_arrivalCell.m_y = otherUnitIndex;
                m_defenderState = AISTATE_ATTACK;
                break;
            case AI_HITANDRUNNER:
            case AI_OBJECTGUARD:
                m_arrivalCell.m_x = otherPlayerIndex;
                m_arrivalCell.m_y = otherUnitIndex;
                m_defenderState = AISTATE_ATTACK;
                break;
            case AI_BATTLEZ_PATH:
                m_arrivalCell.m_x = otherPlayerIndex;
                m_arrivalCell.m_y = otherUnitIndex;
                break;
            default:
                break;
        }

        i32 phase = m_arrivalPhase;
        if ((phase == ARRIVAL_TAG_TRIGGER_B || phase == ARRIVAL_TAG_TRIGGER_A)
            && m_arrivalActive != false) {
            CGrunt* occ =
                m_triggerMgr->m_units[m_arrivalCell.m_x * TM_UNITS_PER_PLAYER + m_arrivalCell.m_y];
            if (occ != NULL) {
                CGameObject* inner = occ->m_object;
                i32 sx = inner->m_screenX;
                i32 sy = inner->m_screenY;
                i32 xMasked = (sx & ~TILE_MASK_PX) + TILE_HALF_PX;
                i32 yMasked = (sy & ~TILE_MASK_PX) + TILE_HALF_PX;
                i32 applied;
                if (phase == ARRIVAL_TAG_TRIGGER_B) {
                    if (VehicleContactContains(xMasked, yMasked) != 0) {
                        FinishActiveAction();
                    }
                    applied = m_triggerMgr->UseToyAt(m_playerIndex, m_unitIndex, sx, sy);
                } else {
                    if (RectContains(xMasked, yMasked) != 0) {
                        FinishActiveAction();
                    }
                    applied = m_triggerMgr->UseEquippedToolAt(m_playerIndex, m_unitIndex, sx, sy);
                }
                if (applied == 0 || applied == 1) {
                    if (applied == 1) {
                        SetEntrancePos(1, 1);
                    }
                }
            }
        }
    } else {
        FaceTowardPixel(otherPxX, otherPxY);

        char** rec0 = g_typeColl.GetNameRecordRaw(m_logicRecord->m_eventCode);
        ActNameConstructGrownSlots();
        bool neH = (strcmp(*rec0, "H") != 0);
        if (neH) {
            i32 keyF = m_logicRecord->m_eventCode;
            g_typeColl.m_grown = 0;
            CString* recF;
            if (keyF < g_typeColl.m_lo || keyF > g_typeColl.m_hi) {
                if (g_typeColl.GrowTo(keyF, 0) != NULL) {
                    recF = g_typeColl.Elem(keyF);
                } else {
                    g_typeColl.Report(g_errOutOfMem, 0xc);
                    recF = g_typeColl.Scratch();
                }
            } else {
                recF = g_typeColl.Elem(keyF);
            }
            ActNameConstructGrownSlots();
            bool neF = (strcmp(*CTypeCollRuntime::NameOf(recF), DATA_COMPGEN(0x0020d2e8, "F")) != 0);
            if (neF) {
                i32 keyO = m_logicRecord->m_eventCode;
                g_typeColl.m_grown = 0;
                CString* recO;
                if (keyO < g_typeColl.m_lo || keyO > g_typeColl.m_hi) {
                    if (g_typeColl.GrowTo(keyO, 0) != NULL) {
                        recO = g_typeColl.Elem(keyO);
                    } else {
                        char* msg = g_errOutOfMem;
                        g_retAddrBreadcrumb = GetRetAddr();
                        g_typeColl.m_errSink->Set(&g_typeColl, msg, 0xc);
                        recO = g_typeColl.Scratch();
                    }
                } else {
                    recO = g_typeColl.Elem(keyO);
                }
                ActNameConstructGrownSlots();
                bool neO = (strcmp(*CTypeCollRuntime::NameOf(recO), "O") != 0);
                if (neO) {
                    ResetGeometry();
                }
            }
        }
    }
    return 1;
}

RVA(0x000597a0, 0x13c0)
i32 CGrunt::LoadGruntCombatAnimations(
    PickupType attackKind,
    i32 struckPose,
    i32 srcPlayerIndex,
    i32 srcUnitIndex,
    i32 srcPxX,
    i32 srcPxY,
    i32 fromProjectile,
    PickupType attackerGruntKind
) {
    if (this->m_gruntKind == GRUNT_INVULNERABLE && this->m_entranceReason != PICKUP_BOMB) {
        return 1;
    }

    if (attackerGruntKind == GRUNT_CONVERSION) {
        CGrunt* enemy = m_triggerMgr->m_units[srcPlayerIndex * TM_UNITS_PER_PLAYER + srcUnitIndex];
        if (enemy != NULL
            && m_triggerMgr->SpawnGrunt(
                   this->m_playerIndex,
                   this->m_unitIndex,
                   srcPlayerIndex,
                   IDX(enemy->m_moveIcon)
               ) != 0) {
            i32 h = enemy->m_health + 0x19;
            if (h >= HEALTH_FULL) {
                h = HEALTH_FULL;
            }
            enemy->m_health = h;

            SoundCueRegistry* registry =
                (static_cast<CDDrawSurfaceMgr*>(m_ownerLogicRecord->m_ownerCtx))->m_soundRegistry;
            if (registry->m_silentMode == false) {
                SoundCue* cue = static_cast<SoundCue*>(registry->Lookup(s_CONVERSIONHIT));
                if (cue != NULL) {
                    cue->PlayIfElapsed(g_soundVolumePercent, 0, 0, false);
                }
            }
            return 0;
        }
    }

    i32 hit = AT(AT(g_hitTable, this->m_entranceReason), attackKind);
    if (g_gameReg->m_isEasyMode != false && g_gameReg->m_gameMode == GAMEMODE_QUESTZ
        && this->m_playerIndex == g_curPlayer) {
        i32 t = hit / 2;
        hit = t + t % 5;
    }

    if (attackerGruntKind == GRUNT_DEATHTOUCH) {
        hit = 0x64;
    } else if (this->m_gruntKind == GRUNT_REACTIVEARMOR) {
        hit = static_cast<i32>((static_cast<float>(hit) * g_quarterScale));
        if (fromProjectile == 0) {
            CGrunt* enemy =
                m_triggerMgr->m_units[srcPlayerIndex * TM_UNITS_PER_PLAYER + srcUnitIndex];
            if (enemy != NULL && enemy->m_entranceCommitted != false) {
                i32 nh = enemy->m_health - hit * 3;
                nh = (nh < 0) ? 0 : nh;
                enemy->m_health = nh;
                if (nh <= 0) {
                    m_triggerMgr->StartUnitDeath(srcPlayerIndex, srcUnitIndex, DEATH_NORMAL, -1);
                }
            }
        }
    }

    i32 nh = this->m_health - hit;
    nh = (nh < 0) ? 0 : nh;
    this->m_health = nh;
    if (this->m_entranceReason == PICKUP_BOMB) {
        m_triggerMgr
            ->StartUnitDeath(this->m_playerIndex, this->m_unitIndex, DEATH_NORMAL, srcPlayerIndex);
        return 0;
    }
    if (nh <= 0) {
        this->m_entranceCommitted = false;
        this->m_killerPlayerIndex = srcPlayerIndex;
    }

    SoundCue* cue = NULL;
    i32 vx = this->m_object->m_screenX;
    i32 vy = this->m_object->m_screenY;
    CGruntzMgr* reg = g_gameReg;
    if (::PtInRect(&reg->m_viewBounds, vx, vy)) {
        if (attackerGruntKind == GRUNT_DEATHTOUCH) {
            LK(s_DEATHTOUCHHIT);
            goto L_cue;
        }
        if (attackKind == PICKUP_NERFGUN || attackKind == PICKUP_GLOVEZ
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
                LK("GRUNTZ_NORMALGRUNT_IMPACTMM3");
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
        if (this->m_entranceReason == PICKUP_TOOB && this->m_coordToggle != false) {
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
                    LK("GRUNTZ_NORMALGRUNT_IMPACTMM3");
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
                    LK("GRUNTZ_NORMALGRUNT_IMPACTMM3");
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
                LK("GRUNTZ_NORMALGRUNT_IMPACTMM3");
                break;
        }

    L_cue:

        if (cue != NULL) {
            b32 soundEnabled = g_soundEnabled;
            i32 volumePercent = g_soundVolumePercent;
            if (soundEnabled != false) {
                i32 cueTimeMs = g_soundCueTimeMs;
                if (static_cast<u32>((cueTimeMs - cue->m_lastPlayTimeMs))
                    >= static_cast<u32>(cue->m_replayDelayMs)) {
                    cue->m_lastPlayTimeMs = cueTimeMs;
                    cue->m_sound->AcquireAndPlay(volumePercent, 0, 0, false);
                }
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
        m_triggerMgr
            ->StartUnitDeath(this->m_playerIndex, this->m_unitIndex, DEATH_BURN, srcPlayerIndex);
        return 0;
    }

    if (this->m_entranceReason == PICKUP_GRAVITYBOOTZ) {
        return 1;
    }

    CString* typeRec = g_typeColl.ScratchResolve(this->m_logicRecord->m_eventCode);
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
    bool isCodeO = (strcmp(*typeRec, "O") == 0);
    if (isCodeO) {
        return 1;
    }

    i32 dy = srcPxY - this->m_object->m_screenY;
    i32 dx = srcPxX - this->m_object->m_screenX;
    Coord newPos;
    if (attackKind == PICKUP_WINGZ) {
        switch (static_cast<WingzKnockbackChoice>(rand() % 8 - 1)) {
            case WINGZ_KNOCKBACK_NORTHEAST:
                SETDIR(
                    s_gruntDirSouthWest,
                    this->m_lastTilePx.m_x + 0x20,
                    this->m_lastTilePx.m_y - 0x20
                );
                break;
            case WINGZ_KNOCKBACK_EAST:
                SETDIR(s_gruntDirWest, this->m_lastTilePx.m_x + 0x20, this->m_lastTilePx.m_y);
                break;
            case WINGZ_KNOCKBACK_SOUTHEAST:
                SETDIR(
                    s_gruntDirNorthWest,
                    this->m_lastTilePx.m_x + 0x20,
                    this->m_lastTilePx.m_y + 0x20
                );
                break;
            case WINGZ_KNOCKBACK_SOUTH:
                SETDIR(s_gruntDirNorth, this->m_lastTilePx.m_x, this->m_lastTilePx.m_y + 0x20);
                break;
            case WINGZ_KNOCKBACK_SOUTHWEST:
                SETDIR(
                    s_gruntDirNorthEast,
                    this->m_lastTilePx.m_x - 0x20,
                    this->m_lastTilePx.m_y + 0x20
                );
                break;
            case WINGZ_KNOCKBACK_WEST:
                SETDIR(s_gruntDirEast, this->m_lastTilePx.m_x - 0x20, this->m_lastTilePx.m_y);
                break;
            case WINGZ_KNOCKBACK_NORTHWEST:
                SETDIR(
                    s_gruntDirSouthEast,
                    this->m_lastTilePx.m_x - 0x20,
                    this->m_lastTilePx.m_y - 0x20
                );
                break;
            default:
                SETDIR(s_gruntDirSouth, this->m_lastTilePx.m_x, this->m_lastTilePx.m_y - 0x20);
                break;
        }
    } else if (dx == 0) {
        if (srcPxY > this->m_object->m_screenY) {
            SETDIR(s_gruntDirSouth, this->m_lastTilePx.m_x, this->m_lastTilePx.m_y - 0x20);
        } else if (srcPxY < this->m_object->m_screenY) {
            SETDIR(s_gruntDirNorth, this->m_lastTilePx.m_x, this->m_lastTilePx.m_y + 0x20);
        }
    } else {
        float slope = static_cast<float>(dy) / dx;
        if (slope > g_slopeTwo || slope < g_slopeNegTwo) {
            if (srcPxY > this->m_object->m_screenY) {
                SETDIR(s_gruntDirSouth, this->m_lastTilePx.m_x, this->m_lastTilePx.m_y - 0x20);
            } else {
                SETDIR(s_gruntDirNorth, this->m_lastTilePx.m_x, this->m_lastTilePx.m_y + 0x20);
            }
        } else if (slope > g_combatSlopeHalf || slope < g_combatSlopeNegHalf) {
            if (slope > g_combatSlopeHalf) {
                if (srcPxX > this->m_object->m_screenX) {
                    SETDIR(
                        s_gruntDirSouthEast,
                        this->m_lastTilePx.m_x - 0x20,
                        this->m_lastTilePx.m_y - 0x20
                    );
                } else {
                    SETDIR(
                        s_gruntDirNorthWest,
                        this->m_lastTilePx.m_x + 0x20,
                        this->m_lastTilePx.m_y + 0x20
                    );
                }
            } else if (slope < g_combatSlopeNegHalf) {
                if (srcPxX > this->m_object->m_screenX) {
                    SETDIR(
                        s_gruntDirNorthEast,
                        this->m_lastTilePx.m_x - 0x20,
                        this->m_lastTilePx.m_y + 0x20
                    );
                } else {
                    SETDIR(
                        s_gruntDirSouthWest,
                        this->m_lastTilePx.m_x + 0x20,
                        this->m_lastTilePx.m_y - 0x20
                    );
                }
            }
        } else {
            if (srcPxX > this->m_object->m_screenX) {
                SETDIR(s_gruntDirEast, this->m_lastTilePx.m_x - 0x20, this->m_lastTilePx.m_y);
            } else {
                SETDIR(s_gruntDirWest, this->m_lastTilePx.m_x + 0x20, this->m_lastTilePx.m_y);
            }
        }
    }

    {
        i32 flags = this->m_arrivalFlags | BRICKZ_CELL_OCCUPIED;
        CMapMgr* grid = static_cast<CMapMgr*>(g_gameReg->m_tileGrid);
        i32 nyt = newPos.m_y >> TILE_SHIFT_PX;
        i32 nxt = newPos.m_x >> TILE_SHIFT_PX;
        i32 oxt = this->m_lastTilePx.m_x >> TILE_SHIFT_PX;
        i32 oyt = this->m_lastTilePx.m_y >> TILE_SHIFT_PX;
        if (!(oxt == nxt && nyt == oyt)) {
            i32 w = grid->m_width;
            if (static_cast<u32>(nxt) >= static_cast<u32>(w)) {
                return 1;
            }
            if (static_cast<u32>(nyt) >= static_cast<u32>(grid->m_height)) {
                return 1;
            }
            BrickzCell* cell = &grid->m_rows[nyt][nxt];
            i32 t = flags & cell->m_flags;
            if (t & BRICKZ_CELL_OCCUPIED) {
                return 1;
            }
            if (t != 0 && (cell->m_flags & (this->m_passableMask | 0x18000482)) == 0) {
                return 1;
            }
            BrickzCell* ocell = &grid->m_rows[oyt][oxt];
            i32 dxt = nxt - oxt;
            i32 dyt = nyt - oyt;
            if (dxt != 0 && dyt != 0) {
                if (dxt > 0 && dyt > 0) {
                    if (((ocell + 1)->m_flags & BRICKZ_CELL_ROUTE_MASKB)
                        || ((ocell + w)->m_flags & BRICKZ_CELL_ROUTE_MASKB)
                        || ((cell - 1)->m_flags & BRICKZ_CELL_ROUTE_MASKB)
                        || ((cell - w)->m_flags & BRICKZ_CELL_ROUTE_MASKB)) {
                        return 1;
                    }
                } else if (dxt < 0 && dyt > 0) {
                    if (((ocell - 1)->m_flags & BRICKZ_CELL_ROUTE_MASKB)
                        || ((ocell + w)->m_flags & BRICKZ_CELL_ROUTE_MASKB)
                        || ((cell + 1)->m_flags & BRICKZ_CELL_ROUTE_MASKB)
                        || ((cell - w)->m_flags & BRICKZ_CELL_ROUTE_MASKB)) {
                        return 1;
                    }
                } else if (dxt > 0 && dyt < 0) {
                    if (((ocell + 1)->m_flags & BRICKZ_CELL_ROUTE_MASKB)
                        || ((ocell - w)->m_flags & BRICKZ_CELL_ROUTE_MASKB)
                        || ((cell - 1)->m_flags & BRICKZ_CELL_ROUTE_MASKB)
                        || ((cell + w)->m_flags & BRICKZ_CELL_ROUTE_MASKB)) {
                        return 1;
                    }
                } else if (dxt < 0 && dyt < 0) {
                    if (((ocell - 1)->m_flags & BRICKZ_CELL_ROUTE_MASKB)
                        || ((ocell - w)->m_flags & BRICKZ_CELL_ROUTE_MASKB)
                        || ((cell + 1)->m_flags & BRICKZ_CELL_ROUTE_MASKB)
                        || ((cell + w)->m_flags & BRICKZ_CELL_ROUTE_MASKB)) {
                        return 1;
                    }
                }
            }
        }

        if (this->m_arrivalPending == false) {
            m_triggerMgr->ApplySwitch(this, this->m_lastTilePx.m_x, this->m_lastTilePx.m_y);
        }
        CMapMgr* oldGrid = static_cast<CMapMgr*>(g_gameReg->m_tileGrid);
        i32 ox = this->m_lastTilePx.m_x >> TILE_SHIFT_PX;
        i32 oy = this->m_lastTilePx.m_y >> TILE_SHIFT_PX;
        oldGrid->m_rows[oy][ox].m_flagBytes[3] &= 0xdf;
        oldGrid->m_rows[oy][ox].m_occupantId = -1;

        CMapMgr* newGrid = static_cast<CMapMgr*>(g_gameReg->m_tileGrid);
        newGrid->m_rows[nyt][nxt].m_flagBytes[3] |= 0x20;
        newGrid->m_rows[nyt][nxt].m_occupantId =
            (this->m_playerIndex << GRUNT_IDENTITY_PLAYER_SHIFT) | this->m_unitIndex;

        if (m_coordList.GetCount() != 0) {
            Coord* node = NULL;
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

        this->m_lastTilePx = newPos;
        SET_ANIMATION_ACT("O");
        double ddx = static_cast<double>(this->m_lastTilePx.m_x) - this->m_object->m_screenX;
        double ddy = static_cast<double>(this->m_lastTilePx.m_y) - this->m_object->m_screenY;
        double dist = sqrt(ddx * ddx + ddy * ddy);
        m_moveSpeed = dist / static_cast<double>(g_buteMgr.GetDword("Grunt", s_knockKey, 200));
        m_movePosX = static_cast<double>((this->m_object->m_screenX));
        m_movePosY = static_cast<double>((this->m_object->m_screenY));

        if (m_coordList.GetCount() != 0) {
            RECYCLE_GRUNT_COORDS_EXPANDED(this)
        }
        this->m_arrivalPending = false;
    }

    return 1;
}

// @early-stop
RVA(0x0005b050, 0x40b)
i32 CGrunt::CommitNeighbor(
    i32 targetPlayerIndex,
    i32 targetUnitIndex,
    i32 targetPxX,
    i32 targetPxY
) {
    if (targetPlayerIndex == m_playerIndex && g_traitorMode == false) {
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
        i32 flags = bd->CellFlagsAt(tx, ty);
        if (flags & 0x80) {
            return 0;
        }
    }

    CreateHealthSprite();
    ArmGruntCombatTimeout(this);
    m_neighborScanEnabled = true;

    CGrunt* nb = m_triggerMgr->m_units[targetPlayerIndex * TM_UNITS_PER_PLAYER + targetUnitIndex];
    if (nb == NULL || nb->m_entranceCommitted == false || m_entranceCommitted == false) {
        return 0;
    }

    bool eq;
    eq = ANIMATION_ACT_EQUALS("F");
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
        RunMoveConfig(targetPxX >> TILE_SHIFT_PX, targetPxY >> TILE_SHIFT_PX);
        return 1;
    }

    eq = ANIMATION_ACT_EQUALS("I");
    if (eq) {
        m_triggerMgr->LoadTileArrivalFx(
            m_playerIndex,
            m_unitIndex,
            m_moveTile.m_x,
            m_moveTile.m_y,
            m_entranceReason,
            WWDDRAW_NO_ANIMATION
        );
    } else {
        eq = ANIMATION_ACT_EQUALS("N");
        if (eq) {
            i32 lastX = m_lastTilePx.m_x;
            i32 lastY = m_lastTilePx.m_y;
            DECLARE_SNAPPED_SCREEN_PIXEL_PAIR(m_object, px, py)
            i32 redo = 1;
            if (PIXEL_PAIR_NOT_AT_POSITION(px, py, lastX, lastY)) {
                if (IsDropReady(1)) {
                    m_coordToggle = (m_coordToggle == false);
                    redo = 0;
                }
            }
            SnapToLastTile(1);
            if (redo) {
                SET_ANIMATION_ACT("D");
                SetupTubeAnim(m_coordToggle);
            }
        }
    }

    if (m_arrivalPending != false) {
        m_triggerMgr->WireTileSwitchLogic(this, m_object->m_screenX, m_object->m_screenY);
        m_arrivalPending = false;
    }
    m_poweredUp = true;
    nb->CreateHealthSprite();
    ArmGruntCombatTimeout(nb);
    HandleCombatContact(targetPxX, targetPxY, true, targetPlayerIndex, targetUnitIndex);
    m_neighborPlayerIndex = targetPlayerIndex;
    m_neighborUnitIndex = targetUnitIndex;
    m_attackTargetPx.m_x = targetPxX;
    m_attackTargetPx.m_y = targetPxY;
    if (m_stamina < STAMINA_FULL || m_entranceActive != false) {
        m_neighborValid = true;
        return 1;
    }
    m_neighborValid = false;
    nb->HandleCombatContact(
        m_object->m_screenX,
        m_object->m_screenY,
        false,
        m_playerIndex,
        m_unitIndex
    );
    StartNeighborAttackAnimation(targetPlayerIndex, targetUnitIndex);
    return 1;
}

// @early-stop
RVA(0x0005b570, 0x12b)
i32 CGrunt::BeginAttack(i32 targetPxX, i32 targetPxY) {
    if (m_entranceCommitted != false) {

        CString* rec = g_typeColl.ScratchResolve(m_logicRecord->m_eventCode);
        ActNameConstructGrownSlots();
        bool eq = (strcmp(*rec, "F") == 0);
        if (!eq) {
            if (m_stamina >= STAMINA_FULL) {

                FaceTowardPixel(targetPxX, targetPxY);
                m_poweredUp = true;
                m_combatActive = true;
                CreateHealthSprite();

                ArmGruntCombatTimeout(this);
                m_neighborScanEnabled = true;
                m_attackTargetPx.m_x = targetPxX;
                m_attackTargetPx.m_y = targetPxY;
                StartRangedAttackAnimation();
                return 1;
            }
        }
    }
    return 0;
}

RVA(0x0005b6f0, 0xb5)
CGrunt* CGrunt::FindGridNeighbor(i32 validate) {
    if (m_neighborPlayerIndex == -1) {
        return NULL;
    }
    if (m_neighborUnitIndex == -1) {
        return NULL;
    }

    CGrunt* n =
        m_triggerMgr->m_units[m_neighborPlayerIndex * TM_UNITS_PER_PLAYER + m_neighborUnitIndex];
    if (n != NULL && n->m_entranceCommitted != false) {
        if (validate != 0) {
            if (n->GRUNT_SCREEN_X_NOT_AT_SAVED_POS(m_object, n)) {
                return NULL;
            }
            if (n->GRUNT_SCREEN_Y_NOT_AT_SAVED_POS(m_object, n)) {
                return NULL;
            }
        }
        if (RectContains(n->m_object->m_screenX, n->m_object->m_screenY)) {
            CommitNeighbor(
                m_neighborPlayerIndex,
                m_neighborUnitIndex,
                n->m_object->m_screenX,
                n->m_object->m_screenY
            );
            return n;
        }
    }

    m_neighborValid = false;
    return NULL;
}

RVA(0x0005b7e0, 0x23)
CObject* SoundCueRegistry::Lookup(const char* key) {
    CObject* value = NULL;
    MapLookup(m_cues, key, value);
    return value;
}

RVA(0x0005baf0, 0xf4)
i32 DispatchGruntLogic(CGameObject* owner) {
    CLogicRecord* record = owner->m_logicRecord;
    switch (record->LogicEvent()) {
        case ACT_UNINITIALISED: {
            record->SetLogicEvent(ACT_LIVE);
            CUserLogic* sub = new CGrunt(owner);
            sub->Activate();
            record->m_userLogic = sub;
            break;
        }
        case ACT_OBJECT_REMOVED:
            record->m_userLogic->OnObjectRemoved();
            break;
        case ACT_LEAVE_ACTIVE_REGION:
            record->m_userLogic->OnLeaveActiveRegion();
            break;
        case ACT_PREPARE_SAVE:
            record->m_userLogic->PrepareSave();
            break;
        case ACT_AFTER_LOAD_REFERENCES:
            record->m_userLogic->AfterLoadReferences();
            break;
        case ACT_AFTER_LOAD:
            record->m_userLogic->AfterLoad();
            break;
        case ACT_AFTER_SAVE:
            record->m_userLogic->AfterSave();
            break;
        case ACT_LIVE:
            break;
        default:
            DispatchUnhandledLogicEvent(record->m_userLogic);
            break;
    }
    return 1;
}

RVA(0x0005bcd0, 0x102)
void CGrunt::FireActivation(i32 id) {
    CActHandler* e = CActRegPool<CGrunt>::s_table.ResolveEntry(id);
    if (*e != NULL) {
        (this->*(*CActRegPool<CGrunt>::s_table.ResolveEntry(id)))();
    }
}

// @early-stop
RVA(0x0005be30, 0x9e5)
void RegisterGruntActions() {
    REGISTER_KEY_644AF0("A", &CGrunt::ResolveEntranceArrival);
    REGISTER_KEY_644AF0("B", &CGrunt::StepWarpExit);
    REGISTER_KEY_644AF0("C", &CGrunt::UpdateDeathAnimation);
    REGISTER_KEY_644AF0("D", &CGrunt::StepArrivalReroll);
    REGISTER_KEY_644AF0("E", &CGrunt::UpdateGruntStatus);
    REGISTER_KEY_644AF0("F", &CGrunt::StepAttackAction);
    REGISTER_KEY_644AF0("G", &CGrunt::UpdateToyUseAnimation);
    REGISTER_KEY_644AF0("H", &CGrunt::FinishStruckAnimation);
    REGISTER_KEY_644AF0("I", &CGrunt::LoadWandGruntItemConfig);
    REGISTER_KEY_644AF0("J", &CGrunt::RunEntranceMove);
    REGISTER_KEY_644AF0("K", &CGrunt::LoadEntranceConfig);
    REGISTER_KEY_644AF0("L", &CGrunt::LoadVehicleGruntAnimations);
    REGISTER_KEY_644AF0("M", &CGrunt::RearmEntranceDrop);
    REGISTER_KEY_644AF0("N", &CGrunt::FinishToobMoveAnimation);
    REGISTER_KEY_644AF0("O", &CGrunt::FinishKnockbackAnimation);
    REGISTER_KEY_644AF0("P", &CGrunt::UpdateEntranceAnim);
    REGISTER_KEY_644AF0("Q", &CGrunt::LoadFreezeSpellAssets);
    REGISTER_KEY_644AF0_TYPED("R", &CGrunt::UpdateDecayFade);
    REGISTER_KEY_644AF0_DERIVED("S", &CGrunt::FinishEntranceMove);
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

    CWwdSpriteObject* h = m_object;
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
    m_entranceActive = false;
    m_arrivalPending = false;
    m_arrivalState = AI_NONE;
    m_poweredUp = false;
    m_resetApplied = false;
    m_arrivalFlags = ARRIVAL_FLAGS_PLAYER;
    m_passableMask = 0;
    m_deathAnimStarted = false;
    m_tileClaimed = false;
}

#undef REGISTER_KEY_644AF0

DATA(0x001e999c)
const float g_quarterScale = 0.25f;

DATA(0x001e99a0)
const float g_slopeTwo = 2.0f;
DATA(0x001e99a4)
const float g_slopeNegTwo = -2.0f;
DATA(0x001e99a8)
const double g_combatSlopeHalf = 0.5;
DATA(0x001e99b0)
const double g_combatSlopeNegHalf = -0.5;
DATA(0x001e9a48)
const double g_wingzScale = 100.0;
DATA(0x001e9a50)
const double g_wingzBias = -0.5;

DATA(0x001e9a68)
const double s_fpZero = 0.0;

// @early-stop
RVA(0x0005d210, 0x1554)
void CGrunt::StepBehavior(char*) {
    if (static_cast<i64>(g_frameTime) - m_struckClock64 >= m_struckTimer64) {
        m_struckCount = 0;
    }
    m_dwell += g_frameDelta;

    if (m_entranceDropActive != false) {
        bool differs = ANIMATION_ACT_DIFFERS("A");
        if (differs) {
            differs = ANIMATION_ACT_DIFFERS("K");
            if (differs) {
                goto dropExpire;
            }
        }

        if (static_cast<i64>(g_frameTime) - m_entranceClock64 >= m_entranceSafeTime64) {
        dropExpire: {
            CWwdSpriteObject* obj = m_object;
            m_entranceDropActive = false;
            obj->m_drawActive = true;
            obj->m_drawFillCmd = SHADE_PAL_16;
        }
            m_entranceSafeTimeLo = 0;
            m_entranceSafeTimeHi = 0;
        } else if (static_cast<i64>(g_frameTime) - m_flashClock64 >= m_flashWindow64) {
            CWwdSpriteObject* obj = m_object;
            if (obj->m_drawFillCmd == SHADE_PAL_ALPHA_16) {
                obj->m_drawActive = true;
                obj->m_drawFillCmd = SHADE_PAL_16;
            } else {
                i32 fade = g_buteMgr.GetInt("Grunt", s_FadeTransparency, 0xc0);
                CWwdSpriteObject* o2 = m_object;
                SET_DRAW_FILL_FRACTION(o2, SHADE_PAL_ALPHA_16, fade);
            }
            i32 flash = g_buteMgr.GetInt("Grunt", s_SafeFlashTime, 0x32);
            if (g_buteMgr.GetInt("Grunt", s_AccelerateFlash, 0) == 1) {
                i64 el = static_cast<i64>(g_frameTime) - m_entranceClock64;
                u32 elapsed = (el < 0 ? 0 : static_cast<u32>(el));

                double span =
                    static_cast<double>(g_buteMgr.GetDword("Grunt", "EntranceSafeTime", 0x1388));
                double frac = static_cast<double>(elapsed) / span - 1.0;
                flash = static_cast<i32>(frac * frac * DATA_COMPGEN(0x001e9a40, 750.0));
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

    if (m_deathAnimStarted != false) {
        return;
    }

    {
        CWwdSpriteObject* obj = m_object;
        if (GRUNT_OBJECT_NOT_AT_SELF_SAVED_SCREEN_POS(obj)) {
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
            CGameObject* found = NULL;
            CGameObject* result = NULL;
            if (MapLookupById(
                    reg->m_world->m_childGroup->m_registeredGameObjectsById,
                    cellObj,
                    found
                )) {
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

                CInGameIcon* icon = static_cast<CInGameIcon*>(result->m_logicRecord->m_userLogic);
                icon->PlaceAt(m_playerIndex, m_unitIndex);
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
            flags = bd->CellFlagsAt(tx, ty);
        }
        if (flags & 0x100000) {
            reg2->m_players[0].m_battlezConfig.ClaimCellFromRow(m_playerIndex, m_unitIndex, tx, ty);
        } else if (flags & 0x200000) {
            reg2->m_players[1].m_battlezConfig.ClaimCellFromRow(m_playerIndex, m_unitIndex, tx, ty);
        } else if (flags & 0x400000) {
            reg2->m_players[2].m_battlezConfig.ClaimCellFromRow(m_playerIndex, m_unitIndex, tx, ty);
        } else if (flags & 0x800000) {
            reg2->m_players[3].m_battlezConfig.ClaimCellFromRow(m_playerIndex, m_unitIndex, tx, ty);
        }

        if (onWingzTile != 0) {
            if (flags & 0xd02) {
                if (m_wingzEnabled == false) {
                    LoadWingzGruntSprites(true);
                    return;
                }
            } else if (m_wingzEnabled != false) {
                LoadWingzGruntSprites(false);
                return;
            }
        } else if (onMoveTile != 0) {
            if (flags & 0x100) {
                if (m_coordToggle == false) {
                    RunMoveConfig(
                        m_lastTilePx.m_x >> TILE_SHIFT_PX,
                        m_lastTilePx.m_y >> TILE_SHIFT_PX
                    );
                    return;
                }
            } else if (m_coordToggle != false) {
                RunMoveConfig(m_lastTilePx.m_x >> TILE_SHIFT_PX, m_lastTilePx.m_y >> TILE_SHIFT_PX);
                return;
            }
        }

        if ((flags & IDX(CELL_FLAG_SPECIAL)) == 0 || m_wingzEnabled != false) {
            i32 mask = m_arrivalFlags & flags;
            if (!(mask & BRICKZ_CELL_OCCUPIED)) {
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
            } else if (cx >= level->m_mainPlane->m_tileColumns) {
                cx = level->m_mainPlane->m_tileColumns - 1;
            }
            i32 cy = pty;
            if (cy < 0) {
                cy = 0;
            } else if (cy >= level->m_mainPlane->m_tileRows) {
                cy = level->m_mainPlane->m_tileRows - 1;
            }
            i32 raw =
                level->m_mainPlane->m_tileHandles[level->m_mainPlane->m_tileRowOffsets[cy] + cx];
            TileCollisionKind kind;
            if (raw == UNINIT_FILL || raw == -1) {
                kind = TILEKIND_PASSABLE;
            } else {
                CTileImageSet* ts = static_cast<CTileImageSet*>(
                    level->m_imageSets.GetAt(raw & WWD_TILE_IMAGE_SET_INDEX_MASK)
                );
                kind = ts->GetCollisionAt(0, 0);
            }

            b32 gate = true;
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
                    hazard = static_cast<GruntDeathType>(cx);
                    gate = false;
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
                            gate = false;
                        }
                    }
                    break;
                }
                case TILEKIND_DEATH:
                case TILEKIND_DEATHBRIDGE_UP:
                case TILEKIND_TOGGLEDEATHBRIDGE_UP:
                    hazard = g_areaPitDeath;
                    break;
            }
            if (gate != false) {
                m_triggerMgr->StartUnitDeath(m_playerIndex, m_unitIndex, hazard, -1);
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
            if (m_wingzEnabled != false) {
                goto afterTile;
            }
            if (m_gruntKind == GRUNT_INVULNERABLE) {
                goto afterTile;
            }
            if (m_entranceReason == PICKUP_BOMB) {
                bool nameDiffers = ANIMATION_ACT_DIFFERS("M");
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

                if (reg3->m_isEasyMode != false && reg3->m_gameMode == GAMEMODE_QUESTZ) {
                    i32 bite = m_health - 5;
                    hp = (bite < 0) ? 0 : bite;
                } else {
                    i32 bite = m_health - 0xa;
                    hp = (bite < 0) ? 0 : bite;
                }
                m_health = hp;
                if (hp <= 0) {
                    ConsiderArrival(1);
                    m_triggerMgr->StartUnitDeath(m_playerIndex, m_unitIndex, DEATH_NORMAL, -1);
                    return;
                }
            }
            {
                CWwdSpriteObject* obj = m_object;
                CGruntzMgr* reg3 = g_gameReg;
                i32 sy = obj->m_screenY;
                i32 sx = obj->m_screenX;
                const RECT* vr = &reg3->m_world->m_level->m_mainPlane->m_planeViewRect;
                if (::PtInRect(vr, sx, sy)) {
                    reg3->m_voiceManager->PlayVoice(this, 0x348, -1, 0, -1, -1);
                }
            }
            m_entranceSafeTimeLo = 0x3e8;
            m_entranceSafeTimeHi = 0;
            m_entranceClockLo = static_cast<i32>(g_frameTime);
            m_entranceClockHi = 0;
        } else if (flags & 0x2000000) {
            if (m_entranceReason == PICKUP_TOOB) {
                CString* node = g_typeColl.ScratchResolve(m_logicRecord->m_eventCode);
                CString* slot = g_typeColl.Slots();
                i32 n = g_typeColl.m_grown;
                while (n-- != 0) {
                    if (slot != NULL) {
                        slot->CString::CString();
                    }
                    slot++;
                }
                bool nameDiffers = (strcmp(*node, "N") != 0);
                if (nameDiffers) {
                    BuildGruntLoseItemAnimation();
                }
            }
        }
    }

afterTile:
    if (m_entranceActive == false && m_entranceCommitted != false) {
        CWwdSpriteObject* obj = m_object;
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
            if (!IsHoldPending()) {
                switch (m_arrivalState) {
                    case AI_DUMBCHASER:
                        StepDumbChaserBehavior();
                        break;
                    case AI_SMARTCHASER:
                        StepSmartChaserBehavior();
                        break;
                    case AI_HITANDRUNNER:
                        StepHitAndRunnerBehavior();
                        break;
                    case AI_DEFENDER:
                        StepDefenderBehavior();
                        break;
                    case AI_POSTGUARD:
                        StepPostGuardBehavior();
                        break;
                    case AI_OBJECTGUARD:
                        StepObjectGuardBehavior();
                        break;
                    case AI_BOMBER:
                        StepBomberBehavior();
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
                        StepGauntletGruntBehavior();
                        break;
                    case AI_TIMEBOMBER:
                        StepTimeBomberBehavior();
                        break;
                    case AI_TOOLTHIEF:
                        StepToolThiefBehavior();
                        break;
                    case AI_TOYER:
                        StepToyerBehavior();
                        break;
                    case AI_MAGICWANDGRUNT:
                        StepMagicWandGruntBehavior();
                        break;
                    case AI_SCROLLGRUNT:
                        StepScrollGruntBehavior();
                        break;
                }
            }
        } else if (m_poweredUp != false && m_neighborValid == false && m_combatActive == false
                   && m_stamina >= STAMINA_FULL && m_neighborScanEnabled != false) {
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
        i64 durationMinusNow = m_toyDuration - static_cast<i64>(g_frameTime);
        i64 left = durationMinusNow + m_toyClock;
        m_toyTime = static_cast<i32>(
            static_cast<double>((left < 0 ? 0 : static_cast<u32>(left)))
                / static_cast<double>(static_cast<u32>(m_toyDurationLo)) * g_wingzScale
            - g_wingzBias
        );
        i64 left2 = m_toyDuration - static_cast<i64>(g_frameTime) + m_toyClock;
        if (static_cast<u32>((left2 < 0 ? 0 : static_cast<u32>(left2))) == 0) {
            m_toyTime = 0;
            HIDE_AND_CLEAR_GRUNT_SPRITE(m_toyTimeSprite)
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
            HIDE_AND_CLEAR_GRUNT_SPRITE(m_staminaSprite)
        }
    }

    if (m_wingzEnabled != false) {
        i64 left = m_wingzDuration64 - static_cast<i64>(g_frameTime) + m_wingzClock64;
        m_wingzTime = static_cast<i32>(
            static_cast<double>((left < 0 ? 0 : static_cast<u32>(left)))
            * DATA_COMPGEN(0x001e9a58, 0.01) - g_wingzBias
            );
        i64 left2 = m_wingzDuration64 - static_cast<i64>(g_frameTime) + m_wingzClock64;
        if (static_cast<u32>((left2 < 0 ? 0 : static_cast<u32>(left2))) == 0) {
            ConsiderArrival(1);
            m_wingzTime = 0;
            LoadWingzGruntSprites(false);
            BuildGruntLoseItemAnimation();
        }
    }

    if (m_arrivalState == AI_BATTLEZ_PATH) {
        if (m_poweredUp != false && m_stamina >= STAMINA_FULL) {
            bool eq;
            {
                CString* node = g_typeColl.ScratchResolve(m_logicRecord->m_eventCode);
                CString* slot = g_typeColl.Slots();
                i32 n = g_typeColl.m_grown;
                while (n-- != 0) {
                    if (slot != NULL) {
                        slot->CString::CString();
                    }
                    slot++;
                }
                eq = (strcmp(*node, "E") == 0);
            }
            if (!eq) {
                CString* node = g_typeColl.ScratchResolve(m_logicRecord->m_eventCode);
                CString* slot = g_typeColl.Slots();
                i32 n = g_typeColl.m_grown;
                while (n-- != 0) {
                    if (slot != NULL) {
                        slot->CString::CString();
                    }
                    slot++;
                }
                eq = (strcmp(*node, "A") == 0);
            }
            if (eq) {
                if (m_poweredUp != false && m_neighborValid == false) {
                    RESET_GRUNT_POWERED_STATE(this)
                }
            }
        }
    } else {
        if (static_cast<i64>(g_frameTime) - m_combatClock64 >= m_combatTimeout64) {
            if (m_poweredUp != false && m_neighborValid == false) {
                RESET_GRUNT_POWERED_STATE(this)
            }
            if (m_arrived == false
                && static_cast<i64>(g_frameTime) - m_hudRetireClock64 >= m_hudRetireWindow64) {
                HIDE_AND_CLEAR_GRUNT_SPRITE(m_healthSprite)
                HIDE_AND_CLEAR_GRUNT_SPRITE(m_toySprite)
                HIDE_AND_CLEAR_GRUNT_SPRITE(m_staminaSprite)
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
                m_triggerMgr->StartUnitDeath(m_playerIndex, m_unitIndex, DEATH_NORMAL, -1);
                return;
            }
            m_convertTimeLo =
                static_cast<i32>(g_buteMgr.GetDword("Powerupz", "ConversionTime", 0x1f4));
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
                CWwdSpriteObject* obj = m_object;
                ShadeMode cmd = obj->m_drawFillCmd;
                SET_DRAW_FILL(obj, cmd, sel);
            }
        }
        i64 left = m_convertTime64 + m_convertClock64 - static_cast<i64>(g_frameTime);
        i32 leftMs = (left < 0 ? 0 : static_cast<i32>(left));
        if (leftMs <= 0xbb8) {
            if (m_gruntKind == GRUNT_GHOST) {

                i64 rem = m_convertTime64 + m_convertClock64 - static_cast<i64>(g_frameTime);
                u32 remMs = (rem < 0 ? 0 : static_cast<u32>(rem));
                i32 frac = static_cast<i32>(
                    static_cast<double>(
                        g_buteMgr.GetInt("Powerupz", "GruntGhostTransparencyOn", 0x100)
                    )
                    * static_cast<double>(remMs) * DATA_COMPGEN(0x001e9a60, 0.0003333333333333333)
                    );
                CWwdSpriteObject* obj = m_object;
                SET_DRAW_FILL_FRACTION(obj, SHADE_PAL_ALPHA_16, frac);
            } else {
                CWwdSpriteObject* obj = m_object;
                if (!HAS(obj->m_stateFlags, SPRITE_STATE_FLASHING)) {
                    obj->m_flashInterval = 0x7d;
                    obj->m_flashCountdown = 0;
                    m_object->m_stateFlags |= SPRITE_STATE_FLASHING;
                }
            }
            if (leftMs == 0) {
                switch (m_gruntKind) {
                    case PICKUP_GHOST: {
                        CWwdSpriteObject* obj = m_object;
                        m_gruntKind = GRUNT_NORMAL;
                        obj->m_drawActive = true;
                        obj->m_drawFillCmd = SHADE_PAL_16;
                        break;
                    }
                    case PICKUP_INVULNERABILITY:
                        m_gruntKind = GRUNT_NORMAL;
                        break;
                    case PICKUP_SUPERSPEED:
                    case PICKUP_ROIDZ:
                    case PICKUP_REACTIVEARMOR: {
                        CWwdSpriteObject* ps = m_powerupSprite;
                        m_gruntKind = GRUNT_NORMAL;
                        if (ps != NULL) {
                            ps->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);
                            m_powerupSprite = NULL;
                        }
                        break;
                    }
                    case PICKUP_DEATHTOUCH: {
                        CWwdSpriteObject* ps = m_powerupSprite;
                        m_gruntKind = GRUNT_NORMAL;
                        if (ps != NULL) {
                            ps->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);
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

    if (m_pendingTrigger != false && m_stamina >= STAMINA_FULL) {
        m_triggerMgr->UseEquippedToolAt(
            m_playerIndex,
            m_unitIndex,
            m_pendingTriggerPx.m_x,
            m_pendingTriggerPx.m_y
        );
        m_pendingTrigger = false;
    }
}

// @early-stop
RVA(0x0005ecd0, 0x4f3)
void CGrunt::FinalizeStep(char* name) {
    CUserLogic::FinalizeStep(name);
    AdvanceMotion();
    if (m_vehicleLoopSound != NULL) {
        bool neL = ANIMATION_ACT_DIFFERS("L");
        if (neL) {
            bool neG = ANIMATION_ACT_DIFFERS("G");
            if (neG) {
                StopVehicleLoopSound();
            }
        }
    }
    if (m_powerupLoopSound != NULL) {
        if (m_gruntKind == GRUNT_NORMAL) {
            StopPowerupLoopSound();
        } else {
            CGruntzMgr* g = g_gameReg;
            i32 y = m_object->m_screenY;
            i32 x = m_object->m_screenX;
            if (!::PtInRect(&g->m_viewBounds, x, y)) {
                StopPowerupLoopSound();
            }
        }
    }
    bool eqO = ANIMATION_ACT_EQUALS("O");
    if (eqO && (GRUNT_NOT_AT_SAVED_SCREEN_POS(this))) {
        GruntDirectionCell c = m_entranceCell;
        i32 row = c.row;
        switch (row) {
            case GRUNT_DIRECTION_GRID_LOW:
                row = GRUNT_DIRECTION_GRID_HIGH;
                break;
            case GRUNT_DIRECTION_GRID_HIGH:
                row = GRUNT_DIRECTION_GRID_LOW;
                break;
            default:
                break;
        }
        i32 column = c.column;
        switch (column) {
            case GRUNT_DIRECTION_GRID_LOW:
                column = GRUNT_DIRECTION_GRID_HIGH;
                break;
            case GRUNT_DIRECTION_GRID_HIGH:
                column = GRUNT_DIRECTION_GRID_LOW;
                break;
            default:
                break;
        }
        i32 base = GRUNT_DIRECTION_GRID_WIDTH * row + column;
        double d48 = m_cells[base].m_motion.m_direction.x;
        double d50 = m_cells[base].m_motion.m_direction.y;
        m_movePosX = static_cast<double>(g_frameDelta) * d48 * m_moveSpeed + m_movePosX;
        m_movePosY = static_cast<double>(g_frameDelta) * d50 * m_moveSpeed + m_movePosY;
        i32 nx = static_cast<i32>((m_cells[base].m_motion.m_step.x + m_movePosX));
        i32 ny = static_cast<i32>((m_cells[base].m_motion.m_step.y + m_movePosY));
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
        CWwdSpriteObject* h = m_object;
        i32 v = h->m_screenY + 0x186a0;
        SET_SORT_KEY_IF_CHANGED(h, v)
        return;
    }

    CString* rec = g_typeColl.ScratchResolve(m_logicRecord->m_eventCode);
    ActNameConstructGrownSlots();
    bool eqPos = (strcmp(*rec, "S") == 0);
    if (eqPos) {
        if (GRUNT_AT_SAVED_SCREEN_POS(this)) {
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
        eq = ANIMATION_ACT_EQUALS("A");
        if (eq && CoordCount() != 0) {
            CoordNode* head = CoordHead();
            Coord* co = head->m_coord;
            i32 fl = g_gameReg->m_tileGrid->m_rowInts[co->m_y][co->m_x * 7];
            if (!(fl & BRICKZ_CELL_OCCUPIED) && !((m_arrivalFlags & fl) & BRICKZ_CELL_OCCUPIED)
                && ((m_arrivalFlags & fl) == 0 || (m_passableMask & fl) != 0)) {
                Coord* tc = (CoordTail())->m_coord;
                SET_TILE_CENTER_PIXEL_PAIR(m_entrancePx.m_x, m_entrancePx.m_y, tc->m_x, tc->m_y)
                m_coordRetryCount = 0;
                StepEntranceReinit();
            } else if (static_cast<u32>(m_coordRetryCount) <= 5) {
                if (PathScan() != 0) {
                    Coord* h2 = (CoordTail())->m_coord;
                    SET_TILE_CENTER_PIXEL_PAIR(m_entrancePx.m_x, m_entrancePx.m_y, h2->m_x, h2->m_y)
                    if (CoordCount() != 0) {
                        Coord* h3 = (CoordHead())->m_coord;
                        i32 fl2 = g_gameReg->m_tileGrid->m_rowInts[h3->m_y][h3->m_x * 7];
                        if (!(fl2 & BRICKZ_CELL_OCCUPIED)) {
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

    CString* code = g_typeColl.ScratchResolve(m_logicRecord->m_eventCode);
    ActNameConstructGrownSlots();
    bool different = strcmp(*code, "D");
    if (different) {
        code = g_typeColl.ScratchResolve(m_logicRecord->m_eventCode);
        ActNameConstructGrownSlots();
        different = strcmp(*code, "N");
        if (different) {
            code = g_typeColl.ScratchResolve(m_logicRecord->m_eventCode);
            ActNameConstructGrownSlots();
            different = strcmp(*code, "L");
            if (different) {
                code = g_typeColl.ScratchResolve(m_logicRecord->m_eventCode);
                ActNameConstructGrownSlots();
                different = strcmp(*code, "M");
                if (different) {
                    return;
                }
                if (m_bombRunActive != false) {
                    return;
                }
            } else if (m_entranceStamped != false) {
                return;
            }
        }
    }
    if (GRUNT_AT_SAVED_SCREEN_POS(this)) {
        if (m_arrivalPending != false) {
            m_triggerMgr->WireTileSwitchLogic(this, m_lastTilePx.m_x, m_lastTilePx.m_y);
            m_arrivalPending = false;

            if (m_arrivalPhase != ARRIVAL_TAG_NONE) {
                i32 result = -1;
                if (m_arrivalPhase == ARRIVAL_TAG_TRIGGER_A) {
                    if (m_arrivalActive != false) {
                        CGrunt* other =
                            m_triggerMgr->m_units
                                [m_arrivalCell.m_x * TM_UNITS_PER_PLAYER + m_arrivalCell.m_y];
                        if (other != NULL) {
                            i32 otherPxX = other->m_object->m_screenX;
                            i32 otherPxY = other->m_object->m_screenY;
                            i32 x = (otherPxX & ~TILE_MASK_PX) + TILE_HALF_PX;
                            i32 y = (otherPxY & ~TILE_MASK_PX) + TILE_HALF_PX;
                            if (m_defenderPx.m_x != x || m_defenderPx.m_y != y) {
                                m_defenderPx.m_x = x;
                                m_defenderPx.m_y = y;
                                if (StepArrivalDrop(x, y, ARRIVAL_TAG_TRIGGER_A, -1, 1, 0)
                                    == ARRIVAL_TAG_NONE) {
                                    m_arrivalPhase = ARRIVAL_TAG_NONE;
                                }
                            }

                            i32 lastX = other->m_lastTilePx.m_x;
                            i32 lastY = other->m_lastTilePx.m_y;
                            i32 targetX = lastX;
                            i32 targetY = lastY;
                            if (RectContains(x, y) != 0) {
                                targetX = otherPxX;
                                targetY = otherPxY;
                            } else if (RectContains(lastX, lastY) != 0) {
                                other->SnapToLastTile(0);
                            } else {
                                targetX = m_arrivalTargetPx.m_x;
                                targetY = m_arrivalTargetPx.m_y;
                            }
                            result = m_triggerMgr->UseEquippedToolAt(
                                m_playerIndex,
                                m_unitIndex,
                                targetX,
                                targetY
                            );
                        } else {
                            result = 0;
                        }
                    } else {
                        result = m_triggerMgr->UseEquippedToolAt(
                            m_playerIndex,
                            m_unitIndex,
                            m_arrivalTargetPx.m_x,
                            m_arrivalTargetPx.m_y
                        );
                    }
                } else if (m_arrivalPhase == ARRIVAL_TAG_TRIGGER_B) {
                    if (m_arrivalActive != false) {
                        CGrunt* other =
                            m_triggerMgr->m_units
                                [m_arrivalCell.m_x * TM_UNITS_PER_PLAYER + m_arrivalCell.m_y];
                        if (other != NULL) {
                            i32 otherPxX = other->m_object->m_screenX;
                            i32 otherPxY = other->m_object->m_screenY;
                            i32 x = (otherPxX & ~TILE_MASK_PX) + TILE_HALF_PX;
                            i32 y = (otherPxY & ~TILE_MASK_PX) + TILE_HALF_PX;
                            if (m_defenderPx.m_x != x || m_defenderPx.m_y != y) {
                                m_defenderPx.m_x = x;
                                m_defenderPx.m_y = y;
                                if (StepArrivalDrop(x, y, ARRIVAL_TAG_TRIGGER_B, -1, 1, 0)
                                    == ARRIVAL_TAG_NONE) {
                                    m_arrivalPhase = ARRIVAL_TAG_NONE;
                                }
                            }

                            i32 lastX = other->m_lastTilePx.m_x;
                            i32 lastY = other->m_lastTilePx.m_y;
                            i32 targetX = lastX;
                            i32 targetY = lastY;
                            if (VehicleContactContains(x, y) != 0) {
                                targetX = otherPxX;
                                targetY = otherPxY;
                            } else if (VehicleContactContains(lastX, lastY) != 0) {
                                other->SnapToLastTile(0);
                            } else {
                                targetX = m_arrivalTargetPx.m_x;
                                targetY = m_arrivalTargetPx.m_y;
                            }
                            result = m_triggerMgr
                                         ->UseToyAt(m_playerIndex, m_unitIndex, targetX, targetY);
                        } else {
                            result = 0;
                        }
                    } else {
                        result = m_triggerMgr->UseToyAt(
                            m_playerIndex,
                            m_unitIndex,
                            m_arrivalTargetPx.m_x,
                            m_arrivalTargetPx.m_y
                        );
                    }
                }

                if (result == 0 || result == 1) {
                    if (result == 1) {
                        SetEntrancePos(1, 1);
                        return;
                    }
                    m_arrivalPhase = 0;
                }
            }
        }

        CString* rec = ActNameLookupCallReport(m_logicRecord->m_eventCode);
        ActNameConstructGrownSlots();
        bool hit = (strcmp(*rec, "N") == 0);
        if (hit) {
            return;
        }
        rec = ActNameLookupCallReport(m_logicRecord->m_eventCode);
        ActNameConstructGrownSlots();
        hit = (strcmp(*rec, "L") == 0);
        if (hit) {
            if (StepCompassMove() != 0) {
                return;
            }
            m_toyDuration = 0;
            return;
        }
        rec = ActNameLookupCallReport(m_logicRecord->m_eventCode);
        ActNameConstructGrownSlots();
        hit = (strcmp(*rec, "M") == 0);
        if (hit) {
            if (ClaimSwitchTile() != 0) {
                return;
            }
            m_triggerMgr->StartUnitDeath(m_playerIndex, m_unitIndex, DEATH_NORMAL, -1);
            return;
        }
        Coord entrance = EntrancePx();
        if (m_lastTilePx.m_x == entrance.m_x && m_lastTilePx.m_y == entrance.m_y) {
            m_arrivalPhase = 0;
            ResetEntranceAnimation(1, 0, 0);
            return;
        }
        if (StepGruntMovement() == 0) {
            return;
        }
    }

    double dirX = EntranceCell()->m_motion.m_direction.x;
    double dirY = EntranceCell()->m_motion.m_direction.y;
    m_movePosX = static_cast<double>(g_frameDelta) * dirX * m_moveSpeed + m_movePosX;
    m_movePosY = static_cast<double>(g_frameDelta) * dirY * m_moveSpeed + m_movePosY;
    i32 x = static_cast<i32>(EntranceCell()->m_motion.m_step.x + m_movePosX);
    i32 y = static_cast<i32>(EntranceCell()->m_motion.m_step.y + m_movePosY);
    if (dirX > s_fpZero) {
        if (x > m_lastTilePx.m_x) {
            x = m_lastTilePx.m_x;
        }
    } else if (dirX < s_fpZero && x < m_lastTilePx.m_x) {
        x = m_lastTilePx.m_x;
    }
    if (dirY > s_fpZero) {
        if (y > m_lastTilePx.m_y) {
            y = m_lastTilePx.m_y;
        }
    } else if (dirY < s_fpZero && y < m_lastTilePx.m_y) {
        y = m_lastTilePx.m_y;
    }
    m_object->m_screenX = x;
    m_object->m_screenY = y;
    CWwdSpriteObject* o = m_object;
    i32 sortKey = o->m_screenY + 0x186a0;
    SET_SORT_KEY_IF_CHANGED(o, sortKey)
}
