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
// @early-stop (all functions): the per-site inline-vs-out-of-line choice for the
// name-registry/per-class slow-path rebuild is the documented non-source-steerable
// A/B asymmetry - retail outlines handler-A's name rebuild (call 0x34960, the
// shared ActAlloc()+Insert helper) while inlining handler-B's identical sequence;
// MSVC5 picks per call site from one inline source. Plus the register-pinning +
// count-down induction wall CStaticHazard/CKitchenSlime/CProjectile::RegisterType
// all carry (slot-vs-id callee-saved coloring -> free-loop count materialization).
// Logic + offsets + every call/immediate/branch/store are byte-faithful.
#include <Gruntz/ActNameRegistry.h> // the shared activation-name registry archetype
#include <Gruntz/ActReg.h>          // the shared activation-registrar archetype (CActReg)
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
static inline void FreeNameList() {
    i32 n = g_nameRegScratch;
    void** list = g_nameRegCurList;
    while (n-- != 0) {
        if (list != 0) {
            ((CActName*)list)->Free();
        }
        list++;
    }
}

// The per-class logic dispatch table (RTTI ULogicFnTable, @0x6445e8); same field
// layout as CActReg. Modeled here through the CActReg lookup path (ResolveEntry),
// so it uses the shared <Gruntz/ActReg.h> CLogicActTable alias rather than the
// polymorphic zDArray view <Gruntz/LogicFnTable.h> the icon/wormhole TUs build with.
DATA(0x002445e8)
extern CLogicActTable g_logicDispatch_6445e8; // 0x6445e8

// The class registries the per-key handler entries live in. g_teleporterActReg
// (RTTI UCTeleporterActReg, @0x6446b0) is CTeleporter's named registry (the shared
// <Gruntz/ActReg.h> CTeleporterActReg alias); the rest are untyped .data named by
// address, typed CActReg.
DATA(0x002446b0)
extern CTeleporterActReg g_teleporterActReg; // 0x6446b0
DATA(0x00246188)
extern CActReg g_actReg_646188; // 0x646188
DATA(0x00246250)
extern CActReg g_actReg_646250; // 0x646250
DATA(0x002514d8)
extern CActReg g_actReg_6514d8; // 0x6514d8

// The per-frame handler entries (ILT thunks) each registrar binds. Referenced by
// address so the `mov [entry],offset handler` store reloc-masks.
extern "C" void Handler_4021f8(); // 0x4021f8
extern "C" void Handler_403418(); // 0x403418
extern "C" void Handler_40187a(); // 0x40187a
extern "C" void Handler_403846(); // 0x403846
extern "C" void Handler_4025db(); // 0x4025db
extern "C" void Handler_402414(); // 0x402414
extern "C" void Handler_4021d5(); // 0x4021d5
extern "C" void Handler_402252(); // 0x402252
extern "C" void Handler_4037bf(); // 0x4037bf
extern "C" void Handler_402dd8(); // 0x402dd8

// ===========================================================================
// RegisterLogic_6445e8 @0x0408b0 - bind handler "A" (0x4021f8) and handler "B"
// (0x403418) into the logic dispatch table @0x6445e8 (proximity owner:
// CWormhole | CGruntPuddle).
// ===========================================================================
// @early-stop
// A/B inline asymmetry + register-pinning wall (see file header).
RVA(0x000408b0, 0x2ac)
void RegisterLogic_6445e8() {
    i32 id = (i32)g_buteTree.Find(s_actKeyA);
    if (id == 0) {
        g_buteTree.Insert(s_actKeyA, (void*)g_nextActId);
        id = g_nextActId;
        char* slot = ActNameLookup(id);
        FreeNameList();
        ((CActName*)slot)->Assign(s_actKeyA);
        g_nextActId++;
    }
    *(void**)g_logicDispatch_6445e8.ResolveEntry(id) = (void*)&Handler_4021f8;

    i32 id2 = (i32)g_buteTree.Find(s_actKeyB);
    if (id2 == 0) {
        g_buteTree.Insert(s_actKeyB, (void*)g_nextActId);
        id2 = g_nextActId;
        char* slot = ActNameLookup(id2);
        FreeNameList();
        ((CActName*)slot)->Assign(s_actKeyB);
        g_nextActId++;
    }
    *(void**)g_logicDispatch_6445e8.ResolveEntry(id2) = (void*)&Handler_403418;
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
        FreeNameList();
        ((CActName*)slot)->Assign(s_actKeyA);
        g_nextActId++;
    }
    *(void**)g_actReg_646188.ResolveEntry(id) = (void*)&Handler_4025db;

    i32 id2 = (i32)g_buteTree.Find(s_actKeyB);
    if (id2 == 0) {
        g_buteTree.Insert(s_actKeyB, (void*)g_nextActId);
        id2 = g_nextActId;
        char* slot = ActNameLookup(id2);
        FreeNameList();
        ((CActName*)slot)->Assign(s_actKeyB);
        g_nextActId++;
    }
    *(void**)g_actReg_646188.ResolveEntry(id2) = (void*)&Handler_402414;
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
        FreeNameList();
        ((CActName*)slot)->Assign(s_actKeyA);
        g_nextActId++;
    }
    *(void**)g_actReg_646250.ResolveEntry(id) = (void*)&Handler_4021d5;

    i32 id2 = (i32)g_buteTree.Find(s_actKeyB);
    if (id2 == 0) {
        g_buteTree.Insert(s_actKeyB, (void*)g_nextActId);
        id2 = g_nextActId;
        char* slot = ActNameLookup(id2);
        FreeNameList();
        ((CActName*)slot)->Assign(s_actKeyB);
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
        FreeNameList();
        ((CActName*)slot)->Assign(s_actKeyA);
        g_nextActId++;
    }
    *(void**)g_actReg_6514d8.ResolveEntry(id) = (void*)&Handler_4037bf;

    i32 id2 = (i32)g_buteTree.Find(s_actKeyB);
    if (id2 == 0) {
        g_buteTree.Insert(s_actKeyB, (void*)g_nextActId);
        id2 = g_nextActId;
        char* slot = ActNameLookup(id2);
        FreeNameList();
        ((CActName*)slot)->Assign(s_actKeyB);
        g_nextActId++;
    }
    *(void**)g_actReg_6514d8.ResolveEntry(id2) = (void*)&Handler_402dd8;
}

// ===========================================================================
// CTeleporter::RegisterActs @0x041680 - bind handler "A" (0x40187a) and handler
// "B" (0x403846) into CTeleporter's activation registry (g_teleporterActReg
// @0x6446b0; built by CTeleporter::InitActReg @0x414a0).
// ===========================================================================
// @early-stop
// A/B inline asymmetry + register-pinning wall (see file header).
RVA(0x00041680, 0x2ac)
void CTeleporter_RegisterActs() {
    i32 id = (i32)g_buteTree.Find(s_actKeyA);
    if (id == 0) {
        g_buteTree.Insert(s_actKeyA, (void*)g_nextActId);
        id = g_nextActId;
        char* slot = ActNameLookup(id);
        FreeNameList();
        ((CActName*)slot)->Assign(s_actKeyA);
        g_nextActId++;
    }
    *(void**)g_teleporterActReg.ResolveEntry(id) = (void*)&Handler_40187a;

    i32 id2 = (i32)g_buteTree.Find(s_actKeyB);
    if (id2 == 0) {
        g_buteTree.Insert(s_actKeyB, (void*)g_nextActId);
        id2 = g_nextActId;
        char* slot = ActNameLookup(id2);
        FreeNameList();
        ((CActName*)slot)->Assign(s_actKeyB);
        g_nextActId++;
    }
    *(void**)g_teleporterActReg.ResolveEntry(id2) = (void*)&Handler_403846;
}

// ===========================================================================
// RegisterActs_644af0 @0x05be30 - the multi-action registrar: interns 19 distinct
// activation keys into the shared bute store + name registry and binds each to its
// handler in the per-class registry @0x644af0 (proximity owner: CGrunt). Same
// per-key block as the 2-handler registrars above, unrolled 19x; with this many
// call sites MSVC outlines both lookups to the shared 0x3864 helper (the whole
// fast-range + slow-rebuild function), so the per-block lookups are calls here.
// ===========================================================================
// With 19 call sites MSVC outlines the whole [lo,hi]-range + slow-rebuild lookup
// to the shared 0x3864 helper - a __thiscall method on the collection (ecx=coll,
// push id, returns slot). CLookupColl is the shared <Gruntz/ActReg.h> CActReg-derived
// alias; its Lookup emits `mov ecx,coll; push id; call` (reloc-masked); inlined at
// the small-registrar sites above, it folds to the same global field reads.
DATA(0x002bf650)
extern CLookupColl g_nameRegColl; // 0x6bf650  (name registry, == g_nameReg)
DATA(0x00244af0)
extern CLookupColl g_reg_644af0; // 0x644af0  (per-class table)

// The 19 action-key strings (s_actKeyA/B come from above; the rest are .rdata
// string constants named by address). Referenced so the `push key` reloc-masks.
extern char k_60cca4[];
extern char k_60d2ec[];
extern char k_60cc94[];
extern char k_60d7f8[];

// The 19 per-action handler entries (ILT thunks), referenced by address.
extern "C" void H_402ac2();
extern "C" void H_4013cf();
extern "C" void H_402888();
extern "C" void H_402491();
extern "C" void H_403de6();
extern "C" void H_402211();
extern "C" void H_403bc5();
extern "C" void H_4040f2();
extern "C" void H_403e3b();
extern "C" void H_401005();
extern "C" void H_403edb();
extern "C" void H_40165e();
extern "C" void H_40321a();
extern "C" void H_4030f3();
extern "C" void H_403fe9();
extern "C" void H_403f21();
extern "C" void H_401195();
extern "C" void H_403e18();
extern "C" void H_4036f2();

// One unrolled per-key registration block (the shared archetype). A macro so all
// 19 expansions are emitted literally inline (an inline helper exhausts MSVC's
// per-function inline budget after ~6 sites and calls the rest, but retail unrolls
// every block); the lookups outline to the shared 0x3864 helper while the node-free
// loop stays inline.
#define REGISTER_KEY_644AF0(key, handler)                                                          \
    {                                                                                              \
        i32 id = (i32)g_buteTree.Find(key);                                                        \
        if (id == 0) {                                                                             \
            g_buteTree.Insert(key, (void*)g_nextActId);                                            \
            id = g_nextActId;                                                                      \
            char* slot = g_nameRegColl.Lookup(id);                                                 \
            i32 n = g_nameRegScratch;                                                              \
            void** list = g_nameRegCurList;                                                        \
            while (n-- != 0) {                                                                     \
                if (list != 0) {                                                                   \
                    ((CActName*)list)->Free();                                                     \
                }                                                                                  \
                list++;                                                                            \
            }                                                                                      \
            ((CActName*)slot)->Assign(key);                                                        \
            g_nextActId++;                                                                         \
        }                                                                                          \
        *(void**)g_reg_644af0.Lookup(id) = (void*)(handler);                                       \
    }

// @early-stop
// count-down free-loop induction wall (~94.7%): every Find/Insert/name-lookup/
// Assign/table-store + all 19 keys and handlers are byte-faithful, both lookups
// outline to the shared helper as retail does and all 19 blocks expand inline.
// Sole residual is the `ecx=cnt; eax=cnt-1; lea ebx,[eax+1]` node-free idiom + the
// slot-vs-id callee-saved coloring repeated per block. Not source-steerable.
RVA(0x0005be30, 0x9e5)
void RegisterActs_644af0() {
    REGISTER_KEY_644AF0(s_actKeyA, &H_402ac2);
    REGISTER_KEY_644AF0(s_actKeyB, &H_4013cf);
    REGISTER_KEY_644AF0(k_60cc90, &H_402888);
    REGISTER_KEY_644AF0(k_60cca4, &H_402491);
    REGISTER_KEY_644AF0(k_60d2ec, &H_403de6);
    REGISTER_KEY_644AF0(k_60d2e8, &H_402211);
    REGISTER_KEY_644AF0(k_60cc9c, &H_403bc5);
    REGISTER_KEY_644AF0(k_60d7fc, &H_4040f2);
    REGISTER_KEY_644AF0(k_60cca0, &H_403e3b);
    REGISTER_KEY_644AF0(k_60cc94, &H_401005);
    REGISTER_KEY_644AF0(k_60d7f8, &H_403edb);
    REGISTER_KEY_644AF0(k_60cc98, &H_40165e);
    REGISTER_KEY_644AF0(k_60d7f4, &H_40321a);
    REGISTER_KEY_644AF0(k_60dc04, &H_4030f3);
    REGISTER_KEY_644AF0(k_60dc0c, &H_403fe9);
    REGISTER_KEY_644AF0(k_60beb8, &H_403f21);
    REGISTER_KEY_644AF0(k_60dc08, &H_401195);
    REGISTER_KEY_644AF0(k_60bebc, &H_403e18);
    REGISTER_KEY_644AF0(k_60df94, &H_4036f2);
}
