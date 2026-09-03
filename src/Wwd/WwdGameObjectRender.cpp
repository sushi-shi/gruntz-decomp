#include <rva.h>

#include <Mfc.h>
#include <MfcWin.h>

#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDSurface.h>
#include <DDrawMgr/LogicRecord.h>
#include <DDrawMgr/LogicRecordRegistry.h>
#include <Enums.h>
#include <Gruntz/WwdGameObject.h>
#include <Ints.h>
#include <Wap32/CoordUnset.h>
#include <Wwd/WwdGameObjectFamily.h>

#include <ddraw.h>
#include <new>
#include <stdlib.h>
#include <string.h>

// @early-stop
RVA(0x001660f0, 0xd1)
void CWwdDotObject::Render(CDDrawSurfacePair* dst) {
    if (m_clip.left == COORD_UNSET) {
        if (m_screenPosition.m_x < 0 || m_screenPosition.m_y < 0
            || m_screenPosition.m_x >= dst->m_width || m_screenPosition.m_y >= dst->m_height) {
            m_dirty.m_armed = -1;
            return;
        }
    } else {
        if (m_screenPosition.m_x < m_clip.left || m_screenPosition.m_y < m_clip.top
            || m_screenPosition.m_x > m_clip.right || m_screenPosition.m_y > m_clip.bottom) {
            m_dirty.m_armed = -1;
            return;
        }
    }

    dst->m_surface->PutPixel(m_screenPosition.m_x, m_screenPosition.m_y, m_dotColor);
    m_dirty.m_lastPosition = m_screenPosition;
    m_dirty.m_size = CSize(1, 1);
    m_dirty.m_armed = 0;
}

RVA(0x001661d0, 0xc2)
void CWwdDotObject::BltDirty(CDDrawSurfacePair* dst, CDDrawSurfacePair* src) {

    m_shadow = m_dirty;
    if (m_shadow.m_armed != -1) {
        u8 pixel =
            src->m_surface->GetPixel(m_shadow.m_lastPosition.m_x, m_shadow.m_lastPosition.m_y);
        dst->m_surface->PutPixel(m_shadow.m_lastPosition.m_x, m_shadow.m_lastPosition.m_y, pixel);
        m_dirty.m_armed = -1;
    }
}

RVA(0x001662a0, 0x1fa)
void CWwdDotObject::BltDirtyEx(
    CDrawSubWorker* dst,
    CDDrawSurfacePair* src,
    CDDrawSurfacePair* restoreSrc
) {
    if (m_dirty.m_armed != -1 && m_shadow.m_armed != -1) {
        Coord delta = (m_dirty.m_lastPosition - m_shadow.m_lastPosition).GetAbs();
        CSize span(delta.m_x + 1, delta.m_y + 1);
        if (span.cx > 0x20 || span.cy > 0x20) {
            dst->BlitDirtyRect(src, m_dirty.m_lastPosition, m_dirty.m_size);
            dst->BlitDirtyRect(src, m_shadow.m_lastPosition, m_shadow.m_size);
        } else {
            Coord position = m_dirty.m_lastPosition;
            position.Min(m_shadow.m_lastPosition);
            dst->BlitDirtyRect(src, position, span);
        }
    } else if (m_dirty.m_armed != -1) {
        dst->BlitDirtyRect(src, m_dirty.m_lastPosition, m_dirty.m_size);
    } else if (m_shadow.m_armed != -1) {
        dst->BlitDirtyRect(src, m_shadow.m_lastPosition, m_shadow.m_size);
    }
}

RVA(0x001664a0, 0x133)
void CWwdDotObject::BltDirtyRegions(
    CDDrawSurfacePair* dst,
    CDDrawSurfacePair* src,
    CDDrawSurfacePair* restoreSrc
) {
    if (m_dirty.m_armed != -1 && m_shadow.m_armed != -1) {
        Coord delta = (m_dirty.m_lastPosition - m_shadow.m_lastPosition).GetAbs();
        CSize span(delta.m_x + 1, delta.m_y + 1);
        if (span.cx > 0x20 || span.cy > 0x20) {
            dst->BlitDirtyRect(src, m_dirty.m_lastPosition, m_dirty.m_size);
            dst->BlitDirtyRect(src, m_shadow.m_lastPosition, m_shadow.m_size);
        } else {
            Coord position = m_dirty.m_lastPosition;
            position.Min(m_shadow.m_lastPosition);
            dst->BlitDirtyRect(src, position, span);
        }
    } else if (m_dirty.m_armed != -1) {
        dst->BlitDirtyRect(src, m_dirty.m_lastPosition, m_dirty.m_size);
    } else if (m_shadow.m_armed != -1) {
        dst->BlitDirtyRect(src, m_shadow.m_lastPosition, m_shadow.m_size);
    }
}

#define CLEAR_WWD_GAME_OBJECT_CHILDREN                                                             \
    POSITION pos = m_children.GetHeadPosition();                                                   \
    while (pos != NULL) {                                                                          \
        CObject* child = m_children.GetNext(pos);                                                  \
        if (child != NULL) {                                                                       \
            delete child;                                                                          \
        }                                                                                          \
    }                                                                                              \
    m_children.RemoveAll()

RVA(0x001665e0, 0x55)
i32 CWwdGameObject::Setup(i32 x, i32 y, i32 sortKey, CLogicRecord* logicTemplate) {
    CLEAR_WWD_GAME_OBJECT_CHILDREN;
    return CGameObject::Setup(x, y, sortKey, logicTemplate) != 0;
}

// @early-stop
RVA(0x00166640, 0x13b)
CWwdGameObject* CWwdGameObject::CreateObject(
    int id,
    int x,
    int y,
    int sortKey,
    CLogicRecord* logicTemplate,
    int objectFlags
) {
    CWwdSpriteObject* result = new CWwdSpriteObject(OwnerMgr(), id, objectFlags, CWapObj::NO_SEED);
    if (result == NULL) {
        return NULL;
    }
    if (result->Setup(x, y, sortKey, logicTemplate) == 0) {
        delete result;
        return NULL;
    }
    POSITION node = m_children.AddTail(static_cast<CObject*>(result));
    if (node == NULL) {
        delete result;
        return NULL;
    }
    result->m_posCache = node;
    if (HAS(static_cast<WwdGameObjectFlags>(result->m_flags),
            WWD_GAME_OBJECT_FLAG_DISPATCH_ON_CREATE)) {
        result->m_logicRecord->m_dispatch(result);
    }
    return static_cast<CWwdGameObject*>(result);
}

static inline CLogicRecord* LookupLogicTemplate(CMapStringToOb& map, LPCTSTR name) {
    CObject* found = NULL;
    map.Lookup(name, found);
    return static_cast<CLogicRecord*>(found);
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00166780, 0x57)
CWwdGameObject*
CWwdGameObject::CreateNamed(int id, int x, int y, int sortKey, const char* name, int objectFlags) {
    CLogicRecord* logicTemplate =
        LookupLogicTemplate(OwnerMgr()->m_logicRegistry->m_templatesByName, name);
    if (logicTemplate == NULL) {
        return NULL;
    }
    return CreateObject(id, x, y, sortKey, logicTemplate, objectFlags);
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001667e0, 0x2f)
i32 CWwdGameObject::AddChild(CGameObject* child) {
    if (child == NULL) {
        return 0;
    }
    POSITION pos = m_children.AddTail(static_cast<CObject*>(child));
    if (pos == NULL) {
        return 0;
    }
    child->m_posCache = pos;
    return 1;
}

RVA(0x00166810, 0x32)
void CWwdGameObject::Clear() {
    CLEAR_WWD_GAME_OBJECT_CHILDREN;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00166850, 0x29)
i32 CWwdGameObject::RemoveChild(CGameObject* child) {
    if (child == NULL) {
        return 0;
    }
    POSITION pos = child->m_posCache;
    if (pos == NULL) {
        return 0;
    }
    m_children.RemoveAt(pos);
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00166880, 0x29)
i32 CWwdGameObject::WalkChildWorkers() {
    i32 count = 0;
    POSITION pos = m_children.GetHeadPosition();
    while (pos != NULL) {
        CGameObject* o = static_cast<CGameObject*>(m_children.GetNext(pos));
        o->m_logicRecord->m_dispatch(o);
        count++;
    }
    return count;
}

RVA(0x001668b0, 0x26)
void CWwdGameObject::Render(CDDrawSurfacePair* ctx) {
    POSITION pos = m_children.GetHeadPosition();
    while (pos != NULL) {
        static_cast<CGameObject*>(m_children.GetNext(pos))->Render(ctx);
    }
}
RVA(0x001668e0, 0x2d)
void CWwdGameObject::BltDirty(CDDrawSurfacePair* dst, CDDrawSurfacePair* src) {
    POSITION pos = m_children.GetHeadPosition();
    while (pos != NULL) {
        static_cast<CGameObject*>(m_children.GetNext(pos))->BltDirty(dst, src);
    }
}
RVA(0x00166910, 0x34)
void CWwdGameObject::BltDirtyEx(
    CDrawSubWorker* dst,
    CDDrawSurfacePair* src,
    CDDrawSurfacePair* restoreSrc
) {
    POSITION pos = m_children.GetHeadPosition();
    while (pos != NULL) {
        static_cast<CGameObject*>(m_children.GetNext(pos))->BltDirtyEx(dst, src, restoreSrc);
    }
}
RVA(0x00166950, 0x34)
void CWwdGameObject::BltDirtyRegions(
    CDDrawSurfacePair* dst,
    CDDrawSurfacePair* src,
    CDDrawSurfacePair* restoreSrc
) {
    POSITION pos = m_children.GetHeadPosition();
    while (pos != NULL) {
        static_cast<CGameObject*>(m_children.GetNext(pos))->BltDirtyRegions(dst, src, restoreSrc);
    }
}
