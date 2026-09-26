#include <StdAfx.h>

#include <rva.h>

#include <Gruntz/FaderSubtypes.h>

#include <math.h>

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
    SelectTarget(m_dstSurface, s->m_targetSurface);
    SelectSource(m_srcSurface, s->m_sourceSurface);
    m_unusedOption = s->m_unusedOption;
    m_durationPercent = s->m_durationPercent;
    m_splitPercent = s->m_splitPercent;
    m_previousFrame = 0;
    m_rowStates = new i32[m_srcSurface->m_apiDesc.dwHeight];
    for (i32 i = 0; i < static_cast<i32>(m_srcSurface->m_apiDesc.dwHeight); i++) {
        m_rowStates[i] = 0;
    }
    return 1;
}

// @early-stop
RVA(0x0017f660, 0x2e6)
void CFaderFlat::RenderFrame(i32 frame) {
    u16* srcBits = static_cast<u16*>(m_srcSurface->Lock(NULL));
    u16* dstBits = static_cast<u16*>(m_dstSurface->Lock(NULL));
    CSize surfaceSize(m_srcSurface->m_apiDesc.dwWidth, m_srcSurface->m_apiDesc.dwHeight);
    i32 base = surfaceSize.cy - frame - 1;
    i32 span = m_durationPercent * surfaceSize.cy / 100;
    if (span + base > surfaceSize.cy) {
        span = surfaceSize.cy - base;
    }
    i32 half = (m_splitPercent * surfaceSize.cx / 100) / 2 + surfaceSize.cx / 2;
    i32 rest = surfaceSize.cx - half;
    i32 end = span + base;
    i32 y = (base < 0) ? 0 : base;
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
            dstBits + m_dstSurface->m_apiDesc.lPitch * y / 2 + surfaceSize.cx - n2,
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
            dstBits + m_dstSurface->m_apiDesc.lPitch * y / 2 + surfaceSize.cx - n1,
            srcBits + m_srcSurface->m_apiDesc.lPitch * y / 2 + rest,
            n1 * 2
        );
        y++;
    }
    i32 lastRow = surfaceSize.cy - 1;
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
            dstBits + m_dstSurface->m_apiDesc.lPitch * y2 / 2,
            srcBits + m_srcSurface->m_apiDesc.lPitch * y2 / 2,
            surfaceSize.cx * 2
        );
        y2++;
    }
    m_previousFrame = frame;

    m_srcSurface->Unlock();
    m_dstSurface->Unlock();
}

RVA(0x0017f950, 0x24)
i32 CFaderFlat::GetFrameCount() {
    i32 n = m_srcSurface->m_apiDesc.dwHeight;
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
