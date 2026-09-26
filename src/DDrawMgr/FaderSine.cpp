#include <rva.h>

#include <Mfc.h>

#include <Gruntz/FaderBufferInline.h>
#include <Gruntz/FaderSubtypes.h>

#include <math.h>

namespace {
#include <Gruntz/GameRand.h>
} // namespace

inline i32 CFaderSine::AccumulateSampleCount(i32 row, i32 delta, float step) {
    i32 count = 0;
    double wanted = delta * step;
    i32 whole = static_cast<i32>(wanted);
    if (whole < wanted) {
        m_fractionalCounts[row] += wanted - whole;
    }
    if (m_fractionalCounts[row] >= 1.0f) {
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

RVA(0x0017fdb0, 0x1a)
CFaderSine::CFaderSine() {
    m_width = 0;
    m_height = 0;
}

RVA_COMPGEN(0x0017fdd0, 0x1e, ??_GCFaderSine@@UAEPAXI@Z)
RVA(0x0017fdf0, 0xb)
CFaderSine::~CFaderSine() {}

RVA(0x0017fe00, 0x12d)
i32 CFaderSine::ApplyInit(CFaderConfig* desc) {
    CSineFaderConfig* cfg = static_cast<CSineFaderConfig*>(desc);
    m_previousFrame = 0;
    m_clearToBlack = cfg->m_clearToBlack;
    SelectTarget(m_targetSurface, cfg->m_targetSurface);
    SelectSource(m_restoreSurface, cfg->m_sourceSurface);
    if (!m_targetSurface) {
        return 0;
    }
    if (!m_restoreSurface) {
        m_clearToBlack = true;
    }
    m_width = m_targetSurface->GetWidth();
    i32 w = m_targetSurface->GetHeight();
    m_height = w;
    i32 p = cfg->m_intensityPercent;
    if (p < 0 || p > 100) {
        return 0;
    }
    m_intensityPercent = p;
    m_fadeRowCount = static_cast<i32>(w * (static_cast<float>(p) * 0.01f));
    for (i32 i = 0; i < 2000; i++) {
        m_appliedCounts[i] = 0;
        m_fractionalCounts[i] = 0;
        m_sampleOrder[i] = 0;
        m_sampleCursors[i] = GetRandomNumber(0, m_width - 1);
    }
    ScatterSamples(m_sampleOrder, 0, m_width, 1);
    return 1;
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
                            sin(static_cast<float>(static_cast<u32>(row + frame - m_height))
                                / m_fadeRowCount * 1.570795f)
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
                n = static_cast<i32>(step * 2.0f);
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
                n = static_cast<i32>(step * 2.0f);
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
                ClearBytes(m_targetBits + m_targetSurface->m_apiDesc.lPitch * done, bpp * m_width);
            } else {
                CopyBytes(
                    m_targetBits + m_targetSurface->m_apiDesc.lPitch * done,
                    m_restoreBits + m_restoreSurface->m_apiDesc.lPitch * done,
                    bpp * m_width
                );
            }
        }
        y++;
    }
    m_previousFrame = frame;
    if (m_targetSurface != NULL) {
        m_targetSurface->Unlock();
    }
    if (m_restoreSurface != NULL) {
        m_restoreSurface->Unlock();
    }
}

RVA(0x00180400, 0xa)
i32 CFaderSine::GetFrameCount() {
    return m_fadeRowCount + m_height;
}
