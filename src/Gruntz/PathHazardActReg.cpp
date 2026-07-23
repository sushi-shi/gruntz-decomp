#include <Gruntz/ActNameRegistry.h> // the shared activation-name registry archetype
#include <Gruntz/TypeKeyColl.h>     // s_codeA/s_actKeyB registration keys
#include <Wap32/ZVec.h>
#include <Gruntz/ActReg.h>           // the shared activation-registrar archetype (CActReg)
#include <Gruntz/PathHazardActReg.h> // CActRegPool<CPathHazard>::s_table decl
#include <Gruntz/PathHazard.h>

// CActRegPool<CPathHazard>::s_table (0x00246250): CActReg - no provable static init (the type has no
// default ctor / is runtime-Init'd), so the datum is named by symbol.
template<> DATA(0x00246250)
CActReg CActRegPool<CPathHazard>::s_table(2000, 2010);

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

RVA_COMPGEN(0x000b3ac0, 0xa, _$E735936)
RVA_COMPGEN(0x000b3ae0, 0x15, _$E735968)
RVA_COMPGEN(0x000b3b10, 0xe, _$E736016)
RVA_COMPGEN(0x000b3b30, 0x1f, _$E736048)

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
    *reinterpret_cast<void**>(CActRegPool<CPathHazard>::s_table.ResolveEntry(id)) =
        static_cast<void*>(&PathHazardActA);

    i32 id2 = reinterpret_cast<i32>(g_buteTree.Find("B"));
    if (id2 == 0) {
        g_buteTree.Insert("B", reinterpret_cast<void*>(g_typeCounter));
        id2 = g_typeCounter;
        char* slot = ActNameLookup(id2);
        FreeNameSlotNodes();
        (reinterpret_cast<CString*>(slot))->operator=("B");
        g_typeCounter++;
    }
    *reinterpret_cast<void**>(CActRegPool<CPathHazard>::s_table.ResolveEntry(id2)) =
        static_cast<void*>(&PathHazardActB);
}
