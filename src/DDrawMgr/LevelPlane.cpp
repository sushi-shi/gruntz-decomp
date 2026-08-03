

#define CGAMEOBJECT_OOL_CTOR

#include <rva.h>

#include <Mfc.h>

#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorkerCache.h>
#include <DDrawMgr/DDrawWorkerHost.h>
#include <DDrawMgr/DDrawWorkerMapSmall.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DDSurface.h>
#include <DDrawMgr/DirectDrawMgr.h>
#include <DDrawMgr/PixelShift.h>
#include <Enums.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/Loadable.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/WwdGameObject.h>
#include <Image/CImage.h>
#include <Image/ImageSet.h>
#include <Io/FileMem.h>
#include <Wwd/WwdSpatialMgr.h>

#include <stdio.h>
#include <string.h>

RVA(0x001615a0, 0x9a)
CDDrawWorkerHost::CDDrawWorkerHost(CDDrawSurfaceMgr* mapData, i32 field04, i32 flags)
    : CLoadable(field04, flags, mapData) {

    m_tileGrid = NULL;
    m_colOffsets = NULL;
    m_scroll = NULL;
    m_scaleX = 1.0f;
    m_scaleY = 1.0f;
    m_bounds50.left = -1;
    memset(&m_bltFx, 0, sizeof(m_bltFx));
    m_bltFx.dwSize = sizeof(DDBLTFX);
}

// @early-stop
RVA(0x00161640, 0x3a2)
i32 CDDrawWorkerHost::Read(
    const WwdPlaneHeader* pd,
    const char* blockBase,
    LevelCoordRect* bounds
) {
    if (pd->headerSize != 0xa0) {
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

            CObject* val;
            val = NULL;
            OwnerMgr()->m_imageRegistry->m_10map.Lookup(nameBuf, val);
            m_frameSets.SetAtGrow(static_cast<char>(n), val);
        }
    }

    m_flags = pd->flags;
    m_movementXPercent = pd->movementXPercent;
    m_movementYPercent = pd->movementYPercent;
    m_scaledX = 0;
    m_scaledY = 0;
    m_zBound = -999999;
    m_gridW = pd->tilesWide;
    m_gridH = pd->tilesHigh;
    m_tilePxW = pd->tilePixelWidth;
    m_tilePxH = pd->tilePixelHeight;
    m_zBound = pd->zCoord;
    m_bounds50.left = bounds->left;
    m_bounds50.top = bounds->top;
    m_bounds50.right = bounds->right;
    m_bounds50.bottom = bounds->bottom;
    m_fillRect.left = 0;
    m_fillRect.top = 0;
    m_fillRect.right = m_tilePxW;
    m_fillRect.bottom = m_tilePxH;
    m_wrapW = m_tilePxW * m_gridW;
    m_wrapH = m_tilePxH * m_gridH;

    if (m_flags & 0x10) {

        CDDrawWorker* set = (m_frameSets.GetSize() > 0) ? FrameSetAt(0) : 0;
        for (i32 f = 0; f < set->m_items.GetSize(); f++) {
            if (set->GetAt(f) != NULL) {
                CImage* first = set->GetAt(f);
                SetTileSize(first->m_width, first->m_height);
                break;
            }
        }
    } else {
        SetTileSize(pd->tilePixelWidth, pd->tilePixelHeight);
    }

    strcpy(m_name, pd->name);
    m_bltFx.dwFillColor = pd->fillColor;
    m_flags = pd->flags;

    if (bounds->left != LEVEL_COORD_UNSET) {
        LevelCoordRect local;
        CopyRect((&local), (bounds));
        m_bounds50 = local;
        m_viewW = m_bounds50.right - m_bounds50.left + 1;
        m_viewH = m_bounds50.bottom - m_bounds50.top + 1;
        m_anchorX = m_viewW / 2;
        m_anchorY = m_viewH / 2;
        RecomputePlaneCoords();
    }

    m_scaleX = static_cast<float>(m_movementXPercent) * 0.01f;
    m_scaleY = static_cast<float>(m_movementYPercent) * 0.01f;

    m_tileGrid = static_cast<i32*>(operator new(m_gridH * m_gridW * 4));
    // Byte-forced view of packed WWD storage.

    const i32* cell = reinterpret_cast<const i32*>(blockBase + pd->tilesOffset);
    for (u32 t = 0; t < static_cast<u32>(m_gridH * m_gridW); t++) {
        m_tileGrid[t] = *cell;
        cell++;
    }

    m_colOffsets = static_cast<i32*>(operator new(m_gridH * 4));
    for (i32 c = 0; c < m_gridH; c++) {
        m_colOffsets[c] = c * m_gridW;
    }

    i32 originY = pd->scrollY;
    i32 originX = pd->scrollX;
    float sy = static_cast<float>(originY);
    float sx = static_cast<float>(originX);
    if ((m_flags & 1) == 0) {
        sx *= m_scaleX;
        sy *= m_scaleY;
    }
    m_scaledX = sx;
    m_scaledY = sy;
    RecomputePlaneCoords();

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
    i32 w,
    i32 h,
    i32 tileW,
    i32 tileH,
    i32 depthX,
    i32 depthY,
    LevelCoordRect* bounds,
    char* name
) {
    m_gridW = w;
    m_gridH = h;
    m_tilePxW = tileW;
    m_tilePxH = tileH;
    m_bounds50.left = bounds->left;
    m_bounds50.top = bounds->top;
    m_bounds50.right = bounds->right;
    m_bounds50.bottom = bounds->bottom;
    m_movementXPercent = depthX;
    m_movementYPercent = depthY;
    m_fillRect.left = 0;
    m_fillRect.top = 0;
    m_fillRect.bottom = tileH;
    m_wrapW = tileW * w;
    m_wrapH = tileH * h;
    m_fillRect.right = tileW;
    m_viewW = m_bounds50.right - m_bounds50.left + 1;
    m_viewH = m_bounds50.bottom - m_bounds50.top + 1;
    m_anchorX = m_viewW / 2;
    m_anchorY = m_viewH / 2;
    m_shiftX = 0;
    if (tileW > 1) {
        i32 v = tileW;
        do {
            v >>= 1;
            m_shiftX = m_shiftX + 1;
        } while (v > 1);
    }
    m_shiftY = 0;
    if (tileW > 1) {
        i32 v = tileW;
        do {
            v >>= 1;
            m_shiftY = m_shiftY + 1;
        } while (v > 1);
    }
    if (name != NULL) {
        strcpy(m_name, name);
    }
    if (bounds->left != static_cast<i32>(0x80000000)) {
        LevelCoordRect local;
        CopyRect((&local), (bounds));
        m_bounds50 = local;
        m_viewW = m_bounds50.right - m_bounds50.left + 1;
        m_viewH = m_bounds50.bottom - m_bounds50.top + 1;
        m_anchorX = m_viewW / 2;
        m_anchorY = m_viewH / 2;
        RecomputePlaneCoords();
    }
    m_scaleX = static_cast<float>(m_movementXPercent) * 0.01f;
    m_scaleY = static_cast<float>(m_movementYPercent) * 0.01f;
    m_tileGrid = static_cast<i32*>(operator new(m_gridW * m_gridH * 4));
    m_colOffsets = static_cast<i32*>(operator new(m_gridH * 4));
    for (i32 i = 0; i < m_gridH; i++) {
        m_colOffsets[i] = i * m_gridW;
    }
    m_scaledX = 0;
    m_scaledY = 0;
    RecomputePlaneCoords();
    return 1;
}

RVA(0x00161bf0, 0x5e)
void CDDrawWorkerHost::Unload() {
    if (m_scroll != NULL) {
        m_scroll->PruneCount();
    }
    CWwdSpatialMgr* g = m_scroll;
    delete g;
    if (m_tileGrid != NULL) {
        ::operator delete(m_tileGrid);
        m_tileGrid = NULL;
    }
    if (m_colOffsets != NULL) {
        ::operator delete(m_colOffsets);
        m_colOffsets = NULL;
    }
}

// @early-stop
RVA(0x00161c50, 0x3f)
void CDDrawWorkerHost::RegisterNamed(char index, const char* key) {
    CObject* val;
    val = NULL;
    OwnerMgr()->m_imageRegistry->m_10map.Lookup(key, val);
    m_frameSets.SetAtGrow(index, val);
}

RVA(0x00161c90, 0x1e4)
void CDDrawWorkerHost::RecomputePlaneCoords() {
    CDDrawWorkerHost* p = this;
    u32 flags = p->m_flags;
    i32 wrapX, wrapY;
    wrapX = flags & 4;

    if (wrapX) {
        if (p->m_scaledX < 0.0f) {
            do {
                p->m_scaledX += static_cast<float>(p->m_wrapW);
            } while (p->m_scaledX < 0.0f);
        }
        if (p->m_scaledX >= static_cast<float>(p->m_wrapW)) {
            float t = p->m_scaledX;
            do {
                t -= static_cast<float>(p->m_wrapW);
            } while (t >= static_cast<float>(p->m_wrapW));
            p->m_scaledX = t;
        }
    } else {
        if (p->m_scaledX < 0.0f) {
            p->m_scaledX = 0;
        } else if (static_cast<float>(p->m_wrapW) <= p->m_scaledX) {
            p->m_scaledX = static_cast<float>((p->m_wrapW - 1));
        }
    }

    wrapY = flags & 8;
    if (wrapY) {
        if (p->m_scaledY < 0.0f) {
            do {
                p->m_scaledY += static_cast<float>(p->m_wrapH);
            } while (p->m_scaledY < 0.0f);
        }
        if (p->m_scaledY >= static_cast<float>(p->m_wrapH)) {
            float t = p->m_scaledY;
            do {
                t -= static_cast<float>(p->m_wrapH);
            } while (t >= static_cast<float>(p->m_wrapH));
            p->m_scaledY = t;
        }
    } else {
        if (p->m_scaledY < 0.0f) {
            p->m_scaledY = 0;
        } else if (static_cast<float>(p->m_wrapH) <= p->m_scaledY) {
            p->m_scaledY = static_cast<float>((p->m_wrapH - 1));
        }
    }

    i32 ix = static_cast<i32>(p->m_scaledX);
    p->m_snappedX = ix;
    i32 iy = static_cast<i32>(p->m_scaledY);
    p->m_snappedY = iy;

    i32 ox = ix - p->m_anchorX;
    p->m_viewRect.left = ox;
    if (ox < 0) {
        if (wrapX) {
            p->m_viewRect.left = p->m_wrapW + ox;
        } else {
            p->m_viewRect.left = 0;
        }
    }

    i32 oy = iy - p->m_anchorY;
    p->m_viewRect.top = oy;
    if (oy < 0) {
        if (wrapY) {
            p->m_viewRect.top = p->m_wrapH + oy;
        } else {
            p->m_viewRect.top = 0;
        }
    }

    i32 ex = p->m_viewW + p->m_viewRect.left - 1;
    i32 ey = p->m_viewH + p->m_viewRect.top - 1;
    p->m_viewRect.right = ex;
    p->m_viewRect.bottom = ey;
    if (ex >= p->m_wrapW && wrapX == 0) {
        i32 over = ex - p->m_wrapW + 1;
        p->m_viewRect.right = ex - over;
        p->m_viewRect.left = p->m_viewRect.left - over;
    }
    if (ey >= p->m_wrapH && wrapY == 0) {
        i32 over = ey - p->m_wrapH + 1;
        p->m_viewRect.bottom = ey - over;
        p->m_viewRect.top = p->m_viewRect.top - over;
    }
}

RVA(0x00161e80, 0x79)
void CDDrawWorkerHost::Build(LevelCoordRect* coords) {
    if (coords->left != static_cast<i32>(0x80000000)) {
        LevelCoordRect local;
        CopyRect((&local), (coords));
        m_bounds50 = local;
        m_viewW = m_bounds50.right - m_bounds50.left + 1;
        m_viewH = m_bounds50.bottom - m_bounds50.top + 1;
        m_anchorX = m_viewW / 2;
        m_anchorY = m_viewH / 2;
        RecomputePlaneCoords();
    }
}

// @early-stop
RVA(0x00161f00, 0x75)
void CDDrawWorkerHost::SetTileSize(i32 tileW, i32 tileH) {
    m_wrapW = m_gridW * tileW;
    m_tilePxH = tileH;
    m_fillRect.bottom = tileH;
    m_tilePxW = tileW;
    m_fillRect.left = 0;
    m_fillRect.top = 0;
    m_fillRect.right = tileW;
    m_wrapH = m_gridH * tileH;
    m_shiftX = 0;
    for (i32 t = tileW; t > 1; t >>= 1) {
        m_shiftX++;
    }
    m_shiftY = 0;
    for (i32 u = tileW; u > 1; u >>= 1) {
        m_shiftY++;
    }
}

RVA(0x00161fa0, 0x6c)
void CDDrawWorkerHost::SetTileSizeFromImageSet(CDDrawWorker* set) {
    for (i32 i = 0; i < set->m_items.GetSize(); i++) {
        if (set->GetAt(i) != NULL) {
            CImage* f = set->GetAt(i);
            SetTileSize(f->m_width, f->m_height);
            break;
        }
    }
}

#define DRAW_CELL(handle, xp, yp, srcp)                                                            \
    do {                                                                                           \
        u32 h_ = static_cast<u32>(handle);                                                         \
        if (h_ == 0xeeeeeeee) {                                                                    \
            dr.left = (xp);                                                                        \
            dr.top = (yp);                                                                         \
            dr.right = (xp) + ((srcp)->right - (srcp)->left);                                      \
            dr.bottom = (yp) + ((srcp)->bottom - (srcp)->top);                                     \
            surf->BltEx(&dr, 0, 0, 0x1000400, &m_bltFx);                                           \
        } else if (h_ != 0xffffffff) {                                                             \
            CDDrawWorker* fr_ = FrameSetAt(h_ >> 16);                                              \
            i32 idx_ = static_cast<i32>(h_ & 0xffff);                                              \
            CImage* e_ = fr_->GetAt(idx_);                                                         \
            surf->BltFast((xp), (yp), e_->m_surface, (srcp), e_->m_loadResult);                    \
        }                                                                                          \
    } while (0)

// @early-stop
RVA(0x00162010, 0x8bd)
void CDDrawWorkerHost::Draw(CDDrawSurfacePair* ctx) {
    if ((m_flags & 2) != 0) {
        return;
    }
    CDDSurface* surf = ctx->m_surface;

    i32 colL = m_viewRect.left >> m_shiftX;
    i32 leftW = ((colL + 1) << m_shiftX) - m_viewRect.left;
    i32 rowT = m_viewRect.top >> m_shiftY;
    i32 topH = ((rowT + 1) << m_shiftY) - m_viewRect.top;
    i32 colR = m_viewRect.right >> m_shiftX;
    i32 rightW = m_viewRect.right - (colR << m_shiftX) + 1;
    i32 rowB = m_viewRect.bottom >> m_shiftY;
    i32 botH = m_viewRect.bottom - (rowB << m_shiftY) + 1;
    i32 nCols = colR - colL - 1;
    i32 nRows = rowB - rowT - 1;

    RECT topSrc = {0, m_tilePxH - topH, m_tilePxW, m_tilePxH};
    RECT leftSrc = {m_tilePxW - leftW, 0, m_tilePxW, m_tilePxH};
    RECT rightSrc = {0, 0, rightW, m_tilePxH};
    RECT corner;
    RECT dr;

    i32 x, y, col, row, i;
    i32 rowBase;

    y = m_bounds50.top;
    x = m_bounds50.left;
    rowBase = m_colOffsets[rowT];
    corner.left = m_tilePxW - leftW;
    corner.top = m_tilePxH - topH;
    corner.right = m_tilePxW;
    corner.bottom = m_tilePxH;
    DRAW_CELL(m_tileGrid[rowBase + colL], x, y, &corner);
    x += leftW;
    col = colL + 1;
    if (col >= m_gridW) {
        col = 0;
    }
    for (i = nCols; i > 0; i--) {
        DRAW_CELL(m_tileGrid[rowBase + col], x, y, &topSrc);
        x += m_tilePxW;
        if (++col >= m_gridW) {
            col = 0;
        }
    }
    corner.left = 0;
    corner.top = m_tilePxH - topH;
    corner.right = rightW;
    corner.bottom = m_tilePxH;
    DRAW_CELL(m_tileGrid[rowBase + col], x, y, &corner);

    y += topH;
    row = rowT + 1;
    if (row >= m_gridH) {
        row = 0;
    }
    for (i32 r = nRows; r > 0; r--) {
        rowBase = m_colOffsets[row];
        x = m_bounds50.left;
        DRAW_CELL(m_tileGrid[rowBase + colL], x, y, &leftSrc);
        x += leftW;
        col = colL + 1;
        if (col >= m_gridW) {
            col = 0;
        }
        for (i = nCols; i > 0; i--) {
            DRAW_CELL(m_tileGrid[rowBase + col], x, y, &m_fillRect);
            x += m_tilePxW;
            if (++col >= m_gridW) {
                col = 0;
            }
        }
        DRAW_CELL(m_tileGrid[rowBase + col], x, y, &rightSrc);
        y += m_tilePxH;
        if (++row >= m_gridH) {
            row = 0;
        }
    }

    RECT botSrc = {0, 0, m_tilePxW, botH};
    x = m_bounds50.left;
    rowBase = m_colOffsets[row];
    corner.left = m_tilePxW - leftW;
    corner.top = 0;
    corner.right = m_tilePxW;
    corner.bottom = botH;
    DRAW_CELL(m_tileGrid[rowBase + colL], x, y, &corner);
    x += leftW;
    col = colL + 1;
    if (col >= m_gridW) {
        col = 0;
    }
    for (i = nCols; i > 0; i--) {
        DRAW_CELL(m_tileGrid[rowBase + col], x, y, &botSrc);
        x += m_tilePxW;
        if (++col >= m_gridW) {
            col = 0;
        }
    }
    corner.left = 0;
    corner.top = 0;
    corner.right = rightW;
    corner.bottom = botH;
    DRAW_CELL(m_tileGrid[rowBase + col], x, y, &corner);
}
#undef DRAW_CELL

inline void* operator new(u32, void* p) {
    return p;
}

RVA(0x001628d0, 0x12)
i32 CDDrawWorkerHost::Prune() {
    if (m_scroll == NULL) {
        return 0;
    }
    return m_scroll->PruneCount();
}

// @early-stop
RVA(0x001628f0, 0x1fc)
i32 CDDrawWorkerHost::RebuildPlanes(const char* base, i32 count) {
    if (base == NULL) {
        return 0;
    }

    CWwdSpatialMgr*& worker = m_scroll;
    if (worker) {
        worker->FreeGrids();
        worker->m_iter.~CWwdGridIter();
        ::operator delete(worker);
        worker = NULL;
    }

    RECT rc;
    rc.left = 0;
    rc.top = 0;
    rc.right = m_wrapW - 1;
    rc.bottom = m_wrapH - 1;

    CDDrawSurfaceMgr* reg = OwnerMgr();
    CDDrawChildGroup* src = reg->m_childGroup;
    if (src == NULL) {
        return 0;
    }
    CGameLevel* hdr = reg->m_level;
    if (hdr == NULL) {
        return 0;
    }

    i32 cellA[2] = {hdr->m_pairA[0], hdr->m_pairA[1]};
    i32 cellB[2] = {hdr->m_pairB[0], hdr->m_pairB[1]};
    i32 cellC[2] = {hdr->m_pairC[0], hdr->m_pairC[1]};
    i32 sizeA[2] = {hdr->m_rectA.w, hdr->m_rectA.h};
    i32 sizeB[2] = {hdr->m_rectB.w, hdr->m_rectB.h};
    i32 sizeC[2] = {hdr->m_rectC.w, hdr->m_rectC.h};

    CWwdSpatialMgr* nw = static_cast<CWwdSpatialMgr*>(::operator new(0xb8));
    if (nw) {

        nw->m_iter.m_grid = NULL;
        nw->m_iter.m_cur = NULL;
        nw->m_mgr = NULL;
        nw->m_grid0 = NULL;
        nw->m_grid1 = NULL;
        nw->m_grid2 = NULL;
        nw->m_curGrid = NULL;
    }
    worker = nw;
    if (nw->Init(src, &rc, cellA, cellB, cellC, sizeA, sizeB, sizeC) == 0) {
        CWwdSpatialMgr* w = m_scroll;
        if (w) {
            w->FreeGrids();

            ::operator delete(w);
        }
        worker = NULL;
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

    CWwdGameObjectA* obj = new CWwdGameObjectA(OwnerMgr(), id, 0);
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

    if (x < 0 || x >= m_wrapW || y < 0 || y >= m_wrapH) {
        delete obj;
        return static_cast<i32>((strCursor - src->m_strings)) + 0x11c;
    }

    AnimWorkerObj* tmpl = 0;
    if (logic.GetLength() != 0) {
        CObject* foundOb = 0;
        OwnerMgr()->m_workerCache->m_workers.Lookup(static_cast<const char*>(logic), foundOb);
        tmpl = static_cast<AnimWorkerObj*>(foundOb);
    }
    if (tmpl == NULL) {
        delete obj;
        return static_cast<i32>((strCursor - src->m_strings)) + 0x11c;
    }

    if (obj->Setup(x, y, z, tmpl) == 0) {
        delete obj;
        return 0;
    }

    obj->m_flags |= 0x40000;

    AnimWorkerObj* anim = obj->m_animWorker;
    if (anim == NULL) {
        delete obj;
        return 0;
    }

    if (imageSet.GetLength() != 0) {
        if (gridIndex != -1) {
            obj->ApplyLookupSprite(static_cast<const char*>(imageSet), gridIndex);
        } else {
            obj->ApplyName(static_cast<const char*>(imageSet));
        }
    }

    if (sound.GetLength() != 0) {
        obj->ApplyLookupGeometry(static_cast<const char*>(sound), 0);
        obj->LookupAnimSprite(static_cast<const char*>(sound));
    }

    if (name.GetLength() != 0) {
        obj->m_name = static_cast<const char*>(name);
    }

    p++;

    u32 dynFlags = static_cast<u32>(*p++);
    obj->m_flags |= dynFlags;
    obj->m_stateFlags = *p++;
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
        obj->m_area.left = static_cast<i32>(0x80000000);
    }
    if (obj->m_extent.left == 0 && obj->m_extent.right == 0) {
        obj->m_extent.left = static_cast<i32>(0x80000000);
    }
    if (obj->m_clip.left == 0 && obj->m_clip.right == 0) {
        obj->m_clip.left = static_cast<i32>(0x80000000);
    }
    if (obj->m_switchRect.left == 0 && obj->m_switchRect.right == 0) {
        obj->m_switchRect.left = static_cast<i32>(0x80000000);
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

    m_scroll->RemoveObject(static_cast<CWwdGameObject*>(obj));

    return static_cast<i32>((strCursor - src->m_strings)) + 0x11c;
}

// @early-stop
RVA(0x00163300, 0x70)
i32 CDDrawWorkerHost::ActivateVisibleObjects() {
    CWwdSpatialMgr* scroll = m_scroll;
    if (scroll == NULL) {
        return 0;
    }

    u32 flags = m_flags;

    i32 x;
    if (flags & 0x4) {
        x = static_cast<i32>(m_scaledX);
    } else {
        x = (m_viewRect.left + m_viewRect.right) / 2 + 1;
    }

    i32 y;
    if (flags & 0x8) {
        y = static_cast<i32>(m_scaledY);
        return scroll->ScrollTo(x, y);
    }
    y = (m_viewRect.top + m_viewRect.bottom) / 2 + 1;
    return scroll->ScrollTo(x, y);
}

// @early-stop
RVA(0x00163370, 0x70)
i32 CDDrawWorkerHost::DeactivateDistantObjects() {
    CWwdSpatialMgr* scroll = m_scroll;
    if (scroll == NULL) {
        return 0;
    }

    u32 flags = m_flags;

    i32 x;
    if (flags & 0x4) {
        x = static_cast<i32>(m_scaledX);
    } else {

        i32 right = m_viewRect.right;
        x = (right + m_viewRect.left) / 2 + 1;
    }

    i32 y;
    if (flags & 0x8) {
        y = static_cast<i32>(m_scaledY);
        return scroll->Relocate(x, y);
    }
    i32 bottom = m_viewRect.bottom;
    y = (bottom + m_viewRect.top) / 2 + 1;
    return scroll->Relocate(x, y);
}

RVA(0x001633e0, 0x12)
i32 CDDrawWorkerHost::GetSize() {
    if (m_scroll == NULL) {
        return 0;
    }
    return m_scroll->GetSize();
}

RVA(0x00163420, 0xf0)
void CDDrawWorkerHost::InitScrollRects() {
    if (m_scroll == NULL) {
        return;
    }
    CGameLevel* g = OwnerMgr()->m_level;
    if (g == NULL) {
        return;
    }

    i32 c8 = g->m_rectA.w;
    i32 cc = g->m_rectA.h;

    LevelDims b;
    b.w = g->m_rectB.w;
    b.h = g->m_rectB.h;
    LevelDims c;
    c.w = g->m_rectC.w;
    c.h = g->m_rectC.h;

    CWwdSpatialMgr* s = m_scroll;
    s->m_rect0.left = 0;
    s->m_rect0.top = 0;
    s->m_rect0.right = c8 - 1;
    s->m_rect0.bottom = cc - 1;
    s->m_org0x = c8 / 2;
    s->m_org0y = cc / 2;

    s = m_scroll;
    s->m_rect1.left = 0;
    s->m_rect1.top = 0;
    s->m_rect1.right = b.w - 1;
    s->m_rect1.bottom = b.h - 1;
    s->m_org1x = b.w / 2;
    s->m_org1y = b.h / 2;

    s = m_scroll;
    s->m_rect2.left = 0;
    s->m_rect2.top = 0;
    s->m_rect2.right = c.w - 1;
    s->m_rect2.bottom = c.h - 1;
    s->m_org2x = c.w / 2;
    s->m_org2y = c.h / 2;

    s = m_scroll;
    s->m_scrollX = -22222;
    s->m_scrollY = -22222;
}

// @early-stop
RVA(0x00163510, 0x156)
i32 CDDrawWorkerHost::ValidateTiles(char* errOut) {
    if (IsLoaded() == 0) {
        return 0;
    }

    char msg[0x80];
    i32 result = 1;
    for (i32 row = 0; row < m_gridH; row++) {
        for (i32 col = 0; col < m_gridW; col++) {
            i32 handle = m_tileGrid[m_colOffsets[row] + col];
            if (handle == -1 || static_cast<u32>(handle) == 0xeeeeeeee) {
                continue;
            }
            u32 setIdx = static_cast<u32>(handle) >> 16;
            CDDrawWorker* frame = FrameSetAt(setIdx);
            if (frame == NULL) {
                result = 0;
                if (errOut != NULL) {
                    sprintf(
                        msg,
                        "Plane %s: Bad map image set value (%i) at %i,%i\n",
                        m_name,
                        setIdx,
                        col,
                        row
                    );
                    strcat(errOut, msg);
                }
                continue;
            }
            i32 tile = handle & 0xffff;
            CImage* resolved = frame->GetAt(tile);
            if (resolved == NULL) {
                result = 0;
                if (errOut != NULL) {
                    sprintf(
                        msg,
                        "Plane %s: Bad map tile value (%i) at %i,%i\n",
                        m_name,
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

// @early-stop
RVA(0x00163670, 0x95)
void CDDrawWorkerHost::ResolveColorKey() {
    i32 format = OwnerMgr()->m_drawTarget->m_frontPair->m_bpp;
    if (format == 8) {
        return;
    }
    if (format != 0x10) {
        return;
    }

    i32 idx = m_bltFx.dwFillColor;
    if (idx < 0) {
        return;
    }
    if (idx > 0xff) {
        return;
    }

    CAniRecordBase2* owner = OwnerMgr()->m_workerMap->m_cachedWorker;
    if (owner == NULL) {
        return;
    }
    PALETTEENTRY* pal = owner->m_buf->m_cacheA;
    if (pal == NULL) {
        return;
    }

    m_bltFx.dwFillColor = static_cast<u16>(
        ((static_cast<u8>((static_cast<u8>(pal[idx].peRed) >> static_cast<u8>(g_rDown))) << g_rUp)
         | (static_cast<u8>((static_cast<u8>(pal[idx].peGreen) >> static_cast<u8>(g_gDown)))
            << g_gUp)
         | static_cast<u8>((static_cast<u8>(pal[idx].peBlue) >> static_cast<u8>(g_bDown))))
    );
}

RVA(0x00163710, 0x60)
i32 CDDrawWorkerHost::SerializeDispatch(CFileMemBase* s, SerialMode kind, LogicTypeId, i32) {
    if (!s) {
        return 0;
    }
    switch (kind) {
        case SERIAL_PRESAVE:
            return 1;
        case SERIAL_SAVE:
            if (!Save(s)) {
                return 0;
            }
            break;
        case SERIAL_POSTSAVE:
            return 1;
        case SERIAL_PRELOAD:
            return 1;
        case SERIAL_LOAD:
            if (!Load(s)) {
                return 0;
            }
            break;
        case SERIAL_POSTLOAD:
            return 1;
    }
    return 1;
}

RVA(0x00163780, 0x134)
i32 CDDrawWorkerHost::Save(CFileMemBase* s) {
    if (s == NULL) {
        return 0;
    }

    s->Write(&m_scaledX, sizeof(m_scaledX));
    s->Write(&m_scaledY, sizeof(m_scaledY));
    s->Write(&m_scaleX, sizeof(m_scaleX));
    s->Write(&m_scaleY, sizeof(m_scaleY));
    s->Write(&m_viewRect.left, 0x10);
    s->Write(&m_zBound, sizeof(m_zBound));
    s->Write(&m_snappedX, sizeof(m_snappedX));
    s->Write(&m_snappedY, sizeof(m_snappedY));
    s->Write(&m_movementXPercent, sizeof(m_movementXPercent));
    s->Write(&m_movementYPercent, sizeof(m_movementYPercent));

    i32 gridSize = m_gridW * m_gridH * 4;
    s->Write(&gridSize, sizeof(gridSize));
    s->Write(m_tileGrid, gridSize);

    char buf[0x80];
    memset(buf, 0, sizeof(buf));
    strcpy(buf, m_name);
    s->Write(buf, 0x80);
    return 1;
}

RVA(0x001638c0, 0x140)
i32 CDDrawWorkerHost::Load(CFileMemBase* s) {
    if (s == NULL) {
        return 0;
    }

    s->Read(&m_scaledX, sizeof(m_scaledX));
    s->Read(&m_scaledY, sizeof(m_scaledY));
    s->Read(&m_scaleX, sizeof(m_scaleX));
    s->Read(&m_scaleY, sizeof(m_scaleY));
    s->Read(&m_viewRect.left, 0x10);
    s->Read(&m_zBound, sizeof(m_zBound));
    s->Read(&m_snappedX, sizeof(m_snappedX));
    s->Read(&m_snappedY, sizeof(m_snappedY));
    s->Read(&m_movementXPercent, sizeof(m_movementXPercent));
    s->Read(&m_movementYPercent, sizeof(m_movementYPercent));

    i32 gridSize = 0;
    s->Read(&gridSize, sizeof(gridSize));
    if (gridSize != m_gridH * m_gridW * 4) {
        return 0;
    }
    s->Read(m_tileGrid, gridSize);

    char buf[0x80];
    s->Read(buf, 0x80);
    strcpy(m_name, buf);
    return 1;
}
