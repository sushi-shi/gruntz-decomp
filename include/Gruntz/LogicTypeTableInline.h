#ifndef GRUNTZ_LOGICTYPETABLEINLINE_H
#define GRUNTZ_LOGICTYPETABLEINLINE_H

#include <rva.h>

#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerCache.h>
#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/UserLogic.h>

// Opt-in inline visibility for CUserLogic::BuildLogicTypeTable (out of line at
// 0x8a40 in UserLogic.cpp, same text on both sides).  This is a WORKAROUND for
// caller-side modelling error, not a proven era structure - no dev writes a
// per-TU visibility header.  Retested 2026-08-22 by collapsing to ONE
// out-of-class inline body in UserLogic.h carrying the RVA pin, with the
// out-of-line definition and all six includes removed:
//   * the "a decliner in actionarea would home the COMDAT before userlogic"
//     objection is not a refutation, it is what HAPPENS and it is CORRECT -
//     actionarea homed 0x8a40, and when Find is visible there too it homes it
//     at 100.00 EXACT.  The keeper-theorem justification for this device is
//     therefore FALSE, exactly as it was for Find;
//   * what the split really buys is keeping the ~11 point-logic TUs that retail
//     CALLS 0x8a40 from from SEEING the body.  With it visible they expand it:
//     CBehindCandy 99.83 -> 66.44, CFrontCandy 99.83 -> 41.18, CEyeCandy
//     97.85 -> 42.92, CSimpleAnimation 98.72 -> 40.07, CTileTriggerTransition
//     100.00 -> 44.76, CParticlez 95.63 -> 50.14, CTeleporter 96.03 -> 58.60,
//     CToyPeek 89.26 -> 53.61, CSpotLight 96.46 -> 70.99 and more (-676 total).
// COUPLING with <DDrawMgr/DDrawWorkerCacheFindInline.h>: collapsing both at
// once takes actionarea's homed 0x8a40 from 65.58 to 100.00 EXACT and homes
// Find in droppedobject at 100.00 EXACT, but costs ??0CLightFx 94.87 -> 0.00
// and drives the same point-logic ctors lower still (-815 total).  The two
// devices hide the same body from the same TUs at two depths.
// REMOVAL CONDITION: model the point-logic ctors accurately enough that cl 5.0
// declines the table on its own budget in the TUs retail calls from; then one
// visible body in UserLogic.h reproduces the split and the device collapses.
// Ledger: docs/patterns/comdat-home-adjudicates-inline-spelling.md.
inline void CUserLogic::BuildLogicTypeTable(CGameObject* obj) {
    if (!obj->OwnerMgr()->m_workerCache->Find("LogicHit")) {
        obj->OwnerMgr()->m_workerCache->CreateWorker(LogicHitFactory, "LogicHit", 2);
    }
    if (!obj->OwnerMgr()->m_workerCache->Find("LogicAttack")) {
        obj->OwnerMgr()->m_workerCache->CreateWorker(LogicAttackFactory, "LogicAttack", 2);
    }
    if (!obj->OwnerMgr()->m_workerCache->Find("LogicBump")) {
        obj->OwnerMgr()->m_workerCache->CreateWorker(LogicBumpFactory, "LogicBump", 2);
    }
}

#endif // GRUNTZ_LOGICTYPETABLEINLINE_H
