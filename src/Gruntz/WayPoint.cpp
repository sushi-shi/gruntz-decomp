// WayPoint.cpp - the waypoint marker (C:\Proj\Gruntz), a CUserLogic leaf. The /GX
// leaf dtor + the 1-arg ctor (the "unrolled" CUserLogic(obj) prologue: this TU
// inlines the built-in logic-type registration, see LogicTypeTableInline.h).
#include <Gruntz/WayPoint.h>
#include <Gruntz/MovingLogicBase.h> // CMovingLogicBase::Serialize (0x16e7f0) - shared serialize chain
#include <Gruntz/SerialObjRef.h>    // the shared serialized-object-reference (Chain @0x8c00)
#include <Gruntz/LogicTypeTableInline.h>
#include <Gruntz/SerialArchive.h> // the serialize stream (== the real CFileMemBase)

// CWayPoint::GetTypeTag (0x00010220) is now an inline member in the class header.

// CWayPoint::Serialize (0x10240) - vtable slot 1: chain the shared CUserLogic
// serialize helper on `this`, then (on success) the +0x34 sub-object's chain,
// normalized to a strict bool. Byte-identical to CSecretTeleporterTrigger::Serialize.
RVA(0x00010240, 0x47)
i32 CWayPoint::Serialize(i32 a, i32 b, i32 c, i32 d) {
    if (!((CMovingLogicBase*)this)->Serialize((CSerialArchive*)(a), b, c, d)) {
        return 0;
    }
    return ((CSerialObjRef*)&m_34)->Chain((CSerialArchive*)a, b, c, (CGameObject*)d) != 0;
}

// CWayPoint::~CWayPoint (0x102e0) - the /GX leaf dtor folds the bare CUserLogic
// teardown: store the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link
// (the embedded ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The
// leaf vptr store is dead-eliminated.
RVA(0x000102e0, 0x44)
CWayPoint::~CWayPoint() {}

// CWayPoint::CWayPoint (0xae3f0) - fold the shared CUserLogic(obj) init (with the
// built-in logic types inlined-registered), then flag the sub-object (+0x40 bit 1).
// @early-stop
// eh-ctor-vptr-restamp-position wall (docs/patterns/eh-ctor-vptr-restamp-position.md):
// body byte-identical (incl. the unrolled built-in logic-type registration); residual
// is the /GX leaf-vptr re-stamp position + EH-state ids.
RVA(0x000ae3f0, 0x18f)
CWayPoint::CWayPoint(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    m_38->m_stateFlags |= 1;
}
