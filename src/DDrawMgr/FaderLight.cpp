#include <rva.h>

#include <Mfc.h>

#include <DDrawMgr/DirectDrawMgr.h>
#include <Gruntz/Fader.h>
#include <Gruntz/FaderBufferInline.h>
#include <Gruntz/FaderLightInline.h>

RVA(0x00180410, 0x19)
CFaderLight::CFaderLight() {
    m_overlay = NULL;
}

RVA_COMPGEN(0x00180430, 0x1e, ??_GCFaderLight@@UAEPAXI@Z)
RVA(0x00180450, 0x4f)
CFaderLight::~CFaderLight() {
    ReleaseBuffers();
}

RVA(0x001804a0, 0x182)
i32 CFaderLight::ApplyInit(CFaderConfig* desc) {
    CLightFaderConfig* d = static_cast<CLightFaderConfig*>(desc);
    m_previousFrame = 0;
    SelectTarget(m_targetSurface, d->m_targetSurface);
    SelectSource(m_restoreSurface, d->m_sourceSurface);
    m_clearMode = d->m_clearMode;
    m_center = d->m_center;
    m_palette = d->m_palette;
    m_spanCount = d->m_spanCount;
    if (m_spanCount > 0 && d->m_shadeTable == NULL && m_palette == NULL) {
        return 0;
    }
    if (m_targetSurface == NULL) {
        return 0;
    }
    if (m_restoreSurface == NULL && m_clearMode == false) {
        return 0;
    }
    m_width = m_targetSurface->GetWidth();
    m_height = m_targetSurface->GetHeight();
    CRect rect(0, 0, m_width, m_height);
    if (!rect.PtInRect(m_center)) {
        return 0;
    }
    if (m_clearMode != false) {
        for (i32 i = 0; i < m_height; i++) {
            m_spanStarts[i] = 0;
            m_spanEnds[i] = m_width;
        }
    } else {
        for (i32 i = 0; i < m_height; i++) {
            m_spanStarts[i] = m_center.x;
            m_spanEnds[i] = m_center.x;
        }
    }
    if (m_spanCount > 0) {
        if (d->m_shadeTable == NULL) {
            PALETTEENTRY* entries = m_palette->m_entries;
            m_table = m_cache.HueRampTable(entries, m_spanCount, 0);
            m_ownsTable = true;
        } else {
            m_table = d->m_shadeTable;
        }
    }
    return 1;
}

RVA(0x00180630, 0x1)
void CFaderLight::ReleaseBuffers() {}

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
        i32 rr = SQR(r);
        i32 v = m_center.y - r - delta;
        i32 row = (v < 0) ? 0 : v;
        for (;;) {
            i32 stop = delta + r + m_center.y;
            if (stop >= m_height) {
                stop = m_height;
            }
            if (row >= stop) {
                break;
            }
            if (row >= m_center.y - r + 1 && row <= r + m_center.y - 1) {
                i32 right;
                i32 left;
                ComputeSpan(row, rr, 1, right, left);
                ClearBytes(
                    m_targetBits + m_targetSurface->m_apiDesc.lPitch * row
                        + m_spanStarts[row] * bpp,
                    (left - m_spanStarts[row]) * bpp
                );
                ClearBytes(
                    m_targetBits + m_targetSurface->m_apiDesc.lPitch * row + right * bpp,
                    (m_spanEnds[row] - right) * bpp
                );
                Render(row, rr, r, lut, m_targetBits, ovlBits);
                m_spanStarts[row] = left;
                m_spanEnds[row] = right;
            } else {
                ClearBytes(m_targetBits + m_targetSurface->m_apiDesc.lPitch * row, m_width);
            }
            row++;
        }
        if (m_overlay != NULL) {
            m_overlay->Unlock();
        }
    } else {
        i32 fr2 = SQR(frame);
        i32 v = m_center.y - frame - delta - m_spanCount;
        i32 row = (v < 0) ? 0 : v;
        for (;;) {
            i32 stop = delta + frame + m_spanCount + m_center.y;
            if (stop >= m_height) {
                stop = m_height;
            }
            if (row >= stop) {
                break;
            }
            if (row > m_center.y - frame && row < frame + m_center.y) {
                i32 right;
                i32 left;
                ComputeSpan(row, fr2, -1, right, left);
                CopyBytes(
                    m_targetBits + m_targetSurface->m_apiDesc.lPitch * row + left * bpp,
                    m_restoreBits + m_restoreSurface->m_apiDesc.lPitch * row + left * bpp,
                    (m_spanStarts[row] - left) * bpp
                );
                CopyBytes(
                    m_targetBits + m_targetSurface->m_apiDesc.lPitch * row + m_spanEnds[row] * bpp,
                    m_restoreBits + m_restoreSurface->m_apiDesc.lPitch * row
                        + m_spanEnds[row] * bpp,
                    (right - m_spanEnds[row]) * bpp
                );
                m_spanStarts[row] = left;
                m_spanEnds[row] = right;
            }
            if (row > m_center.y - frame - m_spanCount && row < frame + m_spanCount + m_center.y) {
                i32 rad = frame + m_spanCount - 1;
                Render(row, SQR(rad), rad, lut, m_targetBits, m_restoreBits);
            }
            row++;
        }
    }
    m_previousFrame = frame;
    if (m_targetSurface != NULL) {
        m_targetSurface->Unlock();
    }
    if (m_restoreSurface != NULL) {
        m_restoreSurface->Unlock();
    }
}

// @early-stop

RVA(0x001814f0, 0x16d)
i32 CFaderLight::GetFrameCount() {
    double pLeft = pow(static_cast<double>(m_center.x), 2.0);
    double pTop = pow(static_cast<double>(m_center.y), 2.0);
    double dTopLeft = sqrt(pLeft + pTop);
    double pBottom =
        pow(static_cast<double>(static_cast<i32>(m_targetSurface->m_apiDesc.dwHeight) - m_center.y),
            2.0);
    double pRight =
        pow(static_cast<double>(static_cast<i32>(m_targetSurface->m_apiDesc.dwWidth) - m_center.x),
            2.0);
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
