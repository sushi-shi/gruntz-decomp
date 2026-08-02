#include <rva.h>

#include <Gruntz/GameLevel.h>

#include <Mfc.h>

#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerHost.h>
#include <Gruntz/ImageSets.h>
#include <Gruntz/ParseSource.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>
#include <Io/FileMem.h>
#include <Io/FileStream.h>
#include <Wap32/Object.h>
#include <Wwd/WwdFile.h>

#include <stdlib.h>
#include <string.h>

static const i32 AXIS_UNSET = static_cast<i32>(0x80000000);

static inline void StampParamBlock(CGameLevel* o) {
    o->m_pairA[0] = 500;
    o->m_pairA[1] = 250;
    o->m_pairB[0] = 1000;
    o->m_pairB[1] = 1000;
    o->m_pairC[0] = 250;
    o->m_pairC[1] = 125;
    o->m_rectA.w = 1600;
    o->m_rectA.h = 1200;
    o->m_rectB.w = 2560;
    o->m_rectB.h = 1920;
    o->m_rectC.w = 768;
    o->m_rectC.h = 576;
}

// @early-stop
RVA(0x0015ccd0, 0x118)
CGameLevel::CGameLevel(CDDrawSurfaceMgr* owner, i32 id, i32 flags) : CLoadable(id, flags, owner) {

    m_maxStepX = 0x40;
    m_maxStepY = 0x40;
    m_pairA[1] = 250;
    m_pairC[0] = 250;
    m_pairB[0] = 1000;
    m_pairB[1] = 1000;

    m_planeCtx.left = LEVEL_COORD_UNSET;
    m_mainPlane = 0;
    m_mainIndex = -1;
    m_checksum = 0;
    m_pairA[0] = 500;
    m_pairC[1] = 125;
    m_rectA.w = 1600;
    m_rectA.h = 1200;
    m_rectB.w = 2560;
    m_rectB.h = 1920;
    m_rectC.w = 768;
    m_rectC.h = 576;
}

RVA(0x0015d280, 0x279)
i32 CGameLevel::LoadWwd(WwdHeader* hdr) {
    ReleaseChildren();

    if (hdr->wwdSignature > 0x5f4) {
        return 0;
    }

    m_header = *hdr;

    // Byte-forced view of packed WWD storage.
    char* block = reinterpret_cast<char*>(hdr);
    Bytef* ehAlloc = 0;

    u32* pflags = &hdr->flags;

    if (*pflags & 0x2) {
        u32 allocSize = hdr->mainBlockLength + hdr->wwdSignature + 0x40;
        Bytef* buf = static_cast<Bytef*>(operator new(allocSize));
        if (buf == 0) {
            return 0;
        }

        // Byte-forced view of packed WWD storage.
        block = reinterpret_cast<char*>(WwdFile_InflateMainBlock(hdr, buf, allocSize - 0x20));
        if (block == 0) {
            operator delete(buf);
            return 0;
        }
        ehAlloc = buf;
    }

    strcpy(m_levelName, hdr->levelName);
    m_flags = *pflags;
    m_checksum = hdr->checksum;

    i32 result = 0;

    char* cursor = block + hdr->planesOffset;
    u32 i = 0;

    if (hdr->numPlanes > 0) {
        do {
            // Byte-forced view of packed WWD storage.

            if (ReadPlane(reinterpret_cast<const WwdPlaneHeader*>(cursor), block, &m_planeCtx)
                == 0) {
                goto fail;
            }
            ++i;
            cursor += 0xa0;
        } while (i < hdr->numPlanes);
    }

    if (hdr->tileDescriptionsOffset != 0) {

        WwdTileDescTable* rec = // Byte-forced view of packed WWD storage.
            reinterpret_cast<WwdTileDescTable*>(block + hdr->tileDescriptionsOffset);
        char* elem = rec->m_descriptors;
        if (elem == 0) {
            result = -1;
        } else if (rec == 0) {
            result = -1;
        } else {
            i32 n = 0;
            i32 j = 0;
            while (static_cast<u32>(j) < rec->m_count) {
                CTileImageSet* set = ReadImageSet(elem);
                if (set == 0) {
                    result = -1;
                    goto check_result;
                }
                ++n;
                elem += set->GetStride();
                m_imageSets.SetAtGrow(j, static_cast<CObject*>(set));
                ++j;
            }
            result = n;
        }
    check_result:
        if (result < 0) {
            goto fail;
        }
    }

    {
        i32 startX = hdr->startX;
        i32 startY = hdr->startY;
        CDDrawWorkerHost* mp = m_mainPlane;
        if (mp->m_flags & 1) {
            mp->m_scaledX = static_cast<float>(startX);
            mp->m_scaledY = static_cast<float>(startY);
        } else {
            mp->m_scaledX = static_cast<float>(startX) * mp->m_scaleX;
            mp->m_scaledY = static_cast<float>(startY) * mp->m_scaleY;
        }
        mp->RecomputePlaneCoords();

        i32 ox = m_mainPlane->m_snappedX;
        i32 oy = m_mainPlane->m_snappedY;
        i32 i2 = 0;
        while (i2 < m_planes.GetSize()) {
            if (i2 != m_mainIndex) {
                CDDrawWorkerHost* p = static_cast<CDDrawWorkerHost*>(m_planes[i2]);
                if (p->m_flags & 1) {
                    p->m_scaledX = static_cast<float>(ox);
                    p->m_scaledY = static_cast<float>(oy);
                } else {
                    p->m_scaledX = static_cast<float>(ox) * p->m_scaleX;
                    p->m_scaledY = static_cast<float>(oy) * p->m_scaleY;
                }
                p->RecomputePlaneCoords();
            }
            ++i2;
        }
    }

    if (ehAlloc != 0) {
        operator delete(ehAlloc);
    }
    return 1;

fail:
    if (ehAlloc != 0) {
        operator delete(ehAlloc);
    }
    return 0;
}

RVA(0x00161190, 0x1f)
i32 CGameLevel::IsLoaded() {
    if (m_planeCtx.left == LEVEL_COORD_UNSET) {
        goto fail;
    }
    if (m_ownerCtx == 0) {
        goto fail;
    }
    if (m_id != -1) {
        return 1;
    }

fail:
    return 0;
}

// @early-stop
RVA(0x0015d030, 0x92)
i32 CGameLevel::SetCoordExtents(i32 w, i32 h) {
    m_planeCtx.left = 0;
    m_planeCtx.top = 0;
    m_planeCtx.right = w - 1;
    m_planeCtx.bottom = h - 1;
    StampParamBlock(this);
    return 1;
}

RVA(0x0015d500, 0x127)
i32 CGameLevel::LoadFromFile(const char* path) {
    CFile file;

    if (!file.Open(path, 0, 0)) {
        return 0;
    }

    void* buf = operator new(file.GetLength());
    if (!buf) {
        return 0;
    }

    file.Read(buf, file.GetLength());
    if (LoadWwd(static_cast<WwdHeader*>(buf)) == 0) {
        operator delete(buf);
        return 0;
    }
    operator delete(buf);
    return 1;
}

RVA(0x0015d630, 0x41)
i32 CGameLevel::LoadFromSource(CParseSource* arg) {
    char* handle = arg->BeginParse();
    if (handle == 0) {
        return 0;
    }

    // Byte-forced view of packed WWD storage.
    if (LoadWwd(reinterpret_cast<WwdHeader*>(handle)) == 0) {
        arg->EndParse();
        return 0;
    }
    arg->EndParse();
    return 1;
}

RVA_COMPGEN(0x001611c0, 0x1e, ??_GCGameLevel@@UAEPAXI@Z)

RVA(0x001611e0, 0x82)
CGameLevel::~CGameLevel() {
    Unload();
}

RVA(0x0015d1f0, 0x87)
void CGameLevel::Unload() {
    i32 i;
    for (i = 0; i < m_planes.GetSize(); i++) {
        CDDrawWorkerHost* child = static_cast<CDDrawWorkerHost*>(m_planes.GetData()[i]);
        if (child) {
            delete child;
        }
    }
    m_planes.SetSize(0, -1);
    for (i = 0; i < m_imageSets.GetSize(); i++) {
        CTileImageSet* child = static_cast<CTileImageSet*>(m_imageSets.GetData()[i]);
        if (child) {
            delete child;
        }
    }
    m_imageSets.SetSize(0, -1);
    m_planeCtx.left = LEVEL_COORD_UNSET;
    m_mainPlane = 0;
    m_mainIndex = -1;
    memset(&m_header, 0, 1524);
}

RVA(0x0015d680, 0x71)
void CGameLevel::ReleaseChildren() {
    i32 i;
    for (i = 0; i < m_planes.GetSize(); i++) {
        CDDrawWorkerHost* child = static_cast<CDDrawWorkerHost*>(m_planes.GetData()[i]);
        if (child) {
            delete child;
        }
    }
    m_planes.SetSize(0, -1);
    for (i = 0; i < m_imageSets.GetSize(); i++) {
        CTileImageSet* child = static_cast<CTileImageSet*>(m_imageSets.GetData()[i]);
        if (child) {
            delete child;
        }
    }
    m_imageSets.SetSize(0, -1);
    m_mainPlane = 0;
    m_mainIndex = -1;
}

RVA(0x0015cdf0, 0xb8)
i32 CGameLevel::LoadFileWithCoords(const char* path, LevelCoordRect* coords) {
    m_planeCtx = *coords;
    StampParamBlock(this);
    if (LoadFromFile(path) == 0) {
        Unload();
        return 0;
    }
    return 1;
}

RVA(0x0015ceb0, 0xb8)
i32 CGameLevel::LoadSourceWithCoords(CParseSource* src, LevelCoordRect* coords) {
    m_planeCtx = *coords;
    StampParamBlock(this);
    if (LoadFromSource(src) == 0) {
        Unload();
        return 0;
    }
    return 1;
}

RVA(0x0015cf70, 0xb8)
i32 CGameLevel::LoadWwdWithCoords(WwdHeader* hdr, LevelCoordRect* coords) {
    m_planeCtx = *coords;
    StampParamBlock(this);
    if (LoadWwd(hdr) == 0) {
        Unload();
        return 0;
    }
    return 1;
}

RVA(0x0015d0d0, 0x99)
i32 CGameLevel::SetCoords(LevelCoordRect* coords) {
    m_planeCtx = *coords;
    StampParamBlock(this);
    return 1;
}

RVA(0x0015d820, 0xa3)
CTileImageSet* CGameLevel::ReadImageSet(void* record) {
    if (record == 0) {
        return 0;
    }
    CTileImageSet* set;
    switch (*static_cast<i32*>(record)) {
        case 1:
            set = new CImageSet1;
            break;
        case 2:
            set = new CImageSet2;
            break;
        case 3:
            set = new CImageSet3;
            break;
        default:
            return 0;
    }

    if (set->Parse(record) == 0) {
        if (set != 0) {
            delete set;
        }
        return 0;
    }
    return set;
}

RVA(0x0015d8d0, 0xc3)
CDDrawWorkerHost*
CGameLevel::ReadPlane(const WwdPlaneHeader* planeData, const char* blockBase, void*) {
    CDDrawWorkerHost* plane = new CDDrawWorkerHost(OwnerMgr(), m_planes.GetSize(), 0);

    if (plane->Read(planeData, blockBase, &m_planeCtx) == 0) {
        if (plane) {
            delete plane;
        }
        return 0;
    }

    m_planes.SetAtGrow(m_planes.GetSize(), static_cast<CObject*>(plane));

    if (plane->m_flags & 1) {
        m_mainPlane = plane;
        m_mainIndex = m_planes.GetSize() - 1;
    }

    return plane;
}

RVA(0x0015d9a0, 0xdc)
CDDrawWorkerHost* CGameLevel::ReadObjectPlane(
    i32 w,
    i32 h,
    i32 tileW,
    i32 tileH,
    i32 depthX,
    i32 depthY,
    const char* name
) {
    CDDrawWorkerHost* plane = new CDDrawWorkerHost(OwnerMgr(), m_planes.GetSize(), 0);

    if (plane
            ->InitGeometry(w, h, tileW, tileH, depthX, depthY, &m_planeCtx, const_cast<char*>(name))
        == 0) {
        if (plane) {
            delete plane;
        }
        return 0;
    }

    m_planes.SetAtGrow(m_planes.GetSize(), static_cast<CObject*>(plane));

    if (plane->m_flags & 1) {
        m_mainPlane = plane;
        m_mainIndex = m_planes.GetSize() - 1;
    }

    return plane;
}

RVA(0x00161270, 0xb2)
i32 CGameLevel::AxisProbe(i32 coord, i32 limit) {

    i32 px = coord;
    if (px < 0) {
        px = 0;
    } else {
        if (px >= m_mainPlane->m_wrapW) {
            px = m_mainPlane->m_wrapW - 1;
        }
    }
    i32 py = limit;
    if (py < 0) {
        py = 0;
    } else {
        if (py >= m_mainPlane->m_wrapH) {
            py = m_mainPlane->m_wrapH - 1;
        }
    }
    CDDrawWorkerHost* pl = m_mainPlane;
    i32 qx = px >> pl->m_shiftX;
    i32 qy = py >> pl->m_shiftY;
    i32 col = qx;
    i32 subX = px - (qx << pl->m_shiftX);
    i32 idx = pl->m_colOffsets[qy] + col;
    i32 subY = py - (qy << pl->m_shiftY);
    i32 tile = pl->m_tileGrid[idx];
    if (tile == TILE_UNINIT || tile == TILE_CLEAR) {
        return 0;
    }
    CTileImageSet* set = static_cast<CTileImageSet*>(m_imageSets[tile & 0xffff]);
    return set->GetCollisionAt(subX, subY);
}

RVA(0x0006b330, 0x2a)
i32 CGameLevel::PointInBounds(const LevelCoordRect* r, i32 x, i32 y) {
    if (x < r->right && x >= r->left && y < r->bottom && y >= r->top) {
        return 1;
    }
    return 0;
}

RVA(0x00082600, 0x73)
i32 CGameLevel::LookupTile(i32 x, i32 y) {
    CDDrawWorkerHost* mp;
    if (x < 0) {
        x = 0;
    } else {
        mp = m_mainPlane;
        if (x >= mp->m_gridW) {
            x = mp->m_gridW - 1;
        }
    }
    if (y < 0) {
        y = 0;
    } else {
        mp = m_mainPlane;
        if (y >= mp->m_gridH) {
            y = mp->m_gridH - 1;
        }
    }
    mp = m_mainPlane;
    i32 tile = mp->m_tileGrid[mp->m_colOffsets[y] + x];
    if (tile == TILE_UNINIT || tile == TILE_CLEAR) {
        return 0;
    }
    CTileImageSet* set = static_cast<CTileImageSet*>(m_imageSets[tile & 0xffff]);
    return set->GetCollisionAt(0, 0);
}

RVA(0x00160ee0, 0xd)
void CGameLevel::MainPlaneNotify() {
    if (m_mainPlane != 0) {
        m_mainPlane->InitScrollRects();
    }
}

RVA(0x00160ef0, 0x42)
i32 CGameLevel::ValidateAllPlanes(char* errOut) {
    i32 ok = 1;
    if (errOut != 0) {
        *errOut = 0;
    }
    for (i32 i = 0; i < m_planes.GetSize(); i++) {
        if ((static_cast<CDDrawWorkerHost*>(m_planes[i]))->ValidateTiles(errOut) == 0) {
            ok = 0;
        }
    }
    return ok;
}

RVA(0x0015da80, 0x47)
void CGameLevel::BuildAllPlanes(LevelCoordRect* coords) {
    m_planeCtx = *coords;
    for (i32 i = 0; i < m_planes.GetSize(); i++) {
        (static_cast<CDDrawWorkerHost*>(m_planes[i]))->Build(coords);
    }
}

RVA(0x0015d700, 0x81)
i32 CGameLevel::SetExtentsAndBuildAll(i32 w, i32 h) {
    if (w <= 0) {
        return 0;
    }
    if (h <= 0) {
        return 0;
    }
    i32 maxX = w - 1;
    i32 maxY = h - 1;
    LevelCoordRect rect;
    rect.left = 0;
    rect.top = 0;
    rect.right = maxX;
    rect.bottom = maxY;
    m_planeCtx = rect;
    i32 i = 0;
    if (m_planes.GetSize() > 0) {
        do {
            (static_cast<CDDrawWorkerHost*>(m_planes.GetData()[i]))->Build(&rect);
            ++i;
        } while (i < m_planes.GetSize());
    }
    return 1;
}

RVA(0x0015dad0, 0x2c)
void CGameLevel::SyncToMainIndex(CDDrawSurfacePair* visitor) {
    i32 i = 0;
    if (m_mainIndex >= 0) {
        do {
            (static_cast<CDDrawWorkerHost*>(m_planes.GetData()[i]))->Draw(visitor);
            ++i;
        } while (i <= m_mainIndex);
    }
}

RVA(0x0015db00, 0x2e)
void CGameLevel::SyncAfterMainIndex(CDDrawSurfacePair* visitor) {
    i32 i = m_mainIndex + 1;
    if (i < m_planes.GetSize()) {
        do {
            (static_cast<CDDrawWorkerHost*>(m_planes.GetData()[i]))->Draw(visitor);
            ++i;
        } while (i < m_planes.GetSize());
    }
}

// @early-stop
RVA(0x0015de40, 0x164)
i32 CGameLevel::MoveToward(CGameObject* target, i32 destX, i32 destY, i32 moveFlags) {
    CGameObject* t = target;
    i32 limX = m_maxStepX;

    i32 sx = t->m_screenX;

    i32 dx = abs(sx - destX);
    if (dx <= limX) {
        i32 dy = abs(t->m_screenY - destY);
        if (dy <= m_maxStepY) {
            return DispatchMove(target, destX, destY, moveFlags);
        }
    }

    if (t->m_flags & 0x10) {
        return DispatchMove(target, destX, destY, moveFlags);
    }

    i32 kind = t->m_moveMode;
    if (kind == 7) {
        return DispatchMove(target, destX, destY, moveFlags);
    }

    i32 stepX = limX;
    i32 goalX = destX;
    if (sx > destX) {
        stepX = -stepX;
    }
    i32 stepY = m_maxStepY;
    if (t->m_screenY > destY) {
        stepY = -stepY;
    }

    i32 ok = 1;
    do {
        i32 nx = stepX + t->m_screenX;
        if (stepX > 0) {
            if (nx > goalX) {
                nx = goalX;
            }
        } else {
            if (nx < goalX) {
                nx = goalX;
            }
        }
        i32 ny = stepY + t->m_screenY;
        if (stepY > 0) {
            if (ny > destY) {
                ny = destY;
            }
        } else {
            if (ny < destY) {
                ny = destY;
            }
        }

        i32 flags = DispatchMove(target, nx, ny, moveFlags);

        if (t->m_moveMode != kind) {
            ok = 0;
        } else if ((flags & 0x10000) != 0) {
            ok = 0;
        } else if (t->m_screenX == goalX && t->m_screenY == destY) {
            ok = 0;
        } else if ((flags & 0x400000) != 0) {
            ok = 0;
        }
    } while (ok != 0);
    return ok;
}

RVA(0x0015dde0, 0x5c)
CDDrawWorkerHost* CGameLevel::FindPlaneByName(const char* name) {
    for (i32 i = 0; i < m_planes.GetSize(); i++) {
        CDDrawWorkerHost* p =
            (i >= 0 && i < m_planes.GetSize()) ? static_cast<CDDrawWorkerHost*>(m_planes[i]) : 0;
        if (_strcmpi(name, p->m_name) == 0) {
            return static_cast<CDDrawWorkerHost*>(p);
        }
    }
    return 0;
}

RVA(0x0015dc90, 0x141)
void CGameLevel::VisitVisible(CDDrawSurfacePair* visitor, CDDrawChildGroup* ctx) {

    CObList* chain = &ctx->m_list;

    if ((m_flags & 1) && chain != 0 && (m_planes.GetSize() > 0 ? m_planes.GetData()[0] : 0) != 0) {
        (static_cast<CDDrawWorkerHost*>((m_planes.GetSize() > 0 ? m_planes.GetData()[0] : 0)))
            ->Draw(visitor);
        POSITION pos = chain->GetHeadPosition();

        i32 i = 1;
        if (m_planes.GetSize() > i) {
            do {
                CDDrawWorkerHost* p = (i >= 0 && i < m_planes.GetSize())
                                          ? static_cast<CDDrawWorkerHost*>(m_planes.GetData()[i])
                                          : 0;
                i32 zBound = p->m_zBound;
                i32 blocked = 0;
                while (pos != 0 && blocked == 0) {
                    POSITION cur = pos;
                    CGameObject* pl = static_cast<CGameObject*>(chain->GetNext(pos));
                    if (pl->m_sortKey < zBound) {
                        pl->Render(visitor);
                    } else {
                        pos = cur;
                        blocked = 1;
                    }
                }

                (i >= 0 && i < m_planes.GetSize()
                     ? static_cast<CDDrawWorkerHost*>(m_planes.GetData()[i])
                     : 0)
                    ->Draw(visitor);
                ++i;
            } while (i < m_planes.GetSize());
        }

        while (pos != 0) {
            static_cast<CGameObject*>(chain->GetNext(pos))->Render(visitor);
        }
        return;
    }

    i32 idx = 0;
    if (m_mainIndex >= 0) {
        do {
            (static_cast<CDDrawWorkerHost*>(m_planes.GetData()[idx]))->Draw(visitor);
            ++idx;
        } while (idx <= m_mainIndex);
    }
    ctx->RenderChildren(visitor);
    i32 j = m_mainIndex + 1;
    if (j < m_planes.GetSize()) {
        do {
            (static_cast<CDDrawWorkerHost*>(m_planes.GetData()[j]))->Draw(visitor);
            ++j;
        } while (j < m_planes.GetSize());
    }
}

RVA(0x00160f40, 0x23)
void CGameLevel::NotifyAllPlanes() {
    for (i32 i = 0; i < m_planes.GetSize(); i++) {
        (static_cast<CDDrawWorkerHost*>(m_planes[i]))->ResolveColorKey();
    }
}

RVA(0x00160f70, 0x120)
i32 CGameLevel::EditDispatch(CFileMemBase* s, i32 mode, i32 typeId, i32 pObj) {
    if (s == 0) {
        return 0;
    }

    char buf[0x80];

    switch (mode) {
        case 3:
            break;
        case SERIAL_SAVE:
            memset(buf, 0, sizeof(buf));
            strcpy(buf, m_levelName);
            s->Write(buf, 0x80);
            break;
        case 5:
            break;
        case 6:
            break;
        case SERIAL_LOAD:
            s->Read(buf, 0x80);
            strcpy(m_levelName, buf);
            break;
        case 8:
            break;
        default:
            goto tail;
    }

tail:
    if (m_mainPlane == 0) {
        return 0;
    }
    return m_mainPlane->SerializeDispatch(s, mode, typeId, pObj) != 0 ? 1 : 0;
}

RVA(0x001610a0, 0x70)
i32 CGameLevel::SaveName(CFileMemBase* s) {
    if (s == 0) {
        return 0;
    }

    char buf[0x80];
    memset(buf, 0, sizeof(buf));
    strcpy(buf, m_levelName);
    s->Write(buf, 0x80);
    return 1;
}

RVA(0x00161110, 0x64)
i32 CGameLevel::LoadName(CFileMemBase* s) {
    if (s == 0) {
        return 0;
    }

    char buf[0x80];
    s->Read(buf, 0x80);
    strcpy(m_levelName, buf);
    return 1;
}

// @early-stop
RVA(0x0015dfb0, 0x180)
i32 CGameLevel::DispatchMove(CGameObject* target, i32 destX, i32 destY, i32 moveFlags) {
    if (m_flags & 4) {
        return ApplyMove(target, destX, destY, moveFlags);
    }

    CGameObject* s = target;
    i32 eax = 0;
    i32 kind = s->m_moveMode;
    i32 prevX = s->m_screenX;
    i32 prevY = s->m_screenY;

    switch (kind) {
        case 1:
        case 2:
        case 5:
            eax = MoveGrounded(s, destX, destY, moveFlags);
            break;
        case 3:
            eax = MoveRising(s, destX, destY, moveFlags);
            if (s->m_moveMode == 4) {
                eax |= 0x800000;
            }
            break;
        case 4:
            eax = MoveFalling(s, destX, destY, moveFlags);
            if (s->m_moveMode == 1) {
                eax |= 0x1000000;
            }
            break;
        case 8:
            if (destY < prevY) {
                eax = MoveRising(s, destX, destY, moveFlags);
                if (s->m_moveMode == 4) {
                    eax |= 0x800000;
                    s->m_moveMode = 8;
                }
            } else {
                eax = MoveFalling(s, destX, destY, moveFlags);
                if (s->m_moveMode == 1) {
                    eax |= 0x1000000;
                }
            }
            break;
        case 6:
            eax = MoveClimbing(s, destX, destY, moveFlags);
            break;
        case 7:
            s->m_screenX = destX;
            s->m_screenY = destY;
            break;
    }

    if (eax & 0x1820000) {
        eax |= 0x10000;
    }
    u32 f = s->m_flags;
    if (f & 0x400000) {
        eax |= 0x100000;
    }
    if (f & 0x10) {
        eax |= 0x200000;
    }
    if (s->m_screenX == prevX && s->m_screenY == prevY) {
        eax |= 0x400000;
    }
    return eax;
}

// @early-stop
RVA(0x0015e130, 0x1bb)
i32 CGameLevel::MoveGrounded(CGameObject* t, i32 destX, i32 destY, i32 moveFlags) {
    i32 result = 0;

    if (destX > t->m_screenX) {
        result = StepAxisLo(t, destX, destY, &destX, moveFlags);
    } else if (destX < t->m_screenX) {
        result = StepAxisHi(t, destX, destY, &destX, moveFlags);
    }

    if (destY < t->m_screenY) {
        destY = ResolveCeilingCollision(t, destX, destY, moveFlags);
    }

    i32 bracket;
    i32 mid;

    if (moveFlags & 1) {
        i32 col = destX;
        i32 limit = t->m_extent.top + destY - 1;
        if (AxisProbe(destX, limit) == kTileHard) {
            bracket = moveFlags & 0x10;
            if (bracket != 0) {
                i32 lo = col;
                i32 hi = col;
                mid = col;
                if (ClampSpan(col, limit, &lo, &hi) != 0) {
                    mid = (hi + lo) / 2;
                }
            } else {
                mid = destX;
            }
            goto rebracket;
        }
    } else if (moveFlags & 2) {
        i32 col = destX;
        i32 limit = t->m_extent.bottom + destY + 2;
        if (AxisProbe(destX, limit) == kTileHard) {
            bracket = moveFlags & 0x10;
            if (bracket != 0) {
                i32 lo = col;
                i32 hi = col;
                mid = col;
                if (ClampSpan(col, limit, &lo, &hi) != 0) {
                    mid = (hi + lo) / 2;
                }
            } else {
                mid = destX;
            }
            goto rebracket;
        }
    }

    if (t->m_flags & 0x10) {
        if (HoldMove(t, t->m_carrier, destX, destY, moveFlags) == 0) {
            t->m_moveMode = 4;
        }
    } else {
        destY = FreeMove(t, destX, destY, moveFlags);
    }
    goto commit;

rebracket:
    if (bracket != 0) {
        destX = mid;
    }
    t->m_moveMode = 6;

commit:
    t->m_screenX = destX;
    t->m_screenY = destY;
    return result;
}

// @early-stop
RVA(0x0015e2f0, 0x1b7)
i32 CGameLevel::MoveFalling(CGameObject* t, i32 destX, i32 destY, i32 moveFlags) {
    i32 savedDestX = destX;
    i32 result = 0;

    if (destX > t->m_screenX) {
        result = StepAxisLo(t, destX, destY, &destX, moveFlags);
    } else if (destX < t->m_screenX) {
        result = StepAxisHi(t, destX, destY, &destX, moveFlags);
    }

    if (moveFlags & 8) {
        i32 outY = destY;
        if (StepAxisAlt(t, destX, destY, &outY, moveFlags) != 0) {
            destY = outY;
        }
    }

    if (t->m_moveMode != 1) {
        destY = ResolveFloorCollision(t, destX, destY, moveFlags);
    }

    if (moveFlags & 1) {
        i32 limit = t->m_extent.top + destY - 1;
        if (AxisProbe(destX, limit) == kTileHard) {
            i32 mid = destX;
            i32 bracket = moveFlags & 0x10;
            if (bracket != 0) {
                i32 lo = destX;
                i32 hi = destX;
                if (ClampSpan(destX, limit, &lo, &hi) != 0) {
                    mid = (hi + lo) / 2;
                }
            }
            if (bracket != 0) {
                destX = mid;
            }
            t->m_moveMode = 6;
        }
    }

    if (t->m_moveMode == 1 && destX != savedDestX) {
        if (result & 0x20000) {
            result &= 0xfff1ffff;
            if (destX > t->m_screenX) {
                result |= StepAxisLo(t, destX, destY, &destX, moveFlags);
            } else if (destX < t->m_screenX) {
                result |= StepAxisHi(t, destX, destY, &destX, moveFlags);
            }
        }
    }

    t->m_screenX = destX;
    t->m_screenY = destY;
    return result;
}

// @early-stop
RVA(0x0015e4b0, 0xf7)
i32 CGameLevel::MoveRising(CGameObject* t, i32 destX, i32 destY, i32 moveFlags) {
    i32 result = 0;

    if (destX > t->m_screenX) {
        result = StepAxisLo(t, destX, destY, &destX, moveFlags);
    } else if (destX < t->m_screenX) {
        result = StepAxisHi(t, destX, destY, &destX, moveFlags);
    }

    destY = ResolveCeilingCollision(t, destX, destY, moveFlags);

    if (moveFlags & 1) {
        i32 limit = t->m_extent.top + destY - 1;
        if (AxisProbe(destX, limit) == kTileHard) {
            i32 mid = destX;
            if (moveFlags & 0x10) {
                i32 lo = destX;
                i32 hi = destX;
                if (ClampSpan(destX, limit, &lo, &hi) != 0) {
                    mid = (hi + lo) / 2;
                }
            }
            if (moveFlags & 0x10) {
                destX = mid;
            }
            t->m_moveMode = 6;
        }
    }

    t->m_screenX = destX;
    t->m_screenY = destY;
    return result;
}

// @early-stop
RVA(0x0015e5b0, 0x162)
i32 CGameLevel::MoveClimbing(CGameObject* t, i32 destX, i32 destY, i32 moveFlags) {
    i32 result = 0;
    i32 cursor;
    i32 coord = destX;

    if (t->m_screenY < destY) {
        cursor = ResolveFloorCollision(t, destX, destY, moveFlags);
        if (t->m_moveMode != 1) {
            i32 hi = t->m_extent.bottom + cursor + 1;
            i32 lo = t->m_extent.top + cursor - 1;
            if (AxisProbe(destX, lo) != kTileHard && AxisProbe(destX, hi) != kTileHard) {
                t->m_moveMode = 4;
            }
        }
    } else {
        cursor = ResolveCeilingCollision(t, destX, destY, moveFlags);
        i32 hi = t->m_extent.bottom + cursor + 1;
        i32 lo = t->m_extent.top + cursor - 1;
        if (AxisProbe(destX, lo) != kTileHard && AxisProbe(destX, hi) != kTileHard) {

            i32 probe;
            i32 top = t->m_extent.bottom + cursor + 1;
            if (SpanCheck(destX, top - cursor + t->m_screenY, top, &probe) != 0 && probe > cursor) {
                t->m_moveMode = 1;
                cursor = probe - t->m_extent.bottom - 1;
            }
        }
    }

    if (coord > t->m_screenX) {
        result = StepAxisLo(t, coord, cursor, &coord, moveFlags);
    } else if (coord < t->m_screenX) {
        result = StepAxisHi(t, coord, cursor, &coord, moveFlags);
    }

    t->m_screenX = coord;
    t->m_screenY = cursor;
    return result;
}

RVA(0x0015e720, 0x14c)
i32 CGameLevel::StepAxisLo(CGameObject* t, i32 destX, i32 destY, i32* outX, i32 moveFlags) {
    i32 mid = t->m_extent.right + destX;
    i32 lo = t->m_extent.top + destY;
    i32 hi = t->m_extent.bottom + destY;
    i32 cur = lo;

    while (cur <= hi) {
        i32 result;
        PROBE_TILE(this, mid, cur, result);
        if (result == kTileSoft) {
            *outX = t->m_screenX;
            return 0x60000;
        }
        if (cur == hi) {
            ++cur;
        } else {
            cur += t->m_strideY;
            if (cur > hi) {
                cur = hi;
            }
        }
    }

    *outX = destX;
    return 0;
}

RVA(0x0015e870, 0x14c)
i32 CGameLevel::StepAxisHi(CGameObject* t, i32 destX, i32 destY, i32* outX, i32 moveFlags) {
    i32 mid = t->m_extent.left + destX;
    i32 lo = t->m_extent.top + destY;
    i32 hi = t->m_extent.bottom + destY;
    i32 cur = lo;

    while (cur <= hi) {
        i32 result;
        PROBE_TILE(this, mid, cur, result);
        if (result == kTileSoft) {
            *outX = t->m_screenX;
            return 0xa0000;
        }
        if (cur == hi) {
            ++cur;
        } else {
            cur += t->m_strideY;
            if (cur > hi) {
                cur = hi;
            }
        }
    }

    *outX = destX;
    return 0;
}

// @early-stop
RVA(0x0015eb00, 0x2d2)
i32 CGameLevel::FreeMove(CGameObject* t, i32 destX, i32 destY, i32 moveFlags) {
    i32 mid = t->m_extent.right + destX;
    i32 cur = t->m_extent.left + destX;
    i32 hiY = t->m_extent.bottom + destY + 1;

    if (cur <= mid) {
        do {
            i32 result;
            PROBE_TILE(this, cur, hiY, result);
            if (result == kTileSoft || result == kTileSoft2) {

                i32 r2;
                PROBE_TILE(this, cur, hiY - 1, r2);
                if (r2 != kTileSoft) {
                    i32 r3;
                    PROBE_TILE(this, cur, hiY - 1, r3);
                    if (r3 != kTileSoft2) {
                        return destY;
                    }
                }
            } else if (t->m_moveMode != 6 && result == kTileHard) {
                if (AxisProbe(cur, hiY) == kTileHard) {
                    if (AxisProbe(cur, hiY - 1) != kTileHard) {
                        return destY;
                    }
                }
            }
            if (cur == mid) {
                ++cur;
            } else {
                cur += t->m_strideX;
            }
        } while (cur <= mid);
    }

    t->m_moveMode = 4;
    return destY;
}

// @early-stop
RVA(0x0015ede0, 0x2a7)
i32 CGameLevel::ResolveFloorCollision(CGameObject* t, i32 destX, i32 destY, i32 moveFlags) {
    i32 lo = t->m_extent.left + destX;
    i32 mid = t->m_extent.right + destX;
    i32 hiY = destY + t->m_extent.bottom + 1;

    i32 first;
    PROBE_TILE(this, destX, hiY, first);
    if (first == kTileSpecial) {
        t->m_flags |= 0x400000;
    }
    i32 base = destY - t->m_screenY;

    i32 cur = lo;
    if (cur <= mid) {
        do {
            i32 result;
            PROBE_TILE(this, cur, hiY, result);
            if (result == kTileSoft || result == kTileSoft2) {
                i32 floor = t->m_screenY + t->m_extent.bottom;
                if (hiY >= floor) {
                    i32 y = hiY;
                    do {
                        i32 g = AxisProbe(cur, y);
                        if (g != kTileSoft && g != kTileSoft2) {
                            t->m_moveMode = 1;
                            return y - t->m_extent.bottom;
                        }
                        --y;
                    } while (y >= floor);
                }
            } else if (t->m_moveMode != 6 && result == kTileHard) {
                i32 floor = hiY - base;
                if (hiY > floor) {
                    i32 y = hiY - 1;
                    if (y >= floor) {
                        do {
                            if (AxisProbe(cur, y) != kTileHard) {
                                t->m_moveMode = 1;
                                return (y + 1) - t->m_extent.bottom - 1;
                            }
                            --y;
                        } while (y >= floor);
                    }
                }
            }
            if (cur == mid) {
                ++cur;
            } else {
                cur += t->m_strideX;
            }
        } while (cur <= mid);
    }

    return destY;
}

// @early-stop
RVA(0x0015f1c0, 0x171)
i32 CGameLevel::ResolveCeilingCollision(CGameObject* t, i32 destX, i32 destY, i32 moveFlags) {
    i32 startCol = t->m_extent.left + destX;
    i32 mid = t->m_extent.right + destX;
    i32 ceil = destY + t->m_extent.top - 1;
    i32 cur = startCol;

    if (cur <= mid) {
        do {
            i32 result;
            PROBE_TILE(this, cur, ceil, result);
            if (result == kTileSoft) {
                i32 floor = t->m_screenY + t->m_extent.top - 1;
                if (ceil <= floor) {
                    i32 y = ceil;
                    do {

                        if (AxisProbe(startCol, y) != kTileSoft) {
                            t->m_moveMode = 4;
                            return y - t->m_extent.top;
                        }
                        ++y;
                    } while (y <= floor);
                }
            }
            if (cur == mid) {
                ++cur;
            } else {
                cur += t->m_strideX;
            }
        } while (cur <= mid);
    }

    return destY;
}

RVA(0x0015f8d0, 0x113)
i32 CGameLevel::SpanCheck(i32 a, i32 b, i32 c, i32* out) {
    if (b <= c) {
        return 0;
    }
    i32 cur = b - 1;
    while (cur >= c) {
        i32 result;
        PROBE_TILE(this, a, cur, result);
        if (result != kTileHard) {
            *out = cur + 1;
            return 1;
        }
        --cur;
    }

    return 0;
}

RVA(0x0015fdb0, 0x8a)
i32 CGameLevel::StepAxisAlt(CGameObject* t, i32 destX, i32 destY, i32* outY, i32 moveFlags) {
    if ((moveFlags & 8) == 0) {
        return 0;
    }

    CObList& chain = OwnerMgr()->m_childGroup->m_list;
    POSITION pos = chain.GetHeadPosition();
    while (pos != 0) {
        CGameObject* pl = static_cast<CGameObject*>(chain.GetNext(pos));
        if (pl->m_objectType == 0x80) {
            if (AltStepValidate(t, pl, destX, destY, outY, moveFlags) != 0) {
                t->m_moveMode = 1;
                t->m_carrier = pl;
                t->m_flags |= 0x10;
                return 1;
            }
        }
    }
    return 0;
}

// @early-stop
RVA(0x0015fe40, 0xd4)
i32 CGameLevel::AltStepValidate(
    CGameObject* t,
    CGameObject* p,
    i32 destX,
    i32 destY,
    i32* outY,
    i32 moveFlags
) {

    if (p->m_area.left == -1) {
        goto fail;
    }
    if (t->m_extent.left == -1) {
        goto fail;
    }
    {
        i32 sy = t->m_screenY;
        if (sy > destY) {
            goto fail;
        }

        i32 boxL = p->m_area.left + p->m_screenX;
        i32 boxR = p->m_area.right + p->m_screenX;
        i32 boxT = p->m_screenY + p->m_area.top;
        i32 tLoA = t->m_extent.left + destX;
        i32 tMid = t->m_extent.right + destX;
        i32 bottom = t->m_extent.bottom;
        i32 tHi = bottom + destY;
        i32 cmpHi = tHi - destY + sy;

        i32 over = p->m_deltaY;
        if (over > 0) {
            over = 0;
        }
        i32 ceil = boxT - over;
        if (cmpHi > ceil) {
            goto fail;
        }
        if (tMid < boxL) {
            goto fail;
        }
        if (tLoA > boxR) {
            goto fail;
        }

        i32 stand = boxT - 1;
        if (cmpHi == tHi) {
            if (tHi != stand) {
                goto fail;
            }
        } else {
            if (tHi < stand) {
                goto fail;
            }
        }

        *outY = boxT - bottom - 1;
        return 1;
    }
fail:
    return 0;
}

// @early-stop
RVA(0x0015ff20, 0xc0)
i32 CGameLevel::HoldMove(CGameObject* et, CGameObject* p, i32 destX, i32 destY, i32 moveFlags) {
    if (p == 0) {
        return 0;
    }
    if ((moveFlags & 8) == 0) {
        return 0;
    }
    if (p->m_objectType != 0x80) {
        return 0;
    }
    if (p->m_area.left == -1) {
        return 0;
    }
    if (et->m_extent.left == -1) {
        return 0;
    }

    i32 ox = p->m_screenX;
    i32 boxL = ox + p->m_area.left;
    i32 boxR = ox + p->m_area.right;
    i32 boxT = p->m_screenY + p->m_area.top;
    i32 tMid = et->m_extent.right + destX;
    i32 tLoA = et->m_extent.left + destX;

    i32 hi = et->m_extent.bottom + destY;
    if (tMid < boxL) {
        return 0;
    }
    if (tLoA > boxR) {
        return 0;
    }
    return hi == boxT - 1;
}

RVA(0x0015ffe0, 0x99)
i32 CGameLevel::ClampSpan(i32 x, i32 y, i32* outLo, i32* outHi) {
    if (x < 0) {
        x = 0;
    } else {
        if (x >= m_mainPlane->m_wrapW) {
            x = m_mainPlane->m_wrapW - 1;
        }
    }
    if (y < 0) {
        y = 0;
    } else {
        if (y >= m_mainPlane->m_wrapH) {
            y = m_mainPlane->m_wrapH - 1;
        }
    }
    CDDrawWorkerHost* pl = m_mainPlane;
    i32 qx = x >> pl->m_shiftX;
    i32 alignedX = qx << pl->m_shiftX;
    i32 qy = y >> pl->m_shiftY;
    i32 idx = pl->m_colOffsets[qy] + qx;
    i32 tile = pl->m_tileGrid[idx];
    if (tile == TILE_UNINIT || tile == TILE_CLEAR) {
        return 0;
    }
    CTileImageSet* set = static_cast<CTileImageSet*>(m_imageSets[tile & 0xffff]);
    *outLo = alignedX;
    *outHi = alignedX + set->m_width - 1;
    return 1;
}

// @early-stop
RVA(0x00160450, 0xd6)
i32 CGameLevel::ProbeHeadSoft(CGameObject* t, i32 dy) {
    i32 px = t->m_screenX;
    i32 py = t->m_screenY + t->m_extent.top + dy;
    i32 result;
    PROBE_TILE(this, px, py, result);
    return result == kTileSoft;
}

RVA(0x00160530, 0x125)
i32 CGameLevel::IsValidWwd(const char* name, void* headerBuf) {
    if (name == 0) {
        return 0;
    }
    if (headerBuf == 0) {
        return 0;
    }

    CFile stream;

    if (stream.Open(name, 0, 0) == 0) {
        return 0;
    }

    if (stream.Read(headerBuf, 0x5f4) != 0x5f4) {
        return 0;
    }

    if (*static_cast<u32*>(headerBuf) > 0x5f4) {
        return 0;
    }

    return 1;
}

RVA(0x00160660, 0x12b)
i32 CGameLevel::ReadWwdHeaderName(const char* name, void* nameOut) {
    WwdHeader header;

    if (name == 0) {
        return 0;
    }
    if (nameOut == 0) {
        return 0;
    }

    CFile stream;

    if (stream.Open(name, 0, 0) == 0) {
        return 0;
    }

    if (stream.Read(&header, 0x5f4) != 0x5f4) {
        return 0;
    }

    if (header.wwdSignature > 0x5f4) {
        return 0;
    }

    strcpy(static_cast<char*>(nameOut), header.levelName);
    return 1;
}

// @early-stop
RVA(0x00160790, 0xd2)
Bytef* __stdcall WwdFile_InflateMainBlock(WwdHeader* src, Bytef* dest, u32 destLen) {
    uLongf outLen;

    if (src == 0) {
        return 0;
    }
    if (dest == 0) {
        return 0;
    }

    if (src->wwdSignature > 0x5f4) {
        return 0;
    }
    if ((src->flags & 0x2) == 0) {
        return 0;
    }
    if (src->mainBlockLength == 0) {
        return 0;
    }
    if (src->mainBlockLength > destLen + src->wwdSignature) {
        return 0;
    }

    memcpy(dest, src, src->wwdSignature);
    outLen = static_cast<uLongf>((destLen - src->wwdSignature));
    if (uncompress(
            dest + src->wwdSignature,
            &outLen,

            // Byte-forced view of packed WWD storage.
            reinterpret_cast<Bytef*>(src) + src->wwdSignature,
            src->mainBlockLength
        )
        != 0) {
        return 0;
    }

    return outLen == src->mainBlockLength ? dest : 0;
}

RVA(0x00160870, 0x43)
i32 __stdcall WwdFile_CompressMainBlock(
    unsigned char* src,
    unsigned long srcLen,
    unsigned char* dest,
    unsigned long destCap
) {
    if (src == 0) {
        return 0;
    }
    if (dest == 0) {
        return 0;
    }
    unsigned long outLen = destCap;
    return WapUncompress(dest, &outLen, src, srcLen) == 0 ? static_cast<i32>(outLen) : 0;
}

RVA(0x001608c0, 0xc0)
i32 CGameLevel::ProbeFeetKind(CGameObject* t, i32 dx) {
    i32 px = t->m_screenX + dx;
    i32 py = t->m_extent.bottom + t->m_screenY;
    i32 result;
    PROBE_TILE(this, px, py, result);
    return result;
}

RVA(0x00160980, 0xc0)
i32 CGameLevel::ProbeColumn(CGameObject* t, i32 dx) {
    i32 px = t->m_screenX + dx;
    i32 py = t->m_extent.top + t->m_screenY;
    i32 result;
    PROBE_TILE(this, px, py, result);
    return result;
}

// @early-stop
RVA(0x00160a40, 0x201)
i32 CGameLevel::WalkColumnDown(CGameObject* t, i32 unused) {
    if (t->m_extent.left == AXIS_UNSET) {
        return 0;
    }
    if (m_mainPlane == 0) {
        return 0;
    }

    i32 px = t->m_screenX;
    i32 row = t->m_extent.bottom + t->m_screenY;
    i32 startRow = row;

    i32 result;
    PROBE_TILE(this, px, row, result);

    while (result != kTileSoft) {
        if (result == kTileSoft2 || result == kTileHard) {
            break;
        }
        ++row;
        if (row >= (m_mainPlane)->m_wrapH) {
            return 0;
        }
        PROBE_TILE(this, px, row, result);
    }

    i32 final = row - startRow - 1;
    t->m_screenY += final;
    return 1;
}

RVA(0x0015d790, 0x8b)
i32 CGameLevel::ReadImageSets(const u32* dir, char* cursor) {
    if (cursor == 0) {
        return -1;
    }
    if (dir == 0) {
        return -1;
    }
    i32 n = 0;
    for (i32 i = 0; static_cast<u32>(i) < dir[2]; i++) {
        CTileImageSet* set = ReadImageSet(cursor);
        if (set == 0) {
            return -1;
        }
        n++;
        cursor += set->GetStride();
        m_imageSets.SetAtGrow(i, static_cast<CObject*>(set));
    }
    return n;
}

RVA(0x0015db30, 0xae)
i32 CGameLevel::RemovePlane(i32 index) {
    CDDrawWorkerHost* p = (index >= 0 && index < m_planes.GetSize())
                              ? static_cast<CDDrawWorkerHost*>(m_planes[index])
                              : 0;
    if (p == 0) {
        return 0;
    }
    i32 wasMain = p->m_flags & 1;
    delete p;
    m_planes.RemoveAt(index, 1);
    if (wasMain) {
        i32 last = m_planes.GetSize() - 1;
        CDDrawWorkerHost* lp = (last >= 0 && last < m_planes.GetSize())
                                   ? static_cast<CDDrawWorkerHost*>(m_planes[last])
                                   : 0;
        if (lp != 0) {
            m_mainIndex = -1;
            m_mainPlane = 0;
            for (i32 i = 0; i < m_planes.GetSize(); i++) {
                (static_cast<CDDrawWorkerHost*>(m_planes[i]))->m_flags &= ~1;
            }
            m_mainIndex = last;
            m_mainPlane = lp;
            lp->m_flags |= 1;
        }
    }
    return 1;
}

RVA(0x0015dbe0, 0x70)
i32 CGameLevel::MovePlane(i32 from, i32 to) {
    if (from >= 0 && to < m_planes.GetSize()) {
        if (from == to) {
            return 1;
        }
        CDDrawWorkerHost* el =
            (from < m_planes.GetSize()) ? static_cast<CDDrawWorkerHost*>(m_planes[from]) : 0;
        if (el != 0) {
            m_planes.RemoveAt(from, 1);
            m_planes.InsertAt(to, static_cast<CObject*>(el), 1);
            if (el == m_mainPlane) {
                m_mainIndex = to;
            }
            return 1;
        }
    }
    return 0;
}

RVA(0x0015e9c0, 0x139)
i32 CGameLevel::ScanSpanTop(CGameObject* t, i32 x, i32 y, i32 unused) {
    i32 hiX = t->m_extent.right + x;
    i32 fixedY = t->m_extent.top + y;
    i32 col = t->m_extent.left + x;
    while (col <= hiX) {
        i32 result;
        PROBE_TILE(this, col, fixedY, result);
        if (result == kTileSoft) {
            return t->m_screenY;
        }
        if (col == hiX) {
            col++;
        } else {
            col += t->m_strideX;
        }
    }
    return y;
}

RVA(0x0015f090, 0x127)
i32 CGameLevel::SnapFloorDown(CGameObject* t, i32 x, i32 y, i32* out) {
    i32 limit = t->m_screenY + t->m_extent.bottom;
    for (i32 row = y; row >= limit; row--) {
        i32 result;
        PROBE_TILE(this, x, row, result);
        if (result != kTileSoft && result != kTileSoft2) {
            *out = row - t->m_extent.bottom;
            return 1;
        }
    }
    return 0;
}

RVA(0x0015f340, 0x124)
i32 CGameLevel::SnapCeilUp(CGameObject* t, i32 x, i32 y, i32* out) {
    i32 limit = t->m_screenY + t->m_extent.top - 1;
    for (i32 row = y; row <= limit; row++) {
        i32 result;
        PROBE_TILE(this, x, row, result);
        if (result != kTileSoft) {
            *out = row - t->m_extent.top;
            return 1;
        }
    }
    return 0;
}

RVA(0x0015f470, 0x193)
i32 CGameLevel::ProbeSpanHard(CGameObject* t, i32 x, i32 off) {
    i32 py2 = t->m_extent.bottom + off + 1;
    i32 py1 = t->m_extent.top + off - 1;
    i32 r1;
    PROBE_TILE(this, x, py1, r1);
    if (r1 == kTileHard) {
        return 1;
    }
    i32 r2;
    PROBE_TILE(this, x, py2, r2);
    return r2 == kTileHard;
}

RVA(0x0015f610, 0x191)
i32 CGameLevel::ResolveMoveDown(CGameObject* t, i32 x, i32 y, i32 flags) {
    y = ResolveCeilingCollision(t, x, y, flags);
    i32 headRow = t->m_extent.bottom + y + 1;
    i32 footRow = t->m_extent.top + y - 1;
    if (AxisProbe(x, footRow) == kTileHard) {
        goto done;
    }
    if (AxisProbe(x, headRow) == kTileHard) {
        goto done;
    }
    {

        i32 head2 = t->m_extent.bottom + y + 1;
        i32 b = head2 - y + t->m_screenY;
        if (b > head2) {
            i32 cur = b - 1;
            while (cur >= head2) {
                i32 result;
                PROBE_TILE(this, x, cur, result);
                if (result != kTileHard) {

                    ++cur;
                    if (cur > y) {
                        y = cur - t->m_extent.bottom - 1;
                        t->m_moveMode = 1;
                    }
                    goto done;
                }
                --cur;
            }
        }
    }
done:
    return y;
}

RVA(0x0015f7b0, 0x11f)
i32 CGameLevel::ResolveMoveUp(CGameObject* t, i32 x, i32 y, i32 flags) {
    y = ResolveFloorCollision(t, x, y, flags);
    if (t->m_moveMode == 1) {
        return y;
    }
    i32 headRow = t->m_extent.bottom + y + 1;
    i32 footRow = t->m_extent.top + y - 1;
    i32 result;
    PROBE_TILE(this, x, footRow, result);
    if (result != kTileHard) {
        if (AxisProbe(x, headRow) != kTileHard) {
            t->m_moveMode = 4;
        }
    }
    return y;
}

// @early-stop
RVA(0x0015f9f0, 0x11a)
i32 CGameLevel::StepGroundDown(CGameObject* t, i32 x, i32 y, i32* out, i32 flags) {
    i32 probeY = t->m_extent.bottom + y + 2;
    i32 result;
    PROBE_TILE(this, x, probeY, result);
    if (result != kTileHard) {
        return 0;
    }
    if (flags & 0x10) {
        i32 lo = x, hi = x;
        *out = x;
        if (ClampSpan(x, probeY, &lo, &hi) != 0) {
            *out = (lo + hi) / 2;
        }
    }
    return 1;
}

RVA(0x0015fb10, 0x119)
i32 CGameLevel::StepGroundUp(CGameObject* t, i32 x, i32 y, i32* out, i32 flags) {
    i32 probeY = t->m_extent.top + y - 1;
    i32 result;
    PROBE_TILE(this, x, probeY, result);
    if (result != kTileHard) {
        return 0;
    }
    if (flags & 0x10) {
        i32 lo = x, hi = x;
        *out = x;
        if (ClampSpan(x, probeY, &lo, &hi) != 0) {
            *out = (lo + hi) / 2;
        }
    }
    return 1;
}

RVA(0x0015fc30, 0x17f)
i32 CGameLevel::ProbeStepEdge(i32 x, i32 y) {
    i32 r1;
    PROBE_TILE(this, x, y, r1);
    if (r1 != kTileHard) {
        return 0;
    }
    i32 r2;
    PROBE_TILE(this, x, y - 1, r2);
    return r2 != kTileHard;
}

RVA(0x00160080, 0x187)
i32 CGameLevel::ProbeFootSoft(CGameObject* t, i32 dx) {
    i32 row = t->m_screenY + t->m_extent.bottom + 1;

    i32 r1;
    PROBE_TILE(this, dx + t->m_screenX, row, r1);
    if (r1 != kTileSoft) {
        i32 r2;
        PROBE_TILE(this, dx + t->m_screenX, row, r2);
        if (r2 != kTileSoft2) {
            return 0;
        }
    }
    return 1;
}

RVA(0x00160210, 0x234)
i32 CGameLevel::ProbeFootBlocked(CGameObject* t, i32 dx) {
    i32 row = t->m_screenY + t->m_extent.bottom + 1;

    i32 r1;
    PROBE_TILE(this, dx + t->m_screenX, row, r1);
    if (r1 != kTileSoft) {
        i32 r2;
        PROBE_TILE(this, dx + t->m_screenX, row, r2);
        if (r2 != kTileSoft2) {
            i32 r3;
            PROBE_TILE(this, dx + t->m_screenX, row, r3);
            if (r3 != kTileHard) {
                return 0;
            }
        }
    }
    return 1;
}

// @early-stop
RVA(0x00160c50, 0x289)
RVA_COMPGEN(0x00161350, 0x1e, ??_GCImageSet1@@UAEPAXI@Z)
RVA_COMPGEN(0x00161440, 0x1e, ??_GCImageSet2@@UAEPAXI@Z)
RVA_COMPGEN(0x001614e0, 0x1e, ??_GCImageSet3@@UAEPAXI@Z)
i32 CGameLevel::ScanRowSpan(i32 x0, i32 y, i32 x1, i32 step) {
    if (x1 > x0) {
        for (i32 col = x0; col <= x1; col += step) {
            i32 r;
            PROBE_TILE(this, col, y, r);
            if (r == kTileSoft) {
                return 0;
            }
        }
    } else {
        for (i32 col = x0; col >= x1; col -= step) {
            i32 r;
            PROBE_TILE(this, col, y, r);
            if (r == kTileSoft) {
                return 0;
            }
        }
    }
    i32 rf;
    PROBE_TILE(this, x1, y, rf);
    return rf != kTileSoft;
}

VTBL(CGameLevel, 0x001f0150);
VTBL(CImageSet1, 0x001f0198);
VTBL(CImageSet2, 0x001f01e0);
VTBL(CImageSet3, 0x001f0228);
