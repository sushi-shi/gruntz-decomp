#include <Gruntz/ActNameRegistry.h> // the shared activation-name registry archetype
#include <Gruntz/TypeKeyColl.h>     // s_codeA/s_actKeyB registration keys
#include <Wap32/ZVec.h>
#include <Gruntz/ActReg.h> // the shared activation-registrar archetype (CActReg)
#include <Globals.h>
#include <Gruntz/PathHazardActReg.h> // g_actReg_646250 decl

// g_actReg_646250 (0x00246250): CActReg - no provable static init (the type has no
// default ctor / is runtime-Init'd), so the datum is named by symbol.
DATA_SYMBOL(0x00246250, 0x0, ?g_actReg_646250@@3UCActReg@@A)


extern "C" void Handler_4021d5(); // 0x4021d5
extern "C" void Handler_402252(); // 0x402252

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

RVA(0x000b3ae0, 0x15)
void ConstructActRange_646250() {
    g_actReg_646250.Construct(0x7d0, 0x7da);
}

// ===========================================================================
// RegisterActs_646250 @0x0b3cc0 - bind handler "A" (0x4021d5) and handler "B"
// (0x402252) into the per-class registry @0x646250.
// ===========================================================================
// @early-stop
// A/B inline asymmetry + register-pinning wall (see SpotLightActReg.cpp header).
RVA(0x000b3cc0, 0x2ac)
void RegisterActs_646250() {
    i32 id = reinterpret_cast<i32>(g_buteTree.Find("A"));
    if (id == 0) {
        g_buteTree.Insert("A", reinterpret_cast<void*>(g_typeCounter));
        id = g_typeCounter;
        char* slot = ActNameLookup(id);
        FreeNameSlotNodes();
        (reinterpret_cast<CString*>(slot))->operator=("A");
        g_typeCounter++;
    }
    *reinterpret_cast<void**>(g_actReg_646250.ResolveEntry(id)) = static_cast<void*>(&Handler_4021d5);

    i32 id2 = reinterpret_cast<i32>(g_buteTree.Find("B"));
    if (id2 == 0) {
        g_buteTree.Insert("B", reinterpret_cast<void*>(g_typeCounter));
        id2 = g_typeCounter;
        char* slot = ActNameLookup(id2);
        FreeNameSlotNodes();
        (reinterpret_cast<CString*>(slot))->operator=("B");
        g_typeCounter++;
    }
    *reinterpret_cast<void**>(g_actReg_646250.ResolveEntry(id2)) = static_cast<void*>(&Handler_402252);
}
