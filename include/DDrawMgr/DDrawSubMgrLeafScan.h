#ifndef GRUNTZ_DDRAWMGR_CDDRAWSUBMGRLEAFSCAN_H
#define GRUNTZ_DDRAWMGR_CDDRAWSUBMGRLEAFSCAN_H

#include <Mfc.h> // real MFC CObject / CMapStringToPtr / CString / POSITION
#include <Ints.h>
#include <rva.h>
#include <Gruntz/Loadable.h> // CLoadable : CWapObj : CObject - the real grand-base

struct SoundStream;  // +0x2c held DSound stream (SoundStream : SoundDevice; Play stops it)
struct LeafCue;      // the 0x1c cache element / map value (<Gruntz/LeafCue.h>)
class CSymTab;       // <Bute/SymTab.h> - the scope node ScanTree walks (was the DirNode view)
struct CParseSource; // the element's draw-source (BeginParse/EndParse; STRUCT key = the def)

// LeafScanBase DISSOLVED: it was a duplicate view of CLoadable (identical m_04/-1
// +0x08/0 m_0c/0 header + field-reset dtor + the SAME CDDrawSurfaceMgr OwnerMgr()
// accessor); slot-6 IsReady is CWapObj's default, slot-8 GetClassId is CLoadable's.
// The scan sub-manager now derives from the real CLoadable.

class CDDrawSubMgrLeafScan : public CLoadable {
public:
    // 9-slot vtable (??_7CDDrawSubMgrLeafScan @0x5efca0): the CObject slots + slots 5/7
    // overriding CLoadable's, slot 6 IsReady INHERITED from CWapObj, slot 8 GetClassId
    // INHERITED from CLoadable. Slot 5/7 bodies live in the sibling CDDrawSubMgrLeaf TU.
    RVA(0x00157530, 0x17)
    virtual i32 IsLoaded() OVERRIDE { // [5] overrides CLoadable::IsLoaded (the worker-gate)
        // retail: both-zero falls through to `return 0`, either gate `jne` to a shared
        // out-of-line `return 1` at the tail (the &&-guard shape, not a setne fold).
        if (m_2c == 0 && m_emitGate == 0) {
            return 0;
        }
        return 1;
    }
    // slot 6 IsReady INHERITED from CWapObj (its `return 1` default @0xd5da0); not
    // redeclared (the old "IsValidImage" was a phantom own-decl of that shared body).
    virtual i32
    Unload() OVERRIDE; // [7] 0x157ae0 (overrides CLoadable::Unload; clears the map; def in DDrawSubMgr.cpp)
    // slot 8 GetClassId INHERITED from CLoadable (@0x154a00 -> CLASSID_NONE); not
    // redeclared (the old "GetTypeId" was a phantom own-decl of that shared body).

    i32 RefreshAsset_114120(const char* key);
    LeafCue* CreateEntry_157d70(const char* key, void* arg2);
    LeafCue* CreateEntry2_157e00(const char* key, void* arg2);
    LeafCue* AddFromSource_157e90(CParseSource* src);     // 0x157e90
    void AddEntry_157ec0(LeafCue* elem, const char* key); // 0x157ec0
    // The recursive asset-tree walker. `tree` is a Bute CSymTab scope: the walker calls
    // FirstSub/NextSub (child scopes) and FirstSym/NextSym/NextSym2/NextSym3 (leaf parse
    // records) on it, and every subdir it recurses into is itself a CSymTab. The leaf
    // records it tags-and-caches are CParseSource. (Both were formerly one `DirNode` view.)
    i32 ScanTree_157ee0(CSymTab* tree, const char* prefix, const char* suffix);

    CObject* Lookup_05b7e0(const char* key);
    i32 RemoveKeysEqual_157c70(const char* base, const char* str);
    i32 SumField_1580b0(const char* str);
    LeafCue* GetFirstValue_158210();
    LeafCue* NextValueAfter_1582c0(LeafCue* target);
    i32 ProbeFirst_1584a0(i32 arg);
    i32 HasKeyEqual_1583c0(const char* str);
    CString FindKeyOfValue_158570(LeafCue* target);
    i32 MatchSub_1584f0(LeafCue* arg1, i32 arg2);

    // These two landed in the SIBLING CDDrawSubMgrLeaf.cpp (name-preserving union):
    i32 ClearMap(); // 0x157bc0 (non-virtual map teardown; i32 residue feeds Unload)
    // Remove one m_10 entry by VALUE, destroying it (0x157b00, DDrawSubMgr.cpp; ex
    // the CSoundResMap/CSoundRes view pair - the values are LeafCue elements).
    void RemoveByValue(struct LeafCue* p);

    virtual ~CDDrawSubMgrLeafScan() OVERRIDE; // overrides slot [1]
    // The `??_G` scalar-deleting destructor (vtable slot 1 @0x157550): run the real
    // ~CDDrawSubMgrLeafScan (direct call), conditionally RezFree, return this. Hand-written
    // non-virtual + RVA pin (the CFileImageSurface::ScalarDelete pattern) so the body emits.
    void* ScalarDtor(u32 flags); // 0x157550

    // 0x1581b0: fire the named CAniBlitTrigger held in the cache, gated on the parent
    // (m_0c) being live and this sub-manager not busy (m_30 == 0). It has NO caller in
    // the retail image (dead / inlined away), which is why it was long parked under the
    // placeholder class name `CAniTriggerMap_1581b0`; the BYTES name it though - it reads
    // m_0c/+0x30 and Lookups m_10 (`add ecx,0x10; call 0x1b8438`), and it sits between
    // this class's RemoveKeysEqual_157c70 and GetFirstValue_158210 (which begins at
    // 0x158210, exactly where this body ends).
    i32 Fire_1581b0(const char* key, i32 pos, i32 range1, i32 range2); // 0x1581b0

    CMapStringToPtr m_10; // +0x10  keyed asset cache (ends +0x2c)
    SoundStream* m_2c;    // +0x2c  held DSound stream (game TUs Stop() it on teardown)
    // +0x30: one field, two established readings (same semantics - nonzero = busy /
    // not ready): the DDrawSubMgr family's "busy/loading guard" (m_30) and the game
    // TUs' "live-surface/emit gate" (m_emitGate; must be 0 to emit). Anonymous union
    // so both spellings bind the one field.
    i32 m_emitGate; // +0x30  live-surface/emit gate (must be 0 to emit; ex "m_30")
    i32 m_34; // +0x34  redraw arg
};

SIZE_UNKNOWN(CDDrawSubMgrLeafScan);

VTBL(
    CDDrawSubMgrLeafScan,
    0x001efca0
); // ??_7CDDrawSubMgrLeafScan@@6B@ (9-slot LeafScanBase-derived)

#endif // GRUNTZ_DDRAWMGR_CDDRAWSUBMGRLEAFSCAN_H
