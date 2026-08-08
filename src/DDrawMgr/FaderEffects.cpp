#include <rva.h>

#include <Mfc.h>

#include <DDrawMgr/ColorDepth.h>
#include <DDrawMgr/DDrawPtrCollections.h>
#include <DDrawMgr/DDSurface.h>
#include <DDrawMgr/DirectDrawMgr.h>
#include <DDrawMgr/PixelShift.h>
#include <DDrawMgr/ShadeTableCache.h>
#include <EmptyString.h>
#include <Gruntz/Fader.h>
#include <Gruntz/FaderMode.h>
#include <Gruntz/FaderSubtypes.h>
#include <Gruntz/FxModeDesc.h>
#include <Gruntz/FxModeT1.h>
#include <Ints.h>
#include <Utils/RecordFill.h>
#include <Wap32/ScreenGeometry.h>

#include <ddraw.h>
#include <math.h>
#include <string.h>

DATA(0x001f07ec)
float g_fxBias = -50.0f;
DATA(0x001f07f4)
float g_fxEps = 1.0f;

DATA(0x001f080c)
const float g_faderHalfPi = 1.570795f;
DATA(0x001f0828)
const float g_faderHalf = 0.5f;
DATA(0x001f0830)
const double g_faderScale = 10000.0;
DATA(0x001f0838)
const double g_faderBiasR = -1.0;
DATA(0x001f0840)
const float g_faderBiasFade = -1.0f;
DATA(0x001f0844)
const float g_faderOne = 1.0f;
DATA(0x001f085c)
const float g_faderScale_5f085c = 0.01f;

DATA(0x001f0860)
const float g_sineHalfPi = 1.570795f;
DATA(0x001f0864)
const float g_sineOne = 1.0f;
DATA(0x001f0888)
const double g_faderPowK = 2.0;

DATA(0x002c3fc8)
i32 g_val_2c3fc8;

RVA(0x0017f530, 0x19)
CFaderFlat::CFaderFlat() {
    m_frames = NULL;
}

RVA_COMPGEN(0x0017f550, 0x1e, ??_GCFaderFlat@@UAEPAXI@Z)

RVA(0x0017f570, 0x61)
CFaderFlat::~CFaderFlat() {
    if (m_frames) {
        delete[] m_frames;
        m_frames = NULL;
    }
}

RVA(0x0017f5e0, 0x7d)
i32 CFaderFlat::ApplyInit(CFxModeDesc* desc) {
    CFxModeT5* s = static_cast<CFxModeT5*>(desc);
    if (s->m_targetSurface == NULL) {
        m_desc04 = m_timerA;
    } else {
        m_desc04 = s->m_targetSurface;
    }
    if (s->m_sourceSurface == NULL) {
        m_src = m_timerB;
    } else {
        m_src = s->m_sourceSurface;
    }
    m_desc0c = s->m_param0c;
    m_percent = s->m_durationPercent;
    m_desc14 = s->m_splitPercent;
    m_previousFrame = 0;
    m_frames = new i32[m_src->m_height];
    for (i32 i = 0; i < m_src->m_height; i++) {
        m_frames[i] = 0;
    }
    return 1;
}

RVA(0x0017f660, 0x2e6)
void CFaderFlat::RenderFrame(i32 frame) {
    u16* srcBits = static_cast<u16*>(m_src->Lock(0));
    u16* dstBits = static_cast<u16*>(m_desc04->Lock(0));
    i32 h = m_src->m_height;
    i32 w = m_src->m_width;
    i32 span = m_percent * h / 100;
    i32 base = h - frame - 1;
    if (span + base > h) {
        span = h - base;
    }
    i32 half = (m_desc14 * w / 100) / 2 + w / 2;
    i32 rest = w - half;
    i32 end = span + base;
    i32 y = (base < 0) ? 0 : base;
    while (y < end) {
        double s = sin(static_cast<double>(y - base) / span * g_faderHalfPi);
        i32 n1 = static_cast<i32>(s * half);
        i32 n2 = static_cast<i32>(s * rest);
        memcpy(
            dstBits + m_desc04->m_pitch * y / 2,
            srcBits + m_src->m_pitch * y / 2 + half - n1,
            n1 * 2
        );
        memcpy(
            dstBits + m_desc04->m_pitch * y / 2 + w - n2,
            srcBits + m_src->m_pitch * y / 2 + half,
            n2 * 2
        );
        y++;
        memcpy(
            dstBits + m_desc04->m_pitch * y / 2,
            srcBits + m_src->m_pitch * y / 2 + rest - n2,
            n2 * 2
        );
        memcpy(
            dstBits + m_desc04->m_pitch * y / 2 + w - n1,
            srcBits + m_src->m_pitch * y / 2 + rest,
            n1 * 2
        );
        y++;
    }
    i32 lastRow = h - 1;
    i32 y0 = lastRow;
    if (y0 >= end) {
        y0 = end;
    }
    i32 y2 = y0;
    for (;;) {
        i32 stop = y0 + frame - m_previousFrame;
        if (lastRow < stop) {
            stop = lastRow;
        }
        if (y2 >= stop) {
            break;
        }
        memcpy(dstBits + m_desc04->m_pitch * y2 / 2, srcBits + m_src->m_pitch * y2 / 2, w * 2);
        y2++;
    }
    m_previousFrame = frame;

    m_src->m_ddSurface->Unlock(0);
    m_desc04->m_ddSurface->Unlock(0);
}

RVA(0x0017f950, 0x24)
i32 CFaderFlat::GetFrameCount() {
    i32 n = m_src->m_height;
    return n + (m_percent * n) / 100;
}

RVA(0x0017f9a0, 0x24)
CFaderRadial::CFaderRadial() {
    m_maxRadius = 0;
    m_reserved40 = 0;
    m_cells = NULL;
    m_reserved48 = 1;
}
RVA_COMPGEN(0x0017f9d0, 0x1e, ??_GCFaderRadial@@UAEPAXI@Z)

RVA(0x0017f9f0, 0x4f)
CFaderRadial::~CFaderRadial() {
    FreeBuffer();
}

// @early-stop
// Retail spills the inner `x` counter and keeps m_srcSurface in edi across the
// Lock/Unlock pair; cl does the reverse, so its frame is one dword smaller.
RVA(0x0017fa40, 0x1f3)
i32 CFaderRadial::ApplyInit(CFxModeDesc* desc) {
    CFxModeT4* cfg = static_cast<CFxModeT4*>(desc);
    if (cfg->m_targetSurface == NULL) {
        m_dstSurface = m_timerA;
    } else {
        m_dstSurface = cfg->m_targetSurface;
    }

    if (cfg->m_sourceSurface == NULL) {
        m_srcSurface = m_timerB;
    } else {
        m_srcSurface = cfg->m_sourceSurface;
    }

    if (cfg->m_shadeTable == NULL) {

        CDDPalette* pal = cfg->m_palette;
        m_table = m_cache.HueRampTable(pal->m_cacheA, 0x10, 0);
        m_flag = 1;
    } else {
        m_table = cfg->m_shadeTable;
        m_flag = 0;
    }
    if (m_table == NULL) {
        return 0;
    }

    CDDSurface* s = m_srcSurface;
    m_fadeDivisor = static_cast<float>(s->m_width) * g_faderHalf;
    m_centerX = s->m_width / 2;
    m_centerY = s->m_height / 2;
    m_cells = new CFaderRadialCell[s->m_height * s->m_width];

    i32 cx = m_centerX;
    i32 cy = m_centerY;
    m_maxRadius = static_cast<i32>((sqrt(static_cast<double>((cx * cx + cy * cy))) * g_faderScale));

    for (i32 y = 0; y < m_srcSurface->m_height; y++) {
        for (i32 x = 0; x < m_srcSurface->m_width; x++) {
            i32 dx = x - m_centerX;
            i32 dy = y - m_centerY;
            CFaderRadialCell cell;
            cell.m_radius = static_cast<float>(
                (static_cast<double>(m_maxRadius)
                 - sqrt(static_cast<double>((dx * dx + dy * dy))) * g_faderScale - g_faderBiasR)
            );
            float fade = cell.m_radius / m_fadeDivisor - g_faderBiasFade;
            cell.m_vx = static_cast<float>(dx) * fade;
            cell.m_vy = static_cast<float>(m_centerY - y) * fade;
            u8* base = static_cast<u8*>(m_srcSurface->Lock(0));
            if (base != NULL) {
                u8 pix = *static_cast<u8*>(
                    (base + m_srcSurface->m_bytesPerPixel * x + m_srcSurface->m_pitch * y)
                );
                m_srcSurface->m_ddSurface->Unlock(0);
                cell.m_pixel = pix;
            } else {
                cell.m_pixel = 0;
            }
            m_cells[y * m_srcSurface->m_width + x] = cell;
        }
    }
    return 1;
}

RVA(0x0017fc40, 0x11)
void CFaderRadial::FreeBuffer() {
    if (m_cells) {
        delete[] m_cells;
    }
}

// @early-stop
RVA(0x0017fc60, 0x136)
void CFaderRadial::RenderFrame(i32 frame) {
    u8* scratch = new u8[m_dstSurface->m_width];
    m_dstSurface->Clear(0);
    m_srcSurface->Lock(0);
    u8* base = static_cast<u8*>(m_dstSurface->Lock(0));
    if (m_table->m_data == NULL) {
        return;
    }

    for (i32 i = 0; i < m_srcSurface->m_width * m_srcSurface->m_height; i++) {
        CFaderRadialCell* c = &m_cells[i];
        float d = c->m_radius - static_cast<float>(static_cast<u32>(frame));
        if (d > g_faderOne) {
            float sf = d / m_fadeDivisor - g_faderBiasFade;
            i32 px = m_centerX + static_cast<i32>((c->m_vx / sf));
            i32 py = m_centerY - static_cast<i32>((c->m_vy / sf));
            if (px > 0 && px < m_dstSurface->m_width && py > 0 && py < m_dstSurface->m_height) {
                (base)[py * m_dstSurface->m_pitch + px] = c->m_pixel;
            }
        }
    }

    m_srcSurface->m_ddSurface->Unlock(0);
    m_dstSurface->m_ddSurface->Unlock(0);
    delete[] scratch;
}
RVA(0x0017fda0, 0x8)
i32 CFaderRadial::GetFrameCount() {
    return m_maxRadius;
}

RVA(0x0017fdb0, 0x1a)
CFaderSine::CFaderSine() {
    m_elemCount = 0;
    m_frameCount = 0;
}

RVA_COMPGEN(0x0017fdd0, 0x1e, ??_GCFaderSine@@UAEPAXI@Z)
RVA(0x0017fdf0, 0xb)
CFaderSine::~CFaderSine() {}

// @early-stop
// Block structure and both surface-pair shapes agree. The residue is register
// colouring: retail lets the m_dstBox value die and RELOADS the member for the
// `if (!m_dstBox)` guard, and its inlined GetRandom keeps the `inc` next to the
// `idiv`; cl keeps the alias in a register and hoists the `inc`.
RVA(0x0017fe00, 0x12d)
i32 CFaderSine::ApplyInit(CFxModeDesc* desc) {
    CFxModeT3* cfg = static_cast<CFxModeT3*>(desc);
    i32 w;
    i32 p;
    i32 i;
    m_previousFrame = 0;
    m_boxParam = cfg->m_clearToBlack;
    if (cfg->m_targetSurface == NULL) {
        m_srcBox = m_timerA;
    } else {
        m_srcBox = cfg->m_targetSurface;
    }
    CDDSurface* alt = cfg->m_sourceSurface;
    if (!alt) {
        alt = m_timerB;
    }
    m_dstBox = alt;
    if (!m_srcBox) {
        goto fail;
    }
    if (!m_dstBox) {
        m_boxParam = 1;
    }
    m_elemCount = m_srcBox->m_width;
    w = m_srcBox->m_height;
    m_frameCount = w;
    p = cfg->m_intensityPercent;

    if (p < 0 || p > 100) {
        goto fail;
    }
    m_intensity = p;
    m_scaledMag = static_cast<i32>(w * (static_cast<float>(p) * g_faderScale_5f085c));
    for (i = 0; i < 2000; i++) {
        m_arr0[i] = 0;
        m_arr2[i] = 0;
        m_arr3[i] = 0;
        m_arr1[i] = GetRandom(0, m_elemCount - 1);
    }
    ScatterSamples(m_arr3, 0, m_elemCount, 1);
    return 1;
fail:
    return 0;
}

// @early-stop
RVA(0x0017ff30, 0x4c2)
void CFaderSine::RenderFrame(i32 frame) {
    if (frame == 0) {
        return;
    }
    if (m_srcBox != NULL) {
        m_srcBits = static_cast<u8*>(m_srcBox->Lock(0));
    }
    if (m_dstBox != NULL) {
        m_dstBits = static_cast<u8*>(m_dstBox->Lock(0));
    }
    i32 bpp = m_srcBox->m_bytesPerPixel;
    float step = static_cast<float>(m_elemCount) / m_scaledMag;
    i32 row = m_frameCount - frame;
    while (row < m_frameCount - frame + m_scaledMag) {
        if (row >= 0 && row < m_frameCount) {
            u8* srcRow = m_srcBits + m_srcBox->m_pitch * row;

            i32 delta = static_cast<i32>(
                            sin(static_cast<double>(static_cast<u32>(row + frame - m_frameCount))
                                / m_scaledMag * g_sineHalfPi)
                            * m_elemCount / step
                        )
                        - m_arr0[row];
            if (m_boxParam != 0) {

                i32 n = 0;
                double want = delta * step;
                i32 whole = static_cast<i32>(want);
                if (whole < want) {
                    m_arr2[row] = static_cast<float>(want - whole) + m_arr2[row];
                }
                if (m_arr2[row] >= g_sineOne) {
                    n = static_cast<i32>(m_arr2[row]);
                    m_arr2[row] = m_arr2[row] - n;
                }
                n += whole;
                while (n > 0) {
                    ++m_arr1[row];
                    if (m_arr1[row] > m_elemCount) {
                        m_arr1[row] = 0;
                    }
                    i32 pick = m_arr3[m_arr1[row]];
                    if (bpp > 0) {
                        memset(srcRow + pick * bpp, 0, bpp);
                    }
                    n--;
                }
                m_arr0[row] += delta;
                n = static_cast<i32>(step + step);
                while (n > 0) {
                    i32 pick = GetRandom(0, m_elemCount - 1);
                    if (bpp > 0) {
                        memset(srcRow + pick * bpp, 0, bpp);
                    }
                    n--;
                }
            } else {
                i32 n = 0;
                u8* dstRow = m_dstBits + m_dstBox->m_pitch * row;
                double want = delta * step;
                i32 whole = static_cast<i32>(want);
                if (whole < want) {
                    m_arr2[row] = static_cast<float>(want - whole) + m_arr2[row];
                }
                if (m_arr2[row] >= g_sineOne) {
                    n = static_cast<i32>(m_arr2[row]);
                    m_arr2[row] = m_arr2[row] - n;
                }
                n += whole;
                while (n > 0) {
                    ++m_arr1[row];
                    if (m_arr1[row] > m_elemCount) {
                        m_arr1[row] = 0;
                    }
                    i32 pick = m_arr3[m_arr1[row]];
                    if (bpp > 0) {
                        u8* p = srcRow + pick * bpp;
                        u8* q = dstRow + pick * bpp;
                        for (i32 j = 0; j < bpp; j++) {
                            p[j] = q[j];
                        }
                    }
                    n--;
                }
                m_arr0[row] += delta;
                n = static_cast<i32>(step + step);
                while (n > 0) {
                    i32 pick = GetRandom(0, m_elemCount - 1);
                    if (bpp > 0) {
                        u8* p = srcRow + pick * bpp;
                        u8* q = dstRow + pick * bpp;
                        for (i32 j = 0; j < bpp; j++) {
                            p[j] = q[j];
                        }
                    }
                    n--;
                }
            }
        }
        row++;
    }

    i32 y = m_previousFrame;
    while (y < frame) {
        i32 done = m_scaledMag - y + m_frameCount - 1;
        if (done >= 0 && done < m_frameCount) {
            if (m_boxParam != 0) {
                i32 span = bpp * m_elemCount;
                if (span > 0) {
                    memset(m_srcBits + m_srcBox->m_pitch * done, 0, span);
                }
            } else {
                u8* dst = m_dstBits + m_dstBox->m_pitch * done;
                u8* src = m_srcBits + m_srcBox->m_pitch * done;
                i32 span = bpp * m_elemCount;
                for (i32 j = 0; j < span; j++) {
                    src[j] = dst[j];
                }
            }
        }
        y++;
    }
    m_previousFrame = frame;
    if (m_srcBox != NULL) {
        m_srcBox->m_ddSurface->Unlock(0);
    }
    if (m_dstBox != NULL) {
        m_dstBox->m_ddSurface->Unlock(0);
    }
}

RVA(0x00180400, 0xa)
i32 CFaderSine::GetFrameCount() {
    return m_scaledMag + m_frameCount;
}

RVA(0x00180410, 0x19)
CFaderLight::CFaderLight() {
    m_overlay = NULL;
}

RVA_COMPGEN(0x00180430, 0x1e, ??_GCFaderLight@@UAEPAXI@Z)
RVA(0x00180450, 0x4f)
CFaderLight::~CFaderLight() {
    SubFree();
}

// @early-stop
// 29/29 blocks. Residue: retail re-materialises the palette pointer through eax
// before the null test and stores m_flag = 1 as an immediate rather than sharing
// the return value's register.
RVA(0x001804a0, 0x182)
i32 CFaderLight::ApplyInit(CFxModeDesc* desc) {
    CFxModeT2* d = static_cast<CFxModeT2*>(desc);
    m_previousFrame = 0;
    if (d->m_targetSurface == NULL) {
        m_surface = m_timerA;
    } else {
        m_surface = d->m_targetSurface;
    }
    if (d->m_sourceSurface == NULL) {
        m_dstSurface = m_timerB;
    } else {
        m_dstSurface = d->m_sourceSurface;
    }
    m_lightGate = d->m_clearMode;
    m_centerX = d->m_centerX;
    m_centerY = d->m_centerY;
    CDDPalette* pal = d->m_palette;
    m_palette = pal;
    i32 cnt = d->m_spanCount;
    m_spanCount = cnt;
    if (cnt > 0 && d->m_shadeTable == NULL && pal == NULL) {
        return 0;
    }
    if (m_surface == NULL) {
        return 0;
    }
    if (m_dstSurface == NULL && m_lightGate == 0) {
        return 0;
    }
    RECT rect;
    rect.right = m_surface->m_width;
    m_surfWidth = rect.right;
    rect.bottom = m_surface->m_height;
    m_surfHeight = rect.bottom;
    rect.left = 0;
    rect.top = 0;
    POINT pt;
    pt.x = m_centerX;
    pt.y = m_centerY;
    if (PtInRect(&rect, pt) == 0) {
        return 0;
    }
    if (m_lightGate != 0) {
        i32 i = 0;
        if (m_surfHeight > 0) {
            do {
                m_spanStarts[i] = 0;
                m_spanEnds[i] = m_surfWidth;
                i++;
            } while (i < m_surfHeight);
        }
    } else {
        i32 i = 0;
        if (m_surfHeight > 0) {
            do {
                m_spanStarts[i] = m_centerX;
                m_spanEnds[i] = m_centerX;
                i++;
            } while (i < m_surfHeight);
        }
    }
    if (m_spanCount > 0) {
        if (d->m_shadeTable == NULL) {
            m_table = m_cache.HueRampTable(m_palette->m_cacheA, m_spanCount, 0);
            m_flag = 1;
            return 1;
        }
        m_table = d->m_shadeTable;
    }
    return 1;
}

RVA(0x00180630, 0x1)
void CFaderLight::SubFree() {}

#define FADER_CLAMPW(v, w) (((v) < 0 ? 0 : (v)) >= (w) ? (w) : ((v) < 0 ? 0 : (v)))

// @early-stop
RVA(0x00180640, 0x96c)
void CFaderLight::RenderFrame(i32 frame) {
    i32 delta = frame - m_previousFrame;
    if (m_surface != NULL) {
        m_srcBits = static_cast<u8*>(m_surface->Lock(0));
    }
    if (m_dstSurface != NULL) {
        m_dstBits = static_cast<u8*>(m_dstSurface->Lock(0));
    }
    i32 bpp = m_surface->m_bytesPerPixel;
    u8* lut = 0;
    if (m_table != NULL) {
        lut = m_table->m_data;
    }
    if (m_lightGate != 0) {
        u8* ovlBits = 0;
        if (m_overlay != NULL) {
            ovlBits = static_cast<u8*>(m_overlay->Lock(0));
        }
        i32 r = m_frameCount - frame;
        i32 rr = r * r;
        i32 v = m_centerY - r - delta;
        i32 row = (v < 0) ? 0 : v;
        for (;;) {
            i32 stop = delta + r + m_centerY;
            if (stop >= m_surfHeight) {
                stop = m_surfHeight;
            }
            if (row >= stop) {
                break;
            }
            if (row >= m_centerY - r + 1 && row <= r + m_centerY - 1) {
                i32 dyv = row - m_centerY;

                i32 hv = -static_cast<i32>(sqrt(static_cast<double>((rr - dyv * dyv))));
                i32 right = FADER_CLAMPW(m_centerX - hv, m_surfWidth);
                i32 lv = hv + m_centerX + 1;
                i32 left = (lv < 0) ? 0 : lv;
                if (left >= m_surfWidth) {
                    left = m_surfWidth;
                }
                i32 oldStart = m_spanStarts[row];
                i32 n1 = (left - oldStart) * bpp;
                if (n1 > 0) {
                    memset(m_srcBits + m_surface->m_pitch * row + oldStart * bpp, 0, n1);
                }
                i32 oldEnd = m_spanEnds[row];
                i32 n2 = (oldEnd - right) * bpp;
                if (n2 > 0) {
                    memset(m_srcBits + m_surface->m_pitch * row + right * bpp, 0, n2);
                }
                u8* bits = m_srcBits;
                if (m_spanCount > 0) {
                    i32 cy = m_centerY;
                    i32 dy = row - cy;
                    i32 dy2 = dy * dy;
                    i32 xcur =
                        m_centerX - static_cast<i32>(sqrt(static_cast<double>((rr - dy2)))) + 1;
                    i32 dist = static_cast<i32>(
                        sqrt(static_cast<double>(((xcur - m_centerX) * (xcur - m_centerX) + dy2)))
                    );
                    i32 srcpitch = m_surface->m_pitch;
                    i32 srcCol = row * srcpitch;
                    u8* rowLsrc = bits + xcur + srcCol;
                    i32 dstpitch = m_dstSurface->m_pitch;
                    i32 dstCol = row * dstpitch;
                    u8* rowLdst = ovlBits + xcur + dstCol;
                    u8* rowRsrc = (bits - xcur) + srcCol + 2 * m_centerX;
                    u8* rowRdst = (ovlBits - xcur) + dstCol + 2 * m_centerX;
                    i32 mid = m_surfHeight / 2;
                    i32 mirSrc;
                    i32 mirDst;
                    if (cy >= mid && row <= cy) {
                        i32 mirRow = 2 * (cy - row);
                        if (mirRow + row < m_surfHeight) {

                            mirSrc = mirRow * srcpitch;
                            mirDst = mirRow * dstpitch;
                            while (dist >= r - m_spanCount) {
                                if (xcur > m_centerX) {
                                    break;
                                }
                                i32 cl = dist - r + m_spanCount;
                                if (xcur >= 0) {
                                    i32 p = *rowLdst;
                                    *rowLsrc = *(lut + p * m_spanCount + cl);
                                    i32 q = *(rowLdst + mirDst);
                                    *(rowLsrc + mirSrc) = *(lut + q * m_spanCount + cl);
                                }
                                rowLsrc++;
                                rowLdst++;
                                if (2 * m_centerX - xcur < m_surfWidth) {
                                    i32 p = *rowRdst;
                                    *rowRsrc = *(lut + p * m_spanCount + cl);
                                    i32 q = *(rowRdst + mirDst);
                                    *(rowRsrc + mirSrc) = *(lut + q * m_spanCount + cl);
                                }
                                rowRsrc--;
                                rowRdst--;
                                xcur++;
                                dist = static_cast<i32>(sqrt(
                                    static_cast<double>(
                                        ((xcur - m_centerX) * (xcur - m_centerX) + dy2)
                                    )
                                ));
                            }
                        } else {
                            while (dist >= r - m_spanCount) {
                                if (xcur > m_centerX) {
                                    break;
                                }
                                i32 cl = dist - r + m_spanCount;
                                if (xcur >= 0) {
                                    i32 p = *rowLdst;
                                    *rowLsrc = *(lut + p * m_spanCount + cl);
                                }
                                rowLsrc++;
                                rowLdst++;
                                if (2 * m_centerX - xcur < m_surfWidth) {
                                    i32 p = *rowRdst;
                                    *rowRsrc = *(lut + p * m_spanCount + cl);
                                }
                                rowRsrc--;
                                rowRdst--;
                                xcur++;
                                dist = static_cast<i32>(sqrt(
                                    static_cast<double>(
                                        ((xcur - m_centerX) * (xcur - m_centerX) + dy2)
                                    )
                                ));
                            }
                        }
                    } else if (cy < mid && row >= cy) {
                        i32 mirRow = 2 * dy;
                        if (row - mirRow >= 0) {

                            mirSrc = mirRow * srcpitch;
                            mirDst = mirRow * dstpitch;
                            while (dist >= r - m_spanCount) {
                                if (xcur > m_centerX) {
                                    break;
                                }
                                i32 cl = dist - r + m_spanCount;
                                if (xcur >= 0) {
                                    i32 p = *rowLdst;
                                    *rowLsrc = *(lut + p * m_spanCount + cl);
                                    i32 q = *(rowLdst - mirDst);
                                    *(rowLsrc - mirSrc) = *(lut + q * m_spanCount + cl);
                                }
                                rowLsrc++;
                                rowLdst++;
                                if (2 * m_centerX - xcur < m_surfWidth) {
                                    i32 p = *rowRdst;
                                    *rowRsrc = *(lut + p * m_spanCount + cl);
                                    i32 q = *(rowRdst - mirDst);
                                    *(rowRsrc - mirSrc) = *(lut + q * m_spanCount + cl);
                                }
                                rowRsrc--;
                                rowRdst--;
                                xcur++;
                                dist = static_cast<i32>(sqrt(
                                    static_cast<double>(
                                        ((xcur - m_centerX) * (xcur - m_centerX) + dy2)
                                    )
                                ));
                            }
                        } else {

                            while (dist >= r - m_spanCount) {
                                if (xcur > m_centerX) {
                                    break;
                                }
                                i32 cl = dist - r + m_spanCount;
                                if (xcur >= 0) {
                                    i32 p = *rowLdst;
                                    *rowLsrc = *(lut + p * m_spanCount + cl);
                                }
                                rowLsrc++;
                                rowLdst++;
                                if (2 * m_centerX - xcur < m_surfWidth) {
                                    i32 p = *rowRdst;
                                    *rowRsrc = *(lut + p * m_spanCount + cl);
                                }
                                rowRsrc--;
                                rowRdst--;
                                xcur++;
                                dist = static_cast<i32>(sqrt(
                                    static_cast<double>(
                                        ((xcur - m_centerX) * (xcur - m_centerX) + dy2)
                                    )
                                ));
                            }
                        }
                    }
                }
                m_spanStarts[row] = left;
                m_spanEnds[row] = right;
            } else {
                i32 w = m_surfWidth;
                if (w > 0) {
                    memset(m_srcBits + m_surface->m_pitch * row, 0, w);
                }
            }
            row++;
        }
        if (m_overlay != NULL) {
            m_overlay->m_ddSurface->Unlock(0);
        }
    } else {
        i32 fr2 = frame * frame;
        i32 v = m_centerY - frame - delta - m_spanCount;
        i32 row = (v < 0) ? 0 : v;
        for (;;) {
            i32 stop = delta + frame + m_spanCount + m_centerY;
            if (stop >= m_surfHeight) {
                stop = m_surfHeight;
            }
            if (row >= stop) {
                break;
            }
            if (row > m_centerY - frame && row < frame + m_centerY) {
                i32 dyv = row - m_centerY;
                i32 hv = -static_cast<i32>(sqrt(static_cast<double>((fr2 - dyv * dyv))));
                i32 right = FADER_CLAMPW(m_centerX - hv, m_surfWidth);
                i32 lv = hv + m_centerX - 1;
                i32 left = (lv < 0) ? 0 : lv;
                if (left >= m_surfWidth) {
                    left = m_surfWidth;
                }
                i32 j;
                i32 n1 = (m_spanStarts[row] - left) * bpp;
                u8* src = m_dstBits + m_dstSurface->m_pitch * row + left * bpp;
                u8* dst = m_srcBits + m_surface->m_pitch * row + left * bpp;
                for (j = 0; j < n1; j++) {
                    dst[j] = src[j];
                }
                i32 oldEnd = m_spanEnds[row];
                i32 n2 = (right - oldEnd) * bpp;
                src = m_dstBits + m_dstSurface->m_pitch * row + oldEnd * bpp;
                dst = m_srcBits + m_surface->m_pitch * row + oldEnd * bpp;
                for (j = 0; j < n2; j++) {
                    dst[j] = src[j];
                }
                m_spanStarts[row] = left;
                m_spanEnds[row] = right;
            }
            if (row > m_centerY - frame - m_spanCount && row < frame + m_spanCount + m_centerY) {
                i32 rad = frame + m_spanCount - 1;
                Render(row, rad * rad, rad, lut, m_srcBits, m_dstBits);
            }
            row++;
        }
    }
    m_previousFrame = frame;
    if (m_surface != NULL) {
        m_surface->m_ddSurface->Unlock(0);
    }
    if (m_dstSurface != NULL) {
        m_dstSurface->m_ddSurface->Unlock(0);
    }
}

// @early-stop
RVA(0x00180fb0, 0x534)

void CFaderLight::Render(i32 row0, i32 radiusSq, i32 radius, u8* lut, u8* srcBits, u8* dstBits) {
    if (m_spanCount <= 0) {
        return;
    }
    i32 cx = m_centerY;
    i32 dx = row0 - cx;
    i32 dx2 = dx * dx;
    i32 row = m_centerX - static_cast<i32>(sqrt(static_cast<double>((radiusSq - dx2)))) + 1;
    i32 len =
        static_cast<i32>(sqrt(static_cast<double>(((row - m_centerX) * (row - m_centerX) + dx2))));

    i32 srcCol = row0 * m_surface->m_pitch;
    u8* rowLsrc = srcBits + row + srcCol;
    i32 dstCol = row0 * m_dstSurface->m_pitch;
    u8* rowLdst = dstBits + row + dstCol;
    u8* rowRsrc = srcBits - row;
    rowRsrc += srcCol;
    rowRsrc += 2 * m_centerX;
    u8* rowRdst = dstBits - row;
    rowRdst += dstCol;
    rowRdst += 2 * m_centerX;

    i32 mid = m_surfHeight / 2;
    i32 mirSrc;
    i32 mirDst;
    if (cx >= mid && row0 <= cx) {
        i32 mirCol = 2 * (cx - row0);
        if (mirCol + row0 < m_surfHeight) {

            mirSrc = mirCol * m_surface->m_pitch;
            mirDst = mirCol * m_dstSurface->m_pitch;
            while (len >= radius - m_spanCount) {
                if (row > m_centerX) {
                    return;
                }
                i32 cl = len - radius + m_spanCount;
                if (row >= 0) {
                    i32 p = *rowLdst;
                    *rowLsrc = *(lut + p * m_spanCount + cl);
                    i32 q = *(rowLdst + mirDst);
                    *(rowLsrc + mirSrc) = *(lut + q * m_spanCount + cl);
                }
                rowLsrc++;
                rowLdst++;
                if (2 * m_centerX - row < m_surfWidth) {
                    i32 p = *rowRdst;
                    *rowRsrc = *(lut + p * m_spanCount + cl);
                    i32 q = *(rowRdst + mirDst);
                    *(rowRsrc + mirSrc) = *(lut + q * m_spanCount + cl);
                }
                rowRsrc--;
                rowRdst--;
                row++;
                len = static_cast<i32>(
                    sqrt(static_cast<double>(((row - m_centerX) * (row - m_centerX) + dx2)))
                );
            }
            return;
        }

        while (len >= radius - m_spanCount) {
            if (row > m_centerX) {
                return;
            }
            i32 cl = len - radius + m_spanCount;
            if (row >= 0) {
                i32 p = *rowLdst;
                *rowLsrc = *(lut + p * m_spanCount + cl);
            }
            rowLsrc++;
            rowLdst++;
            if (2 * m_centerX - row < m_surfWidth) {
                i32 p = *rowRdst;
                *rowRsrc = *(lut + p * m_spanCount + cl);
            }
            rowRsrc--;
            rowRdst--;
            row++;
            len = static_cast<i32>(
                sqrt(static_cast<double>(((row - m_centerX) * (row - m_centerX) + dx2)))
            );
        }
        return;
    }

    if (cx >= mid || row0 < cx) {
        return;
    }

    i32 mirCol = 2 * dx;
    if (row0 - mirCol >= 0) {
        mirSrc = mirCol * m_surface->m_pitch;
        mirDst = mirCol * m_dstSurface->m_pitch;
        while (len >= radius - m_spanCount) {
            if (row > m_centerX) {
                return;
            }
            i32 cl = len - radius + m_spanCount;
            if (row >= 0) {
                i32 p = *rowLdst;
                *rowLsrc = *(lut + p * m_spanCount + cl);
                i32 q = *(rowLdst - mirDst);
                *(rowLsrc - mirSrc) = *(lut + q * m_spanCount + cl);
            }
            rowLsrc++;
            rowLdst++;
            if (2 * m_centerX - row < m_surfWidth) {
                i32 p = *rowRdst;
                *rowRsrc = *(lut + p * m_spanCount + cl);
                i32 q = *(rowRdst - mirDst);
                *(rowRsrc - mirSrc) = *(lut + q * m_spanCount + cl);
            }
            rowRsrc--;
            rowRdst--;
            row++;
            len = static_cast<i32>(
                sqrt(static_cast<double>(((row - m_centerX) * (row - m_centerX) + dx2)))
            );
        }
    } else {
        while (len >= radius - m_spanCount) {
            if (row > m_centerX) {
                return;
            }
            i32 cl = len - radius + m_spanCount;
            if (row >= 0) {
                i32 p = *rowLdst;
                *rowLsrc = *(lut + p * m_spanCount + cl);
            }
            rowLsrc++;
            rowLdst++;
            if (2 * m_centerX - row < m_surfWidth) {
                i32 p = *rowRdst;
                *rowRsrc = *(lut + p * m_spanCount + cl);
            }
            rowRsrc--;
            rowRdst--;
            row++;
            len = static_cast<i32>(
                sqrt(static_cast<double>(((row - m_centerX) * (row - m_centerX) + dx2)))
            );
        }
    }
}

RVA(0x001814f0, 0x16d)
i32 CFaderLight::GetFrameCount() {
    // The furthest corner from (m_centerX, m_centerY) sets the frame count.
    // windef.h's `max` macro, left-folded TL / BR / TR / BL: the ternary
    // duplicates its operands, which is why retail carries eight `fld` reloads
    // of dTopLeft and three separate __ftol/epilogue tails, not an if-chain.
    double pLeft = pow(static_cast<double>(m_centerX), g_faderPowK);
    double pTop = pow(static_cast<double>(m_centerY), g_faderPowK);
    double dTopLeft = sqrt(pLeft + pTop);
    double pBottom = pow(static_cast<double>(m_surface->m_height - m_centerY), g_faderPowK);
    double pRight = pow(static_cast<double>(m_surface->m_width - m_centerX), g_faderPowK);
    double dBottomRight = sqrt(pRight + pBottom);
    double dTopRight = sqrt(pRight + pTop);
    double dBottomLeft = sqrt(pLeft + pBottom);

    i32 r = static_cast<i32>(max(max(max(dTopLeft, dBottomRight), dTopRight), dBottomLeft));
    m_frameCount = r;
    return r;
}

RVA(0x00181660, 0x40)
void CFaderLight::BeginFade() {
    if (m_spanCount > 0 && m_lightGate != 0) {
        CDDSurface* h = m_ptrColl->MakeAndAddB(m_surfWidth, m_surfHeight, BPP_UNSET, 0, -1);
        m_overlay = h;
        h->Blt(m_surface);
    }
}

RVA(0x001816a0, 0x1c)
void CFaderLight::EndFade() {
    if (m_overlay) {
        m_ptrColl->RemoveItemA(m_overlay);
        m_overlay = NULL;
    }
}

RVA(0x001816c0, 0x32)
CFaderShape::CFaderShape() {
    m_warpTable = NULL;
    m_rowOfsA = NULL;
    m_rowOfsB = NULL;
    m_rowOfsC = NULL;
    m_lineBuf = NULL;
    m_shadeRamp = NULL;
    m_previousFrame = 0;
}

RVA_COMPGEN(0x00181700, 0x1e, ??_GCFaderShape@@UAEPAXI@Z)
RVA(0x00181720, 0xb3)
CFaderShape::~CFaderShape() {
    if (m_warpTable) {
        delete[] m_warpTable;
    }
    if (m_rowOfsA) {
        delete[] m_rowOfsA;
    }
    if (m_rowOfsB) {
        delete[] m_rowOfsB;
    }
    if (m_rowOfsC) {
        delete[] m_rowOfsC;
    }
    if (m_lineBuf) {
        delete[] m_lineBuf;
    }
    if (m_shadeRamp) {
        delete[] m_shadeRamp;
    }
}

// @early-stop
// The two signed/unsigned twins are fixed: retail carries the fader mode as an
// UNSIGNED word, so its range guards are `jbe`/`jae` (both m_mode fields are now
// GZ_ENUM_STORAGE(FaderMode, u32)). The rest is register/spill colouring plus one
// inlined-vs-shared fail epilogue.
RVA(0x001817e0, 0x315)
i32 CFaderShape::ApplyInit(CFxModeDesc* desc) {
    CFxModeT1* pInit = static_cast<CFxModeT1*>(desc);
    i32 i;
    i32 mx;
    m_previousFrame = 0;
    if (pInit == NULL) {
        goto fail;
    }

    if (pInit->m_targetSurface == NULL) {
        m_surfA = m_timerA;
    } else {
        m_surfA = pInit->m_targetSurface;
    }
    if (pInit->m_sourceSurface == NULL) {
        m_surfB = m_timerB;
    } else {
        m_surfB = pInit->m_sourceSurface;
    }
    if (m_surfA == NULL) {
        goto fail;
    }
    if (m_surfB == NULL) {
        goto fail;
    }
    if (pInit->m_warpSourceSurface == NULL) {
        m_surfC = m_surfB;
    } else {
        m_surfC = pInit->m_warpSourceSurface;
    }

    if (!m_cache.Init()) {
        goto fail;
    }

    m_span = m_surfA->m_width;
    m_rowCount = m_surfA->m_height;
    m_spanB = m_surfB->m_width;
    m_rowCountB = m_surfB->m_height;
    m_spanC = m_surfC->m_width;
    m_rowCountC = m_surfC->m_height;
    if (m_span != m_spanB) {
        goto fail;
    }
    if (m_rowCount != m_rowCountB) {
        goto fail;
    }
    if (m_span != m_spanC) {
        goto fail;
    }
    if (m_rowCount != m_rowCountC) {
        goto fail;
    }
    if (m_spanC != m_spanB) {
        goto fail;
    }
    if (m_rowCountC != m_rowCountB) {
        goto fail;
    }

    if (pInit->m_mode <= FADER_INVALID) {
        goto fail;
    }
    if (pInit->m_mode >= FADER_COUNT) {
        goto fail;
    }
    m_mode = pInit->m_mode;
    m_stripCopy = pInit->m_stripCopy;
    m_halfWidth = pInit->m_halfWidth;

    if (m_mode == FADER_SWEEP_FORWARD || m_mode == FADER_SWEEP_REVERSE) {
        if (m_span < static_cast<i32>((static_cast<double>(m_halfWidth) * 3.141592653589793))) {
            goto fail;
        }
    }

    m_warpTable = new i32[m_halfWidth * 2];
    for (i = 0; i < 2 * m_halfWidth; i++) {
        m_warpTable[i] = static_cast<i32>(
            (acos(
                 (static_cast<float>(i) - static_cast<float>(m_halfWidth))
                 / static_cast<float>(m_halfWidth)
             )
             * static_cast<float>(m_halfWidth))
        );
    }

    m_useLut = pInit->m_useLut;
    if (m_surfA->m_bitDepth != BPP_PALETTED_8) {
        m_useLut = 0;
    }

    if (m_useLut != 0) {
        if (pInit->m_shadeTable) {
            m_flag = 0;
            m_table = pInit->m_shadeTable;
        } else if (_access(pInit->m_shadeTablePath, 0) == 0) {
            m_table = m_cache.AddFromArray(pInit->m_shadeTablePath);
            if (m_table == NULL) {
                m_useLut = 0;
            }
        } else {
            CDDPalette* pal = pInit->m_palette;
            m_table = m_cache.FlashTable(pal->m_cacheA, 0x20, 0x20, 0x32, 0xc8);
        }

        i32 m = m_halfWidth << 1;
        m_shadeRamp = new u8[m];
        for (i = 0; i < m; i++) {
            i32 t = static_cast<i32>(
                (sin(static_cast<float>(i) / static_cast<float>(m) * 3.14f) * -32.0)
            );
            m_shadeRamp[i] = static_cast<u8>((0x10 - t));
        }
    }

    m_rowOfsA = new i32[m_rowCount];
    m_rowOfsB = new i32[m_rowCountB];
    m_rowOfsC = new i32[m_rowCountC];
    for (i = 0; i < m_rowCount; i++) {
        m_rowOfsA[i] = m_surfA->m_pitch * i;
        m_rowOfsB[i] = m_surfB->m_pitch * i;
        m_rowOfsC[i] = m_surfC->m_pitch * i;
    }

    mx = (m_rowCount > m_span) ? m_rowCount : m_span;
    m_lineBuf = new u8[m_surfA->m_bytesPerPixel * mx];
    return 1;
fail:
    return 0;
}

// @early-stop
RVA(0x00181b00, 0x34f)
void CFaderShape::RenderFrame(i32 frame) {
    m_dstBase = static_cast<u8*>(m_surfA->Lock(0));
    u8* gather = static_cast<u8*>(m_surfB->Lock(0));
    m_straightBase = gather;
    if (m_surfB != m_surfC) {
        gather = static_cast<u8*>(m_surfC->Lock(0));
    }
    m_gatherBase = gather;

    u32 seam = 0;
    i32 stride = m_halfWidth * 2;
    i32 arc = static_cast<i32>(static_cast<double>(m_halfWidth) * 3.14159);
    if (m_mode == FADER_SPLIT_FROM_CENTER && m_stripCopy != 0) {
        seam = m_span / 2;
    }
    if (m_stripCopy == 0 && frame == 0) {
        i32 n = m_surfB->m_pitch;
        if (m_surfA->m_pitch < n) {
            n = m_surfA->m_pitch;
        }
        i32 row = 0;
        while (row < m_rowCount) {
            u8* src = m_straightBase + m_rowOfsB[row];
            u8* dst = m_dstBase + m_rowOfsA[row];
            i32 x = 0;
            while (x < n) {
                *dst++ = *src++;
                x++;
            }
            row++;
        }
    }
    if (m_stripCopy != 0) {
        if (seam + frame <= static_cast<u32>(m_span - arc - m_halfWidth)) {
            switch (m_mode) {
                case FADER_SWEEP_FORWARD:
                    RenderTile(frame, frame - m_previousFrame);
                    break;
                case FADER_SWEEP_REVERSE:
                    RenderTile(m_span - frame - stride, frame - m_previousFrame);
                    break;
                case FADER_SPLIT_FROM_CENTER:
                    m_mode = FADER_SWEEP_FORWARD;
                    RenderTile(m_span / 2 + frame, frame - m_previousFrame);
                    m_mode = FADER_SWEEP_REVERSE;
                    RenderTile(m_span / 2 - frame - stride, frame - m_previousFrame);
                    m_mode = FADER_SPLIT_FROM_CENTER;
                    break;
            }
        } else {
            switch (m_mode) {
                case FADER_SWEEP_FORWARD:
                    RenderWarpTile(frame, frame - m_previousFrame);
                    break;
                case FADER_SWEEP_REVERSE:
                    RenderWarpTile(m_span - frame - stride, frame - m_previousFrame);
                    break;
                case FADER_SPLIT_FROM_CENTER:
                    m_mode = FADER_SWEEP_FORWARD;
                    RenderWarpTile(m_span / 2 + frame, frame - m_previousFrame);
                    m_mode = FADER_SWEEP_REVERSE;
                    RenderWarpTile(m_span - m_span / 2 - frame - stride, frame - m_previousFrame);
                    m_mode = FADER_SPLIT_FROM_CENTER;
                    break;
            }
        }
    } else {
        if (seam + frame > static_cast<u32>(arc - m_halfWidth)) {
            switch (m_mode) {
                case FADER_SWEEP_FORWARD:
                    RenderTile(frame, frame - m_previousFrame);
                    break;
                case FADER_SWEEP_REVERSE:
                    RenderTile(m_span - frame - stride, frame - m_previousFrame);
                    break;
                case FADER_SPLIT_FROM_CENTER:
                    m_mode = FADER_SWEEP_FORWARD;
                    RenderTile(frame, frame - m_previousFrame);
                    m_mode = FADER_SWEEP_REVERSE;
                    RenderTile(m_span - frame - stride, frame - m_previousFrame);
                    m_mode = FADER_SPLIT_FROM_CENTER;
                    break;
            }
        } else {
            switch (m_mode) {
                case FADER_SWEEP_FORWARD:
                    RenderWarpTile(frame, frame - m_previousFrame);
                    break;
                case FADER_SWEEP_REVERSE:
                    RenderWarpTile(m_span - frame - stride, frame - m_previousFrame);
                    break;
                case FADER_SPLIT_FROM_CENTER:
                    m_mode = FADER_SWEEP_FORWARD;
                    RenderWarpTile(frame, frame - m_previousFrame);
                    m_mode = FADER_SWEEP_REVERSE;
                    RenderWarpTile(m_span - frame - stride, frame - m_previousFrame);
                    m_mode = FADER_SPLIT_FROM_CENTER;
                    break;
            }
        }
    }
    m_previousFrame = frame;
    m_surfA->m_ddSurface->Unlock(0);
    m_surfB->m_ddSurface->Unlock(0);
    if (m_surfB != m_surfC) {
        m_surfC->m_ddSurface->Unlock(0);
    }
}

// @early-stop
RVA(0x00181e50, 0x7b9)

void CFaderShape::RenderWarpTile(i32 col, i32 stripWidth) {
    i32 stride = m_halfWidth * 2;
    if (stripWidth <= 0) {
        return;
    }
    i32 arc = static_cast<i32>((static_cast<double>(m_halfWidth) * 3.14159));
    i32 bpp = m_surfA->m_bytesPerPixel;

    i32 colBase;
    if ((m_mode == FADER_SWEEP_FORWARD && m_stripCopy != 0)
        || (m_mode == FADER_SWEEP_REVERSE && m_stripCopy == 0)) {
        i32 arcSpan = arc - m_halfWidth;
        i32 tail = m_span - col - stride;
        colBase = stride - static_cast<i32>(static_cast<double>(stride) / arcSpan * tail);
    } else {
        colBase = col;
    }
    if ((m_mode == FADER_SWEEP_FORWARD && m_stripCopy == 0)
        || (m_mode == FADER_SWEEP_REVERSE && m_stripCopy != 0)) {
        i32 arcSpan = arc - m_halfWidth;
        colBase = static_cast<i32>(static_cast<double>(stride) / arcSpan * col);
    }

    if ((m_mode == FADER_SWEEP_FORWARD && m_stripCopy != 0)
        || (m_mode == FADER_SWEEP_REVERSE && m_stripCopy == 0)) {
        i32 row = 0;
        if (m_rowCount > 0) {
            i32 base = bpp * col;
            do {
                u8* dstLine = m_rowOfsA[row] + base + m_dstBase;
                u8* gsrc = m_rowOfsC[row] + base + m_gatherBase;
                u8* ssrc = m_rowOfsB[row] + base + m_straightBase;
                if (m_useLut != 0) {
                    u8* lut = m_table->m_data;
                    i32 i = 0;
                    if (colBase > 0) {
                        do {
                            m_lineBuf[i] = ssrc[i];
                            i++;
                        } while (i < colBase);
                    }
                    for (i32 t = colBase; t < stride; t++) {
                        m_lineBuf[t] =
                            lut[static_cast<u32>(m_shadeRamp[t])
                                + static_cast<u32>(gsrc[m_warpTable[t]]) * 0x40];
                    }
                } else {
                    if (bpp == PIXEL8_BYTES_PER_PIXEL) {
                        i32 i = 0;
                        if (colBase > 0) {
                            do {
                                m_lineBuf[i] = ssrc[i];
                                i++;
                            } while (i < colBase);
                        }
                        for (i32 t = colBase; t < stride; t++) {
                            m_lineBuf[t] = gsrc[m_warpTable[t]];
                        }
                    } else if (bpp == PIXEL16_BYTES_PER_PIXEL) {
                        i32 i = 0;
                        i32 t = colBase;
                        if (colBase > 0) {
                            do {
                                i32 o = i * 2;
                                m_lineBuf[o] = ssrc[o];
                                m_lineBuf[o + 1] = ssrc[o + 1];
                                i++;
                            } while (i < colBase);
                        }
                        while (t < stride) {
                            i32 e = t + 1;
                            m_lineBuf[e * 2 - 2] = gsrc[m_warpTable[t] * 2];
                            m_lineBuf[e * 2 - 1] = gsrc[m_warpTable[t] * 2 + 1];
                            t = e;
                        }
                    } else if (bpp == PIXEL24_BYTES_PER_PIXEL) {
                        if (colBase > 0) {
                            i32 d = 0;
                            u8* sp = ssrc + 2;
                            i32 c = colBase;
                            do {
                                m_lineBuf[d] = sp[-2];
                                m_lineBuf[d + 1] = sp[-1];
                                m_lineBuf[d + 2] = *sp;
                                d += 3;
                                c--;
                                sp += 3;
                            } while (c != 0);
                        }
                        if (colBase < stride) {
                            i32 d = colBase * 3;
                            for (i32 t = colBase; t < stride; t++) {
                                m_lineBuf[d] = gsrc[m_warpTable[t] * 3];
                                m_lineBuf[d + 1] = gsrc[m_warpTable[t] * 3 + 1];
                                m_lineBuf[d + 2] = gsrc[m_warpTable[t] * 3 + 2];
                                d += 3;
                            }
                        }
                    }
                }
                u8* sp = m_lineBuf;
                i32 cnt = bpp * stride;
                u8* dp = dstLine;
                i32 n = cnt;
                if (cnt > 0) {
                    do {
                        *dp = *sp;
                        sp++;
                        n--;
                        dp++;
                    } while (n != 0);
                }
                if (m_stripCopy == 0) {
                    if (bpp * stripWidth > 0) {
                        memset(dstLine + cnt, 0, bpp * stripWidth);
                    }
                } else {
                    i32 c2 = bpp * stripWidth;
                    dstLine -= c2;
                    u8* s2 = (col - stripWidth) * bpp + m_rowOfsB[row] + m_straightBase;
                    if (c2 > 0) {
                        do {
                            *dstLine = *s2;
                            dstLine++;
                            s2++;
                            c2--;
                        } while (c2 != 0);
                    }
                }
                row++;
            } while (row < m_rowCount);
        }
    } else if (((m_mode == FADER_SWEEP_FORWARD && m_stripCopy == 0)
                || (m_mode == FADER_SWEEP_REVERSE && m_stripCopy != 0))
               && m_rowCount > 0) {
        i32 row = 0;
        i32 base = bpp * col;
        do {
            u8* dstLine = m_rowOfsA[row] + base + m_dstBase;
            u8* gsrc = m_rowOfsC[row] + base + m_gatherBase;
            u8* ssrc = m_rowOfsB[row] + base + m_straightBase;
            if (m_useLut != 0) {
                u8* lut = m_table->m_data;
                i32 i = 0;
                i32 e;
                if (colBase > 0) {
                    do {
                        e = i + 1;
                        m_lineBuf[i] =
                            lut[static_cast<u32>(m_shadeRamp[i])
                                + static_cast<u32>(gsrc[m_warpTable[i]]) * 0x40];
                        i = e;
                    } while (e < colBase);
                }
                for (i32 t = colBase; t < stride; t++) {
                    m_lineBuf[t] = ssrc[t];
                }
            } else {
                if (bpp == PIXEL8_BYTES_PER_PIXEL) {
                    i32 i = 0;
                    i32 e;
                    if (colBase > 0) {
                        do {
                            e = i + 1;
                            m_lineBuf[i] = gsrc[m_warpTable[i]];
                            i = e;
                        } while (e < colBase);
                    }
                    for (i32 t = colBase; t < stride; t++) {
                        m_lineBuf[t] = ssrc[t];
                    }
                } else if (bpp == PIXEL16_BYTES_PER_PIXEL) {
                    i32 i = 0;
                    if (colBase > 0) {
                        do {
                            i32 o = i * 4;
                            i++;
                            m_lineBuf[i * 2 - 2] = gsrc[m_warpTable[o / 4] * 2];
                            m_lineBuf[i * 2 - 1] = gsrc[m_warpTable[i - 1] * 2 + 1];
                        } while (i < colBase);
                    }
                    for (i32 t = colBase; t < stride; t++) {
                        i32 o = t * 2;
                        m_lineBuf[o] = ssrc[o];
                        m_lineBuf[o + 1] = ssrc[o + 1];
                    }
                } else if (bpp == PIXEL24_BYTES_PER_PIXEL) {
                    i32 k = 0;
                    if (colBase > 0) {
                        i32 d = 0;
                        do {
                            m_lineBuf[d] = gsrc[m_warpTable[k] * 3];
                            m_lineBuf[d + 1] = gsrc[m_warpTable[k] * 3 + 1];
                            m_lineBuf[d + 2] = gsrc[m_warpTable[k] * 3 + 2];
                            k++;
                            d += 3;
                        } while (k < colBase);
                    }
                    if (colBase < stride) {
                        i32 d = colBase * 3;
                        i32 c = stride - colBase;
                        u8* sp = ssrc + 2 + d;
                        do {
                            m_lineBuf[d] = sp[-2];
                            m_lineBuf[d + 1] = sp[-1];
                            m_lineBuf[d + 2] = *sp;
                            d += 3;
                            c--;
                            sp += 3;
                        } while (c != 0);
                    }
                }
            }
            u8* sp = m_lineBuf;
            i32 cnt = bpp * stride;
            u8* dp = dstLine;
            i32 n = cnt;
            if (cnt > 0) {
                do {
                    *dp = *sp;
                    sp++;
                    n--;
                    dp++;
                } while (n != 0);
            }
            if (m_stripCopy == 0) {
                if (bpp * stripWidth > 0) {
                    memset(dstLine - bpp * stripWidth, 0, bpp * stripWidth);
                }
            } else {
                i32 c2 = bpp * stripWidth;
                u8* s2 = (col + stride) * bpp + m_rowOfsB[row] + m_straightBase;
                dstLine += cnt;
                if (c2 > 0) {
                    do {
                        *dstLine = *s2;
                        dstLine++;
                        s2++;
                        c2--;
                    } while (c2 != 0);
                }
            }
            row++;
        } while (row < m_rowCount);
    }
}

// @early-stop
RVA(0x00182610, 0x2eb)

void CFaderShape::RenderTile(i32 col, i32 stripWidth) {
    if (stripWidth <= 0) {
        return;
    }
    i32 stride = m_halfWidth * 2;
    i32 rowBytes = stride + stripWidth;
    i32 bpp = m_surfA->m_bytesPerPixel;

    i32 x0;
    u8* src2base;
    u8* destBase;
    if (m_mode == FADER_SWEEP_FORWARD) {
        src2base = m_lineBuf;
        x0 = stripWidth;
        destBase = m_straightBase + (col - stripWidth) * bpp;
    } else {
        src2base = m_lineBuf + bpp * stride;
        x0 = 0;
        destBase = m_straightBase + (col + stride) * bpp;
    }
    if (m_mode != FADER_SWEEP_FORWARD && m_mode != FADER_SWEEP_REVERSE) {
        return;
    }

    u8* srcA = m_dstBase + (col - x0) * bpp;
    u8* srcB = m_gatherBase + (col - x0) * bpp;

    for (i32 j = 0; j < m_rowCount; j++) {
        u8* rowSrcA = srcA + m_rowOfsA[j];
        u8* rowSrcB = srcB + m_rowOfsC[j];

        if (m_useLut) {
            u8* lut = m_table->m_data;
            for (i32 k = 0; k < stride; k++) {
                u8 b = rowSrcB[m_warpTable[k]];
                m_lineBuf[x0 + k] = lut[(b << 6) + m_shadeRamp[k]];
            }
        } else if (bpp == PIXEL8_BYTES_PER_PIXEL) {
            for (i32 k = 0; k < stride; k++) {
                m_lineBuf[x0 + k] = rowSrcB[m_warpTable[k]];
            }
        } else if (bpp == PIXEL16_BYTES_PER_PIXEL) {
            for (i32 k = 0; k < stride; k++) {
                m_lineBuf[(x0 + k) * 2] = rowSrcB[m_warpTable[k] * 2];
                m_lineBuf[(x0 + k) * 2 + 1] = rowSrcB[m_warpTable[k] * 2 + 1];
            }
        } else if (bpp == PIXEL24_BYTES_PER_PIXEL) {
            for (i32 k = 0; k < stride; k++) {
                m_lineBuf[(x0 + k) * 3] = rowSrcB[m_warpTable[k] * 3];
                m_lineBuf[(x0 + k) * 3 + 1] = rowSrcB[m_warpTable[k] * 3 + 1];
                m_lineBuf[(x0 + k) * 3 + 2] = rowSrcB[m_warpTable[k] * 3 + 2];
            }
        }

        if (m_stripCopy) {
            i32 n = bpp * stripWidth;
            u8* s = destBase + m_rowOfsB[j];
            u8* d = src2base;
            while (n-- > 0) {
                *d++ = *s++;
            }
        } else {
            i32 n = bpp * stripWidth;
            u8* d = src2base;
            while (n-- > 0) {
                *d++ = 0;
            }
        }

        u8* s = m_lineBuf;
        u8* d = rowSrcA;
        i32 n = bpp * rowBytes;
        while (n-- > 0) {
            *d++ = *s++;
        }
    }
}

RVA(0x00182900, 0x35)
i32 CFaderShape::GetFrameCount() {
    GZ_ENUM_STORAGE(FaderMode, u32) mode = m_mode;
    if (mode == FADER_SWEEP_FORWARD || mode == FADER_SWEEP_REVERSE) {
        return m_span - m_halfWidth * 2;
    }
    if (mode == FADER_SPLIT_FROM_CENTER) {
        return (m_span - m_halfWidth * 4) / 2;
    }
    return 0;
}
