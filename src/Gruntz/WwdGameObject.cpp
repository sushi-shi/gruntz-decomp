// WwdGameObject.cpp - leaf methods of CWwdGameObject, a runtime "plane object"
// deserialized from WWD level data (constructed by WwdFile::ReadPlaneObjects,
// 0x162af0, via the EH-frame ctor at 0x15b390 which is NOT reconstructed here).
//
// The object owns a sprite-animation worker at +0x7c (AnimWorker, foreign
// vtable - virtuals dispatched, never defined), a command-dispatch sub-object
// at +0x1a0 (CmdMap), and a back-pointer to its owning manager at +0x0c.
//
// Fields are typed named members of CWwdGameObject at their retail offsets
// (matching-neutral: a named member at offset N lowers to the same [this+N] as
// the raw this-offset read it replaced). Provable roles are named (pos/clip/
// worker/mgr/name/flags/command-map/sub-list); the opaque serialized state block
// keeps m_<hexoffset> placeholders. Only the OFFSETS + emitted bytes are
// load-bearing (campaign doctrine).
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
    char m_body[0x3c];                        // embedded sub-object body (+0x1a0..+0x1dc)
};

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

// The owning manager at +0x0c, modeled as a real typed object (WwdMgr, below)
// with its nested reader/map sub-objects, so the chained derefs lower to the
// exact [[mgr+slot]+off] loads with no cast.
//
// The name->object lookup maps each reader sub-object embeds at +0x10 (each a
// distinct NAFXCW CMapStringTo* instantiation -> distinct Lookup body) and the
// kill-cue map at +0x48. Reloc-masked no-body callees.
struct MapLookupA {
    i32 Lookup(const char* key, void** out); // 0x1b8008
};
struct MapLookupB {
    i32 Lookup(const char* key, void** out); // 0x1b8438
};
struct CMapStringToObLite {
    i32 Lookup(const char* key, void* out); // 0x1b8760  NAFXCW Lookup
};

// The engine CString value returned by-value from Op/Build: one pointer to the
// heap char data (data starts at the pointed-to address). Modeled as a POD (no
// C++ ctor/dtor) so cl emits none implicitly and the teardown is the EXPLICIT
// reloc-masked engine ~CString call (Dtor, 0x1b9cde) - a real CString local's
// implicit dtor would perturb the codegen this reconstruction pins.
// authentic: POD CString-value view; Dtor is the modeled engine ~CString extern.
struct EngStr {
    const char* m_data; // +0x00  -> the char buffer
    void Dtor();        // 0x1b9cde  NAFXCW ~CString (reloc-masked, __thiscall)
};

// mgr+0x28 reader: a name-resolver (Op -> CString) with its lookup map at +0x10.
struct MgrSub158570 {
    EngStr* Op(EngStr* out, i32 a); // 0x158570  __thiscall, returns out (CString)
    char m_pad00[0x10];
    MapLookupB m_map; // +0x10  name -> object (0x1b8438)
};

// The archive/stream passed to ReadState/Serialize/Sub150c30/Sub151780/
// WriteSnapshot. Its op is the virtual at +0x2c/+0x30: a fixed-size element
// transfer (read or write `size` bytes).
// authentic: the retail mangling of those methods carries an `int` param (`H`,
// e.g. ?ReadState@CWwdGameObject@@QAEHH@Z) - the archive enters as an int handle,
// so the source param stays `int` (dev-faithful) and the `(Archive*)` reinterpret
// of that handle is the real operation. Retyping the param would rewrite the
// symbol and diverge from what the devs wrote.
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
// mgr+0x14 reader: a name-builder (Build -> CString) with its lookup map at +0x10.
struct MgrSub165360 {
    EngStr* Build(EngStr* out, void* obj); // 0x165360  __thiscall -> CString
    char m_pad00[0x10];
    MapLookupA m_map; // +0x10  name -> object (0x1b8008)
};

// mgr+0x08 sub-object: holds the per-frame kill-cue name map at +0x48.
struct WwdMgrSub08 {
    char m_pad00[0x48];
    CMapStringToObLite m_map; // +0x48  name -> object (0x1b8760)
};
// mgr+0x10 sub-object: a name->object resolver (lookup map at +0x10).
struct WwdMgrSub10 {
    char m_pad00[0x10];
    MapLookupA m_map; // +0x10  (0x1b8008)
};
// CWwdGameObject+0x0c owning manager: four typed reader/map sub-objects.
struct WwdMgr {
    char m_pad00[0x08];
    WwdMgrSub08* m_08; // +0x08  kill-cue map holder
    char m_pad0c[0x10 - 0x0c];
    WwdMgrSub10* m_10;  // +0x10  name resolver
    MgrSub165360* m_14; // +0x14  name builder + resolver map
    char m_pad18[0x28 - 0x18];
    MgrSub158570* m_28; // +0x28  name resolver (Op) + secondary map
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

// CString::operator=(LPCSTR) on the +0xdc name member (NAFXCW, reloc-masked).
// authentic: m_name is the engine's bare CString handle (one `char*`); its
// operator= is an out-of-line extern, so it is modeled as a method on a tiny
// helper the &m_name handle is reinterpreted through (no member to fold into).
struct CStringAssign {
    void Assign(const char* s); // 0x1b9e74
};

// ---------------------------------------------------------------------------
// CWwdGameObject layout (raw-offset access; only offsets are load-bearing).
// ---------------------------------------------------------------------------
struct WwdRenderCtx;
struct WwdSurface; // defined below; m_2c holds the destination surface
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

    void* m_00;       // +0x00  vptr (self virtuals dispatched via WwdSelf cast)
    i32 m_04;         // +0x04
    i32 m_flags;      // +0x08  bit flags (|=0x800000 / 0x1000000)
    WwdMgr* m_mgr;    // +0x0c  owning manager
    i32 m_10;         // +0x10
    i32 m_14;         // +0x14
    i32 m_lastX;      // +0x18  last-drawn column (cached by RenderDot)
    i32 m_lastY;      // +0x1c  last-drawn row
    i32 m_20;         // +0x20
    i32 m_24;         // +0x24
    i32 m_28;         // +0x28
    i32 m_2c;         // +0x2c
    i32 m_30;         // +0x30  set 1 on a successful plot
    i32 m_34;         // +0x34  set 1 on a successful plot
    i32 m_clipResult; // +0x38  clip result (0 plotted / -1 rejected)
    char m_pad3c[0x40 - 0x3c];
    i32 m_40; // +0x40
    i32 m_44; // +0x44
    i32 m_48; // +0x48
    char m_pad4c[0x50 - 0x4c];
    i32 m_50;         // +0x50
    i32 m_54;         // +0x54
    i32 m_58;         // +0x58
    i32 m_posX;       // +0x5c  position column
    i32 m_posY;       // +0x60  position row
    i32 m_clipLeft;   // +0x64  clip rect (0x80000000 = unbounded)
    i32 m_clipTop;    // +0x68
    i32 m_clipRight;  // +0x6c
    i32 m_clipBottom; // +0x70
    i32 m_74;         // +0x74
    char m_pad78[0x7c - 0x78];
    AnimWorker* m_worker; // +0x7c  sprite-animation worker
    void* m_80;           // +0x80  object ref (serialized by name)
    i32 m_84;             // +0x84
    void* m_88;           // +0x88  object ref
    i32 m_8c;             // +0x8c
    void* m_90;           // +0x90  object ref
    i32 m_94;             // +0x94
    void* m_98;           // +0x98  linked object (reads its +0x188)
    char m_pad9c[0xac - 0x9c];
    i32 m_ac;               // +0xac  copy of m_posX
    i32 m_b0;               // +0xb0  copy of m_posY
    CWwdGameObject* m_self; // +0xb4  = this
    char m_b8[0x24];        // +0xb8  serialized state block
    char* m_name;           // +0xdc  CString name (handle = buffer pointer)
    i32 m_e0;               // +0xe0
    i32 m_e4;               // +0xe4
    i32 m_e8;               // +0xe8
    i32 m_ec;               // +0xec
    i32 m_f0;               // +0xf0
    i32 m_f4;               // +0xf4
    i32 m_f8;               // +0xf8
    i32 m_fc;               // +0xfc
    i32 m_100;              // +0x100
    i32 m_104;              // +0x104
    i32 m_108;              // +0x108
    i32 m_10c;              // +0x10c
    i32 m_110;              // +0x110
    i32 m_114;              // +0x114
    i32 m_118;              // +0x118
    i32 m_11c;              // +0x11c
    i32 m_120;              // +0x120
    i32 m_124;              // +0x124
    i32 m_128;              // +0x128
    i32 m_12c;              // +0x12c
    i32 m_130;              // +0x130
    i32 m_134;              // +0x134  block head (0x80000000 sentinel)
    i32 m_138;              // +0x138
    i32 m_13c;              // +0x13c
    i32 m_140;              // +0x140
    i32 m_144;              // +0x144  block head (0x80000000 sentinel)
    i32 m_148;              // +0x148
    i32 m_14c;              // +0x14c
    i32 m_150;              // +0x150
    i32 m_154;              // +0x154  block head (0x80000000 sentinel)
    i32 m_158;              // +0x158
    i32 m_15c;              // +0x15c
    i32 m_160;              // +0x160
    i32 m_164;              // +0x164
    i32 m_168;              // +0x168
    i32 m_16c;              // +0x16c
    i32 m_170;              // +0x170
    i32 m_174;              // +0x174
    i32 m_178;              // +0x178
    i32 m_17c;              // +0x17c
    i32 m_180;              // +0x180
    i32 m_184;              // +0x184
    i32 m_188;              // +0x188
    i32 m_dotColor;         // +0x18c  low byte = dot color / setup flag
    i32 m_190;              // +0x190
    void* m_194;            // +0x194  resolved object ref
    i32 m_198;              // +0x198
    void* m_19c;            // +0x19c  resolved object ref
    CmdMap m_cmdMap;        // +0x1a0  command-dispatch sub-object
    WwdSubList m_subList;   // +0x1dc  CObList of owned sub-objects
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
// authentic: CWwdGameObject cannot be made polymorphic in-TU (its full retail
// vtable is unmatched engine code; declaring real virtuals would make cl emit a
// wrong ??_7). This declared-only interface is the recommended manual-dispatch
// form until the class's whole vtable is modeled.
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
    WwdSurface* m_2c; // +0x2c  destination surface
};

// The 8-bit destination surface: GetRowBase (0x13e6d0) yields the buffer base
// offset for a row, +0x20 is the row pitch, +0xb0 the per-column stride, +0x08 a
// post-plot notifier whose vtable slot +0x80 is a free function fn(self, 0).
struct WwdSurface {
    i32 GetRowBase(i32 a); // 0x13e6d0  __thiscall -> base offset
    char m_pad00[0x08];
    // authentic: +0x08 is a foreign engine notifier whose vtable slot +0x80 holds
    // a FREE function fn(self, 0) (not a __thiscall method) - a manual vtable read
    // through an untyped slot is the only faithful form; kept void*.
    void* m_08; // +0x08
    char m_pad0c[0x20 - 0x0c];
    i32 m_20; // +0x20  row pitch
    char m_pad24[0xb0 - 0x24];
    i32 m_b0; // +0xb0  per-column stride
};

// Raw this-offset read of a foreign engine object reached as an opaque void*/int
// handle (found-object refs, the a4 setup source). authentic: these referents are
// heterogeneous unmodeled engine objects (no single concrete class to type), so a
// documented offset read is the deliberate access - only the offset is load-bearing.
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
    if (m_cmdMap.Find(a1, type, a3, a4) == 0) {
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
    ar->Xfer(&m_dotColor, 4);
    ar->Xfer(&m_190, 4);
    i32 flag = 0;
    if (m_198 != 0) {
        flag = 1;
    }
    ar->Xfer(&flag, 4);

    char tmp[0x80];
    memset(tmp, 0, sizeof(tmp));
    if (m_194 != 0) {
        strcpy(tmp, (char*)m_194 + 0x24);
    }
    ar->Xfer(tmp, 0x80);

    memset(tmp, 0, sizeof(tmp));
    {
        EngStr str;
        m_mgr->m_28->Op(&str, (i32)m_19c);
        strcpy(tmp, str.m_data);
        str.Dtor();
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
    ar->ReadBuf(&m_dotColor, 4);
    ar->ReadBuf(&m_190, 4);
    i32 flag;
    ar->ReadBuf(&flag, 4);
    m_194 = 0;

    char name[0x100];
    ar->ReadBuf(name, 0x80);
    if (strlen(name) != 0) {
        void* found = 0;
        WwdMgr* mgr = m_mgr;
        mgr->m_10->m_map.Lookup(name, &found);
        m_194 = found;
        if (found != 0 && flag == 1) {
            i32 idx = m_190;
            if (idx >= F(found, 0x64, i32) && idx <= F(found, 0x68, i32)) {
                idx = ((i32*)F(found, 0x14, void*))[idx];
            } else {
                idx = 0;
            }
            m_198 = idx;
        }
    }

    m_19c = 0;
    ar->ReadBuf(name, 0x80);
    if (strlen(name) != 0) {
        void* found = 0;
        WwdMgr* mgr = m_mgr;
        mgr->m_28->m_map.Lookup(name, &found);
        m_19c = found;
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
    m_posX = a1;
    m_posY = a2;
    m_74 = a3;
    m_104 = a1;
    AnimWorker* w = m_worker;
    m_108 = a2;
    m_10c = a3;
    m_f8 = 10;
    m_fc = 10;
    m_118 = 0;
    m_114 = 0;
    m_128 = 0;
    m_124 = 0;
    m_11c = 0;
    m_120 = 0;
    m_12c = 0;
    m_130 = 0;
    m_164 = 0;
    m_168 = 0;
    m_e0 = 0;
    m_180 = 0;
    if (w->Init(F((void*)a4, 0x10, i32), F((void*)a4, 0x8, i32)) == 0) {
        return 0;
    }
    m_80 = 0;
    m_88 = 0;
    m_90 = 0;
    m_84 = 0;
    m_8c = 0;
    m_94 = 0;
    m_e8 = 0;
    m_ec = 0;
    m_f0 = 0;
    m_f4 = 0;
    m_134 = (i32)0x80000000;
    m_144 = (i32)0x80000000;
    m_154 = (i32)0x80000000;
    m_self = this;
    m_ac = m_posX;
    m_b0 = m_posY;
    i32 wf = m_worker->m_08;
    if (wf & 1) {
        m_flags |= 0x800000;
        return 1;
    }
    if (wf & 2) {
        m_flags |= 0x1000000;
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
            m_184 = 0;
            if (m_98 != 0) {
                m_184 = F(m_98, 0x188, i32);
            }
            w = m_worker;
            if (w == 0) {
                return 0;
            }
            saved = w->m_1c;
            w->m_1c = 0x50;
            w->Advance(this);
            w = m_worker;
            if (w->m_1c == 0x50) {
                w->m_1c = saved;
            }
            break;
        }
        case 4: {
            if (Serialize(a1) == 0) {
                return 0;
            }
            w = m_worker;
            if (w == 0) {
                return 0;
            }
            saved = w->m_1c;
            w->m_1c = 0x51;
            w->Advance(this);
            w = m_worker;
            if (w->m_1c == 0x51) {
                w->m_1c = saved;
            }
            break;
        }
        case 7: {
            if (Sub151780(a1) == 0) {
                return 0;
            }
            w = m_worker;
            if (w == 0) {
                return 0;
            }
            saved = w->m_1c;
            w->m_1c = 0x52;
            w->Advance(this);
            w = m_worker;
            if (w->m_1c == 0x52) {
                w->m_1c = saved;
            }
            break;
        }
        case 8: {
            node = m_184;
            if (node != 0) {
                void* found = 0;
                CMapStringToObLite* map = &m_mgr->m_08->m_map;
                if (map->Lookup((const char*)node, &found) == 0) {
                    m_98 = 0;
                } else {
                    m_98 = found;
                }
            } else {
                m_98 = 0;
            }
            w = m_worker;
            if (w == 0) {
                return 0;
            }
            saved = w->m_1c;
            w->m_1c = 0x53;
            w->Advance(this);
            w = m_worker;
            if (w->m_1c == 0x53) {
                w->m_1c = saved;
            }
            break;
        }
    }
    return m_worker->Method164830(a1, type, a3, a4) != 0;
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

    ar->Xfer(m_b8, 0x24);

    char tmp[0x80];
    memset(tmp, 0, sizeof(tmp));
    strcpy(tmp, m_name);
    ar->Xfer(tmp, 0x80);

    ar->Xfer(&m_e4, 4);
    ar->Xfer(&m_e8, 4);
    ar->Xfer(&m_ec, 4);
    ar->Xfer(&m_f0, 4);
    ar->Xfer(&m_f4, 4);
    ar->Xfer(&m_f8, 4);
    ar->Xfer(&m_fc, 4);
    ar->Xfer(&m_100, 4);
    ar->Xfer(&m_104, 4);
    ar->Xfer(&m_108, 4);
    ar->Xfer(&m_10c, 4);
    ar->Xfer(&m_110, 4);
    ar->Xfer(&m_114, 4);
    ar->Xfer(&m_118, 4);
    ar->Xfer(&m_11c, 4);
    ar->Xfer(&m_120, 4);
    ar->Xfer(&m_124, 4);
    ar->Xfer(&m_128, 4);
    ar->Xfer(&m_12c, 4);
    ar->Xfer(&m_130, 4);
    ar->Xfer(&m_134, 0x10);
    ar->Xfer(&m_144, 0x10);
    ar->Xfer(&m_154, 0x10);
    ar->Xfer(&m_164, 4);
    ar->Xfer(&m_168, 4);
    ar->Xfer(&m_16c, 4);
    ar->Xfer(&m_170, 4);
    ar->Xfer(&m_174, 4);
    ar->Xfer(&m_178, 4);
    ar->Xfer(&m_17c, 4);
    ar->Xfer(&m_180, 4);
    ar->Xfer(&m_10, 4);
    ar->Xfer(&m_14, 4);
    ar->Xfer(&m_lastX, 0x24); // +0x18 render-state block
    ar->Xfer(&m_40, 4);
    ar->Xfer(&m_44, 4);
    ar->Xfer(&m_48, 4);
    ar->Xfer(&m_50, 4);
    ar->Xfer(&m_54, 4);
    ar->Xfer(&m_58, 4);
    ar->Xfer(&m_clipLeft, 0x10); // +0x64 clip rect
    ar->Xfer(&m_04, 4);
    ar->Xfer(&m_flags, 4);
    ar->Xfer(&m_184, 4);

    memset(tmp, 0, sizeof(tmp));
    if (m_80 != 0) {
        EngStr str;
        m_mgr->m_14->Build(&str, m_80);
        strcpy(tmp, str.m_data);
        str.Dtor();
    }
    ar->Xfer(tmp, 0x80);

    memset(tmp, 0, sizeof(tmp));
    if (m_88 != 0) {
        EngStr str;
        m_mgr->m_14->Build(&str, m_88);
        strcpy(tmp, str.m_data);
        str.Dtor();
    }
    ar->Xfer(tmp, 0x80);

    memset(tmp, 0, sizeof(tmp));
    if (m_90 != 0) {
        EngStr str;
        m_mgr->m_14->Build(&str, m_90);
        strcpy(tmp, str.m_data);
        str.Dtor();
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

    ar->ReadBuf(m_b8, 0x24);

    char name[0x80];
    ar->ReadBuf(name, 0x80);
    ((CStringAssign*)&m_name)->Assign(name);

    ar->ReadBuf(&m_e4, 4);
    ar->ReadBuf(&m_e8, 4);
    ar->ReadBuf(&m_ec, 4);
    ar->ReadBuf(&m_f0, 4);
    ar->ReadBuf(&m_f4, 4);
    ar->ReadBuf(&m_f8, 4);
    ar->ReadBuf(&m_fc, 4);
    ar->ReadBuf(&m_100, 4);
    ar->ReadBuf(&m_104, 4);
    ar->ReadBuf(&m_108, 4);
    ar->ReadBuf(&m_10c, 4);
    ar->ReadBuf(&m_110, 4);
    ar->ReadBuf(&m_114, 4);
    ar->ReadBuf(&m_118, 4);
    ar->ReadBuf(&m_11c, 4);
    ar->ReadBuf(&m_120, 4);
    ar->ReadBuf(&m_124, 4);
    ar->ReadBuf(&m_128, 4);
    ar->ReadBuf(&m_12c, 4);
    ar->ReadBuf(&m_130, 4);
    ar->ReadBuf(&m_134, 0x10);
    ar->ReadBuf(&m_144, 0x10);
    ar->ReadBuf(&m_154, 0x10);
    ar->ReadBuf(&m_164, 4);
    ar->ReadBuf(&m_168, 4);
    ar->ReadBuf(&m_16c, 4);
    ar->ReadBuf(&m_170, 4);
    ar->ReadBuf(&m_174, 4);
    ar->ReadBuf(&m_178, 4);
    ar->ReadBuf(&m_17c, 4);
    ar->ReadBuf(&m_180, 4);
    ar->ReadBuf(&m_10, 4);
    ar->ReadBuf(&m_14, 4);
    ar->ReadBuf(&m_lastX, 0x24); // +0x18 render-state block
    ar->ReadBuf(&m_40, 4);
    ar->ReadBuf(&m_44, 4);
    ar->ReadBuf(&m_48, 4);
    ar->ReadBuf(&m_50, 4);
    ar->ReadBuf(&m_54, 4);
    ar->ReadBuf(&m_58, 4);
    ar->ReadBuf(&m_clipLeft, 0x10); // +0x64 clip rect
    ar->ReadBuf(&m_04, 4);
    ar->ReadBuf(&m_flags, 4);
    ar->ReadBuf(&m_184, 4);

    ar->ReadBuf(name, 0x80);
    if (strlen(name) != 0) {
        void* found = 0;
        m_mgr->m_14->m_map.Lookup(name, &found);
        if (Resolve150eb0(found) == 0) {
            return 0;
        }
    }

    ar->ReadBuf(name, 0x80);
    if (strlen(name) != 0) {
        void* found = 0;
        m_mgr->m_14->m_map.Lookup(name, &found);
        if (Resolve150f90(found) == 0) {
            return 0;
        }
    }

    ar->ReadBuf(name, 0x80);
    if (strlen(name) != 0) {
        void* found = 0;
        m_mgr->m_14->m_map.Lookup(name, &found);
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
    AnimWorker* w = m_worker;
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

    w = m_worker;
    i32 edi = 0;
    if (w->m_18 != 0) {
        edi = w->m_18->Vfunc8();
    }

    WwdSnapshot rec;
    rec.m_00 = m_04;
    rec.m_08 = self->Vfunc20();
    rec.m_04 = m_188;
    rec.m_94 = m_posX;
    rec.m_98 = m_posY;
    rec.m_9c = m_74;
    rec.m_0c = ebx;
    rec.m_10 = edi;

    {
        EngStr str;
        m_mgr->m_14->Build(&str, m_worker);
        strcpy(rec.m_name, str.m_data);
        str.Dtor();
    }
    ar->Xfer(&rec, 0xa0);
    return 1;
}

// ---------------------------------------------------------------------------
// Init (0x15b940): zero +0x19c, construct the +0x1a0 command map, then Setup.
// ---------------------------------------------------------------------------
RVA(0x0015b940, 0x38)
i32 CWwdGameObject::Init(i32 a1, i32 a2, i32 a3, i32 a4) {
    m_19c = 0;
    m_cmdMap.Construct(this);
    return Setup(a1, a2, a3, a4);
}

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
    WwdSubNode* n = m_subList.m_head;
    while (n != 0) {
        WwdSubNode* next = n->m_next;
        WwdSubDel* p = n->m_data;
        if (p != 0) {
            p->DeleteSelf(1);
        }
        n = next;
    }
    m_subList.RemoveAll_1b5a0b();
    return Setup(a1, a2, a3, a4) != 0;
}

// ---------------------------------------------------------------------------
// SetupFlagged (0x15c1d0): stash the byte flag at +0x18c, then forward the four
// args to Setup.  __thiscall, 5 stack args (ret 0x14).
// ---------------------------------------------------------------------------
RVA(0x0015c1d0, 0x26)
i32 CWwdGameObject::SetupFlagged(i32 a1, i32 a2, i32 a3, i32 a4, i32 flag) {
    *(char*)&m_dotColor = (char)flag;
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
    i32 x = m_posX;
    i32 m64 = m_clipLeft;
    i32 y;
    if (m64 == (i32)0x80000000) {
        if (x < 0) {
            goto reject;
        }
        y = m_posY;
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
        y = m_posY;
        if (y < m_clipTop) {
            goto reject;
        }
        if (x > m_clipRight) {
            goto reject;
        }
        if (y > m_clipBottom) {
            goto reject;
        }
    }

    {
        WwdSurface* surf = a->m_2c;
        i32 base = surf->GetRowBase(0);
        if (base != 0) {
            i32 row = surf->m_20 * y;
            i32 col = surf->m_b0 * x;
            *(char*)(base + row + col) = *(char*)&m_dotColor;
            void* n = surf->m_08;
            (*(void (**)(void*, i32))((char*)*(void**)n + 0x80))(n, 0);
        }
    }
    m_lastX = m_posX;
    m_lastY = m_posY;
    m_30 = 1;
    m_34 = 1;
    m_clipResult = 0;
    return;
reject:
    m_clipResult = -1;
}

// class-metadata sweep: grunt/game-object family size annotations (SIZE_UNKNOWN = retail size TBD, at .cpp EOF).
SIZE_UNKNOWN(Archive);
SIZE_UNKNOWN(CMapStringToObLite);
SIZE_UNKNOWN(CStringAssign);
SIZE(CmdMap, 0x3c);
SIZE_UNKNOWN(MapLookupA);
SIZE_UNKNOWN(MapLookupB);
SIZE_UNKNOWN(MgrSub158570);
SIZE_UNKNOWN(MgrSub165360);
SIZE_UNKNOWN(WorkerSub);
SIZE_UNKNOWN(WwdMgr);
SIZE_UNKNOWN(WwdMgrSub08);
SIZE_UNKNOWN(WwdMgrSub10);
SIZE_UNKNOWN(WwdRenderCtx);
SIZE_UNKNOWN(WwdSelf);
SIZE_UNKNOWN(WwdSnapshot);
SIZE_UNKNOWN(WwdSubDel);
SIZE_UNKNOWN(WwdSubList);
SIZE_UNKNOWN(WwdSubNode);
SIZE_UNKNOWN(WwdSurface);
