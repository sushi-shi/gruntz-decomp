// CWayPoint.cpp - the waypoint marker (C:\Proj\Gruntz), a CUserLogic leaf. The /GX
// leaf dtor + the 1-arg ctor (the "unrolled" CUserLogic(obj) prologue: this TU
// inlines the built-in logic-type registration, see LogicTypeTableInline.h).
#include <Gruntz/CWayPoint.h>
#include <Gruntz/LogicTypeTableInline.h>

// The global bute store (g_buteTree @0x6bf620; Find 0x16d190 __thiscall ret 4);
// pinned in src/Gruntz/UserLogic.cpp, re-declared so the "A" node lookup masks.
DATA(0x002bf620)
extern CButeTree g_buteTree;

// The +0x34 serializable sub-object chained by the Serialize override (0x8c00,
// __thiscall ret 0x10; NO-body so the call reloc-masks). It overlays CUserLogic's
// fat-view tail field m_34 (see the size NOTE in <Gruntz/UserLogic.h>), so the
// override reaches it as a struct view over that named base field (&m_34).
struct CSerialSub34 {
    i32 Chain(i32 a, i32 b, i32 c, i32 d); // 0x8c00
};

// CWayPoint::GetTypeTag (0x10220) - vtable slot 2: the class's logic-type id
// (0x420), the 6-byte `mov eax,<id>; ret` accessor archetype. Modeled as a regular
// method (the fat CUserLogic base slots 1/2 carry placeholder signatures the leaf
// overrides cannot match without editing that shared base; the leaf vtable is not a
// diffed symbol, so a plain method reproduces the slot bytes exactly).
RVA(0x00010220, 0x6)
i32 CWayPoint::GetTypeTag() {
    return 0x420;
}

// CWayPoint::Serialize (0x10240) - vtable slot 1: chain the shared CUserLogic
// serialize helper on `this`, then (on success) the +0x34 sub-object's chain,
// normalized to a strict bool. Byte-identical to CSecretTeleporterTrigger::Serialize.
RVA(0x00010240, 0x47)
i32 CWayPoint::Serialize(i32 a, i32 b, i32 c, i32 d) {
    if (!SerializeChain(a, b, c, d)) {
        return 0;
    }
    return ((CSerialSub34*)&m_34)->Chain(a, b, c, d) != 0;
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
    m_38->m_stateFlags |= 1;
}
