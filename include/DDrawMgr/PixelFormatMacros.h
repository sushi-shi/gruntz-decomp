#ifndef DDRAWMGR_PIXELFORMATMACROS_H
#define DDRAWMGR_PIXELFORMATMACROS_H

#define PIXEL_FORMAT_IS_RGB555                                                                     \
    g_rDown == PIXEL16_RED_DOWN&& g_gDown == RGB555_GREEN_DOWN&& g_bDown                           \
        == PIXEL16_BLUE_DOWN&& g_rUp == RGB555_RED_UP&& g_gUp == PIXEL16_GREEN_UP

#define PIXEL_FORMAT_IS_RGB555_FROM_RED_DOWN(redDown)                                              \
    redDown == PIXEL16_RED_DOWN&& g_gDown == redDown&& g_bDown == redDown&& g_rUp                  \
        == RGB555_RED_UP&& g_gUp == PIXEL16_GREEN_UP

#define PACK_PIXEL16_BY_VALUE_RGB(red, green, blue)                                                \
    static_cast<u16>(                                                                              \
        ((static_cast<u8>(red >> g_rDown) << g_rUp) | (static_cast<u8>(green >> g_gDown) << g_gUp) \
         | static_cast<u8>(blue >> g_bDown))                                                       \
    )

#define PACK_PIXEL16_BY_VALUE_RGB_NESTED(red, green, blue)                                         \
    static_cast<u16>(                                                                              \
        ((static_cast<u8>(red >> g_rDown) << g_rUp)                                                \
         | ((static_cast<u8>(green >> g_gDown) << g_gUp) | static_cast<u8>(blue >> g_bDown)))      \
    )

#define PACK_PIXEL16_CAST_RGB(red, green, blue)                                                    \
    static_cast<u16>(                                                                              \
        ((static_cast<u8>((static_cast<u8>(red) >> static_cast<u8>(g_rDown))) << g_rUp)            \
         | (static_cast<u8>((static_cast<u8>(green) >> static_cast<u8>(g_gDown))) << g_gUp)        \
         | static_cast<u8>((static_cast<u8>(blue) >> static_cast<u8>(g_bDown))))                   \
    )

#define PACK_PIXEL16_CAST_GRB(red, green, blue)                                                    \
    static_cast<u16>(                                                                              \
        ((static_cast<u8>((static_cast<u8>(green) >> static_cast<u8>(g_gDown))) << g_gUp)          \
         | (static_cast<u8>((static_cast<u8>(red) >> static_cast<u8>(g_rDown))) << g_rUp)          \
         | static_cast<u8>((static_cast<u8>(blue) >> static_cast<u8>(g_bDown))))                   \
    )

#define PACK_PIXEL16_U16_GRB(red, green, blue)                                                     \
    static_cast<u16>(                                                                              \
        ((static_cast<u16>(static_cast<u8>(green) >> g_gDown) << g_gUp)                            \
         | (static_cast<u16>(static_cast<u8>(red) >> g_rDown) << g_rUp)                            \
         | static_cast<u16>(static_cast<u8>(blue) >> g_bDown))                                     \
    )

#define PACK_PIXEL16_SHIFT_CAST_RGB(red, green, blue)                                              \
    static_cast<u16>(                                                                              \
        ((static_cast<u8>((red >> static_cast<u8>(g_rDown))) << g_rUp)                             \
         | (static_cast<u8>((green >> static_cast<u8>(g_gDown))) << g_gUp)                         \
         | static_cast<u8>((blue >> static_cast<u8>(g_bDown))))                                    \
    )

#define PACK_PIXEL16_GRB_TO(dst, value, green, red, blue)                                          \
    u16 value = static_cast<u16>((static_cast<u8>((static_cast<u8>(green) >> g_gDown)) << g_gUp)); \
    value =                                                                                        \
        static_cast<u16>((value | (static_cast<u8>((static_cast<u8>(red) >> g_rDown)) << g_rUp))); \
    *dst++ = static_cast<u16>((value | static_cast<u8>((static_cast<u8>(blue) >> g_bDown))));

#define PACK_PIXEL16_RGB_TO(dst, value, red, green, blue)                                          \
    u16 value = static_cast<u16>((static_cast<u8>((static_cast<u8>(red) >> g_rDown)) << g_rUp));   \
    value = static_cast<u16>(                                                                      \
        (value | (static_cast<u8>((static_cast<u8>(green) >> g_gDown)) << g_gUp))                  \
    );                                                                                             \
    *dst++ = static_cast<u16>((value | static_cast<u8>((static_cast<u8>(blue) >> g_bDown))));

#endif // DDRAWMGR_PIXELFORMATMACROS_H
