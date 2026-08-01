#include <Mfc.h>            // the REAL MFC CPtrList - CScanList was a fake view of it
#include <Rez/FrameClock.h> // frame-clock band (g_frameDelta/g_frameTime/g_killCueClock/g_engineFrameDelta)
#include <Gruntz/GruntSpawnConfig.h> // the +0x60 cue-sink/spawn-config object (complete type for the cue calls)
#include <Gruntz/GruntzMapMgr.h>  // the real +0x70 board class (ex GruntBoard view)
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/TraitorMode.h> // g_traitorMode
// GruntCombat.cpp - the THIRD original grunt TU (retail text 0x56f80-0x5d084):
// the grunt combat / struck-voice / attack / ability-tuning / spawn family,
// carved out of the conflated Grunt.cpp (grunt-region partition).
//
// original TU: filename unknown (@identity-TODO; named for the dominant combat
// family). ONE-obj evidence:
//   * private .data extents in TU link order: LoadGruntAbilityTuning @0x57100's
//     8 cells (0x20dc64-0x20dd30) sit BETWEEN StepCompassMove's (0x20dbf8, the
//     GruntSteps TU) and BuildGruntLoseItemAnimation @0x57890's (0x20dd40),
//     followed by LoadGruntCombatAnimations @0x597a0's 15 cells (0x20dd4c-
//     0x20df6c) - one contiguous band.
//   * init frags i324-i342 (gruntcombatanim x9 @0x58f60, grunt x9 @0x5b820,
//     logicactregistrars @0x5bc30) are one contiguous CRT-table run at frag RVAs
//     inside this interval.
//   * 4 EH sites in the interval -> /GX (flags "eh").
// In-interval folds: LoadGruntAbilityTuning @0x57100 (ex GruntAssetLoaders.cpp),
// PathScan @0x57db0 (ex GruntPathScan.cpp), LoadGruntCombatAnimations
// @0x597a0 (ex GruntCombatAnim.cpp), GruntSpawnPump @0x5baf0 (ex
// GruntSpawnPump.cpp), ConstructActRange_644af0 @0x5bc50 + RegisterActs_644af0
// @0x5be30 (LogicActRegistrars). NOT folded (COMDAT-at-usage emissions):
// ApplyGeometryDirect @0x58b60 (spriteresource),
// CMotionState::SetParams/SetZ @0x58bc0/0x58ca0 (motionstate), ??0CUserLogic
// @0x58cd0 (userlogicctoremit), CPairRecord::Serialize @0x58ee0
// (trirecordserialize), Lookup @0x5b7e0 (ddrawsubmgrleafscan).
#include <Gruntz/Grunt.h>
#include <DDrawMgr/DDrawSurfaceMgr.h> // the m_0c world root (m_animRegistry hop)
#include <DDrawMgr/DDrawSubMgrLeaf.h> // m_0c->m_animRegistry (the anim-key catalog)
#include <Gruntz/GameLevel.h> // canonical CGameLevel/CDDrawWorkerHost (m_world->m_level visible rect)
#include <Gruntz/ActReg.h>    // CActReg::ResolveEntry
#include <Gruntz/AniElement.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/SerialRecords.h>
#include <Gruntz/MovingLogicSerial.h>
#include <Gruntz/BoundaryLowerMethodsViews.h>
#include <Dsndmgr/DirectSoundMgr.h>
#include <Dsndmgr/DirectSoundMgr.h>
#include <rva.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <Bute/ButeMgr.h>
#include <Bute/ButeTree.h>
#include <Gruntz/WorkerHandler.h> // Owner/Worker + Worker_DefaultPump (GruntSpawnPump)
#include <Wap32/Rect.h> // canonical CRect: the 0x29ac0 direct-store ctor (ex the CScanRectInit Set34a4 carrier view)
#include <new>             // placement CRect ctor  // the PathScan dirty-rect Set34a4 helper
#include <Gruntz/Brickz.h> // canonical CMapMgr (SearchEdge)
#include <Gruntz/BattlezMapConfig.h> // CBattlezMapConfig (the coord-list walk is CoordListOps now)
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/LightFx.h> // CLightFx::Activate (spell LightFx sprites; folded CSpriteRegistrar)
#include <DDrawMgr/DDrawSubMgrLeafScan.h> // CDDrawSubMgrLeafScan::Lookup (rehomed here)
#include <Gruntz/GameRegistry.h> // CDDrawSurfaceMgr - the worker's m_0c owner-context facet
#include <Gruntz/LeafCue.h>      // LeafCue - the launch-sound cue entries
#include <Gruntz/SoundCue.h> // CDDrawSubMgrLeafScan (typedef of CDDrawSubMgrLeafScan) - the cue registry
#include <Gruntz/TriggerMgr.h> // CTriggerMgr - the CGrunt+0x260 board
#include <new>
#include <Gruntz/GruntEntranceArrival.h> // ex Globals.h
#include <Gruntz/SoundState.h>           // ex Globals.h transitive
#include <Gruntz/FreeNodePool.h>         // the coord-node pool object @0x645540
#include <Gruntz/GruntCombat.h>          // CActRegPool<CGrunt>::s_table decl
#include <Utils/MapTyped.h> // typed MFC map lookups (the forced void*& pun at one boundary)
#pragma intrinsic(strcmp, sqrt)

static const char s_GRUNTZ_[] = "GRUNTZ_";
static const char s__MOVING[] = "_MOVING";
static const char s__DEATH[] = "_DEATH";
static const char s__JOY[] = "_JOY";
static const char s__IDLE[] = "_IDLE";
static const char s__BATTLECRY[] = "_BATTLECRY";
DATA(0x0020dd40)
static const char s__LOSEITEM[] = "_LOSEITEM";
DATA(0x0020a680)
static const char s_SingleAnimation[] = "SingleAnimation";
static const char s_keyB[] = "B";
static const char s_keyC[] = "C";
static const char s_keyE[] = "E";
static const char s_keyA[] = "A";
static const char s_keyF[] = "F";

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

// CActRegPool<CGrunt>::s_table (0x00244af0): CActReg - no provable static init (the type has no
// default ctor / is runtime-Init'd), so the datum is named by symbol.
template<> DATA(0x00244af0)
CActReg CActRegPool<CGrunt>::s_table(2000, 2010);

static char s_TimePerTile[] = "TimePerTile";
static char s_Grunt[] = "Grunt";                               // s_Grunt_0060a9ec
static char s_EntranceSafeTime[] = "EntranceSafeTime";         // s_EntranceSafeTime_0060df98
static char s_IdleDelay[] = "IdleDelay";                       // s_IdleDelay_0060e1a0
static char s_PlayerDefenderRadius[] = "PlayerDefenderRadius"; // s_PlayerDefenderRadius_0060e1ac
static char s_CombatTimeout[] = "CombatTimeout";               // s_CombatTimeout_0060df84

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

enum SpellzEffect {
    SPELLZ_FREEZE = 1,       // FreezeRadius
    SPELLZ_HEALTH = 2,       // HealthRadius
    SPELLZ_RESURRECTION = 3, // RessurectionRadius
    SPELLZ_TOYZ = 4,         // ToyzRadius
    SPELLZ_TELEPORT = 5,     // TeleportRadius
    SPELLZ_ROLLINGBALL = 6,  // RollingBallzSpeed/Time (spawns 4 directional ballz)
};

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

// ===========================================================================
// The 5 grunt movement / anim-name dispatch state machines (formerly the
// CUserLogic_* stubs @0x4b370 / 0x4c170 / 0x52fb0 / 0x5f310 / 0x6a6d0). Each
// resolves the grunt's current anim-set node name
// (g_typeColl.GetNameRecord(m_objAux->m_1c), or the scratch-teardown
// ScratchResolve form) and dispatches on its single-letter type code
// (A/D/I/G/L/P/O/Q/J/N/M/K), driving the grunt's movement/arrival state, recycling
// its occupied-coord nodes onto the shared freelist, and re-latching m_objAux->m_1c to
// a new anim set via g_entranceAnimSrc.LookupAnimSet. The inline-strcmp `== bool` setcc
// reject form is per docs/patterns/strcmp-eq-bool-local-setcc.md.
//
// These are the CGrunt analogues of CBattlezMapConfig::StepBoard /
// ChooseIdleBehavior (the documented large-state-machine + grid-regalloc walls). Each is
// reconstructed complete in shape/order; all carry @early-stop on those walls.
// Raw-offset member access (the campaign style used by the cluster above) keeps the
// giant ~0x46c layout tractable.

static inline void GruntScratchTeardown() {
    CString* slot = (g_typeColl.Slots());
    i32 cnt = g_typeColl.m_grown;
    while (cnt != 0) {
        if (slot != 0) {
            slot->~CString();
        }
        slot++;
        cnt--;
    }
}

#define LK(key)                                                                                    \
    do {                                                                                           \
        LeafCue* out = 0;                                                                          \
        MapLookup(g_gameReg->m_world->m_soundRegistry->m_10, (key), out);                          \
        cue = out;                                                                                 \
    } while (0)

#define SETDIR(k, nx, ny)                                                                          \
    do {                                                                                           \
        this->m_entranceCell.col = g_dirVec[k][0];                                                 \
        this->m_entranceCell.row = g_dirVec[k][1];                                                 \
        this->m_entranceCell.reason = g_dirVec[k][2];                                              \
        newX = (nx);                                                                               \
        newY = (ny);                                                                               \
    } while (0)

// (CGruntCombat is GONE - it was CGrunt itself: the +0x31c occupied-coord CPtrList,
//  the +0x400/+0x408/+0x410 knockback doubles and every F()/P() offset it bagged are
//  CGrunt members at the identical offsets (Grunt.h). LoadGruntCombatAnimations is
//  declared there; the "enemy" grid elements below are placed CGruntz.)
// CGrunt::EntranceTileOffset(out) @0x56f80 - the pixel position of the tile adjacent
// to the grunt's last occupied tile (m_lastTilePxX/Y) in the entrance-cell direction
// (m_entranceCell.reason, a 1..8 compass code: 1=N, 2=NE, 3=E, 4=SE, 5=S, 6=SW, 7=W, 8=NW;
// any other value leaves the position unchanged). One tile step is 0x20 px. Writes the
// (x, y) pair through `out`. __thiscall, ret 4.
// @early-stop
RVA(0x00056f80, 0x8e)
void CGrunt::EntranceTileOffset(i32* out) {
    i32 x = m_lastTilePxX;
    i32 y = m_lastTilePxY;
    switch (m_entranceCell.reason) {
        case 1:
            y -= 0x20;
            break;
        case 2:
            x += 0x20;
            y -= 0x20;
            break;
        case 3:
            x += 0x20;
            break;
        case 4:
            x += 0x20;
            y += 0x20;
            break;
        case 5:
            y += 0x20;
            break;
        case 6:
            x -= 0x20;
            y += 0x20;
            break;
        case 7:
            x -= 0x20;
            break;
        case 8:
            x -= 0x20;
            y -= 0x20;
            break;
    }
    out[0] = x;
    out[1] = y;
}

// Re-clip the board dirty rect to the whole board. Retail 0x581ce/0x5813d/0x585db:
// the clip rect is a real CRect LOCAL (direct ctor), the source rect a CRect
// TEMPORARY sliced into a RECT (ctor into a scratch slot + a four-dword copy). No
// placement new anywhere - the earlier `new (&r) CRect(..)` spelling emitted a null
// test per rect that retail does not have (27 spurious basic blocks).
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

// The 0x58097 variant: same thing, but there the CLIP rect is four inline stores
// rather than a CRect ctor call.
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
    double dx = static_cast<double>(m_lastTilePxX) - static_cast<double>(h->m_screenX);
    double dy = static_cast<double>(m_lastTilePxY) - static_cast<double>(h->m_screenY);
    // retail zero-extends m_timePerTile to 64-bit before fild (unsigned->double)
    m_moveSpeed =
        (sqrt(dx * dx + dy * dy) / static_cast<double>(static_cast<u32>(m_timePerTile))) * dt;
    m_408 = static_cast<double>(h->m_screenX);
    m_410 = static_cast<double>(h->m_screenY);
}

static inline CString* ActNameSlots() {
    return g_typeColl.Slots();
}

// The shared tail of every 0x5be30 block: bind `handler` into the pool at `id`.
// CGrunt is MI, so `&CGrunt::Handler` is the 8-byte {code, adjust} member pointer
// while the table slot is the 4-byte single-inheritance form retail stores; the two
// readings are named on GruntActPmf (<Gruntz/Grunt.h>), so no pun is needed here.
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
            /* the name-slot lookup reads the GLOBAL, not `id`: retail CSEs the reload */          \
            /* across two consumers (`push eax; mov edi,eax`) - feeding it `id` gives   */         \
            /* the load one consumer and cl coalesces it into edi, dropping the copy.   */         \
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

// The variant the devs wrote for the LAST key of 0x5be30: the slot comes from the DERIVED
// accessor _zdvec::IndexToPtr (0x310f0, via ILT 0x437c), whose body ALREADY contains the
// per-slot CString fixup loop @0x31156 - so the loop is not open-coded here, and the arg
// is `id` (retail loads g_typeCounter once into callee-saved esi and pushes esi: one
// consumer, versus the `push eax; mov edi,eax` two-consumer form above).
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
    i32 idx = forced;
    if (forced == 0) {
        i32 m = 3;
        if (g_gameReg->m_134 != 1) {
            m = 6;
        }
        if (m == 0) {
            idx = GruntRand() & 1;
        } else {
            idx = GruntRand() % m + 1;
        }
    }

    // the sprite's worker -> owner context (CDDrawSurfaceMgr facet) -> cue host
    CDDrawSubMgrLeafScan* slot =
        (static_cast<CDDrawSurfaceMgr*>(m_3c->m_ownerCtx))->m_soundRegistry;
    if (slot->m_emitGate == 0) {
        LeafCue* sout = 0;
        MapLookup(slot->m_10, s_GAME_ATTACK,
                  sout); // CMapStringToPtr @0x1b8438
        if (sout != 0) {
            // retail reloads the looked-up cue into ecx and __thiscalls 0x1f940
            sout->PlayIfElapsed(g_sndCueTag, 0, 0, 0);
        }
    }

    switch (idx) {
        case SPELLZ_FREEZE: { // freeze
            CGameObject* spr =
                g_gameReg->m_world->m_childGroup
                    ->CreateSprite(0, m_lastTilePxX, m_lastTilePxY, 0xf4240, "LightFx", 0x40003);
            spr->m_7c->m_notify(spr);
            (static_cast<CLightFx*>(spr->m_7c->m_logic))
                ->Activate("GAME_LIGHTING_FLASH", "GAME_FLASH", 9, 1);
            return m_tileMgr->CombatCue(
                m_lastTilePxX,
                m_lastTilePxY,
                g_buteMgr.GetIntDef(s_Spellz, s_FreezeRadius, 8),
                4,
                -1
            );
        }
        case SPELLZ_HEALTH: { // health
            CGameObject* spr =
                g_gameReg->m_world->m_childGroup
                    ->CreateSprite(0, m_lastTilePxX, m_lastTilePxY, 0xf4240, "LightFx", 0x40003);
            spr->m_7c->m_notify(spr);
            (static_cast<CLightFx*>(spr->m_7c->m_logic))
                ->Activate("GAME_LIGHTING_FLASH", "GAME_FLASH", 2, 1);
            return m_tileMgr->CombatCue(
                m_lastTilePxX,
                m_lastTilePxY,
                g_buteMgr.GetIntDef(s_Spellz, s_HealthRadius, 8),
                3,
                -1
            );
        }
        case SPELLZ_RESURRECTION: { // resurrection
            CGameObject* spr =
                g_gameReg->m_world->m_childGroup
                    ->CreateSprite(0, m_lastTilePxX, m_lastTilePxY, 0xf4240, "LightFx", 0x40003);
            spr->m_7c->m_notify(spr);
            (static_cast<CLightFx*>(spr->m_7c->m_logic))
                ->Activate("GAME_LIGHTING_FLASH", "GAME_FLASH", 8, 1);
            return m_tileMgr->LoadGruntResurrectTuning(
                m_lastTilePxX,
                m_lastTilePxY,
                g_buteMgr.GetIntDef(s_Spellz, s_RessurectionRadius, 8)
            );
        }
        case SPELLZ_TOYZ: { // toyz
            CGameObject* spr =
                g_gameReg->m_world->m_childGroup
                    ->CreateSprite(0, m_lastTilePxX, m_lastTilePxY, 0xf4240, "LightFx", 0x40003);
            spr->m_7c->m_notify(spr);
            (static_cast<CLightFx*>(spr->m_7c->m_logic))
                ->Activate("GAME_LIGHTING_FLASH", "GAME_FLASH", 7, 1);
            return m_tileMgr->CombatCue(
                m_lastTilePxX,
                m_lastTilePxY,
                g_buteMgr.GetIntDef(s_Spellz, s_ToyzRadius, 8),
                5,
                -1
            );
        }
        case SPELLZ_TELEPORT: { // teleport
            CGameObject* spr =
                g_gameReg->m_world->m_childGroup
                    ->CreateSprite(0, m_lastTilePxX, m_lastTilePxY, 0xf4240, "LightFx", 0x40003);
            spr->m_7c->m_notify(spr);
            (static_cast<CLightFx*>(spr->m_7c->m_logic))
                ->Activate("GAME_LIGHTING_FLASH", "GAME_FLASH", 3, 1);
            return m_tileMgr->CombatCue(
                m_lastTilePxX,
                m_lastTilePxY,
                g_buteMgr.GetIntDef(s_Spellz, s_TeleportRadius, 8),
                2,
                -1
            );
        }
        case SPELLZ_ROLLINGBALL: { // rolling ball (4 directions)
            CWwdGameObjectA* n = g_gameReg->m_world->m_childGroup->CreateSprite(
                0,
                m_lastTilePxX,
                m_lastTilePxY - 0x20,
                0,
                "RollingBall",
                0x40003
            );
            n->ApplyName("LEVEL_ROLLINGBALL_NORTH");
            AnimWorkerObj* ni = n->m_7c;
            ni->m_bc =
                static_cast<i32>(g_buteMgr.GetDwordDef(s_Spellz, s_RollingBallzSpeed, 0x3e8));
            n->m_124 = 0;
            n->m_118 = static_cast<i32>(g_buteMgr.GetDwordDef(s_Spellz, s_RollingBallzTime, 0x3e8));

            CWwdGameObjectA* e = g_gameReg->m_world->m_childGroup->CreateSprite(
                0,
                m_lastTilePxX + 0x20,
                m_lastTilePxY,
                0,
                "RollingBall",
                0x40003
            );
            e->ApplyName("LEVEL_ROLLINGBALL_EAST");
            AnimWorkerObj* ei = e->m_7c;
            ei->m_bc =
                static_cast<i32>(g_buteMgr.GetDwordDef(s_Spellz, s_RollingBallzSpeed, 0x3e8));
            e->m_124 = 0;
            e->m_118 = static_cast<i32>(g_buteMgr.GetDwordDef(s_Spellz, s_RollingBallzTime, 0x3e8));

            CWwdGameObjectA* s = g_gameReg->m_world->m_childGroup->CreateSprite(
                0,
                m_lastTilePxX,
                m_lastTilePxY + 0x20,
                0,
                "RollingBall",
                0x40003
            );
            s->ApplyName("LEVEL_ROLLINGBALL_SOUTH");
            AnimWorkerObj* si = s->m_7c;
            si->m_bc =
                static_cast<i32>(g_buteMgr.GetDwordDef(s_Spellz, s_RollingBallzSpeed, 0x3e8));
            s->m_124 = 0;
            s->m_118 = static_cast<i32>(g_buteMgr.GetDwordDef(s_Spellz, s_RollingBallzTime, 0x3e8));

            CWwdGameObjectA* w = g_gameReg->m_world->m_childGroup->CreateSprite(
                0,
                m_lastTilePxX - 0x20,
                m_lastTilePxY,
                0,
                "RollingBall",
                0x40003
            );
            w->ApplyName("LEVEL_ROLLINGBALL_WEST");
            AnimWorkerObj* wi = w->m_7c;
            wi->m_bc =
                static_cast<i32>(g_buteMgr.GetDwordDef(s_Spellz, s_RollingBallzSpeed, 0x3e8));
            w->m_124 = 0;
            w->m_118 = static_cast<i32>(g_buteMgr.GetDwordDef(s_Spellz, s_RollingBallzTime, 0x3e8));
            return 1;
        }
        default:
            return 0;
    }
}

RVA(0x00057800, 0x64)
void CGrunt::SelectMoveIcon(i32 a) {
    if (m_1f4_moveIcon == a) {
        return;
    }
    m_1f4_moveIcon = a;
    if (a < 0 || a >= 0x11) {
        m_1f4_moveIcon = 0;
    }
    CShadeTable* sel = g_gameReg->m_spriteFactory->GetSel(m_1f4_moveIcon, m_entranceReason >= 0x17);
    CWwdGameObjectA* h = m_object;
    h->m_drawActive = 1;
    h->m_drawFillCmd = 0xa;
    h->m_drawFillArg = sel;
}

RVA(0x00057890, 0x19c)
i32 CGrunt::BuildGruntLoseItemAnimation() {
    StepAnimDispatchB();
    i32 reason = m_entranceReason;
    if (reason != 0x12 && reason != 0x16 && reason != 0xe) {
        return 0;
    }

    CWwdGameObjectA* spr = g_gameReg->m_world->m_childGroup->CreateSprite(
        0,
        m_object->m_screenX,
        m_object->m_screenY,
        0xcf850,
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

    LoadGruntTypeTable(0, 1, 0, 1);
    m_entranceActive = 0;
    return 1;
}

// ---------------------------------------------------------------------------
// CGrunt::TryPowerupAtTile()   @0x57aa0   (__thiscall, ret 0)
// Gated on a live entrance reason (0 < m_entranceReason < 0x17): read the level
// board's occupancy at the grunt's HUD tile; if it is clear of the blocking bits
// (0x939 / 0x2), probe a move-tile placement via the tile mgr and return 1; else 0.
//
// @early-stop
RVA(0x00057aa0, 0x9b)
i32 CGrunt::TryPowerupAtTile() {
    i32 reason = m_entranceReason;
    if (reason <= 0 || reason >= 0x17) {
        return 0;
    }
    CWwdGameObjectA* h = m_object;
    i32 mx = h->m_screenX;
    i32 my = h->m_screenY;
    CGruntzMapMgr* b = g_gameReg->m_tileGrid;
    i32 px = (mx & ~0x1f) + 0x10;
    i32 py = (my & ~0x1f) + 0x10;
    i32 tx = px >> 5;
    i32 ty = py >> 5;
    i32 flags;
    if (static_cast<u32>(tx) >= static_cast<u32>(b->m_width)
        || static_cast<u32>(ty) >= static_cast<u32>(b->m_height)) {
        flags = 1;
    } else {
        flags = b->m_rowInts[ty][tx * 7];
    }
    if ((flags & 0x939) || (flags & 2)) {
        return 0;
    }
    m_tileMgr->LoadPowerupIconSprites(reason, px, py, 0, 1, 0);
    return 1;
}

// CGrunt::EnsureStruckSlot(key) @0x57b70 - the +0x424-slot sibling of
// EnsureStruckVoice (+0x428): lazily build + play a grunt sound sample for `key`.
// Bails if the +0x424 slot is already filled, or if the registry's m_object gate is
// unset. Otherwise looks `key` up in the global sound table
// (g_gameReg->m_world->m_soundRegistry->m_10), clones a sample from the entry's factory (GetItem),
// stores it into +0x424, and plays it on the registry sound channel (m_11c).
// __thiscall, ret 4. Same lookup shape as EnsureStruckVoice / CProjectile::LaunchSound.
//
// @early-stop
RVA(0x00057b70, 0x77)
void CGrunt::EnsureStruckSlot(const char* key) {
    DirectSoundMgr*& sample = m_struckSlotSound;
    if (sample != 0) {
        return;
    }
    if (g_gameReg->m_soundEnabled == 0) {
        return;
    }
    void* entry_ob = 0;
    g_gameReg->m_world->m_soundRegistry->m_10.Lookup(key, entry_ob); // CMapStringToPtr (void*& out)
    GruntSoundEntry* entry = static_cast<GruntSoundEntry*>(entry_ob);
    if (entry == 0) {
        return;
    }
    if (entry->m_10 == 0) {
        return;
    }
    sample = static_cast<DirectSoundMgr*>(entry->m_10->GetItem());
    if (sample == 0) {
        return;
    }
    sample->ApplyAndPlay(g_gameReg->m_soundVolume, 0, 0, 1);
}

RVA(0x00057c10, 0x1e)
void CGrunt::ClearSubA() {
    DirectSoundMgr* p = m_struckSlotSound;
    if (p) {
        p->StopAndRewind();
        m_struckSlotSound = 0;
    }
}

// CGrunt::EnsureStruckVoice(key) @0x57c40 - lazily build + play the grunt's
// struck-voice sound sample. Bails if already created (the +0x428 slot ClearSubB
// frees). Looks `key` up in the global sound table (g_gameReg->m_world->m_soundRegistry->m_10),
// clones a sample from the entry's factory (GetItem), stores it into +0x428, and
// plays it on the sound channel (g_gameReg->m_soundVolume). __thiscall, ret 4. Same
// sound-lookup shape as CProjectile::LaunchSound (0xe2190).
//
RVA(0x00057c40, 0x71)
void CGrunt::EnsureStruckVoice(const char* key) {
    DirectSoundMgr*& sample = m_struckVoiceSound;
    if (sample != 0) {
        return;
    }
    void* entry_ob = 0;
    g_gameReg->m_world->m_soundRegistry->m_10.Lookup(key, entry_ob); // CMapStringToPtr (void*& out)
    GruntSoundEntry* entry = static_cast<GruntSoundEntry*>(entry_ob);
    if (entry == 0) {
        return;
    }
    if (entry->m_10 == 0) {
        return;
    }
    sample = static_cast<DirectSoundMgr*>(entry->m_10->GetItem());
    if (sample == 0) {
        return;
    }
    sample->ApplyAndPlay(g_gameReg->m_soundVolume, 0, 0, 1);
}

RVA(0x00057ce0, 0x1e)
void CGrunt::ClearSubB() {
    DirectSoundMgr* p = m_struckVoiceSound;
    if (p) {
        p->StopAndRewind();
        m_struckVoiceSound = 0;
    }
}

// CGrunt::ReapplyVoiceParams() @0x57d10 - when the registry sound gate
// (g_gameReg->m_10) is set, re-apply the current sound-channel params
// (g_gameReg->m_soundVolume) to both the struck-slot (+0x424) and struck-voice (+0x428)
// samples via DirectSoundMgr::ApplyAndPlay. __thiscall, no args. Same sample-play
// shape as EnsureStruckSlot/EnsureStruckVoice.
//
RVA(0x00057d10, 0x4e)
void CGrunt::ReapplyVoiceParams() {
    if (g_gameReg->m_soundEnabled == 0) {
        return;
    }
    DirectSoundMgr* a = m_struckSlotSound;
    if (a != 0) {
        a->ApplyAndPlay(g_gameReg->m_soundVolume, 0, 0, 1);
    }
    DirectSoundMgr* b = m_struckVoiceSound;
    if (b != 0) {
        b->ApplyAndPlay(g_gameReg->m_soundVolume, 0, 0, 1);
    }
}

RVA(0x00057d80, 0x11)
void CGrunt::DestroyAnims() {
    ClearSubA();
    ClearSubB();
}

// CGrunt::PathScan() @0x57db0 - the per-tick route re-validation. Re-clip the board
// dirty rect to the 5x5 box around the grunt, then walk the grunt's occupied-coord
// route: for each coord not already marked (and not the route tail) re-route from the
// grunt's cell to it. The first coord that still routes wins - splice the fresh route
// in front of the remainder of the old one and report 1. After five failures (or the
// whole route exhausted) and only if the grunt is within +-4 cells of its GOAL, probe
// the goal's eight neighbours for a reachable substitute, route (grunt -> neighbour)
// then (neighbour -> goal), and adopt that. If nothing routes, clear the board clip
// and report 0.
//
// @early-stop
RVA(0x00057db0, 0x8f8)
i32 CGrunt::PathScan() {
    CMapMgr* grid = g_gameReg->m_tileGrid; // implicit upcast (CGruntzMapMgr : CMapMgr == CMapMgr)
    // retail 0x57ddb caches `&m_31c` into a frame slot BEFORE the count gate and
    // every out-of-line list call takes its `this` from there (0x5802f/0x5805c/
    // 0x584f1/0x58518/0x5853a/0x585bb `mov ecx,[esp+0x10]`) - a local handle.
    CPtrList* coordz = &m_31c;
    if (CoordCount() == 0) {
        return 1;
    }
    // the same MFC walk this function already uses below (0x58015's direct
    // [pos]/[pos+8] loads) - CPtrList::GetNext inlines to node->pNext + node->data
    POSITION node = coordz->GetHeadPosition();

    i32 col5 = m_object->m_screenX >> 5;
    i32 row5 = m_object->m_screenY >> 5;
    // retail 0x57dfc..0x57e8c: the 5x5 dirty box is the +-2 cell box `rs` widened by
    // one on right/bottom, but it is reached through a POINTER that is null-tested
    // (0x57e30 `lea edx,[esp+0x48]; test edx,edx; je`) - the null arm takes the whole
    // board instead. `gb` (the clip rect) is four inline stores here, not a CRect ctor.
    // Braced so the three rects DIE here: retail's whole frame holds only four RECT
    // slots (0x28/0x38/0x48/0x58) reused by every later bounds recompute.
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
        if (pr != 0) {
            // 0x57e3c is a whole-rect COPY then two post-increments, not four
            // widened field reads: cl stores box.right twice (0x57e52 then
            // 0x57e58, around the `inc eax`) and drops the dead first store to
            // box.bottom - exactly 12 instructions, matching retail's block.
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

    CoordNode* tail = CoordTail(); // the member accessor already holds this seam
    i32 tcol = tail->m_coord->m_x;
    i32 trow = tail->m_coord->m_y;
    i32 hits = 0;

    while (node != 0) {
        Coord* co = static_cast<Coord*>(coordz->GetNext(node));
        if (co != 0) {
            // retail 0x57f21: a cell already carrying the 0x20 mark is skipped
            // UNLESS it is the route's own tail - no `fire` temp, the || just
            // short-circuits into the probe (0x57f26 je / 0x57f30 + 0x57f36 jne).
            if ((grid->m_rows[co->m_y][co->m_x].m_flagBytes[3] & 0x20) == 0
                || (co->m_x == tcol && co->m_y == trow)) {
                // retail 0x57f38: arg5 IS the route list `s` itself (0x57f5b's
                // `lea ecx,[esp+0x70]` names the same address the 0x57f3a ctor
                // does), and the "did it find one" test at 0x57f84 reads
                // [esp+0x74] == s+0xc == CPtrList::m_nCount. There is no separate
                // out-int. Args 1/2 are the GRUNT's cell (col5,row5), args 3/4 the
                // coord being probed.
                CPtrList s(0xa);
                i32 res = grid->SearchEdge(
                    col5,
                    row5,
                    co->m_x,
                    co->m_y,
                    &s,
                    1,
                    m_arrivalFlags | 0x20000000,
                    m_24c
                );
                if (res != 0) {
                    if (s.GetCount() != 0) {
                        // 0x57fbe: SearchEdge only routed as far as this coord, so
                        // the REST of the old route (from `node` on) is appended to
                        // the fresh one - each entry a coord popped off g_coordPool's
                        // free list and filled from the old node.
                        while (node != 0) {
                            Coord* src = static_cast<Coord*>(coordz->GetNext(node));
                            Coord* fresh = 0;
                            CoordPoolNode* free = g_coordPool.m_freeHead;
                            if (free->m_next != 0) {
                                fresh = &free->m_coord;
                                fresh->m_x = src->m_x;
                                fresh->m_y = src->m_y;
                                g_coordPool.m_freeHead = g_coordPool.m_freeHead->m_next;
                            }
                            s.AddTail(fresh);
                        }
                        // 0x58001: recycle the grunt's own coordz back to the pool.
                        // Same drain as the ring tail's, but here MFC's inline
                        // CPtrList::GetNext is what walks it (0x58015's direct
                        // [pos]/[pos+8] loads), not CGruntCoordList::NextData.
                        if (CoordCount() != 0) {
                            POSITION pos = m_31c.GetHeadPosition();
                            if (pos != 0) {
                                do {
                                    void* d = m_31c.GetNext(pos);
                                    if (d != 0) {
                                        g_coordPool.Push(d);
                                    }
                                } while (pos != 0);
                            }
                            coordz->RemoveAll();
                        }
                        // 0x58038: adopt the fresh route, dropping the entry that IS
                        // the grunt's current cell - that one is the list head and is
                        // handed straight back to the free list just below.
                        POSITION p = s.GetHeadPosition();
                        if (p != 0) {
                            do {
                                Coord* d = static_cast<Coord*>(s.GetNext(p));
                                if (d != 0) {
                                    if (d->m_x != col5 || d->m_y != row5) {
                                        coordz->AddTail(d);
                                    }
                                }
                            } while (p != 0);
                        }
                        void* elem = s.RemoveHead();
                        if (elem != 0) {
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
        // retail 0x57fab: the five-miss break has its OWN bounds recompute at
        // 0x5813d, on top of the common one after the loop (0x581ce).
        if (hits == 5) {
            SCAN_BOUNDS(grid);
            break;
        }
    }

    // ---- no hit: 3x3 neighbour re-scan ----
    SCAN_BOUNDS(grid);
    // retail 0x5825c: the +-4 box is centred on the ROUTE TAIL (tcol,trow) - the
    // destination cell - and the point tested against it is the grunt's own cell
    // (col5,row5): "only re-scan the neighbourhood when I am within 4 cells of my
    // goal". (It was col5/row5 on BOTH sides here, i.e. tautological.)
    RECT nb;
    nb.left = tcol - 4;
    nb.top = trow - 4;
    nb.right = tcol + 4;
    nb.bottom = trow + 4;
    if (col5 < nb.right && col5 >= nb.left && row5 < nb.bottom && row5 >= nb.top) {
        // retail 0x582ae: the same null-tested-pointer bounds set as the head block
        // (0x582c3 `lea ecx,[esp+0x48]; test ecx,ecx; je`), only here the source rect
        // is the +-4 box and the clip rect IS a real CRect local (0x582be's ctor).
        CRect rb(0, 0, grid->m_width, grid->m_height);
        RECT ra;
        const RECT* pn = &nb;
        if (pn != 0) {
            ra = *pn; // 0x582cb: same copy-then-post-increment shape as the head
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
        // retail 0x58371: the ring is dy,dx = -1..1 with the CENTRE skipped
        // (0x58397 `if (dy != 0) body; if (dx == 0) next`), the bounds test is
        // width-then-height on (tcol+dx, trow+dy), and the cell read walks the row
        // by 0x1c bytes per dx step (== m_rowInts[rr][(tcol+dx)*7]; retail hoists
        // the `(tcol-1)*7*4` byte base to the OUTER loop head at 0x58384).
        // Centre = the route TAIL, not the grunt: the ring probes the goal's
        // neighbours for a reachable substitute cell.
        for (i32 dy = -1; dy < 2; dy++) {
            for (i32 dx = -1; dx < 2; dx++) {
                if (dy == 0 && dx == 0) {
                    continue;
                }
                i32 rr = trow + dy;
                i32 cc = tcol + dx;
                // 0x583bc/0x583cb is a real if/ELSE (the out-of-bounds arm sets 1 in
                // its own block and the in-bounds arm jmps past it), and 0x583d0
                // computes the masked flags ONCE into eax for all three tests.
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
                if (mf != 0 && (m_24c & cf) == 0) {
                    continue;
                }
                CPtrList s(0xa);
                i32 res =
                    grid->SearchEdge(col5, row5, cc, rr, &s, 0, m_arrivalFlags | 0x20040002, m_24c);
                if (res != 0) {
                    // retail 0x5849f / 0x584cd: TWO separate `s.GetCount()` reads -
                    // the RemoveHead between them invalidates the cached count, so cl
                    // reloads [esp+0x74] (== s.m_nCount). Both empty cases bail out
                    // through the SAME `Clip(0); return 0` the function ends with
                    // (0x58686, the copy that also runs ~CPtrList on `s`).
                    if (s.GetCount() == 0) {
                        grid->Clip(0);
                        return 0;
                    }
                    void* elem = s.RemoveHead();
                    if (elem != 0) {
                        FREELIST_PUSH(elem);
                    }
                    if (s.GetCount() == 0) {
                        grid->Clip(0);
                        return 0;
                    }
                    // 0x584d9: recycle the grunt's own coordz (the function's ONLY
                    // call 0x29a30, at 0x584fa) ...
                    if (CoordCount() != 0) {
                        POSITION pos = m_31c.GetHeadPosition();
                        if (pos != 0) {
                            do {
                                void* d = static_cast<CGruntCoordList*>(coordz)->NextData(pos);
                                if (d != 0) {
                                    g_coordPool.Push(d);
                                }
                            } while (pos != 0);
                        }
                        coordz->RemoveAll();
                    }
                    // ... then 0x58521 transfers the fresh route into it.
                    POSITION p = s.GetHeadPosition();
                    if (p != 0) {
                        do {
                            coordz->AddTail(s.GetNext(p));
                        } while (p != 0);
                    }
                    s.RemoveAll();
                    // 0x58555: the SECOND leg - route on from the neighbour cell
                    // (cc,rr) to the original goal (tcol,trow), this time with the
                    // RAW arrival flags (no 0x20040002 punch) and clearFlag 1. Its
                    // route is appended to the one just adopted. Whatever it returns,
                    // the grunt now has a path: recompute the bounds and report 1.
                    if (grid->SearchEdge(cc, rr, tcol, trow, &s, 1, m_arrivalFlags, m_24c) != 0) {
                        if (s.GetCount() != 0) {
                            void* e2 = s.RemoveHead();
                            if (e2 != 0) {
                                FREELIST_PUSH(e2);
                            }
                            if (s.GetCount() != 0) {
                                POSITION q = s.GetHeadPosition();
                                if (q != 0) {
                                    do {
                                        coordz->AddTail(s.GetNext(q));
                                    } while (q != 0);
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

// CGrunt::OnStruck(wasHit) @0x588f0 - the struck/damage reaction step. Re-arm the
// struck cooldown (m_270=0xfa0 window, m_268=game clock now), bump the struck
// counter (m_struckCount), and - if the grunt is on-screen (the registry
// visible-bounds rect at g->m_world->m_level->m_5c+0x40) - fire an escalating struck
// grunt-voice cue (CueA) keyed by whether it was a real hit and the running count.
// __thiscall, ret 4, frameless.
// @early-stop
RVA(0x000588f0, 0x1ea)
void CGrunt::OnStruck(i32 wasHit) {
    m_struckTimerLo = 0xfa0;
    m_struckTimerHi = 0;
    m_struckClockLo = static_cast<i32>(g_frameTime);
    m_struckClockHi = 0;
    i32 c = ++m_struckCount;

    if (wasHit == 0) {
        if (m_gruntKind == 0x36) {
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

// ---------------------------------------------------------------------------
// CGrunt::ArrivalRecycle(a, b, mode, d, e)   @0x59230   (__thiscall, ret 0x14)
// mode==0: latch the pending arrival target (a switch on m_arrivalState seeds
// m_arrivalCol/m_arrivalRow from {d,e} and, for the in-flight states, marks m_defenderState=2), then - when
// committing (m_arrivalPhase 2/3, m_arrivalActive set) - commit the occupied tile slot to its
// settled HUD position (RectContains[Gated]). mode!=0: drive the move-sound then run
// the occupied-coord recycle: for each resolver reject code "H"/"F"/"O", resolve the
// cell record (the resolver's coord->index map) and bail; final miss -> ResetGeometry().
//
// @early-stop
RVA(0x00059230, 0x40d)
i32 CGrunt::ArrivalRecycle(i32 a, i32 b, i32 mode, i32 d, i32 e) {
    if (mode == 0) {
        switch (m_arrivalState) {
            case 2:
                m_arrivalCol = d;
                m_arrivalRow = e;
                break;
            case 1:
            case 4:
                m_arrivalCol = d;
                m_arrivalRow = e;
                m_defenderState = 2;
                break;
            case 5:
                m_arrivalCol = d;
                m_arrivalRow = e;
                m_defenderState = 2;
                break;
            case 3:
            case 6:
                m_arrivalCol = d;
                m_arrivalRow = e;
                m_defenderState = 2;
                break;
            case 0x11:
                m_arrivalCol = d;
                m_arrivalRow = e;
                break;
            default:
                break;
        }

        i32 phase = m_arrivalPhase;
        if ((phase == 3 || phase == 2) && m_arrivalActive != 0) {
            CGrunt* occ = m_tileMgr->m_grid[m_arrivalCol * TM_GRID_COLS + m_arrivalRow];
            if (occ != 0) {
                CGameObject* inner = occ->m_object;
                i32 yMasked = (inner->m_screenY & ~0x1f) + 0x10;
                i32 xMasked = (inner->m_screenX & ~0x1f) + 0x10;
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

    // Occupied-coord recycle: three sequential resolver reject codes. Each block
    // resolves the current anim-set node's cell record (the resolver's coord-range
    // map; the bounds hit is the fast path, the two fallbacks are engine helpers).
    char* nm0 = *g_typeColl.GetNameRecord(m_objAux->m_1c);
    if (strcmp(nm0, s_codeH) == 0) {
        return 1;
    }
    {
        i32 coord = m_objAux->ActKey();
        g_typeColl.m_grown = 0;
        CString* rec;
        if (coord < g_typeColl.m_lo || coord > g_typeColl.m_hi) {
            if (g_typeColl.GrowTo(coord, 0) != 0) {
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
    char* nm1 = *g_typeColl.GetNameRecord(m_objAux->m_1c);
    if (strcmp(nm1, s_codeF) == 0) {
        return 1;
    }
    {
        i32 coord = m_objAux->ActKey();
        g_typeColl.m_grown = 0;
        CString* rec;
        if (coord < g_typeColl.m_lo || coord > g_typeColl.m_hi) {
            if (g_typeColl.GrowTo(coord, 0) != 0) {
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
    char* nm2 = *g_typeColl.GetNameRecord(m_objAux->m_1c);
    if (strcmp(nm2, s_codeO) == 0) {
        return 1;
    }
    ResetGeometry();
    return 1;
}

RVA(0x000597a0, 0x1345)
i32 CGrunt::LoadGruntCombatAnimations(
    i32 attackKind,
    i32 struckPose,
    i32 srcRow,
    i32 srcCol,
    i32 srcPxX,
    i32 srcPxY,
    i32 fromProjectile,
    i32 attackerGruntKind
) {
    if (this->m_gruntKind == 0x38 && this->m_entranceReason != 1) {
        return 1;
    }

    // attackerGruntKind == 0x39: conversion hit - heal the struck enemy, fire GAME_CONVERSIONHIT.
    if (attackerGruntKind == 0x39) {
        CGrunt* enemy = m_tileMgr->m_grid[srcRow * TM_GRID_COLS + srcCol];
        if (enemy != 0
            && m_tileMgr->SpawnGrunt(
                   this->m_tileOwnerHi,
                   this->m_tileOwnerLo,
                   srcRow,
                   enemy->m_1f4_moveIcon
               ) != 0) {
            i32 h = enemy->m_health + 0x19;
            if (h >= 0x64) {
                h = 0x64;
            }
            enemy->m_health = h;
            // worker -> owner context (the world holder facet) -> cue host; retail
            // keeps the host in ecx from the gate test into the Lookup __thiscall.
            CDDrawSubMgrLeafScan* host =
                (static_cast<CDDrawSurfaceMgr*>(m_3c->m_ownerCtx))->m_soundRegistry;
            if (host->m_emitGate == 0) {
                LeafCue* cc = static_cast<LeafCue*>(host->Lookup(s_CONVERSIONHIT));
                if (cc != 0) {
                    cc->PlayIfElapsed(g_sndCueTag, 0, 0, 0);
                }
            }
            return 0;
        }
    }

    // Hit-type byte-table lookup + optional handicap halving.
    i32 hit = g_hitTable[this->m_entranceReason * 23 + attackKind];
    CGruntzMgr* reg = g_gameReg; // cached once (retail keeps the singleton in a reg)
    if (reg->m_isEasyMode != 0 && reg->m_134 == 1 && this->m_tileOwnerHi == g_curPlayer) {
        i32 t = hit / 2;
        hit = t + t % 5;
    }

    // Reactive-armor kind (0x3c == GRUNT_REACTIVEARMOR): scale the hit by g_dtScale, then damage the enemy.
    if (attackerGruntKind == 0x3a) {
        hit = 0x64;
    } else if (this->m_gruntKind == 0x3c) {
        hit = static_cast<i32>((static_cast<float>(hit) * g_dtScale));
        if (fromProjectile == 0) {
            CGrunt* enemy = m_tileMgr->m_grid[srcRow * TM_GRID_COLS + srcCol];
            if (enemy != 0 && enemy->m_entranceCommitted != 0) {
                i32 nh = enemy->m_health - hit * 3;
                if (nh < 0) {
                    nh = 0;
                }
                enemy->m_health = nh;
                if (nh <= 0) {
                    m_tileMgr->CellDispatch(srcRow, srcCol, 1, -1);
                }
            }
        }
    }

    // Self health decrement + reason-1 kill dispatch.
    i32 nh = this->m_health - hit;
    if (nh < 0) {
        nh = 0;
    }
    this->m_health = nh;
    if (this->m_entranceReason == 1) {
        m_tileMgr->CellDispatch(this->m_tileOwnerHi, this->m_tileOwnerLo, 1, srcRow);
        return 0;
    }
    if (nh <= 0) {
        this->m_entranceCommitted = 0;
        this->m_370 = srcRow;
    }

    // On-screen visibility gate, then the hit/block sound-cue resolve.
    LeafCue* cue = 0;
    i32 vx = this->m_object->m_screenX;
    i32 vy = this->m_object->m_screenY;
    if (vx < reg->m_viewBounds.right && vx >= reg->m_viewBounds.left
        && vy < reg->m_viewBounds.bottom && vy >= reg->m_viewBounds.top) {
        if (attackerGruntKind == 0x3a) {
            LK(s_DEATHTOUCHHIT);
            goto L_cue;
        }
        if (attackKind == 6 || attackKind == 0xa || attackKind == 0x16) {
            if (this->m_entranceReason == 8) {
                LK(s_BLOCKBODY2);
            } else {
                LK(s_IMPACTMM2);
            }
            goto L_cue;
        }
        if (this->m_entranceReason == 9) {
            if (attackKind == 5 || attackKind == 0xd || attackKind == 0xe || attackKind == 4) {
                LK(s_IMPACTMM4);
            } else {
                LK(s_IMPACTMM3);
            }
            goto L_cue;
        }
        if (this->m_entranceReason == 0xc) {
            LK(s_BLOCKMETAL1);
            goto L_cue;
        }
        if (this->m_entranceReason == 0xe) {
            if (struckPose == 1) {
                LK(s_SPRING2);
            } else {
                LK(s_SPRING1);
            }
            goto L_cue;
        }
        if (this->m_entranceReason == 0x12 && this->m_coordToggle != 0) {
            LK(s_TOOBZ);
            goto L_cue;
        }
        switch (attackKind) {
            case 0:
                if (struckPose == 0) {
                    LK(s_BLOCKBODY2);
                } else {
                    LK(s_IMPACTMM1);
                }
                break;
            case 2:
                LK(s_IMPACTMM1);
                break;
            case 3:
                if (struckPose == 0) {
                    LK(s_BLOCKBODY2);
                } else {
                    LK(s_IMPACTMM4);
                }
                break;
            case 4:
                if (struckPose == 0) {
                    LK(s_BLOCKBODY2);
                } else {
                    LK(s_IMPACTMM4);
                }
                break;
            case 5:
                if (struckPose == 0) {
                    LK(s_BLOCKBODY2);
                } else {
                    LK(s_IMPACTMM3);
                }
                break;
            case 7:
                if (struckPose == 0) {
                    LK(s_BLOCKBODY2);
                } else {
                    LK(s_IMPACTWM1);
                }
                break;
            case 8:
                if (struckPose == 0) {
                    LK(s_BLOCKBODY1);
                } else {
                    LK(s_IMPACTMM1);
                }
                break;
            case 9:
                LK(s_IMPACTWM2);
                break;
            case 0xb:
                LK(s_IMPACTMM2);
                break;
            case 0xc:
                if (struckPose == 0) {
                    LK(s_BLOCKBODY1);
                } else {
                    LK(s_IMPACTMM4);
                }
                break;
            case 0xd:
                if (struckPose == 0) {
                    LK(s_BLOCKMETAL1);
                } else {
                    LK(s_IMPACTMM4);
                }
                break;
            case 0xe:
                if (struckPose == 0) {
                    LK(s_BLOCKBODY2);
                } else {
                    LK(s_IMPACTWM3);
                }
                break;
            case 0xf:
                if (struckPose == 0) {
                    LK(s_BLOCKBODY2);
                } else {
                    LK(s_IMPACTMM1);
                }
                break;
            case 0x10:
                if (struckPose == 0) {
                    LK(s_BLOCKBODY2);
                } else {
                    LK(s_IMPACTMM3);
                }
                break;
            case 0x12:
                if (struckPose == 0) {
                    LK(s_BLOCKBODY2);
                } else {
                    LK(s_IMPACTMM1);
                }
                break;
            case 0x13:
                if (struckPose == 0) {
                    LK(s_BLOCKBODY2);
                } else {
                    LK(s_IMPACTMM1);
                }
                break;
            case 0x14:
                LK(s_IMPACTWM2);
                break;
            case 0x15:
                LK(s_IMPACTWM2);
                break;
            default:
                LK(s_IMPACTMM3);
                break;
        }

    L_cue:
        // Kill-clock-gated launch cue.
        if (cue != 0 && g_sndEnabled != 0) {
            i32 clk = g_killCueClock;
            if (static_cast<u32>((clk - cue->m_14)) >= static_cast<u32>(cue->m_18)) {
                cue->m_14 = clk;
                cue->m_10->ConfigureItem(g_sndCueTag, 0, 0, 0);
            }
        }
    }

    // Block path (attackKind in {6,0xa,0x16}); otherwise reason 0x15 kills, else return.
    if (!(attackKind == 6 || attackKind == 0xa || attackKind == 0x16)) {
        if (attackKind != 0x15) {
            return 1;
        }
        if (this->m_health > 0) {
            return 1;
        }
        m_tileMgr->CellDispatch(this->m_tileOwnerHi, this->m_tileOwnerLo, 7, srcRow);
        return 0;
    }

    if (this->m_entranceReason == 8) {
        return 1;
    }

    // Rebuild the active-anim-set type-name registry free list.
    CString* typeRec = g_typeColl.ScratchResolve(this->m_objAux->m_1c);
    if (g_typeColl.m_grown != 0) {
        CString* p = g_typeColl.Slots();
        i32 n = g_typeColl.m_grown;
        do {
            if (p != 0) {
                new (p) CString();
            }
            p++;
        } while (--n != 0);
    }
    if (strcmp(*typeRec, s_typeO) == 0) {
        return 1;
    }

    // x87 angle-octant direction resolver: copy the matching g_dirVec triple into
    // CGrunt+0x43c and set the target tile pixel (newX/newY).
    i32 dy = srcPxY - this->m_object->m_screenY;
    i32 dx = srcPxX - this->m_object->m_screenX;
    i32 newX;
    i32 newY;
    if (attackKind == 0x16) {
        switch (rand() % 8 - 1) {
            case 0:
                SETDIR(8, this->m_lastTilePxX + 0x20, this->m_lastTilePxY - 0x20);
                break;
            case 1:
                SETDIR(3, this->m_lastTilePxX + 0x20, this->m_lastTilePxY);
                break;
            case 2:
                SETDIR(5, this->m_lastTilePxX + 0x20, this->m_lastTilePxY + 0x20);
                break;
            case 3:
                SETDIR(1, this->m_lastTilePxX, this->m_lastTilePxY + 0x20);
                break;
            case 4:
                SETDIR(4, this->m_lastTilePxX - 0x20, this->m_lastTilePxY + 0x20);
                break;
            case 5:
                SETDIR(0, this->m_lastTilePxX - 0x20, this->m_lastTilePxY);
                break;
            case 6:
                SETDIR(6, this->m_lastTilePxX - 0x20, this->m_lastTilePxY - 0x20);
                break;
            default:
                SETDIR(2, this->m_lastTilePxX, this->m_lastTilePxY - 0x20);
                break;
        }
    } else if (dx == 0) {
        if (srcPxY > this->m_object->m_screenY) {
            SETDIR(2, this->m_lastTilePxX, this->m_lastTilePxY - 0x20);
        } else if (srcPxY < this->m_object->m_screenY) {
            SETDIR(1, this->m_lastTilePxX, this->m_lastTilePxY + 0x20);
        } else {
            goto L_moveDone;
        }
    } else {
        float slope = static_cast<float>(dy) / dx;
        if (slope > g_tanC0 || slope < g_tanC1) {
            if (srcPxY > this->m_object->m_screenY) {
                SETDIR(2, this->m_lastTilePxX, this->m_lastTilePxY - 0x20);
            } else {
                SETDIR(1, this->m_lastTilePxX, this->m_lastTilePxY + 0x20);
            }
        } else if (slope > g_tanC2 || slope < g_tanC3) {
            if (slope > g_tanC2) {
                if (srcPxX > this->m_object->m_screenX) {
                    SETDIR(6, this->m_lastTilePxX - 0x20, this->m_lastTilePxY - 0x20);
                } else {
                    SETDIR(5, this->m_lastTilePxX + 0x20, this->m_lastTilePxY + 0x20);
                }
            } else if (slope < g_tanC3) {
                if (srcPxX > this->m_object->m_screenX) {
                    SETDIR(4, this->m_lastTilePxX - 0x20, this->m_lastTilePxY + 0x20);
                } else {
                    SETDIR(8, this->m_lastTilePxX + 0x20, this->m_lastTilePxY - 0x20);
                }
            } else {
                goto L_moveDone;
            }
        } else {
            if (srcPxX > this->m_object->m_screenX) {
                SETDIR(0, this->m_lastTilePxX - 0x20, this->m_lastTilePxY);
            } else {
                SETDIR(3, this->m_lastTilePxX + 0x20, this->m_lastTilePxY);
            }
        }
    }

    // Tile-to-tile occupancy + diagonal-corner move check.
    {
        i32 flags = this->m_arrivalFlags | 0x20000000;
        CMapMgr* grid = static_cast<CMapMgr*>(g_gameReg->m_tileGrid); // GruntBoard==CMapMgr facet
        i32 nyt = newY >> 5;
        i32 nxt = newX >> 5;
        i32 oxt = this->m_lastTilePxX >> 5;
        i32 oyt = this->m_lastTilePxY >> 5;
        if (!(oxt == nxt && oyt == nyt)) {
            if (static_cast<u32>(nxt) >= static_cast<u32>(grid->m_width)) {
                return 1;
            }
            if (static_cast<u32>(nyt) >= static_cast<u32>(grid->m_height)) {
                return 1;
            }
            BrickzCell* cell = &grid->m_rows[nyt][nxt];
            i32 t = flags & cell->m_0;
            if (t & 0x20000000) {
                return 1;
            }
            if (t != 0 && (cell->m_0 & (this->m_24c | 0x18000482)) == 0) {
                return 1;
            }
            BrickzCell* ocell = &grid->m_rows[oyt][oxt];
            i32 dxt = nxt - oxt;
            i32 dyt = nyt - oyt;
            if (dxt != 0 && dyt != 0) {
                i32 w = grid->m_width; // vertical neighbor = +-w cells (contiguous rows)
                if (dxt > 0) {
                    if (dyt > 0) {
                        if (((ocell + 1)->m_0 & 0x2000) || ((ocell + w)->m_0 & 0x2000)
                            || ((cell - 1)->m_0 & 0x2000) || ((cell - w)->m_0 & 0x2000)) {
                            return 1;
                        }
                    } else {
                        if (((ocell + 1)->m_0 & 0x2000) || ((ocell - w)->m_0 & 0x2000)
                            || ((cell - 1)->m_0 & 0x2000) || ((cell + w)->m_0 & 0x2000)) {
                            return 1;
                        }
                    }
                } else {
                    if (dyt > 0) {
                        if (((ocell - 1)->m_0 & 0x2000) || ((ocell + w)->m_0 & 0x2000)
                            || ((cell + 1)->m_0 & 0x2000) || ((cell - w)->m_0 & 0x2000)) {
                            return 1;
                        }
                    } else {
                        if (((ocell - 1)->m_0 & 0x2000) || ((ocell - w)->m_0 & 0x2000)
                            || ((cell + 1)->m_0 & 0x2000) || ((cell + w)->m_0 & 0x2000)) {
                            return 1;
                        }
                    }
                }
            }
        }

        // Arrival commit + occupancy re-stamp + knockback trajectory tail.
        if (this->m_arrivalPending == 0) {
            m_tileMgr->ApplySwitch(this, this->m_lastTilePxX, this->m_lastTilePxY);
        }
        CMapMgr* g2 = static_cast<CMapMgr*>(g_gameReg->m_tileGrid); // GruntBoard==CMapMgr facet
        i32 ox = this->m_lastTilePxX >> 5;
        i32 oy = this->m_lastTilePxY >> 5;
        g2->m_rows[oy][ox].m_flagBytes[3] &= 0xdf;
        g2->m_rows[oy][ox].m_4 = -1;
        g2->m_rows[nyt][nxt].m_flagBytes[3] |= 0x20;
        g2->m_rows[nyt][nxt].m_4 = (this->m_tileOwnerHi << 8) | this->m_tileOwnerLo;

        if (m_31c.GetCount() != 0) {
            Coord* node = 0;
            i32 rx = this->m_lastTilePxX >> 5;
            i32 ry = this->m_lastTilePxY >> 5;
            if (g_coordPool.m_freeHead->m_next != 0) {
                node = &g_coordPool.m_freeHead->m_coord;
                node->m_x = rx;
                node->m_y = ry;
                g_coordPool.m_freeHead = g_coordPool.m_freeHead->m_next;
            }
            m_31c.AddHead(node);
        }

        this->m_lastTilePxX = newX;
        this->m_lastTilePxY = newY;
        this->m_prevAnimSetNode = this->m_objAux->m_1c;
        this->m_objAux->m_1c = ActFindId(s_typeO);
        double ddx = static_cast<double>(newX) - this->m_object->m_screenX;
        double ddy = static_cast<double>(newY) - this->m_object->m_screenY;
        double dist = sqrt(ddx * ddx + ddy * ddy);
        u32 kb = g_buteMgr.GetDwordDef(s_gruntSec, s_knockKey, 200);
        m_moveSpeed = dist / static_cast<double>(kb);
        m_408 = static_cast<double>((this->m_object->m_screenX));
        m_410 = static_cast<double>((this->m_object->m_screenY));

        if (m_31c.GetCount() != 0) {
            POSITION pos = m_31c.GetHeadPosition();
            if (pos != 0) {
                do {
                    Coord* data = static_cast<Coord*>(m_31c.GetNext(pos));
                    if (data != 0) {
                        // retail stores the CACHED reg back (`mov [eax],edx; mov edx,eax;
                        // mov ds:g_freeList,edx`) - no source-level cached local.
                        CoordPoolNode* slot = g_coordPool.NodeOf(data);
                        slot->m_next = g_coordPool.m_freeHead;
                        g_coordPool.m_freeHead = slot;
                    }
                } while (pos != 0);
            }
            m_31c.RemoveAll();
        }
        this->m_arrivalPending = 0;
    }

L_moveDone:
    return 1;
}

// ---------------------------------------------------------------------------
// CGrunt::CommitNeighbor(a, b, c, d)   @0x5b050   (__thiscall, ret 0x10)
// React to the grid-neighbour grunt at cell (a,b): gate on a not being self (or a
// global flag), the entrance-reason not being 0x13/0x14, and the committed tile
// (m_17c/m_180) being clear of the spawn-block bit; build the HUD health sprite,
// re-arm the combat-timer block (CombatTimeout config), then resolve the neighbour
// grunt from the tile-mgr's 15-wide cell grid (m_260 + (15a+b)*4 + 0x1c), gate it
// (live, both committed, not anim "F"); dispatch on m_170/m_toolId (==1 -> a move
// config) else on the current anim type code ("I" -> arrival re-notify; "N" -> the
// align-down/drop-ready/snap re-latch); finally run the shared combat finalize:
// commit the in-flight move, latch m_220, build the neighbour's HUD health sprite +
// combat timer, recycle both arrival blocks, and (when not low-stamina/active)
// re-arm the attack anim. __thiscall, ret 0x10; returns 1 on success, 0 on bail.
//
// @early-stop
RVA(0x0005b050, 0x40b)
i32 CGrunt::CommitNeighbor(i32 a, i32 b, i32 c, i32 d) {
    if (a == m_tileOwnerHi && g_traitorMode == 0) {
        return 0;
    }
    i32 reason = m_entranceReason;
    if (reason == 0x14 || reason == 0x13) {
        return 0;
    }
    {
        CGruntzMapMgr* bd = g_gameReg->m_tileGrid;
        i32 tx = m_lastTilePxX >> 5;
        i32 ty = m_lastTilePxY >> 5;
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
    m_358 = 1;

    CGrunt* nb = m_tileMgr->m_grid[a * TM_GRID_COLS + b];
    if (nb == 0 || nb->m_entranceCommitted == 0 || m_entranceCommitted == 0) {
        return 0;
    }

    bool eq;
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), s_codeF) == 0);
    if (eq) {
        return 0;
    }
    // ONE reason variable, overwritten in place - not two conditions. Retail's
    //   mov eax,[m_170] / xor ecx,ecx / cmp eax,0x16 / jle L / mov eax,[m_toolId]
    //   L: cmp eax,1 / jne J / mov ecx,eax / J: test ecx,ecx
    // reaches `cmp eax,1` on BOTH paths, so an m_entranceReason of exactly 1 also
    // fires the move config. The `jle` lands on the compare, not past it.
    // Flag and reason are separate variables: the flag is zeroed before the
    // diamond and assigned the (already-loaded) reason inside the ==1 arm, which
    // is why cl copies a register instead of materialising an immediate 1.
    i32 flag = 0;
    i32 v = m_entranceReason;
    if (v > 0x16) {
        v = m_toolId;
    }
    if (v == 1) {
        flag = v;
    }
    if (flag != 0) {
        RunMoveConfig(c >> 5, d >> 5);
        return 1;
    }

    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), "I") == 0);
    if (eq) {
        m_tileMgr->LoadTileArrivalFx(
            m_tileOwnerHi,
            m_tileOwnerLo,
            m_moveTileX,
            m_moveTileY,
            m_entranceReason,
            -1
        );
    } else {
        eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), s_codeN) == 0);
        if (eq) {
            i32 lastX = m_lastTilePxX;
            i32 lastY = m_lastTilePxY; // retail hoists BOTH latches into registers
            i32 px = (m_object->m_screenX & ~0x1f) + 0x10;
            i32 py = (m_object->m_screenY & ~0x1f) + 0x10;
            i32 redo = 1;
            if (px != lastX || py != lastY) {
                if (IsDropReady(1)) {
                    m_coordToggle = (m_coordToggle == 0);
                    redo = 0;
                }
            }
            SnapToLastTile(1);
            if (redo) {
                m_prevAnimSetNode = m_objAux->m_1c;
                m_objAux->m_1c = ActFindId(s_codeD);
                SetupTubeAnim(m_coordToggle); // 0x5b32f -> ILT 0x1e47 -> 0x50a50
            }
        }
    }

    // The shared combat finalize.
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
    m_neighborCol = a;
    m_neighborRow = b;
    m_208 = c;
    m_20c = d;
    if (m_stamina < 0x64 || m_entranceActive != 0) {
        m_neighborValid = 1;
        return 1;
    }
    m_neighborValid = 0;
    nb->ArrivalRecycle(m_object->m_screenX, m_object->m_screenY, 0, m_tileOwnerHi, m_tileOwnerLo);
    RearmAttackAnim(a, b);
    return 1;
}

// ---------------------------------------------------------------------------
// CGrunt::BeginAttack(a, b)  @0x5b570  (__thiscall, ret 8)
// Gated on the entrance being committed (m_1fc != 0), the current anim NOT being
// the "F"/struck code, and m_stamina >= 0x64. Fires the directional move-sound to
// (a, b), latches the powered-up / +0x218 combat state, builds the HUD health
// sprite, latches the combat-timer block (CombatTimeout config + game clock), and
// re-arms the ATTACK2 anim (RearmAttackAnim2). Returns 1 on commit, else 0.
//
// @early-stop
RVA(0x0005b570, 0x12b)
i32 CGrunt::BeginAttack(i32 a, i32 b) {
    if (m_entranceCommitted == 0) {
        goto fail;
    }
    {
        // retail defers the ->m_name load past the (inlined) scratch teardown loop
        CString* rec = g_typeColl.ScratchResolve(m_objAux->m_1c);
        GruntScratchTeardown();
        bool eq = (strcmp(*rec, s_codeF) == 0);
        if (eq) {
            goto fail;
        }
    }
    if (m_stamina < 0x64) {
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
    m_358 = 1;
    m_208 = a;
    m_20c = b;
    RearmAttackAnim2();
    return 1;
fail:
    return 0;
}

RVA(0x0005b6f0, 0xb5)
CGrunt* CGrunt::FindGridNeighbor(i32 validate) {
    if (m_neighborCol == -1) {
        return 0;
    }
    if (m_neighborRow == -1) {
        return 0;
    }

    CGrunt* n = m_tileMgr->m_grid[m_neighborCol * TM_GRID_COLS + m_neighborRow];
    if (n != 0 && n->m_entranceCommitted != 0) {
        if (validate != 0) {
            if (n->m_object->m_screenX != n->m_lastTilePxX) {
                return 0;
            }
            if (n->m_object->m_screenY != n->m_lastTilePxY) {
                return 0;
            }
        }
        if (RectContains(n->m_object->m_screenX, n->m_object->m_screenY)) {
            CommitNeighbor(
                m_neighborCol,
                m_neighborRow,
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
    m_10.Lookup(key, val); // CMapStringToPtr::Lookup @0x1b8438 (void*& out-param)
    return static_cast<CObject*>(val);
}

RVA(0x0005baf0, 0xf4)
i32 GruntSpawnPump(CGameObject* owner) {
    AnimWorkerObj* rec = owner->m_7c;
    switch (static_cast<u32>(rec->ActKey())) {
        case 0: {
            rec->SetActKey(0x3e8);
            CUserLogic* sub = new CGrunt(owner);
            sub->Activate(); // slot 6 (+0x18)
            rec->m_logic = sub;
            break;
        }
        case 0x1d:
            rec->m_logic->UserLogicVfunc9(); // slot 11 (+0x2c)
            break;
        case 0x1e:
            rec->m_logic->UserLogicVfunc8(); // slot 10 (+0x28)
            break;
        case 0x50:
            rec->m_logic->UserLogicVfuncC(); // slot 14 (+0x38)
            break;
        case 0x53:
            rec->m_logic->UserLogicVfuncD(); // slot 15 (+0x3c)
            break;
        case 0x52:
            rec->m_logic->UserLogicVfuncA(); // slot 12 (+0x30)
            break;
        case 0x51:
            rec->m_logic->UserLogicVfuncB(); // slot 13 (+0x34)
            break;
        case 0x3e8:
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

// (The "count-down free-loop induction / slot-vs-id callee-saved coloring wall" that
// stood here was not a wall - it was the act-registrar bug of
// docs/patterns/act-registrar-counter-cse-and-freeloop.md, twice: the name-slot lookup
// read the local `id` instead of the global g_typeCounter in the 18 macro blocks, and
// the 19th key was expanded through the macro at all when retail writes it through the
// derived accessor. Both fixed; EXACT.)
RVA(0x0005be30, 0x9e5)
void RegisterActs_644af0() {
    REGISTER_KEY_644AF0("A", &CGrunt::ResolveEntranceArrival);
    REGISTER_KEY_644AF0("B", &CGrunt::StepWarpExit);
    REGISTER_KEY_644AF0("C", &CGrunt::LoadGruntDecayConfig);
    REGISTER_KEY_644AF0(s_codeD, &CGrunt::StepArrivalReroll);
    REGISTER_KEY_644AF0("E", &CGrunt::UpdateGruntStatus);
    REGISTER_KEY_644AF0(s_codeF, &CGrunt::DispatchVtbl24);
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
// ---------------------------------------------------------------------------
// CGrunt::Activate()   @0x5caa0   (__thiscall, ret 0)
// The grunt reset/spawn-init step. Fills the per-direction velocity-vector table at
// this+0x4b0 (9 directions, each a 0x78-stride record, 4 doubles/record) from the 9
// runtime direction-index globals (0x644aa0..0x644b48; index = 3*dir[0] + dir[1]) with
// the unit/diagonal direction vectors (0, +-1.0, +-0.5, +-sqrt(2)/2), then resets the
// grunt's spawn state: HUD anchor, health/stamina (100), the entrance flags, the latches.
//
// EXACT since 2026-07-29. The old "x87 FP instruction-scheduling wall" hid TWO real
// value bugs and one shape bug:
//   * `n = -1.0 / s` is -sqrt(2); retail's second divide is `fld -1.0 / fdiv <diag>`,
//     i.e. -1.0/sqrt(2) - the old form doubled every negative diagonal component;
//   * NorthEast's m_dirY was `s` (+0.707) where retail stores the NEGATIVE diagonal;
//   * the per-record `CGruntCellRec* c` pointer collapsed 36 index computations into 9.
// With the subscript spelled at every field and -1.0/diag left as an expression (so cl
// computes it at its first use, where retail has it), the x87 schedule falls out exactly.
RVA(0x0005caa0, 0x5e4)
void CGrunt::Activate() {
    double diag = sqrt(2.0);
    // The two diagonal magnitudes are BOTH +-1.0/sqrt(2) (retail: `fdivr 1.0` and
    // `fld -1.0; fdiv <diag>`, the two doubles at 0x5e9a30/0x5e9a38 over the saved
    // sqrt). The old `n = -1.0 / s` spelling was -sqrt(2), i.e. TWICE the magnitude
    // it should be, on every negative diagonal component.
    // `n` is NOT a hoisted local: retail computes -1.0/diag at its FIRST USE (after
    // the NorthEast m_dirX store) and CSEs it into the [esp+0x10] scratch from there.
    // A `double n = ...;` declaration emits the fld/fdiv up at the declaration and
    // forces an extra `fld st(1)` to get `s` back on top for that first store.
    double s = 1.0 / diag;

    // Each record: 4 doubles at the cell's +0/8/0x10/0x18. The 9 globals are processed
    // in this fixed order (ab0,ae0,aa0,b28,ac0,b48,ad0,b18,b38). The cell subscript is
    // SPELLED OUT at every field, not hoisted into a `CGruntCellRec*` local: retail
    // reloads the direction pair and redoes the 13*(3*row+col) index math for each of
    // the 36 stores (`mov [ecx+edx*8+0x4XX],..`), where a hoisted pointer collapses it
    // into one `lea` per record.
    m_cells[3 * g_gruntDirNorth.row + g_gruntDirNorth.column].m_dirX = 0.0;
    m_cells[3 * g_gruntDirNorth.row + g_gruntDirNorth.column].m_dirY = -1.0;
    m_cells[3 * g_gruntDirNorth.row + g_gruntDirNorth.column].m_stepX = 0.0;
    m_cells[3 * g_gruntDirNorth.row + g_gruntDirNorth.column].m_stepY = -0.5;

    m_cells[3 * g_gruntDirNorthEast.row + g_gruntDirNorthEast.column].m_dirX = s;
    m_cells[3 * g_gruntDirNorthEast.row + g_gruntDirNorthEast.column].m_dirY = -1.0 / diag;
    m_cells[3 * g_gruntDirNorthEast.row + g_gruntDirNorthEast.column].m_stepX = 0.5;
    m_cells[3 * g_gruntDirNorthEast.row + g_gruntDirNorthEast.column].m_stepY = -0.5;

    m_cells[3 * g_gruntDirEast.row + g_gruntDirEast.column].m_dirX = 1.0;
    m_cells[3 * g_gruntDirEast.row + g_gruntDirEast.column].m_dirY = 0.0;
    m_cells[3 * g_gruntDirEast.row + g_gruntDirEast.column].m_stepX = 0.5;
    m_cells[3 * g_gruntDirEast.row + g_gruntDirEast.column].m_stepY = 0.0;

    m_cells[3 * g_gruntDirSouthEast.row + g_gruntDirSouthEast.column].m_dirX = s;
    m_cells[3 * g_gruntDirSouthEast.row + g_gruntDirSouthEast.column].m_dirY = s;
    m_cells[3 * g_gruntDirSouthEast.row + g_gruntDirSouthEast.column].m_stepX = 0.5;
    m_cells[3 * g_gruntDirSouthEast.row + g_gruntDirSouthEast.column].m_stepY = 0.5;

    m_cells[3 * g_gruntDirSouth.row + g_gruntDirSouth.column].m_dirX = 0.0;
    m_cells[3 * g_gruntDirSouth.row + g_gruntDirSouth.column].m_dirY = 1.0;
    m_cells[3 * g_gruntDirSouth.row + g_gruntDirSouth.column].m_stepX = 0.0;
    m_cells[3 * g_gruntDirSouth.row + g_gruntDirSouth.column].m_stepY = 0.5;

    m_cells[3 * g_gruntDirSouthWest.row + g_gruntDirSouthWest.column].m_dirX = -1.0 / diag;
    m_cells[3 * g_gruntDirSouthWest.row + g_gruntDirSouthWest.column].m_dirY = s;
    m_cells[3 * g_gruntDirSouthWest.row + g_gruntDirSouthWest.column].m_stepX = -0.5;
    m_cells[3 * g_gruntDirSouthWest.row + g_gruntDirSouthWest.column].m_stepY = 0.5;

    m_cells[3 * g_gruntDirWest.row + g_gruntDirWest.column].m_dirX = -1.0;
    m_cells[3 * g_gruntDirWest.row + g_gruntDirWest.column].m_dirY = 0.0;
    m_cells[3 * g_gruntDirWest.row + g_gruntDirWest.column].m_stepX = -0.5;
    m_cells[3 * g_gruntDirWest.row + g_gruntDirWest.column].m_stepY = 0.0;

    m_cells[3 * g_gruntDirNorthWest.row + g_gruntDirNorthWest.column].m_dirX = -1.0 / diag;
    m_cells[3 * g_gruntDirNorthWest.row + g_gruntDirNorthWest.column].m_dirY = -1.0 / diag;
    m_cells[3 * g_gruntDirNorthWest.row + g_gruntDirNorthWest.column].m_stepX = -0.5;
    m_cells[3 * g_gruntDirNorthWest.row + g_gruntDirNorthWest.column].m_stepY = -0.5;

    m_cells[3 * g_gruntDirCenter.row + g_gruntDirCenter.column].m_dirX = 0.0;
    m_cells[3 * g_gruntDirCenter.row + g_gruntDirCenter.column].m_dirY = 0.0;
    m_cells[3 * g_gruntDirCenter.row + g_gruntDirCenter.column].m_stepX = 0.0;
    m_cells[3 * g_gruntDirCenter.row + g_gruntDirCenter.column].m_stepY = 0.0;

    // --- spawn-state reset tail (integer field stores) ---
    CWwdGameObjectA* h = m_object;
    i32 px = h->m_screenX;
    m_commitPxX = px;
    m_lastTilePxX = px;
    m_entrancePxX = px;
    i32 py = h->m_screenY;
    m_commitPxY = py;
    m_lastTilePxY = py;
    m_entrancePxY = py;
    m_1dc = 0;
    m_1e0 = 0;
    m_health = 0x64;
    m_stamina = 0x64;
    m_toyTime = 0;
    m_wingzTime = 0;
    m_entranceActive = 0;
    m_arrivalPending = 0;
    m_arrivalState = 0;
    m_poweredUp = 0;
    m_resetApplied = 0;
    m_arrivalFlags = 0x4000901;
    m_24c = 0;
    m_deathAnimStarted = 0;
    m_tileClaimed = 0;
}

#undef REGISTER_KEY_644AF0
