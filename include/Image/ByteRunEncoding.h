#ifndef IMAGE_BYTERUNENCODING_H
#define IMAGE_BYTERUNENCODING_H

#include <Enums.h>

GZ_ENUM_CONST_BEGIN(ByteRunEncoding)
    BYTE_RUN_CONTROL_MASK = 0xc0,
    BYTE_RUN_MARKER = 0xc0,
    BYTE_RUN_LENGTH_MASK = 0x3f
GZ_ENUM_CONST_END(ByteRunEncoding)

#define DECODE_BYTE_RUN_LINE(dst, src, count, hold, value, run, k, step)                           \
    if ((hold) > 0) {                                                                              \
        for ((k) = 0; (k) < (hold); (k)++) {                                                       \
            *(dst) = (value);                                                                      \
            (dst) += (step);                                                                       \
        }                                                                                          \
        (count) -= (hold);                                                                         \
        (hold) = 0;                                                                                \
    }                                                                                              \
    while ((count) > 0) {                                                                          \
        (value) = *(src);                                                                          \
        (src)++;                                                                                   \
        if (((value) & BYTE_RUN_CONTROL_MASK) == BYTE_RUN_MARKER) {                                \
            (run) = (value) & BYTE_RUN_LENGTH_MASK;                                                \
            (value) = *(src);                                                                      \
            (src)++;                                                                               \
            if ((run) > (count)) {                                                                 \
                (hold) = (run) - (count);                                                          \
                (run) = (count);                                                                   \
            }                                                                                      \
            for ((k) = 0; (k) < (run); (k)++) {                                                    \
                *(dst) = (value);                                                                  \
                (dst) += (step);                                                                   \
            }                                                                                      \
            (count) -= (run);                                                                      \
        } else {                                                                                   \
            *(dst) = (value);                                                                      \
            (dst) += (step);                                                                       \
            (count)--;                                                                             \
        }                                                                                          \
    }

#endif // IMAGE_BYTERUNENCODING_H
