#ifndef GRUNTZ_CMOVINGLOGIC_H
#define GRUNTZ_CMOVINGLOGIC_H

#include <Mfc.h>
#include <Gruntz/UserLogic.h> // CUserLogic (0x30) / CTileLogic (0x40) base, CGameObject, AnimWorkerObj
#include <Gruntz/MotionState.h>   // CMotionState (+0x38 member) + SetParams/SetZ
#include <Gruntz/SerialArchive.h> // CFileMemBase (Serialize arg)
#include <rva.h>

extern const double g_movingLogicMin; // 0x5f04b0 (-2147483647.0)
extern const double g_movingLogicMax; // 0x5f04b8 (2147483646.0)

extern "C" u32 g_frameTime;         // 0x645588
extern const double g_motionZScale; // 0x5eaa88
extern u32 g_defaultZ;              // 0x5f04e8

class CMovingLogic : public CUserLogic {
public:
    virtual i32 SerializeMove(CFileMemBase*, i32, i32, i32) OVERRIDE; // slot 1
    virtual LogicTypeId GetTypeTag() OVERRIDE;                        // slot 2
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
    // +0x38 is a real CMotionState member. Both seeded leaf constructors call its
    // constructor between the CUserLogic base construction and the CMovingLogic
    // vptr stamp; the standalone CMovingLogic constructor inlines that same member
    // constructor. The former flat/union view was caused by five CWarlord methods
    // being mis-owned by CGrunt: their +0x38..+0xac fields are CWarlord's CWapX and
    // own members, not an alternate interpretation of CMovingLogic's motion bytes.
    char m_pad34[0x38 - 0x34]; // +0x34  own; no ctor writes it (role unrecovered)
public:
    CMovingLogic(); // 0x13940 (standalone) / inlined into leaves
    // The seeded ctor constructs the CUserLogic base and the m_motion member.
    // CProjectile/CGrunt then apply their distinct bounds, step scale, and Z seed.
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

    CMotionState* Motion() {
        return &m_motion;
    }

    CMotionState m_motion;          // +0x38..+0x13f
    i32 m_140, m_144, m_148, m_14c; // +0x140..+0x14f
};
SIZE_UNKNOWN();

#ifndef CMOVINGLOGIC_STANDALONE_CTOR
inline CMovingLogic::CMovingLogic() {}
#endif // CMOVINGLOGIC_STANDALONE_CTOR

inline CMovingLogic::CMovingLogic(CGameObject* owner) : CUserLogic(owner) {
    // The member initializer phase constructs m_motion at +0x38.
}

extern const double g_motionTimeScale;
#endif // GRUNTZ_CMOVINGLOGIC_H
