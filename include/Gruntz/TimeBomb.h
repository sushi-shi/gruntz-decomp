#ifndef GRUNTZ_CTIMEBOMB_H
#define GRUNTZ_CTIMEBOMB_H

#include <rva.h>
#include <Gruntz/UserLogic.h> // CUserLogic base (CTimeBomb : CUserLogic)

class CTimeBomb : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    RVA(0x00012a20, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_TIMEBOMB;
    } // slot 2
public:
    CTimeBomb(CGameObject* obj);                  // 0x0e1b90 (1-arg leaf ctor)
    virtual void FireActivation(i32 id) OVERRIDE; // 0x0e1830
    static void RegisterActs();    // 0x0e1990 (binds the logic handler to key "A"; static:
                                   //  no this, called this-less by the game-object factory)
    i32 LoadAttributes();          // 0x0e1e60 (per-frame timer/detonate step)
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; @rva-symbol pin in the home TU).
    i32 m_fastPhase; // +0x54  0 = slow phase (re-arms to fast on expiry), 1 = fast phase (detonates)
    i64 m_startTime; // +0x58  phase-start running-clock snapshot (lo dword of the i64 base)
    i64 m_duration;  // +0x60  phase duration (lo dword of the i64)
};
SIZE(0x68);

typedef void (CUserLogic::*TBombHandler)();
struct CTBombEntry {
    TBombHandler m_fn; // [entry]
};
SIZE_UNKNOWN();

#endif // GRUNTZ_CTIMEBOMB_H
