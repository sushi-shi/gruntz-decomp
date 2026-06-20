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
    unsigned m_8;       // +0x08  failure flag word (|= 0x10000)
};

class CSpriteRegistrar {
public:
    int AddA(int a, int b, int c);
    int AddB(int a, int b);
    int AddC(int a, int b, int c);
    int AddD(int a, int b);

    char m_pad0[0x38];
    CSpriteRegRecord *m_38;         // +0x38  (failure-flag record holder)
};

// ---------------------------------------------------------------------------
// The HUD sprite the factory builds. The creators read sprite->m_7c (an inner
// object), call sprite->m_7c->[0x10](sprite) (init), and reach the registrar at
// sprite->m_7c->m_18. Layout-free apart from m_7c.
// ---------------------------------------------------------------------------
struct CSpriteInner {
    char m_pad0[0x10];
    void (*m_init)(void *self);     // +0x10  init virtual (called with sprite)
    char m_pad14[0x18 - 0x14];
    CSpriteRegistrar *m_18;         // +0x18  the registrar
};

struct CHudSprite {
    char m_pad0[0x7c];
    CSpriteInner *m_7c;             // +0x7c
};

// ---------------------------------------------------------------------------
// this->m_10 (a HUD/level geometry source). The factory's two geometry args are
// m_10->m_5c and m_10->m_60 (the latter optionally minus a per-sprite constant).
// ---------------------------------------------------------------------------
struct CGruntHud {
    char m_pad0[0x8];
    int  m_8;           // +0x08   (dirty-flag word; BuildEntrance |= 0x20000)
    char m_padc[0x5c - 0xc];
    int  m_5c;          // +0x5c
    int  m_60;          // +0x60
    char m_pad64[0x74 - 0x64];
    int  m_74;          // +0x74   (entrance: latched anim id; cmp 0xcf850)
    char m_pad78[0x188 - 0x78];
    int  m_188;         // +0x188  (cue arg)
};

// ---------------------------------------------------------------------------
// The global HUD sprite factory, reached via the global registry ptr -> +0x30 -> +0x8.
// CreateSprite is __thiscall(this, 0, geoB, geoA, hint, name, kind) ret 0x18
// Modeled as a method on the registry singleton so the call shape
// (factory-this = g->m_30->m_8) + the 6-arg push fall out; external/no-body so
// the `call rel32` reloc-masks.
// ---------------------------------------------------------------------------
struct CSpriteFactory {
    CHudSprite *CreateSprite(int kind, int geoB, int geoA,
                             int hint, const char *name, int flags);
};
// CSpriteFactoryHolder (the registry +0x30 holder) lives in <Gruntz/CGameRegistry.h>.

class CGruntCueSink;    // defined below (the 5-arg on-screen cue receiver)

// CGameRegistry - the shared global singleton (*g_pGameRegistry). The CGrunt
// resolvers below read the visible-bounds gate (m_134, m_13c..m_148) and fire
// m_60->Cue.
#include <Gruntz/CGameRegistry.h>

// The global manager pointer.
extern CGameRegistry *g_pGameRegistry;

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
CString __stdcall operator+(const char *lhs, const CString &rhs);
CString __stdcall operator+(const CString &lhs, const char *rhs);

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
    int  m_14;          // +0x14
};

struct CAnimDescColl {
    char       m_pad0[0xc];
    CAnimElem **m_c;    // +0x0c  element vector (Idle reads *m_c = first elem)
    int        m_10;    // +0x10  element count (Idle: >0 gate)
};

class CGruntAnimSub {
public:
    void SetGeometry(int srcSprite);    // (this = animState+0x1a0)
};

class CGruntAnimState {
public:
    void SetAnim(const char *key);              // (ret 4)
    void SetAnimEx(const char *key, int frame); // (ret 8)

    char           m_pad0[0x1a0];
    CGruntAnimSub  m_1a0;       // +0x1a0  (geometry sub-player)
    char           m_pad1a4[0x1b4 - 0x1a4];
    CAnimDescColl *m_1b4;       // +0x1b4  active-anim descriptor
};

// The animation-set record the lookup tree (a CButeTree) returns;
// stored into CGrunt::m_14->m_1c. m_1c holds the resolved anim-set node.
struct CAnimLookupNode {
    char  m_pad0[0x1c];
    void *m_1c;         // +0x1c
};

// CButeTree::Find (__thiscall ret 4) - the shared keyed lookup.
class CAnimLookupTree {
public:
    void *Find(const char *key);    // stub
};

// The global animation lookup tree instance.
extern CAnimLookupTree g_animLookupTree;

// A per-grunt time/seed default the Moving resolver copies into m_88.
extern int g_movingSeed;

// The engine LCG rand() (no args) the Moving/Idle/Battlecry resolvers
// use to pick an animation index / start time.
extern "C" int GruntRand();     // stub

// ---------------------------------------------------------------------------
// The on-screen-cue receiver reached via g_pGameRegistry->m_60 (a __thiscall
// ret 0x14 = 5 stack args). The resolvers fire a 5-arg cue when the
// grunt is on-screen (m_134 == 1 -> 4-way visible-bounds test) or unconditionally
// otherwise. External/no-body (reloc-masked; reached via incremental-link thunk).
//
// BuildEntranceAnimation fires a SIX-arg variant (a different cue overload, also
// via g->m_60); modeled as a second method (CueA, ret 0x18). Both reloc-mask.
// ---------------------------------------------------------------------------
class CGrunt;   // fwd-declared for CueA's first arg

class CGruntCueSink {
public:
    void Cue(int a, int b, int c, int d, int e);            // via thunk 0x33b4
    void CueA(CGrunt *g, int b, int c, int d, int e, int f); // 6-arg entrance cue (ret 0x18)
};

// ---------------------------------------------------------------------------
// The entrance-animation sub-object @CGrunt+0x154: a per-grunt animation player.
// BuildEntranceAnimation reaches a name->sprite-set lookup table through
// player->m_c (a resource object) +0x2c +0x10 (the embedded map) and drives the
// geometry sub-player @+0x1a0 with the resolved sprite. The map Lookup, the
// geometry setter, and the frame helper are all external/no-body (reloc-masked).
// ---------------------------------------------------------------------------
struct CSprite;     // opaque looked-up sprite

class CEntranceHashTable {
public:
    int Lookup(const char *szName, CSprite **ppOut);    // (ret 8)
};

struct CEntranceSpriteMgr {
    char               m_pad0[0x10];
    CEntranceHashTable m_10map;     // +0x10
};

struct CEntranceResMgr {
    char                m_pad0[0x2c];
    CEntranceSpriteMgr *m_2c;       // +0x2c
};

// The active-anim descriptor the entrance player exposes (its first element's
// +0x14 frame number is the 2nd arg the frame helper consumes).
struct CEntranceAnimDescColl {
    char        m_pad0[0xc];
    int       **m_c;        // +0x0c  element vector (first elem = *m_c)
    int         m_10;       // +0x10  element count (>0 gate)
};

class CEntranceAnimSub {
public:
    void SetGeometry(int srcSprite);    // FUN_0055c2d0 (this = player+0x1a0, ret 4)
};

class CEntranceAnimPlayer {
public:
    char                   m_pad0[0xc];
    CEntranceResMgr       *m_c;         // +0x0c  resource object (lookup table holder)
    char                   m_pad10[0x1a0 - 0x10];
    CEntranceAnimSub       m_1a0;       // +0x1a0 geometry sub-player
    char                   m_pad1a4[0x1b4 - 0x1a4];
    CEntranceAnimDescColl *m_1b4;       // +0x1b4 active-anim descriptor
};

// The frame helper BuildEntranceAnimation calls at the tail (FUN_005504d0):
// __stdcall(keyStr, frameNum) - callee-pops (no `add esp` at the site). External.
void __stdcall EntranceApplyFrame(const char *keyStr, int frameNum);

// The entrance-anim-set source object (the global at DAT_006bf620). Its
// LookupAnimSet (FUN_0056d190, __thiscall ret 0) returns the new active-anim-set
// node that gets latched into m_14->m_1c. External/no-body (reloc-masked); the
// `mov ecx, &g_entranceAnimSrc; call` is the load-bearing shape.
class CEntranceAnimSrc {
public:
    int LookupAnimSet();                // FUN_0056d190
};
extern CEntranceAnimSrc g_entranceAnimSrc;   // DAT_006bf620
#define EntranceLookupAnimSet() (g_entranceAnimSrc.LookupAnimSet())

// The "focused grunt" sentinel the on-screen flag compares m_1ec against
// (DAT_00644c54, reloc-masked).
extern int g_focusedGruntSentinel;      // DAT_00644c54

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

    // --- animation resolvers (this TU's targets) ---
    int ResolveMovingAnimation();
    int ResolveDeathAnimation();
    int ResolveAnimation();          // (generic / "_JOY")
    int ResolveIdleAnimation();
    int ResolveBattlecryAnimation();

    char             m_pad0[0x10];
    CGruntHud       *m_10;                  // +0x10
    CAnimLookupNode *m_14;                  // +0x14  (anim-set lookup holder)
    char             m_pad18[0x30 - 0x18];
    int              m_30;                  // +0x30
    char             m_pad34[0x38 - 0x34];
    CGruntAnimState *m_38;                  // +0x38  (animation player)
    char             m_pad3c[0x40 - 0x3c];
    int              m_40;                  // +0x40  (cached m_38->m_1b4)
    char             m_pad44[0x54 - 0x44];
    CString        m_typeName;            // +0x54  (grunt-type name CString)
    int              m_58[(0x68 - 0x58) / 4];   // +0x58  (Idle geometry sources)
    int              m_68[(0x74 - 0x68) / 4];   // +0x68  (Battlecry geometry sources)
    int              m_74;                  // +0x74  (generic geometry source)
    int              m_78;                  // +0x78  (death geometry source)
    int              m_7c;                  // +0x7c  (moving geometry source)
    char             m_pad80[0x88 - 0x80];
    int              m_88;                  // +0x88  (moving: = g_movingSeed)
    int              m_8c;                  // +0x8c  (moving: = 0)
    int              m_90;                  // +0x90  (moving: randomized time)
    int              m_94;                  // +0x94  (moving: = 0)
    char             m_pad98[0xa8 - 0x98];
    int              m_a8;                  // +0xa8  (resolve gate / dirty flag)
    int              m_ac;                  // +0xac  (cue arg)
    char             m_padb0[0x154 - 0xb0];
    CEntranceAnimPlayer *m_154;             // +0x154 (entrance animation player)
    char             m_pad158[0x15c - 0x158];
    int              m_15c;                 // +0x15c (= m_154->m_1b4 cache)
    char             m_pad160[0x1b8 - 0x160];
    CHudSprite *m_selectedSprite;           // +0x1b8
    CHudSprite *m_toySprite;                // +0x1bc
    char        m_pad1c0[0x1c4 - 0x1c0];
    CHudSprite *m_healthSprite;             // +0x1c4
    CHudSprite *m_staminaSprite;            // +0x1c8
    CHudSprite *m_toyTimeSprite;            // +0x1cc
    CHudSprite *m_wingzTimeSprite;          // +0x1d0
    CHudSprite *m_powerupSprite;            // +0x1d4
    char        m_pad1d8[0x1e4 - 0x1d8];
    int         m_1e4;                      // +0x1e4 (entrance: set to 1)
    char        m_pad1e8[0x1ec - 0x1e8];
    int         m_1ec;                      // +0x1ec
    int         m_1f0;                      // +0x1f0
    char        m_pad1f4[0x1fc - 0x1f4];
    int         m_1fc;                      // +0x1fc (entrance: cleared)
    char        m_pad200[0x238 - 0x200];
    int         m_238;                      // +0x238
    char        m_pad23c[0x25c - 0x23c];
    int         m_25c;                      // +0x25c (entrance: set to 1)
    void       *m_260;                      // +0x260 (a sub-manager `this`, used by LoadEntranceConfig)
    char        m_pad264[0x3ec - 0x264];
    int         m_3ec;                      // +0x3ec
    int         m_3f0;                      // +0x3f0
    int         m_3f4;                      // +0x3f4
    int         m_3f8;                      // +0x3f8

    // Engine-label backlog stubs.
    void Stub_047a10();
    void Stub_048400();
    void Stub_048470();
    void Stub_062e10(int a, int b, int c);  // (ret 0xc) - 3-arg entrance reset
    void Stub_0633e0();
    void EntrancePrepare();                 // thunk_FUN_0044b240 (void this-method, external)
    void BuildEntranceAnimation(int mode);
    void LoadEntranceConfig();
};

#endif // SRC_GRUNTZ_GRUNT_H
