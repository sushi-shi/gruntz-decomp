#include <rva.h>

#include <Gruntz/LightFxRender.h>

#include <Mfc.h>

#include <DDrawMgr/DDrawPtrCollections.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDSurface.h>
#include <DDrawMgr/PixelShift.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LevelArea.h>
#include <Gruntz/Play.h>
#include <Gruntz/SpriteRefTable.h>
#include <Gruntz/SpriteTeamColorVariant.h>
#include <Gruntz/TriggerMgr.h>
#include <Pix16.h>
#include <Rez/FrameClock.h>
#include <Wap32/TileGeometry.h>

#include <ddraw.h>

static inline SIZE
GridSize(const CGruntzMapMgr* grid) {
    SIZE
    s;
    s.cx = grid->m_width;
    s.cy = grid->m_height;
    return s;
}

static inline i32 PixOffset(const CDDSurface* s, i32 x, i32 y) {
    return y * s->m_pitch + x * s->m_bytesPerPixel;
}

static inline i32 OccupantAt(const CGruntzMapMgr* g, u32 x, u32 y) {
    if (x < g->m_width && y < g->m_height) {
        return g->m_rows[y][x].m_occupantId;
    }
    return -1;
}

static inline i32 TileIdAt(const CGruntzMapMgr* g, u32 x, u32 y) {
    if (x < g->m_width && y < g->m_height) {
        return g->m_rows[y][x].m_tileId;
    }
    return 0;
}

static inline u16 Pack(i32 r, i32 g, i32 b) {
    return static_cast<u16>(
        (((r >> g_rDown) << g_rUp) | ((g >> g_gDown) << g_gUp) | (b >> g_bDown))
    );
}

RVA(0x000a32c0, 0x72)
i32 CLightFxRender::Init(CGruntzMgr* mgr, i32 refreshInterval) {
    if (mgr == NULL) {
        return 0;
    }
    m_mgr = mgr;
    m_cmdGrid = mgr->m_cmdGrid;
    m_tileGrid = mgr->m_tileGrid;
    m_world = mgr->m_world;
    m_refreshInterval = refreshInterval;
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
void CLightFxRender::Reset() {
    FreeSurface();
    m_mgr = NULL;
    m_cmdGrid = NULL;
    m_tileGrid = NULL;
    m_world = NULL;
    m_surface = NULL;
    m_handle = 0;
    m_refreshInterval = 0;
    m_refreshRemaining = 0;
}

RVA(0x000a33a0, 0x23)
void CLightFxRender::FreeSurface() {
    if (m_world != NULL && m_surface != NULL) {
        m_world->m_ptrColl->RemoveItemA(m_surface);
        m_surface = NULL;
    }
}

RVA(0x000a33e0, 0x55)
i32 CLightFxRender::AllocSurface() {
    if (m_tileGrid == NULL) {
        return 0;
    }
    if (m_world == NULL) {
        return 0;
    }
    FreeSurface();
    CGruntzMapMgr* info = m_tileGrid;
    CDDrawSurfaceMgr* mgr = m_world;

    SIZE
    dims = GridSize(info);
    m_surface = mgr->m_ptrColl->MakeAndAddB(dims.cx, dims.cy, BPP_UNSET, 0, -1);
    if (m_surface == NULL) {
        return 0;
    }
    m_surface->Clear(0);
    return 1;
}

RVA(0x000a3460, 0x2f3)
i32 CLightFxRender::Resize(i32 delta, i32 rebuild) {
    if (rebuild == 0) {

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
    if (m_surface == NULL) {
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
    char* base = static_cast<char*>(m_surface->Lock(0));
    if (base == NULL) {
        return 0;
    }
    for (u32 y = 0; y < m_tileGrid->m_height; y++) {
        for (u32 x = 0; x < m_tileGrid->m_width; x++) {
            u16* dst = Pix16(base + y * m_surface->m_pitch + x * m_surface->m_bytesPerPixel);
            i32 tile = OccupantAt(m_tileGrid, x, y);

            if (tile != -1) {

                CGrunt* desc = m_cmdGrid->m_grid[(tile & 0xff) + ((tile >> 8) & 0xff) * 15];
                if (desc == NULL) {
                    continue;
                }
                SpriteTeamColorVariant alt = SPRITE_TEAM_COLOR_PRIMARY;
                if (desc->m_arrived != 0) {
                    alt = SPRITE_TEAM_COLOR_SECONDARY;
                }

                if (static_cast<i64>(g_frameTime) - desc->m_combatClock64 >= desc->m_combatTimeout64
                    || desc->m_tileOwnerHi != g_curPlayer) {
                    CSpriteRef* node = m_mgr->m_spriteFactory->GetTool(IDX(desc->m_moveIcon));
                    if (node == NULL) {
                        *dst = 0;
                        continue;
                    }

                    switch (alt) {
                        case SPRITE_TEAM_COLOR_PRIMARY:
                            *dst = node->m_teamColor1;
                            break;
                        case SPRITE_TEAM_COLOR_SECONDARY:
                            *dst = node->m_teamColor2;
                            break;
                        case SPRITE_TEAM_COLOR_TERTIARY:
                            *dst = node->m_teamColor3;
                            break;
                        default:
                            *dst = node->m_teamColor1;
                            break;
                    }
                } else if (static_cast<u32>(g_timer100) < 0x32) {

                    i32 idx = TileIdAt(m_tileGrid, x, y);
                    if (static_cast<u32>(idx) >= MINIMAP_TILE_COLOR_COUNT) {
                        *dst = 0;
                    } else {
                        *dst = m_buf[idx];
                    }
                } else {
                    CSpriteRef* node = m_mgr->m_spriteFactory->GetTool(IDX(desc->m_moveIcon));
                    if (node == NULL) {
                        *dst = 0;
                        continue;
                    }
                    *dst = node->m_teamColor2;
                }
            } else {
                i32 idx = TileIdAt(m_tileGrid, x, y);
                u16 c;
                if (static_cast<u32>(idx) >= MINIMAP_TILE_COLOR_COUNT) {
                    c = 0;
                } else {
                    c = m_buf[idx];
                }
                *dst = c;
            }
        }
    }
    m_surface->m_ddSurface->Unlock(0);
    return 1;
}

RVA(0x000a3820, 0x18e)
i32 CLightFxRender::ComputeRect(CDDrawSurfacePair* ctx, RECT* src) {
    if (m_surface == NULL) {
        return 0;
    }
    m_srcRect = *src;
    i32 sl = src->left;
    i32 w = src->right - sl + 1;
    i32 st = src->top;
    i32 h = src->bottom - st + 1;

    i32 cx = sl + w / 2;
    i32 cy = st + h / 2;
    i32 qx = w / m_surface->m_width;
    i32 qy = h / m_surface->m_height;

    i32 scale = qy;
    if (qx < qy) {
        scale = qx;
    }

    i32 s = 3;
    if (scale <= 3) {
        s = scale;
    }
    m_scale = s;
    i32 dl = cx - m_surface->m_width * s / 2;
    i32 dt = cy - m_surface->m_height * s / 2;
    RECT* dstRect = &m_dstRect;
    dstRect->left = dl;
    dstRect->top = dt;
    dstRect->right = m_surface->m_width * s + dl;
    dstRect->bottom = m_surface->m_height * s + dt;
    if (ctx->m_surface->BltEx(dstRect, m_surface, 0, 0x1000000, 0) != 0) {
        return 0;
    }

    RECT* vr = &m_world->m_level->m_mainPlane->m_viewRect;
    RECT box;
    box.left = vr->left >> TILE_SHIFT_PX;
    box.top = vr->top >> TILE_SHIFT_PX;
    box.right = vr->right >> TILE_SHIFT_PX;
    box.bottom = vr->bottom >> TILE_SHIFT_PX;
    if (m_scale != 1) {

        box.left *= m_scale;
        box.top *= m_scale;
        box.right *= m_scale;
        box.bottom *= m_scale;
        i32 extension = m_scale - 1;
        box.right += extension;
        box.bottom += extension;
    }
    box.left += dstRect->left;
    box.right += dstRect->left;
    box.top += dstRect->top;
    box.bottom += dstRect->top;
    DrawBorder(&box, ctx, 0xffff);
    return 1;
}

// @early-stop commutative pitch/bpp term order inside the PixOffset expansions
// (canonical operand order, TU-state class); the side-edge loop itself matches -
// stepping each cursor directly after its store keeps both IVs live.
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000a3a20, 0xe2)
void CLightFxRender::DrawBorderRaw(RECT* r, char* base, i32 color) {
    i32 w = r->right - r->left + 1;

    u16* tp = Pix16(base + PixOffset(m_surface, r->left, r->top));
    for (i32 t = 0; t < w; t++) {
        tp[t] = static_cast<u16>(color);
    }

    u16* bp = Pix16(base + PixOffset(m_surface, r->left, r->bottom));
    for (i32 b = 0; b < w; b++) {
        bp[b] = static_cast<u16>(color);
    }

    i32 h = r->bottom - r->top + 1;
    i32 lo = PixOffset(m_surface, r->left, r->top);
    i32 ro = PixOffset(m_surface, r->right, r->top);
    i32 step = m_surface->m_pitch;

    if (h > 0) {
        char* lp = base + lo;
        char* rp = base + ro;
        i32 v = h;
        while (v != 0) {
            *Pix16(lp) = static_cast<u16>(color);
            lp += step;
            *Pix16(rp) = static_cast<u16>(color);
            rp += step;
            v--;
        }
    }
}

RVA(0x000a3b50, 0xfa)
void CLightFxRender::DrawBorder(RECT* r, CDDrawSurfacePair* ctx, i32 color) {
    CDDSurface* surf = ctx->m_surface;
    char* base = static_cast<char*>(surf->Lock(0));
    if (base == NULL) {
        return;
    }
    i32 w = r->right - r->left + 1;

    u16* tp = Pix16(base + r->top * surf->m_pitch + r->left * surf->m_bytesPerPixel);
    for (i32 t = 0; t < w; t++) {
        tp[t] = static_cast<u16>(color);
    }

    u16* bp = Pix16(base + r->bottom * surf->m_pitch + r->left * surf->m_bytesPerPixel);
    for (i32 b = 0; b < w; b++) {
        bp[b] = static_cast<u16>(color);
    }

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

    surf->m_ddSurface->Unlock(0);
}

RVA(0x000a3c90, 0xe8)
i32 CLightFxRender::BuildShape(LevelArea shape) {
    if (shape > AREA_LAST) {
        return 0;
    }
    memset(m_buf, 0, sizeof(m_buf));
    switch (shape) {
        case AREA_ROCKY_ROADZ:
            if (!BuildRockyRoadzPalette()) {
                return 0;
            }
            break;
        case AREA_GRUNTZICLEZ:
            if (!BuildGruntziclezPalette()) {
                return 0;
            }
            break;
        case AREA_TROUBLE_IN_THE_TROPICZ:
            if (!BuildTropiczPalette()) {
                return 0;
            }
            break;
        case AREA_HIGH_ON_SWEETZ:
            if (!BuildHighOnSweetzPalette()) {
                return 0;
            }
            break;
        case AREA_HIGH_ROLLERZ:
            if (!BuildHighRollerzPalette()) {
                return 0;
            }
            break;
        case AREA_HONEY_I_SHRUNK_THE_GRUNTZ:
            if (!BuildHoneyPalette()) {
                return 0;
            }
            break;
        case AREA_MINIATURE_MASTERZ:
            if (!BuildMiniatureMasterzPalette()) {
                return 0;
            }
            break;
        case AREA_GRUNTZ_IN_SPACE:
            if (!BuildSpacePalette()) {
                return 0;
            }
            break;
    }
    m_refreshRemaining = 0;
    return 1;
}

// @early-stop
// One member store sits one push earlier in retail inside the FillSpan argument
// setup. Swapping or moving the two stores is byte-identical; dropping the
// `u16* buf = m_buf` cursor costs 80 diff lines and the container-object shape
// recovers only 2 of those - the cursor is retail's own.
// docs/patterns/member-array-is-a-container-object.md (counter-example)
RVA(0x000a3dc0, 0x85f)
i32 CLightFxRender::BuildRockyRoadzPalette() {
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

// @early-stop
// One member store sits one push earlier in retail inside the FillSpan argument
// setup. Swapping or moving the two stores is byte-identical; dropping the
// `u16* buf = m_buf` cursor costs 80 diff lines and the container-object shape
// recovers only 2 of those - the cursor is retail's own.
// docs/patterns/member-array-is-a-container-object.md (counter-example)
RVA(0x000a4890, 0x852)
i32 CLightFxRender::BuildGruntziclezPalette() {
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
// One member store sits one push earlier in retail inside the FillSpan argument
// setup. Swapping or moving the two stores is byte-identical; dropping the
// `u16* buf = m_buf` cursor costs 80 diff lines and the container-object shape
// recovers only 2 of those - the cursor is retail's own.
// docs/patterns/member-array-is-a-container-object.md (counter-example)
RVA(0x000a5310, 0x855)
i32 CLightFxRender::BuildTropiczPalette() {
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
RVA(0x000a5d90, 0x825)
i32 CLightFxRender::BuildHighOnSweetzPalette() {
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
// One member store sits one push earlier in retail inside the FillSpan argument
// setup. Swapping or moving the two stores is byte-identical; dropping the
// `u16* buf = m_buf` cursor costs 80 diff lines and the container-object shape
// recovers only 2 of those - the cursor is retail's own.
// docs/patterns/member-array-is-a-container-object.md (counter-example)
RVA(0x000a67d0, 0x864)
i32 CLightFxRender::BuildHighRollerzPalette() {
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
RVA(0x000a7260, 0x8c0)
i32 CLightFxRender::BuildHoneyPalette() {
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
RVA(0x000a7d50, 0x94f)
i32 CLightFxRender::BuildMiniatureMasterzPalette() {
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
// One member store sits one push earlier in retail inside the FillSpan argument
// setup. Swapping or moving the two stores is byte-identical; dropping the
// `u16* buf = m_buf` cursor costs 80 diff lines and the container-object shape
// recovers only 2 of those - the cursor is retail's own.
// docs/patterns/member-array-is-a-container-object.md (counter-example)
RVA(0x000a8900, 0x926)
i32 CLightFxRender::BuildSpacePalette() {
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
i32 CLightFxRender::BeginMinimapPan(i32, i32 x, i32 y) {
    i32 cell[2];
    if (!ClampRect(x, y, cell, 0x20)) {
        return 0;
    }

    CPlay* ctx = static_cast<CPlay*>(m_mgr->m_curState);
    if (ctx != NULL) {
        ctx->ResetGoals(cell[0] * 32 + 16, cell[1] * 32 + 16);
    }
    m_handle = 1;
    return 1;
}

RVA(0x000a9500, 0x16)
i32 CLightFxRender::EndMinimapPan(i32, i32, i32) {
    if (m_handle != 0) {
        m_handle = 0;
    }
    return 1;
}

// @identity-TODO: the minimap-handler ABI and false result are proven; the event identity is not.
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000a9530, 0x5)
i32 CLightFxRender::IgnoreMinimapEvent(i32, i32, i32) {
    return 0;
}

RVA(0x000a9550, 0x5b)
i32 CLightFxRender::IssueMinimapCommand(i32, i32 x, i32 y) {
    i32 cell[2];
    if (!ClampRect(x, y, cell, 0x20)) {
        return 0;
    }
    g_gameReg->m_cmdGrid
        ->ResetGroup(cell[0] * 32 + 16, cell[1] * 32 + 16, 0, 0, 0, TARGET_SELECTION_AUTO, 1);
    return 1;
}

RVA(0x000a95d0, 0x69)
i32 CLightFxRender::ContinueMinimapPan(i32, i32 x, i32 y) {
    if (m_handle == 0) {
        return 0;
    }
    i32 cell[2];
    if (!ClampRect(x, y, cell, 0x20)) {
        return 0;
    }
    CPlay* ctx = static_cast<CPlay*>(m_mgr->m_curState);
    if (ctx != NULL) {
        ctx->ResetGoals(cell[0] * 32 + 16, cell[1] * 32 + 16);
    }
    return 1;
}

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

    out[0] = x - m_dstRect.left;
    out[1] = y - m_dstRect.top;
    out[0] /= m_scale;
    out[1] /= m_scale;
    return 1;
}
