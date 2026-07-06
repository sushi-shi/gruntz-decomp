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

struct CDDrawWorker; // 0x6c-byte keyed worker (defined in the owning .cpp); struct-key
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
class CWorkerVtableView {
public:
    virtual void Slot00_1bef01();                         // [ 0] +0x00 0x1bef01 (CObject thunk)
    virtual void ScalarDtor_156df0();                     // [ 1] +0x04 0x156df0
    virtual void Slot08_28ec();                           // [ 2] +0x08 0x0028ec (CObject thunk)
    virtual void Slot0C_106e();                           // [ 3] +0x0c 0x00106e (CObject thunk)
    virtual void Slot10_4034();                           // [ 4] +0x10 0x004034 (CObject thunk)
    virtual void Slot14_156dc0();                         // [ 5] +0x14 0x156dc0
    virtual void ResetScratch_154aa0();                   // [ 6] +0x18 0x154aa0
    virtual void Shutdown_154ac0();                       // [ 7] +0x1c 0x154ac0
    virtual void GetStateId_156de0();                     // [ 8] +0x20 0x156de0
    virtual void DispatchKeyed2C_154df0();                // [ 9] +0x24 0x154df0
    virtual void Forward2C_154f60();                      // [10] +0x28 0x154f60
    virtual void Forward30_154f40();                      // [11] +0x2c 0x154f40
    virtual void DispatchKeyed30_154ce0();                // [12] +0x30 0x154ce0
    virtual void Forward38_154f20();                      // [13] +0x34 0x154f20
    virtual void DispatchKeyed38_154ae0();                // [14] +0x38 0x154ae0
    virtual void Forward34_154f00();                      // [15] +0x3c 0x154f00
    virtual void DispatchKeyed34_154be0();                // [16] +0x40 0x154be0
    virtual void Probe_156e80();                          // [17] +0x44 0x156e80
    virtual i32 Vfunc48(void* a, const char* b, void* c); // [18] +0x48 0x154f80 (tree LoadTree)
    virtual i32 Vfunc4C(void* a, const char* b, void* c); // [19] +0x4c 0x155160
    virtual void RemoveWorker_155280();                   // [20] +0x50 0x155280
    virtual void Vfunc54(const char* key);                // [21] +0x54 0x156ec0 (RemoveByKey)
    virtual void MapTeardown_1552b0();                    // [22] +0x58 0x1552b0
};

// ---------------------------------------------------------------------------
// CDDrawWorkerRegistry - only the load-bearing offset is modeled: the CMapStringToOb at
// +0x10. The matched methods occupy vtable slots (slot number not load-bearing, only body).
// ---------------------------------------------------------------------------
class CDDrawWorkerRegistry {
public:
    i32 IsReady();
    i32 ResetScratch();
    void Shutdown();
    StateId GetStateId();
    i32 DispatchKeyed2C(i32 a1, i32 a2, const char* key, i32 a4, i32 a5);
    i32 Forward2C(i32 a1, i32 a2, CDDrawWorker* worker, i32 a4, i32 a5);
    i32 Forward30(i32 a1, i32 a2, CDDrawWorker* worker, i32 a4, i32 a5);
    i32 DispatchKeyed30(i32 a1, i32 a2, const char* key, i32 a4, i32 a5);
    i32 Forward38(i32 a1, CDDrawWorker* worker, i32 a3, i32 a4);
    i32 DispatchKeyed38(i32 a1, const char* key, i32 a3, i32 a4);
    i32 Forward34(i32 a1, CDDrawWorker* worker, i32 a3, i32 a4);
    i32 DispatchKeyed34(i32 a1, const char* key, i32 a3, i32 a4);
    void RemoveWorker(CDDrawWorker* worker);
    void RemoveByKey(const char* key);
    void DestroyAll();
    void MapTeardown_1552b0();
    i32 StringCopy_155810(const char* src);

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
    void* RegScalarDtor(i32 flag);
    i32 ProbeWorkerKey(CSymTab* a1, i32 a2);
};

SIZE_UNKNOWN(CWorkerVtableView);

// --- vtable catalog (reduced-view classes share their base vtable rva) ---

#endif // GRUNTZ_DDRAWMGR_DDRAWWORKERREGISTRY_H
