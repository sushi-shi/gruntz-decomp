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
