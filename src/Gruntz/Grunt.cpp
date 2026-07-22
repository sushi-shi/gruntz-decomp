#include <Bute/ButeTree.h> // CButeTree::Find - g_buteTree @0x6bf620
#include <Gruntz/GruntSpawnConfig.h> // the +0x60 cue-sink/spawn-config object (complete type for the cue calls)
#include <Gruntz/GruntzMapMgr.h>  // the real +0x70 board class (ex GruntBoard view)
#include <Gruntz/Brickz.h>        // BrickzCell - the 0x1c-stride tile record (neighbor walks)
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GameLevel.h> // CGameLevel + CDDrawWorkerHost (m_world->m_level->m_mainPlane rect)
#include <DDrawMgr/DDrawSurfaceMgr.h> // the m_0c world root (m_animRegistry hop)
#include <DDrawMgr/DDrawSubMgrLeaf.h> // m_0c->m_animRegistry (the anim-key catalog)
#include <Gruntz/TriggerMgr.h>  // CTriggerMgr::ApplySwitch @0x6d300 (the ex-ApplyTileSwitch alias)
#include <Gruntz/TypeKeyColl.h> // g_typeColl (folded CAnimNameResolver anim registry)
#include <Gruntz/GruntHealthSprite.h> // CGruntHealthSprite::SetHealthGlyph (health/stamina/toytime/wingz)
#include <Gruntz/GruntToySprite.h>      // CGruntToySprite::SetCell
#include <Gruntz/GruntPowerupSprite.h>  // CGruntPowerupSprite::SetCell
#include <Gruntz/GruntSelectedSprite.h> // CGruntSelectedSprite::SetCell
#include <Gruntz/ActReg.h> // CLookupColl/CActReg::ResolveEntry (g_reg_644af0 dispatch, RunAct)
#include <Gruntz/AniElement.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/SerialRecords.h>
#include <Gruntz/MovingLogicSerial.h>
#include <Gruntz/BoundaryLowerMethodsViews.h>
#include <Gruntz/Effect6b.h>
#include <Dsndmgr/DirectSoundMgr.h>
#include <Dsndmgr/DirectSoundMgr.h>
#include <rva.h>
#include <Rez/FrameClock.h> // g_frameTicks (grunt birth-frame stamp)
#include <math.h>
#include <stdlib.h>
#include <string.h>

VTBL(CGrunt, 0x001e8754);

static const char s_GruntHealthSprite[] = "GruntHealthSprite";
static const char s_GruntToySprite[] = "GruntToySprite";
static const char s_GruntStaminaSprite[] = "GruntStaminaSprite";
static const char s_GruntToyTimeSprite[] = "GruntToyTimeSprite";
static const char s_GruntWingzTimeSprite[] = "GruntWingzTimeSprite";
static const char s_GruntPowerupSprite[] = "GruntPowerupSprite";
static const char s_GruntSelectedSprite[] = "GruntSelectedSprite";

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

i32 g_movingSeed;

#include <Bute/ButeMgr.h>

static char s_TimePerTile[] = "TimePerTile";
static char s_Grunt[] = "Grunt";                               // s_Grunt_0060a9ec
static char s_EntranceSafeTime[] = "EntranceSafeTime";         // s_EntranceSafeTime_0060df98
static char s_IdleDelay[] = "IdleDelay";                       // s_IdleDelay_0060e1a0
static char s_PlayerDefenderRadius[] = "PlayerDefenderRadius"; // s_PlayerDefenderRadius_0060e1ac
static char s_CombatTimeout[] = "CombatTimeout";               // s_CombatTimeout_0060df84

static const char s_GRUNTZ_DEATHZ_FREEZE[] = "GRUNTZ_DEATHZ_FREEZE";

static const char s_GRUNTZ_DEATHZ_SPARKLE[] = "GRUNTZ_DEATHZ_SPARKLE";   // 0x60ee48
static const char s_GRUNTZ_DEATHZ_UNFREEZE[] = "GRUNTZ_DEATHZ_UNFREEZE"; // 0x60ee1c
static char s_Spellz[] = "Spellz";                                       // 0x60cca8
static char s_FreezeDelay[] = "FreezeDelay";                             // 0x60ee38

static char s_BOMBGRUNT[] = "BOMBGRUNT";                   // 0x60dbd0
static char s_RunningTimePerTile[] = "RunningTimePerTile"; // 0x60e264

VTBL(CMovingLogic, 0x001e87ac);
DATA(0x002455b0)
i32 g_traitorMode; // 0x6455b0 - DEFINED once here; GruntCombat.cpp defined it too (LNK2005),

static const char s_animKeyA[] = "A";
static const char s_animKeyK[] = "K";

static void GruntScratchTeardown();

static const char s_pose_WALK[] = "_WALK";
static const char s_pose_ATTACK1[] = "_ATTACK1";
static const char s_pose_ATTACK2[] = "_ATTACK2";
static const char s_pose_ATTACKIDLE[] = "_ATTACK-IDLE";
static const char s_pose_STRUCK1[] = "_STRUCK1";
static const char s_pose_STRUCK2[] = "_STRUCK2";
static const char s_pose_IDLE1[] = "_IDLE1";
static const char s_pose_IDLE2[] = "_IDLE2";
static const char s_pose_IDLE3[] = "_IDLE3";
static const char s_pose_IDLE4[] = "_IDLE4";
static const char s_pose_IDLE5[] = "_IDLE5";
static const char s_pose_ITEM[] = "_ITEM";
static const char s_pose_ITEM2[] = "_ITEM2";
static const char s_pose_DEATH[] = "_DEATH";
static const char s_pose_TOY1[] = "_TOY1";
static const char s_pose_TOY2[] = "_TOY2";
static const char s_pose_TOYBREAK[] = "_TOY-BREAK";

#define LOAD_POSE(dst, sfx)                                                                        \
    do {                                                                                           \
        CAniElement* _out = 0;                                                                     \
        m_38->OwnerMgr()->m_animRegistry->m_10.Lookup(                                             \
            "GRUNTZ_" + m_animSetName + (sfx),                                                     \
            reinterpret_cast<void*&>(_out)                                                         \
        );                                                                                         \
        (dst) = _out;                                                                              \
    } while (0)

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

void GruntRecycleCoords(CGrunt* g) {
    GruntCoordNode* n = g->CoordHead();
    while (n != 0) {
        GruntCoordNode* cur = n;
        n = n->m_next;
        if (cur->m_coord != 0) {
            CoordPoolNode* node = g_coordPool.NodeOf(cur->m_coord);
            node->m_next = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = node;
        }
    }
    g->m_31c.RemoveAll();
}

static void GruntScratchTeardown() {
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

// ---------------------------------------------------------------------------
// CGrunt::~CGrunt   @0xf2f0   (__thiscall, /GX leaf dtor)
// CGrunt is now a real polymorphic CUserLogic leaf, so the compiler auto-emits
// the /GX frame + the three vptr restamps (CGrunt 0x5e8754 -> CUserLogic 0x5e705c
// -> CUserBase 0x5e70b4) and the per-member descending trylevel teardown: the body
// runs UserLogicVfunc9, then MSVC destructs the six owned members in reverse-decl
// order (m_468[9] via __ehvec_dtor, m_44c/m_448 ~CString, m_338/m_31c ~CPtrList,
// m_animSetName@+0x1c0 ~CString), folds ~CUserLogic (the +0x18 EngStr link) then
// ~CUserBase. Every teardown callee is external/reloc-masked.
//
// @early-stop
// EH-state-base-numbering wall (docs/patterns/eh-dtor-multilevel-polymorphic-chain.md
// + eh-state-numbering-base.md): the real polymorphic CUserBase<-CUserLogic<-CGrunt
// chain now auto-emits the /GX frame, the three vptr restamps (0x5e8754 -> 0x5e705c
// -> 0x5e70b4), the per-member __ehvec_dtor + ~CString/~CPtrList/~EngStr teardowns in
// retail order, and the descending trylevel chain - all byte-faithful in
// shape/order (55.5% -> 94.9%). The COUNT of EH states matches (8), but retail numbers
// them 1..8 (UserLogicVfunc9 region=7, six members 6..1, base m_18=8) while MSVC numbers
// mine 0..7 (off by one) because retail reserves state 0 for the CUserLogic base
// subobject construction, and it reserves an extra local dword (`sub esp,8` vs my
// `push ecx`; `add esp,0x14` vs `0x10`). Closing this needs the base construction
// state propagated into the derived dtor's state table (the CUserLogic ctor visible in
// this TU) - deferred to the final sweep.
RVA(0x0000f2f0, 0xc8)
CGrunt::~CGrunt() {
    UserLogicVfunc9();
}

static const char s_NORMALGRUNT[] = "NORMALGRUNT"; // 0x60d404

// @early-stop
// member-init/body-split wall (~67%): logic/CFG/field offsets/moving-init all
// byte-faithful. CGrunt rides the Gruntz-module SNAPSHOT of CMovingLogic (lean 0x30;
// Grunt.h) - the +0x120 header-layout fix, so every field-init hits its true retail
// offset (m_400 @+0x400 etc.). The base ctor's intermediate vptr stamp now emits the
// REAL ??_7CMovingLogic (0x5e87ac binds; the ex-CGruntMovingBase rename made it an
// unbindable reloc). Remaining residue: (b) MSVC runs the six owned value-member ctors
// (m_animSetName/m_31c/m_338/m_448/m_44c/m_468[9]) in the member-init PHASE while retail
// interleaves them among the scalar inits - but they must stay value-typed for ~CGrunt's
// auto __ehvec_dtor/~CString/~CPtrList teardown (94.9%). (c) the +0x810 timer band's
// lo/hi dword interleave + the /GX EH-state numbering. All entropy/ordering class; the
// vptr residue is byte-verified (llvm-objdump: only the intermediate stamp reloc differs).
// Deferred to the final sweep.

RVA(0x00047a10, 0x770)
CGrunt::CGrunt(void* owner) : CMovingLogic(static_cast<CGameObject*>(owner)) {
    // The band init - THIS ctor's own copy (its projectile sibling @0xdec60 carries
    // a drifted copy: g_motionZScale for the step + the inline Z triple). Bounds
    // via the Motion() view: 0 => MIN/MAX dword copy, else fild-widened int.
    CMotionState* m = Motion();
    m->Init();
    i32 lo0 = m_objAux->m_2c;
    if (lo0 == 0) {
        m->m_70 = g_movingLogicMin;
    } else {
        m->m_70 = static_cast<double>(lo0);
    }
    i32 lo1 = m_objAux->m_34;
    if (lo1 == 0) {
        m->m_78 = g_movingLogicMin;
    } else {
        m->m_78 = static_cast<double>(lo1);
    }
    i32 hi0 = m_objAux->m_30;
    if (hi0 == 0) {
        m->m_88 = g_movingLogicMax;
    } else {
        m->m_88 = static_cast<double>(hi0);
    }
    i32 hi1 = m_objAux->m_38;
    if (hi1 == 0) {
        m->m_90 = g_movingLogicMax;
    } else {
        m->m_90 = static_cast<double>(hi1);
    }
    m->SetParams(
        static_cast<double>(m_object->m_screenX),
        static_cast<double>(m_object->m_screenY),
        0.0,
        static_cast<double>(m_object->m_164),
        static_cast<double>(m_object->m_168),
        0.0,
        0.0,
        0.0,
        0.0,
        static_cast<double>(g_frameTime) * g_gruntSpawnScale,
        0.0
    );
    m->SetZ(static_cast<double>(g_defaultZ));
    // --- CGrunt field-init block (retail offset order) ---
    m_148 = 0;
    m_14c = 0;
    m_object->m_moveMode = 7;
    // The base moving-object per-frame update fired once at spawn - the qualified
    // call is direct, binding the slot-16 body @0x16ea90 (MovingLogic.cpp) for real.
    CMovingLogic::MovingSlot16();
    CGameObject* obj =
        static_cast<CGameObject*>(owner); // owner is void* (ctor mangling ??0CGrunt@@QAE@PAX@Z)
    m_34 = obj;
    m_38 = static_cast<CWwdGameObjectA*>(
        obj
    );                // the owner object doubles as the entrance player (A-kind)
    m_3c = obj->m_7c; // the bound object's AnimWorkerObj (typed)
    m_struckClockLo = 0;
    m_struckTimerLo = 0;
    m_struckClockHi = 0;
    m_struckTimerHi = 0;
    m_278 = 0;
    m_280 = 0;
    m_27c = 0;
    m_284 = 0;
    m_arrivalRerollLo = 0;
    m_arrivalRerollWindowLo = 0;
    m_arrivalRerollHi = 0;
    m_arrivalRerollWindowHi = 0;

    // The +0x810..+0x8cc timer band (24 doubles, zeroed).
    // The +0x810..+0x8cf timer-band zero run (retail's per-block lo,lo,hi,hi
    // dword order; ex the offset-cast GRUNT_ZERO_TIMER_* macros - typed now).
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
    m_850 = 0;
    m_858 = 0;
    m_854 = 0;
    m_85c = 0;
    m_860 = 0;
    m_attackDowntimeLo = 0;
    m_864 = 0;
    m_attackDowntimeHi = 0;
    m_combatClockLo = 0;
    m_combatTimeoutLo = 0;
    m_combatClockHi = 0;
    m_combatTimeoutHi = 0;
    m_880 = 0;
    m_888 = 0;
    m_884 = 0;
    m_88c = 0;
    m_wingzClockLo = 0;
    m_wingzDurationLo = 0;
    m_wingzClockHi = 0;
    m_wingzDurationHi = 0;
    m_8a0 = 0;
    m_8a8 = 0;
    m_8a4 = 0;
    m_8ac = 0;
    m_8b0 = 0;
    m_8b8 = 0;
    m_8b4 = 0;
    m_8bc = 0;
    m_8c0 = 0;
    m_8c8 = 0;
    m_8c4 = 0;
    m_8cc = 0;

    // Second-phase field inits (post CGrunt vptr restamp).
    m_entranceCell.col = g_gruntDefEntranceCell[0];
    m_entranceCell.row = g_gruntDefEntranceCell[1];
    m_entranceCell.reason = g_gruntDefEntranceCell[2];
    m_434 = m_object->m_11c;
    m_438 = g_frameTicks;
    m_object->m_moveMode = 1;
    m_430 = 0;
    m_42c = 0;
    m_poseWalk = 0;
    m_poseAttack1 = 0;
    m_poseAttack2 = 0;
    m_poseAttackIdle = 0;
    m_poseStruck1 = 0;
    m_poseStruck2 = 0;
    m_poseIdle[0] = 0;
    m_poseIdle[1] = 0;
    m_poseIdle[2] = 0;
    m_poseIdle4 = 0;
    m_poseIdle5 = 0;
    m_poseItem = 0;
    m_poseItem2 = 0;
    m_poseDeath = 0;
    m_poseToy1 = 0;
    m_poseToy2 = 0;
    m_poseToyBreak = 0;
    m_pickupGeoSrc = 0;
    m_arrived = 0;
    m_38->m_collCategory = 0x100000;
    m_38->m_ec = 0x3d1;
    m_38->m_flags |= 0x2000100;
    m_38->m_collMask |= 0x103f;
    m_38->m_f0 = 1; // +0xf0 (named below in UserLogic.h)
    m_tileOwnerHi = -1;
    m_tileOwnerLo = -1;
    m_neighborCol = -1;
    m_38c = 0;
    m_entranceReason = 0;
    m_198 = 0;
    m_194 = 0;
    m_gruntKind = 0;
    m_19c = 0;
    m_animSetName = s_NORMALGRUNT;
    m_neighborRow = -1;
    m_entranceCommitted = 1;
    m_healthSprite = 0;
    m_reachRectLeft = -1;
    m_staminaSprite = 0;
    m_toyTimeSprite = 0;
    m_wingzTimeSprite = 0;
    m_selectedSprite = 0;
    m_toySprite = 0;
    m_powerupSprite = 0;
    m_210 = 0;
    m_combatActive = 0;
    m_neighborValid = 0;
    m_arrivalActive = 0;
    m_coordToggle = 0;
    m_wingzEnabled = 0;
    m_tileClaimed = 0;
    m_struckVoiceSound = 0;
    m_reachRectTop = -1;
    m_reachRadius = 1;
    m_reachRectBottom = 1;
    m_2a0 = 0;
    m_2a4 = 0;
    m_2a8 = 0;
    m_2ac = 0;
    m_2b0 = 0;
    m_2b4 = 0;
    m_2b8 = 0;
    m_2bc = 0;
    m_2c0 = 0;
    m_2c4 = 0;
    m_2c8 = 0;
    m_2cc = 0;
    // The +0x810..+0x8cf timer-band zero run (retail's per-block lo,lo,hi,hi
    // dword order; ex the offset-cast GRUNT_ZERO_TIMER_* macros - typed now).
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
    m_850 = 0;
    m_858 = 0;
    m_854 = 0;
    m_85c = 0;
    m_860 = 0;
    m_attackDowntimeLo = 0;
    m_864 = 0;
    m_attackDowntimeHi = 0;
    m_combatClockLo = 0;
    m_combatTimeoutLo = 0;
    m_combatClockHi = 0;
    m_combatTimeoutHi = 0;
    m_880 = 0;
    m_888 = 0;
    m_884 = 0;
    m_88c = 0;
    m_wingzClockLo = 0;
    m_wingzDurationLo = 0;
    m_wingzClockHi = 0;
    m_wingzDurationHi = 0;
    m_8a0 = 0;
    m_8a8 = 0;
    m_8a4 = 0;
    m_8ac = 0;
    m_8b0 = 0;
    m_8b8 = 0;
    m_8b4 = 0;
    m_8bc = 0;
    m_8c0 = 0;
    m_8c8 = 0;
    m_8c4 = 0;
    m_8cc = 0;
    m_arrivalRerollLo = 0;
    m_arrivalRerollWindowLo = 0;
    m_arrivalRerollHi = 0;
    m_arrivalRerollWindowHi = 0;
    m_2f8 = -1;
    m_2fc = -1;
    m_arrivalNotified = 0;
    m_defenderState = 0;
    m_2d8 = 0;
    {
        CWwdGameObjectA* h = m_object;
        i32 lim = h->m_screenY + 0x186a0;
        if (h->m_sortKey != lim) {
            h->m_sortKey = lim;
            h->m_flags |= 0x20000;
        }
    }
    m_390 = 1;
}

DATA(0x00229ad0)
i32 g_serialCounter;

// @early-stop
// reloc-masked-symbol plateau: instruction stream byte-exact vs retail (verified
// llvm-objdump), but the two free-pool globals (g_coordPool.m_freeHead/Base) and the
// three engine calls (Coll::Reset, List::RemoveHead, node deleter) are unnamed,
// so their DIR32/REL32 operands pair to differently named retail symbols and
// score fuzzy. Naming the whole referent set is a final-sweep task.
// CGrunt::UserLogicVfunc9() @0x48360 - tears down the per-grunt name/animation
// caches: walks a small list at +0x320 returning each node's +0x8 buffer to a
// global free pool (head/base at 0x645544/0x64554c), empties the collection at
// +0x31c, then drains the name CPtrList at +0x338 (count = PayloadHead()->m_8; each node
// freed via the engine deleter).
RVA(0x00048360, 0x7e)
i32 CGrunt::UserLogicVfunc9() {
    if (CoordCount() != 0) {
        void** node = reinterpret_cast<void**>(CoordHead());
        if (node) {
            do {
                void* next = node[0];
                void* buf = node[2];
                if (buf) {
                    CoordPoolNode* slot = g_coordPool.NodeOf(buf);
                    slot->m_next = g_coordPool.m_freeHead;
                    g_coordPool.m_freeHead = slot;
                }
                node = static_cast<void**>(next);
            } while (node);
        }
        m_31c.RemoveAll();
    }

    while (1) {
        i32 n = PayloadCount();
        i32 count = n ? reinterpret_cast<i32>(m_338.GetHead()) : 0;
        if (count == 0) {
            return 0;
        }
        if (n == 0) {
            continue;
        }
        void* p = m_338.RemoveHead();
        GruntNode_Delete(p);
    }
    return 0;
}

RVA(0x00048400, 0x47)
void CGrunt::ReadConfigFromButeMgr() {
    m_18c = 0;
    m_418 = 0;

    m_timePerTile = g_buteMgr.GetDwordDef(
        const_cast<char*>(static_cast<const char*>(m_animSetName)),
        s_TimePerTile,
        1000
    );

    if (m_gruntKind == 0x37) {
        m_timePerTile >>= 1;
    }
}

static const char s_d48_NORTHWEST_WALK[] = "_NORTHWEST_WALK";
static const char s_d48_NORTH_WALK[] = "_NORTH_WALK";
static const char s_d48_NORTHEAST_WALK[] = "_NORTHEAST_WALK";
static const char s_d48_WEST_WALK[] = "_WEST_WALK";
static const char s_d48_EAST_WALK[] = "_EAST_WALK";
static const char s_d48_SOUTHWEST_WALK[] = "_SOUTHWEST_WALK";
static const char s_d48_SOUTH_WALK[] = "_SOUTH_WALK";
static const char s_d48_SOUTHEAST_WALK[] = "_SOUTHEAST_WALK";
static const char s_d48_NORTHWEST_STRUCK[] = "_NORTHWEST_STRUCK";
static const char s_d48_NORTH_STRUCK[] = "_NORTH_STRUCK";
static const char s_d48_NORTHEAST_STRUCK[] = "_NORTHEAST_STRUCK";
static const char s_d48_WEST_STRUCK[] = "_WEST_STRUCK";
static const char s_d48_EAST_STRUCK[] = "_EAST_STRUCK";
static const char s_d48_SOUTHWEST_STRUCK[] = "_SOUTHWEST_STRUCK";
static const char s_d48_SOUTH_STRUCK[] = "_SOUTH_STRUCK";
static const char s_d48_SOUTHEAST_STRUCK[] = "_SOUTHEAST_STRUCK";
static const char s_d48_NORTHWEST_ATTACK[] = "_NORTHWEST_ATTACK";
static const char s_d48_NORTH_ATTACK[] = "_NORTH_ATTACK";
static const char s_d48_NORTHEAST_ATTACK[] = "_NORTHEAST_ATTACK";
static const char s_d48_WEST_ATTACK[] = "_WEST_ATTACK";
static const char s_d48_EAST_ATTACK[] = "_EAST_ATTACK";
static const char s_d48_SOUTHWEST_ATTACK[] = "_SOUTHWEST_ATTACK";
static const char s_d48_SOUTH_ATTACK[] = "_SOUTH_ATTACK";
static const char s_d48_SOUTHEAST_ATTACK[] = "_SOUTHEAST_ATTACK";
static const char s_d48_NORTHWEST_IDLE[] = "_NORTHWEST_IDLE";
static const char s_d48_NORTH_IDLE[] = "_NORTH_IDLE";
static const char s_d48_NORTHEAST_IDLE[] = "_NORTHEAST_IDLE";
static const char s_d48_WEST_IDLE[] = "_WEST_IDLE";
static const char s_d48_EAST_IDLE[] = "_EAST_IDLE";
static const char s_d48_SOUTHWEST_IDLE[] = "_SOUTHWEST_IDLE";
static const char s_d48_SOUTH_IDLE[] = "_SOUTH_IDLE";
static const char s_d48_SOUTHEAST_IDLE[] = "_SOUTHEAST_IDLE";
static const char s_d48_NORTHWEST_ITEM[] = "_NORTHWEST_ITEM";
static const char s_d48_NORTH_ITEM[] = "_NORTH_ITEM";
static const char s_d48_NORTHEAST_ITEM[] = "_NORTHEAST_ITEM";
static const char s_d48_WEST_ITEM[] = "_WEST_ITEM";
static const char s_d48_EAST_ITEM[] = "_EAST_ITEM";
static const char s_d48_SOUTHWEST_ITEM[] = "_SOUTHWEST_ITEM";
static const char s_d48_SOUTH_ITEM[] = "_SOUTH_ITEM";
static const char s_d48_SOUTHEAST_ITEM[] = "_SOUTHEAST_ITEM";
static const char s_d48_DEATH[] = "_DEATH";
static const char s_d48_NORTHWEST[] = "_NORTHWEST";
static const char s_d48_NORTH[] = "_NORTH";
static const char s_d48_NORTHEAST[] = "_NORTHEAST";
static const char s_d48_WEST[] = "_WEST";
static const char s_d48_EAST[] = "_EAST";
static const char s_d48_SOUTHWEST[] = "_SOUTHWEST";
static const char s_d48_SOUTH[] = "_SOUTH";
static const char s_d48_SOUTHEAST[] = "_SOUTHEAST";
static const char s_d48_BREAK[] = "_BREAK";

RVA(0x00048470, 0x131b)
void CGrunt::LoadCellAnimNames(i32 kind, i32 dirOnly) {
    if (kind == 0) {
        m_cells[0].m_walk = s_GRUNTZ_ + m_animSetName + s_d48_NORTHWEST_WALK;
        m_cells[1].m_walk = s_GRUNTZ_ + m_animSetName + s_d48_NORTH_WALK;
        m_cells[2].m_walk = s_GRUNTZ_ + m_animSetName + s_d48_NORTHEAST_WALK;
        m_cells[3].m_walk = s_GRUNTZ_ + m_animSetName + s_d48_WEST_WALK;
        m_cells[4].m_walk = s_GRUNTZ_ + m_animSetName + s_d48_NORTH_WALK;
        m_cells[5].m_walk = s_GRUNTZ_ + m_animSetName + s_d48_EAST_WALK;
        m_cells[6].m_walk = s_GRUNTZ_ + m_animSetName + s_d48_SOUTHWEST_WALK;
        m_cells[7].m_walk = s_GRUNTZ_ + m_animSetName + s_d48_SOUTH_WALK;
        m_cells[8].m_walk = s_GRUNTZ_ + m_animSetName + s_d48_SOUTHEAST_WALK;
        m_cells[0].m_struck = s_GRUNTZ_ + m_animSetName + s_d48_NORTHWEST_STRUCK;
        m_cells[1].m_struck = s_GRUNTZ_ + m_animSetName + s_d48_NORTH_STRUCK;
        m_cells[2].m_struck = s_GRUNTZ_ + m_animSetName + s_d48_NORTHEAST_STRUCK;
        m_cells[3].m_struck = s_GRUNTZ_ + m_animSetName + s_d48_WEST_STRUCK;
        m_cells[4].m_struck = s_GRUNTZ_ + m_animSetName + s_d48_NORTH_STRUCK;
        m_cells[5].m_struck = s_GRUNTZ_ + m_animSetName + s_d48_EAST_STRUCK;
        m_cells[6].m_struck = s_GRUNTZ_ + m_animSetName + s_d48_SOUTHWEST_STRUCK;
        m_cells[7].m_struck = s_GRUNTZ_ + m_animSetName + s_d48_SOUTH_STRUCK;
        m_cells[8].m_struck = s_GRUNTZ_ + m_animSetName + s_d48_SOUTHEAST_STRUCK;
        m_cells[0].m_attack = s_GRUNTZ_ + m_animSetName + s_d48_NORTHWEST_ATTACK;
        m_cells[1].m_attack = s_GRUNTZ_ + m_animSetName + s_d48_NORTH_ATTACK;
        m_cells[2].m_attack = s_GRUNTZ_ + m_animSetName + s_d48_NORTHEAST_ATTACK;
        m_cells[3].m_attack = s_GRUNTZ_ + m_animSetName + s_d48_WEST_ATTACK;
        m_cells[4].m_attack = s_GRUNTZ_ + m_animSetName + s_d48_NORTH_ATTACK;
        m_cells[5].m_attack = s_GRUNTZ_ + m_animSetName + s_d48_EAST_ATTACK;
        m_cells[6].m_attack = s_GRUNTZ_ + m_animSetName + s_d48_SOUTHWEST_ATTACK;
        m_cells[7].m_attack = s_GRUNTZ_ + m_animSetName + s_d48_SOUTH_ATTACK;
        m_cells[8].m_attack = s_GRUNTZ_ + m_animSetName + s_d48_SOUTHEAST_ATTACK;
        m_cells[0].m_idle = s_GRUNTZ_ + m_animSetName + s_d48_NORTHWEST_IDLE;
        m_cells[1].m_idle = s_GRUNTZ_ + m_animSetName + s_d48_NORTH_IDLE;
        m_cells[2].m_idle = s_GRUNTZ_ + m_animSetName + s_d48_NORTHEAST_IDLE;
        m_cells[3].m_idle = s_GRUNTZ_ + m_animSetName + s_d48_WEST_IDLE;
        m_cells[4].m_idle = s_GRUNTZ_ + m_animSetName + s_d48_NORTH_IDLE;
        m_cells[5].m_idle = s_GRUNTZ_ + m_animSetName + s_d48_EAST_IDLE;
        m_cells[6].m_idle = s_GRUNTZ_ + m_animSetName + s_d48_SOUTHWEST_IDLE;
        m_cells[7].m_idle = s_GRUNTZ_ + m_animSetName + s_d48_SOUTH_IDLE;
        m_cells[8].m_idle = s_GRUNTZ_ + m_animSetName + s_d48_SOUTHEAST_IDLE;
        m_cells[0].m_item = s_GRUNTZ_ + m_animSetName + s_d48_NORTHWEST_ITEM;
        m_cells[1].m_item = s_GRUNTZ_ + m_animSetName + s_d48_NORTH_ITEM;
        m_cells[2].m_item = s_GRUNTZ_ + m_animSetName + s_d48_NORTHEAST_ITEM;
        m_cells[3].m_item = s_GRUNTZ_ + m_animSetName + s_d48_WEST_ITEM;
        m_cells[4].m_item = s_GRUNTZ_ + m_animSetName + s_d48_NORTH_ITEM;
        m_cells[5].m_item = s_GRUNTZ_ + m_animSetName + s_d48_EAST_ITEM;
        m_cells[6].m_item = s_GRUNTZ_ + m_animSetName + s_d48_SOUTHWEST_ITEM;
        m_cells[7].m_item = s_GRUNTZ_ + m_animSetName + s_d48_SOUTH_ITEM;
        m_cells[8].m_item = s_GRUNTZ_ + m_animSetName + s_d48_SOUTHEAST_ITEM;
        m_44c = s_GRUNTZ_ + m_animSetName + s_d48_DEATH;
    } else if (dirOnly != 0) {
        m_cells[0].m_walk = s_GRUNTZ_ + m_animSetName + s_d48_NORTHWEST;
        m_cells[1].m_walk = s_GRUNTZ_ + m_animSetName + s_d48_NORTH;
        m_cells[2].m_walk = s_GRUNTZ_ + m_animSetName + s_d48_NORTHEAST;
        m_cells[3].m_walk = s_GRUNTZ_ + m_animSetName + s_d48_WEST;
        m_cells[4].m_walk = s_GRUNTZ_ + m_animSetName + s_d48_NORTH;
        m_cells[5].m_walk = s_GRUNTZ_ + m_animSetName + s_d48_EAST;
        m_cells[6].m_walk = s_GRUNTZ_ + m_animSetName + s_d48_SOUTHWEST;
        m_cells[7].m_walk = s_GRUNTZ_ + m_animSetName + s_d48_SOUTH;
        m_cells[8].m_walk = s_GRUNTZ_ + m_animSetName + s_d48_SOUTHEAST;
        m_448 = s_GRUNTZ_ + m_animSetName + s_d48_BREAK;
    } else {
        m_448 = s_GRUNTZ_ + m_animSetName;
    }
    i32 sel = g_gameReg->m_spriteFactory->GetSel(m_1f4_moveIcon, kind);
    CWwdGameObjectA* h = m_object;
    i32 keep50 = h->m_drawFillCmd;
    h->m_drawActive = 1;
    h->m_drawFillCmd = keep50;
    h->m_drawFillArg = sel;
}

// @early-stop
// out-param zero-init scheduling wall (docs/patterns/outparam-zeroinit-scheduling.md):
// instruction MULTISET byte-identical vs retail (verified), logic/CFG/offsets exact;
// residue = retail SINKS the `out=0` store past the Lookup arg pushes + stores the
// table member before the temp-dtors where cl hoists/reorders, permuted per block
// across 22 near-identical Lookup blocks. Source-invariant. ~76%.
RVA(0x00049c60, 0x8d1)
void CGrunt::LoadAnimNameTable(i32 kind, i32 toyOnly) {
    if (kind == 0) {
        LOAD_POSE(m_poseWalk, s_pose_WALK);
        LOAD_POSE(m_poseAttack1, s_pose_ATTACK1);
        LOAD_POSE(m_poseAttack2, s_pose_ATTACK2);
        LOAD_POSE(m_poseAttackIdle, s_pose_ATTACKIDLE);
        LOAD_POSE(m_poseStruck1, s_pose_STRUCK1);
        LOAD_POSE(m_poseStruck2, s_pose_STRUCK2);
        LOAD_POSE(m_poseIdle[0], s_pose_IDLE1);
        LOAD_POSE(m_poseIdle[1], s_pose_IDLE2);
        LOAD_POSE(m_poseIdle[2], s_pose_IDLE3);
        LOAD_POSE(m_poseIdle4, s_pose_IDLE4);
        LOAD_POSE(m_poseIdle5, s_pose_IDLE5);
        LOAD_POSE(m_poseItem, s_pose_ITEM);
        LOAD_POSE(m_poseItem2, s_pose_ITEM2);
        LOAD_POSE(m_poseDeath, s_pose_DEATH);
        return;
    }

    if (toyOnly != 0) {
        LOAD_POSE(m_poseWalk, s_pose_WALK);
    } else {
        LOAD_POSE(m_poseToy1, s_pose_TOY1);
        i32 x = (reinterpret_cast<CAnimSetNode*>(m_poseToy1))->m_10;
        LOAD_POSE(m_poseToy2, s_pose_TOY2);
        i32 y = (reinterpret_cast<CAnimSetNode*>(m_poseToy2))->m_10;
        if (x >= y) {
            m_toyBlendPct = static_cast<i32>((100.0 / (static_cast<double>(x) / y - -1.0) - -0.5));
        } else {
            m_toyBlendPct =
                100 - static_cast<i32>((100.0 / (static_cast<double>(y) / x - -1.0) - -0.5));
        }
    }

    LOAD_POSE(m_poseToyBreak, s_pose_TOYBREAK);
}

#undef LOAD_POSE

RVA(0x0004a9f0, 0x1aa)
i32 CGrunt::winapi_04a9f0_CopyRect_OffsetRect() {
    CGrunt* tgt = m_tileMgr->FindAtPixel(m_object->m_screenX, m_object->m_screenY);
    if (tgt == 0) {
        return 0;
    }
    RECT r;
    CopyRect(&r, &tgt->m_38->m_area);
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

// CGrunt::PlaySound(range, rec)   @0x4ac10   (__thiscall, ret 0x10)
// The directional grunt-voice entrance handler PlayMoveSound fires. `rec` is the
// 3-DWORD compass voice record passed by value {col, row, flag}; the latched cell
// record is m_entranceCell (this+0x43c). It bails if the new record matches the
// latched one (same +8 flag), else dispatches on the grunt's current anim-name
// single-letter type code (F/D/A/K/E/I/M) to one of four geometry arms, re-stamps
// the entrance player's geometry + per-cell frame from the m_474 cell tables, and
// latches the record into m_entranceCell. `range` (1000) is unused here.
// @early-stop
// regalloc/frame plateau (~62%): the full dispatch CFG, all 7 type-code arms (the
// bool-eq inline-strcmp setcc form), the 4 geometry arms, the 3 cell tables
// (0x468/0x474/0x470, (3*col+row)*0x68), and the record latch are byte-exact in
// shape/order. Residue = retail reserves a 0xc scratch frame + spills rec fields
// where mine keeps them in regs (cellrec pinned ebp vs ebx), plus the merged
// E/IDLE GetName tail and per-arm esi/edx placement - pure register/spill
// scheduling, no source lever flips it. Closing this brought PlayMoveSound to 100%.
// Deferred to the final sweep.
RVA(0x0004ac10, 0x402)
void CGrunt::PlaySound(i32 range, CGruntVoiceRec rec) {
    static_cast<void>(range);
    if (CGrunt_IsSameType(
            reinterpret_cast<CGrunt*>(&m_entranceCell),
            reinterpret_cast<CGrunt*>(&rec)
        )) {
        return;
    }

    bool eq;
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), s_codeF) == 0);
    if (eq) {
        return;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), s_codeD) == 0);
    if (eq) {
        goto walk;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), "A") == 0);
    if (eq) {
        goto idle;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), s_codeK) == 0);
    if (eq) {
        goto idle;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), "E") == 0);
    if (eq) {
        // code "E": drive the ATTACK-IDLE geometry, stamp the cell frame from the
        // latched m_entranceCell triple (cell table base 0x468).
        m_value = m_38->m_1a0.m_14;
        m_38->m_1a0.Setup(m_poseAttackIdle);
        {
            CAniElement* desc = m_38->m_1a0.m_14;
            i32* elem = desc->m_records.GetSize() > 0
                            ? reinterpret_cast<i32*>(desc->m_records.GetAt(0))
                            : 0;
            i32 frame = elem[0x14 / 4];
            i32 col = m_entranceCell.col;
            i32 row = m_entranceCell.row;
            i32 index = 3 * col + row;
            const char* nm = reinterpret_cast<const char*>(
                (reinterpret_cast<_zdvec*>(&m_cells[index]))->IndexToPtr(0)
            );
            m_38->ApplyLookupSprite(nm, frame);
        }
        goto store;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), "I") == 0);
    if (eq) {
        goto codeI;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), s_codeM) == 0);
    if (eq) {
        goto walk;
    }

codeI:
    // code "I": latch the record first, drive the IDLE2 geometry, reseed the idle
    // timer. Returns directly (no cell-frame stamp).
    m_entranceCell.col = rec.m_0;
    m_entranceCell.row = rec.m_4;
    m_entranceCell.reason = rec.m_8;
    m_value = m_38->m_1a0.m_14;
    m_38->m_1a0.Setup(m_poseIdle[1]);
    ReseedIdleReset(1, 0, 0);
    return;

idle:
    // codes "A"/"K": drive the IDLE1 geometry (the forwarding setter), stamp the
    // cell frame from the incoming record (cell table base 0x474).
    m_value = m_38->m_1a0.m_14;
    m_38->ApplyGeometryDirect(m_poseIdle[0], 0);
    {
        CAniElement* desc = m_38->m_1a0.m_14;
        i32* elem =
            desc->m_records.GetSize() > 0 ? reinterpret_cast<i32*>(desc->m_records.GetAt(0)) : 0;
        i32 frame = elem[0x14 / 4];
        i32 col = rec.m_0;
        i32 row = rec.m_4;
        i32 index = 3 * col + row;
        const char* nm = reinterpret_cast<const char*>(
            (reinterpret_cast<_zdvec*>(&m_cells[index].m_idle))->IndexToPtr(0)
        );
        m_38->ApplyLookupSprite(nm, frame);
    }
    goto store;

walk:
    // codes "D"/"M" (and the default): drive the WALK geometry, stamp the cell name
    // from the incoming record (cell table base 0x470), set it by name only.
    m_value = m_38->m_1a0.m_14;
    m_38->m_1a0.Setup(m_poseWalk);
    {
        i32 col = rec.m_0;
        i32 row = rec.m_4;
        i32 index = 3 * col + row;
        const char* nm = reinterpret_cast<const char*>(
            (reinterpret_cast<_zdvec*>(&m_cells[index].m_walk))->IndexToPtr(0)
        );
        m_38->ApplyName(nm);
    }

store:
    m_entranceCell.col = rec.m_0;
    m_entranceCell.row = rec.m_4;
    m_entranceCell.reason = rec.m_8;
}

// @early-stop
// identical-return-epilogue-tailmerge wall (docs/patterns/): the m_arrived early
// `return 1;` and the trailing `return 1;` are identical epilogues - retail inlines
// both (je body; mov eax,1;ret), our cl tail-merges to one shared tail. Logic + CFG
// + member stores byte-exact; the six per-arrival calls are the real HUD creators and
// SetEntrancePos. Residual = the tail-merge + the one unnamed tile-mgr notify call.
// CGrunt::CommitArrival() @0x4b130 - finalizes the grunt's arrival on its tile.
// If already arrived (m_arrived) returns 1 immediately. Otherwise, if not yet
// claimed (m_tileClaimed==0): in alt-mode (registry m_134==2) it just notifies the tile
// owner; else it seeds the arrival defender block (m_arrivalRerollLo/m_arrivalRerollWindowLo/.., m_tileClaimed, m_arrivalState,
// m_arrivalFlags &= mask) and records the entrance pos. Then runs the six HUD sprite
// creators and latches m_arrived=1.
RVA(0x0004b130, 0xc8)
i32 CGrunt::CommitArrival() {
    if (m_arrived != 0) {
        return 1;
    }
    if (m_tileClaimed != 0) {
        if (g_gameReg->m_134 == 2) {
            m_tileMgr->PostCellCommand7(m_tileOwnerHi, m_tileOwnerLo); // 0x2c48 -> 0x6daa0
        } else if (m_tileClaimed != 0) {
            m_arrivalRerollLo = 0;
            m_arrivalRerollWindowLo = 0;
            m_arrivalRerollHi = 0;
            m_arrivalRerollWindowHi = 0;
            i32 flags = m_arrivalFlags & 0xe7fbfbfd;
            m_tileClaimed = 0;
            m_arrivalState = 0;
            m_arrivalFlags = flags;
            SetEntrancePos(1, 1);
        }
    }
    CreateSelectedSprite();
    CreateHealthSprite();
    CreateToySprite();
    CreateStaminaSprite();
    CreateToyTimeSprite();
    CreateWingzTimeSprite();
    m_arrived = 1;
    return 1;
}

RVA(0x0004b240, 0xaa)
void CGrunt::ClearAllSprites() {
    if (m_selectedSprite) {
        m_selectedSprite->m_flags |= 0x10000;
        m_selectedSprite = 0;
    }
    if (m_healthSprite) {
        m_healthSprite->m_flags |= 0x10000;
        m_healthSprite = 0;
    }
    if (m_toySprite) {
        m_toySprite->m_flags |= 0x10000;
        m_toySprite = 0;
    }
    if (m_entranceCommitted == 0) {
        if (m_staminaSprite) {
            m_staminaSprite->m_flags |= 0x10000;
            m_staminaSprite = 0;
        }
        if (m_toyTimeSprite) {
            m_toyTimeSprite->m_flags |= 0x10000;
            m_toyTimeSprite = 0;
        }
        if (m_wingzTimeSprite) {
            m_wingzTimeSprite->m_flags |= 0x10000;
            m_wingzTimeSprite = 0;
        }
    }
    m_arrived = 0;
}

// @early-stop
// shuttle-register regalloc wall: logic exact; the target threads the four
// passthrough args (c..f) through one saved esi (push esi; mov esi,[..]; push
// esi x4) while MSVC here pre-loads them into eax/ecx/edx. Pure arg-marshalling
// schedule coin-flip; no source lever flips it (entropy-class).
// CGrunt::TileSwitch(a, b, c, d, e, f) @0x4b320 - scale the two grid coords to
// tile-pixel centers (*0x20 + 0x10) and forward all six args to the engine
// tile-switch helper. __thiscall: the body never reads `this` (byte-identical
// either way, ret 0x18), but every retail caller loads a grunt into ecx - which
// only the member spelling reproduces at the ~25 reconstructed sites.
RVA(0x0004b320, 0x34)
i32 CGrunt::TileSwitch(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f) {
    return GruntTileSwitchImpl(a * 0x20 + 0x10, b * 0x20 + 0x10, c, d, e, f);
}

// ---------------------------------------------------------------------------
// CGrunt::StepArrivalDrop(a,b,c,d,e,f)   @0x4b370   (ret 0x18, /GX EH frame)
// @early-stop
// TRUNCATED reconstruction (~8%): only the head (m_464 clear + "D" strcmp + the
// reachedTarget arg test + a partial coord-node freelist recycle) is present. Retail
// is 838 insns; the base is ~86. The missing ~750 are the arrival-commit tail the
// placeholder `StepDropApply()` stands in for: 4 pathfinder re-probes (0x20f4), the
// CPtrList release/claim churn (0x1b4a03/0x1b48a6), and the per-direction tile-commit
// body. Needs a dedicated leaf-first reconstruction of that shared inlined tail (also
// inlined into MovingSlot16) - deferred to the final sweep, NOT a codegen wall.
RVA(0x0004b370, 0xafd)
void CGrunt::StepArrivalDrop(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f) {
    m_arrivalNotified = 0; // m_464 cleared on entry
    bool eq;
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), s_codeD) == 0);
    if (!eq && a == m_entrancePxX && b == m_entrancePxY) {
        goto reachedTarget;
    }
    // Recycle the occupied-coord nodes onto the CoordPool, empty the list, then
    // probe the destination tile via the engine pathfinder (0x20f4) and either
    // re-anchor (within range) or fall through to the big arrival commit.
    if (CoordCount() != 0) {
        GruntCoordNode* n = CoordHead();
        while (n != 0) {
            GruntCoordNode* cur = n;
            n = n->m_next;
            if (cur->m_coord != 0) {
                g_coordPool.Push(cur->m_coord);
            }
        }
        m_31c.RemoveAll();
    }
    StepDropApply();
    return;

reachedTarget:
    m_tileMgr->CellDispatch(a, b, c, d);
}

// ---------------------------------------------------------------------------
// CGrunt::StepGruntMovement()   @0x4c170   (ret 0)
// The per-tick move step: pop the head occupied-coord, bucket its direction from
// the grunt HUD center into one of the 8 compass move-vector records (g_voiceXX;
// [0]/[1] = the destination tile pixel pos, later committed into m_lastTile), gate
// the destination + last tile against the board occupancy/corner-cut bits, then
// either play the move sound + reset the entrance (blocked) or commit the tile
// occupancy transfer (clear the old tile's 0x20000000 owner bit + set the new
// tile's) and dispatch the entrance reason (0x12 -> RunMoveConfig, 0x16 -> wingz
// sprites, 0xe -> re-stamp the walk geometry).
//
// @early-stop
// record-CSE-liveness regalloc wall (62.4%, up from 5.2%): CFG, every board/
// coord/tile access, both compass picks (shared bd + tgtTile locals), the
// corner-cut diagonal else-if chain, the aliased row-table reloads in the
// occupancy commit, and the reason dispatch are reconstructed byte-for-byte
// where the stack slots align. Residue (llvm-objdump base vs target): after the
// first compass pick retail keeps rec.m_0/m_4/m_8's VALUES live in edi/ebp +
// scratch spills [esp+0x2c]/[esp+0x30] all the way to the first PlaySound sites
// (across the board fetch + 2 calls) IN ADDITION to rec's home [esp+0x3c..0x44]
// -> 0x38 frame; a clean recompile serves those sites from the home and re-uses
// caller-saved regs -> 3 fewer stores/arm-region, 0x30 frame, and the -2-slot
// shift retypes ~every [esp+N] operand byte. Tried: element-wise (best),
// pick-struct + rec=pick copy (52.1), scalars d0-d2 + join copy (61.6),
// by-value CGruntVoiceRec temp (48.9). No spelling extends the CSE ranges.
RVA(0x0004c170, 0xbe7)
i32 CGrunt::StepGruntMovement() {
    i32 coordX, coordY;
    i32 gtX, gtY;
    CGruntVoiceRec rec;
    i32 tgtPxX, tgtPxY;
    i32 flagHead;
    i32 reason12, reason16, reason0e;
    i32 tgtTileX, tgtTileY;
    CGruntzMapMgr* bd;

    {
        i32 entX = m_entrancePxX;
        i32 lastX = m_lastTilePxX;
        i32 entY = m_entrancePxY;
        if (lastX == entX && m_lastTilePxY == entY) {
            goto label_ret1;
        }
    }
    if (m_arrivalState == 0x11) {
        CBattlezMapConfig* slot = &g_gameReg->m_options[m_tileOwnerHi].m_038;
        if (slot != 0 && GruntDropReady029b40(this) == 0) {
            SetEntrancePos(1, 1);
            return 0;
        }
    }
    if (CoordCount() == 0) {
        goto label_dropRet0;
    }
    if (m_arrivalState != 0x11) {
        GruntCoord* co = static_cast<GruntCoord*>(m_31c.RemoveHead());
        coordX = co->m_x;
        coordY = co->m_y;
        CoordPoolNode* p = g_coordPool.NodeOf(co);
        p->m_next = g_coordPool.m_freeHead;
        g_coordPool.m_freeHead = p;
    } else {
        GruntCoord* co = CoordHead()->m_coord;
        coordX = co->m_x;
        coordY = co->m_y;
    }

    gtX = m_object->m_screenX >> 5;
    gtY = m_object->m_screenY >> 5;
    if (coordX > gtX) {
        if (coordY > gtY) {
            rec.m_0 = g_voiceSE.m_0;
            rec.m_4 = g_voiceSE.m_4;
            rec.m_8 = g_voiceSE.m_8;
        } else if (coordY == gtY) {
            rec.m_0 = g_voiceE.m_0;
            rec.m_4 = g_voiceE.m_4;
            rec.m_8 = g_voiceE.m_8;
        } else {
            rec.m_0 = g_voiceNE.m_0;
            rec.m_4 = g_voiceNE.m_4;
            rec.m_8 = g_voiceNE.m_8;
        }
    } else if (coordX < gtX) {
        if (coordY > gtY) {
            rec.m_0 = g_voiceSW.m_0;
            rec.m_4 = g_voiceSW.m_4;
            rec.m_8 = g_voiceSW.m_8;
        } else if (coordY == gtY) {
            rec.m_0 = g_voiceW.m_0;
            rec.m_4 = g_voiceW.m_4;
            rec.m_8 = g_voiceW.m_8;
        } else {
            rec.m_0 = g_voiceNW.m_0;
            rec.m_4 = g_voiceNW.m_4;
            rec.m_8 = g_voiceNW.m_8;
        }
    } else {
        if (coordY < gtY) {
            rec.m_0 = g_voiceS.m_0;
            rec.m_4 = g_voiceS.m_4;
            rec.m_8 = g_voiceS.m_8;
        } else {
            rec.m_0 = g_voiceN.m_0;
            rec.m_4 = g_voiceN.m_4;
            rec.m_8 = g_voiceN.m_8;
        }
    }

    tgtPxX = (coordX << 5) + 0x10;
    tgtPxY = (coordY << 5) + 0x10;
    bd = g_gameReg->m_tileGrid;
    tgtTileX = tgtPxX >> 5;
    tgtTileY = tgtPxY >> 5;
    if (static_cast<u32>(tgtTileX) < static_cast<u32>(bd->m_width)
        && static_cast<u32>(tgtTileY) < static_cast<u32>(bd->m_height)) {
        flagHead = bd->m_rowInts[tgtTileY][tgtTileX * 7];
    } else {
        flagHead = 1;
    }

    {
        i32 blockMove = 1;
        if (m_arrivalState == 6) {
            if (((m_defenderX ^ tgtPxX) & 0xffffffe0) == 0
                && ((m_defenderY ^ tgtPxY) & 0xffffffe0) == 0) {
                blockMove = 0;
            }
        }
        if (blockMove != 0 && !(flagHead & 0x20000000)) {
            i32 mask = m_arrivalFlags & flagHead;
            if (!(mask & 0x20000000)) {
                if (mask == 0) {
                    goto label_4c6e4;
                }
                if (flagHead & m_24c) {
                    goto label_4c6e4;
                }
            }
        }
    }
    if (m_entranceActive != 0) {
        goto label_4c68b;
    }
    {
        i32 lastFlag;
        i32 ltx = m_lastTilePxX >> 5;
        i32 lty = m_lastTilePxY >> 5;
        if (static_cast<u32>(ltx) < static_cast<u32>(bd->m_width)
            && static_cast<u32>(lty) < static_cast<u32>(bd->m_height)) {
            lastFlag = bd->m_rowInts[lty][ltx * 7];
        } else {
            lastFlag = 1;
        }
        if (lastFlag & 0x80) {
            goto label_4c68b;
        }
    }
    if (m_arrivalState == 0x11) {
        goto label_4cb2a;
    }
    if (CoordCount() == 0) {
        goto label_4cb2a;
    }
    {
        i32 mask = m_arrivalFlags & flagHead;
        if (mask & 0x20000000) {
            goto label_4cb2a;
        }
        if (mask != 0 && !(flagHead & m_24c)) {
            goto label_4cb2a;
        }
    }
    if (!(flagHead & 0x20000000)) {
        goto label_4c6e4;
    }
    {
        void* node = 0;
        CoordPoolNode* head = g_coordPool.m_freeHead;
        if (head->m_next != 0) {
            node = &head->m_coord;
            g_coordPool.m_freeHead = head->m_next;
        }
        (static_cast<i32*>(node))[0] = tgtTileX;
        (static_cast<i32*>(node))[1] = tgtTileY;
        m_31c.AddHead(node);
    }
    if (ProbeRetry() == 0) {
        PlaySound(0x3e8, rec);
        SetEntrancePos(1, 0);
        return 0;
    }
    // ProbeRetry() != 0
    if (CoordCount() == 0) {
        goto label_4cb2a;
    }
    {
        GruntCoord* co = CoordHead()->m_coord;
        i32 cx = co->m_x;
        i32 cy = co->m_y;
        tgtPxX = (cx << 5) + 0x10;
        tgtPxY = (cy << 5) + 0x10;
        i32 gx = m_object->m_screenX >> 5;
        i32 gy = m_object->m_screenY >> 5;
        if (cx > gx) {
            if (cy > gy) {
                rec.m_0 = g_voiceSE.m_0;
                rec.m_4 = g_voiceSE.m_4;
                rec.m_8 = g_voiceSE.m_8;
            } else if (cy == gy) {
                rec.m_0 = g_voiceE.m_0;
                rec.m_4 = g_voiceE.m_4;
                rec.m_8 = g_voiceE.m_8;
            } else {
                rec.m_0 = g_voiceNE.m_0;
                rec.m_4 = g_voiceNE.m_4;
                rec.m_8 = g_voiceNE.m_8;
            }
        } else if (cx < gx) {
            if (cy > gy) {
                rec.m_0 = g_voiceSW.m_0;
                rec.m_4 = g_voiceSW.m_4;
                rec.m_8 = g_voiceSW.m_8;
            } else if (cy == gy) {
                rec.m_0 = g_voiceW.m_0;
                rec.m_4 = g_voiceW.m_4;
                rec.m_8 = g_voiceW.m_8;
            } else {
                rec.m_0 = g_voiceNW.m_0;
                rec.m_4 = g_voiceNW.m_4;
                rec.m_8 = g_voiceNW.m_8;
            }
        } else {
            if (cy < gy) {
                rec.m_0 = g_voiceS.m_0;
                rec.m_4 = g_voiceS.m_4;
                rec.m_8 = g_voiceS.m_8;
            } else {
                rec.m_0 = g_voiceN.m_0;
                rec.m_4 = g_voiceN.m_4;
                rec.m_8 = g_voiceN.m_8;
            }
        }
        CGruntzMapMgr* bd = g_gameReg->m_tileGrid;
        if (bd->m_rowInts[cy][cx * 7] & 0x20000000) {
            PlaySound(0x3e8, rec);
            SetEntrancePos(1, 0);
            return 0;
        }
        GruntCoord* co2 = static_cast<GruntCoord*>(m_31c.RemoveHead());
        CoordPoolNode* p = g_coordPool.NodeOf(co2);
        p->m_next = g_coordPool.m_freeHead;
        g_coordPool.m_freeHead = p;
        goto label_4c6e4;
    }

label_4c68b:
    if ((flagHead & 0x20000000) && !(flagHead & 0x80)) {
        i32 owner;
        if (static_cast<u32>(tgtTileX) < static_cast<u32>(bd->m_width)
            && static_cast<u32>(tgtTileY) < static_cast<u32>(bd->m_height)) {
            owner = bd->m_rowInts[tgtTileY][tgtTileX * 7 + 1];
        } else {
            owner = -1;
        }
        m_tileMgr->CellDispatch((owner >> 8) & 0xff, owner & 0xff, 2, m_tileOwnerHi);
    }

label_4c6e4:
    if (m_arrivalState == 0x11 && CoordCount() != 0) {
        GruntCoord* co = static_cast<GruntCoord*>(m_31c.RemoveHead());
        CoordPoolNode* p = g_coordPool.NodeOf(co);
        p->m_next = g_coordPool.m_freeHead;
        g_coordPool.m_freeHead = p;
    }
    if (flagHead & 0x80) {
        m_entranceActive = 1;
    } else {
        CAnimNameRecord* r = g_typeColl.ScratchResolve(m_objAux->m_1c);
        GruntScratchTeardown();
        bool ne;
        ne = (strcmp(r->m_name, "L") != 0);
        if (ne) {
            m_entranceActive = 0;
        }
    }

    reason12 = 0;
    reason16 = 0;
    reason0e = 0;
    if (m_entranceReason == 0x12) {
        reason12 = 1;
    } else if (m_entranceReason == 0x16) {
        reason16 = 1;
    } else if (m_entranceReason == 0xe) {
        reason0e = 1;
    }
    if (reason0e == 0) {
        goto label_4cb4b;
    }

    // reason == 0xe: reflect one tile past the head and re-gate
    if (!(flagHead & 0x1400)) {
        if (!(flagHead & 0x2)) {
            goto label_4cb4b;
        }
    }
    if (tgtPxX == m_entrancePxX && tgtPxY == m_entrancePxY) {
        if ((flagHead & 0x939) == 0) {
            goto label_4c92b;
        }
        goto label_4cb2a;
    }
    {
        i32 beyondPxX = tgtPxX * 2 - m_lastTilePxX;
        i32 beyondPxY = tgtPxY * 2 - m_lastTilePxY;
        i32 btx = beyondPxX >> 5;
        i32 bty = beyondPxY >> 5;
        i32 beyondFlag;
        CGruntzMapMgr* bd = g_gameReg->m_tileGrid;
        if (static_cast<u32>(btx) < static_cast<u32>(bd->m_width)
            && static_cast<u32>(bty) < static_cast<u32>(bd->m_height)) {
            beyondFlag = bd->m_rowInts[bty][btx * 7];
        } else {
            beyondFlag = 1;
        }
        if (beyondFlag & 0x20000939) {
            goto label_4cb2a;
        }
        if (CoordCount() != 0 && m_arrivalState != 0x11) {
            GruntCoord* co = static_cast<GruntCoord*>(m_31c.RemoveHead());
            if (co->m_x == btx && co->m_y == bty) {
                CoordPoolNode* p = g_coordPool.NodeOf(co);
                p->m_next = g_coordPool.m_freeHead;
                g_coordPool.m_freeHead = p;
            } else {
                m_31c.AddHead(co);
            }
        }
        i32 hudY = m_object->m_screenY;
        i32 hudX = m_object->m_screenX;
        CCueRect* rr =
            reinterpret_cast<CCueRect*>(&g_gameReg->m_world->m_level->m_mainPlane->m_originX);
        if (hudX < rr->right && hudX >= rr->left && hudY < rr->bottom && hudY >= rr->top) {
            g_gameReg->m_cueSink->CueSpawn(this, 8, -1, -1, -1);
        }
        tgtPxX = beyondPxX;
        tgtPxY = beyondPxY;
    }

label_4c92b: {
    i32 lastTileX = m_lastTilePxX >> 5;
    tgtTileX = tgtPxX >> 5;
    i32 lastTileY = m_lastTilePxY >> 5;
    tgtTileY = tgtPxY >> 5;
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
    i32 tgtFlag = tgtT->m_0;
    i32 mask = m_arrivalFlags & tgtFlag;
    if (mask & 0x20000000) {
        goto label_4cb2a;
    }
    if (mask != 0 && !(tgtFlag & m_24c)) {
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
    // neighbor steps: +-1 cell horizontally, +-xbound cells vertically (rows are
    // one contiguous block; retail precomputes xbound*sizeof(BrickzCell))
    if (dx > 0 && dy > 0) {
        if ((lastT + 1)->m_0 & 0x2000) {
            goto label_4cb2a;
        }
        if ((lastT + xbound)->m_0 & 0x2000) {
            goto label_4cb2a;
        }
        if ((tgtT - 1)->m_0 & 0x2000) {
            goto label_4cb2a;
        }
        if (!((tgtT - xbound)->m_0 & 0x2000)) {
            goto label_4cb4b;
        }
        goto label_4cb2a;
    } else if (dx < 0 && dy > 0) {
        if ((lastT - 1)->m_0 & 0x2000) {
            goto label_4cb2a;
        }
        if ((lastT + xbound)->m_0 & 0x2000) {
            goto label_4cb2a;
        }
        if ((tgtT + 1)->m_0 & 0x2000) {
            goto label_4cb2a;
        }
        if (!((tgtT - xbound)->m_0 & 0x2000)) {
            goto label_4cb4b;
        }
        goto label_4cb2a;
    } else if (dx > 0 && dy < 0) {
        if ((lastT + 1)->m_0 & 0x2000) {
            goto label_4cb2a;
        }
        if ((lastT - xbound)->m_0 & 0x2000) {
            goto label_4cb2a;
        }
        if ((tgtT - 1)->m_0 & 0x2000) {
            goto label_4cb2a;
        }
        if (!((tgtT + xbound)->m_0 & 0x2000)) {
            goto label_4cb4b;
        }
        goto label_4cb2a;
    } else if (dx < 0 && dy < 0) {
        if ((lastT - 1)->m_0 & 0x2000) {
            goto label_4cb2a;
        }
        if ((lastT - xbound)->m_0 & 0x2000) {
            goto label_4cb2a;
        }
        if ((tgtT + 1)->m_0 & 0x2000) {
            goto label_4cb2a;
        }
        if (!((tgtT + xbound)->m_0 & 0x2000)) {
            goto label_4cb4b;
        }
        goto label_4cb2a;
    }
    goto label_4cb4b;
}

label_4cb2a:
    PlaySound(0x3e8, rec);
    SetEntrancePos(1, 1);
    return 0;

label_4cb4b:
    m_210 = 0;
    m_tileMgr->ApplySwitch(this, m_lastTilePxX,
                           m_lastTilePxY); // real 0x6d300
    m_coordRetryCount = 0;
    PlaySound(0x3e8, rec);
    {
        m_commitPxX = m_lastTilePxX;
        m_commitPxY = m_lastTilePxY;
        i32 lastTileX = m_lastTilePxX >> 5;
        i32 lastTileY = m_lastTilePxY >> 5;
        CGruntzMapMgr* bdl = g_gameReg->m_tileGrid;
        // Two separate row-table walks: the byte-store may alias m_8, so retail
        // reloads the row table between them.
        bdl->m_rows[lastTileY][lastTileX].m_flagBytes[3] &= 0xdf;
        bdl->m_rows[lastTileY][lastTileX].m_4 = -1;

        tgtTileX = tgtPxX >> 5;
        tgtTileY = tgtPxY >> 5;
        CGruntzMapMgr* bd2 = g_gameReg->m_tileGrid;
        bd2->m_rows[tgtTileY][tgtTileX].m_0 |= 0x20000000;
        bd2->m_rows[tgtTileY][tgtTileX].m_4 = (m_tileOwnerHi << 8) | m_tileOwnerLo;

        m_lastTilePxX = rec.m_0;
        m_lastTilePxY = rec.m_4;
        ComputeFacing(1.0);
    }
    m_arrivalPending = 1;
    if (reason12) {
        if (flagHead & 0x100) {
            if (m_coordToggle != 0) {
                goto label_ret1;
            }
        } else {
            if (m_coordToggle == 0) {
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
        if (m_wingzEnabled != 0) {
            goto label_ret1;
        }
        LoadWingzGruntSprites(1);
        return 1;
    }
    if (reason0e) {
        m_value = m_38->m_1a0.m_14;
        m_38->m_1a0.Setup(m_poseWalk);
        return 1;
    }
    goto label_ret1;

label_dropRet0:
    SetEntrancePos(1, 1);
    return 0;

label_ret1:
    return 1;
}

// @early-stop
// reloc-masked-symbol plateau: instruction stream byte-exact vs retail (verified
// llvm-objdump), residual is the two unnamed free-pool globals (g_coordPool.m_freeHead/
// Base) + the Coll::Reset call pairing to differently named retail symbols.
// CGrunt::SetEntrancePos(a, b) @0x4d060 - records the grunt's current tile as
// its committed entrance position (m_174/m_178 = m_lastTilePxX/m_lastTilePxY), clears the
// arrival timers (m_210); if `a`, also clears m_450/m_arrivalActive; and if `b` and the
// grunt is not a special kind (m_arrivalState!=0x11) it drains the name list at +0x320
// into the global free pool and resets the collection at +0x31c.
RVA(0x0004d060, 0x98)
void CGrunt::SetEntrancePos(i32 a, i32 b) {
    m_entrancePxX = m_lastTilePxX;
    m_entrancePxY = m_lastTilePxY;
    m_210 = 0;
    if (a) {
        m_arrivalPhase = 0;
        m_arrivalActive = 0;
    }
    if (b && m_arrivalState != 0x11 && CoordCount() != 0) {
        void** node = reinterpret_cast<void**>(CoordHead());
        if (node) {
            do {
                void* next = node[0];
                void* buf = node[2];
                if (buf) {
                    CoordPoolNode* slot = g_coordPool.NodeOf(buf);
                    slot->m_next = g_coordPool.m_freeHead;
                    g_coordPool.m_freeHead = slot;
                }
                node = static_cast<void**>(next);
            } while (node);
        }
        m_31c.RemoveAll();
    }
}

RVA(0x0004d130, 0xb5)
i32 CGrunt::CreateHealthSprite() {
    if (m_healthSprite || m_health <= 0) {
        return 0;
    }

    m_healthSprite = g_gameReg->m_world->m_childGroup->CreateSprite(
        0,
        m_object->m_screenX,
        m_object->m_screenY - 0x19,
        0xdbba0,
        s_GruntHealthSprite,
        0x40003
    );
    m_healthSprite->m_7c->m_notify(m_healthSprite);

    AnimWorkerObj* inner = m_healthSprite->m_7c;
    CGruntHealthSprite* reg = static_cast<CGruntHealthSprite*>(inner->m_logic);
    if (!reg->SetHealthGlyph(m_tileOwnerHi, m_tileOwnerLo, m_health)) {
        reg->m_38->m_flags |= 0x10000;
        m_healthSprite = 0;
        return 0;
    }
    return 1;
}

RVA(0x0004d220, 0x9c)
i32 CGrunt::CreateToySprite() {
    if (m_toySprite) {
        return 0;
    }

    m_toySprite = g_gameReg->m_world->m_childGroup->CreateSprite(
        0,
        m_object->m_screenX,
        m_object->m_screenY - 0x19,
        0xdbba0,
        s_GruntToySprite,
        0x40003
    );
    m_toySprite->m_7c->m_notify(m_toySprite);

    CGruntToySprite* reg = static_cast<CGruntToySprite*>(m_toySprite->m_7c->m_logic);
    if (!reg->SetCell(m_tileOwnerHi, m_tileOwnerLo)) {
        reg->m_38->m_flags |= 0x10000;
        m_toySprite = 0;
        return 0;
    }
    return 1;
}

RVA(0x0004d2f0, 0xb4)
i32 CGrunt::CreateStaminaSprite() {
    if (m_staminaSprite || m_stamina == 0x64) {
        return 0;
    }

    m_staminaSprite = g_gameReg->m_world->m_childGroup->CreateSprite(
        0,
        m_object->m_screenX,
        m_object->m_screenY - 0x20,
        0xdbba0,
        s_GruntStaminaSprite,
        0x40003
    );
    m_staminaSprite->m_7c->m_notify(m_staminaSprite);

    AnimWorkerObj* inner = m_staminaSprite->m_7c;
    CGruntHealthSprite* reg = static_cast<CGruntHealthSprite*>(inner->m_logic);
    if (!reg->SetHealthGlyph(m_tileOwnerHi, m_tileOwnerLo, m_stamina)) {
        reg->m_38->m_flags |= 0x10000;
        m_staminaSprite = 0;
        return 0;
    }
    return 1;
}

RVA(0x0004d3e0, 0xf5)
i32 CGrunt::CreateToyTimeSprite() {
    if (m_toyTimeSprite || m_toyTime == 0) {
        return 0;
    }

    if (m_staminaSprite) {
        m_staminaSprite->m_flags |= 0x10000;
        m_staminaSprite = 0;
    }
    if (m_wingzTimeSprite) {
        m_wingzTimeSprite->m_flags |= 0x10000;
        m_wingzTimeSprite = 0;
    }

    m_toyTimeSprite = g_gameReg->m_world->m_childGroup->CreateSprite(
        0,
        m_object->m_screenX,
        m_object->m_screenY - 0x20,
        0xdbba0,
        s_GruntToyTimeSprite,
        0x40003
    );
    m_toyTimeSprite->m_7c->m_notify(m_toyTimeSprite);

    AnimWorkerObj* inner = m_toyTimeSprite->m_7c;
    CGruntHealthSprite* reg = static_cast<CGruntHealthSprite*>(inner->m_logic);
    if (!reg->SetHealthGlyph(m_tileOwnerHi, m_tileOwnerLo, m_toyTime)) {
        reg->m_38->m_flags |= 0x10000;
        m_toyTimeSprite = 0;
        return 0;
    }
    return 1;
}

RVA(0x0004d520, 0xe3)
i32 CGrunt::CreateWingzTimeSprite() {
    if (m_wingzTimeSprite || m_wingzEnabled == 0 || m_wingzTime == 0) {
        return 0;
    }

    if (m_toyTimeSprite) {
        m_toyTimeSprite->m_flags |= 0x10000;
        m_toyTimeSprite = 0;
    }

    m_wingzTimeSprite = g_gameReg->m_world->m_childGroup->CreateSprite(
        0,
        m_object->m_screenX,
        m_object->m_screenY - 0x26,
        0xdbba0,
        s_GruntWingzTimeSprite,
        0x40003
    );
    m_wingzTimeSprite->m_7c->m_notify(m_wingzTimeSprite);

    AnimWorkerObj* inner = m_wingzTimeSprite->m_7c;
    CGruntHealthSprite* reg = static_cast<CGruntHealthSprite*>(inner->m_logic);
    if (!reg->SetHealthGlyph(m_tileOwnerHi, m_tileOwnerLo, m_wingzTime)) {
        reg->m_38->m_flags |= 0x10000;
        m_wingzTimeSprite = 0;
        return 0;
    }
    return 1;
}

RVA(0x0004d650, 0xa1)
i32 CGrunt::CreatePowerupSprite(i32 a) {
    if (m_powerupSprite) {
        return 0;
    }

    m_powerupSprite = g_gameReg->m_world->m_childGroup->CreateSprite(
        0,
        m_object->m_screenX,
        m_object->m_screenY,
        0x15,
        s_GruntPowerupSprite,
        0x40003
    );
    m_powerupSprite->m_7c->m_notify(m_powerupSprite);

    AnimWorkerObj* inner = m_powerupSprite->m_7c;
    CGruntPowerupSprite* reg = static_cast<CGruntPowerupSprite*>(inner->m_logic);
    if (!reg->SetCell(m_tileOwnerHi, m_tileOwnerLo, a)) {
        reg->m_38->m_flags |= 0x10000;
        m_powerupSprite = 0;
        return 0;
    }
    return 1;
}

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
        s_GruntSelectedSprite,
        0x40003
    );
    m_selectedSprite->m_7c->m_notify(m_selectedSprite);

    CGruntSelectedSprite* reg = static_cast<CGruntSelectedSprite*>(m_selectedSprite->m_7c->m_logic);
    if (!reg->SetCell(m_tileOwnerHi, m_tileOwnerLo)) {
        reg->m_38->m_flags |= 0x10000;
        m_selectedSprite = 0;
        return 0;
    }
    return 1;
}

// ===========================================================================
// TAIL ORPHANS: the five fns below are NOT part of the
// grunt-main obj (0x47a10-0x4d7c6). Each sits in its own single-fn retail
// interval with no dominant foreign unit and no private-.data / init-frag
// anchor, so their owning original TUs are unrecovered:
//   0x5f310  MovingSlot16            (interval 0x5f310-0x5fe6e, between the
//            0x5ecd0 single and the 0x60150 asset-loader obj)
//   0xec670 / 0xf26f0 / 0xf2b20 / 0xf8240  the far arrival-defense family
//            (the 0xea990-0xf8800 region: init runs ?x45 | gruntx9 |
//            gruntarrivalscanx7 | gruntx20 | ?x27 - a future partition package)
// They stay here (sorted, at the file tail) pending those partitions
// (@identity-TODO). CGrunt::Load @0xd8060 (also in this tail band) carries its
// own blocked-move note (the play-TU move is header-gated).
// ===========================================================================

// ---------------------------------------------------------------------------
// CGrunt::MovingSlot16()   @0x5f310   (ret 0)
// @early-stop
// TRUNCATED reconstruction (~9%): the coord-probe head (claim the head coord's tile if
// free, else retry within m_coordRetryCount) + the scratch-resolver "D" reject are the
// only reconstructed part. Retail is 938 insns; the base is ~132. The missing ~800 are
// the arrival-commit block at 0x5f490 (a second GetNameRecords/scratch-teardown + a
// "D"-gated arrival-processing body: pathfinder re-probe, tile release/claim, the
// per-direction m_cells[base] {m_dirX..m_stepY} double movement-integration tail). This
// is the shared inlined arrival-commit tail (the placeholder `StepDropApply()` call
// stands in for it here). Needs a dedicated leaf-first reconstruction of that tail
// (shared with StepArrivalDrop) - deferred to the final sweep, NOT a codegen wall.
RVA(0x0005f310, 0xb5e)
void CGrunt::MovingSlot16() {
    if (m_arrivalState != 0x11) {
        bool eq;
        eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), "A") == 0);
        if (eq && CoordCount() != 0) {
            GruntCoordNode* head = CoordHead();
            GruntCoord* co = head->m_coord;
            i32 fl =
                (reinterpret_cast<i32*>(g_gameReg->m_tileGrid->m_rowBytes[co->m_y]))[co->m_x * 7];
            i32 mask = m_arrivalFlags & fl;
            if (!(fl & 0x20000000) && !(mask & 0x20000000)
                && (mask == 0 || (m_arrivalNotified & fl) != 0)) {
                m_entrancePxX = (co->m_x << 5) + 0x10;
                m_entrancePxY = (co->m_y << 5) + 0x10;
                m_coordRetryCount = 0;
                NotifyDrop();
            } else if (m_coordRetryCount <= 5) {
                if (ProbeRetry() != 0) {
                    GruntCoord* h2 = (CoordHead())->m_coord;
                    m_entrancePxX = (h2->m_x << 5) + 0x10;
                    m_entrancePxY = (h2->m_y << 5) + 0x10;
                    if (CoordCount() != 0) {
                        GruntCoord* h3 = (CoordHead())->m_coord;
                        i32 fl2 = (reinterpret_cast<i32*>(
                            g_gameReg->m_tileGrid->m_rowBytes[h3->m_y]
                        ))[h3->m_x * 7];
                        if (!(fl2 & 0x20000000)) {
                            m_coordRetryCount = 0;
                            NotifyDrop();
                        }
                    }
                } else {
                    (m_coordRetryCount)++;
                }
            }
        }
    }
    // The scratch-resolver D-code reject cascade (each via GetNameRecords + the
    // scratch CString teardown).
    GruntScratchTeardown();
    bool eq2;
    eq2 = (strcmp(g_typeColl.GetNameRecords(m_objAux->m_1c)->m_name, s_codeD) == 0);
    static_cast<void>(eq2);
    GruntScratchTeardown();
    OnMoveFinishA(0);
}
