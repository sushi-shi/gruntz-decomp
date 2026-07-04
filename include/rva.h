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
//   SIZE(type, bytes)   - at file scope, preferably ATOP a class, record the
//                         class's retail byte size. A clang-only `annotate`
//                         carrier (see the class-metadata note below); MSVC sees
//                         NOTHING. Consumed by gruntz.match.class_sizes.
//   SIZE_UNKNOWN(type)  - the tracking sibling of SIZE for a class whose retail
//                         byte size is not yet pinned: marks it size-annotated
//                         (value TBD) so the completeness check counts it, with NO
//                         exact size. Every class carries SIZE(..) xor
//                         SIZE_UNKNOWN(..).
//   VTBL(type, addr)    - atop a class, bind ??_7<type>@@6B@ at retail RVA `addr`
//                         as a DATA symbol (vtable catalog). labels.py text-scans
//                         it tree-wide -> symbol_names.csv. Matching-NEUTRAL
//                         tracking (a vtable name is reloc-masked in objdiff), not
//                         a match lever.
//
// IMPORTANT - the same source is compiled by clang (the label step) AND by MSVC
// 5.0 under wine (the base objs). MSVC 5.0 predates __attribute__, [[...]], AND
// C99 variadic macros (__VA_ARGS__), so each macro must be FIXED-arity and must
// compile to nothing under any non-clang compiler. Under MSVC ALL of these macros
// (RVA/RVAU/SYMBOL/DATA/OVERRIDE and now SIZE/SIZE_UNKNOWN/VTBL) vanish - they are
// purely clang-side labels and never perturb matched code. (SIZE/SIZE_UNKNOWN/VTBL
// were once a negative-size typedef that FORCED sizeof under MSVC and rescheduled
// includers' /O2 codegen; they are now MSVC-empty, so they are matching-neutral
// wherever placed - in particular directly atop a class in a hot header.)
//
// IR caveat (MEASURED): clang only emits an annotation into
// @llvm.global.annotations for a DEFINED, live global. (1) An `extern`
// declaration's annotation is dropped even when the global is referenced, and
// `used` does NOT rescue a declaration - so DATA labels are NOT read from IR;
// labels.py scans the DATA(...) macro in source text and binds it to the clang-AST
// VarDecl mangledName. (2) An annotation on a CLASS/RECORD is dropped too. (3) A
// DEFINED global (even an unused `static`) reaches the IR IFF it is kept alive -
// hence SIZE/SIZE_UNKNOWN/VTBL use a `used` static carrier (they DO reach the IR).
// The carriers are named `gruntz_clsmeta_*`; labels.py's AST DATA scan skips that
// prefix so a carrier can never steal a DATA(...) binding. Functions/SYMBOL go
// through IR; VTBL is still text-scanned (tree-wide / include-independent).
#ifndef GRUNTZ_RVA_H
#define GRUNTZ_RVA_H

// Fixed-width integer aliases (i8/u8/.../i64/u64). rva.h is included by every
// source/header here, so this makes the aliases visible tree-wide. Additive
// (typedefs only) -> matching-neutral.
#include <Ints.h>

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

// SIZE / SIZE_UNKNOWN / VTBL - class-metadata annotations written at file scope,
// preferably DIRECTLY ATOP the class definition (uniform with RVA/DATA). Under
// clang each expands to a `static` CARRIER variable bearing an `annotate`
// attribute that encodes {class name, byte size or vtable rva}. clang DROPS an
// annotate placed on a class/record from @llvm.global.annotations (MEASURED), but
// a GLOBAL variable's annotate DOES reach the IR - hence the carrier - and `used`
// keeps the otherwise-unreferenced carrier (and its annotation) from being
// eliminated before it is emitted. The carrier name is __COUNTER__-uniquified so
// many annotations in one TU never clash. EVERY call site ends in `;` (the
// existing statement-form sites and new atop-the-class ones alike), which
// completes the carrier declaration. The carrier is CLANG-ONLY (the MSVC branch
// is empty), so it never reaches a matched base obj -> matching-neutral wherever
// placed. `type` is only stringized (not required to be a complete type here);
// gruntz.match.class_sizes / .class_vtables enforce completeness + coverage.
#define GRUNTZ_META_CAT_(a, b) a##b
#define GRUNTZ_META_CAT(a, b) GRUNTZ_META_CAT_(a, b)
#define GRUNTZ_META(str)                                                                           \
    static char __attribute__((annotate(str), used)) GRUNTZ_META_CAT(                              \
        gruntz_clsmeta_,                                                                           \
        __COUNTER__                                                                                \
    ) = 0
// SIZE(type, bytes)  - the class's exact retail byte size.
#define SIZE(type, bytes) GRUNTZ_META("size:" #bytes " class:" #type)
// SIZE_UNKNOWN(type) - the size-tracking sibling for a class whose exact retail
// byte size is not yet pinned (records "size-annotated, value TBD" so the
// completeness check counts it). Every class carries SIZE(..) xor SIZE_UNKNOWN(..).
#define SIZE_UNKNOWN(type) GRUNTZ_META("size:unknown class:" #type)
// VTBL(type, addr) - bind the class's virtual-table symbol ??_7<type>@@6B@ at the
// retail RVA `addr` as a DATA symbol: the single source of truth for the vtable
// catalog. labels.py TEXT-SCANS this macro tree-wide (src/ + include/) and emits
// the ??_7 row into build/gen/symbol_names.csv (a TARGET-side name - the EXE has
// no debug symbols, so the delinked datum's name is ours to assign). A vtable
// NAME is reloc-masked in objdiff, so this is matching-neutral TRACKING, not a
// match lever. Only simple global-namespace class names lower cleanly to
// ??_7<type>@@6B@ - templated/namespaced vtables keep using config/vtable_names.csv
// or a `// @data-symbol:` label. (The carrier's annotate also reaches the IR, so a
// future sweep could read VTBL from @llvm.global.annotations; the text scan is
// retained because it is tree-wide / include-independent - see labels.py.)
#define VTBL(type, addr) GRUNTZ_META("vtbl:" #addr " class:" #type)

#else // MSVC 5.0 (and any other non-clang compiler): compile the labels out.

#define RVA(addr, size)
#define RVAU(addr)
#define SYMBOL(mangled)
#define DATA(addr)
#define OVERRIDE

// SIZE / SIZE_UNKNOWN / VTBL expand to NOTHING under MSVC 5.0, exactly like
// RVA/DATA. The class-metadata is a clang-only annotation; MSVC must see zero of
// it. The call-site trailing `;` becomes a stray file-scope semicolon, which
// MSVC 5.0 tolerates. Emitting nothing is the WHOLE POINT: the old negative-size
// `typedef char[sizeof(type)?1:-1]` FORCED sizeof and rescheduled an includer's
// /O2 codegen (measured: CGrunt::StepCompassMove -0.52% when placed in a hot
// header), so it could not be put atop a class. An empty macro is neutral
// wherever it is placed.
#define SIZE(type, bytes)
#define SIZE_UNKNOWN(type)
#define VTBL(type, addr)

#endif

#endif // GRUNTZ_RVA_H
