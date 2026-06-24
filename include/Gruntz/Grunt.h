// Grunt.h - the engine's CGrunt (the player/enemy grunt entity), the HUD
// sprite-creator cluster only. Field names are placeholders (m_<hexoffset>);
// ONLY the OFFSETS + code bytes are load-bearing (campaign doctrine). CGrunt is
// huge; this header models ONLY the member offsets the 7 HUD sprite creators
// touch + the small external classes they call into.
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
    unsigned m_8; // +0x08  failure flag word (|= 0x10000)
};

class CSpriteRegistrar {
public:
    int AddA(int a, int b, int c);
    int AddB(int a, int b);
    int AddC(int a, int b, int c);
    int AddD(int a, int b);

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
    int m_8; // +0x08   (dirty-flag word; BuildEntrance |= 0x20000)
    char m_padc[0x5c - 0xc];
    int m_5c; // +0x5c
    int m_60; // +0x60
    char m_pad64[0x74 - 0x64];
    int m_74; // +0x74   (entrance: latched anim id; cmp 0xcf850)
    char m_pad78[0x134 - 0x78];
    int m_134; // +0x134  (arrival: view-cull mode cleared)
    int m_138; // +0x138  (arrival: view-cull, cleared)
    int m_13c; // +0x13c  (arrival: view-cull, cleared)
    int m_140; // +0x140  (arrival: view-cull, cleared)
    char m_pad144[0x188 - 0x144];
    int m_188; // +0x188  (cue arg)
};

// ---------------------------------------------------------------------------
// The global HUD sprite factory, reached via the global registry ptr -> +0x30 -> +0x8.
// CreateSprite is __thiscall(this, 0, geoB, geoA, hint, name, kind) ret 0x18
// Modeled as a method on the registry singleton so the call shape
// (factory-this = g->m_30->m_8) + the 6-arg push fall out; external/no-body so
// the `call rel32` reloc-masks.
// ---------------------------------------------------------------------------
struct CSpriteFactory {
    CHudSprite* CreateSprite(int kind, int geoB, int geoA, int hint, const char* name, int flags);
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
//     resolver caches m_1b4 into CGrunt::m_40 before the call, and Idle reads
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
    int m_14; // +0x14
};

struct CAnimDescColl {
    char m_pad0[0xc];
    CAnimElem** m_c; // +0x0c  element vector (Idle reads *m_c = first elem)
    int m_10;        // +0x10  element count (Idle: >0 gate)
};

class CGruntAnimSub {
public:
    void SetGeometry(int srcSprite); // (this = animState+0x1a0)
};

class CGruntAnimState {
public:
    void SetAnim(const char* key);              // (ret 4)
    void SetAnimEx(const char* key, int frame); // (ret 8)

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

// A per-grunt time/seed default the Moving resolver copies into m_88.
extern int g_movingSeed;

// The engine LCG rand() (no args) the Moving/Idle/Battlecry resolvers
// use to pick an animation index / start time.
extern "C" int GruntRand(); // stub

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
    void Cue(int a, int b, int c, int d, int e);             // via thunk 0x33b4
    void CueA(CGrunt* g, int b, int c, int d, int e, int f); // 6-arg entrance cue (ret 0x18)
};

// The entrance-reset (Stub_062e10) cue-gate visibility helper (thunk_FUN_0046b330,
// __cdecl(viewport, x, y) ret int): tests whether the grunt's HUD point is inside
// the viewport rect. External/no-body (reloc-masked).
int CueVisible(int viewport, int x, int y);

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
    int Lookup(const char* szName, CSprite** ppOut); // (ret 8)
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
    int** m_c; // +0x0c  element vector (first elem = *m_c)
    int m_10;  // +0x10  element count (>0 gate)
};

class CEntranceAnimSub {
public:
    void SetGeometry(int srcSprite); // FUN_0055c2d0 (this = player+0x1a0, ret 4)
    // The geometry-state setter LoadEntranceConfig calls on entry; returns 1 when
    // the player is ready (FUN_0055c360, __thiscall ret 4 = 1 stack arg). Same
    // engine fn as SpriteResource's SetGeoSource, but the int return is used here.
    int SetGeoSourceR(int src); // FUN_0055c360
    // Data-less view: the geometry sub-player's m_20/m_28 (abs CGrunt+0x154+0x1a0
    // +0x20/+0x28) live PAST the player's own m_1b4, so they are not modeled as
    // embedded data here (that would corrupt m_1b4's offset). LoadEntranceConfig's
    // tail reads them via raw offsets off &player->m_1a0 instead.
};

// A per-cell entrance record (0x68-byte stride at CGrunt+0x474). GetName(flag)
// resolves the cell's frame name (__thiscall, 1 arg). External (reloc-masked).
class CGruntCell {
public:
    const char* GetName(int flag);
};

class CEntranceAnimPlayer {
public:
    void SetAnimFrame(const char* name, int frame); // FUN_005504d0-class (ret 8)

    char m_pad0[0xc];
    CEntranceResMgr* m_c; // +0x0c  resource object (lookup table holder)
    char m_pad10[0x1a0 - 0x10];
    CEntranceAnimSub m_1a0; // +0x1a0 geometry sub-player
    char m_pad1a4[0x1b4 - 0x1a4];
    CEntranceAnimDescColl* m_1b4; // +0x1b4 active-anim descriptor
    char m_pad1b8[0x1c0 - 0x1b8];
    int m_1c0; // +0x1c0 (entrance-done flag B: 0 -> run reset)
    char m_pad1c4[0x1c8 - 0x1c4];
    int m_1c8; // +0x1c8 (entrance-done flag A: nonzero -> bail)
};

// The frame helper BuildEntranceAnimation calls at the tail (FUN_005504d0):
// __stdcall(keyStr, frameNum) - callee-pops (no `add esp` at the site). External.
void __stdcall EntranceApplyFrame(const char* keyStr, int frameNum);

// The entrance-anim-set source object (the global at DAT_006bf620). Its
// LookupAnimSet (FUN_0056d190, __thiscall ret 4) takes a single-char anim key
// and returns the new active-anim-set node that gets latched into m_14->m_1c.
// External/no-body (reloc-masked); the `push key; mov ecx, &g_entranceAnimSrc;
// call` is the load-bearing shape.
class CEntranceAnimSrc {
public:
    int LookupAnimSet(const char* key); // FUN_0056d190 (ret 4)
};
extern CEntranceAnimSrc g_entranceAnimSrc; // DAT_006bf620
#define EntranceLookupAnimSet(k) (g_entranceAnimSrc.LookupAnimSet(k))

// The grunt's current-anim-name resolver (the global at DAT_006bf650). Its
// GetNameRecord (thunk_FUN_004310f0, __thiscall ret 4) maps an anim-set node to
// a record whose first field is the anim's name char*. External/no-body.
class CAnimNameResolver {
public:
    char** GetNameRecord(void* node); // thunk_FUN_004310f0 (ret 4)
};
extern CAnimNameResolver g_animNameResolver; // DAT_006bf650

// The "focused grunt" sentinel the on-screen flag compares m_1ec against
// (DAT_00644c54, reloc-masked).
extern int g_focusedGruntSentinel; // DAT_00644c54

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
    void SetTile(int a, int b, int c, int d);   // thunk_FUN_0046bcb0
    void ClaimTile(int a, int b, int c, int d); // thunk_FUN_0046bfd0
    int ReleaseTile(int a, int b);              // thunk_FUN_004784d0
    void PostWire();                            // WireTileSwitchLogic (0-arg)
    void NotifyArrival(int a, int b);           // thunk_FUN_0046da60 (2-arg)
};

// The registry focused-grunt slot the arrival gate reads: an array at
// g_pGameRegistry+0x150, stride 0x238 (= 71*8) indexed by the grunt's m_1ec.
// Each slot's +0x14 is a non-null gate the arrival path checks. External view.
struct CFocusSlot {
    char m_pad0[0x14];
    int m_14; // +0x14
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
    virtual void slot2c();
    virtual void Write(const void* data, int size); // vtable slot +0x30
};

// The grunt's name-id resolver the Save reaches via m_158->m_c->m_2c: maps an
// integer id to its name CString (returned by value). __thiscall, ret 4.
class CGruntNameMap {
public:
    CString LookupName(int id);
};

// The +0x158 "type catalog" object: Save reads its m_c (a non-null owner that
// also holds the name-id map at m_2c). External; modeled minimally.
struct CGruntTypeCatalog {
    char m_pad0[0xc];
    CGruntNameMap* m_c; // +0x0c  owner -> name-id map
};

// The global serialize counter Save bumps before each variable-length record
// (DAT_00629ad0). TU-local (reloc-masked); shared in retail.
extern int g_serialCounter;

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
extern int g_freePoolBase;    // DAT_0064554c (raw subtrahend)

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
//   +0x1ec  m_1ec / +0x1f0 m_1f0  (Add* args)
//   +0x238  m_238     (WingzTime gate)
//   +0x3ec  m_3ec     (Health stat / Add arg)
//   +0x3f0  m_3f0     (Stamina stat / gate / Add arg)
//   +0x3f4  m_3f4     (ToyTime gate / Add arg)
//   +0x3f8  m_3f8     (WingzTime gate / Add arg)
// ---------------------------------------------------------------------------
class CGrunt {
public:
    int CreateHealthSprite();
    int CreateToySprite();
    int CreateStaminaSprite();
    int CreateToyTimeSprite();
    int CreateWingzTimeSprite();
    int CreatePowerupSprite(int a); // (ret 4)
    int CreateSelectedSprite();

    void ReadConfigFromButeMgr();
    void LoadGruntMovingDeathConfig();

    // --- animation resolvers (this TU's targets) ---
    int ResolveMovingAnimation();
    int ResolveDeathAnimation();
    int ResolveAnimation(); // (generic / "_JOY")
    int ResolveIdleAnimation();
    int ResolveBattlecryAnimation();

    char m_pad0[0x10];
    CGruntHud* m_10;       // +0x10
    CAnimLookupNode* m_14; // +0x14  (anim-set lookup holder)
    char m_pad18[0x30 - 0x18];
    int m_30; // +0x30
    char m_pad34[0x38 - 0x34];
    CGruntAnimState* m_38; // +0x38  (animation player)
    char m_pad3c[0x40 - 0x3c];
    int m_40; // +0x40  (cached m_38->m_1b4)
    char m_pad44[0x54 - 0x44];
    CString m_typeName;          // +0x54  (grunt-type name CString)
    int m_58[(0x68 - 0x58) / 4]; // +0x58  (Idle geometry sources)
    int m_68[(0x74 - 0x68) / 4]; // +0x68  (Battlecry geometry sources)
    int m_74;                    // +0x74  (generic geometry source)
    int m_78;                    // +0x78  (death geometry source)
    int m_7c;                    // +0x7c  (moving geometry source)
    char m_pad80[0x88 - 0x80];
    int m_88; // +0x88  (moving: = g_movingSeed)
    int m_8c; // +0x8c  (moving: = 0)
    int m_90; // +0x90  (moving: randomized time)
    int m_94; // +0x94  (moving: = 0)
    char m_pad98[0xa8 - 0x98];
    int m_a8; // +0xa8  (resolve gate / dirty flag)
    int m_ac; // +0xac  (cue arg)
    char m_padb0[0x154 - 0xb0];
    CEntranceAnimPlayer* m_154; // +0x154 (entrance animation player)
    char m_pad158[0x15c - 0x158];
    int m_15c; // +0x15c (= m_154->m_1b4 cache)
    char m_pad160[0x170 - 0x160];
    int m_170; // +0x170 (entrance-reason / movement state)
    char m_pad174[0x17c - 0x174];
    int m_17c; // +0x17c (LoadEntranceConfig: last occupied tile X, pixel; -1 = none)
    int m_180; // +0x180 (LoadEntranceConfig: last occupied tile Y, pixel; -1 = none)
    char m_pad184[0x1b8 - 0x184];
    CHudSprite* m_selectedSprite; // +0x1b8
    CHudSprite* m_toySprite;      // +0x1bc
    char m_pad1c0[0x1c4 - 0x1c0];
    CHudSprite* m_healthSprite;    // +0x1c4
    CHudSprite* m_staminaSprite;   // +0x1c8
    CHudSprite* m_toyTimeSprite;   // +0x1cc
    CHudSprite* m_wingzTimeSprite; // +0x1d0
    CHudSprite* m_powerupSprite;   // +0x1d4
    int m_1d8;                     // +0x1d8 (entrance-arrival gate)
    char m_pad1dc[0x1e4 - 0x1dc];
    int m_1e4; // +0x1e4 (entrance: set to 1)
    char m_pad1e8[0x1ec - 0x1e8];
    int m_1ec; // +0x1ec
    int m_1f0; // +0x1f0
    char m_pad1f4[0x1fc - 0x1f4];
    int m_1fc; // +0x1fc (entrance: cleared)
    char m_pad200[0x230 - 0x200];
    int m_230; // +0x230 (entrance-arrival: cleared)
    char m_pad234[0x238 - 0x234];
    int m_238; // +0x238
    char m_pad23c[0x244 - 0x23c];
    int m_244; // +0x244 (entrance-reset: 0 then 1 = "applied" flag)
    int m_248; // +0x248 (arrival flag word; |= 0x18040402)
    char m_pad24c[0x25c - 0x24c];
    int m_25c;            // +0x25c (entrance: set to 1)
    CGruntTileMgr* m_260; // +0x260 (path/occupancy sub-manager)
    char m_pad264[0x2d0 - 0x264];
    int m_2d0; // +0x2d0 (arrival: = 4)
    int m_2d4; // +0x2d4 (arrival: = 0)
    char m_pad2d8[0x2dc - 0x2d8];
    int m_2dc; // +0x2dc (defender radius / arrival kind)
    char m_pad2e0[0x2f0 - 0x2e0];
    int m_2f0; // +0x2f0 (arrival: = -1)
    int m_2f4; // +0x2f4 (arrival: = -1)
    char m_pad2f8[0x300 - 0x2f8];
    int m_300; // +0x300 (arrival: = m_17c)
    int m_304; // +0x304 (arrival: = m_180)
    int m_308; // +0x308 (arrival: cleared)
    int m_30c; // +0x30c (arrival: cleared)
    int m_310; // +0x310 (arrival: cleared)
    int m_314; // +0x314 (arrival: cleared)
    char m_pad318[0x364 - 0x318];
    int m_364; // +0x364 (entrance: set to 1)
    char m_pad368[0x3ac - 0x368];
    int m_3ac[3]; // +0x3ac (entrance geometry sources; [0]=default, [1]/[2] variants)
    char m_pad3b8[0x3ec - 0x3b8];
    int m_3ec; // +0x3ec
    int m_3f0; // +0x3f0
    int m_3f4; // +0x3f4
    int m_3f8; // +0x3f8
    char m_pad3fc[0x420 - 0x3fc];
    int m_420; // +0x420 (arrival-claimed latch)
    char m_pad424[0x43c - 0x424];
    int m_43c[3]; // +0x43c (entrance-cell triple: [0]=col, [1]=row, [2]=m_444 reason)
    char m_pad448[0x464 - 0x448];
    int m_464; // +0x464 (entrance-reset latch flag)
    char m_pad468[0x474 - 0x468];
    char m_474[1]; // +0x474 (entrance-cell record table; 0x68-byte stride records)
    char m_pad475[0x820 - 0x475];
    int m_820; // +0x820 (idle-timer: low)
    int m_824; // +0x824 (idle-timer: high)
    int m_828; // +0x828 (idle-timer: delay low)
    int m_82c; // +0x82c (idle-timer: delay high)
    int m_830; // +0x830 (idle-anchor: low)
    int m_834; // +0x834 (idle-anchor: high)
    int m_838; // +0x838 (idle-window: low = 0x3a98)
    int m_83c; // +0x83c (idle-window: high = 0)
    int m_840; // +0x840 (entrance: = g_645588 game clock, low dword)
    int m_844; // +0x844 (entrance: = 0, high dword)
    int m_848; // +0x848 (entrance: = EntranceSafeTime config)
    int m_84c; // +0x84c (entrance: = 0)
    char m_pad850[0x858 - 0x850];
    int m_858; // +0x858 (entrance: = 0)
    int m_85c; // +0x85c (entrance: = 0)

    // Engine-label backlog stubs.
    void Stub_047a10();
    void Stub_048400();
    void Stub_048470(int a, int b);        // (2-arg; called from LoadEntranceConfig tail)
    void Stub_062e10(int a, int b, int c); // (ret 0xc) - 3-arg entrance reset
    void Stub_0633e0();
    void EntrancePrepare(); // thunk_FUN_0044b240 (void this-method, external)
    void BuildEntranceAnimation(int mode);
    void LoadEntranceConfig();
    // LoadEntranceConfig tail helpers (this-methods reached via incremental-link
    // thunks; external/no-body, reloc-masked).
    void EntranceFinishWire(int a, int b);  // thunk_FUN_00449c60 (2-arg)
    void EntranceOnReleased();              // thunk_FUN_0044b130 (0-arg)
    void EntranceArrivalHook(int a, int b); // thunk_FUN_0044d060 (2-arg; arrival commit)

    // ---- migrated CGrunt cluster (ex-CUserLogic_*) ----
    int Save(CGruntArchive* ar);     // @0x53f90 serialize
    void ClearAllSprites();          // @0x4b240
    int CommitArrival();             // @0x4b130
    void ClearSubA();                // @0x57c10
    void ClearSubB();                // @0x57ce0
    void DestroyAnims();             // @0x57d80
    void AnimTeardownA();            // engine thunk (DestroyAnims step 1)
    void AnimTeardownB();            // engine thunk (DestroyAnims step 2)
    void ArrivalClaim(int a, int b); // CommitArrival's this->claim(1,1)
    // CommitArrival's six per-arrival this-call hooks (engine thunks).
    void ArrivalHook0();
    void ArrivalHook1();
    void ArrivalHook2();
    void ArrivalHook3();
    void ArrivalHook4();
    void ArrivalHook5();
    int CanShowStamina();              // @0x514a0
    void SetEntrancePos(int a, int b); // @0x4d060 (ret 8)
    void ComputeFacing(double dt);     // @0x57060 (ret 8)
    void FreeNameList();               // @0x48360
    int ResetGeometry();               // @0x616e0
    void DispatchVtbl24();             // @0x6b260 (jmp [vtbl+0x24])
};

// CGrunt::IsSameType(a, b) @0x3c7f0 - a free (__cdecl) comparator: returns
// (a->m_8 == b->m_8). Not a member (reads both args off the stack).
int CGrunt_IsSameType(CGrunt* a, CGrunt* b);

// CGrunt::TileSwitch(...) @0x4b320 - a 6-arg (__stdcall, ret 0x18) passthrough
// that scales the first two args to tile pixel coords (*0x20+0x10) and forwards
// all six to an engine helper. External callee reloc-masks.
void __stdcall CGrunt_TileSwitch(int a, int b, int c, int d, int e, int f);
// The engine tile-switch helper TileSwitch forwards to (__stdcall ret 0x18).
void __stdcall GruntTileSwitchImpl(int a, int b, int c, int d, int e, int f);

#endif // SRC_GRUNTZ_GRUNT_H
