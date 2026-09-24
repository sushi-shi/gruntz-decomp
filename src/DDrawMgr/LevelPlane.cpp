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
#include <DDrawMgr/DDrawWorkerHostDrawMacros.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DDSurface.h>
#include <DDrawMgr/DirectDrawMgr.h>
#include <DDrawMgr/LogicRecordRegistry.h>
#include <DDrawMgr/LogicRecordRegistryFindInline.h>
#include <DDrawMgr/PixelShift.h>
#include <DDrawMgr/WorkerLookup.h>
#include <Enums.h>
#include <Gruntz/CoordNode.h>
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
#include <Utils/MapTyped.h>
#include <Wap32/CoordUnset.h>
#include <Wap32/WapObj.h>
#include <Wwd/WwdSpatialMgr.h>

#include <new>
#include <stdio.h>
#include <string.h>

RVA(0x001615a0, 0x9a)
CDDrawWorkerHost::CDDrawWorkerHost(CDDrawSurfaceMgr* owner, i32 id, i32 flags)
    : CWapObj(owner, id, flags, CWapObj::NO_SEED) {

    m_tileHandles = NULL;
    m_tileRowOffsets = NULL;
    m_spatialMgr = NULL;
    SET_VECTOR2_COMPONENTS(m_scrollScale, 1.0f, 1.0f);
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
    if (pd->m_headerSize != WWD_PLANE_HEADER_SIZE) {
        return 0;
    }

    char nameBuf[0x80];
    i32 pos = 0;
    const char* names = blockBase + pd->m_imageSetsOffset;
    for (u32 n = 0; n < pd->m_imageSetsCount; n++) {
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

            m_imageSets.SetAtGrow(static_cast<char>(n), (OwnerMgr())->FindWorker(nameBuf));
        }
    }

    m_flags = IDX(pd->m_flags);
    m_movementPercent.m_x = pd->m_movementXPercent;
    m_movementPercent.m_y = pd->m_movementYPercent;
    m_scrollCenter.m_x = 0;
    m_scrollCenter.m_y = 0;
    m_zCoord = -999999;
    m_tileGridSize.cx = pd->m_tilesWide;
    m_tileGridSize.cy = pd->m_tilesHigh;
    m_tilePixelSize.cx = pd->m_tilePixelWidth;
    m_tilePixelSize.cy = pd->m_tilePixelHeight;
    m_zCoord = pd->m_zCoord;
    m_viewportRect.left = bounds->left;
    m_viewportRect.top = bounds->top;
    m_viewportRect.right = bounds->right;
    m_viewportRect.bottom = bounds->bottom;
    m_tileRect.left = 0;
    m_tileRect.top = 0;
    m_tileRect.right = m_tilePixelSize.cx;
    m_tileRect.bottom = m_tilePixelSize.cy;
    m_planePixelSize.cx = m_tilePixelSize.cx * m_tileGridSize.cx;
    m_planePixelSize.cy = m_tilePixelSize.cy * m_tileGridSize.cy;

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
        SetTileSize(pd->m_tilePixelWidth, pd->m_tilePixelHeight);
    }

    strcpy(m_planeName, pd->m_name);
    m_fillFx.dwFillColor = pd->m_fillColor;
    m_flags = IDX(pd->m_flags);

    APPLY_WORKER_HOST_BOUNDS(bounds);

    m_scrollScale.m_x = static_cast<float>(m_movementPercent.m_x) * 0.01f;
    m_scrollScale.m_y = static_cast<float>(m_movementPercent.m_y) * 0.01f;

    m_tileHandles = new i32[m_tileGridSize.cy * m_tileGridSize.cx];
    // Byte-forced view of packed WWD storage.

    const i32* cell = reinterpret_cast<const i32*>(blockBase + pd->m_tilesOffset);
    for (u32 t = 0; t < static_cast<u32>(m_tileGridSize.cy * m_tileGridSize.cx); t++) {
        m_tileHandles[t] = *cell;
        cell++;
    }

    m_tileRowOffsets = new i32[m_tileGridSize.cy];
    for (i32 c = 0; c < m_tileGridSize.cy; c++) {
        m_tileRowOffsets[c] = c * m_tileGridSize.cx;
    }

    i32 originY = pd->m_scrollY;
    i32 originX = pd->m_scrollX;
    float sy = static_cast<float>(originY);
    float sx = static_cast<float>(originX);
    if ((m_flags & IDX(WWD_PLANE_FLAG_MAIN)) == 0) {
        sx *= m_scrollScale.m_x;
        sy *= m_scrollScale.m_y;
    }
    m_scrollCenter.m_x = sx;
    m_scrollCenter.m_y = sy;
    UpdatePlaneViewRect();

    if (pd->m_objectsOffset != 0) {
        if (RebuildPlanes(blockBase + pd->m_objectsOffset, pd->m_objectsCount) == 0) {
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
    m_tileGridSize.cx = tileColumns;
    m_tileGridSize.cy = tileRows;
    m_tilePixelSize.cx = tileWidthPx;
    m_tilePixelSize.cy = tileHeightPx;
    m_viewportRect.left = viewportRect->left;
    m_viewportRect.top = viewportRect->top;
    m_viewportRect.right = viewportRect->right;
    m_viewportRect.bottom = viewportRect->bottom;
    m_movementPercent.m_x = movementXPercent;
    m_movementPercent.m_y = movementYPercent;
    m_tileRect.left = 0;
    m_tileRect.top = 0;
    m_tileRect.bottom = tileHeightPx;
    m_planePixelSize.cx = tileWidthPx * tileColumns;
    m_planePixelSize.cy = tileHeightPx * tileRows;
    m_tileRect.right = tileWidthPx;
    m_viewportSize.cx = m_viewportRect.right - m_viewportRect.left + 1;
    m_viewportSize.cy = m_viewportRect.bottom - m_viewportRect.top + 1;
    m_viewHalfSize.cx = m_viewportSize.cx / 2;
    m_viewHalfSize.cy = m_viewportSize.cy / 2;
    m_tileShift.m_x = 0;
    i32 v = tileWidthPx;
    while (v > 1) {
        v >>= 1;
        m_tileShift.m_x = m_tileShift.m_x + 1;
    }
    m_tileShift.m_y = 0;
    v = tileWidthPx;
    while (v > 1) {
        v >>= 1;
        m_tileShift.m_y = m_tileShift.m_y + 1;
    }
    if (planeName != NULL) {
        strcpy(m_planeName, planeName);
    }
    APPLY_WORKER_HOST_BOUNDS(viewportRect);
    m_scrollScale.m_x = static_cast<float>(m_movementPercent.m_x) * 0.01f;
    m_scrollScale.m_y = static_cast<float>(m_movementPercent.m_y) * 0.01f;
    m_tileHandles = new i32[m_tileGridSize.cx * m_tileGridSize.cy];
    m_tileRowOffsets = new i32[m_tileGridSize.cy];
    for (i32 i = 0; i < m_tileGridSize.cy; i++) {
        m_tileRowOffsets[i] = i * m_tileGridSize.cx;
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
    m_imageSets.SetAtGrow(
        index,
        MapFind<CDDrawWorker>(OwnerMgr()->m_imageRegistry->m_workersByName, key)
    );
}

// @early-stop
RVA(0x00161c90, 0x1e4)
void CDDrawWorkerHost::UpdatePlaneViewRect() {
    CDDrawWorkerHost* p = this;
    WwdPlaneFlags flags = static_cast<WwdPlaneFlags>(p->m_flags);
    i32 wrapX, wrapY;
    wrapX = HAS(flags, WWD_PLANE_FLAG_WRAP_X);

    if (wrapX) {
        if (p->m_scrollCenter.m_x < 0.0f) {
            do {
                p->m_scrollCenter.m_x += static_cast<float>(p->m_planePixelSize.cx);
            } while (p->m_scrollCenter.m_x < 0.0f);
        }
        if (p->m_scrollCenter.m_x >= static_cast<float>(p->m_planePixelSize.cx)) {
            float t = p->m_scrollCenter.m_x;
            do {
                t -= static_cast<float>(p->m_planePixelSize.cx);
            } while (t >= static_cast<float>(p->m_planePixelSize.cx));
            p->m_scrollCenter.m_x = t;
        }
    } else {
        if (p->m_scrollCenter.m_x < 0.0f) {
            p->m_scrollCenter.m_x = 0;
        } else if (static_cast<float>(p->m_planePixelSize.cx) <= p->m_scrollCenter.m_x) {
            p->m_scrollCenter.m_x = static_cast<float>((p->m_planePixelSize.cx - 1));
        }
    }

    wrapY = HAS(flags, WWD_PLANE_FLAG_WRAP_Y);
    if (wrapY) {
        if (p->m_scrollCenter.m_y < 0.0f) {
            do {
                p->m_scrollCenter.m_y += static_cast<float>(p->m_planePixelSize.cy);
            } while (p->m_scrollCenter.m_y < 0.0f);
        }
        if (p->m_scrollCenter.m_y >= static_cast<float>(p->m_planePixelSize.cy)) {
            float t = p->m_scrollCenter.m_y;
            do {
                t -= static_cast<float>(p->m_planePixelSize.cy);
            } while (t >= static_cast<float>(p->m_planePixelSize.cy));
            p->m_scrollCenter.m_y = t;
        }
    } else {
        if (p->m_scrollCenter.m_y < 0.0f) {
            p->m_scrollCenter.m_y = 0;
        } else if (static_cast<float>(p->m_planePixelSize.cy) <= p->m_scrollCenter.m_y) {
            p->m_scrollCenter.m_y = static_cast<float>((p->m_planePixelSize.cy - 1));
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

RVA(0x00161e80, 0x79)
void CDDrawWorkerHost::SetViewportRect(LevelCoordRect* coords) {
    APPLY_WORKER_HOST_BOUNDS(coords);
}

RVA(0x00161f00, 0x75)
void CDDrawWorkerHost::SetTileSize(i32 tileWidthPx, i32 tileHeightPx) {
    m_tilePixelSize.cx = tileWidthPx;
    m_tilePixelSize.cy = tileHeightPx;
    SET_RECT_COMPONENTS(m_tileRect, 0, 0, tileWidthPx, tileHeightPx);
    m_planePixelSize.cx = m_tileGridSize.cx * tileWidthPx;
    m_planePixelSize.cy = m_tileGridSize.cy * tileHeightPx;
    TILE_SHIFT_INTO(m_tileShift.m_x, tileWidthPx);
    TILE_SHIFT_INTO(m_tileShift.m_y, tileWidthPx);
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

RVA(0x00162010, 0x8bd)
void CDDrawWorkerHost::Draw(CDDrawSurfacePair* ctx) {
    if ((m_flags & IDX(WWD_PLANE_FLAG_NO_DRAW)) != 0) {
        return;
    }
    i32 colL = m_planeViewRect.left >> m_tileShift.m_x;
    i32 leftW = ((colL + 1) << m_tileShift.m_x) - m_planeViewRect.left;
    i32 rowT = m_planeViewRect.top >> m_tileShift.m_y;
    i32 topH = ((rowT + 1) << m_tileShift.m_y) - m_planeViewRect.top;
    i32 colR = m_planeViewRect.right >> m_tileShift.m_x;
    i32 rightW = m_planeViewRect.right - (colR << m_tileShift.m_x) + 1;
    i32 rowB = m_planeViewRect.bottom >> m_tileShift.m_y;
    i32 botH = m_planeViewRect.bottom - (rowB << m_tileShift.m_y) + 1;
    RECT topSrc = MakeRect(0, m_tilePixelSize.cy - topH, m_tilePixelSize.cx, m_tilePixelSize.cy);
    RECT leftSrc = MakeRect(m_tilePixelSize.cx - leftW, 0, m_tilePixelSize.cx, m_tilePixelSize.cy);
    RECT rightSrc = {0, 0, rightW, m_tilePixelSize.cy};
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
    corner.left = m_tilePixelSize.cx - leftW;
    corner.top = m_tilePixelSize.cy - topH;
    corner.right = m_tilePixelSize.cx;
    corner.bottom = m_tilePixelSize.cy;
    DRAW_CELL(m_tileHandles[rowBase + colL], x, y, &corner);
    x += leftW;
    col = colL + 1;
    if (col >= m_tileGridSize.cx) {
        col = 0;
    }
    for (i = nCols; i > 0; i--) {
        DRAW_CELL(m_tileHandles[rowBase + col], x, y, &topSrc);
        x += m_tilePixelSize.cx;
        if (++col >= m_tileGridSize.cx) {
            col = 0;
        }
    }
    corner.left = 0;
    corner.top = m_tilePixelSize.cy - topH;
    corner.right = rightW;
    corner.bottom = m_tilePixelSize.cy;
    DRAW_CELL(m_tileHandles[rowBase + col], x, y, &corner);

    y += topH;
    row = rowT + 1;
    if (row >= m_tileGridSize.cy) {
        row = 0;
    }
    for (i32 r = nRows; r > 0; r--) {
        rowBase = m_tileRowOffsets[row];
        x = m_viewportRect.left;
        DRAW_CELL(m_tileHandles[rowBase + colL], x, y, &leftSrc);
        x += leftW;
        col = colL + 1;
        if (col >= m_tileGridSize.cx) {
            col = 0;
        }
        for (i = nCols; i > 0; i--) {
            DRAW_CELL(m_tileHandles[rowBase + col], x, y, &m_tileRect);
            x += m_tilePixelSize.cx;
            if (++col >= m_tileGridSize.cx) {
                col = 0;
            }
        }
        DRAW_CELL(m_tileHandles[rowBase + col], x, y, &rightSrc);
        y += m_tilePixelSize.cy;
        if (++row >= m_tileGridSize.cy) {
            row = 0;
        }
    }

    RECT botSrc = {0, 0, m_tilePixelSize.cx, botH};
    x = m_viewportRect.left;
    rowBase = m_tileRowOffsets[row];
    corner.left = m_tilePixelSize.cx - leftW;
    corner.top = 0;
    corner.right = m_tilePixelSize.cx;
    corner.bottom = botH;
    DRAW_CELL(m_tileHandles[rowBase + colL], x, y, &corner);
    x += leftW;
    col = colL + 1;
    if (col >= m_tileGridSize.cx) {
        col = 0;
    }
    for (i = nCols; i > 0; i--) {
        DRAW_CELL(m_tileHandles[rowBase + col], x, y, &botSrc);
        x += m_tilePixelSize.cx;
        if (++col >= m_tileGridSize.cx) {
            col = 0;
        }
    }
    corner.left = 0;
    corner.top = 0;
    corner.right = rightW;
    corner.bottom = botH;
    DRAW_CELL(m_tileHandles[rowBase + col], x, y, &corner);
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
            &defaultCellSize.m_w,
            &largeCellSize.m_w,
            &smallCellSize.m_w,
            &defaultRegionSize.m_w,
            &largeRegionSize.m_w,
            &smallRegionSize.m_w
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

    if (x < 0 || x >= m_planePixelSize.cx || y < 0 || y >= m_planePixelSize.cy) {
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
    obj->m_speed.m_x = *p++;
    obj->m_speed.m_y = *p++;
    anim->m_tweak.m_x = *p++;
    anim->m_tweak.m_y = *p++;
    anim->m_counter = *p++;
    anim->m_speed = *p++;
    anim->m_size.cx = *p++;
    anim->m_size.cy = *p++;
    obj->m_direction = *p++;
    obj->m_faceDirection = *p++;
    anim->m_timeDelay = *p++;
    anim->m_frameDelay = *p++;
    obj->m_objectType = *p++;
    obj->m_hitTypeFlags = *p++;

    u32 w = static_cast<u32>(*p++);
    if (w > 0) {
        obj->m_stride.m_x = static_cast<i32>(w);
    }
    u32 h = static_cast<u32>(*p++);
    if (h > 0) {
        obj->m_stride.m_y = static_cast<i32>(h);
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
        x = static_cast<i32>(m_scrollCenter.m_x);
    } else {
        i32 right = m_planeViewRect.right;
        x = (right + m_planeViewRect.left) / 2 + 1;
    }
    if (flags & IDX(WWD_PLANE_FLAG_WRAP_Y)) {
        y = static_cast<i32>(m_scrollCenter.m_y);
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
        x = static_cast<i32>(m_scrollCenter.m_x);
    } else {
        i32 right = m_planeViewRect.right;
        x = (right + m_planeViewRect.left) / 2 + 1;
    }
    if (flags & IDX(WWD_PLANE_FLAG_WRAP_Y)) {
        y = static_cast<i32>(m_scrollCenter.m_y);
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

    i32 defaultWidth = level->m_defaultActiveRegionSize.m_w;
    i32 defaultHeight = level->m_defaultActiveRegionSize.m_h;

    LevelDims largeSize;
    largeSize.m_w = level->m_largeActiveRegionSize.m_w;
    largeSize.m_h = level->m_largeActiveRegionSize.m_h;
    LevelDims smallSize;
    smallSize.m_w = level->m_smallActiveRegionSize.m_w;
    smallSize.m_h = level->m_smallActiveRegionSize.m_h;

    CWwdSpatialMgr* spatialMgr = m_spatialMgr;
    spatialMgr->m_defaultRegionRect.left = 0;
    spatialMgr->m_defaultRegionRect.top = 0;
    spatialMgr->m_defaultRegionRect.right = defaultWidth - 1;
    spatialMgr->m_defaultRegionRect.bottom = defaultHeight - 1;
    SET_SIZE_COMPONENTS(spatialMgr->m_defaultRegionHalfSize, defaultWidth / 2, defaultHeight / 2);

    spatialMgr = m_spatialMgr;
    spatialMgr->m_largeRegionRect.left = 0;
    spatialMgr->m_largeRegionRect.top = 0;
    spatialMgr->m_largeRegionRect.right = largeSize.m_w - 1;
    spatialMgr->m_largeRegionRect.bottom = largeSize.m_h - 1;
    SET_SIZE_COMPONENTS(spatialMgr->m_largeRegionHalfSize, largeSize.m_w / 2, largeSize.m_h / 2);

    spatialMgr = m_spatialMgr;
    spatialMgr->m_smallRegionRect.left = 0;
    spatialMgr->m_smallRegionRect.top = 0;
    spatialMgr->m_smallRegionRect.right = smallSize.m_w - 1;
    spatialMgr->m_smallRegionRect.bottom = smallSize.m_h - 1;
    SET_SIZE_COMPONENTS(spatialMgr->m_smallRegionHalfSize, smallSize.m_w / 2, smallSize.m_h / 2);

    spatialMgr = m_spatialMgr;
    SET_VECTOR2_COMPONENTS(spatialMgr->m_activeCenter, -22222, -22222);
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
            if (handle == s_tileClear || static_cast<u32>(handle) == UNINIT_FILL) {
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

    s->Write(&m_scrollCenter.m_x, sizeof(m_scrollCenter.m_x));
    s->Write(&m_scrollCenter.m_y, sizeof(m_scrollCenter.m_y));
    s->Write(&m_scrollScale.m_x, sizeof(m_scrollScale.m_x));
    s->Write(&m_scrollScale.m_y, sizeof(m_scrollScale.m_y));
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

    s->Read(&m_scrollCenter.m_x, sizeof(m_scrollCenter.m_x));
    s->Read(&m_scrollCenter.m_y, sizeof(m_scrollCenter.m_y));
    s->Read(&m_scrollScale.m_x, sizeof(m_scrollScale.m_x));
    s->Read(&m_scrollScale.m_y, sizeof(m_scrollScale.m_y));
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
