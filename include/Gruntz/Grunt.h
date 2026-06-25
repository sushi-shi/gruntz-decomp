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
};

struct CHudSprite {
    char m_pad0[0x7c];
    CSpriteInner* m_7c; // +0x7c
};

// ---------------------------------------------------------------------------
// this->m_10 (a HUD/level geometry source). The factory's two geometry args are
// m_10->m_5c and m_10->m_60 (the latter optionally minus a per-sprite constant).
// ---------------------------------------------------------------------------
struct CGruntHud {
    char m_pad0[0x8];
    i32 m_8; // +0x08   (dirty-flag word; BuildEntrance |= 0x20000)
    char m_padc[0x5c - 0xc];
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
    char m_pad0[0x10];
    CEntranceHashTable m_10map; // +0x10
};

struct CEntranceResMgr {
    char m_pad0[0x2c];
    CEntranceSpriteMgr* m_2c; // +0x2c
};

// The active-anim descriptor the entrance player exposes (its first element's
// +0x14 frame number is the 2nd arg the frame helper consumes).
struct CEntranceAnimDescColl {
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
    // tail reads them via raw offsets off &player->m_1a0 instead.
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

    char m_pad0[0xc];
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
};
extern CAnimNameResolver g_animNameResolver; // DAT_006bf650

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
struct WwdGameReg {
    char m_pad0[0x68];
    i32 m_68; // +0x68  (SerializeMove mode-8: -> CGrunt::m_tileMgr)
    char m_pad6c[0x70 - 0x6c];
    GruntBoard* m_70; // +0x70  the level board
    char m_pad74[0x8];
};
extern WwdGameReg* g_gameReg; // ?g_gameReg@@3PAUWwdGameReg@@A @0x64556c

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
    i32 LookupTile(i32 x, i32 y, i32* outA, i32* outB, i32 flag); // FUN_00475af0 (ret 0x14)
    // The grunt anim-dispatch state machines drive the tile-mgr through two more
    // thunks (external/no-body, reloc-masked): a 6-arg arrival notify and a 4-arg
    // tile state set.
    void ArrivalNotify6(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f); // thunk (ret 0x18)
    void SetTileState4(i32 a, i32 b, i32 c, i32 d);                // thunk (ret 0x10)
    i32 ProbeFreeTile(i32 a, i32 b, void* c, i32 d, void* e, i32 f, i32 g, i32 h, i32 i); // probe
};

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

// A small owned sub-object the grunt destroys on teardown (slots +0x424/+0x428).
// Free() is __thiscall, no args, reloc-masked.
class CGruntSub {
public:
    void Free();
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
    void CommitNeighbor(i32 a, i32 b, i32 c, i32 d); // @0x5b050 (ret 0x10)
    CGrunt* FindGridNeighbor(i32 validate);          // @0x5b6f0 (ret 4)
    i32 UpdateGruntStatus();                         // @0x617c0 (ret 0)

    // --- animation resolvers (this TU's targets) ---
    i32 ResolveMovingAnimation();
    i32 ResolveDeathAnimation();
    i32 ResolveAnimation(); // (generic / "_JOY")
    i32 ResolveIdleAnimation();
    i32 ResolveBattlecryAnimation();

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
    CEntranceAnimPlayer* m_154; // +0x154 (entrance animation player)
    char m_pad158[0x15c - 0x158];
    i32 m_prevEntranceDesc; // +0x15c (= m_154->m_1b4 cache)
    char m_pad160[0x170 - 0x160];
    i32 m_entranceReason; // +0x170 (entrance-reason / movement state)
    i32 m_entrancePxX;    // +0x174 (SetEntrancePos: committed entrance position X, pixel)
    i32 m_entrancePxY;    // +0x178 (SetEntrancePos: committed entrance position Y, pixel)
    i32 m_lastTilePxX;    // +0x17c (LoadEntranceConfig: last occupied tile X, pixel; -1 = none)
    i32 m_lastTilePxY;    // +0x180 (LoadEntranceConfig: last occupied tile Y, pixel; -1 = none)
    char m_pad184[0x190 - 0x184];
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
    char m_pad1e8[0x1ec - 0x1e8];
    i32 m_tileOwnerHi; // +0x1ec
    i32 m_tileOwnerLo; // +0x1f0
    char m_pad1f4[0x1fc - 0x1f4];
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
    char m_pad448[0x460 - 0x448];
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
    i32 Save(CGruntArchive* ar);     // @0x53f90 serialize
    void ClearAllSprites();          // @0x4b240
    i32 CommitArrival();             // @0x4b130
    void ClearSubA();                // @0x57c10
    void ClearSubB();                // @0x57ce0
    void DestroyAnims();             // @0x57d80
    void AnimTeardownA();            // engine thunk (DestroyAnims step 1)
    void AnimTeardownB();            // engine thunk (DestroyAnims step 2)
    void ArrivalClaim(i32 a, i32 b); // CommitArrival's this->claim(1,1)
    // CommitArrival's six per-arrival this-call hooks (engine thunks).
    void ArrivalHook0();
    void ArrivalHook1();
    void ArrivalHook2();
    void ArrivalHook3();
    void ArrivalHook4();
    void ArrivalHook5();
    i32 CanShowStamina();              // @0x514a0
    void SetEntrancePos(i32 a, i32 b); // @0x4d060 (ret 8)
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
    void StepArrivalDrop(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f); // @0x4b370 (ret 0x18, /GX)
    i32 StepGruntMovement(); // @0x4c170 (ret 0)         - the per-tick move step
    i32 StepAnimDispatchA(i32 a, i32 b, i32 c, i32 d); // @0x52fb0 (ret 0x10)
    void StepCoordResolve();                           // @0x5f310 (ret 0)
    i32 StepAnimDispatchB();                           // @0x6a6d0 (ret 0)

    // The engine helpers these machines call (all external/no-body, reloc-masked;
    // modeled as __thiscall methods on the grunt so `mov ecx,this; ...; call`
    // falls out). Names describe the observed effect, not a recovered symbol.
    i32 IsDropReady();                                     // thunk_0x17df (drop-ready predicate)
    void ApplySetState1(i32 v);                            // thunk_0x4322 (1-arg state apply)
    void SetMoveStateA(i32 v, i32 a, i32 b, i32 c);        // thunk_0x3bd9 (4-arg state set)
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
};

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
