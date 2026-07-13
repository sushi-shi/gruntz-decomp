#ifndef GRUNTZ_DDRAWMGR_CDDRAWSUBMGRLEAFSCAN_H
#define GRUNTZ_DDRAWMGR_CDDRAWSUBMGRLEAFSCAN_H

// DDrawSubMgrLeafScan.h - THE single-source shape of CDDrawSubMgrLeafScan, the keyed-
// asset CACHE variant of the tomalla-named CDDrawSubMgr surface-family sub-manager
// (sibling of CDDrawSubMgrLeaf / CDDrawWorkerRegistry). Own vtable ??_7 @0x5efca0; a
// CObject-like grand-base (LeafScanBase, vtable 0x5e8cb4) supplies the vptr + the three
// header fields; the keyed cache map sits at +0x10, a held DSound device at +0x2c, a
// busy/loading guard at +0x30 and a redraw arg at +0x34.
//
// This header is the union of the two former per-TU views (name-preserving):
//   - DDrawSubMgrLeafScan.cpp: the full method set + the ??_7/??1 (real dtor) emission
//   - DDrawSubMgrLeaf.cpp: the ??_G scalar-deleting dtor (0x157550, SYMBOL-pinned there)
//     + IsReady (0x157530) + ClearMap (0x157bc0) that landed in the SIBLING TU.
// The ??_7/??1 stay emitted ONLY in DDrawSubMgrLeafScan.cpp (the class's virtual dtor is
// declared-only here, so a consumer that does NOT define ~ emits no competing vtable);
// the ??_G/IsReady/ClearMap bodies stay in DDrawSubMgrLeaf.cpp. Field names are
// placeholders; only OFFSETS + emitted code bytes are load-bearing (campaign doctrine).

// THE +0x10 MAP IS CMapStringToPtr, NOT CMapStringToOb (mfc_class --audit, 2026-07-12):
// every map rva retail calls from these methods - Lookup 0x1b8438, RemoveKey 0x1b84de,
// GetNextAssoc 0x1b8546, ~map 0x1b8322 - lies in [0x1b8247, 0x1b85b1), the band whose
// ctor stamps ??_7CMapStringToPtr@@6B@ (0x1eb014).  CMapStringToOb's band is
// [0x1b7e17, 0x1b8247) (Lookup 0x1b8008) and NOTHING here enters it.  The two classes
// are byte-identical, so the FID rows are all AMBIG and the tree had guessed wrong;
// `python -m gruntz.analysis.mfc_class 0x1b8438` asks the binary.  (A sibling map -
// CDDrawWorkerRegistry::m_map - really IS CMapStringToOb, so this is per-site.)
#include <Mfc.h> // real MFC CObject / CMapStringToPtr / CString / POSITION
#include <Ints.h>
#include <rva.h>

// Body-only dependency types (defined in the owning .cpp; only pointers/values in the
// class declaration, so a forward decl suffices and keeps lean consumers lean).
class SoundDevice;      // +0x2c held DSound device (SoundStream : SoundDevice at the site)
struct LeafElementObj;  // 0x1c cache element (CreateEntry factory output)
class CSymTab;          // <Bute/SymTab.h> - the scope node ScanTree walks (was the DirNode view)
class LeafScanValue;    // a looked-up map value (scalar-dtor slot + held sound-arg)
class LeafScanSoundArg; // MatchSub arg (its m_10 is the real DirectSoundMgr wrapper)
class CParseSource;     // the element's draw-source (BeginParse/EndParse)

// ---------------------------------------------------------------------------
// The shared CObject-like grand-base: vptr + status word at +0x04 + handle at +0x0c.
// Modeled as a REAL polymorphic base (its 5-slot vtable is the shared grand-base
// g_wapObjectDtorVtbl @0x5e8cb4) so cl emits the implicit grand-base vptr re-stamp
// (masks 0x5e8cb4) at the derived dtor's tail - no manual `*(void**)this = &g_*Vtbl`.
// NAME-AUDIT: maps to RTTI CObject @0x1e8cb4, but KEPT as a real intermediate (carries
// the m_04/m_08/m_0c header past the bare vptr) - NOT a bare-CObject fold.
// ---------------------------------------------------------------------------
class LeafScanBase : public CObject {
public:
    virtual ~LeafScanBase()
        OVERRIDE; // [1] scalar-deleting dtor; slots 0/2/3/4 inherited from CObject

    i32 m_04;                  // +0x04  -1 when inactive
    char m_pad08[0x0c - 0x08]; // +0x08..0x0b
    i32 m_0c;                  // +0x0c  parent/root handle
    LeafScanBase() {}
};

inline LeafScanBase::~LeafScanBase() {
    m_04 = -1;
    *(i32*)&m_pad08[0] = 0; // +0x08 = 0
    m_0c = 0;
}

// ---------------------------------------------------------------------------
// The cache sub-manager. Map at +0x10 (CMapStringToPtr, 0x1c bytes -> ends at +0x2c).
// m_2c is the held DSound device, m_30 the busy guard, m_34 a redraw arg.
// ---------------------------------------------------------------------------
class CDDrawSubMgrLeafScan : public LeafScanBase {
public:
    // 9-slot vtable (??_7CDDrawSubMgrLeafScan @0x5efca0): 5 shared LeafScanBase slots
    // (slot 1 = the virtual dtor below), then 4 leaf virtuals at slots 5..8. Slots
    // 5/7 point to functions in the sibling CDDrawSubMgrLeaf TU (0x157530 / 0x157ae0);
    // 6/8 are unreconstructed -> declared-only, reloc-masked vtable references.
    RVA(0x00157530, 0x17)
    virtual i32 IsReady() {
        if (m_2c != 0) {
            return 1;
        }
        if (m_30 != 0) {
            return 1;
        }
        return 0;
    }
    virtual void IsValidImage(); // [6] 0x001c08 (shared thunk, declared-only)
    virtual void
    ClearContext(); // [7] 0x157ae0 (defined in DDrawSubMgr.cpp - this class's own slot)
    virtual void Slot08_154a00(); // [8] 0x154a00 (shared, declared-only)

    i32 RefreshAsset_114120(const char* key);
    LeafElementObj* CreateEntry_157d70(const char* key, void* arg2);
    LeafElementObj* CreateEntry2_157e00(const char* key, void* arg2);
    LeafElementObj* AddFromSource_157e90(CParseSource* src);     // 0x157e90
    void AddEntry_157ec0(LeafElementObj* elem, const char* key); // 0x157ec0
    // The recursive asset-tree walker. `tree` is a Bute CSymTab scope: the walker calls
    // FirstSub/NextSub (child scopes) and FirstSym/NextSym/NextSym2/NextSym3 (leaf parse
    // records) on it, and every subdir it recurses into is itself a CSymTab. The leaf
    // records it tags-and-caches are CParseSource. (Both were formerly one `DirNode` view.)
    i32 ScanTree_157ee0(CSymTab* tree, const char* prefix, const char* suffix);

    CObject* Lookup_05b7e0(const char* key);
    i32 RemoveKeysEqual_157c70(const char* base, const char* str);
    i32 SumField_1580b0(const char* str);
    LeafScanValue* GetFirstValue_158210();
    LeafScanValue* NextValueAfter_1582c0(LeafScanValue* target);
    i32 ProbeFirst_1584a0(i32 arg);
    i32 HasKeyEqual_1583c0(const char* str);
    CString FindKeyOfValue_158570(LeafScanValue* target);
    i32 MatchSub_1584f0(LeafScanSoundArg* arg1, i32 arg2);

    // These two landed in the SIBLING CDDrawSubMgrLeaf.cpp (name-preserving union):
    void ClearMap(); // 0x157bc0 (non-virtual map teardown)

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
    SoundDevice* m_2c;    // +0x2c  held DSound device
    i32 m_30;             // +0x30  busy/loading guard
    i32 m_34;             // +0x34  redraw arg
};

SIZE_UNKNOWN(LeafScanBase);
SIZE_UNKNOWN(CDDrawSubMgrLeafScan);

// --- vtable catalog (reduced-view classes share their base vtable rva) ---
VTBL(
    CDDrawSubMgrLeafScan,
    0x001efca0
); // ??_7CDDrawSubMgrLeafScan@@6B@ (9-slot LeafScanBase-derived)

#endif // GRUNTZ_DDRAWMGR_CDDRAWSUBMGRLEAFSCAN_H
