#ifndef GRUNTZ_DDRAWMGR_DDRAWWORKERREGISTRY_H
#define GRUNTZ_DDRAWMGR_DDRAWWORKERREGISTRY_H

// DDrawWorkerRegistry.h - THE canonical shape of CDDrawWorkerRegistry, a tomalla-named
// CDirectDrawMgr surface/page sub-manager (the "DDraw surface manager" family; see
// docs/ddraw-family-names.md; sibling of CDDrawSubMgrLeaf / CDDrawSubMgrLeafScan). Owns
// a CMapStringToOb at +0x10 (m_map) keyed by const char* worker keys; the map values
// are the keyed CDDrawWorker objects (FindOrCreateWorker is the only insert path).
//
// REAL POLYMORPHIC (2026-07-14): the 23-slot vtable ??_7CDDrawWorkerRegistry @0x1efd28
// is cl-emitted from this one class. The old three-way split - a non-polymorphic
// method-holder + the CWorkerVtableView dispatch view + the DDrawSubMgr.cpp-local
// CDDrawRegistryDtorHost dtor facet - is FOLDED: the dtor (??1 @0x156e10 in
// DDrawSubMgr.cpp, the vtable-emitting ctor/dtor boundary; the retail body stamps this
// very 0x5efd28) proves the identity, and the retail caller graph proves the all-virtual
// shape: EVERY slot body's ONLY .text reference is its vtable slot (zero direct rel32
// callers; the lone direct call to Unload @0x154ac0 is the dtor's own devirtualized
// call). Base is the canonical CLoadable (slots 5-8 = IsLoaded/IsReady/Unload/GetClassId
// scheme; CLoadable's inline dtor supplies the retail post-member-teardown
// m_04/-1 m_08/0 m_0c/0 resets).
//
// Slot 8 note: the family's "GetStateId" and CLoadable's "GetClassId" are the SAME
// slot-8 virtual over ONE tag space (STATE_* 0x10-0x14 and CLASSID_* are disjoint
// ranges of the same id space); as a CLoadable override it must carry the base name.
//
// Field names are placeholders; only OFFSETS + emitted code bytes are load-bearing.

#include <Mfc.h>             // CObject / CMapStringToOb / POSITION / CString
#include <Gruntz/StateId.h>  // StateId (the slot-8 tag value STATE_WORKERREGISTRY)
#include <Gruntz/Loadable.h> // CLoadable : CWapObj : CObject (m_04/m_08/m_0c + reset dtor)
#include <Ints.h>
#include <rva.h>

class CDDrawWorker; // 0x6c-byte keyed worker (canonical def <DDrawMgr/DDrawWorker.h>);
                    // class-key MUST match the definition (mangling: PAV vs PAU)
                    // is load-bearing: it keeps the PAU (not PAV) tag in the Forward*/
                    // RemoveWorker method manglings that take CDDrawWorker*.
class CDDrawWorker;             // CImageSet IS CDDrawWorker (<DDrawMgr/DDrawWorker.h>);
typedef CDDrawWorker CImageSet; // identical repeat of ImageSet.h's typedef - legal, and
                                // keeps this header pointer-only/include-light.
class CImage; // the frame element (AnyValueMatches probes each set for it)
class CSymTab; // Bute/SymTab.h (the dir-tree cursor InstallTree/LoadNamespace walk)

// ---------------------------------------------------------------------------
// CDDrawWorkerRegistry - the one polymorphic registry class. Retail vtable
// ??_7CDDrawWorkerRegistry @0x1efd28 (23 slots): [0..4] CObject, [5..8] the
// CLoadable-scheme overrides, [9..22] own virtuals in declaration order.
// InstallTree/LoadNamespace take `void*` trees: the call sites hand them the
// untyped handles of CSymTab::FindSub/ResolvePath (heterogeneous by API shape);
// the walker bodies read them as CSymTab scopes.
// ---------------------------------------------------------------------------
class CDDrawWorkerRegistry : public CLoadable {
public:
    // [1] ??1 @0x156e10 (DDrawSubMgr.cpp - the family dtor pocket; cl emits the
    // ??_G scalar-deleting dtor @0x156df0 + this vtable there).
    virtual ~CDDrawWorkerRegistry() OVERRIDE;
    virtual i32 IsLoaded() OVERRIDE;   // [ 5] 0x156dc0 (Ghidra size-0 gap; declared-only)
    virtual i32 IsReady() OVERRIDE;    // [ 6] 0x154aa0 (re-seeds the 25-dword blt-fx scratch)
    virtual i32 Unload() OVERRIDE;     // [ 7] 0x154ac0 (self-dispatch MapTeardown + clear flags)
    virtual i32 GetClassId() OVERRIDE; // [ 8] 0x156de0 (STATE_WORKERREGISTRY = 0x12)
    virtual i32 DispatchKeyed2C(i32 a1, i32 a2, const char* key, i32 a4, i32 a5); // [ 9] 0x154df0
    virtual i32 Forward2C(i32 a1, i32 a2, CDDrawWorker* worker, i32 a4, i32 a5);  // [10] 0x154f60
    virtual i32 Forward30(i32 a1, i32 a2, CDDrawWorker* worker, i32 a4, i32 a5);  // [11] 0x154f40
    virtual i32 DispatchKeyed30(i32 a1, i32 a2, const char* key, i32 a4, i32 a5); // [12] 0x154ce0
    virtual i32 Forward38(void* rec, CDDrawWorker* worker, i32 a3, i32 a4);       // [13] 0x154f20
    virtual i32 DispatchKeyed38(void* rec, const char* key, i32 a3, i32 a4);      // [14] 0x154ae0
    virtual i32 Forward34(i32 a1, CDDrawWorker* worker, i32 a3, i32 a4);          // [15] 0x154f00
    virtual i32 DispatchKeyed34(i32 a1, const char* key, i32 a3, i32 a4);         // [16] 0x154be0
    // [17] 0x156e80 (DDrawSubMgr.cpp) - probe a resolved sub-key, install its tree.
    virtual i32 ProbeWorkerKey(CSymTab* arg1, i32 arg2);
    // [18] 0x154f80 - install a resolved symbol TREE under a (name, separator) prefix;
    // recurses over child scopes through THIS slot (virtual self-dispatch). The
    // ex-CImageRegistry/ObjImageRegistry "Install"/"LoadTree"/"InsertWorkerKey" names
    // all denoted this one slot.
    virtual i32 InstallTree(void* tree, const char* szName, const char* szKey);
    // [19] 0x155160 - the read-side twin: validate a resolved namespace under a prefix;
    // returns -1 on failure (the RESOURCE-facet op the game-state activators reach).
    virtual i32 LoadNamespace(void* tree, const char* szName, const char* szKey);
    // [20] 0x155280 - drop a worker from the registry by its +0x24 key and destroy it.
    virtual void RemoveWorker(CDDrawWorker* worker);
    virtual void RemoveByKey(const char* key); // [21] 0x156ec0 (DDrawSubMgr.cpp)
    virtual void MapTeardown_1552b0();         // [22] 0x1552b0 (destroy every map value)

    CMapStringToOb m_10map; // +0x10  the name -> worker/sprite hash table

    // Non-virtual map-scan helpers (direct-called from the worker code region).
    void DestroyAll();
    i32 RemoveKeysEqual_155360(const char* base, const char* str);
    i32 SumSizesEqual_155460(const char* str, i32 a2);
    i32 HasKeyEqual_155550(const char* str);
    // Reverse frame lookup: scan every map value (a CImageSet) for `frame`; on a hit
    // copy the set name into outName + the frame index into outIndex (FindFrame).
    i32 AnyValueMatches_155630(CImage* frame, char* outName, i32* outIndex);
    // FindKeyOfValue_165360 (0x165360) moved to CDDrawWorkerCache (its true owner: the
    // only callers reverse-look-up a worker in the +0x14 worker cache, xref-confirmed).
    // 0x155630 (frame-name reverse lookup; reloc-masked direct call).
    void ReadField(i32 handle, char* tmp, i32* outZero);
};

SIZE_UNKNOWN(CDDrawWorkerRegistry);
VTBL(CDDrawWorkerRegistry, 0x001efd28); // ??_7CDDrawWorkerRegistry@@6B@ (23 slots)

#endif // GRUNTZ_DDRAWMGR_DDRAWWORKERREGISTRY_H
