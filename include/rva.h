// rva.h - RVA/data label macros for the binary-matching pipeline.

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

#define SIZE(bytes)
#define SIZE_UNKNOWN()
#define VTBL(type, addr) GRUNTZ_META("vtbl:" #addr " class:" #type)
#define VTBL2(derived, base, addr)

// @TODO: Match debt. Those macros should be with proper structure/inheritance modeled.
#define VTBL_ABSENT(type) GRUNTZ_META("vtbl-absent class:" #type)
#define DATA(addr) __attribute__((annotate("data:" #addr)))

// @TODO: Most likely match debt.
#define RVA_COMPGEN(addr, size, symbol)
#define DATA_SYMBOL(addr, size, symbol)

#else // MSVC 5.0 (and any other non-clang compiler): compile the labels out.

#define RVA(addr, size)
#define DATA(addr)
#define OVERRIDE

#define SIZE(bytes)
#define SIZE_UNKNOWN()
#define VTBL(type, addr)
#define VTBL2(derived, base, addr)
#define VTBL_ABSENT(type)
#define RVA_COMPGEN(addr, size, symbol)
#define DATA_SYMBOL(addr, size, symbol)

#endif

#endif // GRUNTZ_RVA_H
