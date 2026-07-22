#ifndef GRUNTZ_DDRAWMGR_DDRAWWORKERREGISTRY_H
#define GRUNTZ_DDRAWMGR_DDRAWWORKERREGISTRY_H

#include <Mfc.h>             // CObject / CMapStringToOb / POSITION / CString
#include <Gruntz/StateId.h>  // StateId (the slot-8 tag value STATE_WORKERREGISTRY)
#include <Gruntz/Loadable.h> // CLoadable : CWapObj : CObject (m_04/m_08/m_0c + reset dtor)
#include <Ints.h>
#include <rva.h>

class CDDrawWorker; // 0x6c-byte keyed worker (canonical def <DDrawMgr/DDrawWorker.h>);
class CDDrawWorker;             // CImageSet IS CDDrawWorker (<DDrawMgr/DDrawWorker.h>);
typedef CDDrawWorker CImageSet; // identical repeat of ImageSet.h's typedef - legal, and
class CImage; // the frame element (AnyValueMatches probes each set for it)
class CSymTab; // Bute/SymTab.h (the dir-tree cursor InstallTree/LoadNamespace walk)

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
    virtual void MapTeardown();         // [22] 0x1552b0 (destroy every map value)

    CMapStringToOb m_10map; // +0x10  the name -> worker/sprite hash table

    // Non-virtual map-scan helpers (direct-called from the worker code region).
    void DestroyAll();
    i32 RemoveKeysEqual(const char* base, const char* str);
    i32 SumSizesEqual(const char* str, i32 a2);
    i32 HasKeyEqual(const char* str);
    // Reverse frame lookup: scan every map value (a CImageSet) for `frame`; on a hit
    // copy the set name into outName + the frame index into outIndex (FindFrame).
    i32 AnyValueMatches(CImage* frame, char* outName, i32* outIndex);
    // FindKeyOfValue (0x165360) moved to CDDrawWorkerCache (its true owner: the
    // only callers reverse-look-up a worker in the +0x14 worker cache, xref-confirmed).
    // 0x155630 (frame-name reverse lookup; reloc-masked direct call).
    void ReadField(i32 handle, char* tmp, i32* outZero);
};
SIZE_UNKNOWN();


#endif // GRUNTZ_DDRAWMGR_DDRAWWORKERREGISTRY_H
