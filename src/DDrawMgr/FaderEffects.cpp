#include <rva.h>

#include <Mfc.h>

#include <DDrawMgr/ColorDepth.h>
#include <DDrawMgr/DDrawDeviceManager.h>
#include <DDrawMgr/DDSurface.h>
#include <DDrawMgr/DirectDrawMgr.h>
#include <DDrawMgr/PixelShift.h>
#include <DDrawMgr/ShadeTableCache.h>
#include <Gruntz/Fader.h>
#include <Gruntz/FaderConfig.h>
#include <Gruntz/FaderMode.h>
#include <Gruntz/FaderSubtypes.h>
#include <Gruntz/ShapeFaderConfig.h>
#include <Ints.h>
#include <Utils/RecordFill.h>
#include <Wap32/ScreenGeometry.h>

#include <ddraw.h>
#include <math.h>
#include <string.h>

DATA(0x001f07ec)
const float g_fxBias = -50.0f;
DATA(0x001f07f4)
const float g_fxEps = 1.0f;

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
const float g_faderPercentToUnit = 0.01f;

DATA(0x001f0860)
const float g_sineHalfPi = 1.570795f;
DATA(0x001f0864)
const float g_sineOne = 1.0f;
DATA(0x001f0888)
const double g_faderPowK = 2.0;

inline i32 CFaderSine::AccumulateSampleCount(i32 row, i32 delta, float step) {
    i32 count = 0;
    double wanted = delta * step;
    i32 whole = static_cast<i32>(wanted);
    if (whole < wanted) {
        m_fractionalCounts[row] += wanted - whole;
    }
    if (m_fractionalCounts[row] >= g_sineOne) {
        count = static_cast<i32>(m_fractionalCounts[row]);
        m_fractionalCounts[row] -= count;
    }
    count += whole;
    return count;
}

inline i32 CFaderSine::AdvanceSampleCursor(i32 row) {
    ++m_sampleCursors[row];
    if (m_sampleCursors[row] > m_width) {
        m_sampleCursors[row] = 0;
    }
    return m_sampleOrder[m_sampleCursors[row]];
}

RVA(0x0017f530, 0x19)
CFaderFlat::CFaderFlat() {
    m_rowStates = NULL;
}

RVA_COMPGEN(0x0017f550, 0x1e, ??_GCFaderFlat@@UAEPAXI@Z)

RVA(0x0017f570, 0x61)
CFaderFlat::~CFaderFlat() {
    if (m_rowStates) {
        delete[] m_rowStates;
        m_rowStates = NULL;
    }
}

RVA(0x0017f5e0, 0x7d)
i32 CFaderFlat::ApplyInit(CFaderConfig* desc) {
    CFlatFaderConfig* s = static_cast<CFlatFaderConfig*>(desc);
    if (s->m_targetSurface == NULL) {
        m_dstSurface = m_primarySurface;
    } else {
        m_dstSurface = s->m_targetSurface;
    }
    if (s->m_sourceSurface == NULL) {
        m_srcSurface = m_secondarySurface;
    } else {
        m_srcSurface = s->m_sourceSurface;
    }
    m_unusedOption = s->m_unusedOption;
    m_durationPercent = s->m_durationPercent;
    m_splitPercent = s->m_splitPercent;
    m_previousFrame = 0;
    m_rowStates = new i32[m_srcSurface->m_height];
    for (i32 i = 0; i < m_srcSurface->m_height; i++) {
        m_rowStates[i] = 0;
    }
    return 1;
}

// @early-stop
RVA(0x0017f660, 0x2e6)
void CFaderFlat::RenderFrame(i32 frame) {
    u16* srcBits = static_cast<u16*>(m_srcSurface->Lock(NULL));
    u16* dstBits = static_cast<u16*>(m_dstSurface->Lock(NULL));
    i32 h = m_srcSurface->m_height;
    i32 w = m_srcSurface->m_width;
    i32 base = h - frame - 1;
    i32 span = m_durationPercent * h / 100;
    if (span + base > h) {
        span = h - base;
    }
    i32 half = (m_splitPercent * w / 100) / 2 + w / 2;
    i32 rest = w - half;
    i32 end = span + base;
    i32 y = (base < 0) ? 0 : base;
    while (y < end) {
        double s = sin(static_cast<double>(y - base) / span * g_faderHalfPi);
        i32 n1 = static_cast<i32>(s * half);
        i32 n2 = static_cast<i32>(s * rest);
        memcpy(
            dstBits + m_dstSurface->m_pitch * y / 2,
            srcBits + m_srcSurface->m_pitch * y / 2 + half - n1,
            n1 * 2
        );
        memcpy(
            dstBits + m_dstSurface->m_pitch * y / 2 + w - n2,
            srcBits + m_srcSurface->m_pitch * y / 2 + half,
            n2 * 2
        );
        y++;
        memcpy(
            dstBits + m_dstSurface->m_pitch * y / 2,
            srcBits + m_srcSurface->m_pitch * y / 2 + rest - n2,
            n2 * 2
        );
        memcpy(
            dstBits + m_dstSurface->m_pitch * y / 2 + w - n1,
            srcBits + m_srcSurface->m_pitch * y / 2 + rest,
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
        memcpy(
            dstBits + m_dstSurface->m_pitch * y2 / 2,
            srcBits + m_srcSurface->m_pitch * y2 / 2,
            w * 2
        );
        y2++;
    }
    m_previousFrame = frame;

    m_srcSurface->m_ddSurface->Unlock(NULL);
    m_dstSurface->m_ddSurface->Unlock(NULL);
}

RVA(0x0017f950, 0x24)
i32 CFaderFlat::GetFrameCount() {
    i32 n = m_srcSurface->m_height;
    return n + (m_durationPercent * n) / 100;
}

// @identity-TODO: owner and no-op behavior are proven; the method identity is not.
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0017f980, 0x1)
void CFaderFlat::PrepareFrame() {}

// @identity-TODO: owner and no-op behavior are proven; the method identity is not.
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0017f990, 0x1)
void CFaderFlat::FinishFrame() {}

RVA(0x0017f9a0, 0x24)
CFaderRadial::CFaderRadial() {
    m_maxRadius = 0;
    m_unusedZero = 0;
    m_cells = NULL;
    m_unusedOne = 1;
}
RVA_COMPGEN(0x0017f9d0, 0x1e, ??_GCFaderRadial@@UAEPAXI@Z)

RVA(0x0017f9f0, 0x4f)
CFaderRadial::~CFaderRadial() {
    FreeBuffer();
}

RVA(0x0017fa40, 0x1f3)
i32 CFaderRadial::ApplyInit(CFaderConfig* desc) {
    CRadialFaderConfig* cfg = static_cast<CRadialFaderConfig*>(desc);
    if (cfg->m_targetSurface == NULL) {
        m_dstSurface = m_primarySurface;
    } else {
        m_dstSurface = cfg->m_targetSurface;
    }

    if (cfg->m_sourceSurface == NULL) {
        m_srcSurface = m_secondarySurface;
    } else {
        m_srcSurface = cfg->m_sourceSurface;
    }

    if (cfg->m_shadeTable == NULL) {

        CDDPalette* pal = cfg->m_palette;
        m_table = m_cache.HueRampTable(pal->m_entries, 0x10, 0);
        m_ownsTable = true;
    } else {
        m_table = cfg->m_shadeTable;
        m_ownsTable = false;
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
            cell.m_pixel = m_srcSurface->GetPixel(x, y);
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
    m_srcSurface->Lock(NULL);
    u8* base = static_cast<u8*>(m_dstSurface->Lock(NULL));
    if (m_table->m_data == NULL) {
        return;
    }

    for (i32 i = 0; i < m_srcSurface->m_width * m_srcSurface->m_height; i++) {
        float d = m_cells[i].m_radius - static_cast<float>(static_cast<u32>(frame));
        if (d > g_faderOne) {
            float sf = d / m_fadeDivisor - g_faderBiasFade;
            i32 px = m_centerX + static_cast<i32>((m_cells[i].m_vx / sf));
            i32 py = m_centerY - static_cast<i32>((m_cells[i].m_vy / sf));
            if (px > 0 && px < m_dstSurface->m_width && py > 0 && py < m_dstSurface->m_height) {
                base[py * m_dstSurface->m_pitch + px] = m_cells[i].m_pixel;
            }
        }
    }

    m_srcSurface->m_ddSurface->Unlock(NULL);
    m_dstSurface->m_ddSurface->Unlock(NULL);
    delete[] scratch;
}
RVA(0x0017fda0, 0x8)
i32 CFaderRadial::GetFrameCount() {
    return m_maxRadius;
}

RVA(0x0017fdb0, 0x1a)
CFaderSine::CFaderSine() {
    m_width = 0;
    m_height = 0;
}

RVA_COMPGEN(0x0017fdd0, 0x1e, ??_GCFaderSine@@UAEPAXI@Z)
RVA(0x0017fdf0, 0xb)
CFaderSine::~CFaderSine() {}

// @early-stop
RVA(0x0017fe00, 0x12d)
i32 CFaderSine::ApplyInit(CFaderConfig* desc) {
    CSineFaderConfig* cfg = static_cast<CSineFaderConfig*>(desc);
    i32 w;
    i32 p;
    i32 i;
    m_previousFrame = 0;
    m_clearToBlack = cfg->m_clearToBlack;
    if (cfg->m_targetSurface == NULL) {
        m_targetSurface = m_primarySurface;
    } else {
        m_targetSurface = cfg->m_targetSurface;
    }
    CDDSurface* dst = cfg->m_sourceSurface;
    if (dst == NULL) {
        dst = m_secondarySurface;
    }
    m_restoreSurface = dst;
    if (!m_targetSurface) {
        goto fail;
    }
    if (!m_restoreSurface) {
        m_clearToBlack = true;
    }
    m_width = m_targetSurface->m_width;
    w = m_targetSurface->m_height;
    m_height = w;
    p = cfg->m_intensityPercent;

    if (p < 0 || p > 100) {
        goto fail;
    }
    m_intensityPercent = p;
    m_fadeRowCount = static_cast<i32>(w * (static_cast<float>(p) * g_faderPercentToUnit));
    for (i = 0; i < 2000; i++) {
        m_appliedCounts[i] = 0;
        m_fractionalCounts[i] = 0;
        m_sampleOrder[i] = 0;
        m_sampleCursors[i] = GetRandom(0, m_width - 1);
    }
    ScatterSamples(m_sampleOrder, 0, m_width, 1);
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
    if (m_targetSurface != NULL) {
        m_targetBits = static_cast<u8*>(m_targetSurface->Lock(NULL));
    }
    if (m_restoreSurface != NULL) {
        m_restoreBits = static_cast<u8*>(m_restoreSurface->Lock(NULL));
    }
    i32 bpp = m_targetSurface->m_bytesPerPixel;
    float step = static_cast<float>(m_width) / m_fadeRowCount;
    i32 row = m_height - frame;
    while (row < m_height - frame + m_fadeRowCount) {
        if (row >= 0 && row < m_height) {
            u8* targetRow = m_targetBits + m_targetSurface->m_pitch * row;

            i32 delta = static_cast<i32>(
                            sin(static_cast<double>(static_cast<u32>(row + frame - m_height))
                                / m_fadeRowCount * g_sineHalfPi)
                            * m_width / step
                        )
                        - m_appliedCounts[row];
            if (m_clearToBlack != false) {

                i32 n = AccumulateSampleCount(row, delta, step);
                while (n > 0) {
                    i32 pick = AdvanceSampleCursor(row);
                    if (bpp > 0) {
                        memset(targetRow + pick * bpp, 0, bpp);
                    }
                    n--;
                }
                m_appliedCounts[row] += delta;
                n = static_cast<i32>(step + step);
                while (n > 0) {
                    i32 pick = GetRandom(0, m_width - 1);
                    if (bpp > 0) {
                        memset(targetRow + pick * bpp, 0, bpp);
                    }
                    n--;
                }
            } else {
                u8* restoreRow = m_restoreBits + m_restoreSurface->m_pitch * row;
                i32 n = AccumulateSampleCount(row, delta, step);
                while (n > 0) {
                    i32 pick = AdvanceSampleCursor(row);
                    for (i32 j = 0; j < bpp; j++) {
                        targetRow[pick * bpp + j] = restoreRow[pick * bpp + j];
                    }
                    n--;
                }
                m_appliedCounts[row] += delta;
                n = static_cast<i32>(step + step);
                while (n > 0) {
                    i32 pick = GetRandom(0, m_width - 1);
                    for (i32 j = 0; j < bpp; j++) {
                        targetRow[pick * bpp + j] = restoreRow[pick * bpp + j];
                    }
                    n--;
                }
            }
        }
        row++;
    }

    i32 y = m_previousFrame;
    while (y < frame) {
        i32 done = m_fadeRowCount - y + m_height - 1;
        if (done >= 0 && done < m_height) {
            if (m_clearToBlack != false) {
                u8* clrRow = m_targetBits + m_targetSurface->m_pitch * done;
                i32 span = bpp * m_width;
                while (span-- > 0) {
                    *clrRow++ = 0;
                }
            } else {
                u8* restore = m_restoreBits + m_restoreSurface->m_pitch * done;
                u8* target = m_targetBits + m_targetSurface->m_pitch * done;
                i32 span = bpp * m_width;
                while (span-- > 0) {
                    *target++ = *restore++;
                }
            }
        }
        y++;
    }
    m_previousFrame = frame;
    if (m_targetSurface != NULL) {
        m_targetSurface->m_ddSurface->Unlock(NULL);
    }
    if (m_restoreSurface != NULL) {
        m_restoreSurface->m_ddSurface->Unlock(NULL);
    }
}

RVA(0x00180400, 0xa)
i32 CFaderSine::GetFrameCount() {
    return m_fadeRowCount + m_height;
}

RVA(0x00180410, 0x19)
CFaderLight::CFaderLight() {
    m_overlay = NULL;
}

RVA_COMPGEN(0x00180430, 0x1e, ??_GCFaderLight@@UAEPAXI@Z)
RVA(0x00180450, 0x4f)
CFaderLight::~CFaderLight() {
    ReleaseBuffers();
}

// @early-stop
RVA(0x001804a0, 0x182)
i32 CFaderLight::ApplyInit(CFaderConfig* desc) {
    CLightFaderConfig* d = static_cast<CLightFaderConfig*>(desc);
    m_previousFrame = 0;
    if (d->m_targetSurface == NULL) {
        m_targetSurface = m_primarySurface;
    } else {
        m_targetSurface = d->m_targetSurface;
    }
    if (d->m_sourceSurface == NULL) {
        m_restoreSurface = m_secondarySurface;
    } else {
        m_restoreSurface = d->m_sourceSurface;
    }
    m_clearMode = d->m_clearMode;
    m_centerX = d->m_centerX;
    m_centerY = d->m_centerY;
    CDDPalette* pal = d->m_palette;
    m_palette = pal;
    i32 cnt = d->m_spanCount;
    m_spanCount = cnt;
    if (cnt > 0 && d->m_shadeTable == NULL && m_palette == NULL) {
        return 0;
    }
    if (m_targetSurface == NULL) {
        return 0;
    }
    if (m_restoreSurface == NULL && m_clearMode == false) {
        return 0;
    }
    RECT rect;
    m_width = m_targetSurface->m_width;
    rect.right = m_width;
    m_height = m_targetSurface->m_height;
    rect.bottom = m_height;
    rect.left = 0;
    rect.top = 0;
    POINT pt;
    pt.x = m_centerX;
    pt.y = m_centerY;
    if (PtInRect(&rect, pt) == false) {
        return 0;
    }
    if (m_clearMode != false) {
        i32 i = 0;
        if (m_height > 0) {
            do {
                m_spanStarts[i] = 0;
                m_spanEnds[i] = m_width;
                i++;
            } while (i < m_height);
        }
    } else {
        i32 i = 0;
        if (m_height > 0) {
            do {
                m_spanStarts[i] = m_centerX;
                m_spanEnds[i] = m_centerX;
                i++;
            } while (i < m_height);
        }
    }
    if (m_spanCount > 0) {
        if (d->m_shadeTable == NULL) {
            m_table = m_cache.HueRampTable(m_palette->m_entries, m_spanCount, 0);
            m_ownsTable = true;
            return 1;
        }
        m_table = d->m_shadeTable;
    }
    return 1;
}

RVA(0x00180630, 0x1)
void CFaderLight::ReleaseBuffers() {}

#define FADER_MAX0(v) ((v) < 0 ? 0 : (v))
#define FADER_CLAMPW(v, w) (FADER_MAX0(v) < (w) ? FADER_MAX0(v) : (w))

inline void CFaderLight::ComputeSpan(i32 row, i32 radiusSq, i32 edgeOffset, i32& right, i32& left) {
    i32 dy = row - m_centerY;
    i32 dx = -static_cast<i32>(sqrt(static_cast<double>(radiusSq - dy * dy)));
    right = FADER_CLAMPW(m_centerX - dx, m_width);
    i32 x = dx + m_centerX + edgeOffset;
    left = (x < 0) ? 0 : x;
    if (left >= m_width) {
        left = m_width;
    }
}

// @early-stop
RVA(0x00180640, 0x96c)
void CFaderLight::RenderFrame(i32 frame) {
    i32 delta = frame - m_previousFrame;
    if (m_targetSurface != NULL) {
        m_targetBits = static_cast<u8*>(m_targetSurface->Lock(NULL));
    }
    if (m_restoreSurface != NULL) {
        m_restoreBits = static_cast<u8*>(m_restoreSurface->Lock(NULL));
    }
    i32 bpp = m_targetSurface->m_bytesPerPixel;
    u8* lut = NULL;
    if (m_table != NULL) {
        lut = m_table->m_data;
    }
    if (m_clearMode != false) {
        u8* ovlBits;
        if (m_overlay != NULL) {
            ovlBits = static_cast<u8*>(m_overlay->Lock(NULL));
        }
        i32 r = m_frameCount - frame;
        i32 rr = r * r;
        i32 v = m_centerY - r - delta;
        i32 row = (v < 0) ? 0 : v;
        for (;;) {
            i32 stop = delta + r + m_centerY;
            if (stop >= m_height) {
                stop = m_height;
            }
            if (row >= stop) {
                break;
            }
            if (row >= m_centerY - r + 1 && row <= r + m_centerY - 1) {
                i32 right;
                i32 left;
                ComputeSpan(row, rr, 1, right, left);
                i32 oldStart = m_spanStarts[row];
                u8* clrL = m_targetBits + m_targetSurface->m_pitch * row + oldStart * bpp;
                i32 n1 = (left - oldStart) * bpp;
                if (n1 > 0) {
                    memset(clrL, 0, n1);
                }
                i32 oldEnd = m_spanEnds[row];
                u8* clrR = m_targetBits + m_targetSurface->m_pitch * row + right * bpp;
                i32 n2 = (oldEnd - right) * bpp;
                if (n2 > 0) {
                    memset(clrR, 0, n2);
                }
                u8* bits = m_targetBits;
                Render(row, rr, r, lut, bits, ovlBits);
                m_spanStarts[row] = left;
                m_spanEnds[row] = right;
            } else {
                u8* clrRow = m_targetBits + m_targetSurface->m_pitch * row;
                i32 w = m_width;
                if (w > 0) {
                    memset(clrRow, 0, w);
                }
            }
            row++;
        }
        if (m_overlay != NULL) {
            m_overlay->m_ddSurface->Unlock(NULL);
        }
    } else {
        i32 fr2 = frame * frame;
        i32 v = m_centerY - frame - delta - m_spanCount;
        i32 row = (v < 0) ? 0 : v;
        for (;;) {
            i32 stop = delta + frame + m_spanCount + m_centerY;
            if (stop >= m_height) {
                stop = m_height;
            }
            if (row >= stop) {
                break;
            }
            if (row > m_centerY - frame && row < frame + m_centerY) {
                i32 right;
                i32 left;
                ComputeSpan(row, fr2, -1, right, left);
                i32 n1 = (m_spanStarts[row] - left) * bpp;
                u8* src = m_restoreBits + m_restoreSurface->m_pitch * row + left * bpp;
                u8* dst = m_targetBits + m_targetSurface->m_pitch * row + left * bpp;
                while (n1-- > 0) {
                    *dst++ = *src++;
                }
                i32 oldEnd = m_spanEnds[row];
                i32 n2 = (right - oldEnd) * bpp;
                src = m_restoreBits + m_restoreSurface->m_pitch * row + oldEnd * bpp;
                dst = m_targetBits + m_targetSurface->m_pitch * row + oldEnd * bpp;
                while (n2-- > 0) {
                    *dst++ = *src++;
                }
                m_spanStarts[row] = left;
                m_spanEnds[row] = right;
            }
            if (row > m_centerY - frame - m_spanCount && row < frame + m_spanCount + m_centerY) {
                i32 rad = frame + m_spanCount - 1;
                Render(row, rad * rad, rad, lut, m_targetBits, m_restoreBits);
            }
            row++;
        }
    }
    m_previousFrame = frame;
    if (m_targetSurface != NULL) {
        m_targetSurface->m_ddSurface->Unlock(NULL);
    }
    if (m_restoreSurface != NULL) {
        m_restoreSurface->m_ddSurface->Unlock(NULL);
    }
}

// @early-stop
RVA(0x00180fb0, 0x534)

inline void
CFaderLight::Render(i32 row0, i32 radiusSq, i32 radius, u8* lut, u8* srcBits, u8* dstBits) {
    if (m_spanCount <= 0) {
        return;
    }
    i32 cx = m_centerY;
    i32 dx = row0 - cx;
    i32 dx2 = dx * dx;
    i32 row = m_centerX - static_cast<i32>(sqrt(static_cast<double>((radiusSq - dx2)))) + 1;
    i32 len =
        static_cast<i32>(sqrt(static_cast<double>(((row - m_centerX) * (row - m_centerX) + dx2))));

    i32 srcCol = row0 * m_targetSurface->m_pitch;
    u8* rowLsrc = srcBits + row + srcCol;
    i32 dstCol = row0 * m_restoreSurface->m_pitch;
    u8* rowLdst = dstBits + row + dstCol;
    u8* rowRsrc = srcBits - row;
    rowRsrc += srcCol;
    rowRsrc += 2 * m_centerX;
    u8* rowRdst = dstBits - row;
    rowRdst += dstCol;
    rowRdst += 2 * m_centerX;

    i32 mid = m_height / 2;
    i32 mirSrc;
    i32 mirDst;
    if (cx >= mid && row0 <= cx) {
        i32 mirCol = 2 * (cx - row0);
        if (mirCol + row0 < m_height) {

            mirSrc = mirCol * m_targetSurface->m_pitch;
            mirDst = mirCol * m_restoreSurface->m_pitch;
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
                if (2 * m_centerX - row < m_width) {
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
            if (2 * m_centerX - row < m_width) {
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
        mirSrc = mirCol * m_targetSurface->m_pitch;
        mirDst = mirCol * m_restoreSurface->m_pitch;
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
            if (2 * m_centerX - row < m_width) {
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
            if (2 * m_centerX - row < m_width) {
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
    double pLeft = pow(static_cast<double>(m_centerX), g_faderPowK);
    double pTop = pow(static_cast<double>(m_centerY), g_faderPowK);
    double dTopLeft = sqrt(pLeft + pTop);
    double pBottom = pow(static_cast<double>(m_targetSurface->m_height - m_centerY), g_faderPowK);
    double pRight = pow(static_cast<double>(m_targetSurface->m_width - m_centerX), g_faderPowK);
    double dBottomRight = sqrt(pRight + pBottom);
    double dTopRight = sqrt(pRight + pTop);
    double dBottomLeft = sqrt(pLeft + pBottom);

    i32 r = static_cast<i32>(max(max(max(dTopLeft, dBottomRight), dTopRight), dBottomLeft));
    m_frameCount = r;
    return r;
}

RVA(0x00181660, 0x40)
void CFaderLight::BeginFade() {
    if (m_spanCount > 0 && m_clearMode != false) {
        CDDSurface* h =
            m_deviceManager->CreateOffscreenSurface(m_width, m_height, BPP_UNSET, 0, -1);
        m_overlay = h;
        h->Blt(m_targetSurface);
    }
}

RVA(0x001816a0, 0x1c)
void CFaderLight::EndFade() {
    if (m_overlay) {
        m_deviceManager->RemoveSurface(m_overlay);
        m_overlay = NULL;
    }
}

RVA(0x001816c0, 0x32)
CFaderShape::CFaderShape() {
    m_warpTable = NULL;
    m_targetRowOffsets = NULL;
    m_sourceRowOffsets = NULL;
    m_warpRowOffsets = NULL;
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
    if (m_targetRowOffsets) {
        delete[] m_targetRowOffsets;
    }
    if (m_sourceRowOffsets) {
        delete[] m_sourceRowOffsets;
    }
    if (m_warpRowOffsets) {
        delete[] m_warpRowOffsets;
    }
    if (m_lineBuf) {
        delete[] m_lineBuf;
    }
    if (m_shadeRamp) {
        delete[] m_shadeRamp;
    }
}

// @early-stop
RVA(0x001817e0, 0x315)
i32 CFaderShape::ApplyInit(CFaderConfig* desc) {
    CShapeFaderConfig* pInit = static_cast<CShapeFaderConfig*>(desc);
    i32 i;
    i32 mx;
    m_previousFrame = 0;
    if (pInit == NULL) {
        return 0;
    }

    if (pInit->m_targetSurface == NULL) {
        m_targetSurface = m_primarySurface;
    } else {
        m_targetSurface = pInit->m_targetSurface;
    }
    if (pInit->m_sourceSurface == NULL) {
        m_sourceSurface = m_secondarySurface;
    } else {
        m_sourceSurface = pInit->m_sourceSurface;
    }
    if (m_targetSurface == NULL) {
        return 0;
    }
    if (m_sourceSurface == NULL) {
        return 0;
    }
    if (pInit->m_warpSourceSurface == NULL) {
        m_warpSourceSurface = m_sourceSurface;
    } else {
        m_warpSourceSurface = pInit->m_warpSourceSurface;
    }

    if (!m_cache.Init()) {
        return 0;
    }

    m_targetHeight = m_targetSurface->m_height;
    m_targetWidth = m_targetSurface->m_width;
    m_sourceHeight = m_sourceSurface->m_height;
    m_sourceWidth = m_sourceSurface->m_width;
    m_warpHeight = m_warpSourceSurface->m_height;
    m_warpWidth = m_warpSourceSurface->m_width;
    if (m_targetHeight != m_sourceHeight) {
        return 0;
    }
    if (m_targetWidth != m_sourceWidth) {
        return 0;
    }
    if (m_targetHeight != m_warpHeight) {
        return 0;
    }
    if (m_targetWidth != m_warpWidth) {
        return 0;
    }
    if (m_warpHeight != m_sourceHeight) {
        return 0;
    }
    if (m_warpWidth != m_sourceWidth) {
        return 0;
    }

    if (pInit->m_mode <= FADER_INVALID || pInit->m_mode >= FADER_COUNT) {
        return 0;
    }
    m_mode = pInit->m_mode;
    m_stripCopy = pInit->m_stripCopy;
    m_halfWidth = pInit->m_halfWidth;

    if (m_mode == FADER_SWEEP_FORWARD || m_mode == FADER_SWEEP_REVERSE) {
        if (m_targetWidth < static_cast<i32>((static_cast<double>(m_halfWidth) * 3.14159))) {
            return 0;
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
    if (m_targetSurface->m_bitDepth != BPP_PALETTED_8) {
        m_useLut = false;
    }

    if (m_useLut != false) {
        if (pInit->m_shadeTable) {
            m_ownsTable = false;
            m_table = pInit->m_shadeTable;
        } else if (_access(pInit->m_shadeTablePath, 0) == 0) {
            m_table = m_cache.AddFromArray(pInit->m_shadeTablePath);
            if (m_table == NULL) {
                m_useLut = false;
            }
        } else {
            CDDPalette* pal = pInit->m_palette;
            m_table = m_cache.FlashTable(
                pal->m_entries,
                FLASH_SHADE_DARK_RAMP_STEPS,
                FLASH_SHADE_BRIGHT_RAMP_STEPS,
                FLASH_SHADE_START_PERCENT,
                FLASH_SHADE_END_PERCENT
            );
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

    m_targetRowOffsets = new i32[m_targetHeight];
    m_sourceRowOffsets = new i32[m_sourceHeight];
    m_warpRowOffsets = new i32[m_warpHeight];
    for (i = 0; i < m_targetHeight; i++) {
        m_targetRowOffsets[i] = m_targetSurface->m_pitch * i;
        m_sourceRowOffsets[i] = m_sourceSurface->m_pitch * i;
        m_warpRowOffsets[i] = m_warpSourceSurface->m_pitch * i;
    }

    mx = m_targetWidth;
    if (m_targetHeight > m_targetWidth) {
        mx = m_targetHeight;
    }
    m_lineBuf = new u8[m_targetSurface->m_bytesPerPixel * mx];
    return 1;
}

// @early-stop
RVA(0x00181b00, 0x34f)
void CFaderShape::RenderFrame(i32 frame) {
    m_dstBase = static_cast<u8*>(m_targetSurface->Lock(NULL));
    u8* gather = static_cast<u8*>(m_sourceSurface->Lock(NULL));
    m_straightBase = gather;
    if (m_sourceSurface != m_warpSourceSurface) {
        gather = static_cast<u8*>(m_warpSourceSurface->Lock(NULL));
    }
    m_gatherBase = gather;

    i32 stride = m_halfWidth * 2;
    i32 arc = static_cast<i32>(static_cast<double>(m_halfWidth) * 3.14159);
    u32 seam = 0;
    if (m_mode == FADER_SPLIT_FROM_CENTER && m_stripCopy != false) {
        seam = m_targetWidth / 2;
    }
    if (m_stripCopy == false && frame == 0) {
        i32 targetPitch = m_targetSurface->m_pitch;
        i32 sourcePitch = m_sourceSurface->m_pitch;
        i32 n = (targetPitch < sourcePitch) ? targetPitch : sourcePitch;
        i32 row = 0;
        while (row < m_targetHeight) {
            u8* src = m_straightBase + m_sourceRowOffsets[row];
            u8* dst = m_dstBase + m_targetRowOffsets[row];
            i32 x = 0;
            while (x < n) {
                *dst++ = *src++;
                x++;
            }
            row++;
        }
    }
    if (m_stripCopy != false) {
        if (seam + frame <= static_cast<u32>(m_targetWidth - arc - m_halfWidth)) {
            switch (m_mode) {
                case FADER_SWEEP_FORWARD:
                    RenderTile(frame, frame - m_previousFrame);
                    break;
                case FADER_SWEEP_REVERSE:
                    RenderTile(m_targetWidth - frame - stride, frame - m_previousFrame);
                    break;
                case FADER_SPLIT_FROM_CENTER:
                    m_mode = FADER_SWEEP_FORWARD;
                    RenderTile(m_targetWidth / 2 + frame, frame - m_previousFrame);
                    m_mode = FADER_SWEEP_REVERSE;
                    RenderTile(m_targetWidth / 2 - frame - stride, frame - m_previousFrame);
                    m_mode = FADER_SPLIT_FROM_CENTER;
                    break;
            }
        } else {
            switch (m_mode) {
                case FADER_SWEEP_FORWARD:
                    RenderWarpTile(frame, frame - m_previousFrame);
                    break;
                case FADER_SWEEP_REVERSE:
                    RenderWarpTile(m_targetWidth - frame - stride, frame - m_previousFrame);
                    break;
                case FADER_SPLIT_FROM_CENTER:
                    m_mode = FADER_SWEEP_FORWARD;
                    RenderWarpTile(m_targetWidth / 2 + frame, frame - m_previousFrame);
                    m_mode = FADER_SWEEP_REVERSE;
                    RenderWarpTile(
                        m_targetWidth - m_targetWidth / 2 - frame - stride,
                        frame - m_previousFrame
                    );
                    m_mode = FADER_SPLIT_FROM_CENTER;
                    break;
            }
        }
    }
    if (m_stripCopy == false) {
        if (seam + frame > static_cast<u32>(arc - m_halfWidth)) {
            switch (m_mode) {
                case FADER_SWEEP_FORWARD:
                    RenderTile(frame, frame - m_previousFrame);
                    break;
                case FADER_SWEEP_REVERSE:
                    RenderTile(m_targetWidth - frame - stride, frame - m_previousFrame);
                    break;
                case FADER_SPLIT_FROM_CENTER:
                    m_mode = FADER_SWEEP_FORWARD;
                    RenderTile(frame, frame - m_previousFrame);
                    m_mode = FADER_SWEEP_REVERSE;
                    RenderTile(m_targetWidth - frame - stride, frame - m_previousFrame);
                    m_mode = FADER_SPLIT_FROM_CENTER;
                    break;
            }
        } else {
            switch (m_mode) {
                case FADER_SWEEP_FORWARD:
                    RenderWarpTile(frame, frame - m_previousFrame);
                    break;
                case FADER_SWEEP_REVERSE:
                    RenderWarpTile(m_targetWidth - frame - stride, frame - m_previousFrame);
                    break;
                case FADER_SPLIT_FROM_CENTER:
                    m_mode = FADER_SWEEP_FORWARD;
                    RenderWarpTile(frame, frame - m_previousFrame);
                    m_mode = FADER_SWEEP_REVERSE;
                    RenderWarpTile(m_targetWidth - frame - stride, frame - m_previousFrame);
                    m_mode = FADER_SPLIT_FROM_CENTER;
                    break;
            }
        }
    }
    m_previousFrame = frame;
    m_targetSurface->m_ddSurface->Unlock(NULL);
    m_sourceSurface->m_ddSurface->Unlock(NULL);
    if (m_sourceSurface != m_warpSourceSurface) {
        m_warpSourceSurface->m_ddSurface->Unlock(NULL);
    }
}

RVA(0x00181e50, 0x7b9)

void CFaderShape::RenderWarpTile(i32 col, i32 stripWidth) {
    i32 stride = m_halfWidth * 2;
    if (stripWidth <= 0) {
        return;
    }
    i32 arc = static_cast<i32>((static_cast<double>(m_halfWidth) * 3.14159));
    i32 bpp = m_targetSurface->m_bytesPerPixel;

    i32 colBase;
    if ((m_mode == FADER_SWEEP_FORWARD && m_stripCopy != false)
        || (m_mode == FADER_SWEEP_REVERSE && m_stripCopy == false)) {
        u32 arcSpan = arc - m_halfWidth;
        i32 tail = m_targetWidth - col - stride;
        colBase = stride - static_cast<i32>(static_cast<double>(stride) / arcSpan * tail);
    } else {
        colBase = col;
    }
    if ((m_mode == FADER_SWEEP_FORWARD && m_stripCopy == false)
        || (m_mode == FADER_SWEEP_REVERSE && m_stripCopy != false)) {
        u32 arcSpan = arc - m_halfWidth;
        colBase = static_cast<i32>(static_cast<double>(stride) / arcSpan * col);
    }

    if ((m_mode == FADER_SWEEP_FORWARD && m_stripCopy != false)
        || (m_mode == FADER_SWEEP_REVERSE && m_stripCopy == false)) {
        i32 row = 0;
        if (m_targetHeight > 0) {
            i32 base = bpp * col;
            do {
                u8* dstLine = m_targetRowOffsets[row] + base + m_dstBase;
                u8* gsrc = m_warpRowOffsets[row] + base + m_gatherBase;
                u8* ssrc = m_sourceRowOffsets[row] + base + m_straightBase;
                if (m_useLut != false) {
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
                        if (colBase > 0) {
                            do {
                                m_lineBuf[i * 2] = ssrc[i * 2];
                                m_lineBuf[i * 2 + 1] = ssrc[i * 2 + 1];
                                i++;
                            } while (i < colBase);
                        }
                        for (i32 t = colBase; t < stride; t++) {
                            m_lineBuf[t * 2] = gsrc[m_warpTable[t] * 2];
                            m_lineBuf[t * 2 + 1] = gsrc[m_warpTable[t] * 2 + 1];
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
                while (n-- > 0) {
                    *dp++ = *sp++;
                }
                if (m_stripCopy != false) {
                    i32 c2 = bpp * stripWidth;
                    dstLine -= c2;
                    u8* s2 = (col - stripWidth) * bpp + m_sourceRowOffsets[row] + m_straightBase;
                    while (c2-- > 0) {
                        *dstLine++ = *s2++;
                    }
                } else {
                    if (bpp * stripWidth > 0) {
                        memset(dstLine + cnt, 0, bpp * stripWidth);
                    }
                }
                row++;
            } while (row < m_targetHeight);
        }
    } else if (((m_mode == FADER_SWEEP_FORWARD && m_stripCopy == false)
                || (m_mode == FADER_SWEEP_REVERSE && m_stripCopy != false))
               && m_targetHeight > 0) {
        i32 row = 0;
        i32 base = bpp * col;
        do {
            u8* dstLine = m_targetRowOffsets[row] + base + m_dstBase;
            u8* gsrc = m_warpRowOffsets[row] + base + m_gatherBase;
            u8* ssrc = m_sourceRowOffsets[row] + base + m_straightBase;
            if (m_useLut != false) {
                u8* lut = m_table->m_data;
                i32 i = 0;
                if (colBase > 0) {
                    do {
                        m_lineBuf[i] =
                            lut[static_cast<u32>(m_shadeRamp[i])
                                + static_cast<u32>(gsrc[m_warpTable[i]]) * 0x40];
                        i++;
                    } while (i < colBase);
                }
                for (i32 t = colBase; t < stride; t++) {
                    m_lineBuf[t] = ssrc[t];
                }
            } else {
                if (bpp == PIXEL8_BYTES_PER_PIXEL) {
                    i32 i = 0;
                    if (colBase > 0) {
                        do {
                            m_lineBuf[i] = gsrc[m_warpTable[i]];
                            i++;
                        } while (i < colBase);
                    }
                    for (i32 t = colBase; t < stride; t++) {
                        m_lineBuf[t] = ssrc[t];
                    }
                } else if (bpp == PIXEL16_BYTES_PER_PIXEL) {
                    i32 i = 0;
                    if (colBase > 0) {
                        do {
                            m_lineBuf[i * 2] = gsrc[m_warpTable[i] * 2];
                            m_lineBuf[i * 2 + 1] = gsrc[m_warpTable[i] * 2 + 1];
                            i++;
                        } while (i < colBase);
                    }
                    for (i32 t = colBase; t < stride; t++) {
                        m_lineBuf[t * 2] = ssrc[t * 2];
                        m_lineBuf[t * 2 + 1] = ssrc[t * 2 + 1];
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
            while (n-- > 0) {
                *dp++ = *sp++;
            }
            if (m_stripCopy != false) {
                i32 c2 = bpp * stripWidth;
                u8* s2 = (col + stride) * bpp + m_sourceRowOffsets[row] + m_straightBase;
                dstLine += cnt;
                while (c2-- > 0) {
                    *dstLine++ = *s2++;
                }
            } else {
                if (bpp * stripWidth > 0) {
                    memset(dstLine - bpp * stripWidth, 0, bpp * stripWidth);
                }
            }
            row++;
        } while (row < m_targetHeight);
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
    i32 bpp = m_targetSurface->m_bytesPerPixel;

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

    u8* targetColumnBase = m_dstBase + (col - x0) * bpp;
    u8* warpColumnBase = m_gatherBase + (col - x0) * bpp;

    for (i32 j = 0; j < m_targetHeight; j++) {
        u8* targetRow = targetColumnBase + m_targetRowOffsets[j];
        u8* warpRow = warpColumnBase + m_warpRowOffsets[j];

        if (m_useLut) {
            u8* lut = m_table->m_data;
            for (i32 k = 0; k < stride; k++) {
                u8 b = warpRow[m_warpTable[k]];
                m_lineBuf[x0 + k] = lut[(b << 6) + m_shadeRamp[k]];
            }
        } else if (bpp == PIXEL8_BYTES_PER_PIXEL) {
            for (i32 k = 0; k < stride; k++) {
                m_lineBuf[x0 + k] = warpRow[m_warpTable[k]];
            }
        } else if (bpp == PIXEL16_BYTES_PER_PIXEL) {
            for (i32 k = 0; k < stride; k++) {
                m_lineBuf[(x0 + k) * 2] = warpRow[m_warpTable[k] * 2];
                m_lineBuf[(x0 + k) * 2 + 1] = warpRow[m_warpTable[k] * 2 + 1];
            }
        } else if (bpp == PIXEL24_BYTES_PER_PIXEL) {
            for (i32 k = 0; k < stride; k++) {
                m_lineBuf[(x0 + k) * 3] = warpRow[m_warpTable[k] * 3];
                m_lineBuf[(x0 + k) * 3 + 1] = warpRow[m_warpTable[k] * 3 + 1];
                m_lineBuf[(x0 + k) * 3 + 2] = warpRow[m_warpTable[k] * 3 + 2];
            }
        }

        if (m_stripCopy) {
            i32 n = bpp * stripWidth;
            u8* s = destBase + m_sourceRowOffsets[j];
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
        u8* d = targetRow;
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
        return m_targetWidth - m_halfWidth * 2;
    }
    if (mode == FADER_SPLIT_FROM_CENTER) {
        return (m_targetWidth - m_halfWidth * 4) / 2;
    }
    return 0;
}
