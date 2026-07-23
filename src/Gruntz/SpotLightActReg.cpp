#include <Gruntz/ActNameRegistry.h> // the shared activation-name registry archetype
#include <Gruntz/TypeKeyColl.h>     // s_codeA/s_actKeyB registration keys
#include <Wap32/ZVec.h>
#include <Gruntz/ActReg.h>          // the shared activation-registrar archetype (CActReg)
#include <Gruntz/SpotLightActReg.h> // CActRegPool<CSpotLight>::s_table decl
#include <Gruntz/SpotLight.h>

// CActRegPool<CSpotLight>::s_table (0x00246188): CActReg - no provable static init (the type has no
// default ctor / is runtime-Init'd), so the datum is named by symbol.
template<> DATA(0x00246188)
CActReg CActRegPool<CSpotLight>::s_table(2000, 2010);

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
// RegisterActs_646188 @0x0b1790 - bind handler "A" (0x4025db) and handler "B"
// (0x402414) into the per-class registry @0x646188.
// ===========================================================================
// @early-stop
// A/B inline asymmetry + register-pinning wall: retail outlines handler-A's name
// rebuild (call 0x34960, the shared GetRetAddr()+Insert helper) while inlining
// handler-B's identical sequence; plus the slot-vs-id callee-saved coloring ->
// free-loop count materialization. Logic + offsets + every call/immediate/branch/
// store are byte-faithful.
RVA(0x000b1790, 0x2ac)
void RegisterActs_646188() {
    i32 id = reinterpret_cast<i32>(g_buteTree.Find("A"));
    if (id == 0) {
        g_buteTree.Insert("A", reinterpret_cast<void*>(g_typeCounter));
        id = g_typeCounter;
        char* slot = ActNameLookup(id);
        FreeNameSlotNodes();
        (reinterpret_cast<CString*>(slot))->operator=("A");
        g_typeCounter++;
    }
    *reinterpret_cast<void**>(CActRegPool<CSpotLight>::s_table.ResolveEntry(id)) =
        static_cast<void*>(&SpotLightActA);

    i32 id2 = reinterpret_cast<i32>(g_buteTree.Find("B"));
    if (id2 == 0) {
        g_buteTree.Insert("B", reinterpret_cast<void*>(g_typeCounter));
        id2 = g_typeCounter;
        char* slot = ActNameLookup(id2);
        FreeNameSlotNodes();
        (reinterpret_cast<CString*>(slot))->operator=("B");
        g_typeCounter++;
    }
    *reinterpret_cast<void**>(CActRegPool<CSpotLight>::s_table.ResolveEntry(id2)) =
        static_cast<void*>(&SpotLightActB);
}
