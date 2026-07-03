// CBrickz.cpp - the CBrickz game-object (C:\Proj\Gruntz). Continues the
// CUserBase/CUserLogic hierarchy (see <Gruntz/CBrickz.h>).
//
// CBrickz::CBrickz(CGameObject*) (0x10e800) is the 1-arg leaf ctor: the standard
// CUserLogic(obj) init (folded inline) plus the Brickz tail (cache the anim-set
// node off the "A" bute key, raise the bound object's logic/collision flag bits,
// seed its tile-coordinate fields). Like the rest of the family it constructs a
// throwing CUserBaseLink, so MSVC emits the /GX EH frame -> built eh.
//
// The big LoadAttributes (0x0810f0) is NOT here: it is the level-load attribute
// parser (a 2228-byte multi-switch bute reader) - parked in src/Stub/CBrickz.cpp
// for the final sweep (a leaf-first redo).
#include <Gruntz/CBrickz.h>
#include <Bute/ButeMgr.h> // CButeTree (g_buteTree)

// The global bute store (g_buteTree @0x6bf620; Find 0x16d190).
extern CButeTree g_buteTree;

// Vtable anchor: declaring ~CBrickz gives the leaf its own most-derived vftable
// (0x5e7c54) so the ctor's vptr store falls out. Empty body is enough.
CBrickz::~CBrickz() {}

// ---------------------------------------------------------------------------
// CBrickz::CBrickz @0x10e800 - the 1-arg leaf ctor.
// @early-stop
// EH-state-numbering wall (docs/patterns/eh-state-numbering-base.md): the body is
// byte-identical (the CUserLogic init, the "A" anim-set cache, the two separate
// m_38->m_08 RMW + the m_38->m_40 bit, the m_164/m_168/m_04 tile-coord seed); the
// residue is this ctor's own __ehfuncinfo + a 1-slot pop-edi scheduling delta in
// the tail. Not source-steerable; ~92%. Parked for the final sweep.
RVA(0x0010e800, 0x17d)
CBrickz::CBrickz(CGameObject* obj) : CUserLogic(obj) {
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_38->m_08 |= 2;
    m_38->m_08 |= 1;
    m_38->m_40 |= 1;
    m_object->m_164 = m_object->m_5c >> 5;
    m_object->m_168 = m_object->m_60 >> 5;
    m_object->m_04 = (m_object->m_164 << 8) + m_object->m_168;
}

// The +0x34 serializable sub-object chained by the Serialize override (0x8c00,
// __thiscall ret 0x10; NO-body so the call reloc-masks). It overlays CUserLogic's
// fat-view tail field m_34 (see the size NOTE in <Gruntz/UserLogic.h>), so the
// override reaches it as a struct view over that named base field (&m_34).
struct CSerialSub34 {
    i32 Chain(i32 a, i32 b, i32 c, i32 d); // 0x8c00
};

// CBrickz::GetTypeTag @0x011300 - vtable slot 2: the class's logic-type id (0x409),
// the 6-byte `mov eax,<id>; ret` accessor archetype. Regular method (the fat
// CUserLogic base slots 1/2 carry placeholder signatures the leaf overrides cannot
// match without editing that shared base; the leaf vtable is not a diffed symbol).
RVA(0x00011300, 0x6)
i32 CBrickz::GetTypeTag() {
    return 0x409;
}

// CBrickz::Serialize @0x011320 - vtable slot 1: chain the shared CUserLogic
// serialize helper on `this`, then (on success) the +0x34 sub-object's chain,
// normalized to a strict bool. Byte-identical to CSecretTeleporterTrigger::Serialize.
RVA(0x00011320, 0x47)
i32 CBrickz::Serialize(i32 a, i32 b, i32 c, i32 d) {
    if (!SerializeChain(a, b, c, d)) {
        return 0;
    }
    return ((CSerialSub34*)&m_34)->Chain(a, b, c, d) != 0;
}
