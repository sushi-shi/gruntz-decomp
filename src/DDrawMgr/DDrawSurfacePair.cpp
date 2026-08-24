#include <rva.h>

#include <DDrawMgr/DDrawSurfacePair.h>

#include <Mfc.h>

#include <AddrWord.h>
#include <DDrawMgr/AnimWorkerObj.h>
#include <DDrawMgr/AniRecord.h>
#include <DDrawMgr/ColorDepth.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawPtrCollections.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorker.h>
#include <DDrawMgr/DDrawWorkerCache.h>
#include <DDrawMgr/DDrawWorkerCtx.h>
#include <DDrawMgr/DDrawWorkerList.h>
#include <DDrawMgr/DDrawWorkerMapSmall.h>
#include <DDrawMgr/DDrawWorkerNode.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DDSurface.h>
#include <DDrawMgr/DirectDrawMgr.h>
#include <DDrawMgr/LogicRecord.h>
#include <Enums.h>
#include <Gruntz/AniElement.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/MapStringToOb.h>
#include <Gruntz/ParseSource.h>
#include <Gruntz/ResolveNode.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/String.h>
#include <Gruntz/UserLogic.h>
#include <Image/ImageSet.h>
#include <Io/FileMem.h>
#include <Pix16.h>
#include <Rez/RezTypeTag.h>
#include <Utils/MapTyped.h>

#include <ddraw.h>
#include <stdio.h>
#include <string.h>

RVA(0x00163bc0, 0x2c)
void CDDrawWorkerList::Unload() {
    POSITION pos = m_workers.GetHeadPosition();
    while (pos) {
        CDDrawWorkerBase* child = static_cast<CDDrawWorkerBase*>(m_workers.GetNext(pos));
        if (child) {
            delete child;
        }
    }
    m_workers.RemoveAll();
}

RVA(0x00163bf0, 0x6d)
void CDDrawWorkerList::RenderAndPruneWorkers(
    CDDrawSurfacePair* backBuffer,
    CDDrawSurfacePair* overlay
) {
    POSITION pos = m_workers.GetHeadPosition();
    while (pos) {
        POSITION cur = pos;
        CDDrawWorkerBase* child = static_cast<CDDrawWorkerBase*>(m_workers.GetNext(pos));
        child->RenderFrame(backBuffer, overlay);
        child->m_refCount--;
        if ((overlay->m_surface != NULL && (overlay->m_flags & 0x20000) == 0)
            || child->m_refCount <= 0) {
            m_workers.RemoveAt(cur);
            if (child) {
                delete child;
            }
        }
    }
}

RVA(0x00163c60, 0x2c)
void CDDrawWorkerList::ClearWorkers() {
    POSITION pos = m_workers.GetHeadPosition();
    while (pos) {
        CDDrawWorkerBase* child = static_cast<CDDrawWorkerBase*>(m_workers.GetNext(pos));
        if (child) {
            delete child;
        }
    }
    m_workers.RemoveAll();
}

// @early-stop
// one scheduling slot: retail computes `cmp m_id,1` BEFORE the geometry stores
// (flags live across the flag-neutral movs), ours places it at the branch. A
// comparison-valued local (`front = m_id == 1`) materializes and scores lower.
RVA(0x00163c90, 0x116)
i32 CDDrawSurfacePair::Create(i32 w, i32 h, ColorDepth bpp, i32 flags) {
    m_flags = flags;
    if (w <= 0 || h <= 0) {

        if (m_id == 1) {
            if (OwnerMgr()->m_lastError == WORLDERR_NONE) {
                OwnerMgr()->m_lastError = WORLDERR_FRONT_DIMENSIONS;
            }
        } else {
            if (OwnerMgr()->m_lastError == WORLDERR_NONE) {
                OwnerMgr()->m_lastError = WORLDERR_BACK_DIMENSIONS;
            }
        }
        return 0;
    }
    i32 kind = m_id;

    m_width = w;
    m_height = h;
    m_bpp = bpp;
    RECT* rect = &m_srcRect;
    rect->left = 0;
    rect->top = 0;
    rect->right = w;
    rect->bottom = h;
    if (kind == 1) {
        CDDrawSurfaceMgr* mgr = OwnerMgr();
        m_surface = mgr->m_ptrColl->CreatePoolItem(mgr->m_drawTarget->m_frontPair->m_surface, 4);
        if (m_surface == NULL) {
            if (OwnerMgr()->m_lastError == WORLDERR_NONE) {
                OwnerMgr()->m_lastError = WORLDERR_FRONT_SURFACE_COPY;
            }
            return 0;
        }
    }
    if (m_id != 1) {
        if (m_flags & 0x10000) {
            m_surface = OwnerMgr()->m_ptrColl->MakeAndAddB(w, h, BPP_UNSET, 0, -1);
        } else {
            m_surface = OwnerMgr()->m_ptrColl->CreateKeyedSurface(w, h, BPP_UNSET, 0, -1);
        }
        if (m_surface == NULL) {
            if (OwnerMgr()->m_lastError == WORLDERR_NONE) {
                OwnerMgr()->m_lastError = WORLDERR_BACK_SURFACE_CREATE;
            }
            return 0;
        }
    }
    m_ownsSurface = 1;
    return 1;
}

RVA(0x00163db0, 0x64)
i32 CDDrawSurfacePair::InitFromSurface(CDDSurface* src) {

    if (src == NULL) {
        return 0;
    }
    i32 w = src->m_width;
    ColorDepth bpp = src->m_bitDepth;
    i32 h = src->m_height;
    if (w <= 0 || h <= 0) {
        return 0;
    }
    m_width = w;
    m_srcRect.right = w;
    m_height = h;
    m_bpp = bpp;
    m_srcRect.left = 0;
    m_srcRect.top = 0;
    m_srcRect.bottom = h;
    m_id = 0x63;
    m_surface = src;
    m_ownsSurface = 0;
    return 1;
}

RVA(0x00163e20, 0x2d)
void CDDrawSurfacePair::Unload() {
    if (m_surface != NULL && m_ownsSurface != 0) {
        CDDrawPtrCollections* pool = OwnerMgr()->m_ptrColl;
        pool->RemoveItemA(m_surface);
        m_surface = NULL;
    }
    m_width = 0;
}

RVA(0x00163e50, 0x8b)
i32 CDDrawSurfacePair::LoadImage(CParseSource* src) {
    BEGIN_FILE_IMAGE_PARSE(src, type, buf)
    i32 r = m_surface->Resolve(OwnerMgr()->m_ptrColl, buf, type, src->m_length, 0);
    src->EndParse();
    return r;
}

RVA(0x00163ee0, 0x19)
i32 CDDrawSurfacePair::ResolveImageName(char* name) {
    return m_surface->MakeImageKey(OwnerMgr()->m_ptrColl, name, 0);
}

RVA(0x00163f00, 0x40)
i32 CDDrawSurfacePair::RestoreIfLost() {
    if (m_surface == NULL) {
        return 1;
    }
    IDirectDrawSurface* s = m_surface->m_ddSurface;
    if (s != NULL && s->IsLost() == 0) {
        return 1;
    }

    CDDSurface* held = m_surface;
    IDirectDrawSurface* r = held->m_ddSurface;

    i32 hr = r->Restore();
    return hr == 0;
}

// @early-stop
// The site-1 offset is now the FIRST statement of each bpp arm, which is what makes
// cl hoist the common surface load above the branch the way retail does (the `n =
// 2 * w` doubling has to follow it, not precede it). Term order inside the sum is
// still inert - cl canonicalises a 2-term sum, re-confirmed here. Residue: cl
// hoists m_pitch out of the arms where retail hoists m_bytesPerPixel, and the
// frame slot for the row count lands at 0x1c instead of 0x14.
RVA(0x00163f40, 0x23e)
void CDDrawSurfacePair::DrawBox(RECT* rect, i32 color) {

    if (rect->left < 0 || rect->left >= m_width) {
        return;
    }
    if (rect->top < 0 || rect->top >= m_height) {
        return;
    }
    if (rect->right < 0 || rect->right >= m_width) {
        return;
    }
    if (rect->bottom < 0 || rect->bottom >= m_height) {
        return;
    }
    char* base = static_cast<char*>(m_surface->Lock(0));
    if (base == NULL) {
        return;
    }

    u8 c = static_cast<u8>(color);
    i32 w = rect->right - rect->left + 1;

    if (m_bpp == BPP_RGB_16) {
        i32 offTop = m_surface->m_pitch * rect->top + m_surface->m_bytesPerPixel * rect->left;
        i32 n = 2 * w;
        if (n > 0) {
            memset(base + offTop, color, n);
        }
        i32 offBot = m_surface->m_pitch * rect->bottom + m_surface->m_bytesPerPixel * rect->left;
        if (n > 0) {
            memset(base + offBot, color, n);
        }
    } else {
        i32 offTop = m_surface->m_pitch * rect->top + m_surface->m_bytesPerPixel * rect->left;
        if (w > 0) {
            memset(base + offTop, color, w);
        }
        i32 offBot = m_surface->m_pitch * rect->bottom + m_surface->m_bytesPerPixel * rect->left;
        if (w > 0) {
            memset(base + offBot, color, w);
        }
    }

    {
        i32 h = rect->bottom - rect->top + 1;
        for (i32 y = 0; y < h; ++y) {
            if (m_bpp == BPP_RGB_16) {
                i32 lo =
                    (rect->top + y) * m_surface->m_pitch + m_surface->m_bytesPerPixel * rect->left;
                base[lo] = c;
                base[lo + 1] = c;
                i32 ro =
                    (rect->top + y) * m_surface->m_pitch + m_surface->m_bytesPerPixel * rect->right;
                base[ro] = c;
                base[ro + 1] = c;
            } else {
                i32 lo =
                    (rect->top + y) * m_surface->m_pitch + m_surface->m_bytesPerPixel * rect->left;
                base[lo] = c;
                i32 ro =
                    (rect->top + y) * m_surface->m_pitch + m_surface->m_bytesPerPixel * rect->right;
                base[ro] = c;
            }
        }
    }

    m_surface->m_ddSurface->Unlock(NULL);
}

// @early-stop
// Register-rotation cursor phase at the Unlock tail; the streams are otherwise
// identical (docs/patterns/register-colour-is-cursor-phase-not-a-work-item.md).
RVA(0x00164180, 0xcd)
void CDDrawSurfacePair::DrawCross(i32 x, i32 y) {
    if (x - 4 < 0) {
        return;
    }
    if (x + 4 >= m_width) {
        return;
    }
    if (y - 4 < 0) {
        return;
    }
    if (y + 4 >= m_height) {
        return;
    }
    char* base = static_cast<char*>(m_surface->Lock(0));
    if (base == NULL) {
        return;
    }
    i32 off = m_surface->m_bytesPerPixel * x + m_surface->m_pitch * y;

    i32 i;
    char* p = base + off - 1;
    for (i = 0; i < 3; ++i) {
        *p = 0;
        --p;
    }

    for (i = 1; i <= 3; ++i) {
        base[off + i] = 0;
    }

    i32 up = off - m_surface->m_pitch;
    for (i = 0; i < 3; ++i) {
        base[up] = static_cast<char>(0xff);
        up -= m_surface->m_pitch;
    }

    i32 down = off + m_surface->m_pitch;
    for (i = 0; i < 3; ++i) {
        base[down] = static_cast<char>(0xff);
        down += m_surface->m_pitch;
    }

    m_surface->m_ddSurface->Unlock(NULL);
}

RVA(0x00164250, 0x12b)
i32 CDDrawSurfacePair::SetGeom(i32 w, i32 h, ColorDepth bpp) {
    if (m_width != w || m_height != h || m_bpp != bpp) {
        i32 sysmem;
        if (static_cast<DDrawPageKind>(m_id) == DDRAW_PAGE_OVERLAY) {
            DDSCAPS caps;
            if (0 == m_surface->m_ddSurface->GetCaps(&caps)) {
                sysmem = 0x800 & caps.dwCaps;
            } else {
                sysmem = 0;
            }
        }
        OwnerMgr()->m_ptrColl->RemoveItemA(m_surface);
        m_surface = NULL;
        if (static_cast<DDrawPageKind>(m_id) == DDRAW_PAGE_BACK) {
            CDDrawSurfaceMgr* mgr = OwnerMgr();
            m_surface =
                mgr->m_ptrColl->CreatePoolItem(mgr->m_drawTarget->m_frontPair->m_surface, 4);
            if (m_surface == NULL) {
                return 0;
            }
        }
        if (m_id != 1) {
            if (sysmem != 0) {
                m_surface = OwnerMgr()->m_ptrColl->MakeAndAddB(w, h, bpp, 0, -1);
            } else {
                m_surface = OwnerMgr()->m_ptrColl->CreateKeyedSurface(w, h, bpp, 0, -1);
            }
            if (m_surface == NULL) {
                return 0;
            }
        }
        if (w <= 0 || h <= 0
            || (bpp != BPP_PALETTED_8 && bpp != BPP_RGB_16 && bpp != BPP_RGB_24
                && bpp != BPP_RGB_32)) {
            return 0;
        }
        m_srcRect.left = 0;
        m_srcRect.top = 0;
        m_width = w;
        m_height = h;
        m_bpp = bpp;
        m_srcRect.right = w;
        m_srcRect.bottom = h;
    }
    return 1;
}

RVA(0x00164380, 0x98)
void CDDrawSurfacePair::DrawCount(RECT* rc, i32 n) {
    char buf[0x20];
    sprintf(buf, "%i", n);
    CDDSurface* w = m_surface;
    if (!w) {
        return;
    }
    HDC hdc = NULL;
    w->m_ddSurface->GetDC(&hdc);
    if (!hdc) {
        return;
    }
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, 0xffffff);
    DrawTextA(hdc, buf, strlen(buf), rc, 0x25);
    w->m_ddSurface->ReleaseDC(hdc);
}

RVA(0x00164420, 0x79)
void CDDrawSurfacePair::DrawLabel(RECT* rc, char* text) {
    CDDSurface* w = m_surface;
    if (!w) {
        return;
    }
    HDC hdc = NULL;
    w->m_ddSurface->GetDC(&hdc);
    if (!hdc) {
        return;
    }
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, 0xffffff);
    DrawTextA(hdc, text, strlen(text), rc, 0x25);
    w->m_ddSurface->ReleaseDC(hdc);
}

// @early-stop
// one block placement: the shared CREATE_DEVICE (0xbb9) arm - reached by both
// the switch fall-out and the err==NONE path - sits INLINE right after the five
// jump-table arms in retail; our cl sinks the merged copy to the end of the
// function. Duplicated arms, a deduplicated fall-through, and an explicit
// `default:` arm all produce the same sunk layout (over-merge placement family).
RVA(0x001644a0, 0x1b0)
i32 CDDrawSurfaceChildA::SetGeometry(i32 w, i32 h, ColorDepth bpp) {
    CDDrawSurfaceMgr* mgr = OwnerMgr();
    m_width = w;
    m_height = h;
    m_bpp = bpp;
    CDDrawPtrCollections* pool = mgr->m_ptrColl;
    i32 mode = 0x11;
    if (w <= 0x140) {
        mode = 0x51;
    }
    i32 hr;
    if (mgr->m_flags & 0x10) {

        // DirectDrawCreate takes its two emulation selectors AS the lpGUID.
        AddrWord<GUID> emulationOnly;
        emulationOnly.m_word = DDCREATE_EMULATIONONLY;
        hr = pool->CreateDevice(mgr->m_hWnd, emulationOnly.m_addr, w, h, bpp, mode);
    } else {
        hr = pool->CreateDevice(mgr->m_hWnd, NULL, w, h, bpp, mode);
    }
    if (hr == 0) {
        DDrawDeviceError err = pool->m_lastError;
        if (err != DDRAWERR_NONE) {
            switch (err) {
                case DDRAWERR_CREATE: {
                    CDDrawSurfaceMgr* m = OwnerMgr();
                    if (m->m_lastError == WORLDERR_NONE) {
                        m->m_lastError = WORLDERR_DDRAW_CREATE;
                    }
                    return 0;
                }
                case DDRAWERR_COOPERATIVE_LEVEL: {
                    CDDrawSurfaceMgr* m = OwnerMgr();
                    if (m->m_lastError == WORLDERR_NONE) {
                        m->m_lastError = WORLDERR_DDRAW_COOPERATIVE_LEVEL;
                    }
                    return 0;
                }
                case DDRAWERR_CAPABILITIES: {
                    CDDrawSurfaceMgr* m = OwnerMgr();
                    if (m->m_lastError == WORLDERR_NONE) {
                        m->m_lastError = WORLDERR_DDRAW_CAPABILITIES;
                    }
                    return 0;
                }
                case DDRAWERR_DISPLAY_MODE: {
                    CDDrawSurfaceMgr* m = OwnerMgr();
                    if (m->m_lastError == WORLDERR_NONE) {
                        m->m_lastError = WORLDERR_DDRAW_DISPLAY_MODE;
                    }
                    return 0;
                }
                case DDRAWERR_COLOR_MASKS: {
                    CDDrawSurfaceMgr* m = OwnerMgr();
                    if (m->m_lastError == WORLDERR_NONE) {
                        m->m_lastError = WORLDERR_DDRAW_COLOR_MASKS;
                    }
                    return 0;
                }
            }
        }
        CDDrawSurfaceMgr* md = OwnerMgr();
        if (md->m_lastError == WORLDERR_NONE) {
            md->m_lastError = WORLDERR_CREATE_DEVICE;
        }
        return 0;
    }
    CDDrawSurfaceMgr* m2 = OwnerMgr();
    i32 amode = 1;
    if (m2->m_flags & 2) {
        amode = 2;
    }
    CDDSurface* surf = pool->Create24BitPaletteSurface(amode);
    m_surface = surf;
    if (surf != NULL && surf->IsValid()) {
        return 1;
    }
    CDDrawSurfaceMgr* m3 = OwnerMgr();
    if (m3->m_lastError == WORLDERR_NONE) {
        m3->m_lastError = WORLDERR_CREATE_PALETTE_SURFACE;
    }
    return 0;
}

RVA(0x00164650, 0x3)
void CDDrawSurfacePair::BlitDirtyRect(CDDrawSurfacePair* other, i32* pos, i32* size) {}

RVA(0x00164660, 0x46)
i32 CDrawSubWorker::Probe() {
    CDDSurface* s = m_surface;
    if (s != NULL) {
        IDirectDrawSurface* dd = s->m_ddSurface;
        if (dd == NULL || dd->IsLost() != 0) {
            s = m_surface;
            if (s->m_ddSurface->Restore() != 0) {
                s = m_surface;
                if (s->m_ddSurface->Restore() != 0) {
                    return 0;
                }
            }
        }
    }
    return 1;
}

RVA(0x001646b0, 0xde)
i32 CDDrawSurfaceChildA::SetGeom(i32 w, i32 h, ColorDepth bpp) {
    if (m_width == w && m_height == h && m_bpp == bpp) {
        return 1;
    }
    CDDrawPtrCollections* pool = OwnerMgr()->m_ptrColl;
    if (pool == NULL) {
        return 0;
    }
    pool->RemoveItemA(m_surface);
    m_surface = NULL;
    if (pool->ConfigureSurface(w, h, bpp, 0, 0) != BPP_UNSET) {
        return 0;
    }
    i32 amode = 1;
    if (OwnerMgr()->m_flags & 2) {
        amode = 2;
    }
    m_surface = pool->Create24BitPaletteSurface(amode);
    if (m_surface == NULL) {
        return 0;
    }
    if (!m_surface->IsValid()) {
        return 0;
    }
    if (w > 0 && h > 0
        && (bpp == BPP_PALETTED_8 || bpp == BPP_RGB_16 || bpp == BPP_RGB_24 || bpp == BPP_RGB_32)) {
        m_bpp = bpp;
        m_width = w;
        m_height = h;
        m_srcRect.left = 0;
        m_srcRect.top = 0;
        m_srcRect.right = w;
        m_srcRect.bottom = h;
        return 1;
    }
    return 0;
}

// @early-stop
// retail loads OwnerMgr() into eax right after the zero block, which keeps the
// const 1 un-CSE'd (imm stores for m_flashInterval/m_drawFillCmd, late
// `mov eax,1`); our cl loads the owner into edx after the screenY store and
// CSEs the 1. An owner-local hoist compiles byte-identically; permuter flat.
RVA(0x00164790, 0x41)
i32 CResolveNode::SetPosition(i32 x, i32 y) {
    m_screenX = x;
    m_plotDX = 0;
    m_plotDY = 0;
    m_stateFlags = 0;
    m_flashCountdown = 0;
    m_drawFillArg = NULL;
    m_drawActive = 0;
    m_screenY = y;
    m_flashInterval = 0x32;
    m_drawFillCmd = SHADE_COPY;
    m_level = OwnerMgr()->m_level;
    return 1;
}

RVA(0x001647e0, 0x48)
i32 CResolveNode::Init(
    CDDrawSurfaceMgr* owner,
    i32 field04,
    i32 resolveX,
    i32 resolveY,
    i32 field40,
    i32 field08
) {
    m_ownerCtx = owner;
    m_id = field04;
    m_flags = field08;
    m_drawFillArg = NULL;
    m_drawActive = 0;
    m_drawFillCmd = SHADE_COPY;
    SetPosition(resolveX, resolveY);
    m_stateFlags = field40;
    return 1;
}

// @early-stop
// Two coupled scratch-register swaps, both measured inert against a 3x5 spelling
// matrix (res/out order x logic/obj/result locals): the POSTLOAD lookup chain is
// coloured ecx/edx (retail edx/ecx; same MapLookupById family wall as
// ResolveTarget below), and the SerializeMove call transports param d in eax
// where retail uses edx (vtbl edx vs eax). res-before-out sank the out=0 store
// into retail's slot; the rest is the coupled coloring. Tail rows past the last
// ret are the delinker jump-table artifact.
RVA(0x00164830, 0xec)
i32 AnimWorkerObj::Dispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    if (ar == NULL) {
        return 0;
    }
    switch (mode) {
        case SERIAL_PRESAVE:
            m_targetId = 0;
            if (m_target) {
                m_targetId = m_target->m_objectId;
            }
            break;
        case SERIAL_SAVE:

            if (Save(ar) == 0) {
                return 0;
            }
            break;
        case SERIAL_LOAD:

            if (Load(ar) == 0) {
                return 0;
            }
            break;
        case SERIAL_POSTLOAD:
            if (m_targetId) {
                CMapPtrToPtr* res = &m_ownerCtx->m_childGroup->m_registeredGameObjectsById;
                CWwdGameObject* out = NULL;
                if (MapLookupById(*res, m_targetId, out)) {
                    m_target = out;
                }
            }
            break;
        default:
            break;
    }
    if (m_logic) {
        if (m_logic->SerializeMove(ar, mode, typeId, object) == 0) {
            return 0;
        }
    }
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00164920, 0x35)
i32 AnimWorkerObj::CacheTargetId(void* a) {
    if (a == NULL) {
        return 0;
    }
    m_targetId = 0;
    if (m_target) {
        m_targetId = m_target->m_objectId;
    }
    return 1;
}

RVA(0x00164960, 0x41a)
i32 AnimWorkerObj::Save(CFileMemBase* ar) {
    if (ar == NULL) {
        return 0;
    }
    ar->Write(&m_actKey, sizeof(m_actKey));
    ar->Write(&m_timeDelay, sizeof(m_timeDelay));
    ar->Write(&m_frameDelay, sizeof(m_frameDelay));
    ar->Write(&m_userFlags, sizeof(m_userFlags));
    ar->Write(&m_minX, sizeof(m_minX));
    ar->Write(&m_maxX, sizeof(m_maxX));
    ar->Write(&m_minY, sizeof(m_minY));
    ar->Write(&m_maxY, sizeof(m_maxY));
    ar->Write(&m_pad3c, sizeof(m_pad3c));
    ar->Write(&m_reserved40, sizeof(m_reserved40));
    ar->Write(&m_tweakX, sizeof(m_tweakX));
    ar->Write(&m_tweakY, sizeof(m_tweakY));
    ar->Write(&m_scrollTargetX, sizeof(m_scrollTargetX));
    ar->Write(&m_scrollTargetY, sizeof(m_scrollTargetY));
    ar->Write(&m_pad54, sizeof(m_pad54));
    ar->Write(&m_reserved58, sizeof(m_reserved58));
    ar->Write(&m_reserved5c, sizeof(m_reserved5c));
    ar->Write(&m_reserved60, sizeof(m_reserved60));
    ar->Write(&m_user1, sizeof(m_user1));
    ar->Write(&m_user2, sizeof(m_user2));
    ar->Write(&m_user3, sizeof(m_user3));
    ar->Write(&m_user4, sizeof(m_user4));
    ar->Write(&m_user5, sizeof(m_user5));
    ar->Write(&m_user6, sizeof(m_user6));
    ar->Write(&m_user7, sizeof(m_user7));
    ar->Write(&m_user8, sizeof(m_user8));
    ar->Write(&m_reserved84, sizeof(m_reserved84));
    ar->Write(&m_reserved88, sizeof(m_reserved88));
    ar->Write(&m_reserved8c, sizeof(m_reserved8c));
    ar->Write(&m_reserved90, sizeof(m_reserved90));
    ar->Write(&m_reserved94, sizeof(m_reserved94));
    ar->Write(&m_reserved98, sizeof(m_reserved98));
    ar->Write(&m_reserved9c, sizeof(m_reserved9c));
    ar->Write(&m_reserveda0, sizeof(m_reserveda0));
    ar->Write(&m_reserveda4, sizeof(m_reserveda4));
    ar->Write(&m_reserveda8, sizeof(m_reserveda8));
    ar->Write(&m_reservedac, sizeof(m_reservedac));
    ar->Write(&m_reservedb0, sizeof(m_reservedb0));
    ar->Write(&m_reservedb4, sizeof(m_reservedb4));
    ar->Write(&m_counter, sizeof(m_counter));
    ar->Write(&m_speed, sizeof(m_speed));
    ar->Write(&m_padc0, sizeof(m_padc0));
    ar->Write(&m_reservedc4, sizeof(m_reservedc4));
    ar->Write(&m_width, sizeof(m_width));
    ar->Write(&m_height, sizeof(m_height));
    ar->Write(&m_reservedd0, sizeof(m_reservedd0));
    ar->Write(&m_reservede0, sizeof(m_reservede0));
    ar->Write(&m_userRect1, sizeof(m_userRect1));
    ar->Write(&m_userRect2, sizeof(m_userRect2));
    ar->Write(&m_pad110, sizeof(m_pad110));
    ar->Write(&m_reserved120, sizeof(m_reserved120));
    ar->Write(&m_sparkleDelay, sizeof(m_sparkleDelay));
    ar->Write(&m_pad134, sizeof(m_pad134));
    ar->Write(&m_reserved138, sizeof(m_reserved138));
    ar->Write(&m_reserved13c, sizeof(m_reserved13c));
    ar->Write(&m_reserved140, sizeof(m_reserved140));
    ar->Write(&m_reserved144, sizeof(m_reserved144));
    ar->Write(&m_reserved148, sizeof(m_reserved148));
    ar->Write(&m_reserved14c, sizeof(m_reserved14c));
    ar->Write(&m_reserved150, sizeof(m_reserved150));
    ar->Write(&m_reserved154, sizeof(m_reserved154));
    ar->Write(&m_reserved158, sizeof(m_reserved158));
    ar->Write(&m_reserved15c, sizeof(m_reserved15c));
    ar->Write(&m_reserved160, sizeof(m_reserved160));
    ar->Write(&m_reserved164, sizeof(m_reserved164));
    ar->Write(&m_targetId, sizeof(m_targetId));
    ar->Write(&m_payloadSize, sizeof(m_payloadSize));
    u8* payload = m_payload;
    if (payload && m_payloadSize > 0) {
        ar->Write(payload, m_payloadSize);
    }
    return 1;
}

RVA(0x00164d80, 0x421)
i32 AnimWorkerObj::Load(CFileMemBase* ar) {
    if (ar == NULL) {
        return 0;
    }
    ar->Read(&m_actKey, sizeof(m_actKey));
    ar->Read(&m_timeDelay, sizeof(m_timeDelay));
    ar->Read(&m_frameDelay, sizeof(m_frameDelay));
    ar->Read(&m_userFlags, sizeof(m_userFlags));
    ar->Read(&m_minX, sizeof(m_minX));
    ar->Read(&m_maxX, sizeof(m_maxX));
    ar->Read(&m_minY, sizeof(m_minY));
    ar->Read(&m_maxY, sizeof(m_maxY));
    ar->Read(&m_pad3c, sizeof(m_pad3c));
    ar->Read(&m_reserved40, sizeof(m_reserved40));
    ar->Read(&m_tweakX, sizeof(m_tweakX));
    ar->Read(&m_tweakY, sizeof(m_tweakY));
    ar->Read(&m_scrollTargetX, sizeof(m_scrollTargetX));
    ar->Read(&m_scrollTargetY, sizeof(m_scrollTargetY));
    ar->Read(&m_pad54, sizeof(m_pad54));
    ar->Read(&m_reserved58, sizeof(m_reserved58));
    ar->Read(&m_reserved5c, sizeof(m_reserved5c));
    ar->Read(&m_reserved60, sizeof(m_reserved60));
    ar->Read(&m_user1, sizeof(m_user1));
    ar->Read(&m_user2, sizeof(m_user2));
    ar->Read(&m_user3, sizeof(m_user3));
    ar->Read(&m_user4, sizeof(m_user4));
    ar->Read(&m_user5, sizeof(m_user5));
    ar->Read(&m_user6, sizeof(m_user6));
    ar->Read(&m_user7, sizeof(m_user7));
    ar->Read(&m_user8, sizeof(m_user8));
    ar->Read(&m_reserved84, sizeof(m_reserved84));
    ar->Read(&m_reserved88, sizeof(m_reserved88));
    ar->Read(&m_reserved8c, sizeof(m_reserved8c));
    ar->Read(&m_reserved90, sizeof(m_reserved90));
    ar->Read(&m_reserved94, sizeof(m_reserved94));
    ar->Read(&m_reserved98, sizeof(m_reserved98));
    ar->Read(&m_reserved9c, sizeof(m_reserved9c));
    ar->Read(&m_reserveda0, sizeof(m_reserveda0));
    ar->Read(&m_reserveda4, sizeof(m_reserveda4));
    ar->Read(&m_reserveda8, sizeof(m_reserveda8));
    ar->Read(&m_reservedac, sizeof(m_reservedac));
    ar->Read(&m_reservedb0, sizeof(m_reservedb0));
    ar->Read(&m_reservedb4, sizeof(m_reservedb4));
    ar->Read(&m_counter, sizeof(m_counter));
    ar->Read(&m_speed, sizeof(m_speed));
    ar->Read(&m_padc0, sizeof(m_padc0));
    ar->Read(&m_reservedc4, sizeof(m_reservedc4));
    ar->Read(&m_width, sizeof(m_width));
    ar->Read(&m_height, sizeof(m_height));
    ar->Read(&m_reservedd0, sizeof(m_reservedd0));
    ar->Read(&m_reservede0, sizeof(m_reservede0));
    ar->Read(&m_userRect1, sizeof(m_userRect1));
    ar->Read(&m_userRect2, sizeof(m_userRect2));
    ar->Read(&m_pad110, sizeof(m_pad110));
    ar->Read(&m_reserved120, sizeof(m_reserved120));
    ar->Read(&m_sparkleDelay, sizeof(m_sparkleDelay));
    ar->Read(&m_pad134, sizeof(m_pad134));
    ar->Read(&m_reserved138, sizeof(m_reserved138));
    ar->Read(&m_reserved13c, sizeof(m_reserved13c));
    ar->Read(&m_reserved140, sizeof(m_reserved140));
    ar->Read(&m_reserved144, sizeof(m_reserved144));
    ar->Read(&m_reserved148, sizeof(m_reserved148));
    ar->Read(&m_reserved14c, sizeof(m_reserved14c));
    ar->Read(&m_reserved150, sizeof(m_reserved150));
    ar->Read(&m_reserved154, sizeof(m_reserved154));
    ar->Read(&m_reserved158, sizeof(m_reserved158));
    ar->Read(&m_reserved15c, sizeof(m_reserved15c));
    ar->Read(&m_reserved160, sizeof(m_reserved160));
    ar->Read(&m_reserved164, sizeof(m_reserved164));
    ar->Read(&m_targetId, sizeof(m_targetId));
    ar->Read(&m_payloadSize, sizeof(m_payloadSize));
    if (m_payloadSize > 0) {
        m_payload = new u8[m_payloadSize];
        ar->Read(m_payload, m_payloadSize);
    }
    return 1;
}

// @early-stop
// MapLookupById out-param family (same wall as CTriggerMgr::ScanGroup): the two
// scratch registers and the `out = 0` store slot are COUPLED through one knob.
// Precomputing any link of the chain (res/grp/mgr, any decl order, 6 spellings
// measured) keeps the store sunk below the pushes but colours the chain ecx/edx
// (retail edx/ecx); evaluating the chain in-call flips the registers to retail's
// but lifts the store above the pushes. No spelling decouples them; islands and
// depth-1 trees inert.
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001651b0, 0x5d)
i32 AnimWorkerObj::ResolveTarget(void* a) {
    if (a == NULL) {
        return 0;
    }
    if (m_targetId) {
        CMapPtrToPtr* res = &m_ownerCtx->m_childGroup->m_registeredGameObjectsById;
        CWwdGameObject* out = NULL;
        if (!MapLookupById(*res, m_targetId, out)) {
            m_target = NULL;
        } else {
            m_target = out;
        }
    }
    return 1;
}

RVA(0x00165210, 0xa2)
void CDDrawWorkerCache::Unload() {
    CObject* val = NULL;
    POSITION pos = m_workers.GetStartPosition();
    CString key;
    if (pos != NULL) {
        do {
            m_workers.GetNextAssoc(pos, key, val);
            if (val != NULL) {
                delete (static_cast<CDDrawWorker*>(val));
            }
        } while (pos != NULL);
    }
    m_workers.RemoveAll();
}

RVA(0x001652c0, 0x92)
AnimWorkerObj*
CDDrawWorkerCache::CreateWorker(GameObjNotifyFn factory, const char* key, i32 flags) {

    AnimWorkerObj* w = new AnimWorkerObj(OwnerMgr(), m_workers.GetCount());

    if (w->Init(factory, flags) == 0) {
        if (w != NULL) {
            delete w;
        }
        return NULL;
    }
    m_workers[key] = static_cast<CObject*>(w);
    return w;
}

RVA(0x00165360, 0xf1)
CString CDDrawWorkerCache::FindKeyOfValue(CObject* target) {
    CObject* val = NULL;
    POSITION pos = m_workers.GetStartPosition();
    CString key;
    while (pos != NULL) {
        m_workers.GetNextAssoc(pos, key, val);

        if (val != NULL
            && static_cast<AnimWorkerObj*>(val)->m_notify
                   == static_cast<AnimWorkerObj*>(target)->m_notify) {
            return key;
        }
    }
    CString empty;
    return empty;
}

// @early-stop
// head register derivation: retail loads src into ebp, copies ebx, then
// ADVANCES ebp (`mov ebx,ebp; add ebp,0x20`) and folds src->m_flags through
// the cursor ([ebp-0x18]); ours derives the cursor with one lea from ebx.
// Downstream ecx/edx/eax rotation follows. Cursor-init reorder and 250
// generated variants are flat.
#define DELETE_ANI_ELEMENT_CONTENTS(index)                                                         \
    for (index = 0; index < m_records.GetSize(); index++) {                                        \
        CObject* item = m_records.GetAt(index);                                                    \
        if (item != NULL) {                                                                        \
            delete (static_cast<CAniRecordView*>(item));                                           \
        }                                                                                          \
    }                                                                                              \
    if (m_name != NULL) {                                                                          \
        delete[] m_name;                                                                           \
        m_name = NULL;                                                                             \
    }                                                                                              \
    m_records.SetSize(0, -1)

// @early-stop
// Every instruction, offset and constant agrees; both loops and the whole tail from
// the SetAtGrow call to the `ret 0xc` are exact. What differs is which register the
// `src` parameter is materialized into. Retail reads it into EBP one push later
// (`push ebx / push ebp / mov ebp,[esp+0x10]`), copies it to EBX and bumps EBP by
// 0x20, so `src->m_flags` reads as `[ebp-0x18]`; cl reads it into EBX one push
// earlier and derives the cursor with `lea ebp,[ebx+0x20]`, one instruction shorter,
// and then keeps EBX as the base for every `src->` load. Same final assignment
// (EBX = src, EBP = cursor), opposite derivation. Declaring `cursor` as the first
// statement was measured and the prologue does not move - the scheduler absorbs the
// statement order, so the binding is not reachable from this body.
RVA(0x00165460, 0x156)
i32 CAniElement::Build(CDDrawSubMgrLeafScan* ctx, CAniSource* src, i32 flags) {
    m_flags = flags;
    m_scale = 1.0f;
    m_total = 0;
    const char* cursor = src->m_data;
    m_flags = src->m_flags | flags;

    if (src->m_namelen != 0) {
        m_name = new char[src->m_namelen + 2];
        u32 n;
        for (n = 0; n < src->m_namelen; n++) {
            m_name[n] = *cursor++;
        }
        m_name[n] = 0;
    } else {
        m_name = NULL;
    }

    CAniRecordView* rec = NULL;
    i32 i;
    for (i = 0; i < src->m_count; i++) {
        rec = new CAniRecordView;

        Pix16CPtr head;
        head.m_chars = cursor;
        if (rec->Parse(ctx, head.m_swords) == 0) {
            goto fail;
        }
        m_records.SetAtGrow(m_records.GetSize(), static_cast<CObject*>(rec));
        cursor += g_aniParsedNameLen + 0x14;
        m_total += rec->GetSize();
    }
    return 1;

fail:
    if (rec != NULL) {
        delete rec;
    }
    DELETE_ANI_ELEMENT_CONTENTS(i);
    return 0;
}

RVA(0x001655c0, 0x53)
i32 CAniElement::Configure(CDDrawSubMgrLeafScan* ctx, CParseSource* entry, i32 flags) {
    if (entry->GetEntryTag() != REZ_TAG_ANI) {
        return 0;
    }
    m_flags = flags;
    RecordBytes<CAniSource> src;
    src.m_chars = entry->BeginParse();
    if (src.m_chars == NULL) {
        return 0;
    }
    i32 r = Build(ctx, src.m_rec, 0);
    entry->EndParse();
    return r;
}

RVA(0x00165620, 0x101)
i32 CAniElement::LoadFile(CDDrawSubMgrLeafScan* ctx, const char* filename, i32 unused) {
    CFile fr;
    if (fr.Open(filename, 0, NULL) == 0) {
        return 0;
    }
    u32 size = fr.GetLength();
    RecordBytes<CAniSource> source;
    source.m_bytes = new u8[size];
    if (fr.Read(source.m_bytes, size) == 0) {
        delete[] source.m_bytes;
        return 0;
    }
    i32 r = Build(ctx, source.m_rec, 0);
    delete[] source.m_bytes;
    return r;
}

RVA(0x00165730, 0x4c)
void CAniElement::DeleteAll() {
    i32 i;
    DELETE_ANI_ELEMENT_CONTENTS(i);
}

// CAniRecordView header inlines this TU materializes: link.exe kept the
// copies inside ddrawsurfacepair.obj's contribution.
RVA_COMPGEN(0x00165780, 0x1e, ??_GCAniRecordView@@UAEPAXI@Z)
RVA_COMPGEN(0x001657a0, 0x66, ??1CAniRecordView@@UAE@XZ)

RVA(0x00165810, 0xa9)
void CDDrawWorkerMapSmall::Unload() {
    CObject* val = NULL;
    POSITION pos = m_map1.GetStartPosition();
    CString key;
    if (pos != NULL) {
        do {
            m_map1.GetNextAssoc(pos, key, val);
            if (val != NULL) {
                delete (static_cast<CAniRecordBase2*>(val));
            }
        } while (pos != NULL);
    }
    m_map1.RemoveAll();
    m_cachedWorker = NULL;
}

RVA(0x001658c0, 0xcc)
CAniRecordBase2*
CDDrawWorkerMapSmall::LoadPaletteFromSource(CParseSource* src, const char* key, i32 flags) {
    RecordBytes<char> source;
    source.m_chars = src->BeginParse();
    u8* data = source.m_bytes;
    if (data == NULL) {
        return NULL;
    }
    CAniRecordBase2* w = new CAniRecordBase2(m_map1.GetCount(), m_ownerCtx);
    if (w->CreatePaletteFromRgb(data, flags) == 0) {
        src->EndParse();
        if (w != NULL) {
            delete w;
        }
        return NULL;
    }
    src->EndParse();
    char buf[0x50];
    if (key != NULL) {
        strcpy(buf, key);
    } else {
        strcpy(buf, src->m_name);
    }
    m_map1[buf] = static_cast<CObject*>(w);
    return w;
}

RVA(0x00165990, 0x77)
CAniRecordBase2* CDDrawWorkerMapSmall::CreateWorkerFromData(u8* data, const char* key, i32 flags) {
    CAniRecordBase2* w = new CAniRecordBase2(m_map1.GetCount(), m_ownerCtx);
    if (w->CreatePaletteFromRgb(data, flags) == 0) {
        if (w != NULL) {
            delete w;
        }
        return NULL;
    }
    m_map1[key] = static_cast<CObject*>(w);
    return w;
}

RVA(0x00165a10, 0x77)
CAniRecordBase2*
CDDrawWorkerMapSmall::CreateWorkerFromFile(char* path, const char* key, i32 flags) {
    CAniRecordBase2* w = new CAniRecordBase2(m_map1.GetCount(), m_ownerCtx);
    if (w->LoadPaletteFromFile(path, flags) == 0) {
        if (w != NULL) {
            delete w;
        }
        return NULL;
    }
    m_map1[key] = static_cast<CObject*>(w);
    return w;
}

RVA(0x00165a90, 0xf4)
CAniRecordBase2*
CDDrawWorkerMapSmall::LoadSizedPaletteFromSource(CParseSource* src, i32 key, i32 flags) {
    if (src->GetEntryTag() != IMGTAG_XCP) {
        return NULL;
    }
    char* data = src->BeginParse();
    if (data == NULL) {
        return NULL;
    }

    i32 length = static_cast<i32>(src->m_length);
    CAniRecordBase2* w = new CAniRecordBase2(m_map1.GetCount(), m_ownerCtx);
    if (w->CreatePaletteFromTrailingData(data, length, flags) == 0) {
        if (w != NULL) {
            delete w;
        }
        return NULL;
    }

    AddrWord<char> keyArg;
    keyArg.m_word = key;
    char buf[0x50];
    if (keyArg.m_addr != NULL) {
        strcpy(buf, keyArg.m_addr);
    } else {
        strcpy(buf, src->m_name);
    }
    m_map1[buf] = static_cast<CObject*>(w);
    return w;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00165b90, 0xa9)
void CDDrawWorkerMapSmall::ResetSlots() {
    CObject* val = NULL;
    POSITION pos = m_map1.GetStartPosition();
    CString key;
    if (pos != NULL) {
        do {
            m_map1.GetNextAssoc(pos, key, val);
            if (val != NULL) {
                delete (static_cast<CAniRecordBase2*>(val));
            }
        } while (pos != NULL);
    }
    m_map1.RemoveAll();
    m_cachedWorker = NULL;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00165c40, 0xe7)
i32 CDDrawWorkerMapSmall::RemoveByValue(CObject* obj) {
    CAniRecordBase2* w = static_cast<CAniRecordBase2*>(obj);
    if (m_cachedWorker == w) {
        m_cachedWorker = NULL;
    }
    CObject* val = NULL;
    POSITION pos = m_map1.GetStartPosition();
    CString key;
    while (pos != NULL) {
        m_map1.GetNextAssoc(pos, key, val);
        if (val == obj) {
            m_map1.RemoveKey(key);
            if (w != NULL) {
                delete w;
            }
            return 1;
        }
    }
    return 0;
}

static inline CAniRecordBase2* LookupRecord(CMapStringToOb& map, LPCTSTR name) {
    CObject* found = NULL;
    map.Lookup(name, found);
    return static_cast<CAniRecordBase2*>(found);
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00165d30, 0x5f)
i32 CDDrawWorkerMapSmall::RemoveByKey(const char* key) {
    CAniRecordBase2* w = LookupRecord(m_map1, key);
    if (w == NULL) {
        return 0;
    }
    if (m_cachedWorker == w) {
        m_cachedWorker = NULL;
    }
    m_map1.RemoveKey(key);
    delete w;
    return 1;
}

// CAniRecordBase2 header inlines this TU materializes (same mechanism).
RVA_COMPGEN(0x00165db0, 0x1e, ??_GCAniRecordBase2@@UAEPAXI@Z)
RVA_COMPGEN(0x00165dd0, 0x5b, ??1CAniRecordBase2@@UAE@XZ)

RVA(0x00165e30, 0x27)
i32 CFileMemBase::SetName(const char* name, i32 mode, i32 option) {
    m_name = name;
    m_mode = mode;
    m_option = option;
    return 1;
}

RVA(0x00165e60, 0x82)
i32 CFileMem::Open() {
    if (m_name.GetLength() == 0) {
        return 0;
    }

    if (WantRead()) {
        CFile* io = &m_file;
        if (!io->Open(m_name, 0, NULL)) {
            return 0;
        }
        m_length = io->GetLength();
        m_offset = 0;
        return 1;
    }

    CFile* out = &m_file;
    if (!out->Open(m_name, 0x1001, NULL)) {
        return 0;
    }
    m_length = 0;
    m_offset = 0;
    return 1;
}

RVA(0x00165ef0, 0xf)
i32 CFileMem::Ready() {
    CFile* io = &m_file;
    io->Close();
    return 1;
}

RVA(0x00165f00, 0x48)
i32 CFileMem::Read(void* buf, i32 n) {
    if (buf == NULL) {
        return 0;
    }
    if (n == 0) {
        return 0;
    }
    CFile* io = &m_file;
    if (io->Read(buf, n) != static_cast<u32>(n)) {
        return 0;
    }
    m_offset += n;
    return 1;
}

RVA(0x00165f50, 0x45)
i32 CFileMem::Write(const void* buf, i32 n) {
    if (buf == NULL) {
        return 0;
    }
    if (n == 0) {
        return 0;
    }
    CFile* io = &m_file;
    io->Write(buf, n);
    m_length += n;
    m_offset += n;
    return 1;
}

RVA(0x00165fa0, 0x93)
void CDDrawWorkerA::RenderFrame(CDDrawSurfacePair* backBuffer, CDDrawSurfacePair* overlay) {
    {

        char c = m_pixelValue;
        i32 y = m_screenY;
        i32 x = m_screenX;
        CDDSurface* s = overlay->m_surface;
        char* base = static_cast<char*>(s->Lock(0));
        if (base != NULL) {
            base[s->m_bytesPerPixel * x + s->m_pitch * y] = c;
            s->m_ddSurface->Unlock(NULL);
        }
    }
    {
        char c = m_pixelValue;
        i32 y = m_screenY;
        i32 x = m_screenX;
        CDDSurface* s = backBuffer->m_surface;
        char* base = static_cast<char*>(s->Lock(0));
        if (base != NULL) {
            base[s->m_bytesPerPixel * x + y * s->m_pitch] = c;
            s->m_ddSurface->Unlock(NULL);
        }
    }
}

static inline CDDrawWorker* LookupWorker(CMapStringToOb& map, LPCTSTR name) {
    CObject* found = NULL;
    map.Lookup(name, found);
    return static_cast<CDDrawWorker*>(found);
}

RVA(0x00166040, 0x66)
i32 CDDrawWorkerB::ResolveFrame(const char* workerName, i32 frameIndex) {
    CDDrawWorker* p = LookupWorker(OwnerMgr()->m_imageRegistry->m_workersByName, workerName);
    CImage* v = p != NULL ? p->GetAt(frameIndex) : NULL;
    m_frame = v;
    return v != NULL;
}

RVA(0x001660b0, 0x33)
void CDDrawWorkerB::RenderFrame(CDDrawSurfacePair* backBuffer, CDDrawSurfacePair* overlay) {
    m_frame->RenderImage(this, backBuffer);
    if (overlay->m_surface != NULL && (overlay->m_flags & 0x20000) == 0) {
        m_frame->RenderImage(this, overlay);
    }
}
