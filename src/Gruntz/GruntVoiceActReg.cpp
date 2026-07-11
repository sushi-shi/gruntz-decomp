// GruntVoiceActReg.cpp - CGruntVoice's two-key activation registrar (C:\Proj\Gruntz).
//
// Split out of the LogicActRegistrars.cpp holding TU (REHOME D1). Owner CRACKED:
// the registry table @0x6514d8 is CGruntVoice's activation-dispatch registry
// (g_vactColl @0x6514d8; GruntVoice.cpp already builds its [2000,2010] range and
// GruntIndicatorSprite.h names it CGruntVoice's g_vactColl). RegisterActs_6514d8
// @0x119fa0 is text-contained in CGruntVoice's obj (interleaved between
// GruntVoice.cpp's 0x119e40 and 0x11a320 methods; its Construct already lives in
// GruntVoice.cpp) - a deeper pass may FOLD it into GruntVoice.cpp; kept standalone
// here (owner-TU completeness + parallel-worker scope).
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

// The second activation key string "B" (0x60d1bc); "A" + g_nextActId + the name
// registry come from <Gruntz/ActNameRegistry.h>.
DATA(0x0020d1bc)
extern char s_actKeyB[];

// CGruntVoice's per-class activation registry (untyped .data named by address, typed CActReg).
DATA(0x002514d8)
extern CActReg g_actReg_6514d8; // 0x6514d8

// The per-frame handler entries (ILT thunks) this registrar binds.
extern "C" void Handler_4037bf(); // 0x4037bf
extern "C" void Handler_402dd8(); // 0x402dd8

// The shared name-slot free loop both key blocks run before assigning the key.
static inline void FreeNameSlotNodes() {
    i32 n = g_nameRegScratch;
    void** list = g_nameRegCurList;
    while (n-- != 0) {
        if (list != 0) {
            ((CString*)list)->CString::~CString();
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
    i32 id = (i32)g_buteTree.Find(s_actKeyA);
    if (id == 0) {
        g_buteTree.Insert(s_actKeyA, (void*)g_nextActId);
        id = g_nextActId;
        char* slot = ActNameLookup(id);
        FreeNameSlotNodes();
        ((CString*)slot)->operator=(s_actKeyA);
        g_nextActId++;
    }
    *(void**)g_actReg_6514d8.ResolveEntry(id) = (void*)&Handler_4037bf;

    i32 id2 = (i32)g_buteTree.Find(s_actKeyB);
    if (id2 == 0) {
        g_buteTree.Insert(s_actKeyB, (void*)g_nextActId);
        id2 = g_nextActId;
        char* slot = ActNameLookup(id2);
        FreeNameSlotNodes();
        ((CString*)slot)->operator=(s_actKeyB);
        g_nextActId++;
    }
    *(void**)g_actReg_6514d8.ResolveEntry(id2) = (void*)&Handler_402dd8;
}
