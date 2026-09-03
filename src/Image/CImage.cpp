#include <rva.h>

#include <Image/CImage.h>

#include <Mfc.h>
#include <MfcWin.h>

#include <DDrawMgr/DDrawDeviceManager.h>
#include <DDrawMgr/DDrawShadeBlit.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDSurface.h>
#include <Enums.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/ResolveNode.h>
#include <Gruntz/State.h>
#include <Image/ImageClipMacros.h>
#include <MakeRect.h>
#include <Pix16.h>
#include <Rez/FrameClock.h>
#include <Rez/RezArchiveDir.h>
#include <Rez/RezArchiveEntry.h>
#include <Rez/RezTypeTag.h>
#include <Wap32/CoordUnset.h>
#include <Wwd/WwdFile.h>

#include <ddraw.h>
#include <stdio.h>

DATA(0x002bf318)
DDBLTFX g_bltFx = {0};
DATA(0x002bf37c)
b32 g_resourceInstallActive = false;
DATA(0x002bf380)
i32 g_surfaceColorKey = 0;

static inline Coord ResolveImagePosition(
    const CResolveNode* info,
    const POINT& origin,
    const POINT& anchor,
    b32 mirrorX,
    b32 mirrorY
) {
    Coord offset = Coord(origin.x, origin.y) + info->m_plotOffset;
    if (mirrorX != false) {
        offset.m_x = -offset.m_x;
    }
    if (mirrorY != false) {
        offset.m_y = -offset.m_y;
    }
    return info->m_screenPosition + offset - Coord(anchor.x, anchor.y);
}

RVA(0x00152e90, 0x8b)
i32 CImage::Create(char* path, i32 keyed) {
    i32 colorKey = (keyed != 0) ? g_surfaceColorKey : -1;
    i32 surfaceCaps = 0;
    if (g_resourceInstallActive != false) {
        surfaceCaps = DDSCAPS_SYSTEMMEMORY;
    }
    CDDSurface* item = m_ownerCtx->m_deviceManager->LoadFileSurface(path, surfaceCaps, colorKey);
    m_surface = item;
    if (item == NULL) {
        return 0;
    }

    m_width = item->m_width;
    m_height = item->m_height;
    m_anchor = CPoint(m_width >> 1, m_height >> 1);
    if (item->m_hasColorKey != false) {
        m_bltFastFlags = DDBLTFAST_WAIT | DDBLTFAST_SRCCOLORKEY;
    } else {
        m_bltFastFlags = DDBLTFAST_WAIT;
    }
    m_origin = CPoint(0, 0);
    return 1;
}

RVA(0x00152f20, 0x86)
i32 CImage::Resolve(CRezItm* src, i32 keyed) {
    BEGIN_FILE_IMAGE_PARSE(src, index, resolved)

    RecordBytes<PidHeader> blob;
    blob.m_bytes = resolved;
    i32 result = this->LoadDispatch(

        static_cast<PidHeader*>(blob.m_rec),
        index,
        src->GetSize(),
        keyed
    );
    src->UnLoad();
    return result;
}

RVA(0x00152fb0, 0x123)
i32 CImage::LoadDispatch(PidHeader* desc, FileImageFormat mode, u32 size, i32 keyed) {
    if (mode != FMT_BMP && mode != FMT_PCX && mode != FMT_RID && mode != FMT_PID) {
        return 0;
    }

    if (mode == FMT_PID && (HAS(desc->flags, PID_GRAMMAR_SKIPRUN))) {
        if (!BuildShadeBlitter(desc, size)) {
            return 0;
        }

        if (m_owned != NULL && (HAS(desc->flags, PID_SRC_8BPP_SHADE))) {
            m_owned->Select(SHADE_DST_BY_SRC, NULL);
            return 1;
        }
        return 1;
    }
    i32 colorKey = (keyed != 0) ? g_surfaceColorKey : -1;
    if (mode == FMT_PID || mode == FMT_RID) {
        m_origin = CPoint(desc->offsetX, desc->offsetY);
    } else {
        m_origin = CPoint(0, 0);
    }
    i32 surfaceCaps = 0;
    if (g_resourceInstallActive != false) {
        surfaceCaps = DDSCAPS_SYSTEMMEMORY;
    }

    CDDSurface* item =
        m_ownerCtx->m_deviceManager->LoadSurfaceFromPid(desc, mode, size, surfaceCaps, colorKey);
    m_surface = item;
    if (item == NULL) {
        return 0;
    }
    CSize imageSize(item->m_width, item->m_height);
    m_width = imageSize.cx;
    m_height = imageSize.cy;
    m_anchor = CPoint(imageSize.cx >> 1, imageSize.cy >> 1);
    if (item->m_hasColorKey != false) {
        m_bltFastFlags = DDBLTFAST_WAIT | DDBLTFAST_SRCCOLORKEY;
        return 1;
    }
    m_bltFastFlags = DDBLTFAST_WAIT;
    return 1;
}

RVA(0x001530e0, 0x92)
i32 CImage::CreateBlankSurface(i32 width, i32 height, i32 keyed) {
    i32 colorKey = (keyed != 0) ? g_surfaceColorKey : -1;
    i32 surfaceCaps = 0;
    if (g_resourceInstallActive != false) {
        surfaceCaps = DDSCAPS_SYSTEMMEMORY;
    }
    CDDSurface* item = m_ownerCtx->m_deviceManager
                           ->CreateKeyedSurface(width, height, BPP_UNSET, surfaceCaps, colorKey);
    m_surface = item;
    if (item == NULL) {
        return 0;
    }
    CSize imageSize(item->m_width, item->m_height);
    m_width = imageSize.cx;
    m_height = imageSize.cy;
    m_anchor = CPoint(imageSize.cx >> 1, imageSize.cy >> 1);
    if (item->m_hasColorKey != false) {
        m_bltFastFlags = DDBLTFAST_WAIT | DDBLTFAST_SRCCOLORKEY;
    } else {
        m_bltFastFlags = DDBLTFAST_WAIT;
    }
    m_origin = CPoint(0, 0);
    return 1;
}

RVA(0x00153180, 0xda)
i32 CImage::BuildShadeBlitter(PidHeader* desc, u32 size) {
    CDDrawShadeBlit* owned = new CDDrawShadeBlit();
    m_owned = owned;
    if (owned == NULL) {
        return 0;
    }

    ColorDepth fmt = m_ownerCtx->m_drawTarget->m_frontSurface->m_bpp;
    if (!owned->Build(desc, static_cast<i32>(size), fmt)) {
        return 0;
    }
    CSize imageSize(m_owned->m_width, m_owned->m_height);
    m_width = imageSize.cx;
    m_height = imageSize.cy;
    m_bltFastFlags = DDBLTFAST_WAIT | DDBLTFAST_SRCCOLORKEY;
    m_anchor = CPoint(imageSize.cx >> 1, imageSize.cy >> 1);
    m_origin = CPoint(desc->offsetX, desc->offsetY);
    return 1;
}

RVA(0x00153260, 0x41)
void CImage::Unload() {
    m_width = 0;
    m_height = 0;
    if (m_surface != NULL) {
        m_ownerCtx->m_deviceManager->RemoveSurface(m_surface);
        m_surface = NULL;
    }
    CDDrawShadeBlit* owned = m_owned;
    if (owned != NULL) {
        owned->Teardown();
        delete owned;
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

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00153330, 0x36)
i32 CImage::SetOrigin(PidHeader* desc, FileImageFormat mode) {
    if (mode == FMT_PID || mode == FMT_RID) {
        m_origin = CPoint(desc->offsetX, desc->offsetY);
    } else {
        m_origin = CPoint(0, 0);
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
i32 CImage::Reload(CRezItm* src, i32 keyed) {

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
        this->Unload();
        return this->Resolve(src, keyed);
    }

    BEGIN_FILE_IMAGE_PARSE(src, index, resolved)
    if (src->GetSize() == 0) {
        return 0;
    }

    return m_surface->Resolve(
        m_ownerCtx->m_deviceManager,
        resolved,
        index,
        static_cast<u32>(src->GetSize()),
        g_surfaceColorKey
    );
}

// @early-stop
RVA(0x00153470, 0x31a)
void CImage::RenderImage(CResolveNode* info, CDDrawSurfacePair* dst) {
    SpriteStateFlags mode = info->m_stateFlags;
    if (HAS(mode, SPRITE_STATE_HIDDEN)) {
        info->m_dirty.m_armed = -1;
        return;
    }
    if (HAS(mode, SPRITE_STATE_FLASHING)) {
        if (g_engineFrameDelta >= info->m_flashCountdown) {
            info->m_flashCountdown = info->m_flashInterval;
            mode ^= SPRITE_STATE_FLASH_VISIBLE;
            info->m_stateFlags = mode;
        } else {
            info->m_flashCountdown -= g_engineFrameDelta;
        }
        mode = info->m_stateFlags;
        if (!HAS(mode, SPRITE_STATE_FLASH_VISIBLE)) {
            info->m_dirty.m_armed = -1;
            return;
        }
    }
    i32 mirrorX = HAS(mode, SPRITE_STATE_MIRROR_X);
    i32 mirrorY = HAS(mode, SPRITE_STATE_MIRROR_Y);
    if (mirrorX && mirrorY) {
        if (m_owned) {
            BlitShadeNorm(info, dst);
        } else {
            BlitNorm(info, dst);
        }
        return;
    }
    if (mirrorX) {
        if (m_owned) {
            BlitShadeFlipV(info, dst);
        } else {
            BlitFlipV(info, dst);
        }
        return;
    }
    if (mirrorY) {
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

    Coord resolvedPosition = ResolveImagePosition(info, m_origin, m_anchor, false, false);
    CPoint position(resolvedPosition.m_x, resolvedPosition.m_y);
    if (info->m_flags & IDX(WWD_GAME_OBJECT_FLAG_WORLD_SPACE)) {
        info->m_level->m_mainPlane->WorldToViewport(&position.x, &position.y);
    }
    CPoint farCorner = position + CSize(m_width - 1, m_height - 1);
    CRect destination(position.x, position.y, farCorner.x, farCorner.y);
    CSize visibleSize;
    if (!ClipImageRect(&destination, &visibleSize, info, dst, m_ownerCtx)) {
        info->m_dirty.m_armed = -1;
        return;
    }
    CRect s(
        destination.left - position.x,
        destination.top - position.y,
        destination.left - position.x + visibleSize.cx,
        destination.top - position.y + visibleSize.cy
    );
    dst->m_surface->BltFast(destination.left, destination.top, m_surface, &s, m_bltFastFlags);
    info->m_dirty.m_lastPosition.Set(destination.left, destination.top);
    info->m_dirty.m_size = visibleSize;
    info->m_dirty.m_armed = 0;
    info->m_dirty.m_rect = destination;
}

// @early-stop
RVA(0x00153790, 0x6a)
void CImage::RenderFrame(CDDrawSurfacePair* target, i32 x, i32 y, i32 flags) {
    RVA_DYNINIT(0x00153800, 0x10, clip)
    DATA(0x002bf2a0)
    static CResolveNode clip;
    if (clip.Init(m_ownerCtx, 0, x, y, flags, 0)) {
        this->RenderImage(&clip, target);
    }
}

// @early-stop
RVA(0x00153810, 0x95)
void CImage::RenderFrameClipped(
    CDDrawSurfacePair* target,
    i32 x,
    i32 y,
    RECT* clipRect,
    i32 flags
) {
    RVA_DYNINIT(0x001538b0, 0x10, clip)
    DATA(0x002bf228)
    static CResolveNode clip;
    if (clip.Init(m_ownerCtx, 0, x, y, flags, 0)) {
        if (clipRect != NULL) {
            clip.m_clip = *clipRect;
        }
        this->RenderImage(&clip, target);
    }
}

// @early-stop
RVA(0x001538c0, 0x257)
void CImage::BlitNorm(CResolveNode* info, CDDrawSurfacePair* dst) {
    Coord resolvedPosition = ResolveImagePosition(info, m_origin, m_anchor, true, true);
    CPoint position(resolvedPosition.m_x, resolvedPosition.m_y);
    if (info->m_flags & IDX(WWD_GAME_OBJECT_FLAG_WORLD_SPACE)) {
        info->m_level->m_mainPlane->WorldToViewport(&position.x, &position.y);
    }
    CPoint farCorner = position + CSize(m_width - 1, m_height - 1);
    CRect d(position.x, position.y, farCorner.x, farCorner.y);
    CSize visibleSize;
    if (!ClipImageRect(&d, &visibleSize, info, dst, m_ownerCtx)) {
        info->m_dirty.m_armed = -1;
        return;
    }
    CRect s = MakeRect(
        farCorner.x - d.right,
        farCorner.y - d.bottom,
        farCorner.x - d.right + visibleSize.cx,
        farCorner.y - d.bottom + visibleSize.cy
    );
    g_bltFx.dwDDFX = DDBLTFX_MIRRORLEFTRIGHT | DDBLTFX_MIRRORUPDOWN;
    d.InflateRect(0, 0, 1, 1);
    dst->m_surface->BltEx(&d, m_surface, &s, DDBLT_DDFX | DDBLT_KEYSRC, &g_bltFx);
    d.DeflateRect(0, 0, 1, 1);
    info->m_dirty.m_lastPosition.Set(d.left, d.top);
    info->m_dirty.m_rect = d;
    info->m_dirty.m_size = visibleSize;
    info->m_dirty.m_armed = 0;
}

// @early-stop
RVA(0x00153b20, 0x270)
void CImage::BlitFlipV(CResolveNode* info, CDDrawSurfacePair* dst) {
    Coord resolvedPosition = ResolveImagePosition(info, m_origin, m_anchor, true, false);
    CPoint position(resolvedPosition.m_x, resolvedPosition.m_y);
    if (info->m_flags & IDX(WWD_GAME_OBJECT_FLAG_WORLD_SPACE)) {
        info->m_level->m_mainPlane->WorldToViewport(&position.x, &position.y);
    }
    CPoint farCorner = position + CSize(m_width - 1, m_height - 1);
    CRect d(position.x, position.y, farCorner.x, farCorner.y);
    CSize visibleSize;
    if (!ClipImageRect(&d, &visibleSize, info, dst, m_ownerCtx)) {
        info->m_dirty.m_armed = -1;
        return;
    }
    CRect s = MakeRect(
        farCorner.x - d.right,
        d.top - position.y,
        farCorner.x - d.right + visibleSize.cx,
        d.top - position.y + visibleSize.cy
    );
    d.InflateRect(0, 0, 1, 1);
    g_bltFx.dwDDFX = DDBLTFX_MIRRORLEFTRIGHT;
    dst->m_surface->BltEx(&d, m_surface, &s, DDBLT_DDFX | DDBLT_KEYSRC, &g_bltFx);
    d.DeflateRect(0, 0, 1, 1);
    info->m_dirty.m_lastPosition.Set(d.left, d.top);
    info->m_dirty.m_rect = d;
    info->m_dirty.m_size = visibleSize;
    info->m_dirty.m_armed = 0;
}

// @early-stop
RVA(0x00153d90, 0x259)
void CImage::BlitFlipH(CResolveNode* info, CDDrawSurfacePair* dst) {
    Coord resolvedPosition = ResolveImagePosition(info, m_origin, m_anchor, false, true);
    CPoint position(resolvedPosition.m_x, resolvedPosition.m_y);
    if (info->m_flags & IDX(WWD_GAME_OBJECT_FLAG_WORLD_SPACE)) {
        info->m_level->m_mainPlane->WorldToViewport(&position.x, &position.y);
    }
    CPoint farCorner = position + CSize(m_width - 1, m_height - 1);
    CRect d(position.x, position.y, farCorner.x, farCorner.y);
    CSize visibleSize;
    if (!ClipImageRect(&d, &visibleSize, info, dst, m_ownerCtx)) {
        info->m_dirty.m_armed = -1;
        return;
    }
    CRect s = MakeRect(
        d.left - position.x,
        farCorner.y - d.bottom,
        d.left - position.x + visibleSize.cx,
        farCorner.y - d.bottom + visibleSize.cy
    );
    d.InflateRect(0, 0, 1, 1);
    g_bltFx.dwDDFX = DDBLTFX_MIRRORUPDOWN;
    dst->m_surface->BltEx(&d, m_surface, &s, DDBLT_DDFX | DDBLT_KEYSRC, &g_bltFx);
    d.DeflateRect(0, 0, 1, 1);
    info->m_dirty.m_lastPosition.Set(d.left, d.top);
    info->m_dirty.m_rect = d;
    info->m_dirty.m_size = visibleSize;
    info->m_dirty.m_armed = 0;
}

// @early-stop
RVA(0x00153ff0, 0x280)
void CImage::BlitShadeFlipHV(CResolveNode* info, CDDrawSurfacePair* dst) {
    Coord resolvedPosition = ResolveImagePosition(info, m_origin, m_anchor, false, false);
    CPoint position(resolvedPosition.m_x, resolvedPosition.m_y);
    if (info->m_flags & IDX(WWD_GAME_OBJECT_FLAG_WORLD_SPACE)) {
        info->m_level->m_mainPlane->WorldToViewport(&position.x, &position.y);
    }
    CPoint farCorner = position + CSize(m_width - 1, m_height - 1);
    ShadeRect d = MakeRect(position.x, position.y, farCorner.x, farCorner.y);
    CSize visibleSize;
    if (!ClipImageRect(&d, &visibleSize, info, dst, m_ownerCtx)) {
        info->m_dirty.m_armed = -1;
        return;
    }
    ShadeRect s;
    s = MakeRect(
        d.left - position.x,
        d.top - position.y,
        d.left - position.x + visibleSize.cx - 1,
        d.top - position.y + visibleSize.cy - 1
    );
    if (info->m_drawActive) {
        m_owned->Select(info->m_drawFillCmd, info->m_drawFillArg);
        m_owned->m_light = info->m_fillFraction;
    }
    m_owned->Blit(&d, dst->m_surface, &s, 0, 0);
    info->m_dirty.m_lastPosition.Set(d.left, d.top);
    info->m_dirty.m_rect = d;
    info->m_dirty.m_size = visibleSize;
    info->m_dirty.m_armed = 0;
}

RVA(0x00154270, 0x257)
void CImage::BlitShadeNorm(CResolveNode* info, CDDrawSurfacePair* dst) {
    Coord resolvedPosition = ResolveImagePosition(info, m_origin, m_anchor, true, true);
    CPoint position(resolvedPosition.m_x, resolvedPosition.m_y);
    if (info->m_flags & IDX(WWD_GAME_OBJECT_FLAG_WORLD_SPACE)) {
        info->m_level->m_mainPlane->WorldToViewport(&position.x, &position.y);
    }
    CPoint farCorner = position + CSize(m_width - 1, m_height - 1);
    ShadeRect d = MakeRect(position.x, position.y, farCorner.x, farCorner.y);
    CSize visibleSize;
    if (!ClipImageRect(&d, &visibleSize, info, dst, m_ownerCtx)) {
        info->m_dirty.m_armed = -1;
        return;
    }
    ShadeRect s;
    s = MakeRect(
        farCorner.x - d.right,
        farCorner.y - d.bottom,
        farCorner.x - d.right + visibleSize.cx - 1,
        farCorner.y - d.bottom + visibleSize.cy - 1
    );
    if (info->m_drawActive) {
        m_owned->Select(info->m_drawFillCmd, info->m_drawFillArg);
    }
    m_owned->Blit(&d, dst->m_surface, &s, 1, 1);
    info->m_dirty.m_lastPosition.Set(d.left, d.top);
    info->m_dirty.m_rect = d;
    info->m_dirty.m_size = visibleSize;
    info->m_dirty.m_armed = 0;
}

// @early-stop
RVA(0x001544d0, 0x275)
void CImage::BlitShadeFlipV(CResolveNode* info, CDDrawSurfacePair* dst) {
    Coord resolvedPosition = ResolveImagePosition(info, m_origin, m_anchor, true, false);
    CPoint position(resolvedPosition.m_x, resolvedPosition.m_y);
    if (info->m_flags & IDX(WWD_GAME_OBJECT_FLAG_WORLD_SPACE)) {
        info->m_level->m_mainPlane->WorldToViewport(&position.x, &position.y);
    }
    CPoint farCorner = position + CSize(m_width - 1, m_height - 1);
    ShadeRect d = MakeRect(position.x, position.y, farCorner.x, farCorner.y);
    CSize visibleSize;
    if (!ClipImageRect(&d, &visibleSize, info, dst, m_ownerCtx)) {
        info->m_dirty.m_armed = -1;
        return;
    }
    ShadeRect s;
    s = MakeRect(
        d.left - position.x,
        d.top - position.y,
        d.left - position.x + visibleSize.cx - 1,
        d.top - position.y + visibleSize.cy - 1
    );
    if (info->m_drawActive) {
        m_owned->Select(info->m_drawFillCmd, info->m_drawFillArg);
    }
    m_owned->Blit(&d, dst->m_surface, &s, 1, 0);
    info->m_dirty.m_lastPosition.Set(d.left, d.top);
    info->m_dirty.m_rect = d;
    info->m_dirty.m_size = visibleSize;
    info->m_dirty.m_armed = 0;
}

// @early-stop
RVA(0x00154750, 0x275)
void CImage::BlitShadeFlipH(CResolveNode* info, CDDrawSurfacePair* dst) {
    Coord resolvedPosition = ResolveImagePosition(info, m_origin, m_anchor, false, true);
    CPoint position(resolvedPosition.m_x, resolvedPosition.m_y);
    if (info->m_flags & IDX(WWD_GAME_OBJECT_FLAG_WORLD_SPACE)) {
        info->m_level->m_mainPlane->WorldToViewport(&position.x, &position.y);
    }
    CPoint farCorner = position + CSize(m_width - 1, m_height - 1);
    ShadeRect d = MakeRect(position.x, position.y, farCorner.x, farCorner.y);
    CSize visibleSize;
    if (!ClipImageRect(&d, &visibleSize, info, dst, m_ownerCtx)) {
        info->m_dirty.m_armed = -1;
        return;
    }
    ShadeRect s;
    s = MakeRect(
        d.left - position.x,
        farCorner.y - d.bottom,
        d.left - position.x + visibleSize.cx - 1,
        farCorner.y - d.bottom + visibleSize.cy - 1
    );
    if (info->m_drawActive) {
        m_owned->Select(info->m_drawFillCmd, info->m_drawFillArg);
    }
    m_owned->Blit(&d, dst->m_surface, &s, 0, 1);
    info->m_dirty.m_lastPosition.Set(d.left, d.top);
    info->m_dirty.m_rect = d;
    info->m_dirty.m_size = visibleSize;
    info->m_dirty.m_armed = 0;
}
