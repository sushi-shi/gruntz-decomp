#ifndef GRUNTZ_DDRAWMGR_DDRAWWORKERREGISTRY_H
#define GRUNTZ_DDRAWMGR_DDRAWWORKERREGISTRY_H

// DDrawWorkerRegistry.h - THE canonical shape of CDDrawWorkerRegistry, a tomalla-named
// CDirectDrawMgr surface/page sub-manager (the "DDraw surface manager" family; see
// docs/ddraw-family-names.md; sibling of CDDrawSubMgrLeaf / CDDrawSubMgrLeafScan). Owns
// a CMapStringToOb at +0x10 (m_map) keyed by const char* worker keys.
//
// NOT modeled as a real polymorphic class: its own 23-slot vtable (??_7 @0x5efd28)
// cannot be cl-emitted here - there is no ctor/dtor in the owning TU (ctor-boundary
// rule: a g_*Vtbl stamp realizes to a real ??_7 ONLY inside a real ctor/dtor body), and
// the keyed slot bodies interleave with the sibling manager-family vtables. Virtual
// dispatch on `this` therefore goes through CWorkerVtableView - a typed self-view of
// vtable 0x5efd28, the SINGLE unified device that folds together the two former per-TU
// views: the 23-slot vtable view + RegView48 (which only reached slots 0x48/0x4c/0x54).
// Verified: the 23 slot targets read out of retail 0x5efd28 exactly match this layout.
//
// Field names are placeholders; only OFFSETS + emitted code bytes are load-bearing.

#include <Mfc.h>            // CObject / CByteArray / CMapStringToOb / POSITION / CString
#include <Gruntz/StateId.h> // StateId (GetStateId return)
#include <Ints.h>
#include <rva.h>

class CDDrawWorker; // 0x6c-byte keyed worker (canonical def <DDrawMgr/DDrawWorker.h>);
                    // class-key MUST match the definition (mangling: PAV vs PAU)
                    // is load-bearing: it keeps the PAU (not PAV) tag in the Forward*/
                    // RemoveWorker method manglings that take CDDrawWorker*.
class CImageSet;
class CWorkerMapValue; // a map value viewed for +0x10 + probe (foreign, other TU)
class CSymTab;         // Bute/SymTab.h (the dir-tree cursor)
class CSymTab;         // probe chain (foreign, other TU)

// ---------------------------------------------------------------------------
// The registry's own 23-slot vtable (??_7CDDrawWorkerRegistry @0x5efd28), each slot named
// from its retail slot-function RVA + role. Dispatched-through slots (0x48/0x4c/0x54/0x58)
// carry their call-site signatures; the rest are 0-arg placeholders (never dispatched via
// the view, only reached by direct name-mangled calls to the registry's own methods).
// Unifies the two former per-TU views (the 23-slot vtable view + RegView48).
// ---------------------------------------------------------------------------
// The looked-up map value as torn down by the registry method set: only the
// scalar-deleting destructor slot (+0x04) is load-bearing. Declarations only -
// never defined, so no ??_7 is emitted (shared by the E/G/T method hosts).
class CWorkerValue {
public:
    virtual void GetRuntimeClass(); // [0] 0x1bef01 (shared thunk, declared-only)
    virtual ~CWorkerValue();        // slot 1 (deleting dtor -> cl-emitted ??_G)
};
SIZE_UNKNOWN(CWorkerValue);

class CWorkerVtableView : public CObject {
public:
    virtual ~CWorkerVtableView() OVERRIDE; // slot 1; slots 0/2/3/4 inherited from CObject
    virtual void Slot14_156dc0();          // [ 5] +0x14 0x156dc0
    virtual void ResetScratch_154aa0();    // [ 6] +0x18 0x154aa0
    virtual void Shutdown_154ac0();        // [ 7] +0x1c 0x154ac0
    virtual void GetStateId_156de0();      // [ 8] +0x20 0x156de0
    virtual void DispatchKeyed2C_154df0(); // [ 9] +0x24 0x154df0
    virtual void Forward2C_154f60();       // [10] +0x28 0x154f60
    virtual void Forward30_154f40();       // [11] +0x2c 0x154f40
    virtual void DispatchKeyed30_154ce0(); // [12] +0x30 0x154ce0
    virtual void Forward38_154f20();       // [13] +0x34 0x154f20
    virtual void DispatchKeyed38_154ae0(); // [14] +0x38 0x154ae0
    virtual void Forward34_154f00();       // [15] +0x3c 0x154f00
    virtual void DispatchKeyed34_154be0(); // [16] +0x40 0x154be0
    virtual void Probe_156e80();           // [17] +0x44 0x156e80
    // [18] +0x48 0x154f80 - install a resolved symbol TREE under a (name, separator)
    // prefix. The ex-CImageRegistry/ObjImageRegistry "Install" and the
    // ex-CDDrawWorkerRegistry "LoadTree" are this ONE slot (the const char*/char* and
    // void*/const char* arg spellings of the three views unify as the superset below).
    virtual i32 InstallTree(void* tree, const char* szName, const char* szKey);
    // [19] +0x4c 0x155160 - register a resolved namespace under a prefix; returns -1 on
    // failure (the RESOURCE-facet op the game-state activators reach: CBootyState /
    // CMenuState / CPlay slot-8 loaders, StateImages' m_c->m_10->LoadNamespace).
    virtual i32 LoadNamespace(void* tree, const char* szName, const char* szKey);
    // [20] +0x50 0x155280 - drop a worker/value from the registry (the ex-ObjImageRegistry
    // "ProcessNew(void*)" call site hands it the raw map element).
    virtual void RemoveWorker(void* value);
    virtual void Vfunc54(const char* key); // [21] +0x54 0x156ec0 (RemoveByKey)
    virtual void MapTeardown_1552b0();     // [22] +0x58 0x1552b0

    // --- layout (the union of the three ex-views; all agreed) ---------------------
    // vptr occupies +0x00..+0x03.
    char m_pad04[0x10 - 0x4];
    CMapStringToOb m_10map; // +0x10  the name -> sprite/worker hash table

    // Non-virtual entries the registry's own method set exposes (reloc-masked; the
    // ex-views reached these by direct call, not through a slot).
    void ReadField(i32 handle, char* tmp, i32* outZero); // 0x155630 (frame-name reverse lookup)
};

// ---------------------------------------------------------------------------
// CDDrawWorkerRegistry - only the load-bearing offset is modeled: the CMapStringToOb at
// +0x10. The matched methods occupy vtable slots (slot number not load-bearing, only body).
// ---------------------------------------------------------------------------
SIZE_UNKNOWN(CDDrawWorkerRegistry); // (the annotation the dissolved MgrSettings.h twin carried)
class CDDrawWorkerRegistry {
public:
    // IsReady (0x1576d0, slot 5) now homes on CDDrawWorkerCache's real vtable slot
    // (constructed there so cl+clang emit it); dropped from this non-instantiated body view.
    i32 ResetScratch();
    void Shutdown();
    // GetStateId (0x156de0, slot 8): CWorkerVtableView is never instantiated so its
    // vtable never emits; kept here as the non-virtual body (still unmatched - deferred).
    StateId GetStateId(); // 0x156de0 (out-of-line)
    i32 DispatchKeyed2C(i32 a1, i32 a2, const char* key, i32 a4, i32 a5);
    i32 Forward2C(i32 a1, i32 a2, CDDrawWorker* worker, i32 a4, i32 a5);
    i32 Forward30(i32 a1, i32 a2, CDDrawWorker* worker, i32 a4, i32 a5);
    i32 DispatchKeyed30(i32 a1, i32 a2, const char* key, i32 a4, i32 a5);
    i32 Forward38(void* rec, CDDrawWorker* worker, i32 a3, i32 a4);
    i32 DispatchKeyed38(void* rec, const char* key, i32 a3, i32 a4);
    i32 Forward34(i32 a1, CDDrawWorker* worker, i32 a3, i32 a4);
    i32 DispatchKeyed34(i32 a1, const char* key, i32 a3, i32 a4);
    void RemoveWorker(CDDrawWorker* worker);
    void RemoveByKey(const char* key);
    void DestroyAll();
    void MapTeardown_1552b0();

    // Map-scan helpers (non-virtual; direct-called from the worker code region).
    i32 RemoveKeysEqual_155360(const char* base, const char* str);
    i32 SumSizesEqual_155460(const char* str, i32 a2);
    i32 HasKeyEqual_155550(const char* str);
    i32 AnyValueMatches_155630(i32 a1, i32 a2, i32 a3);
    CString FindKeyOfValue_165360(CImageSet* target);

    char _vft0[4];             // +0x00 foreign object vptr (reduced view; not owned/dispatched)
    i32 m_status;              // +0x04  initialized to -1 when inactive
    char m_pad08[0x0c - 0x08]; // +0x08..0x0b
    i32 m_0c;                  // +0x0c  parent/root handle
    CMapStringToOb m_map;      // +0x10  worker-by-key map

    // Engine-label backlog stubs.
    i32 InsertWorkerKey(CSymTab* dir, const char* sub, const char* prefix);
    i32 LookupWorkerKey(CSymTab* dir, const char* sub, const char* prefix);
    i32 ProbeWorkerKey(CSymTab* a1, i32 a2);
};

SIZE_UNKNOWN(CWorkerVtableView);
// The 23-slot registry vtable (0x5efd28). Its ??_G/dtor is stamped in the dtor-facet
// CDDrawRegistryDtorHost (DDrawSubMgr.cpp) - a compiler-model wall (the vtable realizes
// only in a real ctor/dtor, which lives on the dtor-facet, not this method-holder), so
// this real 23-virtual class is the load-bearing dispatch model (ProbeWorkerKey casts
// `this` to it to force the exact indirect [vtbl+0x48] dispatch retail uses).
VTBL(CWorkerVtableView, 0x001efd28);

// --- vtable catalog (reduced-view classes share their base vtable rva) ---

#endif // GRUNTZ_DDRAWMGR_DDRAWWORKERREGISTRY_H
