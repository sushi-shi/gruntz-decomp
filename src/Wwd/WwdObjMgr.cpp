// WwdObjMgr.cpp - the 0x1591e0-0x15b2b0 original TU (wave4-L dossier #15, block
// H): the CWwdObjMgr collection obj - the object list/map ops (RemoveAll/
// InsertSorted/ForEach*/Prune*/FindBy*), the per-kind CreateObject factories +
// their CreateNamed front-ends, the per-frame kill-cue tick, LoadObjects, the
// CDDrawChildGroup walk dispatchers woven in (IDENTITY: CDDrawChildGroup IS
// CWwdObjMgr - see DDrawChildGroup.h), the CSpriteFactory pair (the factory IS
// this collection under the sprite-creation view - see SpriteFactory.h), and the
// three embedded sub-object ctors at the block tail. Held at the dossier-#9
// boundaries 3/4 (0x1591e0 / 0x15b2c0); a correct partial - it may yet merge with
// its neighbors.
//
// original TU: filename unknown (@identity-TODO - no __FILE__ anchor).
//
// Field names are placeholders; only OFFSETS + emitted bytes are load-bearing.
#include <rva.h>

#include <Gruntz/WwdObjMgr.h>     // the shared object-collection manager class
#include <Gruntz/SerialArchive.h> // the shared CSerialArchive stream (level reader, Read @+0x2c)
#include <Mfc.h> // CPtrList, CMapPtrToPtr (real afxcoll, for the m_10/m_2c/m_48 layout)
#include <Globals.h>
#include <Gruntz/SpriteFactory.h> // CSpriteFactory (CreateSprite @0x1597b0 / AttachSprite @0x159830)
#include <Gruntz/ResMgr.h>        // CResMgr + the factory's sprite-set registry (m_14)
#include <Gruntz/Sprite.h>        // CSprite (frame-data template value)
#include <Gruntz/WwdWorker.h>     // the shared per-object worker class (+0x7c; Kick)
#include <Gruntz/LogicRecord.h>   // CLogicRecord (the +0x7c kill-cue record: Consume/m_10/m_1c)
#include <Gruntz/ResolveNode.h>   // canonical CResolveNode (the factory base sub-object)
#include <Gruntz/AniAdvanceCursor.h>   // CAniAdvanceCursor (the +0x1a0 sub-object; ctor 0x15b730)
#include <Wwd/WwdFactoryObject.h>      // CWwdFactoryObject/CWwdNotifier/CDDrawRect/RectsOverlap
#include <Wwd/WwdGameObjCtor.h>        // WwdCtorBase/CWwdGameObj15b390/WwdAnimWorker (ctor cluster)
#include <DDrawMgr/DDrawChildGroup.h>  // CDDrawChildGroup (the walk dispatchers; IDENTITY == this)
#include <DDrawMgr/DDrawSubMgrPages.h> // CDDrawSubMgrPages (Method_159ef0 tail-forward)
#include <Gruntz/ObList.h>
#include <Gruntz/Viewport.h>        // CViewport (m_parent->m_24->m_5c world transform)
#include <Gruntz/ResLoadersViews.h> // ResLoaders::DrawHost_164380 (counter draw)
#include <Win32.h>                  // SetRect + RECT

// Engine heap allocator (operator new / RezAlloc). Reloc-masked __cdecl extern.
extern "C" void* RezAlloc(unsigned int size); // 0x1b9b46

// Placement new: construct a sub-object in place at a factory-computed offset.
inline void* operator new(u32, void* p) {
    return p;
}

// The +0x1a0 draw sub-manager the 0x1598d0 factory placement-constructs (real ctor
// 0x156cb0 in the G obj; ctor-only view - the name-conflation with the CObject-
// derived <DDrawMgr/DDrawSubMgr.h> base is flagged there for the identity pass).
class CDDrawSurfaceMgr;
struct CDDrawSubMgr { // the +0x1a0 draw sub-manager (real ctor in DDrawSubMgr.cpp)
    CDDrawSubMgr(CDDrawSurfaceMgr* mgr, i32 a2, i32 a3); // 0x156cb0
};
SIZE_UNKNOWN(CDDrawSubMgr);

// ===========================================================================
// The managed objects (CWwdObject, the 0x1dc-byte factory output) carry: a flag
// word at +0x08, a sort key at +0x74, a CPtrList POSITION cache at +0x78, and the
// map key at +0x188. Modeled polymorphically only so the vtable dispatches lower
// to the exact `mov eax,[obj]; call [eax+slot]`; virtuals are never defined here.
// ===========================================================================
class CWwdObject {
public:
    virtual void Slot00();
    virtual ~CWwdObject(); // slot 1 (deleting dtor -> cl-emitted ??_G)
    virtual void Slot08();
    virtual void Slot0C();
    virtual i32 Slot10(void* a); // +0x10 (1-arg op, called from 159600)
    virtual void Slot14();
    virtual void Slot18();
    virtual void Slot1C();
    virtual i32 Slot20(); // status probe (5 == matched)
    virtual void Slot24();
    virtual void Slot28();
    virtual void Slot2C();
    virtual void Slot30(); // +0x30 archive write
    virtual void Slot34();
    virtual void Slot38();
    virtual i32 Slot3C(i32 a1, i32 a2, i32 a3, void* obj); // +0x3c
    i32 m_04;                  // +0x04 type/kind key (FindBy* match field)
    i32 m_flags;               // +0x08 flag word
    char m_pad0c[0x5c - 0x0c]; // +0x0c..0x5b
    i32 m_5c;                  // +0x5c geometry term (SumWeighted_15aaf0)
    i32 m_60;                  // +0x60 geometry term (SumWeighted_15aaf0)
    char m_pad64[0x74 - 0x64]; // +0x64..0x73
    i32 m_sortKey;             // +0x74 sort key
    i32 m_posCache;            // +0x78 CPtrList POSITION cache
    CLogicRecord* m_killCue;   // +0x7c kill-cue record (Consume/callback/refcount)
    char m_pad80[0x188 - 0x80];
    // authentic: the void* key of the CMapPtrToPtr/CMapPtrToOb maps (m_2c/m_48);
    // MFC types the map key void* - it is passed straight to RemoveKey/operator[].
    void* m_key; // +0x188 map key
};
SIZE_UNKNOWN(CWwdObject);

// CPtrList CNode shape (pNext@0, pPrev@4, data@8); the list head node is at
// CWwdObjMgr+0x14 (= m_10.m_pNodeHead).
struct CWwdNode {
    CWwdNode* m_next;  // +0x00
    CWwdNode* m_prev;  // +0x04  (CPtrList node pPrev; not walked here)
    CWwdObject* m_obj; // +0x08
};
SIZE_UNKNOWN(CWwdNode);

// The object reached via m_parent->+0x24->+0x5c is a CImageSet3 (the WWD image-set
// collection); its Prune_1628d0 (0x1628d0) forwards to the spatial grid's Prune.
class CImageSet3 : public CObject {
public:
    i32 Prune_1628d0(); // 0x1628d0 (__thiscall)
};
SIZE_UNKNOWN(CImageSet3);

// The per-object descriptor the level reader fills (0xa0 bytes). +0x04 is the
// dedup id, +0x08 the kind selector, +0x14 the object's name string.
struct WwdObjDesc {
    i32 m_00;               // +0x00  passed to the factory
    i32 m_04;               // +0x04  dedup key / object id
    i32 m_08;               // +0x08  kind selector
    i32 m_0c;               // +0x0c  ARM-0x1c child tag
    i32 m_10;               // +0x10  merge child-build selector
    char m_14[0x94 - 0x14]; // +0x14  name string (resolver key)
    i32 m_94;               // +0x94
    i32 m_98;               // +0x98
    i32 m_9c;               // +0x9c
};
SIZE_UNKNOWN(WwdObjDesc);

// The level-file object (this->m_0c): m_14 fronts the string-resolve map (+0x10);
// BuildChild spawns a sub-record for the object. External, reloc-masked.
struct WwdFile {
    char m_pad0[0x14];
    char* m_14;                                                      // +0x14
    i32 BuildChild(void* reader, i32 tag, i32 selector, void** out); // 0x156a90
};

// The created game object as LoadObjects reads it: +0x7c is its aux record, whose
// +0x18 receives the child built in the merge tail.
struct WwdGameObjAux {
    char m_pad0[0x18];
    void* m_18; // +0x18
};
SIZE_UNKNOWN(WwdGameObjAux);
class CWwdGameObject {
public:
    char m_pad0[0x7c];
    WwdGameObjAux* m_7c; // +0x7c
    // 0x151c00 - the +0x2c/+0x48-map objects are real CWwdGameObjects; WriteSnapshot
    // emits their 0xa0-byte snapshot record (ForEachProbe calls it; 2nd arg unused).
    i32 WriteSnapshot(i32 dst, i32 unused);
};
SIZE_UNKNOWN(CWwdGameObject);

// The wide objects' polymorphic build interfaces (cast-only; declared-only
// virtuals so cl emits no ??_7). Each factory's object has a distinct vtable, so
// the Build slot differs: 0x159250 -> +0x40 (5 args), 0x159440 -> +0x40 (2 args),
// 0x1598d0/0x159600 -> +0x28 (4 args). All share the scalar-deleting dtor at +0x04.
class CWwdFactoryA { // 159250 (+0x40, 5 args) and 1598d0/159600 (+0x28, 4 args)
public:
    virtual void Vs00();
    virtual ~CWwdFactoryA(); // slot 1 (deleting dtor -> cl-emitted ??_G)
    virtual void Vs08();
    virtual void Vs0C();
    virtual void Vs10();
    virtual void Vs14();
    virtual void Vs18();
    virtual void Vs1C();
    virtual void Vs20();
    virtual void Vs24();
    virtual int Build4(int a, int b, int c, int d); // +0x28
    virtual void Vs2C();
    virtual void Vs30();
    virtual void Vs34();
    virtual void Vs38();
    virtual void Vs3C();
    virtual int Build5(int a, int b, int c, int d, int e); // +0x40
};
SIZE_UNKNOWN(CWwdFactoryA);
class CWwdFactoryB { // 159440 (+0x40, 2 args)
public:
    virtual void Vs00();
    virtual ~CWwdFactoryB(); // slot 1 (deleting dtor -> cl-emitted ??_G)
    virtual void Vs08();
    virtual void Vs0C();
    virtual void Vs10();
    virtual void Vs14();
    virtual void Vs18();
    virtual void Vs1C();
    virtual void Vs20();
    virtual void Vs24();
    virtual void Vs28();
    virtual void Vs2C();
    virtual void Vs30();
    virtual void Vs34();
    virtual void Vs38();
    virtual void Vs3C();
    virtual int Build2(int a, int b); // +0x40
};
SIZE_UNKNOWN(CWwdFactoryB);

// The +0x9c sub-object built by the 0x159250/0x159440/0x159600 factories: two
// zeroed dword fields (ctor 0x15b2a0 below) + the +0x18 the factories clear.
class CWwdSlot9c {
public:
    char m_pad00[0x08]; // +0x00..0x07
    i32 m_08;           // +0x08
    i32 m_0c;           // +0x0c
    char m_pad10[0x18 - 0x10];
    i32 m_18; // +0x18  -> obj+0xb4
    CWwdSlot9c();
};
SIZE_UNKNOWN(CWwdSlot9c);
// The +0xb8 sub-object (ctor 0x15b270 below); seeds +0x8 = INT_MIN, +0x20 = -1.
class Obj15b270 {
public:
    Obj15b270();
    char m_pad0[0x8];
    i32 m_8;
    char m_pad0c[0x20 - 0xc];
    i32 m_20;
};
SIZE_UNKNOWN(Obj15b270);
// The +0x9c sibling sub-object of the 0x159600 factory (ctor 0x15b2b0 below).
class Obj15b2b0 {
public:
    Obj15b2b0();
    char m_pad0[0x8];
    i32 m_8;
    i32 m_c;
    char m_pad10[0x18 - 0x10];
    i32 m_18;
};
SIZE_UNKNOWN(Obj15b2b0);

// The shared kill-cue clock (advanced once per tick) + its per-frame delta, and
// the cached timeGetTime import (bound in DirPal.cpp).
extern u32(WINAPI* g_pTimeGetTime)(); // 0x6c4650
extern "C" u32 g_killCueClock;        // 0x6bf3c0 kill-cue clock (prev now)
extern "C" u32 g_6bf3bc;              // 0x6bf3bc per-frame delta

// The per-object cue callback fired when a cue expires (obj+0x7c +0x10; __cdecl,
// one arg = the owning object).
typedef void(__cdecl* KillCueFn)(void*);

// ---------------------------------------------------------------------------
// CDDrawChildGroup::ForwardTo3C (0x1591e0): forward to Slot3C.
RVA(0x001591e0, 0x5)
void CDDrawChildGroup::ForwardTo3C() {
    this->Slot3C();
}

// ---------------------------------------------------------------------------
// 0x1591f0: ClearAll cleanup - run m_parent->+0x24->+0x5c->0x1628d0 (when present),
// walk the +0x10 CObList destroying each node's child via its scalar-deleting
// destructor, then RemoveAll the +0x10 list and the +0x2c / +0x48 collections.
RVA(0x001591f0, 0x54)
void CDDrawChildGroup::DestroyChildren() {
    void* p = *(void**)((char*)m_parent + 0x24);
    if (p != 0) {
        CImageSet3* q = *(CImageSet3**)((char*)p + 0x5c);
        if (q != 0) {
            q->Prune_1628d0();
        }
    }
    CDDrawGroupNode* n = m_head;
    while (n != 0) {
        CDDrawGroupNode* cur = n;
        n = n->m_next;
        CDDrawGroupChild* obj = cur->m_obj;
        if (obj != 0) {
            delete obj;
        }
    }
    ((CObList*)((char*)this + 0x10))->RemoveAll();
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
CWwdObjMgr::CreateObject_159250(int a1, int a2, int a3, int a4, int a5, int a6, int a7) {
    char* obj = (char*)RezAlloc(0x190);
    CWwdGameObject* result;
    if (obj != 0) {
        int root = (int)m_0c;
        new (obj) CResolveNode(root, a1, a7);
        CWwdSlot9c* s9c = (CWwdSlot9c*)(obj + 0x9c);
        new (s9c) CWwdSlot9c();
        s9c->m_18 = 0;
        new (obj + 0xb8) Obj15b270();
        new (obj + 0xdc) CString();
        // factory ctor vptr install dropped (model as compiler-emitted vtable; % ok per drive-to-0)
        *(int*)(obj + 0x5c) = (int)0x80000000;
        *(int*)(obj + 0x78) = 0;
        char* worker = (char*)RezAlloc(0x17c);
        if (worker != 0) {
            ((CWwdWorker*)worker)->Ctor(root, a1, 0);
        } else {
            worker = 0;
        }
        *(void**)(obj + 0x7c) = worker;
        *(int*)(obj + 0x98) = 0;
        *(int*)(obj + 0x80) = 0;
        *(int*)(obj + 0x88) = 0;
        *(int*)(obj + 0x90) = 0;
        *(int*)(obj + 0x188) = g_wwdObjIdCounter;
        g_wwdObjIdCounter = g_wwdObjIdCounter + 1;
        // factory ctor vptr install dropped (model as compiler-emitted vtable; % ok per drive-to-0)
        *(char*)(obj + 0x18c) = 0;
        result = (CWwdGameObject*)obj;
    } else {
        result = 0;
    }
    if (((CWwdFactoryA*)result)->Build5(a2, a3, a4, a5, a6) == 0) {
        if (result != 0) {
            delete ((CWwdFactoryA*)result);
        }
        return 0;
    }
    InsertSorted_159e40((CWwdObject*)result, 1);
    if (a7 & 0x200000) {
        ((CWwdWorker*)*(void**)((char*)result + 0x7c))->Kick(result);
    }
    return result;
}

// CreateNamed_1593e0 (__thiscall, ret 0x1c => 7 args). Resolve `name` through the
// level string map to a value, then create the 7-arg kind with it as arg5.
// @early-stop
// 93.5% - logic byte-exact. Residual: MSVC5 schedules the `val = 0` pre-init store
// BETWEEN the two Lookup arg-pushes (push &val / [val]=0 / push name) where retail
// emits it AFTER both pushes. A non-steerable /O2 statement-scheduling coin-flip
// (permuter + map-hoist both tried). Shared with CreateNamed_1595b0/159a10/166780.
RVA(0x001593e0, 0x53)
CWwdGameObject*
CWwdObjMgr::CreateNamed_1593e0(int a1, int a2, int a3, int a4, const char* name, int a6, int a7) {
    void* val = 0;
    ((CMapStringToPtr*)(m_0c->m_14 + 0x10))->Lookup(name, val);
    return CreateObject_159250(a1, a2, a3, a4, (int)val, a6, a7);
}

// ===========================================================================
// 0x159440 - factory for the 0x18c-byte kind. __thiscall, 4 stack args (ret 0x10).
// @early-stop
// rezalloc-placement-new wall (sibling of 0x159600). frame absent, body exact.
// ===========================================================================
RVA(0x00159440, 0x170)
CWwdGameObject* CWwdObjMgr::CreateObject_159440(int a1, int a2, int a3, int a4) {
    char* obj = (char*)RezAlloc(0x18c);
    CWwdGameObject* result;
    if (obj != 0) {
        int root = (int)m_0c;
        new (obj) CResolveNode(root, a1, a4);
        CWwdSlot9c* s9c = (CWwdSlot9c*)(obj + 0x9c);
        new (s9c) CWwdSlot9c();
        s9c->m_18 = 0;
        new (obj + 0xb8) Obj15b270();
        new (obj + 0xdc) CString();
        // factory ctor vptr install dropped (model as compiler-emitted vtable; % ok per drive-to-0)
        *(int*)(obj + 0x5c) = (int)0x80000000;
        *(int*)(obj + 0x78) = 0;
        char* worker = (char*)RezAlloc(0x17c);
        if (worker != 0) {
            ((CWwdWorker*)worker)->Ctor(root, a1, 0);
        } else {
            worker = 0;
        }
        *(void**)(obj + 0x7c) = worker;
        *(int*)(obj + 0x98) = 0;
        *(int*)(obj + 0x80) = 0;
        *(int*)(obj + 0x88) = 0;
        *(int*)(obj + 0x90) = 0;
        *(int*)(obj + 0x188) = g_wwdObjIdCounter;
        g_wwdObjIdCounter = g_wwdObjIdCounter + 1;
        // factory ctor vptr install dropped (model as compiler-emitted vtable; % ok per drive-to-0)
        result = (CWwdGameObject*)obj;
    } else {
        result = 0;
    }
    if (((CWwdFactoryB*)result)->Build2(a2, a3) == 0) {
        if (result != 0) {
            delete ((CWwdFactoryB*)result);
        }
        return 0;
    }
    InsertSorted_159e40((CWwdObject*)result, 1);
    if (a4 & 0x200000) {
        ((CWwdWorker*)*(void**)((char*)result + 0x7c))->Kick(result);
    }
    return result;
}

// CreateNamed_1595b0 (__thiscall, ret 0x10 => 4 args). Resolve `name` -> value and
// create the 4-arg kind with it substituted for arg3.
// @early-stop
// 92% - logic byte-exact; same val=0 arg-push scheduling residual as CreateNamed_1593e0.
RVA(0x001595b0, 0x44)
CWwdGameObject* CWwdObjMgr::CreateNamed_1595b0(int a1, int a2, const char* name, int a4) {
    void* val = 0;
    ((CMapStringToPtr*)(m_0c->m_14 + 0x10))->Lookup(name, val);
    return CreateObject_159440(a1, a2, (int)val, a4);
}

// ===========================================================================
// 0x159600 - CWwdObjMgr::CreateObject (a.k.a. CSpriteFactory::CreateSpriteImpl):
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
CWwdGameObject* CWwdObjMgr::CreateObject_159600(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 flags) {
    char* obj = (char*)RezAlloc(0x1dc);
    CWwdGameObject* result;
    if (obj != 0) {
        i32 root = (i32)m_0c;
        new (obj) CResolveNode(root, a1, flags);
        new (obj + 0x9c) Obj15b2b0();
        new (obj + 0xb8) Obj15b270();
        new (obj + 0xdc) CString();
        // factory ctor vptr install dropped (model as compiler-emitted vtable; % ok per drive-to-0)
        *(i32*)(obj + 0x5c) = (i32)0x80000000;
        *(i32*)(obj + 0x78) = 0;
        char* worker = (char*)RezAlloc(0x17c);
        if (worker != 0) {
            ((CWwdWorker*)worker)->Ctor(root, a1, flags);
        } else {
            worker = 0;
        }
        *(void**)(obj + 0x7c) = worker;
        *(i32*)(obj + 0x98) = 0;
        *(i32*)(obj + 0x80) = 0;
        *(i32*)(obj + 0x88) = 0;
        *(i32*)(obj + 0x90) = 0;
        *(i32*)(obj + 0x188) = g_wwdObjIdCounter;
        g_wwdObjIdCounter = g_wwdObjIdCounter + 1;
        new (obj + 0x1a0) CAniAdvanceCursor(root, a1, flags);
        // factory ctor vptr install dropped (model as compiler-emitted vtable; % ok per drive-to-0)
        *(i32*)(obj + 0x18c) = -1;
        *(i32*)(obj + 0x190) = -1;
        *(i32*)(obj + 0x198) = 0;
        *(i32*)(obj + 0x194) = 0;
        *(i32*)(obj + 0x19c) = 0;
        result = (CWwdGameObject*)obj;
    } else {
        result = 0;
    }
    if (((CWwdFactoryA*)result)->Build4(a2, a3, a4, a5) == 0) {
        if (result != 0) {
            delete ((CWwdFactoryA*)result);
        }
        return 0;
    }
    InsertSorted_159e40((CWwdObject*)result, 1);
    if (flags & 0x200000) {
        ((CWwdWorker*)*(void**)((char*)result + 0x7c))->Kick(result);
    }
    return result;
}

// ---------------------------------------------------------------------------
// The sprite object AttachSprite installs/initialises (arg0). Its init virtual
// lives at vtable slot 0x28 (a 4-arg method returning a success flag); its +0x7c
// sub-table holds a plain fn-ptr entry at +0x10 driven post-attach. CSprite2 is
// forward-declared (with __single_inheritance) in <Gruntz/SpriteFactory.h>.
struct CSprite2SubTable {
    void* _00[4];             // +0x00..0x0c
    void (*Entry)(CSprite2*); // +0x10  post-attach driver
};
// Real polymorphic view: Init is slot 10 (+0x28), a real virtual (10 filler slots
// precede it); the compiler emits the vptr at +0x00. obj->Init() -> call [eax+0x28].
struct CSprite2 {
    virtual void Slot0();
    virtual void Slot1();
    virtual void Slot2();
    virtual void Slot3();
    virtual void Slot4();
    virtual void Slot5();
    virtual void Slot6();
    virtual void Slot7();
    virtual void Slot8();
    virtual void Slot9();
    virtual i32 Init(i32 a, i32 b, i32 c, CSprite* tmpl); // +0x28 (slot 10)
    char _04[0x08 - 0x04];
    i32 m_08; // +0x08  flags slot
    char _0c[0x7c - 0x0c];
    CSprite2SubTable* m_7c; // +0x7c  fn-ptr sub-table (entry @+0x10)
};
SIZE_UNKNOWN(CSprite2);
SIZE_UNKNOWN(CSprite2SubTable);

// ===========================================================================
// CSpriteFactory::CreateSprite @0x1597b0 - the public sprite-creation entry the
// HUD / level loaders call. Look the sprite TEMPLATE up by class-NAME (the `name`
// arg) in the factory's sprite-set table (m_c->m_14->map); on a hit forward the
// four leading build args + the resolved template + flags to CreateSpriteImpl
// @0x159600 (== CreateObject_159600 above). __thiscall, ret 0x18.
// ===========================================================================
RVA(0x001597b0, 0x57)
CGameObject*
CSpriteFactory::CreateSprite(i32 kind, i32 geoB, i32 geoA, i32 hint, const char* name, i32 flags) {
    CObject* tmpl_ob = 0;
    m_c->m_14->m_10map.Lookup(name, tmpl_ob);
    CSprite* tmpl = (CSprite*)tmpl_ob;
    if (!tmpl) {
        return 0;
    }
    return CreateSpriteImpl(kind, geoB, geoA, hint, tmpl, flags);
}

// ===========================================================================
// CSpriteFactory::AttachSprite @0x159830 - initialise an already-allocated sprite
// (arg0) against a named template. Returns 1 on success. __thiscall, ret 0x18.
// @early-stop
// 99.5% ebx<->edi coloring wall: byte-identical except retail colors this->edi /
// flags->ebx while MSVC5 picks this->ebx / flags->edi (6 reg-only instr diffs); the
// instruction selection (PMF init call, m_7c dispatch) is exact, not source-steerable.
RVA(0x00159830, 0x92)
i32 CSpriteFactory::AttachSprite(
    CSprite2* obj,
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
    m_c->m_14->m_10map.Lookup(name, tmpl_ob);
    CSprite* tmpl = (CSprite*)tmpl_ob;
    if (!tmpl) {
        return 0;
    }
    obj->m_08 = flags;
    if (!obj->Init(a1, a2, a3, tmpl)) {
        return 0;
    }
    // 0x159e40 is CWwdObjMgr::InsertSorted_159e40 (the factory IS the object manager -
    // same `this`); bind the real method (reloc-masked ?InsertSorted_159e40@CWwdObjMgr).
    ((CWwdObjMgr*)this)->InsertSorted_159e40((CWwdObject*)obj, 1);
    if (flags & 0x200000) {
        obj->m_7c->Entry(obj);
    }
    return 1;
}

// ===========================================================================
// 0x1598d0 - factory for the 0x1fc-byte kind. __thiscall, 6 stack args (ret 0x18).
// @early-stop
// rezalloc-placement-new wall (sibling of 0x159600). frame absent, body exact.
// ===========================================================================
RVA(0x001598d0, 0x13d)
CWwdGameObject* CWwdObjMgr::CreateObject_1598d0(int a1, int a2, int a3, int a4, int a5, int a6) {
    char* obj = (char*)RezAlloc(0x1fc);
    CWwdGameObject* result;
    if (obj != 0) {
        int root = (int)m_0c;
        new (obj) CWwdGameObj15b390(root, a1, a6);
        new (obj + 0x1a0) CDDrawSubMgr((CDDrawSurfaceMgr*)root, a1, a6);
        // factory ctor vptr install dropped (model as compiler-emitted vtable; % ok per drive-to-0)
        *(int*)(obj + 0x1b0) = 0;
        *(int*)(obj + 0x1b4) = 0;
        *(int*)(obj + 0x1b8) = 0;
        // factory ctor vptr install dropped (model as compiler-emitted vtable; % ok per drive-to-0)
        *(int*)(obj + 0x18c) = -1;
        *(int*)(obj + 0x190) = -1;
        *(int*)(obj + 0x198) = 0;
        *(int*)(obj + 0x194) = 0;
        *(int*)(obj + 0x19c) = 0;
        new (obj + 0x1dc) CObList(0xa);
        // factory ctor vptr install dropped (model as compiler-emitted vtable; % ok per drive-to-0)
        *(int*)(obj + 0x1f8) = 0;
        result = (CWwdGameObject*)obj;
    } else {
        result = 0;
    }
    if (((CWwdFactoryA*)result)->Build4(a2, a3, a4, a5) == 0) {
        if (result != 0) {
            delete ((CWwdFactoryA*)result);
        }
        return 0;
    }
    InsertSorted_159e40((CWwdObject*)result, 1);
    if (a6 & 0x200000) {
        ((CWwdWorker*)*(void**)((char*)result + 0x7c))->Kick(result);
    }
    return result;
}

// CreateNamed_159a10 (__thiscall, ret 0x18 => 6 args). Resolve `name` -> value; if
// the lookup produced nothing, bail; else create the 6-arg kind with the value as arg5.
// @early-stop
// 94% - logic byte-exact; same val=0 arg-push scheduling residual as CreateNamed_1593e0.
RVA(0x00159a10, 0x57)
CWwdGameObject*
CWwdObjMgr::CreateNamed_159a10(int a1, int a2, int a3, int a4, const char* name, int a6) {
    void* val = 0;
    ((CMapStringToPtr*)(m_0c->m_14 + 0x10))->Lookup(name, val);
    if (val == 0) {
        return 0;
    }
    return CreateObject_1598d0(a1, a2, a3, a4, (int)val, a6);
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
void CWwdObjMgr::TickKillCues_159a70(i32 advance) {
    static CObArray killQueue; // 0x6bf3a8  the 0x10000 (destroy) queue
    static CObArray sortQueue; // 0x6bf390  the 0x20000 (re-sort) queue
    killQueue.SetSize(0, -1);
    sortQueue.SetSize(0, -1);

    if (advance != 0) {
        u32 now = g_pTimeGetTime();
        u32 delta = now - g_killCueClock;
        g_killCueClock = now;
        g_6bf3bc = delta;
    }

    CWwdNode* node = (CWwdNode*)m_10.GetHeadPosition();
    while (node != 0) {
        CWwdNode* cur = node;
        node = node->m_next;
        CWwdObject* obj = cur->m_obj;
        CLogicRecord* rec = obj->m_killCue;
        if (rec->Consume((i32)g_6bf3bc) == 0) {
            i32* refc = (i32*)((char*)rec + 0x24);
            if (*refc != 0) {
                --*refc;
            } else {
                ((KillCueFn)rec->m_10)(obj);
            }
        }
        i32 flags = obj->m_flags;
        if (flags & 0x10000) {
            killQueue.Add((CObject*)obj);
        } else if (flags & 0x20000) {
            sortQueue.Add((CObject*)obj);
        }
    }

    i32 i;
    for (i = 0; i < killQueue.GetSize(); i++) {
        CWwdObject* obj = (CWwdObject*)killQueue.GetData()[i];
        if (obj->m_flags & 0x80000) {
            CLogicRecord* rec = obj->m_killCue;
            rec->m_1c = 0x1d;
            ((KillCueFn)rec->m_10)(obj);
        }
        if (obj->m_flags & 0x800) {
            if (obj != 0) {
                delete obj;
            }
        } else {
            m_10.RemoveAt((POSITION)obj->m_posCache);
            m_48.RemoveKey(obj->m_key);
            m_2c.RemoveKey(obj->m_key);
            if (obj != 0) {
                delete obj;
            }
        }
    }

    for (i = 0; i < sortQueue.GetSize(); i++) {
        CWwdObject* obj = (CWwdObject*)sortQueue.GetData()[i];
        obj->m_flags &= ~0x20000;
        m_10.RemoveAt((POSITION)obj->m_posCache);
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
void CDDrawChildGroup::WalkDispatch2C(i32 a1) {
    CDDrawGroupNode* n = m_head;
    if (n != 0) {
        do {
            CDDrawGroupNode* cur = n;
            n = n->m_next;
            cur->m_obj->Slot2C(a1);
        } while (n != 0);
    }
}

RVA(0x00159cc0, 0x2a)
void CDDrawChildGroup::WalkDispatch30(i32 a1, i32 a2) {
    CDDrawGroupNode* n = m_head;
    if (n != 0) {
        do {
            CDDrawGroupNode* cur = n;
            n = n->m_next;
            cur->m_obj->Slot30(a1, a2);
        } while (n != 0);
    }
}

RVA(0x00159cf0, 0x42)
void CDDrawChildGroup::WalkDispatch34(i32 a1, i32 a2, i32 a3) {
    CDDrawGroupNode* n = m_head;
    if (n != 0) {
        do {
            n->m_obj->Vfunc34(a1, a2, a3);
            n = n->m_next;
        } while (n != 0);
    }
    WalkDispatch30(a2, a3);
}

RVA(0x00159d40, 0x42)
void CDDrawChildGroup::WalkDispatch38(i32 a1, i32 a2, i32 a3) {
    CDDrawGroupNode* n = m_head;
    if (n != 0) {
        do {
            n->m_obj->Vfunc38(a1, a2, a3);
            n = n->m_next;
        } while (n != 0);
    }
    WalkDispatch30(a2, a3);
}

// ---------------------------------------------------------------------------
// Walk the +0x14 list setting each child's field at +0xd8 to -1.
RVA(0x00159d90, 0x1c)
void CDDrawChildGroup::ResetChildD8() {
    CDDrawGroupNode* n = m_head;
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
void CWwdObjMgr::RemoveAndDelete_159db0(CWwdObject* obj) {
    if (obj->m_flags & 0x800) {
        delete obj;
        return;
    }
    m_10.RemoveAt((POSITION)obj->m_posCache);
    m_48.RemoveKey(obj->m_key);
    m_2c.RemoveKey(obj->m_key);
    delete obj;
}

// ---------------------------------------------------------------------------
// 0x159e10: clear obj's re-sort flag (0x20000), unlink it from the list at its
// cached POSITION, then re-insert it in sorted order. __thiscall, 1 arg (ret 4).
RVA(0x00159e10, 0x2e)
void CWwdObjMgr::ReinsertUnflagged_159e10(CWwdObject* obj) {
    obj->m_flags &= 0xfffdffff;
    m_10.RemoveAt((POSITION)obj->m_posCache);
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
void CWwdObjMgr::InsertSorted_159e40(CWwdObject* obj, i32 addToMaps) {
    if (obj->m_flags & 0x800) {
        obj->m_posCache = 0;
        return;
    }
    if (addToMaps != 0) {
        m_2c[obj->m_key] = obj;
        m_48[obj->m_key] = obj;
    }
    CWwdNode* node = (CWwdNode*)m_10.GetHeadPosition();
    i32 key = obj->m_sortKey;
    while (node != 0) {
        CWwdNode* cur = node;
        CWwdObject* data = cur->m_obj;
        node = node->m_next;
        if (data->m_sortKey > key && !(data->m_flags & 0x20000)) {
            obj->m_posCache = (i32)m_10.InsertBefore((POSITION)cur, (CObject*)obj);
            return;
        }
    }
    obj->m_posCache = (i32)m_10.AddTail((CObject*)obj);
}

// CDDrawSubMgrPages::Method_159ef0 (0x159ef0): forward to Slot0F_157a00.
RVA(0x00159ef0, 0x5)
void CDDrawSubMgrPages::Method_159ef0() {
    this->Slot0F_157a00();
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
// Forward decls for the Slot40 body (their definitions follow at 0x15a130 in
// RVA order): the wide-object AABB view + the box-overlap predicate.
struct CWwdBox;
i32 __stdcall BoxesOverlap_15a130(CWwdBox* a1, CWwdBox* a2);

// NOTE: the inner-pair body is flattened with `continue` guards (identical CFG;
// MSVC5's parser corrupts its state on the fully-nested spelling in THIS include
// context - a compiler front-end bug, verified by bisection; the nested form
// compiled fine in the smaller pre-split TU).
RVA(0x00159f00, 0x22e)
void CDDrawChildGroup::Slot40() {
    CDDrawGroupNode* outer = m_head;
    while (outer != 0) {
        char* oi = (char*)outer->m_obj;
        CDDrawGroupNode* nextOuter = outer->m_next;
        if (!(*(i32*)(oi + 8) & 1)) {
            CDDrawGroupNode* inner = nextOuter;
            for (; inner != 0; inner = inner->m_next) {
                char* oj = (char*)inner->m_obj;
                i32 fj = *(i32*)(oj + 8);
                if (fj & 1) {
                    continue;
                }
                i32 fi = *(i32*)(oi + 8);
                if ((fi ^ fj) & 0x40000) {
                    continue;
                }
                // --- RECT PHASE (skipped when i&4 or j&0x80) ---
                if (!(fi & 4) && !(fj & 0x80)) {
                    i32 mask1 = *(i32*)(oj + 0xe8) & *(i32*)(oi + 0xec);
                    i32 mask2 = *(i32*)(oi + 0xe8) & *(i32*)(oj + 0xf0);
                    if (mask1 || mask2) {
                        i32 overlap;
                        if (*(i32*)(oj + 0x154) == (i32)0x80000000) {
                            overlap = 0;
                        } else if (*(i32*)(oi + 0x144) == (i32)0x80000000) {
                            overlap = 0;
                        } else {
                            CDDrawRect ra, rb;
                            i32 xi = *(i32*)(oi + 0x5c);
                            i32 yi = *(i32*)(oi + 0x60);
                            ra.left = *(i32*)(oi + 0x144) + xi;
                            ra.top = *(i32*)(oi + 0x148) + yi;
                            ra.right = *(i32*)(oi + 0x14c) + xi;
                            ra.bottom = *(i32*)(oi + 0x150) + yi;
                            i32 xj = *(i32*)(oj + 0x5c);
                            i32 yj = *(i32*)(oj + 0x60);
                            rb.left = *(i32*)(oj + 0x154) + xj;
                            rb.top = *(i32*)(oj + 0x158) + yj;
                            rb.right = *(i32*)(oj + 0x15c) + xj;
                            rb.bottom = *(i32*)(oj + 0x160) + yj;
                            overlap = RectsOverlap_15bfb0(&ra, &rb);
                        }
                        if (overlap) {
                            if (mask2) {
                                CWwdNotifier* nf = *(CWwdNotifier**)(oj + 0x88);
                                if (nf != 0) {
                                    *(void**)(oj + 0x8c) = oi;
                                    nf->m_callback(oj);
                                }
                            }
                            if (mask1) {
                                if (*(i32*)(oi + 8) & 8) {
                                    i32 v = *(i32*)(oi + 0x128) - *(i32*)(oj + 0x120);
                                    *(i32*)(oi + 0x128) = v;
                                    if (v <= 0) {
                                        *(i32*)(*(char**)(oi + 0x7c) + 0x1c) = 0x1c;
                                    }
                                } else {
                                    CWwdNotifier* nf = *(CWwdNotifier**)(oi + 0x80);
                                    if (nf != 0) {
                                        *(void**)(oi + 0x84) = oj;
                                        nf->m_callback(oi);
                                    }
                                }
                            }
                        }
                    }
                }
                // --- BOX PHASE (skipped when j&4 or i&0x80) ---
                if (*(i32*)(oj + 8) & 4) {
                    continue;
                }
                if (*(i32*)(oi + 8) & 0x80) {
                    continue;
                }
                i32 mask1b = *(i32*)(oj + 0xec) & *(i32*)(oi + 0xe8);
                i32 mask2b = *(i32*)(oj + 0xe8) & *(i32*)(oi + 0xf0);
                if ((mask1b || mask2b) && BoxesOverlap_15a130((CWwdBox*)oj, (CWwdBox*)oi)) {
                    if (mask2b) {
                        CWwdNotifier* nf = *(CWwdNotifier**)(oi + 0x88);
                        if (nf != 0) {
                            *(void**)(oi + 0x8c) = oj;
                            nf->m_callback(oi);
                        }
                    }
                    if (mask1b) {
                        ((CWwdFactoryObject*)oj)->Notify_15b650(oi);
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
struct CWwdBox {
    char m_pad00[0x5c];
    i32 m_screenX; // screen X
    i32 m_screenY; // screen Y
    char m_pad64[0x144 - 0x64];
    i32 m_aabb0Left;   // a1 AABB: left
    i32 m_aabb0Top;    // a1 AABB: top
    i32 m_aabb0Right;  // a1 AABB: right
    i32 m_aabb0Bottom; // a1 AABB: bottom
    i32 m_aabb1Left;   // a2 AABB: left
    i32 m_aabb1Top;    // a2 AABB: top
    i32 m_aabb1Right;  // a2 AABB: right
    i32 m_aabb1Bottom; // a2 AABB: bottom
};
SIZE_UNKNOWN(CWwdBox);
RVA(0x0015a130, 0xdc)
i32 __stdcall BoxesOverlap_15a130(CWwdBox* a1, CWwdBox* a2) {
    if (a2->m_aabb1Left == (i32)0x80000000) {
        return 0;
    }
    if (a1->m_aabb0Left == (i32)0x80000000) {
        return 0;
    }
    i32 a1L = a1->m_aabb0Left + a1->m_screenX;
    i32 a1R = a1->m_aabb0Right + a1->m_screenX;
    i32 a1T = a1->m_aabb0Top + a1->m_screenY;
    i32 a1B = a1->m_aabb0Bottom + a1->m_screenY;
    i32 a2L = a2->m_aabb1Left + a2->m_screenX;
    i32 a2T = a2->m_aabb1Top + a2->m_screenY;
    i32 a2B = a2->m_aabb1Bottom + a2->m_screenY;
    i32 a2R = a2->m_aabb1Right + a2->m_screenX;
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
// geometry via CViewport::WrapCoord: CDDrawSurfacePair::DrawBox(RECT*,color) x3,
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
    char* mgr = (char*)m_parent;
    CDDrawGroupNode* node = m_head;
    ResLoaders::DrawHost_164380* drawHost =
        *(ResLoaders::DrawHost_164380**)(*(char**)(mgr + 4) + 0x14);
    CViewport* view = *(CViewport**)(*(char**)(mgr + 0x24) + 0x5c);
    if (node == 0) {
        return;
    }
    do {
        char* obj = (char*)node->m_obj;
        node = node->m_next;
        i32 ox = *(i32*)(obj + 0x5c);
        i32 oy = *(i32*)(obj + 0x60);
        RECT box;
        SetRect(&box, ox - 0x20, oy - 8, ox + 0x20, oy + 8);
        RECT rc;
        rc.right = box.right;
        rc.bottom = box.bottom;
        i32 wl = box.left;
        i32 wt = box.top;
        i32 fl = view->m_flags;
        if (fl & 4) {
            i32 w = view->m_worldWidth;
            if (box.left < 0) {
                wl = box.left + w;
            } else if (box.left >= w) {
                wl = box.left - w;
            }
            i32 farEdge = view->m_edgeR;
            if (farEdge >= w && wl < view->m_edgeL && wl <= farEdge - w) {
                wl += w;
            }
        }
        if (fl & 8) {
            i32 h = view->m_worldHeight;
            if (box.top < 0) {
                wt = box.top + h;
            } else if (box.top >= h) {
                wt = box.top - h;
            }
            i32 farEdge = view->m_edgeB;
            if (farEdge >= h && wt < view->m_edgeT && wt <= farEdge - h) {
                wt += h;
            }
        }
        rc.left = wl - view->m_edgeL + view->m_scrollX;
        rc.top = wt - view->m_edgeT + view->m_scrollY;
        view->WrapCoord(&rc.right, &rc.bottom);
        drawHost->DrawCount(&rc, *(i32*)(obj + 0x74));
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
i32 CWwdObjMgr::CheckSortOrder_15a780() {
    CWwdNode* node = (CWwdNode*)m_10.GetHeadPosition();
    CWwdObject* anchor = node->m_obj;
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
            CWwdNode* cur = node;
            node = node->m_next;
            anchor = cur->m_obj;
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
        CWwdNode* cur = node;
        node = node->m_next;
        CWwdObject* obj = cur->m_obj;
        if ((obj->m_flags & 0x20000) == 0) {
            i32 curKey = obj->m_sortKey;
            if (key > curKey) {
                anchor->Slot20();
                obj->Slot20();
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
CWwdObject* CWwdObjMgr::FindByType04_15a7f0(i32 type) {
    CWwdNode* node = (CWwdNode*)m_10.GetHeadPosition();
    while (node != 0) {
        CWwdNode* cur = node;
        node = node->m_next;
        CWwdObject* obj = cur->m_obj;
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
CWwdObject* CWwdObjMgr::FindByTypeProbe_15a810(i32 type) {
    CWwdNode* node = (CWwdNode*)m_10.GetHeadPosition();
    while (node != 0) {
        CWwdNode* cur = node;
        node = node->m_next;
        CWwdObject* obj = cur->m_obj;
        if (obj->Slot20() == 5 && obj->m_04 == type) {
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
CWwdObject* CWwdObjMgr::FindByWorker_15a860(i32 type, void* key) {
    CWwdNode* node = (CWwdNode*)m_10.GetHeadPosition();
    if (node == 0) {
        return 0;
    }
    do {
        CWwdNode* cur = node;
        node = node->m_next;
        CWwdObject* obj = cur->m_obj;
        if (obj->Slot20() == 5 && *(i32*)((char*)obj + 0x4) == type) {
            void* worker = *(void**)((char*)obj + 0x7c);
            if (*(i32*)((char*)worker + 0x10) == *(i32*)((char*)key + 0x10)) {
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
struct CChildFinder_15a8c0 {
    char m_pad00[0x0c];
    char* m_parent; // +0x0c
    char m_pad10[0x14 - 0x10];
    void* m_listHead; // +0x14
    void* Find_15a8c0(i32 id, const char* key);
};
SIZE_UNKNOWN(CChildFinder_15a8c0);
RVA(0x0015a8c0, 0x7d)
void* CChildFinder_15a8c0::Find_15a8c0(i32 id, const char* key) {
    CObject* found = 0;
    ((CMapStringToOb*)(*(char**)(m_parent + 0x14) + 0x10))->Lookup(key, found);
    char* node = (char*)m_listHead;
    if (node == 0) {
        return 0;
    }
    char* fp = (char*)found;
    do {
        char* obj = *(char**)(node + 8);
        node = *(char**)node;
        i32 tag = ((CWwdFactoryObject*)obj)->Vs20();
        if (tag == 5 && *(i32*)(obj + 4) == id
            && *(i32*)(*(char**)(obj + 0x7c) + 0x10) == *(i32*)(fp + 0x10)) {
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
CWwdObject* CWwdObjMgr::FindByField_15a940(i32 type, void* key) {
    CWwdNode* node = (CWwdNode*)m_10.GetHeadPosition();
    if (node == 0) {
        return 0;
    }
    do {
        CWwdNode* cur = node;
        node = node->m_next;
        CWwdObject* obj = cur->m_obj;
        if (obj->Slot20() == 5 && *(i32*)((char*)obj + 0x4) == type
            && *(void**)((char*)obj + 0xe8) == key) {
            return obj;
        }
    } while (node != 0);
    return 0;
}

// ---------------------------------------------------------------------------
// 0x15a9a0: return the first list object whose map key (+0x188) equals `key`.
RVA(0x0015a9a0, 0x23)
CWwdObject* CWwdObjMgr::FindByKey_15a9a0(void* key) {
    CWwdNode* node = (CWwdNode*)m_10.GetHeadPosition();
    while (node != 0) {
        CWwdNode* cur = node;
        node = node->m_next;
        CWwdObject* obj = cur->m_obj;
        if (obj->m_key == key) {
            return obj;
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// 0x15a9d0: return the first list object whose status probe (slot +0x20) is 5 and
// whose map key (+0x188) equals `key`.
RVA(0x0015a9d0, 0x45)
CWwdObject* CWwdObjMgr::FindByStatusKey_15a9d0(void* key) {
    CWwdNode* node = (CWwdNode*)m_10.GetHeadPosition();
    while (node != 0) {
        CWwdNode* cur = node;
        node = node->m_next;
        CWwdObject* obj = cur->m_obj;
        if (obj->Slot20() == 5 && obj->m_key == key) {
            return obj;
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// 0x15aa20: uniqueness predicate - return 1 unless two or more list objects share
// +0x04 == `kind`.
RVA(0x0015aa20, 0x3c)
i32 CWwdObjMgr::IsKindUnique_15aa20(i32 kind) {
    CWwdObject* found = 0;
    CWwdNode* node = (CWwdNode*)m_10.GetHeadPosition();
    while (node != 0) {
        CWwdNode* cur = node;
        node = node->m_next;
        CWwdObject* obj = cur->m_obj;
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
i32 CWwdObjMgr::CountByKind_15aa60(i32 kind) {
    i32 count = 0;
    CWwdNode* node = (CWwdNode*)m_10.GetHeadPosition();
    while (node != 0) {
        CWwdNode* cur = node;
        node = node->m_next;
        CWwdObject* obj = cur->m_obj;
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
void CWwdObjMgr::PruneList_15aa90() {
    CWwdNode* node = (CWwdNode*)m_10.GetHeadPosition();
    while (node != 0) {
        CWwdNode* cur = node;
        node = node->m_next;
        CWwdObject* obj = cur->m_obj;
        if (obj != 0 && !(obj->m_flags & 0x200)) {
            m_10.RemoveAt((POSITION)cur);
            m_2c.RemoveKey(obj->m_key);
            m_48.RemoveKey(obj->m_key);
            delete obj;
        }
    }
}

// ---------------------------------------------------------------------------
// 0x15aaf0: accumulate SUM over the list of index*(obj->m_5c + m_74 + m_60 + m_04).
// @early-stop
// 99.15% - logic/CFG/offsets byte-exact. Residual: cl reassociates the 4-term
// commutative sum to load m_74 into the accumulator first, where retail loads m_5c
// first. Documented add-reassociation wall (permuter no-op).
RVA(0x0015aaf0, 0x35)
i32 CWwdObjMgr::SumWeighted_15aaf0() {
    i32 sum = 0;
    i32 i = 0;
    CWwdNode* node = (CWwdNode*)m_10.GetHeadPosition();
    while (node != 0) {
        CWwdNode* cur = node;
        node = node->m_next;
        CWwdObject* obj = cur->m_obj;
        sum += i * (obj->m_5c + obj->m_sortKey + obj->m_60 + obj->m_04);
        ++i;
    }
    return sum;
}

// ---------------------------------------------------------------------------
// 0x15ab30: drop a list slot + BOTH map entries (the +0x2c primary AND the
// +0x48 active set).
RVA(0x0015ab30, 0x38)
void CWwdObjMgr::RemoveAll_15ab30(i32 pos, CWwdObject* obj) {
    m_10.RemoveAt((POSITION)pos);
    m_2c.RemoveKey(obj->m_key);
    m_48.RemoveKey(obj->m_key);
}

// ---------------------------------------------------------------------------
// 0x15ab70: drop a list slot + its primary-map entry.
RVA(0x0015ab70, 0x27)
void CWwdObjMgr::RemoveByPosition_15ab70(i32 pos, CWwdObject* obj) {
    m_10.RemoveAt((POSITION)pos);
    m_2c.RemoveKey(obj->m_key);
}

// ---------------------------------------------------------------------------
// 0x15aba0: m_48[obj->key] = obj.
RVA(0x0015aba0, 0x1a)
void CWwdObjMgr::AddToMap48_15aba0(CWwdObject* obj) {
    m_48[obj->m_key] = obj;
}

// ---------------------------------------------------------------------------
// 0x15abc0: count m_48 entries whose object lacks flag 0x4000000.
RVA(0x0015abc0, 0x5e)
i32 CWwdObjMgr::CountActive_15abc0() {
    i32 n = 0;
    POSITION pos = (POSITION)(m_48.GetCount() != 0 ? -1 : 0);
    if (pos != 0) {
        do {
            void* key = 0;
            CWwdObject* val = 0;
            m_48.GetNextAssoc(pos, key, (void*&)val);
            if (val != 0 && !(val->m_flags & 0x4000000)) {
                ++n;
            }
        } while (pos != 0);
    }
    return n;
}

// ---------------------------------------------------------------------------
// 0x15ac20: for each active m_48 object, dispatch its +0x3c virtual with the
// three args + the object; always returns 1. Returns 0 immediately if a1==0.
RVA(0x0015ac20, 0x81)
i32 CWwdObjMgr::ForEachDispatch_15ac20(i32 a1, i32 a2, i32 a3) {
    if (a1 == 0) {
        return 0;
    }
    POSITION pos = (POSITION)(m_48.GetCount() != 0 ? -1 : 0);
    if (pos != 0) {
        do {
            void* key = 0;
            CWwdObject* val = 0;
            m_48.GetNextAssoc(pos, key, (void*&)val);
            if (val != 0 && !(val->m_flags & 0x4000000)) {
                val->Slot3C(a1, a2, a3, val);
            }
        } while (pos != 0);
    }
    return 1;
}

// ---------------------------------------------------------------------------
// 0x15acb0: for each active m_48 object, run its __thiscall probe at 0x151c00
// with (a1, a2); always returns 1. Returns 0 immediately if a1==0.
RVA(0x0015acb0, 0x76)
i32 CWwdObjMgr::ForEachProbe_15acb0(i32 a1, i32 a2) {
    if (a1 == 0) {
        return 0;
    }
    POSITION pos = (POSITION)(m_48.GetCount() != 0 ? -1 : 0);
    if (pos != 0) {
        do {
            void* key = 0;
            CWwdObject* val = 0;
            m_48.GetNextAssoc(pos, key, (void*&)val);
            if (val != 0 && !(val->m_flags & 0x4000000)) {
                ((CWwdGameObject*)val)->WriteSnapshot(a1, a2);
            }
        } while (pos != 0);
    }
    return 1;
}

// ===========================================================================
// CWwdObjMgr::LoadObjects @0x15ad30 - iterate the level reader's `count` object
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
i32 CWwdObjMgr::LoadObjects(CSerialArchive* reader, u32 count, i32 unused) {
    i32 savedCounter = 0;
    if (reader == 0) {
        return 0;
    }
    for (u32 i = 0; i < count; i++) {
        WwdObjDesc desc;
        reader->Read(&desc, 0xa0);

        void* found;
        if (m_48.Lookup((void*)desc.m_04, found) && found != 0) {
            return 0;
        }

        savedCounter = g_wwdObjIdCounter;
        g_wwdObjIdCounter = desc.m_04;

        CWwdGameObject* createdObj = 0;
        switch (desc.m_08) {
            case 5: {
                void* val;
                ((CMapStringToPtr*)(m_0c->m_14 + 0x10))
                    ->Lookup((const char*)desc.m_14, (void*&)val);
                if (val != 0) {
                    createdObj = CreateObject_159600(
                        desc.m_00,
                        desc.m_94,
                        desc.m_98,
                        desc.m_9c,
                        (i32)val,
                        0
                    );
                }
                break;
            }
            case 0x16: {
                void* val;
                ((CMapStringToPtr*)(m_0c->m_14 + 0x10))
                    ->Lookup((const char*)desc.m_14, (void*&)val);
                createdObj = CreateObject_159440(desc.m_00, desc.m_9c, (i32)val, 0);
                break;
            }
            case 0x1b: {
                void* val;
                ((CMapStringToPtr*)(m_0c->m_14 + 0x10))
                    ->Lookup((const char*)desc.m_14, (void*&)val);
                if (val != 0) {
                    createdObj = CreateObject_1598d0(
                        desc.m_00,
                        desc.m_94,
                        desc.m_98,
                        desc.m_9c,
                        (i32)val,
                        0
                    );
                }
                break;
            }
            case 0x1c: {
                void* rec = 0;
                if (m_0c->BuildChild(reader, 0xa, desc.m_0c, &rec) == 0) {
                    return 0;
                }
                if (rec == 0) {
                    return 0;
                }
                *(i32*)((char*)rec + 4) = desc.m_00;
                if (Init_159830(rec, desc.m_94, desc.m_98, desc.m_9c, desc.m_14, 0) == 0) {
                    return 0;
                }
                createdObj = (CWwdGameObject*)rec;
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
            if (m_0c->BuildChild(reader, 9, desc.m_10, &child) == 0) {
                return 0;
            }
            if (child == 0) {
                return 0;
            }
            createdObj->m_7c->m_18 = child;
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// 0x15b020: for each active m_48 object, write its key to the archive then run
// its +0x3c virtual; bail to 0 on a 0 result, else 1. Returns 0 if ar==0.
// @early-stop
// 90.1% - the per-element block is byte-exact; the residual is the loop-tail
// structure: retail emits a bottom-tested loop with two distinct `return 1`
// epilogues, our cl hoists the body and merges the epilogue. An optimizer
// CFG-shape choice; logic exact.
RVA(0x0015b020, 0xc0)
i32 CWwdObjMgr::ForEachSerialize_15b020(CSerialArchive* ar, i32 a2) {
    if (ar == 0) {
        return 0;
    }
    POSITION pos = (POSITION)(m_48.GetCount() != 0 ? -1 : 0);
    if (pos != 0) {
        do {
            void* key = 0;
            CWwdObject* val = 0;
            m_48.GetNextAssoc(pos, key, (void*&)val);
            if (val != 0 && !(val->m_flags & 0x4000000)) {
                void* k = val->m_key;
                ar->Write(&k, 4);
                if (val->Slot3C((i32)ar, 4, a2, val) == 0) {
                    return 0;
                }
            }
        } while (pos != 0);
    }
    return 1;
}

// ---------------------------------------------------------------------------
// 0x15b0e0: the read/load counterpart of ForEachSerialize - pull `count` object
// keys back through the archive, resolve each in m_48, require it present + alive
// (+0x7c != 0), then run its +0x3c dispatch with (ar, 7, flag, obj).
// @early-stop
// retail carries a conditional CString-cleanup latch (a name temp tracked by a
// stack "alive" flag) whose CONSTRUCTION the optimizer elided in this
// instantiation, leaving a never-taken `~CString` cleanup branch. The latch isn't
// reproducible from C without re-introducing the (here dead) name build; logic /
// CFG / offsets are exact, the dead cleanup branch is the residual.
RVA(0x0015b0e0, 0xec)
i32 CWwdObjMgr::Deserialize_15b0e0(CSerialArchive* ar, u32 count, i32 flag) {
    if (ar == 0) {
        return 0;
    }
    for (u32 i = 0; i < count; i++) {
        void* key = 0;
        ar->Read(&key, 4);
        if (key == 0) {
            return 0;
        }
        CWwdObject* obj = 0;
        if (!m_48.Lookup(key, (void*&)obj)) {
            obj = 0;
        }
        if (obj == 0) {
            return 0;
        }
        if (*(i32*)((char*)obj + 0x7c) == 0) {
            return 0;
        }
        if (obj->Slot3C((i32)ar, 7, flag, obj) == 0) {
            return 0;
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// 0x15b1d0: for each m_48 object, look its key up in m_2c; if absent, remove it
// from m_48 and destroy it. Returns the number removed.
// @early-stop
// 94.2% - logic/CFG/offsets exact; residual is the Lookup `found`-slot handling:
// retail reads `found` into a register only on Lookup-success, our cl re-zeroes
// the slot and compares memory. A found-slot regalloc coin-flip.
RVA(0x0015b1d0, 0x9b)
i32 CWwdObjMgr::PruneOrphans_15b1d0() {
    i32 n = 0;
    POSITION pos = (POSITION)(m_48.GetCount() != 0 ? -1 : 0);
    if (pos != 0) {
        do {
            void* key = 0;
            CWwdObject* val = 0;
            m_48.GetNextAssoc(pos, key, (void*&)val);
            if (val != 0) {
                void* found = 0;
                if (!m_2c.Lookup(val->m_key, found)) {
                    found = 0;
                }
                if (found == 0) {
                    m_48.RemoveKey(val->m_key);
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
Obj15b270::Obj15b270() {
    m_8 = (i32)0x80000000;
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
Obj15b2b0::Obj15b2b0() {
    m_c = 0;
    m_8 = 0;
    m_18 = 0;
}
