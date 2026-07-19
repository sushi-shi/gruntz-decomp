// GuardPoint.cpp - the guard-point marker (C:\Proj\Gruntz), a CUserLogic leaf.
// The /GX leaf dtor + the 1-arg ctor (the "unrolled" CUserLogic(obj) prologue).
#include <Gruntz/GuardPoint.h>
#include <Gruntz/SerialArchive.h> // CSerialArchive (the inherited CWapX::Chain arg; ex SerialObjRef.h)
#include <Gruntz/LogicTypeTableInline.h>
#include <Gruntz/SerialArchive.h> // the serialize stream (== the real CFileMemBase)

// CGuardPoint::GetTypeTag (0x00010350) is now an inline member in the class header.

// CGuardPoint::Serialize (0x10370) - vtable slot 1: chain the shared CUserLogic
// serialize helper on `this`, then (only on success) the +0x34 sub-object's chain,
// normalized to a strict bool. Byte-identical to CSecretTeleporterTrigger::Serialize.
RVA(0x00010370, 0x47)
i32 CGuardPoint::SerializeMove(CGruntArchive* a, i32 b, i32 c, i32 d) {
    if (!CUserLogic::SerializeMove(a, b, c, d)) {
        return 0;
    }
    return Chain(a, b, c, reinterpret_cast<CGameObject*>(d)) != 0;
}

// CGuardPoint::~CGuardPoint (0x10410) - the /GX leaf dtor folds the bare
// CUserLogic teardown: store the CUserLogic vptr (0x5e705c), inline-destruct the
// +0x18 link (the embedded ~EngStr call 0x16d2a0), store the CUserBase vptr
// (0x5e70b4). The leaf vptr store is dead-eliminated.
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CGuardPoint() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
// @rva-symbol: ??1CGuardPoint@@UAE@XZ 0x00010410 0x44

// CGuardPoint::CGuardPoint (0xae5f0) - fold the shared CUserLogic(obj) init (with
// the built-in logic types inlined-registered), then flag the sub-object.
// @early-stop
// eh-ctor-vptr-restamp-position wall (docs/patterns/eh-ctor-vptr-restamp-position.md):
// body byte-identical (incl. the unrolled logic-type registration); residual is the
// /GX leaf-vptr re-stamp position + EH-state ids.
RVA(0x000ae5f0, 0x18f)
CGuardPoint::CGuardPoint(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_38->m_stateFlags |= 1;
}
