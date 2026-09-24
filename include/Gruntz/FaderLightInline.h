#ifndef GRUNTZ_GRUNTZ_FADERLIGHTINLINE_H
#define GRUNTZ_GRUNTZ_FADERLIGHTINLINE_H

#include <Gruntz/FaderSubtypes.h>
#include <Lith/BDefs.h>

#include <math.h>

#define FADER_LIGHT_SPAN_CAPACITY 1024

#define FADER_MAX0(v) ((v) < 0 ? 0 : (v))

#define FADER_CLAMPW(v, w) (FADER_MAX0(v) < (w) ? FADER_MAX0(v) : (w))

inline void CFaderLight::ComputeSpan(i32 row, i32 radiusSq, i32 edgeOffset, i32& right, i32& left) {
    i32 dy = row - m_center.m_y;
    i32 dx = -static_cast<i32>(sqrt(static_cast<double>(radiusSq - dy * dy)));
    right = FADER_CLAMPW(m_center.m_x - dx, m_width);
    i32 x = dx + m_center.m_x + edgeOffset;
    left = (x < 0) ? 0 : x;
    if (left >= m_width) {
        left = m_width;
    }
}

#define FADER_SHADE_PIXEL(dst, src, lut, count, column)                                            \
    do {                                                                                           \
        i32 pixel = *(src);                                                                        \
        *(dst) = (lut)[pixel * (count) + (column)];                                                \
    } while (0)

#define FADER_DISTANCE(x, center, dySquared)                                                       \
    static_cast<i32>(sqrt(static_cast<double>(((x) - (center)) * ((x) - (center)) + (dySquared))))

RVA(0x00180fb0, 0x534)

inline void
CFaderLight::Render(i32 row0, i32 radiusSq, i32 radius, u8* lut, u8* srcBits, u8* dstBits) {
    if (m_spanCount <= 0) {
        return;
    }
    i32 centerRow = m_center.m_y;
    Coord radialOffset(0, row0 - centerRow);
    radialOffset.m_x =
        -static_cast<i32>(sqrt(static_cast<double>(radiusSq - SQR(radialOffset.m_y)))) + 1;
    i32 row = m_center.m_x + radialOffset.m_x;
    i32 len = radialOffset.Mag();

    i32 srcCol = row0 * m_targetSurface->m_apiDesc.lPitch;
    u8* rowLsrc = srcBits + row + srcCol;
    i32 dstCol = row0 * m_restoreSurface->m_apiDesc.lPitch;
    u8* rowLdst = dstBits + row + dstCol;
    u8* rowRsrc = srcBits - row;
    rowRsrc += srcCol;
    rowRsrc += 2 * m_center.m_x;
    u8* rowRdst = dstBits - row;
    rowRdst += dstCol;
    rowRdst += 2 * m_center.m_x;

    i32 mid = m_height / 2;
    i32 mirSrc;
    i32 mirDst;
    if (centerRow >= mid && row0 <= centerRow) {
        i32 mirCol = 2 * (centerRow - row0);
        if (mirCol + row0 < m_height) {

            mirSrc = mirCol * m_targetSurface->m_apiDesc.lPitch;
            mirDst = mirCol * m_restoreSurface->m_apiDesc.lPitch;
            while (len >= radius - m_spanCount) {
                if (row > m_center.m_x) {
                    return;
                }
                i32 cl = len - radius + m_spanCount;
                if (row >= 0) {
                    FADER_SHADE_PIXEL(rowLsrc, rowLdst, lut, m_spanCount, cl);
                    FADER_SHADE_PIXEL((rowLsrc + mirSrc), (rowLdst + mirDst), lut, m_spanCount, cl);
                }
                rowLsrc++;
                rowLdst++;
                if (2 * m_center.m_x - row < m_width) {
                    FADER_SHADE_PIXEL(rowRsrc, rowRdst, lut, m_spanCount, cl);
                    FADER_SHADE_PIXEL((rowRsrc + mirSrc), (rowRdst + mirDst), lut, m_spanCount, cl);
                }
                rowRsrc--;
                rowRdst--;
                row++;
                len = Coord(row - m_center.m_x, radialOffset.m_y).Mag();
            }
            return;
        }

        while (len >= radius - m_spanCount) {
            if (row > m_center.m_x) {
                return;
            }
            i32 cl = len - radius + m_spanCount;
            if (row >= 0) {
                FADER_SHADE_PIXEL(rowLsrc, rowLdst, lut, m_spanCount, cl);
            }
            rowLsrc++;
            rowLdst++;
            if (2 * m_center.m_x - row < m_width) {
                FADER_SHADE_PIXEL(rowRsrc, rowRdst, lut, m_spanCount, cl);
            }
            rowRsrc--;
            rowRdst--;
            row++;
            len = Coord(row - m_center.m_x, radialOffset.m_y).Mag();
        }
        return;
    }

    if (centerRow >= mid || row0 < centerRow) {
        return;
    }

    i32 mirCol = 2 * radialOffset.m_y;
    if (row0 - mirCol >= 0) {
        mirSrc = mirCol * m_targetSurface->m_apiDesc.lPitch;
        mirDst = mirCol * m_restoreSurface->m_apiDesc.lPitch;
        while (len >= radius - m_spanCount) {
            if (row > m_center.m_x) {
                return;
            }
            i32 cl = len - radius + m_spanCount;
            if (row >= 0) {
                FADER_SHADE_PIXEL(rowLsrc, rowLdst, lut, m_spanCount, cl);
                FADER_SHADE_PIXEL((rowLsrc - mirSrc), (rowLdst - mirDst), lut, m_spanCount, cl);
            }
            rowLsrc++;
            rowLdst++;
            if (2 * m_center.m_x - row < m_width) {
                FADER_SHADE_PIXEL(rowRsrc, rowRdst, lut, m_spanCount, cl);
                FADER_SHADE_PIXEL((rowRsrc - mirSrc), (rowRdst - mirDst), lut, m_spanCount, cl);
            }
            rowRsrc--;
            rowRdst--;
            row++;
            len = Coord(row - m_center.m_x, radialOffset.m_y).Mag();
        }
    } else {
        while (len >= radius - m_spanCount) {
            if (row > m_center.m_x) {
                return;
            }
            i32 cl = len - radius + m_spanCount;
            if (row >= 0) {
                FADER_SHADE_PIXEL(rowLsrc, rowLdst, lut, m_spanCount, cl);
            }
            rowLsrc++;
            rowLdst++;
            if (2 * m_center.m_x - row < m_width) {
                FADER_SHADE_PIXEL(rowRsrc, rowRdst, lut, m_spanCount, cl);
            }
            rowRsrc--;
            rowRdst--;
            row++;
            len = Coord(row - m_center.m_x, radialOffset.m_y).Mag();
        }
    }
}

#endif // GRUNTZ_GRUNTZ_FADERLIGHTINLINE_H
