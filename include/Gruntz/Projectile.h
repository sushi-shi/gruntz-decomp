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
    virtual int MovingLogicVfunc();   // one added slot anchors the new vftable

    // CMovingLogic's own data begins at +0x40 (CUserLogic ends at +0x40). The
    // ctor also re-zeroes the inherited CUserLogic m_38/m_3c.
    // +0x40..+0x8f: twenty motion ints.
    int m_40, m_44, m_48, m_4c, m_50, m_54, m_58, m_5c;
    int m_60, m_64, m_68, m_6c, m_70, m_74, m_78, m_7c, m_80, m_84, m_88, m_8c;
    char m_pad90[0xa8 - 0x90];
    // +0xa8..+0xd7: three "ranges", each {double lo, double hi}.
    double m_a8, m_b0, m_b8; // the three lo bounds (default MIN)
    double m_c0, m_c8, m_d0; // the three hi bounds (default MAX)
    char m_padd8[0xf0 - 0xd8];
    int m_f0;                     // +0xf0
    char m_padf4[0xf8 - 0xf4];    // gap
    int m_f8, m_fc, m_100, m_104; // +0xf8..+0x107
    int m_108, m_10c;             // +0x108..+0x10f
    // +0x110..+0x13f: six doubles, all seeded to MAX.
    double m_110, m_118, m_120, m_128, m_130, m_138;
};

// ---------------------------------------------------------------------------
// CProjectile : CMovingLogic - 18 virtuals (vftable 0x5e798c). Adds the
// tracked-hit CObList at +0x204.
// ---------------------------------------------------------------------------
class CProjectile : public CMovingLogic {
public:
    CProjectile();                   // 0x126e0 (no-arg)
    virtual ~CProjectile() OVERRIDE; // most-derived dtor
    virtual int ProjectileVfunc();   // one added slot anchors the new vftable

    char m_pad140[0x204 - 0x140];
    CObList m_204; // +0x204 (MFC block size 10)
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
