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
// CLightFxRender::Shape1  (0x0a3dc0, 2143B) - a ~2KB FPU/shift shape generator
// that fills the +0x4c buffer with computed 16-bit colors via the screen RGB
// shift globals (0x683ea0..0x683eb4). DEFERRED to the final sweep (big, float-
// heavy, leaf-first redo). The RVA stub binds ?Shape1@ to 0xa3dc0 so BuildShape's
// dispatch call reloc-masks.
// ===========================================================================
// @early-stop
// deferred: ~2KB 16-bit-color shape generator (FPU + RGB-shift mask), one of 8.
RVA(0x000a3dc0, 0x85f)
i32 CLightFxRender::Shape1() {
    return 0;
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
// deferred: ~2KB shape generator (newly discovered via the BuildShape switch).
RVA(0x000a4890, 0x852)
i32 CLightFxRender::Shape2() {
    return 0;
}
// @early-stop
// deferred: ~2KB shape generator (target).
RVA(0x000a5310, 0x855)
i32 CLightFxRender::Shape3() {
    return 0;
}
// @early-stop
// deferred: ~2KB shape generator (target).
RVA(0x000a5d90, 0x825)
i32 CLightFxRender::Shape4() {
    return 0;
}
// @early-stop
// deferred: ~2KB shape generator (newly discovered via the BuildShape switch).
RVA(0x000a67d0, 0x864)
i32 CLightFxRender::Shape5() {
    return 0;
}
// @early-stop
// deferred: ~2KB shape generator (target).
RVA(0x000a7260, 0x8c0)
i32 CLightFxRender::Shape6() {
    return 0;
}
// @early-stop
// deferred: ~2KB shape generator (newly discovered via the BuildShape switch).
RVA(0x000a7d50, 0x94f)
i32 CLightFxRender::Shape7() {
    return 0;
}
// @early-stop
// deferred: ~2KB shape generator (newly discovered via the BuildShape switch).
RVA(0x000a8900, 0x926)
i32 CLightFxRender::Shape8() {
    return 0;
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
