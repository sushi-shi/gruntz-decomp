#include <rva.h>

#include <Image/CImage.h>

#include <Mfc.h>

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
    m_anchorX = m_width >> 1;
    m_anchorY = m_height >> 1;
    if (item->m_hasColorKey != false) {
        m_bltFastFlags = DDBLTFAST_WAIT | DDBLTFAST_SRCCOLORKEY;
    } else {
        m_bltFastFlags = DDBLTFAST_WAIT;
    }
    m_originX = 0;
    m_originY = 0;
    return 1;
}

RVA(0x00152f20, 0x86)
i32 CImage::Resolve(CRezArchiveEntry* src, i32 keyed) {
    BEGIN_FILE_IMAGE_PARSE(src, index, resolved)

    RecordBytes<PidHeader> blob;
    blob.m_chars = resolved;
    i32 result = this->LoadDispatch(

        static_cast<PidHeader*>(blob.m_rec),
        index,
        src->m_size,
        keyed
    );
    src->ReleaseData();
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
        i32 g10 = desc->offsetX;
        i32 g14 = desc->offsetY;
        m_originX = g10;
        m_originY = g14;
    } else {
        m_originX = 0;
        m_originY = 0;
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
    i32 w = item->m_width;
    m_width = w;
    i32 h = item->m_height;
    m_height = h;
    m_anchorX = w >> 1;
    m_anchorY = h >> 1;
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
    i32 w = item->m_width;
    m_width = w;
    i32 h = item->m_height;
    m_height = h;
    m_anchorX = w >> 1;
    m_anchorY = h >> 1;
    if (item->m_hasColorKey != false) {
        m_bltFastFlags = DDBLTFAST_WAIT | DDBLTFAST_SRCCOLORKEY;
    } else {
        m_bltFastFlags = DDBLTFAST_WAIT;
    }
    m_originX = 0;
    m_originY = 0;
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
    i32 w = m_owned->m_width;
    m_width = w;
    i32 h = m_owned->m_height;
    m_height = h;
    m_bltFastFlags = DDBLTFAST_WAIT | DDBLTFAST_SRCCOLORKEY;
    m_anchorX = w >> 1;
    m_anchorY = h >> 1;
    m_originX = desc->offsetX;
    m_originY = desc->offsetY;
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

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00153330, 0x36)
i32 CImage::SetOrigin(PidHeader* desc, FileImageFormat mode) {
    if (mode == FMT_PID || mode == FMT_RID) {
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
i32 CImage::Reload(CRezArchiveEntry* src, i32 keyed) {

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
    if (src->m_size == 0) {
        return 0;
    }

    return m_surface->Resolve(
        m_ownerCtx->m_deviceManager,
        resolved,
        index,
        static_cast<u32>(src->m_size),
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

    LONG x = m_originX - m_anchorX + info->m_plotDX + info->m_screenX;
    LONG y = m_originY - m_anchorY + info->m_plotDY + info->m_screenY;
    if (info->m_flags & IDX(WWD_GAME_OBJECT_FLAG_WORLD_SPACE)) {
        info->m_level->m_mainPlane->WorldToViewport(&x, &y);
    }
    i32 right = m_width + x - 1;
    i32 bottom = m_height + y - 1;
    i32 dleft = x;
    i32 dtop = y;
    i32 dright = right;
    i32 dbottom = bottom;
    if (info->m_flags & IDX(WWD_GAME_OBJECT_FLAG_WORLD_SPACE)) {
        BlitRect srcClip = m_ownerCtx->m_level->m_viewportRect;
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
    } else if (info->m_clip.left == COORD_UNSET) {
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
    dst->m_surface->BltFast(dleft, dtop, m_surface, &s, m_bltFastFlags);
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
    LONG x = info->m_screenX - m_originX - info->m_plotDX - m_anchorX;
    LONG y = info->m_screenY - m_originY - info->m_plotDY - m_anchorY;
    if (info->m_flags & IDX(WWD_GAME_OBJECT_FLAG_WORLD_SPACE)) {
        info->m_level->m_mainPlane->WorldToViewport(&x, &y);
    }
    i32 right = m_width + x - 1;
    i32 bottom = m_height + y - 1;
    DECLARE_CLIPPED_IMAGE_RECT(RECT, d, info, dst, x, y, right, bottom, w, h)
    RECT s;
    s.left = right - d.right;
    s.top = bottom - d.bottom;
    s.right = s.left + w;
    s.bottom = s.top + h;
    g_bltFx.dwDDFX = DDBLTFX_MIRRORLEFTRIGHT | DDBLTFX_MIRRORUPDOWN;
    d.right += 1;
    d.bottom += 1;
    dst->m_surface->BltEx(&d, m_surface, &s, DDBLT_DDFX | DDBLT_KEYSRC, &g_bltFx);
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
    if (info->m_flags & IDX(WWD_GAME_OBJECT_FLAG_WORLD_SPACE)) {
        info->m_level->m_mainPlane->WorldToViewport(&x, &y);
    }
    i32 right = m_width + x - 1;
    i32 bottom = m_height + y - 1;
    DECLARE_CLIPPED_IMAGE_RECT(RECT, d, info, dst, x, y, right, bottom, w, h)
    RECT s;
    s.left = right - d.right;
    s.top = d.top - y;
    s.right = s.left + w;
    s.bottom = s.top + h;
    d.right += 1;
    d.bottom += 1;
    g_bltFx.dwDDFX = DDBLTFX_MIRRORLEFTRIGHT;
    dst->m_surface->BltEx(&d, m_surface, &s, DDBLT_DDFX | DDBLT_KEYSRC, &g_bltFx);
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
    if (info->m_flags & IDX(WWD_GAME_OBJECT_FLAG_WORLD_SPACE)) {
        info->m_level->m_mainPlane->WorldToViewport(&x, &y);
    }
    i32 right = m_width + x - 1;
    i32 bottom = m_height + y - 1;
    DECLARE_CLIPPED_IMAGE_RECT(RECT, d, info, dst, x, y, right, bottom, w, h)
    RECT s;
    s.left = d.left - x;
    s.top = bottom - d.bottom;
    s.right = s.left + w;
    s.bottom = s.top + h;
    d.right += 1;
    d.bottom += 1;
    g_bltFx.dwDDFX = DDBLTFX_MIRRORUPDOWN;
    dst->m_surface->BltEx(&d, m_surface, &s, DDBLT_DDFX | DDBLT_KEYSRC, &g_bltFx);
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
    if (info->m_flags & IDX(WWD_GAME_OBJECT_FLAG_WORLD_SPACE)) {
        info->m_level->m_mainPlane->WorldToViewport(&x, &y);
    }
    i32 right = m_width + x - 1;
    i32 bottom = m_height + y - 1;
    DECLARE_CLIPPED_IMAGE_RECT(ShadeRect, d, info, dst, x, y, right, bottom, w, h)
    ShadeRect s;
    s.left = d.left - x;
    s.top = d.top - y;
    s.right = s.left + w - 1;
    s.bottom = s.top + h - 1;
    if (info->m_drawActive) {
        m_owned->Select(info->m_drawFillCmd, info->m_drawFillArg);
        m_owned->m_light = info->m_fillFraction;
    }
    m_owned->Blit(&d, dst->m_surface, &s, 0, 0);
    info->m_dirty.m_lastX = d.left;
    info->m_dirty.m_lastY = d.top;
    info->m_dirty.m_rect = *(&d);
    info->m_dirty.m_w = w;
    info->m_dirty.m_h = h;
    info->m_dirty.m_armed = 0;
}

RVA(0x00154270, 0x257)
void CImage::BlitShadeNorm(CResolveNode* info, CDDrawSurfacePair* dst) {
    LONG x = info->m_screenX - m_originX - m_anchorX - info->m_plotDX;
    LONG y = info->m_screenY - m_originY - m_anchorY - info->m_plotDY;
    if (info->m_flags & IDX(WWD_GAME_OBJECT_FLAG_WORLD_SPACE)) {
        info->m_level->m_mainPlane->WorldToViewport(&x, &y);
    }
    i32 right = m_width + x - 1;
    i32 bottom = m_height + y - 1;
    DECLARE_CLIPPED_IMAGE_RECT(ShadeRect, d, info, dst, x, y, right, bottom, w, h)
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
    if (info->m_flags & IDX(WWD_GAME_OBJECT_FLAG_WORLD_SPACE)) {
        info->m_level->m_mainPlane->WorldToViewport(&x, &y);
    }
    i32 right = m_width + x - 1;
    i32 bottom = m_height + y - 1;
    DECLARE_CLIPPED_IMAGE_RECT(ShadeRect, d, info, dst, x, y, right, bottom, w, h)
    ShadeRect s;
    s.left = d.left - x;
    s.top = d.top - y;
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
    if (info->m_flags & IDX(WWD_GAME_OBJECT_FLAG_WORLD_SPACE)) {
        info->m_level->m_mainPlane->WorldToViewport(&x, &y);
    }
    i32 right = m_width + x - 1;
    i32 bottom = m_height + y - 1;
    DECLARE_CLIPPED_IMAGE_RECT(ShadeRect, d, info, dst, x, y, right, bottom, w, h)
    ShadeRect s;
    s.left = d.left - x;
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
