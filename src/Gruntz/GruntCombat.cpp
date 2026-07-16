#include <Mfc.h>                // the REAL MFC CPtrList - CScanList was a fake view of it
#include <Gruntz/TraitorMode.h> // g_traitorMode
// GruntCombat.cpp - the THIRD original grunt TU (retail text 0x56f80-0x5d084):
// the grunt combat / struck-voice / attack / ability-tuning / spawn family,
// carved out of the conflated Grunt.cpp (wave3-I grunt-region partition).
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
// PathScan57db0 @0x57db0 (ex GruntPathScan.cpp), LoadGruntCombatAnimations
// @0x597a0 (ex GruntCombatAnim.cpp), GruntSpawnPump @0x5baf0 (ex
// GruntSpawnPump.cpp), ConstructActRange_644af0 @0x5bc50 + RegisterActs_644af0
// @0x5be30 (ex LogicActRegistrars.cpp). NOT folded (COMDAT-at-usage emissions,
// per the TU_MIGRATION legend): ApplyGeometryDirect @0x58b60 (spriteresource),
// CMotionState::SetParams/SetZ @0x58bc0/0x58ca0 (motionstate), ??0CUserLogic
// @0x58cd0 (userlogicctoremit), CPairRecord::Serialize @0x58ee0
// (trirecordserialize), Lookup_05b7e0 @0x5b7e0 (ddrawsubmgrleafscan).
#include <Gruntz/Grunt.h>
#include <DDrawMgr/DDrawSurfaceMgr.h> // the m_0c world root (m_animRegistry hop)
#include <DDrawMgr/DDrawSubMgrLeaf.h> // m_0c->m_animRegistry (the anim-key catalog)
#include <Gruntz/GameLevel.h> // canonical CGameLevel/CLevelPlane (m_world->m_level visible rect)
#include <Gruntz/ActReg.h>    // CLookupColl/CActReg::ResolveEntry
#include <Gruntz/AniElement.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/SerialRecords.h>
#include <Gruntz/MovingLogicSerial.h>
#include <Gruntz/BoundaryLowerMethodsViews.h>
#include <Gruntz/Effect6b.h>
#include <Dsndmgr/DirectSoundMgr.h>
#include <Dsndmgr/DirectSoundMgr.h>
extern "C" WwdGameReg* g_gameReg; // 0x64556c (the WwdGameReg view, as in Grunt.cpp)
#include <rva.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <Bute/ButeMgr.h>
#include <Bute/ButeTree.h>
#include <Globals.h>
#include <Gruntz/WorkerHandler.h> // Owner/Worker + Worker_DefaultPump (GruntSpawnPump)
#include <Gruntz/ScanRectInit.h>  // the PathScan dirty-rect Set34a4 helper
#include <Gruntz/Brickz.h>        // canonical CBrickzGrid (SearchEdge)
#include <Gruntz/TypeKeyColl.h>
// WERE the fake g_animScratch / g_animScratchCount
// globals (defined in 5 TUs each; LNK2005)
#include <Gruntz/LightFx.h> // CLightFx::Activate (spell LightFx sprites; folded CSpriteRegistrar)
#include <DDrawMgr/DDrawSubMgrLeafScan.h> // CDDrawSubMgrLeafScan::Lookup_05b7e0 (rehomed here)
#include <Gruntz/GameRegistry.h> // CDDrawSurfaceMgr - the worker's m_0c owner-context facet
#include <Gruntz/LeafCue.h>      // LeafCue - the launch-sound cue entries (ex CombatCue/ConvCue)
#include <Gruntz/SoundCue.h>     // CSndHost (typedef of CDDrawSubMgrLeafScan) - the cue registry
#include <Gruntz/TriggerMgr.h>   // CTriggerMgr - the CGrunt+0x260 board (ex CombatTileMgr)
#include <Gruntz/GruntBehaviorLeaf.h> // CGruntBehaviorLeaf - 3 of the 19 act handlers (decay/wand AI leaves)
#include <new>
#pragma intrinsic(strcmp, sqrt)

// ---------------------------------------------------------------------------
// Animation-resolver cluster (the 5 CGrunt::Resolve*Animation methods, the
// SECOND wave on this CGrunt TU). Each builds an animation-key string
//   "GRUNTZ_" + this->m_typeName + "_<CATEGORY>"
// (via the two engine global operator+ overloads -> a pair of stack CString
// temporaries with dtors -> the unit needs /GX for the C++ EH frame), feeds the
// resolved geometry source into the grunt's animation player (m_38), then looks
// the key up in the global animation tree (CButeTree::Find) and
// caches the result into m_14->m_1c. Several also fire a 5-arg on-screen "cue"
// (via g_gameReg->m_cueSink) gated on the grunt being inside the visible view
// rect (registry m_134==1 -> a 4-way bounds test).
//
//   CGrunt::ResolveMovingAnimation()    ("_MOVING", key "B")
//   CGrunt::ResolveDeathAnimation()     ("_DEATH",  key "C")
//   CGrunt::ResolveAnimation()          ("_JOY",    key "E")
//   CGrunt::ResolveIdleAnimation()      ("_IDLE",   key "A")
//   CGrunt::ResolveBattlecryAnimation() ("_BATTLECRY", key "F")
//
// The category suffix strings + the single-char tree keys are literal .rodata
// (reloc-masked DIR32 operands). The geometry source member differs per category
// (Moving m_movingGeoSrc, Death m_deathGeoSrc, generic m_joyGeoSrc, Idle m_idleGeoSrc[idx], Battlecry m_battlecryGeoSrc[idx]);
// Idle/Battlecry pick idx via the engine LCG rand (Idle %3+1, Battlecry %3).
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

// Entrance-animation globals (reloc-masked; see Grunt.h).

// AUTHENTIC-FLOOR NOTE (cast audit): the casts remaining in this TU are intentional -
//   * CString-array stride access - GruntStrGetBuffer((char*)this + idx*8 + 0x4NN):
//     the per-anim CString bags at +0x468/+0x46c/+0x470/+0x000 are 8-byte-strided arrays.
//   * grid/record stride - (const char*)((zDArray*)((char*)this + (3*col+row+0xb)*0x68)),
//     ((CFocusSlot*)((char*)g + 0x150 + owner*0x238)), (double*)((char*)this + 0x4b0)
//     [0x78-stride]: raw byte arithmetic into stride records, not 2D pointer arrays.
//   * int-as-pointer pose handles - ((CAnimSetNode*)m_poseToyN)->m_10 / (void*)m_poseIdle[0]:
//     m_poseIdle/m_poseToy* are i32 handles used dually as null-compared ints and pointers.
//   * grunt freelist recycle - (void**)((char*)node - g_coordPool.m_linkOffset / g_coordPool.m_linkOffset).
//   * MFC CString -> char* - (char*)(const char*)m_animSetName for char*-taking bute APIs.
//   * tiny-method-view over this - ((CGruntUpdateThis/CVtSlot9*)this)->M() for reloc-masked
//     external __thiscall engine methods.
//   * DELIBERATELY-raw member writes - ar->Write((char*)this + 0x400/0x408/0x410, 8): the
//     m_400/408/410 doubles are modeled but kept raw because &m_400 shifts a neighbor's
//     regalloc (tested-and-reverted; see the inline m_400 note).
// numeric-conversion casts ((u32)m_dwell / (i32)m_14->m_1c / (double)...) document width and stay.
// g_buteMgr comes from <Bute/ButeMgr.h>.
static char s_TimePerTile[] = "TimePerTile";
static char s_Grunt[] = "Grunt";                               // s_Grunt_0060a9ec
static char s_EntranceSafeTime[] = "EntranceSafeTime";         // s_EntranceSafeTime_0060df98
static char s_IdleDelay[] = "IdleDelay";                       // s_IdleDelay_0060e1a0
static char s_PlayerDefenderRadius[] = "PlayerDefenderRadius"; // s_PlayerDefenderRadius_0060e1ac
static char s_CombatTimeout[] = "CombatTimeout";               // s_CombatTimeout_0060df84

// A global enable flag the neighbor-combat gate reads when the candidate IS self
// (DAT_006455b0, reloc-masked).

// The global running game clock (DAT_00645588) snapshotted into m_entranceClockLo.
extern "C" u32 g_frameTime;

// The scratch CString teardown the GetNameRecords reject paths run (defined with the
// dispatch-machine cluster below); forward-declared for the two entrance-step
// methods (StepEntranceReinit / RunEntranceMove) defined earlier in RVA order.
static void GruntScratchTeardown();

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

// ==== LoadGruntAbilityTuning @0x57100 (ex GruntAssetLoaders.cpp; its 8 private .data cells sit in this TU's band) ====
// ---------------------------------------------------------------------------
// CGrunt::LoadGruntAbilityTuning(int forced)   @0x57100   (ret 4)
// Fire the grunt's spell-ability effect for the given (or randomly-rolled)
// ability index: play the GAME_ATTACK launch sound (when the slot is armed),
// then dispatch on the index to spawn the matching LightFx flash + area tuning
// cue (FreezeRadius/HealthRadius/RessurectionRadius/ToyzRadius/TeleportRadius) or
// the 4-direction rolling-ball sprites.
//
// The ability-sound resource chain (m_158 -> m_0c -> m_28) IS the canonical facet
// ctor-proven), its m_0c the owner/world context == the CDDrawSurfaceMgr facet
// (<Gruntz/GameRegistry.h>), whose m_28 is the CSndHost cue registry
// (<Gruntz/SoundCue.h>: emit gate m_emitGate @+0x30, CMapStringToPtr map @+0x10,
// Lookup_05b7e0). The ex-CGruntSndSlot/CGruntSndRes/CGruntSndResMgr re-modeled
// those three hops; the map here really is CMapStringToPtr - retail's GAME_ATTACK
// Lookup is 0x1b8438 (the Ptr band), NOT CMapStringToOb's 0x1b8008.

// The launch-sound cue tag (reloc-masked global).
// DEFINED in GruntzMgr.cpp (owner TU); plain C++ extern here.
// (The flat `extern "C" PlayIfElapsed` alias is GONE: thunk 0x25fe IS
// ?PlayIfElapsed@LeafCue@@ - retail loads the looked-up cue into ecx and
// __thiscalls it; callers now call the real method on the cue.)

// The GAME_ATTACK sound cue tag (const char*). The other spell-effect key
// strings (CreateSprite/Activate/ApplyName args) are spelled as inline literals
// below so cl pools them into the shared read-only ??_C@ constants retail uses.
static const char s_GAME_ATTACK[] = "GAME_ATTACK";
// The bute config tag/keys (GetIntDef/GetDwordDef take char*).
static char s_Spellz[] = "Spellz";
static char s_FreezeRadius[] = "FreezeRadius";
static char s_HealthRadius[] = "HealthRadius";
static char s_RessurectionRadius[] = "RessurectionRadius";
static char s_ToyzRadius[] = "ToyzRadius";
static char s_TeleportRadius[] = "TeleportRadius";
static char s_RollingBallzSpeed[] = "RollingBallzSpeed";
static char s_RollingBallzTime[] = "RollingBallzTime";

// The random grunt spell/ability effect LoadGruntAbilityTuning dispatches on (idx);
// each name is confirmed by its case comment + its "Spellz" bute radius key. Same
// immediates as the bare labels -> naming is matching-neutral.
enum SpellzEffect {
    SPELLZ_FREEZE = 1,       // FreezeRadius
    SPELLZ_HEALTH = 2,       // HealthRadius
    SPELLZ_RESURRECTION = 3, // RessurectionRadius
    SPELLZ_TOYZ = 4,         // ToyzRadius
    SPELLZ_TELEPORT = 5,     // TeleportRadius
    SPELLZ_ROLLINGBALL = 6,  // RollingBallzSpeed/Time (spawns 4 directional ballz)
};

// ==== LoadGruntCombatAnimations @0x597a0 (ex GruntCombatAnim.cpp; its 15 private .data cells + the
// i324-i332 frag run sit in this TU) ====

// (The P offset-cast macro is GONE, 2026-07-16: its two residual reaches are typed -
// +0x260 IS Grunt.h's m_tileMgr, now canonically CTriggerMgr* (the CGruntTileMgr
// GetHeadPosition() + the shared CoordNode (<Gruntz/CoordNode.h>).)

// (CombatCue is GONE - it was LeafCue (<Gruntz/LeafCue.h>): m_10 ConfigureItem
//  owner, m_14 last-fire clock, m_18 cooldown - and its throttled fire is the real
//  ?PlayIfElapsed@LeafCue@@. CombatSprInner/CombatSprCat are GONE - the
//  world holder + cue host are the canonical CDDrawSurfaceMgr / CSndHost, and
//  the launch-sound map is CMapStringToPtr: every LK Lookup in 0x597a0 calls
//  0x1b8438 (the Ptr band) - the old `CMapStringToOb m_10` bound the WRONG library
//  body, 0x1b8008.)

// (the CombatGrid view is GONE - it was a THIRD name in this one TU for the board at
// g_gameReg+0x70, alongside CScanPlane and the GruntBoard the canonical g_gameReg already
// hands back. Its row table at +0x08 and its dims at +0x0c/+0x10 are CMapMgr's m_8 /
// m_width / m_height, and its 7-dword cell IS BrickzCell. The board is the real RTTI class
// CGruntzMapMgr : CMapMgr (vtbl 0x1e9bb4) - <Gruntz/GruntzMgr.h> proves it from Close()'s
// +0x70 teardown leg - so the pointer is typed with the canonical CBrickzGrid (== CMapMgr)
// and every field lands on the real member.)

// (CombatReg is GONE - g_gameReg's own WwdGameReg canonical carries every field
//  the combat paths read: m_isEasyMode/m_134/m_viewOrigin* (the view-edge quad was
//  merged into <Gruntz/WwdGameReg.h>) and the +0x70 board, reached as the
//  canonical CBrickzGrid via a facet cast (GruntBoard == CBrickzGrid == CMapMgr,
//  a pending unification).)

// (CombatTileMgr is GONE - the CGrunt+0x260 "tile-mgr grunt board" is the real
//  CTriggerMgr (<Gruntz/TriggerMgr.h>): its m_grid 4x15 grid at +0x1c, and its
//  three "engine ops" were phantom aliases of already-reconstructed CTriggerMgr
//  methods, read off the thunks: `CheckSpawn` (ILT 0x14a1) == ?SpawnGrunt@
//  CTriggerMgr@@ @0x7c110, `ApplyCellEffect` (ILT 0x2e96) == ?CellDispatch@
//  CTriggerMgr@@ @0x6bcb0, `ApplySwitch` (ILT 0x26df) == ?ApplySwitch@CTriggerMgr@@
//  @0x6d300 - the sites call the real names.)

// (CombatConvCue/CombatConvLookup are GONE - retail loads the m_158->m_0c->m_28
//  cue host into ecx and __thiscalls 0x2cca == ?Lookup_05b7e0@CDDrawSubMgrLeafScan@@
//  (the CSndHost, defined below in this very TU), then __thiscalls the looked-up
//  LeafCue's PlayIfElapsed. The "__cdecl lookup" was a latent-ecx flat alias.)

// The active-anim-set type-name registry: ((_zvec*)&g_typeColl)->IndexToPtr(node) -> record whose
// first field is the name string; g_typeColl.m_alloc[0..g_typeColl.m_grown) each get Reset.
// The bute-config manager (canonical CButeMgr, g_buteMgr from <Bute/ButeMgr.h>):
// GetDwordDef (0x1721e0) is reloc-masked __thiscall.

// (CombatCoordList is GONE - the CGrunt+0x31c occupied-coord recycle list is the
//  REAL MFC CPtrList member m_31c (Grunt.h); its AddHead/RemoveAll @0x1b4967/
//  0x1b48a6 are the library CPtrList bodies, now bound by name.)
// g_coordPool.m_freeHead / g_coordPool.m_linkOffset are declared above (PathScan section).

// The kill-clock + sound-enable + cue-tag globals.
extern "C" i32 g_killCueClock; // _g_killCueClock @0x6bf3c0
// C++ linkage: ?g_sndEnabled@@3HA is now the ONE name bound at 0x61ab20 (defined in
// GruntzMgr.cpp); the old extern "C" spelling emitted _g_sndEnabled, a second name for
// the same storage.
// g_sndCueTag is declared above (LoadGruntAbilityTuning section).

// The 8 octant direction-vector triples (16-byte stride) copied into CGrunt+0x43c.
extern "C" i32 g_dirVec[9][4]; // DAT_00644970

// The hit-type byte table [reason][a0] (row stride 23) + the octant tangent thresholds.
extern "C" unsigned char g_hitTable[]; // DAT_005e9788
extern "C" float g_dtScale;            // DAT_005e999c death-touch duration scale
extern "C" float g_tanC0;              // DAT_005e99a0
extern "C" float g_tanC1;              // DAT_005e99a4
extern "C" double g_tanC2;             // DAT_005e99a8
extern "C" double g_tanC3;             // DAT_005e99b0

// The impact/block sound-cue keys (literal .rodata; reloc-masked).
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

// Resolve a launch-sound cue by key into a fresh out slot (the same cast-free
// facet chain EnsureStruckSlot/EnsureStruckVoice walk: world holder -> cue host ->
// its CMapStringToPtr; retail Lookup 0x1b8438).
#define LK(key)                                                                                    \
    do {                                                                                           \
        LeafCue* out = 0;                                                                          \
        g_gameReg->m_world->m_soundRegistry->m_10.Lookup((key), (void*&)out);                      \
        cue = out;                                                                                 \
    } while (0)

// Copy octant direction-vector triple k into CGrunt+0x43c; set the target tile pixel.
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

// ==== PathScan57db0 @0x57db0 (ex GruntPathScan.cpp) ====
// (g_gameReg is the WwdGameReg* view declared at the top of this TU; byte-view uses cast it)

#include <Gruntz/FreeNodePool.h> // the coord-node pool object @0x645540
// The pool's INTERIOR FIELDS - m_freeHead (+0x04) and m_linkOffset (+0x0c) - used to be
// declared here as the standalone globals g_coordPool.m_freeHead / g_coordPool.m_linkOffset. They are not
// globals: they are fields of g_coordPool (DEFINED in src/Gruntz/GameText.cpp), which is
// why the free-list push/pop code reads exactly [pool+4] and [pool+0xc].

// --- local grid view (the tile-plane's dirty-rect view is richer than GruntBoard) ---
// The scratch list is the REAL MFC CPtrList (a stack instance forces the /GX EH frame).
// It was a fake `CScanList` view whose methods were declared-only -> 6 PHANTOM externals.
// The FID library tables name every one of its RVAs with HIGH, reloc-anchored confidence:
//   0x1b4867 ??0CPtrList@@QAE@H@Z          0x1b48c6 ??1CPtrList@@UAE@XZ
//   0x1b48a6 ?RemoveAll@CPtrList@@QAEXXZ   0x1b4991 ?AddTail@CPtrList@@...
//   0x1b4a03 ?RemoveHead@CPtrList@@QAEPAXXZ
// (The CObList rows at those same RVAs are AMBIG - CObList/CPtrList are COMDAT-identical.
//  CPtrList wins on two counts: the HIGH/reloc-anchored FID rows, and the element type -
//  every element stored here is a raw void*/GruntCoord*, which is CPtrList's element type;
//  CObList would force (CObject*) casts that this code does not have.)
// Find1de8 was never a method at all: 0x1de8 is an ILT thunk to 0x29a30, the free __stdcall
// ListNodeAdvance(void**) already defined in BattlezMapConfig.cpp.
void* __stdcall ListNodeAdvance(void** pos); // 0x29a30 (thunk 0x1de8)
// (the CScanPlane + CScanCell views and the ApiMisc::ClipHost_02b340 shell are all GONE -
// XREF-settled, and they were one object. The board at g_gameReg+0x70 IS the CBrickzGrid /
// CMapMgr grid: its +0x08 row table, +0x0c/+0x10 dims and +0x60..+0x74 bound-rect/extents are
// CMapMgr's field for field, its 0x1c-byte cell IS BrickzCell, and the code already had to
// CAST it - `((CBrickzGrid*)grid)->SearchEdge(...)` - which was the type telling the truth.
// The two "ApiMisc" entry points were never free functions: 0x20f4 thunks to
// CBrickzGrid::SearchEdge and 0x43ea to ?Clip@CMapMgr@@QAEXPBUtagRECT@@@Z (0x2b340,
// reconstructed in src/Gruntz/BrickzClip_02b340.cpp). Typing the pointer with the real class
// dropped the views, the shell and the cast together.)

// Recompute the plane dirty rect (m_60) as {0,0,w,h} intersected with a copy.
#define SCAN_BOUNDS(grid)                                                                          \
    {                                                                                              \
        RECT ra;                                                                                   \
        RECT rb;                                                                                   \
        ((CScanRectInit*)&ra)->Set34a4(0, 0, (grid)->m_width, (grid)->m_height);                   \
        RECT* pb = ((CScanRectInit*)&rb)->Set34a4(0, 0, (grid)->m_width, (grid)->m_height);        \
        ra.left = pb->left;                                                                        \
        ra.top = pb->top;                                                                          \
        ra.right = pb->right;                                                                      \
        ra.bottom = pb->bottom;                                                                    \
        if (!IntersectRect((RECT*)&(grid)->m_originX, &ra, &rb)) {                                 \
            *(RECT*)&(grid)->m_originX = ra;                                                       \
        }                                                                                          \
        (grid)->m_gridW = (grid)->m_boundRight - (grid)->m_originX;                                \
        (grid)->m_gridH = (grid)->m_boundBottom - (grid)->m_originY;                               \
    }

#define FREELIST_PUSH(elem)                                                                        \
    {                                                                                              \
        void** node = (void**)((char*)(elem) - g_coordPool.m_linkOffset);                          \
        *node = g_coordPool.m_freeHead;                                                            \
        g_coordPool.m_freeHead = node;                                                             \
    }

// @early-stop
// FP instruction-scheduling wall: logic + offsets exact, but MSVC's x87 stack
// scheduling for the magnitude/normalize rarely matches from C source (an x87
// fxch/fsubp ordering the compiler picks); see docs/patterns (FP scheduling).
// CGrunt::ComputeFacing(double dt) @0x57060 - sets the grunt's facing/velocity:
//   m_400 = (sqrt(dx*dx + dy*dy) / m_41c) * dt   (dx=m_lastTilePxX-m_5c, dy=m_lastTilePxY-m_60)
//   m_408 = (double)m_10->m_5c;  m_410 = (double)m_10->m_60
// dt is the per-tile time step; m_41c is the configured TimePerTile.
RVA(0x00057060, 0x6f)
void CGrunt::ComputeFacing(double dt) {
    CGameObject* h = m_10;
    double dx = (double)m_lastTilePxX - (double)h->m_screenX;
    double dy = (double)m_lastTilePxY - (double)h->m_screenY;
    m_400 = (sqrt(dx * dx + dy * dy) / (double)m_timePerTile) * dt;
    m_408 = (double)h->m_screenX;
    m_410 = (double)h->m_screenY;
}

SIZE_UNKNOWN(CombatItemOwner);
SIZE_UNKNOWN(CombatTypeNode);

// One unrolled per-key registration block (the shared archetype). A macro so all
// 19 expansions are emitted literally inline (an inline helper exhausts MSVC's
// per-function inline budget after ~6 sites and calls the rest, but retail unrolls
// every block); the lookups outline to the shared 0x3864 helper while the node-free
// loop stays inline.
#define REGISTER_KEY_644AF0(key, handler)                                                          \
    {                                                                                              \
        i32 id = (i32)g_buteTree.Find(key);                                                        \
        if (id == 0) {                                                                             \
            g_buteTree.Insert(key, (void*)g_typeCounter);                                          \
            id = g_typeCounter;                                                                    \
            char* slot = (char*)((_zvec*)&g_typeColl)->IndexToPtr(id);                             \
            i32 n = g_typeColl.m_grown;                                                            \
            void** list = (void**)g_typeColl.m_alloc;                                              \
            while (n-- != 0) {                                                                     \
                if (list != 0) {                                                                   \
                    ((CString*)list)->CString::~CString();                                         \
                }                                                                                  \
                list++;                                                                            \
            }                                                                                      \
            ((CString*)slot)->operator=(key);                                                      \
            g_typeCounter++;                                                                       \
        }                                                                                          \
        ((CGruntActEntry*)(char*)((_zvec*)&g_reg_644af0)->IndexToPtr(id))->m_fn =                  \
            (GruntActHandler)(handler);                                                            \
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
    CSndHost* slot = ((CDDrawSurfaceMgr*)m_158->m_0c)->m_soundRegistry;
    if (slot->m_emitGate == 0) {
        LeafCue* sout = 0;
        slot->m_10.Lookup(s_GAME_ATTACK, (void*&)sout); // CMapStringToPtr @0x1b8438
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
            ((CLightFx*)spr->m_7c->m_logic)
                ->Activate((i32) "GAME_LIGHTING_FLASH", (i32) "GAME_FLASH", 9, 1);
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
            ((CLightFx*)spr->m_7c->m_logic)
                ->Activate((i32) "GAME_LIGHTING_FLASH", (i32) "GAME_FLASH", 2, 1);
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
            ((CLightFx*)spr->m_7c->m_logic)
                ->Activate((i32) "GAME_LIGHTING_FLASH", (i32) "GAME_FLASH", 8, 1);
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
            ((CLightFx*)spr->m_7c->m_logic)
                ->Activate((i32) "GAME_LIGHTING_FLASH", (i32) "GAME_FLASH", 7, 1);
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
            ((CLightFx*)spr->m_7c->m_logic)
                ->Activate((i32) "GAME_LIGHTING_FLASH", (i32) "GAME_FLASH", 3, 1);
            return m_tileMgr->CombatCue(
                m_lastTilePxX,
                m_lastTilePxY,
                g_buteMgr.GetIntDef(s_Spellz, s_TeleportRadius, 8),
                2,
                -1
            );
        }
        case SPELLZ_ROLLINGBALL: { // rolling ball (4 directions)
            CGameObject* n = g_gameReg->m_world->m_childGroup->CreateSprite(
                0,
                m_lastTilePxX,
                m_lastTilePxY - 0x20,
                0,
                "RollingBall",
                0x40003
            );
            n->ApplyName("LEVEL_ROLLINGBALL_NORTH");
            AnimWorkerObj* ni = n->m_7c;
            ni->m_bc = (i32)g_buteMgr.GetDwordDef(s_Spellz, s_RollingBallzSpeed, 0x3e8);
            n->m_124 = 0;
            n->m_118 = (i32)g_buteMgr.GetDwordDef(s_Spellz, s_RollingBallzTime, 0x3e8);

            CGameObject* e = g_gameReg->m_world->m_childGroup->CreateSprite(
                0,
                m_lastTilePxX + 0x20,
                m_lastTilePxY,
                0,
                "RollingBall",
                0x40003
            );
            e->ApplyName("LEVEL_ROLLINGBALL_EAST");
            AnimWorkerObj* ei = e->m_7c;
            ei->m_bc = (i32)g_buteMgr.GetDwordDef(s_Spellz, s_RollingBallzSpeed, 0x3e8);
            e->m_124 = 0;
            e->m_118 = (i32)g_buteMgr.GetDwordDef(s_Spellz, s_RollingBallzTime, 0x3e8);

            CGameObject* s = g_gameReg->m_world->m_childGroup->CreateSprite(
                0,
                m_lastTilePxX,
                m_lastTilePxY + 0x20,
                0,
                "RollingBall",
                0x40003
            );
            s->ApplyName("LEVEL_ROLLINGBALL_SOUTH");
            AnimWorkerObj* si = s->m_7c;
            si->m_bc = (i32)g_buteMgr.GetDwordDef(s_Spellz, s_RollingBallzSpeed, 0x3e8);
            s->m_124 = 0;
            s->m_118 = (i32)g_buteMgr.GetDwordDef(s_Spellz, s_RollingBallzTime, 0x3e8);

            CGameObject* w = g_gameReg->m_world->m_childGroup->CreateSprite(
                0,
                m_lastTilePxX - 0x20,
                m_lastTilePxY,
                0,
                "RollingBall",
                0x40003
            );
            w->ApplyName("LEVEL_ROLLINGBALL_WEST");
            AnimWorkerObj* wi = w->m_7c;
            wi->m_bc = (i32)g_buteMgr.GetDwordDef(s_Spellz, s_RollingBallzSpeed, 0x3e8);
            w->m_124 = 0;
            w->m_118 = (i32)g_buteMgr.GetDwordDef(s_Spellz, s_RollingBallzTime, 0x3e8);
            return 1;
        }
        default:
            return 0;
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
extern "C" WwdGameReg* g_gameReg; // ?g_gameReg@@3PAUWwdGameReg@@A @0x64556c
                                  // src/Gruntz/GameText.cpp (the pool's owner TU).
                                  // It used to be DEFINED here too: six .cpp files each
                                  // defined it, i.e. six .bss objects for one global
                                  // (LNK2005). Only the owner defines; everyone externs.

// The single-letter anim type-code literals live ONCE in retail .rdata and are shared by
// every TU that compares against them (s_codeA..s_codeQ, declared in <Gruntz/Grunt.h>,
// DATA-bound in src/Globals.cpp). They used to be re-DEFINED here - 14 external symbols
// duplicated across 5 objs = a duplicate-symbol link defect.

// ---------------------------------------------------------------------------
// CGrunt::SelectMoveIcon(a)   @0x57800   (__thiscall, ret 4)
// Update the move-cursor icon index (m_1f4_moveIcon). No-op if unchanged; else
// clamp to [0, 0x11), resolve the matching sprite from the registry sprite-ref
// table (g_gameReg->m_74->GetSel(icon, reason>=0x17)) and stamp it onto the HUD
// (m_10->m_4c = sprite, m_50 = 0xa, m_58 = 1).
RVA(0x00057800, 0x64)
void CGrunt::SelectMoveIcon(i32 a) {
    if (m_1f4_moveIcon == a) {
        return;
    }
    m_1f4_moveIcon = a;
    if (a < 0 || a >= 0x11) {
        m_1f4_moveIcon = 0;
    }
    i32 sel = g_gameReg->m_74->GetSel(m_1f4_moveIcon, m_entranceReason >= 0x17);
    CGameObject* h = m_10;
    h->m_drawActive = 1;
    h->m_drawFillCmd = 0xa;
    h->m_drawFillArg = sel;
}

// ---------------------------------------------------------------------------
// CGrunt::BuildGruntLoseItemAnimation()   @0x57890   (__thiscall, ret 0, /GX)
// Step the anim dispatch, then if the entrance reason is a lose-item pose
// (0x12/0x16/0xe) spawn the one-shot "SingleAnimation" sprite, set its
// GRUNTZ_<animSet>_LOSEITEM key (both ApplyName + ApplyLookupGeometry forms),
// fire the on-screen spawn cue when the grunt's HUD point is in the cue rect,
// re-run the type-table step, and clear m_entranceActive.
//
// 99.9% (reloc-mask tail): code bytes match incl. the /GX frame + the two
// `s_GRUNTZ_ + m_animSetName + s__LOSEITEM` concat chains; the residual is the
// differently-named reloc operands (ApplyName/ApplyLookupGeometry/LoadGruntTypeTable
// resolve to the engine's own CGameObject/CUserLogic symbols) - the entropy tail.
RVA(0x00057890, 0x19c)
i32 CGrunt::BuildGruntLoseItemAnimation() {
    StepAnimDispatchB();
    i32 reason = m_entranceReason;
    if (reason != 0x12 && reason != 0x16 && reason != 0xe) {
        return 0;
    }

    CGameObject* spr = g_gameReg->m_world->m_childGroup->CreateSprite(
        0,
        m_10->m_screenX,
        m_10->m_screenY,
        0xcf850,
        s_SingleAnimation,
        0x40003
    );
    spr->ApplyName(s_GRUNTZ_ + m_animSetName + s__LOSEITEM);
    spr->ApplyLookupGeometry(s_GRUNTZ_ + m_animSetName + s__LOSEITEM, 0);

    CGameRegistry* g = (CGameRegistry*)g_gameReg;
    i32 x = m_10->m_screenX;
    i32 y = m_10->m_screenY;
    CCueRect* rc = (CCueRect*)&g->m_world->m_level->m_mainPlane->m_originX;
    if (x < rc->right && x >= rc->left && y < rc->bottom && y >= rc->top) {
        g->m_cueSink->CueSpawn(this, 0xe, -1, -1, -1);
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
// byte-exact. Residue = cl loads board->m_c (`mov ecx,[ebx+0xc]`) one slot earlier
// than retail (retail defers it past `add eax,0x10`) and reads g_gameReg before m_10
// vs retail's m_10-first; the rest is identical. ~93%. Final sweep.
RVA(0x00057aa0, 0x9b)
i32 CGrunt::TryPowerupAtTile() {
    i32 reason = m_entranceReason;
    if (reason <= 0 || reason >= 0x17) {
        return 0;
    }
    CGameObject* h = m_10;
    i32 mx = h->m_screenX;
    i32 my = h->m_screenY;
    GruntBoard* b = g_gameReg->m_tileGrid;
    i32 px = (mx & ~0x1f) + 0x10;
    i32 py = (my & ~0x1f) + 0x10;
    i32 tx = px >> 5;
    i32 ty = py >> 5;
    i32 flags;
    if ((u32)tx >= (u32)b->m_c || (u32)ty >= (u32)b->m_10) {
        flags = 1;
    } else {
        flags = ((i32*)b->m_8[ty])[tx * 7];
    }
    if ((flags & 0x939) || (flags & 2)) {
        return 0;
    }
    m_tileMgr->LoadPowerupIconSprites(reason, px, py, 0, 1, 0);
    return 1;
}

// CGrunt::EnsureStruckSlot(key) @0x57b70 - the +0x424-slot sibling of
// EnsureStruckVoice (+0x428): lazily build + play a grunt sound sample for `key`.
// Bails if the +0x424 slot is already filled, or if the registry's m_10 gate is
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
    DirectSoundMgr*& sample = m_424;
    if (sample != 0) {
        return;
    }
    if (*(i32*)((char*)g_gameReg + 0x10) == 0) {
        return;
    }
    void* entry_ob = 0;
    g_gameReg->m_world->m_soundRegistry->m_10.Lookup(key, entry_ob); // CMapStringToPtr (void*& out)
    GruntSoundEntry* entry = (GruntSoundEntry*)entry_ob;
    if (entry == 0) {
        return;
    }
    if (entry->m_10 == 0) {
        return;
    }
    sample = (DirectSoundMgr*)entry->m_10->GetItem();
    if (sample == 0) {
        return;
    }
    sample->ApplyAndPlay(g_gameReg->m_11c, 0, 0, 1);
}

// CGrunt::ClearSubA() @0x57c10 - destroy the optional sub-object at +0x424.
RVA(0x00057c10, 0x1e)
void CGrunt::ClearSubA() {
    DirectSoundMgr* p = m_424;
    if (p) {
        p->StopAndRewind();
        m_424 = 0;
    }
}

// CGrunt::EnsureStruckVoice(key) @0x57c40 - lazily build + play the grunt's
// struck-voice sound sample. Bails if already created (the +0x428 slot ClearSubB
// frees). Looks `key` up in the global sound table (g_gameReg->m_world->m_soundRegistry->m_10),
// clones a sample from the entry's factory (GetItem), stores it into +0x428, and
// plays it on the sound channel (g_gameReg->m_11c). __thiscall, ret 4. Same
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
    DirectSoundMgr*& sample = m_428;
    if (sample != 0) {
        return;
    }
    void* entry_ob = 0;
    g_gameReg->m_world->m_soundRegistry->m_10.Lookup(key, entry_ob); // CMapStringToPtr (void*& out)
    GruntSoundEntry* entry = (GruntSoundEntry*)entry_ob;
    if (entry == 0) {
        return;
    }
    if (entry->m_10 == 0) {
        return;
    }
    sample = (DirectSoundMgr*)entry->m_10->GetItem();
    if (sample == 0) {
        return;
    }
    sample->ApplyAndPlay(g_gameReg->m_11c, 0, 0, 1);
}

// CGrunt::ClearSubB() @0x57ce0 - destroy the optional sub-object at +0x428.
RVA(0x00057ce0, 0x1e)
void CGrunt::ClearSubB() {
    DirectSoundMgr* p = m_428;
    if (p) {
        p->StopAndRewind();
        m_428 = 0;
    }
}

// CGrunt::ReapplyVoiceParams() @0x57d10 - when the registry sound gate
// (g_gameReg->m_10) is set, re-apply the current sound-channel params
// (g_gameReg->m_11c) to both the struck-slot (+0x424) and struck-voice (+0x428)
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
    if (*(i32*)((char*)g_gameReg + 0x10) == 0) {
        return;
    }
    DirectSoundMgr* a = m_424;
    if (a != 0) {
        a->ApplyAndPlay(g_gameReg->m_11c, 0, 0, 1);
    }
    DirectSoundMgr* b = m_428;
    if (b != 0) {
        b->ApplyAndPlay(g_gameReg->m_11c, 0, 0, 1);
    }
}

// CGrunt::DestroyAnims() @0x57d80 - the two-step anim teardown (both this-calls
// reach engine cleanup; reloc-masked).
RVA(0x00057d80, 0x11)
void CGrunt::DestroyAnims() {
    ClearSubA();
    ClearSubB();
}

// ==== ConstructActRange_644af0 @0x5bc50 + RegisterActs_644af0 @0x5be30 (ex
// LogicActRegistrars.cpp; the i342 frag + both bodies are text-contained in this
// TU, between GruntSpawnPump and RunAct/Activate) ====

// The shared activation-name registry pieces (same shape <Gruntz/ActNameRegistry.h>
// models; declared bare here because this TU's CGrunt world already carries its own
// CString/bute decls). All reloc-masked.
// (g_typeColl.m_grown @0x6bf670 / g_typeColl.m_alloc @0x6bf66c declared canonically above)
DATA(0x00244af0)
CLookupColl g_reg_644af0;

// The 19 action-key strings (s_codeA/B above; the rest are .rdata string
// constants named by address, declared in <Globals.h> or here).
// k_60cca4 was a SECOND NAME for s_codeD (0x20cca4) - same address,
// so nothing ever defined it. Unified onto the canonical.
extern char s_codeD[];
// k_60d2ec was a SECOND NAME for s_codeE (0x20d2ec) - same address,
// so nothing ever defined it. Unified onto the canonical.
extern char s_codeE[];
// k_60cc94 was a SECOND NAME for s_codeJ (0x20cc94) - same address,
// so nothing ever defined it. Unified onto the canonical.
extern char s_codeJ[];
// k_60d7f8 was a SECOND NAME for s_codeK (0x20d7f8) - same address,
// so nothing ever defined it. Unified onto the canonical.
extern char s_codeK[];

// The 19 per-action handler bodies are the CGrunt / CGruntBehaviorLeaf entrance /
// arrival / decay state-step methods, already reconstructed in GruntEntranceArrival.cpp
// / GruntEntranceMove.cpp / GruntBehaviorLeaf.cpp. Each retail `&H_<va>` reference was
// the function's ILT jmp-thunk (e.g. H_402ac2 @0x402ac2 -> jmp 0x4633e0 ==
// ?ResolveEntranceArrival@CGrunt@@); RegisterActs stores a 4-byte member-function
// pointer to the real body into the entry's m_fn PMF slot (thunk-following makes the
// reloc bind to the real symbol). Mapping (act key -> real handler, RVA):
//   A(s_codeA)   ResolveEntranceArrival        0x633e0 (CGrunt)
//   B(s_actKeyB) StepWarpExit                  0x64540 (CGrunt)
//   C(k_60cc90)  LoadGruntDecayConfig          0x612a0 (CGruntBehaviorLeaf)
//   D(s_codeD)   StepArrivalReroll             0x63b60 (CGrunt)
//   E(s_codeE)   UpdateGruntStatus             0x617c0 (CGrunt)
//   F(s_codeF)   DispatchVtbl24                0x6b260 (CGrunt)
//   G(s_codeG)   StepEntranceRelatchA          0x62840 (CGrunt)
//   H(s_codeH)   StepArrivalCommitA            0x65300 (CGrunt)
//   I(s_codeI)   LoadWandGruntItemConfig       0x65a60 (CGruntBehaviorLeaf)
//   J(s_codeJ)   RunEntranceMove               0x67850 (CGrunt)
//   K(s_codeK)   LoadEntranceConfig            0x67f80 (CGrunt)
//   L(s_codeL)   LoadVehicleGruntAnimations    0x63db0 (CGrunt)
//   M(s_codeM)   RearmEntranceDrop             0x68370 (CGrunt)
//   N(s_codeN)   StepEntranceRelatchB          0x65c20 (CGrunt)
//   O(s_codeO)   StepArrivalCommitB            0x654b0 (CGrunt)
//   P(s_codeP)   UpdateEntranceAnim            0x690a0 (CGrunt)
//   Q(s_codeQ)   LoadFreezeSpellAssets         0x69d60 (CGrunt)
//   (k_60bebc)   LoadGruntDecayConfig2         0x61570 (CGruntBehaviorLeaf)
//   (k_60df94)   FinishEntranceMove            0x69fd0 (CGrunt)

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
i32 CGrunt::PathScan57db0() {
    CBrickzGrid* grid = *(CBrickzGrid**)((char*)g_gameReg + 0x70);
    if (CoordCount() == 0) {
        return 1;
    }
    GruntCoordNode* node = CoordHead();

    i32 col5 = m_10->m_screenX >> 5;
    i32 row5 = m_10->m_screenY >> 5;
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
    if (!IntersectRect((RECT*)&grid->m_originX, &box, &gb)) {
        *(RECT*)&grid->m_originX = box;
    }
    grid->m_gridW = grid->m_boundRight - grid->m_originX;
    grid->m_gridH = grid->m_boundBottom - grid->m_originY;

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
                void** fn = (void**)g_coordPool.m_freeHead;
                fn[0] = (void*)co->m_x;
                fn[1] = (void*)co->m_y;
                g_coordPool.m_freeHead = *fn;
                m_31c.AddTail(fn);
            }
        }
        if (CoordCount() != 0) {
            GruntCoordNode* nd = CoordHead();
            while (nd != 0) {
                void* r = ListNodeAdvance((void**)&nd);
                if (*(i32*)r != 0) {
                    g_coordPool.Push((void*)*(i32*)r);
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
                if ((u32)rr < (u32)grid->m_height && (u32)cc < (u32)grid->m_width) {
                    cf = ((i32*)grid->m_8[rr])[cc];
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
    m_struckClockLo = (i32)g_frameTime;
    m_struckClockHi = 0;
    i32 c = ++m_struckCount;

    if (wasHit == 0) {
        if (m_gruntKind == 0x36) {
            return;
        }
        i32 x = m_10->m_screenX;
        i32 y = m_10->m_screenY;
        if (c < 5) {
            CGameRegistry* g = (CGameRegistry*)g_gameReg;
            i32* vr = &g->m_world->m_level->m_mainPlane->m_originX;
            if (x < vr[2] && x >= vr[0] && y < vr[3] && y >= vr[1]) {
                g->m_cueSink->CueA(this, 0x370, -1, 0, -1, -1);
            }
            return;
        }
        CGameRegistry* g = (CGameRegistry*)g_gameReg;
        i32* vr = &g->m_world->m_level->m_mainPlane->m_originX;
        if (x < vr[2] && x >= vr[0] && y < vr[3] && y >= vr[1]) {
            g->m_cueSink->CueA(this, 0x371, -1, 0, -1, -1);
        } else {
            m_struckCount = 0;
        }
        return;
    }

    if (c < 5) {
        i32 x = m_10->m_screenX;
        i32 y = m_10->m_screenY;
        CGameRegistry* g = (CGameRegistry*)g_gameReg;
        i32* vr = &g->m_world->m_level->m_mainPlane->m_originX;
        if (x < vr[2] && x >= vr[0] && y < vr[3] && y >= vr[1]) {
            g->m_cueSink->CueA(this, 0x320, -1, 0, -1, -1);
        }
        return;
    }
    if (c < 0xa) {
        i32 x = m_10->m_screenX;
        i32 y = m_10->m_screenY;
        CGameRegistry* g = (CGameRegistry*)g_gameReg;
        i32* vr = &g->m_world->m_level->m_mainPlane->m_originX;
        if (x < vr[2] && x >= vr[0] && y < vr[3] && y >= vr[1]) {
            g->m_cueSink->CueA(this, 0x321, -1, 0, -1, -1);
        }
        return;
    }
    {
        i32 x = m_10->m_screenX;
        i32 y = m_10->m_screenY;
        m_struckCount = 0;
        CGameRegistry* g = (CGameRegistry*)g_gameReg;
        i32* vr = &g->m_world->m_level->m_mainPlane->m_originX;
        if (x < vr[2] && x >= vr[0] && y < vr[3] && y >= vr[1]) {
            g->m_cueSink->CueA(this, 0x322, -1, 0, -1, -1);
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
                CGameObject* inner = occ->m_10;
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
    char* nm0 = *g_typeColl.GetNameRecord(m_14->m_1c);
    if (strcmp(nm0, s_codeH) == 0) {
        return 1;
    }
    {
        i32 coord = (i32)m_14->m_1c;
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
        (void)rec;
    }
    char* nm1 = *g_typeColl.GetNameRecord(m_14->m_1c);
    if (strcmp(nm1, s_codeF) == 0) {
        return 1;
    }
    {
        i32 coord = (i32)m_14->m_1c;
        g_typeColl.m_grown = 0;
        i32 rec;
        if (coord < g_cellLo || coord > g_cellHi) {
            if (g_typeColl.MapCellIndex(coord, 0) != 0) {
                rec = (coord - g_cellLo) * g_cellScale + g_cellBase;
            } else {
                i32 pin = g_typeColl.PinCellIndex();
                g_cellRecordRet = g_typeColl.MapCellRecord2(g_cellRecordBase, 0xc);
                rec = g_cellRet;
                (void)pin;
            }
        } else {
            rec = (coord - g_cellLo) * g_cellScale + g_cellBase;
        }
        GruntScratchTeardown();
        (void)rec;
    }
    char* nm2 = *g_typeColl.GetNameRecord(m_14->m_1c);
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
            CSndHost* host = ((CDDrawSurfaceMgr*)m_158->m_0c)->m_soundRegistry;
            if (host->m_emitGate == 0) {
                LeafCue* cc = (LeafCue*)host->Lookup_05b7e0(s_CONVERSIONHIT);
                if (cc != 0) {
                    cc->PlayIfElapsed(g_sndCueTag, 0, 0, 0);
                }
            }
            return 0;
        }
    }

    // Hit-type byte-table lookup + optional handicap halving.
    i32 hit = g_hitTable[this->m_entranceReason * 23 + a0];
    WwdGameReg* reg = g_gameReg; // cached once (retail keeps the singleton in a reg)
    if (reg->m_isEasyMode != 0 && reg->m_134 == 1 && this->m_tileOwnerHi == g_curPlayer) {
        i32 t = hit / 2;
        hit = t + t % 5;
    }

    // Duration scale (kind 0x3c death-touch): scale by g_dtScale, then damage the enemy.
    if (a7 == 0x3a) {
        hit = 0x64;
    } else if (this->m_gruntKind == 0x3c) {
        hit = (i32)((float)hit * g_dtScale);
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
            if ((u32)(clk - cue->m_14) >= (u32)cue->m_18) {
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
    char** typeRec = (char**)((_zvec*)&g_typeColl)->IndexToPtr((i32)(this->m_14->m_1c));
    if (g_typeColl.m_grown != 0) {
        char* p = (char*)g_typeColl.m_alloc;
        i32 n = g_typeColl.m_grown;
        do {
            if (p != 0) {
                new ((void*)p) CString();
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
        float slope = (float)dy / dx;
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
        CBrickzGrid* grid = g_gameReg->m_tileGrid; // GruntBoard==CBrickzGrid facet
        i32 nyt = newY >> 5;
        i32 nxt = newX >> 5;
        i32 oxt = this->m_lastTilePxX >> 5;
        i32 oyt = this->m_lastTilePxY >> 5;
        if (!(oxt == nxt && oyt == nyt)) {
            if ((u32)nxt >= (u32)grid->m_width) {
                return 1;
            }
            if ((u32)nyt >= (u32)grid->m_height) {
                return 1;
            }
            i32* cell = grid->m_8[nyt] + nxt * 7;
            i32 t = flags & cell[0];
            if (t & 0x20000000) {
                return 1;
            }
            if (t != 0 && (cell[0] & (this->m_24c | 0x18000482)) == 0) {
                return 1;
            }
            i32* ocell = grid->m_8[oyt] + oxt * 7;
            i32 dxt = nxt - oxt;
            i32 dyt = nyt - oyt;
            if (dxt != 0 && dyt != 0) {
                i32 rb = grid->m_width * 7 * 4;
                if (dxt > 0) {
                    if (dyt > 0) {
                        if ((*(i32*)((char*)ocell + 0x1c) & 0x2000)
                            || (*(i32*)((char*)ocell + rb) & 0x2000)
                            || (*(i32*)((char*)cell - 0x1c) & 0x2000)
                            || (*(i32*)((char*)cell - rb) & 0x2000)) {
                            return 1;
                        }
                    } else {
                        if ((*(i32*)((char*)ocell + 0x1c) & 0x2000)
                            || (*(i32*)((char*)ocell - rb) & 0x2000)
                            || (*(i32*)((char*)cell - 0x1c) & 0x2000)
                            || (*(i32*)((char*)cell + rb) & 0x2000)) {
                            return 1;
                        }
                    }
                } else {
                    if (dyt > 0) {
                        if ((*(i32*)((char*)ocell - 0x1c) & 0x2000)
                            || (*(i32*)((char*)ocell + rb) & 0x2000)
                            || (*(i32*)((char*)cell + 0x1c) & 0x2000)
                            || (*(i32*)((char*)cell - rb) & 0x2000)) {
                            return 1;
                        }
                    } else {
                        if ((*(i32*)((char*)ocell - 0x1c) & 0x2000)
                            || (*(i32*)((char*)ocell - rb) & 0x2000)
                            || (*(i32*)((char*)cell + 0x1c) & 0x2000)
                            || (*(i32*)((char*)cell + rb) & 0x2000)) {
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
        CBrickzGrid* g2 = g_gameReg->m_tileGrid; // GruntBoard==CBrickzGrid facet
        i32 ox = this->m_lastTilePxX >> 5;
        i32 oy = this->m_lastTilePxY >> 5;
        i32* oc = g2->m_8[oy] + ox * 7;
        *((unsigned char*)oc + 3) &= 0xdf;
        i32* oc2 = g2->m_8[oy] + ox * 7;
        oc2[1] = -1;
        i32* nc = g2->m_8[nyt] + nxt * 7;
        *((unsigned char*)nc + 3) |= 0x20;
        i32* nc2 = g2->m_8[nyt] + nxt * 7;
        nc2[1] = (this->m_tileOwnerHi << 8) | this->m_tileOwnerLo;

        if (m_31c.GetCount() != 0) {
            i32* node = 0;
            i32 rx = this->m_lastTilePxX >> 5;
            i32 ry = this->m_lastTilePxY >> 5;
            if (*(void**)g_coordPool.m_freeHead != 0) {
                node = (i32*)((char*)g_coordPool.m_freeHead + 4);
                node[0] = rx;
                node[1] = ry;
                g_coordPool.m_freeHead = *(void**)g_coordPool.m_freeHead;
            }
            m_31c.AddHead(node);
        }

        this->m_lastTilePxX = newX;
        this->m_lastTilePxY = newY;
        this->m_prevAnimSetNode = this->m_14->m_1c;
        this->m_14->m_1c = g_buteTree.Find(s_typeO);
        double ddx = (double)newX - this->m_object->m_screenX;
        double ddy = (double)newY - this->m_object->m_screenY;
        double dist = sqrt(ddx * ddx + ddy * ddy);
        u32 kb = g_buteMgr.GetDwordDef(s_gruntSec, s_knockKey, 200);
        m_400 = dist / (double)kb;
        m_408 = (double)(this->m_object->m_screenX);
        m_410 = (double)(this->m_object->m_screenY);

        if (m_31c.GetCount() != 0) {
            CoordNode* node = (CoordNode*)m_31c.GetHeadPosition();
            if (node != 0) {
                void* fl = g_coordPool.m_freeHead;
                do {
                    CoordNode* cur = node;
                    node = cur->m_next;
                    Coord* data = cur->m_coord;
                    if (data != 0) {
                        CoordPoolNode* slot =
                            (CoordPoolNode*)((char*)data - g_coordPool.m_linkOffset);
                        slot->m_next = (CoordPoolNode*)fl;
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
        GruntBoard* bd = g_gameReg->m_tileGrid;
        i32 tx = m_lastTilePxX >> 5;
        i32 ty = m_lastTilePxY >> 5;
        i32 flags;
        if ((u32)tx >= (u32)bd->m_c || (u32)ty >= (u32)bd->m_10) {
            flags = 1;
        } else {
            flags = ((i32*)bd->m_8[ty])[tx * 7];
        }
        if (flags & 0x80) {
            return 0;
        }
    }

    CreateHealthSprite();
    m_combatTimeoutLo = (i32)g_buteMgr.GetDwordDef(s_Grunt, s_CombatTimeout, 0x1388);
    m_combatTimeoutHi = 0;
    m_combatClockLo = (i32)g_frameTime;
    m_combatClockHi = 0;
    m_358 = 1;

    CGrunt* nb = m_tileMgr->m_grid[a * TM_GRID_COLS + b];
    if (nb == 0 || nb->m_entranceCommitted == 0 || m_entranceCommitted == 0) {
        return 0;
    }

    bool eq;
    eq = (strcmp(*g_typeColl.GetNameRecord(m_14->m_1c), s_codeF) == 0);
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

    eq = (strcmp(*g_typeColl.GetNameRecord(m_14->m_1c), s_codeI) == 0);
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
        eq = (strcmp(*g_typeColl.GetNameRecord(m_14->m_1c), s_codeN) == 0);
        if (eq) {
            i32 px = (m_10->m_screenX & ~0x1f) + 0x10;
            i32 py = (m_10->m_screenY & ~0x1f) + 0x10;
            i32 redo = 1;
            if (px != m_lastTilePxX || py != m_lastTilePxY) {
                if (IsDropReady(1)) {
                    m_coordToggle = (m_coordToggle == 0);
                    redo = 0;
                }
            }
            SnapToLastTile(1);
            if (redo) {
                m_prevAnimSetNode = m_14->m_1c;
                m_14->m_1c = (void*)g_buteTree.Find(s_codeD);
                OnCoordCommit(m_coordToggle);
            }
        }
    }

    // The shared combat finalize.
    if (m_arrivalPending != 0) {
        m_tileMgr->WireTileSwitchLogic(this, m_10->m_screenX, m_10->m_screenY);
        m_arrivalPending = 0;
    }
    m_poweredUp = 1;
    nb->CreateHealthSprite();
    nb->m_combatTimeoutLo = (i32)g_buteMgr.GetDwordDef(s_Grunt, s_CombatTimeout, 0x1388);
    nb->m_combatTimeoutHi = 0;
    nb->m_combatClockLo = (i32)g_frameTime;
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
    nb->ArrivalRecycle(m_10->m_screenX, m_10->m_screenY, 0, m_tileOwnerHi, m_tileOwnerLo);
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
// inline-strcmp result-register coin-flip (docs/patterns/return-bool-via-local-setcc):
// CFG, every member offset/gate, the GetNameRecords+scratch-teardown, the inline
// CRT strcmp, the PlayMoveSound/CreateHealthSprite/GetDwordDef call shapes, the
// combat-timer block, and both returns are byte-faithful. Residue = retail lands the
// inline-strcmp result + the sete bool in eax/cl where cl picks ecx/al, cascading the
// register pairing through the bool-eval block; source-invariant (named `eq` local
// already in place). Deferred to the final sweep.
RVA(0x0005b570, 0x12b)
i32 CGrunt::BeginAttack(i32 a, i32 b) {
    if (m_entranceCommitted == 0) {
        return 0;
    }
    char* nm = g_typeColl.GetNameRecords(m_14->m_1c)->m_name;
    GruntScratchTeardown();
    bool eq = (strcmp(nm, s_codeF) == 0);
    if (eq) {
        return 0;
    }
    if (m_stamina < 0x64) {
        return 0;
    }

    PlayMoveSound(a, b);
    m_poweredUp = 1;
    m_combatActive = 1;
    CreateHealthSprite();

    m_combatTimeoutLo = (i32)g_buteMgr.GetDwordDef(s_Grunt, s_CombatTimeout, 0x1388);
    m_combatTimeoutHi = 0;
    m_combatClockLo = (i32)g_frameTime;
    m_combatClockHi = 0;
    m_358 = 1;
    m_208 = a;
    m_20c = b;
    RearmAttackAnim2();
    return 1;
}

// ---------------------------------------------------------------------------
// CGrunt::FindGridNeighbor(int validate)   @0x5b6f0
// Resolves the grunt's stored grid-cell neighbour (the CGrunt* at
// m_tileMgr's 15-wide cell grid indexed by the latched coords m_neighborCol/m_neighborRow), validates
// it occupies the expected tile, runs the tile-rect predicate + commit, and
// returns it. __thiscall, ret 4. Frameless. On any miss it clears m_neighborValid and
// returns 0; a validate-mismatch returns 0 WITHOUT touching m_neighborValid.
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
            if (n->m_10->m_screenX != n->m_lastTilePxX) {
                return 0;
            }
            if (n->m_10->m_screenY != n->m_lastTilePxY) {
                return 0;
            }
        }
        if (RectContains(n->m_10->m_screenX, n->m_10->m_screenY)) {
            CommitNeighbor(m_neighborCol, m_neighborRow, n->m_10->m_screenX, n->m_10->m_screenY);
            return n;
        }
    }

    m_neighborValid = 0;
    return 0;
}

// ---------------------------------------------------------------------------
// @rehomed CDDrawSubMgrLeafScan::Lookup_05b7e0 <- ddrawsubmgr (rule-(c) interleaver)
// CDDrawSubMgrLeafScan::Lookup (0x0005b7e0) - homed here from DDrawSubMgr.cpp
// (REHOME D10). Retail emits this own-class-out-of-line COMDAT INSIDE CGrunt's
// gruntcombat block (0x0005b6f0 FindGridNeighbor .. 0x0005baf0 GruntSpawnPump, both
// gruntcombat), a rule-(c) interleaver surrounded by gruntcombat on both sides.
// Look up `key` in the +0x10 map, return the found CObject* (null if absent). The
// class is now declared via <DDrawMgr/DDrawSubMgrLeafScan.h> (added above).
RVA(0x0005b7e0, 0x23)
CObject* CDDrawSubMgrLeafScan::Lookup_05b7e0(const char* key) {
    void* val = 0;
    m_10.Lookup(key, val); // CMapStringToPtr::Lookup @0x1b8438 (void*& out-param)
    return (CObject*)val;
}

// ==== GruntSpawnPump @0x5baf0 (ex GruntSpawnPump.cpp; a worker-pump handler whose leaf is CGrunt) ====
RVA(0x0005baf0, 0xf4)
i32 GruntSpawnPump(Owner* owner) {
    Worker* rec = owner->m_7c;
    switch (rec->m_1c) {
        case 0: {
            rec->m_1c = 0x3e8;
            CUserLogic* sub = new CGrunt((CGameObject*)owner);
            sub->Activate(); // slot 6 (+0x18)
            rec->m_18 = sub;
            break;
        }
        case 0x1d:
            rec->m_18->UserLogicVfunc9(); // slot 11 (+0x2c)
            break;
        case 0x1e:
            rec->m_18->UserLogicVfunc8(); // slot 10 (+0x28)
            break;
        case 0x50:
            rec->m_18->UserLogicVfuncC(); // slot 14 (+0x38)
            break;
        case 0x53:
            rec->m_18->UserLogicVfuncD(); // slot 15 (+0x3c)
            break;
        case 0x52:
            rec->m_18->UserLogicVfuncA(); // slot 12 (+0x30)
            break;
        case 0x51:
            rec->m_18->UserLogicVfuncB(); // slot 13 (+0x34)
            break;
        case 0x3e8:
            break;
        default:
            Worker_DefaultPump(rec->m_18);
            break;
    }
    return 1;
}

// CGrunt::RunAct @0x05bcd0 - vtable slot-4 activation dispatcher. Resolve `id`'s
// handler in the per-class registry g_reg_644af0 (the ResolveEntry fast [lo,hi]
// range + slow GrowTo/GetRetAddr/Insert rebuild, inlined twice - side-effecting, so
// cl re-evaluates it for the guarded call); when a handler is bound, dispatch it as
// a PMF on `this`, else return the entry pointer. Same archetype as CPathHazard::RunAct.
extern CLookupColl g_reg_644af0; // 0x644af0  (CGrunt's per-class activation registry)
// The static initializer that builds registry 0x644af0's fast [0x7d0, 0x7da] id range.
RVA(0x0005bc50, 0x15)
void ConstructActRange_644af0() {
    ((CZDArrayDerived*)&g_reg_644af0)->Construct(0x7d0, 0x7da);
}

RVA(0x0005bcd0, 0x102)
i32 CGrunt::RunAct(i32 id) {
    CGruntActEntry* e = (CGruntActEntry*)g_reg_644af0.ResolveEntry(id);
    if (e->m_fn != 0) {
        return (this->*((CGruntActEntry*)g_reg_644af0.ResolveEntry(id))->m_fn)();
    }
    return (i32)e;
}

// @early-stop
// count-down free-loop induction wall (~94.7%): every Find/Insert/name-lookup/
// Assign/table-store + all 19 keys and handlers are byte-faithful, both lookups
// outline to the shared helper as retail does and all 19 blocks expand inline.
// Sole residual is the `ecx=cnt; eax=cnt-1; lea ebx,[eax+1]` node-free idiom + the
// slot-vs-id callee-saved coloring repeated per block. Not source-steerable.
RVA(0x0005be30, 0x9e5)
void RegisterActs_644af0() {
    REGISTER_KEY_644AF0(s_codeA, &CGrunt::ResolveEntranceArrival);
    REGISTER_KEY_644AF0(s_actKeyB, &CGrunt::StepWarpExit);
    REGISTER_KEY_644AF0(k_60cc90, &CGruntBehaviorLeaf::LoadGruntDecayConfig);
    REGISTER_KEY_644AF0(s_codeD, &CGrunt::StepArrivalReroll);
    REGISTER_KEY_644AF0(s_codeE, &CGrunt::UpdateGruntStatus);
    REGISTER_KEY_644AF0(s_codeF, &CGrunt::DispatchVtbl24);
    REGISTER_KEY_644AF0(s_codeG, &CGrunt::StepEntranceRelatchA);
    REGISTER_KEY_644AF0(s_codeH, &CGrunt::StepArrivalCommitA);
    REGISTER_KEY_644AF0(s_codeI, &CGruntBehaviorLeaf::LoadWandGruntItemConfig);
    REGISTER_KEY_644AF0(s_codeJ, &CGrunt::RunEntranceMove);
    REGISTER_KEY_644AF0(s_codeK, &CGrunt::LoadEntranceConfig);
    REGISTER_KEY_644AF0(s_codeL, &CGrunt::LoadVehicleGruntAnimations);
    REGISTER_KEY_644AF0(s_codeM, &CGrunt::RearmEntranceDrop);
    REGISTER_KEY_644AF0(s_codeN, &CGrunt::StepEntranceRelatchB);
    REGISTER_KEY_644AF0(s_codeO, &CGrunt::StepArrivalCommitB);
    REGISTER_KEY_644AF0(s_codeP, &CGrunt::UpdateEntranceAnim);
    REGISTER_KEY_644AF0(s_codeQ, &CGrunt::LoadFreezeSpellAssets);
    REGISTER_KEY_644AF0(k_60bebc, &CGruntBehaviorLeaf::LoadGruntDecayConfig2);
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
i32 CGrunt::Activate() {
    double diag = sqrt(g_dirConst2);              // sqrt(2.0)
    double* tbl = (double*)((char*)this + 0x4b0); // 0x78-stride records (15 doubles each)
    const i32 W = 0x78 / 8;                       // 15 doubles per record

    double s = g_dirConst1 / diag; // 1 / sqrt2
    double n = g_dirConstN1 / s;   // -1 / (1/sqrt2)

    // Each record: 4 doubles at the cell's +0/8/0x10/0x18. The 9 globals are processed
    // in this fixed order (ab0,ae0,aa0,b28,ac0,b48,ad0,b18,b38).
    {
        i32 i = W * (3 * g_dirAb0[0] + g_dirAb0[1]);
        tbl[i + 0] = 0.0;
        tbl[i + 1] = -1.0;
        tbl[i + 2] = 0.0;
        tbl[i + 3] = -0.5;
    }
    {
        i32 i = W * (3 * g_dirAe0[0] + g_dirAe0[1]);
        tbl[i + 0] = s;
        tbl[i + 1] = s;
        tbl[i + 2] = 0.5;
        tbl[i + 3] = -0.5;
    }
    {
        i32 i = W * (3 * g_dirAa0[0] + g_dirAa0[1]);
        tbl[i + 0] = 1.0;
        tbl[i + 1] = 0.0;
        tbl[i + 2] = 0.5;
        tbl[i + 3] = 0.0;
    }
    {
        i32 i = W * (3 * g_dirB28[0] + g_dirB28[1]);
        tbl[i + 0] = s;
        tbl[i + 1] = s;
        tbl[i + 2] = 0.5;
        tbl[i + 3] = 0.5;
    }
    {
        i32 i = W * (3 * g_dirAc0[0] + g_dirAc0[1]);
        tbl[i + 0] = 0.0;
        tbl[i + 1] = 1.0;
        tbl[i + 2] = 0.0;
        tbl[i + 3] = 0.5;
    }
    {
        i32 i = W * (3 * g_dirB48[0] + g_dirB48[1]);
        tbl[i + 0] = n;
        tbl[i + 1] = s;
        tbl[i + 2] = -0.5;
        tbl[i + 3] = 0.5;
    }
    {
        i32 i = W * (3 * g_dirAd0[0] + g_dirAd0[1]);
        tbl[i + 0] = -1.0;
        tbl[i + 1] = 0.0;
        tbl[i + 2] = -0.5;
        tbl[i + 3] = 0.0;
    }
    {
        i32 i = W * (3 * g_dirB18[0] + g_dirB18[1]);
        tbl[i + 0] = n;
        tbl[i + 1] = n;
        tbl[i + 2] = -0.5;
        tbl[i + 3] = -0.5;
    }
    {
        i32 i = W * (3 * g_dirB38[0] + g_dirB38[1]);
        tbl[i + 0] = 0.0;
        tbl[i + 1] = 0.0;
        tbl[i + 2] = 0.0;
        tbl[i + 3] = 0.0;
    }

    // --- spawn-state reset tail (integer field stores) ---
    CGameObject* h = m_10;
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
    return 0;
}

#undef REGISTER_KEY_644AF0
