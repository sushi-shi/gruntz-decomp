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

// The image-source format tag (CImageSource::GetTag, a packed 3-char fourcc) that
// both Resolve and Reload dispatch on to pick the format loader index (1..4). Named
// by the literal tag bytes (low byte first); PMB/XCP are the reversed extension
// (-> BMP / PCX loaders). Same immediates as the bare literals -> matching-neutral;
// the switch key stays (u32) so the compare keeps its unsigned ja/je codegen.
enum ImageFormatTag {
    IMGTAG_PMB = 0x424d50, // "PMB" -> BMP loader (index 1)
    IMGTAG_XCP = 0x504358, // "XCP" -> PCX loader (index 2)
    IMGTAG_DIR = 0x524944, // "DIR" -> loader index 3
    IMGTAG_DIP = 0x504944, // "DIP" -> loader index 4
};

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
// (vtable slot 12): Create. The Create24 sibling without the mode args:
// allocate a surface from the parent pool's 3-arg create (CreateC @0x142560) for
// (desc, capArg, flagsArg) - where flagsArg = keyed ? g_severusCounterB : -1 and
// capArg = g_severusCounterA ? 0x800 : 0 - then cache the surface geometry (w/h,
// halved) and clear the m_20/m_24 origin. __thiscall, ret 8 (2 stack args).
// @early-stop
// 99.09% - structurally identical to the 100% sibling Create24, plus the two
// g_severusCounterA/B DIR32 reloc artifacts. The lone code residual is the geometry
// temp register: retail caches w(item->m_1c) in edx and h in ecx; cl (with one fewer
// call arg than Create24) swaps them to ecx/edx. The 3-vs-5-arg call shifts the post-
// call allocator state - not steerable from Create's own source.
// ---------------------------------------------------------------------------
RVA(0x00152e90, 0x8b)
i32 CImage::Create(CImageFrameDesc* desc, i32 keyed) {
    i32 flagsArg = (keyed != 0) ? g_severusCounterB : -1;
    i32 capArg = 0;
    if (g_severusCounterA != 0) {
        capArg = 0x800;
    }
    CImageSurfaceItem* item = m_0c->m_1c->CreateC((i32)desc, capArg, flagsArg);
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
    } else {
        m_28 = 0x10;
    }
    m_20 = 0;
    m_24 = 0;
    return 1;
}

// ---------------------------------------------------------------------------
// (vtable slot 11): Resolve. Read the source's 3-char format tag, map it
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
        case IMGTAG_PMB: // BMP
            index = 1;
            break;
        case IMGTAG_XCP: // PCX
            index = 2;
            break;
        case IMGTAG_DIR:
            index = 3;
            break;
        case IMGTAG_DIP:
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
// (vtable slot 10): LoadDispatch. The format-index switch (1..4); index 4
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
// (vtable slot 9): Create24. Allocate a surface from the parent pool's
// Create24 variant (CreateB @0x1423c0) for (desc, mode, NULL, capArg, flagsArg) -
// where flagsArg = keyed ? g_severusCounterB : -1 and capArg = g_severusCounterA ?
// 0x800 : 0 - then cache the surface geometry (w/h, halved) and clear the m_20/m_24
// origin. __thiscall, ret 0xc (3 stack args).
// ---------------------------------------------------------------------------
RVA(0x001530e0, 0x92)
i32 CImage::Create24(CImageFrameDesc* desc, i32 mode, i32 keyed) {
    i32 flagsArg = (keyed != 0) ? g_severusCounterB : -1;
    i32 capArg = 0;
    if (g_severusCounterA != 0) {
        capArg = 0x800;
    }
    CImageSurfaceItem* item = m_0c->m_1c->CreateB((i32)desc, mode, 0, capArg, flagsArg);
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
    } else {
        m_28 = 0x10;
    }
    m_20 = 0;
    m_24 = 0;
    return 1;
}

// ---------------------------------------------------------------------------
// The slot-13 build path (non-virtual /GX builder). Allocate the owned
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
// The cleanup virtual (vtable slot 7). Remove the held surface (m_2c)
// from the parent collection's surface pool (m_0c->m_1c), then destroy + free the
// owned +0x30 object; both handles cleared. m_10/m_14 zeroed up front.
// ---------------------------------------------------------------------------
RVA(0x00153260, 0x41)
void CImage::FreeAll() {
    m_10 = 0;
    m_14 = 0;
    if (m_2c != 0) {
        m_0c->m_1c->RemoveItemA(m_2c);
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
// CopyFrom - clone another image's surface into this one. Fails unless
// both images own a held surface (m_2c) and neither carries an owned object (m_30),
// and the two geometries match (m_10/m_14). On a match it Prepares the held surface
// (Prepare(0)) then Blts the other image's surface into it, returning whether the
// blit succeeded. __thiscall, ret 4.
//
// The five reject paths each emit the bare `xor eax,eax; pop; pop; ret 4` epilogue
// inline -> SIBLING guards, not nesting (docs/patterns/redundant-sibling-guard-retest).
// ---------------------------------------------------------------------------
RVA(0x001532b0, 0x80)
i32 CImage::CopyFrom(CImage* other) {
    if (other == 0) {
        return 0;
    }
    if (other->m_30 != 0) {
        return 0;
    }
    if (m_2c == 0) {
        return 0;
    }
    if (m_30 != 0) {
        return 0;
    }
    if (m_10 != other->m_10) {
        return 0;
    }
    if (m_14 != other->m_14) {
        return 0;
    }
    m_2c->Prepare(0);
    i32 ok = m_2c->Blt(other->m_2c);
    return ok != 0;
}

// ---------------------------------------------------------------------------
// 0x153380 (vtable slot 13): Reload - refresh the held surface from its parse
// source. If there is no held surface, succeed. If the surface's source sub-object
// (m_2c->m_08) reports still-clean (IsClean, vtbl[0x60]), succeed. Else if it still
// has a live source descriptor (HasSource, vtbl[0x6c]), re-run FreeAll + Resolve
// (the slot-7 + slot-11 virtuals) with the caller's args. Otherwise parse the new
// source directly: read its 3-char tag (BMP=1/CPX=2/RID=3/PID=4), prime it, then
// rebuild the held surface via the pool's Reload (0x13e550) with the resolved
// index/source/severus-flag. __thiscall, ret 8.
//
// The tag switch is the same unsigned binary-search tree as Resolve (key u32, the
// ja/je fourcc compares); docs/patterns/switch-key-unsigned-ja-vs-jg.md.
//
// The FreeAll (slot 7, +0x1c) + Resolve (slot 11, +0x2c) re-runs go through the
// vtable (`call [this->vptr+N]`), so they are dispatched via a typed vtable view -
// the manual-vtable CImage's own FreeAll/Resolve are not declared virtual.
// ---------------------------------------------------------------------------
struct CImageReloadVtbl;
class CImageReloadDispatch {
public:
    CImageReloadVtbl* vptr;                   // +0x00
    void FreeAllV();                          // vtbl[0x1c] slot 7
    i32 ResolveV(CImageSource* src, i32 arg); // vtbl[0x2c] slot 11
};
typedef void (CImageReloadDispatch::*FreeAllVFn)();
typedef i32 (CImageReloadDispatch::*ResolveVFn)(CImageSource*, i32);
struct CImageReloadVtbl {
    char _00[0x1c - 0x00];
    FreeAllVFn FreeAll; // [0x1c]
    char _20[0x2c - 0x20];
    ResolveVFn Resolve; // [0x2c]
};
inline void CImageReloadDispatch::FreeAllV() {
    (this->*(vptr->FreeAll))();
}
inline i32 CImageReloadDispatch::ResolveV(CImageSource* src, i32 arg) {
    return (this->*(vptr->Resolve))(src, arg);
}

RVA(0x00153380, 0xeb)
i32 CImage::Reload(CImageSource* src, i32 arg) {
    if (m_2c == 0) {
        return 1;
    }
    CImageSurfaceSrc* s = m_2c->m_08;
    if (s != 0) {
        if (s->vptr->IsClean(s) == 0) {
            return 1;
        }
    }
    s = m_2c->m_08;
    if (s->vptr->HasSource(s) != 0) {
        ((CImageReloadDispatch*)this)->FreeAllV();
        return ((CImageReloadDispatch*)this)->ResolveV(src, arg);
    }

    i32 index;
    switch ((u32)src->GetTag()) {
        case IMGTAG_PMB: // BMP
            index = 1;
            break;
        case IMGTAG_XCP: // PCX
            index = 2;
            break;
        case IMGTAG_DIR:
            index = 3;
            break;
        case IMGTAG_DIP:
            index = 4;
            break;
        default:
            return 0;
    }
    i32 resolved = src->Resolve();
    if (resolved == 0) {
        return 0;
    }
    if (src->m_0c == 0) {
        return 0;
    }
    return m_2c->Reload((void*)m_0c->m_1c, resolved, index, (void*)src->m_0c, g_severusCounterB);
}

// ---------------------------------------------------------------------------
// The virtual destructor. MSVC stamps this class's own vtable
// (??_7CImage, catalog auto-named) at entry, runs the cleanup virtual (FreeAll),
// then the base subobject dtor folds in (sets m_04=-1, zeroes m_08/m_0c, stamps
// the grand-base dtor vtable). Both vptr stamps are compiler-implicit now, so they
// land in the retail "stamp-first" order. The /GX EH frame falls out of the
// non-trivial CImageBase subobject.
RVA(0x000d5e80, 0x5b)
CImage::~CImage() {
    FreeAll();
    m_04 = -1; // base-field resets (precede the folded ~CImageBase grand stamp)
    m_08 = 0;
    m_0c = 0;
    // ~CImageBase() folds here: emits only the grand-base vptr re-stamp.
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

// ===========================================================================
// Class-metadata annotations (EOF-hosted: CImage.h is included by several /O2
// Image TUs whose leaf decoders are byte-exact-sensitive, so keep the completeness
// typedefs after the last function). VTBL skips (logged): CImageBase's vtable is
// the shared grand-base 0x5e8cb4 (already ?g_severusWorkerDtorVtbl); CImageSurfaceItem
// / CImageSource are flagged [virtual] only via their polymorphic Gruntz defs, not
// this view. CImage itself is RTTI-catalogued (??_7CImage@@ @0x5eaa2c).
// ===========================================================================
// --- CImage.h header classes ---
SIZE(CImageBase, 0x10); // polymorphic base (CImage fields start at +0x10)
SIZE_UNKNOWN(CImageSurfaceSrc);
SIZE_UNKNOWN(CImageSurfaceSrcVtbl);
SIZE(CImageSurfaceItem, 0xc0); // RE'd CPoolItemA item size (0xc0)
SIZE_UNKNOWN(CImageSurfacePool);
SIZE(CImageFrameRebuildDesc, 0x20); // 8-dword by-value frame descriptor
SIZE(CImageOwned, 0x3c);            // operator new(0x3c) owned object
SIZE_UNKNOWN(CImageBuildDesc);
SIZE_UNKNOWN(CDDrawSurfaceDesc);
SIZE(BlitRect, 0x10); // {left,top,right,bottom} RECT
SIZE_UNKNOWN(CBlitClipOwner);
SIZE_UNKNOWN(CImageParent);
SIZE_UNKNOWN(CImageFrameDesc);
SIZE_UNKNOWN(CImageSource);
SIZE_UNKNOWN(CImage); // RTTI CImage (partial model; RTTI-vtable catalogued)
// --- CImage.cpp local dispatch/vtbl views ---
SIZE_UNKNOWN(CImageLoadDispatch);
SIZE_UNKNOWN(CImageLoadVtbl);
SIZE_UNKNOWN(CImageReloadDispatch);
SIZE_UNKNOWN(CImageReloadVtbl);
SIZE_UNKNOWN(CRemusClip);
SIZE_UNKNOWN(CImageDispatch);
SIZE_UNKNOWN(CImageVtbl);
