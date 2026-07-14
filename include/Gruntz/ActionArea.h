// ActionArea.h - the action-area trigger tile-logic game-object (C:\Proj\Gruntz),
// a CUserLogic leaf (RTTI .?AVCActionArea@@). The 1-arg ctor (0x7da0) folds the
// shared CUserLogic(obj) prologue then a per-class tail; the leaf state begins at
// +0x54. Offsets + code bytes are load-bearing; field names are placeholders.
#ifndef GRUNTZ_CACTIONAREA_H
#define GRUNTZ_CACTIONAREA_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/UserLogic.h>

#include <Gruntz/SerialArchive.h> // CSerialArchive (== CFileMemBase) - CPulseHighlight::Serialize
                                  // arg; the typedef keeps the mangling PAVCFileMemBase (a bare
                                  // `class CSerialArchive;` would shadow it and break the match)

SIZE_UNKNOWN(CActionArea);
class CActionArea : public CUserLogic {
public:
    TILE_LOGIC_TAIL
public:
    CActionArea(CGameObject* obj); // 0x7da0
    // FireActivation (0x80e0) is the slot-4 (UserLogicVfunc2) per-coordinate
    // activation dispatch body; the fat CUserLogic base models slot 4 with the
    // no-arg UserLogicVfunc2() placeholder below, so the int-arg real shape is a
    // plain method (the leaf vtable slot stays base-attributed - same pattern as
    // CProjectile::RunAct). RegisterType (0x8240) is the static level-load class
    // registrar. (Both were the .cpp-local CProjActObj view - vtable_hierarchy
    // proves ??_7CActionArea slot 4 IS FireActivation.)
    void FireActivation(i32 coord); // 0x80e0
    static void RegisterType();     // 0x8240 (static: no this, called by the factory)
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
    virtual i32 UserLogicVfunc2() OVERRIDE;                            // slot 4
    virtual ~CActionArea() OVERRIDE; // 0x7fd0 (folds the CUserLogic teardown)

    char m_pad40[0x54 - 0x40];
    i32 m_54; // +0x54
    i32 m_58; // +0x58
    i32 m_5c; // +0x5c
    i32 m_60; // +0x60
    i32 m_64; // +0x64
};
VTBL(CActionArea, 0x001e7004);

// The action-area projectile-action entry: its first dword is the registered
// handler, dispatched __thiscall on `this` (4-byte single-inheritance PMF ->
// `mov ecx,this; call [entry]`; CActionArea is complete above so the PMF stays 4
// bytes). Was the .cpp-local R3Entry view.
typedef void (CActionArea::*ProjActHandler)();
struct CActionAreaActEntry {
    ProjActHandler m_fn; // [entry] - the registered handler
};
SIZE_UNKNOWN(CActionAreaActEntry); // only the first dword (the handler) is modeled

// CPulseHighlight : CUserLogic - the pulse-highlight sprite leaf (ex pulsehighlight
// unit, merged into ActionArea.cpp): a per-frame brightness ramp (Tick 0x8440) +
// serialize round-trip (0x8600). Was defined .cpp-local in ActionArea.cpp.
class CPulseHighlight : public CUserLogic {
public:
    TILE_LOGIC_TAIL
public:
    i32 Tick();                                               // 0x8440
    i32 Serialize(CSerialArchive* ar, i32 tag, i32 c, i32 d); // 0x8600

    // Leaf pulse-timer fields past the CUserLogic base. Serialize transfers them
    // as a raw byte stream (the +0x54..+0x67 block, kept as documented offset
    // access); Tick reads them as scalars, so they are named here.
    char m_pad40[0x54 - 0x40]; // +0x40..+0x53 (the +0x34/+0x38 base-region per-TU views)
    i32 m_phase;               // +0x54 pulse phase flag (toggles every m_duration ms)
    i64 m_timestamp;           // +0x58 last-toggle game clock
    i64 m_duration;            // +0x60 current interval (ms)
};

#endif // GRUNTZ_CACTIONAREA_H
