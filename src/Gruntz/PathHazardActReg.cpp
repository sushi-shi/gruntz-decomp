// PathHazardActReg.cpp - CPathHazard's two-key activation registrar (C:\Proj\Gruntz).
//
// Split out of the LogicActRegistrars.cpp holding TU (REHOME D1). Owner CRACKED:
// the registry table @0x646250 is CPathHazard's activation-dispatch registry
// (g_actReg_646250 is walked by CPathHazard's activation handler - see
// PathHazard.cpp). The Construct+Register pair @0xb3ae0/0xb3cc0 is text-contained in
// CPathHazard's obj (interleaved with PathHazard.cpp's 0xb35a0/0xb3b60 methods) - a
// deeper pass may FOLD it into PathHazard.cpp; kept standalone here (owner-TU
// completeness + parallel-worker scope).
//
// Interns two activation keys ("A" 0x60a454 and "B" 0x60d1bc) into the shared bute
// store + name registry, then binds each key's per-frame handler into the registry.
// Bodies are owner-independent (every global is a reloc-masked DATA extern, every
// callee external/no-body), so the byte match holds regardless.
#include <Gruntz/ActNameRegistry.h> // the shared activation-name registry archetype
#include <Wap32/ZDArrayDerived.h>   // CZDArrayDerived::Construct (the [lo,hi] range static-init)
#include <Wap32/ZVec.h>
#include <Gruntz/ActReg.h> // the shared activation-registrar archetype (CActReg)
#include <Globals.h>

// The second activation key string "B" (0x60d1bc); "A" + g_typeCounter + the name
// registry come from <Gruntz/ActNameRegistry.h>.
DATA(0x0020d1bc)
extern char s_actKeyB[];

// CPathHazard's per-class activation registry (untyped .data named by address, typed CActReg).
DATA(0x00246250)
CActReg g_actReg_646250; // 0x646250

// The per-frame handler entries (ILT thunks) this registrar binds.
extern "C" void Handler_4021d5(); // 0x4021d5
extern "C" void Handler_402252(); // 0x402252

// The shared name-slot free loop both key blocks run before assigning the key.
static inline void FreeNameSlotNodes() {
    i32 n = g_typeCount;
    void** list = (void**)g_typeNodes;
    while (n-- != 0) {
        if (list != 0) {
            ((CString*)list)->CString::~CString();
        }
        list++;
    }
}

// The static initializer that builds registry 646250's fast [0x7d0, 0x7da] id range.
RVA(0x000b3ae0, 0x15)
void ConstructActRange_646250() {
    ((CZDArrayDerived*)&g_actReg_646250)->Construct(0x7d0, 0x7da);
}

// ===========================================================================
// RegisterActs_646250 @0x0b3cc0 - bind handler "A" (0x4021d5) and handler "B"
// (0x402252) into the per-class registry @0x646250.
// ===========================================================================
// @early-stop
// A/B inline asymmetry + register-pinning wall (see SpotLightActReg.cpp header).
RVA(0x000b3cc0, 0x2ac)
void RegisterActs_646250() {
    i32 id = (i32)g_buteTree.Find(s_codeA);
    if (id == 0) {
        g_buteTree.Insert(s_codeA, (void*)g_typeCounter);
        id = g_typeCounter;
        char* slot = ActNameLookup(id);
        FreeNameSlotNodes();
        ((CString*)slot)->operator=(s_codeA);
        g_typeCounter++;
    }
    *(void**)g_actReg_646250.ResolveEntry(id) = (void*)&Handler_4021d5;

    i32 id2 = (i32)g_buteTree.Find(s_actKeyB);
    if (id2 == 0) {
        g_buteTree.Insert(s_actKeyB, (void*)g_typeCounter);
        id2 = g_typeCounter;
        char* slot = ActNameLookup(id2);
        FreeNameSlotNodes();
        ((CString*)slot)->operator=(s_actKeyB);
        g_typeCounter++;
    }
    *(void**)g_actReg_646250.ResolveEntry(id2) = (void*)&Handler_402252;
}
