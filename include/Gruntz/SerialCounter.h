// SerialCounter.h - the serialized-object sequence counter.
//
// g_serialCounter (?g_serialCounter@@3HA @0x229ad0) is the monotonically-increasing id
// the save/load layer stamps onto each serialized object (reset to 0 at the start of a
// serialize pass, bumped per object). Its ONE definition + DATA pin live in
// src/Gruntz/Grunt.cpp; a reference is DATA-reloc-masked. Declared once here (a minimal
// owner header) so the save/load + record TUs stop re-`extern`-ing it per-TU and the
// three headers that used to re-declare it (Grunt.h/MgrSettings.h/InGameIcon.h) share it.
#ifndef INCLUDE_GRUNTZ_SERIALCOUNTER_H
#define INCLUDE_GRUNTZ_SERIALCOUNTER_H
#include <Ints.h>

extern i32 g_serialCounter; // 0x229ad0 (defined in Grunt.cpp)

#endif // INCLUDE_GRUNTZ_SERIALCOUNTER_H
