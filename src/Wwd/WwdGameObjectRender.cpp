

#define CGAMEOBJECT_OOL_CTOR

#include <Mfc.h>
#include <Rez/RezAlloc.h>
#include <rva.h>
#include <Ints.h>
#include <Win32.h>
#include <ddraw.h>
#include <string.h>
#include <stdlib.h>
#include <DDrawMgr/DDSurface.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <Wwd/WwdGameObjectFamily.h>
#include <Gruntz/WwdGameObject.h>
#include <DDrawMgr/AnimWorkerObj.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerCache.h>

inline void* operator new(u32, void* p) {
    return p;
}

// @early-stop
RVA(0x001660f0, 0xd1)
void CWwdGameObjectC::Render(CDDrawSurfacePair* a) {
    i32 m64 = m_clip.left;
    if (m64 == static_cast<i32>(0x80000000)) {
        if (m_screenX < 0 || m_screenY < 0 || m_screenX >= a->m_width || m_screenY >= a->m_height) {
            m_dirty.m_armed = -1;
            return;
        }
    } else {
        if (m_screenX < m64 || m_screenY < m_clip.top || m_screenX > m_clip.right
            || m_screenY > m_clip.bottom) {
            m_dirty.m_armed = -1;
            return;
        }
    }

    {
        CDDSurface* surf = a->m_surface;
        u8 color = m_dotColor;
        u8* base = static_cast<u8*>(surf->Lock(0));
        if (base != 0) {
            i32 row = surf->m_pitch * m_screenY;
            i32 col = surf->m_bytesPerPixel * m_screenX;
            base[row + col] = color;
            surf->m_ddSurface->Unlock(0);
        }
    }
    m_dirty.m_lastX = m_screenX;
    m_dirty.m_lastY = m_screenY;
    m_dirty.m_w = 1;
    m_dirty.m_h = 1;
    m_dirty.m_armed = 0;
}

// @early-stop
RVA(0x001661d0, 0xc2)
void CWwdGameObjectC::BltDirty(CDDrawSurfacePair* a, CDDrawSurfacePair* b) {

    m_shadow = m_dirty;
    if (m_shadow.m_armed != -1) {
        i32 x = m_shadow.m_lastX;
        i32 y = m_shadow.m_lastY;
        char pixel;
        CDDSurface* sb = b->m_surface;
        char* base = static_cast<char*>(sb->Lock(0));
        if (base != 0) {
            pixel = base[sb->m_bytesPerPixel * x + sb->m_pitch * y];
            sb->m_ddSurface->Unlock(0);
        } else {
            pixel = 0;
        }
        CDDSurface* sa = a->m_surface;
        char* base2 = static_cast<char*>(sa->Lock(0));
        if (base2 != 0) {
            base2[sa->m_bytesPerPixel * x + sa->m_pitch * y] = pixel;
            sa->m_ddSurface->Unlock(0);
        }
        m_dirty.m_armed = -1;
    }
}

// @early-stop
RVA(0x001662a0, 0x1fa)
void CWwdGameObjectC::BltDirtyEx(CDDrawSurfacePair* a, CDDrawSurfacePair* b, CDDrawSurfacePair* c) {
    i32 rc[4];
    if (m_dirty.m_armed != -1 && m_shadow.m_armed != -1) {
        i32 dx = abs(m_dirty.m_lastX - m_shadow.m_lastX) + 1;
        i32 dy = abs(m_dirty.m_lastY - m_shadow.m_lastY) + 1;
        if (dx > 0x20 || dy > 0x20) {
            rc[0] = m_dirty.m_lastX;
            rc[1] = m_dirty.m_lastY;
            rc[2] = m_dirty.m_lastX + m_dirty.m_w;
            rc[3] = m_dirty.m_lastY + m_dirty.m_h;
            a->m_surface->BltEx(rc, b->m_surface, rc, 0x1000000, 0);
            rc[0] = m_shadow.m_lastX;
            rc[1] = m_shadow.m_lastY;
            rc[2] = m_shadow.m_lastX + m_shadow.m_w;
            rc[3] = m_shadow.m_lastY + m_shadow.m_h;
            a->m_surface->BltEx(rc, b->m_surface, rc, 0x1000000, 0);
        } else {
            i32 left = m_dirty.m_lastX < m_shadow.m_lastX ? m_dirty.m_lastX : m_shadow.m_lastX;
            i32 top = m_dirty.m_lastY < m_shadow.m_lastY ? m_dirty.m_lastY : m_shadow.m_lastY;
            rc[0] = left;
            rc[1] = top;
            rc[2] = left + dx;
            rc[3] = top + dy;
            a->m_surface->BltEx(rc, b->m_surface, rc, 0x1000000, 0);
        }
    } else if (m_dirty.m_armed != -1) {
        rc[0] = m_dirty.m_lastX;
        rc[1] = m_dirty.m_lastY;
        rc[2] = m_dirty.m_lastX + m_dirty.m_w;
        rc[3] = m_dirty.m_lastY + m_dirty.m_h;
        a->m_surface->BltEx(rc, b->m_surface, rc, 0x1000000, 0);
    } else if (m_shadow.m_armed != -1) {
        rc[0] = m_shadow.m_lastX;
        rc[1] = m_shadow.m_lastY;
        rc[2] = m_shadow.m_lastX + m_shadow.m_w;
        rc[3] = m_shadow.m_lastY + m_shadow.m_h;
        a->m_surface->BltEx(rc, b->m_surface, rc, 0x1000000, 0);
    }
}

RVA(0x001664a0, 0x133)
void CWwdGameObjectC::BltDirtyRegions(
    CDDrawSurfacePair* a,
    CDDrawSurfacePair* b,
    CDDrawSurfacePair* c
) {
    if (m_dirty.m_armed != -1 && m_shadow.m_armed != -1) {
        i32 dx = abs(m_dirty.m_lastX - m_shadow.m_lastX) + 1;
        i32 dy = abs(m_dirty.m_lastY - m_shadow.m_lastY) + 1;
        if (dx > 0x20 || dy > 0x20) {
            a->BlitDirtyRect(b, &m_dirty.m_lastX, &m_dirty.m_w);
            a->BlitDirtyRect(b, &m_shadow.m_lastX, &m_shadow.m_w);
        } else {
            i32 left = m_dirty.m_lastX < m_shadow.m_lastX ? m_dirty.m_lastX : m_shadow.m_lastX;
            i32 top = m_dirty.m_lastY < m_shadow.m_lastY ? m_dirty.m_lastY : m_shadow.m_lastY;
            i32 pos[2];
            i32 size[2];
            size[1] = dy;
            size[0] = dx;
            pos[1] = top;
            pos[0] = left;
            a->BlitDirtyRect(b, pos, size);
        }
    } else if (m_dirty.m_armed != -1) {
        a->BlitDirtyRect(b, &m_dirty.m_lastX, &m_dirty.m_w);
    } else if (m_shadow.m_armed != -1) {
        a->BlitDirtyRect(b, &m_shadow.m_lastX, &m_shadow.m_w);
    }
}

RVA(0x001665e0, 0x55)
i32 CWwdGameObject::Setup(i32 x, i32 y, i32 sortKey, AnimWorkerObj* tmpl) {
    POSITION pos = m_1dc.GetHeadPosition();
    while (pos != 0) {
        CObject* p = m_1dc.GetNext(pos);
        if (p != 0) {
            delete p;
        }
    }
    m_1dc.RemoveAll();
    return CGameObject::Setup(x, y, sortKey, tmpl) != 0;
}

// @early-stop
RVA(0x00166640, 0x13b)
CWwdGameObject* CWwdGameObject::CreateObject(
    int id,
    int x,
    int y,
    int sortKey,
    AnimWorkerObj* tmpl,
    int stateFlags
) {
    CWwdGameObjectA* result = new CWwdGameObjectA(OwnerMgr(), id, stateFlags);
    if (result == 0) {
        return 0;
    }
    if (result->Setup(x, y, sortKey, tmpl) == 0) {
        delete result;
        return 0;
    }
    POSITION node = m_1dc.AddTail(static_cast<CObject*>(result));
    if (node == 0) {
        delete result;
        return 0;
    }
    result->m_posCache = node;
    if (result->m_flags & 0x200000) {

        result->m_animWorker->m_notify(result);
    }

    return static_cast<CWwdGameObject*>(result);
}

// @early-stop
RVA(0x00166780, 0x57)
CWwdGameObject*
CWwdGameObject::CreateNamed(int id, int x, int y, int sortKey, const char* name, int stateFlags) {
    CObject* val = 0;

    OwnerMgr()->m_workerCache->m_10.Lookup(name, val);
    if (val == 0) {
        return 0;
    }
    return CreateObject(id, x, y, sortKey, static_cast<AnimWorkerObj*>(val), stateFlags);
}

RVA(0x001667e0, 0x2f)
i32 CWwdGameObject::AddChild(CGameObject* child) {
    if (child == 0) {
        return 0;
    }
    POSITION pos = m_1dc.AddTail(static_cast<CObject*>(child));
    if (pos == 0) {
        return 0;
    }
    child->m_posCache = pos;
    return 1;
}

RVA(0x00166810, 0x32)
void CWwdGameObject::Clear() {
    POSITION pos = m_1dc.GetHeadPosition();
    while (pos != 0) {
        CGameObject* o = static_cast<CGameObject*>(m_1dc.GetNext(pos));
        if (o) {
            delete o;
        }
    }
    m_1dc.RemoveAll();
}

RVA(0x00166850, 0x29)
i32 CWwdGameObject::RemoveChild(CGameObject* child) {
    if (child == 0) {
        return 0;
    }
    POSITION pos = child->m_posCache;
    if (pos == 0) {
        return 0;
    }
    m_1dc.RemoveAt(pos);
    return 1;
}

RVA(0x00166880, 0x29)
i32 CWwdGameObject::WalkChildWorkers() {
    i32 count = 0;
    POSITION pos = m_1dc.GetHeadPosition();
    while (pos != 0) {
        CGameObject* o = static_cast<CGameObject*>(m_1dc.GetNext(pos));
        o->m_animWorker->m_notify(o);
        count++;
    }
    return count;
}

RVA(0x001668b0, 0x26)
void CWwdGameObject::Render(CDDrawSurfacePair* ctx) {
    POSITION pos = m_1dc.GetHeadPosition();
    while (pos != 0) {
        static_cast<CGameObject*>(m_1dc.GetNext(pos))->Render(ctx);
    }
}
RVA(0x001668e0, 0x2d)
void CWwdGameObject::BltDirty(CDDrawSurfacePair* a, CDDrawSurfacePair* b) {
    POSITION pos = m_1dc.GetHeadPosition();
    while (pos != 0) {
        static_cast<CGameObject*>(m_1dc.GetNext(pos))->BltDirty(a, b);
    }
}
RVA(0x00166910, 0x34)
void CWwdGameObject::BltDirtyEx(CDDrawSurfacePair* a, CDDrawSurfacePair* b, CDDrawSurfacePair* c) {
    POSITION pos = m_1dc.GetHeadPosition();
    while (pos != 0) {
        static_cast<CGameObject*>(m_1dc.GetNext(pos))->BltDirtyEx(a, b, c);
    }
}
RVA(0x00166950, 0x34)
void CWwdGameObject::BltDirtyRegions(
    CDDrawSurfacePair* a,
    CDDrawSurfacePair* b,
    CDDrawSurfacePair* c
) {
    POSITION pos = m_1dc.GetHeadPosition();
    while (pos != 0) {
        static_cast<CGameObject*>(m_1dc.GetNext(pos))->BltDirtyRegions(a, b, c);
    }
}
