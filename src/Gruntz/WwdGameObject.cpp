// WwdGameObject.cpp - leaf methods of CWwdGameObject, a runtime "plane object"
// deserialized from WWD level data (constructed by WwdFile::ReadPlaneObjects,
// 0x162af0, via the EH-frame ctor at 0x15b390 which is NOT reconstructed here).
//
// The object owns a sprite-animation worker at +0x7c (AnimWorker, foreign
// vtable - virtuals dispatched, never defined), a command-dispatch sub-object
// at +0x1a0 (CmdMap), and a back-pointer to its owning manager at +0x0c.
//
// Field names are placeholders m_<hexoffset>; only the OFFSETS + emitted bytes
// are load-bearing (campaign doctrine). The large object (size >0x1a0) is
// accessed via documented raw this-offset reads - a deliberate, justified choice
// for naming-independent codegen (the family TUs do the same).
//
// These are plain /O2 /MT leaves: NO SEH frame (the throwing ctor lives in the
// eh unit). External callees (the sub-object ctor/find, archive Read/Write
// virtual [+0x30], CString dtor, NAFXCW Lookup, sibling readers) are modeled
// with no body so their rel32 calls reloc-mask.
#include <rva.h>
#include <string.h> // inlined memset / strcpy (rep stos / repne scas + rep movs)
#include <Gruntz/WwdGameObject.h>

// ---------------------------------------------------------------------------
// The +0x1a0 command-dispatch sub-object. Ctor 0x15c290 (1 arg = owner),
// Find 0x15c900 (4 args). Modeled as a tiny class so the __thiscall dispatch
// lowers to `lea ecx,[this+0x1a0]; call` with no cast.
// ---------------------------------------------------------------------------
struct CmdMap {
    void Construct(void* owner);              // 0x15c290
    i32 Find(i32 a1, i32 a2, i32 a3, i32 a4); // 0x15c900
};

// The owning manager at +0x0c and its nested helpers (reached as [[+0xc]+slot]).
// Modeled via typed vtable-style structs so the chained derefs match exactly.
// A CString-like value (4-byte handle = pointer to the heap char data). The
// engine's CString is one pointer; its data starts at the pointed-to address.
struct EngStr {
    const char* m_data; // +0x00  -> the char buffer
};
struct MgrSub158570 {
    EngStr* Op(EngStr* out, i32 a); // 0x158570  __thiscall, returns out (CString)
};

// The archive/stream passed to ReadState/Serialize/WriteSnapshot. Its op is the
// virtual at +0x30: a fixed-size element transfer (read or write `size` bytes).
struct Archive {
    virtual void Slot00();
    virtual void Slot04();
    virtual void Slot08();
    virtual void Slot0C();
    virtual void Slot10();
    virtual void Slot14();
    virtual void Slot18();
    virtual void Slot1C();
    virtual void Slot20();
    virtual void Slot24();
    virtual void Slot28();
    virtual void ReadBuf(void* buf, i32 size); // +0x2c (the read/load direction)
    virtual void Xfer(void* buf, i32 size);    // +0x30 (the write/store direction)
};
struct EngStr;
struct MgrSub165360 {
    EngStr* Build(EngStr* out, void* obj); // 0x165360  __thiscall -> CString
};

// The 0xa0-byte snapshot record WriteSnapshot assembles on the stack.
struct WwdSnapshot {
    i32 m_00;          // m_4
    i32 m_04;          // m_188
    i32 m_08;          // this->Vfunc20()
    i32 m_0c;          // 0 or this->Vfunc40() when Vfunc20()==0x1c
    i32 m_10;          // 0 or worker->m_18->Vfunc8()
    char m_name[0x80]; // +0x14  name string from the mgr
    i32 m_94;          // m_5c
    i32 m_98;          // m_60
    i32 m_9c;          // m_74
};
struct CMapStringToObLite {
    i32 Lookup(const char* key, void* out); // 0x1b8760  NAFXCW Lookup
};

// The two name->object maps Sub150c30 (the read direction) resolves through,
// each a distinct NAFXCW CMapStringTo* instantiation (different Lookup body).
// Found via the owning mgr's sub-objects at mgr+0x10 and mgr+0x28; the map sits
// 0x10 into each sub-object. Reloc-masked no-body callees.
struct MapLookupA {
    i32 Lookup(const char* key, void** out); // 0x1b8008
};
struct MapLookupB {
    i32 Lookup(const char* key, void** out); // 0x1b8438
};

// CString::operator=(LPCSTR) on the +0xdc name member (NAFXCW, reloc-masked).
struct CStringAssign {
    void Assign(const char* s); // 0x1b9e74
};

// CString stack-local destructor (NAFXCW out-of-line, reloc-masked rel32).
struct CStringDtor {
    void Dtor(); // 0x1b9cde
};

// AnimWorker extra non-virtual method (0x164830, __thiscall, 4 args).
struct AnimWorkerEx {
    i32 Method164830(i32 a1, i32 type, i32 a3, i32 a4); // 0x164830
};

// ---------------------------------------------------------------------------
// CWwdGameObject layout (raw-offset access; only offsets are load-bearing).
// ---------------------------------------------------------------------------
struct WwdRenderCtx;
class CWwdGameObject {
public:
    // Dispatch entry (0x150a70) and the methods it routes to.
    i32 Dispatch(i32 a1, i32 type, i32 a3, i32 a4);             // 0x150a70
    i32 ReadState(i32 src);                                     // 0x150b00
    i32 Setup(i32 a1, i32 a2, i32 a3, i32 a4);                  // 0x150d60 (vtbl +0x28)
    i32 Play(i32 a1, i32 type, i32 a3, i32 a4);                 // 0x151150 (vtbl +0x3c)
    i32 Serialize(i32 ar);                                      // 0x151320
    i32 WriteSnapshot(i32 dst);                                 // 0x151c00
    i32 Init(i32 a1, i32 a2, i32 a3, i32 a4);                   // 0x15b940
    i32 ResetAndSetup(i32 a1, i32 a2, i32 a3, i32 a4);          // 0x1665e0
    i32 SetupFlagged(i32 a1, i32 a2, i32 a3, i32 a4, i32 flag); // 0x15c1d0
    i32 SetupDeferred(i32 a3, i32 a4);                          // 0x15bc30
    void RenderDot(WwdRenderCtx* a);                            // 0x1660f0

    // Sibling helpers (modeled as same-class methods so ecx=this matches).
    i32 Helper164790(i32 a2, i32 a1); // 0x164790  __thiscall
    i32 Sub150c30(i32 a1);            // 0x150c30
    i32 Sub151780(i32 a1);            // 0x151780

    // The three "resolve object reference" setters Sub151780 dispatches the
    // deserialized name lookups into (sibling __thiscall methods, reloc-masked).
    i32 Resolve150eb0(void* obj); // 0x150eb0
    i32 Resolve150f90(void* obj); // 0x150f90
    i32 Resolve151070(void* obj); // 0x151070

    void* m_00;                // +0x00  vptr (self virtuals at +0x20/+0x40)
    i32 m_04;                  // +0x04
    i32 m_08;                  // +0x08  flag bits
    char m_pad0c[0x0c - 0x0c]; // (no gap)
};

// The sub-object hung off the worker at AnimWorker+0x18 (own vtable; its +0x8
// virtual is read in WriteSnapshot).
class WorkerSub {
public:
    virtual void Slot00();
    virtual void Slot04();
    virtual i32 Vfunc8(); // +0x08
};

// CWwdGameObject's own polymorphic interface (vtable @0x5f0020). Declared-only
// virtuals at the slots WriteSnapshot dispatches (+0x20, +0x40); cast `this` to
// this interface so `mov eax,[this]; call [eax+off]` falls out as __thiscall.
class WwdSelf {
public:
    virtual void Slot00();
    virtual void Slot04();
    virtual void Slot08();
    virtual void Slot0C();
    virtual void Slot10();
    virtual void Slot14();
    virtual void Slot18();
    virtual void Slot1C();
    virtual i32 Vfunc20(); // +0x20
    virtual void Slot24();
    virtual void Slot28();
    virtual void Slot2C();
    virtual void Slot30();
    virtual void Slot34();
    virtual void Slot38();
    virtual void Slot3C();
    virtual i32 Vfunc40(); // +0x40
};

// The render context RenderDot (0x1660f0) plots into: a clip extent at +0x10/
// +0x14 and the destination surface at +0x2c.
struct WwdRenderCtx {
    char m_pad00[0x10];
    i32 m_10; // +0x10  clip width
    i32 m_14; // +0x14  clip height
    char m_pad18[0x2c - 0x18];
    void* m_2c; // +0x2c  destination surface
};

// The 8-bit destination surface: GetRowBase (0x13e6d0) yields the buffer base
// offset for a row, +0x20 is the row pitch, +0xb0 the per-column stride, +0x08 a
// post-plot notifier whose vtable slot +0x80 is a free function fn(self, 0).
struct WwdSurface {
    i32 GetRowBase(i32 a); // 0x13e6d0  __thiscall -> base offset
    char m_pad00[0x08];
    void* m_08; // +0x08
    char m_pad0c[0x20 - 0x0c];
    i32 m_20; // +0x20  row pitch
    char m_pad24[0xb0 - 0x24];
    i32 m_b0; // +0xb0  per-column stride
};

// Raw this-offset helpers (documented offset access for the wide object).
#define F(p, off, ty) (*(ty*)((char*)(p) + (off)))

// ---------------------------------------------------------------------------
// Dispatch (0x150a70): look the request up in the +0x1a0 command map; on a hit,
// route by `type`: 4 -> ReadState, 7 -> Sub150c30 (abort on failure), then play.
// ---------------------------------------------------------------------------
RVA(0x00150a70, 0x89)
i32 CWwdGameObject::Dispatch(i32 a1, i32 type, i32 a3, i32 a4) {
    if (a1 == 0) {
        return 0;
    }
    if (((CmdMap*)((char*)this + 0x1a0))->Find(a1, type, a3, a4) == 0) {
        return 0;
    }
    switch (type) {
        case 4:
            if (ReadState(a1) == 0) {
                return 0;
            }
            break;
        case 7:
            if (Sub150c30(a1) == 0) {
                return 0;
            }
            break;
    }
    return Play(a1, type, a3, a4) != 0;
}

// ---------------------------------------------------------------------------
// ReadState (0x150b00): pull four fields back through the archive at the
// requested object (ebx), copy its name string, then re-emit them.
// ---------------------------------------------------------------------------
RVA(0x00150b00, 0x12b)
i32 CWwdGameObject::ReadState(i32 src) {
    Archive* ar = (Archive*)src;
    if (ar == 0) {
        return 0;
    }
    ar->Xfer((char*)this + 0x18c, 4);
    ar->Xfer((char*)this + 0x190, 4);
    i32 flag = 0;
    if (F(this, 0x198, i32) != 0) {
        flag = 1;
    }
    ar->Xfer(&flag, 4);

    char tmp[0x80];
    memset(tmp, 0, sizeof(tmp));
    if (F(this, 0x194, void*) != 0) {
        strcpy(tmp, (char*)F(this, 0x194, void*) + 0x24);
    }
    ar->Xfer(tmp, 0x80);

    memset(tmp, 0, sizeof(tmp));
    {
        EngStr str;
        ((MgrSub158570*)F(F(this, 0xc, void*), 0x28, void*))->Op(&str, F(this, 0x19c, i32));
        strcpy(tmp, str.m_data);
        ((CStringDtor*)&str)->Dtor();
    }
    ar->Xfer(tmp, 0x80);
    return 1;
}

// ---------------------------------------------------------------------------
// Sub150c30 (0x150c30): the read/load counterpart of ReadState - pull two ints
// (+0x18c/+0x190), a flag, and a name back through the archive's +0x2c slot,
// look the name up in the mgr's first map to resolve m_194; when the flag is 1
// and the lookup hit, read m_198 from the resolved object's bounded +0x14 table
// indexed by m_190. Then read a second name, look it up in the mgr's second map
// to resolve m_19c. (Dispatch case 7.)
// ---------------------------------------------------------------------------
RVA(0x00150c30, 0x130)
i32 CWwdGameObject::Sub150c30(i32 src) {
    Archive* ar = (Archive*)src;
    if (ar == 0) {
        return 0;
    }
    ar->ReadBuf((char*)this + 0x18c, 4);
    ar->ReadBuf((char*)this + 0x190, 4);
    i32 flag;
    ar->ReadBuf(&flag, 4);
    F(this, 0x194, i32) = 0;

    char name[0x100];
    ar->ReadBuf(name, 0x80);
    if (strlen(name) != 0) {
        void* found = 0;
        void* mgr = F(this, 0xc, void*);
        ((MapLookupA*)((char*)F(mgr, 0x10, void*) + 0x10))->Lookup(name, &found);
        F(this, 0x194, void*) = found;
        if (found != 0 && flag == 1) {
            i32 idx = F(this, 0x190, i32);
            if (idx >= F(found, 0x64, i32) && idx <= F(found, 0x68, i32)) {
                idx = ((i32*)F(found, 0x14, void*))[idx];
            } else {
                idx = 0;
            }
            F(this, 0x198, i32) = idx;
        }
    }

    F(this, 0x19c, i32) = 0;
    ar->ReadBuf(name, 0x80);
    if (strlen(name) != 0) {
        void* found = 0;
        void* mgr = F(this, 0xc, void*);
        ((MapLookupB*)((char*)F(mgr, 0x28, void*) + 0x10))->Lookup(name, &found);
        F(this, 0x19c, void*) = found;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// Setup (0x150d60, vtbl +0x28): wire 3 args + worker, init the wide state
// block, init the worker (vtbl +0x24), and fold its flag bits into m_08.
// ---------------------------------------------------------------------------
// @early-stop
// ~97% scheduling wall: cl hoists the m_5c/m_60 loads (for the m_ac/m_b0
// stores) to the top of the post-Init block and interleaves the three
// 0x80000000 stores into the zero-fill run; retail loads them just-in-time.
// Logic complete; the unrolled init-block store order is not steerable.
RVA(0x00150d60, 0x14d)
i32 CWwdGameObject::Setup(i32 a1, i32 a2, i32 a3, i32 a4) {
    Helper164790(a1, a2);
    F(this, 0x5c, i32) = a1;
    F(this, 0x60, i32) = a2;
    F(this, 0x74, i32) = a3;
    F(this, 0x104, i32) = a1;
    AnimWorker* w = (AnimWorker*)F(this, 0x7c, void*);
    F(this, 0x108, i32) = a2;
    F(this, 0x10c, i32) = a3;
    F(this, 0xf8, i32) = 10;
    F(this, 0xfc, i32) = 10;
    F(this, 0x118, i32) = 0;
    F(this, 0x114, i32) = 0;
    F(this, 0x128, i32) = 0;
    F(this, 0x124, i32) = 0;
    F(this, 0x11c, i32) = 0;
    F(this, 0x120, i32) = 0;
    F(this, 0x12c, i32) = 0;
    F(this, 0x130, i32) = 0;
    F(this, 0x164, i32) = 0;
    F(this, 0x168, i32) = 0;
    F(this, 0xe0, i32) = 0;
    F(this, 0x180, i32) = 0;
    if (w->Init(F((void*)a4, 0x10, i32), F((void*)a4, 0x8, i32)) == 0) {
        return 0;
    }
    F(this, 0x80, i32) = 0;
    F(this, 0x88, i32) = 0;
    F(this, 0x90, i32) = 0;
    F(this, 0x84, i32) = 0;
    F(this, 0x8c, i32) = 0;
    F(this, 0x94, i32) = 0;
    F(this, 0xe8, i32) = 0;
    F(this, 0xec, i32) = 0;
    F(this, 0xf0, i32) = 0;
    F(this, 0xf4, i32) = 0;
    F(this, 0x134, i32) = (i32)0x80000000;
    F(this, 0x144, i32) = (i32)0x80000000;
    F(this, 0x154, i32) = (i32)0x80000000;
    F(this, 0xb4, void*) = this;
    F(this, 0xac, i32) = F(this, 0x5c, i32);
    F(this, 0xb0, i32) = F(this, 0x60, i32);
    i32 wf = ((AnimWorker*)F(this, 0x7c, void*))->m_08;
    if (wf & 1) {
        F(this, 0x8, i32) |= 0x800000;
        return 1;
    }
    if (wf & 2) {
        F(this, 0x8, i32) |= 0x1000000;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// Play (0x151150, vtbl +0x3c): switch on `type` (3..8); drive the worker
// through animation states 0x50..0x53 around the inner step.
// ---------------------------------------------------------------------------
// @early-stop
// tail-merge wall (docs/patterns/identical-return-epilogue-tailmerge.md): retail
// inlines a separate play-state dance per case (3,4 distinct; 7,8 share the
// 0x15129b tail via fall-through) using different scratch regs (edx vs ecx for
// the worker) per case context; our cl cross-jumps all four dances to one shared
// tail. Logic complete; the per-case regalloc/tail-merge layout is a
// compiler-internal choice not steerable from C.
RVA(0x00151150, 0x175)
i32 CWwdGameObject::Play(i32 a1, i32 type, i32 a3, i32 a4) {
    if (a1 == 0) {
        return 0;
    }
    AnimWorker* w;
    i32 saved;
    i32 node;
    switch (type) {
        case 3: {
            F(this, 0x184, i32) = 0;
            if (F(this, 0x98, void*) != 0) {
                F(this, 0x184, i32) = F(F(this, 0x98, void*), 0x188, i32);
            }
            w = (AnimWorker*)F(this, 0x7c, void*);
            if (w == 0) {
                return 0;
            }
            saved = w->m_1c;
            w->m_1c = 0x50;
            w->Advance(this);
            w = (AnimWorker*)F(this, 0x7c, void*);
            if (w->m_1c == 0x50) {
                w->m_1c = saved;
            }
            break;
        }
        case 4: {
            if (Serialize(a1) == 0) {
                return 0;
            }
            w = (AnimWorker*)F(this, 0x7c, void*);
            if (w == 0) {
                return 0;
            }
            saved = w->m_1c;
            w->m_1c = 0x51;
            w->Advance(this);
            w = (AnimWorker*)F(this, 0x7c, void*);
            if (w->m_1c == 0x51) {
                w->m_1c = saved;
            }
            break;
        }
        case 7: {
            if (Sub151780(a1) == 0) {
                return 0;
            }
            w = (AnimWorker*)F(this, 0x7c, void*);
            if (w == 0) {
                return 0;
            }
            saved = w->m_1c;
            w->m_1c = 0x52;
            w->Advance(this);
            w = (AnimWorker*)F(this, 0x7c, void*);
            if (w->m_1c == 0x52) {
                w->m_1c = saved;
            }
            break;
        }
        case 8: {
            node = F(this, 0x184, i32);
            if (node != 0) {
                void* found = 0;
                CMapStringToObLite* map =
                    (CMapStringToObLite*)((char*)F(F(this, 0xc, void*), 0x8, void*) + 0x48);
                if (map->Lookup((const char*)node, &found) == 0) {
                    F(this, 0x98, void*) = 0;
                } else {
                    F(this, 0x98, void*) = found;
                }
            } else {
                F(this, 0x98, i32) = 0;
            }
            w = (AnimWorker*)F(this, 0x7c, void*);
            if (w == 0) {
                return 0;
            }
            saved = w->m_1c;
            w->m_1c = 0x53;
            w->Advance(this);
            w = (AnimWorker*)F(this, 0x7c, void*);
            if (w->m_1c == 0x53) {
                w->m_1c = saved;
            }
            break;
        }
    }
    return ((AnimWorkerEx*)F(this, 0x7c, void*))->Method164830(a1, type, a3, a4) != 0;
}

// ---------------------------------------------------------------------------
// Serialize (0x151320): read a 0x24 block + a name string, then ~13 dwords.
// ---------------------------------------------------------------------------
RVA(0x00151320, 0x454)
i32 CWwdGameObject::Serialize(i32 arParam) {
    Archive* ar = (Archive*)arParam;
    if (ar == 0) {
        return 0;
    }

    ar->Xfer((char*)this + 0xb8, 0x24);

    char tmp[0x80];
    memset(tmp, 0, sizeof(tmp));
    strcpy(tmp, (char*)F(this, 0xdc, void*));
    ar->Xfer(tmp, 0x80);

    ar->Xfer((char*)this + 0xe4, 4);
    ar->Xfer((char*)this + 0xe8, 4);
    ar->Xfer((char*)this + 0xec, 4);
    ar->Xfer((char*)this + 0xf0, 4);
    ar->Xfer((char*)this + 0xf4, 4);
    ar->Xfer((char*)this + 0xf8, 4);
    ar->Xfer((char*)this + 0xfc, 4);
    ar->Xfer((char*)this + 0x100, 4);
    ar->Xfer((char*)this + 0x104, 4);
    ar->Xfer((char*)this + 0x108, 4);
    ar->Xfer((char*)this + 0x10c, 4);
    ar->Xfer((char*)this + 0x110, 4);
    ar->Xfer((char*)this + 0x114, 4);
    ar->Xfer((char*)this + 0x118, 4);
    ar->Xfer((char*)this + 0x11c, 4);
    ar->Xfer((char*)this + 0x120, 4);
    ar->Xfer((char*)this + 0x124, 4);
    ar->Xfer((char*)this + 0x128, 4);
    ar->Xfer((char*)this + 0x12c, 4);
    ar->Xfer((char*)this + 0x130, 4);
    ar->Xfer((char*)this + 0x134, 0x10);
    ar->Xfer((char*)this + 0x144, 0x10);
    ar->Xfer((char*)this + 0x154, 0x10);
    ar->Xfer((char*)this + 0x164, 4);
    ar->Xfer((char*)this + 0x168, 4);
    ar->Xfer((char*)this + 0x16c, 4);
    ar->Xfer((char*)this + 0x170, 4);
    ar->Xfer((char*)this + 0x174, 4);
    ar->Xfer((char*)this + 0x178, 4);
    ar->Xfer((char*)this + 0x17c, 4);
    ar->Xfer((char*)this + 0x180, 4);
    ar->Xfer((char*)this + 0x10, 4);
    ar->Xfer((char*)this + 0x14, 4);
    ar->Xfer((char*)this + 0x18, 0x24);
    ar->Xfer((char*)this + 0x40, 4);
    ar->Xfer((char*)this + 0x44, 4);
    ar->Xfer((char*)this + 0x48, 4);
    ar->Xfer((char*)this + 0x50, 4);
    ar->Xfer((char*)this + 0x54, 4);
    ar->Xfer((char*)this + 0x58, 4);
    ar->Xfer((char*)this + 0x64, 0x10);
    ar->Xfer((char*)this + 0x4, 4);
    ar->Xfer((char*)this + 0x8, 4);
    ar->Xfer((char*)this + 0x184, 4);

    memset(tmp, 0, sizeof(tmp));
    if (F(this, 0x80, void*) != 0) {
        EngStr str;
        ((MgrSub165360*)F(F(this, 0xc, void*), 0x14, void*))->Build(&str, F(this, 0x80, void*));
        strcpy(tmp, str.m_data);
        ((CStringDtor*)&str)->Dtor();
    }
    ar->Xfer(tmp, 0x80);

    memset(tmp, 0, sizeof(tmp));
    if (F(this, 0x88, void*) != 0) {
        EngStr str;
        ((MgrSub165360*)F(F(this, 0xc, void*), 0x14, void*))->Build(&str, F(this, 0x88, void*));
        strcpy(tmp, str.m_data);
        ((CStringDtor*)&str)->Dtor();
    }
    ar->Xfer(tmp, 0x80);

    memset(tmp, 0, sizeof(tmp));
    if (F(this, 0x90, void*) != 0) {
        EngStr str;
        ((MgrSub165360*)F(F(this, 0xc, void*), 0x14, void*))->Build(&str, F(this, 0x90, void*));
        strcpy(tmp, str.m_data);
        ((CStringDtor*)&str)->Dtor();
    }
    ar->Xfer(tmp, 0x80);
    return 1;
}

// ---------------------------------------------------------------------------
// Sub151780 (0x151780): the read/load mirror of Serialize - pull the same field
// block back through the archive's +0x2c read slot (assigning the +0xdc name
// CString), then resolve three object references by reading a name, looking it
// up in the mgr's registry map, and handing the hit to the matching setter.
// (Dispatch/Play case 7.) Same offset/size sweep as Serialize, reversed.
// ---------------------------------------------------------------------------
RVA(0x00151780, 0x40d)
i32 CWwdGameObject::Sub151780(i32 arParam) {
    Archive* ar = (Archive*)arParam;
    if (ar == 0) {
        return 0;
    }

    ar->ReadBuf((char*)this + 0xb8, 0x24);

    char name[0x80];
    ar->ReadBuf(name, 0x80);
    ((CStringAssign*)((char*)this + 0xdc))->Assign(name);

    ar->ReadBuf((char*)this + 0xe4, 4);
    ar->ReadBuf((char*)this + 0xe8, 4);
    ar->ReadBuf((char*)this + 0xec, 4);
    ar->ReadBuf((char*)this + 0xf0, 4);
    ar->ReadBuf((char*)this + 0xf4, 4);
    ar->ReadBuf((char*)this + 0xf8, 4);
    ar->ReadBuf((char*)this + 0xfc, 4);
    ar->ReadBuf((char*)this + 0x100, 4);
    ar->ReadBuf((char*)this + 0x104, 4);
    ar->ReadBuf((char*)this + 0x108, 4);
    ar->ReadBuf((char*)this + 0x10c, 4);
    ar->ReadBuf((char*)this + 0x110, 4);
    ar->ReadBuf((char*)this + 0x114, 4);
    ar->ReadBuf((char*)this + 0x118, 4);
    ar->ReadBuf((char*)this + 0x11c, 4);
    ar->ReadBuf((char*)this + 0x120, 4);
    ar->ReadBuf((char*)this + 0x124, 4);
    ar->ReadBuf((char*)this + 0x128, 4);
    ar->ReadBuf((char*)this + 0x12c, 4);
    ar->ReadBuf((char*)this + 0x130, 4);
    ar->ReadBuf((char*)this + 0x134, 0x10);
    ar->ReadBuf((char*)this + 0x144, 0x10);
    ar->ReadBuf((char*)this + 0x154, 0x10);
    ar->ReadBuf((char*)this + 0x164, 4);
    ar->ReadBuf((char*)this + 0x168, 4);
    ar->ReadBuf((char*)this + 0x16c, 4);
    ar->ReadBuf((char*)this + 0x170, 4);
    ar->ReadBuf((char*)this + 0x174, 4);
    ar->ReadBuf((char*)this + 0x178, 4);
    ar->ReadBuf((char*)this + 0x17c, 4);
    ar->ReadBuf((char*)this + 0x180, 4);
    ar->ReadBuf((char*)this + 0x10, 4);
    ar->ReadBuf((char*)this + 0x14, 4);
    ar->ReadBuf((char*)this + 0x18, 0x24);
    ar->ReadBuf((char*)this + 0x40, 4);
    ar->ReadBuf((char*)this + 0x44, 4);
    ar->ReadBuf((char*)this + 0x48, 4);
    ar->ReadBuf((char*)this + 0x50, 4);
    ar->ReadBuf((char*)this + 0x54, 4);
    ar->ReadBuf((char*)this + 0x58, 4);
    ar->ReadBuf((char*)this + 0x64, 0x10);
    ar->ReadBuf((char*)this + 0x4, 4);
    ar->ReadBuf((char*)this + 0x8, 4);
    ar->ReadBuf((char*)this + 0x184, 4);

    ar->ReadBuf(name, 0x80);
    if (strlen(name) != 0) {
        void* found = 0;
        ((MapLookupA*)((char*)F(F(this, 0xc, void*), 0x14, void*) + 0x10))->Lookup(name, &found);
        if (Resolve150eb0(found) == 0) {
            return 0;
        }
    }

    ar->ReadBuf(name, 0x80);
    if (strlen(name) != 0) {
        void* found = 0;
        ((MapLookupA*)((char*)F(F(this, 0xc, void*), 0x14, void*) + 0x10))->Lookup(name, &found);
        if (Resolve150f90(found) == 0) {
            return 0;
        }
    }

    ar->ReadBuf(name, 0x80);
    if (strlen(name) != 0) {
        void* found = 0;
        ((MapLookupA*)((char*)F(F(this, 0xc, void*), 0x14, void*) + 0x10))->Lookup(name, &found);
        if (Resolve151070(found) == 0) {
            return 0;
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// WriteSnapshot (0x151c00): assemble a 0xa0-byte record from this + the worker
// and emit it through the archive at +0x30.
// ---------------------------------------------------------------------------
// @early-stop
// ~96% reloc/scheduling plateau: the two externals (Build/Dtor) reloc-mask
// against differently-named symbols (entropy tail) and a couple of record-field
// stores schedule one slot off retail. Logic complete; not steerable.
RVA(0x00151c00, 0x118)
i32 CWwdGameObject::WriteSnapshot(i32 dst) {
    Archive* ar = (Archive*)dst;
    if (ar == 0) {
        return 0;
    }
    AnimWorker* w = (AnimWorker*)F(this, 0x7c, void*);
    if (w == 0) {
        return 0;
    }
    if (w->m_1c == 0) {
        w->Advance(this);
    }

    WwdSelf* self = (WwdSelf*)this;
    i32 ebx = 0;
    if (self->Vfunc20() == 0x1c) {
        ebx = self->Vfunc40();
    }

    w = (AnimWorker*)F(this, 0x7c, void*);
    i32 edi = 0;
    if (w->m_18 != 0) {
        edi = ((WorkerSub*)w->m_18)->Vfunc8();
    }

    WwdSnapshot rec;
    rec.m_00 = F(this, 0x4, i32);
    rec.m_08 = self->Vfunc20();
    rec.m_04 = F(this, 0x188, i32);
    rec.m_94 = F(this, 0x5c, i32);
    rec.m_98 = F(this, 0x60, i32);
    rec.m_9c = F(this, 0x74, i32);
    rec.m_0c = ebx;
    rec.m_10 = edi;

    {
        EngStr str;
        ((MgrSub165360*)F(F(this, 0xc, void*), 0x14, void*))->Build(&str, F(this, 0x7c, void*));
        strcpy(rec.m_name, str.m_data);
        ((CStringDtor*)&str)->Dtor();
    }
    ar->Xfer(&rec, 0xa0);
    return 1;
}

// ---------------------------------------------------------------------------
// Init (0x15b940): zero +0x19c, construct the +0x1a0 command map, then Setup.
// ---------------------------------------------------------------------------
RVA(0x0015b940, 0x38)
i32 CWwdGameObject::Init(i32 a1, i32 a2, i32 a3, i32 a4) {
    F(this, 0x19c, i32) = 0;
    ((CmdMap*)((char*)this + 0x1a0))->Construct(this);
    return Setup(a1, a2, a3, a4);
}

// The +0x1dc CObList of owned sub-objects (CObject base vtbl@+0, head@+4) and its
// list nodes {next@0, prev@4, data@8}; RemoveAll (0x1b5a0b) frees the node cells.
struct WwdSubDel {
    virtual void Slot00();
    virtual void DeleteSelf(i32 flag); // +0x04  scalar deleting dtor
};
struct WwdSubNode {
    WwdSubNode* m_next; // +0x00
    WwdSubNode* m_prev; // +0x04
    WwdSubDel* m_data;  // +0x08  owned polymorphic payload
};
struct WwdSubList {
    void RemoveAll_1b5a0b(); // 0x1b5a0b  CObList::RemoveAll (reloc-masked)
    void* m_vtbl;            // +0x00
    WwdSubNode* m_head;      // +0x04
};

// ---------------------------------------------------------------------------
// ResetAndSetup (0x1665e0): delete every owned sub-object in the +0x1dc CObList
// (its payload at node+8, via the deleting dtor), empty the list, then re-run
// Setup with the four forwarded args. Returns Setup() != 0.
//
// @early-stop
// shrink-wrapped-callee-save-push wall (~80%, inverted): retail pushes esi+edi
// together upfront; cl pushes edi upfront and sinks `push esi` into the
// list-walk loop (esi only live there). Loop body + delete + Setup-forward
// byte-equivalent; the push placement + arg-reload offsets cascade, not
// source-steerable (docs/patterns/shrink-wrapped-callee-save-push.md).
// ---------------------------------------------------------------------------
RVA(0x001665e0, 0x55)
i32 CWwdGameObject::ResetAndSetup(i32 a1, i32 a2, i32 a3, i32 a4) {
    WwdSubNode* n = F(this, 0x1e0, WwdSubNode*);
    while (n != 0) {
        WwdSubNode* next = n->m_next;
        WwdSubDel* p = n->m_data;
        if (p != 0) {
            p->DeleteSelf(1);
        }
        n = next;
    }
    ((WwdSubList*)((char*)this + 0x1dc))->RemoveAll_1b5a0b();
    return Setup(a1, a2, a3, a4) != 0;
}

// ---------------------------------------------------------------------------
// SetupFlagged (0x15c1d0): stash the byte flag at +0x18c, then forward the four
// args to Setup.  __thiscall, 5 stack args (ret 0x14).
// ---------------------------------------------------------------------------
RVA(0x0015c1d0, 0x26)
i32 CWwdGameObject::SetupFlagged(i32 a1, i32 a2, i32 a3, i32 a4, i32 flag) {
    F(this, 0x18c, char) = (char)flag;
    return Setup(a1, a2, a3, a4);
}

// ---------------------------------------------------------------------------
// SetupDeferred (0x15bc30): forward (0, 0, a3, a4) to Setup.  __thiscall, 2
// stack args (ret 0x8).
// ---------------------------------------------------------------------------
RVA(0x0015bc30, 0x16)
i32 CWwdGameObject::SetupDeferred(i32 a3, i32 a4) {
    return Setup(0, 0, a3, a4);
}

// ---------------------------------------------------------------------------
// RenderDot (0x1660f0): plot the object's (+0x5c,+0x60) position as a single
// 8-bit pixel into the render context's surface, after a bounds check (either
// against the context clip extent when +0x64 is unbounded (0x80000000) or
// against the object's own +0x64..+0x70 clip rect). On a successful plot, cache
// the position to +0x18/+0x1c, mark +0x30/+0x34 dirty and +0x38 = 0; on a clip
// reject, +0x38 = -1.  __thiscall, 1 stack arg (ret 4), no EH frame.
//
// @early-stop
// regalloc-coloring wall (~57%): logic byte-equivalent, but cl swaps x/y across
// the lone free callee-saved pair (x->ebp,y->ebx vs retail x->ebx,y->ebp) so
// every x/y modrm differs, and the 8-bit color either pins bl (forcing an x
// spill + `push ecx`, 47%) or reads inline (dropping retail's early-load+stack-
// spill of color, shrinking the body). No source spelling reproduces retail's
// "x in ebx + color spilled" layout. See const-materialize-into-reg-vs-immediate.
// ---------------------------------------------------------------------------
RVA(0x001660f0, 0xd1)
void CWwdGameObject::RenderDot(WwdRenderCtx* a) {
    i32 x = F(this, 0x5c, i32);
    i32 m64 = F(this, 0x64, i32);
    i32 y;
    if (m64 == (i32)0x80000000) {
        if (x < 0) {
            goto reject;
        }
        y = F(this, 0x60, i32);
        if (y < 0) {
            goto reject;
        }
        if (x >= a->m_10) {
            goto reject;
        }
        if (y >= a->m_14) {
            goto reject;
        }
    } else {
        if (x < m64) {
            goto reject;
        }
        y = F(this, 0x60, i32);
        if (y < F(this, 0x68, i32)) {
            goto reject;
        }
        if (x > F(this, 0x6c, i32)) {
            goto reject;
        }
        if (y > F(this, 0x70, i32)) {
            goto reject;
        }
    }

    {
        WwdSurface* surf = (WwdSurface*)a->m_2c;
        i32 base = surf->GetRowBase(0);
        if (base != 0) {
            i32 row = surf->m_20 * y;
            i32 col = surf->m_b0 * x;
            *(char*)(base + row + col) = F(this, 0x18c, char);
            void* n = surf->m_08;
            (*(void (**)(void*, i32))((char*)*(void**)n + 0x80))(n, 0);
        }
    }
    F(this, 0x18, i32) = F(this, 0x5c, i32);
    F(this, 0x1c, i32) = F(this, 0x60, i32);
    F(this, 0x30, i32) = 1;
    F(this, 0x34, i32) = 1;
    F(this, 0x38, i32) = 0;
    return;
reject:
    F(this, 0x38, i32) = -1;
}
