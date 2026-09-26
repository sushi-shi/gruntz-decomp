#ifndef GRUNTZ_UTILS_PACKEDREADWRITE_H
#define GRUNTZ_UTILS_PACKEDREADWRITE_H

#include <Ints.h>

#include <string.h>

static inline i16 PeekI16(const char* p) {
    i16 value;
    memcpy(&value, p, sizeof(value));
    return value;
}

static inline i32 PeekI32(const char* p) {
    i32 value;
    memcpy(&value, p, sizeof(value));
    return value;
}

static inline void PokeI16(char* p, i16 v) {
    memcpy(p, &v, sizeof(v));
}

static inline u32 ReadPackedDWORD(const u8* bytes) {
    u32 value;
    memcpy(&value, bytes, sizeof(value));
    return value;
}

#endif // GRUNTZ_UTILS_PACKEDREADWRITE_H
