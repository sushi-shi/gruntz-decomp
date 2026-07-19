// WwdObjMgr.cpp - the 0x1591e0-0x15b2b0 original TU (wave4-L dossier #15, block
// H): the CDDrawChildGroup collection obj - the object list/map ops (RemoveAll/
// InsertSorted/ForEach*/Prune*/FindBy*), the per-kind CreateObject factories +
// their CreateNamed front-ends, the per-frame kill-cue tick, LoadObjects, the
// CDDrawChildGroup walk dispatchers woven in (IDENTITY: CDDrawChildGroup IS
// CDDrawChildGroup - see DDrawChildGroup.h), the CDDrawChildGroup pair (the factory IS
// this collection under the sprite-creation view - see SpriteFactory.h), and the
// three embedded sub-object ctors at the block tail. Held at the dossier-#9
// boundaries 3/4 (0x1591e0 / 0x15b2c0); a correct partial - it may yet merge with
// its neighbors.
//
// original TU: filename unknown (@identity-TODO - no __FILE__ anchor).
//
// Field names are placeholders; only OFFSETS + emitted bytes are load-bearing.
#include <rva.h>
#include <Rez/RezAlloc.h> // RezAlloc/RezFree
#include <Io/FileMem.h>   // the serialize stream (CSerialArchive == the real CFileMemBase)

#include <DDrawMgr/DDrawChildGroup.h> // the shared object-collection manager class
#include <DDrawMgr/DDrawSubMgrPages.h> // CDDrawSubMgrPages (DrawObjectCounts m_drawTarget->m_backPair)
#include <Gruntz/SerialArchive.h> // the shared CSerialArchive stream (level reader, Read @+0x2c)
#include <Mfc.h> // CPtrList, CMapPtrToPtr (real afxcoll, for the m_10/m_map2c/m_map48 layout)
#include <Globals.h>
#include <Gruntz/Sprite.h> // CSprite (frame-data template value)
#include <DDrawMgr/AnimWorkerObj.h> // the canonical +0x7c worker/logic record (ex CWwdWorker/CLogicRecord views)
#include <Gruntz/ResolveNode.h>        // canonical CResolveNode (the factory base sub-object)
#include <Gruntz/AniAdvanceCursor.h>   // CAniAdvanceCursor (the +0x1a0 sub-object; ctor 0x15b730)
#include <Wwd/WwdFactoryObject.h>      // CWwdFactoryObject/CWwdNotifier/CDDrawRect/RectsOverlap
#include <Wwd/WwdGameObjCtor.h>        // WwdCtorBase/CWwdGameObjBaseCtor/WwdAnimWorker (ctor cluster)
#include <Gruntz/WwdGameObject.h>      // canonical flat CWwdGameObject (the managed objects)
#include <Wwd/WwdGameObjectFamily.h>   // the concrete kinds A/B/C/F + the embedded records
#include <DDrawMgr/DDrawChildGroup.h>  // CDDrawChildGroup (the walk dispatchers; IDENTITY == this)
#include <DDrawMgr/DDrawSurfaceMgr.h>  // canonical m_0c owner (InvokeCallback + m_workerCache)
#include <DDrawMgr/DDrawWorkerCache.h> // m_workerCache full type (the +0x10 name map)
#include <Gruntz/ObList.h>
#include <Gruntz/UserLogic.h>          // CGameObject - the real class of the AABB pair
                                       // BoxesOverlap_15a130 tests (was the CWwdBox view)
#include <Wwd/WwdFile.h>               // CPlaneRender (m_parent->m_24->m_5c world transform)
#include <DDrawMgr/DDrawSurfacePair.h> // CDDrawSurfacePair (DrawCount - ex the DrawHost_164380 view)
#include <Gruntz/GameLevel.h>          // CGameLevel (m_parent->m_level) + CLevelPlane
#include <Win32.h>                     // SetRect + RECT

// Engine heap allocator (operator new / RezAlloc). Reloc-masked __cdecl extern.

// The WWD object-id counter (0x61ab14; retail .data init = 1). Owner-TU definition:
// this manager stamps it into every created object's +0x188 and post-increments it
// (WwdFactoryObject + DDrawSurfaceMgr's save/load header publish share the cell).
DATA(0x0021ab14)
i32 g_wwdObjIdCounter = 1;

// Placement new: construct a sub-object in place at a factory-computed offset.
inline void* operator new(u32, void* p) {
    return p;
}

// The +0x1a0 embedded sub-object the 0x1598d0 factory placement-constructs is a
// CLoadable (base ctor 0x156cb0; via <Gruntz/ResolveNode.h> -> <Gruntz/Loadable.h>).
// The former ctor-only `CDDrawSubMgr` view is DISSOLVED (2026-07-14): that name was
// CLoadable's second identity (see Loadable.h).

// ===========================================================================
// The managed objects ARE the CWwdGameObject family (<Gruntz/WwdGameObject.h>
// flat model + <Wwd/WwdGameObjectFamily.h> concrete kinds A/B/C/F at the exact
// factory sizes 0x1dc/0x1fc/0x190/0x18c). The ex-`CWwdObject` element view is
// DISSOLVED: its m_04/m_flags/m_5c/m_60/m_74(sort key)/m_78(POSITION cache)/
// m_7c(worker)/m_188(map key) are the flat class's members at the same offsets,
// its slot-8 probe is GetTypeId, and its +0x3c dispatch is Slot3C (== Play).
// ===========================================================================
// (CDDrawGroupNode - the typed CObList node the walkers step - lives in
// <DDrawMgr/DDrawChildGroup.h> next to the list it walks.)

// The object reached via m_parent->+0x24->+0x5c is the plane/grid-owner
// CDDrawWorkerHost (ex a CImageSet3 mis-attribution - the 0x1628d0 body reads
// +0xb0 = m_spatialWorker, which the 0x18-byte record cannot hold); its
// Prune_1628d0 (0x1628d0) forwards to the spatial grid's Prune. Canonical:
// <DDrawMgr/DDrawWorkerHost.h> (included below).
#include <DDrawMgr/DDrawWorkerHost.h>

// (WwdObjDesc - the 0xa0-byte per-object level record LoadObjects reads - is
// hoisted to <DDrawMgr/DDrawChildGroup.h> next to its reader.)

// The +0x0c owner is the canonical CDDrawSurfaceMgr (<DDrawMgr/DDrawSurfaceMgr.h>):
// the former local `WwdFile` view's BuildChild @0x156a90 IS CDDrawSurfaceMgr::
// InvokeCallback (100% EXACT), and its `m_14 + 0x10` map is m_workerCache->m_10.
// SETTLED Ob-vs-Ptr (2026-07-16, mfc_class band authority): every retail Lookup
// this TU makes on that map - CreateNamed_1593e0, Find_15a8c0, LoadObjects x3 -
// calls 0x1b8008, which lies in the CMapStringToOb .obj band [0x1b7e17, 0x1b8247)
// (the class whose head ctor stamps its own ??_7; FID rows in these bands are all
// AMBIG noise). The member IS the declared CMapStringToOb, and the former
// `(CMapStringToPtr*)&...` casts were wrong-band misbindings (they made the
// recompile call 0x1b8438) - removed.

// (The former .cpp-local `CWwdGameObject`/`WwdGameObjAux` pair was a DUP of the
// canonical <Gruntz/WwdGameObject.h> class - same WriteSnapshot @0x151c00, same
// +0x7c worker; the aux's +0x18 child slot is AnimWorkerObj::m_logic, the owned
// polymorphic sub-record.)

// The factories' Build dispatches are the family kinds' OWN vtable slots
// (<Wwd/WwdGameObjectFamily.h>): 0x159600/0x1598d0 dispatch slot 10 (+0x28, the
// 4-arg build - A::Setup28 / B::Slot10_1665e0), 0x159250 slot 16 (+0x40, the C
// kind's 5-arg SetupFlagged16), 0x159440 slot 16 (+0x40, the F kind's 2-arg
// SetupDeferredV). The ex-CWwdFactoryA/CWwdFactoryB dispatch views are gone.

// (The three embedded sub-object record types - the +0x9c pair CWwdSlot9c /
// CWwdSlot9cA and the +0xb8 shadow record CWwdShadowRec - are hoisted to
// <Wwd/WwdGameObjectFamily.h>: they are member-range records of the family kinds
// (the +0xb8 one IS the E-level shadow dirty-rect block whose m_c0/m_d8 sentinels
// the family dtors clear). Their ctor bodies stay below at their retail RVAs.)

// The shared kill-cue clock (advanced once per tick) + its per-frame delta, and
// the cached timeGetTime import (bound in DirPal.cpp).
extern "C" u32 g_killCueClock;     // 0x6bf3c0 kill-cue clock (prev now)
extern "C" u32 g_engineFrameDelta; // 0x6bf3bc per-frame delta

// The manager's map key is the object id (+0x188) used as the MFC void* key.
// (Spelled through this inline - the direct `WwdKey(obj)` argument-cast
// trips the documented MSVC5 parser-state bug in this TU's include context;
// /O2 inlines it to the identical `mov reg,[obj+0x188]; push reg`.)
inline void* WwdKey(CWwdGameObject* o) {
    return reinterpret_cast<void*>(o->m_188);
}

// (The per-object expiry callback at worker+0x10 is now TYPED on the record -
// AnimWorkerObj::m_notify - so the old KillCueFn cast-at-fire-site is gone.)

// ---------------------------------------------------------------------------
// CDDrawChildGroup::ForwardTo3C (0x1591e0): forward to Slot3C.
RVA(0x001591e0, 0x5)
void CDDrawChildGroup::ForwardTo3C() {
    this->DestroyChildren();
}

// ---------------------------------------------------------------------------
// 0x1591f0: ClearAll cleanup - run m_parent->+0x24->+0x5c->0x1628d0 (when present),
// walk the +0x10 CObList destroying each node's child via its scalar-deleting
// destructor, then RemoveAll the +0x10 list and the +0x2c / +0x48 collections.
RVA(0x001591f0, 0x54)
void CDDrawChildGroup::DestroyChildren() {
    CGameLevel* p = m_parent->m_level;
    if (p != 0) {
        // m_mainPlane IS the plane/grid-owner CDDrawWorkerHost (the plane-family
        // unification: slots 9/10 of ??_7CDDrawWorkerHost are CLevelPlane methods).
        CDDrawWorkerHost* q = static_cast<CDDrawWorkerHost*>(p->m_mainPlane);
        if (q != 0) {
            q->Prune_1628d0();
        }
    }
    CDDrawGroupNode* n = reinterpret_cast<CDDrawGroupNode*>(m_list.GetHeadPosition());
    while (n != 0) {
        CDDrawGroupNode* cur = n;
        n = n->m_next;
        CWwdGameObjectE* obj = cur->m_obj;
        if (obj != 0) {
            delete obj;
        }
    }
    m_list.RemoveAll();
    m_map2c.RemoveAll();
    m_map48.RemoveAll();
}

// ===========================================================================
// 0x159250 - factory for the 0x190-byte kind. __thiscall, 7 stack args (ret 0x1c).
// @early-stop
// rezalloc-placement-new wall: construction body byte-exact, /GX EH frame absent
// (sibling of 0x159600 @ 66.6%). docs/patterns/rezalloc-placement-new-no-eh-frame.md
// ===========================================================================
RVA(0x00159250, 0x185)
CWwdGameObject*
CDDrawChildGroup::CreateObject_159250(int a1, int a2, int a3, int a4, int a5, int a6, int a7) {
    char* obj = static_cast<char*>(RezAlloc(0x190));
    CWwdGameObjectC* result; // the 0x190 kind (vtable 0x5effd0)
    if (obj != 0) {
        int root = reinterpret_cast<int>(m_parent);
        new (obj) CResolveNode(root, a1, a7);
        CWwdSlot9c* s9c = reinterpret_cast<CWwdSlot9c*>((obj + 0x9c));
        new (s9c) CWwdSlot9c();
        s9c->m_18 = 0;
        new (obj + 0xb8) CWwdShadowRec();
        new (obj + 0xdc) CString();
        // factory ctor vptr install dropped (model as compiler-emitted vtable; % ok per drive-to-0)
        *reinterpret_cast<int*>((obj + 0x5c)) = static_cast<int>(0x80000000);
        *reinterpret_cast<int*>((obj + 0x78)) = 0;
        // alloc + construct the real worker via the throwing operator new (test-else-0
        // shape == retail)
        AnimWorkerObj* worker = new AnimWorkerObj(root, a1, 0);
        *reinterpret_cast<void**>((obj + 0x7c)) = worker;
        *reinterpret_cast<int*>((obj + 0x98)) = 0;
        *reinterpret_cast<int*>((obj + 0x80)) = 0;
        *reinterpret_cast<int*>((obj + 0x88)) = 0;
        *reinterpret_cast<int*>((obj + 0x90)) = 0;
        *reinterpret_cast<int*>((obj + 0x188)) = g_wwdObjIdCounter;
        g_wwdObjIdCounter = g_wwdObjIdCounter + 1;
        // factory ctor vptr install dropped (model as compiler-emitted vtable; % ok per drive-to-0)
        *static_cast<char*>((obj + 0x18c)) = 0;
        result = reinterpret_cast<CWwdGameObjectC*>(obj);
    } else {
        result = 0;
    }
    if (result->SetupFlagged16(a2, a3, a4, a5, a6) == 0) {
        if (result != 0) {
            delete result; // virtual scalar-deleting dtor (slot 1)
        }
        return 0;
    }
    InsertSorted_159e40(static_cast<CWwdGameObject*>(static_cast<void*>(result)), 1);
    if (a7 & 0x200000) {
        // retail fires the +0x10 FN POINTER (m_notify), never a vtable slot
        result->m_7c->m_notify(reinterpret_cast<CGameObject*>(result));
    }
    return static_cast<CWwdGameObject*>(static_cast<void*>(result));
}

// CreateNamed_1593e0 (__thiscall, ret 0x1c => 7 args). Resolve `name` through the
// level string map to a value, then create the 7-arg kind with it as arg5.
// @early-stop
// 93.5% - logic byte-exact. Residual: MSVC5 schedules the `val = 0` pre-init store
// BETWEEN the two Lookup arg-pushes (push &val / [val]=0 / push name) where retail
// emits it AFTER both pushes. A non-steerable /O2 statement-scheduling coin-flip
// (permuter + map-hoist both tried). Shared with CreateNamed_1595b0/159a10/166780.
RVA(0x001593e0, 0x53)
CWwdGameObject* CDDrawChildGroup::CreateNamed_1593e0(
    int a1,
    int a2,
    int a3,
    int a4,
    const char* name,
    int a6,
    int a7
) {
    CObject* val = 0;
    m_parent->m_workerCache->m_10.Lookup(name, val);
    return CreateObject_159250(a1, a2, a3, a4, reinterpret_cast<int>(val), a6, a7);
}

// ===========================================================================
// 0x159440 - factory for the 0x18c-byte kind. __thiscall, 4 stack args (ret 0x10).
// @early-stop
// rezalloc-placement-new wall (sibling of 0x159600). frame absent, body exact.
// ===========================================================================
RVA(0x00159440, 0x170)
CWwdGameObject* CDDrawChildGroup::CreateObject_159440(int a1, int a2, int a3, int a4) {
    char* obj = static_cast<char*>(RezAlloc(0x18c));
    CWwdGameObjectF* result; // the 0x18c kind (vtable 0x5f0060)
    if (obj != 0) {
        int root = reinterpret_cast<int>(m_parent);
        new (obj) CResolveNode(root, a1, a4);
        CWwdSlot9c* s9c = reinterpret_cast<CWwdSlot9c*>((obj + 0x9c));
        new (s9c) CWwdSlot9c();
        s9c->m_18 = 0;
        new (obj + 0xb8) CWwdShadowRec();
        new (obj + 0xdc) CString();
        // factory ctor vptr install dropped (model as compiler-emitted vtable; % ok per drive-to-0)
        *reinterpret_cast<int*>((obj + 0x5c)) = static_cast<int>(0x80000000);
        *reinterpret_cast<int*>((obj + 0x78)) = 0;
        // alloc + construct the real worker via the throwing operator new (test-else-0
        // shape == retail)
        AnimWorkerObj* worker = new AnimWorkerObj(root, a1, 0);
        *reinterpret_cast<void**>((obj + 0x7c)) = worker;
        *reinterpret_cast<int*>((obj + 0x98)) = 0;
        *reinterpret_cast<int*>((obj + 0x80)) = 0;
        *reinterpret_cast<int*>((obj + 0x88)) = 0;
        *reinterpret_cast<int*>((obj + 0x90)) = 0;
        *reinterpret_cast<int*>((obj + 0x188)) = g_wwdObjIdCounter;
        g_wwdObjIdCounter = g_wwdObjIdCounter + 1;
        // factory ctor vptr install dropped (model as compiler-emitted vtable; % ok per drive-to-0)
        result = reinterpret_cast<CWwdGameObjectF*>(obj);
    } else {
        result = 0;
    }
    if (result->SetupDeferredV(a2, a3) == 0) {
        if (result != 0) {
            delete result; // virtual scalar-deleting dtor (slot 1)
        }
        return 0;
    }
    InsertSorted_159e40(static_cast<CWwdGameObject*>(static_cast<void*>(result)), 1);
    if (a4 & 0x200000) {
        // retail fires the +0x10 FN POINTER (m_notify), never a vtable slot
        result->m_7c->m_notify(reinterpret_cast<CGameObject*>(result));
    }
    return static_cast<CWwdGameObject*>(static_cast<void*>(result));
}

// CreateNamed_1595b0 (__thiscall, ret 0x10 => 4 args). Resolve `name` -> value and
// create the 4-arg kind with it substituted for arg3.
// @early-stop
// 92% - logic byte-exact; same val=0 arg-push scheduling residual as CreateNamed_1593e0.
RVA(0x001595b0, 0x44)
CWwdGameObject* CDDrawChildGroup::CreateNamed_1595b0(int a1, int a2, const char* name, int a4) {
    CObject* val = 0;
    m_parent->m_workerCache->m_10.Lookup(name, val);
    return CreateObject_159440(a1, a2, reinterpret_cast<int>(val), a4);
}

// ===========================================================================
// 0x159600 - CDDrawChildGroup::CreateObject (a.k.a. CDDrawChildGroup::CreateSpriteImpl):
// allocate + construct a 0x1dc-byte CWwdGameObject, register it in the manager
// (InsertSorted_159e40), and (when `flags & 0x200000`) kick its worker's
// slot +0x10. __thiscall, 6 stack args, ret 0x18.
// @early-stop
// RezAlloc + placement-construct EH-frame wall (docs/patterns/rezalloc-placement-
// new-no-eh-frame.md): the body is byte-exact, but MSVC5 predates placement
// operator delete so the in-place sub-object construction emits NO ctor-in-flight
// /GX EH state - retail's full `push -1 / fs:0` frame + shared jmp epilogue is
// absent, shifting every byte offset. Deferred to the final sweep when the
// wide-object ctor + a class-`operator new` real-allocator path can emit the
// retail frame. Logic/fields/offsets complete.
// ===========================================================================
RVA(0x00159600, 0x1ab)
CWwdGameObject*
CDDrawChildGroup::CreateObject_159600(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 flags) {
    char* obj = static_cast<char*>(RezAlloc(0x1dc));
    CWwdGameObjectA* result; // the 0x1dc kind (vtable 0x5f00a8)
    if (obj != 0) {
        i32 root = reinterpret_cast<i32>(m_parent);
        new (obj) CResolveNode(root, a1, flags);
        new (obj + 0x9c) CWwdSlot9cA();
        new (obj + 0xb8) CWwdShadowRec();
        new (obj + 0xdc) CString();
        // factory ctor vptr install dropped (model as compiler-emitted vtable; % ok per drive-to-0)
        *reinterpret_cast<i32*>((obj + 0x5c)) = static_cast<i32>(0x80000000);
        *reinterpret_cast<i32*>((obj + 0x78)) = 0;
        // alloc + construct the real worker via the throwing operator new (test-else-0
        // shape == retail)
        AnimWorkerObj* worker = new AnimWorkerObj(root, a1, flags);
        *reinterpret_cast<void**>((obj + 0x7c)) = worker;
        *reinterpret_cast<i32*>((obj + 0x98)) = 0;
        *reinterpret_cast<i32*>((obj + 0x80)) = 0;
        *reinterpret_cast<i32*>((obj + 0x88)) = 0;
        *reinterpret_cast<i32*>((obj + 0x90)) = 0;
        *reinterpret_cast<i32*>((obj + 0x188)) = g_wwdObjIdCounter;
        g_wwdObjIdCounter = g_wwdObjIdCounter + 1;
        new (obj + 0x1a0) CAniAdvanceCursor(root, a1, flags);
        // factory ctor vptr install dropped (model as compiler-emitted vtable; % ok per drive-to-0)
        *reinterpret_cast<i32*>((obj + 0x18c)) = -1;
        *reinterpret_cast<i32*>((obj + 0x190)) = -1;
        *reinterpret_cast<i32*>((obj + 0x198)) = 0;
        *reinterpret_cast<i32*>((obj + 0x194)) = 0;
        *reinterpret_cast<i32*>((obj + 0x19c)) = 0;
        result = reinterpret_cast<CWwdGameObjectA*>(obj);
    } else {
        result = 0;
    }
    if (result->Setup28(a2, a3, a4, a5) == 0) {
        if (result != 0) {
            delete result; // virtual scalar-deleting dtor (slot 1)
        }
        return 0;
    }
    InsertSorted_159e40(static_cast<CWwdGameObject*>(static_cast<void*>(result)), 1);
    if (flags & 0x200000) {
        // retail fires the +0x10 FN POINTER (m_notify), never a vtable slot
        result->m_7c->m_notify(reinterpret_cast<CGameObject*>(result));
    }
    return static_cast<CWwdGameObject*>(static_cast<void*>(result));
}

// ---------------------------------------------------------------------------
// The sprite object AttachSprite installs/initialises (arg0) is a wide object of
// the CWwdGameObject family: its init virtual is the same slot-10 (+0x28) 4-arg
// build the factories dispatch (the 4th arg carries the resolved CSprite template
// on this path), its +0x08 is the shared flags word, and its +0x7c worker's +0x10
// entry is the SAME callback slot TickKillCues fires (AnimWorkerObj::m_notify). The
// ex-CSprite2/CSprite2SubTable views are gone.

// ===========================================================================
// CDDrawChildGroup::CreateSprite @0x1597b0 - the public sprite-creation entry the
// HUD / level loaders call. Look the sprite TEMPLATE up by class-NAME (the `name`
// arg) in the owner's worker-cache table (m_parent->m_workerCache->m_10); on a hit forward the
// four leading build args + the resolved template + flags to CreateSpriteImpl
// @0x159600 (== CreateObject_159600 above). __thiscall, ret 0x18.
// ===========================================================================
RVA(0x001597b0, 0x57)
CGameObject* CDDrawChildGroup::CreateSprite(
    i32 kind,
    i32 geoB,
    i32 geoA,
    i32 hint,
    const char* name,
    i32 flags
) {
    CObject* tmpl_ob = 0;
    m_parent->m_workerCache->m_10.Lookup(name, tmpl_ob);
    CSprite* tmpl = static_cast<CSprite*>(tmpl_ob);
    if (!tmpl) {
        return 0;
    }
    // 0x159600 is CDDrawChildGroup::CreateObject_159600 (the factory IS the manager); the
    // old ?CreateSpriteImpl@CDDrawChildGroup@ decl was a PHANTOM second name for it.
    return static_cast<CGameObject*>(static_cast<void*>(CreateObject_159600(kind, geoB, geoA, hint, reinterpret_cast<i32>(tmpl), flags)));
}

// ===========================================================================
// CDDrawChildGroup::AttachSprite @0x159830 - initialise an already-allocated sprite
// (arg0) against a named template. Returns 1 on success. __thiscall, ret 0x18.
// @early-stop
// 99.5% ebx<->edi coloring wall: byte-identical except retail colors this->edi /
// flags->ebx while MSVC5 picks this->ebx / flags->edi (6 reg-only instr diffs); the
// instruction selection (PMF init call, m_7c dispatch) is exact, not source-steerable.
RVA(0x00159830, 0x92)
i32 CDDrawChildGroup::AttachSprite(
    CWwdGameObject* obj,
    i32 a1,
    i32 a2,
    i32 a3,
    const char* name,
    i32 flags
) {
    if (!obj) {
        return 0;
    }
    CObject* tmpl_ob = 0;
    m_parent->m_workerCache->m_10.Lookup(name, tmpl_ob);
    CSprite* tmpl = static_cast<CSprite*>(tmpl_ob);
    if (!tmpl) {
        return 0;
    }
    obj->m_flags = flags;
    if (!obj->Setup(a1, a2, a3, reinterpret_cast<i32>(tmpl))) {
        return 0;
    }
    // 0x159e40 is CDDrawChildGroup::InsertSorted_159e40 (the factory IS the object manager -
    // same `this`); bind the real method (reloc-masked ?InsertSorted_159e40@CDDrawChildGroup).
    this->InsertSorted_159e40(obj, 1);
    if (flags & 0x200000) {
        // the worker fire callback - the same slot TickKillCues fires
        obj->m_7c->m_notify(static_cast<CGameObject*>(obj));
    }
    return 1;
}

// ===========================================================================
// 0x1598d0 - factory for the 0x1fc-byte kind. __thiscall, 6 stack args (ret 0x18).
// @early-stop
// rezalloc-placement-new wall (sibling of 0x159600). frame absent, body exact.
// ===========================================================================
RVA(0x001598d0, 0x13d)
CWwdGameObject*
CDDrawChildGroup::CreateObject_1598d0(int a1, int a2, int a3, int a4, int a5, int a6) {
    char* obj = static_cast<char*>(RezAlloc(0x1fc));
    CWwdGameObjectB* result; // the 0x1fc kind (vtable 0x5f00e8)
    if (obj != 0) {
        int root = reinterpret_cast<int>(m_parent);
        new (obj) CWwdGameObjBaseCtor(root, a1, a6);
        new (obj + 0x1a0) CLoadable(root, a1, a6); // the embedded loadable (ctor 0x156cb0)
        // factory ctor vptr install dropped (model as compiler-emitted vtable; % ok per drive-to-0)
        *reinterpret_cast<int*>((obj + 0x1b0)) = 0;
        *reinterpret_cast<int*>((obj + 0x1b4)) = 0;
        *reinterpret_cast<int*>((obj + 0x1b8)) = 0;
        // factory ctor vptr install dropped (model as compiler-emitted vtable; % ok per drive-to-0)
        *reinterpret_cast<int*>((obj + 0x18c)) = -1;
        *reinterpret_cast<int*>((obj + 0x190)) = -1;
        *reinterpret_cast<int*>((obj + 0x198)) = 0;
        *reinterpret_cast<int*>((obj + 0x194)) = 0;
        *reinterpret_cast<int*>((obj + 0x19c)) = 0;
        new (obj + 0x1dc) CObList(0xa);
        // factory ctor vptr install dropped (model as compiler-emitted vtable; % ok per drive-to-0)
        *reinterpret_cast<int*>((obj + 0x1f8)) = 0;
        result = reinterpret_cast<CWwdGameObjectB*>(obj);
    } else {
        result = 0;
    }
    if (result->Setup28(a2, a3, a4, a5) == 0) {
        if (result != 0) {
            delete result; // virtual scalar-deleting dtor (slot 1)
        }
        return 0;
    }
    InsertSorted_159e40(static_cast<CWwdGameObject*>(static_cast<void*>(result)), 1);
    if (a6 & 0x200000) {
        // retail fires the +0x10 FN POINTER (m_notify), never a vtable slot
        result->m_7c->m_notify(reinterpret_cast<CGameObject*>(result));
    }
    return static_cast<CWwdGameObject*>(static_cast<void*>(result));
}

// CreateNamed_159a10 (__thiscall, ret 0x18 => 6 args). Resolve `name` -> value; if
// the lookup produced nothing, bail; else create the 6-arg kind with the value as arg5.
// @early-stop
// 94% - logic byte-exact; same val=0 arg-push scheduling residual as CreateNamed_1593e0.
RVA(0x00159a10, 0x57)
CWwdGameObject*
CDDrawChildGroup::CreateNamed_159a10(int a1, int a2, int a3, int a4, const char* name, int a6) {
    CObject* val = 0;
    m_parent->m_workerCache->m_10.Lookup(name, val);
    if (val == 0) {
        return 0;
    }
    return CreateObject_1598d0(a1, a2, a3, a4, reinterpret_cast<int>(val), a6);
}

// ---------------------------------------------------------------------------
// 0x159a70 (vtable slot 9): per-frame kill-cue tick. Advances the shared kill
// clock (when `advance`), walks the sorted list running each object's cue
// (m_7c->Consume(delta)); an expired cue (Consume==0) either decrements its
// refcount (+0x24) or fires its callback (+0x10). Objects flagged 0x10000 /
// 0x20000 are queued into two function-local static arrays; a post-pass then
// (0x10000) unlinks+destroys them (unless flag 0x800 => destroy only) and
// (0x20000) clears the flag and re-sorts them back into the list.
// @early-stop
// 93%. Residual is (1) unmatchable reloc NAMES: the function-local static CObArrays
// + their guard are compiler-mangled locals that the delinker only knows as
// DAT_006bf3a8/DAT_006bf390/DAT_006bf388, and the NAFXCW helpers (CObArray ctor/
// SetSize/SetAtGrow/atexit) are unannotated - all reloc-masked (bytes match);
// (2) a regalloc/encoding coin-flip in the two array-append count loads: retail
// pins the count in EAX (compact `a1` moffs32 form), our cl uses ecx/edx (`8b 0d`),
// a 1-byte-per-load size slip that cascades the tail offsets.
RVA(0x00159a70, 0x200)
void CDDrawChildGroup::TickKillCues_159a70(i32 advance) {
    static CObArray killQueue; // 0x6bf3a8  the 0x10000 (destroy) queue
    static CObArray sortQueue; // 0x6bf390  the 0x20000 (re-sort) queue
    killQueue.SetSize(0, -1);
    sortQueue.SetSize(0, -1);

    if (advance != 0) {
        u32 now = ::timeGetTime();
        u32 delta = now - g_killCueClock;
        g_killCueClock = now;
        g_engineFrameDelta = delta;
    }

    CDDrawGroupNode* node = reinterpret_cast<CDDrawGroupNode*>(m_list.GetHeadPosition());
    while (node != 0) {
        CDDrawGroupNode* cur = node;
        node = node->m_next;
        CWwdGameObject* obj = cur->m_wwd;
        AnimWorkerObj* rec = obj->m_7c;
        if (rec->Consume(static_cast<i32>(g_engineFrameDelta)) == 0) {
            i32* refc = &rec->m_24;
            if (*refc != 0) {
                --*refc;
            } else {
                rec->m_notify(static_cast<CGameObject*>(obj));
            }
        }
        i32 flags = obj->m_flags;
        if (flags & 0x10000) {
            killQueue.Add(reinterpret_cast<CObject*>(obj));
        } else if (flags & 0x20000) {
            sortQueue.Add(reinterpret_cast<CObject*>(obj));
        }
    }

    i32 i;
    for (i = 0; i < killQueue.GetSize(); i++) {
        CWwdGameObject* obj = reinterpret_cast<CWwdGameObject*>(killQueue.GetData()[i]);
        if (obj->m_flags & 0x80000) {
            AnimWorkerObj* rec = obj->m_7c;
            rec->m_1c = reinterpret_cast<void*>(0x1d);
            rec->m_notify(static_cast<CGameObject*>(obj));
        }
        if (obj->m_flags & 0x800) {
            if (obj != 0) {
                delete obj;
            }
        } else {
            m_list.RemoveAt(reinterpret_cast<POSITION>(obj->m_posCache));
            m_map48.RemoveKey(WwdKey(obj));
            m_map2c.RemoveKey(WwdKey(obj));
            if (obj != 0) {
                delete obj;
            }
        }
    }

    for (i = 0; i < sortQueue.GetSize(); i++) {
        CWwdGameObject* obj = reinterpret_cast<CWwdGameObject*>(sortQueue.GetData()[i]);
        obj->m_flags &= ~0x20000;
        m_list.RemoveAt(reinterpret_cast<POSITION>(obj->m_posCache));
        InsertSorted_159e40(obj, 0);
    }
}

// ---------------------------------------------------------------------------
// CDDrawChildGroup walk dispatchers (0x159c90-0x159d90): walk the +0x14 list
// dispatching one of the child's sibling virtuals per node.
// RESIDUE (~89%, WalkDispatch34/38): loop-advance scheduling plateau - retail
// keeps the live node in ESI across the virtual call and advances at the BOTTOM;
// MSVC5 floats the next-pointer load ABOVE the call via a node copy. Every loop
// form tried; the register set, offsets, arg order, and CFG are byte-exact.
RVA(0x00159c90, 0x23)
void CDDrawChildGroup::WalkDispatch2C(CDDrawSurfacePair* target) {
    CDDrawGroupNode* n = reinterpret_cast<CDDrawGroupNode*>(m_list.GetHeadPosition());
    if (n != 0) {
        do {
            CDDrawGroupNode* cur = n;
            n = n->m_next;
            cur->m_obj->Render(target);
        } while (n != 0);
    }
}

RVA(0x00159cc0, 0x2a)
void CDDrawChildGroup::WalkDispatch30(i32 a1, i32 a2) {
    CDDrawGroupNode* n = reinterpret_cast<CDDrawGroupNode*>(m_list.GetHeadPosition());
    if (n != 0) {
        do {
            CDDrawGroupNode* cur = n;
            n = n->m_next;
            cur->m_obj->BltDirty(reinterpret_cast<CDDrawSurfacePair*>(a1), reinterpret_cast<CDDrawSurfacePair*>(a2));
        } while (n != 0);
    }
}

RVA(0x00159cf0, 0x42)
void CDDrawChildGroup::WalkDispatch34(i32 a1, i32 a2, i32 a3) {
    CDDrawGroupNode* n = reinterpret_cast<CDDrawGroupNode*>(m_list.GetHeadPosition());
    if (n != 0) {
        do {
            n->m_obj->BltDirtyEx(reinterpret_cast<CDDrawSurfacePair*>(a1), reinterpret_cast<CDDrawSurfacePair*>(a2), a3);
            n = n->m_next;
        } while (n != 0);
    }
    WalkDispatch30(a2, a3);
}

RVA(0x00159d40, 0x42)
void CDDrawChildGroup::WalkDispatch38(i32 a1, i32 a2, i32 a3) {
    CDDrawGroupNode* n = reinterpret_cast<CDDrawGroupNode*>(m_list.GetHeadPosition());
    if (n != 0) {
        do {
            n->m_obj->BltDirtyRegions(reinterpret_cast<CDDrawSurfacePair*>(a1), reinterpret_cast<CDDrawSurfacePair*>(a2), a3);
            n = n->m_next;
        } while (n != 0);
    }
    WalkDispatch30(a2, a3);
}

// ---------------------------------------------------------------------------
// Walk the +0x14 list setting each child's field at +0xd8 to -1.
RVA(0x00159d90, 0x1c)
void CDDrawChildGroup::ResetChildD8() {
    CDDrawGroupNode* n = reinterpret_cast<CDDrawGroupNode*>(m_list.GetHeadPosition());
    if (n != 0) {
        do {
            CDDrawGroupNode* cur = n;
            n = n->m_next;
            cur->m_obj->m_d8 = -1;
        } while (n != 0);
    }
}

// ---------------------------------------------------------------------------
// 0x159db0: retire `obj` - when it is transient (flag 0x800) just delete it;
// otherwise unlink it from the list (its cached POSITION) and both maps, then
// delete it. __thiscall, 1 arg (ret 4).
RVA(0x00159db0, 0x5e)
void CDDrawChildGroup::RemoveAndDelete_159db0(CWwdGameObject* obj) {
    if (obj->m_flags & 0x800) {
        delete obj;
        return;
    }
    m_list.RemoveAt(reinterpret_cast<POSITION>(obj->m_posCache));
    m_map48.RemoveKey(WwdKey(obj));
    m_map2c.RemoveKey(WwdKey(obj));
    delete obj;
}

// ---------------------------------------------------------------------------
// 0x159e10: clear obj's re-sort flag (0x20000), unlink it from the list at its
// cached POSITION, then re-insert it in sorted order. __thiscall, 1 arg (ret 4).
RVA(0x00159e10, 0x2e)
void CDDrawChildGroup::ReinsertUnflagged_159e10(CWwdGameObject* obj) {
    obj->m_flags &= 0xfffdffff;
    m_list.RemoveAt(reinterpret_cast<POSITION>(obj->m_posCache));
    InsertSorted_159e40(obj, 0);
}

// ---------------------------------------------------------------------------
// 0x159e40: register `obj` - if its flag 0x800 is set, clear its POSITION cache
// and bail; otherwise (when addToMaps) record it in both maps, then insert it
// into the sorted list before the first node whose object has a larger sort key
// and lacks flag 0x20000. The returned POSITION is cached in obj->+0x78.
// @early-stop
// 97.75% - code bytes match retail; residual is the reloc-typing scoring
// artifact on the two CMapPtrToPtr::operator[] calls (REL32 vs cl's DIR32 vs a
// differently-named symbol). docs/patterns + objdiff-reloc-scoring.
RVA(0x00159e40, 0xaa)
void CDDrawChildGroup::InsertSorted_159e40(CWwdGameObject* obj, i32 addToMaps) {
    if (obj->m_flags & 0x800) {
        obj->m_posCache = 0;
        return;
    }
    if (addToMaps != 0) {
        m_map2c[WwdKey(obj)] = obj;
        m_map48[WwdKey(obj)] = obj;
    }
    CDDrawGroupNode* node = reinterpret_cast<CDDrawGroupNode*>(m_list.GetHeadPosition());
    i32 key = obj->m_latchedAnimId;
    while (node != 0) {
        CDDrawGroupNode* cur = node;
        CWwdGameObject* data = cur->m_wwd;
        node = node->m_next;
        if (data->m_latchedAnimId > key && !(data->m_flags & 0x20000)) {
            obj->m_posCache = reinterpret_cast<i32>(m_list.InsertBefore(reinterpret_cast<POSITION>(cur), reinterpret_cast<CObject*>(obj)));
            return;
        }
    }
    obj->m_posCache = reinterpret_cast<i32>(m_list.AddTail(reinterpret_cast<CObject*>(obj)));
}

// CDDrawChildGroup::DestroyChildren_159ef0 (0x159ef0): non-virtual entry that
// virtual-dispatches slot 15 - `mov eax,[ecx]; jmp [eax+0x3c]`. Receiver proven:
// CDDrawSurfaceMgr::RestoreChildren calls it on [this+0x8] (the child group), and
// +0x3c is only in-bounds on this 17-slot vtable (the old CDDrawSubMgrPages
// attribution read past that class's 10-slot table into ??_7CFileMem).
RVA(0x00159ef0, 0x5)
void CDDrawChildGroup::DestroyChildren_159ef0() {
    DestroyChildren();
}

// ---------------------------------------------------------------------------
// 0x159f00: CDDrawChildGroup::Slot40 (vtable slot 16, @+0x40). Pairwise
// collision broadcast over the +0x14 child list: for every ordered pair (i<j)
// of active objects (flag bit0 clear) that share the 0x40000 class bit, test
// overlap and, on a hit, fire the registered +0x80/+0x88 collision callbacks,
// the +0x128 damage budget latch, or CWwdFactoryObject::Notify. Two phases:
// a RECT overlap (skipped when i&4 or j&0x80) using each object's +0x144.. AABB
// and RectsOverlap, then a BOX overlap (skipped when j&4 or i&0x80) via
// BoxesOverlap. __thiscall, no args.
// @early-stop
// 87.9% - logic/CFG/field-offsets/arg-order byte-identical. Residual is a
// zero-register-pinning / dead-spill wall: retail spills `this` to [esp] at
// entry and reloads it (dead) before the stdcall BoxesOverlap, giving a 0x30
// frame; our cl never spills the unused `this` (0x2c frame), shifting every stack
// slot offset + rotating the mask temp register (retail ebp vs our ecx). No
// source lever forces a dead self-spill (docs/patterns/zero-register-pinning.md).
// Forward decl for the Slot40 body (definition follows at 0x15a130 in RVA order):
// the box-overlap predicate over two CGameObjects (<Gruntz/UserLogic.h>; the old
// CWwdBox fwd decl mismatched the definition and left the call reloc UNBOUND).
i32 __stdcall BoxesOverlap_15a130(CGameObject* a1, CGameObject* a2);

// NOTE: the inner-pair body is flattened with `continue` guards (identical CFG;
// MSVC5's parser corrupts its state on the fully-nested spelling in THIS include
// context - a compiler front-end bug, verified by bisection; the nested form
// compiled fine in the smaller pre-split TU).
RVA(0x00159f00, 0x22e)
void CDDrawChildGroup::CollideBroadcast() {
    CDDrawGroupNode* outer = reinterpret_cast<CDDrawGroupNode*>(m_list.GetHeadPosition());
    while (outer != 0) {
        char* oi = reinterpret_cast<char*>(outer->m_obj);
        CDDrawGroupNode* nextOuter = outer->m_next;
        if (!(*reinterpret_cast<i32*>((oi + 8)) & 1)) {
            CDDrawGroupNode* inner = nextOuter;
            for (; inner != 0; inner = inner->m_next) {
                char* oj = reinterpret_cast<char*>(inner->m_obj);
                i32 fj = *reinterpret_cast<i32*>((oj + 8));
                if (fj & 1) {
                    continue;
                }
                i32 fi = *reinterpret_cast<i32*>((oi + 8));
                if ((fi ^ fj) & 0x40000) {
                    continue;
                }
                // --- RECT PHASE (skipped when i&4 or j&0x80) ---
                if (!(fi & 4) && !(fj & 0x80)) {
                    i32 mask1 = *reinterpret_cast<i32*>((oj + 0xe8)) & *reinterpret_cast<i32*>((oi + 0xec));
                    i32 mask2 = *reinterpret_cast<i32*>((oi + 0xe8)) & *reinterpret_cast<i32*>((oj + 0xf0));
                    if (mask1 || mask2) {
                        i32 overlap;
                        if (*reinterpret_cast<i32*>((oj + 0x154)) == static_cast<i32>(0x80000000)) {
                            overlap = 0;
                        } else if (*reinterpret_cast<i32*>((oi + 0x144)) == static_cast<i32>(0x80000000)) {
                            overlap = 0;
                        } else {
                            CDDrawRect ra, rb;
                            i32 xi = *reinterpret_cast<i32*>((oi + 0x5c));
                            i32 yi = *reinterpret_cast<i32*>((oi + 0x60));
                            ra.left = *(i32*)(oi + 0x144) + xi;
                            ra.top = *(i32*)(oi + 0x148) + yi;
                            ra.right = *(i32*)(oi + 0x14c) + xi;
                            ra.bottom = *(i32*)(oi + 0x150) + yi;
                            i32 xj = *reinterpret_cast<i32*>((oj + 0x5c));
                            i32 yj = *reinterpret_cast<i32*>((oj + 0x60));
                            rb.left = *(i32*)(oj + 0x154) + xj;
                            rb.top = *(i32*)(oj + 0x158) + yj;
                            rb.right = *(i32*)(oj + 0x15c) + xj;
                            rb.bottom = *(i32*)(oj + 0x160) + yj;
                            overlap = RectsOverlap_15bfb0(&ra, &rb);
                        }
                        if (overlap) {
                            if (mask2) {
                                AnimWorkerObj* nf = *reinterpret_cast<AnimWorkerObj**>((oj + 0x88));
                                if (nf != 0) {
                                    *reinterpret_cast<void**>((oj + 0x8c)) = oi;
                                    nf->m_notify(reinterpret_cast<CGameObject*>(oj));
                                }
                            }
                            if (mask1) {
                                if (*reinterpret_cast<i32*>((oi + 8)) & 8) {
                                    i32 v = *(i32*)(oi + 0x128) - *reinterpret_cast<i32*>((oj + 0x120));
                                    *reinterpret_cast<i32*>((oi + 0x128)) = v;
                                    if (v <= 0) {
                                        *reinterpret_cast<i32*>((*(char**)(oi + 0x7c) + 0x1c)) = 0x1c;
                                    }
                                } else {
                                    AnimWorkerObj* nf = *reinterpret_cast<AnimWorkerObj**>((oi + 0x80));
                                    if (nf != 0) {
                                        *reinterpret_cast<void**>((oi + 0x84)) = oj;
                                        nf->m_notify(reinterpret_cast<CGameObject*>(oi));
                                    }
                                }
                            }
                        }
                    }
                }
                // --- BOX PHASE (skipped when j&4 or i&0x80) ---
                if (*reinterpret_cast<i32*>((oj + 8)) & 4) {
                    continue;
                }
                if (*reinterpret_cast<i32*>((oi + 8)) & 0x80) {
                    continue;
                }
                i32 mask1b = *reinterpret_cast<i32*>((oj + 0xec)) & *reinterpret_cast<i32*>((oi + 0xe8));
                i32 mask2b = *reinterpret_cast<i32*>((oj + 0xe8)) & *reinterpret_cast<i32*>((oi + 0xf0));
                if ((mask1b || mask2b) && BoxesOverlap_15a130(reinterpret_cast<CGameObject*>(oj), reinterpret_cast<CGameObject*>(oi))) {
                    if (mask2b) {
                        AnimWorkerObj* nf = *reinterpret_cast<AnimWorkerObj**>((oi + 0x88));
                        if (nf != 0) {
                            *reinterpret_cast<void**>((oi + 0x8c)) = oj;
                            nf->m_notify(reinterpret_cast<CGameObject*>(oi));
                        }
                    }
                    if (mask1b) {
                        (reinterpret_cast<CWwdGameObjectE*>(oj))->Notify_15b650(oi);
                    }
                }
            }
        }
        outer = nextOuter;
    }
}

// 0x15a130: bounding-box overlap test between two wide objects. Each box is its
// screen pos (+0x5c/+0x60) plus a local AABB (a1: +0x144..+0x150; a2: +0x154..
// +0x160). Either box invalid (its first AABB field == INT_MIN) -> no overlap.
// __stdcall, 2 args (ret 0x8).
// @early-stop
// 76% - logic/CFG/offsets/compares byte-exact; the residual is a spill-frame
// strategy difference: retail allocates a fresh `sub esp,0x20` local frame for the
// 3 spilled box edges, our cl reuses the incoming arg stack slots - shifting every
// spill offset + rotating esi/edi. A non-steerable codegen heuristic
// (zero-register-pinning family).
// The two boxes are CGameObjects (<Gruntz/UserLogic.h>) - the `CWwdBox` view is gone. Every
// field it declared is a canonical CGameObject member at the same offset: the screen
// position (m_screenX @+0x5c / m_screenY @+0x60) and the two 4-dword boxes at +0x144 and
// +0x154 (m_areaL/T/R/B - "the derived activation/stand box L/T/R/B" - and the m_154 quad).
// The 0x80000000 "invalid" sentinel this reads is the SAME unset marker CGameObject's
// extent/area block documents.
RVA(0x0015a130, 0xdc)
i32 __stdcall BoxesOverlap_15a130(CGameObject* a1, CGameObject* a2) {
    if (a2->m_154 == static_cast<i32>(0x80000000)) {
        return 0;
    }
    if (a1->m_areaL == static_cast<i32>(0x80000000)) {
        return 0;
    }
    i32 a1L = a1->m_areaL + a1->m_screenX;
    i32 a1R = a1->m_areaR + a1->m_screenX;
    i32 a1T = a1->m_areaT + a1->m_screenY;
    i32 a1B = a1->m_areaB + a1->m_screenY;
    i32 a2L = a2->m_154 + a2->m_screenX;
    i32 a2T = a2->m_158 + a2->m_screenY;
    i32 a2B = a2->m_160 + a2->m_screenY;
    i32 a2R = a2->m_15c + a2->m_screenX;
    if (a1L > a2R) {
        return 0;
    }
    if (a1R < a2L) {
        return 0;
    }
    if (a1T > a2B) {
        return 0;
    }
    return a1B >= a2T;
}

// @early-stop
// 0x15a210 (1074 B) = a CDDrawChildGroup-family debug OVERLAY, twin of
// DrawObjectCounts_15a650 (same subsystem, both dead-in-retail). __thiscall, gated
// on a +0x08 debug flag; walks the +0x14 child list and per object draws debug
// geometry via CPlaneRender::WrapCoord: CDDrawSurfacePair::DrawBox(RECT*,color) x3,
// DrawCross(x,y), ResLoaders::DrawHost2_164420::DrawLabel(RECT*,char*) (falling
// back to "???" @0x1f0a94). Draws gated by g_dbg61ab28/2c/30. Held pending
// reconstruction (>512 B, novel per-object geometry).
RVA(0x0015a210, 0x432)
i32 Gap_15a210(void) {
    return 0;
}

// ---------------------------------------------------------------------------
// 0x15a650: per-object debug-count overlay. Only active when the +0x08 flag word
// carries 0x200000. For each child in the +0x14 list, box its screen position,
// world-wrap the top-left into the visible range, transform the bottom-right
// through the viewport's WrapCoord, and draw the object's +0x74 count into the
// resulting rect via the counter draw-host (m_parent->m_4->m_14). __thiscall.
// @early-stop
// 86.8% - logic/CFG/field-offsets/arg-order byte-identical. Residual is a
// zero-register-pinning wall: retail rotates the (drawHost/view/obj/box) live
// values through edx/ecx/eax/ebx where our cl picks ecx/eax/edx/ebx, and
// allocates one fewer scratch slot - flipping the ModRM byte of most accesses.
RVA(0x0015a650, 0x12c)
void CDDrawChildGroup::DrawObjectCounts_15a650() {
    if (!(m_flags08 & 0x200000)) {
        return;
    }
    CDDrawGroupNode* node = reinterpret_cast<CDDrawGroupNode*>(m_list.GetHeadPosition());
    CDDrawSurfacePair* drawHost = m_parent->m_drawTarget->m_backPair;
    CPlaneRender* view = m_parent->m_level->m_mainPlane;
    if (node == 0) {
        return;
    }
    do {
        char* obj = reinterpret_cast<char*>(node->m_obj);
        node = node->m_next;
        i32 ox = *reinterpret_cast<i32*>((obj + 0x5c));
        i32 oy = *reinterpret_cast<i32*>((obj + 0x60));
        RECT box;
        SetRect(&box, ox - 0x20, oy - 8, ox + 0x20, oy + 8);
        RECT rc;
        rc.right = box.right;
        rc.bottom = box.bottom;
        i32 wl = box.left;
        i32 wt = box.top;
        i32 fl = view->m_flags;
        if (fl & 4) {
            i32 w = view->m_wrapW;
            if (box.left < 0) {
                wl = box.left + w;
            } else if (box.left >= w) {
                wl = box.left - w;
            }
            i32 farEdge = view->m_extentX;
            if (farEdge >= w && wl < view->m_originX && wl <= farEdge - w) {
                wl += w;
            }
        }
        if (fl & 8) {
            i32 h = view->m_wrapH;
            if (box.top < 0) {
                wt = box.top + h;
            } else if (box.top >= h) {
                wt = box.top - h;
            }
            i32 farEdge = view->m_extentY;
            if (farEdge >= h && wt < view->m_originY && wt <= farEdge - h) {
                wt += h;
            }
        }
        rc.left = wl - view->m_originX + view->m_viewX;
        rc.top = wt - view->m_originY + view->m_viewY;
        view->WrapCoord(reinterpret_cast<i32*>(&rc.right), reinterpret_cast<i32*>(&rc.bottom)); // LONG*->i32* (same width; PAH sig)
        drawHost->DrawCount(&rc, *reinterpret_cast<i32*>((obj + 0x74)));
    } while (node != 0);
}

// ---------------------------------------------------------------------------
// 0x15a780: walk the sorted list; advance past the leading run of objects that
// carry the 0x20000 flag to the first object WITHOUT it (the "anchor"), then scan
// the rest. Each subsequent un-flagged object with a sort key >= the anchor's
// becomes the new anchor; one with a SMALLER key triggers a status-probe (slot
// +0x20) on BOTH the anchor and the offender. Always returns 1.
// @early-stop
// 78.7% - loop B, the phase-2 setup and the epilogue are byte-exact; the residual
// is the loop-A rotation only (cl rotates the loop so the 0x20000 flag-test
// becomes the header). Tried while / for(;;) / do-while / explicit-goto - a
// codegen loop-rotation wall, not a source-shape bug.
RVA(0x0015a780, 0x70)
i32 CDDrawChildGroup::CheckSortOrder_15a780() {
    CDDrawGroupNode* node = reinterpret_cast<CDDrawGroupNode*>(m_list.GetHeadPosition());
    CWwdGameObject* anchor = node->m_wwd;
    node = node->m_next;
    if (anchor == 0) {
        return 1;
    }
    if (node != 0) {
        do {
            if (anchor == 0) {
                return 1;
            }
            if ((anchor->m_flags & 0x20000) == 0) {
                break;
            }
            CDDrawGroupNode* cur = node;
            node = node->m_next;
            anchor = cur->m_wwd;
        } while (node != 0);
    }
    if (anchor == 0) {
        return 1;
    }
    i32 key = anchor->m_latchedAnimId;
    if (node == 0) {
        return 1;
    }
    do {
        CDDrawGroupNode* cur = node;
        node = node->m_next;
        CWwdGameObject* obj = cur->m_wwd;
        if ((obj->m_flags & 0x20000) == 0) {
            i32 curKey = obj->m_latchedAnimId;
            if (key > curKey) {
                anchor->GetTypeId();
                obj->GetTypeId();
            } else {
                key = curKey;
                anchor = obj;
            }
        }
    } while (node != 0);
    return 1;
}

// ---------------------------------------------------------------------------
// 0x15a7f0: scan the sorted list for the first object whose +0x04 key matches
// `type`; return it, or 0 when none.
RVA(0x0015a7f0, 0x20)
CWwdGameObject* CDDrawChildGroup::FindByType04_15a7f0(i32 type) {
    CDDrawGroupNode* node = reinterpret_cast<CDDrawGroupNode*>(m_list.GetHeadPosition());
    while (node != 0) {
        CDDrawGroupNode* cur = node;
        node = node->m_next;
        CWwdGameObject* obj = cur->m_wwd;
        if (obj->m_04 == type) {
            return obj;
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// 0x15a810: scan the sorted list for the first object whose status probe (slot
// +0x20) is 5 AND whose +0x04 key matches `type`; return it, or 0 when none.
RVA(0x0015a810, 0x42)
CWwdGameObject* CDDrawChildGroup::FindByTypeProbe_15a810(i32 type) {
    CDDrawGroupNode* node = reinterpret_cast<CDDrawGroupNode*>(m_list.GetHeadPosition());
    while (node != 0) {
        CDDrawGroupNode* cur = node;
        node = node->m_next;
        CWwdGameObject* obj = cur->m_wwd;
        if (obj->GetTypeId() == 5 && obj->m_04 == type) {
            return obj;
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// 0x15a860: scan the sorted list for the first object whose status probe (slot
// +0x20) is 5, whose +0x04 key matches `type`, and whose worker's +0x10 geometry
// matches the requested key's +0x10. Returns 0 if none.
// @early-stop
// 86% - logic/offsets/CFG byte-exact; the residual is the loop-tail epilogue:
// retail bottom-tests `jne looptop` and falls through to a SEPARATE `xor eax,eax`
// return-0, our cl shares one return-0. The documented loop-epilogue-merge wall.
RVA(0x0015a860, 0x57)
CWwdGameObject* CDDrawChildGroup::FindByWorker_15a860(i32 type, void* key) {
    CDDrawGroupNode* node = reinterpret_cast<CDDrawGroupNode*>(m_list.GetHeadPosition());
    if (node == 0) {
        return 0;
    }
    do {
        CDDrawGroupNode* cur = node;
        node = node->m_next;
        CWwdGameObject* obj = cur->m_wwd;
        if (obj->GetTypeId() == 5 && *reinterpret_cast<i32*>((reinterpret_cast<char*>(obj) + 0x4)) == type) {
            void* worker = *reinterpret_cast<void**>((reinterpret_cast<char*>(obj) + 0x7c));
            if (*reinterpret_cast<i32*>((reinterpret_cast<char*>(worker) + 0x10)) == *reinterpret_cast<i32*>((reinterpret_cast<char*>(key) + 0x10))) {
                return obj;
            }
        }
    } while (node != 0);
    return 0;
}

// ---------------------------------------------------------------------------
// @identity-TODO: 0x15a8c0 unreferenced (dead / inlined-away), owner unrecovered.
// `this`: a parent back-pointer @+0x0c and an intrusive child-list head @+0x14.
// It Lookups `key` in the CMapStringToOb at parent->m_14 +0x10, then scans the
// child list for the game object whose type tag (vtable slot 8, @+0x20) == 5,
// whose +0x04 id == `id`, and whose +0x7c sub-object's +0x10 equals the looked-up
// object's +0x10.
// @early-stop
// 79% - logic/CFG/offsets/vtable-dispatch byte-faithful. Residual is a regalloc/
// branch-layout wall: retail pins `found` in ebp and the node in eax (twin-copy),
// allocates the out slot 4 B higher, and lays out the match path as fall-through -
// same values, same stores (docs/patterns/zero-register-pinning.md).
// The `CChildFinder_15a8c0` placeholder class is GONE: it WAS this manager. Its two fields
// were CDDrawChildGroup's own - m_parent @+0x0c is m_0c (the CDDrawSurfaceMgr owner, whose
// +0x14 worker cache holds the name map this looks the key up in, exactly as
// CreateNamed_1593e0/1595b0 do) and m_listHead @+0x14 is m_10's head node (the CObList sits
// at +0x10). So Find_15a8c0 is a plain CDDrawChildGroup method, walking the same list with the
// same `reinterpret_cast<CDDrawGroupNode*>(m_list.GetHeadPosition())` idiom every sibling FindBy* here uses.
RVA(0x0015a8c0, 0x7d)
void* CDDrawChildGroup::Find_15a8c0(i32 id, const char* key) {
    CObject* found = 0;
    m_parent->m_workerCache->m_10.Lookup(key, found);
    char* node = reinterpret_cast<char*>(m_list.GetHeadPosition());
    if (node == 0) {
        return 0;
    }
    char* fp = reinterpret_cast<char*>(found);
    do {
        char* obj = *reinterpret_cast<char**>((node + 8));
        node = *reinterpret_cast<char**>(node);
        i32 tag = (reinterpret_cast<CWwdGameObjectE*>(obj))->GetClassId(); // vtable slot 8 (the type tag)
        if (tag == 5 && *reinterpret_cast<i32*>((obj + 4)) == id
            && *reinterpret_cast<i32*>((*(char**)(obj + 0x7c) + 0x10)) == *reinterpret_cast<i32*>((fp + 0x10))) {
            return obj;
        }
    } while (node != 0);
    return 0;
}

// ---------------------------------------------------------------------------
// 0x15a940: the +0xe8-field twin of FindByWorker_15a860.
// @early-stop
// 85% - same loop-epilogue-merge wall as FindByWorker_15a860.
RVA(0x0015a940, 0x52)
CWwdGameObject* CDDrawChildGroup::FindByField_15a940(i32 type, void* key) {
    CDDrawGroupNode* node = reinterpret_cast<CDDrawGroupNode*>(m_list.GetHeadPosition());
    if (node == 0) {
        return 0;
    }
    do {
        CDDrawGroupNode* cur = node;
        node = node->m_next;
        CWwdGameObject* obj = cur->m_wwd;
        if (obj->GetTypeId() == 5 && *reinterpret_cast<i32*>((reinterpret_cast<char*>(obj) + 0x4)) == type
            && *reinterpret_cast<void**>((reinterpret_cast<char*>(obj) + 0xe8)) == key) {
            return obj;
        }
    } while (node != 0);
    return 0;
}

// ---------------------------------------------------------------------------
// 0x15a9a0: return the first list object whose map key (+0x188) equals `key`.
RVA(0x0015a9a0, 0x23)
CWwdGameObject* CDDrawChildGroup::FindByKey_15a9a0(void* key) {
    CDDrawGroupNode* node = reinterpret_cast<CDDrawGroupNode*>(m_list.GetHeadPosition());
    while (node != 0) {
        CDDrawGroupNode* cur = node;
        node = node->m_next;
        CWwdGameObject* obj = cur->m_wwd;
        if (WwdKey(obj) == key) {
            return obj;
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// 0x15a9d0: return the first list object whose status probe (slot +0x20) is 5 and
// whose map key (+0x188) equals `key`.
RVA(0x0015a9d0, 0x45)
CWwdGameObject* CDDrawChildGroup::FindByStatusKey_15a9d0(void* key) {
    CDDrawGroupNode* node = reinterpret_cast<CDDrawGroupNode*>(m_list.GetHeadPosition());
    while (node != 0) {
        CDDrawGroupNode* cur = node;
        node = node->m_next;
        CWwdGameObject* obj = cur->m_wwd;
        if (obj->GetTypeId() == 5 && WwdKey(obj) == key) {
            return obj;
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// 0x15aa20: uniqueness predicate - return 1 unless two or more list objects share
// +0x04 == `kind`.
RVA(0x0015aa20, 0x3c)
i32 CDDrawChildGroup::IsKindUnique_15aa20(i32 kind) {
    CWwdGameObject* found = 0;
    CDDrawGroupNode* node = reinterpret_cast<CDDrawGroupNode*>(m_list.GetHeadPosition());
    while (node != 0) {
        CDDrawGroupNode* cur = node;
        node = node->m_next;
        CWwdGameObject* obj = cur->m_wwd;
        if (obj->m_04 == kind) {
            if (found != 0) {
                return 0;
            }
            found = obj;
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// 0x15aa60: count the list objects whose +0x04 == `kind`.
RVA(0x0015aa60, 0x23)
i32 CDDrawChildGroup::CountByKind_15aa60(i32 kind) {
    i32 count = 0;
    CDDrawGroupNode* node = reinterpret_cast<CDDrawGroupNode*>(m_list.GetHeadPosition());
    while (node != 0) {
        CDDrawGroupNode* cur = node;
        node = node->m_next;
        CWwdGameObject* obj = cur->m_wwd;
        if (obj->m_04 == kind) {
            ++count;
        }
    }
    return count;
}

// ---------------------------------------------------------------------------
// 0x15aa90: walk the list; for every object lacking flag 0x200, drop it from
// the list + both maps and destroy it.
// @early-stop
// 95.6% - list-walk twin-copy regalloc wall: retail materializes the cur node
// in two registers (eax+ecx); our cl keeps cur in one reg. Logic/CFG/offsets
// exact. docs/patterns/linked-list-walk-node-eax-rotation.md.
RVA(0x0015aa90, 0x5d)
void CDDrawChildGroup::PruneList_15aa90() {
    CDDrawGroupNode* node = reinterpret_cast<CDDrawGroupNode*>(m_list.GetHeadPosition());
    while (node != 0) {
        CDDrawGroupNode* cur = node;
        node = node->m_next;
        CWwdGameObject* obj = cur->m_wwd;
        if (obj != 0 && !(obj->m_flags & 0x200)) {
            m_list.RemoveAt(reinterpret_cast<POSITION>(cur));
            m_map2c.RemoveKey(WwdKey(obj));
            m_map48.RemoveKey(WwdKey(obj));
            delete obj;
        }
    }
}

// ---------------------------------------------------------------------------
// 0x15aaf0: accumulate SUM over the list of index*(obj->m_screenX + m_74 + m_60 + m_04).
// @early-stop
// 99.15% - logic/CFG/offsets byte-exact. Residual: cl reassociates the 4-term
// commutative sum to load m_74 into the accumulator first, where retail loads m_5c
// first. Documented add-reassociation wall (permuter no-op).
RVA(0x0015aaf0, 0x35)
i32 CDDrawChildGroup::SumWeighted_15aaf0() {
    i32 sum = 0;
    i32 i = 0;
    CDDrawGroupNode* node = reinterpret_cast<CDDrawGroupNode*>(m_list.GetHeadPosition());
    while (node != 0) {
        CDDrawGroupNode* cur = node;
        node = node->m_next;
        CWwdGameObject* obj = cur->m_wwd;
        sum += i * (obj->m_screenX + obj->m_latchedAnimId + obj->m_screenY + obj->m_04);
        ++i;
    }
    return sum;
}

// ---------------------------------------------------------------------------
// 0x15ab30: drop a list slot + BOTH map entries (the +0x2c primary AND the
// +0x48 active set).
RVA(0x0015ab30, 0x38)
void CDDrawChildGroup::RemoveAll_15ab30(i32 pos, CWwdGameObject* obj) {
    m_list.RemoveAt(reinterpret_cast<POSITION>(pos));
    m_map2c.RemoveKey(WwdKey(obj));
    m_map48.RemoveKey(WwdKey(obj));
}

// ---------------------------------------------------------------------------
// 0x15ab70: drop a list slot + its primary-map entry.
RVA(0x0015ab70, 0x27)
void CDDrawChildGroup::RemoveByPosition_15ab70(i32 pos, CWwdGameObject* obj) {
    m_list.RemoveAt(reinterpret_cast<POSITION>(pos));
    m_map2c.RemoveKey(WwdKey(obj));
}

// ---------------------------------------------------------------------------
// 0x15aba0: m_map48[obj->key] = obj.
RVA(0x0015aba0, 0x1a)
void CDDrawChildGroup::AddToMap48_15aba0(CWwdGameObject* obj) {
    m_map48[WwdKey(obj)] = obj;
}

// ---------------------------------------------------------------------------
// 0x15abc0: count m_map48 entries whose object lacks flag 0x4000000.
RVA(0x0015abc0, 0x5e)
i32 CDDrawChildGroup::CountActive_15abc0() {
    i32 n = 0;
    POSITION pos = reinterpret_cast<POSITION>((m_map48.GetCount() != 0 ? -1 : 0));
    if (pos != 0) {
        do {
            void* key = 0;
            CWwdGameObject* val = 0;
            m_map48.GetNextAssoc(pos, key, reinterpret_cast<void*&>(val));
            if (val != 0 && !(val->m_flags & 0x4000000)) {
                ++n;
            }
        } while (pos != 0);
    }
    return n;
}

// ---------------------------------------------------------------------------
// 0x15ac20: for each active m_map48 object, dispatch its +0x3c virtual with the
// three args + the object; always returns 1. Returns 0 immediately if a1==0.
RVA(0x0015ac20, 0x81)
i32 CDDrawChildGroup::ForEachDispatch_15ac20(i32 a1, i32 a2, i32 a3) {
    if (a1 == 0) {
        return 0;
    }
    POSITION pos = reinterpret_cast<POSITION>((m_map48.GetCount() != 0 ? -1 : 0));
    if (pos != 0) {
        do {
            void* key = 0;
            CWwdGameObject* val = 0;
            m_map48.GetNextAssoc(pos, key, reinterpret_cast<void*&>(val));
            if (val != 0 && !(val->m_flags & 0x4000000)) {
                val->Slot3C(a1, a2, a3, val);
            }
        } while (pos != 0);
    }
    return 1;
}

// ---------------------------------------------------------------------------
// 0x15acb0: for each active m_map48 object, run its __thiscall probe at 0x151c00
// with (a1, a2); always returns 1. Returns 0 immediately if a1==0.
RVA(0x0015acb0, 0x76)
i32 CDDrawChildGroup::ForEachProbe_15acb0(i32 a1, i32 a2) {
    if (a1 == 0) {
        return 0;
    }
    POSITION pos = reinterpret_cast<POSITION>((m_map48.GetCount() != 0 ? -1 : 0));
    if (pos != 0) {
        do {
            void* key = 0;
            CWwdGameObject* val = 0;
            m_map48.GetNextAssoc(pos, key, reinterpret_cast<void*&>(val));
            if (val != 0 && !(val->m_flags & 0x4000000)) {
                val->WriteSnapshot(a1, a2);
            }
        } while (pos != 0);
    }
    return 1;
}

// ===========================================================================
// CDDrawChildGroup::LoadObjects @0x15ad30 - iterate the level reader's `count` object
// descriptors, dedup each against the active-set map (+0x48), publish its id to
// g_wwdObjIdCounter, and dispatch by the object kind (5 / 0x16 / 0x1b / 0x1c) to
// the matching factory; then build + link each object's child record.
// ===========================================================================
// @early-stop
// jump-table + foreign-chain plateau (>512 B): logic + the four kind arms
// (Resolve + the matching factory), the +0x48 dedup gate, the g_wwdObjIdCounter
// publish/restore, the merge createdObj/m_7c guards, and the child-build link are
// reconstructed in shape + order. Residual: the switch lowers to MSVC5's
// byte-index jump table (reloc-typed scoring artifact) and the heavy descriptor
// stack-buffer + null-register (ebp=0) regalloc across the arms is non-steerable
// under /O2. Final sweep.
RVA(0x0015ad30, 0x2be)
i32 CDDrawChildGroup::LoadObjects(CSerialArchive* reader, u32 count, i32 unused) {
    i32 savedCounter = 0;
    if (reader == 0) {
        return 0;
    }
    for (u32 i = 0; i < count; i++) {
        WwdObjDesc desc;
        reader->Read(&desc, 0xa0);

        void* found;
        if (m_map48.Lookup(reinterpret_cast<void*>(desc.m_04), found) && found != 0) {
            return 0;
        }

        savedCounter = g_wwdObjIdCounter;
        g_wwdObjIdCounter = desc.m_04;

        CWwdGameObject* createdObj = 0;
        switch (desc.m_08) {
            case 5: {
                CObject* val;
                m_parent->m_workerCache->m_10.Lookup(static_cast<const char*>(desc.m_14), val);
                if (val != 0) {
                    createdObj = CreateObject_159600(
                        desc.m_00,
                        desc.m_94,
                        desc.m_98,
                        desc.m_9c,
                        reinterpret_cast<i32>(val),
                        0
                    );
                }
                break;
            }
            case 0x16: {
                CObject* val;
                m_parent->m_workerCache->m_10.Lookup(static_cast<const char*>(desc.m_14), val);
                createdObj = CreateObject_159440(desc.m_00, desc.m_9c, reinterpret_cast<i32>(val), 0);
                break;
            }
            case 0x1b: {
                CObject* val;
                m_parent->m_workerCache->m_10.Lookup(static_cast<const char*>(desc.m_14), val);
                if (val != 0) {
                    createdObj = CreateObject_1598d0(
                        desc.m_00,
                        desc.m_94,
                        desc.m_98,
                        desc.m_9c,
                        reinterpret_cast<i32>(val),
                        0
                    );
                }
                break;
            }
            case 0x1c: {
                void* rec = 0;
                if (m_parent->InvokeCallback(reader, 0xa, desc.m_0c, reinterpret_cast<i32>(&rec)) == 0) {
                    return 0;
                }
                if (rec == 0) {
                    return 0;
                }
                *reinterpret_cast<i32*>((reinterpret_cast<char*>(rec) + 4)) = desc.m_00;
                // 0x159830 == CDDrawChildGroup::AttachSprite (the manager IS the factory)
                if (AttachSprite(
                        static_cast<CWwdGameObject*>(rec),
                        desc.m_94,
                        desc.m_98,
                        desc.m_9c,
                        desc.m_14,
                        0
                    )
                    == 0) {
                    return 0;
                }
                createdObj = static_cast<CWwdGameObject*>(rec);
                break;
            }
            default:
                break;
        }

        g_wwdObjIdCounter = savedCounter;
        if (createdObj == 0) {
            return 0;
        }
        if (createdObj->m_7c == 0) {
            return 0;
        }
        if (desc.m_10 != 0) {
            void* child = 0;
            if (m_parent->InvokeCallback(reader, 9, desc.m_10, reinterpret_cast<i32>(&child)) == 0) {
                return 0;
            }
            if (child == 0) {
                return 0;
            }
            // the worker's owned bound-logic slot (AnimWorkerObj::m_logic)
            createdObj->m_7c->m_logic = static_cast<CUserLogic*>(child);
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// 0x15b020: for each active m_map48 object, write its key to the archive then run
// its +0x3c virtual; bail to 0 on a 0 result, else 1. Returns 0 if ar==0.
// @early-stop
// 90.1% - the per-element block is byte-exact; the residual is the loop-tail
// structure: retail emits a bottom-tested loop with two distinct `return 1`
// epilogues, our cl hoists the body and merges the epilogue. An optimizer
// CFG-shape choice; logic exact.
RVA(0x0015b020, 0xc0)
i32 CDDrawChildGroup::ForEachSerialize_15b020(CSerialArchive* ar, i32 a2) {
    if (ar == 0) {
        return 0;
    }
    POSITION pos = reinterpret_cast<POSITION>((m_map48.GetCount() != 0 ? -1 : 0));
    if (pos != 0) {
        do {
            void* key = 0;
            CWwdGameObject* val = 0;
            m_map48.GetNextAssoc(pos, key, reinterpret_cast<void*&>(val));
            if (val != 0 && !(val->m_flags & 0x4000000)) {
                void* k = WwdKey(val);
                ar->Write(&k, 4);
                if (val->Slot3C(reinterpret_cast<i32>(ar), 4, a2, val) == 0) {
                    return 0;
                }
            }
        } while (pos != 0);
    }
    return 1;
}

// ---------------------------------------------------------------------------
// 0x15b0e0: the read/load counterpart of ForEachSerialize - pull `count` object
// keys back through the archive, resolve each in m_map48, require it present + alive
// (+0x7c != 0), then run its +0x3c dispatch with (ar, 7, flag, obj).
// @early-stop
// retail carries a conditional CString-cleanup latch (a name temp tracked by a
// stack "alive" flag) whose CONSTRUCTION the optimizer elided in this
// instantiation, leaving a never-taken `~CString` cleanup branch. The latch isn't
// reproducible from C without re-introducing the (here dead) name build; logic /
// CFG / offsets are exact, the dead cleanup branch is the residual.
RVA(0x0015b0e0, 0xec)
i32 CDDrawChildGroup::Deserialize_15b0e0(CSerialArchive* ar, u32 count, i32 flag) {
    if (ar == 0) {
        return 0;
    }
    for (u32 i = 0; i < count; i++) {
        void* key = 0;
        ar->Read(&key, 4);
        if (key == 0) {
            return 0;
        }
        CWwdGameObject* obj = 0;
        if (!m_map48.Lookup(key, reinterpret_cast<void*&>(obj))) {
            obj = 0;
        }
        if (obj == 0) {
            return 0;
        }
        if (*reinterpret_cast<i32*>((reinterpret_cast<char*>(obj) + 0x7c)) == 0) {
            return 0;
        }
        if (obj->Slot3C(reinterpret_cast<i32>(ar), 7, flag, obj) == 0) {
            return 0;
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// 0x15b1d0: for each m_map48 object, look its key up in m_map2c; if absent, remove it
// from m_map48 and destroy it. Returns the number removed.
// @early-stop
// 94.2% - logic/CFG/offsets exact; residual is the Lookup `found`-slot handling:
// retail reads `found` into a register only on Lookup-success, our cl re-zeroes
// the slot and compares memory. A found-slot regalloc coin-flip.
RVA(0x0015b1d0, 0x9b)
i32 CDDrawChildGroup::PruneOrphans_15b1d0() {
    i32 n = 0;
    POSITION pos = reinterpret_cast<POSITION>((m_map48.GetCount() != 0 ? -1 : 0));
    if (pos != 0) {
        do {
            void* key = 0;
            CWwdGameObject* val = 0;
            m_map48.GetNextAssoc(pos, key, reinterpret_cast<void*&>(val));
            if (val != 0) {
                void* found = 0;
                if (!m_map2c.Lookup(WwdKey(val), found)) {
                    found = 0;
                }
                if (found == 0) {
                    m_map48.RemoveKey(WwdKey(val));
                    if (val != 0) {
                        delete val;
                    }
                    ++n;
                }
            }
        } while (pos != 0);
    }
    return n;
}

// ===========================================================================
// The three embedded sub-object ctors at the block tail (0x15b270-0x15b2b0):
// placement-new'd by the factories above.
// ===========================================================================
// 0x15b270 - the +0xb8 sub-object ctor; seed +0x8 = INT_MIN and +0x20 = -1.
// Stamps no vtable of its own -> the concrete member class has no recoverable
// RTTI name (identity-TODO).
RVA(0x0015b270, 0x11)
CWwdShadowRec::CWwdShadowRec() {
    m_8 = static_cast<i32>(0x80000000);
    m_20 = -1;
}

// 0x15b2a0: zero +0x0c then +0x08; returns `this` (ctor).
RVA(0x0015b2a0, 0xb)
CWwdSlot9c::CWwdSlot9c() {
    m_0c = 0;
    m_08 = 0;
}

// 0x15b2b0 - a sibling embedded sub-object ctor (placement-new'd at obj+0x9c);
// zero +0x0c, +0x08, +0x18. Stamps no vtable of its own (identity-TODO).
RVA(0x0015b2b0, 0xe)
CWwdSlot9cA::CWwdSlot9cA() {
    m_c = 0;
    m_8 = 0;
    m_18 = 0;
}
