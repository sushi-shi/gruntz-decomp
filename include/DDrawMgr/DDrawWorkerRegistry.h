#ifndef GRUNTZ_DDRAWMGR_DDRAWWORKERREGISTRY_H
#define GRUNTZ_DDRAWMGR_DDRAWWORKERREGISTRY_H

#include <Mfc.h>             // CObject / CMapStringToOb / POSITION / CString
#include <Gruntz/StateId.h>  // StateId (the slot-8 tag value STATE_WORKERREGISTRY)
#include <Gruntz/Loadable.h> // CLoadable : CWapObj : CObject (m_04/m_08/m_0c + reset dtor)
#include <Ints.h>
#include <rva.h>

class CDDrawWorker; // 0x6c-byte keyed worker (canonical def <DDrawMgr/DDrawWorker.h>);
class CDDrawWorker; // CDDrawWorker IS CDDrawWorker (<DDrawMgr/DDrawWorker.h>);

class CImage;     // the frame element (AnyValueMatches probes each set for it)
struct PidHeader; // the descriptor the create/dispatch slots take
class CSymTab;    // Bute/SymTab.h (the dir-tree cursor InstallTree/LoadNamespace walk)

class CDDrawWorkerRegistry : public CLoadable {
public:
    // INLINE ctor - expanded in place by CDDrawSurfaceMgr::Init @0x155a37: the 1-arg
    // CLoadable base (vptr + the three header stores spelled inline), the +0x10 map
    // member ctor, then the ??_7 stamp 0x5efd28.
    CDDrawWorkerRegistry(CDDrawSurfaceMgr* owner) : CLoadable(owner) {}
    // [1] ??1 @0x156e10 (DDrawSubMgr.cpp - the family dtor pocket; cl emits the
    // ??_G scalar-deleting dtor @0x156df0 + this vtable there).
    virtual ~CDDrawWorkerRegistry() OVERRIDE;
    virtual i32 IsLoaded() OVERRIDE;   // [ 5] 0x156dc0 (Ghidra size-0 gap; declared-only)
    virtual i32 IsReady() OVERRIDE;    // [ 6] 0x154aa0 (re-seeds the 25-dword blt-fx scratch)
    virtual void Unload() OVERRIDE;    // [ 7] 0x154ac0 (self-dispatch MapTeardown + clear flags)
    virtual i32 GetClassId() OVERRIDE; // [ 8] 0x156de0 (CLASSID_WORKERREGISTRY = 0x12)
    // Slots 9-16 are four PAIRS (find-the-worker-by-key / take-the-worker-directly)
    // over the same four CDDrawWorker create slots, and each pair forwards its leading
    // args verbatim. Their types are therefore the worker slot's - see the proof block
    // over CDDrawWorker::CreateFrame24/28/30 in <DDrawMgr/DDrawWorker.h>
    // (SETTLED 2026-07-27): 2C -> CreateFrame24 (width,height), 30 -> CreateFrame28
    // (desc,mode,...,size), 34 -> CreateFrame30 (path), 38 -> InsertFrame (record).
    virtual CImage*
    DispatchKeyed2C(i32 width, i32 height, const char* key, i32 index, i32 keyed); // [ 9] 0x154df0
    virtual CImage*
    Forward2C(i32 width, i32 height, CDDrawWorker* worker, i32 index, i32 keyed); // [10] 0x154f60
    // [11] 0x154f40
    virtual CImage* Forward30(PidHeader* desc, i32 mode, CDDrawWorker* worker, i32 index, u32 size);
    // [12] 0x154ce0
    virtual CImage*
    DispatchKeyed30(PidHeader* desc, i32 mode, const char* key, i32 index, u32 size);
    // (index, mode) are CDDrawWorker::InsertFrame's own trailing slots: its body
    // @0x151f00 subscripts m_items with the index and hands the mode to CImage::Resolve.
    virtual CImage* Forward38(void* rec, CDDrawWorker* worker, i32 index, i32 mode);
    // [13] 0x154f20
    virtual CImage* DispatchKeyed38(void* rec, const char* key, i32 index, i32 mode);
    // [14] 0x154ae0
    virtual CImage*
    Forward34(char* path, CDDrawWorker* worker, i32 index, i32 keyed); // [15] 0x154f00
    virtual CImage*
    DispatchKeyed34(char* path, const char* key, i32 index, i32 keyed); // [16] 0x154be0
    // [17] 0x156e80 (DDrawSubMgr.cpp) - probe a resolved sub-key, install its tree.
    virtual i32 ProbeWorkerKey(class CSymParser* parser, const char* key);
    // [18] 0x154f80 - install a resolved symbol TREE under a (name, separator) prefix;
    // recurses over child scopes through THIS slot (virtual self-dispatch). The
    // ex-CDDrawWorkerRegistry/ObjImageRegistry "Install"/"LoadTree"/"InsertWorkerKey" names
    // all denoted this one slot.
    virtual i32 InstallTree(void* tree, const char* szName, const char* szKey);
    // [19] 0x155160 - the read-side twin: validate a resolved namespace under a prefix;
    // returns -1 on failure (the RESOURCE-facet op the game-state activators reach).
    virtual i32 LoadNamespace(void* tree, const char* szName, const char* szKey);
    // [20] 0x155280 - drop a worker from the registry by its +0x24 key and destroy it.
    virtual void RemoveWorker(CDDrawWorker* worker);
    virtual void RemoveByKey(const char* key); // [21] 0x156ec0 (DDrawSubMgr.cpp)
    virtual void MapTeardown();                // [22] 0x1552b0 (destroy every map value)

    CMapStringToOb m_10map; // +0x10  the name -> worker/sprite hash table

    // Non-virtual map-scan helpers (direct-called from the worker code region).
    // (DestroyAll @0x165210 moved to CDDrawWorkerCache - its true owner: the only
    //  call site is that class's dtor and the only data ref its vtable slot 7.)
    i32 RemoveKeysEqual(const char* base, const char* str);
    // `raw` is CDDrawWorker::GetMemoryUsage's own parameter - nonzero drops the
    // per-frame sizeof(CImage) overhead from each frame's decoded byte size.
    i32 SumSizesEqual(const char* str, i32 raw);
    i32 HasKeyEqual(const char* str);
    // Reverse frame lookup: scan every map value (a CDDrawWorker) for `frame`; on a hit
    // copy the set name into outName + the frame index into outIndex (FindFrame).
    i32 AnyValueMatches(CImage* frame, char* outName, i32* outIndex);
    // FindKeyOfValue (0x165360) moved to CDDrawWorkerCache (its true owner: the
    // only callers reverse-look-up a worker in the +0x14 worker cache, xref-confirmed).
    // 0x155630 (frame-name reverse lookup; reloc-masked direct call).
    void ReadField(i32 handle, char* tmp, i32* outZero);
};
SIZE_UNKNOWN();

#endif // GRUNTZ_DDRAWMGR_DDRAWWORKERREGISTRY_H
