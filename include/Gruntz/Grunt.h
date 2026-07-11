// Grunt.h - the engine's CGrunt (the player/enemy grunt entity). Members with a
// recovered role carry semantic names; offsets whose role isn't yet provable keep
// the m_<hexoffset> placeholder. ONLY the OFFSETS + code bytes are load-bearing
// (names are codegen-neutral at /O2). CGrunt is huge; this header models the
// member offsets the matched methods touch + the small external classes they call.
//
// The 7 creators (CGrunt::Create*Sprite) all share one
// shape: bail if the target sprite slot is already populated (some also gate on
// a stat/flags member), then ask the global HUD sprite factory to build a named
// sprite (the factory is reached via the global registry ptr -> +0x30 -> +0x8, a __thiscall
// taking the sprite class-name string + a couple of HUD-geometry values derived
// from this->m_10 (+0x5c / +0x60)), store the new sprite into the slot, run its
// slot-0x10 init virtual, then register it into the grunt's sprite collection
// (sprite->m_7c->m_18 . Add*(...)). If the register call fails, set a flag bit
// (0x10000) on the collection's record, null the slot, and return 0; else 1.
#ifndef SRC_GRUNTZ_GRUNT_H
#define SRC_GRUNTZ_GRUNT_H

class CAniElement; // folded CEntranceAnimDescColl

class FreeNodePool; // folded GruntCoordPool

class CDDrawSubMgrLeaf; // folded CGruntNameMap

class CSoundCueMgr; // folded GruntSampleFactory

class DirectSoundMgr; // folded GruntSoundSample

#include <Ints.h>
#include <Gruntz/LogicTypeId.h>
#include <rva.h> // SIZE_UNKNOWN/VTBL class-metadata macros used below
#include <Gruntz/SpriteFactory.h>
#include <Gruntz/UserBaseLink.h>   // shared CUserBaseLink (+0x18 link; ~EngStr 0x16d2a0)
#include <Gruntz/SpriteRefTable.h> // CSpriteRefTable (g_gameReg->m_74; GetSel)
#include <Gruntz/WwdGameReg.h>     // the canonical WwdGameReg singleton layout (g_gameReg)
#include <Gruntz/UserLogic.h>
#include <Gruntz/MovingLogic.h>

// ---------------------------------------------------------------------------
// The receiver the Add* registration call runs on: edi = sprite->m_7c->m_18.
// On failure the creators do `edi = edi->m_38; edi->m_8 |= 0x10000`. The Add*
// methods are unmatched engine methods (reached via incremental-link thunks);
// declared external/no-body so their `call rel32` displacements reloc-mask.
//   AddA(a, b, c)  (Health/Stamina/ToyTime/WingzTime; 3 args)
//   AddB(a, b)    (Toy; 2 args)
//   AddC(a, b, c)  (Powerup; 3 args)
//   AddD(a, b)    (Selected; 2 args)
// ---------------------------------------------------------------------------
SIZE_UNKNOWN(CSpriteRegRecord);
struct CSpriteRegRecord {
    char m_pad0[0x8];
    u32 m_8; // +0x08  failure flag word (|= 0x10000)
};

SIZE_UNKNOWN(CSpriteRegistrar);
class CSpriteRegistrar {
public:
    i32 AddA(i32 a, i32 b, i32 c);
    i32 AddB(i32 a, i32 b);
    i32 AddC(i32 a, i32 b, i32 c);
    i32 AddD(i32 a, i32 b);
    // The LightFx activate (0x2117): m_18->Activate(setKey, flashKey, frame, loop).
    void Activate(const char* setKey, const char* flashKey, i32 frame, i32 loop);

    char m_pad0[0x38];
    CSpriteRegRecord* m_38; // +0x38  (failure-flag record holder)
};

// ---------------------------------------------------------------------------
// The HUD sprite the factory builds. The creators read sprite->m_7c (an inner
// object), call sprite->m_7c->[0x10](sprite) (init), and reach the registrar at
// sprite->m_7c->m_18. Layout-free apart from m_7c.
// ---------------------------------------------------------------------------
SIZE_UNKNOWN(CSpriteInner);
struct CSpriteInner {
    char m_pad0[0x10];
    void (*m_init)(void* self); // +0x10  init virtual (called with sprite)
    char m_pad14[0x18 - 0x14];
    CSpriteRegistrar* m_18; // +0x18  the registrar
    char m_pad1c[0xbc - 0x1c];
    i32 m_bc; // +0xbc  (rolling-ball speed; LoadGruntAbilityTuning)
};

SIZE_UNKNOWN(CHudSprite);
struct CHudSprite {
    // The CGameObject-base name/geometry setters the one-shot "SingleAnimation"
    // sprite (BuildGruntLoseItemAnimation) drives: ApplyName(key) (0x150540, ret 4)
    // and ApplyLookupGeometry(key, frame) (0x1505b0, ret 8). External/reloc-masked.
    // The CGameObject-base name/sprite setters, folded here (the former per-TU CGruntSprite
    // facet view is gone); the created sprite IS this game object.
    void CacheFirstFrame(const char* name);               // 0x1504d0
    void CacheFrame(const char* key, i32 frame);          // 0x150540 (ApplyName)
    void ApplyLookupGeometry(const char* key, i32 frame); // 0x1505b0

    char m_pad0[0x8];
    i32 m_8; // +0x08  (sprite flag word; arrival sets |= 0x10000 to retire it)
    char m_padc[0x7c - 0xc];
    CSpriteInner* m_7c; // +0x7c
    char m_pad80[0x118 - 0x80];
    i32 m_118; // +0x118  (rolling-ball time; LoadGruntAbilityTuning)
    char m_pad11c[0x124 - 0x11c];
    i32 m_124; // +0x124  (rolling-ball; cleared)
};

// The on-screen cue gate's visibility rect, reached as
// (g->m_30->m_24->m_5c + 0x40): {left, top, right, bottom}. The viewport's m_5c is
// a base address (modeled i32 in CGameRegistry.h), so the +0x40 view is a cast.
SIZE_UNKNOWN(CCueRect);
struct CCueRect {
    i32 left;   // +0x00
    i32 top;    // +0x04
    i32 right;  // +0x08
    i32 bottom; // +0x0c
};

// ---------------------------------------------------------------------------
// this->m_10 (a HUD/level geometry source). The factory's two geometry args are
// m_10->m_5c and m_10->m_60 (the latter optionally minus a per-sprite constant).
//
// NOT the same object as GruntArriveResolve.cpp's CArriveMgr (investigated - a prior
// note wrongly conflated them). The two only share coords at +0x5c/+0x60 (coincident
// offset in two classes), but DIVERGE at +0x08: CGruntHud (this, a CGameObject-derived
// sprite) uses +0x08 as the standard CGameObject FLAGS word (the `m_flags |= 0x20000`
// pattern shared by ~dozen TUs; corroborated by +0xe4 CGameObject update-state, +0x74
// latched anim id, +0x134-140 view-cull). CArriveMgr uses +0x08 as a MOVER POINTER
// (mov ecx,[this+0x8]; call Move14bf) because it is a different class entirely: the
// BATTLEZ board/logic object. Proof it is not this HUD - CArriveMgr's method
// Resolve2c690 (0x2c690) sits inside CBattlezMapConfig's RVA band (0x25020..0x358a0)
// and is reached only from CBattlezMapConfig code (Method_025d90 @0x25d90 ->
// winapi_0267c0 @0x267c0, which at 0x282f1 does `mov ecx,ebp; push esi; call Resolve`),
// and its shape (cell-grid @+0x0c, mover @+0x08, finder @+0x14) is a board manager, not
// a sprite. So there is NO single object with both a flags word and a mover at +0x08;
// the "conflict" was a spurious cross-class merge. CArriveMgr IS CBattlezMapConfig
// (run-phase view; offsets +0x08/+0x0c/+0x10(->+0x2e4)/+0x14/+0x18/+0x5c/+0x60 match
// BattlezMapConfig.h exactly) - it folds there, NOT onto CGruntHud.
// ---------------------------------------------------------------------------
SIZE_UNKNOWN(CGruntHud);
struct CGruntHud {
    char m_pad0[0x8];
    i32 m_8; // +0x08   CGameObject flags word (BuildEntrance |= 0x20000; NOT the battlez mover)
    char m_padc[0x40 - 0xc];
    i32 m_40; // +0x40   (sprite-state flag word; ExitAnim/RunConfig clears bit 8)
    char m_pad44[0x4c - 0x44];
    i32 m_4c; // +0x4c   (SelectMoveIcon: = GetSel result)
    i32 m_50; // +0x50   (SelectMoveIcon: = 0xa)
    char m_pad54[0x58 - 0x54];
    i32 m_58; // +0x58   (SelectMoveIcon: = 1)
    i32 m_5c; // +0x5c
    i32 m_60; // +0x60
    char m_pad64[0x74 - 0x64];
    i32 m_74; // +0x74   (entrance: latched anim id; cmp 0xcf850)
    char m_pad78[0xe4 - 0x78];
    i32 m_e4; // +0xe4   (CGameObject dirty/update state; ctor stamps 7 then 1)
    char m_pade8[0x11c - 0xe8];
    i32 m_11c; // +0x11c  (ctor snapshot -> m_434)
    char m_pad120[0x134 - 0x120];
    i32 m_134; // +0x134  (arrival: view-cull mode cleared)
    i32 m_138; // +0x138  (arrival: view-cull, cleared)
    i32 m_13c; // +0x13c  (arrival: view-cull, cleared)
    i32 m_140; // +0x140  (arrival: view-cull, cleared)
    char m_pad144[0x148 - 0x144];
    i32 m_148;   // +0x148
    i32 m_14c;   // +0x14c
    void* m_150; // +0x150
    char m_pad154[0x188 - 0x154];
    i32 m_188; // +0x188  (cue arg)
};

// ---------------------------------------------------------------------------
// The global HUD sprite factory, reached via the global registry ptr -> +0x30 -> +0x8.
// CreateSprite is __thiscall(this, 0, geoB, geoA, hint, name, kind) ret 0x18
// Modeled as a method on the registry singleton so the call shape
// (factory-this = g->m_30->m_8) + the 6-arg push fall out; external/no-body so
// the `call rel32` reloc-masks.
// ---------------------------------------------------------------------------
// The object-table entry resolved through the factory's +0x48 map (Lookup
// 0x1b8760). Validated by a virtual kind() at vtable slot +0x20 (== 5 -> keep).
// +0x7c is the same inner-object slot CHudSprite carries (CSpriteInner: its
// +0x18 receiver takes the death resolve in HandleCommand's 0x8106 cheat).
// CSpriteFactoryHolder (the registry +0x30 holder) lives in <Gruntz/GameRegistry.h>.

class CGruntCueSink; // defined below (the 5-arg on-screen cue receiver)

// CGameRegistry - the shared global singleton (*g_pGameRegistry). The CGrunt
// resolvers below read the visible-bounds gate (m_134, m_13c..m_148) and fire
// m_60->Cue.
#include <Gruntz/GameRegistry.h>

// The global manager pointer.
extern CGameRegistry* g_pGameRegistry;

// ===========================================================================
// Animation-resolver cluster support (the 5 CGrunt::Resolve*Animation methods)
// ===========================================================================
//
// An MFC-style CString (a single char* @+0). CGrunt stores its grunt-type name
// in a CString member @+0x54; each resolver builds an animation key string
//   "GRUNTZ_" + this->m_typeName + "_<CATEGORY>"
// via the two engine global operator+ overloads, then hands the resulting char*
// to the animator's lookup setter. Only the calls the resolvers emit are
// modeled (external/no-body so the `call rel32` displacements reloc-mask): the
// two operator+ overloads and the dtor.
#include <Gruntz/String.h>

// operator+(LPCTSTR, const CString&)  ("GRUNTZ_" + m_typeName)
// operator+(const CString&, LPCTSTR)  (... + "_CATEGORY")
// AFXAPI == __stdcall: the callee pops the hidden return slot + both args
// (ret 0xc), so there is NO `add esp` at the call site.
CString __stdcall operator+(const char* lhs, const CString& rhs);
CString __stdcall operator+(const CString& lhs, const char* rhs);

// ---------------------------------------------------------------------------
// CGruntAnimState - the per-grunt animation player the resolver drives (CGrunt
// member @+0x38). The resolver feeds it the resolved animation in two steps:
//   * SetGeometry(srcSprite) on the sub-player @+0x1a0 (engine,
//     __thiscall ret 4) - also exposes m_1b4 (the active anim descriptor; the
//     resolver caches m_1b4 into CGrunt::m_activeAnimDesc before the call, and Idle reads
//     m_1b4->{m_c,m_10} to derive a 2nd lookup arg);
//   * SetAnim(key) (engine, __thiscall ret 4)  - 1-arg form, OR
//     SetAnimEx(key, frame) (engine __thiscall ret 8) - 2-arg form
//     (Idle only) - given the built animation-key char*.
// All three are external/no-body (reloc-masked). m_1a0 is a raw sub-object the
// geometry setter runs on; m_1b4 is the active-anim descriptor pointer.
// ---------------------------------------------------------------------------
// An animation-frame element the Idle resolver reads a sub-arg from.
SIZE_UNKNOWN(CAnimElem);
struct CAnimElem {
    char m_pad0[0x14];
    i32 m_14; // +0x14
};

SIZE_UNKNOWN(CGruntAnimSub);
class CGruntAnimSub {
public:
    // The geometry sub-player setter (0x15c2d0, external/reloc-masked; formerly reached
    // by a per-TU CDDrawBlitParam facet cast on &state->m_1a0).
    void SetGeometry(i32 src);
};

SIZE_UNKNOWN(CGruntAnimState);
class CGruntAnimState {
public:
    void SetAnim(const char* key);              // (ret 4)
    void SetAnimEx(const char* key, i32 frame); // (ret 8)

    char m_pad0[0x1a0];
    CGruntAnimSub m_1a0; // +0x1a0  (geometry sub-player)
    i32 m_1a4;           // +0x1a4
    char m_pad1a8[0x1b4 - 0x1a8];
    CAniElement* m_1b4; // +0x1b4  active-anim descriptor
};

// The animation-set record the lookup tree (a CButeTree) returns;
// stored into CGrunt::m_14->m_1c. m_1c holds the resolved anim-set node.
SIZE_UNKNOWN(CAnimLookupNode);
struct CAnimLookupNode {
    char m_pad0[0x1c];
    void* m_1c; // +0x1c
};

// CButeTree::Find (__thiscall ret 4) - the shared keyed lookup.
SIZE_UNKNOWN(CAnimLookupTree);
class CAnimLookupTree {
public:
    void* Find(const char* key); // stub
};

// The global animation lookup tree instance.
extern CAnimLookupTree g_animLookupTree;

// A per-grunt time/seed default the Moving resolver copies into m_moveSeed.
extern i32 g_movingSeed;

// The engine LCG rand() (no args) the Moving/Idle/Battlecry resolvers
// use to pick an animation index / start time.
extern "C" i32 GruntRand(); // stub

// ---------------------------------------------------------------------------
// The on-screen-cue receiver reached via g_pGameRegistry->m_cueSink (a __thiscall
// ret 0x14 = 5 stack args). The resolvers fire a 5-arg cue when the
// grunt is on-screen (m_134 == 1 -> 4-way visible-bounds test) or unconditionally
// otherwise. External/no-body (reloc-masked; reached via incremental-link thunk).
//
// BuildEntranceAnimation fires a SIX-arg variant (a different cue overload, also
// via g->m_60); modeled as a second method (CueA, ret 0x18). Both reloc-mask.
// ---------------------------------------------------------------------------
class CGrunt; // fwd-declared for CueA's first arg

SIZE_UNKNOWN(CGruntCueSink);
class CGruntCueSink {
public:
    void Cue(i32 a, i32 b, i32 c, i32 d, i32 e);             // via thunk 0x33b4
    void Cue1(i32 a);                                        // 1-arg cue (thunk_0x1163 -> 0x51c730)
    void CueA(CGrunt* g, i32 b, i32 c, i32 d, i32 e, i32 f); // 6-arg entrance cue (ret 0x18)
    void CueSpawn(CGrunt* g, i32 b, i32 c, i32 d, i32 e);    // via thunk 0x27ac (ret 0x14)
    // 0x39f4: the on-screen event cue the per-tick game-object managers (Obj0f7d90::Update)
    // fire on the registry's m_cueSink when the managed object is inside the viewport rect.
    void CueEvent(void* obj, i32 id, i32 c, i32 d, i32 e, i32 f);
};

// The entrance-reset (ResetEntranceAnimation) cue-gate visibility helper (thunk_FUN_0046b330,
// __cdecl(viewport, x, y) ret int): tests whether the grunt's HUD point is inside
// the viewport rect. External/no-body (reloc-masked).
i32 CueVisible(i32 viewport, i32 x, i32 y);

// ---------------------------------------------------------------------------
// The entrance-animation sub-object @CGrunt+0x154: a per-grunt animation player.
// BuildEntranceAnimation reaches a name->sprite-set lookup table through
// player->m_c (a resource object) +0x2c +0x10 (the embedded map) and drives the
// geometry sub-player @+0x1a0 with the resolved sprite. The map Lookup, the
// geometry setter, and the frame helper are all external/no-body (reloc-masked).
// ---------------------------------------------------------------------------
struct CSprite; // opaque looked-up sprite

SIZE_UNKNOWN(CEntranceHashTable);
class CEntranceHashTable {}; // MFC CMapStringToOb (Lookup @0x1b8438); cast at each call

SIZE_UNKNOWN(CEntranceSpriteMgr);
struct CEntranceSpriteMgr {
    // 1-arg lookup returning the resolved sprite directly (the EXIT/RUN loaders
    // use this form; the 2-arg m_10map.Lookup writes through an out-param instead).
    // (Formerly reached by a per-TU CDDrawSubMgrLeaf facet cast; folded here.)
    void* LookupValue_06b2a0(const char* key); // 0x6b2a0 (external/reloc-masked)

    char m_pad0[0x10];
    CEntranceHashTable m_10map; // +0x10
};

// The grunt's exit-animation holder at CGrunt+0x150 (a 4-byte member just before
// the entrance player pointer). BuildGruntExitAnimation drives its Apply (0x6b2e0,
// 2-arg __thiscall) with the resolved sprite. External/no-body (reloc-masked).
SIZE_UNKNOWN(CEntranceResMgr);
struct CEntranceResMgr {
    char m_pad0[0x2c];
    CEntranceSpriteMgr* m_2c; // +0x2c
};

// The active-anim descriptor the entrance player exposes (its first element's
// +0x14 frame number is the 2nd arg the frame helper consumes).
SIZE_UNKNOWN(CEntranceAnimSub);
class CEntranceAnimSub {
public:
    void SetGeometry(i32 srcSprite); // FUN_0055c2d0 (this = player+0x1a0, ret 4)
    // The geometry-source ready probe (0x15c360, ret 4; formerly reached by a per-TU
    // CAniAdvanceCursor facet cast on &player->m_1a0). External/reloc-masked.
    i32 Advance_15c360(unsigned int i);
    // The geometry-state setter LoadEntranceConfig calls on entry; returns 1 when
    // the player is ready (FUN_0055c360, __thiscall ret 4 = 1 stack arg). Same
    // engine fn as SpriteResource's SetGeoSource, but the int return is used here.
    // SetGeoSourceR @0x15c360 IS CAniAdvanceCursor::Advance_15c360; cast at each call.
    // Data-less view: the geometry sub-player's m_20/m_28 (abs CGrunt+0x154+0x1a0
    // +0x20/+0x28) live PAST the player's own m_1b4, so they are not modeled as
    // embedded data here (that would corrupt m_1b4's offset). LoadEntranceConfig's
    // tail and UpdateEntranceAnim's armed-but-not-running gate read them via raw
    // offsets off &player->m_1a0 instead (keeps cl on one `add eax,0x1a0` base).
};

// A per-cell entrance record (0x68-byte stride at CGrunt+0x474). GetName(flag)
// resolves the cell's frame name (__thiscall, 1 arg). External (reloc-masked).
SIZE_UNKNOWN(CGruntCell);
class CGruntCell {
public:
    // GetName @0x310f0 IS zDArray::IndexToPtr; cast at each call.
};

SIZE_UNKNOWN(CEntranceAnimPlayer);
class CEntranceAnimPlayer {
public:
    // Geometry setter that forwards to m_1a0.SetGeometry(src) then, if flag!=0,
    // a 2nd setter (FUN_00458b60, ret 8). PlaySound's IDLE arm drives it directly.
    // A 1-arg setter the WALK/E arms call on the player itself (FUN_00550540,
    // FUN_005504d0 is the 2-arg form). Takes the resolved cell name.
    // The death/freeze finalize 2-arg geometry setter (FUN_005505b0); takes the
    // resolved key string + a flag. External/reloc-masked.
    // The combat-reaction dispatch (@0x646b0) drives the player through its
    // CGameObject base name/sprite setters (0x150540 / 0x1504d0, not the 0x55xxxx
    // entrance forms). External/no-body so the call rel32 reloc-masks.
    // The CGameObject-base lookup-geometry setter (same 0x1505b0 slot CHudSprite
    // uses) the death/freeze finalize drives with the DEATHZ_SPARKLE/UNFREEZE keys.
    //
    // The CGameObject-base name/sprite/geometry setters the asset loaders drive on the
    // player directly (external/reloc-masked so the call rel32 masks; the former per-TU
    // CGruntSprite/CGruntAnimPlayer facet views are folded here). The player IS the
    // created game object.
    void CacheFirstFrame(const char* name); // 0x1504d0
    // The 2-arg frame-cache form at the same 0x1504d0 slot (the CGameObject-base
    // ApplyLookupSprite(key, flag); the entrance re-stamp steps drive it with the
    // built cell-name buffer + the active descriptor's first-element frame index).
    void CacheFrameIndexed(const char* key, i32 frame);   // 0x1504d0 (2-arg)
    void CacheFrame(const char* key, i32 frame);          // 0x150540
    void ApplyLookupGeometry(const char* key, i32 frame); // 0x1505b0
    void ApplyGeometryDirect(i32 src, i32 flag);          // 0x58b60

    char m_pad0[0x8];
    i32 m_8;              // +0x08  state-flag word (death loader |= 1 / |= 0x10000)
    CEntranceResMgr* m_c; // +0x0c  resource object (lookup table holder)
    char m_pad10[0xe8 - 0x10];
    i32 m_e8; // +0xe8  (ctor = 0x100000)
    i32 m_ec; // +0xec  (ctor = 0x3d1)
    i32 m_f0; // +0xf0  (ctor = 1)
    i32 m_f4; // +0xf4  (ctor |= 0x103f)
    char m_padf8[0x148 - 0xf8];
    i32 m_148;   // +0x148
    i32 m_14c;   // +0x14c
    void* m_150; // +0x150
    char m_pad154[0x18c - 0x154];
    i32 m_18c; // +0x18c
    char m_pad190[0x194 - 0x190];
    i32 m_194;              // +0x194
    i32 m_198;              // +0x198
    i32 m_19c;              // +0x19c
    CEntranceAnimSub m_1a0; // +0x1a0 geometry sub-player
    i32 m_1a4;              // +0x1a4
    char m_pad1a8[0x1b4 - 0x1a8];
    CAniElement* m_1b4; // +0x1b4 active-anim descriptor
    char m_pad1b8[0x1c0 - 0x1b8];
    i32 m_1c0; // +0x1c0 (entrance-done flag B: 0 -> run reset)
    char m_pad1c4[0x1c8 - 0x1c4];
    i32 m_1c8; // +0x1c8 (entrance-done flag A: nonzero -> bail)
};

// CString::GetBuffer(int) the entrance-anim update (@0x690a0) calls to hand the
// grunt's +0x448 name CString raw to SetAnimFrame. __thiscall (this = &CString),
// ret 4. External/reloc-masked (the engine CString TU); modeled as a free helper
// taking the CString address so `lea ecx,[this+0x448]; push 0; call` falls out.
// NOTE: the correct __thiscall model (((CGruntStrBuf*)str)->GetBuffer(0)) nets +3-4%
// on RearmAttackAnim2/LoadVehicleGruntAnimations/StartBombGruntRun but REGRESSES
// UpdateEntranceAnim (-3.5%) + RearmAttackAnim via regalloc leakage - a TU-wide
// model change deferred to the final sweep (must fix the regressed callers in tandem).
char* GruntStrGetBuffer(void* str, i32 minLen); // 0x1ba11c

// The frame helper BuildEntranceAnimation calls at the tail (FUN_005504d0):
// __stdcall(keyStr, frameNum) - callee-pops (no `add esp` at the site). External.
void __stdcall EntranceApplyFrame(const char* keyStr, i32 frameNum);

// The entrance-anim-set source object (the global at DAT_006bf620). Its
// LookupAnimSet (FUN_0056d190, __thiscall ret 4) takes a single-char anim key
// and returns the new active-anim-set node that gets latched into m_14->m_1c.
// External/no-body (reloc-masked); the `push key; mov ecx, &g_entranceAnimSrc;
// call` is the load-bearing shape.
SIZE_UNKNOWN(CEntranceAnimSrc);
class CEntranceAnimSrc {
public:
    i32 LookupAnimSet(const char* key); // FUN_0056d190 (ret 4)
};
extern CEntranceAnimSrc g_entranceAnimSrc; // DAT_006bf620
#define EntranceLookupAnimSet(k) (((CButeTree*)&g_entranceAnimSrc)->Find(k))

// The grunt's current-anim-name resolver (the global at DAT_006bf650). Its
// GetNameRecord (thunk_FUN_004310f0, __thiscall ret 4) maps an anim-set node to
// a record whose first field is the anim's name char*. External/no-body.
//
// GetNameRecords (thunk_FUN_004312a0, ret 4) is the SECOND resolver: it resolves
// into the g_animScratch CString[] (count g_animScratchCount), returning the
// record whose +0 is the name char*. The grunt anim-dispatch state machines that
// reject by single-letter type code use the scratch form for the later codes (so
// each reject is followed by the scratch CString teardown loop, the shared
// loop-strength-reduction wall from docs/patterns).
SIZE_UNKNOWN(CAnimNameRecord);
class CAnimNameRecord {
public:
    char* m_name; // +0x00
};
SIZE_UNKNOWN(CAnimNameResolver);
class CAnimNameResolver {
public:
    char** GetNameRecord(void* node);            // thunk_FUN_004310f0 (ret 4)
    CAnimNameRecord* GetNameRecords(void* node); // thunk_FUN_004312a0 (ret 4)
    i32 Probe(i32 a, i32 b);                     // 0x016da80
    i32 Reserve(CAnimNameRecord* rec, i32 n);    // 0x034960
    // The scratch-resolve variant (thunk 0x3864, zDArray::IndexToPtr-class): returns
    // the resolved record; the caller then tears down the g_animScratch CString[]
    // (the inlined teardown loop) before reading rec->m_name. Reloc-masked.
    CAnimNameRecord* ScratchResolve(void* node); // 0x3864
    // The entrance-cell coordinate -> record-index mappers the arrival recycle
    // step (0x59230) drives when the raw bounds test misses. Both __thiscall on the
    // resolver (this = 0x6bf650); external/no-body so the calls reloc-mask.
    i32 MapCellIndex(i32 coord, i32 flag);  // FUN_0056da80 (ret 8)
    i32 MapCellRecord(i32 base, i32 size);  // FUN_00434960 (ret 8; block-1 fallback)
    i32 MapCellRecord2(i32 base, i32 size); // FUN_0056d850 (ret 0xc; block-2 fallback)
    i32 PinCellIndex();                     // FUN_0056d990 (ret 0; pop/push/ret stub)
};
extern CAnimNameResolver g_animNameResolver; // DAT_006bf650

// The resolver's coordinate-range fields (consecutive globals at 0x6bf654..0x6bf664;
// the cell-resolve path reads them by name - separate externs, reloc-masked - the same
// way g_animScratch/g_animScratchCount are aliased rather than embedded).
extern i32 g_cellLo;    // DAT_006bf658
extern i32 g_cellHi;    // DAT_006bf65c
extern i32 g_cellBase;  // DAT_006bf660
extern i32 g_cellRet;   // DAT_006bf664
extern i32 g_cellScale; // DAT_006bf668

// The entrance-cell record table base + the resolver fallback count (separate
// globals the cell-resolve path reads; reloc-masked).
extern i32 g_cellRecordBase; // DAT_006bf464
extern i32 g_cellRecordRet;  // DAT_006bf428

// ---------------------------------------------------------------------------
// The 9 runtime direction-index globals Activate (0x5caa0) reads to place
// each direction record (index = 3*[0] + [1]). Each is an adjacent {lo, hi} int
// pair; runtime-filled (.data), so modeled as a 2-int view extern (reloc-masked).
// The FP unit constants the diagonal vectors are built from (read-only .rodata).
// ---------------------------------------------------------------------------
extern i32 g_dirAb0[2];     // DAT_00644ab0
extern i32 g_dirAe0[2];     // DAT_00644ae0
extern i32 g_dirAa0[2];     // DAT_00644aa0
extern i32 g_dirB28[2];     // DAT_00644b28
extern i32 g_dirAc0[2];     // DAT_00644ac0
extern i32 g_dirB48[2];     // DAT_00644b48
extern i32 g_dirAd0[2];     // DAT_00644ad0
extern i32 g_dirB18[2];     // DAT_00644b18
extern i32 g_dirB38[2];     // DAT_00644b38
extern double g_dirConst2;  // DAT_005e9a28 = 2.0
extern double g_dirConst1;  // DAT_005e9a30 = 1.0
extern double g_dirConstN1; // DAT_005e9a38 = -1.0

// The second-resolver scratch CString[] (data @0x6bf66c, count @0x6bf670). Each
// reject path that resolves via GetNameRecords tears these down (Release each
// non-null slot, count times). Reloc-masked DATA.
SIZE_UNKNOWN(CAnimScratchString);
struct CAnimScratchString {
    char* m_str; // +0x00  (4-byte stride)
    // Release @0x1b9b93 IS CString::~CString; cast at each call.
};
extern CAnimScratchString* g_animScratch; // DAT_006bf66c
extern i32 g_animScratchCount;            // DAT_006bf670

// The single-letter anim type-code literals the grunt dispatch machines compare
// the current anim name against (each a 1-char .rodata string, reloc-masked).
//   "A"=idle  "D" "I" "G" "L" "P" "O" "Q" "J" "N" "M" "K"
extern const char g_codeA[]; // 0x60a454 "A"
extern const char g_codeD[]; // 0x60cca4 "D"
extern const char g_codeI[]; // 0x60cca0 "I"
extern const char g_codeG[]; // 0x60cc9c "G"
extern const char g_codeL[]; // 0x60cc98 "L"
extern const char g_codeP[]; // 0x60beb8 "P"
extern const char g_codeO[]; // 0x60dc0c "O"
extern const char g_codeQ[]; // 0x60dc08 "Q"
extern const char g_codeJ[]; // 0x60cc94 "J"
extern const char g_codeN[]; // 0x60dc04 "N"
extern const char g_codeM[]; // 0x60d7f4 "M"
extern const char g_codeK[]; // 0x60d7f8 "K"
extern const char g_codeF[]; // 0x60d2e8 "F"  (PlaySound entrance handler)
extern const char g_codeE[]; // 0x60d2ec "E"  (PlaySound entrance handler)
extern const char g_codeH[]; // 0x60d7fc "H"  (arrival-recycle reject code)

// The keyed anim-set lookup is g_entranceAnimSrc.LookupAnimSet (FUN_0056d190 @
// the global @0x6bf620, already modeled above): maps a single-char anim key to a
// new active anim-set node latched into m_14->m_1c. The grunt dispatch machines
// reach it as `mov ecx,0x6bf620; call 0x16d190`.

// ---------------------------------------------------------------------------
// The WwdGameReg per-level registry singleton
// (see the duplicate g_focusedGruntSentinel below; declared once at block end)
// --------------------------------------------------------------------------- (?g_gameReg @0x64556c). The grunt
// movement/arrival machines reach the level board via g_gameReg->m_tileGrid (a Board*),
// whose m_8 is the row-pointer table (rows[y][x] -> a 0x1c-byte tile record whose
// first dword carries the occupancy/flag bits) and m_c/m_10 the x/y in-bounds
// limits. Reloc-masked DATA; a struct (mangles `U`) gives the retail name.
// ---------------------------------------------------------------------------
SIZE_UNKNOWN(GruntBoard);
struct GruntBoard {
    char m_pad0[0x8];
    char** m_8; // +0x08  row table: m_8[y][x] -> tile record
    i32 m_c;    // +0x0c  x bound
    i32 m_10;   // +0x10  y bound
};
// The entrance-cell triple at CGrunt+0x43c: {col, row, reason}. Several CGrunt step
// methods take a by-value copy of it (GruntEntranceCell cell = *ptr) before indexing
// the m_cells table by 3*col+row; MSVC5 /O2 loads all three ints and dead-spills the
// unread `reason`, reserving `sub esp,0xc` (the frame several methods must reproduce).
SIZE(GruntEntranceCell, 0xc);
struct GruntEntranceCell {
    i32 col;
    i32 row;
    i32 reason;
};
// The per-effect sound-table object reached via g_gameReg->m_world (the sound
// category), whose m_28 holds a CMapStringToOb at +0x10 (the launch-sound lookup
// the struck-voice creator @0x57c40 queries). Same shape Projectile's LaunchSound
// reaches. All external (reloc-masked).
struct GruntSoundEntry; // map value: per-effect sound entry (factory at +0x10)
struct GruntSoundInner; // m_30->m_28: holds the lookup map at +0x10
// The on-screen viewport the movement-step spawn-cue gate reaches through the
// sound category: (world->m_24->m_5c + 0x40) is the visible CCueRect (m_5c is a
// base address, so +0x40 is a cast - the same CCueRect pattern).
SIZE_UNKNOWN(CGruntViewMid);
struct CGruntViewMid {
    char m_pad0[0x5c];
    i32 m_5c; // +0x5c  base address of the visible rect (-0x40)
};
SIZE_UNKNOWN(GruntSoundCat);
struct GruntSoundCat { // m_30: the sound-category / world-resource holder object
    char m_pad0[0x8];
    // +0x08  the sprite/object factory (CreateSprite @0x1597b0, <Gruntz/
    // SpriteFactory.h>) - the attack-fire step (UserLogicVfunc7, ProjectileUpdate.cpp)
    // spawns the projectile eye-candy sprites through it. Same slot GameRegistry.h's
    // CSpriteFactoryHolder models (this holder object is that facet's grunt view).
    struct CSpriteFactory* m_8;
    char m_padc[0x24 - 0xc];
    CGruntViewMid* m_24;   // +0x24  -> the visible-viewport gate (movement spawn cue)
    GruntSoundInner* m_28; // +0x28  -> the lookup map lives at (*m_28)+0x10
};
// WwdGameReg (the g_gameReg singleton) is the canonical <Gruntz/WwdGameReg.h>;
// its grunt-facet slot types (m_world=GruntSoundCat, m_cueSink=CGruntCueSink,
// m_tileGrid=GruntBoard, m_74=CSpriteRefTable) are completed by the defs above.
// g_gameReg decl moved to consumers (was WwdGameReg*; clashes with CGameRegistry* view) - include WwdGameReg.h + declare locally where needed

// The struck-voice sound model (creator @0x57c40). The lookup returns a
// GruntSoundEntry whose +0x10 sub-object owns a sample factory (GetItem @0x135d70,
// __thiscall ret 0 -> new sample); the sample is Play'd (0x136300) and lives in the
// grunt's +0x428 slot (freed by ClearSubB). All engine callees external/reloc-masked.
SIZE_UNKNOWN(GruntSoundEntry);
struct GruntSoundEntry {
    char m_pad0[0x10];
    CSoundCueMgr* m_10; // +0x10  the sample factory
};
SIZE_UNKNOWN(GruntSoundMap);
struct GruntSoundMap {}; // MFC CMapStringToOb (Lookup @0x1b8438); cast at the call
SIZE_UNKNOWN(GruntSoundInner);
struct GruntSoundInner {
    char m_pad0[0x10];
    GruntSoundMap m_10; // +0x10  the lookup map (call this->m_10.Lookup)
};

// The intrusive coord-node freelist the grunt machines recycle occupied-coord
// nodes onto (head @0x645544, bias @0x64554c) - the SAME pool g_freePoolHead/Base
// front; aliased here under the names the movement machines read. Reloc-masked.
extern void* g_gruntFreeList;   // DAT_00645544
extern i32 g_gruntFreeListBias; // DAT_0064554c

// The coord-node free pool (DAT_00645540): Recycle(elem) (FUN_004311b0, thunk
// 0x163b) pushes (elem - bias) onto the freelist headed at this->m_04. Reloc-masked
// DATA; modeled as a tiny object so `mov ecx,0x645540; push elem; call` falls out.
SIZE_UNKNOWN(GruntCoordPool);
extern FreeNodePool g_coordPool; // DAT_00645540 (folded GruntCoordPool)

// A grunt occupied-coord list node: ->next at +0, ->coord at +8 (an {x,y} pair).
SIZE_UNKNOWN(GruntCoord);
struct GruntCoord {
    i32 m_x; // +0x00
    i32 m_y; // +0x04
};
SIZE_UNKNOWN(GruntCoordNode);
struct GruntCoordNode {
    GruntCoordNode* m_next; // +0x00
    char m_pad4[0x4];
    GruntCoord* m_coord; // +0x08
};

// The "focused grunt" sentinel the on-screen flag compares m_tileOwnerHi against
// (DAT_00644c54, reloc-masked).
extern i32 g_focusedGruntSentinel; // DAT_00644c54

// The level board-dimension path the area cues read off the tile-mgr's +0x22c
// registry: m_22c -> m_24 -> m_5c -> {m_28 = width, m_2c = height}.
SIZE_UNKNOWN(CTileBoardDims);
struct CTileBoardDims {
    char m_pad0[0x28];
    i32 m_28; // +0x28  board width
    i32 m_2c; // +0x2c  board height
};
SIZE_UNKNOWN(CTileRegMid);
struct CTileRegMid {
    char m_pad0[0x5c];
    CTileBoardDims* m_5c; // +0x5c
};
SIZE_UNKNOWN(CTileReg);
struct CTileReg {
    char m_pad0[0x24];
    CTileRegMid* m_24; // +0x24
};

// ---------------------------------------------------------------------------
// The grunt's path/occupancy sub-manager (CGrunt+0x260). LoadEntranceConfig
// drives it through four engine thunks (all external/no-body, reloc-masked):
//   SetTile(a,b,c,d)     thunk_FUN_0046bcb0  (cell-owner mismatch notify; 4 args)
//   ClaimTile(a,b,c,d)   thunk_FUN_0046bfd0  (claim the new tile; 4 args)
//   ReleaseTile(a,b)     thunk_FUN_004784d0  (release on lookup miss; ret int)
//   PostWire()           the 0-arg wire call after the grid stamp (WireTileSwitchLogic)
// ---------------------------------------------------------------------------
// A node of the tile-mgr's live-grunt list (CGruntTileMgr::m_4 chain): m_next @+0,
// and @+8 the live-grunt entry (raw-offset col@0x54 / row@0x58 / busy@0x5c).
SIZE_UNKNOWN(CGruntLiveNode);
struct CGruntLiveNode {
    CGruntLiveNode* m_next; // +0x00
    char m_pad4[0x8 - 0x4];
    char* m_entry; // +0x08  live-grunt entry (raw offsets col/row/busy at +0x54/58/5c)
};

SIZE_UNKNOWN(CGruntTileMgr);
class CGruntTileMgr {
public:
    void SetTile(i32 a, i32 b, i32 c, i32 d);   // thunk_FUN_0046bcb0
    void ClaimTile(i32 a, i32 b, i32 c, i32 d); // thunk_FUN_0046bfd0
    i32 ReleaseTile(i32 a, i32 b);              // thunk_FUN_004784d0
    void PostWire();                            // WireTileSwitchLogic (0-arg)
    void NotifyArrival(i32 a, i32 b);           // thunk_FUN_0046da60 (2-arg)
    CGrunt* GetOccupant(CGrunt* g);             // FUN_00477df0 (1-arg, returns grunt)
    CGrunt* FindAtPixel(i32 x, i32 y);          // call 0x2b67 (grunt under a HUD pixel)
    i32 LookupTile(i32 x, i32 y, i32* outA, i32* outB, i32 flag); // FUN_00475af0 (ret 0x14)
    // The grunt anim-dispatch state machines drive the tile-mgr through two more
    // thunks (external/no-body, reloc-masked): a 6-arg arrival notify and a 4-arg
    // tile state set.
    void ArrivalNotify6(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f); // thunk (ret 0x18)
    void SetTileState4(i32 a, i32 b, i32 c, i32 d);                // thunk (ret 0x10)
    i32 ProbeFreeTile(i32 a, i32 b, void* c, i32 d, void* e, i32 f, i32 g, i32 h, i32 i); // probe
    // The grunt-step scan machines (WanderIdleStep / GruntArrivalScan / GruntUpdateStep)
    // reach two more operations, both already modeled above under other names (same
    // reloc-masked targets): FindGrunt (thunk 0x253b -> 0x477df0) IS GetOccupant, and
    // Scatter (thunk 0x14bf -> 0x6dae0) IS CommitTileSlot2. No new methods needed.
    // UpdateEntranceAnim's arrival-commit: thunk_0x3dfa (0x6c130), __thiscall on the
    // tile-mgr, the grunt + its last-tile pixel coords as args. Reloc-masked.
    void CommitArrivalMove(CGrunt* g, i32 x, i32 y);
    // ClaimSwitchTile's tile-mgr apply (thunk_0x26df -> 0x6d300 ApplySwitch),
    // __thiscall(grunt, lastX, lastY) ret 0xc. External/reloc-masked.
    void ApplyTileSwitch(CGrunt* g, i32 x, i32 y);
    // TryPowerupAtTile's tile-mgr probe (thunk_0x152d -> 0x7c620), 6 args ret 0x18.
    void ProbeMoveTile(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f);
    // The two big tile-mgr occupancy-commit helpers the arrival/update steps drive
    // (the in-flight-arrival path commits the grunt's occupied slot to a settled
    // position). thunk 0x3030 -> 0x6e120 (4-arg) and thunk 0x14bf -> 0x6dae0 (4-arg);
    // external/no-body so the calls reloc-mask.
    void CommitTileSlot(i32 ownerHi, i32 ownerLo, i32 px, i32 py); // 0x6e120
    // 0x6dae0 - returns -1 when the slot couldn't be committed (the reposition step
    // @0xec670 gates on != -1); other callers discard the result.
    i32 CommitTileSlot2(i32 ownerHi, i32 ownerLo, i32 px, i32 py); // 0x6dae0
    // FinishEntranceMove's tile-mgr drop notify (thunk_0x2a72 -> 0x79fb0), 3 args.
    void NotifyEntranceDrop(i32 ownerHi, i32 ownerLo, i32 flag); // 0x79fb0
    // The death/struck-reaction tile-mgr commit (thunk_0x10eb -> 0x78260), 3 args.
    void CommitStruckTile(i32 ownerHi, i32 ownerLo, i32 flag); // 0x78260
    // The DEATHZ finalize tile-notify at the grunt's HUD pos (thunk_0x290a -> 0x79ea0),
    // 3 args (px, py, m_38c). External/no-body (reloc-masked).
    void NotifyDeathTile(i32 px, i32 py, i32 c); // 0x79ea0
    // The run-start drop notify at the grunt's HUD pos (thunk_0x2fb3 -> 0x7b330), 4 args.
    void NotifyMoveAt(i32 px, i32 py, i32 a, i32 b); // 0x7b330
    // RunMoveConfig's I-pose tile load (thunk_0x3945 -> 0x75e90), 6 args. External.
    void Load6(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f); // 0x75e90
    // The spell-ability area cues (LoadGruntAbilityTuning): the 5-arg combat-area cue
    // (thunk 0x400c -> LoadGruntCombatTuning 0x7b930) + the 3-arg resurrect-area cue
    // (thunk 0x1fff -> LoadGruntResurrectTuning). External/no-body (reloc-masked).
    i32 CombatCue(i32 x, i32 y, i32 radius, i32 tier, i32 flag); // 0x400c (ret 0x14)
    i32 ResurrectCue(i32 x, i32 y, i32 radius);                  // 0x1fff (ret 0xc)
    // CombatCue's per-grid-cell effect for tiers 1/6/7 (thunk 0x2e96, __thiscall on
    // the tile-mgr, ret 0x10): apply the (i,j) cell effect with the tier value + flag.
    void ApplyCellEffect(i32 i, i32 j, i32 k, i32 flag); // 0x2e96

    // The tile-mgr's live-grunt list head (+0x04): the WanderIdle/ArrivalScan seek
    // walks it for the nearest targetable grunt. Its node is {next @+0, entry @+8};
    // the entry's tile col/row/busy live at raw +0x54/+0x58/+0x5c (a per-entry view
    // distinct from CGrunt's own layout), so the entry is left a raw pointer.
    char m_pad0[0x4];
    struct CGruntLiveNode* m_4; // +0x04  live-grunt list head
    char m_pad8[0x1c - 0x8];
    // CombatCue's grid (this+0x1c): a 4x15 grunt-pointer board scanned i=0..3, j=0..14
    // (the address is monotonic, so retail strength-reduces it to a running pointer).
    // Also the WanderIdle/UpdateStep move grid (indexed [col][row] = flat col*15+row).
    CGrunt* m_grid[4][15]; // +0x1c
    char m_pad10c[0x148 - 0x10c];
    i32 m_148;   // +0x148
    i32 m_14c;   // +0x14c
    void* m_150; // +0x150
    char m_pad154[0x18c - 0x154];
    i32 m_18c; // +0x18c
    char m_pad190[0x194 - 0x190];
    i32 m_194;      // +0x194
    i32 m_198;      // +0x198
    i32 m_19c;      // +0x19c
    i32 m_moveMode; // +0x1a0
    i32 m_1a4;      // +0x1a4
    char m_pad1a8[0x1dc - 0x1a8];
    i32 m_1dc; // +0x1dc
    i32 m_1e0; // +0x1e0
    char m_pad1e4[0x208 - 0x1e4];
    i32 m_208; // +0x208
    i32 m_20c; // +0x20c
    i32 m_210; // +0x210
    i32 m_214; // +0x214
    i32 m_218; // +0x218
    char m_pad21c[0x228 - 0x21c];
    i32 m_228;       // +0x228
    CTileReg* m_22c; // +0x22c  the level registry (board dims)
};

// The on-screen point-visibility predicate the arrival/update steps gate the cue
// on (FUN_0046b330, 0x6b330; __cdecl 3 args -> `add esp,0xc`): given a probe
// kind and the grunt's HUD point, returns whether it falls inside the live view
// rect. External/no-body (reloc-masked).
i32 GruntPointVisible(i32 px, i32 py, i32 cmp);

// The drop-ready predicate the per-tick move step calls (FUN_00429b40, thunk 0x1807,
// __stdcall(grunt) - callee-cleans (no `add esp,4` at the site); nonzero when the
// grunt cannot yet drop. External/reloc-masked.
i32 __stdcall GruntDropReady029b40(CGrunt* g);

// The registry focused-grunt slot the arrival gate reads is CFocusSlot, the
// canonical element of g_pGameRegistry->m_focusSlots[] (+0x150, stride 0x238),
// defined in <Gruntz/GameRegistry.h> (included above). The arrival path checks
// its +0x14 gate.

// ---------------------------------------------------------------------------
// The serialization sink CGrunt::Save drives: a custom archive whose vtable
// slot 0x30 is a `Write(const void* data, int size)` (member fn, thiscall).
// Modeled as a polymorphic class with 13 virtuals (slot 0x30 = the 13th) so
// each `mov edx,[ebx]; push size; push &field; mov ecx,ebx; call [edx+0x30]`
// falls out. The archive is external (never instantiated here, so no vtable is
// emitted); Write's body is reloc-masked.
// ---------------------------------------------------------------------------
SIZE_UNKNOWN(CGruntArchive);
// `struct` (not class) to match the UserLogic.h forward decl: MSVC mangles the
// type by its first-seen tag (struct -> U), so a `class` definition here made the
// clang label step emit V while wine cl emits U -> label MISS. Keep them uniform.
struct CGruntArchive {
    virtual void slot00();
    virtual void slot04();
    virtual void slot08();
    virtual void slot0c();
    virtual void slot10();
    virtual void slot14();
    virtual void slot18();
    virtual void slot1c();
    virtual void slot20();
    virtual void slot24();
    virtual void slot28();
    virtual void Read(void* data, i32 size);        // vtable slot +0x2c
    virtual void Write(const void* data, i32 size); // vtable slot +0x30
};

// A grunt-embedded sub-record serializer (the CGrunt move/timer state has several
// at +0x150/+0x278/+0x308/+0x43c/+0x890..+0x8c0). Each is a __thiscall(ar, mode,
// a3, a4) ret 0x10 reached through an incremental-link thunk; external/no-body so
// the `lea ecx,[this+N]; call rel32` reloc-masks.
// The grunt's name-id resolver the Save reaches via m_158->m_c->m_2c: maps an
// integer id to its name CString (returned by value). __thiscall, ret 4.
// The +0x158 "type catalog" object: Save reads its m_c (a non-null owner that
// also holds the name-id map at m_2c). External; modeled minimally.
SIZE_UNKNOWN(CGruntTypeCatalog);
struct CGruntTypeCatalog {
    char m_pad0[0xc];
    CDDrawSubMgrLeaf* m_c; // +0x0c  owner -> name-id map
};

// The global serialize counter Save bumps before each variable-length record
// (DAT_00629ad0). TU-local (reloc-masked); shared in retail.
extern i32 g_serialCounter;

// The linked-list node Save's tail walks (m_33c head): {next @+0, data @+0x8}.
SIZE_UNKNOWN(CGruntListNode);
struct CGruntListNode {
    CGruntListNode* m_next; // +0x00
    char m_pad4[0x8 - 0x4];
    void* m_data; // +0x08  serialized payload (0x2c bytes)
};

// The global running game clock (DAT_00645588) - already declared as g_645588
// in the .cpp; the Save serialize loop's name-table lookup helper.
class CArchive; // (unused MFC fwd; Save uses CGruntArchive)

// ---------------------------------------------------------------------------
// CGrunt::Load(ar) @0xd8060 support - the symmetric inverse of Save (each member
// read back via ar->Read, vtable slot +0x2c). It rebuilds the grunt's owned
// node-collections from the global coord free-list and re-resolves anim/object
// names through the resource manager (g_gameReg->m_world).
//
// The CObArray-family collections it rebuilds (this+0x370, the 4x stride-0x14
// group at this+0x3a4, this+0x488). Each is the engine CObArray {vtbl, m_data,
// m_count, m_max, m_grow} (0x14 bytes); SetSize/SetAtGrow are external (reloc-
// masked). The recycled nodes ride the same g_gruntFreeList pool as the movement
// machines (node usable area = head+4; head[0] = next).
SIZE_UNKNOWN(GruntLoadColl);
// SetSize @0x1b4f75 / SetAtGrow @0x1b5144 ARE MFC CPtrArray's; cast at each call.
struct GruntLoadColl {
    void SetSize(i32 n, i32 grow);    // 0x1b4f75
    void SetAtGrow(i32 idx, void* p); // 0x1b5144
    char _vft0[4]; // +0x00 foreign/base object vptr (reduced view; not owned/dispatched)
    void** m_data; // +0x04
    i32 m_count;   // +0x08
    i32 m_max;     // +0x0c
    i32 m_grow;    // +0x10
};

// The CString member the load streams a 0x200-byte buffer into (this+0x410);
// operator=(const char*) is external (0x1b9e74, reloc-masked).
SIZE_UNKNOWN(GruntLoadStr);
struct GruntLoadStr {
    // Assign @0x1b9e74 IS CString::operator=; cast at the call.
};

// The anim-name id table entry resolved through res->m_10's CMapStringToOb (+0x10,
// Lookup 0x1b8008): a range [m_64..m_68] and the id array at +0x14.
SIZE_UNKNOWN(GruntIdEntry);
struct GruntIdEntry {
    char m_pad0[0x14];
    i32* m_14; // +0x14  id array
    char m_pad18[0x64 - 0x18];
    i32 m_64; // +0x64  lo index
    i32 m_68; // +0x68  hi index
};
SIZE_UNKNOWN(GruntNameIdMap);
// GruntObjEntry / GruntObjMap moved above CSpriteFactory (the map is its +0x48
// embedded member); the declarations stay canonical there.
// The resource manager (g_gameReg->m_world): m_8 owns the object map, m_10 the
// sprite/name manager (with the CMapStringToOb at +0x10).
SIZE_UNKNOWN(GruntResMgr);
struct GruntResMgr {
    char m_pad0[0x8];
    char* m_8; // +0x08
    char m_pad0c[0x10 - 0xc];
    char* m_10; // +0x10
};
// The global DAT_00612618 dword the load streams a record into (reloc-masked).
extern i32 g_load612618;

// A small owned sub-object the grunt destroys on teardown (slots +0x424/+0x428).
// Free() is __thiscall, no args, reloc-masked.
SIZE_UNKNOWN(CGruntSub);
class CGruntSub {
public:
    // Free @0x69d60 IS CGrunt::LoadFreezeSpellAssets; cast at each call.
};

// ---------------------------------------------------------------------------
// ~CGrunt teardown support (the leaf dtor @0xf2f0). CGrunt is a real CUserLogic
// leaf (most-derived vtable 0x5e8754); the base chain CUserBase <- CUserLogic <-
// CGrunt is modeled polymorphic below, so the compiler AUTO-emits the three vptr
// restamps (CGrunt 0x5e8754 -> CUserLogic 0x5e705c -> CUserBase 0x5e70b4) and the
// per-member /GX trylevel teardown. CGrunt OWNS six destructible sub-objects torn
// down (in /GX trylevel order) before the base teardown folds in:
//   +0x468  CGruntCellRec[9] (stride 0x68) torn via the MSVC __ehvec_dtor iterator
//           with the per-element dtor 0x4023a6
//   +0x44c  ~CString (0x1b9cde)        +0x448  ~CString (0x1b9cde)
//   +0x338  ~CObList (0x1b48c6)        +0x31c  ~CObList (0x1b48c6)
//   +0x1c0  ~CString = m_animSetName (0x1b9cde)
//   +0x18   ~EngStr  (0x16d2a0) the CUserLogic link base teardown (in ~CUserLogic)
// All teardown callees are external/no-body (reloc-masked).

// The +0x468 owned-cell array element (9 x 0x68), one per direction. Five per-pose
// anim-name CStrings at +0/4/8/c/10 (ATTACK/STRUCK/WALK/IDLE/ITEM) the entrance
// name loader (LoadCellAnimNames) fills; the serialized-record dwords at +0x14/+0x40/+0x64
// the Load path streams. Its per-element ctor/dtor are engine callbacks (0x401e9c /
// 0x4023a6, external/reloc-masked); as a value-array member of CGrunt, MSVC auto-emits
// the __ehvec_ctor/__ehvec_dtor(base, 0x68, 9, &CGruntCellRec::{ctor,dtor}).
SIZE_UNKNOWN(CGruntCellRec);
struct CGruntCellRec {
    CString m_attack; // +0x00  "GRUNTZ_<name>_<DIR>_ATTACK"
    CString m_struck; // +0x04  "GRUNTZ_<name>_<DIR>_STRUCK"
    CString m_walk;   // +0x08  "GRUNTZ_<name>_<DIR>_WALK" / "GRUNTZ_<name>_<DIR>"
    CString m_idle;   // +0x0c  "GRUNTZ_<name>_<DIR>_IDLE"
    CString m_item;   // +0x10  "GRUNTZ_<name>_<DIR>_ITEM"
    i32 m_14;         // +0x14  (serialized record dword)
    i32 m_18;         // +0x18  (serialized record dword)
    i32 m_1c;         // +0x1c  (serialized record dword)
    i32 m_20;         // +0x20  (serialized record dword)
    i32 m_24;         // +0x24  (serialized record dword)
    i32 m_28;         // +0x28  (serialized record dword)
    i32 m_2c;         // +0x2c  (serialized record dword)
    i32 m_30;         // +0x30  (serialized record dword)
    i32 m_34;         // +0x34  (serialized record dword)
    char m_pad38[0x40 - 0x38];
    i32 m_40; // +0x40  (serialized record dword)
    i32 m_44; // +0x44  (serialized record dword)
    // The per-direction movement vector (abs +0x4b0.. as the "+0x4b0 dir-vector
    // table", cell index 3*col+row, stride 0x68): unit direction {m_dirX, m_dirY}
    // + half-tile step offsets {m_stepX, m_stepY}. Activate (@0x5caa0)
    // writes them as doubles ([ecx+13a*8+0x4b0..0x4c8]); the movement-integration
    // tail of MovingSlot16 (@0x5f310) reads all four. The serialize/load path
    // streams raw 4-byte halves of these (the (char*)+4 spellings in Load).
    double m_dirX;    // +0x48  unit direction X
    double m_dirY;    // +0x50  unit direction Y
    double m_stepX;   // +0x58  half-tile step X (+-0.5)
    double m_stepY;   // +0x60  half-tile step Y (+-0.5)
    CGruntCellRec();  // 0x401e9c (per-element ctor; the __ehvec_ctor callback)
    ~CGruntCellRec(); // 0x4023a6 (per-element dtor; reloc-masked)
};
// Each owned sub-object is torn down by its engine dtor reached __thiscall (this in
// ecx, no stack arg/cleanup). Modeled as a 1-method receiver so `lea ecx,[this+off];
// call` falls out, and as a real value member with `~T(){Dtor();}` so the /GX frame's
// per-member descending trylevel chain is what the compiler emits. The default ctor
// mirrors it (the CGrunt ctor member-inits each via the engine CString/CObList ctor,
// reloc-masked): CString() @0x1b9b93, CObList(nBlock=0xa) @0x1b4867.
SIZE_UNKNOWN(GruntStrSub);
struct GruntStrSub { // +0x44c / +0x448 / +0x1c0  (~CString 0x1b9cde)
    void CtorImpl(); // 0x1b9b93 (CString default ctor)
    void Dtor();
    GruntStrSub() {
        CtorImpl();
    }
    ~GruntStrSub() {
        Dtor();
    }
};
SIZE_UNKNOWN(GruntListSub);
struct GruntListSub {          // +0x338 / +0x31c  (~CObList 0x1b48c6)
    void CtorImpl(i32 nBlock); // 0x1b4867 (CObList ctor, block size)
    void Dtor();
    void RemoveAll();          // 0x1b48a6 (CObList::RemoveAll - empty in place, keep the object)
    void* Find1de8(void** it); // 0x1de8 (CObList iterate; was CMoveObList)
    // The step machines pop/push the occupied-coord CObList head directly (the
    // stored value IS the GruntCoord*). External/reloc-masked (MFC CObList slots).
    struct GruntCoord* RemoveHead(); // 0x1b4a03 (CObList::RemoveHead - returns the coord)
    void AddHead(void* p);           // 0x1b4967 (CObList::AddHead - push a coord)
    GruntListSub() {
        CtorImpl(0xa);
    }
    ~GruntListSub() {
        Dtor();
    }
};
// The +0x18 CUserLogic link is the shared CUserBaseLink (EngStr, ~EngStr 0x16d2a0)
// from <Gruntz/UserBaseLink.h> - identical sub-object to the tile-logic family's.

// A 10-virtual interface view for CGrunt::DispatchVtbl24's tail call (vtable
// slot 0x24 = index 9). Calling Slot9() emits `mov eax,[ecx]; jmp [eax+0x24]`.
SIZE_UNKNOWN(CVtSlot9);
class CVtSlot9 {
public:
    virtual void s0();
    virtual void s1();
    virtual void s2();
    virtual void s3();
    virtual void s4();
    virtual void s5();
    virtual void s6();
    virtual void s7();
    virtual void s8();
    virtual void Slot9();
};

// The name/animation cache collections UserLogicVfunc9 drains (sub-objects of CGrunt
// at +0x31c and +0x338). External engine collections; only the called methods
// are modeled (reloc-masked).
SIZE_UNKNOWN(CGruntColl);
// CGruntColl was a view of the m_31c CObList; Reset = CObList::RemoveAll (cast at each call).
SIZE_UNKNOWN(CGruntList);
// __cdecl node deleter (operator delete-style; push p; call; add esp,4).
void GruntNode_Delete(void* p);

// The global free-list pool the name caches recycle into.
extern void** g_freePoolHead; // DAT_00645544
extern i32 g_freePoolBase;    // DAT_0064554c (raw subtrahend)

// ---------------------------------------------------------------------------
// CGrunt::PlayMoveSound(x, y) @0x511b0 - the directional grunt-voice dispatcher.
// Computes the screen vector from the grunt's HUD center (m_10->m_5c/m_60) to
// (x, y), buckets it into one of 8 compass directions by the slope dy/dx vs the
// thresholds {+-2.0 (float), +-0.5 (double)}, and fires the matching grunt-voice
// record via the entrance handler @0x4ac10 (PlaySound below). Each direction is a
// 3-DWORD runtime-filled .data record {soundId, a, b}; PlaySound takes them by
// value plus a constant 1000 (0x3e8) range/volume. The 8 records + PlaySound are
// external/no-body (reloc-masked).
//
// The compass records (each 3 DWORDs at .data, runtime-filled). Modeled as 24
// individual i32 externs so each `mov ds:addr` reloc-masks against retail.
extern i32 g_voiceN[3];  // 0x6448e8  (dx==0, dy>0  -> South: down)
extern i32 g_voiceS[3];  // 0x6448d8  (dx==0, dy<0  -> North: up)
extern i32 g_voiceE[3];  // 0x6448c8  (shallow +, dx>0 -> East)
extern i32 g_voiceW[3];  // 0x6448f8  (shallow +, dx<0 -> West)
extern i32 g_voiceSE[3]; // 0x644928  (mid +, dx>0)
extern i32 g_voiceNW[3]; // 0x644918  (mid +, dx<0)
extern i32 g_voiceNE[3]; // 0x644908  (mid -, dx>0)
extern i32 g_voiceSW[3]; // 0x644948  (mid -, dx<0)

// The grunt-voice record passed by value to PlaySound (3 DWORDs). Building it
// from a named [3] record makes cl emit the 3 `mov ds:addr; mov [stk],reg` copies
// the target uses; passing it by value forces the `sub esp,0xc; ...; ret 0x10`.
SIZE_UNKNOWN(CGruntVoiceRec);
struct CGruntVoiceRec {
    i32 m_0;
    i32 m_4;
    i32 m_8;
};

// The {x, y} tile-coordinate pair GetTilePos (@0x31c70) writes its result into
// (the grunt's HUD pixel pos >> 5). Returned by pointer (the out arg).
SIZE_UNKNOWN(GruntTilePos);
struct GruntTilePos {
    i32 m_x; // +0x00
    i32 m_y; // +0x04
};

// ---------------------------------------------------------------------------
// CGrunt::StepCompassMove (@0x51c00) builds a small intrusive byte bag of the 8
// compass move-directions (1..8), then random-picks + tries each in turn. Modeled
// as a tiny CByteArray-style object {?, data@+4, count@+8} so the /GX-framed local
// + the SetAtGrow/RemoveAt/dtor calls fall out (all engine, external/reloc-masked).
// ---------------------------------------------------------------------------
SIZE_UNKNOWN(CToyTileBag);
class CToyTileBag {
public:
    CToyTileBag();                    // 0x1b527e (ctor)
    ~CToyTileBag();                   // 0x1b52b1 (dtor)
    void SetAtGrow(i32 idx, i32 val); // 0x1b5485 (append at idx, grow)
    void RemoveAt(i32 idx, i32 n);    // 0x1b5525 (remove n at idx)
    i32 m_0;                          // +0x00
    u8* m_data;                       // +0x04  byte data
    i32 m_count;                      // +0x08  element count
};

// ---------------------------------------------------------------------------
// CGrunt - only the members the HUD sprite creators touch. CGrunt is large;
// this is a deliberately partial model (load-bearing offsets only).
//   +0x10   m_10      CGruntHud* (factory geometry source)
//   +0x1b8  m_selectedSprite   (CreateSelectedSprite)
//   +0x1bc  m_toySprite        (CreateToySprite)
//   +0x1c4  m_healthSprite     (CreateHealthSprite)
//   +0x1c8  m_staminaSprite    (CreateStaminaSprite; ToyTime clears it)
//   +0x1cc  m_toyTimeSprite    (CreateToyTimeSprite; WingzTime clears it)
//   +0x1d0  m_wingzTimeSprite  (CreateWingzTimeSprite; ToyTime clears it)
//   +0x1d4  m_powerupSprite    (CreatePowerupSprite)
//   +0x1ec  m_tileOwnerHi / +0x1f0 m_tileOwnerLo  (Add* args)
//   +0x238  m_wingzEnabled     (WingzTime gate)
//   +0x3ec  m_health     (Health stat / Add arg)
//   +0x3f0  m_stamina     (Stamina stat / gate / Add arg)
//   +0x3f4  m_toyTime     (ToyTime gate / Add arg)
//   +0x3f8  m_wingzTime     (WingzTime gate / Add arg)
// ---------------------------------------------------------------------------
// CGrunt's game-object base chain (single inheritance; RTTI-recovered slot counts
// 3 / 16 / 17). Each level is polymorphic with a virtual dtor at slot 0, so the
// compiler auto-stamps the three vptrs (in ~CGrunt: CGrunt 0x5e8754 -> CUserLogic
// 0x5e705c -> CUserBase 0x5e70b4) and folds the base teardowns. Slot names are
// placeholders except the ones CGrunt defines in Grunt.cpp (slot 1 SerializeMove
// 0x53b80, slot 6 Activate 0x5caa0, slot 11 UserLogicVfunc9 0x48360, slot 16
// MovingSlot16 0x5f310); the rest are declared-only (impls external/reloc-
// masked). This is a CGrunt-local reconstruction of CUserBase/CUserLogic modeled
// at CUserLogic's TRUE 0x30 boundary: the base ctor 0x58cd0 inits only through
// +0x2c, and CGrunt's own byte-exact members start at +0x30, so the base is 0x30
// (NOT the fat 0x40 the tile-logic family's <Gruntz/UserLogic.h> view uses, which
// absorbs those leaves' shared 0x30..0x3c tail - a byte-neutral boundary label;
// see the size NOTE in UserLogic.h + docs/vtable-conversion-log.md). The +0x18
// EngStr link is the SHARED CUserBaseLink (<Gruntz/UserBaseLink.h>), so this world and
// the tile-logic world tear it down via the identical ~EngStr (0x16d2a0). The two
// CUserLogic class views still never coexist in one TU; the CGrunt-HUD sprites
// that read CGrunt fields (CGruntStaminaSprite/CGruntWingzTimeSprite) include only
// this header.
// size 0x18

// size 0x30

// ---------------------------------------------------------------------------
// CMovingLogic : CUserLogic - CGrunt's true moving-object base (RTTI vftable
// 0x5e87ac; the same base CProjectile derives from). It adds three virtuals over
// the CUserBase/CUserLogic chain but NO data of its own that shifts CGrunt's
// members (the motion band it initializes at +0x38 and the +0xa8 coordinate
// bounds overlay CGrunt's own named members). Its ctor is inlined into every leaf
// (here CGrunt::CGrunt), so it is modeled inline. Its leaf dtor is trivial and its
// most-derived vptr restamp dead-eliminates at /O2 (see CMovingLogicDtor.h), so
// ~CGrunt still folds CGrunt -> CUserLogic -> CUserBase unchanged.
//
// The CMotionState motion band embedded at +0x38 (reached via a cast so the CGrunt
// overlay layout stays put) + the shared default-bound doubles the ctor seeds.
SIZE_UNKNOWN(CGruntMotionBand);
struct CGruntMotionBand {
    void Init(); // 0x136d0 (CMotionState ctor; retail via thunk 0x34db)
    i32 SetParams(
        double a0,
        double a1,
        double a2,
        double a3,
        double a4,
        double a5,
        double a6,
        double a7,
        double a8,
        double a9,
        double a10
    );                   // 0x58bc0 (thunk 0x2ccf)
    void SetZ(double z); // 0x58ca0 (thunk 0x3ea9)
};
extern const double g_movingLogicMin;  // 0x5f04b0 (-2147483647.0)
extern const double g_movingLogicMax;  // 0x5f04b8 (2147483646.0)
extern const double g_gruntSpawnScale; // 0x5e9738 (spawn-seed velocity scale)
extern u32 g_5f04e8;                   // 0x5f04e8 (default-Z int)
extern u32 g_gruntSpawnClock;          // 0x645588 (spawn-seed clock; reloc-masked)

// ---------------------------------------------------------------------------
// CProjectile : CMovingLogic - the projectile game-object's expression in THIS
// (grunt) header chain. The CANONICAL full model lives in <Gruntz/Projectile.h>
// (which rides the sibling CUserLogic/CMovingLogic chain and cannot coexist in
// one TU with this header's - the documented dual-chain). Modeled here only as
// far as the attack-fire step (UserLogicVfunc7, ProjectileUpdate.cpp) needs it: the
// created "Projectile"/"Boomerang" sprite's aux (m_7c->m_18) setup object IS a
// CProjectile; the step dispatches its slot-17 LoadProjectileSprites (declared
// on CMovingLogic above) and, on failure, retires its +0x154 sprite. Never
// instantiated in this chain (dispatch-only; no vtable emitted).
// ---------------------------------------------------------------------------
class CProjectile; // canonical full model in <Gruntz/Projectile.h> (MFC-full); pointer-only here

// ---------------------------------------------------------------------------
// CGruntMovingBase - CGrunt's LEAN moving-object base: the CGrunt-world expression
// of CMovingLogic. It derives from the canonical 0x30 CUserLogic, adds EXACTLY the
// one new virtual (slot 16, MovingSlot16 / Update @0x16ea90) and NO data of its own,
// so CGrunt's own members land at their true +0x30.. offsets.
//
// The canonical CMovingLogic (<Gruntz/MovingLogic.h>) embeds the 0x108-byte
// CMotionState band + the twelve bound doubles as REAL members, making it a fat
// 0x150 class. That view is correct for CProjectile, but as CGrunt's base it pushes
// every CGrunt member +0x120 too far (m_400 -> +0x520, etc.), silently capping the
// whole CGrunt high-member/FP family - the +0x120 header-layout bug. The two cannot
// share the CMovingLogic name in the four dual-include TUs (Projectile/Boomerang/
// ProjectileUpdate/GruntEntranceArrival), so CGrunt rides its own lean base.
//
// The 1-arg ctor is inlined into CGrunt::CGrunt @0x47a10: it seeds the CTileLogic
// back-pointers, builds the +0x38 CMotionState band via Motion()->Init(), seeds the
// four coordinate bounds from the per-type config (m_14), and runs SetParams. The
// band + bounds are the real CMotionState fields, reached through the Motion()
// sub-object accessor (the blessed +0x38 cast the canonical CMovingLogic uses);
// they OVERLAY CGrunt's own named members (m_38/m_activeAnimDesc/m_animResolved/...),
// keeping CGrunt's overlay layout put (the same idiom CProjectile.cpp uses).
SIZE_UNKNOWN(CGruntMovingBase);
class CGruntMovingBase : public CUserLogic {
public:
    CGruntMovingBase(CGameObject* owner);
    virtual void MovingSlot16(); // slot 16 (offset 0x40) 0x16ea90 - the ONE new virtual
    CMotionState* Motion() {
        return (CMotionState*)((char*)this + 0x38);
    }
};

inline CGruntMovingBase::CGruntMovingBase(CGameObject* owner) : CUserLogic(owner) {
    // Build the +0x38 CMotionState band, then seed its per-axis bounds. The band +
    // the bounds physically OVERLAY CGrunt's own members (m_animResolved etc.); they
    // are the real CMotionState fields, reached through the Motion() sub-object
    // accessor (the same blessed +0x38 cast the canonical CMovingLogic uses), so no
    // raw-offset store is needed. Each bound: 0 => the shared MIN/MAX double copied
    // dword-wise; else the per-type-config int widened via fild (if/else, not ?:, so
    // the constant branch stays a mov/mov copy instead of a folded fld/fstp).
    CMotionState* m = Motion();
    m->Init();
    i32 lo0 = ((CProjBoundCfg*)m_objAux)->m_2c;
    if (lo0 == 0) {
        m->m_70 = g_movingLogicMin;
    } else {
        m->m_70 = (double)lo0;
    }
    i32 lo1 = ((CProjBoundCfg*)m_objAux)->m_34;
    if (lo1 == 0) {
        m->m_78 = g_movingLogicMin;
    } else {
        m->m_78 = (double)lo1;
    }
    i32 hi0 = ((CProjBoundCfg*)m_objAux)->m_30;
    if (hi0 == 0) {
        m->m_88 = g_movingLogicMax;
    } else {
        m->m_88 = (double)hi0;
    }
    i32 hi1 = ((CProjBoundCfg*)m_objAux)->m_38;
    if (hi1 == 0) {
        m->m_90 = g_movingLogicMax;
    } else {
        m->m_90 = (double)hi1;
    }
    m->SetParams(
        (double)m_object->m_screenX,
        (double)m_object->m_screenY,
        0.0,
        (double)m_object->m_164,
        (double)m_object->m_168,
        0.0,
        0.0,
        0.0,
        0.0,
        (double)g_645588 * g_gruntSpawnScale,
        0.0
    );
    m->SetZ((double)g_5f04e8);
}

// ---------------------------------------------------------------------------
SIZE_UNKNOWN(CGrunt);
class CGrunt : public CGruntMovingBase {
public:
    // vtable overrides in slot order (see the base chain above):
    virtual ~CGrunt() OVERRIDE; // slot 0  @0xf2f0
    virtual i32 SerializeMove(CGruntArchive* ar, i32 mode, i32 a3, i32 a4)
        OVERRIDE; // slot 1  @0x53b80
    RVA(0x0000f2a0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_GRUNT;
    } // slot 2  (0xf2a0)
    virtual i32 UserLogicVfunc1() OVERRIDE; // slot 3  (0x5d210)
    virtual i32 UserLogicVfunc2() OVERRIDE; // slot 4  (0x5bcd0)
    // RunAct (0x5bcd0): the class's vtable slot-4 (UserLogicVfunc2) activation
    // dispatcher body - a plain method (the no-arg UserLogicVfunc2() base placeholder
    // blocks the int-arg OVERRIDE spelling). Resolves `id`'s handler in the per-class
    // registry g_reg_644af0 and dispatches it as a PMF on `this`; else returns the
    // entry pointer. Same archetype as CPathHazard::RunAct.
    i32 RunAct(i32 id);
    virtual i32 UserLogicVfunc3() OVERRIDE; // slot 5  (0x5ecd0)
    virtual i32 Activate() OVERRIDE;        // slot 6  @0x5caa0
    virtual i32 UserLogicVfunc6() OVERRIDE; // slot 8  (0x62b40)
    virtual i32 UserLogicVfunc7() OVERRIDE; // slot 9  @0x61cb0 (attack-fire step)
    // slot 9 @0x61cb0 - the per-frame ATTACK-FIRE step (defined in
    // ProjectileUpdate.cpp): ticks the attack anim; at the fire cue spawns the
    // ranged projectile ("Projectile"/"Boomerang"/"TimeBomb" by tool kind) or
    // delivers the melee hit to the neighbor-cell grunt, then applies the
    // "AttackDowntime" timer. Returns 0.
    virtual i32 UserLogicVfunc9() OVERRIDE; // slot 11 @0x48360
    virtual void MovingSlot16() OVERRIDE;   // slot 16 @0x5f310

    i32 CreateHealthSprite();
    i32 CreateToySprite();
    i32 CreateStaminaSprite();
    i32 CreateToyTimeSprite();
    i32 CreateWingzTimeSprite();
    i32 CreatePowerupSprite(i32 a); // (ret 4)
    i32 CreateSelectedSprite();

    void ReadConfigFromButeMgr();
    i32 LoadGruntMovingDeathConfig();
    void LoadAnimNameTable(i32 a, i32 b); // @0x49c60 (ret 8)
    // @0x51850 (ret 8) tile-rect predicate; reconstruction deferred to the final
    // sweep (a register-relative rect-walk regalloc wall - cl folds this+const to
    // absolute loads, overshooting 0x165 B). Called external/reloc-masked here.
    i32 RectContains(i32 x, i32 y);
    i32
    RectContainsGated(i32 x, i32 y); // @0x51a20 (ret 8) sibling; m_198 gate, rects +0x2b0/+0x2c0
    i32 CommitNeighbor(i32 a, i32 b, i32 c, i32 d); // @0x5b050 (ret 0x10)
    CGrunt* FindGridNeighbor(i32 validate);         // @0x5b6f0 (ret 4)
    i32 UpdateGruntStatus();                        // @0x617c0 (ret 0)
    // @0x51c00 (ret 0, /GX) - the per-tick compass-move driver: resolves the grunt's
    // next move tile by the 8-way direction code (m_444), tests/stamps the board
    // occupancy + owner, fires the matching compass grunt-voice record, and commits
    // the move/arrival. Big switch + /GX EH frame + grid raw-offset state machine.
    i32 StepCompassMove();
    // @0x692f0 (ret 0) - the death/struck reaction dispatch: gated on m_1fc, resolves
    // the current anim name + dispatches on its type code (A/D/I/G/L/P/O/J/N/M), then
    // runs the shared arrival/clear-sprites/DEATHZ_FREEZE finalize tail.
    i32 StepArrivalCommit();
    // @0x65630 (2-arg this-method the I-arm of CommitNeighbor runs).
    void RunMoveConfig(i32 a, i32 b);
    // @0x641b0 - tears down the grunt's HUD sprites + plays the "GRUNTZ_EXITZ" exit
    // animation (rand-bucketed ONE/TWO/THREE variant + on-screen cue). __thiscall ret 0.
    i32 BuildGruntExitAnimation();
    // @0x63db0 - (re)loads the vehicle-grunt (gokart/bigwheel) entrance animation set.
    void LoadVehicleGruntAnimations();

    // --- GruntAssetLoaders.cpp cluster (mechanical asset/sprite/tuning loaders) ---
    // @0x68880 (ret 4, /base) - (re)load the wingz-grunt's per-direction sprite
    // name cells (flying ITEM set when enabled, WALK/IDLE set when disabled), the
    // pose-index lookups, then re-stamp the current entrance-cell frame.
    i32 LoadWingzGruntSprites(i32 enable);
    // @0x57100 (ret 4, /base) - the spell-ability tuning loader: fire the attack
    // sound cue, then dispatch on the (random or forced) ability index to build
    // the matching LightFx/rolling-ball effect + tuning.
    i32 LoadGruntAbilityTuning(i32 forced);
    // @0x60150 (ret 8) - the grunt death dispatch: tear down the running anim state,
    // retire the HUD sprites, latch the "C" death anim-set, then switch on the death
    // type to resolve + apply the matching GRUNTZ_DEATHZ_* sprite + cue.
    i32 LoadGruntDeathAnimations(i32 deathType, i32 a2);
    // @0x65e80 (ret 0x14, /base) - the pickup/powerup entrance-sprite loader: gate on
    // grunt-kind/entrance state, bump the per-owner pickup stats, latch the "J" anim-set,
    // then a ~90-way switch on the pickup type resolves the matching GRUNTZ_PICKUPS_*
    // sprite (megaphone runs a 2nd unit-type switch) + fires the on-screen entrance cue.
    i32 LoadPickupSprites(i32 type, i32 a2, i32 a3, i32 a4, i32 a5);
    // Pickup-loader helper this-methods (reloc-masked engine thunks).
    void PickupResetA();                    // thunk 0x214e (__thiscall ret 0)
    void PickupResetB(i32 a, i32 b, i32 c); // thunk 0x136b (__thiscall ret 0xc)

    // @0x57890 (__thiscall ret 0, /GX) - when the entrance reason is a lose-item
    // pose (0x12/0x16/0xe), spawn the one-shot "SingleAnimation" GRUNTZ_<set>_LOSEITEM
    // sprite, fire the on-screen spawn cue, then re-run the type-table step.
    i32 BuildGruntLoseItemAnimation();
    // The big CUserLogic-base step driver reached via thunk 0x3bd9 -> 0x4dd50
    // (LoadGruntTypeTable / SelfImpact); external/reloc-masked here. Returns i32
    // (InGameIcon reads the result; return type is mangling-neutral).
    i32 LoadGruntTypeTable(i32 a, i32 b, i32 c, i32 d);
    // @0x50ca0 (RunEntranceMove tail): reload the type table for `typeId` then reset
    // the move-mode pair (m_moveMode=-1, m_1a4=0). Re-homed from Stub.
    void LoadTypeTableClearMove(i32 typeId);

    // --- arrival / move-step helper cluster (proximity-attributed targets) ---
    void PlayMoveSoundAtTile(i32 tx, i32 ty); // @0x514e0 (ret 8) tile->pixel + PlayMoveSound
    void SnapToLastTile(i32 a);               // @0x517b0 (ret 4) snap m_10 to last tile + commit
    i32 ClaimSwitchTile();                    // @0x52c70 (ret 0) switch-dir tile claim
    void SetArrivalTarget(i32 a, i32 b, i32 c, i32 d); // @0x52ed0 (ret 0x10)
    void ConsiderArrival(i32 a);                       // @0x52f40 (ret 4) arrival/drop gate
    void SelectMoveIcon(i32 a);                        // @0x57800 (ret 4) pick move-cursor icon
    i32 TryPowerupAtTile();                            // @0x57aa0 (ret 0) probe move tile
    // @0x57db0 (ret 0, /GX) - the per-tick grunt path-cell scan (GruntPathScan.cpp):
    // 5x5 dirty box, tracked-coord list scan firing the plane trigger, freelist recycle.
    i32 PathScan57db0();

    // --- animation resolvers (this TU's targets) ---
    i32 ResolveMovingAnimation();
    i32 ResolveDeathAnimation();
    void NotifyFortUnderAttack(); // 0x45270 (reloc-masked)
    i32 ResolveAnimation();       // (generic / "_JOY")
    i32 ResolveIdleAnimation();
    i32 ResolveBattlecryAnimation();

    // @0x4a9f0 (ret 0) - the 4-way reachability probe: resolve the grunt under the
    // HUD center (m_tileMgr->FindAtPixel), copy its entrance rect (+0x144) offset by
    // its HUD origin, then test 4 segments (vertical / horizontal / two diagonals,
    // +-1000 px) through the grunt's own HUD center; return 1 on the first hit.
    i32 winapi_04a9f0_CopyRect_OffsetRect();
    i32 RectSegProbe(void* r, void* a, void* b); // call 0x4138 (__thiscall, 3 args)

    // Data members. vptr(+0), m_10(+0x10), m_14(+0x14) are in CUserBase; the +0x18
    // EngStr link is CUserLogic::m_18. CGrunt's own members begin at +0x30.
    // +0x30 opaque anim-set node handle (m_14->m_1c). The shared CUserLogic base
    // (<Gruntz/UserLogic.h>) authoritatively types this +0x30 slot void*; it is a
    // bute-tree node token passed straight to g_animNameResolver.GetNameRecord(void*),
    // so void* is the authentic recovered type (not a placeholder).
    void* m_prevAnimSetNode; // +0x30  (saved old m_14->m_1c before re-latch)
    char m_pad34[0x38 - 0x34];
    CGruntAnimState* m_38; // +0x38  (animation player)
    char m_pad3c[0x40 - 0x3c];
    CAniElement* m_activeAnimDesc; // +0x40  (cached m_38->m_1b4)
    char m_pad44[0x54 - 0x44];
    // +0x54 grunt-type name. Stored as a raw CString body (a single char* -
    // m_pszData) so ~CGrunt does NOT auto-destruct it (retail's leaf dtor tears
    // down only the six members below, NOT +0x54); viewed as a CString via
    // TypeName() at its five concat sites (codegen-neutral: same CString lvalue).
    void* m_typeName; // +0x54  (grunt-type CString body; not owned by ~CGrunt)
    CString& TypeName() {
        return *(CString*)&m_typeName;
    }
    i32 m_idleGeoSrc[(0x68 - 0x58) / 4];      // +0x58  (Idle geometry sources)
    i32 m_battlecryGeoSrc[(0x74 - 0x68) / 4]; // +0x68  (Battlecry geometry sources)
    i32 m_joyGeoSrc;                          // +0x74  (generic/_JOY geometry source)
    i32 m_deathGeoSrc;                        // +0x78  (death geometry source)
    i32 m_movingGeoSrc;                       // +0x7c  (moving geometry source)
    char m_pad80[0x88 - 0x80];
    i32 m_moveSeed;      // +0x88  (moving: = g_movingSeed)
    i32 m_moveTimeHi;    // +0x8c  (moving: = 0)
    i32 m_moveStartTime; // +0x90  (moving: randomized time)
    i32 m_moveSeedHi;    // +0x94  (moving: = 0)
    char m_pad98[0xa8 - 0x98];
    // +0xa8..+0xd0: the CMovingLogic base ctor overlays a movement-bound box here
    // (minX@0xa8, minY@0xb0, maxX@0xc0, maxY@0xc8, doubles); CGrunt reuses the
    // +0xa8 dword pair as the resolve gate + cue arg. The base ctor writes these via
    // raw offsets by necessity (a CUserLogic-derived base cannot name CGrunt members
    // without shifting CGrunt's layout), so they stay raw - authentic, not a hack.
    i32 m_animResolved; // +0xa8  (resolve gate / dirty flag; == moveMinX double lo)
    i32 m_deathCueArg;  // +0xac  (cue arg; == moveMinX double hi)
    char m_padb0[0x148 - 0xb0];
    i32 m_148;                       // +0x148
    i32 m_14c;                       // +0x14c
    void* m_150;                     // +0x150
    CEntranceAnimPlayer* m_154;      // +0x154 (entrance animation player)
    struct CGruntSndResMgr* m_158;   // +0x158 (ability/sound resource mgr)
    CAniElement* m_prevEntranceDesc; // +0x15c (= m_154->m_1b4 cache)
    char m_pad160[0x170 - 0x160];
    // +0x170 (entrance-reason / movement state). The attack-fire step (UserLogicVfunc7)
    // reads this slot as the grunt's current TOOL/attack kind (switched over the
    // GRUNTZ tool ids 2=Boomerang/9=Gunhat/10=Nerfgun/11=Rock/17=TimeBomb/
    // 21=Welder/22=Wingz; >0x16 = melee) and forwards it as the projectile kind -
    // the slot multiplexes the current-action kind; reconcile the name when the
    // entrance machines' reading is re-verified.
    i32 m_entranceReason;
    i32 m_entrancePxX; // +0x174 (SetEntrancePos: committed entrance position X, pixel)
    i32 m_entrancePxY; // +0x178 (SetEntrancePos: committed entrance position Y, pixel)
    i32 m_lastTilePxX; // +0x17c (LoadEntranceConfig: last occupied tile X, pixel; -1 = none)
    i32 m_lastTilePxY; // +0x180 (LoadEntranceConfig: last occupied tile Y, pixel; -1 = none)
    i32 m_commitPxX; // +0x184 (committed position snapshot X, pixel; = m_lastTilePxX after switch)
    i32 m_commitPxY; // +0x188 (committed position snapshot Y, pixel; = m_lastTilePxY after switch)
    i32 m_18c;       // +0x18c
    i32 m_toyBlendPct;             // +0x190 (anim-name loader: TOY1/TOY2 blend percent)
    i32 m_194;                     // +0x194
    i32 m_198;                     // +0x198
    i32 m_19c;                     // +0x19c
    i32 m_moveMode;                // +0x1a0
    i32 m_1a4;                     // +0x1a4
    i32 m_1a8;                     // +0x1a8 (serialized)
    i32 m_1ac;                     // +0x1ac (serialized)
    i32 m_1b0;                     // +0x1b0 (serialized)
    i32 m_1b4;                     // +0x1b4 (serialized)
    CHudSprite* m_selectedSprite;  // +0x1b8
    CHudSprite* m_toySprite;       // +0x1bc
    CString m_animSetName;         // +0x1c0  (anim-name loader: "GRUNTZ_"+m_animSetName+...)
    CHudSprite* m_healthSprite;    // +0x1c4
    CHudSprite* m_staminaSprite;   // +0x1c8
    CHudSprite* m_toyTimeSprite;   // +0x1cc
    CHudSprite* m_wingzTimeSprite; // +0x1d0
    CHudSprite* m_powerupSprite;   // +0x1d4
    i32 m_arrived;                 // +0x1d8 (entrance-arrival gate)
    i32 m_1dc;                     // +0x1dc
    i32 m_1e0;                     // +0x1e0
    i32 m_entranceActive;          // +0x1e4 (entrance: set to 1)
    i32 m_arrivalPending;          // +0x1e8 (SnapToLastTile/ClaimSwitchTile arrival-commit latch)
    i32 m_tileOwnerHi;             // +0x1ec
    i32 m_tileOwnerLo;             // +0x1f0
    i32 m_1f4_moveIcon;            // +0x1f4 (SelectMoveIcon: clamped icon index, [0,0x11))
    i32 m_1f8;                     // +0x1f8 (serialized)
    i32 m_entranceCommitted;       // +0x1fc (entrance: cleared)
    i32 m_neighborCol;             // +0x200 (grid-neighbor: column, -1 = none)
    i32 m_neighborRow;             // +0x204 (grid-neighbor: row, -1 = none)
    i32 m_208;                     // +0x208
    i32 m_20c;                     // +0x20c
    i32 m_210;                     // +0x210
    i32 m_214;                     // +0x214
    i32 m_combatActive; // +0x218 (combat/attack-active latch; set with m_poweredUp on attack, gates stamina display)
    i32 m_neighborValid;   // +0x21c (grid-neighbor: cleared on miss)
    i32 m_poweredUp;       // +0x220 (powered-up gate; 0 = run entrance reset)
    i32 m_224;             // +0x224 (serialized)
    i32 m_entranceStamped; // +0x228 (one-time entrance/toy-break stamp latch; set 1 at rest)
    i32 m_22c;             // +0x22c (entrance-drop: latched anim re-init gate)
    i32 m_arrivalActive; // +0x230 (arrival commit armed; seeded 1 w/ defender block, gates commit)
    i32 m_coordToggle;   // +0x234 (parity bit toggled per coord commit; arg to OnCoordCommit)
    i32 m_wingzEnabled;  // +0x238
    i32 m_freezeDelayDone; // +0x23c (freeze finalize: 0 = sparkle-delay window running, 1 = elapsed)
    i32 m_freezeUnfrozen;  // +0x240 (freeze finalize: set 1 when DEATHZ_UNFREEZE applied)
    i32 m_resetApplied;    // +0x244 (entrance-reset: 0 then 1 = "applied" flag)
    i32 m_arrivalFlags;    // +0x248 (arrival flag word; |= 0x18040402)
    i32 m_24c;             // +0x24c
    i32 m_250;             // +0x250 (serialized)
    i32 m_254;             // +0x254 (serialized)
    i32 m_gruntKind;       // +0x258 (grunt type/kind; ==0x37 -> halve TimePerTile)
    i32 m_entranceArmed;   // +0x25c (entrance: set to 1)
    CGruntTileMgr* m_tileMgr; // +0x260 (path/occupancy sub-manager)
    i32 m_struckCount;        // +0x264 (struck-reaction counter; cue tier 5/0xa)
    i32 m_struckClockLo;      // +0x268 (= g_645588 game clock at last struck)
    i32 m_struckClockHi;      // +0x26c (= 0)
    i32 m_struckTimerLo;      // +0x270 (= 0xfa0 struck cooldown window)
    i32 m_struckTimerHi;      // +0x274 (= 0)
    i32 m_278;                // +0x278
    i32 m_27c;                // +0x27c
    i32 m_280;                // +0x280
    i32 m_284;                // +0x284
    i32 m_288;                // +0x288 (serialized)
    i32 m_28c;                // +0x28c (serialized)
    // The grunt's reach/collision bounds rect (tile-space {left,top,right,bottom};
    // RectContains reads it via &m_reachRectLeft). m_reachRadius is the rect's right
    // edge AND a scalar reach radius (GruntInRadius radius-sum). GruntTubeAnim seeds
    // it {-1,-1,1,1}.
    i32 m_reachRectLeft;   // +0x290
    i32 m_reachRectTop;    // +0x294
    i32 m_reachRadius;     // +0x298 (grunt reach/collision radius; also reach-rect right edge)
    i32 m_reachRectBottom; // +0x29c
    i32 m_2a0;             // +0x2a0
    i32 m_2a4;             // +0x2a4
    i32 m_2a8;             // +0x2a8
    i32 m_2ac;             // +0x2ac
    i32 m_2b0;             // +0x2b0
    i32 m_2b4;             // +0x2b4
    i32 m_2b8;             // +0x2b8
    i32 m_2bc;             // +0x2bc
    i32 m_2c0;             // +0x2c0
    i32 m_2c4;             // +0x2c4
    i32 m_2c8;             // +0x2c8
    i32 m_2cc;             // +0x2cc
    i32 m_arrivalState;    // +0x2d0 (arrival: = 4)
    i32 m_defenderState;   // +0x2d4 (arrival defender dispatch sub-state: switch 0/1/2/3)
    i32 m_2d8;             // +0x2d8
    i32 m_defenderRadius;  // +0x2dc (defender radius / arrival kind)
    i32 m_2e0;             // +0x2e0 (serialized)
    i32 m_2e4;             // +0x2e4 (serialized)
    i32 m_2e8;             // +0x2e8 (serialized)
    i32 m_dwell;           // +0x2ec
    i32 m_arrivalCol;      // +0x2f0 (arrival target grid col; grid index = 15*col+row; -1 = none)
    i32 m_arrivalRow;      // +0x2f4 (arrival target grid row)
    i32 m_2f8;             // +0x2f8
    i32 m_2fc;             // +0x2fc
    i32 m_defenderX;       // +0x300 (arrival: = m_lastTilePxX)
    i32 m_defenderY;       // +0x304 (arrival: = m_lastTilePxY)
    i32 m_arrivalRerollLo; // +0x308 (arrival re-roll idle timer: anchor clock lo; i64 w/ m_arrivalRerollHi)
    i32 m_arrivalRerollHi;       // +0x30c (arrival re-roll idle timer: anchor clock hi)
    i32 m_arrivalRerollWindowLo; // +0x310 (arrival re-roll idle window lo = GruntRand()%0x7530 + 0x7530)
    i32 m_arrivalRerollWindowHi; // +0x314 (arrival re-roll idle window hi)
    i32 m_318;                   // +0x318
    GruntListSub m_31c;          // +0x31c (~CObList 0x1b48c6; destructed by ~CGrunt)
    char m_pad31d[0x320 - 0x31d];
    GruntCoordNode* m_320; // +0x320  (occupied-coord CObList head)
    GruntCoordNode* m_324; // +0x324  (occupied-coord CObList tail node)
    i32 m_coordCount;      // +0x328 (occupied-coord list count; non-empty gate for the m_320 list)
    char m_pad32c[0x338 - 0x32c];
    GruntListSub m_338; // +0x338 (~CObList 0x1b48c6; destructed by ~CGrunt)
    char m_pad339[0x33c - 0x339];
    CGruntListNode* m_33c; // +0x33c
    char m_pad340[0x344 - 0x340];
    void* m_344; // +0x344
    char m_pad348[0x354 - 0x348];
    i32 m_354;       // +0x354 (serialized)
    i32 m_358;       // +0x358
    i32 m_35c;       // +0x35c
    i32 m_deathType; // +0x360 (last LoadGruntDeathAnimations kind; serialized w/ m_364)
    i32 m_entranceDropActive; // +0x364 (entrance-drop safe period active; set 1 on drop commit, gates combat reaction)
    i32 m_deathAnimStarted; // +0x368 (death-animation-started latch)
    i32 m_36c;              // +0x36c
    i32 m_370;             // +0x370 (death-notify arg a2; serialized; also a CObArray view in Load)
    i32 m_374;             // +0x374
    i32 m_378;             // +0x378 (serialized)
    i32 m_moveKind;        // +0x37c (move-kind fallback; randomized if 0, arg to ApplyMoveKind)
    i32 m_moveVariant;     // +0x380 (selected entrance/move variant index 1..N; from m_374 or rand)
    i32 m_coordRetryCount; // +0x384 (MovingSlot16: head-coord tile-claim retry budget, <=5)
    i32 m_toyTileIndex;    // +0x388 (toy-tile index; gated < Grunt ToyTiles config count)
    i32 m_38c;             // +0x38c
    i32 m_390;             // +0x390
    // The per-pose animation-name index table (LoadAnimNameTable @0x49c60 fills
    // it from "GRUNTZ_"+m_animSetName+"_<POSE>" lookups). The entrance code reads
    // the IDLE1/2/3 slots (m_poseIdle/m_3b0/m_3b4) as its geometry-source triple.
    i32 m_poseWalk;       // +0x394 (_WALK)
    i32 m_poseAttack1;    // +0x398 (_ATTACK1)
    i32 m_poseAttack2;    // +0x39c (_ATTACK2)
    i32 m_poseAttackIdle; // +0x3a0 (_ATTACK-IDLE)
    i32 m_poseStruck1;    // +0x3a4 (_STRUCK1)
    i32 m_poseStruck2;    // +0x3a8 (_STRUCK2)
    i32 m_poseIdle[3];    // +0x3ac (_IDLE1/2/3) (entrance geometry-source triple [0..2])
    i32 m_poseIdle4;      // +0x3b8 (_IDLE4)
    i32 m_poseIdle5;      // +0x3bc (_IDLE5)
    i32 m_poseDeath;      // +0x3c0 (_DEATH)
    i32 m_poseToy1;       // +0x3c4 (_TOY1)
    i32 m_poseToy2;       // +0x3c8 (_TOY2)
    i32 m_poseToyBreak;   // +0x3cc (_TOY-BREAK)
    i32 m_poseItem;       // +0x3d0 (_ITEM)
    i32 m_poseItem2;      // +0x3d4 (_ITEM2)
    i32 m_pickupGeoSrc; // +0x3d8  (LoadPickupSprites: resolved pickup-sprite handle / SetGeometry source; serialized by name)
    i32 m_3dc;          // +0x3dc (serialized)
    i32 m_3e0;          // +0x3e0 (serialized)
    i32 m_moveTileX; // +0x3e4 (destination tile X; ArrivalNotify6/Load6 arg, = PlayMoveSoundAtTile x)
    i32 m_moveTileY; // +0x3e8 (destination tile Y; ArrivalNotify6/Load6 arg, = PlayMoveSoundAtTile y)
    i32 m_health;    // +0x3ec
    i32 m_stamina;   // +0x3f0
    i32 m_toyTime;   // +0x3f4
    i32 m_wingzTime; // +0x3f8
    char m_pad3fc[0x400 - 0x3fc];
    double m_400;      // +0x400
    double m_408;      // +0x408
    double m_410;      // +0x410
    i32 m_418;         // +0x418
    i32 m_timePerTile; // +0x41c (TimePerTile config; ComputeFacing time divisor; halved for kind 0x37)
    i32 m_tileClaimed;                // +0x420 (arrival-claimed latch)
    CGruntSub* m_424;                 // +0x424
    CGruntSub* m_428;                 // +0x428
    i32 m_42c;                        // +0x42c
    i32 m_430;                        // +0x430
    i32 m_434;                        // +0x434
    i32 m_438;                        // +0x438
    GruntEntranceCell m_entranceCell; // +0x43c (entrance-cell triple {col, row, reason})
    GruntStrSub m_448;                // +0x448 (~CString 0x1b9cde; destructed by ~CGrunt)
    char m_pad449[0x44c - 0x449];
    GruntStrSub m_44c; // +0x44c (~CString 0x1b9cde; destructed by ~CGrunt)
    char m_pad44d[0x450 - 0x44d];
    i32 m_arrivalPhase;    // +0x450 (arrival/update dispatch phase: 2 = in-flight, 3 = committing)
    i32 m_454;             // +0x454 (serialized)
    i32 m_458;             // +0x458 (serialized)
    i32 m_45c;             // +0x45c (serialized)
    i32 m_lowStaminaCued;  // +0x460 (low-stamina off-screen cue latch)
    i32 m_arrivalNotified; // +0x464 (entrance-reset latch flag)
    // +0x468 owned-cell array (9 x 0x68, +0x468..+0x810; entrance-cell record table,
    // 0x68-byte stride). Value array so ~CGrunt auto-emits the __ehvec_dtor teardown.
    CGruntCellRec m_cells[9]; // +0x468..+0x810  (per-direction anim-name cell records)
    i32 m_toyClockLo;         // +0x810 (toy timer: anchor clock lo; i64 w/ m_toyClockHi)
    i32 m_toyClockHi;         // +0x814 (toy timer: anchor clock hi)
    i32 m_toyDurationLo;      // +0x818 (toy timer: duration lo)
    i32 m_toyDurationHi;      // +0x81c (toy timer: duration hi)
    i32 m_idleAnchorLo;       // +0x820 (idle-timer: low)
    i32 m_idleAnchorHi;       // +0x824 (idle-timer: high)
    i32 m_idleDelayLo;        // +0x828 (idle-timer: delay low)
    i32 m_idleDelayHi;        // +0x82c (idle-timer: delay high)
    i32 m_idleTimerLo;        // +0x830 (idle-anchor: low)
    i32 m_idleTimerHi;        // +0x834 (idle-anchor: high)
    i32 m_idleWindowLo;       // +0x838 (idle-window: low = 0x3a98)
    i32 m_idleWindowHi;       // +0x83c (idle-window: high = 0)
    i32 m_entranceClockLo;    // +0x840 (entrance: = g_645588 game clock, low dword)
    i32 m_entranceClockHi;    // +0x844 (entrance: = 0, high dword)
    i32 m_entranceSafeTimeLo; // +0x848 (entrance: = EntranceSafeTime config)
    i32 m_entranceSafeTimeHi; // +0x84c (entrance: = 0)
    i32 m_850;                // +0x850 (timer record base, SerializeMove)
    i32 m_854;                // +0x854
    i32 m_858;                // +0x858 (entrance: = 0)
    i32 m_85c;                // +0x85c (entrance: = 0)
    // +0x860..+0x86f: the attack-downtime timer record (same {clock i64, duration
    // i64} shape as the combat/wingz timers below; SerializeMove round-trips it
    // from m_860). The attack-fire step (UserLogicVfunc7) stamps it at each impact:
    // clock = g_645588 (lo) / 0 (hi), duration = "AttackDowntime" bute (lo) / 0.
    i32 m_860;              // +0x860 (attack timer: anchor clock lo = g_645588)
    i32 m_864;              // +0x864 (attack timer: anchor clock hi = 0)
    i32 m_attackDowntimeLo; // +0x868 (attack timer: duration lo = AttackDowntime config)
    i32 m_attackDowntimeHi; // +0x86c (attack timer: duration hi = 0)
    // Combat/wingz state timers (the GruntAssetLoaders cluster fills them).
    i32 m_combatClockLo; // +0x870 (combat timer: anchor clock lo = g_645588; i64 w/ m_combatClockHi)
    i32 m_combatClockHi;   // +0x874 (combat timer: anchor clock hi = 0)
    i32 m_combatTimeoutLo; // +0x878 (combat timer: duration lo = CombatTimeout config)
    i32 m_combatTimeoutHi; // +0x87c (combat timer: duration hi = 0)
    i32 m_880;             // +0x880 (timer record base, SerializeMove)
    char m_pad884[0x890 - 0x884];
    i32 m_wingzClockLo; // +0x890 (wingz timer: anchor clock lo = g_645588; i64 w/ m_wingzClockHi)
    i32 m_wingzClockHi; // +0x894 (wingz timer: anchor clock hi = 0)
    i32 m_wingzDurationLo; // +0x898 (wingz timer: duration lo = (long)(m_wingzTime*scale-bias))
    i32 m_wingzDurationHi; // +0x89c (wingz timer: duration hi = 0)
    i32 m_8a0;             // +0x8a0 (sub-ser record base, SerializeMove)
    char m_pad8a4[0x8b0 - 0x8a4];
    i32 m_8b0; // +0x8b0 (sub-ser record base, SerializeMove)
    char m_pad8b4[0x8c0 - 0x8b4];
    i32 m_8c0; // +0x8c0
    i32 m_8c4; // +0x8c4
    i32 m_8c8; // +0x8c8
    i32 m_8cc; // +0x8cc
    i32 m_8d0; // +0x8d0
    i32 m_8d4; // +0x8d4  (trailing member; sizeof(CGrunt) == 0x8d8, the `new CGrunt` size)

    // The grunt's spawn constructor @0x47a10 (__thiscall, the CMovingLogic-base
    // moving-object ctor: base CUserLogic(owner), the CMotionState motion band at
    // +0x38 + the twelve default coordinate bounds, then the huge field-init block).
    CGrunt(void* owner);

    // Engine-label backlog stubs. (0x048400 is CGrunt::ReadConfigFromButeMgr, declared above.)
    void LoadCellAnimNames(i32 a, i32 b); // (2-arg; called from LoadEntranceConfig tail)
    void ResetEntranceAnimation(i32 a, i32 b, i32 c); // (ret 0xc) - 3-arg entrance reset
    void ResolveEntranceArrival();
    void EntrancePrepare(); // thunk_FUN_0044b240 (void this-method, external)
    void BuildEntranceAnimation(i32 mode);
    void LoadEntranceConfig();
    // LoadEntranceConfig tail helpers (this-methods reached via incremental-link
    // thunks; external/no-body, reloc-masked).
    void EntranceFinishWire(i32 a, i32 b);  // thunk_FUN_00449c60 (2-arg)
    void EntranceOnReleased();              // thunk_FUN_0044b130 (0-arg)
    void EntranceArrivalHook(i32 a, i32 b); // thunk_FUN_0044d060 (2-arg; arrival commit)

    // ---- migrated CGrunt cluster (ex-CUserLogic_*) ----
    // (~CGrunt / SerializeMove / Activate / UserLogicVfunc9 / MovingSlot16
    // are the vtable slots declared at the top of the class.)
    void EnsureStruckSlot(const char* key); // @0x57b70 lazily build/play the +0x424 sample
    i32 UpdateEntranceAnim();               // @0x690a0 entrance-anim/arrival update step
    void ApplyMoveKind(i32 v);              // @0x57100 (thunk_0x3c29) 1-arg move-kind apply
    i32 Save(CGruntArchive* ar);            // @0x53f90 serialize
    i32 Load(CGruntArchive* ar);            // @0xd8060 deserialize (Read inverse of Save)
    void ClearAllSprites();                 // @0x4b240
    i32 CommitArrival();                    // @0x4b130
    void ClearSubA();                       // @0x57c10
    void ClearSubB();                       // @0x57ce0
    void ReapplyVoiceParams();              // @0x57d10 replay both voices on the registry gate
    void DestroyAnims();                    // @0x57d80
    // @0x31c70 (ret 4) - write the grunt's HUD tile coords (m_10->m_5c/m_60 >> 5)
    // into the caller's {x,y} out slot and return it.
    struct GruntTilePos* GetTilePos(struct GruntTilePos* out); // 0x31c70 (out-of-line in Grunt.cpp)
    // @0x57c40 (ret 4) - lazily build + play the grunt's struck-voice sample for the
    // given sound key (stored into the +0x428 slot ClearSubB frees).
    void EnsureStruckVoice(const char* key);
    // DestroyAnims' two teardown steps ARE ClearSubA/ClearSubB (0x57c10/0x57ce0),
    // declared above - no separate AnimTeardownA/B shadow (those were unbound fakes).
    // Legacy placeholder decls retained: CommitArrival now calls the real creators +
    // SetEntrancePos, but dropping these shifts the unit's symbol set and drifts an
    // unrelated function's fuzzy score (matching-patterns.md symbol-set sensitivity).
    void ArrivalClaim(i32 a, i32 b);
    void ArrivalHook0();
    void ArrivalHook1();
    void ArrivalHook2();
    void ArrivalHook3();
    void ArrivalHook4();
    void ArrivalHook5();
    i32 CanShowStamina();              // @0x514a0
    void SetEntrancePos(i32 a, i32 b); // @0x4d060 (ret 8)
    void EntranceTileOffset(i32* out); // @0x56f80 (ret 4) adjacent-tile pixel pos
    void ComputeFacing(double dt);     // @0x57060 (ret 8)
    i32 ResetGeometry();               // @0x616e0
    void DispatchVtbl24();             // 0x6b260 (out-of-line in Grunt.cpp)

    void PlayMoveSound(i32 x, i32 y);              // @0x511b0 (ret 8)
    void PlaySound(i32 range, CGruntVoiceRec rec); // @0x4ac10 (ret 0x10) external
    void OnStruck(i32 wasHit);                     // @0x588f0 (ret 4)
    i32 ResolveArrivalNeighbor();                  // @0xf26f0 (ret 0)
    void RearmEntranceDrop();                      // @0x68370 (ret 0)

    // ---- the move/timer record serializer (@0x53b80, ret 0x10) ----
    // SerializeMove(ar, mode) drives the grunt move/idle-timer state through an
    // archive whose Read/Write are vtable slots +0x2c/+0x30. mode 4 = save, 7 =
    // load. The eight 16-byte (double-pair) records at +0x810..+0x880 stream
    // directly; the sub-records at +0x150/+0x43c/+0x278/+0x308/+0x890..+0x8c0
    // serialize through their own engine helpers (external/reloc-masked).
    // (SerializeMove is the vtable slot-1 override, declared at the top of CGrunt.)
    // The head sub-serializer @0x16e7f0 (uncertain shape; left external).
    i32 SerializeAnimState(CGruntArchive* ar, i32 mode, i32 a3, i32 a4);
    i32 SerPreStep4(CGruntArchive* ar); // mode-4 pre-step (engine thunk)
    i32 SerPreStep7(CGruntArchive* ar); // mode-7 pre-step (engine thunk)

    // ---- grunt movement / anim-name dispatch state machines (this TU) ----
    // The 5 big per-pose/anim-name resolution state machines: each resolves the
    // grunt's current anim name (via g_animNameResolver) and dispatches on its
    // single-letter type code (A/D/I/G/L/P/O/Q/J/N/M/K), driving the grunt's
    // movement/arrival state + occupied-coord recycle + a re-latch of m_14->m_1c.
    // The arrival/update dispatch trio (ex-CUserLogic_* stubs; really CGrunt - every
    // member offset they touch is in this layout). All three drive the grunt's
    // arrival/entrance bookkeeping + the occupied-slot recycle.
    i32 ArrivalRecycle(i32 a, i32 b, i32 mode, i32 d, i32 e); // @0x59230 (ret 0x14)
    // (Activate is the vtable slot-6 override, declared at the top of CGrunt.)
    i32 UpdateArrival(i32 a1, i32 a2); // @0x62110 (ret 0x8)

    void StepArrivalDrop(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f); // @0x4b370 (ret 0x18, /GX)
    i32 StepGruntMovement(); // @0x4c170 (ret 0)         - the per-tick move step
    i32 StepAnimDispatchA(i32 a, i32 b, i32 c, i32 d); // @0x52fb0 (ret 0x10)
    // (MovingSlot16 is the vtable slot-16 override, declared at the top of CGrunt.)
    i32 StepAnimDispatchB(); // @0x6a6d0 (ret 0)
    // @0x637a0 (ret 0) - the I-code entrance re-stamp dispatch step: D/L reject,
    // reset the +0x8c0 struck timer, on the "I" anim re-notify the tile mgr, then
    // (if the grunt's head tile / HUD point is unobstructed) re-latch a fresh anim
    // set and re-stamp the first entrance-cell frame.
    i32 StepEntranceReinit();
    // @0x67850 (ret 0) - the entrance-move update step: drive the geometry source,
    // gate on the armed-but-not-running sub-player, resolve the current anim name
    // (scratch form), re-latch on "D", create the HUD stat sprites on arrival, then
    // dispatch the +0x1a0 move mode.
    i32 RunEntranceMove();

    // The engine helpers these machines call (all external/no-body, reloc-masked;
    // modeled as __thiscall methods on the grunt so `mov ecx,this; ...; call`
    // falls out). Names describe the observed effect, not a recovered symbol.
    i32 IsDropReady(i32 a = 0); // thunk_0x17df (drop-ready predicate; 1-arg __thiscall)
    void ApplySetState1(i32 v); // thunk_0x4322 (1-arg state apply)
    i32 SetMoveStateA(i32 v, i32 a, i32 b, i32 c); // thunk_0x3bd9 (4-arg; nonzero = re-roll)
    void SetMoveStateB(i32 v, i32 a, i32 b, i32 c, i32 d); // thunk_0x1401 (5-arg-ish)
    void EmitMoveCueQ(i32 a);                              // thunk_0x4336 (1-arg cue/state)
    void EmitMoveCueShort(i32 a, i32 b, i32 c);            // thunk_0x1163 (3-arg cue on m_10)
    void ReseedIdleReset(i32 a, i32 b, i32 c);             // thunk_0x136b (3-arg idle reset)
    // Attack-fire step (UserLogicVfunc7 @0x61cb0, ProjectileUpdate.cpp) helpers
    // (external/no-body thunks, reloc-masked; names = observed roles):
    void GetSpawnPos(i32* out); // thunk_0x1a73 (TimeBomb placement pos, {x,y} out-pair)
    void FinishAttackPowered(); // thunk_0x3dd7 (finish-tail hook when m_poweredUp is set)
    void NotifyAttackImpact();  // thunk_0x22de (impact hook when m_healthSprite exists)
    // Melee hit delivery: the attacker's fire step runs this ON THE TARGET grunt
    // (the neighbor-cell m_grid occupant) with the attacker's tool kind, cell and
    // screen pos; the target's reaction machinery takes over. thunk_0x1bf9, 8 args.
    void
    TakeHit(i32 kind, i32 a2, i32 ownerHi, i32 ownerLo, i32 px, i32 py, i32 a7, i32 attackerKind);
    void OnMoveFinishA(i32 a);             // thunk_0x3ea4 (1-arg finish)
    void CommitMoveA(i32 a, i32 b, i32 c); // thunk_0x3dfa (3-arg move commit)
    void StepCoordTick();                  // thunk_0x245a (0-arg coord tick)
    void OnCoordCommit(i32 a);             // thunk_0x1e47 (1-arg commit)
    void NotifyDrop();                     // thunk_0x119a (0-arg drop notify)
    i32 ProbeRetry();                      // thunk_0x3c0b (retry predicate)
    void OnReanchor(i32 a);                // thunk_0x3cce (1-arg reanchor)
    void StepDropApply();                  // thunk (drop-apply tail)
    i32 ApplyMoveMode(i32 v);              // thunk_0x3b75 -> 0x50ca0 (the >=0x32 / <0x17 mode arm)

    // ---- chunk-2 attributed targets (RearmAttack family + entrance-move tail) ----
    // @0x5b570 (ret 8) - begin the grunt's attack/combat reaction: gated on the
    // entrance being committed (m_1fc) AND the current anim NOT being "F" AND
    // m_stamina>=0x64; fires the directional move-sound, latches the powered-up /
    // combat-timer state, builds the HUD health sprite, and re-arms the ATTACK2 anim.
    i32 BeginAttack(i32 a, i32 b); // @0x5b570
    // @0x61940 (ret 8) - re-arm the grunt's attack/struck anim by entrance-reason:
    // gated on m_entranceReason<0x17; latches neighbor col/row, re-latches the "F"
    // anim set, switches on (m_entranceReason-2) to pick the powered/random branch,
    // latches the combat-timer state, fires the focused-grunt drop cue when visible,
    // marks HUD dirty, drives the ATTACK1/ATTACK2 geometry, re-stamps the cell frame.
    i32 RearmAttackAnim(i32 col, i32 row); // @0x61940
    // @0x61bc0 (ret 0) - the simple ATTACK2 re-arm: re-latch "F" anim set, drive the
    // m_poseAttack2 geometry, re-stamp the entrance-cell frame, set m_214.
    i32 RearmAttackAnim2(); // @0x61bc0
    // @0x67b00 (ret 8) - the grunt-in-radius predicate: given a cell coord (col,row)
    // resolve the occupant grunt via the tile-mgr's 15-wide cell grid, gate it (live,
    // committed, not state 0x36), then test whether the squared tile-distance from
    // this grunt to it is within the (radius-sum)^2 threshold.
    i32 GruntInRadius(i32 col, i32 row); // @0x67b00
    // @0x69fd0 (ret 0) - finish the entrance move: arm the entrance geometry source,
    // gate on the armed-but-not-running sub-player, notify the tile-mgr of the drop
    // (unless m_36c set), then retire the entrance player (m_154->m_8 |= 0x10000).
    i32 FinishEntranceMove(); // @0x69fd0
    // @0x69d60 (ret 0) - the freeze-spell entrance-anim finalize step (DEATHZ_SPARKLE
    // -> idle-delay window -> DEATHZ_UNFREEZE + on-screen entrance cue).
    i32 LoadFreezeSpellAssets(); // @0x69d60
    // @0xec670 (ret 0 -> 1) - the arrival-reposition step: resolve the tile occupant,
    // gate on in-radius, commit its slot (or, when no occupant, re-roll a random
    // in-region target after the idle window elapses) + fire the on-screen entrance cue.
    i32 ResolveArrivalReposition(); // @0xec670
    // @0xf2b20 (ret 0 -> 1) - the multi-state arrival-defender step. Latch the
    // defender position to the last tile, then dispatch on m_2d4 (0/1/2): resolve
    // the cell occupant (the m_tileMgr 15-wide grid in states 1/2, GetOccupant in
    // state 0), gate it (in-radius, committed, settled, on-screen via RectContains),
    // and commit/neighbor-link onto it or, on the no-occupant path, re-roll a random
    // in-region defender target + fire the on-screen entrance cue + reset the idle timer.
    i32 StepArrivalDefense();
    // @0xf8240 (ret 0 -> 1) - the leaner sibling of StepArrivalDefense. Gated on the
    // current anim not being "I"; same m_2d4 (0/1/2) defender dispatch over the grid
    // occupant, but without the m_neighborValid/m_198 CommitTileSlot arms (straight to
    // CommitNeighbor), sets the +0x2ec dwell to 0x1f4 on a state-1 latch, and the
    // state-0 path commits the occupant's tile slot on a rand%100 roll + re-rolls a
    // random in-region target / resets the idle timer.
    i32 StepArrivalDefenseLean();
    // @0xf1c70 (ret 0 -> 1) - the powered-up arrival-defender variant (trace
    // mis-attributed to "tomalla-42"; every offset/helper proves CGrunt). Sets
    // m_arrivalFlags |= 0x40000, then either runs the powered-up release gate
    // (m_poweredUp!=0: FindGridNeighbor + clear-state) or the m_2d4 (0/1/2/3) defender
    // dispatch: GetOccupant/grid-occupant settle + CommitNeighbor, the 4-way
    // StepArrivalDrop tile walk toward m_defenderX/Y, and the on-screen entrance cue.
    i32 StepArrivalDefenseAlt();
    // @0xf60f0 (ret 4, /GX) - the arrival/relocation phase step. Gated on the grunt
    // type name (g_typeColl.Lookup(m_14->m_1c) vs "F"); drives the m_defenderState
    // machine (0/2/4/0x19/0x1a) recomputing the target tile, building the 16 border
    // cells of the 5x5 block into a point accumulator, random-picking a free cell to
    // relocate/arrive on (m_tileMgr TileSwitch6 / CommitTileSlot2), and recycling the
    // visited-coord nodes onto the shared free list.
    i32 PhaseStep();
    // The grunt per-tick arrival/scan/wander step machines (ex-CGruntStep/CGruntWander
    // per-TU aliases; every offset they touch is a CGrunt member, every helper a CGrunt
    // method - so they are CGrunt methods). All __thiscall ret 4, drive the tile-to-tile
    // move via the tile-mgr grid + the on-screen CueA cue.
    i32 UpdateArrival(); // @0xf0130  (GruntUpdateStep.cpp) "I"-grunt arrival update
    i32 SeekTarget();    // @0xf71c0  (GruntUpdateStep.cpp) seek/scan variant
    i32 WanderStep();    // @0xed9f0  (WanderIdleStep.cpp) idle/wander 6-phase step
    i32 ArrivalScanA();  // @0xecc90  (GruntArrivalScan.cpp) grid-cell box scan
    i32 ArrivalScanB();  // @0xf0e20  (GruntArrivalScan.cpp) live-grunt-list scan
    i32 ArrivalScanC();  // @0xf36a0  (GruntArrivalScan.cpp) 0x10000-flag box scan
    // CUserLogic::GetScreenPos (0x29a50) reached on the occupant grunt: copies its
    // m_10->{m_5c,m_60} into the out point. External/reloc-masked.
    void GetScreenPos(struct GruntTilePos* out); // 0x29a50
    // The 0x4b320 tile-switch entry reached __thiscall here (this in ecx, 6 stack args,
    // ret 0x18; returns nonzero on success). Same engine fn as the free CGrunt_TileSwitch
    // passthrough; modeled as a method so `mov ecx,this; ...; call` falls out.
    i32 TileSwitch6(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f); // 0x4b320 (thiscall view)
    // @0x6a060 (ret 0) - the SINK/FALL death-finalize step the death-anim loader runs
    // after the entrance-drop notify. External/no-body (reloc-masked here).
    void Step6a060();

    // @0x68520 (ret 0 -> 0) - begin the bomb-grunt run reaction: retire the HUD stat
    // sprites, latch the entrance/struck state, then either re-notify the move or (when
    // re-rolling) pick a random adjacent tile, play the move sound, latch the "M" anim,
    // load RunningTimePerTile, fire the on-screen spawn cue, and re-stamp the cell frame.
    i32 StartBombGruntRun(); // @0x68520

    // @0x5ecd0 (ret 4, vtable slot-5 body; 1-arg - the no-arg UserLogicVfunc3()
    // base placeholder blocks the OVERRIDE spelling, so it is a plain RVA method
    // like RunAct). Finalize + slot-16 tick, gated anim-code cleanups (L/G ->
    // ClearSubA, off-screen -> ClearSubB), then on the "O" (or scratch-resolved)
    // anim code smoothly interpolate the grunt's HUD position toward the target
    // tile using the per-cell velocity records (m_cells[base] +0x48..+0x60 doubles),
    // clamping on overshoot, and mark the HUD scroll dirty.
    i32 RunPositionInterpStep(i32 arg);
    void FinalizeStep(i32 a); // @0x8b90 (thunk 0x3913; run on `this`)

    // --- entrance/arrival per-tick steps (RunAct-dispatched; GruntEntranceArrival.cpp) ---
    // Each advances the entrance geometry sub-player (m_154->m_1a0.Advance_15c360),
    // and (once the sub-player is armed-but-not-running: m_1a0.m_28!=0 && m_1a0.m_20==0)
    // runs an arrival/entrance commit driven by the tile-mgr + tile-occupancy board.
    i32 StepEntranceRelatchA(); // @0x62840 (re-latch "A" anim, board-gated commit/HUD)
    i32 StepArrivalReroll();    // @0x63b60 (idle re-roll timer -> on-screen entrance cue)
    i32 StepArrivalCommitA();   // @0x65300 (health-gated tile commit / drop / reset)
    i32 StepArrivalCommitB();   // @0x654b0 (snap+commit then the same board-gated tail)
    i32 StepEntranceRelatchB(); // @0x65c20 (re-latch "D" anim, board-gated icon place)

    // @0x646b0 (ret 0x20, 8 stack args) - the combat-reaction anim-dispatch state
    // machine. MISATTRIBUTED to CUserLogic by proximity bracketing; the +0x1fc gate,
    // g_animNameResolver grunt anim codes (A/D/I/G/L/P/O/Q/J/N), m_10 HUD dirty bit
    // and "Grunt"/"CombatTimeout" bute section all prove `this` is a CGrunt (CUserLogic
    // is only 0x40 bytes; +0x1fc is impossible). Re-homed here.
    i32 StepCombatReaction(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7);

    // StepCombatReaction's engine thunks (external/no-body, reloc-masked).
    void UpdateCombatTimer();   // call 0x243c (0-arg tail step)
    void OnTileMismatch(i32 v); // call 0x2cb6 (1-arg)
    i32 ForwardCombatStep(
        i32 a0,
        i32 a1,
        i32 a2,
        i32 a3,
        i32 a4,
        i32 a5,
        i32 a6,
        i32 a7
    );                                                 // call 0x1451
    i32 IsInCombatRange(i32 x, i32 y);                 // call 0x3c4c (2-arg predicate)
    void CommitCombatMove(i32 a, i32 b, i32 c, i32 d); // call 0x302b (4-arg)
    // Battlez arrival resolver's neighbour-pick trigger (CGrunt_TileSwitch).
    i32 TileSwitch(i32 col, i32 row, i32 flags, i32 a4, i32 a5, i32 a6); // thunk 0x1640

    // @0xee800 (ret 4, /GX) - the arrival/defender reticle scan (mis-homed under
    // ?winapi_0ee800_IntersectRect_PtInRect@CUserLogic before xref recovery: this
    // is CGrunt - the this-object extends to +0x3f0 and the caller (0x5d210, CGrunt
    // vtable slot 3) runs it on a CGrunt). Snapshots the reach-box grid cells into a
    // local CByteArray, marks the tiles within m_defenderRadius of the defender tile,
    // clips against the tile grid + viewport (PtInRect/IntersectRect), and recycles
    // occupied-coord nodes through g_coordPool. Reconstruction in GruntReticle.cpp.
    i32 ArrivalReticleScan(); // 0xee800

    // CombatCue per-grunt spell effects (external/no-body, reloc-masked):
    //   TeleportMove(dx,dy,a,b) thunk 0x2f3b (ret 0x10; nonzero = moved)
    //   FreezeApply()           thunk 0x28d8 (0-arg freeze)
    i32 TeleportMove(i32 dx, i32 dy, i32 a, i32 b); // 0x2f3b
    void FreezeApply();                             // 0x28d8
};
// VTBL(CGrunt) for its RTTI vtable 0x1e8754 is bound in src/Gruntz/Grunt.cpp (CGrunt's
// home; moved there from the deleted ApiWrappers stub). It is referenced by scored
// CGrunt/CSpotLight code, so the catalogue was kept as-is (pre-existing binding).

// The per-class activation-registry entry (g_reg_644af0 slot): a 4-byte PMF handler
// on this complete single-inheritance class. RunAct dispatches it on `this`.
typedef i32 (CGrunt::*GruntActHandler)();
struct CGruntActEntry {
    GruntActHandler m_fn;
};
SIZE(CGruntActEntry, 0x4);

// CGrunt segment-vs-box overlap test @0x62b70 - a free (__stdcall, ret 0xc) helper:
// does the directed segment e1->e2 cross into the axis-aligned box `p`
// {m_0=x0, m_4=y0, m_8=x1, m_c=y1}? Tests the segment against each of the box's
// four edges (top y=m_4, bottom y=m_c, left x=m_0, right x=m_8), interpolating the
// crossing point in float and checking it falls within the opposite span. Returns 1
// on the first crossing, else 0. Pure stack args (no this); FP-heavy.
SIZE_UNKNOWN(GruntBox);
struct GruntBox {
    i32 m_0; // +0x00 x0
    i32 m_4; // +0x04 y0
    i32 m_8; // +0x08 x1
    i32 m_c; // +0x0c y1
};
SIZE_UNKNOWN(GruntSegEnd);
struct GruntSegEnd {
    i32 m_0; // +0x00 x
    i32 m_4; // +0x04 y
};
i32 __stdcall CGrunt_SegBoxOverlap(GruntBox* p, GruntSegEnd* e1, GruntSegEnd* e2);

// CGrunt::IsSameType(a, b) @0x3c7f0 - a free (__cdecl) comparator: returns
// (a->m_8 == b->m_8). Not a member (reads both args off the stack).
bool CGrunt_IsSameType(CGrunt* a, CGrunt* b);

// CGrunt::TileSwitch(...) @0x4b320 - a 6-arg (__stdcall, ret 0x18) passthrough
// that scales the first two args to tile pixel coords (*0x20+0x10) and forwards
// all six to an engine helper. External callee reloc-masks.
void GruntRecycleCoords(
    CGrunt* g
); // 0x343f0 coord-recycle (GridUnit::RecycleCoords is the matched view-named twin)
i32 __stdcall CGrunt_TileSwitch(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f);
// The engine tile-switch helper TileSwitch forwards to (__stdcall ret 0x18).
i32 __stdcall GruntTileSwitchImpl(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f);

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---

#endif // SRC_GRUNTZ_GRUNT_H
