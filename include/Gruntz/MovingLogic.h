#ifndef GRUNTZ_CMOVINGLOGIC_H
#define GRUNTZ_CMOVINGLOGIC_H

#include <Mfc.h>
#include <Gruntz/UserLogic.h> // CUserLogic (0x30) / CTileLogic (0x40) base, CGameObject, AnimWorkerObj
#include <Gruntz/MotionState.h>   // CMotionState (+0x38 band) + Init/SetParams/SetZ
#include <Gruntz/SerialArchive.h> // CSerialArchive (Serialize arg)
#include <rva.h>

extern const double g_movingLogicMin; // 0x5f04b0 (-2147483647.0)
extern const double g_movingLogicMax; // 0x5f04b8 (2147483646.0)

extern "C" u32 g_frameTime;         // 0x645588
extern const double g_motionZScale; // 0x5eaa88
extern u32 g_defaultZ;              // 0x5f04e8

class CWwdGameObjectA; // the A-kind sprite (the grunt arm's m_animPlayer)
class CAniElement;     // resolved anim-geometry handle (the grunt arm's *GeoSrc)

class CMovingLogic : public CUserLogic {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    virtual LogicTypeId GetTypeTag() OVERRIDE;                         // slot 2
    // slot 5 (0x13c70; out-of-line body in Projectile.cpp - the deferred-callback
    // release + the MovingSlot16 tail; was bound as `CProjectile::ReleaseDeferred`,
    // but the slot lives in ??_7CMovingLogic @0x1e87ac - CProjectile INHERITS it).
    virtual void FinalizeStep(i32 unused) OVERRIDE;
    // NO back-pointer fields at +0x34..+0x3c (MI1, 2026-07-17). They used to be spelled
    // here (the blanket ex-TILE_LOGIC_TAIL injection, from when this class was thought to
    // derive the fake CTileLogic) and they were a FABRICATION: retail's CProjectile ctor
    // @0xdec60 - which folds this class's 1-arg init - never stores to +0x34/+0x38/+0x3c
    // (its only +0x38 touch is `lea edi,[esi+0x38]`, the CMotionState band pointer), and
    // the no-arg 0x13940's +0x38/+0x3c writes are that band's zero-init. The three
    // back-pointers this class appeared to own are the CWapX base's, and they live at
    // +0x150 on the DERIVED classes (CGrunt/CProjectile), not here: CMovingLogic's own
    // CHD @VA 0x5f3cf0 is attributes=0 and carries no CWapX at all. Deleting them also
    // resolves the m_34/m_38/m_3c name collision the CWapX base would otherwise create
    // in CProjectile (see union-member-name-collision-rebind).
    // +0x38.. is the CMotionState band, reached via Motion(). Its first two dwords are
    // ALSO spelled flat below (m_band38/m_band3c) because the no-arg ctor zero-inits the
    // band field-by-field in one interleaved run with m_40..m_104 - the established
    // overlay idiom. They are NOT back-pointers, and they are deliberately NOT named
    // m_38/m_3c: those names now belong to the CWapX base at +0x150 on the derived
    // classes, and a same-name/different-offset pair is exactly the silent-rebind trap.
    char m_pad34[0x38 - 0x34]; // +0x34  own; no ctor writes it (role unrecovered)
public:
    CMovingLogic(); // 0x13940 (standalone) / inlined into leaves
    // The THIN seeded ctor (inline below): CUserLogic(owner) + this vptr stamp,
    // nothing else. The band init (Motion()->Init() / bounds / SetParams / Z seed)
    // is each derived ctor's OWN copy-pasted body code - the two retail folds
    // (??0CProjectile @0xdec60, ??0CGrunt @0x47a10) drift on the step global
    // (g_motionZScale vs g_gruntSpawnScale) and the Z spelling (inline triple vs
    // SetZ call), which body-level copies explain and one shared ctor cannot.
    CMovingLogic(CGameObject* owner);
    virtual ~CMovingLogic() OVERRIDE; // slot 0 (leaf dtor 0x13bd0)
    // Update lands at its true slot 16 (offset 0x40) directly: CUserLogic now models
    // all 16 of its real slots (0..15), so Update is CMovingLogic's first added
    // virtual with no filler needed.
    virtual void MovingSlot16(); // slot 16 (offset 0x40) 0x16ea90 - the ONE new virtual
                                 // (CGrunt step-coord stub / CProjectile trajectory advance)

    // Slot 1 override (bute-text serialize). Kept a plain method (its (arc,mode,..)
    // signature does not match CUserBase's slot-1 prototype, so it is not spelled as
    // a C++ override); the body @0x16f4a0 is what matters, the vtable slot is stamped.

    // The +0x38 kinematic band as a CMotionState (the real dev subobject), reached
    // via a cast so the flat-int ctor zero-init layout below overlays the same bytes.
    CMotionState* Motion() {
        return reinterpret_cast<CMotionState*>(&m_band38);
    }

    // +0x38..+0x14f: the kinematic band, overlaid per LOGIC KIND (the same retail
    // union idiom as the CWwdGameObject +0x114 slot). Arm 1 is the flat motion
    // layout the ctors zero/seed (and Motion() views as a CMotionState); arm 2 is
    // the grunt logic's reuse of the band (CGrunt parks its resolved anim/type
    // state in bytes its kinematics never touch after spawn).
    union {
        struct {                    // motion layout (ctor zero-run order; overlays Motion())
            i32 m_band38, m_band3c; // +0x38/+0x3c  band dwords 0/1 (NOT m_38/m_3c -
                                    //   those names are the CWapX flats @+0x150)
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
        struct { // grunt-logic reuse (CGrunt's recovered identities for the band)
            CWwdGameObjectA* m_animPlayer; // +0x38  the bound A-kind sprite driven as
                                           //   the animation player (tile-leaf m_38==obj)
            char m_gpad3c[0x40 - 0x3c];
            CAniElement* m_activeAnimDesc; // +0x40  cached m_animPlayer->m_1a0.m_14
            char m_gpad44[0x54 - 0x44];
            // +0x54 grunt-type name: a raw CString body ptr (a single char*) so
            // ~CGrunt does NOT auto-destruct it; viewed via CGrunt::TypeName().
            char* m_typeName;
            // +0x58..+0x7c: the resolved per-anim geometry sources (CAniElement*
            // handles from the anim-key catalog).
            CAniElement* m_idleGeoSrc[(0x68 - 0x58) / 4];      // +0x58
            CAniElement* m_battlecryGeoSrc[(0x74 - 0x68) / 4]; // +0x68
            CAniElement* m_joyGeoSrc;                          // +0x74
            CAniElement* m_deathGeoSrc;                        // +0x78
            CAniElement* m_movingGeoSrc;                       // +0x7c
            char m_gpad80[0x88 - 0x80];
            i32 m_moveSeed;      // +0x88  (moving: = g_movingSeed)
            i32 m_moveTimeHi;    // +0x8c  (moving: = 0)
            i32 m_moveStartTime; // +0x90  (moving: randomized time)
            i32 m_moveSeedHi;    // +0x94  (moving: = 0)
            char m_gpad98[0xa8 - 0x98];
            i32 m_animResolved; // +0xa8  resolve gate / dirty flag (== moveMinX lo)
            i32 m_deathCueArg;  // +0xac  cue arg (== moveMinX hi)
        };
    };
};
SIZE_UNKNOWN();
VTBL(CMovingLogic, 0x1e87ac);

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
    m_band38 = 0; // the CMotionState band's dword 0 (flat overlay zero-init)
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
    m_band3c = 0; // the band's dword 1
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

inline CMovingLogic::CMovingLogic(CGameObject* owner) : CUserLogic(owner) {
    // Thin: the CUserLogic seed + this vptr stamp only. (No +0x34..0x3c back-pointer
    // stores - the ex-TILE_LOGIC_SEED fabrication; the real ones are the CWapX flats
    // @+0x150, assigned by each derived ctor's body.)
}

#endif // GRUNTZ_CMOVINGLOGIC_H
