// CMovingLogicCtor.cpp - the out-of-line CMovingLogic constructor (0x13940),
// re-homed from src/Stub/CMovingLogic.cpp (C:\Proj\Gruntz).
//
// CMovingLogic : CUserLogic : CUserBase (see include/Gruntz/Projectile.h). The
// engine emits TWO copies of this ctor: the inline one that folds into
// CProjectile::CProjectile (modeled inline in Projectile.h) AND this standalone
// at 0x13940. The standalone cannot be emitted from src/Gruntz/Projectile.cpp -
// forcing it there makes CProjectile call it out-of-line and drops that ctor
// 99%->19.7% (see the CAVEAT note in Projectile.cpp). So it lives in its OWN TU:
// here the most-derived class IS CMovingLogic, so the final vptr is its own
// (0x5e87ac) and the body is identical to the inline version.
//
// The CMovingLogic layout is re-declared (matching Projectile.h exactly) so this
// TU can hang the out-of-line ctor; that is matching-neutral (a separate TU). The
// throwing CUserBaseLink in the CUserLogic base forces the /GX EH frame -> eh.
#include <Mfc.h>
#include <Gruntz/UserLogic.h>
#include <rva.h>

// The default coordinate bounds (retail .rdata 0x5f04b0/0x5f04b8). Defined +
// DATA-pinned in Projectile.cpp; declared extern here so the ctor's dword loads
// reloc-mask against the named symbols.
extern const double g_movingLogicMin; // 0x5f04b0 (-2147483647.0)
extern const double g_movingLogicMax; // 0x5f04b8 (2147483646.0)

// ---------------------------------------------------------------------------
// CMovingLogic : CUserLogic - mirrors include/Gruntz/Projectile.h (17 virtuals,
// vftable 0x5e87ac). Only the offsets the ctor initializes are modeled.
// ---------------------------------------------------------------------------
class CMovingLogic : public CUserLogic {
public:
    CMovingLogic(); // 0x13940 (standalone, out-of-line)
    virtual ~CMovingLogic() OVERRIDE;
    virtual i32 MovingLogicVfunc();
    virtual i32 MovingLogicVfunc2();
    virtual i32 MovingLogicVfunc3();

    // CMovingLogic's own data begins at +0x40; the ctor also re-zeroes the
    // inherited CUserLogic m_38/m_3c.
    i32 m_40, m_44, m_48, m_4c, m_50, m_54, m_58, m_5c;
    i32 m_60, m_64, m_68, m_6c, m_70, m_74, m_78, m_7c, m_80, m_84, m_88, m_8c;
    char m_pad90[0xa8 - 0x90];
    double m_a8, m_b0, m_b8; // three lo bounds (default MIN)
    double m_c0, m_c8, m_d0; // three hi bounds (default MAX)
    char m_padd8[0xf0 - 0xd8];
    i32 m_f0;
    char m_padf4[0xf8 - 0xf4];
    i32 m_f8, m_fc, m_100, m_104;
    i32 m_108, m_10c;
    double m_110, m_118, m_120, m_128, m_130, m_138; // all seeded to MAX
};

// Out-of-line vtable anchors (give CMovingLogic a real vftable in this TU so the
// ctor's vptr store falls out). Bodies are not matched.
CMovingLogic::~CMovingLogic() {}
i32 CMovingLogic::MovingLogicVfunc() {
    return 0;
}
i32 CMovingLogic::MovingLogicVfunc2() {
    return 0;
}
i32 CMovingLogic::MovingLogicVfunc3() {
    return 0;
}

// The standalone ctor. The +0x38..+0x10c motion ints are zeroed in retail's
// scheduled "column" order (all .a/low fields, then all .b/high fields), then the
// twelve coordinate bounds are seeded to the default [MIN,MAX] box.
// @early-stop
// 98.4% (vptr-stamp scheduling artifact, 6 residual bytes = ONE instruction):
// prologue, the /GX EH frame, the byte-exact field-zero "column" schedule, the
// min/max fan-out and the epilogue all match. Retail keeps the intermediate
// CUserLogic vptr stamp in the post-link slot and stamps the most-derived
// CMovingLogic vptr LATE (after the field init); cl here instead MOVES the
// CMovingLogic stamp into that early slot (DSE'ing the CUserLogic stamp) and emits
// no late stamp - because the min/max double loads clobber ecx, so cl cannot hoist
// the SEH-restore load into the post-link slot the way the byte-exact CPathHazard
// no-arg ctor (0x13170, no double init -> ecx free) does. A non-steerable cl
// scheduling/DSE micro-decision; logic complete. Deferred to the final sweep.
RVA(0x00013940, 0x1e1)
CMovingLogic::CMovingLogic() {
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
