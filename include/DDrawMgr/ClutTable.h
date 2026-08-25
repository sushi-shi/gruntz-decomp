#ifndef DDRAWMGR_CLUTTABLE_H
#define DDRAWMGR_CLUTTABLE_H

#include <Enums.h>
#include <Ints.h>

GZ_ENUM_CONST_BEGIN(ClutTableLayout)
    CLUT_CHANNEL_ENTRY_COUNT = 0x8000,
    CLUT_CHANNEL_COUNT = 3,
    CLUT_ENTRY_COUNT = CLUT_CHANNEL_COUNT * CLUT_CHANNEL_ENTRY_COUNT,
    CLUT_GREEN_OFFSET = 0,
    CLUT_BLUE_OFFSET = CLUT_CHANNEL_ENTRY_COUNT,
    CLUT_RED_OFFSET = CLUT_CHANNEL_ENTRY_COUNT * 2,
    CLUT_ALPHA_BANK_ENTRY_COUNT = 0x400
GZ_ENUM_CONST_END(ClutTableLayout)

extern u16 g_clut[CLUT_ENTRY_COUNT];

union ClutByteCursor {
    u8* m_bytes;
    u16* m_words;
};

static inline u16* ClutAtByteOffset(u32 byteOffset) {
    ClutByteCursor cursor;
    cursor.m_words = g_clut;
    cursor.m_bytes += byteOffset;
    return cursor.m_words;
}

#endif // DDRAWMGR_CLUTTABLE_H
