# Enum domains: MSVC 5.0 sizes every enum as `int`, but retyping `i32` -> enum is BYTE-NEUTRAL
tags: cpp:enum cpp:switch | asm:mov asm:cmp | topic:codegen-idiom
symptoms: `case 0x3e8:` / `== 0x36` magic numbers; one value space spelled under several names; a domain stored `u8` in a struct but passed as `i32`; "would an enum move bytes?"
confidence: 10/10

Measured on the real toolchain (VC5.0 SP3, `cl 11.00`, `/O2 /MT`), not assumed.
This is the fact the whole enum-domain layer (`include/Enums.h`) rests on.

## What MSVC 5.0 accepts

| construct | verdict |
|---|---|
| `enum class E { … };` | **rejected** — `error C2236: unexpected 'class' 'E'` |
| `enum E : u8 { … };` (fixed underlying type) | **rejected** — `error C2059: syntax error : ':'` |
| `sizeof(E)` for any `enum E` | **always 4** — for a 1-enumerator enum and for `{ C = 0x7fffffff }` alike |
| `inline E operator|(E, E)` (C++98 operator overload on an enum) | **accepted** |
| `enum E;` — OPAQUE forward declaration, no definition | **accepted** as a member, parameter and return type |

The first two are why the layer needs two expansions at all. The third is why a
1-byte field can never be enum-typed in the matching build — hence
`GZ_ENUM_STORAGE(Domain, u8)`, which stays `u8` in retail.

## Retyping is byte-neutral; only the mangling moves

Compiling the same function twice — once with `i32` members/params/returns and
bare literals, once with a real `enum` domain, named enumerators and an
enum-typed `switch` key — gives **byte-identical `.text`** and identical
relocations:

```
$ llvm-objdump -s --section=.text n_int.obj  > a
$ llvm-objdump -s --section=.text n_enum.obj > b
$ diff a b && echo IDENTICAL          # IDENTICAL (43 instructions)
```

The *only* difference is the mangled name of a signature that changed:

```
?Classify@S@@QAEH H     @Z      i32 S::Classify(i32)
?Classify@S@@QAEH W4Kind@@ @Z   i32 S::Classify(Kind)
```

So: retyping a **member** is free (a 4-byte enum field is layout-identical to
`i32`); retyping a **parameter or return** is free in code bytes but rewrites the
symbol, which flows into `build/gen/symbol_names.csv` -> synth PDB -> delink.
Update the `RVA_COMPGEN` pins in the same commit; `verify_unique_names` is the gate.

## The opaque forward declaration is the butterfly escape hatch

`enum Kind;` with no definition compiles, makes the containing class 4 bytes,
produces **the same mangling** and **the same `.text`** as the complete enum:

```
?Classify@S@@QAEHW4Kind@@@Z     <- from `enum Kind;`      (opaque)
?Classify@S@@QAEHW4Kind@@@Z     <- from `enum Kind {…};`  (complete)
```

This is what lets a header take an enum-typed member, parameter or return
*without* `#include`-ing the domain's definition — which is how the type-application
work avoids the documented "adding an include perturbs /O2 regalloc" butterfly
(`docs/gotchas.md`, header butterfly). Spell it `GZ_ENUM_FORWARD(Kind);` so the
strict build sees `enum class Kind : i32` instead (ISO C++ has no opaque
unscoped enum; the bare form is an MSVC extension and clang rejects it).

Precedent already in the tree: `include/Wap32/Wap32.h:11` declares
`enum GruntzCommand;` and `Wap32.h:147` uses it as a virtual parameter type.

## Real enums type-check in the MATCHING build, not just the strict one

Because gruntz ships no PDB, nothing external pins a declared type, so value
domains can be real `enum`s in the retail branch — unlike `homm2-decomp`, whose
CodeView stream forced `typedef i32` on every domain. That buys two of the three
classes of domain bug from `cl` itself:

```
m_neg.cpp(23) : error C2664: 'takesKind' : cannot convert parameter 1
                             from 'enum CmdKind' to 'enum Kind'     <- wrong domain
m_neg.cpp(24) : error C2440: '=' : cannot convert from 'int' to 'enum Kind'
                                                                    <- raw int into a domain
```

Only "domain used as a raw array index" needs the strict C++20 pass (`array
subscript is not an integer`). Note what still compiles silently in retail and
must not be mistaken for safety: `enum == int` comparisons and `case 0x36:` on an
enum key both promote, so the *reads* are unchecked. The strict build is what
closes those.

## A BIASED switch key can be un-biased for free — cl 5.0 normalises it

`switch (typeCode - 1)` / `switch (type - 0x33)` is how a biased dispatch reads
after transcription, and it forces every label to be spelled as an offset, which
is exactly what keeps a domain unnamed. Rewriting to the natural

```cpp
switch (typeCode) { case TILEKIND_SWITCH_A: ... }     // unbiased labels
```

is **byte-identical** — measured on `BrickzCellFlags.cpp` (`llvm-objdump -s
--section=.text`, both objs equal). cl 5.0 folds the constant bias into the
jump-table base itself, so the subtraction never existed as an instruction. Do
the rewrite; it is the difference between 100 magic labels and a named domain.

This does NOT extend to a bias the target actually computes. `switch (IDX(x) -
IDX(PICKUP_BABYWALKER))` was measured the other way — there the bias IS
load-bearing and appears in the disasm. Measure, do not assume, and record the
measurement at the site.

## When the SDK already owns the domain, do not declare one

`WM_*`, `VK_*`, `IDOK`/`IDCANCEL`, `SC_*`, `LBN_*`, `MM_MCINOTIFY` are Windows'
domains, not ours: use the SDK spelling and add no header of our own. Letters and
digits keep character literals (`'Y'`, `'1'`) because the SDK deliberately
defines no `VK_A`. These are usually **free of the include butterfly** — the TU
already reaches `<Mfc.h>`/`<Win32.h>` transitively, so nothing new is included.

Such a switch is often self-verifying, which is why it needs no separate
evidence hunt: `GameWindowProc`'s arms name their own ids (`0x000f` -> `OnPaint`,
`0x001c` -> `OnActivateApp`), and `GameKeyHandler` dispatches the numpad twice,
NumLock-off and on, so the two halves cross-check each other value for value.

## Already-typed switches: let the compiler name the labels

Once a switch key IS an enum, each integer label has exactly one correct
enumerator and nothing needs inferring. `python -m gruntz.audit.enum_case_labels`
finds those via libclang and `--apply` rewrites them; it refuses any value with
alias enumerators rather than pick a reading. It reports 0 today. Treat a green 0
as a claim, not a result — the tool was verified by injecting a known positive
(`docs/gotchas.md`, "a green 0 is a claim to verify"); its first version reported
0 only because libclang silently discarded the MSVC-style compdb flags
(`--driver-mode=cl` is mandatory).

## Band and count markers: never compare against a member

A bound written against whichever member happens to sit at the edge —
`n > PICKUP_WINGZ` for "not an equippable tool" — reads as a fact about Wingz
when it is a fact about the band. Every domain that gets range-tested declares
markers instead, and the test names the marker:

| suffix | meaning | use |
|---|---|---|
| `_BEGIN` | first value of a band, INCLUSIVE | `x >= B_BEGIN` |
| `_END` | one PAST the last, exclusive | `x < B_END`, `x >= B_END` |
| `_LAST` | the last value, INCLUSIVE | `x > B_LAST` — only where retail's compare needs the inclusive value |
| `_COUNT` | how many values the domain has | bounds, iteration, table sizes |

`_LAST` exists only because **the compare form is load-bearing**. `> 22` and
`>= 23` are the same predicate but not the same instruction:

```
n > K_WINGZ        83 7c 24 04 16   cmpl $0x16, 0x4(%esp) / 7e  jle
n >= K_BABYWALKER  83 7c 24 04 17   cmpl $0x17, 0x4(%esp) / 7c  jl
```

So a marker must be declared **at the value retail actually compares against**,
and rewriting `> LAST` into `>= END` to tidy it up moves bytes. PickupType
carries both (`PICKUP_EQUIPPABLE_LAST` = 22, `PICKUP_EQUIPPABLE_END` = 23)
because retail spells the same test both ways in different functions. Naming them
is free; normalising them is not.

`_COUNT` is the count, which for a 0-based domain also happens to be one past the
last (`TINT_COUNT = 17` over 0..16) and for a 1-based one does not
(`AREA_COUNT = 8` over 1..8, so its bound is `<= AREA_COUNT`). When a domain has
a ring plus an off-ring value, say so with two markers rather than one ambiguous
number — `DIR_RING_COUNT = 8` is the rotate modulus, `DIR_COUNT = 9` counts
`DIR_CENTER` too.

Declare a marker when a site range-tests the domain. A marker with no use site is
an invention like any other.

## Traps

- **Retyping a VIRTUAL's parameter must update every override in the family, and
  `cl` will not tell you.** `OVERRIDE` expands to nothing under MSVC 5.0, so a
  derived declaration whose signature no longer matches silently becomes a NEW
  virtual — the MSVC build stays green. It breaks later, in the label pass, where
  clang compiles with `/DGRUNTZ_EMIT_META` and `OVERRIDE` becomes real `override`:
  `'X' marked 'override' but does not override any member functions`, reported as
  "clang -emit-llvm produced no IR". If a TU that compiles under `cl` suddenly
  contributes no labels after an enum retype, look for a half-updated override
  family first.
- **Never let an `#include` end up above the header guard.** Injecting a domain
  header and then running `include_order --fix` can hoist the whole block above
  `#ifndef` — `cl` tolerates it, the label pass does not.

- **Assignment is the error, comparison is not.** `m_kind = 0x36;` fails to
  compile once `m_kind` is a domain; `if (m_kind == 0x36)` still compiles. Expect
  the conversion work to concentrate at ingest points (file/network/`WM_COMMAND`),
  which is where it belongs — `src/Wap32/GameWnd.cpp:299` is the precedent:
  `HandleCommand(notifyCode, static_cast<GruntzCommand>(cmdId), lParam)`.
- **Flag domains stay integers in retail.** `|`, `&`, and retail's flag-clearing
  subtraction must remain plain arithmetic; an inline operator in a hot header
  perturbs cl 5.0's /Ob1 inline budget. `GZ_ENUM_FLAGS_*` therefore expands to
  `enum { … }; typedef i32 Name;` in retail and only gets operators under strict.
- **Adding the definition header to a TU that lacked it is NOT neutral** — that
  is the regalloc butterfly, and it is unrelated to the enum itself. Prefer
  `GZ_ENUM_FORWARD`.
- Verify enum edits by **object identity** (`llvm-objdump -dr -s -t`), not by `%`:
  a value-identical edit renumbers `$L`/`$T` labels, which objdiff scoring cannot
  see.

related: `include/Enums.h`, `docs/enum-modeling-plan.md`,
`switch-density-byte-index-table-vs-tree.md`, `switch-cases-source-order.md`,
`literal-comparison-form-survives-o2.md`, `header-fwd-decl-count-regalloc-butterfly.md`.
