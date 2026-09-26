#ifndef DDRAWMGR_PIXELFORMATMACROS_H
#define DDRAWMGR_PIXELFORMATMACROS_H

#include <Ints.h>

#define RGB_TO_16(entry)                                                                           \
    static_cast<u16>(                                                                              \
        ((static_cast<u16>((entry).peRed) >> 3) << 10)                                             \
        | ((static_cast<u16>((entry).peGreen) >> 3) << 5)                                          \
        | (static_cast<u16>((entry).peBlue) >> 3)                                                  \
    )

#endif // DDRAWMGR_PIXELFORMATMACROS_H
