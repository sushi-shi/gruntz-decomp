#include <rva.h>

#include <Mfc.h>

#include <Gruntz/FaderBufferInline.h>
#include <Gruntz/FaderSineInline.h>

#include <math.h>

namespace {
#include <Gruntz/GameRand.h>
} // namespace

DATA(0x001f085c)
const float g_faderPercentToUnit = 0.01f;
DATA(0x001f0860)
const float g_sineHalfPi = 1.570795f;
DATA(0x001f0864)
const float g_sineOne = 1.0f;

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
    m_width = m_targetSurface->m_apiDesc.dwWidth;
    w = m_targetSurface->m_apiDesc.dwHeight;
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
        m_sampleCursors[i] = GetRandomNumber(0, m_width - 1);
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
            u8* targetRow = m_targetBits + m_targetSurface->m_apiDesc.lPitch * row;

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
                    ClearSample(targetRow, pick, bpp);
                    n--;
                }
                m_appliedCounts[row] += delta;
                n = static_cast<i32>(step + step);
                while (n > 0) {
                    i32 pick = GetRandomNumber(0, m_width - 1);
                    ClearSample(targetRow, pick, bpp);
                    n--;
                }
            } else {
                u8* restoreRow = m_restoreBits + m_restoreSurface->m_apiDesc.lPitch * row;
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
                    i32 pick = GetRandomNumber(0, m_width - 1);
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
                u8* clrRow = m_targetBits + m_targetSurface->m_apiDesc.lPitch * done;
                ClearBytes(clrRow, bpp * m_width);
            } else {
                u8* restore = m_restoreBits + m_restoreSurface->m_apiDesc.lPitch * done;
                u8* target = m_targetBits + m_targetSurface->m_apiDesc.lPitch * done;
                i32 span = bpp * m_width;
                CopyBytes(target, restore, span);
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
