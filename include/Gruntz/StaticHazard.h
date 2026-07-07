// StaticHazard.h - a static hazard game-object (C:\Proj\Gruntz).
//
// CStaticHazard : CUserLogic - a tile-logic leaf in the same game-object
// hierarchy as CTimeBomb (proven by its dtor @0x012b30 stamping the CUserLogic
// vftable 0x5e705c then the CUserBase vftable 0x5e70b4, tearing down the +0x18
// link via the embedded ~EngStr at 0x16d2a0 - byte-identical in shape to
// ~CTimeBomb @0x012a70). The leaf adds its own activation/pulse bookkeeping
// (+0x40..+0x68) on top of CUserLogic (0x40).
//
// The 1-arg ctor (0x0fb7a0) is the standard CUserLogic(CGameObject*) leaf init
// plus a static-hazard tail (re-arm the IDLE animation, snap the bound object to
// tile center, seed the pulse window from the "Hazardz/AniPad" bute int).
// LoadAttributes (0x0fc1a0) / LoadAttributes2 (0x0fc0b0) are the periodic
// tick/pulse, the same time-gated animation idiom as the CGrunt::Resolve*Animation
// cluster. FireActivation (0x0fbbf0) is the per-coordinate activation-registry
// dispatcher (the CTimeBomb::FireActivation archetype).
//
// Field names are placeholders; only OFFSETS + the inheritance chain are
// load-bearing.
#ifndef GRUNTZ_CSTATICHAZARD_H
#define GRUNTZ_CSTATICHAZARD_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/UserLogic.h>   // CUserLogic base (CStaticHazard : CUserLogic)

class CStaticHazard : public CUserLogic {
public:
    TILE_LOGIC_TAIL
public:
    // vtable slot 2 (per-class logic-type id); regular method - the fat CUserLogic
    // base models this slot with a placeholder signature (see CGuardPoint.cpp).
    virtual LogicTypeId GetTypeTag() OVERRIDE;                         // 0x012ae0
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    virtual i32 UserLogicVfunc2() OVERRIDE;                            // slot 4
    CStaticHazard(CGameObject* obj);                                   // 0x0fb7a0 (1-arg ctor)
    static void RegisterActs();        // 0x0fbd50 (binds "A"/"B" handlers)
    i32 LoadAttributes2();             // 0x0fc0b0 (time-gated pulse)
    i32 LoadAttributes();              // 0x0fc1a0 (periodic tick/update)
    void FireActivation(i32 coord);    // 0x0fbbf0 (vtable slot 4)
    virtual ~CStaticHazard() OVERRIDE; // 0x012b30 (folds the CUserLogic teardown)

    // CStaticHazard's own data begins at +0x40 (CUserLogic ends at +0x40).
    i32 m_prevAnimNode; // +0x40  snapshot of the bound object's active-anim descriptor
    char m_pad44[0x54 - 0x44];
    u32 m_pulseEpoch;   // +0x54  pulse epoch (latched g_645588 at construction)
    i32 m_activeWindow; // +0x58  active-window length (Hazardz/AniPad bute int + offset)
    i32 m_idleWindow;   // +0x5c  idle-window length
    i32 m_fired;        // +0x60  fired flag (0/1)
    i32 m_tileCol;      // +0x64  tile column (bound object screen-X >> 5)
    i32 m_tileRow;      // +0x68  tile row    (bound object screen-Y >> 5)
};
VTBL(CStaticHazard, 0x001e7824);

#endif // GRUNTZ_CSTATICHAZARD_H
