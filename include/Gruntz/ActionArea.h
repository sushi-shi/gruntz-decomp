#ifndef GRUNTZ_CACTIONAREA_H
#define GRUNTZ_CACTIONAREA_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/UserLogic.h>

#include <Gruntz/SerialArchive.h> // CSerialArchive (== CFileMemBase) - CPulseHighlight::Serialize

class CActionArea : public CUserLogic, public CWapX {
public:
public:
    CActionArea(CGameObject* obj); // 0x7da0
    // vtable_hierarchy proves ??_7CActionArea slot 4 IS FireActivation. (The static
    // level-load registrar @0x8240 stays CProjActObj::RegisterType - the
    // ObjTypeRegistrars.h shell the factory calls; see ActionArea.cpp.)
    virtual void FireActivation(i32 id) OVERRIDE; // 0x80e0
    // ApplyColor (0x8580): re-name the bound object's sprite for the owning team
    // (owner 1 -> "GAME_ACTIONAREA_BLUE", owner 2 -> "GAME_ACTIONAREA_RED"), reset
    // its image set's pixel-format types (SetAllTypes 8), clear the object's
    // active bit. owner outside {1,2} is rejected (returns 0).
    i32 ApplyColor(i32 owner);
    // vtable slot 2 (per-class logic-type id); regular method - the fat CUserLogic
    // base models this slot with a placeholder signature (see CGuardPoint.cpp).
    // 0x00007f80 vtable slot 2: per-class logic-type id, inline (one
    // deduped COMDAT copy in retail; see docs on header-inline members).
    RVA(0x00007f80, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_ACTIONAREA;
    }
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; RVA_COMPGEN pin in the home TU).
    i32 m_54; // +0x54
    i32 m_58; // +0x58
    i32 m_5c; // +0x5c
    i32 m_60; // +0x60
    i32 m_64; // +0x64
};
SIZE_UNKNOWN();

typedef void (CUserLogic::*ProjActHandler)();
struct CActionAreaActEntry {
    ProjActHandler m_fn; // [entry] - the registered handler
};
SIZE_UNKNOWN(); // only the first dword (the handler) is modeled

// CPulseHighlight : CUserLogic - the pulse-highlight sprite leaf (ex pulsehighlight
// unit, merged into ActionArea.cpp): a per-frame brightness ramp (Tick 0x8440) +
// serialize round-trip (0x8600). Was defined .cpp-local in ActionArea.cpp.
// The CWapX second base is BEHAVIOURALLY proven here (no RTTI COL carries this
// name): retail Serialize @0x8600 runs the shared CWapX::Chain on this+0x34, and
// this class's own fields start at +0x54 = 0x34 (CUserLogic) + 0x20 (CWapX) exactly.
// @identity-TODO: no `.?AVCPulseHighlight@@` TypeDescriptor exists - the name is a
// placeholder for a real RTTI-65 leaf whose vtable holds the 0x3512/0x1235 thunks
// (Tick 0x8440 / Serialize 0x8600); chase via vtable_scan when that family is audited.
class CPulseHighlight : public CUserLogic, public CWapX {
public:
    i32 Tick();                                               // 0x8440
    i32 Serialize(CSerialArchive* ar, i32 tag, i32 c, i32 d); // 0x8600

    // Leaf pulse-timer fields past the CUserLogic base. Serialize transfers them
    // as a raw byte stream (the +0x54..+0x67 block, kept as documented offset
    // access); Tick reads them as scalars, so they are named here.
    i32 m_phase;               // +0x54 pulse phase flag (toggles every m_duration ms)
    i64 m_timestamp;           // +0x58 last-toggle game clock
    i64 m_duration;            // +0x60 current interval (ms)
};
SIZE_UNKNOWN();

#endif // GRUNTZ_CACTIONAREA_H
