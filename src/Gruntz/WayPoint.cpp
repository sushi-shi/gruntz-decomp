#include <Gruntz/WayPoint.h>
#include <Gruntz/SerialArchive.h> // CFileMemBase (the inherited CWapX::Chain arg; ex SerialObjRef.h)
#include <Gruntz/LogicTypeTableInline.h>
#include <Gruntz/SerialArchive.h> // the serialize stream (== the real CFileMemBase)
#include <rva.h>

RVA(0x00010240, 0x47)
i32 CWayPoint::SerializeMove(CFileMemBase* a, i32 b, i32 c, CGameObject* d) {
    if (!CUserLogic::SerializeMove(a, b, c, d)) {
        return 0;
    }
    return Chain(a, b, c, d) != 0;
}

// CWayPoint::~CWayPoint (0x102e0) - the /GX leaf dtor folds the bare CUserLogic
// teardown: store the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link
// (the embedded ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The
// leaf vptr store is dead-eliminated.
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CWayPoint() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
RVA_COMPGEN(0x000102b0, 0x1e, ??_GCWayPoint@@UAEPAXI@Z)
RVA_COMPGEN(0x000102e0, 0x44, ??1CWayPoint@@UAE@XZ)
VTBL(CWayPoint, 0x001e74b4);

// CWayPoint::CWayPoint (0xae3f0) - fold the shared CUserLogic(obj) init (with the
// built-in logic types inlined-registered), then flag the sub-object (+0x40 bit 1).
// @early-stop
// eh-ctor-vptr-restamp-position wall, all-inline-base variant (99.68%; see the
// "~99.6% variant is a LOAD HOIST" section of docs/patterns/eh-ctor-vptr-restamp-position.md).
// All 399 bytes match except ONE adjacent transposition at 0xae557: retail stamps the leaf
// vptr and THEN loads m_38 (`mov [esi],<vtbl>` / `mov eax,[esi+0x38]`); cl hoists the load one
// slot over the stamp. The stamp is NOT sunk (a pure-store body emits it first) - a load moves.
// Unreachable from source: local temp, r/m/w pair, inline-member call, mem-init order swap and
// CWapX store-order permutations are all byte-identical, and `permute variants` finds 0 AST
// mutations (single-statement body) with 48 TU-state trials moving nothing. Suppressed only by
// leaf DATA MEMBERS between the last base store and the stamp (why CPathHazard @0xb35a0 matches),
// and SIZE 0x54 == CUserLogic 0x34 + CWapX 0x20 leaves no room for one.
RVA(0x000ae3f0, 0x18f)
CWayPoint::CWayPoint(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_38->m_stateFlags |= 1;
}
