// EyeCandyAni.cpp - the animated eyecandy game-object (C:\Proj\Gruntz), a
// CUserLogic leaf. Reconstructs the 1-arg ctor, the two vtable overrides, and the
// per-class activation acts (RegisterActs 0xacd10 + AdvanceAnim 0xacf10; the SAME
// archetype as the CFrontCandyAni sibling, but over CEyeCandyAni's 0x646060
// registry). The 0xacb30 InitActReg that constructs that registry lives in
// src/Gruntz/LogicDispatchInit.cpp (InitLogicDispatch_646060).
#include <Gruntz/EyeCandyAni.h>
#include <Gruntz/SerialObjRef.h>    // the shared serialized-object-reference (Chain @0x8c00)
#include <Gruntz/ActNameRegistry.h> // shared activation-name registry (g_buteTree/g_nameReg/CActName)
#include <Gruntz/ActReg.h>          // shared CActReg coordinate-registry archetype (ResolveEntry)
#include <Gruntz/AnimSink.h>        // the m_38+0x1a0 animation sink (SetAnim @0x15c360)

// g_buteTree (@0x6bf620; Find 0x16d190 __thiscall ret 4; Insert 0x16db90) comes from
// <Gruntz/ActNameRegistry.h>, along with g_nextActId / s_actKeyA / the shared name
// registry + CActName - the SAME globals every CUserLogic leaf's RegisterActs uses.

// The per-frame draw-delta mirror (_g_6bf3bc @0x6bf3bc) AdvanceAnim hands the sink;
// the value-load reloc-masks. Declared extern "C" (canonical decoration).
DATA(0x002bf3bc)
extern "C" u32 g_6bf3bc;

// CEyeCandyAni's activation-coordinate registry singleton (@0x646060), built over the
// fixed [2000,2010] range by InitLogicDispatch_646060 (0xacb30, LogicDispatchInit.cpp,
// where the DATA(0x00246060) symbol is pinned as g_logicDispatch_646060 - the same
// object viewed as its zDArray dispatch table). Here it is the CActReg activation
// view (ResolveEntry); referenced extern-only so the loads reloc-mask.
struct CEyeCandyActReg : public CActReg {};
extern CEyeCandyActReg g_eyeCandyActReg; // 0x646060

// The handler entry the registry yields: its first dword receives the per-frame
// handler PMF (AdvanceAnim, a 4-byte code ptr on this single-inheritance class).
typedef i32 (CEyeCandyAni::*EyeCandyHandler)();
struct CEyeCandyActEntry {
    EyeCandyHandler m_fn;
};

// CEyeCandyAni::GetTypeTag @0x00ff00 - the vtable slot-2 logic-type id accessor
// (the 6-byte `mov eax,<id>; ret` archetype).
RVA(0x0000ff00, 0x6)
LogicTypeId CEyeCandyAni::GetTypeTag() {
    return LOGIC_EYECANDYANI; // 0x3f4
}

// CEyeCandyAni::Serialize @0x00ff20 - the vtable slot-1 override: chain the shared
// CUserLogic serialize helper on `this`, and (only on success) the +0x34 sub-object's
// chain; both run the same (ar, tag, c, d) tuple. Returns the second chain's success
// normalized to a bool. Byte-identical to CCursorSnapSprite::Serialize (0x011880)
// save the two call displacements.
RVA(0x0000ff20, 0x47)
i32 CEyeCandyAni::Serialize(i32 ar, i32 tag, i32 c, i32 d) {
    if (!SerializeChain(ar, tag, c, d)) {
        return 0;
    }
    return ((CSerialObjRef*)&m_34)->Chain((CSerialArchive*)ar, tag, c, (CSerialObj*)d) != 0;
}

// CEyeCandyAni::CEyeCandyAni (0xac870) - fold the shared CUserLogic(obj) init, bind
// the "A" bute node, apply the cycle geometry, then run the shared eyecandy z-clamp
// + BigActHeight de-prioritize tail (the SAME archetype as CEyeCandy/CBehindCandyAni).
// @early-stop
// eh-ctor-vptr-restamp-position wall (docs/patterns/eh-ctor-vptr-restamp-position.md):
// body byte-identical; residual is the /GX leaf-vptr re-stamp position + EH-state ids.
RVA(0x000ac870, 0x20e)
CEyeCandyAni::CEyeCandyAni(CGameObject* obj) : CTileLogic(obj) {
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    if (m_38->m_geoId == 0) {
        m_savedGeoId = m_38->m_geoId;
        m_38->ApplyLookupGeometry("GAME_CYCLE100", 0);
    }
    CGameObject* o = m_object;
    if (o->m_latchedAnimId == 0 && o->m_layer != 0) {
        i32 v = o->m_layer->m_1c + o->m_screenY + 0x186a0;
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

// CEyeCandyAni::RegisterActs @0x0acd10 - bind the class's per-frame handler
// (AdvanceAnim @0x0acf10) to the activation key "A" via the shared name registry,
// then bind id->entry in the class's coordinate registry (g_eyeCandyActReg
// @0x646060). The SAME archetype as CFrontCandyAni::RegisterActs (0x0ad310).
//
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): logic
// byte-faithful (every call/immediate/branch/offset + the `mov [entry],offset
// AdvanceAnim` handler store match retail); residual is the slot-vs-id callee-saved
// register choice cascading into the free-loop count materialization. Deferred.
RVA(0x000acd10, 0x18d)
void CEyeCandyAni::RegisterActs() {
    i32 id = (i32)g_buteTree.Find(s_actKeyA);
    if (id == 0) {
        id = g_nextActId;
        g_buteTree.Insert(s_actKeyA, (void*)id);
        char* slot = ActNameLookup(id);
        i32 n = g_nameRegScratch;
        void** list = g_nameRegCurList;
        while (n-- != 0) {
            if (list != 0) {
                ((CActName*)list)->Free();
            }
            list++;
        }
        ((CActName*)slot)->Assign(s_actKeyA);
        g_nextActId++;
    }
    ((CEyeCandyActEntry*)g_eyeCandyActReg.ResolveEntry(id))->m_fn = &CEyeCandyAni::AdvanceAnim;
}

// CEyeCandyAni::AdvanceAnim @0x0acf10 - re-target the bound object's animation
// sub-object (m_38 + 0x1a0) to the current draw-delta (g_6bf3bc) and return 0.
// Byte-identical to CFrontCandyAni::AdvanceAnim (0x0ad510) save the call displacement.
RVA(0x000acf10, 0x17)
i32 CEyeCandyAni::AdvanceAnim() {
    ((CAnimSink*)((char*)m_38 + 0x1a0))->SetAnim(g_6bf3bc);
    return 0;
}

#include <rva.h>
SIZE_UNKNOWN(CEyeCandyActEntry);
SIZE_UNKNOWN(CEyeCandyActReg);
