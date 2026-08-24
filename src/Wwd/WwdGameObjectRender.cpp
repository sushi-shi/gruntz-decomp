#include <rva.h>

#include <Mfc.h>

#include <DDrawMgr/AnimWorkerObj.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorkerCache.h>
#include <DDrawMgr/DDSurface.h>
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
void CWwdGameObjectC::Render(CDDrawSurfacePair* dst) {
    if (m_clip.left == COORD_UNSET) {
        if (m_screenX < 0 || m_screenY < 0 || m_screenX >= dst->m_width
            || m_screenY >= dst->m_height) {
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

    dst->m_surface->PutPixel(m_screenX, m_screenY, m_dotColor);
    m_dirty.m_lastX = m_screenX;
    m_dirty.m_lastY = m_screenY;
    m_dirty.m_w = 1;
    m_dirty.m_h = 1;
    m_dirty.m_armed = 0;
}

RVA(0x001661d0, 0xc2)
void CWwdGameObjectC::BltDirty(CDDrawSurfacePair* dst, CDDrawSurfacePair* src) {

    m_shadow = m_dirty;
    if (m_shadow.m_armed != -1) {
        u8 pixel = src->m_surface->GetPixel(m_shadow.m_lastX, m_shadow.m_lastY);
        dst->m_surface->PutPixel(m_shadow.m_lastX, m_shadow.m_lastY, pixel);
        m_dirty.m_armed = -1;
    }
}

RVA(0x001662a0, 0x1fa)
void CWwdGameObjectC::BltDirtyEx(
    CDrawSubWorker* dst,
    CDDrawSurfacePair* src,
    CDDrawSurfacePair* restoreSrc
) {
    if (m_dirty.m_armed != -1 && m_shadow.m_armed != -1) {
        i32 dx = abs(m_dirty.m_lastX - m_shadow.m_lastX) + 1;
        i32 dy = abs(m_dirty.m_lastY - m_shadow.m_lastY) + 1;
        if (dx > 0x20 || dy > 0x20) {
            dst->BlitDirtyRect(src, &m_dirty.m_lastX, &m_dirty.m_w);
            dst->BlitDirtyRect(src, &m_shadow.m_lastX, &m_shadow.m_w);
        } else {
            i32 left = m_dirty.m_lastX < m_shadow.m_lastX ? m_dirty.m_lastX : m_shadow.m_lastX;
            i32 top = m_dirty.m_lastY < m_shadow.m_lastY ? m_dirty.m_lastY : m_shadow.m_lastY;
            i32 pos[2];
            i32 size[2];
            size[1] = dy;
            size[0] = dx;
            pos[1] = top;
            pos[0] = left;
            dst->BlitDirtyRect(src, pos, size);
        }
    } else if (m_dirty.m_armed != -1) {
        dst->BlitDirtyRect(src, &m_dirty.m_lastX, &m_dirty.m_w);
    } else if (m_shadow.m_armed != -1) {
        dst->BlitDirtyRect(src, &m_shadow.m_lastX, &m_shadow.m_w);
    }
}

RVA(0x001664a0, 0x133)
void CWwdGameObjectC::BltDirtyRegions(
    CDDrawSurfacePair* dst,
    CDDrawSurfacePair* src,
    CDDrawSurfacePair* restoreSrc
) {
    if (m_dirty.m_armed != -1 && m_shadow.m_armed != -1) {
        i32 dx = abs(m_dirty.m_lastX - m_shadow.m_lastX) + 1;
        i32 dy = abs(m_dirty.m_lastY - m_shadow.m_lastY) + 1;
        if (dx > 0x20 || dy > 0x20) {
            dst->BlitDirtyRect(src, &m_dirty.m_lastX, &m_dirty.m_w);
            dst->BlitDirtyRect(src, &m_shadow.m_lastX, &m_shadow.m_w);
        } else {
            i32 left = m_dirty.m_lastX < m_shadow.m_lastX ? m_dirty.m_lastX : m_shadow.m_lastX;
            i32 top = m_dirty.m_lastY < m_shadow.m_lastY ? m_dirty.m_lastY : m_shadow.m_lastY;
            i32 pos[2];
            i32 size[2];
            size[1] = dy;
            size[0] = dx;
            pos[1] = top;
            pos[0] = left;
            dst->BlitDirtyRect(src, pos, size);
        }
    } else if (m_dirty.m_armed != -1) {
        dst->BlitDirtyRect(src, &m_dirty.m_lastX, &m_dirty.m_w);
    } else if (m_shadow.m_armed != -1) {
        dst->BlitDirtyRect(src, &m_shadow.m_lastX, &m_shadow.m_w);
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
i32 CWwdGameObject::Setup(i32 x, i32 y, i32 sortKey, AnimWorkerObj* tmpl) {
    CLEAR_WWD_GAME_OBJECT_CHILDREN;
    return CGameObject::Setup(x, y, sortKey, tmpl) != 0;
}

// @early-stop
// Ctor chain byte-exact.  Residue: retail sinks the AddTail-failure block below the
// `ret` (`je <tail>`, success falls through) where cl lays it inline (`jne <success>`).
// Both `if (node != NULL) {...}` and a `goto fail` label after the return were measured
// and are worse (78.65 / 80.70) - cl cross-jumps the two `delete result; return 0`
// copies retail keeps apart.  tail-block-placement-cross-jump-wall.md.
RVA(0x00166640, 0x13b)
CWwdGameObject* CWwdGameObject::CreateObject(
    int id,
    int x,
    int y,
    int sortKey,
    AnimWorkerObj* tmpl,
    int stateFlags
) {
    CWwdGameObjectA* result = new CWwdGameObjectA(OwnerMgr(), id, stateFlags, CWapObj::NO_SEED);
    if (result == NULL) {
        return NULL;
    }
    if (result->Setup(x, y, sortKey, tmpl) == 0) {
        delete result;
        return NULL;
    }
    POSITION node = m_children.AddTail(static_cast<CObject*>(result));
    if (node == NULL) {
        delete result;
        return NULL;
    }
    result->m_posCache = node;
    if (result->m_flags & 0x200000) {
        result->m_animWorker->m_notify(result);
    }
    return static_cast<CWwdGameObject*>(result);
}

static inline AnimWorkerObj* LookupWorker(CMapStringToOb& map, LPCTSTR name) {
    CObject* found = NULL;
    map.Lookup(name, found);
    return static_cast<AnimWorkerObj*>(found);
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00166780, 0x57)
CWwdGameObject*
CWwdGameObject::CreateNamed(int id, int x, int y, int sortKey, const char* name, int stateFlags) {
    AnimWorkerObj* tmpl = LookupWorker(OwnerMgr()->m_workerCache->m_workers, name);
    if (tmpl == NULL) {
        return NULL;
    }
    return CreateObject(id, x, y, sortKey, tmpl, stateFlags);
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
        o->m_animWorker->m_notify(o);
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
