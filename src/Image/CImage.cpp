#include <StdAfx.h>

#include <rva.h>

#include <Image/CImage.h>

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
#include <RectMacros.h>
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

static inline i32 SurfaceColorKey(i32 keyed) {
    return (keyed != 0) ? g_surfaceColorKey : -1;
}

inline void CImage::SetBltFastFlags(CDDSurface* surface) {
    if (surface->m_hasColorKey != false) {
        m_bltFastFlags = DDBLTFAST_WAIT | DDBLTFAST_SRCCOLORKEY;
    } else {
        m_bltFastFlags = DDBLTFAST_WAIT;
    }
}

RVA(0x00152e90, 0x8b)
i32 CImage::Create(char* path, i32 keyed) {
    i32 colorKey = SurfaceColorKey(keyed);
    i32 surfaceCaps = 0;
    if (g_resourceInstallActive != false) {
        surfaceCaps = DDSCAPS_SYSTEMMEMORY;
    }
    CDDSurface* item = OwnerMgr()->GetDeviceManager()->LoadFileSurface(path, surfaceCaps, colorKey);
    m_surface = item;
    if (item == NULL) {
        return 0;
    }

    m_width = item->GetWidth();
    m_height = item->GetHeight();
    SET_POINT_COMPONENTS(m_anchor, m_width >> 1, m_height >> 1);
    SetBltFastFlags(item);
    SET_POINT_COMPONENTS(m_origin, 0, 0);
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

    if (mode == FMT_PID && (HAS(desc->m_flags, PID_GRAMMAR_SKIPRUN))) {
        if (!BuildShadeBlitter(desc, size)) {
            return 0;
        }

        if (m_owned != NULL && (HAS(desc->m_flags, PID_SRC_8BPP_SHADE))) {
            m_owned->Select(SHADE_DST_BY_SRC, NULL);
            return 1;
        }
        return 1;
    }
    i32 colorKey = SurfaceColorKey(keyed);
    if (mode == FMT_PID || mode == FMT_RID) {
        i32 imageOffsetX = desc->m_offsetX;
        i32 imageOffsetY = desc->m_offsetY;
        SET_POINT_COMPONENTS(m_origin, imageOffsetX, imageOffsetY);
    } else {
        SET_POINT_COMPONENTS(m_origin, 0, 0);
    }
    i32 surfaceCaps = 0;
    if (g_resourceInstallActive != false) {
        surfaceCaps = DDSCAPS_SYSTEMMEMORY;
    }

    CDDSurface* item =
        OwnerMgr()->GetDeviceManager()->LoadSurfaceFromPid(desc, mode, size, surfaceCaps, colorKey);
    m_surface = item;
    if (item == NULL) {
        return 0;
    }
    i32 w = item->GetWidth();
    m_width = w;
    i32 h = item->GetHeight();
    m_height = h;
    SET_POINT_COMPONENTS(m_anchor, w >> 1, h >> 1);
    SetBltFastFlags(item);
    return 1;
}

RVA(0x001530e0, 0x92)
i32 CImage::CreateBlankSurface(i32 width, i32 height, i32 keyed) {
    i32 colorKey = SurfaceColorKey(keyed);
    i32 surfaceCaps = 0;
    if (g_resourceInstallActive != false) {
        surfaceCaps = DDSCAPS_SYSTEMMEMORY;
    }
    CDDSurface* item = OwnerMgr()
                           ->GetDeviceManager()
                           ->CreateKeyedSurface(width, height, BPP_UNSET, surfaceCaps, colorKey);
    m_surface = item;
    if (item == NULL) {
        return 0;
    }
    i32 w = item->GetWidth();
    m_width = w;
    i32 h = item->GetHeight();
    m_height = h;
    SET_POINT_COMPONENTS(m_anchor, w >> 1, h >> 1);
    SetBltFastFlags(item);
    SET_POINT_COMPONENTS(m_origin, 0, 0);
    return 1;
}

RVA(0x00153180, 0xda)
i32 CImage::BuildShadeBlitter(PidHeader* desc, u32 size) {
    CDDrawShadeBlit* owned = new CDDrawShadeBlit();
    m_owned = owned;
    if (owned == NULL) {
        return 0;
    }

    ColorDepth fmt = OwnerMgr()->m_drawTarget->GetFrontSurface()->m_bpp;
    if (!owned->Build(desc, static_cast<i32>(size), fmt)) {
        return 0;
    }
    i32 w = m_owned->m_width;
    m_width = w;
    i32 h = m_owned->m_height;
    m_height = h;
    m_bltFastFlags = DDBLTFAST_WAIT | DDBLTFAST_SRCCOLORKEY;
    SET_POINT_COMPONENTS(m_anchor, w >> 1, h >> 1);
    SET_POINT_COMPONENTS(m_origin, desc->m_offsetX, desc->m_offsetY);
    return 1;
}

RVA(0x00153260, 0x41)
void CImage::Unload() {
    m_width = 0;
    m_height = 0;
    if (m_surface != NULL) {
        OwnerMgr()->GetDeviceManager()->RemoveSurface(m_surface);
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
        i32 oy = desc->m_offsetY;
        i32 ox = desc->m_offsetX;
        SET_POINT_COMPONENTS(m_origin, ox, oy);
    } else {
        SET_POINT_COMPONENTS(m_origin, 0, 0);
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
    IDirectDrawSurface* s = surf->GetDirectDrawSurface();
    if (s != NULL) {
        if (s->IsLost() == 0) {
            return 1;
        }
    }
    surf = m_surface;
    // The direct COM receiver preserves the restoration-path load lifetime.
    if (surf->m_ddSurface->Restore() != 0) {
        this->Unload();
        return this->Resolve(src, keyed);
    }

    BEGIN_FILE_IMAGE_PARSE(src, index, resolved)
    if (src->GetSize() == 0) {
        return 0;
    }

    return m_surface->Resolve(
        OwnerMgr()->GetDeviceManager(),
        resolved,
        index,
        static_cast<u32>(src->GetSize()),
        g_surfaceColorKey
    );
}

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

    LONG x = IMAGE_POSITION_COMPONENT(
        m_origin.x,
        m_anchor.x,
        info->m_plotOffset.m_x,
        info->m_screenPosition.m_x
    );
    LONG y = IMAGE_POSITION_COMPONENT(
        m_origin.y,
        m_anchor.y,
        info->m_plotOffset.m_y,
        info->m_screenPosition.m_y
    );
    DECLARE_IMAGE_DEST_EXTENTS(info, x, y, right, bottom);
    RECT d;
    d.left = x;
    d.top = y;
    d.right = right;
    d.bottom = bottom;
    if (info->m_flags & IDX(WWD_GAME_OBJECT_FLAG_WORLD_SPACE)) {
        BlitRect srcClip = OwnerMgr()->m_level->m_viewportRect;
        RECT destClip;
        CopyRect(&destClip, static_cast<const RECT*>(&srcClip));
        if (x < destClip.left) {
            d.left += destClip.left - x;
        }
        if (right > destClip.right) {
            d.right = destClip.right;
        }
        if (y < destClip.top) {
            d.top += destClip.top - y;
        }
        if (bottom > destClip.bottom) {
            d.bottom = destClip.bottom;
        }
    } else if (info->m_clip.left == COORD_UNSET) {
        if (x < 0) {
            d.left = 0;
        }
        if (right >= dst->GetWidth()) {
            d.right = dst->GetWidth() - 1;
        }
        if (y < 0) {
            d.top = 0;
        }
        if (bottom >= dst->GetHeight()) {
            d.bottom = dst->GetHeight() - 1;
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
    s.left = d.left - x;
    s.top = d.top - y;
    s.right = s.left + w;
    s.bottom = s.top + h;
    dst->GetSurface()->BltFast(d.left, d.top, m_surface, &s, m_bltFastFlags);
    info->m_dirty.Set(d, w, h);
}

RVA(0x00153790, 0x6a)
void CImage::RenderFrame(CDDrawSurfacePair* target, i32 x, i32 y, i32 flags) {
    RVA_DYNINIT(0x00153800, 0x10, s_clip)
    DATA(0x002bf2a0)
    static CResolveNode s_clip;
    if (s_clip.Init(OwnerMgr(), 0, x, y, flags, 0)) {
        this->RenderImage(&s_clip, target);
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
    RVA_DYNINIT(0x001538b0, 0x10, s_clip)
    DATA(0x002bf228)
    static CResolveNode s_clip;
    if (s_clip.Init(OwnerMgr(), 0, x, y, flags, 0)) {
        if (clipRect != NULL) {
            s_clip.m_clip = *clipRect;
        }
        this->RenderImage(&s_clip, target);
    }
}

RVA(0x001538c0, 0x257)
void CImage::BlitNorm(CResolveNode* info, CDDrawSurfacePair* dst) {
    LONG x = IMAGE_MIRROR_COMPONENT(
        info->m_screenPosition.m_x,
        m_origin.x,
        info->m_plotOffset.m_x,
        m_anchor.x
    );
    LONG y = IMAGE_MIRROR_COMPONENT(
        info->m_screenPosition.m_y,
        m_origin.y,
        info->m_plotOffset.m_y,
        m_anchor.y
    );
    DECLARE_IMAGE_DEST_EXTENTS(info, x, y, right, bottom);
    DECLARE_CLIPPED_IMAGE_RECT(RECT, d, info, dst, x, y, right, bottom, w, h)
    RECT s;
    s.left = right - d.right;
    s.top = bottom - d.bottom;
    s.right = s.left + w;
    s.bottom = s.top + h;
    g_bltFx.dwDDFX = DDBLTFX_MIRRORLEFTRIGHT | DDBLTFX_MIRRORUPDOWN;
    d.right += 1;
    d.bottom += 1;
    dst->GetSurface()->BltEx(&d, m_surface, &s, DDBLT_DDFX | DDBLT_KEYSRC, &g_bltFx);
    d.right -= 1;
    d.bottom -= 1;
    info->m_dirty.Set(d, w, h);
}

RVA(0x00153b20, 0x270)
void CImage::BlitFlipV(CResolveNode* info, CDDrawSurfacePair* dst) {
    LONG x = info->m_screenPosition.m_x - info->m_plotOffset.m_x - m_anchor.x - m_origin.x;
    LONG y = IMAGE_POSITION_COMPONENT(
        m_origin.y,
        m_anchor.y,
        info->m_plotOffset.m_y,
        info->m_screenPosition.m_y
    );
    DECLARE_IMAGE_DEST_EXTENTS(info, x, y, right, bottom);
    DECLARE_CLIPPED_IMAGE_RECT(RECT, d, info, dst, x, y, right, bottom, w, h)
    RECT s;
    SET_RECT_COMPONENTS(s, right - d.right, d.top - y, s.left + w, s.top + h);
    d.right += 1;
    d.bottom += 1;
    g_bltFx.dwDDFX = DDBLTFX_MIRRORLEFTRIGHT;
    dst->GetSurface()->BltEx(&d, m_surface, &s, DDBLT_DDFX | DDBLT_KEYSRC, &g_bltFx);
    d.right -= 1;
    d.bottom -= 1;
    info->m_dirty.Set(d, w, h);
}

RVA(0x00153d90, 0x259)
void CImage::BlitFlipH(CResolveNode* info, CDDrawSurfacePair* dst) {
    LONG x = info->m_plotOffset.m_x - m_anchor.x + m_origin.x + info->m_screenPosition.m_x;
    LONG y = info->m_screenPosition.m_y - m_origin.y - m_anchor.y - info->m_plotOffset.m_y;
    DECLARE_IMAGE_DEST_EXTENTS(info, x, y, right, bottom);
    DECLARE_CLIPPED_IMAGE_RECT(RECT, d, info, dst, x, y, right, bottom, w, h)
    RECT s;
    SET_RECT_COMPONENTS(s, d.left - x, bottom - d.bottom, s.left + w, s.top + h);
    d.right += 1;
    d.bottom += 1;
    g_bltFx.dwDDFX = DDBLTFX_MIRRORUPDOWN;
    dst->GetSurface()->BltEx(&d, m_surface, &s, DDBLT_DDFX | DDBLT_KEYSRC, &g_bltFx);
    d.right -= 1;
    d.bottom -= 1;
    info->m_dirty.Set(d, w, h);
}

RVA(0x00153ff0, 0x280)
void CImage::BlitShadeFlipHV(CResolveNode* info, CDDrawSurfacePair* dst) {
    LONG x = info->m_screenPosition.m_x - m_anchor.x + m_origin.x + info->m_plotOffset.m_x;
    LONG y = info->m_screenPosition.m_y - m_anchor.y + m_origin.y + info->m_plotOffset.m_y;
    DECLARE_IMAGE_DEST_EXTENTS(info, x, y, right, bottom);
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
    m_owned->Blit(&d, dst->GetSurface(), &s, 0, 0);
    info->m_dirty.m_lastPosition.x = d.left;
    info->m_dirty.m_lastPosition.y = d.top;
    info->m_dirty.m_rect = *(&d);
    info->m_dirty.m_size.cx = w;
    info->m_dirty.m_size.cy = h;
    info->m_dirty.m_armed = 0;
}

RVA(0x00154270, 0x257)
void CImage::BlitShadeNorm(CResolveNode* info, CDDrawSurfacePair* dst) {
    LONG x = info->m_screenPosition.m_x - m_origin.x - m_anchor.x - info->m_plotOffset.m_x;
    LONG y = info->m_screenPosition.m_y - m_origin.y - m_anchor.y - info->m_plotOffset.m_y;
    DECLARE_IMAGE_DEST_EXTENTS(info, x, y, right, bottom);
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
    SET_DIRTY_RECT(info, &d, w, h);
}

RVA(0x001544d0, 0x275)
void CImage::BlitShadeFlipV(CResolveNode* info, CDDrawSurfacePair* dst) {
    LONG x = info->m_screenPosition.m_x - m_anchor.x - info->m_plotOffset.m_x - m_origin.x;
    LONG y = m_origin.y + info->m_plotOffset.m_y + info->m_screenPosition.m_y - m_anchor.y;
    DECLARE_IMAGE_DEST_EXTENTS(info, x, y, right, bottom);
    DECLARE_CLIPPED_IMAGE_RECT(ShadeRect, d, info, dst, x, y, right, bottom, w, h)
    ShadeRect s;
    s.left = d.left - x;
    s.top = d.top - y;
    s.right = s.left + w - 1;
    s.bottom = s.top + h - 1;
    if (info->m_drawActive) {
        m_owned->Select(info->m_drawFillCmd, info->m_drawFillArg);
    }
    m_owned->Blit(&d, dst->GetSurface(), &s, 1, 0);
    info->m_dirty.m_lastPosition.x = d.left;
    info->m_dirty.m_lastPosition.y = d.top;
    info->m_dirty.m_rect = *(&d);
    info->m_dirty.m_size.cx = w;
    info->m_dirty.m_size.cy = h;
    info->m_dirty.m_armed = 0;
}

RVA(0x00154750, 0x275)
void CImage::BlitShadeFlipH(CResolveNode* info, CDDrawSurfacePair* dst) {
    LONG x = info->m_plotOffset.m_x + m_origin.x + info->m_screenPosition.m_x - m_anchor.x;
    LONG y = IMAGE_MIRROR_COMPONENT(
        info->m_screenPosition.m_y,
        m_origin.y,
        info->m_plotOffset.m_y,
        m_anchor.y
    );
    DECLARE_IMAGE_DEST_EXTENTS(info, x, y, right, bottom);
    DECLARE_CLIPPED_IMAGE_RECT(ShadeRect, d, info, dst, x, y, right, bottom, w, h)
    ShadeRect s;
    s.left = d.left - x;
    s.top = bottom - d.bottom;
    s.right = s.left + w - 1;
    s.bottom = s.top + h - 1;
    if (info->m_drawActive) {
        m_owned->Select(info->m_drawFillCmd, info->m_drawFillArg);
    }
    m_owned->Blit(&d, dst->GetSurface(), &s, 0, 1);
    info->m_dirty.Set(d, w, h);
}
