// LogicActRegistrars.cpp - the two-key activation/logic registrars (C:\Proj\Gruntz).
//
// Each function here is a level-load class registrar that interns TWO activation
// keys ("A" and "B") into the shared bute store + name registry (@0x6bf650, the
// SAME archetype as CStaticHazard::RegisterActs done twice), then resolves each
// key's id in the class's OWN per-coordinate registry (or the per-class logic
// dispatch table) and records the per-key handler entry. Two back-to-back
// single-key registrations; the byte-faithful shape is the shared
// fast-range/slow-Find/rebuild lookup the whole engine reuses.
//
// The owning leaf class for each table is not yet pinned (the registration keys
// live in .data, not a string constant); each is named by its registry/table
// address. The bodies are owner-independent (every global is a reloc-masked DATA
// extern, every callee external/no-body), so the byte match holds regardless.
//
// Re-homed to their ORIGINAL TUs (wave3-I):
//   RegisterLogic_6445e8 @0x0408b0 + CTeleporter_RegisterActs @0x041680
//     -> Wormhole.cpp (text-contained in the wormhole-trio obj)
//   ConstructActRange_644af0 @0x05bc50 + RegisterActs_644af0 @0x05be30
//     -> GruntCombat.cpp (text-contained in the 0x56f80 grunt obj; frag i342)
//
// @early-stop (all functions): the per-site inline-vs-out-of-line choice for the
// name-registry/per-class slow-path rebuild is the documented non-source-steerable
// A/B asymmetry - retail outlines handler-A's name rebuild (call 0x34960, the
// shared GetRetAddr()+Insert helper) while inlining handler-B's identical sequence;
// MSVC5 picks per call site from one inline source. Plus the register-pinning +
// count-down induction wall CStaticHazard/CKitchenSlime/CProjectile::RegisterType
// all carry (slot-vs-id callee-saved coloring -> free-loop count materialization).
// Logic + offsets + every call/immediate/branch/store are byte-faithful.
#include <Gruntz/ActNameRegistry.h> // the shared activation-name registry archetype
#include <Wap32/ZDArrayDerived.h>   // CZDArrayDerived::Construct (the [lo,hi] range static-init)
#include <Wap32/ZVec.h>
#include <Gruntz/ActReg.h> // the shared activation-registrar archetype (CActReg)
#include <Globals.h>

// The second activation key string "B" (0x60d1bc); g_nextActId/s_actKeyA + the
// name registry come from <Gruntz/ActNameRegistry.h>.
DATA(0x0020d1bc)
extern char s_actKeyB[];

// The per-class activation registry / logic dispatch table the final store targets
// is the shared <Gruntz/ActReg.h> CActReg archetype ([lo,hi] fast-range + slow
// Find/rebuild; scratch zeroed first, base+(id-lo)*stride yields the slot, cur is
// the slow result). The slow rebuild is written inline; MSVC outlines it to the
// shared 0x34960 helper at most sites.

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

// The class registries the per-key handler entries live in (untyped .data named
// by address, typed CActReg).
DATA(0x00246188)
extern CActReg g_actReg_646188; // 0x646188
DATA(0x00246250)
extern CActReg g_actReg_646250; // 0x646250
DATA(0x002514d8)
extern CActReg g_actReg_6514d8; // 0x6514d8

// The per-frame handler entries (ILT thunks) each registrar binds. Referenced by
// address so the `mov [entry],offset handler` store reloc-masks.
extern "C" void Handler_4025db(); // 0x4025db
extern "C" void Handler_402414(); // 0x402414
extern "C" void Handler_4021d5(); // 0x4021d5
extern "C" void Handler_402252(); // 0x402252
extern "C" void Handler_4037bf(); // 0x4037bf
extern "C" void Handler_402dd8(); // 0x402dd8

// The static initializers that build each registry's fast [0x7d0, 0x7da] id range
// (CZDArrayDerived::Construct). Re-homed from src/Stub/BoundaryLowerThunks.cpp
// (were RegRangeb15b0 / RegRangeb3ae0).
RVA(0x000b15b0, 0x15)
void ConstructActRange_646188() {
    ((CZDArrayDerived*)&g_actReg_646188)->Construct(0x7d0, 0x7da);
}

// ===========================================================================
// RegisterActs_646188 @0x0b1790 - bind handler "A" (0x4025db) and handler "B"
// (0x402414) into the per-class registry @0x646188.
// ===========================================================================
// @early-stop
// A/B inline asymmetry + register-pinning wall (see file header).
RVA(0x000b1790, 0x2ac)
void RegisterActs_646188() {
    i32 id = (i32)g_buteTree.Find(s_actKeyA);
    if (id == 0) {
        g_buteTree.Insert(s_actKeyA, (void*)g_nextActId);
        id = g_nextActId;
        char* slot = ActNameLookup(id);
        FreeNameSlotNodes();
        ((CString*)slot)->operator=(s_actKeyA);
        g_nextActId++;
    }
    *(void**)g_actReg_646188.ResolveEntry(id) = (void*)&Handler_4025db;

    i32 id2 = (i32)g_buteTree.Find(s_actKeyB);
    if (id2 == 0) {
        g_buteTree.Insert(s_actKeyB, (void*)g_nextActId);
        id2 = g_nextActId;
        char* slot = ActNameLookup(id2);
        FreeNameSlotNodes();
        ((CString*)slot)->operator=(s_actKeyB);
        g_nextActId++;
    }
    *(void**)g_actReg_646188.ResolveEntry(id2) = (void*)&Handler_402414;
}

RVA(0x000b3ae0, 0x15)
void ConstructActRange_646250() {
    ((CZDArrayDerived*)&g_actReg_646250)->Construct(0x7d0, 0x7da);
}

// ===========================================================================
// RegisterActs_646250 @0x0b3cc0 - bind handler "A" (0x4021d5) and handler "B"
// (0x402252) into the per-class registry @0x646250.
// ===========================================================================
// @early-stop
// A/B inline asymmetry + register-pinning wall (see file header).
RVA(0x000b3cc0, 0x2ac)
void RegisterActs_646250() {
    i32 id = (i32)g_buteTree.Find(s_actKeyA);
    if (id == 0) {
        g_buteTree.Insert(s_actKeyA, (void*)g_nextActId);
        id = g_nextActId;
        char* slot = ActNameLookup(id);
        FreeNameSlotNodes();
        ((CString*)slot)->operator=(s_actKeyA);
        g_nextActId++;
    }
    *(void**)g_actReg_646250.ResolveEntry(id) = (void*)&Handler_4021d5;

    i32 id2 = (i32)g_buteTree.Find(s_actKeyB);
    if (id2 == 0) {
        g_buteTree.Insert(s_actKeyB, (void*)g_nextActId);
        id2 = g_nextActId;
        char* slot = ActNameLookup(id2);
        FreeNameSlotNodes();
        ((CString*)slot)->operator=(s_actKeyB);
        g_nextActId++;
    }
    *(void**)g_actReg_646250.ResolveEntry(id2) = (void*)&Handler_402252;
}

// ===========================================================================
// RegisterActs_6514d8 @0x0119fa0 - bind handler "A" (0x4037bf) and handler "B"
// (0x402dd8) into the per-class registry @0x6514d8.
// ===========================================================================
// @early-stop
// A/B inline asymmetry + register-pinning wall (see file header).
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

// Tree-wide SIZE anchors for the shared registrar-collection archetypes
// (<Gruntz/ActReg.h>; used across many TUs). Moved here from the deleted
// src/Stub/BoundaryLowerThunks.cpp, next to this TU's CActReg globals.
SIZE_UNKNOWN(CLogicActTable);
SIZE_UNKNOWN(CLookupColl);
SIZE_UNKNOWN(CActReg);
