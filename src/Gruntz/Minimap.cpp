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
#include <Gruntz/MapCellInline.h>
#include <Gruntz/Play.h>
#include <Gruntz/SpriteRefTable.h>
#include <Gruntz/SpriteTeamColorVariant.h>
#include <Gruntz/TriggerMgr.h>
#include <Pix16.h>
#include <RectMacros.h>
#include <Rez/FrameClock.h>
#include <Wap32/TileGeometry.h>

#include <ddraw.h>

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
    SET_RECT_COMPONENTS(m_drawRect, 0, 0, 0, 0);
    SET_RECT_COMPONENTS(m_boundsRect, 0, 0, 0, 0);
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
    m_panActive = false;
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
    size = mapMgr->GetGridSize();
    m_surface = world->m_deviceManager->CreateOffscreenSurface(size.cx, size.cy, BPP_UNSET, 0, -1);
    if (m_surface == NULL) {
        return 0;
    }
    m_surface->Clear(0);
    return 1;
}

inline void CMinimap::GetTileColor(i32 tileId, u16& color) {
    if (static_cast<u32>(tileId) >= MINIMAP_TILE_COLOR_COUNT) {
        color = 0;
    } else {
        color = m_tileColors[tileId];
    }
}

RVA(0x000a3460, 0x2f3)
i32 CMinimap::Refresh(i32 elapsedMs, b32 forceRefresh) {
    if (forceRefresh == false) {

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
    if (m_surface->GetWidth() != static_cast<i32>(m_mapMgr->m_width)
        || m_surface->GetHeight() != static_cast<i32>(m_mapMgr->m_height)) {
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
            u16* pixel = Pix16(pixels + m_surface->PixelOffset(x, y));
            i32 occupantId = m_mapMgr->OccupantAt(x, y);

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
                if (grunt->m_arrived != false) {
                    teamColor = SPRITE_TEAM_COLOR_SECONDARY;
                }

                if (grunt->m_combatTiming.Expired() || grunt->m_playerIndex != g_curPlayer) {
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

                    GetTileColor(m_mapMgr->TileIdAt(x, y), *pixel);
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
                u16 color;
                GetTileColor(m_mapMgr->TileIdAt(x, y), color);
                *pixel = color;
            }
        }
    }
    m_surface->Unlock();
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
    i32 scaleX = width / static_cast<i32>(m_surface->m_apiDesc.dwWidth);
    i32 scaleY = height / static_cast<i32>(m_surface->m_apiDesc.dwHeight);

    i32 scale = scaleY;
    if (scaleX < scaleY) {
        scale = scaleX;
    }

    i32 cellScale = MINIMAP_MAX_CELL_SCALE;
    if (scale <= MINIMAP_MAX_CELL_SCALE) {
        cellScale = scale;
    }
    m_cellScale = cellScale;
    i32 drawLeft = centerX - static_cast<i32>(m_surface->m_apiDesc.dwWidth) * cellScale / 2;
    i32 drawTop = centerY - static_cast<i32>(m_surface->m_apiDesc.dwHeight) * cellScale / 2;
    RECT* dstRect = &m_drawRect;
    SET_RECT_COMPONENTS(
        *dstRect,
        drawLeft,
        drawTop,
        m_surface->m_apiDesc.dwWidth * cellScale + drawLeft,
        m_surface->m_apiDesc.dwHeight * cellScale + drawTop
    );
    if (target->m_surface->BltEx(dstRect, m_surface, NULL, DDBLT_WAIT, NULL) != 0) {
        return 0;
    }

    RECT* vr = &m_world->m_level->m_mainPlane->m_planeViewRect;
    RECT box;
    SET_RECT_COMPONENTS(
        box,
        vr->left >> TILE_SHIFT_PX,
        vr->top >> TILE_SHIFT_PX,
        vr->right >> TILE_SHIFT_PX,
        vr->bottom >> TILE_SHIFT_PX
    );
    if (m_cellScale != 1) {

        box.left *= m_cellScale;
        box.top *= m_cellScale;
        box.right *= m_cellScale;
        box.bottom *= m_cellScale;
        i32 extension = m_cellScale - 1;
        box.right += extension;
        box.bottom += extension;
    }
    OFFSET_RECT_X_EDGES(box, dstRect->left, dstRect->left);
    OFFSET_RECT_Y_EDGES(box, dstRect->top, dstRect->top);
    DrawBorder(&box, target, MINIMAP_BORDER_COLOR_16);
    return 1;
}

// @early-stop
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000a3a20, 0xe2)
void CMinimap::DrawBorderRaw(RECT* rect, char* pixels, i32 color) {
    i32 width = rect->right - rect->left + 1;

    u16* topPixels = Pix16(pixels + m_surface->PixelOffset(rect->left, rect->top));
    for (i32 topX = 0; topX < width; topX++) {
        topPixels[topX] = static_cast<u16>(color);
    }

    u16* bottomPixels = Pix16(pixels + m_surface->PixelOffset(rect->left, rect->bottom));
    for (i32 bottomX = 0; bottomX < width; bottomX++) {
        bottomPixels[bottomX] = static_cast<u16>(color);
    }

    i32 height = rect->bottom - rect->top + 1;
    i32 leftOffset = m_surface->PixelOffset(rect->left, rect->top);
    i32 rightOffset = m_surface->PixelOffset(rect->right, rect->top);
    i32 rowStride = m_surface->m_apiDesc.lPitch;

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

    u16* topPixels = Pix16(
        pixels + rect->top * surface->m_apiDesc.lPitch + rect->left * surface->m_bytesPerPixel
    );
    for (i32 topX = 0; topX < width; topX++) {
        topPixels[topX] = static_cast<u16>(color);
    }

    u16* bottomPixels = Pix16(
        pixels + rect->bottom * surface->m_apiDesc.lPitch + rect->left * surface->m_bytesPerPixel
    );
    for (i32 bottomX = 0; bottomX < width; bottomX++) {
        bottomPixels[bottomX] = static_cast<u16>(color);
    }

    i32 height = rect->bottom - rect->top + 1;
    i32 leftOffset = rect->left * surface->m_bytesPerPixel + rect->top * surface->m_apiDesc.lPitch;
    i32 rightOffset =
        rect->right * surface->m_bytesPerPixel + rect->top * surface->m_apiDesc.lPitch;
    i32 rowStride = surface->m_apiDesc.lPitch;
    for (i32 y = 0; y < height; y++) {
        *Pix16(pixels + leftOffset) = static_cast<u16>(color);
        *Pix16(pixels + rightOffset) = static_cast<u16>(color);
        leftOffset += rowStride;
        rightOffset += rowStride;
    }

    surface->Unlock();
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

RVA(0x000a3dc0, 0x85f)
i32 CMinimap::BuildRockyRoadzPalette() {
    u16* buf = m_tileColors;
    u16 c00 = PackRgb16(0x4f, 0x14, 0x01);
    u16 c01 = PackRgb16(0x63, 0x37, 0x13);
    u16 c02 = PackRgb16(0x5c, 0x0d, 0x06);
    u16 c03 = PackRgb16(0x10, 0x28, 0x71);
    u16 c04 = PackRgb16(0x26, 0x62, 0x71);
    u16 c05 = PackRgb16(0x00, 0x00, 0x00);
    u16 c06 = PackRgb16(0x20, 0x20, 0x20);
    u16 c07 = PackRgb16(0x78, 0x78, 0x5f);
    u16 c08 = PackRgb16(0x64, 0x64, 0x64);
    u16 c09 = PackRgb16(0xff, 0xd9, 0x13);
    u16 c10 = PackRgb16(0xff, 0xd2, 0x47);
    u16 c11 = PackRgb16(0xa1, 0x2b, 0xff);
    u16 c12 = PackRgb16(0x45, 0x7c, 0xff);
    u16 c13 = PackRgb16(0x45, 0xff, 0x45);
    u16 c14 = PackRgb16(0xff, 0x26, 0x26);
    u16 c15 = PackRgb16(0xff, 0x92, 0x2b);
    u16 c16 = PackRgb16(0xd7, 0xd7, 0xd7);
    u16 c17 = PackRgb16(0x37, 0x37, 0x37);
    u16 c18 = PackRgb16(0xb4, 0x61, 0x39);
    u16 c19 = PackRgb16(0x37, 0x30, 0x30);
    u16 c20 = PackRgb16(0xa0, 0xa0, 0x27);
    FillSpan(1, 8, c00);
    FillSpan(17, 36, c00);
    SetTileColor(buf, 90, c00);
    FillSpan(195, 196, c00);
    SetTileColor(buf, 199, c00);
    SetTileColor(buf, 301, c00);
    FillSpan(9, 16, c01);
    SetTileColor(buf, 91, c01);
    FillSpan(197, 198, c01);
    FillSpan(40, 73, c02);
    FillSpan(270, 281, c02);
    FillSpan(104, 115, c06);
    FillSpan(120, 123, c06);
    FillSpan(128, 139, c06);
    FillSpan(144, 155, c04);
    FillSpan(160, 163, c04);
    FillSpan(168, 179, c04);
    FillSpan(157, 158, c03);
    FillSpan(165, 166, c03);
    SetTileColor(buf, 258, c03);
    SetTileColor(buf, 264, c03);
    FillSpan(117, 118, c05);
    FillSpan(0x7d, 0x7e, c05);
    SetTileColor(buf, 260, c05);
    SetTileColor(buf, 266, c05);
    FillSpan(0x11a, 0x11d, c07);
    SetTileColor(buf, 257, c07);
    SetTileColor(buf, 259, c07);
    FillSpan(0x105, 0x107, c07);
    SetTileColor(buf, 265, c07);
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
inline void CMinimap::FillSpan(u32 x1, u32 x2, u16 color) {
    if (x1 > x2) {
        return;
    }
    for (; x1 <= x2; x1++) {
        m_tileColors[x1] = color;
    }
}

RVA(0x000a4890, 0x852)
i32 CMinimap::BuildGruntziclezPalette() {
    u16* buf = m_tileColors;
    u16 c00 = PackRgb16(0xe0, 0xed, 0xfe);
    u16 c01 = PackRgb16(0x89, 0x6e, 0x58);
    u16 c02 = PackRgb16(0xd7, 0xe5, 0xfa);
    u16 c03 = PackRgb16(0x10, 0x28, 0x71);
    u16 c04 = PackRgb16(0x26, 0x62, 0x71);
    u16 c05 = PackRgb16(0x00, 0x00, 0x00);
    u16 c06 = PackRgb16(0x20, 0x20, 0x20);
    u16 c07 = PackRgb16(0x49, 0x65, 0x84);
    u16 c08 = PackRgb16(0xff, 0xd9, 0x13);
    u16 c09 = PackRgb16(0xff, 0xd2, 0x47);
    u16 c10 = PackRgb16(0xa1, 0x2b, 0xff);
    u16 c11 = PackRgb16(0x45, 0x7c, 0xff);
    u16 c12 = PackRgb16(0x45, 0xff, 0x45);
    u16 c13 = PackRgb16(0xff, 0x26, 0x26);
    u16 c14 = PackRgb16(0xff, 0x92, 0x2b);
    u16 c15 = PackRgb16(0xd7, 0xd7, 0xd7);
    u16 c16 = PackRgb16(0x37, 0x37, 0x37);
    u16 c17 = PackRgb16(0xb4, 0x61, 0x39);
    u16 c18 = PackRgb16(0x37, 0x30, 0x30);
    u16 c19 = PackRgb16(0xa0, 0xa0, 0x27);
    FillSpan(1, 8, c00);
    FillSpan(17, 36, c00);
    SetTileColor(buf, 90, c00);
    FillSpan(195, 196, c00);
    SetTileColor(buf, 199, c00);
    SetTileColor(buf, 301, c00);
    FillSpan(9, 16, c01);
    SetTileColor(buf, 91, c01);
    FillSpan(197, 198, c01);
    FillSpan(40, 73, c02);
    FillSpan(270, 281, c02);
    FillSpan(104, 115, c06);
    FillSpan(120, 123, c06);
    FillSpan(128, 139, c06);
    FillSpan(144, 155, c04);
    FillSpan(160, 163, c04);
    FillSpan(168, 179, c04);
    FillSpan(157, 158, c03);
    FillSpan(165, 166, c03);
    SetTileColor(buf, 258, c03);
    SetTileColor(buf, 264, c03);
    FillSpan(117, 118, c05);
    FillSpan(0x7d, 0x7e, c05);
    SetTileColor(buf, 260, c05);
    SetTileColor(buf, 266, c05);
    FillSpan(0x11a, 0x11d, c00);
    SetTileColor(buf, 257, c00);
    SetTileColor(buf, 259, c00);
    FillSpan(0x105, 0x107, c00);
    SetTileColor(buf, 265, c00);
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
RVA(0x000a5310, 0x855)
i32 CMinimap::BuildTropiczPalette() {
    u16* buf = m_tileColors;
    u16 c00 = PackRgb16(0x4e, 0x78, 0x1c);
    u16 c01 = PackRgb16(0x23, 0x23, 0x23);
    u16 c02 = PackRgb16(0x24, 0x37, 0x0f);
    u16 c03 = PackRgb16(0x10, 0x28, 0x71);
    u16 c04 = PackRgb16(0x26, 0x62, 0x71);
    u16 c05 = PackRgb16(0xb4, 0x3d, 0x0b);
    u16 c06 = PackRgb16(0x64, 0x0c, 0x03);
    u16 c07 = PackRgb16(0xb0, 0x85, 0x1f);
    u16 c08 = PackRgb16(0x59, 0x17, 0x0f);
    u16 c09 = PackRgb16(0xff, 0xd9, 0x13);
    u16 c10 = PackRgb16(0xff, 0xd2, 0x47);
    u16 c11 = PackRgb16(0xa1, 0x2b, 0xff);
    u16 c12 = PackRgb16(0x45, 0x7c, 0xff);
    u16 c13 = PackRgb16(0x45, 0xff, 0x45);
    u16 c14 = PackRgb16(0xff, 0x26, 0x26);
    u16 c15 = PackRgb16(0xff, 0x92, 0x2b);
    u16 c16 = PackRgb16(0xd7, 0xd7, 0xd7);
    u16 c17 = PackRgb16(0x37, 0x37, 0x37);
    u16 c18 = PackRgb16(0xb4, 0x61, 0x39);
    u16 c19 = PackRgb16(0x37, 0x30, 0x30);
    u16 c20 = PackRgb16(0xa0, 0xa0, 0x27);
    FillSpan(1, 8, c00);
    FillSpan(17, 36, c00);
    SetTileColor(buf, 90, c00);
    FillSpan(195, 196, c00);
    SetTileColor(buf, 199, c00);
    SetTileColor(buf, 301, c00);
    FillSpan(9, 16, c01);
    SetTileColor(buf, 91, c01);
    FillSpan(197, 198, c01);
    FillSpan(40, 73, c02);
    FillSpan(270, 281, c02);
    FillSpan(104, 115, c06);
    FillSpan(120, 123, c06);
    FillSpan(128, 139, c06);
    FillSpan(144, 155, c04);
    FillSpan(160, 163, c04);
    FillSpan(168, 179, c04);
    FillSpan(157, 158, c03);
    FillSpan(165, 166, c03);
    SetTileColor(buf, 258, c03);
    SetTileColor(buf, 264, c03);
    FillSpan(117, 118, c05);
    FillSpan(0x7d, 0x7e, c05);
    SetTileColor(buf, 260, c05);
    SetTileColor(buf, 266, c05);
    FillSpan(0x11a, 0x11d, c07);
    SetTileColor(buf, 257, c07);
    SetTileColor(buf, 259, c07);
    FillSpan(0x105, 0x107, c07);
    SetTileColor(buf, 265, c07);
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
RVA(0x000a5d90, 0x825)
i32 CMinimap::BuildHighOnSweetzPalette() {
    u16* buf = m_tileColors;
    u16 c00 = PackRgb16(0x8b, 0x9f, 0xfd);
    u16 c01 = PackRgb16(0x00, 0xc1, 0xa7);
    u16 c02 = PackRgb16(0x47, 0x65, 0xf1);
    u16 c03 = PackRgb16(0x01, 0x00, 0x5e);
    u16 c04 = PackRgb16(0x0d, 0x20, 0xbe);
    u16 c05 = PackRgb16(0x00, 0x00, 0x00);
    u16 c06 = PackRgb16(0x45, 0x2e, 0x0d);
    u16 c07 = PackRgb16(0xff, 0xc5, 0xe0);
    u16 c08 = PackRgb16(0xff, 0xd9, 0x13);
    u16 c09 = PackRgb16(0xff, 0xd2, 0x47);
    u16 c10 = PackRgb16(0xa1, 0x2b, 0xff);
    u16 c11 = PackRgb16(0x45, 0x7c, 0xff);
    u16 c12 = PackRgb16(0x45, 0xff, 0x45);
    u16 c13 = PackRgb16(0xff, 0x26, 0x26);
    u16 c14 = PackRgb16(0xff, 0x92, 0x2b);
    u16 c15 = PackRgb16(0xd7, 0xd7, 0xd7);
    u16 c16 = PackRgb16(0x37, 0x37, 0x37);
    u16 c17 = PackRgb16(0xb4, 0x61, 0x39);
    u16 c18 = PackRgb16(0x37, 0x30, 0x30);
    u16 c19 = PackRgb16(0xa0, 0xa0, 0x27);
    FillSpan(1, 8, c00);
    FillSpan(17, 36, c00);
    SetTileColor(buf, 90, c00);
    FillSpan(195, 196, c00);
    SetTileColor(buf, 199, c00);
    SetTileColor(buf, 301, c00);
    FillSpan(9, 16, c01);
    SetTileColor(buf, 91, c01);
    FillSpan(197, 198, c01);
    FillSpan(40, 73, c02);
    FillSpan(270, 281, c02);
    FillSpan(104, 115, c05);
    FillSpan(120, 123, c05);
    FillSpan(128, 139, c05);
    FillSpan(144, 155, c04);
    FillSpan(160, 163, c04);
    FillSpan(168, 179, c04);
    FillSpan(157, 158, c03);
    FillSpan(165, 166, c03);
    SetTileColor(buf, 258, c03);
    SetTileColor(buf, 264, c03);
    FillSpan(117, 118, c05);
    FillSpan(0x7d, 0x7e, c05);
    SetTileColor(buf, 260, c05);
    SetTileColor(buf, 266, c05);
    FillSpan(0x11a, 0x11d, c06);
    SetTileColor(buf, 257, c06);
    SetTileColor(buf, 259, c06);
    FillSpan(0x105, 0x107, c06);
    SetTileColor(buf, 265, c06);
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
    u16 c00 = PackRgb16(0x3c, 0x0e, 0x15);
    u16 c01 = PackRgb16(0x68, 0x08, 0x07);
    u16 c02 = PackRgb16(0xf2, 0xfe, 0x9b);
    u16 c03 = PackRgb16(0x23, 0x7d, 0xb5);
    u16 c04 = PackRgb16(0x1b, 0x3c, 0x64);
    u16 c05 = PackRgb16(0x00, 0x00, 0x00);
    u16 c06 = PackRgb16(0x6e, 0x19, 0x46);
    u16 c07 = PackRgb16(0xfc, 0xfc, 0xfc);
    u16 c08 = PackRgb16(0xff, 0xd9, 0x13);
    u16 c09 = PackRgb16(0xff, 0xd2, 0x47);
    u16 c10 = PackRgb16(0xa1, 0x2b, 0xff);
    u16 c11 = PackRgb16(0x45, 0x7c, 0xff);
    u16 c12 = PackRgb16(0x45, 0xff, 0x45);
    u16 c13 = PackRgb16(0xff, 0x26, 0x26);
    u16 c14 = PackRgb16(0xff, 0x92, 0x2b);
    u16 c15 = PackRgb16(0xd7, 0xd7, 0xd7);
    u16 c16 = PackRgb16(0x37, 0x37, 0x37);
    u16 c17 = PackRgb16(0xb4, 0x61, 0x39);
    u16 c18 = PackRgb16(0x37, 0x30, 0x30);
    u16 c19 = PackRgb16(0xa0, 0xa0, 0x27);
    FillSpan(1, 8, c00);
    FillSpan(17, 36, c00);
    SetTileColor(buf, 90, c00);
    FillSpan(195, 196, c00);
    SetTileColor(buf, 199, c00);
    SetTileColor(buf, 301, c00);
    FillSpan(9, 16, c01);
    SetTileColor(buf, 91, c01);
    FillSpan(197, 198, c01);
    FillSpan(39, 74, c02);
    FillSpan(270, 281, c02);
    FillSpan(102, 113, c05);
    FillSpan(116, 121, c05);
    FillSpan(124, 138, c05);
    FillSpan(144, 155, c04);
    FillSpan(159, 163, c04);
    FillSpan(168, 179, c04);
    FillSpan(157, 158, c03);
    FillSpan(165, 166, c03);
    SetTileColor(buf, 258, c03);
    SetTileColor(buf, 264, c03);
    FillSpan(114, 115, c05);
    FillSpan(0x7a, 0x7b, c05);
    SetTileColor(buf, 260, c05);
    SetTileColor(buf, 266, c05);
    FillSpan(0x11a, 0x11d, c06);
    SetTileColor(buf, 257, c06);
    SetTileColor(buf, 259, c06);
    FillSpan(0x105, 0x107, c06);
    SetTileColor(buf, 265, c06);
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
RVA(0x000a7260, 0x8c0)
i32 CMinimap::BuildHoneyPalette() {
    u16* buf = m_tileColors;
    u16 c00 = PackRgb16(0x85, 0x73, 0x6f);
    u16 c01 = PackRgb16(0x28, 0x25, 0xc8);
    u16 c02 = PackRgb16(0xaf, 0xad, 0xc4);
    u16 c03 = PackRgb16(0x32, 0x99, 0xae);
    u16 c04 = PackRgb16(0x10, 0x77, 0x8c);
    u16 c05 = PackRgb16(0x35, 0x38, 0x42);
    u16 c06 = PackRgb16(0x4d, 0x50, 0x5a);
    u16 c07 = PackRgb16(0xa7, 0x83, 0x48);
    u16 c08 = PackRgb16(0xfb, 0xfb, 0xfb);
    u16 c09 = PackRgb16(0xff, 0xd9, 0x13);
    u16 c10 = PackRgb16(0xff, 0xd2, 0x47);
    u16 c11 = PackRgb16(0xa1, 0x2b, 0xff);
    u16 c12 = PackRgb16(0x45, 0x7c, 0xff);
    u16 c13 = PackRgb16(0x45, 0xff, 0x45);
    u16 c14 = PackRgb16(0xff, 0x26, 0x26);
    u16 c15 = PackRgb16(0xff, 0x92, 0x2b);
    u16 c16 = PackRgb16(0xd7, 0xd7, 0xd7);
    u16 c17 = PackRgb16(0x37, 0x37, 0x37);
    u16 c18 = PackRgb16(0xb4, 0x61, 0x39);
    u16 c19 = PackRgb16(0x37, 0x30, 0x30);
    u16 c20 = PackRgb16(0xa0, 0xa0, 0x27);
    FillSpan(1, 8, c00);
    FillSpan(17, 36, c00);
    SetTileColor(buf, 90, c00);
    FillSpan(195, 196, c00);
    SetTileColor(buf, 199, c00);
    SetTileColor(buf, 301, c00);
    FillSpan(9, 16, c01);
    SetTileColor(buf, 91, c01);
    FillSpan(197, 198, c01);
    FillSpan(39, 74, c02);
    FillSpan(270, 281, c02);
    FillSpan(102, 113, c06);
    FillSpan(116, 121, c06);
    FillSpan(124, 138, c06);
    FillSpan(144, 155, c04);
    FillSpan(159, 163, c04);
    FillSpan(168, 179, c04);
    FillSpan(157, 158, c03);
    FillSpan(165, 166, c03);
    SetTileColor(buf, 258, c03);
    SetTileColor(buf, 264, c03);
    FillSpan(114, 115, c05);
    FillSpan(122, 123, c05);
    SetTileColor(buf, 260, c05);
    SetTileColor(buf, 266, c05);
    FillSpan(0x11a, 0x11d, c07);
    SetTileColor(buf, 257, c07);
    SetTileColor(buf, 259, c07);
    FillSpan(0x105, 0x107, c07);
    SetTileColor(buf, 265, c07);
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
    SetTileColor(buf, 259, c01);
    SetTileColor(buf, 265, c00);
    FillSpan(0x5c, 0x5f, c00);
    return 1;
}
RVA(0x000a7d50, 0x94f)
i32 CMinimap::BuildMiniatureMasterzPalette() {
    u16* buf = m_tileColors;
    u16 c00 = PackRgb16(0x40, 0xb5, 0x13);
    u16 c01 = PackRgb16(0x00, 0x7a, 0x2f);
    u16 c02 = PackRgb16(0x68, 0x71, 0x7c);
    u16 c03 = PackRgb16(0x6a, 0xb9, 0xff);
    u16 c04 = PackRgb16(0x43, 0x85, 0xff);
    u16 c05 = PackRgb16(0xc3, 0xc0, 0x73);
    u16 c06 = PackRgb16(0x86, 0x8b, 0x7f);
    u16 c07 = PackRgb16(0x78, 0x78, 0x5f);
    u16 c08 = PackRgb16(0x81, 0x55, 0xf6);
    u16 c09 = PackRgb16(0xff, 0xd9, 0x13);
    u16 c10 = PackRgb16(0xff, 0xd2, 0x47);
    u16 c11 = PackRgb16(0xa1, 0x2b, 0xff);
    u16 c12 = PackRgb16(0x45, 0x7c, 0xff);
    u16 c13 = PackRgb16(0x45, 0xff, 0x45);
    u16 c14 = PackRgb16(0xff, 0x26, 0x26);
    u16 c15 = PackRgb16(0xff, 0x92, 0x2b);
    u16 c16 = PackRgb16(0xd7, 0xd7, 0xd7);
    u16 c17 = PackRgb16(0x37, 0x37, 0x37);
    u16 c18 = PackRgb16(0xb4, 0x61, 0x39);
    u16 c19 = PackRgb16(0x37, 0x30, 0x30);
    u16 c20 = PackRgb16(0xa0, 0xa0, 0x27);
    u16 c21 = PackRgb16(0xe2, 0x70, 0x00);
    u16 c22 = PackRgb16(0xa1, 0xf5, 0xff);
    u16 c23 = PackRgb16(0xfd, 0xe5, 0x00);
    FillSpan(1, 8, c00);
    FillSpan(17, 36, c00);
    SetTileColor(buf, 90, c00);
    FillSpan(195, 196, c00);
    SetTileColor(buf, 199, c00);
    SetTileColor(buf, 301, c00);
    FillSpan(9, 16, c01);
    SetTileColor(buf, 91, c01);
    FillSpan(197, 198, c01);
    FillSpan(39, 74, c02);
    FillSpan(270, 281, c02);
    FillSpan(102, 113, c06);
    FillSpan(116, 121, c06);
    FillSpan(124, 138, c06);
    FillSpan(144, 155, c04);
    FillSpan(159, 163, c04);
    FillSpan(168, 179, c04);
    FillSpan(157, 158, c03);
    FillSpan(165, 166, c03);
    SetTileColor(buf, 258, c03);
    SetTileColor(buf, 264, c03);
    FillSpan(114, 115, c05);
    FillSpan(122, 123, c05);
    SetTileColor(buf, 260, c05);
    SetTileColor(buf, 266, c05);
    FillSpan(0x11a, 0x11d, c07);
    SetTileColor(buf, 257, c07);
    SetTileColor(buf, 259, c07);
    FillSpan(0x105, 0x107, c07);
    SetTileColor(buf, 265, c07);
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
    SetTileColor(buf, 257, c21);
    SetTileColor(buf, 259, c21);
    FillSpan(0x105, 0x106, c22);
    SetTileColor(buf, 263, c23);
    SetTileColor(buf, 265, c23);
    FillSpan(0x5c, 0x5f, c00);
    return 1;
}
RVA(0x000a8900, 0x926)
i32 CMinimap::BuildSpacePalette() {
    u16* buf = m_tileColors;
    u16 c00 = PackRgb16(0x5e, 0x5e, 0x5e);
    u16 c01 = PackRgb16(0x28, 0x28, 0x28);
    u16 c02 = PackRgb16(0x96, 0x96, 0x96);
    u16 c03 = PackRgb16(0x30, 0x64, 0x6f);
    u16 c04 = PackRgb16(0x33, 0x50, 0x57);
    u16 c05 = PackRgb16(0x00, 0x00, 0x00);
    u16 c06 = PackRgb16(0x78, 0x78, 0x5f);
    u16 c07 = PackRgb16(0x94, 0xa7, 0xbd);
    u16 c08 = PackRgb16(0xff, 0xd9, 0x13);
    u16 c09 = PackRgb16(0xff, 0xd2, 0x47);
    u16 c10 = PackRgb16(0xa1, 0x2b, 0xff);
    u16 c11 = PackRgb16(0x45, 0x7c, 0xff);
    u16 c12 = PackRgb16(0x45, 0xff, 0x45);
    u16 c13 = PackRgb16(0xff, 0x26, 0x26);
    u16 c14 = PackRgb16(0xff, 0x92, 0x2b);
    u16 c15 = PackRgb16(0xd7, 0xd7, 0xd7);
    u16 c16 = PackRgb16(0x37, 0x37, 0x37);
    u16 c17 = PackRgb16(0xb4, 0x61, 0x39);
    u16 c18 = PackRgb16(0x37, 0x30, 0x30);
    u16 c19 = PackRgb16(0xa0, 0xa0, 0x27);
    u16 c20 = PackRgb16(0x12, 0xd2, 0x18);
    u16 c21 = PackRgb16(0x00, 0x72, 0xe4);
    u16 c22 = PackRgb16(0xe4, 0x00, 0x26);
    FillSpan(1, 8, c00);
    FillSpan(17, 36, c00);
    SetTileColor(buf, 90, c00);
    FillSpan(195, 196, c00);
    SetTileColor(buf, 199, c00);
    SetTileColor(buf, 301, c00);
    FillSpan(9, 16, c01);
    SetTileColor(buf, 91, c01);
    FillSpan(197, 198, c01);
    FillSpan(39, 74, c02);
    FillSpan(270, 281, c02);
    FillSpan(102, 113, c05);
    FillSpan(116, 121, c05);
    FillSpan(124, 138, c05);
    FillSpan(144, 155, c04);
    FillSpan(159, 163, c04);
    FillSpan(168, 179, c04);
    FillSpan(157, 158, c03);
    FillSpan(165, 166, c03);
    SetTileColor(buf, 258, c03);
    SetTileColor(buf, 264, c03);
    FillSpan(114, 115, c05);
    FillSpan(122, 123, c05);
    SetTileColor(buf, 260, c05);
    SetTileColor(buf, 266, c05);
    FillSpan(0x11a, 0x11d, c06);
    SetTileColor(buf, 257, c06);
    SetTileColor(buf, 259, c06);
    FillSpan(0x105, 0x107, c06);
    SetTileColor(buf, 265, c06);
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
    SetTileColor(buf, 257, c20);
    SetTileColor(buf, 259, c20);
    FillSpan(0x105, 0x106, c21);
    SetTileColor(buf, 263, c22);
    SetTileColor(buf, 265, c22);
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
    m_panActive = true;
    return 1;
}

RVA(0x000a9500, 0x16)
i32 CMinimap::EndMinimapPan(i32, i32, i32) {
    if (m_panActive != false) {
        m_panActive = false;
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
    if (m_panActive == false) {
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
