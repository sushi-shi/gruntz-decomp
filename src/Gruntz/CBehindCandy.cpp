// CBehindCandy.cpp - the behind-candy eyecandy tile-logic game-object (C:\Proj\Gruntz).
//
// Two CBehindCandy methods, defined in ascending retail-RVA order:
//   GetTypeTag      @0x00fb70 - the 6-byte per-class logic-type id accessor (0x3f0).
//   ~CBehindCandy   @0x00fc30 - the /GX leaf dtor (folds the CUserLogic teardown).
//
// CBehindCandy : CUserLogic (the base hierarchy comes from <Gruntz/UserLogic.h>).
// Only offsets / code bytes are load-bearing; names are placeholders for the
// recovered engine identities.
#include <Gruntz/CBehindCandy.h>
#include <Gruntz/LogicTypeId.h>

// The +0x34 serializable sub-object chained by the Serialize override (0x8c00,
// __thiscall ret 0x10; NO-body so the call reloc-masks). It overlays CUserLogic's
// fat-view tail field m_34 (see the size NOTE in <Gruntz/UserLogic.h>), so the
// override reaches it as a struct view over that named base field (&m_34).
struct CSerialSub34 {
    i32 Chain(i32 a, i32 b, i32 c, i32 d); // 0x8c00
};

// CBehindCandy::GetTypeTag @0x00fb70 - vtable slot 2: return the class's logic-type
// id. The same 6-byte `mov eax,<id>; ret` accessor archetype as
// CTileTriggerTransition::GetTypeTag (0x011730). Regular method (the fat CUserLogic
// base slots 1/2 carry placeholder signatures the leaf overrides cannot match
// without editing that shared base; the leaf vtable is not a diffed symbol).
RVA(0x0000fb70, 0x6)
i32 CBehindCandy::GetTypeTag() {
    return LOGIC_BEHINDCANDY; // 0x3f0
}

// CBehindCandy::Serialize @0x00fb90 - vtable slot 1: chain the shared CUserLogic
// serialize helper on `this`, then (on success) the +0x34 sub-object's chain,
// normalized to a strict bool. Byte-identical to CSecretTeleporterTrigger::Serialize.
RVA(0x0000fb90, 0x47)
i32 CBehindCandy::Serialize(i32 a, i32 b, i32 c, i32 d) {
    if (!SerializeChain(a, b, c, d)) {
        return 0;
    }
    return ((CSerialSub34*)&m_34)->Chain(a, b, c, d) != 0;
}

// CBehindCandy::~CBehindCandy @0x00fc30 - the leaf adds no destructible members
// beyond CUserLogic, so its dtor folds the bare CUserLogic teardown: store the
// CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the embedded
// ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The destructible
// link forces the /GX EH frame. Byte-identical in shape to ~CTimeBomb @0x012a70;
// the empty body is enough for cl.
RVA(0x0000fc30, 0x44)
CBehindCandy::~CBehindCandy() {}

#include <rva.h>
