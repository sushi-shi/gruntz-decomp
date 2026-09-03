#include <rva.h>

#include <Mfc.h>
#include <MfcWin.h>

#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawPaletteRegistry.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorkerHost.h>
#include <DDrawMgr/DDrawWorkerHostBuildInline.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DDSurface.h>
#include <DDrawMgr/DirectDrawMgr.h>
#include <DDrawMgr/LogicRecordRegistry.h>
#include <DDrawMgr/LogicRecordRegistryFindInline.h>
#include <DDrawMgr/PixelShift.h>
#include <Enums.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/WwdGameObject.h>
#include <Image/CImage.h>
#include <Image/ImageSet.h>
#include <Io/FileMem.h>
#include <MakeRect.h>
#include <Wap32/CoordUnset.h>
#include <Wap32/WapObj.h>
#include <Wwd/WwdSpatialMgr.h>

#include <new>
#include <stdio.h>
#include <string.h>

static inline CDDrawWorker* LookupWorker(CMapStringToOb& map, LPCTSTR name) {
    CObject* found = NULL;
    map.Lookup(name, found);
    return static_cast<CDDrawWorker*>(found);
}

static inline CDDrawWorker* LookupWorker(CDDrawSurfaceMgr* host, LPCTSTR name) {
    CObject* found = NULL;
    host->m_imageRegistry->m_workersByName.Lookup(name, found);
    return static_cast<CDDrawWorker*>(found);
}

RVA(0x001615a0, 0x9a)
CDDrawWorkerHost::CDDrawWorkerHost(CDDrawSurfaceMgr* owner, i32 id, i32 flags)
    : CWapObj(owner, id, flags, CWapObj::NO_SEED) {

    m_tileHandles = NULL;
    m_tileRowOffsets = NULL;
    m_spatialMgr = NULL;
    m_scrollScale.Init(1.0f, 1.0f);
    m_viewportRect.left = -1;
    memset(&m_fillFx, 0, sizeof(m_fillFx));
    m_fillFx.dwSize = sizeof(DDBLTFX);
}

// @early-stop
RVA(0x00161640, 0x3a2)
i32 CDDrawWorkerHost::Read(
    const WwdPlaneHeader* pd,
    const char* blockBase,
    LevelCoordRect* bounds
) {
    if (pd->headerSize != WWD_PLANE_HEADER_SIZE) {
        return 0;
    }

    char nameBuf[0x80];
    i32 pos = 0;
    const char* names = blockBase + pd->imageSetsOffset;
    for (u32 n = 0; n < pd->imageSetsCount; n++) {
        i32 len = 0;
        while ((names[pos] < '0' || names[pos] > 0x80) && names[pos] != 0) {
            pos++;
        }
        while (names[pos] >= '0' && names[pos] < 0x80 && names[pos] != 0) {
            nameBuf[len] = names[pos];
            len++;
            pos++;
        }
        nameBuf[len] = 0;
        if (len > 0) {

            m_imageSets.SetAtGrow(static_cast<char>(n), LookupWorker(OwnerMgr(), nameBuf));
        }
    }

    m_flags = IDX(pd->flags);
    m_movementPercent.Set(pd->movementXPercent, pd->movementYPercent);
    m_scrollCenter.Init(0.0f, 0.0f);
    m_zCoord = -999999;
    m_tileGridSize = CSize(pd->tilesWide, pd->tilesHigh);
    m_tilePixelSize = CSize(pd->tilePixelWidth, pd->tilePixelHeight);
    m_zCoord = pd->zCoord;
    m_viewportRect = *bounds;
    SetRect(&m_tileRect, 0, 0, m_tilePixelSize.cx, m_tilePixelSize.cy);
    m_planePixelSize =
        CSize(m_tilePixelSize.cx * m_tileGridSize.cx, m_tilePixelSize.cy * m_tileGridSize.cy);

    if (m_flags & IDX(WWD_PLANE_FLAG_AUTO_TILE_SIZE)) {

        CDDrawWorker* set = (m_imageSets.GetSize() > 0) ? ImageSetAt(0) : NULL;
        for (i32 f = 0; f < set->m_items.GetSize(); f++) {
            if (set->GetAt(f) != NULL) {
                CImage* first = set->GetAt(f);
                SetTileSizeFromImage(first);
                break;
            }
        }
    } else {
        SetTileSize(pd->tilePixelWidth, pd->tilePixelHeight);
    }

    strcpy(m_planeName, pd->name);
    m_fillFx.dwFillColor = pd->fillColor;
    m_flags = IDX(pd->flags);

    SetViewportRect(bounds);

    m_scrollScale = FloatVector2(m_movementPercent) * 0.01f;

    m_tileHandles = new i32[m_tileGridSize.cy * m_tileGridSize.cx];
    // Byte-forced view of packed WWD storage.

    const i32* cell = reinterpret_cast<const i32*>(blockBase + pd->tilesOffset);
    for (u32 t = 0; t < static_cast<u32>(m_tileGridSize.cy * m_tileGridSize.cx); t++) {
        m_tileHandles[t] = *cell;
        cell++;
    }

    m_tileRowOffsets = new i32[m_tileGridSize.cy];
    for (i32 c = 0; c < m_tileGridSize.cy; c++) {
        m_tileRowOffsets[c] = c * m_tileGridSize.cx;
    }

    FloatVector2 scrollCenter(Coord(pd->scrollX, pd->scrollY));
    if ((m_flags & IDX(WWD_PLANE_FLAG_MAIN)) == 0) {
        scrollCenter.x *= m_scrollScale.x;
        scrollCenter.y *= m_scrollScale.y;
    }
    m_scrollCenter = scrollCenter;
    UpdatePlaneViewRect();

    if (pd->objectsOffset != 0) {
        if (RebuildPlanes(blockBase + pd->objectsOffset, pd->objectsCount) == 0) {
            return 0;
        }
    }
    return 1;
}

// @early-stop
RVA(0x001619f0, 0x1f7)
i32 CDDrawWorkerHost::InitGeometry(
    i32 tileColumns,
    i32 tileRows,
    i32 tileWidthPx,
    i32 tileHeightPx,
    i32 movementXPercent,
    i32 movementYPercent,
    LevelCoordRect* viewportRect,
    char* planeName
) {
    m_tileGridSize = CSize(tileColumns, tileRows);
    m_tilePixelSize = CSize(tileWidthPx, tileHeightPx);
    m_viewportRect = *viewportRect;
    m_movementPercent.Set(movementXPercent, movementYPercent);
    m_planePixelSize = CSize(tileWidthPx * tileColumns, tileHeightPx * tileRows);
    SetRect(&m_tileRect, 0, 0, tileWidthPx, tileHeightPx);
    m_viewportSize = CRect(m_viewportRect).Size() + CSize(1, 1);
    m_viewHalfSize = CSize(m_viewportSize.cx / 2, m_viewportSize.cy / 2);
    m_tileShift.Set(TileShiftForSize(tileWidthPx), TileShiftForSize(tileHeightPx));
    if (planeName != NULL) {
        strcpy(m_planeName, planeName);
    }
    SetViewportRect(viewportRect);
    m_scrollScale = FloatVector2(m_movementPercent) * 0.01f;
    m_tileHandles = new i32[m_tileGridSize.cx * m_tileGridSize.cy];
    m_tileRowOffsets = new i32[m_tileGridSize.cy];
    for (i32 i = 0; i < m_tileGridSize.cy; i++) {
        m_tileRowOffsets[i] = i * m_tileGridSize.cx;
    }
    SetScrollPosition(0, 0);
    return 1;
}

RVA(0x00161bf0, 0x5e)
void CDDrawWorkerHost::Unload() {
    if (m_spatialMgr != NULL) {
        m_spatialMgr->PruneCount();
    }
    CWwdSpatialMgr* g = m_spatialMgr;
    delete g;
    if (m_tileHandles != NULL) {
        delete[] m_tileHandles;
        m_tileHandles = NULL;
    }
    if (m_tileRowOffsets != NULL) {
        delete[] m_tileRowOffsets;
        m_tileRowOffsets = NULL;
    }
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00161c50, 0x3f)
void CDDrawWorkerHost::SetImageSetByName(char index, const char* key) {
    m_imageSets.SetAtGrow(index, LookupWorker(OwnerMgr()->m_imageRegistry->m_workersByName, key));
}

// @early-stop
RVA(0x00161c90, 0x1e4)
void CDDrawWorkerHost::UpdatePlaneViewRect() {
    CDDrawWorkerHost* p = this;
    WwdPlaneFlags flags = static_cast<WwdPlaneFlags>(p->m_flags);
    i32 wrapX, wrapY;
    wrapX = HAS(flags, WWD_PLANE_FLAG_WRAP_X);

    if (wrapX) {
        if (p->m_scrollCenter.x < 0.0f) {
            do {
                p->m_scrollCenter.x += static_cast<float>(p->m_planePixelSize.cx);
            } while (p->m_scrollCenter.x < 0.0f);
        }
        if (p->m_scrollCenter.x >= static_cast<float>(p->m_planePixelSize.cx)) {
            float t = p->m_scrollCenter.x;
            do {
                t -= static_cast<float>(p->m_planePixelSize.cx);
            } while (t >= static_cast<float>(p->m_planePixelSize.cx));
            p->m_scrollCenter.x = t;
        }
    } else {
        if (p->m_scrollCenter.x < 0.0f) {
            p->m_scrollCenter.x = 0;
        } else if (static_cast<float>(p->m_planePixelSize.cx) <= p->m_scrollCenter.x) {
            p->m_scrollCenter.x = static_cast<float>((p->m_planePixelSize.cx - 1));
        }
    }

    wrapY = HAS(flags, WWD_PLANE_FLAG_WRAP_Y);
    if (wrapY) {
        if (p->m_scrollCenter.y < 0.0f) {
            do {
                p->m_scrollCenter.y += static_cast<float>(p->m_planePixelSize.cy);
            } while (p->m_scrollCenter.y < 0.0f);
        }
        if (p->m_scrollCenter.y >= static_cast<float>(p->m_planePixelSize.cy)) {
            float t = p->m_scrollCenter.y;
            do {
                t -= static_cast<float>(p->m_planePixelSize.cy);
            } while (t >= static_cast<float>(p->m_planePixelSize.cy));
            p->m_scrollCenter.y = t;
        }
    } else {
        if (p->m_scrollCenter.y < 0.0f) {
            p->m_scrollCenter.y = 0;
        } else if (static_cast<float>(p->m_planePixelSize.cy) <= p->m_scrollCenter.y) {
            p->m_scrollCenter.y = static_cast<float>((p->m_planePixelSize.cy - 1));
        }
    }

    p->m_scrollPixel = p->m_scrollCenter.ToCoord();
    i32 iy = p->m_scrollPixel.m_y;

    p->m_planeViewRect.left = p->m_scrollPixel.m_x - p->m_viewHalfSize.cx;
    if (p->m_planeViewRect.left < 0) {
        if (wrapX) {
            p->m_planeViewRect.left = p->m_planePixelSize.cx + p->m_planeViewRect.left;
        } else {
            p->m_planeViewRect.left = 0;
        }
    }

    i32 oy = iy - p->m_viewHalfSize.cy;
    p->m_planeViewRect.top = oy;
    if (oy < 0) {
        if (wrapY) {
            p->m_planeViewRect.top = p->m_planePixelSize.cy + oy;
        } else {
            p->m_planeViewRect.top = 0;
        }
    }

    CPoint farCorner = CPoint(p->m_planeViewRect.left, p->m_planeViewRect.top)
                       + CSize(p->m_viewportSize.cx - 1, p->m_viewportSize.cy - 1);
    p->m_planeViewRect.right = farCorner.x;
    p->m_planeViewRect.bottom = farCorner.y;
    if (farCorner.x >= p->m_planePixelSize.cx && wrapX == 0) {
        i32 over = farCorner.x - p->m_planePixelSize.cx + 1;
        p->m_planeViewRect.right = farCorner.x - over;
        p->m_planeViewRect.left = p->m_planeViewRect.left - over;
    }
    if (farCorner.y >= p->m_planePixelSize.cy && wrapY == 0) {
        i32 over = farCorner.y - p->m_planePixelSize.cy + 1;
        p->m_planeViewRect.bottom = farCorner.y - over;
        p->m_planeViewRect.top = p->m_planeViewRect.top - over;
    }
}

RVA(0x00161f00, 0x75)
void CDDrawWorkerHost::SetTileSize(i32 tileWidthPx, i32 tileHeightPx) {
    m_tilePixelSize = CSize(tileWidthPx, tileHeightPx);
    SetRect(&m_tileRect, 0, 0, tileWidthPx, tileHeightPx);
    m_planePixelSize = CSize(m_tileGridSize.cx * tileWidthPx, m_tileGridSize.cy * tileHeightPx);
    m_tileShift.Set(TileShiftForSize(tileWidthPx), TileShiftForSize(tileHeightPx));
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00161fa0, 0x6c)
void CDDrawWorkerHost::SetTileSizeFromImageSet(CDDrawWorker* set) {
    for (i32 i = 0; i < set->m_items.GetSize(); i++) {
        if (set->GetAt(i) != NULL) {
            CImage* f = set->GetAt(i);
            SetTileSizeFromImage(f);
            break;
        }
    }
}

static inline void
DrawCell(CDDrawWorkerHost* host, CDDSurface* surface, i32 handle, i32 x, i32 y, RECT* source) {
    u32 tileHandle = static_cast<u32>(handle);
    if (tileHandle == UNINIT_FILL) {
        CRect destination =
            MakeRect(x, y, x + (source->right - source->left), y + (source->bottom - source->top));
        surface->BltEx(&destination, NULL, NULL, DDBLT_WAIT | DDBLT_COLORFILL, &host->m_fillFx);
    } else if (tileHandle != static_cast<u32>(TILE_CLEAR)) {
        CDDrawWorker* frames = host->ImageSetAt(tileHandle >> 16);
        i32 index = static_cast<i32>(tileHandle & WWD_TILE_IMAGE_SET_INDEX_MASK);
        CImage* image = frames->GetAt(index);
        surface->BltFast(x, y, image->m_surface, source, image->m_bltFastFlags);
    }
}

// @early-stop
RVA(0x00162010, 0x8bd)
void CDDrawWorkerHost::Draw(CDDrawSurfacePair* ctx) {
    if ((m_flags & IDX(WWD_PLANE_FLAG_NO_DRAW)) != 0) {
        return;
    }
    CRect tileBounds(
        m_planeViewRect.left >> m_tileShift.m_x,
        m_planeViewRect.top >> m_tileShift.m_y,
        m_planeViewRect.right >> m_tileShift.m_x,
        m_planeViewRect.bottom >> m_tileShift.m_y
    );
    CSize nearTileSize(
        ((tileBounds.left + 1) << m_tileShift.m_x) - m_planeViewRect.left,
        ((tileBounds.top + 1) << m_tileShift.m_y) - m_planeViewRect.top
    );
    CSize farTileSize(
        m_planeViewRect.right - (tileBounds.right << m_tileShift.m_x) + 1,
        m_planeViewRect.bottom - (tileBounds.bottom << m_tileShift.m_y) + 1
    );
    CRect topSrc =
        MakeRect(0, m_tilePixelSize.cy - nearTileSize.cy, m_tilePixelSize.cx, m_tilePixelSize.cy);
    CRect leftSrc =
        MakeRect(m_tilePixelSize.cx - nearTileSize.cx, 0, m_tilePixelSize.cx, m_tilePixelSize.cy);
    CRect rightSrc = MakeRect(0, 0, farTileSize.cx, m_tilePixelSize.cy);
    CRect corner;
    CDDSurface* surf = ctx->m_surface;
    CSize interiorTileCount(tileBounds.Width() - 1, tileBounds.Height() - 1);

    CPoint position(m_viewportRect.left, m_viewportRect.top);
    i32 col, row, i;
    i32 rowBase;

    rowBase = m_tileRowOffsets[tileBounds.top];
    corner = MakeRect(
        m_tilePixelSize.cx - nearTileSize.cx,
        m_tilePixelSize.cy - nearTileSize.cy,
        m_tilePixelSize.cx,
        m_tilePixelSize.cy
    );
    DrawCell(this, surf, m_tileHandles[rowBase + tileBounds.left], position.x, position.y, &corner);
    position.x += nearTileSize.cx;
    col = tileBounds.left + 1;
    if (col >= m_tileGridSize.cx) {
        col = 0;
    }
    for (i = interiorTileCount.cx; i > 0; i--) {
        DrawCell(this, surf, m_tileHandles[rowBase + col], position.x, position.y, &topSrc);
        position.x += m_tilePixelSize.cx;
        if (++col >= m_tileGridSize.cx) {
            col = 0;
        }
    }
    corner = MakeRect(0, m_tilePixelSize.cy - nearTileSize.cy, farTileSize.cx, m_tilePixelSize.cy);
    DrawCell(this, surf, m_tileHandles[rowBase + col], position.x, position.y, &corner);

    position.y += nearTileSize.cy;
    row = tileBounds.top + 1;
    if (row >= m_tileGridSize.cy) {
        row = 0;
    }
    for (i32 r = interiorTileCount.cy; r > 0; r--) {
        rowBase = m_tileRowOffsets[row];
        position.x = m_viewportRect.left;
        DrawCell(
            this,
            surf,
            m_tileHandles[rowBase + tileBounds.left],
            position.x,
            position.y,
            &leftSrc
        );
        position.x += nearTileSize.cx;
        col = tileBounds.left + 1;
        if (col >= m_tileGridSize.cx) {
            col = 0;
        }
        for (i = interiorTileCount.cx; i > 0; i--) {
            DrawCell(this, surf, m_tileHandles[rowBase + col], position.x, position.y, &m_tileRect);
            position.x += m_tilePixelSize.cx;
            if (++col >= m_tileGridSize.cx) {
                col = 0;
            }
        }
        DrawCell(this, surf, m_tileHandles[rowBase + col], position.x, position.y, &rightSrc);
        position.y += m_tilePixelSize.cy;
        if (++row >= m_tileGridSize.cy) {
            row = 0;
        }
    }

    CRect botSrc = MakeRect(0, 0, m_tilePixelSize.cx, farTileSize.cy);
    position.x = m_viewportRect.left;
    rowBase = m_tileRowOffsets[row];
    corner = MakeRect(m_tilePixelSize.cx - nearTileSize.cx, 0, m_tilePixelSize.cx, farTileSize.cy);
    DrawCell(this, surf, m_tileHandles[rowBase + tileBounds.left], position.x, position.y, &corner);
    position.x += nearTileSize.cx;
    col = tileBounds.left + 1;
    if (col >= m_tileGridSize.cx) {
        col = 0;
    }
    for (i = interiorTileCount.cx; i > 0; i--) {
        DrawCell(this, surf, m_tileHandles[rowBase + col], position.x, position.y, &botSrc);
        position.x += m_tilePixelSize.cx;
        if (++col >= m_tileGridSize.cx) {
            col = 0;
        }
    }
    corner = MakeRect(0, 0, farTileSize.cx, farTileSize.cy);
    DrawCell(this, surf, m_tileHandles[rowBase + col], position.x, position.y, &corner);
}

static inline u16 PackPalEntry16(u8 r, u8 g, u8 b) {
    return static_cast<u16>(
        (static_cast<u8>(r >> g_rDown) << g_rUp) | (static_cast<u8>(g >> g_gDown) << g_gUp)
        | static_cast<u8>(b >> g_bDown)
    );
}

RVA(0x001628d0, 0x12)
i32 CDDrawWorkerHost::Prune() {
    if (m_spatialMgr == NULL) {
        return 0;
    }
    return m_spatialMgr->PruneCount();
}

// @early-stop
RVA(0x001628f0, 0x1fc)
i32 CDDrawWorkerHost::RebuildPlanes(const char* base, i32 count) {
    if (base == NULL) {
        return 0;
    }

    CWwdSpatialMgr*& spatialMgr = m_spatialMgr;
    if (spatialMgr) {
        delete spatialMgr;
        spatialMgr = NULL;
    }

    CRect rc = MakeRect(0, 0, m_planePixelSize.cx - 1, m_planePixelSize.cy - 1);

    CDDrawSurfaceMgr* reg = OwnerMgr();
    CDDrawChildGroup* activeGroup = reg->m_childGroup;
    if (activeGroup == NULL) {
        return 0;
    }
    CGameLevel* level = reg->m_level;
    if (level == NULL) {
        return 0;
    }

    LevelDims defaultCellSize = level->m_defaultActiveGridCellSize;
    LevelDims largeCellSize = level->m_largeActiveGridCellSize;
    LevelDims smallCellSize = level->m_smallActiveGridCellSize;
    LevelDims defaultRegionSize = level->m_defaultActiveRegionSize;
    LevelDims largeRegionSize = level->m_largeActiveRegionSize;
    LevelDims smallRegionSize = level->m_smallActiveRegionSize;

    CWwdSpatialMgr* newSpatialMgr = new CWwdSpatialMgr;
    spatialMgr = newSpatialMgr;
    if (newSpatialMgr->Init(
            activeGroup,
            &rc,
            &defaultCellSize.w,
            &largeCellSize.w,
            &smallCellSize.w,
            &defaultRegionSize.w,
            &largeRegionSize.w,
            &smallRegionSize.w
        )
        == 0) {
        delete m_spatialMgr;
        spatialMgr = NULL;
        return 0;
    }

    for (i32 i = 0; i < count; i++) {
        // Byte-forced view of packed WWD storage.

        i32 r = ReadPlaneObjects(reinterpret_cast<const PlaneObjectRecord*>(base));
        if (r == 0) {
            return 0;
        }
        base += r;
    }
    return 1;
}

// @early-stop
RVA(0x00162af0, 0x806)

i32 CDDrawWorkerHost::ReadPlaneObjects(const PlaneObjectRecord* src) {
    if (src == NULL) {
        return 0;
    }

    i32 nameLen = src->m_nameLen;
    i32 logicLen = src->m_logicLen;
    i32 imageSetLen = src->m_imageSetLen;
    i32 soundLen = src->m_soundLen;
    CPoint position(src->m_x, src->m_y);
    i32 z = src->m_z;
    i32 gridIndex = src->m_gridIndex;
    i32 id = src->m_id;

    CWwdSpriteObject* obj = new CWwdSpriteObject(OwnerMgr(), id, 0);
    if (obj == NULL) {
        return 0;
    }

    const char* strCursor = src->m_strings;
    char buf[0x400];

    i32 n = 0;
    if (nameLen > 0) {
        memcpy(buf, strCursor, nameLen);
        strCursor += nameLen;
        n = nameLen;
    }
    buf[n] = 0;
    CString name(buf);

    n = 0;
    if (logicLen > 0) {
        memcpy(buf, strCursor, logicLen);
        strCursor += logicLen;
        n = logicLen;
    }
    buf[n] = 0;
    CString logic(buf);

    n = 0;
    if (imageSetLen > 0) {
        memcpy(buf, strCursor, imageSetLen);
        strCursor += imageSetLen;
        n = imageSetLen;
    }
    buf[n] = 0;
    CString imageSet(buf);

    n = 0;
    if (soundLen > 0) {
        memcpy(buf, strCursor, soundLen);
        strCursor += soundLen;
        n = soundLen;
    }
    buf[n] = 0;
    CString sound(buf);

    if (position.x < 0 || position.x >= m_planePixelSize.cx || position.y < 0
        || position.y >= m_planePixelSize.cy) {
        i32 used = static_cast<i32>((strCursor - src->m_strings)) + 0x11c;
        delete obj;
        return used;
    }

    if (logic.IsEmpty()) {
        i32 used = static_cast<i32>((strCursor - src->m_strings)) + 0x11c;
        delete obj;
        return used;
    }

    CLogicRecord* logicTemplate =
        OwnerMgr()->m_logicRegistry->FindTemplate(static_cast<const char*>(logic));
    if (logicTemplate == NULL) {
        i32 used = static_cast<i32>((strCursor - src->m_strings)) + 0x11c;
        delete obj;
        return used;
    }

    if (obj->Setup(position.x, position.y, z, logicTemplate) == 0) {
        delete obj;
        return 0;
    }

    obj->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_WORLD_SPACE);

    CLogicRecord* anim = obj->m_logicRecord;
    if (anim == NULL) {
        delete obj;
        return 0;
    }

    if (imageSet.GetLength() != 0) {
        if (gridIndex != -1) {
            obj->SetImageFrameByName(static_cast<const char*>(imageSet), gridIndex);
        } else {
            obj->SetImageSetByName(static_cast<const char*>(imageSet));
        }
    }

    if (sound.GetLength() != 0) {
        obj->SetAnimationByName(static_cast<const char*>(sound), 0);
        obj->SetSoundCueByName(static_cast<const char*>(sound));
    }

    if (name.GetLength() != 0) {
        obj->m_name = static_cast<const char*>(name);
    }

    obj->m_flags |= static_cast<u32>(src->m_dynamicFlags);
    obj->m_stateFlags = static_cast<SpriteStateFlags>(src->m_stateFlags);
    anim->m_userFlags = src->m_userFlags;

    obj->m_score = src->m_score;
    obj->m_points = src->m_points;
    obj->m_powerup = src->m_powerup;
    obj->m_damage = src->m_damage;
    obj->m_smarts = src->m_smarts;
    obj->m_health = src->m_health;
    obj->m_extent = src->m_extent;
    obj->m_area = src->m_area;
    obj->m_switchRect = src->m_switchRect;
    obj->m_clip = src->m_clip;

    if (obj->m_area.left == 0 && obj->m_area.right == 0) {
        obj->m_area.left = COORD_UNSET;
    }
    if (obj->m_extent.left == 0 && obj->m_extent.right == 0) {
        obj->m_extent.left = COORD_UNSET;
    }
    if (obj->m_clip.left == 0 && obj->m_clip.right == 0) {
        obj->m_clip.left = COORD_UNSET;
    }
    if (obj->m_switchRect.left == 0 && obj->m_switchRect.right == 0) {
        obj->m_switchRect.left = COORD_UNSET;
    }

    anim->m_userRect1 = src->m_userRect1;
    anim->m_userRect2 = src->m_userRect2;
    anim->m_user1 = src->m_user[0];
    anim->m_user2 = src->m_user[1];
    anim->m_user3 = src->m_user[2];
    anim->m_user4 = src->m_user[3];
    anim->m_user5 = src->m_user[4];
    anim->m_user6 = src->m_user[5];
    anim->m_user7 = src->m_user[6];
    anim->m_user8 = src->m_user[7];
    anim->m_minX = src->m_minX;
    anim->m_minY = src->m_minY;
    anim->m_maxX = src->m_maxX;
    anim->m_maxY = src->m_maxY;
    obj->m_speed = Coord(src->m_speedX, src->m_speedY);
    anim->m_tweak = Coord(src->m_tweakX, src->m_tweakY);
    anim->m_counter = src->m_counter;
    anim->m_speed = src->m_speed;
    anim->m_size = CSize(src->m_width, src->m_height);
    obj->m_direction = src->m_direction;
    obj->m_faceDirection = src->m_faceDirection;
    anim->m_timeDelay = src->m_timeDelay;
    anim->m_frameDelay = src->m_frameDelay;
    obj->m_objectType = src->m_objectType;
    obj->m_hitTypeFlags = src->m_hitTypeFlags;

    Coord stride = obj->m_stride;
    u32 strideX = static_cast<u32>(src->m_strideX);
    if (strideX > 0) {
        stride.m_x = static_cast<i32>(strideX);
    }
    u32 strideY = static_cast<u32>(src->m_strideY);
    if (strideY > 0) {
        stride.m_y = static_cast<i32>(strideY);
    }
    obj->m_stride = stride;

    m_spatialMgr->ParkObject(static_cast<CWwdGameObject*>(obj));

    return static_cast<i32>((strCursor - src->m_strings)) + 0x11c;
}

RVA(0x00163300, 0x70)
i32 CDDrawWorkerHost::ActivateVisibleObjects() {
    CWwdSpatialMgr* scroll = m_spatialMgr;
    if (scroll == NULL) {
        return 0;
    }

    u32 flags = m_flags;

    CPoint position(
        (flags & IDX(WWD_PLANE_FLAG_WRAP_X))
            ? static_cast<i32>(m_scrollCenter.x)
            : (m_planeViewRect.right + m_planeViewRect.left) / 2 + 1,
        (flags & IDX(WWD_PLANE_FLAG_WRAP_Y))
            ? static_cast<i32>(m_scrollCenter.y)
            : (m_planeViewRect.bottom + m_planeViewRect.top) / 2 + 1
    );
    return scroll->ActivateAt(position.x, position.y);
}

RVA(0x00163370, 0x70)
i32 CDDrawWorkerHost::DeactivateDistantObjects() {
    CWwdSpatialMgr* scroll = m_spatialMgr;
    if (scroll == NULL) {
        return 0;
    }

    u32 flags = m_flags;

    CPoint position(
        (flags & IDX(WWD_PLANE_FLAG_WRAP_X))
            ? static_cast<i32>(m_scrollCenter.x)
            : (m_planeViewRect.right + m_planeViewRect.left) / 2 + 1,
        (flags & IDX(WWD_PLANE_FLAG_WRAP_Y))
            ? static_cast<i32>(m_scrollCenter.y)
            : (m_planeViewRect.bottom + m_planeViewRect.top) / 2 + 1
    );
    return scroll->DeactivateOutside(position.x, position.y);
}

RVA(0x001633e0, 0x12)
i32 CDDrawWorkerHost::ActivateKeepActiveObjects() {
    if (m_spatialMgr == NULL) {
        return 0;
    }
    return m_spatialMgr->ActivateKeepActiveObjects();
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00163400, 0x12)
i32 CDDrawWorkerHost::FlushAllObjects() {
    if (m_spatialMgr == NULL) {
        return 0;
    }
    return m_spatialMgr->FlushAll();
}

RVA(0x00163420, 0xf0)
void CDDrawWorkerHost::UpdateActiveRegionSizes() {
    if (m_spatialMgr == NULL) {
        return;
    }
    CGameLevel* level = OwnerMgr()->m_level;
    if (level == NULL) {
        return;
    }

    LevelDims defaultSize = level->m_defaultActiveRegionSize;

    LevelDims largeSize = level->m_largeActiveRegionSize;
    LevelDims smallSize = level->m_smallActiveRegionSize;

    CWwdSpatialMgr* spatialMgr = m_spatialMgr;
    spatialMgr->m_defaultRegionRect = MakeRect(0, 0, defaultSize.w - 1, defaultSize.h - 1);
    spatialMgr->m_defaultRegionHalfSize = CSize(defaultSize.w / 2, defaultSize.h / 2);

    spatialMgr = m_spatialMgr;
    spatialMgr->m_largeRegionRect = MakeRect(0, 0, largeSize.w - 1, largeSize.h - 1);
    spatialMgr->m_largeRegionHalfSize = CSize(largeSize.w / 2, largeSize.h / 2);

    spatialMgr = m_spatialMgr;
    spatialMgr->m_smallRegionRect = MakeRect(0, 0, smallSize.w - 1, smallSize.h - 1);
    spatialMgr->m_smallRegionHalfSize = CSize(smallSize.w / 2, smallSize.h / 2);

    spatialMgr = m_spatialMgr;
    spatialMgr->m_activeCenter = Coord(-22222, -22222);
}

// @early-stop
RVA(0x00163510, 0x156)
i32 CDDrawWorkerHost::ValidateTiles(char* errOut) {
    if (IsLoaded() == 0) {
        return 0;
    }

    char msg[0x80];
    i32 result = 1;
    for (i32 row = 0; row < m_tileGridSize.cy; row++) {
        for (i32 col = 0; col < m_tileGridSize.cx; col++) {
            i32 handle = m_tileHandles[m_tileRowOffsets[row] + col];
            if (handle == TILE_CLEAR || static_cast<u32>(handle) == UNINIT_FILL) {
                continue;
            }
            u32 setIdx = static_cast<u32>(handle) >> 16;
            CDDrawWorker* frame = ImageSetAt(setIdx);
            if (frame == NULL) {
                result = 0;
                if (errOut != NULL) {
                    sprintf(
                        msg,
                        "Plane %s: Bad map image set value (%i) at %i,%i\n",
                        m_planeName,
                        setIdx,
                        col,
                        row
                    );
                    strcat(errOut, msg);
                }
                continue;
            }
            i32 tile = handle & WWD_TILE_IMAGE_SET_INDEX_MASK;
            CImage* resolved = frame->GetAt(tile);
            if (resolved == NULL) {
                result = 0;
                if (errOut != NULL) {
                    sprintf(
                        msg,
                        "Plane %s: Bad map tile value (%i) at %i,%i\n",
                        m_planeName,
                        tile,
                        col,
                        row
                    );
                    strcat(errOut, msg);
                }
            }
        }
    }
    return result;
}

RVA(0x00163670, 0x95)
void CDDrawWorkerHost::ResolveColorKey() {
    ColorDepth format = OwnerMgr()->m_drawTarget->m_frontSurface->m_bpp;
    if (format == BPP_PALETTED_8) {
        return;
    }
    if (format != BPP_RGB_16) {
        return;
    }

    i32 idx = m_fillFx.dwFillColor;
    if (idx < 0) {
        return;
    }
    if (idx > 0xff) {
        return;
    }

    CDDrawPaletteResource* owner = OwnerMgr()->m_paletteRegistry->m_activePalette;
    if (owner == NULL) {
        return;
    }
    PALETTEENTRY* pal = owner->m_palette->m_entries;
    if (pal == NULL) {
        return;
    }

    u16 packed = PackPalEntry16(pal[idx].peRed, pal[idx].peGreen, pal[idx].peBlue);
    m_fillFx.dwFillColor = packed;
}

RVA(0x00163710, 0x60)
i32 CDDrawWorkerHost::SerializeDispatch(CFileMemBase* ar, SerialMode mode, LogicTypeId, i32) {
    if (!ar) {
        return 0;
    }
    switch (mode) {
        case SERIAL_PRESAVE:
            return 1;
        case SERIAL_SAVE:
            if (!Save(ar)) {
                return 0;
            }
            break;
        case SERIAL_POSTSAVE:
            return 1;
        case SERIAL_PRELOAD:
            return 1;
        case SERIAL_LOAD:
            if (!Load(ar)) {
                return 0;
            }
            break;
        case SERIAL_POSTLOAD:
            return 1;
    }
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00163770, 0xe)
i32 CDDrawWorkerHost::CanSave(CFileMemBase* s) {
    return s != NULL;
}

RVA(0x00163780, 0x134)
i32 CDDrawWorkerHost::Save(CFileMemBase* s) {
    if (s == NULL) {
        return 0;
    }

    s->Write(&m_scrollCenter.x, sizeof(m_scrollCenter.x));
    s->Write(&m_scrollCenter.y, sizeof(m_scrollCenter.y));
    s->Write(&m_scrollScale.x, sizeof(m_scrollScale.x));
    s->Write(&m_scrollScale.y, sizeof(m_scrollScale.y));
    s->Write(&m_planeViewRect.left, sizeof(m_planeViewRect));
    s->Write(&m_zCoord, sizeof(m_zCoord));
    s->Write(&m_scrollPixel.m_x, sizeof(m_scrollPixel.m_x));
    s->Write(&m_scrollPixel.m_y, sizeof(m_scrollPixel.m_y));
    s->Write(&m_movementPercent.m_x, sizeof(m_movementPercent.m_x));
    s->Write(&m_movementPercent.m_y, sizeof(m_movementPercent.m_y));

    i32 gridSize = m_tileGridSize.cx * m_tileGridSize.cy * 4;
    s->Write(&gridSize, sizeof(gridSize));
    s->Write(m_tileHandles, gridSize);

    char buf[SERIAL_NAME_LEN];
    memset(buf, 0, sizeof(buf));
    strcpy(buf, m_planeName);
    s->Write(buf, SERIAL_NAME_LEN);
    return 1;
}

RVA(0x001638c0, 0x140)
i32 CDDrawWorkerHost::Load(CFileMemBase* s) {
    if (s == NULL) {
        return 0;
    }

    s->Read(&m_scrollCenter.x, sizeof(m_scrollCenter.x));
    s->Read(&m_scrollCenter.y, sizeof(m_scrollCenter.y));
    s->Read(&m_scrollScale.x, sizeof(m_scrollScale.x));
    s->Read(&m_scrollScale.y, sizeof(m_scrollScale.y));
    s->Read(&m_planeViewRect.left, sizeof(m_planeViewRect));
    s->Read(&m_zCoord, sizeof(m_zCoord));
    s->Read(&m_scrollPixel.m_x, sizeof(m_scrollPixel.m_x));
    s->Read(&m_scrollPixel.m_y, sizeof(m_scrollPixel.m_y));
    s->Read(&m_movementPercent.m_x, sizeof(m_movementPercent.m_x));
    s->Read(&m_movementPercent.m_y, sizeof(m_movementPercent.m_y));

    i32 gridSize = 0;
    s->Read(&gridSize, sizeof(gridSize));
    if (gridSize != m_tileGridSize.cy * m_tileGridSize.cx * 4) {
        return 0;
    }
    s->Read(m_tileHandles, gridSize);

    char buf[SERIAL_NAME_LEN];
    s->Read(buf, SERIAL_NAME_LEN);
    strcpy(m_planeName, buf);
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00163a00, 0xe)
i32 CDDrawWorkerHost::CanLoad(CFileMemBase* s) {
    return s != NULL;
}

RVA_COMPGEN(0x00163a10, 0x7, ??1CWwdGridIter@@UAE@XZ)
RVA_COMPGEN(0x00163a40, 0x41, ??1CWwdSpatialMgr@@QAE@XZ)
