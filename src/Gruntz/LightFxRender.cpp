#include <Mfc.h>
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <DDrawMgr/PixelShift.h> // g_rUp/g_gUp/g_bUp/g_rDown/g_gDown/g_bDown
#include <DDrawMgr/DDSurface.h>
#include <Mfc.h> // MFC superset of <Win32.h> (afx first): <Gruntz/SoundCue.h> now needs
#include <DDrawMgr/DDrawPtrCollections.h>
#include <Gruntz/SpriteRefTable.h>
#include <ddraw.h> // real IDirectDrawSurface dispatch (Unlock, slot 32 +0x80) on
#include <Gruntz/LightFxRender.h>

#include <Gruntz/GameRegistry.h>       // the g_gameReg singleton (0x24556c) canonical view
#include <Gruntz/TriggerMgr.h>         // CTriggerMgr (m_cmdGrid board; ResetGroup @0x79520)
#include <Gruntz/Play.h>               // mgr->m_curState play state: CPlay::ResetGoals @0xd5f00
#include <Gruntz/GruntzMgr.h>          // canonical CGruntzMgr (the ex-LfxMgr identity)
#include <Gruntz/Grunt.h>              // canonical CGrunt (the board cells; ex LfxTileDesc)
#include <Gruntz/Brickz.h>             // BrickzCell (the 0x1c map cell; ex LfxCell)
#include <Gruntz/GameLevel.h>          // CGameLevel + CDDrawWorkerHost (ex LfxView/LfxWorldRect)
#include <DDrawMgr/DDrawSurfacePair.h> // CDDrawSurfacePair (ex LfxBorderCtx)
#include <rva.h>
#include <Rez/FrameClock.h> // g_timer100 (detail threshold)

static inline u16 Pack(i32 r, i32 g, i32 b) {
    return static_cast<u16>(
        (((r >> g_rDown) << g_rUp) | ((g >> g_gDown) << g_gUp) | (b >> g_bDown))
    );
}

RVA(0x000a32c0, 0x72)
i32 CLightFxRender::Init(CGruntzMgr* mgr, i32 arg2) {
    if (mgr == 0) {
        return 0;
    }
    m_mgr = mgr;
    m_cmdGrid = mgr->m_cmdGrid;
    m_tileGrid = mgr->m_tileGrid;
    m_world = mgr->m_world;
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

RVA(0x000a3360, 0x29)
void CLightFxRender::Ctor() {
    FreeSurface();
    m_mgr = 0;
    m_cmdGrid = 0;
    m_tileGrid = 0;
    m_world = 0;
    m_surface = 0;
    m_handle = 0;
    m_refreshInterval = 0;
    m_refreshRemaining = 0;
}

RVA(0x000a33a0, 0x23)
void CLightFxRender::FreeSurface() {
    if (m_world != 0 && m_surface != 0) {
        m_world->m_ptrColl->RemoveItemA(m_surface);
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
    if (m_tileGrid == 0) {
        return 0;
    }
    if (m_world == 0) {
        return 0;
    }
    FreeSurface();
    CGruntzMapMgr* info = m_tileGrid;
    CDDrawSurfaceMgr* mgr = m_world;
    m_surface = mgr->m_ptrColl->MakeAndAddB(info->m_width, info->m_height, 0, 0, -1);
    if (m_surface == 0) {
        return 0;
    }
    m_surface->Clear(0); // 0x13edb0 - was a duplicate decl `Init0` of this same body
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
        if (static_cast<u32>(delta) < static_cast<u32>(m_refreshRemaining)) {
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
    CGruntzMapMgr* grid = m_tileGrid;
    if (m_surface->m_width != static_cast<i32>(grid->m_width)
        || m_surface->m_height != static_cast<i32>(grid->m_height)) {
        if (!AllocSurface()) {
            return 0;
        }
    }
    u16* base = reinterpret_cast<u16*>(m_surface->Lock(0));
    if (base == 0) {
        return 0;
    }
    CTriggerMgr* board = m_cmdGrid;
    for (u32 y = 0; y < grid->m_height; y++) {
        for (u32 x = 0; x < grid->m_width; x++) {
            u16* dst = reinterpret_cast<u16*>(
                (reinterpret_cast<char*>(base) + y * m_surface->m_pitch
                 + x * m_surface->m_bytesPerPixel)
            );
            i32 tile;
            if (x < grid->m_width && y < grid->m_height) {
                tile = grid->m_rows[y][x].m_4;
            } else {
                tile = -1;
            }
            if (tile == -1) {
                i32 idx;
                if (x < grid->m_width && y < grid->m_height) {
                    idx = grid->m_rows[y][x].m_c;
                } else {
                    idx = 0;
                }
                if (static_cast<u32>(idx) >= 0x1f4) {
                    *dst = 0;
                } else {
                    *dst = m_buf[idx];
                }
                continue;
            }
            // The packed cell id (low byte = col, high byte = row) indexes the
            // trigger mgr's 4x15 grunt board; the cell IS a CGrunt.
            CGrunt* desc = board->m_grid[(tile & 0xff) + ((tile >> 8) & 0xff) * 15];
            if (desc == 0) {
                continue;
            }
            i32 alt = 0;
            if (desc->m_arrived != 0) {
                alt = 1;
            }
            // The combat clock/timeout i64 pairs are stored as lo/hi i32 halves
            // (every writer stamps them as (lo, hi=0)); the 64-bit compare reads
            // them as the i64 they are - the documented int-pair overlay.
            if (static_cast<i64>(static_cast<u32>(g_frameTime))
                        - *reinterpret_cast<i64*>(&desc->m_combatClockLo)
                    >= *reinterpret_cast<i64*>(&desc->m_combatTimeoutLo)
                || desc->m_tileOwnerHi != g_curPlayer) {
                CSpriteRef* node = m_mgr->m_spriteFactory->GetA(desc->m_1f4_moveIcon);
                if (node == 0) {
                    *dst = 0;
                    continue;
                }
                if (alt == 0) {
                    *dst = node->m_teamColor1;
                } else if (alt == 1) {
                    *dst = node->m_teamColor2;
                } else if (alt == 2) {
                    *dst = node->m_teamColor3;
                } else {
                    *dst = node->m_teamColor1;
                }
                continue;
            }
            if (g_timer100 >= 0x32) {
                CSpriteRef* node = m_mgr->m_spriteFactory->GetA(desc->m_1f4_moveIcon);
                if (node == 0) {
                    *dst = 0;
                    continue;
                }
                *dst = node->m_teamColor2;
                continue;
            }
            i32 idx;
            if (x < grid->m_width && y < grid->m_height) {
                idx = grid->m_rows[y][x].m_c;
            } else {
                idx = 0;
            }
            if (static_cast<u32>(idx) >= 0x1f4) {
                *dst = 0;
            } else {
                *dst = m_buf[idx];
            }
        }
    }
    m_surface->m_ddSurface->Unlock(0);
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
i32 CLightFxRender::ComputeRect(CDDrawSurfacePair* ctx, RECT* src) {
    CDDSurface* surf = m_surface;
    if (surf == 0) {
        return 0;
    }
    RECT* srcRect = reinterpret_cast<RECT*>(&m_srcL);
    *srcRect = *src;
    i32 w = src->right - src->left + 1;
    i32 h = src->bottom - src->top + 1;
    i32 cx = src->left + ((w - (w >> 31)) >> 1);
    i32 cy = src->top + ((h - (h >> 31)) >> 1);
    i32 qx = w / surf->m_width;
    i32 qy = h / surf->m_height;
    i32 scale = (qx < qy) ? qx : qy;
    if (scale > 3) {
        scale = 3;
    }
    m_scale = scale;
    i32 wpx = surf->m_width * scale;
    i32 hpx = surf->m_height * scale;
    m_dstL = cx - ((wpx - (wpx >> 31)) >> 1);
    m_dstT = cy - ((hpx - (hpx >> 31)) >> 1);
    m_dstR = surf->m_width * scale + m_dstL;
    m_dstB = surf->m_height * scale + m_dstT;
    if (ctx->m_surface->BltEx(&m_dstL, m_surface, 0, 0x1000000, 0) != 0) {
        return 0;
    }
    // The live world rect is the main plane's origin/extent quad (+0x40..+0x4c);
    // >>5 converts world pixels to tile units.
    CDDrawWorkerHost* world = m_world->m_level->m_mainPlane;
    i32 l = world->m_originX >> 5;
    i32 t = world->m_originY >> 5;
    i32 rr = world->m_extentX >> 5;
    i32 b = world->m_extentY >> 5;
    if (m_scale != 1) {
        l *= m_scale;
        t *= m_scale;
        rr = rr * m_scale + m_scale - 1;
        b = b * m_scale + m_scale - 1;
    }
    RECT box;
    box.left = l + m_dstL;
    box.right = rr + m_dstL;
    box.top = t + m_dstT;
    box.bottom = b + m_dstT;
    DrawBorder(&box, ctx, 0xffff);
    return 1;
}

// ===========================================================================
// CLightFxRender::DrawBorderRaw  (0x0a3a20)  - paint the four edges of `r` with a
// 16-bit color straight into the caller-locked buffer `base`, using this->m_surface's
// geometry (m_pitch per row, m_b0 per column). Top/bottom are contiguous word runs
// (the u16-memset idiom -> rep stos); left/right step a column down each row. No
// lock/unlock (the caller owns them). Returns void.
// ===========================================================================
// @early-stop
// ~71% - same regalloc/frame wall as its twin DrawBorder (docs/patterns/zero-register-
// pinning.md, topic:wall topic:regalloc). Instruction SEQUENCE is byte-identical
// (verified sema disasm --diff), incl. the u16-memset rep-stos idiom + the edge-step
// loops; permuter confirmed no operand-order spelling closes it (70.644 -> 70.644).
// Residual: retail spills `this` (push ecx / mov [esp+0x10],ecx) and colors the
// color-dword into ebx, where cl keeps `this` in a callee-saved reg - a register-
// coloring choice not source-steerable. Logic byte-for-byte correct.
RVA(0x000a3a20, 0xe2)
void CLightFxRender::DrawBorderRaw(RECT* r, void* base, i32 color) {
    i32 w = r->right - r->left + 1;
    // Top edge (m_surface reloaded per block, matching the retail spill of `this`).
    u16* tp = reinterpret_cast<u16*>(
        (reinterpret_cast<char*>(base) + r->top * m_surface->m_pitch
         + r->left * m_surface->m_bytesPerPixel)
    );
    for (i32 t = 0; t < w; t++) {
        tp[t] = static_cast<u16>(color);
    }
    // Bottom edge.
    u16* bp = reinterpret_cast<u16*>(
        (reinterpret_cast<char*>(base) + r->bottom * m_surface->m_pitch
         + r->left * m_surface->m_bytesPerPixel)
    );
    for (i32 b = 0; b < w; b++) {
        bp[b] = static_cast<u16>(color);
    }
    // Left / right edges (column step = m_pitch per row).
    i32 h = r->bottom - r->top + 1;
    char* lp = reinterpret_cast<char*>(base) + r->left * m_surface->m_bytesPerPixel
               + r->top * m_surface->m_pitch;
    char* rp = reinterpret_cast<char*>(base) + r->right * m_surface->m_bytesPerPixel
               + r->top * m_surface->m_pitch;
    for (i32 v = 0; v < h; v++) {
        *reinterpret_cast<u16*>(lp) = static_cast<u16>(color);
        *reinterpret_cast<u16*>(rp) = static_cast<u16>(color);
        lp += m_surface->m_pitch;
        rp += m_surface->m_pitch;
    }
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
void CLightFxRender::DrawBorder(RECT* r, CDDrawSurfacePair* ctx, i32 color) {
    CDDSurface* surf = ctx->m_surface;
    u16* base = reinterpret_cast<u16*>(surf->Lock(0));
    if (base == 0) {
        return;
    }
    i32 w = r->right - r->left + 1;
    // Top edge.
    u16* tp = reinterpret_cast<u16*>(
        (reinterpret_cast<char*>(base) + r->top * surf->m_pitch + r->left * surf->m_bytesPerPixel)
    );
    for (i32 t = 0; t < w; t++) {
        tp[t] = static_cast<u16>(color);
    }
    // Bottom edge.
    u16* bp = reinterpret_cast<u16*>((
        reinterpret_cast<char*>(base) + r->bottom * surf->m_pitch + r->left * surf->m_bytesPerPixel
    ));
    for (i32 b = 0; b < w; b++) {
        bp[b] = static_cast<u16>(color);
    }
    // Left / right edges (column step = m_20 per row).
    i32 h = r->bottom - r->top + 1;
    char* lp =
        reinterpret_cast<char*>(base) + r->left * surf->m_bytesPerPixel + r->top * surf->m_pitch;
    char* rp =
        reinterpret_cast<char*>(base) + r->right * surf->m_bytesPerPixel + r->top * surf->m_pitch;
    for (i32 v = 0; v < h; v++) {
        *reinterpret_cast<u16*>(lp) = static_cast<u16>(color);
        *reinterpret_cast<u16*>(rp) = static_cast<u16>(color);
        lp += surf->m_pitch;
        rp += surf->m_pitch;
    }
    // Retail reloads the cached pair surface from its spill and unlocks ITS held
    // DirectDraw surface: [surf+0x8] -> IDirectDrawSurface::Unlock (slot 32).
    surf->m_ddSurface->Unlock(0);
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
        u32* p = reinterpret_cast<u32*>(m_buf);
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

RVA(0x000a9480, 0x5c)
i32 CLightFxRender::ApplyA(i32, i32 x, i32 y) {
    i32 cell[2];
    if (!ClampRect(x, y, cell, 0x20)) {
        return 0;
    }
    // The +0x2c slot is the current game state; the draw path only runs in play,
    // so the concrete state is the CPlay (the PickPlayOrPausedState downcast).
    CPlay* ctx = static_cast<CPlay*>(m_mgr->m_curState);
    if (ctx != 0) {
        ctx->ResetGoals(cell[0] * 32 + 16, cell[1] * 32 + 16);
    }
    m_handle = 1;
    return 1;
}

RVA(0x000a9500, 0x16)
i32 CLightFxRender::ClearHandle(i32, i32, i32) {
    if (m_handle != 0) {
        m_handle = 0;
    }
    return 1;
}

RVA(0x000a9550, 0x5b)
i32 CLightFxRender::ApplyGlobal(i32, i32 x, i32 y) {
    i32 cell[2];
    if (!ClampRect(x, y, cell, 0x20)) {
        return 0;
    }
    g_gameReg->m_cmdGrid->ResetGroup(cell[0] * 32 + 16, cell[1] * 32 + 16, 0, 0, 0, 0, 1);
    return 1;
}

RVA(0x000a95d0, 0x69)
i32 CLightFxRender::ApplyB(i32, i32 x, i32 y) {
    if (m_handle == 0) {
        return 0;
    }
    i32 cell[2];
    if (!ClampRect(x, y, cell, 0x20)) {
        return 0;
    }
    CPlay* ctx = static_cast<CPlay*>(m_mgr->m_curState);
    if (ctx != 0) {
        ctx->ResetGoals(cell[0] * 32 + 16, cell[1] * 32 + 16);
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

