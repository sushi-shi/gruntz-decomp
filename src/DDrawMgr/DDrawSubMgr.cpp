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

#include <DDrawMgr/DDrawSubMgr.h> // own extern surface
#include <Dsndmgr/DirectSoundMgr.h>
#include <Rez/FrameClock.h> // frame-clock band (g_frameDelta/g_frameTime/g_killCueClock/g_engineFrameDelta)
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
#include <Gruntz/StateId.h>  // StateId (GetStateId return type)
#include <Gruntz/Loadable.h> // CLoadable - the 9-slot loadable base (3-arg ctor def below)
#include <Mfc.h>             // real MFC CMapStringToPtr / CString / POSITION
#include <Bute/SymTab.h>     // CSymTab (ProbeWorkerKey's probe chain)
#include <string.h>
#include <DDrawMgr/DirectDrawMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>    // single-source CDDrawSurfacePair
#include <Gruntz/AniAdvanceCursor.h>      // CAniAdvanceCursor
#include <Gruntz/SerialArchive.h>         // the shared CFileMemBase stream
#include <DDrawMgr/DDrawSurfaceMgr.h>     // canonical CDDrawSurfaceMgr
#include <DDrawMgr/DDrawSubMgrPages.h>    // single-source CDDrawSubMgrPages (surface ops)
#include <DDrawMgr/DDrawChildGroup.h>     // CDDrawChildGroup (the 3-map dtor-host twin)
#include <DDrawMgr/DDrawWorkerRegistry.h> // canonical CDDrawWorkerRegistry (real polymorphic)
#include <DDrawMgr/DDrawWorker.h>         // CDDrawWorker (the registry map values)
#include <DDrawMgr/DDrawSubMgrLeaf.h>     // CDDrawSubMgrLeaf (the ANI catalog host)
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <DDrawMgr/DDrawWorkerHost.h> // CDDrawWorkerHost (the m_ctx geometry chain)
#include <Gruntz/GameLevel.h>         // CGameLevel::m_mainPlane (the m_ctx geometry chain)
#include <DDrawMgr/AniAdvance.h>      // CAniBlitTrigger (the per-frame sound trigger)
#include <Wap32/WapObj.h>             // CWapObj : CObject
#include <Gruntz/SoundState.h> // ex Globals.h transitive

void operator delete(void*);

// The ctor 0x156cb0 stamps 0x5efc30 - CLoadable's OWN vtable - so CDDrawSubMgr IS
// CLoadable under a second name (<Gruntz/Loadable.h> records the proof). The 3-arg
// CLoadable base ctor is defined below at its retail RVA.
// 0x155720 is CLoadable's ??_G scalar-deleting-dtor COMDAT copy (member-teardown ~ at
// 0xd5d70, the CImage-band pool) - both are cl auto-emitted, byte-identical to
// retail, and RVA_COMPGEN-bound as the REAL ??_GCLoadable/??1CLoadable in
// DDrawWorkerRegistry.cpp.

VTBL(CLoadable, 0x001efc30); // ??_7CLoadable (the shared 9-slot loadable-base vtable)
VTBL(CDDrawSubMgrLeaf, 0x001efc78); // ??_7CDDrawSubMgrLeaf (was g_catalogVtbl)
VTBL(CDDrawSubMgrLeafScan, 0x001efca0); // ??_7 (9-slot, LeafScanBase-derived)
VTBL(CDDrawWorkerMapSmall, 0x001efcc8); // ??_7CDDrawWorkerMapSmall @0x5efcc8
VTBL(CDDrawWorkerCache, 0x001efd00);
VTBL(CDDrawWorkerRegistry, 0x001efd28); // ??_7CDDrawWorkerRegistry@@6B@ (23 slots)
VTBL(CDDrawWorkerList, 0x001efd88); // ??_7CDDrawWorkerList@@6B@ (14-slot vtable)
VTBL(CDDrawChildGroup, 0x001efdc0); // ??_7CDDrawChildGroup@@6B@ (17-slot vtable)
VTBL(CDDrawSubMgrPages, 0x001efe08); // ??_7CDDrawSubMgrPages@@6B@ (10-slot CWapObj-derived vtable)
VTBL(CFileMem, 0x001efe30);
VTBL(CFileMemBase, 0x001efe68);
VTBL(CDDrawWorkerA, 0x001efea0); // vtable_names -> code (RTTI game class)
VTBL(CDDrawWorkerB, 0x001efed0);
VTBL(LeafCue, 0x001eff08); // ??_7LeafCue (9-slot CLoadable leaf; was g_leafElemVtbl)
DATA(0x001eff2c)
float g_sndPanScale = 0.009999999776482582f;
VTBL(CDDrawSurfacePair, 0x001eff30);
VTBL(CDDrawSurfaceChildA, 0x001eff70); // ??_7CDDrawSurfaceChildA@@6B@ (11 slots)
VTBL(CDrawSubWorker, 0x001effa0); // ??_7CDrawSubWorker (11-slot CLoadable leaf)

void* operator new(u32 n);
void operator delete(void* p);

static inline i32 LeafReadMapCount(const CDDrawSubMgrLeafScan* p) {
    return *reinterpret_cast<const i32*>((reinterpret_cast<const char*>(p) + 0x1c));
}

RVA(0x00114120, 0x70)
i32 CDDrawSubMgrLeafScan::RefreshAsset(const char* key) {
    if (m_emitGate != 0) {
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
    LeafCue* p = static_cast<LeafCue*>(val);
    // Throttle: when the interval has elapsed, restamp the clock and tail-return the
    // (void-modeled) ConfigureItem result so the success epilogue falls through
    // WITHOUT zeroing eax (retail's split-epilogue shape: the guard-failure paths
    // return 0 via the trailing `xor eax,eax` exit, success is the fall-through).
    if (g_killCueClock - static_cast<u32>(p->m_14) >= static_cast<u32>(p->m_18)) {
        p->m_14 = g_killCueClock;
        return p->m_10->ConfigureItem(item, 0, 0, 0);
    }
    return 0;
}

RVA(0x00156cb0, 0x20)
CLoadable::CLoadable(i32 owner, i32 field04, i32 field08) {
    m_id = field04;
    m_flags = field08;
    m_ownerCtx = owner;
}

RVA(0x00156cd0, 0x16)
i32 CDDrawWorkerMapSmall::IsLoaded() {
    if (m_ownerCtx == 0) {
        goto fail;
    }
    if (m_id != -1) {
        return 1;
    }

fail:
    return 0;
}

// [8] the class id (was a Ghidra recovery gap / declared-only).
RVA(0x00156cf0, 0x6)
i32 CDDrawWorkerMapSmall::GetClassId() {
    return CLASSID_WORKERMAPSMALL; // 0x14
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
    Unload();
    // m_map3 / m_map2 / m_map1 (reverse decl order) and the grand-base
    // auto-destruct here under the /GX member-teardown trylevels.
}

RVA(0x00156db0, 0x6)
i32 CDDrawWorkerMapSmall::IsReady() {
    return 1;
}

RVA(0x00156de0, 0x6)
i32 CDDrawWorkerRegistry::GetClassId() {
    return CLASSID_WORKERREGISTRY; // 0x12
}

// ---------------------------------------------------------------------------
// 0x156e10: ~CDDrawWorkerRegistry (real ??1; the compiler-generated ??_G at
// 0x156df0 calls it). cl stamps ??_7CDDrawWorkerRegistry (0x5efd28) at entry, runs
// Unload (slot 7, devirtualized in the dtor to the retail direct `call 0x154ac0`),
// destructs the CMapStringToOb member (+0x10, retail `call 0x1b7ef2`), then the
// inline ~CLoadable resets m_04/-1 m_08/0 m_0c/0 and the real CObject grand-base
// sinks the 0x5e8cb4 re-stamp after them (the ~CDDrawWorker-proven model). /GX.
// The cl-auto scalar-deleting destructor (vtable slot 1):
RVA_COMPGEN(0x00156df0, 0x1e, ??_GCDDrawWorkerRegistry@@UAEPAXI@Z)
RVA(0x00156e10, 0x68)
CDDrawWorkerRegistry::~CDDrawWorkerRegistry() {
    Unload();
    // implicit: ~m_10map (CMapStringToOb), then ~CLoadable (field resets + grand-base
    // 0x5e8cb4 re-stamp) - reproduces retail's teardown order.
}

RVA(0x00156e80, 0x38)
i32 CDDrawWorkerRegistry::ProbeWorkerKey(CSymTab* arg1, i32 arg2) {
    void* result = arg1->Get_13b900()->FindSub(reinterpret_cast<const char*>(arg2));
    // retail: the InstallTree path is the fall-through, return 0 out-of-line at the tail.
    if (result != 0) {
        return InstallTree(result, g_emptyString, "_"); // slot-18 self-dispatch
    }
    return 0;
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
    if (m_10map.Lookup(key, val)) {
        m_10map.RemoveKey(key);
        delete (static_cast<CDDrawWorker*>(val)); // the map values ARE the keyed workers
    }
}

RVA(0x00156f00, 0x16)
i32 CDDrawWorkerList::IsLoaded() {
    if (m_ownerCtx == 0) {
        goto fail;
    }
    if (m_id != -1) {
        return 1;
    }

fail:
    return 0;
}

RVA(0x00156f20, 0x6)
i32 CDDrawWorkerList::GetClassId() {
    return CLASSID_WORKERLIST; // 0x11
}

RVA(0x00156f50, 0x68)
CDDrawWorkerList::~CDDrawWorkerList() {
    Unload();
    // implicit: ~m_workers (CObList) then ~CLoadable (field resets + base restamp).
}

RVA(0x00156fc0, 0x6)
i32 CDDrawWorkerList::IsReady() {
    return 1;
}

RVA(0x00156fd0, 0x8b)
void* CDDrawWorkerList::CreateWorkerA(i32 a1, i32 a2, i32 a3) {
    CDDrawWorkerA* w = new CDDrawWorkerA(OwnerMgr());
    if (w->Vfunc2C(a1, a2, a3) == 0) {
        if (w != 0) {
            delete w;
        }
        return 0;
    }
    m_workers.AddTail(static_cast<CObject*>(w));
    return w;
}

RVA(0x00157080, 0x19)
i32 CDDrawWorkerBase::SetPosition(i32 x, i32 y) {
    m_refCount = 2;
    return CResolveNode::SetPosition(x, y); // direct base call (retail rel32 0x164790)
}

// ~CDDrawWorkerA (0x1570d0; ??_G wrapper 0x1570b0): poison the timing/marker
// fields to their sentinels (the m_20/m_38 pair reset THREE times, via volatile
// lvalues so cl keeps all three), null the header; retail then INLINES the whole
// ~CResolveNode/~CLoadable chain down to the single CObject grand-base stamp.
// @early-stop
// (A)-form base-dtor wall (~59%, twin of ~CDDrawWorkerB): every poison/reset
// store matches; the residual is (1) a tail `jmp ??1CResolveNode` where retail
// inlined the base teardown (our ~CResolveNode is deliberately OUT-OF-LINE at
// 0x154a50 - see ResolveNode.h; an extern base dtor cannot be inlined here), and
// (2) the entry ??_7CDDrawWorkerA stamp the tail call keeps alive (retail's died
// into the final CObject stamp). Fix = the family-wide inline/(B)-form dtor flip,
// deferred with CLoadable's. Pre-rebase this was ~94% on a fake CObject base;
// the CResolveNode truth is worth the drop (factories/Vfuncs all EXACT).
RVA(0x001570d0, 0x39)
CDDrawWorkerA::~CDDrawWorkerA() {
    volatile LONG* pHi = &m_dirtyRect.left;
    volatile i32* pLo = &m_dirtyArmed;
    m_78b = 0;
    *pHi = static_cast<LONG>(0x80000000);
    *pLo = -1;
    *pHi = static_cast<LONG>(0x80000000);
    *pLo = -1;
    m_screenX = static_cast<i32>(0x80000000);
    *pHi = static_cast<LONG>(0x80000000);
    *pLo = -1;
    m_id = -1;
    m_flags = 0;
    m_ownerCtx = 0; // the owner-ctx handle
}

RVA(0x00157110, 0x20)
i32 CDDrawWorkerA::Vfunc2C(i32 a1, i32 a2, i32 a3) {
    m_78b = static_cast<char>(a3);
    m_refCount = 2;
    return CResolveNode::SetPosition(a1, a2); // direct base call (retail rel32 0x164790)
}

RVA(0x00157150, 0xa5)
void* CDDrawWorkerList::CreateWorkerB30(i32 a1, i32 a2, i32 a3, i32 a4, i32 addHead) {
    CDDrawWorkerB* w = new CDDrawWorkerB(OwnerMgr());
    if (w->PlaceBound(a1, a2, a3, a4) == 0) {
        if (w != 0) {
            delete w;
        }
        return 0;
    }
    if (addHead & 1) {
        m_workers.AddHead(static_cast<CObject*>(w));
    } else {
        m_workers.AddTail(static_cast<CObject*>(w));
    }
    return w;
}

RVA(0x00157200, 0xb)
i32 CDDrawWorkerBase::IsLoaded() {
    return m_78 != 0;
}

RVA(0x00157210, 0x6)
i32 CDDrawWorkerBase::GetClassId() {
    return CLASSID_WORKERNODE; // 8
}

// ~CDDrawWorkerB (0x157240; ??_G wrapper 0x157220). Mirror of ~CDDrawWorkerA -
// the int-frame worker's m_78 is a DWORD here (byte in A).
// @early-stop
// (A)-form base-dtor wall (~59%): same residual as ~CDDrawWorkerA (tail
// `jmp ??1CResolveNode` vs retail's inlined base teardown + the kept entry stamp).
RVA(0x00157240, 0x3c)
CDDrawWorkerB::~CDDrawWorkerB() {
    volatile LONG* pHi = &m_dirtyRect.left;
    volatile i32* pLo = &m_dirtyArmed;
    m_78 = 0;
    *pHi = static_cast<LONG>(0x80000000);
    *pLo = -1;
    *pHi = static_cast<LONG>(0x80000000);
    *pLo = -1;
    m_screenX = static_cast<i32>(0x80000000);
    *pHi = static_cast<LONG>(0x80000000);
    *pLo = -1;
    m_id = -1;
    m_flags = 0;
    m_ownerCtx = 0; // the owner-ctx handle
}

RVA(0x00157280, 0x30)
i32 CDDrawWorkerB::PlaceBound(i32 a1, i32 a2, i32 a3, i32 a4) {
    Helper(a3, a4);
    m_refCount = 2;
    return CResolveNode::SetPosition(a1, a2); // direct base call (retail rel32 0x164790)
}

RVA(0x001572b0, 0x38)
i32 CDDrawWorkerB::PlaceFrame(i32 a1, i32 a2, CDDrawWorker* src, i32 a4) {
    i32 frame;
    if (a4 >= src->m_minIndex && a4 <= src->m_maxIndex) {
        frame = reinterpret_cast<i32>(src->m_items[a4]); // CObArray operator[] inline = m_pData[a4]
    } else {
        frame = 0;
    }
    m_78 = frame;
    m_refCount = 2;
    return CResolveNode::SetPosition(a1, a2); // direct base call (retail rel32 0x164790)
}

RVA(0x001572f0, 0x20)
i32 CDDrawWorkerB::Vfunc2C(i32 a1, i32 a2, i32 a3) {
    m_78 = a3;
    m_refCount = 2;
    return CResolveNode::SetPosition(a1, a2); // direct base call (retail rel32 0x164790)
}

RVA(0x00157310, 0x1a)
void CDDrawWorkerBase::Unload() {
    // void per the CLoadable slot; retail's eax residue is the INT_MIN the
    // stores materialize.
    i32 v = static_cast<i32>(0x80000000);
    m_78 = 0;
    m_screenX = v;
    m_dirtyRect.left = v;
    m_dirtyArmed = -1;
}

RVA(0x00157330, 0xa5)
void* CDDrawWorkerList::CreateWorkerB2C(i32 a1, i32 a2, CDDrawWorker* a3, i32 a4, i32 addHead) {
    CDDrawWorkerB* w = new CDDrawWorkerB(OwnerMgr());
    if (w->PlaceFrame(a1, a2, a3, a4) == 0) {
        if (w != 0) {
            delete w;
        }
        return 0;
    }
    if (addHead & 1) {
        m_workers.AddHead(static_cast<CObject*>(w));
    } else {
        m_workers.AddTail(static_cast<CObject*>(w));
    }
    return w;
}

RVA(0x001573e0, 0xa0)
void* CDDrawWorkerList::CreateWorkerB28(i32 a1, i32 a2, i32 a3, i32 addHead) {
    CDDrawWorkerB* w = new CDDrawWorkerB(OwnerMgr());
    if (w->Vfunc2C(a1, a2, a3) == 0) {
        if (w != 0) {
            delete w;
        }
        return 0;
    }
    if (addHead & 1) {
        m_workers.AddHead(static_cast<CObject*>(w));
    } else {
        m_workers.AddTail(static_cast<CObject*>(w));
    }
    return w;
}

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

// 0x1574b0 is the compiler-generated scalar-deleting destructor (auto-emitted COMDAT).
RVA_COMPGEN(0x001574b0, 0x1e, ??_GCDDrawSubMgrPages@@UAEPAXI@Z)

RVA(0x001574d0, 0x5b)
CDDrawSubMgrPages::~CDDrawSubMgrPages() {
    Unload();
    // implicit ~CLoadable (the three header resets) -> ~CObject grand-base re-stamp.
}

// 0x157550 is the compiler-generated scalar-deleting destructor (auto-emitted COMDAT).
RVA_COMPGEN(0x00157550, 0x1e, ??_GCDDrawSubMgrLeafScan@@UAEPAXI@Z)

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
    // Unload (0x157ae0) is slot [7] of this class's own vtable; a virtual call on
    // `this` inside the dtor devirtualizes to the retail direct rel32, so no view
    // cast is needed.
    Unload();
    // m_10 (CMapStringToPtr) member dtor auto-fires here, then the ~CLoadable base
    // destructor resets +0x04/+0x08/+0x0c and restamps the grand-base vtable.
}

RVA(0x001575e0, 0x16)
i32 CDDrawChildGroup::IsLoaded() {
    if (m_ownerCtx == 0 || m_id == -1) {
        return 0;
    }
    return 1;
}

RVA(0x00157600, 0x6)
i32 CDDrawChildGroup::GetClassId() {
    return CLASSID_CHILDGROUP; // 0x10
}

// ---------------------------------------------------------------------------
// 0x157630: ~CDDrawChildGroup (real ??1; the cl-generated ??_G @0x157610 calls it).
// cl stamps ??_7CDDrawChildGroup (0x5efdc0) at entry, runs Unload (slot 7,
// devirtualized in the dtor to retail's direct `call 0x1591e0`), resets the three
// header words, then the members auto-destruct in reverse decl order under the /GX
// trylevels - ~m_map48/~m_map2c (CMapPtrToPtr @0x1b8665), ~m_list (CObList
// @0x1b5a2b) - and ~CObject folds the grand-base re-stamp last.
// (Reset-position residual RESOLVED by the : CLoadable re-base 2026-07-22: the
// three header resets now come from ~CLoadable AFTER the member teardown -
// retail.s exact order.)
// The cl-auto scalar-deleting destructor (vtable slot 1):
RVA_COMPGEN(0x00157610, 0x1e, ??_GCDDrawChildGroup@@UAEPAXI@Z)
RVA(0x00157630, 0x82)
CDDrawChildGroup::~CDDrawChildGroup() {
    Unload();
    // implicit: ~m_map48, ~m_map2c, ~m_list, then ~CLoadable (the three header
    // resets + grand-base re-stamp) - retail.s reset-after-members order.
}

RVA(0x001576c0, 0x6)
i32 CDDrawChildGroup::IsReady() {
    return 1;
}

// ---------------------------------------------------------------------------
// CDDrawWorkerCache dtor (0x157720; ??_G pin 0x157700).
// Scalar-deleting destructor: run the real member-teardown ~, then operator
// delete this if the low flag bit is set. RVA_COMPGEN pins the ??_G mangling.
RVA_COMPGEN(0x00157700, 0x1e, ??_GCDDrawWorkerCache@@UAEPAXI@Z)

// The real member-teardown destructor (0x157720, /GX): cl stamps
// ??_7CDDrawWorkerCache (masks 0x5efd00) at entry, runs the class's own slot-7
// DestroyAll (@0x165210, devirtualized in the dtor to `call 0x165210` - the
// binary-proven single call site), then destructs the CMapStringToPtr member and
// the grand-base. /GX member-teardown frame from the destructible map.
// @early-stop
// vptr-position wall (~95%, twin of CDDrawSubMgrLeaf/CDDrawWorker): every
// instruction matches retail EXCEPT the grand-base vptr re-stamp POSITION + the
// reloc-masked EH-state/teardown/vtable symbol names. Logic complete.
RVA(0x00157720, 0x68)
CDDrawWorkerCache::~CDDrawWorkerCache() {
    Unload();
    // implicit: ~m_10 (CMapStringToPtr), then the grand-base field resets + the
    // ??_7 re-stamp - reproduces retail's teardown order.
}

RVA(0x001577a0, 0x16)
i32 CDDrawSubMgrLeaf::IsLoaded() {
    if (m_ownerCtx == 0) {
        goto fail;
    }
    if (m_id != -1) {
        return 1;
    }

fail:
    return 0;
}

// Scalar-deleting destructor: COMPILER-GENERATED from the class's virtual ~.
RVA_COMPGEN(0x001577c0, 0x1e, ??_GCDDrawSubMgrLeaf@@UAEPAXI@Z)

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
    FreeAll();
    // implicit: ~m_10 (CMapStringToPtr), then ~CDDrawSubMgrGrandBase (resets the three
    // header fields + restamps the base vtable) - reproduces retail's teardown order.
}

RVA(0x00157850, 0x54)
CFileMemBase::CFileMemBase() {
    m_4 = 0;
    m_mode = 0;
    m_name.Empty();
}

// ~CFileMemBase (0x1578b0) - base teardown.
// @early-stop
// EH-dtor virtual-dispatch wall (~89%): the base teardown logic is byte-faithful,
// but retail dispatches Reset as an absolute indirect through the base vtable
// slot 3 - a virtual dispatch inside a dtor that MSVC5 devirtualizes to a direct
// call from clean C++.
RVA(0x001578b0, 0x51)
CFileMemBase::~CFileMemBase() {
    Reset();
}

RVA(0x00157920, 0x20)
CString CFileMemBase::GetName() {
    return m_name;
}

// CFileMem::~CFileMem (0x157980): cl stamps the derived vtable at entry, run
// Reset() (derived), destruct the inner CFile, call the base Reset(), then cl
// folds the base vtable restamp + the CString member dtor on unwind.
// @early-stop
// EH-dtor scheduling wall (~59%): the teardown logic is byte-faithful, but the
// virtual-dtor auto vtable restamps + the /GX trylevel store sequencing + the
// member-dtor dispatch differ from retail's manual sequence.
RVA(0x00157980, 0x74)
CFileMem::~CFileMem() {
    Reset();
    m_file.~CFile();
    CFileMemBase::Reset();
}

RVA(0x00157a40, 0x10)
void CFileMemBase::Reset() {
    m_4 = 0;
    m_mode = 0;
    m_name.Empty();
}

RVA(0x00157a50, 0x16)
void CFileMem::Reset() {
    m_length = 0;
    m_offset = 0;
    m_4 = 0;
    m_mode = 0;
    m_name.Empty();
}

RVA(0x00157a80, 0x51)
i32 CAniAdvanceCursor::SelectCue(void* force) {
    char* mgr = reinterpret_cast<char*>(m_ownerCtx); // the +0x0c owner (cue-role: the sub-manager)
    if (mgr == 0) {
        return 0;
    }
    char* cue = *reinterpret_cast<char**>((mgr + 0x20));
    if (force == 0) {
        if (cue == 0) {
            return 0;
        }
        if (*reinterpret_cast<i32*>((cue + 0x78)) == 0) {
            return 0;
        }
    }
    if (cue == 0) {
        m_pendingDraw = 1; // +0x30 (cue-role: cue-absent flag)
    } else {
        m_pendingDraw = 0;
    }
    m_2c = reinterpret_cast<i32>(cue);
    g_sndCueTag = 0x64;
    return 1;
}

RVA(0x00157ae0, 0x11)
void CDDrawSubMgrLeafScan::Unload() { // slot 7 (CLoadable::Unload override; clears the map)
    ClearMap();
    m_2c = 0; // clear the held stream (+0x2c; retail movl [esi+0x2c],0)
}

// ===========================================================================
// CDDrawSubMgrLeafScan::RemoveByValue (0x157b00): remove one map entry by its value
// (ex the CSoundResMap/CSoundRes view pair - the "sound registry" IS the leaf-scan
// class and the values ARE LeafCue elements, same as ClearMap below)
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
void CDDrawSubMgrLeafScan::RemoveByValue(LeafCue* p) {
    if (p == 0) {
        return;
    }
    POSITION pos = reinterpret_cast<POSITION>((m_10.GetCount() != 0 ? -1 : 0));
    CString key;
    void* value = 0;
    if (pos != static_cast<POSITION>(0)) {
        do {
            m_10.GetNextAssoc(pos, key, value);
            if (value == p) {
                m_10.RemoveKey(key);
                delete p;
                break;
            }
        } while (pos != static_cast<POSITION>(0));
    }
}

// ---------------------------------------------------------------------------
// 0x157bc0: iterate every entry of the name-keyed map via GetNextAssoc, destroying
// each value through its scalar-deleting destructor (vtbl +0x4 arg 1), then
// RemoveAll. /GX EH frame for the local CString key.
// @early-stop
// store-scheduling coin-flip (~94%): byte-identical to retail except the `val = 0`
// store position + the reloc-masked EH-state push (same family wall as
// FreeAll). docs/patterns/zero-register-pinning.md.
RVA(0x00157bc0, 0xa2)
i32 CDDrawSubMgrLeafScan::ClearMap() {
    void* val = 0;
    POSITION pos = reinterpret_cast<POSITION>((m_10.GetCount() != 0 ? -1 : 0));
    CString key;
    if (*reinterpret_cast<volatile i32*>(&pos) != 0) {
        do {
            m_10.GetNextAssoc(pos, key, val);
            if (val != 0) {
                delete (static_cast<LeafCue*>(val)); // the cache values ARE the LeafCue elements
            }
        } while (pos != 0);
    }
    m_10.RemoveAll();
    return reinterpret_cast<i32>(val); // i32 (Unload's residue-carrier): retail returns the
                                       // trailing ~CString(key) eax; val (0 at exit) stands in.
}

RVA(0x00157c70, 0xf8)
i32 CDDrawSubMgrLeafScan::RemoveKeysEqual(const char* base, const char* str) {
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
                delete (static_cast<LeafCue*>(val));
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
// ~99.3% - register-naming coin-flip (was 99.81 pre-CLoadable): code bytes match EXCEPT the
// ecx<->edx assignment for the two seed reads (count<-this+0x1c, handle<-this+0x0c).
// Retail pins count in ecx, handle in edx; MSVC5 here swaps them. Same values,
// same stores, same order; not source-steerable (tried count-first / handle-first /
// helper-extracted reads). docs/patterns/zero-register-pinning.md.
RVA(0x00157d70, 0x90)
LeafCue* CDDrawSubMgrLeafScan::CreateEntry(const char* key, void* arg2) {
    if (m_emitGate != 0) {
        return 0;
    }
    LeafCue* e = new LeafCue(LeafReadMapCount(this), m_ownerCtx);
    if (e == 0) {
        return 0;
    }
    if (e->Configure(static_cast<CParseSource*>(arg2)) == 0) {
        delete e; // virtual scalar-deleting dtor (vtbl[1](1))
        return 0;
    }
    m_10[key] = e;
    e->m_18 = m_34; // +0x18 = redraw arg
    return e;
}

// ---------------------------------------------------------------------------
// 0x157e00: the second cache-element factory. Byte-for-byte twin of
// CreateEntry except the element configure goes through the file-path
// LoadSoundB (0x158720) instead of the parsed Configure (0x158760): allocate +
// seed the element from the map count (this+0x1c) and handle (this+0x0c), run
// Configure2 keyed by `arg2`; on failure scalar-delete + return 0, on success
// link into the map under `key` + stamp the redraw arg (this+0x34). 2 args (ret 8).
// @early-stop
// register-naming coin-flip (twin of CreateEntry's 99.81%): every code byte
// matches retail EXCEPT the ecx<->edx assignment for the two seed reads. Same
// values/stores/order; not source-steerable. docs/patterns/zero-register-pinning.md.
RVA(0x00157e00, 0x90)
LeafCue* CDDrawSubMgrLeafScan::CreateEntry2(const char* key, void* arg2) {
    if (m_emitGate != 0) {
        return 0;
    }
    LeafCue* e = new LeafCue(LeafReadMapCount(this), m_ownerCtx);
    if (e == 0) {
        return 0;
    }
    if (e->LoadSoundB(arg2) == 0) {
        delete e; // virtual scalar-deleting dtor (vtbl[1](1))
        return 0;
    }
    m_10[key] = e;
    e->m_18 = m_34; // +0x18 = redraw arg
    return e;
}

RVA(0x00157e90, 0x23)
LeafCue* CDDrawSubMgrLeafScan::AddFromSource(CParseSource* src) {
    if (m_emitGate != 0) {
        return 0;
    }
    if (src == 0) {
        return 0;
    }
    return CreateEntry(src->m_name, src);
}

RVA(0x00157ec0, 0x20)
void CDDrawSubMgrLeafScan::AddEntry(LeafCue* elem, const char* key) {
    m_10[key] = elem;
    elem->m_18 = m_34;
}

RVA(0x00157ee0, 0x1c6)
i32 CDDrawSubMgrLeafScan::ScanTree(CSymTab* tree, const char* prefix, const char* suffix) {
    if (m_emitGate != 0) {
        return 0;
    }
    i32 count = 0;
    char* buf = static_cast<char*>(operator new(0x100));
    if (buf == 0) {
        return 0;
    }
    buf[0] = 0;
    CSymTab* node = static_cast<CSymTab*>(tree->FirstSub());
    while (node != 0) {
        if (prefix != 0 && *prefix != 0) {
            sprintf(buf, "%s%s%s", prefix, suffix, node->m_name);
        } else {
            strcpy(buf, node->m_name);
        }
        count += ScanTree(node, buf, suffix);
        node = static_cast<CSymTab*>(tree->NextSub(node));
    }
    // `file` stays void*: the outer leaf-table record has its next-link at +0x04 (NextSym)
    // and its entry chain at +0x24 (NextSym2) - neither offset is CParseSource's, so this
    // record's class is NOT proven here. Left honest rather than guessed.
    void* file = tree->FirstSym();
    if (file != 0) {
        do {
            CParseSource* fn = static_cast<CParseSource*>(tree->NextSym2(file));
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
                        if (CreateEntry(buf, fn) != 0) {
                            ++count;
                        }
                    }
                }
                fn = static_cast<CParseSource*>(tree->NextSym3(fn));
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
i32 CDDrawSubMgrLeafScan::SumField(const char* str) {
    if (m_emitGate != 0) {
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
                sum += (static_cast<LeafCue*>(val))->m_10->m_sampleCount;
            } else if (strncmp(key, str, strlen(str)) == 0) {
                sum += (static_cast<LeafCue*>(val))->m_10->m_sampleCount;
            }
        }
    }
    return sum;
}
RVA(0x001581b0, 0x5b)
i32 CDDrawSubMgrLeafScan::Fire(const char* key, i32 pos, i32 range1, i32 range2) {
    char* p24 = reinterpret_cast<char*>(OwnerMgr()->m_level);
    if (p24 != 0 && *reinterpret_cast<char**>((p24 + 0x5c)) != 0 && m_emitGate == 0) {
        void* val = 0;
        m_10.Lookup(key, val);
        if (val != 0) {
            return (static_cast<CAniBlitTrigger*>(val))
                ->TriggerBlit(pos, -1, range1, range2);
        }
    }
    return 0;
}

RVA(0x00158210, 0xaa)
LeafCue* CDDrawSubMgrLeafScan::GetFirstValue() {
    if (m_emitGate != 0) {
        return 0;
    }
    POSITION pos = reinterpret_cast<POSITION>((m_10.GetCount() != 0 ? -1 : 0));
    if (pos == 0) {
        return 0;
    }
    void* val = 0;
    CString key;
    m_10.GetNextAssoc(pos, key, val);
    return static_cast<LeafCue*>(val);
}

RVA(0x001582c0, 0xf6)
LeafCue* CDDrawSubMgrLeafScan::NextValueAfter(LeafCue* target) {
    if (target == 0) {
        return 0;
    }
    if (m_emitGate != 0) {
        return 0;
    }
    POSITION pos = reinterpret_cast<POSITION>((m_10.GetCount() != 0 ? -1 : 0));
    if (pos == 0) {
        return 0;
    }
    void* val = 0;
    CString key;
    while (pos != 0) {
        m_10.GetNextAssoc(pos, key, val);
        if (val == static_cast<void*>(target)) {
            if (pos == 0) {
                return 0;
            }
            val = 0;
            m_10.GetNextAssoc(pos, key, val);
            return static_cast<LeafCue*>(val);
        }
    }
    return 0;
}

RVA(0x001583c0, 0xdc)
i32 CDDrawSubMgrLeafScan::HasKeyEqual(const char* str) {
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

RVA(0x001584a0, 0x43)
i32 CDDrawSubMgrLeafScan::ProbeFirst(i32 arg) {
    if (m_2c == 0) {
        return 0;
    }
    LeafCue* val = GetFirstValue();
    if (val == 0) {
        return 0;
    }
    // Retail reads val->m_10 only to null-check it, then passes `val` itself to
    // MatchSub (whose arg1->m_10 reaches the same held buffer).
    if (val->m_10 == 0) {
        return 0;
    }
    return MatchSub(val, arg) != 0;
}

RVA(0x001584f0, 0x80)
i32 CDDrawSubMgrLeafScan::MatchSub(LeafCue* arg1, i32 arg2) {
    if (arg1 == 0) {
        return reinterpret_cast<i32>(arg1);
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

RVA(0x00158570, 0xd4)
CString CDDrawSubMgrLeafScan::FindKeyOfValue(LeafCue* target) {
    CString key;
    if (target == 0) {
        return key;
    }
    void* val = 0;
    POSITION pos = m_10.GetStartPosition();
    while (pos != 0) {
        m_10.GetNextAssoc(pos, key, val);
        if (val == static_cast<void*>(target)) {
            return key;
        }
    }
    key.Empty();
    return key;
}

// ---------------------------------------------------------------------------
// 0x158680: ~LeafCue (the non-deleting destructor). cl auto-stamps ??_7LeafCue at
// entry, runs Unload (slot 7, devirtualized in the dtor to retail's direct
// `call 0x1587c0`), then the inline ~CLoadable resets the header words and the
// real CObject grand-base sinks the 0x5e8cb4 re-stamp after them. /GX EH frame --
// Unload runs while the base subobject is still live, so its teardown is unwind-
// protected (the half-destructed-element cleanup edge).
// The cl-auto scalar-deleting destructor (vtable slot 1):
RVA_COMPGEN(0x00158660, 0x1e, ??_GLeafCue@@UAEPAXI@Z)
RVA(0x00158680, 0x5b)
LeafCue::~LeafCue() {
    Unload();
    // implicit: ~CLoadable (m_04/-1 m_08/0 m_0c/0) + the grand-base re-stamp.
}

RVA(0x001586e0, 0x34)
i32 LeafCue::LoadSoundA(void* riff) {
    SoundDevice* dev = OwnerMgr()->m_soundStream;
    if (!dev) {
        return 0;
    }
    m_10 = dev->Acquire(riff, 0x100ea, 0);
    return m_10 != 0;
}

RVA(0x00158720, 0x34)
i32 LeafCue::LoadSoundB(void* src) {
    SoundDevice* dev = OwnerMgr()->m_soundStream;
    if (!dev) {
        return 0;
    }
    m_10 = dev->AcquireFile(static_cast<char*>(src), 0x100ea, 0);
    return m_10 != 0;
}

// ---------------------------------------------------------------------------
// 0x158760: LeafCue::Configure. Parse the draw-source for its RIFF/WAVE
// blob; if the parse failed, fail. Otherwise, when the owner's SoundDevice is up,
// acquire a buffer for the blob into m_10. EndParse always runs; returns whether a
// buffer was acquired (0 when the device is down). 1 stack arg (ret 4).
// @early-stop
// 41% -- regalloc-pinning wall (docs/patterns/zero-register-pinning.md): the CFG,
// all three calls (BeginParse/Acquire/EndParse), all field stores, and the result
// merge are reproduced. MSVC5 homes the `src` param into a 3rd callee-saved register
// (ebx) and carries the return value differently than retail (which pins this->esi,
// src->edi and reuses esi as the return carrier, computing ok eagerly before
// EndParse). Tried 3 result/store spellings; no source lever flips the homing. Logic complete.
RVA(0x00158760, 0x59)
i32 LeafCue::Configure(CParseSource* src) {
    i32 blob = src->BeginParse();
    if (blob == 0) {
        return 0;
    }
    SoundDevice* dev = OwnerMgr()->m_soundStream;
    if (dev == 0) {
        src->EndParse();
        return 0;
    }
    DSoundCloneInst* buf = dev->Acquire(reinterpret_cast<void*>(blob), 0x100ea, 0);
    m_10 = buf;
    i32 ok = buf != 0;
    src->EndParse();
    return ok;
}

// ---------------------------------------------------------------------------
// 0x1587c0: LeafCue::Unload (vtable slot 7 - the CLoadable-scheme release hook; the
// ex "Release_1587c0"). When a buffer is held and the owner's SoundDevice is still
// up, remove the buffer through the device (reaps voices + releases + unlinks +
// scalar-deletes), then clear the held pointer. VOID per the CLoadable slot -
// the old i32 model was the documented "return-carrier residual" wall (~76%):
// spelling `return r` forced edi homing + mov eax,edi that retail never had.
RVA(0x001587c0, 0x23)
void LeafCue::Unload() {
    if (m_10 != 0) {
        SoundDevice* dev = OwnerMgr()->m_soundStream;
        if (dev != 0) {
            dev->RemoveBuffer(m_10);
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
i32 CAniBlitTrigger::TriggerBlit(i32 pos, i32 center, i32 range1, i32 range2) {
    if (g_sndEnabled == 0) {
        return 0;
    }
    if (center <= 0) {
        center = m_ctx->m_level->m_mainPlane->m_snappedX;
    }
    if (range1 <= 0) {
        range1 = m_ctx->m_drawTarget->m_frontPair->m_width << 2;
    }
    if (range2 <= 0) {
        range2 = m_ctx->m_drawTarget->m_frontPair->m_width / 3;
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
        vscale = static_cast<i32>((amp * (cue * g_sndPanScale)));
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
    // The real inline derived ctor: retail emits `call 0x158f30` (the out-of-line
    // CDrawSubWorker base ctor) + the own ??_7 stamp + m_surface = 0.
    CDDrawSurfaceChildA* a = new CDDrawSurfaceChildA(m_ownerCtx, 0, 0);
    m_frontPair = reinterpret_cast<CDDrawSurfacePair*>(a);

    CDDrawSurfacePair* b = static_cast<CDDrawSurfacePair*>(operator new(0x34));
    if (b != 0) {
        new (b) CDDrawSurfacePair(m_ownerCtx, 1, 0);
        b->m_width = 0;
        b->m_surface = 0;
        b->m_ownsSurface = 1;
    }
    m_backPair = b;

    CDDrawSurfacePair* c = static_cast<CDDrawSurfacePair*>(operator new(0x34));
    if (c != 0) {
        new (c) CDDrawSurfacePair(m_ownerCtx, 2, 0);
        c->m_width = 0;
        c->m_surface = 0;
        c->m_ownsSurface = 1;
    }
    m_overlayPair = c;

    if (a->SetGeometry(a1, a2, a3) == 0) { // slot-9 dispatch [vtbl+0x24] (the mode-surface creator)
        if (OwnerMgr()->m_lastError == 0) {
            OwnerMgr()->m_lastError = 0x7d1;
        }
        return 0;
    }
    if (b->Create(a1, a2, a3, 0) == 0) {
        if (OwnerMgr()->m_lastError == 0) {
            OwnerMgr()->m_lastError = 0x7d2;
        }
        return 0;
    }
    if (!(a4 & 1)) {
        if (c->Create(a1, a2, a3, 0) == 0) {
            if (OwnerMgr()->m_lastError == 0) {
                OwnerMgr()->m_lastError = 0x7d3;
            }
            return 0;
        }
    }
    return 1;
}

RVA(0x00158ac0, 0x44)
void CDDrawSubMgrPages::Unload() {
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

RVA(0x00158b10, 0x2c)
i32 CDDrawSubMgrPages::ResolvePageImage(CParseSource* src, i32 arg2) {
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
    return p->ResolveImage_163ee0(src);
}

RVA(0x00158b40, 0x2c)
i32 CDDrawSubMgrPages::LoadPageImage(CParseSource* src, i32 arg2) {
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
    return p->LoadImage(src);
}

RVA(0x00158b90, 0x28)
void CDDrawSubMgrPages::FlipAndNotify() {
    m_frontPair->m_surface->Flip(0);
    CDDrawSurfaceMgr* n = OwnerMgr();
    CDDrawChildGroup* c = n->m_childGroup;
    CDDrawSubMgrPages* s = n->m_drawTarget;
    c->WalkDispatch30(
        reinterpret_cast<i32>(s->m_backPair),
        reinterpret_cast<i32>(s->m_overlayPair)
    );
}

RVA(0x00158bc0, 0x2e)
i32 CDDrawSubMgrPages::PagesReady() {
    if (m_frontPair && !m_frontPair->Probe()) {
        return 0;
    }
    if (m_overlayPair && !m_overlayPair->RestoreIfLost()) {
        return 0;
    }
    return 1;
}

RVA(0x00158bf0, 0x7f)
i32 CDDrawSubMgrPages::ResizePages(i32 a1, i32 a2, i32 a3) {
    CDDrawSurfacePair* p = m_frontPair;
    if (p->m_width != a1 || p->m_height != a2 || p->m_bpp != a3) {
        if (!m_frontPair->SetGeom(a1, a2, a3)) {
            return 0;
        }
        if (!m_backPair->SetGeom(a1, a2, a3)) {
            return 0;
        }
        if (m_overlayPair && m_overlayPair->IsLoaded()) {
            if (!m_overlayPair->SetGeom(a1, a2, a3)) {
                return 0;
            }
        }
    }
    return 1;
}

RVA(0x00158c70, 0x36)
i32 CDDrawSubMgrPages::BlitPage(CDDrawSurfacePair* dst) {
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

RVA(0x00158cb0, 0x6a)
i32 CDDrawSubMgrPages::CreateOverlay(i32 a1, i32 a2) {
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

RVA(0x00158d20, 0x16)
i32 CDDrawSubMgrPages::HasOverlay() {
    if (!m_overlayPair) {
        return 0;
    }
    return m_overlayPair->IsLoaded() != 0;
}

RVA(0x00158d50, 0x61)
void CDDrawSubMgrPages::ClearAllPages(i32 a1) {
    m_backPair->m_surface->Fill(a1);
    m_frontPair->m_surface->Flip(0);
    m_backPair->m_surface->Fill(a1);
    m_frontPair->m_surface->Flip(0);
    if (OwnerMgr()->m_flags & 2) {
        m_backPair->m_surface->Fill(a1);
        m_frontPair->m_surface->Flip(0);
    }
}

// 0x158dc0: blt m_backPair's surface <- m_frontPair's surface; if the m_worker flag
// bit1 is set, flip m_frontPair and re-blt.
// @early-stop
// ~84% - init `ok=0` up front + only deref m_backPair->m_surface INSIDE the p10
// guard (was 71%: caching a `p14=m_backPair` local + an else{ok=0} made cl pin
// m_backPair in edi across the p10 checks and hoist/share the ok=0). Residual:
// retail keeps the m_backPair pointer in ecx (loaded early) + inlines the ok=0
// block after the checks, where cl loads m_backPair late + hoists ok=0 to the
// prologue - a regalloc/branch-layout coin-flip (flat &&-else regressed to 68%).
// docs/patterns/zero-register-pinning.md.
RVA(0x00158dc0, 0x7d)
i32 CDDrawSubMgrPages::PresentBackPage() {
    CDDrawSurfacePair* p10 = m_frontPair;
    i32 ok = 0;
    if (p10 && p10->m_surface) {
        CDDSurface* s10 = p10->m_surface;
        CDDSurface* s14 = m_backPair->m_surface;
        if (s14) {
            i32 hr = s14->Blt(s10);
            ok = (hr == 0);
        }
    }
    if (!ok) {
        return ok;
    }
    if (!(OwnerMgr()->m_flags & 2)) {
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
// ~88% - flattened the nested `if(overlay && IsLoaded){...} return 0` into a flat
// guard chain (each `if(!x) return 0`), matching retail's per-guard inline return-0
// (was 50%: the nesting made cl share ONE return-0 via `je`). Residual is a single
// tail-merge coin-flip: retail shares the FIRST guard's return-0 (m_overlayPair,
// `je shared`) while inlining the rest; cl inlines the first + shares the second.
// Hoisting `a=m_overlayPair` before the guards regressed to 57% (regalloc). Not
// steerable further. docs/patterns/zero-register-pinning.md.
RVA(0x00158e40, 0x4c)
i32 CDDrawSubMgrPages::TransEnter() {
    if (!m_overlayPair) {
        return 0;
    }
    if (!m_overlayPair->IsLoaded()) {
        return 0;
    }
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

RVA(0x00158e90, 0x47)
i32 CDDrawSubMgrPages::TransTitle() {
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

RVA(0x00158ee0, 0x47)
i32 CDDrawSubMgrPages::TransExit() {
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

RVA(0x00158f30, 0x27)
CDrawSubWorker::CDrawSubWorker(i32 a1, i32 a2, i32 a3) {
    m_id = a2;
    m_flags = a3;
    m_ownerCtx = a1;
    m_width = 0;
}
RVA_COMPGEN(0x00158f90, 0x1e, ??_GCDrawSubWorker@@UAEPAXI@Z)
// The inline ~CDrawSubWorker's linker-kept out-of-line COMDAT copy + its
// cl-generated scalar-deleting dtor (vtable slot 1):
RVA_COMPGEN(0x00158fb0, 0x19, ??1CDrawSubWorker@@UAE@XZ)

// 0x158fd0 (slot 9): SetGeometry - cache the {w,h,bpp} pixel geometry and a
// {0,0,w,h} src rect. ONE body, CDrawSubWorker's own: CDDrawSurfacePair INHERITS
// it (both vtables' slot 9 hold this RVA - no ICF in MSVC5, so a shared slot
// target can only be an inherited method; the old "shared body (ICF)" note was
// the mis-model the 2026-07-22 rebase dissolved).
// @early-stop
// 83.86% - regalloc coin-flip: retail materializes bpp up-front into edi; the
// /O2 scheduler on identical source keeps bpp in eax and loads it lazily.
RVA(0x00158fd0, 0x41)
i32 CDrawSubWorker::SetGeometry(i32 w, i32 h, i32 bpp) {
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

RVA(0x00159020, 0x55)
i32 CDrawSubWorker::SetGeom(i32 w, i32 h, i32 bpp) {
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

void* operator new(u32 n);
inline void* operator new(u32, void* p) {
    return p;
}

RVA(0x00159090, 0x24)
i32 CDDrawSurfacePair::IsLoaded() {
    if (m_surface != 0 && m_width > 0 && m_ownerCtx != 0 && m_id != -1) {
        return 1;
    }
    return 0;
}

RVA(0x001590f0, 0x56)
CDDrawSurfacePair::~CDDrawSurfacePair() {
    Unload(); // devirtualized in the dtor (slot-7 body @0x163e20)
    // ~CDrawSubWorker (m_width=0) + ~CLoadable (m_04=-1, m_flags=0, m_0c=0) fold
    // the base-field resets here, then the grand-base re-stamp - the stores the
    // old flat model spelled by hand.
}

RVA(0x00159150, 0x24)
i32 CDDrawSurfaceChildA::IsLoaded() {
    if (m_surface != 0 && m_width > 0 && m_ownerCtx != 0 && m_id != -1) {
        return 1;
    }
    return 0;
}

// 0x1591b0: ~CDDrawSurfaceChildA (the ex "WapObjBase::BaseInit" view; its ??_G
// @0x159190 is 0x1eff70's slot 1). The out-of-line body is EMPTY - retail's four
// resets (+0x04/-1, +0x10/0, +0x08/0, +0x0c/0) are the inlined ~CDrawSubWorker
// (m_width = 0) + ~CLoadable (header resets), and the entry own-vptr stamp is
// dead-stored into the final CObject grand-base re-stamp.
RVA_COMPGEN(0x00159190, 0x1e, ??_GCDDrawSurfaceChildA@@UAEPAXI@Z)
RVA(0x001591b0, 0x19)
CDDrawSurfaceChildA::~CDDrawSurfaceChildA() {
    // empty: ~CDrawSubWorker + ~CLoadable fold the resets + the grand-base stamp.
}
