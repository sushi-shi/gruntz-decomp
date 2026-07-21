#include <Gruntz/ActNameRegistry.h> // the shared activation-name registry archetype
#include <Wap32/ZVec.h>
#include <Gruntz/ActReg.h> // the shared CActReg coordinate-registry archetype
#include <Gruntz/AniCycle.h>
#include <Gruntz/SerialArchive.h> // CSerialArchive (the inherited CWapX::Chain arg; ex SerialObjRef.h)

DATA(0x00246088)
extern CActReg g_aniCycleActReg; // (the CActReg archetype IS the type) // 0x646088

RVA(0x0000f470, 0x47)
i32 CAniCycle::SerializeMove(CGruntArchive* ar, i32 tag, i32 c, i32 d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    return Chain(ar, tag, c, reinterpret_cast<CGameObject*>(d)) != 0;
}

// CAniCycle::~CAniCycle @0x0f510 - empty vtable-anchor dtor; folds the CUserLogic
// teardown (the destructible +0x18 link forces the /GX EH frame).
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CAniCycle() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
#include <rva.h>
RVA_COMPGEN(0x0000f510, 0x44, ??1CAniCycle@@UAE@XZ)

RVA(0x000aad20, 0x15c)
CAniCycle::CAniCycle(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_38->m_flags |= 1;
    if (m_38->m_1a0.m_14 == 0) {
        m_value = m_38->m_1a0.m_14;
        m_38->ApplyLookupGeometry("GAME_CYCLE100", 0);
    }
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
}

RVA(0x000aaf00, 0x15)
void CAniCycle::InitActReg() {
    g_aniCycleActReg.Construct(2000, 2010);
}

RVA(0x000aaf80, 0x102)
void CAniCycle::FireActivation(i32 id) {
    CAniCycleActEntry* e = reinterpret_cast<CAniCycleActEntry*>(g_aniCycleActReg.ResolveEntry(id));
    if (e->m_fn != 0) {
        (this->*(reinterpret_cast<CAniCycleActEntry*>(g_aniCycleActReg.ResolveEntry(id)))->m_fn)();
    }
}

// CAniCycle::RegisterActs @0x0ab0e0 - bind the class's per-frame handler
// (AdvanceAnim @0x0ab2e0) to the activation key "A" via the shared name registry.
// The SAME archetype as CBehindCandyAni::RegisterActs.
//
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): logic
// byte-faithful (every call/immediate/branch/offset + the `mov [entry],offset
// AdvanceAnim` handler store match retail); residual is the slot-vs-id callee-saved
// register choice cascading into the free-loop count materialization. Deferred.
RVA(0x000ab0e0, 0x18d)
void CAniCycle::RegisterActs() {
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
    (reinterpret_cast<CAniCycleActEntry*>(g_aniCycleActReg.ResolveEntry(id)))->m_fn = static_cast<i32 (CUserLogic::*)()>(&CAniCycle::AdvanceAnim);
}

#include <rva.h>
#include <Wap32/ZVec.h>
#include <Gruntz/SerialArchive.h> // the serialize stream (== the real CFileMemBase)
SIZE_UNKNOWN(CAniCycleActEntry);
SIZE_UNKNOWN(CAniCycleActReg);
