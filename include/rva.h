// rva.h - RVA/data label macros for the binary-matching pipeline.
//
// Matched code/data carries its retail address as a clang `annotate` ATTRIBUTE
// instead of a `// @address:` comment. labels.py compiles each TU to LLVM IR and
// reads @llvm.global.annotations - which pairs the MANGLED SYMBOL directly with
// the annotation string - so there is no fragile "nearest definition below the
// comment" positional join (an inline header def can no longer steal an address).
//
//   RVA(addr, size)     - on a matched FUNCTION definition. `size` is the retail
//                         byte extent; synth_pdb uses it as the authoritative
//                         boundary for functions Ghidra never recovered as
//                         objects.
//   RVAU(addr)          - the rare matched function with NO known size ("U" =
//                         unsized); the functions.csv boundary is used instead.
//   SYMBOL(mangled)     - explicit mangled-name override for a function whose
//                         clang MS-mangling differs from the retail symbol.
//   DATA(addr)          - on an `extern` declaration of a matched GLOBAL (the
//                         DATA symbol it is referenced through).
//   SIZE(type, bytes)   - at file scope right AFTER a class, assert sizeof(type)
//                         == bytes. UNLIKE the labels above this is a REAL
//                         compile-time check, active under BOTH clang and MSVC
//                         5.0 (whose sizeof is the matching ground truth). clang
//                         uses static_assert (clear diagnostic); MSVC 5.0 predates
//                         it, so there a negative-size typedef, __LINE__-uniquified
//                         so many SIZE()s coalesce into one TU (All.cpp) without a
//                         clash. Emits no code -> matching-neutral. (cf. vostok's
//                         STATIC_SIZE_ASSERT.)
//
// IMPORTANT - the same source is compiled by clang (the label step) AND by MSVC
// 5.0 under wine (the base objs). MSVC 5.0 predates __attribute__, [[...]], AND
// C99 variadic macros (__VA_ARGS__), so each macro must be FIXED-arity and must
// compile to nothing under any non-clang compiler. Under MSVC the attributes
// vanish, so they are purely clang-side labels and never perturb matched code.
// (SIZE is the one deliberate exception: a real sizeof assert that IS active
// under MSVC too - it still emits no code, so it stays matching-neutral.)
//
// IR caveat (MEASURED): clang only emits an annotation into
// @llvm.global.annotations for a DEFINED global; an `extern` declaration's
// annotation is dropped from IR even when the global is referenced, and `used`
// does NOT rescue it. So DATA labels are NOT read from IR - labels.py scans the
// DATA(...) macro in the source text and binds it to the clang-AST VarDecl
// mangledName (the same non-fragile variable join used before, keyed off the
// macro instead of a `// @data:` comment). Functions/SYMBOL go through IR.
#ifndef GRUNTZ_RVA_H
#define GRUNTZ_RVA_H

#ifdef __clang__

#define RVA(addr, size) __attribute__((annotate("rva:" #addr " size:" #size)))
#define RVAU(addr) __attribute__((annotate("rva:" #addr)))
#define SYMBOL(mangled) __attribute__((annotate("symbol:" #mangled)))
// DATA is text-scanned (see header note); the attribute is still emitted so the
// annotation is self-documenting in the AST. `used` is harmless on a declaration
// (and ignored by clang there, which is exactly why DATA cannot ride the IR).
#define DATA(addr) __attribute__((annotate("data:" #addr)))

// OVERRIDE - a compile-time-only check (emits NO code) that a method really
// overrides a base virtual. clang enforces it (catches a wrong slot/signature in
// the IR/label pass); MSVC 5.0 predates C++11 and has no `override`, so there it
// compiles to nothing. Use it when modeling a matched class's vtable.
#define OVERRIDE override

// SIZE (see header note) - clang gets static_assert for a clear diagnostic.
#define SIZE(type, bytes) static_assert(sizeof(type) == (bytes), "sizeof(" #type ") != " #bytes)

#else // MSVC 5.0 (and any other non-clang compiler): compile the labels out.

#define RVA(addr, size)
#define RVAU(addr)
#define SYMBOL(mangled)
#define DATA(addr)
#define OVERRIDE

// MSVC 5.0 has no static_assert: classic negative-size typedef, name uniquified
// by __LINE__ so aggregating many SIZE()s into one TU (All.cpp) cannot clash.
#define GRUNTZ_SIZE_CAT_(a, b) a##b
#define GRUNTZ_SIZE_CAT(a, b) GRUNTZ_SIZE_CAT_(a, b)
#define SIZE(type, bytes)                                                                          \
    typedef char GRUNTZ_SIZE_CAT(gruntz_size_check_, __LINE__)[(sizeof(type) == (bytes)) ? 1 : -1]

#endif

#endif // GRUNTZ_RVA_H
