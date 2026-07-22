#ifndef GRUNTZ_CACTIONAREA_H
#define GRUNTZ_CACTIONAREA_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/UserLogic.h>

#include <Gruntz/SerialArchive.h> // CFileMemBase (== CFileMemBase) - SerializeMove

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
    // slot 1 @0x8600 (ex "CPulseHighlight::Serialize" - the binary holds exactly
    // ONE reference to that body: ??_7CActionArea+0x4, this slot; the
    // "pulse-highlight leaf" WAS this class): CUserLogic chain + CWapX Chain +
    // the raw +0x54..+0x67 pulse-block byte stream.
    virtual i32 SerializeMove(CFileMemBase*, i32, i32, i32) OVERRIDE;
    // 0x8440 (ex "CPulseHighlight::Tick"): per-frame brightness ramp over the
    // bound sprite's image set. UNREFERENCED in retail (no /OPT:REF keeps it);
    // its ILT thunk 0x3517 has no referent either.
    i32 Tick();
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; RVA_COMPGEN pin in the home TU).
    i32 m_phase;     // +0x54  pulse phase flag (toggles every m_duration ms)
    i64 m_timestamp; // +0x58  last-toggle game clock
    i64 m_duration;  // +0x60  current interval (ms)
};
SIZE_UNKNOWN();

typedef void (CUserLogic::*ProjActHandler)();
struct CActionAreaActEntry {
    ProjActHandler m_fn; // [entry] - the registered handler
};
SIZE_UNKNOWN(); // only the first dword (the handler) is modeled

// TU-local thunk/table names this TU registers (moved from the .cpp; the
// addresses are ILT thunk VAs, reloc-masked at every use).
extern "C" void ProjActHandlerThunk(); // 0x403517 (ILT thunk)

// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---
#include <Gruntz/HaznColl.h> // CCoordColl (for the extern below)
extern CCoordColl g_projReg;

#endif // GRUNTZ_CACTIONAREA_H
