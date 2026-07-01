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

#include <Ints.h>
#include <Gruntz/SpriteRefTable.h> // CSpriteRefTable (g_gameReg->m_74; GetSel)

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
struct CSpriteRegRecord {
    char m_pad0[0x8];
    u32 m_8; // +0x08  failure flag word (|= 0x10000)
};

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
struct CSpriteInner {
    char m_pad0[0x10];
    void (*m_init)(void* self); // +0x10  init virtual (called with sprite)
    char m_pad14[0x18 - 0x14];
    CSpriteRegistrar* m_18; // +0x18  the registrar
    char m_pad1c[0xbc - 0x1c];
    i32 m_bc; // +0xbc  (rolling-ball speed; LoadGruntAbilityTuning)
};

struct CHudSprite {
    // The CGameObject-base name/geometry setters the one-shot "SingleAnimation"
    // sprite (BuildGruntLoseItemAnimation) drives: ApplyName(key) (0x150540, ret 4)
    // and ApplyLookupGeometry(key, frame) (0x1505b0, ret 8). External/reloc-masked.
    void ApplyName(const char* key);                      // 0x150540
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
struct CCueRect {
    i32 left;   // +0x00
    i32 top;    // +0x04
    i32 right;  // +0x08
    i32 bottom; // +0x0c
};

// ---------------------------------------------------------------------------
// this->m_10 (a HUD/level geometry source). The factory's two geometry args are
// m_10->m_5c and m_10->m_60 (the latter optionally minus a per-sprite constant).
// ---------------------------------------------------------------------------
struct CGruntHud {
    char m_pad0[0x8];
    i32 m_8; // +0x08   (dirty-flag word; BuildEntrance |= 0x20000)
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
    char m_pad78[0x134 - 0x78];
    i32 m_134; // +0x134  (arrival: view-cull mode cleared)
    i32 m_138; // +0x138  (arrival: view-cull, cleared)
    i32 m_13c; // +0x13c  (arrival: view-cull, cleared)
    i32 m_140; // +0x140  (arrival: view-cull, cleared)
    char m_pad144[0x188 - 0x144];
    i32 m_188; // +0x188  (cue arg)
};

// ---------------------------------------------------------------------------
// The global HUD sprite factory, reached via the global registry ptr -> +0x30 -> +0x8.
// CreateSprite is __thiscall(this, 0, geoB, geoA, hint, name, kind) ret 0x18
// Modeled as a method on the registry singleton so the call shape
// (factory-this = g->m_30->m_8) + the 6-arg push fall out; external/no-body so
// the `call rel32` reloc-masks.
// ---------------------------------------------------------------------------
struct CSpriteFactory {
    CHudSprite* CreateSprite(i32 kind, i32 geoB, i32 geoA, i32 hint, const char* name, i32 flags);
};
// CSpriteFactoryHolder (the registry +0x30 holder) lives in <Gruntz/CGameRegistry.h>.

class CGruntCueSink; // defined below (the 5-arg on-screen cue receiver)

// CGameRegistry - the shared global singleton (*g_pGameRegistry). The CGrunt
// resolvers below read the visible-bounds gate (m_134, m_13c..m_148) and fire
// m_60->Cue.
#include <Gruntz/CGameRegistry.h>

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
#include <Gruntz/CString.h>

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
struct CAnimElem {
    char m_pad0[0x14];
    i32 m_14; // +0x14
};

struct CAnimDescColl {
    char m_pad0[0xc];
    CAnimElem** m_c; // +0x0c  element vector (Idle reads *m_c = first elem)
    i32 m_10;        // +0x10  element count (Idle: >0 gate)
};

class CGruntAnimSub {
public:
    void SetGeometry(i32 srcSprite); // (this = animState+0x1a0)
};

class CGruntAnimState {
public:
    void SetAnim(const char* key);              // (ret 4)
    void SetAnimEx(const char* key, i32 frame); // (ret 8)

    char m_pad0[0x1a0];
    CGruntAnimSub m_1a0; // +0x1a0  (geometry sub-player)
    char m_pad1a4[0x1b4 - 0x1a4];
    CAnimDescColl* m_1b4; // +0x1b4  active-anim descriptor
};

// The animation-set record the lookup tree (a CButeTree) returns;
// stored into CGrunt::m_14->m_1c. m_1c holds the resolved anim-set node.
struct CAnimLookupNode {
    char m_pad0[0x1c];
    void* m_1c; // +0x1c
};

// CButeTree::Find (__thiscall ret 4) - the shared keyed lookup.
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
// The on-screen-cue receiver reached via g_pGameRegistry->m_60 (a __thiscall
// ret 0x14 = 5 stack args). The resolvers fire a 5-arg cue when the
// grunt is on-screen (m_134 == 1 -> 4-way visible-bounds test) or unconditionally
// otherwise. External/no-body (reloc-masked; reached via incremental-link thunk).
//
// BuildEntranceAnimation fires a SIX-arg variant (a different cue overload, also
// via g->m_60); modeled as a second method (CueA, ret 0x18). Both reloc-mask.
// ---------------------------------------------------------------------------
class CGrunt; // fwd-declared for CueA's first arg

class CGruntCueSink {
public:
    void Cue(i32 a, i32 b, i32 c, i32 d, i32 e);             // via thunk 0x33b4
    void Cue1(i32 a);                                        // 1-arg cue (thunk_0x1163 -> 0x51c730)
    void CueA(CGrunt* g, i32 b, i32 c, i32 d, i32 e, i32 f); // 6-arg entrance cue (ret 0x18)
    void CueSpawn(CGrunt* g, i32 b, i32 c, i32 d, i32 e);    // via thunk 0x27ac (ret 0x14)
};

// The entrance-reset (Stub_062e10) cue-gate visibility helper (thunk_FUN_0046b330,
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

class CEntranceHashTable {
public:
    i32 Lookup(const char* szName, CSprite** ppOut); // (ret 8)
};

struct CEntranceSpriteMgr {
    // 1-arg lookup returning the resolved sprite directly (the EXIT/RUN loaders
    // use this form; the 2-arg m_10map.Lookup writes through an out-param instead).
    CSprite* LookupValue(const char* name); // 0x6b2a0

    char m_pad0[0x10];
    CEntranceHashTable m_10map; // +0x10
};

// The grunt's exit-animation holder at CGrunt+0x150 (a 4-byte member just before
// the entrance player pointer). BuildGruntExitAnimation drives its Apply (0x6b2e0,
// 2-arg __thiscall) with the resolved sprite. External/no-body (reloc-masked).
struct CGruntExitHolder {
    void Apply(CSprite* spr, i32 flag); // 0x6b2e0
};

struct CEntranceResMgr {
    char m_pad0[0x2c];
    CEntranceSpriteMgr* m_2c; // +0x2c
};

// The active-anim descriptor the entrance player exposes (its first element's
// +0x14 frame number is the 2nd arg the frame helper consumes).
struct CEntranceAnimDescColl {
    // Bounds-checked indexer (FUN_0046b270): returns the idx'th element ptr (or 0
    // when out of range). The EXIT/RUN loaders read elem[+0x14] (frame number).
    i32* At(i32 idx); // 0x6b270

    char m_pad0[0xc];
    i32** m_c; // +0x0c  element vector (first elem = *m_c)
    i32 m_10;  // +0x10  element count (>0 gate)
};

class CEntranceAnimSub {
public:
    void SetGeometry(i32 srcSprite); // FUN_0055c2d0 (this = player+0x1a0, ret 4)
    // The geometry-state setter LoadEntranceConfig calls on entry; returns 1 when
    // the player is ready (FUN_0055c360, __thiscall ret 4 = 1 stack arg). Same
    // engine fn as SpriteResource's SetGeoSource, but the int return is used here.
    i32 SetGeoSourceR(i32 src); // FUN_0055c360
    // Data-less view: the geometry sub-player's m_20/m_28 (abs CGrunt+0x154+0x1a0
    // +0x20/+0x28) live PAST the player's own m_1b4, so they are not modeled as
    // embedded data here (that would corrupt m_1b4's offset). LoadEntranceConfig's
    // tail and UpdateEntranceAnim's armed-but-not-running gate read them via raw
    // offsets off &player->m_1a0 instead (keeps cl on one `add eax,0x1a0` base).
};

// A per-cell entrance record (0x68-byte stride at CGrunt+0x474). GetName(flag)
// resolves the cell's frame name (__thiscall, 1 arg). External (reloc-masked).
class CGruntCell {
public:
    const char* GetName(i32 flag);
};

class CEntranceAnimPlayer {
public:
    void SetAnimFrame(const char* name, i32 frame); // FUN_005504d0-class (ret 8)
    // Geometry setter that forwards to m_1a0.SetGeometry(src) then, if flag!=0,
    // a 2nd setter (FUN_00458b60, ret 8). PlaySound's IDLE arm drives it directly.
    void SetGeometryEx(i32 src, i32 flag); // FUN_00458b60
    // A 1-arg setter the WALK/E arms call on the player itself (FUN_00550540,
    // FUN_005504d0 is the 2-arg form). Takes the resolved cell name.
    void SetAnimName(const char* name); // FUN_00550540 (ret 4)
    // The death/freeze finalize 2-arg geometry setter (FUN_005505b0); takes the
    // resolved key string + a flag. External/reloc-masked.
    void ApplyLookupGeometry(const char* key, i32 flag); // FUN_005505b0 (ret 8)
    // The combat-reaction dispatch (@0x646b0) drives the player through its
    // CGameObject base name/sprite setters (0x150540 / 0x1504d0, not the 0x55xxxx
    // entrance forms). External/no-body so the call rel32 reloc-masks.
    void GameApplyName(const char* name);                  // 0x150540 (ret 4)
    void GameApplyLookupSprite(const char* key, i32 flag); // 0x1504d0 (ret 8)
    // The CGameObject-base lookup-geometry setter (same 0x1505b0 slot CHudSprite
    // uses) the death/freeze finalize drives with the DEATHZ_SPARKLE/UNFREEZE keys.
    void GameApplyLookupGeometry(const char* key, i32 flag); // 0x1505b0 (ret 8)

    char m_pad0[0x8];
    i32 m_8;              // +0x08  state-flag word (death loader |= 1 / |= 0x10000)
    CEntranceResMgr* m_c; // +0x0c  resource object (lookup table holder)
    char m_pad10[0x1a0 - 0x10];
    CEntranceAnimSub m_1a0; // +0x1a0 geometry sub-player
    char m_pad1a4[0x1b4 - 0x1a4];
    CEntranceAnimDescColl* m_1b4; // +0x1b4 active-anim descriptor
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
class CEntranceAnimSrc {
public:
    i32 LookupAnimSet(const char* key); // FUN_0056d190 (ret 4)
};
extern CEntranceAnimSrc g_entranceAnimSrc; // DAT_006bf620
#define EntranceLookupAnimSet(k) (g_entranceAnimSrc.LookupAnimSet(k))

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
class CAnimNameRecord {
public:
    char* m_name; // +0x00
};
class CAnimNameResolver {
public:
    char** GetNameRecord(void* node);            // thunk_FUN_004310f0 (ret 4)
    CAnimNameRecord* GetNameRecords(void* node); // thunk_FUN_004312a0 (ret 4)
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
// The 9 runtime direction-index globals InitDirVectors (0x5caa0) reads to place
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
struct CAnimScratchString {
    char* m_str;    // +0x00  (4-byte stride for the teardown walk)
    void Release(); // FUN_001b9b93 (engine CString release)
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
// movement/arrival machines reach the level board via g_gameReg->m_70 (a Board*),
// whose m_8 is the row-pointer table (rows[y][x] -> a 0x1c-byte tile record whose
// first dword carries the occupancy/flag bits) and m_c/m_10 the x/y in-bounds
// limits. Reloc-masked DATA; a struct (mangles `U`) gives the retail name.
// ---------------------------------------------------------------------------
struct GruntBoard {
    char m_pad0[0x8];
    char** m_8; // +0x08  row table: m_8[y][x] -> tile record
    i32 m_c;    // +0x0c  x bound
    i32 m_10;   // +0x10  y bound
};
// The per-effect sound-table object reached via g_gameReg->m_30 (the sound
// category), whose m_28 holds a CMapStringToOb at +0x10 (the launch-sound lookup
// the struck-voice creator @0x57c40 queries). Same shape Projectile's LaunchSound
// reaches. All external (reloc-masked).
struct GruntSoundEntry; // map value: per-effect sound entry (factory at +0x10)
struct GruntSoundInner; // m_30->m_28: holds the lookup map at +0x10
struct GruntSoundCat {  // m_30: the sound-category object
    char m_pad0[0x28];
    GruntSoundInner* m_28; // +0x28  -> the lookup map lives at (*m_28)+0x10
};
struct WwdGameReg {
    char m_pad0[0x30];
    GruntSoundCat* m_30; // +0x30  the sound category (struck-voice lookup root)
    char m_pad34[0x60 - 0x34];
    CGruntCueSink* m_60; // +0x60  the on-screen cue receiver (CueA/CueSpawn)
    char m_pad64[0x68 - 0x64];
    i32 m_68; // +0x68  (SerializeMove mode-8: -> CGrunt::m_tileMgr)
    char m_pad6c[0x70 - 0x6c];
    GruntBoard* m_70;      // +0x70  the level board
    CSpriteRefTable* m_74; // +0x74  the sprite/animation reference table (GetSel)
    char m_pad78[0x11c - 0x78];
    i32 m_11c; // +0x11c  the sound-channel param (struck-voice Play arg)
    char m_pad120[0x134 - 0x120];
    i32 m_134; // +0x134  the view-cull mode gate (==1 -> rand%3, else rand%6)
};
extern WwdGameReg* g_gameReg; // ?g_gameReg@@3PAUWwdGameReg@@A @0x64556c

// The struck-voice sound model (creator @0x57c40). The lookup returns a
// GruntSoundEntry whose +0x10 sub-object owns a sample factory (GetItem @0x135d70,
// __thiscall ret 0 -> new sample); the sample is Play'd (0x136300) and lives in the
// grunt's +0x428 slot (freed by ClearSubB). All engine callees external/reloc-masked.
struct GruntSoundSample {
    i32 Play(i32 channel, i32 a, i32 b, i32 c); // 0x136300 (__thiscall, 4 args)
};
struct GruntSampleFactory {
    GruntSoundSample* GetItem(); // 0x135d70 (__thiscall, 0 args; returns a new sample)
};
struct GruntSoundEntry {
    char m_pad0[0x10];
    GruntSampleFactory* m_10; // +0x10  the sample factory
};
struct GruntSoundMap {
    i32 Lookup(const char* key, GruntSoundEntry** out); // 0x1b8438 (ret 8)
};
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
struct GruntCoordPool {
    void Recycle(void* elem); // FUN_004311b0
};
extern GruntCoordPool g_coordPool; // DAT_00645540

// A grunt occupied-coord list node: ->next at +0, ->coord at +8 (an {x,y} pair).
struct GruntCoord {
    i32 m_x; // +0x00
    i32 m_y; // +0x04
};
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
struct CTileBoardDims {
    char m_pad0[0x28];
    i32 m_28; // +0x28  board width
    i32 m_2c; // +0x2c  board height
};
struct CTileRegMid {
    char m_pad0[0x5c];
    CTileBoardDims* m_5c; // +0x5c
};
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

    // CombatCue's grid (this+0x1c): a 4x15 grunt-pointer board scanned i=0..3, j=0..14
    // (the address is monotonic, so retail strength-reduces it to a running pointer).
    char m_pad0[0x1c];
    CGrunt* m_grid[4][15]; // +0x1c
    char m_pad10c[0x22c - 0x10c];
    CTileReg* m_22c; // +0x22c  the level registry (board dims)
};

// The on-screen point-visibility predicate the arrival/update steps gate the cue
// on (FUN_0046b330, 0x6b330; __cdecl 3 args -> `add esp,0xc`): given a probe
// kind and the grunt's HUD point, returns whether it falls inside the live view
// rect. External/no-body (reloc-masked).
i32 GruntPointVisible(i32 px, i32 py, i32 cmp);

// The registry focused-grunt slot the arrival gate reads: an array at
// g_pGameRegistry+0x150, stride 0x238 (= 71*8) indexed by the grunt's m_tileOwnerHi.
// Each slot's +0x14 is a non-null gate the arrival path checks. External view.
struct CFocusSlot {
    char m_pad0[0x14];
    i32 m_14; // +0x14
};

// ---------------------------------------------------------------------------
// The serialization sink CGrunt::Save drives: a custom archive whose vtable
// slot 0x30 is a `Write(const void* data, int size)` (member fn, thiscall).
// Modeled as a polymorphic class with 13 virtuals (slot 0x30 = the 13th) so
// each `mov edx,[ebx]; push size; push &field; mov ecx,ebx; call [edx+0x30]`
// falls out. The archive is external (never instantiated here, so no vtable is
// emitted); Write's body is reloc-masked.
// ---------------------------------------------------------------------------
class CGruntArchive {
public:
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
class CGruntSubSer {
public:
    i32 Serialize(CGruntArchive* ar, i32 mode, i32 a3, i32 a4);
};

// The grunt's name-id resolver the Save reaches via m_158->m_c->m_2c: maps an
// integer id to its name CString (returned by value). __thiscall, ret 4.
class CGruntNameMap {
public:
    CString LookupName(i32 id);
};

// The +0x158 "type catalog" object: Save reads its m_c (a non-null owner that
// also holds the name-id map at m_2c). External; modeled minimally.
struct CGruntTypeCatalog {
    char m_pad0[0xc];
    CGruntNameMap* m_c; // +0x0c  owner -> name-id map
};

// The global serialize counter Save bumps before each variable-length record
// (DAT_00629ad0). TU-local (reloc-masked); shared in retail.
extern i32 g_serialCounter;

// The linked-list node Save's tail walks (m_33c head): {next @+0, data @+0x8}.
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
// names through the resource manager (g_gameReg->m_30).
//
// The CObArray-family collections it rebuilds (this+0x370, the 4x stride-0x14
// group at this+0x3a4, this+0x488). Each is the engine CObArray {vtbl, m_data,
// m_count, m_max, m_grow} (0x14 bytes); SetSize/SetAtGrow are external (reloc-
// masked). The recycled nodes ride the same g_gruntFreeList pool as the movement
// machines (node usable area = head+4; head[0] = next).
struct GruntLoadColl {
    void SetSize(i32 n, i32 grow);    // 0x1b4f75
    void SetAtGrow(i32 idx, void* p); // 0x1b5144
    void* m_vtbl;                     // +0x00
    void** m_data;                    // +0x04
    i32 m_count;                      // +0x08
    i32 m_max;                        // +0x0c
    i32 m_grow;                       // +0x10
};

// The CString member the load streams a 0x200-byte buffer into (this+0x410);
// operator=(const char*) is external (0x1b9e74, reloc-masked).
struct GruntLoadStr {
    void Assign(const char* s); // operator= 0x1b9e74
};

// The anim-name id table entry resolved through res->m_10's CMapStringToOb (+0x10,
// Lookup 0x1b8008): a range [m_64..m_68] and the id array at +0x14.
struct GruntIdEntry {
    char m_pad0[0x14];
    i32* m_14; // +0x14  id array
    char m_pad18[0x64 - 0x18];
    i32 m_64; // +0x64  lo index
    i32 m_68; // +0x68  hi index
};
struct GruntNameIdMap {                              // res->m_10 + 0x10
    i32 Lookup(const char* key, GruntIdEntry** out); // 0x1b8008
    i32 LookupNode(const char* key, void** out);     // 0x1b8008 (2nd block: raw entry)
};
// The object-table entry resolved through res->m_8's map (+0x48, Lookup 0x1b8760).
// Validated by a virtual kind() at vtable slot +0x20 (== 5 -> keep).
class GruntObjEntry {
public:
    virtual void s00();
    virtual void s04();
    virtual void s08();
    virtual void s0c();
    virtual void s10();
    virtual void s14();
    virtual void s18();
    virtual void s1c();
    virtual i32 Kind(); // vtable slot +0x20
};
struct GruntObjMap {                            // res->m_8 + 0x48
    i32 Lookup(void* key, GruntObjEntry** out); // 0x1b8760
};
// The resource manager (g_gameReg->m_30): m_8 owns the object map, m_10 the
// sprite/name manager (with the CMapStringToOb at +0x10).
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
class CGruntSub {
public:
    void Free();
};

// ---------------------------------------------------------------------------
// ~CGrunt teardown support (the leaf dtor @0xf2f0). CGrunt is a CUserLogic leaf
// (most-derived vtable 0x5e8754) that, unlike the bare game-object leaves, OWNS
// six destructible sub-objects torn down (in /GX trylevel order) before the base
// teardown folds in (CUserLogic vptr 0x5e705c -> inline ~EngStr on the +0x18
// link -> CUserBase vptr 0x5e70b4). All callees external/no-body (reloc-masked).
//   +0x468  a 9-elem array (stride 0x68) torn via the MSVC vector-dtor iterator
//           (FUN_0051f640) with the per-element dtor &g_gruntCellDtor (0x4023a6)
//   +0x44c  ~CString (0x1b9cde)        +0x448  ~CString (0x1b9cde)
//   +0x338  ~CObList (0x1b48c6)        +0x31c  ~CObList (0x1b48c6)
//   +0x1c0  ~CString = m_animSetName (0x1b9cde)
//   +0x18   ~EngStr  (0x16d2a0) the CUserLogic link base teardown
// The three vptr stores reference the RETAIL vtables by address (reloc-masked
// DATA externs); the most-derived class isn't fully modelled so the compiler
// can't emit them.
extern void* g_cgruntVtbl;    // 0x5e8754  ??_7CGrunt (most-derived)
extern void* g_userLogicVtbl; // 0x5e705c  ??_7CUserLogic
extern void* g_userBaseVtbl;  // 0x5e70b4  ??_7CUserBase
extern void GruntCellDtor();  // 0x4023a6  per-cell vector-dtor element callback
// The MSVC __ehvec_dtor-style array teardown helper: (base, stride, count, dtor).
void GruntVecDtor(void* base, i32 stride, i32 count, void (*dtor)()); // 0x51f640
// Each owned sub-object is torn down by its engine dtor reached __thiscall (this in
// ecx, no stack arg/cleanup). Modeled as a 1-method receiver so `lea ecx,[this+off];
// call` falls out, and as a real value member with `~T(){Dtor();}` so the /GX frame's
// per-member descending trylevel chain is what the compiler emits.
struct GruntStrSub { // +0x44c / +0x448 / +0x1c0  (~CString 0x1b9cde)
    void Dtor();
    ~GruntStrSub() {
        Dtor();
    }
};
struct GruntListSub { // +0x338 / +0x31c  (~CObList 0x1b48c6)
    void Dtor();
    ~GruntListSub() {
        Dtor();
    }
};
struct GruntLinkSub { // +0x18  the CUserLogic base link (~EngStr 0x16d2a0)
    void Dtor();
    ~GruntLinkSub() {
        Dtor();
    }
};

// A 10-virtual interface view for CGrunt::DispatchVtbl24's tail call (vtable
// slot 0x24 = index 9). Calling Slot9() emits `mov eax,[ecx]; jmp [eax+0x24]`.
class CVtblSlot9 {
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

// The name/animation cache collections FreeNameList drains (sub-objects of CGrunt
// at +0x31c and +0x338). External engine collections; only the called methods
// are modeled (reloc-masked).
class CGruntColl {
public:
    void Reset(); // empty the collection in place
};
class CGruntList {
public:
    void* RemoveHead(); // pop the head node, return it
};
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
struct CGruntVoiceRec {
    i32 m_0;
    i32 m_4;
    i32 m_8;
};

// The {x, y} tile-coordinate pair GetTilePos (@0x31c70) writes its result into
// (the grunt's HUD pixel pos >> 5). Returned by pointer (the out arg).
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
class CGrunt {
public:
    i32 CreateHealthSprite();
    i32 CreateToySprite();
    i32 CreateStaminaSprite();
    i32 CreateToyTimeSprite();
    i32 CreateWingzTimeSprite();
    i32 CreatePowerupSprite(i32 a); // (ret 4)
    i32 CreateSelectedSprite();

    void ReadConfigFromButeMgr();
    void LoadGruntMovingDeathConfig();
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
    void PickupResetA();                 // thunk 0x214e (__thiscall ret 0)
    void PickupResetB(i32 a, i32 b, i32 c); // thunk 0x136b (__thiscall ret 0xc)

    // @0x57890 (__thiscall ret 0, /GX) - when the entrance reason is a lose-item
    // pose (0x12/0x16/0xe), spawn the one-shot "SingleAnimation" GRUNTZ_<set>_LOSEITEM
    // sprite, fire the on-screen spawn cue, then re-run the type-table step.
    i32 BuildGruntLoseItemAnimation();
    // The big CUserLogic-base step driver reached via thunk 0x3bd9 -> 0x4dd50
    // (LoadGruntTypeTable / SelfImpact); external/reloc-masked here.
    void LoadGruntTypeTable(i32 a, i32 b, i32 c, i32 d);

    // --- arrival / move-step helper cluster (proximity-attributed targets) ---
    void PlayMoveSoundAtTile(i32 tx, i32 ty); // @0x514e0 (ret 8) tile->pixel + PlayMoveSound
    void SnapToLastTile(i32 a);               // @0x517b0 (ret 4) snap m_10 to last tile + commit
    i32 ClaimSwitchTile();                    // @0x52c70 (ret 0) switch-dir tile claim
    void SetArrivalTarget(i32 a, i32 b, i32 c, i32 d); // @0x52ed0 (ret 0x10)
    void ConsiderArrival(i32 a);                       // @0x52f40 (ret 4) arrival/drop gate
    void SelectMoveIcon(i32 a);                        // @0x57800 (ret 4) pick move-cursor icon
    i32 TryPowerupAtTile();                            // @0x57aa0 (ret 0) probe move tile

    // --- animation resolvers (this TU's targets) ---
    i32 ResolveMovingAnimation();
    i32 ResolveDeathAnimation();
    i32 ResolveAnimation(); // (generic / "_JOY")
    i32 ResolveIdleAnimation();
    i32 ResolveBattlecryAnimation();

    // @0x4a9f0 (ret 0) - the 4-way reachability probe: resolve the grunt under the
    // HUD center (m_tileMgr->FindAtPixel), copy its entrance rect (+0x144) offset by
    // its HUD origin, then test 4 segments (vertical / horizontal / two diagonals,
    // +-1000 px) through the grunt's own HUD center; return 1 on the first hit.
    i32 winapi_04a9f0_CopyRect_OffsetRect();
    i32 RectSegProbe(void* r, void* a, void* b); // call 0x4138 (__thiscall, 3 args)

    char m_pad0[0x10];
    CGruntHud* m_10;       // +0x10
    CAnimLookupNode* m_14; // +0x14  (anim-set lookup holder)
    char m_pad18[0x30 - 0x18];
    i32 m_prevAnimSetNode; // +0x30  (saved old m_14->m_1c before re-latch)
    char m_pad34[0x38 - 0x34];
    CGruntAnimState* m_38; // +0x38  (animation player)
    char m_pad3c[0x40 - 0x3c];
    i32 m_activeAnimDesc; // +0x40  (cached m_38->m_1b4)
    char m_pad44[0x54 - 0x44];
    CString m_typeName;                       // +0x54  (grunt-type name CString)
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
    i32 m_animResolved; // +0xa8  (resolve gate / dirty flag)
    i32 m_deathCueArg;  // +0xac  (cue arg)
    char m_padb0[0x154 - 0xb0];
    CEntranceAnimPlayer* m_154;    // +0x154 (entrance animation player)
    struct CGruntSndResMgr* m_158; // +0x158 (ability/sound resource mgr)
    i32 m_prevEntranceDesc;        // +0x15c (= m_154->m_1b4 cache)
    char m_pad160[0x170 - 0x160];
    i32 m_entranceReason; // +0x170 (entrance-reason / movement state)
    i32 m_entrancePxX;    // +0x174 (SetEntrancePos: committed entrance position X, pixel)
    i32 m_entrancePxY;    // +0x178 (SetEntrancePos: committed entrance position Y, pixel)
    i32 m_lastTilePxX;    // +0x17c (LoadEntranceConfig: last occupied tile X, pixel; -1 = none)
    i32 m_lastTilePxY;    // +0x180 (LoadEntranceConfig: last occupied tile Y, pixel; -1 = none)
    i32 m_184;            // +0x184 (ClaimSwitchTile: = m_lastTilePxX after switch)
    i32 m_188_tilePxY;    // +0x188 (ClaimSwitchTile: = m_lastTilePxY after switch)
    char m_pad18c[0x190 - 0x18c];
    i32 m_toyBlendPct; // +0x190 (anim-name loader: TOY1/TOY2 blend percent)
    char m_pad194[0x1b8 - 0x194];
    CHudSprite* m_selectedSprite;  // +0x1b8
    CHudSprite* m_toySprite;       // +0x1bc
    CString m_animSetName;         // +0x1c0  (anim-name loader: "GRUNTZ_"+m_animSetName+...)
    CHudSprite* m_healthSprite;    // +0x1c4
    CHudSprite* m_staminaSprite;   // +0x1c8
    CHudSprite* m_toyTimeSprite;   // +0x1cc
    CHudSprite* m_wingzTimeSprite; // +0x1d0
    CHudSprite* m_powerupSprite;   // +0x1d4
    i32 m_arrived;                 // +0x1d8 (entrance-arrival gate)
    char m_pad1dc[0x1e4 - 0x1dc];
    i32 m_entranceActive; // +0x1e4 (entrance: set to 1)
    i32 m_arrivalPending; // +0x1e8 (SnapToLastTile/ClaimSwitchTile arrival-commit latch)
    i32 m_tileOwnerHi;    // +0x1ec
    i32 m_tileOwnerLo;    // +0x1f0
    i32 m_1f4_moveIcon;   // +0x1f4 (SelectMoveIcon: clamped icon index, [0,0x11))
    char m_pad1f8[0x1fc - 0x1f8];
    i32 m_entranceCommitted; // +0x1fc (entrance: cleared)
    i32 m_neighborCol;       // +0x200 (grid-neighbor: column, -1 = none)
    i32 m_neighborRow;       // +0x204 (grid-neighbor: row, -1 = none)
    char m_pad208[0x21c - 0x208];
    i32 m_neighborValid; // +0x21c (grid-neighbor: cleared on miss)
    i32 m_poweredUp;     // +0x220 (powered-up gate; 0 = run entrance reset)
    char m_pad224[0x22c - 0x224];
    i32 m_22c; // +0x22c (entrance-drop: latched anim re-init gate)
    i32 m_230; // +0x230 (entrance-arrival: cleared)
    char m_pad234[0x238 - 0x234];
    i32 m_wingzEnabled; // +0x238
    char m_pad23c[0x244 - 0x23c];
    i32 m_resetApplied; // +0x244 (entrance-reset: 0 then 1 = "applied" flag)
    i32 m_arrivalFlags; // +0x248 (arrival flag word; |= 0x18040402)
    char m_pad24c[0x258 - 0x24c];
    i32 m_gruntKind;          // +0x258 (grunt type/kind; ==0x37 -> halve TimePerTile)
    i32 m_entranceArmed;      // +0x25c (entrance: set to 1)
    CGruntTileMgr* m_tileMgr; // +0x260 (path/occupancy sub-manager)
    i32 m_struckCount;        // +0x264 (struck-reaction counter; cue tier 5/0xa)
    i32 m_struckClockLo;      // +0x268 (= g_645588 game clock at last struck)
    i32 m_struckClockHi;      // +0x26c (= 0)
    i32 m_struckTimerLo;      // +0x270 (= 0xfa0 struck cooldown window)
    i32 m_struckTimerHi;      // +0x274 (= 0)
    char m_pad278[0x2d0 - 0x278];
    i32 m_arrivalState; // +0x2d0 (arrival: = 4)
    i32 m_2d4;          // +0x2d4 (arrival: = 0)
    char m_pad2d8[0x2dc - 0x2d8];
    i32 m_defenderRadius; // +0x2dc (defender radius / arrival kind)
    char m_pad2e0[0x2f0 - 0x2e0];
    i32 m_2f0; // +0x2f0 (arrival: = -1)
    i32 m_2f4; // +0x2f4 (arrival: = -1)
    char m_pad2f8[0x300 - 0x2f8];
    i32 m_defenderX; // +0x300 (arrival: = m_lastTilePxX)
    i32 m_defenderY; // +0x304 (arrival: = m_lastTilePxY)
    i32 m_308;       // +0x308 (arrival: cleared)
    i32 m_30c;       // +0x30c (arrival: cleared)
    i32 m_310;       // +0x310 (arrival: cleared)
    i32 m_314;       // +0x314 (arrival: cleared)
    char m_pad318[0x364 - 0x318];
    i32 m_364; // +0x364 (entrance: set to 1)
    char m_pad368[0x394 - 0x368];
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
    char m_pad3d8[0x3ec - 0x3d8];
    i32 m_health;    // +0x3ec
    i32 m_stamina;   // +0x3f0
    i32 m_toyTime;   // +0x3f4
    i32 m_wingzTime; // +0x3f8
    char m_pad3fc[0x41c - 0x3fc];
    i32 m_timePerTile; // +0x41c (TimePerTile config; ComputeFacing time divisor; halved for kind 0x37)
    i32 m_tileClaimed; // +0x420 (arrival-claimed latch)
    char m_pad424[0x43c - 0x424];
    i32 m_entranceCell[3]; // +0x43c (entrance-cell triple: [0]=col, [1]=row, [2]=m_444 reason)
    char m_pad448[0x450 - 0x448];
    i32 m_arrivalPhase; // +0x450 (arrival/update dispatch phase: 2 = in-flight, 3 = committing)
    char m_pad454[0x460 - 0x454];
    i32 m_lowStaminaCued;  // +0x460 (low-stamina off-screen cue latch)
    i32 m_arrivalNotified; // +0x464 (entrance-reset latch flag)
    char m_pad468[0x474 - 0x468];
    char m_474[1]; // +0x474 (entrance-cell record table; 0x68-byte stride records)
    char m_pad475[0x820 - 0x475];
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
    char m_pad850[0x858 - 0x850];
    i32 m_858; // +0x858 (entrance: = 0)
    i32 m_85c; // +0x85c (entrance: = 0)
    char m_pad860[0x870 - 0x860];
    // Combat/wingz state timers (the GruntAssetLoaders cluster fills them).
    i32 m_870; // +0x870 (combat: = g_645588 clock low)
    i32 m_874; // +0x874 (combat: = 0)
    i32 m_878; // +0x878 (combat: = CombatTimeout config)
    i32 m_87c; // +0x87c (combat: = 0)
    char m_pad880[0x890 - 0x880];
    i32 m_890; // +0x890 (wingz: = g_645588 clock low)
    i32 m_894; // +0x894 (wingz: = 0)
    i32 m_898; // +0x898 (wingz: = wingz-duration; (long)(m_wingzTime*scale-bias))
    i32 m_89c; // +0x89c (wingz: = 0)

    // Engine-label backlog stubs.
    void Stub_047a10();
    void Stub_048400();
    void Stub_048470(i32 a, i32 b);        // (2-arg; called from LoadEntranceConfig tail)
    void Stub_062e10(i32 a, i32 b, i32 c); // (ret 0xc) - 3-arg entrance reset
    void Stub_0633e0();
    void EntrancePrepare(); // thunk_FUN_0044b240 (void this-method, external)
    void BuildEntranceAnimation(i32 mode);
    void LoadEntranceConfig();
    // LoadEntranceConfig tail helpers (this-methods reached via incremental-link
    // thunks; external/no-body, reloc-masked).
    void EntranceFinishWire(i32 a, i32 b);  // thunk_FUN_00449c60 (2-arg)
    void EntranceOnReleased();              // thunk_FUN_0044b130 (0-arg)
    void EntranceArrivalHook(i32 a, i32 b); // thunk_FUN_0044d060 (2-arg; arrival commit)

    // ---- migrated CGrunt cluster (ex-CUserLogic_*) ----
    ~CGrunt();                              // @0xf2f0  /GX leaf dtor (6 members + base fold)
    void EnsureStruckSlot(const char* key); // @0x57b70 lazily build/play the +0x424 sample
    i32 UpdateEntranceAnim();               // @0x690a0 entrance-anim/arrival update step
    void ApplyMoveKind(i32 v);              // @0x57100 (thunk_0x3c29) 1-arg move-kind apply
    i32 Save(CGruntArchive* ar);            // @0x53f90 serialize
    i32 Load(CGruntArchive* ar);            // @0xd8060 deserialize (Read inverse of Save)
    void ClearAllSprites();                 // @0x4b240
    i32 CommitArrival();                    // @0x4b130
    void ClearSubA();                       // @0x57c10
    void ClearSubB();                       // @0x57ce0
    void DestroyAnims();                    // @0x57d80
    // @0x31c70 (ret 4) - write the grunt's HUD tile coords (m_10->m_5c/m_60 >> 5)
    // into the caller's {x,y} out slot and return it.
    struct GruntTilePos* GetTilePos(struct GruntTilePos* out);
    // @0x57c40 (ret 4) - lazily build + play the grunt's struck-voice sample for the
    // given sound key (stored into the +0x428 slot ClearSubB frees).
    void EnsureStruckVoice(const char* key);
    void AnimTeardownA(); // engine thunk (DestroyAnims step 1)
    void AnimTeardownB(); // engine thunk (DestroyAnims step 2)
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
    void FreeNameList();               // @0x48360
    i32 ResetGeometry();               // @0x616e0
    void DispatchVtbl24();             // @0x6b260 (jmp [vtbl+0x24])

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
    i32 SerializeMove(CGruntArchive* ar, i32 mode, i32 a3, i32 a4);
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
    void InitDirVectors();                                    // @0x5caa0 (ret 0; reset/init)
    i32 UpdateArrival(i32 a1, i32 a2);                        // @0x62110 (ret 0x8)

    void StepArrivalDrop(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f); // @0x4b370 (ret 0x18, /GX)
    i32 StepGruntMovement(); // @0x4c170 (ret 0)         - the per-tick move step
    i32 StepAnimDispatchA(i32 a, i32 b, i32 c, i32 d); // @0x52fb0 (ret 0x10)
    void StepCoordResolve();                           // @0x5f310 (ret 0)
    i32 StepAnimDispatchB();                           // @0x6a6d0 (ret 0)
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
    void OnMoveFinishA(i32 a);                             // thunk_0x3ea4 (1-arg finish)
    void CommitMoveA(i32 a, i32 b, i32 c);                 // thunk_0x3dfa (3-arg move commit)
    void StepCoordTick();                                  // thunk_0x245a (0-arg coord tick)
    void OnCoordCommit(i32 a);                             // thunk_0x1e47 (1-arg commit)
    void NotifyDrop();                                     // thunk_0x119a (0-arg drop notify)
    i32 ProbeRetry();                                      // thunk_0x3c0b (retry predicate)
    void OnReanchor(i32 a);                                // thunk_0x3cce (1-arg reanchor)
    void StepDropApply();                                  // thunk (drop-apply tail)
    i32 ApplyMoveMode(i32 v); // thunk_0x3b75 -> 0x50ca0 (the >=0x32 / <0x17 mode arm)

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
    // mis-attributed to "ClassUnknown_42"; every offset/helper proves CGrunt). Sets
    // m_arrivalFlags |= 0x40000, then either runs the powered-up release gate
    // (m_poweredUp!=0: FindGridNeighbor + clear-state) or the m_2d4 (0/1/2/3) defender
    // dispatch: GetOccupant/grid-occupant settle + CommitNeighbor, the 4-way
    // StepArrivalDrop tile walk toward m_defenderX/Y, and the on-screen entrance cue.
    i32 StepArrivalDefenseAlt();
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

    // CombatCue per-grunt spell effects (external/no-body, reloc-masked):
    //   TeleportMove(dx,dy,a,b) thunk 0x2f3b (ret 0x10; nonzero = moved)
    //   FreezeApply()           thunk 0x28d8 (0-arg freeze)
    i32 TeleportMove(i32 dx, i32 dy, i32 a, i32 b); // 0x2f3b
    void FreezeApply();                             // 0x28d8
};

// CGrunt segment-vs-box overlap test @0x62b70 - a free (__stdcall, ret 0xc) helper:
// does the directed segment e1->e2 cross into the axis-aligned box `p`
// {m_0=x0, m_4=y0, m_8=x1, m_c=y1}? Tests the segment against each of the box's
// four edges (top y=m_4, bottom y=m_c, left x=m_0, right x=m_8), interpolating the
// crossing point in float and checking it falls within the opposite span. Returns 1
// on the first crossing, else 0. Pure stack args (no this); FP-heavy.
struct GruntBox {
    i32 m_0; // +0x00 x0
    i32 m_4; // +0x04 y0
    i32 m_8; // +0x08 x1
    i32 m_c; // +0x0c y1
};
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
void __stdcall CGrunt_TileSwitch(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f);
// The engine tile-switch helper TileSwitch forwards to (__stdcall ret 0x18).
void __stdcall GruntTileSwitchImpl(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f);

#endif // SRC_GRUNTZ_GRUNT_H
