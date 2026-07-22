#include <Mfc.h> // real MFC CMapStringToOb/CString (the type-table + Chain registry maps)
#include <Gruntz/LogicTypeId.h> // LogicTypeId (CUserBase/CUserLogic GetTypeTag)
#include <Gruntz/UserLogic.h>   // CWapX / CUserBase / CUserLogic - the classes this TU defines
#include <Gruntz/GameObjectFactory.h> // the Logic*Factory registrants (BuildLogicTypeTable)
#include <Gruntz/Grunt.h>       // CAnimLookupNode (the m_14 aux FinalizeStep's guard reads at +0x1c)
#include <Gruntz/SerialArchive.h>     // the archive (Read/Write slots)
#include <Gruntz/AniElement.h>        // full CAniElement (m_value upcasts to CObject at KeyOfValue)
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerCache.h> // the worker cache (m_10 Lookup / CreateWorker)
#include <DDrawMgr/DDrawSubMgrLeaf.h>  // the name registry (m_10 Lookup / KeyOfValue)
#include <Io/FileMem.h> // the serialize stream (CFileMemBase == the real CFileMemBase)
#include <rva.h>

#include <string.h> // strlen / strcpy (inlined to repne scasb / rep movs)

// The retail UserLogic/WapX base-classes TU [0x87b0..0x8d52]: the CUserBase +
// CUserLogic default virtuals, the free type-table builder, and CWapX::Chain -
// ONE contiguous obj (ex FOUR fragment units: the 14 bodies sat here as
// RVA_COMPGEN "dummy anchors" while logictypetable/serialobjref carved out the
// middle and the stubs' 0x4d..0x5d RVAs stretched the unit across the CGrunt
// bands). The ??1/??_G dtor pins for this band stay in ActionArea.cpp - its obj
// is the one that EMITS those COMDATs (labels.py authority).
//
// The two out-of-line base-ctor COMDATs (CUserLogic() @0x138d0 / CUserLogic(obj)
// @0x58cd0) are emitted + RVA_COMPGEN pinned in a SEPARATE unit,
// src/Gruntz/UserLogicCtorEmit.cpp. They must NOT be forced here: the 1-arg copy
// needs an inline (Lookup-based) BuildLogicTypeTable body to match retail's
// inlined registration, and that body, if visible in THIS TU, folds into every
// leaf 1-arg ctor at depth 2 and regresses them all (retail leaves CALL 0x8a40 at
// depth 2). Isolating the forcer + inline body in its own TU keeps the leaves here
// calling the out-of-line helper.

RVA(0x000087d0, 0x8)
i32 CUserBase::SerializeMove(CFileMemBase*, i32, i32, i32) {
    return 1;
}

RVA(0x000087f0, 0x3)
LogicTypeId CUserBase::GetTypeTag() {
    return static_cast<LogicTypeId>(0);
}

RVA(0x000088d0, 0x1)
void CUserLogic::Activate() {}

RVA(0x000088f0, 0x6)
i32 CUserLogic::UserLogicVfunc5() {
    return 1;
}

RVA(0x00008910, 0x6)
i32 CUserLogic::UserLogicVfunc6() {
    return 1;
}

RVA(0x00008930, 0x6)
i32 CUserLogic::StepAttackFire() {
    return 1;
}

RVA(0x00008950, 0x1)
void CUserLogic::UserLogicVfunc8() {}

RVA(0x00008970, 0x1)
void CUserLogic::UserLogicVfunc9() {}

RVA(0x00008990, 0x1)
void CUserLogic::UserLogicVfuncA() {}

RVA(0x000089b0, 0x1)
void CUserLogic::UserLogicVfuncB() {}

RVA(0x000089d0, 0x1)
void CUserLogic::UserLogicVfuncC() {}

RVA(0x000089f0, 0x1)
void CUserLogic::UserLogicVfuncD() {}

RVA(0x00008a40, 0xc8)
void __stdcall BuildLogicTypeTable(CGameObject* obj) {
    {
        CObject* found = 0;
        obj->OwnerMgr()->m_workerCache->m_10.Lookup("LogicHit", found);
        if (!found) {
            obj->OwnerMgr()->m_workerCache->CreateWorker(LogicHitFactory, "LogicHit", 2);
        }
    }
    {
        CObject* found = 0;
        obj->OwnerMgr()->m_workerCache->m_10.Lookup("LogicAttack", found);
        if (!found) {
            obj->OwnerMgr()->m_workerCache->CreateWorker(LogicAttackFactory, "LogicAttack", 2);
        }
    }
    {
        CObject* found = 0;
        obj->OwnerMgr()->m_workerCache->m_10.Lookup("LogicBump", found);
        if (!found) {
            obj->OwnerMgr()->m_workerCache->CreateWorker(LogicBumpFactory, "LogicBump", 2);
        }
    }
}

RVA(0x00008b50, 0x3)
void CUserLogic::XferName(char* name) {
    // ret-4 one-arg no-op; leaves override to receive their serialized type name.
}

RVA(0x00008b70, 0x3)
void CUserLogic::FireActivation(i32) {}

typedef void (CUserLogic::*UserLogicCallback)(); // 4 bytes (complete class, single inheritance)

RVA(0x00008b90, 0x40)
void CUserLogic::FinalizeStep(i32 /*unused*/) {
    if (m_deferredCallback == 0) {
        return;
    }
    if (m_gatedCallback != 0 && reinterpret_cast<i32>(m_objAux->m_1c) == m_28) {
        (this->*reinterpret_cast<UserLogicCallback&>(m_gatedCallback))();
        m_gatedCallback = 0;
    }
    (this->*reinterpret_cast<UserLogicCallback&>(m_deferredCallback))();
    m_deferredCallback = 0;
    m_28 = 0x3e9;
}

// ---------------------------------------------------------------------------
// 0x8c00: serialize one referenced object + its key name.
// @early-stop
// 88.0% - logic byte-faithful: the read/write virtual dispatch (Read @ +0x2c /
// Write @ +0x30), the strlen-key gate, the CMapStringToOb::Lookup + KeyOfValue
// registry chain (both real symbols paired), the inline strlen/strcpy/memset, and
// the frameless /GX-elided CString temp all match. The residual is one stack-slot-
// coloring difference (docs/patterns/stack-slot-coalesce-frame-4b.md): retail gives
// the read-path Lookup `out` and the write-path CString temp DISTINCT slots
// (sub esp,0x88: out@0x10, CString@0x14, buf@0x18) while cl COALESCES the two
// mutually-exclusive locals into one slot (sub esp,0x84) - shifting every [esp+M]
// by 4 and spilling the KeyOfValue RVO result instead of using eax. Not steerable
// from source under /O2 (function-scope `val` and inner-scope reshapes both
// regressed). Logic complete; deferred to the final sweep.
RVA(0x00008c00, 0x152)
i32 CWapX::Chain(CFileMemBase* arc, i32 mode, i32 unused, CGameObject* obj) {
    char name[0x80];

    if (arc == 0) {
        return 0;
    }
    if (mode == 7) {
        // READ: pull the key name + the 0x10 blob, then resolve the key.
        arc->Read(name, 0x80);
        arc->Read(m_blob, 0x10);
        m_34 = obj;
        m_38 = static_cast<CWwdGameObjectA*>(obj); // the bound obj IS the A-kind sprite
        m_3c = obj->m_7c;
        if (strlen(name) == 0) {
            m_value = 0;
            return 1;
        }
        void* val = 0; // CMapStringToPtr::Lookup (0x1b8438) takes a void&
        m_3c->m_0c->m_animRegistry->m_10.Lookup(name, val);
        m_value = static_cast<CAniElement*>(val); // the map stores void*; KeyOfValue takes the CObject* upcast
        return 1;
    }
    if (mode == 4) {
        // WRITE: re-derive the value's name into the key buffer, then write both.
        for (i32 i = 0; i < 0x20; i++) {
            (reinterpret_cast<i32*>(name))[i] = 0;
        }
        if (m_value != 0) {
            CString nm = m_3c->m_0c->m_animRegistry->KeyOfValue(m_value);
            strcpy(name, static_cast<const char*>(nm));
        }
        arc->Write(name, 0x80);
        arc->Write(m_blob, 0x10);
    }
    return 1;
}
