// WwdObjMgrFactories.cpp - the per-kind CWwdGameObject factories that are
// siblings of CWwdObjMgr::CreateObject (0x159600, modelled in CDDrawSubMgr.cpp).
// Each RezAlloc's a wide object, constructs its base + sub-objects in place,
// stamps the intermediate then final vtable, registers it (InsertSorted_159e40),
// runs the polymorphic Build, and (when flags & 0x200000) kicks the +0x7c worker.
//
// Self-contained TU (no shared header): the helper sub-object ctors, the manager
// view (only +0x0c is read), and the wide-object vtable externs are modelled
// locally so every call rel32 / DIR32 reloc-masks. Field names are placeholders;
// only OFFSETS + emitted bytes are load-bearing (campaign doctrine).
//
// All three are @early-stop on the rezalloc-placement-new wall: MSVC5 predates
// placement operator delete, so a `(T*)RezAlloc(); placement-new ctor` body
// emits NO ctor-in-flight /GX EH frame, while retail allocates via a throwing
// class operator new and carries the full frame (push -1/fs:0 + the per-member
// EH-state chain + one shared jmp epilogue). The construction body is byte-exact;
// only the absent frame + frameless early-returns cascade the offsets (~47-66%).
// See docs/patterns/rezalloc-placement-new-no-eh-frame.md. The final-sweep upgrade
// is to model each wide object as a real `new T`-with-throwing-ctor class.
#include <rva.h>
#include <Mfc.h> // CObList (m_1dc fold)

#include <Gruntz/WwdObjMgr.h> // the shared object-collection manager class
#include <Gruntz/WwdWorker.h> // the shared per-object worker class
#include <Globals.h>

inline void* operator new(u32, void* p) {
    return p;
} // placement (factory sub-object ctors)

// Engine heap allocator (operator new / RezAlloc). Reloc-masked __cdecl extern.
extern "C" void* RezAlloc(unsigned int size); // 0x1b9b46

// The running WWD object-id counter (?g_wwdObjIdCounter@@3HA @ 0x61ab14).

// Wide-object vtables the factories INLINE-stamp mid-construction. The disasm
// (dump_target 0x1598d0 @0x159948/0x159957/0x159991) proves retail writes each
// `mov [reg], offset <vtable>` INLINE - NOT a ctor call - so these stay factory
// inline-construction stamps (a real ctor call would emit `call Ctor`, mismatch).
// Reloc-masked externs (RVA = VA - 0x400000).
//
// The five wide-object tables below are REALIZED dtor-first in
// src/Gruntz/WwdGameObjectEh.cpp (CWwdGameObjectE/C/F/A/B), but only the dtor slot
// is modeled there (cl emits a short orphan ??_7 with the dtor at slot 0), while
// retail's tables are 16-19 slots with the scalar-deleting dtor at slot 1 - so a
// VTBL binding here would name a wrong-slot datum (a tooling artifact). The g_ DATA
// symbol therefore keeps naming the (un-modeled) retail datum; a real ??_7 catalog
// name awaits the wide-object-ctor `new`-rewrite sweep. Kept pinned.
// The +0x1a0 sub-object table 0x5f0128 IS fully realized: ??_7CAniAdvanceCursor
// (9 slots, dtor at slot 1) in src/Wwd/AniAdvanceCursor.cpp, which OWNS the RVA
// catalog name via VTBL. UNPINNED so the factory's inline sub-object stamp
// reloc-masks against the real ??_7 (the manual g_wwdSubVtbl DATA placeholder is
// drained).

class CWwdGameObject;

// --- sub-object ctors (all __thiscall, reloc-masked no-body) -----------------
// DISPOSITION: the wide CWwdGameObject's base is the real CResolveNode (RTTI vtable
// @0x5efbc0, named), but its embedded sub-objects (CWwdSub9c/SubB8/Cmd1a0/List1dc)
// have NO RTTI on their vtables - the only nameable identity in this cluster is
// CResolveNode. These placeholders are the embedded-sub-object ctors modeled by
// this-offset so the placement-ctor calls reloc-mask; the whole factory sits at the
// rezalloc-placement-new EH wall (@early-stop), so they are kept as honest minimal
// models (no fabricated class names) pending a matcher that models `new T`.
// Base "CResolveNode" 3-arg ctor (root, a, flags). 0x15b2c0 for the 159250/159440
// objects, 0x15b390 for the 1598d0 object.
SIZE_UNKNOWN(CResolveNode);
struct CResolveNode {
    CResolveNode(int root, int a, int flags); // 0x15b2c0
};
class CWwdResolveBaseB {
public:
    void Ctor(int root, int a, int flags); // 0x15b390
};
// The +0x9c sub-object (159250/159440): ctor 0x15b2a0, then its +0x18 is zeroed.
struct CWwdSlot9c {
    CWwdSlot9c(); // 0x15b2a0
    char m_pad00[0x18];
    int m_18; // +0x18  -> obj+0xb4
};
// The +0xb8 sub-object ctor. 0x15b270.
SIZE_UNKNOWN(Obj15b270);
struct Obj15b270 {
    Obj15b270(); // 0x15b270
};
// The +0x7c sprite-animation worker (0x17c bytes): ctor 0x15b300, kick at +0x10.
// CWwdWorker is the shared <Gruntz/WwdWorker.h> class (the per-object worker at +0x7c).
class CWwdCmd1a0 {
public:
    void Ctor(int root, int a, int flags); // 0x156cb0
};
// The +0x1dc list sub-object ctor (1598d0), 1 arg (block size 0xa). 0x1b59cc.

// The wide objects' polymorphic interfaces (cast-only; declared-only virtuals so
// cl emits no ??_7). Each factory's object has a distinct vtable, so the Build
// slot differs: 0x159250 -> +0x40 (5 args), 0x159440 -> +0x40 (2 args), 0x1598d0
// -> +0x28 (4 args). All share the scalar-deleting dtor at +0x04.
class CWwdFactoryA { // 159250 (+0x40, 5 args) and 1598d0 (+0x28, 4 args)
public:
    virtual void Vs00();
    virtual int ScalarDtor(int flag); // +0x04
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
class CWwdFactoryB { // 159440 (+0x40, 2 args)
public:
    virtual void Vs00();
    virtual int ScalarDtor(int flag); // +0x04
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

// CWwdObjMgr is the shared <Gruntz/WwdObjMgr.h> class; here only the +0x0c parent
// handle is read (as a raw int) and InsertSorted_159e40 publishes each object.
// The CWwdGameObject the factories return is cast to the shared CWwdObject* param.
class CWwdGameObject;

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
            ((CWwdFactoryA*)result)->ScalarDtor(1);
        }
        return 0;
    }
    InsertSorted_159e40((CWwdObject*)result, 1);
    if (a7 & 0x200000) {
        ((CWwdWorker*)*(void**)((char*)result + 0x7c))->Kick(result);
    }
    return result;
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
            ((CWwdFactoryB*)result)->ScalarDtor(1);
        }
        return 0;
    }
    InsertSorted_159e40((CWwdObject*)result, 1);
    if (a4 & 0x200000) {
        ((CWwdWorker*)*(void**)((char*)result + 0x7c))->Kick(result);
    }
    return result;
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
        ((CWwdResolveBaseB*)obj)->Ctor(root, a1, a6);
        ((CWwdCmd1a0*)(obj + 0x1a0))->Ctor(root, a1, a6);
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
            ((CWwdFactoryA*)result)->ScalarDtor(1);
        }
        return 0;
    }
    InsertSorted_159e40((CWwdObject*)result, 1);
    if (a6 & 0x200000) {
        ((CWwdWorker*)*(void**)((char*)result + 0x7c))->Kick(result);
    }
    return result;
}

// ===========================================================================
// 0x166640 - factory for the 0x1dc-byte kind, sibling of the above but published
// into the manager's own CPtrList (AddTail) at +0x1dc rather than InsertSorted.
// __thiscall, 6 stack args (ret 0x18).  Build slot +0x28 (4 args), dtor +0x04.
// @early-stop
// rezalloc-placement-new wall (same family as 0x1598d0): the object construction
// + field stores + vtable stamps are byte-exact, but retail allocates the object
// through the throwing class operator new and carries the /GX ctor-in-flight EH
// frame (push -1/fs:0 + trylevel-0 cleanup), while the RezAlloc + placement body
// emits no frame.  docs/patterns/rezalloc-placement-new-no-eh-frame.md.
// ===========================================================================
// The manager's own published-objects list (CPtrList) at +0x1dc; AddTail returns
// the new node pointer (stored into the object's +0x78).  Reloc-masked thiscall.
class CWwdObjMgrL {
public:
    CWwdGameObject* CreateObject_166640(int a1, int a2, int a3, int a4, int a5, int a6);
    char m_pad00[0x0c];
    int m_0c; // +0x0c parent handle
    char m_pad10[0x1dc - 0x10];
    CObList m_1dc; // +0x1dc published-objects list (real MFC, main's fold)
};

RVA(0x00166640, 0x13b)
CWwdGameObject* CWwdObjMgrL::CreateObject_166640(int a1, int a2, int a3, int a4, int a5, int a6) {
    char* obj = (char*)RezAlloc(0x1dc);
    CWwdGameObject* result;
    if (obj != 0) {
        int root = m_0c;
        ((CWwdResolveBaseB*)obj)->Ctor(root, a1, a6);
        *(int*)(obj + 0x1a4) = a1;
        *(int*)(obj + 0x1a8) = a6;
        *(int*)(obj + 0x1ac) = root;
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
        result = (CWwdGameObject*)obj;
    } else {
        result = 0;
    }
    if (result == 0) {
        return 0;
    }
    if (((CWwdFactoryA*)result)->Build4(a2, a3, a4, a5) == 0) {
        ((CWwdFactoryA*)result)->ScalarDtor(1);
        return 0;
    }
    void* node = m_1dc.AddTail((CObject*)result);
    if (node == 0) {
        ((CWwdFactoryA*)result)->ScalarDtor(1);
        return 0;
    }
    *(void**)(obj + 0x78) = node;
    if (*(int*)(obj + 8) & 0x200000) {
        ((CWwdWorker*)*(void**)(obj + 0x7c))->Kick(result);
    }
    return result;
}

// ---------------------------------------------------------------------------
// CWwdGameObj15b390::Construct (0x15b390) - the shared CWwdGameObject base-object
// ctor the wide-object factories call (CreateObject_1598d0/166640 do
// `((CWwdResolveBaseB*)obj)->Ctor(...)` == call 0x15b390; also WwdFile::ReadPlaneObjects
// 0x162af0). It is a REAL /GX ctor: the CResolveNode base subobject stamps ??_7CResolveNode
// (0x5efbc0) + its +0x04..+0xd8 fields, then the CString label member (+0xdc, ??0CString@@
// 0x1b9b93) constructs, then the derived body final-stamps g_wwdGameObjectVtbl (0x5f0020),
// `new`-allocates the +0x7c AnimWorkerObj worker (??_7 0x5efb80, 0x17c bytes; op-new 0x1b9b46
// is null-checked -> MSVC5 nothrow new), and publishes g_wwdObjIdCounter (0x61ab14).
// The CString member + throwing op-new give the ctor its retail /GX ctor-in-flight EH frame.
//
// Foreign vtables reloc-mask (the real ??_7CResolveNode / ??_7AnimWorkerObj live in the
// Image / DDrawWorkerCache TUs; g_wwdGameObjectVtbl is the un-modeled 0x5f0020 datum).

// The +0x7c sprite/anim worker (0x17c bytes). `new`-constructed inline in the base ctor;
// self-contained inline-construction view (only the field stores + vtable stamp matter).
struct WwdAnimWorker {
    WwdAnimWorker(int b, int a) {
        m_04 = b;
        m_08 = 0;
        m_0c = a;
        // vptr install dropped -> compiler-emitted vtable (% ok per drive-to-0) // 0x5efb80
        m_10 = 0;
        m_14 = 0;
        m_18 = 0;
        m_170 = 0;
        m_1c = 0;
        m_174 = 0;
        m_178 = 0;
    }
    char _vft0[4]; // +0x00 foreign object vptr (reduced view; not owned/dispatched)
    int m_04, m_08, m_0c, m_10, m_14, m_18, m_1c;
    char _p20[0x170 - 0x20];
    int m_170, m_174, m_178;
};

// The CResolveNode base subobject: stamps 0x5efbc0 + the +0x04..+0xd8 field block.
struct WwdCtorBase {
    WwdCtorBase(int a, int b, int c) {
        m_08 = c;
        m_04 = b;
        m_0c = a;
        m_20 = (int)0x80000000;
        m_38 = -1;
        // vptr install dropped -> compiler-emitted vtable (% ok per drive-to-0) // 0x5efbc0
        m_5c = (int)0x80000000;
        m_64 = (int)0x80000000;
        m_3c = 0;
        m_40 = 0;
        m_a8 = 0;
        m_a4 = 0;
        m_b4 = 0;
        m_c0 = (int)0x80000000;
        m_d8 = -1;
    }
    char _vft0[4]; // +0x00 foreign object vptr (reduced view; not owned/dispatched)
    int m_04, m_08, m_0c;
    char _p10[0x20 - 0x10];
    int m_20;
    char _p24[0x38 - 0x24];
    int m_38, m_3c, m_40;
    char _p44[0x5c - 0x44];
    int m_5c;
    char _p60[0x64 - 0x60];
    int m_64;
    char _p68[0x78 - 0x68];
    int m_78;
    void* m_7c; // +0x7c worker
    int m_80;
    char _p84[0x88 - 0x84];
    int m_88;
    char _p8c[0x90 - 0x8c];
    int m_90;
    char _p94[0x98 - 0x94];
    int m_98;
    char _p9c[0xa4 - 0x9c];
    int m_a4, m_a8;
    char _pac[0xb4 - 0xac];
    int m_b4;
    char _pb8[0xc0 - 0xb8];
    int m_c0;
    char _pc4[0xd8 - 0xc4];
    int m_d8;
};

struct CWwdGameObj15b390 : public WwdCtorBase {
    CString m_label; // +0xdc  ??0CString (0x1b9b93)
    char _pe0[0x188 - 0xe0];
    int m_188; // +0x188  object id
    CWwdGameObj15b390(int a, int b, int c);
};

// @early-stop
// eh-member-state wall (59.7%, up from a 1.77% bare @stub): the real /GX ctor
// (CResolveNode base stamp 0x5efbc0 + CString member ctor 0x1b9b93 + final stamp 0x5f0020
// + `new` AnimWorkerObj worker + id publish) is byte-faithful store-for-store, but MSVC5
// declines to emit the retail member-construction EH-state machine: retail bumps the state
// cookies (`mov [esp+0x1c],0` -> `BYTE 2` before the CString ctor -> `BYTE 3` before the
// worker op-new) so an unwind destroys the live CString, and pins `lea ecx,[esi+0xdc]` at
// that state-2 boundary; our cl treats the inline worker `new` as non-throwing (no throw
// point after the member), so it emits no state cookies and freely hoists the `lea`. Not
// source-steerable: `::operator new`+placement REGRESSED (57.2%), a declared worker dtor is
// neutral, and out-of-lining the worker ctor would mismatch retail's inline stores.
// docs/patterns/eh-state-numbering-base.md + throwing-operator-new-eh-state-transition.md.
RVA(0x0015b390, 0x128)
CWwdGameObj15b390::CWwdGameObj15b390(int a, int b, int c) : WwdCtorBase(a, b, c) {
    // factory ctor vptr install dropped (model as compiler-emitted vtable; % ok per drive-to-0) // 0x5f0020 final stamp
    m_5c = (int)0x80000000;
    m_78 = 0;
    m_7c = new WwdAnimWorker(b, a);
    m_98 = 0;
    m_80 = 0;
    m_88 = 0;
    m_90 = 0;
    m_188 = g_wwdObjIdCounter;
    g_wwdObjIdCounter = g_wwdObjIdCounter + 1;
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
SIZE_UNKNOWN(CWwdCmd1a0);
SIZE_UNKNOWN(CWwdFactoryA);
SIZE_UNKNOWN(CWwdFactoryB);
SIZE_UNKNOWN(CWwdObjMgrL);
SIZE_UNKNOWN(CWwdResolveBaseB);
SIZE_UNKNOWN(CWwdSlot9c);
SIZE_UNKNOWN(CWwdGameObj15b390); // 0x15b390 per-kind wide-object ctor (CResolveNode base)
SIZE_UNKNOWN(WwdCtorBase);       // CResolveNode base subobject (+0x00..+0xd8)
SIZE(WwdAnimWorker, 0x17c);      // the +0x7c anim worker

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---
