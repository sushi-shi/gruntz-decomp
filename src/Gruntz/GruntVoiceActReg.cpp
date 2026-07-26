#include <Gruntz/ActNameRegistry.h> // the shared activation-name registry archetype
#include <Gruntz/TypeKeyColl.h>     // s_codeA/s_actKeyB registration keys
#include <Wap32/ZVec.h>
#include <Gruntz/ActReg.h>           // the shared activation-registrar archetype (CActReg)
#include <Gruntz/GruntVoiceActReg.h> // CActRegPool<CGruntVoice>::s_table decl
#include <Gruntz/GruntVoice.h>

static inline void FreeNameSlotNodes() {
    i32 n = g_typeColl.m_grown;
    CString* list = reinterpret_cast<CString*>(g_typeColl.m_alloc);
    while (n-- != 0) {
        if (list != 0) {
            list->CString::~CString();
        }
        list++;
    }
}

// ===========================================================================
// RegisterActs_6514d8 @0x0119fa0 - bind handler "A" (0x4037bf) and handler "B"
// (0x402dd8) into the per-class registry @0x6514d8. (The registry's Construct
// already lives in GruntVoice.cpp, over [2000, 2010].)
// ===========================================================================
// @early-stop
// A/B inline asymmetry + register-pinning wall (see SpotLightActReg.cpp header).
RVA(0x00119fa0, 0x2ac)
void RegisterActs_6514d8() {
    i32 id = ActFindId("A");
    if (id == 0) {
        ActInsertId("A", g_typeCounter);
        id = g_typeCounter;
        CString* slot = ActNameLookup(id);
        FreeNameSlotNodes();
        *slot = "A";
        g_typeCounter++;
    }
    *reinterpret_cast<void**>(CActRegPool<CGruntVoice>::s_table.ResolveEntry(id)) =
        static_cast<void*>(&GruntVoiceActA);

    i32 id2 = ActFindId("B");
    if (id2 == 0) {
        ActInsertId("B", g_typeCounter);
        id2 = g_typeCounter;
        CString* slot = ActNameLookup(id2);
        FreeNameSlotNodes();
        *slot = "B";
        g_typeCounter++;
    }
    *reinterpret_cast<void**>(CActRegPool<CGruntVoice>::s_table.ResolveEntry(id2)) =
        static_cast<void*>(&GruntVoiceActB);
}
