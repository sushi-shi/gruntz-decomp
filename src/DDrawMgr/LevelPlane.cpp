#include <rva.h>

#include <Mfc.h>

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
    m_scrollScaleX = 1.0f;
    m_scrollScaleY = 1.0f;
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
    m_movementXPercent = pd->movementXPercent;
    m_movementYPercent = pd->movementYPercent;
    m_scrollCenterX = 0;
    m_scrollCenterY = 0;
    m_zCoord = -999999;
    m_tileColumns = pd->tilesWide;
    m_tileRows = pd->tilesHigh;
    m_tileWidthPx = pd->tilePixelWidth;
    m_tileHeightPx = pd->tilePixelHeight;
    m_zCoord = pd->zCoord;
    m_viewportRect.left = bounds->left;
    m_viewportRect.top = bounds->top;
    m_viewportRect.right = bounds->right;
    m_viewportRect.bottom = bounds->bottom;
    m_tileRect.left = 0;
    m_tileRect.top = 0;
    m_tileRect.right = m_tileWidthPx;
    m_tileRect.bottom = m_tileHeightPx;
    m_planePixelWidth = m_tileWidthPx * m_tileColumns;
    m_planePixelHeight = m_tileHeightPx * m_tileRows;

    if (m_flags & IDX(WWD_PLANE_FLAG_AUTO_TILE_SIZE)) {

        CDDrawWorker* set = (m_imageSets.GetSize() > 0) ? ImageSetAt(0) : NULL;
        for (i32 f = 0; f < set->m_items.GetSize(); f++) {
            if (set->GetAt(f) != NULL) {
                CImage* first = set->GetAt(f);
                SET_TILE_SIZE_FROM_IMAGE(first);
                break;
            }
        }
    } else {
        SetTileSize(pd->tilePixelWidth, pd->tilePixelHeight);
    }

    strcpy(m_planeName, pd->name);
    m_fillFx.dwFillColor = pd->fillColor;
    m_flags = IDX(pd->flags);

    APPLY_WORKER_HOST_BOUNDS(bounds);

    m_scrollScaleX = static_cast<float>(m_movementXPercent) * 0.01f;
    m_scrollScaleY = static_cast<float>(m_movementYPercent) * 0.01f;

    m_tileHandles = new i32[m_tileRows * m_tileColumns];
    // Byte-forced view of packed WWD storage.

    const i32* cell = reinterpret_cast<const i32*>(blockBase + pd->tilesOffset);
    for (u32 t = 0; t < static_cast<u32>(m_tileRows * m_tileColumns); t++) {
        m_tileHandles[t] = *cell;
        cell++;
    }

    m_tileRowOffsets = new i32[m_tileRows];
    for (i32 c = 0; c < m_tileRows; c++) {
        m_tileRowOffsets[c] = c * m_tileColumns;
    }

    i32 originY = pd->scrollY;
    i32 originX = pd->scrollX;
    float sy = static_cast<float>(originY);
    float sx = static_cast<float>(originX);
    if ((m_flags & IDX(WWD_PLANE_FLAG_MAIN)) == 0) {
        sx *= m_scrollScaleX;
        sy *= m_scrollScaleY;
    }
    m_scrollCenterX = sx;
    m_scrollCenterY = sy;
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
    m_tileColumns = tileColumns;
    m_tileRows = tileRows;
    m_tileWidthPx = tileWidthPx;
    m_tileHeightPx = tileHeightPx;
    m_viewportRect.left = viewportRect->left;
    m_viewportRect.top = viewportRect->top;
    m_viewportRect.right = viewportRect->right;
    m_viewportRect.bottom = viewportRect->bottom;
    m_movementXPercent = movementXPercent;
    m_movementYPercent = movementYPercent;
    m_tileRect.left = 0;
    m_tileRect.top = 0;
    m_tileRect.bottom = tileHeightPx;
    m_planePixelWidth = tileWidthPx * tileColumns;
    m_planePixelHeight = tileHeightPx * tileRows;
    m_tileRect.right = tileWidthPx;
    m_viewportWidth = m_viewportRect.right - m_viewportRect.left + 1;
    m_viewportHeight = m_viewportRect.bottom - m_viewportRect.top + 1;
    m_viewHalfWidth = m_viewportWidth / 2;
    m_viewHalfHeight = m_viewportHeight / 2;
    m_shiftX = 0;
    i32 v = tileWidthPx;
    while (v > 1) {
        v >>= 1;
        m_shiftX = m_shiftX + 1;
    }
    m_shiftY = 0;
    v = tileWidthPx;
    while (v > 1) {
        v >>= 1;
        m_shiftY = m_shiftY + 1;
    }
    if (planeName != NULL) {
        strcpy(m_planeName, planeName);
    }
    APPLY_WORKER_HOST_BOUNDS(viewportRect);
    m_scrollScaleX = static_cast<float>(m_movementXPercent) * 0.01f;
    m_scrollScaleY = static_cast<float>(m_movementYPercent) * 0.01f;
    m_tileHandles = new i32[m_tileColumns * m_tileRows];
    m_tileRowOffsets = new i32[m_tileRows];
    for (i32 i = 0; i < m_tileRows; i++) {
        m_tileRowOffsets[i] = i * m_tileColumns;
    }
    SET_SCROLL_POSITION_ZERO(this);
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
        if (p->m_scrollCenterX < 0.0f) {
            do {
                p->m_scrollCenterX += static_cast<float>(p->m_planePixelWidth);
            } while (p->m_scrollCenterX < 0.0f);
        }
        if (p->m_scrollCenterX >= static_cast<float>(p->m_planePixelWidth)) {
            float t = p->m_scrollCenterX;
            do {
                t -= static_cast<float>(p->m_planePixelWidth);
            } while (t >= static_cast<float>(p->m_planePixelWidth));
            p->m_scrollCenterX = t;
        }
    } else {
        if (p->m_scrollCenterX < 0.0f) {
            p->m_scrollCenterX = 0;
        } else if (static_cast<float>(p->m_planePixelWidth) <= p->m_scrollCenterX) {
            p->m_scrollCenterX = static_cast<float>((p->m_planePixelWidth - 1));
        }
    }

    wrapY = HAS(flags, WWD_PLANE_FLAG_WRAP_Y);
    if (wrapY) {
        if (p->m_scrollCenterY < 0.0f) {
            do {
                p->m_scrollCenterY += static_cast<float>(p->m_planePixelHeight);
            } while (p->m_scrollCenterY < 0.0f);
        }
        if (p->m_scrollCenterY >= static_cast<float>(p->m_planePixelHeight)) {
            float t = p->m_scrollCenterY;
            do {
                t -= static_cast<float>(p->m_planePixelHeight);
            } while (t >= static_cast<float>(p->m_planePixelHeight));
            p->m_scrollCenterY = t;
        }
    } else {
        if (p->m_scrollCenterY < 0.0f) {
            p->m_scrollCenterY = 0;
        } else if (static_cast<float>(p->m_planePixelHeight) <= p->m_scrollCenterY) {
            p->m_scrollCenterY = static_cast<float>((p->m_planePixelHeight - 1));
        }
    }

    p->m_scrollPixelX = static_cast<i32>(p->m_scrollCenterX);
    i32 iy = static_cast<i32>(p->m_scrollCenterY);
    p->m_scrollPixelY = iy;

    p->m_planeViewRect.left = p->m_scrollPixelX - p->m_viewHalfWidth;
    if (p->m_planeViewRect.left < 0) {
        if (wrapX) {
            p->m_planeViewRect.left = p->m_planePixelWidth + p->m_planeViewRect.left;
        } else {
            p->m_planeViewRect.left = 0;
        }
    }

    i32 oy = iy - p->m_viewHalfHeight;
    p->m_planeViewRect.top = oy;
    if (oy < 0) {
        if (wrapY) {
            p->m_planeViewRect.top = p->m_planePixelHeight + oy;
        } else {
            p->m_planeViewRect.top = 0;
        }
    }

    i32 ex = p->m_viewportWidth + p->m_planeViewRect.left - 1;
    i32 ey = p->m_viewportHeight + p->m_planeViewRect.top - 1;
    p->m_planeViewRect.right = ex;
    p->m_planeViewRect.bottom = ey;
    if (ex >= p->m_planePixelWidth && wrapX == 0) {
        i32 over = ex - p->m_planePixelWidth + 1;
        p->m_planeViewRect.right = ex - over;
        p->m_planeViewRect.left = p->m_planeViewRect.left - over;
    }
    if (ey >= p->m_planePixelHeight && wrapY == 0) {
        i32 over = ey - p->m_planePixelHeight + 1;
        p->m_planeViewRect.bottom = ey - over;
        p->m_planeViewRect.top = p->m_planeViewRect.top - over;
    }
}

RVA(0x00161e80, 0x79)
void CDDrawWorkerHost::SetViewportRect(LevelCoordRect* coords) {
    APPLY_WORKER_HOST_BOUNDS(coords);
}

RVA(0x00161f00, 0x75)
void CDDrawWorkerHost::SetTileSize(i32 tileWidthPx, i32 tileHeightPx) {
    m_tileWidthPx = tileWidthPx;
    m_tileHeightPx = tileHeightPx;
    m_tileRect.left = 0;
    m_tileRect.top = 0;
    m_tileRect.right = tileWidthPx;
    m_tileRect.bottom = tileHeightPx;
    m_planePixelWidth = m_tileColumns * tileWidthPx;
    m_planePixelHeight = m_tileRows * tileHeightPx;
    m_shiftX = 0;
    i32 v = tileWidthPx;
    while (v > 1) {
        v >>= 1;
        m_shiftX = m_shiftX + 1;
    }
    m_shiftY = 0;
    v = tileWidthPx;
    while (v > 1) {
        v >>= 1;
        m_shiftY = m_shiftY + 1;
    }
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00161f80, 0x14)
void CDDrawWorkerHost::SetTileSizeFromImage(CImage* image) {
    SET_TILE_SIZE_FROM_IMAGE(image);
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00161fa0, 0x6c)
void CDDrawWorkerHost::SetTileSizeFromImageSet(CDDrawWorker* set) {
    for (i32 i = 0; i < set->m_items.GetSize(); i++) {
        if (set->GetAt(i) != NULL) {
            CImage* f = set->GetAt(i);
            SET_TILE_SIZE_FROM_IMAGE(f);
            break;
        }
    }
}

#define DRAW_CELL(handle, xp, yp, srcp)                                                            \
    do {                                                                                           \
        u32 h_ = static_cast<u32>(handle);                                                         \
        if (h_ == UNINIT_FILL) {                                                                   \
            dr.left = (xp);                                                                        \
            dr.top = (yp);                                                                         \
            dr.right = (xp) + ((srcp)->right - (srcp)->left);                                      \
            dr.bottom = (yp) + ((srcp)->bottom - (srcp)->top);                                     \
            surf->BltEx(&dr, 0, 0, DDBLT_WAIT | DDBLT_COLORFILL, &m_fillFx);                       \
        } else if (h_ != static_cast<u32>(TILE_CLEAR)) {                                           \
            CDDrawWorker* fr_ = ImageSetAt(h_ >> 16);                                              \
            i32 idx_ = static_cast<i32>(h_ & WWD_TILE_IMAGE_SET_INDEX_MASK);                       \
            CImage* e_ = fr_->GetAt(idx_);                                                         \
            surf->BltFast((xp), (yp), e_->m_surface, (srcp), e_->m_bltFastFlags);                  \
        }                                                                                          \
    } while (0)

// @early-stop
RVA(0x00162010, 0x8bd)
void CDDrawWorkerHost::Draw(CDDrawSurfacePair* ctx) {
    if ((m_flags & IDX(WWD_PLANE_FLAG_NO_DRAW)) != 0) {
        return;
    }
    i32 colL = m_planeViewRect.left >> m_shiftX;
    i32 leftW = ((colL + 1) << m_shiftX) - m_planeViewRect.left;
    i32 rowT = m_planeViewRect.top >> m_shiftY;
    i32 topH = ((rowT + 1) << m_shiftY) - m_planeViewRect.top;
    i32 colR = m_planeViewRect.right >> m_shiftX;
    i32 rightW = m_planeViewRect.right - (colR << m_shiftX) + 1;
    i32 rowB = m_planeViewRect.bottom >> m_shiftY;
    i32 botH = m_planeViewRect.bottom - (rowB << m_shiftY) + 1;
    RECT topSrc = MakeRect(0, m_tileHeightPx - topH, m_tileWidthPx, m_tileHeightPx);
    RECT leftSrc = MakeRect(m_tileWidthPx - leftW, 0, m_tileWidthPx, m_tileHeightPx);
    RECT rightSrc = {0, 0, rightW, m_tileHeightPx};
    RECT corner;
    RECT dr;
    CDDSurface* surf = ctx->m_surface;
    i32 nCols = colR - colL - 1;
    i32 nRows = rowB - rowT - 1;

    i32 x, y, col, row, i;
    i32 rowBase;

    rowBase = m_tileRowOffsets[rowT];
    y = m_viewportRect.top;
    x = m_viewportRect.left;
    corner.left = m_tileWidthPx - leftW;
    corner.top = m_tileHeightPx - topH;
    corner.right = m_tileWidthPx;
    corner.bottom = m_tileHeightPx;
    DRAW_CELL(m_tileHandles[rowBase + colL], x, y, &corner);
    x += leftW;
    col = colL + 1;
    if (col >= m_tileColumns) {
        col = 0;
    }
    for (i = nCols; i > 0; i--) {
        DRAW_CELL(m_tileHandles[rowBase + col], x, y, &topSrc);
        x += m_tileWidthPx;
        if (++col >= m_tileColumns) {
            col = 0;
        }
    }
    corner.left = 0;
    corner.top = m_tileHeightPx - topH;
    corner.right = rightW;
    corner.bottom = m_tileHeightPx;
    DRAW_CELL(m_tileHandles[rowBase + col], x, y, &corner);

    y += topH;
    row = rowT + 1;
    if (row >= m_tileRows) {
        row = 0;
    }
    for (i32 r = nRows; r > 0; r--) {
        rowBase = m_tileRowOffsets[row];
        x = m_viewportRect.left;
        DRAW_CELL(m_tileHandles[rowBase + colL], x, y, &leftSrc);
        x += leftW;
        col = colL + 1;
        if (col >= m_tileColumns) {
            col = 0;
        }
        for (i = nCols; i > 0; i--) {
            DRAW_CELL(m_tileHandles[rowBase + col], x, y, &m_tileRect);
            x += m_tileWidthPx;
            if (++col >= m_tileColumns) {
                col = 0;
            }
        }
        DRAW_CELL(m_tileHandles[rowBase + col], x, y, &rightSrc);
        y += m_tileHeightPx;
        if (++row >= m_tileRows) {
            row = 0;
        }
    }

    RECT botSrc = {0, 0, m_tileWidthPx, botH};
    x = m_viewportRect.left;
    rowBase = m_tileRowOffsets[row];
    corner.left = m_tileWidthPx - leftW;
    corner.top = 0;
    corner.right = m_tileWidthPx;
    corner.bottom = botH;
    DRAW_CELL(m_tileHandles[rowBase + colL], x, y, &corner);
    x += leftW;
    col = colL + 1;
    if (col >= m_tileColumns) {
        col = 0;
    }
    for (i = nCols; i > 0; i--) {
        DRAW_CELL(m_tileHandles[rowBase + col], x, y, &botSrc);
        x += m_tileWidthPx;
        if (++col >= m_tileColumns) {
            col = 0;
        }
    }
    corner.left = 0;
    corner.top = 0;
    corner.right = rightW;
    corner.bottom = botH;
    DRAW_CELL(m_tileHandles[rowBase + col], x, y, &corner);
}
#undef DRAW_CELL

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

    RECT rc;
    rc.left = 0;
    rc.top = 0;
    rc.right = m_planePixelWidth - 1;
    rc.bottom = m_planePixelHeight - 1;

    CDDrawSurfaceMgr* reg = OwnerMgr();
    CDDrawChildGroup* activeGroup = reg->m_childGroup;
    if (activeGroup == NULL) {
        return 0;
    }
    CGameLevel* level = reg->m_level;
    if (level == NULL) {
        return 0;
    }

    i32 defaultCellSize[2] = {
        level->m_defaultActiveGridCellSize[0],
        level->m_defaultActiveGridCellSize[1]
    };
    i32 largeCellSize[2] = {
        level->m_largeActiveGridCellSize[0],
        level->m_largeActiveGridCellSize[1]
    };
    i32 smallCellSize[2] = {
        level->m_smallActiveGridCellSize[0],
        level->m_smallActiveGridCellSize[1]
    };
    i32 defaultRegionSize[2] = {
        level->m_defaultActiveRegionSize.w,
        level->m_defaultActiveRegionSize.h
    };
    i32 largeRegionSize[2] = {level->m_largeActiveRegionSize.w, level->m_largeActiveRegionSize.h};
    i32 smallRegionSize[2] = {level->m_smallActiveRegionSize.w, level->m_smallActiveRegionSize.h};

    CWwdSpatialMgr* newSpatialMgr = new CWwdSpatialMgr;
    spatialMgr = newSpatialMgr;
    if (newSpatialMgr->Init(
            activeGroup,
            &rc,
            defaultCellSize,
            largeCellSize,
            smallCellSize,
            defaultRegionSize,
            largeRegionSize,
            smallRegionSize
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

    const i32* p = src->m_fields;
    i32 nameLen = *p++;
    i32 logicLen = *p++;
    i32 imageSetLen = *p++;
    i32 soundLen = *p++;
    i32 x = *p++;
    i32 y = *p++;
    i32 z = *p++;
    i32 gridIndex = *p++;
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

    if (x < 0 || x >= m_planePixelWidth || y < 0 || y >= m_planePixelHeight) {
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

    if (obj->Setup(x, y, z, logicTemplate) == 0) {
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

    p++;

    obj->m_flags |= static_cast<u32>(*p++);
    obj->m_stateFlags = static_cast<SpriteStateFlags>(*p++);
    anim->m_userFlags = *p++;

    obj->m_score = *p++;
    obj->m_points = *p++;
    obj->m_powerup = *p++;
    obj->m_damage = *p++;
    obj->m_smarts = *p++;
    obj->m_health = *p++;
    obj->m_extent.left = *p++;
    obj->m_extent.top = *p++;
    obj->m_extent.right = *p++;
    obj->m_extent.bottom = *p++;
    obj->m_area.left = *p++;
    obj->m_area.top = *p++;
    obj->m_area.right = *p++;
    obj->m_area.bottom = *p++;
    obj->m_switchRect.left = *p++;
    obj->m_switchRect.top = *p++;
    obj->m_switchRect.right = *p++;
    obj->m_switchRect.bottom = *p++;
    obj->m_clip.left = *p++;
    obj->m_clip.top = *p++;
    obj->m_clip.right = *p++;
    obj->m_clip.bottom = *p++;

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

    anim->m_userRect1.left = *p++;
    anim->m_userRect1.top = *p++;
    anim->m_userRect1.right = *p++;
    anim->m_userRect1.bottom = *p++;
    anim->m_userRect2.left = *p++;
    anim->m_userRect2.top = *p++;
    anim->m_userRect2.right = *p++;
    anim->m_userRect2.bottom = *p++;
    anim->m_user1 = *p++;
    anim->m_user2 = *p++;
    anim->m_user3 = *p++;
    anim->m_user4 = *p++;
    anim->m_user5 = *p++;
    anim->m_user6 = *p++;
    anim->m_user7 = *p++;
    anim->m_user8 = *p++;
    anim->m_minX = *p++;
    anim->m_minY = *p++;
    anim->m_maxX = *p++;
    anim->m_maxY = *p++;
    obj->m_speedX = *p++;
    obj->m_speedY = *p++;
    anim->m_tweakX = *p++;
    anim->m_tweakY = *p++;
    anim->m_counter = *p++;
    anim->m_speed = *p++;
    anim->m_width = *p++;
    anim->m_height = *p++;
    obj->m_direction = *p++;
    obj->m_faceDirection = *p++;
    anim->m_timeDelay = *p++;
    anim->m_frameDelay = *p++;
    obj->m_objectType = *p++;
    obj->m_hitTypeFlags = *p++;

    u32 w = static_cast<u32>(*p++);
    if (w > 0) {
        obj->m_strideX = static_cast<i32>(w);
    }
    u32 h = static_cast<u32>(*p++);
    if (h > 0) {
        obj->m_strideY = static_cast<i32>(h);
    }

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

    i32 x, y;
    if (flags & IDX(WWD_PLANE_FLAG_WRAP_X)) {
        x = static_cast<i32>(m_scrollCenterX);
    } else {
        i32 right = m_planeViewRect.right;
        x = (right + m_planeViewRect.left) / 2 + 1;
    }
    if (flags & IDX(WWD_PLANE_FLAG_WRAP_Y)) {
        y = static_cast<i32>(m_scrollCenterY);
    } else {
        i32 bottom = m_planeViewRect.bottom;
        y = (bottom + m_planeViewRect.top) / 2 + 1;
    }
    return scroll->ActivateAt(x, y);
}

RVA(0x00163370, 0x70)
i32 CDDrawWorkerHost::DeactivateDistantObjects() {
    CWwdSpatialMgr* scroll = m_spatialMgr;
    if (scroll == NULL) {
        return 0;
    }

    u32 flags = m_flags;

    i32 x, y;
    if (flags & IDX(WWD_PLANE_FLAG_WRAP_X)) {
        x = static_cast<i32>(m_scrollCenterX);
    } else {
        i32 right = m_planeViewRect.right;
        x = (right + m_planeViewRect.left) / 2 + 1;
    }
    if (flags & IDX(WWD_PLANE_FLAG_WRAP_Y)) {
        y = static_cast<i32>(m_scrollCenterY);
    } else {
        i32 bottom = m_planeViewRect.bottom;
        y = (bottom + m_planeViewRect.top) / 2 + 1;
    }
    return scroll->DeactivateOutside(x, y);
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

    i32 defaultWidth = level->m_defaultActiveRegionSize.w;
    i32 defaultHeight = level->m_defaultActiveRegionSize.h;

    LevelDims largeSize;
    largeSize.w = level->m_largeActiveRegionSize.w;
    largeSize.h = level->m_largeActiveRegionSize.h;
    LevelDims smallSize;
    smallSize.w = level->m_smallActiveRegionSize.w;
    smallSize.h = level->m_smallActiveRegionSize.h;

    CWwdSpatialMgr* spatialMgr = m_spatialMgr;
    spatialMgr->m_defaultRegionRect.left = 0;
    spatialMgr->m_defaultRegionRect.top = 0;
    spatialMgr->m_defaultRegionRect.right = defaultWidth - 1;
    spatialMgr->m_defaultRegionRect.bottom = defaultHeight - 1;
    spatialMgr->m_defaultRegionHalfWidth = defaultWidth / 2;
    spatialMgr->m_defaultRegionHalfHeight = defaultHeight / 2;

    spatialMgr = m_spatialMgr;
    spatialMgr->m_largeRegionRect.left = 0;
    spatialMgr->m_largeRegionRect.top = 0;
    spatialMgr->m_largeRegionRect.right = largeSize.w - 1;
    spatialMgr->m_largeRegionRect.bottom = largeSize.h - 1;
    spatialMgr->m_largeRegionHalfWidth = largeSize.w / 2;
    spatialMgr->m_largeRegionHalfHeight = largeSize.h / 2;

    spatialMgr = m_spatialMgr;
    spatialMgr->m_smallRegionRect.left = 0;
    spatialMgr->m_smallRegionRect.top = 0;
    spatialMgr->m_smallRegionRect.right = smallSize.w - 1;
    spatialMgr->m_smallRegionRect.bottom = smallSize.h - 1;
    spatialMgr->m_smallRegionHalfWidth = smallSize.w / 2;
    spatialMgr->m_smallRegionHalfHeight = smallSize.h / 2;

    spatialMgr = m_spatialMgr;
    spatialMgr->m_activeCenterX = -22222;
    spatialMgr->m_activeCenterY = -22222;
}

// @early-stop
RVA(0x00163510, 0x156)
i32 CDDrawWorkerHost::ValidateTiles(char* errOut) {
    if (IsLoaded() == 0) {
        return 0;
    }

    char msg[0x80];
    i32 result = 1;
    for (i32 row = 0; row < m_tileRows; row++) {
        for (i32 col = 0; col < m_tileColumns; col++) {
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

    s->Write(&m_scrollCenterX, sizeof(m_scrollCenterX));
    s->Write(&m_scrollCenterY, sizeof(m_scrollCenterY));
    s->Write(&m_scrollScaleX, sizeof(m_scrollScaleX));
    s->Write(&m_scrollScaleY, sizeof(m_scrollScaleY));
    s->Write(&m_planeViewRect.left, sizeof(m_planeViewRect));
    s->Write(&m_zCoord, sizeof(m_zCoord));
    s->Write(&m_scrollPixelX, sizeof(m_scrollPixelX));
    s->Write(&m_scrollPixelY, sizeof(m_scrollPixelY));
    s->Write(&m_movementXPercent, sizeof(m_movementXPercent));
    s->Write(&m_movementYPercent, sizeof(m_movementYPercent));

    i32 gridSize = m_tileColumns * m_tileRows * 4;
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

    s->Read(&m_scrollCenterX, sizeof(m_scrollCenterX));
    s->Read(&m_scrollCenterY, sizeof(m_scrollCenterY));
    s->Read(&m_scrollScaleX, sizeof(m_scrollScaleX));
    s->Read(&m_scrollScaleY, sizeof(m_scrollScaleY));
    s->Read(&m_planeViewRect.left, sizeof(m_planeViewRect));
    s->Read(&m_zCoord, sizeof(m_zCoord));
    s->Read(&m_scrollPixelX, sizeof(m_scrollPixelX));
    s->Read(&m_scrollPixelY, sizeof(m_scrollPixelY));
    s->Read(&m_movementXPercent, sizeof(m_movementXPercent));
    s->Read(&m_movementYPercent, sizeof(m_movementYPercent));

    i32 gridSize = 0;
    s->Read(&gridSize, sizeof(gridSize));
    if (gridSize != m_tileRows * m_tileColumns * 4) {
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
