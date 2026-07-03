// CGuardPoint.cpp - the guard-point marker (C:\Proj\Gruntz), a CUserLogic leaf.
// The /GX leaf dtor + the 1-arg ctor (the "unrolled" CUserLogic(obj) prologue).
#include <Gruntz/CGuardPoint.h>
#include <Gruntz/CSerialObjRef.h> // the shared serialized-object-reference (Chain @0x8c00)
#include <Gruntz/LogicTypeTableInline.h>

// CGuardPoint::GetTypeTag (0x10350) - vtable slot 2: return the class's logic-type
// id (0x42a). The 6-byte `mov eax,<id>; ret` accessor archetype (cf.
// CBehindCandy::GetTypeTag). Modeled as a regular method: the fat CUserLogic view
// in <Gruntz/UserLogic.h> declares this slot as UserBaseVfunc2 with a placeholder
// signature, and its two later slots (Serialize/FireActivation) take args the base
// placeholders lack, so the leaf overrides cannot be spelled OVERRIDE without
// editing that shared base - the leaf vtable is not a diffed symbol, so a plain
// method reproduces the slot function's bytes exactly.
RVA(0x00010350, 0x6)
LogicTypeId CGuardPoint::GetTypeTag() {
    return LOGIC_GUARDPOINT; // 0x42a
}

// CGuardPoint::Serialize (0x10370) - vtable slot 1: chain the shared CUserLogic
// serialize helper on `this`, then (only on success) the +0x34 sub-object's chain,
// normalized to a strict bool. Byte-identical to CSecretTeleporterTrigger::Serialize.
RVA(0x00010370, 0x47)
i32 CGuardPoint::Serialize(i32 a, i32 b, i32 c, i32 d) {
    if (!SerializeChain(a, b, c, d)) {
        return 0;
    }
    return ((CSerialObjRef*)&m_34)->Chain((CSerialArchive*)a, b, c, (CSerialObj*)d) != 0;
}

// CGuardPoint::~CGuardPoint (0x10410) - the /GX leaf dtor folds the bare
// CUserLogic teardown: store the CUserLogic vptr (0x5e705c), inline-destruct the
// +0x18 link (the embedded ~EngStr call 0x16d2a0), store the CUserBase vptr
// (0x5e70b4). The leaf vptr store is dead-eliminated.
RVA(0x00010410, 0x44)
CGuardPoint::~CGuardPoint() {}

// CGuardPoint::CGuardPoint (0xae5f0) - fold the shared CUserLogic(obj) init (with
// the built-in logic types inlined-registered), then flag the sub-object.
// @early-stop
// eh-ctor-vptr-restamp-position wall (docs/patterns/eh-ctor-vptr-restamp-position.md):
// body byte-identical (incl. the unrolled logic-type registration); residual is the
// /GX leaf-vptr re-stamp position + EH-state ids.
RVA(0x000ae5f0, 0x18f)
CGuardPoint::CGuardPoint(CGameObject* obj) : CUserLogic(obj) {
    m_38->m_stateFlags |= 1;
}
