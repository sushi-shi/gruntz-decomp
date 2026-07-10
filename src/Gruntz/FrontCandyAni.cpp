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
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/ActReg.h> // the shared CActReg coordinate-registry archetype
#include <Gruntz/FrontCandy.h> // 0xfa60 is CFrontCandy's slot-1 (??_7CFrontCandy+0x4), not CFrontCandyAni's
#include <Gruntz/FrontCandyAni.h>
#include <Gruntz/AnimSink.h>
#include <Gruntz/SerialObjRef.h> // CSerialObjRef::Chain (0x8c00) - the +0x34 sub-object round-trip

// CFrontCandy::Serialize @0x00fa60 - the vtable slot-1 body (??_7CFrontCandy slot 1,
// via thunk 0x2e46). Was MIS-ATTRIBUTED to CFrontCandyAni; the retail vtable read proves
// 0xfa60's data-ref is ??_7CFrontCandy+0x4 (CFrontCandyAni's slot-1 is 0xfdf0). Same
// two-chain body: base CUserLogic chain + the +0x34 sub-object chain.
RVA(0x0000fa60, 0x47)
i32 CFrontCandy::Serialize(i32 ar, i32 tag, i32 c, i32 d) {
    if (!SerializeChain(ar, tag, c, d)) {
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
    if (!SerializeChain(ar, tag, c, d)) {
        return 0;
    }
    return ((CSerialObjRef*)&m_34)->Chain((CSerialArchive*)ar, tag, c, (CSerialObj*)d) != 0;
}

// CFrontCandyAni::~CFrontCandyAni @0xfe90 - empty vtable-anchor dtor (??_7CFrontCandyAni
// slot 0 -> sdd 0xfe60); folds the CUserLogic teardown (the /GX leaf-dtor archetype).
RVA(0x0000fe90, 0x44)
CFrontCandyAni::~CFrontCandyAni() {}

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
extern CFrontCandyActReg g_frontCandyActReg; // 0x6460b0

// CFrontCandyAni::InitActReg @0x0ad130 - construct the class's activation-
// coordinate registry singleton (g_frontCandyActReg @0x6460b0) over the fixed
// range [2000, 2010] via the shared registry ctor (0x408710). Free init thunk.
RVA(0x000ad130, 0x15)
void CFrontCandyAni::InitActReg() {
    ((CZDArrayDerived*)&g_frontCandyActReg)->Construct(2000, 2010);
}

// The global the advance hands the sink (_g_6bf3bc; the per-frame draw-delta
// mirror). Declared extern "C" here so the value-load reloc-masks.
DATA(0x002bf3bc)
extern "C" u32 g_6bf3bc;

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
    i32 id = (i32)g_buteTree.Find(s_actKeyA);
    if (id == 0) {
        id = g_nextActId;
        g_buteTree.Insert(s_actKeyA, (void*)id);
        char* slot = ActNameLookup(id);
        i32 n = g_nameRegScratch;
        void** list = g_nameRegCurList;
        while (n-- != 0) {
            if (list != 0) {
                ((CString*)list)->CString::~CString();
            }
            list++;
        }
        ((CString*)slot)->operator=(s_actKeyA);
        g_nextActId++;
    }
    ((CFrontCandyActEntry*)g_frontCandyActReg.ResolveEntry(id))->m_fn =
        &CFrontCandyAni::AdvanceAnim;
}

// CFrontCandyAni::AdvanceAnim @0x0ad510 - re-target the bound object's animation
// sub-object (m_38 + 0x1a0) to the current draw-delta (g_6bf3bc) and return 0.
// Byte-identical to CBehindCandyAni::AdvanceAnim save the call displacement.
RVA(0x000ad510, 0x17)
i32 CFrontCandyAni::AdvanceAnim() {
    ((CAniAdvanceCursor*)((char*)m_38 + 0x1a0))->Advance_15c360(g_6bf3bc);
    return 0;
}

#include <rva.h>
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>
SIZE_UNKNOWN(CFrontCandyActEntry);
SIZE_UNKNOWN(CFrontCandyActReg);
SIZE_UNKNOWN(CFrontCandyAni);
