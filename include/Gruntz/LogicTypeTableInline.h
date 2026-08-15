#ifndef GRUNTZ_LOGICTYPETABLEINLINE_H
#define GRUNTZ_LOGICTYPETABLEINLINE_H

#include <rva.h>

#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerCache.h>
#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/UserLogic.h>

// Opt-in inline visibility for CUserLogic::BuildLogicTypeTable (out of line at
// 0x8a40 in UserLogic.cpp; same text) - a surviving per-TU visibility split.
// A single visible body is refuted by retail's own layout: ~45 ctors call
// 0x8a40 through the guard, including CActionArea's in link-seq-0 actionarea
// (a decliner there would home the COMDAT before userlogic, seq 2), and the
// call/expand distribution is INVERSE to caller size (big ctors call, the
// small point-logic ctors expand), which no budget arithmetic produces.
// Inside the expanded copies Find stays a call by ITS device's budget; the
// standalone 0x8a40/0x58cd0 compiles expand it.  Ledger: docs/patterns/comdat-home-adjudicates-inline-spelling.md.
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
