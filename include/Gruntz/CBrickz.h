#ifndef GRUNTZ_CBRICKZ_H
#define GRUNTZ_CBRICKZ_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/UserLogic.h>   // CUserLogic base (CBrickz : CUserLogic)

class CBrickz : public CUserLogic, public CWapX {
public:
public:
    CBrickz(CGameObject* obj);   // 0x10e800 (1-arg ctor)
    // NO user-declared dtor: retail 0x113c0 is COMPILER-GENERATED (implicit; the
    // vtable-owner-probe proof is in <Gruntz/MapLogic.h>; RVA_COMPGEN pin in
    // TileLogicPump.cpp).
    // The class's own CUserLogic slot overrides, reconstructed as regular methods
    // (the fat base models slots 1/2 with placeholder signatures; see the .cpp).
    // 0x00011300 vtable slot 2: per-class logic-type id, inline (one
    // deduped COMDAT copy in retail; see docs on header-inline members).
    RVA(0x00011300, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_BRICKZ;
    }
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    // slot 4 (0x10ea80, body in TileLogicPump.cpp): the act-registry PMF dispatcher
    // over g_brickzActReg. RE-ATTRIBUTED from CCheckpointTrigger - the TileLogicPump
    // act clusters were shifted one class down. RTTI: CBrickz[4] override -> 0x0012b2,
    // and 0x0012b2 -> jmp 0x10ea80 (sema xref).
    virtual void FireActivation(i32 id) OVERRIDE; // 0x10ea80
    static void InitActReg();   // 0x10ea00 (constructs g_brickzActReg @0x64e7c0)
    static void RegisterActs(); // 0x10ebe0 (binds the "A" activation handler)
    i32 Trigger();              // 0x10ede0 (the activation handler; declared-only, used as a PMF)
    // Declared-only (body 0x810f0, Brickz.cpp): load the puzzle's tile attributes.
    // Added so Play.cpp can call it on the real class instead of on the fabricated `Eng`
    // conflation (the call reloc-masks either way, but only this spelling binds to the
    // symbol retail actually calls).
    i32 LoadAttributes(i32 a, i32 b); // 0x0810f0

    // No own data: the two bases already span the whole object -
    // CUserLogic (0x34) + CWapX (0x20) == 0x54 == SIZE. The ex `m_own[0x54-0x40]`
    // was the CWapX base spelled as padding (the pre-CWapX model thought the base
    // ended at +0x40 and the leaf added 0x14); the CWapX sweep missed it because it
    // was named m_own, not m_pad40. Removing it makes the body compute 0x54 exactly.
};
SIZE(0x54); // CUserLogic (0x34) + CWapX (0x20)

typedef i32 (CUserLogic::*BrickzHandler)();
struct CBrickzActEntry {
    BrickzHandler m_fn;
};
SIZE_UNKNOWN();

#endif // GRUNTZ_CBRICKZ_H
