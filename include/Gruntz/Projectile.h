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

// ---------------------------------------------------------------------------
// CMovingLogic : CUserLogic - moving-object motion state. 17 virtuals
// (vftable 0x5e87ac); only the offsets the ctor initializes are modeled.
//
// The +0x38..+0x10c band is a set of int pairs zeroed at construction; the
// +0xa8..+0x13f band is twelve doubles seeded to the default min/max bounds.
// ---------------------------------------------------------------------------
class CMovingLogic : public CUserLogic {
public:
    CMovingLogic();
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
// FUN_0055c360 (0x15c360, __thiscall, 1 arg) re-targets the active animation.
struct CProjAnim {
    i32 SetAnim(u32 mode); // 0x15c360
};

// A render object the projectile owns/points at (the +0x154 sprite/animation and
// the +0x1fc shadow companion). Only the offsets the reconstructed methods touch
// are modeled: +0x08 flag word, +0x40 flag word, +0x5c/+0x60 screen position,
// +0x1a0 animation sub-object, +0x1c0/+0x1c8 state gates.
struct CProjRenderObj {
    char m_pad00[0x08];
    u32 m_08; // +0x08  flag word (|= 0x10000)
    char m_pad0c[0x40 - 0x0c];
    u32 m_40; // +0x40  flag word (&= ~1)
    char m_pad44[0x5c - 0x44];
    i32 m_5c; // +0x5c  screen X
    i32 m_60; // +0x60  screen Y
    char m_pad64[0x1a0 - 0x64];
    CProjAnim m_1a0; // +0x1a0  animation sub-object (SetAnim(g_6bf3bc))
    char m_pad1a4[0x1c0 - 0x1a4];
    i32 m_1c0; // +0x1c0
    char m_pad1c4[0x1c8 - 0x1c4];
    i32 m_1c8; // +0x1c8
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
    virtual ~CProjectile() OVERRIDE; // most-derived dtor (0xdef60)
    virtual i32 ProjectileVfunc();   // one added slot anchors the new vftable

    void ReleaseDeferred(i32 arg); // 0x13c70 (fire/release the two queued callbacks; arg ignored)
    i32 DetachRenderObj();         // 0xe05e0  (clear +0x154's flag, detach, gate hide)
    void StepMotion();             // 0xe08b0  (advance the parabolic motion + render pos)
    void ScanTargets(i32 impact);  // 0xe0b10  (15x15 grid hit-scan against nearby grunts)
    i32 LaunchSound(const char* key); // 0xe2190 (create + play the launch CSample)

    char m_pad140[0x150 - 0x140];
    i32 m_150;                    // +0x150
    CProjRenderObj* m_154;        // +0x154  primary render object
    i32 m_158;                    // +0x158
    char m_pad15c[0x170 - 0x15c]; //
    i32 m_170, m_174, m_178;      // +0x170..+0x17b  (grid cell + target id)
    i32 m_17c, m_180;             // +0x17c/+0x180   (last screen position)
    char m_pad184[0x198 - 0x184]; //
    double m_198;                 // +0x198  per-frame scale
    double m_1a0;                 // +0x1a0  render X (double)
    double m_1a8;                 // +0x1a8  render Y (double)
    char m_pad1b0[0x1ec - 0x1b0]; //
    i32 m_1ec, m_1f0;             // +0x1ec/+0x1f0  spawn cell key
    char m_pad1f4[0x1fc - 0x1f4]; //
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

#endif // GRUNTZ_PROJECTILE_H
