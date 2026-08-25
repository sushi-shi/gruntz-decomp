#include <rva.h>

#include <Gruntz/Minimap.h>

#include <Mfc.h>

#include <DDrawMgr/DDrawDeviceManager.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDSurface.h>
#include <DDrawMgr/PixelShift.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntIdentity.h>
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
GridSize(const CGruntzMapMgr* mapMgr) {
    SIZE
    size;
    size.cx = mapMgr->m_width;
    size.cy = mapMgr->m_height;
    return size;
}

static inline i32 PixOffset(const CDDSurface* surface, i32 x, i32 y) {
    return y * surface->m_pitch + x * surface->m_bytesPerPixel;
}

static inline i32 OccupantAt(const CGruntzMapMgr* mapMgr, u32 x, u32 y) {
    if (x < mapMgr->m_width && y < mapMgr->m_height) {
        return mapMgr->m_rows[y][x].m_occupantId;
    }
    return -1;
}

static inline i32 TileIdAt(const CGruntzMapMgr* mapMgr, u32 x, u32 y) {
    if (x < mapMgr->m_width && y < mapMgr->m_height) {
        return mapMgr->m_rows[y][x].m_tileId;
    }
    return 0;
}

static inline u16 Pack(i32 r, i32 g, i32 b) {
    return static_cast<u16>(
        (((r >> g_rDown) << g_rUp) | ((g >> g_gDown) << g_gUp) | (b >> g_bDown))
    );
}

RVA(0x000a32c0, 0x72)
i32 CMinimap::Init(CGruntzMgr* gameMgr, i32 refreshIntervalMs) {
    if (gameMgr == NULL) {
        return 0;
    }
    m_gameMgr = gameMgr;
    m_triggerMgr = gameMgr->m_triggerMgr;
    m_mapMgr = gameMgr->m_tileGrid;
    m_world = gameMgr->m_world;
    m_refreshInterval = refreshIntervalMs;
    m_cellScale = 1;
    m_refreshRemaining = 0;
    if (!AllocSurface()) {
        return 0;
    }
    m_drawRect.left = 0;
    m_drawRect.top = 0;
    m_drawRect.right = 0;
    m_drawRect.bottom = 0;
    m_boundsRect.left = 0;
    m_boundsRect.top = 0;
    m_boundsRect.right = 0;
    m_boundsRect.bottom = 0;
    return 1;
}

RVA(0x000a3360, 0x29)
void CMinimap::Reset() {
    FreeSurface();
    m_gameMgr = NULL;
    m_triggerMgr = NULL;
    m_mapMgr = NULL;
    m_world = NULL;
    m_surface = NULL;
    m_panActive = 0;
    m_refreshInterval = 0;
    m_refreshRemaining = 0;
}

RVA(0x000a33a0, 0x23)
void CMinimap::FreeSurface() {
    if (m_world != NULL && m_surface != NULL) {
        m_world->m_deviceManager->RemoveSurface(m_surface);
        m_surface = NULL;
    }
}

RVA(0x000a33e0, 0x55)
i32 CMinimap::AllocSurface() {
    if (m_mapMgr == NULL) {
        return 0;
    }
    if (m_world == NULL) {
        return 0;
    }
    FreeSurface();
    CGruntzMapMgr* mapMgr = m_mapMgr;
    CDDrawSurfaceMgr* world = m_world;

    SIZE
    size = GridSize(mapMgr);
    m_surface = world->m_deviceManager->CreateOffscreenSurface(size.cx, size.cy, BPP_UNSET, 0, -1);
    if (m_surface == NULL) {
        return 0;
    }
    m_surface->Clear(0);
    return 1;
}

RVA(0x000a3460, 0x2f3)
i32 CMinimap::Refresh(i32 elapsedMs, i32 forceRefresh) {
    if (forceRefresh == 0) {

        if (static_cast<u32>(elapsedMs) >= static_cast<u32>(m_refreshRemaining)) {
            m_refreshRemaining = 0;
        } else {
            m_refreshRemaining -= elapsedMs;
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
    if (m_surface->m_width != static_cast<i32>(m_mapMgr->m_width)
        || m_surface->m_height != static_cast<i32>(m_mapMgr->m_height)) {
        if (!AllocSurface()) {
            return 0;
        }
    }
    char* pixels = static_cast<char*>(m_surface->Lock(NULL));
    if (pixels == NULL) {
        return 0;
    }
    for (u32 y = 0; y < m_mapMgr->m_height; y++) {
        for (u32 x = 0; x < m_mapMgr->m_width; x++) {
            u16* pixel = Pix16(pixels + y * m_surface->m_pitch + x * m_surface->m_bytesPerPixel);
            i32 occupantId = OccupantAt(m_mapMgr, x, y);

            if (occupantId != -1) {

                CGrunt* grunt = m_triggerMgr->m_units
                                    [(occupantId & GRUNT_IDENTITY_COMPONENT_MASK)
                                     + ((occupantId >> GRUNT_IDENTITY_PLAYER_SHIFT)
                                        & GRUNT_IDENTITY_COMPONENT_MASK)
                                           * TM_UNITS_PER_PLAYER];
                if (grunt == NULL) {
                    continue;
                }
                SpriteTeamColorVariant teamColor = SPRITE_TEAM_COLOR_PRIMARY;
                if (grunt->m_arrived != 0) {
                    teamColor = SPRITE_TEAM_COLOR_SECONDARY;
                }

                if (static_cast<i64>(g_frameTime) - grunt->m_combatClock64
                        >= grunt->m_combatTimeout64
                    || grunt->m_playerIndex != g_curPlayer) {
                    CSpriteRef* spriteRef =
                        m_gameMgr->m_spriteFactory->GetTool(IDX(grunt->m_moveIcon));
                    if (spriteRef == NULL) {
                        *pixel = 0;
                        continue;
                    }

                    switch (teamColor) {
                        case SPRITE_TEAM_COLOR_PRIMARY:
                            *pixel = spriteRef->m_teamColor1;
                            break;
                        case SPRITE_TEAM_COLOR_SECONDARY:
                            *pixel = spriteRef->m_teamColor2;
                            break;
                        case SPRITE_TEAM_COLOR_TERTIARY:
                            *pixel = spriteRef->m_teamColor3;
                            break;
                        default:
                            *pixel = spriteRef->m_teamColor1;
                            break;
                    }
                } else if (static_cast<u32>(g_period100CountdownMs)
                           < MINIMAP_COMBAT_BLINK_PHASE_MS) {

                    i32 tileId = TileIdAt(m_mapMgr, x, y);
                    if (static_cast<u32>(tileId) >= MINIMAP_TILE_COLOR_COUNT) {
                        *pixel = 0;
                    } else {
                        *pixel = m_tileColors[tileId];
                    }
                } else {
                    CSpriteRef* spriteRef =
                        m_gameMgr->m_spriteFactory->GetTool(IDX(grunt->m_moveIcon));
                    if (spriteRef == NULL) {
                        *pixel = 0;
                        continue;
                    }
                    *pixel = spriteRef->m_teamColor2;
                }
            } else {
                i32 tileId = TileIdAt(m_mapMgr, x, y);
                u16 color;
                if (static_cast<u32>(tileId) >= MINIMAP_TILE_COLOR_COUNT) {
                    color = 0;
                } else {
                    color = m_tileColors[tileId];
                }
                *pixel = color;
            }
        }
    }
    m_surface->m_ddSurface->Unlock(NULL);
    return 1;
}

RVA(0x000a3820, 0x18e)
i32 CMinimap::Draw(CDDrawSurfacePair* target, RECT* bounds) {
    if (m_surface == NULL) {
        return 0;
    }
    m_boundsRect = *bounds;
    i32 left = bounds->left;
    i32 width = bounds->right - left + 1;
    i32 top = bounds->top;
    i32 height = bounds->bottom - top + 1;

    i32 centerX = left + width / 2;
    i32 centerY = top + height / 2;
    i32 scaleX = width / m_surface->m_width;
    i32 scaleY = height / m_surface->m_height;

    i32 scale = scaleY;
    if (scaleX < scaleY) {
        scale = scaleX;
    }

    i32 cellScale = MINIMAP_MAX_CELL_SCALE;
    if (scale <= MINIMAP_MAX_CELL_SCALE) {
        cellScale = scale;
    }
    m_cellScale = cellScale;
    i32 drawLeft = centerX - m_surface->m_width * cellScale / 2;
    i32 drawTop = centerY - m_surface->m_height * cellScale / 2;
    RECT* dstRect = &m_drawRect;
    dstRect->left = drawLeft;
    dstRect->top = drawTop;
    dstRect->right = m_surface->m_width * cellScale + drawLeft;
    dstRect->bottom = m_surface->m_height * cellScale + drawTop;
    if (target->m_surface->BltEx(dstRect, m_surface, NULL, DDBLT_WAIT, NULL) != 0) {
        return 0;
    }

    RECT* vr = &m_world->m_level->m_mainPlane->m_planeViewRect;
    RECT box;
    box.left = vr->left >> TILE_SHIFT_PX;
    box.top = vr->top >> TILE_SHIFT_PX;
    box.right = vr->right >> TILE_SHIFT_PX;
    box.bottom = vr->bottom >> TILE_SHIFT_PX;
    if (m_cellScale != 1) {

        box.left *= m_cellScale;
        box.top *= m_cellScale;
        box.right *= m_cellScale;
        box.bottom *= m_cellScale;
        i32 extension = m_cellScale - 1;
        box.right += extension;
        box.bottom += extension;
    }
    box.left += dstRect->left;
    box.right += dstRect->left;
    box.top += dstRect->top;
    box.bottom += dstRect->top;
    DrawBorder(&box, target, MINIMAP_BORDER_COLOR_16);
    return 1;
}

// @early-stop
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000a3a20, 0xe2)
void CMinimap::DrawBorderRaw(RECT* rect, char* pixels, i32 color) {
    i32 width = rect->right - rect->left + 1;

    u16* topPixels = Pix16(pixels + PixOffset(m_surface, rect->left, rect->top));
    for (i32 topX = 0; topX < width; topX++) {
        topPixels[topX] = static_cast<u16>(color);
    }

    u16* bottomPixels = Pix16(pixels + PixOffset(m_surface, rect->left, rect->bottom));
    for (i32 bottomX = 0; bottomX < width; bottomX++) {
        bottomPixels[bottomX] = static_cast<u16>(color);
    }

    i32 height = rect->bottom - rect->top + 1;
    i32 leftOffset = PixOffset(m_surface, rect->left, rect->top);
    i32 rightOffset = PixOffset(m_surface, rect->right, rect->top);
    i32 rowStride = m_surface->m_pitch;

    if (height > 0) {
        char* leftPixel = pixels + leftOffset;
        char* rightPixel = pixels + rightOffset;
        i32 rowsRemaining = height;
        while (rowsRemaining != 0) {
            *Pix16(leftPixel) = static_cast<u16>(color);
            leftPixel += rowStride;
            *Pix16(rightPixel) = static_cast<u16>(color);
            rightPixel += rowStride;
            rowsRemaining--;
        }
    }
}

RVA(0x000a3b50, 0xfa)
void CMinimap::DrawBorder(RECT* rect, CDDrawSurfacePair* target, i32 color) {
    CDDSurface* surface = target->m_surface;
    char* pixels = static_cast<char*>(surface->Lock(NULL));
    if (pixels == NULL) {
        return;
    }
    i32 width = rect->right - rect->left + 1;

    u16* topPixels =
        Pix16(pixels + rect->top * surface->m_pitch + rect->left * surface->m_bytesPerPixel);
    for (i32 topX = 0; topX < width; topX++) {
        topPixels[topX] = static_cast<u16>(color);
    }

    u16* bottomPixels =
        Pix16(pixels + rect->bottom * surface->m_pitch + rect->left * surface->m_bytesPerPixel);
    for (i32 bottomX = 0; bottomX < width; bottomX++) {
        bottomPixels[bottomX] = static_cast<u16>(color);
    }

    i32 height = rect->bottom - rect->top + 1;
    i32 leftOffset = rect->left * surface->m_bytesPerPixel + rect->top * surface->m_pitch;
    i32 rightOffset = rect->right * surface->m_bytesPerPixel + rect->top * surface->m_pitch;
    i32 rowStride = surface->m_pitch;
    for (i32 y = 0; y < height; y++) {
        *Pix16(pixels + leftOffset) = static_cast<u16>(color);
        *Pix16(pixels + rightOffset) = static_cast<u16>(color);
        leftOffset += rowStride;
        rightOffset += rowStride;
    }

    surface->m_ddSurface->Unlock(NULL);
}

RVA(0x000a3c90, 0xe8)
i32 CMinimap::SetAreaPalette(LevelArea area) {
    if (area > AREA_LAST) {
        return 0;
    }
    memset(m_tileColors, 0, sizeof(m_tileColors));
    switch (area) {
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
RVA(0x000a3dc0, 0x85f)
i32 CMinimap::BuildRockyRoadzPalette() {
    u16* buf = m_tileColors;
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
void CMinimap::FillSpan(u32 x1, u32 x2, u16 color) {
    if (x1 > x2) {
        return;
    }
    for (u32 i = x1; i <= x2; i++) {
        m_tileColors[i] = color;
    }
}

// @early-stop
RVA(0x000a4890, 0x852)
i32 CMinimap::BuildGruntziclezPalette() {
    u16* buf = m_tileColors;
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
RVA(0x000a5310, 0x855)
i32 CMinimap::BuildTropiczPalette() {
    u16* buf = m_tileColors;
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
i32 CMinimap::BuildHighOnSweetzPalette() {
    u16* buf = m_tileColors;
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
RVA(0x000a67d0, 0x864)
i32 CMinimap::BuildHighRollerzPalette() {
    u16* buf = m_tileColors;
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
i32 CMinimap::BuildHoneyPalette() {
    u16* buf = m_tileColors;
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
i32 CMinimap::BuildMiniatureMasterzPalette() {
    u16* buf = m_tileColors;
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
RVA(0x000a8900, 0x926)
i32 CMinimap::BuildSpacePalette() {
    u16* buf = m_tileColors;
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
i32 CMinimap::BeginMinimapPan(i32, i32 cursorX, i32 cursorY) {
    i32 cell[2];
    if (!ScreenPointToCell(cursorX, cursorY, cell, MINIMAP_SNAP_MARGIN_PX)) {
        return 0;
    }

    CPlay* play = static_cast<CPlay*>(m_gameMgr->m_curState);
    if (play != NULL) {
        play->ResetGoals(
            cell[0] * TILE_SIZE_PX + TILE_HALF_PX,
            cell[1] * TILE_SIZE_PX + TILE_HALF_PX
        );
    }
    m_panActive = 1;
    return 1;
}

RVA(0x000a9500, 0x16)
i32 CMinimap::EndMinimapPan(i32, i32, i32) {
    if (m_panActive != 0) {
        m_panActive = 0;
    }
    return 1;
}

// @identity-TODO: the minimap-handler ABI and false result are proven; the event identity is not.
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000a9530, 0x5)
i32 CMinimap::IgnoreMinimapEvent(i32, i32, i32) {
    return 0;
}

RVA(0x000a9550, 0x5b)
i32 CMinimap::IssueMinimapCommand(i32, i32 cursorX, i32 cursorY) {
    i32 cell[2];
    if (!ScreenPointToCell(cursorX, cursorY, cell, MINIMAP_SNAP_MARGIN_PX)) {
        return 0;
    }
    g_gameReg->m_triggerMgr->HandleTargetSelection(
        cell[0] * TILE_SIZE_PX + TILE_HALF_PX,
        cell[1] * TILE_SIZE_PX + TILE_HALF_PX,
        0,
        0,
        0,
        TARGET_SELECTION_AUTO,
        1
    );
    return 1;
}

RVA(0x000a95d0, 0x69)
i32 CMinimap::ContinueMinimapPan(i32, i32 cursorX, i32 cursorY) {
    if (m_panActive == 0) {
        return 0;
    }
    i32 cell[2];
    if (!ScreenPointToCell(cursorX, cursorY, cell, MINIMAP_SNAP_MARGIN_PX)) {
        return 0;
    }
    CPlay* play = static_cast<CPlay*>(m_gameMgr->m_curState);
    if (play != NULL) {
        play->ResetGoals(
            cell[0] * TILE_SIZE_PX + TILE_HALF_PX,
            cell[1] * TILE_SIZE_PX + TILE_HALF_PX
        );
    }
    return 1;
}

RVA(0x000a9660, 0xca)
i32 CMinimap::ScreenPointToCell(i32 cursorX, i32 cursorY, i32* outCell, i32 snapMargin) {
    if (cursorX < m_boundsRect.left || cursorX > m_boundsRect.right || cursorY < m_boundsRect.top
        || cursorY > m_boundsRect.bottom) {
        return 0;
    }
    if (snapMargin > 0) {
        if (cursorX < m_drawRect.left && m_drawRect.left - cursorX <= snapMargin) {
            cursorX = m_drawRect.left;
        }
        if (cursorX > m_drawRect.right && cursorX - m_drawRect.right <= snapMargin) {
            cursorX = m_drawRect.right;
        }
        if (cursorY < m_drawRect.top && m_drawRect.top - cursorY <= snapMargin) {
            cursorY = m_drawRect.top;
        }
        if (cursorY > m_drawRect.bottom && cursorY - m_drawRect.bottom <= snapMargin) {
            cursorY = m_drawRect.bottom;
        }
    }
    if (cursorX < m_drawRect.left || cursorX > m_drawRect.right || cursorY < m_drawRect.top
        || cursorY > m_drawRect.bottom) {
        return 0;
    }

    outCell[0] = cursorX - m_drawRect.left;
    outCell[1] = cursorY - m_drawRect.top;
    outCell[0] /= m_cellScale;
    outCell[1] /= m_cellScale;
    return 1;
}
