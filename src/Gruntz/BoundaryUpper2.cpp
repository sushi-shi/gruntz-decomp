// BoundaryUpper2.cpp - upper-half (RVA >= 0x133370) engine_boundary backlog,
// reconstructed (second pass over the harder remainder). These sit at class
// boundaries in GRUNTZ.EXE across the DinMgr2 / Dsndmgr / DDrawMgr / Rez engine
// modules; RTTI cannot attribute the COMDAT-folded leaf methods so the owning
// class names here are placeholders. Only OFFSETS + the code shape are
// load-bearing (campaign doctrine). Non-EH (base /O2) bodies only; the /GX
// EH-frame siblings live in BoundaryUpper2Eh.cpp. The per-use owner/referent
// views now live in <Gruntz/BoundaryUpper2Views.h> (pure code motion).
#include <Ints.h>
#include <string.h> // memset -> rep stos at /O2
#include <rva.h>
#include <Gruntz/BoundaryUpper2Views.h> // owner/referent views for this TU (pulls Blk6c.h)
#include <Globals.h>

// The engine __cdecl allocator/deallocator (operator new/delete; reloc-masked
// rel32). 0x1b9b46 / 0x1b9b82.
extern "C" void RezFree(void* p);
void* operator new(u32 n); // engine allocator @0x1b9b46 (same as RezAlloc)
inline void* operator new(u32, void* p) {
    return p;
} // placement new (construct-in-place; no allocation)

// The wap-object teardown grand-base vtable (0x5e8cb4); stamped by address
// (named elsewhere, reloc-masked).
extern void* g_wapObjectDtorVtbl;

// ---------------------------------------------------------------------------
// 0x184b70 - global-object tail-forward: load the singleton address into ecx and
// tail-jump the shared teardown (0x185000). The DATA global is named; the callee
// is reloc-masked.
// ---------------------------------------------------------------------------
DATA(0x002bf848)
// The 0x185000 leaf is CDebugConfig::InitFromEnv (header-less DebugPrintf class); local decl.
class Obj1397a0 { // SymEntry2::Teardown @0x1397a0
public:
    void Teardown();
};
class CDebugConfig {
public:
    CDebugConfig* InitFromEnv();
};
// The 0x13c210 leaf is CSymParser::AddNode (header-less symparser class); local decl.
class CSymParser {
public:
    void AddNode(void* p);
};
extern CHashTail g_hash184b70; // 0x6bf848
RVA(0x00184b70, 0xa)
void ClearHash_184b70() {
    ((CDebugConfig*)&g_hash184b70)->InitFromEnv();
}

// ---------------------------------------------------------------------------
// 0x133370 - DirectInput device-config grand-base dtor: stamp the C-level vftable
// (@0x5ef670) then tail-jump the base subobject teardown (0x134d50). __thiscall.
// ---------------------------------------------------------------------------
extern void* g_deviceConfigVtblC; // 0x5ef670 (named in DirectInputMgr2.cpp)
RVA(0x00133370, 0xb)
void DICfgC::DtorC() {
    *(void**)this = &g_deviceConfigVtblC;
    BaseTeardown();
}

// ---------------------------------------------------------------------------
// 0x1396f0 - CParseSource-area init: stamp the +0x1c vftable (@0x5ef740), zero
// the bookkeeping fields, self-link +0x30. Returns `this`. __thiscall.
//
// NOTE: the 0x5ef740 vtable is a SECONDARY / embedded intrusive-hash-node vtable
// (stamped at +0x1c, NOT a class's primary +0 vptr) in the CSymParser/CParseSource
// subsystem. Realized as a cl-emitted ??_7HashNode1396f0 (an embedded polymorphic node
// member at +0x1c, placement-constructed so its implicit vptr stamp supplies the
// `mov [ecx+0x1c],offset ??_7` store) - the mandated real-polymorphic form.
// @early-stop
// vptr-middle re-install wall: the node ctor's implicit vptr stamp reschedules
// relative to the `volatile`-pinned +0x30 dead-store sequence this init depends on, so
// the store order diverges from retail (documented compiler-model wall, accepted per
// the manual-vtable-removal mandate). Logic (the stamp + all field inits) complete;
// ??_7 named via VTBL so the stamp operand still reloc-masks.
// ---------------------------------------------------------------------------
RVA(0x001396f0, 0x1a)
CParseSource* CParseSource::Init() {
    new (&m_1c) HashNode1396f0;
    m_30 = 0;
    m_34 = 0;
    m_10 = 0;
    m_0 = 0;
    m_30 = this;
    return this;
}

// ---------------------------------------------------------------------------
// 0x1437e0 - install the DDraw "restore lost surfaces" handler (re-homed from
// src/Stub/EngineExternFns.cpp): store the supplied function pointer to
// g_restoreHandler (0x683edc; read back by RestoreLostSurfaces_1437f0). __cdecl.
// ---------------------------------------------------------------------------
RVA(0x001437e0, 0xa)
void RelayHwnd(i32 (*handler)()) {
    g_restoreHandler = handler;
}

// ---------------------------------------------------------------------------
// 0x1437f0 - CDDrawPtrCollections "restore lost surfaces" trampoline: if the
// restore-handler function pointer is installed, tail-jump it; else log a warning
// and return 0. __cdecl.
// ---------------------------------------------------------------------------
extern void __cdecl DDrawLogLine(char* line); // 0x141cb0
RVA(0x001437f0, 0x1b)
i32 RestoreLostSurfaces_1437f0() {
    if (g_restoreHandler) {
        return g_restoreHandler();
    }
    DDrawLogLine("WARNING - Surface(s) lost but no restore handler is available\n");
    return 0;
}

// ---------------------------------------------------------------------------
// 0x1570d0 / 0x157240 - CDDrawWorkerA / CDDrawWorkerB reset (flat volatile-tuned
// copy, renamed CDDWorkerFlatA/B): clear the worker fields (three inlined
// timing-resets of +0x20/+0x38, +0x5c, +0x4/+0x8/+0xc) then restamp the grand-base
// dtor vftable (@0x5e8cb4). A has a byte +0x78; B a dword. __thiscall.
// ---------------------------------------------------------------------------
// @early-stop
// redundant-store + scheduling wall (~90%): retail resets the +0x20/+0x38 timer
// pair THREE times (identical stores MSVC normally DCEs); `volatile` on m_20/m_38
// reproduces the redundant stores + the edx/eax constant split, but the non-volatile
// m_78/m_5c stores then schedule one slot off retail. No source spelling pins both
// the redundant pairs AND the m_78-first / m_5c-mid order; logic complete.
RVA(0x001570d0, 0x39)
void CDDWorkerFlatA::Reset() {
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
    *(void**)this = &g_wapObjectDtorVtbl;
}
// @early-stop
// redundant-store + scheduling wall (~90%): mirror of CDDWorkerFlatA::Reset (m_78 here
// is a dword). Same volatile-pinned redundant pairs; m_78/m_5c schedule one slot off.
RVA(0x00157240, 0x3c)
void CDDWorkerFlatB::Reset() {
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
    *(void**)this = &g_wapObjectDtorVtbl;
}

// ---------------------------------------------------------------------------
// CDDPageMgr-boundary page-table store (renamed CPageStore17b510; NOT the real
// CDDPageMgr in CDirectDrawMgr.h): a page-table object with an embedded polymorphic
// sub-object at +0x124 (vtbl slots 0x30 and 0x54 are used), a CObArray-like at +0x138
// (RemoveAt @0x1b4bad) and an int* table at +0x13c.
// ---------------------------------------------------------------------------
RVA(0x0017b510, 0x55)
i32 CPageStore17b510::Init() {
    if (m_initialized) {
        return 0;
    }
    m_4 = 0;
    m_8 = 0;
    m_pageArr.RemoveAt(0, -1);
    memset(&m_c, 0, 12); // m_c, m_10, m_count
    memset(m_18, 0, sizeof(m_18));
    m_134 = 0;
    m_initialized = 1;
    return 1;
}

RVA(0x0017b570, 0x24)
void CPageStore17b510::Close() {
    if (!m_initialized) {
        return;
    }
    Free();
    m_pageArr.RemoveAt(0, -1);
    m_initialized = 0;
}

RVA(0x0017b5a0, 0x48)
i32 CPageStore17b510::Free() {
    if (m_initialized && (m_4 || m_8)) {
        m_sub.v21();
        m_4 = 0;
        m_8 = 0;
        m_134 = 0;
        return 1;
    }
    return 0;
}

RVA(0x0017b840, 0x53)
i32 CPageStore17b510::Lookup(u32 idx) {
    if (m_4 && m_initialized && idx <= m_count && idx != 0) {
        i32* slot = &m_slots[idx - 1];
        if (m_sub.v12(*slot, 0) == *slot) {
            return m_128;
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// 0x174ed0 / 0x175780 - CImagePool free/clear pair: RE-HOMED to the imagepool unit
// (src/Image/ImagePool.cpp) alongside their siblings (RemovePalette 0x174f30,
// CRezImage::Free 0x175c90 / SetPalette 0x176ad0). See that TU.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// 0x137200 - SoundDevice::StartPrimary: RE-HOMED to the directsoundmgr unit
// (src/Dsndmgr/DirectSoundMgr.cpp) alongside CreatePrimaryBuffer (0x137260); it is
// the real StartPrimary (SoundDevice.h), not a "Restore". See that TU.
// ---------------------------------------------------------------------------

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
    if (!s) {
        return 0;
    }
    u32 r = 0;
    u8* rb = (u8*)&r;
    i32 len = (i32)strlen(s);
    if (len > 0) {
        rb[len - 1] = s[0];
    }
    if (len > 1) {
        rb[len - 2] = s[1];
    }
    if (len > 2) {
        rb[len - 3] = s[2];
    }
    if (len > 3) {
        rb[len - 4] = s[3];
    }
    return r;
}

// ---------------------------------------------------------------------------
// 0x13b970 - inverse of PackTag: unpack a DWORD tag into a string (high non-zero
// byte first), null-terminated. __stdcall, 2 args (tag, dst).
// ---------------------------------------------------------------------------
RVA(0x0013b970, 0x72)
void __stdcall UnpackTag_13b970(u32 tag, char* dst) {
    if (!dst) {
        return;
    }
    u8* tb = (u8*)&tag;
    i32 len = 0;
    if (tb[3]) {
        len = 4;
    } else if (tb[2]) {
        len = 3;
    } else if (tb[1]) {
        len = 2;
    } else if (tb[0]) {
        len = 1;
    }
    if (len > 0) {
        dst[0] = tb[len - 1];
    }
    if (len > 1) {
        dst[1] = tb[len - 2];
    }
    if (len > 2) {
        dst[2] = tb[len - 3];
    }
    if (len > 3) {
        dst[3] = tb[len - 4];
    }
    dst[len] = 0;
}

// ---------------------------------------------------------------------------
// 0x13e7d0 - CDDSurface restore: build a 0x64-byte descriptor (size + arg2), call
// the restore helper (0x13eef0); on failure log a DIRSURF error. Returns success.
// __thiscall, 2 args.
// ---------------------------------------------------------------------------
extern void __cdecl DirSurfLog(const char* file, i32 line, i32 hr); // 0x141400
RVA(0x0013e7d0, 0x73)
i32 CDDSurface::Restore(void* arg1, i32 arg2) {
    if (!arg1) {
        return 0;
    }
    RestoreDesc d;
    memset(&d, 0, sizeof(d));
    d.size = 0x64;
    d.m_50 = arg2;
    i32 hr = H(arg1, 0, 0, 0x1000400, &d);
    if (hr) {
        DirSurfLog("C:\\Proj\\DDrawMgr\\DIRSURF.CPP", 0x26d, hr);
    }
    return hr == 0;
}

// ---------------------------------------------------------------------------
// 0x143040 - CDDrawPtrCollections factory: allocate+zero a 0x38-byte node, Init it
// (this->m_0, a, b); on success Add it to this and return it, else Cleanup+free and
// return 0. __thiscall, 2 args.
// ---------------------------------------------------------------------------
RVA(0x00143040, 0x7c)
CDDPalette* CDDrawPtrCollections::Create(i32 a, i32 b) {
    CDDPalette* o = new CDDPalette();
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
// object's centre (m_centerX/m_centerY) and half-extents (m_extent->m_halfW/m_halfH),
// then bounds-check against either the camera rect (+0x40 of m_ctx->m_camera->m_5c,
// flag 0x40000 set) or the grid extents (m_ctx->m_grid->m_limits). __thiscall, 0 args.
// ---------------------------------------------------------------------------
// @early-stop
// regalloc wall (~73%): the four derived edges (right/left/top/bottom) +
// m_extent/m_ctx/m_flags need 4 callee-saved regs in this reconstruction where retail
// packs them into 3 (ebx/esi/edi) by keeping m_extent in edi and testing m_flags
// directly from memory. No source spelling reproduces retail's exact edge-register assignment;
// logic (both the camera-rect and grid-extent bounds checks) complete.
RVA(0x001509c0, 0xab)
i32 CWwdGameObject::Test() {
    WwdExtent* e = m_extent;
    if (!e) {
        return 0;
    }
    i32 right = m_centerX + e->m_halfW;
    i32 left = m_centerX - e->m_halfW;
    i32 top = m_centerY - e->m_halfH;
    i32 bottom = m_centerY + e->m_halfH;
    if (m_flags & 0x40000) {
        WwdCamRect* r = (WwdCamRect*)(m_ctx->m_camera->m_5c + 0x40);
        if (right < r->a) {
            return 0;
        }
        if (left > r->c) {
            return 0;
        }
        if (bottom < r->b) {
            return 0;
        }
        return top <= r->d;
    } else {
        WwdGridLim* g = m_ctx->m_grid->m_limits;
        if (right < 0) {
            return 0;
        }
        if (left >= g->m_width) {
            return 0;
        }
        if (bottom < 0) {
            return 0;
        }
        return top < g->m_height;
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
    if (!p) {
        return 0;
    }
    switch (kind) {
        case 4:
            if (!Probe163780(p)) {
                return 0;
            }
            break;
        case 7:
            if (!Probe1638c0(p)) {
                return 0;
            }
            break;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// 0x13a530 - CSymTab remove-entry: shrink the running size (m_size) by the entry's
// span (m_span), unlink it via the +0x24 helper (0x184ab0), run the entry's own
// teardown (0x1397a0), drop it from the list (0x13c210) and clear the list's count.
// __thiscall, 2 args.
// ---------------------------------------------------------------------------
RVA(0x0013a530, 0x47)
i32 CSymTab::Remove(SymEntry1* a1, SymEntry2* a2) {
    m_size -= a2->m_span;
    a1->m_24.Unlink(&a2->m_1c);
    ((Obj1397a0*)a2)->Teardown();
    ((CSymParser*)m_list)->AddNode(a2);
    m_list->m_count = 0;
    return 1;
}

// ---------------------------------------------------------------------------
// 0x17e230 - destroy a by-value CDataBuffer-like parameter: the only work is the
// parameter's own destructor (0x1b9cde) at function exit. __stdcall, 1 arg.
// ---------------------------------------------------------------------------
RVA(0x0017e230, 0xc)
void __stdcall Destroy17e230(DBuf17e230 b) {
    (void)b;
}

// ---------------------------------------------------------------------------
// 0x143950 - CDDrawPtrCollections palette upload: copy a 256-entry RGB triplet
// table into the object's BGRA0 palette at +0x53c, then mark dirty (+0x93c) and
// store the tag (+0x940). __thiscall, 2 args.
// ---------------------------------------------------------------------------
// @early-stop
// strength-reduction/regalloc wall (~78%): retail walks the dst palette via a
// pre-incremented edx (dst+1, -1/-4/-3/-2 displacements) with src in eax and inc;
// MSVC here keeps src in edx (advance by 3) and dst in eax (advance by 4) - the
// mirror register assignment. Loop logic complete.
RVA(0x00143950, 0x56)
i32 CPalObj143950::SetPalette(const u8* src, i32 tag) {
    if (!src) {
        return 0;
    }
    u8* dst = m_pal[0];
    for (i32 i = 0; i < 256; i++) {
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
        dst[3] = 0;
        dst += 4;
        src += 3;
    }
    m_dirty = 1;
    m_tag = tag;
    return 1;
}

// ---------------------------------------------------------------------------
// 0x148af0 - CImageOwned setup: zero the 0x6c-byte transform descriptor at +0x10,
// fill {size, flags|0x200, fields}, run Apply (0x13e0a0); on success run the commit
// virtual (slot 10). __thiscall, 4 args.
// ---------------------------------------------------------------------------
RVA(0x00148af0, 0x58)
i32 ImgOwnedX::Setup(i32 a1, i32 a2, i32 a3, i32 a4) {
    memset(&m_10, 0, 0x6c);
    m_10.d[0] = 0x6c;
    m_10.d[0x1a] = a2 | 0x200;
    m_10.d[1] = a3;
    m_10.d[5] = a4;
    if (!Apply(a1, 0)) {
        return 0;
    }
    Commit();
    return 1;
}

// ---------------------------------------------------------------------------
// 0x148a50 / 0x148c40 - CImageOwned blit-setup variants: build a 0x6c-byte local
// transform descriptor and run Apply with a mode (7 / 0x47), returning success.
// __thiscall.
// ---------------------------------------------------------------------------
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
