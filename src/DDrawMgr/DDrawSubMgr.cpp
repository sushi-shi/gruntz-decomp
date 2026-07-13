// DDrawSubMgr.cpp - the 0x156cb0-0x1591c9 original TU (wave4-L dossier #15, block
// G): the DDraw submgr worker-family obj - the CDDrawSubMgr base arg-ctor, the
// family's dtor/IsReady/GetStateId quartets (registry-host, child-group-host,
// leaf, leaf-scan tail bits), the CDDrawBlitParam cue-selector, the per-frame
// sound trigger, the CDDrawSubMgrPages surface ops, and the CDrawSubWorker leaf.
// Internally WOVEN (the #9 ~0x157a80/~0x1588f0 sub-splits are refuted); held at
// the dossier-#9 boundaries 2/3 (0x156cb0/0x1591e0) - the keeper-argument LEANS
// F==G (see dossier #15) but the escape (out-of-line tiny virtuals in a second
// file) is unexcluded, so F and G stay two files.
//
// original TU: filename unknown (@identity-TODO - no __FILE__ anchor).
//
// Field names are placeholders; only OFFSETS + emitted code bytes are load-bearing.

// The CObject-like family grand-base: 5-slot vtable (masks 0x5e8cb4), header fields
// +0x04..+0x0c, non-virtual ~ = the field resets + the implicit base ??_7 re-stamp.
// Slot 1 is the (declared-only) ??_G scalar-deleting dtor. Same shape as
// CDDrawSubMgrGrandBase / CDDrawWorkerCacheBase.
#include <Gruntz/SoundCueMgr.h>
#include <Gruntz/ParseSource.h>     // canonical CParseSource - MUST precede the Leaf headers
#include <Dsndmgr/DirectSoundMgr.h> // real DSound types (MatchSub GetFormat/SetPrimaryFormat)
#include <Dsndmgr/SoundDevice.h>
#include <Dsndmgr/SoundStream.h>      // the +0x2c held stream full def (base SoundDevice methods)
#include <stdio.h>                    // sprintf (the %s%s%s path walkers)
#include <DDrawMgr/DDrawWorkerNode.h> // CDDrawWorkerBase/A/B (the list-spawned workers)
#include <DDrawMgr/DDrawWorkerList.h> // CDDrawWorkerList (hoisted; factories here)
#include <DDrawMgr/DDrawWorkerMapSmall.h> // CDDrawWorkerMapSmall (hoisted; quartet here)
#include <DDrawMgr/DDrawWorkerCache.h>    // CDDrawWorkerCache (dtor here)
#include <DDrawMgr/DDrawPtrCollections.h> // the surface pool (CreateChildren children)
#include <Io/FileMem.h>                   // CFileMem/CFileMemBase (the ctor/dtor pocket)
#include <Gruntz/Sprite.h>
#include <Gruntz/LeafCue.h>
#include <Gruntz/AniElement.h>
#include <Wap32/Object.h>
#include <rva.h>
#include <Gruntz/StateId.h> // StateId (GetStateId return type)
#include <Mfc.h>            // real MFC CMapStringToPtr / CString / POSITION
#include <Bute/SymTab.h>    // CSymTab (ProbeWorkerKey's probe chain)
#include <string.h>
#include <DDrawMgr/DirectDrawMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>    // single-source CDDrawSurfacePair
#include <DDrawMgr/DDrawBlitParam.h>      // single-source CDDrawBlitParam
#include <Gruntz/SerialArchive.h>         // the shared CSerialArchive stream
#include <DDrawMgr/DDrawSurfaceMgr.h>     // canonical CDDrawSurfaceMgr
#include <DDrawMgr/DDrawSubMgrPages.h>    // single-source CDDrawSubMgrPages (surface ops)
#include <DDrawMgr/DDrawChildGroup.h>     // CDDrawChildGroup (the 3-map dtor-host twin)
#include <DDrawMgr/DDrawWorkerRegistry.h> // canonical CDDrawWorkerRegistry (+CWorkerValue)
#include <DDrawMgr/DDrawSubMgrLeaf.h>     // CDDrawSubMgrLeaf + CCatalogNode (hoisted)
#include <DDrawMgr/DDrawSubMgrLeafScan.h> // canonical CDDrawSubMgrLeafScan
#include <DDrawMgr/AniAdvance.h>          // CAniBlitTrigger (the per-frame sound trigger)
#include <Dsndmgr/SoundResMap.h>          // CSoundResMap (RemoveByValue @0x157b00)
#include <Wap32/WapObj.h>                 // CWapObj : CObject
#include <Globals.h>
struct FamilyMapBase {
    virtual void s0();        // [0]
    virtual ~FamilyMapBase(); // slot 1 (deleting dtor -> cl-emitted ??_G)
    virtual void s2();        // [2]
    virtual void s3();        // [3]
    virtual void s4();        // [4]
    i32 m_04;                 // +0x04
    i32 m_08;                 // +0x08
    i32 m_0c;                 // +0x0c
    FamilyMapBase() {}
};
inline FamilyMapBase::~FamilyMapBase() {
    m_04 = -1;
    m_08 = 0;
    m_0c = 0;
}
SIZE_UNKNOWN(FamilyMapBase);

// 3-map sibling (vtable 0x5efdc0): member-teardown ~ at 0x157630; its ??_G scalar-dtor
// (0x157610) lives in the mapsmall quartet and calls this ~.
struct CDDrawChildGroupDtorHost : public FamilyMapBase {
    ~CDDrawChildGroupDtorHost(); // 0x157630
    void* ScalarDtor(u32 flags); // 0x157610 (the ??_G; runs ~ then RezFree)
    CMapStringToPtr m_10;        // +0x10
    CMapStringToPtr m_2c;        // +0x2c
    CMapStringToPtr m_48;        // +0x48
    i32 m_64;                    // +0x64
};
SIZE_UNKNOWN(CDDrawChildGroupDtorHost);

// 1-map sibling (vtable 0x5efd28): member-teardown ~ at 0x156e10; its ??_G scalar-dtor
// (0x156df0) calls this ~ then RezFree.
struct CDDrawRegistryDtorHost : public FamilyMapBase {
    ~CDDrawRegistryDtorHost();   // 0x156e10
    void* ScalarDtor(u32 flags); // 0x156df0
    CMapStringToPtr m_10;        // +0x10
};
SIZE_UNKNOWN(CDDrawRegistryDtorHost);

// operator delete (NAFXCW ??3@YAXPAX@Z @0x1b9b82) - the scalar-dtor free path.
void operator delete(void*);

// The CLoadable-shaped base arg-ctor class (its own local model; the CObject-derived
// <DDrawMgr/DDrawSubMgr.h> base is a DIFFERENT class wearing the same name - the
// conflation is flagged there for the identity pass).
class CDDrawSubMgrBase {
public:
    CDDrawSubMgrBase() {}
    CDDrawSubMgrBase(i32 x) {
        m_base04 = x;
    }
    virtual ~CDDrawSubMgrBase() {}
    i32 m_base04; // +0x04
};
SIZE_UNKNOWN(CDDrawSubMgrBase);

class CDDrawSubMgr : public CDDrawSubMgrBase {
public:
    CDDrawSubMgr(CDDrawSurfaceMgr* pSurfaceMgr, i32 a2, i32 a3);
    virtual ~CDDrawSubMgr() OVERRIDE {}
    virtual void IsReady();
    virtual i32 Init();
    virtual i32 OnDestroy();      // declared-only (the old 0x1576c0 attribution was wrong:
                                  // that body is CDDrawChildGroup::IsReady, vtbl slot 6)
    virtual StateId GetStateId(); // 0x157790 (out-of-line at its RVA position)

    i32 m_field08;                   // +0x08
    CDDrawSurfaceMgr* m_pSurfaceMgr; // +0x0c
};
SIZE_UNKNOWN(CDDrawSubMgr);
// 0x155720 is NOT this local CDDrawSubMgr's ??_G: it is CDDrawSubMgrFar's scalar-
// deleting destructor (member-teardown ~ at 0xd5d70, CImage.cpp), now modeled as
// CDDrawSubMgrFar::ScalarDtor in DDrawWorkerRegistry.cpp (its RVA-correct obj span)
// so the member-dtor CALL binds to ??1CDDrawSubMgrFar. The prior @rva-symbol pinned
// this local class's empty-dtor ??_G here, leaving that call UNBOUND (misattribution).

// The small per-frame blit-param source (Setup/SelectCue's +0x14 resolved source).
class CDDrawBlitParamSrc {
public:
    char m_pad00[0x0c]; // +0x00 .. +0x0b
    // authentic: worker-node-like element ptr; only ever raw-offset read.
    void* m_elements; // +0x0c -> worker node
    i32 m_count;      // +0x10 count
    char m_pad14[0x20 - 0x14];
    float m_scale; // +0x20
};
SIZE_UNKNOWN(CDDrawBlitParamSrc);

// The sound-cue enable flag, a float pan/volume scale constant, and the cue tag.
// g_sndEnabled / g_sndCueTag are DEFINED in src/Gruntz/GruntzMgr.cpp (the owner TU);
// the plain externs come from <Globals.h>. The DATA pins that used to sit on these two
// DECLARATIONS are gone - a pin on an extern is not a definition, and it was competing
// with LevelPreview.cpp's extern-"C" pin at the same rvas.
DATA(0x001eff2c)
extern float g_sndPanScale; // 0x5eff2c

// The shared empty-string datum (labeled by netmgrerror; declared-only here).
extern char g_emptyString[]; // 0x2293f4

// ===========================================================================
// The CDDrawSubMgrLeafScan section type skeleton (views/types from the merged
// DDrawSubMgrLeafScan.cpp, hoisted ahead of the RVA-ordered fn chunks).
// ===========================================================================
// The sound-cue globals (g_sndEnabled/g_sndCueTag - defined in GruntzMgr.cpp, declared
// in <Globals.h>; g_killCueClock is bound by triggermgr). The donor-name aliases
// (g_61ab20/g_61ab24/g_scrollEnabled/g_scrollDelta/g_killCueClock/g_aniCueItem) are unified
// onto these canonicals so the reloc targets bind to their real rvas; reloc-masked,
// matching-neutral.
extern "C" u32 g_killCueClock; // 0x2bf3c0
// DDrawSubMgrLeafScan.cpp - a sibling sub-manager of the tomalla-named
// CDDrawSubMgrLeaf family (a CDirectDrawMgr surface/page sub-manager in the
// "DDraw surface manager" group; see docs/ddraw-family-names.md). This is
// the keyed-asset CACHE variant: it owns a CMapStringToPtr at +0x10 keyed by
// const char* strings, a busy/loading guard at +0x30, plus the shared base
// fields (status word at +0x04, parent/root handle at +0x0c).
//
// The cluster splits into two co-operating groups, both keyed on the same +0x10
// map:
//   - map-scan helpers (Lookup / HasKeyEqual / RemoveKeysEqual / FindKeyOfValue)
//     -- byte-for-byte twins of the same helpers on CDDrawWorkerRegistry.
//   - the directory-scan / factory group (the recursive %s%s%s path walker, the
//     0x1c-byte element factory, and the throttled per-asset refresh) -- builds
//     cache entries by enumerating files via the engine's FindFirst/Next API.
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + emitted code
// bytes are load-bearing (campaign doctrine). All the engine callees (the MFC
// CMapStringToPtr/CString thunks, sprintf/strncmp, the file-iteration API, the
// per-element ConfigureItem) are reloc-masked external calls.
// ---------------------------------------------------------------------------

// <Mfc.h> brings real MFC CObject / CMapStringToPtr / CString / POSITION; afx-first
// so it precedes any <windows.h>/DirectX header.

// Real DSound types so MatchSub_1584f0's GetFormat / SetPrimaryFormat calls
// mangle to the retail names (the relocs pair instead of staying fuzzy).

// The map value: only the scalar-deleting destructor slot (+0x04) is load-
// bearing for the RemoveKeysEqual/FindKey teardown dispatch. Declared only -
// never defined, so no ??_7 emitted here. m_10 is the held sound-arg the probe
// helpers forward to MatchSub; FindKeyOfValue compares the value pointer itself.
class LeafScanValue {
public:
    virtual void GetRuntimeClass(); // [0] 0x1bef01 (shared thunk, declared-only)
    virtual ~LeafScanValue();       // slot 1 (deleting dtor -> cl-emitted ??_G)
    char m_pad04[0x10 - 0x04];      // +0x04..0x0f (after the vptr)
    void* m_10;                     // +0x10  held sound-arg (LeafScanSoundArg*)
};

// The SumField source: the value's m_10 points at a record whose +0x2c word is
// the per-entry count the prefix scan accumulates. A layout-compatible view so
// the read lowers to `mov eax,[val+0x10]; add ebp,[eax+0x2c]` (offsets load-bearing).
struct LeafSumSource {
    char m_pad00[0x2c];
    i32 m_2c; // +0x2c  the accumulated count
};

// ----- DSNDMGR sub-objects used by MatchSub_1584f0 -----
// arg1->m_10 is the real DirectSoundMgr buffer wrapper (its GetFormat @0x135ac0
// pairs by name). The held SoundDevice at this+0x2c exposes SetPrimaryFormat
// (0x1371a0, pairs by name) and a still-unnamed start-primary thunk at 0x137200.
class LeafScanSoundArg {
public:
    char m_pad00[0x10];   // +0x00..0x0f
    DirectSoundMgr* m_10; // +0x10
};
// The sub-manager's OWN vtable (0x5efca0) is no longer an extern: CDDrawSubMgrLeafScan
// is real-polymorphic, so cl emits ??_7CDDrawSubMgrLeafScan + the implicit grand-base
// re-stamp. The 0x1c cache element (LeafElementObj) is now real-polymorphic too
// (below), so its grand-base dtor vtable (0x5e8cb4) is the cl-emitted
// ??_7LeafElementBase - the manual g_wapObjectDtorVtbl stamp is gone from this TU.

// ----- The throttled per-asset refresh (RefreshAsset_114120) -----
// The map value, when refreshed, is a draw-cue record: its +0x10 player drives
// ConfigureItem (0x1360d0); +0x14 is the last draw-clock, +0x18 the throttle
// interval. Same shape as the CSBI_MenuItem cue path. Externals are reloc-masked.
struct LeafCuePlayer;
// The reentrancy gate + cue-item id pair the refresh plays through, and the
// draw-clock mirror (wrap-safe gate compare). Shared globals (see SBI_MenuItem).

// ----- The 0x1c-byte cache element + its factory (CreateEntry_157d70) -----
// operator new(0x1c); the factory constructs the element (real ctor auto-stamps the
// element vtable 0x5eff08 = cl-emitted ??_7LeafElementObj), copies the map count
// (this+0x1c) and handle (this+0x0c), zeroes the rest, then runs the element's
// Configure (0x158760) keyed by arg2; on success links it into the map and stamps
// the redraw arg (this+0x34). LeafElementObj is real-polymorphic now (VTBL at EOF).
// The element's draw-source the factory passes to Configure is the canonical
// CParseSource (included above): BeginParse (0x139960 -> the parsed RIFF/WAVE
// blob, or 0) and EndParse (0x1399d0). The `mov ecx,src; call <thunk>` reloc-
// masks; the trace tagged the same reader (the symtab/parse-stream node).
// The parent root handle the base stores at +0x0c (a raw word in the LeafScanBase
// shape): its +0x20 word is the SoundDevice the element acquires/releases its
// buffer through. The handle is a raw word in the base, so reaching the device is
// an authentic int->object reinterpret (`mov eax,[this+0xc]; mov ecx,[eax+0x20]`).
struct LeafRootHandle {
    char m_pad0[0x20];
    SoundDevice* m_20; // +0x20  the owning DSound device
};

// The element's CObject-like grand-base subobject (vptr + status word at +0x04 +
// root handle at +0x0c). Modeled as a REAL polymorphic base (its 5-slot vtable is
// the shared grand-base) so cl emits the implicit grand-base vptr re-stamp (masks
// 0x5e8cb4) at the derived dtor's tail -- no manual `*(void**)this = &g_*Vtbl`. Its
// virtual ~ resets the three fields; the base transition stamp is implicit. Because
// this base carries a NON-TRIVIAL dtor, the derived ~LeafElementObj gets a /GX EH
// frame protecting the base teardown across the Release() call (the half-destructed
// element cleanup edge). Same shape as LeafScanBase / CResolveNode.
// NAME-AUDIT (vtable_hierarchy --name-audit): maps to RTTI CObject @0x1e8cb4, but
// KEPT as a real intermediate - it carries the m_04/m_08/m_0c header past the bare
// vptr, so it is NOT a bare-CObject fold (Wap32/Object.h). Do not rename to
// CObject (would ODR-clash + collapse the /GX dtor teardown level).
struct LeafElementBase {
    virtual void GetRuntimeClass(); // [0] 0x1bef01 (shared thunk, declared-only)
    virtual ~LeafElementBase();     // [1] scalar-deleting dtor (0x158660 ??_G)
    virtual void Serialize();       // [2] 0x0028ec (shared thunk, declared-only)
    virtual void AssertValid();     // [3] 0x00106e (shared thunk, declared-only)
    virtual void Dump();            // [4] 0x004034 (shared thunk, declared-only)

    i32 m_04; // +0x04 = parent map count (-1 when dead)
    i32 m_08; // +0x08 = 0
    i32 m_0c; // +0x0c = parent root handle (LeafRootHandle*)
    LeafElementBase() {}
};
inline LeafElementBase::~LeafElementBase() {
    m_04 = -1;
    m_08 = 0;
    m_0c = 0;
}
// The 0x1c-byte element layout. Only the seeded offsets are load-bearing. Its 9-slot
// vtable (??_7LeafElementObj @0x5eff08) is 5 shared grand-base slots (slot 1 = the
// virtual dtor) + 4 leaf virtuals (slots 5..8), declared-only so cl references them
// externally (reloc-masked). Its ctor auto-stamps the element vtable + seeds the
// fields; ~dtor (0x158680) auto-stamps it, runs Release, then the base subobject dtor
// auto-fires (reset +0x04/+0x08/+0x0c + implicit grand-base re-stamp). Configure
// (0x158760) loads + acquires the element's buffer; Release (0x1587c0) frees it (both
// non-virtual __thiscall members reached only from the element).
struct LeafElementObj : public CObject {     // was : LeafElementBase (merged intermediate)
    i32 m_04, m_08, m_0c;                    // +0x04..0x0f (from merged LeafElementBase)
    virtual void LeafSlot5_158650();         // [5] 0x158650 (declared-only)
    virtual void IsValidImage();             // [6] 0x001c08 (shared thunk, declared-only)
    virtual void LeafSlot7_1587c0();         // [7] 0x1587c0 (declared-only; == Release addr)
    virtual void LeafSlot8_154a00();         // [8] 0x154a00 (declared-only)
    virtual ~LeafElementObj() OVERRIDE;      // overrides slot [1]
    LeafElementObj(i32 count, i32 handle);   // inline; folded into the factory
    i32 Configure_158760(CParseSource* src); // 0x158760 __thiscall element configure
    i32 Configure2_158720(void* riff);       // 0x158720 raw-RIFF configure variant
    void Release_1587c0();                   // 0x1587c0 release the acquired buffer

    i32 m_10; // +0x10 = 0  the acquired DirectSound buffer
    i32 m_14; // +0x14 = 0
    i32 m_18; // +0x18 = 0 (-> parent->m_34 on success)
}; // size = 0x1c
// Seed order mirrors the factory's writes: count, 0, handle (base fields), then the
// zeroed tail with +0x18 before +0x14. The vptr is cl-auto-stamped (ctor prologue).
inline LeafElementObj::LeafElementObj(i32 count, i32 handle) {
    m_04 = count;
    m_08 = 0;
    m_0c = handle;
    m_10 = 0;
    m_18 = 0;
    m_14 = 0;
}

// ----- The recursive directory walker (ScanTree_157ee0) -----
// The former `DirNode` view is DISSOLVED (2026-07-13). Its own comment block already
// named every method it "declared" as a CSymTab method, and the body cast `(CSymTab*)
// tree` at all six call sites - the view was a shape nobody had connected to its owner.
// It also CONFLATED two classes at one name:
//   - the scope nodes (tree / the FirstSub-NextSub subdir chain) are Bute CSymTab
//     (<Bute/SymTab.h>): m_name @+0x00, and each subdir is recursed into AS a tree.
//   - the leaf records (the NextSym2/NextSym3 chain) are CParseSource
//     (<Gruntz/ParseSource.h>): m_name @+0x00 AND GetEntryTag @0x139800 (the 'WAV'
//     tag gate) - and NextSym3 advances via the node at rec+0x1c, which is exactly
//     CParseSource::m_node1c. That +0x1c node pins the leaf type independently.
// Both are real, already-modeled classes; the walker now uses them directly.
// The buffer freed at the walker's tail (::operator delete, ??3 @0x1b9b82).
// Global operator new/delete (engine NAFXCW, operator_new @0x1b9b46); external/
// no-body. `delete e` on the polymorphic element routes through operator delete.
void* operator new(u32 n);
void operator delete(void* p);

// The canonical CDDrawSubMgrLeafScan + its LeafScanBase grand-base now live in the
// shared <DDrawMgr/DDrawSubMgrLeafScan.h> (included above): the class def is the
// single-source union of this TU's method set and the sibling CDDrawSubMgrLeaf TU's
// ??_G/IsReady/ClearMap. The body-only dep types (LeafElementObj / LeafScanValue /
// LeafScanSoundArg) are forward-declared in the header and fully defined above so the
// method bodies below can dereference them. (ScanTree's tree/leaf types are no longer
// among them: they are the real CSymTab / CParseSource from their own headers.)

// Read the map count at parent+0x1c (inside the CMapStringToPtr's internal area,
// its m_nCount). A separate inline so its read schedules before the handle read,
// matching the factory's register assignment.
static inline i32 LeafReadMapCount(const CDDrawSubMgrLeafScan* p) {
    return *(const i32*)((const char*)p + 0x1c);
}

// Inline element constructor. A real `new LeafElementObj(count, handle)`: cl emits
// the operator-new + null-guarded ctor call; the ctor auto-stamps the element vptr
// (??_7LeafElementObj) and seeds the fields (map count, handle, zeroed tail). The
// count/handle reads happen before the alloc (they are the ctor args).
static inline LeafElementObj* MakeLeafElement(const CDDrawSubMgrLeafScan* parent) {
    i32 count = LeafReadMapCount(parent);
    i32 handle = parent->m_0c;
    return new LeafElementObj(count, handle);
}

// LeafScanBase / CDDrawSubMgrLeafScan SIZE_UNKNOWN now live in the shared header.
// ??_7LeafElementObj (was g_leafElemVtbl @0x5eff08, LeafElemVtbl / the LeafElementObj vtable).
// cl auto-emits it from the real-polymorphic element; retail's 9-slot datum is
// reloc-masked -> matching-neutral catalog tracking.

// --- vtable catalog (reduced-view classes share their base vtable rva) ---

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---

// --- end of type preamble; function bodies below in strict RVA order ---
// ===========================================================================
// Two FOREIGN lone methods carved out of this obj (operation REHOME, package D8):
//   - 0x1f940 LeafCue::PlayIfElapsed  -> src/Gruntz/LeafCuePlay.cpp
//   - 0x31250 CQueueDrainHost::Drain  -> src/Gruntz/QueueDrainHost.cpp
// Both sit ~1.2 MB before the DDraw submgr block (0x156cb0+); each is its own obj.
// ===========================================================================

// ---------------------------------------------------------------------------
// 0x5b7e0: CDDrawSubMgrLeafScan::Lookup_05b7e0 was HOMED to src/Gruntz/GruntCombat.cpp
// (REHOME D10). Retail's out-of-line COMDAT sits inside CGrunt's gruntcombat block
// (0x0005b6f0 FindGridNeighbor .. 0x0005baf0 GruntSpawnPump) - a rule-(c) interleaver
// surrounded by gruntcombat on both sides. GruntCombat.cpp now includes
// <DDrawMgr/DDrawSubMgrLeafScan.h>, unblocking the move. This drops the 0x5b7e0 stray
// from this obj's .text (contiguity win).

// ---------------------------------------------------------------------------
// 0x114120: throttled per-asset refresh. While not loading (m_30==0), look up the
// keyed cue in the map; if present and the reentrancy gate is open, and the
// throttle interval has elapsed since its last draw-clock, restamp the clock and
// re-run its player's ConfigureItem with the gated cue-item id. Returns 0 always
// (the success path falls off the end of ConfigureItem's void return). 1 stack
// arg (ret 4); same cue-refresh idiom as CSBI_MenuItem's highlight path.
// @interleaver CDDrawSubMgrLeafScan::RefreshAsset_114120 emitted-in <boundary: unreconstructed>
// (REHOME D10 re-classified: this own-class out-of-line COMDAT sits at a BOUNDARY, NOT
// inside a single host. Retail neighbours are tileswitchlogic
// CTileActionEvent::DeserializeFields @0x114040 (before) + toobspikez ToobSpikezLogic
// @0x114480 (after) - different units on each side. The D8 "emitted-in
// tiletriggerswitchlogic" claim was a proximity-window guess, not adjacency; the true
// owning obj is the unreconstructed 0x114xxx run. Unlike the 0x5b7e0 twin (a clean
// rule-(c) interleaver in gruntcombat, now homed), this one has no single reconstructed
// host to home into. Kept-in-place + flagged.)
RVA(0x00114120, 0x70)
i32 CDDrawSubMgrLeafScan::RefreshAsset_114120(const char* key) {
    if (m_30 != 0) {
        return 0;
    }
    void* val = 0;
    m_10.Lookup(key, val);
    if (val == 0) {
        return 0;
    }
    i32 gate = g_sndEnabled;
    i32 item = g_sndCueTag;
    if (gate == 0) {
        return 0;
    }
    LeafCue* p = (LeafCue*)val;
    // Throttle: when the interval has elapsed, restamp the clock and tail-return the
    // (void-modeled) ConfigureItem result so the success epilogue falls through
    // WITHOUT zeroing eax (retail's split-epilogue shape: the guard-failure paths
    // return 0 via the trailing `xor eax,eax` exit, success is the fall-through).
    if (g_killCueClock - (u32)p->m_14 >= (u32)p->m_18) {
        p->m_14 = g_killCueClock;
        return p->m_10->ConfigureItem(item, 0, 0, 0);
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CDDrawSubMgr::CDDrawSubMgr (0x156cb0): chains the base(int) ctor (inlined:
// this+0x04 = a2), stamps the CDDrawSubMgr vtable (compiler-generated), then
// seeds the remaining fields.
RVA(0x00156cb0, 0x20)
CDDrawSubMgr::CDDrawSubMgr(CDDrawSurfaceMgr* pSurfaceMgr, i32 a2, i32 a3) : CDDrawSubMgrBase(a2) {
    m_field08 = a3;
    m_pSurfaceMgr = pSurfaceMgr;
}

// Out-of-line stubs for unmatched virtuals (anchors the vtable in this TU).
void CDDrawSubMgr::IsReady() {}
i32 CDDrawSubMgr::Init() {
    return 0;
}

// ---------------------------------------------------------------------------
// CDDrawWorkerMapSmall quartet (0x156cd0-0x157610).
RVA(0x00156cd0, 0x16)
i32 CDDrawWorkerMapSmall::IsReady() {
    if (m_0c == 0) {
        goto fail;
    }
    if (m_04 != -1) {
        return 1;
    }

fail:
    return 0;
}

// ~CDDrawWorkerMapSmall (0x156d20, __thiscall, /GX): REAL virtual dtor. cl stamps
// ??_7CDDrawWorkerMapSmall (masks 0x5efcc8) at entry, runs the map teardown
// (DestroyAll, T obj), then destructs the three CMapStringToPtr members (reverse
// decl order, descending trylevels) and the grand-base.
// @early-stop
// vptr-position wall (~94%, twin of CDDrawWorker::~CDDrawWorker): every
// instruction matches retail EXCEPT the grand-base vptr re-stamp POSITION (cl
// stamps at base-dtor entry, retail sinks it after the field resets) + the
// reloc-masked EH names. WALL RE-PROVEN for the clean polymorphic model
// (eh-dtor-implicit-vptr-stamp-first.md sub-case 2 does NOT apply - three
// destructible members intervene). Logic complete.
RVA(0x00156d20, 0x82)
CDDrawWorkerMapSmall::~CDDrawWorkerMapSmall() {
    DestroyAll();
    // m_map3 / m_map2 / m_map1 (reverse decl order) and the grand-base
    // auto-destruct here under the /GX member-teardown trylevels.
}

RVA(0x00156db0, 0x6)
i32 CDDrawWorkerMapSmall::Slot06_156db0() {
    return 1;
}

// CDDrawWorkerRegistry::GetStateId (0x156de0): the class's state id.
RVA(0x00156de0, 0x6)
StateId CDDrawWorkerRegistry::GetStateId() {
    return STATE_WORKERREGISTRY; // 0x12
}

// ---------------------------------------------------------------------------
// 0x156df0: the ??_G scalar-deleting destructor of the 1-map sibling
// CDDrawRegistryDtorHost - run the real member-teardown ~ (0x156e10, below)
// then, when the low deleting-flag bit is set, RezFree(this); return this.
// Hand-written RVA-pinned method (the CGruntzCommand::ScalarDtor pattern).
RVA(0x00156df0, 0x1e)
void* CDDrawRegistryDtorHost::ScalarDtor(u32 flags) {
    this->CDDrawRegistryDtorHost::~CDDrawRegistryDtorHost();
    if (flags & 1) {
        ::operator delete(this);
    }
    return this;
}

// ---------------------------------------------------------------------------
// 0x156e10: member-teardown ~ of the 1-map sibling CDDrawRegistryDtorHost (vtable
// 0x5efd28). Runs the cleanup helper (0x154ac0), then the CMapStringToPtr member
// and the FamilyMapBase grand-base auto-destruct. /GX member-teardown frame.
// @early-stop
// vptr-position wall (~95%, family twin): grand-base vptr re-stamp position + the
// reloc-masked EH-state/teardown/map-dtor names are the residual.
RVA(0x00156e10, 0x68)
CDDrawRegistryDtorHost::~CDDrawRegistryDtorHost() {
    ((CDDrawWorkerRegistry*)this)->Shutdown();
    // implicit: ~m_10, ~FamilyMapBase (resets + base restamp).
}

// ---------------------------------------------------------------------------
// 0x156e80: probe `arg1` through 0x13b900(arg2) -> object, deref via 0x13a230; if
// the result is non-null, dispatch this->+0x48 with (result, g_emptyString,
// &g_dat60b588) and return it, else 0. __thiscall, 2 args (ret 8).
RVA(0x00156e80, 0x38)
i32 CDDrawWorkerRegistry::ProbeWorkerKey(CSymTab* arg1, i32 arg2) {
    CSymTab* obj = arg1->Get_13b900();
    void* result = obj->FindSub((const char*)arg2);
    if (result == 0) {
        return 0;
    }
    return ((CWorkerVtableView*)this)
        ->InstallTree(result, g_emptyString, (const char*)&g_dat60b588);
}

// ---------------------------------------------------------------------------
// 0x156ec0: Lookup `key` in the map; if found, RemoveKey it and run the value's
// scalar-deleting destructor (vtbl +0x4, arg 1).
// @early-stop
// ~77.5% - register-allocation + store/load-scheduling entropy: the logic, CFG,
// the val=0 init, both library calls, their args, and the dtor dispatch are all
// reproduced; only the register schedule differs (retail holds `key` in EDI and
// keeps `val` purely on the stack; MSVC5 caches key in EBX and val in EDI).
// Every source form tried produced the identical schedule; the surrounding
// symbol-set is what re-rolls the allocation. Left as the plateau.
RVA(0x00156ec0, 0x40)
void CDDrawWorkerRegistry::RemoveByKey(const char* key) {
    CObject* val = 0;
    if (m_map.Lookup(key, val)) {
        m_map.RemoveKey(key);
        delete ((CWorkerValue*)val);
    }
}

// ---------------------------------------------------------------------------
// CDDrawWorkerList quartet + factories (0x156f00-0x1573e0).
RVA(0x00156f00, 0x16)
i32 CDDrawWorkerList::IsReady() {
    if (m_pSurfaceMgr == 0) {
        goto fail;
    }
    if (m_status != -1) {
        return 1;
    }

fail:
    return 0;
}

RVA(0x00156f20, 0x6)
StateId CDDrawWorkerList::GetStateId() {
    return STATE_WORKERLIST; // 0x11
}

// 0x156f50: real member-teardown ~ of the sibling manager (vtable 0x5efd88): stamp
// the class vtable, run the cleanup helper (0x163bc0, T obj), destruct the CObList
// member @+0x10, reset the header fields + restamp the grand-base vtable.
// @early-stop
// (formerly the eh-unit-rule wall: the donor list file was /MT base; this G obj is
// /GX like retail, so re-measure on the next sweep.) The vtable stamps / cleanup
// call / CObList destruct / field resets are reproduced.
RVA(0x00156f50, 0x68)
CDDrawWorkerListSib::~CDDrawWorkerListSib() {
    ((CDDrawWorkerList*)this)->~CDDrawWorkerList();
    // implicit: ~m_10 (CObList) then ~WorkerListSibBase (field resets + base restamp).
}

RVA(0x00156fc0, 0x6)
i32 CDDrawWorkerList::IsReadyPredicate() {
    return 1;
}

// Inline worker constructors. Each new's the raw block, and on success seeds the
// fields THROUGH the allocation register and returns it. Defined inline so they
// fold into each factory, reproducing the target's "init via eax, commit to esi
// only at the merge" register schedule.
static inline CDDrawWorkerB* MakeWorkerB(const CDDrawWorkerList* parent) {
    CDDrawWorkerB* w = new CDDrawWorkerB;
    if (w != 0) {
        CDDrawWorkerCtx* surfaceMgr = parent->m_pSurfaceMgr;
        w->m_04 = 0;
        w->m_ctx = surfaceMgr;
        w->m_08 = 0;
        w->m_20 = (i32)0x80000000;
        w->m_38 = -1;
        w->m_5c = (i32)0x80000000;
        w->m_64 = (i32)0x80000000;
        w->m_3c = 0;
        w->m_40 = 0;
        w->m_78 = 0;
    }
    return w;
}

static inline CDDrawWorkerA* MakeWorkerA(const CDDrawWorkerList* parent) {
    CDDrawWorkerA* w = new CDDrawWorkerA;
    if (w != 0) {
        CDDrawWorkerCtx* surfaceMgr = parent->m_pSurfaceMgr;
        w->m_04 = 0;
        w->m_ctx = surfaceMgr;
        w->m_08 = 0;
        w->m_20 = (i32)0x80000000;
        w->m_38 = -1;
        w->m_5c = (i32)0x80000000;
        w->m_64 = (i32)0x80000000;
        w->m_3c = 0;
        w->m_40 = 0;
        w->m_78 = 0;
    }
    return w;
}

// Allocates a BYTE-flag worker, constructs it, calls its +0x2c virtual with
// (a1,a2,a3). On success appends it to the list (AddTail) and returns it.
RVA(0x00156fd0, 0x8b)
void* CDDrawWorkerList::CreateWorkerA(i32 a1, i32 a2, i32 a3) {
    CDDrawWorkerA* w = MakeWorkerA(this);
    if (w->Vfunc2C(a1, a2, a3) == 0) {
        if (w != 0) {
            delete w;
        }
        return 0;
    }
    m_workers.AddTail((::CObject*)w);
    return w;
}

// ---------------------------------------------------------------------------
// CDDrawWorkerA/B dtors + frame-set virtuals (0x1570d0-0x1572f0).
// The frame-source passed as Vfunc30's a3: an int array @+0x14 indexed by a4,
// bounded by [m_lowerBound, m_upperBound].
struct CDDrawFrameSource {
    char _pad00[0x14];
    i32* m_frameTable; // +0x14  frame table
    char _pad18[0x64 - 0x18];
    i32 m_lowerBound; // +0x64  lower bound
    i32 m_upperBound; // +0x68  upper bound
};
SIZE_UNKNOWN(CDDrawFrameSource);

// ~CDDrawWorkerA (0x1570d0; ??_G wrapper 0x1570b0): poison the timing/marker
// fields to their sentinels (the m_20/m_38 pair reset THREE times, via volatile
// lvalues so cl keeps all three), null the header; cl stamps the CObject
// grand-base at the tail via the implicit base teardown.
// @early-stop
// constant-materialization scheduling wall: byte-faithful except a 2-instruction
// swap (retail hoists `or eax,-1` ahead of the m_78 byte store).
RVA(0x001570d0, 0x39)
CDDrawWorkerA::~CDDrawWorkerA() {
    volatile i32* pHi = &m_20;
    volatile i32* pLo = &m_38;
    m_78 = 0;
    *pHi = (i32)0x80000000;
    *pLo = -1;
    *pHi = (i32)0x80000000;
    *pLo = -1;
    m_5c = (i32)0x80000000;
    *pHi = (i32)0x80000000;
    *pLo = -1;
    m_04 = -1;
    m_08 = 0;
    m_ctx = 0;
}

RVA(0x00157110, 0x20)
i32 CDDrawWorkerA::Vfunc2C(i32 a1, i32 a2, i32 a3) {
    m_78 = (char)a3;
    m_74 = 2;
    return Helper_164790(a1, a2);
}

// Int-flag worker; calls the worker's +0x34 virtual with (a1,a2,a3,a4).
RVA(0x00157150, 0xa5)
void* CDDrawWorkerList::CreateWorkerB30(i32 a1, i32 a2, i32 a3, i32 a4, i32 addHead) {
    CDDrawWorkerB* w = MakeWorkerB(this);
    if (w->Vfunc34(a1, a2, a3, a4) == 0) {
        if (w != 0) {
            delete w;
        }
        return 0;
    }
    if (addHead & 1) {
        m_workers.AddHead((::CObject*)w);
    } else {
        m_workers.AddTail((::CObject*)w);
    }
    return w;
}

// ~CDDrawWorkerB (0x157240; ??_G wrapper 0x157220). Mirror of ~CDDrawWorkerA -
// the int-frame worker's m_78 is a DWORD here (byte in A).
// @early-stop
// constant-materialization scheduling wall: same as ~CDDrawWorkerA.
RVA(0x00157240, 0x3c)
CDDrawWorkerB::~CDDrawWorkerB() {
    volatile i32* pHi = &m_20;
    volatile i32* pLo = &m_38;
    m_78 = 0;
    *pHi = (i32)0x80000000;
    *pLo = -1;
    *pHi = (i32)0x80000000;
    *pLo = -1;
    m_5c = (i32)0x80000000;
    *pHi = (i32)0x80000000;
    *pLo = -1;
    m_04 = -1;
    m_08 = 0;
    m_ctx = 0;
}

RVA(0x00157280, 0x30)
i32 CDDrawWorkerB::Vfunc34(i32 a1, i32 a2, i32 a3, i32 a4) {
    Helper_166040(a3, a4);
    m_74 = 2;
    return Helper_164790(a1, a2);
}

// 0x1572b0: store frame `src->m_frameTable[a4]` (0 if a4 out of bounds) into
// m_78, set m_74=2, then forward (a1,a2) to Helper_164790.
RVA(0x001572b0, 0x38)
i32 CDDrawWorkerB::Vfunc30(i32 a1, i32 a2, CDDrawFrameSource* src, i32 a4) {
    i32 frame;
    if (a4 >= src->m_lowerBound && a4 <= src->m_upperBound) {
        frame = src->m_frameTable[a4];
    } else {
        frame = 0;
    }
    m_78 = frame;
    m_74 = 2;
    return Helper_164790(a1, a2);
}

RVA(0x001572f0, 0x20)
i32 CDDrawWorkerB::Vfunc2C(i32 a1, i32 a2, i32 a3) {
    m_78 = a3;
    m_74 = 2;
    return Helper_164790(a1, a2);
}

// Int-flag worker; calls the worker's +0x30 virtual with (a1,a2,a3,a4).
RVA(0x00157330, 0xa5)
void* CDDrawWorkerList::CreateWorkerB2C(
    i32 a1,
    i32 a2,
    CDDrawFrameSource* a3,
    i32 a4,
    i32 addHead
) {
    CDDrawWorkerB* w = MakeWorkerB(this);
    if (w->Vfunc30(a1, a2, a3, a4) == 0) {
        if (w != 0) {
            delete w;
        }
        return 0;
    }
    if (addHead & 1) {
        m_workers.AddHead((::CObject*)w);
    } else {
        m_workers.AddTail((::CObject*)w);
    }
    return w;
}

// As CreateWorkerA but the int-flag worker; trailing bool selects AddHead/AddTail.
RVA(0x001573e0, 0xa0)
void* CDDrawWorkerList::CreateWorkerB28(i32 a1, i32 a2, i32 a3, i32 addHead) {
    CDDrawWorkerB* w = MakeWorkerB(this);
    if (w->Vfunc2C(a1, a2, a3) == 0) {
        if (w != 0) {
            delete w;
        }
        return 0;
    }
    if (addHead & 1) {
        m_workers.AddHead((::CObject*)w);
    } else {
        m_workers.AddTail((::CObject*)w);
    }
    return w;
}

// ---------------------------------------------------------------------------
// CDDrawSubMgrPages quartet + children (0x157480-0x1591b0).
// slot 5 (IsLoaded, 0x157480): ready when all three owned child pointers are set.
RVA(0x00157480, 0x1e)
i32 CDDrawSubMgrPages::IsLoaded() {
    if (m_backPair == 0) {
        goto fail;
    }
    if (m_overlayPair == 0) {
        goto fail;
    }
    if (m_frontPair != 0) {
        return 1;
    }

fail:
    return 0;
}

// CDDrawSubMgrPages::ScalarDtor - the slot-1 `??_G` scalar-deleting dtor (0x1574b0):
// run the real ~CDDrawSubMgrPages (direct call), conditionally RezFree, return this.
RVA(0x001574b0, 0x1e)
void* CDDrawSubMgrPages::ScalarDtor(u32 flags) {
    this->CDDrawSubMgrPages::~CDDrawSubMgrPages();
    if (flags & 1) {
        ::operator delete(this);
    }
    return this;
}

// Member-teardown destructor (0x1574d0; retail ~CDDrawSubMgrDraco). cl stamps the
// own vftable ??_7CDDrawSubMgrPages (masks 0x5efe08) at entry, devirtualizes
// DestroyChildren (slot 7) to a direct call, resets the three header words, then
// the empty grand-base subobject dtor folds the re-stamp last. /GX EH frame.
RVA(0x001574d0, 0x5b)
CDDrawSubMgrPages::~CDDrawSubMgrPages() {
    DestroyChildren();
    m_04 = -1;
    m_08 = 0;
    m_0c = 0;
    // implicit ~CWapObj -> ~CObject folds the grand-base re-stamp (0x5e8cb4) last.
}

// CDDrawSubMgrLeafScan::ScalarDtor - the slot-1 `??_G` scalar-deleting dtor (0x157550):
// run the real ~CDDrawSubMgrLeafScan (direct call), conditionally RezFree, return this.
// Hand-written non-virtual + RVA pin (the CFileImageSurface::ScalarDelete pattern).
RVA(0x00157550, 0x1e)
void* CDDrawSubMgrLeafScan::ScalarDtor(u32 flags) {
    this->CDDrawSubMgrLeafScan::~CDDrawSubMgrLeafScan();
    if (flags & 1) {
        ::operator delete(this);
    }
    return this;
}

// ---------------------------------------------------------------------------
// 0x157570: the (non-deleting) destructor. Now a real virtual dtor: cl stamps
// ??_7CDDrawSubMgrLeafScan (masks g_leafScanVtbl @0x5efca0) at entry, runs the VM18
// cleanup (clears the map + zeroes +0x2c), the +0x10 map's own destructor, then the
// LeafScanBase grand-base teardown (field resets + implicit ??_7-base re-stamp masking
// 0x5e8cb4). No manual `m_vptr = &g_*Vtbl`. /GX EH frame (VM18 / map dtor may throw).
// @early-stop
// vptr-position wall (~95%, twin of CDDrawWorker/CDDrawSubMgrLeaf): every code
// byte matches retail EXCEPT the grand-base re-stamp position (cl emits it before the
// m_04/m_08/m_0c resets; the implicit base transition forces stamp-first, retail sinks
// it after) + the reloc-masked EH unwind / VM18 / ~CMapStringToPtr / vtable symbol
// names. objdiff-reloc-scoring.
RVA(0x00157570, 0x68)
CDDrawSubMgrLeafScan::~CDDrawSubMgrLeafScan() {
    // VM18 (0x157ae0) is slot [7] of this class's own vtable (ClearContext); a
    // virtual call on `this` inside the dtor devirtualizes to the retail direct
    // rel32, so no view cast is needed.
    ClearContext();
    // m_10 (CMapStringToPtr) member dtor auto-fires here, then the LeafScanBase
    // destructor resets +0x04/+0x08/+0x0c and restamps the grand-base vtable.
}

// CDDrawChildGroup::IsLoaded (0x1575e0, vtable slot 5 - the CWapObj IsLoaded
// override): loaded unless parent-less or in the error (m_status == -1) state.
// (Referenced ONLY from ??_7CDDrawChildGroup+0x14.)
RVA(0x001575e0, 0x16)
i32 CDDrawChildGroup::IsLoaded() {
    if (m_parent == 0 || m_status == -1) {
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CDDrawChildGroup::GetStateId (0x157600, vtable slot 8): the child group's state
// id. (Was misbound to CDDrawWorkerMapSmall: 0x157600's ONLY reference in the
// binary is ??_7CDDrawChildGroup+0x20; MapSmall's real slot 8 is 0x156cf0,
// `mov eax,0x14; ret` - a Ghidra recovery gap, unreconstructed.)
RVA(0x00157600, 0x6)
StateId CDDrawChildGroup::GetStateId() {
    return STATE_CHILDGROUP; // 0x10
}

// 0x157610: the ??_G scalar-deleting destructor of the sibling
// CDDrawChildGroupDtorHost (vtable 0x5efdc0). Runs the real member-teardown ~
// (0x157630, above) then, under the low deleting-flag bit, RezFree(this).
RVA(0x00157610, 0x1e)
void* CDDrawChildGroupDtorHost::ScalarDtor(u32 flags) {
    this->CDDrawChildGroupDtorHost::~CDDrawChildGroupDtorHost();
    if (flags & 1) {
        ::operator delete(this);
    }
    return this;
}

// ---------------------------------------------------------------------------
// 0x157630: member-teardown ~ of the 3-map sibling CDDrawChildGroupDtorHost
// (vtable 0x5efdc0). Runs the cleanup helper (0x1591e0), then the three
// CMapStringToPtr members and the FamilyMapBase grand-base auto-destruct (reverse
// decl order + field resets + the grand-base ??_7 re-stamp). /GX frame.
// @early-stop
// vptr-position wall (~95%, family twin): every instruction matches retail except
// the grand-base vptr re-stamp position (cl stamps at base-dtor entry, retail
// sinks it after the field resets) + the reloc-masked EH-state/teardown names.
RVA(0x00157630, 0x82)
CDDrawChildGroupDtorHost::~CDDrawChildGroupDtorHost() {
    ((CDDrawChildGroup*)this)->CDDrawChildGroup::ForwardTo3C();
    // implicit: ~m_48, ~m_2c, ~m_10, ~FamilyMapBase (resets + base restamp).
}

// CDDrawChildGroup::IsReady (0x1576c0, vtable slot 6 - the class compiles its OWN
// `return 1` copy of the CWapObj default; no MSVC5 ICF, so it gets its own body).
// (Was misbound as "CDDrawSubMgr::OnDestroy": 0x1576c0's ONLY reference in the
// binary is ??_7CDDrawChildGroup+0x18 - CDDrawSubMgr's own slot 6 is the shared
// 0x001c08 default thunk.)
RVA(0x001576c0, 0x6)
i32 CDDrawChildGroup::IsReady() {
    return 1;
}

// ---------------------------------------------------------------------------
// CDDrawWorkerCache dtor (0x157720; ??_G pin 0x157700).
// Scalar-deleting destructor: run the real member-teardown ~, then operator
// delete this if the low flag bit is set. SYMBOL() pins the ??_G mangling.
// @rva-symbol: ??_GCDDrawWorkerCache@@UAEPAXI@Z 0x00157700 0x1e

// The real member-teardown destructor (0x157720, /GX): cl stamps
// ??_7CDDrawWorkerCache (masks 0x5efd00) at entry, runs the map teardown (the
// shared DestroyAll @0x165210), then destructs the CMapStringToPtr member and the
// grand-base. /GX member-teardown frame from the destructible map.
// @early-stop
// vptr-position wall (~95%, twin of CDDrawSubMgrLeaf/CDDrawWorker): every
// instruction matches retail EXCEPT the grand-base vptr re-stamp POSITION + the
// reloc-masked EH-state/teardown/vtable symbol names. Logic complete.
RVA(0x00157720, 0x68)
CDDrawWorkerCache::~CDDrawWorkerCache() {
    DestroyAll();
    // implicit: ~m_10 (CMapStringToPtr), then the grand-base field resets + the
    // ??_7 re-stamp - reproduces retail's teardown order.
}

RVA(0x00157790, 0x6)
StateId CDDrawSubMgr::GetStateId() {
    return STATE_SUBMGR; // 1
}

// [5] 0x1577a0: leaf ready iff +0x0c is bound and the +0x04 status latch isn't -1.
RVA(0x001577a0, 0x16)
i32 CDDrawSubMgrLeaf::IsReady() {
    if (m_0c == 0) {
        goto fail;
    }
    if (m_04 != -1) {
        return 1;
    }

fail:
    return 0;
}

// Scalar-deleting destructor: COMPILER-GENERATED from the class's virtual ~.
// @rva-symbol: ??_GCDDrawSubMgrLeaf@@UAEPAXI@Z 0x001577c0 0x1e

// ---------------------------------------------------------------------------
// 0x1577e0 - ~CDDrawSubMgrLeaf (real ??1 body; the ??_G at 0x1577c0 calls it):
// real polymorphic teardown. cl stamps ??_7CDDrawSubMgrLeaf (masks 0x5efc78) at
// entry, runs the cleanup virtual, then the embedded map dtor and the
// CDDrawSubMgrGrandBase grand-base dtor. /GX EH frame.
// @early-stop
// vptr-position wall + reloc-masked EH-state push (~95%): byte-identical to retail
// EXCEPT the grand-base re-stamp position + the entry `push <ehfuncinfo>` reloc
// operand. docs/patterns/eh-state-numbering-base.md.
RVA(0x001577e0, 0x68)
CDDrawSubMgrLeaf::~CDDrawSubMgrLeaf() {
    // retail's dtor calls the non-virtual map teardown (0x152720) DIRECTLY, not the
    // virtual Cleanup slot (0x152650, which merely tail-calls it) - bind to 0x152720.
    FreeAll_152720();
    // implicit: ~m_10 (CMapStringToPtr), then ~CDDrawSubMgrGrandBase (resets the three
    // header fields + restamps the base vtable) - reproduces retail's teardown order.
}

// ---------------------------------------------------------------------------
// The CFileMem ctor/dtor COMDAT pocket (0x157850-0x157a66): the F/G obj first
// constructs CFileMem (SnapshotChildren/Gap_156ad0 stamps), so the inline
// ctor/dtor COMDATs are kept HERE; the class's runtime core (SetName/Open/
// Ready/Read/Write @0x165e30+) lives in the T obj (dossier #15).
// CFileMemBase::CFileMemBase (0x157850): base sub-object ctor - cl auto-stamps
// the base vptr, then zero the two scalar fields + Empty the name. The CString
// member installs the MSVC5 EH unwind frame.
RVA(0x00157850, 0x54)
CFileMemBase::CFileMemBase() {
    m_4 = 0;
    m_8 = 0;
    m_name.Empty();
}

// ~CFileMemBase (0x1578b0) - base teardown.
// @early-stop
// EH-dtor virtual-dispatch wall (~89%): the base teardown logic is byte-faithful,
// but retail dispatches Reset as an absolute indirect through the base vtable
// slot 3 - a virtual dispatch inside a dtor that MSVC5 devirtualizes to a direct
// call from clean C++. (NOTE: homed into this multi-fn obj per the wave4-L
// partition; the old FileMemBaseDtor.cpp isolation note predicted a ~CFileMem
// re-pack crater - accepted, rehome doctrine.)
RVA(0x001578b0, 0x51)
CFileMemBase::~CFileMemBase() {
    Reset();
}

// GetName (slot 4, 0x157920): return a by-value copy of the stream's name.
RVA(0x00157920, 0x20)
CString CFileMemBase::GetName() {
    return m_name;
}

// CFileMem::~CFileMem (0x157980): cl stamps the derived vtable at entry, run
// Reset() (derived), destruct the inner CFileIO, call the base Reset(), then cl
// folds the base vtable restamp + the CString member dtor on unwind.
// @early-stop
// EH-dtor scheduling wall (~59%): the teardown logic is byte-faithful, but the
// virtual-dtor auto vtable restamps + the /GX trylevel store sequencing + the
// member-dtor dispatch differ from retail's manual sequence.
RVA(0x00157980, 0x74)
CFileMem::~CFileMem() {
    Reset();
    m_file.~CFileIO();
    CFileMemBase::Reset();
}

// ---------------------------------------------------------------------------
// 0x157a80: pick the active cue object from m_worker->+0x20. When `force` is null,
// require the cue present and its +0x78 set; cache it at m_2c (m_30 = present?
// 0 : 1), tag the global cue, and report success. __thiscall, 1 arg (ret 0x4).
RVA(0x00157a80, 0x51)
i32 CDDrawBlitParam::SelectCue_157a80(void* force) {
    char* mgr = (char*)m_worker;
    if (mgr == 0) {
        return 0;
    }
    char* cue = *(char**)(mgr + 0x20);
    if (force == 0) {
        if (cue == 0) {
            return 0;
        }
        if (*(i32*)(cue + 0x78) == 0) {
            return 0;
        }
    }
    if (cue == 0) {
        m_30 = 1;
    } else {
        m_30 = 0;
    }
    m_2c = (i32)cue;
    g_sndCueTag = 0x64;
    return 1;
}

// ---------------------------------------------------------------------------
// 0x157ae0: CDDrawSubMgrLeafScan::ClearContext (slot [7] of ??_7CDDrawSubMgrLeafScan) -
// clear the keyed scan map then zero the parent handle. `this` is a CDDrawSubMgrLeafScan
// (it drives its own ClearMap + LeafScanBase m_0c), so the ~LeafScan dtor's devirtualized
// slot-7 call + the vtable slot both bind to this real RVA (was mis-attributed to
// CDDrawSubMgrLeaf, which left the dtor call reloc-unbound; the cast was a no-op).
RVA(0x00157ae0, 0x11)
void CDDrawSubMgrLeafScan::ClearContext() {
    ClearMap();
    m_0c = 0;
}

// ===========================================================================
// CSoundResMap::RemoveByValue (0x157b00): remove one map entry by its value
// pointer and delete the object (position-homed: the leaf/ani catalog IS the
// sound-res map neighborhood - dossier #15).
// ===========================================================================
// @early-stop
// Loop-rotation + EH-frame stack-slot wall (topic:regalloc; see
// docs/patterns/stack-slot-coalesce-frame-4b.md + loop-preheader-vs-exit-block-
// order.md). The logic + instruction selection are byte-identical to retail, but
// (a) retail keeps ONE loop body (post-tested do-while); our cl peels the first
// iteration into a second GetNextAssoc copy; (b) the {pos, key, value} stack
// slots are assigned differently. The find-one-and-stop break is what flips
// both. ~74.8%, logic complete; deferred to the final sweep.
RVA(0x00157b00, 0xb2)
void CSoundResMap::RemoveByValue(CSoundRes* p) {
    if (p == 0) {
        return;
    }
    POSITION pos = (POSITION)(m_map.GetCount() != 0 ? -1 : 0);
    CString key;
    void* value = 0;
    if (pos != (POSITION)0) {
        do {
            m_map.GetNextAssoc(pos, key, value);
            if (value == p) {
                m_map.RemoveKey(key);
                delete p;
                break;
            }
        } while (pos != (POSITION)0);
    }
}

// ---------------------------------------------------------------------------
// 0x157bc0: iterate every entry of the name-keyed map via GetNextAssoc, destroying
// each value through its scalar-deleting destructor (vtbl +0x4 arg 1), then
// RemoveAll. /GX EH frame for the local CString key.
// @early-stop
// store-scheduling coin-flip (~94%): byte-identical to retail except the `val = 0`
// store position + the reloc-masked EH-state push (same family wall as
// FreeAll_152720). docs/patterns/zero-register-pinning.md.
RVA(0x00157bc0, 0xa2)
void CDDrawSubMgrLeafScan::ClearMap() {
    void* val = 0;
    POSITION pos = (POSITION)(m_10.GetCount() != 0 ? -1 : 0);
    CString key;
    if (*(volatile i32*)&pos != 0) {
        do {
            m_10.GetNextAssoc(pos, key, val);
            if (val != 0) {
                delete ((CCatalogNode*)val);
            }
        } while (pos != 0);
    }
    m_10.RemoveAll();
}

// ---------------------------------------------------------------------------
// 0x157c70: remove every map entry whose key strncmp-equals `str` (over its full
// length), destroying each removed value via its scalar dtor; returns the count.
// The compare string is a CString built from `base` then assigned `str`. Twin of
// CDDrawWorkerRegistry::RemoveKeysEqual_155360.
RVA(0x00157c70, 0xf8)
i32 CDDrawSubMgrLeafScan::RemoveKeysEqual_157c70(const char* base, const char* str) {
    CString match(base);
    match = str;
    i32 len = match.GetLength();
    CString key;
    void* val = 0;
    POSITION pos = m_10.GetStartPosition();
    i32 n = 0;
    while (pos != 0) {
        m_10.GetNextAssoc(pos, key, val);
        if (strncmp(key, match, len) == 0) {
            m_10.RemoveKey(key);
            if (val != 0) {
                delete ((LeafScanValue*)val);
            }
            ++n;
        }
    }
    return n;
}

// ---------------------------------------------------------------------------
// 0x157d70: the 0x1c-byte cache-element factory. While not loading (m_30==0),
// allocate the element, stamp its vtable + seed it from the map count (m_10's
// m_nCount at this+0x1c) and the handle (this+0x0c), then run its Configure keyed
// by `arg2`. On Configure failure, destroy the element via its scalar dtor and
// return 0; on success link it into the map under `key` and stamp the redraw arg
// (this+0x34). 2 stack args (ret 8). Returns the element (or 0).
// @early-stop
// 99.81% — register-naming coin-flip: every code byte matches retail EXCEPT the
// ecx<->edx assignment for the two seed reads (count<-this+0x1c, handle<-this+0x0c).
// Retail pins count in ecx, handle in edx; MSVC5 here swaps them. Same values,
// same stores, same order; not source-steerable (tried count-first / handle-first /
// helper-extracted reads). docs/patterns/zero-register-pinning.md.
RVA(0x00157d70, 0x90)
LeafElementObj* CDDrawSubMgrLeafScan::CreateEntry_157d70(const char* key, void* arg2) {
    if (m_30 != 0) {
        return 0;
    }
    LeafElementObj* e = MakeLeafElement(this);
    if (e == 0) {
        return 0;
    }
    if (e->Configure_158760((CParseSource*)arg2) == 0) {
        delete e; // virtual scalar-deleting dtor (vtbl[1](1))
        return 0;
    }
    m_10[key] = e;
    e->m_18 = m_34; // +0x18 = redraw arg
    return e;
}

// ---------------------------------------------------------------------------
// 0x157e00: the second cache-element factory. Byte-for-byte twin of
// CreateEntry_157d70 except the element configure goes through the raw-RIFF
// Configure2 (0x158720) instead of the parsed Configure (0x158760): allocate +
// seed the element from the map count (this+0x1c) and handle (this+0x0c), run
// Configure2 keyed by `arg2`; on failure scalar-delete + return 0, on success
// link into the map under `key` + stamp the redraw arg (this+0x34). 2 args (ret 8).
// @early-stop
// register-naming coin-flip (twin of CreateEntry_157d70's 99.81%): every code byte
// matches retail EXCEPT the ecx<->edx assignment for the two seed reads. Same
// values/stores/order; not source-steerable. docs/patterns/zero-register-pinning.md.
RVA(0x00157e00, 0x90)
LeafElementObj* CDDrawSubMgrLeafScan::CreateEntry2_157e00(const char* key, void* arg2) {
    if (m_30 != 0) {
        return 0;
    }
    LeafElementObj* e = MakeLeafElement(this);
    if (e == 0) {
        return 0;
    }
    if (e->Configure2_158720(arg2) == 0) {
        delete e; // virtual scalar-deleting dtor (vtbl[1](1))
        return 0;
    }
    m_10[key] = e;
    e->m_18 = m_34; // +0x18 = redraw arg
    return e;
}

// ---------------------------------------------------------------------------
// 0x157e90: create a cache element from a parse source, keyed by its name. While
// not loading (m_30==0) and the source is non-null, run CreateEntry keyed by
// src->m_name with src as the parse-source arg. 1 stack arg (ret 4).
RVA(0x00157e90, 0x23)
LeafElementObj* CDDrawSubMgrLeafScan::AddFromSource_157e90(CParseSource* src) {
    if (m_30 != 0) {
        return 0;
    }
    if (src == 0) {
        return 0;
    }
    return CreateEntry_157d70(src->m_name, src);
}

// ---------------------------------------------------------------------------
// 0x157ec0: insert a pre-built element into the map under `key` (CMapStringToPtr::
// operator[]) and stamp its redraw arg (elem->m_18 = m_34). 2 stack args (ret 8).
RVA(0x00157ec0, 0x20)
void CDDrawSubMgrLeafScan::AddEntry_157ec0(LeafElementObj* elem, const char* key) {
    m_10[key] = elem;
    elem->m_18 = m_34;
}

// ---------------------------------------------------------------------------
// 0x157ee0: recursive directory walker. While not loading (m_30==0), allocate a
// 0x100-byte path buffer, then for each subdirectory of `tree` build the joined
// path (sprintf "%s%s%s" of prefix/suffix/name when prefix is non-empty, else a
// plain strcpy of the name) and recurse, summing the entry count. Then for each
// file, for each 'WAV'-tagged entry not already cached, build its path and create
// the cache element, counting successes. Frees the buffer and returns the count.
// 3 stack args (ret 0xc).
RVA(0x00157ee0, 0x1c6)
i32 CDDrawSubMgrLeafScan::ScanTree_157ee0(CSymTab* tree, const char* prefix, const char* suffix) {
    if (m_30 != 0) {
        return 0;
    }
    i32 count = 0;
    char* buf = (char*)operator new(0x100);
    if (buf == 0) {
        return 0;
    }
    buf[0] = 0;
    CSymTab* node = (CSymTab*)tree->FirstSub();
    while (node != 0) {
        if (prefix != 0 && *prefix != 0) {
            sprintf(buf, "%s%s%s", prefix, suffix, node->m_name);
        } else {
            strcpy(buf, node->m_name);
        }
        count += ScanTree_157ee0(node, buf, suffix);
        node = (CSymTab*)tree->NextSub(node);
    }
    // `file` stays void*: the outer leaf-table record has its next-link at +0x04 (NextSym)
    // and its entry chain at +0x24 (NextSym2) - neither offset is CParseSource's, so this
    // record's class is NOT proven here. Left honest rather than guessed.
    void* file = tree->FirstSym();
    if (file != 0) {
        do {
            CParseSource* fn = (CParseSource*)tree->NextSym2(file);
            while (fn != 0) {
                if (fn->GetEntryTag() == PARSETAG_VAW) {
                    if (prefix != 0 && *prefix != 0) {
                        sprintf(buf, "%s%s%s", prefix, suffix, fn->m_name);
                    } else {
                        strcpy(buf, fn->m_name);
                    }
                    void* val = 0;
                    m_10.Lookup(buf, val);
                    if (val == 0) {
                        if (CreateEntry_157d70(buf, fn) != 0) {
                            ++count;
                        }
                    }
                }
                fn = (CParseSource*)tree->NextSym3(fn);
            }
            file = tree->NextSym(file);
        } while (file != 0);
    }
    ::operator delete(buf);
    return count;
}

// ---------------------------------------------------------------------------
// 0x1580b0: sum each matching entry's count. While not loading (m_30==0), walk
// the map via GetNextAssoc; for each present value, when `str` is null/empty add
// its count unconditionally, else add it only when the key strncmp-matches `str`
// over strlen(str). Returns the accumulated count. /GX EH frame for the local key.
// @early-stop
// zero-register-pin wall (~70%): map-scan idiom applied (top-tested while + real
// GetStartPosition kills the peel, docs/patterns/mfc-map-walk-while-not-guard-
// dowhile.md), body/CFG/calls/args/offsets reproduced. Residue: retail pins 0 in
// ebx (xor ebx,ebx) and uses cmp ebx,X / cmpb bl,(esi) for all 7 null/zero checks
// where our cl emits test/cmp-imm - regalloc coin-flip, docs/patterns/zero-
// register-pinning.md. No source lever.
RVA(0x001580b0, 0xf6)
i32 CDDrawSubMgrLeafScan::SumField_1580b0(const char* str) {
    if (m_30 != 0) {
        return 0;
    }
    CString key;
    void* val = 0;
    POSITION pos = m_10.GetStartPosition();
    i32 sum = 0;
    while (pos != 0) {
        m_10.GetNextAssoc(pos, key, val);
        if (val != 0) {
            if (str == 0 || *str == 0) {
                sum += ((LeafSumSource*)((LeafScanValue*)val)->m_10)->m_2c;
            } else if (strncmp(key, str, strlen(str)) == 0) {
                sum += ((LeafSumSource*)((LeafScanValue*)val)->m_10)->m_2c;
            }
        }
    }
    return sum;
}
// 0x1581b0: fire the named CAniBlitTrigger from the cache, gated on the parent being
// live and the sub-manager not busy.
//
// The placeholder `CAniTriggerMap_1581b0` view is GONE - `this` is the canonical
// CDDrawSubMgrLeafScan. The old note said the owning class was "unrecovered" because
// 0x1581b0 has no caller in the image (dead/inlined-away), but the caller graph was
// never the evidence that mattered:
//   - the retail bytes read `[ecx+0x0c]` (LeafScanBase::m_0c, the parent/root handle),
//     `[ecx+0x30]` (m_30, the busy guard) and `add ecx,0x10; call 0x1b8438` - the
//     CMapStringToPtr::Lookup on m_10, at the very rva mfc_class pins to
//     CMapStringToPtr. That is CDDrawSubMgrLeafScan's layout, member for member.
//   - it is bracketed by this class's own methods: RemoveKeysEqual_157c70,
//     HasKeyEqual_1583c0, and GetFirstValue_158210 - which starts at 0x158210, i.e.
//     immediately after this body ends (0x1581b0 + 0x5b = 0x15820b) and dispatches on
//     the SAME m_10/m_30 members.
// A dead function still belongs to a class; here its own neighbours name it.
RVA(0x001581b0, 0x5b)
i32 CDDrawSubMgrLeafScan::Fire_1581b0(const char* key, i32 pos, i32 range1, i32 range2) {
    char* p24 = *(char**)((char*)m_0c + 0x24);
    if (p24 != 0 && *(char**)(p24 + 0x5c) != 0 && m_30 == 0) {
        void* val = 0;
        m_10.Lookup(key, val);
        if (val != 0) {
            return ((CAniBlitTrigger*)val)->TriggerBlit_1587f0(pos, -1, range1, range2);
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// 0x158210: return the first map value (or 0). While not loading (m_30==0) and
// the map is non-empty, a single GetNextAssoc reads the head entry's value.
// /GX EH frame for the local CString key. NB: the single-pass pos test must NOT
// be `volatile` (that forces a redundant reload retail omits): a plain `pos == 0`
// matches byte-for-byte. (The looping siblings DO need the volatile to keep the
// loop-carried pos in a slot.)
RVA(0x00158210, 0xaa)
LeafScanValue* CDDrawSubMgrLeafScan::GetFirstValue_158210() {
    if (m_30 != 0) {
        return 0;
    }
    POSITION pos = (POSITION)(m_10.GetCount() != 0 ? -1 : 0);
    if (pos == 0) {
        return 0;
    }
    void* val = 0;
    CString key;
    m_10.GetNextAssoc(pos, key, val);
    return (LeafScanValue*)val;
}

// ---------------------------------------------------------------------------
// 0x1582c0: find the map entry whose VALUE POINTER equals `target`, and return the
// value of the entry that IMMEDIATELY FOLLOWS it in iteration order. Returns 0 when
// target is null, the map is busy (m_30) or empty, target is not found, or the match
// was the last entry. Guarded like GetFirstValue (m_30 + GetCount()!=0?-1:0 start
// position); the second GetNextAssoc reads the successor value. /GX EH frame for key.
RVA(0x001582c0, 0xf6)
LeafScanValue* CDDrawSubMgrLeafScan::NextValueAfter_1582c0(LeafScanValue* target) {
    if (target == 0) {
        return 0;
    }
    if (m_30 != 0) {
        return 0;
    }
    POSITION pos = (POSITION)(m_10.GetCount() != 0 ? -1 : 0);
    if (pos == 0) {
        return 0;
    }
    void* val = 0;
    CString key;
    while (pos != 0) {
        m_10.GetNextAssoc(pos, key, val);
        if (val == (void*)target) {
            if (pos == 0) {
                return 0;
            }
            val = 0;
            m_10.GetNextAssoc(pos, key, val);
            return (LeafScanValue*)val;
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// 0x1583c0: return 1 if any map key strncmp-equals `str` over strlen(str), else
// 0. Twin of CDDrawWorkerRegistry::HasKeyEqual_155550. /GX EH frame for the local
// CString key.
RVA(0x001583c0, 0xdc)
i32 CDDrawSubMgrLeafScan::HasKeyEqual_1583c0(const char* str) {
    i32 len = strlen(str);
    CString key;
    void* val = 0;
    POSITION pos = m_10.GetStartPosition();
    while (pos != 0) {
        m_10.GetNextAssoc(pos, key, val);
        if (strncmp(key, str, len) == 0) {
            return 1;
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// 0x1584a0: probe the first cached value against the held DSound device. When a
// device is held (m_2c) and the first map value (GetFirstValue) carries a held
// sound-arg (its m_10), forward it through MatchSub with `arg`; return whether
// MatchSub succeeded (the !=0 normalized to 1). 1 stack arg (ret 4).
RVA(0x001584a0, 0x43)
i32 CDDrawSubMgrLeafScan::ProbeFirst_1584a0(i32 arg) {
    if (m_2c == 0) {
        return 0;
    }
    LeafScanValue* val = GetFirstValue_158210();
    if (val == 0) {
        return 0;
    }
    // Retail reads val->m_10 only to null-check it, then passes `val` itself to
    // MatchSub (whose arg1->m_10 reaches the same held sound-arg).
    if (val->m_10 == 0) {
        return 0;
    }
    return MatchSub_1584f0((LeafScanSoundArg*)val, arg) != 0;
}

// ---------------------------------------------------------------------------
// 0x1584f0: if `arg1` and the held DSound manager (m_2c) are both present, probe
// arg1's sound source for format 0x12 (vtable +0x14), then Prepare the manager,
// then -- only when arg2 is set -- Commit it. Returns 1 on full success; the
// failing sub-result (0) otherwise. arg1==0 returns arg1 (0).
RVA(0x001584f0, 0x80)
i32 CDDrawSubMgrLeafScan::MatchSub_1584f0(LeafScanSoundArg* arg1, i32 arg2) {
    if (arg1 == 0) {
        return (i32)arg1;
    }
    if (m_2c == 0) {
        return 0;
    }
    char fmt[0x12]; // WAVEFORMATEX scratch (0x12 = 18 bytes)
    if (arg1->m_10->GetFormat(fmt, 0x12, 0) == 0) {
        return 0;
    }
    i32 prep;
    if (m_2c->SetPrimaryFormat(&prep) == 0) {
        return 0;
    }
    if (arg2 != 0) {
        if (m_2c->StartPrimary() == 0) {
            return 0;
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// 0x158570: return (by value) the key of the first map entry whose VALUE POINTER
// equals `target`; the (empty) key if `target` is null or no entry matches.
// Returns the running `key` CString in every exit (NRVO into the return slot).
// Closed by the map-scan idiom (top-tested while + real GetStartPosition) plus the
// key.Empty() before the final return that retail emits on the no-match tail.
RVA(0x00158570, 0xd4)
CString CDDrawSubMgrLeafScan::FindKeyOfValue_158570(LeafScanValue* target) {
    CString key;
    if (target == 0) {
        return key;
    }
    void* val = 0;
    POSITION pos = m_10.GetStartPosition();
    while (pos != 0) {
        m_10.GetNextAssoc(pos, key, val);
        if (val == (void*)target) {
            return key;
        }
    }
    key.Empty();
    return key;
}

SIZE_UNKNOWN(LeafCuePlayer);
SIZE_UNKNOWN(LeafElementBase);
SIZE(LeafElementObj, 0x1c);
SIZE_UNKNOWN(LeafRootHandle);
// LeafScanBase / CDDrawSubMgrLeafScan SIZE_UNKNOWN now live in the shared header.
SIZE_UNKNOWN(LeafScanSoundArg);
SIZE_UNKNOWN(LeafScanValue);
SIZE_UNKNOWN(LeafSumSource);
// ??_7LeafElementObj (was g_leafElemVtbl @0x5eff08, LeafElemVtbl / the LeafElementObj vtable).
// cl auto-emits it from the real-polymorphic element; retail's 9-slot datum is
// reloc-masked -> matching-neutral catalog tracking.
VTBL(LeafElementObj, 0x001eff08);

// --- vtable catalog (reduced-view classes share their base vtable rva) ---

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---

// ---------------------------------------------------------------------------
// 0x158680: ~LeafElementObj (the non-deleting destructor). cl auto-stamps the
// element's own vtable (??_7LeafElementObj) at entry, runs Release (frees the
// acquired buffer), then chains the base teardown: reset +0x04/+0x08/+0x0c and the
// implicit grand-base re-stamp (??_7LeafElementBase, masks 0x5e8cb4). /GX EH frame --
// Release runs while the base subobject is still live, so its teardown is unwind-
// protected (the half-destructed-element cleanup edge).
// @early-stop
// EH-state/funclet plateau (docs/seh-eh.md): the vtable is now cl-emitted (real
// polymorphic, ALL-VTABLES mandate) so the own-vptr + grand-base stamps are compiler
// implicit; residual is the /GX EH unwind-map index + the one-position schedule of the
// EH-state store + the reloc-masked ??_7/handler symbol names (pair against differently
// -named retail symbols at the SAME addresses). Logic complete.
RVA(0x00158680, 0x5b)
LeafElementObj::~LeafElementObj() {
    Release_1587c0();
    // cl auto-stamps ??_7LeafElementObj at entry; ~LeafElementBase auto-fires here:
    // reset +0x04/+0x08/+0x0c + implicit grand-base re-stamp (masks 0x5e8cb4).
}

// ---------------------------------------------------------------------------
// 0x158760: LeafElementObj::Configure. Parse the draw-source for its RIFF/WAVE
// blob; if the parse failed, fail. Otherwise, when the parent root handle's
// SoundDevice is up, acquire a buffer for the blob into m_10. EndParse always
// runs; returns whether a buffer was acquired (0 when the device is down).
// 1 stack arg (ret 4).
// @early-stop
// 41% -- regalloc-pinning wall (docs/patterns/zero-register-pinning.md): the CFG,
// all three calls (BeginParse/Acquire/EndParse), all field stores, and the result
// merge are reproduced. MSVC5 homes the `src` param into a 3rd callee-saved register
// (ebx) and carries the return value differently than retail (which pins this->esi,
// src->edi and reuses esi as the return carrier, computing ok eagerly before
// EndParse). Tried 3 result/store spellings; no source lever flips the homing. Logic complete.
RVA(0x00158760, 0x59)
i32 LeafElementObj::Configure_158760(CParseSource* src) {
    i32 blob = src->BeginParse();
    if (blob == 0) {
        return 0;
    }
    SoundDevice* dev = ((LeafRootHandle*)m_0c)->m_20;
    if (dev == 0) {
        src->EndParse();
        return 0;
    }
    DirectSoundMgr* buf = dev->Acquire((void*)blob, 0x100ea, 0);
    m_10 = (i32)buf;
    i32 ok = buf != 0;
    src->EndParse();
    return ok;
}

// ---------------------------------------------------------------------------
// 0x1587c0: LeafElementObj::Release. When a buffer is held and the root handle's
// SoundDevice is still up, remove the buffer through the device (reaps voices +
// releases + unlinks + scalar-deletes), then clear the held pointer. __thiscall,
// no args.
RVA(0x001587c0, 0x23)
void LeafElementObj::Release_1587c0() {
    if (m_10 != 0) {
        SoundDevice* dev = ((LeafRootHandle*)m_0c)->m_20;
        if (dev != 0) {
            dev->RemoveBuffer((SoundBuf*)m_10);
            m_10 = 0;
        }
    }
}

// ---------------------------------------------------------------------------
// 0x1587f0: per-frame sound-cue trigger. Defaults the (center,range1,range2)
// triple from the geometry context when non-positive, clamps the signed offset
// (pos-center) to +/-min(range1,range2), scales it to a [-100,100] pan, derives
// the volume, and hands both to the +0x10 sound player. __thiscall, 4 args
// (ret 0x10). No-op (0) when sound is disabled.
// @early-stop
// 72% - logic/CFG/offsets/stack-arg flow are instruction-for-instruction identical
// to retail; the entire residual is a register-allocation rotation: retail pins
// `this` in a 4th callee-saved register (ebp) and keeps the quad in
// ebx/edi/esi/ecx, our cl reuses ebx for `this` and rotates the quad - flipping
// the ModRM byte of nearly every access. No source lever picks ebp for `this`
// (docs/patterns/zero-register-pinning.md).
RVA(0x001587f0, 0xf1)
i32 CAniBlitTrigger::TriggerBlit_1587f0(i32 pos, i32 center, i32 range1, i32 range2) {
    if (g_sndEnabled == 0) {
        return 0;
    }
    if (center <= 0) {
        center = *(i32*)(*(char**)(*(char**)((char*)m_ctx + 0x24) + 0x5c) + 0x84);
    }
    if (range1 <= 0) {
        char* m4 = *(char**)((char*)m_ctx + 0x4);
        range1 = *(i32*)(*(char**)(m4 + 0x10) + 0x10) << 2;
    }
    if (range2 <= 0) {
        char* m4 = *(char**)((char*)m_ctx + 0x4);
        range2 = *(i32*)(*(char**)(m4 + 0x10) + 0x10) / 3;
    }
    i32 d = pos - center;
    i32 pan;
    if (d >= 0) {
        if (d < range1 && d < range2) {
            pan = d;
        } else {
            pan = range1 >= range2 ? range2 : range1;
        }
    } else {
        i32 ad = -d;
        if (ad < range1 && ad < range2) {
            pan = d;
        } else {
            pan = range1 < range2 ? range1 : range2;
            pan = -pan;
        }
    }
    i32 vol = (pan * 100) / range2;
    i32 cue = g_sndCueTag;
    i32 amp = 100;
    i32 vscale;
    if (cue == 100) {
        vscale = amp;
    } else {
        vscale = (i32)(amp * (cue * g_sndPanScale));
    }
    return m_soundPlayer->ConfigureItem(vscale, vol, 0, 0);
}

// slot 9 (CreateChildren, 0x1588f0): build the three owned children then run
// their per-stage init; on any failure stamp the root's m_lastError
// (0x7d1/0x7d2/0x7d3 if not already set) and return 0. /GX EH frame.
// @early-stop
// vptr-position / worker-ctor-shape wall: retail stamps each child's derived
// vtable AFTER the base ctor + field seeds (vptr-last); the placement `new`
// model stamps vptr-first. Logic/CFG/offsets/error-codes reproduced.
RVA(0x001588f0, 0x1c5)
i32 CDDrawSubMgrPages::CreateChildren(i32 a1, i32 a2, i32 a3, i32 a4) {
    CDDrawSurfaceChildA* a = (CDDrawSurfaceChildA*)operator new(0x30);
    if (a != 0) {
        new (a) CDDrawSurfaceChildA((i32)m_0c, 0, 0);
        a->m_surface = 0;
    }
    m_frontPair = (CDDrawSurfacePair*)a;

    CDDrawSurfacePair* b = (CDDrawSurfacePair*)operator new(0x34);
    if (b != 0) {
        new (b) CDDrawSurfacePair((i32)m_0c, 1, 0);
        b->m_width = 0;
        b->m_surface = 0;
        b->m_ownsSurface = 1;
    }
    m_backPair = b;

    CDDrawSurfacePair* c = (CDDrawSurfacePair*)operator new(0x34);
    if (c != 0) {
        new (c) CDDrawSurfacePair((i32)m_0c, 2, 0);
        c->m_width = 0;
        c->m_surface = 0;
        c->m_ownsSurface = 1;
    }
    m_overlayPair = c;

    if (a->CreateModeSurface_1644a0(a1, a2, a3) == 0) {
        if (m_0c->m_lastError == 0) {
            m_0c->m_lastError = 0x7d1;
        }
        return 0;
    }
    if (b->Create(a1, a2, a3, 0) == 0) {
        if (m_0c->m_lastError == 0) {
            m_0c->m_lastError = 0x7d2;
        }
        return 0;
    }
    if (!(a4 & 1)) {
        if (c->Create(a1, a2, a3, 0) == 0) {
            if (m_0c->m_lastError == 0) {
                m_0c->m_lastError = 0x7d3;
            }
            return 0;
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CDDrawSurfaceChildA / CDDrawSubMgrPages children (0x1588f0-0x1591b0).
// slot 7 (DestroyChildren, 0x158ac0): delete each owned child + null the slot.
RVA(0x00158ac0, 0x44)
void CDDrawSubMgrPages::DestroyChildren() {
    if (m_frontPair != 0) {
        delete m_frontPair;
        m_frontPair = 0;
    }
    if (m_backPair != 0) {
        delete m_backPair;
        m_backPair = 0;
    }
    if (m_overlayPair != 0) {
        delete m_overlayPair;
        m_overlayPair = 0;
    }
}

// ===========================================================================
// CDDrawSubMgrPages surface ops (0x158b10-0x158ee0): the front/back/overlay
// surface-pair dispatch cluster.
// ===========================================================================

// 0x158b10: pick m_overlayPair (arg2==2) or m_backPair, null-check, dispatch slot 0x38
// (ResolveImage) with arg1. Twin of Method_158b40 (which dispatches slot 0x34).
RVA(0x00158b10, 0x2c)
i32 CDDrawSubMgrPages::Method_158b10(i32 arg1, i32 arg2) {
    CDDrawSurfacePair* p;
    if (arg2 == 2) {
        p = m_overlayPair;
        if (!p) {
            return 0;
        }
    } else {
        p = m_backPair;
        if (!p) {
            return 0;
        }
    }
    return p->ResolveImage_163ee0((CParseSource*)arg1);
}

// 0x158b40: pick m_overlayPair (arg2==2) or m_backPair, null-check, dispatch slot 0x34.
RVA(0x00158b40, 0x2c)
i32 CDDrawSubMgrPages::Method_158b40(i32 arg1, i32 arg2) {
    CDDrawSurfacePair* p;
    if (arg2 == 2) {
        p = m_overlayPair;
        if (!p) {
            return 0;
        }
    } else {
        p = m_backPair;
        if (!p) {
            return 0;
        }
    }
    return p->LoadImage_163e50((CParseSource*)arg1);
}

// 0x158b90: flip m_frontPair's surface, then broadcast (back-pair, overlay-pair)
// through the parent's +0x08 child-group dispatcher (WalkDispatch30, slot 0x2c).
RVA(0x00158b90, 0x28)
void CDDrawSubMgrPages::Method_158b90() {
    m_frontPair->m_surface->Flip(0);
    CDDrawSurfaceMgr* n = m_0c;
    CDDrawChildGroup* c = n->m_childGroup;
    CDDrawSubMgrPages* s = n->m_pages;
    c->WalkDispatch30((i32)s->m_backPair, (i32)s->m_overlayPair);
}

// 0x158bc0: ready predicate over m_frontPair (Probe_164660) and m_overlayPair
// (RestoreIfLost).
RVA(0x00158bc0, 0x2e)
i32 CDDrawSubMgrPages::Method_158bc0() {
    if (m_frontPair && !m_frontPair->Probe_164660()) {
        return 0;
    }
    if (m_overlayPair && !m_overlayPair->RestoreIfLost()) {
        return 0;
    }
    return 1;
}

// 0x158bf0: if m_frontPair's cached geometry already == (a1,a2,a3) return 1; else set
// geometry on m_frontPair, m_backPair, and (if ready) m_overlayPair.
RVA(0x00158bf0, 0x7f)
i32 CDDrawSubMgrPages::Method_158bf0(i32 a1, i32 a2, i32 a3) {
    CDDrawSurfacePair* p = m_frontPair;
    if (p->m_width != a1 || p->m_height != a2 || p->m_bpp != a3) {
        if (!m_frontPair->SetGeom_164250(a1, a2, a3)) {
            return 0;
        }
        if (!m_backPair->SetGeom_164250(a1, a2, a3)) {
            return 0;
        }
        if (m_overlayPair && m_overlayPair->IsLoaded()) {
            if (!m_overlayPair->SetGeom_164250(a1, a2, a3)) {
                return 0;
            }
        }
    }
    return 1;
}

// 0x158c70: blt dst's surface <- m_frontPair's surface; return (hr == 0).
RVA(0x00158c70, 0x36)
i32 CDDrawSubMgrPages::Method_158c70(CDDrawSurfacePair* dst) {
    if (!m_frontPair) {
        return 0;
    }
    CDDSurface* s = m_frontPair->m_surface;
    if (!s) {
        return 0;
    }
    CDDSurface* d = dst->m_surface;
    if (!d) {
        return 0;
    }
    i32 hr = d->Blt(s);
    return hr == 0;
}

// 0x158cb0: if m_overlayPair is ready, bail; else copy m_backPair's geometry into
// m_overlayPair via slot 0x30 and (if a2) BltFast m_backPair's surface into it.
RVA(0x00158cb0, 0x6a)
i32 CDDrawSubMgrPages::Method_158cb0(i32 a1, i32 a2) {
    if (m_overlayPair->IsLoaded()) {
        return 0;
    }
    CDDrawSurfacePair* s14 = m_backPair;
    if (!m_overlayPair->Create(s14->m_width, s14->m_height, s14->m_bpp, a2)) {
        return 0;
    }
    if (a1) {
        m_overlayPair->m_surface->BltFast(0, 0, m_backPair->m_surface, m_backPair->m_srcRect, 0x10);
    }
    return 1;
}

// 0x158d20: return m_overlayPair->IsLoaded() != 0.
RVA(0x00158d20, 0x16)
i32 CDDrawSubMgrPages::Method_158d20() {
    if (!m_overlayPair) {
        return 0;
    }
    return m_overlayPair->IsLoaded() != 0;
}

// 0x158d50: fill m_backPair's surface and flip m_frontPair's, twice unconditionally,
// then once more if the node's +0x34 flag bit1 is set.
RVA(0x00158d50, 0x61)
void CDDrawSubMgrPages::Method_158d50(i32 a1) {
    m_backPair->m_surface->Fill(a1);
    m_frontPair->m_surface->Flip(0);
    m_backPair->m_surface->Fill(a1);
    m_frontPair->m_surface->Flip(0);
    if (m_0c->m_flags & 2) {
        m_backPair->m_surface->Fill(a1);
        m_frontPair->m_surface->Flip(0);
    }
}

// 0x158dc0: blt m_backPair's surface <- m_frontPair's surface; if the m_worker flag
// bit1 is set, flip m_frontPair and re-blt.
// @early-stop
// 71% - logic + offsets exact; residual is branch-layout/scheduling of the
// first-block (Blt-fall-through vs our jmp) and the esi-pop placement, a
// regalloc/scheduling wall (see docs/patterns/zero-register-pinning.md).
RVA(0x00158dc0, 0x7d)
i32 CDDrawSubMgrPages::Method_158dc0() {
    CDDrawSurfacePair* p10 = m_frontPair;
    CDDrawSurfacePair* p14 = m_backPair;
    i32 ok;
    if (p10 && p10->m_surface) {
        CDDSurface* s10 = p10->m_surface;
        CDDSurface* s14 = p14->m_surface;
        if (s14) {
            i32 hr = s14->Blt(s10);
            ok = (hr == 0);
        } else {
            ok = 0;
        }
    } else {
        ok = 0;
    }
    if (!ok) {
        return ok;
    }
    if (!(m_0c->m_flags & 2)) {
        return ok;
    }
    m_frontPair->m_surface->Flip(0);
    CDDrawSurfacePair* a = m_backPair;
    CDDrawSurfacePair* b = m_frontPair;
    if (!b) {
        return 0;
    }
    CDDSurface* bs = b->m_surface;
    if (!bs) {
        return 0;
    }
    if (!a->m_surface) {
        return 0;
    }
    i32 hr2 = bs->Blt(a->m_surface);
    return hr2 == 0;
}

// 0x158e40: if m_overlayPair->IsLoaded(): blt m_overlayPair's surface <-
// m_frontPair's surface, return (==0).
// @early-stop
// 50% - structure/offsets byte-exact; the only residual is the `pop esi`
// scheduling (retail interleaves it before the test/sete; our cl emits it in
// the epilogue), a scheduling coin-flip (docs/patterns/zero-register-pinning.md).
RVA(0x00158e40, 0x4c)
i32 CDDrawSubMgrPages::Method_158e40() {
    if (m_overlayPair && m_overlayPair->IsLoaded()) {
        CDDrawSurfacePair* a = m_overlayPair;
        CDDrawSurfacePair* b = m_frontPair;
        if (!b) {
            return 0;
        }
        CDDSurface* bs = b->m_surface;
        if (!bs) {
            return 0;
        }
        CDDSurface* as = a->m_surface;
        if (!as) {
            return 0;
        }
        i32 hr = as->Blt(bs);
        return hr == 0;
    }
    return 0;
}

// 0x158e90: if m_backPair and m_overlayPair->IsLoaded(): BltFast(0,0,m_backPair
// surf, &m_backPair[+0x1c], 0x10).
RVA(0x00158e90, 0x47)
i32 CDDrawSubMgrPages::Method_158e90() {
    if (!m_backPair) {
        return 0;
    }
    if (!m_overlayPair) {
        return 0;
    }
    if (!m_overlayPair->IsLoaded()) {
        return 0;
    }
    CDDrawSurfacePair* a = m_backPair;
    CDDrawSurfacePair* b = m_overlayPair;
    b->m_surface->BltFast(0, 0, a->m_surface, a->m_srcRect, 0x10);
    return 1;
}

// 0x158ee0: if m_backPair, m_overlayPair and m_overlayPair->IsLoaded():
// BltFast(0,0,m_overlayPair surf,&m_overlayPair[+0x1c],0x10).
RVA(0x00158ee0, 0x47)
i32 CDDrawSubMgrPages::Method_158ee0() {
    if (!m_backPair) {
        return 0;
    }
    if (!m_overlayPair) {
        return 0;
    }
    if (!m_overlayPair->IsLoaded()) {
        return 0;
    }
    CDDrawSurfacePair* a = m_overlayPair;
    CDDrawSurfacePair* b = m_backPair;
    b->m_surface->BltFast(0, 0, a->m_surface, a->m_srcRect, 0x10);
    return 1;
}

// ===========================================================================
// The CDrawSubWorker leaf (0x158f30-0x159020).
// ===========================================================================

// 0x158f30: 3-arg leaf-worker ctor - store the three args at +0x4/+0x8/+0xc,
// stamp the leaf vtable (cl-implicit vptr-first), zero +0x10. __thiscall, ret 0xc.
struct CDrawSubWorker : public CObject { // CObject slots 0-4 inherited
    virtual ~CDrawSubWorker() OVERRIDE;  // slot 1 (own dtor override; reloc-masked)
    i32 m_04;                            // +0x04
    i32 m_08;                            // +0x08
    i32 m_0c;                            // +0x0c
    i32 m_width;                         // +0x10 (zeroed in ctor)
    i32 m_height;                        // +0x14
    i32 m_bpp;                           // +0x18
    i32 m_srcRect[4];                    // +0x1c..+0x28
    CDrawSubWorker(i32 a1, i32 a2, i32 a3);
    virtual void VtSlotFill0(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill1(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill2(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill3(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill4(); // slot 9 (0x158fd0, ICF w/ CDDrawSurfacePair; declared-only)
    virtual i32 SetGeom_159020(i32 w, i32 h, i32 bpp); // slot 10 (@0x28) 0x159020
};
SIZE_UNKNOWN(CDrawSubWorker);
RVA(0x00158f30, 0x27)
CDrawSubWorker::CDrawSubWorker(i32 a1, i32 a2, i32 a3) {
    m_04 = a2;
    m_08 = a3;
    m_0c = a1;
    m_width = 0;
}
VTBL(CDrawSubWorker, 0x001effa0); // ??_7CDrawSubWorker (was g_drawSubWorkerVtbl)

// 0x158fb0: DDraw worker base re-init - +0x4 = -1, +0x8/+0xc/+0x10 = 0, stamp
// the base vtable. A void method (keeps `this` in ecx; not a ctor). ret 0.
class CDrawSubWorkerBase : public CObject {
public:
    i32 m_04; // +0x04
    i32 m_08; // +0x08
    i32 m_0c; // +0x0c
    i32 m_10; // +0x10
    void Init_158fb0();
};
SIZE_UNKNOWN(CDrawSubWorkerBase);
RVA(0x00158fb0, 0x19)
void CDrawSubWorkerBase::Init_158fb0() {
    m_04 = -1;
    m_10 = 0;
    m_08 = 0;
    m_0c = 0;
    // base vptr auto-stamped via CObject (manual stamp dropped, % ok)
}

// ---------------------------------------------------------------------------
// CDDrawSurfacePair G-heads (0x158fd0-0x1590f0): the pair's quartet bits kept in
// this obj; the pair's meat lives in the T obj.
// 0x158fd0 (slot 9): SetGeometry - cache the {w,h,bpp} pixel geometry and a
// {0,0,w,h} src rect. Shared body (ICF) with CDrawSubWorker slot 9.
// @early-stop
// 83.86% - regalloc coin-flip: retail materializes bpp up-front into edi; the
// /O2 scheduler on identical source keeps bpp in eax and loads it lazily.
RVA(0x00158fd0, 0x41)
i32 CDDrawSurfacePair::SetGeometry_158fd0(i32 w, i32 h, i32 bpp) {
    if (w <= 0 || h <= 0) {
        return 0;
    }
    m_height = h;
    m_srcRect[3] = h;
    m_width = w;
    m_bpp = bpp;
    m_srcRect[0] = 0;
    m_srcRect[1] = 0;
    m_srcRect[2] = w;
    return 1;
}

// 0x159020 (slot 10): SetGeom with bpp validation - cache {w,h,bpp} + a {0,0,w,h}
// src rect; reject non-positive w/h and bpp not in {8,16,24,32}. __thiscall, ret 0xc.
RVA(0x00159020, 0x55)
i32 CDrawSubWorker::SetGeom_159020(i32 w, i32 h, i32 bpp) {
    if (w <= 0 || h <= 0) {
        return 0;
    }
    if (bpp != 8 && bpp != 16 && bpp != 24 && bpp != 32) {
        return 0;
    }
    m_height = h;
    m_srcRect[3] = h;
    m_width = w;
    m_bpp = bpp;
    m_srcRect[0] = 0;
    m_srcRect[1] = 0;
    m_srcRect[2] = w;
    return 1;
}

// ===========================================================================
// wave4-L CP3b: the G-obj take-ins from the dissolved per-class files
// (mapsmall/list/workers/cache/pages/leafscan/filemem quartets + pocket +
// the surface-pair heads). Ordering pass follows (CP3c).
// ===========================================================================
// The engine block allocator + placement-new (child spawns / path buffers).
void* operator new(u32 n);
inline void* operator new(u32, void* p) {
    return p;
}

// 0x159090 (slot 5): IsLoaded - the surface-ready predicate.
RVA(0x00159090, 0x24)
i32 CDDrawSurfacePair::IsLoaded() {
    if (m_surface != 0 && m_width > 0 && m_mgr != 0 && m_status != -1) {
        return 1;
    }
    return 0;
}

// 0x1590f0: the (non-deleting) destructor. cl emits the implicit
// ??_7CDDrawSurfacePair own-vptr stamp in the ENTRY state (stamp-first), runs
// TeardownSurface, zeroes the width + the moved-down base fields, then the empty
// ~CSurfacePairBase folds the grand-base re-stamp last. /GX EH frame.
RVA(0x001590f0, 0x56)
CDDrawSurfacePair::~CDDrawSurfacePair() {
    TeardownSurface();
    m_width = 0;
    // base fields, moved out of ~CSurfacePairBase so they precede the grand-stamp:
    m_flags = 0;
    m_mgr = 0;
    m_status = -1;
    // empty ~CSurfacePairBase() folds the implicit grand-base re-stamp here, last.
}

// 0x159150 (slot 5): CDDrawSurfaceChildA::IsLoaded - ready when the child holds
// a surface, has a positive width, a parent manager, and an active status word.
RVA(0x00159150, 0x24)
i32 CDDrawSurfaceChildA::IsLoaded() {
    if (m_surface != 0 && m_width > 0 && m_mgr != 0 && m_status != -1) {
        return 1;
    }
    return 0;
}

// 0x1591b0 - a wap-object base re-init: seed m_4=-1, zero m_8/m_c/m_10. A
// standalone void METHOD (not a ctor). The owning wap-object class identity is
// unrecovered (@identity-TODO; only caller is an unmatched ??_G @0x159190).
class WapObjBase : public CObject {
public:
    void BaseInit();
    i32 m_4;
    i32 m_8;
    i32 m_c;
    i32 m_10;
};
SIZE_UNKNOWN(WapObjBase);
RVA(0x001591b0, 0x19)
void WapObjBase::BaseInit() {
    m_4 = -1;
    m_10 = 0;
    m_8 = 0;
    m_c = 0;
    // base vptr auto-stamped via CObject (retail manual stamp dropped, % ok)
}