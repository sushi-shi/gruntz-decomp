#include <StdAfx.h>

#include <Ints.h>

#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDSurface.h>
#include <DDrawMgr/DrawSubWorkerInline.h>
#include <DDrawMgr/LogicRecord.h>
#include <DDrawMgr/LogicRecordRegistry.h>
#include <Enums.h>
#include <Gruntz/WwdGameObject.h>
#include <Ints.h>
#include <Utils/MapTyped.h>
#include <Wap32/CoordUnset.h>
#include <Wwd/WwdGameObjectFamily.h>

#include <ddraw.h>
#include <new>
#include <stdlib.h>
#include <string.h>

void CWwdDotObject::Render(CDDrawSurfacePair* dst) {
    if (m_clip.left == COORD_UNSET) {
        if (m_screenX < 0 || m_screenY < 0 || m_screenX >= dst->GetWidth()
            || m_screenY >= dst->GetHeight()) {
            m_dirty.m_armed = -1;
            return;
        }
    } else {
        if (m_screenX < m_clip.left || m_screenY < m_clip.top || m_screenX > m_clip.right
            || m_screenY > m_clip.bottom) {
            m_dirty.m_armed = -1;
            return;
        }
    }

    dst->GetSurface()->PutPixel(m_screenX, m_screenY, m_dotColor);
    m_dirty.m_position.x = m_screenX;
    m_dirty.m_position.y = m_screenY;
    m_dirty.m_size.cx = 1;
    m_dirty.m_size.cy = 1;
    m_dirty.m_armed = 0;
}

void CWwdDotObject::BltDirty(CDDrawSurfacePair* dst, CDDrawSurfacePair* src) {

    m_shadow = m_dirty;
    if (m_shadow.m_armed != -1) {
        u8 pixel = src->GetSurface()->GetPixel(m_shadow.m_position.x, m_shadow.m_position.y);
        dst->GetSurface()->PutPixel(m_shadow.m_position.x, m_shadow.m_position.y, pixel);
        m_dirty.m_armed = -1;
    }
}

void CWwdDotObject::BltDirtyEx(
    CDrawSubWorker* dst,
    CDDrawSurfacePair* src,
    CDDrawSurfacePair* restoreSrc
) {
    if (m_dirty.m_armed != -1 && m_shadow.m_armed != -1) {
        i32 dx = abs(m_dirty.m_position.x - m_shadow.m_position.x) + 1;
        i32 dy = abs(m_dirty.m_position.y - m_shadow.m_position.y) + 1;
        if (dx > 0x20 || dy > 0x20) {
            dst->BlitDirtyRect(src, m_dirty.m_position, m_dirty.m_size);
            dst->BlitDirtyRect(src, m_shadow.m_position, m_shadow.m_size);
        } else {
            i32 left = min(m_dirty.m_position.x, m_shadow.m_position.x);
            i32 top = min(m_dirty.m_position.y, m_shadow.m_position.y);
            CPoint pos(left, top);
            CSize size(dx, dy);
            dst->BlitDirtyRect(src, pos, size);
        }
    } else if (m_dirty.m_armed != -1) {
        dst->BlitDirtyRect(src, m_dirty.m_position, m_dirty.m_size);
    } else if (m_shadow.m_armed != -1) {
        dst->BlitDirtyRect(src, m_shadow.m_position, m_shadow.m_size);
    }
}

void CWwdDotObject::BltDirtyRegions(
    CDDrawSurfacePair* dst,
    CDDrawSurfacePair* src,
    CDDrawSurfacePair* restoreSrc
) {
    if (m_dirty.m_armed != -1 && m_shadow.m_armed != -1) {
        const POINT& dirtyPos = m_dirty.m_position;
        const POINT& shadowPos = m_shadow.m_position;
        i32 dx = abs(m_dirty.m_position.x - m_shadow.m_position.x) + 1;
        i32 dy = abs(m_dirty.m_position.y - m_shadow.m_position.y) + 1;
        if (dx > 0x20 || dy > 0x20) {
            dst->BlitDirtyRect(src, dirtyPos, m_dirty.m_size);
            dst->BlitDirtyRect(src, shadowPos, m_shadow.m_size);
        } else {
            i32 left = min(m_dirty.m_position.x, m_shadow.m_position.x);
            i32 top = min(m_dirty.m_position.y, m_shadow.m_position.y);
            CPoint pos(left, top);
            CSize size(dx, dy);
            dst->BlitDirtyRect(src, pos, size);
        }
    } else if (m_dirty.m_armed != -1) {
        dst->BlitDirtyRect(src, m_dirty.m_position, m_dirty.m_size);
    } else if (m_shadow.m_armed != -1) {
        dst->BlitDirtyRect(src, m_shadow.m_position, m_shadow.m_size);
    }
}

i32 CWwdGameObject::Setup(i32 x, i32 y, i32 sortKey, CLogicRecord* logicTemplate) {
    CLEAR_WWD_GAME_OBJECT_CHILDREN;
    return CGameObject::Setup(x, y, sortKey, logicTemplate) != 0;
}

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
    std::list<CGameObject*>::iterator node = m_children.insert(m_children.end(), (result));
    if (node == m_children.end()) {
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

CWwdGameObject*
CWwdGameObject::CreateNamed(int id, int x, int y, int sortKey, const char* name, int objectFlags) {
    CLogicRecord* logicTemplate =
        MapFind<CLogicRecord>(OwnerMgr()->m_logicRegistry->m_templatesByName, name);
    if (logicTemplate == NULL) {
        return NULL;
    }
    return CreateObject(id, x, y, sortKey, logicTemplate, objectFlags);
}

i32 CWwdGameObject::AddChild(CGameObject* child) {
    if (child == NULL) {
        return 0;
    }
    std::list<CGameObject*>::iterator pos = m_children.insert(m_children.end(), (child));
    if (pos == m_children.end()) {
        return 0;
    }
    child->m_posCache = pos;
    return 1;
}

void CWwdGameObject::Clear() {
    CLEAR_WWD_GAME_OBJECT_CHILDREN;
}

i32 CWwdGameObject::RemoveChild(CGameObject* child) {
    if (child == NULL) {
        return 0;
    }
    std::list<CGameObject*>::iterator pos = std::find(m_children.begin(), m_children.end(), child);
    if (pos == m_children.end()) {
        return 0;
    }
    m_children.erase(pos);
    child->m_posCache = m_children.end();
    return 1;
}

i32 CWwdGameObject::WalkChildWorkers() {
    i32 count = 0;
    std::list<CGameObject*>::iterator pos = m_children.begin();
    while (pos != m_children.end()) {
        CGameObject* o = static_cast<CGameObject*>(*(pos++));
        o->m_logicRecord->m_dispatch(o);
        count++;
    }
    return count;
}

void CWwdGameObject::Render(CDDrawSurfacePair* ctx) {
    std::list<CGameObject*>::iterator pos = m_children.begin();
    while (pos != m_children.end()) {
        static_cast<CGameObject*>(*(pos++))->Render(ctx);
    }
}

void CWwdGameObject::BltDirty(CDDrawSurfacePair* dst, CDDrawSurfacePair* src) {
    std::list<CGameObject*>::iterator pos = m_children.begin();
    while (pos != m_children.end()) {
        static_cast<CGameObject*>(*(pos++))->BltDirty(dst, src);
    }
}

void CWwdGameObject::BltDirtyEx(
    CDrawSubWorker* dst,
    CDDrawSurfacePair* src,
    CDDrawSurfacePair* restoreSrc
) {
    std::list<CGameObject*>::iterator pos = m_children.begin();
    while (pos != m_children.end()) {
        static_cast<CGameObject*>(*(pos++))->BltDirtyEx(dst, src, restoreSrc);
    }
}

void CWwdGameObject::BltDirtyRegions(
    CDDrawSurfacePair* dst,
    CDDrawSurfacePair* src,
    CDDrawSurfacePair* restoreSrc
) {
    std::list<CGameObject*>::iterator pos = m_children.begin();
    while (pos != m_children.end()) {
        static_cast<CGameObject*>(*(pos++))->BltDirtyRegions(dst, src, restoreSrc);
    }
}
