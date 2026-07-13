// MovingLogic.h - the ONE canonical CMovingLogic : CTileLogic (RTTI
// .?AVCMovingLogic@@, vftable 0x5e87ac). Replaces the former per-TU reduced-view
// definitions - the tile-logic world now shares this single class. C:\Proj\Gruntz.
//
// CMovingLogic is the moving-object logic base (parent of CProjectile). It owns:
//   * the CTileLogic base (true-0x30 CUserLogic + the 0x30-0x3c tail = 0x40 total; the
//     vptr + the +0x18 link + back-pointers);
//   * a 0x108-byte CMotionState kinematic band embedded at +0x38 (the real dev
//     subobject). Because the CTileLogic base ends at +0x40, the band at +0x38
//     overlaps the tail m_38/m_3c, so it is reached through the Motion() accessor
//     (a `+0x38` cast) while the flat-int ctor zero-init view overlays the same
//     bytes - the established byte-proven idiom (see Projectile.cpp);
//   * twelve default-bound doubles the ctor seeds to the [MIN,MAX] box;
//   * four trailing ints (+0x140..+0x14c) the per-frame Update / bute Serialize touch.
//
// Vtable (17 slots, per vtable_hierarchy --class CMovingLogic): all 16 inherited from
// CUserLogic (fully modeled at 16 slots) + exactly ONE new virtual, Update
// (slot 16 / offset 0x40, RVA 0x16ea90).
// Serialize (0x16f4a0, slot 1 override) and the leaf dtor (slot 0, 0x13bd0) are the
// class's own remaining methods.
//
// NOTE (dual-world): the CGrunt world (<Gruntz/Grunt.h>) models CMovingLogic against
// its OWN CUserBase/CUserLogic (CGruntHud* m_10).
//
// STALE CLAIM REMOVED (2026-07-13): this used to add "so it cannot include this canonical
// view - the two never coexist in a TU". False - Grunt.h #includes BOTH <Gruntz/UserLogic.h>
// AND this header, so they already coexist in every TU that includes Grunt.h; CUserLogic has
// exactly one definition in the tree (SIZE 0x30) and the "fat 0x40" view it warned about is
// gone. The CUserLogic 0x30 base is UNIFIED (CTileLogic is the fat tile-logic intermediate);
// only the remaining CGrunt ODR merge stays deferred (blocked on the inline-vs-out-of-
// line ctor model + the i32/void vtable-signature split - see the NOTE in UserLogic.h).
#ifndef GRUNTZ_CMOVINGLOGIC_H
#define GRUNTZ_CMOVINGLOGIC_H

#include <Mfc.h>
#include <Gruntz/UserLogic.h> // CUserLogic (0x30) / CTileLogic (0x40) base, CGameObject, CGameObjAux
#include <Gruntz/MotionState.h>   // CMotionState (+0x38 band) + Init/SetParams/SetZ
#include <Gruntz/SerialArchive.h> // CSerialArchive (Serialize arg)
#include <rva.h>

// The default coordinate bounds the ctor copies dword-wise (retail .rdata
// 0x5f04b0/0x5f04b8 - the same doubles MotionState.h names g_motionMin/g_motionMax;
// a second name for the same reloc-masked cells). DATA-pinned in Projectile.cpp.
extern const double g_movingLogicMin; // 0x5f04b0 (-2147483647.0)
extern const double g_movingLogicMax; // 0x5f04b8 (2147483646.0)

// The 1-arg ctor's velocity/scale seeds: g_frameTime (spawn seed int, scaled by the
// .rdata double g_5eaa88) and g_5f04e8 (the default-Z int). Read unsigned -> the
// fild {lo,0} idiom.
extern "C" u32 g_frameTime;   // 0x645588
extern const double g_5eaa88; // 0x5eaa88
extern u32 g_5f04e8;          // 0x5f04e8

// The per-type config the 1-arg ctor reads its coordinate bounds from
// (CUserLogic::m_14, the bound object's +0x7c aux). Only the four bound ints.
SIZE_UNKNOWN(CProjBoundCfg);
struct CProjBoundCfg {
    char _00[0x2c];
    i32 m_2c; // +0x2c  -> lo bound A (0 => default MIN)
    i32 m_30; // +0x30  -> hi bound A (0 => default MAX)
    i32 m_34; // +0x34  -> lo bound B (0 => default MIN)
    i32 m_38; // +0x38  -> hi bound B (0 => default MAX)
};

// ---------------------------------------------------------------------------
// CMovingLogic : CTileLogic - moving-object motion state.
// ---------------------------------------------------------------------------
SIZE_UNKNOWN(CMovingLogic);
class CMovingLogic : public CUserLogic {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    virtual LogicTypeId GetTypeTag() OVERRIDE;                         // slot 2
    virtual i32 UserLogicVfunc3() OVERRIDE;                            // slot 5
    TILE_LOGIC_TAIL
public:
    CMovingLogic();                   // 0x13940 (standalone) / inlined into leaves
    CMovingLogic(CGameObject* owner); // 1-arg (folds into CProjectile(owner))
    virtual ~CMovingLogic() OVERRIDE; // slot 0 (leaf dtor 0x13bd0)
    // Update lands at its true slot 16 (offset 0x40) directly: CUserLogic now models
    // all 16 of its real slots (0..15), so Update is CMovingLogic's first added
    // virtual with no filler needed.
    virtual void MovingSlot16(); // slot 16 (offset 0x40) 0x16ea90 - the ONE new virtual
                                 // (CGrunt step-coord stub / CProjectile trajectory advance)

    // Slot 1 override (bute-text serialize). Kept a plain method (its (arc,mode,..)
    // signature does not match CUserBase's slot-1 prototype, so it is not spelled as
    // a C++ override); the body @0x16f4a0 is what matters, the vtable slot is stamped.
    i32 Serialize(CSerialArchive* arc, i32 mode, i32 a3, i32 a4); // 0x16f4a0

    // The +0x38 kinematic band as a CMotionState (the real dev subobject), reached
    // via a cast so the flat-int ctor zero-init layout below overlays the same bytes.
    CMotionState* Motion() {
        return (CMotionState*)((char*)this + 0x38);
    }

    // CMovingLogic's own data begins at +0x40 (CTileLogic base ends at +0x40). The
    // ctor also re-zeroes the inherited CTileLogic m_38/m_3c (the band's first double).
    // +0x40..+0x8f: twenty motion ints (zeroed dword-wise; overlay Motion()).
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
    // +0x140..+0x14c: four trailing ints (Update / Serialize round-trip).
    i32 m_140, m_144, m_148, m_14c;
};
VTBL(CMovingLogic, 0x1e87ac);

// Inline no-arg CMovingLogic init - folds into every leaf. Zeroes the +0x38..+0x10c
// motion ints and seeds the twelve coordinate-bound doubles to the default [MIN,MAX]
// box. MSVC schedules the zero stores in an interleaved "column" order (all .a fields
// of the 8-byte pairs, then all .b); listed here in that retail order.
//
// CMovingLogicCtor.cpp defines CMOVINGLOGIC_STANDALONE_CTOR to drop this inline and
// hang the byte-exact standalone out-of-line COMDAT copy (0x13940) instead - the
// engine emits both an inlined copy (folded into leaves) and this standalone.
#ifndef CMOVINGLOGIC_STANDALONE_CTOR
inline CMovingLogic::CMovingLogic() {
    m_78 = 0;
    m_80 = 0;
    m_88 = 0;
    m_60 = 0;
    m_68 = 0;
    m_70 = 0;
    m_48 = 0;
    m_50 = 0;
    m_58 = 0;
    m_38 = 0; // inherited CTileLogic field (re-zeroed in the no-arg path)
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
    m_3c = 0; // inherited CTileLogic field
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
#endif // CMOVINGLOGIC_STANDALONE_CTOR

// The 1-arg CMovingLogic init (folded into CProjectile(owner)): chains the base
// CUserLogic(owner), runs the +0x38 motion-band init out-of-line (0x136d0 via the
// Motion() cast), seeds the four coordinate bounds from the per-type config (m_14,
// default MIN/MAX when 0), then the 11-double coordinate setter (0x58bc0) and the
// default-Z band.
inline CMovingLogic::CMovingLogic(CGameObject* owner) : CUserLogic(owner) {
    TILE_LOGIC_SEED(owner);
    Motion()->Init();
    // Each bound: 0 => the shared MIN/MAX double copied dword-wise; else the int
    // widened via fild. Written as if/else (not ?:) so the constant branch stays a
    // mov/mov dword copy instead of being unified into an x87 fld/fstp.
    i32 lo0 = ((CProjBoundCfg*)m_objAux)->m_2c;
    if (lo0 == 0) {
        m_a8 = g_movingLogicMin;
    } else {
        m_a8 = (double)lo0;
    }
    i32 lo1 = ((CProjBoundCfg*)m_objAux)->m_34;
    if (lo1 == 0) {
        m_b0 = g_movingLogicMin;
    } else {
        m_b0 = (double)lo1;
    }
    i32 hi0 = ((CProjBoundCfg*)m_objAux)->m_30;
    if (hi0 == 0) {
        m_c0 = g_movingLogicMax;
    } else {
        m_c0 = (double)hi0;
    }
    i32 hi1 = ((CProjBoundCfg*)m_objAux)->m_38;
    if (hi1 == 0) {
        m_c8 = g_movingLogicMax;
    } else {
        m_c8 = (double)hi1;
    }
    Motion()->SetParams(
        (double)m_object->m_screenX,
        (double)m_object->m_screenY,
        0.0,
        (double)m_object->m_164,
        (double)m_object->m_168,
        0.0,
        0.0,
        0.0,
        0.0,
        (double)g_frameTime * g_5eaa88,
        0.0
    );
    m_110 = m_118 = m_120 = (double)g_5f04e8;
}

#endif // GRUNTZ_CMOVINGLOGIC_H
