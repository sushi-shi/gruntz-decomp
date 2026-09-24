#ifndef GRUNTZ_RVA_H
#define GRUNTZ_RVA_H

#include <Ints.h>

#if defined(__clang__) && defined(GRUNTZ_EMIT_META)

#define RVA(addr, size) __attribute__((annotate("rva:" #addr " size:" #size), used))

#define OVERRIDE override

#define DATA(addr) __attribute__((annotate("data:" #addr)))

// Attach to BEGIN_MESSAGE_MAP: the SDK owns both generated data definitions.
#define DATA_MESSAGE_MAP(map, entries)                                                             \
    __attribute__((annotate("mfc-map:" #map " entries:" #entries)))

#define RVA_COMPGEN(addr, size, symbol)
#define RVA_DYNINIT(addr, size, owner)
#define DATA_COMPGEN(addr, value) value

#else

#define RVA(addr, size)
#define DATA(addr)
#define DATA_MESSAGE_MAP(map, entries)
#define OVERRIDE

#define RVA_COMPGEN(addr, size, symbol)
#define RVA_DYNINIT(addr, size, owner)
#define DATA_COMPGEN(addr, value) value

#endif

#endif // GRUNTZ_RVA_H
