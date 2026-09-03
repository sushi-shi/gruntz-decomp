#include <rva.h>

#include <Gruntz/GameLevel.h>

#include <Mfc.h>

#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerHost.h>
#include <Enums.h>
#include <Gruntz/ImageSets.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>
#include <Io/FileMem.h>
#include <Io/FileStream.h>
#include <MakeRect.h>
#include <Pix16.h>
#include <Rez/RezArchiveEntry.h>
#include <Wap32/CoordUnset.h>
#include <Wap32/Object.h>
#include <Wap32/WapCompress.h>
#include <Wwd/MoveFlags.h>
#include <Wwd/MoveMode.h>
#include <Wwd/WwdFile.h>
#include <Wwd/WwdObjectType.h>

#include <stdlib.h>
#include <string.h>

#define DRAW_PLANES_THROUGH_MAIN(visitor, index)                                                   \
    i32 index = 0;                                                                                 \
    if (m_mainIndex >= 0) {                                                                        \
        do {                                                                                       \
            (static_cast<CDDrawWorkerHost*>(m_planes.GetData()[index]))->Draw(visitor);            \
            ++index;                                                                               \
        } while (index <= m_mainIndex);                                                            \
    }

#define DRAW_PLANES_AFTER_MAIN(visitor, index)                                                     \
    i32 index = m_mainIndex + 1;                                                                   \
    if (index < m_planes.GetSize()) {                                                              \
        do {                                                                                       \
            (static_cast<CDDrawWorkerHost*>(m_planes.GetData()[index]))->Draw(visitor);            \
            ++index;                                                                               \
        } while (index < m_planes.GetSize());                                                      \
    }

#define RESET_MAIN_PLANE_SELECTION(index)                                                          \
    m_mainIndex = -1;                                                                              \
    m_mainPlane = NULL;                                                                            \
    for (i32 index = 0; index < m_planes.GetSize(); index++) {                                     \
        static_cast<CDDrawWorkerHost*>(m_planes.GetData()[index])->m_flags &=                      \
            ~IDX(WWD_PLANE_FLAG_MAIN);                                                             \
    }

#define RELEASE_LEVEL_CHILDREN                                                                     \
    i32 i;                                                                                         \
    for (i = 0; i < m_planes.GetSize(); i++) {                                                     \
        CDDrawWorkerHost* child = static_cast<CDDrawWorkerHost*>(m_planes.GetData()[i]);           \
        if (child) {                                                                               \
            delete child;                                                                          \
        }                                                                                          \
    }                                                                                              \
    m_planes.SetSize(0, -1);                                                                       \
    for (i = 0; i < m_imageSets.GetSize(); i++) {                                                  \
        CTileImageSet* child = static_cast<CTileImageSet*>(m_imageSets.GetData()[i]);              \
        if (child) {                                                                               \
            delete child;                                                                          \
        }                                                                                          \
    }                                                                                              \
    m_imageSets.SetSize(0, -1);                                                                    \
    m_mainPlane = NULL;                                                                            \
    m_mainIndex = -1

RVA(0x0015ccd0, 0x118)
CGameLevel::CGameLevel(CDDrawSurfaceMgr* owner, i32 id, i32 flags)
    : CWapObj(owner, id, flags, CWapObj::NO_SEED) {

    m_maxStep.Set(0x40, 0x40);
    SetSpatialDefaults();

    m_viewportRect.left = COORD_UNSET;
    m_mainPlane = NULL;
    m_mainIndex = -1;
    m_checksum = 0;
}

RVA(0x0015cdf0, 0xb8)
i32 CGameLevel::LoadFileWithCoords(const char* path, LevelCoordRect* coords) {
    m_viewportRect = *coords;
    SetSpatialDefaults();
    if (LoadFromFile(path) == 0) {
        Unload();
        return 0;
    }
    return 1;
}

RVA(0x0015ceb0, 0xb8)
i32 CGameLevel::LoadSourceWithCoords(CRezItm* src, LevelCoordRect* coords) {
    m_viewportRect = *coords;
    SetSpatialDefaults();
    if (LoadFromSource(src) == 0) {
        Unload();
        return 0;
    }
    return 1;
}

RVA(0x0015cf70, 0xb8)
i32 CGameLevel::LoadWwdWithCoords(WwdHeader* hdr, LevelCoordRect* coords) {
    m_viewportRect = *coords;
    SetSpatialDefaults();
    if (LoadWwd(hdr) == 0) {
        Unload();
        return 0;
    }
    return 1;
}

static inline void SetLevelViewport(LevelCoordRect* rect, i32 w, i32 h) {
    *rect = MakeRect(0, 0, w - 1, h - 1);
}

RVA(0x0015d030, 0x92)
i32 CGameLevel::SetViewportSize(i32 w, i32 h) {
    SetLevelViewport(&m_viewportRect, w, h);
    SetSpatialDefaults();
    return 1;
}

RVA(0x0015d0d0, 0x99)
i32 CGameLevel::SetViewportRect(LevelCoordRect* coords) {
    m_viewportRect = *coords;
    SetSpatialDefaults();
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0015d170, 0x73)
void CGameLevel::ResetSpatialDefaults() {
    SetSpatialDefaults();
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
    m_viewportRect.left = COORD_UNSET;
    m_mainPlane = NULL;
    m_mainIndex = -1;
    memset(&m_header, 0, 1524);
}

// @early-stop
RVA(0x0015d280, 0x279)
i32 CGameLevel::LoadWwd(WwdHeader* hdr) {
    ReleaseChildren();

    WwdHeader* source = hdr;
    if (source->headerSize > sizeof(*source)) {
        return 0;
    }

    m_header = *source;

    // Byte-forced view of packed WWD storage.
    char* block = reinterpret_cast<char*>(source);
    Bytef* ehAlloc = NULL;

    u32* pflags = &source->flags;

    if (*pflags & WWD_LEVEL_FLAG_COMPRESSED) {
        u32 capacity = source->mainBlockLength + source->headerSize + 0x20;
        Bytef* buf = new Bytef[capacity + 0x20];
        if (buf == NULL) {
            return 0;
        }

        // Byte-forced view of packed WWD storage.
        hdr = reinterpret_cast<WwdHeader*>(InflateMainBlock(source, buf, capacity));
        if (hdr == NULL) {
            delete[] buf;
            return 0;
        }
        // Byte-forced view of packed WWD storage.
        block = reinterpret_cast<char*>(hdr);
        ehAlloc = buf;
    }

    strcpy(m_levelName, source->levelName);
    m_flags = *pflags;
    m_checksum = source->checksum;

    i32 result = 0;

    char* cursor = block + source->planesOffset;

    for (u32 i = 0; i < source->numPlanes; ++i) {
        // Byte-forced view of packed WWD storage.

        if (ReadPlane(reinterpret_cast<const WwdPlaneHeader*>(cursor), block, &m_viewportRect)
            == NULL) {
            goto fail;
        }
        cursor += 0xa0;
    }

    if (source->tileDescriptionsOffset > 0) {

        WwdTileDescTable* rec = // Byte-forced view of packed WWD storage.
            reinterpret_cast<WwdTileDescTable*>(block + source->tileDescriptionsOffset);
        char* elem = rec->m_descriptors;
        if (elem == NULL) {
            result = -1;
        } else if (rec == NULL) {
            result = -1;
        } else {
            i32 n = 0;
            i32 j = 0;
            while (static_cast<u32>(j) < rec->m_count) {
                RecordBytes<WwdTileImageRecord> record;
                record.m_chars = elem;
                CTileImageSet* set = ReadImageSet(record.m_rec);
                if (set == NULL) {
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
        Coord startPosition(source->startX, source->startY);
        CDDrawWorkerHost* mp = m_mainPlane;
        mp->SetScrollPosition(startPosition.m_x, startPosition.m_y);

        Coord scrollPosition = m_mainPlane->m_scrollPixel;
        i32 i2 = 0;
        while (i2 < m_planes.GetSize()) {
            if (i2 != m_mainIndex) {
                CDDrawWorkerHost* p = static_cast<CDDrawWorkerHost*>(m_planes[i2]);
                p->SetScrollPosition(scrollPosition.m_x, scrollPosition.m_y);
            }
            ++i2;
        }
    }

    if (ehAlloc != NULL) {
        delete[] ehAlloc;
    }
    return 1;

fail:
    if (ehAlloc != NULL) {
        delete[] ehAlloc;
    }
    return 0;
}

RVA(0x0015d500, 0x127)
i32 CGameLevel::LoadFromFile(const char* path) {
    CFile file;

    if (!file.Open(path, 0, NULL)) {
        return 0;
    }

    RecordBytes<WwdHeader> fileData;
    fileData.m_bytes = new u8[file.GetLength()];
    if (!fileData.m_bytes) {
        return 0;
    }

    file.Read(fileData.m_bytes, file.GetLength());
    if (LoadWwd(fileData.m_rec) == 0) {
        delete[] fileData.m_bytes;
        return 0;
    }
    delete[] fileData.m_bytes;
    return 1;
}

RVA(0x0015d630, 0x41)
i32 CGameLevel::LoadFromSource(CRezItm* source) {
    u8* handle = source->Load();
    if (handle == NULL) {
        return 0;
    }

    // Byte-forced view of packed WWD storage.
    if (LoadWwd(static_cast<WwdHeader*>(static_cast<void*>(handle))) == 0) {
        source->UnLoad();
        return 0;
    }
    source->UnLoad();
    return 1;
}

RVA(0x0015d680, 0x71)
void CGameLevel::ReleaseChildren() {
    RELEASE_LEVEL_CHILDREN;
}

RVA(0x0015d700, 0x81)
i32 CGameLevel::SetViewportSizeAndUpdatePlanes(i32 w, i32 h) {
    if (w <= 0) {
        return 0;
    }
    if (h <= 0) {
        return 0;
    }
    LevelCoordRect rect;
    SetRect(&rect, 0, 0, w - 1, h - 1);
    m_viewportRect = rect;
    i32 i = 0;
    if (m_planes.GetSize() > 0) {
        do {
            (static_cast<CDDrawWorkerHost*>(m_planes.GetData()[i]))->SetViewportRect(&rect);
            ++i;
        } while (i < m_planes.GetSize());
    }
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0015d790, 0x8b)
i32 CGameLevel::ReadImageSets(const u32* dir, char* cursor) {
    if (cursor == NULL) {
        return -1;
    }
    if (dir == NULL) {
        return -1;
    }
    i32 n = 0;
    for (i32 i = 0; static_cast<u32>(i) < dir[2]; i++) {
        RecordBytes<WwdTileImageRecord> record;
        record.m_chars = cursor;
        CTileImageSet* set = ReadImageSet(record.m_rec);
        if (set == NULL) {
            return -1;
        }
        n++;
        cursor += set->GetStride();
        m_imageSets.SetAtGrow(i, static_cast<CObject*>(set));
    }
    return n;
}

RVA(0x0015d820, 0xa3)
CTileImageSet* CGameLevel::ReadImageSet(WwdTileImageRecord* record) {
    if (record == NULL) {
        return NULL;
    }
    CTileImageSet* set;
    switch (record->m_kind) {
        case TILE_IMAGESET_UNIFORM:
            set = new CUniformTileImageSet;
            break;
        case TILE_IMAGESET_RECT:
            set = new CRectTileImageSet;
            break;
        case TILE_IMAGESET_PIXELS:
            set = new CPixelTileImageSet;
            break;
        default:
            return NULL;
    }

    if (set->Parse(record) == 0) {
        if (set != NULL) {
            delete set;
        }
        return NULL;
    }
    return set;
}

RVA(0x0015d8d0, 0xc3)
CDDrawWorkerHost*
CGameLevel::ReadPlane(const WwdPlaneHeader* planeData, const char* blockBase, RECT*) {
    CDDrawWorkerHost* plane = new CDDrawWorkerHost(OwnerMgr(), m_planes.GetSize(), 0);

    if (plane->Read(planeData, blockBase, &m_viewportRect) == 0) {
        if (plane) {
            delete plane;
        }
        return NULL;
    }

    m_planes.SetAtGrow(m_planes.GetSize(), static_cast<CObject*>(plane));

    if (HAS(static_cast<WwdPlaneFlags>(plane->m_flags), WWD_PLANE_FLAG_MAIN)) {
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

    if (plane->InitGeometry(
            w,
            h,
            tileW,
            tileH,
            depthX,
            depthY,
            &m_viewportRect,
            const_cast<char*>(name)
        )
        == 0) {
        if (plane) {
            delete plane;
        }
        return NULL;
    }

    m_planes.SetAtGrow(m_planes.GetSize(), static_cast<CObject*>(plane));

    if (HAS(static_cast<WwdPlaneFlags>(plane->m_flags), WWD_PLANE_FLAG_MAIN)) {
        m_mainPlane = plane;
        m_mainIndex = m_planes.GetSize() - 1;
    }

    return plane;
}

RVA(0x0015da80, 0x47)
void CGameLevel::UpdatePlaneViewports(LevelCoordRect* coords) {
    m_viewportRect = *coords;
    for (i32 i = 0; i < m_planes.GetSize(); i++) {
        (static_cast<CDDrawWorkerHost*>(m_planes[i]))->SetViewportRect(coords);
    }
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0015dad0, 0x2c)
void CGameLevel::SyncToMainIndex(CDDrawSurfacePair* visitor){DRAW_PLANES_THROUGH_MAIN(visitor, i)}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0015db00, 0x2e)
void CGameLevel::SyncAfterMainIndex(CDDrawSurfacePair* visitor){DRAW_PLANES_AFTER_MAIN(visitor, i)}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0015db30, 0xae)
i32 CGameLevel::RemovePlane(i32 index) {
    CDDrawWorkerHost* p = (index >= 0 && index < m_planes.GetSize())
                              ? static_cast<CDDrawWorkerHost*>(m_planes[index])
                              : NULL;
    if (p == NULL) {
        return 0;
    }
    b32 wasMain = HAS(static_cast<WwdPlaneFlags>(p->m_flags), WWD_PLANE_FLAG_MAIN);
    delete p;
    m_planes.RemoveAt(index, 1);
    if (wasMain) {
        i32 last = m_planes.GetSize() - 1;
        CDDrawWorkerHost* lp = (last >= 0 && last < m_planes.GetSize())
                                   ? static_cast<CDDrawWorkerHost*>(m_planes[last])
                                   : NULL;
        if (lp != NULL) {
            RESET_MAIN_PLANE_SELECTION(i)
            m_mainIndex = last;
            m_mainPlane = lp;
            lp->m_flags |= IDX(WWD_PLANE_FLAG_MAIN);
        }
    }
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0015dbe0, 0x70)
i32 CGameLevel::MovePlane(i32 from, i32 to) {
    if (from >= 0 && to < m_planes.GetSize()) {
        if (from == to) {
            return 1;
        }
        CDDrawWorkerHost* el =
            (from < m_planes.GetSize()) ? static_cast<CDDrawWorkerHost*>(m_planes[from]) : NULL;
        if (el != NULL) {
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

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0015dc50, 0x33)
void CGameLevel::ResetMainPlane(){RESET_MAIN_PLANE_SELECTION(i)}

RVA(0x0015dc90, 0x141)
void CGameLevel::VisitVisible(CDDrawSurfacePair* visitor, CDDrawChildGroup* ctx) {

    CObList* chain = &ctx->m_list;

    if ((m_flags & WWD_LEVEL_FLAG_USE_Z_COORDS) && chain != NULL
        && (m_planes.GetSize() > 0 ? m_planes.GetData()[0] : NULL) != NULL) {
        (static_cast<CDDrawWorkerHost*>((m_planes.GetSize() > 0 ? m_planes.GetData()[0] : NULL)))
            ->Draw(visitor);
        POSITION pos = chain->GetHeadPosition();

        i32 i = 1;
        if (m_planes.GetSize() > i) {
            do {
                CDDrawWorkerHost* p = (i >= 0 && i < m_planes.GetSize())
                                          ? static_cast<CDDrawWorkerHost*>(m_planes.GetData()[i])
                                          : NULL;
                i32 zBound = p->m_zCoord;
                i32 blocked = 0;
                while (pos != NULL && blocked == 0) {
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
                     : NULL)
                    ->Draw(visitor);
                ++i;
            } while (i < m_planes.GetSize());
        }

        while (pos != NULL) {
            static_cast<CGameObject*>(chain->GetNext(pos))->Render(visitor);
        }
        return;
    }

    DRAW_PLANES_THROUGH_MAIN(visitor, idx)
    ctx->RenderChildren(visitor);
    DRAW_PLANES_AFTER_MAIN(visitor, j)
}

RVA(0x0015dde0, 0x5c)
CDDrawWorkerHost* CGameLevel::FindPlaneByName(const char* name) {
    for (i32 i = 0; i < m_planes.GetSize(); i++) {
        CDDrawWorkerHost* p =
            (i >= 0 && i < m_planes.GetSize()) ? static_cast<CDDrawWorkerHost*>(m_planes[i]) : NULL;
        if (_strcmpi(name, p->m_planeName) == 0) {
            return static_cast<CDDrawWorkerHost*>(p);
        }
    }
    return NULL;
}

static inline i32 StepTowardGoal(i32 current, i32 step, i32 goal) {
    i32 next = step;
    next += current;
    if (step > 0) {
        if (next > goal) {
            next = goal;
        }
    } else if (next < goal) {
        next = goal;
    }
    return next;
}

static inline i32 SignedStepToward(i32 current, i32 goal, i32 magnitude) {
    return current > goal ? -magnitude : magnitude;
}

static inline b32 IsWithinStep(i32 current, i32 goal, i32 magnitude) {
    return abs(current - goal) <= magnitude;
}

RVA(0x0015de40, 0x164)
i32 CGameLevel::MoveToward(CGameObject* target, i32 destX, i32 destY, i32 moveFlags) {
    i32 step = m_maxStep.m_x;

    i32 flags;
    if (IsWithinStep(target->m_screenPosition.m_x, destX, step)
        && IsWithinStep(target->m_screenPosition.m_y, destY, m_maxStep.m_y)) {
        flags = DispatchMove(target, destX, destY, moveFlags);
    } else if (target->m_flags & IDX(WWD_GAME_OBJECT_FLAG_ON_CARRIER)) {
        flags = DispatchMove(target, destX, destY, moveFlags);
    } else {
        MoveMode kind = target->m_moveMode;
        if (kind == MOVE_DIRECT) {
            flags = DispatchMove(target, destX, destY, moveFlags);
        } else {
            b32 ok = true;
            Coord goal(destX, destY);
            destX = SignedStepToward(target->m_screenPosition.m_x, goal.m_x, step);
            step = SignedStepToward(target->m_screenPosition.m_y, goal.m_y, m_maxStep.m_y);
            do {
                Coord next(
                    StepTowardGoal(target->m_screenPosition.m_x, destX, goal.m_x),
                    StepTowardGoal(target->m_screenPosition.m_y, step, goal.m_y)
                );

                flags = DispatchMove(target, next.m_x, next.m_y, moveFlags);

                if (target->m_moveMode != kind) {
                    ok = false;
                } else if ((flags & IDX(MOVE_RESULT_TILE_COLLISION)) != 0) {
                    ok = false;
                } else if (target->m_screenPosition == goal) {
                    ok = false;
                } else if ((flags & IDX(MOVE_RESULT_NO_POSITION_CHANGE)) != 0) {
                    ok = false;
                }
            } while (ok != false);
        }
    }
    return flags;
}

RVA(0x0015dfb0, 0x180)
i32 CGameLevel::DispatchMove(CGameObject* target, i32 destX, i32 destY, i32 moveFlags) {
    if (m_flags & WWD_LEVEL_FLAG_DIRECT_MOVEMENT) {
        return ApplyMove(target, destX, destY, moveFlags);
    }

    i32 result = 0;
    MoveMode moveMode = target->m_moveMode;
    Coord previousPosition = target->ScreenPos();

    switch (moveMode) {
        case MOVE_GROUNDED:
        case MOVE_GROUNDED_2:
        case MOVE_GROUNDED_5:
            result = MoveGrounded(target, destX, destY, moveFlags);
            break;
        case MOVE_RISING:
            result = MoveRising(target, destX, destY, moveFlags);
            if (target->m_moveMode == MOVE_FALLING) {
                result |= IDX(MOVE_RESULT_TILE_TOP);
            }
            break;
        case MOVE_FALLING:
            result = MoveFalling(target, destX, destY, moveFlags);
            if (target->m_moveMode == MOVE_GROUNDED) {
                result |= IDX(MOVE_RESULT_TILE_BOTTOM);
            }
            break;
        case MOVE_AUTO_VERTICAL:
            if (destY < previousPosition.m_y) {
                result = MoveRising(target, destX, destY, moveFlags);
                if (target->m_moveMode == MOVE_FALLING) {
                    result |= IDX(MOVE_RESULT_TILE_TOP);
                    target->m_moveMode = MOVE_AUTO_VERTICAL;
                }
            } else {
                result = MoveFalling(target, destX, destY, moveFlags);
                if (target->m_moveMode == MOVE_GROUNDED) {
                    result |= IDX(MOVE_RESULT_TILE_BOTTOM);
                }
            }
            break;
        case MOVE_CLIMBING:
            result = MoveClimbing(target, destX, destY, moveFlags);
            break;
        case MOVE_DIRECT:
            target->SetScreenPos(destX, destY);
            break;
    }

    if (result & IDX(MOVE_RESULT_AXIS_BLOCKED | MOVE_RESULT_TILE_TOP | MOVE_RESULT_TILE_BOTTOM)) {
        result |= IDX(MOVE_RESULT_TILE_COLLISION);
    }
    u32 objectFlags = target->m_flags;
    if (objectFlags & IDX(WWD_GAME_OBJECT_FLAG_TOUCHED_DEATH_TILE)) {
        result |= IDX(MOVE_RESULT_DEATH_TILE);
    }
    if (objectFlags & IDX(WWD_GAME_OBJECT_FLAG_ON_CARRIER)) {
        result |= IDX(MOVE_RESULT_ON_CARRIER);
    }
    if (target->ScreenPos() == previousPosition) {
        result |= IDX(MOVE_RESULT_NO_POSITION_CHANGE);
    }
    return result;
}

// @early-stop
RVA(0x0015e130, 0x1bb)
i32 CGameLevel::MoveGrounded(CGameObject* t, i32 destX, i32 destY, i32 moveFlags) {
    i32 result = 0;

    if (destX > t->m_screenPosition.m_x) {
        result = StepAxisLo(t, destX, destY, &destX, moveFlags);
    } else if (destX < t->m_screenPosition.m_x) {
        result = StepAxisHi(t, destX, destY, &destX, moveFlags);
    }

    if (destY < t->m_screenPosition.m_y) {
        destY = ResolveCeilingCollision(t, destX, destY, moveFlags);
    }

    i32 bracket;
    i32 mid;

    if (moveFlags & IDX(MOVE_REQUEST_PROBE_TOP)) {
        i32 col = destX;
        i32 limit = t->m_extent.top + destY - 1;
        if (AxisProbe(destX, limit) == TILEKIND_CLIMB) {
            bracket = moveFlags & IDX(MOVE_REQUEST_CENTER_ON_CLIMB);
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
    } else if (moveFlags & IDX(MOVE_REQUEST_PROBE_BOTTOM)) {
        i32 col = destX;
        i32 limit = t->m_extent.bottom + destY + 2;
        if (AxisProbe(destX, limit) == TILEKIND_CLIMB) {
            bracket = moveFlags & IDX(MOVE_REQUEST_CENTER_ON_CLIMB);
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

    if (t->m_flags & IDX(WWD_GAME_OBJECT_FLAG_ON_CARRIER)) {
        if (HoldMove(t, t->m_carrier, destX, destY, moveFlags) == 0) {
            t->m_moveMode = MOVE_FALLING;
        }
    } else {
        destY = FreeMove(t, destX, destY, moveFlags);
    }
    goto commit;

rebracket:
    if (bracket != 0) {
        destX = mid;
    }
    t->m_moveMode = MOVE_CLIMBING;

commit:
    t->SetScreenPos(destX, destY);
    return result;
}

RVA(0x0015e2f0, 0x1b7)
i32 CGameLevel::MoveFalling(CGameObject* t, i32 destX, i32 destY, i32 moveFlags) {
    i32 savedDestX = destX;
    i32 result = 0;

    if (destX > t->m_screenPosition.m_x) {
        result = StepAxisLo(t, destX, destY, &destX, moveFlags);
    } else if (destX < t->m_screenPosition.m_x) {
        result = StepAxisHi(t, destX, destY, &destX, moveFlags);
    }

    if (moveFlags & IDX(MOVE_REQUEST_LAND_ON_PLATFORM)) {
        i32 outY;
        if (TryLandOnPlatform(t, destX, destY, &outY, moveFlags) != 0) {
            destY = outY;
        }
    }

    if (t->m_moveMode != MOVE_GROUNDED) {
        destY = ResolveFloorCollision(t, destX, destY, moveFlags);
    }

    if (moveFlags & IDX(MOVE_REQUEST_PROBE_TOP)) {
        i32 coord = destX;
        i32 limit = t->m_extent.top + destY - 1;
        if (AxisProbe(coord, limit) == TILEKIND_CLIMB) {
            if (moveFlags & IDX(MOVE_REQUEST_CENTER_ON_CLIMB)) {
                i32 lo = coord;
                i32 hi = coord;
                if (ClampSpan(coord, limit, &lo, &hi) != 0) {
                    coord = (hi + lo) / 2;
                }
            } else {
                coord = destX;
            }
            if (moveFlags & IDX(MOVE_REQUEST_CENTER_ON_CLIMB)) {
                destX = coord;
                t->m_moveMode = MOVE_CLIMBING;
            } else {
                t->m_moveMode = MOVE_CLIMBING;
            }
        }
    }

    if (t->m_moveMode == MOVE_GROUNDED && destX != savedDestX) {
        if (result & IDX(MOVE_RESULT_AXIS_BLOCKED)) {
            result &=
                ~IDX(MOVE_RESULT_AXIS_BLOCKED | MOVE_RESULT_TILE_RIGHT | MOVE_RESULT_TILE_LEFT);
            if (destX > t->m_screenPosition.m_x) {
                result |= StepAxisLo(t, destX, destY, &destX, moveFlags);
            } else if (destX < t->m_screenPosition.m_x) {
                result |= StepAxisHi(t, destX, destY, &destX, moveFlags);
            }
        }
    }

    t->SetScreenPos(destX, destY);
    return result;
}

RVA(0x0015e4b0, 0xf7)
i32 CGameLevel::MoveRising(CGameObject* t, i32 destX, i32 destY, i32 moveFlags) {
    i32 result = 0;

    if (destX > t->m_screenPosition.m_x) {
        result = StepAxisLo(t, destX, destY, &destX, moveFlags);
    } else if (destX < t->m_screenPosition.m_x) {
        result = StepAxisHi(t, destX, destY, &destX, moveFlags);
    }

    destY = ResolveCeilingCollision(t, destX, destY, moveFlags);

    if (moveFlags & IDX(MOVE_REQUEST_PROBE_TOP)) {
        i32 coord = destX;
        i32 limit = t->m_extent.top + destY - 1;
        if (AxisProbe(coord, limit) == TILEKIND_CLIMB) {
            if (moveFlags & IDX(MOVE_REQUEST_CENTER_ON_CLIMB)) {
                i32 lo = coord;
                i32 hi = coord;
                if (ClampSpan(coord, limit, &lo, &hi) != 0) {
                    coord = (hi + lo) / 2;
                }
            } else {
                coord = destX;
            }
            if (moveFlags & IDX(MOVE_REQUEST_CENTER_ON_CLIMB)) {
                destX = coord;
            }
            t->m_moveMode = MOVE_CLIMBING;
        }
    }

    t->SetScreenPos(destX, destY);
    return result;
}

// @early-stop
RVA(0x0015e5b0, 0x162)
i32 CGameLevel::MoveClimbing(CGameObject* t, i32 destX, i32 destY, i32 moveFlags) {
    i32 result = 0;
    i32 cursor;

    if (t->m_screenPosition.m_y < destY) {
        cursor = ResolveFloorCollision(t, destX, destY, moveFlags);
        if (t->m_moveMode != MOVE_GROUNDED) {
            i32 hi = t->m_extent.bottom + cursor + 1;
            i32 lo = t->m_extent.top + cursor - 1;
            if (AxisProbe(destX, lo) != TILEKIND_CLIMB && AxisProbe(destX, hi) != TILEKIND_CLIMB) {
                t->m_moveMode = MOVE_FALLING;
            }
        }
    } else {
        cursor = ResolveCeilingCollision(t, destX, destY, moveFlags);
        i32 hi = t->m_extent.bottom + cursor + 1;
        i32 lo = t->m_extent.top + cursor - 1;
        if (AxisProbe(destX, lo) != TILEKIND_CLIMB && AxisProbe(destX, hi) != TILEKIND_CLIMB) {

            i32 probe;
            i32 top = t->m_extent.bottom + cursor + 1;
            if (SpanCheck(destX, top - cursor + t->m_screenPosition.m_y, top, &probe) != 0
                && probe > cursor) {
                t->m_moveMode = MOVE_GROUNDED;
                cursor = probe - t->m_extent.bottom - 1;
            }
        }
    }

    i32 coord = destX;
    if (coord > t->m_screenPosition.m_x) {
        result = StepAxisLo(t, coord, cursor, &coord, moveFlags);
    } else if (coord < t->m_screenPosition.m_x) {
        result = StepAxisHi(t, coord, cursor, &coord, moveFlags);
    }

    t->SetScreenPos(coord, cursor);
    return result;
}

RVA(0x0015e720, 0x14c)
i32 CGameLevel::StepAxisLo(CGameObject* t, i32 destX, i32 destY, i32* outX, i32 moveFlags) {
    i32 mid = t->m_extent.right + destX;
    i32 lo = t->m_extent.top + destY;
    i32 hi = t->m_extent.bottom + destY;
    i32 cur = lo;

    while (cur <= hi) {
        TileCollisionKind result;
        result = ProbeTile(mid, cur);
        if (result == TILEKIND_SOLID) {
            *outX = t->m_screenPosition.m_x;
            return 0x60000;
        }
        if (cur == hi) {
            ++cur;
        } else {
            cur += t->m_stride.m_y;
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
        TileCollisionKind result;
        result = ProbeTile(mid, cur);
        if (result == TILEKIND_SOLID) {
            *outX = t->m_screenPosition.m_x;
            return 0xa0000;
        }
        if (cur == hi) {
            ++cur;
        } else {
            cur += t->m_stride.m_y;
            if (cur > hi) {
                cur = hi;
            }
        }
    }

    *outX = destX;
    return 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0015e9c0, 0x139)
i32 CGameLevel::ScanSpanTop(CGameObject* t, i32 x, i32 y, i32 unused) {
    i32 hiX = t->m_extent.right + x;
    i32 fixedY = t->m_extent.top + y;
    i32 col = t->m_extent.left + x;
    while (col <= hiX) {
        TileCollisionKind result;
        result = ProbeTile(col, fixedY);
        if (result == TILEKIND_SOLID) {
            return t->m_screenPosition.m_y;
        }
        if (col == hiX) {
            col++;
        } else {
            col += t->m_stride.m_x;
        }
    }
    return y;
}

// @early-stop
RVA(0x0015eb00, 0x2d2)
i32 CGameLevel::FreeMove(CGameObject* t, i32 destX, i32 destY, i32 moveFlags) {
    i32 mid = t->m_extent.right + destX;
    i32 cur = t->m_extent.left + destX;
    i32 hiY = t->m_extent.bottom + destY + 1;

    if (cur <= mid) {
        do {
            TileCollisionKind result;
            result = ProbeTile(cur, hiY);
            if (result == TILEKIND_SOLID || result == TILEKIND_GROUND) {

                TileCollisionKind r2;
                r2 = ProbeTile(cur, hiY - 1);
                if (r2 != TILEKIND_SOLID) {
                    TileCollisionKind r3;
                    r3 = ProbeTileViaHandle(cur, hiY - 1);
                    if (r3 != TILEKIND_GROUND) {
                        return destY;
                    }
                }
            } else if (t->m_moveMode != MOVE_CLIMBING && result == TILEKIND_CLIMB) {
                if (AxisProbe(cur, hiY) == TILEKIND_CLIMB) {
                    if (AxisProbe(cur, hiY - 1) != TILEKIND_CLIMB) {
                        return destY;
                    }
                }
            }
            if (cur == mid) {
                ++cur;
            } else {
                cur += t->m_stride.m_x;
            }
        } while (cur <= mid);
    }

    t->m_moveMode = MOVE_FALLING;
    return destY;
}

// @early-stop
RVA(0x0015ede0, 0x2a7)
i32 CGameLevel::ResolveFloorCollision(CGameObject* t, i32 destX, i32 destY, i32 moveFlags) {
    i32 lo = t->m_extent.left + destX;
    i32 mid = t->m_extent.right + destX;
    i32 hiY = destY + t->m_extent.bottom + 1;

    TileCollisionKind first;
    first = ProbeTile(destX, hiY);
    if (first == TILEKIND_DEATH) {
        t->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_TOUCHED_DEATH_TILE);
    }
    i32 base = destY - t->m_screenPosition.m_y;

    i32 cur = lo;
    if (cur <= mid) {
        do {
            TileCollisionKind result;
            result = ProbeTile(cur, hiY);
            if (result == TILEKIND_SOLID || result == TILEKIND_GROUND) {
                i32 floor = t->m_screenPosition.m_y + t->m_extent.bottom;
                if (hiY >= floor) {
                    i32 y = hiY;
                    do {
                        TileCollisionKind g = AxisProbe(cur, y);
                        if (g != TILEKIND_SOLID && g != TILEKIND_GROUND) {
                            t->m_moveMode = MOVE_GROUNDED;
                            return y - t->m_extent.bottom;
                        }
                        --y;
                    } while (y >= floor);
                }
            } else if (t->m_moveMode != MOVE_CLIMBING && result == TILEKIND_CLIMB) {
                i32 floor = hiY - base;
                i32 hi = hiY;
                if (hi > floor) {
                    i32 y = hi - 1;
                    if (y >= floor) {
                        do {
                            if (AxisProbe(cur, y) != TILEKIND_CLIMB) {
                                t->m_moveMode = MOVE_GROUNDED;
                                return hi - t->m_extent.bottom - 1;
                            }
                            hi = y;
                            --y;
                        } while (y >= floor);
                    }
                }
            }
            if (cur == mid) {
                ++cur;
            } else {
                cur += t->m_stride.m_x;
            }
        } while (cur <= mid);
    }

    return destY;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0015f090, 0x127)
i32 CGameLevel::SnapFloorDown(CGameObject* t, i32 x, i32 y, i32* out) {
    i32 limit = t->m_screenPosition.m_y + t->m_extent.bottom;
    for (i32 row = y; row >= limit; row--) {
        TileCollisionKind result;
        result = ProbeTile(x, row);
        if (result != TILEKIND_SOLID && result != TILEKIND_GROUND) {
            *out = row - t->m_extent.bottom;
            return 1;
        }
    }
    return 0;
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
            TileCollisionKind result;
            result = ProbeTile(cur, ceil);
            if (result == TILEKIND_SOLID) {
                i32 floor = t->m_screenPosition.m_y + t->m_extent.top - 1;
                if (ceil <= floor) {
                    i32 y = ceil;
                    do {

                        if (AxisProbe(startCol, y) != TILEKIND_SOLID) {
                            t->m_moveMode = MOVE_FALLING;
                            return y - t->m_extent.top;
                        }
                        ++y;
                    } while (y <= floor);
                }
            }
            if (cur == mid) {
                ++cur;
            } else {
                cur += t->m_stride.m_x;
            }
        } while (cur <= mid);
    }

    return destY;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0015f340, 0x124)
i32 CGameLevel::SnapCeilUp(CGameObject* t, i32 x, i32 y, i32* out) {
    i32 limit = t->m_screenPosition.m_y + t->m_extent.top - 1;
    for (i32 row = y; row <= limit; row++) {
        TileCollisionKind result;
        result = ProbeTile(x, row);
        if (result != TILEKIND_SOLID) {
            *out = row - t->m_extent.top;
            return 1;
        }
    }
    return 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0015f470, 0x193)
i32 CGameLevel::ProbeSpanHard(CGameObject* t, i32 x, i32 off) {
    i32 py2 = t->m_extent.bottom + off + 1;
    i32 py1 = t->m_extent.top + off - 1;
    TileCollisionKind r1;
    r1 = ProbeTile(x, py1);
    if (r1 == TILEKIND_CLIMB) {
        return 1;
    }
    TileCollisionKind r2;
    r2 = ProbeTile(x, py2);
    return r2 == TILEKIND_CLIMB;
}

// @early-stop
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0015f610, 0x191)
i32 CGameLevel::ResolveMoveDown(CGameObject* t, i32 x, i32 y, i32 flags) {
    y = ResolveCeilingCollision(t, x, y, flags);
    i32 headRow = t->m_extent.bottom + y + 1;
    i32 footRow = t->m_extent.top + y - 1;
    if (AxisProbe(x, footRow) == TILEKIND_CLIMB) {
        goto done;
    }
    if (AxisProbe(x, headRow) == TILEKIND_CLIMB) {
        goto done;
    }
    {

        i32 head2 = t->m_extent.bottom + y + 1;
        i32 b = head2 - y + t->m_screenPosition.m_y;
        if (b > head2) {
            i32 cur = b - 1;
            while (cur >= head2) {
                TileCollisionKind result;
                result = ProbeTile(x, cur);
                if (result != TILEKIND_CLIMB) {

                    ++cur;
                    if (cur > y) {
                        y = cur - t->m_extent.bottom - 1;
                        t->m_moveMode = MOVE_GROUNDED;
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

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0015f7b0, 0x11f)
i32 CGameLevel::ResolveMoveUp(CGameObject* t, i32 x, i32 y, i32 flags) {
    y = ResolveFloorCollision(t, x, y, flags);
    if (t->m_moveMode == MOVE_GROUNDED) {
        return y;
    }
    i32 headRow = t->m_extent.bottom + y + 1;
    i32 footRow = t->m_extent.top + y - 1;
    TileCollisionKind result;
    result = ProbeTile(x, footRow);
    if (result != TILEKIND_CLIMB) {
        if (AxisProbe(x, headRow) != TILEKIND_CLIMB) {
            t->m_moveMode = MOVE_FALLING;
        }
    }
    return y;
}

RVA(0x0015f8d0, 0x113)
i32 CGameLevel::SpanCheck(i32 x, i32 yEndExclusive, i32 yBegin, i32* outY) {
    if (yEndExclusive <= yBegin) {
        return 0;
    }
    i32 y = yEndExclusive - 1;
    while (y >= yBegin) {
        TileCollisionKind result;
        result = ProbeTile(x, y);
        if (result != TILEKIND_CLIMB) {
            *outY = y + 1;
            return 1;
        }
        --y;
    }

    return 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0015f9f0, 0x11a)
i32 CGameLevel::StepGroundDown(CGameObject* t, i32 x, i32 y, i32* out, i32 flags) {
    i32 footY = t->m_extent.bottom + y + 1;
    TileCollisionKind result;
    result = ProbeTile(x, footY + 1);
    if (result != TILEKIND_CLIMB) {
        return 0;
    }
    if (flags & IDX(MOVE_REQUEST_CENTER_ON_CLIMB)) {
        i32 lo = x, hi = x;
        *out = x;
        if (ClampSpan(x, footY + 1, &lo, &hi) != 0) {
            *out = (lo + hi) / 2;
        }
    }
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0015fb10, 0x119)
i32 CGameLevel::StepGroundUp(CGameObject* t, i32 x, i32 y, i32* out, i32 flags) {
    i32 probeY = t->m_extent.top + y - 1;
    TileCollisionKind result;
    result = ProbeTile(x, probeY);
    if (result != TILEKIND_CLIMB) {
        return 0;
    }
    if (flags & IDX(MOVE_REQUEST_CENTER_ON_CLIMB)) {
        i32 lo = x, hi = x;
        *out = x;
        if (ClampSpan(x, probeY, &lo, &hi) != 0) {
            *out = (lo + hi) / 2;
        }
    }
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0015fc30, 0x17f)
i32 CGameLevel::ProbeStepEdge(i32 x, i32 y) {
    TileCollisionKind r1;
    r1 = ProbeTile(x, y);
    if (r1 != TILEKIND_CLIMB) {
        return 0;
    }
    TileCollisionKind r2;
    r2 = ProbeTile(x, y - 1);
    return r2 != TILEKIND_CLIMB;
}

RVA(0x0015fdb0, 0x8a)
i32 CGameLevel::TryLandOnPlatform(
    CGameObject* object,
    i32 destX,
    i32 destY,
    i32* outLandingY,
    i32 moveFlags
) {
    if ((moveFlags & IDX(MOVE_REQUEST_LAND_ON_PLATFORM)) == 0) {
        return 0;
    }

    CDDrawChildGroup* children = OwnerMgr()->m_childGroup;
    POSITION pos = children->m_list.GetHeadPosition();
    while (pos != NULL) {
        CGameObject* platform = children->NextChild(pos);
        if (platform->m_objectType == WWD_OBJECT_TYPE_PLATFORM) {
            if (CanLandOnPlatform(object, platform, destX, destY, outLandingY, moveFlags) != 0) {
                object->m_moveMode = MOVE_GROUNDED;
                object->m_carrier = platform;
                object->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_ON_CARRIER);
                return 1;
            }
        }
    }
    return 0;
}

// @early-stop
RVA(0x0015fe40, 0xd4)
i32 CGameLevel::CanLandOnPlatform(
    CGameObject* object,
    CGameObject* platform,
    i32 destX,
    i32 destY,
    i32* outLandingY,
    i32 moveFlags
) {

    if (platform->m_area.left == -1) {
        goto fail;
    }
    if (object->m_extent.left == -1) {
        goto fail;
    }
    {
        i32 sy = object->m_screenPosition.m_y;
        if (sy > destY) {
            goto fail;
        }

        i32 boxL = platform->m_area.left + platform->m_screenPosition.m_x;
        i32 boxR = platform->m_area.right + platform->m_screenPosition.m_x;
        i32 boxT = platform->m_screenPosition.m_y + platform->m_area.top;
        i32 tLoA = object->m_extent.left + destX;
        i32 tMid = object->m_extent.right + destX;
        i32 bottom = object->m_extent.bottom;
        i32 tHi = bottom + destY;
        i32 cmpHi = tHi - destY + sy;

        i32 over = platform->m_delta.m_y;
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

        *outLandingY = boxT - bottom - 1;
        return 1;
    }
fail:
    return 0;
}

RVA(0x0015ff20, 0xc0)
i32 CGameLevel::HoldMove(CGameObject* et, CGameObject* p, i32 destX, i32 destY, i32 moveFlags) {
    if (p == NULL) {
        return 0;
    }
    if ((moveFlags & IDX(MOVE_REQUEST_LAND_ON_PLATFORM)) == 0) {
        return 0;
    }
    if (p->m_objectType != WWD_OBJECT_TYPE_PLATFORM) {
        return 0;
    }
    if (p->m_area.left == -1) {
        return 0;
    }
    if (et->m_extent.left == -1) {
        return 0;
    }

    i32 ox = p->m_screenPosition.m_x;
    i32 boxL = ox + p->m_area.left;
    i32 boxR = ox + p->m_area.right;
    i32 boxT = p->m_screenPosition.m_y + p->m_area.top;
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
    Coord pixel(x, y);
    pixel.Clamp(
        Coord(0, 0),
        Coord(m_mainPlane->m_planePixelSize.cx - 1, m_mainPlane->m_planePixelSize.cy - 1)
    );
    CDDrawWorkerHost* pl = m_mainPlane;
    Coord tilePosition(pixel.m_x >> pl->m_tileShift.m_x, pixel.m_y >> pl->m_tileShift.m_y);
    i32 alignedX = tilePosition.m_x << pl->m_tileShift.m_x;
    i32 idx = pl->m_tileRowOffsets[tilePosition.m_y] + tilePosition.m_x;
    i32 tileId = pl->m_tileHandles[idx];
    if (tileId == UNINIT_FILL || tileId == TILE_CLEAR) {
        return 0;
    }
    CTileImageSet* set =
        static_cast<CTileImageSet*>(m_imageSets[tileId & WWD_TILE_IMAGE_SET_INDEX_MASK]);
    *outLo = alignedX;
    *outHi = alignedX + set->m_width - 1;
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00160080, 0x187)
i32 CGameLevel::ProbeFootSoft(CGameObject* t, i32 dx) {
    i32 row = t->m_screenPosition.m_y + t->m_extent.bottom + 1;

    TileCollisionKind r1;
    r1 = ProbeTile(dx + t->m_screenPosition.m_x, row);
    if (r1 != TILEKIND_SOLID) {
        TileCollisionKind r2;
        r2 = ProbeTile(dx + t->m_screenPosition.m_x, row);
        if (r2 != TILEKIND_GROUND) {
            return 0;
        }
    }
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00160210, 0x234)
i32 CGameLevel::ProbeFootBlocked(CGameObject* t, i32 dx) {
    i32 row = t->m_screenPosition.m_y + t->m_extent.bottom + 1;

    TileCollisionKind r1;
    r1 = ProbeTile(dx + t->m_screenPosition.m_x, row);
    if (r1 != TILEKIND_SOLID) {
        TileCollisionKind r2;
        r2 = ProbeTile(dx + t->m_screenPosition.m_x, row);
        if (r2 != TILEKIND_GROUND) {
            TileCollisionKind r3;
            r3 = ProbeTile(dx + t->m_screenPosition.m_x, row);
            if (r3 != TILEKIND_CLIMB) {
                return 0;
            }
        }
    }
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00160450, 0xd6)
i32 CGameLevel::ProbeHeadSoft(CGameObject* t, i32 dy) {
    Coord probe = t->ScreenPos();
    probe.m_y += t->m_extent.top + dy;
    TileCollisionKind result;
    result = ProbeTile(probe.m_x, probe.m_y);
    return result == TILEKIND_SOLID;
}

RVA(0x00160530, 0x125)
i32 CGameLevel::IsValidWwd(const char* name, WwdHeader* headerBuf) {
    if (name == NULL) {
        return 0;
    }
    if (headerBuf == NULL) {
        return 0;
    }

    CFile stream;

    if (stream.Open(name, 0, NULL) == false) {
        return 0;
    }

    if (stream.Read(headerBuf, sizeof(WwdHeader)) != sizeof(WwdHeader)) {
        return 0;
    }

    if (headerBuf->headerSize > sizeof(WwdHeader)) {
        return 0;
    }

    return 1;
}

RVA(0x00160660, 0x12b)
i32 CGameLevel::ReadWwdHeaderName(const char* name, char* nameOut) {
    WwdHeader header;

    if (name == NULL) {
        return 0;
    }
    if (nameOut == NULL) {
        return 0;
    }

    CFile stream;

    if (stream.Open(name, 0, NULL) == false) {
        return 0;
    }

    if (stream.Read(&header, sizeof(header)) != sizeof(header)) {
        return 0;
    }

    if (header.headerSize > sizeof(header)) {
        return 0;
    }

    strcpy(nameOut, header.levelName);
    return 1;
}

// @early-stop
RVA(0x00160790, 0xd2)
Bytef* CGameLevel::InflateMainBlock(WwdHeader* src, Bytef* dest, u32 destLen) {
    uLongf outLen;

    if (src == NULL) {
        return NULL;
    }
    if (dest == NULL) {
        return NULL;
    }

    if (src->headerSize > sizeof(*src)) {
        return NULL;
    }
    if ((src->flags & WWD_LEVEL_FLAG_COMPRESSED) == 0) {
        return NULL;
    }
    if (src->mainBlockLength == 0) {
        return NULL;
    }
    if (src->mainBlockLength > destLen + src->headerSize) {
        return NULL;
    }

    memcpy(dest, src, src->headerSize);
    outLen = static_cast<uLongf>((destLen - src->headerSize));
    if (uncompress(
            dest + src->headerSize,
            &outLen,

            // Byte-forced view of packed WWD storage.
            reinterpret_cast<Bytef*>(src) + src->headerSize,
            src->mainBlockLength
        )
        != 0) {
        return NULL;
    }

    return outLen == src->mainBlockLength ? dest : NULL;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00160870, 0x43)
i32 __stdcall WwdFile_CompressMainBlock(
    unsigned char* src,
    unsigned long srcLen,
    unsigned char* dest,
    unsigned long destCap
) {
    if (src == NULL) {
        return 0;
    }
    if (dest == NULL) {
        return 0;
    }
    unsigned long outLen = destCap;
    return WapCompress(dest, &outLen, src, srcLen) == 0 ? static_cast<i32>(outLen) : 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001608c0, 0xc0)
TileCollisionKind CGameLevel::ProbeFeetKind(CGameObject* t, i32 dx) {
    Coord probe = t->ScreenPos();
    probe += Coord(dx, t->m_extent.bottom);
    TileCollisionKind result;
    result = ProbeTile(probe.m_x, probe.m_y);
    return result;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00160980, 0xc0)
TileCollisionKind CGameLevel::ProbeColumn(CGameObject* t, i32 dx) {
    Coord probe = t->ScreenPos();
    probe += Coord(dx, t->m_extent.top);
    TileCollisionKind result;
    result = ProbeTile(probe.m_x, probe.m_y);
    return result;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00160a40, 0x201)
i32 CGameLevel::WalkColumnDown(CGameObject* t, i32 unused) {
    if (t->m_extent.left == COORD_UNSET) {
        return 0;
    }
    if (m_mainPlane == NULL) {
        return 0;
    }

    i32 px = t->m_screenPosition.m_x;
    i32 row = t->m_extent.bottom + t->m_screenPosition.m_y;

    TileCollisionKind result;
    result = ProbeTile(px, row);

    i32 startRow = row;
    i32 wrapH = m_mainPlane->m_planePixelSize.cy;
    while (result != TILEKIND_SOLID) {
        if (result == TILEKIND_GROUND || result == TILEKIND_CLIMB) {
            break;
        }
        ++row;
        if (row >= wrapH) {
            return 0;
        }
        result = ProbeTile(px, row);
    }

    i32 final = row - startRow - 1;
    t->m_screenPosition.m_y += final;
    return 1;
}

// @early-stop
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00160c50, 0x289)
i32 CGameLevel::ScanRowSpan(i32 x0, i32 y, i32 x1, i32 step) {
    if (x1 > x0) {
        for (i32 col = x0; col <= x1; col += step) {
            TileCollisionKind r;
            r = ProbeTile(col, y);
            if (r == TILEKIND_SOLID) {
                return 0;
            }
        }
    } else {
        for (i32 col = x0; col >= x1; col -= step) {
            TileCollisionKind r;
            r = ProbeTile(col, y);
            if (r == TILEKIND_SOLID) {
                return 0;
            }
        }
    }
    TileCollisionKind rf;
    rf = ProbeTile(x1, y);
    return rf != TILEKIND_SOLID;
}

RVA(0x00160ee0, 0xd)
void CGameLevel::MainPlaneNotify() {
    if (m_mainPlane != NULL) {
        m_mainPlane->UpdateActiveRegionSizes();
    }
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00160ef0, 0x42)
i32 CGameLevel::ValidateAllPlanes(char* errOut) {
    b32 ok = true;
    if (errOut != NULL) {
        *errOut = 0;
    }
    for (i32 i = 0; i < m_planes.GetSize(); i++) {
        if ((static_cast<CDDrawWorkerHost*>(m_planes[i]))->ValidateTiles(errOut) == 0) {
            ok = false;
        }
    }
    return ok;
}

RVA(0x00160f40, 0x23)
void CGameLevel::NotifyAllPlanes() {
    for (i32 i = 0; i < m_planes.GetSize(); i++) {
        (static_cast<CDDrawWorkerHost*>(m_planes[i]))->ResolveColorKey();
    }
}

RVA(0x00160f70, 0x120)
i32 CGameLevel::SerializeDispatch(
    CFileMemBase* s,
    SerialMode mode,
    LogicTypeId typeId,
    i32 payload
) {
    if (s == NULL) {
        return 0;
    }

    char buf[SERIAL_NAME_LEN];

    switch (mode) {
        case SERIAL_PRESAVE:
            break;
        case SERIAL_SAVE:
            memset(buf, 0, sizeof(buf));
            strcpy(buf, m_levelName);
            s->Write(buf, SERIAL_NAME_LEN);
            break;
        case SERIAL_POSTSAVE:
            break;
        case SERIAL_PRELOAD:
            break;
        case SERIAL_LOAD:
            s->Read(buf, SERIAL_NAME_LEN);
            strcpy(m_levelName, buf);
            break;
        case SERIAL_POSTLOAD:
            break;
        default:
            goto tail;
    }

tail:
    if (m_mainPlane == NULL) {
        return 0;
    }
    return m_mainPlane->SerializeDispatch(s, mode, typeId, payload) != 0 ? 1 : 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00161090, 0xe)
i32 CGameLevel::CanSaveName(CFileMemBase* s) {
    return s != NULL;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001610a0, 0x70)
i32 CGameLevel::SaveName(CFileMemBase* s) {
    if (s == NULL) {
        return 0;
    }

    char buf[SERIAL_NAME_LEN];
    memset(buf, 0, sizeof(buf));
    strcpy(buf, m_levelName);
    s->Write(buf, SERIAL_NAME_LEN);
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00161110, 0x64)
i32 CGameLevel::LoadName(CFileMemBase* s) {
    if (s == NULL) {
        return 0;
    }

    char buf[SERIAL_NAME_LEN];
    s->Read(buf, SERIAL_NAME_LEN);
    strcpy(m_levelName, buf);
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00161180, 0xe)
i32 CGameLevel::CanLoadName(CFileMemBase* s) {
    return s != NULL;
}

RVA(0x00161190, 0x1f)
i32 CGameLevel::IsLoaded() {
    if (m_viewportRect.left == COORD_UNSET) {
        goto fail;
    }
    if (m_ownerCtx == NULL) {
        goto fail;
    }
    if (m_id != -1) {
        return 1;
    }

fail:
    return 0;
}

RVA_COMPGEN(0x001611c0, 0x1e, ??_GCGameLevel@@UAEPAXI@Z)

RVA(0x001611e0, 0x82)
CGameLevel::~CGameLevel() {
    Unload();
}

RVA(0x00161270, 0xb2)
TileCollisionKind CGameLevel::AxisProbe(i32 coord, i32 limit) {

    Coord pixel(coord, limit);
    pixel.Clamp(
        Coord(0, 0),
        Coord(m_mainPlane->m_planePixelSize.cx - 1, m_mainPlane->m_planePixelSize.cy - 1)
    );
    CDDrawWorkerHost* pl = m_mainPlane;
    Coord tilePosition(pixel.m_x >> pl->m_tileShift.m_x, pixel.m_y >> pl->m_tileShift.m_y);
    Coord tileOrigin(
        tilePosition.m_x << pl->m_tileShift.m_x,
        tilePosition.m_y << pl->m_tileShift.m_y
    );
    Coord sub = pixel - tileOrigin;
    i32 idx = pl->m_tileRowOffsets[tilePosition.m_y] + tilePosition.m_x;
    i32 tileId = pl->m_tileHandles[idx];
    if (tileId == UNINIT_FILL || tileId == TILE_CLEAR) {
        return TILEKIND_PASSABLE;
    }
    CTileImageSet* set =
        static_cast<CTileImageSet*>(m_imageSets[tileId & WWD_TILE_IMAGE_SET_INDEX_MASK]);
    return set->GetCollisionAt(sub.m_x, sub.m_y);
}

RVA_COMPGEN(0x00161350, 0x1e, ??_GCUniformTileImageSet@@UAEPAXI@Z)
RVA_COMPGEN(0x00161370, 0x7, ??1CUniformTileImageSet@@UAE@XZ)
RVA_COMPGEN(0x00161440, 0x1e, ??_GCRectTileImageSet@@UAEPAXI@Z)
RVA_COMPGEN(0x00161460, 0x7, ??1CRectTileImageSet@@UAE@XZ)
RVA_COMPGEN(0x001614e0, 0x1e, ??_GCPixelTileImageSet@@UAEPAXI@Z)
RVA_COMPGEN(0x00161500, 0x58, ??1CPixelTileImageSet@@UAE@XZ)
