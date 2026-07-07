#include <Gruntz/SoundCueMgr.h>
#include <Gruntz/LeafCue.h>
#include <rva.h>
// DDrawSubMgrLeafScan.cpp - a sibling sub-manager of the tomalla-named
// CDDrawSubMgrLeaf family (a CDirectDrawMgr surface/page sub-manager in the
// "DDraw surface manager" group; see docs/ddraw-family-names.md). This is
// the keyed-asset CACHE variant: it owns a CMapStringToOb at +0x10 keyed by
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
// CMapStringToOb/CString thunks, sprintf/strncmp, the file-iteration API, the
// per-element ConfigureItem) are reloc-masked external calls.
// ---------------------------------------------------------------------------

// <Mfc.h> brings real MFC CObject / CMapStringToOb / CString / POSITION; afx-first
// so it precedes any <windows.h>/DirectX header.
#include <Mfc.h>
#include <string.h> // strncmp / sprintf inline CRT, strcpy in the path copy

// Real DSound types so MatchSub_1584f0's GetFormat / SetPrimaryFormat calls
// mangle to the retail names (the relocs pair instead of staying fuzzy).
#include <Dsndmgr/DirectSoundMgr.h>
#include <Dsndmgr/SoundDevice.h>
#include <Gruntz/ParseSource.h>           // canonical CParseSource (BeginParse/EndParse)
#include <DDrawMgr/DDrawSubMgrLeafScan.h> // THE canonical CDDrawSubMgrLeafScan + LeafScanBase
class CSymTab {
public:
    void* FirstSub();
    void* NextSub(void* n);
    void* FirstSym();
    void* NextSym(void* n);
    void* NextSym2(void* n);
    void* NextSym3(void* n);
}; // DirNode views

// The map value: only the scalar-deleting destructor slot (+0x04) is load-
// bearing for the RemoveKeysEqual/FindKey teardown dispatch. Declared only -
// never defined, so no ??_7 emitted here. m_10 is the held sound-arg the probe
// helpers forward to MatchSub; FindKeyOfValue compares the value pointer itself.
class LeafScanValue {
public:
    virtual void GetRuntimeClass();   // [0] 0x1bef01 (shared thunk, declared-only)
    virtual i32 ScalarDtor(i32 flag); // +0x04 scalar-deleting destructor
    char m_pad04[0x10 - 0x04];        // +0x04..0x0f (after the vptr)
    void* m_10;                       // +0x10  held sound-arg (LeafScanSoundArg*)
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
DATA(0x0061ab20)
extern i32 g_61ab20; // reentrancy gate
DATA(0x0061ab24)
extern i32 g_61ab24; // cue-item id
DATA(0x006bf3c0)
extern "C" u32 g_6bf3c0; // draw-clock mirror

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
// vptr, so it is NOT a bare-Wap::CObject fold (Wap32/Object.h). Do not rename to
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
struct LeafElementObj : public LeafElementBase {
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
// The tree node is recursive: each subdir returned by FirstSubdir is itself a tree
// the walker recurses into. m_name (+0x00) is the entry name; GetTag (0x139800)
// reads the type tag (0x574156 == 'WAV' is the asset gate); the subdir/file
// iterators are __thiscall externals. All reloc-masked.
class DirNode {
public:
    // FirstSub @0x13a260 IS CSymTab::FirstSub; cast at each call.
    // NextSubdir @0x13a280 IS CSymTab::NextSub; cast at each call.
    // FirstFile @0x13a2b0 IS CSymTab::FirstSym; cast at each call.
    // NextFile @0x13a2d0 IS CSymTab::NextSym; cast at each call.
    // FirstEntry @0x13a2f0 IS CSymTab::NextSym2; cast at each call.
    // NextEntry @0x13a310 IS CSymTab::NextSym3; cast at each call.
    // GetTag @0x139800 IS CParseSource::GetEntryTag; cast at each call.

    const char* m_name; // +0x00
};
// The buffer freed at the walker's tail (_RezFree @0x1b9b82).
extern "C" void RezFree(void* p);
// Global operator new/delete (engine NAFXCW, operator_new @0x1b9b46); external/
// no-body. `delete e` on the polymorphic element routes through operator delete.
void* operator new(u32 n);
void operator delete(void* p);

// The canonical CDDrawSubMgrLeafScan + its LeafScanBase grand-base now live in the
// shared <DDrawMgr/DDrawSubMgrLeafScan.h> (included above): the class def is the
// single-source union of this TU's method set and the sibling CDDrawSubMgrLeaf TU's
// ??_G/IsReady/ClearMap. The body-only dep types (LeafElementObj / DirNode /
// LeafScanValue / LeafScanSoundArg) are forward-declared in the header and fully
// defined above so the method bodies below can dereference them.

// Read the map count at parent+0x1c (inside the CMapStringToOb's internal area,
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

// ---------------------------------------------------------------------------
// 0x1f940: LeafCue::PlayIfElapsed -- the cue's own gated play entry. When the
// reentrancy gate is open AND the throttle interval has elapsed since the last
// draw-clock, restamp the clock and tail-forward the 4 caller args to the player's
// ConfigureItem (returning its result); otherwise return 0. 4 stack args (ret 0x10).
// @early-stop
// 66% -- split-epilogue wall (twin of RefreshAsset_114120's 100% idiom, but 4-arg):
// the gate/interval guards, the wrap-safe clock compare, the clock restamp, and the
// identical 4-arg push sequence into ConfigureItem all match. MSVC5 here MERGES the
// two guard-failure `return 0` exits into one shared `pop esi; ret 0x10` tail
// (je/jb to a shared epilogue) where retail emits each failure as its own inline
// `xor eax; pop esi; ret 0x10` (jne/jae split). The ConfigureItem callee is
// reloc-masked (CStatusBarMgr vs LeafCuePlayer at the same 0x1360d0). Not source-
// steerable (tried flat + nested guard forms). Logic complete.
RVA(0x0001f940, 0x4c)
i32 LeafCue::PlayIfElapsed_01f940(i32 a0, i32 a1, i32 a2, i32 a3) {
    if (g_61ab20 == 0) {
        return 0;
    }
    if (g_6bf3c0 - (u32)m_14 >= (u32)m_18) {
        m_14 = g_6bf3c0;
        return m_10->ConfigureItem(a0, a1, a2, a3);
    }
    return 0;
}

// ---------------------------------------------------------------------------
// 0x114120: throttled per-asset refresh. While not loading (m_30==0), look up the
// keyed cue in the map; if present and the reentrancy gate is open, and the
// throttle interval has elapsed since its last draw-clock, restamp the clock and
// re-run its player's ConfigureItem with the gated cue-item id. Returns 0 always
// (the success path falls off the end of ConfigureItem's void return). 1 stack
// arg (ret 4); same cue-refresh idiom as CSBI_MenuItem's highlight path.
RVA(0x00114120, 0x70)
i32 CDDrawSubMgrLeafScan::RefreshAsset_114120(const char* key) {
    if (m_30 != 0) {
        return 0;
    }
    CObject* val = 0;
    m_10.Lookup(key, val);
    if (val == 0) {
        return 0;
    }
    i32 gate = g_61ab20;
    i32 item = g_61ab24;
    if (gate == 0) {
        return 0;
    }
    LeafCue* p = (LeafCue*)val;
    // Throttle: when the interval has elapsed, restamp the clock and tail-return the
    // (void-modeled) ConfigureItem result so the success epilogue falls through
    // WITHOUT zeroing eax (retail's split-epilogue shape: the guard-failure paths
    // return 0 via the trailing `xor eax,eax` exit, success is the fall-through).
    if (g_6bf3c0 - (u32)p->m_14 >= (u32)p->m_18) {
        p->m_14 = g_6bf3c0;
        return p->m_10->ConfigureItem(item, 0, 0, 0);
    }
    return 0;
}

// ---------------------------------------------------------------------------
// 0x5b7e0: Lookup `key` in the map and return the found CObject* (null if not).
RVA(0x0005b7e0, 0x23)
CObject* CDDrawSubMgrLeafScan::Lookup_05b7e0(const char* key) {
    CObject* val = 0;
    m_10.Lookup(key, val);
    return val;
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
// it after) + the reloc-masked EH unwind / VM18 / ~CMapStringToOb / vtable symbol
// names. objdiff-reloc-scoring.
RVA(0x00157570, 0x68)
CDDrawSubMgrLeafScan::~CDDrawSubMgrLeafScan() {
    // VM18 (0x157ae0) is slot [7] of this class's own vtable (ClearContext); a
    // virtual call on `this` inside the dtor devirtualizes to the retail direct
    // rel32, so no view cast is needed.
    ClearContext();
    // m_10 (CMapStringToOb) member dtor auto-fires here, then the LeafScanBase
    // destructor resets +0x04/+0x08/+0x0c and restamps the grand-base vtable.
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
    CObject* val = 0;
    POSITION pos = m_10.GetStartPosition();
    i32 n = 0;
    while (pos != 0) {
        m_10.GetNextAssoc(pos, key, val);
        if (strncmp(key, match, len) == 0) {
            m_10.RemoveKey(key);
            if (val != 0) {
                ((LeafScanValue*)val)->ScalarDtor(1);
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
    m_10[key] = (CObject*)e;
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
    m_10[key] = (CObject*)e;
    e->m_18 = m_34; // +0x18 = redraw arg
    return e;
}

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
// 0x157ee0: recursive directory walker. While not loading (m_30==0), allocate a
// 0x100-byte path buffer, then for each subdirectory of `tree` build the joined
// path (sprintf "%s%s%s" of prefix/suffix/name when prefix is non-empty, else a
// plain strcpy of the name) and recurse, summing the entry count. Then for each
// file, for each 'WAV'-tagged entry not already cached, build its path and create
// the cache element, counting successes. Frees the buffer and returns the count.
// 3 stack args (ret 0xc).
RVA(0x00157ee0, 0x1c6)
i32 CDDrawSubMgrLeafScan::ScanTree_157ee0(DirNode* tree, const char* prefix, const char* suffix) {
    if (m_30 != 0) {
        return 0;
    }
    i32 count = 0;
    char* buf = (char*)operator new(0x100);
    if (buf == 0) {
        return 0;
    }
    buf[0] = 0;
    DirNode* node = (DirNode*)((CSymTab*)tree)->FirstSub();
    while (node != 0) {
        if (prefix != 0 && *prefix != 0) {
            sprintf(buf, "%s%s%s", prefix, suffix, node->m_name);
        } else {
            strcpy(buf, node->m_name);
        }
        count += ScanTree_157ee0(node, buf, suffix);
        node = (DirNode*)((CSymTab*)tree)->NextSub(node);
    }
    void* file = ((CSymTab*)tree)->FirstSym();
    if (file != 0) {
        do {
            DirNode* fn = (DirNode*)((CSymTab*)tree)->NextSym2(file);
            while (fn != 0) {
                if (((CParseSource*)fn)->GetEntryTag() == 0x574156) {
                    if (prefix != 0 && *prefix != 0) {
                        sprintf(buf, "%s%s%s", prefix, suffix, fn->m_name);
                    } else {
                        strcpy(buf, fn->m_name);
                    }
                    CObject* val = 0;
                    m_10.Lookup(buf, val);
                    if (val == 0) {
                        if (CreateEntry_157d70(buf, fn) != 0) {
                            ++count;
                        }
                    }
                }
                fn = (DirNode*)((CSymTab*)tree)->NextSym3(fn);
            }
            file = ((CSymTab*)tree)->NextSym(file);
        } while (file != 0);
    }
    RezFree(buf);
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
    CObject* val = 0;
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
    CObject* val = 0;
    CString key;
    m_10.GetNextAssoc(pos, key, val);
    return (LeafScanValue*)val;
}

// ---------------------------------------------------------------------------
// 0x1583c0: return 1 if any map key strncmp-equals `str` over strlen(str), else
// 0. Twin of CDDrawWorkerRegistry::HasKeyEqual_155550. /GX EH frame for the local
// CString key.
RVA(0x001583c0, 0xdc)
i32 CDDrawSubMgrLeafScan::HasKeyEqual_1583c0(const char* str) {
    i32 len = strlen(str);
    CString key;
    CObject* val = 0;
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
    CObject* val = 0;
    POSITION pos = m_10.GetStartPosition();
    while (pos != 0) {
        m_10.GetNextAssoc(pos, key, val);
        if (val == (CObject*)target) {
            return key;
        }
    }
    key.Empty();
    return key;
}

SIZE_UNKNOWN(DirNode);
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
