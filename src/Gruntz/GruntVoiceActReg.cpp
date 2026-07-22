#include <Gruntz/ActNameRegistry.h> // the shared activation-name registry archetype
#include <Gruntz/TypeKeyColl.h>     // s_codeA/s_actKeyB registration keys
#include <Wap32/ZVec.h>
#include <Gruntz/ActReg.h> // the shared activation-registrar archetype (CActReg)
#include <Globals.h>
#include <Gruntz/GruntVoiceActReg.h> // g_actReg_6514d8 decl

// g_actReg_6514d8 (0x002514d8): CActReg - no provable static init (the type has no
// default ctor / is runtime-Init'd), so the datum is named by symbol.
DATA_SYMBOL(0x002514d8, 0x0, ?g_actReg_6514d8@@3UCActReg@@A)



static inline void FreeNameSlotNodes() {
    i32 n = g_typeColl.m_grown;
    void** list = reinterpret_cast<void**>(g_typeColl.m_alloc);
    while (n-- != 0) {
        if (list != 0) {
            (reinterpret_cast<CString*>(list))->CString::~CString();
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
    i32 id = reinterpret_cast<i32>(g_buteTree.Find("A"));
    if (id == 0) {
        g_buteTree.Insert("A", reinterpret_cast<void*>(g_typeCounter));
        id = g_typeCounter;
        char* slot = ActNameLookup(id);
        FreeNameSlotNodes();
        (reinterpret_cast<CString*>(slot))->operator=("A");
        g_typeCounter++;
    }
    *reinterpret_cast<void**>(g_actReg_6514d8.ResolveEntry(id)) = static_cast<void*>(&Handler_4037bf);

    i32 id2 = reinterpret_cast<i32>(g_buteTree.Find("B"));
    if (id2 == 0) {
        g_buteTree.Insert("B", reinterpret_cast<void*>(g_typeCounter));
        id2 = g_typeCounter;
        char* slot = ActNameLookup(id2);
        FreeNameSlotNodes();
        (reinterpret_cast<CString*>(slot))->operator=("B");
        g_typeCounter++;
    }
    *reinterpret_cast<void**>(g_actReg_6514d8.ResolveEntry(id2)) = static_cast<void*>(&Handler_402dd8);
}
