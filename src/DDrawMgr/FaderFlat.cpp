#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/FaderSubtypes.h>

#include <math.h>

CFaderFlat::CFaderFlat() {
    m_rowStates = NULL;
}

CFaderFlat::~CFaderFlat() {
    if (m_rowStates) {
        delete[] m_rowStates;
        m_rowStates = NULL;
    }
}

i32 CFaderFlat::ApplyInit(CFaderConfig* desc) {
    CFlatFaderConfig* s = static_cast<CFlatFaderConfig*>(desc);
    SelectTarget(m_dstSurface, s->m_targetSurface);
    SelectSource(m_srcSurface, s->m_sourceSurface);
    m_unusedOption = s->m_unusedOption;
    m_durationPercent = s->m_durationPercent;
    m_splitPercent = s->m_splitPercent;
    m_previousFrame = 0;
    m_rowStates = new i32[m_srcSurface->GetHeight()];
    for (i32 i = 0; i < m_srcSurface->GetHeight(); i++) {
        m_rowStates[i] = 0;
    }
    return 1;
}

void CFaderFlat::RenderFrame(i32 frame) {
    u16* srcBits = static_cast<u16*>(m_srcSurface->Lock(NULL));
    u16* dstBits = static_cast<u16*>(m_dstSurface->Lock(NULL));
    i32 h = m_srcSurface->GetHeight();
    i32 w = m_srcSurface->GetWidth();
    i32 base = h - frame - 1;
    i32 span = m_durationPercent * h / 100;
    if (span + base > h) {
        span = h - base;
    }
    i32 half = (m_splitPercent * w / 100) / 2 + w / 2;
    i32 rest = w - half;
    i32 end = span + base;
    i32 y = max(0, base);
    while (y < end) {
        double s = sin(static_cast<float>(y - base) / span * 1.570795f);
        i32 n1 = static_cast<i32>(s * half);
        i32 n2 = static_cast<i32>(s * rest);
        memcpy(
            dstBits + m_dstSurface->m_apiDesc.lPitch * y / 2,
            srcBits + m_srcSurface->m_apiDesc.lPitch * y / 2 + half - n1,
            n1 * 2
        );
        memcpy(
            dstBits + m_dstSurface->m_apiDesc.lPitch * y / 2 + w - n2,
            srcBits + m_srcSurface->m_apiDesc.lPitch * y / 2 + half,
            n2 * 2
        );
        y++;
        memcpy(
            dstBits + m_dstSurface->m_apiDesc.lPitch * y / 2,
            srcBits + m_srcSurface->m_apiDesc.lPitch * y / 2 + rest - n2,
            n2 * 2
        );
        memcpy(
            dstBits + m_dstSurface->m_apiDesc.lPitch * y / 2 + w - n1,
            srcBits + m_srcSurface->m_apiDesc.lPitch * y / 2 + rest,
            n1 * 2
        );
        y++;
    }
    i32 lastRow = h - 1;
    i32 y0 = lastRow;
    y0 = min(y0, end);
    i32 y2 = y0;
    for (;;) {
        i32 stop = y0 + frame - m_previousFrame;
        stop = min(lastRow, stop);
        if (y2 >= stop) {
            break;
        }
        memcpy(
            dstBits + m_dstSurface->m_apiDesc.lPitch * y2 / 2,
            srcBits + m_srcSurface->m_apiDesc.lPitch * y2 / 2,
            w * 2
        );
        y2++;
    }
    m_previousFrame = frame;

    m_srcSurface->Unlock();
    m_dstSurface->Unlock();
}

i32 CFaderFlat::GetFrameCount() {
    i32 n = m_srcSurface->GetHeight();
    return n + (m_durationPercent * n) / 100;
}

void CFaderFlat::PrepareFrame() {}

void CFaderFlat::FinishFrame() {}
