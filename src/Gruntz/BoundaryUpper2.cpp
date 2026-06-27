// BoundaryUpper2.cpp - upper-half (RVA >= 0x133370) engine_boundary backlog,
// reconstructed (second pass over the harder remainder). These sit at class
// boundaries in GRUNTZ.EXE across the DinMgr2 / Dsndmgr / DDrawMgr / Rez engine
// modules; RTTI cannot attribute the COMDAT-folded leaf methods so the owning
// class names here are placeholders. Only OFFSETS + the code shape are
// load-bearing (campaign doctrine). Non-EH (base /O2) bodies only; the /GX
// EH-frame siblings live in BoundaryUpper2Eh.cpp.
#include <Ints.h>
#include <string.h> // memset -> rep stos at /O2
#include <rva.h>

// The engine __cdecl allocator/deallocator (operator new/delete; reloc-masked
// rel32). 0x1b9b46 / 0x1b9b82.
extern "C" void* RezAlloc(u32 n);
extern "C" void RezFree(void* p);
void* operator new(u32 n); // engine allocator @0x1b9b46 (same as RezAlloc)

// The severus-worker teardown grand-base vtable (0x5e8cb4); stamped by address
// (named elsewhere, reloc-masked).
extern void* g_severusWorkerDtorVtbl;

// ---------------------------------------------------------------------------
// 0x184b70 - global-object tail-forward: load the singleton address into ecx and
// tail-jump the shared teardown (0x185000). The DATA global is named; the callee
// is reloc-masked.
// ---------------------------------------------------------------------------
struct CHashTail {
    void Clear(); // 0x185000
};
DATA(0x002bf848)
extern CHashTail g_hash184b70; // 0x6bf848
RVA(0x00184b70, 0xa)
void ClearHash_184b70() { g_hash184b70.Clear(); }

// ---------------------------------------------------------------------------
// 0x133370 - DirectInput device-config grand-base dtor: stamp the C-level vftable
// (@0x5ef670) then tail-jump the base subobject teardown (0x134d50). __thiscall.
// ---------------------------------------------------------------------------
extern void* g_deviceConfigVtblC; // 0x5ef670 (named in DirectInputMgr2.cpp)
struct DICfgC {
    void BaseTeardown(); // 0x134d50
    void DtorC();
};
RVA(0x00133370, 0xb)
void DICfgC::DtorC() {
    *(void**)this = &g_deviceConfigVtblC;
    BaseTeardown();
}

// ---------------------------------------------------------------------------
// 0x1396f0 - CRemusReadStream-area init: stamp the +0x1c vftable (@0x5ef740), zero
// the bookkeeping fields, self-link +0x30. Returns `this`. __thiscall.
// ---------------------------------------------------------------------------
DATA(0x001ef740)
extern void* g_vtbl_1396f0; // 0x5ef740
struct C1396f0 {
    void* m_0; // +0x00
    i32 _4[(0x10 - 0x4) / 4];
    i32 m_10; // +0x10
    i32 _14[(0x1c - 0x14) / 4];
    void* volatile m_1c; // +0x1c
    i32 _20[(0x30 - 0x20) / 4];
    void* volatile m_30; // +0x30 (0 then self; volatile pins the dead store + order)
    i32 m_34;   // +0x34
    C1396f0* Init();
};
RVA(0x001396f0, 0x1a)
C1396f0* C1396f0::Init() {
    m_1c = &g_vtbl_1396f0;
    m_30 = 0;
    m_34 = 0;
    m_10 = 0;
    m_0 = 0;
    m_30 = this;
    return this;
}

// ---------------------------------------------------------------------------
// 0x1437f0 - CDDrawPtrCollections "restore lost surfaces" trampoline: if the
// restore-handler function pointer is installed, tail-jump it; else log a warning
// and return 0. __cdecl.
// ---------------------------------------------------------------------------
DATA(0x00283edc)
extern i32 (*g_restoreHandler)(); // 0x683edc
extern void __cdecl DDrawLogLine(char* line); // 0x141cb0
RVA(0x001437f0, 0x1b)
i32 RestoreLostSurfaces_1437f0() {
    if (g_restoreHandler)
        return g_restoreHandler();
    DDrawLogLine("WARNING - Surface(s) lost but no restore handler is available\n");
    return 0;
}

// ---------------------------------------------------------------------------
// 0x1570d0 / 0x157240 - CDDrawWorkerA / CDDrawWorkerB reset: clear the worker
// fields (three inlined timing-resets of +0x20/+0x38, +0x5c, +0x4/+0x8/+0xc) then
// restamp the grand-base dtor vftable (@0x5e8cb4). A has a byte +0x78; B a dword.
// __thiscall.
// ---------------------------------------------------------------------------
struct CDDrawWorkerA {
    void* m_0; // +0x00 vptr
    i32 m_4;   // +0x04
    i32 m_8;   // +0x08
    i32 m_c;   // +0x0c
    i32 _10[(0x20 - 0x10) / 4];
    volatile i32 m_20; // +0x20 (written thrice - redundant stores kept in retail)
    i32 _24[(0x38 - 0x24) / 4];
    volatile i32 m_38; // +0x38
    i32 _3c[(0x5c - 0x3c) / 4];
    i32 m_5c; // +0x5c
    i32 _60[(0x78 - 0x60) / 4];
    i8 m_78; // +0x78 (byte)
    void Reset();
};
// @early-stop
// redundant-store + scheduling wall (~90%): retail resets the +0x20/+0x38 timer
// pair THREE times (identical stores MSVC normally DCEs); `volatile` on m_20/m_38
// reproduces the redundant stores + the edx/eax constant split, but the non-volatile
// m_78/m_5c stores then schedule one slot off retail. No source spelling pins both
// the redundant pairs AND the m_78-first / m_5c-mid order; logic complete.
RVA(0x001570d0, 0x39)
void CDDrawWorkerA::Reset() {
    m_78 = 0;
    m_20 = (i32)0x80000000;
    m_38 = -1;
    m_20 = (i32)0x80000000;
    m_38 = -1;
    m_5c = (i32)0x80000000;
    m_20 = (i32)0x80000000;
    m_38 = -1;
    m_4 = -1;
    m_8 = 0;
    m_c = 0;
    *(void**)this = &g_severusWorkerDtorVtbl;
}

struct CDDrawWorkerB {
    void* m_0; // +0x00 vptr
    i32 m_4;
    i32 m_8;
    i32 m_c;
    i32 _10[(0x20 - 0x10) / 4];
    volatile i32 m_20;
    i32 _24[(0x38 - 0x24) / 4];
    volatile i32 m_38;
    i32 _3c[(0x5c - 0x3c) / 4];
    i32 m_5c;
    i32 _60[(0x78 - 0x60) / 4];
    i32 m_78; // +0x78 (dword)
    void Reset();
};
// @early-stop
// redundant-store + scheduling wall (~90%): mirror of CDDrawWorkerA::Reset (m_78 here
// is a dword). Same volatile-pinned redundant pairs; m_78/m_5c schedule one slot off.
RVA(0x00157240, 0x3c)
void CDDrawWorkerB::Reset() {
    m_78 = 0;
    m_20 = (i32)0x80000000;
    m_38 = -1;
    m_20 = (i32)0x80000000;
    m_38 = -1;
    m_5c = (i32)0x80000000;
    m_20 = (i32)0x80000000;
    m_38 = -1;
    m_4 = -1;
    m_8 = 0;
    m_c = 0;
    *(void**)this = &g_severusWorkerDtorVtbl;
}

// ---------------------------------------------------------------------------
// CDDPageMgr (TextRange/CDDPageMgr boundary) cluster: a page-table object with an
// embedded polymorphic sub-object at +0x124 (vtbl slots 0x30 and 0x54 are used),
// a CObArray-like at +0x138 (RemoveAt @0x1b4bad) and an int* table at +0x13c.
// ---------------------------------------------------------------------------
struct DDPageSub {
    virtual void v0();
    virtual void v1();
    virtual void v2();
    virtual void v3();
    virtual void v4();
    virtual void v5();
    virtual void v6();
    virtual void v7();
    virtual void v8();
    virtual void v9();
    virtual void v10();
    virtual void v11();
    virtual i32 v12(i32, i32); // slot 0x30
    virtual void v13();
    virtual void v14();
    virtual void v15();
    virtual void v16();
    virtual void v17();
    virtual void v18();
    virtual void v19();
    virtual void v20();
    virtual void v21(); // slot 0x54
};
struct DDPageArr {
    void RemoveAt(i32, i32); // 0x1b4bad
};
struct CDDPageMgr {
    i32 m_0; // +0x00
    i32 m_4; // +0x04
    i32 m_8; // +0x08
    i32 m_c; // +0x0c
    i32 m_10; // +0x10
    u32 m_14; // +0x14
    i32 m_18[0x43]; // +0x18 .. +0x123
    DDPageSub m_124; // +0x124
    i32 m_128; // +0x128
    i32 _12c[(0x134 - 0x12c) / 4];
    i32 m_134; // +0x134
    DDPageArr m_138; // +0x138
    i32* m_13c; // +0x13c
    i32 Init();        // 0x17b510
    void Close();      // 0x17b570
    i32 Free();        // 0x17b5a0
    i32 Lookup(u32);   // 0x17b840
};

RVA(0x0017b510, 0x55)
i32 CDDPageMgr::Init() {
    if (m_0)
        return 0;
    m_4 = 0;
    m_8 = 0;
    m_138.RemoveAt(0, -1);
    memset(&m_c, 0, 12); // m_c, m_10, m_14
    memset(m_18, 0, sizeof(m_18));
    m_134 = 0;
    m_0 = 1;
    return 1;
}

RVA(0x0017b570, 0x24)
void CDDPageMgr::Close() {
    if (!m_0)
        return;
    Free();
    m_138.RemoveAt(0, -1);
    m_0 = 0;
}

RVA(0x0017b5a0, 0x48)
i32 CDDPageMgr::Free() {
    if (m_0 && (m_4 || m_8)) {
        m_124.v21();
        m_4 = 0;
        m_8 = 0;
        m_134 = 0;
        return 1;
    }
    return 0;
}

RVA(0x0017b840, 0x53)
i32 CDDPageMgr::Lookup(u32 idx) {
    if (m_4 && m_0 && idx <= m_14 && idx != 0) {
        i32* slot = &m_13c[idx - 1];
        if (m_124.v12(*slot, 0) == *slot)
            return m_128;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// 0x174ed0 / 0x175780 - CImage owner free/clear pair. The owner (this) holds a
// CObArray-like sub at +0x10; the argument object carries m_44c/m_454/m_458 and a
// teardown D (0x175c90) and a method X (0x176ad0). __thiscall.
// ---------------------------------------------------------------------------
struct CImgArg {
    i32 _0[0x44c / 4];
    i32 m_44c; // +0x44c
    i32 _450;  // +0x450
    i32 m_454; // +0x454
    i32 m_458; // +0x458
    void X(i32, i32); // 0x176ad0
    void D();         // 0x175c90
};
struct CImgSub10 {
    void Add(i32); // 0x1b4ac7
};
struct CImgOwner {
    i32 _0[4];      // +0x00 .. +0x0c
    CImgSub10 m_10; // +0x10
    void A(i32);                // 0x174f30
    void B(CImgArg*, i32, i32); // 0x175780
    void Free(CImgArg*);        // 0x174ed0
};
RVA(0x00174ed0, 0x5d)
void CImgOwner::Free(CImgArg* o) {
    if (!o)
        return;
    if (o->m_458 && o->m_454) {
        A(o->m_458);
        B(0, 0, 0);
    }
    if (o->m_44c)
        m_10.Add(o->m_44c);
    o->D();
    RezFree(o);
}
RVA(0x00175780, 0x3f)
void CImgOwner::B(CImgArg* o, i32 a, i32 b) {
    if (o->m_458 && o->m_454) {
        A(o->m_458);
        o->X(0, 0);
    }
    o->X(a, b);
}

// ---------------------------------------------------------------------------
// 0x137200 - SoundDevice restore: if the COM buffer (+0x84) is present and the
// device Probe (0x137260) succeeds, call its Restore (vtbl slot 0xc) and on
// failure log a Dsndmgr error. __thiscall.
// ---------------------------------------------------------------------------
struct ISndBuf;
struct ISndBufVtbl {
    i32 _0[0x30 / 4];
    i32(__stdcall* Restore)(ISndBuf*, i32, i32, i32); // +0x30
};
struct ISndBuf {
    ISndBufVtbl* vtbl;
};
extern void __cdecl SndErr(const char* file, i32 line, i32 flag); // 0x138150
struct SndDevice {
    i32 _0[0x78 / 4];
    void* m_78; // +0x78
    i32 _7c[(0x84 - 0x7c) / 4];
    ISndBuf* m_84; // +0x84
    i32 Probe();   // 0x137260
    i32 Restore();
};
RVA(0x00137200, 0x53)
i32 SndDevice::Restore() {
    if (!m_78)
        return 0;
    if (!Probe())
        return 0;
    i32 ok = m_84->vtbl->Restore(m_84, 0, 0, 1) != 0;
    if (ok) {
        SndErr("C:\\Proj\\Dsndmgr\\DSNDMGR.CPP", 0x68b, ok);
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// 0x13ba70 - CSymParser thunk: allocate a 4-byte local, pass its address to the
// sub (0x120210), return its result. __cdecl, no args.
// ---------------------------------------------------------------------------
extern i32 Sub120210(void* p); // 0x120210
RVA(0x0013ba70, 0x10)
i32 Thunk13ba70() {
    i32 local;
    return Sub120210(&local);
}

// ---------------------------------------------------------------------------
// 0x13b910 - pack up to 4 leading chars of a string into a right-justified DWORD
// (reverse byte order). __stdcall, 1 arg.
// ---------------------------------------------------------------------------
RVA(0x0013b910, 0x58)
u32 __stdcall PackTag_13b910(const char* s) {
    if (!s)
        return 0;
    u32 r = 0;
    u8* rb = (u8*)&r;
    i32 len = (i32)strlen(s);
    if (len > 0)
        rb[len - 1] = s[0];
    if (len > 1)
        rb[len - 2] = s[1];
    if (len > 2)
        rb[len - 3] = s[2];
    if (len > 3)
        rb[len - 4] = s[3];
    return r;
}

// ---------------------------------------------------------------------------
// 0x13b970 - inverse of PackTag: unpack a DWORD tag into a string (high non-zero
// byte first), null-terminated. __stdcall, 2 args (tag, dst).
// ---------------------------------------------------------------------------
RVA(0x0013b970, 0x72)
void __stdcall UnpackTag_13b970(u32 tag, char* dst) {
    if (!dst)
        return;
    u8* tb = (u8*)&tag;
    i32 len = 0;
    if (tb[3])
        len = 4;
    else if (tb[2])
        len = 3;
    else if (tb[1])
        len = 2;
    else if (tb[0])
        len = 1;
    if (len > 0)
        dst[0] = tb[len - 1];
    if (len > 1)
        dst[1] = tb[len - 2];
    if (len > 2)
        dst[2] = tb[len - 3];
    if (len > 3)
        dst[3] = tb[len - 4];
    dst[len] = 0;
}

// ---------------------------------------------------------------------------
// 0x13e7d0 - CDDSurface restore: build a 0x64-byte descriptor (size + arg2), call
// the restore helper (0x13eef0); on failure log a DIRSURF error. Returns success.
// __thiscall, 2 args.
// ---------------------------------------------------------------------------
struct RestoreDesc {
    i32 size; // +0x00
    i32 _4[(0x50 - 0x4) / 4];
    i32 m_50; // +0x50
    i32 _54[(0x64 - 0x54) / 4];
};
extern void __cdecl DirSurfLog(const char* file, i32 line, i32 hr); // 0x141400
struct CDDSurf13e7d0 {
    i32 Restore(void* arg1, i32 arg2);
    i32 H(void* a, i32 b, i32 c, i32 flags, RestoreDesc* d); // 0x13eef0
};
RVA(0x0013e7d0, 0x73)
i32 CDDSurf13e7d0::Restore(void* arg1, i32 arg2) {
    if (!arg1)
        return 0;
    RestoreDesc d;
    memset(&d, 0, sizeof(d));
    d.size = 0x64;
    d.m_50 = arg2;
    i32 hr = H(arg1, 0, 0, 0x1000400, &d);
    if (hr)
        DirSurfLog("C:\\Proj\\DDrawMgr\\DIRSURF.CPP", 0x26d, hr);
    return hr == 0;
}

// ---------------------------------------------------------------------------
// 0x143040 - CDDrawPtrCollections factory: allocate+zero a 0x38-byte node, Init it
// (this->m_0, a, b); on success Add it to this and return it, else Cleanup+free and
// return 0. __thiscall, 2 args.
// ---------------------------------------------------------------------------
struct Node38 {
    i32 m_0;
    i32 m_4;
    i32 m_8;
    i32 m_c;
    i32 m_10;
    i32 m_14;
    i32 m_18;
    i32 _1c[(0x2c - 0x1c) / 4];
    i32 m_2c;
    i32 m_30;
    i32 m_34;
    Node38() {
        m_4 = 0;
        m_0 = 0;
        m_8 = 0;
        m_c = 0;
        m_10 = 0;
        m_34 = 0;
        m_18 = 0;
        m_14 = 0;
        m_2c = 0;
        m_30 = 0;
    }
    i32 Init(i32, i32, i32); // 0x147390
    void Cleanup();          // 0x147530
};
struct CNodeFactory {
    i32 m_0; // +0x00
    void Add(Node38*); // 0x142eb0
    Node38* Create(i32 a, i32 b);
};
RVA(0x00143040, 0x7c)
Node38* CNodeFactory::Create(i32 a, i32 b) {
    Node38* o = new Node38();
    if (!o->Init(m_0, a, b)) {
        if (o) {
            o->Cleanup();
            RezFree(o);
        }
        return 0;
    }
    Add(o);
    return o;
}

// ---------------------------------------------------------------------------
// 0x1509c0 - CWwdGameObject visibility test: derive the four edges from the
// object's centre (m_5c/m_60) and half-extents (m_198->m_18/m_1c), then bounds-check
// against either the camera rect (+0x40 of m_c->m_24->m_5c, flag 0x40000 set) or the
// grid extents (m_c->m_4->m_10). __thiscall, 0 args.
// ---------------------------------------------------------------------------
struct WwdExtent {
    i32 _0[0x18 / 4];
    i32 m_18; // +0x18
    i32 m_1c; // +0x1c
};
struct WwdCamRect {
    i32 a; // +0x40 (left)
    i32 b; // +0x44 (top)
    i32 c; // +0x48 (right)
    i32 d; // +0x4c (bottom)
};
struct WwdCamHolder {
    i32 _0[0x5c / 4];
    char* m_5c; // +0x5c -> &m_40 rect via +0x40
};
struct WwdGridLim {
    i32 _0[0x10 / 4];
    i32 m_10; // +0x10
    i32 m_14; // +0x14
};
struct WwdGridHolder {
    i32 _0[0x10 / 4];
    WwdGridLim* m_10; // +0x10
};
struct WwdCtx {
    i32 _0[1];      // +0x00
    WwdGridHolder* m_4;  // +0x04
    i32 _8[(0x24 - 0x8) / 4];
    WwdCamHolder* m_24;  // +0x24
};
struct CWwdObj1509 {
    i32 _0[2];      // +0x00
    u32 m_8;        // +0x08 flags
    WwdCtx* m_c;    // +0x0c
    i32 _10[(0x5c - 0x10) / 4];
    i32 m_5c;       // +0x5c
    i32 m_60;       // +0x60
    i32 _64[(0x198 - 0x64) / 4];
    WwdExtent* m_198; // +0x198
    i32 Test();
};
// @early-stop
// regalloc wall (~73%): the four derived edges (right/left/top/bottom) +
// m_198/m_c/m_8 need 4 callee-saved regs in this reconstruction where retail packs
// them into 3 (ebx/esi/edi) by keeping m_198 in edi and testing m_8 directly from
// memory. No source spelling reproduces retail's exact edge-register assignment;
// logic (both the camera-rect and grid-extent bounds checks) complete.
RVA(0x001509c0, 0xab)
i32 CWwdObj1509::Test() {
    WwdExtent* e = m_198;
    if (!e)
        return 0;
    i32 right = m_5c + e->m_18;
    i32 left = m_5c - e->m_18;
    i32 top = m_60 - e->m_1c;
    i32 bottom = m_60 + e->m_1c;
    if (m_8 & 0x40000) {
        WwdCamRect* r = (WwdCamRect*)(m_c->m_24->m_5c + 0x40);
        if (right < r->a)
            return 0;
        if (left > r->c)
            return 0;
        if (bottom < r->b)
            return 0;
        return top <= r->d;
    } else {
        WwdGridLim* g = m_c->m_4->m_10;
        if (right < 0)
            return 0;
        if (left >= g->m_10)
            return 0;
        if (bottom < 0)
            return 0;
        return top < g->m_14;
    }
}

// ---------------------------------------------------------------------------
// 0x163710 - CDDrawWorkerList dispatch: switch on the kind (3..8); kind 4 probes
// via 0x163780, kind 7 via 0x1638c0; failure returns 0, every other case returns 1.
// __stdcall, 4 args (only the first two used).
// ---------------------------------------------------------------------------
extern i32 __stdcall Probe163780(void* p); // 0x163780
extern i32 __stdcall Probe1638c0(void* p); // 0x1638c0
// @early-stop
// jump-table-shape wall (~84%): retail lowers the kind switch (cases 3..8, only 4
// and 7 active) to a dense `jmp [eax*4+table]`; MSVC here folds the 4 default-equal
// cases and emits a compare ladder instead. Forcing 6 explicit cases still merges
// them (78%); the 2-case ladder is closest. Logic complete.
RVA(0x00163710, 0x42)
i32 __stdcall Dispatch163710(void* p, i32 kind, i32, i32) {
    if (!p)
        return 0;
    switch (kind) {
    case 4:
        if (!Probe163780(p))
            return 0;
        break;
    case 7:
        if (!Probe1638c0(p))
            return 0;
        break;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// 0x13a530 - CSymTab remove-entry: shrink the running size (m_10) by the entry's
// span (m_c), unlink it via the +0x24 helper (0x184ab0), run the entry's own
// teardown (0x1397a0), drop it from the list (0x13c210) and clear the list's count.
// __thiscall, 2 args.
// ---------------------------------------------------------------------------
struct SymHelper24 {
    void Unlink(void* p); // 0x184ab0
};
struct SymEntry2 {
    i32 _0[0xc / 4];
    i32 m_c;  // +0x0c span
    i32 _10[(0x1c - 0x10) / 4];
    i32 m_1c; // +0x1c
    void Teardown(); // 0x1397a0
};
struct SymEntry1 {
    i32 _0[0x24 / 4];
    SymHelper24 m_24; // +0x24
};
struct SymList18 {
    i32 _0[2];
    i32 m_8; // +0x08
    void Drop(void* p); // 0x13c210
};
struct CSymTab13a530 {
    i32 _0[4];
    i32 m_10; // +0x10
    i32 _14;  // +0x14
    SymList18* m_18; // +0x18
    i32 Remove(SymEntry1* a1, SymEntry2* a2);
};
RVA(0x0013a530, 0x47)
i32 CSymTab13a530::Remove(SymEntry1* a1, SymEntry2* a2) {
    m_10 -= a2->m_c;
    a1->m_24.Unlink(&a2->m_1c);
    a2->Teardown();
    m_18->Drop(a2);
    m_18->m_8 = 0;
    return 1;
}

// ---------------------------------------------------------------------------
// 0x17e230 - destroy a by-value CDataBuffer-like parameter: the only work is the
// parameter's own destructor (0x1b9cde) at function exit. __stdcall, 1 arg.
// ---------------------------------------------------------------------------
struct DBuf17e230 {
    void* p;
    ~DBuf17e230(); // 0x1b9cde
};
RVA(0x0017e230, 0xc)
void __stdcall Destroy17e230(DBuf17e230 b) { (void)b; }

// ---------------------------------------------------------------------------
// 0x143950 - CDDrawPtrCollections palette upload: copy a 256-entry RGB triplet
// table into the object's BGRA0 palette at +0x53c, then mark dirty (+0x93c) and
// store the tag (+0x940). __thiscall, 2 args.
// ---------------------------------------------------------------------------
struct CPalObj143950 {
    i32 _0[0x53c / 4];
    u8 m_pal[256][4]; // +0x53c
    i32 m_93c;        // +0x93c
    i32 m_940;        // +0x940
    i32 SetPalette(const u8* src, i32 tag);
};
// @early-stop
// strength-reduction/regalloc wall (~78%): retail walks the dst palette via a
// pre-incremented edx (dst+1, -1/-4/-3/-2 displacements) with src in eax and inc;
// MSVC here keeps src in edx (advance by 3) and dst in eax (advance by 4) - the
// mirror register assignment. Loop logic complete.
RVA(0x00143950, 0x56)
i32 CPalObj143950::SetPalette(const u8* src, i32 tag) {
    if (!src)
        return 0;
    u8* dst = m_pal[0];
    for (i32 i = 0; i < 256; i++) {
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
        dst[3] = 0;
        dst += 4;
        src += 3;
    }
    m_93c = 1;
    m_940 = tag;
    return 1;
}

// ---------------------------------------------------------------------------
// 0x148af0 - CImageOwned setup: zero the 0x6c-byte transform descriptor at +0x10,
// fill {size, flags|0x200, fields}, run Apply (0x13e0a0); on success run the commit
// virtual (slot 10). __thiscall, 4 args.
// ---------------------------------------------------------------------------
struct Blk6c {
    i32 d[0x1b];
};
struct ImgOwnedX {
    virtual void v0();
    virtual void v1();
    virtual void v2();
    virtual void v3();
    virtual void v4();
    virtual void v5();
    virtual void v6();
    virtual void v7();
    virtual void v8();
    virtual void v9();
    virtual void Commit(); // slot 10 (+0x28)
    i32 _4[(0x10 - 0x4) / 4];
    Blk6c m_10; // +0x10
    i32 Apply(i32 mode, const void* src); // 0x13e0a0
    i32 Setup(i32 a1, i32 a2, i32 a3, i32 a4);
};
RVA(0x00148af0, 0x58)
i32 ImgOwnedX::Setup(i32 a1, i32 a2, i32 a3, i32 a4) {
    memset(&m_10, 0, 0x6c);
    m_10.d[0] = 0x6c;
    m_10.d[0x1a] = a2 | 0x200;
    m_10.d[1] = a3;
    m_10.d[5] = a4;
    if (!Apply(a1, 0))
        return 0;
    Commit();
    return 1;
}

// ---------------------------------------------------------------------------
// 0x148a50 / 0x148c40 - CImageOwned blit-setup variants: build a 0x6c-byte local
// transform descriptor and run Apply with a mode (7 / 0x47), returning success.
// __thiscall.
// ---------------------------------------------------------------------------
struct ImgOwnedY {
    i32 Apply(i32 mode, const void* src); // 0x13e0a0
    i32 Blit7(i32 a1, i32 a2, i32 a3, i32 a4);
    i32 Blit47(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7);
};
// @early-stop
// descriptor-fill scheduling wall (~82%): same Apply path as the 100% ImgOwnedX::Setup
// but into a stack-local descriptor; retail hoists the a4 load (eax, or al,0x80) ahead
// of a2 while MSVC here loads a2 first, swapping the eax/ecx assignment + a couple
// store slots. Logic complete.
RVA(0x00148a50, 0x6b)
i32 ImgOwnedY::Blit7(i32 a1, i32 a2, i32 a3, i32 a4) {
    Blk6c d;
    memset(&d, 0, 0x6c);
    d.d[3] = a2;
    d.d[0x1a] = a4 | 0x80;
    d.d[2] = a3;
    d.d[0x10] = 1;
    d.d[0x11] = 1;
    d.d[0] = 0x6c;
    d.d[1] = 7;
    return Apply(a1, &d) != 0;
}
// @early-stop
// descriptor-fill scheduling wall (~85%): mirror of Blit7 (7-arg / mode 0x47). Same
// stack-local-descriptor load/store scheduling divergence. Logic complete.
RVA(0x00148c40, 0x75)
i32 ImgOwnedY::Blit47(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7) {
    (void)a6;
    Blk6c d;
    memset(&d, 0, 0x6c);
    d.d[3] = a2;
    d.d[0x1a] = a5 | a4 | 0x20000;
    d.d[6] = a7;
    d.d[2] = a3;
    d.d[0] = 0x6c;
    d.d[1] = 0x47;
    return Apply(a1, &d) != 0;
}
