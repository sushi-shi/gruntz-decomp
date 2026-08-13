#ifndef GRUNTZ_RVA_H
#define GRUNTZ_RVA_H

#include <Ints.h>

#if defined(__clang__) && defined(GRUNTZ_EMIT_META)

#define RVA(addr, size) __attribute__((annotate("rva:" #addr " size:" #size)))

#define OVERRIDE override

#define GRUNTZ_META_CAT_(a, b) a##b
#define GRUNTZ_META_CAT(a, b) GRUNTZ_META_CAT_(a, b)
#define GRUNTZ_META(str)                                                                           \
    static char __attribute__((annotate(str), used)) GRUNTZ_META_CAT(                              \
        gruntz_clsmeta_,                                                                           \
        __COUNTER__                                                                                \
    ) = 0

#define VTBL_ABSENT(type) GRUNTZ_META("vtbl-absent class:" #type)
#define DATA(addr) __attribute__((annotate("data:" #addr)))

#define RVA_COMPGEN(addr, size, symbol)
#define RVA_DYNINIT(addr, size, owner)
#define DATA_COMPGEN(addr, value) value

#else

#define RVA(addr, size)
#define DATA(addr)
#define OVERRIDE

#define VTBL_ABSENT(type)
#define RVA_COMPGEN(addr, size, symbol)
#define RVA_DYNINIT(addr, size, owner)
#define DATA_COMPGEN(addr, value) value

#endif

#endif // GRUNTZ_RVA_H
