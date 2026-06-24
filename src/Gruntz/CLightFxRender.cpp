// CLightFxRender.cpp - software light/glow effect renderer (tracer placeholder
// ClassUnknown_68), a non-polymorphic helper in the lighting module. Methods in
// ascending retail-RVA order. The class owns an embedded 16-bit pixel buffer at
// +0x4c that the shape generators fill; it allocates a DirectDraw work surface
// via the surface manager and blits the computed light. Engine callees (Lock /
// BltEx / surface alloc) are reloc-masked (no body). Intra-class calls go through
// the ILT thunks in retail; objdiff masks the rel32 displacement.
//
// Field names are placeholders (m_<hexoffset>); only offsets + code bytes are
// load-bearing. See <Gruntz/CLightFxRender.h> for the layout.
#include <Gruntz/CLightFxRender.h>

#include <rva.h>

// The DirectDraw work surface (this+0x10, callers pass it at +0x2c).
struct LfxSurface {
    char m_pad[0x20];
    i32 m_20; // +0x20 bytes-per-pixel / x stride
    char m_pad2[0x8c];
    i32 m_b0;         // +0xb0 pitch
    i32 Init0(i32 a); // engine method on a freshly-alloc'd surface (FUN_0053edb0)
    u16* Lock(i32 a); // CDirSurf::Lock (FUN_0053e6d0) -> locked pixel base
};

// The unlock interface (LfxBorderCtx::m_8): a COM-like object whose vtable slot
// 0x20 releases the surface lock. Modeled as a typed vtable so the dispatch
// (mov eax,[iface]; call [eax+0x80]) falls out with no cast.
struct LfxUnlockIface {
    struct Vtbl {
        void* s0[0x20];
        void(__stdcall* Unlock)(LfxUnlockIface*, i32); // slot 0x20 -> [vtbl+0x80]
    }* vtbl;
};

// The border-draw context (DrawBorder's 2nd arg): +0x08 the unlock interface,
// +0x2c the locked work surface (pitch / stride / Lock).
struct LfxBorderCtx {
    char m_pad0[0x8];
    LfxUnlockIface* m_08; // +0x08 unlock interface
    char m_pad0c[0x2c - 0xc];
    LfxSurface* m_2c; // +0x2c the work surface
};

// The surface pool the manager points at (m_0c->m_1c). __thiscall Free/Alloc map
// to the engine routines FUN_00542160 / FUN_00542e60 (reloc-masked, no body).
struct LfxSurfPool {
    void Free(LfxSurface* s);                                // FUN_00542160
    LfxSurface* Alloc(i32 w, i32 h, i32 a3, i32 a4, i32 a5); // FUN_00542e60
};

// The surface manager (this+0xc): +0x1c holds the surface pool.
struct LfxSurfMgr {
    char m_pad[0x1c];
    LfxSurfPool* m_1c; // +0x1c the surface pool
};

// The surface-info source (this+0x8): +0xc / +0x10 are the requested w / h.
struct LfxSurfInfo {
    char m_pad[0xc];
    i32 m_0c; // +0xc width
    i32 m_10; // +0x10 height
};

// The draw context the apply-paths hand the pixel coords to (FUN_004d5f00).
struct LfxDrawCtx {
    void DrawAt(i32 px, i32 py); // FUN_004d5f00 (__thiscall on m_00->m_2c)
};

// The render manager (this+0x00, set by Init). Init copies +0x68/+0x70/+0x30 into
// this+0x4/0x8/0xc; the apply paths draw through +0x2c.
struct LfxMgr {
    char m_pad0[0x2c];
    LfxDrawCtx* m_2c; // +0x2c draw context
    void* m_30;       // +0x30 surface manager  -> this+0xc
    char m_pad34[0x68 - 0x34];
    void* m_68; // +0x68 surface info     -> this+0x4
    char m_pad6c[0x70 - 0x6c];
    void* m_70; // +0x70                  -> this+0x8
};

// A surface the global-apply path blits through (g_gameReg->m_68). The 7-arg
// engine entry FUN_00479520 is reloc-masked (no body).
struct LfxBlitTarget {
    void Blit(i32 px, i32 py, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7); // FUN_00479520
};

// g_gameReg singleton (*0x64556c); only the +0x68 surface slot is read.
struct CGameReg {
    char m_pad[0x68];
    LfxBlitTarget* m_68;
};
extern CGameReg* g_gameReg;

// The live screen RGB-format shift table (VA 0x683ea0..0x683eb4 = RVA 0x283ea0..):
// per channel a right-shift (8-bit -> channel width) then a left-shift into the
// channel's slot. B sits at bit 0, so it has no left-shift.
// See docs/patterns/rgb-pack-variable-shift.md.
DATA(0x00283ea0)
extern i32 g_rUp; // red   up-shift (into position)
DATA(0x00283ea4)
extern i32 g_gUp; // green up-shift
DATA(0x00283eac)
extern i32 g_rDown; // red   down-shift (scale 8-bit -> width)
DATA(0x00283eb0)
extern i32 g_gDown; // green down-shift
DATA(0x00283eb4)
extern i32 g_bDown; // blue  down-shift

// Pack an 8-bit (r,g,b) constant triple into a screen-native 16-bit pixel.
static inline u16 Pack(i32 r, i32 g, i32 b) {
    return (u16)(((r >> g_rDown) << g_rUp) | ((g >> g_gDown) << g_gUp) | (b >> g_bDown));
}

// ===========================================================================
// CLightFxRender::Init  (0x0a32c0)  - bind the manager, validate, zero state.
// ===========================================================================
RVA(0x000a32c0, 0x72)
i32 CLightFxRender::Init(LfxMgr* mgr, i32 arg2) {
    if (mgr == 0) {
        return 0;
    }
    m_00 = mgr;
    m_04 = mgr->m_68;
    m_08 = mgr->m_70;
    m_0c = (LfxSurfMgr*)mgr->m_30;
    m_434 = arg2;
    m_44 = 1;
    m_438 = 0;
    if (!AllocSurface()) {
        return 0;
    }
    m_34 = 0;
    m_38 = 0;
    m_3c = 0;
    m_40 = 0;
    m_24 = 0;
    m_28 = 0;
    m_2c = 0;
    m_30 = 0;
    return 1;
}

// ===========================================================================
// CLightFxRender::Ctor  (0x0a3360)  - zero core pointers + sizes.
// ===========================================================================
RVA(0x000a3360, 0x29)
void CLightFxRender::Ctor() {
    FreeSurface();
    m_00 = 0;
    m_04 = 0;
    m_08 = 0;
    m_0c = 0;
    m_10 = 0;
    m_48 = 0;
    m_434 = 0;
    m_438 = 0;
}

// ===========================================================================
// CLightFxRender::FreeSurface  (0x0a33a0)
// ===========================================================================
RVA(0x000a33a0, 0x23)
void CLightFxRender::FreeSurface() {
    if (m_0c != 0 && m_10 != 0) {
        m_0c->m_1c->Free(m_10);
        m_10 = 0;
    }
}

// ===========================================================================
// CLightFxRender::AllocSurface  (0x0a33e0)
// ===========================================================================
// @early-stop
// 99.68% - regalloc tail: the two adjacent loads info->m_0c / info->m_10 land in
// swapped registers vs retail (edx<->eax); values + push order identical.
RVA(0x000a33e0, 0x55)
i32 CLightFxRender::AllocSurface() {
    if (m_08 == 0) {
        return 0;
    }
    if (m_0c == 0) {
        return 0;
    }
    FreeSurface();
    LfxSurfInfo* info = (LfxSurfInfo*)m_08;
    LfxSurfMgr* mgr = m_0c;
    m_10 = mgr->m_1c->Alloc(info->m_0c, info->m_10, 0, 0, -1);
    if (m_10 == 0) {
        return 0;
    }
    m_10->Init0(0);
    return 1;
}

// ===========================================================================
// CLightFxRender::DrawBorder  (0x0a3b50)  - lock the ctx work surface, paint the
// four edges of `r` with `color`, unlock. `this`/ecx is unused. The top/bottom
// edges are contiguous word runs (left..right); the left/right edges step a
// column down each row. The fill at each edge is the inlined u16-memset idiom.
// ===========================================================================
// @early-stop
// 70% - regalloc/frame wall (zero-register-pinning family): the body is byte-
// identical in shape and the u16-fill idiom matches exactly, but retail pins the
// surface in ebp / base in ebx (callee-saved, live across Lock) and frames with
// push ebx/ebp, while MSVC for this source pins surface=esi/base=eax and spills
// `this` via `push ecx`, a 1-instr phase shift that renames registers through
// the whole body. Logic 100% correct.
RVA(0x000a3b50, 0xfa)
void CLightFxRender::DrawBorder(LfxRect* r, LfxBorderCtx* ctx, i32 color) {
    LfxSurface* surf = ctx->m_2c;
    u16* base = surf->Lock(0);
    if (base == 0) {
        return;
    }
    i32 w = r->right - r->left + 1;
    // Top edge.
    u16* tp = (u16*)((char*)base + r->top * surf->m_20 + r->left * surf->m_b0);
    for (i32 t = 0; t < w; t++) {
        tp[t] = (u16)color;
    }
    // Bottom edge.
    u16* bp = (u16*)((char*)base + r->bottom * surf->m_20 + r->left * surf->m_b0);
    for (i32 b = 0; b < w; b++) {
        bp[b] = (u16)color;
    }
    // Left / right edges (column step = m_20 per row).
    i32 h = r->bottom - r->top + 1;
    char* lp = (char*)base + r->left * surf->m_b0 + r->top * surf->m_20;
    char* rp = (char*)base + r->right * surf->m_b0 + r->top * surf->m_20;
    for (i32 v = 0; v < h; v++) {
        *(u16*)lp = (u16)color;
        *(u16*)rp = (u16)color;
        lp += surf->m_20;
        rp += surf->m_20;
    }
    ctx->m_08->vtbl->Unlock(ctx->m_08, 0);
}

// ===========================================================================
// CLightFxRender::BuildShape  (0x0a3c90)  - zero the +0x4c pixel buffer, then
// dispatch one of the 8 shape generators by (shape - 1). Any generator returning
// nonzero (or shape <= 0) clears m_438 and returns 1; a generator returning 0
// fails (return 0). shape > 8 is rejected up front.
// ===========================================================================
// @early-stop
// 86% - dispatch + all 8 case bodies are byte-identical; residual is the 8
// reloc-masked call displacements + a 1-instr head schedule swap (lea edi before
// mov ecx,0xfa vs retail's mov-ecx-first). Logic 100% correct.
RVA(0x000a3c90, 0xc7)
i32 CLightFxRender::BuildShape(i32 shape) {
    if (shape > 8) {
        return 0;
    }
    {
        u32* p = (u32*)m_buf;
        for (i32 i = 0; i < 0xfa; i++) {
            p[i] = 0;
        }
    }
    switch (shape - 1) {
        case 0:
            if (!Shape1()) {
                return 0;
            }
            break;
        case 1:
            if (!Shape2()) {
                return 0;
            }
            break;
        case 2:
            if (!Shape3()) {
                return 0;
            }
            break;
        case 3:
            if (!Shape4()) {
                return 0;
            }
            break;
        case 4:
            if (!Shape5()) {
                return 0;
            }
            break;
        case 5:
            if (!Shape6()) {
                return 0;
            }
            break;
        case 6:
            if (!Shape7()) {
                return 0;
            }
            break;
        case 7:
            if (!Shape8()) {
                return 0;
            }
            break;
    }
    m_438 = 0;
    return 1;
}

// ===========================================================================
// CLightFxRender::Shape1  (0x0a3dc0, 2143B) - the first of 8 shape generators.
// Pre-computes ~22 screen-native 16-bit colors from 8-bit (R,G,B) triples via the
// RGB shift table (Pack, see docs/patterns/rgb-pack-variable-shift.md), paints a
// fixed icon into the +0x4c pixel buffer (direct word runs + FillSpan spans).
// ===========================================================================
// @early-stop
// ~70% wall (docs/patterns/rgb-pack-variable-shift.md): complete, correct body -
// the per-channel `(c>>down)<<up` packs and the FillSpan spans match in shape, but
// the /O2 optimizer CSEs the 5 shared shift globals and fuses partial channel
// results across the ~22 colors, scheduling them into a register/stack-slot
// arrangement no uniform `Pack()` source reproduces byte-for-byte. Same scheduling
// wall as Shape2..Shape8 (all ~2KB); deferred to the final sweep for a leaf-first
// redo once the pack scheduling is steerable.
RVA(0x000a3dc0, 0x85f)
i32 CLightFxRender::Shape1() {
    u16* buf = m_buf;
    i32 i;
    u16 c00 = Pack(0x4f, 0x14, 0x01);
    u16 c01 = Pack(0x00, 0x37, 0x00);
    u16 c02 = Pack(0x00, 0x00, 0x13);
    u16 c03 = Pack(0x63, 0x00, 0x00);
    u16 c04 = Pack(0x5c, 0x0d, 0x06);
    u16 c05 = Pack(0x10, 0x28, 0x71);
    u16 c06 = Pack(0x26, 0x62, 0x00);
    u16 c07 = Pack(0x00, 0x00, 0x00);
    u16 c08 = Pack(0x20, 0x20, 0x20);
    u16 c09 = Pack(0x78, 0x78, 0x5f);
    u16 c10 = Pack(0x64, 0x64, 0x64);
    u16 c11 = Pack(0xff, 0xd9, 0x00);
    u16 c12 = Pack(0x00, 0xd2, 0x47);
    u16 c13 = Pack(0x00, 0x00, 0xff);
    u16 c14 = Pack(0xa1, 0x2b, 0x00);
    u16 c15 = Pack(0x45, 0x00, 0x00);
    u16 c16 = Pack(0x00, 0x7c, 0x00);
    u16 c17 = Pack(0x00, 0xff, 0x45);
    u16 c18 = Pack(0x00, 0x26, 0x26);
    u16 c19 = Pack(0x00, 0x92, 0x2b);
    u16 c20 = Pack(0xd7, 0xd7, 0xd7);
    u16 c21 = Pack(0x37, 0x00, 0x00);
    u16 c22 = Pack(0x00, 0x00, 0x37);
    u16 c23 = Pack(0xb4, 0x30, 0x30);
    buf[1] = c00;
    buf[2] = c00;
    buf[3] = c00;
    buf[4] = c00;
    buf[5] = c00;
    buf[6] = c00;
    buf[7] = c00;
    buf[8] = c00;
    for (i = 17; i < 37; i++) {
        buf[i] = c00;
    }
    buf[90] = c00;
    buf[195] = c00;
    buf[196] = c00;
    buf[199] = c00;
    buf[301] = c00;
    buf[9] = c00;
    buf[10] = c00;
    buf[11] = c00;
    buf[12] = c00;
    buf[13] = c00;
    buf[14] = c00;
    buf[15] = c00;
    buf[16] = c00;
    buf[91] = c03;
    buf[197] = c03;
    buf[198] = c03;
    for (i = 40; i < 74; i++) {
        buf[i] = c04;
    }
    for (i = 270; i < 282; i++) {
        buf[i] = c04;
    }
    for (i = 104; i < 116; i++) {
        buf[i] = c08;
    }
    buf[120] = c01;
    buf[121] = c01;
    buf[122] = c01;
    buf[123] = c01;
    for (i = 128; i < 140; i++) {
        buf[i] = c08;
    }
    for (i = 144; i < 156; i++) {
        buf[i] = c06;
    }
    buf[160] = c01;
    buf[161] = c01;
    buf[162] = c01;
    buf[163] = c01;
    for (i = 168; i < 180; i++) {
        buf[i] = c06;
    }
    buf[157] = c01;
    buf[158] = c01;
    buf[165] = c01;
    buf[166] = c01;
    buf[258] = c05;
    buf[264] = c05;
    buf[117] = c05;
    buf[118] = c05;
    FillSpan(0x7d, 0x7e, c07);
    buf[260] = c07;
    buf[266] = c07;
    FillSpan(0x11a, 0x11d, c09);
    buf[257] = c09;
    buf[259] = c09;
    FillSpan(0x105, 0x107, c09);
    buf[265] = c09;
    FillSpan(0x4d, 0x54, c10);
    FillSpan(0x11e, 0x126, c10);
    FillSpan(0xc9, 0xd1, c11);
    FillSpan(0xdd, 0xe0, c12);
    FillSpan(0xf1, 0xf2, c00);
    FillSpan(0xed, 0xee, c14);
    FillSpan(0xff, 0x100, c14);
    FillSpan(0xe1, 0xe4, c16);
    FillSpan(0xe5, 0xe8, c17);
    FillSpan(0xfb, 0xfc, c17);
    FillSpan(0xe9, 0xec, c18);
    FillSpan(0xfd, 0xfe, c18);
    FillSpan(0xef, 0xf0, c19);
    FillSpan(0xf7, 0xf8, c19);
    FillSpan(0xd9, 0xda, c20);
    FillSpan(0xf9, 0xfa, c20);
    FillSpan(0xf3, 0xf6, c22);
    FillSpan(0x12e, 0x143, c12);
    FillSpan(0xd5, 0xd6, c23);
    FillSpan(0xd7, 0xd8, c21);
    FillSpan(0x5c, 0x5f, c00);
    return 1;
}

// ===========================================================================
// CLightFxRender::FillSpan  (0x0a4840)  - fill a 16-bit span in the +0x4c buffer.
// ===========================================================================
RVA(0x000a4840, 0x32)
void CLightFxRender::FillSpan(u32 x1, u32 x2, u16 color) {
    if (x1 > x2) {
        return;
    }
    for (u32 i = x1; i <= x2; i++) {
        m_buf[i] = color;
    }
}

// ===========================================================================
// CLightFxRender shape generators 2-8 (0x0a4890, 0x0a5310, 0x0a5d90, 0x0a67d0,
// 0x0a7260, 0x0a7d50, 0x0a8900) - the remaining seven ~2KB 16-bit-color shape
// builders the BuildShape switch dispatches to. All DEFERRED to the final sweep
// (big, FPU/RGB-shift-mask heavy). The RVA stubs bind the ?ShapeN@ symbols so
// BuildShape's dispatch calls reloc-mask. Shape3/4/6 are in this TU's target set;
// Shape2/5/7/8 are newly-discovered methods of the same class (the switch proves
// membership - they are the case 2/5/7/8 generators).
// ===========================================================================
// @early-stop
// ~70% wall (docs/patterns/rgb-pack-variable-shift.md) - complete, correct body
// (Pack colors + buffer stores + FillSpan spans); the /O2 CSE+scheduling of the
// ~22 shared-global packs is the same wall as Shape1.
RVA(0x000a4890, 0x852)
i32 CLightFxRender::Shape2() {
    u16* buf = m_buf;
    i32 i;
    u16 c00 = Pack(0x89, 0x6e, 0x58);
    u16 c01 = Pack(0xd7, 0x00, 0x00);
    u16 c02 = Pack(0x00, 0xe5, 0xfa);
    u16 c03 = Pack(0x10, 0x28, 0x71);
    u16 c04 = Pack(0x26, 0x62, 0x00);
    u16 c05 = Pack(0x00, 0x00, 0x00);
    u16 c06 = Pack(0x20, 0x20, 0x20);
    u16 c07 = Pack(0x49, 0x65, 0x84);
    u16 c08 = Pack(0xff, 0xd9, 0x13);
    u16 c09 = Pack(0x00, 0xd2, 0x47);
    u16 c10 = Pack(0x00, 0x00, 0xff);
    u16 c11 = Pack(0xa1, 0x2b, 0x00);
    u16 c12 = Pack(0x45, 0x00, 0x00);
    u16 c13 = Pack(0x00, 0x7c, 0x00);
    u16 c14 = Pack(0x00, 0xff, 0x45);
    u16 c15 = Pack(0x00, 0x26, 0x26);
    u16 c16 = Pack(0x00, 0x92, 0x2b);
    u16 c17 = Pack(0x00, 0xd7, 0xd7);
    u16 c18 = Pack(0x37, 0x00, 0x00);
    u16 c19 = Pack(0x00, 0x37, 0x37);
    u16 c20 = Pack(0xb4, 0x30, 0x30);
    u16 c21 = Pack(0xa0, 0x00, 0x00);
    buf[1] = c21;
    buf[2] = c21;
    buf[3] = c21;
    buf[4] = c21;
    buf[5] = c21;
    buf[6] = c21;
    buf[7] = c21;
    buf[8] = c21;
    for (i = 17; i < 37; i++) {
        buf[i] = c21;
    }
    buf[90] = c00;
    buf[195] = c21;
    buf[196] = c21;
    buf[199] = c00;
    buf[301] = c00;
    buf[9] = c00;
    buf[10] = c00;
    buf[11] = c00;
    buf[12] = c00;
    buf[13] = c00;
    buf[14] = c00;
    buf[15] = c00;
    buf[16] = c00;
    buf[91] = c00;
    buf[197] = c00;
    buf[198] = c00;
    for (i = 40; i < 74; i++) {
        buf[i] = c02;
    }
    for (i = 270; i < 282; i++) {
        buf[i] = c02;
    }
    for (i = 104; i < 116; i++) {
        buf[i] = c06;
    }
    buf[120] = c10;
    buf[121] = c10;
    buf[122] = c10;
    buf[123] = c10;
    for (i = 128; i < 140; i++) {
        buf[i] = c06;
    }
    for (i = 144; i < 156; i++) {
        buf[i] = c04;
    }
    buf[160] = c10;
    buf[161] = c10;
    buf[162] = c10;
    buf[163] = c10;
    for (i = 168; i < 180; i++) {
        buf[i] = c04;
    }
    buf[157] = c10;
    buf[158] = c10;
    buf[165] = c10;
    buf[166] = c10;
    buf[258] = c03;
    buf[264] = c03;
    buf[117] = c03;
    buf[118] = c03;
    FillSpan(0x7d, 0x7e, c05);
    buf[260] = c05;
    buf[266] = c05;
    FillSpan(0x11a, 0x11d, c00);
    buf[257] = c00;
    buf[259] = c00;
    FillSpan(0x105, 0x107, c00);
    buf[265] = c00;
    FillSpan(0x4d, 0x54, c07);
    FillSpan(0x11e, 0x126, c07);
    FillSpan(0xc9, 0xd1, c08);
    FillSpan(0xdd, 0xe0, c09);
    FillSpan(0xf1, 0xf2, c00);
    FillSpan(0xed, 0xee, c11);
    FillSpan(0xff, 0x100, c11);
    FillSpan(0xe1, 0xe4, c13);
    FillSpan(0xe5, 0xe8, c14);
    FillSpan(0xfb, 0xfc, c14);
    FillSpan(0xe9, 0xec, c15);
    FillSpan(0xfd, 0xfe, c15);
    FillSpan(0xef, 0xf0, c16);
    FillSpan(0xf7, 0xf8, c16);
    FillSpan(0xd9, 0xda, c17);
    FillSpan(0xf9, 0xfa, c17);
    FillSpan(0xf3, 0xf6, c19);
    FillSpan(0x12e, 0x143, c09);
    FillSpan(0xd5, 0xd6, c20);
    FillSpan(0xd7, 0xd8, c18);
    FillSpan(0x5c, 0x5f, c00);
    return 1;
}
// @early-stop
// ~70% wall (docs/patterns/rgb-pack-variable-shift.md) - complete, correct body
// (Pack colors + buffer stores + FillSpan spans); the /O2 CSE+scheduling of the
// ~22 shared-global packs is the same wall as Shape1.
RVA(0x000a5310, 0x855)
i32 CLightFxRender::Shape3() {
    u16* buf = m_buf;
    i32 i;
    u16 c00 = Pack(0x4e, 0x78, 0x1c);
    u16 c01 = Pack(0x23, 0x23, 0x23);
    u16 c02 = Pack(0x00, 0x37, 0x00);
    u16 c03 = Pack(0x00, 0x00, 0x0f);
    u16 c04 = Pack(0x24, 0x00, 0x00);
    u16 c05 = Pack(0x10, 0x28, 0x71);
    u16 c06 = Pack(0x26, 0x62, 0x00);
    u16 c07 = Pack(0xb4, 0x00, 0x00);
    u16 c08 = Pack(0x00, 0x3d, 0x0b);
    u16 c09 = Pack(0x64, 0x0c, 0x03);
    u16 c10 = Pack(0xb0, 0x85, 0x1f);
    u16 c11 = Pack(0x59, 0x17, 0x00);
    u16 c12 = Pack(0xff, 0xd9, 0x13);
    u16 c13 = Pack(0x00, 0xd2, 0x47);
    u16 c14 = Pack(0x00, 0x00, 0xff);
    u16 c15 = Pack(0xa1, 0x2b, 0x00);
    u16 c16 = Pack(0x45, 0x00, 0x00);
    u16 c17 = Pack(0x00, 0x7c, 0x00);
    u16 c18 = Pack(0x00, 0xff, 0x45);
    u16 c19 = Pack(0x00, 0x26, 0x26);
    u16 c20 = Pack(0x00, 0x92, 0x2b);
    u16 c21 = Pack(0x37, 0xd7, 0xd7);
    u16 c22 = Pack(0x00, 0x00, 0x37);
    u16 c23 = Pack(0x00, 0x61, 0x39);
    u16 c24 = Pack(0x00, 0x30, 0x30);
    buf[1] = c00;
    buf[2] = c00;
    buf[3] = c00;
    buf[4] = c00;
    buf[5] = c00;
    buf[6] = c00;
    buf[7] = c00;
    buf[8] = c00;
    for (i = 17; i < 37; i++) {
        buf[i] = c00;
    }
    buf[90] = c00;
    buf[195] = c00;
    buf[196] = c00;
    buf[199] = c00;
    buf[301] = c00;
    buf[9] = c00;
    buf[10] = c00;
    buf[11] = c00;
    buf[12] = c00;
    buf[13] = c00;
    buf[14] = c00;
    buf[15] = c00;
    buf[16] = c00;
    buf[91] = c01;
    buf[197] = c01;
    buf[198] = c01;
    for (i = 40; i < 74; i++) {
        buf[i] = c04;
    }
    for (i = 270; i < 282; i++) {
        buf[i] = c04;
    }
    for (i = 104; i < 116; i++) {
        buf[i] = c09;
    }
    buf[120] = c02;
    buf[121] = c02;
    buf[122] = c02;
    buf[123] = c02;
    for (i = 128; i < 140; i++) {
        buf[i] = c09;
    }
    for (i = 144; i < 156; i++) {
        buf[i] = c06;
    }
    buf[160] = c02;
    buf[161] = c02;
    buf[162] = c02;
    buf[163] = c02;
    for (i = 168; i < 180; i++) {
        buf[i] = c06;
    }
    buf[157] = c02;
    buf[158] = c02;
    buf[165] = c02;
    buf[166] = c02;
    buf[258] = c05;
    buf[264] = c05;
    buf[117] = c05;
    buf[118] = c05;
    FillSpan(0x7d, 0x7e, c08);
    buf[260] = c08;
    buf[266] = c08;
    FillSpan(0x11a, 0x11d, c10);
    buf[257] = c10;
    buf[259] = c10;
    FillSpan(0x105, 0x107, c10);
    buf[265] = c10;
    FillSpan(0x4d, 0x54, c11);
    FillSpan(0x11e, 0x126, c11);
    FillSpan(0xc9, 0xd1, c12);
    FillSpan(0xdd, 0xe0, c13);
    FillSpan(0xf1, 0xf2, c00);
    FillSpan(0xed, 0xee, c15);
    FillSpan(0xff, 0x100, c15);
    FillSpan(0xe1, 0xe4, c17);
    FillSpan(0xe5, 0xe8, c18);
    FillSpan(0xfb, 0xfc, c18);
    FillSpan(0xe9, 0xec, c19);
    FillSpan(0xfd, 0xfe, c19);
    FillSpan(0xef, 0xf0, c20);
    FillSpan(0xf7, 0xf8, c20);
    FillSpan(0xd9, 0xda, c20);
    FillSpan(0xf9, 0xfa, c20);
    FillSpan(0xf3, 0xf6, c22);
    FillSpan(0x12e, 0x143, c23);
    FillSpan(0xd5, 0xd6, c24);
    FillSpan(0xd7, 0xd8, c21);
    FillSpan(0x5c, 0x5f, c00);
    return 1;
}
// @early-stop
// ~70% wall (docs/patterns/rgb-pack-variable-shift.md) - complete, correct body
// (Pack colors + buffer stores + FillSpan spans); the /O2 CSE+scheduling of the
// ~22 shared-global packs is the same wall as Shape1.
RVA(0x000a5d90, 0x825)
i32 CLightFxRender::Shape4() {
    u16* buf = m_buf;
    i32 i;
    u16 c00 = Pack(0x00, 0xfd, 0xfd);
    u16 c01 = Pack(0x00, 0xc1, 0xa7);
    u16 c02 = Pack(0x47, 0x65, 0xf1);
    u16 c03 = Pack(0x00, 0x00, 0x00);
    u16 c04 = Pack(0x01, 0x00, 0x5e);
    u16 c05 = Pack(0x0d, 0x20, 0xbe);
    u16 c06 = Pack(0x00, 0x00, 0x00);
    u16 c07 = Pack(0x45, 0x00, 0x00);
    u16 c08 = Pack(0x00, 0x2e, 0x0d);
    u16 c09 = Pack(0xff, 0xc5, 0xe0);
    u16 c10 = Pack(0x00, 0xd9, 0x13);
    u16 c11 = Pack(0x00, 0xd2, 0x47);
    u16 c12 = Pack(0x00, 0x00, 0xff);
    u16 c13 = Pack(0xa1, 0x2b, 0x00);
    u16 c14 = Pack(0x00, 0x7c, 0x00);
    u16 c15 = Pack(0x00, 0xff, 0x45);
    u16 c16 = Pack(0x00, 0x26, 0x26);
    u16 c17 = Pack(0xd7, 0xd7, 0xd7);
    u16 c18 = Pack(0x37, 0x00, 0x00);
    u16 c19 = Pack(0x00, 0x37, 0x37);
    u16 c20 = Pack(0xb4, 0x61, 0x39);
    u16 c21 = Pack(0x00, 0x30, 0x30);
    u16 c22 = Pack(0xa0, 0x00, 0x00);
    u16 c23 = Pack(0x00, 0x00, 0x00);
    buf[1] = c22;
    buf[2] = c22;
    buf[3] = c22;
    buf[4] = c22;
    buf[5] = c22;
    buf[6] = c22;
    buf[7] = c22;
    buf[8] = c22;
    for (i = 17; i < 37; i++) {
        buf[i] = c22;
    }
    buf[90] = c00;
    buf[195] = c22;
    buf[196] = c22;
    buf[199] = c00;
    buf[301] = c00;
    buf[9] = c00;
    buf[10] = c00;
    buf[11] = c00;
    buf[12] = c00;
    buf[13] = c00;
    buf[14] = c00;
    buf[15] = c00;
    buf[16] = c00;
    buf[91] = c01;
    buf[197] = c01;
    buf[198] = c01;
    for (i = 40; i < 74; i++) {
        buf[i] = c01;
    }
    for (i = 270; i < 282; i++) {
        buf[i] = c01;
    }
    for (i = 104; i < 116; i++) {
        buf[i] = c01;
    }
    buf[120] = c01;
    buf[121] = c01;
    buf[122] = c01;
    buf[123] = c01;
    for (i = 128; i < 140; i++) {
        buf[i] = c01;
    }
    for (i = 144; i < 156; i++) {
        buf[i] = c02;
    }
    buf[160] = c07;
    buf[161] = c07;
    buf[162] = c07;
    buf[163] = c07;
    for (i = 168; i < 180; i++) {
        buf[i] = c02;
    }
    buf[157] = c07;
    buf[158] = c07;
    buf[165] = c07;
    buf[166] = c07;
    buf[258] = c10;
    buf[264] = c10;
    buf[117] = c10;
    buf[118] = c10;
    FillSpan(0x7d, 0x7e, c06);
    FillSpan(0x11a, 0x11d, c08);
    buf[257] = c08;
    buf[259] = c08;
    FillSpan(0x105, 0x107, c08);
    buf[265] = c08;
    FillSpan(0x4d, 0x54, c09);
    FillSpan(0x11e, 0x126, c09);
    FillSpan(0xc9, 0xd1, c10);
    FillSpan(0xdd, 0xe0, c11);
    FillSpan(0xf1, 0xf2, c00);
    FillSpan(0xed, 0xee, c13);
    FillSpan(0xff, 0x100, c13);
    FillSpan(0xe1, 0xe4, c14);
    FillSpan(0xe5, 0xe8, c15);
    FillSpan(0xfb, 0xfc, c15);
    FillSpan(0xe9, 0xec, c16);
    FillSpan(0xfd, 0xfe, c16);
    FillSpan(0xef, 0xf0, c16);
    FillSpan(0xf7, 0xf8, c16);
    FillSpan(0xd9, 0xda, c17);
    FillSpan(0xf9, 0xfa, c17);
    FillSpan(0xf3, 0xf6, c19);
    FillSpan(0x12e, 0x143, c20);
    FillSpan(0xd5, 0xd6, c21);
    FillSpan(0xd7, 0xd8, c23);
    FillSpan(0x5c, 0x5f, c00);
    return 1;
}
// @early-stop
// ~70% wall (docs/patterns/rgb-pack-variable-shift.md) - complete, correct body
// (Pack colors + buffer stores + FillSpan spans); the /O2 CSE+scheduling of the
// ~22 shared-global packs is the same wall as Shape1.
RVA(0x000a67d0, 0x864)
i32 CLightFxRender::Shape5() {
    u16* buf = m_buf;
    i32 i;
    u16 c00 = Pack(0x3c, 0x0e, 0x15);
    u16 c01 = Pack(0x68, 0x08, 0x07);
    u16 c02 = Pack(0xf2, 0xfe, 0x9b);
    u16 c03 = Pack(0x23, 0x7d, 0xb5);
    u16 c04 = Pack(0x1b, 0x3c, 0x64);
    u16 c05 = Pack(0x00, 0x00, 0x00);
    u16 c06 = Pack(0x00, 0x00, 0x00);
    u16 c07 = Pack(0x6e, 0x19, 0x46);
    u16 c08 = Pack(0xfc, 0xfc, 0xfc);
    u16 c09 = Pack(0xff, 0xd9, 0x13);
    u16 c10 = Pack(0x00, 0xd2, 0x47);
    u16 c11 = Pack(0x00, 0x00, 0xff);
    u16 c12 = Pack(0xa1, 0x2b, 0x00);
    u16 c13 = Pack(0x45, 0x00, 0x00);
    u16 c14 = Pack(0x00, 0x7c, 0x00);
    u16 c15 = Pack(0x00, 0xff, 0x45);
    u16 c16 = Pack(0x00, 0x26, 0x26);
    u16 c17 = Pack(0x00, 0x92, 0x2b);
    u16 c18 = Pack(0xd7, 0xd7, 0xd7);
    u16 c19 = Pack(0x37, 0x00, 0x00);
    u16 c20 = Pack(0x00, 0x37, 0x37);
    u16 c21 = Pack(0xb4, 0x30, 0x30);
    u16 c22 = Pack(0xa0, 0xa0, 0x00);
    buf[1] = c00;
    buf[2] = c00;
    buf[3] = c00;
    buf[4] = c00;
    buf[5] = c00;
    buf[6] = c00;
    buf[7] = c00;
    buf[8] = c00;
    for (i = 17; i < 37; i++) {
        buf[i] = c00;
    }
    buf[90] = c00;
    buf[195] = c00;
    buf[196] = c00;
    buf[199] = c00;
    buf[301] = c00;
    buf[9] = c00;
    buf[10] = c00;
    buf[11] = c00;
    buf[12] = c00;
    buf[13] = c00;
    buf[14] = c00;
    buf[15] = c00;
    buf[16] = c00;
    buf[91] = c01;
    buf[197] = c01;
    buf[198] = c01;
    for (i = 39; i < 75; i++) {
        buf[i] = c01;
    }
    for (i = 270; i < 282; i++) {
        buf[i] = c01;
    }
    for (i = 102; i < 114; i++) {
        buf[i] = c01;
    }
    buf[116] = c01;
    buf[117] = c01;
    buf[118] = c01;
    buf[119] = c01;
    buf[120] = c01;
    buf[121] = c01;
    for (i = 124; i < 138; i++) {
        buf[i] = c01;
    }
    for (i = 144; i < 156; i++) {
        buf[i] = c04;
    }
    buf[159] = c11;
    buf[160] = c11;
    buf[161] = c11;
    buf[162] = c11;
    buf[163] = c11;
    for (i = 168; i < 180; i++) {
        buf[i] = c04;
    }
    buf[157] = c11;
    buf[158] = c11;
    buf[165] = c11;
    buf[166] = c11;
    buf[258] = c03;
    buf[264] = c03;
    buf[114] = c03;
    buf[115] = c03;
    FillSpan(0x7a, 0x7b, c06);
    FillSpan(0x11a, 0x11d, c07);
    buf[257] = c07;
    buf[259] = c07;
    FillSpan(0x105, 0x107, c07);
    buf[265] = c07;
    FillSpan(0x4d, 0x54, c08);
    FillSpan(0x11e, 0x126, c08);
    FillSpan(0xc9, 0xd1, c09);
    FillSpan(0xdd, 0xe0, c10);
    FillSpan(0xf1, 0xf2, c00);
    FillSpan(0xed, 0xee, c12);
    FillSpan(0xff, 0x100, c12);
    FillSpan(0xe1, 0xe4, c14);
    FillSpan(0xe5, 0xe8, c15);
    FillSpan(0xfb, 0xfc, c15);
    FillSpan(0xe9, 0xec, c16);
    FillSpan(0xfd, 0xfe, c16);
    FillSpan(0xef, 0xf0, c17);
    FillSpan(0xf7, 0xf8, c17);
    FillSpan(0xd9, 0xda, c18);
    FillSpan(0xf9, 0xfa, c18);
    FillSpan(0xf3, 0xf6, c20);
    FillSpan(0x12e, 0x143, c10);
    FillSpan(0xd5, 0xd6, c21);
    FillSpan(0xd7, 0xd8, c22);
    FillSpan(0x5c, 0x5f, c00);
    return 1;
}
// @early-stop
// ~70% wall (docs/patterns/rgb-pack-variable-shift.md) - complete, correct body
// (Pack colors + buffer stores + FillSpan spans); the /O2 CSE+scheduling of the
// ~22 shared-global packs is the same wall as Shape1.
RVA(0x000a7260, 0x8c0)
i32 CLightFxRender::Shape6() {
    u16* buf = m_buf;
    i32 i;
    u16 c00 = Pack(0x85, 0x73, 0x6f);
    u16 c01 = Pack(0x28, 0x25, 0xc8);
    u16 c02 = Pack(0xaf, 0xad, 0xc4);
    u16 c03 = Pack(0x32, 0x99, 0xae);
    u16 c04 = Pack(0x10, 0x77, 0x8c);
    u16 c05 = Pack(0x35, 0x38, 0x42);
    u16 c06 = Pack(0x4d, 0x50, 0x5a);
    u16 c07 = Pack(0xa7, 0x83, 0x48);
    u16 c08 = Pack(0xfb, 0xfb, 0xfb);
    u16 c09 = Pack(0xff, 0xd9, 0x13);
    u16 c10 = Pack(0x00, 0xd2, 0x47);
    u16 c11 = Pack(0x00, 0x00, 0xff);
    u16 c12 = Pack(0xa1, 0x2b, 0x00);
    u16 c13 = Pack(0x45, 0x00, 0x00);
    u16 c14 = Pack(0x00, 0x7c, 0x00);
    u16 c15 = Pack(0x00, 0xff, 0x45);
    u16 c16 = Pack(0x00, 0x26, 0x26);
    u16 c17 = Pack(0x00, 0x92, 0x2b);
    u16 c18 = Pack(0xd7, 0xd7, 0xd7);
    u16 c19 = Pack(0x37, 0x00, 0x00);
    u16 c20 = Pack(0x00, 0x37, 0x37);
    u16 c21 = Pack(0xb4, 0x30, 0x30);
    buf[1] = c00;
    buf[2] = c00;
    buf[3] = c00;
    buf[4] = c00;
    buf[5] = c00;
    buf[6] = c00;
    buf[7] = c00;
    buf[8] = c00;
    for (i = 17; i < 37; i++) {
        buf[i] = c00;
    }
    buf[90] = c00;
    buf[195] = c00;
    buf[196] = c00;
    buf[199] = c00;
    buf[301] = c00;
    buf[9] = c00;
    buf[10] = c00;
    buf[11] = c00;
    buf[12] = c00;
    buf[13] = c00;
    buf[14] = c00;
    buf[15] = c00;
    buf[16] = c00;
    buf[91] = c01;
    buf[197] = c01;
    buf[198] = c01;
    for (i = 39; i < 75; i++) {
        buf[i] = c02;
    }
    for (i = 270; i < 282; i++) {
        buf[i] = c02;
    }
    for (i = 102; i < 114; i++) {
        buf[i] = c06;
    }
    buf[116] = c11;
    buf[117] = c11;
    buf[118] = c11;
    buf[119] = c11;
    buf[120] = c11;
    buf[121] = c11;
    for (i = 124; i < 138; i++) {
        buf[i] = c06;
    }
    for (i = 144; i < 156; i++) {
        buf[i] = c04;
    }
    buf[159] = c11;
    buf[160] = c11;
    buf[161] = c11;
    buf[162] = c11;
    buf[163] = c11;
    for (i = 168; i < 180; i++) {
        buf[i] = c04;
    }
    buf[157] = c11;
    buf[158] = c11;
    buf[165] = c11;
    buf[166] = c11;
    buf[258] = c03;
    buf[264] = c03;
    buf[114] = c11;
    buf[115] = c11;
    buf[122] = c11;
    buf[123] = c11;
    buf[260] = c09;
    buf[266] = c09;
    FillSpan(0x11a, 0x11d, c07);
    buf[257] = c07;
    buf[259] = c07;
    FillSpan(0x105, 0x107, c07);
    buf[265] = c07;
    FillSpan(0x4d, 0x54, c08);
    FillSpan(0x11e, 0x126, c08);
    FillSpan(0xc9, 0xd1, c09);
    FillSpan(0xdd, 0xe0, c10);
    FillSpan(0xf1, 0xf2, c00);
    FillSpan(0xed, 0xee, c12);
    FillSpan(0xff, 0x100, c12);
    FillSpan(0xe1, 0xe4, c14);
    FillSpan(0xe5, 0xe8, c15);
    FillSpan(0xfb, 0xfc, c15);
    FillSpan(0xe9, 0xec, c16);
    FillSpan(0xfd, 0xfe, c16);
    FillSpan(0xef, 0xf0, c17);
    FillSpan(0xf7, 0xf8, c17);
    FillSpan(0xd9, 0xda, c18);
    FillSpan(0xf9, 0xfa, c18);
    FillSpan(0xf3, 0xf6, c20);
    FillSpan(0x12e, 0x143, c10);
    FillSpan(0xd5, 0xd6, c21);
    FillSpan(0xd7, 0xd8, c19);
    buf[259] = c20;
    buf[265] = c00;
    FillSpan(0x5c, 0x5f, c00);
    return 1;
}
// @early-stop
// ~70% wall (docs/patterns/rgb-pack-variable-shift.md) - complete, correct body
// (Pack colors + buffer stores + FillSpan spans); the /O2 CSE+scheduling of the
// ~22 shared-global packs is the same wall as Shape1.
RVA(0x000a7d50, 0x94f)
i32 CLightFxRender::Shape7() {
    u16* buf = m_buf;
    i32 i;
    u16 c00 = Pack(0x40, 0x40, 0x00);
    u16 c01 = Pack(0x00, 0x7a, 0x2f);
    u16 c02 = Pack(0x68, 0x71, 0x7c);
    u16 c03 = Pack(0x6a, 0xb9, 0xff);
    u16 c04 = Pack(0x43, 0x85, 0x00);
    u16 c05 = Pack(0xc3, 0xc0, 0x73);
    u16 c06 = Pack(0x86, 0x8b, 0x7f);
    u16 c07 = Pack(0x78, 0x78, 0x5f);
    u16 c08 = Pack(0x81, 0x55, 0xf6);
    u16 c09 = Pack(0xff, 0x00, 0x00);
    u16 c10 = Pack(0x00, 0xd9, 0x00);
    u16 c11 = Pack(0x00, 0xd2, 0x47);
    u16 c12 = Pack(0xa1, 0x00, 0x00);
    u16 c13 = Pack(0x00, 0x2b, 0x00);
    u16 c14 = Pack(0x45, 0x00, 0x00);
    u16 c15 = Pack(0x00, 0x7c, 0x00);
    u16 c16 = Pack(0x00, 0xff, 0x45);
    u16 c17 = Pack(0x00, 0x26, 0x26);
    u16 c18 = Pack(0x00, 0x92, 0x2b);
    u16 c19 = Pack(0xd7, 0xd7, 0xd7);
    u16 c20 = Pack(0x37, 0x00, 0x00);
    u16 c21 = Pack(0x00, 0x37, 0x37);
    u16 c22 = Pack(0xb4, 0x61, 0x39);
    u16 c23 = Pack(0x00, 0x30, 0x30);
    u16 c24 = Pack(0xa0, 0xa0, 0x27);
    u16 c25 = Pack(0x00, 0x00, 0x00);
    buf[1] = c00;
    buf[2] = c00;
    buf[3] = c00;
    buf[4] = c00;
    buf[5] = c00;
    buf[6] = c00;
    buf[7] = c00;
    buf[8] = c00;
    for (i = 17; i < 37; i++) {
        buf[i] = c00;
    }
    buf[90] = c00;
    buf[195] = c00;
    buf[196] = c00;
    buf[199] = c00;
    buf[301] = c00;
    buf[9] = c00;
    buf[10] = c00;
    buf[11] = c00;
    buf[12] = c00;
    buf[13] = c00;
    buf[14] = c00;
    buf[15] = c00;
    buf[16] = c00;
    buf[91] = c01;
    buf[197] = c01;
    buf[198] = c01;
    for (i = 39; i < 75; i++) {
        buf[i] = c02;
    }
    for (i = 270; i < 282; i++) {
        buf[i] = c02;
    }
    for (i = 102; i < 114; i++) {
        buf[i] = c06;
    }
    buf[116] = c12;
    buf[117] = c12;
    buf[118] = c12;
    buf[119] = c12;
    buf[120] = c12;
    buf[121] = c12;
    for (i = 124; i < 138; i++) {
        buf[i] = c06;
    }
    for (i = 144; i < 156; i++) {
        buf[i] = c04;
    }
    buf[159] = c12;
    buf[160] = c12;
    buf[161] = c12;
    buf[162] = c12;
    buf[163] = c12;
    for (i = 168; i < 180; i++) {
        buf[i] = c04;
    }
    buf[157] = c12;
    buf[158] = c12;
    buf[165] = c12;
    buf[166] = c12;
    buf[258] = c03;
    buf[264] = c03;
    buf[114] = c12;
    buf[115] = c12;
    buf[122] = c12;
    buf[123] = c12;
    buf[260] = c10;
    buf[266] = c10;
    FillSpan(0x11a, 0x11d, c07);
    buf[257] = c07;
    buf[259] = c07;
    FillSpan(0x105, 0x107, c07);
    buf[265] = c07;
    FillSpan(0x4d, 0x54, c08);
    FillSpan(0x11e, 0x126, c08);
    FillSpan(0xc9, 0xd1, c10);
    FillSpan(0xdd, 0xe0, c11);
    FillSpan(0xf1, 0xf2, c00);
    FillSpan(0xed, 0xee, c13);
    FillSpan(0xff, 0x100, c13);
    FillSpan(0xe1, 0xe4, c15);
    FillSpan(0xe5, 0xe8, c16);
    FillSpan(0xfb, 0xfc, c16);
    FillSpan(0xe9, 0xec, c17);
    FillSpan(0xfd, 0xfe, c17);
    FillSpan(0xef, 0xf0, c18);
    FillSpan(0xf7, 0xf8, c18);
    FillSpan(0xd9, 0xda, c19);
    FillSpan(0xf9, 0xfa, c19);
    FillSpan(0xf3, 0xf6, c21);
    FillSpan(0x12e, 0x143, c22);
    FillSpan(0xd5, 0xd6, c23);
    FillSpan(0xd7, 0xd8, c24);
    FillSpan(0x105, 0x106, c12);
    buf[263] = c25;
    buf[265] = c25;
    FillSpan(0x5c, 0x5f, c00);
    return 1;
}
// @early-stop
// ~70% wall (docs/patterns/rgb-pack-variable-shift.md) - complete, correct body
// (Pack colors + buffer stores + FillSpan spans); the /O2 CSE+scheduling of the
// ~22 shared-global packs is the same wall as Shape1.
RVA(0x000a8900, 0x926)
i32 CLightFxRender::Shape8() {
    u16* buf = m_buf;
    i32 i;
    u16 c00 = Pack(0x5e, 0x5e, 0x5e);
    u16 c01 = Pack(0x28, 0x28, 0x28);
    u16 c02 = Pack(0x96, 0x96, 0x96);
    u16 c03 = Pack(0x30, 0x64, 0x6f);
    u16 c04 = Pack(0x33, 0x50, 0x57);
    u16 c05 = Pack(0x00, 0x00, 0x00);
    u16 c06 = Pack(0x00, 0x00, 0x00);
    u16 c07 = Pack(0x00, 0x00, 0x00);
    u16 c08 = Pack(0x78, 0x78, 0x5f);
    u16 c09 = Pack(0x94, 0xa7, 0xbd);
    u16 c10 = Pack(0xff, 0xd9, 0x13);
    u16 c11 = Pack(0x00, 0xd2, 0x00);
    u16 c12 = Pack(0x00, 0x00, 0x47);
    u16 c13 = Pack(0x00, 0x00, 0xff);
    u16 c14 = Pack(0xa1, 0x2b, 0x00);
    u16 c15 = Pack(0x45, 0x00, 0x00);
    u16 c16 = Pack(0x00, 0x7c, 0x00);
    u16 c17 = Pack(0x00, 0xff, 0x45);
    u16 c18 = Pack(0x00, 0x00, 0x26);
    u16 c19 = Pack(0x00, 0x26, 0x00);
    u16 c20 = Pack(0x00, 0x92, 0x2b);
    u16 c21 = Pack(0xd7, 0xd7, 0xd7);
    u16 c22 = Pack(0x37, 0x00, 0x00);
    u16 c23 = Pack(0x00, 0x37, 0x37);
    u16 c24 = Pack(0xb4, 0x61, 0x39);
    u16 c25 = Pack(0x00, 0x30, 0x30);
    u16 c26 = Pack(0x12, 0xa0, 0x18);
    u16 c27 = Pack(0x00, 0x72, 0x00);
    buf[1] = c05;
    buf[2] = c05;
    buf[3] = c05;
    buf[4] = c05;
    buf[5] = c05;
    buf[6] = c05;
    buf[7] = c05;
    buf[8] = c05;
    for (i = 17; i < 37; i++) {
        buf[i] = c05;
    }
    buf[90] = c00;
    buf[195] = c05;
    buf[196] = c05;
    buf[199] = c00;
    buf[301] = c00;
    buf[9] = c06;
    buf[10] = c06;
    buf[11] = c06;
    buf[12] = c06;
    buf[13] = c06;
    buf[14] = c06;
    buf[15] = c06;
    buf[16] = c06;
    buf[91] = c01;
    buf[197] = c01;
    buf[198] = c01;
    for (i = 39; i < 75; i++) {
        buf[i] = c01;
    }
    for (i = 270; i < 282; i++) {
        buf[i] = c01;
    }
    for (i = 102; i < 114; i++) {
        buf[i] = c01;
    }
    buf[116] = c01;
    buf[117] = c01;
    buf[118] = c01;
    buf[119] = c01;
    buf[120] = c01;
    buf[121] = c01;
    for (i = 124; i < 138; i++) {
        buf[i] = c01;
    }
    for (i = 144; i < 156; i++) {
        buf[i] = c04;
    }
    buf[159] = c18;
    buf[160] = c18;
    buf[161] = c18;
    buf[162] = c18;
    buf[163] = c18;
    for (i = 168; i < 180; i++) {
        buf[i] = c04;
    }
    buf[157] = c18;
    buf[158] = c18;
    buf[165] = c18;
    buf[166] = c18;
    buf[258] = c03;
    buf[264] = c03;
    buf[114] = c03;
    buf[115] = c03;
    buf[122] = c03;
    buf[123] = c03;
    buf[260] = c07;
    buf[266] = c07;
    FillSpan(0x11a, 0x11d, c08);
    buf[257] = c08;
    buf[259] = c08;
    FillSpan(0x105, 0x107, c08);
    buf[265] = c08;
    FillSpan(0x4d, 0x54, c09);
    FillSpan(0x11e, 0x126, c09);
    FillSpan(0xc9, 0xd1, c10);
    FillSpan(0xdd, 0xe0, c12);
    FillSpan(0xf1, 0xf2, c00);
    FillSpan(0xed, 0xee, c14);
    FillSpan(0xff, 0x100, c14);
    FillSpan(0xe1, 0xe4, c16);
    FillSpan(0xe5, 0xe8, c17);
    FillSpan(0xfb, 0xfc, c17);
    FillSpan(0xe9, 0xec, c19);
    FillSpan(0xfd, 0xfe, c19);
    FillSpan(0xef, 0xf0, c20);
    FillSpan(0xf7, 0xf8, c20);
    FillSpan(0xd9, 0xda, c21);
    FillSpan(0xf9, 0xfa, c21);
    FillSpan(0xf3, 0xf6, c23);
    FillSpan(0x12e, 0x143, c24);
    FillSpan(0xd5, 0xd6, c25);
    FillSpan(0xd7, 0xd8, c23);
    buf[257] = c26;
    buf[259] = c26;
    FillSpan(0x105, 0x106, c27);
    buf[263] = c06;
    buf[265] = c06;
    FillSpan(0x5c, 0x5f, c00);
    return 1;
}

// ===========================================================================
// CLightFxRender::ApplyA  (0x0a9480)  - clamp (x,y) to a tile cell; if the mgr
// has a draw context, convert the cell to a pixel center (cell*32+16) and draw.
// Always latches m_48 = 1.
// ===========================================================================
RVA(0x000a9480, 0x5c)
i32 CLightFxRender::ApplyA(i32, i32 x, i32 y) {
    i32 cell[2];
    if (!ClampRect(x, y, cell, 0x20)) {
        return 0;
    }
    LfxDrawCtx* ctx = m_00->m_2c;
    if (ctx != 0) {
        ctx->DrawAt(cell[0] * 32 + 16, cell[1] * 32 + 16);
    }
    m_48 = 1;
    return 1;
}

// ===========================================================================
// CLightFxRender::ClearHandle  (0x0a9500)
// ===========================================================================
RVA(0x000a9500, 0x16)
i32 CLightFxRender::ClearHandle(i32, i32, i32) {
    if (m_48 != 0) {
        m_48 = 0;
    }
    return 1;
}

// ===========================================================================
// CLightFxRender::ApplyGlobal  (0x0a9550)  - clamp (x,y) to a tile cell, then
// blit the effect through the global g_gameReg surface (+0x68).
// ===========================================================================
RVA(0x000a9550, 0x5b)
i32 CLightFxRender::ApplyGlobal(i32, i32 x, i32 y) {
    i32 cell[2];
    if (!ClampRect(x, y, cell, 0x20)) {
        return 0;
    }
    g_gameReg->m_68->Blit(cell[0] * 32 + 16, cell[1] * 32 + 16, 0, 0, 0, 0, 1);
    return 1;
}

// ===========================================================================
// CLightFxRender::ApplyB  (0x0a95d0)  - like ApplyA, but only if m_48 is latched.
// ===========================================================================
RVA(0x000a95d0, 0x69)
i32 CLightFxRender::ApplyB(i32, i32 x, i32 y) {
    if (m_48 == 0) {
        return 0;
    }
    i32 cell[2];
    if (!ClampRect(x, y, cell, 0x20)) {
        return 0;
    }
    LfxDrawCtx* ctx = m_00->m_2c;
    if (ctx != 0) {
        ctx->DrawAt(cell[0] * 32 + 16, cell[1] * 32 + 16);
    }
    return 1;
}

// ===========================================================================
// CLightFxRender::ClampRect  (0x0a9660)  - validate (x,y) is in the source rect,
// snap toward the screen-rect edges within 'margin', re-validate against the
// screen rect, and emit the (x,y) -> tile-cell pair into out[0]/out[1].
// ===========================================================================
// @early-stop
// 88% - regalloc wall (docs/patterns/zero-register-pinning.md family): the body
// is structurally byte-identical, but MSVC pins x in eax / y in esi where retail
// pins x in edx / y in eax - a pervasive register rename through every instr.
RVA(0x000a9660, 0xca)
i32 CLightFxRender::ClampRect(i32 x, i32 y, i32* out, i32 margin) {
    if (x < m_24 || x > m_2c || y < m_28 || y > m_30) {
        return 0;
    }
    if (margin > 0) {
        if (x < m_34 && m_34 - x <= margin) {
            x = m_34;
        }
        if (x > m_3c && x - m_3c <= margin) {
            x = m_3c;
        }
        if (y < m_38 && m_38 - y <= margin) {
            y = m_38;
        }
        if (y > m_40 && y - m_40 <= margin) {
            y = m_40;
        }
    }
    if (x < m_34 || x > m_3c || y < m_38 || y > m_40) {
        return 0;
    }
    out[0] = (x - m_34) / m_44;
    out[1] = (y - m_38) / m_44;
    return 1;
}
