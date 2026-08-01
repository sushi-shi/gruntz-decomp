// WwdGameObjectRender.cpp - the 0x1660f0-0x166984 wwd render/broadcast block:
// CWwdGameObject::RenderDot + the CWwdGameObjectC render slots + ResetAndSetup +
// the CWwdGameObject child-object factory pair (ex-CWwdObjMgrL, dissolved onto the
// family class) + the CWwdGameObject broadcast walkers.
//
// original TU: filename unknown (@identity-TODO - wave4-L dossier #15 block R: the
// 0x1660f0-0x1670d0+ zone (these wwd render fns + the imageset1/2/3 Parse/Query
// families) is BEYOND the wave4-L brief cap and was NOT boundary-mapped; this file
// holds exactly the wwd fns as a correct partial until that zone's own dossier.

// CWwdGameObject::CreateObject @0x166640 CALLS the shared CGameObject ctor COMDAT
// (0x15b390) instead of folding it, so this TU takes the declaration-only form of
// it - the per-TU guard described in <Gruntz/WwdGridIter.h>. The body lives in
// src/Wwd/WwdFactoryObject.cpp.
#define CGAMEOBJECT_OOL_CTOR

#include <Mfc.h>
#include <Rez/RezAlloc.h> // RezAlloc/RezFree
#include <rva.h>
#include <Ints.h>
#include <Win32.h> // windows.h base types (ddraw.h needs them first)
#include <ddraw.h> // IDirectDrawSurface::Unlock for the pixel plots
#include <string.h>
#include <stdlib.h> // abs() (the Slot34/38 dirty-rect deltas)
#include <DDrawMgr/DDSurface.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <Wwd/WwdGameObjectFamily.h>   // the CGameObject/A/F/B/C hierarchy
#include <Gruntz/WwdGameObject.h>      // canonical CWwdGameObject
#include <DDrawMgr/AnimWorkerObj.h>    // the canonical +0x7c worker (m_notify fire callback)
#include <DDrawMgr/DDrawChildGroup.h>  // CDDrawChildGroup (the broadcast child list)
#include <DDrawMgr/DDrawSurfaceMgr.h>  // the CWwdGameObject owner (m_0c) real class
#include <DDrawMgr/DDrawWorkerCache.h> // m_workerCache full type (the +0x10 name map)

inline void* operator new(u32, void* p) {
    return p;
} // placement (factory base-object ctor)

// ---------------------------------------------------------------------------
// RenderDot (0x1660f0): plot the object's (+0x5c,+0x60) position as a single
// 8-bit pixel into the render context's surface, after a bounds check (either
// against the context clip extent when +0x64 is unbounded (0x80000000) or
// against the object's own +0x64..+0x70 clip rect). On a successful plot, cache
// the position to +0x18/+0x1c, mark +0x30/+0x34 dirty and +0x38 = 0; on a clip
// reject, +0x38 = -1.  __thiscall, 1 stack arg (ret 4), no EH frame.
//
// Two things the single shared `goto reject` label hid. Retail's block order is
// chainA / reject / chainB / plot - i.e. EACH arm carries its OWN
// `m_dirty.m_armed = -1; return;` (cl tail-merges them down to the two copies at
// 0x166128 and 0x1661b3), which is why the unbounded arm's last test is `jl`
// INTO the plot while the clipped arm falls through into it. One shared label
// gives one reject block and inverts that last test. And `m_dotColor` is read
// into a local BEFORE the Lock call (`mov al,[esi+0x18c]` + a stack spill at
// 0x166154), not off `this` after it.
//
// Spelling matters here: the per-TEST reject (one `if`/`return` per bound) makes
// cl emit EIGHT copies of the epilogue and craters the function to 0.15%. It has
// to be the `||` chain - one reject statement per ARM - which cl lays out as
// chainA / rejectA / chainB / plot and then cross-jumps seven of the eight exits
// onto rejectB, leaving retail's two copies.
//
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

// ---------------------------------------------------------------------------
// 0x1661d0 (vtable slot 12): snapshot the live 9-dword state block (@0x18) into the
// shadow block (@0xb8), then - if the shadow's just-copied armed flag
// is still set - restore the background pixel at the shadow position (m_shadow.m_lastX/m_y):
// read it from the back pair `b`'s surface and write it onto the front pair `a`'s,
// then disarm the live flag (m_38 = -1). __thiscall, 2 ptr args (ret 0x8).
// @early-stop
RVA(0x001661d0, 0xc2)
void CWwdGameObjectC::BltDirty(CDDrawSurfacePair* a, CDDrawSurfacePair* b) {
    // the live +0x18 record snapshotted onto the shadow +0xb8 one - the SAME
    // 0x24-byte WwdDirtyRect shape at both offsets, which is why one 36-byte move does it
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
        m_dirty.m_armed = -1; // m_38
    }
}

// ---------------------------------------------------------------------------
// 0x1662a0 (vtable slot 13): blit the object's dirty region(s) from the back pair
// `b`'s surface onto the front pair `a`'s (CDDSurface::BltEx, same rect for src+dst).
// When both the live (m_dirty.m_armed) and shadow (m_shadow.m_armed) records are armed, cover them with ONE
// BltEx over the union rect if their corners are within 32 px in both axes, else two
// BltEx (one per record). Only one armed -> just that record's rect. Each rect is
// {x, y, x+w, y+h}. Arg `c` unused. __thiscall, 3 args (ret 0xc).
// @early-stop
RVA(0x001662a0, 0x1fa)
void CWwdGameObjectC::BltDirtyEx(CDDrawSurfacePair* a, CDDrawSurfacePair* b, CDDrawSurfacePair* c) {
    i32 rc[4];                                             // one reused src+dst rect buffer
    if (m_dirty.m_armed != -1 && m_shadow.m_armed != -1) { // both armed
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

// ---------------------------------------------------------------------------
// 0x1664a0 (vtable slot 14): dispatch the front pair `a`'s empty dirty-rect blit
// hook (0x164650) over the object's dirty region(s). When both the live (m_38) and
// shadow (m_shadow.m_armed) records are armed, cover them with ONE combined region if their
// corners are within 32 px in both axes (the union {min pos, |delta|+1 size}), else
// emit both records separately. Only one armed -> just that record. Arg `c` unused.
// __thiscall, 3 args (ret 0xc).
RVA(0x001664a0, 0x133)
void CWwdGameObjectC::BltDirtyRegions(
    CDDrawSurfacePair* a,
    CDDrawSurfacePair* b,
    CDDrawSurfacePair* c
) {
    if (m_dirty.m_armed != -1 && m_shadow.m_armed != -1) { // both armed -> combined region
        i32 dx = abs(m_dirty.m_lastX - m_shadow.m_lastX) + 1;
        i32 dy = abs(m_dirty.m_lastY - m_shadow.m_lastY) + 1;
        if (dx > 0x20 || dy > 0x20) {
            a->BlitDirtyRect(b, &m_dirty.m_lastX, &m_dirty.m_w);   // live record
            a->BlitDirtyRect(b, &m_shadow.m_lastX, &m_shadow.m_w); // shadow record
        } else {
            i32 left =
                m_dirty.m_lastX < m_shadow.m_lastX ? m_dirty.m_lastX : m_shadow.m_lastX; // min x
            i32 top =
                m_dirty.m_lastY < m_shadow.m_lastY ? m_dirty.m_lastY : m_shadow.m_lastY; // min y
            i32 pos[2];
            i32 size[2];
            size[1] = dy;
            size[0] = dx;
            pos[1] = top;
            pos[0] = left;
            a->BlitDirtyRect(b, pos, size);
        }
    } else if (m_dirty.m_armed != -1) {
        a->BlitDirtyRect(b, &m_dirty.m_lastX, &m_dirty.m_w); // live record only
    } else if (m_shadow.m_armed != -1) {
        a->BlitDirtyRect(b, &m_shadow.m_lastX, &m_shadow.m_w); // shadow record only
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

// ===========================================================================
// 0x166640 - CWwdGameObject::CreateObject: build a child A-kind object and publish
// it into this manager's own CObList at +0x1dc (AddTail). __thiscall, 6 stack args
// (ret 0x18). Retail CALLS the shared CGameObject ctor COMDAT (0x15b390) and
// INLINES the +0x1a0 cursor - the two halves of `new CWwdGameObjectA(...)`, which
// is now exactly what this TU emits (the CGAMEOBJECT_OOL_CTOR guard at the top of
// the file; 42.4 -> 71.7 %).
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
        delete result; // virtual scalar-deleting dtor (slot 1)
        return 0;
    }
    POSITION node = m_1dc.AddTail(static_cast<CObject*>(result));
    if (node == 0) {
        delete result; // virtual scalar-deleting dtor (slot 1)
        return 0;
    }
    result->m_posCache = node;
    if (result->m_flags & 0x200000) {
        // retail fires the +0x10 FN POINTER (m_notify), never a vtable slot
        result->m_7c->m_notify(result);
    }
    // CWwdGameObject derives from CWwdGameObjectA, so this is the ordinary
    // single-inheritance downcast - no void* hop. It is still a mis-model: the two
    // classes are two reconstructions of the ONE retail object (offset-0 identity),
    // and merging them is what retires the cast (@identity-TODO, see
    // <Wwd/WwdGameObjectFamily.h>).
    return static_cast<CWwdGameObject*>(result);
}

// CreateNamed (__thiscall, ret 0x18 => 6 args). Resolve `name` -> value; if
// nothing resolved, bail; else create the 0x1dc-byte kind with the value as arg5.
// @early-stop
RVA(0x00166780, 0x57)
CWwdGameObject*
CWwdGameObject::CreateNamed(int id, int x, int y, int sortKey, const char* name, int stateFlags) {
    CObject* val = 0;
    // m_0c is the CLoadable owner int handle == the CDDrawSurfaceMgr; its worker-cache name
    // map (CMapStringToOb @+0x10, Lookup 0x1b8008 - disasm-confirmed, NOT the CMapStringToPtr
    // the ex-view guessed) resolves `name` -> the registered type template. The out-param
    // is CObject*& (the MFC container's own interface), so the narrowing to the map's one
    // real value type is language-forced here, not a mis-model: CDDrawWorkerCache::
    // CreateWorker @0x1652c0 is the map's ONLY writer and every value it stores is a
    // 0x17c-byte ??_7AnimWorkerObj@@6B@-stamped record.
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
        o->m_7c->m_notify(o);
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
