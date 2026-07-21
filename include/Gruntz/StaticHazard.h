#ifndef GRUNTZ_CSTATICHAZARD_H
#define GRUNTZ_CSTATICHAZARD_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/UserLogic.h>   // CUserLogic base (CStaticHazard : CUserLogic)

class CStaticHazard : public CUserLogic, public CWapX {
public:
public:
    // vtable slot 2 (per-class logic-type id); regular method - the fat CUserLogic
    // base models this slot with a placeholder signature (see CGuardPoint.cpp).
    // 0x00012ae0 vtable slot 2: per-class logic-type id, inline (one
    // deduped COMDAT copy in retail; see docs on header-inline members).
    RVA(0x00012ae0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_STATICHAZARD;
    }
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    CStaticHazard(CGameObject* obj);                                   // 0x0fb7a0 (1-arg ctor)
    static void RegisterActs();                   // 0x0fbd50 (binds "A"/"B" handlers)
    i32 LoadAttributes2();                        // 0x0fc0b0 (time-gated pulse)
    i32 LoadAttributes();                         // 0x0fc1a0 (periodic tick/update)
    virtual void FireActivation(i32 id) OVERRIDE; // 0x0fbbf0 (vtable slot 4)
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; @rva-symbol pin in the home TU).

    // CStaticHazard's own data begins at +0x40 (CUserLogic ends at +0x40).
    u32 m_pulseEpoch;   // +0x54  pulse epoch (latched g_frameTime at construction)
    i32 m_activeWindow; // +0x58  active-window length (Hazardz/AniPad bute int + offset)
    i32 m_idleWindow;   // +0x5c  idle-window length
    i32 m_fired;        // +0x60  fired flag (0/1)
    i32 m_tileCol;      // +0x64  tile column (bound object screen-X >> 5)
    i32 m_tileRow;      // +0x68  tile row    (bound object screen-Y >> 5)
};
VTBL(CStaticHazard, 0x001e7824);

// ONE entry type: the registered handlers (LoadAttributes/LoadAttributes2) return
// i32; the dispatcher ignores the result. (The ex-CHaznEntry2 void-return twin was
// the same 4-byte slot.)
typedef i32 (CUserLogic::*HaznHandler)();
struct CHaznEntry {
    HaznHandler m_fn;
};

#endif // GRUNTZ_CSTATICHAZARD_H
