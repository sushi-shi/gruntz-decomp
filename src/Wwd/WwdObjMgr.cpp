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
#include <Rez/FrameClock.h> // frame-clock band (g_frameDelta/g_frameTime/g_killCueClock/g_engineFrameDelta)
#include <Rez/RezAlloc.h> // RezAlloc/RezFree
#include <Io/FileMem.h>   // the serialize stream (CFileMemBase == the real CFileMemBase)

#include <DDrawMgr/DDrawChildGroup.h> // the shared object-collection manager class
#include <DDrawMgr/DDrawSubMgrPages.h> // CDDrawSubMgrPages (DrawObjectCounts m_drawTarget->m_backPair)
#include <Gruntz/SerialArchive.h> // the shared CFileMemBase stream (level reader, Read @+0x2c)
#include <Mfc.h> // CPtrList, CMapPtrToPtr (real afxcoll, for the m_10/m_map2c/m_map48 layout)
#include <Gruntz/Sprite.h> // CDDrawWorker (frame-data template value)
#include <DDrawMgr/AnimWorkerObj.h> // the canonical +0x7c worker/logic record (ex CWwdWorker/CLogicRecord views)
#include <Gruntz/ResolveNode.h>      // canonical CResolveNode (the factory base sub-object)
#include <Gruntz/AniAdvanceCursor.h> // CAniAdvanceCursor (the +0x1a0 sub-object; ctor 0x15b730)
#include <Wwd/WwdFactoryObject.h>    // CWwdFactoryObject/CWwdNotifier/CDDrawRect/RectsOverlap
#include <Wwd/WwdGameObjCtor.h>      // WwdCtorBase/CWwdGameObjBaseCtor/WwdAnimWorker (ctor cluster)
#include <Gruntz/WwdGameObject.h>    // canonical flat CWwdGameObject (the managed objects)
#include <Wwd/WwdGameObjectFamily.h> // the concrete kinds A/B/C/F + the embedded records
#include <DDrawMgr/DDrawChildGroup.h>  // CDDrawChildGroup (the walk dispatchers; IDENTITY == this)
#include <DDrawMgr/DDrawSurfaceMgr.h>  // canonical m_0c owner (InvokeCallback + m_workerCache)
#include <DDrawMgr/DDrawWorkerCache.h> // m_workerCache full type (the +0x10 name map)
#include <Gruntz/ObList.h>
#include <Gruntz/UserLogic.h>          // CGameObject - the real class of the AABB pair
#include <Wwd/WwdFile.h>               // CDDrawWorkerHost (m_parent->m_24->m_5c world transform)
#include <DDrawMgr/DDrawSurfacePair.h> // CDDrawSurfacePair (DrawCount - ex the DrawHost_164380 view)
#include <Gruntz/GameLevel.h>          // CGameLevel (m_parent->m_level) + CDDrawWorkerHost
#include <Win32.h>                     // SetRect + RECT
#include <Wwd/WwdObjMgr.h> // own exported globals (ex Globals.h)

DATA(0x0021ab14)
i32 g_wwdObjIdCounter = 1;

inline void* operator new(u32, void* p) {
    return p;
}

#include <DDrawMgr/DDrawWorkerHost.h>

inline void* WwdKey(CGameObject* o) {
    return reinterpret_cast<void*>(o->m_188);
}

RVA(0x001591e0, 0x5)
void CDDrawChildGroup::Unload() {
    this->DestroyChildren();
}

RVA(0x001591f0, 0x54)
void CDDrawChildGroup::DestroyChildren() {
    CGameLevel* p = OwnerMgr()->m_level;
    if (p != 0) {
        // m_mainPlane IS the plane/grid-owner CDDrawWorkerHost (the plane-family
        // unification: slots 9/10 of ??_7CDDrawWorkerHost are CDDrawWorkerHost methods).
        CDDrawWorkerHost* q = static_cast<CDDrawWorkerHost*>(p->m_mainPlane);
        if (q != 0) {
            q->Prune();
        }
    }
    CDDrawGroupNode* n = reinterpret_cast<CDDrawGroupNode*>(m_list.GetHeadPosition());
    while (n != 0) {
        CDDrawGroupNode* cur = n;
        n = n->m_next;
        CGameObject* obj = cur->m_obj;
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
        int root = m_ownerCtx;
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
    if (result->SetupFlagged(a2, a3, a4, a5, a6) == 0) {
        if (result != 0) {
            delete result; // virtual scalar-deleting dtor (slot 1)
        }
        return 0;
    }
    InsertSorted(result, 1); // the launder dies - base-typed param
    if (a7 & 0x200000) {
        // retail fires the +0x10 FN POINTER (m_notify), never a vtable slot
        result->m_7c->m_notify(result);
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
    OwnerMgr()->m_workerCache->m_10.Lookup(name, val);
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
        int root = m_ownerCtx;
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
    if (result->SetupDeferred(a2, a3) == 0) {
        if (result != 0) {
            delete result; // virtual scalar-deleting dtor (slot 1)
        }
        return 0;
    }
    InsertSorted(result, 1); // the launder dies - base-typed param
    if (a4 & 0x200000) {
        // retail fires the +0x10 FN POINTER (m_notify), never a vtable slot
        result->m_7c->m_notify(result);
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
    OwnerMgr()->m_workerCache->m_10.Lookup(name, val);
    return CreateObject_159440(a1, a2, reinterpret_cast<int>(val), a4);
}

// ===========================================================================
// 0x159600 - CDDrawChildGroup::CreateObject (a.k.a. CDDrawChildGroup::CreateSpriteImpl):
// allocate + construct a 0x1dc-byte CWwdGameObject, register it in the manager
// (InsertSorted), and (when `flags & 0x200000`) kick its worker's
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
CWwdGameObjectA*
CDDrawChildGroup::CreateObject_159600(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 flags) {
    char* obj = static_cast<char*>(RezAlloc(0x1dc));
    CWwdGameObjectA* result; // the 0x1dc kind (vtable 0x5f00a8)
    if (obj != 0) {
        i32 root = m_ownerCtx;
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
    if (result->Setup(a2, a3, a4, a5) == 0) {
        if (result != 0) {
            delete result; // virtual scalar-deleting dtor (slot 1)
        }
        return 0;
    }
    InsertSorted(result, 1); // the launder dies - base-typed param
    if (flags & 0x200000) {
        // retail fires the +0x10 FN POINTER (m_notify), never a vtable slot
        result->m_7c->m_notify(result);
    }
    return static_cast<CWwdGameObject*>(static_cast<void*>(result));
}

RVA(0x001597b0, 0x57)
CWwdGameObjectA* CDDrawChildGroup::CreateSprite(
    i32 kind,
    i32 geoB,
    i32 geoA,
    i32 hint,
    const char* name,
    i32 flags
) {
    CObject* tmpl_ob = 0;
    OwnerMgr()->m_workerCache->m_10.Lookup(name, tmpl_ob);
    CDDrawWorker* tmpl = static_cast<CDDrawWorker*>(tmpl_ob);
    if (!tmpl) {
        return 0;
    }
    // 0x159600 is CDDrawChildGroup::CreateObject_159600 (the factory IS the manager); the
    // old ?CreateSpriteImpl@CDDrawChildGroup@ decl was a PHANTOM second name for it.
    return CreateObject_159600(
        kind,
        geoB,
        geoA,
        hint,
        reinterpret_cast<i32>(tmpl),
        flags
    ); // the launder dies - one type now
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
    OwnerMgr()->m_workerCache->m_10.Lookup(name, tmpl_ob);
    CDDrawWorker* tmpl = static_cast<CDDrawWorker*>(tmpl_ob);
    if (!tmpl) {
        return 0;
    }
    obj->m_flags = flags;
    if (!obj->Setup(a1, a2, a3, reinterpret_cast<i32>(tmpl))) {
        return 0;
    }
    // 0x159e40 is CDDrawChildGroup::InsertSorted (the factory IS the object manager -
    // same `this`); bind the real method (reloc-masked ?InsertSorted@CDDrawChildGroup).
    this->InsertSorted(obj, 1);
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
    CWwdGameObject* result; // the 0x1fc kind (vtable 0x5f00e8)
    if (obj != 0) {
        int root = m_ownerCtx;
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
        result = reinterpret_cast<CWwdGameObject*>(obj);
    } else {
        result = 0;
    }
    if (result->Setup(a2, a3, a4, a5) == 0) {
        if (result != 0) {
            delete result; // virtual scalar-deleting dtor (slot 1)
        }
        return 0;
    }
    InsertSorted(result, 1); // the launder dies - base-typed param
    if (a6 & 0x200000) {
        // retail fires the +0x10 FN POINTER (m_notify), never a vtable slot
        result->m_7c->m_notify(result);
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
    OwnerMgr()->m_workerCache->m_10.Lookup(name, val);
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
void CDDrawChildGroup::TickKillCues(i32 advance) {
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
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(cur->m_obj);
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
            killQueue.Add(static_cast<CObject*>(obj));
        } else if (flags & 0x20000) {
            sortQueue.Add(static_cast<CObject*>(obj));
        }
    }

    i32 i;
    for (i = 0; i < killQueue.GetSize(); i++) {
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(killQueue.GetData()[i]);
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
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(sortQueue.GetData()[i]);
        obj->m_flags &= ~0x20000;
        m_list.RemoveAt(reinterpret_cast<POSITION>(obj->m_posCache));
        InsertSorted(obj, 0);
    }
}

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
            cur->m_obj->BltDirty(
                reinterpret_cast<CDDrawSurfacePair*>(a1),
                reinterpret_cast<CDDrawSurfacePair*>(a2)
            );
        } while (n != 0);
    }
}

RVA(0x00159cf0, 0x42)
void CDDrawChildGroup::WalkDispatch34(i32 a1, i32 a2, i32 a3) {
    CDDrawGroupNode* n = reinterpret_cast<CDDrawGroupNode*>(m_list.GetHeadPosition());
    if (n != 0) {
        do {
            n->m_obj->BltDirtyEx(
                reinterpret_cast<CDDrawSurfacePair*>(a1),
                reinterpret_cast<CDDrawSurfacePair*>(a2),
                a3
            );
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
            n->m_obj->BltDirtyRegions(
                reinterpret_cast<CDDrawSurfacePair*>(a1),
                reinterpret_cast<CDDrawSurfacePair*>(a2),
                a3
            );
            n = n->m_next;
        } while (n != 0);
    }
    WalkDispatch30(a2, a3);
}

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

RVA(0x00159db0, 0x5e)
void CDDrawChildGroup::RemoveAndDelete(CWwdGameObject* obj) {
    if (obj->m_flags & 0x800) {
        delete obj;
        return;
    }
    m_list.RemoveAt(reinterpret_cast<POSITION>(obj->m_posCache));
    m_map48.RemoveKey(WwdKey(obj));
    m_map2c.RemoveKey(WwdKey(obj));
    delete obj;
}

RVA(0x00159e10, 0x2e)
void CDDrawChildGroup::ReinsertUnflagged(CWwdGameObject* obj) {
    obj->m_flags &= 0xfffdffff;
    m_list.RemoveAt(reinterpret_cast<POSITION>(obj->m_posCache));
    InsertSorted(obj, 0);
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
void CDDrawChildGroup::InsertSorted(CGameObject* obj, i32 addToMaps) {
    if (obj->m_flags & 0x800) {
        obj->m_posCache = 0;
        return;
    }
    if (addToMaps != 0) {
        m_map2c[WwdKey(obj)] = obj;
        m_map48[WwdKey(obj)] = obj;
    }
    CDDrawGroupNode* node = reinterpret_cast<CDDrawGroupNode*>(m_list.GetHeadPosition());
    i32 key = obj->m_sortKey;
    while (node != 0) {
        CDDrawGroupNode* cur = node;
        CWwdGameObject* data = static_cast<CWwdGameObject*>(cur->m_obj);
        node = node->m_next;
        if (data->m_sortKey > key && !(data->m_flags & 0x20000)) {
            obj->m_posCache = reinterpret_cast<i32>(
                m_list.InsertBefore(reinterpret_cast<POSITION>(cur), static_cast<CObject*>(obj))
            );
            return;
        }
    }
    obj->m_posCache = reinterpret_cast<i32>(m_list.AddTail(static_cast<CObject*>(obj)));
}

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
i32 __stdcall BoxesOverlap(CGameObject* a1, CGameObject* a2);

RVA(0x00159f00, 0x22e)
void CDDrawChildGroup::CollideBroadcast() {
    CDDrawGroupNode* outer = reinterpret_cast<CDDrawGroupNode*>(m_list.GetHeadPosition());
    while (outer != 0) {
        CGameObject* oi = outer->m_obj;
        CDDrawGroupNode* nextOuter = outer->m_next;
        if (!(oi->m_flags & 1)) {
            CDDrawGroupNode* inner = nextOuter;
            for (; inner != 0; inner = inner->m_next) {
                CGameObject* oj = inner->m_obj;
                i32 fj = oj->m_flags;
                if (fj & 1) {
                    continue;
                }
                i32 fi = oi->m_flags;
                if ((fi ^ fj) & 0x40000) {
                    continue;
                }
                // --- RECT PHASE (skipped when i&4 or j&0x80) ---
                if (!(fi & 4) && !(fj & 0x80)) {
                    i32 mask1 = static_cast<i32>(oj->m_collCategory) & oi->m_ec;
                    i32 mask2 = static_cast<i32>(oi->m_collCategory) & oj->m_f0;
                    if (mask1 || mask2) {
                        i32 overlap;
                        if (oj->m_switchRect.left == static_cast<i32>(0x80000000)) {
                            overlap = 0;
                        } else if (oi->m_area.left == static_cast<i32>(0x80000000)) {
                            overlap = 0;
                        } else {
                            CDDrawRect ra, rb;
                            i32 xi = oi->m_screenX;
                            i32 yi = oi->m_screenY;
                            ra.left = oi->m_area.left + xi;
                            ra.top = oi->m_area.top + yi;
                            ra.right = oi->m_area.right + xi;
                            ra.bottom = oi->m_area.bottom + yi;
                            i32 xj = oj->m_screenX;
                            i32 yj = oj->m_screenY;
                            rb.left = oj->m_switchRect.left + xj;
                            rb.top = oj->m_switchRect.top + yj;
                            rb.right = oj->m_switchRect.right + xj;
                            rb.bottom = oj->m_switchRect.bottom + yj;
                            overlap = RectsOverlap(&ra, &rb);
                        }
                        if (overlap) {
                            if (mask2) {
                                AnimWorkerObj* nf = oj->m_88;
                                if (nf != 0) {
                                    oj->m_8c = oi;
                                    // (the E*->CGameObject* respell dies at the flat-merge typedef)
                                    nf->m_notify(oj);
                                }
                            }
                            if (mask1) {
                                if (oi->m_flags & 8) {
                                    i32 v = oi->m_placeMode - oj->m_120;
                                    oi->m_placeMode = v;
                                    if (v <= 0) {
                                        // latch the worker's error/death state (m_1c is
                                        // the documented int|ptr role-union)
                                        oi->m_7c->m_1c = reinterpret_cast<void*>(0x1c);
                                    }
                                } else {
                                    AnimWorkerObj* nf = oi->m_80;
                                    if (nf != 0) {
                                        oi->m_84 = oj;
                                        nf->m_notify(oi);
                                    }
                                }
                            }
                        }
                    }
                }
                // --- BOX PHASE (skipped when j&4 or i&0x80) ---
                if (oj->m_flags & 4) {
                    continue;
                }
                if (oi->m_flags & 0x80) {
                    continue;
                }
                i32 mask1b = oj->m_ec & static_cast<i32>(oi->m_collCategory);
                i32 mask2b = static_cast<i32>(oj->m_collCategory) & oi->m_f0;
                if ((mask1b || mask2b) && BoxesOverlap(oj, oi)) {
                    if (mask2b) {
                        AnimWorkerObj* nf = oi->m_88;
                        if (nf != 0) {
                            oi->m_8c = oj;
                            nf->m_notify(oi);
                        }
                    }
                    if (mask1b) {
                        oj->Notify(oi);
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
// The two boxes are wide game objects (CGameObject family) - the `CWwdBox` view is
// gone. Every field is a canonical member at the same offset: the screen position
// (m_screenX @+0x5c / m_screenY @+0x60, CResolveNode) and the two 4-dword boxes
// m_area (+0x144) / m_switchRect (+0x154) on E. The 0x80000000 "invalid" sentinel is
// the family's documented unset marker.
// @early-stop
// ~85% (was 75.9%: materializing the two screen-space AABBs as CDDrawRect stack rects
// forces retail's 0x20 frame instead of cl keeping the 8 coords in registers). Residual
// is the rect-field store scheduling + stack-slot assignment (retail interleaves the
// coord adds/stores differently and colours the a2.switchRect.left probe into ebp);
// pure /O2 scheduling/regalloc, not source-steerable (caching the .left probes regressed).
RVA(0x0015a130, 0xdc)
i32 __stdcall BoxesOverlap(CGameObject* a1, CGameObject* a2) {
    if (a2->m_switchRect.left == static_cast<i32>(0x80000000)) {
        return 0;
    }
    if (a1->m_area.left == static_cast<i32>(0x80000000)) {
        return 0;
    }
    // retail materializes the two screen-space AABBs as 0x10-byte stack rects (0x20 frame)
    CDDrawRect ra, rb;
    i32 xi = a1->m_screenX;
    i32 yi = a1->m_screenY;
    ra.left = a1->m_area.left + xi;
    ra.top = a1->m_area.top + yi;
    ra.right = a1->m_area.right + xi;
    ra.bottom = a1->m_area.bottom + yi;
    i32 xj = a2->m_screenX;
    i32 yj = a2->m_screenY;
    rb.left = a2->m_switchRect.left + xj;
    rb.top = a2->m_switchRect.top + yj;
    rb.right = a2->m_switchRect.right + xj;
    rb.bottom = a2->m_switchRect.bottom + yj;
    if (ra.left > rb.right) {
        return 0;
    }
    if (ra.right < rb.left) {
        return 0;
    }
    if (ra.top > rb.bottom) {
        return 0;
    }
    return ra.bottom >= rb.top;
}

// @early-stop
// 0x15a210 (1074 B) = a CDDrawChildGroup-family debug OVERLAY, twin of
// DrawObjectCounts (same subsystem, both dead-in-retail). __thiscall, gated
// on a +0x08 debug flag; walks the +0x14 child list and per object draws debug
// geometry via CDDrawWorkerHost::WrapCoord: CDDrawSurfacePair::DrawBox(RECT*,color) x3,
// DrawCross(x,y), ResLoaders::DrawHost2_164420::DrawLabel(RECT*,char*) (falling
// back to "???" @0x1f0a94). Draws gated by g_dbg61ab28/2c/30. Held pending
// reconstruction (>512 B, novel per-object geometry).
RVA(0x0015a210, 0x432)
i32 DrawObjectDebugGeometry(void) {
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
void CDDrawChildGroup::DrawObjectCounts() {
    if (!(m_flags & 0x200000)) {
        return;
    }
    CDDrawGroupNode* node = reinterpret_cast<CDDrawGroupNode*>(m_list.GetHeadPosition());
    CDDrawSurfacePair* drawHost = OwnerMgr()->m_drawTarget->m_backPair;
    CDDrawWorkerHost* view = OwnerMgr()->m_level->m_mainPlane;
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
        rc.left = wl - view->m_originX + view->m_bounds50.left;
        rc.top = wt - view->m_originY + view->m_bounds50.top;
        view->WrapCoord(
            reinterpret_cast<i32*>(&rc.right),
            reinterpret_cast<i32*>(&rc.bottom)
        ); // LONG*->i32* (same width; PAH sig)
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
i32 CDDrawChildGroup::CheckSortOrder() {
    CDDrawGroupNode* node = reinterpret_cast<CDDrawGroupNode*>(m_list.GetHeadPosition());
    CWwdGameObject* anchor = static_cast<CWwdGameObject*>(node->m_obj);
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
            anchor = static_cast<CWwdGameObject*>(cur->m_obj);
        } while (node != 0);
    }
    if (anchor == 0) {
        return 1;
    }
    i32 key = anchor->m_sortKey;
    if (node == 0) {
        return 1;
    }
    do {
        CDDrawGroupNode* cur = node;
        node = node->m_next;
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(cur->m_obj);
        if ((obj->m_flags & 0x20000) == 0) {
            i32 curKey = obj->m_sortKey;
            if (key > curKey) {
                anchor->GetClassId();
                obj->GetClassId();
            } else {
                key = curKey;
                anchor = obj;
            }
        }
    } while (node != 0);
    return 1;
}

RVA(0x0015a7f0, 0x20)
CWwdGameObject* CDDrawChildGroup::FindByType04(i32 type) {
    CDDrawGroupNode* node = reinterpret_cast<CDDrawGroupNode*>(m_list.GetHeadPosition());
    while (node != 0) {
        CDDrawGroupNode* cur = node;
        node = node->m_next;
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(cur->m_obj);
        if (obj->m_id == type) {
            return obj;
        }
    }
    return 0;
}

RVA(0x0015a810, 0x42)
CWwdGameObject* CDDrawChildGroup::FindByTypeProbe(i32 type) {
    CDDrawGroupNode* node = reinterpret_cast<CDDrawGroupNode*>(m_list.GetHeadPosition());
    while (node != 0) {
        CDDrawGroupNode* cur = node;
        node = node->m_next;
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(cur->m_obj);
        if (obj->GetClassId() == CLASSID_SERIALREF && obj->m_id == type) {
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
CWwdGameObject* CDDrawChildGroup::FindByWorker(i32 type, void* key) {
    CDDrawGroupNode* node = reinterpret_cast<CDDrawGroupNode*>(m_list.GetHeadPosition());
    if (node == 0) {
        return 0;
    }
    do {
        CDDrawGroupNode* cur = node;
        node = node->m_next;
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(cur->m_obj);
        if (obj->GetClassId() == CLASSID_SERIALREF && obj->m_id == type) {
            // the worker notify fn doubles as the kind marker - match it against the
            // key worker (same idiom as the TriggerMgr grunt-notify compare)
            AnimWorkerObj* worker = obj->m_7c;
            if (worker->m_notify == (static_cast<AnimWorkerObj*>(key))->m_notify) {
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
// at +0x10). So Find is a plain CDDrawChildGroup method, walking the same list with the
// same `reinterpret_cast<CDDrawGroupNode*>(m_list.GetHeadPosition())` idiom every sibling FindBy* here uses.
RVA(0x0015a8c0, 0x7d)
void* CDDrawChildGroup::Find(i32 id, const char* key) {
    CObject* found = 0;
    OwnerMgr()->m_workerCache->m_10.Lookup(key, found);
    CDDrawGroupNode* node = reinterpret_cast<CDDrawGroupNode*>(m_list.GetHeadPosition());
    if (node == 0) {
        return 0;
    }
    AnimWorkerObj* fp = static_cast<AnimWorkerObj*>(found);
    do {
        CDDrawGroupNode* cur = node;
        node = node->m_next;
        CGameObject* obj = cur->m_obj;
        i32 tag = obj->GetClassId(); // vtable slot 8 (the type tag)
        if (tag == 5 && obj->m_id == id && obj->m_7c->m_notify == fp->m_notify) {
            return obj;
        }
    } while (node != 0);
    return 0;
}

// ---------------------------------------------------------------------------
// 0x15a940: the +0xe8-field twin of FindByWorker.
// @early-stop
// 85% - same loop-epilogue-merge wall as FindByWorker.
RVA(0x0015a940, 0x52)
CWwdGameObject* CDDrawChildGroup::FindByField(i32 type, void* key) {
    CDDrawGroupNode* node = reinterpret_cast<CDDrawGroupNode*>(m_list.GetHeadPosition());
    if (node == 0) {
        return 0;
    }
    do {
        CDDrawGroupNode* cur = node;
        node = node->m_next;
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(cur->m_obj);
        if (obj->GetClassId() == CLASSID_SERIALREF && obj->m_id == type
            && reinterpret_cast<void*>(obj->m_collCategory) == key) {
            return obj;
        }
    } while (node != 0);
    return 0;
}

RVA(0x0015a9a0, 0x23)
CWwdGameObject* CDDrawChildGroup::FindByKey(void* key) {
    CDDrawGroupNode* node = reinterpret_cast<CDDrawGroupNode*>(m_list.GetHeadPosition());
    while (node != 0) {
        CDDrawGroupNode* cur = node;
        node = node->m_next;
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(cur->m_obj);
        if (WwdKey(obj) == key) {
            return obj;
        }
    }
    return 0;
}

RVA(0x0015a9d0, 0x45)
CWwdGameObject* CDDrawChildGroup::FindByStatusKey(void* key) {
    CDDrawGroupNode* node = reinterpret_cast<CDDrawGroupNode*>(m_list.GetHeadPosition());
    while (node != 0) {
        CDDrawGroupNode* cur = node;
        node = node->m_next;
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(cur->m_obj);
        if (obj->GetClassId() == CLASSID_SERIALREF && WwdKey(obj) == key) {
            return obj;
        }
    }
    return 0;
}

RVA(0x0015aa20, 0x3c)
i32 CDDrawChildGroup::IsKindUnique(i32 kind) {
    CWwdGameObject* found = 0;
    CDDrawGroupNode* node = reinterpret_cast<CDDrawGroupNode*>(m_list.GetHeadPosition());
    while (node != 0) {
        CDDrawGroupNode* cur = node;
        node = node->m_next;
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(cur->m_obj);
        if (obj->m_id == kind) {
            if (found != 0) {
                return 0;
            }
            found = obj;
        }
    }
    return 1;
}

RVA(0x0015aa60, 0x23)
i32 CDDrawChildGroup::CountByKind(i32 kind) {
    i32 count = 0;
    CDDrawGroupNode* node = reinterpret_cast<CDDrawGroupNode*>(m_list.GetHeadPosition());
    while (node != 0) {
        CDDrawGroupNode* cur = node;
        node = node->m_next;
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(cur->m_obj);
        if (obj->m_id == kind) {
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
void CDDrawChildGroup::PruneList() {
    CDDrawGroupNode* node = reinterpret_cast<CDDrawGroupNode*>(m_list.GetHeadPosition());
    while (node != 0) {
        CDDrawGroupNode* cur = node;
        node = node->m_next;
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(cur->m_obj);
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
i32 CDDrawChildGroup::SumWeighted() {
    i32 sum = 0;
    i32 i = 0;
    CDDrawGroupNode* node = reinterpret_cast<CDDrawGroupNode*>(m_list.GetHeadPosition());
    while (node != 0) {
        CDDrawGroupNode* cur = node;
        node = node->m_next;
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(cur->m_obj);
        sum += i * (obj->m_screenX + obj->m_sortKey + obj->m_screenY + obj->m_id);
        ++i;
    }
    return sum;
}

RVA(0x0015ab30, 0x38)
void CDDrawChildGroup::RemoveAll(i32 pos, CWwdGameObject* obj) {
    m_list.RemoveAt(reinterpret_cast<POSITION>(pos));
    m_map2c.RemoveKey(WwdKey(obj));
    m_map48.RemoveKey(WwdKey(obj));
}

RVA(0x0015ab70, 0x27)
void CDDrawChildGroup::RemoveByPosition(i32 pos, CWwdGameObject* obj) {
    m_list.RemoveAt(reinterpret_cast<POSITION>(pos));
    m_map2c.RemoveKey(WwdKey(obj));
}

RVA(0x0015aba0, 0x1a)
void CDDrawChildGroup::AddToMap48(CWwdGameObject* obj) {
    m_map48[WwdKey(obj)] = obj;
}

RVA(0x0015abc0, 0x5e)
i32 CDDrawChildGroup::CountActive() {
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

RVA(0x0015ac20, 0x81)
i32 CDDrawChildGroup::ForEachDispatch(i32 a1, i32 a2, i32 a3) {
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
                val->Play(a1, a2, a3, val);
            }
        } while (pos != 0);
    }
    return 1;
}

RVA(0x0015acb0, 0x76)
i32 CDDrawChildGroup::ForEachProbe(i32 a1, i32 a2) {
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
i32 CDDrawChildGroup::LoadObjects(CFileMemBase* reader, u32 count, i32 unused) {
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

        CWwdGameObjectA* createdObj = 0;
        switch (desc.m_08) {
            case 5: {
                CObject* val;
                OwnerMgr()->m_workerCache->m_10.Lookup(static_cast<const char*>(desc.m_14), val);
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
                OwnerMgr()->m_workerCache->m_10.Lookup(static_cast<const char*>(desc.m_14), val);
                createdObj =
                    CreateObject_159440(desc.m_00, desc.m_9c, reinterpret_cast<i32>(val), 0);
                break;
            }
            case 0x1b: {
                CObject* val;
                OwnerMgr()->m_workerCache->m_10.Lookup(static_cast<const char*>(desc.m_14), val);
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
                if (OwnerMgr()->InvokeCallback(reader, 0xa, desc.m_0c, reinterpret_cast<i32>(&rec))
                    == 0) {
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
            if (OwnerMgr()->InvokeCallback(reader, 9, desc.m_10, reinterpret_cast<i32>(&child))
                == 0) {
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
i32 CDDrawChildGroup::ForEachSerialize(CFileMemBase* ar, i32 a2) {
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
                if (val->Play(reinterpret_cast<i32>(ar), 4, a2, val) == 0) {
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
i32 CDDrawChildGroup::Deserialize(CFileMemBase* ar, u32 count, i32 flag) {
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
        if (obj->m_7c == 0) {
            return 0;
        }
        if (obj->Play(reinterpret_cast<i32>(ar), 7, flag, obj) == 0) {
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
i32 CDDrawChildGroup::PruneOrphans() {
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

RVA(0x0015b270, 0x11)
CWwdShadowRec::CWwdShadowRec() {
    m_8 = static_cast<i32>(0x80000000);
    m_20 = -1;
}

RVA(0x0015b2a0, 0xb)
CWwdSlot9c::CWwdSlot9c() {
    m_0c = 0;
    m_08 = 0;
}

RVA(0x0015b2b0, 0xe)
CWwdSlot9cA::CWwdSlot9cA() {
    m_c = 0;
    m_8 = 0;
    m_18 = 0;
}
