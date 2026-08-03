#ifndef GRUNTZ_RVA_H
#define GRUNTZ_RVA_H

#include <Ints.h>

#if defined(__clang__) && defined(GRUNTZ_EMIT_META)

#define RVA(addr, size) __attribute__((annotate("rva:" #addr " size:" #size)))

// A body the LINKER placed, not the compiland: an inline/implicit member that every
// TU seeing the class emits as a COMDAT, of which one copy survives - in whichever
// object first referenced it. It has an address like RVA(), but it is a different
// storage class, exactly as DATA() is (an address does not imply membership in this
// TU's .text contribution). Consumers that reason about a compiland's EXTENT - the
// linker-order invariant, the interleave metric - must skip these; objdiff still
// byte-matches them, in the unit the linker actually put them in.
#define COMDAT(addr, size) __attribute__((annotate("rva:" #addr " size:" #size " comdat:1")))

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

#define VTBL_ABSENT(type) GRUNTZ_META("vtbl-absent class:" #type)
#define DATA(addr) __attribute__((annotate("data:" #addr)))

#define RVA_COMPGEN(addr, size, symbol)

#else

#define RVA(addr, size)
#define COMDAT(addr, size)
#define DATA(addr)
#define OVERRIDE

#define SIZE(bytes)
#define SIZE_UNKNOWN()
#define VTBL(type, addr)
#define VTBL2(derived, base, addr)
#define VTBL_ABSENT(type)
#define RVA_COMPGEN(addr, size, symbol)

#endif

#endif // GRUNTZ_RVA_H
