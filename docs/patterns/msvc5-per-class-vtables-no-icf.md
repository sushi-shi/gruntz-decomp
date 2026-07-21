# MSVC 5.0 emits a distinct vtable per polymorphic class — `RELOC_VTBL` was fake

> **Status (2026-07):** the `RELOC_VTBL` macro is now DELETED — every site has been
> dissolved (to a real per-class vtable, or, for a library COMDAT, a
> `config/library_labels.csv` carve). The toolchain finding below stands; it is kept
> as the proof behind the campaign rule. `RELOC_VTBL(...)` no longer exists in the
> tree, so treat any mention as historical.

**Verdict (empirically confirmed, 2026-07-10):** MSVC 5.0 emits a **separate
`??_7<Class>@@6B@` vtable for every polymorphic class** — the base *and* every
derived class — even when the derived class overrides nothing, is empty, or has a
vtable whose contents would be byte-identical. There is **no vtable sharing and no
COMDAT/ICF folding** across classes. Therefore any reconstruction that points one
class's vtable at *another* class's RVA (`RELOC_VTBL(X, <Y's rva>)`, and the
`L_<rva>` / `S_<rva>` placeholder "shared-vtable" classes) is modeling a fiction and
must be dissolved into a real, distinct per-class vtable.

This is the toolchain proof behind the campaign rule in the working notes; run it
yourself with the commands in "Reproduction" below.

## The experiments

Eight programs compiled with the base profile (`wine cl /nologo /c /O2 /MT`, RTTI
off — MSVC5 default), symbols dumped with `llvm-nm`:

| # | Setup | `??_7` vtables emitted |
|---|-------|------------------------|
| e1 | base w/ **virtual** dtor + virtual `f()`; derived overrides **nothing** | `??_7B` **and** `??_7D` |
| e2 | **non-virtual** dtor + virtual `f()`; derived overrides nothing | `??_7B` **and** `??_7D` |
| e3 | derived overrides `f()` (control) | `??_7B`, `??_7D` |
| e4 | **truly empty** `struct D : B {}` | `??_7B` **and** `??_7D` |
| e5 | **two** derived, neither overrides | `??_7B`, `??_7D1`, `??_7D2` (three distinct) |
| e6 | derived adds a **new** virtual (extends vtable) | `??_7B`, `??_7D` |
| e7 | derived is **never instantiated** (only `new B`) | `??_7B` **only** — no `??_7D` |
| e8 | **abstract** base (pure virtual) + concrete derived | `??_7B`, `??_7D` |

Every polymorphic class that is instantiated gets its own vtable. Two sibling
derived classes produce *three* distinct vtables (e5), never a shared one.

## Why — the mechanism (and why "shared vtable" is UB)

The `.rdata` relocations of e1's two vtables (`llvm-objdump -r`):

```
??_7B  slot 0  ->  ??_GB   (B's scalar-deleting destructor)
??_7D  slot 0  ->  ??_GD   (D's scalar-deleting destructor)
```

With a **virtual destructor**, MSVC places a **per-class `??_G`** (scalar-deleting
destructor) in vtable slot 0. `??_GD` destroys D's members and frees `sizeof(D)`;
`??_GB` does B's. So the two vtables aren't merely separate symbols — they hold
**different content**, so even a compiler *with* `/OPT:ICF` (MSVC5 has none) could
not fold them.

Almost every polymorphic class in a real codebase has or inherits a virtual
destructor, so it gets its own `??_G` → its own vtable. Making class `X` "share"
class `Y`'s vtable would put `??_GY` in `X`'s slot 0 → `delete (X*)p` dispatches to
`Y`'s destructor and frees the wrong object size = **undefined behavior**. That is
the concrete danger behind the placeholder-shared-vtable classes.

Footnotes:
- `/GR` (RTTI on) just adds a `??_7type_info` pointer at vtable[-1]; the per-class
  behavior is unchanged.
- A class's vtable emits **only where its ctor is emitted** (e7): a never-`new`'d
  derived has *no* vtable at all — still not a shared one. When a real class looks
  "vtable-less", it is either never instantiated (no vtable exists) or its vtable is
  un-emitted because a `.cpp`-local view in its ctor-TU shadows the real body (see
  `docs/` / the inline-virtual-shadowing note) — never that it borrows a sibling's.

## Implication for the reconstruction

- Dissolve the `RELOC_VTBL(<Class>, <other-class-rva>)` sites and the `L_<rva>` /
  `S_<rva>` placeholder classes into real, distinct per-class vtables (real C++
  `virtual` + the class's own `VTBL()` at its own RVA).
- If a delinked class seems to lack a vtable, resolve it as an emission problem
  (shadowed inline virtual, or genuinely-uninstantiated) — do **not** re-point it at
  a sibling's RVA.

## Reproduction

`e1` (the key case); the others vary only as the table describes:

```cpp
// e1_virtdtor_nooverride.cpp
struct B { int x; virtual ~B(); virtual int f(); };
struct D : B { int y; };            // overrides NOTHING
B::~B() {} int B::f() { return 1; }
B* mkB() { return new B; }          // force ctor/vtable emission
D* mkD() { return new D; }
```

```sh
# inside `nix develop .#build`
python3 scripts/gruntz/build/cc_wrap.py --out e1.obj --src e1_virtdtor_nooverride.cpp -- /nologo /c /O2 /MT
llvm-nm e1.obj | grep -oE '\?\?_7[A-Za-z0-9]+'           # -> ??_7B AND ??_7D
llvm-objdump -r e1.obj | grep -E '\?\?_(7|G)'            # ??_7B[0]->??_GB, ??_7D[0]->??_GD
```
