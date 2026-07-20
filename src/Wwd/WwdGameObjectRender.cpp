// WwdGameObjectRender.cpp - the 0x1660f0-0x166984 wwd render/broadcast block:
// CWwdGameObject::RenderDot + the CWwdGameObjectC render slots + ResetAndSetup +
// the CWwdGameObject child-object factory pair (ex-CWwdObjMgrL, dissolved onto the
// family class) + the CWwdGameObject broadcast walkers.
//
// original TU: filename unknown (@identity-TODO - wave4-L dossier #15 block R: the
// 0x1660f0-0x1670d0+ zone (these wwd render fns + the imageset1/2/3 Parse/Query
// families) is BEYOND the wave4-L brief cap and was NOT boundary-mapped; this file
// holds exactly the wwd fns as a correct partial until that zone's own dossier.
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
#include <DDrawMgr/DDrawChildGroup.h>  // CDDrawGroupNode (the broadcast child-list node)
#include <DDrawMgr/DDrawSurfaceMgr.h>  // the CWwdGameObject owner (m_0c) real class
#include <DDrawMgr/DDrawWorkerCache.h> // m_workerCache full type (the +0x10 name map)
#include <Wwd/WwdGameObjCtor.h>        // CWwdGameObjBaseCtor - the shared 0x15b390 base-object ctor

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
// @early-stop
// regalloc-coloring wall (~57%): logic byte-equivalent, but cl swaps x/y across
// the lone free callee-saved pair (x->ebp,y->ebx vs retail x->ebx,y->ebp) so
// every x/y modrm differs, and the 8-bit color either pins bl (forcing an x
// spill + `push ecx`, 47%) or reads inline (dropping retail's early-load+stack-
// spill of color, shrinking the body). No source spelling reproduces retail's
// "x in ebx + color spilled" layout. See const-materialize-into-reg-vs-immediate.
// ---------------------------------------------------------------------------
RVA(0x001660f0, 0xd1)
void CWwdGameObjectC::Render(CDDrawSurfacePair* a) {
    i32 x = m_screenX;
    i32 m64 = m_clip.left;
    i32 y;
    if (m64 == static_cast<i32>(0x80000000)) {
        if (x < 0) {
            goto reject;
        }
        y = m_screenY;
        if (y < 0) {
            goto reject;
        }
        if (x >= a->m_width) {
            goto reject;
        }
        if (y >= a->m_height) {
            goto reject;
        }
    } else {
        if (x < m64) {
            goto reject;
        }
        y = m_screenY;
        if (y < m_clip.top) {
            goto reject;
        }
        if (x > m_clip.right) {
            goto reject;
        }
        if (y > m_clip.bottom) {
            goto reject;
        }
    }

    {
        CDDSurface* surf = a->m_surface;
        i32 base = surf->Lock(static_cast<void*>(0));
        if (base != 0) {
            i32 row = surf->m_pitch * y;
            i32 col = surf->m_b0 * x;
            *reinterpret_cast<char*>((base + row + col)) = m_dotColor;
            surf->m_8->Unlock(0);
        }
    }
    m_lastX = m_screenX;
    m_lastY = m_screenY;
    m_dirtyW = 1;
    m_dirtyH = 1;
    m_38 = 0;
    return;
reject:
    m_38 = -1;
}

// ---------------------------------------------------------------------------
// 0x1661d0 (vtable slot 12): snapshot the live 9-dword state block (@0x18) into the
// shadow block (@0xb8), then - if the shadow's just-copied flag (m_d8, == old m_38)
// is still armed - restore the background pixel at the shadow position (m_b8,m_bc):
// read it from the back pair `b`'s surface and write it onto the front pair `a`'s,
// then disarm the live flag (m_38 = -1). __thiscall, 2 ptr args (ret 0x8).
// @early-stop
// ~73% zero-register-pinning regalloc wall. Logic/CFG/offsets/the 9-dword rep-movs
// snapshot/both lock-read-unlock + lock-write-unlock pixel ops/m_38 disarm all
// reproduced. Residual: retail dedicates the callee-saved ebp to `this` for the whole
// body (surviving the rep-movs + both Lock calls) and spills the restored pixel to a
// stack local (ebx is reused for m_b8); our cl keeps `this` in caller-saved eax and
// spills IT instead, keeping the pixel in bl - so the register operands differ
// throughout. Same values/stores. The permuter found no source spelling that flips
// the this/pixel spill choice. docs/patterns/zero-register-pinning.md.
RVA(0x001661d0, 0xc2)
void CWwdGameObjectC::BltDirty(CDDrawSurfacePair* a, CDDrawSurfacePair* b) {
    memcpy(&m_b8, &m_lastX, 36);
    if (m_d8 != -1) {
        i32 x = m_b8;
        i32 y = m_bc;
        char pixel;
        CDDSurface* sb = b->m_surface;
        char* base = reinterpret_cast<char*>(sb->Lock(0));
        if (base != 0) {
            pixel = base[sb->m_b0 * x + sb->m_pitch * y];
            sb->m_8->Unlock(0);
        } else {
            pixel = 0;
        }
        CDDSurface* sa = a->m_surface;
        char* base2 = reinterpret_cast<char*>(sa->Lock(0));
        if (base2 != 0) {
            base2[sa->m_b0 * x + sa->m_pitch * y] = pixel;
            sa->m_8->Unlock(0);
        }
        m_38 = -1; // m_38
    }
}

// ---------------------------------------------------------------------------
// 0x1662a0 (vtable slot 13): blit the object's dirty region(s) from the back pair
// `b`'s surface onto the front pair `a`'s (CDDSurface::BltEx, same rect for src+dst).
// When both the live (m_38) and shadow (m_d8) records are armed, cover them with ONE
// BltEx over the union rect if their corners are within 32 px in both axes, else two
// BltEx (one per record). Only one armed -> just that record's rect. Each rect is
// {x, y, x+w, y+h}. Arg `c` unused. __thiscall, 3 args (ret 0xc).
// @early-stop
// ~76% tail-merge + regalloc wall (twin of Slot38 which hits 99.7%). Logic/CFG/the
// abs+min bbox/the four BltEx sites + their {x,y,x+w,y+h} rect builds all reproduced,
// AND the single reused rect buffer gives retail's `sub esp,0x14` frame. Residual:
// because every region calls the IDENTICAL `BltEx(rc, b->m_surface, rc, ...)` on the
// one shared `rc` buffer, our cl CROSS-JUMPS (tail-merges) block-C's BltEx to a shared
// copy (a `jmp`) where retail keeps each inline; plus a callee-saved m_b8/m_1c coloring
// swap cascading from the extra BltEx register pressure. Slot38's twin avoids this
// because its four dispatch calls take DIFFERENT pointer args (no merge). Not source-
// steerable (separate rc buffers fix the merge but re-inflate the frame; permuter
// no-op). docs/patterns/zero-register-pinning.md / tail-merge layout.
RVA(0x001662a0, 0x1fa)
void CWwdGameObjectC::BltDirtyEx(CDDrawSurfacePair* a, CDDrawSurfacePair* b, i32 c) {
    i32 rc[4];                      // one reused src+dst rect buffer
    if (m_38 != -1 && m_d8 != -1) { // both armed
        i32 dx = abs(m_lastX - m_b8) + 1;
        i32 dy = abs(m_lastY - m_bc) + 1;
        if (dx > 0x20 || dy > 0x20) {
            rc[0] = m_lastX;
            rc[1] = m_lastY;
            rc[2] = m_lastX + m_dirtyW;
            rc[3] = m_lastY + m_dirtyH;
            a->m_surface->BltEx(rc, b->m_surface, rc, 0x1000000, 0);
            rc[0] = m_b8;
            rc[1] = m_bc;
            rc[2] = m_b8 + m_d0;
            rc[3] = m_bc + m_d4;
            a->m_surface->BltEx(rc, b->m_surface, rc, 0x1000000, 0);
        } else {
            i32 left = m_lastX < m_b8 ? m_lastX : m_b8;
            i32 top = m_lastY < m_bc ? m_lastY : m_bc;
            rc[0] = left;
            rc[1] = top;
            rc[2] = left + dx;
            rc[3] = top + dy;
            a->m_surface->BltEx(rc, b->m_surface, rc, 0x1000000, 0);
        }
    } else if (m_38 != -1) {
        rc[0] = m_lastX;
        rc[1] = m_lastY;
        rc[2] = m_lastX + m_dirtyW;
        rc[3] = m_lastY + m_dirtyH;
        a->m_surface->BltEx(rc, b->m_surface, rc, 0x1000000, 0);
    } else if (m_d8 != -1) {
        rc[0] = m_b8;
        rc[1] = m_bc;
        rc[2] = m_b8 + m_d0;
        rc[3] = m_bc + m_d4;
        a->m_surface->BltEx(rc, b->m_surface, rc, 0x1000000, 0);
    }
}

// ---------------------------------------------------------------------------
// 0x1664a0 (vtable slot 14): dispatch the front pair `a`'s empty dirty-rect blit
// hook (0x164650) over the object's dirty region(s). When both the live (m_38) and
// shadow (m_d8) records are armed, cover them with ONE combined region if their
// corners are within 32 px in both axes (the union {min pos, |delta|+1 size}), else
// emit both records separately. Only one armed -> just that record. Arg `c` unused.
// __thiscall, 3 args (ret 0xc).
// @early-stop
// 99.70% - logic/CFG/block-layout/the abs+min bbox/all four dispatch sites byte-exact.
// The lone residual is a callee-saved coin-flip: the two hoisted record base addresses
// (&m_18, &m_b8) land in retail's edi/ebx but our cl's ebx/edi (swapped), which cascades
// only the two push operands in the large-delta path. Same addresses. The permuter found
// no source spelling that flips the pair. docs/patterns/zero-register-pinning.md.
RVA(0x001664a0, 0x133)
void CWwdGameObjectC::BltDirtyRegions(CDDrawSurfacePair* a, CDDrawSurfacePair* b, i32 c) {
    if (m_38 != -1 && m_d8 != -1) { // both armed -> combined region
        i32 dx = abs(m_lastX - m_b8) + 1;
        i32 dy = abs(m_lastY - m_bc) + 1;
        if (dx > 0x20 || dy > 0x20) {
            a->BlitDirtyRect_164650(b, &m_lastX, &m_dirtyW); // live record
            a->BlitDirtyRect_164650(b, &m_b8, &m_d0);        // shadow record
        } else {
            i32 left = m_lastX < m_b8 ? m_lastX : m_b8; // min x
            i32 top = m_lastY < m_bc ? m_lastY : m_bc;  // min y
            i32 pos[2];
            i32 size[2];
            size[1] = dy;
            size[0] = dx;
            pos[1] = top;
            pos[0] = left;
            a->BlitDirtyRect_164650(b, pos, size);
        }
    } else if (m_38 != -1) {
        a->BlitDirtyRect_164650(b, &m_lastX, &m_dirtyW); // live record only
    } else if (m_d8 != -1) {
        a->BlitDirtyRect_164650(b, &m_b8, &m_d0); // shadow record only
    }
}

RVA(0x001665e0, 0x55)
i32 CWwdGameObject::Setup(i32 a1, i32 a2, i32 a3, i32 a4) {
    POSITION pos = m_1dc.GetHeadPosition();
    while (pos != 0) {
        CObject* p = static_cast<CObject*>(static_cast<void*>(m_1dc.GetNext(pos)));
        if (p != 0) {
            delete p;
        }
    }
    m_1dc.RemoveAll();
    return CGameObject::Setup(a1, a2, a3, a4) != 0;
}

// ===========================================================================
// 0x166640 - factory for the 0x1dc-byte kind, published into the manager's own
// CPtrList (AddTail) at +0x1dc. __thiscall, 6 stack args (ret 0x18). Build slot
// +0x28 (4 args), dtor +0x04.
// @early-stop
// rezalloc-placement-new wall (same family as 0x1598d0): the object construction
// + field stores + vtable stamps are byte-exact, but retail allocates the object
// through the throwing class operator new and carries the /GX ctor-in-flight EH
// frame (push -1/fs:0 + trylevel-0 cleanup), while the RezAlloc + placement body
// emits no frame.  docs/patterns/rezalloc-placement-new-no-eh-frame.md.
// ===========================================================================
RVA(0x00166640, 0x13b)
CWwdGameObject*
CWwdGameObject::CreateObject_166640(int a1, int a2, int a3, int a4, int a5, int a6) {
    char* obj = static_cast<char*>(RezAlloc(0x1dc));
    CWwdGameObjectA* result;
    if (obj != 0) {
        int root = m_0c; // the CLoadable owner int handle (== this->m_0c, the CDDrawSurfaceMgr)
        new (obj) CWwdGameObjBaseCtor(root, a1, a6);
        result = reinterpret_cast<CWwdGameObjectA*>(obj);
        // the embedded +0x1a0 CAniAdvanceCursor(owner=root, field04=a1, field08=a6): retail
        // INLINES the ctor here (no call), spelled out so the store shape matches; its 0x5f0128
        // vptr stamp is compiler-emitted-vtable-dropped (% ok per drive-to-0).
        result->m_1a0.m_04 = a1;
        result->m_1a0.m_flags = a6;
        result->m_1a0.m_0c = root;
        result->m_1a0.m_10 = 0;
        result->m_1a0.m_14 = 0;
        result->m_1a0.m_element = 0;
        // the CWwdGameObjectA vptr (0x5f00a8) stamp is compiler-emitted-vtable-dropped.
        result->m_18c = -1;
        result->m_190 = -1;
        result->m_layer = 0;
        result->m_194 = 0;
        result->m_19c = 0;
    } else {
        result = 0;
    }
    if (result == 0) {
        return 0;
    }
    if (result->Setup(a2, a3, a4, a5) == 0) {
        delete result; // virtual scalar-deleting dtor (slot 1)
        return 0;
    }
    POSITION node = m_1dc.AddTail(static_cast<CObject*>(result));
    if (node == 0) {
        delete result; // virtual scalar-deleting dtor (slot 1)
        return 0;
    }
    result->m_posCache = reinterpret_cast<i32>(node);
    if (result->m_flags & 0x200000) {
        // retail fires the +0x10 FN POINTER (m_notify), never a vtable slot
        result->m_7c->m_notify(result);
    }
    // the flat CWwdGameObject dispatch model and the CWwdGameObjectA family model are two
    // reconstructions of the ONE retail object (offset-0 identity); the return reinterprets.
    return static_cast<CWwdGameObject*>(static_cast<void*>(result));
}

// CreateNamed_166780 (__thiscall, ret 0x18 => 6 args). Resolve `name` -> value; if
// nothing resolved, bail; else create the 0x1dc-byte kind with the value as arg5.
// @early-stop
// 94% - logic byte-exact; same val=0 arg-push scheduling residual as CreateNamed_1593e0.
RVA(0x00166780, 0x57)
CWwdGameObject*
CWwdGameObject::CreateNamed_166780(int a1, int a2, int a3, int a4, const char* name, int a6) {
    CObject* val = 0;
    // m_0c is the CLoadable owner int handle == the CDDrawSurfaceMgr; its worker-cache name
    // map (CMapStringToOb @+0x10, Lookup 0x1b8008 - disasm-confirmed, NOT the CMapStringToPtr
    // the ex-view guessed) resolves `name` -> the child's arg5.
    OwnerMgr()->m_workerCache->m_10.Lookup(name, val);
    if (val == 0) {
        return 0;
    }
    return CreateObject_166640(a1, a2, a3, a4, reinterpret_cast<int>(val), a6);
}

RVA(0x001667e0, 0x2f)
i32 CWwdGameObject::AddChild_1667e0(CGameObject* child) {
    if (child == 0) {
        return 0;
    }
    POSITION pos = m_1dc.AddTail(static_cast<CObject*>(child));
    if (pos == 0) {
        return 0;
    }
    child->m_posCache = reinterpret_cast<i32>(pos);
    return 1;
}

RVA(0x00166810, 0x32)
void CWwdGameObject::Clear_166810() {
    CDDrawGroupNode* n = reinterpret_cast<CDDrawGroupNode*>(m_1dc.GetHeadPosition());
    while (n) {
        CDDrawGroupNode* cur = n;
        n = n->m_next;
        if (cur->m_obj) {
            delete cur->m_obj;
        }
    }
    m_1dc.RemoveAll();
}

RVA(0x00166850, 0x29)
i32 CWwdGameObject::RemoveChild_166850(CGameObject* child) {
    if (child == 0) {
        return 0;
    }
    POSITION pos = reinterpret_cast<POSITION>(child->m_posCache);
    if (pos == 0) {
        return 0;
    }
    m_1dc.RemoveAt(pos);
    return 1;
}

RVA(0x00166880, 0x29)
i32 CWwdGameObject::WalkChildWorkers_166880() {
    i32 count = 0;
    for (CDDrawGroupNode* n = reinterpret_cast<CDDrawGroupNode*>(m_1dc.GetHeadPosition()); n != 0;) {
        CDDrawGroupNode* cur = n;
        n = n->m_next;
        CGameObject* o = cur->m_obj;
        o->m_7c->m_notify(o);
        count++;
    }
    return count;
}

RVA(0x001668b0, 0x26)
void CWwdGameObject::Render(CDDrawSurfacePair* ctx) {
    CDDrawGroupNode* n = reinterpret_cast<CDDrawGroupNode*>(m_1dc.GetHeadPosition());
    if (n != 0) {
        do {
            CDDrawGroupNode* cur = n;
            n = n->m_next;
            cur->m_obj->Render(ctx);
        } while (n != 0);
    }
}
RVA(0x001668e0, 0x2d)
void CWwdGameObject::BltDirty(CDDrawSurfacePair* a, CDDrawSurfacePair* b) {
    CDDrawGroupNode* n = reinterpret_cast<CDDrawGroupNode*>(m_1dc.GetHeadPosition());
    if (n != 0) {
        do {
            CDDrawGroupNode* cur = n;
            n = n->m_next;
            cur->m_obj->BltDirty(a, b);
        } while (n != 0);
    }
}
RVA(0x00166910, 0x34)
void CWwdGameObject::BltDirtyEx(CDDrawSurfacePair* a, CDDrawSurfacePair* b, i32 c) {
    CDDrawGroupNode* n = reinterpret_cast<CDDrawGroupNode*>(m_1dc.GetHeadPosition());
    if (n != 0) {
        do {
            CDDrawGroupNode* cur = n;
            n = n->m_next;
            cur->m_obj->BltDirtyEx(a, b, c);
        } while (n != 0);
    }
}
RVA(0x00166950, 0x34)
void CWwdGameObject::BltDirtyRegions(CDDrawSurfacePair* a, CDDrawSurfacePair* b, i32 c) {
    CDDrawGroupNode* n = reinterpret_cast<CDDrawGroupNode*>(m_1dc.GetHeadPosition());
    if (n != 0) {
        do {
            CDDrawGroupNode* cur = n;
            n = n->m_next;
            cur->m_obj->BltDirtyRegions(a, b, c);
        } while (n != 0);
    }
}
