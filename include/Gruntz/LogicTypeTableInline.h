// LogicTypeTableInline.h - the INLINE (Find-based) body of
// CUserLogic::BuildLogicTypeTable, for the tile-logic leaf ctors whose TU inlined
// the built-in logic-type registration (the "unrolled" prologue: a 3-way
// Find/RegisterType block) rather than calling the out-of-line 0x8a40
// (Lookup-based) helper the other leaves chain.
//
// Each block re-reads ctx->m_0c->m_14 (the logic-type registry) for BOTH the Find
// and the RegisterType call - the registry's own Find (0x1703 thunk, __thiscall,
// returns the found type or 0) and its virtual registrar (vtable slot +0x24).
// Including this header BEFORE a leaf ctor makes MSVC inline the block into the
// folded CUserLogic(obj) prologue. Field names are placeholders; only offsets +
// code bytes are load-bearing.
#ifndef GRUNTZ_LOGICTYPETABLEINLINE_H
#define GRUNTZ_LOGICTYPETABLEINLINE_H

#include <rva.h>
#include <Gruntz/UserLogic.h>

// The three built-in logic-type factory callbacks (real engine .text routines we
// do not match here); declared no-body so pushing their address emits the DIR32
// relocation that reloc-masks against retail's LAB_0056e4c0/d0/e0.
extern "C" {
    i32 LogicHitFactory(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)    // 0x56e4c0
    i32 LogicAttackFactory(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant) // 0x56e4d0
    i32 LogicBumpFactory(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)   // 0x56e4e0
}

// REGISTRY IDENTITY RECOVERED (Fable A2, 2026-07-14): the logic-type registry at
// world(+0x14) IS the canonical CDDrawWorkerCache (<DDrawMgr/DDrawWorkerCache.h>,
// ??_7 @0x1efd00, 10 slots) - the CDDrawSurfaceMgr/CDDrawSurfaceMgr +0x14
// string-keyed worker cache. The dispatched "+0x24 registrar" is its slot 9
// CreateWorker (0x1652c0); the "Find" probe (thunk 0x1703 -> 0x9cab0) is its
// out-param wrapper over the +0x10 CMapStringToOb (Lookup @0x1b8008). The former
// 16-slot CLogicTypeReg view (9 Slot fillers + 6 VtSlotFill pads) and the
// bound CGameObject (its +0xc world ctx, the typed CDDrawSurfaceMgr).
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerCache.h>

inline void CUserLogic::BuildLogicTypeTable(CGameObject* obj) {
    // Each block re-reads world->m_workerCache for BOTH the Find and the
    // CreateWorker call (retail reloads the chain at each site - do not hoist).
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

// --- vtable catalog ---

#endif // GRUNTZ_LOGICTYPETABLEINLINE_H
