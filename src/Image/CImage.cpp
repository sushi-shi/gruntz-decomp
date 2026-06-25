#include <rva.h>
// CImage.cpp - the RTTI-confirmed polymorphic CImage (`.?AVCImage@@`, primary
// vftable @0x5eaa2c), a surface-backed image element in the DDrawMgr "Remus"
// image family and a sibling of CDDrawSurfacePair. Three methods (retail-RVA
// order): the cleanup virtual FreeAll (0x153260, slot 7), the /GX destructor
// ~CImage (0x0d5e80), and RenderFrame (0x153790) - the sprite-frame draw that
// resolves a clip rectangle through a shared CRemusNode singleton then dispatches
// the +0x38 render virtual.
//
// See include/Image/CImage.h for the layout + the loader-CImage naming note.
// Field names are placeholders; only the OFFSETS + emitted bytes are load-bearing.
// The TU carries a destructible base subobject -> /GX EH frame (flags="eh").
// ---------------------------------------------------------------------------

#include <Image/CImage.h>

// The engine __cdecl deallocator (reloc-masked rel32). _RezFree @0x1b9b82.
extern "C" void RezFree(void* p);

// The +0x28 load virtual (LoadDispatch, slot 10) reached via the vtable from
// Resolve. Modeled as a __thiscall pointer-to-member in a typed vtable struct (the
// MSVC5-period idiom; the class is complete -> 4-byte PMF) so the call lowers to
// `mov edx,[this]; ...; call [edx+0x28]` with callee-side cleanup.
struct CImageLoadVtbl;
class CImageLoadDispatch {
public:
    CImageLoadVtbl* vptr;                                             // +0x00
    i32 LoadVirtual(CImageFrameDesc* desc, u32 mode, void* a, i32 b); // vtbl[0x28]
};
typedef i32 (CImageLoadDispatch::*LoadVirtualFn)(CImageFrameDesc*, u32, void*, i32);
struct CImageLoadVtbl {
    char _00[0x28 - 0x00];     // slots 0..9 (@0x00..0x24)
    LoadVirtualFn LoadVirtual; // [0x28]
};
inline i32 CImageLoadDispatch::LoadVirtual(CImageFrameDesc* desc, u32 mode, void* a, i32 b) {
    return (this->*(vptr->LoadVirtual))(desc, mode, a, b);
}

// ---------------------------------------------------------------------------
// 0x152f20 (vtable slot 11): Resolve. Read the source's 3-char format tag, map it
// to a format index (BMP=1, "CPX"=2, "RID"=3, "PID"=4), prime the source, then
// dispatch the +0x28 load virtual (LoadDispatch) with (resolved, index, src->m_0c,
// arg); finally release the source and return the load result. __thiscall, ret 8.
//
// The tag compare lowers to MSVC's unsigned binary-search switch tree
// (cmp;ja/je against the fourcc constants) -> the key must be unsigned (the source
// returns the tag as a u32). docs/patterns/switch-key-unsigned-ja-vs-jg.md.
// ---------------------------------------------------------------------------
RVA(0x00152f20, 0x86)
i32 CImage::Resolve(CImageSource* src, i32 arg) {
    i32 index;
    switch ((u32)src->GetTag()) {
        case 0x424d50: // 'PMB' (BMP)
            index = 1;
            break;
        case 0x504358: // 'XCP'
            index = 2;
            break;
        case 0x524944: // 'DIR'
            index = 3;
            break;
        case 0x504944: // 'DIP'
            index = 4;
            break;
        default:
            return 0;
    }
    i32 resolved = src->Resolve();
    if (resolved == 0) {
        return 0;
    }
    i32 result = ((CImageLoadDispatch*)this)
                     ->LoadVirtual((CImageFrameDesc*)resolved, (u32)index, (void*)src->m_0c, arg);
    src->Release();
    return result;
}

// ---------------------------------------------------------------------------
// 0x152fb0 (vtable slot 10): LoadDispatch. The format-index switch (1..4); index 4
// with the desc's 0x20 flag takes the slot-13 build path (an EH /GX builder),
// otherwise the default path allocates a surface from the parent pool (CreateA),
// caching its geometry into m_10..m_28. __thiscall, ret 0x10 (4 stack args).
//
// The 1..4 range-check lowers to a chain of `cmp;je` (no jump table) -> a sequence
// of explicit case tests, key unsigned (matches the cmp;ja in the caller).
// @early-stop
// 99.86% - all 108 instructions byte-identical to retail (verified llvm-objdump
// base vs target). The lone residual is the objdiff reloc-typing scoring artifact
// on the two g_severusCounterA/B DIR32 refs (REL32-vs-DIR32 against differently-
// named symbols); the code bytes match. See MEMORY objdiff-reloc-scoring.
// ---------------------------------------------------------------------------
RVA(0x00152fb0, 0x123)
i32 CImage::LoadDispatch(CImageFrameDesc* desc, u32 mode, void* a, i32 b) {
    if (mode != 1 && mode != 2 && mode != 3 && mode != 4) {
        return 0;
    }
    if (mode == 4 && (desc->m_04 & 0x20)) {
        if (!BuildSlot13(desc, a)) {
            return 0;
        }
        if (m_30 != 0 && (desc->m_04 & 0x40)) {
            ImageNotify(2, 0);
            return 1;
        }
        return 1;
    }
    i32 flagsArg = (b != 0) ? g_severusCounterB : -1;
    if (mode == 4 || mode == 3) {
        i32 g10 = desc->m_10;
        i32 g14 = desc->m_14;
        m_20 = g10;
        m_24 = g14;
    } else {
        m_20 = 0;
        m_24 = 0;
    }
    i32 capArg = 0;
    if (g_severusCounterA != 0) {
        capArg = 0x800;
    }
    CImageSurfaceItem* item = m_0c->m_1c->CreateA((i32)desc, (i32)mode, a, capArg, flagsArg);
    m_2c = item;
    if (item == 0) {
        return 0;
    }
    i32 w = item->m_1c;
    m_10 = w;
    i32 h = item->m_18;
    m_14 = h;
    m_18 = w >> 1;
    m_1c = h >> 1;
    if (item->m_bc != 0) {
        m_28 = 0x11;
        return 1;
    }
    m_28 = 0x10;
    return 1;
}

// ---------------------------------------------------------------------------
// 0x153180: the slot-13 build path (non-virtual /GX builder). Allocate the owned
// +0x30 object (a CImageOwned), decode one frame into it (Build) with the parent's
// active surface format, then cache the decoded geometry (w/h, halved) and the
// descriptor's m_10/m_14 origin into the image. m_28 = 0x11 on success.
//
// The `new CImageOwned()` carries the C++ EH state machine (the [esp+0x14] try-
// level writes 0 then -1 around the ctor), which puts the /GX frame on this method.
// __thiscall, ret 8 (2 stack args).
// @early-stop
// 98.7% - all 70 instructions present and logic byte-faithful (the 0x3c new, the
// EH try-level machine, the parent-chain fmt, the geometry copy). The residual is
// (1) the reloc/EH scoring artifacts (push Unwind handler, call _RezAlloc vs
// operator new, the fs:0 __except_list writes - all reloc-masked, code bytes match)
// and (2) a 2-3 byte regalloc/scheduling wall: retail loads `a` into edx before
// completing the fmt chain (we finish the chain first), and orders the tail
// geometry copy before the fs:0 restore (we interleave). No source lever flips it
// under /O2 (tried inline vs local for both the chain and the arg). Logic complete;
// deferred to the final sweep.
// ---------------------------------------------------------------------------
RVA(0x00153180, 0xda)
i32 CImage::BuildSlot13(CImageFrameDesc* desc, void* a) {
    CImageOwned* owned = new CImageOwned();
    m_30 = owned;
    if (owned == 0) {
        return 0;
    }
    if (!owned->Build((CImageBuildDesc*)desc, (i32)a, m_0c->m_04->m_10[0x18 / 4])) {
        return 0;
    }
    i32 w = m_30->m_04;
    m_10 = w;
    i32 h = m_30->m_08;
    m_14 = h;
    m_28 = 0x11;
    m_18 = w >> 1;
    m_1c = h >> 1;
    m_20 = desc->m_10;
    m_24 = desc->m_14;
    return 1;
}

// ---------------------------------------------------------------------------
// 0x153260: the cleanup virtual (vtable slot 7). Remove the held surface (m_2c)
// from the parent collection's surface pool (m_0c->m_1c), then destroy + free the
// owned +0x30 object; both handles cleared. m_10/m_14 zeroed up front.
// ---------------------------------------------------------------------------
RVA(0x00153260, 0x41)
void CImage::FreeAll() {
    m_10 = 0;
    m_14 = 0;
    if (m_2c != 0) {
        CImageSurfacePool* pool = *(CImageSurfacePool**)((char*)m_0c + 0x1c);
        pool->RemoveItemA(m_2c);
        m_2c = 0;
    }
    CImageOwned* owned = m_30;
    if (owned != 0) {
        owned->Teardown();
        RezFree(owned);
        m_30 = 0;
    }
}

// ---------------------------------------------------------------------------
// 0x0d5e80: the (non-deleting) destructor. Stamps this class's own vtable, runs
// the cleanup virtual (FreeAll), then the base subobject dtor folds in (sets
// m_04=-1, zeroes m_08/m_0c, stamps the grand-base CObject dtor vtable). The /GX
// EH frame falls out of the non-trivial base subobject.
// @early-stop
// 94.29% - body byte-identical. Two non-steerable /GX EH-machine residuals:
// (1) the entry trylevel-0 write `mov [esp+0x10],0` and the vptr re-stamp
//     `mov [esi],&g_imageVtbl` are emitted in the opposite order vs retail's
//     stamp-first (docs/patterns/eh-dtor-vptr-stamp-vs-trylevel-order.md, ~94%);
// (2) the EH scope-table cookie `push 0x8`/Unwind@005de0e0 vs our `push 0x0`/$L303
//     (docs/patterns/gx-scoped-local-eh-frame-size.md). Both are the EH-state
//     machine's, not source-steerable. Logic complete. Deferred to the final sweep.
RVA(0x000d5e80, 0x5b)
CImage::~CImage() {
    *(void**)this = &g_imageVtbl;
    FreeAll();
    // ~CImageBase() folds here: m_04=-1; m_08=0; m_0c=0; stamp grand-base vtable.
}

// ---------------------------------------------------------------------------
// 0x153790: render a sprite frame. Resolve a clip context through a shared
// CRemusNode singleton (a function-local static, built once via MSVC5's guarded
// magic-static init), feeding it the parent collection (m_0c) and the (b,c,d)
// geometry; on success dispatch the +0x38 render virtual passing the clip context
// and the first arg. __thiscall, ret 0x10 (4 stack args).
// ---------------------------------------------------------------------------

// The shared clip/resolve singleton: a CRemusNode (vtable 0x5efbc0). Its default
// ctor (0x1549d0) + the resolve method (0x1647e0) are external engine __thiscall
// callees, modeled here as a tiny host so the magic-static init + the resolve
// call reloc-mask. The atexit dtor thunk (0x553800) is the static's cleanup.
class CRemusClip {
public:
    CRemusClip();                                                         // 0x1549d0
    i32 Resolve(void* parent, i32 z1, void* b, void* c, void* d, i32 z2); // 0x1647e0
};

// The +0x38 render virtual (0x153470), reached via the vtable. Modeled as a
// __thiscall pointer-to-member-fn in a typed vtable struct (the MSVC5-period
// idiom; raw __thiscall fn-ptrs are rejected) so the dispatch lowers to
// `mov ecx,this; call [vptr+0x38]` with callee-side cleanup.
struct CImageVtbl;
class CImageDispatch {
public:
    CImageVtbl* vptr;                            // +0x00
    void RenderImage(CRemusClip* clip, void* a); // vtbl[0x38]
};
typedef void (CImageDispatch::*RenderImageFn)(CRemusClip*, void*);
struct CImageVtbl {
    char _00[0x38 - 0x00];     // slots 0..13 (@0x00..0x34)
    RenderImageFn RenderImage; // [0x38]
};
inline void CImageDispatch::RenderImage(CRemusClip* clip, void* a) {
    (this->*(vptr->RenderImage))(clip, a);
}

RVA(0x00153790, 0x6a)
void CImage::RenderFrame(void* a, void* b, void* c, void* d) {
    static CRemusClip clip; // magic-static guard @0x6bf314, ctor 0x1549d0 + atexit
    if (clip.Resolve(m_0c, 0, b, c, d, 0)) {
        ((CImageDispatch*)this)->RenderImage(&clip, a);
    }
}

// ---------------------------------------------------------------------------
// 0x153810: render a sprite frame into a caller-supplied clip rect. Same shape as
// RenderFrame (resolve through a shared CRemusClip singleton, then dispatch the
// +0x38 render virtual) but with a second magic-static singleton (guard @0x6bf29c,
// object @0x6bf228) and an extra `rect` arg whose 4 ints are stashed into a static
// clip rect (@0x6bf28c) before the dispatch. __thiscall, ret 0x14 (5 stack args).
// ---------------------------------------------------------------------------

// The static clip rect updated each call (4 consecutive ints @0x6bf28c). File
// scope so it lands in .bss adjacent to the function's singleton; reloc-masked.
static i32 g_imageClipRect[4]; // @0x6bf28c

RVA(0x00153810, 0x95)
void CImage::RenderFrameClipped(void* a, void* b, void* c, void* rect, void* d) {
    static CRemusClip clip; // magic-static guard @0x6bf29c, ctor 0x1549d0 + atexit
    if (clip.Resolve(m_0c, 0, b, c, d, 0)) {
        if (rect != 0) {
            g_imageClipRect[0] = ((i32*)rect)[0];
            g_imageClipRect[1] = ((i32*)rect)[1];
            g_imageClipRect[2] = ((i32*)rect)[2];
            g_imageClipRect[3] = ((i32*)rect)[3];
        }
        ((CImageDispatch*)this)->RenderImage(&clip, a);
    }
}
