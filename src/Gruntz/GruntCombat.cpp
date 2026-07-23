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
#include <Gruntz/ActReg.h>    // CLookupColl/CActReg::ResolveEntry
#include <Gruntz/AniElement.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/SerialRecords.h>
#include <Gruntz/MovingLogicSerial.h>
#include <Gruntz/BoundaryLowerMethodsViews.h>
#include <Gruntz/Effect6b.h>
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
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/LightFx.h> // CLightFx::Activate (spell LightFx sprites; folded CSpriteRegistrar)
#include <DDrawMgr/DDrawSubMgrLeafScan.h> // CDDrawSubMgrLeafScan::Lookup (rehomed here)
#include <Gruntz/GameRegistry.h> // CDDrawSurfaceMgr - the worker's m_0c owner-context facet
#include <Gruntz/LeafCue.h>      // LeafCue - the launch-sound cue entries
#include <Gruntz/SoundCue.h> // CDDrawSubMgrLeafScan (typedef of CDDrawSubMgrLeafScan) - the cue registry
#include <Gruntz/TriggerMgr.h> // CTriggerMgr - the CGrunt+0x260 board
#include <Gruntz/GruntBehaviorLeaf.h> // CGruntBehaviorLeaf - 3 of the 19 act handlers (decay/wand AI leaves)
#include <new>
#include <Gruntz/GruntEntranceArrival.h> // ex Globals.h
#include <Gruntz/SoundState.h>           // ex Globals.h transitive
#pragma intrinsic(strcmp, sqrt)

static const char s_GRUNTZ_[] = "GRUNTZ_";
static const char s__MOVING[] = "_MOVING";
static const char s__DEATH[] = "_DEATH";
static const char s__JOY[] = "_JOY";
static const char s__IDLE[] = "_IDLE";
static const char s__BATTLECRY[] = "_BATTLECRY";
static const char s__LOSEITEM[] = "_LOSEITEM";
static const char s_SingleAnimation[] = "SingleAnimation";
static const char s_keyB[] = "B";
static const char s_keyC[] = "C";
static const char s_keyE[] = "E";
static const char s_keyA[] = "A";
static const char s_keyF[] = "F";

static char s_TimePerTile[] = "TimePerTile";
static char s_Grunt[] = "Grunt";                               // s_Grunt_0060a9ec
static char s_EntranceSafeTime[] = "EntranceSafeTime";         // s_EntranceSafeTime_0060df98
static char s_IdleDelay[] = "IdleDelay";                       // s_IdleDelay_0060e1a0
static char s_PlayerDefenderRadius[] = "PlayerDefenderRadius"; // s_PlayerDefenderRadius_0060e1ac
static char s_CombatTimeout[] = "CombatTimeout";               // s_CombatTimeout_0060df84

static inline void GruntScratchTeardown();

// ===========================================================================
// The 5 grunt movement / anim-name dispatch state machines (formerly the
// CUserLogic_* stubs @0x4b370 / 0x4c170 / 0x52fb0 / 0x5f310 / 0x6a6d0). Each
// resolves the grunt's current anim-set node name
// (g_typeColl.GetNameRecord(m_objAux->m_1c), or the scratch-teardown
// GetNameRecords form) and dispatches on its single-letter type code
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
    CAnimScratchString* slot = (reinterpret_cast<CAnimScratchString*>(g_typeColl.m_alloc));
    i32 cnt = g_typeColl.m_grown;
    while (cnt != 0) {
        if (slot != 0) {
            (reinterpret_cast<CString*>(slot))->~CString();
        }
        slot++;
        cnt--;
    }
}

static const char s_GAME_ATTACK[] = "GAME_ATTACK";
static char s_Spellz[] = "Spellz";
static char s_FreezeRadius[] = "FreezeRadius";
static char s_HealthRadius[] = "HealthRadius";
static char s_RessurectionRadius[] = "RessurectionRadius";
static char s_ToyzRadius[] = "ToyzRadius";
static char s_TeleportRadius[] = "TeleportRadius";
static char s_RollingBallzSpeed[] = "RollingBallzSpeed";
static char s_RollingBallzTime[] = "RollingBallzTime";

enum SpellzEffect {
    SPELLZ_FREEZE = 1,       // FreezeRadius
    SPELLZ_HEALTH = 2,       // HealthRadius
    SPELLZ_RESURRECTION = 3, // RessurectionRadius
    SPELLZ_TOYZ = 4,         // ToyzRadius
    SPELLZ_TELEPORT = 5,     // TeleportRadius
    SPELLZ_ROLLINGBALL = 6,  // RollingBallzSpeed/Time (spawns 4 directional ballz)
};

static const char s_CONVERSIONHIT[] = "GAME_CONVERSIONHIT";
static const char s_DEATHTOUCHHIT[] = "GAME_DEATHTOUCHHIT";
static const char s_IMPACTMM1[] = "GRUNTZ_NORMALGRUNT_IMPACTMM1";
static const char s_IMPACTMM2[] = "GRUNTZ_NORMALGRUNT_IMPACTMM2";
static const char s_IMPACTMM3[] = "GRUNTZ_NORMALGRUNT_IMPACTMM3";
static const char s_IMPACTMM4[] = "GRUNTZ_NORMALGRUNT_IMPACTMM4";
static const char s_IMPACTWM1[] = "GRUNTZ_NORMALGRUNT_IMPACTWM1";
static const char s_IMPACTWM2[] = "GRUNTZ_NORMALGRUNT_IMPACTWM2";
static const char s_IMPACTWM3[] = "GRUNTZ_NORMALGRUNT_IMPACTWM3";
static const char s_BLOCKBODY1[] = "GRUNTZ_NORMALGRUNT_BLOCKBODY1";
static const char s_BLOCKBODY2[] = "GRUNTZ_NORMALGRUNT_BLOCKBODY2";
static const char s_BLOCKMETAL1[] = "GRUNTZ_NORMALGRUNT_BLOCKMETAL1";
static const char s_SPRING2[] = "GRUNTZ_SPRINGGRUNT_SPRINGGRUNTS2S1";
static const char s_SPRING1[] = "GRUNTZ_SPRINGGRUNT_SPRINGGRUNTS1S1";
static const char s_TOOBZ[] = "GRUNTZ_TOOBGRUNT_TOOBZGRUNTUI1B";
static const char s_typeO[] = "O";
static const char s_knockKey[] = "KnockBackTimePerTile";
static const char s_gruntSec[] = "Grunt";

#define LK(key)                                                                                    \
    do {                                                                                           \
        LeafCue* out = 0;                                                                          \
        g_gameReg->m_world->m_soundRegistry->m_10.Lookup((key), reinterpret_cast<void*&>(out));    \
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
// register-pinning / tail-merge wall (~60%): the 8-entry jump table + the per-case
// ±0x20 deltas are logically exact. Retail pins y in the callee-saved esi (push esi),
// which lets it TAIL-MERGE every case into one shared `mov eax,[esp+8]; mov [eax],edx;
// mov [eax+4],esi; pop esi; ret` epilogue (the cardinal cases enter mid-block, skipping
// the diagonal's first sub). cl allocates volatiles (x->eax, y->edx) with nothing to
// restore, so it DUPLICATES the tiny store+ret into all 8 case blocks instead of
// merging. The esi choice (zero-register-pinning) is the root and is not source-
// steerable; the cascading register operands + duplicated tails are the residual.
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

#include <Gruntz/FreeNodePool.h> // the coord-node pool object @0x645540
#include <Gruntz/GruntCombat.h>  // g_reg_644af0 decl

DATA(0x0020d7fc)
char s_codeH[] = "H";

DATA(0x0020d2e8)
char s_codeF[] = "F";

// g_reg_644af0 (0x00244af0): CLookupColl - no provable static init (the type has no
// default ctor / is runtime-Init'd), so the datum is named by symbol.
DATA_SYMBOL(0x00244af0, 0x0, ?g_reg_644af0@@3UCLookupColl@@A)

void* __stdcall ListNodeAdvance(void** pos); // 0x29a30 (thunk 0x1de8)

#define SCAN_BOUNDS(grid)                                                                          \
    {                                                                                              \
        RECT ra;                                                                                   \
        RECT rb;                                                                                   \
        static_cast<RECT*>(new (&ra) CRect(0, 0, (grid)->m_width, (grid)->m_height));              \
        RECT* pb = static_cast<RECT*>(new (&rb) CRect(0, 0, (grid)->m_width, (grid)->m_height));   \
        ra.left = pb->left;                                                                        \
        ra.top = pb->top;                                                                          \
        ra.right = pb->right;                                                                      \
        ra.bottom = pb->bottom;                                                                    \
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

// @early-stop
// FP instruction-scheduling wall: logic + offsets exact, but MSVC's x87 stack
// scheduling for the magnitude/normalize rarely matches from C source (an x87
// fxch/fsubp ordering the compiler picks); see docs/patterns (FP scheduling).
// CGrunt::ComputeFacing(double dt) @0x57060 - sets the grunt's facing/velocity:
//   m_400 = (sqrt(dx*dx + dy*dy) / m_41c) * dt   (dx=m_lastTilePxX-m_5c, dy=m_lastTilePxY-m_60)
//   m_408 = (double)m_object->m_5c;  m_410 = (double)m_object->m_60
// dt is the per-tile time step; m_41c is the configured TimePerTile.
RVA(0x00057060, 0x6f)
void CGrunt::ComputeFacing(double dt) {
    CWwdGameObjectA* h = m_object;
    double dx = static_cast<double>(m_lastTilePxX) - static_cast<double>(h->m_screenX);
    double dy = static_cast<double>(m_lastTilePxY) - static_cast<double>(h->m_screenY);
    // retail zero-extends m_timePerTile to 64-bit before fild (unsigned->double)
    m_400 = (sqrt(dx * dx + dy * dy) / static_cast<double>(static_cast<u32>(m_timePerTile))) * dt;
    m_408 = static_cast<double>(h->m_screenX);
    m_410 = static_cast<double>(h->m_screenY);
}

#define REGISTER_KEY_644AF0(key, handler)                                                          \
    {                                                                                              \
        i32 id = reinterpret_cast<i32>(g_buteTree.Find(key));                                      \
        if (id == 0) {                                                                             \
            g_buteTree.Insert(key, reinterpret_cast<void*>(g_typeCounter));                        \
            id = g_typeCounter;                                                                    \
            char* slot = g_typeColl._zvec::IndexToPtr(id);                                         \
            i32 n = g_typeColl.m_grown;                                                            \
            void** list = reinterpret_cast<void**>(g_typeColl.m_alloc);                            \
            while (n-- != 0) {                                                                     \
                if (list != 0) {                                                                   \
                    (reinterpret_cast<CString*>(list))->CString::~CString();                       \
                }                                                                                  \
                list++;                                                                            \
            }                                                                                      \
            (reinterpret_cast<CString*>(slot))->operator=(key);                                    \
            g_typeCounter++;                                                                       \
        }                                                                                          \
        (reinterpret_cast<CGruntActEntry*>(g_reg_644af0.Resolve(id)))->m_fn =                      \
            reinterpret_cast<GruntActHandler>(handler);                                            \
    }

// @early-stop
// Create*-sprite register coin-flip wall (~90.5%): the prologue (incl. the ebp=0 +
// 4th-saved-reg pin recovered via the rolling-ball m_7c temps), the random-index pick,
// the GAME_ATTACK sound gate, every switch case (CreateSprite, the init vtable call,
// Activate, the GetIntDef-into-CombatCue/ResurrectCue area cue) and the cross-case
// tail-merge are byte-correct in shape/offsets/symbols/CFG. Residue is the documented
// Create*-sprite scheduling coin-flip (which callee-saved reg holds m_180/m_17c/g per
// case; the same wall the 7 CGrunt Create* carry at 99% each, compounded over the 10
// CreateSprite calls) + a 1-instr `cmp edi,ebp` operand-order flip. Deferred to the
// final sweep.
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
    CDDrawSubMgrLeafScan* slot = (static_cast<CDDrawSurfaceMgr*>(m_3c->m_0c))->m_soundRegistry;
    if (slot->m_emitGate == 0) {
        LeafCue* sout = 0;
        slot->m_10.Lookup(
            s_GAME_ATTACK,
            reinterpret_cast<void*&>(sout)
        ); // CMapStringToPtr @0x1b8438
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
                ->Activate(
                    reinterpret_cast<i32>("GAME_LIGHTING_FLASH"),
                    reinterpret_cast<i32>("GAME_FLASH"),
                    9,
                    1
                );
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
                ->Activate(
                    reinterpret_cast<i32>("GAME_LIGHTING_FLASH"),
                    reinterpret_cast<i32>("GAME_FLASH"),
                    2,
                    1
                );
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
                ->Activate(
                    reinterpret_cast<i32>("GAME_LIGHTING_FLASH"),
                    reinterpret_cast<i32>("GAME_FLASH"),
                    8,
                    1
                );
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
                ->Activate(
                    reinterpret_cast<i32>("GAME_LIGHTING_FLASH"),
                    reinterpret_cast<i32>("GAME_FLASH"),
                    7,
                    1
                );
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
                ->Activate(
                    reinterpret_cast<i32>("GAME_LIGHTING_FLASH"),
                    reinterpret_cast<i32>("GAME_FLASH"),
                    3,
                    1
                );
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
    i32 sel = g_gameReg->m_spriteFactory->GetSel(m_1f4_moveIcon, m_entranceReason >= 0x17);
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
    i32 x = m_object->m_screenX;
    i32 y = m_object->m_screenY;
    CCueRect* rc = reinterpret_cast<CCueRect*>(&g->m_world->m_level->m_mainPlane->m_originX);
    if (x < rc->right && x >= rc->left && y < rc->bottom && y >= rc->top) {
        g->m_cueSink->LoadGruntSpawnConfig(reinterpret_cast<i32>(this), 0xe, -1, -1, -1);
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
// single-instruction scheduling coin-flip: logic/CFG/offsets/board-deref/both returns
// byte-exact. Residue = cl loads board->m_width (`mov ecx,[ebx+0xc]`) one slot earlier
// than retail (retail defers it past `add eax,0x10`) and reads g_gameReg before m_object
// vs retail's m_object-first; the rest is identical. ~93%. Final sweep.
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
        flags = (reinterpret_cast<i32*>(b->m_rowBytes[ty]))[tx * 7];
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
// reloc-naming scoring artifact (objdiff-reloc-scoring memory): the three engine
// callees - the sound-map Lookup (0x1b8438), the sample factory GetItem (0x135d70)
// and the sample Play (0x136300) - are not yet RVA-annotated, so their REL32
// operands pair to the target's FUN_ names and stay fuzzy until those engine fns
// get stubs (the SAME referent set EnsureStruckVoice waits on). g_gameReg IS named.
// Logic complete; flips to exact once that shared referent set is named.
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
// @early-stop
// reloc-naming scoring artifact (docs/patterns, objdiff-reloc-scoring memory):
// instruction stream byte-identical vs retail (verified llvm-objdump), 100% fuzzy.
// g_gameReg IS named (the two ds: relocs pair); the three engine callees - the
// sound-map Lookup (0x1b8438), the sample factory GetItem (0x135d70) and the
// sample Play (0x136300) - are not yet RVA-annotated, so their REL32 operands pair
// to the target's FUN_ names and stay fuzzy. Flips to exact once those engine
// functions get stubs (the SAME referent set LaunchSound waits on). Logic complete.
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
// @early-stop  (~99.9%, logic/control-flow/reloc-set byte-exact)
// FLAGGED for the cleanup loop, NOT a codegen wall: the two m_424/m_428 member
// reads compile to [esi+0x544]/[esi+0x548] instead of retail's +0x424/+0x428 - the
// SHARED pre-existing CGrunt layout drift. CMovingLogic (the base) already declares
// the 0x30..0x14c field band, and CGrunt REDECLARES the same band from +0x30, so
// CGrunt's own fields start at the base sizeof (0x150) instead of 0x30 - every
// CGrunt own-member >= 0x30 lands +0x120 high (sizeof CGrunt = 2552). Identical
// state to EnsureStruckSlot/EnsureStruckVoice/ClearSubA/ClearSubB; all flip to 100%
// together once the CMovingLogic/CGrunt field duplication is reconciled.
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

// @early-stop
// large grunt path-cell scan reconstruction (final-sweep candidate): the /GX EH frame
// from the scratch CObList local, the coord-count gate, the 5x5 dirty box + IntersectRect
// clamp, the tracked-coord scan loop firing Probe20f4 (m_arrivalFlags|0x20000000 / m_24c)
// capped at five hits, the g_coordPool.m_freeHead pop/push + g_coordPool recycle drains, the 9x9
// neighbour re-scan (flag 0x20040002) and the plane dirty-rect recompute are byte-shaped
// and the DATA refs (g_gameReg / g_coordPool.m_freeHead family / g_coordPool / IntersectRect) pair.
// Residual walls: the overlapping stack-slot schedule of the box/coord temps, the
// per-iteration CObList EH-state stamps and the 8-arg Probe20f4 push ordering diverge from
// retail's regalloc - re-attack leaf-first in the sweep.
RVA(0x00057db0, 0x8f8)
i32 CGrunt::PathScan() {
    CMapMgr* grid = g_gameReg->m_tileGrid; // implicit upcast (CGruntzMapMgr : CMapMgr == CMapMgr)
    if (CoordCount() == 0) {
        return 1;
    }
    GruntCoordNode* node = CoordHead();

    i32 col5 = m_object->m_screenX >> 5;
    i32 row5 = m_object->m_screenY >> 5;
    RECT box;
    box.left = col5 - 2;
    box.top = row5 - 2;
    box.right = col5 + 3;
    box.bottom = row5 + 3;
    RECT gb;
    gb.left = 0;
    gb.top = 0;
    gb.right = grid->m_width;
    gb.bottom = grid->m_height;
    if (!IntersectRect(&grid->m_bounds, &box, &gb)) {
        grid->m_bounds = box;
    }
    grid->m_gridW = grid->m_bounds.right - grid->m_bounds.left;
    grid->m_gridH = grid->m_bounds.bottom - grid->m_bounds.top;

    i32 tcol = CoordTail()->m_coord->m_x;
    i32 trow = CoordTail()->m_coord->m_y;
    i32 hits = 0;
    i32 hitFound = 0;

    while (node != 0) {
        GruntCoordNode* cur = node;
        node = node->m_next;
        GruntCoord* co = cur->m_coord;
        if (co != 0) {
            i32 c = co->m_x;
            i32 r = co->m_y;
            i32 fire = 1;
            if (grid->m_rows[r][c].m_flagBytes[3] & 0x20) {
                fire = (co->m_x == tcol && co->m_y == trow) ? 1 : 0;
            }
            if (fire) {
                CPtrList s(0xa);
                i32 scanHit = 0; // SearchEdge's out-slot (NOT a list field: `s` is
                                 // never passed to it - only this cell's address is)
                i32 res = grid->SearchEdge(
                    c,
                    r,
                    co->m_x,
                    co->m_y,
                    &scanHit,
                    1,
                    m_arrivalFlags | 0x20000000,
                    m_24c
                );
                if (res != 0) {
                    if (scanHit != 0) {
                        hitFound = 1;
                        break;
                    }
                } else {
                    hits++;
                }
            }
        }
        if (hits == 5) {
            break;
        }
    }

    if (hitFound) {
        // recover the remaining tracked nodes onto the free-list
        while (node != 0) {
            GruntCoordNode* cur = node;
            node = node->m_next;
            if (g_coordPool.m_freeHead != 0) {
                GruntCoord* co = cur->m_coord;
                void** fn = reinterpret_cast<void**>(g_coordPool.m_freeHead);
                fn[0] = reinterpret_cast<void*>(co->m_x);
                fn[1] = reinterpret_cast<void*>(co->m_y);
                g_coordPool.m_freeHead = static_cast<CoordPoolNode*>(*fn);
                m_31c.AddTail(fn);
            }
        }
        if (CoordCount() != 0) {
            GruntCoordNode* nd = CoordHead();
            while (nd != 0) {
                void* r = ListNodeAdvance(reinterpret_cast<void**>(&nd));
                if (*static_cast<i32*>(r) != 0) {
                    g_coordPool.Push(reinterpret_cast<void*>(*static_cast<i32*>(r)));
                }
            }
            m_31c.RemoveAll();
        }
        void* elem = m_31c.RemoveHead();
        if (elem != 0) {
            FREELIST_PUSH(elem);
        }
        m_31c.RemoveAll();
        SCAN_BOUNDS(grid);
        return 1;
    }

    // ---- no hit: 9x9 neighbour re-scan ----
    SCAN_BOUNDS(grid);
    i32 nl = col5 - 4;
    i32 nr = col5 + 4;
    i32 nt = row5 - 4;
    i32 nb = row5 + 4;
    if (col5 >= nl && col5 < nr && row5 >= nt && row5 < nb) {
        SCAN_BOUNDS(grid);
        for (i32 dy = 0; dy < 2; dy++) {
            for (i32 dx = 0; dx < 2; dx++) {
                i32 rr = row5 + dy;
                i32 cc = col5 * 7 + dx;
                i32 cf = 1;
                if (static_cast<u32>(rr) < static_cast<u32>(grid->m_height)
                    && static_cast<u32>(cc) < static_cast<u32>(grid->m_width)) {
                    cf = ((grid->m_rowInts[rr]))[cc];
                }
                if (((m_arrivalFlags | 0x20040002) & cf) & 0x20000000) {
                    continue;
                }
                if (((m_arrivalFlags | 0x20040002) & cf) != 0 && (m_24c & cf) == 0) {
                    continue;
                }
                CPtrList s(0xa);
                i32 scanHit = 0; // SearchEdge's out-slot (NOT a list field: `s` is
                                 // never passed to it - only this cell's address is)
                i32 res = grid->SearchEdge(
                    col5,
                    row5,
                    col5,
                    row5,
                    &scanHit,
                    1,
                    m_arrivalFlags | 0x20040002,
                    m_24c
                );
                if (res != 0 && scanHit != 0) {
                    void* elem = s.RemoveHead();
                    if (elem != 0) {
                        FREELIST_PUSH(elem);
                    }
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
// regalloc wall: logic/CFG/member-offsets/cue-ids byte-exact (the on-screen push
// blocks match verbatim); residue = retail pins `this` in esi (push esi; mov
// esi,ecx) for the whole 4-branch body, mine keeps it in ecx and reloads the cue
// sink per branch in a different slot - pure register placement, no source lever
// flips it. ~76%, deferred to the final sweep.
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
            i32* vr = &g->m_world->m_level->m_mainPlane->m_originX;
            if (x < vr[2] && x >= vr[0] && y < vr[3] && y >= vr[1]) {
                g->m_cueSink->SpawnVoiceDriver(reinterpret_cast<i32>(this), 0x370, -1, 0, -1, -1);
            }
            return;
        }
        CGruntzMgr* g = g_gameReg;
        i32* vr = &g->m_world->m_level->m_mainPlane->m_originX;
        if (x < vr[2] && x >= vr[0] && y < vr[3] && y >= vr[1]) {
            g->m_cueSink->SpawnVoiceDriver(reinterpret_cast<i32>(this), 0x371, -1, 0, -1, -1);
        } else {
            m_struckCount = 0;
        }
        return;
    }

    if (c < 5) {
        i32 x = m_object->m_screenX;
        i32 y = m_object->m_screenY;
        CGruntzMgr* g = g_gameReg;
        i32* vr = &g->m_world->m_level->m_mainPlane->m_originX;
        if (x < vr[2] && x >= vr[0] && y < vr[3] && y >= vr[1]) {
            g->m_cueSink->SpawnVoiceDriver(reinterpret_cast<i32>(this), 0x320, -1, 0, -1, -1);
        }
        return;
    }
    if (c < 0xa) {
        i32 x = m_object->m_screenX;
        i32 y = m_object->m_screenY;
        CGruntzMgr* g = g_gameReg;
        i32* vr = &g->m_world->m_level->m_mainPlane->m_originX;
        if (x < vr[2] && x >= vr[0] && y < vr[3] && y >= vr[1]) {
            g->m_cueSink->SpawnVoiceDriver(reinterpret_cast<i32>(this), 0x321, -1, 0, -1, -1);
        }
        return;
    }
    {
        i32 x = m_object->m_screenX;
        i32 y = m_object->m_screenY;
        m_struckCount = 0;
        CGruntzMgr* g = g_gameReg;
        i32* vr = &g->m_world->m_level->m_mainPlane->m_originX;
        if (x < vr[2] && x >= vr[0] && y < vr[3] && y >= vr[1]) {
            g->m_cueSink->SpawnVoiceDriver(reinterpret_cast<i32>(this), 0x322, -1, 0, -1, -1);
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
// large-state-machine + custom-resolver-internals plateau (sibling of RunEntranceMove
// 0x67850 / StepEntranceReinit 0x637a0): CFG, the switch jump-table mapping, every
// member offset/gate, the 15-stride tile index, the RectContains-gated commit, and the
// three sequential strcmp-reject cell-resolves are reconstructed in shape/order. Residue:
// the resolver coord-range field reads + the two distinct cell-record fallback helpers
// reloc-mask to differently-named externals, the inline-strcmp setcc sentinel pinning,
// and the cross-block regalloc on the shared resolve tail. Deferred to the final sweep.
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
                    OnReanchor(0);
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
        i32 coord = reinterpret_cast<i32>(m_objAux->m_1c);
        g_typeColl.m_grown = 0;
        i32 rec;
        if (coord < g_cellLo || coord > g_cellHi) {
            if (g_typeColl.MapCellIndex(coord, 0) != 0) {
                rec = (coord - g_cellLo) * g_cellScale + g_cellBase;
            } else {
                g_typeColl.MapCellRecord(g_cellRecordBase, 0xc);
                rec = g_cellRet;
            }
        } else {
            rec = (coord - g_cellLo) * g_cellScale + g_cellBase;
        }
        GruntScratchTeardown();
        static_cast<void>(rec);
    }
    char* nm1 = *g_typeColl.GetNameRecord(m_objAux->m_1c);
    if (strcmp(nm1, s_codeF) == 0) {
        return 1;
    }
    {
        i32 coord = reinterpret_cast<i32>(m_objAux->m_1c);
        g_typeColl.m_grown = 0;
        i32 rec;
        if (coord < g_cellLo || coord > g_cellHi) {
            if (g_typeColl.MapCellIndex(coord, 0) != 0) {
                rec = (coord - g_cellLo) * g_cellScale + g_cellBase;
            } else {
                i32 pin = g_typeColl.PinCellIndex();
                g_cellRecordRet = g_typeColl.MapCellRecord2(g_cellRecordBase, 0xc);
                rec = g_cellRet;
                static_cast<void>(pin);
            }
        } else {
            rec = (coord - g_cellLo) * g_cellScale + g_cellBase;
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
    i32 a0,
    i32 a1,
    i32 a2,
    i32 a3,
    i32 a4,
    i32 a5,
    i32 a6,
    i32 a7
) {
    if (this->m_gruntKind == 0x38 && this->m_entranceReason != 1) {
        return 1;
    }

    // a7 == 0x39: conversion hit - heal the struck enemy, fire GAME_CONVERSIONHIT.
    if (a7 == 0x39) {
        CGrunt* enemy = m_tileMgr->m_grid[a2 * TM_GRID_COLS + a3];
        if (enemy != 0
            && m_tileMgr->SpawnGrunt(
                   this->m_tileOwnerHi,
                   this->m_tileOwnerLo,
                   a2,
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
                (static_cast<CDDrawSurfaceMgr*>(m_3c->m_0c))->m_soundRegistry;
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
    i32 hit = g_hitTable[this->m_entranceReason * 23 + a0];
    CGruntzMgr* reg = g_gameReg; // cached once (retail keeps the singleton in a reg)
    if (reg->m_isEasyMode != 0 && reg->m_134 == 1 && this->m_tileOwnerHi == g_curPlayer) {
        i32 t = hit / 2;
        hit = t + t % 5;
    }

    // Reactive-armor kind (0x3c == GRUNT_REACTIVEARMOR): scale the hit by g_dtScale, then damage the enemy.
    if (a7 == 0x3a) {
        hit = 0x64;
    } else if (this->m_gruntKind == 0x3c) {
        hit = static_cast<i32>((static_cast<float>(hit) * g_dtScale));
        if (a6 == 0) {
            CGrunt* enemy = m_tileMgr->m_grid[a2 * TM_GRID_COLS + a3];
            if (enemy != 0 && enemy->m_entranceCommitted != 0) {
                i32 nh = enemy->m_health - hit * 3;
                if (nh < 0) {
                    nh = 0;
                }
                enemy->m_health = nh;
                if (nh <= 0) {
                    m_tileMgr->CellDispatch(a2, a3, 1, -1);
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
        m_tileMgr->CellDispatch(this->m_tileOwnerHi, this->m_tileOwnerLo, 1, a2);
        return 0;
    }
    if (nh <= 0) {
        this->m_entranceCommitted = 0;
        this->m_370 = a2;
    }

    // On-screen visibility gate, then the hit/block sound-cue resolve.
    LeafCue* cue = 0;
    i32 vx = this->m_object->m_screenX;
    i32 vy = this->m_object->m_screenY;
    if (vx < reg->m_viewOriginR && vx >= reg->m_viewOriginL && vy < reg->m_viewOriginB
        && vy >= reg->m_viewOriginT) {
        if (a7 == 0x3a) {
            LK(s_DEATHTOUCHHIT);
            goto L_cue;
        }
        if (a0 == 6 || a0 == 0xa || a0 == 0x16) {
            if (this->m_entranceReason == 8) {
                LK(s_BLOCKBODY2);
            } else {
                LK(s_IMPACTMM2);
            }
            goto L_cue;
        }
        if (this->m_entranceReason == 9) {
            if (a0 == 5 || a0 == 0xd || a0 == 0xe || a0 == 4) {
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
            if (a1 == 1) {
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
        switch (a0) {
            case 0:
                if (a1 == 0) {
                    LK(s_BLOCKBODY2);
                } else {
                    LK(s_IMPACTMM1);
                }
                break;
            case 2:
                LK(s_IMPACTMM1);
                break;
            case 3:
                if (a1 == 0) {
                    LK(s_BLOCKBODY2);
                } else {
                    LK(s_IMPACTMM4);
                }
                break;
            case 4:
                if (a1 == 0) {
                    LK(s_BLOCKBODY2);
                } else {
                    LK(s_IMPACTMM4);
                }
                break;
            case 5:
                if (a1 == 0) {
                    LK(s_BLOCKBODY2);
                } else {
                    LK(s_IMPACTMM3);
                }
                break;
            case 7:
                if (a1 == 0) {
                    LK(s_BLOCKBODY2);
                } else {
                    LK(s_IMPACTWM1);
                }
                break;
            case 8:
                if (a1 == 0) {
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
                if (a1 == 0) {
                    LK(s_BLOCKBODY1);
                } else {
                    LK(s_IMPACTMM4);
                }
                break;
            case 0xd:
                if (a1 == 0) {
                    LK(s_BLOCKMETAL1);
                } else {
                    LK(s_IMPACTMM4);
                }
                break;
            case 0xe:
                if (a1 == 0) {
                    LK(s_BLOCKBODY2);
                } else {
                    LK(s_IMPACTWM3);
                }
                break;
            case 0xf:
                if (a1 == 0) {
                    LK(s_BLOCKBODY2);
                } else {
                    LK(s_IMPACTMM1);
                }
                break;
            case 0x10:
                if (a1 == 0) {
                    LK(s_BLOCKBODY2);
                } else {
                    LK(s_IMPACTMM3);
                }
                break;
            case 0x12:
                if (a1 == 0) {
                    LK(s_BLOCKBODY2);
                } else {
                    LK(s_IMPACTMM1);
                }
                break;
            case 0x13:
                if (a1 == 0) {
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

    // Block path (a0 in {6,0xa,0x16}); otherwise reason 0x15 kills, else return.
    if (!(a0 == 6 || a0 == 0xa || a0 == 0x16)) {
        if (a0 != 0x15) {
            return 1;
        }
        if (this->m_health > 0) {
            return 1;
        }
        m_tileMgr->CellDispatch(this->m_tileOwnerHi, this->m_tileOwnerLo, 7, a2);
        return 0;
    }

    if (this->m_entranceReason == 8) {
        return 1;
    }

    // Rebuild the active-anim-set type-name registry free list.
    char** typeRec =
        reinterpret_cast<char**>((static_cast<_zvec*>(&g_typeColl))
                                     ->IndexToPtr(reinterpret_cast<i32>((this->m_objAux->m_1c))));
    if (g_typeColl.m_grown != 0) {
        char* p = reinterpret_cast<char*>(g_typeColl.m_alloc);
        i32 n = g_typeColl.m_grown;
        do {
            if (p != 0) {
                new (static_cast<void*>(p)) CString();
            }
            p += 4;
        } while (--n != 0);
    }
    if (strcmp(*typeRec, s_typeO) == 0) {
        return 1;
    }

    // x87 angle-octant direction resolver: copy the matching g_dirVec triple into
    // CGrunt+0x43c and set the target tile pixel (newX/newY).
    i32 dy = a5 - this->m_object->m_screenY;
    i32 dx = a4 - this->m_object->m_screenX;
    i32 newX;
    i32 newY;
    if (a0 == 0x16) {
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
        if (a5 > this->m_object->m_screenY) {
            SETDIR(2, this->m_lastTilePxX, this->m_lastTilePxY - 0x20);
        } else if (a5 < this->m_object->m_screenY) {
            SETDIR(1, this->m_lastTilePxX, this->m_lastTilePxY + 0x20);
        } else {
            goto L_moveDone;
        }
    } else {
        float slope = static_cast<float>(dy) / dx;
        if (slope > g_tanC0 || slope < g_tanC1) {
            if (a5 > this->m_object->m_screenY) {
                SETDIR(2, this->m_lastTilePxX, this->m_lastTilePxY - 0x20);
            } else {
                SETDIR(1, this->m_lastTilePxX, this->m_lastTilePxY + 0x20);
            }
        } else if (slope > g_tanC2 || slope < g_tanC3) {
            if (slope > g_tanC2) {
                if (a4 > this->m_object->m_screenX) {
                    SETDIR(6, this->m_lastTilePxX - 0x20, this->m_lastTilePxY - 0x20);
                } else {
                    SETDIR(5, this->m_lastTilePxX + 0x20, this->m_lastTilePxY + 0x20);
                }
            } else if (slope < g_tanC3) {
                if (a4 > this->m_object->m_screenX) {
                    SETDIR(4, this->m_lastTilePxX - 0x20, this->m_lastTilePxY + 0x20);
                } else {
                    SETDIR(8, this->m_lastTilePxX + 0x20, this->m_lastTilePxY - 0x20);
                }
            } else {
                goto L_moveDone;
            }
        } else {
            if (a4 > this->m_object->m_screenX) {
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
        i32* oc = g2->m_rowInts[oy] + ox * 7;
        *(reinterpret_cast<unsigned char*>(oc) + 3) &= 0xdf;
        i32* oc2 = g2->m_rowInts[oy] + ox * 7;
        oc2[1] = -1;
        i32* nc = g2->m_rowInts[nyt] + nxt * 7;
        *(reinterpret_cast<unsigned char*>(nc) + 3) |= 0x20;
        i32* nc2 = g2->m_rowInts[nyt] + nxt * 7;
        nc2[1] = (this->m_tileOwnerHi << 8) | this->m_tileOwnerLo;

        if (m_31c.GetCount() != 0) {
            i32* node = 0;
            i32 rx = this->m_lastTilePxX >> 5;
            i32 ry = this->m_lastTilePxY >> 5;
            if (*reinterpret_cast<void**>(g_coordPool.m_freeHead) != 0) {
                node = reinterpret_cast<i32*>(&g_coordPool.m_freeHead->m_coord);
                node[0] = rx;
                node[1] = ry;
                g_coordPool.m_freeHead = g_coordPool.m_freeHead->m_next;
            }
            m_31c.AddHead(node);
        }

        this->m_lastTilePxX = newX;
        this->m_lastTilePxY = newY;
        this->m_prevAnimSetNode = this->m_objAux->m_1c;
        this->m_objAux->m_1c = g_buteTree.Find(s_typeO);
        double ddx = static_cast<double>(newX) - this->m_object->m_screenX;
        double ddy = static_cast<double>(newY) - this->m_object->m_screenY;
        double dist = sqrt(ddx * ddx + ddy * ddy);
        u32 kb = g_buteMgr.GetDwordDef(s_gruntSec, s_knockKey, 200);
        m_400 = dist / static_cast<double>(kb);
        m_408 = static_cast<double>((this->m_object->m_screenX));
        m_410 = static_cast<double>((this->m_object->m_screenY));

        if (m_31c.GetCount() != 0) {
            CoordNode* node = reinterpret_cast<CoordNode*>(m_31c.GetHeadPosition());
            if (node != 0) {
                CoordPoolNode* fl = g_coordPool.m_freeHead;
                do {
                    CoordNode* cur = node;
                    node = cur->m_next;
                    Coord* data = cur->m_coord;
                    if (data != 0) {
                        CoordPoolNode* slot = g_coordPool.NodeOf(data);
                        slot->m_next = fl;
                        fl = slot;
                        g_coordPool.m_freeHead = fl;
                    }
                } while (node != 0);
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
// (live, both committed, not anim "F"); dispatch on m_170/m_19c (==1 -> a move
// config) else on the current anim type code ("I" -> arrival re-notify; "N" -> the
// align-down/drop-ready/snap re-latch); finally run the shared combat finalize:
// commit the in-flight move, latch m_220, build the neighbour's HUD health sprite +
// combat timer, recycle both arrival blocks, and (when not low-stamina/active)
// re-arm the attack anim. __thiscall, ret 0x10; returns 1 on success, 0 on bail.
//
// @early-stop
// large-state-machine + grid-regalloc plateau: the dispatch CFG, all three strcmp
// type-code arms (the inline-strcmp setcc form), the combat-timer + 15-wide cell grid
// index, the align-down/IsDropReady block, and the dual ArrivalRecycle finalize are
// reconstructed in shape/order. Residue = the deep g_gameReg->m_tileGrid board chain modeled
// by raw offset, the cross-arm regalloc (the neighbour `ebx` pinned across the tail),
// and the strcmp-eq sentinel pinning. Deferred to the final sweep.
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
            flags = (reinterpret_cast<i32*>(bd->m_rowBytes[ty]))[tx * 7];
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
    i32 v = m_entranceReason;
    if (v > 0x16) {
        v = m_19c;
    }
    if (v == 1) {
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
            i32 px = (m_object->m_screenX & ~0x1f) + 0x10;
            i32 py = (m_object->m_screenY & ~0x1f) + 0x10;
            i32 redo = 1;
            if (px != m_lastTilePxX || py != m_lastTilePxY) {
                if (IsDropReady(1)) {
                    m_coordToggle = (m_coordToggle == 0);
                    redo = 0;
                }
            }
            SnapToLastTile(1);
            if (redo) {
                m_prevAnimSetNode = m_objAux->m_1c;
                m_objAux->m_1c = static_cast<void*>(g_buteTree.Find(s_codeD));
                OnCoordCommit(m_coordToggle);
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
// ~78% (was 53.7%: the mislabeled note claimed byte-faithful, but retail SHARES one
// return-0 tail all gates jump to - shared `goto fail` merges them - INLINES the scratch
// teardown loop (marked inline) and defers the record->m_name load past it). Residual:
// the inlined teardown's loop-induction form (retail dec/lea pre-adjusts the counter vs
// cl's test/use) + the inline-strcmp result-bool register (retail eax, cl ecx). Both
// MSVC5 /O2 coin-flips; not source-steerable.
RVA(0x0005b570, 0x12b)
i32 CGrunt::BeginAttack(i32 a, i32 b) {
    if (m_entranceCommitted == 0) {
        goto fail;
    }
    {
        // retail defers the ->m_name load past the (inlined) scratch teardown loop
        CAnimNameRecord* rec = g_typeColl.GetNameRecords(m_objAux->m_1c);
        GruntScratchTeardown();
        bool eq = (strcmp(rec->m_name, s_codeF) == 0);
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
    switch (reinterpret_cast<u32>(rec->m_1c)) {
        case 0: {
            rec->m_1c = reinterpret_cast<void*>(0x3e8);
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

RVA(0x0005bc50, 0x15)
void ConstructActRange_644af0() {
    g_reg_644af0.Construct(0x7d0, 0x7da);
}

RVA(0x0005bcd0, 0x102)
void CGrunt::FireActivation(i32 id) {
    CGruntActEntry* e = reinterpret_cast<CGruntActEntry*>(g_reg_644af0.ResolveEntry(id));
    if (e->m_fn != 0) {
        (this->*(reinterpret_cast<CGruntActEntry*>(g_reg_644af0.ResolveEntry(id)))->m_fn)();
    }
}

// @early-stop
// count-down free-loop induction wall (~94.7%): every Find/Insert/name-lookup/
// Assign/table-store + all 19 keys and handlers are byte-faithful, both lookups
// outline to the shared helper as retail does and all 19 blocks expand inline.
// Sole residual is the `ecx=cnt; eax=cnt-1; lea ebx,[eax+1]` node-free idiom + the
// slot-vs-id callee-saved coloring repeated per block. Not source-steerable.
RVA(0x0005be30, 0x9e5)
void RegisterActs_644af0() {
    REGISTER_KEY_644AF0("A", &CGrunt::ResolveEntranceArrival);
    REGISTER_KEY_644AF0("B", &CGrunt::StepWarpExit);
    REGISTER_KEY_644AF0("C", &CGruntBehaviorLeaf::LoadGruntDecayConfig);
    REGISTER_KEY_644AF0(s_codeD, &CGrunt::StepArrivalReroll);
    REGISTER_KEY_644AF0("E", &CGrunt::UpdateGruntStatus);
    REGISTER_KEY_644AF0(s_codeF, &CGrunt::DispatchVtbl24);
    REGISTER_KEY_644AF0("G", &CGrunt::StepEntranceRelatchA);
    REGISTER_KEY_644AF0(s_codeH, &CGrunt::StepArrivalCommitA);
    REGISTER_KEY_644AF0("I", &CGruntBehaviorLeaf::LoadWandGruntItemConfig);
    REGISTER_KEY_644AF0("J", &CGrunt::RunEntranceMove);
    REGISTER_KEY_644AF0(s_codeK, &CGrunt::LoadEntranceConfig);
    REGISTER_KEY_644AF0("L", &CGrunt::LoadVehicleGruntAnimations);
    REGISTER_KEY_644AF0(s_codeM, &CGrunt::RearmEntranceDrop);
    REGISTER_KEY_644AF0(s_codeN, &CGrunt::StepEntranceRelatchB);
    REGISTER_KEY_644AF0(s_codeO, &CGrunt::StepArrivalCommitB);
    REGISTER_KEY_644AF0("P", &CGrunt::UpdateEntranceAnim);
    REGISTER_KEY_644AF0(s_codeQ, &CGrunt::LoadFreezeSpellAssets);
    REGISTER_KEY_644AF0("R", &CGruntBehaviorLeaf::LoadGruntDecayConfig2);
    REGISTER_KEY_644AF0(k_60df94, &CGrunt::FinishEntranceMove);
}
// ---------------------------------------------------------------------------
// CGrunt::Activate()   @0x5caa0   (__thiscall, ret 0)
// The grunt reset/spawn-init step. Fills the per-direction velocity-vector table at
// this+0x4b0 (9 directions, each a 0x78-stride record, 4 doubles/record) from the 9
// runtime direction-index globals (0x644aa0..0x644b48; index = 3*dir[0] + dir[1]) with
// the unit/diagonal direction vectors (0, +-1.0, +-0.5, +-sqrt(2)/2), then resets the
// grunt's spawn state: HUD anchor, health/stamina (100), the entrance flags, the latches.
//
// @early-stop
// x87 FP instruction-scheduling wall (same family as ComputeFacing 0x57060): the integer
// stores, the 9 unrolled direction records, the 3*lo+hi index math, and the trailing
// stat/flag reset are reconstructed faithfully, but MSVC's fld/fst/fstp/fdivr/fdiv stack
// juggling for sqrt(2.0) and the 1/sqrt2 diagonals (the `fld st(1)` scheduling) rarely
// matches from C source. Deferred to the final sweep.
RVA(0x0005caa0, 0x5e4)
void CGrunt::Activate() {
    double diag = sqrt(g_dirConst2); // sqrt(2.0)
    // the per-direction m_cells records (stride 0x68; retail index math is 13*(3*lo+hi)
    // doubles == exactly &m_cells[3*lo+hi].m_dirX - the old "15-double stride" tbl was
    // a mis-decode writing past the record for every index > 0)

    double s = g_dirConst1 / diag; // 1 / sqrt2
    double n = g_dirConstN1 / s;   // -1 / (1/sqrt2)

    // Each record: 4 doubles at the cell's +0/8/0x10/0x18. The 9 globals are processed
    // in this fixed order (ab0,ae0,aa0,b28,ac0,b48,ad0,b18,b38).
    {
        CGruntCellRec* c = &m_cells[3 * g_dirAb0[0] + g_dirAb0[1]];
        c->m_dirX = 0.0;
        c->m_dirY = -1.0;
        c->m_stepX = 0.0;
        c->m_stepY = -0.5;
    }
    {
        CGruntCellRec* c = &m_cells[3 * g_dirAe0[0] + g_dirAe0[1]];
        c->m_dirX = s;
        c->m_dirY = s;
        c->m_stepX = 0.5;
        c->m_stepY = -0.5;
    }
    {
        CGruntCellRec* c = &m_cells[3 * g_dirAa0[0] + g_dirAa0[1]];
        c->m_dirX = 1.0;
        c->m_dirY = 0.0;
        c->m_stepX = 0.5;
        c->m_stepY = 0.0;
    }
    {
        CGruntCellRec* c = &m_cells[3 * g_dirB28[0] + g_dirB28[1]];
        c->m_dirX = s;
        c->m_dirY = s;
        c->m_stepX = 0.5;
        c->m_stepY = 0.5;
    }
    {
        CGruntCellRec* c = &m_cells[3 * g_dirAc0[0] + g_dirAc0[1]];
        c->m_dirX = 0.0;
        c->m_dirY = 1.0;
        c->m_stepX = 0.0;
        c->m_stepY = 0.5;
    }
    {
        CGruntCellRec* c = &m_cells[3 * g_dirB48[0] + g_dirB48[1]];
        c->m_dirX = n;
        c->m_dirY = s;
        c->m_stepX = -0.5;
        c->m_stepY = 0.5;
    }
    {
        CGruntCellRec* c = &m_cells[3 * g_dirAd0[0] + g_dirAd0[1]];
        c->m_dirX = -1.0;
        c->m_dirY = 0.0;
        c->m_stepX = -0.5;
        c->m_stepY = 0.0;
    }
    {
        CGruntCellRec* c = &m_cells[3 * g_dirB18[0] + g_dirB18[1]];
        c->m_dirX = n;
        c->m_dirY = n;
        c->m_stepX = -0.5;
        c->m_stepY = -0.5;
    }
    {
        CGruntCellRec* c = &m_cells[3 * g_dirB38[0] + g_dirB38[1]];
        c->m_dirX = 0.0;
        c->m_dirY = 0.0;
        c->m_stepX = 0.0;
        c->m_stepY = 0.0;
    }

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
