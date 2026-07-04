// MovingLogicCtor.cpp - the out-of-line CMovingLogic constructor (0x13940),
// re-homed from src/Stub/CMovingLogic.cpp (C:\Proj\Gruntz).
//
// The engine emits TWO copies of this ctor: the inline one that folds into
// CProjectile::CProjectile (modeled inline in <Gruntz/MovingLogic.h>) AND this
// standalone at 0x13940. The standalone cannot be emitted from Projectile.cpp -
// forcing it there makes CProjectile call it out-of-line and drops that ctor
// 99%->19.7% (see the CAVEAT note in Projectile.cpp). So it lives in its OWN TU:
// CMOVINGLOGIC_STANDALONE_CTOR drops the header's inline no-arg ctor here so this
// TU can hang the byte-exact out-of-line copy. The throwing CUserBaseLink in the
// CUserLogic base forces the /GX EH frame -> eh.
#define CMOVINGLOGIC_STANDALONE_CTOR
#include <Gruntz/MovingLogic.h>
#include <rva.h>

// ---------------------------------------------------------------------------
// Out-of-line vtable anchors - give CMovingLogic a real vftable in this TU so the
// ctor's vptr store falls out. Bodies are not matched. (slot 16 Update is defined
// in MovingLogicUpdate.cpp, referenced externally.)
// ---------------------------------------------------------------------------
CMovingLogic::~CMovingLogic() {}

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
