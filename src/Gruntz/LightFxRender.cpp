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
#include <Pix16.h>          // the byte-cursor / 16bpp-value pointer pair
#include <Rez/FrameClock.h> // g_timer100 (detail threshold)

// The grid's two dimensions read as ONE pair.  CLightFxRender::AllocSurface copies the
// 8-byte {m_width,m_height} block out ASCENDING (`mov edx,[info+0xc]; mov eax,[info+0x10]`,
// arg1 in edx) - a struct-valued read; two independent member loads give the opposite
// register pair because cl then evaluates the call's arguments right-to-left.
static inline SIZE
GridSize(const CGruntzMapMgr* grid) {
    SIZE
    s;
    s.cx = grid->m_width;
    s.cy = grid->m_height;
    return s;
}

static inline u16 Pack(i32 r, i32 g, i32 b) {
    return static_cast<u16>(
        (((r >> g_rDown) << g_rUp) | ((g >> g_gDown) << g_gUp) | (b >> g_bDown))
    );
}

// A locked surface row is BYTES with a byte pitch while the pixels are 16bpp - that
// conversion is forced by the surface API, so it is named here.
static inline u16* Pix16(void* p) {
    return static_cast<u16*>(p);
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
    m_dstRect.left = 0;
    m_dstRect.top = 0;
    m_dstRect.right = 0;
    m_dstRect.bottom = 0;
    m_srcRect.left = 0;
    m_srcRect.top = 0;
    m_srcRect.right = 0;
    m_srcRect.bottom = 0;
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
    // WIDTH first, then height: retail pushes [info+0xc] as arg1 and [info+0x10] as
    // arg2 (the ex "edx<->eax regalloc tail" was these two arguments TRANSPOSED - the
    // work surface was being allocated with the grid's rows/columns swapped).
    // The pair is read as ONE 8-byte value (GridSize) - that is what makes cl load the
    // two dimensions ASCENDING; passing the two members straight into the call makes it
    // evaluate them right-to-left and swaps the edx/eax assignment.
    SIZE
    dims = GridSize(info);
    m_surface = mgr->m_ptrColl->MakeAndAddB(dims.cx, dims.cy, 0, 0, -1);
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
// zero-register-pinning WALL (docs/patterns/zero-register-pinning.md, INVERSE case).
// Three real defects were fixed here first: (1) the decay if/else was INVERTED - retail
// `jb` skips to the subtract, so the clamp-to-zero is the `if` body; (2) both buffer-copy
// arms are the `else` sides, not early-out `continue`s - retail parks them at the tail
// (`cmp eax,-1 / je <far>`, `cmp g_timer100,0x32 / jae <far>`) and falls through into
// them; (3) g_timer100 is compared UNSIGNED (`jae`), and the alt->color pick is a
// `switch`, not an if-else chain - and (4) that switch's case list STARTS AT 0:
// retail dispatches `mov ecx,edi / sub ecx,0x0 / je c0 / dec / je c1 / dec / jne c0`,
// and `sub ecx,0` is cl subtracting the LOWEST case value, so `case 0:` is spelled
// out even though its body equals `default:` (they tail-merge onto the +0x8 arm).
// Omitting it makes cl start the range at 1 and emit `dec / je / dec / je`
// (56.8 -> 59.6). Both instruction streams are now 239 long. What is left is the
// allocation: retail pins `this` in ebp with a 2-slot frame and stores immediates
// (`mov [ebp+0x438],0`), while our cl materialises the constant 0 in esi for the
// prologue tests, caches m_tileGrid->m_width in ebp (retail re-reads it at each of
// its 3 uses) and therefore has to spill `alt` to a 3rd frame slot where retail
// keeps it in edi - a 1-instr phase shift that renames every register through the
// 755B body, plus a cold-block placement difference at the tail.
RVA(0x000a3460, 0x2f3)
i32 CLightFxRender::Resize(i32 delta, i32 rebuild) {
    if (rebuild == 0) {
        // Retail tests the SATURATING side: `jb` skips to the subtract, so the `if`
        // body is the clamp-to-zero and the subtract is the else.
        if (static_cast<u32>(delta) >= static_cast<u32>(m_refreshRemaining)) {
            m_refreshRemaining = 0;
        } else {
            m_refreshRemaining -= delta;
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
    if (m_surface->m_width != static_cast<i32>(m_tileGrid->m_width)
        || m_surface->m_height != static_cast<i32>(m_tileGrid->m_height)) {
        if (!AllocSurface()) {
            return 0;
        }
    }
    char* base = static_cast<char*>(m_surface->Lock(0)); // the lock hands back bytes
    if (base == 0) {
        return 0;
    }
    for (u32 y = 0; y < m_tileGrid->m_height; y++) {
        for (u32 x = 0; x < m_tileGrid->m_width; x++) {
            u16* dst = Pix16(base + y * m_surface->m_pitch + x * m_surface->m_bytesPerPixel);
            i32 tile;
            if (x < m_tileGrid->m_width && y < m_tileGrid->m_height) {
                tile = m_tileGrid->m_rows[y][x].m_4;
            } else {
                tile = -1;
            }
            // The live-tile path is the FALLTHROUGH (`cmp eax,-1 / je <far>`): both
            // buffer-copy arms sit at the tail of the body in retail, so they are the
            // `else` sides here, not early-out `continue`s.
            if (tile != -1) {
                // The packed cell id (low byte = col, high byte = row) indexes the
                // trigger mgr's 4x15 grunt board; the cell IS a CGrunt.
                CGrunt* desc = m_cmdGrid->m_grid[(tile & 0xff) + ((tile >> 8) & 0xff) * 15];
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
                if (static_cast<i64>(static_cast<u32>(g_frameTime)) - desc->m_combatClock64
                        >= desc->m_combatTimeout64
                    || desc->m_tileOwnerHi != g_curPlayer) {
                    CSpriteRef* node = m_mgr->m_spriteFactory->GetA(desc->m_1f4_moveIcon);
                    if (node == 0) {
                        *dst = 0;
                        continue;
                    }
                    // The case list STARTS at 0: retail dispatches `mov ecx,edi /
                    // sub ecx,0x0 / je c0 / dec / je c1 / dec / jne c0` - `sub ecx,0`
                    // is cl subtracting the LOWEST case value, so `case 0:` is
                    // written out even though its body equals `default:` (they
                    // tail-merge onto the +0x8 arm). Dropping it makes cl start the
                    // range at 1 and emit `dec / je / dec / je` instead.
                    switch (alt) {
                        case 0:
                            *dst = node->m_teamColor1;
                            break;
                        case 1:
                            *dst = node->m_teamColor2;
                            break;
                        case 2:
                            *dst = node->m_teamColor3;
                            break;
                        default:
                            *dst = node->m_teamColor1;
                            break;
                    }
                } else if (static_cast<u32>(g_timer100) < 0x32) {
                    // the UNDER-threshold arm is the fallthrough (`jae` jumps to the
                    // sprite lookup), so the detail-copy is the `if` side here.
                    i32 idx;
                    if (x < m_tileGrid->m_width && y < m_tileGrid->m_height) {
                        idx = m_tileGrid->m_rows[y][x].m_c;
                    } else {
                        idx = 0;
                    }
                    if (static_cast<u32>(idx) >= 0x1f4) {
                        *dst = 0;
                    } else {
                        *dst = m_buf[idx];
                    }
                } else {
                    CSpriteRef* node = m_mgr->m_spriteFactory->GetA(desc->m_1f4_moveIcon);
                    if (node == 0) {
                        *dst = 0;
                        continue;
                    }
                    *dst = node->m_teamColor2;
                }
            } else {
                i32 idx;
                if (x < m_tileGrid->m_width && y < m_tileGrid->m_height) {
                    idx = m_tileGrid->m_rows[y][x].m_c;
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
// Three real defects fixed: (1) the centering/halving was hand-spelled
// `(x - (x>>31)) >> 1`, which emits `sar reg,0x1f`; retail's `cdq; sub eax,edx;
// sar eax,1` IS cl's lowering of a plain signed `x / 2`. (2) The world rect is read
// through a RECT* cursor (`add eax,0x40` once, then [eax]/[eax+4]/...), not four
// `plane->m_viewRect.<field>` loads with +0x40 folded into each disp8
// (docs/patterns/member-aggregate-copied-not-field-by-field.md). (3) The border rect
// is built IN PLACE in the local RECT - retail stores each `>> 5` straight into the
// box slot (dead stores the scaling then overwrites), which loose i32 temporaries
// never produce. (4) The scale min/clamp are SEEDED IFs, not ternaries: retail seeds
// the destination with the THEN value and branches over the copy-back
// (`cmp edx,eax / jge / mov eax,edx`), where cl lowers `a>=b ? b : a` the other way
// round and rewrites `s = s>3 ? 3 : s` as an in-place `if`. Residual is a
// spill-choice wall: retail keeps `surf` in ecx across the whole body and spills
// `qx` to a frame slot; our cl spills `surf` and keeps qx, so every `[ecx+0x1c]`
// becomes a reload through `[esp+0x10]`. Also cl fuses `right*s + s - 1` into
// `(right+1)*s - 1` (`inc/imul/dec`) where retail keeps `imul` + `lea [r+s-1]`; no
// statement spelling tried blocks that reassociation.
RVA(0x000a3820, 0x18e)
i32 CLightFxRender::ComputeRect(CDDrawSurfacePair* ctx, RECT* src) {
    CDDSurface* surf = m_surface;
    if (surf == 0) {
        return 0;
    }
    RECT* srcRect = &m_srcRect;
    *srcRect = *src;
    i32 w = src->right - src->left + 1;
    i32 h = src->bottom - src->top + 1;
    // Signed /2, not (x - (x>>31)) >> 1: retail's `cdq; sub eax,edx; sar eax,1` IS
    // cl's lowering of `x / 2` - the hand-written round-toward-zero spelling emits
    // `sar reg,0x1f` instead.
    i32 cx = src->left + w / 2;
    i32 cy = src->top + h / 2;
    i32 qx = w / surf->m_width;
    i32 qy = h / surf->m_height;
    // retail SEEDS the result with qy and copies qx in on the `jge` fallthrough
    // (`cmp edx,eax / jge / mov eax,edx`) - a seeded `if`, not a ternary: cl lowers
    // `(qx >= qy) ? qy : qx` the other way round (seeds the destination with qx and
    // branches on `jl`).
    i32 scale = qy;
    if (qx < qy) {
        scale = qx;
    }
    // Same shape for the clamp - retail materializes 3 into a FRESH register and
    // jumps over the copy-back (`cmp eax,3 / mov ebp,3 / jg / mov ebp,eax`); a
    // ternary whose destination is `scale` itself becomes an in-place `if`.
    i32 s = 3;
    if (scale <= 3) {
        s = scale;
    }
    m_scale = s;
    m_dstRect.left = cx - surf->m_width * s / 2;
    m_dstRect.top = cy - surf->m_height * s / 2;
    m_dstRect.right = surf->m_width * s + m_dstRect.left;
    m_dstRect.bottom = surf->m_height * s + m_dstRect.top;
    if (ctx->m_surface->BltEx(&m_dstRect.left, surf, 0, 0x1000000, 0) != 0) {
        return 0;
    }
    // The live world rect is the main plane's origin/extent quad (+0x40..+0x4c);
    // >>5 converts world pixels to tile units. Retail materializes the rect's ADDRESS
    // once (`add eax,0x40`) and reads [eax]/[eax+4]/... off it - a RECT* cursor, not
    // four `plane->m_viewRect.<field>` loads with +0x40 folded into each disp8.
    // The border rect is built IN PLACE - retail stores each `>> 5` straight into the
    // local RECT's slot (dead stores that the scaling then overwrites), which loose
    // i32 l/t/rr/b temporaries never produce.
    RECT* vr = &m_world->m_level->m_mainPlane->m_viewRect;
    RECT box;
    box.left = vr->left >> 5;
    box.top = vr->top >> 5;
    box.right = vr->right >> 5;
    box.bottom = vr->bottom >> 5;
    if (m_scale != 1) {
        // four imuls THEN the two `+ scale - 1` folds (`lea edx,[edx+eax-1]`); the
        // fused `right * m_scale + m_scale - 1` is strength-reduced to inc/imul/dec.
        box.left *= m_scale;
        box.top *= m_scale;
        box.right *= m_scale;
        box.bottom *= m_scale;
        box.right += m_scale - 1;
        box.bottom += m_scale - 1;
    }
    box.left += m_dstRect.left;
    box.right += m_dstRect.left;
    box.top += m_dstRect.top;
    box.bottom += m_dstRect.top;
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
// The vertical-edge loop body now matches retail instruction-for-instruction (two
// bare stores + two adds of a hoisted step + dec/jne): the row step MUST be a local
// (`lp += m_surface->m_pitch` in the loop re-loads this->m_surface every iteration -
// the u16 stores defeat cl's CSE across them) AND the two cursors must be real
// POINTERS, so cl folds `base` in once in the preheader instead of emitting
// base-indexed `[eax+ebp]` stores. Residual is the register-colouring split: retail
// spills `this` (`push ecx` + `mov [esp+0x10],ecx`) and so has ebp free for `color`
// and ebx as scratch, while our cl keeps `this` in ebx, pins `base` in ebp and
// re-loads `color` from its argument slot at each edge. That one allocation choice
// also makes retail RE-READ m_pitch/m_bytesPerPixel at each use where we CSE them.
RVA(0x000a3a20, 0xe2)
void CLightFxRender::DrawBorderRaw(RECT* r, void* base, i32 color) {
    i32 w = r->right - r->left + 1;
    // Top edge (m_surface reloaded per block, matching the retail spill of `this`).
    // The COLUMN term is added to `base` first here (`add base,bpp*left` then
    // `add pitch*top,...`) - the bottom edge below is the other way round; the two
    // edges really are spelled differently in retail.
    // `base` is the locked surface cursor and the row/column steps are the surface's
    // BYTE quantities (m_pitch, m_bytesPerPixel); the pixels are 16bpp (<Pix16.h>).
    Pix16Ptr top;
    top.m_chars =
        (static_cast<char*>(base) + r->left * m_surface->m_bytesPerPixel
         + r->top * m_surface->m_pitch);
    u16* tp = top.m_words;
    for (i32 t = 0; t < w; t++) {
        tp[t] = static_cast<u16>(color);
    }
    // Bottom edge.
    // same m_pitch/m_bytesPerPixel byte arithmetic on the locked cursor
    Pix16Ptr bot;
    bot.m_chars =
        (static_cast<char*>(base) + r->bottom * m_surface->m_pitch
         + r->left * m_surface->m_bytesPerPixel);
    u16* bp = bot.m_words;
    for (i32 b = 0; b < w; b++) {
        bp[b] = static_cast<u16>(color);
    }
    // Left / right edges. The row step is a LOCAL: retail reads m_pitch a third time
    // into a register and the loop body is only `mov/mov/add/add/dec/jne` - reading
    // m_surface->m_pitch in the loop instead re-loads this->m_surface every iteration
    // (the u16 stores defeat cl's CSE across them).
    i32 h = r->bottom - r->top + 1;
    i32 lo = r->top * m_surface->m_pitch + r->left * m_surface->m_bytesPerPixel;
    i32 ro = r->top * m_surface->m_pitch + r->right * m_surface->m_bytesPerPixel;
    i32 step = m_surface->m_pitch;
    // The two column cursors are real POINTERS: retail folds `base` in once in the
    // loop preheader (`mov edx,[esp+0x1c] / add edi,edx / add edx,esi`) and the body
    // is a bare store/store/add/add/dec/jne. Keeping `lo`/`ro` as indices instead
    // makes cl emit base-indexed `[eax+ebp]` stores and pin `base` all loop.
    // (Its twin DrawBorder is the OTHER way round - there retail really does index.)
    char* lp = static_cast<char*>(base) + lo;
    char* rp = static_cast<char*>(base) + ro;
    for (i32 v = 0; v < h; v++) {
        *Pix16(lp) = static_cast<u16>(color);
        *Pix16(rp) = static_cast<u16>(color);
        lp += step;
        rp += step;
    }
}

// ===========================================================================
// CLightFxRender::DrawBorder  (0x0a3b50)  - lock the ctx work surface, paint the
// four edges of `r` with `color`, unlock. `this`/ecx is unused. The top/bottom
// edges are contiguous word runs (left..right); the left/right edges step a
// column down each row. The fill at each edge is the inlined u16-memset idiom.
// ===========================================================================
// @early-stop
// 71 -> 88%: the vertical edges keep BYTE OFFSETS and index the locked base at each
// store (`mov [eax+ebx],dx`) with the row step hoisted into a register - absolute
// `char*` cursors plus `lp += surf->m_pitch` re-loaded the pitch every iteration.
// Residual is the commutative-imul operand pick (docs/patterns/commutative-imul-
// operand-in-eax.md: retail `mov ecx,top; imul ecx,[pitch]`, cl the other way round)
// plus the load scheduling around it - proven not source-steerable.
RVA(0x000a3b50, 0xfa)
void CLightFxRender::DrawBorder(RECT* r, CDDrawSurfacePair* ctx, i32 color) {
    CDDSurface* surf = ctx->m_surface;
    char* base = static_cast<char*>(surf->Lock(0)); // the lock hands back bytes
    if (base == 0) {
        return;
    }
    i32 w = r->right - r->left + 1;
    // Top edge.
    u16* tp = Pix16(base + r->top * surf->m_pitch + r->left * surf->m_bytesPerPixel);
    for (i32 t = 0; t < w; t++) {
        tp[t] = static_cast<u16>(color);
    }
    // Bottom edge.
    u16* bp = Pix16(base + r->bottom * surf->m_pitch + r->left * surf->m_bytesPerPixel);
    for (i32 b = 0; b < w; b++) {
        bp[b] = static_cast<u16>(color);
    }
    // Left / right edges. Retail keeps the two cursors as BYTE OFFSETS and indexes the
    // locked base at each store (`mov [eax+ebx],dx`), with the row step hoisted into a
    // register - so the offsets are i32 here and `base` is added at the use.
    i32 h = r->bottom - r->top + 1;
    i32 lo = r->left * surf->m_bytesPerPixel + r->top * surf->m_pitch;
    i32 ro = r->right * surf->m_bytesPerPixel + r->top * surf->m_pitch;
    i32 step = surf->m_pitch;
    for (i32 v = 0; v < h; v++) {
        *Pix16(base + lo) = static_cast<u16>(color);
        *Pix16(base + ro) = static_cast<u16>(color);
        lo += step;
        ro += step;
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
// The COMDAT is code (0xc7) + a 1-byte align pad + the 8-entry switch jump table at
// 0xa3d58 (the `jmp [eax*4+0x4a3d58]` reloc target), so the RVA span is 0xe8, not 0xc7:
// carved at 0xc7 the delinked target obj lost the table and objdiff scored our 9 extra
// rows (nop + 8 dwords) as inserts. The code bytes were already identical.
RVA(0x000a3c90, 0xe8)
i32 CLightFxRender::BuildShape(i32 shape) {
    if (shape > 8) {
        return 0;
    }
    memset(m_buf, 0, sizeof(m_buf)); // retail: mov ecx,0xfa / rep stos dword (1000 B)
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
// Pre-computes 21 screen-native 16-bit colors from 8-bit (R,G,B) triples via the
// RGB shift table (Pack, see docs/patterns/rgb-pack-variable-shift.md), paints a
// fixed icon into the +0x4c pixel buffer (direct word runs + FillSpan spans).
// All eight generators share one icon layout and differ only in the palette.
// ===========================================================================
// @early-stop
// Two shared defects are fixed here; both were in all eight generators.
// (1) The PALETTES: cl CSEs a repeated per-channel `(v>>down)<<up` term into a stack
//     slot, and a previous read of the disassembly took each of those slots for a
//     SEPARATE single-channel colour - see
//     docs/patterns/cse-partial-term-is-not-a-separate-constant.md. The 21 colours
//     are now decoded symbolically from the retail body and verified against it.
// (2) The STORE PROGRAM: every run of adjacent same-colour pixels is a LOOP, not a
//     list of assignments. cl5 gives a loop its FILL expansion - the value is
//     duplicated into a dword (`mov cx,ax / shl ecx,0x10 / mov cx,ax`) and the range
//     is written with DWORD stores (`rep stos` when long, unrolled `mov DWORD PTR
//     [esi+N],eax` when short, `+ stos WORD` when the count is odd) - while separate
//     assignments emit one 16-bit store each. Converting the 67 short runs took the
//     eight from 82.7-85.4% to 98.5-99.5% and, as a side effect, moved the frame
//     from 20 to 21 dwords, which dissolved the "retail enregisters g_rDown in ebx /
//     our cl gives ebx to colour #0" split previously filed as this family's wall.
//     docs/patterns/adjacent-same-value-stores-are-a-loop.md
// Residual (all eight): the scheduler places the buf[257] store one slot later than
// retail inside the second FillSpan's push sequence (retail pairs push/store, we
// emit push,push,store,store); Shape2/5/6/7 additionally flip the R-vs-G term order
// in one or two colours and Shape5 permutes two stack slots. Statement reordering
// and both Pack() operand orders were tried and are byte-identical.
RVA(0x000a3dc0, 0x85f)
i32 CLightFxRender::Shape1() {
    u16* buf = m_buf;
    i32 i;
    u16 c00 = Pack(0x4f, 0x14, 0x01);
    u16 c01 = Pack(0x63, 0x37, 0x13);
    u16 c02 = Pack(0x5c, 0x0d, 0x06);
    u16 c03 = Pack(0x10, 0x28, 0x71);
    u16 c04 = Pack(0x26, 0x62, 0x71);
    u16 c05 = Pack(0x00, 0x00, 0x00);
    u16 c06 = Pack(0x20, 0x20, 0x20);
    u16 c07 = Pack(0x78, 0x78, 0x5f);
    u16 c08 = Pack(0x64, 0x64, 0x64);
    u16 c09 = Pack(0xff, 0xd9, 0x13);
    u16 c10 = Pack(0xff, 0xd2, 0x47);
    u16 c11 = Pack(0xa1, 0x2b, 0xff);
    u16 c12 = Pack(0x45, 0x7c, 0xff);
    u16 c13 = Pack(0x45, 0xff, 0x45);
    u16 c14 = Pack(0xff, 0x26, 0x26);
    u16 c15 = Pack(0xff, 0x92, 0x2b);
    u16 c16 = Pack(0xd7, 0xd7, 0xd7);
    u16 c17 = Pack(0x37, 0x37, 0x37);
    u16 c18 = Pack(0xb4, 0x61, 0x39);
    u16 c19 = Pack(0x37, 0x30, 0x30);
    u16 c20 = Pack(0xa0, 0xa0, 0x27);
    for (i = 1; i < 9; i++) {
        buf[i] = c00;
    }
    for (i = 17; i < 37; i++) {
        buf[i] = c00;
    }
    buf[90] = c00;
    for (i = 195; i < 197; i++) {
        buf[i] = c00;
    }
    buf[199] = c00;
    buf[301] = c00;
    for (i = 9; i < 17; i++) {
        buf[i] = c01;
    }
    buf[91] = c01;
    for (i = 197; i < 199; i++) {
        buf[i] = c01;
    }
    for (i = 40; i < 74; i++) {
        buf[i] = c02;
    }
    for (i = 270; i < 282; i++) {
        buf[i] = c02;
    }
    for (i = 104; i < 116; i++) {
        buf[i] = c06;
    }
    for (i = 120; i < 124; i++) {
        buf[i] = c06;
    }
    for (i = 128; i < 140; i++) {
        buf[i] = c06;
    }
    for (i = 144; i < 156; i++) {
        buf[i] = c04;
    }
    for (i = 160; i < 164; i++) {
        buf[i] = c04;
    }
    for (i = 168; i < 180; i++) {
        buf[i] = c04;
    }
    for (i = 157; i < 159; i++) {
        buf[i] = c03;
    }
    for (i = 165; i < 167; i++) {
        buf[i] = c03;
    }
    buf[258] = c03;
    buf[264] = c03;
    for (i = 117; i < 119; i++) {
        buf[i] = c05;
    }
    FillSpan(0x7d, 0x7e, c05);
    buf[260] = c05;
    buf[266] = c05;
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
    FillSpan(0xed, 0xee, c11);
    FillSpan(0xff, 0x100, c11);
    FillSpan(0xe1, 0xe4, c12);
    FillSpan(0xe5, 0xe8, c13);
    FillSpan(0xfb, 0xfc, c13);
    FillSpan(0xe9, 0xec, c14);
    FillSpan(0xfd, 0xfe, c14);
    FillSpan(0xef, 0xf0, c15);
    FillSpan(0xf7, 0xf8, c15);
    FillSpan(0xd9, 0xda, c16);
    FillSpan(0xf9, 0xfa, c16);
    FillSpan(0xf3, 0xf6, c17);
    FillSpan(0x12e, 0x143, c18);
    FillSpan(0xd5, 0xd6, c19);
    FillSpan(0xd7, 0xd8, c20);
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
// Palette + store program re-decoded from the retail body and verified against it
// (see Shape1 for both defects: the CSE mis-read that had zeroed a channel in most
// colours, and the run-of-stores-is-a-loop fill expansion). Residual is Shape1's
// scheduler placement of the buf[257] store.
RVA(0x000a4890, 0x852)
i32 CLightFxRender::Shape2() {
    u16* buf = m_buf;
    i32 i;
    u16 c00 = Pack(0xe0, 0xed, 0xfe);
    u16 c01 = Pack(0x89, 0x6e, 0x58);
    u16 c02 = Pack(0xd7, 0xe5, 0xfa);
    u16 c03 = Pack(0x10, 0x28, 0x71);
    u16 c04 = Pack(0x26, 0x62, 0x71);
    u16 c05 = Pack(0x00, 0x00, 0x00);
    u16 c06 = Pack(0x20, 0x20, 0x20);
    u16 c07 = Pack(0x49, 0x65, 0x84);
    u16 c08 = Pack(0xff, 0xd9, 0x13);
    u16 c09 = Pack(0xff, 0xd2, 0x47);
    u16 c10 = Pack(0xa1, 0x2b, 0xff);
    u16 c11 = Pack(0x45, 0x7c, 0xff);
    u16 c12 = Pack(0x45, 0xff, 0x45);
    u16 c13 = Pack(0xff, 0x26, 0x26);
    u16 c14 = Pack(0xff, 0x92, 0x2b);
    u16 c15 = Pack(0xd7, 0xd7, 0xd7);
    u16 c16 = Pack(0x37, 0x37, 0x37);
    u16 c17 = Pack(0xb4, 0x61, 0x39);
    u16 c18 = Pack(0x37, 0x30, 0x30);
    u16 c19 = Pack(0xa0, 0xa0, 0x27);
    for (i = 1; i < 9; i++) {
        buf[i] = c00;
    }
    for (i = 17; i < 37; i++) {
        buf[i] = c00;
    }
    buf[90] = c00;
    for (i = 195; i < 197; i++) {
        buf[i] = c00;
    }
    buf[199] = c00;
    buf[301] = c00;
    for (i = 9; i < 17; i++) {
        buf[i] = c01;
    }
    buf[91] = c01;
    for (i = 197; i < 199; i++) {
        buf[i] = c01;
    }
    for (i = 40; i < 74; i++) {
        buf[i] = c02;
    }
    for (i = 270; i < 282; i++) {
        buf[i] = c02;
    }
    for (i = 104; i < 116; i++) {
        buf[i] = c06;
    }
    for (i = 120; i < 124; i++) {
        buf[i] = c06;
    }
    for (i = 128; i < 140; i++) {
        buf[i] = c06;
    }
    for (i = 144; i < 156; i++) {
        buf[i] = c04;
    }
    for (i = 160; i < 164; i++) {
        buf[i] = c04;
    }
    for (i = 168; i < 180; i++) {
        buf[i] = c04;
    }
    for (i = 157; i < 159; i++) {
        buf[i] = c03;
    }
    for (i = 165; i < 167; i++) {
        buf[i] = c03;
    }
    buf[258] = c03;
    buf[264] = c03;
    for (i = 117; i < 119; i++) {
        buf[i] = c05;
    }
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
    FillSpan(0xed, 0xee, c10);
    FillSpan(0xff, 0x100, c10);
    FillSpan(0xe1, 0xe4, c11);
    FillSpan(0xe5, 0xe8, c12);
    FillSpan(0xfb, 0xfc, c12);
    FillSpan(0xe9, 0xec, c13);
    FillSpan(0xfd, 0xfe, c13);
    FillSpan(0xef, 0xf0, c14);
    FillSpan(0xf7, 0xf8, c14);
    FillSpan(0xd9, 0xda, c15);
    FillSpan(0xf9, 0xfa, c15);
    FillSpan(0xf3, 0xf6, c16);
    FillSpan(0x12e, 0x143, c17);
    FillSpan(0xd5, 0xd6, c18);
    FillSpan(0xd7, 0xd8, c19);
    FillSpan(0x5c, 0x5f, c00);
    return 1;
}
// @early-stop
// Palette + store program re-decoded from the retail body and verified against it
// (see Shape1 for both defects: the CSE mis-read that had zeroed a channel in most
// colours, and the run-of-stores-is-a-loop fill expansion). Residual is Shape1's
// scheduler placement of the buf[257] store.
RVA(0x000a5310, 0x855)
i32 CLightFxRender::Shape3() {
    u16* buf = m_buf;
    i32 i;
    u16 c00 = Pack(0x4e, 0x78, 0x1c);
    u16 c01 = Pack(0x23, 0x23, 0x23);
    u16 c02 = Pack(0x24, 0x37, 0x0f);
    u16 c03 = Pack(0x10, 0x28, 0x71);
    u16 c04 = Pack(0x26, 0x62, 0x71);
    u16 c05 = Pack(0xb4, 0x3d, 0x0b);
    u16 c06 = Pack(0x64, 0x0c, 0x03);
    u16 c07 = Pack(0xb0, 0x85, 0x1f);
    u16 c08 = Pack(0x59, 0x17, 0x0f);
    u16 c09 = Pack(0xff, 0xd9, 0x13);
    u16 c10 = Pack(0xff, 0xd2, 0x47);
    u16 c11 = Pack(0xa1, 0x2b, 0xff);
    u16 c12 = Pack(0x45, 0x7c, 0xff);
    u16 c13 = Pack(0x45, 0xff, 0x45);
    u16 c14 = Pack(0xff, 0x26, 0x26);
    u16 c15 = Pack(0xff, 0x92, 0x2b);
    u16 c16 = Pack(0xd7, 0xd7, 0xd7);
    u16 c17 = Pack(0x37, 0x37, 0x37);
    u16 c18 = Pack(0xb4, 0x61, 0x39);
    u16 c19 = Pack(0x37, 0x30, 0x30);
    u16 c20 = Pack(0xa0, 0xa0, 0x27);
    for (i = 1; i < 9; i++) {
        buf[i] = c00;
    }
    for (i = 17; i < 37; i++) {
        buf[i] = c00;
    }
    buf[90] = c00;
    for (i = 195; i < 197; i++) {
        buf[i] = c00;
    }
    buf[199] = c00;
    buf[301] = c00;
    for (i = 9; i < 17; i++) {
        buf[i] = c01;
    }
    buf[91] = c01;
    for (i = 197; i < 199; i++) {
        buf[i] = c01;
    }
    for (i = 40; i < 74; i++) {
        buf[i] = c02;
    }
    for (i = 270; i < 282; i++) {
        buf[i] = c02;
    }
    for (i = 104; i < 116; i++) {
        buf[i] = c06;
    }
    for (i = 120; i < 124; i++) {
        buf[i] = c06;
    }
    for (i = 128; i < 140; i++) {
        buf[i] = c06;
    }
    for (i = 144; i < 156; i++) {
        buf[i] = c04;
    }
    for (i = 160; i < 164; i++) {
        buf[i] = c04;
    }
    for (i = 168; i < 180; i++) {
        buf[i] = c04;
    }
    for (i = 157; i < 159; i++) {
        buf[i] = c03;
    }
    for (i = 165; i < 167; i++) {
        buf[i] = c03;
    }
    buf[258] = c03;
    buf[264] = c03;
    for (i = 117; i < 119; i++) {
        buf[i] = c05;
    }
    FillSpan(0x7d, 0x7e, c05);
    buf[260] = c05;
    buf[266] = c05;
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
    FillSpan(0xed, 0xee, c11);
    FillSpan(0xff, 0x100, c11);
    FillSpan(0xe1, 0xe4, c12);
    FillSpan(0xe5, 0xe8, c13);
    FillSpan(0xfb, 0xfc, c13);
    FillSpan(0xe9, 0xec, c14);
    FillSpan(0xfd, 0xfe, c14);
    FillSpan(0xef, 0xf0, c15);
    FillSpan(0xf7, 0xf8, c15);
    FillSpan(0xd9, 0xda, c16);
    FillSpan(0xf9, 0xfa, c16);
    FillSpan(0xf3, 0xf6, c17);
    FillSpan(0x12e, 0x143, c18);
    FillSpan(0xd5, 0xd6, c19);
    FillSpan(0xd7, 0xd8, c20);
    FillSpan(0x5c, 0x5f, c00);
    return 1;
}
// @early-stop
// Palette + store program re-decoded from the retail body and verified against it
// (see Shape1 for both defects: the CSE mis-read that had zeroed a channel in most
// colours, and the run-of-stores-is-a-loop fill expansion). Residual is Shape1's
// scheduler placement of the buf[257] store.
RVA(0x000a5d90, 0x825)
i32 CLightFxRender::Shape4() {
    u16* buf = m_buf;
    i32 i;
    u16 c00 = Pack(0x8b, 0x9f, 0xfd);
    u16 c01 = Pack(0x00, 0xc1, 0xa7);
    u16 c02 = Pack(0x47, 0x65, 0xf1);
    u16 c03 = Pack(0x01, 0x00, 0x5e);
    u16 c04 = Pack(0x0d, 0x20, 0xbe);
    u16 c05 = Pack(0x00, 0x00, 0x00);
    u16 c06 = Pack(0x45, 0x2e, 0x0d);
    u16 c07 = Pack(0xff, 0xc5, 0xe0);
    u16 c08 = Pack(0xff, 0xd9, 0x13);
    u16 c09 = Pack(0xff, 0xd2, 0x47);
    u16 c10 = Pack(0xa1, 0x2b, 0xff);
    u16 c11 = Pack(0x45, 0x7c, 0xff);
    u16 c12 = Pack(0x45, 0xff, 0x45);
    u16 c13 = Pack(0xff, 0x26, 0x26);
    u16 c14 = Pack(0xff, 0x92, 0x2b);
    u16 c15 = Pack(0xd7, 0xd7, 0xd7);
    u16 c16 = Pack(0x37, 0x37, 0x37);
    u16 c17 = Pack(0xb4, 0x61, 0x39);
    u16 c18 = Pack(0x37, 0x30, 0x30);
    u16 c19 = Pack(0xa0, 0xa0, 0x27);
    for (i = 1; i < 9; i++) {
        buf[i] = c00;
    }
    for (i = 17; i < 37; i++) {
        buf[i] = c00;
    }
    buf[90] = c00;
    for (i = 195; i < 197; i++) {
        buf[i] = c00;
    }
    buf[199] = c00;
    buf[301] = c00;
    for (i = 9; i < 17; i++) {
        buf[i] = c01;
    }
    buf[91] = c01;
    for (i = 197; i < 199; i++) {
        buf[i] = c01;
    }
    for (i = 40; i < 74; i++) {
        buf[i] = c02;
    }
    for (i = 270; i < 282; i++) {
        buf[i] = c02;
    }
    for (i = 104; i < 116; i++) {
        buf[i] = c05;
    }
    for (i = 120; i < 124; i++) {
        buf[i] = c05;
    }
    for (i = 128; i < 140; i++) {
        buf[i] = c05;
    }
    for (i = 144; i < 156; i++) {
        buf[i] = c04;
    }
    for (i = 160; i < 164; i++) {
        buf[i] = c04;
    }
    for (i = 168; i < 180; i++) {
        buf[i] = c04;
    }
    for (i = 157; i < 159; i++) {
        buf[i] = c03;
    }
    for (i = 165; i < 167; i++) {
        buf[i] = c03;
    }
    buf[258] = c03;
    buf[264] = c03;
    for (i = 117; i < 119; i++) {
        buf[i] = c05;
    }
    FillSpan(0x7d, 0x7e, c05);
    buf[260] = c05;
    buf[266] = c05;
    FillSpan(0x11a, 0x11d, c06);
    buf[257] = c06;
    buf[259] = c06;
    FillSpan(0x105, 0x107, c06);
    buf[265] = c06;
    FillSpan(0x4d, 0x54, c07);
    FillSpan(0x11e, 0x126, c07);
    FillSpan(0xc9, 0xd1, c08);
    FillSpan(0xdd, 0xe0, c09);
    FillSpan(0xf1, 0xf2, c00);
    FillSpan(0xed, 0xee, c10);
    FillSpan(0xff, 0x100, c10);
    FillSpan(0xe1, 0xe4, c11);
    FillSpan(0xe5, 0xe8, c12);
    FillSpan(0xfb, 0xfc, c12);
    FillSpan(0xe9, 0xec, c13);
    FillSpan(0xfd, 0xfe, c13);
    FillSpan(0xef, 0xf0, c14);
    FillSpan(0xf7, 0xf8, c14);
    FillSpan(0xd9, 0xda, c15);
    FillSpan(0xf9, 0xfa, c15);
    FillSpan(0xf3, 0xf6, c16);
    FillSpan(0x12e, 0x143, c17);
    FillSpan(0xd5, 0xd6, c18);
    FillSpan(0xd7, 0xd8, c19);
    FillSpan(0x5c, 0x5f, c00);
    return 1;
}
// @early-stop
// Palette + store program re-decoded from the retail body and verified against it
// (see Shape1 for both defects: the CSE mis-read that had zeroed a channel in most
// colours, and the run-of-stores-is-a-loop fill expansion). Residual is Shape1's
// scheduler placement of the buf[257] store.
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
    u16 c06 = Pack(0x6e, 0x19, 0x46);
    u16 c07 = Pack(0xfc, 0xfc, 0xfc);
    u16 c08 = Pack(0xff, 0xd9, 0x13);
    u16 c09 = Pack(0xff, 0xd2, 0x47);
    u16 c10 = Pack(0xa1, 0x2b, 0xff);
    u16 c11 = Pack(0x45, 0x7c, 0xff);
    u16 c12 = Pack(0x45, 0xff, 0x45);
    u16 c13 = Pack(0xff, 0x26, 0x26);
    u16 c14 = Pack(0xff, 0x92, 0x2b);
    u16 c15 = Pack(0xd7, 0xd7, 0xd7);
    u16 c16 = Pack(0x37, 0x37, 0x37);
    u16 c17 = Pack(0xb4, 0x61, 0x39);
    u16 c18 = Pack(0x37, 0x30, 0x30);
    u16 c19 = Pack(0xa0, 0xa0, 0x27);
    for (i = 1; i < 9; i++) {
        buf[i] = c00;
    }
    for (i = 17; i < 37; i++) {
        buf[i] = c00;
    }
    buf[90] = c00;
    for (i = 195; i < 197; i++) {
        buf[i] = c00;
    }
    buf[199] = c00;
    buf[301] = c00;
    for (i = 9; i < 17; i++) {
        buf[i] = c01;
    }
    buf[91] = c01;
    for (i = 197; i < 199; i++) {
        buf[i] = c01;
    }
    for (i = 39; i < 75; i++) {
        buf[i] = c02;
    }
    for (i = 270; i < 282; i++) {
        buf[i] = c02;
    }
    for (i = 102; i < 114; i++) {
        buf[i] = c05;
    }
    for (i = 116; i < 122; i++) {
        buf[i] = c05;
    }
    for (i = 124; i < 139; i++) {
        buf[i] = c05;
    }
    for (i = 144; i < 156; i++) {
        buf[i] = c04;
    }
    for (i = 159; i < 164; i++) {
        buf[i] = c04;
    }
    for (i = 168; i < 180; i++) {
        buf[i] = c04;
    }
    for (i = 157; i < 159; i++) {
        buf[i] = c03;
    }
    for (i = 165; i < 167; i++) {
        buf[i] = c03;
    }
    buf[258] = c03;
    buf[264] = c03;
    for (i = 114; i < 116; i++) {
        buf[i] = c05;
    }
    FillSpan(0x7a, 0x7b, c05);
    buf[260] = c05;
    buf[266] = c05;
    FillSpan(0x11a, 0x11d, c06);
    buf[257] = c06;
    buf[259] = c06;
    FillSpan(0x105, 0x107, c06);
    buf[265] = c06;
    FillSpan(0x4d, 0x54, c07);
    FillSpan(0x11e, 0x126, c07);
    FillSpan(0xc9, 0xd1, c08);
    FillSpan(0xdd, 0xe0, c09);
    FillSpan(0xf1, 0xf2, c00);
    FillSpan(0xed, 0xee, c10);
    FillSpan(0xff, 0x100, c10);
    FillSpan(0xe1, 0xe4, c11);
    FillSpan(0xe5, 0xe8, c12);
    FillSpan(0xfb, 0xfc, c12);
    FillSpan(0xe9, 0xec, c13);
    FillSpan(0xfd, 0xfe, c13);
    FillSpan(0xef, 0xf0, c14);
    FillSpan(0xf7, 0xf8, c14);
    FillSpan(0xd9, 0xda, c15);
    FillSpan(0xf9, 0xfa, c15);
    FillSpan(0xf3, 0xf6, c16);
    FillSpan(0x12e, 0x143, c17);
    FillSpan(0xd5, 0xd6, c18);
    FillSpan(0xd7, 0xd8, c19);
    FillSpan(0x5c, 0x5f, c00);
    return 1;
}
// @early-stop
// Palette + store program re-decoded from the retail body and verified against it
// (see Shape1 for both defects: the CSE mis-read that had zeroed a channel in most
// colours, and the run-of-stores-is-a-loop fill expansion). Residual is Shape1's
// scheduler placement of the buf[257] store.
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
    u16 c10 = Pack(0xff, 0xd2, 0x47);
    u16 c11 = Pack(0xa1, 0x2b, 0xff);
    u16 c12 = Pack(0x45, 0x7c, 0xff);
    u16 c13 = Pack(0x45, 0xff, 0x45);
    u16 c14 = Pack(0xff, 0x26, 0x26);
    u16 c15 = Pack(0xff, 0x92, 0x2b);
    u16 c16 = Pack(0xd7, 0xd7, 0xd7);
    u16 c17 = Pack(0x37, 0x37, 0x37);
    u16 c18 = Pack(0xb4, 0x61, 0x39);
    u16 c19 = Pack(0x37, 0x30, 0x30);
    u16 c20 = Pack(0xa0, 0xa0, 0x27);
    for (i = 1; i < 9; i++) {
        buf[i] = c00;
    }
    for (i = 17; i < 37; i++) {
        buf[i] = c00;
    }
    buf[90] = c00;
    for (i = 195; i < 197; i++) {
        buf[i] = c00;
    }
    buf[199] = c00;
    buf[301] = c00;
    for (i = 9; i < 17; i++) {
        buf[i] = c01;
    }
    buf[91] = c01;
    for (i = 197; i < 199; i++) {
        buf[i] = c01;
    }
    for (i = 39; i < 75; i++) {
        buf[i] = c02;
    }
    for (i = 270; i < 282; i++) {
        buf[i] = c02;
    }
    for (i = 102; i < 114; i++) {
        buf[i] = c06;
    }
    for (i = 116; i < 122; i++) {
        buf[i] = c06;
    }
    for (i = 124; i < 139; i++) {
        buf[i] = c06;
    }
    for (i = 144; i < 156; i++) {
        buf[i] = c04;
    }
    for (i = 159; i < 164; i++) {
        buf[i] = c04;
    }
    for (i = 168; i < 180; i++) {
        buf[i] = c04;
    }
    for (i = 157; i < 159; i++) {
        buf[i] = c03;
    }
    for (i = 165; i < 167; i++) {
        buf[i] = c03;
    }
    buf[258] = c03;
    buf[264] = c03;
    for (i = 114; i < 116; i++) {
        buf[i] = c05;
    }
    for (i = 122; i < 124; i++) {
        buf[i] = c05;
    }
    buf[260] = c05;
    buf[266] = c05;
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
    FillSpan(0xed, 0xee, c11);
    FillSpan(0xff, 0x100, c11);
    FillSpan(0xe1, 0xe4, c12);
    FillSpan(0xe5, 0xe8, c13);
    FillSpan(0xfb, 0xfc, c13);
    FillSpan(0xe9, 0xec, c14);
    FillSpan(0xfd, 0xfe, c14);
    FillSpan(0xef, 0xf0, c15);
    FillSpan(0xf7, 0xf8, c15);
    FillSpan(0xd9, 0xda, c16);
    FillSpan(0xf9, 0xfa, c16);
    FillSpan(0xf3, 0xf6, c17);
    FillSpan(0x12e, 0x143, c18);
    FillSpan(0xd5, 0xd6, c19);
    FillSpan(0xd7, 0xd8, c20);
    buf[259] = c01;
    buf[265] = c00;
    FillSpan(0x5c, 0x5f, c00);
    return 1;
}
// @early-stop
// Palette + store program re-decoded from the retail body and verified against it
// (see Shape1 for both defects: the CSE mis-read that had zeroed a channel in most
// colours, and the run-of-stores-is-a-loop fill expansion). Residual is Shape1's
// scheduler placement of the buf[257] store.
RVA(0x000a7d50, 0x94f)
i32 CLightFxRender::Shape7() {
    u16* buf = m_buf;
    i32 i;
    u16 c00 = Pack(0x40, 0xb5, 0x13);
    u16 c01 = Pack(0x00, 0x7a, 0x2f);
    u16 c02 = Pack(0x68, 0x71, 0x7c);
    u16 c03 = Pack(0x6a, 0xb9, 0xff);
    u16 c04 = Pack(0x43, 0x85, 0xff);
    u16 c05 = Pack(0xc3, 0xc0, 0x73);
    u16 c06 = Pack(0x86, 0x8b, 0x7f);
    u16 c07 = Pack(0x78, 0x78, 0x5f);
    u16 c08 = Pack(0x81, 0x55, 0xf6);
    u16 c09 = Pack(0xff, 0xd9, 0x13);
    u16 c10 = Pack(0xff, 0xd2, 0x47);
    u16 c11 = Pack(0xa1, 0x2b, 0xff);
    u16 c12 = Pack(0x45, 0x7c, 0xff);
    u16 c13 = Pack(0x45, 0xff, 0x45);
    u16 c14 = Pack(0xff, 0x26, 0x26);
    u16 c15 = Pack(0xff, 0x92, 0x2b);
    u16 c16 = Pack(0xd7, 0xd7, 0xd7);
    u16 c17 = Pack(0x37, 0x37, 0x37);
    u16 c18 = Pack(0xb4, 0x61, 0x39);
    u16 c19 = Pack(0x37, 0x30, 0x30);
    u16 c20 = Pack(0xa0, 0xa0, 0x27);
    u16 c21 = Pack(0xe2, 0x70, 0x00);
    u16 c22 = Pack(0xa1, 0xf5, 0xff);
    u16 c23 = Pack(0xfd, 0xe5, 0x00);
    for (i = 1; i < 9; i++) {
        buf[i] = c00;
    }
    for (i = 17; i < 37; i++) {
        buf[i] = c00;
    }
    buf[90] = c00;
    for (i = 195; i < 197; i++) {
        buf[i] = c00;
    }
    buf[199] = c00;
    buf[301] = c00;
    for (i = 9; i < 17; i++) {
        buf[i] = c01;
    }
    buf[91] = c01;
    for (i = 197; i < 199; i++) {
        buf[i] = c01;
    }
    for (i = 39; i < 75; i++) {
        buf[i] = c02;
    }
    for (i = 270; i < 282; i++) {
        buf[i] = c02;
    }
    for (i = 102; i < 114; i++) {
        buf[i] = c06;
    }
    for (i = 116; i < 122; i++) {
        buf[i] = c06;
    }
    for (i = 124; i < 139; i++) {
        buf[i] = c06;
    }
    for (i = 144; i < 156; i++) {
        buf[i] = c04;
    }
    for (i = 159; i < 164; i++) {
        buf[i] = c04;
    }
    for (i = 168; i < 180; i++) {
        buf[i] = c04;
    }
    for (i = 157; i < 159; i++) {
        buf[i] = c03;
    }
    for (i = 165; i < 167; i++) {
        buf[i] = c03;
    }
    buf[258] = c03;
    buf[264] = c03;
    for (i = 114; i < 116; i++) {
        buf[i] = c05;
    }
    for (i = 122; i < 124; i++) {
        buf[i] = c05;
    }
    buf[260] = c05;
    buf[266] = c05;
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
    FillSpan(0xed, 0xee, c11);
    FillSpan(0xff, 0x100, c11);
    FillSpan(0xe1, 0xe4, c12);
    FillSpan(0xe5, 0xe8, c13);
    FillSpan(0xfb, 0xfc, c13);
    FillSpan(0xe9, 0xec, c14);
    FillSpan(0xfd, 0xfe, c14);
    FillSpan(0xef, 0xf0, c15);
    FillSpan(0xf7, 0xf8, c15);
    FillSpan(0xd9, 0xda, c16);
    FillSpan(0xf9, 0xfa, c16);
    FillSpan(0xf3, 0xf6, c17);
    FillSpan(0x12e, 0x143, c18);
    FillSpan(0xd5, 0xd6, c19);
    FillSpan(0xd7, 0xd8, c20);
    buf[257] = c21;
    buf[259] = c21;
    FillSpan(0x105, 0x106, c22);
    buf[263] = c23;
    buf[265] = c23;
    FillSpan(0x5c, 0x5f, c00);
    return 1;
}
// @early-stop
// Palette + store program re-decoded from the retail body and verified against it
// (see Shape1 for both defects: the CSE mis-read that had zeroed a channel in most
// colours, and the run-of-stores-is-a-loop fill expansion). Residual is Shape1's
// scheduler placement of the buf[257] store.
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
    u16 c06 = Pack(0x78, 0x78, 0x5f);
    u16 c07 = Pack(0x94, 0xa7, 0xbd);
    u16 c08 = Pack(0xff, 0xd9, 0x13);
    u16 c09 = Pack(0xff, 0xd2, 0x47);
    u16 c10 = Pack(0xa1, 0x2b, 0xff);
    u16 c11 = Pack(0x45, 0x7c, 0xff);
    u16 c12 = Pack(0x45, 0xff, 0x45);
    u16 c13 = Pack(0xff, 0x26, 0x26);
    u16 c14 = Pack(0xff, 0x92, 0x2b);
    u16 c15 = Pack(0xd7, 0xd7, 0xd7);
    u16 c16 = Pack(0x37, 0x37, 0x37);
    u16 c17 = Pack(0xb4, 0x61, 0x39);
    u16 c18 = Pack(0x37, 0x30, 0x30);
    u16 c19 = Pack(0xa0, 0xa0, 0x27);
    u16 c20 = Pack(0x12, 0xd2, 0x18);
    u16 c21 = Pack(0x00, 0x72, 0xe4);
    u16 c22 = Pack(0xe4, 0x00, 0x26);
    for (i = 1; i < 9; i++) {
        buf[i] = c00;
    }
    for (i = 17; i < 37; i++) {
        buf[i] = c00;
    }
    buf[90] = c00;
    for (i = 195; i < 197; i++) {
        buf[i] = c00;
    }
    buf[199] = c00;
    buf[301] = c00;
    for (i = 9; i < 17; i++) {
        buf[i] = c01;
    }
    buf[91] = c01;
    for (i = 197; i < 199; i++) {
        buf[i] = c01;
    }
    for (i = 39; i < 75; i++) {
        buf[i] = c02;
    }
    for (i = 270; i < 282; i++) {
        buf[i] = c02;
    }
    for (i = 102; i < 114; i++) {
        buf[i] = c05;
    }
    for (i = 116; i < 122; i++) {
        buf[i] = c05;
    }
    for (i = 124; i < 139; i++) {
        buf[i] = c05;
    }
    for (i = 144; i < 156; i++) {
        buf[i] = c04;
    }
    for (i = 159; i < 164; i++) {
        buf[i] = c04;
    }
    for (i = 168; i < 180; i++) {
        buf[i] = c04;
    }
    for (i = 157; i < 159; i++) {
        buf[i] = c03;
    }
    for (i = 165; i < 167; i++) {
        buf[i] = c03;
    }
    buf[258] = c03;
    buf[264] = c03;
    for (i = 114; i < 116; i++) {
        buf[i] = c05;
    }
    for (i = 122; i < 124; i++) {
        buf[i] = c05;
    }
    buf[260] = c05;
    buf[266] = c05;
    FillSpan(0x11a, 0x11d, c06);
    buf[257] = c06;
    buf[259] = c06;
    FillSpan(0x105, 0x107, c06);
    buf[265] = c06;
    FillSpan(0x4d, 0x54, c07);
    FillSpan(0x11e, 0x126, c07);
    FillSpan(0xc9, 0xd1, c08);
    FillSpan(0xdd, 0xe0, c09);
    FillSpan(0xf1, 0xf2, c00);
    FillSpan(0xed, 0xee, c10);
    FillSpan(0xff, 0x100, c10);
    FillSpan(0xe1, 0xe4, c11);
    FillSpan(0xe5, 0xe8, c12);
    FillSpan(0xfb, 0xfc, c12);
    FillSpan(0xe9, 0xec, c13);
    FillSpan(0xfd, 0xfe, c13);
    FillSpan(0xef, 0xf0, c14);
    FillSpan(0xf7, 0xf8, c14);
    FillSpan(0xd9, 0xda, c15);
    FillSpan(0xf9, 0xfa, c15);
    FillSpan(0xf3, 0xf6, c16);
    FillSpan(0x12e, 0x143, c17);
    FillSpan(0xd5, 0xd6, c18);
    FillSpan(0xd7, 0xd8, c19);
    buf[257] = c20;
    buf[259] = c20;
    FillSpan(0x105, 0x106, c21);
    buf[263] = c22;
    buf[265] = c22;
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
RVA(0x000a9660, 0xca)
i32 CLightFxRender::ClampRect(i32 x, i32 y, i32* out, i32 margin) {
    if (x < m_srcRect.left || x > m_srcRect.right || y < m_srcRect.top || y > m_srcRect.bottom) {
        return 0;
    }
    if (margin > 0) {
        if (x < m_dstRect.left && m_dstRect.left - x <= margin) {
            x = m_dstRect.left;
        }
        if (x > m_dstRect.right && x - m_dstRect.right <= margin) {
            x = m_dstRect.right;
        }
        if (y < m_dstRect.top && m_dstRect.top - y <= margin) {
            y = m_dstRect.top;
        }
        if (y > m_dstRect.bottom && y - m_dstRect.bottom <= margin) {
            y = m_dstRect.bottom;
        }
    }
    if (x < m_dstRect.left || x > m_dstRect.right || y < m_dstRect.top || y > m_dstRect.bottom) {
        return 0;
    }
    // Retail STORES each difference into out[] and READS IT BACK for the divide
    // (`mov [esi],edx / ... / mov eax,[esi] / cdq / idiv / mov [esi],eax`), so the
    // subtract and the divide are two statements - fusing them into one expression
    // keeps the value in a register and drops both reloads.
    // docs/patterns/member-store-direct-not-via-temporary.md
    out[0] = x - m_dstRect.left;
    out[1] = y - m_dstRect.top;
    out[0] /= m_scale;
    out[1] /= m_scale;
    return 1;
}
