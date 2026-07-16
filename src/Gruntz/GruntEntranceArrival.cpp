// GruntEntranceArrival.cpp - the FOURTH original grunt TU (retail text
// 0x616e0-0x65df5, the WOVEN grunt+gruntentrancearrival interval): the CGrunt
// entrance/arrival step machine + the attack-rearm/status/fire-view family
// (grunt-region partition; weave 0.38 = single TU).
//
// ONE-obj evidence:
//   * grunt/gruntentrancearrival fns interleave A-B-A-B throughout the interval
//     (impossible for separate objs at first link).
//   * private .data extents in TU link order: CGrunt::StepAttackFire @0x61cb0's
//     cell (0x20e180) HEADS the band, then 0x62110/0x62e10/0x63db0/0x641b0's
//     cells, then RunMoveConfig/StartBombGruntRun's (0x20e264) and
//     LoadWandGruntItemConfig @0x65a60's (0x20e27c-0x20e28c) INSIDE the band
//     (before gruntpickupload's 0x20e29c) - so 0x61cb0 and 0x65a60 belong to THIS obj;
//     StepWarpExit @0x64540 is text-contained (between 0x641b0 and 0x646b0, deep inside
//     the weave).
//   * 2 EH sites in the interval -> /GX; the unit's flags flip base->eh.
// NOT this TU: CGrunt::FinalizeStep @0x5ecd0 (kept below with a note) - its
// birth interval is the lone 0x5ecd0-0x5f1c3 block between the 0x5d210 userlogic
// single and MovingSlot16 @0x5f310, OUTSIDE this obj's text; the owning original
// TU is unrecovered (@identity-TODO), it stays here pending that partition.
//
#include <Bute/ButeTree.h> // CButeTree::Find - g_buteTree @0x6bf620
#include <Gruntz/Random.h> // g_randSeed/g_randSeeded
#include <Gruntz/Grunt.h>
#include <DDrawMgr/AniAdvance.h>      // CAniDesc (the descriptor record)
#include <Bute/SymTab.h>              // CSymTab (ResolveQualified)
#include <DDrawMgr/DDrawSurfaceMgr.h> // the m_0c world root (m_animRegistry hop)
#include <DDrawMgr/DDrawSubMgrLeaf.h> // m_0c->m_animRegistry (the anim-key catalog)
#include <Gruntz/Enums.h>             // GruntType tool/powerup kinds + GruntDeathKind + RezTypeTag
#include <Gruntz/State.h>       // CState (m_levelIndex/m_levelBank - StepWarpExit's level lookup)
#include <Wap32/Wap32.h>        // CGameWnd (m_hwnd - StepWarpExit's level-switch post target)
#include <Gruntz/GameLevel.h>   // canonical CGameLevel/CLevelPlane (m_world->m_level visible rect)
#include <Gruntz/TypeKeyColl.h> // g_typeColl (folded CAnimNameResolver anim registry)
                                // WERE the fake g_animScratch / g_animScratchCount
                                // globals (defined in 5 TUs each; LNK2005)
#include <Gruntz/ActReg.h>      // CLookupColl/CActReg::ResolveEntry
#include <Gruntz/AniElement.h>
#include <Gruntz/AniAdvanceCursor.h> // CAniAdvanceCursor (value member of the warp/decay views)
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/SerialRecords.h>
#include <Gruntz/MovingLogicSerial.h>
#include <Gruntz/BoundaryLowerMethodsViews.h>
#include <Gruntz/Effect6b.h>
#include <Dsndmgr/DirectSoundMgr.h>
#include <Dsndmgr/DirectSoundMgr.h>
#include <Gruntz/GameRegistry.h> // canonical CGameRegistry (the reconciled singleton view)
// The game-manager singleton (0x64556c), declared CGameRegistry* (InGameIcon.h's decl;
// extern "C" cannot dual-type one symbol in one TU). CGameRegistry is the RICHER
// reconciled view (every field this TU reads exists on it).
#include <rva.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <Bute/ButeMgr.h>

// The tile-icon PlaceAt (@0x986b0, __thiscall(idx, gridBase); reloc-masked) is the
// real CInGameIcon's (<Gruntz/InGameIcon.h>). The old `GruntTileIcon` call-shape shim
// justified itself with "InGameIcon.h pulls <Mfc.h>" - STALE: Grunt.h itself pulls
// <Mfc.h> (its CPtrList value members), so the real class costs nothing.
#include <Gruntz/InGameIcon.h>

// The entrance geometry-source global (0x2bf3bc) each step arms the entrance
// sub-player with is declared with its DATA() binding below as the tree-wide
// keep-last winner `_g_6bf3bc` (see the conflation note there).
// g_frameTime (the running game clock) is declared by MovingLogic.h via Grunt.h.

// The primary MS-CRT LCG generator state (inlined by the re-roll step; reloc-masked).

// The per-tick draw-clock delta the position interpolation scales by (reloc-masked).
extern "C" u32 g_frameDelta; // 0x645584
// The FP sign threshold (0.0) the overshoot clamp compares the per-cell velocity to.
DATA(0x001e9a68)
double s_fpZero = 0.0; // 0x5e9a68
// The scratch-branch anim type code the position step latches on (reloc-masked).
extern const char k_60df94[]; // 0x60df94

// The ((CAnimScratchString*)g_typeColl.m_alloc)[] CString teardown the scratch-resolve reject path runs (Release
// each non-null slot, g_typeColl.m_grown times). The shared loop-strength-reduction
// wall (docs/patterns; cl `mov edi,count` vs retail `lea edi,[eax+1]`).
static void GruntPosScratchTeardown() {
    CAnimScratchString* slot = ((CAnimScratchString*)g_typeColl.m_alloc);
    i32 cnt = g_typeColl.m_grown;
    while (cnt != 0) {
        if (slot != 0) {
            ((CString*)slot)->~CString();
        }
        slot++;
        cnt--;
    }
}

// ==== the 13 CGrunt fns of this obj + their support decls ====

// Entrance-animation globals (reloc-masked; see Grunt.h).
// g_buteMgr comes from <Bute/ButeMgr.h>.
static char s_TimePerTile[] = "TimePerTile";
static char s_Grunt[] = "Grunt";                               // s_Grunt_0060a9ec
static char s_EntranceSafeTime[] = "EntranceSafeTime";         // s_EntranceSafeTime_0060df98
static char s_IdleDelay[] = "IdleDelay";                       // s_IdleDelay_0060e1a0
static char s_PlayerDefenderRadius[] = "PlayerDefenderRadius"; // s_PlayerDefenderRadius_0060e1ac
static char s_CombatTimeout[] = "CombatTimeout";               // s_CombatTimeout_0060df84

// StartBombGruntRun (@0x68520) bute tag/key (reloc-masked).
static char s_BOMBGRUNT[] = "BOMBGRUNT";                   // 0x60dbd0
static char s_RunningTimePerTile[] = "RunningTimePerTile"; // 0x60e264

// The single-char anim-set keys the entrance reads/looks-up (reloc-masked
// .rodata; DAT_0060a454 = "A" = the idle anim key, DAT_0060d7f8 = "K" =
// BuildEntranceAnimation's latch key).
static const char s_animKeyA[] = "A";
static const char s_animKeyK[] = "K";

// The global running game clock (DAT_00645588) snapshotted into m_entranceClockLo.

// The scratch CString teardown the GetNameRecords reject paths run (defined with the
// dispatch-machine cluster below); forward-declared for the two entrance-step
// methods (StepEntranceReinit / RunEntranceMove) defined earlier in RVA order.
static void GruntScratchTeardown();

// The board tile-flag helper shared by the entrance/arrival steps (the same
// inline the GruntSteps TU carries for StepCompassMove).

// Read the tile-flag word at board cell (tx, ty); out-of-bounds -> 1 (blocking).
static __inline i32 s_TileFlags(CTileGrid* b, i32 tx, i32 ty) {
    if (static_cast<u32>(tx) >= static_cast<u32>(b->m_c) || static_cast<u32>(ty) >= static_cast<u32>(b->m_10)) {
        return 1;
    }
    return ((i32*)b->m_8[ty])[tx * 7];
}

// ===========================================================================
// The 5 grunt movement / anim-name dispatch state machines (formerly the
// CUserLogic_* stubs @0x4b370 / 0x4c170 / 0x52fb0 / 0x5f310 / 0x6a6d0). Each
// resolves the grunt's current anim-set node name
// (g_typeColl.GetNameRecord(m_14->m_1c), or the scratch-teardown
// GetNameRecords form) and dispatches on its single-letter type code
// (A/D/I/G/L/P/O/Q/J/N/M/K), driving the grunt's movement/arrival state, recycling
// its occupied-coord nodes onto the shared freelist, and re-latching m_14->m_1c to
// a new anim set via g_entranceAnimSrc.LookupAnimSet. The inline-strcmp `== bool` setcc
// reject form is per docs/patterns/strcmp-eq-bool-local-setcc.md.
//
// These are the CGrunt analogues of CBattlezMapConfig::Method_025d90 /
// Method_02f620 (the documented large-state-machine + grid-regalloc walls). Each is
// reconstructed complete in shape/order; all carry @early-stop on those walls.
// Raw-offset member access (the campaign style used by the cluster above) keeps the
// giant ~0x46c layout tractable.

// The scratch CString teardown the GetNameRecords reject paths run (Release each
// non-null slot, g_typeColl.m_grown times). The shared loop-strength-reduction
// wall (docs/patterns; cl `mov edi,count` vs retail `lea edi,[eax+1]`).
static void GruntScratchTeardown() {
    CAnimScratchString* slot = ((CAnimScratchString*)g_typeColl.m_alloc);
    i32 cnt = g_typeColl.m_grown;
    while (cnt != 0) {
        if (slot != 0) {
            ((CString*)slot)->~CString();
        }
        slot++;
        cnt--;
    }
}

// ===========================================================================
// Migrated CGrunt cluster (formerly the CUserLogic_* stubs in
// src/Stub/Discovered.cpp). A prior matcher proved this whole block is CGrunt:
// the dtor @0xf2f0 stamps vtable 0x5e8754 over CUserLogic/CUserBase bases, and
// the anim loader @0x49c60 builds "GRUNTZ_<type>_<POSE>" keys. Reconstructed in
// ascending retail-RVA order. Raw-offset member access (the campaign style used
// by ReadConfigFromButeMgr above) keeps the giant ~0x46c layout tractable.
// ===========================================================================

// The global free-list pool the name caches recycle into (head @0x645544, base
// subtrahend @0x64554c). Defined TU-local (reloc-masked); shared in retail.

// The grunt movement / anim-name dispatch state machines' reloc-masked data.
// All TU-local definitions (reloc-masked against the retail symbols); the grunt
// freelist aliases the same g_coordPool.m_freeHead/Base pool (0x645544 / 0x64554c).
// src/Gruntz/GameText.cpp (the pool's owner TU).
// Only the owner defines; everyone externs.

// The single-letter anim type-code literals live ONCE in retail .rdata and are shared by
// every TU that compares against them (s_codeA..s_codeQ, declared in <Gruntz/Grunt.h>,
// DATA-bound in src/Globals.cpp).

// ==== CGrunt::UserLogicVfunc7 @0x61cb0 (its private .data cell 0x20e180 HEADS this
// TU's band) ====
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/GameRegistry.h>      // canonical CGameRegistry (fire-view cast)
#include <Gruntz/Projectile.h>        // canonical CProjectile (slot-17 LoadProjectileSprites)
#include <DDrawMgr/DDrawChildGroup.h> // the ONE CDDrawChildGroup (CreateSprite @0x1597b0)
#include <Gruntz/UserLogic.h>         // CGameObject (the created sprite + the bound object)
// The entrance geometry-source global at 0x2bf3bc every step in this TU arms the
// entrance sub-player with (fed to CAniAdvanceCursor::Advance). 0x2bf3bc is a
// tree-wide name conflation: ~17 TUs bind it via this extern "C" `_g_6bf3bc` (the
// keep-last DATA() winner), others as `g_slimeTick`/`g_defaultGeo` (unconfirmed
// guesses). Bound here as the winner so all refs resolve to 0x2bf3bc (reloc-faithful).
extern "C" i32 g_engineFrameDelta;
// g_frameTime (the running game clock stamped into the downtime timer record) comes
// from <Gruntz/MovingLogic.h> (extern "C" u32; Projectile.cpp owns the DATA pin).

// The created eye-candy sprite is the shared CGameObject (<Gruntz/UserLogic.h>);
// its +0x7c AnimWorkerObj control block carries the Init driver (+0x10) and the
// per-class setup slot m_18 (+0x18) - for "Projectile"/"Boomerang"/"TimeBomb"
// the bound setup/logic object IS a CProjectile (downcast per site; the slot is
// generically typed on the canonical aux; proven-heterogeneous across classes).
// The HUD sprite factory reached as g_gameReg->m_world->m_childGroup is the canonical
// CDDrawChildGroup (shared <Gruntz/SpriteFactory.h>).
// The game-registry singleton (0x64556c) is the CGameRegistry-typed g_gameReg
// (declared above) - the attack-fire paths read it natively, cast-free.

// The attack target resolved from the neighbor grid cell is another CGrunt (the
// 0x1bf9 TakeHit thunk delivers the attack; its m_entranceReason multiplexes the
// tool kind, m_19c the fallback gate). The attacker `this` IS CGrunt - the old
// slot-9 body (??_7CGrunt@@6B@+0x24 @0x1e8778 holds its ILT thunk 0x119f - the
// only retail reference), i.e. the UserLogicVfunc7 override Grunt.h declares.
// View->CGrunt member map: m_object == the CUserLogic +0x10 union arm (CGameObject),
// m_154 == CEntranceAnimPlayer* (anim sub @+0x1a0, done-gates m_1c0/m_1c8),
// m_toolKind == m_entranceReason, m_1c4 == m_healthSprite, m_3f0 == m_stamina
// (reset to 0 at each impact), m_460 == m_lowStaminaCued, m_868/m_86c ==
// m_attackDowntimeLo/Hi; ArmMode == SetMoveStateA (0x3bd9), Teardown3dd7 ==
// FinishAttackPowered, ArmFinish == ReseedIdleReset (0x136b), Teardown22de ==
// NotifyAttackImpact - all already declared on CGrunt.

// ==== LoadWandGruntItemConfig @0x65a60 (ex GruntBehaviorLeaf.cpp; its private .data cells
// 0x20e27c-0x20e28c sit INSIDE this TU's band, not the 0x612a0 behaviorleaf extent) ====
#include <Gruntz/GruntBehaviorLeaf.h>
// ---------------------------------------------------------------------------
// CGrunt::FinalizeStep(arg)   @0x5ecd0   (ret 4, the settled vtable slot-5 override)
// @early-stop
// complete reconstruction (all logic + control flow: the Finalize + slot-16 tick,
// the L/G ClearSubA gate, the off-screen ClearSubB gate, and BOTH the "O" and the
// scratch-resolved position-interpolation arms with the per-cell velocity math +
// overshoot clamp). A documented x87-scheduling wall (the fld/fmul/fadd/fcom + the
// fnstsw/test-ah overshoot clamp is not source-steerable), compounded by the
// strcmp-eq-setcc dispatch and the scratch-teardown loop-strength-reduction wall,
// same family as StepEntranceReinit / StepCombatReaction in Grunt.cpp. Homed here
// (out of the Gap stub) as a real CGrunt method so its 0x5ecd0 symbol resolves for
// callers; the % is a codegen wall, not a shape error.
RVA(0x0005ecd0, 0x4f3)
void CGrunt::FinalizeStep(i32 arg) {
    CUserLogic::FinalizeStep(arg); // direct base call (retail `call 0x3913`)
    MovingSlot16();
    if (m_424 != 0) {
        bool neL = (strcmp(*g_typeColl.GetNameRecord(m_14->m_1c), s_codeL) != 0);
        if (neL && strcmp(*g_typeColl.GetNameRecord(m_14->m_1c), s_codeG) != 0) {
            ClearSubA();
        }
    }
    if (m_428 != 0) {
        if (m_gruntKind == 0) {
            ClearSubB();
        } else {
            CGameRegistry* g = g_gameReg;
            i32 x = m_10->m_screenX;
            i32 y = m_10->m_screenY;
            if (!(x < g->m_viewOriginR && x >= g->m_viewOriginL && y < g->m_viewOriginB
                  && y >= g->m_viewOriginT)) {
                ClearSubB();
            }
        }
    }
    if (strcmp(*g_typeColl.GetNameRecord(m_14->m_1c), s_codeO) == 0) {
        // "O": flip the entrance cell col/row (0<->2), interpolate toward the target.
        if (m_10->m_screenX == m_lastTilePxX && m_10->m_screenY == m_lastTilePxY) {
            return;
        }
        GruntEntranceCell c = m_entranceCell;
        i32 col = (c.col == 0) ? 2 : (c.col == 2 ? 0 : c.col);
        i32 row = (c.row == 0) ? 2 : (c.row == 2 ? 0 : c.row);
        i32 base = 3 * col + row;
        char* cell = (char*)&m_cells[base];
        double d48 = *(double*)(cell + 0x48);
        double d50 = *(double*)(cell + 0x50);
        m_408 = static_cast<double>(static_cast<i64>(static_cast<u32>(g_frameDelta))) * d48 * m_400 + m_408;
        m_410 = static_cast<double>(static_cast<i64>(static_cast<u32>(g_frameDelta))) * d50 * m_400 + m_410;
        i32 nx = static_cast<i32>((*(double*)(cell + 0x58) + m_408));
        i32 ny = static_cast<i32>((*(double*)(cell + 0x60) + m_410));
        if ((d48 > s_fpZero && nx > m_lastTilePxX) || (d48 < s_fpZero && nx < m_lastTilePxX)) {
            nx = m_lastTilePxX;
        }
        if ((d50 > s_fpZero && ny > m_lastTilePxY) || (d50 < s_fpZero && ny < m_lastTilePxY)) {
            ny = m_lastTilePxY;
        }
        m_10->m_screenX = nx;
        m_10->m_screenY = ny;
        CGameObject* h = m_10;
        i32 v = h->m_screenY + 0x186a0;
        if (h->m_latchedAnimId != v) {
            h->m_latchedAnimId = v;
            h->m_flags |= 0x20000;
        }
        return;
    }
    // scratch-resolved branch: tear down the scratch CString[], latch on k_60df94.
    CAnimNameRecord* rec = g_typeColl.ScratchResolve(m_14->m_1c);
    GruntPosScratchTeardown();
    if (strcmp(rec->m_name, k_60df94) == 0) {
        if (m_10->m_screenX == m_lastTilePxX && m_10->m_screenY == m_lastTilePxY) {
            return;
        }
        GruntEntranceCell c = m_entranceCell;
        i32 base = 3 * c.col + c.row;
        char* cell = (char*)&m_cells[base];
        double d48 = *(double*)(cell + 0x48);
        double d50 = *(double*)(cell + 0x50);
        m_408 = static_cast<double>(static_cast<i64>(static_cast<u32>(g_frameDelta))) * d48 * m_400 + m_408;
        m_410 = static_cast<double>(static_cast<i64>(static_cast<u32>(g_frameDelta))) * d50 * m_400 + m_410;
        i32 nx = static_cast<i32>((*(double*)(cell + 0x58) + m_408));
        i32 ny = static_cast<i32>((*(double*)(cell + 0x60) + m_410));
        if ((d48 > s_fpZero && nx > m_lastTilePxX) || (d48 < s_fpZero && nx < m_lastTilePxX)) {
            nx = m_lastTilePxX;
        }
        if ((d50 > s_fpZero && ny > m_lastTilePxY) || (d50 < s_fpZero && ny < m_lastTilePxY)) {
            ny = m_lastTilePxY;
        }
        m_10->m_screenX = nx;
        m_10->m_screenY = ny;
        CGameObject* h = m_10;
        i32 v = h->m_screenY + 0x186a0;
        if (h->m_latchedAnimId != v) {
            h->m_latchedAnimId = v;
            h->m_flags |= 0x20000;
        }
    }
    return;
}

// The geometry sub-player's m_20/m_28 live PAST the player's own m_1b4, so they
// are read via raw offsets off &player->m_1a0 (same as FinishEntranceMove) to keep
// cl on one `add eax,0x1a0` base.

// @early-stop
// reloc-masked-extern plateau: instruction stream byte-exact (verified
// llvm-objdump - prologue, geometry call, desc/frame read, cell-index math
// `0x468 + (3*col+row)*0x68`, GetName/SetAnimFrame, anim-set latch all match),
// residual is the 4 unnamed engine calls (SetGeometry/GetName/SetAnimFrame/
// LookupAnimSet) pairing to differently named retail symbols.
// ---------------------------------------------------------------------------
// CGrunt::ResetGeometry() @0x616e0 - re-arms the entrance player's geometry to
// the m_poseAttackIdle source, re-stamps the active anim-set node, and re-applies the
// per-cell entrance frame. Reads the active-anim descriptor's first element's
// frame (+0x14), looks the per-cell name up by the m_entranceCell {col,row} triple (cell
// stride 0x68 at +0x468), applies the frame, then latches a fresh idle anim-set
// node (g_entranceAnimSrc.LookupAnimSet) into m_14->m_1c. __thiscall, ret 0.
RVA(0x000616e0, 0xa8)
i32 CGrunt::ResetGeometry() {
    m_prevEntranceDesc = m_154->m_1a0.m_14;
    m_154->m_1a0.Setup_15c2d0(m_poseAttackIdle);

    CAniElement* desc = m_154->m_1a0.m_14;
    i32* elem = desc->m_records.m_nSize > 0 ? (i32*)*desc->m_records.m_pData : 0;
    i32 frame = elem[0x14 / 4];

    i32 col = m_entranceCell.col;
    i32 row = m_entranceCell.row;
    i32 index = 3 * col + row;
    const char* name = (const char*)((zDArray*)&m_cells[index])->IndexToPtr(0);
    m_154->ApplyLookupSprite(name, frame);

    m_prevAnimSetNode = m_14->m_1c;
    m_14->m_1c = (void*)g_buteTree.Find(s_animKeyA);
    return 0;
}

// ---------------------------------------------------------------------------
// CGrunt::UpdateGruntStatus()   @0x617c0
// The per-frame grunt status step. If not powered-up (m_poweredUp==0) it just runs the
// entrance reset and bails. Otherwise it re-arms the entrance geometry source and,
// gated on the grunt's stamina (m_stamina): when full (>=100) it resolves + commits its
// grid-cell neighbour (same 15-wide m_tileMgr grid as FindGridNeighbor); when low
// (0x33..0x63, latched once via m_lowStaminaCued) and on-screen, it fires a spawn cue.
// __thiscall, ret 0, frameless.
// @early-stop
// lazy callee-saved-reg save: instruction MULTISET byte-identical vs retail
// (verified), logic/CFG/offsets exact; residue = retail defers `push edi` until
// AFTER the m_poweredUp==0 early-bail (the cold path uses only esi) where cl saves edi
// in the prolog. Pure regalloc placement, not steerable from source. ~94.5%.
RVA(0x000617c0, 0x127)
i32 CGrunt::UpdateGruntStatus() {
    if (m_poweredUp == 0) {
        ResetEntranceAnimation(1, 0, 0);
        return 0;
    }

    m_154->m_1a0.Advance(static_cast<u32>(g_engineFrameDelta));

    if (m_stamina >= 0x64) {
        if (m_neighborValid == 0) {
            return 0;
        }
        m_neighborValid = 0;
        CGrunt* n = m_tileMgr->m_grid[m_neighborCol * TM_GRID_COLS + m_neighborRow];
        if (n == 0 || n->m_entranceCommitted == 0) {
            return 0;
        }
        if (RectContains(n->m_10->m_screenX, n->m_10->m_screenY)) {
            CommitNeighbor(m_neighborCol, m_neighborRow, n->m_10->m_screenX, n->m_10->m_screenY);
        }
        return 0;
    }

    if (m_stamina <= 0x32) {
        return 0;
    }
    if (m_lowStaminaCued != 0) {
        return 0;
    }

    CGameRegistry* g = g_gameReg;
    i32 x = m_10->m_screenX;
    i32 y = m_10->m_screenY;
    i32* vr = (i32*)((i32)&g->m_world->m_level->m_mainPlane->m_originX);
    if (x < vr[2] && x >= vr[0] && y < vr[3] && y >= vr[1]) {
        g->m_cueSink->CueSpawn(this, 2, -1, -1, -1);
    }
    m_lowStaminaCued = 1;
    return 0;
}

// ---------------------------------------------------------------------------
// CGrunt::RearmAttackAnim(col, row)  @0x61940  (__thiscall, ret 8)
// Gated on m_entranceReason < 0x17. Latches the neighbor col/row (+0x200/+0x204),
// re-latches the "F" anim set, then switches on (m_entranceReason - 2) to pick the
// branch: reason 2 sets m_entranceActive when m_arrivalState is live; reasons
// 9/10/11/17/20/21/22 force index 1; else a rand-bit picks index 0/1. Sets the
// +0x218 combat latch, latches the combat-timer block, fires the focused-grunt drop
// cue when the grunt is on-screen, marks the HUD anim id dirty, drives the
// ATTACK1/ATTACK2 geometry by index, and re-stamps the entrance-cell frame.
//
// @early-stop
// switch/regalloc + cell-frame scratch-spill plateau: CFG, the (m_entranceReason-2)
// switch (the byte+jump table reloc-masks), every member offset/gate, the
// CreateHealthSprite/GetDwordDef/CueSpawn/SetGeometry/SetAnimFrame call shapes, the
// combat-timer block, the on-screen cue gate, and the HUD-dirty stamp are
// byte-faithful. Residue = the switch index register (ecx vs edx), the rand()%2 mask
// (and eax,1 vs the CSE'd ebx=1), the dead cell[2] read retail spills into a scratch
// frame, and the cross-arm regalloc; source-invariant. Deferred to the final sweep.
RVA(0x00061940, 0x1cf)
i32 CGrunt::RearmAttackAnim(i32 col, i32 row) {
    if (m_entranceReason >= 0x17) {
        return 0;
    }

    m_neighborCol = col;
    m_neighborRow = row;
    m_prevAnimSetNode = m_14->m_1c;
    m_14->m_1c = (void*)g_buteTree.Find(s_codeF);

    m_combatActive = 1;

    i32 idx;
    switch (m_entranceReason - 2) {
        case 0:
            if (m_arrivalState != 0) {
                m_entranceActive = 1;
            }
            idx = 1;
            break;
        case 7:
        case 8:
        case 9:
        case 15:
        case 18:
        case 19:
        case 20:
            idx = 1;
            break;
        default:
            idx = GruntRand() % 2;
            break;
    }

    CreateHealthSprite();

    m_combatTimeoutLo = static_cast<i32>(g_buteMgr.GetDwordDef(s_Grunt, s_CombatTimeout, 0x1388));
    m_combatTimeoutHi = 0;
    m_combatClockLo = static_cast<i32>(g_frameTime);
    m_combatClockHi = 0;

    {
        CGameObject* h = m_10;
        CGameRegistry* g = g_gameReg;
        i32 yy = h->m_screenY;
        i32 xx = h->m_screenX;
        i32* rect = (i32*)(*(i32*)(*(char**)((char*)g->m_world + 0x24) + 0x5c) + 0x40);
        if (xx < rect[2] && xx >= rect[0] && yy < rect[3] && yy >= rect[1]) {
            g->m_cueSink->CueSpawn(this, 1, -1, -1, -1);
        }
    }

    {
        CGameObject* h = m_10;
        i32 z = h->m_screenY + 0x186c1;
        if (h->m_latchedAnimId != z) {
            h->m_latchedAnimId = z;
            h->m_flags |= 0x20000;
        }
    }

    CGameObject* p = m_154;
    m_prevEntranceDesc = p->m_1a0.m_14;
    p->m_1a0.Setup_15c2d0((&m_poseAttack1)[idx]);

    CAniElement* desc = m_154->m_1a0.m_14;
    i32* el = desc->m_records.m_nSize > 0 ? (i32*)*desc->m_records.m_pData : 0;
    i32 frame = el[0x14 / 4];

    GruntEntranceCell cell = m_entranceCell;
    i32 cc = cell.col;
    i32 cr = cell.row;
    i32 base = cc + (cr + 2 * cc);
    i32 idx2 = base + base * 12;
    char* buf = ((CString*)((char*)this + idx2 * 8 + 0x468))->GetBuffer(0);
    m_154->ApplyLookupSprite(buf, frame);
    m_214 = 1;
    return 0;
}

// ---------------------------------------------------------------------------
// CGrunt::RearmAttackAnim2()  @0x61bc0  (__thiscall, ret 0)
// The simple ATTACK2 re-arm: re-latch the "F" anim set, drive the m_poseAttack2
// geometry, re-stamp the entrance-cell frame, set the +0x214 latch. Returns 0.
//
// @early-stop
// cell-frame scratch-spill plateau: CFG, every member offset, the g_buteTree.Find
// re-latch, the m_poseAttack2 geometry drive, the first-elem frame read, the
// (3*col+row)*0x68 cell-frame index, and the SetAnimFrame call are byte-faithful.
// Residue = retail loads the cell as a 3-int read and spills the dead cell[2] to a
// 0xc scratch frame (sub esp,0xc) where cl strips the unused read (no frame), plus
// the cell-base register; source-invariant. Deferred to the final sweep.
RVA(0x00061bc0, 0xb2)
i32 CGrunt::RearmAttackAnim2() {
    m_prevAnimSetNode = m_14->m_1c;
    m_14->m_1c = (void*)g_buteTree.Find(s_codeF);

    CGameObject* p = m_154;
    m_prevEntranceDesc = p->m_1a0.m_14;
    p->m_1a0.Setup_15c2d0(m_poseAttack2);

    CAniElement* desc = m_154->m_1a0.m_14;
    i32* el = desc->m_records.m_nSize > 0 ? (i32*)*desc->m_records.m_pData : 0;
    i32 frame = el[0x14 / 4];

    GruntEntranceCell cell = m_entranceCell;
    i32 col = cell.col;
    i32 row = cell.row;
    i32 base = col + (row + 2 * col);
    i32 idx2 = base + base * 12;
    char* buf = ((CString*)((char*)this + idx2 * 8 + 0x468))->GetBuffer(0);
    m_154->ApplyLookupSprite(buf, frame);
    m_214 = 1;
    return 0;
}

// ==== CGrunt::StepWarpExit @0x64540 (ex CUserLogic::winapi_064540_PostMessageA;
// text-contained in this obj) ====
// The `this` is CGrunt, PROVEN two ways: (1) RegisterActs_644af0 @0x5be30 stores
// its ILT thunk 0x13cf into the g_reg_644af0 act registry under the anim-code key
// "C" @0x60cc90 - the registry CGrunt::RunAct dispatches as PMFs on the grunt;
// (2) every offset is the CGrunt layout (m_154 entrance player + the identical
// m_154->m_flags |= 0x10000 retire idiom as FinishEntranceMove, m_tileOwnerHi/Lo fed
// to the tile-cell notify exactly as the attack step feeds CellDispatch, m_tileMgr
// +0x260, the m_36c suppress gate FinishEntranceMove also reads). The five old
// == CEntranceAnimPlayer, CWarpMgr == CGameRegistry (m_gameWnd +0x04 / m_curState
// +0x2c), CWarpMgrWnd == CGameWnd (<Wap32/Wap32.h>, m_hwnd +0x04), CWarpLevelReg
// == CState (<Gruntz/State.h>: m_levelIndex +0x1c, m_levelBank +0x28 - the level
// asset bank the "WORLDZ\LEVEL%i" key resolves against, level+100 = the secret
// level). CSymTab's definition here is BoundaryLowerMethodsViews.h's (the real
// <Bute/SymTab.h> def would redefine it in this TU; same one mangled symbol).
// The frame-clock snapshot fed to the arrival poke (ds:0x6bf3bc).
extern "C" i32 g_engineFrameDelta;
// The mgr singleton (same 0x64556c datum); the warp-dialog facet casts to CWarpMgr.
// (The old `WarpPostFn g_pPostMessageA` fn-ptr global @0x2c44c8 is GONE: that
// address is USER32's PostMessageA IAT slot - retail's `call [0x6c44c8]` is just
// the dllimport call. A plain ::PostMessageA call is the honest source.)
// @source: string-xref
// @early-stop
// jump-table-data-overlap wall (fuzzy % is an alignment artifact): logic complete
// and byte-verified vs retail (`sema disasm 0x61cb0 --diff`): the prologue is
// exact (`sub esp,8`; the retry flag in callee-saved ebx - the
// exact retail shape (`mov edx,[edi]; mov ecx,edi; call [edx+0x44]`, then
// `mov edi,[edi+0x154]; or [edi+0x8],0x10000`), and the 5-slot dense switch on
// m_entranceReason (the multiplexed tool kind; bias 2, range 0x14) + the named
// "Projectile"/"Boomerang"/"TimeBomb"
// /"AttackDowntime" DIR32 strings + g_gameReg/g_buteMgr/g_frameTime reproduce
// retail. Residues: (1) DOMINANT - cl emits the byte index-table + dword
// jump-table as $L COMDATs while the delinker INLINES the switchdataD_* tables
// into .text, so the tables never pair and objdiff mis-aligns the ENTIRE switch
// tail as inserted lines (docs/patterns/jumptable-data-overlap.md +
// switch-jumptable-separate-comdat.md - the % floor, not a logic diff);
// (2) minor - retail schedules the two zeroing xors (ebp/ebx) AFTER the Tick
// call, cl hoists them into the call-setup latency slots (2-line reorder, not
// source-steerable). Types 9-11 and 21-22 share one tail-merged "Projectile"
// arm across two jump-table slots.
RVA(0x00061cb0, 0x34a)
i32 CGrunt::StepAttackFire() {
    i32 flag = 0;
    if (m_154->m_1a0.Advance(g_engineFrameDelta) == 2) {
        // The +0x170 slot holds the grunt's current TOOL/attack kind here (the
        // GruntType/PickupType id space, <Gruntz/Enums.h>); >0x16 kinds = melee.
        switch (m_entranceReason) {
            case GRUNT_GUNHAT:
            case GRUNT_NERFGUN:
            case GRUNT_ROCK: {
                CGameObject* spr = g_gameReg->m_world->m_childGroup->CreateSprite(
                    0,
                    m_object->m_screenX,
                    m_object->m_screenY,
                    0,
                    "Projectile",
                    0x40003
                );
                spr->m_7c->m_notify(spr);
                CProjectile* s = (CProjectile*)spr->m_7c->m_logic;
                if (s->LoadProjectileSprites(
                        m_entranceReason,
                        m_tileOwnerHi,
                        m_tileOwnerLo,
                        m_208,
                        m_20c,
                        m_object->m_screenX,
                        m_object->m_screenY
                    )
                    == 0) {
                    s->m_sprite->m_flags |= 0x10000;
                }
                break;
            }
            case GRUNT_BOOMERANG: {
                CGameObject* spr = g_gameReg->m_world->m_childGroup->CreateSprite(
                    0,
                    m_object->m_screenX,
                    m_object->m_screenY,
                    0,
                    "Boomerang",
                    0x40003
                );
                spr->m_7c->m_notify(spr);
                CProjectile* s = (CProjectile*)spr->m_7c->m_logic;
                if (s->LoadProjectileSprites(
                        m_entranceReason,
                        m_tileOwnerHi,
                        m_tileOwnerLo,
                        m_208,
                        m_20c,
                        m_object->m_screenX,
                        m_object->m_screenY
                    )
                    == 0) {
                    s->m_sprite->m_flags |= 0x10000;
                }
                break;
            }
            case GRUNT_TIMEBOMB: {
                i32 pos[2];
                GetSpawnPos(pos);
                CGameObject* spr = g_gameReg->m_world->m_childGroup
                                       ->CreateSprite(0, pos[0], pos[1], 0xf, "TimeBomb", 0x40003);
                spr->m_120 = 0;
                spr->m_7c->m_notify(spr);
                spr->m_124 = m_tileOwnerHi;
                break;
            }
            case GRUNT_WELDER:
            case GRUNT_WINGZ: {
                CGameObject* spr = g_gameReg->m_world->m_childGroup->CreateSprite(
                    0,
                    m_object->m_screenX,
                    m_object->m_screenY,
                    0,
                    "Projectile",
                    0x40003
                );
                spr->m_7c->m_notify(spr);
                CProjectile* s = (CProjectile*)spr->m_7c->m_logic;
                if (s->LoadProjectileSprites(
                        m_entranceReason,
                        m_tileOwnerHi,
                        m_tileOwnerLo,
                        m_208,
                        m_20c,
                        m_object->m_screenX,
                        m_object->m_screenY
                    )
                    == 0) {
                    s->m_sprite->m_flags |= 0x10000;
                }
                break;
            }
            default: {
                // melee: hit the grunt at the neighbor grid cell (15-row grid).
                CGrunt* tgt = m_tileMgr->m_grid[m_neighborCol * TM_GRID_COLS + m_neighborRow];
                if (tgt == 0) {
                    flag = 1;
                    break;
                }
                tgt->TakeHit(
                    m_entranceReason,
                    m_214,
                    m_tileOwnerHi,
                    m_tileOwnerLo,
                    m_object->m_screenX,
                    m_object->m_screenY,
                    0,
                    m_gruntKind
                );
                i32 t = tgt->m_entranceReason;
                if (t > 0x16) {
                    t = tgt->m_19c;
                }
                if (t == 1 && m_gruntKind != GRUNT_INVULNERABLE) {
                    m_tileMgr->CellDispatch(m_tileOwnerHi, m_tileOwnerLo, 0xb, m_neighborCol);
                    return 0;
                }
                break;
            }
        }

        // impact tail (0x61f08)
        m_entranceActive = 1;
        i32 dt = g_buteMgr.GetDword(*(char**)&m_animSetName, "AttackDowntime");
        if (m_gruntKind == GRUNT_ROIDZ) {
            dt = 0; // Roidz grunt: no attack recovery time
        }
        m_attackDowntimeLo = dt;
        m_attackDowntimeHi = 0;
        m_860 = static_cast<i32>(g_frameTime);
        m_864 = 0;
        m_lowStaminaCued = 0;
        m_stamina = 0; // stamina drains fully at each attack
        if (m_healthSprite != 0) {
            NotifyAttackImpact();
        }
        m_combatActive = 0;
    }

    // finish tail (0x61f74): bail while the cursor is still un-armed or running.
    CGameObject* r = m_154;
    if ((r->m_1a0.m_28 == 0 || r->m_1a0.m_20 != 0) && flag == 0) {
        return 0;
    }
    if (m_entranceReason == GRUNT_BOOMERANG) {
        SetMoveStateA(0, 1, 0, 0);
    }
    CGameObject* h = m_object;
    i32 zkey = h->m_screenY + 0x186a0;
    if (h->m_latchedAnimId != zkey) {
        h->m_latchedAnimId = zkey;
        h->m_flags |= 0x20000;
    }
    i32 v220 = m_poweredUp;
    m_entranceActive = 0;
    if (v220 != 0) {
        FinishAttackPowered();
        return 0;
    }
    ReseedIdleReset(1, 0, 0);
    return 0;
}

// ===========================================================================
// The arrival/update dispatch trio (@0x59230 / 0x5caa0 /
// 0x62110). RTTI/this-layout IDENTIFY them as CGrunt methods, NOT CUserLogic:
// every member offset they touch (m_arrivalState 0x2d0, m_arrivalCol/m_arrivalRow, m_arrivalPhase
// 0x450, m_tileMgr 0x260, m_tileOwnerHi/Lo 0x1ec/0x1f0, m_14, m_154, the +0x470
// entrance-cell record table, the +0x4b0 dir-vector table, m_health 0x3ec, ...) is
// in the CGrunt layout above, and they call the same CGrunt this-method thunks the
// rest of this TU does. The "CUserLogic" stub attribution was a mislabel (CUserLogic
// is a small vptr+link base; it has no 0x2d0/0x450/0x4b0 fields - the same mislabel a
// prior matcher found for other CGrunt stubs, RTTI vtable 0x5e8754 = .?AVCGrunt@@).
// ===========================================================================

// The "ToyTime" bute key the update step reads (reloc-masked .rodata @0x60e194).
static char s_ToyTime[] = "ToyTime";

// ---------------------------------------------------------------------------
// CGrunt::UpdateArrival(a1, a2)   @0x62110   (__thiscall, ret 0x8)
// The per-frame arrival/entrance update step (sibling of UpdateEntranceAnim 0x690a0 and
// RunEntranceMove 0x67850). a2!=0 -> the commit pass (clear the coord sub, commit the
// in-flight occupied tile slot, reset the entrance latches, recycle the occupied-coord
// list onto the free pool + RemoveAll, OR 0x10000 into the toy/health sprite flag words,
// then either re-latch a "P" anim set + roll a rand toy pose + fire the cue, or load the
// ToyTime config + snapshot the clock). a1!=0 -> the "L" re-latch + walk geometry + cell
// SetAnimName + halved-ToyTime timer. a1==0 -> the "G" re-latch + HUD z-clamp + toy-timer
// pose select + the visible-bounds CueSpawn.
//
// @early-stop
// large-state-machine + reloc-masked-extern plateau (sibling of 0x690a0/0x637a0): CFG, the
// two-flag dispatch, every member offset/gate, the 15-stride tile index, the board attr
// chains, the rand()%N pose rolls, the 64-bit toy-timer compare/select, the +0x810/0x820
// timer snapshots, and all cue/anim call shapes are byte-faithful. Residue: the engine
// callees reached via incremental-link thunks reloc-mask to differently-named retail thunks,
// plus the cross-arm regalloc / zero-register pinning. Deferred to the final sweep.
RVA(0x00062110, 0x5bc)
i32 CGrunt::UpdateArrival(i32 a1, i32 a2) {
    if (a2 != 0) {
        ClearSubA();
        if (m_arrivalPhase == 3 && m_arrivalActive != 0) {
            CGrunt* occ = m_tileMgr->m_grid[m_arrivalCol * TM_GRID_COLS + m_arrivalRow];
            if (occ != 0) {
                CGameObject* inner = occ->m_10;
                i32 yMasked = (inner->m_screenY & ~0x1f) + 0x10;
                i32 xMasked = (inner->m_screenX & ~0x1f) + 0x10;
                if (RectContainsGated(xMasked, yMasked) != 0) {
                    m_tileMgr->ApplyTriggerB(
                        m_tileOwnerHi,
                        m_tileOwnerLo,
                        inner->m_screenX,
                        inner->m_screenY
                    );
                }
            }
        }

        if (m_poweredUp != 0 && m_neighborValid == 0) {
            m_entranceActive = 0;
            m_combatActive = 0;
            m_neighborValid = 0;
            m_poweredUp = 0;
            ReseedIdleReset(1, 0, 0);
        }
        m_entranceActive = 1;
        SetEntrancePos(1, 1);

        // Recycle the occupied-coord list (+0x320) onto the free pool, then RemoveAll.
        if (CoordCount() != 0) {
            void** node = (void**)CoordHead();
            while (node != 0) {
                void* next = node[0];
                void* buf = node[2];
                if (buf != 0) {
                    void** sp = (void**)((char*)buf - g_coordPool.m_linkOffset);
                    *sp = g_coordPool.m_freeHead;
                    g_coordPool.m_freeHead = sp;
                }
                node = (void**)next;
            }
            m_31c.RemoveAll();
        }

        m_entranceStamped = 0;
        if (m_healthSprite != 0) {
            m_healthSprite->m_flags |= 0x10000;
            m_healthSprite = 0;
        }
        if (m_toySprite != 0) {
            m_toySprite->m_flags |= 0x10000;
            m_toySprite = 0;
        }

        if (m_entranceReason == 0x1e) {
            m_prevAnimSetNode = m_14->m_1c;
            m_14->m_1c = (void*)g_buteTree.Find(s_codeP);
            i32 toyIdx = rand() % 2;
            m_prevEntranceDesc = m_154->m_1a0.m_14;
            m_154->ApplyGeometryDirect((&m_poseToy1)[toyIdx], 0);

            CAniElement* desc = m_154->m_1a0.m_14;
            i32* el = desc->m_records.m_nSize > 0 ? (i32*)*desc->m_records.m_pData : 0;
            i32 frame = el[0x14 / 4];
            char* buf = ((CString*)&m_448)->GetBuffer(0);
            m_154->ApplyLookupSprite(buf, frame);

            i32 cueTier = ((toyIdx != 0) ? 0xa : 0) + 0x406;
            CGameRegistry* g = g_gameReg;
            i32 m380 = m_moveVariant;
            if (m380 != 0) {
                i32 tier = cueTier + m380 - 1;
                i32 anchor = *(i32*)(*(char**)((char*)g->m_world + 0x24) + 0x5c) + 0x40;
                if (GruntPointVisible(m_10->m_screenY, m_10->m_screenX, anchor) != 0) {
                    g->m_cueSink->CueA(this, tier, 0, -1, -1, -1);
                }
            } else {
                if (m_moveKind == 0) {
                    i32 md = (g->m_134 == 1) ? 3 : 6;
                    m_moveKind = rand() % md + 1;
                }
                i32 tier = cueTier + m_moveKind - 1;
                i32 anchor = *(i32*)(*(char**)((char*)g->m_world + 0x24) + 0x5c) + 0x40;
                if (GruntPointVisible(m_10->m_screenY, m_10->m_screenX, anchor) != 0) {
                    g->m_cueSink->CueA(this, tier, 0, -1, -1, -1);
                }
            }
            return 0;
        } else {
            DWORD tt = g_buteMgr.GetDword(*(char**)&m_animSetName, s_ToyTime);
            m_toyDurationLo = static_cast<i32>(tt);
            m_toyDurationHi = 0;
            m_toyClockLo = static_cast<i32>(g_frameTime);
            m_toyClockHi = 0;
            m_toyTime = 0x64;
            CreateToyTimeSprite();
        }
    }

    if (a1 != 0) {
        // a1 != 0: the "L" re-latch + walk geometry + cell SetAnimName + halved-ToyTime timer.
        m_toyTileIndex = 0;
        if (m_poweredUp != 0 && m_neighborValid == 0) {
            m_entranceActive = 0;
            m_combatActive = 0;
            m_neighborValid = 0;
            m_poweredUp = 0;
            ReseedIdleReset(1, 0, 0);
        }
        m_prevAnimSetNode = m_14->m_1c;
        m_14->m_1c = (void*)g_buteTree.Find(s_codeL);
        m_prevEntranceDesc = m_154->m_1a0.m_14;
        m_154->m_1a0.Setup_15c2d0(m_poseWalk);
        GruntEntranceCell cell = m_entranceCell;
        i32 colv = cell.row + cell.col * 2;
        i32 basev = cell.col + colv;
        char* nm = m_cells[basev].m_walk.GetBuffer(0);
        m_154->ApplyName(nm);

        DWORD tt = g_buteMgr.GetDword(*(char**)&m_animSetName, s_ToyTime);
        m_idleDelayLo = static_cast<i32>((tt >> 1));
        m_idleDelayHi = 0;
        m_idleAnchorLo = static_cast<i32>(g_frameTime);
        m_idleAnchorHi = 0;
        return 0;
    }

    // a1 == 0: the "G" re-latch + HUD z-clamp + toy-timer pose select + visible-bounds cue.
    m_prevAnimSetNode = m_14->m_1c;
    m_14->m_1c = (void*)g_buteTree.Find(s_codeG);

    CGameObject* h = m_10;
    i32 z = h->m_screenY + 0xc3500;
    if (h->m_latchedAnimId != z) {
        h->m_latchedAnimId = z;
        h->m_flags |= 0x20000;
    }

    // Pick the active toy pose by comparing the two toy-pose timers (m_3c4/m_3c8 ->+0x24)
    // against the elapsed toy timer (m_toyClockLo/m_toyClockHi - clock), then re-stamp on change.
    i32 t0 = *(i32*)((char*)m_poseToy1 + 0x24);
    i32 t1 = *(i32*)((char*)m_poseToy2 + 0x24);
    i64 elapsed = *(i64*)&m_toyClockLo - static_cast<i64>(static_cast<u32>(g_frameTime));
    i32 cap = static_cast<i32>(elapsed);
    if (elapsed < 0) {
        cap = 0;
    }
    i32 d0 = (t0 > cap) ? (t0 - cap) : 0;
    i32 d1 = (t1 > cap) ? (t1 - cap) : 0;
    i32 sel;
    if (d0 != 0) {
        sel = (d1 != 0) ? ((d0 < d1) ? 0 : 1) : 0;
    } else if (d1 != 0) {
        sel = 1;
    } else {
        i32 r = rand() % 0x64 + 1;
        sel = (r >= m_toyBlendPct) ? 1 : 0;
    }

    CAniElement* cur = m_154->m_1a0.m_14;
    CAniElement* want = (&m_poseToy1)[sel];
    if (cur != want) {
        m_prevEntranceDesc = m_154->m_1a0.m_14;
        m_154->m_1a0.Setup_15c2d0(want);
        CAniElement* desc = m_154->m_1a0.m_14;
        i32* el = desc->m_records.m_nSize > 0 ? (i32*)*desc->m_records.m_pData : 0;
        i32 frame = el[0x14 / 4];
        char* buf = ((CString*)&m_448)->GetBuffer(0);
        m_154->ApplyLookupSprite(buf, frame);
    }

    // The visible-bounds cue: probe the grunt's HUD point against the live view rect,
    // fire CueSpawn(this, 0xa|0xb, -1,-1,-1) when inside.
    CGameObject* hud = m_10;
    CGameRegistry* g = g_gameReg;
    i32 yy = hud->m_screenY;
    i32 xx = hud->m_screenX;
    i32* rectBase = (i32*)(*(char**)((char*)g->m_world + 0x24) + 0x5c);
    i32 lim = rectBase[0x48 / 4];
    i32* rect = (i32*)((char*)rectBase + 0x40);
    if (sel != 0) {
        if (xx < lim && xx >= rect[0] && yy < rect[3] && yy >= rect[1]) {
            g->m_cueSink->CueSpawn(this, 0xb, -1, -1, -1);
        }
    } else {
        if (xx < lim && xx >= rect[0] && yy < rect[3] && yy >= rect[1]) {
            g->m_cueSink->CueSpawn(this, 0xa, -1, -1, -1);
        }
    }
    return 0;
}

// ===========================================================================
// Chunk-2 attributed targets (RearmAttack family + entrance-move tail). Same
// raw-offset campaign style; reconstructed in ascending retail-RVA order.
// ===========================================================================
// (s_CombatTimeout is defined near the top of this TU; shared with CommitNeighbor.)

// ---------------------------------------------------------------------------
// CGrunt::StepEntranceRelatchA()   @0x62840   (ret 0)
// Advance the entrance geometry. When the sub-player is armed-but-not-running:
// (re)create the HUD stat sprites on arrival, re-latch the "A"(idle) anim set,
// reload the grunt type table, then (board-block-bit set) commit the arrival move,
// else clamp the HUD scroll. When NOT armed-but-running: the toy-break timer path -
// once the toy timer has elapsed and the entrance is unstamped and the sub-player
// just went ready, retire the toy-time HUD sprite, drive the TOY-BREAK geometry +
// re-stamp the cell frame, and fire the on-screen spawn cue; otherwise tick the
// coord list / update the arrival.
RVA(0x00062840, 0x25d)
i32 CGrunt::StepEntranceRelatchA() {
    i32 ready = m_154->m_1a0.Advance(static_cast<u32>(g_engineFrameDelta));
    CAniAdvanceCursor* sub = &m_154->m_1a0;
    if (sub->m_28 != 0 && sub->m_20 == 0) {
        if (m_arrived != 0) {
            CreateHealthSprite();
            CreateStaminaSprite();
            CreateToySprite();
        }
        m_prevAnimSetNode = m_14->m_1c;
        m_14->m_1c = (void*)g_buteTree.Find(s_codeA);
        LoadGruntTypeTable(m_19c, 1, 0, 0);
        m_entranceActive = 0;
        CGameRegistry* g = g_gameReg;
        CTileGrid* grid = g->m_tileGrid;
        i32 tx = m_lastTilePxX >> 5;
        i32 ty = m_lastTilePxY >> 5;
        i32 flags;
        if (static_cast<u32>(tx) >= static_cast<u32>(grid->m_c) || static_cast<u32>(ty) >= static_cast<u32>(grid->m_10)) {
            flags = 1;
        } else {
            flags = ((i32*)grid->m_8[ty])[tx * 7];
        }
        if (flags & 0x80) {
            SetEntrancePos(1, 1);
            m_tileMgr->WireTileSwitchLogic(this, m_lastTilePxX, m_lastTilePxY);
            return 0;
        }
        CGameObject* h = m_10;
        i32 v = h->m_screenY + 0x186a0;
        if (h->m_latchedAnimId != v) {
            h->m_latchedAnimId = v;
            h->m_flags |= 0x20000;
        }
        return 0;
    }
    // sub-player armed-but-still-running: the toy-break timer path.
    i64 diff = static_cast<i64>(static_cast<u32>(g_frameTime)) - *(i64*)&m_toyClockLo;
    if (diff >= *(i64*)&m_toyDurationLo && m_entranceStamped == 0 && ready == 1) {
        if (m_toyTimeSprite != 0) {
            m_toyTimeSprite->m_flags |= 0x10000;
            m_toyTimeSprite = 0;
        }
        m_prevEntranceDesc = m_154->m_1a0.m_14;
        m_154->m_1a0.Setup_15c2d0(m_poseToyBreak);
        CAniElement* desc = m_154->m_1a0.m_14;
        CAniDesc* elem = desc->m_records.m_nSize > 0 ? (CAniDesc*)*desc->m_records.m_pData : 0;
        i32 frame = elem->m_param;
        char* nm = ((CString*)&m_448)->GetBuffer(0);
        m_154->ApplyLookupSprite(nm, frame);
        m_entranceStamped = 1;
        CGameObject* h = m_10;
        CGameRegistry* g = g_gameReg;
        i32 x = h->m_screenX;
        i32 y = h->m_screenY;
        CCueRect* r = (CCueRect*)((i32)&g->m_world->m_level->m_mainPlane->m_originX);
        if (x < r->right && x >= r->left && y < r->bottom && y >= r->top) {
            g->m_cueSink->CueSpawn(this, 0xc, -1, -1, -1);
        }
        return 0;
    }
    ClearSubA();
    if (ready == 1) {
        UpdateArrival(0, 0);
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CGrunt_SegBoxOverlap(p, e1, e2)  @0x62b70  (__stdcall, ret 0xc)
// Does the directed segment e1->e2 cross into the axis-aligned box `p`
// {x0,y0,x1,y1}? Tests the segment against each box edge (top/bottom horizontals
// at y0/y1, left/right verticals at x0/x1): when the two endpoints straddle the
// edge line, interpolate the crossing coordinate in float and test it falls within
// the opposite span. Returns 1 on the first crossing, else 0. Pure stack args.
//
// @early-stop
// x87 FP instruction-scheduling wall (same family as ComputeFacing 0x57060 /
// docs/patterns): CFG, the 4 edge tests, every straddle setl/setg sign test, the
// int subtractions feeding fild/fidiv/fimul/fiadd, and the fcompp comparison
// structure are byte-faithful in shape/order. Residue = the x87 register-stack
// scheduling + which spill slot holds each interpolation operand (source-invariant
// on this leaf). Deferred to the final sweep.
// reloc-fidelity: 0x62b70 IS CGrunt::RectSegProbe - CGrunt's rect/seg-probe caller
// (winapi_04a9f0) invokes it thiscall-style (mov ecx,this ignored; 3 pushed args) so it
// emits ?RectSegProbe@CGrunt@@. SYMBOL exports it under that name so those 4 calls bind;
// the body stays the byte-matched __stdcall leaf (it ignores ecx / uses pure stack args).
SYMBOL(?RectSegProbe@CGrunt@@QAEHPAX00@Z)
RVA(0x00062b70, 0x205)
i32 __stdcall CGrunt_SegBoxOverlap(GruntBox* p, GruntSegEnd* e1, GruntSegEnd* e2) {
    i32 e1y = e1->m_4;
    i32 e2y = e2->m_4;

    // Top edge (y = p->m_4): straddle in y, interpolate x.
    i32 py = p->m_4;
    if ((e1y < py) != (e2y < py)) {
        float t = static_cast<float>((py - e1y)) / static_cast<float>((e2y - e1y));
        float ix = static_cast<float>(e1->m_0) + t * static_cast<float>((e2->m_0 - e1->m_0));
        if (static_cast<float>(p->m_0) <= ix && ix <= static_cast<float>(p->m_8)) {
            return 1;
        }
    }

    // Bottom edge (y = p->m_c).
    i32 pyc = p->m_c;
    if ((e1y < pyc) != (e2y < pyc)) {
        float t = static_cast<float>((pyc - e1y)) / static_cast<float>((e2y - e1y));
        float ix = static_cast<float>(e1->m_0) + t * static_cast<float>((e2->m_0 - e1->m_0));
        if (static_cast<float>(p->m_0) <= ix && ix <= static_cast<float>(p->m_8)) {
            return 1;
        }
    }

    // Left edge (x = p->m_0): straddle in x, interpolate y.
    i32 e1x = e1->m_0;
    i32 e2x = e2->m_0;
    i32 px = p->m_0;
    if ((e1x > px) != (e2x > px)) {
        float t = static_cast<float>((e2x - px)) / static_cast<float>((e2x - e1x));
        float iy = static_cast<float>(e1y) + t * static_cast<float>((e2y - e1y));
        if (static_cast<float>(p->m_4) <= iy && iy <= static_cast<float>(p->m_c)) {
            return 1;
        }
    }

    // Right edge (x = p->m_8).
    i32 pxr = p->m_8;
    if ((e1x > pxr) != (e2x > pxr)) {
        float t = static_cast<float>((e2x - pxr)) / static_cast<float>((e2x - e1x));
        float iy = static_cast<float>(e1y) + t * static_cast<float>((e2y - e1y));
        if (static_cast<float>(p->m_4) <= iy && iy <= static_cast<float>(p->m_c)) {
            return 1;
        }
    }

    return 0;
}

// ---------------------------------------------------------------------------
// CGrunt::ResetEntranceAnimation(int apply, int cycle, int cue)   @0x62e10
// The shared entrance/idle-anim reset the entrance state machine (and its two
// callers BuildEntranceAnimation + LoadEntranceConfig) run to (re)select and
// arm the grunt's entrance animation. __thiscall, ret 0xc (/GX - the per-cell
// key CString temp carries a C++ EH frame).
//
//   * clears the "applied" flag (m_resetApplied = 0), then reverse-looks-up the current
//     active-anim-set node's NAME and tests it against the idle key "A".
//   * if the grunt is NOT already idle and `cycle`==0: re-anchor the idle timer
//     (m_idleAnchorLo.. bookkeeping) off the IdleDelay config (rand window).
//   * else dispatches on the geometry-source array m_poseIdle[0..2]: a single source
//     (m_3b0==0) re-arms it; the 2-arg branch (cycle!=0) randomly cycles among
//     the available sources, firing a focused-grunt on-screen "cue" (consts
//     4/5/6 by index) when `cue`!=0 and the grunt is visible.
//   * latches a fresh active-anim-set node into m_14->m_1c (saving the old into
//     m_prevAnimSetNode), and finally - when something was applied or `apply`!=0 - rebuilds
//     the per-cell entrance-position key string (CString from the m_474 cell
//     table, indexed 3*col+row by either the per-grunt triple m_entranceCell or a preset
//     by reason m_444) and stamps the first frame.
static i32 s_entrancePreset0[3]; // DAT_00644aa0
static i32 s_entrancePreset1[3]; // DAT_00644ac0
static i32 s_entrancePreset2[3]; // DAT_00644ad0

RVA(0x00062e10, 0x47e)
void CGrunt::ResetEntranceAnimation(i32 apply, i32 cycle, i32 cue) {
    m_resetApplied = 0;

    i32 notIdle = strcmp(*g_typeColl.GetNameRecord(m_14->m_1c), s_animKeyA) != 0;
    i32 applied = 0;

    if (notIdle && cycle == 0) {
        // Re-anchor the idle timer to a randomized IdleDelay window.
        m_prevEntranceDesc = m_154->m_1a0.m_14;
        m_154->m_1a0.Setup_15c2d0(m_poseIdle[0]);
        m_idleWindowLo = 0x3a98;
        m_idleWindowHi = 0;
        m_idleTimerLo = static_cast<i32>(g_frameTime);
        m_idleTimerHi = 0;
        i32 n = static_cast<i32>(g_buteMgr.GetDwordDef(s_Grunt, s_IdleDelay, 0x7530)) + 1;
        m_idleDelayLo = GruntRand() % n + 0x7530;
        m_idleDelayHi = 0;
        m_idleAnchorLo = static_cast<i32>(g_frameTime);
        m_idleAnchorHi = 0;
        applied = 1;
    } else if (m_poseIdle[1] == 0) {
        // Single geometry source: re-arm it (no flag set).
        m_prevEntranceDesc = m_154->m_1a0.m_14;
        m_154->m_1a0.Setup_15c2d0(m_poseIdle[0]);
    } else if (cycle == 0) {
        // Already on this source? nothing to do.
        if (m_154->m_1a0.m_14 == m_poseIdle[0]) {
            goto latch;
        }
        m_prevEntranceDesc = m_154->m_1a0.m_14;
        m_154->m_1a0.Setup_15c2d0(m_poseIdle[0]);
        {
            i32 d = static_cast<i32>(g_buteMgr.GetDwordDef(s_Grunt, s_IdleDelay, 0x7530));
            applied = 1;
            m_idleDelayLo = GruntRand() % (d - 0x4e1f) + 0x4e20;
            m_idleDelayHi = 0;
            m_idleAnchorLo = static_cast<i32>(g_frameTime);
            m_idleAnchorHi = 0;
        }
    } else {
        // Cycle among the available sources, with the focused-grunt cue.
        i32 count = (m_poseIdle[2] == 0) ? 1 : 2;
        i32 idx = GruntRand() % count + 1;
        if (cue != 0) {
            CGameRegistry* g = g_gameReg;
            g->CuePrep();
            i32 focused = (m_tileOwnerHi == g_curPlayer);
            if (focused && idx > 0x5a) {
                if (CueVisible(
                        (i32)&g->m_world->m_level->m_mainPlane->m_originX,
                        m_10->m_screenX,
                        m_10->m_screenY
                    )) {
                    g->m_cueSink->Cue((i32)this, 4, -1, -1, -1);
                }
            } else if (focused || m_entranceReason != 0) {
                if (idx == 1) {
                    if (CueVisible(
                            (i32)&g->m_world->m_level->m_mainPlane->m_originX,
                            m_10->m_screenX,
                            m_10->m_screenY
                        )) {
                        g->m_cueSink->Cue((i32)this, 5, -1, -1, -1);
                    }
                } else if (idx == 2) {
                    if (CueVisible(
                            (i32)&g->m_world->m_level->m_mainPlane->m_originX,
                            m_10->m_screenX,
                            m_10->m_screenY
                        )) {
                        g->m_cueSink->Cue((i32)this, 6, -1, -1, -1);
                    }
                }
            }
        }
        m_prevEntranceDesc = m_154->m_1a0.m_14;
        m_154->m_1a0.Setup_15c2d0(m_poseIdle[idx]);
        m_resetApplied = 1;
        applied = 1;
    }

latch:
    m_prevAnimSetNode = m_14->m_1c;
    m_14->m_1c = (void*)g_buteTree.Find(s_animKeyA);

    if (!applied && apply == 0) {
        return;
    }

    // Rebuild the per-cell entrance key string + first frame. The cell is the
    // per-grunt triple {col,row,reason}, unless a non-default entrance reason
    // selects a preset triple.
    i32 col = m_entranceCell.col;
    i32 row = m_entranceCell.row;
    i32 reason = m_entranceCell.reason;
    if (m_154->m_1a0.m_14 != m_poseIdle[0]) {
        switch (reason) {
            case 2:
            case 3:
                col = s_entrancePreset0[0];
                row = s_entrancePreset0[1];
                break;
            case 4:
            case 5:
                col = s_entrancePreset1[0];
                row = s_entrancePreset1[1];
                break;
            case 6:
            case 7:
            case 8:
                col = s_entrancePreset2[0];
                row = s_entrancePreset2[1];
                break;
            default:
                break;
        }
    }

    CString key = (const char*)&m_cells[3 * col + row].m_idle;

    CAniElement* desc = m_154->m_1a0.m_14;
    i32* elem = desc->m_records.m_nSize > 0 ? (i32*)*desc->m_records.m_pData : 0;
    EntranceApplyFrame(key, elem[0x14 / 4]);
}

// ---------------------------------------------------------------------------
// CGrunt::ResolveEntranceArrival()   @0x633e0
// The per-tick entrance-arrival resolver: once the grunt has settled on its
// destination tile it commits the "arrival" (claims the tile, seeds the
// per-grunt defender bookkeeping, clears the view-cull state on m_10) and, when
// the entrance window has elapsed + the grunt is off-screen/unfocused, runs the
// entrance reset (ResetEntranceAnimation). __thiscall, ret 0.
//
//   * if the entrance is active (m_entranceActive) and the grunt has not moved off its
//     last tile (m_5c==m_lastTilePxX, m_60==m_lastTilePxY) and that tile's high occupancy bit is
//     clear, clear m_entranceActive.
//   * arm the entrance geometry source (m_154->m_1a0.SetGeoSourceR); gate the
//     arrival on the elapsed-time window (clock - m_830_64 >= m_838_64) and the
//     grunt being off-screen (registry m_134 != 1) + its focus slot live.
//   * the arrival commit: notify the tile manager, latch m_tileClaimed, seed the
//     defender block (m_defenderX..m_arrivalRerollWindowHi, m_arrivalState/m_defenderState/
//     m_defenderRadius/m_arrivalCol/m_arrivalRow, m_arrivalFlags |= 0x18040402),
//     clear m_10's view-cull rect, run the arrival hook.
//   * tail: if the entrance anim is done (m_154->m_1a0.m_14 != m_poseIdle[0]) run
//     ResetEntranceAnimation(0,0,0) on the lookup-miss flags; else, when the idle window has
//     elapsed and the geometry source is ready, run ResetEntranceAnimation(0,1,1).
RVA(0x000633e0, 0x2ca)
void CGrunt::ResolveEntranceArrival() {
    if (m_entranceActive != 0 && m_10->m_screenX == m_lastTilePxX
        && m_10->m_screenY == m_lastTilePxY) {
        CGameRegistry* g = g_gameReg;
        CTileGrid* grid = g->m_tileGrid;
        i32 tx = m_10->m_screenX >> 5;
        i32 ty = m_10->m_screenY >> 5;
        i32 flags;
        if (static_cast<u32>(tx) >= static_cast<u32>(grid->m_c) || static_cast<u32>(ty) >= static_cast<u32>(grid->m_10)) {
            flags = 1;
        } else {
            flags = ((i32*)grid->m_8[ty])[tx * 7];
        }
        if (!(flags & 0x80)) {
            m_entranceActive = 0;
        }
    }

    i32 ready = m_154->m_1a0.Advance(static_cast<u32>(g_engineFrameDelta));

    if (static_cast<i64>(static_cast<u32>(g_frameTime)) - *(i64*)&m_idleTimerLo >= *(i64*)&m_idleWindowLo) {
        CGameRegistry* g = g_gameReg;
        i32 mode = g->m_134;
        if (mode != 1) {
            CFocusSlot* slot = &g->m_focusSlots[m_tileOwnerHi];
            if (slot != 0 && slot->m_14 != 0) {
                if (m_tileClaimed == 0 && m_arrivalNotified == 0 && mode == 2
                    && g_curPlayer == m_tileOwnerHi && m_arrived == 0) {
                    m_tileMgr->PostCellCommand6(m_tileOwnerHi, m_tileOwnerLo); // 0x275c -> 0x6da60
                    m_arrivalNotified = 1;
                    goto tail;
                }
                if (mode != 2 && g_curPlayer == m_tileOwnerHi && m_arrived == 0
                    && m_tileClaimed != 1) {
                    m_arrivalRerollLo = 0;
                    m_arrivalRerollWindowLo = 0;
                    m_arrivalRerollHi = 0;
                    m_arrivalRerollWindowHi = 0;
                    m_defenderX = m_lastTilePxX;
                    m_defenderY = m_lastTilePxY;
                    m_tileClaimed = 1;
                    i32 kind = m_entranceReason;
                    switch (kind) {
                        case 2:
                            m_defenderRadius = 1;
                            break;
                        default:
                            m_defenderRadius =
                                g_buteMgr.GetIntDef(s_Grunt, s_PlayerDefenderRadius, 3) + 1;
                            break;
                    }
                    m_arrivalCol = -1;
                    m_arrivalRow = -1;
                    m_arrivalState = 4;
                    m_defenderState = 0;
                    m_arrivalActive = 0;
                    m_arrivalFlags |= 0x18040402;
                    m_10->m_extentL = 0;
                    m_10->m_extentR = 0;
                    m_10->m_extentT = 0;
                    m_10->m_extentB = 0;
                    EntranceArrivalHook(0, 0);
                }
            }
        }
    }

tail:
    if (m_154->m_1a0.m_14 != m_poseIdle[0]) {
        if (m_154->m_1a0.m_28 == 0 && m_154->m_1a0.m_20 != 0) {
            ResetEntranceAnimation(0, 0, 0);
        }
        return;
    }
    if (static_cast<i64>(static_cast<u32>(g_frameTime)) - *(i64*)&m_idleAnchorLo >= *(i64*)&m_idleDelayLo && ready == 1) {
        ResetEntranceAnimation(0, 1, 1);
    }
}

// ---------------------------------------------------------------------------
// CGrunt::StepEntranceReinit()   @0x637a0   (ret 0)
// @early-stop
// large-state-machine + grid-regalloc plateau: the D/L early rejects, the +0x8c0
// struck-timer reset, the "I"-code tile-mgr re-notify + idle reseed, and the two
// visibility-gated anim-set re-latch + entrance-cell frame re-stamp arms are
// reconstructed in shape/order. Residue is the strcmp-eq setcc sentinel pinning
// (docs/patterns/strcmp-eq-bool-local-setcc + zero-register-pinning), the deep
// g_gameReg->m_tileGrid board chains modeled by raw offset, and the cross-arm regalloc on
// the shared cell-frame tail. Deferred to the final sweep.
RVA(0x000637a0, 0x2f8)
i32 CGrunt::StepEntranceReinit() {
    bool eq;
    eq = (strcmp(*g_typeColl.GetNameRecord(m_14->m_1c), s_codeD) == 0);
    if (eq) {
        return 0;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_14->m_1c), s_codeL) == 0);
    if (eq) {
        return 0;
    }
    // Reset the struck-cooldown timer window (m_8c0..m_8cc) to a fresh 0x7530 from
    // the running game clock.
    m_8c8 = 0x7530;
    m_8cc = 0;
    m_8c0 = static_cast<i32>(g_frameTime);
    m_8c4 = 0;
    m_358 = 0;

    eq = (strcmp(*g_typeColl.GetNameRecord(m_14->m_1c), s_codeI) == 0);
    if (eq) {
        // code "I": re-notify the tile mgr of the arrival.
        m_tileMgr->LoadTileArrivalFx(
            m_tileOwnerHi,
            m_tileOwnerLo,
            m_moveTileX,
            m_moveTileY,
            m_entranceReason,
            -1
        );
    }
    if (m_poweredUp != 0 && m_neighborValid == 0) {
        m_entranceActive = 0;
        m_combatActive = 0;
        m_neighborValid = 0;
        m_poweredUp = 0;
        ReseedIdleReset(1, 0, 0);
    }
    m_35c = 0;
    if (CoordCount() == 0) {
        return 0;
    }

    // The head occupied-coord's tile is clear of the spawn-block bit -> re-latch a
    // fresh "D" anim set and re-stamp the first entrance-cell frame.
    GruntCoord* co = (GruntCoord*)m_31c.GetHead();
    CTileGrid* b = g_gameReg->m_tileGrid;
    i32 flag;
    if (static_cast<u32>(co->m_x) >= static_cast<u32>(b->m_c) || static_cast<u32>(co->m_y) >= static_cast<u32>(b->m_10)) {
        flag = 1;
    } else {
        flag = ((i32*)b->m_8[co->m_y])[co->m_x * 7];
    }
    if (!(flag & 0x20000000)) {
        m_prevAnimSetNode = m_14->m_1c;
        m_14->m_1c = (void*)g_buteTree.Find(s_codeD);
        m_prevEntranceDesc = m_154->m_1a0.m_14;
        m_154->m_1a0.Setup_15c2d0(m_poseWalk);
    } else {
        // The grunt's own HUD point is unobstructed (the 0x80 walkable bit).
        i32 tx = m_10->m_screenX >> 5;
        i32 ty = m_10->m_screenY >> 5;
        i32 flag2;
        if (static_cast<u32>(tx) >= static_cast<u32>(b->m_c) || static_cast<u32>(ty) >= static_cast<u32>(b->m_10)) {
            flag2 = 1;
        } else {
            flag2 = ((i32*)b->m_8[ty])[tx * 7];
        }
        if (!(flag2 & 0x80)) {
            return 0;
        }
        m_entranceActive = 1;
        m_prevAnimSetNode = m_14->m_1c;
        m_14->m_1c = (void*)g_buteTree.Find(s_codeD);
        m_prevEntranceDesc = m_154->m_1a0.m_14;
        m_154->m_1a0.Setup_15c2d0(m_poseWalk);
    }
    GruntEntranceCell cell = m_entranceCell;
    i32 col = cell.row + cell.col * 2;
    i32 base = cell.col + col;
    // &m_cells[base].m_walk == this + base*0x68 + 0x470: cl strength-reduces base*0x68
    // (sizeof CGruntCellRec) to the same lea chain the raw offset produced (verified
    // byte-identical) - no imul, so the real member access is faithful.
    char* nm = m_cells[base].m_walk.GetBuffer(0);
    m_154->ApplyName(nm);
    return 0;
}

// ---------------------------------------------------------------------------
// CGrunt::StepArrivalReroll()   @0x63b60   (ret 0)
// The arrival re-roll idle timer: advance the entrance geometry, then once the
// struck-cooldown timer (m_8c0) has elapsed past 10000ms on an exact 1000ms tick,
// roll the MS-CRT LCG (inlined) against a shrinking window and, when it clears the
// 0x7148 threshold, roll a 0..100 pick and fire the on-screen event/spawn cue
// (CueEvent when pick>0x19, else CueSpawn) if the grunt is inside the visible rect.
// @early-stop
// complete reconstruction (all logic incl. the three inlined LCG rolls); ~76%
// plateau. Residue is the saturated-elapsed i64 sbb/branch regalloc + the repeated
// inline-rand seed materialization scheduling (not source-steerable) + reloc-masked
// cue operands. Correct shape + control flow; a codegen wall.
RVA(0x00063b60, 0x1cf)
i32 CGrunt::StepArrivalReroll() {
    m_154->m_1a0.Advance(static_cast<u32>(g_engineFrameDelta));
    i64 diff = static_cast<i64>(static_cast<u32>(g_frameTime)) - *(i64*)&m_8c0;
    u32 elapsed;
    if (diff >= 0) {
        elapsed = static_cast<u32>(diff);
    } else {
        elapsed = 0;
    }
    if (elapsed <= 0x2710) {
        return 0;
    }
    if (elapsed % 1000 != 0) {
        return 0;
    }
    i32 v;
    i32 range = 0x7531 - elapsed;
    u32 x;
    if (range == 0) {
        if (!(g_randSeeded & 1)) {
            g_randSeeded |= 1;
            x = ::timeGetTime();
        } else {
            x = g_randSeed;
        }
        g_randSeed = x * 214013 + 2531011;
        if (g_randSeed & 0x10000) {
            v = elapsed;
        } else {
            v = 0x7530;
        }
    } else {
        if (!(g_randSeeded & 1)) {
            g_randSeeded |= 1;
            x = ::timeGetTime();
        } else {
            x = g_randSeed;
        }
        g_randSeed = x * 214013 + 2531011;
        v = ((static_cast<i32>(g_randSeed) >> 16) & 0x7fff) % range + elapsed;
    }
    if (v <= 0x7148) {
        return 0;
    }
    u32 x2;
    if (!(g_randSeeded & 1)) {
        g_randSeeded |= 1;
        x2 = ::timeGetTime();
    } else {
        x2 = g_randSeed;
    }
    g_randSeed = x2 * 214013 + 2531011;
    i32 pick = ((static_cast<i32>(g_randSeed) >> 16) & 0x7fff) % 0x65;
    CGameObject* h = m_10;
    i32 y = h->m_screenY;
    i32 xp = h->m_screenX;
    CGameRegistry* g = g_gameReg;
    CCueRect* r = (CCueRect*)((i32)&g->m_world->m_level->m_mainPlane->m_originX);
    if (pick > 0x19) {
        if (xp < r->right && xp >= r->left && y < r->bottom && y >= r->top) {
            g->m_cueSink->CueEvent(this, 0x15d, -1, 0, -1, -1);
        }
    } else {
        if (xp < r->right && xp >= r->left && y < r->bottom && y >= r->top) {
            g->m_cueSink->CueSpawn(this, 9, -1, -1, -1);
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CGrunt::BuildEntranceAnimation(int mode)   @0x67bd0
// Selects + loads the grunt's entrance animation (the "drop in / resurrect /
// random arrival" sequence). Latches a fresh active-anim-set node into m_14->m_1c
// (saving the old into m_prevAnimSetNode), seeds the entrance bookkeeping (m_entranceArmed=1, m_entranceCommitted=0,
// m_entranceActive=1), marks the HUD geometry dirty (m_10->m_74 = 0xcf850; m_10->m_8 |=
// 0x20000), then picks an entrance-key string by `mode`:
//   mode==1 : a rand()%0x1e1-bucketed arrival (ENTRANCEZ_ONE / _TWO / _THREE)
//   mode==2 : ENTRANCEZ_DROP
//   else    : ENTRANCEZ_RESSURECT (key) / DEATHZ_MELT (base)
// looks that sprite-set up in the entrance player's table (m_154->m_c->m_2c map),
// fires a 6-arg on-screen "cue" when the grunt is visible/focused, copies the base
// "GRUNTZ_ENTRANCEZ"/"GRUNTZ_DEATHZ_MELT" key into a CString, and finally either
// runs the entrance reset (ResetEntranceAnimation(1,0,0)) on a lookup miss, or applies the
// resolved geometry to the player's sub-player (+0x1a0) plus the first frame.
// /GX (the CString temp carries a C++ EH frame). __thiscall, ret 4.
//
// `mode`-string table (reloc-masked .rodata literals):
static const char s_GRUNTZ_ENTRANCEZ[] = "GRUNTZ_ENTRANCEZ";
static const char s_GRUNTZ_ENTRANCEZ_ONE[] = "GRUNTZ_ENTRANCEZ_ONE";
static const char s_GRUNTZ_ENTRANCEZ_TWO[] = "GRUNTZ_ENTRANCEZ_TWO";
static const char s_GRUNTZ_ENTRANCEZ_THREE[] = "GRUNTZ_ENTRANCEZ_THREE";
static const char s_GRUNTZ_ENTRANCEZ_DROP[] = "GRUNTZ_ENTRANCEZ_DROP";
static const char s_GRUNTZ_ENTRANCEZ_RESSURECT[] = "GRUNTZ_ENTRANCEZ_RESSURECT";
static const char s_GRUNTZ_DEATHZ_MELT[] = "GRUNTZ_DEATHZ_MELT";

// BuildGruntExitAnimation (@0x641b0) keys (reloc-masked .rodata).
static const char s_exitKeyB[] = "B";                            // 0x60d1bc
static const char s_GRUNTZ_EXITZ[] = "GRUNTZ_EXITZ";             // 0x60bd28
static const char s_GRUNTZ_EXITZ_ONE[] = "GRUNTZ_EXITZ_ONE";     // 0x60e250
static const char s_GRUNTZ_EXITZ_TWO[] = "GRUNTZ_EXITZ_TWO";     // 0x60e23c
static const char s_GRUNTZ_EXITZ_THREE[] = "GRUNTZ_EXITZ_THREE"; // 0x60e224

// LoadVehicleGruntAnimations (@0x63db0) vehicle-grunt looping-sound keys.
static const char s_GRUNTZ_GOKARTGRUNT[] = "GRUNTZ_GOKARTGRUNT_GOKARTGRUNTLOOP";       // 0x60e1f8
static const char s_GRUNTZ_BIGWHEELGRUNT[] = "GRUNTZ_BIGWHEELGRUNT_BIGWHEELGRUNTLOOP"; // 0x60e1c8

// ---------------------------------------------------------------------------
// CGrunt::LoadVehicleGruntAnimations()   @0x63db0   (__thiscall, ret 0)
// Re-arms the vehicle-grunt (gokart/bigwheel) entrance: re-seeds the entrance
// player geometry, and on the armed-but-not-running gate (m_1a0.m_28!=0 &&
// m_1a0.m_20==0) (re)creates the HUD stat sprites when arrived, latches the idle
// ("A") anim-set node, reloads the type table and commits the arrival when the
// last tile carries the 0x80 attribute. Otherwise it runs the two entrance time-
// window cue tiers (a 64-bit elapsed-time compare against the +0x810/+0x820 safe-
// time anchors): the past-safe path stamps the TOY-BREAK pose + fires cue 0xc, and
// the within-window path fires cue 0xd and dispatches the looping vehicle sound by
// kind (m_170 0x1a=gokart, 0x19=bigwheel).
//
// @early-stop
// ~95.4%: CFG, every member offset, the 64-bit elapsed-time compares, all three
// visibility gates + cue ids, and the kind dispatch are byte-faithful. GruntStrGetBuffer
// is now the real __thiscall CString::GetBuffer(this=&m_448) (`lea ecx; push 0; call`).
// Residue = the toy-break-setup edx/eax push-order + elapsed `xor` register coin-flips
// (the documented regalloc tail).
RVA(0x00063db0, 0x32f)
void CGrunt::LoadVehicleGruntAnimations() {
    m_154->m_1a0.Advance(static_cast<u32>(g_engineFrameDelta));

    CAniAdvanceCursor* sub = &m_154->m_1a0;
    if (sub->m_28 != 0 && sub->m_20 == 0) {
        if (m_arrived) {
            CreateHealthSprite();
            CreateStaminaSprite();
            CreateToySprite();
        }
        m_prevAnimSetNode = m_14->m_1c;
        m_14->m_1c = (void*)g_buteTree.Find(s_animKeyA);
        LoadGruntTypeTable(m_19c, 1, 0, 0);
        m_entranceActive = 0;

        CTileGrid* grid = g_gameReg->m_tileGrid;
        i32 tx = m_lastTilePxX >> 5;
        i32 ty = m_lastTilePxY >> 5;
        i32 flags;
        if (static_cast<u32>(tx) >= static_cast<u32>(grid->m_c) || static_cast<u32>(ty) >= static_cast<u32>(grid->m_10)) {
            flags = 1;
        } else {
            flags = ((i32*)grid->m_8[ty])[tx * 7];
        }
        if (flags & 0x80) {
            SetEntrancePos(1, 1);
            m_tileMgr->WireTileSwitchLogic(this, m_lastTilePxX, m_lastTilePxY);
        }
        return;
    }

    i64 elapsed = static_cast<i64>(static_cast<u64>(g_frameTime)) - *(i64*)&m_toyClockLo;
    if (elapsed >= *(i64*)&m_toyDurationLo) {
        if (m_entranceStamped == 0 && m_10->m_screenX == m_lastTilePxX
            && m_10->m_screenY == m_lastTilePxY) {
            if (m_toyTimeSprite) {
                m_toyTimeSprite->m_flags |= 0x10000;
                m_toyTimeSprite = 0;
            }
            SetEntrancePos(1, 1);
            m_entranceStamped = 1;
            m_prevEntranceDesc = m_154->m_1a0.m_14;
            m_154->ApplyGeometryDirect(m_poseToyBreak, 0);

            CAniElement* desc = m_154->m_1a0.m_14;
            i32* elem = desc->m_records.m_nSize > 0 ? (i32*)*desc->m_records.m_pData : 0;
            char* buf = ((CString*)&m_448)->GetBuffer(0);
            m_154->ApplyLookupSprite(buf, elem[0x14 / 4]);

            CGameObject* h = m_10;
            CGameRegistry* g = g_gameReg;
            i32 x = h->m_screenX;
            i32 y = h->m_screenY;
            i32* rect = (i32*)((i32)&g->m_world->m_level->m_mainPlane->m_originX);
            if (x < rect[2] && x >= rect[0] && y < rect[3] && y >= rect[1]) {
                g->m_cueSink->CueSpawn(this, 0xc, -1, -1, -1);
                ClearSubA();
                return;
            }
        }
        ClearSubA();
        return;
    }

    i64 elapsed2 = static_cast<i64>(static_cast<u64>(g_frameTime)) - *(i64*)&m_idleAnchorLo;
    if (elapsed2 >= *(i64*)&m_idleDelayLo) {
        CGameObject* h = m_10;
        CGameRegistry* g = g_gameReg;
        i32 x = h->m_screenX;
        i32 y = h->m_screenY;
        i32* rect = (i32*)((i32)&g->m_world->m_level->m_mainPlane->m_originX);
        if (x < rect[2] && x >= rect[0] && y < rect[3] && y >= rect[1]) {
            g->m_cueSink->CueSpawn(this, 0xd, -1, -1, -1);
        }
    }

    CGameObject* h2 = m_10;
    CGameRegistry* g2 = g_gameReg;
    i32 hx = h2->m_screenX;
    i32 hy = h2->m_screenY;
    if (hx < g2->m_viewOriginR && hx >= g2->m_viewOriginL && hy < g2->m_viewOriginB
        && hy >= g2->m_viewOriginT) {
        if (m_entranceReason == 0x1a) {
            EnsureStruckSlot(s_GRUNTZ_GOKARTGRUNT);
            return;
        }
        if (m_entranceReason == 0x19) {
            EnsureStruckSlot(s_GRUNTZ_BIGWHEELGRUNT);
            return;
        }
        return;
    }
    ClearSubA();
}

// ---------------------------------------------------------------------------
// CGrunt::BuildGruntExitAnimation()   @0x641b0   (__thiscall, ret 0)
// Sibling of BuildEntranceAnimation: bail if the exit anim is already running
// (m_deathAnimStarted!=0), tear down the running anim state (StepAnimDispatchB/ClearSubA/B),
// clear the per-grunt sprite-state bit (m_10->m_40 &= ~8), retire all 7 HUD stat
// sprites (|= 0x10000 then null the slot), drop the powered-up reset, commit the
// struck tile, latch a fresh "B" anim-set node, then rand-bucket the EXITZ_ONE/
// TWO/THREE variant (each: lookup sprite + on-screen cue 0x384/5/6), and finally
// drive the exit holder + apply the "GRUNTZ_EXITZ" first-frame geometry.
RVA(0x000641b0, 0x2c1)
i32 CGrunt::BuildGruntExitAnimation() {
    if (m_deathAnimStarted != 0) {
        return 0;
    }

    StepAnimDispatchB();
    ClearSubA();
    ClearSubB();

    m_10->m_stateFlags &= ~8;
    m_entranceCommitted = 0;
    m_deathAnimStarted = 1;

    if (m_healthSprite) {
        m_healthSprite->m_flags |= 0x10000;
        m_healthSprite = 0;
    }
    if (m_staminaSprite) {
        m_staminaSprite->m_flags |= 0x10000;
        m_staminaSprite = 0;
    }
    if (m_toySprite) {
        m_toySprite->m_flags |= 0x10000;
        m_toySprite = 0;
    }
    if (m_toyTimeSprite) {
        m_toyTimeSprite->m_flags |= 0x10000;
        m_toyTimeSprite = 0;
    }
    if (m_wingzTimeSprite) {
        m_wingzTimeSprite->m_flags |= 0x10000;
        m_wingzTimeSprite = 0;
    }
    if (m_powerupSprite) {
        m_powerupSprite->m_flags |= 0x10000;
        m_powerupSprite = 0;
    }
    if (m_selectedSprite) {
        m_selectedSprite->m_flags |= 0x10000;
        m_selectedSprite = 0;
    }

    m_gruntKind = 0;
    if (m_poweredUp != 0 && m_neighborValid == 0) {
        m_entranceActive = 0;
        m_combatActive = 0;
        m_neighborValid = 0;
        m_poweredUp = 0;
        ResetEntranceAnimation(1, 0, 0);
    }

    m_entranceActive = 1;
    m_tileMgr->RemoveCellRecord(m_tileOwnerHi, m_tileOwnerLo, 1);

    m_prevAnimSetNode = m_14->m_1c;
    m_14->m_1c = (void*)g_buteTree.Find(s_exitKeyB);

    CSprite* found;
    i32 r = GruntRand() % 0x1e1;
    if (r > 0x140) {
        found = (CSprite*)m_154->m_0c->m_animRegistry->LookupValue_06b2a0(s_GRUNTZ_EXITZ_ONE);
        CGameRegistry* g = g_gameReg;
        if (GruntPointVisible(
                (i32)&g->m_world->m_level->m_mainPlane->m_originX,
                m_10->m_screenX,
                m_10->m_screenY
            )) {
            g->m_cueSink->CueA(this, 0x384, -1, 0, -1, -1);
        }
    } else if (r > 0xa0) {
        found = (CSprite*)m_154->m_0c->m_animRegistry->LookupValue_06b2a0(s_GRUNTZ_EXITZ_TWO);
        CGameRegistry* g = g_gameReg;
        if (GruntPointVisible(
                (i32)&g->m_world->m_level->m_mainPlane->m_originX,
                m_10->m_screenX,
                m_10->m_screenY
            )) {
            g->m_cueSink->CueA(this, 0x385, -1, 0, -1, -1);
        }
    } else {
        found = (CSprite*)m_154->m_0c->m_animRegistry->LookupValue_06b2a0(s_GRUNTZ_EXITZ_THREE);
        CGameRegistry* g = g_gameReg;
        if (GruntPointVisible(
                (i32)&g->m_world->m_level->m_mainPlane->m_originX,
                m_10->m_screenX,
                m_10->m_screenY
            )) {
            g->m_cueSink->CueA(this, 0x386, -1, 0, -1, -1);
        }
    }

    ((CEffect6b*)(&m_150))->Apply((i32)found, 0);
    i32* elem = (i32*)m_154->m_1a0.m_14->AtChecked_06b270(0);
    i32 frame = elem[0x14 / 4];
    m_154->ApplyLookupSprite(s_GRUNTZ_EXITZ, frame);
    return 0;
}

// @early-stop
// 86.4%: logic byte-faithful. Residual is the register scheduling around the m_154
// reloads + the /GX CString unwind state ordering; not source-steerable.
RVA(0x00064540, 0x11c)
i32 CGrunt::StepWarpExit() {
    m_154->m_1a0.Advance(g_engineFrameDelta);
    CAniAdvanceCursor* sub = &m_154->m_1a0;
    if (sub->m_28 == 0) {
        return 0;
    }
    if (sub->m_20 != 0) {
        return 0;
    }
    if (m_deathType == GRUNT_DEATH_WARPOUT) {
        CState* st = g_gameReg->m_curState;
        i32 lvl = st->m_levelIndex + 0x64; // secret level = level + 100
        CString s;
        s.Format("WORLDZ\\LEVEL%i", lvl);
        if (st->m_levelBank->ResolveQualified((LPCTSTR)s, (void*)REZ_TAG_WWD)) {
            PostMessageA(g_gameReg->m_gameWnd->m_hwnd, WM_COMMAND, GOTOLEVEL, lvl);
        }
    }
    if (m_36c == 0) {
        m_tileMgr->NotifyCell(m_tileOwnerHi, m_tileOwnerLo, 1);
    }
    m_154->m_flags |= 0x10000;
    return 0;
}

// ---------------------------------------------------------------------------
// CGrunt::StepCombatReaction(...)   @0x646b0   (ret 0x20, 8 stack args)
// The combat-reaction anim-dispatch state machine (proximity-MISATTRIBUTED to
// CUserLogic; really CGrunt - see the header note). Gated on the entrance being
// committed (m_1fc) and idle (m_entranceDropActive==0), it clamps the HUD scroll, resolves the
// grunt's current anim name via g_typeColl and dispatches on its single-
// letter type code (A/D/I/G/L/P/O/Q/J/N), driving the grunt's combat/arrival
// bookkeeping; then the shared tail re-arms the combat-timeout window, forwards the
// 8 args down, resolves the "F"/"O" scratch codes, drives the attack-pose geometry +
// entrance-cell frame, and fires the focused-grunt spawn cue when on-screen.
//
// @early-stop
// large anim-dispatch state-machine plateau (the same family as StepEntranceReinit /
// RunEntranceMove in this TU): the +0x1fc/+0x364 gate, the HUD-scroll clamp, the 10
// inline-strcmp dispatch arms + their state transitions, the m_1a0 move-mode switch,
// the combat-timeout re-arm, the 8-arg forward, the two scratch-resolver (GetNameRecords
// + scratch CString teardown) re-latches, the cell-frame restamp and the on-screen
// spawn-cue gate are all reconstructed in shape/order. Residue is the shared
// strcmp-eq setcc/zero-register pinning (no source spelling), the scratch loop-
// strength-reduction, and the deep cross-arm regalloc. Final sweep.
RVA(0x000646b0, 0x9c8)
i32 CGrunt::StepCombatReaction(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7) {
    if (m_entranceCommitted == 0 || m_entranceDropActive != 0) {
        return 0;
    }
    {
        CGameObject* h = m_10;
        i32 v = h->m_screenY + 0x186a0;
        if (h->m_latchedAnimId != v) {
            h->m_latchedAnimId = v;
            h->m_flags |= 0x20000;
        }
    }

    if (strcmp(*g_typeColl.GetNameRecord(m_14->m_1c), s_codeA) == 0) {
        goto tail;
    }
    if (strcmp(*g_typeColl.GetNameRecord(m_14->m_1c), s_codeD) == 0) {
        goto tail;
    }
    if (strcmp(*g_typeColl.GetNameRecord(m_14->m_1c), s_codeI) == 0) {
        if (m_entranceReason == 0x13) {
            g_gameReg->m_cueSink->Cue1(m_10->m_188);
        }
        m_tileMgr->LoadTileArrivalFx(
            m_tileOwnerHi,
            m_tileOwnerLo,
            m_moveTileX,
            m_moveTileY,
            m_entranceReason,
            -1
        );
        goto tail;
    }
    if (strcmp(*g_typeColl.GetNameRecord(m_14->m_1c), s_codeG) == 0) {
        goto reject;
    }
    if (strcmp(*g_typeColl.GetNameRecord(m_14->m_1c), s_codeL) == 0) {
        goto reject;
    }
    if (strcmp(*g_typeColl.GetNameRecord(m_14->m_1c), s_codeP) == 0) {
        goto reject;
    }
    if (strcmp(*g_typeColl.GetNameRecord(m_14->m_1c), s_codeO) == 0) {
        ApplySetState1(1);
        m_tileMgr->WireTileSwitchLogic(this, m_lastTilePxX, m_lastTilePxY);
        goto tail;
    }
    if (strcmp(*g_typeColl.GetNameRecord(m_14->m_1c), s_codeQ) == 0) {
        m_tileMgr->CellDispatch(m_tileOwnerHi, m_tileOwnerLo, 6, a2);
        return 0;
    }
    if (strcmp(*g_typeColl.GetNameRecord(m_14->m_1c), s_codeJ) == 0) {
        m_entranceActive = 0;
        if (strcmp(*g_typeColl.GetNameRecord(m_prevAnimSetNode), s_codeD) == 0) {
            if (m_poweredUp != 0 && m_neighborValid == 0) {
                m_entranceActive = 0;
                m_combatActive = 0;
                m_neighborValid = 0;
                m_poweredUp = 0;
                ReseedIdleReset(1, 0, 0);
            }
            m_35c = 0;
            m_prevAnimSetNode = m_14->m_1c;
            m_14->m_1c = (void*)g_buteTree.Find(s_codeD);
            m_prevEntranceDesc = m_154->m_1a0.m_14;
            m_154->m_1a0.Setup_15c2d0(m_poseWalk);
            GruntEntranceCell cell = m_entranceCell;
            i32 col = cell.row + cell.col * 2;
            i32 base = cell.col + col;
            i32 row = base * 3;
            i32 idx = base + row * 4;
            char* cn = ((CString*)((char*)this + idx * 8 + 0x470))->GetBuffer(0);
            m_154->ApplyName(cn);
        } else {
            ReseedIdleReset(1, 0, 0);
        }
        i32 mode = m_moveMode;
        if (mode >= 0x32) {
            SetMoveStateA(mode, 1, 0, 1);
            m_moveMode = -1;
            m_1a4 = 0;
        } else if (mode >= 0x22) {
            m_194 = mode;
            m_moveMode = -1;
        } else if (mode >= 0x17) {
            EmitMoveCueQ(mode);
        } else {
            SetMoveStateA(mode, 1, 0, 1);
            m_moveMode = -1;
        }
        goto tail;
    }
    if (strcmp(*g_typeColl.GetNameRecord(m_14->m_1c), s_codeN) == 0) {
        CGameObject* h = m_10;
        i32 hx = (h->m_screenX & ~0x1f) + 0x10;
        i32 hy = (h->m_screenY & ~0x1f) + 0x10;
        i32 flag = 1;
        if (hx != m_lastTilePxX || hy != m_lastTilePxY) {
            if (IsDropReady(1)) {
                m_coordToggle = (m_coordToggle == 0) ? 1 : 0;
                flag = 0;
            }
        }
        ApplySetState1(1);
        if (flag != 0) {
            m_prevAnimSetNode = m_14->m_1c;
            m_14->m_1c = (void*)g_buteTree.Find(s_codeD);
            OnCoordCommit(m_coordToggle);
        }
    }
    goto tail;

reject:
    if (m_entranceReason == 0x1e) {
        g_gameReg->m_cueSink->Cue1(m_10->m_188);
    }
    SetMoveStateA(m_19c, 1, 0, 1);
    {
        CGameObject* h = m_10;
        i32 v = h->m_screenY + 0x186a0;
        if (h->m_latchedAnimId != v) {
            h->m_latchedAnimId = v;
            h->m_flags |= 0x20000;
        }
    }
    if (m_toyTimeSprite != 0) {
        m_toyTimeSprite->m_flags |= 0x10000;
        m_toyTimeSprite = 0;
    }
    m_toyTime = 0;
    StepCoordTick();

tail:
    UpdateCombatTimer();
    m_combatTimeoutLo = g_buteMgr.GetIntDef(s_Grunt, s_CombatTimeout, 0x1388);
    m_combatTimeoutHi = 0;
    m_combatClockLo = static_cast<i32>(g_frameTime);
    m_combatClockHi = 0;
    if (m_10->m_screenX != m_lastTilePxX || m_10->m_screenY != m_lastTilePxY) {
        OnTileMismatch(1);
    }
    if (ForwardCombatStep(a0, a1, a2, a3, a4, a5, a6, a7) == 0) {
        return 0;
    }

    {
        CAnimNameRecord* rec = g_typeColl.GetNameRecords(m_14->m_1c);
        GruntScratchTeardown();
        if (strcmp(rec->m_name, s_codeF) == 0) {
            if (m_entranceCommitted != 0) {
                return 0;
            }
        }
    }
    m_entranceActive = 1;
    {
        CAnimNameRecord* rec = g_typeColl.GetNameRecords(m_14->m_1c);
        GruntScratchTeardown();
        if (strcmp(rec->m_name, s_codeO) != 0) {
            m_prevAnimSetNode = m_14->m_1c;
            m_14->m_1c = (void*)g_buteTree.Find(s_codeH);
            void* cellObj = m_tileMgr->m_grid[a2 * TM_GRID_COLS + a3];
            if (cellObj != 0) {
                CGameObject* oh = ((CGrunt*)cellObj)->m_10;
                i32 cx = oh->m_screenX;
                i32 cy = oh->m_screenY;
                if (m_358 != 0 && m_entranceCommitted != 0 && IsInCombatRange(cx, cy)) {
                    if (!(s_TileFlags(g_gameReg->m_tileGrid, m_lastTilePxX >> 5, m_lastTilePxY >> 5)
                          & 0x80)) {
                        CommitCombatMove(a2, a3, cx, cy);
                    }
                }
            }
        }
    }

    m_combatActive = 0;
    CAniElement* pose = (&m_poseStruck1)[a1];
    m_prevEntranceDesc = m_154->m_1a0.m_14;
    m_154->m_1a0.Setup_15c2d0(pose);
    i32 frame;
    {
        CAniElement* desc = m_154->m_1a0.m_14;
        i32* elem;
        if (desc->m_records.m_nSize > 0) {
            elem = (i32*)desc->m_records.m_pData[0];
        } else {
            elem = 0;
        }
        frame = elem[0x14 / 4];
    }
    {
        GruntEntranceCell cell = m_entranceCell;
        i32 col = cell.row + cell.col * 2;
        i32 base = cell.col + col;
        i32 row = base * 3;
        i32 idx = base + row * 4;
        char* cn = ((CString*)((char*)this + idx * 8 + 0x46c))->GetBuffer(0);
        m_154->ApplyLookupSprite(cn, frame);
    }
    {
        CGameObject* h = m_10;
        i32 vx = h->m_screenX;
        i32 vy = h->m_screenY;
        char* sc = *(char**)((char*)g_gameReg->m_world + 0x24);
        i32* rect = (i32*)(*(char**)(sc + 0x5c) + 0x40);
        if (vx < rect[2] && vx >= rect[0] && vy < rect[3] && vy >= rect[1]) {
            g_gameReg->m_cueSink->CueSpawn(this, 7, -1, -1, -1);
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CGrunt::StepArrivalCommitA()   @0x65300   (ret 0)
// Advance the entrance geometry; once the sub-player is armed-but-not-running:
// if the grunt is dead (m_health <= 0) clear the entrance-commit flag and set the
// tile back, else clear the entrance-active flag and test the head tile's spawn-
// block bit (0x80): set -> commit the arrival move; clear + (m_358==0 && m_35c!=0)
// -> arrival-drop; else m_entranceReason==0x14 -> reset the entrance anim; else
// reset the geometry.
RVA(0x00065300, 0x148)
i32 CGrunt::StepArrivalCommitA() {
    m_154->m_1a0.Advance(static_cast<u32>(g_engineFrameDelta));
    CAniAdvanceCursor* sub = &m_154->m_1a0;
    if (sub->m_28 == 0 || sub->m_20 != 0) {
        return 0;
    }
    if (m_health <= 0) {
        m_entranceCommitted = 0;
        m_tileMgr->CellDispatch(m_tileOwnerHi, m_tileOwnerLo, 1, m_370);
        return 0;
    }
    m_entranceActive = 0;
    CGameRegistry* g = g_gameReg;
    CTileGrid* grid = g->m_tileGrid;
    i32 tx = m_lastTilePxX >> 5;
    i32 ty = m_lastTilePxY >> 5;
    i32 flags;
    if (static_cast<u32>(tx) >= static_cast<u32>(grid->m_c) || static_cast<u32>(ty) >= static_cast<u32>(grid->m_10)) {
        flags = 1;
    } else {
        flags = ((i32*)grid->m_8[ty])[tx * 7];
    }
    if (flags & 0x80) {
        SetEntrancePos(1, 1);
        m_tileMgr->WireTileSwitchLogic(this, m_lastTilePxX, m_lastTilePxY);
        return 0;
    }
    if (m_358 == 0 && m_35c != 0) {
        StepArrivalDrop(m_commitPxX, m_commitPxY, 0, -1, 1, 0);
        return 0;
    }
    if (m_entranceReason == 0x14) {
        ResetEntranceAnimation(1, 0, 0);
        return 0;
    }
    ResetGeometry();
    return 0;
}

// ---------------------------------------------------------------------------
// CGrunt::StepArrivalCommitB()   @0x654b0   (ret 0)
// Sibling of StepArrivalCommitA: once armed-but-not-running, unconditionally snap
// to the last tile + commit the arrival move, then the same health/board-gated
// tail (health<=0 -> set tile back; else block-bit set -> return; else drop or
// reset the geometry).
RVA(0x000654b0, 0x130)
i32 CGrunt::StepArrivalCommitB() {
    // 0x15c360 is CAniAdvanceCursor::Advance (cast the m_1a0 geometry facet)
    m_154->m_1a0.Advance(static_cast<u32>(g_engineFrameDelta));
    CAniAdvanceCursor* sub = &m_154->m_1a0;
    if (sub->m_28 == 0 || sub->m_20 != 0) {
        return 0;
    }
    m_entranceActive = 0;
    SnapToLastTile(1);
    SetEntrancePos(1, 1);
    // 0x6c130 = CTriggerMgr::WireTileSwitchLogic, 0x6bcb0 = CTriggerMgr::CellDispatch
    m_tileMgr->WireTileSwitchLogic(this, m_lastTilePxX, m_lastTilePxY);
    if (m_health <= 0) {
        m_entranceCommitted = 0;
        m_tileMgr->CellDispatch(m_tileOwnerHi, m_tileOwnerLo, 1, m_370);
        return 0;
    }
    CGameRegistry* g = g_gameReg;
    CTileGrid* grid = g->m_tileGrid;
    i32 tx = m_lastTilePxX >> 5;
    i32 ty = m_lastTilePxY >> 5;
    i32 flags;
    if (static_cast<u32>(tx) >= static_cast<u32>(grid->m_c) || static_cast<u32>(ty) >= static_cast<u32>(grid->m_10)) {
        flags = 1;
    } else {
        flags = ((i32*)grid->m_8[ty])[tx * 7];
    }
    if (flags & 0x80) {
        return 0;
    }
    if (m_358 == 0 && m_35c != 0) {
        StepArrivalDrop(m_commitPxX, m_commitPxY, 0, -1, 1, 0);
        return 0;
    }
    ResetGeometry();
    return 0;
}

// ---------------------------------------------------------------------------
// CGrunt::RunMoveConfig(a, b)   @0x65630   (__thiscall, ret 8)
// The move-config / entrance-reason dispatch. If the current anim is the "I"
// (idle) record it commits the tile load; otherwise (when on-screen) it fires the
// move cue (8). It then plays the move sound at (a,b), records the move tile
// (m_moveTileX/m_moveTileY), drops the powered-up reset, and dispatches on m_entranceReason:
//   1    -> BOMBGRUNT run config ("M" anim set + RunningTimePerTile bute read)
//   0x12 -> "N" anim set + toggle m_coordToggle
//   0x13 -> "I" anim set + random idle/item variant cue (CueA), pose m_3d0[poseIdx]
//   else -> "I" anim set
// and applies the resolved pose geometry + entrance-cell name to the player.
//
// @early-stop
// ~72%: full CFG, the m_entranceReason switch (1/0x12/0x13/else), the inline
// strcmp (eq=sete idiom, shared with StepArrivalCommit), all bute reads, both cue
// variants, and the entrance-cell pose/name tail are byte-faithful. Residue = a
// whole-function stack-frame coin-flip: retail allocates 0xc scratch locals (it
// spills the entrance-cell triple + an extra temp for the cell-stride math),
// where cl reuses registers and allocates 4 - shifting every [esp+N] arg/local
// offset by 8 and cascading a register-renumber through the 843-byte body. No
// source lever forces the larger frame; deferred to the final sweep.
RVA(0x00065630, 0x34b)
void CGrunt::RunMoveConfig(i32 a, i32 b) {
    i32 poseIdx = 0; // ebx

    i32 eq = (strcmp(*g_typeColl.GetNameRecord(m_14->m_1c), s_codeI) == 0);
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
        CGameObject* h = m_10;
        CGameRegistry* g = g_gameReg;
        i32* rect = (i32*)((i32)&g->m_world->m_level->m_mainPlane->m_originX);
        if (GruntPointVisible((i32)rect, h->m_screenX, h->m_screenY)) {
            g->m_cueSink->CueSpawn(this, 8, -1, -1, -1);
        }
    }

    PlayMoveSoundAtTile(a, b);
    m_moveTileX = a;
    m_moveTileY = b;
    if (m_poweredUp != 0 && m_neighborValid == 0) {
        m_entranceActive = 0;
        m_combatActive = 0;
        m_neighborValid = 0;
        m_poweredUp = 0;
        ResetEntranceAnimation(1, 0, 0);
    }

    if (m_entranceReason == 1) {
        m_prevAnimSetNode = m_14->m_1c;
        m_14->m_1c = (void*)g_buteTree.Find(s_codeM);
        m_10->m_stateFlags &= ~8;
        m_timePerTile = g_buteMgr.GetDwordDef(s_BOMBGRUNT, s_RunningTimePerTile, 0x64);
        m_entranceActive = 1;
        m_22c = 1;
        SetEntrancePos(1, 1);
    } else if (m_entranceReason == 0x12) {
        m_entranceActive = 1;
        m_prevAnimSetNode = m_14->m_1c;
        m_14->m_1c = (void*)g_buteTree.Find(s_codeN);
        m_coordToggle = (m_coordToggle == 0);
    } else if (m_entranceReason == 0x13) {
        i32 base;
        if (GruntRand() % 0x64 < 0x50) {
            poseIdx = 1;
            base = 0x41a;
        } else {
            poseIdx = 0;
            base = 0x424;
        }

        i32 variant = m_374;
        m_moveVariant = variant;
        if (variant == 0) {
            i32 n = (g_gameReg->m_134 == 1) ? 3 : 6;
            m_moveVariant = GruntRand() % n + 1;
        }

        i32 cueId = base + m_moveVariant - 1;
        CGameObject* h = m_10;
        CGameRegistry* g = g_gameReg;
        i32 x = h->m_screenX;
        i32 y = h->m_screenY;
        i32* rect = (i32*)((i32)&g->m_world->m_level->m_mainPlane->m_originX);
        if (x < rect[2] && x >= rect[0] && y < rect[3] && y >= rect[1]) {
            g->m_cueSink->CueA(this, cueId, -1, 0, -1, -1);
        }

        m_prevAnimSetNode = m_14->m_1c;
        m_14->m_1c = (void*)g_buteTree.Find(s_codeI);
        m_entranceActive = 1;
        SetEntrancePos(1, 1);
    } else {
        m_prevAnimSetNode = m_14->m_1c;
        m_14->m_1c = (void*)g_buteTree.Find(s_codeI);
        SetEntrancePos(1, 1);
    }

    m_prevEntranceDesc = m_154->m_1a0.m_14;
    m_154->m_1a0.Setup_15c2d0((&m_poseItem)[poseIdx]);

    GruntEntranceCell cell = m_entranceCell;
    i32 col = cell.row + cell.col * 2;
    i32 base = cell.col + col + 0xb;
    i32 idx = base + base * 3 * 4;
    char* name = ((CString*)((char*)this + idx * 8))->GetBuffer(0);
    m_154->ApplyName(name);
}

// LoadWandGruntItemConfig (0x65a60): per-frame wand-grunt item logic. Advance the
// arrival probe; on the peak phase (0x63) latch the item downtime timer, tick the
// wand health loss, and fire the depletion anim; every active frame run the wand
// projectile step; finally, once arrived + idle, clear the latch + run the reset.
// @early-stop
// ~95%: whole body byte-identical (incl. the branchless max(0,hp) sub/sets/dec/and
// idiom) except cl schedules the `if (m_1c4)` load a few slots earlier than retail
// (which interleaves it among the timer zero-stores). Pure scheduling; not steerable.
RVA(0x00065a60, 0x159)
i32 CGruntBehaviorLeaf::LoadWandGruntItemConfig() {
    i32 phase = m_drawState->m_1a0.Advance(g_engineFrameDelta);
    if (phase > 0) {
        if (phase == 0x63) {
            m_downtimeLatch = 1;
            u32 downtime = g_buteMgr.GetDword(m_gruntTypeTag, "ItemDowntime");
            if (m_typeDisc == GRUNT_ROIDZ) { // Roidz: no item downtime either
                downtime = 0;
            }
            m_wandDowntimeLo = downtime;
            m_wandDowntimeHi = 0;
            m_wandTimerLo = g_frameTime;
            m_wandTimerHi = 0;
            m_460 = 0;
            m_3f0 = 0;
            if (m_1c4 != 0) {
                RefreshDecay();
            }
            if (m_gruntSubState == 0x13) {
                SetDecayTarget(m_380);
                i32 hp = m_health - g_buteMgr.GetIntDef("WANDGRUNT", "HealthLoss", 0x19);
                m_health = hp < 0 ? 0 : hp;
                if (m_health <= 0) {
                    m_260->CellDispatch(m_animArg0, m_animArg1, 1, -1);
                }
            }
        }
        m_260->LoadTileArrivalFx(m_animArg0, m_animArg1, m_3e4, m_3e8, m_gruntSubState, phase);
    }
    CAniAdvanceCursor* sub = &m_drawState->m_1a0;
    if (sub->m_28 != 0 && sub->m_20 == 0) {
        m_downtimeLatch = 0;
        InitAnimState(1, 0, 0);
    }
    return 0;
}
// ---------------------------------------------------------------------------
// CGrunt::StepEntranceRelatchB()   @0x65c20   (ret 0)
// Advance the entrance geometry (and, when it is still running, drive the tile-mgr
// I-pose load). Once the sub-player is armed-but-not-running: (re)create the HUD
// stat sprites on arrival, re-latch the "D" anim set, commit the coord toggle, then
// two tile-board reads: if the head tile carries the lose-item bit (0x2000000) play
// the lose-item anim; then resolve the tile's placed object (cell dword +8) through
// the sprite factory's key->object map and, when found, place its in-game icon at
// the grunt's owner cell; else clear the tile's object slot + the 0x40000 flag bit.
// @early-stop
// complete reconstruction (all logic + control flow); ~73% plateau. Residue is the
// deep board-cell index regalloc across the TWO tile-board reads + the cell-clear
// (the strength-reduced tx*0x1c / ty*4 running-pointer forms), the reloaded-`g`
// (0x64556c) scheduling around the lose-item call, and the reloc-masked map-lookup /
// PlaceAt / cue operands. A register-allocation/scheduling wall, not a shape error.
RVA(0x00065c20, 0x1d5)
i32 CGrunt::StepEntranceRelatchB() {
    i32 ready = m_154->m_1a0.Advance(static_cast<u32>(g_engineFrameDelta));
    if (ready > 0) {
        m_tileMgr->LoadTileArrivalFx(
            m_tileOwnerHi,
            m_tileOwnerLo,
            m_moveTileX,
            m_moveTileY,
            m_entranceReason,
            ready
        );
    }
    CAniAdvanceCursor* sub = &m_154->m_1a0;
    if (sub->m_28 == 0 || sub->m_20 != 0) {
        return 0;
    }
    m_entranceActive = 0;
    if (m_arrived != 0) {
        CreateHealthSprite();
        CreateStaminaSprite();
        CreateToySprite();
    }
    m_prevAnimSetNode = m_14->m_1c;
    m_14->m_1c = (void*)g_buteTree.Find(s_codeD);
    OnCoordCommit(m_coordToggle);
    CGameRegistry* g = g_gameReg;
    CTileGrid* grid = g->m_tileGrid;
    i32 tx = m_lastTilePxX >> 5;
    i32 ty = m_lastTilePxY >> 5;
    i32 f1;
    if (static_cast<u32>(tx) >= static_cast<u32>(grid->m_c) || static_cast<u32>(ty) >= static_cast<u32>(grid->m_10)) {
        f1 = 1;
    } else {
        f1 = ((i32*)grid->m_8[ty])[tx * 7];
    }
    if (f1 & 0x2000000) {
        BuildGruntLoseItemAnimation();
        g = g_gameReg;
    }
    grid = g->m_tileGrid;
    void* cellObj;
    if (static_cast<u32>(tx) >= static_cast<u32>(grid->m_c) || static_cast<u32>(ty) >= static_cast<u32>(grid->m_10)) {
        cellObj = 0;
    } else {
        cellObj = (void*)((i32*)grid->m_8[ty])[tx * 7 + 2];
    }
    if (cellObj == 0) {
        return 0;
    }
    CGameObject* found = 0;
    CGameObject* result = 0;
    if (g->m_world->m_childGroup->m_map48.Lookup(cellObj, (void*&)found)) {
        result = found;
    }
    if (result != 0) {
        // The cell sprite's bound logic leaf (worker m_logic) IS the in-game tile
        // icon here (the +2 grid slot only ever holds icon sprites) - the one
        // base->leaf downcast the language forces.
        CInGameIcon* icon = (CInGameIcon*)result->m_7c->m_logic;
        icon->PlaceAt(m_tileOwnerHi, m_tileOwnerLo);
        return 0;
    }
    grid = g_gameReg->m_tileGrid;
    if (static_cast<u32>(tx) < static_cast<u32>(grid->m_c) && static_cast<u32>(ty) < static_cast<u32>(grid->m_10)) {
        ((i32*)grid->m_8[ty])[tx * 7 + 2] = 0;
        ((i32*)grid->m_8[ty])[tx * 7] &= ~0x40000;
    }
    return 0;
}
