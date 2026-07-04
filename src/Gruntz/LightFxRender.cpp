// LightFxRender.cpp - software light/glow effect renderer (tracer placeholder
// tomalla-68), a non-polymorphic helper in the lighting module. Methods in
// ascending retail-RVA order. The class owns an embedded 16-bit pixel buffer at
// +0x4c that the shape generators fill; it allocates a DirectDraw work surface
// via the surface manager and blits the computed light. Engine callees (Lock /
// BltEx / surface alloc) are reloc-masked (no body). Intra-class calls go through
// the ILT thunks in retail; objdiff masks the rel32 displacement.
//
// Field names are placeholders (m_<hexoffset>); only offsets + code bytes are
// load-bearing. See <Gruntz/LightFxRender.h> for the layout.
#include <DDrawMgr/DDSurface.h> // IDirectDrawSurface (the held surface's Unlock, slot 32)
#include <Win32.h>              // windows.h base types (ddraw.h needs them first)
#include <ddraw.h>              // real IDirectDrawSurface dispatch (Unlock)
#include <Gruntz/LightFxRender.h>

#include <Gruntz/GameRegistry.h> // the g_gameReg singleton (0x24556c) canonical view
#include <rva.h>

// The held surface (LfxBorderCtx::m_08 / LfxSurface::m_08) is the real
// IDirectDrawSurface COM interface: `s->m_08->Unlock(0)` dispatches through slot 32
// (+0x80). The canonical IDirectDrawSurface lives in <DDrawMgr/DDSurface.h>.

// Forward declares so LfxMgr / the class members can hold typed pointers to the
// tile-descriptor bank and the tile grid (full layouts defined further down).
struct LfxTileBank;
struct LfxGrid;

// The DirectDraw work surface (this+0x10, callers pass it at +0x2c).
//   +0x08 unlock interface (Resize's own unlock path)
//   +0x18 / +0x1c the tile/zoom pixel dims (idiv divisors in Resize/ComputeRect)
//   +0x20 row stride (per y) / +0xb0 column stride (per x), in bytes
struct LfxSurface {
    char m_pad[0x8];
    IDirectDrawSurface* m_08; // +0x08 held DirectDraw surface (Unlock via slot 32)
    char m_pad0c[0x18 - 0xc];
    i32 m_18; // +0x18 surface height-in-tiles / divisor
    i32 m_1c; // +0x1c surface width-in-tiles / divisor
    i32 m_20; // +0x20 row stride (per scanline)
    char m_pad24[0xb0 - 0x24];
    i32 m_b0;         // +0xb0 column stride (per pixel)
    i32 Init0(i32 a); // engine method on a freshly-alloc'd surface (FUN_0053edb0)
    u16* Lock(i32 a); // CDirSurf::Lock (FUN_0053e6d0) -> locked pixel base
    // CDirSurf::BltEx (FUN_0053eef0) - stretch-blit `src` into the dest rect.
    i32 BltEx(void* destRect, LfxSurface* src, i32 a3, i32 a4, i32 a5);
};

// The border-draw context (DrawBorder's 2nd arg): +0x08 the unlock interface,
// +0x2c the locked work surface (pitch / stride / Lock).
struct LfxBorderCtx {
    char m_pad0[0x8];
    IDirectDrawSurface* m_08; // +0x08 held DirectDraw surface (Unlock via slot 32)
    char m_pad0c[0x2c - 0xc];
    LfxSurface* m_2c; // +0x2c the work surface
};

// The surface pool the manager points at (m_0c->m_1c). __thiscall Free/Alloc map
// to the engine routines FUN_00542160 / FUN_00542e60 (reloc-masked, no body).
struct LfxSurfPool {
    void Free(LfxSurface* s);                                // FUN_00542160
    LfxSurface* Alloc(i32 w, i32 h, i32 a3, i32 a4, i32 a5); // FUN_00542e60
};

// The world-rect holder (LfxView::m_5c): +0x40 is the live world RECT (4 ints,
// in world pixels; >>5 converts to tile units).
struct LfxWorldRect {
    char m_pad[0x40];
    LfxRect m_40; // +0x40 world rect (world pixels)
};

// The viewport/world-state object (LfxSurfMgr::m_24): +0x5c points at the holder.
struct LfxView {
    char m_pad[0x5c];
    LfxWorldRect* m_5c; // +0x5c -> the world-rect holder
};

// The surface manager (this+0xc): +0x1c holds the surface pool, +0x24 the view.
struct LfxSurfMgr {
    char m_pad[0x1c];
    LfxSurfPool* m_1c; // +0x1c the surface pool
    char m_pad20[0x24 - 0x20];
    LfxView* m_24; // +0x24 the view/world-state object
};

// The draw context the apply-paths hand the pixel coords to (FUN_004d5f00).
struct LfxDrawCtx {
    void DrawAt(i32 px, i32 py); // FUN_004d5f00 (__thiscall on m_00->m_2c)
};

// The per-tile color/animation node returned by the ref table (GetA). The
// renderer reads one of three 16-bit color slots (+0x8 / +0xa / +0xc) by the
// alternate-set selector. Modeled NO-body; the GetA call reloc-masks.
struct LfxColorNode {
    char m_pad[0x8];
    u16 m_08; // +0x08 color (alt==0 / default)
    u16 m_0a; // +0x0a color (alt==2)
    u16 m_0c; // +0x0c color (alt==1)
};

// The game-registry sprite/animation reference table (g_gameReg+0x74). GetA maps
// a kind index to its color node (FUN_004e2360, reloc-masked, ret 0x4).
struct LfxRefTable {
    LfxColorNode* GetA(i32 kind);
};

// The render manager (this+0x00, set by Init). Init copies +0x68/+0x70/+0x30 into
// this+0x4/0x8/0xc; the apply paths draw through +0x2c; the resize path resolves
// tile colors through +0x74 (the ref table).
struct LfxMgr {
    char m_pad0[0x2c];
    LfxDrawCtx* m_2c;    // +0x2c draw context
    LfxSurfMgr* m_world; // +0x30 surface manager     -> this+0xc
    char m_pad34[0x68 - 0x34];
    LfxTileBank* m_68; // +0x68 tile-descriptor bank -> this+0x4
    char m_pad6c[0x70 - 0x6c];
    LfxGrid* m_tileGrid; // +0x70 tile grid           -> this+0x8
    LfxRefTable* m_74;   // +0x74 sprite/animation ref table
};

// A surface the global-apply path blits through (g_gameReg->m_68). The 7-arg
// engine entry FUN_00479520 is reloc-masked (no body).
struct LfxBlitTarget {
    void Blit(i32 px, i32 py, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7); // FUN_00479520
};

// g_gameReg singleton (*0x64556c) - the canonical CGameRegistry view; only the
// +0x68 surface slot is read (cast locally to LfxBlitTarget at the deref site).
extern CGameRegistry* g_gameReg;

// One tile cell of the grid row (stride 0x1c): +0x4 the tile id (-1 = empty),
// +0xc a direct index into the +0x4c color buffer.
struct LfxCell {
    char m_pad[0x4];
    i32 m_04; // +0x04 tile id (high byte = bank, low byte = slot; -1 = empty)
    char m_pad08[0xc - 0x8];
    i32 m_0c; // +0x0c direct color-buffer index
};

// The tile grid the resize path walks (this+0x08): +0x08 the row table (one row
// pointer per y), +0x0c width, +0x10 height. Cells are stride 0x1c (LfxCell).
struct LfxGrid {
    char m_pad[0x8];
    LfxCell** m_08; // +0x08 row table  (m_08[y] -> first cell of row y)
    u32 m_0c;       // +0x0c width  (cells per row)
    u32 m_10;       // +0x10 height (rows)
};

// A tile descriptor (one slot of the LfxTileBank::m_1c table). The renderer reads:
//   +0x1d8 a flag (nonzero -> select the alternate color slot)
//   +0x870 / +0x878 a pair of 64-bit timestamps (spawn time / lifetime), compared
//          against the running game clock to decide live vs. expired
//   +0x1ec the owning area index (vs g_644c54)
//   +0x1f4 the kind passed to the ref table's GetA
struct LfxTileDesc {
    char m_pad1d8[0x1d8];
    i32 m_1d8; // +0x1d8 alt-slot select flag
    char m_pad1dc[0x1ec - 0x1dc];
    i32 m_1ec; // +0x1ec owning area index
    char m_pad1f0[0x1f4 - 0x1f0];
    i32 m_1f4; // +0x1f4 ref-table kind
    char m_pad1f8[0x870 - 0x1f8];
    i64 m_870; // +0x870 spawn timestamp (64-bit)
    i64 m_878; // +0x878 lifetime (64-bit)
};

// The tile-descriptor bank (this+0x04): a base whose +0x1c is a flat pointer table
// indexed by the packed tile id (slot + bank*15). Null entries skip the cell.
struct LfxTileBank {
    char m_pad[0x1c];
    LfxTileDesc* m_1c[1]; // +0x1c descriptor pointer table (flexible)
};

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

// Engine globals the resize repaint path reads (reloc-masked DIR32 loads):
//   g_645588 - the running game clock (low 32 bits of the engine ms counter)
//   g_644c54 - the current area / world index
//   g_645594 - a frame-quality / detail threshold (>=0x32 picks the live color)
DATA(0x00245588)
extern i32 g_645588;
DATA(0x00244c54)
extern i32 g_644c54;
DATA(0x00245594)
extern i32 g_645594;

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
    m_mgr = mgr;
    m_tileBank = mgr->m_68;
    m_grid = mgr->m_tileGrid;
    m_surfMgr = mgr->m_world;
    m_refreshInterval = arg2;
    m_scale = 1;
    m_refreshRemaining = 0;
    if (!AllocSurface()) {
        return 0;
    }
    m_dstL = 0;
    m_dstT = 0;
    m_dstR = 0;
    m_dstB = 0;
    m_srcL = 0;
    m_srcT = 0;
    m_srcR = 0;
    m_srcB = 0;
    return 1;
}

// ===========================================================================
// CLightFxRender::Ctor  (0x0a3360)  - zero core pointers + sizes.
// ===========================================================================
RVA(0x000a3360, 0x29)
void CLightFxRender::Ctor() {
    FreeSurface();
    m_mgr = 0;
    m_tileBank = 0;
    m_grid = 0;
    m_surfMgr = 0;
    m_surface = 0;
    m_handle = 0;
    m_refreshInterval = 0;
    m_refreshRemaining = 0;
}

// ===========================================================================
// CLightFxRender::FreeSurface  (0x0a33a0)
// ===========================================================================
RVA(0x000a33a0, 0x23)
void CLightFxRender::FreeSurface() {
    if (m_surfMgr != 0 && m_surface != 0) {
        m_surfMgr->m_1c->Free(m_surface);
        m_surface = 0;
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
    if (m_grid == 0) {
        return 0;
    }
    if (m_surfMgr == 0) {
        return 0;
    }
    FreeSurface();
    LfxGrid* info = m_grid;
    LfxSurfMgr* mgr = m_surfMgr;
    m_surface = mgr->m_1c->Alloc(info->m_0c, info->m_10, 0, 0, -1);
    if (m_surface == 0) {
        return 0;
    }
    m_surface->Init0(0);
    return 1;
}

// ===========================================================================
// CLightFxRender::Resize  (0x0a3460, 755B)  - the rebuild/repaint path. With
// `rebuild` clear it just decays the +0x438 remaining-count by `delta` and bails
// while still nonzero; otherwise it (re-)allocs the work surface to the grid's
// dimensions, locks it, and repaints every cell: an empty / static cell copies a
// color straight out of the +0x4c buffer, a live tile resolves its color through
// the descriptor bank + the game ref table, then unlocks.
// ===========================================================================
// @early-stop
// ~49% - zero-register-pinning WALL (docs/patterns/zero-register-pinning.md, the
// INVERSE case): the opcode skeleton matches - the two-tier grid walk, the i64
// clock compare (sub/sbb), the tile-id unpack (slot + bank*15), the GetA dispatch
// by the alt selector, and the dual buffer-copy arms are all present in shape -
// but retail pins `this` in ebp (`mov ebp,ecx`) with a 2-slot frame, while our cl
// pins the constant 0 in ebp (`xor ebp,ebp`, reused for the `=0` stores + null
// tests) and spills `this` to esi/`[esp+0x10]` with a 4-slot frame. That 1-instr
// phase shift renames every register through the 755B body. No source lever flips
// the pinning under /O2 (per the pattern). Logic 100% correct; deferred to the
// final sweep / a leaf-first redo.
RVA(0x000a3460, 0x2f3)
i32 CLightFxRender::Resize(i32 delta, i32 rebuild) {
    if (rebuild == 0) {
        if ((u32)delta < (u32)m_refreshRemaining) {
            m_refreshRemaining -= delta;
        } else {
            m_refreshRemaining = 0;
        }
        if (m_refreshRemaining != 0) {
            return 1;
        }
        m_refreshRemaining = m_refreshInterval;
    }
    m_refreshRemaining = m_refreshInterval;
    if (m_surface == 0) {
        if (!AllocSurface()) {
            return 0;
        }
    }
    LfxGrid* grid = m_grid;
    if (m_surface->m_1c != (i32)grid->m_0c || m_surface->m_18 != (i32)grid->m_10) {
        if (!AllocSurface()) {
            return 0;
        }
    }
    u16* base = m_surface->Lock(0);
    if (base == 0) {
        return 0;
    }
    LfxTileBank* bank = m_tileBank;
    for (u32 y = 0; y < grid->m_10; y++) {
        for (u32 x = 0; x < grid->m_0c; x++) {
            u16* dst = (u16*)((char*)base + y * m_surface->m_20 + x * m_surface->m_b0);
            i32 tile;
            if (x < grid->m_0c && y < grid->m_10) {
                tile = grid->m_08[y][x].m_04;
            } else {
                tile = -1;
            }
            if (tile == -1) {
                i32 idx;
                if (x < grid->m_0c && y < grid->m_10) {
                    idx = grid->m_08[y][x].m_0c;
                } else {
                    idx = 0;
                }
                if ((u32)idx >= 0x1f4) {
                    *dst = 0;
                } else {
                    *dst = m_buf[idx];
                }
                continue;
            }
            LfxTileDesc* desc = bank->m_1c[(tile & 0xff) + ((tile >> 8) & 0xff) * 15];
            if (desc == 0) {
                continue;
            }
            i32 alt = 0;
            if (desc->m_1d8 != 0) {
                alt = 1;
            }
            if ((i64)(u32)g_645588 - desc->m_870 >= desc->m_878 || desc->m_1ec != g_644c54) {
                LfxColorNode* node = m_mgr->m_74->GetA(desc->m_1f4);
                if (node == 0) {
                    *dst = 0;
                    continue;
                }
                if (alt == 0) {
                    *dst = node->m_08;
                } else if (alt == 1) {
                    *dst = node->m_0c;
                } else if (alt == 2) {
                    *dst = node->m_0a;
                } else {
                    *dst = node->m_08;
                }
                continue;
            }
            if (g_645594 >= 0x32) {
                LfxColorNode* node = m_mgr->m_74->GetA(desc->m_1f4);
                if (node == 0) {
                    *dst = 0;
                    continue;
                }
                *dst = node->m_0c;
                continue;
            }
            i32 idx;
            if (x < grid->m_0c && y < grid->m_10) {
                idx = grid->m_08[y][x].m_0c;
            } else {
                idx = 0;
            }
            if ((u32)idx >= 0x1f4) {
                *dst = 0;
            } else {
                *dst = m_buf[idx];
            }
        }
    }
    m_surface->m_08->Unlock(0);
    return 1;
}

// ===========================================================================
// CLightFxRender::ComputeRect  (0x0a3820, 398B)  - copy the source rect into the
// state block, center it, choose an integer scale (clamped to 3) from the work
// surface's tile dims, derive the centered screen rect (+0x34..+0x40), alloc/blit
// the work surface to it, then draw the border framing the live world rect.
// ===========================================================================
// @early-stop
// ~59% - idiv scale-clamp + scheduling wall: the rect copy now matches exactly
// (docs/patterns/struct-copy-via-member-pointer-lea.md, `*p = *src` through &m_24),
// and the centering (W via cdq;sub;sar;>>1), the two idiv scale divisions, the
// min/clamp-to-3, and the +0x34..+0x40 rect derivation all match in shape - but
// MSVC keeps W/H and the center/scale temporaries in a different register/stack-
// slot arrangement than retail (which holds W in ebp live across the H compute +
// first idiv, +1 frame slot). Logic 100% correct; deferred to the final sweep.
RVA(0x000a3820, 0x18e)
i32 CLightFxRender::ComputeRect(LfxBorderCtx* ctx, LfxRect* src) {
    LfxSurface* surf = m_surface;
    if (surf == 0) {
        return 0;
    }
    LfxRect* srcRect = (LfxRect*)&m_srcL;
    *srcRect = *src;
    i32 w = src->right - src->left + 1;
    i32 h = src->bottom - src->top + 1;
    i32 cx = src->left + ((w - (w >> 31)) >> 1);
    i32 cy = src->top + ((h - (h >> 31)) >> 1);
    i32 qx = w / surf->m_1c;
    i32 qy = h / surf->m_18;
    i32 scale = (qx < qy) ? qx : qy;
    if (scale > 3) {
        scale = 3;
    }
    m_scale = scale;
    i32 wpx = surf->m_1c * scale;
    i32 hpx = surf->m_18 * scale;
    m_dstL = cx - ((wpx - (wpx >> 31)) >> 1);
    m_dstT = cy - ((hpx - (hpx >> 31)) >> 1);
    m_dstR = surf->m_1c * scale + m_dstL;
    m_dstB = surf->m_18 * scale + m_dstT;
    if (ctx->m_2c->BltEx(&m_dstL, m_surface, 0, 0x1000000, 0) != 0) {
        return 0;
    }
    LfxRect* world = &m_surfMgr->m_24->m_5c->m_40;
    i32 l = world->left >> 5;
    i32 t = world->top >> 5;
    i32 rr = world->right >> 5;
    i32 b = world->bottom >> 5;
    if (m_scale != 1) {
        l *= m_scale;
        t *= m_scale;
        rr = rr * m_scale + m_scale - 1;
        b = b * m_scale + m_scale - 1;
    }
    LfxRect box;
    box.left = l + m_dstL;
    box.right = rr + m_dstL;
    box.top = t + m_dstT;
    box.bottom = b + m_dstT;
    DrawBorder(&box, ctx, 0xffff);
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
    ctx->m_08->Unlock(0);
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
    m_refreshRemaining = 0;
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
    LfxDrawCtx* ctx = m_mgr->m_2c;
    if (ctx != 0) {
        ctx->DrawAt(cell[0] * 32 + 16, cell[1] * 32 + 16);
    }
    m_handle = 1;
    return 1;
}

// ===========================================================================
// CLightFxRender::ClearHandle  (0x0a9500)
// ===========================================================================
RVA(0x000a9500, 0x16)
i32 CLightFxRender::ClearHandle(i32, i32, i32) {
    if (m_handle != 0) {
        m_handle = 0;
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
    ((LfxBlitTarget*)g_gameReg->m_cmdGrid)->Blit(cell[0] * 32 + 16, cell[1] * 32 + 16, 0, 0, 0, 0, 1);
    return 1;
}

// ===========================================================================
// CLightFxRender::ApplyB  (0x0a95d0)  - like ApplyA, but only if m_48 is latched.
// ===========================================================================
RVA(0x000a95d0, 0x69)
i32 CLightFxRender::ApplyB(i32, i32 x, i32 y) {
    if (m_handle == 0) {
        return 0;
    }
    i32 cell[2];
    if (!ClampRect(x, y, cell, 0x20)) {
        return 0;
    }
    LfxDrawCtx* ctx = m_mgr->m_2c;
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
    if (x < m_srcL || x > m_srcR || y < m_srcT || y > m_srcB) {
        return 0;
    }
    if (margin > 0) {
        if (x < m_dstL && m_dstL - x <= margin) {
            x = m_dstL;
        }
        if (x > m_dstR && x - m_dstR <= margin) {
            x = m_dstR;
        }
        if (y < m_dstT && m_dstT - y <= margin) {
            y = m_dstT;
        }
        if (y > m_dstB && y - m_dstB <= margin) {
            y = m_dstB;
        }
    }
    if (x < m_dstL || x > m_dstR || y < m_dstT || y > m_dstB) {
        return 0;
    }
    out[0] = (x - m_dstL) / m_scale;
    out[1] = (y - m_dstT) / m_scale;
    return 1;
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
SIZE_UNKNOWN(CLightFxRender);
SIZE_UNKNOWN(LfxBlitTarget);
SIZE_UNKNOWN(LfxBorderCtx);
SIZE_UNKNOWN(LfxCell);
SIZE_UNKNOWN(LfxColorNode);
SIZE_UNKNOWN(LfxDrawCtx);
SIZE_UNKNOWN(LfxGrid);
SIZE_UNKNOWN(LfxMgr);
SIZE_UNKNOWN(LfxRect);
SIZE_UNKNOWN(LfxRefTable);
SIZE_UNKNOWN(LfxSurfMgr);
SIZE_UNKNOWN(LfxSurfPool);
SIZE_UNKNOWN(LfxSurface);
SIZE_UNKNOWN(LfxTileBank);
SIZE_UNKNOWN(LfxTileDesc);
SIZE_UNKNOWN(LfxUnlockIface);
SIZE_UNKNOWN(LfxView);
SIZE_UNKNOWN(LfxWorldRect);
