// SpotLightActReg.cpp - CSpotLight's two-key activation registrar (C:\Proj\Gruntz).
//
// Split out of the LogicActRegistrars.cpp holding TU (REHOME D1). Owner CRACKED:
// the registry table @0x646188 is CSpotLight's activation-dispatch registry
// (g_actReg_646188 is walked by CSpotLight's activation handler - see
// SpotLightCtor.cpp). The Construct+Register pair @0xb15b0/0xb1790 is text-contained
// in CSpotLight's ctor obj (interleaved between SpotLightCtor.cpp's 0xb1200 ctor and
// its 0xb1630/0xb1af0 methods) - a deeper pass may FOLD it into SpotLightCtor.cpp;
// kept standalone here (owner-TU completeness + parallel-worker scope).
//
// Interns two activation keys ("A" 0x60a454 and "B" 0x60d1bc) into the shared bute
// store + name registry, then binds each key's per-frame handler into the registry.
// Bodies are owner-independent (every global is a reloc-masked DATA extern, every
// callee external/no-body), so the byte match holds regardless.
#include <Gruntz/ActNameRegistry.h> // the shared activation-name registry archetype
#include <Gruntz/TypeKeyColl.h>     // s_codeA/s_actKeyB registration keys
#include <Wap32/ZDArrayDerived.h>   // CZDArrayDerived::Construct (the [lo,hi] range static-init)
#include <Wap32/ZVec.h>
#include <Gruntz/ActReg.h> // the shared activation-registrar archetype (CActReg)
#include <Globals.h>

// The second activation key string "B" (0x60d1bc); "A" + g_typeCounter + the name
// registry come from <Gruntz/ActNameRegistry.h>.

// CSpotLight's per-class activation registry (untyped .data named by address, typed CActReg).
DATA(0x00246188)
CActReg g_actReg_646188; // 0x646188

// The per-frame handler entries (ILT thunks) this registrar binds.
extern "C" void Handler_4025db(); // 0x4025db
extern "C" void Handler_402414(); // 0x402414

// The shared name-slot free loop both key blocks run before assigning the key.
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

// The static initializer that builds registry 646188's fast [0x7d0, 0x7da] id range.
RVA(0x000b15b0, 0x15)
void ConstructActRange_646188() {
    (reinterpret_cast<CZDArrayDerived*>(&g_actReg_646188))->Construct(0x7d0, 0x7da);
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
    *reinterpret_cast<void**>(g_actReg_646188.ResolveEntry(id)) = static_cast<void*>(&Handler_4025db);

    i32 id2 = reinterpret_cast<i32>(g_buteTree.Find("B"));
    if (id2 == 0) {
        g_buteTree.Insert("B", reinterpret_cast<void*>(g_typeCounter));
        id2 = g_typeCounter;
        char* slot = ActNameLookup(id2);
        FreeNameSlotNodes();
        (reinterpret_cast<CString*>(slot))->operator=("B");
        g_typeCounter++;
    }
    *reinterpret_cast<void**>(g_actReg_646188.ResolveEntry(id2)) = static_cast<void*>(&Handler_402414);
}

// Tree-wide SIZE anchors for the shared registrar-collection archetypes
// (<Gruntz/ActReg.h>; used across many TUs). Relocated here from the drained
// LogicActRegistrars.cpp / LogicActReg.cpp so they survive tree-wide exactly once.
SIZE_UNKNOWN(CLogicActTable);
SIZE_UNKNOWN(CLookupColl);
SIZE_UNKNOWN(CActReg);
