#ifndef INCLUDE_GRUNTZ_SERIALCOUNTER_H
#define INCLUDE_GRUNTZ_SERIALCOUNTER_H
#include <Ints.h>

extern i32 g_serialCounter; // 0x229ad0 (defined in Grunt.cpp)

// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---
extern i32 g_serialCounter; // ?g_serialCounter@@3HA   (VA 0x629ad0)

#endif // INCLUDE_GRUNTZ_SERIALCOUNTER_H
