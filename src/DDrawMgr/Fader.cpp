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

DATA(0x002c279c)
u8 g_fxRandSeeded;
DATA(0x002c27a8)
i32 g_fxRandSeed;
DATA(0x001f085c)
const float g_faderScale_5f085c = 0.01f;
DATA(0x001f0888)
const double g_faderPowK = 2.0;
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

RVA(0x0017e450, 0x23)
CFader::CFader() {
    m_table = NULL;
    m_flag = 1;
}

RVA_COMPGEN(0x0017e480, 0x1e, ??_GCFader@@UAEPAXI@Z)
RVA(0x0017e4a0, 0x69)
CFader::~CFader() {
    if (m_table && m_flag) {
        m_cache.FindRemove(m_table);
        m_table = NULL;
    }
}

RVA(0x0017e510, 0x23)
void CFader::Wait(i32 delay) {
    DWORD target = GetTickCount() + delay;
    while (GetTickCount() < target) {
    }
}

RVA(0x0017e760, 0x11)
void CFader::SetTimers(CDDSurface* a, CDDSurface* b) {
    m_timerA = a;
    m_timerB = b;
}

RVA(0x0017e780, 0xa)
void CFader::Set2c(CDDrawPtrCollections* pool) {
    m_ptrColl = pool;
}

RVA(0x0017e790, 0x1)
void CFader::BeginFade() {}

RVA(0x0017e7a0, 0x1)
void CFader::EndFade() {}

RVA(0x0017e7b0, 0x9)
CFxModeDesc::CFxModeDesc() {
    m_type = FXMODE_UNTAGGED;
}

RVA(0x0017e7c0, 0x7a)
CFxModeT1::CFxModeT1() {
    m_type = FXMODE_SHAPE;
    m_targetSurface = NULL;
    m_sourceSurface = NULL;
    m_warpSourceSurface = NULL;
    m_halfWidth = 0x32;
    m_mode = FADER_SWEEP_FORWARD;
    m_stripCopy = 1;
    m_useLut = 0;
    m_shadeTable = NULL;
    m_shadeTablePath = g_emptyString;
    m_palette = NULL;
}

// @early-stop
RVA(0x0017e840, 0x37)
CFxModeT2::CFxModeT2() {
    m_type = FXMODE_LIGHT;
    m_targetSurface = NULL;
    m_sourceSurface = NULL;
    m_clearMode = 1;
    m_spanCount = 0;
    m_centerX = SCREEN_HALF_W_PX;
    m_centerY = SCREEN_HALF_H_PX;
    m_shadeTable = NULL;
}

RVA(0x0017e880, 0x28)
CFxModeT3::CFxModeT3() {
    m_type = FXMODE_SINE;
    m_targetSurface = NULL;
    m_sourceSurface = NULL;
    m_clearToBlack = 1;
    m_intensityPercent = 0xf;
}

RVA(0x0017e8b0, 0x27)
CFxModeT4::CFxModeT4() {
    m_type = FXMODE_RADIAL;
    m_targetSurface = NULL;
    m_sourceSurface = NULL;
    m_palette = NULL;
    m_shadeTable = NULL;
    m_param0c = 1;
}

RVA(0x0017e8e0, 0x27)
CFxModeT5::CFxModeT5() {
    m_type = FXMODE_FLAT;
    m_targetSurface = NULL;
    m_sourceSurface = NULL;
    m_param0c = 0;
    m_splitPercent = 0;
    m_durationPercent = 0x19;
}

RVA(0x0017e910, 0x29)
CFxModeT6::CFxModeT6() {
    m_type = FXMODE_MESH;
    m_targetSurface = NULL;
    m_sourceSurface = NULL;
    m_flipTarget = NULL;
    m_reverseOrder = 0;
    m_param18 = 0;
    m_cols = 0;
    m_rows = 0;
}

RVA(0x0017e940, 0x27)
CFaderMesh::CFaderMesh() {}

RVA_COMPGEN(0x0017e970, 0x1e, ??_GCFaderMesh@@UAEPAXI@Z)
RVA(0x0017e990, 0x6b)
CFaderMesh::~CFaderMesh() {}

// @early-stop
RVA(0x0017ef00, 0x21c)
void CFaderMesh::RenderFrame(i32 frame) {
    CDDSurface* dst = m_dstSurface;
    if (m_primeSrc != NULL) {
        dst->Blt(m_primeSrc);
    } else {
        dst->Clear(0);
    }
    if (m_meshBuf.m_nSize > 0) {
        float ff = static_cast<float>(frame);
        RezElem40* recs = m_meshBuf.m_pData;
        for (i32 i = 0; i < m_meshBuf.m_nSize; i++) {
            RezElem40* rec = &recs[i];
            i32 r0 = rec->m_startRect.left, r1 = rec->m_startRect.top;
            i32 r2 = rec->m_startRect.right, r3 = rec->m_startRect.bottom;
            i32 r4 = rec->m_endRect.left, r5 = rec->m_endRect.top;
            i32 r6 = rec->m_endRect.right, r7 = rec->m_endRect.bottom;
            float t = ff / static_cast<float>(GetFrameCount());

            i32 x0 = r0 + static_cast<i32>((static_cast<float>((r4 - r0)) * t));
            i32 y0 = r1 + static_cast<i32>((static_cast<float>((r5 - r1)) * t));
            i32 x1 = r2 + static_cast<i32>((static_cast<float>((r6 - r2)) * t));
            i32 y1 = r3 + static_cast<i32>((static_cast<float>((r7 - r3)) * t));

            i32 bx0 = r4, by0 = r5, bx1 = r6, by1 = r7;
            if (x0 < 0 && x1 > 0) {
                bx0 = r4 - x0;
                x0 = 0;
            } else if (x1 <= dst->m_width && x0 > dst->m_width) {
                bx1 = r6 - x1 + dst->m_width;
                x1 = dst->m_width - 1;
            }
            if (y0 < 0 && y1 > 0) {
                by0 = r5 - y0;
                y0 = 0;
            } else if (y1 <= dst->m_height && y0 > dst->m_height) {
                by1 = r7 - y1 + dst->m_height;
                y1 = dst->m_height - 1;
            }

            RECT dstRect = {x0, y0, x1, y1};
            RECT srcRect;
            if (m_recOrderFlag != 0) {
                srcRect.left = r0;
                srcRect.top = r1;
                srcRect.right = r2;
                srcRect.bottom = r3;
            } else {
                srcRect.left = bx0;
                srcRect.top = by0;
                srcRect.right = bx1;
                srcRect.bottom = by1;
            }
            dst->BltEx(&dstRect, m_bltSrc, &srcRect, 0x1000000, 0);
        }
    }
    m_flipTarget->Flip(0);
}

RVA(0x0017f120, 0x6)
i32 CFaderMesh::GetFrameCount() {
    return 0x1f4;
}

RVA_COMPGEN(0x0017f310, 0x1e, ??_GCRezBufferObject@@UAEPAXI@Z)
RVA_COMPGEN(0x0017f330, 0x51, ??1CRezBufferObject@@UAE@XZ)
RVA(0x0017f500, 0x23)
void __stdcall ZeroRecords(void* dst, int count) {
    memset(dst, 0, count * 0x28);
}

RVA_COMPGEN(0x0017f550, 0x1e, ??_GCFaderFlat@@UAEPAXI@Z)
RVA_COMPGEN(0x0017f9d0, 0x1e, ??_GCFaderRadial@@UAEPAXI@Z)
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

static __inline i32 FxRand(i32 range) {
    u32 x;
    if (!(g_fxRandSeeded & 1)) {
        g_fxRandSeeded |= 1;
        x = timeGetTime();
    } else {
        x = g_fxRandSeed;
    }
    g_fxRandSeed = x * 214013 + 2531011;
    return ((static_cast<i32>(g_fxRandSeed) >> 16) & 0x7fff) % range;
}

// @early-stop
RVA(0x0017fe00, 0x12d)
i32 CFaderSine::ApplyInit(CFxModeDesc* desc) {
    CFxModeT3* cfg = static_cast<CFxModeT3*>(desc);
    i32 w;
    i32 p;
    i32 i;
    m_previousFrame = 0;
    m_boxParam = cfg->m_clearToBlack;
    CDDSurface* src = cfg->m_targetSurface;
    if (!src) {
        src = m_timerA;
    }
    m_srcBox = src;
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
    m_scaledMag = static_cast<i32>((static_cast<float>(p) * g_faderScale_5f085c * w));
    for (i = 0; i < 2000; i++) {
        m_arr0[i] = 0;
        m_arr2[i] = 0;
        m_arr3[i] = 0;
        m_arr1[i] = FxRand(m_elemCount);
    }
    ScatterSamples(m_arr3, 0, m_elemCount, 1);
    return 1;
fail:
    return 0;
}

RVA(0x0017f530, 0x19)
CFaderFlat::CFaderFlat() {
    m_frames = NULL;
}

RVA(0x0017f570, 0x61)
CFaderFlat::~CFaderFlat() {
    if (m_frames) {
        delete[] m_frames;
        m_frames = NULL;
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
RVA(0x001804a0, 0x182)
i32 CFaderLight::ApplyInit(CFxModeDesc* desc) {
    CFxModeT2* d = static_cast<CFxModeT2*>(desc);
    m_previousFrame = 0;
    CDDSurface* s = d->m_targetSurface;
    if (s == NULL) {
        s = m_timerA;
    }
    m_surface = s;
    CDDSurface* b = d->m_sourceSurface;
    if (b == NULL) {
        m_dstSurface = m_timerB;
    } else {
        m_dstSurface = b;
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
                i32 R = m_spanCount;
                u8* bits = m_srcBits;
                if (R > 0) {
                    i32 cy = m_centerY;
                    i32 dy = row - cy;
                    i32 dy2 = dy * dy;
                    i32 cx = m_centerX;
                    i32 xcur = cx - static_cast<i32>(sqrt(static_cast<double>((rr - dy2)))) + 1;
                    i32 dist = static_cast<i32>(
                        sqrt(static_cast<double>(((xcur - cx) * (xcur - cx) + dy2)))
                    );
                    i32 srcpitch = m_surface->m_pitch;
                    i32 srcCol = row * srcpitch;
                    u8* rowLsrc = bits + xcur + srcCol;
                    i32 dstpitch = m_dstSurface->m_pitch;
                    i32 dstCol = row * dstpitch;
                    u8* rowLdst = ovlBits + xcur + dstCol;
                    u8* rowRsrc = (bits - xcur) + srcCol + 2 * cx;
                    u8* rowRdst = (ovlBits - xcur) + dstCol + 2 * cx;
                    i32 mid = m_surfHeight / 2;
                    i32 mirSrc;
                    i32 mirDst;
                    if (cy >= mid && row <= cy) {
                        i32 mirRow = 2 * (cy - row);
                        if (mirRow + row < m_surfHeight) {

                            mirSrc = mirRow * srcpitch;
                            mirDst = mirRow * dstpitch;
                            if (dist >= r - R) {
                                do {
                                    if (xcur > cx) {
                                        break;
                                    }
                                    i32 cl = dist - r + R;
                                    if (xcur >= 0) {
                                        i32 p = *rowLdst;
                                        *rowLsrc = *(lut + p * R + cl);
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
                                    dist = static_cast<i32>(
                                        sqrt(static_cast<double>(((xcur - cx) * (xcur - cx) + dy2)))
                                    );
                                } while (dist >= r - m_spanCount);
                            }
                        } else if (dist >= r - R) {

                            do {
                                if (xcur > cx) {
                                    break;
                                }
                                i32 cl = dist - r + R;
                                if (xcur >= 0) {
                                    i32 p = *rowLdst;
                                    *rowLsrc = *(lut + p * R + cl);
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
                                dist = static_cast<i32>(
                                    sqrt(static_cast<double>(((xcur - cx) * (xcur - cx) + dy2)))
                                );
                            } while (dist >= r - m_spanCount);
                        }
                    } else if (cy < mid && row >= cy) {
                        i32 mirRow = 2 * dy;
                        if (row - mirRow < 0) {

                            if (dist >= r - R) {
                                do {
                                    if (xcur > cx) {
                                        break;
                                    }
                                    i32 cl = dist - r + R;
                                    if (xcur >= 0) {
                                        i32 p = *rowLdst;
                                        *rowLsrc = *(lut + p * R + cl);
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
                                    dist = static_cast<i32>(
                                        sqrt(static_cast<double>(((xcur - cx) * (xcur - cx) + dy2)))
                                    );
                                } while (dist >= r - m_spanCount);
                            }
                        } else {

                            mirSrc = mirRow * srcpitch;
                            mirDst = mirRow * dstpitch;
                            if (dist >= r - R) {
                                do {
                                    if (xcur > cx) {
                                        break;
                                    }
                                    i32 cl = dist - r + R;
                                    if (xcur >= 0) {
                                        i32 p = *rowLdst;
                                        *rowLsrc = *(lut + p * R + cl);
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
                                    dist = static_cast<i32>(
                                        sqrt(static_cast<double>(((xcur - cx) * (xcur - cx) + dy2)))
                                    );
                                } while (dist >= r - m_spanCount);
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
    i32 R = m_spanCount;
    if (R <= 0) {
        return;
    }
    i32 cx = m_centerY;
    i32 dx = row0 - cx;
    i32 dx2 = dx * dx;
    i32 cy = m_centerX;
    i32 row = cy - static_cast<i32>(sqrt(static_cast<double>((radiusSq - dx2)))) + 1;
    i32 len = static_cast<i32>(sqrt(static_cast<double>(((row - cy) * (row - cy) + dx2))));

    i32 srcpitch = m_surface->m_pitch;
    i32 srcCol = row0 * srcpitch;
    u8* rowLsrc = srcBits + row + srcCol;
    i32 dstpitch = m_dstSurface->m_pitch;
    i32 dstCol = row0 * dstpitch;
    u8* rowLdst = dstBits + row + dstCol;
    u8* rowRsrc = (srcBits - row) + srcCol + 2 * cy;
    u8* rowRdst = (dstBits - row) + dstCol + 2 * cy;

    i32 mid = m_surfHeight / 2;
    i32 mirSrc;
    i32 mirDst;
    if (cx >= mid && row0 <= cx) {
        i32 mirCol = 2 * (cx - row0);
        if (mirCol + row0 < m_surfHeight) {

            mirSrc = mirCol * srcpitch;
            mirDst = mirCol * dstpitch;
            if (len < radius - R) {
                return;
            }
            do {
                if (row > cy) {
                    return;
                }
                i32 cl = len - radius + R;
                if (row >= 0) {
                    i32 p = *rowLdst;
                    *rowLsrc = *(lut + p * R + cl);
                    i32 q = *(rowLdst + mirDst);
                    *(rowLsrc + mirSrc) = *(lut + q * m_spanCount + cl);
                }
                rowLsrc++;
                rowLdst++;
                if (2 * cy - row < m_surfWidth) {
                    i32 p = *rowRdst;
                    *rowRsrc = *(lut + p * m_spanCount + cl);
                    i32 q = *(rowRdst + mirDst);
                    *(rowRsrc + mirSrc) = *(lut + q * m_spanCount + cl);
                }
                rowRsrc--;
                rowRdst--;
                row++;
                len = static_cast<i32>(sqrt(static_cast<double>(((row - cy) * (row - cy) + dx2))));
            } while (len >= radius - m_spanCount);
            return;
        }

        if (len < radius - R) {
            return;
        }
        do {
            if (row > cy) {
                return;
            }
            i32 cl = len - radius + R;
            if (row >= 0) {
                i32 p = *rowLdst;
                *rowLsrc = *(lut + p * R + cl);
            }
            rowLsrc++;
            rowLdst++;
            if (2 * cy - row < m_surfWidth) {
                i32 p = *rowRdst;
                *rowRsrc = *(lut + p * m_spanCount + cl);
            }
            rowRsrc--;
            rowRdst--;
            row++;
            len = static_cast<i32>(sqrt(static_cast<double>(((row - cy) * (row - cy) + dx2))));
        } while (len >= radius - m_spanCount);
        return;
    }

    if (cx >= mid) {
        if (row0 >= mid) {
            return;
        }
    }
    {
        i32 mirCol = 2 * dx;
        i32 right = len - mirCol;
        if (right < 0) {

            if (len < radius - R) {
                return;
            }
            do {
                if (row > cy) {
                    return;
                }
                i32 cl = len - radius + R;
                if (row >= 0) {
                    i32 p = *rowLdst;
                    *rowLsrc = *(lut + p * R + cl);
                }
                rowLsrc++;
                rowLdst++;
                if (2 * cy - row < m_surfWidth) {
                    i32 p = *rowRdst;
                    *rowRsrc = *(lut + p * m_spanCount + cl);
                }
                rowRsrc--;
                rowRdst--;
                row++;
                len = static_cast<i32>(sqrt(static_cast<double>(((row - cy) * (row - cy) + dx2))));
            } while (len >= radius - m_spanCount);
            return;
        }

        mirSrc = mirCol * srcpitch;
        mirDst = mirCol * dstpitch;
        if (len < radius - R) {
            return;
        }
        do {
            if (row > cy) {
                return;
            }
            i32 cl = len - radius + R;
            if (row >= 0) {
                i32 p = *rowLdst;
                *rowLsrc = *(lut + p * R + cl);
                i32 q = *(rowLdst - mirDst);
                *(rowLsrc - mirSrc) = *(lut + q * m_spanCount + cl);
            }
            rowLsrc++;
            rowLdst++;
            if (2 * cy - row < m_surfWidth) {
                i32 p = *rowRdst;
                *rowRsrc = *(lut + p * m_spanCount + cl);
                i32 q = *(rowRdst - mirDst);
                *(rowRsrc - mirSrc) = *(lut + q * m_spanCount + cl);
            }
            rowRsrc--;
            rowRdst--;
            row++;
            len = static_cast<i32>(sqrt(static_cast<double>(((row - cy) * (row - cy) + dx2))));
        } while (len >= radius - m_spanCount);
    }
}

RVA(0x001814f0, 0x16d)
i32 CFaderLight::GetFrameCount() {
    i32 cx = m_centerX;
    i32 cy = m_centerY;
    i32 w = m_surface->m_width;
    i32 h = m_surface->m_height;

    double pA = pow(static_cast<double>(cx), g_faderPowK);
    double pB = pow(static_cast<double>(cy), g_faderPowK);
    double pH = pow(static_cast<double>((h - cy)), g_faderPowK);
    double pW = pow(static_cast<double>((w - cx)), g_faderPowK);

    double d0 = sqrt(pA + pB);
    double d1 = sqrt(pW + pB);
    double d2 = sqrt(pA + pH);
    double d3 = sqrt(pW + pH);

    double m = d0;
    if (d1 > m) {
        m = d1;
    }
    if (d2 > m) {
        m = d2;
    }
    if (d3 > m) {
        m = d3;
    }
    i32 r = static_cast<i32>(m);
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

RVA(0x0017f9a0, 0x24)
CFaderRadial::CFaderRadial() {
    m_maxRadius = 0;
    m_reserved40 = 0;
    m_cells = NULL;
    m_reserved48 = 1;
}

RVA(0x0017f9f0, 0x4f)
CFaderRadial::~CFaderRadial() {
    FreeBuffer();
}

RVA(0x0017fc40, 0x11)
void CFaderRadial::FreeBuffer() {
    if (m_cells) {
        delete[] m_cells;
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

// @early-stop
RVA(0x0017e540, 0xd8)
void CFader::RunFadeStepped(i32 step, i32 lead, i32 vsync) {
    i32 count = GetFrameCount();
    if (count < 1) {
        return;
    }
    BeginFade();
    RenderFrame(0);
    Wait(lead);
    DWORD startTick = GetTickCount();
    i32 loops = 0;
    i32 frame = 1;
    if (count >= 1) {
        do {
            if (vsync && m_ptrColl) {
                m_ptrColl->m_device->WaitForVerticalBlank(DDWAITVB_BLOCKBEGIN, 0);
            }
            RenderFrame(frame);
            loops++;
            frame += step;
        } while (frame <= count);
    }
    if (frame != count) {
        RenderFrame(count);
        loops++;
    }
    float fLoops = static_cast<float>(loops);
    DWORD elapsed = GetTickCount() - startTick;
    m_measuredFps = static_cast<i32>((fLoops / (static_cast<float>(elapsed) * 0.001f)));
    EndFade();
}

// @early-stop
RVA(0x0017e620, 0x13b)
void CFader::RunFade(u32 dur, i32 lead, i32 vsync) {
    i32 prev = 0;
    i32 frame = 0;
    i32 count = GetFrameCount();
    if (count < 1) {
        return;
    }
    BeginFade();
    RenderFrame(0);
    Wait(lead);
    i32 loops = 0;
    DWORD startTick = GetTickCount();
    float fStart = static_cast<float>(startTick);
    float fDur = static_cast<float>(dur);
    float fCount = static_cast<float>(count);
    if (count >= 0) {
        do {
            frame =
                static_cast<i32>(((static_cast<float>(GetTickCount()) - fStart) / fDur * fCount));
            if (prev != frame && frame <= count && frame > 0) {
                if (vsync && m_ptrColl) {
                    m_ptrColl->m_device->WaitForVerticalBlank(DDWAITVB_BLOCKBEGIN, 0);
                }
                RenderFrame(frame);
                loops++;
            }
            prev = frame;
        } while (frame <= count);
    }
    if (frame != count) {
        RenderFrame(count);
        loops++;
    }
    float fLoops = static_cast<float>(loops);
    DWORD elapsed = GetTickCount() - startTick;
    m_measuredFps = static_cast<i32>((fLoops / (static_cast<float>(elapsed) * 0.001f)));
    EndFade();
}

// @early-stop
RVA(0x0017ea00, 0x4fc)
i32 CFaderMesh::ApplyInit(CFxModeDesc* descOpaque) {

    CFxModeT6* cfg = static_cast<CFxModeT6*>(descOpaque);
    CRezBufferObject* mesh = &m_meshBuf;

    m_dstSurface = cfg->m_targetSurface ? cfg->m_targetSurface : m_timerA;
    m_bltSrc = cfg->m_sourceSurface ? cfg->m_sourceSurface : m_timerB;
    if (cfg->m_flipTarget == NULL) {
        return 0;
    }
    m_primeSrc = cfg->m_primeSource;
    m_flipTarget = cfg->m_flipTarget;
    m_desc18 = cfg->m_param18;
    m_recOrderFlag = cfg->m_reverseOrder;
    m_cols = cfg->m_cols;
    m_rows = cfg->m_rows;

    mesh->SetSize(0, -1);

    i32 halfW = m_dstSurface->m_height / 2;
    i32 halfH = m_dstSurface->m_width / 2;
    i32 dx = m_bltSrc->m_width / m_cols;
    i32 dy = m_bltSrc->m_height / m_rows;
    float radius = static_cast<float>(sqrt(static_cast<double>((dx * dx + dy * dy))));
    if (m_rows <= 0) {
        return 1;
    }

    for (i32 row = 0; row < m_rows; row++) {
        i32 cellW2 = halfW * halfW;
        i32 cellD = halfW * halfW + halfH * halfH;
        float cellR = static_cast<float>(sqrt(static_cast<double>(cellD))) + radius - g_fxBias;
        if (m_cols <= 0) {
            continue;
        }
        for (i32 col = 0; col < m_cols; col++) {
            i32 d2 = halfH * halfH + cellW2;
            float v = static_cast<float>(sqrt(static_cast<double>(d2)));
            float normX, normY;
            if (v > g_fxEps) {
                normY = static_cast<float>((row - halfH)) / v;
                normX = static_cast<float>((col - halfW)) / v;
            } else {
                normY = 0.0f;
                normX = 1.0f;
            }

            RECT pt48;
            pt48.left = 0;
            pt48.top = 0;
            pt48.right = dx;
            pt48.bottom = dy;
            OffsetRect(&pt48, row, col);
            i32 ox = static_cast<i32>((cellR * normX));
            i32 oy = static_cast<i32>((cellR * normY));
            OffsetRect(&pt48, oy, ox);

            RECT pt64;
            pt64.left = 0;
            pt64.top = 0;
            pt64.right = d2;
            pt64.bottom = dy;
            OffsetRect(&pt64, row, col);

            RezElem40 elem;
            if (m_recOrderFlag) {
                elem.m_startRect.left = pt64.right;
                elem.m_startRect.top = pt64.bottom;
                elem.m_startRect.right = pt64.left;
                elem.m_startRect.bottom = pt64.top;
                elem.m_endRect = pt48;
            } else {
                elem.m_startRect = pt48;
                elem.m_endRect.left = pt64.right;
                elem.m_endRect.top = pt64.bottom;
                elem.m_endRect.right = pt64.left;
                elem.m_endRect.bottom = pt64.top;
            }
            elem.m_reserved20 = 0;
            elem.m_scale = 1.0f;

            i32 idx = mesh->m_nSize;
            i32 newSize = idx + 1;
            // Reserve raw capacity: unused serialized mesh slots are zero-filled explicitly.
            if (newSize == 0) {
                if (mesh->m_pData) {
                    ::operator delete(mesh->m_pData);
                    mesh->m_pData = NULL;
                }
                mesh->m_nMaxSize = 0;
                mesh->m_nSize = 0;
            } else if (mesh->m_pData == NULL) {
                mesh->m_pData =
                    static_cast<RezElem40*>(::operator new(newSize * sizeof(RezElem40)));
                memset(mesh->m_pData, 0, newSize * sizeof(RezElem40));
                mesh->m_nMaxSize = newSize;
                mesh->m_nSize = newSize;
            } else if (newSize <= mesh->m_nMaxSize) {
                if (newSize > idx) {
                    memset(&mesh->m_pData[idx], 0, (newSize - idx) * sizeof(RezElem40));
                }
                mesh->m_nSize = newSize;
            } else {
                i32 grow = mesh->m_nGrowBy;
                if (grow == 0) {
                    grow = idx / 8;
                    if (grow < 4) {
                        grow = 4;
                    } else if (grow > 0x400) {
                        grow = 0x400;
                    }
                }
                i32 newMax = mesh->m_nMaxSize + grow;

                if (newSize >= newMax) {
                    newMax = newSize;
                }
                RezElem40* nd = static_cast<RezElem40*>(::operator new(newMax * sizeof(RezElem40)));
                memcpy(nd, mesh->m_pData, mesh->m_nSize * sizeof(RezElem40));
                memset(&nd[mesh->m_nSize], 0, (newSize - mesh->m_nSize) * sizeof(RezElem40));
                ::operator delete(mesh->m_pData);
                mesh->m_pData = nd;
                mesh->m_nSize = newSize;
                mesh->m_nMaxSize = newMax;
            }
            memcpy(&mesh->m_pData[idx], &elem, sizeof(RezElem40));
        }
    }
    return 1;
}

// @early-stop
RVA(0x0017f390, 0x164)
void CRezBufferObject::SetSize(i32 nNewSize, i32 nGrowBy) {
    // Reserve raw capacity: MFC-style growth constructs only newly materialized elements.
    if (nGrowBy != -1) {
        m_nGrowBy = nGrowBy;
    }
    if (nNewSize == 0) {
        if (m_pData != NULL) {
            ::operator delete(m_pData);
            m_pData = NULL;
        }
        m_nSize = m_nMaxSize = 0;
    } else if (m_pData == NULL) {
        m_pData = static_cast<RezElem40*>(::operator new(nNewSize * sizeof(RezElem40)));
        memset(m_pData, 0, nNewSize * sizeof(RezElem40));
        m_nSize = m_nMaxSize = nNewSize;
    } else if (nNewSize <= m_nMaxSize) {
        if (nNewSize > m_nSize) {
            memset(&m_pData[m_nSize], 0, (nNewSize - m_nSize) * sizeof(RezElem40));
        }
        m_nSize = nNewSize;
    } else {
        i32 grow = m_nGrowBy;
        if (grow == 0) {
            grow = m_nSize / 8;
            if (grow < 4) {
                grow = 4;
            } else if (grow > 1024) {
                grow = 1024;
            }
        }
        i32 nNewMax;
        if (nNewSize < m_nMaxSize + grow) {
            nNewMax = m_nMaxSize + grow;
        } else {
            nNewMax = nNewSize;
        }
        RezElem40* pNewData = static_cast<RezElem40*>(::operator new(nNewMax * sizeof(RezElem40)));
        memcpy(pNewData, m_pData, m_nSize * sizeof(RezElem40));
        memset(&pNewData[m_nSize], 0, (nNewSize - m_nSize) * sizeof(RezElem40));
        ::operator delete(m_pData);
        m_pData = pNewData;
        m_nSize = nNewSize;
        m_nMaxSize = nNewMax;
    }
}

// @early-stop
RVA(0x0017f5e0, 0x7d)
i32 CFaderFlat::ApplyInit(CFxModeDesc* desc) {
    CFxModeT5* s = static_cast<CFxModeT5*>(desc);
    CDDSurface* a = s->m_targetSurface;
    if (!a) {
        a = m_timerA;
    }
    m_desc04 = a;
    if (s->m_sourceSurface) {
        m_src = s->m_sourceSurface;
    } else {
        m_src = m_timerB;
    }
    m_desc0c = s->m_param0c;
    m_percent = s->m_durationPercent;
    m_previousFrame = 0;
    m_desc14 = s->m_splitPercent;
    m_frames = new i32[m_src->m_height];
    for (i32 i = 0; i < m_src->m_height; i++) {
        m_frames[i] = 0;
    }
    return 1;
}

RVA(0x0017f950, 0x24)
i32 CFaderFlat::GetFrameCount() {
    i32 n = m_src->m_height;
    return n + (m_percent * n) / 100;
}

// @early-stop
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
            float r = static_cast<float>(
                (static_cast<double>(m_maxRadius)
                 - sqrt(static_cast<double>((dx * dx + dy * dy))) * g_faderScale - g_faderBiasR)
            );
            float fade = r / m_fadeDivisor - g_faderBiasFade;
            float vx = static_cast<float>(dx) * fade;
            float vy = static_cast<float>(dy) * fade;
            u8 pix;
            u8* base = static_cast<u8*>(m_srcSurface->Lock(0));
            if (base != NULL) {
                pix = *static_cast<u8*>(
                    (base + m_srcSurface->m_bytesPerPixel * x + m_srcSurface->m_pitch * y)
                );
                m_srcSurface->UnlockThunk();
            } else {
                pix = 0;
            }
            CFaderRadialCell* cell = &m_cells[y * m_srcSurface->m_width + x];
            cell->m_vx = vx;
            cell->m_vy = vy;
            cell->m_fadeLevel = fade;
            cell->m_pixel = pix;
        }
    }
    return 1;
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
        float d = c->m_fadeLevel - static_cast<float>(static_cast<u32>(frame));
        if (d > g_faderOne) {
            float sf = d / m_fadeDivisor - g_faderBiasFade;
            i32 px = m_centerX + static_cast<i32>((c->m_vx / sf));
            i32 py = m_centerY - static_cast<i32>((c->m_vy / sf));
            if (px > 0 && px < m_dstSurface->m_width && py > 0 && py < m_dstSurface->m_height) {
                (base)[py * m_dstSurface->m_pitch + px] = static_cast<u8>(c->m_pixel);
            }
        }
    }

    m_srcSurface->m_ddSurface->Unlock(0);
    m_dstSurface->m_ddSurface->Unlock(0);
    delete[] scratch;
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
RVA(0x001817e0, 0x315)
i32 CFaderShape::ApplyInit(CFxModeDesc* desc) {
    CFxModeT1* pInit = static_cast<CFxModeT1*>(desc);
    i32 i;
    i32 mx;
    m_previousFrame = 0;
    if (pInit == NULL) {
        goto fail;
    }

    m_surfA = pInit->m_targetSurface ? pInit->m_targetSurface : m_timerA;
    m_surfB = pInit->m_sourceSurface ? pInit->m_sourceSurface : m_timerB;
    if (m_surfA == NULL) {
        goto fail;
    }
    if (m_surfB == NULL) {
        goto fail;
    }
    m_surfC = pInit->m_warpSourceSurface ? pInit->m_warpSourceSurface : m_surfB;

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
        colBase = stride
                  - static_cast<i32>(
                      (static_cast<double>(stride) / (arc - m_halfWidth) * (m_span - col - stride))
                  );
    } else {
        colBase = col;
    }
    if ((m_mode == FADER_SWEEP_FORWARD && m_stripCopy == 0)
        || (m_mode == FADER_SWEEP_REVERSE && m_stripCopy != 0)) {
        colBase = static_cast<i32>((static_cast<double>(stride) / (arc - m_halfWidth) * col));
    }

    if ((m_mode == FADER_SWEEP_FORWARD && m_stripCopy != 0)
        || (m_mode == FADER_SWEEP_REVERSE && m_stripCopy == 0)) {
        i32 col = 0;
        if (m_rowCount > 0) {
            i32 base = bpp * col;
            do {
                u8* dstLine = m_rowOfsA[col] + base + m_dstBase;
                u8* gsrc = m_rowOfsC[col] + base + m_gatherBase;
                u8* ssrc = m_rowOfsB[col] + base + m_straightBase;
                if (m_useLut == 0) {
                    if (bpp == PIXEL8_BYTES_PER_PIXEL) {
                        i32 i = 0;
                        i32 t = colBase;
                        if (colBase > 0) {
                            do {
                                m_lineBuf[i] = ssrc[i];
                                i++;
                            } while (i < colBase);
                        }
                        for (; t < stride; t++) {
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
                } else {
                    u8* lut = m_table->m_data;
                    i32 i = 0;
                    i32 t = colBase;
                    if (colBase > 0) {
                        do {
                            m_lineBuf[i] = ssrc[i];
                            i++;
                        } while (i < colBase);
                    }
                    for (; t < stride; t++) {
                        m_lineBuf[t] =
                            lut[static_cast<u32>(m_shadeRamp[t])
                                + static_cast<u32>(gsrc[m_warpTable[t]]) * 0x40];
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
                    u8* s2 = (col - stripWidth) * bpp + m_rowOfsB[col] + m_straightBase;
                    if (c2 > 0) {
                        do {
                            *dstLine = *s2;
                            dstLine++;
                            s2++;
                            c2--;
                        } while (c2 != 0);
                    }
                }
                col++;
            } while (col < m_rowCount);
        }
    } else if (((m_mode == FADER_SWEEP_FORWARD && m_stripCopy == 0)
                || (m_mode == FADER_SWEEP_REVERSE && m_stripCopy != 0))
               && m_rowCount > 0) {
        i32 col = 0;
        i32 base = bpp * col;
        do {
            u8* dstLine = m_rowOfsA[col] + base + m_dstBase;
            u8* gsrc = m_rowOfsC[col] + base + m_gatherBase;
            u8* ssrc = m_rowOfsB[col] + base + m_straightBase;
            if (m_useLut == 0) {
                if (bpp == PIXEL8_BYTES_PER_PIXEL) {
                    i32 i = 0;
                    i32 t = colBase;
                    i32 e;
                    if (colBase > 0) {
                        do {
                            e = i + 1;
                            m_lineBuf[i] = gsrc[m_warpTable[i]];
                            i = e;
                        } while (e < colBase);
                    }
                    for (; t < stride; t++) {
                        m_lineBuf[t] = ssrc[t];
                    }
                } else if (bpp == PIXEL16_BYTES_PER_PIXEL) {
                    i32 i = 0;
                    i32 t = colBase;
                    if (colBase > 0) {
                        do {
                            i32 o = i * 4;
                            i++;
                            m_lineBuf[i * 2 - 2] = gsrc[m_warpTable[o / 4] * 2];
                            m_lineBuf[i * 2 - 1] = gsrc[m_warpTable[i - 1] * 2 + 1];
                        } while (i < colBase);
                    }
                    for (; t < stride; t++) {
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
            } else {
                u8* lut = m_table->m_data;
                i32 i = 0;
                i32 t = colBase;
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
                for (; t < stride; t++) {
                    m_lineBuf[t] = ssrc[t];
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
                u8* s2 = (col + stride) * bpp + m_rowOfsB[col] + m_straightBase;
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
            col++;
        } while (col < m_rowCount);
    }
}

RVA(0x00182900, 0x35)
i32 CFaderShape::GetFrameCount() {
    FaderMode mode = m_mode;
    if (mode == FADER_SWEEP_FORWARD || mode == FADER_SWEEP_REVERSE) {
        return m_span - m_halfWidth * 2;
    }
    if (mode == FADER_SPLIT_FROM_CENTER) {
        return (m_span - m_halfWidth * 4) / 2;
    }
    return 0;
}

DATA(0x001f080c)
const float g_faderHalfPi = 1.570795f;

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

DATA(0x001f0860)
const float g_sineHalfPi = 1.570795f;
DATA(0x001f0864)
const float g_sineOne = 1.0f;

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
                    i32 pick = FxRand(m_elemCount);
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
                    i32 pick = FxRand(m_elemCount);
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
