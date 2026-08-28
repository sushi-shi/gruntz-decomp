#include <rva.h>

#include <DDrawMgr/DDrawSurfacePair.h>

#include <Mfc.h>

#include <AddrWord.h>
#include <DDrawMgr/AniRecord.h>
#include <DDrawMgr/ColorDepth.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawDeviceManager.h>
#include <DDrawMgr/DDrawPaletteRegistry.h>
#include <DDrawMgr/DDrawPlacedWorker.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorker.h>
#include <DDrawMgr/DDrawWorkerCtx.h>
#include <DDrawMgr/DDrawWorkerList.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DDSurface.h>
#include <DDrawMgr/DirectDrawMgr.h>
#include <DDrawMgr/LogicRecord.h>
#include <DDrawMgr/LogicRecordRegistry.h>
#include <Enums.h>
#include <Gruntz/AniElement.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/MapStringToOb.h>
#include <Gruntz/ResolveNode.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/String.h>
#include <Gruntz/UserLogic.h>
#include <Image/ImageSet.h>
#include <Io/FileMem.h>
#include <Pix16.h>
#include <Rez/RezArchiveEntry.h>
#include <Rez/RezTypeTag.h>
#include <Utils/MapTyped.h>

#include <ddraw.h>
#include <stdio.h>
#include <string.h>

RVA(0x00163bc0, 0x2c)
void CDDrawWorkerList::Unload() {
    POSITION pos = m_workers.GetHeadPosition();
    while (pos) {
        CDDrawPlacedWorker* child = static_cast<CDDrawPlacedWorker*>(m_workers.GetNext(pos));
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
        CDDrawPlacedWorker* child = static_cast<CDDrawPlacedWorker*>(m_workers.GetNext(pos));
        child->RenderFrame(backBuffer, overlay);
        child->m_refCount--;
        if ((overlay->m_surface != NULL
             && !HAS(
                 static_cast<DDrawSurfacePairFlags>(overlay->m_flags),
                 SURFACEPAIR_SKIP_OVERLAY_WORKER_RENDER
             ))
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
        CDDrawPlacedWorker* child = static_cast<CDDrawPlacedWorker*>(m_workers.GetNext(pos));
        if (child) {
            delete child;
        }
    }
    m_workers.RemoveAll();
}

// @early-stop
RVA(0x00163c90, 0x116)
i32 CDDrawSurfacePair::Create(i32 w, i32 h, ColorDepth bpp, i32 flags) {
    m_flags = flags;
    if (w <= 0 || h <= 0) {

        if (m_id == IDX(DDRAW_PAGE_BACK)) {
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
    DDrawPageKind kind = static_cast<DDrawPageKind>(m_id);

    m_width = w;
    m_height = h;
    m_bpp = bpp;
    RECT* rect = &m_srcRect;
    rect->left = 0;
    rect->top = 0;
    rect->right = w;
    rect->bottom = h;
    if (kind == DDRAW_PAGE_BACK) {
        CDDrawSurfaceMgr* mgr = OwnerMgr();
        m_surface = mgr->m_deviceManager->WrapAttachedSurface(
            mgr->m_drawTarget->m_frontSurface->m_surface,
            DDSCAPS_BACKBUFFER
        );
        if (m_surface == NULL) {
            if (OwnerMgr()->m_lastError == WORLDERR_NONE) {
                OwnerMgr()->m_lastError = WORLDERR_FRONT_SURFACE_COPY;
            }
            return 0;
        }
    }
    if (m_id != IDX(DDRAW_PAGE_BACK)) {
        if (HAS(static_cast<DDrawSurfacePairFlags>(m_flags), SURFACEPAIR_SYSTEM_MEMORY)) {
            m_surface = OwnerMgr()->m_deviceManager->CreateOffscreenSurface(w, h, BPP_UNSET, 0, -1);
        } else {
            m_surface = OwnerMgr()->m_deviceManager->CreateKeyedSurface(w, h, BPP_UNSET, 0, -1);
        }
        if (m_surface == NULL) {
            if (OwnerMgr()->m_lastError == WORLDERR_NONE) {
                OwnerMgr()->m_lastError = WORLDERR_BACK_SURFACE_CREATE;
            }
            return 0;
        }
    }
    m_ownsSurface = true;
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
    m_ownsSurface = false;
    return 1;
}

RVA(0x00163e20, 0x2d)
void CDDrawSurfacePair::Unload() {
    if (m_surface != NULL && m_ownsSurface != false) {
        CDDrawDeviceManager* manager = OwnerMgr()->m_deviceManager;
        manager->RemoveSurface(m_surface);
        m_surface = NULL;
    }
    m_width = 0;
}

RVA(0x00163e50, 0x8b)
i32 CDDrawSurfacePair::LoadImage(CRezArchiveEntry* src) {
    BEGIN_FILE_IMAGE_PARSE(src, type, buf)
    i32 r = m_surface->Resolve(OwnerMgr()->m_deviceManager, buf, type, src->m_size, 0);
    src->ReleaseData();
    return r;
}

RVA(0x00163ee0, 0x19)
i32 CDDrawSurfacePair::ResolveImageName(char* name) {
    return m_surface->MakeImageKey(OwnerMgr()->m_deviceManager, name, 0);
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
    char* base = static_cast<char*>(m_surface->Lock(NULL));
    if (base == NULL) {
        return;
    }

    u8 c = static_cast<u8>(color);
    i32 left = rect->left;
    i32 w = rect->right - left + 1;

    CDDSurface* surface = m_surface;
    if (m_bpp == BPP_RGB_16) {
        i32 offTop = surface->m_pitch * rect->top + surface->m_bytesPerPixel * left;
        i32 n = 2 * w;
        if (n > 0) {
            memset(base + offTop, color, n);
        }
        CDDSurface* bottomSurface = m_surface;
        i32 offBot =
            bottomSurface->m_pitch * rect->bottom + bottomSurface->m_bytesPerPixel * rect->left;
        if (n > 0) {
            memset(base + offBot, color, n);
        }
    } else {
        i32 offTop = surface->m_pitch * rect->top + surface->m_bytesPerPixel * left;
        if (w > 0) {
            memset(base + offTop, color, w);
        }
        CDDSurface* bottomSurface = m_surface;
        i32 offBot =
            bottomSurface->m_pitch * rect->bottom + bottomSurface->m_bytesPerPixel * rect->left;
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
    char* base = static_cast<char*>(m_surface->Lock(NULL));
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
                sysmem = DDSCAPS_SYSTEMMEMORY & caps.dwCaps;
            } else {
                sysmem = 0;
            }
        }
        OwnerMgr()->m_deviceManager->RemoveSurface(m_surface);
        m_surface = NULL;
        if (static_cast<DDrawPageKind>(m_id) == DDRAW_PAGE_BACK) {
            CDDrawSurfaceMgr* mgr = OwnerMgr();
            m_surface = mgr->m_deviceManager->WrapAttachedSurface(
                mgr->m_drawTarget->m_frontSurface->m_surface,
                DDSCAPS_BACKBUFFER
            );
            if (m_surface == NULL) {
                return 0;
            }
        }
        if (m_id != IDX(DDRAW_PAGE_BACK)) {
            if (sysmem != 0) {
                m_surface = OwnerMgr()->m_deviceManager->CreateOffscreenSurface(w, h, bpp, 0, -1);
            } else {
                m_surface = OwnerMgr()->m_deviceManager->CreateKeyedSurface(w, h, bpp, 0, -1);
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
    DrawTextA(hdc, buf, strlen(buf), rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
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
    DrawTextA(hdc, text, strlen(text), rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    w->m_ddSurface->ReleaseDC(hdc);
}

RVA(0x001644a0, 0x1b0)
i32 CDDrawFrontSurface::SetGeometry(i32 w, i32 h, ColorDepth bpp) {
    CDDrawSurfaceMgr* surfaceManager = OwnerMgr();
    m_width = w;
    m_height = h;
    m_bpp = bpp;
    CDDrawDeviceManager* deviceManager = surfaceManager->m_deviceManager;
    i32 mode = DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE;
    if (w <= 0x140) {
        mode |= DDSCL_ALLOWMODEX;
    }
    i32 hr;
    if (HAS(static_cast<DDrawSurfaceMgrFlags>(surfaceManager->m_flags),
            SURFACEMGR_EMULATION_ONLY)) {

        AddrWord<GUID> emulationOnly;
        emulationOnly.m_word = DDCREATE_EMULATIONONLY;
        hr = deviceManager
                 ->CreateDevice(surfaceManager->m_hWnd, emulationOnly.m_addr, w, h, bpp, mode);
    } else {
        hr = deviceManager->CreateDevice(surfaceManager->m_hWnd, NULL, w, h, bpp, mode);
    }
    if (hr == 0) {
        DDrawDeviceError err = deviceManager->m_lastError;
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
                default: {
                    CDDrawSurfaceMgr* m = OwnerMgr();
                    if (m->m_lastError == WORLDERR_NONE) {
                        m->m_lastError = WORLDERR_CREATE_DEVICE;
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
    if (HAS(static_cast<DDrawSurfaceMgrFlags>(m2->m_flags), SURFACEMGR_TRIPLE_BUFFER)) {
        amode = 2;
    }
    CDDSurface* surf = deviceManager->Create24BitPrimarySurface(amode);
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
i32 CDDrawFrontSurface::SetGeom(i32 w, i32 h, ColorDepth bpp) {
    if (m_width == w && m_height == h && m_bpp == bpp) {
        return 1;
    }
    CDDrawDeviceManager* manager = OwnerMgr()->m_deviceManager;
    if (manager == NULL) {
        return 0;
    }
    manager->RemoveSurface(m_surface);
    m_surface = NULL;
    if (manager->ConfigureSurface(w, h, bpp, 0, 0) != BPP_UNSET) {
        return 0;
    }
    i32 amode = 1;
    if (HAS(static_cast<DDrawSurfaceMgrFlags>(OwnerMgr()->m_flags), SURFACEMGR_TRIPLE_BUFFER)) {
        amode = 2;
    }
    m_surface = manager->Create24BitPrimarySurface(amode);
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

static inline void ResetResolveDrawFill(CResolveNode* node) {
    node->m_drawFillArg = NULL;
    node->m_drawFillCmd = SHADE_COPY;
    node->m_drawActive = false;
}

RVA(0x00164790, 0x41)
i32 CResolveNode::SetPosition(i32 x, i32 y) {
    m_screenX = x;
    m_plotDX = 0;
    m_plotDY = 0;
    m_stateFlags = SPRITE_STATE_NONE;
    m_flashCountdown = 0;
    m_screenY = y;
    m_flashInterval = 0x32;
    ResetResolveDrawFill(this);
    m_level = OwnerMgr()->m_level;
    return 1;
}

RVA(0x001647e0, 0x48)
i32 CResolveNode::Init(
    CDDrawSurfaceMgr* owner,
    i32 id,
    i32 resolveX,
    i32 resolveY,
    GZ_ENUM_PARAM(SpriteStateFlags, i32) stateFlags,
    i32 flags
) {
    m_ownerCtx = owner;
    m_id = id;
    m_flags = flags;
    ResetResolveDrawFill(this);
    SetPosition(resolveX, resolveY);
    m_stateFlags = stateFlags;
    return 1;
}

// @early-stop
RVA(0x00164830, 0xec)
i32 CLogicRecord::SerializeDispatch(
    CFileMemBase* archive,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    if (archive == NULL) {
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

            if (Save(archive) == 0) {
                return 0;
            }
            break;
        case SERIAL_LOAD:

            if (Load(archive) == 0) {
                return 0;
            }
            break;
        case SERIAL_POSTLOAD:
            if (m_targetId) {
                CMapPtrToPtr* objectsById = &m_ownerCtx->m_childGroup->m_registeredGameObjectsById;
                CWwdGameObject* target = NULL;
                if (MapLookupById(*objectsById, m_targetId, target)) {
                    m_target = target;
                }
            }
            break;
        default:
            break;
    }
    if (m_userLogic) {
        if (m_userLogic->SerializeDispatch(archive, mode, typeId, object) == 0) {
            return 0;
        }
    }
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00164920, 0x35)
i32 CLogicRecord::CacheTargetId(void* context) {
    if (context == NULL) {
        return 0;
    }
    m_targetId = 0;
    if (m_target) {
        m_targetId = m_target->m_objectId;
    }
    return 1;
}

RVA(0x00164960, 0x41a)
i32 CLogicRecord::Save(CFileMemBase* ar) {
    if (ar == NULL) {
        return 0;
    }
    ar->Write(&m_eventCode, sizeof(m_eventCode));
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
i32 CLogicRecord::Load(CFileMemBase* ar) {
    if (ar == NULL) {
        return 0;
    }
    ar->Read(&m_eventCode, sizeof(m_eventCode));
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
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001651b0, 0x5d)
i32 CLogicRecord::ResolveTarget(void* context) {
    if (context == NULL) {
        return 0;
    }
    if (m_targetId) {
        CMapPtrToPtr* objectsById = &m_ownerCtx->m_childGroup->m_registeredGameObjectsById;
        CWwdGameObject* target = NULL;
        if (!MapLookupById(*objectsById, m_targetId, target)) {
            m_target = NULL;
        } else {
            m_target = target;
        }
    }
    return 1;
}

RVA(0x00165210, 0xa2)
void CLogicRecordRegistry::Unload() {
    CObject* value = NULL;
    POSITION pos = m_templatesByName.GetStartPosition();
    CString key;
    if (pos != NULL) {
        do {
            m_templatesByName.GetNextAssoc(pos, key, value);
            if (value != NULL) {
                delete static_cast<CLogicRecord*>(value);
            }
        } while (pos != NULL);
    }
    m_templatesByName.RemoveAll();
}

RVA(0x001652c0, 0x92)
CLogicRecord* CLogicRecordRegistry::RegisterLogicType(
    LogicRecordDispatchFn dispatch,
    const char* key,
    i32 flags
) {

    CLogicRecord* record = new CLogicRecord(OwnerMgr(), m_templatesByName.GetCount());

    if (record->Init(dispatch, flags) == 0) {
        if (record != NULL) {
            delete record;
        }
        return NULL;
    }
    m_templatesByName[key] = static_cast<CObject*>(record);
    return record;
}

RVA(0x00165360, 0xf1)
CString CLogicRecordRegistry::FindLogicTypeKey(CLogicRecord* record) {
    CObject* value = NULL;
    POSITION pos = m_templatesByName.GetStartPosition();
    CString key;
    while (pos != NULL) {
        m_templatesByName.GetNextAssoc(pos, key, value);

        if (value != NULL && static_cast<CLogicRecord*>(value)->m_dispatch == record->m_dispatch) {
            return key;
        }
    }
    CString empty;
    return empty;
}

// @early-stop
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
RVA(0x00165460, 0x156)
i32 CAniElement::Build(SoundCueRegistry* ctx, CAniSource* src, i32 flags) {
    m_flags = flags;
    m_scale = 1.0f;
    m_durationMs = 0;
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
        m_durationMs += rec->GetDurationMs();
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
i32 CAniElement::Configure(SoundCueRegistry* ctx, CRezArchiveEntry* entry, i32 flags) {
    if (entry->GetTypeTag() != REZ_TAG_ANI) {
        return 0;
    }
    m_flags = flags;
    RecordBytes<CAniSource> src;
    src.m_chars = entry->LoadData();
    if (src.m_chars == NULL) {
        return 0;
    }
    i32 r = Build(ctx, src.m_rec, 0);
    entry->ReleaseData();
    return r;
}

RVA(0x00165620, 0x101)
i32 CAniElement::LoadFile(SoundCueRegistry* ctx, const char* filename, i32 unused) {
    CFile fr;
    if (fr.Open(filename, CFile::modeRead, NULL) == false) {
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

RVA_COMPGEN(0x00165780, 0x1e, ??_GCAniRecordView@@UAEPAXI@Z)
RVA_COMPGEN(0x001657a0, 0x66, ??1CAniRecordView@@UAE@XZ)

RVA(0x00165810, 0xa9)
void CDDrawPaletteRegistry::Unload() {
    CObject* val = NULL;
    POSITION pos = m_palettesByName.GetStartPosition();
    CString key;
    if (pos != NULL) {
        do {
            m_palettesByName.GetNextAssoc(pos, key, val);
            if (val != NULL) {
                delete (static_cast<CDDrawPaletteResource*>(val));
            }
        } while (pos != NULL);
    }
    m_palettesByName.RemoveAll();
    m_activePalette = NULL;
}

RVA(0x001658c0, 0xcc)
CDDrawPaletteResource*
CDDrawPaletteRegistry::LoadPaletteFromSource(CRezArchiveEntry* src, const char* key, i32 flags) {
    RecordBytes<char> source;
    source.m_chars = src->LoadData();
    u8* data = source.m_bytes;
    if (data == NULL) {
        return NULL;
    }
    CDDrawPaletteResource* w = new CDDrawPaletteResource(m_palettesByName.GetCount(), m_ownerCtx);
    if (w->CreatePaletteFromRgb(data, flags) == 0) {
        src->ReleaseData();
        if (w != NULL) {
            delete w;
        }
        return NULL;
    }
    src->ReleaseData();
    char buf[0x50];
    if (key != NULL) {
        strcpy(buf, key);
    } else {
        strcpy(buf, src->m_name);
    }
    m_palettesByName[buf] = static_cast<CObject*>(w);
    return w;
}

RVA(0x00165990, 0x77)
CDDrawPaletteResource*
CDDrawPaletteRegistry::CreatePaletteFromRgb(u8* data, const char* key, i32 flags) {
    CDDrawPaletteResource* w = new CDDrawPaletteResource(m_palettesByName.GetCount(), m_ownerCtx);
    if (w->CreatePaletteFromRgb(data, flags) == 0) {
        if (w != NULL) {
            delete w;
        }
        return NULL;
    }
    m_palettesByName[key] = static_cast<CObject*>(w);
    return w;
}

RVA(0x00165a10, 0x77)
CDDrawPaletteResource*
CDDrawPaletteRegistry::LoadPaletteFromFile(char* path, const char* key, i32 flags) {
    CDDrawPaletteResource* w = new CDDrawPaletteResource(m_palettesByName.GetCount(), m_ownerCtx);
    if (w->LoadPaletteFromFile(path, flags) == 0) {
        if (w != NULL) {
            delete w;
        }
        return NULL;
    }
    m_palettesByName[key] = static_cast<CObject*>(w);
    return w;
}

RVA(0x00165a90, 0xf4)
CDDrawPaletteResource*
CDDrawPaletteRegistry::LoadPaletteFromTrailingData(CRezArchiveEntry* src, i32 key, i32 flags) {
    if (src->GetTypeTag() != IMGTAG_XCP) {
        return NULL;
    }
    char* data = src->LoadData();
    if (data == NULL) {
        return NULL;
    }

    i32 length = static_cast<i32>(src->m_size);
    CDDrawPaletteResource* w = new CDDrawPaletteResource(m_palettesByName.GetCount(), m_ownerCtx);
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
    m_palettesByName[buf] = static_cast<CObject*>(w);
    return w;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00165b90, 0xa9)
void CDDrawPaletteRegistry::ClearPalettes() {
    CObject* val = NULL;
    POSITION pos = m_palettesByName.GetStartPosition();
    CString key;
    if (pos != NULL) {
        do {
            m_palettesByName.GetNextAssoc(pos, key, val);
            if (val != NULL) {
                delete (static_cast<CDDrawPaletteResource*>(val));
            }
        } while (pos != NULL);
    }
    m_palettesByName.RemoveAll();
    m_activePalette = NULL;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00165c40, 0xe7)
i32 CDDrawPaletteRegistry::RemovePalette(CObject* obj) {
    CDDrawPaletteResource* w = static_cast<CDDrawPaletteResource*>(obj);
    if (m_activePalette == w) {
        m_activePalette = NULL;
    }
    CObject* val = NULL;
    POSITION pos = m_palettesByName.GetStartPosition();
    CString key;
    while (pos != NULL) {
        m_palettesByName.GetNextAssoc(pos, key, val);
        if (val == obj) {
            m_palettesByName.RemoveKey(key);
            if (w != NULL) {
                delete w;
            }
            return 1;
        }
    }
    return 0;
}

static inline CDDrawPaletteResource* LookupRecord(CMapStringToOb& map, LPCTSTR name) {
    CObject* found = NULL;
    map.Lookup(name, found);
    return static_cast<CDDrawPaletteResource*>(found);
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00165d30, 0x5f)
i32 CDDrawPaletteRegistry::RemovePaletteByName(const char* key) {
    CDDrawPaletteResource* w = LookupRecord(m_palettesByName, key);
    if (w == NULL) {
        return 0;
    }
    if (m_activePalette == w) {
        m_activePalette = NULL;
    }
    m_palettesByName.RemoveKey(key);
    delete w;
    return 1;
}

RVA_COMPGEN(0x00165db0, 0x1e, ??_GCDDrawPaletteResource@@UAEPAXI@Z)
RVA_COMPGEN(0x00165dd0, 0x5b, ??1CDDrawPaletteResource@@UAE@XZ)

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
        if (!io->Open(m_name, CFile::modeRead, NULL)) {
            return 0;
        }
        m_length = io->GetLength();
        m_offset = 0;
        return 1;
    }

    CFile* out = &m_file;
    if (!out->Open(m_name, CFile::modeCreate | CFile::modeWrite, NULL)) {
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
void CDDrawPixelWorker::RenderFrame(CDDrawSurfacePair* backBuffer, CDDrawSurfacePair* overlay) {
    {

        char c = m_pixelValue;
        i32 y = m_screenY;
        i32 x = m_screenX;
        CDDSurface* s = overlay->m_surface;
        char* base = static_cast<char*>(s->Lock(NULL));
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
        char* base = static_cast<char*>(s->Lock(NULL));
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
i32 CDDrawFrameWorker::ResolveFrame(const char* workerName, i32 frameIndex) {
    CDDrawWorker* p = LookupWorker(OwnerMgr()->m_imageRegistry->m_workersByName, workerName);
    CImage* v = p != NULL ? p->GetAt(frameIndex) : NULL;
    m_frame = v;
    return v != NULL;
}

RVA(0x001660b0, 0x33)
void CDDrawFrameWorker::RenderFrame(CDDrawSurfacePair* backBuffer, CDDrawSurfacePair* overlay) {
    m_frame->RenderImage(this, backBuffer);
    if (overlay->m_surface != NULL
        && !HAS(
            static_cast<DDrawSurfacePairFlags>(overlay->m_flags),
            SURFACEPAIR_SKIP_OVERLAY_WORKER_RENDER
        )) {
        m_frame->RenderImage(this, overlay);
    }
}
