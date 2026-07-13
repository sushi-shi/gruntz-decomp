// FrontCandyAni.cpp - a front-candy eyecandy animation game-object
// (C:\Proj\Gruntz). The mirror of CBehindCandyAni, sharing the per-class
// activation-registry archetype:
//   FireActivation @0x0ad1b0 - the per-coordinate activation-registry dispatcher.
//   RegisterActs   @0x0ad310 - bind the per-frame handler to the "A" key.
//   AdvanceAnim    @0x0ad510 - the per-frame animation-advance (ret 0).
//
// CFrontCandyAni : CUserLogic (the base hierarchy comes from <Gruntz/UserLogic.h>).
// Only offsets / code bytes are load-bearing; names are placeholders for the
// recovered engine identities.
#include <Gruntz/ActNameRegistry.h> // the shared activation-name registry archetype
#include <Gruntz/MovingLogicBase.h> // CMovingLogicBase::Serialize (0x16e7f0) - shared serialize chain
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/ActReg.h>       // the shared CActReg coordinate-registry archetype
#include <Gruntz/LogicFnTable.h> // g_logicDispatch_646060's canonical LogicFnTable type
#include <Gruntz/FrontCandy.h> // 0xfa60 is CFrontCandy's slot-1 (??_7CFrontCandy+0x4), not CFrontCandyAni's
#include <Gruntz/FrontCandyAni.h>
#include <Gruntz/EyeCandyAni.h> // CEyeCandyAni (its TU folds in below, wave3-J)

// The global the advance handlers hand the sink (_g_6bf3bc; the per-frame
// draw-delta mirror). Declared extern "C" so the value-load reloc-masks.
extern "C" u32 g_engineFrameDelta;
#include <Gruntz/AnimSink.h>
#include <Gruntz/SerialObjRef.h> // CSerialObjRef::Chain (0x8c00) - the +0x34 sub-object round-trip

// CFrontCandy::Serialize @0x00fa60 - the vtable slot-1 body (??_7CFrontCandy slot 1,
// via thunk 0x2e46). Was MIS-ATTRIBUTED to CFrontCandyAni; the retail vtable read proves
// 0xfa60's data-ref is ??_7CFrontCandy+0x4 (CFrontCandyAni's slot-1 is 0xfdf0). Same
// two-chain body: base CUserLogic chain + the +0x34 sub-object chain.
RVA(0x0000fa60, 0x47)
i32 CFrontCandy::Serialize(i32 ar, i32 tag, i32 c, i32 d) {
    if (!((CMovingLogicBase*)this)->Serialize((CSerialArchive*)(ar), tag, c, d)) {
        return 0;
    }
    return ((CSerialObjRef*)&m_34)->Chain((CSerialArchive*)ar, tag, c, (CSerialObj*)d) != 0;
}

// CFrontCandy::~CFrontCandy @0x0fb00 - empty vtable-anchor dtor; folds the CUserLogic
// teardown (the /GX leaf-dtor archetype). Adjacent to CFrontCandy::Serialize (0xfa60).
RVA(0x0000fb00, 0x44)
CFrontCandy::~CFrontCandy() {}

// CFrontCandyAni::Serialize @0xfdf0 - the vtable slot-1 two-chain body (??_7CFrontCandyAni
// slot 1, via thunk 0x19a6): base CUserLogic chain + the +0x34 sub-object chain.
RVA(0x0000fdf0, 0x47)
i32 CFrontCandyAni::Serialize(i32 ar, i32 tag, i32 c, i32 d) {
    if (!((CMovingLogicBase*)this)->Serialize((CSerialArchive*)(ar), tag, c, d)) {
        return 0;
    }
    return ((CSerialObjRef*)&m_34)->Chain((CSerialArchive*)ar, tag, c, (CSerialObj*)d) != 0;
}

// CFrontCandyAni::~CFrontCandyAni @0xfe90 - empty vtable-anchor dtor (??_7CFrontCandyAni
// slot 0 -> sdd 0xfe60); folds the CUserLogic teardown (the /GX leaf-dtor archetype).
RVA(0x0000fe90, 0x44)
CFrontCandyAni::~CFrontCandyAni() {}

// CEyeCandyAni::GetTypeTag (0x0000ff00) is now an inline member in the class header.

// CEyeCandyAni::Serialize @0x00ff20 - the vtable slot-1 override: chain the shared
// CUserLogic serialize helper on `this`, and (only on success) the +0x34 sub-object's
// chain; both run the same (ar, tag, c, d) tuple. Returns the second chain's success
// normalized to a bool. Byte-identical to CCursorSnapSprite::Serialize (0x011880)
// save the two call displacements.
RVA(0x0000ff20, 0x47)
i32 CEyeCandyAni::Serialize(i32 ar, i32 tag, i32 c, i32 d) {
    if (!((CMovingLogicBase*)this)->Serialize((CSerialArchive*)(ar), tag, c, d)) {
        return 0;
    }
    return ((CSerialObjRef*)&m_34)->Chain((CSerialArchive*)ar, tag, c, (CSerialObj*)d) != 0;
}

// CEyeCandyAni::~CEyeCandyAni @0x0ffc0 - empty vtable-anchor dtor; folds the
// CUserLogic teardown (the /GX leaf-dtor archetype).
RVA(0x0000ffc0, 0x44)
CEyeCandyAni::~CEyeCandyAni() {}

// --- CFrontCandy (0x0abfa0), vptr 0x5e84ec --- CFrontCandy's Serialize (0xfa60) +
// dtor (0xfb00) already live in this TU; the ctor anchors GetTypeTag @0xfa40 + the
// ??_7CFrontCandy vtable here, reuniting the whole class. Folds the inline
// CUserLogic(obj) base + the shared z-clamp tail.
RVA(0x000abfa0, 0x1b6)
CFrontCandy::CFrontCandy(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    if (m_object->m_latchedAnimId != 0xf4240) {
        m_object->m_latchedAnimId = 0xf4240;
        m_object->m_flags |= 0x20000;
    }
    CGameObjLayer* aux = m_object->m_layer;
    if (aux != 0) {
        if (aux->m_zClampLo >= g_buteMgr.GetInt("World", "BigActHeight")
            || m_object->m_layer->m_zClampHi >= g_buteMgr.GetInt("World", "BigActHeight")) {
            if (m_object->m_7c != 0) {
                m_object->m_7c->m_08 &= ~6;
                m_object->m_7c->m_08 |= 1;
                m_38->m_flags &= ~0x1000002;
                m_38->m_flags |= 0x800000;
            }
        }
    }
}

// ===========================================================================
// CEyeCandyAni (ex EyeCandyAni.cpp, merged wave3-J): the 0x0abfa0-0x0ad527
// interval is ONE original TU - the text is an F-E-F sandwich (front @0xabfa0 |
// eye 0xac870..0xacf10 | front 0xacf40..0xad510) and the frontcandyani init
// frag @0xad110 sits in the front tail. Same shared ActNameRegistry/ActReg
// archetype; the sibling registry at 0x646060.
// ===========================================================================

// CEyeCandyAni's activation-coordinate registry singleton (@0x646060): the SAME cell
// pinned in LogicDispatchInit.cpp as g_logicDispatch_646060 (built over the fixed
// [2000,2010] range there via the shared zDArray dispatch-table ctor). That is its
// one canonical DATA-bound symbol; here we reference it through its CActReg activation
// facet (ResolveEntry) so the loads reloc-mask against the correctly-bound 0x246060.
extern LogicFnTable g_logicDispatch_646060; // 0x646060 (?g_logicDispatch_646060@@3ULogicFnTable@@A)

// The handler entry the registry yields: its first dword receives the per-frame
// handler PMF (AdvanceAnim, a 4-byte code ptr on this single-inheritance class).
typedef i32 (CEyeCandyAni::*EyeCandyHandler)();
struct CEyeCandyActEntry {
    EyeCandyHandler m_fn;
};

// CEyeCandyAni::CEyeCandyAni (0xac870) - fold the shared CUserLogic(obj) init, bind
// the "A" bute node, apply the cycle geometry, then run the shared eyecandy z-clamp
// + BigActHeight de-prioritize tail (the SAME archetype as CEyeCandy/CBehindCandyAni).
// @early-stop
// eh-ctor-vptr-restamp-position wall (docs/patterns/eh-ctor-vptr-restamp-position.md):
// body byte-identical; residual is the /GX leaf-vptr re-stamp position + EH-state ids.
RVA(0x000ac870, 0x20e)
CEyeCandyAni::CEyeCandyAni(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    if (m_38->m_geoId == 0) {
        m_savedGeoId = m_38->m_geoId;
        m_38->ApplyLookupGeometry("GAME_CYCLE100", 0);
    }
    CGameObject* o = m_object;
    if (o->m_latchedAnimId == 0 && o->m_layer != 0) {
        i32 v = o->m_layer->m_halfHeight + o->m_screenY + 0x186a0;
        if (o->m_latchedAnimId != v) {
            o->m_latchedAnimId = v;
            o->m_flags |= 0x20000;
        }
    }
    CGameObjLayer* aux = m_object->m_layer;
    if (aux != 0) {
        if (aux->m_zClampLo >= g_buteMgr.GetInt("World", "BigActHeight")
            || m_object->m_layer->m_zClampHi >= g_buteMgr.GetInt("World", "BigActHeight")) {
            if (m_object->m_7c != 0) {
                m_object->m_7c->m_08 &= ~6;
                m_object->m_7c->m_08 |= 1;
                m_38->m_flags &= ~0x1000002;
                m_38->m_flags |= 0x800000;
            }
        }
    }
}

// CEyeCandyAni::RunAct @0x0acbb0 - resolve the registry entry for id; if a handler
// is bound, re-resolve and invoke it as a PMF on this, else return the entry
// pointer. Same archetype as CAniCycle::RunAct (g_logicDispatch_646060 @0x646060
// viewed through its CActReg activation facet).
RVA(0x000acbb0, 0x102)
i32 CEyeCandyAni::RunAct(i32 id) {
    CEyeCandyActEntry* e =
        (CEyeCandyActEntry*)((CActReg*)&g_logicDispatch_646060)->ResolveEntry(id);
    if (e->m_fn != 0) {
        return (this->*((CEyeCandyActEntry*)((CActReg*)&g_logicDispatch_646060)->ResolveEntry(id))->m_fn)();
    }
    return (i32)e;
}

// CEyeCandyAni::RegisterActs @0x0acd10 - bind the class's per-frame handler
// (AdvanceAnim @0x0acf10) to the activation key "A" via the shared name registry,
// then bind id->entry in the class's coordinate registry (g_logicDispatch_646060
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
    i32 id = (i32)g_buteTree.Find(s_codeA);
    if (id == 0) {
        id = g_typeCounter;
        g_buteTree.Insert(s_codeA, (void*)id);
        char* slot = ActNameLookup(id);
        i32 n = g_typeColl.m_grown;
        void** list = (void**)g_typeColl.m_alloc;
        while (n-- != 0) {
            if (list != 0) {
                ((CString*)list)->CString::~CString();
            }
            list++;
        }
        ((CString*)slot)->operator=(s_codeA);
        g_typeCounter++;
    }
    ((CEyeCandyActEntry*)((CActReg*)&g_logicDispatch_646060)->ResolveEntry(id))->m_fn =
        &CEyeCandyAni::AdvanceAnim;
}

// CEyeCandyAni::AdvanceAnim @0x0acf10 - re-target the bound object's animation
// sub-object (m_38 + 0x1a0) to the current draw-delta (g_engineFrameDelta) and return 0.
// Byte-identical to CFrontCandyAni::AdvanceAnim (0x0ad510) save the call displacement.
RVA(0x000acf10, 0x17)
i32 CEyeCandyAni::AdvanceAnim() {
    ((CAniAdvanceCursor*)((char*)m_38 + 0x1a0))->Advance_15c360(g_engineFrameDelta);
    return 0;
}

// --- CFrontCandyAni (0x0acf40), vptr 0x5e83e4 --- the ctor anchors the
// ??_7CFrontCandyAni vtable in this TU. Folds the inline CUserLogic(obj) base.
RVA(0x000acf40, 0x16e)
CFrontCandyAni::CFrontCandyAni(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    if (m_38->m_geoId == 0) {
        m_40 = m_38->m_geoId;
        m_38->ApplyLookupGeometry("GAME_CYCLE100", 0);
    }
    if (m_object->m_latchedAnimId != 0xf4240) {
        m_object->m_latchedAnimId = 0xf4240;
        m_object->m_flags |= 0x20000;
    }
}

// The handler entry the per-class registry yields: its first dword receives the
// per-frame handler PMF (AdvanceAnim, a 4-byte code ptr on this single-inheritance
// class). FireActivation invokes it __thiscall on the trigger.
typedef i32 (CFrontCandyAni::*FrontCandyHandler)();
struct CFrontCandyActEntry {
    FrontCandyHandler m_fn;
};

// The class's activation-coordinate registry singleton (@0x6460b0), built over the
// fixed [2000,2010] range by the shared registry ctor (0x408710). CFrontCandyActReg
// is the shared <Gruntz/ActReg.h> CActReg archetype (was a per-file duplicate of its
// layout + ResolveEntry); it keeps its own placeholder name so the DATA-pinned
// global symbol is unchanged.
struct CFrontCandyActReg : public CActReg {};
DATA(0x002460b0)
CFrontCandyActReg g_frontCandyActReg; // 0x6460b0

// CFrontCandyAni::InitActReg @0x0ad130 - construct the class's activation-
// coordinate registry singleton (g_frontCandyActReg @0x6460b0) over the fixed
// range [2000, 2010] via the shared registry ctor (0x408710). Free init thunk.
RVA(0x000ad130, 0x15)
void CFrontCandyAni::InitActReg() {
    ((CZDArrayDerived*)&g_frontCandyActReg)->Construct(2000, 2010);
}

// CFrontCandyAni::FireActivation @0x0ad1b0 - look the activation coordinate up in
// the registry; if the entry has a registered handler, look it up again and
// dispatch it __thiscall on this. Byte-identical archetype to
// CParticlez::FireActivation (0x046d30).
RVA(0x000ad1b0, 0x102)
void CFrontCandyAni::FireActivation(i32 coord) {
    CFrontCandyActEntry* e = (CFrontCandyActEntry*)g_frontCandyActReg.ResolveEntry(coord);
    if (e->m_fn != 0) {
        CFrontCandyActEntry* e2 = (CFrontCandyActEntry*)g_frontCandyActReg.ResolveEntry(coord);
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
    i32 id = (i32)g_buteTree.Find(s_codeA);
    if (id == 0) {
        id = g_typeCounter;
        g_buteTree.Insert(s_codeA, (void*)id);
        char* slot = ActNameLookup(id);
        i32 n = g_typeColl.m_grown;
        void** list = (void**)g_typeColl.m_alloc;
        while (n-- != 0) {
            if (list != 0) {
                ((CString*)list)->CString::~CString();
            }
            list++;
        }
        ((CString*)slot)->operator=(s_codeA);
        g_typeCounter++;
    }
    ((CFrontCandyActEntry*)g_frontCandyActReg.ResolveEntry(id))->m_fn =
        &CFrontCandyAni::AdvanceAnim;
}

// CFrontCandyAni::AdvanceAnim @0x0ad510 - re-target the bound object's animation
// sub-object (m_38 + 0x1a0) to the current draw-delta (g_engineFrameDelta) and return 0.
// Byte-identical to CBehindCandyAni::AdvanceAnim save the call displacement.
RVA(0x000ad510, 0x17)
i32 CFrontCandyAni::AdvanceAnim() {
    ((CAniAdvanceCursor*)((char*)m_38 + 0x1a0))->Advance_15c360(g_engineFrameDelta);
    return 0;
}

#include <rva.h>
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>
#include <Gruntz/SerialArchive.h> // the serialize stream (== the real CFileMemBase)
SIZE_UNKNOWN(CFrontCandyActEntry);
SIZE_UNKNOWN(CFrontCandyActReg);
SIZE_UNKNOWN(CFrontCandyAni);
SIZE_UNKNOWN(CEyeCandyActEntry);
