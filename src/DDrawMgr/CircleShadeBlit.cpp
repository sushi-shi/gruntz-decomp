#include <rva.h>

#include <DDrawMgr/DDSurface.h>
#include <Gruntz/FaderSubtypes.h>
#include <Ints.h>

#include <math.h>

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
