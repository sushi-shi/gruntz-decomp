// Projectile.h - the CMovingLogic / CProjectile game-object subtree
// (C:\Proj\Gruntz), continuing the CUserBase/CUserLogic hierarchy in
// <Gruntz/UserLogic.h>.
//
// Hierarchy (RTTI in GRUNTZ.EXE):
//     CUserBase                         vftable 0x5e70b4
//       +-- CUserLogic : CUserBase      vftable 0x5e705c
//             +-- CMovingLogic          vftable 0x5e87ac  (17 virtuals)
//                   +-- CProjectile     vftable 0x5e798c  (18 virtuals)
//
// CMovingLogic adds the projectile/moving-object motion state: a band of
// per-axis bookkeeping ints (+0x38..+0x10c) plus twelve `double` coordinate
// bounds (+0xa8..+0x13f) seeded to a default [-INF,+INF]-style box. Its ctor
// (out-of-line 0x13940) is also INLINED into the leaf ctors; modeled inline here
// so MSVC folds it into CProjectile::CProjectile.
//
// CProjectile adds the CObList at +0x204 (the projectile's tracked-hit list, MFC
// block size 10) and its own most-derived vftable. The no-arg ctor (0x126e0)
// folds the whole CMovingLogic init, constructs the list, and stamps its vptr.
#ifndef GRUNTZ_PROJECTILE_H
#define GRUNTZ_PROJECTILE_H

#include <Mfc.h> // CObList (+0x204 member)
#include <Gruntz/UserLogic.h>
#include <rva.h>

// The default coordinate bounds the CMovingLogic ctor seeds. Retail copies them
// from a shared 8-byte .rdata double (0x5f04b0 = MIN, 0x5f04b8 = MAX) via plain
// integer dword moves - so they are modeled as named double constants the ctor
// reads, not foldable literals (a literal would materialize 32-bit immediates).
// -0x7fffffff and +0x7ffffffe as doubles.
extern const double g_movingLogicMin; // 0x5f04b0 (-2147483647.0)
extern const double g_movingLogicMax; // 0x5f04b8 (2147483646.0)

// The 1-arg ctor's velocity/scale constants: g_645588 (spawn seed int, scaled by
// the .rdata double g_5eaa88) and g_5f04e8 (the default-Z .rdata int). All read
// unsigned -> double (the fild {lo,0} idiom).
extern "C" u32 g_645588;      // 0x645588
extern const double g_5eaa88; // 0x5eaa88
extern u32 g_5f04e8;          // 0x5f04e8

// ---------------------------------------------------------------------------
// CMovingLogic : CUserLogic - moving-object motion state. 17 virtuals
// (vftable 0x5e87ac); only the offsets the ctor initializes are modeled.
//
// The +0x38..+0x10c band is a set of int pairs zeroed at construction; the
// +0xa8..+0x13f band is twelve doubles seeded to the default min/max bounds.
// ---------------------------------------------------------------------------
// The +0x38 motion band the CMovingLogic ctor initializes. The no-arg ctor
// inlines its zero-init; the 1-arg ctor calls the out-of-line copy (0x136d0)
// and its 11-double coordinate setter (0x58bc0, returns 1). __thiscall on
// this+0x38. Reached via a cast so the shared flat-int layout (proven by the
// byte-exact no-arg ctor) is untouched.
struct CProjMotionBand {
    void Init(); // 0x136d0
    i32 SetCoords(
        double,
        double,
        double,
        double,
        double,
        double,
        double,
        double,
        double,
        double,
        double
    ); // 0x58bc0
};

// The per-type config the 1-arg ctor reads its coordinate bounds from
// (CUserLogic::m_14, the bound object's +0x7c aux). Only the four bound ints.
struct CProjBoundCfg {
    char _00[0x2c];
    i32 m_2c; // +0x2c  -> lo bound A (0 => default MIN)
    i32 m_30; // +0x30  -> hi bound A (0 => default MAX)
    i32 m_34; // +0x34  -> lo bound B (0 => default MIN)
    i32 m_38; // +0x38  -> hi bound B (0 => default MAX)
};

class CMovingLogic : public CUserLogic {
public:
    CMovingLogic();
    CMovingLogic(CGameObject* owner); // 1-arg (folds into CProjectile(owner))
    virtual ~CMovingLogic() OVERRIDE; // most-derived dtor (slot 0)
    // CMovingLogic adds three virtuals over the CUserBase/CUserLogic chain; the
    // last lands at vtable offset 0x40 (the slot ReleaseDeferred dispatches).
    virtual i32 MovingLogicVfunc();  // slot 14
    virtual i32 MovingLogicVfunc2(); // slot 15
    virtual i32 MovingLogicVfunc3(); // slot 16 (offset 0x40)

    // CMovingLogic's own data begins at +0x40 (CUserLogic ends at +0x40). The
    // ctor also re-zeroes the inherited CUserLogic m_38/m_3c.
    // +0x40..+0x8f: twenty motion ints.
    i32 m_40, m_44, m_48, m_4c, m_50, m_54, m_58, m_5c;
    i32 m_60, m_64, m_68, m_6c, m_70, m_74, m_78, m_7c, m_80, m_84, m_88, m_8c;
    char m_pad90[0xa8 - 0x90];
    // +0xa8..+0xd7: three "ranges", each {double lo, double hi}.
    double m_a8, m_b0, m_b8; // the three lo bounds (default MIN)
    double m_c0, m_c8, m_d0; // the three hi bounds (default MAX)
    char m_padd8[0xf0 - 0xd8];
    i32 m_f0;                     // +0xf0
    char m_padf4[0xf8 - 0xf4];    // gap
    i32 m_f8, m_fc, m_100, m_104; // +0xf8..+0x107
    i32 m_108, m_10c;             // +0x108..+0x10f
    // +0x110..+0x13f: six doubles, all seeded to MAX.
    double m_110, m_118, m_120, m_128, m_130, m_138;
};

// The animation sub-object embedded in a render object at +0x1a0; its setter
// FUN_0055c360 (0x15c360, __thiscall, 1 arg) re-targets the active animation, and
// Setup (0x15c2d0, __thiscall, 1 arg) installs the resolved frame-0 sprite.
struct CProjAnim {
    i32 SetAnim(u32 mode);   // 0x15c360
    i32 Setup(void* frame0); // 0x15c2d0
};

// The name->sprite geometry map the sprite object owns (the CMapStringToOb the
// loaders Lookup by frame name). Reached via m_154->m_c->m_2c, map embedded @+0x10.
struct CProjSpriteMap {
    i32 Lookup(const char* key, void** out); // 0x1b8438 (__thiscall, ret 8)
};
struct CProjSpriteMgr {
    char m_pad00[0x10];
    CProjSpriteMap m_10; // +0x10  the lookup map
};
struct CProjResMgr {
    char m_pad00[0x2c];
    CProjSpriteMgr* m_2c; // +0x2c
};

// A render object the projectile owns/points at (the +0x154 sprite/animation and
// the +0x1fc shadow companion). Only the offsets the reconstructed methods touch
// are modeled: +0x08 flag word, +0x0c resource host (frame lookup), +0x40 flag
// word, +0x5c/+0x60 screen position, +0x1a0 animation sub-object, +0x1b4 geometry
// word, +0x1c0/+0x1c8 state gates.
struct CProjRenderObj {
    char m_pad00[0x08];
    u32 m_08;         // +0x08  flag word (|= 0x10000)
    CProjResMgr* m_c; // +0x0c  resource host (name->sprite map via m_2c)
    char m_pad10[0x40 - 0x10];
    u32 m_40; // +0x40  flag word (&= ~1)
    char m_pad44[0x5c - 0x44];
    i32 m_5c; // +0x5c  screen X
    i32 m_60; // +0x60  screen Y
    char m_pad64[0x7c - 0x64];
    struct CProjShadowVtbl* m_7c; // +0x7c  shadow sub-table (Init @+0x10, host @+0x18)
    char m_pad80[0x1a0 - 0x80];
    CProjAnim m_1a0; // +0x1a0  animation sub-object (SetAnim(g_6bf3bc))
    char m_pad1a4[0x1b4 - 0x1a4];
    i32 m_1b4; // +0x1b4  geometry word (copied into CProjectile::m_15c)
    char m_pad1b8[0x1c0 - 0x1b8];
    i32 m_1c0; // +0x1c0
    char m_pad1c4[0x1c8 - 0x1c4];
    i32 m_1c8; // +0x1c8

    void CacheFirstFrame(const char* name);             // 0x150540 (__thiscall, ret 4)
    i32 ApplyLookupGeometry(const char* key, i32 flag); // 0x1505b0 (__thiscall, ret 8)
};

// The shadow companion's post-create sub-table (m_1fc->m_7c): an Init fn-ptr at
// +0x10 (fired with the shadow) and an "activation host" at +0x18 whose Activate
// (0x9d520) installs the shadow's two frame names.
struct CProjShadowActivate {
    void Activate(const char* shadowName, const char* baseName, i32 a, i32 b); // 0x9d520
};
struct CProjShadowVtbl {
    char m_pad00[0x10];
    void (*Init)(CProjRenderObj* self); // +0x10
    char m_pad14[0x18 - 0x14];
    CProjShadowActivate* m_18; // +0x18
};

// The CSample-like sound sample object the projectile launches (+0x200). Its
// StopAndRewind (0x135380) is reached as an out-of-line engine method.
struct CProjSample {
    i32 StopAndRewind();                        // 0x135380 (__thiscall, 0 args)
    i32 Play(i32 channel, i32 a, i32 b, i32 c); // 0x136300 (__thiscall, 4 args)
};

// ---------------------------------------------------------------------------
// CProjectile : CMovingLogic - 18 virtuals (vftable 0x5e798c). Adds the
// tracked-hit CObList at +0x204 plus the projectile's render/motion state
// (+0x140..+0x258). Field names are placeholders; the OFFSETS + code bytes are
// the load-bearing facts.
// ---------------------------------------------------------------------------
class CProjectile : public CMovingLogic {
public:
    CProjectile();                   // 0x126e0 (no-arg)
    CProjectile(CGameObject* owner); // 0xdec60 (1-arg spawn ctor)
    virtual ~CProjectile() OVERRIDE; // most-derived dtor (0xdef60)
    virtual i32 ProjectileVfunc();   // one added slot anchors the new vftable

    static void RegisterRange();   // 0xdf920 (seed the activation-table fast range)
    static void RegisterType();    // 0xdfb00 (level-load class registrar)
    void ReleaseDeferred(i32 arg); // 0x13c70 (fire/release the two queued callbacks; arg ignored)
    i32 DetachRenderObj();         // 0xe05e0  (clear +0x154's flag, detach, gate hide)
    void StepMotion();             // 0xe08b0  (advance the parabolic motion + render pos)
    void ScanTargets(i32 impact);  // 0xe0b10  (15x15 grid hit-scan against nearby grunts)
    i32 LaunchSound(const char* key); // 0xe2190 (create + play the launch CSample)
    // Level-load: resolve the projectile's per-type sprite frames + trajectory
    // (0xdf050, /GX) and the impact/particle effects (0xdfd00). Args are the two
    // grid endpoints + z + the target/owner ids.
    i32 LoadProjectileSprites(i32 kind, i32 a, i32 b, i32 sx, i32 sy, i32 t0, i32 t1); // 0xdf050
    void LoadProjectileEffects();                                                      // 0xdfd00

    char m_pad140[0x148 - 0x140];
    i32 m_148;                    // +0x148  (1-arg ctor zeroes)
    i32 m_14c;                    // +0x14c  (1-arg ctor zeroes)
    i32 m_150;                    // +0x150
    CProjRenderObj* m_154;        // +0x154  primary sprite/render object
    i32 m_158;                    // +0x158
    i32 m_15c;                    // +0x15c  copied from m_154->m_1b4 in LoadProjectileSprites
    char m_pad160[0x170 - 0x160]; //
    i32 m_170, m_174, m_178;      // +0x170..+0x17b  (grid cell + target id)
    i32 m_17c, m_180;             // +0x17c/+0x180   (last screen position)
    i32 m_184;                    // +0x184
    double m_188;                 // +0x188  velocity magnitude (fabs)
    i32 m_190;                    // +0x190  ProjectileTimePerTile (GetDwordDef)
    i32 m_194;                    // +0x194
    double m_198;                 // +0x198  per-frame scale
    double m_1a0;                 // +0x1a0  render X (double)
    double m_1a8;                 // +0x1a8  render Y (double)
    double m_1b0;                 // +0x1b0  velocity X basis
    double m_1b8;                 // +0x1b8  velocity Y basis
    i32 m_1c0, m_1c4;             // +0x1c0/+0x1c4  X-sign double {lo,hi} (0.0/+-0.5)
    i32 m_1c8, m_1cc;             // +0x1c8/+0x1cc  Y-sign double {lo,hi}
    i32 m_1d0, m_1d4;             // +0x1d0/+0x1d4  muzzle screen X/Y (owner)
    i32 m_1d8;                    // +0x1d8  arc/loop flag (per-type)
    i32 m_1dc;                    // +0x1dc  "effects loaded" latch
    void* m_1e0;                  // +0x1e0  sprite frame 1 ("<base>1")
    void* m_1e4;                  // +0x1e4  sprite frame 2
    void* m_1e8;                  // +0x1e8  sprite frame 3
    i32 m_1ec, m_1f0;             // +0x1ec/+0x1f0  frames 4/5 (spawn cell key elsewhere)
    void* m_1f4;                  // +0x1f4  IMPACT sprite
    void* m_1f8;                  // +0x1f8  FALL sprite
    CProjRenderObj* m_1fc;        // +0x1fc  shadow render companion
    CProjSample* m_200;           // +0x200  launch sound sample
    CObList m_204;                // +0x204  tracked-hit list (block size 10)
    i32 m_220, m_224;             // +0x220/+0x224  read by ScanTargets
    char m_pad228[0x230 - 0x228]; //
    double m_230, m_238;          // +0x230/+0x238  velocity basis
    double m_240, m_248, m_250;   // +0x240/+0x248/+0x250  position accumulators
    i32 m_258;                    // +0x258  "launched" flag
};

// Inline CMovingLogic init - folds into every leaf (and the out-of-line ctor).
// Zeroes the +0x38..+0x10c motion ints and seeds the twelve coordinate-bound
// doubles to the default [MIN,MAX] box.
inline CMovingLogic::CMovingLogic() {
    // The zero-init writes the +0x38..+0x10c motion band. MSVC schedules the
    // stores in an interleaved "column" order (all .a fields of the 8-byte pairs,
    // then all .b); listed here in that retail order.
    m_78 = 0;
    m_80 = 0;
    m_88 = 0;
    m_60 = 0;
    m_68 = 0;
    m_70 = 0;
    m_48 = 0;
    m_50 = 0;
    m_58 = 0;
    m_38 = 0; // inherited CUserLogic field (re-zeroed in the no-arg path)
    m_40 = 0;
    m_f8 = 0;
    m_100 = 0;
    m_108 = 0;
    m_7c = 0;
    m_84 = 0;
    m_8c = 0;
    m_64 = 0;
    m_6c = 0;
    m_74 = 0;
    m_4c = 0;
    m_54 = 0;
    m_5c = 0;
    m_3c = 0; // inherited CUserLogic field
    m_44 = 0;
    m_fc = 0;
    m_104 = 0;
    m_10c = 0;
    m_f0 = 0;
    m_a8 = g_movingLogicMin;
    m_c0 = g_movingLogicMax;
    m_b0 = g_movingLogicMin;
    m_c8 = g_movingLogicMax;
    m_b8 = g_movingLogicMin;
    m_d0 = g_movingLogicMax;
    m_110 = g_movingLogicMax;
    m_118 = g_movingLogicMax;
    m_120 = g_movingLogicMax;
    m_128 = g_movingLogicMax;
    m_130 = g_movingLogicMax;
    m_138 = g_movingLogicMax;
}

// The 1-arg CMovingLogic init (folded into CProjectile(owner)): chains the base
// CUserLogic(owner) (out-of-line 0x58cd0 when this ctor is too big to inline),
// runs the +0x38 motion-band init out-of-line (0x136d0), seeds the four
// coordinate bounds from the per-type config (m_14, default MIN/MAX when 0),
// then the 11-double coordinate setter (0x58bc0) and the default-Z band.
inline CMovingLogic::CMovingLogic(CGameObject* owner) : CUserLogic(owner) {
    ((CProjMotionBand*)((char*)this + 0x38))->Init();
    // Each bound: 0 => the shared MIN/MAX double copied dword-wise; else the int
    // widened via fild. Written as if/else (not ?:) so the constant branch stays
    // a mov/mov dword copy instead of being unified into an x87 fld/fstp.
    i32 lo0 = ((CProjBoundCfg*)m_14)->m_2c;
    if (lo0 == 0) {
        m_a8 = g_movingLogicMin;
    } else {
        m_a8 = (double)lo0;
    }
    i32 lo1 = ((CProjBoundCfg*)m_14)->m_34;
    if (lo1 == 0) {
        m_b0 = g_movingLogicMin;
    } else {
        m_b0 = (double)lo1;
    }
    i32 hi0 = ((CProjBoundCfg*)m_14)->m_30;
    if (hi0 == 0) {
        m_c0 = g_movingLogicMax;
    } else {
        m_c0 = (double)hi0;
    }
    i32 hi1 = ((CProjBoundCfg*)m_14)->m_38;
    if (hi1 == 0) {
        m_c8 = g_movingLogicMax;
    } else {
        m_c8 = (double)hi1;
    }
    ((CProjMotionBand*)((char*)this + 0x38))
        ->SetCoords(
            (double)m_10->m_5c,
            (double)m_10->m_60,
            0.0,
            (double)m_10->m_164,
            (double)m_10->m_168,
            0.0,
            0.0,
            0.0,
            0.0,
            (double)g_645588 * g_5eaa88,
            0.0
        );
    m_110 = m_118 = m_120 = (double)g_5f04e8;
}

#endif // GRUNTZ_PROJECTILE_H
