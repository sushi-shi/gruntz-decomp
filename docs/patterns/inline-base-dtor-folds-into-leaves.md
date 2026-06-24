# Inline the trivial base dtor so leaf `??1` destructors FOLD the whole base teardown (and pin the base COMDAT with @rva-symbol)
tags: cpp:dtor cpp:eh cpp:inline cpp:virtual | asm:mov asm:call | topic:codegen-idiom topic:eh
symptoms: a family of identical 0x44-byte /GX leaf dtors that each fold the SAME base teardown inline (store base vptr; `lea ecx,[esi+0x18]; call ~Member`; store grand-base vptr; `mov fs:0`); NONE calls another `??1`; the genuine base dtor ALSO exists standalone
confidence: 8/10
variants: eh-dtor-needs-base-subobject.md, inline-vdtor-for-scalar-deleting.md, gx-frame-destructible-local.md

A leaf class `D : B : ...` whose `~D(){}` is empty and whose base `~B` is trivial:
retail emits `~D` as an out-of-line `??1` that INLINES `~B`'s teardown directly
(store B's vptr, destruct B's destructible member, store the grand-base vptr — the
whole /GX-framed fold), with NO `call ~B`. Reproduce it by defining `~B()` **inline
in the shared header** (`virtual ~B() OVERRIDE {}`); then every leaf `~D` folds it,
exactly as `~B`'s own dtor folds. The /GX EH frame falls out of the real
non-trivial base subobject — the manual-vptr model can't (see
eh-dtor-needs-base-subobject). The leaf must derive from the REAL base (so the
ctor's `call ??0B` and the dtor fold both fall out), NOT a stub `struct DBase`.

The catch: `~B` itself still needs its out-of-line standalone copy (called by B's
`??_G` scalar-deleting dtor) at its own RVA. An inline-defined dtor can't hang an
`RVA()` attribute — clang would tag BOTH `??1B` and the synthesized `??_GB` with
that one address (duplicate-RVA guard fires). Pin the out-of-line COMDAT by mangled
name instead, with an `@rva-symbol` comment.

```cpp
// shared header (MFC-capable TU only — the fold needs the real base):
class B : public Base {
public:
    B(GameObj*);              // out-of-line (leaf ctors `call` it)
    virtual ~B() OVERRIDE {}  // INLINE & trivial -> folds into every leaf ~D
};
class D : public B { public: D(GameObj* o); virtual ~D() OVERRIDE; };  // adds no data

// the .cpp: leaf dtors are independent folds; pin B's out-of-line COMDAT by name
// (an inline dtor can't carry RVA() without also tagging ??_GB):
// @rva-symbol: ??1B@@UAE@XZ 0x00011290 0x44
RVA(0x00011540, 0x44) D::~D() {}            // folds B's teardown inline
RVA(0x0010fa90, 0x19) D::D(GameObj* o) : B(o) {}  // call ??0B; stamp leaf vptr
```
```asm
411540: push -1; push <handler>; mov fs:0,esp   ; /GX frame
        mov esi,ecx
        mov [esi],0x5e705c                       ; store B-base (CUserLogic) vptr
        lea ecx,[esi+0x18]; call ~Member          ; destruct +0x18 link (~EngStr)
        mov [esi],0x5e70b4                       ; store grand-base (CUserBase) vptr
        mov fs:0,ecx; pop esi; add esp,0xc; ret
```
STEERABLE — the inline base dtor is the counter to the eh-dtor-needs-base-subobject
and eh-dtor-vptr-restamp-presence walls (which otherwise cap these at ~50-60% /
~92%). Evidence: CTileTrigger (0x11290) + its three leaves CTileSecretTrigger
(0x11540), CGiantRock (0x11600), CCoveredPowerup (0x116c0) — all four 0x44-byte
dtors 100% once `~CTileTrigger` went inline and the standalone copy was `@rva-symbol`
pinned; the three leaf ctors (0x10fa60/90/c0) also 100% in the same model.
