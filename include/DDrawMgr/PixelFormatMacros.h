#ifndef DDRAWMGR_PIXELFORMATMACROS_H
#define DDRAWMGR_PIXELFORMATMACROS_H

#define PIXEL_FORMAT_IS_RGB555                                                                     \
    g_rDown == PIXEL16_RED_DOWN&& g_gDown == RGB555_GREEN_DOWN&& g_bDown                           \
        == PIXEL16_BLUE_DOWN&& g_rUp == RGB555_RED_UP&& g_gUp == PIXEL16_GREEN_UP

#define PACK_PIXEL16(r, g, b)                                                                      \
    static_cast<u16>(                                                                              \
        ((static_cast<u8>((r) >> g_rDown) << g_rUp)                                                \
         | ((static_cast<u8>((g) >> g_gDown) << g_gUp) | static_cast<u8>((b) >> g_bDown)))         \
    )

#define RGB_TO_16(entry)                                                                           \
    static_cast<u16>(                                                                              \
        ((static_cast<u16>((entry).peRed) >> 3) << 10)                                             \
        | ((static_cast<u16>((entry).peGreen) >> 3) << 5)                                          \
        | (static_cast<u16>((entry).peBlue) >> 3)                                                  \
    )

#endif // DDRAWMGR_PIXELFORMATMACROS_H
