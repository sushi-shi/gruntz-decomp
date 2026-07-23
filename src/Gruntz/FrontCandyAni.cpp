#include <Gruntz/ActNameRegistry.h> // the shared activation-name registry archetype
#include <Rez/FrameClock.h> // frame-clock band (g_frameDelta/g_frameTime/g_killCueClock/g_engineFrameDelta)
#include <Image/CImage.h> // the +0x198 cached frame (ex CGameObjLayer view)
#include <Wap32/ZVec.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/ActReg.h>       // the shared CActReg coordinate-registry archetype
#include <Gruntz/LogicFnTable.h> // CActRegPool<CEyeCandy>::s_table's canonical CActReg type
#include <Gruntz/FrontCandy.h> // 0xfa60 is CFrontCandy's slot-1 (??_7CFrontCandy+0x4), not CFrontCandyAni's
#include <Gruntz/FrontCandyAni.h>
#include <Gruntz/EyeCandy.h>
#include <Gruntz/EyeCandyAni.h> // CEyeCandyAni (its TU folds in below, wave3-J)

#include <Gruntz/AnimSink.h>
#include <Gruntz/SerialArchive.h> // CFileMemBase (the inherited CWapX::Chain arg; ex SerialObjRef.h)

template<> DATA(0x002460b0)
CActReg CActRegPool<CFrontCandyAni>::s_table(2000, 2010);

RVA(0x0000fa60, 0x47)
i32 CFrontCandy::SerializeMove(CFileMemBase* ar, i32 tag, i32 c, i32 d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    return Chain(ar, tag, c, reinterpret_cast<CGameObject*>(d)) != 0;
}

// CFrontCandy::~CFrontCandy @0x0fb00 - empty vtable-anchor dtor; folds the CUserLogic
// teardown (the /GX leaf-dtor archetype). Adjacent to CFrontCandy::Serialize (0xfa60).
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CFrontCandy() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
#include <rva.h>
RVA_COMPGEN(0x0000fb00, 0x44, ??1CFrontCandy@@UAE@XZ)

RVA(0x0000fdf0, 0x47)
i32 CFrontCandyAni::SerializeMove(CFileMemBase* ar, i32 tag, i32 c, i32 d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    return Chain(ar, tag, c, reinterpret_cast<CGameObject*>(d)) != 0;
}

// CFrontCandyAni::~CFrontCandyAni @0xfe90 - empty vtable-anchor dtor (??_7CFrontCandyAni
// slot 0 -> sdd 0xfe60); folds the CUserLogic teardown (the /GX leaf-dtor archetype).
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CFrontCandyAni() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
RVA_COMPGEN(0x0000fe90, 0x44, ??1CFrontCandyAni@@UAE@XZ)

RVA(0x0000ff20, 0x47)
i32 CEyeCandyAni::SerializeMove(CFileMemBase* ar, i32 tag, i32 c, i32 d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    return Chain(ar, tag, c, reinterpret_cast<CGameObject*>(d)) != 0;
}

// CEyeCandyAni::~CEyeCandyAni @0x0ffc0 - empty vtable-anchor dtor; folds the
// CUserLogic teardown (the /GX leaf-dtor archetype).
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CEyeCandyAni() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
RVA_COMPGEN(0x0000ffc0, 0x44, ??1CEyeCandyAni@@UAE@XZ)

RVA(0x000abfa0, 0x1b6)
CFrontCandy::CFrontCandy(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    if (m_object->m_sortKey != 0xf4240) {
        m_object->m_sortKey = 0xf4240;
        m_object->m_flags |= 0x20000;
    }
    CImage* aux = m_object->m_layer;
    if (aux != 0) {
        if (aux->m_width >= g_buteMgr.GetInt("World", "BigActHeight")
            || m_object->m_layer->m_height >= g_buteMgr.GetInt("World", "BigActHeight")) {
            if (m_object->m_7c != 0) {
                m_object->m_7c->m_08 &= ~6;
                m_object->m_7c->m_08 |= 1;
                m_38->m_flags &= ~0x1000002;
                m_38->m_flags |= 0x800000;
            }
        }
    }
}

// CEyeCandyAni::CEyeCandyAni (0xac870) - fold the shared CUserLogic(obj) init, bind
// the "A" bute node, apply the cycle geometry, then run the shared eyecandy z-clamp
// + BigActHeight de-prioritize tail (the SAME archetype as CEyeCandy/CBehindCandyAni).
// @early-stop
// eh-ctor-vptr-restamp-position wall (docs/patterns/eh-ctor-vptr-restamp-position.md):
// body byte-identical; residual is the /GX leaf-vptr re-stamp position + EH-state ids.
RVA(0x000ac870, 0x20e)
CEyeCandyAni::CEyeCandyAni(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    if (m_38->m_1a0.m_14 == 0) {
        m_value = m_38->m_1a0.m_14;
        m_38->ApplyLookupGeometry("GAME_CYCLE100", 0);
    }
    CWwdGameObjectA* o = m_object;
    if (o->m_sortKey == 0 && o->m_layer != 0) {
        i32 v = o->m_layer->m_anchorY + o->m_screenY + 0x186a0;
        if (o->m_sortKey != v) {
            o->m_sortKey = v;
            o->m_flags |= 0x20000;
        }
    }
    CImage* aux = m_object->m_layer;
    if (aux != 0) {
        if (aux->m_width >= g_buteMgr.GetInt("World", "BigActHeight")
            || m_object->m_layer->m_height >= g_buteMgr.GetInt("World", "BigActHeight")) {
            if (m_object->m_7c != 0) {
                m_object->m_7c->m_08 &= ~6;
                m_object->m_7c->m_08 |= 1;
                m_38->m_flags &= ~0x1000002;
                m_38->m_flags |= 0x800000;
            }
        }
    }
}

RVA(0x000acbb0, 0x102)
void CEyeCandyAni::FireActivation(i32 id) {
    CEyeCandyActEntry* e =
        reinterpret_cast<CEyeCandyActEntry*>(CActRegPool<CEyeCandy>::s_table.ResolveEntry(id));
    if (e->m_fn != 0) {
        (this
             ->*(reinterpret_cast<CEyeCandyActEntry*>(
                 CActRegPool<CEyeCandy>::s_table.ResolveEntry(id)
             ))
             ->m_fn)();
    }
}

// CEyeCandyAni::RegisterActs @0x0acd10 - bind the class's per-frame handler
// (AdvanceAnim @0x0acf10) to the activation key "A" via the shared name registry,
// then bind id->entry in the class's coordinate registry (CActRegPool<CEyeCandy>::s_table
// @0x646060, CActReg facet). The SAME archetype as CFrontCandyAni::RegisterActs (0x0ad310).
//
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): logic
// byte-faithful (every call/immediate/branch/offset + the `mov [entry],offset
// AdvanceAnim` handler store match retail); residual is the slot-vs-id callee-saved
// register choice cascading into the free-loop count materialization. Deferred.
RVA(0x000acd10, 0x18d)
void CEyeCandyAni::RegisterActs() {
    i32 id = reinterpret_cast<i32>(g_buteTree.Find("A"));
    if (id == 0) {
        id = g_typeCounter;
        g_buteTree.Insert("A", reinterpret_cast<void*>(id));
        char* slot = ActNameLookup(id);
        i32 n = g_typeColl.m_grown;
        void** list = reinterpret_cast<void**>(g_typeColl.m_alloc);
        while (n-- != 0) {
            if (list != 0) {
                (reinterpret_cast<CString*>(list))->CString::~CString();
            }
            list++;
        }
        (reinterpret_cast<CString*>(slot))->operator=("A");
        g_typeCounter++;
    }
    (reinterpret_cast<CEyeCandyActEntry*>(CActRegPool<CEyeCandy>::s_table.ResolveEntry(id)))->m_fn =
        static_cast<i32 (CUserLogic::*)()>(&CEyeCandyAni::AdvanceAnim);
}

RVA(0x000acf10, 0x17)
i32 CEyeCandyAni::AdvanceAnim() {
    m_38->m_1a0.Advance(g_engineFrameDelta);
    return 0;
}

RVA(0x000acf40, 0x16e)
CFrontCandyAni::CFrontCandyAni(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    if (m_38->m_1a0.m_14 == 0) {
        m_value = m_38->m_1a0.m_14;
        m_38->ApplyLookupGeometry("GAME_CYCLE100", 0);
    }
    if (m_object->m_sortKey != 0xf4240) {
        m_object->m_sortKey = 0xf4240;
        m_object->m_flags |= 0x20000;
    }
}

VTBL(CEyeCandyAni, 0x001e8334);
VTBL(CFrontCandyAni, 0x001e83e4);
VTBL(CFrontCandy, 0x001e84ec);

RVA_COMPGEN(0x000ad110, 0xa, _$E708880)
RVA_COMPGEN(0x000ad130, 0x15, _$E708912)
RVA_COMPGEN(0x000ad160, 0xe, _$E708960)
RVA_COMPGEN(0x000ad180, 0x1f, _$E708992)

RVA(0x000ad1b0, 0x102)
void CFrontCandyAni::FireActivation(i32 coord) {
    CFrontCandyActEntry* e = reinterpret_cast<CFrontCandyActEntry*>(
        CActRegPool<CFrontCandyAni>::s_table.ResolveEntry(coord)
    );
    if (e->m_fn != 0) {
        CFrontCandyActEntry* e2 = reinterpret_cast<CFrontCandyActEntry*>(
            CActRegPool<CFrontCandyAni>::s_table.ResolveEntry(coord)
        );
        (this->*(e2->m_fn))();
    }
}

// CFrontCandyAni::RegisterActs @0x0ad310 - bind the class's per-frame handler
// (AdvanceAnim @0x0ad510) to the activation key "A" via the shared name registry.
// The SAME archetype as CBehindCandyAni::RegisterActs.
//
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): logic
// byte-faithful (every call/immediate/branch/offset + the `mov [entry],offset
// AdvanceAnim` handler store match retail); residual is the slot-vs-id callee-saved
// register choice cascading into the free-loop count materialization. Deferred.
RVA(0x000ad310, 0x18d)
void CFrontCandyAni::RegisterActs() {
    i32 id = reinterpret_cast<i32>(g_buteTree.Find("A"));
    if (id == 0) {
        id = g_typeCounter;
        g_buteTree.Insert("A", reinterpret_cast<void*>(id));
        char* slot = ActNameLookup(id);
        i32 n = g_typeColl.m_grown;
        void** list = reinterpret_cast<void**>(g_typeColl.m_alloc);
        while (n-- != 0) {
            if (list != 0) {
                (reinterpret_cast<CString*>(list))->CString::~CString();
            }
            list++;
        }
        (reinterpret_cast<CString*>(slot))->operator=("A");
        g_typeCounter++;
    }
    (reinterpret_cast<CFrontCandyActEntry*>(CActRegPool<CFrontCandyAni>::s_table.ResolveEntry(id)))
        ->m_fn = static_cast<i32 (CUserLogic::*)()>(&CFrontCandyAni::AdvanceAnim);
}

RVA(0x000ad510, 0x17)
i32 CFrontCandyAni::AdvanceAnim() {
    m_38->m_1a0.Advance(g_engineFrameDelta);
    return 0;
}

#include <rva.h>
#include <Wap32/ZVec.h>
#include <Gruntz/SerialArchive.h> // the serialize stream (== the real CFileMemBase)
