#ifndef GRUNTZ_GRUNTZ_FADERCOPYMACROS_H
#define GRUNTZ_GRUNTZ_FADERCOPYMACROS_H

#define COPY_STRAIGHT8(src, first, last)                                                           \
    do {                                                                                           \
        warpIndex = (first);                                                                       \
        if (warpIndex < (last)) {                                                                  \
            do {                                                                                   \
                m_lineBuf[warpIndex] = (src)[warpIndex];                                           \
                warpIndex++;                                                                       \
            } while (warpIndex < (last));                                                          \
        }                                                                                          \
    } while (0)

#define COPY_WARP8(src, first, last)                                                               \
    do {                                                                                           \
        warpIndex = (first);                                                                       \
        if (warpIndex < (last)) {                                                                  \
            do {                                                                                   \
                m_lineBuf[warpIndex] = (src)[m_warpTable[warpIndex]];                              \
                warpIndex++;                                                                       \
            } while (warpIndex < (last));                                                          \
        }                                                                                          \
    } while (0)

#define COPY_STRAIGHT16(src, first, last)                                                          \
    do {                                                                                           \
        warpIndex = (first);                                                                       \
        if (warpIndex < (last)) {                                                                  \
            do {                                                                                   \
                m_lineBuf[warpIndex * 2] = (src)[warpIndex * 2];                                   \
                m_lineBuf[warpIndex * 2 + 1] = (src)[warpIndex * 2 + 1];                           \
                warpIndex++;                                                                       \
            } while (warpIndex < (last));                                                          \
        }                                                                                          \
    } while (0)

#define COPY_WARP16(src, first, last)                                                              \
    do {                                                                                           \
        warpIndex = (first);                                                                       \
        if (warpIndex < (last)) {                                                                  \
            do {                                                                                   \
                m_lineBuf[warpIndex * 2] = (src)[m_warpTable[warpIndex] * 2];                      \
                m_lineBuf[warpIndex * 2 + 1] = (src)[m_warpTable[warpIndex] * 2 + 1];              \
                warpIndex++;                                                                       \
            } while (warpIndex < (last));                                                          \
        }                                                                                          \
    } while (0)

#define COPY_STRAIGHT24(src, first, last)                                                          \
    do {                                                                                           \
        i32 copyOffset = (first) * 3;                                                              \
        u8* copySource = (src) + copyOffset;                                                       \
        i32 copyCount = (last) - (first);                                                          \
        if (copyCount > 0) {                                                                       \
            do {                                                                                   \
                m_lineBuf[copyOffset] = copySource[0];                                             \
                m_lineBuf[copyOffset + 1] = copySource[1];                                         \
                m_lineBuf[copyOffset + 2] = copySource[2];                                         \
                copySource += 3;                                                                   \
                copyOffset += 3;                                                                   \
                copyCount--;                                                                       \
            } while (copyCount != 0);                                                              \
        }                                                                                          \
    } while (0)

#define COPY_WARP24(src, first, last)                                                              \
    do {                                                                                           \
        warpIndex = (first);                                                                       \
        if (warpIndex < (last)) {                                                                  \
            warpOffset = warpIndex * 3;                                                            \
            do {                                                                                   \
                m_lineBuf[warpOffset] = (src)[m_warpTable[warpIndex] * 3];                         \
                m_lineBuf[warpOffset + 1] = (src)[m_warpTable[warpIndex] * 3 + 1];                 \
                m_lineBuf[warpOffset + 2] = (src)[m_warpTable[warpIndex] * 3 + 2];                 \
                warpIndex++;                                                                       \
                warpOffset += 3;                                                                   \
            } while (warpIndex < (last));                                                          \
        }                                                                                          \
    } while (0)

#define COPY_PIXEL_OP_INNER(op, depth) COPY_##op##depth

#define COPY_PIXEL_OP(op, depth) COPY_PIXEL_OP_INNER(op, depth)

#define COPY_WARP_SEGMENTS(firstOp, firstSrc, secondOp, secondSrc, split, end)                     \
    do {                                                                                           \
        if (bpp == PIXEL8_BYTES_PER_PIXEL) {                                                       \
            COPY_PIXEL_OP(firstOp, 8)(firstSrc, 0, split);                                         \
            COPY_PIXEL_OP(secondOp, 8)(secondSrc, split, end);                                     \
        } else if (bpp == PIXEL16_BYTES_PER_PIXEL) {                                               \
            COPY_PIXEL_OP(firstOp, 16)(firstSrc, 0, split);                                        \
            COPY_PIXEL_OP(secondOp, 16)(secondSrc, split, end);                                    \
        } else if (bpp == PIXEL24_BYTES_PER_PIXEL) {                                               \
            COPY_PIXEL_OP(firstOp, 24)(firstSrc, 0, split);                                        \
            COPY_PIXEL_OP(secondOp, 24)(secondSrc, split, end);                                    \
        }                                                                                          \
    } while (0)

#endif // GRUNTZ_GRUNTZ_FADERCOPYMACROS_H
