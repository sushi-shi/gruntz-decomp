// CWwdObjMgrFactories.cpp - the per-kind CWwdGameObject factories that are
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

#include <Gruntz/CWwdObjMgr.h> // the shared object-collection manager class
#include <Gruntz/CWwdWorker.h> // the shared per-object worker class
#include <Globals.h>

// Engine heap allocator (operator new / RezAlloc). Reloc-masked __cdecl extern.
extern "C" void* RezAlloc(unsigned int size); // 0x1b9b46

// The running WWD object-id counter (?g_wwdObjIdCounter@@3HA @ 0x61ab14).

// Intermediate (post-base) and final wide-object vtables. Reloc-masked DATA
// externs (RVA = VA - 0x400000). Each factory's object has its own final vtable.
DATA(0x001f0020)
extern void* g_wwdGameObjectVtbl; // 0x5f0020  (post-base, 159250/159440)
DATA(0x001effd0)
extern void* g_wwd159250FinalVtbl; // 0x5effd0
DATA(0x001f0060)
extern void* g_wwd159440FinalVtbl; // 0x5f0060
DATA(0x001f0128)
extern void* g_wwdSubVtbl; // 0x5f0128  (+0x1a0 sub-object, 1598d0)
DATA(0x001f00a8)
extern void* g_wwdObjVtbl; // 0x5f00a8  (post-base, 1598d0)
DATA(0x001f00e8)
extern void* g_wwd1598d0FinalVtbl; // 0x5f00e8

class CWwdGameObject;

// --- sub-object ctors (all __thiscall, reloc-masked no-body) -----------------
// Base "CRemusNode" 3-arg ctor (root, a, flags). 0x15b2c0 for the 159250/159440
// objects, 0x15b390 for the 1598d0 object.
class CWwdRemusBaseA {
public:
    void Ctor(int root, int a, int flags); // 0x15b2c0
};
class CWwdRemusBaseB {
public:
    void Ctor(int root, int a, int flags); // 0x15b390
};
// The +0x9c sub-object (159250/159440): ctor 0x15b2a0, then its +0x18 is zeroed.
class CWwdSub9c {
public:
    void Ctor(); // 0x15b2a0
    char m_pad00[0x18];
    int m_18; // +0x18  -> obj+0xb4
};
// The +0xb8 sub-object ctor. 0x15b270.
class CWwdSubB8 {
public:
    void Ctor(); // 0x15b270
};
// The CString label ctor at +0xdc. 0x1b9b93.
class CWwdLabel {
public:
    void Ctor(); // 0x1b9b93
};
// The +0x7c sprite-animation worker (0x17c bytes): ctor 0x15b300, kick at +0x10.
// CWwdWorker is the shared <Gruntz/CWwdWorker.h> class (the per-object worker at +0x7c).
// The +0x1a0 command sub-object ctor (1598d0). 0x156cb0.
class CWwdCmd1a0 {
public:
    void Ctor(int root, int a, int flags); // 0x156cb0
};
// The +0x1dc list sub-object ctor (1598d0), 1 arg (block size 0xa). 0x1b59cc.
class CWwdList1dc {
public:
    void Ctor(int blockSize); // 0x1b59cc
};

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

// CWwdObjMgr is the shared <Gruntz/CWwdObjMgr.h> class; here only the +0x0c parent
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
        ((CWwdRemusBaseA*)obj)->Ctor(root, a1, a7);
        CWwdSub9c* s9c = (CWwdSub9c*)(obj + 0x9c);
        s9c->Ctor();
        s9c->m_18 = 0;
        ((CWwdSubB8*)(obj + 0xb8))->Ctor();
        ((CWwdLabel*)(obj + 0xdc))->Ctor();
        *(void**)obj = &g_wwdGameObjectVtbl;
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
        *(void**)obj = &g_wwd159250FinalVtbl;
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
        ((CWwdRemusBaseA*)obj)->Ctor(root, a1, a4);
        CWwdSub9c* s9c = (CWwdSub9c*)(obj + 0x9c);
        s9c->Ctor();
        s9c->m_18 = 0;
        ((CWwdSubB8*)(obj + 0xb8))->Ctor();
        ((CWwdLabel*)(obj + 0xdc))->Ctor();
        *(void**)obj = &g_wwdGameObjectVtbl;
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
        *(void**)obj = &g_wwd159440FinalVtbl;
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
        ((CWwdRemusBaseB*)obj)->Ctor(root, a1, a6);
        ((CWwdCmd1a0*)(obj + 0x1a0))->Ctor(root, a1, a6);
        *(void**)(obj + 0x1a0) = &g_wwdSubVtbl;
        *(int*)(obj + 0x1b0) = 0;
        *(int*)(obj + 0x1b4) = 0;
        *(int*)(obj + 0x1b8) = 0;
        *(void**)obj = &g_wwdObjVtbl;
        *(int*)(obj + 0x18c) = -1;
        *(int*)(obj + 0x190) = -1;
        *(int*)(obj + 0x198) = 0;
        *(int*)(obj + 0x194) = 0;
        *(int*)(obj + 0x19c) = 0;
        ((CWwdList1dc*)(obj + 0x1dc))->Ctor(0xa);
        *(void**)obj = &g_wwd1598d0FinalVtbl;
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
class CWwdObjList {
public:
    void* AddTail(void* obj); // 0x1b5af6
};
class CWwdObjMgrL {
public:
    CWwdGameObject* CreateObject_166640(int a1, int a2, int a3, int a4, int a5, int a6);
    char m_pad00[0x0c];
    int m_0c; // +0x0c parent handle
    char m_pad10[0x1dc - 0x10];
    CWwdObjList m_1dc; // +0x1dc published-objects list
};

RVA(0x00166640, 0x13b)
CWwdGameObject* CWwdObjMgrL::CreateObject_166640(int a1, int a2, int a3, int a4, int a5, int a6) {
    char* obj = (char*)RezAlloc(0x1dc);
    CWwdGameObject* result;
    if (obj != 0) {
        int root = m_0c;
        ((CWwdRemusBaseB*)obj)->Ctor(root, a1, a6);
        *(int*)(obj + 0x1a4) = a1;
        *(int*)(obj + 0x1a8) = a6;
        *(int*)(obj + 0x1ac) = root;
        *(void**)(obj + 0x1a0) = &g_wwdSubVtbl;
        *(int*)(obj + 0x1b0) = 0;
        *(int*)(obj + 0x1b4) = 0;
        *(int*)(obj + 0x1b8) = 0;
        *(void**)obj = &g_wwdObjVtbl;
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
    void* node = m_1dc.AddTail(result);
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

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
SIZE_UNKNOWN(CWwdCmd1a0);
SIZE_UNKNOWN(CWwdFactoryA);
SIZE_UNKNOWN(CWwdFactoryB);
SIZE_UNKNOWN(CWwdList1dc);
SIZE_UNKNOWN(CWwdObjList);
SIZE_UNKNOWN(CWwdObjMgrL);
SIZE_UNKNOWN(CWwdRemusBaseA);
SIZE_UNKNOWN(CWwdRemusBaseB);
SIZE_UNKNOWN(CWwdSub9c);
SIZE_UNKNOWN(CWwdSubB8);
