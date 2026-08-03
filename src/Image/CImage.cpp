#define CIMAGE_INLINE_DTOR

#include <rva.h>

#include <Image/CImage.h>

#include <Mfc.h>

#include <Bute/SymTab.h>
#include <DDrawMgr/DDrawPtrCollections.h>
#include <DDrawMgr/DDrawShadeBlit.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDSurface.h>
#include <Enums.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/ParseSource.h>
#include <Gruntz/ResolveNode.h>
#include <Gruntz/State.h>
#include <Pix16.h>
#include <Rez/FrameClock.h>
#include <Rez/RezTypeTag.h>
#include <Wwd/WwdFile.h>

#include <ddraw.h>
#include <stdio.h>

DATA(0x002bf28c)
i32 g_imageClipRect[4] = {0};
DATA(0x002bf318)
DDBLTFX g_bltFx = {0};
DATA(0x002bf37c)
i32 g_resourceInstallActive = 0;
DATA(0x002bf380)
i32 g_surfaceColorKey = 0;
VTBL(CImage, 0x001eaa2c);

// @identity-TODO DrawScreenTextImage@CState - thunk oracle: retail gave this an incremental
// thunk, so it was compiled into a LINK-LINE OBJECT, while the rest of this TU
// (20 fns) came from the static library. It belongs to another compiland.
RVA(0x00152e90, 0x8b)
i32 CImage::Create(char* path, i32 keyed) {
    i32 flagsArg = (keyed != 0) ? g_surfaceColorKey : -1;
    i32 capArg = 0;
    if (g_resourceInstallActive != 0) {
        capArg = 0x800;
    }
    CDDSurface* item = m_ownerCtx->m_ptrColl->LoadFileSurface(path, capArg, flagsArg);
    m_surface = item;
    if (item == NULL) {
        return 0;
    }

    m_width = item->m_width;
    m_height = item->m_height;
    m_anchorX = m_width >> 1;
    m_anchorY = m_height >> 1;
    if (item->m_hasColorKey != 0) {
        m_loadResult = 0x11;
    } else {
        m_loadResult = 0x10;
    }
    m_originX = 0;
    m_originY = 0;
    return 1;
}

RVA(0x00152f20, 0x86)
i32 CImage::Resolve(CParseSource* src, i32 arg) {
    FileImageFormat index;
    switch (src->GetEntryTag()) {
        case IMGTAG_PMB:
            index = FMT_BMP;
            break;
        case IMGTAG_XCP:
            index = FMT_PCX;
            break;
        case IMGTAG_DIR:
            index = FMT_DIR;
            break;
        case IMGTAG_DIP:
            index = FMT_PID;
            break;
        default:
            return 0;
    }
    char* resolved = src->BeginParse();
    if (resolved == NULL) {
        return 0;
    }

    RecordBytes<PidHeader> blob;
    blob.m_chars = resolved;
    i32 result = this->LoadDispatch(

        static_cast<PidHeader*>(blob.m_rec),
        index,
        src->m_length,
        arg
    );
    src->EndParse();
    return result;
}

RVA(0x00152fb0, 0x123)
i32 CImage::LoadDispatch(PidHeader* desc, FileImageFormat mode, u32 size, i32 keyed) {
    if (mode != FMT_BMP && mode != FMT_PCX && mode != FMT_DIR && mode != FMT_PID) {
        return 0;
    }

    if (mode == PID_SYSTEM_MEMORY && (HAS(desc->flags, PID_GRAMMAR_SKIPRUN))) {
        if (!BuildShadeBlitter(desc, size)) {
            return 0;
        }

        if (m_owned != NULL && (HAS(desc->flags, PID_SRC_8BPP_SHADE))) {
            m_owned->Select(SHADE_DST_BY_SRC, 0);
            return 1;
        }
        return 1;
    }
    i32 flagsArg = (keyed != 0) ? g_surfaceColorKey : -1;
    if (mode == FMT_PID || mode == FMT_DIR) {
        i32 g10 = desc->offsetX;
        i32 g14 = desc->offsetY;
        m_originX = g10;
        m_originY = g14;
    } else {
        m_originX = 0;
        m_originY = 0;
    }
    i32 capArg = 0;
    if (g_resourceInstallActive != 0) {
        capArg = 0x800;
    }

    CDDSurface* item =
        m_ownerCtx->m_ptrColl->LoadSurfaceFromPid(desc, mode, size, capArg, flagsArg);
    m_surface = item;
    if (item == NULL) {
        return 0;
    }
    i32 w = item->m_width;
    m_width = w;
    i32 h = item->m_height;
    m_height = h;
    m_anchorX = w >> 1;
    m_anchorY = h >> 1;
    if (item->m_hasColorKey != 0) {
        m_loadResult = 0x11;
        return 1;
    }
    m_loadResult = 0x10;
    return 1;
}

RVA(0x001530e0, 0x92)
i32 CImage::CreateBlankSurface(i32 width, i32 height, i32 keyed) {
    i32 flagsArg = (keyed != 0) ? g_surfaceColorKey : -1;
    i32 capArg = 0;
    if (g_resourceInstallActive != 0) {
        capArg = 0x800;
    }
    CDDSurface* item =
        m_ownerCtx->m_ptrColl->CreateKeyedSurface(width, height, 0, capArg, flagsArg);
    m_surface = item;
    if (item == NULL) {
        return 0;
    }
    i32 w = item->m_width;
    m_width = w;
    i32 h = item->m_height;
    m_height = h;
    m_anchorX = w >> 1;
    m_anchorY = h >> 1;
    if (item->m_hasColorKey != 0) {
        m_loadResult = 0x11;
    } else {
        m_loadResult = 0x10;
    }
    m_originX = 0;
    m_originY = 0;
    return 1;
}

// @early-stop
RVA(0x00153180, 0xda)
i32 CImage::BuildShadeBlitter(PidHeader* desc, u32 size) {
    CDDrawShadeBlit* owned = new CDDrawShadeBlit();
    m_owned = owned;
    if (owned == NULL) {
        return 0;
    }

    if (!owned->Build(desc, static_cast<i32>(size), m_ownerCtx->m_drawTarget->m_frontPair->m_bpp)) {
        return 0;
    }
    i32 w = m_owned->m_width;
    m_width = w;
    i32 h = m_owned->m_height;
    m_height = h;
    m_loadResult = 0x11;
    m_anchorX = w >> 1;
    m_anchorY = h >> 1;
    m_originX = desc->offsetX;
    m_originY = desc->offsetY;
    return 1;
}

RVA(0x00153260, 0x41)
void CImage::FreeAll() {
    m_width = 0;
    m_height = 0;
    if (m_surface != NULL) {
        m_ownerCtx->m_ptrColl->RemoveItemA(m_surface);
        m_surface = NULL;
    }
    CDDrawShadeBlit* owned = m_owned;
    if (owned != NULL) {
        owned->Teardown();
        ::operator delete(owned);
        m_owned = NULL;
    }
}

RVA(0x001532b0, 0x80)
i32 CImage::CopyFrom(CImage* other) {
    if (other == NULL) {
        return 0;
    }
    if (other->m_owned != NULL) {
        return 0;
    }
    if (m_surface == NULL) {
        return 0;
    }
    if (m_owned != NULL) {
        return 0;
    }
    if (m_width != other->m_width) {
        return 0;
    }
    if (m_height != other->m_height) {
        return 0;
    }
    m_surface->Fill(0);
    i32 ok = m_surface->Blt(other->m_surface);
    return ok != 0;
}

RVA(0x00153330, 0x36)
i32 CImage::SetOrigin(PidHeader* desc, i32 mode) {
    if (mode == 4 || mode == 3) {
        i32 oy = desc->offsetY;
        i32 ox = desc->offsetX;
        m_originX = ox;
        m_originY = oy;
    } else {
        m_originX = 0;
        m_originY = 0;
    }
    return 1;
}

RVA(0x00153370, 0xf)
void CImage::FlipVertical(void*) {
    if (m_surface) {
        m_surface->FlipVertical();
    }
}

RVA(0x00153380, 0xeb)
i32 CImage::Reload(CParseSource* src, i32 arg) {

    CDDSurface* surf = m_surface;
    if (surf == NULL) {
        return 1;
    }
    IDirectDrawSurface* s = surf->m_ddSurface;
    if (s != NULL) {
        if (s->IsLost() == 0) {
            return 1;
        }
    }
    surf = m_surface;
    if (surf->m_ddSurface->Restore() != 0) {
        this->FreeAll();
        return this->Resolve(src, arg);
    }

    FileImageFormat index;
    switch (src->GetEntryTag()) {
        case IMGTAG_PMB:
            index = FMT_BMP;
            break;
        case IMGTAG_XCP:
            index = FMT_PCX;
            break;
        case IMGTAG_DIR:
            index = FMT_DIR;
            break;
        case IMGTAG_DIP:
            index = FMT_PID;
            break;
        default:
            return 0;
    }
    char* resolved = src->BeginParse();
    if (resolved == NULL) {
        return 0;
    }
    if (src->m_length == 0) {
        return 0;
    }

    return m_surface->Resolve(
        m_ownerCtx->m_ptrColl,
        resolved,
        index,
        static_cast<u32>(src->m_length),
        g_surfaceColorKey
    );
}

// @early-stop
RVA(0x00153470, 0x31a)
void CImage::RenderImage(CResolveNode* info, CDDrawSurfacePair* dst) {
    i32 mode = info->m_stateFlags;
    if (mode & 1) {
        info->m_dirty.m_armed = -1;
        return;
    }
    if (mode & 8) {
        if (g_engineFrameDelta >= info->m_flashCountdown) {
            info->m_flashCountdown = info->m_flashInterval;
            mode ^= 0x10000000;
            info->m_stateFlags = mode;
        } else {
            info->m_flashCountdown -= g_engineFrameDelta;
        }
        mode = info->m_stateFlags;
        if (!(mode & 0x10000000)) {
            info->m_dirty.m_armed = -1;
            return;
        }
    }
    i32 hFlip = mode & 4;
    i32 vFlip = mode & 2;
    if (vFlip) {
        if (hFlip) {
            if (m_owned) {
                BlitShadeNorm(info, dst);
            } else {
                BlitNorm(info, dst);
            }
        } else {
            if (m_owned) {
                BlitShadeFlipV(info, dst);
            } else {
                BlitFlipV(info, dst);
            }
        }
        return;
    }
    if (hFlip) {
        if (m_owned) {
            BlitShadeFlipH(info, dst);
        } else {
            BlitFlipH(info, dst);
        }
        return;
    }
    if (m_owned) {
        BlitShadeFlipHV(info, dst);
        return;
    }

    LONG x = m_originX - m_anchorX + info->m_plotDX + info->m_screenX;
    LONG y = m_originY - m_anchorY + info->m_plotDY + info->m_screenY;
    if (info->m_flags & 0x40000) {
        info->m_level->m_mainPlane->WrapCoord(&x, &y);
    }
    i32 right = m_width + x - 1;
    i32 bottom = m_height + y - 1;
    i32 dleft = x;
    i32 dtop = y;
    i32 dright = right;
    i32 dbottom = bottom;
    if (info->m_flags & 0x40000) {
        BlitRect srcClip = m_ownerCtx->m_level->m_planeCtx;
        RECT destClip;
        CopyRect(&destClip, static_cast<const RECT*>(&srcClip));
        if (x < destClip.left) {
            dleft += destClip.left - x;
        }
        if (right > destClip.right) {
            dright = destClip.right;
        }
        if (y < destClip.top) {
            dtop += destClip.top - y;
        }
        if (bottom > destClip.bottom) {
            dbottom = destClip.bottom;
        }
    } else if (info->m_clip.left == static_cast<i32>(0x80000000)) {
        if (x < 0) {
            dleft = 0;
        }
        if (right >= dst->m_width) {
            dright = dst->m_width - 1;
        }
        if (y < 0) {
            dtop = 0;
        }
        if (bottom >= dst->m_height) {
            dbottom = dst->m_height - 1;
        }
    } else {
        if (x < info->m_clip.left) {
            dleft = info->m_clip.left;
        }
        if (right > info->m_clip.right) {
            dright = info->m_clip.right;
        }
        if (y < info->m_clip.top) {
            dtop = info->m_clip.top;
        }
        if (bottom > info->m_clip.bottom) {
            dbottom = info->m_clip.bottom;
        }
    }
    i32 w = dright - dleft + 1;
    i32 h = dbottom - dtop + 1;
    if (w <= 0 || h <= 0) {
        info->m_dirty.m_armed = -1;
        return;
    }
    RECT s;
    s.left = dleft - x;
    s.top = dtop - y;
    s.right = s.left + w;
    s.bottom = s.top + h;
    dst->m_surface->BltFast(dleft, dtop, m_surface, &s, m_loadResult);
    info->m_dirty.m_lastX = dleft;
    info->m_dirty.m_rect.left = dleft;
    info->m_dirty.m_lastY = dtop;
    info->m_dirty.m_w = w;
    info->m_dirty.m_rect.top = dtop;
    info->m_dirty.m_h = h;
    info->m_dirty.m_armed = 0;
    info->m_dirty.m_rect.right = dright;
    info->m_dirty.m_rect.bottom = dbottom;
}

RVA(0x00153790, 0x6a)
void CImage::RenderFrame(CDDrawSurfacePair* target, i32 x, i32 y, i32 flags) {
    static CResolveNode clip;
    if (clip.Init(m_ownerCtx, 0, x, y, flags, 0)) {
        this->RenderImage(&clip, target);
    }
}

RVA(0x00153810, 0x95)
void CImage::RenderFrameClipped(
    CDDrawSurfacePair* target,
    i32 x,
    i32 y,
    RECT* clipRect,
    i32 flags
) {
    static CResolveNode clip;
    if (clip.Init(m_ownerCtx, 0, x, y, flags, 0)) {
        if (clipRect != NULL) {
            g_imageClipRect[0] = clipRect->left;
            g_imageClipRect[1] = clipRect->top;
            g_imageClipRect[2] = clipRect->right;
            g_imageClipRect[3] = clipRect->bottom;
        }
        this->RenderImage(&clip, target);
    }
}

// @early-stop
RVA(0x001538c0, 0x257)
void CImage::BlitNorm(CResolveNode* info, CDDrawSurfacePair* dst) {
    LONG x = info->m_screenX - m_originX - info->m_plotDX - m_anchorX;
    LONG y = info->m_screenY - m_originY - info->m_plotDY - m_anchorY;
    if (info->m_flags & 0x40000) {
        info->m_level->m_mainPlane->WrapCoord(&x, &y);
    }
    i32 right = m_width + x - 1;
    i32 bottom = m_height + y - 1;
    RECT d;
    d.left = x;
    d.top = y;
    d.right = right;
    d.bottom = bottom;
    if (info->m_flags & 0x40000) {
        BlitRect clipA = m_ownerCtx->m_level->m_planeCtx;
        RECT clip;
        CopyRect(&clip, static_cast<const RECT*>(&clipA));
        if (x < clip.left) {
            d.left += clip.left - x;
        }
        if (right > clip.right) {
            d.right += clip.right - right;
        }
        if (y < clip.top) {
            d.top += clip.top - y;
        }
        if (bottom > clip.bottom) {
            d.bottom += clip.bottom - bottom;
        }
    } else if (info->m_clip.left == static_cast<i32>(0x80000000)) {
        if (x < 0) {
            d.left = 0;
        }
        if (right >= dst->m_width) {
            d.right = dst->m_width - 1;
        }
        if (y < 0) {
            d.top = 0;
        }
        if (bottom >= dst->m_height) {
            d.bottom = dst->m_height - 1;
        }
    } else {
        if (x < info->m_clip.left) {
            d.left = info->m_clip.left;
        }
        if (right > info->m_clip.right) {
            d.right = info->m_clip.right;
        }
        if (y < info->m_clip.top) {
            d.top = info->m_clip.top;
        }
        if (bottom > info->m_clip.bottom) {
            d.bottom = info->m_clip.bottom;
        }
    }
    i32 w = d.right - d.left + 1;
    i32 h = d.bottom - d.top + 1;
    if (w <= 0 || h <= 0) {
        info->m_dirty.m_armed = -1;
        return;
    }
    RECT s;
    s.left = right - d.right;
    s.top = bottom - d.bottom;
    s.right = s.left + w;
    s.bottom = s.top + h;
    g_bltFx.dwDDFX = DDBLTFX_MIRRORLEFTRIGHT | DDBLTFX_MIRRORUPDOWN;
    d.right += 1;
    d.bottom += 1;
    dst->m_surface->BltEx(&d, m_surface, &s, 0x8800, &g_bltFx);
    d.right -= 1;
    d.bottom -= 1;
    info->m_dirty.m_lastX = d.left;
    info->m_dirty.m_lastY = d.top;
    info->m_dirty.m_rect = *(&d);
    info->m_dirty.m_w = w;
    info->m_dirty.m_h = h;
    info->m_dirty.m_armed = 0;
}

// @early-stop
RVA(0x00153b20, 0x270)
void CImage::BlitFlipV(CResolveNode* info, CDDrawSurfacePair* dst) {
    LONG x = info->m_screenX - info->m_plotDX - m_anchorX - m_originX;
    LONG y = m_originY - m_anchorY + info->m_plotDY + info->m_screenY;
    if (info->m_flags & 0x40000) {
        info->m_level->m_mainPlane->WrapCoord(&x, &y);
    }
    i32 right = m_width + x - 1;
    i32 bottom = m_height + y - 1;
    RECT d;
    d.left = x;
    d.top = y;
    d.right = right;
    d.bottom = bottom;
    if (info->m_flags & 0x40000) {
        BlitRect clipA = m_ownerCtx->m_level->m_planeCtx;
        RECT clip;
        CopyRect(&clip, static_cast<const RECT*>(&clipA));
        if (x < clip.left) {
            d.left += clip.left - x;
        }
        if (right > clip.right) {
            d.right += clip.right - right;
        }
        if (y < clip.top) {
            d.top += clip.top - y;
        }
        if (bottom > clip.bottom) {
            d.bottom += clip.bottom - bottom;
        }
    } else if (info->m_clip.left == static_cast<i32>(0x80000000)) {
        if (x < 0) {
            d.left = 0;
        }
        if (right >= dst->m_width) {
            d.right = dst->m_width - 1;
        }
        if (y < 0) {
            d.top = 0;
        }
        if (bottom >= dst->m_height) {
            d.bottom = dst->m_height - 1;
        }
    } else {
        if (x < info->m_clip.left) {
            d.left = info->m_clip.left;
        }
        if (right > info->m_clip.right) {
            d.right = info->m_clip.right;
        }
        if (y < info->m_clip.top) {
            d.top = info->m_clip.top;
        }
        if (bottom > info->m_clip.bottom) {
            d.bottom = info->m_clip.bottom;
        }
    }
    i32 w = d.right - d.left + 1;
    i32 h = d.bottom - d.top + 1;
    if (w <= 0 || h <= 0) {
        info->m_dirty.m_armed = -1;
        return;
    }
    RECT s;
    s.left = right - d.right;
    s.top = bottom - d.bottom;
    s.right = s.left + w;
    s.bottom = s.top + h;
    d.right += 1;
    d.bottom += 1;
    g_bltFx.dwDDFX = DDBLTFX_MIRRORLEFTRIGHT;
    dst->m_surface->BltEx(&d, m_surface, &s, 0x8800, &g_bltFx);
    d.right -= 1;
    d.bottom -= 1;
    info->m_dirty.m_lastX = d.left;
    info->m_dirty.m_lastY = d.top;
    info->m_dirty.m_rect = *(&d);
    info->m_dirty.m_w = w;
    info->m_dirty.m_h = h;
    info->m_dirty.m_armed = 0;
}

// @early-stop
RVA(0x00153d90, 0x259)
void CImage::BlitFlipH(CResolveNode* info, CDDrawSurfacePair* dst) {
    LONG x = info->m_plotDX - m_anchorX + m_originX + info->m_screenX;
    LONG y = info->m_screenY - m_originY - m_anchorY - info->m_plotDY;
    if (info->m_flags & 0x40000) {
        info->m_level->m_mainPlane->WrapCoord(&x, &y);
    }
    i32 right = m_width + x - 1;
    i32 bottom = m_height + y - 1;
    RECT d;
    d.left = x;
    d.top = y;
    d.right = right;
    d.bottom = bottom;
    if (info->m_flags & 0x40000) {
        BlitRect clipA = m_ownerCtx->m_level->m_planeCtx;
        RECT clip;
        CopyRect(&clip, static_cast<const RECT*>(&clipA));
        if (x < clip.left) {
            d.left += clip.left - x;
        }
        if (right > clip.right) {
            d.right += clip.right - right;
        }
        if (y < clip.top) {
            d.top += clip.top - y;
        }
        if (bottom > clip.bottom) {
            d.bottom += clip.bottom - bottom;
        }
    } else if (info->m_clip.left == static_cast<i32>(0x80000000)) {
        if (x < 0) {
            d.left = 0;
        }
        if (right >= dst->m_width) {
            d.right = dst->m_width - 1;
        }
        if (y < 0) {
            d.top = 0;
        }
        if (bottom >= dst->m_height) {
            d.bottom = dst->m_height - 1;
        }
    } else {
        if (x < info->m_clip.left) {
            d.left = info->m_clip.left;
        }
        if (right > info->m_clip.right) {
            d.right = info->m_clip.right;
        }
        if (y < info->m_clip.top) {
            d.top = info->m_clip.top;
        }
        if (bottom > info->m_clip.bottom) {
            d.bottom = info->m_clip.bottom;
        }
    }
    i32 w = d.right - d.left + 1;
    i32 h = d.bottom - d.top + 1;
    if (w <= 0 || h <= 0) {
        info->m_dirty.m_armed = -1;
        return;
    }
    RECT s;
    s.left = right - d.right;
    s.top = bottom - d.bottom;
    s.right = s.left + w;
    s.bottom = s.top + h;
    d.right += 1;
    d.bottom += 1;
    g_bltFx.dwDDFX = DDBLTFX_MIRRORUPDOWN;
    dst->m_surface->BltEx(&d, m_surface, &s, 0x8800, &g_bltFx);
    d.right -= 1;
    d.bottom -= 1;
    info->m_dirty.m_lastX = d.left;
    info->m_dirty.m_lastY = d.top;
    info->m_dirty.m_rect = *(&d);
    info->m_dirty.m_w = w;
    info->m_dirty.m_h = h;
    info->m_dirty.m_armed = 0;
}

// @early-stop
RVA(0x00153ff0, 0x280)
void CImage::BlitShadeFlipHV(CResolveNode* info, CDDrawSurfacePair* dst) {
    LONG x = info->m_screenX - m_anchorX + m_originX + info->m_plotDX;
    LONG y = info->m_screenY - m_anchorY + m_originY + info->m_plotDY;
    if (info->m_flags & 0x40000) {
        info->m_level->m_mainPlane->WrapCoord(&x, &y);
    }
    i32 right = m_width + x - 1;
    i32 bottom = m_height + y - 1;
    ShadeRect d;
    d.left = x;
    d.top = y;
    d.right = right;
    d.bottom = bottom;
    if (info->m_flags & 0x40000) {
        BlitRect clipA = m_ownerCtx->m_level->m_planeCtx;
        RECT clip;
        CopyRect(&clip, static_cast<const RECT*>(&clipA));
        if (x < clip.left) {
            d.left += clip.left - x;
        }
        if (right > clip.right) {
            d.right += clip.right - right;
        }
        if (y < clip.top) {
            d.top += clip.top - y;
        }
        if (bottom > clip.bottom) {
            d.bottom += clip.bottom - bottom;
        }
    } else if (info->m_clip.left == static_cast<i32>(0x80000000)) {
        if (x < 0) {
            d.left = 0;
        }
        if (right >= dst->m_width) {
            d.right = dst->m_width - 1;
        }
        if (y < 0) {
            d.top = 0;
        }
        if (bottom >= dst->m_height) {
            d.bottom = dst->m_height - 1;
        }
    } else {
        if (x < info->m_clip.left) {
            d.left = info->m_clip.left;
        }
        if (right > info->m_clip.right) {
            d.right = info->m_clip.right;
        }
        if (y < info->m_clip.top) {
            d.top = info->m_clip.top;
        }
        if (bottom > info->m_clip.bottom) {
            d.bottom = info->m_clip.bottom;
        }
    }
    i32 w = d.right - d.left + 1;
    i32 h = d.bottom - d.top + 1;
    if (w <= 0 || h <= 0) {
        info->m_dirty.m_armed = -1;
        return;
    }
    ShadeRect s;
    s.left = right - d.right;
    s.top = bottom - d.bottom;
    s.right = s.left + w - 1;
    s.bottom = s.top + h - 1;
    if (info->m_drawActive) {
        m_owned->Select(info->m_drawFillCmd, info->m_drawFillArg);
    }
    m_owned->Blit(&d, dst->m_surface, &s, 0, 0);
    info->m_dirty.m_lastX = d.left;
    info->m_dirty.m_lastY = d.top;
    info->m_dirty.m_rect = *(&d);
    info->m_dirty.m_w = w;
    info->m_dirty.m_h = h;
    info->m_dirty.m_armed = 0;
}

// @early-stop
RVA(0x00154270, 0x257)
void CImage::BlitShadeNorm(CResolveNode* info, CDDrawSurfacePair* dst) {
    LONG x = info->m_screenX - m_originX - m_anchorX - info->m_plotDX;
    LONG y = info->m_screenY - m_originY - m_anchorY - info->m_plotDY;
    if (info->m_flags & 0x40000) {
        info->m_level->m_mainPlane->WrapCoord(&x, &y);
    }
    i32 right = m_width + x - 1;
    i32 bottom = m_height + y - 1;
    ShadeRect d;
    d.left = x;
    d.top = y;
    d.right = right;
    d.bottom = bottom;
    if (info->m_flags & 0x40000) {
        BlitRect clipA = m_ownerCtx->m_level->m_planeCtx;
        RECT clip;
        CopyRect(&clip, static_cast<const RECT*>(&clipA));
        if (x < clip.left) {
            d.left += clip.left - x;
        }
        if (right > clip.right) {
            d.right += clip.right - right;
        }
        if (y < clip.top) {
            d.top += clip.top - y;
        }
        if (bottom > clip.bottom) {
            d.bottom += clip.bottom - bottom;
        }
    } else if (info->m_clip.left == static_cast<i32>(0x80000000)) {
        if (x < 0) {
            d.left = 0;
        }
        if (right >= dst->m_width) {
            d.right = dst->m_width - 1;
        }
        if (y < 0) {
            d.top = 0;
        }
        if (bottom >= dst->m_height) {
            d.bottom = dst->m_height - 1;
        }
    } else {
        if (x < info->m_clip.left) {
            d.left = info->m_clip.left;
        }
        if (right > info->m_clip.right) {
            d.right = info->m_clip.right;
        }
        if (y < info->m_clip.top) {
            d.top = info->m_clip.top;
        }
        if (bottom > info->m_clip.bottom) {
            d.bottom = info->m_clip.bottom;
        }
    }
    i32 w = d.right - d.left + 1;
    i32 h = d.bottom - d.top + 1;
    if (w <= 0 || h <= 0) {
        info->m_dirty.m_armed = -1;
        return;
    }
    ShadeRect s;
    s.left = right - d.right;
    s.top = bottom - d.bottom;
    s.right = s.left + w - 1;
    s.bottom = s.top + h - 1;
    if (info->m_drawActive) {
        m_owned->Select(info->m_drawFillCmd, info->m_drawFillArg);
    }
    m_owned->Blit(&d, dst->m_surface, &s, 1, 1);
    info->m_dirty.m_lastX = d.left;
    info->m_dirty.m_lastY = d.top;
    info->m_dirty.m_rect = *(&d);
    info->m_dirty.m_w = w;
    info->m_dirty.m_h = h;
    info->m_dirty.m_armed = 0;
}

// @early-stop
RVA(0x001544d0, 0x275)
void CImage::BlitShadeFlipV(CResolveNode* info, CDDrawSurfacePair* dst) {
    LONG x = info->m_screenX - m_anchorX - info->m_plotDX - m_originX;
    LONG y = m_originY + info->m_plotDY + info->m_screenY - m_anchorY;
    if (info->m_flags & 0x40000) {
        info->m_level->m_mainPlane->WrapCoord(&x, &y);
    }
    i32 right = m_width + x - 1;
    i32 bottom = m_height + y - 1;
    ShadeRect d;
    d.left = x;
    d.top = y;
    d.right = right;
    d.bottom = bottom;
    if (info->m_flags & 0x40000) {
        BlitRect clipA = m_ownerCtx->m_level->m_planeCtx;
        RECT clip;
        CopyRect(&clip, static_cast<const RECT*>(&clipA));
        if (x < clip.left) {
            d.left += clip.left - x;
        }
        if (right > clip.right) {
            d.right += clip.right - right;
        }
        if (y < clip.top) {
            d.top += clip.top - y;
        }
        if (bottom > clip.bottom) {
            d.bottom += clip.bottom - bottom;
        }
    } else if (info->m_clip.left == static_cast<i32>(0x80000000)) {
        if (x < 0) {
            d.left = 0;
        }
        if (right >= dst->m_width) {
            d.right = dst->m_width - 1;
        }
        if (y < 0) {
            d.top = 0;
        }
        if (bottom >= dst->m_height) {
            d.bottom = dst->m_height - 1;
        }
    } else {
        if (x < info->m_clip.left) {
            d.left = info->m_clip.left;
        }
        if (right > info->m_clip.right) {
            d.right = info->m_clip.right;
        }
        if (y < info->m_clip.top) {
            d.top = info->m_clip.top;
        }
        if (bottom > info->m_clip.bottom) {
            d.bottom = info->m_clip.bottom;
        }
    }
    i32 w = d.right - d.left + 1;
    i32 h = d.bottom - d.top + 1;
    if (w <= 0 || h <= 0) {
        info->m_dirty.m_armed = -1;
        return;
    }
    ShadeRect s;
    s.left = right - d.right;
    s.top = bottom - d.bottom;
    s.right = s.left + w - 1;
    s.bottom = s.top + h - 1;
    if (info->m_drawActive) {
        m_owned->Select(info->m_drawFillCmd, info->m_drawFillArg);
    }
    m_owned->Blit(&d, dst->m_surface, &s, 1, 0);
    info->m_dirty.m_lastX = d.left;
    info->m_dirty.m_lastY = d.top;
    info->m_dirty.m_rect = *(&d);
    info->m_dirty.m_w = w;
    info->m_dirty.m_h = h;
    info->m_dirty.m_armed = 0;
}

// @early-stop
RVA(0x00154750, 0x275)
void CImage::BlitShadeFlipH(CResolveNode* info, CDDrawSurfacePair* dst) {
    LONG x = info->m_plotDX + m_originX + info->m_screenX - m_anchorX;
    LONG y = info->m_screenY - m_originY - info->m_plotDY - m_anchorY;
    if (info->m_flags & 0x40000) {
        info->m_level->m_mainPlane->WrapCoord(&x, &y);
    }
    i32 right = m_width + x - 1;
    i32 bottom = m_height + y - 1;
    ShadeRect d;
    d.left = x;
    d.top = y;
    d.right = right;
    d.bottom = bottom;
    if (info->m_flags & 0x40000) {
        BlitRect clipA = m_ownerCtx->m_level->m_planeCtx;
        RECT clip;
        CopyRect(&clip, static_cast<const RECT*>(&clipA));
        if (x < clip.left) {
            d.left += clip.left - x;
        }
        if (right > clip.right) {
            d.right += clip.right - right;
        }
        if (y < clip.top) {
            d.top += clip.top - y;
        }
        if (bottom > clip.bottom) {
            d.bottom += clip.bottom - bottom;
        }
    } else if (info->m_clip.left == static_cast<i32>(0x80000000)) {
        if (x < 0) {
            d.left = 0;
        }
        if (right >= dst->m_width) {
            d.right = dst->m_width - 1;
        }
        if (y < 0) {
            d.top = 0;
        }
        if (bottom >= dst->m_height) {
            d.bottom = dst->m_height - 1;
        }
    } else {
        if (x < info->m_clip.left) {
            d.left = info->m_clip.left;
        }
        if (right > info->m_clip.right) {
            d.right = info->m_clip.right;
        }
        if (y < info->m_clip.top) {
            d.top = info->m_clip.top;
        }
        if (bottom > info->m_clip.bottom) {
            d.bottom = info->m_clip.bottom;
        }
    }
    i32 w = d.right - d.left + 1;
    i32 h = d.bottom - d.top + 1;
    if (w <= 0 || h <= 0) {
        info->m_dirty.m_armed = -1;
        return;
    }
    ShadeRect s;
    s.left = right - d.right;
    s.top = bottom - d.bottom;
    s.right = s.left + w - 1;
    s.bottom = s.top + h - 1;
    if (info->m_drawActive) {
        m_owned->Select(info->m_drawFillCmd, info->m_drawFillArg);
    }
    m_owned->Blit(&d, dst->m_surface, &s, 0, 1);
    info->m_dirty.m_lastX = d.left;
    info->m_dirty.m_lastY = d.top;
    info->m_dirty.m_rect = *(&d);
    info->m_dirty.m_w = w;
    info->m_dirty.m_h = h;
    info->m_dirty.m_armed = 0;
}
