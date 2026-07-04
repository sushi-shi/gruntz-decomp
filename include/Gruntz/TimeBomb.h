// CTimeBomb.h - the timez-bomb pickup game-object (C:\Proj\Gruntz).
//
// CTimeBomb : CUserLogic - a tile-logic leaf in the same game-object hierarchy
// as CKitchenSlime (proven by its dtor stamping the CUserLogic vftable 0x5e705c
// then the CUserBase vftable 0x5e70b4, tearing down the +0x18 link via the
// embedded ~EngStr at 0x16d2a0 - byte-identical in shape to ~CKitchenSlime
// @0x013100). The leaf adds no destructible members beyond CUserLogic, so its
// dtor folds the bare CUserLogic teardown (the /GX leaf-dtor archetype).
//
// FireActivation (0x0e1830) is the SAME per-coordinate activation-registry
// dispatcher archetype as CKitchenSlime::FireActivation (0x0b2940), but a
// DIFFERENT registry instance (CTimeBomb's, at 0x64c780 vs the slime's at
// 0x646228). This is the archetype for the CParticlez / CStaticHazard /
// CToobSpikez / CDroppedObject 0x102-sized FireActivation siblings.
//
// Field names are placeholders; only OFFSETS + the inheritance chain are
// load-bearing.
#ifndef GRUNTZ_CTIMEBOMB_H
#define GRUNTZ_CTIMEBOMB_H

#include <rva.h>
#include <Gruntz/UserLogic.h> // CUserLogic base (CTimeBomb : CUserLogic)

class CTimeBomb : public CUserLogic {
public:
    CTimeBomb(CGameObject* obj);    // 0x0e1b90 (1-arg leaf ctor)
    void FireActivation(i32 coord); // 0x0e1830
    void RegisterActs();            // 0x0e1990 (binds the logic handler to key "A")
    i32 LoadAttributes();           // 0x0e1e60 (per-frame timer/detonate step)
    virtual ~CTimeBomb() OVERRIDE;  // 0x012a70 (folds the CUserLogic teardown)

    i32 m_prevAnimNode; // +0x40  m_38->m_1b4 snapshot
    char m_pad44[0x54 - 0x44];
    i32 m_fastPhase; // +0x54  0 = slow phase (re-arms to fast on expiry), 1 = fast phase (detonates)
    i32 m_startTimeLo; // +0x58  phase-start running-clock snapshot (lo dword of the i64 base)
    i32 m_startTimeHi; // +0x5c  (hi dword)
    i32 m_durationLo;  // +0x60  phase duration (lo dword of the i64)
    i32 m_durationHi;  // +0x64  (hi dword)
};
SIZE(CTimeBomb, 0x68);

#endif // GRUNTZ_CTIMEBOMB_H
